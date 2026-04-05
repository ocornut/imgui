#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "dobby.h"

// ImGui 核心与 OpenGL3 后端
#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"

#define TAG "JKMenu_TRACE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// --- 全局变量 ---
bool g_Initialized = false;
bool g_ShowMenu = true;
int g_FrameCount = 0;
EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = nullptr;

// 获取模块基址
uintptr_t get_module_base(const char* name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    char line[1024];
    uintptr_t base = 0;
    while (fp && fgets(line, sizeof(line), fp)) {
        if (strstr(line, name)) {
            base = (uintptr_t)strtoull(line, NULL, 16);
            break;
        }
    }
    if (fp) fclose(fp);
    return base;
}

// --- 渲染保护与 UI 绘制 ---
void DrawImGui(int width, int height) {
    if (!g_ShowMenu) return;

    // 1. 保存当前所有可能被 ImGui 改变的 GL 状态
    GLint last_program, last_texture, last_array_buffer, last_vao;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vao);
    
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // 2. ImGui 渲染帧启动
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("JKMenu Render Test", &g_ShowMenu)) {
        ImGui::Text("Render Status: Active");
        ImGui::Text("Wait for Input Implementation...");
        ImGui::Separator();
        
        static float f = 0.5f;
        ImGui::SliderFloat("Alpha Test", &f, 0.0f, 1.0f);
        
        if (ImGui::Button("Close Menu")) {
            g_ShowMenu = false;
        }
    }
    ImGui::End();

    // 3. 提交绘制
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 4. 恢复原始状态 (极其重要，防止游戏闪退)
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindVertexArray(last_vao);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
    
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
}

// --- EGL SwapBuffers Hook ---
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    // 简单的重入锁
    static bool is_rendering = false;
    if (is_rendering) return old_eglSwapBuffers(dpy, surface);
    is_rendering = true;

    g_FrameCount++;

    // 初始化逻辑：等待 200 帧确保 Context 完全稳定
    if (!g_Initialized && g_FrameCount > 200) {
        if (eglGetCurrentContext() != EGL_NO_CONTEXT) {
            LOGI("[TRACE] Step 3: Starting ImGui Render Init");
            
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr;
            
            // 解决高分屏 UI 太小的问题
            io.FontGlobalScale = 2.0f;

            if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
                g_Initialized = true;
                LOGI("[TRACE] Step 4: Render Init Success");
            }
        }
    }

    if (g_Initialized && g_ShowMenu) {
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        
        ImGui::GetIO().DisplaySize = ImVec2((float)w, (float)h);
        DrawImGui(w, h);
    }

    is_rendering = false;
    return old_eglSwapBuffers(dpy, surface);
}

// --- 初始化线程 ---
void* init_thread(void*) {
    LOGI("[TRACE] Step 1: Plugin Thread Started");
    
    // 等待核心图形库加载
    while (!get_module_base("libEGL.so") || !get_module_base("libGLESv3.so")) {
        usleep(100000);
    }
    
    // 仅 Hook 渲染函数
    void* sym_egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (sym_egl) {
        DobbyHook(sym_egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        LOGI("[TRACE] Step 2: eglSwapBuffers Hooked (Render Only)");
    }

    return nullptr;
}

__attribute__((constructor)) void _init() {
    pthread_t tid;
    pthread_create(&tid, nullptr, init_thread, nullptr);
    pthread_detach(tid);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    return JNI_VERSION_1_6;
}
