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

#define LOG_TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- 全局状态 ---
static bool g_Initialized = false;
static bool g_ShowMenu = true;
static float g_Width = 0, g_Height = 0;

// 记录触摸
struct {
    float x, y;
    bool down;
} g_Touch = {0, 0, false};

// --- 原函数指针 ---
typedef int32_t (*p_getPointerId)(const AInputEvent* motion_event, size_t pointer_index);
static p_getPointerId old_getPointerId = nullptr;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 稳定的坐标获取方案 ---
int32_t hook_getPointerId(const AInputEvent* event, size_t index) {
    if (g_ShowMenu && event != nullptr) {
        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
            int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
            
            // 获取手指 0 的绝对坐标
            float x = AMotionEvent_getX(event, 0);
            float y = AMotionEvent_getY(event, 0);

            // 过滤非法值，防止之前出现的天文数字
            if (x >= 0 && y >= 0 && x < 4000 && y < 4000) {
                g_Touch.x = x;
                g_Touch.y = y;

                if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
                    g_Touch.down = true;
                } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
                    g_Touch.down = false;
                }
            }
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
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_Width, g_Height);
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        // 注意：不调用 ImGui_ImplAndroid_Init，我们手动处理 IO
        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        
        g_Initialized = true;
        LOGI("ImGui Simple Ready: %.fx%.f", g_Width, g_Height);
    }

    // 核心：强制同步 IO 状态
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;
    
    // 固定的 DeltaTime 保证拖拽逻辑不失效
    io.DeltaTime = 1.0f / 60.0f;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("AndKitty SGame Lite", &g_ShowMenu)) {
            ImGui::Text("Diagnosis:");
            ImGui::BulletText("Pos: %.1f, %.1f", io.MousePos.x, io.MousePos.y);
            ImGui::BulletText("Down: %s", io.MouseDown[0] ? "True" : "False");
            
            ImGui::Separator();
            static float f = 0.5f;
            ImGui::SliderFloat("Slider Test", &f, 0.0f, 1.0f);
            
            if (ImGui::Button("Close Menu")) g_ShowMenu = false;
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    // 注入后只等 2 秒，确保 libandroid.so 加载
    sleep(2);

    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        void* getPointerId = dlsym(libandroid, "AMotionEvent_getPointerId");
        if (getPointerId) {
            DobbyHook(getPointerId, (void*)hook_getPointerId, (void**)&old_getPointerId);
            LOGI("Touch Hook Applied.");
        }
    }

    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
