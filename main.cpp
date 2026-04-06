#include <list>
#include <vector>
#include <string.h>
#include <pthread.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
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

// --- 全局变量与状态 ---
static bool g_Initialized = false;
static bool g_ShowMenu = true;
static float g_MenuX = 100.0f;
static float g_MenuY = 100.0f;

// 触摸状态全局快照 (用于底层拦截同步)
struct {
    float x;
    float y;
    bool down;
} g_RealTimeTouch = {0.0f, 0.0f, false};

// --- 原函数指针声明 ---
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef float (*p_AMotionEvent_getX)(const AInputEvent* motion_event, size_t pointer_index);
static p_AMotionEvent_getX old_getX = nullptr;

typedef float (*p_AMotionEvent_getY)(const AInputEvent* motion_event, size_t pointer_index);
static p_AMotionEvent_getY old_getY = nullptr;

typedef int32_t (*p_AMotionEvent_getAction)(const AInputEvent* motion_event);
static p_AMotionEvent_getAction old_getAction = nullptr;

// --- 触摸 Hook 回调 ---
// 拦截系统底层坐标换算，确保拿到最真实的点击位置
float hook_AMotionEvent_getX(const AInputEvent* event, size_t index) {
    float x = old_getX(event, index);
    if (index == 0) g_RealTimeTouch.x = x; 
    return x;
}

float hook_AMotionEvent_getY(const AInputEvent* event, size_t index) {
    float y = old_getY(event, index);
    if (index == 0) g_RealTimeTouch.y = y;
    return y;
}

int32_t hook_AMotionEvent_getAction(const AInputEvent* event) {
    int32_t action = old_getAction(event);
    int32_t code = action & AMOTION_EVENT_ACTION_MASK;
    
    // 处理按下与抬起状态
    if (code == AMOTION_EVENT_ACTION_DOWN || code == AMOTION_EVENT_ACTION_POINTER_DOWN) 
        g_RealTimeTouch.down = true;
    else if (code == AMOTION_EVENT_ACTION_UP || code == AMOTION_EVENT_ACTION_POINTER_UP || code == AMOTION_EVENT_ACTION_CANCEL) 
        g_RealTimeTouch.down = false;
    
    return action;
}

// --- 渲染逻辑 ---
void SetupImGui(int width, int height) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // 设置字体大小等 UI 风格
    ImGui::StyleColorsDark();
    ImGui_ImplOpenGL3_Init("#version 300 es");
    g_Initialized = true;
    LOGI("ImGui Initialized: %dx%d", width, height);
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    static int width = 0, height = 0;
    if (width == 0 || height == 0) {
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        width = w; height = h;
    }

    if (!g_Initialized) {
        SetupImGui(width, height);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGuiIO& io = ImGui::GetIO();
        
        // 【核心注入】：将底层 Hook 拿到的坐标强制塞给 ImGui
        io.MousePos = ImVec2(g_RealTimeTouch.x, g_RealTimeTouch.y);
        io.MouseDown[0] = g_RealTimeTouch.down;
        io.DisplaySize = ImVec2((float)width, (float)height);

        // 绘制菜单界面
        ImGui::SetNextWindowPos(ImVec2(g_MenuX, g_MenuY), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("AndKitty Mod Menu", &g_ShowMenu)) {
            ImGui::Text("Status: Running");
            ImGui::Separator();
            
            static bool esp = false;
            ImGui::Checkbox("Enable ESP", &esp);
            
            static float aim_speed = 5.0f;
            ImGui::SliderFloat("Aim Speed", &aim_speed, 1.0f, 20.0f);

            if (ImGui::Button("Hide Menu")) {
                g_ShowMenu = false;
            }
            
            // 调试信息：显示实时捕获的坐标
            ImGui::Text("Touch: X:%.1f Y:%.1f (%s)", 
                        g_RealTimeTouch.x, g_RealTimeTouch.y, 
                        g_RealTimeTouch.down ? "DOWN" : "UP");
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surface);
}

// --- 初始化线程 ---
void* init_thread(void*) {
    LOGI("Plugin thread started, waiting for game initialization...");
    sleep(15); // 等待游戏过掉检测和 Logo 加载

    // 1. Hook 渲染函数
    void* egl_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl_addr) {
        DobbyHook(egl_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Hooked eglSwapBuffers at %p", egl_addr);
    }

    // 2. Hook 底层触摸 (libandroid.so)
    // 即使游戏屏蔽了上层的 InputQueue，底层的这些函数依然会被调用
    void* h_android = dlopen("libandroid.so", RTLD_NOW);
    if (h_android) {
        void* getX = dlsym(h_android, "AMotionEvent_getX");
        void* getY = dlsym(h_android, "AMotionEvent_getY");
        void* getAction = dlsym(h_android, "AMotionEvent_getAction");

        if (getX) DobbyHook(getX, (void*)hook_AMotionEvent_getX, (void**)&old_getX);
        if (getY) DobbyHook(getY, (void*)hook_AMotionEvent_getY, (void**)&old_getY);
        if (getAction) DobbyHook(getAction, (void*)hook_AMotionEvent_getAction, (void**)&old_getAction);
        
        LOGI("Touch Force-Sync Hooks Applied!");
    }

    return nullptr;
}

// .so 加载入口
__attribute__((constructor))
void main_entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
