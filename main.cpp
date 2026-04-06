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

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 触摸处理逻辑 ---
// 处理 Android 原始输入事件并同步给 ImGui
bool HandleAndroidInput(AInputEvent* event, ImGuiIO& io) {
    int32_t type = AInputEvent_getType(event);
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        
        io.MousePos = ImVec2(x, y);
        
        if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            io.MouseDown[0] = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP) {
            io.MouseDown[0] = false;
        }
        
        // 如果点击在 ImGui 窗口上，返回 true 以拦截事件，不传给游戏
        return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemHovered();
    }
    return false;
}

// Hook 目标：AInputQueue_getEvent (这是 Android Native 游戏获取触摸的最上层接口)
typedef int32_t (*p_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** outEvent);
static p_AInputQueue_getEvent old_getEvent = nullptr;

int32_t hook_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** outEvent) {
    int32_t res = old_getEvent(queue, outEvent);
    if (res >= 0 && g_ShowMenu && g_Initialized) {
        // 将事件发给 ImGui 处理
        if (HandleAndroidInput(*outEvent, ImGui::GetIO())) {
            // 如果 ImGui 消耗了此事件，我们可以在此处对 event 进行 finish 处理
            // 但为了稳定性，通常我们只同步坐标而不完全拦截，或者根据需要返回
        }
    }
    return res;
}

// 渲染钩子
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        LOGI("Initializing ImGui for SGame...");
        EGLint width, height;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
        
        // 针对王者高分辨率屏优化缩放
        float scale = (float)width / 1920.0f; 
        ImGui::GetStyle().ScaleAllSizes(scale > 1.2f ? scale : 1.2f);
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
        LOGI("ImGui Render Initialized: %d x %d", width, height);
    }

    if (g_ShowMenu) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(550, 380), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty Menu (SGame)", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Touch Status: Hooked");
            ImGui::Separator();
            
            static bool esp_line = false;
            ImGui::Checkbox("ESP Line", &esp_line);
            
            static float f_val = 0.5f;
            ImGui::SliderFloat("FOV", &f_val, 0.0f, 1.0f);
            
            if (ImGui::Button("Hide Menu")) {
                g_ShowMenu = false;
            }
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Plugin thread: Waiting 10s for SGame environment...");
    sleep(10); 

    // 1. Hook 渲染
    void* swap_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (swap_addr) {
        LOGI("Hooking eglSwapBuffers...");
        DobbyHook(swap_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    }

    // 2. Hook 输入 (针对王者荣耀这种 Native 架构)
    void* getEvent_addr = dlsym(RTLD_DEFAULT, "AInputQueue_getEvent");
    if (getEvent_addr) {
        LOGI("Hooking AInputQueue_getEvent for Touch...");
        DobbyHook(getEvent_addr, (void*)hook_AInputQueue_getEvent, (void**)&old_getEvent);
    } else {
        // 备选方案：尝试从 libandroid.so 加载
        void* h_android = dlopen("libandroid.so", RTLD_NOW);
        if (h_android) {
            getEvent_addr = dlsym(h_android, "AInputQueue_getEvent");
            if (getEvent_addr) {
                DobbyHook(getEvent_addr, (void*)hook_AInputQueue_getEvent, (void**)&old_getEvent);
            }
        }
    }

    LOGI("Hooking Process Completed. Ready for SGame.");
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
