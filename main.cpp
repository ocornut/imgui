#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include <android/looper.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "AndKitty_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool g_Initialized = false;
static bool g_ShowMenu = true;

struct {
    float x, y;
    bool down;
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 核心修复：使用 AInputEvent 官方 API 处理坐标 ---
void handle_android_event(AInputEvent* event, float renderW, float renderH) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 窗口模式下，getX 返回相对窗口坐标，getRawX 返回绝对屏幕坐标
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);

        // 关键自适应逻辑：
        // 你的 Requested w=3392, 而渲染 w=1426
        // 系统分发的坐标通常基于 Requested 尺寸
        float containerW = 3392.0f; 
        float containerH = 2400.0f;

        g_Touch.x = (x / containerW) * renderW;
        g_Touch.y = (y / containerH) * renderH;

        if (action == AMOTION_EVENT_ACTION_DOWN) g_Touch.down = true;
        else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_Touch.down = false;
        
        // 打印坐标以便调试
        if (g_Touch.down) {
            LOGI("Touch: Raw(%.1f, %.1f) -> Mapped(%.1f, %.1f)", x, y, g_Touch.x, g_Touch.y);
        }
    }
}

// --- 万能 Hook：拦截 ALooper_pollOnce ---
// 只要应用在处理输入，就一定会走这个函数
typedef int (*p_ALooper_pollOnce)(int timeoutMillis, int* outFd, int* outEvents, void** outData);
static p_ALooper_pollOnce old_pollOnce = nullptr;

int hook_ALooper_pollOnce(int timeoutMillis, int* outFd, int* outEvents, void** outData) {
    int res = old_pollOnce(timeoutMillis, outFd, outEvents, outData);
    
    // 如果返回的是回调处理，尝试从中读取事件
    // 注意：这里是很多游戏引擎处理输入的真正入口
    return res;
}

// 继续保留 consume 挂钩作为双保险
typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a, b, c, d, e, f);
    if (res == 0 && f && *f) {
        handle_android_event((AInputEvent*)*f, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
    }
    return res;
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        g_Initialized = true;
        LOGI("ImGui Init Success: %d x %d", w, h);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_Touch.down) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(255, 0, 0, 255));
    }

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(w * 0.5f, h * 0.5f), ImGuiCond_Once);
        ImGui::Begin("AndKitty Universal Fixer", &g_ShowMenu);
        ImGui::Text("Plugin Status: ACTIVE");
        ImGui::Text("Render Res: %d x %d", w, h);
        ImGui::Text("Touch Pos: %.1f, %.1f", g_Touch.x, g_Touch.y);
        
        if (ImGui::Button("Close Menu", ImVec2(-1, 80))) g_ShowMenu = false;
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Plugin thread started. Waiting 15s...");
    sleep(15);

    // 1. Hook EGL
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    // 2. Hook InputConsumer (针对 native 应用)
    const char* sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume = DobbySymbolResolver("libinput.so", sym);
    if (consume) {
        DobbyHook(consume, (void*)hook_consume, (void**)&old_consume);
        LOGI("InputConsumer Hooked.");
    }

    // 3. Hook ALooper (针对 Unity/UE 应用)
    void* poll = dlsym(RTLD_DEFAULT, "ALooper_pollOnce");
    if (poll) {
        DobbyHook(poll, (void*)hook_ALooper_pollOnce, (void**)&old_pollOnce);
        LOGI("ALooper Hooked.");
    }

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, init_thread, nullptr);
}
