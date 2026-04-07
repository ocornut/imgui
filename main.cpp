#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <android/input.h>
#include <fcntl.h>
#include <dirent.h>
#include <link.h>
#include <sys/mman.h>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <cstdlib>
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
constexpr size_t kMaxPlayers = 10;

// ============================================================================
// ★ W2S 核心数学引擎 (列优先 Column-Major)
// ============================================================================
struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };

// 采用一维数组存储 16 个浮点数，完美契合 Unity 列优先内存布局
struct Matrix4x4 {
    float m[16]; 
};

// 矩阵乘法：计算 Proj * View
Matrix4x4 MultiplyMatrix(const Matrix4x4& a, const Matrix4x4& b) {
    Matrix4x4 res;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            res.m[c * 4 + r] = 
                a.m[0 * 4 + r] * b.m[c * 4 + 0] +
                a.m[1 * 4 + r] * b.m[c * 4 + 1] +
                a.m[2 * 4 + r] * b.m[c * 4 + 2] +
                a.m[3 * 4 + r] * b.m[c * 4 + 3];
        }
    }
    return res;
}

// 核心 W2S 算法：世界 3D 坐标转换为屏幕 2D 像素坐标 (严格列优先计算)
bool WorldToScreen(const Vector3& pos, const Matrix4x4& viewProj, Vector2& screenPos, float screenW, float screenH) {
    // 1. 计算裁剪空间的 W (深度/Z轴)
    float clipW = pos.x * viewProj.m[3] + pos.y * viewProj.m[7] + pos.z * viewProj.m[11] + viewProj.m[15];
    
    // 如果 W < 0.1，说明物体在摄像机背面或紧贴镜头，不进行绘制
    if (clipW < 0.1f) return false;

    // 2. 计算裁剪空间的 X 和 Y
    float clipX = pos.x * viewProj.m[0] + pos.y * viewProj.m[4] + pos.z * viewProj.m[8] + viewProj.m[12];
    float clipY = pos.x * viewProj.m[1] + pos.y * viewProj.m[5] + pos.z * viewProj.m[9] + viewProj.m[13];

    // 3. 透视除法，转换为 NDC (标准化设备坐标) [-1.0, 1.0]
    float ndcX = clipX / clipW;
    float ndcY = clipY / clipW;

    // 4. 映射到屏幕物理像素坐标
    screenPos.x = (screenW / 2.0f) * (ndcX + 1.0f);
    // Unity 和大多数引擎中，屏幕 Y 轴向下递增，NDC Y 轴向上递增，所以用 1.0f - ndcY 反转
    screenPos.y = (screenH / 2.0f) * (1.0f - ndcY); 

    return true;
}

// ============================================================================
// 日志系统类
// ============================================================================
class Logger {
public:
    static Logger& GetInstance() { static Logger instance; return instance; }

    void Print(const char* level, const char* fmt, ...) {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        __android_log_print(ANDROID_LOG_INFO, kTag, "%s%s", level, buf);

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_logs.size() >= kMaxLogSize) m_logs.pop_front();
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
// 内存读取工具类 (Mincore 防闪退方案)
// ============================================================================
class MemoryReader {
public:
    static bool IsValidAddress(uintptr_t address) {
        if (address < 0x10000000 || address > 0x00007FFFFFFFFFFF) return false;
        unsigned char vec[1];
        uintptr_t pageStart = address & ~0xFFF; 
        if (mincore(reinterpret_cast<void*>(pageStart), 4096, vec) != 0) return false; 
        return true;
    }

    template <typename T>
    static T SafeRead(uintptr_t address) {
        if (!IsValidAddress(address)) return T{};
        return *reinterpret_cast<T*>(address);
    }

    template <typename T>
    static T FastRead(uintptr_t address) {
        return *reinterpret_cast<T*>(address);
    }
};

// ============================================================================
// 偏移结构体
// ============================================================================
struct DynamicOffsets {
    uint32_t x0ToAddr1    = 0x18; 
    uint32_t addr1ToAddr2 = 0x8;   
    uint32_t arrayStart   = 0x20;  
    uint32_t arrayStep    = 0x10;  
    uint32_t itemToAddr3  = 0x0;   
    
