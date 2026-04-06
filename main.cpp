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
#include <android/keycodes.h>

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
typedef int (*p_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** out_event);
static p_AInputQueue_getEvent old_getEvent = nullptr;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 【核心】底层输入拦截器 ---
int hook_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** out_event) {
    int res = old_getEvent(queue, out_event);
    if (res >= 0 && out_event != nullptr && *out_event != nullptr) {
        AInputEvent* event = *out_event;
        int type = AInputEvent_getType(event);
        
        if (type == AINPUT_EVENT_TYPE_MOTION) {
            int action = AMotionEvent_getAction(event);
            int action_code = action & AMOTION_EVENT_ACTION_MASK;
            
            // 记录第一个手指的坐标
            g_TouchState.x = AMotionEvent_getX(event, 0);
            g_TouchState.y = AMotionEvent_getY(event, 0);

            if (action_code == AMOTION_EVENT_ACTION_DOWN || action_code == AMOTION_EVENT_ACTION_POINTER_DOWN) {
                g_TouchState.down = true;
            } else if (action_code == AMOTION_EVENT_ACTION_UP || action_code == AMOTION_EVENT_ACTION_POINTER_UP || action_code == AMOTION_EVENT_ACTION_CANCEL) {
                g_TouchState.down = false;
            }
        }
    }
    return res;
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
        LOGI("ImGui Init: %.fx%.f", g_ScreenWidth, g_ScreenHeight);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_ScreenWidth, g_ScreenHeight);
        
        // 强制注入从 AInputQueue 拦截到的坐标
        io.MousePos = ImVec2(g_TouchState.x, g_TouchState.y);
        io.MouseDown[0] = g_TouchState.down;

        ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("AndKitty SGame Ultimate", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Input: AInputQueue Hooked");
            ImGui::Separator();
            
            ImGui::Text("Screen: %.0f x %.0f", g_ScreenWidth, g_ScreenHeight);
            ImGui::Text("Touch Pos: %.1f, %.1f", g_TouchState.x, g_TouchState.y);
            ImGui::Text("Status: %s", g_TouchState.down ? "DOWN" : "UP");
            
            ImGui::Separator();
            static float speed = 5.0f;
            ImGui::SliderFloat("Speed", &speed, 1.0f, 10.0f);
            
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
    LOGI("Plugin thread started. Waiting for libs...");
    
    // 1. Hook SwapBuffers (渲染)
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 2. Hook AInputQueue_getEvent (libandroid.so)
    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        void* getEvent = dlsym(libandroid, "AInputQueue_getEvent");
        if (getEvent) {
            DobbyHook(getEvent, (void*)hook_AInputQueue_getEvent, (void**)&old_getEvent);
            LOGI("AInputQueue_getEvent Hooked at %p", getEvent);
        } else {
            LOGE("Failed to find AInputQueue_getEvent");
        }
    }

    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
