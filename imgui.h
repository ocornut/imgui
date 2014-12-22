// ImGui library v1.19 wip
// See .cpp file for commentary.
// See ImGui::ShowTestWindow() for sample code.
// Read 'Programmer guide' in .cpp for notes on how to setup ImGui in your codebase.
// Get latest version at https://github.com/ocornut/imgui

#pragma once

struct ImDrawList;
struct ImFont;
struct ImGuiAabb;
struct ImGuiIO;
struct ImGuiStorage;
struct ImGuiStyle;
struct ImGuiWindow;

#include "imconfig.h"
#include <float.h>          // FLT_MAX
#include <stdarg.h>         // va_list
#include <stddef.h>         // ptrdiff_t
#include <stdlib.h>         // NULL, malloc

#ifndef IM_ASSERT
#include <assert.h>
#define IM_ASSERT(_EXPR)    assert(_EXPR)
#endif

#ifndef IMGUI_API
#define IMGUI_API
#endif

typedef unsigned int ImU32;
typedef unsigned short ImWchar;     // hold a character for display
typedef ImU32 ImGuiID;              // hold widget unique ID
typedef int ImGuiCol;               // enum ImGuiCol_
typedef int ImGuiStyleVar;          // enum ImGuiStyleVar_
typedef int ImGuiKey;               // enum ImGuiKey_
typedef int ImGuiColorEditMode;     // enum ImGuiColorEditMode_
typedef int ImGuiWindowFlags;       // enum ImGuiWindowFlags_
typedef int ImGuiSetCondition;      // enum ImGuiSetCondition_
typedef int ImGuiInputTextFlags;    // enum ImGuiInputTextFlags_
struct ImGuiTextEditCallbackData;

struct ImVec2
{
    float x, y;
    ImVec2() {}
    ImVec2(float _x, float _y) { x = _x; y = _y; }

#ifdef IM_VEC2_CLASS_EXTRA
    IM_VEC2_CLASS_EXTRA
#endif
};

struct ImVec4
{
    float x, y, z, w;
    ImVec4() {}
    ImVec4(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }

#ifdef IM_VEC4_CLASS_EXTRA
    IM_VEC4_CLASS_EXTRA
#endif
};

namespace ImGui
{
    // Proxy functions to access the MemAllocFn/MemFreeFn/MemReallocFn pointers in ImGui::GetIO(). The only reason they exist here is to allow ImVector<> to compile inline.
    IMGUI_API void*       MemAlloc(size_t sz);
    IMGUI_API void        MemFree(void* ptr);
    IMGUI_API void*       MemRealloc(void* ptr, size_t sz);
}

// std::vector<> like class to avoid dragging dependencies (also: windows implementation of STL with debug enabled is absurdly slow, so let's bypass it so our code runs fast in debug). 
// Use '#define ImVector std::vector' if you want to use the STL type or your own type.
// Our implementation does NOT call c++ constructors! because the data types we use don't need them (but that could be added as well). Only provide the minimum functionalities we need.
#ifndef ImVector
template<typename T>
class ImVector
{
protected:
    size_t                      Size;
    size_t                      Capacity;
    T*                          Data;

public:
    typedef T                   value_type;
    typedef value_type*         iterator;
    typedef const value_type*   const_iterator;

    ImVector()                  { Size = Capacity = 0; Data = NULL; }
    ~ImVector()                 { if (Data) ImGui::MemFree(Data); }

    inline bool                 empty() const                   { return Size == 0; }
    inline size_t               size() const                    { return Size; }
    inline size_t               capacity() const                { return Capacity; }

    inline value_type&          at(size_t i)                    { IM_ASSERT(i < Size); return Data[i]; }
    inline const value_type&    at(size_t i) const              { IM_ASSERT(i < Size); return Data[i]; }
    inline value_type&          operator[](size_t i)            { IM_ASSERT(i < Size); return Data[i]; }
    inline const value_type&    operator[](size_t i) const      { IM_ASSERT(i < Size); return Data[i]; }

    inline void                 clear()                         { if (Data) { Size = Capacity = 0; ImGui::MemFree(Data); Data = NULL; } }
    inline iterator             begin()                         { return Data; }
    inline const_iterator       begin() const                   { return Data; }
    inline iterator             end()                           { return Data + Size; }
    inline const_iterator       end() const                     { return Data + Size; }
    inline value_type&          front()                         { return at(0); }
    inline const value_type&    front() const                   { return at(0); }
    inline value_type&          back()                          { IM_ASSERT(Size > 0); return at(Size-1); }
    inline const value_type&    back() const                    { IM_ASSERT(Size > 0); return at(Size-1); }
    inline void                 swap(ImVector<T>& rhs)          { const size_t rhs_size = rhs.Size; rhs.Size = Size; Size = rhs_size; const size_t rhs_cap = rhs.Capacity; rhs.Capacity = Capacity; Capacity = rhs_cap; value_type* rhs_data = rhs.Data; rhs.Data = Data; Data = rhs_data; }

    inline void                 reserve(size_t new_capacity)    { Data = (value_type*)ImGui::MemRealloc(Data, new_capacity * sizeof(value_type)); Capacity = new_capacity; }
    inline void                 resize(size_t new_size)         { if (new_size > Capacity) reserve(new_size); Size = new_size; }

    inline void                 push_back(const value_type& v)  { if (Size == Capacity) reserve(Capacity ? Capacity * 2 : 4); Data[Size++] = v; }
    inline void                 pop_back()                      { IM_ASSERT(Size > 0); Size--; }

    inline iterator             erase(const_iterator it)        { IM_ASSERT(it >= begin() && it < end()); const ptrdiff_t off = it - begin(); memmove(Data + off, Data + off + 1, (Size - (size_t)off - 1) * sizeof(value_type)); Size--; return Data + off; }
    inline iterator             insert(const_iterator it, const value_type& v)  { IM_ASSERT(it >= begin() && it <= end()); const ptrdiff_t off = it - begin(); if (Size == Capacity) reserve(Capacity ? Capacity * 2 : 4); if (off < (int)Size) memmove(Data + off + 1, Data + off, (Size - (size_t)off) * sizeof(value_type)); Data[off] = v; Size++; return Data + off; }
};
#endif // #ifndef ImVector

