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
} g_Touch;

static float g_DynamicScale = 1.0f;
static float g_BaseWindowHeight = 0.0f; 

// =================================================================
// 内核级安全读取 (文件描述符长期保持打开以提升性能)
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
// 核心架构升级：分离式快照数据结构 (Snapshot)
// =================================================================
struct PlayerData {
    int hp = 0, maxHp = 0, mana = 0, gold = 0, level = 0;
    float worldX = 0, worldY = 0, worldZ = 0;
    float mapX = 0, mapY = 0;
};

// 完整的指针链快照（用于排错）
struct DebugChain {
    uintptr_t baseX0 = 0;
    uintptr_t addr1 = 0;
    uintptr_t addr2 = 0;
    struct Item {
        uintptr_t base = 0, addr3 = 0, addr4 = 0, addr5 = 0, addr6 = 0;
    } items[15];
};

// 全局快照包裹
struct GameSnapshot {
    bool inMatch = false;
    DebugChain debug;
    std::vector<PlayerData> players;
};

// 线程间数据交换的核心锁
std::mutex g_SnapshotMutex;
GameSnapshot g_CurrentSnapshot; 
std::atomic<uintptr_t> g_ActorManager{0};

// =================================================================
// 后台数据工人线程 (与渲染彻底解耦，永不卡死画面)
// =================================================================
void DataWorkerThread() {
    while (true) {
        GameSnapshot newSnap;
        uintptr_t baseX0 = g_ActorManager.load();
        
        if (baseX0 != 0) {
            newSnap.inMatch = true;
            newSnap.debug.baseX0 = baseX0;
            
            uintptr_t addr1 = SafeRead<uintptr_t>(baseX0 + 0x20);
            newSnap.debug.addr1 = addr1;
            
            if (addr1) {
                uintptr_t addr2 = SafeRead<uintptr_t>(addr1 + 0x10);
                newSnap.debug.addr2 = addr2;
                
                if (addr2) {
                    for (int i = 0; i < 15; ++i) { 
                        uintptr_t item = SafeRead<uintptr_t>(addr2 + (i * 0x8));
                        newSnap.debug.items[i].base = item;
                        if (!item) continue;

                        uintptr_t addr3 = SafeRead<uintptr_t>(item + 0x18);
                        newSnap.debug.items[i].addr3 = addr3;
                        if (!addr3) continue;

                        PlayerData p;
                        
                        uintptr_t addr4 = SafeRead<uintptr_t>(addr3 + 0x30);
                        newSnap.debug.items[i].addr4 = addr4;
                        if (addr4) {
                            p.gold  = SafeRead<int32_t>(addr4 + 0x20);
                            p.hp    = SafeRead<int32_t>(addr4 + 0x30);
                            p.maxHp = SafeRead<int32_t>(addr4 + 0x32); 
                            p.level = SafeRead<int32_t>(addr4 + 0x50);
                        }

                        uintptr_t addr5 = SafeRead<uintptr_t>(addr3 + 0x100);
                        newSnap.debug.items[i].addr5 = addr5;
                        if (addr5) {
                            p.worldX = SafeRead<float>(addr5 + 0x0);
                            p.worldZ = SafeRead<float>(addr5 + 0x4);
                            p.worldY = SafeRead<float>(addr5 + 0x8);
                            p.mana   = SafeRead<int32_t>(addr5 + 0x28);
                        }

                        uintptr_t addr6 = SafeRead<uintptr_t>(addr3 + 0x70);
                        newSnap.debug.items[i].addr6 = addr6;
                        if (addr6) {
                            p.mapX = SafeRead<float>(addr6 + 0x50);
                            p.mapY = SafeRead<float>(addr6 + 0x54);
                        }

                        if (p.maxHp > 0 && p.maxHp < 50000) {
                            newSnap.players.push_back(p);
                        }
                    }
                }
            }
        }
        
        // 安全交换快照
        {
            std::lock_guard<std::mutex> lock(g_SnapshotMutex);
            g_CurrentSnapshot = newSnap;
        }
        
        // 休眠 16ms 限制性能消耗 (约 60FPS 刷新率)
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
// 渲染循环与 UI 设计 (学习 2.txt)
// =================================================================
void DrawPointerDebugger(const GameSnapshot& snap, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.5f, h * 0.7f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Pointer Debugger (Find Breakpoints)")) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Color Legend: Green = Valid | Red = Nullptr (Broken)");
        ImGui::Separator();
        
        const auto& d = snap.debug;
        
        // 顶级基址展示
        ImGui::TextColored(d.baseX0 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "[Base] InitActor x0: 0x%lx", d.baseX0);
        ImGui::TextColored(d.addr1 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "  └─ [+0x20] Addr1: 0x%lx", d.addr1);
        ImGui::TextColored(d.addr2 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), "      └─ [+0x10] HeroArray: 0x%lx", d.addr2);
        
        ImGui::Separator();
        
        if (d.addr2 != 0) {
            for (int i = 0; i < 15; i++) {
                if (d.items[i].base == 0) continue; 
                
                char nodeName[64];
                snprintf(nodeName, sizeof(nodeName), "Hero Array Index [%d] - Addr: 0x%lx", i, d.items[i].base);
                
                // 使用 TreeNode 仿造类似 CE 的内存层级视图
                if (ImGui::TreeNodeEx(nodeName, ImGuiTreeNodeFlags_Framed)) {
                    
                    ImGui::TextColored(d.items[i].addr3 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                                       "  └─ [+0x18] EntityData: 0x%lx", d.items[i].addr3);
                    
                    if (d.items[i].addr3) {
                        ImGui::TextColored(d.items[i].addr4 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                                           "      ├─ [+0x30] Props Addr: 0x%lx", d.items[i].addr4);
                        ImGui::TextColored(d.items[i].addr5 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                                           "      ├─ [+0x100] World Addr: 0x%lx", d.items[i].addr5);
                        ImGui::TextColored(d.items[i].addr6 ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                                           "      └─ [+0x70] Radar Addr: 0x%lx", d.items[i].addr6);
                    }
                    ImGui::TreePop();
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Waiting for Hero Array to initialize...");
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
        g_Touch.x = AMotionEvent_getX(event, 0) * autoScaleX;
        g_Touch.y = AMotionEvent_getY(event, 0) * autoScaleY;

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

    // 【核心】从后台线程窃取一张"快照"，渲染层永不参与真实内存读写
    GameSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(g_SnapshotMutex);
        snapshot = g_CurrentSnapshot;
    }

    if (!g_Initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_Initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Touch.renderW, g_Touch.renderH);
    io.MousePos = ImVec2(g_Touch.x, g_Touch.y);
    io.MouseDown[0] = g_Touch.down.load(); 

    float deviceScale = (float)h / 1000.0f;
    if (deviceScale < 1.0f) deviceScale = 1.0f;
    float finalScale = deviceScale * g_DynamicScale; 

    static ImGuiStyle default_style;
    static bool style_backed_up = false;
    if (!style_backed_up) {
        default_style = ImGui::GetStyle();
        style_backed_up = true;
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style = default_style;
    style.ScaleAllSizes(3.0f * finalScale);
    style.WindowRounding = 12.0f;
    style.WindowMinSize = ImVec2(100.0f, 100.0f); 
    io.FontGlobalScale = 3.0f * finalScale;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_Touch.down.load()) {
        ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(0, 255, 0, 200));
    }

    if (g_BaseWindowHeight == 0.0f) {
        g_BaseWindowHeight = h * 0.45f; 
    }

    // 菜单展示
    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * 0.6f, g_BaseWindowHeight), ImGuiCond_FirstUseEver);
    
    static bool show_pointer_debugger = false; 
    static bool show_radar = true;

    if (ImGui::Begin("Game Inspector (Auto-Adaptive)")) {
        float currentHeight = ImGui::GetWindowHeight();
        g_DynamicScale = currentHeight / g_BaseWindowHeight;
        if (g_DynamicScale < 0.4f) g_DynamicScale = 0.4f;
        if (g_DynamicScale > 3.0f) g_DynamicScale = 3.0f;

        ImGui::Checkbox("Show Radar (小地图透视)", &show_radar);
        ImGui::SameLine();
        ImGui::Checkbox("Pointer Debugger (指针调试器)", &show_pointer_debugger);
        
        ImGui::Separator();
        
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Game Status: %s", snapshot.inMatch ? "In Match (局内)" : "Waiting (大厅)");
        ImGui::Text("Players Count: %d", (int)snapshot.players.size());
        
        if (ImGui::BeginTable("PlayersTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Level");
            ImGui::TableSetupColumn("HP / MaxHP");
            ImGui::TableSetupColumn("Mana");
            ImGui::TableSetupColumn("Gold");
            ImGui::TableSetupColumn("Map Coord");
            ImGui::TableHeadersRow();

            // 直接用 snapshot 的数据画图，0 消耗，0 崩溃风险
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

    // 呼叫调试窗口
    if (show_pointer_debugger) {
        DrawPointerDebugger(snapshot, w, h);
    }

    // 雷达绘制
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
    
    // 【核心】启动专门的后台查内存工人
    std::thread(DataWorkerThread).detach();

    uintptr_t gameBase = GetModuleBase(GAME_MODULE);
    if (!gameBase) {
        LOGE("未找到 %s", GAME_MODULE);
    } else {
        DobbyHook((void*)(gameBase + 0x73507bc), (void*)hook_InitActorParams, (void**)&old_InitActorParams);
        DobbyHook((void*)(gameBase + 0x734bc10), (void*)hook_ClearActor, (void**)&old_ClearActor);
        LOGI("[*] 注入成功! 基址: %p", (void*)gameBase);
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
