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

#define TAG "MT_Drive"

static bool g_Initialized = false;

// --- MT 专用可视化校准结构 ---
struct {
    // 系统底层的真实数据
    float rawX = 0.0f;
    float rawY = 0.0f;
    
    // 映射到 ImGui 的数据
    float mappedX = 0.0f;
    float mappedY = 0.0f;
    
    bool down = false;
    
    float renderW = 0.0f; 
    float renderH = 0.0f;

    // 校准参数 (默认 1:1)
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
} g_Touch;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 1. 获取系统传来的绝对生肉坐标
        float currentRawX = AMotionEvent_getX(event, 0);
        float currentRawY = AMotionEvent_getY(event, 0);
        
        g_Touch.rawX = currentRawX;
        g_Touch.rawY = currentRawY;

        // 2. 映射公式：(原始 + 偏移) * 缩放
        g_Touch.mappedX = (currentRawX + g_Touch.offsetX) * g_Touch.scaleX;
        g_Touch.mappedY = (currentRawY + g_Touch.offsetY) * g_Touch.scaleY;

        if (action == AMOTION_EVENT_ACTION_DOWN) {
            g_Touch.down = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down = false;
        }
    }
}

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
    
    if (w <= 0 || h <= 0) return old_eglSwapBuffers(dpy, surface);

    g_Touch.renderW = (float)w;
    g_Touch.renderH = (float)h;

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        // 放大 UI 以防看不清或点不到
        float uiScale = (float)h / 1000.0f;
        if (uiScale < 1.0f) uiScale = 1.0f;
        ImGui::GetStyle().ScaleAllSizes(3.5f * uiScale);
        ImGui::GetIO().FontGlobalScale = 3.5f * uiScale;
        
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);
    io.MousePos = ImVec2(g_Touch.mappedX, g_Touch.mappedY);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 绘制准星（这代表游戏接收到的坐标）
    if (g_Touch.down) {
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        draw->AddLine(ImVec2(0, g_Touch.mappedY), ImVec2(g_Touch.renderW, g_Touch.mappedY), IM_COL32(0, 255, 0, 255), 3.0f);
        draw->AddLine(ImVec2(g_Touch.mappedX, 0), ImVec2(g_Touch.mappedX, g_Touch.renderH), IM_COL32(0, 255, 0, 255), 3.0f);
        draw->AddCircleFilled(ImVec2(g_Touch.mappedX, g_Touch.mappedY), 25.0f, IM_COL32(255, 0, 0, 255));
    }

    // --- 极简调试看板 ---
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.9f, h * 0.8f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("MT Visual Calibration", nullptr, ImGuiWindowFlags_NoCollapse)) {
        
        // 第一步展示区：系统原生数据
        ImGui::TextColored(ImVec4(1,1,0,1), "【1】看这里获取坐标 (Raw)");
        ImGui::Text("系统原坐标 RAW X: %.1f", g_Touch.rawX);
        ImGui::Text("系统原坐标 RAW Y: %.1f", g_Touch.rawY);
        ImGui::Separator();
        
        // 大按钮调节区 (防止滑块点不准)
        ImGui::TextColored(ImVec4(0,1,1,1), "【2】如果不准，用大按键调节");
        ImGui::Text("当前 Offset X: %.1f", g_Touch.offsetX);
        
        ImVec2 btnSize(140, 90); // 巨大按钮
        if (ImGui::Button("-100 X", btnSize)) g_Touch.offsetX -= 100.0f; ImGui::SameLine();
        if (ImGui::Button("-10 X", btnSize)) g_Touch.offsetX -= 10.0f; ImGui::SameLine();
        if (ImGui::Button("+10 X", btnSize)) g_Touch.offsetX += 10.0f; ImGui::SameLine();
        if (ImGui::Button("+100 X", btnSize)) g_Touch.offsetX += 100.0f;
        
        ImGui::Text("当前 Offset Y: %.1f", g_Touch.offsetY);
        if (ImGui::Button("-100 Y", btnSize)) g_Touch.offsetY -= 100.0f; ImGui::SameLine();
        if (ImGui::Button("-10 Y", btnSize)) g_Touch.offsetY -= 10.0f; ImGui::SameLine();
        if (ImGui::Button("+10 Y", btnSize)) g_Touch.offsetY += 10.0f; ImGui::SameLine();
        if (ImGui::Button("+100 Y", btnSize)) g_Touch.offsetY += 100.0f;
        
        ImGui::Separator();
        ImGui::Text("当前 Scale X: %.3f | Scale Y: %.3f", g_Touch.scaleX, g_Touch.scaleY);
        if (ImGui::Button("-0.1 SX", btnSize)) g_Touch.scaleX -= 0.1f; ImGui::SameLine();
        if (ImGui::Button("+0.1 SX", btnSize)) g_Touch.scaleX += 0.1f; ImGui::SameLine();
        if (ImGui::Button("-0.1 SY", btnSize)) g_Touch.scaleX -= 0.1f; ImGui::SameLine();
        if (ImGui::Button("+0.1 SY", btnSize)) g_Touch.scaleX += 0.1f;

        ImGui::Separator();
        if (ImGui::Button("重置所有参数 (RESET)", ImVec2(-1, 100))) {
            g_Touch.offsetX = 0; g_Touch.offsetY = 0;
            g_Touch.scaleX = 1.0f; g_Touch.scaleY = 1.0f;
        }

        ImGui::End();
    }

    ImGui::Render();
    
    GLint last_depth;
    glGetIntegerv(GL_DEPTH_TEST, &last_depth);
    glDisable(GL_DEPTH_TEST);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (last_depth) glEnable(GL_DEPTH_TEST);

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    sleep(6); 
    void* sym_egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (sym_egl) DobbyHook(sym_egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    
    void* consume_addr = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (consume_addr) DobbyHook(consume_addr, (void*)hook_consume, (void**)&old_consume);
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