// Helpers at bottom of the file:
// - IMGUI_ONCE_UPON_A_FRAME            // Execute a block of code once per frame only (convenient for creating UI within deep-nested code that runs multiple times)
// - struct ImGuiTextFilter             // Parse and apply text filters. In format "aaaaa[,bbbb][,ccccc]"
// - struct ImGuiTextBuffer             // Text buffer for logging/accumulating text
// - struct ImGuiStorage                // Custom key value storage (if you need to alter open/close states manually)
// - struct ImDrawList                  // Draw command list
// - struct ImFont                      // Bitmap font loader

// ImGui End-user API
// In a namespace so that user can add extra functions (e.g. Value() helpers for your vector or common types)
namespace ImGui
{
    // Main
    IMGUI_API ImGuiIO&      GetIO();
    IMGUI_API ImGuiStyle&   GetStyle();
    IMGUI_API void          NewFrame();
    IMGUI_API void          Render();
    IMGUI_API void          Shutdown();
    IMGUI_API void          ShowUserGuide();
    IMGUI_API void          ShowStyleEditor(ImGuiStyle* ref = NULL);
    IMGUI_API void          ShowTestWindow(bool* open = NULL);

    // Window
    IMGUI_API bool          Begin(const char* name = "Debug", bool* p_opened = NULL, ImVec2 size = ImVec2(0,0), float fill_alpha = -1.0f, ImGuiWindowFlags flags = 0);// return false when window is collapsed, so you can early out in your code. passing 'bool* p_opened' displays a Close button on the upper-right corner of the window, the pointed value will be set to false when the button is pressed.
    IMGUI_API void          End();
    IMGUI_API void          BeginChild(const char* str_id, ImVec2 size = ImVec2(0,0), bool border = false, ImGuiWindowFlags extra_flags = 0);                         // size==0.0f: use remaining window size, size<0.0f: use remaining window size minus abs(size). on each axis.
    IMGUI_API void          EndChild();
    IMGUI_API bool          GetWindowIsFocused();
    IMGUI_API ImVec2        GetContentRegionMax();                                              // window or current column boundaries
    IMGUI_API ImVec2        GetWindowContentRegionMin();                                        // window boundaries
    IMGUI_API ImVec2        GetWindowContentRegionMax();
    IMGUI_API ImDrawList*   GetWindowDrawList();                                                // get rendering command-list if you want to append your own draw primitives.
    IMGUI_API ImFont*       GetWindowFont();
    IMGUI_API float         GetWindowFontSize();
    IMGUI_API void          SetWindowFontScale(float scale);                                    // per-window font scale. Adjust IO.FontGlobalScale if you want to scale all windows.
    IMGUI_API ImVec2        GetWindowPos();                                                     // you should rarely need/care about the window position, but it can be useful if you want to do your own drawing.
    IMGUI_API ImVec2        GetWindowSize();                                                    // get current window position.
    IMGUI_API float         GetWindowWidth();
    IMGUI_API bool          GetWindowCollapsed();
    IMGUI_API void          SetWindowPos(const ImVec2& pos, ImGuiSetCondition cond = 0);        // set current window position.
    IMGUI_API void          SetWindowSize(const ImVec2& size, ImGuiSetCondition cond = 0);      // set current window size. set to ImVec2(0,0) to force an auto-fit
    IMGUI_API void          SetWindowCollapsed(bool collapsed, ImGuiSetCondition cond = 0);     // set current window collapsed state.
    IMGUI_API void          SetNextWindowPos(const ImVec2& pos, ImGuiSetCondition cond = 0);    // set next window position.
    IMGUI_API void          SetNextWindowSize(const ImVec2& size, ImGuiSetCondition cond = 0);  // set next window size. set to ImVec2(0,0) to force an auto-fit
    IMGUI_API void          SetNextWindowCollapsed(bool collapsed, ImGuiSetCondition cond = 0); // set next window collapsed state.

    IMGUI_API void          SetScrollPosHere();                                                 // adjust scrolling position to center into the current cursor position.
    IMGUI_API void          SetKeyboardFocusHere(int offset = 0);                               // focus keyboard on the next widget. Use positive 'offset' to access sub components of a multiple component widget.
    IMGUI_API void          SetTreeStateStorage(ImGuiStorage* tree);                            // replace tree state storage with our own (if you want to manipulate it yourself, typically clear subsection of it).
    IMGUI_API ImGuiStorage* GetTreeStateStorage();

    IMGUI_API void          PushItemWidth(float item_width);                                    // width of items for the common item+label case. default to ~2/3 of windows width.
    IMGUI_API void          PopItemWidth();
    IMGUI_API float         GetItemWidth();
    IMGUI_API void          PushAllowKeyboardFocus(bool v);                                     // allow focusing using TAB/Shift-TAB, enabled by default but you can disable it for certain widgets.
    IMGUI_API void          PopAllowKeyboardFocus();
    IMGUI_API void          PushStyleColor(ImGuiCol idx, const ImVec4& col);
    IMGUI_API void          PopStyleColor(int count = 1);
    IMGUI_API void          PushStyleVar(ImGuiStyleVar idx, float val);
    IMGUI_API void          PushStyleVar(ImGuiStyleVar idx, const ImVec2& val);
    IMGUI_API void          PopStyleVar(int count = 1);
    IMGUI_API void          PushTextWrapPos(float wrap_pos_x = 0.0f);                           // word-wrapping for Text*() commands. < 0.0f: no wrapping; 0.0f: wrap to end of window (or column); > 0.0f: wrap at 'wrap_pos_x' position in window local space.
    IMGUI_API void          PopTextWrapPos();

    // Tooltip
    IMGUI_API void          SetTooltip(const char* fmt, ...);                                   // set tooltip under mouse-cursor, typically use with ImGui::IsHovered(). last call wins.
    IMGUI_API void          SetTooltipV(const char* fmt, va_list args);
    IMGUI_API void          BeginTooltip();                                                     // use to create full-featured tooltip windows that aren't just text.
    IMGUI_API void          EndTooltip();

