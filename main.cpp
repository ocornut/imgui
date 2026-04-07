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
#include <setjmp.h>
#include <signal.h>
#include <cstdarg>
#include <cstring>
#include <errno.h>   // 引入 errno 以捕获 SELinux 拦截原因

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
static std::string g_HookStatusMsg = "等待系统指令录入... (请在大厅执行挂载)";

static bool g_IsCompatibilityMode = true; 

struct {
    float x = 0.0f;
    float y = 0.0f;
    std::atomic<bool> down{false};
    float renderW = 0.0f;
    float renderH = 0.0f;
    float physW = 3392.0f;
    float physH = 2400.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
} g_Touch;

static float g_DynamicScale = 1.0f;
static float g_UserUIScale = 1.0f;

static uintptr_t g_GameBase = 0;
static uintptr_t g_HookAddr_InitActor = 0;

static std::atomic<uintptr_t> g_CapturedX0{0};
static std::atomic<uintptr_t> g_CapturedX1{0};
static std::atomic<uintptr_t> g_CapturedX2{0};

static std::vector<uintptr_t> g_ExploreHistory;
static uintptr_t g_CurrentExploreAddr = 0;
static uintptr_t g_LastRootBase = 0;
static bool g_ForceDirectRead = false;

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

// ==========================================
// 内存校验终极方案：mincore (无视 SELinux 限制)
// ==========================================
template <typename T>
T SafeRead(uintptr_t address) {
    T value{};
    if (address < 0x10000000 || address > 0x00007FFFFFFFFFFF) return value;

    // 使用 mincore 验证内存页是否被映射，无需任何文件系统权限，纯内存校验！
    unsigned char vec[1];
    uintptr_t pageStart = address & ~0xFFF; // 4K 内存页对齐
    if (mincore(reinterpret_cast<void*>(pageStart), 4096, vec) != 0) {
        return value; // 内存未映射 (ENOMEM) 或拒绝访问，直接返回空，绝不闪退
    }

    // 确定内存安全后，直接极速解引用
    return *(volatile T*)address; 
}

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
                        uintptr_t item = SafeRead<uintptr_t>(addr2 + 0x20 + (i * 0x8));
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

struct ModuleInfo { const char* name; uintptr_t base; };

