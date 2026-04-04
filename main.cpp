#include <jni.h>
#include <dlfcn.h>
#include <android/input.h>
#include <cstdio>      // 修复 snprintf 的关键头文件
#include <cstring>     // 备用，处理字符串常用
#include "imgui.h"

// --- 诊断变量 ---
static bool g_InputHooked = false;
static char g_DiagnosticMsg[256] = "尚未检测...";

// 检查注入器是否 Hook 了安卓输入队列
void CheckInputHookStatus() {
    // 尝试打开系统库
    void* handle = dlopen("libandroid.so", RTLD_LAZY);
    if (handle) {
        // 获取输入事件获取函数的地址
        void* addr = dlsym(handle, "AInputQueue_getEvent");
        if (addr) {
            unsigned char* ptr = (unsigned char*)addr;
            
            // 常见的 Hook 检测逻辑：检查函数开头是否被修改为跳转指令 (0x50 0x00 这种只是示例)
            // 实际上大多数移动端 Hook 框架会修改前 4 或 8 个字节
            if (ptr[0] != 0x50) { // 如果字节码发生了变化
                 g_InputHooked = true;
                 snprintf(g_DiagnosticMsg, 256, "状态: 检测到 Hook (输入可能已重定向)");
            } else {
                 g_InputHooked = false;
                 snprintf(g_DiagnosticMsg, 256, "状态: 原生函数 (未发现 Hook 迹象)");
            }
        }
        dlclose(handle);
    } else {
        snprintf(g_DiagnosticMsg, 256, "错误: 无法加载 libandroid.so");
    }
}

// 在 ImGui 渲染循环中调用此函数
void DrawInputDebugger() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("输入注入诊断面板", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    // 状态显示
    if (g_InputHooked) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", g_DiagnosticMsg);
    } else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", g_DiagnosticMsg);
    }
    
    ImGui::Separator();
    
    // 实时数据流监控
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("ImGui 坐标: (%.1f, %.1f)", io.MousePos.x, io.MousePos.y);
    ImGui::Text("左键按下状态: %s", io.MouseDown[0] ? "TRUE" : "FALSE");
    
    ImGui::Spacing();
    
    if (ImGui::Button("点击测试按钮", ImVec2(-1, 60))) {
        snprintf(g_DiagnosticMsg, 256, "恭喜：点击事件已成功传达到 ImGui！");
    }
    
    if (!g_InputHooked) {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.5f, 0, 1));
        ImGui::TextWrapped("提示：如果点击没反应，请确认你的 .cpp 项目中存在对 AInputQueue_getEvent 的 Hook。没有这个 Hook，系统点击就不会发给菜单。");
        ImGui::PopStyleColor();
    }
    
    ImGui::End();
}
