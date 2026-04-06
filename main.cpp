#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include <android/looper.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool g_Initialized = false;
static bool g_ShowMenu = true;

// 根据 Frida 抓取的 Logic X: 2802 进行硬核对齐
struct {
    float x, y;          
    bool down;
    // 初始值设为根据 Frida 计算的比例 (1426 / 2802 ≈ 0.5089)
    float scaleX = 0.5089f; 
    float scaleY = 0.5089f; // 通常 Y 轴比例与 X 一致
    float offsetX = 0.0f; 
    float offsetY = 0.0f;
    
    float lastLocalX, lastLocalY;
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// 坐标映射处理
void handle_android_event(AInputEvent* event, float renderW, float renderH) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        float lx = AMotionEvent_getX(event, 0);
        float ly = AMotionEvent_getY(event, 0);

        g_Touch.lastLocalX = lx; 
        g_Touch.lastLocalY = ly;

        // 应用 Frida 测出的精确比例和偏移
        g_Touch.x = (lx * g_Touch.scaleX) + g_Touch.offsetX;
        g_Touch.y = (ly * g_Touch.scaleY) + g_Touch.offsetY;

        if (action == AMOTION_EVENT_ACTION_DOWN) g_Touch.down = true;
        else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_Touch.down = false;
    }
}

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a, b, c, d, e, f);
    if (res == 0 && f && *f) {
        handle_android_event((AInputEvent*)*f, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
    }
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
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 绘制调试红点
    if (g_Touch.down) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(255, 0, 0, 255));
        // 十字辅助线
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2(0, g_Touch.y), ImVec2(io.DisplaySize.x, g_Touch.y), IM_COL32(255, 255, 255, 100));
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2(g_Touch.x, 0), ImVec2(g_Touch.x, io.DisplaySize.y), IM_COL32(255, 255, 255, 100));
    }

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(w * 0.8f, 0), ImGuiCond_Once);
        ImGui::Begin("AndKitty Frida-Synced Menu", &g_ShowMenu);
        
        ImGui::Text("Frida Local: %.1f, %.1f", g_Touch.lastLocalX, g_Touch.lastLocalY);
        ImGui::Text("ImGui Mapped: %.1f, %.1f", g_Touch.x, g_Touch.y);

        ImGui::Separator();
        
        // 允许手动微调
        ImGui::SliderFloat("Scale X", &g_Touch.scaleX, 0.4000f, 0.6000f, "%.4f");
        ImGui::SliderFloat("Scale Y", &g_Touch.scaleY, 0.4000f, 1.5000f, "%.4f");
        ImGui::SliderFloat("Offset X", &g_Touch.offsetX, -100.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Offset Y", &g_Touch.offsetY, -100.0f, 100.0f, "%.1f");

        if (ImGui::Button("Sync to Frida Data (2802)", ImVec2(-1, 80))) {
            g_Touch.scaleX = (float)w / 2802.0f;
            g_Touch.scaleY = (float)h / 734.0f; // 这里的 734 是基于你日志最后几行的 Logic Y
        }

        if (ImGui::Button("Close Menu", ImVec2(-1, 80))) g_ShowMenu = false;
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
    if (consume) {
        DobbyHook(consume, (void*)hook_consume, (void**)&old_consume);
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, init_thread, nullptr);
}
