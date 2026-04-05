#include <jni.h>
#include <thread>
#include <chrono>
#include <dlfcn.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include "dobby.h"
#include "imgui.h"
#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_opengl3.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ModMenu", __VA_ARGS__)

// 全局变量
static ANativeWindow* g_AndroidWindow = nullptr;
static bool g_ImGuiReady = false;
static int g_GlWidth = 0;
static int g_GlHeight = 0;

// 原始 eglSwapBuffers 函数指针类型定义
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surface);
eglSwapBuffers_t original_eglSwapBuffers = nullptr;

// ==================== ImGui 菜单绘制 ====================
void RenderImGui() {
    if (!g_ImGuiReady) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    // 在这里绘制你的菜单内容
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

// ==================== Hook 函数 ====================
EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // 延迟初始化 ImGui（只执行一次）
    static bool initialized = false;
    if (!initialized && g_AndroidWindow != nullptr) {
        // 获取 EGL 上下文信息
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_GlWidth);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_GlHeight);

        // 初始化 ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplAndroid_Init(g_AndroidWindow);
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::StyleColorsDark();

        initialized = true;
        g_ImGuiReady = true;
        LOGI("ImGui initialized successfully! Width: %d, Height: %d", g_GlWidth, g_GlHeight);
    }

    // 执行 ImGui 绘制
    if (g_ImGuiReady) {
        RenderImGui();
    }

    // 调用原函数，保证游戏画面正常显示
    return original_eglSwapBuffers(dpy, surface);
}

// ==================== 安装 Hook ====================
void InstallHook() {
    void* libegl = dlopen("libEGL.so", RTLD_LAZY);
    if (!libegl) {
        LOGI("Failed to load libEGL.so");
        return;
    }
    void* func = dlsym(libegl, "eglSwapBuffers");
    if (func) {
        DobbyHook(func, (void*)hooked_eglSwapBuffers, (void**)&original_eglSwapBuffers);
        LOGI("Dobby hook installed on eglSwapBuffers");
    }
    dlclose(libegl);
}

// ==================== 等待 libEGL 加载并 Hook ====================
void SetupHooksThread() {
    LOGI("SetupHooksThread started, waiting for libEGL.so...");
    while (true) {
        if (dlopen("libEGL.so", RTLD_LAZY | RTLD_NOLOAD)) {
            LOGI("libEGL.so found, installing hook...");
            InstallHook();
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// ==================== so 入口 ====================
__attribute__((constructor))
void Init() {
    LOGI("libmodmenu.so loaded!");
    std::thread(SetupHooksThread).detach();
}

// ==================== JNI 函数，用于从 Java 层传递 ANativeWindow ====================
extern "C" JNIEXPORT void JNICALL
Java_com_yourpackage_ModMenu_setNativeWindow(JNIEnv* env, jobject thiz, jobject surface) {
    if (surface != nullptr) {
        g_AndroidWindow = ANativeWindow_fromSurface(env, surface);
        LOGI("Native window received from Java layer");
    }
}