    uint32_t addr3ToWorld = 0x4C4; 
    uint32_t addr3ToAddr4 = 0x3B8; 
    
    uint32_t addr4ToGold  = 0x30;  
    uint32_t addr4ToHp    = 0x38;  
    uint32_t addr4ToMaxHp = 0x3C;  
    uint32_t addr4ToLevel = 0x58;  
    uint32_t addr4ToAddr5 = 0x50;  
    
    uint32_t addr5ToMana  = 0x24;  
    uint32_t addr5ToMaxMana = 0x28;

    // ★ 同步你提供的最新相机矩阵偏移
    uint32_t cameraViewOffset = 0x4C; // WorldToCameraMatrix
    uint32_t cameraProjOffset = 0x8C; // ProjectionMatrix
};

struct PlayerData {
    int32_t hp = 0, maxHp = 0, mana = 0, maxMana = 0, gold = 0, level = 0;
    Vector3 worldPos; 
    float mapX = 0.0f, mapY = 0.0f;
};

struct DebugItem { uintptr_t base = 0, addr3 = 0, addr4 = 0, addr5 = 0; };
struct DebugChain { uintptr_t baseX0 = 0, addr1 = 0, addr2 = 0; DebugItem items[kMaxPlayers]; };

struct GameSnapshot {
    bool inMatch = false;
    DynamicOffsets offsets;
    DebugChain debug;
    std::vector<PlayerData> players;
    Matrix4x4 viewMatrix; 
    Matrix4x4 rawViewMatrix; // ★ 新增：保存原始 View 矩阵用于格式化打印
    Matrix4x4 rawProjMatrix; // ★ 新增：保存原始 Proj 矩阵用于格式化打印
};

// ============================================================================
// 触控管理类
// ============================================================================
class TouchManager {
public:
    static TouchManager& GetInstance() { static TouchManager instance; return instance; }

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
    
    float m_offsetX = 0.0f, m_offsetY = 0.0f;
    float m_renderW = 0.0f, m_renderH = 0.0f;

private:
    TouchManager() = default;
    float m_x = 0.0f, m_y = 0.0f;
    std::atomic<bool> m_isDown{false};
    float m_physW = 3392.0f, m_physH = 2400.0f;
};

// ============================================================================
// 核心数据引擎
// ============================================================================
class EngineCore {
public:
    static EngineCore& GetInstance() { static EngineCore instance; return instance; }
    void InitWorkerThread() { std::thread(&EngineCore::DataWorkerLoop, this).detach(); }

    std::shared_ptr<GameSnapshot> GetLatestSnapshot() {
        std::lock_guard<std::mutex> lock(m_snapshotMutex);
        return m_currentSnapshot;
    }

    DynamicOffsets GetOffsets() { std::lock_guard<std::mutex> lock(m_offsetMutex); return m_offsets; }
    void SetActorManager(uintptr_t address) { m_actorManager.store(address); }
    void SetCameraMatrixPtr(uintptr_t address) { m_cameraMatrixPtr.store(address); }
    uintptr_t GetCameraMatrixPtr() { return m_cameraMatrixPtr.load(); }
    
    std::mutex& GetOffsetMutex() { return m_offsetMutex; }
    DynamicOffsets& GetOffsetsRef() { return m_offsets; }

private:
    EngineCore() : m_currentSnapshot(std::make_shared<GameSnapshot>()) {}

