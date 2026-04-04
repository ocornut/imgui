#include <jni.h>
#include <dlfcn.h>
#include <android/input.h>
#include "imgui.h"

// --- 诊断变量 ---
static bool g_InputHooked = false;
static char g_DiagnosticMsg[256] = "尚未检测...";

// 1. 检查你的注入器是否 Hook 了安卓输入队列
// 如果这个函数返回 false，说明你的菜单只能看，不能点
void CheckInputHookStatus() {
    void* handle = dlopen("libandroid.so", RTLD_LAZY);
    if (handle) {
        // 尝试获取 AInputQueue_getEvent 的地址
        void* addr = dlsym(handle, "AInputQueue_getEvent");
        
        // 检查这个地址的前几个字节是否被修改（常见的 Hook 特征）
        unsigned char* ptr = (unsigned char*)addr;
        if (ptr[0] == 0x50 && ptr[1] == 0x00) { // 示例特征，视具体 Hook 库而定
            g_InputHooked = true;
            snprintf(g_DiagnosticMsg, 256, "状态: 已 Hook (输入应正常)");
        } else {
            g_InputHooked = false;
            snprintf(g_DiagnosticMsg, 256, "错误: AInputQueue 未被拦截！菜单无法操作。");
        }
        dlclose(handle);
    }
}

// 2. 强力修复方案：如果你的 Hook 库没做输入拦截，手动实现一个
// 在你的 eglSwapBuffers 中调用此 UI 检查
void DrawInputDebugger() {
    ImGui::Begin("输入诊断面板");
    
    ImGui::Text("%s", g_DiagnosticMsg);
    ImGui::Separator();
    
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("当前手指坐标: (%.1f, %.1f)", io.MousePos.x, io.MousePos.y);
    ImGui::Text("是否按下: %s", io.MouseDown[0] ? "是" : "否");
    
    if (!g_InputHooked) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
        ImGui::TextWrapped("警告：点击无效是因为你目前的注入代码中缺少了 Input Hook。");
        ImGui::TextWrapped("你需要检查你的 .cpp 文件里是否有 Hook 如下函数：\n1. AInputQueue_getEvent\n2. AInputQueue_finishEvent");
        ImGui::PopStyleColor();
    }
    
    // 手动测试按钮：如果这个按钮能变色，说明输入是通的
    if (ImGui::Button("测试点击响应", ImVec2(-1, 80))) {
        snprintf(g_DiagnosticMsg, 256, "点击成功！输入链路正常。");
    }
    
    ImGui::End();
}

// 3. 如何解决“拖不动”的终极代码块
// 如果你发现诊断面板显示未 Hook，请在你的代码里加上这一段：
/*
typedef int (*t_AInputQueue_getEvent)(AInputQueue*, AInputEvent**);
t_AInputQueue_getEvent orig_getEvent;

int hooked_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** out_event) {
    int res = orig_getEvent(queue, out_event);
    if (res >= 0 && *out_event != nullptr) {
        // 在这里把坐标传给 ImGui
        ImGuiIO& io = ImGui::GetIO();
        int32_t type = AInputEvent_getType(*out_event);
        if (type == AINPUT_EVENT_TYPE_MOTION) {
            float x = AMotionEvent_getX(*out_event, 0);
            float y = AMotionEvent_getY(*out_event, 0);
            io.MousePos = ImVec2(x, y);
            
            int32_t action = AMotionEvent_getAction(*out_event) & AMOTION_EVENT_ACTION_MASK;
            if (action == AMOTION_EVENT_ACTION_DOWN) io.MouseDown[0] = true;
            if (action == AMOTION_EVENT_ACTION_UP) io.MouseDown[0] = false;
            
            // 如果菜单显示中，拦截掉这个点击，不让游戏响应
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
                // return 1; // 这一行根据你的 Hook 框架决定是否启用
            }
        }
    }
    return res;
}
*/
