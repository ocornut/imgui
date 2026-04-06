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
#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "ZeroConfig_Drive"
#define GAME_MODULE "libGameCore.so" 

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static bool g_Initialized = false;

struct {
    float x = 0.0f;
    float y = 0.0f;
    std::atomic<bool> down{false};
    float renderW = 0.0f; 
    float renderH = 0.0f;
    float physW = 3392.0f;
    float physH = 2400.0f;
    bool isSizeAutoDetected = false;
    
    // 手动触控偏移
    float offsetX = 0.0f;
    float offsetY = 0.0f;
} g_Touch;

// 缩放与 UI 控制
static float g_DynamicScale = 1.0f;
static float g_BaseWindowHeight = 0.0f; 
static float g_UserUIScale = 1.0f; 

// Hook 调试信息全局存储
static uintptr_t g_GameBase = 0;
static uintptr_t g_HookAddr_InitActor = 0;
static uintptr_t g_HookAddr_ClearActor = 0;

// =================================================================
// 动态偏移量配置 (可在 UI 中实时修改)
// =================================================================
struct DynamicOffsets {
    uint32_t x0_to_addr1    = 0x20;
    uint32_t addr1_to_addr2 = 0x10;
    uint32_t item_to_addr3  = 0x18;
    uint32_t addr3_to_addr4 = 0x30;
    uint32_t addr3_to_addr5 = 0x100;
    uint32_t addr3_to_addr6 = 0x70;
    
    // 数据属性的偏移也可以视需要动态化，这里先固定常用的
    uint32_t prop_gold  = 0x20;
    uint32_t prop_hp    = 0x30;
    uint32_t prop_maxhp = 0x32;
    uint32_t prop_level = 0x50;
};

DynamicOffsets g_Offsets;
std::mutex g_OffsetMutex;

// =================================================================
// 内核级安全读取
// =================================================================
static int g_MemFd = -1;

template <typename T>
T SafeRead(uintptr_t address) {
    T value{};
    if (address < 0x10000000 || address > 0x00007FFFFFFFFFFF) return value; 
    
    if (g_MemFd == -1) g_MemFd = open("/proc/self/mem", O_RDONLY);
    if (g_MemFd >= 0) pread(g_MemFd, &value, sizeof(T), address);
    
    return value;
}

// =================================================================
// 分离式快照数据结构 (Snapshot)
// =================================================================
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
    DynamicOffsets offsets; // 记录生成这份快照时使用的偏移
    DebugChain debug;
    std::vector<PlayerData> players;
};

std::mutex g_SnapshotMutex;
GameSnapshot g_CurrentSnapshot; 
std::atomic<uintptr_t> g_ActorManager{0};

