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

// 触摸状态全局变量 (数据补偿池)
struct {
    float x;
    float y;
    bool down;
    int count;
} g_CompensatedTouch = {0, 0, false, 0};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 深度补偿 Hook: 即使 Unity 不走 Queue，只要它读坐标，我们就截获 ---

typedef float (*p_AMotionEvent_getRawX)(const AInputEvent* motion_event, size_t pointer_index);
static p_AMotionEvent_getRawX old_getRawX = nullptr;

typedef float (*p_AMotionEvent_getRawY)(const AInputEvent* motion_event, size_t pointer_index);
static p_AMotionEvent_getRawY old_getRawY = nullptr;

typedef int32_t (*p_AMotionEvent_getAction)(const AInputEvent* motion_event);
static p_AMotionEvent_getAction old_getAction = nullptr;

float hook_AMotionEvent_getRawX(const AInputEvent* motion_event, size_t pointer_index) {
    float x = old_getRawX(motion_event, pointer_index);
    if (g_ShowMenu && pointer_index == 0) {
        g_CompensatedTouch.x = x;
        g_CompensatedTouch.count++;
    }
    return x;
}

float hook_AMotionEvent_getRawY(const AInputEvent* motion_event, size_t pointer_index) {
    float y = old_getRawY(motion_event, pointer_index);
    if (g_ShowMenu && pointer_index == 0) {
        g_CompensatedTouch.y = y;
    }
    return y;
}

int32_t hook_AMotionEvent_getAction(const AInputEvent* motion_event) {
    int32_t action = old_getAction(motion_event);
    if (g_ShowMenu) {
        int32_t code = action & AMOTION_EVENT_ACTION_MASK;
        if (code == AMOTION_EVENT_ACTION_DOWN || code == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            g_CompensatedTouch.down = true;
        } else if (code == AMOTION_EVENT_ACTION_UP || code == AMOTION_EVENT_ACTION_POINTER_UP || code == AMOTION_EVENT_ACTION_CANCEL) {
            g_CompensatedTouch.down = false;
        }
    }
    return action;
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
        LOGI("ImGui Context Created.");
    }

    if (g_ShowMenu) {
        ImGuiIO& io = ImGui::GetIO();
        
        // 强制将补偿的触摸数据注入 ImGui
        io.MousePos = ImVec2(g_CompensatedTouch.x, g_CompensatedTouch.y);
        io.MouseDown[0] = g_CompensatedTouch.down;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty SGame Ultimate", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(0,1,1,1), "Direct Hook Active");
            ImGui::Text("Captured Count: %d", g_CompensatedTouch.count);
            ImGui::Text("Last Pos: %.1f, %.1f", g_CompensatedTouch.x, g_CompensatedTouch.y);
            ImGui::Text("Touch Status: %s", g_CompensatedTouch.down ? "HOLDING" : "IDLE");
            
            ImGui::Separator();
            if (ImGui::Button("Reset Trace")) g_CompensatedTouch.count = 0;
            
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
    // 等待 libunity.so 及其依赖库加载
    LOGI("Plugin thread: waiting for modules...");
    while (true) {
        if (dlopen("libunity.so", RTLD_NOLOAD)) break;
        usleep(500000);
    }
    sleep(10); 

    // 1. Hook 渲染
    void* swap_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (swap_addr) DobbyHook(swap_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 2. 深度 Hook libandroid.so 导出的原始函数
    // 即使 Unity 绕过了 AInputQueue，它读坐标时也大概率会调这些导出的 API
    void* h_android = dlopen("libandroid.so", RTLD_NOW);
    if (h_android) {
        void* getRawX = dlsym(h_android, "AMotionEvent_getRawX");
        void* getRawY = dlsym(h_android, "AMotionEvent_getRawY");
        void* getAction = dlsym(h_android, "AMotionEvent_getAction");

        if (getRawX) DobbyHook(getRawX, (void*)hook_AMotionEvent_getRawX, (void**)&old_getRawX);
        if (getRawY) DobbyHook(getRawY, (void*)hook_AMotionEvent_getRawY, (void**)&old_getRawY);
        if (getAction) DobbyHook(getAction, (void*)hook_AMotionEvent_getAction, (void**)&old_getAction);
        
        LOGI("Deep Touch Hooks applied.");
    }

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
