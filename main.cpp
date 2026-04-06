#include <pthread.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <time.h>
#include <unistd.h>
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "dobby.h"
#include <android/log.h>

#define LOG_TAG "AndKitty_Final"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- 状态存储 ---
static bool g_Initialized = false;
static bool g_ShowMenu = true;
static float g_Width = 0, g_Height = 0;

struct {
    float x, y;
    bool down;
} g_TouchData = {0, 0, false};

// --- 原函数指针 ---
// 对应日志中的符号: android::InputConsumer::consume(...)
typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, nsecs_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 核心底层 Hook ---
int hook_InputConsumer_consume(void* instance, void* factory, bool consumeBatches, nsecs_t frameTime, uint32_t* outSeq, void** outEvent) {
    // 先执行原函数，获取事件
    int result = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);

    // result == 0 说明成功消耗了一个事件 (OK)
    if (result == 0 && outEvent && *outEvent) {
        void* event = *outEvent;
        
        // 这里的 event 实际上是 android::MotionEvent*
        // 我们通过导入 libandroid.so 的函数来解析这个对象，这是最稳妥的，不需要管偏移
        typedef int (*p_getType)(void*);
        typedef int (*p_getAction)(void*);
        typedef float (*p_getX)(void*, size_t);
        typedef float (*p_getY)(void*, size_t);

        static p_getType _getType = (p_getType)dlsym(RTLD_DEFAULT, "AInputEvent_getType");
        static p_getAction _getAction = (p_getAction)dlsym(RTLD_DEFAULT, "AMotionEvent_getAction");
        static p_getX _getX = (p_getX)dlsym(RTLD_DEFAULT, "AMotionEvent_getX");
        static p_getY _getY = (p_getY)dlsym(RTLD_DEFAULT, "AMotionEvent_getY");

        if (_getType && _getType(event) == 1) { // AINPUT_EVENT_TYPE_MOTION
            int action = _getAction(event) & 0xFF; // AMOTION_EVENT_ACTION_MASK
            float x = _getX(event, 0);
            float y = _getY(event, 0);

            if (x >= 0 && y >= 0) {
                g_TouchData.x = x;
                g_TouchData.y = y;
                if (action == 0) g_TouchData.down = true;       // ACTION_DOWN
                else if (action == 1) g_TouchData.down = false; // ACTION_UP
                else if (action == 3) g_TouchData.down = false; // ACTION_CANCEL
            }
        }
    }
    return result;
}

// --- 渲染逻辑 ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (g_Width <= 0) {
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        g_Width = (float)w; g_Height = (float)h;
    }

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(g_Width, g_Height);
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::StyleColorsDark();
        g_Initialized = true;
    }

    // 强制同步底层拦截到的坐标
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(g_TouchData.x, g_TouchData.y);
    io.MouseDown[0] = g_TouchData.down;
    io.DeltaTime = 1.0f / 60.0f;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("AndKitty SGame Final", &g_ShowMenu)) {
            ImGui::Text("Touch Pos: %.1f, %.1f", g_TouchData.x, g_TouchData.y);
            ImGui::Text("Status: %s", g_TouchData.down ? "DOWN" : "UP");
            static float val = 0.5f;
            ImGui::SliderFloat("Slider", &val, 0.0f, 1.0f);
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    sleep(5);

    // 1. Hook 渲染
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 2. Hook 底层输入 (使用日志中发现的符号)
    // 注意：libinput.so 在不同系统可能路径不同，直接从内存搜索符号最稳
    void* consume_addr = dlsym(RTLD_DEFAULT, "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (consume_addr) {
        DobbyHook(consume_addr, (void*)hook_InputConsumer_consume, (void**)&old_consume);
        LOGI("Successfully Hooked InputConsumer::consume");
    } else {
        LOGI("Failed to find consume symbol via RTLD_DEFAULT");
    }

    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
