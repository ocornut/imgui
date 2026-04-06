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
#include <cstdarg>
#include <cstring>
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "IL2CPP_Hook"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// 全局变量
static std::atomic<uintptr_t> g_ActorManager{0};
static int g_MemFd = -1;
static bool g_ImGuiReady = false;
static float g_touchX = 0, g_touchY = 0;
static std::atomic<bool> g_touchDown{false};

// 偏移结构（可动态调整）
struct Offsets {
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
} g_Offsets;

// 玩家数据结构
struct PlayerData {
    int hp = 0, maxHp = 0, mana = 0, gold = 0, level = 0;
    float mapX = 0, mapY = 0;
};

// 快照
struct GameSnapshot {
    bool inMatch = false;
    uintptr_t x0 = 0;
    uintptr_t addr1 = 0;
    uintptr_t addr2 = 0;
    std::vector<PlayerData> players;
};

static GameSnapshot g_Snapshot;
static std::mutex g_SnapMutex;

// 安全读取
template<typename T>
T SafeRead(uintptr_t addr) {
    T val{};
    if (addr < 0x10000 || addr > 0x7fffffffffff) return val;
    if (g_MemFd == -1) g_MemFd = open("/proc/self/mem", O_RDONLY);
    if (g_MemFd >= 0) pread(g_MemFd, &val, sizeof(T), addr);
    return val;
}

