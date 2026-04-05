#include <jni.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>     // 必须包含，修复 sleep 报错
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <vulkan/vulkan.h>

#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "JKMenu_Core"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool g_Initialized = false;
static bool g_VulkanMode  = false;

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
static eglSwapBuffers_t old_eglSwapBuffers = nullptr;

// 简单的 ImGui 菜单界面
void DrawJKMenu() {
    if (!ImGui::GetCurrentContext()) return;
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("JKMenu - Version 1.0", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Render: %s", g_VulkanMode ? "Vulkan (Limited Support)" : "GLES 3.0 (Active)");
    ImGui::Separator();
    if (ImGui::Button("Log Test")) { LOGI("ImGui Button Clicked!"); }
    ImGui::End();
}

// OpenGL 渲染 Hook
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplAndroid_Init(nullptr);
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
    DrawJKMenu();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surf);
}

void* init_thread(void*) {
    LOGI("JKMenu init_thread started...");
    sleep(5); // 等待游戏完全进入

    // 探测 Vulkan 驱动
    if (dlopen("vulkan.adreno.so", RTLD_NOLOAD)) {
        g_VulkanMode = true;
        LOGI("Detected Vulkan. Switch to Compatibility Mode if menu is invisible.");
    }

    // 挂载 EGL Hook (libEGL.so 在 Android 兼容模式下必用)
    void* egl_addr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl_addr) {
        DobbyHook(egl_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("GLES Hooked successfully.");
    }
    return nullptr;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    pthread_t tid;
    pthread_create(&tid, nullptr, init_thread, nullptr);
    return JNI_VERSION_1_6;
}
