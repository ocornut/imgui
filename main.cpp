#include <jni.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>

// 核心库包含
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include <GLES3/gl3.h>
#include <EGL/egl.h>

#define LOG_TAG "JKMenu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 原始函数指针存储
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surface);
eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

// 全局状态
bool g_Initialized = false;
bool g_ShowMenu = true;
int g_Width = 0;
int g_Height = 0;

// 菜单渲染逻辑
void DrawImGuiMenu() {
    if (!g_ShowMenu) return;

    ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
    ImGui::Begin("JKMenu - com.tencent.tmgp.sgame", &g_ShowMenu);

    ImGui::Text("Device: OnePlus Pad Pro 2");
    ImGui::Text("Screen: %dx%d", g_Width, g_Height);
    ImGui::Separator();

    if (ImGui::Button("功能开关 1")) {
        LOGI("Feature 1 Toggled");
    }

    if (ImGui::Button("隐藏菜单")) {
        g_ShowMenu = false;
    }

    ImGui::End();
}

// Hook 后的渲染函数
EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        // 获取当前 Surface 的宽高
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_Height);

        // 初始化 ImGui 上下文
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        // 设置显示大小
        io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);

        // 初始化后端
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        g_Initialized = true;
        LOGI("ImGui Initialized Successfully for SGame");
    }

    // 开始新帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 绘制 UI
    DrawImGuiMenu();

    // 渲染输出
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return orig_eglSwapBuffers(dpy, surface);
}

// 注入后的初始化线程
void* hack_thread(void*) {
    LOGI("Hack Thread Started");
    
    // 等待游戏加载 libEGL.so
    sleep(5);

    // 使用 Dobby 寻找目标函数地址
    void* target = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (target) {
        LOGI("Found eglSwapBuffers at: %p", target);
        
        // 执行 Inline Hook
        DobbyHook(target, 
                  (dobby_dummy_func_t)hooked_eglSwapBuffers, 
                  (dobby_dummy_func_t*)&orig_eglSwapBuffers);
        
        LOGI("Dobby Hook Applied");
    } else {
        LOGI("Failed to find target function");
    }

    return nullptr;
}

// .so 加载时的入口点
void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, hack_thread, nullptr);
}
