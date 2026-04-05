#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "dobby.h"

// ImGui 包含
#include "imgui.h"
#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_opengl3.h"

#define TAG "JKMenu_SGame"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// --- 全局变量 ---
bool g_Initialized = false;
bool g_ShowMenu = true;
EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = nullptr;

// 获取模块基址
uintptr_t get_module_base(const char* name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    char line[1024];
    uintptr_t base = 0;
    while (fp && fgets(line, sizeof(line), fp)) {
        if (strstr(line, name)) {
            base = (uintptr_t)strtoull(line, NULL, 16);
            break;
        }
    }
    if (fp) fclose(fp);
    return base;
}

// --- ImGui 绘制逻辑 ---
void DrawImGui() {
    if (!g_ShowMenu) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("JKMenu v3.0 - Android 15 Stable", &g_ShowMenu)) {
        ImGui::Text("Hello, SGame Player!");
        ImGui::Separator();
        static bool cheat1 = false;
        ImGui::Checkbox("Enable ESP", &cheat1);
        
        if (ImGui::Button("Close Menu")) {
            g_ShowMenu = false;
        }
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// --- Hook 后的 eglSwapBuffers ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        // 初始化 ImGui 上下文
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr; // 不保存配置文件
        
        ImGui_ImplAndroid_Init(nullptr); // 王者荣耀通常不需要窗口句柄
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        ImGui::StyleColorsDark();
        g_Initialized = true;
        LOGI("ImGui Initialized inside eglSwapBuffers.");
    }

    DrawImGui();

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Thread started, entering monitoring loop...");

    // 1. 等待 libEGL.so 加载（渲染菜单必须 Hook 它）
    uintptr_t egl_base = 0;
    while (egl_base == 0) {
        egl_base = get_module_base("libEGL.so");
        if (egl_base == 0) sleep(1);
    }
    LOGI("Found libEGL.so at: %p", (void*)egl_base);

    // 2. 查找 eglSwapBuffers 符号地址
    // 也可以直接用 dlsym(RTLD_NEXT, "eglSwapBuffers")
    void* sym_eglSwapBuffers = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (!sym_eglSwapBuffers) {
        LOGE("Error: Cannot find eglSwapBuffers symbol!");
        return nullptr;
    }

    // 3. 执行 Hook
    LOGI("Hooking eglSwapBuffers at %p", sym_eglSwapBuffers);
    DobbyHook(sym_eglSwapBuffers, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    LOGI("Hook applied. Waiting for frame render...");
    return nullptr;
}

__attribute__((constructor))
void _init() {
    pthread_t tid;
    pthread_create(&tid, nullptr, init_thread, nullptr);
    pthread_detach(tid);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    return JNI_VERSION_1_6;
}
