#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <string>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool g_Initialized = false;
static bool g_ShowMenu = true;

// 状态追踪日志，用于在 UI 上显示
static char g_HookStatus[256] = "Waiting...";

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 触摸数据结构 ---
struct TouchState {
    float x;
    float y;
    bool down;
    int event_count; // 记录收到了多少次事件
} g_Touch;

// --- 核心 Hook 逻辑 ---

typedef float (*p_AMotionEvent_getRawX)(const AInputEvent* motion_event, size_t pointer_index);
static p_AMotionEvent_getRawX old_getRawX = nullptr;

typedef float (*p_AMotionEvent_getRawY)(const AInputEvent* motion_event, size_t pointer_index);
static p_AMotionEvent_getRawY old_getRawY = nullptr;

typedef int32_t (*p_AMotionEvent_getAction)(const AInputEvent* motion_event);
static p_AMotionEvent_getAction old_getAction = nullptr;

float hook_AMotionEvent_getRawX(const AInputEvent* motion_event, size_t pointer_index) {
    float x = old_getRawX(motion_event, pointer_index);
    if (g_ShowMenu && g_Initialized && pointer_index == 0) {
        g_Touch.x = x;
        g_Touch.event_count++;
    }
    return x;
}

float hook_AMotionEvent_getRawY(const AInputEvent* motion_event, size_t pointer_index) {
    float y = old_getRawY(motion_event, pointer_index);
    if (g_ShowMenu && g_Initialized && pointer_index == 0) {
        g_Touch.y = y;
    }
    return y;
}

int32_t hook_AMotionEvent_getAction(const AInputEvent* motion_event) {
    int32_t action = old_getAction(motion_event);
    if (g_ShowMenu && g_Initialized) {
        int32_t code = action & AMOTION_EVENT_ACTION_MASK;
        if (code == AMOTION_EVENT_ACTION_DOWN || code == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            g_Touch.down = true;
        } else if (code == AMOTION_EVENT_ACTION_UP || code == AMOTION_EVENT_ACTION_POINTER_UP || code == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down = false;
        }
    }
    return action;
}

// 渲染钩子
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        EGLint width, height;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
        
        float scale = (float)width / 1920.0f;
        ImGui::GetStyle().ScaleAllSizes(scale > 1.2f ? scale : 1.2f);
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
        LOGI("ImGui Initialized successfully.");
    }

    if (g_ShowMenu) {
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
        io.MouseDown[0] = g_Touch.down;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty SGame Debugger", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Hook Status: %s", g_HookStatus);
            ImGui::Text("Touch Data: %s (%.1f, %.1f)", g_Touch.down ? "DOWN" : "UP", g_Touch.x, g_Touch.y);
            ImGui::Text("Total Events Captured: %d", g_Touch.event_count);
            
            ImGui::Separator();
            
            if (ImGui::Button("Reset Touch State")) {
                g_Touch.down = false;
                g_Touch.event_count = 0;
            }

            ImGui::Checkbox("Menu Visible", &g_ShowMenu);
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Init thread started. Target: libunity.so");
    
    // 持续检查模块加载情况
    int retry = 0;
    while (retry < 60) { // 最多等待 30 秒
        if (dlopen("libunity.so", RTLD_NOLOAD)) {
            LOGI("libunity.so found!");
            break;
        }
        usleep(500000);
        retry++;
    }

    // 1. Hook 渲染
    void* swap_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (swap_addr) {
        DobbyHook(swap_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("eglSwapBuffers Hooked.");
    } else {
        LOGI("FAILED to find eglSwapBuffers");
    }

    // 2. Hook 触摸
    void* getRawX_addr = dlsym(RTLD_DEFAULT, "AMotionEvent_getRawX");
    void* getRawY_addr = dlsym(RTLD_DEFAULT, "AMotionEvent_getRawY");
    void* getAction_addr = dlsym(RTLD_DEFAULT, "AMotionEvent_getAction");

    if (getRawX_addr && getRawY_addr && getAction_addr) {
        DobbyHook(getRawX_addr, (void*)hook_AMotionEvent_getRawX, (void**)&old_getRawX);
        DobbyHook(getRawY_addr, (void*)hook_AMotionEvent_getRawY, (void**)&old_getRawY);
        DobbyHook(getAction_addr, (void*)hook_AMotionEvent_getAction, (void**)&old_getAction);
        snprintf(g_HookStatus, sizeof(g_HookStatus), "All Hooks OK (Unity Mode)");
        LOGI("AMotionEvent Hooks applied successfully.");
    } else {
        snprintf(g_HookStatus, sizeof(g_HookStatus), "Hook FAILED: %p %p %p", getRawX_addr, getRawY_addr, getAction_addr);
        LOGI("FAILED to find AMotionEvent symbols.");
    }

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
