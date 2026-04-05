#include <jni.h>
#include <thread>
#include <chrono>
#include <dlfcn.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>   // 必须添加，提供 ANativeWindow_fromSurface
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include "dobby.h"
#include "imgui.h"
#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_opengl3.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ModMenu", __VA_ARGS__)

static ANativeWindow* g_AndroidWindow = nullptr;
static bool g_ImGuiReady = false;
static int g_GlWidth = 0, g_GlHeight = 0;

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surface);
eglSwapBuffers_t original_eglSwapBuffers = nullptr;

void RenderImGui() {
    if (!g_ImGuiReady) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Mod Menu");
    ImGui::Text("Injected via eglSwapBuffers Hook");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    if (ImGui::Button("Click me")) {
        LOGI("Button clicked!");
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    static bool initialized = false;
    if (!initialized && g_AndroidWindow != nullptr) {
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_GlWidth);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_GlHeight);
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplAndroid_Init(g_AndroidWindow);
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::StyleColorsDark();
        initialized = true;
        g_ImGuiReady = true;
        LOGI("ImGui initialized! %dx%d", g_GlWidth, g_GlHeight);
    }
    if (g_ImGuiReady) RenderImGui();
    return original_eglSwapBuffers(dpy, surface);
}

void InstallHook() {
    void* libegl = dlopen("libEGL.so", RTLD_LAZY);
    if (!libegl) { LOGI("Failed to load libEGL.so"); return; }
    void* func = dlsym(libegl, "eglSwapBuffers");
    if (func) {
        DobbyHook(func, (void*)hooked_eglSwapBuffers, (void**)&original_eglSwapBuffers);
        LOGI("Hook installed");
    }
    dlclose(libegl);
}

void SetupHooksThread() {
    while (true) {
        if (dlopen("libEGL.so", RTLD_LAZY | RTLD_NOLOAD)) {
            InstallHook();
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

__attribute__((constructor))
void Init() {
    std::thread(SetupHooksThread).detach();
}

extern "C" JNIEXPORT void JNICALL
Java_com_yourpackage_ModMenu_setNativeWindow(JNIEnv* env, jobject thiz, jobject surface) {
    if (surface != nullptr) {
        g_AndroidWindow = ANativeWindow_fromSurface(env, surface);
    }
}
