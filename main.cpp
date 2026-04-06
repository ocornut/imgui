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
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "ZeroConfig_Drive"
#define GAME_MODULE "libil2cpp.so" 

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static bool g_Initialized = false;
static bool g_FontLoaded = false; 

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

static float g_DynamicScale = 1.0f;
static float g_UserUIScale = 1.2f; 

// Hook 状态实时监控
static std::atomic<int> g_InitActorTriggerCount{0};
static std::atomic<int> g_DobbyStatus{-99}; // -99 表示尚未尝试
static uintptr_t g_GameBase = 0;
static uintptr_t g_HookAddr_InitActor = 0;

// =================================================================
// 核心：使用管道进行安全内存读取
// =================================================================
static int g_SafePipe[2] = {-1, -1};
void InitSafePipe() {
    if (g_SafePipe[0] == -1 && pipe(g_SafePipe) == -1) {
        LOGE("[-] 管道创建失败！");
    }
}

template <typename T>
T SafeRead(uintptr_t address) {
    T value{};
    if (address < 0x10000000 || address > 0x00007FFFFFFFFFFF) return value;
    if (g_SafePipe[0] == -1) InitSafePipe();
    if (write(g_SafePipe[1], (void*)address, sizeof(T)) == -1) return value;
    read(g_SafePipe[0], &value, sizeof(T));
    return value;
}

struct DynamicOffsets {
    uint32_t x0_to_addr1    = 32;   
    uint32_t addr1_to_addr2 = 16;   
    uint32_t item_to_addr3  = 24;   
    uint32_t addr3_to_addr4 = 48;   
    uint32_t prop_hp        = 48;
};

DynamicOffsets g_Offsets;
std::mutex g_OffsetMutex;

struct DebugChain {
    uintptr_t baseX0 = 0;
    uintptr_t addr1 = 0;
    uintptr_t addr2 = 0;
    struct Item {
        uintptr_t base = 0, addr3 = 0, addr4 = 0, hp_final_ptr = 0;
    } items[10];
};

struct GameSnapshot {
    bool inMatch = false;
    DynamicOffsets offsets; 
    DebugChain debug;
};

std::mutex g_SnapshotMutex;
GameSnapshot g_CurrentSnapshot; 
std::atomic<uintptr_t> g_ActorManager{0};

// 数据抓取后台线程
void DataWorkerThread() {
    while (true) {
        GameSnapshot newSnap;
        DynamicOffsets currOff;
        { std::lock_guard<std::mutex> lock(g_OffsetMutex); currOff = g_Offsets; }
        newSnap.offsets = currOff;

        uintptr_t baseX0 = g_ActorManager.load();
        if (baseX0 != 0) {
            newSnap.inMatch = true;
            newSnap.debug.baseX0 = baseX0;
            uintptr_t addr1 = SafeRead<uintptr_t>(baseX0 + currOff.x0_to_addr1);
            newSnap.debug.addr1 = addr1;
            if (addr1) {
                uintptr_t addr2 = SafeRead<uintptr_t>(addr1 + currOff.addr1_to_addr2);
                newSnap.debug.addr2 = addr2;
                if (addr2) {
                    for (int i = 0; i < 10; ++i) { 
                        uintptr_t item = SafeRead<uintptr_t>(addr2 + (i * 0x8)); 
                        newSnap.debug.items[i].base = item;
                        if (!item) continue;
                        uintptr_t addr3 = SafeRead<uintptr_t>(item + currOff.item_to_addr3);
                        newSnap.debug.items[i].addr3 = addr3;
                        if (addr3) {
                            uintptr_t addr4 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr4);
                            newSnap.debug.items[i].addr4 = addr4;
                            if (addr4) newSnap.debug.items[i].hp_final_ptr = addr4 + currOff.prop_hp;
                        }
                    }
                }
            }
        }
        { std::lock_guard<std::mutex> lock(g_SnapshotMutex); g_CurrentSnapshot = newSnap; }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

uintptr_t GetModuleBase(const char* module_name) {
    uintptr_t base = 0;
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, module_name)) {
            sscanf(line, "%lx", &base);
            break;
        }
    }
    fclose(f);
    return base;
}

// =================================================================
// 目标 Hook 函数
// =================================================================
void (*old_InitActorParams)(void* x0, void* x1, void* x2, void* x3);
void hook_InitActorParams(void* x0, void* x1, void* x2, void* x3) {
    g_InitActorTriggerCount++;
    LOGI("[****************] InitActorParams Hooked! x0 = %p [****************]", x0);
    g_ActorManager.store((uintptr_t)x0);
    old_InitActorParams(x0, x1, x2, x3);
}

