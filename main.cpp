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

// 第三方库
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "IL2CPP_Drive"
#define GAME_MODULE "libil2cpp.so"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

// ===================== [1. 针对 ARM64 的寄存器定义] =====================
#ifndef RegisterContext
typedef struct {
    union {
        uint64_t x[29];
        struct { uint64_t x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22,x23,x24,x25,x26,x27,x28; };
    } general;
    uint64_t fp; uint64_t lr; uint64_t sp; uint64_t pc;
} RegisterContext;
#endif

// ===================== [2. 数据结构定义] =====================

struct DynamicOffsets {
    uint32_t x0_to_addr1 = 0x20; uint32_t addr1_to_addr2 = 0x10;
    uint32_t item_to_addr3 = 0x18; uint32_t addr3_to_addr4 = 0x30;
    uint32_t addr3_to_addr5 = 0x100; uint32_t addr3_to_addr6 = 0x70;
    uint32_t prop_gold = 0x20; uint32_t prop_hp = 0x30;
    uint32_t prop_maxhp = 0x32; uint32_t prop_level = 0x50;
    uint32_t prop_mana = 0x28;
};

struct PlayerData {
    int hp = 0, maxHp = 0, mana = 0, gold = 0, level = 0;
    float worldX = 0, worldY = 0, worldZ = 0;
    float mapX = 0, mapY = 0;
};

struct DebugChain {
    uintptr_t baseX0 = 0, addr1 = 0, addr2 = 0;
    struct Item { uintptr_t base = 0, addr3 = 0, addr4 = 0, addr5 = 0, addr6 = 0; } items[15];
};

struct GameSnapshot {
    bool inMatch = false;
    DynamicOffsets offsets;
    DebugChain debug;
    std::vector<PlayerData> players;
};

// ===================== [3. 前置声明] =====================
void handle_android_event(AInputEvent* event);
void DrawPointerDebugger(const GameSnapshot& snap, float w, float h);
void DrawRadar(const GameSnapshot& snap, float w, float h, float finalScale);
void instrument_InitActorParams(RegisterContext *ctx, const HookEntryInfo *info);
void instrument_ClearActor(RegisterContext *ctx, const HookEntryInfo *info);
uintptr_t GetModuleBase(const char* module_name);

// ===================== [4. 全局变量] =====================
static bool g_Initialized = false, g_FontLoaded = false;
static int g_MemFd = -1;
static uintptr_t g_GameBase = 0, g_HookAddr_InitActor = 0, g_HookAddr_ClearActor = 0;
static float g_DynamicScale = 1.0f, g_BaseWindowHeight = 0.0f, g_UserUIScale = 1.0f;

struct {
    float x = 0.0f, y = 0.0f; std::atomic<bool> down{false};
    float renderW = 0.0f, renderH = 0.0f; float physW = 3392.0f, physH = 2400.0f;
    float offsetX = 0.0f, offsetY = 0.0f;
} g_Touch;

DynamicOffsets g_Offsets;
std::mutex g_OffsetMutex, g_SnapshotMutex;
GameSnapshot g_CurrentSnapshot;
std::atomic<uintptr_t> g_ActorManager{0};

// ===================== [5. 内存读取与数据线程] =====================

template <typename T>
T SafeRead(uintptr_t address) {
    T value{};
    if (address < 0x10000000 || address > 0x00007FFFFFFFFFFF) return value;
    if (g_MemFd == -1) g_MemFd = open("/proc/self/mem", O_RDONLY);
    if (g_MemFd >= 0) pread(g_MemFd, &value, sizeof(T), address);
    return value;
}

