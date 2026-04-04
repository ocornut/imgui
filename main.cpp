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
    ImGui::Begin("JKMenu - SGame Multi-Hook", &g_ShowMenu);

    ImGui::Text("Device: OnePlus Pad Pro 2");
    ImGui::Text("Target: com.tencent.tmgp.sgame");
    ImGui::Text("Screen: %dx%d", g_Width, g_Height);
    ImGui::Separator();

    static float f = 0.5f;
    ImGui::SliderFloat("Menu Scale", &f, 0.1f, 2.0f);

    if (ImGui::Button("Hide Menu")) {
        g_ShowMenu = false;
    }

    ImGui::End();
}

// 核心渲染执行器
void PerformRender(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        LOGI("SUCCESS! A Hooked Render function was triggered.");
        
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_Height);

        if (g_Width <= 0 || g_Height <= 0) return;

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);

        if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
            g_Initialized = true;
            LOGI("ImGui Initialized: %dx%d", g_Width, g_Height);
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

// Hook 回调函数 (通用)
EGLBoolean hooked_universal_swap(EGLDisplay dpy, EGLSurface surface) {
    PerformRender(dpy, surface);
    return orig_eglSwapBuffers(dpy, surface);
}

// 动态全量 Hook 逻辑
void* hack_thread(void*) {
    LOGI("SO Loaded! Waiting for engine to settle...");
    sleep(10);

    // 王者荣耀/金铲铲等腾讯游戏可能使用的渲染函数候选名单
    const char* symbol_list[] = {
        "eglSwapBuffers",           // 标准 EGL
        "nw_main_swap_buffers",     // 腾讯常用渲染 Hook 点
        "GameCanvas_SwapBuffers",   // 部分引擎私有实现
        "_Z22nw_main_swap_buffersP10EGLDisplayP10EGLSurface" // 混淆后的符号名
    };

    void* target = nullptr;

    for (const char* sym : symbol_list) {
        // 尝试从不同的库寻找
        target = DobbySymbolResolver("libEGL.so", sym);
        if (!target) target = DobbySymbolResolver("libGLESv2.so", sym);
        if (!target) target = DobbySymbolResolver("libunity.so", sym); // 针对 Unity 游戏
        if (!target) target = dlsym(RTLD_DEFAULT, sym);

        if (target) {
            LOGI("Found potential render target: %s at %p", sym, target);
            DobbyHook(target, (dobby_dummy_func_t)hooked_universal_swap, (dobby_dummy_func_t*)&orig_eglSwapBuffers);
            if (orig_eglSwapBuffers) {
                LOGI("Successfully hooked: %s", sym);
                // 只要成功 Hook 到了一个，通常就不需要再 Hook 其他的了
                // 但如果游戏有多个渲染管线，可以继续
            }
        }
    }

    if (!orig_eglSwapBuffers) {
        LOGE("CRITICAL: Failed to find ANY render function. Game might be using Vulkan.");
    }

    return nullptr;
}

void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, hack_thread, nullptr);
}
