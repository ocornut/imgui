#pragma once

#include "imgui.h"

namespace ImScoped
{
    struct Window
    {
        bool IsContentVisible;

        Window(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0) { IsContentVisible = ImGui::Begin(name, p_open, flags); }
        ~Window() { ImGui::End(); }

        operator bool() { return IsContentVisible; }

        Window(Window &&) = delete;
        Window &operator=(Window &&) = delete;
        Window(const Window &) = delete;
        Window &operator=(Window &) = delete;
    };

    struct PushID
    {
        PushID(const char* str_id) { ImGui::PushID(str_id); }
        PushID(const char* str_id_begin, const char* str_id_end) { ImGui::PushID(str_id_begin, str_id_end); }
        PushID(const void* ptr_id) { ImGui::PushID(ptr_id); }
        PushID(int int_id) { ImGui::PushID(int_id); }
        ~PushID() { ImGui::PopID(); }

        PushID(PushID &&) = delete;
        PushID &operator=(PushID &&) = delete;
        PushID(const PushID &) = delete;
        PushID &operator=(PushID &) = delete;
    };

    struct TreeNode
    {
        bool IsOpen;

        TreeNode(const char* label) { IsOpen = ImGui::TreeNode(label); }
        TreeNode(const char* str_id, const char* fmt, ...) IM_FMTARGS(3) { va_list ap; va_start(ap, fmt); IsOpen = ImGui::TreeNodeV(str_id, fmt, ap); va_end(ap); }
        TreeNode(const void* ptr_id, const char* fmt, ...) IM_FMTARGS(3) { va_list ap; va_start(ap, fmt); IsOpen = ImGui::TreeNodeV(ptr_id, fmt, ap); va_end(ap); }
        ~TreeNode() { if (IsOpen) ImGui::TreePop(); }

        operator bool() { return IsOpen; }

        TreeNode(TreeNode &&) = delete;
        TreeNode &operator=(TreeNode &&) = delete;
        TreeNode(const TreeNode &) = delete;
        TreeNode &operator=(TreeNode &) = delete;
    };

    struct TreeNodeV
    {
        bool IsOpen;

        TreeNodeV(const char* str_id, const char* fmt, va_list args) IM_FMTLIST(3) { IsOpen = ImGui::TreeNodeV(str_id, fmt, args); }
        TreeNodeV(const void* ptr_id, const char* fmt, va_list args) IM_FMTLIST(3) { IsOpen = ImGui::TreeNodeV(ptr_id, fmt, args); }
        ~TreeNodeV() { if (IsOpen) ImGui::TreePop(); }

        operator bool() { return IsOpen; }

        TreeNodeV(TreeNodeV &&) = delete;
        TreeNodeV &operator=(TreeNodeV &&) = delete;
        TreeNodeV(const TreeNodeV &) = delete;
        TreeNodeV &operator=(TreeNodeV &) = delete;
    };

    struct TreeNodeEx
    {
        bool IsOpen;

        TreeNodeEx(const char* label, ImGuiTreeNodeFlags flags = 0) { IsOpen = ImGui::TreeNodeEx(label, flags); }
        TreeNodeEx(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) IM_FMTARGS(4) { va_list ap; va_start(ap, fmt); IsOpen = ImGui::TreeNodeExV(str_id, flags, fmt, ap); va_end(ap); }
        TreeNodeEx(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) IM_FMTARGS(4) { va_list ap; va_start(ap, fmt); IsOpen = ImGui::TreeNodeExV(ptr_id, flags, fmt, ap); va_end(ap); }
        ~TreeNodeEx() { if (IsOpen) ImGui::TreePop(); }

        operator bool() { return IsOpen; }

        TreeNodeEx(TreeNodeEx &&) = delete;
        TreeNodeEx &operator=(TreeNodeEx &&) = delete;
        TreeNodeEx(const TreeNodeEx &) = delete;
        TreeNodeEx &operator=(TreeNodeEx &) = delete;
    };

    struct TreeNodeExV
    {
        bool IsOpen;

        TreeNodeExV(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args) IM_FMTLIST(4) { IsOpen = ImGui::TreeNodeExV(str_id, flags, fmt, args); }
        TreeNodeExV(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args) IM_FMTLIST(4) { IsOpen = ImGui::TreeNodeExV(ptr_id, flags, fmt, args); }
        ~TreeNodeExV() { if (IsOpen) ImGui::TreePop(); }

        operator bool() { return IsOpen; }

        TreeNodeExV(TreeNodeExV &&) = delete;
        TreeNodeExV &operator=(TreeNodeExV &&) = delete;
        TreeNodeExV(const TreeNodeExV &) = delete;
        TreeNodeExV &operator=(TreeNodeExV &) = delete;
    };

} // namespace ImScoped
