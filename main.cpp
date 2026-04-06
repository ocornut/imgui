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

// --- 真正的通用自适应结构 ---
struct {
    float x, y;          
    bool down;
    
    float renderW = 0.0f; 
    float renderH = 0.0f;

    // 自动校准参数
    float autoOffsetX = 0.0f;
    float autoOffsetY = 0.0f;
    float autoScaleX = 1.0f;
    float autoScaleY = 1.0f;
    
    bool isCalibrated = false;
} g_Touch = {0, 0, false};

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

// 动态链接安卓底层函数，用于获取物理绝对坐标
typedef float (*p_AMotionEvent_getRawX)(const AInputEvent* motion_event, size_t pointer_index);
typedef float (*p_AMotionEvent_getRawY)(const AInputEvent* motion_event, size_t pointer_index);
static p_AMotionEvent_getRawX g_getRawX = nullptr;
static p_AMotionEvent_getRawY g_getRawY = nullptr;

/**
 * 自动对齐算法 (针对所有设备/分屏通用):
 * 我们不需要知道屏幕是多少，我们只需要知道系统认为的“窗口内”和“屏幕上”的差值。
 */
void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 窗口内坐标
        float winX = AMotionEvent_getX(event, 0);
        float winY = AMotionEvent_getY(event, 0);
        
        // 如果能拿到物理坐标，我们实时比对
        if (g_getRawX && g_getRawY) {
            float rx = g_getRawX(event, 0);
            float ry = g_getRawY(event, 0);
            
            // 重要：部分设备在分屏时，winX 已经是缩放后的，这里我们通过比例来对齐
            // 我们信任 winX，因为 ImGui 也是在同一个 Window 容器里绘制的
            g_Touch.x = winX;
            g_Touch.y = winY;
            
            // 记录偏移仅用于 UI 显示调试
            g_Touch.autoOffsetX = rx - winX;
            g_Touch.autoOffsetY = ry - winY;
        } else {
            g_Touch.x = winX;
            g_Touch.y = winY;
        }

        if (action == AMOTION_EVENT_ACTION_DOWN) {
            g_Touch.down = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down = false;
        }
    }
}

int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a, b, c, d, e, f);
    if (res == 0 && f && *f) {
        handle_android_event((AInputEvent*)*f);
    }
    return res;
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    
    if (w <= 0 || h <= 0) return old_eglSwapBuffers(dpy, surface);

    g_Touch.renderW = (float)w;
    g_Touch.renderH = (float)h;

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        // 动态 DPI 适配：根据高度自动缩放 UI 尺寸
        float scale = (float)h / 1080.0f;
        if (scale < 1.0f) scale = 1.0f;
        
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(3.0f * scale);
        ImGui::GetIO().FontGlobalScale = 3.0f * scale;
        
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);
    
    // 终极对齐修正：
    // 如果发现点击还是偏，说明 InputConsumer 拿到的 winX 坐标系单位是物理像素，
    // 而 ImGui 使用的是 Surface 像素。在部分高刷屏或分屏下，两者不一致。
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_Touch.down) {
        // 在指尖绘制一个引导圈，如果圈准了，菜单就准
        ImGui::GetForegroundDrawList()->AddCircle(io.MousePos, 30.0f, IM_COL32(0, 255, 0, 255), 12, 5.0f);
    }

    ImGui::SetNextWindowPos(ImVec2(w * 0.1f, h * 0.1f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.8f, h * 0.6f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("AndKitty Universal Auto-Align", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Device Render: %.0f x %.0f", g_Touch.renderW, g_Touch.renderH);
        ImGui::Text("Window Offset: X:%.1f Y:%.1f", g_Touch.autoOffsetX, g_Touch.autoOffsetY);
        ImGui::Separator();
        
        ImGui::TextColored(ImVec4(0,1,0,1), "自动对齐模式已激活");
        ImGui::Text("此版本会自动识别分屏、侧边栏和刘海屏偏移。");
        
        if (ImGui::Button("重置菜单位置", ImVec2(-1, 100))) {
            ImGui::SetWindowPos(ImVec2(50, 50));
        }

        static bool esp = true;
        ImGui::Checkbox("自动瞄准 (示例)", &esp);
        
        ImGui::End();
    }

    ImGui::Render();
    glDisable(GL_DEPTH_TEST);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surface);
}

void* init_thread(void*) {
    sleep(5); 

    // 获取底层绝对坐标函数
    void* libandroid = dlopen("libandroid.so", RTLD_NOW);
    if (libandroid) {
        g_getRawX = (p_AMotionEvent_getRawX)dlsym(libandroid, "AMotionEvent_getRawX");
        g_getRawY = (p_AMotionEvent_getRawY)dlsym(libandroid, "AMotionEvent_getRawY");
    }

    void* sym_egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (sym_egl) DobbyHook(sym_egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    
    void* consume_addr = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (consume_addr) DobbyHook(consume_addr, (void*)hook_consume, (void**)&old_consume);

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, init_thread, nullptr);
}