void DataWorkerThread() {
    while (true) {
        GameSnapshot newSnap;
        { std::lock_guard<std::mutex> lock(g_OffsetMutex); newSnap.offsets = g_Offsets; }
        uintptr_t baseX0 = g_ActorManager.load();
        if (baseX0 != 0) {
            newSnap.inMatch = true; newSnap.debug.baseX0 = baseX0;
            uintptr_t a1 = SafeRead<uintptr_t>(baseX0 + newSnap.offsets.x0_to_addr1);
            newSnap.debug.addr1 = a1;
            if (a1) {
                uintptr_t a2 = SafeRead<uintptr_t>(a1 + newSnap.offsets.addr1_to_addr2);
                newSnap.debug.addr2 = a2;
                if (a2) {
                    for (int i = 0; i < 15; ++i) {
                        uintptr_t item = SafeRead<uintptr_t>(a2 + (i * 0x8));
                        newSnap.debug.items[i].base = item; if (!item) continue;
                        uintptr_t a3 = SafeRead<uintptr_t>(item + newSnap.offsets.item_to_addr3);
                        newSnap.debug.items[i].addr3 = a3; if (!a3) continue;

                        PlayerData p;
                        uintptr_t a4 = SafeRead<uintptr_t>(a3 + newSnap.offsets.addr3_to_addr4);
                        newSnap.debug.items[i].addr4 = a4;
                        if (a4) {
                            p.gold = SafeRead<int32_t>(a4 + newSnap.offsets.prop_gold);
                            p.hp = SafeRead<int32_t>(a4 + newSnap.offsets.prop_hp);
                            p.maxHp = (int)SafeRead<int16_t>(a4 + newSnap.offsets.prop_maxhp);
                            p.level = SafeRead<int32_t>(a4 + newSnap.offsets.prop_level);
                        }
                        uintptr_t a5 = SafeRead<uintptr_t>(a3 + newSnap.offsets.addr3_to_addr5);
                        newSnap.debug.items[i].addr5 = a5;
                        if (a5) {
                            p.worldX = SafeRead<float>(a5 + 0x0); p.worldZ = SafeRead<float>(a5 + 0x4); p.worldY = SafeRead<float>(a5 + 0x8);
                            p.mana = SafeRead<int32_t>(a5 + newSnap.offsets.prop_mana);
                        }
                        uintptr_t a6 = SafeRead<uintptr_t>(a3 + newSnap.offsets.addr3_to_addr6);
                        newSnap.debug.items[i].addr6 = a6;
                        if (a6) { p.mapX = SafeRead<float>(a6 + 0x50); p.mapY = SafeRead<float>(a6 + 0x54); }
                        if (p.maxHp > 0) newSnap.players.push_back(p);
                    }
                }
            }
        }
        { std::lock_guard<std::mutex> lock(g_SnapshotMutex); g_CurrentSnapshot = newSnap; }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

// ===================== [6. 绘制逻辑 (找回被删的功能)] =====================

void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowSize(ImVec2(w * 0.5f, h * 0.7f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("指针断点排错器 (IL2CPP)")) {
        if (ImGui::CollapsingHeader("动态偏移修改面板", ImGuiTreeNodeFlags_DefaultOpen)) {
            std::lock_guard<std::mutex> lock(g_OffsetMutex);
            auto btn = [](const char* l, uint32_t& v) {
                ImGui::PushID(&v); ImGui::Text("%s: %d (0x%X)", l, v, v); ImGui::SameLine();
                if(ImGui::Button("-1")) v--; ImGui::SameLine(); if(ImGui::Button("+1")) v++; ImGui::PopID();
            };
            btn("Addr1", g_Offsets.x0_to_addr1); btn("Array", g_Offsets.addr1_to_addr2);
            btn("Entity", g_Offsets.item_to_addr3); btn("Attr", g_Offsets.addr3_to_addr4);
        }
        ImGui::Separator();
        ImGui::Text("BaseX0: 0x%lx", snap.debug.baseX0);
        // ... (此处可以遍历 snap.debug.items 显示更多地址)
    }
    ImGui::End();
}

void DrawRadar(const GameSnapshot& snap, float w, float h, float finalScale) {
    ImGui::SetNextWindowPos(ImVec2(w - 320, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 300));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.4f));
    if (ImGui::Begin("Radar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        ImVec2 p0 = ImGui::GetCursorScreenPos(); ImVec2 sz = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRect(p0, ImVec2(p0.x + sz.x, p0.y + sz.y), IM_COL32(255,255,255,100));
        for (const auto& p : snap.players) {
            float nX = p.mapX / 10000.0f; float nY = p.mapY / 10000.0f;
            float rX = p0.x + (nX * sz.x); float rY = p0.y + sz.y - (nY * sz.y);
            if (nX > 0 && nX < 1 && nY > 0 && nY < 1)
                dl->AddCircleFilled(ImVec2(rX, rY), 6.0f * finalScale, IM_COL32(255,0,0,200));
        }
    }
    ImGui::End(); ImGui::PopStyleColor();
}

// ===================== [7. Hook & Event 逻辑] =====================

void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float scX = g_Touch.renderW / g_Touch.physW; float scY = g_Touch.renderH / g_Touch.physH;
        g_Touch.x = (AMotionEvent_getX(event, 0) * scX) + g_Touch.offsetX;
        g_Touch.y = (AMotionEvent_getY(event, 0) * scY) + g_Touch.offsetY;
        if (action == AMOTION_EVENT_ACTION_DOWN) g_Touch.down.store(true);
        else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) g_Touch.down.store(false);
    }
}

