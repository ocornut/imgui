#include <android/log.h>
#include "imgui.h"

// 定义日志宏，方便在 PC 端通过 adb logcat -s IMG_DEBUG 查看
#define LOG_TAG "IMG_DEBUG"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 模拟全局状态
bool g_ShowDebugWindow = true;

/**
 * 核心功能：处理 Android 原生输入事件并传递给 ImGui
 * 你需要确保你的 AInputQueue 或者 dispatchTouchEvent 正在调用此逻辑
 */
void HandleAndroidInput(int action, float x, float y) {
    ImGuiIO& io = ImGui::GetIO();
    
    // 更新坐标
    io.MousePos = ImVec2(x, y);

    // 更新点击状态
    if (action == 0) { // AMOTION_EVENT_ACTION_DOWN
        io.MouseDown[0] = true;
        LOGI("Touch DOWN at: x=%.1f, y=%.1f", x, y);
    } 
    else if (action == 1) { // AMOTION_EVENT_ACTION_UP
        io.MouseDown[0] = false;
        LOGI("Touch UP at: x=%.1f, y=%.1f", x, y);
    }
    else if (action == 2) { // AMOTION_EVENT_ACTION_MOVE
        // Move 事件通常不需要设置 MouseDown，只需更新坐标
    }
}

/**
 * 核心功能：每帧调用的 UI 渲染函数
 * 请将此函数放入你的渲染循环中
 */
void RenderImGuiFrame() {
    ImGuiIO& io = ImGui::GetIO();

    // 1. 设置窗口样式 (可选，为了更好看)
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    
    // 2. 开始渲染调试窗口
    // 注意：这里使用纯英文，避免截图中的 ?? 乱码问题
    if (ImGui::Begin("INPUT DEBUGGER v1.0", &g_ShowDebugWindow)) {
        
        ImGui::Text("Device: OnePlus Pad (Testing)");
        ImGui::Separator();

        // --- 坐标实时显示 ---
        // 如果你手指在屏幕上移动，这两个数字必须跳动
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Real-time Mouse Position:");
        ImGui::BulletText("X: %.1f", io.MousePos.x);
        ImGui::BulletText("Y: %.1f", io.MousePos.y);

        ImGui::Spacing();

        // --- 点击状态显示 ---
        // 按下时显示红色 "PRESSED"，松开时显示灰色 "IDLE"
        ImGui::Text("Touch Status: ");
        ImGui::SameLine();
        if (io.MouseDown[0]) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "PRESSED");
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "IDLE");
        }

        ImGui::Separator();

        // --- 功能测试按钮 ---
        // 这是一个物理检测：如果坐标对，但点不动这个按钮，说明 io.MouseDown 没传对
        if (ImGui::Button("TEST CLICK FEEDBACK", ImVec2(-1, 80))) {
            LOGI("Button Clicked via ImGui!");
        }

        if (ImGui::Button("RESET COORDINATES", ImVec2(-1, 40))) {
            io.MousePos = ImVec2(-1, -1);
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Check Logcat with: adb logcat -s %s", LOG_TAG);
    }
    ImGui::End();
}

/**
 * 简易说明：
 * 1. 如果窗口里的 X/Y 始终是 0 或者固定值，说明 HandleAndroidInput 没被触发。
 * 2. 如果 X/Y 在动，但按钮点不下去，说明 io.MouseDown[0] 的赋值逻辑有问题。
 * 3. 请确保 RenderImGuiFrame 在你的绘图指令（DrawData）之前执行。
 */
