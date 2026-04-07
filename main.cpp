#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include <stdio.h>
#include <algorithm>
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <dirent.h>
#include <iomanip>
#include <sstream>
#include <link.h>
#include <sys/mman.h>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "IL2CPP_Drive"
#define GAME_MODULE "libil2cpp.so"

// --- 实时日志系统 ---
std::atomic<int> g_InitCallCount{0};
std::vector<std::string> g_AppLogs;
std::mutex g_LogMutex;

void AddLog(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    __android_log_print(ANDROID_LOG_INFO, TAG, "%s", buf);
    
    std::lock_guard<std::mutex> lock(g_LogMutex);
    if (g_AppLogs.size() > 30) g_AppLogs.erase(g_AppLogs.begin());
    g_AppLogs.push_back(buf);
}

#undef LOGI
#undef LOGE
#define LOGI(...) AddLog(__VA_ARGS__)
#define LOGE(...) AddLog("[ERROR] " __VA_ARGS__)

static bool g_Initialized = false;
static bool g_FontLoaded = false;
static bool g_IsHookInstalled = false;

struct {
    float x = 0.0f;
    float y = 0.0f;
    std::atomic<bool> down{false};
    float renderW = 0.0f;
    float renderH = 0.0f;
    float physW = 3392.0f;
    float physH = 2400.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
} g_Touch;

static float g_UserUIScale = 1.0f;
static uintptr_t g_GameBase = 0;
static uintptr_t g_HookAddr_InitActor = 0;

static std::atomic<uintptr_t> g_CapturedX0{0};
static std::atomic<uintptr_t> g_CapturedX1{0};
static std::atomic<uintptr_t> g_CapturedX2{0};

static int g_MemFd = -1;

// 辅助：读取内存转为 Hex 字符串
std::string ReadMemoryHex(uintptr_t address, size_t size) {
    if (address == 0) return "null";
    unsigned char buffer[32] = {0};
    if (g_MemFd == -1) g_MemFd = open("/proc/self/mem", O_RDONLY);
    if (g_MemFd >= 0) {
        ssize_t bytes_read = pread(g_MemFd, buffer, std::min(size, sizeof(buffer)), address);
        if (bytes_read > 0) {
            std::stringstream ss;
            for (ssize_t i = 0; i < bytes_read; ++i) {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i] << " ";
            }
            return ss.str();
        }
    }
    return "read_failed";
}

// ==========================================
// 【核心修复】使用底层 Linker 迭代器获取真实基址 (Frida 同款原理)
// ==========================================
struct ModuleInfo {
    const char* name;
    uintptr_t base;
};

static int dl_iterate_callback(struct dl_phdr_info *info, size_t size, void *data) {
    ModuleInfo* modInfo = (ModuleInfo*)data;
    if (info->dlpi_name && strstr(info->dlpi_name, modInfo->name)) {
        modInfo->base = info->dlpi_addr;
        return 1; // 找到真实基址，停止遍历
    }
    return 0;
}

uintptr_t GetRealModuleBase(const char* module_name) {
    ModuleInfo info = {module_name, 0};
    dl_iterate_phdr(dl_iterate_callback, &info);
    
    // 如果系统 linker 找不到，再作为备选去查 maps (防极端情况)
    if (info.base == 0) {
        FILE* f = fopen("/proc/self/maps", "r");
        if (f) {
            char line[512];
            while (fgets(line, sizeof(line), f)) {
                if (strstr(line, module_name)) {
                    uintptr_t start;
                    if (sscanf(line, "%lx", &start) == 1) {
                        info.base = start;
                        break;
                    }
                }
            }
            fclose(f);
        }
    }
    return info.base;
}

// ==========================================
// Hook 目标：InitActor
// ==========================================
typedef void (*t_InitActorParams)(void* x0, void* x1, void* x2, void* x3, void* x4, void* x5, void* x6, void* x7);
t_InitActorParams old_InitActorParams = nullptr;

void hook_InitActorParams(void* x0, void* x1, void* x2, void* x3, void* x4, void* x5, void* x6, void* x7) {
    g_InitCallCount++; 
    
    if (x0 != nullptr) g_CapturedX0.store((uintptr_t)x0);
    if (x1 != nullptr) g_CapturedX1.store((uintptr_t)x1);
    if (x2 != nullptr) g_CapturedX2.store((uintptr_t)x2);
    
    if (g_InitCallCount.load() % 5 == 1) { // 降低打印频率防卡死
        LOGI("[+] 拦截到实体生成! 当前捕获 x0: 0x%lx", (uintptr_t)x0);
    }
    
    if (old_InitActorParams) {
        old_InitActorParams(x0, x1, x2, x3, x4, x5, x6, x7);
    }
}

