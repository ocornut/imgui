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

// 存储自动校准后的比例和状态
struct {
    float x, y;
    bool down;
    float scaleX = 1.0f; // 默认缩放
    float scaleY = 1.0f;
    bool calibrated = false; // 是否已完成首次校准
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// 核心自适应处理：自动校准逻辑
void handle_android_event(AInputEvent* event, float renderW, float renderH) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 获取逻辑坐标 (系统分发的，可能基于 3392x2400)
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        
        // 如果还没有校准，我们通过 AInputEvent 的内部属性动态计算
        // 逻辑：如果逻辑坐标 x 远大于渲染宽度 renderW，说明需要缩放
        if (!g_Touch.calibrated && action == AMOTION_EVENT_ACTION_DOWN) {
            // 假设系统最大逻辑坐标是容器宽度，我们通过它和当前渲染宽度的比例来对齐
            // 常见的 Android 逻辑单位基准是 3392.0f, 这里我们做动态探测
            if (x > renderW || y > renderH) {
                g_Touch.scaleX = renderW / 3392.0f; // 你的 Requested w 默认值
                g_Touch.scaleY = renderH / 2400.0f; // 你的 Requested h 默认值
            } else {
                g_Touch.scaleX = 1.0f;
                g_Touch.scaleY = 1.0f;
            }
            g_Touch.calibrated = true; 
            LOGI("Auto Calibrated: ScaleX=%.3f, ScaleY=%.3f", g_Touch.scaleX, g_Touch.scaleY);
        }

        // 应用缩放系数，将坐标映射回 ImGui 渲染区域
        g_Touch.x = x * g_Touch.scaleX;
        g_Touch.y = y * g_Touch.scaleY;

        if (action == AMOTION_EVENT_ACTION_DOWN) g_Touch.down = true;
        else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_Touch.down = false;
        
        if (g_Touch.down) {
            LOGI("Touch Event: Logic(%.1f, %.1f) -> Mapped(%.1f, %.1f)", x, y, g_Touch.x, g_Touch.y);
        }
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
        ImGui::GetStyle().ScaleAllSizes(3.0f); // 针对高分屏放大 UI
        g_Initialized = true;
        LOGI("ImGui Init: %d x %d", w, h);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 在触碰点画一个红色指示圆圈，用于验证是否完全对齐
    if (g_Touch.down) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(255, 0, 0, 255));
    }

    if (g_ShowMenu) {
        ImGui::Begin("AndKitty Auto-Adaptive", &g_ShowMenu);
        ImGui::Text("Render Res: %.0f x %.0f", io.DisplaySize.x, io.DisplaySize.y);
        ImGui::Text("Touch Pos: %.1f, %.1f", g_Touch.x, g_Touch.y);
        ImGui::Separator();
        
        if (ImGui::SliderFloat("Manual Scale X", &g_Touch.scaleX, 0.1f, 2.0f)) g_Touch.calibrated = true;
        if (ImGui::SliderFloat("Manual Scale Y", &g_Touch.scaleY, 0.1f, 2.0f)) g_Touch.calibrated = true;
        
        if (ImGui::Button("Reset Calibration", ImVec2(-1, 80))) {
            g_Touch.calibrated = false;
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
        LOGI("InputConsumer Hooked.");
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, init_thread, nullptr);
}
