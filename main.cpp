#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include <fcntl.h>
#include <dirent.h>

#include <cstdio>
#include <algorithm>
#include <atomic>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <chrono>
#include <memory>
#include <string>

#include "dobby.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

namespace Overlay {

// ============================================================================
// 常量定义
// ============================================================================
constexpr const char* kTag = "IL2CPP_Drive";
constexpr const char* kGameModule = "libil2cpp.so";
constexpr size_t kMaxLogSize = 30;
constexpr size_t kMaxPlayers = 15;

// ============================================================================
// 日志系统类 (封装)
// ============================================================================
class Logger {
public:
    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }

    void Print(const char* level, const char* fmt, ...) {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        __android_log_print(ANDROID_LOG_INFO, kTag, "%s%s", level, buf);

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_logs.size() >= kMaxLogSize) {
            m_logs.pop_front();
        }
        m_logs.emplace_back(buf);
    }

    const std::deque<std::string>& GetLogs() const { return m_logs; }
    std::mutex& GetMutex() { return m_mutex; }

private:
    Logger() = default;
    std::deque<std::string> m_logs;
    std::mutex m_mutex;
};

#define LOG_INFO(...) Overlay::Logger::GetInstance().Print("", __VA_ARGS__)
#define LOG_ERROR(...) Overlay::Logger::GetInstance().Print("[ERROR] ", __VA_ARGS__)

// ============================================================================
// 内存读取工具类 (封装与安全保障)
// ============================================================================
class MemoryReader {
public:
    static bool IsValidAddress(uintptr_t address) {
        if (address < 0x10000000 || address > 0x00007FFFFFFFFFFF) return false;
        
        static int devRandomFd = open("/dev/random", O_WRONLY);
        if (devRandomFd < 0) {
            devRandomFd = open("/dev/null", O_WRONLY);
        }
        
        ssize_t ret = write(devRandomFd, reinterpret_cast<void*>(address), 4);
        return ret >= 0;
    }

    template <typename T>
    static T SafeRead(uintptr_t address) {
        if (!IsValidAddress(address)) return T{};
        return *reinterpret_cast<volatile T*>(address);
    }

    template <typename T>
    static T FastRead(uintptr_t address) {
        return *reinterpret_cast<volatile T*>(address);
    }
};

// ============================================================================
// 游戏数据结构定义
// ============================================================================
struct DynamicOffsets {
    uint32_t x0ToAddr1    = 0x20;
    uint32_t addr1ToAddr2 = 0x10;
    uint32_t itemToAddr3  = 0x18;
    uint32_t addr3ToAddr4 = 0x30;
    uint32_t addr3ToAddr5 = 0x100;
    uint32_t addr3ToAddr6 = 0x70;

    uint32_t propGold   = 0x20;
    uint32_t propHp     = 0x30;
    uint32_t propMaxHp  = 0x32;   
    uint32_t propLevel  = 0x50;
    uint32_t propMana   = 0x28;   
};

struct PlayerData {
    int32_t hp = 0;
    int16_t maxHp = 0;
    int32_t mana = 0;
    int32_t gold = 0;
    int32_t level = 0;
    float worldX = 0.0f, worldY = 0.0f, worldZ = 0.0f;
    float mapX = 0.0f, mapY = 0.0f;
};

struct DebugItem {
    uintptr_t base = 0, addr3 = 0, addr4 = 0, addr5 = 0, addr6 = 0;
};

struct DebugChain {
    uintptr_t baseX0 = 0, addr1 = 0, addr2 = 0;
    DebugItem items[kMaxPlayers];
};

struct GameSnapshot {
    bool inMatch = false;
    DynamicOffsets offsets;
    DebugChain debug;
    std::vector<PlayerData> players;
};

// ============================================================================
// 触控管理类
// ============================================================================
class TouchManager {
public:
    static TouchManager& GetInstance() {
        static TouchManager instance;
        return instance;
    }

    void ProcessEvent(AInputEvent* event) {
        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
            int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
            
            float autoScaleX = m_renderW / m_physW;
            float autoScaleY = m_renderH / m_physH;
            
            m_x = (AMotionEvent_getX(event, 0) * autoScaleX) + m_offsetX;
            m_y = (AMotionEvent_getY(event, 0) * autoScaleY) + m_offsetY;
            
            if (action == AMOTION_EVENT_ACTION_DOWN) m_isDown.store(true);
            else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) m_isDown.store(false);
        }
    }

    void UpdateRenderSize(float w, float h) { m_renderW = w; m_renderH = h; }
    
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    bool IsDown() const { return m_isDown.load(); }
    
    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;
    float m_renderW = 0.0f;
    float m_renderH = 0.0f;

