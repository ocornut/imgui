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

// --- 高级自适应结构 ---
struct {
    float x, y;          
    bool down;
    // 你的物理分辨率 wm size: 2400 x 3392
    float screenW = 2400.0f; 
    float screenH = 3392.0f;
    
    // 运行时检测到的渲染分辨率
    float renderW = 1426.0f; 
    float renderH = 1008.0f;

    // 偏移量（部分手机分屏会有黑边填充）
    float offsetX = 0.0f;
    float offsetY = 0.0f;
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// 核心映射函数：解决“点不准”的根源
void update_touch_position(float rawX, float rawY) {
    // 逻辑：将物理坐标(0~2400)映射到渲染坐标(0~1426)
    // 如果还是不准，通常是因为系统在分屏上方或左侧留了空隙
    float scaleX = g_Touch.renderW / g_Touch.screenW;
    float scaleY = g_Touch.renderH / g_Touch.screenH;

    // 自动修正坐标：假设分屏是在屏幕顶部或左侧开始的
    // 如果你的分屏是在屏幕中间，这个公式会通过 renderW/H 自动适配
    g_Touch.x = (rawX * scaleX);
    g_Touch.y = (rawY * scaleY);
    
    // 容错处理：防止坐标超出菜单边界
    if(g_Touch.x < 0) g_Touch.x = 0;
    if(g_Touch.y < 0) g_Touch.y = 0;
}

void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        float rx = AMotionEvent_getX(event, 0);
        float ry = AMotionEvent_getY(event, 0);
        
        update_touch_position(rx, ry);

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

// 渲染钩子
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);

    // 动态更新渲染尺寸
    g_Touch.renderW = (float)w;
    g_Touch.renderH = (float)h;

    if (!g_Initialized) {
        LOGI("ImGui Init: %d x %d", w, h);
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        // 缩放适配：基于 1080p 标准缩放 UI
        float base_scale = (float)w / 1080.0f;
        if(base_scale < 1.0f) base_scale = 1.0f;
        
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(3.0f * base_scale);
        ImGui::GetIO().FontGlobalScale = 3.0f * base_scale;
        
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // --- 调试辅助线：帮你定位为什么不准 ---
    if (g_Touch.down) {
        // 画一个大十字架跟随映射后的坐标
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        draw->AddLine(ImVec2(0, g_Touch.y), ImVec2(g_Touch.renderW, g_Touch.y), IM_COL32(255, 255, 0, 200), 2.0f);
        draw->AddLine(ImVec2(g_Touch.x, 0), ImVec2(g_Touch.x, g_Touch.renderH), IM_COL32(255, 255, 0, 200), 2.0f);
    }

    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.8f, h * 0.5f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("AndKitty Auto-Fix", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Physical: 2400 x 3392");
        ImGui::Text("Render: %.0f x %.0f", g_Touch.renderW, g_Touch.renderH);
        ImGui::Separator();
        
        // 显示当前计算出的缩放比，方便对比
        ImGui::Text("Scale: X=%.3f, Y=%.3f", g_Touch.renderW/g_Touch.screenW, g_Touch.renderH/g_Touch.screenH);
        ImGui::Text("Mouse: %.1f, %.1f", g_Touch.x, g_Touch.y);

        if (ImGui::Button("Reset Menu", ImVec2(-1, 100))) {
            ImGui::SetWindowPos(ImVec2(10, 10));
        }

        static bool test = false;
        ImGui::Checkbox("Feature Toggle", &test);
        
        ImGui::End();
    }

    ImGui::Render();
    
    // GL 状态恢复
    GLint last_depth;
    glGetIntegerv(GL_DEPTH_TEST, &last_depth);
    glDisable(GL_DEPTH_TEST);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (last_depth) glEnable(GL_DEPTH_TEST);

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    // 5秒延迟
    sleep(5); 

    // Hook EGL
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // Hook Input (libinput.so)
    const char* sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* addr = DobbySymbolResolver("libinput.so", sym);
    if (addr) DobbyHook(addr, (void*)hook_consume, (void**)&old_consume);

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
