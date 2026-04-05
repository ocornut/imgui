#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
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
void DrawImGui(int width, int height) {
    if (!g_ShowMenu) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    // 强制设置一个显眼的窗口位置和大小
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("JKMenu v3.5 - Android 15", &g_ShowMenu)) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Injection Status: SUCCESS");
        ImGui::Text("Screen Res: %d x %d", width, height);
        ImGui::Separator();
        
        static bool esp = false;
        ImGui::Checkbox("Enemy ESP (Test)", &esp);
        
        if (ImGui::Button("Hide Menu")) {
            g_ShowMenu = false;
        }
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// --- Hook 后的 eglSwapBuffers ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // 获取当前屏幕分辨率，用于设置 ImGui 视口
    EGLint width, height;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

    if (!g_Initialized) {
        LOGI("Initializing ImGui inside eglSwapBuffers (Viewport: %dx%d)", width, height);
        
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.DisplaySize = ImVec2((float)width, (float)height);

        // 核心修复：Android 环境下必须手动处理字体，或者使用默认字体
        ImGui::StyleColorsDark();
        
        // 初始化后端
        ImGui_ImplAndroid_Init(nullptr); 
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        g_Initialized = true;
    }

    // 更新屏幕尺寸（处理转屏）
    ImGui::GetIO().DisplaySize = ImVec2((float)width, (float)height);

    // 开始绘制
    DrawImGui(width, height);

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Thread started, entering monitoring loop...");

    // 1. 等待 libEGL.so
    uintptr_t egl_base = 0;
    while (egl_base == 0) {
        egl_base = get_module_base("libEGL.so");
        if (egl_base == 0) sleep(1);
    }
    LOGI("Found libEGL.so at: %p", (void*)egl_base);

    // 2. 查找符号
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
