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
    
    // 触控偏移
    float offsetX = 0.0f;
    float offsetY = 0.0f;
} g_Touch;

static float g_DynamicScale = 1.0f;
static float g_BaseWindowHeight = 0.0f; 
static float g_UserUIScale = 1.1f; 

static std::atomic<int> g_InitActorTriggerCount{0};
static uintptr_t g_GameBase = 0;
static uintptr_t g_HookAddr_InitActor = 0;
static uintptr_t g_HookAddr_ClearActor = 0;

// =================================================================
// 核心读取逻辑：使用 Pipe (管道) 模式
// =================================================================
static int g_SafePipe[2] = {-1, -1};

void InitSafePipe() {
    if (g_SafePipe[0] == -1 && pipe(g_SafePipe) == -1) {
        LOGE("[-] 管道创建失败！");
    }
}

template <typename T>
T SafeRead(uintptr_t address) {
    T value{};
    if (address < 0x10000000 || address > 0x00007FFFFFFFFFFF) return value;
    if (g_SafePipe[0] == -1) InitSafePipe();
    if (write(g_SafePipe[1], (void*)address, sizeof(T)) == -1) return value;
    read(g_SafePipe[0], &value, sizeof(T));
    return value;
}

// 动态偏移配置
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
                        if (addr4) {
                            p.gold = SafeRead<int32_t>(addr4 + currOff.prop_gold);
                            p.hp = SafeRead<int32_t>(addr4 + currOff.prop_hp);
                            p.maxHp = SafeRead<int32_t>(addr4 + currOff.prop_maxhp); 
                            p.level = SafeRead<int32_t>(addr4 + currOff.prop_level);
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
    g_ActorManager.store((uintptr_t)x0);
    old_InitActorParams(x0, x1, x2, x3);
}

void (*old_ClearActor)(void* x0);
void hook_ClearActor(void* x0) {
    g_ActorManager.store(0);
    old_ClearActor(x0);
}

// 指针调试窗
void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(w * 0.4f, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.55f, h * 0.8f), ImGuiCond_FirstUseEver);
    
    // 确保不带 NoMove 标志
    if (ImGui::Begin("内存指针链实时诊断", nullptr, ImGuiWindowFlags_None)) {
        if (ImGui::CollapsingHeader("10进制偏移微调 (按 1 调节)", ImGuiTreeNodeFlags_DefaultOpen)) {
            std::lock_guard<std::mutex> lock(g_OffsetMutex);
            auto drawOffsetBtn = [](const char* label, uint32_t& val) {
                ImGui::PushID(&val);
                ImGui::Text("%s: %d", label, val);
                if (ImGui::Button("-8")) val -= 8; ImGui::SameLine();
                if (ImGui::Button("-1")) val -= 1; ImGui::SameLine();
                if (ImGui::Button("+1")) val += 1; ImGui::SameLine();
                if (ImGui::Button("+8")) val += 8;
                ImGui::PopID();
            };
            drawOffsetBtn("地址1", g_Offsets.x0_to_addr1);
            drawOffsetBtn("数组", g_Offsets.addr1_to_addr2);
            drawOffsetBtn("实体", g_Offsets.item_to_addr3);
        }
        ImGui::Separator();
        const auto& d = snap.debug;
        ImGui::TextColored(d.baseX0 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "X0 基址: 0x%lx (触发:%d)", d.baseX0, g_InitActorTriggerCount.load());
        ImGui::TextColored(d.addr1 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), " └─ 地址1: 0x%lx", d.addr1);
        ImGui::TextColored(d.addr2 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "     └─ 数组: 0x%lx", d.addr2);
    }
    ImGui::End();
}

