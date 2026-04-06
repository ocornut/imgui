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

// 存储触摸信息与 Frida 测得的比例
struct {
    float x, y;          
    bool down;
    // 基于 Frida 测量值：Logic Width 2802 / Render Width 1426 ≈ 1.965 (反向即 0.5089)
    float scaleX = 0.5089f; 
    float scaleY = 0.5089f; 
    float offsetX = 0.0f; 
    float offsetY = 0.0f;
    float lastLocalX, lastLocalY;
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

// 统一处理触摸事件
void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float lx = AMotionEvent_getX(event, 0);
        float ly = AMotionEvent_getY(event, 0);
        
        g_Touch.lastLocalX = lx; 
        g_Touch.lastLocalY = ly;
        // 应用比例转换
        g_Touch.x = (lx * g_Touch.scaleX) + g_Touch.offsetX;
        g_Touch.y = (ly * g_Touch.scaleY) + g_Touch.offsetY;

        if (action == AMOTION_EVENT_ACTION_DOWN) g_Touch.down = true;
        else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_Touch.down = false;
    }
}

// 挂钩底层的 InputConsumer 以获取最准确的输入流
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
    EGLint width, height;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

    if (width <= 0 || height <= 0) return old_eglSwapBuffers(dpy, surface);

    if (!g_Initialized) {
        LOGI("Successfully Entered Hook! Display: %d x %d", width, height);
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::StyleColorsDark();
        
        // 针对高分屏调整尺寸
        ImGui::GetStyle().ScaleAllSizes(3.0f); 
        io.FontGlobalScale = 3.0f;
        
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 绘制调试红点，确认触摸是否对齐
    if (g_Touch.down) {
        ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(255, 0, 0, 255));
    }

    if (g_ShowMenu) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty SGame Menu", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: ACTIVE (Frida Swapped)");
            ImGui::Text("Logic Pos: %.1f, %.1f", g_Touch.lastLocalX, g_Touch.lastLocalY);
            ImGui::Text("Render Pos: %.1f, %.1f", g_Touch.x, g_Touch.y);
            
            ImGui::Separator();
            // 在这里微调比例
            ImGui::SliderFloat("Scale X", &g_Touch.scaleX, 0.3f, 1.0f, "%.4f");
            ImGui::SliderFloat("Offset X", &g_Touch.offsetX, -100.0f, 100.0f, "%.1f");

            if (ImGui::Button("Reset Position", ImVec2(-1, 80))) {
                ImGui::SetWindowPos(ImVec2(100, 100));
            }

            if (ImGui::Button("Hide Menu", ImVec2(-1, 80))) g_ShowMenu = false;
        }
        ImGui::End();
    } else {
        // 隐藏后点左上角恢复
        ImGui::GetForegroundDrawList()->AddCircleFilled(ImVec2(50, 50), 30.0f, IM_COL32(0, 255, 0, 150));
        if (g_Touch.down && g_Touch.x < 100 && g_Touch.y < 100) g_ShowMenu = true;
    }

    ImGui::Render();

    // 渲染保护：防止被游戏深度缓冲区遮挡
    GLint last_depth_test;
    glGetIntegerv(GL_DEPTH_TEST, &last_depth_test);
    glDisable(GL_DEPTH_TEST);
    
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (last_depth_test) glEnable(GL_DEPTH_TEST);

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    LOGI("Step 1: Thread Started, sleeping 20s for SGame...");
    sleep(20); 

    // 1. Hook 渲染
    void* sym_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (sym_addr) {
        DobbyHook(sym_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Step 2: eglSwapBuffers Hooked!");
    }

    // 2. Hook 底层输入 (解决坐标不准的核心)
    const char* consume_sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume_addr = DobbySymbolResolver("libinput.so", consume_sym);
    if (consume_addr) {
        DobbyHook(consume_addr, (void*)hook_consume, (void**)&old_consume);
        LOGI("Step 3: InputConsumer Hooked!");
    }
    
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