static int dl_iterate_callback(struct dl_phdr_info *info, size_t size, void *data) {
    ModuleInfo* modInfo = (ModuleInfo*)data;
    if (info->dlpi_name && strstr(info->dlpi_name, modInfo->name)) {
        modInfo->base = info->dlpi_addr; return 1;
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

typedef void (*t_InitActorParams)(void* x0, void* x1, void* x2, void* x3, void* x4, void* x5, void* x6, void* x7);
t_InitActorParams old_InitActorParams = nullptr;

void hook_InitActorParams(void* x0, void* x1, void* x2, void* x3, void* x4, void* x5, void* x6, void* x7) {
    g_InitCallCount++; 
    if (x0 != nullptr && (uintptr_t)x0 > 0x10000000) { 
        g_ActorManager.store((uintptr_t)x0);
    }
    if (old_InitActorParams) {
        old_InitActorParams(x0, x1, x2, x3, x4, x5, x6, x7);
    }
}

void InstallCoreHook() {
    if (g_IsHookInstalled) return;

    uintptr_t base = GetRealModuleBase(GAME_MODULE);
    if (!base) {
        g_HookStatusMsg = "ERR: 未找到 libil2cpp.so 游戏未启动完全";
        return;
    }
    
    g_GameBase = base;
    g_HookAddr_InitActor = base + 0x73507bc;

    void* page_start = (void*)(g_HookAddr_InitActor & ~0xFFF);
    if (mprotect(page_start, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        LOGE("mprotect 授权失败! (可能是 SELinux 拦截)。错误码: %d", errno);
    }

    int ret = DobbyHook((void*)g_HookAddr_InitActor, (void*)hook_InitActorParams, (void**)&old_InitActorParams);
    __builtin___clear_cache((char*)g_HookAddr_InitActor, (char*)(g_HookAddr_InitActor + 16));

    if (ret == 0) {
        g_IsHookInstalled = true;
        g_HookStatusMsg = "✅ 核心态链路挂载成功！";
    } else {
        g_HookStatusMsg = "❌ 挂载被拒绝(SELinux限制): " + std::to_string(ret);
    }
}

// ============================================================================
// 科技风着色器
// ============================================================================
void ApplyCyberpunkTechStyle(ImGuiStyle& style) {
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.85f, 0.95f, 1.00f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.09f, 0.96f);
    colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.80f, 1.00f, 0.60f); 
    colors[ImGuiCol_FrameBg]                = ImVec4(0.10f, 0.14f, 0.22f, 0.80f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.18f, 0.35f, 0.58f, 0.80f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.22f, 0.45f, 0.75f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.08f, 0.20f, 0.38f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.00f, 1.00f, 0.80f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.00f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.00f, 1.00f, 0.80f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.10f, 0.30f, 0.55f, 0.80f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.15f, 0.40f, 0.70f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.25f, 0.55f, 0.85f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.12f, 0.25f, 0.45f, 0.80f);

    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding      = 0.0f;
}

void DrawPointerDebugger(const GameSnapshot& snap) {
    if (ImGui::Begin("内存探针 [已开启自动兼容]", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Checkbox("开启暴力裸读 (防止反作弊异常)", &g_ForceDirectRead);
        ImGui::Separator();
        
        uintptr_t rootBase = g_ActorManager.load();
        ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "当前捕获基址(x0): 0x%lx", rootBase);
        
        ImGui::Separator();
        std::lock_guard<std::mutex> lock(g_OffsetMutex);
        ImGui::Text("偏移量调整矩阵:");
        if (ImGui::Button("-4##x0")) { g_Offsets.x0_to_addr1 -= 4; } ImGui::SameLine();
        if (ImGui::Button("+4##x0")) { g_Offsets.x0_to_addr1 += 4; } ImGui::SameLine();
        ImGui::Text("x0_to_addr1: 0x%X", g_Offsets.x0_to_addr1);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0,1,0,1), "实体层级1: 0x%lx", snap.debug.addr1);
        ImGui::TextColored(ImVec4(0,1,0,1), "实体层级2: 0x%lx", snap.debug.addr2);
    }
    ImGui::End();
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
        LOGI("检测到首次渲染，正在初始化终端...");
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGuiIO& io = ImGui::GetIO();
        
        io.IniFilename = nullptr; 
        ApplyCyberpunkTechStyle(ImGui::GetStyle());
        
        io.Fonts->AddFontDefault(); 

        ImFontConfig config;
        config.MergeMode = true; 
        const char* systemFonts[] = { "/system/fonts/SysSans-Hans-Regular.ttf", "/system/fonts/NotoSansCJKjp-Regular.otc", "/system/fonts/DroidSansFallback.ttf" };
        for (int i = 0; i < 3; i++) {
            if (access(systemFonts[i], R_OK) == 0) {
                io.Fonts->AddFontFromFileTTF(systemFonts[i], 32.0f, &config, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
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
    io.FontGlobalScale = 1.2f * finalScale;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_Touch.down.load()) {
        ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 20.0f * finalScale, IM_COL32(0, 200, 255, 120));
        ImGui::GetForegroundDrawList()->AddCircle(io.MousePos, 28.0f * finalScale, IM_COL32(0, 200, 255, 200), 0, 2.0f);
    }

    static bool show_pointer_debugger = false;
    static bool show_radar = true;

    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("CYBER-CORE 战术终端", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        
        if (g_IsCompatibilityMode) {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "⚠ [系统警告] SELinux=Enforcing 状态检测！");
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1), "如果 Hook 失败，请使用 APatch 的 ZygiskNext 模块或设置 Permissive。");
            ImGui::Separator();
        }

        if (!g_IsHookInstalled) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.2f, 1.0f));
            if (ImGui::Button("🔴 建立挂载 (Kernel Hook)", ImVec2(280 * finalScale, 45 * finalScale))) InstallCoreHook();
            ImGui::PopStyleColor();
        } else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ [ ONLINE ] 核心链路稳定连接中");
        }
        ImGui::Text("终端日志: %s", g_HookStatusMsg.c_str());
        
        ImGui::Separator();
        
        ImGui::PushItemWidth(200.0f * finalScale);
        ImGui::SliderFloat("UI ZOOM (终端缩放)", &g_UserUIScale, 0.5f, 2.5f);
        ImGui::PopItemWidth();
        
        ImGui::Separator();
        ImGui::Checkbox("启动全息雷达 (HUD Radar)", &show_radar);
        ImGui::Checkbox("启动内存探针 (Debugger)", &show_pointer_debugger);
        ImGui::Separator();
        
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "DATA_STREAM // 实体数据流");
        if (!snapshot.inMatch) {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "等待有效战场数据载入...");
        } else {
            for (const auto& p : snapshot.players) {
                ImGui::Text("Lv.%-2d | HP: %d/%d | MP: %-4d | Gold: %-4d | 坐标: %.0f, %.0f", 
                            p.level, p.hp, p.maxHp, p.mana, p.gold, p.mapX, p.mapY);
            }
        }
    }
    ImGui::End();

    if (show_pointer_debugger) DrawPointerDebugger(snapshot);

    if (show_radar && snapshot.inMatch) {
        ImGui::SetNextWindowPos(ImVec2(w - 350, 50), ImGuiCond_FirstUseEver);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.05f, 0.08f, 0.85f));
        
        if (ImGui::Begin("HUD_Radar", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Dummy(ImVec2(260 * finalScale, 260 * finalScale)); 
            
            ImVec2 p0 = ImGui::GetItemRectMin();
            ImVec2 size = ImGui::GetItemRectSize();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 center = ImVec2(p0.x + size.x * 0.5f, p0.y + size.y * 0.5f);

            dl->AddRect(p0, ImVec2(p0.x + size.x, p0.y + size.y), IM_COL32(0, 180, 255, 150), 0, 0, 1.5f);
            dl->AddLine(ImVec2(center.x, p0.y), ImVec2(center.x, p0.y + size.y), IM_COL32(0, 150, 255, 80));
            dl->AddLine(ImVec2(p0.x, center.y), ImVec2(p0.x + size.x, center.y), IM_COL32(0, 150, 255, 80));
            dl->AddCircle(center, size.x * 0.25f, IM_COL32(0, 200, 255, 60), 32, 1.0f);

            for (const auto& p : snapshot.players) {
                float nX = p.mapX / 10000.0f;
                float nY = p.mapY / 10000.0f;
                float rX = p0.x + (nX * size.x);
                float rY = p0.y + size.y - (nY * size.y);
                
                if (nX > 0 && nX < 1 && nY > 0 && nY < 1) {
                    dl->AddCircleFilled(ImVec2(rX, rY), 5.0f * finalScale, IM_COL32(255, 50, 50, 150));
                }
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
    sleep(2);
    LOGI("底层初始化：挂载 EGL SwapBuffers...");
    
    void* egl = nullptr;
    int retry = 0;
    while (!egl && retry < 10) { 
        egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers"); 
        if (!egl) { sleep(1); retry++; }
    }
    
    if(egl) {
        int ret = DobbyHook(egl, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
        if (ret != 0) LOGE("EGL Hook 失败！错误码: %d. SELinux 强制模式可能拦截了 libEGL 的修改(execmod)。", ret);
    }

    LOGI("底层初始化：挂载 InputConsumer...");
    void* ins = nullptr;
    retry = 0;
    while (!ins && retry < 10) { 
        ins = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE"); 
        if (!ins) { sleep(1); retry++; }
    }
    if(ins) DobbyHook(ins, (void*)hook_consume, (void**)&old_consume);

    std::thread(DataWorkerThread).detach();
    return nullptr;
}

__attribute__((constructor)) void init_plugin() {
    pthread_t t;
    pthread_create(&t, nullptr, DelayedHookThread, nullptr);
}
