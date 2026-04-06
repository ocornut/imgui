#include <pthread.h>
#include <unistd.h>
#include <mutex>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"

#define TAG "PerfectMenu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// ========================= 全局数据结构 =========================
struct TouchState {
    float x = 0.0f, y = 0.0f;          // 经过校准后的屏幕坐标（用于 ImGui）
    bool down = false;                  // 是否有手指按下
    float renderW = 0.0f, renderH = 0.0f; // 实际渲染区域尺寸（eglQuerySurface）
    
    // 自动校准参数
    float windowOffsetX = 0.0f;         // 窗口左上角相对于屏幕原点的 X 偏移
    float windowOffsetY = 0.0f;         // 窗口左上角相对于屏幕原点的 Y 偏移
    bool calibrated = false;            // 是否已完成校准
    
    // 手动微调（用于极少数设备自动校准仍有误差的情况）
    float manualOffsetX = 0.0f;
    float manualOffsetY = 0.0f;
};

static TouchState g_Touch;
static std::mutex g_TouchMutex;         // 保护 g_Touch 的互斥锁
static bool g_ImGuiInitialized = false;
static int g_FingerCount = 0;           // 当前按下的手指数量

// ========================= 函数指针定义 =========================
// eglSwapBuffers
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// InputConsumer::consume
typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches,
                                       int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

// AMotionEvent 扩展函数（用于获取原始屏幕坐标）
typedef float (*p_AMotionEvent_getRawX)(const AInputEvent* motion_event, size_t pointer_index);
typedef float (*p_AMotionEvent_getRawY)(const AInputEvent* motion_event, size_t pointer_index);
static p_AMotionEvent_getRawX g_getRawX = nullptr;
static p_AMotionEvent_getRawY g_getRawY = nullptr;

// ========================= 辅助函数 =========================
/**
 * 从 AInputEvent 中提取原始屏幕坐标，并自动校准窗口偏移
 * 使用互斥锁保护触摸数据
 */
void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return;
    
    int32_t action = AMotionEvent_getAction(event);
    int32_t pointer_idx = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    int32_t action_mask = action & AMOTION_EVENT_ACTION_MASK;
    
    // 获取窗口相对坐标（系统已扣除状态栏/导航栏，但可能未考虑窗口偏移）
    float winX = AMotionEvent_getX(event, pointer_idx);
    float winY = AMotionEvent_getY(event, pointer_idx);
    
    float finalX = winX, finalY = winY;
    
    // 如果有 Raw 坐标函数，尝试自动校准
    if (g_getRawX && g_getRawY) {
        float rawX = g_getRawX(event, pointer_idx);
        float rawY = g_getRawY(event, pointer_idx);
        
        std::lock_guard<std::mutex> lock(g_TouchMutex);
        
        // 首次按下时自动校准窗口偏移（取差值）
        if (!g_Touch.calibrated && action_mask == AMOTION_EVENT_ACTION_DOWN) {
            g_Touch.windowOffsetX = rawX - winX;
            g_Touch.windowOffsetY = rawY - winY;
            g_Touch.calibrated = true;
            LOGI("Auto-calibrated: offsetX=%.1f, offsetY=%.1f", g_Touch.windowOffsetX, g_Touch.windowOffsetY);
        }
        
        // 使用原始坐标减去窗口偏移，得到真正渲染区域内的坐标
        finalX = rawX - g_Touch.windowOffsetX;
        finalY = rawY - g_Touch.windowOffsetY;
        
        // 应用用户手动微调
        finalX += g_Touch.manualOffsetX;
        finalY += g_Touch.manualOffsetY;
    }
    
    // 更新手指计数和按下状态
    {
        std::lock_guard<std::mutex> lock(g_TouchMutex);
        
        if (action_mask == AMOTION_EVENT_ACTION_DOWN || action_mask == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            g_FingerCount++;
            g_Touch.down = true;
        } else if (action_mask == AMOTION_EVENT_ACTION_UP || action_mask == AMOTION_EVENT_ACTION_POINTER_UP ||
                   action_mask == AMOTION_EVENT_ACTION_CANCEL) {
            g_FingerCount--;
            if (g_FingerCount <= 0) {
                g_Touch.down = false;
                g_FingerCount = 0;
            }
        }
        
        // 始终用当前触摸事件提供的最主要手指坐标（简化处理，若需跟踪特定手指可扩展）
        g_Touch.x = finalX;
        g_Touch.y = finalY;
    }
}

// ========================= Hook 函数 =========================
/**
 * Hook InputConsumer::consume
 * 此函数在输入线程中被调用，需要尽快返回，不能做耗时操作
 */
int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a, b, c, d, e, f);
    if (res == 0 && f && *f) {
        handle_android_event((AInputEvent*)*f);
    }
    return res;
}