    // Layout
    IMGUI_API void          Separator();                                                        // horizontal line
    IMGUI_API void          SameLine(int column_x = 0, int spacing_w = -1);                     // call between widgets to layout them horizontally
    IMGUI_API void          Spacing();
    IMGUI_API void          Columns(int count = 1, const char* id = NULL, bool border=true);    // setup number of columns
    IMGUI_API void          NextColumn();                                                       // next column
    IMGUI_API float         GetColumnOffset(int column_index = -1);
    IMGUI_API void          SetColumnOffset(int column_index, float offset);
    IMGUI_API float         GetColumnWidth(int column_index = -1);
    IMGUI_API ImVec2        GetCursorPos();                                                     // cursor position is relative to window position
    IMGUI_API void          SetCursorPos(const ImVec2& pos);                                    // "
    IMGUI_API void          SetCursorPosX(float x);                                             // "
    IMGUI_API void          SetCursorPosY(float y);                                             // "
    IMGUI_API ImVec2        GetCursorScreenPos();                                               // cursor position in screen space
    IMGUI_API void          AlignFirstTextHeightToWidgets();                                    // call once if the first item on the line is a Text() item and you want to vertically lower it to match subsequent (bigger) widgets.
    IMGUI_API float         GetTextLineSpacing();
    IMGUI_API float         GetTextLineHeight();

    // ID scopes
    // If you are creating repeated widgets in a loop you most likely want to push a unique identifier so ImGui can differentiate them.
    IMGUI_API void          PushID(const char* str_id);                                         // push identifier into the ID stack. IDs are hash of the *entire* stack!
    IMGUI_API void          PushID(const void* ptr_id);
    IMGUI_API void          PushID(const int int_id);
    IMGUI_API void          PopID();
    IMGUI_API ImGuiID       GetID(const char* str_id);                                          // calculate unique ID (hash of whole ID stack + given parameter). useful if you want to query into ImGuiStorage yourself. otherwise rarely needed.
    IMGUI_API ImGuiID       GetID(const void* ptr_id);

