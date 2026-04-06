#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool g_Initialized = false;
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// 渲染钩子函数
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        LOGI("Successfully Entered Hook! Initializing ImGui...");
        
        // 获取真实的屏幕分辨率
        EGLint width, height;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);
        LOGI("Detected Display Size: %d x %d", width, height);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
        
        // 设置基本样式，让菜单大一点
        ImGui::GetStyle().ScaleAllSizes(3.0f); 
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
        LOGI("ImGui Initialized Successfully!");
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 强制显示一个简单的测试窗口
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("AndKitty Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1,1,0,1), "Plugin Status: ACTIVE");
        ImGui::Separator();
        ImGui::Text("Package: com.tencent.jkchess");
        static bool test_check = true;
        ImGui::Checkbox("Test Feature", &test_check);
        if (ImGui::Button("Close Menu")) {
            // 逻辑处理
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Step 1: Thread Started, sleeping 10s...");
    sleep(10); 

    LOGI("Step 2: Searching for eglSwapBuffers...");
    // 使用 dlsym 确保获取的是 libEGL.so 里的导出符号地址
    void* sym_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    
    if (sym_addr) {
        LOGI("Step 3: Found addr at %p, Hooking...", sym_addr);
        DobbyHook(sym_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Step 4: All Done!");
    } else {
        LOGI("Step 3: Failed to find symbol!");
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
