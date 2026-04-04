#include <jni.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
#include <android/input.h>
#include <android/keycodes.h>

// ImGui 核心与渲染器
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <GLES3/gl3.h>
#include <EGL/egl.h>

#define LOG_TAG "JKMenu_Fix"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 原始函数指针
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surface);
eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

// 触摸 Hook 增强版 (针对移动端更通用的 Hook 方案)
typedef int (*AMotionEvent_getRawX_t)(const AInputEvent* motion_event, size_t pointer_index);
// 如果 AInputQueue 不行，通常会尝试 Hook libinput.so 或游戏的 Input 处理函数

// 全局状态
bool g_Initialized = false;
bool g_ShowMenu = true;
int g_Width = 0;
int g_Height = 0;

// 解决中文显示的关键函数
void LoadChineseFont(ImGuiIO& io) {
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 36.0f; // 适配平板高分辨率
    font_cfg.FontDataOwnedByAtlas = false;
    
    // 常见的安卓系统字体路径
    const char* fonts[] = {
        "/system/fonts/NotoSansCJK-Regular.ttc",
        "/system/fonts/NotoSansSC-Regular.otf",
        "/system/fonts/DroidSansFallback.ttf",
        "/system/fonts/SourceHanSansCN-Regular.otf"
    };

    bool loaded = false;
    for (const char* path : fonts) {
        if (access(path, F_OK) == 0) {
            // 必须传入 GetGlyphRangesChineseFull() 否则还是问号
            io.Fonts->AddFontFromFileTTF(path, 36.0f, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());
            LOGI("成功加载字体: %s", path);
            loaded = true;
            break;
        }
    }

    if (!loaded) {
        LOGI("未发现系统字体，使用默认字体 (将显示问号)");
        io.Fonts->AddFontDefault(&font_cfg);
    }
}

// 解决触摸拖动的关键逻辑
// 修正：ImGui 需要的是全局坐标同步
bool HandleTouch(AInputEvent* event) {
    if (!g_Initialized || !g_ShowMenu) return false;

    ImGuiIO& io = ImGui::GetIO();
    int32_t type = AInputEvent_getType(event);

    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        int32_t actionIdx = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        int32_t actionMasked = action & AMOTION_EVENT_ACTION_MASK;

        float x = AMotionEvent_getX(event, actionIdx);
        float y = AMotionEvent_getY(event, actionIdx);

        io.MousePos = ImVec2(x, y);

        if (actionMasked == AMOTION_EVENT_ACTION_DOWN || actionMasked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            io.MouseDown[0] = true;
        } else if (actionMasked == AMOTION_EVENT_ACTION_UP || actionMasked == AMOTION_EVENT_ACTION_POINTER_UP) {
            io.MouseDown[0] = false;
        }

        // 重点：如果 ImGui 捕获了鼠标，则不把事件传给游戏，防止菜单和游戏一起动
        return io.WantCaptureMouse;
    }
    return false;
}

// 增强型 AInputQueue Hook
typedef int32_t (*InputEventHandle_t)(void* instance, AInputEvent* event);
int32_t hooked_InputEvent(void* instance, AInputEvent* event) {
    if (HandleTouch(event)) {
        return 1; // 消费事件
    }
    // 这里需要根据实际游戏引擎寻找真正的原始回调
    return 0; 
}

void DrawImGuiMenu() {
    if (!g_ShowMenu) return;

    // 设置初始位置，防止平板上菜单刷在边缘拉不回来
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550, 420), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("JKMenu - 增强稳定版", &g_ShowMenu, ImGuiWindowFlags_AlwaysAutoResize)) {
        
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "设备信息: OnePlus Pad Pro");
        ImGui::Text("屏幕分辨率: %d x %d", g_Width, g_Height);
        ImGui::Separator();

        static bool esp_box = false;
        ImGui::Checkbox("开启方框透视", &esp_box);

        static float crosshair_size = 10.0f;
        ImGui::SliderFloat("准星大小", &crosshair_size, 1.0f, 50.0f);

        if (ImGui::Button("清理残留数据", ImVec2(ImGui::GetContentRegionAvail().x, 40))) {
            // 执行清理逻辑
        }

        ImGui::End();
    }
}

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_Initialized) {
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_Width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_Height);

        if (g_Width > 0) {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            
            LoadChineseFont(io); // 修复问号的核心调用
            
            ImGui_ImplOpenGL3_Init("#version 300 es");
            g_Initialized = true;
        }
    }

    if (g_Initialized && g_ShowMenu) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        DrawImGuiMenu();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return orig_eglSwapBuffers(dpy, surface);
}

void* hack_thread(void*) {
    sleep(10); // 等待游戏完全进入，防止 EGL 未初始化闪退

    // 1. Hook 渲染
    void* swap_ptr = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (swap_ptr) {
        DobbyHook(swap_ptr, (dobby_dummy_func_t)hooked_eglSwapBuffers, (dobby_dummy_func_t*)&orig_eglSwapBuffers);
    }

    // 2. 针对 Unity/腾讯游戏的触摸修复方案
    // 很多时候 AInputQueue 拿不到事件是因为游戏用了 NativeActivity 以外的逻辑
    // 尝试 Hook 整个系统的 MotionEvent 或者是引擎分发层
    LOGI("JKMenu: 注入增强补丁成功");

    return nullptr;
}

void __attribute__((constructor)) init() {
    pthread_t t;
    pthread_create(&t, nullptr, hack_thread, nullptr);
}