    // Widgets
    IMGUI_API void          Text(const char* fmt, ...);
    IMGUI_API void          TextV(const char* fmt, va_list args);
    IMGUI_API void          TextColored(const ImVec4& col, const char* fmt, ...);               // shortcut for PushStyleColor(ImGuiCol_Text, col); Text(fmt, ...); PopStyleColor();
    IMGUI_API void          TextColoredV(const ImVec4& col, const char* fmt, va_list args);
    IMGUI_API void          TextWrapped(const char* fmt, ...);                                  // shortcut for PushTextWrapPos(0.0f); Text(fmt, ...); PopTextWrapPos();
    IMGUI_API void          TextWrappedV(const char* fmt, va_list args);
    IMGUI_API void          TextUnformatted(const char* text, const char* text_end = NULL);     // doesn't require null terminated string if 'text_end' is specified. no copy done to any bounded stack buffer, recommended for long chunks of text.
    IMGUI_API void          LabelText(const char* label, const char* fmt, ...);                 // display text+label aligned the same way as value+label widgets 
    IMGUI_API void          LabelTextV(const char* label, const char* fmt, va_list args);
    IMGUI_API void          BulletText(const char* fmt, ...);
    IMGUI_API void          BulletTextV(const char* fmt, va_list args);
    IMGUI_API bool          Button(const char* label, ImVec2 size = ImVec2(0,0), bool repeat_when_held = false);
    IMGUI_API bool          SmallButton(const char* label);
    IMGUI_API bool          CollapsingHeader(const char* label, const char* str_id = NULL, const bool display_frame = true, const bool default_open = false);
    IMGUI_API bool          SliderFloat(const char* label, float* v, float v_min, float v_max, const char* display_format = "%.3f", float power = 1.0f);     // adjust display_format to decorate the value with a prefix or a suffix. Use power!=1.0 for logarithmic sliders.
    IMGUI_API bool          SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* display_format = "%.3f", float power = 1.0f);
    IMGUI_API bool          SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* display_format = "%.3f", float power = 1.0f);
    IMGUI_API bool          SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* display_format = "%.3f", float power = 1.0f);
    IMGUI_API bool          SliderAngle(const char* label, float* v, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f);     // *v in radians
    IMGUI_API bool          SliderInt(const char* label, int* v, int v_min, int v_max, const char* display_format = "%.0f");
    IMGUI_API void          PlotLines(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0), size_t stride = sizeof(float));
    IMGUI_API void          PlotLines(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0));
    IMGUI_API void          PlotHistogram(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0), size_t stride = sizeof(float));
    IMGUI_API void          PlotHistogram(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0));
    IMGUI_API bool          Checkbox(const char* label, bool* v);
    IMGUI_API bool          CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value);
    IMGUI_API bool          RadioButton(const char* label, bool active);
    IMGUI_API bool          RadioButton(const char* label, int* v, int v_button);
    IMGUI_API bool          InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, void (*callback)(ImGuiTextEditCallbackData*) = NULL, void* user_data = NULL);
    IMGUI_API bool          InputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);
    IMGUI_API bool          InputFloat2(const char* label, float v[2], int decimal_precision = -1);
    IMGUI_API bool          InputFloat3(const char* label, float v[3], int decimal_precision = -1);
    IMGUI_API bool          InputFloat4(const char* label, float v[4], int decimal_precision = -1);
    IMGUI_API bool          InputInt(const char* label, int* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags extra_flags = 0);
    IMGUI_API bool          Combo(const char* label, int* current_item, const char** items, int items_count, int popup_height_items = 7);
    IMGUI_API bool          Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_height_items = 7);      // separate items with \0, end item-list with \0\0
    IMGUI_API bool          Combo(const char* label, int* current_item, bool (*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int popup_height_items = 7);
    IMGUI_API bool          ColorButton(const ImVec4& col, bool small_height = false, bool outline_border = true);
    IMGUI_API bool          ColorEdit3(const char* label, float col[3]);
    IMGUI_API bool          ColorEdit4(const char* label, float col[4], bool show_alpha = true);
    IMGUI_API void          ColorEditMode(ImGuiColorEditMode mode);
    IMGUI_API bool          TreeNode(const char* str_label_id);                                 // if returning 'true' the node is open and the user is responsible for calling TreePop
    IMGUI_API bool          TreeNode(const char* str_id, const char* fmt, ...);                 // "
    IMGUI_API bool          TreeNode(const void* ptr_id, const char* fmt, ...);                 // "
    IMGUI_API bool          TreeNodeV(const char* str_id, const char* fmt, va_list args);       // "
    IMGUI_API bool          TreeNodeV(const void* ptr_id, const char* fmt, va_list args);       // "
    IMGUI_API void          TreePush(const char* str_id = NULL);                                // already called by TreeNode(), but you can call Push/Pop yourself for layouting purpose
    IMGUI_API void          TreePush(const void* ptr_id = NULL);                                // "
    IMGUI_API void          TreePop();
    IMGUI_API void          OpenNextNode(bool open);                                            // force open/close the next TreeNode or CollapsingHeader

    // Value helper output "name: value". tip: freely declare your own within the ImGui namespace!
    IMGUI_API void          Value(const char* prefix, bool b);
    IMGUI_API void          Value(const char* prefix, int v);
    IMGUI_API void          Value(const char* prefix, unsigned int v);
    IMGUI_API void          Value(const char* prefix, float v, const char* float_format = NULL);
    IMGUI_API void          Color(const char* prefix, const ImVec4& v);
    IMGUI_API void          Color(const char* prefix, unsigned int v);

    // Logging
    IMGUI_API void          LogButtons();
    IMGUI_API void          LogToTTY(int max_depth = -1);
    IMGUI_API void          LogToFile(int max_depth = -1, const char* filename = NULL);
    IMGUI_API void          LogToClipboard(int max_depth = -1);

    // Utilities
    IMGUI_API bool          IsItemHovered();                                                    // was the last item active area hovered by mouse?
    IMGUI_API bool          IsItemFocused();                                                    // was the last item focused for keyboard input?
    IMGUI_API ImVec2        GetItemBoxMin();                                                    // get bounding box of last item
    IMGUI_API ImVec2        GetItemBoxMax();                                                    // get bounding box of last item
    IMGUI_API bool          IsClipped(const ImVec2& item_size);                                 // to perform coarse clipping on user's side (as an optimization)
    IMGUI_API bool          IsKeyPressed(int key_index, bool repeat = true);                    // key_index into the keys_down[512] array, imgui doesn't know the semantic of each entry
    IMGUI_API bool          IsMouseClicked(int button, bool repeat = false);
    IMGUI_API bool          IsMouseDoubleClicked(int button);
    IMGUI_API bool          IsMouseHoveringWindow();                                            // is mouse hovering current window ("window" in API names always refer to current window)
    IMGUI_API bool          IsMouseHoveringAnyWindow();                                         // is mouse hovering any active imgui window
    IMGUI_API bool          IsMouseHoveringBox(const ImVec2& box_min, const ImVec2& box_max);   // is mouse hovering given bounding box
    IMGUI_API bool          IsPosHoveringAnyWindow(const ImVec2& pos);                          // is given position hovering any active imgui window
    IMGUI_API ImVec2        GetMousePos();                                                      // shortcut to ImGui::GetIO().MousePos provided by user, to be consistent with other calls
    IMGUI_API float         GetTime();
    IMGUI_API int           GetFrameCount();
    IMGUI_API const char*   GetStyleColorName(ImGuiCol idx);
    IMGUI_API void          GetDefaultFontData(const void** fnt_data, unsigned int* fnt_size, const void** png_data, unsigned int* png_size);
    IMGUI_API ImVec2        CalcTextSize(const char* text, const char* text_end = NULL, bool hide_text_after_double_hash = true, float wrap_width = -1.0f);

} // namespace ImGui

// Flags for ImGui::Begin()
enum ImGuiWindowFlags_
{
    // Default: 0
    ImGuiWindowFlags_ShowBorders            = 1 << 0,
    ImGuiWindowFlags_NoTitleBar             = 1 << 1,
    ImGuiWindowFlags_NoResize               = 1 << 2,
    ImGuiWindowFlags_NoMove                 = 1 << 3,
    ImGuiWindowFlags_NoScrollbar            = 1 << 4,
    ImGuiWindowFlags_NoScrollWithMouse      = 1 << 5,
    ImGuiWindowFlags_AlwaysAutoResize       = 1 << 6,
    ImGuiWindowFlags_NoSavedSettings        = 1 << 7,   // Never load/save settings in .ini file
    ImGuiWindowFlags_ChildWindow            = 1 << 8,   // For internal use by BeginChild()
    ImGuiWindowFlags_ChildWindowAutoFitX    = 1 << 9,   // For internal use by BeginChild()
    ImGuiWindowFlags_ChildWindowAutoFitY    = 1 << 10,  // For internal use by BeginChild()
    ImGuiWindowFlags_ComboBox               = 1 << 11,  // For internal use by ComboBox()
    ImGuiWindowFlags_Tooltip                = 1 << 12   // For internal use by Render() when using Tooltip
};

// Flags for ImGui::InputText()
enum ImGuiInputTextFlags_
{
    // Default: 0
    ImGuiInputTextFlags_CharsDecimal        = 1 << 0,   // Allow 0123456789.+-*/
    ImGuiInputTextFlags_CharsHexadecimal    = 1 << 1,   // Allow 0123456789ABCDEFabcdef
    ImGuiInputTextFlags_AutoSelectAll       = 1 << 2,   // Select entire text when first taking focus
    ImGuiInputTextFlags_EnterReturnsTrue    = 1 << 3,   // Return 'true' when Enter is pressed (as opposed to when the value was modified)
    ImGuiInputTextFlags_CallbackCompletion  = 1 << 4,   // Call user function on pressing TAB (for completion handling)
    ImGuiInputTextFlags_CallbackHistory     = 1 << 5,   // Call user function on pressing Up/Down arrows (for history handling)
    ImGuiInputTextFlags_CallbackAlways      = 1 << 6    // Call user function every time
    //ImGuiInputTextFlags_AlignCenter       = 1 << 6,
};

