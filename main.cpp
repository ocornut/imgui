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
#include <vector>          // 必须包含 vector
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "IL2CPP_Instrument"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

// 日志缓冲区
#define MAX_LOG_LINES 200
#define MAX_LOG_LEN   256
static std::mutex g_LogMutex;
static char g_LogBuffer[MAX_LOG_LINES][MAX_LOG_LEN];
static int g_LogHead = 0, g_LogCount = 0;

void AddLog(const char* fmt, ...) {
    char buf[MAX_LOG_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    std::lock_guard<std::mutex> lock(g_LogMutex);
    strncpy(g_LogBuffer[g_LogHead], buf, MAX_LOG_LEN-1);
    g_LogBuffer[g_LogHead][MAX_LOG_LEN-1] = '\0';
    g_LogHead = (g_LogHead + 1) % MAX_LOG_LINES;
    if (g_LogCount < MAX_LOG_LINES) g_LogCount++;
    LOGI("%s", buf);
}

static uintptr_t g_ActorManager = 0;
static int g_MemFd = -1;

template<typename T>
T SafeRead(uintptr_t addr) {
    T val{};
    if (addr < 0x10000) return val;
    if (g_MemFd == -1) g_MemFd = open("/proc/self/mem", O_RDONLY);
    if (g_MemFd >= 0) pread(g_MemFd, &val, sizeof(T), addr);
    return val;
}

// 偏移链（可调整）
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

// 玩家数据结构
struct PlayerData {
    int hp = 0, maxHp = 0, mana = 0, gold = 0, level = 0;
    float mapX = 0, mapY = 0;
};

// 快照结构
struct GameSnapshot {
    bool inMatch = false;
    uintptr_t x0 = 0;
    std::vector<PlayerData> players;
};

static GameSnapshot g_Snapshot;
static std::mutex g_SnapMutex;

// 后台数据读取线程
void DataThread() {
    while (true) {
        GameSnapshot snap;
        uintptr_t base = g_ActorManager;
        if (base) {
            snap.inMatch = true;
            snap.x0 = base;
            uintptr_t addr1 = SafeRead<uintptr_t>(base + g_Offsets.x0_to_addr1);
            if (addr1) {
                uintptr_t addr2 = SafeRead<uintptr_t>(addr1 + g_Offsets.addr1_to_addr2);
                if (addr2) {
                    for (int i = 0; i < 15; i++) {
                        uintptr_t item = SafeRead<uintptr_t>(addr2 + i * 8);
                        if (!item) continue;
                        uintptr_t addr3 = SafeRead<uintptr_t>(item + g_Offsets.item_to_addr3);
                        if (!addr3) continue;
                        PlayerData p;
                        uintptr_t addr4 = SafeRead<uintptr_t>(addr3 + g_Offsets.addr3_to_addr4);
                        if (addr4) {
                            p.gold = SafeRead<int32_t>(addr4 + g_Offsets.prop_gold);
                            p.hp = SafeRead<int32_t>(addr4 + g_Offsets.prop_hp);
                            p.maxHp = SafeRead<int16_t>(addr4 + g_Offsets.prop_maxhp);
                            p.level = SafeRead<int32_t>(addr4 + g_Offsets.prop_level);
                        }
                        uintptr_t addr5 = SafeRead<uintptr_t>(addr3 + g_Offsets.addr3_to_addr5);
                        if (addr5) {
                            p.mana = SafeRead<int32_t>(addr5 + g_Offsets.prop_mana);
                        }
                        uintptr_t addr6 = SafeRead<uintptr_t>(addr3 + g_Offsets.addr3_to_addr6);
                        if (addr6) {
                            p.mapX = SafeRead<float>(addr6 + 0x50);
                            p.mapY = SafeRead<float>(addr6 + 0x54);
                        }
                        if (p.maxHp > 0) snap.players.push_back(p);
                    }
                }
            }
        }
        // 替换快照（使用锁保护）
        {
            std::lock_guard<std::mutex> lock(g_SnapMutex);
            g_Snapshot = std::move(snap);  // 使用移动语义避免拷贝问题
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

// 获取模块基址
uintptr_t GetBase(const char* name) {
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

// ========== DobbyInstrument 回调（不需要知道参数个数）==========
#ifdef __aarch64__
#include <dobby.h>  // RegisterContext 定义在此
void instrument_InitActor(RegisterContext* ctx, void* userdata) {
    uintptr_t x0 = ctx->general.x0;
    AddLog("[Instrument] InitActor called, x0 = %p", (void*)x0);
    if (x0) g_ActorManager = x0;
}
void instrument_ClearActor(RegisterContext* ctx, void* userdata) {
    AddLog("[Instrument] ClearActor called, x0 = %p", (void*)ctx->general.x0);
    g_ActorManager = 0;
}
#endif

// ========== 触摸处理 ==========
static float touchX, touchY;
static std::atomic<bool> touchDown{false};
void HandleEvent(AInputEvent* e) {
    if (AInputEvent_getType(e) == AINPUT_EVENT_TYPE_MOTION) {
        int act = AMotionEvent_getAction(e) & AMOTION_EVENT_ACTION_MASK;
        touchX = AMotionEvent_getX(e, 0);
        touchY = AMotionEvent_getY(e, 0);
        touchDown = (act == AMOTION_EVENT_ACTION_DOWN);
    }
}
typedef int (*ConsumeFn)(void*, void*, bool, int64_t, uint32_t*, void**);
ConsumeFn oldConsume = nullptr;
int hookConsume(void* ins, void* fac, bool batch, int64_t time, uint32_t* seq, void** ev) {
    int r = oldConsume(ins, fac, batch, time, seq, ev);
    if (r == 0 && ev && *ev) HandleEvent((AInputEvent*)*ev);
    return r;
}

// ========== eglSwapBuffers Hook ==========
typedef EGLBoolean (*SwapFn)(EGLDisplay, EGLSurface);
SwapFn oldSwap = nullptr;
static bool imguiInit = false;

EGLBoolean hookSwap(EGLDisplay dpy, EGLSurface surf) {
    EGLint w, h;
    eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    if (w > 0 && h > 0) {
        if (!imguiInit) {
            ImGui::CreateContext();
            ImGui_ImplOpenGL3_Init("#version 300 es");
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            imguiInit = true;
            AddLog("ImGui 初始化完成");
        }
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(w, h);
        io.MousePos = ImVec2(touchX, touchY);
        io.MouseDown[0] = touchDown.load();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        // 获取快照
        GameSnapshot snap;
        {
            std::lock_guard<std::mutex> lock(g_SnapMutex);
            snap = g_Snapshot;  // 拷贝（vector 可拷贝）
        }

        // 主窗口
        ImGui::Begin("IL2CPP Hook");
        ImGui::Text("状态: %s", snap.inMatch ? "对局中" : "大厅");
        ImGui::Text("ActorManager x0: 0x%lx", snap.x0);
        ImGui::Text("玩家数量: %d", (int)snap.players.size());
        ImGui::Separator();
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

        // 日志窗口
        ImGui::Begin("日志");
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

        // 小地图雷达
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
    }
    return oldSwap(dpy, surf);
}

// ========== 延迟初始化线程 ==========
void* DelayedHookThread(void*) {
    AddLog("插件启动，等待游戏模块...");
    // 等待 libil2cpp.so 加载并注册 Instrument
    while (true) {
        uintptr_t base = GetBase("libil2cpp.so");
        if (base) {
            uintptr_t initAddr = base + 0x73507bc;
            uintptr_t clearAddr = base + 0x734bc10;
            AddLog("基址: 0x%lx, InitActor: 0x%lx, ClearActor: 0x%lx", base, initAddr, clearAddr);
#ifdef __aarch64__
            int ret1 = DobbyInstrument((void*)initAddr, instrument_InitActor);
            int ret2 = DobbyInstrument((void*)clearAddr, instrument_ClearActor);
            AddLog("DobbyInstrument 结果: InitActor=%d, ClearActor=%d", ret1, ret2);
#endif
            break;
        }
        sleep(1);
    }

    // Hook EGL 和 Input
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hookSwap, (void**)&oldSwap);
    void* inp = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (inp) DobbyHook(inp, (void*)hookConsume, (void**)&oldConsume);

    // 启动后台数据线程
    std::thread(DataThread).detach();
    AddLog("所有初始化完成");
    return nullptr;
}

__attribute__((constructor)) void init() {
    pthread_t t;
    pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
