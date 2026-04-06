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
    float rawX = 0.0f;
    float rawY = 0.0f;
    
    float mappedX = 0.0f;
    float mappedY = 0.0f;
    
    bool down = false;
    
    float renderW = 0.0f; 
    float renderH = 0.0f;

    // 根据你的设备 3392x2400 与 1426x1008 计算出的完美黄金比例
    float scaleX = 0.4204f; // 1426 / 3392
    float scaleY = 0.4200f; // 1008 / 2400
    
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
        
        float currentRawX = AMotionEvent_getX(event, 0);
        float currentRawY = AMotionEvent_getY(event, 0);
        
        g_Touch.rawX = currentRawX;
        g_Touch.rawY = currentRawY;

        // 核心公式修正：先计算比例，再加偏移。
        // 这样一来，偏移量的单位就等于屏幕上的实际像素！
        g_Touch.mappedX = (currentRawX * g_Touch.scaleX) + g_Touch.offsetX;
        g_Touch.mappedY = (currentRawY * g_Touch.scaleY) + g_Touch.offsetY;

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

    // 绘制调试十字和红点
    if (g_Touch.down) {
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        draw->AddLine(ImVec2(0, g_Touch.mappedY), ImVec2(g_Touch.renderW, g_Touch.mappedY), IM_COL32(0, 255, 0, 200), 3.0f);
        draw->AddLine(ImVec2(g_Touch.mappedX, 0), ImVec2(g_Touch.mappedX, g_Touch.renderH), IM_COL32(0, 255, 0, 200), 3.0f);
        draw->AddCircleFilled(ImVec2(g_Touch.mappedX, g_Touch.mappedY), 25.0f, IM_COL32(255, 0, 0, 255));
    }

    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.9f, h * 0.8f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("MT Visual Calibration", nullptr, ImGuiWindowFlags_NoCollapse)) {
        
        ImGui::TextColored(ImVec4(1,1,0,1), "【法则】随手指拖动而距离变化=调Scale | 距离固定偏=调Offset");
        ImGui::Text("系统 RAW X: %.1f | Y: %.1f", g_Touch.rawX, g_Touch.rawY);
        ImGui::Separator();
        
        ImVec2 btnSize(140, 80); 
        
        // 比例调节区（微调）
        ImGui::TextColored(ImVec4(0,1,1,1), "【第一步】调比例 (让红点和手指移动速度一样)");
        ImGui::Text("Scale X: %.4f", g_Touch.scaleX);
        if (ImGui::Button("-0.01 SX", btnSize)) g_Touch.scaleX -= 0.01f; ImGui::SameLine();
        if (ImGui::Button("+0.01 SX", btnSize)) g_Touch.scaleX += 0.01f; ImGui::SameLine();
        if (ImGui::Button("-0.001 SX", btnSize)) g_Touch.scaleX -= 0.001f; ImGui::SameLine();
        if (ImGui::Button("+0.001 SX", btnSize)) g_Touch.scaleX += 0.001f;

        ImGui::Text("Scale Y: %.4f", g_Touch.scaleY);
        if (ImGui::Button("-0.01 SY", btnSize)) g_Touch.scaleY -= 0.01f; ImGui::SameLine();
        if (ImGui::Button("+0.01 SY", btnSize)) g_Touch.scaleY += 0.01f; ImGui::SameLine();
        if (ImGui::Button("-0.001 SY", btnSize)) g_Touch.scaleY -= 0.001f; ImGui::SameLine();
        if (ImGui::Button("+0.001 SY", btnSize)) g_Touch.scaleY += 0.001f;

        ImGui::Separator();
        
        // 偏移调节区（单位为像素）
        ImGui::TextColored(ImVec4(0,1,1,1), "【第二步】调偏移 (让红点对准手指中心)");
        ImGui::Text("Offset X: %.1f", g_Touch.offsetX);
        if (ImGui::Button("-50 X", btnSize)) g_Touch.offsetX -= 50.0f; ImGui::SameLine();
        if (ImGui::Button("-10 X", btnSize)) g_Touch.offsetX -= 10.0f; ImGui::SameLine();
        if (ImGui::Button("+10 X", btnSize)) g_Touch.offsetX += 10.0f; ImGui::SameLine();
        if (ImGui::Button("+50 X", btnSize)) g_Touch.offsetX += 50.0f;
        
        ImGui::Text("Offset Y: %.1f", g_Touch.offsetY);
        if (ImGui::Button("-50 Y", btnSize)) g_Touch.offsetY -= 50.0f; ImGui::SameLine();
        if (ImGui::Button("-10 Y", btnSize)) g_Touch.offsetY -= 10.0f; ImGui::SameLine();
        if (ImGui::Button("+10 Y", btnSize)) g_Touch.offsetY += 10.0f; ImGui::SameLine();
        if (ImGui::Button("+50 Y", btnSize)) g_Touch.offsetY += 50.0f;
        
        ImGui::Separator();
        if (ImGui::Button("重置默认黄金比例", ImVec2(-1, 100))) {
            g_Touch.offsetX = 0; g_Touch.offsetY = 0;
            g_Touch.scaleX = 0.4204f; g_Touch.scaleY = 0.4200f;
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
