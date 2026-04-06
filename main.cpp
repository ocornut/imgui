#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool g_Initialized = false;
static bool g_ShowMenu = true;

// 触摸状态全局变量
struct {
    float x;
    float y;
    bool down;
    int count;
} g_TouchData = {0, 0, false, 0};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 针对 Unity 绕过导出的最强 Hook 方案 ---

typedef int32_t (*p_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** outEvent);
static p_AInputQueue_getEvent old_getEvent = nullptr;

int32_t hook_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** outEvent) {
    int32_t res = old_getEvent(queue, outEvent);
    if (res >= 0 && outEvent && *outEvent) {
        AInputEvent* event = *outEvent;
        int32_t type = AInputEvent_getType(event);
        
        if (type == AINPUT_EVENT_TYPE_MOTION) {
            int32_t action = AMotionEvent_getAction(event);
            int32_t code = action & AMOTION_EVENT_ACTION_MASK;
            
            // 直接在这里调用系统函数读取（即使 Unity 自己不调用，我们可以在 Hook 里主动读）
            g_TouchData.x = AMotionEvent_getX(event, 0);
            g_TouchData.y = AMotionEvent_getY(event, 0);
            
            if (code == AMOTION_EVENT_ACTION_DOWN || code == AMOTION_EVENT_ACTION_POINTER_DOWN) {
                g_TouchData.down = true;
            } else if (code == AMOTION_EVENT_ACTION_UP || code == AMOTION_EVENT_ACTION_POINTER_UP || code == AMOTION_EVENT_ACTION_CANCEL) {
                g_TouchData.down = false;
            }
            g_TouchData.count++;
        }
    }
    return res;
}

// 渲染钩子
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        EGLint width, height;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
        
        float scale = (float)width / 1920.0f;
        ImGui::GetStyle().ScaleAllSizes(scale > 1.2f ? scale : 1.2f);
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
        LOGI("ImGui Initialized: %d x %d", width, height);
    }

    if (g_ShowMenu) {
        // 【核心】在渲染前强行同步触摸状态
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = ImVec2(g_TouchData.x, g_TouchData.y);
        io.MouseDown[0] = g_TouchData.down;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty SGame Debugger", &g_ShowMenu)) {
            ImGui::Text("Captured: %d", g_TouchData.count);
            ImGui::Text("Pos: %.1f, %.1f", g_TouchData.x, g_TouchData.y);
            ImGui::Text("Status: %s", g_TouchData.down ? "DOWN" : "UP");
            ImGui::Separator();
            
            static bool test = false;
            ImGui::Checkbox("Test Feature", &test);
            
            if (ImGui::Button("Close Menu")) g_ShowMenu = false;
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    // 既然日志显示 08:57 已经加载了 libunity.so，我们缩短等待
    sleep(5); 

    // 1. Hook 渲染
    void* swap_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (swap_addr) DobbyHook(swap_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 2. Hook AInputQueue_getEvent
    // 它是 Unity 拿事件的总阀门，不管 Unity 内部怎么读，只要它拿了，我们就拦
    void* getEvent_addr = dlsym(RTLD_DEFAULT, "AInputQueue_getEvent");
    if (!getEvent_addr) {
        void* h_android = dlopen("libandroid.so", RTLD_NOW);
        if (h_android) getEvent_addr = dlsym(h_android, "AInputQueue_getEvent");
    }

    if (getEvent_addr) {
        LOGI("Hooking AInputQueue_getEvent...");
        DobbyHook(getEvent_addr, (void*)hook_AInputQueue_getEvent, (void**)&old_getEvent);
    }

    LOGI("New Hook Strategy Applied.");
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
