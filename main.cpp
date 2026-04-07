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
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "IL2CPP_Drive"
#define GAME_MODULE "libil2cpp.so"

// --- 实时日志与计数器系统 ---
std::atomic<int> g_InitCallCount{0};
std::vector<std::string> g_AppLogs;
std::mutex g_LogMutex;

void AddLog(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    // 输出到安卓标准日志
    __android_log_print(ANDROID_LOG_INFO, TAG, "%s", buf);
    
    // 同时输出到 ImGui 悬浮窗
    std::lock_guard<std::mutex> lock(g_LogMutex);
    if (g_AppLogs.size() > 30) g_AppLogs.erase(g_AppLogs.begin());
    g_AppLogs.push_back(buf);
}

#undef LOGI
#undef LOGE
#define LOGI(...) AddLog(__VA_ARGS__)
#define LOGE(...) AddLog("[ERROR] " __VA_ARGS__)
// ----------------------------------

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

// --- 寄存器动态切换器与状态保存 ---
static int g_RootRegister = 0; // 0=x0, 1=x1, 2=x2
static std::atomic<uintptr_t> g_CapturedX0{0};
static std::atomic<uintptr_t> g_CapturedX1{0};
static std::atomic<uintptr_t> g_CapturedX2{0};
// ----------------------------------------

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

// 辅助函数：将内存读取为 Hex 字符串用于排错
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

// 唯一 Hook 目标：InitActor
typedef void (*t_InitActorParams)(void* x0, void* x1, void* x2, void* x3, void* x4, void* x5, void* x6, void* x7);
t_InitActorParams old_InitActorParams = nullptr;

void hook_InitActorParams(void* x0, void* x1, void* x2, void* x3, void* x4, void* x5, void* x6, void* x7) {
    g_InitCallCount++; 
    
    if (x0 != nullptr) g_CapturedX0.store((uintptr_t)x0);
    if (x1 != nullptr) g_CapturedX1.store((uintptr_t)x1);
    if (x2 != nullptr) g_CapturedX2.store((uintptr_t)x2);
    
    uintptr_t target_ptr = 0;
    if (g_RootRegister == 0) target_ptr = (uintptr_t)x0;
    else if (g_RootRegister == 1) target_ptr = (uintptr_t)x1;
    else target_ptr = (uintptr_t)x2;
    
    if (target_ptr != 0 && target_ptr > 0x10000000) { 
        g_ActorManager.store(target_ptr);
        LOGI("[Hook-Init] 拦截到了! 次数:%d, 设定的基址: 0x%lx", g_InitCallCount.load(), target_ptr);
    }
    
    if (old_InitActorParams) {
        old_InitActorParams(x0, x1, x2, x3, x4, x5, x6, x7);
    }
}

void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.55f, h * 0.8f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("指针断点排错器 (IL2CPP)")) {
        
        if (ImGui::CollapsingHeader("拦截寄存器选择 (基址来源)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "当前捕获的值:");
            ImGui::Text("x0 (通常是 this): 0x%lx", g_CapturedX0.load());
            ImGui::Text("x1 (通常是 参数1): 0x%lx", g_CapturedX1.load());
            ImGui::Text("x2 (MethodInfo):  0x%lx", g_CapturedX2.load());
            ImGui::Separator();
            ImGui::Text("动态切换起点 (改完后下次生成英雄生效，或立即生效):");
            if (ImGui::RadioButton("使用 x0", &g_RootRegister, 0)) g_ActorManager.store(g_CapturedX0.load()); ImGui::SameLine();
            if (ImGui::RadioButton("使用 x1", &g_RootRegister, 1)) g_ActorManager.store(g_CapturedX1.load()); ImGui::SameLine();
            if (ImGui::RadioButton("使用 x2", &g_RootRegister, 2)) g_ActorManager.store(g_CapturedX2.load());
        }
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
        ImGui::Separator();

        if (ImGui::CollapsingHeader("实时运行日志 (排错必看)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextColored(g_InitCallCount > 0 ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), 
                "> InitActor 函数被拦截次数: %d", g_InitCallCount.load());
            
            if (g_InitCallCount == 0) {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "[状态] 正在等待进入对局...\n提示: 因为 InitActor 只在生成实体时触发，大厅为0是正常现象。\n请进入对局/训练营。如果进对局后仍为0，则说明地址找错了。");
            } else {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "[状态] 成功拦截！请根据需要在上方切换使用 x0/x1/x2 作为基址测试数据。");
            }

            ImGui::BeginChild("LogRegion", ImVec2(0, 120), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            std::lock_guard<std::mutex> lock(g_LogMutex);
            for (const auto& log : g_AppLogs) {
                ImGui::TextUnformatted(log.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();
        }
        ImGui::Separator();

        const auto& d = snap.debug;
        const auto& off = snap.offsets;
        ImGui::TextColored(d.baseX0 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "[起点] InitActor x0/x1/x2: 0x%lx", d.baseX0);
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

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

int hook_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int res = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    if (res == 0 && outEvent && *outEvent) handle_android_event((AInputEvent*)(*outEvent));
    return res;
}

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

void* DelayedHookThread(void*) {
    // 界面与输入的 Hook 不需要等待游戏逻辑
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    void* ins = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (ins) DobbyHook(ins, (void*)hook_consume, (void**)&old_consume);

    std::thread(DataWorkerThread).detach();

    while (true) {
        uintptr_t base = GetModuleBase(GAME_MODULE);
        if (base) {
            g_GameBase = base;
            LOGI("[*] 发现 libil2cpp.so 内存基址: 0x%lx", base);
            
            LOGI("[*] 等待 5 秒，确保引擎彻底解密和初始化内存...");
            sleep(5); 

#if defined(__arm__) 
            g_HookAddr_InitActor = base + 0x73507bc + 1;
            LOGI("[*] 架构: 32位 (Thumb +1)");
#else
            g_HookAddr_InitActor = base + 0x73507bc;
            LOGI("[*] 架构: 64位");
#endif

            std::string hexInit = ReadMemoryHex(g_HookAddr_InitActor & ~1, 8);
            LOGI("[*] 内存自检 InitActor(0x%lx) 前 8 字节: %s", g_HookAddr_InitActor, hexInit.c_str());
            if (hexInit == "00 00 00 00 00 00 00 00 ") {
                LOGE("[!] 致命警告: 目标内存仍然全是00！说明地址被加密、未初始化，或者找错偏移了！");
            }

            // 只安装一个 Hook
            int ret1 = DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
            
            if (ret1 == 0) {
                LOGI("[*] Dobby Hook 下发成功! (ret1:%d) 现在进游戏测试...", ret1);
            } else {
                LOGE("[!] Dobby Hook 下发失败！请检查地址或保护机制 (ret1:%d)", ret1);
            }
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
