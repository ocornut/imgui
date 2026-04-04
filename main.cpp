#include <jni.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>

// ImGui 核心与渲染器
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include <GLES3/gl3.h>
#include <EGL/egl.h>

#define LOG_TAG "JKMenu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 原始函数指针
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surface);
eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

// 全局状态
bool g_Initialized = false;
bool g_ShowMenu = true;
int g_Width = 0;
int g_Height = 0;

// 菜单绘制内容
void DrawImGuiMenu() {
    if (!g_ShowMenu) return;

    ImGui::SetNextWindowSize(ImVec2(450, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("JKMenu - OpenGL Mode", &g_ShowMenu);

    ImGui::Text("Device: OnePlus Pad Pro 2");
    ImGui::Text("Package: com.tencent.jkchess");
    ImGui::Text("Screen: %dx%d", g_Width, g_Height);
    ImGui::Separator();

    static bool feature1 = false;
    ImGui::Checkbox("显示射线", &feature1);
    
    static float menu_scale = 1.0f;
    ImGui::SliderFloat("菜单缩放", &menu_scale, 0.5f, 2.0f);

    if (ImGui::Button("隐藏菜单 (需重启注入显示)")) {
        g_ShowMenu = false;
    }

    ImGui::End();
}

// 核心 Hook 函数
EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        LOGI("SUCCESS: OpenGL Render triggered via eglSwapBuffers!");
        
        // 获取实际渲染表面的宽高
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_Height);

        if (g_Width > 0 && g_Height > 0) {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            
            // 针对高分辨率平板优化字体大小
            io.FontGlobalScale = 2.5f; 
            io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);

            if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
                g_Initialized = true;
                LOGI("ImGui Initialized Successfully!");
            }
        }
    }

    if (g_Initialized) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        DrawImGuiMenu();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return orig_eglSwapBuffers(dpy, surface);
}

// 注入后的初始化线程
void* hack_thread(void*) {
    LOGI("SO Loaded! Waiting for Game Engine...");
    
    // 等待游戏加载，金铲铲这类游戏建议等 10-15 秒
    sleep(12);

    // 尝试寻找 Hook 点
    // 1. 标准 EGL 函数
    void* target = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    
    // 2. 如果标准没搜到，尝试搜腾讯私有渲染函数
    if (!target) {
        target = dlsym(RTLD_DEFAULT, "nw_main_swap_buffers");
    }

    if (target) {
        LOGI("Found render target at %p, applying DobbyHook...", target);
        DobbyHook(target, 
                  (dobby_dummy_func_t)hooked_eglSwapBuffers, 
                  (dobby_dummy_func_t*)&orig_eglSwapBuffers);
        
        if (orig_eglSwapBuffers) {
            LOGI("Dobby Hook Applied Successfully!");
        }
    } else {
        LOGE("CRITICAL: No render function found. Is the game still in Vulkan mode?");
    }

    return nullptr;
}

// 构造函数：SO 加载时自动运行
void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, hack_thread, nullptr);
}