    void DataWorkerLoop() {
        while (true) {
            auto newSnap = std::make_shared<GameSnapshot>();
            newSnap->offsets = GetOffsets();

            // 1. 读取并融合摄像机矩阵 (自动获取)
            uintptr_t camBase = m_cameraMatrixPtr.load();
            if (camBase != 0 && MemoryReader::IsValidAddress(camBase)) {
                // 根据你提供的偏移读取列优先矩阵
                Matrix4x4 view = MemoryReader::SafeRead<Matrix4x4>(camBase + newSnap->offsets.cameraViewOffset);
                Matrix4x4 proj = MemoryReader::SafeRead<Matrix4x4>(camBase + newSnap->offsets.cameraProjOffset);
                
                // ★ 记录原始矩阵用于 UI 完美复刻截图输出
                newSnap->rawViewMatrix = view;
                newSnap->rawProjMatrix = proj;

                // 矩阵乘法：Projection * View
                newSnap->viewMatrix = MultiplyMatrix(proj, view); 
            } else {
                memset(&newSnap->viewMatrix, 0, sizeof(Matrix4x4));
                memset(&newSnap->rawViewMatrix, 0, sizeof(Matrix4x4));
                memset(&newSnap->rawProjMatrix, 0, sizeof(Matrix4x4));
            }

            // 2. 读取游戏英雄数据
            uintptr_t baseX0 = m_actorManager.load();
            if (baseX0 != 0 && MemoryReader::IsValidAddress(baseX0)) {
                newSnap->inMatch = true;
                newSnap->debug.baseX0 = baseX0;
                
                uintptr_t addr1 = MemoryReader::SafeRead<uintptr_t>(baseX0 + newSnap->offsets.x0ToAddr1);
                newSnap->debug.addr1 = addr1;
                if (addr1) {
                    uintptr_t addr2 = MemoryReader::SafeRead<uintptr_t>(addr1 + newSnap->offsets.addr1ToAddr2);
                    newSnap->debug.addr2 = addr2;
                    if (addr2) ParsePlayers(newSnap, addr2);
                }
            }

            {
                std::lock_guard<std::mutex> lock(m_snapshotMutex);
                m_currentSnapshot = newSnap;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60Hz 刷新率
        }
    }

    void ParsePlayers(std::shared_ptr<GameSnapshot>& snap, uintptr_t addr2) {
        for (size_t i = 0; i < kMaxPlayers; ++i) {
            uintptr_t itemAddr = addr2 + snap->offsets.arrayStart + (i * snap->offsets.arrayStep);
            snap->debug.items[i].base = itemAddr;

            uintptr_t addr3 = MemoryReader::SafeRead<uintptr_t>(itemAddr + snap->offsets.itemToAddr3);
            snap->debug.items[i].addr3 = addr3;
            if (!addr3) continue;

            PlayerData p;
            p.worldPos.x = MemoryReader::SafeRead<float>(addr3 + snap->offsets.addr3ToWorld + 0x0);
            p.worldPos.y = MemoryReader::SafeRead<float>(addr3 + snap->offsets.addr3ToWorld + 0x4);
            p.worldPos.z = MemoryReader::SafeRead<float>(addr3 + snap->offsets.addr3ToWorld + 0x8);
            
            p.mapX = p.worldPos.x; p.mapY = p.worldPos.z;

            uintptr_t addr4 = MemoryReader::SafeRead<uintptr_t>(addr3 + snap->offsets.addr3ToAddr4);
            snap->debug.items[i].addr4 = addr4;
            
            if (addr4) {
                p.gold  = MemoryReader::FastRead<int32_t>(addr4 + snap->offsets.addr4ToGold);
                p.hp    = MemoryReader::FastRead<int32_t>(addr4 + snap->offsets.addr4ToHp);
                p.maxHp = MemoryReader::FastRead<int32_t>(addr4 + snap->offsets.addr4ToMaxHp);
                p.level = MemoryReader::FastRead<int32_t>(addr4 + snap->offsets.addr4ToLevel);
                
                uintptr_t addr5 = MemoryReader::FastRead<uintptr_t>(addr4 + snap->offsets.addr4ToAddr5);
                snap->debug.items[i].addr5 = addr5;
                if (addr5) {
                    p.mana    = MemoryReader::FastRead<int32_t>(addr5 + snap->offsets.addr5ToMana);
                    p.maxMana = MemoryReader::FastRead<int32_t>(addr5 + snap->offsets.addr5ToMaxMana);
                }
            }
            if (p.maxHp > 0 && p.maxHp < 50000) snap->players.push_back(p);
        }
    }

    std::shared_ptr<GameSnapshot> m_currentSnapshot;
    std::mutex m_snapshotMutex;
    DynamicOffsets m_offsets;
    std::mutex m_offsetMutex;
    
    std::atomic<uintptr_t> m_actorManager{0};
    std::atomic<uintptr_t> m_cameraMatrixPtr{0}; 
};

// ============================================================================
// UI 渲染引擎 
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

