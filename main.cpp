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
// 目标模块
#define GAME_MODULE "libGameCore.so" 

#define LOGI(...) \__android_log_print(ANDROID_LOG_INFO, TAG, \__VA_ARGS_\_)
#define LOGE(...) \__android_log_print(ANDROID_LOG_ERROR, TAG, \__VA_ARGS_\_)

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
    bool isSizeAutoDetected = false;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
} g_Touch;

static float g_DynamicScale = 1.0f;
static float g_BaseWindowHeight = 0.0f; 
static float g_UserUIScale = 1.0f; 

static uintptr_t g_GameBase = 0;
static uintptr_t g_HookAddr_InitActor = 0;
static uintptr_t g_HookAddr_ClearActor = 0;

// 【已更新】根据您的描述精确填充了所有偏移
struct DynamicOffsets {
    uint32_t x0_to_addr1    = 0x20;
    uint32_t addr1_to_addr2 = 0x10;
    uint32_t item_to_addr3  = 0x18;
    uint32_t addr3_to_addr4 = 0x30;
    uint32_t addr3_to_addr5 = 0x100;
    uint32_t addr3_to_addr6 = 0x70;
    
    uint32_t prop_gold  = 0x20;
    uint32_t prop_hp    = 0x30;
    uint32_t prop_maxhp = 0x32;
    uint32_t prop_level = 0x50;
    uint32_t prop_mana  = 0x28;

    uint32_t radar_map_x = 0x50;
    uint32_t radar_map_y = 0x54;
};
DynamicOffsets g_Offsets;
std::mutex g_OffsetMutex;

static int g_MemFd = -1;

template <typename T>
T SafeRead(uintptr_t address) {
    T value{};
    if (address < 0x10000000 || address > 0x00007FFFFFFFFFFF) return value; 
    if (g_MemFd == -1) g_MemFd = open("/proc/self/mem", O_RDONLY);
    if (g_MemFd >= 0) pread(g_MemFd, &value, sizeof(T), address);
    return value;
}

struct PlayerData {
    int hp = 0, maxHp = 0, mana = 0, gold = 0, level = 0;
    float worldX = 0, worldY = 0, worldZ = 0;
    float mapX = 0, mapY = 0;
};

// 【已更新】调试器结构，与您的指针链一一对应
struct DebugChain {
    uintptr_t baseX0 = 0;
    uintptr_t addr1 = 0;
    uintptr_t addr2 = 0;
    struct Item {
        uintptr_t base; // 英雄item
        uintptr_t addr3, addr4, addr5, addr6;
    } items[15];
};

struct GameSnapshot {
    bool inMatch = false;
    DynamicOffsets offsets; 
    DebugChain debug;
    std::vector<PlayerData> players;
};
std::mutex g_SnapshotMutex;
GameSnapshot g_CurrentSnapshot; 

// 我们要找的起始指针，从 InitActorParams 的 x0 参数获得
std::atomic<uintptr_t> g_ActorManager{0};

