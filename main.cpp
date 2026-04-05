#include <jni.h>
#include <dlfcn.h>
#include <pthread.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <vulkan/vulkan.h>

// 引入 Dobby Hook 库
#include "dobby.h"

// 引入 ImGui
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

// Vulkan 偷取到的上下文
struct {
    VkInstance       Instance       = VK_NULL_HANDLE;
    VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
    VkDevice         Device         = VK_NULL_HANDLE;
    VkQueue          Queue          = VK_NULL_HANDLE;
    uint32_t         QueueFamily    = 0;
} g_VulkanCtx;

// --- 原函数指针备份 ---
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
static eglSwapBuffers_t old_eglSwapBuffers = nullptr;

typedef VkResult (*vkQueueSubmit_t)(VkQueue, uint32_t, const VkSubmitInfo*, VkFence);
static vkQueueSubmit_t old_vkQueueSubmit = nullptr;

// --- 统一的菜单绘制逻辑 ---
void DrawJKMenu() {
    if (!ImGui::GetCurrentContext()) return;

    // 这里处理你的功能逻辑，比如读取 com.tencent.jkchess 的内存偏移
    ImGui::Begin("JKMenu v1.0", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Render: %s", g_VulkanMode ? "Vulkan (Adreno)" : "OpenGL ES 3.0");
    ImGui::Separator();
    
    static bool test_check = false;
    ImGui::Checkbox("示例功能开关", &test_check);
    
    if (ImGui::Button("清理日志")) {
        LOGI("用户点击了清理按钮");
    }
    
    ImGui::End();
}

// --- OpenGL Hook 实现 (兼容模式用) ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    if (!g_Initialized) {
        LOGI("正在初始化 OpenGL ImGui...");
        ImGui::CreateContext();
        ImGui_ImplAndroid_Init(nullptr); // 实际项目中需传入 NativeWindow
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

// --- Vulkan Hook 实现 (普通模式用) ---
VkResult hook_vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {
    // Vulkan 模式下的逻辑更复杂，通常建议在兼容模式稳定后再深入
    // 这里仅做环境记录，防止崩溃
    if (!g_Initialized) {
        LOGI("检测到 Vulkan 提交，当前为普通模式运行。");
        // 注意：Vulkan 的 ImGui 需要大量的 DescriptorPool 初始化，建议先引导用户用兼容模式
        g_Initialized = true; 
    }
    return old_vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

// --- 核心探测与 Hook 线程 ---
void* init_thread(void*) {
    LOGI("JKMenu 线程已启动，等待游戏渲染模块加载...");
    sleep(5); // 等待游戏初始化完成

    // 1. 优先检查是否为 Vulkan (普通模式)
    void* vulkan_handle = dlopen("vulkan.adreno.so", RTLD_NOLOAD);
    if (vulkan_handle) {
        g_VulkanMode = true;
        LOGI(">>> 发现 Vulkan 驱动，尝试挂载普通模式 Hook...");
        
        // 使用你之前日志里的高通特有符号
        void* vk_submit = dlsym(vulkan_handle, "_ZN11qglinternal13vkQueueSubmitEP9VkQueue_TjPK12VkSubmitInfoP9VkFence_T");
        if (vk_submit) {
            DobbyHook(vk_submit, (void*)hook_vkQueueSubmit, (void**)&old_vkQueueSubmit);
            LOGI("Vulkan Hook 成功！");
        }
    } 
    
    // 2. 无论是否发现 Vulkan，都 Hook EGL (兼容模式)
    // 因为用户可能随时在游戏设置里切换
    void* egl_addr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl_addr) {
        DobbyHook(egl_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("OpenGL (兼容模式) Hook 成功！");
    }

    return nullptr;
}

// --- JNI 入口 ---
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("JKMenu .so 已成功注入进程: com.tencent.jkchess");
    pthread_t tid;
    pthread_create(&tid, nullptr, init_thread, nullptr);
    return JNI_VERSION_1_6;
}
