#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include <stdio.h>
#include <atomic>
#include <thread>
#include <fcntl.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "SimpleHook"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

// 全局变量
static uintptr_t g_ActorManager = 0;
static int g_MemFd = -1;
static float g_touchX = 0, g_touchY = 0;
static std::atomic<bool> g_touchDown{false};

// 安全读取
template<typename T>
T Read(uintptr_t addr) {
    T val{};
    if (addr < 0x10000) return val;
    if (g_MemFd == -1) g_MemFd = open("/proc/self/mem", O_RDONLY);
    if (g_MemFd >= 0) pread(g_MemFd, &val, sizeof(T), addr);
    return val;
}

// 获取模块基址
uintptr_t GetBase(const char* name) {
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return 0;
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, name) && strstr(line, "r-xp")) {
            sscanf(line, "%lx", &base);
            break;
        }
    }
    fclose(f);
    return base;
}

// 原函数只有1个参数，直接 Hook
void (*old_InitActor)(void* x0);
void hook_InitActor(void* x0) {
    LOGI("[HOOK] InitActor x0 = %p", x0);
    g_ActorManager = (uintptr_t)x0;
    old_InitActor(x0);
}

void (*old_ClearActor)(void* x0);
void hook_ClearActor(void* x0) {
    LOGI("[HOOK] ClearActor x0 = %p", x0);
    g_ActorManager = 0;
    old_ClearActor(x0);
}

// 触摸事件处理
void handleTouch(AInputEvent* e) {
    if (AInputEvent_getType(e) == AINPUT_EVENT_TYPE_MOTION) {
        int act = AMotionEvent_getAction(e) & AMOTION_EVENT_ACTION_MASK;
        g_touchX = AMotionEvent_getX(e, 0);
        g_touchY = AMotionEvent_getY(e, 0);
        g_touchDown = (act == AMOTION_EVENT_ACTION_DOWN);
    }
}
typedef int (*Consume_t)(void*,void*,bool,int64_t,uint32_t*,void**);
Consume_t oldConsume = nullptr;
int hookConsume(void* a,void* b,bool c,int64_t d,uint32_t* e,void** f) {
    int r = oldConsume(a,b,c,d,e,f);
    if (r==0 && f && *f) handleTouch((AInputEvent*)*f);
    return r;
}

// eglSwapBuffers Hook
typedef EGLBoolean (*Swap_t)(EGLDisplay,EGLSurface);
Swap_t oldSwap = nullptr;
static bool imguiReady = false;
EGLBoolean hookSwap(EGLDisplay dpy, EGLSurface surf) {
    EGLint w, h;
    eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    if (w > 0 && h > 0) {
        if (!imguiReady) {
            ImGui::CreateContext();
            ImGui_ImplOpenGL3_Init("#version 300 es");
            ImGui::GetIO().Fonts->AddFontDefault();
            imguiReady = true;
        }
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(w, h);
        io.MousePos = ImVec2(g_touchX, g_touchY);
        io.MouseDown[0] = g_touchDown.load();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        // 简单显示
        int hp = 0;
        if (g_ActorManager) {
            uintptr_t addr1 = Read<uintptr_t>(g_ActorManager + 0x20);
            if (addr1) {
                uintptr_t addr2 = Read<uintptr_t>(addr1 + 0x10);
                if (addr2) {
                    uintptr_t item = Read<uintptr_t>(addr2);
                    if (item) {
                        uintptr_t addr3 = Read<uintptr_t>(item + 0x18);
                        if (addr3) {
                            uintptr_t addr4 = Read<uintptr_t>(addr3 + 0x30);
                            if (addr4) hp = Read<int32_t>(addr4 + 0x30);
                        }
                    }
                }
            }
        }

        ImGui::Begin("Hook Test");
        ImGui::Text("ActorManager: 0x%lx", g_ActorManager);
        ImGui::Text("Player HP: %d", hp);
        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, w, h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    return oldSwap(dpy, surf);
}

// 初始化线程
void* InitThread(void*) {
    // 等待 libil2cpp.so 加载
    uintptr_t base;
    while (!(base = GetBase("libil2cpp.so"))) sleep(1);
    uintptr_t initAddr = base + 0x73507bc;
    uintptr_t clearAddr = base + 0x734bc10;
    LOGI("Base=0x%lx, Init=0x%lx, Clear=0x%lx", base, initAddr, clearAddr);
    DobbyHook((void*)initAddr, (void*)hook_InitActor, (void**)&old_InitActor);
    DobbyHook((void*)clearAddr, (void*)hook_ClearActor, (void**)&old_ClearActor);
    // Hook EGL 和 Input
    DobbyHook(DobbySymbolResolver("libEGL.so", "eglSwapBuffers"), (void*)hookSwap, (void**)&oldSwap);
    DobbyHook(DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE"), (void*)hookConsume, (void**)&oldConsume);
    return nullptr;
}

__attribute__((constructor)) void entry() {
    pthread_t t;
    pthread_create(&t, nullptr, InitThread, nullptr);
}