// =================================================================
// UI 渲染
// =================================================================
void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(w * 0.45f, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.52f, h * 0.85f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("全量地址链路监控 (Debug Mode)")) {
        
        // 1. Hook 诊断区 (核心)
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "---- 核心 Hook 状态诊断 ----");
        ImGui::Text("Dobby 挂载状态: "); ImGui::SameLine();
        if (g_DobbyStatus == 0) ImGui::TextColored(ImVec4(0, 1, 0, 1), "已挂载 (Success)");
        else if (g_DobbyStatus == -99) ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "尚未尝试...");
        else ImGui::TextColored(ImVec4(1, 0, 0, 1), "挂载失败! 错误码: %d", (int)g_DobbyStatus);

        ImGui::Text("Hook 触发次数: "); ImGui::SameLine();
        ImGui::TextColored(g_InitActorTriggerCount > 0 ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0.5f, 0, 1), 
                           "%d 次", (int)g_InitActorTriggerCount);
        
        if (g_InitActorTriggerCount == 0) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "注意: 如果你已在对局中但触发次数为0，说明 Hook 没生效!");
        }
        ImGui::Separator();

        // 2. 偏移调节
        if (ImGui::CollapsingHeader("偏移微调 (十进制按钮)", ImGuiTreeNodeFlags_DefaultOpen)) {
            std::lock_guard<std::mutex> lock(g_OffsetMutex);
            auto drawOffsetBtn = [](const char* label, uint32_t& val) {
                ImGui::PushID(&val);
                ImGui::Text("%s: %u (0x%X)", label, val, val);
                if (ImGui::Button("-1")) val -= 1; ImGui::SameLine();
                if (ImGui::Button("+1")) val += 1; ImGui::SameLine();
                if (ImGui::Button("+8")) val += 8;
                ImGui::PopID();
                ImGui::Separator();
            };
            drawOffsetBtn("第一级 (X0 + ?)", g_Offsets.x0_to_addr1);
            drawOffsetBtn("英雄数组 (Addr1 + ?)", g_Offsets.addr1_to_addr2);
            drawOffsetBtn("英雄数据 (Item + ?)", g_Offsets.item_to_addr3);
        }
        
        // 3. 地址树
        ImGui::Separator();
        const auto& d = snap.debug;
        const auto& off = snap.offsets;

        ImGui::TextColored(ImVec4(0, 1, 1, 1), "libil2cpp 基址: 0x%lx", g_GameBase);
        ImGui::TextColored(d.baseX0 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "[起点] X0 基地址: 0x%lx", d.baseX0);
        
        if (d.baseX0) {
            ImGui::TextColored(d.addr1 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                               " └─ Addr1: 0x%lx (X0+%d)", d.addr1, off.x0_to_addr1);
            ImGui::TextColored(d.addr2 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                               "     └─ 英雄数组: 0x%lx (Addr1+%d)", d.addr2, off.addr1_to_addr2);
            
            if (d.addr2) {
                ImGui::Separator();
                for (int i = 0; i < 5; i++) {
                    if (d.items[i].base == 0) continue;
                    char lbl[128]; snprintf(lbl, 128, "英雄[%d] 实例: 0x%lx", i, d.items[i].base);
                    if (ImGui::TreeNode(lbl)) {
                        ImGui::Text("Addr3: 0x%lx", d.items[i].addr3);
                        ImGui::Text("Addr4: 0x%lx", d.items[i].addr4);
                        ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "最终数据存放点: 0x%lx", d.items[i].hp_final_ptr);
                        ImGui::TreePop();
                    }
                }
            }
        }
    }
    ImGui::End();
}

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h; eglQuerySurface(dpy, surface, EGL_WIDTH, &w); eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    if (w <= 0 || h <= 0) return old_eglSwapBuffers(dpy, surface);
    g_Touch.renderW = (float)w; g_Touch.renderH = (float)h;

    GameSnapshot snapshot;
    { std::lock_guard<std::mutex> lock(g_SnapshotMutex); snapshot = g_CurrentSnapshot; }

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGuiIO& io = ImGui::GetIO();
        const char* myFonts[] = { "/system/fonts/SysSans-Hans-Regular.ttf", "/system/fonts/NotoSansCJKjp-Regular.otc", "/system/fonts/NotoSansSC-Regular.otf", "/system/fonts/DroidSansFallback.ttf" };
        for (int i = 0; i < 4; i++) {
            if (access(myFonts[i], R_OK) == 0) {
                io.Fonts->AddFontFromFileTTF(myFonts[i], 32.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                g_FontLoaded = true; break;
            }
        }
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down.load(); 

    float finalScale = (h / 1000.0f) * g_UserUIScale; 
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(1.2f * finalScale);
    style.WindowRounding = 10.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    io.FontGlobalScale = 1.2f * finalScale;

    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
    
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.4f, h * 0.3f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("自动对局控制台 (SGame/V3)")) {
        ImGui::SliderFloat("菜单大小", &g_UserUIScale, 0.5f, 2.0f);
        if (ImGui::Button(" 重置触控校准 (RESET) ")) { g_Touch.offsetX = 0; g_Touch.offsetY = 0; }
        ImGui::Separator();
        ImGui::Text("数据读取状态: %s", snapshot.inMatch ? "读取中" : "待命中");
    }
    ImGui::End();

    DrawPointerDebugger(snapshot, w, h);
    
    ImGui::Render();
    GLint v[4]; glGetIntegerv(GL_VIEWPORT, v); glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, w, h); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glViewport(v[0], v[1], v[2], v[3]);
    return old_eglSwapBuffers(dpy, surface);
}

void* DelayedHookThread(void*) {
    InitSafePipe();
    
    // 1. 立即挂载渲染 Hook，菜单秒弹
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    
    std::thread(DataWorkerThread).detach();

    // 2. 后台循环等待并挂载 RVA Hook
    while (true) {
        uintptr_t base = GetModuleBase(GAME_MODULE);
        if (base) {
            g_GameBase = base;
            g_HookAddr_InitActor = base + 0x73507bc;
            
            // 尝试 Hook 并记录状态
            int ret = DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
            g_DobbyStatus.store(ret);
            
            if (ret == 0) {
                LOGI("[+] libil2cpp Hook 成功挂载!");
                // 清理函数 Hook (非核心，顺带做)
                void* old_tmp;
                DobbyHook((void*)(base + 0x734bc10), (void*)hook_InitActorParams, &old_tmp);
            } else {
                LOGE("[-] libil2cpp Hook 挂载失败! Code: %d", ret);
            }
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
