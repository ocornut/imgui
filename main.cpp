#include <jni.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>     // 修复 sleep 报错的关键头文件
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <vulkan/vulkan.h>

// 引入 Dobby Hook 库
#include "dobby.h"

// 引入 ImGui 核心与后端
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_vulkan.h"

#define TAG "JKMenu_Core"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// --- 全局变量与状态 ---
static bool g_Initialized = false;
static bool g_VulkanMode  = false;

// --- 原函数指针备份 ---
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
static eglSwapBuffers_t old_eglSwapBuffers = nullptr;

typedef VkResult (*vkQueueSubmit_t)(VkQueue, uint32_t, const VkSubmitInfo*, VkFence);
static vkQueueSubmit_t old_vkQueueSubmit = nullptr;

// --- 统一的菜单绘制逻辑 ---
void DrawJKMenu() {
    if (!ImGui::GetCurrentContext()) return;

    // 这里是你的菜单 UI 逻辑
    ImGui::Begin("JKMenu v1.0", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Render Mode: %s", g_VulkanMode ? "Vulkan (Adreno)" : "OpenGL ES 3.0");
    ImGui::Separator();
    
    static bool show_esp = false;
    ImGui::Checkbox("显示射线 (ESP)", &show_esp);
    
    if (ImGui::Button("强制卸载菜单")) {
        LOGI("用户请求卸载...");
    }
    
    ImGui::End();
}

// --- OpenGL Hook 实现 ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    if (!g_Initialized) {
        LOGI(">>> 正在初始化 OpenGL 渲染后端...");
        ImGui::CreateContext();
        // 初始化 Android 平台后端
        ImGui_ImplAndroid_Init(nullptr); 
        // 初始化 OpenGL3 后端
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    DrawJKMenu();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surf);
}

// --- Vulkan Hook 实现 (探测占位) ---
VkResult hook_vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {
    // 如果检测到是 Vulkan 模式，建议先打印日志，确认 Hook 是否生效
    if (!g_Initialized) {
        LOGI(">>> 发现游戏正在使用 Vulkan 渲染，请切换至【兼容模式】使用菜单。");
        g_Initialized = true; 
    }
    return old_vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

// --- 核心探测与 Hook 线程 ---
void* init_thread(void*) {
    LOGI("JKMenu 核心线程启动，等待系统环境稳定...");
    
    // 给游戏一点启动时间，防止在加载库时发生冲突
    sleep(5); 

    // 1. 探测 Vulkan 驱动 (针对 Adreno GPU)
    void* vulkan_handle = dlopen("vulkan.adreno.so", RTLD_NOLOAD);
    if (vulkan_handle) {
        LOGI("检测到 Adreno Vulkan 驱动，尝试定位提交函数...");
        // 使用之前日志抓取到的特定符号
        void* vk_submit = dlsym(vulkan_handle, "_ZN11qglinternal13vkQueueSubmitEP9VkQueue_TjPK12VkSubmitInfoP9VkFence_T");
        if (vk_submit) {
            g_VulkanMode = true;
            DobbyHook(vk_submit, (void*)hook_vkQueueSubmit, (void**)&old_vkQueueSubmit);
            LOGI("Vulkan Hook 成功挂载！");
        }
    } 
    
    // 2. 探测 OpenGL 环境 (libEGL.so)
    // 无论是否是 Vulkan 模式，都挂载 EGL，这样切换模式后能立刻生效
    void* egl_addr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl_addr) {
        DobbyHook(egl_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("OpenGL (兼容模式) Hook 成功挂载！");
    }

    LOGI("探测完成，JKMenu 运行中。");
    return nullptr;
}

// --- JNI 入口 ---
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("libJKMenu.so 已加载。包名: com.tencent.jkchess");
    pthread_t tid;
    pthread_create(&tid, nullptr, init_thread, nullptr);
    return JNI_VERSION_1_6;
}
