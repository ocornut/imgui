// ============================================================
// 精简版：仅保留 ImGui 菜单窗口，无任何游戏功能
// 可用于测试注入后是否稳定，定位是否为功能模块导致闪退
// 编译：aarch64-linux-android-clang++ -shared -fPIC -O2 menu_only.cpp -o libMyMenu.so \
//       -I./imgui -I./imgui/backends -lGLESv3 -lEGL -llog -ldl
// ============================================================

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include <thread>
#include <chrono>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <unistd.h>
#include <mutex>
#include <atomic>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

// Dobby 用于 Hook eglSwapBuffers（若未使用 Dobby 可替换为其他 Hook 框架）
#include "dobby.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "JKMenu", __VA_ARGS__)

// ------------------------------------------------------------
// 全局变量
// ------------------------------------------------------------
int g_gl_width = 0, g_gl_height = 0;
float g_autoScale = 1.0f;
float g_menuX = 100.0f, g_menuY = 100.0f, g_menuW = 500.0f, g_menuH = 650.0f;
bool g_menuCollapsed = false;
bool g_needUpdateFontSafe = false;
ImFont* g_mainFont = nullptr;
float g_current_rendered_size = 0.0f;

// ------------------------------------------------------------
// 字体更新（自适应屏幕）
// ------------------------------------------------------------
void UpdateFontHD(bool force = false) {
    ImGuiIO& io = ImGui::GetIO();
    float screenH = (io.DisplaySize.y > 100.0f) ? io.DisplaySize.y : 2400.0f;
    g_autoScale = screenH / 1080.0f;
    float targetSize = std::clamp(20.0f * g_autoScale, 16.0f, 45.0f);
    if (!force && std::abs(targetSize - g_current_rendered_size) < 2.0f) return;

    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    io.Fonts->Clear();
    g_mainFont = nullptr;

    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 2;
    config.PixelSnapH = false;

    // 尝试加载系统字体（支持中文）
    const char* fontPaths[] = {
        "/system/fonts/Miui-Regular.ttf",
        "/system/fonts/SysSans-Hans-Regular.ttf",
        "/system/fonts/NotoSansCJK-Regular.ttc",
        "/system/fonts/DroidSansFallback.ttf",
        "/system/fonts/Roboto-Regular.ttf"
    };
    for (const char* path : fontPaths) {
        if (access(path, R_OK) == 0) {
            g_mainFont = io.Fonts->AddFontFromFileTTF(
                path, targetSize, &config, io.Fonts->GetGlyphRangesChineseSimplifiedCommon()
            );
            if (g_mainFont) break;
        }
    }
    if (!g_mainFont) {
        g_mainFont = io.Fonts->AddFontDefault();
    }

    io.Fonts->Build();
    ImGui_ImplOpenGL3_CreateDeviceObjects();
    g_current_rendered_size = targetSize;
}

// ------------------------------------------------------------
// 菜单绘制
// ------------------------------------------------------------
void DrawMenu() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    // 样式
    style.WindowRounding = 16.0f * g_autoScale;
    style.FrameRounding = 8.0f * g_autoScale;
    style.WindowPadding = ImVec2(16.0f * g_autoScale, 16.0f * g_autoScale);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.11f, 0.92f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.09f, 0.11f, 0.95f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.13f, 0.15f, 0.95f);

    static bool firstOpen = true;
    if (firstOpen) {
        ImGui::SetNextWindowPos(ImVec2(g_menuX, g_menuY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(g_menuW, g_menuH), ImGuiCond_Always);
        ImGui::SetNextWindowCollapsed(g_menuCollapsed, ImGuiCond_Always);
        firstOpen = false;
    }

    if (ImGui::Begin("金铲铲助手", nullptr, ImGuiWindowFlags_NoSavedSettings)) {
        if (!ImGui::IsWindowCollapsed()) {
            // 记录窗口状态
            g_menuX = ImGui::GetWindowPos().x;
            g_menuY = ImGui::GetWindowPos().y;
            g_menuW = ImGui::GetWindowSize().x;
            g_menuH = ImGui::GetWindowSize().y;
            g_menuCollapsed = false;

            ImGui::TextColored(ImVec4(0.0f, 0.85f, 0.55f, 1.0f), "精简测试菜单 (无功能)");
            ImGui::Separator();
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("屏幕分辨率: %dx%d", g_gl_width, g_gl_height);

            if (ImGui::Button("退出菜单")) {
                // 可在此添加卸载逻辑（例如 dlclose 自身）
                LOGI("Exit button clicked");
            }

            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "若此菜单稳定不闪退，说明注入框架正常");
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "功能已全部移除，可在此基础上逐步加回功能定位崩溃");
        } else {
            g_menuCollapsed = true;
        }
    }
    ImGui::End();
}

