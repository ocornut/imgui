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
#include "backends/imgui_impl_opengl3.h"

#define TAG "JKMenu_SGame"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// --- 全局变量 ---
bool g_Initialized = false;
bool g_ShowMenu = true;
pthread_mutex_t g_CtxMutex = PTHREAD_MUTEX_INITIALIZER;
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
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("JKMenu v3.6 - Stable Build", &g_ShowMenu)) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Render Thread Active");
        ImGui::Text("Viewport: %d x %d", width, height);
        ImGui::Separator();
        
        static bool test_check = false;
        ImGui::Checkbox("Test Feature", &test_check);
        
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
    pthread_mutex_lock(&g_CtxMutex);
    
    EGLint width, height;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

    if (!g_Initialized) {
        LOGI("Attempting ImGui Init (Viewport: %dx%d)", width, height);
        
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
        io.IniFilename = nullptr;

        // 在 Hook 模式下，跳过 ImGui_ImplAndroid_Init
        // 因为我们没有 NativeWindow，直接初始化渲染器后端
        if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
            g_Initialized = true;
            LOGI("ImGui Init Success.");
        } else {
            LOGE("ImGui_ImplOpenGL3_Init Failed!");
        }
    }

    if (g_Initialized) {
        ImGui::GetIO().DisplaySize = ImVec2((float)width, (float)height);
        DrawImGui(width, height);
    }

    pthread_mutex_unlock(&g_CtxMutex);
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Thread started, entering monitoring loop...");

    // 等待 libEGL.so
    uintptr_t egl_base = 0;
    while (egl_base == 0) {
        egl_base = get_module_base("libEGL.so");
        if (egl_base == 0) sleep(1);
    }

    void* sym_eglSwapBuffers = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (sym_eglSwapBuffers) {
        LOGI("Hooking eglSwapBuffers at %p", sym_eglSwapBuffers);
        DobbyHook(sym_eglSwapBuffers, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    }

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
