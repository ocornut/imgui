#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include <stdio.h>
#include <algorithm>
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <dirent.h>
#include <iomanip>
#include <sstream>
#include <link.h>
#include <sys/mman.h>
#include <setjmp.h>
#include <signal.h>
#include <cstdarg>
#include <cstring>
#include <errno.h>

#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "IL2CPP_Drive"

// === 增强型日志宏 ===
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static bool g_Initialized = false;
static std::atomic<int> g_EglCallCount{0}; // 记录 eglSwapBuffers 被调用的次数

struct {
    float x = 0.0f; float y = 0.0f;
    std::atomic<bool> down{false};
    float renderW = 0.0f; float renderH = 0.0f;
} g_Touch;

// ---------------------------------------------------------
// 核心 Hook 定义区
// ---------------------------------------------------------
typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

int hook_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int res = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    return res;
}

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // 限制只打印前10帧，防止日志刷屏卡死
    int current_frame = g_EglCallCount.fetch_add(1);
    if (current_frame < 10) {
        LOGI("[STEP 5] 成功拦截 eglSwapBuffers! 第 %d 帧", current_frame);
    }

    EGLint w, h;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    
    if (w <= 0 || h <= 0) return old_eglSwapBuffers(dpy, surface);
    g_Touch.renderW = (float)w;
    g_Touch.renderH = (float)h;

    if (!g_Initialized) {
        LOGI("[STEP 6] 开始初始化 ImGui 上下文...");
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::GetIO().IniFilename = nullptr; // 防越界
        ImGui::GetIO().Fonts->AddFontDefault();
        g_Initialized = true;
        LOGI("[STEP 7] ImGui 初始化完成!");
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 画一个绝对安全的红色大方块在左上角，排除所有排版问题
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);
    if (ImGui::Begin("DEBUG_PANEL", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "如果你能看到这个，说明渲染层彻底通了！");
        ImGui::Text("当前分辨率: %.0f x %.0f", g_Touch.renderW, g_Touch.renderH);
        ImGui::Text("渲染帧数: %d", current_frame);
    }
    ImGui::End();

    ImGui::Render();
    GLint v[4]; glGetIntegerv(GL_VIEWPORT, v);
    GLint d; glGetIntegerv(GL_DEPTH_TEST, &d);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, w, h);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glViewport(v[0], v[1], v[2], v[3]);
    if (d) glEnable(GL_DEPTH_TEST);

    return old_eglSwapBuffers(dpy, surface);
}

// ---------------------------------------------------------
// 线程挂载区 (排错核心)
// ---------------------------------------------------------
void* DelayedHookThread(void*) {
    LOGI("[STEP 2] 进入挂载线程，等待 3 秒避开游戏高并发初始化...");
    sleep(3);
    
    LOGI("[STEP 3] 开始解析 libEGL.so -> eglSwapBuffers");
    void* egl = nullptr;
    int retry = 0;
    while (!egl && retry < 5) { 
        egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers"); 
        if (!egl) { 
            LOGE("未找到 eglSwapBuffers，重试 %d/5...", retry+1);
            sleep(1); retry++; 
        }
    }
    
    if (egl) {
        LOGI("找到 eglSwapBuffers，地址: %p。准备 DobbyHook...", egl);
        int ret = DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        if (ret == 0) {
            LOGI("[STEP 4] DobbyHook(eglSwapBuffers) 成功! 等待游戏调用...");
        } else {
            LOGE("[FATAL] DobbyHook(eglSwapBuffers) 失败! 错误码: %d (可能是 SELinux 拦截)", ret);
        }
    } else {
        LOGE("[FATAL] 彻底找不到 libEGL.so。你的设备或游戏架构可能有变！");
    }

    LOGI("[---] 开始解析 libinput.so...");
    void* ins = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE"); 
    if (ins) {
        int ret2 = DobbyHook(ins, (void*)hook_consume, (void**)&old_consume);
        LOGI("DobbyHook(InputConsumer) 结果: %d", ret2);
    } else {
        LOGE("未找到 libinput.so 中的 consume 函数。");
    }

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    LOGI("==================================================");
    LOGI("[STEP 1] 模块 constructor 被加载! 注入已发生。");
    LOGI("==================================================");
    pthread_t t;
    pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
