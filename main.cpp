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

// 增强型自适应状态存储
struct {
    float x, y;          // 映射后的坐标
    bool down;
    float scaleX = 1.0f; 
    float scaleY = 1.0f;
    float offsetX = 0.0f; // 窗口偏移修正
    float offsetY = 0.0f;
    bool calibrated = false;
    
    // 探测到的原始数据（用于 UI 显示辅助调试）
    float lastRawX, lastRawY;
    float lastLocalX, lastLocalY;
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// 核心自适应处理：双坐标对齐逻辑
void handle_android_event(AInputEvent* event, float renderW, float renderH) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 1. 获取相对窗口的逻辑坐标
        float lx = AMotionEvent_getX(event, 0);
        float ly = AMotionEvent_getY(event, 0);
        
        // 2. 获取屏幕绝对坐标（用于分析偏移）
        static auto _getRawX = (float (*)(const AInputEvent*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getRawX");
        static auto _getRawY = (float (*)(const AInputEvent*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getRawY");
        float rx = _getRawX ? _getRawX(event, 0) : lx;
        float ry = _getRawY ? _getRawY(event, 0) : ly;

        g_Touch.lastLocalX = lx; g_Touch.lastLocalY = ly;
        g_Touch.lastRawX = rx;   g_Touch.lastRawY = ry;

        // 3. 首次点击自动校准逻辑
        if (!g_Touch.calibrated && action == AMOTION_EVENT_ACTION_DOWN) {
            // 针对王者荣耀分屏模式的经验公式
            // 如果逻辑坐标很大（如 > 3000），说明它基于 RequestedSize 3392
            if (lx > renderW || ly > renderH || lx > 2000) {
                g_Touch.scaleX = renderW / 3392.0f; 
                g_Touch.scaleY = renderH / 2400.0f;
            } else {
                g_Touch.scaleX = 1.0f;
                g_Touch.scaleY = 1.0f;
            }
            g_Touch.calibrated = true;
            LOGI("Auto Calibrate: Scale(%.3f, %.3f)", g_Touch.scaleX, g_Touch.scaleY);
        }

        // 4. 计算最终坐标：(原始坐标 + 偏移量) * 缩放
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

    // 辅助圆圈：红点
    if (g_Touch.down) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(io.MousePos, 25.0f, IM_COL32(255, 0, 0, 255));
        ImGui::GetBackgroundDrawList()->AddCircle(io.MousePos, 40.0f, IM_COL32(255, 255, 255, 255), 12, 2.0f);
    }

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(w * 0.8f, 0), ImGuiCond_Once);
        ImGui::Begin("AndKitty Final Fixer", &g_ShowMenu);
        
        ImGui::Text("Render: %.0f x %.0f", io.DisplaySize.x, io.DisplaySize.y);
        ImGui::Text("Local Input: %.1f, %.1f", g_Touch.lastLocalX, g_Touch.lastLocalY);
        ImGui::Text("Mapped ImGui: %.1f, %.1f", g_Touch.x, g_Touch.y);
        
        ImGui::Separator();
        ImGui::Text("Calibration (If red dot is not under finger):");
        
        ImGui::SliderFloat("Scale X", &g_Touch.scaleX, 0.1f, 1.5f, "%.4f");
        ImGui::SliderFloat("Scale Y", &g_Touch.scaleY, 0.1f, 1.5f, "%.4f");
        ImGui::SliderFloat("Offset X (Horizontal)", &g_Touch.offsetX, -500.0f, 500.0f, "%.0f");
        ImGui::SliderFloat("Offset Y (Vertical)", &g_Touch.offsetY, -500.0f, 500.0f, "%.0f");

        if (ImGui::Button("Quick Sync (3392 Mode)", ImVec2(w*0.4f, 80))) {
            g_Touch.scaleX = w / 3392.0f;
            g_Touch.scaleY = h / 2400.0f;
            g_Touch.calibrated = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset All", ImVec2(w*0.4f, 80))) {
            g_Touch.scaleX = 1.0f; g_Touch.scaleY = 1.0f;
            g_Touch.offsetX = 0.0f; g_Touch.offsetY = 0.0f;
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
        LOGI("Input System Hooked.");
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, init_thread, nullptr);
}