private:
    TouchManager() = default;
    float m_x = 0.0f, m_y = 0.0f;
    std::atomic<bool> m_isDown{false};
    float m_physW = 3392.0f;
    float m_physH = 2400.0f;
};

// ============================================================================
// 核心引擎引擎类 (单例，管理整个外挂生命周期)
// ============================================================================
class EngineCore {
public:
    static EngineCore& GetInstance() {
        static EngineCore instance;
        return instance;
    }

    void InitWorkerThread() {
        std::thread(&EngineCore::DataWorkerLoop, this).detach();
    }

    std::shared_ptr<GameSnapshot> GetLatestSnapshot() {
        std::lock_guard<std::mutex> lock(m_snapshotMutex);
        return m_currentSnapshot;
    }

    DynamicOffsets GetOffsets() {
        std::lock_guard<std::mutex> lock(m_offsetMutex);
        return m_offsets;
    }

    void SetOffsets(const DynamicOffsets& offsets) {
        std::lock_guard<std::mutex> lock(m_offsetMutex);
        m_offsets = offsets;
    }

    void SetActorManager(uintptr_t address) {
        m_actorManager.store(address);
    }

    std::mutex& GetOffsetMutex() { return m_offsetMutex; }
    DynamicOffsets& GetOffsetsRef() { return m_offsets; }

private:
    EngineCore() : m_currentSnapshot(std::make_shared<GameSnapshot>()) {}