// User fill ImGuiIO.KeyMap[] array with indices into the ImGuiIO.KeysDown[512] array
enum ImGuiKey_
{
    ImGuiKey_Tab,
    ImGuiKey_LeftArrow,
    ImGuiKey_RightArrow,
    ImGuiKey_UpArrow,
    ImGuiKey_DownArrow,
    ImGuiKey_Home,
    ImGuiKey_End,
    ImGuiKey_Delete,
    ImGuiKey_Backspace,
    ImGuiKey_Enter,
    ImGuiKey_Escape,
    ImGuiKey_A,         // for CTRL+A: select all
    ImGuiKey_C,         // for CTRL+C: copy
    ImGuiKey_V,         // for CTRL+V: paste
    ImGuiKey_X,         // for CTRL+X: cut
    ImGuiKey_Y,         // for CTRL+Y: redo
    ImGuiKey_Z,         // for CTRL+Z: undo
    ImGuiKey_COUNT
};

// Enumeration for PushStyleColor() / PopStyleColor()
enum ImGuiCol_
{
    ImGuiCol_Text,
    ImGuiCol_WindowBg,
    ImGuiCol_Border,
    ImGuiCol_BorderShadow,
    ImGuiCol_FrameBg,               // Background of checkbox, radio button, plot, slider, text input
    ImGuiCol_TitleBg,
    ImGuiCol_TitleBgCollapsed,
    ImGuiCol_ScrollbarBg,
    ImGuiCol_ScrollbarGrab,
    ImGuiCol_ScrollbarGrabHovered,
    ImGuiCol_ScrollbarGrabActive,
    ImGuiCol_ComboBg,
    ImGuiCol_CheckHovered,
    ImGuiCol_CheckActive,
    ImGuiCol_SliderGrab,
    ImGuiCol_SliderGrabActive,
    ImGuiCol_Button,
    ImGuiCol_ButtonHovered,
    ImGuiCol_ButtonActive,
    ImGuiCol_Header,
    ImGuiCol_HeaderHovered,
    ImGuiCol_HeaderActive,
    ImGuiCol_Column,
    ImGuiCol_ColumnHovered,
    ImGuiCol_ColumnActive,
    ImGuiCol_ResizeGrip,
    ImGuiCol_ResizeGripHovered,
    ImGuiCol_ResizeGripActive,
    ImGuiCol_CloseButton,
    ImGuiCol_CloseButtonHovered,
    ImGuiCol_CloseButtonActive,
    ImGuiCol_PlotLines,
    ImGuiCol_PlotLinesHovered,
    ImGuiCol_PlotHistogram,
    ImGuiCol_PlotHistogramHovered,
    ImGuiCol_TextSelectedBg,
    ImGuiCol_TooltipBg,
    ImGuiCol_COUNT
};

// Enumeration for PushStyleVar() / PopStyleVar()
// NB: the enum only refers to fields of ImGuiStyle() which makes sense to be pushed/poped in UI code. Feel free to add others.
enum ImGuiStyleVar_
{
    ImGuiStyleVar_Alpha,             // float
    ImGuiStyleVar_WindowPadding,     // ImVec2
    ImGuiStyleVar_WindowRounding,    // float
    ImGuiStyleVar_FramePadding,      // ImVec2
    ImGuiStyleVar_ItemSpacing,       // ImVec2
    ImGuiStyleVar_ItemInnerSpacing,  // ImVec2
    ImGuiStyleVar_TreeNodeSpacing,   // float
    ImGuiStyleVar_ColumnsMinSpacing  // float 
};

// Enumeration for ColorEditMode()
enum ImGuiColorEditMode_
{
    ImGuiColorEditMode_UserSelect = -1,
    ImGuiColorEditMode_RGB = 0,
    ImGuiColorEditMode_HSV = 1,
    ImGuiColorEditMode_HEX = 2
};

// Condition flags for ImGui::SetWindow***() and SetNextWindow***() functions
// Those functions treat 0 as a shortcut to ImGuiSetCondition_Always
enum ImGuiSetCondition_
{
    ImGuiSetCondition_Always              = 1 << 0, // Set the variable
    ImGuiSetCondition_FirstUseThisSession = 1 << 1, // Only set the variable on the first call for this window (once per session)
    ImGuiSetCondition_FirstUseEver        = 1 << 2, // Only set the variable if the window doesn't exist in the .ini file
};

struct ImGuiStyle
{
    float       Alpha;                      // Global alpha applies to everything in ImGui
    ImVec2      WindowPadding;              // Padding within a window
    ImVec2      WindowMinSize;              // Minimum window size
    ImVec2      FramePadding;               // Padding within a framed rectangle (used by most widgets)
    ImVec2      ItemSpacing;                // Horizontal and vertical spacing between widgets/lines
    ImVec2      ItemInnerSpacing;           // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)
    ImVec2      TouchExtraPadding;          // Expand bounding box for touch-based system where touch position is not accurate enough (unnecessary for mouse inputs). Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget running. So dont grow this too much!
    ImVec2      AutoFitPadding;             // Extra space after auto-fit (double-clicking on resize grip)
    float       WindowFillAlphaDefault;     // Default alpha of window background, if not specified in ImGui::Begin()
    float       WindowRounding;             // Radius of window corners rounding. Set to 0.0f to have rectangular windows
    float       TreeNodeSpacing;            // Horizontal spacing when entering a tree node
    float       ColumnsMinSpacing;          // Minimum horizontal spacing between two columns
    float       ScrollBarWidth;             // Width of the vertical scroll bar
    ImVec4      Colors[ImGuiCol_COUNT];

    IMGUI_API ImGuiStyle();
};

