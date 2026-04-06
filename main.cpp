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

// --- 模拟 GitHub 流行项目的自动对齐结构 ---
struct {
    float x, y;          
    bool down;
    // 自动获取的屏幕/渲染比例
    float screenW = 2400.0f; // 物理宽 (从 wm size 获取)
    float screenH = 3392.0f; // 物理高 (从 wm size 获取)
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// 模仿 GitHub 开源库：百分比坐标转换算法
// 这种方法不依赖死固定的比例，兼容性最强
void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 1. 获取物理触碰点 (0 ~ 2400)
        float rawX = AMotionEvent_getX(event, 0);
        float rawY = AMotionEvent_getY(event, 0);
        
        // 2. 获取当前 ImGui 的显示区域 (渲染大小)
        ImGuiIO& io = ImGui::GetIO();
        float renderW = io.DisplaySize.x;
        float renderH = io.DisplaySize.y;

        // 3. 百分比映射逻辑 (GitHub 大神常用公式)
        // 手指在屏幕的位置百分比 * 游戏渲染的实际像素
        g_Touch.x = (rawX / g_Touch.screenW) * renderW;
        g_Touch.y = (rawY / g_Touch.screenH) * renderH;

        if (action == AMOTION_EVENT_ACTION_DOWN) {
            g_Touch.down = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down = false;
        }
    }
}

// Hook 输入消费流
typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a, b, c, d, e, f);
    if (res == 0 && f && *f) {
        handle_android_event((AInputEvent*)*f);
    }
    return res;
}

// 渲染主钩子
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        // 自动适配 UI 大小 (GitHub 风格：根据宽度自动缩放字体)
        float scale = (float)w / 1440.0f; 
        ImGui::GetStyle().ScaleAllSizes(3.0f * scale);
        ImGui::GetIO().FontGlobalScale = 3.0f * scale;
        
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    
    // 传递百分比对齐后的坐标
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 绘制点击反馈 (GitHub 调试必备)
    if (g_Touch.down) {
        ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 15.0f, IM_COL32(0, 255, 0, 255));
    }

    ImGui::SetNextWindowPos(ImVec2(w * 0.1f, h * 0.1f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.6f, h * 0.4f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("AndKitty GitHub Mode", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Screen: %.0f x %.0f", g_Touch.screenW, g_Touch.screenH);
        ImGui::Text("Render: %d x %d", w, h);
        ImGui::Separator();
        
        static bool cheat_esp = true;
        ImGui::Checkbox("ESP Overlay", &cheat_esp);
        
        if (ImGui::Button("Reset Menu", ImVec2(-1, 80))) {
            ImGui::SetWindowPos(ImVec2(100, 100));
        }
        ImGui::End();
    }

    ImGui::Render();
    
    // 强制顶层渲染逻辑
    GLint depth_test;
    glGetIntegerv(GL_DEPTH_TEST, &depth_test);
    glDisable(GL_DEPTH_TEST);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (depth_test) glEnable(GL_DEPTH_TEST);

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    sleep(5); 
    
    // Hook 渲染
    DobbyHook((void*)dlsym(RTLD_DEFAULT, "eglSwapBuffers"), (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    
    // Hook 输入 (采用 libinput.so 符号)
    const char* sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* addr = DobbySymbolResolver("libinput.so", sym);
    if (addr) DobbyHook(addr, (void*)hook_consume, (void**)&old_consume);

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
