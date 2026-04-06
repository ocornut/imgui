#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool g_Initialized = false;

// --- 动态自适应结构 (不写死偏移) ---
struct {
    float x, y;          
    bool down;
    
    // 渲染区域的大小 (ImGui 绘制区域)
    float renderW = 0.0f; 
    float renderH = 0.0f;

    // 自动探测的参数
    float autoScaleX = 1.0f;
    float autoScaleY = 1.0f;
    float autoOffsetX = 0.0f;
    float autoOffsetY = 0.0f;
} g_Touch = {0, 0, false};

// 声明我们要用到的底层函数指针 (从 libandroid.so 或动态获取)
typedef float (*p_AMotionEvent_getRawX)(const AInputEvent* motion_event, size_t pointer_index);
typedef float (*p_AMotionEvent_getRawY)(const AInputEvent* motion_event, size_t pointer_index);
static p_AMotionEvent_getRawX g_getRawX = nullptr;
static p_AMotionEvent_getRawY g_getRawY = nullptr;

/**
 * 终极自适应算法：
 * getRawX() 返回手指在物理屏幕上的绝对位置 (0-2400)。
 * getX() 返回手指在当前 Activity 窗口内的相对位置。
 * 两者的差值就是侧边栏/状态栏导致的偏移量。
 */
void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 1. 获取窗口内坐标 (如果是分屏，这里通常是相对于游戏窗口左上角的)
        float winX = AMotionEvent_getX(event, 0);
        float winY = AMotionEvent_getY(event, 0);
        
        // 2. 尝试获取物理绝对坐标进行比对
        if (g_getRawX && g_getRawY) {
            float rawX = g_getRawX(event, 0);
            float rawY = g_getRawY(event, 0);
            
            // 记录偏移：物理位置 - 窗口位置
            g_Touch.autoOffsetX = rawX - winX;
            g_Touch.autoOffsetY = rawY - winY;
        }

        /**
         * 修正映射逻辑：
         * 王者荣耀在分屏下，渲染分辨率(1426)和窗口大小并不一定完全等同。
         * 我们直接信任 winX/winY，但需要处理 DPI 缩放。
         * 如果发现点击偏离，是因为 ImGui 的 io.DisplaySize 与系统给出的 winX 坐标系单位不一致。
         */
        g_Touch.x = winX; 
        g_Touch.y = winY;

        if (action == AMOTION_EVENT_ACTION_DOWN) {
            g_Touch.down = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down = false;
        }
    }
}

// Hook InputConsumer::consume
typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a, b, c, d, e, f);
    if (res == 0 && f && *f) {
        handle_android_event((AInputEvent*)*f);
    }
    return res;
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    g_Touch.renderW = (float)w;
    g_Touch.renderH = (float)h;

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        // 根据高度动态适配 UI 大小，避免分屏下菜单太大或太小
        float fontScale = (float)h / 1000.0f;
        if (fontScale < 1.0f) fontScale = 1.0f;
        ImGui::GetStyle().ScaleAllSizes(3.5f * fontScale);
        ImGui::GetIO().FontGlobalScale = 3.5f * fontScale;
        
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 指点探测 (绿色代表自动模式)
    if (g_Touch.down) {
        ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 15.0f, IM_COL32(0, 255, 0, 255));
    }

    ImGui::SetNextWindowPos(ImVec2(w * 0.1f, h * 0.1f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.7f, h * 0.5f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("AndKitty Dynamic Adaptive", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Detected Offset: X=%.1f, Y=%.1f", g_Touch.autoOffsetX, g_Touch.autoOffsetY);
        ImGui::Text("Render Area: %.0f x %.0f", g_Touch.renderW, g_Touch.renderH);
        ImGui::Separator();
        
        if (ImGui::Button("Reset Menu Position", ImVec2(-1, 80))) {
            ImGui::SetWindowPos(ImVec2(100, 100));
        }
        
        static bool feature_esp = true;
        ImGui::Checkbox("ESP Player", &feature_esp);
        
        ImGui::End();
    }

    ImGui::Render();
    glDisable(GL_DEPTH_TEST);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    sleep(5); 

    // 获取关键的 Raw 坐标函数，用于计算侧边栏偏移
    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        g_getRawX = (p_AMotionEvent_getRawX)dlsym(libandroid, "AMotionEvent_getRawX");
        g_getRawY = (p_AMotionEvent_getRawY)dlsym(libandroid, "AMotionEvent_getRawY");
    }

    DobbyHook((void*)dlsym(RTLD_DEFAULT, "eglSwapBuffers"), (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    
    void* consume_addr = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (consume_addr) DobbyHook(consume_addr, (void*)hook_consume, (void**)&old_consume);

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
