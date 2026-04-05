#include <jni.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "JKMenu_SGame"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool g_Initialized = false;
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
static eglSwapBuffers_t old_eglSwapBuffers = nullptr;

void DrawJKMenu() {
    if (!ImGui::GetCurrentContext()) return;
    ImGui::Begin("JKMenu for SGame", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Build Date: 2026-04-06");
    ImGui::Separator();
    static bool hack_enabled = false;
    ImGui::Checkbox("功能开关示例", &hack_enabled);
    ImGui::End();
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplAndroid_Init(nullptr);
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
        LOGI("ImGui Initialized in SGame process.");
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
    // 王者荣耀启动较慢，建议延迟 10 秒等待渲染库加载完毕
    sleep(10); 
    
    void* egl_addr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl_addr) {
        DobbyHook(egl_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Hook eglSwapBuffers Success!");
    }
    return nullptr;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    pthread_t tid;
    pthread_create(&tid, nullptr, init_thread, nullptr);
    return JNI_VERSION_1_6;
}
