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

// 对应日志中的底层符号: android::InputConsumer::consume(...)
typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

// --- 底层输入钩子 ---
int hook_InputConsumer_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int result = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);

    // 如果成功获取到事件包
    if (result == 0 && outEvent && *outEvent) {
        void* event = *outEvent;

        // 动态获取解析函数，防止库加载顺序问题
        static auto _getType = (int (*)(void*))dlsym(RTLD_DEFAULT, "AInputEvent_getType");
        static auto _getAction = (int (*)(void*))dlsym(RTLD_DEFAULT, "AMotionEvent_getAction");
        static auto _getX = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getX");
        static auto _getY = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getY");

        if (_getType && _getType(event) == AINPUT_EVENT_TYPE_MOTION) {
            int action = _getAction(event) & AMOTION_EVENT_ACTION_MASK;
            float x = _getX(event, 0);
            float y = _getY(event, 0);

            // 存入全局变量供 ImGui 使用
            if (x >= 0 && y >= 0) {
                g_TouchData.x = x;
                g_TouchData.y = y;
                if (action == AMOTION_EVENT_ACTION_DOWN) g_TouchData.down = true;
                else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_TouchData.down = false;
            }
        }
    }
    return result;
}

// --- 渲染钩子函数 ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        LOGI("Initializing ImGui...");
        EGLint width, height;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);
        
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen; // 开启触摸标志
        
        ImGui::GetStyle().ScaleAllSizes(3.0f); 
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
    }

    // --- 关键：将底层捕获的输入喂给 ImGui ---
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(g_TouchData.x, g_TouchData.y);
    io.MouseDown[0] = g_TouchData.down;
    io.DeltaTime = 1.0f / 60.0f; // 维持恒定帧时，防止拖拽失效

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty Menu", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(1,1,0,1), "Plugin Status: ACTIVE");
            ImGui::Text("Input: %.0f, %.0f (%s)", g_TouchData.x, g_TouchData.y, g_TouchData.down ? "DOWN" : "UP");
            ImGui::Separator();
            
            static bool test_check = true;
            ImGui::Checkbox("Test Feature", &test_check);
            
            if (ImGui::Button("Close Menu")) {
                g_ShowMenu = false;
            }
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Step 1: Thread Started, sleeping 10s...");
    sleep(10); 

    // 1. Hook 渲染 (eglSwapBuffers)
    void* egl_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl_addr) {
        DobbyHook(egl_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Hooked eglSwapBuffers");
    }

    // 2. Hook 底层输入 (InputConsumer::consume)
    // 使用你 Frida 跑出来的那个精准符号
    const char* consume_sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume_addr = dlsym(RTLD_DEFAULT, consume_sym);
    
    if (consume_addr) {
        DobbyHook(consume_addr, (void*)hook_InputConsumer_consume, (void**)&old_consume);
        LOGI("Hooked InputConsumer::consume Successfully!");
    } else {
        LOGI("Failed to find InputConsumer::consume symbol!");
    }

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