// This is where your app communicate with ImGui. Call ImGui::GetIO() to access.
// Read 'Programmer guide' section in .cpp file for general usage.
struct ImGuiIO
{
    //------------------------------------------------------------------
    // Settings (fill once)                 // Default value:
    //------------------------------------------------------------------

    ImVec2      DisplaySize;                // <unset>                  // Display size, in pixels. For clamping windows positions.
    float       DeltaTime;                  // = 1.0f/60.0f             // Time elapsed since last frame, in seconds.
    float       IniSavingRate;              // = 5.0f                   // Maximum time between saving .ini file, in seconds.
    const char* IniFilename;                // = "imgui.ini"            // Path to .ini file. NULL to disable .ini saving.
    const char* LogFilename;                // = "imgui_log.txt"        // Path to .log file (default parameter to ImGui::LogToFile when no file is specified).
    float       MouseDoubleClickTime;       // = 0.30f                  // Time for a double-click, in seconds.
    float       MouseDoubleClickMaxDist;    // = 6.0f                   // Distance threshold to stay in to validate a double-click, in pixels.
    int         KeyMap[ImGuiKey_COUNT];     // <unset>                  // Map of indices into the KeysDown[512] entries array
    ImFont*     Font;                       // <auto>                   // Font (also see 'Settings' fields inside ImFont structure for details)
    float       FontGlobalScale;            // = 1.0f                   // Global scale all fonts
    bool        FontAllowUserScaling;       // = false                  // Allow user scaling text of individual window with CTRL+Wheel.
    float       PixelCenterOffset;          // = 0.0f                   // Try to set to 0.5f or 0.375f if rendering is blurry

    void*       UserData;                   // = NULL                   // Store your own data for retrieval by callbacks.

    //------------------------------------------------------------------
    // User Functions
    //------------------------------------------------------------------

    // REQUIRED: rendering function. 
    // See example code if you are unsure of how to implement this.
    void        (*RenderDrawListsFn)(ImDrawList** const draw_lists, int count);      

    // Optional: access OS clipboard (default to use native Win32 clipboard on Windows, otherwise use a ImGui private clipboard)
    // Override to access OS clipboard on other architectures.
    const char* (*GetClipboardTextFn)();
    void        (*SetClipboardTextFn)(const char* text);

    // Optional: override memory allocations (default to posix malloc/realloc/free)
    void*       (*MemAllocFn)(size_t sz);
    void*       (*MemReallocFn)(void* ptr, size_t sz);
    void        (*MemFreeFn)(void* ptr);

    // Optional: notify OS Input Method Editor of the screen position of your cursor for text input position (e.g. when using Japanese/Chinese inputs in Windows)
    void        (*ImeSetInputScreenPosFn)(int x, int y);

    //------------------------------------------------------------------
    // Input - Fill before calling NewFrame()
    //------------------------------------------------------------------

    ImVec2      MousePos;                   // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
    bool        MouseDown[5];               // Mouse buttons. ImGui itself only uses button 0 (left button) but you can use others as storage for convenience.
    float       MouseWheel;                 // Mouse wheel: 1 unit scrolls about 5 lines text.  
    bool        KeyCtrl;                    // Keyboard modifier pressed: Control
    bool        KeyShift;                   // Keyboard modifier pressed: Shift
    bool        KeysDown[512];              // Keyboard keys that are pressed (in whatever order user naturally has access to keyboard data)
    ImWchar     InputCharacters[16+1];      // List of characters input (translated by user from keypress+keyboard state). Fill using AddInputCharacter() helper.

    // Function
    IMGUI_API void AddInputCharacter(ImWchar c); // Helper to add a new character into InputCharacters[]

    //------------------------------------------------------------------
    // Output - Retrieve after calling NewFrame(), you can use them to discard inputs or hide them from the rest of your application
    //------------------------------------------------------------------

    bool        WantCaptureMouse;           // Mouse is hovering a window or widget is active (= ImGui will use your mouse input)
    bool        WantCaptureKeyboard;        // Widget is active (= ImGui will use your keyboard input)

    //------------------------------------------------------------------
    // [Internal] ImGui will maintain those fields for you
    //------------------------------------------------------------------

    ImVec2      MousePosPrev;
    ImVec2      MouseDelta;
    bool        MouseClicked[5];
    ImVec2      MouseClickedPos[5];
    float       MouseClickedTime[5];
    bool        MouseDoubleClicked[5];
    float       MouseDownTime[5];
    float       KeysDownTime[512];

    IMGUI_API ImGuiIO();
};

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

// Helper: execute a block of code once a frame only
// Convenient if you want to quickly create an UI within deep-nested code that runs multiple times every frame.
// Usage:
//   IMGUI_ONCE_UPON_A_FRAME
//   {
//      // code block will be executed one per frame
//   }
// Attention! the macro expands into 2 statement so make sure you don't use it within e.g. an if() statement without curly braces.
#define IMGUI_ONCE_UPON_A_FRAME    static ImGuiOnceUponAFrame imgui_oaf##__LINE__; if (imgui_oaf##__LINE__)
struct ImGuiOnceUponAFrame
{
    ImGuiOnceUponAFrame() { RefFrame = -1; }
    mutable int RefFrame;
    operator bool() const { const int current_frame = ImGui::GetFrameCount(); if (RefFrame == current_frame) return false; RefFrame = current_frame; return true; }
};

// Helper: Parse and apply text filters. In format "aaaaa[,bbbb][,ccccc]"
struct ImGuiTextFilter
{
    struct TextRange
    {
        const char* b;
        const char* e;

        TextRange() { b = e = NULL; }
        TextRange(const char* _b, const char* _e) { b = _b; e = _e; }
        const char* begin() const { return b; }
        const char* end() const { return e; }
        bool empty() const { return b == e; }
        char front() const { return *b; }
        static bool isblank(char c) { return c == ' ' || c == '\t'; }
        void trim_blanks() { while (b < e && isblank(*b)) b++; while (e > b && isblank(*(e-1))) e--; }
        IMGUI_API void split(char separator, ImVector<TextRange>& out);
    };

