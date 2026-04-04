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
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

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
    ImGui::Begin("JKMenu - SGame Debug", &g_ShowMenu);

    ImGui::Text("Device: OnePlus Pad Pro 2");
    ImGui::Text("Status: Hook Active");
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
        LOGI("Hooked eglSwapBuffers called for the first time!");
        
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_Height);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);

        if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
            g_Initialized = true;
            LOGI("ImGui Renderer Initialized: %dx%d", g_Width, g_Height);
        } else {
            LOGE("ImGui Renderer Initialization FAILED!");
        }
    }

    // 开始新帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    DrawImGuiMenu();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return orig_eglSwapBuffers(dpy, surface);
}

// 注入后的初始化线程
void* hack_thread(void*) {
    LOGI("SO Loaded! Hack thread started.");
    
    // 增加等待时间，确保游戏引擎初始化完成
    sleep(10);

    // 尝试寻找 eglSwapBuffers
    void* target = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (!target) {
        // 备选方案：尝试直接从当前进程获取地址
        target = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    }

    if (target) {
        LOGI("Target eglSwapBuffers found at: %p, starting hook...", target);
        
        DobbyHook(target, 
                  (dobby_dummy_func_t)hooked_eglSwapBuffers, 
                  (dobby_dummy_func_t*)&orig_eglSwapBuffers);
        
        if (orig_eglSwapBuffers) {
            LOGI("Dobby Hook Applied Successfully!");
        } else {
            LOGE("Dobby Hook Failed to apply!");
        }
    } else {
        LOGE("CRITICAL: eglSwapBuffers not found in libEGL.so or RTLD_DEFAULT");
    }

    return nullptr;
}

// .so 加载时的入口点
void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, hack_thread, nullptr);
}
