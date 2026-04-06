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

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 【核心】极简拦截器 ---
// 参考主流做法：Hook getPointerId 是获取 InputEvent 最稳妥的时机
int32_t hook_getPointerId(const AInputEvent* event, size_t index) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 记录坐标和状态
        g_Touch.x = AMotionEvent_getX(event, 0);
        g_Touch.y = AMotionEvent_getY(event, 0);

        if (action == AMOTION_EVENT_ACTION_DOWN) g_Touch.down = true;
        else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_Touch.down = false;
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
        ImGui::GetStyle().ScaleAllSizes(3.0f); // 适配高分屏
        g_Initialized = true;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_Width, g_Height);
        io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
        io.MouseDown[0] = g_Touch.down;

        ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("AndKitty SGame Lite", &g_ShowMenu)) {
            ImGui::Text("Simple Input Mode Active");
            ImGui::Separator();
            ImGui::Text("Touch: %.1f, %.1f (%s)", g_Touch.x, g_Touch.y, g_Touch.down ? "DOWN" : "UP");
            
            static float f = 0.5f;
            ImGui::SliderFloat("Value Test", &f, 0.0f, 1.0f);
            
            if (ImGui::Button("Hide Menu")) g_ShowMenu = false;
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    // 渲染 Hook
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 输入 Hook: 锁定 libandroid.so 或直接从 RTLD_DEFAULT 找
    // 绝大多数 Mod 使用 AMotionEvent_getPointerId 作为切入点
    void* getPointerId = dlsym(RTLD_DEFAULT, "AMotionEvent_getPointerId");
    if (getPointerId) {
        DobbyHook(getPointerId, (void*)hook_getPointerId, (void**)&old_getPointerId);
        LOGI("Simple Hook Applied: getPointerId");
    }

    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
