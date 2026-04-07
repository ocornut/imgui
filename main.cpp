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
static std::string g_HookStatusMsg = "等待手动挂钩... (请在大厅/选区界面操作)";

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

static std::atomic<uintptr_t> g_CapturedX0{0};
static std::atomic<uintptr_t> g_CapturedX1{0};
static std::atomic<uintptr_t> g_CapturedX2{0};

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

// ==========================================
// 数据读取后台线程 (根据捕获的基址读取偏移)
// ==========================================
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
                            p.hp   = SafeRead<int32_t>(addr4 + currOff.prop_hp);
                            p.maxHp = SafeRead<int16_t>(addr4 + currOff.prop_maxhp);
                            p.level = SafeRead<int32_t>(addr4 + currOff.prop_level);
                        }

                        uintptr_t addr5 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr5);
                        newSnap.debug.items[i].addr5 = addr5;
                        if (addr5) {
                            p.worldX = SafeRead<float>(addr5 + 0x0);
                            p.worldZ = SafeRead<float>(addr5 + 0x4);
                            p.worldY = SafeRead<float>(addr5 + 0x8);
                            p.mana   = SafeRead<int32_t>(addr5 + currOff.prop_mana);
                        }

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

// ==========================================
// 底层 Linker 迭代器获取真实基址
// ==========================================
struct ModuleInfo {
    const char* name;
    uintptr_t base;
};

static int dl_iterate_callback(struct dl_phdr_info *info, size_t size, void *data) {
    ModuleInfo* modInfo = (ModuleInfo*)data;
    if (info->dlpi_name && strstr(info->dlpi_name, modInfo->name)) {
        modInfo->base = info->dlpi_addr;
        return 1;
    }
    return 0;
}

uintptr_t GetRealModuleBase(const char* module_name) {
    ModuleInfo info = {module_name, 0};
    dl_iterate_phdr(dl_iterate_callback, &info);
    if (info.base == 0) {
        FILE* f = fopen("/proc/self/maps", "r");
        if (f) {
            char line[512];
            while (fgets(line, sizeof(line), f)) {
                if (strstr(line, module_name)) {
                    uintptr_t start;
                    if (sscanf(line, "%lx", &start) == 1) { info.base = start; break; }
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
    
    // 捕获寄存器
    if (x0 != nullptr) g_CapturedX0.store((uintptr_t)x0);
    if (x1 != nullptr) g_CapturedX1.store((uintptr_t)x1);
    if (x2 != nullptr) g_CapturedX2.store((uintptr_t)x2);
    
    // 直接使用 x0 作为基址来源，传给读取线程
    if (x0 != nullptr && (uintptr_t)x0 > 0x10000000) { 
        g_ActorManager.store((uintptr_t)x0);
        
        // 降低刷屏频率
        if (g_InitCallCount.load() % 5 == 1) { 
            LOGI("[+] 拦截到实体生成! 捕获基址 x0: 0x%lx", (uintptr_t)x0);
        }
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
        g_HookStatusMsg = "未找到 libil2cpp.so，游戏可能未加载完全！";
        LOGE("[-] %s", g_HookStatusMsg.c_str());
        return;
    }
    
    g_GameBase = base;
    g_HookAddr_InitActor = base + 0x73507bc;

    LOGI("[*] 准备硬核挂钩! 真实基址: 0x%lx, 目标: 0x%lx", base, g_HookAddr_InitActor);

    // 强制解锁内存页权限
    void* page_start = (void*)(g_HookAddr_InitActor & ~0xFFF);
    mprotect(page_start, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);

    // 执行 Dobby Hook (无视内存读取报错)
    int ret = DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
    
    // 清理 CPU 指令缓存，强制生效
    __builtin___clear_cache((char*)g_HookAddr_InitActor, (char*)(g_HookAddr_InitActor + 16));

    if (ret == 0) {
        g_IsHookInstalled = true;
        g_HookStatusMsg = "✅ 核心 Hook 注入成功！请进入对局。";
        LOGI("[+] 手动安装 Hook 成功！");
    } else {
        g_HookStatusMsg = "❌ 挂钩失败，Dobby 错误码: " + std::to_string(ret);
        LOGE("[-] %s", g_HookStatusMsg.c_str());
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

    GameSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(g_SnapshotMutex);
        snapshot = g_CurrentSnapshot;
    }

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

    if (ImGui::Begin("IL2CPP 透视框架 (防闪退正式版)")) {
        
        // --- 顶部控制台 ---
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
        ImGui::Text("状态: %s", g_HookStatusMsg.c_str());
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Separator();

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
        
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "状态: %s", snapshot.inMatch ? "局内" : "未获取到对象");

        // --- 英雄数据读取表 ---
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

    // --- 断点排错器面板 ---
    if (show_pointer_debugger) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(w * 0.55f, h * 0.8f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("指针断点排错器")) {
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
            
            const auto& d = snapshot.debug;
            const auto& off = snapshot.offsets;
            ImGui::TextColored(d.baseX0 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "[起点] 捕获到的 x0: 0x%lx", d.baseX0);
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
            ImGui::Separator();
            if (ImGui::CollapsingHeader("实时运行日志", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextColored(g_InitCallCount > 0 ? ImVec4(0, 1, 0, 1) : ImVec4(1, 1, 0, 1), 
                    "> 当前成功拦截次数: %d", g_InitCallCount.load());
                ImGui::BeginChild("LogRegion", ImVec2(0, 120), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                std::lock_guard<std::mutex> lock(g_LogMutex);
                for (const auto& log : g_AppLogs) { ImGui::TextUnformatted(log.c_str()); }
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
                ImGui::EndChild();
            }
        }
        ImGui::End();
    }

    // --- 雷达绘制 ---
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

    // 启动数据读取线程，负责用捕获到的 x0 去读取各级偏移
    std::thread(DataWorkerThread).detach();

    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