// ==========================================
// 手动强力安装 Hook 
// ==========================================
void InstallCoreHook() {
    if (g_IsHookInstalled) return;

    uintptr_t base = GetRealModuleBase(GAME_MODULE);
    if (!base) {
        LOGE("[-] 未找到 libil2cpp.so，游戏可能未加载完全！");
        return;
    }
    
    g_GameBase = base;
    g_HookAddr_InitActor = base + 0x73507bc;

    LOGI("[*] 真实基址: 0x%lx", base);
    LOGI("[*] 目标地址: 0x%lx", g_HookAddr_InitActor);

    // 1. 挂钩前读取原始机器码
    std::string bytesBefore = ReadMemoryHex(g_HookAddr_InitActor, 8);
    LOGI("[*] 挂钩前内存: %s", bytesBefore.c_str());

    // 2. 强制解锁内存页权限 (防止 Dobby 内部 mprotect 被拦截导致挂钩失败)
    void* page_start = (void*)(g_HookAddr_InitActor & ~0xFFF);
    mprotect(page_start, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);

    // 3. 执行 Dobby Hook
    int ret = DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
    
    // 4. 清理 CPU 指令缓存，强制生效
    __builtin___clear_cache((char*)g_HookAddr_InitActor, (char*)(g_HookAddr_InitActor + 16));

    // 5. 挂钩后重新读取机器码，验证是否真的写进去了
    std::string bytesAfter = ReadMemoryHex(g_HookAddr_InitActor, 8);
    LOGI("[*] 挂钩后内存: %s", bytesAfter.c_str());

    if (ret == 0) {
        if (bytesBefore == bytesAfter) {
            LOGE("[-] 致命错误：Dobby 返回成功，但内存完全没变！说明 Hook 未生效（被壳拦截）。");
        } else {
            g_IsHookInstalled = true;
            LOGI("[+] 手动安装 Hook 成功！请开始对局。");
        }
    } else {
        LOGE("[-] 挂钩失败，Dobby 错误码: %d", ret);
    }
}

// 触摸事件拦截
void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float autoScaleX = g_Touch.renderW / g_Touch.physW;
        float autoScaleY = g_Touch.renderH / g_Touch.physH;
        g_Touch.x = (AMotionEvent_getX(event, 0) * autoScaleX) + g_Touch.offsetX;
        g_Touch.y = (AMotionEvent_getY(event, 0) * autoScaleY) + g_Touch.offsetY;
        if (action == AMOTION_EVENT_ACTION_DOWN) g_Touch.down.store(true);
        else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_Touch.down.store(false);
    }
}

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

int hook_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int res = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    if (res == 0 && outEvent && *outEvent) handle_android_event((AInputEvent*)(*outEvent));
    return res;
}

// eglSwapBuffers 渲染 Hook
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    if (w <= 0 || h <= 0) return old_eglSwapBuffers(dpy, surface);
    g_Touch.renderW = (float)w;
    g_Touch.renderH = (float)h;

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGuiIO& io = ImGui::GetIO();
        const char* systemFonts[] = { "/system/fonts/SysSans-Hans-Regular.ttf", "/system/fonts/NotoSansCJKjp-Regular.otc", "/system/fonts/NotoSansSC-Regular.otf", "/system/fonts/DroidSansFallback.ttf" };
        for (int i = 0; i < 4; i++) {
            if (access(systemFonts[i], R_OK) == 0) {
                io.Fonts->AddFontFromFileTTF(systemFonts[i], 32.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                g_FontLoaded = true; break;
            }
        }
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down.load();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("单钩子纯净测试版")) {
        
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.15f, 1.0f));
        ImGui::BeginChild("HookController", ImVec2(0, 70), true);
        if (!g_IsHookInstalled) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("🔴 在大厅界面点击此按钮注入", ImVec2(-1, 40))) {
                InstallCoreHook();
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ Hook 注入完成！快去进对局测试。");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "捕获到的最新指针:");
        ImGui::Text("x0: 0x%lx", g_CapturedX0.load());
        ImGui::Text("x1: 0x%lx", g_CapturedX1.load());
        ImGui::Text("x2: 0x%lx", g_CapturedX2.load());

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[库] libil2cpp.so 基址: 0x%lx", g_GameBase);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[钩] InitActor 绝对地址: 0x%lx", g_HookAddr_InitActor);

        ImGui::Separator();
        ImGui::TextColored(g_InitCallCount > 0 ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), 
            "> 当前成功拦截次数: %d", g_InitCallCount.load());

        ImGui::BeginChild("LogRegion", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        std::lock_guard<std::mutex> lock(g_LogMutex);
        for (const auto& log : g_AppLogs) {
            ImGui::TextUnformatted(log.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::Render();
    GLint v[4]; glGetIntegerv(GL_VIEWPORT, v);
    GLint d; glGetIntegerv(GL_DEPTH_TEST, &d);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, w, h);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glViewport(v[0], v[1], v[2], v[3]);
    if (d) glEnable(GL_DEPTH_TEST);

    return old_eglSwapBuffers(dpy, surface);
}

void* DelayedHookThread(void*) {
    void* egl = nullptr;
    while (!egl) { egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers"); if (!egl) sleep(1); }
    DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    void* ins = nullptr;
    while (!ins) { ins = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE"); if (!ins) sleep(1); }
    DobbyHook(ins, (void*)hook_consume, (void**)&old_consume);

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
