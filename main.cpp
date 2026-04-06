#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <string>
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

// --- 触摸同步逻辑 ---

void SyncImGuiInput(AInputEvent* event) {
    if (!g_Initialized || !g_ShowMenu) return;

    ImGuiIO& io = ImGui::GetIO();
    int32_t type = AInputEvent_getType(event);

    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        int32_t action_code = action & AMOTION_EVENT_ACTION_MASK;
        
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        io.MousePos = ImVec2(x, y);

        if (action_code == AMOTION_EVENT_ACTION_DOWN || action_code == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            io.MouseDown[0] = true;
        } else if (action_code == AMOTION_EVENT_ACTION_UP || action_code == AMOTION_EVENT_ACTION_POINTER_UP || action_code == AMOTION_EVENT_ACTION_CANCEL) {
            io.MouseDown[0] = false;
        }
    }
}

// --- Hook 1: AInputQueue (通用入口) ---
typedef int32_t (*p_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** outEvent);
static p_AInputQueue_getEvent old_AInputQueue_getEvent = nullptr;

int32_t hook_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** outEvent) {
    int32_t res = old_AInputQueue_getEvent(queue, outEvent);
    if (res >= 0 && outEvent && *outEvent) {
        SyncImGuiInput(*outEvent);
    }
    return res;
}

// --- Hook 2: 直接 Hook AMotionEvent 获取函数 (防止 libunity.so 绕过 Queue) ---
typedef float (*p_AMotionEvent_getX)(const AInputEvent* motion_event, size_t pointer_index);
static p_AMotionEvent_getX old_getX = nullptr;

float hook_AMotionEvent_getX(const AInputEvent* motion_event, size_t pointer_index) {
    float x = old_getX(motion_event, pointer_index);
    if (g_ShowMenu && g_Initialized && pointer_index == 0) {
        ImGui::GetIO().MousePos.x = x;
    }
    return x;
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
        LOGI("ImGui Initialized for Unity SGame: %d x %d", width, height);
    }

    if (g_ShowMenu) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_FirstUseEver);
        
        // 确保没有任何禁止移动的 Flag
        if (ImGui::Begin("AndKitty SGame Unity", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(0, 1, 1, 1), "Module: libunity.so Context");
            ImGui::Separator();
            
            static bool esp = false;
            ImGui::Checkbox("ESP Enable", &esp);
            
            if (ImGui::Button("Close Menu")) g_ShowMenu = false;
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Waiting for libunity.so and libandroid.so...");
    
    // 循环等待 libunity.so 加载完成，这对 Unity 游戏很重要
    while (true) {
        if (dlsym(RTLD_DEFAULT, "eglSwapBuffers") && dlopen("libunity.so", RTLD_NOLOAD)) {
            break;
        }
        usleep(500000);
    }
    sleep(5); // 额外等待确保引擎初始化

    // 1. Hook 渲染
    void* swap_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    DobbyHook(swap_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 2. Hook 系统输入队列 (通用)
    void* input_addr = dlsym(RTLD_DEFAULT, "AInputQueue_getEvent");
    if (input_addr) {
        DobbyHook(input_addr, (void*)hook_AInputQueue_getEvent, (void**)&old_AInputQueue_getEvent);
    }

    // 3. 针对 Unity 的“强制同步”：Hook AMotionEvent_getX
    // 很多版本的 Unity 直接调用这个函数获取坐标，Hook 这里可以强行同步 ImGui 坐标
    void* getx_addr = dlsym(RTLD_DEFAULT, "AMotionEvent_getX");
    if (getx_addr) {
        LOGI("Hooking AMotionEvent_getX for Unity focus...");
        DobbyHook(getx_addr, (void*)hook_AMotionEvent_getX, (void**)&old_getX);
    }

    LOGI("Unity SGame Hooks Ready.");
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
