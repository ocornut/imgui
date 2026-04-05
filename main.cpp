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

    // 绘制一个调试背景，确保能看到渲染层
    ImGui::GetForegroundDrawList()->AddRectFilled(
        ImVec2(50, 50), ImVec2(250, 150), 
        IM_COL32(255, 0, 0, 150) // 半透明红色矩形
    );

    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("JKMenu v3.7 - Render Test", &g_ShowMenu)) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Rendering");
        ImGui::Text("Screen: %d x %d", width, height);
        ImGui::Separator();
        
        static bool cheat_esp = false;
        ImGui::Checkbox("Enable ESP Test", &cheat_esp);
        
        if (ImGui::Button("Close Menu")) {
            g_ShowMenu = false;
        }
    }
    ImGui::End();

    ImGui::Render();
    
    // 渲染前保存 GL 状态（可选，但安全）
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// --- Hook 后的 eglSwapBuffers ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    pthread_mutex_lock(&g_CtxMutex);
    
    EGLint width, height;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

    if (!g_Initialized) {
        LOGI("ImGui Initializing (Viewport: %dx%d)", width, height);
        
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        // 关键：即使没有输入后端，也必须设置显示大小
        io.DisplaySize = ImVec2((float)width, (float)height);
        io.IniFilename = nullptr;

        // 关键：构建默认字体图轴
        unsigned char* pixels;
        int font_w, font_h;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &font_w, &font_h);
        
        if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
            g_Initialized = true;
            LOGI("ImGui Init Success at %dx%d", width, height);
        }
    }

    if (g_Initialized && width > 0 && height > 0) {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
        
        // 执行绘制
        DrawImGui(width, height);
    }

    pthread_mutex_unlock(&g_CtxMutex);
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Thread started, entering monitoring loop...");

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