// 获取模块基址
uintptr_t GetModuleBase(const char* name) {
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

// 后台数据读取线程
void DataWorkerThread() {
    while (true) {
        GameSnapshot snap;
        uintptr_t baseX0 = g_ActorManager.load();
        if (baseX0 != 0) {
            snap.inMatch = true;
            snap.x0 = baseX0;
            uintptr_t addr1 = SafeRead<uintptr_t>(baseX0 + g_Offsets.x0_to_addr1);
            snap.addr1 = addr1;
            if (addr1) {
                uintptr_t addr2 = SafeRead<uintptr_t>(addr1 + g_Offsets.addr1_to_addr2);
                snap.addr2 = addr2;
                if (addr2) {
                    for (int i = 0; i < 15; ++i) {
                        uintptr_t item = SafeRead<uintptr_t>(addr2 + i * 0x8);
                        if (!item) continue;
                        uintptr_t addr3 = SafeRead<uintptr_t>(item + g_Offsets.item_to_addr3);
                        if (!addr3) continue;
                        PlayerData p;
                        uintptr_t addr4 = SafeRead<uintptr_t>(addr3 + g_Offsets.addr3_to_addr4);
                        if (addr4) {
                            p.gold = SafeRead<int32_t>(addr4 + g_Offsets.prop_gold);
                            p.hp   = SafeRead<int32_t>(addr4 + g_Offsets.prop_hp);
                            p.maxHp = SafeRead<int16_t>(addr4 + g_Offsets.prop_maxhp);
                            p.level = SafeRead<int32_t>(addr4 + g_Offsets.prop_level);
                        }
                        uintptr_t addr5 = SafeRead<uintptr_t>(addr3 + g_Offsets.addr3_to_addr5);
                        if (addr5) p.mana = SafeRead<int32_t>(addr5 + g_Offsets.prop_mana);
                        uintptr_t addr6 = SafeRead<uintptr_t>(addr3 + g_Offsets.addr3_to_addr6);
                        if (addr6) {
                            p.mapX = SafeRead<float>(addr6 + 0x50);
                            p.mapY = SafeRead<float>(addr6 + 0x54);
                        }
                        if (p.maxHp > 0 && p.maxHp < 50000)
                            snap.players.push_back(p);
                    }
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(g_SnapMutex);
            g_Snapshot = std::move(snap);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

// ======================== Hook 函数（只有 1 个参数）========================
void (*old_InitActorParams)(void* x0);
void hook_InitActorParams(void* x0) {
    LOGI("[HOOK] InitActorParams called! x0 = %p", x0);
    if (x0) {
        g_ActorManager.store((uintptr_t)x0);
        uintptr_t test = SafeRead<uintptr_t>((uintptr_t)x0 + g_Offsets.x0_to_addr1);
        LOGI("[HOOK] x0+0x%x = %p", g_Offsets.x0_to_addr1, (void*)test);
    }
    old_InitActorParams(x0);
}

void (*old_ClearActor)(void* x0);
void hook_ClearActor(void* x0) {
    LOGI("[HOOK] ClearActor called! x0 = %p", x0);
    g_ActorManager.store(0);
    old_ClearActor(x0);
}

// ======================== 触摸事件处理（ImGui 输入）========================
void HandleAndroidEvent(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        g_touchX = AMotionEvent_getX(event, 0);
        g_touchY = AMotionEvent_getY(event, 0);
        g_touchDown = (action == AMOTION_EVENT_ACTION_DOWN);
    }
}

typedef int (*InputConsumer_consume_t)(void*, void*, bool, int64_t, uint32_t*, void**);
InputConsumer_consume_t old_consume = nullptr;
int hook_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int res = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    if (res == 0 && outEvent && *outEvent)
        HandleAndroidEvent((AInputEvent*)*outEvent);
    return res;
}

// ======================== eglSwapBuffers Hook（ImGui 渲染）========================
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
eglSwapBuffers_t old_eglSwap = nullptr;

void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.55f, h * 0.8f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("指针调试器")) {
        // 动态偏移修改面板
        auto drawOffset = [](const char* label, uint32_t& val) {
            ImGui::PushID(&val);
            ImGui::Text("%s: %d (0x%X)", label, val, val);
            if (ImGui::Button("-8")) { if(val>=8) val-=8; } ImGui::SameLine();
            if (ImGui::Button("-4")) { if(val>=4) val-=4; } ImGui::SameLine();
            if (ImGui::Button("-1")) { if(val>=1) val-=1; } ImGui::SameLine();
            if (ImGui::Button("+1")) val+=1; ImGui::SameLine();
            if (ImGui::Button("+4")) val+=4; ImGui::SameLine();
            if (ImGui::Button("+8")) val+=8;
            ImGui::PopID();
        };
        drawOffset("x0->addr1", g_Offsets.x0_to_addr1);
        drawOffset("addr1->addr2", g_Offsets.addr1_to_addr2);
        drawOffset("item->addr3", g_Offsets.item_to_addr3);
        drawOffset("addr3->addr4", g_Offsets.addr3_to_addr4);
        drawOffset("addr3->addr5", g_Offsets.addr3_to_addr5);
        drawOffset("addr3->addr6", g_Offsets.addr3_to_addr6);
        ImGui::Separator();
        ImGui::Text("x0: 0x%lx", snap.x0);
        ImGui::Text("addr1: 0x%lx", snap.addr1);
        ImGui::Text("addr2: 0x%lx", snap.addr2);
    }
    ImGui::End();
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    EGLint w, h;
    eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    if (w <= 0 || h <= 0) return old_eglSwap(dpy, surf);

    GameSnapshot snap;
    {
        std::lock_guard<std::mutex> lock(g_SnapMutex);
        snap = g_Snapshot;
    }

    if (!g_ImGuiReady) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGui::GetIO().Fonts->AddFontDefault();
        g_ImGuiReady = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(w, h);
    io.MousePos = ImVec2(g_touchX, g_touchY);
    io.MouseDown[0] = g_touchDown.load();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 主窗口
    ImGui::Begin("IL2CPP Hook");
    ImGui::Text("状态: %s", snap.inMatch ? "对局中" : "大厅");
    ImGui::Text("ActorManager: 0x%lx", snap.x0);
    ImGui::Text("玩家数量: %d", (int)snap.players.size());
    if (ImGui::BeginTable("Players", 5, ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("等级");
        ImGui::TableSetupColumn("血量");
        ImGui::TableSetupColumn("蓝量");
        ImGui::TableSetupColumn("金币");
        ImGui::TableSetupColumn("小地图");
        ImGui::TableHeadersRow();
        for (const auto& p : snap.players) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", p.level);
            ImGui::TableNextColumn(); ImGui::Text("%d/%d", p.hp, p.maxHp);
            ImGui::TableNextColumn(); ImGui::Text("%d", p.mana);
            ImGui::TableNextColumn(); ImGui::Text("%d", p.gold);
            ImGui::TableNextColumn(); ImGui::Text("%.0f,%.0f", p.mapX, p.mapY);
        }
        ImGui::EndTable();
    }
    ImGui::End();

    // 雷达
    if (snap.inMatch && !snap.players.empty()) {
        ImGui::SetNextWindowPos(ImVec2(w - 250, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
        ImGui::Begin("雷达", nullptr, ImGuiWindowFlags_NoTitleBar);
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 sz = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRect(p0, ImVec2(p0.x+sz.x, p0.y+sz.y), IM_COL32(255,255,255,100));
        for (const auto& p : snap.players) {
            float nx = p.mapX / 10000.0f;
            float ny = p.mapY / 10000.0f;
            float rx = p0.x + nx * sz.x;
            float ry = p0.y + sz.y - ny * sz.y;
            if (nx>0 && nx<1 && ny>0 && ny<1)
                dl->AddCircleFilled(ImVec2(rx, ry), 5.0f, IM_COL32(255,0,0,255));
        }
        ImGui::End();
    }

    // 调试器
    DrawPointerDebugger(snap, w, h);

    ImGui::Render();
    glViewport(0, 0, w, h);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwap(dpy, surf);
}

// 延迟初始化线程
void* DelayedHookThread(void*) {
    // 等待 libil2cpp.so 加载
    uintptr_t base;
    while (!(base = GetModuleBase("libil2cpp.so"))) sleep(1);
    uintptr_t initAddr = base + 0x73507bc;
    uintptr_t clearAddr = base + 0x734bc10;
    LOGI("Base: 0x%lx, InitActor: 0x%lx, ClearActor: 0x%lx", base, initAddr, clearAddr);
    DobbyHook((void*)initAddr, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
    DobbyHook((void*)clearAddr, (void*)hook_ClearActor, (void**)&old_ClearActor);
    LOGI("Hooks installed");

    // Hook EGL 和 Input
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwap);
    void* inp = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (inp) DobbyHook(inp, (void*)hook_consume, (void**)&old_consume);

    // 启动数据线程
    std::thread(DataWorkerThread).detach();
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
