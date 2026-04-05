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

// --- 安全的 GL 状态保护 ---
struct GLStateBackup {
    GLint last_program;
    GLint last_texture;
    GLint last_active_texture;
    GLint last_array_buffer;
    GLint last_element_array_buffer;
    GLint last_vertex_array;
    GLint last_viewport[4];
    GLboolean last_enable_blend;
    GLboolean last_enable_cull_face;
    GLboolean last_enable_depth_test;
    GLboolean last_enable_scissor_test;

    void Backup() {
        glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
        glGetIntegerv(GL_VIEWPORT, last_viewport);
        last_enable_blend = glIsEnabled(GL_BLEND);
        last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
        last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
        last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
    }

    void Restore() {
        glUseProgram(last_program);
        glActiveTexture(last_active_texture);
        glBindTexture(GL_TEXTURE_2D, last_texture);
        glBindVertexArray(last_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
        glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
        if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    }
};

void DrawImGui(int width, int height) {
    if (!g_ShowMenu) return;

    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down;

    // 状态保护开始
    GLStateBackup gl_state;
    gl_state.Backup();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("JKMenu v3.9 - Android 15 Fix", &g_ShowMenu)) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Safe Render Active");
        ImGui::Text("Res: %dx%d | Touch: %.1f,%.1f", width, height, g_Touch.x, g_Touch.y);
        ImGui::Separator();
        
        static bool esp = false;
        ImGui::Checkbox("ESP Overlay", &esp);
        static float f = 0.0f;
        ImGui::SliderFloat("FOV", &f, 0.0f, 180.0f);

        if (ImGui::Button("Hide")) g_ShowMenu = false;
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 状态保护结束
    gl_state.Restore();
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    pthread_mutex_lock(&g_CtxMutex);
    
    EGLint width, height;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);

    if (!g_Initialized && width > 0) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.DisplaySize = ImVec2((float)width, (float)height);

        if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
            g_Initialized = true;
            LOGI("Safe ImGui Init Success.");
        }
    }

    if (g_Initialized && width > 0 && height > 0) {
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
    while (!get_module_base("libEGL.so")) sleep(1);

    void* sym_egl = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (sym_egl) DobbyHook(sym_egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    void* sym_input = dlsym(RTLD_DEFAULT, "AInputEvent_getType");
    if (sym_input) DobbyHook(sym_input, (void*)hook_AInputEvent_getType, (void**)&old_AInputEvent_getType);

    return nullptr;
}

__attribute__((constructor))
void _init() {
    pthread_t tid;
    pthread_create(&tid, nullptr, init_thread, nullptr);
    pthread_detach(tid);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) { return JNI_VERSION_1_6; }
