#include <pthread.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "dobby.h"
#include <android/log.h>
#include <android/input.h>

#define LOG_TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// --- 全局状态 ---
static bool g_Initialized = false;
static bool g_ShowMenu = true;
static float g_Width = 0, g_Height = 0;

struct TouchState {
    float x, y;
    bool down;
    int frameCounter;
} g_Touch = {0, 0, false, 0};

// --- 原函数指针 ---
typedef int32_t (*p_getPointerId)(const AInputEvent* motion_event, size_t pointer_index);
static p_getPointerId old_getPointerId = nullptr;

typedef float (*p_getRawX)(const AInputEvent* motion_event, size_t pointer_index);
static p_getRawX old_getRawX = nullptr;

typedef float (*p_getRawY)(const AInputEvent* motion_event, size_t pointer_index);
static p_getRawY old_getRawY = nullptr;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// ---------------------------------------------------------------------
// 实时更新坐标和触摸状态
// ---------------------------------------------------------------------
static void UpdateTouchFromEvent(const AInputEvent* event, size_t pointer_index) {
    if (!event || AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION)
        return;
    
    int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    // 只跟踪第一个手指（通常菜单只需单点）
    if (pointer_index == 0) {
        if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            g_Touch.down = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP ||
                   action == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down = false;
        }
    }
}

float hook_getRawX(const AInputEvent* event, size_t index) {
    if (event && old_getRawX) {
        float x = old_getRawX(event, index);
        g_Touch.x = x;
        UpdateTouchFromEvent(event, index);
        return x;
    }
    return 0.0f;
}

float hook_getRawY(const AInputEvent* event, size_t index) {
    if (event && old_getRawY) {
        float y = old_getRawY(event, index);
        g_Touch.y = y;
        UpdateTouchFromEvent(event, index);
        return y;
    }
    return 0.0f;
}

int32_t hook_getPointerId(const AInputEvent* event, size_t index) {
    return old_getPointerId ? old_getPointerId(event, index) : 0;
}

// ---------------------------------------------------------------------
// 渲染 Hook
// ---------------------------------------------------------------------
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (g_Width <= 0 || g_Height <= 0) {
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        g_Width = (float)w;
        g_Height = (float)h;
        LOGI("Screen size: %.0f x %.0f", g_Width, g_Height);
    }

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        // 移除不兼容的样式设置（TouchPadding 和 ScaleAllSizes）
        g_Initialized = true;
        LOGI("ImGui initialized");
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Width, g_Height);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    if (g_Touch.frameCounter++ % 120 == 0) {
        LOGI("Touch: (%.1f, %.1f) down=%d", g_Touch.x, g_Touch.y, g_Touch.down);
    }

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("AndKitty SGame Final", &g_ShowMenu)) {
            ImGui::Text("Status: Input Hooked (RawX/Y)");
            ImGui::Text("Touch Pos: %.1f, %.1f", g_Touch.x, g_Touch.y);
            ImGui::Text("Touch State: %s", g_Touch.down ? "DOWN" : "UP");
            ImGui::Separator();

            static bool testFlag = false;
            ImGui::Checkbox("Function Test", &testFlag);

            if (ImGui::Button("Hide Menu")) {
                g_ShowMenu = false;
                LOGI("Menu hidden by button");
            }
        }
        ImGui::End();
    } else {
        // 小窗口用于重新显示菜单
        ImGui::SetNextWindowSize(ImVec2(100, 50), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        if (ImGui::Begin("Menu Controller", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
            if (ImGui::Button("Show Menu")) {
                g_ShowMenu = true;
                LOGI("Menu shown by button");
            }
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surface);
}

// ---------------------------------------------------------------------
// 初始化线程
// ---------------------------------------------------------------------
void* init_thread(void*) {
    LOGI("Init thread started");

    void* eglFunc = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (eglFunc) {
        DobbyHook(eglFunc, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Hooked eglSwapBuffers at %p", eglFunc);
    } else {
        LOGE("Failed to find eglSwapBuffers");
    }

    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        void* getPointerId = dlsym(libandroid, "AMotionEvent_getPointerId");
        void* getRawX = dlsym(libandroid, "AMotionEvent_getRawX");
        void* getRawY = dlsym(libandroid, "AMotionEvent_getRawY");

        if (!getRawX) getRawX = dlsym(libandroid, "AMotionEvent_getX");
        if (!getRawY) getRawY = dlsym(libandroid, "AMotionEvent_getY");

        if (getRawX) {
            DobbyHook(getRawX, (void*)hook_getRawX, (void**)&old_getRawX);
            LOGI("Hooked getRawX at %p", getRawX);
        } else {
            LOGE("Cannot find getX/getRawX");
        }
        if (getRawY) {
            DobbyHook(getRawY, (void*)hook_getRawY, (void**)&old_getRawY);
            LOGI("Hooked getRawY at %p", getRawY);
        } else {
            LOGE("Cannot find getY/getRawY");
        }
        if (getPointerId) {
            DobbyHook(getPointerId, (void*)hook_getPointerId, (void**)&old_getPointerId);
            LOGI("Hooked getPointerId at %p", getPointerId);
        }
        // 注意：不要 dlclose(libandroid)，避免地址失效
    } else {
        LOGE("Failed to load libandroid.so");
    }

    LOGI("Init thread finished");
    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t thread;
    pthread_create(&thread, nullptr, init_thread, nullptr);
    pthread_detach(thread);
}
