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

#define TAG "AndKitty_Fixer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static bool g_Initialized = false;
static bool g_ShowMenu = true;

// 存储最终计算出的自适应坐标
struct {
    float x, y;
    bool down;
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

// 自适应处理核心函数
void handle_adaptive_event(void* event, float renderW, float renderH) {
    if (!event) return;
    static auto _getType = (int (*)(void*))dlsym(RTLD_DEFAULT, "AInputEvent_getType");
    static auto _getAction = (int (*)(void*))dlsym(RTLD_DEFAULT, "AMotionEvent_getAction");
    
    // NDK 原生 API
    static auto _getX = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getX");
    static auto _getY = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getY");
    
    // 尝试获取原始坐标和物理范围
    // 在窗口模式下，RawX/Y 通常是屏幕物理像素，而 getX/Y 往往已经被系统转换到了窗口局部坐标
    if (_getType && _getType(event) == 1) { // AINPUT_EVENT_TYPE_MOTION
        int action = _getAction(event) & 0xff;
        
        /* * 自适应原理：
         * 大多数 Android 窗口系统在分发事件给 InputConsumer 时，
         * 已经将坐标转换为了相对于 Surface 的局部坐标。
         * 如果点不准，通常是因为 ImGui 渲染的分辨率与窗口实际 Surface 分辨率不一致。
         */
        float rawX = _getX(event, 0);
        float rawY = _getY(event, 0);

        // 如果你的 dumpsys 显示 Requested w=3392 而 ImGui 是 1426
        // 我们需要做一个简单的动态比例缩放
        // 这里的 3392.0f 和 2400.0f 是从你 dumpsys 里的 Requested w/h 获取的
        float containerW = 3392.0f;
        float containerH = 2400.0f;

        g_Touch.x = (rawX / containerW) * renderW;
        g_Touch.y = (rawY / containerH) * renderH;

        if (action == 0) g_Touch.down = true;
        else if (action == 1 || action == 3) g_Touch.down = false;
    }
}

int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a, b, c, d, e, f);
    // 注意：在这里我们还拿不到渲染窗口的宽高，所以只存下原始事件
    // 具体的坐标转换放在 eglSwapBuffers 里实时计算
    if (res == 0 && f && *f) {
        // 传递当前全局事件指针
        handle_adaptive_event(*f, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
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
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);

    // 应用自适应坐标
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_Touch.down) {
        // 画出红色触控点，用于验证自适应是否成功
        ImGui::GetBackgroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(255, 0, 0, 255));
    }

    if (g_ShowMenu) {
        ImGui::Begin("AndKitty Adaptive Menu", &g_ShowMenu);
        ImGui::Text("Window Size: %.0f x %.0f", io.DisplaySize.x, io.DisplaySize.y);
        ImGui::Text("Touch Logic Pos: %.1f, %.1f", g_Touch.x, g_Touch.y);
        
        if (ImGui::Button("隐藏菜单", ImVec2(-1, 80))) g_ShowMenu = false;
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    sleep(15);
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    const char* sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume = DobbySymbolResolver("libinput.so", sym);
    if (consume) DobbyHook(consume, (void*)hook_consume, (void**)&old_consume);
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, init_thread, nullptr);
}
