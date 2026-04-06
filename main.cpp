#include <list>
#include <vector>
#include <string.h>
#include <pthread.h>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <dlfcn.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "dobby.h"
#include <android/log.h>
#include <android/input.h>

#define LOG_TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// --- 状态与坐标 ---
static bool g_Initialized = false;
static bool g_ShowMenu = true;
static float g_ScreenWidth = 0.0f;
static float g_ScreenHeight = 0.0f;

// 记录触摸状态
struct {
    float x;
    float y;
    bool down;
} g_TouchState = {0.0f, 0.0f, false};

// --- 原函数指针 ---
typedef float (*p_getRawX)(const AInputEvent* motion_event, size_t pointer_index);
static p_getRawX old_getRawX = nullptr;

typedef float (*p_getRawY)(const AInputEvent* motion_event, size_t pointer_index);
static p_getRawY old_getRawY = nullptr;

typedef int32_t (*p_getAction)(const AInputEvent* motion_event);
static p_getAction old_getAction = nullptr;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 【核心】直接拦截系统坐标读取 ---
// 无论游戏怎么绕，它最终必须调用这些函数来获取坐标
float hook_getRawX(const AInputEvent* event, size_t index) {
    float x = old_getRawX(event, index);
    if (index == 0) g_TouchState.x = x;
    return x;
}

float hook_getRawY(const AInputEvent* event, size_t index) {
    float y = old_getRawY(event, index);
    if (index == 0) g_TouchState.y = y;
    return y;
}

int32_t hook_getAction(const AInputEvent* event) {
    int32_t action = old_getAction(event);
    int32_t code = action & AMOTION_EVENT_ACTION_MASK;
    
    if (code == AMOTION_EVENT_ACTION_DOWN || code == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        g_TouchState.down = true;
    } else if (code == AMOTION_EVENT_ACTION_UP || code == AMOTION_EVENT_ACTION_POINTER_UP || code == AMOTION_EVENT_ACTION_CANCEL) {
        g_TouchState.down = false;
    }
    return action;
}

// --- 渲染逻辑 ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (g_ScreenWidth <= 0) {
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        g_ScreenWidth = (float)w;
        g_ScreenHeight = (float)h;
    }

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        g_Initialized = true;
        LOGI("ImGui Render Started: %.fx%.f", g_ScreenWidth, g_ScreenHeight);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_ScreenWidth, g_ScreenHeight);
        
        // 关键注入
        io.MousePos = ImVec2(g_TouchState.x, g_TouchState.y);
        io.MouseDown[0] = g_TouchState.down;

        ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("AndKitty SGame Final", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(0, 1, 1, 1), "Input Mode: Raw System Hook");
            ImGui::Separator();
            
            ImGui::Text("Screen: %.0f x %.0f", g_ScreenWidth, g_ScreenHeight);
            ImGui::Text("Touch Pos: %.1f, %.1f", g_TouchState.x, g_TouchState.y);
            ImGui::Text("Touch Down: %s", g_TouchState.down ? "YES" : "NO");
            
            ImGui::Separator();
            static bool esp = false;
            ImGui::Checkbox("Enable Visuals", &esp);
            
            if (ImGui::Button("Close Menu")) g_ShowMenu = false;
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

// --- 初始化线程 ---
void* init_thread(void*) {
    LOGI("New Input Strategy: Hooking AMotionEvent_getRaw...");
    
    // 1. Hook 渲染
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 2. Hook libandroid.so 的底层坐标获取函数
    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        void* getX = dlsym(libandroid, "AMotionEvent_getRawX");
        void* getY = dlsym(libandroid, "AMotionEvent_getRawY");
        void* getAct = dlsym(libandroid, "AMotionEvent_getAction");

        // 如果没有 getRawX (旧版本安卓)，尝试 getX
        if (!getX) getX = dlsym(libandroid, "AMotionEvent_getX");
        if (!getY) getY = dlsym(libandroid, "AMotionEvent_getY");

        if (getX) DobbyHook(getX, (void*)hook_getRawX, (void**)&old_getRawX);
        if (getY) DobbyHook(getY, (void*)hook_getRawY, (void**)&old_getRawY);
        if (getAct) DobbyHook(getAct, (void*)hook_getAction, (void**)&old_getAction);

        LOGI("Raw Hooks Result: X:%p Y:%p Act:%p", getX, getY, getAct);
    }

    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
