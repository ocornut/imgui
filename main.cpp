#include <pthread.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <time.h>
#include <unistd.h>     // 修复 sleep 报错：增加此头文件
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

// --- 精确时间工具 ---
double get_tick_count() {
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    return (double)res.tv_sec + (double)res.tv_nsec / 1e9;
}

// --- 截获并解析事件 ---
int hook_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** out_event) {
    int res = old_getEvent(queue, out_event);
    if (res >= 0 && g_ShowMenu && out_event && *out_event && g_Initialized) {
        AInputEvent* event = *out_event;
        ImGuiIO& io = ImGui::GetIO();

        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
            int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
            
            // 手动提取坐标并强制覆盖，解决负数天文数字问题
            float x = AMotionEvent_getX(event, 0);
            float y = AMotionEvent_getY(event, 0);
            io.MousePos = ImVec2(x, y);

            if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
                io.MouseDown[0] = true;
            } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
                io.MouseDown[0] = false;
            }

            // 同步官方后端状态
            ImGui_ImplAndroid_HandleInputEvent(event);
            
            // 再次强制覆盖坐标，确保不被后端内部错误的转换逻辑覆盖
            io.MousePos = ImVec2(x, y);
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
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen; // 开启触摸优化

        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui_ImplAndroid_Init(nullptr); 

        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(3.0f); // 适配手机高分屏
        
        g_Initialized = true;
        last_time = current_time;
        LOGI("ImGui Full Init: %.fx%.f", g_Width, g_Height);
    }

    // 核心修复：计算时间差，否则 ImGui 无法处理“拖动”动作
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = (float)(current_time - last_time);
    if (io.DeltaTime <= 0) io.DeltaTime = 1.0f/60.0f;
    last_time = current_time;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("AndKitty Input Fixer", &g_ShowMenu)) {
            ImGui::Text("Mouse Pos: %.1f, %.1f", io.MousePos.x, io.MousePos.y);
            ImGui::Text("WantCapture: %s", io.WantCaptureMouse ? "YES" : "NO");
            ImGui::Separator();
            
            static float f = 0.5f;
            ImGui::SliderFloat("Drag Test", &f, 0.0f, 1.0f);
            
            if (ImGui::Button("Reset Position")) {
                ImGui::SetWindowPos(ImVec2(100, 100));
            }

            if (ImGui::Button("Hide Menu")) g_ShowMenu = false;
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    // 增加延迟，避开游戏启动时的保护检测
    sleep(10); 

    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        void* getEvent = dlsym(libandroid, "AInputQueue_getEvent");
        if (getEvent) {
            DobbyHook(getEvent, (void*)hook_AInputQueue_getEvent, (void**)&old_getEvent);
            LOGI("Input Interceptor Deployed.");
        }
    }

    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