/**
 * Hook eglSwapBuffers – 渲染主线程，绘制 ImGui 菜单
 */
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // 获取渲染区域尺寸（物理像素）
    EGLint w = 0, h = 0;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    if (w <= 0 || h <= 0) {
        return old_eglSwapBuffers(dpy, surface);
    }
    
    // 更新渲染尺寸（线程安全读取）
    {
        std::lock_guard<std::mutex> lock(g_TouchMutex);
        g_Touch.renderW = (float)w;
        g_Touch.renderH = (float)h;
    }
    
    // 初始化 ImGui（只执行一次，OpenGL 上下文已激活）
    if (!g_ImGuiInitialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        // 根据屏幕高度动态调整 UI 大小
        float baseScale = (float)h / 1080.0f;  // 以 1080p 为基准
        if (baseScale < 1.0f) baseScale = 1.0f;
        ImGui::GetStyle().ScaleAllSizes(2.5f * baseScale);
        ImGui::GetIO().FontGlobalScale = 2.5f * baseScale;
        
        g_ImGuiInitialized = true;
        LOGI("ImGui initialized, render size: %dx%d", w, h);
    }
    
    // 获取当前触摸数据（线程安全拷贝）
    float touchX, touchY, renderW, renderH;
    bool touchDown;
    {
        std::lock_guard<std::mutex> lock(g_TouchMutex);
        touchX = g_Touch.x;
        touchY = g_Touch.y;
        touchDown = g_Touch.down;
        renderW = g_Touch.renderW;
        renderH = g_Touch.renderH;
    }
    
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(renderW, renderH);
    io.MousePos = ImVec2(touchX, touchY);
    io.MouseDown[0] = touchDown;
    
    // 开始新帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    
    // 调试：在手指位置画一个圆点（绿色表示按下，红色表示未按下但显示位置）
    if (touchDown) {
        ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(0, 255, 0, 180));
    } else {
        ImGui::GetForegroundDrawList()->AddCircle(io.MousePos, 20.0f, IM_COL32(255, 0, 0, 180), 0, 2.0f);
    }
    
    // ==================== 绘制主菜单 ====================
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(renderW * 0.7f, renderH * 0.6f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Perfect Menu", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // 状态显示
        ImGui::Text("Render Area: %.0f x %.0f", renderW, renderH);
        ImGui::Text("Touch Pos: (%.1f, %.1f) %s", touchX, touchY, touchDown ? "[DOWN]" : "[UP]");
        
        // 校准信息
        {
            std::lock_guard<std::mutex> lock(g_TouchMutex);
            ImGui::Text("Auto Offset: (%.1f, %.1f)  Calibrated: %s",
                        g_Touch.windowOffsetX, g_Touch.windowOffsetY,
                        g_Touch.calibrated ? "Yes" : "No");
        }
        
        ImGui::Separator();
        
        // 手动微调偏移（解决极少数设备自动校准不完美的情况）
        float manualX = 0, manualY = 0;
        {
            std::lock_guard<std::mutex> lock(g_TouchMutex);
            manualX = g_Touch.manualOffsetX;
            manualY = g_Touch.manualOffsetY;
        }
        if (ImGui::DragFloat("Manual Offset X", &manualX, 0.5f, -200.0f, 200.0f)) {
            std::lock_guard<std::mutex> lock(g_TouchMutex);
            g_Touch.manualOffsetX = manualX;
        }
        if (ImGui::DragFloat("Manual Offset Y", &manualY, 0.5f, -200.0f, 200.0f)) {
            std::lock_guard<std::mutex> lock(g_TouchMutex);
            g_Touch.manualOffsetY = manualY;
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Reset Menu Position", ImVec2(-1, 60))) {
            ImGui::SetWindowPos(ImVec2(100, 100));
        }
        
        if (ImGui::Button("Recalibrate (Touch screen now)", ImVec2(-1, 60))) {
            std::lock_guard<std::mutex> lock(g_TouchMutex);
            g_Touch.calibrated = false;   // 下次触摸时会重新校准
            g_FingerCount = 0;
            g_Touch.down = false;
            LOGI("Recalibration requested, next touch will reset offset.");
        }
        
        ImGui::Separator();
        
        // 你的功能示例
        static bool featureEsp = true;
        ImGui::Checkbox("ESP Players", &featureEsp);
        
        static float aimFov = 30.0f;
        ImGui::SliderFloat("Aim FOV", &aimFov, 0.0f, 180.0f);
        
        ImGui::End();
    }
    
    // 渲染 ImGui
    ImGui::Render();
    GLint last_depth;
    glGetIntegerv(GL_DEPTH_TEST, &last_depth);
    glDisable(GL_DEPTH_TEST);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (last_depth) glEnable(GL_DEPTH_TEST);
    
    return old_eglSwapBuffers(dpy, surface);
}

// ========================= 初始化线程 =========================
void* init_thread(void*) {
    sleep(5);  // 等待目标进程完全启动
    
    // 1. 获取 libandroid.so 中的 Raw 坐标函数
    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        g_getRawX = (p_AMotionEvent_getRawX)dlsym(libandroid, "AMotionEvent_getRawX");
        g_getRawY = (p_AMotionEvent_getRawY)dlsym(libandroid, "AMotionEvent_getRawY");
        if (!g_getRawX || !g_getRawY) {
            LOGE("Failed to get AMotionEvent_getRawX/Y, will use window coordinates only (may be inaccurate)");
        } else {
            LOGI("Raw coordinate functions acquired");
        }
    } else {
        LOGE("libandroid.so not found, raw coordinates unavailable");
    }
    
    // 2. Hook eglSwapBuffers（渲染）
    void* sym_egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (sym_egl) {
        DobbyHook(sym_egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Hooked eglSwapBuffers");
    } else {
        LOGE("eglSwapBuffers not found");
    }
    
    // 3. Hook InputConsumer::consume（输入）
    // 符号名来自 libinput.so，可能需要根据 Android 版本调整
    void* consume_addr = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (consume_addr) {
        DobbyHook(consume_addr, (void*)hook_consume, (void**)&old_consume);
        LOGI("Hooked InputConsumer::consume");
    } else {
        LOGE("InputConsumer::consume not found, touch input may not work");
    }
    
    return nullptr;
}

// ========================= 入口点 =========================
__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
