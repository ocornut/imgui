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

// 记录全局触摸，如果自动处理失败，我们将手动强制注入
struct {
    float x;
    float y;
    bool down;
    int count;
} g_ManualTouch = {0, 0, false, 0};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 针对王者荣耀 Unity 深度定制的输入拦截 ---

typedef int32_t (*p_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** outEvent);
static p_AInputQueue_getEvent old_getEvent = nullptr;

typedef void (*p_AInputQueue_finishEvent)(AInputQueue* queue, AInputEvent* event, int handled);
static p_AInputQueue_finishEvent old_finishEvent = nullptr;

// 核心：拦截事件分发
int32_t hook_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** outEvent) {
    int32_t res = old_getEvent(queue, outEvent);
    if (res >= 0 && outEvent && *outEvent) {
        AInputEvent* event = *outEvent;
        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
            g_ManualTouch.count++;
            
            // 尝试获取坐标
            g_ManualTouch.x = AMotionEvent_getX(event, 0);
            g_ManualTouch.y = AMotionEvent_getY(event, 0);
            
            int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
            if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
                g_ManualTouch.down = true;
            } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
                g_ManualTouch.down = false;
            }

            // 如果菜单显示，尝试交给 ImGui 处理
            if (g_ShowMenu && g_Initialized) {
                // 调用 ImGui 后端自带的处理器
                ImGui_ImplAndroid_HandleInputEvent(event, 1.0f, 1.0f);
            }
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
        
        // 缩放适配
        float scale = (float)width / 1920.0f;
        ImGui::GetStyle().ScaleAllSizes(scale > 1.2f ? scale : 1.2f);
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
        LOGI("ImGui Context Created: %d x %d", width, height);
    }

    if (g_ShowMenu) {
        ImGuiIO& io = ImGui::GetIO();
        
        // 【双重保险】如果后端处理没生效，手动覆盖状态
        io.MousePos = ImVec2(g_ManualTouch.x, g_ManualTouch.y);
        io.MouseDown[0] = g_ManualTouch.down;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty SGame Ultimate", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(0,1,0,1), "Event Count: %d", g_ManualTouch.count);
            ImGui::Text("Last Pos: %.1f, %.1f", g_ManualTouch.x, g_ManualTouch.y);
            ImGui::Text("Status: %s", g_ManualTouch.down ? "HOLDING" : "IDLE");
            
            ImGui::Separator();
            if (ImGui::Button("Reset Counter")) g_ManualTouch.count = 0;
            
            static bool esp = true;
            ImGui::Checkbox("Draw ESP", &esp);
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    // 针对王者荣耀，我们等待更久，等 libunity.so 完全展开
    sleep(12); 

    // 1. Hook 渲染
    void* swap_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (swap_addr) DobbyHook(swap_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 2. 尝试从 libandroid.so 强制获取符号
    void* h_android = dlopen("libandroid.so", RTLD_NOW);
    if (h_android) {
        void* getEvent_addr = dlsym(h_android, "AInputQueue_getEvent");
        if (getEvent_addr) {
            LOGI("AInputQueue_getEvent found in libandroid.so, Hooking...");
            DobbyHook(getEvent_addr, (void*)hook_AInputQueue_getEvent, (void**)&old_getEvent);
        }
    }

    LOGI("Ultimate Input Strategy Loaded.");
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
