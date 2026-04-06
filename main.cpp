#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>      // 修复：添加此头文件以支持 dlsym, dlopen, RTLD_DEFAULT
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// 状态控制
static bool g_Initialized = false;
static bool g_ShowMenu = true;

// 原始函数指针
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// 菜单渲染逻辑
void DrawMenu() {
    if (!g_ShowMenu) return;

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("AndKitty Mod Menu", &g_ShowMenu)) {
        ImGui::Text("Status: Injected Successfully");
        ImGui::Separator();
        static bool feature_test = false;
        ImGui::Checkbox("ESP Line", &feature_test);
        
        if (ImGui::Button("Hide Menu")) {
            g_ShowMenu = false;
        }
    }
    ImGui::End();
}

// 钩子函数
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        LOGI("检测到渲染上下文，正在初始化 ImGui...");
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        // 默认分辨率设置，防止初始化失败
        io.DisplaySize = ImVec2(2400.0f, 1080.0f); 
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
        LOGI("ImGui 初始化完毕！");
    }

    if (g_ShowMenu) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        DrawMenu();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return old_eglSwapBuffers(dpy, surface);
}

// 插件入口线程
void* init_thread(void*) {
    LOGI("插件线程已启动，等待 40 秒避开自检...");
    
    // 王者荣耀建议等待 40 秒，确保进入大厅
    sleep(40); 

    // 获取 eglSwapBuffers 地址
    void* sym_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (!sym_addr) {
        LOGI("RTLD_DEFAULT 失败，尝试直接从 libEGL.so 获取");
        void* h_egl = dlopen("libEGL.so", RTLD_NOW);
        if (h_egl) sym_addr = dlsym(h_egl, "eglSwapBuffers");
    }

    if (sym_addr) {
        LOGI("目标 eglSwapBuffers 地址: %p，正在执行 Hook...", sym_addr);
        DobbyHook(sym_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Hook 完成！请检查屏幕。");
    } else {
        LOGE("错误：无法找到 eglSwapBuffers 符号！");
    }

    return nullptr;
}

// SO 加载自动执行
__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