    void DataWorkerLoop() {
        while (true) {
            auto newSnap = std::make_shared<GameSnapshot>();
            newSnap->offsets = GetOffsets();

            uintptr_t baseX0 = m_actorManager.load();
            if (baseX0 != 0 && MemoryReader::IsValidAddress(baseX0)) {
                newSnap->inMatch = true;
                newSnap->debug.baseX0 = baseX0;

                uintptr_t addr1 = MemoryReader::SafeRead<uintptr_t>(baseX0 + newSnap->offsets.x0ToAddr1);
                newSnap->debug.addr1 = addr1;
                
                if (addr1) {
                    uintptr_t addr2 = MemoryReader::SafeRead<uintptr_t>(addr1 + newSnap->offsets.addr1ToAddr2);
                    newSnap->debug.addr2 = addr2;
                    
                    if (addr2) {
                        ParsePlayers(newSnap, addr2);
                    }
                }
            }

            {
                std::lock_guard<std::mutex> lock(m_snapshotMutex);
                m_currentSnapshot = newSnap;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    void ParsePlayers(std::shared_ptr<GameSnapshot>& snap, uintptr_t addr2) {
        for (size_t i = 0; i < kMaxPlayers; ++i) {
            uintptr_t item = MemoryReader::SafeRead<uintptr_t>(addr2 + 0x20 + (i * 0x8));
            snap->debug.items[i].base = item;
            if (!item) continue;

            uintptr_t addr3 = MemoryReader::SafeRead<uintptr_t>(item + snap->offsets.itemToAddr3);
            snap->debug.items[i].addr3 = addr3;
            if (!addr3) continue;

            PlayerData p;
            
            uintptr_t addr4 = MemoryReader::SafeRead<uintptr_t>(addr3 + snap->offsets.addr3ToAddr4);
            snap->debug.items[i].addr4 = addr4;
            if (addr4) {
                p.gold  = MemoryReader::FastRead<int32_t>(addr4 + snap->offsets.propGold);
                p.hp    = MemoryReader::FastRead<int32_t>(addr4 + snap->offsets.propHp);
                p.maxHp = MemoryReader::FastRead<int16_t>(addr4 + snap->offsets.propMaxHp);
                p.level = MemoryReader::FastRead<int32_t>(addr4 + snap->offsets.propLevel);
            }

            uintptr_t addr5 = MemoryReader::SafeRead<uintptr_t>(addr3 + snap->offsets.addr3ToAddr5);
            snap->debug.items[i].addr5 = addr5;
            if (addr5) {
                p.worldX = MemoryReader::FastRead<float>(addr5 + 0x0);
                p.worldZ = MemoryReader::FastRead<float>(addr5 + 0x4);
                p.worldY = MemoryReader::FastRead<float>(addr5 + 0x8);
                p.mana   = MemoryReader::FastRead<int32_t>(addr5 + snap->offsets.propMana);
            }

            uintptr_t addr6 = MemoryReader::SafeRead<uintptr_t>(addr3 + snap->offsets.addr3ToAddr6);
            snap->debug.items[i].addr6 = addr6;
            if (addr6) {
                p.mapX = MemoryReader::FastRead<float>(addr6 + 0x50);
                p.mapY = MemoryReader::FastRead<float>(addr6 + 0x54);
            }

            if (p.maxHp > 0 && p.maxHp < 50000) {
                snap->players.push_back(p);
            }
        }
    }

    std::shared_ptr<GameSnapshot> m_currentSnapshot;
    std::mutex m_snapshotMutex;
    
    DynamicOffsets m_offsets;
    std::mutex m_offsetMutex;
    
    std::atomic<uintptr_t> m_actorManager{0};
};

// ============================================================================
// UI 渲染器 (分离界面绘制逻辑)
// ============================================================================
class UIRenderer {
public:
    static void RenderFrame(float w, float h, std::shared_ptr<GameSnapshot>& snap) {
        static bool initialized = false;
        if (!initialized) {
            InitializeImGui();
            initialized = true;
        }

        auto& touch = TouchManager::GetInstance();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(w, h);
        io.MousePos = ImVec2(touch.GetX(), touch.GetY());
        io.MouseDown[0] = touch.IsDown();

        float finalScale = std::max(1.0f, h / 1000.0f) * m_userScale;
        ImGuiStyle& style = ImGui::GetStyle();
        style = m_defaultStyle;
        style.ScaleAllSizes(1.2f * finalScale);
        style.WindowRounding = 12.0f;
        io.FontGlobalScale = 1.2f * finalScale;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        if (touch.IsDown()) {
            ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 20.0f, IM_COL32(0, 255, 0, 200));
        }

        DrawMainWindow(w, h, snap);
        
        if (m_showDebugger) DrawDebugger(w, h, snap);
        if (m_showRadar && snap->inMatch) DrawRadar(w, h, snap, finalScale);

        ImGui::Render();
    }

    static bool m_isHookInstalled;
    static std::string m_hookStatusMsg;

private:
    static float m_userScale;
    static ImGuiStyle m_defaultStyle;
    static bool m_showDebugger;
    static bool m_showRadar;

    static void InitializeImGui() {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGuiIO& io = ImGui::GetIO();
        m_defaultStyle = ImGui::GetStyle();
        
        const char* systemFonts[] = { "/system/fonts/SysSans-Hans-Regular.ttf", "/system/fonts/NotoSansCJKjp-Regular.otc" };
        for (const char* fontPath : systemFonts) {
            if (access(fontPath, R_OK) == 0) {
                io.Fonts->AddFontFromFileTTF(fontPath, 32.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                break;
            }
        }
    }

    static void DrawMainWindow(float w, float h, std::shared_ptr<GameSnapshot>& snap) {
        ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(w * 0.6f, h * 0.45f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("IL2CPP 性能极客版透视框架 [重构版]")) {
            ImGui::BeginChild("HookController", ImVec2(0, 70), true);
            if (!m_isHookInstalled) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("🔴 注入基址", ImVec2(-1, 40))) {
                    extern void InstallHooks();
                    InstallHooks();
                }
                ImGui::PopStyleColor();
            } else {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ Hook 注入完成！");
            }
            ImGui::Text("状态: %s", m_hookStatusMsg.c_str());
            ImGui::EndChild();

            if (ImGui::CollapsingHeader("基础校准")) {
                ImGui::SliderFloat("菜单缩放", &m_userScale, 0.5f, 2.0f);
                ImGui::Checkbox("雷达 (Radar)", &m_showRadar); ImGui::SameLine();
                ImGui::Checkbox("指针断点调试", &m_showDebugger);
            }
            
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "状态: %s", snap->inMatch ? "局内" : "等待数据...");

            if (ImGui::BeginTable("PlayersTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("等级"); ImGui::TableSetupColumn("血量");
                ImGui::TableSetupColumn("蓝量"); ImGui::TableSetupColumn("金币");
                ImGui::TableSetupColumn("坐标"); ImGui::TableHeadersRow();
                for (const auto& p : snap->players) {
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
    }

    static void DrawRadar(float w, float h, std::shared_ptr<GameSnapshot>& snap, float finalScale) {
        ImGui::SetNextWindowPos(ImVec2(w - 350, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
        
        if (ImGui::Begin("Radar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImVec2 size = ImGui::GetContentRegionAvail();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRect(p0, ImVec2(p0.x + size.x, p0.y + size.y), IM_COL32(255, 255, 255, 100));
            
            for (const auto& p : snap->players) {
                float nX = p.mapX / 10000.0f;
                float nY = p.mapY / 10000.0f;
                float rX = p0.x + (nX * size.x);
                float rY = p0.y + size.y - (nY * size.y);
                if (nX > 0 && nX < 1 && nY > 0 && nY < 1) {
                    dl->AddCircleFilled(ImVec2(rX, rY), 5.0f * finalScale, IM_COL32(255, 0, 0, 255));
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
    }

    static void DrawDebugger(float w, float h, std::shared_ptr<GameSnapshot>& snap) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(w * 0.65f, h * 0.85f), ImGuiCond_FirstUseEver); 
        
        if (ImGui::Begin("指针断点排错器")) {
            if (ImGui::CollapsingHeader("动态偏移修改面板 (十进制)", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& engine = EngineCore::GetInstance();
                std::lock_guard<std::mutex> lock(engine.GetOffsetMutex());
                auto& offsets = engine.GetOffsetsRef();
                
                auto drawOffsetBtn = [](const char* label, uint32_t& val) {
                    ImGui::PushID(&val);
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s -> 当前: %d (0x%X)", label, val, val);
                    if (ImGui::Button("-4")) { if (val >= 4) val -= 4; else val = 0; } ImGui::SameLine();
                    if (ImGui::Button("+4")) val += 4;
                    ImGui::Separator();
                    ImGui::PopID();
                };
                
                drawOffsetBtn("地址1", offsets.x0ToAddr1);
                drawOffsetBtn("英雄数组", offsets.addr1ToAddr2);
                drawOffsetBtn("实体数据", offsets.itemToAddr3);
            }
            ImGui::Separator();
            
            if (ImGui::CollapsingHeader("实时运行日志", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::BeginChild("LogRegion", ImVec2(0, 120), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                auto& logger = Logger::GetInstance();
                std::lock_guard<std::mutex> lock(logger.GetMutex());
                for (const auto& log : logger.GetLogs()) { ImGui::TextUnformatted(log.c_str()); }
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
                ImGui::EndChild();
            }
        }
        ImGui::End();
    }
};

float UIRenderer::m_userScale = 1.0f;
ImGuiStyle UIRenderer::m_defaultStyle;
bool UIRenderer::m_showDebugger = false;
bool UIRenderer::m_showRadar = true;
bool UIRenderer::m_isHookInstalled = false;
std::string UIRenderer::m_hookStatusMsg = "等待点击注入...";

// ============================================================================
// Dobby Hook 回调函数与模块基址寻找 (C 风格回调区)
// ============================================================================
struct ModuleInfo {
    const char* name;
    uintptr_t base;
};

int DlIterateCallback(struct dl_phdr_info *info, size_t size, void *data) {
    auto* modInfo = reinterpret_cast<ModuleInfo*>(data);
    if (info->dlpi_name && strstr(info->dlpi_name, modInfo->name)) {
        modInfo->base = info->dlpi_addr;
        return 1;
    }
    return 0;
}

uintptr_t GetModuleBase(const char* moduleName) {
    ModuleInfo info = {moduleName, 0};
    dl_iterate_phdr(DlIterateCallback, &info);
    
    if (info.base == 0) {
        if (FILE* f = fopen("/proc/self/maps", "r")) {
            char line[512];
            while (fgets(line, sizeof(line), f)) {
                if (strstr(line, moduleName)) {
                    uintptr_t start;
                    if (sscanf(line, "%lx", &start) == 1) { info.base = start; break; }
                }
            }
            fclose(f);
        }
    }
    return info.base;
}

typedef void (*InitActorParamsFunc)(void* x0, void* x1, void* x2, void* x3, void* x4, void* x5, void* x6, void* x7);
static InitActorParamsFunc g_oldInitActorParams = nullptr;

void HookInitActorParams(void* x0, void* x1, void* x2, void* x3, void* x4, void* x5, void* x6, void* x7) {
    if (x0 != nullptr && reinterpret_cast<uintptr_t>(x0) > 0x10000000) { 
        EngineCore::GetInstance().SetActorManager(reinterpret_cast<uintptr_t>(x0));
    }
    if (g_oldInitActorParams) {
        g_oldInitActorParams(x0, x1, x2, x3, x4, x5, x6, x7);
    }
}

void InstallHooks() {
    if (UIRenderer::m_isHookInstalled) return;

    uintptr_t base = GetModuleBase(kGameModule);
    if (!base) {
        UIRenderer::m_hookStatusMsg = "未找到 libil2cpp.so！";
        LOG_ERROR("未找到目标游戏模块模块！");
        return;
    }
    
    uintptr_t targetAddr = base + 0x73507bc;
    LOG_INFO("真实基址: 0x%lx, 目标: 0x%lx", base, targetAddr);

    void* pageStart = reinterpret_cast<void*>(targetAddr & ~0xFFF);
    mprotect(pageStart, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);

    int ret = DobbyHook(reinterpret_cast<void*>(targetAddr), 
                        reinterpret_cast<void*>(HookInitActorParams), 
                        reinterpret_cast<void**>(&g_oldInitActorParams));

    if (ret == 0) {
        UIRenderer::m_isHookInstalled = true;
        UIRenderer::m_hookStatusMsg = "✅ 核心 Hook 注入成功！";
        LOG_INFO("手动安装 Hook 成功！");
    } else {
        UIRenderer::m_hookStatusMsg = "❌ 挂钩失败";
        LOG_ERROR("挂钩失败, 返回码: %d", ret);
    }
}

// ---------------- EGL & Input Hook ----------------
typedef int (*InputConsumerConsumeFunc)(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent);
static InputConsumerConsumeFunc g_oldConsume = nullptr;

int HookConsume(void* instance, void* factory, bool consumeBatches, int64_t frameTime, uint32_t* outSeq, void** outEvent) {
    int res = g_oldConsume(instance, factory, consumeBatches, frameTime, outSeq, outEvent);
    if (res == 0 && outEvent && *outEvent) {
        TouchManager::GetInstance().ProcessEvent(reinterpret_cast<AInputEvent*>(*outEvent));
    }
    return res;
}

typedef EGLBoolean (*EglSwapBuffersFunc)(EGLDisplay dpy, EGLSurface surface);
static EglSwapBuffersFunc g_oldEglSwapBuffers = nullptr;

EGLBoolean HookEglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    EGLint w = 0, h = 0;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    
    if (w <= 0 || h <= 0) return g_oldEglSwapBuffers(dpy, surface);
    
    TouchManager::GetInstance().UpdateRenderSize(static_cast<float>(w), static_cast<float>(h));
    auto snapshot = EngineCore::GetInstance().GetLatestSnapshot();

    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
    GLint depthTest; glGetIntegerv(GL_DEPTH_TEST, &depthTest);
    
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, w, h);

    UIRenderer::RenderFrame(static_cast<float>(w), static_cast<float>(h), snapshot);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    if (depthTest) glEnable(GL_DEPTH_TEST);

    return g_oldEglSwapBuffers(dpy, surface);
}

void* BootstrapThread(void*) {
    void* eglSym = nullptr;
    while (!(eglSym = DobbySymbolResolver("libEGL.so", "eglSwapBuffers"))) { sleep(1); }
    DobbyHook(eglSym, reinterpret_cast<void*>(HookEglSwapBuffers), reinterpret_cast<void**>(&g_oldEglSwapBuffers));

    void* inputSym = nullptr;
    while (!(inputSym = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE"))) { sleep(1); }
    DobbyHook(inputSym, reinterpret_cast<void*>(HookConsume), reinterpret_cast<void**>(&g_oldConsume));

    EngineCore::GetInstance().InitWorkerThread();
    return nullptr;
}

} // namespace Overlay

// ============================================================================
// 模块入口点
// ============================================================================
__attribute__((constructor)) void InitializePlugin() {
    pthread_t threadId;
    pthread_create(&threadId, nullptr, Overlay::BootstrapThread, nullptr);
}
