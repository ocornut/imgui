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

// --- 原函数指针 ---
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

// 解析 MotionEvent 坐标
void handle_motion_event(void* event) {
    if (!event) return;

    static auto _getType = (int (*)(void*))dlsym(RTLD_DEFAULT, "AInputEvent_getType");
    static auto _getAction = (int (*)(void*))dlsym(RTLD_DEFAULT, "AMotionEvent_getAction");
    static auto _getX = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getX");
    static auto _getY = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getY");

    if (_getType && _getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int action = _getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 获取原始触摸点
        float x = _getX(event, 0);
        float y = _getY(event, 0);

        if (x >= 0 && y >= 0) {
            g_TouchData.x = x;
            g_TouchData.y = y;
            if (action == AMOTION_EVENT_ACTION_DOWN) g_TouchData.down = true;
            else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_TouchData.down = false;
        }
    }
}

// 输入钩子回调
int hook_InputConsumer_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int result = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    if (result == 0 && outEvent && *outEvent) {
        handle_motion_event(*outEvent);
    }
    return result;
}

// 渲染钩子
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // 动态获取当前渲染表面的宽高（适配窗口化/分屏）
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
    // 每一帧都更新显示大小，防止窗口缩放导致坐标偏移
    io.DisplaySize = ImVec2((float)width, (float)height);
    
    // 关键：将捕获的输入传递给 ImGui
    io.MousePos = ImVec2(g_TouchData.x, g_TouchData.y);
    io.MouseDown[0] = g_TouchData.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(width * 0.5f, height * 0.5f), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty Menu", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Plugin Status: ACTIVE");
            ImGui::Text("WinSize: %d x %d", width, height);
            ImGui::Text("Touch: %.1f, %.1f (%s)", g_TouchData.x, g_TouchData.y, g_TouchData.down ? "DOWN" : "UP");
            ImGui::Separator();
            
            static bool test_check = true;
            ImGui::Checkbox("Feature Test", &test_check);
            
            if (ImGui::Button("Close Menu")) {
                // g_ShowMenu = false;
            }
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Plugin thread started. Waiting 15s...");
    sleep(15); 

    // 1. Hook 渲染 (eglSwapBuffers)
    void* egl_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl_addr) {
        DobbyHook(egl_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("eglSwapBuffers Hooked.");
    }

    // 2. 强力查找 libinput.so 的 consume 符号
    const char* consume_sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume_addr = DobbySymbolResolver("libinput.so", consume_sym);

    if (!consume_addr) {
        void* hLibInput = dlopen("libinput.so", RTLD_NOW);
        if (hLibInput) {
            consume_addr = dlsym(hLibInput, consume_sym);
        }
    }

    if (consume_addr) {
        DobbyHook(consume_addr, (void*)hook_InputConsumer_consume, (void**)&old_consume);
        LOGI("Input Consumer Hooked at %p", consume_addr);
    } else {
        LOGI("CRITICAL ERROR: Failed to locate InputConsumer::consume!");
    }

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
