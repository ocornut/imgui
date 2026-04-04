#include <jni.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <string>

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

// --- 原始函数指针存储 ---
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surface);
eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

// Vulkan 相关的简单定义 (用于寻找符号)
typedef void* (*vkQueuePresentKHR_t)(void* queue, const void* pPresentInfo);
vkQueuePresentKHR_t orig_vkQueuePresentKHR = nullptr;

// --- 全局状态 ---
bool g_Initialized = false;
bool g_ShowMenu = true;
int g_Width = 0;
int g_Height = 0;

// 菜单渲染逻辑
void DrawImGuiMenu() {
    if (!g_ShowMenu) return;

    ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
    ImGui::Begin("JKMenu - Dual Renderer Support", &g_ShowMenu);

    ImGui::Text("Device: OnePlus Pad Pro 2");
    ImGui::Text("Target: com.tencent.jkchess");
    ImGui::Text("Screen: %dx%d", g_Width, g_Height);
    ImGui::Separator();

    static float f = 0.5f;
    ImGui::SliderFloat("Scale", &f, 0.1f, 2.0f);

    if (ImGui::Button("Hide Menu")) {
        g_ShowMenu = false;
    }

    ImGui::End();
}

// OpenGL 渲染执行器
void PerformRenderGLES(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        LOGI("SUCCESS! OpenGL Render function triggered.");
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_Height);
        if (g_Width <= 0 || g_Height <= 0) return;

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);
        if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
            g_Initialized = true;
            LOGI("ImGui GLES Initialized: %dx%d", g_Width, g_Height);
        }
    }

    if (g_Initialized) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        DrawImGuiMenu();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

// Hook 回调: OpenGL
EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    PerformRenderGLES(dpy, surface);
    return orig_eglSwapBuffers(dpy, surface);
}

// Hook 回调: Vulkan (仅做日志记录，提醒用户切换模式)
void* hooked_vkQueuePresentKHR(void* queue, const void* pPresentInfo) {
    static bool logged_vk = false;
    if (!logged_vk) {
        LOGI("DETECTED: Game is using Vulkan Renderer! ImGui GLES will NOT show.");
        LOGI("Please disable 'High Frame Rate' or 'Ultra Graphics' in game settings.");
        logged_vk = true;
    }
    return orig_vkQueuePresentKHR(queue, pPresentInfo);
}

// 动态搜索并 Hook
void* hack_thread(void*) {
    LOGI("SO Loaded! Waiting 10s for engine...");
    sleep(10);

    // 1. Hook OpenGL (主要渲染器)
    const char* gles_syms[] = {"eglSwapBuffers", "nw_main_swap_buffers"};
    for (const char* sym : gles_syms) {
        void* target = DobbySymbolResolver("libEGL.so", sym);
        if (!target) target = dlsym(RTLD_DEFAULT, sym);
        if (target) {
            LOGI("Found GLES target: %s at %p", sym, target);
            DobbyHook(target, (dobby_dummy_func_t)hooked_eglSwapBuffers, (dobby_dummy_func_t*)&orig_eglSwapBuffers);
        }
    }

    // 2. Hook Vulkan (用于监测)
    void* vk_target = DobbySymbolResolver("libvulkan.so", "vkQueuePresentKHR");
    if (vk_target) {
        LOGI("Found Vulkan target: vkQueuePresentKHR at %p", vk_target);
        DobbyHook(vk_target, (dobby_dummy_func_t)hooked_vkQueuePresentKHR, (dobby_dummy_func_t*)&orig_vkQueuePresentKHR);
    }

    if (!orig_eglSwapBuffers && !orig_vkQueuePresentKHR) {
        LOGE("CRITICAL: No render symbols found at all.");
    }

    return nullptr;
}

void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, hack_thread, nullptr);
}