typedef EGLBoolean (*p_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static p_eglSwapBuffers old_eglSwapBuffers = nullptr;

typedef int (*p_InputConsumer_consume)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static p_InputConsumer_consume old_consume = nullptr;

void handle_android_event(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // 使用 renderW/physW 进行比例换算
        float autoScaleX = (g_Touch.renderW > 0) ? (g_Touch.renderW / g_Touch.physW) : 1.0f;
        float autoScaleY = (g_Touch.renderH > 0) ? (g_Touch.renderH / g_Touch.physH) : 1.0f;
        
        // 关键：实时更新 X/Y
        g_Touch.x = (AMotionEvent_getX(event, 0) * autoScaleX) + g_Touch.offsetX;
        g_Touch.y = (AMotionEvent_getY(event, 0) * autoScaleY) + g_Touch.offsetY;

        if (action == AMOTION_EVENT_ACTION_DOWN) {
            g_Touch.down.store(true);
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            g_Touch.down.store(false);
        }
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
        
        // 加载字体逻辑
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

    // --- UI 样式优化：加厚标题栏以便于拖动 ---
    static ImGuiStyle default_style = ImGui::GetStyle();
    ImGuiStyle& style = ImGui::GetStyle(); 
    style = default_style;
    style.ScaleAllSizes(1.2f * finalScale); 
    style.WindowRounding = 12.0f;
    style.FramePadding = ImVec2(10, 10); // 加厚交互区域
    style.TitleAlign = ImVec2(0.5f, 0.5f); // 标题居中
    io.FontGlobalScale = 1.2f * finalScale;

    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
    
    // 准星调试圆点
    if (g_Touch.down.load()) ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 22.0f, IM_COL32(0, 255, 0, 180));

    if (g_BaseWindowHeight == 0.0f) g_BaseWindowHeight = h * 0.45f; 
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.6f, g_BaseWindowHeight), ImGuiCond_FirstUseEver);
    
    static bool show_pointer_debugger = true; 

    if (ImGui::Begin("全自动对局助手 (libil2cpp版)")) {
        if (!g_FontLoaded) ImGui::TextColored(ImVec4(1,0,0,1), "警告: 无法加载字库，中文变问号!");
        
        if (ImGui::CollapsingHeader("校准与设置", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("UI 缩放", &g_UserUIScale, 0.5f, 2.0f);
            
            // 解决“拖不了”的核心：重置偏移
            ImGui::Text("当前触控偏移: X:%.1f Y:%.1f", g_Touch.offsetX, g_Touch.offsetY);
            if (ImGui::Button(" 重置所有偏移量 (RESET) ", ImVec2(-1, 0))) {
                g_Touch.offsetX = 0;
                g_Touch.offsetY = 0;
            }
            
            if (ImGui::Button("-10##x")) g_Touch.offsetX -= 10; ImGui::SameLine();
            if (ImGui::Button("+10##x")) g_Touch.offsetX += 10; ImGui::SameLine();
            ImGui::Text("微调 X");
            if (ImGui::Button("-10##y")) g_Touch.offsetY -= 10; ImGui::SameLine();
            if (ImGui::Button("+10##y")) g_Touch.offsetY += 10;
            ImGui::Text("微调 Y");
        }
        
        ImGui::Separator();
        ImGui::Checkbox("开启指针诊断窗", &show_pointer_debugger);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "对局状态: %s", snapshot.inMatch ? "正在对局" : "空闲中");
        
        if (ImGui::BeginTable("PlayersTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("等级"); ImGui::TableSetupColumn("血量数据");
            ImGui::TableHeadersRow();
            for (const auto& p : snapshot.players) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("Lv.%d", p.level);
                ImGui::TableNextColumn(); ImGui::Text("%d/%d", p.hp, p.maxHp);
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
    InitSafePipe();
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (egl) DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    void* ins = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (ins) DobbyHook(ins, (void*)hook_consume, (void**)&old_consume);
    
    std::thread(DataWorkerThread).detach();

    while (true) {
        uintptr_t base = GetModuleBase(GAME_MODULE);
        if (base) {
            g_GameBase = base;
            g_HookAddr_InitActor = base + 0x73507bc;
            g_HookAddr_ClearActor = base + 0x734bc10;
            DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
            DobbyHook((void*)g_HookAddr_ClearActor, (void*)hook_ClearActor, (void**)&old_ClearActor);
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t; pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
