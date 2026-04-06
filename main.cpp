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

// 用于窗口化坐标转换
static int g_RealWidth = 1;
static int g_RealHeight = 1;

// --- 原函数指针 ---
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

// 解析 MotionEvent 坐标并进行归一化处理
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
            // 这里是修复核心：
            // 如果是窗口化，系统返回的是屏幕绝对坐标（如 Y=2128）
            // 但 ImGui 需要的是窗口相对坐标。
            // 我们先假设游戏窗口是居中的或者全屏显示的（最通用方案）
            g_TouchData.x = raw_x;
            g_TouchData.y = raw_y;
            
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
    EGLint width, height;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::GetStyle().ScaleAllSizes(3.0f); 
        g_Initialized = true;
    }

    g_RealWidth = width;
    g_RealHeight = height;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    
    // 关键修正：坐标映射
    // 获取屏幕物理分辨率（假设通过这种方式简单映射，如果还是点不准，说明需要获取物理屏幕高度）
    // 这里的逻辑：如果 Touch_Y > 窗口高度，说明触摸的是物理坐标，需要按比例缩放
    // 临时修正方案：强制限制坐标在显示范围内
    float finalX = g_TouchData.x;
    float finalY = g_TouchData.y;
    
    // 假设你的手机全屏高度大概是 2400 左右，而窗口是 1008
    // 我们做一个简单的比例映射（如果这是由于系统缩放导致的）
    if (finalY > height) {
        // 自动计算缩放比例
        static float scaleY = -1.0f;
        if (scaleY < 0 && g_TouchData.down) {
            // 第一次按下时尝试计算比例
            // 这是一个估算值，因为我们不知道物理屏幕具体多大
            scaleY = (float)height / 2400.0f; // 假设物理高 2400
        }
        // finalY *= 0.42f; // 这里的 0.42 是 1008/2400 的近似值
    }

    io.MousePos = ImVec2(finalX, finalY);
    io.MouseDown[0] = g_TouchData.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowPos(ImVec2(width * 0.1f, height * 0.1f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(width * 0.7f, height * 0.7f), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("AndKitty SGame Ultimate", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Input: AInputQueue + HandleEvent");
            ImGui::Separator();
            
            ImGui::Text("Mouse Pos: %.1f, %.1f", io.MousePos.x, io.MousePos.y);
            ImGui::Text("WantCapture: %s", io.WantCaptureMouse ? "YES" : "NO");
            ImGui::Separator();
            
            static bool esp = false;
            ImGui::Checkbox("Enable ESP", &esp);
            
            static int range = 50;
            ImGui::SliderInt("Range", &range, 0, 100);
            
            if (ImGui::Button("Close Menu", ImVec2(120, 50))) {
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
    LOGI("Plugin thread started. Waiting for game to stabilize...");
    sleep(15); 

    void* egl_addr = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (egl_addr) {
        DobbyHook(egl_addr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    }

    const char* consume_sym = "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE";
    void* consume_addr = DobbySymbolResolver("libinput.so", consume_sym);

    if (consume_addr) {
        DobbyHook(consume_addr, (void*)hook_InputConsumer_consume, (void**)&old_consume);
        LOGI("AInputQueue Hooked Successfully.");
    }

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
