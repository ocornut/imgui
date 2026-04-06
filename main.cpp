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
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// --- 全局状态 ---
static bool g_Initialized = false;
static bool g_ShowMenu = true;
static float g_Width = 0, g_Height = 0;

struct TouchState {
    float x, y;
    bool down;
    // 用于简单去抖或日志
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
// 真正实时更新坐标和触摸状态的 Hook 函数
// ---------------------------------------------------------------------

// 辅助函数：从事件中提取动作并更新 down 状态
static void UpdateTouchFromEvent(const AInputEvent* event, size_t pointer_index) {
    if (!event || AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION)
        return;
    
    int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    int32_t pointerId = AMotionEvent_getPointerId(event, pointer_index);
    // 简单起见，只跟踪第一个手指 (pointer_index == 0)
    // 如果游戏使用多指，可以根据需要扩展
    if (pointer_index == 0) {
        if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            g_Touch.down = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP ||
                   action == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down = false;
        }
        // MOVE 事件时保持 down 状态不变（已经为 true）
    }
}

// Hook getRawX: 每次游戏获取 X 坐标时更新 x 和触摸状态
float hook_getRawX(const AInputEvent* event, size_t index) {
    if (event && old_getRawX) {
        // 先获取原始 X 坐标
        float x = old_getRawX(event, index);
        // 更新全局触摸坐标
        g_Touch.x = x;
        // 更新触摸状态（down/up）
        UpdateTouchFromEvent(event, index);
        return x;
    }
    return 0.0f;
}

// Hook getRawY: 每次游戏获取 Y 坐标时更新 y
float hook_getRawY(const AInputEvent* event, size_t index) {
    if (event && old_getRawY) {
        float y = old_getRawY(event, index);
        g_Touch.y = y;
        // 再次更新状态（可选，和上面重复但无害）
        UpdateTouchFromEvent(event, index);
        return y;
    }
    return 0.0f;
}

// 保留对 getPointerId 的 Hook，但只转发调用，不再更新坐标（避免重复）
int32_t hook_getPointerId(const AInputEvent* event, size_t index) {
    // 简单转发，不做额外处理
    return old_getPointerId ? old_getPointerId(event, index) : 0;
}

// ---------------------------------------------------------------------
// 渲染 Hook
// ---------------------------------------------------------------------
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // 获取屏幕尺寸（仅一次或尺寸改变时更新）
    if (g_Width <= 0 || g_Height <= 0) {
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        g_Width = (float)w;
        g_Height = (float)h;
        LOGI("Screen size: %.0f x %.0f", g_Width, g_Height);
    }

    // 初始化 ImGui（只一次）
    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        // 可选：调整样式以适应触摸屏
        ImGui::GetStyle().TouchPadding = ImVec2(10, 10);
        ImGui::GetStyle().ScaleAllSizes(2.5f);
        g_Initialized = true;
        LOGI("ImGui initialized");
    }

    // 开始 ImGui 帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 更新 ImGui IO 的鼠标输入
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Width, g_Height);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    // 可选：打印调试信息（每 120 帧打印一次，避免刷屏）
    if (g_Touch.frameCounter++ % 120 == 0) {
        LOGI("Touch: (%.1f, %.1f) down=%d", g_Touch.x, g_Touch.y, g_Touch.down);
    }

    // 绘制菜单
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
        // 可选：显示一个小的重新打开按钮（例如通过音量键或触摸区域，这里简单通过坐标判断）
        // 为了演示，添加一个“Show”按钮在左上角（即使菜单关闭也显示一个小区域）
        // 实际使用时建议用系统按键回调，这里简化处理
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

    // 渲染
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 调用原始 eglSwapBuffers
    return old_eglSwapBuffers(dpy, surface);
}

// ---------------------------------------------------------------------
// 初始化线程
// ---------------------------------------------------------------------
void* init_thread(void*) {
    LOGI("Init thread started");

    // 1. Hook eglSwapBuffers（渲染）
    void* eglFunc = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (eglFunc) {
        DobbyHook(eglFunc, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Hooked eglSwapBuffers at %p", eglFunc);
    } else {
        LOGE("Failed to find eglSwapBuffers");
    }

    // 2. Hook Android 输入函数（关键修复点）
    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        // 获取需要 Hook 的函数地址
        void* getPointerId = dlsym(libandroid, "AMotionEvent_getPointerId");
        void* getRawX = dlsym(libandroid, "AMotionEvent_getRawX");
        void* getRawY = dlsym(libandroid, "AMotionEvent_getRawY");

        // 如果找不到 RawX/RawY，回退到普通 getX/getY（相对坐标，可能不准）
        if (!getRawX) {
            getRawX = dlsym(libandroid, "AMotionEvent_getX");
            LOGI("Using AMotionEvent_getX as fallback");
        }
        if (!getRawY) {
            getRawY = dlsym(libandroid, "AMotionEvent_getY");
            LOGI("Using AMotionEvent_getY as fallback");
        }

        // Hook getRawX 和 getRawY（这是实时更新坐标的关键）
        if (getRawX) {
            DobbyHook(getRawX, (void*)hook_getRawX, (void**)&old_getRawX);
            LOGI("Hooked getRawX at %p", getRawX);
        } else {
            LOGE("Cannot find getX/getRawX function");
        }
        if (getRawY) {
            DobbyHook(getRawY, (void*)hook_getRawY, (void**)&old_getRawY);
            LOGI("Hooked getRawY at %p", getRawY);
        } else {
            LOGE("Cannot find getY/getRawY function");
        }

        // Hook getPointerId 保持兼容（可选）
        if (getPointerId) {
            DobbyHook(getPointerId, (void*)hook_getPointerId, (void**)&old_getPointerId);
            LOGI("Hooked getPointerId at %p", getPointerId);
        }

        dlclose(libandroid); // 注意：dlclose 可能使 dlsym 返回的地址失效，但 DobbyHook 已复制指令，通常安全。建议不要 dlclose。
        // 更安全的做法是不调用 dlclose，保持 libandroid 加载。这里注释掉 dlclose。
        // dlclose(libandroid);
    } else {
        LOGE("Failed to load libandroid.so");
    }

    LOGI("Init thread finished");
    return nullptr;
}

// ---------------------------------------------------------------------
// 入口点（构造函数）
// ---------------------------------------------------------------------
__attribute__((constructor))
void entry() {
    pthread_t thread;
    pthread_create(&thread, nullptr, init_thread, nullptr);
    pthread_detach(thread); // 让线程自动回收
}