// 【已更新】数据抓取线程，严格按照您的描述实现
void DataWorkerThread() {
    while (true) {
        GameSnapshot newSnap;
        DynamicOffsets currOff;
        { std::lock_guard<std::mutex> lock(g_OffsetMutex); currOff = g_Offsets; }
        newSnap.offsets = currOff;

        uintptr_t baseX0 = g_ActorManager.load();
        newSnap.debug.baseX0 = baseX0;

        if (baseX0 != 0) {
            newSnap.inMatch = true;
            
            uintptr_t addr1 = SafeRead<uintptr_t>(baseX0 + currOff.x0_to_addr1);
            newSnap.debug.addr1 = addr1;

            if (addr1) {
                uintptr_t addr2 = SafeRead<uintptr_t>(addr1 + currOff.addr1_to_addr2);
                newSnap.debug.addr2 = addr2;

                if (addr2) {
                    for (int i = 0; i < 15; ++i) { // 假设最多15个单位
                        uintptr_t item = SafeRead<uintptr_t>(addr2 + (i * 0x8)); // 遍历英雄数组
                        newSnap.debug.items[i].base = item;
                        if (!item) continue;

                        uintptr_t addr3 = SafeRead<uintptr_t>(item + currOff.item_to_addr3);
                        newSnap.debug.items[i].addr3 = addr3;
                        if (!addr3) continue;

                        PlayerData p;

                        // 读取世界坐标和蓝量 (通过 addr5)
                        uintptr_t addr5 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr5);
                        newSnap.debug.items[i].addr5 = addr5;
                        if (addr5) {
                            p.worldX = SafeRead<float>(addr5 + 0x0);
                            p.worldZ = SafeRead<float>(addr5 + 0x4); // Z/Y 取决于游戏引擎坐标系
                            p.worldY = SafeRead<float>(addr5 + 0x8);
                            p.mana = SafeRead<int32_t>(addr5 + currOff.prop_mana);
                        }
                        
                        // 读取金币、血量、等级 (通过 addr4)
                        uintptr_t addr4 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr4);
                        newSnap.debug.items[i].addr4 = addr4;
                        if (addr4) {
                            p.gold = SafeRead<int32_t>(addr4 + currOff.prop_gold);
                            p.hp = SafeRead<int32_t>(addr4 + currOff.prop_hp);
                            p.maxHp = SafeRead<int16_t>(addr4 + currOff.prop_maxhp); // maxhp是16位
                            p.level = SafeRead<int32_t>(addr4 + currOff.prop_level);
                        }

                        // 读取小地图坐标 (通过 addr6)
                        uintptr_t addr6 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr6);
                        newSnap.debug.items[i].addr6 = addr6;
                        if (addr6) {
                            p.mapX = SafeRead<float>(addr6 + currOff.radar_map_x);
                            p.mapY = SafeRead<float>(addr6 + currOff.radar_map_y);
                        }

                        if (p.maxHp > 100 && p.maxHp < 50000) {
                            newSnap.players.push_back(p);
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
        if (strstr(line, module_name) && strstr(line, "x")) { // 确保是可执行段
            sscanf(line, "%lx", &base);
            break;
        }
    }
    fclose(f);
    return base;
}

// Hook InitActorParams，目的是获取它的第一个参数 x0
void (*old_InitActorParams)(void* x0, void* x1, void* x2, void* x3);
void hook_InitActorParams(void* x0, void* x1, void* x2, void* x3) {
    g_ActorManager.store((uintptr_t)x0);
    old_InitActorParams(x0, x1, x2, x3);
}

// Hook ClearActor，目的是知道游戏结束
void (*old_ClearActor)(void* x0);
void hook_ClearActor(void* x0) {
    g_ActorManager.store(0); // 游戏结束，清空指针
    old_ClearActor(x0);
}

// 【已更新】指针调试器，完全匹配您的指针链
void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.55f, h * 0.8f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("指针断点排错器 (Pointer Debugger)")) {
        // 动态偏移修改面板... (代码不变)

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[核心库] %s 基址: 0x%lx", GAME_MODULE, g_GameBase);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[Hook 1] InitActor (RVA 0x73507bc) -> 0x%lx", g_HookAddr_InitActor);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[Hook 2] ClearActor (RVA 0x734bc10) -> 0x%lx", g_HookAddr_ClearActor);
        ImGui::Separator();

        const auto& d = snap.debug;
        const auto& off = snap.offsets;

        ImGui::TextColored(d.baseX0 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "[起点] InitActor x0: 0x%lx", d.baseX0);
        ImGui::TextColored(d.addr1 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), " └─ [+0x%X] 地址 1: 0x%lx", off.x0_to_addr1, d.addr1);
        ImGui::TextColored(d.addr2 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     └─ [+0x%X] 英雄数组: 0x%lx", off.addr1_to_addr2, d.addr2);
        
        if (d.addr2 != 0) {
            for (int i = 0; i < 15; i++) {
                if (d.items[i].base == 0) continue; 
                char nodeName[64]; snprintf(nodeName, sizeof(nodeName), "数组序号 [%d] - 0x%lx", i, d.items[i].base);
                if (ImGui::TreeNodeEx(nodeName, ImGuiTreeNodeFlags_Framed)) {
                    ImGui::TextColored(d.items[i].addr3 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), " └─ [+0x%X] 实体数据 (addr3): 0x%lx", off.item_to_addr3, d.items[i].addr3);
                    if (d.items[i].addr3) {
                        ImGui::TextColored(d.items[i].addr4 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     ├─ [+0x%X] 属性区 (addr4): 0x%lx", off.addr3_to_addr4, d.items[i].addr4);
                        ImGui::TextColored(d.items[i].addr5 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     ├─ [+0x%X] 坐标/蓝量区 (addr5): 0x%lx", off.addr3_to_addr5, d.items[i].addr5);
                        ImGui::TextColored(d.items[i].addr6 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     └─ [+0x%X] 小地图区 (addr6): 0x%lx", off.addr3_to_addr6, d.items[i].addr6);
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::End();
}


//
// =========================================================================
// 以下是渲染、输入、UI主窗口等通用代码，无需修改
// =========================================================================
//

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;
typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;
void AutoFetchDeviceResolution() { /* ... */ }
void handle_android_event(AInputEvent* event) { /* ... */ }
int hook_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) { /* ... */ return 0;}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h; eglQuerySurface(dpy, surface, EGL_WIDTH, &w); eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    if (w <= 0 || h <= 0) return old_eglSwapBuffers(dpy, surface);

    g_Touch.renderW = (float)w; g_Touch.renderH = (float)h;
    GameSnapshot snapshot;
    { std::lock_guard<std::mutex> lock(g_SnapshotMutex); snapshot = g_CurrentSnapshot; }

    if (!g_Initialized) {
        // ... ImGui 初始化和字体加载 ... (代码不变)
        g_Initialized = true;
    }
    
    ImGuiIO& io = ImGui::GetIO();
    // ... ImGui NewFrame, Style, etc. ... (代码不变)
    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
    
    // 主窗口UI
    if (ImGui::Begin("全自动游戏透视框架 (OPPO/适配版)")) {
        // ... (UI代码不变)
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "状态: %s", snapshot.inMatch ? "局内" : "大厅");
        if (ImGui::BeginTable("PlayersTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            // ... (表格代码不变)
        }
    }
    ImGui::End();

    DrawPointerDebugger(snapshot, w, h);
    // ... Radar, ImGui::Render 等 ... (代码不变)
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_eglSwapBuffers(dpy, surface);
}

// 【已更新】Hook线程，填入您的RVA
void* DelayedHookThread(void*) {
    // 1. 无延迟 Hook：渲染和输入 (代码不变)
    
    // 2. 启动数据线程
    std::thread(DataWorkerThread).detach();

    // 3. 后台轮询，等待 libGameCore.so 加载
    while (true) {
        uintptr_t base = GetModuleBase(GAME_MODULE);
        if (base) {
            g_GameBase = base;
            
            // Hook InitActorParams
            g_HookAddr_InitActor = base + 0x73507bc;
            DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
            
            // Hook ClearActor
            g_HookAddr_ClearActor = base + 0x734bc10;
            DobbyHook((void*)g_HookAddr_ClearActor, (void*)hook_ClearActor, (void**)&old_ClearActor);

            LOGI("[*] 核心 Hook 成功! (目标: %s)", GAME_MODULE);
            break;
        }
        sleep(1);
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
