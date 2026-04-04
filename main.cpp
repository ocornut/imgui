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
    ImGui::Begin("JKMenu - SGame Final Debug", &g_ShowMenu);

    ImGui::Text("Device: OnePlus Pad Pro 2");
    ImGui::Text("Status: Rendering...");
    ImGui::Text("Screen: %dx%d", g_Width, g_Height);
    ImGui::Separator();

    static float f = 0.0f;
    ImGui::SliderFloat("测试滑块", &f, 0.0f, 1.0f);

    if (ImGui::Button("隐藏菜单")) {
        g_ShowMenu = false;
    }

    ImGui::End();
}

// 统一渲染入口
void PerformRender(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        LOGI("Hooked Swap triggered! Initializing ImGui...");
        
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_Height);

        if (g_Width <= 0 || g_Height <= 0) return;

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);

        if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
            g_Initialized = true;
            LOGI("ImGui Renderer Initialized: %dx%d", g_Width, g_Height);
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

// Hook 后的 eglSwapBuffers
EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    PerformRender(dpy, surface);
    return orig_eglSwapBuffers(dpy, surface);
}

// 尝试 Hook 王者荣耀内部可能使用的其它渲染函数名
void* hack_thread(void*) {
    LOGI("SO Loaded! Searching for render symbols...");
    
    // 等待更久一点，确保 libGLESv2/libEGL 加载完全
    sleep(10);

    // 尝试多个可能的 Hook 点
    const char* symbols[] = {"eglSwapBuffers", "nw_main_swap_buffers"};
    void* target = nullptr;

    for (int i = 0; i < 2; i++) {
        target = DobbySymbolResolver("libEGL.so", symbols[i]);
        if (!target) target = DobbySymbolResolver("libGLESv2.so", symbols[i]);
        if (!target) target = dlsym(RTLD_DEFAULT, symbols[i]);

        if (target) {
            LOGI("Found target %s at: %p", symbols[i], target);
            DobbyHook(target, (dobby_dummy_func_t)hooked_eglSwapBuffers, (dobby_dummy_func_t*)&orig_eglSwapBuffers);
            if (orig_eglSwapBuffers) {
                LOGI("Hooked %s Successfully!", symbols[i]);
                // 如果 Hook 成功一个点，可以根据需要决定是否继续搜
            }
        }
    }

    if (!orig_eglSwapBuffers) {
        LOGE("Failed to hook any render function. Checking if Vulkan is used...");
    }

    return nullptr;
}

void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, hack_thread, nullptr);
}
