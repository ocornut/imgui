#include <jni.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
#include <android/input.h>
#include <android/keycodes.h>

// ImGui 核心与渲染器
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <GLES3/gl3.h>
#include <EGL/egl.h>

#define LOG_TAG "JKMenu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 原始函数指针
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surface);
eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

// 触摸 Hook 相关 (针对 Android Native 注入)
typedef int (*InputEvent_t)(void* event);
InputEvent_t orig_InputEvent = nullptr;

// 全局状态
bool g_Initialized = false;
bool g_ShowMenu = true;
int g_Width = 0;
int g_Height = 0;

// 处理触摸事件
void HandleInput(void* event) {
    if (!g_Initialized) return;
    
    ImGuiIO& io = ImGui::GetIO();
    int32_t type = AInputEvent_getType((AInputEvent*)event);
    
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction((AInputEvent*)event) & AMOTION_EVENT_ACTION_MASK;
        float x = AMotionEvent_getX((AInputEvent*)event, 0);
        float y = AMotionEvent_getY((AInputEvent*)event, 0);
        
        io.MousePos = ImVec2(x, y);
        
        if (action == AMOTION_EVENT_ACTION_DOWN) io.MouseDown[0] = true;
        else if (action == AMOTION_EVENT_ACTION_UP) io.MouseDown[0] = false;
        
        // 如果菜单显示，截断触摸，不让点击穿透到游戏
        if (g_ShowMenu && ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
            // 这里根据实际情况处理是否消费事件
        }
    }
}

// Hook 触摸事件 (尝试拦截 InputEvent)
int hooked_InputEvent(void* event) {
    HandleInput(event);
    return orig_InputEvent(event);
}

// 菜单绘制内容
void DrawImGuiMenu() {
    if (!g_ShowMenu) return;

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("JKMenu - SGame Test", &g_ShowMenu);

    ImGui::Text("设备: OnePlus Pad Pro 2");
    ImGui::Text("包名: com.tencent.jkchess");
    ImGui::Text("分辨率: %dx%d", g_Width, g_Height);
    ImGui::Separator();

    static bool feature1 = false;
    ImGui::Checkbox("功能开关 1", &feature1);
    
    static float menu_scale = 1.0f;
    if (ImGui::SliderFloat("菜单大小", &menu_scale, 0.5f, 2.0f)) {
        ImGui::GetIO().FontGlobalScale = 2.0f * menu_scale;
    }

    if (ImGui::Button("退出菜单")) {
        g_ShowMenu = false;
    }

    ImGui::End();
}

// 核心渲染 Hook
EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_Height);

        if (g_Width > 0 && g_Height > 0) {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            
            // 解决中文乱码：加载字体 (Android 环境建议加载默认字体)
            ImFontConfig font_cfg;
            font_cfg.SizePixels = 24.0f;
            io.Fonts->AddFontDefault(&font_cfg);
            // 注意：要显示中文，通常需要加载字库文件，这里先通过 io.IniFilename 尝试初始化环境
            
            io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);
            io.FontGlobalScale = 2.5f; 

            if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
                g_Initialized = true;
                LOGI("ImGui GLES Initialized!");
            }
        }
    }

    if (g_Initialized) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        DrawImGuiMenu();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return orig_eglSwapBuffers(dpy, surface);
}

void* hack_thread(void*) {
    LOGI("SO Loaded! Start Hooking...");
    sleep(12);

    // 1. Hook 渲染
    void* swap_ptr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (swap_ptr) {
        DobbyHook(swap_ptr, (dobby_dummy_func_t)hooked_eglSwapBuffers, (dobby_dummy_func_t*)&orig_eglSwapBuffers);
    }

    // 2. Hook 触摸 (寻找 Input 模块，这在不同 Android 版本可能不同)
    // 尝试拦截 libinput.so 或相关处理函数
    void* input_ptr = DobbySymbolResolver("libandroid.so", "AInputQueue_getEvent");
    if (input_ptr) {
        LOGI("Found Input Queue hook point.");
        // 注意：触摸 Hook 通常比渲染复杂，需要根据游戏具体使用的 Input 库来定
    }

    return nullptr;
}

void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, hack_thread, nullptr);
}
