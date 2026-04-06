#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include <stdio.h>
#include <atomic>
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

// ======================== 日志缓冲区（悬浮窗显示）========================
#define MAX_LOG_LINES 200
#define MAX_LOG_LEN   256

static std::mutex g_LogMutex;
static char g_LogBuffer[MAX_LOG_LINES][MAX_LOG_LEN];
static int g_LogHead = 0;
static int g_LogCount = 0;

void AddLog(const char* fmt, ...) {
    char buf[MAX_LOG_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    std::lock_guard<std::mutex> lock(g_LogMutex);
    strncpy(g_LogBuffer[g_LogHead], buf, MAX_LOG_LEN - 1);
    g_LogBuffer[g_LogHead][MAX_LOG_LEN - 1] = '\0';
    g_LogHead = (g_LogHead + 1) % MAX_LOG_LINES;
    if (g_LogCount < MAX_LOG_LINES) g_LogCount++;
    LOGI("%s", buf);
}

// ======================== 全局变量 ========================
static bool g_ImGuiReady = false;
static uintptr_t g_ActorManager = 0;
static int g_MemFd = -1;

// 偏移链（可动态调整）
struct {
    uint32_t x0_to_addr1 = 0x20;
    uint32_t addr1_to_addr2 = 0x10;
    uint32_t item_to_addr3 = 0x18;
    uint32_t addr3_to_addr4 = 0x30;
    uint32_t addr3_to_addr5 = 0x100;
    uint32_t addr3_to_addr6 = 0x70;
    uint32_t prop_gold = 0x20;
    uint32_t prop_hp = 0x30;
    uint32_t prop_maxhp = 0x32;
    uint32_t prop_level = 0x50;
    uint32_t prop_mana = 0x28;
} g_Offsets;

// ======================== 安全内存读取 ========================
template<typename T>
T SafeRead(uintptr_t addr) {
    T val{};
    if (addr < 0x10000 || addr > 0x7fffffffffff) return val;
    if (g_MemFd == -1) g_MemFd = open("/proc/self/mem", O_RDONLY);
    if (g_MemFd >= 0) pread(g_MemFd, &val, sizeof(T), addr);
    return val;
}

// ======================== 玩家数据结构 ========================
struct PlayerData {
    int hp = 0, maxHp = 0, mana = 0, gold = 0, level = 0;
    float worldX = 0, worldY = 0, worldZ = 0;
    float mapX = 0, mapY = 0;
};

struct GameSnapshot {
    bool inMatch = false;
    std::vector<PlayerData> players;
    uintptr_t x0 = 0;
    uintptr_t addr1 = 0;
    uintptr_t addr2 = 0;
};

static GameSnapshot g_CurrentSnapshot;
static std::mutex g_SnapshotMutex;

// ======================== 后台数据读取线程 ========================
void DataWorkerThread() {
    AddLog("[DataThread] 启动");
    while (true) {
        GameSnapshot newSnap;
        uintptr_t baseX0 = g_ActorManager;
        
        if (baseX0 != 0) {
            newSnap.inMatch = true;
            newSnap.x0 = baseX0;
            
            uintptr_t addr1 = SafeRead<uintptr_t>(baseX0 + g_Offsets.x0_to_addr1);
            newSnap.addr1 = addr1;
            if (addr1) {
                uintptr_t addr2 = SafeRead<uintptr_t>(addr1 + g_Offsets.addr1_to_addr2);
                newSnap.addr2 = addr2;
                if (addr2) {
                    // 遍历玩家数组（最多15个）
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
                        if (addr5) {
                            p.worldX = SafeRead<float>(addr5 + 0x0);
                            p.worldZ = SafeRead<float>(addr5 + 0x4);
                            p.worldY = SafeRead<float>(addr5 + 0x8);
                            p.mana   = SafeRead<int32_t>(addr5 + g_Offsets.prop_mana);
                        }
                        uintptr_t addr6 = SafeRead<uintptr_t>(addr3 + g_Offsets.addr3_to_addr6);
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

// ======================== 模块基址获取 ========================
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

// ======================== Hook 函数（只有1个参数）========================
void (*old_InitActorParams)(void* x0);
void hook_InitActorParams(void* x0) {
    AddLog("[HOOK] InitActorParams called, x0 = %p", x0);
    if (x0 != nullptr) {
        g_ActorManager = (uintptr_t)x0;
        // 测试读取偏移验证
        uintptr_t test = SafeRead<uintptr_t>((uintptr_t)x0 + g_Offsets.x0_to_addr1);
        AddLog("[HOOK] x0+0x%x = %p", g_Offsets.x0_to_addr1, (void*)test);
    } else {
        AddLog("[HOOK] x0 is null!");
    }
    old_InitActorParams(x0);
}

void (*old_ClearActor)(void* x0);
void hook_ClearActor(void* x0) {
    AddLog("[HOOK] ClearActor called, x0 = %p", x0);
    g_ActorManager = 0;
    old_ClearActor(x0);
}

// ======================== 触摸事件处理（ImGui 输入）========================
static float touchX = 0, touchY = 0;
static std::atomic<bool> touchDown{false};

void HandleAndroidEvent(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        touchX = AMotionEvent_getX(event, 0);
        touchY = AMotionEvent_getY(event, 0);
        touchDown = (action == AMOTION_EVENT_ACTION_DOWN);
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

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    EGLint w, h;
    eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    if (w <= 0 || h <= 0) return old_eglSwap(dpy, surf);
    
    static float last_w = 0, last_h = 0;
    if (!g_ImGuiReady) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();  // 使用默认字体，不折腾中文
        g_ImGuiReady = true;
        AddLog("[ImGui] 初始化完成");
    }
    
    // 获取快照
    GameSnapshot snap;
    {
        std::lock_guard<std::mutex> lock(g_SnapshotMutex);
        snap = g_CurrentSnapshot;
    }
    
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(w, h);
    io.MousePos = ImVec2(touchX, touchY);
    io.MouseDown[0] = touchDown.load();
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    
    // ========== 主窗口：显示玩家信息 ==========
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("IL2CPP Hook Debug");
    ImGui::Text("状态: %s", snap.inMatch ? "对局中" : "大厅中");
    ImGui::Text("ActorManager x0: 0x%lx", snap.x0);
    ImGui::Text("addr1: 0x%lx", snap.addr1);
    ImGui::Text("addr2: 0x%lx", snap.addr2);
    ImGui::Separator();
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
    
    // ========== 日志窗口 ==========
    ImGui::SetNextWindowPos(ImVec2(500, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("实时日志");
    ImGui::BeginChild("LogScroll", ImVec2(0, 0), false);
    {
        std::lock_guard<std::mutex> lock(g_LogMutex);
        int start = (g_LogHead - g_LogCount + MAX_LOG_LINES) % MAX_LOG_LINES;
        for (int i = 0; i < g_LogCount; i++) {
            int idx = (start + i) % MAX_LOG_LINES;
            ImGui::TextUnformatted(g_LogBuffer[idx]);
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    ImGui::End();
    
    // ========== 小地图雷达（如果有坐标） ==========
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
    
    ImGui::Render();
    glViewport(0, 0, w, h);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    return old_eglSwap(dpy, surf);
}

// ======================== 延迟初始化线程 ========================
void* DelayedHookThread(void*) {
    AddLog("[Init] 插件加载，等待游戏模块...");
    
    // 1. 等待 libil2cpp.so 加载并 Hook 游戏函数
    while (true) {
        uintptr_t base = GetModuleBase("libil2cpp.so");
        if (base != 0) {
            uintptr_t initAddr = base + 0x73507bc;
            uintptr_t clearAddr = base + 0x734bc10;
            AddLog("[Game] libil2cpp.so base = 0x%lx", base);
            AddLog("[Game] InitActor absolute = 0x%lx", initAddr);
            AddLog("[Game] ClearActor absolute = 0x%lx", clearAddr);
            
            int ret1 = DobbyHook((void*)initAddr, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
            int ret2 = DobbyHook((void*)clearAddr, (void*)hook_ClearActor, (void**)&old_ClearActor);
            AddLog("[Hook] InitActor result: %d, ClearActor result: %d", ret1, ret2);
            break;
        }
        sleep(1);
    }
    
    // 2. Hook EGL 和 Input（保证 UI 显示）
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl) {
        int ret = DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwap);
        AddLog("[EGL] DobbyHook result: %d", ret);
    } else {
        AddLog("[EGL] eglSwapBuffers not found");
    }
    
    void* inp = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (inp) {
        int ret = DobbyHook(inp, (void*)hook_consume, (void**)&old_consume);
        AddLog("[Input] DobbyHook result: %d", ret);
    } else {
        AddLog("[Input] InputConsumer::consume not found");
    }
    
    // 3. 启动后台数据线程
    std::thread(DataWorkerThread).detach();
    AddLog("[Init] 所有初始化完成");
    return nullptr;
}

// ======================== 入口 ========================
__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