    char                InputBuf[256];
    ImVector<TextRange> Filters;
    int                 CountGrep;

    ImGuiTextFilter();
    void Clear() { InputBuf[0] = 0; Build(); }
    void Draw(const char* label = "Filter (inc,-exc)", float width = -1.0f);    // Helper calling InputText+Build
    bool PassFilter(const char* val) const;
    bool IsActive() const { return !Filters.empty(); }
    IMGUI_API void Build();
};

// Helper: Text buffer for logging/accumulating text
struct ImGuiTextBuffer
{
    ImVector<char>      Buf;

    ImGuiTextBuffer()   { Buf.push_back(0); }
    ~ImGuiTextBuffer()  { clear(); }
    const char*         begin() const { return &Buf.front(); }
    const char*         end() const { return &Buf.back(); }      // Buf is zero-terminated, so end() will point on the zero-terminator
    size_t              size() const { return Buf.size()-1; }
    bool                empty() { return Buf.empty(); }
    void                clear() { Buf.clear(); Buf.push_back(0); }
    IMGUI_API void      append(const char* fmt, ...);
};

// Helper: Key->value storage
// - Store collapse state for a tree (Int 0/1)
// - Store color edit options (Int using values in ImGuiColorEditMode enum).
// - Custom user storage for temporary values.
// Typically you don't have to worry about this since a storage is held within each Window.
// Declare your own storage if:
// - You want to manipulate the open/close state of a particular sub-tree in your interface (tree node uses Int 0/1 to store their state).
// - You want to store custom debug data easily without adding or editing structures in your code.
struct ImGuiStorage
{
    struct Pair 
    { 
        ImGuiID key; 
        union { int val_i; float val_f; };        
        Pair(ImGuiID _key, int _val_i) { key = _key; val_i = _val_i; } 
        Pair(ImGuiID _key, float _val_f) { key = _key; val_f = _val_f; } 
    };
    ImVector<Pair>    Data;

    // - Get***() functions find pair, never add/allocate. Pairs are sorted so a query is O(log N)
    // - Set***() functions find pair, insertion on demand if missing.
    // - Get***Ptr() functions find pair, insertion on demand if missing, return pointer. Useful if you intend to do Get+Set. 
    //   A typical use case where this is very convenient:
    //      float* pvar = ImGui::GetIntPtr(key); ImGui::SliderInt("var", pvar, 0, 100); some_var += *pvar;
    // - Sorted insertion is costly but should amortize. A typical frame shouldn't need to insert any new pair.
    IMGUI_API void    Clear();
    IMGUI_API int     GetInt(ImGuiID key, int default_val = 0) const;
    IMGUI_API void    SetInt(ImGuiID key, int val);
    IMGUI_API int*    GetIntPtr(ImGuiID key, int default_val = 0);
    IMGUI_API float   GetFloat(ImGuiID key, float default_val = 0.0f) const;
    IMGUI_API void    SetFloat(ImGuiID key, float val);
    IMGUI_API float*  GetFloatPtr(ImGuiID key, float default_val = 0);
    IMGUI_API void    SetAllInt(int val);    // Use on your own storage if you know only integer are being stored.
};

// Shared state of InputText(), passed to callback when a ImGuiInputTextFlags_Callback* flag is used.
struct ImGuiTextEditCallbackData
{
    ImGuiKey            EventKey;       // Key pressed (Up/Down/TAB)        // Read-only    
    char*               Buf;            // Current text                     // Read-write (pointed data only)
    size_t              BufSize;        //                                  // Read-only
    bool                BufDirty;       // Set if you modify Buf directly   // Write
    ImGuiInputTextFlags Flags;          // What user passed to InputText()  // Read-only
    int                 CursorPos;      //                                  // Read-write
    int                 SelectionStart; //                                  // Read-write (== to SelectionEnd when no selection)
    int                 SelectionEnd;   //                                  // Read-write
    void*               UserData;       // What user passed to InputText()

    // NB: calling those function loses selection.
    void DeleteChars(int pos, int bytes_count);
    void InsertChars(int pos, const char* text, const char* text_end = NULL);
};

//-----------------------------------------------------------------------------
// Draw List
// Hold a series of drawing commands. The user provide a renderer for ImDrawList
//-----------------------------------------------------------------------------

struct ImDrawCmd
{
    unsigned int    vtx_count;
    ImVec4          clip_rect;
};

#ifndef IMGUI_OVERRIDE_DRAWVERT_STRUCT_LAYOUT
// Default vertex layout
struct ImDrawVert
{
    ImVec2  pos;
    ImVec2  uv;
    ImU32   col;
};
#else
// You can change the vertex format layout by defining IMGUI_OVERRIDE_DRAWVERT_STRUCT_LAYOUT.
// The code expect ImVec2 pos (8 bytes), ImVec2 uv (8 bytes), ImU32 col (4 bytes), but you can re-order them or add other fields as needed to simplify integration in your engine.
// The type has to be described by the #define (you can either declare the struct or use a typedef)
IMGUI_OVERRIDE_DRAWVERT_STRUCT_LAYOUT;
#endif

// Draw command list
// This is the low-level list of polygon that ImGui:: functions are creating. At the end of the frame, all command lists are passed to your ImGuiIO::RenderDrawListFn function for rendering.
// At the moment, each ImGui window contains its own ImDrawList but they could potentially be merged.
// If you want to add custom rendering within a window, you can use ImGui::GetWindowDrawList() to access the current draw list and add your own primitives.
// You can interleave normal ImGui:: calls and adding primitives to the current draw list.
// Note that this only gives you access to rendering polygons. If your intent is to create custom widgets and the publicly exposed functions/data aren't sufficient, you can add code in imgui_user.inl
struct ImDrawList
{
    // This is what you have to render
    ImVector<ImDrawCmd>     commands;           // commands
    ImVector<ImDrawVert>    vtx_buffer;         // each command consume ImDrawCmd::vtx_count of those

    // [Internal to ImGui]
    ImVector<ImVec4>        clip_rect_stack;    // [internal] clip rect stack while building the command-list (so text command can perform clipping early on)
    ImDrawVert*             vtx_write;          // [internal] point within vtx_buffer after each add command (to avoid using the ImVector<> operators too much)

