#include <pthread.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <time.h>
#include <unistd.h>
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "dobby.h"
#include <android/log.h>
#include <android/input.h>
#include <android/native_window.h>

#define LOG_TAG "AndKitty_Diag"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- 诊断数据结构 ---
static struct {
    bool initialized = false;
    bool show_menu = true;
    float screen_w = 0, screen_h = 0;
    
    // 捕获到的实时数据
    float raw_x = 0, raw_y = 0;
    int last_action = -1;
    int event_counter = 0;
    double last_frame_time = 0;
    float current_delta = 0;
} g_Data;

// --- 原函数指针 ---
typedef int (*p_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** out_event);
static p_AInputQueue_getEvent old_getEvent = nullptr;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

double get_now() {
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    return (double)res.tv_sec + (double)res.tv_nsec / 1e9;
}

// --- 核心诊断 Hook ---
int hook_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** out_event) {
    int res = old_getEvent(queue, out_event);
    if (res >= 0 && out_event && *out_event && g_Data.initialized) {
        AInputEvent* event = *out_event;
        
        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
            g_Data.event_counter++;
            g_Data.last_action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
            g_Data.raw_x = AMotionEvent_getX(event, 0);
            g_Data.raw_y = AMotionEvent_getY(event, 0);

            if (g_Data.show_menu) {
                ImGuiIO& io = ImGui::GetIO();
                // 强制修正：手动同步坐标
                io.MousePos = ImVec2(g_Data.raw_x, g_Data.raw_y);
                
                if (g_Data.last_action == AMOTION_EVENT_ACTION_DOWN) io.MouseDown[0] = true;
                else if (g_Data.last_action == AMOTION_EVENT_ACTION_UP) io.MouseDown[0] = false;

                // 依然让官方后端跑一下，处理多指逻辑
                ImGui_ImplAndroid_HandleInputEvent(event);
                
                // 【诊断关键点】再次强行锁死坐标，防止 HandleInputEvent 内部由于 Window 指针丢失导致的负数崩溃
                io.MousePos = ImVec2(g_Data.raw_x, g_Data.raw_y);
            }
        }
    }
    return res;
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    double now = get_now();
    
    if (g_Data.screen_w <= 0) {
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        g_Data.screen_w = (float)w; g_Data.screen_h = (float)h;
    }

    if (!g_Data.initialized) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_Data.screen_w, g_Data.screen_h);
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui_ImplAndroid_Init(nullptr); 

        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        
        g_Data.initialized = true;
        g_Data.last_frame_time = now;
    }

    // 每一帧更新时间，否则无法拖拽
    ImGuiIO& io = ImGui::GetIO();
    g_Data.current_delta = (float)(now - g_Data.last_frame_time);
    io.DeltaTime = (g_Data.current_delta > 0) ? g_Data.current_delta : (1.0f/60.0f);
    g_Data.last_frame_time = now;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    if (g_Data.show_menu) {
        ImGui::SetNextWindowSize(ImVec2(750, 550), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("AndKitty Diagnosis Tool", &g_Data.show_menu)) {
            
            // --- 诊断面板 ---
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "SYSTEM STATUS:");
            ImGui::Text("Screen: %.0f x %.0f", g_Data.screen_w, g_Data.screen_h);
            ImGui::Text("DeltaTime: %.4f", io.DeltaTime);
            ImGui::Text("Total Events: %d", g_Data.event_counter);
            
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0, 1, 1, 1), "INPUT TRACKING:");
            ImGui::Text("Raw NDK Pos: %.1f, %.1f", g_Data.raw_x, g_Data.raw_y);
            ImGui::Text("ImGui IO Pos: %.1f, %.1f", io.MousePos.x, io.MousePos.y);
            ImGui::Text("Action Code: %d (%s)", g_Data.last_action, io.MouseDown[0] ? "DOWN" : "UP");
            
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 0, 1, 1), "CAPTURE TEST:");
            ImGui::Checkbox("Capture Mouse?", &io.WantCaptureMouse);
            
            static float test_f = 0.5f;
            ImGui::SliderFloat("Drag Logic Test", &test_f, 0.0f, 1.0f);
            
            if (ImGui::Button("Reset Counter")) g_Data.event_counter = 0;
            
            ImGui::Separator();
            ImGui::TextWrapped("GUIDE: If 'Raw NDK Pos' moves but 'ImGui IO Pos' does not, the override failed.");
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    sleep(10); // 等待游戏加载

    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        void* getEvent = dlsym(libandroid, "AInputQueue_getEvent");
        if (getEvent) {
            DobbyHook(getEvent, (void*)hook_AInputQueue_getEvent, (void**)&old_getEvent);
            LOGI("Diagnostics Hooked Successfully.");
        }
    }
    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