        float deviceScale = std::min(2.0f, std::max(0.5f, h / 1080.0f));
        float finalScale = deviceScale * m_userScale;

        ImGuiStyle& style = ImGui::GetStyle();
        style = m_defaultStyle; 
        style.ScaleAllSizes(finalScale); 
        io.FontGlobalScale = finalScale; 

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        // === 3D 屏幕透视绘制引擎 (Draw ESP) ===
        // m[15] (第四行第四列) 不为0说明矩阵已经成功读取并融合
        if (m_showESP && snap->inMatch && snap->viewMatrix.m[15] != 0.0f) {
            DrawESP(w, h, snap, finalScale);
        }

        if (touch.IsDown()) {
            ImGui::GetForegroundDrawList()->AddCircleFilled(io.MousePos, 15.0f * finalScale, IM_COL32(0, 255, 255, 120));
            ImGui::GetForegroundDrawList()->AddCircle(io.MousePos, 22.0f * finalScale, IM_COL32(0, 255, 255, 200), 0, 2.0f);
        }

        if (m_showMenu) DrawMainWindow(w, h, snap, finalScale);
        if (m_showDebugger) DrawDebugger(w, h, snap, finalScale);
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
    static bool m_showESP;     
    static bool m_showMenu;    

    static void ApplyTechStyle(ImGuiStyle& style) {
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.95f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.05f, 0.05f, 0.08f, 0.94f); 
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.12f, 0.98f);
        colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.60f, 0.80f, 0.60f); 
        colors[ImGuiCol_FrameBg]                = ImVec4(0.10f, 0.14f, 0.22f, 0.70f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.15f, 0.35f, 0.55f, 0.80f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.45f, 0.75f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.03f, 0.03f, 0.05f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.05f, 0.15f, 0.30f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.00f, 0.95f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.00f, 0.75f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.00f, 0.95f, 1.00f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.08f, 0.25f, 0.45f, 0.80f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.12f, 0.40f, 0.65f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.55f, 0.85f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.10f, 0.25f, 0.45f, 0.80f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.15f, 0.35f, 0.60f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.45f, 0.75f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.15f, 0.30f, 0.50f, 0.80f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.08f, 0.15f, 0.25f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.10f, 0.30f, 0.50f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.10f, 0.20f, 0.35f, 0.50f);

        style.WindowPadding     = ImVec2(16.0f, 16.0f);
        style.FramePadding      = ImVec2(12.0f, 8.0f);
        style.CellPadding       = ImVec2(10.0f, 8.0f);
        style.ItemSpacing       = ImVec2(12.0f, 10.0f);
        
        style.WindowRounding    = 4.0f;
        style.ChildRounding     = 2.0f;
        style.FrameRounding     = 2.0f;
        style.WindowBorderSize  = 1.0f;
        style.FrameBorderSize   = 0.0f;
    }

    static void InitializeImGui() {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr; 
        
        m_defaultStyle = ImGui::GetStyle();
        ApplyTechStyle(m_defaultStyle);
        
        io.Fonts->Clear();
        bool fontLoaded = false;
        const char* systemFonts[] = { 
            "/system/fonts/MiSans-Regular.ttf",      
            "/system/fonts/SysSans-Hans-Regular.ttf", 
            "/system/fonts/NotoSansCJKjp-Regular.otc", 
            "/system/fonts/NotoSansSC-Regular.otf",
            "/system/fonts/DroidSansFallback.ttf" 
        };
        for (const char* fontPath : systemFonts) {
            if (access(fontPath, R_OK) == 0) {
                io.Fonts->AddFontFromFileTTF(fontPath, 24.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                fontLoaded = true; break;
            }
        }
        if (!fontLoaded) io.Fonts->AddFontDefault(); 
    }

    static void DrawESP(float w, float h, std::shared_ptr<GameSnapshot>& snap, float scale) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList(); 
        ImVec2 screenBottomCenter(w / 2.0f, h); 

        for (const auto& p : snap->players) {
            if (p.hp <= 0 || p.maxHp <= 0) continue; 

            // 1. 底端坐标转换为屏幕坐标
            Vector2 screenFoot;
            if (!WorldToScreen(p.worldPos, snap->viewMatrix, screenFoot, w, h)) continue; 
            
            // 2. 头部坐标转换 (此处假设人物高度是 2.0f，你可以根据游戏实际模型适当调整)
            Vector3 headPos = { p.worldPos.x, p.worldPos.y + 2.0f, p.worldPos.z };
            Vector2 screenHead;
            if (!WorldToScreen(headPos, snap->viewMatrix, screenHead, w, h)) continue;

            // 3. 计算方框大小
            float boxHeight = screenFoot.y - screenHead.y;
            float boxWidth = boxHeight / 1.5f; // 高宽比 1.5:1

            if (screenFoot.y < 0 || screenHead.y > h || screenFoot.x + boxWidth/2 < 0 || screenFoot.x - boxWidth/2 > w) continue;

            ImVec2 topLeft(screenHead.x - boxWidth / 2, screenHead.y);
            ImVec2 bottomRight(screenFoot.x + boxWidth / 2, screenFoot.y);

            // 绘制科幻青色边框
            drawList->AddRect(topLeft, bottomRight, IM_COL32(0, 255, 255, 200), 0.0f, 0, 1.5f * scale);
            drawList->AddRect(topLeft, bottomRight, IM_COL32(0, 0, 0, 255), 0.0f, 0, 3.0f * scale); // 外阴影

            // 射线连线
            drawList->AddLine(screenBottomCenter, ImVec2(screenFoot.x, screenFoot.y), IM_COL32(255, 255, 255, 80), 1.0f * scale);

            // 血条渲染
            float hpRatio = (float)p.hp / (float)p.maxHp;
            ImVec2 hpTopLeft(topLeft.x - 10.0f * scale, bottomRight.y - boxHeight * hpRatio);
            ImVec2 hpBottomRight(topLeft.x - 4.0f * scale, bottomRight.y);
            
            drawList->AddRectFilled(ImVec2(topLeft.x - 10.0f * scale, topLeft.y), ImVec2(topLeft.x - 4.0f * scale, bottomRight.y), IM_COL32(0, 0, 0, 180));
            ImU32 hpColor = hpRatio > 0.5f ? IM_COL32(0, 255, 0, 255) : (hpRatio > 0.2f ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
            drawList->AddRectFilled(hpTopLeft, hpBottomRight, hpColor);

            // 顶部信息
            char info[64];
            snprintf(info, sizeof(info), "Lv.%d | G:%d", p.level, p.gold);
            ImVec2 textSize = ImGui::CalcTextSize(info);
            drawList->AddRectFilled(ImVec2(screenHead.x - textSize.x/2 - 4, screenHead.y - textSize.y - 6), ImVec2(screenHead.x + textSize.x/2 + 4, screenHead.y - 2), IM_COL32(0, 0, 0, 180));
            drawList->AddText(ImVec2(screenHead.x - textSize.x / 2, screenHead.y - textSize.y - 4), IM_COL32(255, 255, 255, 255), info);
        }
    }

    static void DrawMainWindow(float w, float h, std::shared_ptr<GameSnapshot>& snap, float finalScale) {
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(w * 0.50f, h * 0.55f), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("CYBER-CORE // 战术数据终端", &m_showMenu, ImGuiWindowFlags_NoCollapse)) {
            
            float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() + 5.0f;
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);
            
            ImGui::BeginChild("HookController", ImVec2(0, 85), true);
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "SYS_STATUS // 系统内核状态");
            ImGui::Separator();
            if (!m_isHookInstalled) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.15f, 0.25f, 1.0f));
                if (ImGui::Button("[ ⚠ ] 建立核心态神经链路 (Kernel Hook)", ImVec2(-1, 0))) { extern void InstallHooks(); InstallHooks(); }
                ImGui::PopStyleColor(1);
            } else {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.4f, 1.0f), "✅ [ ONLINE ] 核心链路稳定连接中...");
            }
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "LOG: %s", m_hookStatusMsg.c_str());
            ImGui::EndChild();

            if (ImGui::CollapsingHeader("TACTICAL_CALIBRATION // 模块校准", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("屏幕 ESP 透视", &m_showESP); ImGui::SameLine(0, 30.0f * finalScale);
                ImGui::Checkbox("HUD 小雷达", &m_showRadar); ImGui::SameLine(0, 30.0f * finalScale);
                ImGui::Checkbox("DBG 内存探针", &m_showDebugger);

                // 动态显示相机矩阵挂载状态
                if (snap->viewMatrix.m[15] == 0.0f) {
                    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "[!] 警告: ESP 等待相机矩阵抓取 (等待15秒自动同步)...");
                } else {
                    ImGui::TextColored(ImVec4(0, 1.0f, 0, 1), "[√] 相机矩阵已自动链接，W2S (ESP透视) 在线。");
                }
            }
            ImGui::Spacing();
            
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "DATA_STREAM // 实战数据解析");
            if (!snap->inMatch || snap->players.empty()) {
                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), ">> 等待战场实体同步...");
            } else {
                if (ImGui::BeginTable("PlayersTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("LVL", ImGuiTableColumnFlags_WidthFixed); 
                    ImGui::TableSetupColumn("HP"); ImGui::TableSetupColumn("MP"); 
                    ImGui::TableSetupColumn("GOLD"); ImGui::TableSetupColumn("POS(X, Y, Z)"); 
                    ImGui::TableHeadersRow();

                    for (const auto& p : snap->players) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "%d", p.level);
                        ImGui::TableNextColumn(); ImGui::Text("%d/%d", p.hp, p.maxHp);
                        ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(0, 0.8f, 1, 1), "%d/%d", p.mana, p.maxMana);
                        ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1), "%d", p.gold);
                        ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1), "%.1f, %.1f, %.1f", p.worldPos.x, p.worldPos.y, p.worldPos.z);
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::EndChild();

            ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y));
            ImGui::Separator();
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "[ STABLE ]");
            ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();
            
            ImGui::PushItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.4f, 0.6f, 0.4f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.0f, 0.9f, 1.0f, 1.0f));
            ImGui::SliderFloat("##GlobalScale", &m_userScale, 0.5f, 2.5f, "UI ZOOM: %.2fx");
            ImGui::PopStyleColor(2);
            ImGui::PopItemWidth();
        }
        ImGui::End();
    }

    static void DrawRadar(float w, float h, std::shared_ptr<GameSnapshot>& snap, float finalScale) {
        float radarSize = 300.0f * finalScale;
        ImGui::SetNextWindowPos(ImVec2(w - radarSize - 40, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(radarSize, radarSize), ImGuiCond_Always);
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.05f, 0.08f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.8f, 1.0f, 0.6f));
        
        if (ImGui::Begin("HUD_Radar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
            ImVec2 p0 = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 center = ImVec2(p0.x + size.x * 0.5f, p0.y + size.y * 0.5f);

            dl->AddLine(ImVec2(center.x, p0.y), ImVec2(center.x, p0.y + size.y), IM_COL32(0, 150, 255, 80));
            dl->AddLine(ImVec2(p0.x, center.y), ImVec2(p0.x + size.x, center.y), IM_COL32(0, 150, 255, 80));
            dl->AddCircle(center, size.x * 0.25f, IM_COL32(0, 200, 255, 60), 32, 1.0f);
            dl->AddCircle(center, size.x * 0.45f, IM_COL32(0, 200, 255, 60), 32, 1.0f);

            for (const auto& p : snap->players) {
                float nX = p.mapX / 10000.0f;
                float nY = p.mapY / 10000.0f;
                
                if (nX > 0 && nX < 1 && nY > 0 && nY < 1) {
                    float rX = p0.x + (nX * size.x);
                    float rY = p0.y + size.y - (nY * size.y); 
                    ImVec2 point(rX, rY);
                    dl->AddCircleFilled(point, 6.0f * finalScale, IM_COL32(255, 50, 50, 80)); 
                    dl->AddCircleFilled(point, 3.0f * finalScale, IM_COL32(255, 80, 80, 255)); 
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
    }

    static void DrawDebugger(float w, float h, std::shared_ptr<GameSnapshot>& snap, float finalScale) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(w * 0.50f, h * 0.7f), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("MEMORY_PROBE // 内存深度探查", &m_showDebugger)) {
            auto& engine = EngineCore::GetInstance();
            std::lock_guard<std::mutex> lock(engine.GetOffsetMutex());
            auto& offsets = engine.GetOffsetsRef();
            
            auto drawOffsetBtn = [](const char* label, uint32_t& val) {
                ImGui::PushID(&val);
                ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "%-20s: %d (0x%X)", label, val, val);
                ImGui::SameLine(ImGui::GetWindowWidth() - 120.0f);
                if (ImGui::Button("-4", ImVec2(40, 0))) { if (val >= 4) val -= 4; else val = 0; } ImGui::SameLine();
                if (ImGui::Button("+4", ImVec2(40, 0))) val += 4;
                ImGui::PopID();
            };
            
            if (ImGui::CollapsingHeader("OFFSET_MATRIX // 数据指针树", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "当前 x0 基址: 0x%lx", snap->debug.baseX0);
                drawOffsetBtn("x0_to_addr1", offsets.x0ToAddr1);
                drawOffsetBtn("addr1_to_addr2", offsets.addr1ToAddr2);
                drawOffsetBtn("arrayStart", offsets.arrayStart);
                drawOffsetBtn("item_to_addr3", offsets.itemToAddr3);
                drawOffsetBtn("addr3ToAddr4", offsets.addr3ToAddr4);
                drawOffsetBtn("addr4ToAddr5", offsets.addr4ToAddr5);
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "当前 Camera 基址: 0x%lx", engine.GetCameraMatrixPtr());
                drawOffsetBtn("cameraViewOffset", offsets.cameraViewOffset);
                drawOffsetBtn("cameraProjOffset", offsets.cameraProjOffset);
            }
            
            // ★ 新增：完全还原截图里的矩阵格式化输出
            if (ImGui::CollapsingHeader("MATRIX_DUMP // 内存矩阵专项测试输出", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto drawMatrix = [](const char* title, const Matrix4x4& mat, uint32_t offset) {
                    ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "[专项测试] 偏移 0x%X (疑似 %s)", offset, title);
                    // 按内存顺序(每4个浮点数一行)输出，完美复刻截图日志格式
                    for (int i = 0; i < 4; ++i) {
                        ImGui::Text("  %9.5f  %9.5f  %9.5f  %9.5f", 
                                    mat.m[i*4 + 0], mat.m[i*4 + 1], mat.m[i*4 + 2], mat.m[i*4 + 3]);
                    }
                    ImGui::Spacing();
                };

                drawMatrix("m_prevWorldToCameraMatrix", snap->rawViewMatrix, offsets.cameraViewOffset);
                drawMatrix("m_prevProjectionMatrix", snap->rawProjMatrix, offsets.cameraProjOffset);
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "SYS_LOG // 终端底层回执");
            auto& logger = Logger::GetInstance();
            std::lock_guard<std::mutex> logLock(logger.GetMutex());
            
            ImGui::BeginChild("LogArea", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            for (const auto& log : logger.GetLogs()) { 
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f), "> %s", log.c_str()); 
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();
        }
        ImGui::End();
    }
};

