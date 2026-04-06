#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include <stdio.h>
#include <algorithm>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "ZeroConfig_Drive"

static bool g_Initialized = false;

// --- Zero-Config Core Structure ---
struct {
    float x = 0.0f;
    float y = 0.0f;
    bool down = false;
    
    float renderW = 0.0f; 
    float renderH = 0.0f;

    // Physical screen resolution
    float physW = 3392.0f;
    float physH = 2400.0f;
    
    bool isSizeAutoDetected = false;
} g_Touch;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

// --- Auto Fetch Device Physical Resolution ---
void AutoFetchDeviceResolution() {
    FILE* pipe = popen("wm size", "r");
    if (!pipe) return;
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        int w = 0, h = 0;
        if (sscanf(buffer, "Physical size: %dx%d", &w, &h) == 2 ||
            sscanf(buffer, "Override size: %dx%d", &w, &h) == 2) {
            
            // Landscape force logic
            g_Touch.physW = (float)std::max(w, h);
            g_Touch.physH = (float)std::min(w, h);
            g_Touch.isSizeAutoDetected = true;
        }
    }
    pclose(pipe);
}

// --- Input Event Handler ---
void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        float rawX = AMotionEvent_getX(event, 0);
        float rawY = AMotionEvent_getY(event, 0);
        
        // Auto Scaling Logic
        float autoScaleX = g_Touch.renderW / g_Touch.physW;
        float autoScaleY = g_Touch.renderH / g_Touch.physH;

        g_Touch.x = rawX * autoScaleX;
        g_Touch.y = rawY * autoScaleY;

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

    // --- 纯净原生初始化 ---
    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        // 仅在初始化时进行一次移动端适配放大，不再干涉后续的原生运行逻辑
        float initScale = (float)h / 1000.0f;
        if (initScale < 1.0f) initScale = 1.0f;
        
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(3.5f * initScale);
        ImGui::GetIO().FontGlobalScale = 3.5f * initScale;
        
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 绘制点击反馈点
    if (g_Touch.down) {
        ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(0, 255, 0, 200));
    }

    // --- 菜单 UI ---
    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.6f, h * 0.5f), ImGuiCond_FirstUseEver);
    
    // 原汁原味的 ImGui 窗口
    if (ImGui::Begin("Zero-Config Engine")) {
        
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[*] Pure Native ImGui");
        ImGui::Separator();
        
        ImGui::Text("Render Surface: %.0f x %.0f", g_Touch.renderW, g_Touch.renderH);
        
        if (g_Touch.isSizeAutoDetected) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Physical Screen: %.0f x %.0f (Auto)", g_Touch.physW, g_Touch.physH);
        } else {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Physical Screen: %.0f x %.0f (Fallback)", g_Touch.physW, g_Touch.physH);
        }

        ImGui::Text("Current Scale: X=%.4f, Y=%.4f", 
                    g_Touch.renderW / g_Touch.physW, 
                    g_Touch.renderH / g_Touch.physH);
        
        ImGui::Separator();
        ImGui::Text("- Drag bottom-right corner to resize.");
        ImGui::Text("- Double-click title bar to collapse.");
        ImGui::Separator();

        static bool esp_enabled = true;
        ImGui::Checkbox("Enable ESP (Demo)", &esp_enabled);

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
    AutoFetchDeviceResolution();
    
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
