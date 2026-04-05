#include <jni.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h> // 修复：必须包含此头文件以使用 dlopen 和 dlsym

#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "dobby.h"

// 日志配置
#define LOG_TAG "AndKitty"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- 全局状态 ---
bool g_Initialized = false;
bool g_ShowMenu = true;
int g_Width = 0;
int g_Height = 0;

// --- Hook 目标定义 ---
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surf);
eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

// --- 渲染 UI 逻辑 ---
void DrawImGuiMenu() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        // 设置菜单初始大小
        ImGui::SetNextWindowSize(ImVec2(g_Width / 3.5f, g_Height / 2.5f), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty Mod Menu", &g_ShowMenu)) {
            ImGui::Text("分辨率: %d x %d", g_Width, g_Height);
            ImGui::Separator();
            
            static bool aim = false;
            ImGui::Checkbox("自动瞄准 (Aimbot)", &aim);
            
            if (ImGui::Button("隐藏菜单 (Hide)")) {
                g_ShowMenu = false;
            }
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// --- Hook 后的 eglSwapBuffers ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    if (!g_Initialized) {
        // 动态获取真实屏幕宽高
        eglQuerySurface(dpy, surf, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surf, EGL_HEIGHT, &g_Height);

        if (g_Width > 0 && g_Height > 0) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            
            // 将真实分辨率赋给 ImGui
            io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);

            // 初始化后端
            ImGui_ImplAndroid_Init(nullptr);
            ImGui_ImplOpenGL3_Init("#version 300 es");
            
            ImGui::StyleColorsDark();
            
            // 调整 UI 缩放以适配高分屏
            ImGui::GetStyle().ScaleAllSizes(g_Height / 1080.0f);
            
            g_Initialized = true;
            LOGI("ImGui 适配完成: %d x %d", g_Width, g_Height);
        }
    }

    DrawImGuiMenu();
    return orig_eglSwapBuffers(dpy, surf);
}

// --- 渲染 Hook 查找 ---
void setup_render_hook() {
    // 显式使用动态链接库操作
    void* egl_handle = dlopen("libEGL.so", RTLD_LAZY);
    if (egl_handle) {
        void* target = dlsym(egl_handle, "eglSwapBuffers");
        if (target) {
            DobbyHook(target, (dobby_dummy_func_t)hook_eglSwapBuffers, (dobby_dummy_func_t*)&orig_eglSwapBuffers);
            LOGI("成功挂载 eglSwapBuffers");
        }
    }
}

// --- AndKittyInjector 入口 (JNI_OnLoad) ---
extern "C" jint JNIEXPORT JNI_OnLoad(JavaVM* vm, void *key) {
    // 校验 AndKittyInjector 的特有 Key (1337)
    if (key != (void*)1337) {
        return JNI_VERSION_1_6;
    }

    LOGI("AndKittyInjector 授权成功，准备注入...");

    std::thread([]() {
        // 等待游戏渲染环境完全启动
        sleep(5); 
        setup_render_hook();
    }).detach();

    return JNI_VERSION_1_6;
}
