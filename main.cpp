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
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static bool g_Initialized = false;
static int g_Width = 0;
static int g_Height = 0;

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
static eglSwapBuffers_t old_eglSwapBuffers = nullptr;

// 绘制菜单
void DrawJKMenu() {
    if (!ImGui::GetCurrentContext()) return;
    
    // 设置默认位置和大小（仅第一次）
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

    ImGui::Begin("JKMenu for SGame", nullptr, ImGuiWindowFlags_None);
    ImGui::Text("Build Date: 2026-04-06");
    ImGui::Text("Viewport: %dx%d", g_Width, g_Height);
    ImGui::Separator();
    
    static bool hack_enabled = false;
    if (ImGui::Button("功能开关示例")) {
        hack_enabled = !hack_enabled;
    }
    
    ImGui::Text("状态: %s", hack_enabled ? "已开启" : "已关闭");
    
    if (ImGui::Button("卸载插件")) {
        // 可以在这里写卸载逻辑
    }
    
    ImGui::End();
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    // 获取屏幕分辨率
    eglQuerySurface(dpy, surf, EGL_WIDTH, &g_Width);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &g_Height);

    if (!g_Initialized) {
        if (g_Width > 0 && g_Height > 0) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            
            // 字体缩放自适应手机高分屏
            io.FontGlobalScale = 2.0f; 
            
            // 初始化渲染后端
            // 注意：在 Hook 环境下不需要调用 ImGui_ImplAndroid_Init
            // 因为我们没有 NativeWindow，通常只用 OpenGL3 后端
            ImGui_ImplOpenGL3_Init("#version 300 es");
            
            g_Initialized = true;
            LOGI("ImGui Initialized! Screen: %dx%d", g_Width, g_Height);
        }
    }

    if (g_Initialized) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);
        io.DeltaTime = 1.0f / 60.0f;

        ImGui::NewFrame();
        DrawJKMenu();
        ImGui::Render();
        
        // 渲染 ImGui
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return old_eglSwapBuffers(dpy, surf);
}

void* init_thread(void*) {
    LOGI("Thread started, waiting for game load...");
    // 增加延迟确保 libEGL.so 已经完全加载到内存
    sleep(15); 
    
    // 尝试寻找 eglSwapBuffers 地址
    void* egl_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (!egl_addr) {
        egl_addr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    }

    if (egl_addr) {
        LOGI("Found eglSwapBuffers at %p, hooking...", egl_addr);
        DobbyHook(egl_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        if (old_eglSwapBuffers) {
            LOGI("Hook eglSwapBuffers Success!");
        } else {
            LOGE("DobbyHook failed to return original function.");
        }
    } else {
        LOGE("Failed to find eglSwapBuffers!");
    }
    
    return nullptr;
}

// 当 SO 被 dlopen 加载时自动调用
__attribute__((constructor))
void _init() {
    pthread_t tid;
    pthread_create(&tid, nullptr, init_thread, nullptr);
}

// 也可以保留 JNI_OnLoad 作为双重保险
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    return JNI_VERSION_1_6;
}
