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

/**
 * 分屏坐标对齐逻辑：
 * 物理分辨率 (wm size): 2400 x 3392
 * 渲染分辨率 (EGL Query): 1426 x 1008
 * 比例计算：
 * X = 1426 / 2400 = 0.594166667
 * Y = 1008 / 3392 = 0.297169811
 */
struct {
    float x, y;          
    bool down;
    // 采用物理分辨率对齐后的精确比例
    float scaleX = 0.594166667f; 
    float scaleY = 0.297169811f; 
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// 触摸事件分发处理
void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 拿取系统原始物理坐标
        float lx = AMotionEvent_getX(event, 0);
        float ly = AMotionEvent_getY(event, 0);
        
        // 像素级映射转换
        g_Touch.x = lx * g_Touch.scaleX;
        g_Touch.y = ly * g_Touch.scaleY;

        if (action == AMOTION_EVENT_ACTION_DOWN) {
            g_Touch.down = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down = false;
        }
    }
}

// Hook InputConsumer::consume (解决分屏触摸的关键)
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
    EGLint width, height;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

    if (width <= 0 || height <= 0) return old_eglSwapBuffers(dpy, surface);

    if (!g_Initialized) {
        LOGI("Initializing ImGui... Render: %d x %d", width, height);
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        // 针对 2400x3392 高分屏调整 UI 大小
        ImGui::GetStyle().ScaleAllSizes(4.0f); 
        io.FontGlobalScale = 4.0f;
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    
    // 将转换后的坐标传递给 ImGui
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 绘制一个同步圆点：如果这个圆点不在你指尖，说明 wm size 有误或系统有额外偏移
    if (g_Touch.down) {
        ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(255, 0, 0, 255));
    }

    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("AndKitty SGame Precision", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Physical: 2400x3392");
        ImGui::Text("Render: %d x %d", width, height);
        ImGui::Text("Input: %.1f, %.1f", g_Touch.x, g_Touch.y);
        ImGui::Separator();
        
        static bool test_check = true;
        ImGui::Checkbox("Precision Mode", &test_check);
        
        if (ImGui::Button("Reset Menu Pos", ImVec2(-1, 80))) {
            ImGui::SetWindowPos(ImVec2(100, 100));
        }
        
        ImGui::End();
    }

    ImGui::Render();
    
    // 渲染保护
    GLint last_depth;
    glGetIntegerv(GL_DEPTH_TEST, &last_depth);
    glDisable(GL_DEPTH_TEST);
    
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    if (last_depth) glEnable(GL_DEPTH_TEST);

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    // 注入后等待 5 秒，确保游戏渲染已稳定
    LOGI("Plugin thread started. Sleeping 5s...");
    sleep(5); 

    // 1. Hook 渲染
    void* sym_egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (sym_egl) {
        DobbyHook(sym_egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    }

    // 2. Hook 输入
    const char* consume_sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume_addr = DobbySymbolResolver("libinput.so", consume_sym);
    if (consume_addr) {
        DobbyHook(consume_addr, (void*)hook_consume, (void**)&old_consume);
        LOGI("InputConsumer Hooked Successfully!");
    }

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
