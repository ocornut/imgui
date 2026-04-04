#include <jni.h>
#include <android/log.h>
#include <android/input.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <string>
#include <unistd.h>

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

// --- 字体加载核心修复 ---
void LoadChineseFont() {
    ImGuiIO& io = ImGui::GetIO();
    
    // 完整的 CJK 字符集范围
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations
        0x4E00, 0x9FAF, // CJK Unified Ideographs (简体中文核心)
        0xFF00, 0xFFEF, // Halfwidth and Fullwidth Forms
        0,
    };

    ImFontConfig config;
    config.SizePixels = 40.0f; // 针对高分辨率屏幕调大
    config.OversampleH = 2;
    config.OversampleV = 2;

    const char* fontPath = "/system/fonts/NotoSansCJK-Regular.ttc";

    if (access(fontPath, F_OK) == 0) {
        // 重要：TTC 是集合文件。Index 2 或 3 通常才是简体中文 (SC)
        // 我们尝试加载 Index 2，如果不行再退回 0
        if (!io.Fonts->AddFontFromFileTTF(fontPath, 40.0f, &config, ranges)) {
             LOGI("JKMenu: 加载索引 2 失败，尝试默认索引");
             io.Fonts->AddFontFromFileTTF(fontPath, 40.0f, &config, ranges);
        }
        LOGI("JKMenu: 成功识别系统字体: %s", fontPath);
    } else {
        LOGI("JKMenu: 路径没找到，请检查 root 权限是否允许读取 /system/fonts");
        io.Fonts->AddFontDefault();
    }
}

// --- 触摸与拖动修复 ---
// 请确保你的 Input Hook 调用了这个函数
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

        // 核心：如果点击在 ImGui 窗口上，返回 true 拦截掉游戏的点击
        // 这样你拖动窗口时，背景的游戏角色才不会跑
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
            
            // 样式美化：圆角和间距适配手指
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 12.0f;
            style.FrameRounding = 8.0f;
            style.TouchExtraPadding = ImVec2(5, 5); // 增加触摸判定区域

            LoadChineseFont();
            ImGui_ImplOpenGL3_Init("#version 300 es");
            
            io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);
            g_Initialized = true;
        }
    }

    if (g_Initialized && g_ShowMenu) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        // 允许拖动且不自动折叠
        ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("JKMenu 菜单 (可拖动)", &g_ShowMenu, ImGuiWindowFlags_NoCollapse)) {
            
            ImGui::Text("系统字体检测: 正常");
            ImGui::Separator();

            static bool esp = false;
            ImGui::Checkbox("开启透视 (ESP)", &esp);

            static float aim_speed = 10.0f;
            ImGui::SliderFloat("自瞄速度", &aim_speed, 1.0f, 100.0f);

            if (ImGui::Button("关闭菜单", ImVec2(-1, 50))) {
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
    sleep(5); // 减少等待时间
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
