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

struct TouchPos { float x, y; };
struct {
    float x, y;
    bool down;
} g_RawTouch = {0, 0, false};

// --- 自动校准存储 ---
static TouchPos g_P1 = {0, 0}; // 窗口左上角在 Raw 坐标系下的位置
static TouchPos g_P2 = {0, 0}; // 窗口右下角在 Raw 坐标系下的位置
static float g_FinalX = 0, g_FinalY = 0;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

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
        // 设置默认值 (基于你之前的截图尝试)
        g_P1 = {600.0f, 200.0f}; 
        g_P2 = {2000.0f, 1200.0f};
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);

    // --- 两点线性映射算法 ---
    // 原理：将 [P1, P2] 的 Raw 范围映射到 [0, DisplaySize] 的渲染范围
    float divX = (g_P2.x - g_P1.x);
    float divY = (g_P2.y - g_P1.y);
    
    if (divX != 0 && divY != 0) {
        g_FinalX = (g_RawTouch.x - g_P1.x) / divX * (float)w;
        g_FinalY = (g_RawTouch.y - g_P1.y) / divY * (float)h;
    }

    io.MousePos = ImVec2(g_FinalX, g_FinalY);
    io.MouseDown[0] = g_RawTouch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 辅助红点
    if (g_RawTouch.down) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(io.MousePos, 25.0f, IM_COL32(255, 0, 0, 255));
    }

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(w * 0.8f, h * 0.8f), ImGuiCond_Once);
        ImGui::Begin("AndKitty SGame Touch Fixer", &g_ShowMenu);
        
        ImGui::Text("Render: %dx%d", w, h);
        ImGui::Text("Raw Touch: %.1f, %.1f", g_RawTouch.x, g_RawTouch.y);
        ImGui::Separator();

        if (ImGui::Button("第一步：点击窗口[左上角]后点我", ImVec2(-1, 80))) {
            g_P1 = {g_RawTouch.x, g_RawTouch.y};
        }
        ImGui::Text("P1(Min): %.1f, %.1f", g_P1.x, g_P1.y);

        ImGui::Spacing();

        if (ImGui::Button("第二步：点击窗口[右下角]后点我", ImVec2(-1, 80))) {
            g_P2 = {g_RawTouch.x, g_RawTouch.y};
        }
        ImGui::Text("P2(Max): %.1f, %.1f", g_P2.x, g_P2.y);

        ImGui::Separator();
        ImGui::TextColored(ImVec2(0,1,0), "Mapped Pos: %.1f, %.1f", g_FinalX, g_FinalY);
        
        if (ImGui::Button("重置校准", ImVec2(200, 50))) {
            g_P1 = {0,0}; g_P2 = {(float)w, (float)h};
        }

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
