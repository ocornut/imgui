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

#define LOG_TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- 全局状态 ---
static bool g_Initialized = false;
static bool g_ShowMenu = true;
static float g_Width = 0, g_Height = 0;

struct {
    float x, y;
    bool down;
} g_Touch = {0, 0, false};

// --- 原函数指针 ---
typedef int32_t (*p_getPointerId)(const AInputEvent* motion_event, size_t pointer_index);
static p_getPointerId old_getPointerId = nullptr;

typedef float (*p_getRawX)(const AInputEvent* motion_event, size_t pointer_index);
static p_getRawX old_getRawX = nullptr;

typedef float (*p_getRawY)(const AInputEvent* motion_event, size_t pointer_index);
static p_getRawY old_getRawY = nullptr;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 【核心】多重拦截逻辑 ---
// 我们在游戏获取 PointerId 的那一刻，顺便把坐标存下来
int32_t hook_getPointerId(const AInputEvent* event, size_t index) {
    if (event != nullptr && AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 尝试获取原始坐标
        if (old_getRawX && old_getRawY) {
            g_Touch.x = old_getRawX(event, 0);
            g_Touch.y = old_getRawY(event, 0);
        } else {
            g_Touch.x = AMotionEvent_getX(event, 0);
            g_Touch.y = AMotionEvent_getY(event, 0);
        }

        if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            g_Touch.down = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down = false;
        }
    }
    return old_getPointerId(event, index);
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
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        g_Initialized = true;
        LOGI("ImGui Ready: %.fx%.f", g_Width, g_Height);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_Width, g_Height);
        
        // 关键：将拦截到的坐标注入 ImGui
        io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
        io.MouseDown[0] = g_Touch.down;

        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("AndKitty SGame Final", &g_ShowMenu)) {
            ImGui::Text("Status: Input Hooked");
            ImGui::Text("Touch Pos: %.1f, %.1f", g_Touch.x, g_Touch.y);
            ImGui::Text("Action: %s", g_Touch.down ? "DOWN" : "UP");
            ImGui::Separator();
            
            static bool test = false;
            ImGui::Checkbox("Function Test", &test);
            
            if (ImGui::Button("Hide Menu")) g_ShowMenu = false;
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    // 1. 渲染 Hook
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 2. 输入 Hook
    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        void* getPointerId = dlsym(libandroid, "AMotionEvent_getPointerId");
        void* getRawX = dlsym(libandroid, "AMotionEvent_getRawX");
        void* getRawY = dlsym(libandroid, "AMotionEvent_getRawY");

        // 如果找不到 getRawX，就找 getX
        if (!getRawX) getRawX = dlsym(libandroid, "AMotionEvent_getX");
        if (!getRawY) getRawY = dlsym(libandroid, "AMotionEvent_getY");

        // 我们 Hook 这三个函数，但重点是 getPointerId
        if (getPointerId) DobbyHook(getPointerId, (void*)hook_getPointerId, (void**)&old_getPointerId);
        if (getRawX) old_getRawX = (p_getRawX)getRawX;
        if (getRawY) old_getRawY = (p_getRawY)getRawY;

        LOGI("Input Hooks Ready: PointerId:%p, RawX:%p", getPointerId, getRawX);
    }

    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
