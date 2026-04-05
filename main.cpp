#include <jni.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>

#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "dobby.h"

#define KITTY_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "AndKitty", __VA_ARGS__)

// --- 全局状态 ---
static bool g_Initialized = false;
static bool g_ShowMenu = true;
static int g_Width = 0;
static int g_Height = 0;

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surf);
static eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

// 渲染主函数
void DrawImGuiMenu() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(g_Width / 3.0f, g_Height / 2.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("AndKitty Mod Menu", &g_ShowMenu)) {
            ImGui::Text("Package: com.tencent.tmgp.sgame");
            ImGui::Text("Display: %d x %d", g_Width, g_Height);
            ImGui::Separator();
            
            static bool example_feature = false;
            ImGui::Checkbox("Feature Test", &example_feature);
            
            if (ImGui::Button("Hide Menu")) {
                g_ShowMenu = false;
            }
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// 核心 Hook 函数
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    // 状态保护：保存当前 GL 绑定，防止 ImGui 渲染干扰游戏逻辑
    GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_buffer; glGetIntegerv(GL_ARRAY_BUFFER, &last_buffer);

    if (!g_Initialized) {
        eglQuerySurface(dpy, surf, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surf, EGL_HEIGHT, &g_Height);

        if (g_Width > 0 && g_Height > 0) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2((float)g_Width, (float)g_Height);

            ImGui_ImplAndroid_Init(nullptr);
            ImGui_ImplOpenGL3_Init("#version 300 es");
            ImGui::StyleColorsDark();
            
            // 根据高度自动缩放 UI
            ImGui::GetStyle().ScaleAllSizes(g_Height / 1080.0f);
            
            g_Initialized = true;
            KITTY_LOGI("ImGui Initialized: %dx%d", g_Width, g_Height);
        }
    }

    if (g_Initialized && g_ShowMenu) {
        DrawImGuiMenu();
    }

    // 恢复状态
    glUseProgram(last_program);
    glBindBuffer(GL_ARRAY_BUFFER, last_buffer);

    return orig_eglSwapBuffers(dpy, surf);
}

// 实际的线程函数
void thread_function() {
    KITTY_LOGI("Thread started, waiting for EGL...");
    // 增加等待时间至 15 秒，确保游戏环境自检完成
    sleep(15); 
    
    void* handle = dlopen("libEGL.so", RTLD_LAZY);
    if (handle) {
        void* target = dlsym(handle, "eglSwapBuffers");
        if (target) {
            DobbyHook(target, (dobby_dummy_func_t)hook_eglSwapBuffers, (dobby_dummy_func_t*)&orig_eglSwapBuffers);
            KITTY_LOGI("eglSwapBuffers Hooked Successfully!");
        }
        dlclose(handle);
    }
}

// 注入器专用入口
extern "C" jint JNIEXPORT JNI_OnLoad(JavaVM* vm, void *key)
{
    // 严格校验注入器 Key
    if (key != (void*)1337)
        return JNI_VERSION_1_6;

    KITTY_LOGI("JNI_OnLoad called by injector.");

    JNIEnv *env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK)
    {
        KITTY_LOGI("JavaEnv acquired: %p.", env);
    }
    
    // 按照文档推荐方式启动线程
    std::thread(thread_function).detach();
    
    return JNI_VERSION_1_6;
}
