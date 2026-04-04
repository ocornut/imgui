#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
#include <atomic>

#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include <GLES3/gl3.h>
#include <EGL/egl.h>

#define LOG_TAG "JKMenu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 存储原始函数
typedef void (*glFlush_t)();
glFlush_t orig_glFlush = nullptr;

bool g_Initialized = false;

// 渲染逻辑
void PerformRender() {
    if (!g_Initialized) {
        LOGI("SUCCESS!!! glFlush Triggered rendering.");
        
        // 获取当前屏幕宽高
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)viewport[2], (float)viewport[3]);
        
        if (ImGui_ImplOpenGL3_Init("#version 300 es")) {
            g_Initialized = true;
            LOGI("ImGui Initialized via glFlush: %d x %d", viewport[2], viewport[3]);
        }
    }

    if (g_Initialized) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        
        ImGui::Begin("JKMenu - Emergency Hook");
        ImGui::Text("If you see this, glFlush Hook works!");
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

// Hook 回调
void hooked_glFlush() {
    PerformRender();
    orig_glFlush();
}

void* hack_thread(void*) {
    LOGI("SO Loaded. Searching for glFlush in libGLESv2...");
    sleep(10);

    // glFlush 是 GLES 必调用的函数，比 SwapBuffers 更底层
    void* target = DobbySymbolResolver("libGLESv2.so", "glFlush");
    if (target) {
        LOGI("Found glFlush at %p, applying hook...", target);
        DobbyHook(target, (dobby_dummy_func_t)hooked_glFlush, (dobby_dummy_func_t*)&orig_glFlush);
    } else {
        LOGI("Failed to find glFlush. This is very strange.");
    }

    return nullptr;
}

void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, hack_thread, nullptr);
}
