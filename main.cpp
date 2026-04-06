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
    float maxSeenX = 0.0f; // 用于动态探测屏幕逻辑宽度
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

        // 核心修复：动态探测逻辑宽度
        // 如果红点往右偏，通常是因为 scaleX 太大。
        // 我们通过追踪用户点击的最大 X 坐标，来推断系统的 Requested Width
        if (action == AMOTION_EVENT_ACTION_MOVE || action == AMOTION_EVENT_ACTION_DOWN) {
            if (lx > g_Touch.maxSeenX) g_Touch.maxSeenX = lx;
        }

        if (!g_Touch.calibrated && action == AMOTION_EVENT_ACTION_DOWN) {
            // 初始猜测：如果是超大坐标系
            if (lx > renderW && lx > 2000.0f) {
                g_Touch.scaleX = renderW / 3392.0f; 
                g_Touch.scaleY = renderH / 2400.0f;
            } else {
                g_Touch.scaleX = 1.0f;
                g_Touch.scaleY = 1.0f;
            }
            // 注意：这里先不设 calibrated = true，允许动态调整
        }

        // 计算最终坐标
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

    if (g_Touch.down) {
        // 绘制十字准星辅助对齐
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2(0, g_Touch.y), ImVec2(io.DisplaySize.x, g_Touch.y), IM_COL32(255, 0, 0, 150));
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2(g_Touch.x, 0), ImVec2(g_Touch.x, io.DisplaySize.y), IM_COL32(255, 0, 0, 150));
        ImGui::GetBackgroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(255, 0, 0, 255));
    }

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(w * 0.85f, 0), ImGuiCond_Once);
        ImGui::Begin("AndKitty Precision Fix", &g_ShowMenu);
        
        ImGui::Text("Render Res: %d x %d", w, h);
        ImGui::Text("Max Input X Detected: %.1f", g_Touch.maxSeenX);
        ImGui::Text("Current ScaleX: %.4f", g_Touch.scaleX);

        ImGui::Separator();
        ImGui::Text("If red dot is too far RIGHT, decrease Scale X");
        
        if (ImGui::SliderFloat("Scale X", &g_Touch.scaleX, 0.1f, 1.2f, "%.4f")) g_Touch.calibrated = true;
        if (ImGui::SliderFloat("Offset X", &g_Touch.offsetX, -300.0f, 300.0f, "%.0f")) g_Touch.calibrated = true;
        
        ImGui::Spacing();
        
        // 针对红点往右偏的快速修复方案
        if (ImGui::Button("Fix: Red Dot Too Far Right", ImVec2(-1, 100))) {
            // 如果 maxSeenX 存在，我们尝试用它作为分母
            if (g_Touch.maxSeenX > w) {
                g_Touch.scaleX = (float)w / g_Touch.maxSeenX;
                g_Touch.scaleY = (float)h / (g_Touch.maxSeenX * (2400.0f/3392.0f));
            } else {
                g_Touch.scaleX *= 0.9f; // 每次缩小 10%
            }
            g_Touch.calibrated = true;
        }

        if (ImGui::Button("Reset Calibration", ImVec2(-1, 80))) {
            g_Touch.calibrated = false;
            g_Touch.maxSeenX = 0;
            g_Touch.scaleX = 1.0f; g_Touch.scaleY = 1.0f;
            g_Touch.offsetX = 0; g_Touch.offsetY = 0;
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
        LOGI("Input System Active.");
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, init_thread, nullptr);
}