// =================================================================
// 后台数据工人线程 (热更新读取)
// =================================================================
void DataWorkerThread() {
    while (true) {
        GameSnapshot newSnap;
        
        // 读取最新的动态偏移
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
            
            // 按照最新的动态偏移进行抓取
            uintptr_t addr1 = SafeRead<uintptr_t>(baseX0 + currOff.x0_to_addr1);
            newSnap.debug.addr1 = addr1;
            
            if (addr1) {
                uintptr_t addr2 = SafeRead<uintptr_t>(addr1 + currOff.addr1_to_addr2);
                newSnap.debug.addr2 = addr2;
                
                if (addr2) {
                    for (int i = 0; i < 15; ++i) { 
                        uintptr_t item = SafeRead<uintptr_t>(addr2 + (i * 0x8)); // 假设指针数组大小为 8 字节
                        newSnap.debug.items[i].base = item;
                        if (!item) continue;

                        uintptr_t addr3 = SafeRead<uintptr_t>(item + currOff.item_to_addr3);
                        newSnap.debug.items[i].addr3 = addr3;
                        if (!addr3) continue;

                        PlayerData p;
                        
                        uintptr_t addr4 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr4);
                        newSnap.debug.items[i].addr4 = addr4;
                        if (addr4) {
                            p.gold  = SafeRead<int32_t>(addr4 + currOff.prop_gold);
                            p.hp    = SafeRead<int32_t>(addr4 + currOff.prop_hp);
                            p.maxHp = SafeRead<int32_t>(addr4 + currOff.prop_maxhp); 
                            p.level = SafeRead<int32_t>(addr4 + currOff.prop_level);
                        }

                        uintptr_t addr5 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr5);
                        newSnap.debug.items[i].addr5 = addr5;
                        if (addr5) {
                            p.worldX = SafeRead<float>(addr5 + 0x0);
                            p.worldZ = SafeRead<float>(addr5 + 0x4);
                            p.worldY = SafeRead<float>(addr5 + 0x8);
                            p.mana   = SafeRead<int32_t>(addr5 + 0x28);
                        }

                        uintptr_t addr6 = SafeRead<uintptr_t>(addr3 + currOff.addr3_to_addr6);
                        newSnap.debug.items[i].addr6 = addr6;
                        if (addr6) {
                            p.mapX = SafeRead<float>(addr6 + 0x50);
                            p.mapY = SafeRead<float>(addr6 + 0x54);
                        }

                        // 有效性过滤
                        if (p.maxHp > 0 && p.maxHp < 50000) {
                            newSnap.players.push_back(p);
                        }
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

// =================================================================
// 基础环境准备
// =================================================================
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
    g_ActorManager.store((uintptr_t)x0);
    old_InitActorParams(x0, x1, x2, x3);
}

void (*old_ClearActor)(void* x0);
void hook_ClearActor(void* x0) {
    g_ActorManager.store(0);
    old_ClearActor(x0);
}

// =================================================================
// 渲染循环与 UI 设计 (全中文汉化)
// =================================================================
void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.55f, h * 0.8f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("指针断点诊断工具 (Pointer Debugger)")) {
        
        // --- 1. 运行时偏移量实时修改面板 ---
        if (ImGui::CollapsingHeader("动态偏移修改器 (点击下方按钮调节)", ImGuiTreeNodeFlags_DefaultOpen)) {
            std::lock_guard<std::mutex> lock(g_OffsetMutex);
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "注：点击下方按钮直接微调，步长为 10 进制");
            ImGui::Separator();
            
            auto drawOffsetBtn = [](const char* label, uint32_t& val) {
                ImGui::PushID(&val);
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s  ->  当前: %d (0x%X)", label, val, val);
                
                if (ImGui::Button("-8")) { if (val >= 8) val -= 8; else val = 0; } ImGui::SameLine();
                if (ImGui::Button("-4")) { if (val >= 4) val -= 4; else val = 0; } ImGui::SameLine();
                if (ImGui::Button("-1")) { if (val >= 1) val -= 1; else val = 0; } ImGui::SameLine();
                
                if (ImGui::Button("+1")) val += 1; ImGui::SameLine();
                if (ImGui::Button("+4")) val += 4; ImGui::SameLine();
                if (ImGui::Button("+8")) val += 8;
                
                ImGui::Separator();
                ImGui::PopID();
            };

            drawOffsetBtn("X0 -> Addr1 (地址1)", g_Offsets.x0_to_addr1);
            drawOffsetBtn("Addr1 -> Addr2 (英雄数组)", g_Offsets.addr1_to_addr2);
            drawOffsetBtn("Hero Item -> Addr3 (实体数据)", g_Offsets.item_to_addr3);
            drawOffsetBtn("Addr3 -> Addr4 (血量属性)", g_Offsets.addr3_to_addr4);
            drawOffsetBtn("Addr3 -> Addr5 (世界坐标)", g_Offsets.addr3_to_addr5);
            drawOffsetBtn("Addr3 -> Addr6 (小地图)", g_Offsets.addr3_to_addr6);
        }

        ImGui::Separator();

        // --- 2. 核心模块与 RVA 展示 ---
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[核心库] %s 基址: 0x%lx", GAME_MODULE, g_GameBase);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[Hook 1] InitActor (RVA 0x73507bc) -> 实际内存: 0x%lx", g_HookAddr_InitActor);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[Hook 2] ClearActor (RVA 0x734bc10) -> 实际内存: 0x%lx", g_HookAddr_ClearActor);
        ImGui::Separator();
        
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "断点图例: 绿色 = 读取成功 | 红色 = 0x0 (请排查上方该级偏移是否正确)");
        ImGui::Separator();
        
        // --- 3. 动态实时断点树 ---
        const auto& d = snap.debug;
        const auto& off = snap.offsets;
        
        ImGui::TextColored(d.baseX0 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "[起点] InitActor x0: 0x%lx", d.baseX0);
        ImGui::TextColored(d.addr1 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "  └─ [+%d] 地址 1: 0x%lx", off.x0_to_addr1, d.addr1);
        ImGui::TextColored(d.addr2 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "      └─ [+%d] 英雄数组: 0x%lx", off.addr1_to_addr2, d.addr2);
        
        ImGui::Separator();
        
        if (d.addr2 != 0) {
            for (int i = 0; i < 15; i++) {
                if (d.items[i].base == 0) continue; 
                
                char nodeName[64];
                snprintf(nodeName, sizeof(nodeName), "英雄数组序号 [%d] - 内存地址: 0x%lx", i, d.items[i].base);
                
                if (ImGui::TreeNodeEx(nodeName, ImGuiTreeNodeFlags_Framed)) {
                    ImGui::TextColored(d.items[i].addr3 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                                       "  └─ [+%d] 实体数据: 0x%lx", off.item_to_addr3, d.items[i].addr3);
                    
                    if (d.items[i].addr3) {
                        ImGui::TextColored(d.items[i].addr4 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                                           "      ├─ [+%d] 属性数据区: 0x%lx", off.addr3_to_addr4, d.items[i].addr4);
                        ImGui::TextColored(d.items[i].addr5 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                                           "      ├─ [+%d] 世界坐标区: 0x%lx", off.addr3_to_addr5, d.items[i].addr5);
                        ImGui::TextColored(d.items[i].addr6 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                                           "      └─ [+%d] 雷达小地图区: 0x%lx", off.addr3_to_addr6, d.items[i].addr6);
                    }
                    ImGui::TreePop();
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "等待英雄数组初始化...");
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
        if (sscanf(buffer, "Physical size: %dx%d", &w, &h) == 2 ||
            sscanf(buffer, "Override size: %dx%d", &w, &h) == 2) {
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

int hook_consume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int res = old_consume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    if (res == 0 && outEvent && *outEvent) {
        handle_android_event((AInputEvent*)(*outEvent));
    }
    return res;
}

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
        
        // 自动加载安卓系统自带的中文字体，解决全部变成问号的问题
        ImGuiIO& io = ImGui::GetIO();
        const char* fontPaths[] = {
            "/system/fonts/NotoSansCJK-Regular.ttc",
            "/system/fonts/NotoSansSC-Regular.otf",
            "/system/fonts/DroidSansFallback.ttf",
            "/system/fonts/Miui-Regular.ttf"
        };
        for (int i = 0; i < 4; ++i) {
            if (access(fontPaths[i], F_OK) == 0) {
                io.Fonts->AddFontFromFileTTF(fontPaths[i], 28.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
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

    static ImGuiStyle default_style;
    static bool style_backed_up = false;
    if (!style_backed_up) {
        default_style = ImGui::GetStyle();
        style_backed_up = true;
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style = default_style;
    style.ScaleAllSizes(1.2f * finalScale); 
    style.WindowRounding = 12.0f;
    style.WindowMinSize = ImVec2(100.0f, 100.0f); 
    io.FontGlobalScale = 1.2f * finalScale;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_Touch.down.load()) {
        ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(0, 255, 0, 200));
    }

    if (g_BaseWindowHeight == 0.0f) {
        g_BaseWindowHeight = h * 0.45f; 
    }

    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.6f, g_BaseWindowHeight), ImGuiCond_FirstUseEver);
    
    static bool show_pointer_debugger = true; 
    static bool show_radar = true;

    if (ImGui::Begin("游戏全自动透视框架 (全中文适配版)")) {
        float currentHeight = ImGui::GetWindowHeight();
        g_DynamicScale = currentHeight / g_BaseWindowHeight;
        if (g_DynamicScale < 0.4f) g_DynamicScale = 0.4f;
        if (g_DynamicScale > 3.0f) g_DynamicScale = 3.0f;

        if (ImGui::CollapsingHeader("基础设置与触控校准", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("菜单缩放倍率", &g_UserUIScale, 0.5f, 2.5f, "%.2f");
            
            ImGui::Text("触控准星偏移 X: %.1f 像素", g_Touch.offsetX);
            ImGui::SameLine();
            if (ImGui::Button("-50##x")) g_Touch.offsetX -= 50; ImGui::SameLine();
            if (ImGui::Button("-10##x")) g_Touch.offsetX -= 10; ImGui::SameLine();
            if (ImGui::Button("+10##x")) g_Touch.offsetX += 10; ImGui::SameLine();
            if (ImGui::Button("+50##x")) g_Touch.offsetX += 50;

            ImGui::Text("触控准星偏移 Y: %.1f 像素", g_Touch.offsetY);
            ImGui::SameLine();
            if (ImGui::Button("-50##y")) g_Touch.offsetY -= 50; ImGui::SameLine();
            if (ImGui::Button("-10##y")) g_Touch.offsetY -= 10; ImGui::SameLine();
            if (ImGui::Button("+10##y")) g_Touch.offsetY += 10; ImGui::SameLine();
            if (ImGui::Button("+50##y")) g_Touch.offsetY += 50;
        }

        ImGui::Separator();
        ImGui::Checkbox("显示小地图雷达", &show_radar);
        ImGui::SameLine();
        ImGui::Checkbox("打开指针断点排错器", &show_pointer_debugger);
        
        ImGui::Separator();
        
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "当前游戏状态: %s", snapshot.inMatch ? "对局中" : "大厅待命");
        ImGui::Text("场上玩家总数: %d", (int)snapshot.players.size());
        
        if (ImGui::BeginTable("PlayersTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("等级");
            ImGui::TableSetupColumn("血量 / 最大血量");
            ImGui::TableSetupColumn("蓝量");
            ImGui::TableSetupColumn("金币");
            ImGui::TableSetupColumn("小地图坐标");
            ImGui::TableHeadersRow();

            for (const auto& p : snapshot.players) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("Lv.%d", p.level);
                ImGui::TableNextColumn(); 
                ImVec4 hpColor = ((float)p.hp / (p.maxHp+1)) < 0.3f ? ImVec4(1, 0, 0, 1) : ImVec4(0, 1, 0, 1);
                ImGui::TextColored(hpColor, "%d / %d", p.hp, p.maxHp);
                ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "%d", p.mana);
                ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "%d", p.gold);
                ImGui::TableNextColumn(); ImGui::Text("%.1f, %.1f", p.mapX, p.mapY);
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();

    if (show_pointer_debugger) {
        DrawPointerDebugger(snapshot, w, h);
    }

    if (show_radar && snapshot.inMatch) {
        ImGui::SetNextWindowPos(ImVec2(w - 350, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
        if (ImGui::Begin("Radar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
            
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImVec2 size = ImGui::GetContentRegionAvail();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            
            draw_list->AddRect(p0, ImVec2(p0.x + size.x, p0.y + size.y), IM_COL32(255, 255, 255, 100));

            for (const auto& p : snapshot.players) {
                float mapScale = 10000.0f; 
                float normalizedX = p.mapX / mapScale;
                float normalizedY = p.mapY / mapScale;
                
                float radarX = p0.x + (normalizedX * size.x);
                float radarY = p0.y + size.y - (normalizedY * size.y); 

                if (normalizedX > 0 && normalizedX < 1 && normalizedY > 0 && normalizedY < 1) {
                    draw_list->AddCircleFilled(ImVec2(radarX, radarY), 4.0f * finalScale, IM_COL32(255, 0, 0, 255));
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    ImGui::Render();
    
    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_depth;
    glGetIntegerv(GL_DEPTH_TEST, &last_depth);
    
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, w, h); 
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
    if (last_depth) glEnable(GL_DEPTH_TEST);

    return old_eglSwapBuffers(dpy, surface);
}

void* DelayedHookThread(void*) {
    sleep(15); 
    
    std::thread(DataWorkerThread).detach();

    uintptr_t gameBase = GetModuleBase(GAME_MODULE);
    if (!gameBase) {
        LOGE("未找到 %s", GAME_MODULE);
    } else {
        g_GameBase = gameBase;
        g_HookAddr_InitActor = gameBase + 0x73507bc;
        g_HookAddr_ClearActor = gameBase + 0x734bc10;

        LOGI("[*] 准备 Hook InitActorParams, RVA: 0x73507bc, 绝对地址: 0x%lx", g_HookAddr_InitActor);
        DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
        
        LOGI("[*] 准备 Hook ClearActor, RVA: 0x734bc10, 绝对地址: 0x%lx", g_HookAddr_ClearActor);
        DobbyHook((void*)g_HookAddr_ClearActor, (void*)hook_ClearActor, (void**)&old_ClearActor);
        
        LOGI("[*] 注入成功! 模块基址: %p", (void*)gameBase);
    }

    void* sym_egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (sym_egl) DobbyHook(sym_egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    
    void* consume_addr = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
    if (consume_addr) DobbyHook(consume_addr, (void*)hook_consume, (void**)&old_consume);
    
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
