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
    uint32_t x0_to_addr1    = 32;   // 0x20
    uint32_t addr1_to_addr2 = 16;   // 0x10
    uint32_t item_to_addr3  = 24;   // 0x18
    uint32_t addr3_to_addr4 = 48;   // 0x30
    uint32_t addr3_to_addr5 = 256;  // 0x100
    uint32_t addr3_to_addr6 = 112;  // 0x70
    
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
    LOGI("[TRIGGER] InitActorParams (libil2cpp) Called! x0 = %p", x0);
    g_ActorManager.store((uintptr_t)x0);
    old_InitActorParams(x0, x1, x2, x3);
}

void (*old_ClearActor)(void* x0);
void hook_ClearActor(void* x0) {
    LOGI("[TRIGGER] ClearActor (libil2cpp) Called!");
    g_ActorManager.store(0);
    old_ClearActor(x0);
}

void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.55f, h * 0.8f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("指针断点排错器 (Pointer Debugger)")) {
        
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "对局初始化 Hook 触发次数: %d", g_InitActorTriggerCount.load());
        ImGui::Separator();

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
        }
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[IL2CPP] 基址: 0x%lx", g_GameBase);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[Hook] InitActor (RVA 0x73507bc) -> 实际地址: 0x%lx", g_HookAddr_InitActor);
        ImGui::Separator();

        const auto& d = snap.debug;
        const auto& off = snap.offsets;
        ImGui::TextColored(d.baseX0 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "[起点] InitActor x0: 0x%lx", d.baseX0);
        ImGui::TextColored(d.addr1 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), " └─ [+%d] 层级 1: 0x%lx", off.x0_to_addr1, d.addr1);
        ImGui::TextColored(d.addr2 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     └─ [+%d] 实体数组: 0x%lx", off.addr1_to_addr2, d.addr2);
        
        if (d.addr2 != 0) {
            for (int i = 0; i < 15; i++) {
                if (d.items[i].base == 0) continue; 
                char nodeName[64]; snprintf(nodeName, sizeof(nodeName), "英雄序号 [%d] - 内存: 0x%lx", i, d.items[i].base);
                if (ImGui::TreeNodeEx(nodeName, ImGuiTreeNodeFlags_Framed)) {
                    ImGui::TextColored(d.items[i].addr3 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), " └─ [+%d] 详细数据: 0x%lx", off.item_to_addr3, d.items[i].addr3);
                    if (d.items[i].addr3) {
                        ImGui::TextColored(d.items[i].addr4 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     ├─ [+%d] 属性区: 0x%lx", off.addr3_to_addr4, d.items[i].addr4);
                        ImGui::TextColored(d.items[i].addr5 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     ├─ [+%d] 坐标区: 0x%lx", off.addr3_to_addr5, d.items[i].addr5);
                        ImGui::TextColored(d.items[i].addr6 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     └─ [+%d] 雷达区: 0x%lx", off.addr3_to_addr6, d.items[i].addr6);
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::End();
}

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

void AutoFetchDeviceResolution() {
    FILE* pipe = popen("wm size", "r");
    if (!pipe) return;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        int w = 0, h = 0;
        if (sscanf(buffer, "Physical size: %dx%d", &w, &h) == 2 || sscanf(buffer, "Override size: %dx%d", &w, &h) == 2) {
            g_Touch.physW = (float)std::max(w, h); g_Touch.physH = (float)std::min(w, h); g_Touch.isSizeAutoDetected = true;
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

int hook_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int res = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    if (res == 0 && outEvent && *outEvent) handle_android_event((AInputEvent*)(*outEvent));
    return res;
}

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
        
        // 自动适配你的 OPPO 手机中文字体
        const char* systemFonts[] = {
            "/system/fonts/SysSans-Hans-Regular.ttf", // 你的日志中的 OPPO Sans
            "/system/fonts/NotoSansCJKjp-Regular.otc", 
            "/system/fonts/NotoSansSC-Regular.otf",
            "/system/fonts/DroidSansFallback.ttf"
        };
        for (int i = 0; i < 4; i++) {
            if (access(systemFonts[i], R_OK) == 0) {
                io.Fonts->AddFontFromFileTTF(systemFonts[i], 32.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                g_FontLoaded = true;
                LOGI("[*] 成功加载系统字体: %s", systemFonts[i]);
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
    ImGuiStyle& style = ImGui::GetStyle(); style = default_style;
    style.ScaleAllSizes(1.2f * finalScale); 
    style.WindowRounding = 12.0f; style.WindowMinSize = ImVec2(100.0f, 100.0f); 
    io.FontGlobalScale = 1.2f * finalScale;

    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
    if (g_Touch.down.load()) ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(0, 255, 0, 200));

    if (g_BaseWindowHeight == 0.0f) g_BaseWindowHeight = h * 0.45f; 
    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.6f, g_BaseWindowHeight), ImGuiCond_FirstUseEver);
    
    static bool show_pointer_debugger = true; 

    if (ImGui::Begin("全自动对局数据控制台 (Unity/IL2CPP版)")) {
        if (!g_FontLoaded) ImGui::TextColored(ImVec4(1,0,0,1), "警告: 无法读取字库，请检查 /system/fonts/ 权限!");
        
        if (ImGui::CollapsingHeader("基础设置 (校准与缩放)")) {
            ImGui::SliderFloat("菜单缩放", &g_UserUIScale, 0.5f, 2.0f);
            ImGui::Text("准星偏移 X:%.1f Y:%.1f", g_Touch.offsetX, g_Touch.offsetY);
            if (ImGui::Button("-10##x")) g_Touch.offsetX -= 10; ImGui::SameLine();
            if (ImGui::Button("+10##x")) g_Touch.offsetX += 10; ImGui::SameLine();
            if (ImGui::Button("-10##y")) g_Touch.offsetY -= 10; ImGui::SameLine();
            if (ImGui::Button("+10##y")) g_Touch.offsetY += 10;
        }
        
        ImGui::Separator();
        ImGui::Checkbox("显示断点排错窗 (Pointer Debugger)", &show_pointer_debugger);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "状态: %s", snapshot.inMatch ? "活跃对局中" : "大厅空闲");
        
        if (ImGui::BeginTable("PlayersTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("等级"); ImGui::TableSetupColumn("血量");
            ImGui::TableSetupColumn("蓝量"); ImGui::TableSetupColumn("金币");
            ImGui::TableSetupColumn("雷达"); ImGui::TableHeadersRow();
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

    ImGui::Render();
    GLint v[4]; glGetIntegerv(GL_VIEWPORT, v); 
    glDisable(GL_DEPTH_TEST); glViewport(0, 0, w, h); 
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glViewport(v[0], v[1], v[2], v[3]);
    return old_eglSwapBuffers(dpy, surface);
}

void* DelayedHookThread(void*) {
    // 1. 无延迟 Hook：渲染和输入必须秒挂，让菜单瞬间弹出
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    void* ins = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (ins) DobbyHook(ins, (void*)hook_consume, (void**)&old_consume);
    
    // 2. 启动数据更新死循环线程
    std::thread(DataWorkerThread).detach();

    // 3. 后台轮询：不卡主流程，慢慢等 libil2cpp.so 加载
    while (true) {
        uintptr_t base = GetModuleBase(GAME_MODULE);
        if (base) {
            g_GameBase = base;
            g_HookAddr_InitActor = base + 0x73507bc;
            g_HookAddr_ClearActor = base + 0x734bc10;
            DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
            DobbyHook((void*)g_HookAddr_ClearActor, (void*)hook_ClearActor, (void**)&old_ClearActor);
            LOGI("[*] libil2cpp.so 核心 Hook 已全部就绪!"); 
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
