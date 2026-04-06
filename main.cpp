#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
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
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        LOGI("Successfully Entered Hook!");
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Menu");
    ImGui::Text("Success!");
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Step 1: Thread Started, sleeping 10s...");
    sleep(10); 

    LOGI("Step 2: Searching for eglSwapBuffers...");
    // 直接查找，不进行复杂的 dlopen
    void* sym_addr = (void*)eglSwapBuffers; 
    
    if (sym_addr) {
        LOGI("Step 3: Found addr at %p, Hooking...", sym_addr);
        DobbyHook(sym_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Step 4: All Done!");
    } else {
        LOGI("Step 3: Failed to find symbol!");
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
