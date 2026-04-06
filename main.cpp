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

// 用于窗口化坐标转换的全局变量
static float g_ScreenScaleX = 1.0f;
static float g_ScreenScaleY = 1.0f;
static bool g_NeedCalibrate = true;

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
        
        float raw_x = _getX(event, 0);
        float raw_y = _getY(event, 0);

        if (raw_x >= 0 && raw_y >= 0) {
            g_TouchData.x = raw_x;
            g_TouchData.y = raw_y;
            
            if (action == AMOTION_EVENT_ACTION_DOWN) g_TouchData.down = true;
            else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_TouchData.down = false;
        }
    }
}

int hook_InputConsumer_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int result = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    if (result == 0 && outEvent && *outEvent) {
        handle_motion_event(*outEvent);
    }
    return result;
}

// 渲染钩子
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
    
    // --- 核心修复：动态坐标映射 ---
    // 逻辑：如果捕获到的坐标 Y (如2128) 大于 渲染高度 (1008)
    // 说明系统给的是物理全屏坐标。我们需要计算缩放比例。
    if (g_TouchData.y > (float)height && g_NeedCalibrate) {
        // 假设常见手机高度为 2400 左右，这里我们根据第一次点击自动校准
        // 也可以手动设置：g_ScreenScaleY = (float)height / 2400.0f;
        // 更稳妥的做法是：如果超出范围，按当前最高比例缩放
        g_ScreenScaleY = (float)height / 2400.0f; // 这里的 2400 是假设值，后续可微调
        g_ScreenScaleX = (float)width / 1080.0f;  
        // g_NeedCalibrate = false; 
    }

    // 如果你发现还是点不准，最简单的暴力解法：
    // 根据你的截图比例 1008/2128 ≈ 0.47
    float mappedX = g_TouchData.x * ((float)width / 1080.0f); // 假设宽1080
    float mappedY = g_TouchData.y * ((float)height / 2400.0f); // 假设高2400

    // 或者直接使用固定比例（根据你的截图计算所得）
    // mappedY = g_TouchData.y * 0.473f; 
    // mappedX = g_TouchData.x * 0.594f; // 1426/2400 左右

    io.MousePos = ImVec2(mappedX, mappedY);
    io.MouseDown[0] = g_TouchData.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(width * 0.8f, height * 0.8f), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty SGame Ultimate", &g_ShowMenu)) {
            ImGui::Text("Render Size: %d x %d", width, height);
            ImGui::Text("Raw Touch: %.1f, %.1f", g_TouchData.x, g_TouchData.y);
            ImGui::Text("Mapped Pos: %.1f, %.1f", mappedX, mappedY);
            
            ImGui::Separator();
            static float manualScaleX = 0.59f;
            static float manualScaleY = 0.47f;
            ImGui::SliderFloat("Scale X", &manualScaleX, 0.1f, 2.0f);
            ImGui::SliderFloat("Scale Y", &manualScaleY, 0.1f, 2.0f);
            
            // 使用滑块实时调整，直到点准为止
            io.MousePos = ImVec2(g_TouchData.x * manualScaleX, g_TouchData.y * manualScaleY);

            ImGui::Separator();
            static bool esp = false;
            ImGui::Checkbox("Enable ESP", &esp);
            if (ImGui::Button("Close Menu")) { /* g_ShowMenu = false; */ }
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

    void* egl_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl_addr) {
        DobbyHook(egl_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    }

    const char* consume_sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume_addr = DobbySymbolResolver("libinput.so", consume_sym);

    if (consume_addr) {
        DobbyHook(consume_addr, (void*)hook_InputConsumer_consume, (void**)&old_consume);
        LOGI("Input Hook Success.");
    }

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