// ------------------------------------------------------------
// eglSwapBuffers Hook（注入 ImGui 渲染）
// ------------------------------------------------------------
EGLBoolean (*old_eglSwap)(EGLDisplay, EGLSurface) = nullptr;
std::atomic<bool> g_engine_rendering{false};

EGLBoolean hook_eglSwap(EGLDisplay dpy, EGLSurface surf) {
    if (!g_engine_rendering.load()) {
        g_engine_rendering.store(true);
    }

    // 获取窗口尺寸
    eglQuerySurface(dpy, surf, EGL_WIDTH, &g_gl_width);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &g_gl_height);
    if (g_gl_width <= 0 || g_gl_height <= 0) {
        return old_eglSwap(dpy, surf);
    }

    // ---------- ImGui 初始化 ----------
    static bool init = false;
    static auto last_time = std::chrono::high_resolution_clock::now();

    // 当 OpenGL 上下文变化时重建 ImGui
    static EGLContext last_ctx = EGL_NO_CONTEXT;
    EGLContext current_ctx = eglGetCurrentContext();
    if (current_ctx != last_ctx) {
        if (init) {
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            g_needUpdateFontSafe = true;
        }
        last_ctx = current_ctx;
    }

    if (!init) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr; // 不保存 imgui.ini
        ImGui_ImplOpenGL3_Init("#version 300 es");
        io.DisplaySize = ImVec2((float)g_gl_width, (float)g_gl_height);
        UpdateFontHD(true);
        init = true;
    }
    if (g_needUpdateFontSafe) {
        UpdateFontHD(true);
        g_needUpdateFontSafe = false;
    }

    // ---------- 更新 ImGui 时间 ----------
    auto now = std::chrono::high_resolution_clock::now();
    float delta = std::chrono::duration<float>(now - last_time).count();
    last_time = now;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)g_gl_width, (float)g_gl_height);
    io.DeltaTime = (delta > 0.0001f) ? delta : (1.0f / 120.0f);

    // ---------- 绘制 ----------
    ProcessTextureQueue? 无功能，无需调用

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    glDisable(GL_SCISSOR_TEST);

    // 仅绘制菜单
    DrawMenu();

    ImGui::Render();
    glViewport(0, 0, g_gl_width, g_gl_height);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // ---------- 调用原始 eglSwapBuffers ----------
    return old_eglSwap(dpy, surf);
}

// ------------------------------------------------------------
// 初始化线程（延迟 Hook eglSwapBuffers）
// ------------------------------------------------------------
void* SetupThread(void*) {
    // 等待 libEGL.so 加载并获取 eglSwapBuffers 地址
    void* egl_ptr = nullptr;
    while ((egl_ptr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers")) == nullptr) {
        sleep(1);
    }
    LOGI("eglSwapBuffers found at %p, hooking...", egl_ptr);
    DobbyHook(egl_ptr, (void*)hook_eglSwap, (void**)&old_eglSwap);
    LOGI("Hook installed.");
    return nullptr;
}

// ------------------------------------------------------------
// SO 加载入口
// ------------------------------------------------------------
__attribute__((constructor)) void Init() {
    LOGI("Menu-only SO loaded.");
    pthread_t t;
    pthread_create(&t, 0, SetupThread, 0);
    pthread_detach(t);
}