typedef int (*p_consume)(void*, void*, bool, int64_t, uint32_t*, void**);
static p_consume old_consume = nullptr;
int hook_consume(void* a, void* b, bool c, int64_t d, uint32_t* e, void** f) {
    int res = old_consume(a,b,c,d,e,f);
    if (res == 0 && f && *f) handle_android_event((AInputEvent*)*f);
    return res;
}

typedef EGLBoolean (*p_swap)(EGLDisplay, EGLSurface);
static p_swap old_swap = nullptr;
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    EGLint w, h; eglQuerySurface(dpy, surf, EGL_WIDTH, &w); eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    if (w <= 0 || h <= 0) return old_swap(dpy, surf);
    g_Touch.renderW = (float)w; g_Touch.renderH = (float)h;

    if (!g_Initialized) {
        ImGui::CreateContext(); ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGuiIO& io = ImGui::GetIO();
        const char* fonts[] = {"/system/fonts/NotoSansCJKjp-Regular.otc", "/system/fonts/DroidSansFallback.ttf"};
        for (auto f : fonts) { if(access(f, R_OK)==0) { io.Fonts->AddFontFromFileTTF(f, 32.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon()); g_FontLoaded=true; break; }}
        g_Initialized = true;
    }

    GameSnapshot snap; { std::lock_guard<std::mutex> lock(g_SnapshotMutex); snap = g_CurrentSnapshot; }
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y); io.MouseDown[0] = g_Touch.down.load();

    float finalScale = ((float)h / 1000.0f) * g_DynamicScale * g_UserUIScale;
    ImGui::GetStyle().ScaleAllSizes(finalScale); io.FontGlobalScale = finalScale;

    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();

    static bool show_db = true, show_rd = true;
    if (ImGui::Begin("IL2CPP 菜单 (满血版)")) {
        ImGui::Checkbox("调试器", &show_db); ImGui::SameLine(); ImGui::Checkbox("雷达", &show_rd);
        ImGui::SliderFloat("UI 缩放", &g_UserUIScale, 0.5f, 2.0f);
        if (ImGui::BeginTable("PT", 4, ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("等级"); ImGui::TableSetupColumn("血量"); ImGui::TableSetupColumn("金币"); ImGui::TableSetupColumn("坐标"); ImGui::TableHeadersRow();
            for (const auto& p : snap.players) {
                ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::Text("Lv.%d", p.level);
                ImGui::TableNextColumn(); ImGui::Text("%d/%d", p.hp, p.maxHp);
                ImGui::TableNextColumn(); ImGui::Text("%d", p.gold);
                ImGui::TableNextColumn(); ImGui::Text("%.0f,%.0f", p.mapX, p.mapY);
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();

    if(show_db) DrawPointerDebugger(snap, w, h);
    if(show_rd && snap.inMatch) DrawRadar(snap, w, h, finalScale);

    ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return old_swap(dpy, surf);
}

// ===================== [8. 核心入口] =====================

void instrument_InitActorParams(RegisterContext *ctx, const HookEntryInfo *info) {
    g_ActorManager.store(ctx->general.x[0]);
}

void instrument_ClearActor(RegisterContext *ctx, const HookEntryInfo *info) {
    g_ActorManager.store(0);
}

uintptr_t GetModuleBase(const char* m) {
    uintptr_t b = 0; FILE* f = fopen("/proc/self/maps", "r"); if(!f) return 0;
    char l[512]; while(fgets(l, 512, f)) { if(strstr(l, m)) { sscanf(l, "%lx", &b); break; } }
    fclose(f); return b;
}

void* DelayedHookThread(void*) {
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_swap);
    void* ins = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (ins) DobbyHook(ins, (void*)hook_consume, (void**)&old_consume);

    std::thread(DataWorkerThread).detach();
    while (true) {
        uintptr_t b = GetModuleBase(GAME_MODULE);
        if (b) {
            g_GameBase = b;
            DobbyInstrument((void*)(b + 0x73507bc), instrument_InitActorParams);
            DobbyInstrument((void*)(b + 0x734bc10), instrument_ClearActor);
            break;
        }
        sleep(1);
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