float UIRenderer::m_userScale = 1.0f;
ImGuiStyle UIRenderer::m_defaultStyle;
bool UIRenderer::m_showDebugger = false;
bool UIRenderer::m_showRadar = true;
bool UIRenderer::m_showESP = true;     
bool UIRenderer::m_showMenu = true;
bool UIRenderer::m_isHookInstalled = false;
std::string UIRenderer::m_hookStatusMsg = "等待用户指令...";

// ============================================================================
// Dobby Hook 回调区
// ============================================================================
struct ModuleInfo { const char* name; uintptr_t base; };

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

// === 核心数据 Hook ===
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

// === 【新增】相机矩阵抓取 Hook ===
typedef void (*CameraUpdateFunc)(void* x0);
static CameraUpdateFunc g_oldCameraUpdate = nullptr;

void HookCameraUpdate(void* x0) {
    if (x0 != nullptr && reinterpret_cast<uintptr_t>(x0) > 0x10000000) {
        EngineCore::GetInstance().SetCameraMatrixPtr(reinterpret_cast<uintptr_t>(x0));
    }
    if (g_oldCameraUpdate) {
        g_oldCameraUpdate(x0);
    }
}

// === 延时 15 秒后自动挂载相机矩阵 ===
void DelayedCameraHookTask() {
    LOG_INFO("等待 15 秒后执行相机 (0xC4B5CD4) 拦截...");
    sleep(15);
    
    uintptr_t base = GetModuleBase(kGameModule);
    if (!base) return;

    uintptr_t targetAddr = base + 0xC4B5CD4;
    LOG_INFO("开始挂载 CameraData: 0x%lx", targetAddr);

    void* pageStart = reinterpret_cast<void*>(targetAddr & ~0xFFF);
    mprotect(pageStart, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);

    int ret = DobbyHook(reinterpret_cast<void*>(targetAddr), 
                        reinterpret_cast<void*>(HookCameraUpdate), 
                        reinterpret_cast<void**>(&g_oldCameraUpdate));
                        
    if (ret == 0) {
        LOG_INFO("CameraData Hook 挂载成功！(15秒等待完成)");
    } else {
        LOG_ERROR("CameraData Hook 挂载失败！");
    }
}

