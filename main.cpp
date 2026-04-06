#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

// --- 状态存储 ---
static bool g_Initialized = false;
static bool g_ShowMenu = true;

struct {
    float x, y;
    bool down;
} g_TouchData = {0, 0, false};

// 坐标映射修正参数
static float g_FixedScaleX = 1.0f;
static float g_FixedScaleY = 1.0f;
static bool g_SwapXY = false; // 是否需要旋转坐标

// --- 原函数指针 ---
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

void handle_motion_event(void* event) {
    if (!event) return;

    static auto _getType = (int (*)(void*))dlsym(RTLD_DEFAULT, "AInputEvent_getType");
    static auto _getAction = (int (*)(void*))dlsym(RTLD_DEFAULT, "AMotionEvent_getAction");
    static auto _getX = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getX");
    static auto _getY = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getY");

    if (_getType && _getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int action = _getAction(event) & AMOTION_EVENT_ACTION_MASK;
        g_TouchData.x = _getX(event, 0);
        g_TouchData.y = _getY(event, 0);
        
        if (action == AMOTION_EVENT_ACTION_DOWN) g_TouchData.down = true;
        else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_TouchData.down = false;
    }
}

int hook_InputConsumer_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int result = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    if (result == 0 && outEvent && *outEvent) {
        handle_motion_event(*outEvent);
    }
    return result;
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint width, height;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::GetStyle().ScaleAllSizes(3.0f); 
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    
    // --- 核心修复逻辑：基于滑块和自动映射 ---
    // 根据截图 18:55 的数据计算的初始值：
    // 原始 X=1752 (其实是竖屏的 Y), 原始 Y=934 (其实是竖屏的 X)
    // 渲染 W=1426, H=1008
    static float sX = 0.81f; // 1426 / 1752 左右
    static float sY = 1.07f; // 1008 / 934 左右
    static bool flipX = false;
    static bool flipY = false;

    float finalX = g_TouchData.x * sX;
    float finalY = g_TouchData.y * sY;

    if (g_SwapXY) {
        finalX = g_TouchData.y * sX;
        finalY = g_TouchData.x * sY;
    }

    io.MousePos = ImVec2(finalX, finalY);
    io.MouseDown[0] = g_TouchData.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(width * 0.9f, height * 0.9f), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty SGame Fixer", &g_ShowMenu)) {
            ImGui::Text("Win: %dx%d | Raw: %.1f, %.1f", width, height, g_TouchData.x, g_TouchData.y);
            ImGui::Text("Mapped: %.1f, %.1f", finalX, finalY);
            ImGui::Separator();
            
            // 调试滑块
            ImGui::Checkbox("Swap X/Y (旋转坐标轴)", &g_SwapXY);
            ImGui::SliderFloat("Scale X (宽度映射)", &sX, 0.0f, 2.0f);
            ImGui::SliderFloat("Scale Y (高度映射)", &sY, 0.0f, 2.0f);
            
            ImGui::Separator();
            if (ImGui::Button("自动校准 (点这里后点击屏幕右下角)", ImVec2(-1, 80))) {
                // 点击后请点一下游戏小窗口的最右下角
                if (g_TouchData.x > 100 && g_TouchData.y > 100) {
                   if (g_TouchData.x > g_TouchData.y) {
                       sX = (float)width / g_TouchData.x;
                       sY = (float)height / g_TouchData.y;
                       g_SwapXY = false;
                   } else {
                       sX = (float)width / g_TouchData.y;
                       sY = (float)height / g_TouchData.x;
                       g_SwapXY = true;
                   }
                }
            }
            
            ImGui::Separator();
            static bool esp = false;
            ImGui::Checkbox("Enable Feature", &esp);
            if (ImGui::Button("Close Menu", ImVec2(200, 60))) { /* g_ShowMenu = false; */ }
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    sleep(15); 

    void* egl_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl_addr) DobbyHook(egl_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    const char* consume_sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume_addr = DobbySymbolResolver("libinput.so", consume_sym);
    if (consume_addr) DobbyHook(consume_addr, (void*)hook_InputConsumer_consume, (void**)&old_consume);

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
