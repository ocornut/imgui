#include <pthread.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "dobby.h"
#include <android/log.h>
#include <android/input.h>
#include <android/native_window.h>

#define LOG_TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- 全局状态 ---
static bool g_Initialized = false;
static bool g_ShowMenu = true;
static float g_Width = 0, g_Height = 0;

// --- 原函数指针 ---
typedef int (*p_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** out_event);
static p_AInputQueue_getEvent old_getEvent = nullptr;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 【核心】GitHub 常用：ImGui 官方 Android 事件处理逻辑 ---
// 这种方式不需要手动写坐标，直接把 Android 原生事件丢给 ImGui
int hook_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** out_event) {
    int res = old_getEvent(queue, out_event);
    if (res >= 0 && g_ShowMenu && out_event && *out_event) {
        // 调用 ImGui 内部的 Android 处理函数 (需要保证 ImGui 已初始化)
        if (g_Initialized) {
            ImGui_ImplAndroid_HandleInputEvent(*out_event);
        }
        
        // 可选：如果触摸在菜单内，返回 1 消耗掉事件，让游戏收不到点击
        /*
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            // 注意：某些游戏拦截事件会导致摇杆卡死，初次建议不拦截
        }
        */
    }
    return res;
}

// --- 渲染逻辑 ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (g_Width <= 0) {
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        g_Width = (float)w; g_Height = (float)h;
    }

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        // 关键：告诉 ImGui 屏幕尺寸
        io.DisplaySize = ImVec2(g_Width, g_Height);
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        // 设置样式
        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        
        g_Initialized = true;
        LOGI("ImGui Full Init Success: %.fx%.f", g_Width, g_Height);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        // 设置窗口位置和大小
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("AndKitty SGame Ultimate", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(0, 1, 1, 1), "Input: AInputQueue + HandleEvent");
            ImGui::Separator();
            
            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("Mouse Pos: %.1f, %.1f", io.MousePos.x, io.MousePos.y);
            ImGui::Text("WantCapture: %s", io.WantCaptureMouse ? "YES" : "NO");

            ImGui::Separator();
            static bool esp = false;
            ImGui::Checkbox("Enable ESP", &esp);
            
            static int radius = 50;
            ImGui::SliderInt("Range", &radius, 10, 200);

            if (ImGui::Button("Close Menu")) g_ShowMenu = false;
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Applying Source-Level Input Strategy...");

    // 1. Hook SwapBuffers
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 2. Hook AInputQueue_getEvent
    // 这是 Android 系统的“咽喉”，所有事件都会流经这里
    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        void* getEvent = dlsym(libandroid, "AInputQueue_getEvent");
        if (getEvent) {
            DobbyHook(getEvent, (void*)hook_AInputQueue_getEvent, (void**)&old_getEvent);
            LOGI("AInputQueue Hooked Successfully.");
        }
    }

    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
