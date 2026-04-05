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

#define TAG "JKMenu_TRACE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// --- 全局变量 ---
bool g_Initialized = false;
bool g_ShowMenu = true;
int g_FrameCount = 0;
pthread_mutex_t g_CtxMutex = PTHREAD_MUTEX_INITIALIZER;
EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = nullptr;

struct TouchData { float x, y; bool down; } g_Touch;

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

void DrawImGui(int width, int height) {
    if (!g_ShowMenu) return;

    // STEP 4.1: 进入绘制逻辑
    LOGI("[TRACE] Step 4.1: Enter DrawImGui");

    // 保存状态
    GLint last_fbo, last_vao, last_program;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_fbo);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vao);
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);

    // STEP 4.2: 状态解绑
    glBindFramebuffer(GL_FRAMEBUFFER, 0); 
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    for (int i = 0; i < 8; i++) glDisableVertexAttribArray(i);
    LOGI("[TRACE] Step 4.2: State Unbind Success");

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Debug Trace Menu", &g_ShowMenu)) {
        ImGui::Text("If you see this, Step 4.3 is OK");
        if (ImGui::Button("Test Crash Point")) { LOGI("Button Clicked"); }
    }
    ImGui::End();

    // STEP 4.3: 生成绘制数据
    ImGui::Render();
    LOGI("[TRACE] Step 4.3: ImGui Render Data Generated");

    // STEP 4.4: 提交 GPU 绘制 (最容易闪退的点)
    LOGI("[TRACE] Step 4.4: Executing glDraw...");
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    LOGI("[TRACE] Step 4.5: glDraw Finished");

    // 还原状态
    glBindFramebuffer(GL_FRAMEBUFFER, last_fbo);
    glBindVertexArray(last_vao);
    glUseProgram(last_program);
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (pthread_mutex_trylock(&g_CtxMutex) != 0) return old_eglSwapBuffers(dpy, surface);
    
    g_FrameCount++;

    // STEP 3: 检测初始化时机
    if (!g_Initialized && g_FrameCount > 150) {
        LOGI("[TRACE] Step 3.1: Start ImGui Init");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        io.DisplaySize = ImVec2((float)w, (float)h);

        if (ImGui_ImplOpenGL3_Init("#version 100")) {
            g_Initialized = true;
            LOGI("[TRACE] Step 3.2: ImGui Init Success");
        } else {
            LOGE("[TRACE] Step 3.2 ERROR: ImGui Init Failed");
        }
    }

    if (g_Initialized) {
        EGLint w, h;
        eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
        DrawImGui(w, h);
    }

    pthread_mutex_unlock(&g_CtxMutex);
    return old_eglSwapBuffers(dpy, surface);
}

// 输入处理 (Step 5)
int (*old_AInputEvent_getType)(const AInputEvent* event) = nullptr;
int hook_AInputEvent_getType(const AInputEvent* event) {
    int type = old_AInputEvent_getType(event);
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        g_Touch.x = AMotionEvent_getRawX(event, 0);
        g_Touch.y = AMotionEvent_getRawY(event, 0);
        int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        g_Touch.down = (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_MOVE);
    }
    return type;
}

void* init_thread(void*) {
    LOGI("[TRACE] Step 1: Thread Started");
    while (!get_module_base("libEGL.so")) usleep(100000);
    
    void* egl_handle = dlopen("libEGL.so", RTLD_LAZY);
    if (egl_handle) {
        void* sym = dlsym(egl_handle, "eglSwapBuffers");
        if (sym) {
            DobbyHook(sym, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
            LOGI("[TRACE] Step 2.1: eglSwapBuffers Hooked");
        }
        dlclose(egl_handle);
    }

    void* sym_in = dlsym(RTLD_DEFAULT, "AInputEvent_getType");
    if (sym_in) {
        DobbyHook(sym_in, (void*)hook_AInputEvent_getType, (void**)&old_AInputEvent_getType);
        LOGI("[TRACE] Step 2.2: Input Hooked");
    }
    return nullptr;
}

__attribute__((constructor)) void _init() {
    pthread_t tid;
    pthread_create(&tid, nullptr, init_thread, nullptr);
    pthread_detach(tid);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) { return JNI_VERSION_1_6; }
