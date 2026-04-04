#include <jni.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
#include <atomic>

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

typedef void* (*vkQueuePresentKHR_t)(void* queue, const void* pPresentInfo);
vkQueuePresentKHR_t orig_vkQueuePresentKHR = nullptr;

// --- 计数器：用于判断哪个渲染器在工作 ---
std::atomic<int> g_gles_calls{0};
std::atomic<int> g_vulkan_calls{0};

// --- 全局状态 ---
bool g_Initialized = false;
bool g_ShowMenu = true;
int g_Width = 0;
int g_Height = 0;

// 菜单渲染逻辑 (GLES)
void DrawImGuiMenu() {
    if (!g_ShowMenu) return;
    ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
    ImGui::Begin("JKMenu - Renderer Detector", &g_ShowMenu);
    ImGui::Text("Target: com.tencent.jkchess");
    ImGui::Separator();
    ImGui::Text("GLES Call Count: %d", g_gles_calls.load());
    ImGui::Text("Vulkan Call Count: %d", g_vulkan_calls.load());
    ImGui::Separator();
    if (g_vulkan_calls > 0 && g_gles_calls == 0) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "STATUS: VULKAN ACTIVE");
        ImGui::Text("Menu cannot show in GLES mode.");
    } else {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "STATUS: GLES ACTIVE");
        static float f = 0.5f;
        ImGui::SliderFloat("Scale", &f, 0.1f, 2.0f);
    }
    ImGui::End();
}

// Hook 回调: OpenGL
EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    g_gles_calls++;
    if (!g_Initialized) {
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_Height);
        if (g_Width > 0 && g_Height > 0) {
            ImGui::CreateContext();
            if (ImGui_ImplOpenGL3_Init("#version 300 es")) g_Initialized = true;
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

// Hook 回调: Vulkan
void* hooked_vkQueuePresentKHR(void* queue, const void* pPresentInfo) {
    g_vulkan_calls++;
    return orig_vkQueuePresentKHR(queue, pPresentInfo);
}

// 探测线程
void* detector_thread(void*) {
    LOGI("Detector started. Analyzing rendering pipeline...");
    sleep(10); // 等待游戏进入主画面

    // Hook 两个可能的入口点
    void* gles_ptr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (gles_ptr) DobbyHook(gles_ptr, (dobby_dummy_func_t)hooked_eglSwapBuffers, (dobby_dummy_func_t*)&orig_eglSwapBuffers);

    void* vk_ptr = DobbySymbolResolver("libvulkan.so", "vkQueuePresentKHR");
    if (vk_ptr) DobbyHook(vk_ptr, (dobby_dummy_func_t)hooked_vkQueuePresentKHR, (dobby_dummy_func_t*)&orig_vkQueuePresentKHR);

    // 每 3 秒输出一次探测结果
    while (true) {
        int gles = g_gles_calls.exchange(0);
        int vk = g_vulkan_calls.exchange(0);

        if (gles > 0) {
            LOGI("[Detector] Result: OpenGL (GLES) is ACTIVE (%d fps)", gles);
        } else if (vk > 0) {
            LOGE("[Detector] Result: VULKAN is ACTIVE (%d fps). GLES Hook will not work!", vk);
        } else {
            LOGI("[Detector] Result: No rendering detected yet...");
        }
        sleep(3);
    }
    return nullptr;
}

void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, detector_thread, nullptr);
}
