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

#define TAG "MT_Auto_Drive"

static bool g_Initialized = false;

// --- 全自动自适应结构 ---
struct {
    float rawX = 0.0f;
    float rawY = 0.0f;
    float mappedX = 0.0f;
    float mappedY = 0.0f;
    bool down = false;
    
    float renderW = 0.0f; 
    float renderH = 0.0f;

    // 设备固定的物理分辨率 (从你之前的 wm size 获取)
    // 只要设备不换，这两个数字永远不用动
    float physicalW = 3392.0f;
    float physicalH = 2400.0f;

    // 内部计算参数 (自动生成，不再需要手动调)
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
} g_Touch;

// 自动定点校准状态机
static bool g_WaitingForTopLeft = false;

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

        if (action == AMOTION_EVENT_ACTION_DOWN) {
            // --- 一键自动对齐逻辑 ---
            if (g_WaitingForTopLeft) {
                // 原理：当手指点击游戏画面真正的左上角时，Mapped坐标应该是 (0,0)
                // 推出公式：0 = (Raw + Offset) * Scale => Offset = -Raw
                g_Touch.offsetX = -currentRawX;
                g_Touch.offsetY = -currentRawY;
                g_WaitingForTopLeft = false;
                return; // 吃掉这次点击，防止误触菜单
            }
            g_Touch.down = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down = false;
        }

        // 核心映射：动态比例 + 自动偏移
        g_Touch.mappedX = (currentRawX + g_Touch.offsetX) * g_Touch.scaleX;
        g_Touch.mappedY = (currentRawY + g_Touch.offsetY) * g_Touch.scaleY;
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

    // --- 真正的自适应缩放核心 ---
    // 每一帧自动计算比例，如果你在游戏中途拉动分屏线条，这里会瞬间无缝适应！
    g_Touch.scaleX = g_Touch.renderW / g_Touch.physicalW;
    g_Touch.scaleY = g_Touch.renderH / g_Touch.physicalH;

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

    // 绘制自动校准状态提示
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    if (g_WaitingForTopLeft) {
        // 全屏半透明黑底 + 红色提示，引导用户点击
        draw->AddRectFilled(ImVec2(0, 0), ImVec2(w, h), IM_COL32(0, 0, 0, 150));
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.5f, ImVec2(w * 0.1f, h * 0.4f), 
                      IM_COL32(255, 50, 50, 255), "【自动校准中】\n请用手指准确点击游戏画面的【最左上角】一次！", NULL, 0.0f);
    } else if (g_Touch.down) {
        // 正常点按时的准星
        draw->AddLine(ImVec2(0, g_Touch.mappedY), ImVec2(w, g_Touch.mappedY), IM_COL32(0, 255, 0, 150), 2.0f);
        draw->AddLine(ImVec2(g_Touch.mappedX, 0), ImVec2(g_Touch.mappedX, h), IM_COL32(0, 255, 0, 150), 2.0f);
        draw->AddCircleFilled(ImVec2(g_Touch.mappedX, g_Touch.mappedY), 20.0f, IM_COL32(255, 0, 0, 255));
    }

    // 菜单界面
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.8f, h * 0.8f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Auto-Adaptive Menu", nullptr, ImGuiWindowFlags_NoCollapse)) {
        
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "★ 全自动适配引擎已启动 ★");
        ImGui::Text("Render Res (实时): %.0f x %.0f", g_Touch.renderW, g_Touch.renderH);
        ImGui::Text("Auto Scale (实时): X=%.4f, Y=%.4f", g_Touch.scaleX, g_Touch.scaleY);
        ImGui::Separator();
        
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "如果点不准，只需点击下方按钮，");
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "然后摸一下游戏【最左上角】即可。");
        
        if (ImGui::Button("🎯 一键自动校准偏移", ImVec2(-1, 120))) {
            g_WaitingForTopLeft = true;
        }

        ImGui::Separator();
        ImGui::Text("当前 Offset: X=%.1f, Y=%.1f", g_Touch.offsetX, g_Touch.offsetY);
        
        // 预留设备修改接口，如果换手机了，修改这里的物理分辨率即可
        if (ImGui::CollapsingHeader("高级设置 (换设备才需要点开)")) {
            ImGui::Text("设备物理分辨率 (横屏)");
            ImGui::DragFloat("Physical W", &g_Touch.physicalW, 1.0f, 1000.0f, 5000.0f);
            ImGui::DragFloat("Physical H", &g_Touch.physicalH, 1.0f, 1000.0f, 5000.0f);
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
