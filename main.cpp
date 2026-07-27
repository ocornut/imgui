// 启用 ImVec2 的数学运算符支持
#define IMGUI_DEFINE_MATH_OPERATORS 

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include <thread>       
#include <cmath>       
#include <fstream>      
#include <string>
#include <vector>
#include <chrono>
#include <GLES3/gl3.h>
#include <EGL/egl.h>    
#include <android/log.h>
#include <algorithm>
#include <unistd.h>
#include <jni.h>
#include <mutex>
#include <atomic>       
#include <memory>
#include "dobby.h"
#include <fcntl.h>
#include <stdarg.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ImGui", __VA_ARGS__)

int g_gl_width = 0, g_gl_height = 0;
bool g_menuVisible = true;

// =================================================================
// 字体初始化
// =================================================================
void InitFonts() {
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    
    // 加载安卓自带的中文字体
    const char* fontPath = "/system/fonts/NotoSansCJK-Regular.ttc";
    if (access(fontPath, F_OK) == -1) {
        // 如果上面这个不存在，尝试其它常见的系统中文字体路径
        fontPath = "/system/fonts/DroidSansFallback.ttf";
    }
    
    io.Fonts->Clear();
    if (access(fontPath, F_OK) == 0) {
        io.Fonts->AddFontFromFileTTF(fontPath, 32.0f, &font_cfg, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    } else {
        io.Fonts->AddFontDefault();
    }
    io.Fonts->Build();
}

// =================================================================
// 绘制菜单
// =================================================================
void DrawMenu() {
    if (!g_menuVisible) return;
    
    // 拖动和基本窗口设置
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(u8"IMGUI 注入菜单模板 (支持拖动)", &g_menuVisible, ImGuiWindowFlags_NoSavedSettings)) {
        
        ImGui::Text(u8"你好！这是一个极简的 IMGUI 注入模板。");
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text(u8"已保留功能：");
        ImGui::BulletText(u8"IMGUI菜单界面");
        ImGui::BulletText(u8"窗口自由拖动");
        ImGui::BulletText(u8"中文字体加载");
        ImGui::BulletText(u8"EGL SwapBuffers 注入及基础环境");
        
        ImGui::Spacing();
        if (ImGui::Button(u8"点我测试", ImVec2(120, 40))) {
            LOGI("测试按钮被点击");
        }
        
    }
    ImGui::End();
}

// =================================================================
// 核心渲染循环 EGLSwapBuffers Hook
// =================================================================
EGLBoolean (*old_eglSwap)(EGLDisplay, EGLSurface) = nullptr;

EGLBoolean hook_eglSwap(EGLDisplay dpy, EGLSurface surf) {
    eglQuerySurface(dpy, surf, EGL_WIDTH, &g_gl_width);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &g_gl_height);
    
    if (g_gl_width <= 0 || g_gl_height <= 0) {
        return old_eglSwap(dpy, surf);
    }

    static bool init = false;
    static EGLContext last_ctx = EGL_NO_CONTEXT;
    EGLContext current_ctx = eglGetCurrentContext();
    
    if (current_ctx != last_ctx) {
        if (init) {
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
        }
        last_ctx = current_ctx;
    }

    if (!init) {
        ImGui::CreateContext(); 
        ImGuiIO& io = ImGui::GetIO(); 
        io.IniFilename = nullptr; // 禁用生成 ini 文件
        
        ImGui_ImplOpenGL3_Init("#version 300 es"); 
        
        InitFonts();
        ImGui::StyleColorsDark();
        
        init = true;
    }
    
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)g_gl_width, (float)g_gl_height);
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    
    // 绘制菜单
    DrawMenu();
    
    ImGui::Render();
    glViewport(0, 0, g_gl_width, g_gl_height);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwap(dpy, surf);
}

// =================================================================
// 初始化入口
// =================================================================
void* SetupThread(void*) {
    void* egl_ptr = nullptr;
    
    // 循环等待直到引擎真正加载了 libEGL.so
    while ((egl_ptr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers")) == nullptr) {
        sleep(1); // 每秒检测一次
    }
    
    // Hook eglSwapBuffers
    DobbyHook(egl_ptr, (void*)hook_eglSwap, (void**)&old_eglSwap);
    LOGI("IMGUI Menu Template Injected Successfully.");
    
    return nullptr;
}

__attribute__((constructor)) void Init() { 
    pthread_t t; 
    pthread_create(&t, 0, SetupThread, 0); 
    pthread_detach(t); 
}
