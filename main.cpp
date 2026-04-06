#include <pthread.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <time.h>
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

// 用于诊断的数据
struct {
    float rawX, rawY;
    bool isDown;
    int eventCount;
} g_Diag = {0, 0, false, 0};

// --- 原函数指针 ---
typedef int (*p_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** out_event);
static p_AInputQueue_getEvent old_getEvent = nullptr;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 获取精确时间 ---
double get_tick_count() {
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    return (double)res.tv_sec + (double)res.tv_nsec / 1e9;
}

// --- 核心：强制输入修正 ---
int hook_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** out_event) {
    int res = old_getEvent(queue, out_event);
    if (res >= 0 && g_ShowMenu && out_event && *out_event && g_Initialized) {
        AInputEvent* event = *out_event;
        ImGuiIO& io = ImGui::GetIO();

        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
            int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
            
            // 1. 记录原始坐标用于 UI 显示诊断
            g_Diag.rawX = AMotionEvent_getX(event, 0);
            g_Diag.rawY = AMotionEvent_getY(event, 0);
            g_Diag.eventCount++;

            // 2. 强制覆盖 ImGui 的坐标系统 (核心修复)
            io.MousePos = ImVec2(g_Diag.rawX, g_Diag.rawY);

            if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
                g_Diag.isDown = true;
                io.MouseDown[0] = true;
            } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
                g_Diag.isDown = false;
                io.MouseDown[0] = false;
            }
            
            // 3. 调用官方处理函数 (主要为了处理键盘或其他状态，但坐标已被我们抢占)
            ImGui_ImplAndroid_HandleInputEvent(event);
            
            // 再次强制覆盖，防止 HandleInputEvent 把坐标改回负数
            io.MousePos = ImVec2(g_Diag.rawX, g_Diag.rawY);
        }
    }
    return res;
}

// --- 渲染逻辑 ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    static double last_time = 0;
    double current_time = get_tick_count();
    
    if (g_Width <= 0) {
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        g_Width = (float)w; g_Height = (float)h;
    }

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_Width, g_Height);
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui_ImplAndroid_Init(nullptr); // 不传入 window，手动接管

        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        
        g_Initialized = true;
        last_time = current_time;
        LOGI("Diagnostic ImGui Init: %.fx%.f", g_Width, g_Height);
    }

    // 修复 DeltaTime 确保拖拽逻辑生效
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = (float)(current_time - last_time);
    if (io.DeltaTime <= 0) io.DeltaTime = 1.0f/60.0f;
    last_time = current_time;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("AndKitty Input Debugger", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Diagnosis Info:");
            ImGui::BulletText("Screen Size: %.0f x %.0f", g_Width, g_Height);
            ImGui::BulletText("Raw NDK Pos: %.1f, %.1f", g_Diag.rawX, g_Diag.rawY);
            ImGui::BulletText("ImGui IO Pos: %.1f, %.1f", io.MousePos.x, io.MousePos.y);
            ImGui::BulletText("Touch State: %s", g_Diag.isDown ? "DOWN" : "UP");
            ImGui::BulletText("Events Captured: %d", g_Diag.eventCount);
            
            ImGui::Separator();
            
            if (ImGui::Button("Reset Event Count")) g_Diag.eventCount = 0;
            
            ImGui::Separator();
            static float test_slider = 0.5f;
            ImGui::SliderFloat("Test Drag", &test_slider, 0.0f, 1.0f);
            
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "If 'ImGui IO Pos' is same as 'Raw NDK Pos',\ndragging should work now.");

            if (ImGui::Button("Close Menu")) g_ShowMenu = false;
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    sleep(5); // 等待游戏环境稳定

    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        void* getEvent = dlsym(libandroid, "AInputQueue_getEvent");
        if (getEvent) {
            DobbyHook(getEvent, (void*)hook_AInputQueue_getEvent, (void**)&old_getEvent);
            LOGI("Debugger Hook Ready.");
        }
    }

    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
