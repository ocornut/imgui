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

#define TAG "IL2CPP_Drive"
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
static float g_UserUIScale = 1.0f;

static uintptr_t g_GameBase = 0;
static uintptr_t g_HookAddr_InitActor = 0;
static uintptr_t g_HookAddr_ClearActor = 0;

struct DynamicOffsets {
    // 指针链偏移
    uint32_t x0_to_addr1    = 0x20;
    uint32_t addr1_to_addr2 = 0x10;
    uint32_t item_to_addr3  = 0x18;
    uint32_t addr3_to_addr4 = 0x30;
    uint32_t addr3_to_addr5 = 0x100;
    uint32_t addr3_to_addr6 = 0x70;

    // 属性偏移 (addr4 内)
    uint32_t prop_gold  = 0x20;
    uint32_t prop_hp    = 0x30;
    uint32_t prop_maxhp = 0x32;   // 注意：可能是 int16
    uint32_t prop_level = 0x50;

    // 世界坐标区内偏移 (addr5 内)
    uint32_t prop_mana  = 0x28;   // 蓝量偏移
    // 世界坐标 XYZ 固定 0,4,8
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

// 后台数据读取线程
void DataWorkerThread() {
    while (true) {
        GameSnapshot newSnap;
        DynamicOffsets currOff;
        {
            std::lock_guard<std::mutex> lock(g_OffsetMutex);
            currOff = g_Offsets;
        }
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
                    // 遍历玩家数组（每个槽位8字节指针，最大15个）
                    for (int i = 0; i < 15; ++i) {
                        uintptr_t item = SafeRead<uintptr_t>(addr2 + (i * 0x8));
                        newSnap.debug.items[i].base = item;
                        if (!item) continue;

                        uintptr_t addr3 = SafeRead<uintptr_t>(item + currOff.item_to_addr3);
                        newSnap.debug.items[i].addr3 = addr3;
                        if (!addr3) continue;

                        PlayerData p;

                        // 属性区 addr4
                        uintptr_t addr4 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr4);
                        newSnap.debug.items[i].addr4 = addr4;
                        if (addr4) {
                            p.gold = SafeRead<int32_t>(addr4 + currOff.prop_gold);
                            p.hp   = SafeRead<int32_t>(addr4 + currOff.prop_hp);
                            // maxhp 偏移 0x32，可能是 int16，安全读取后转为 int32
                            p.maxHp = SafeRead<int16_t>(addr4 + currOff.prop_maxhp);
                            p.level = SafeRead<int32_t>(addr4 + currOff.prop_level);
                        }

                        // 世界坐标区 addr5
                        uintptr_t addr5 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr5);
                        newSnap.debug.items[i].addr5 = addr5;
                        if (addr5) {
                            p.worldX = SafeRead<float>(addr5 + 0x0);
                            p.worldZ = SafeRead<float>(addr5 + 0x4);
                            p.worldY = SafeRead<float>(addr5 + 0x8);
                            p.mana   = SafeRead<int32_t>(addr5 + currOff.prop_mana);
                        }

                        // 小地图坐标区 addr6
                        uintptr_t addr6 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr6);
                        newSnap.debug.items[i].addr6 = addr6;
                        if (addr6) {
                            p.mapX = SafeRead<float>(addr6 + 0x50);
                            p.mapY = SafeRead<float>(addr6 + 0x54);
                        }

                        if (p.maxHp > 0 && p.maxHp < 50000)
                            newSnap.players.push_back(p);
                    }
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(g_SnapshotMutex);
            g_CurrentSnapshot = newSnap;
        }
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

// ========== 修改点：Hook 函数改为单参数 ==========
void (*old_InitActorParams)(void* x0);   // 原只有1个参数
void hook_InitActorParams(void* x0) {
    g_ActorManager.store((uintptr_t)x0);
    LOGI("[InitActor] x0 = %p", x0);
    old_InitActorParams(x0);              // 调用时只传 x0
}

