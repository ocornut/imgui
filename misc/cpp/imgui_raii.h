#include "imgui.h"

#pragma once

struct ImWindow
{
    bool IsExpanded;

    ImWindow(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0) { IsExpanded = ImGui::Begin(name, p_open, flags); }
    ~ImWindow() { ImGui::End(); }

    operator bool() { return IsExpanded; }
};

struct ImPushID
{
    ImPushID(const char* str_id) { ImGui::PushID(str_id); }
    ImPushID(const char* str_id_begin, const char* str_id_end) { ImGui::PushID(str_id_begin, str_id_end); }
    ImPushID(const void* ptr_id) { ImGui::PushID(ptr_id); }
    ImPushID(int int_id) { ImGui::PushID(int_id); }
    ~ImPushID() { ImGui::PopID(); }
};

struct ImTreeNode
{
    bool IsOpen;

    ImTreeNode(const char* label) { IsOpen = ImGui::TreeNode(label); }
    ImTreeNode(const char* str_id, const char* fmt, ...) IM_FMTARGS(3) { va_list ap; va_start(ap, fmt); IsOpen = ImGui::TreeNodeV(str_id, fmt, ap); va_end(ap); }
    ImTreeNode(const void* ptr_id, const char* fmt, ...) IM_FMTARGS(3) { va_list ap; va_start(ap, fmt); IsOpen = ImGui::TreeNodeV(ptr_id, fmt, ap); va_end(ap); }
    ~ImTreeNode() { if (IsOpen) ImGui::TreePop(); }

    operator bool() { return IsOpen; }
};

struct ImTreeNodeV
{
    bool IsOpen;

    ImTreeNodeV(const char* str_id, const char* fmt, va_list args) IM_FMTLIST(3) { IsOpen = ImGui::TreeNodeV(str_id, fmt, args); }
    ImTreeNodeV(const void* ptr_id, const char* fmt, va_list args) IM_FMTLIST(3) { IsOpen = ImGui::TreeNodeV(ptr_id, fmt, args); }
    ~ImTreeNodeV() { if (IsOpen) ImGui::TreePop(); }

    operator bool() { return IsOpen; }
};

struct ImTreeNodeEx
{
    bool IsOpen;

    ImTreeNodeEx(const char* label, ImGuiTreeNodeFlags flags = 0) { IsOpen = ImGui::TreeNodeEx(label, flags); }
    ImTreeNodeEx(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) IM_FMTARGS(4) { va_list ap; va_start(ap, fmt); IsOpen = ImGui::TreeNodeExV(str_id, flags, fmt, ap); va_end(ap); }
    ImTreeNodeEx(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) IM_FMTARGS(4) { va_list ap; va_start(ap, fmt); IsOpen = ImGui::TreeNodeExV(ptr_id, flags, fmt, ap); va_end(ap); }
    ~ImTreeNodeEx() { if (IsOpen) ImGui::TreePop(); }

    operator bool() { return IsOpen; }
};

struct ImTreeNodeExV
{
    bool IsOpen;

    ImTreeNodeExV(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args) IM_FMTLIST(4) { IsOpen = ImGui::TreeNodeExV(str_id, flags, fmt, args); }
    ImTreeNodeExV(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args) IM_FMTLIST(4) { IsOpen = ImGui::TreeNodeExV(ptr_id, flags, fmt, args); }
    ~ImTreeNodeExV() { if (IsOpen) ImGui::TreePop(); }

    operator bool() { return IsOpen; }
};
