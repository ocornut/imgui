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

    struct Child
    {
        bool IsContentVisible;

        Child(const char* str_id, const ImVec2& size = ImVec2(0,0), bool border = false, ImGuiWindowFlags flags = 0) { IsContentVisible = ImGui::BeginChild(str_id, size, 0); }
        Child(ImGuiID id, const ImVec2& size = ImVec2(0,0), bool border = false, ImGuiWindowFlags flags = 0) { IsContentVisible = ImGui::BeginChild(id, size, 0); }
        ~Child() { ImGui::EndChild(); }

        operator bool() { return IsContentVisible; }

        Child(Child &&) = delete;
        Child &operator=(Child &&) = delete;
        Child(const Child &) = delete;
        Child &operator=(Child &) = delete;
    };

    struct Font
    {
        Font(ImFont* font) { ImGui::PushFont(font); }
        ~Font() { ImGui::PopFont(); }

        Font(Font &&) = delete;
        Font &operator=(Font &&) = delete;
        Font(const Font &) = delete;
        Font &operator=(Font &) = delete;
    };

    struct StyleColor
    {
        StyleColor(ImGuiCol idx, ImU32 col) { ImGui::PushStyleColor(idx, col); }
        StyleColor(ImGuiCol idx, const ImVec4& col) { ImGui::PushStyleColor(idx, col); }
        ~StyleColor() { ImGui::PopStyleColor(); }

        StyleColor(StyleColor &&) = delete;
        StyleColor &operator=(StyleColor &&) = delete;
        StyleColor(const StyleColor &) = delete;
        StyleColor &operator=(StyleColor &) = delete;
    };

    struct StyleVar
    {
        StyleVar(ImGuiStyleVar idx, float val) { ImGui::PushStyleVar(idx, val); }
        StyleVar(ImGuiStyleVar idx, const ImVec2& val) { ImGui::PushStyleVar(idx, val); }
        ~StyleVar() { ImGui::PopStyleVar(); }

        StyleVar(StyleVar &&) = delete;
        StyleVar &operator=(StyleVar &&) = delete;
        StyleVar(const StyleVar &) = delete;
        StyleVar &operator=(StyleVar &) = delete;
    };

    struct ItemWidth
    {
        ItemWidth(float item_width) { ImGui::PushItemWidth(item_width); }
        ~ItemWidth() { ImGui::PopItemWidth(); }

        ItemWidth(ItemWidth &&) = delete;
        ItemWidth &operator=(ItemWidth &&) = delete;
        ItemWidth(const ItemWidth &) = delete;
        ItemWidth &operator=(ItemWidth &) = delete;
    };

    struct TextWrapPos
    {
        TextWrapPos(float wrap_pos_x = 0.0f) { ImGui::PushTextWrapPos(wrap_pos_x); }
        ~TextWrapPos() { ImGui::PopTextWrapPos(); }

        TextWrapPos(TextWrapPos &&) = delete;
        TextWrapPos &operator=(TextWrapPos &&) = delete;
        TextWrapPos(const TextWrapPos &) = delete;
        TextWrapPos &operator=(TextWrapPos &) = delete;
    };

    struct AllowKeyboardFocus
    {
        AllowKeyboardFocus(bool allow_keyboard_focus) { ImGui::PushAllowKeyboardFocus(allow_keyboard_focus); }
        ~AllowKeyboardFocus() { ImGui::PopAllowKeyboardFocus(); }

        AllowKeyboardFocus(AllowKeyboardFocus &&) = delete;
        AllowKeyboardFocus &operator=(AllowKeyboardFocus &&) = delete;
        AllowKeyboardFocus(const AllowKeyboardFocus &) = delete;
        AllowKeyboardFocus &operator=(AllowKeyboardFocus &) = delete;
    };

    struct ButtonRepeat
    {
        ButtonRepeat(bool repeat) { ImGui::PushButtonRepeat(repeat); }
        ~ButtonRepeat() { ImGui::PopButtonRepeat(); }

        ButtonRepeat(ButtonRepeat &&) = delete;
        ButtonRepeat &operator=(ButtonRepeat &&) = delete;
        ButtonRepeat(const ButtonRepeat &) = delete;
        ButtonRepeat &operator=(ButtonRepeat &) = delete;
    };

    struct ID
    {
        ID(const char* str_id) { ImGui::PushID(str_id); }
        ID(const char* str_id_begin, const char* str_id_end) { ImGui::PushID(str_id_begin, str_id_end); }
        ID(const void* ptr_id) { ImGui::PushID(ptr_id); }
        ID(int int_id) { ImGui::PushID(int_id); }
        ~ID() { ImGui::PopID(); }

        ID(ID &&) = delete;
        ID &operator=(ID &&) = delete;
        ID(const ID &) = delete;
        ID &operator=(ID &) = delete;
    };

    struct Combo
    {
        bool IsOpen;

        Combo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0) { IsOpen = ImGui::BeginCombo(label, preview_value, flags); }
        ~Combo() { if (IsOpen) ImGui::EndCombo(); }

        operator bool() { return IsOpen; }

        Combo(Combo &&) = delete;
        Combo &operator=(Combo &&) = delete;
        Combo(const Combo &) = delete;
        Combo &operator=(Combo &) = delete;
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

    struct TreePush
    {
        TreePush(const char* str_id) { ImGui::TreePush(str_id); }
        TreePush(const void* ptr_id = NULL) { ImGui::TreePush(ptr_id); }
        ~TreePush() { ImGui::TreePop(); }

        TreePush(TreePush &&) = delete;
        TreePush &operator=(TreePush &&) = delete;
        TreePush(const TreePush &) = delete;
        TreePush &operator=(TreePush &) = delete;
    };

    struct Menu
    {
        bool IsOpen;

        Menu(const char* label, bool enabled = true) { IsOpen = ImGui::BeginMenu(label, enabled); }
        ~Menu() { if (IsOpen) ImGui::EndMenu(); }

        operator bool() { return IsOpen; }

        Menu(Menu &&) = delete;
        Menu &operator=(Menu &&) = delete;
        Menu(const Menu &) = delete;
        Menu &operator=(Menu &) = delete;
    };

    struct Popup
    {
        bool IsOpen;

        Popup(const char* str_id, ImGuiWindowFlags flags = 0) { IsOpen = ImGui::BeginPopup(str_id, flags); }
        ~Popup() { if (IsOpen) ImGui::EndPopup(); }

        operator bool() { return IsOpen; }

        Popup(Popup &&) = delete;
        Popup &operator=(Popup &&) = delete;
        Popup(const Popup &) = delete;
        Popup &operator=(Popup &) = delete;
    };

    struct PopupContextItem
    {
        bool IsOpen;

        PopupContextItem(const char* str_id = NULL, int mouse_button = 1) { IsOpen = ImGui::BeginPopupContextItem(str_id, mouse_button); }
        ~PopupContextItem() { if (IsOpen) ImGui::EndPopup(); }

        operator bool() { return IsOpen; }

        PopupContextItem(PopupContextItem &&) = delete;
        PopupContextItem &operator=(PopupContextItem &&) = delete;
        PopupContextItem(const PopupContextItem &) = delete;
        PopupContextItem &operator=(PopupContextItem &) = delete;
    };

    struct PopupContextWindow
    {
        bool IsOpen;

        PopupContextWindow(const char* str_id = NULL, int mouse_button = 1, bool also_over_items = true) { IsOpen = ImGui::BeginPopupContextWindow(str_id, mouse_button, also_over_items); }
        ~PopupContextWindow() { if (IsOpen) ImGui::EndPopup(); }

        operator bool() { return IsOpen; }

        PopupContextWindow(PopupContextWindow &&) = delete;
        PopupContextWindow &operator=(PopupContextWindow &&) = delete;
        PopupContextWindow(const PopupContextWindow &) = delete;
        PopupContextWindow &operator=(PopupContextWindow &) = delete;
    };

    struct PopupContextVoid
    {
        bool IsOpen;

        PopupContextVoid(const char* str_id = NULL, int mouse_button = 1) { IsOpen = ImGui::BeginPopupContextVoid(str_id, mouse_button); }
        ~PopupContextVoid() { if (IsOpen) ImGui::EndPopup(); }

        operator bool() { return IsOpen; }

        PopupContextVoid(PopupContextVoid &&) = delete;
        PopupContextVoid &operator=(PopupContextVoid &&) = delete;
        PopupContextVoid(const PopupContextVoid &) = delete;
        PopupContextVoid &operator=(PopupContextVoid &) = delete;
    };

    struct PopupModal
    {
        bool IsOpen;

        PopupModal(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0) { IsOpen = ImGui::BeginPopupModal(name, p_open, flags); }
        ~PopupModal() { if (IsOpen) ImGui::EndPopup(); }

        operator bool() { return IsOpen; }

        PopupModal(PopupModal &&) = delete;
        PopupModal &operator=(PopupModal &&) = delete;
        PopupModal(const PopupModal &) = delete;
        PopupModal &operator=(PopupModal &) = delete;
    };

    struct DragDropSource
    {
        bool IsOpen;

        DragDropSource(ImGuiDragDropFlags flags = 0) { IsOpen = ImGui::BeginDragDropSource(flags); }
        ~DragDropSource() { if (IsOpen) ImGui::EndDragDropSource(); }

        operator bool() { return IsOpen; }

        DragDropSource(DragDropSource &&) = delete;
        DragDropSource &operator=(DragDropSource &&) = delete;
        DragDropSource(const DragDropSource &) = delete;
        DragDropSource &operator=(DragDropSource &) = delete;
    };

    struct ClipRect
    {
        ClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect) { ImGui::PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect); }
        ~ClipRect() { ImGui::PopClipRect(); }

        ClipRect(ClipRect &&) = delete;
        ClipRect &operator=(ClipRect &&) = delete;
        ClipRect(const ClipRect &) = delete;
        ClipRect &operator=(ClipRect &) = delete;
    };

    struct ChildFrame
    {
        bool IsOpen;

        ChildFrame(ImGuiID id, const ImVec2& size, ImGuiWindowFlags flags = 0) { IsOpen = ImGui::BeginChildFrame(id, size, flags); }
        ~ChildFrame() { ImGui::EndChildFrame(); }

        operator bool() { return IsOpen; }

        ChildFrame(ChildFrame &&) = delete;
        ChildFrame &operator=(ChildFrame &&) = delete;
        ChildFrame(const ChildFrame &) = delete;
        ChildFrame &operator=(ChildFrame &) = delete;
    };

} // namespace ImScoped
