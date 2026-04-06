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
    float scaleX = 0.5089f; // 基于之前 Frida 测得的 1426/2802
    float scaleY = 0.5089f; 
    float offsetX = 0.0f; 
    float offsetY = 0.0f;
    float lastLocalX, lastLocalY;
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

void handle_android_event(AInputEvent* event, float renderW, float renderH) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float lx = AMotionEvent_getX(event, 0);
        float ly = AMotionEvent_getY(event, 0);
        g_Touch.lastLocalX = lx; 
        g_Touch.lastLocalY = ly;
        g_Touch.x = (lx * g_Touch.scaleX) + g_Touch.offsetX;
        g_Touch.y = (ly * g_Touch.scaleY) + g_Touch.offsetY;

        if (action == AMOTION_EVENT_ACTION_DOWN) g_Touch.down = true;
        else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_Touch.down = false;
    }
}

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

    if (w <= 0 || h <= 0) return old_eglSwapBuffers(dpy, surface);

    if (!g_Initialized) {
        LOGI("Initializing ImGui for Surface: %dx%d", w, h);
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::GetStyle().ScaleAllSizes(3.0f); // 适配高分屏
        
        // 设置字体大小
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = 3.5f; 
        
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(); // 确保 Android 后端也更新
    ImGui::NewFrame();

    // 绘制常驻指示器 (如果菜单不出来，至少能看到左上角这个方块)
    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(10, 10), ImVec2(60, 60), IM_COL32(0, 255, 0, 200));

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(w * 0.8f, h * 0.6f), ImGuiCond_Once);
        ImGui::Begin("AndKitty Menu (SGame Fixed)", &g_ShowMenu, ImGuiWindowFlags_NoCollapse);
        
        ImGui::Text("Render Res: %d x %d", w, h);
        ImGui::Text("Input Logic: %.1f, %.1f", g_Touch.lastLocalX, g_Touch.lastLocalY);
        
        ImGui::Separator();
        ImGui::SliderFloat("Scale X", &g_Touch.scaleX, 0.3f, 1.0f, "%.4f");
        ImGui::SliderFloat("Offset X", &g_Touch.offsetX, -100.0f, 100.0f, "%.1f");

        if (ImGui::Button("Reset Menu Position", ImVec2(-1, 80))) {
            ImGui::SetWindowPos(ImVec2(50, 50));
        }

        if (ImGui::Button("Hide Menu (Click Green Box to show)", ImVec2(-1, 80))) {
            g_ShowMenu = false;
        }

        ImGui::End();
    } else {
        // 如果菜单隐藏了，点击左上角绿方块恢复
        if (g_Touch.down && g_Touch.x < 100 && g_Touch.y < 100) {
            g_ShowMenu = true;
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    // 王者荣耀建议延迟 20-25 秒，避开启动时的环境检测和黑屏阶段
    LOGI("Thread started, waiting 20s for injection...");
    sleep(20);
    
    void* egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl) {
        DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("Hooked eglSwapBuffers at %p", egl);
    }

    const char* sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume = DobbySymbolResolver("libinput.so", sym);
    if (consume) {
        DobbyHook(consume, (void*)hook_consume, (void**)&old_consume);
        LOGI("Hooked InputConsumer::consume");
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, init_thread, nullptr);
}