void (*old_ClearActor)(void* x0);
void hook_ClearActor(void* x0) {
    g_ActorManager.store(0);
    LOGI("[ClearActor] Game ended");
    old_ClearActor(x0);
}
// ========== 修改结束 ==========

// 指针调试器窗口（与原代码类似，但偏移面板已适配新偏移）
void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.55f, h * 0.8f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("指针断点排错器 (IL2CPP)")) {
        if (ImGui::CollapsingHeader("动态偏移修改面板 (十进制)", ImGuiTreeNodeFlags_DefaultOpen)) {
            std::lock_guard<std::mutex> lock(g_OffsetMutex);
            auto drawOffsetBtn = [](const char* label, uint32_t& val) {
                ImGui::PushID(&val);
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s -> 当前: %d (0x%X)", label, val, val);
                if (ImGui::Button("-8")) { if (val >= 8) val -= 8; else val = 0; } ImGui::SameLine();
                if (ImGui::Button("-4")) { if (val >= 4) val -= 4; else val = 0; } ImGui::SameLine();
                if (ImGui::Button("-1")) { if (val >= 1) val -= 1; else val = 0; } ImGui::SameLine();
                if (ImGui::Button("+1")) val += 1; ImGui::SameLine();
                if (ImGui::Button("+4")) val += 4; ImGui::SameLine();
                if (ImGui::Button("+8")) val += 8;
                ImGui::Separator();
                ImGui::PopID();
            };
            drawOffsetBtn("地址1 (x0 + ?)", g_Offsets.x0_to_addr1);
            drawOffsetBtn("英雄数组 (addr1 + ?)", g_Offsets.addr1_to_addr2);
            drawOffsetBtn("实体数据 (item + ?)", g_Offsets.item_to_addr3);
            drawOffsetBtn("血量属性 (addr3 + ?)", g_Offsets.addr3_to_addr4);
            drawOffsetBtn("坐标数据 (addr3 + ?)", g_Offsets.addr3_to_addr5);
            drawOffsetBtn("雷达坐标 (addr3 + ?)", g_Offsets.addr3_to_addr6);
            ImGui::Separator();
            drawOffsetBtn("金币偏移", g_Offsets.prop_gold);
            drawOffsetBtn("血量偏移", g_Offsets.prop_hp);
            drawOffsetBtn("最大血量偏移", g_Offsets.prop_maxhp);
            drawOffsetBtn("等级偏移", g_Offsets.prop_level);
            drawOffsetBtn("蓝量偏移", g_Offsets.prop_mana);
        }
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[核心库] %s 基址: 0x%lx", GAME_MODULE, g_GameBase);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[Hook 1] InitActor (RVA 0x73507bc) -> 0x%lx", g_HookAddr_InitActor);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[Hook 2] ClearActor (RVA 0x734bc10) -> 0x%lx", g_HookAddr_ClearActor);
        ImGui::Separator();

        const auto& d = snap.debug;
        const auto& off = snap.offsets;
        ImGui::TextColored(d.baseX0 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "[起点] InitActor x0: 0x%lx", d.baseX0);
        ImGui::TextColored(d.addr1 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), " └─ [+%d] 地址 1: 0x%lx", off.x0_to_addr1, d.addr1);
        ImGui::TextColored(d.addr2 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     └─ [+%d] 英雄数组: 0x%lx", off.addr1_to_addr2, d.addr2);
        if (d.addr2 != 0) {
            for (int i = 0; i < 15; i++) {
                if (d.items[i].base == 0) continue;
                char nodeName[64];
                snprintf(nodeName, sizeof(nodeName), "数组序号 [%d] - 0x%lx", i, d.items[i].base);
                if (ImGui::TreeNodeEx(nodeName, ImGuiTreeNodeFlags_Framed)) {
                    ImGui::TextColored(d.items[i].addr3 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), " └─ [+%d] 实体数据: 0x%lx", off.item_to_addr3, d.items[i].addr3);
                    if (d.items[i].addr3) {
                        ImGui::TextColored(d.items[i].addr4 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     ├─ [+%d] 属性区: 0x%lx", off.addr3_to_addr4, d.items[i].addr4);
                        ImGui::TextColored(d.items[i].addr5 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     ├─ [+%d] 坐标区: 0x%lx", off.addr3_to_addr5, d.items[i].addr5);
                        ImGui::TextColored(d.items[i].addr6 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     └─ [+%d] 小地图: 0x%lx", off.addr3_to_addr6, d.items[i].addr6);
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::End();
}

// 触摸及分辨率辅助
void AutoFetchDeviceResolution() {
    FILE* pipe = popen("wm size", "r");
    if (!pipe) return;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        int w = 0, h = 0;
        if (sscanf(buffer, "Physical size: %dx%d", &w, &h) == 2 || sscanf(buffer, "Override size: %dx%d", &w, &h) == 2) {
            g_Touch.physW = (float)std::max(w, h);
            g_Touch.physH = (float)std::min(w, h);
            g_Touch.isSizeAutoDetected = true;
        }
    }
    pclose(pipe);
}

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

// InputConsumer Hook
typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

int hook_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int res = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    if (res == 0 && outEvent && *outEvent) handle_android_event((AInputEvent*)(*outEvent));
    return res;
}

// eglSwapBuffers Hook
typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w, h;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    if (w <= 0 || h <= 0) return old_eglSwapBuffers(dpy, surface);
    g_Touch.renderW = (float)w;
    g_Touch.renderH = (float)h;

    GameSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(g_SnapshotMutex);
        snapshot = g_CurrentSnapshot;
    }

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGuiIO& io = ImGui::GetIO();

        // 加载中文字体（根据你的系统字体路径）
        const char* systemFonts[] = {
            "/system/fonts/SysSans-Hans-Regular.ttf",
            "/system/fonts/NotoSansCJKjp-Regular.otc",
            "/system/fonts/NotoSansSC-Regular.otf",
            "/system/fonts/DroidSansFallback.ttf"
        };
        for (int i = 0; i < 4; i++) {
            if (access(systemFonts[i], R_OK) == 0) {
                io.Fonts->AddFontFromFileTTF(systemFonts[i], 32.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                g_FontLoaded = true;
                LOGI("[*] 加载字体成功: %s", systemFonts[i]);
                break;
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

    static ImGuiStyle default_style = ImGui::GetStyle();
    ImGuiStyle& style = ImGui::GetStyle();
    style = default_style;
    style.ScaleAllSizes(1.2f * finalScale);
    style.WindowRounding = 12.0f;
    style.WindowMinSize = ImVec2(100.0f, 100.0f);
    io.FontGlobalScale = 1.2f * finalScale;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_Touch.down.load())
        ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(0, 255, 0, 200));

    if (g_BaseWindowHeight == 0.0f) g_BaseWindowHeight = h * 0.45f;
    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.6f, g_BaseWindowHeight), ImGuiCond_FirstUseEver);

    static bool show_pointer_debugger = true;
    static bool show_radar = true;

    if (ImGui::Begin("IL2CPP 透视框架 (偏移可调)")) {
        if (!g_FontLoaded) ImGui::TextColored(ImVec4(1,0,0,1), "警告: 无法加载系统字库，将显示为问号!");
        float currentHeight = ImGui::GetWindowHeight();
        g_DynamicScale = currentHeight / g_BaseWindowHeight;
        if (g_DynamicScale < 0.4f) g_DynamicScale = 0.4f;
        if (g_DynamicScale > 3.0f) g_DynamicScale = 3.0f;

        if (ImGui::CollapsingHeader("基础校准 (触控与缩放)")) {
            ImGui::SliderFloat("菜单缩放", &g_UserUIScale, 0.5f, 2.0f);
            ImGui::Text("坐标偏移 X:%.1f Y:%.1f", g_Touch.offsetX, g_Touch.offsetY);
            if (ImGui::Button("-10##x")) g_Touch.offsetX -= 10; ImGui::SameLine();
            if (ImGui::Button("+10##x")) g_Touch.offsetX += 10; ImGui::SameLine();
            if (ImGui::Button("-10##y")) g_Touch.offsetY -= 10; ImGui::SameLine();
            if (ImGui::Button("+10##y")) g_Touch.offsetY += 10;
        }
        ImGui::Separator();
        ImGui::Checkbox("雷达 (Radar)", &show_radar); ImGui::SameLine();
        ImGui::Checkbox("指针断点调试", &show_pointer_debugger);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "状态: %s", snapshot.inMatch ? "局内" : "大厅");

        if (ImGui::BeginTable("PlayersTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("等级"); ImGui::TableSetupColumn("血量");
            ImGui::TableSetupColumn("蓝量"); ImGui::TableSetupColumn("金币");
            ImGui::TableSetupColumn("雷达坐标"); ImGui::TableHeadersRow();
            for (const auto& p : snapshot.players) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("Lv.%d", p.level);
                ImGui::TableNextColumn(); ImGui::Text("%d/%d", p.hp, p.maxHp);
                ImGui::TableNextColumn(); ImGui::Text("%d", p.mana);
                ImGui::TableNextColumn(); ImGui::Text("%d", p.gold);
                ImGui::TableNextColumn(); ImGui::Text("%.0f, %.0f", p.mapX, p.mapY);
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();

    if (show_pointer_debugger) DrawPointerDebugger(snapshot, w, h);
    if (show_radar && snapshot.inMatch) {
        ImGui::SetNextWindowPos(ImVec2(w - 350, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
        if (ImGui::Begin("Radar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImVec2 size = ImGui::GetContentRegionAvail();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRect(p0, ImVec2(p0.x + size.x, p0.y + size.y), IM_COL32(255, 255, 255, 100));
            for (const auto& p : snapshot.players) {
                float nX = p.mapX / 10000.0f;
                float nY = p.mapY / 10000.0f;
                float rX = p0.x + (nX * size.x);
                float rY = p0.y + size.y - (nY * size.y);
                if (nX > 0 && nX < 1 && nY > 0 && nY < 1)
                    dl->AddCircleFilled(ImVec2(rX, rY), 5.0f * finalScale, IM_COL32(255, 0, 0, 255));
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    ImGui::Render();
    GLint v[4];
    glGetIntegerv(GL_VIEWPORT, v);
    GLint d;
    glGetIntegerv(GL_DEPTH_TEST, &d);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, w, h);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glViewport(v[0], v[1], v[2], v[3]);
    if (d) glEnable(GL_DEPTH_TEST);

    return old_eglSwapBuffers(dpy, surface);
}

// 延迟 Hook 线程
void* DelayedHookThread(void*) {
    // 1. 立即 Hook 渲染和输入，保证菜单能显示
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    void* ins = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (ins) DobbyHook(ins, (void*)hook_consume, (void**)&old_consume);

    // 2. 启动数据线程（后台读取）
    std::thread(DataWorkerThread).detach();

    // 3. 等待 libil2cpp.so 加载，然后 Hook 游戏函数
    while (true) {
        uintptr_t base = GetModuleBase(GAME_MODULE);
        if (base) {
            g_GameBase = base;
            g_HookAddr_InitActor = base + 0x73507bc;
            g_HookAddr_ClearActor = base + 0x734bc10;
            LOGI("[*] libil2cpp.so base = 0x%lx", base);
            LOGI("[*] InitActor addr = 0x%lx", g_HookAddr_InitActor);
            LOGI("[*] ClearActor addr = 0x%lx", g_HookAddr_ClearActor);

            int ret1 = DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
            int ret2 = DobbyHook((void*)g_HookAddr_ClearActor, (void*)hook_ClearActor, (void**)&old_ClearActor);
            LOGI("[*] Hook InitActor result: %d, ClearActor result: %d", ret1, ret2);
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
