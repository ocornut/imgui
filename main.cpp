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

// --- 触摸数据结构 ---
struct {
    float x, y;          
    bool down;
    // 使用 Frida 测出的分屏比例 (1426 / 2802)
    float scaleX = 0.5089f; 
    float scaleY = 0.5089f; 
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// --- 触摸事件处理逻辑 ---
void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float lx = AMotionEvent_getX(event, 0);
        float ly = AMotionEvent_getY(event, 0);
        
        // 映射坐标到 ImGui
        g_Touch.x = lx * g_Touch.scaleX;
        g_Touch.y = ly * g_Touch.scaleY;

        if (action == AMOTION_EVENT_ACTION_DOWN) g_Touch.down = true;
        else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_Touch.down = false;
    }
}

// --- InputConsumer Hook ---
typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a, b, c, d, e, f);
    if (res == 0 && f && *f) {
        handle_android_event((AInputEvent*)*f);
    }
    return res;
}

// 渲染钩子函数
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        LOGI("Successfully Entered Hook! Initializing ImGui...");
        
        EGLint width, height;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);
        LOGI("Detected Display Size: %d x %d", width, height);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
        
        ImGui::GetStyle().ScaleAllSizes(3.0f); 
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
        LOGI("ImGui Initialized Successfully!");
    }

    // 同步触摸状态到 ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 强制显示一个简单的测试窗口
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("AndKitty Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1,1,0,1), "Plugin Status: ACTIVE");
        ImGui::Separator();
        
        ImGui::Text("Touch Pos: %.1f, %.1f", g_Touch.x, g_Touch.y);
        ImGui::Text("Package: com.tencent.jkchess");
        
        static bool test_check = true;
        ImGui::Checkbox("Test Feature", &test_check);
        
        ImGui::SliderFloat("ScaleX", &g_Touch.scaleX, 0.4f, 1.2f);
        
        if (ImGui::Button("Close Menu", ImVec2(-1, 60))) {
            // 逻辑处理
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    // 已根据要求修改为 5 秒
    LOGI("Step 1: Thread Started, sleeping 5s...");
    sleep(5); 

    LOGI("Step 2: Searching for symbols...");
    
    // 1. Hook 渲染
    void* sym_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (sym_addr) {
        LOGI("Step 3: Found eglSwapBuffers at %p, Hooking...", sym_addr);
        DobbyHook(sym_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    }

    // 2. Hook 输入
    const char* consume_sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume_addr = DobbySymbolResolver("libinput.so", consume_sym);
    if (consume_addr) {
        LOGI("Step 3.5: Found InputConsumer at %p, Hooking...", consume_addr);
        DobbyHook(consume_addr, (void*)hook_consume, (void**)&old_consume);
    }

    LOGI("Step 4: All Done!");
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
