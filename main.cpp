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

static bool g_Initialized = false;
static bool g_ShowMenu = true;

// 最终映射后的坐标
struct {
    float x, y;
    bool down;
} g_FinalTouch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

// 获取系统内置转换
typedef float (*p_getAxisValue)(void* event, int32_t axis, size_t pointerIndex);
static p_getAxisValue g_getAxisValue = nullptr;

void process_input_event(void* event, float screenW, float screenH) {
    if (!event) return;
    
    static auto _getType = (int (*)(void*))dlsym(RTLD_DEFAULT, "AInputEvent_getType");
    static auto _getAction = (int (*)(void*))dlsym(RTLD_DEFAULT, "AMotionEvent_getAction");
    static auto _getX = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getX");
    static auto _getY = (float (*)(void*, size_t))dlsym(RTLD_DEFAULT, "AMotionEvent_getY");
    
    if (_getType && _getType(event) == 1) { // MotionEvent
        int action = _getAction(event) & 0xff;
        
        /* * 自适应核心逻辑：
         * 在窗口化模式下，AInputEvent_getX 返回的是相对于窗口(Surface)的坐标。
         * 但是，这个坐标的单位是系统的“逻辑单位”(例如 3392x2400)。
         * 我们需要根据 ImGui 实际渲染的窗口大小 (renderW/H) 进行二次比例转换。
         */
        float sysX = _getX(event, 0);
        float sysY = _getY(event, 0);

        // 这里 3392 和 2400 是根据你 dumpsys 里的物理逻辑尺寸
        // 如果是全屏，这两个值通常等于 renderW/H，比例就是 1:1
        // 如果是分屏，系统会自动处理偏移，我们只需要缩放比例
        float designW = 3392.0f; 
        float designH = 2400.0f;

        g_FinalTouch.x = (sysX / designW) * screenW;
        g_FinalTouch.y = (sysY / designH) * screenH;

        if (action == 0) g_FinalTouch.down = true;
        else if (action == 1 || action == 3) g_FinalTouch.down = false;
    }
}

int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a, b, c, d, e, f);
    if (res == 0 && f && *f) {
        // 在此处捕获坐标
        process_input_event(*f, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
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

    // 映射坐标到 ImGui
    io.MousePos = ImVec2(g_FinalTouch.x, g_FinalTouch.y);
    io.MouseDown[0] = g_FinalTouch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 可视化调试：在触碰点画一个圆
    if (g_FinalTouch.down) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(0, 255, 0, 255));
    }

    if (g_ShowMenu) {
        ImGui::Begin("AndKitty Universal Fixer", &g_ShowMenu);
        ImGui::Text("Render Res: %d x %d", w, h);
        ImGui::Text("Calculated Pos: %.1f, %.1f", g_FinalTouch.x, g_FinalTouch.y);
        
        if (ImGui::Button("Close", ImVec2(-1, 80))) g_ShowMenu = false;
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
    
    g_getAxisValue = (p_getAxisValue)dlsym(RTLD_DEFAULT, "AMotionEvent_getAxisValue");
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, init_thread, nullptr);
}
