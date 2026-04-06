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

#define LOG_TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

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

// --- Unity 函数指针 ---
typedef bool (*p_GetMouseButton)(int button);
static p_GetMouseButton old_GetMouseButton = nullptr;

typedef void* (*p_get_mousePosition)(void* outPos);
static p_get_mousePosition old_get_mousePosition = nullptr;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- Hook 回调 ---
bool hook_GetMouseButton(int button) {
    if (old_GetMouseButton) {
        bool res = old_GetMouseButton(button);
        if (button == 0) g_TouchState.down = res;
        return res;
    }
    return false;
}

void* hook_get_mousePosition(void* outPos) {
    if (old_get_mousePosition) {
        void* res = old_get_mousePosition(outPos);
        if (outPos) {
            float* pos = (float*)outPos; 
            g_TouchState.x = pos[0];
            // 只有拿到屏幕高度后才进行 Y 轴翻转，否则先存原始值
            g_TouchState.y = (g_ScreenHeight > 0) ? (g_ScreenHeight - pos[1]) : pos[1];
        }
        return res;
    }
    return outPos;
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
        ImGuiIO& io = ImGui::GetIO();
        
        // 针对移动端的优化设置
        io.IniFilename = nullptr; 
        ImGui::StyleColorsDark();
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        // 字体缩放适配高分屏
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        g_Initialized = true;
        LOGI("ImGui Render Init Success: %.fx%.f", g_ScreenWidth, g_ScreenHeight);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_ScreenWidth, g_ScreenHeight);
        
        // 关键：强制注入 Unity 捕获的坐标
        io.MousePos = ImVec2(g_TouchState.x, g_TouchState.y);
        io.MouseDown[0] = g_TouchState.down;

        ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("AndKitty SGame Debugger", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Input Mode: Unity Global Hook");
            ImGui::Separator();
            
            ImGui::Text("Screen: %.0f x %.0f", g_ScreenWidth, g_ScreenHeight);
            ImGui::Text("Touch Pos: %.1f, %.1f", g_TouchState.x, g_TouchState.y);
            ImGui::Text("Touch Status: %s", g_TouchState.down ? "DOWN" : "UP");
            
            ImGui::Separator();
            static bool esp = false;
            ImGui::Checkbox("Enable ESP Line", &esp);

            if (ImGui::Button("Reset Touch State")) {
                g_TouchState.down = false;
            }
            
            if (ImGui::Button("Close Menu")) g_ShowMenu = false;
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

// --- 自动搜索符号并 Hook ---
void* init_thread(void*) {
    LOGI("Plugin thread started. Waiting for libunity.so...");
    
    void* handle = nullptr;
    while (!handle) {
        handle = dlopen("libunity.so", RTLD_NOW);
        if (!handle) usleep(500000);
    }
    LOGI("libunity.so found at %p", handle);

    // 1. Hook SwapBuffers (渲染)
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 2. 尝试多种可能的 Unity 输入符号 (SGame 特化)
    // 列表包含常见混淆名和标准名
    const char* btn_syms[] = {
        "_ZN5Unity5Input14GetMouseButtonEi", 
        "UnityInputGetMouseButton",
        "Input_GetMouseButton",
        "_ZN11UnityEngine5Input14GetMouseButtonEi"
    };

    const char* pos_syms[] = {
        "_ZN5Unity5Input17get_mousePositionEv",
        "UnityInputGetMousePosition",
        "Input_get_mousePosition",
        "_ZN11UnityEngine5Input17get_mousePositionEv"
    };

    void* get_btn = nullptr;
    void* get_pos = nullptr;

    for (const char* s : btn_syms) {
        get_btn = dlsym(handle, s);
        if (get_btn) {
            LOGI("Found MouseButton Sym: %s", s);
            break;
        }
    }

    for (const char* s : pos_syms) {
        get_pos = dlsym(handle, s);
        if (get_pos) {
            LOGI("Found MousePosition Sym: %s", s);
            break;
        }
    }

    if (get_btn) DobbyHook(get_btn, (void*)hook_GetMouseButton, (void**)&old_GetMouseButton);
    if (get_pos) DobbyHook(get_pos, (void*)hook_get_mousePosition, (void**)&old_get_mousePosition);

    LOGI("Hooks Final Status - Render: %p, BTN: %p, POS: %p", egl, get_btn, get_pos);
    
    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
