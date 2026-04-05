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
#include <android/input.h>
#include "dobby.h"

// ImGui 包含
#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"

#define TAG "JKMenu_SGame"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// --- 全局变量 ---
bool g_Initialized = false;
bool g_ShowMenu = true;
int g_FrameCount = 0;
pthread_mutex_t g_CtxMutex = PTHREAD_MUTEX_INITIALIZER;
EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = nullptr;

struct TouchData {
    float x, y;
    bool down;
} g_Touch;

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

// --- 增强版 GL 状态保护 ---
struct GLStateBackup {
    GLint last_program, last_texture, last_active_texture, last_array_buffer, last_element_array_buffer;
    GLint last_vertex_array, last_fbo, last_viewport[4], last_scissor_box[4];
    GLboolean last_enable_blend, last_enable_cull_face, last_enable_depth_test, last_enable_scissor_test;

    void Backup() {
        glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_fbo);
        glGetIntegerv(GL_VIEWPORT, last_viewport);
        glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
        last_enable_blend = glIsEnabled(GL_BLEND);
        last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
        last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
        last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
    }

    void Restore() {
        glUseProgram(last_program);
        glBindFramebuffer(GL_FRAMEBUFFER, last_fbo);
        glActiveTexture(last_active_texture);
        glBindTexture(GL_TEXTURE_2D, last_texture);
        glBindVertexArray(last_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
        if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
        glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
        glScissor(last_scissor_box[0], last_scissor_box[1], last_scissor_box[2], last_scissor_box[3]);
    }
};

void DrawImGui(int width, int height) {
    if (!g_ShowMenu) return;

    GLStateBackup gl_state;
    gl_state.Backup();

    // 核心保护：强制切断与游戏原有渲染管线的所有联系
    glBindFramebuffer(GL_FRAMEBUFFER, 0); 
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisable(GL_SCISSOR_TEST); // 防止 ImGui 窗口被游戏的裁剪区域切掉
    for (int i = 0; i < 8; i++) glDisableVertexAttribArray(i);

    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 350), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("JKMenu v4.6 - Android 15 Master", &g_ShowMenu)) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Safe Rendering Enabled");
        ImGui::Text("Frames: %d | FPS: %.1f", g_FrameCount, io.Framerate);
        ImGui::Separator();
        static bool esp = false;
        ImGui::Checkbox("Enemy ESP", &esp);
        if (ImGui::Button("Hide Menu")) g_ShowMenu = false;
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    gl_state.Restore();
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (pthread_mutex_trylock(&g_CtxMutex) != 0) {
        return old_eglSwapBuffers(dpy, surface);
    }
    
    g_FrameCount++;

    EGLint width, height;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

    // 增加延迟，确保游戏已经完全进入稳定渲染循环
    if (!g_Initialized && g_FrameCount > 200 && width > 0 && height > 0) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.DisplaySize = ImVec2((float)width, (float)height);

        if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
            g_Initialized = true;
            LOGI("ImGui Context Initialized (Frame: %d)", g_FrameCount);
        }
    }

    if (g_Initialized && width > 10 && height > 10) {
        ImGui::GetIO().DisplaySize = ImVec2((float)width, (float)height);
        DrawImGui(width, height);
    }

    pthread_mutex_unlock(&g_CtxMutex);
    return old_eglSwapBuffers(dpy, surface);
}

int (*old_AInputEvent_getType)(const AInputEvent* event) = nullptr;
int hook_AInputEvent_getType(const AInputEvent* event) {
    if (!event) return old_AInputEvent_getType(event);
    int type = old_AInputEvent_getType(event);
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        g_Touch.x = AMotionEvent_getRawX(event, 0);
        g_Touch.y = AMotionEvent_getRawY(event, 0);
        int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        if (action == AMOTION_EVENT_ACTION_DOWN) g_Touch.down = true;
        if (action == AMOTION_EVENT_ACTION_UP) g_Touch.down = false;
    }
    return type;
}

void* init_thread(void*) {
    LOGI("Monitoring started...");
    while (!get_module_base("libEGL.so")) usleep(100000);
    
    void* egl_handle = dlopen("libEGL.so", RTLD_LAZY);
    if (egl_handle) {
        void* sym_egl = dlsym(egl_handle, "eglSwapBuffers");
        if (sym_egl) {
            DobbyHook(sym_egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
            LOGI("eglSwapBuffers Hook applied.");
        }
        dlclose(egl_handle);
    }

    void* sym_input = dlsym(RTLD_DEFAULT, "AInputEvent_getType");
    if (sym_input) {
        DobbyHook(sym_input, (void*)hook_AInputEvent_getType, (void**)&old_AInputEvent_getType);
        LOGI("AInputEvent_getType Hook applied.");
    }
    return nullptr;
}

__attribute__((constructor))
void _init() {
    pthread_t tid;
    pthread_create(&tid, nullptr, init_thread, nullptr);
    pthread_detach(tid);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) { return JNI_VERSION_1_6; }