    ImDrawList() { Clear(); }

    IMGUI_API void  Clear();
    IMGUI_API void  PushClipRect(const ImVec4& clip_rect);
    IMGUI_API void  PopClipRect();
    IMGUI_API void  ReserveVertices(unsigned int vtx_count);
    IMGUI_API void  AddVtx(const ImVec2& pos, ImU32 col);
    IMGUI_API void  AddVtxLine(const ImVec2& a, const ImVec2& b, ImU32 col);

    // Primitives   
    IMGUI_API void  AddLine(const ImVec2& a, const ImVec2& b, ImU32 col);
    IMGUI_API void  AddRect(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding = 0.0f, int rounding_corners=0x0F);
    IMGUI_API void  AddRectFilled(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding = 0.0f, int rounding_corners=0x0F);
    IMGUI_API void  AddTriangleFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col);
    IMGUI_API void  AddCircle(const ImVec2& centre, float radius, ImU32 col, int num_segments = 12);
    IMGUI_API void  AddCircleFilled(const ImVec2& centre, float radius, ImU32 col, int num_segments = 12);
    IMGUI_API void  AddArc(const ImVec2& center, float rad, ImU32 col, int a_min, int a_max, bool tris = false, const ImVec2& third_point_offset = ImVec2(0,0));
    IMGUI_API void  AddText(ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = NULL, float wrap_width = 0.0f);
};

// Bitmap font data loader & renderer into vertices
// Using the .fnt format exported by BMFont
//  - tool: http://www.angelcode.com/products/bmfont
//  - file-format: http://www.angelcode.com/products/bmfont/doc/file_format.html
// Assume valid file data (won't handle invalid/malicious data)
// Handle a subset of the options, namely:
//  - kerning pair are not supported (because some ImGui code does per-character CalcTextSize calls, need to turn it into something more state-ful to allow for kerning)
struct ImFont
{
    struct FntInfo;
    struct FntCommon;
    struct FntGlyph;
    struct FntKerning;

    // Settings
    float                       Scale;              // = 1.0f          // Base font scale, multiplied by the per-window font scale which you can adjust with SetFontScale()
    ImVec2                      DisplayOffset;      // = (0.0f,0.0f    // Offset font rendering by xx pixels
    ImVec2                      TexUvForWhite;      // = (0.0f,0.0f)   // Font texture must have a white pixel at this UV coordinate. Adjust if you are using custom texture.
    ImWchar                     FallbackChar;       // = '?'           // Replacement glyph is one isn't found.

    // Data
    unsigned char*              Data;               // Raw data, content of .fnt file
    size_t                      DataSize;           //
    bool                        DataOwned;          // 
    const FntInfo*              Info;               // (point into raw data)
    const FntCommon*            Common;             // (point into raw data)
    const FntGlyph*             Glyphs;             // (point into raw data)
    size_t                      GlyphsCount;        //
    const FntKerning*           Kerning;            // (point into raw data) - NB: kerning is unsupported
    size_t                      KerningCount;       //
    ImVector<const char*>       Filenames;          // (point into raw data)
    ImVector<int>               IndexLookup;        // (built)
    const FntGlyph*             FallbackGlyph;      // == FindGlyph(FontFallbackChar)

    IMGUI_API ImFont();
    IMGUI_API ~ImFont()         { Clear(); }

    IMGUI_API bool              LoadFromMemory(const void* data, size_t data_size);
    IMGUI_API bool              LoadFromFile(const char* filename);
    IMGUI_API void              Clear();
    IMGUI_API void              BuildLookupTable();
    IMGUI_API const FntGlyph*   FindGlyph(unsigned short c) const;
    IMGUI_API bool              IsLoaded() const { return Info != NULL && Common != NULL && Glyphs != NULL; }

    // 'max_width' stops rendering after a certain width (could be turned into a 2d size). FLT_MAX to disable.
    // 'wrap_width' enable automatic word-wrapping across multiple lines to fit into given width. 0.0f to disable.
    IMGUI_API ImVec2            CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end = NULL, const char** remaining = NULL) const; // utf8
    IMGUI_API ImVec2            CalcTextSizeW(float size, float max_width, const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining = NULL) const;                 // wchar
    IMGUI_API void              RenderText(float size, ImVec2 pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, ImDrawVert*& out_vertices, float wrap_width = 0.0f) const;

    IMGUI_API const char*       CalcWordWrapPositionA(float scale, const char* text, const char* text_end, float wrap_width) const;

#pragma pack(push, 1)
    struct FntInfo
    {
        signed short    FontSize;
        unsigned char   BitField;       // bit 0: smooth, bit 1: unicode, bit 2: italic, bit 3: bold, bit 4: fixedHeight, bits 5-7: reserved
        unsigned char   CharSet;
        unsigned short  StretchH;
        unsigned char   AA;
        unsigned char   PaddingUp, PaddingRight, PaddingDown, PaddingLeft;
        unsigned char   SpacingHoriz, SpacingVert, Outline;
        //char          FontName[];
    };

    struct FntCommon
    {
        unsigned short  LineHeight, Base;
        unsigned short  ScaleW, ScaleH;
        unsigned short  Pages;
        unsigned char   BitField;
        unsigned char   Channels[4];
    };

    struct FntGlyph
    {
        unsigned int    Id;
        unsigned short  X, Y, Width, Height;
        signed short    XOffset, YOffset;
        signed short    XAdvance;
        unsigned char   Page;
        unsigned char   Channel;
    };

    struct FntKerning
    {
        unsigned int    IdFirst;
        unsigned int    IdSecond;
        signed short    Amount;
    };
#pragma pack(pop)
};

//---- Include imgui_user.h at the end of imgui.h
//---- So you can include code that extends ImGui using any of the types declared above.
//---- (also convenient for user to only explicitly include vanilla imgui.h)
#ifdef IMGUI_INCLUDE_IMGUI_USER_H
#include "imgui_user.h"
#endif
