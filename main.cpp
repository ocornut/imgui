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

#define TAG "Gemma4_Drive"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool g_Initialized = false;

// --- Gemma 4.0 核心自适应结构 ---
struct {
    float x, y;          
    bool down;
    
    // 渲染表面的实际物理像素尺寸 (由 eglQuerySurface 实时获取)
    float renderW = 0.0f; 
    float renderH = 0.0f;

    // 自动对齐监控：物理全屏 vs 窗口内
    float autoOffsetX = 0.0f;
    float autoOffsetY = 0.0f;
} g_Touch = {0, 0, false};

// 修复：必须在此声明原始函数指针，防止编译报错
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

// 动态获取安卓底层函数，用于跨设备对齐
typedef float (*p_AMotionEvent_getRawX)(const AInputEvent* motion_event, size_t pointer_index);
typedef float (*p_AMotionEvent_getRawY)(const AInputEvent* motion_event, size_t pointer_index);
static p_AMotionEvent_getRawX g_getRawX = nullptr;
static p_AMotionEvent_getRawY g_getRawY = nullptr;

/**
 * Gemma 4.0 自动对齐算法：
 * 1. 放弃所有写死的 2400 或 3392 等数值。
 * 2. 信任 InputConsumer 分发的 winX/winY (窗口内坐标)。
 * 3. 强制 ImGui 的 DisplaySize 与游戏的渲染 Surface 对齐。
 */
void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 获取窗口内相对坐标 (分屏模式下系统会自动处理大部分偏移)
        float winX = AMotionEvent_getX(event, 0);
        float winY = AMotionEvent_getY(event, 0);
        
        // 计算与物理全屏的偏移量 (用于显示调试信息)
        if (g_getRawX && g_getRawY) {
            g_Touch.autoOffsetX = g_getRawX(event, 0) - winX;
            g_Touch.autoOffsetY = g_getRawY(event, 0) - winY;
        }

        // 将坐标同步给 ImGui 变量
        g_Touch.x = winX;
        g_Touch.y = winY;

        if (action == AMOTION_EVENT_ACTION_DOWN) {
            g_Touch.down = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down = false;
        }
    }
}

// Hook 输入层
int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a, b, c, d, e, f);
    if (res == 0 && f && *f) {
        handle_android_event((AInputEvent*)*f);
    }
    return res;
}

// Hook 渲染层
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    
    // 渲染尺寸有效性保护
    if (w <= 0 || h <= 0) return old_eglSwapBuffers(dpy, surface);

    g_Touch.renderW = (float)w;
    g_Touch.renderH = (float)h;

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        // 根据高度动态适配 UI 大小，解决不同设备 DPI 差异
        float uiScale = (float)h / 1080.0f;
        if (uiScale < 1.0f) uiScale = 1.0f;
        
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(3.0f * uiScale);
        ImGui::GetIO().FontGlobalScale = 3.0f * uiScale;
        style.WindowRounding = 10.0f;
        
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    // 强制同步渲染区域，这是全对齐的核心逻辑
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 在指尖绘制绿色探测圈 (如果绿圈准了，菜单就一定准)
    if (g_Touch.down) {
        ImGui::GetForegroundDrawList()->AddCircle(io.MousePos, 30.0f, IM_COL32(0, 255, 0, 255), 12, 4.0f);
    }

    ImGui::SetNextWindowPos(ImVec2(w * 0.1f, h * 0.1f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.8f, h * 0.6f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("AndKitty Gemma 4.0 Alignment", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Current Surface: %.0f x %.0f", g_Touch.renderW, g_Touch.renderH);
        ImGui::Text("Window Offset: X:%.1f Y:%.1f", g_Touch.autoOffsetX, g_Touch.autoOffsetY);
        ImGui::Separator();
        
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Gemma 4.0 Driver Active");
        ImGui::Text("This mode auto-detects split-screen & sidebar.");
        
        if (ImGui::Button("Reset Menu Position", ImVec2(-1, 100))) {
            ImGui::SetWindowPos(ImVec2(50, 50));
        }

        static bool feature_test = true;
        ImGui::Checkbox("Enable Visual Test", &feature_test);
        
        ImGui::End();
    }

    ImGui::Render();
    
    // 渲染前关闭深度测试，防止 UI 被游戏物体遮挡
    GLint last_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // 恢复渲染程序，防止干扰游戏
    glUseProgram(last_program);

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    // 延迟注入，等待游戏 Surface 初始化完毕
    sleep(6); 

    // 加载底层坐标获取函数
    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        g_getRawX = (p_AMotionEvent_getRawX)dlsym(libandroid, "AMotionEvent_getRawX");
        g_getRawY = (p_AMotionEvent_getRawY)dlsym(libandroid, "AMotionEvent_getRawY");
    }

    // Hook 渲染 (eglSwapBuffers)
    void* sym_egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (sym_egl) {
        DobbyHook(sym_egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    }
    
    // Hook 输入 (InputConsumer)
    void* consume_addr = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (consume_addr) {
        DobbyHook(consume_addr, (void*)hook_consume, (void**)&old_consume);
    }

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
