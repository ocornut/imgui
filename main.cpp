#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "AndKitty_Fixer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool g_Initialized = false;
static bool g_ShowMenu = true;

struct {
    float x, y;
    bool down;
} g_RawTouch = {0, 0, false};

// 校准参数 (根据你的截图初始设定)
static float g_ScaleX = 0.594f; 
static float g_ScaleY = 0.473f;
static bool g_SwapXY = false;
static bool g_FlipX = false;
static bool g_FlipY = false;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

// 处理触摸事件
void handle_event(void* event) {
    if (!event) return;
    static auto _getType = (int (*)(void*))dlsym(RTLD_DEFAULT, "AInputEvent_getType");
    static auto _getAction = (int (*)(void*))dlsym(RTLD_DEFAULT, "AMotionEvent_getAction");
    static auto _getX = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getX");
    static auto _getY = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getY");

    if (_getType && _getType(event) == 1) {
        int action = _getAction(event) & 0xff;
        g_RawTouch.x = _getX(event, 0);
        g_RawTouch.y = _getY(event, 0);
        if (action == 0) g_RawTouch.down = true;
        else if (action == 1 || action == 3) g_RawTouch.down = false;
    }
}

int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a, b, c, d, e, f);
    if (res == 0 && f && *f) handle_event(*f);
    return res;
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);

    // 计算映射后的坐标
    float tx = g_RawTouch.x;
    float ty = g_RawTouch.y;

    if (g_SwapXY) { float tmp = tx; tx = ty; ty = tmp; }
    
    float mappedX = tx * g_ScaleX;
    float mappedY = ty * g_ScaleY;
    
    if (g_FlipX) mappedX = (float)w - mappedX;
    if (g_FlipY) mappedY = (float)h - mappedY;

    io.MousePos = ImVec2(mappedX, mappedY);
    io.MouseDown[0] = g_RawTouch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 1. 在背景画一个“触摸指示器” (非常有用的调试工具)
    if (g_RawTouch.down) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(255, 0, 0, 255));
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(io.MousePos.x + 30, io.MousePos.y), IM_COL32(255, 255, 0, 255), "HERE");
    }

    if (g_ShowMenu) {
        ImGui::Begin("AndKitty Touch Calibrator", &g_ShowMenu);
        ImGui::Text("Render: %dx%d", w, h);
        ImGui::Text("Raw: %.1f, %.1f", g_RawTouch.x, g_RawTouch.y);
        ImGui::Text("Mapped: %.1f, %.1f", mappedX, mappedY);
        
        ImGui::Separator();
        ImGui::Checkbox("Swap X/Y (轴翻转)", &g_SwapXY);
        ImGui::Checkbox("Flip X (镜像X)", &g_FlipX);
        ImGui::Checkbox("Flip Y (镜像Y)", &g_FlipY);
        
        ImGui::SliderFloat("Scale X", &g_ScaleX, 0.0f, 3.0f);
        ImGui::SliderFloat("Scale Y", &g_ScaleY, 0.0f, 3.0f);

        if (ImGui::Button("Reset Parameters", ImVec2(-1, 60))) {
            g_ScaleX = 1.0f; g_ScaleY = 1.0f; g_SwapXY = false;
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec2(0, 1, 0), "Step: 滑动滑块，直到红点跟着手指走");
        
        if (ImGui::Button("Close Menu", ImVec2(-1, 60))) { g_ShowMenu = false; }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    sleep(15);
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    const char* sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume = DobbySymbolResolver("libinput.so", sym);
    if (consume) DobbyHook(consume, (void*)hook_consume, (void**)&old_consume);
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, init_thread, nullptr);
}
