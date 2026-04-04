#include <jni.h>
#include <android/log.h>
#include <android/input.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <string>
#include <unistd.h>
#include <pthread.h>

#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "dobby.h"

#define LOG_TAG "JK_DEBUG"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static bool g_Initialized = false;
static bool g_ShowMenu = true;
static int g_Width = 0, g_Height = 0;

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surface);
static eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

// --- 字体加载函数 ---
void LoadChineseFont() {
    ImGuiIO& io = ImGui::GetIO();
    
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols
        0x4E00, 0x9FAF, // CJK Unified Ideographs
        0xFF00, 0xFFEF, // Halfwidth/Fullwidth
        0,
    };

    ImFontConfig config;
    config.SizePixels = 40.0f;
    config.OversampleH = 2;
    config.OversampleV = 2;

    const char* fontPath = "/system/fonts/NotoSansCJK-Regular.ttc";

    if (access(fontPath, R_OK) == 0) {
        // 尝试加载索引 2 (通常是简体中文)
        if (!io.Fonts->AddFontFromFileTTF(fontPath, 40.0f, &config, ranges)) {
             LOGI("JKMenu: 索引 2 加载失败，回退到默认索引");
             io.Fonts->AddFontFromFileTTF(fontPath, 40.0f, &config, ranges);
        }
        LOGI("JKMenu: 字体加载路径: %s", fontPath);
    } else {
        LOGI("JKMenu: 无法访问系统字体，使用默认字体");
        io.Fonts->AddFontDefault();
    }
}

// --- 输入拦截 ---
bool HandleInput(AInputEvent* event) {
    if (!g_Initialized || !g_ShowMenu) return false;

    ImGuiIO& io = ImGui::GetIO();
    int32_t type = AInputEvent_getType(event);

    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);

        io.MousePos = ImVec2(x, y);

        if (action == AMOTION_EVENT_ACTION_DOWN) io.MouseDown[0] = true;
        else if (action == AMOTION_EVENT_ACTION_UP) io.MouseDown[0] = false;

        // 如果点击在菜单窗口上，拦截掉，不传递给游戏
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemActive()) {
            return true; 
        }
    }
    return false;
}

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_Height);

        if (g_Width > 0 && g_Height > 0) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); // 修复点：在这里获取 io 引用

            // 样式调整
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 10.0f;
            style.FrameRounding = 6.0f;
            style.ScaleAllSizes(2.0f); // 针对高分辨率屏幕整体放大 UI

            LoadChineseFont();
            ImGui_ImplOpenGL3_Init("#version 300 es");
            
            io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);
            g_Initialized = true;
        }
    }

    if (g_Initialized && g_ShowMenu) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("JKMenu 控制面板", &g_ShowMenu)) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "状态: 运行中 (绘制正常)");
            ImGui::Separator();

            static bool esp = false;
            ImGui::Checkbox("玩家透视", &esp);

            static float radius = 150.0f;
            ImGui::SliderFloat("自瞄范围", &radius, 50.0f, 500.0f);

            if (ImGui::Button("退出菜单", ImVec2(-1, 60))) {
                g_ShowMenu = false;
            }
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return orig_eglSwapBuffers(dpy, surface);
}

void* main_thread(void*) {
    sleep(5);
    void* swap_ptr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (swap_ptr) {
        DobbyHook(swap_ptr, (dobby_dummy_func_t)hooked_eglSwapBuffers, (dobby_dummy_func_t*)&orig_eglSwapBuffers);
    }
    return nullptr;
}

void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, main_thread, nullptr);
}
