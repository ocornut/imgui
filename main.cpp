#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

// 状态控制
static bool g_Initialized = false;
static bool g_ShowMenu = true;

// 原始函数指针
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// 菜单渲染逻辑
void DrawMenu() {
    if (!g_ShowMenu) return;

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("AndKitty Mod Menu", &g_ShowMenu)) {
        ImGui::Text("注入成功！");
        ImGui::Separator();
        static bool cheat1 = false;
        ImGui::Checkbox("显示射线", &cheat1);
        if (ImGui::Button("隐藏菜单")) {
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
        io.DisplaySize = ImVec2(2400, 1080); // 这里可以动态获取
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
        LOGI("ImGui 初始化完毕！");
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    DrawMenu();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surface);
}

// 插件入口线程
void* init_thread(void*) {
    LOGI("插件线程已启动，等待 25 秒避开游戏自检...");
    sleep(25); // 王者荣耀建议等待时间拉长，直到进入登录界面

    // 获取 eglSwapBuffers 地址
    void* sym_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (!sym_addr) {
        LOGI("无法通过 RTLD_DEFAULT 找到 eglSwapBuffers，尝试直接从 libEGL.so 获取");
        void* h_egl = dlopen("libEGL.so", RTLD_NOW);
        sym_addr = dlsym(h_egl, "eglSwapBuffers");
    }

    if (sym_addr) {
        LOGI("目标 eglSwapBuffers 地址: %p，正在执行 Hook...", sym_addr);
        DobbyHook(sym_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Hook 完成！请检查屏幕是否有菜单。");
    } else {
        LOGI("错误：找不到 eglSwapBuffers 符号！");
    }

    return nullptr;
}

// SO 加载自动执行
__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
