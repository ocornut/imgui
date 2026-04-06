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

struct {
    float x, y;          
    bool down;
    float scaleX = 1.0f; 
    float scaleY = 1.0f;
    float offsetX = 0.0f; 
    float offsetY = 0.0f;
    bool calibrated = false;
    
    float lastLocalX, lastLocalY;
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

void handle_android_event(AInputEvent* event, float renderW, float renderH) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        float lx = AMotionEvent_getX(event, 0);
        float ly = AMotionEvent_getY(event, 0);

        g_Touch.lastLocalX = lx; 
        g_Touch.lastLocalY = ly;

        // 核心修复：改进自动校准逻辑
        // 只有在还没校准过，且点击的坐标明显属于“超大坐标系”时才触发
        if (!g_Touch.calibrated && action == AMOTION_EVENT_ACTION_DOWN) {
            // 如果坐标 > 2000 且 远大于渲染宽度，判定为 3392 模式
            if (lx > renderW && lx > 2000.0f) {
                g_Touch.scaleX = renderW / 3392.0f; 
                g_Touch.scaleY = renderH / 2400.0f;
                g_Touch.calibrated = true;
                LOGI("Auto Sync to 3392 Mode: Scale=%.4f", g_Touch.scaleX);
            } else if (lx > 0) {
                // 如果坐标在渲染范围内，可能是 1:1 模式
                g_Touch.scaleX = 1.0f;
                g_Touch.scaleY = 1.0f;
                g_Touch.calibrated = true;
                LOGI("Auto Sync to 1:1 Mode");
            }
        }

        // 计算最终坐标
        g_Touch.x = (lx + g_Touch.offsetX) * g_Touch.scaleX;
        g_Touch.y = (ly + g_Touch.offsetY) * g_Touch.scaleY;

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

    // 绘制辅助红点
    if (g_Touch.down) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(255, 0, 0, 255));
    }

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(w * 0.8f, 0), ImGuiCond_Once);
        ImGui::Begin("AndKitty Anti-Jump Fix", &g_ShowMenu);
        
        ImGui::Text("Render: %d x %d", w, h);
        ImGui::Text("Input Logic: %.1f, %.1f", g_Touch.lastLocalX, g_Touch.lastLocalY);
        ImGui::Text("Scale: %.4f | Offset: %.1f", g_Touch.scaleX, g_Touch.offsetX);

        ImGui::Separator();
        
        // 允许手动微调，且调整后不再自动校准（除非重置）
        if (ImGui::SliderFloat("Manual Scale X", &g_Touch.scaleX, 0.1f, 1.5f)) g_Touch.calibrated = true;
        if (ImGui::SliderFloat("Manual Offset X", &g_Touch.offsetX, -200.0f, 200.0f)) g_Touch.calibrated = true;
        
        if (ImGui::Button("Reset & Re-Calibrate", ImVec2(-1, 80))) {
            g_Touch.calibrated = false;
            g_Touch.offsetX = 0;
            g_Touch.offsetY = 0;
        }

        if (ImGui::Button("Lock Current Scale", ImVec2(-1, 80))) {
            g_Touch.calibrated = true;
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
        LOGI("Input Fixed.");
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, init_thread, nullptr);
}
