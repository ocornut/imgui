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

// 存储触摸数据
struct {
    float x, y;
    bool down;
} g_Touch = {0, 0, false};

// --- 原函数指针 ---
typedef int32_t (*p_getPointerId)(const AInputEvent* motion_event, size_t pointer_index);
static p_getPointerId old_getPointerId = nullptr;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 【GitHub 常用方案】直接从 PointerId 入手 ---
int32_t hook_getPointerId(const AInputEvent* event, size_t index) {
    if (g_ShowMenu && event != nullptr) {
        int type = AInputEvent_getType(event);
        if (type == AINPUT_EVENT_TYPE_MOTION) {
            int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
            
            // 获取手指 0 的位置
            float rawX = AMotionEvent_getX(event, 0);
            float rawY = AMotionEvent_getY(event, 0);

            // 只有当坐标有效时才更新，防止跳动
            if (rawX >= 0 && rawY >= 0) {
                g_Touch.x = rawX;
                g_Touch.y = rawY;

                if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
                    g_Touch.down = true;
                } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
                    g_Touch.down = false;
                }
            }
            
            // 如果菜单显示，且触摸在菜单范围内，你可以选择性拦截 (此处暂不拦截以保证游戏操作)
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
        
        // 样式适配
        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        
        g_Initialized = true;
        LOGI("ImGui Context Ready. Resolution: %.fx%.f", g_Width, g_Height);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_Width, g_Height);
        
        // 【核心】将 Hook 到的坐标精准同步给 ImGui
        io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
        io.MouseDown[0] = g_Touch.down;

        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("AndKitty SGame Stable", &g_ShowMenu)) {
            ImGui::Text("Github Strategy: PointerId Monitoring");
            ImGui::Separator();
            
            ImGui::Text("MousePos: %.1f, %.1f", io.MousePos.x, io.MousePos.y);
            ImGui::Text("IsDown: %s", io.MouseDown[0] ? "True" : "False");
            
            ImGui::Separator();
            static float menu_alpha = 0.8f;
            ImGui::SliderFloat("Menu Alpha", &menu_alpha, 0.1f, 1.0f);
            
            if (ImGui::Button("Close Menu")) g_ShowMenu = false;
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Applying Stable Input Hooks...");
    
    // 渲染
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 输入: 参考 GitHub 方案，锁定 libandroid.so 中的 AMotionEvent_getPointerId
    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        void* getPointerId = dlsym(libandroid, "AMotionEvent_getPointerId");
        if (getPointerId) {
            DobbyHook(getPointerId, (void*)hook_getPointerId, (void**)&old_getPointerId);
            LOGI("Input Monitoring Active via PointerId");
        }
    }

    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