void InstallHooks() {
    if (UIRenderer::m_isHookInstalled) return;

    system("su -c 'setenforce 0'");

    uintptr_t base = GetModuleBase(kGameModule);
    if (!base) {
        UIRenderer::m_hookStatusMsg = "ERR: 未找到目标模块 libil2cpp.so";
        LOG_ERROR("未找到目标游戏模块！");
        return;
    }
    uintptr_t targetAddr = base + 0x73507bc;
    LOG_INFO("SYS_BASE: 0x%lx, TARGET: 0x%lx", base, targetAddr);

    void* pageStart = reinterpret_cast<void*>(targetAddr & ~0xFFF);
    mprotect(pageStart, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);

    int ret = DobbyHook(reinterpret_cast<void*>(targetAddr), 
                        reinterpret_cast<void*>(HookInitActorParams), 
                        reinterpret_cast<void**>(&g_oldInitActorParams));

    if (ret == 0) {
        UIRenderer::m_isHookInstalled = true;
        UIRenderer::m_hookStatusMsg = "OK: 神经链路挂载完毕。";
        LOG_INFO("核心数据 Hook 成功！");
        
        // ★ 核心 Hook 成功后，启动 15 秒倒计时去勾取相机矩阵
        std::thread(DelayedCameraHookTask).detach();
        
    } else {
        UIRenderer::m_hookStatusMsg = "FATAL: 挂载被拒绝(SELinux限制)";
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
    system("su -c 'setenforce 0'");
    sleep(2);

    void* eglSym = nullptr;
    void* eglHandle = dlopen("libEGL.so", RTLD_LAZY);
    if (eglHandle) {
        eglSym = dlsym(eglHandle, "eglSwapBuffers");
    }
    if (!eglSym) {
        int eglRetries = 0;
        while (!(eglSym = DobbySymbolResolver("libEGL.so", "eglSwapBuffers")) && eglRetries < 5) { 
            sleep(1); eglRetries++;
        }
    }

    if (eglSym) {
        DobbyHook(eglSym, reinterpret_cast<void*>(HookEglSwapBuffers), reinterpret_cast<void**>(&g_oldEglSwapBuffers));
    }

    void* inputSym = nullptr;
    int inputRetries = 0;
    while (!(inputSym = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE")) && inputRetries < 5) { 
        sleep(1); inputRetries++;
    }
    
    if (inputSym) {
        DobbyHook(inputSym, reinterpret_cast<void*>(HookConsume), reinterpret_cast<void**>(&g_oldConsume));
    }

    EngineCore::GetInstance().InitWorkerThread();
    return nullptr;
}

} // namespace Overlay

__attribute__((constructor)) void InitializePlugin() {
    pthread_t threadId;
    pthread_create(&threadId, nullptr, Overlay::BootstrapThread, nullptr);
}
