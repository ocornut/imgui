#include <jni.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
#include <android/input.h>
#include <android/keycodes.h>

// ImGui 核心与渲染器
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <GLES3/gl3.h>
#include <EGL/egl.h>

#define LOG_TAG "JKMenu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 原始函数指针
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surface);
eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

// 触摸 Hook 相关
typedef int (*AInputQueue_getEvent_t)(AInputQueue* queue, AInputEvent** out_event);
AInputQueue_getEvent_t orig_AInputQueue_getEvent = nullptr;

// 全局状态
bool g_Initialized = false;
bool g_ShowMenu = true;
int g_Width = 0;
int g_Height = 0;

// 处理触摸事件逻辑
bool HandleAndroidInput(AInputEvent* event) {
    if (!g_Initialized || !g_ShowMenu) return false;

    ImGuiIO& io = ImGui::GetIO();
    int32_t eventType = AInputEvent_getType(event);

    if (eventType == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);

        io.MousePos = ImVec2(x, y);

        if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            io.MouseDown[0] = true;
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP) {
            io.MouseDown[0] = false;
        }

        // 如果点击在 ImGui 窗口范围内，返回 true 表示消费掉这个事件，不传给游戏
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemHovered()) {
            return true; 
        }
    }
    return false;
}

// Hook 触摸队列获取
int hooked_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** out_event) {
    int res = orig_AInputQueue_getEvent(queue, out_event);
    if (res >= 0 && out_event != nullptr && *out_event != nullptr) {
        // 如果 ImGui 消费了事件，我们可以尝试处理，但 Native Hook 通常建议只做同步
        HandleAndroidInput(*out_event);
    }
    return res;
}

// 菜单绘制内容
void DrawImGuiMenu() {
    if (!g_ShowMenu) return;

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    // 使用 UTF-8 编码的中文标题
    ImGui::Begin("JKMenu - 腾讯游戏通用菜单", &g_ShowMenu);

    ImGui::Text("设备: OnePlus Pad Pro 2");
    ImGui::Text("状态: 注入成功 (OpenGL 模式)");
    ImGui::Text("分辨率: %dx%d", g_Width, g_Height);
    ImGui::Separator();

    static bool feature_esp = false;
    ImGui::Checkbox("显示射线 (ESP)", &feature_esp);
    
    static float menu_scale = 1.0f;
    if (ImGui::SliderFloat("菜单缩放", &menu_scale, 0.5f, 2.0f)) {
        ImGui::GetIO().FontGlobalScale = 1.0f * menu_scale;
    }

    if (ImGui::Button("完全关闭菜单")) {
        g_ShowMenu = false;
    }

    ImGui::End();
}

// 核心渲染 Hook
EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_Height);

        if (g_Width > 0 && g_Height > 0) {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            
            // --- 解决中文乱码的关键 ---
            ImFontConfig font_cfg;
            font_cfg.SizePixels = 32.0f; // 平板分辨率高，初始字体大一点
            
            // 尝试加载安卓系统自带的中文字体路径
            const char* font_path = "/system/fonts/NotoSansCJK-Regular.ttc";
            if (access(font_path, F_OK) == -1) {
                font_path = "/system/fonts/DroidSansFallback.ttf"; // 兼容旧版
            }

            if (access(font_path, F_OK) != -1) {
                io.Fonts->AddFontFromFileTTF(font_path, 32.0f, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());
            } else {
                io.Fonts->AddFontDefault(&font_cfg);
            }
            
            io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);
            io.IniFilename = nullptr; // 不保存配置文件

            if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
                g_Initialized = true;
                LOGI("ImGui Render + Font Initialized!");
            }
        }
    }

    if (g_Initialized) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        DrawImGuiMenu();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return orig_eglSwapBuffers(dpy, surface);
}

void* hack_thread(void*) {
    LOGI("JKMenu: Starting Deep Hooking...");
    sleep(12);

    // 1. Hook 渲染 (eglSwapBuffers)
    void* swap_ptr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (swap_ptr) {
        DobbyHook(swap_ptr, (dobby_dummy_func_t)hooked_eglSwapBuffers, (dobby_dummy_func_t*)&orig_eglSwapBuffers);
    }

    // 2. Hook 触摸 (AInputQueue_getEvent)
    // 这是 Android NDK 处理输入最标准的地方
    void* get_event_ptr = DobbySymbolResolver("libandroid.so", "AInputQueue_getEvent");
    if (get_event_ptr) {
        LOGI("Found AInputQueue_getEvent, hooking touch...");
        DobbyHook(get_event_ptr, (dobby_dummy_func_t)hooked_AInputQueue_getEvent, (dobby_dummy_func_t*)&orig_AInputQueue_getEvent);
    }

    return nullptr;
}

void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, hack_thread, nullptr);
}
