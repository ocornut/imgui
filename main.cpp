#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_android.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <dobby.h>
#include <dlfcn.h>
#include "memory_utils.h"

// --- 全局变量与原始函数指针 ---
using T_eglSwapBuffers = EGLBoolean (*)(EGLDisplay, EGLSurface);
T_eglSwapBuffers orig_eglSwapBuffers = nullptr;

bool g_Initialized = false;
bool g_ShowMenu = true;

// --- 渲染状态备份 (王者荣耀专项优化：深度还原) ---
void Hook_eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
    if (!g_Initialized) {
        // 1. 初始化 ImGui 上下文
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr; // 不保存配置文件，防止被游戏扫描到
        
        // 2. 初始化后台渲染器 (王者荣耀使用 GLES3)
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        g_Initialized = true;
    }

    if (g_ShowMenu) {
        // --- [核心：全状态备份] 学习自高级 Hook 逻辑 ---
        GLint last_program, last_vertex_array, last_array_buffer, last_element_array_buffer;
        GLint last_viewport[4];
        GLint last_scissor_box[4];
        
        glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
        glGetIntegerv(GL_VIEWPORT, last_viewport);
        glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);

        // 备份关键能力开关
        GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
        GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
        GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
        GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

        // --- 开始 ImGui 帧 ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        // 绘制王者荣耀专属风格菜单
        ImGui::SetNextWindowSize(ImVec2(400, 320), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("JKInternal - SGame (王者荣耀)", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "渲染注入成功: libEGL.so -> eglSwapBuffers");
            ImGui::Separator();
            
            static bool esp_box = false;
            static bool esp_line = false;
            ImGui::Checkbox("英雄方框", &esp_box);
            ImGui::SameLine();
            ImGui::Checkbox("英雄射线", &esp_line);

            static float draw_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
            ImGui::ColorEdit4("绘制颜色", draw_color);

            ImGui::Separator();
            if (ImGui::Button("隐藏菜单 (按音量键恢复-未实现)")) {
                g_ShowMenu = false;
            }
        }
        ImGui::End();

        // --- 渲染 ImGui ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // --- [核心：全状态还原] 让游戏管线无感知 ---
        glUseProgram(last_program);
        glBindVertexArray(last_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
        glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
        glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);

        if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    }

    // 调用原始函数指针，完成屏幕交换
    orig_eglSwapBuffers(display, surface);
}

// --- 注入入口 ---
__attribute__((constructor))
void MainEntry() {
    // 动态获取 eglSwapBuffers 地址并进行 Dobby Hook
    void* handle = dlopen("libEGL.so", RTLD_LAZY);
    if (handle) {
        void* target = dlsym(handle, "eglSwapBuffers");
        if (target) {
            DobbyHook(target, (void*)Hook_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
        }
    }
}
