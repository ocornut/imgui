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
// 修改为目标模块 libil2cpp.so
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
    bool isSizeAutoDetected = false;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
} g_Touch;

static float g_DynamicScale = 1.0f;
static float g_BaseWindowHeight = 0.0f; 
static float g_UserUIScale = 1.2f; 

// 调试统计
static std::atomic<int> g_InitActorTriggerCount{0};
static uintptr_t g_GameBase = 0;
static uintptr_t g_HookAddr_InitActor = 0;
static uintptr_t g_HookAddr_ClearActor = 0;

struct DynamicOffsets {
    uint32_t x0_to_addr1    = 32;
    uint32_t addr1_to_addr2 = 16;
    uint32_t item_to_addr3  = 24;
    uint32_t addr3_to_addr4 = 48;
    uint32_t addr3_to_addr5 = 256;
    uint32_t addr3_to_addr6 = 112;
    
    uint32_t prop_gold  = 32;
    uint32_t prop_hp    = 48;
    uint32_t prop_maxhp = 50;
    uint32_t prop_level = 80;
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

struct DebugChain {
    uintptr_t baseX0 = 0;
    uintptr_t addr1 = 0;
    uintptr_t addr2 = 0;
    struct Item {
        uintptr_t base = 0, addr3 = 0, addr4 = 0, addr5 = 0, addr6 = 0;
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
std::atomic<uintptr_t> g_ActorManager{0};

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
                    for (int i = 0; i < 15; ++i) { 
                        uintptr_t item = SafeRead<uintptr_t>(addr2 + (i * 0x8)); 
                        newSnap.debug.items[i].base = item;
                        if (!item) continue;
                        uintptr_t addr3 = SafeRead<uintptr_t>(item + currOff.item_to_addr3);
                        newSnap.debug.items[i].addr3 = addr3;
                        if (!addr3) continue;
                        PlayerData p;
                        uintptr_t addr4 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr4);
                        newSnap.debug.items[i].addr4 = addr4;
                        if (addr4) {
                            p.gold = SafeRead<int32_t>(addr4 + currOff.prop_gold);
                            p.hp = SafeRead<int32_t>(addr4 + currOff.prop_hp);
                            p.maxHp = SafeRead<int32_t>(addr4 + currOff.prop_maxhp); 
                            p.level = SafeRead<int32_t>(addr4 + currOff.prop_level);
                        }
                        uintptr_t addr5 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr5);
                        newSnap.debug.items[i].addr5 = addr5;
                        if (addr5) {
                            p.worldX = SafeRead<float>(addr5 + 0x0);
                            p.worldZ = SafeRead<float>(addr5 + 0x4);
                            p.worldY = SafeRead<float>(addr5 + 0x8);
                            p.mana = SafeRead<int32_t>(addr5 + 0x28);
                        }
                        uintptr_t addr6 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr6);
                        newSnap.debug.items[i].addr6 = addr6;
                        if (addr6) {
                            p.mapX = SafeRead<float>(addr6 + 0x50);
                            p.mapY = SafeRead<float>(addr6 + 0x54);
                        }
                        if (p.maxHp > 0 && p.maxHp < 50000) newSnap.players.push_back(p);
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

void (*old_InitActorParams)(void* x0, void* x1, void* x2, void* x3);
void hook_InitActorParams(void* x0, void* x1, void* x2, void* x3) {
    g_InitActorTriggerCount++;
    LOGI("[TRIGGER] InitActorParams 被调用 (libil2cpp)! x0 = %p", x0);
    g_ActorManager.store((uintptr_t)x0);
    old_InitActorParams(x0, x1, x2, x3);
}

void (*old_ClearActor)(void* x0);
void hook_ClearActor(void* x0) {
    LOGI("[TRIGGER] ClearActor 被调用! 对局结束重置。");
    g_ActorManager.store(0);
    old_ClearActor(x0);
}

void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.55f, h * 0.8f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("对局指针诊断 (IL2CPP Debugger)")) {
        
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "函数触发次数: %d", g_InitActorTriggerCount.load());
        if (g_InitActorTriggerCount == 0) {
            ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "状态: 等待进入对局 (尚未触发 Hook)");
        }
        ImGui::Separator();

        if (ImGui::CollapsingHeader("偏移量微调面板", ImGuiTreeNodeFlags_DefaultOpen)) {
            std::lock_guard<std::mutex> lock(g_OffsetMutex);
            auto drawOffsetBtn = [](const char* label, uint32_t& val) {
                ImGui::PushID(&val);
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s -> 当前: %d (0x%X)", label, val, val);
                if (ImGui::Button("-1")) { if (val >= 1) val -= 1; else val = 0; } ImGui::SameLine();
                if (ImGui::Button("+1")) val += 1; ImGui::SameLine();
                if (ImGui::Button("+4")) val += 4; ImGui::SameLine();
                if (ImGui::Button("+8")) val += 8;
                ImGui::Separator();
                ImGui::PopID();
            };
            drawOffsetBtn("基址偏移 (x0 + ?)", g_Offsets.x0_to_addr1);
            drawOffsetBtn("数组偏移 (addr1 + ?)", g_Offsets.addr1_to_addr2);
            drawOffsetBtn("实体数据 (item + ?)", g_Offsets.item_to_addr3);
        }
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[库基址] 0x%lx (libil2cpp)", g_GameBase);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[Hook点] 0x%lx", g_HookAddr_InitActor);
        ImGui::Separator();

        const auto& d = snap.debug;
        const auto& off = snap.offsets;
        ImGui::TextColored(d.baseX0 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "[X0] 基准地址: 0x%lx", d.baseX0);
        ImGui::TextColored(d.addr1 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), " └─ [+%d] 层级 1: 0x%lx", off.x0_to_addr1, d.addr1);
        ImGui::TextColored(d.addr2 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     └─ [+%d] 实体数组: 0x%lx", off.addr1_to_addr2, d.addr2);
        
        if (d.addr2 != 0) {
            for (int i = 0; i < 15; i++) {
                if (d.items[i].base == 0) continue; 
                char nodeName[64]; snprintf(nodeName, sizeof(nodeName), "玩家 [%d] - 0x%lx", i, d.items[i].base);
                if (ImGui::TreeNodeEx(nodeName, ImGuiTreeNodeFlags_Framed)) {
                    ImGui::TextColored(d.items[i].addr3 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), " └─ [+%d] 详细数据: 0x%lx", off.item_to_addr3, d.items[i].addr3);
                    ImGui::TreePop();
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
        
        // 自动检测系统中的中文字体 (优先寻找 OPPO SysSans)
        const char* systemFonts[] = {
            "/system/fonts/SysSans-Hans-Regular.ttf",
            "/system/fonts/NotoSansCJKjp-Regular.otc",
            "/system/fonts/NotoSansSC-Regular.otf",
            "/system/fonts/DroidSansFallback.ttf"
        };
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

    float deviceScale = (float)h / 1000.0f;
    if (deviceScale < 1.0f) deviceScale = 1.0f;
    float finalScale = deviceScale * g_DynamicScale * g_UserUIScale; 

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(1.1f * finalScale); 
    io.FontGlobalScale = 1.1f * finalScale;

    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
    if (g_Touch.down.load()) ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(0, 255, 0, 200));

    if (g_BaseWindowHeight == 0.0f) g_BaseWindowHeight = h * 0.45f; 
    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.6f, g_BaseWindowHeight), ImGuiCond_FirstUseEver);
    
    static bool show_pointer_debugger = true; 

    if (ImGui::Begin("Unity/IL2CPP 数据诊断控制台")) {
        if (!g_FontLoaded) ImGui::TextColored(ImVec4(1,0,0,1), "警告: 无法加载手机字库，请检查权限!");
        ImGui::SliderFloat("菜单缩放", &g_UserUIScale, 0.5f, 2.0f);
        ImGui::Checkbox("指针断点调试窗", &show_pointer_debugger);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "对局状态: %s", snapshot.inMatch ? "活跃对局中" : "大厅空闲");
        ImGui::Text("英雄单位数量: %d", (int)snapshot.players.size());
    }
    ImGui::End();

    if (show_pointer_debugger) DrawPointerDebugger(snapshot, w, h);

    ImGui::Render();
    GLint v[4]; glGetIntegerv(GL_VIEWPORT, v); 
    glDisable(GL_DEPTH_TEST); glViewport(0, 0, w, h); 
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glViewport(v[0], v[1], v[2], v[3]);
    return old_eglSwapBuffers(dpy, surface);
}

void* DelayedHookThread(void*) {
    // 渲染和输入立即挂钩，确保菜单秒弹
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    
    std::thread(DataWorkerThread).detach();

    // 在后台轮询等待 libil2cpp.so 加载
    while (true) {
        uintptr_t base = GetModuleBase(GAME_MODULE);
        if (base) {
            g_GameBase = base;
            g_HookAddr_InitActor = base + 0x73507bc;
            // 挂钩对局初始化函数
            DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
            // 挂钩对局清理函数
            DobbyHook((void*)(base + 0x734bc10), (void*)hook_ClearActor, (void**)&old_ClearActor);
            LOGI("[*] libil2cpp.so 挂钩任务已完成!"); 
            break;
        }
        sleep(1);
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
