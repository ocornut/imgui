// ImGui library v1.11
// See .cpp file for commentary.
// See ImGui::ShowTestWindow() for sample code.
// Read 'Programmer guide' in .cpp for notes on how to setup ImGui in your codebase.
// Get latest version at https://github.com/ocornut/imgui

#pragma once

struct ImDrawList;
struct ImBitmapFont;
struct ImGuiAabb;
struct ImGuiIO;
struct ImGuiStorage;
struct ImGuiStyle;
struct ImGuiWindow;

#include "imconfig.h"
#include <float.h>          // FLT_MAX
#include <stdarg.h>         // va_list
#include <stdlib.h>         // NULL, malloc

#ifndef IM_MALLOC
#define IM_MALLOC(_SIZE) malloc((_SIZE))
#endif

#ifndef IM_FREE
#define IM_FREE(_PTR) free((_PTR))
#endif

#ifndef IM_REALLOC
#define IM_REALLOC(_PTR, _SIZE) realloc((_PTR), (_SIZE))
#endif

#ifndef IM_ASSERT
#include <assert.h>
#define IM_ASSERT(_EXPR)    assert(_EXPR)
#endif

typedef unsigned int ImU32;
typedef ImU32 ImGuiID;
typedef int ImGuiCol;               // enum ImGuiCol_
typedef int ImGuiKey;               // enum ImGuiKey_
typedef int ImGuiColorEditMode;     // enum ImGuiColorEditMode_
typedef int ImGuiWindowFlags;       // enum ImGuiWindowFlags_
typedef int ImGuiInputTextFlags;    // enum ImGuiInputTextFlags_
typedef ImBitmapFont* ImFont;

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

// std::vector<> like class to avoid dragging dependencies (also: windows implementation of STL with debug enabled is absurdly slow, so let's bypass it so our code runs fast in debug). 
// this implementation does NOT call c++ constructors! we don't need them! also only provide the minimum functionalities we need.
#ifndef ImVector
template<typename T>
class ImVector
{
private:
    size_t                      Size;
    size_t                      Capacity;
    T*                          Data;

public:
    typedef T                   value_type;
    typedef value_type*         iterator;
    typedef const value_type*   const_iterator;

    ImVector()                  { Size = Capacity = 0; Data = NULL; }
    ~ImVector()                 { if (Data) IM_FREE(Data); }

    inline bool                 empty() const                   { return Size == 0; }
    inline size_t               size() const                    { return Size; }
    inline size_t               capacity() const                { return Capacity; }

    inline value_type&          at(size_t i)                    { IM_ASSERT(i < Size); return Data[i]; }
    inline const value_type&    at(size_t i) const              { IM_ASSERT(i < Size); return Data[i]; }
    inline value_type&          operator[](size_t i)            { IM_ASSERT(i < Size); return Data[i]; }
    inline const value_type&    operator[](size_t i) const      { IM_ASSERT(i < Size); return Data[i]; }

    inline void                 clear()                         { if (Data) { Size = Capacity = 0; IM_FREE(Data); Data = NULL; } }
    inline iterator             begin()                         { return Data; }
    inline const_iterator       begin() const                   { return Data; }
    inline iterator             end()                           { return Data + Size; }
    inline const_iterator       end() const                     { return Data + Size; }
    inline value_type&          front()                         { return at(0); }
    inline const value_type&    front() const                   { return at(0); }
    inline value_type&          back()                          { IM_ASSERT(Size > 0); return at(Size-1); }
    inline const value_type&    back() const                    { IM_ASSERT(Size > 0); return at(Size-1); }
    inline void                 swap(ImVector<T>& rhs)          { const size_t rhs_size = rhs.Size; rhs.Size = Size; Size = rhs_size; const size_t rhs_cap = rhs.Capacity; rhs.Capacity = Capacity; Capacity = rhs_cap; value_type* rhs_data = rhs.Data; rhs.Data = Data; Data = rhs_data; }

    inline void                 reserve(size_t new_capacity)    { Data = (value_type*)IM_REALLOC(Data, new_capacity * sizeof(value_type)); Capacity = new_capacity; }
    inline void                 resize(size_t new_size)         { if (new_size > Capacity) reserve(new_size); Size = new_size; }

    inline void                 push_back(const value_type& v)  { if (Size == Capacity) reserve(Capacity ? Capacity * 2 : 4); Data[Size++] = v; }
    inline void                 pop_back()                      { IM_ASSERT(Size > 0); Size--; }

    inline iterator             erase(const_iterator it)        { IM_ASSERT(it >= begin() && it < end()); const int off = it - begin(); memmove(Data + off, Data + off + 1, (Size - off - 1) * sizeof(value_type)); Size--; return Data + off; }
    inline void                 insert(const_iterator it, const value_type& v)  { IM_ASSERT(it >= begin() && it <= end()); const int off = it - begin(); if (Size == Capacity) reserve(Capacity ? Capacity * 2 : 4); if (off < (int)Size) memmove(Data + off + 1, Data + off, (Size - off) * sizeof(value_type)); Data[off] = v; Size++; }
};
#endif // #ifndef ImVector

// Helpers at bottom of the file:
// - if (IMGUI_ONCE_UPON_A_FRAME)       // Execute a block of code once per frame only
// - struct ImGuiTextFilter             // Parse and apply text filters. In format "aaaaa[,bbbb][,ccccc]"
// - struct ImGuiTextBuffer             // Text buffer for logging/accumulating text
// - struct ImGuiStorage                // Custom key value storage (if you need to alter open/close states manually)
// - struct ImDrawList                  // Draw command list
// - struct ImBitmapFont                // Bitmap font loader

// ImGui End-user API
// In a namespace so that user can add extra functions (e.g. Value() helpers for your vector or common types)
namespace ImGui
{
    // Main
    ImGuiIO&    GetIO();
    ImGuiStyle& GetStyle();
    void        NewFrame();
    void        Render();
    void        Shutdown();
    void        ShowUserGuide();
    void        ShowStyleEditor(ImGuiStyle* ref = NULL);
    void        ShowTestWindow(bool* open = NULL);

    // Window
    bool        Begin(const char* name = "Debug", bool* open = NULL, ImVec2 size = ImVec2(0,0), float fill_alpha = -1.0f, ImGuiWindowFlags flags = 0);
    void        End();
    void        BeginChild(const char* str_id, ImVec2 size = ImVec2(0,0), bool border = false, ImGuiWindowFlags extra_flags = 0);
    void        EndChild();
    bool        GetWindowIsFocused();
    ImVec2      GetWindowSize();
    float       GetWindowWidth();
    ImVec2      GetWindowPos();                                                     // you should rarely need/care about the window position, but it can be useful if you want to use your own drawing.
    void        SetWindowPos(const ImVec2& pos);                                    // set current window pos.
    ImVec2      GetWindowContentRegionMin();
    ImVec2      GetWindowContentRegionMax();
    ImDrawList* GetWindowDrawList();                                                // get rendering command-list if you want to append your own draw primitives.
    void        SetFontScale(float scale);
    void        SetScrollPosHere();                                                 // adjust scrolling position to center into the current cursor position.
    void        SetTreeStateStorage(ImGuiStorage* tree);                            // replace tree state storage with our own (if you want to manipulate it yourself, typically clear subsection of it).
    ImGuiStorage* GetTreeStateStorage();
    void        PushItemWidth(float item_width);
    void        PopItemWidth();
    float       GetItemWidth();
    void        PushAllowKeyboardFocus(bool v);
    void        PopAllowKeyboardFocus();
    void        PushStyleColor(ImGuiCol idx, const ImVec4& col);
    void        PopStyleColor();

    // Tooltip
    void        SetTooltip(const char* fmt, ...);                                   // set tooltip under mouse-cursor, typically use with ImGui::IsHovered(). last call wins.
    void        BeginTooltip();                                                     // use to create full-featured tooltip windows that aren't just text.
    void        EndTooltip();

    // Layout
    void        Separator();                                                        // horizontal line
    void        SameLine(int column_x = 0, int spacing_w = -1);                     // call between widgets to layout them horizontally
    void        Spacing();
    void        Columns(int count = 1, const char* id = NULL, bool border=true);    // setup number of columns
    void        NextColumn();                                                       // next column
    float       GetColumnOffset(int column_index = -1);
    void        SetColumnOffset(int column_index, float offset);
    float       GetColumnWidth(int column_index = -1);
    ImVec2      GetCursorPos();                                                     // cursor position relative to window position
    void        SetCursorPos(const ImVec2& pos);                                    // "
    void        SetCursorPosX(float x);                                             // "
    void        SetCursorPosY(float y);                                             // "
    ImVec2      GetCursorScreenPos();												// cursor position in screen space
    void        AlignFirstTextHeightToWidgets();                                    // call once if the first item on the line is a Text() item and you want to vertically lower it to match subsequent (bigger) widgets.
    float       GetTextLineSpacing();
    float       GetTextLineHeight();

    // ID scopes
    void        PushID(const char* str_id);
    void        PushID(const void* ptr_id);
    void        PushID(const int int_id);
    void        PopID();

    // Widgets
    void        Text(const char* fmt, ...);
    void        TextV(const char* fmt, va_list args);
    void        TextColored(const ImVec4& col, const char* fmt, ...);               // shortcut to doing PushStyleColor(ImGuiCol_Text, col); Text(fmt, ...); PopStyleColor();
    void        TextUnformatted(const char* text, const char* text_end = NULL);     // doesn't require null terminated string if 'text_end' is specified. no copy done to any bounded stack buffer, better for long chunks of text.
    void        LabelText(const char* label, const char* fmt, ...);
    void        BulletText(const char* fmt, ...);
    bool        Button(const char* label, ImVec2 size = ImVec2(0,0), bool repeat_when_held = false);
    bool        SmallButton(const char* label);
    bool        CollapsingHeader(const char* label, const char* str_id = NULL, const bool display_frame = true, const bool default_open = false);
    bool        SliderFloat(const char* label, float* v, float v_min, float v_max, const char* display_format = "%.3f", float power = 1.0f);     // adjust display_format to decorate the value with a prefix or a suffix. Use power!=1.0 for logarithmic sliders.
    bool        SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* display_format = "%.3f", float power = 1.0f);
    bool        SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* display_format = "%.3f", float power = 1.0f);
    bool        SliderFloat4(const char* label, float v[3], float v_min, float v_max, const char* display_format = "%.3f", float power = 1.0f);
    bool        SliderAngle(const char* label, float* v, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f);     // *v in radians
    bool        SliderInt(const char* label, int* v, int v_min, int v_max, const char* display_format = "%.0f");
    void        PlotLines(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0), size_t stride = sizeof(float));
    void        PlotHistogram(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0), size_t stride = sizeof(float));
    bool        Checkbox(const char* label, bool* v);
    bool        CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value);
    bool        RadioButton(const char* label, bool active);
    bool        RadioButton(const char* label, int* v, int v_button);
    bool        InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0);
    bool        InputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);
    bool        InputFloat2(const char* label, float v[2], int decimal_precision = -1);
    bool        InputFloat3(const char* label, float v[3], int decimal_precision = -1);
    bool        InputFloat4(const char* label, float v[4], int decimal_precision = -1);
    bool        InputInt(const char* label, int* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags extra_flags = 0);
    bool        Combo(const char* label, int* current_item, const char** items, int items_count, int popup_height_items = 7);
    bool        Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_height_items = 7);      // Separate items with \0, end item-list with \0\0
    bool        Combo(const char* label, int* current_item, bool (*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int popup_height_items = 7);
    bool        ColorButton(const ImVec4& col, bool small_height = false, bool outline_border = true);
    bool        ColorEdit3(const char* label, float col[3]);
    bool        ColorEdit4(const char* label, float col[4], bool show_alpha = true);
    void        ColorEditMode(ImGuiColorEditMode mode);
    bool        TreeNode(const char* str_label_id);                                 // if returning 'true' the node is open and the user is responsible for calling TreePop
    bool        TreeNode(const char* str_id, const char* fmt, ...);                 // "
    bool        TreeNode(const void* ptr_id, const char* fmt, ...);                 // "
    void        TreePush(const char* str_id = NULL);                                // already called by TreeNode(), but you can call Push/Pop yourself for layouting purpose
    void        TreePush(const void* ptr_id = NULL);                                // "
    void        TreePop();
    void        OpenNextNode(bool open);                                            // force open/close the next TreeNode or CollapsingHeader

    // Value helper output "name: value"
    // Freely declare your own in the ImGui namespace.
    void        Value(const char* prefix, bool b);
    void        Value(const char* prefix, int v);
    void        Value(const char* prefix, unsigned int v);
    void        Value(const char* prefix, float v, const char* float_format = NULL);
    void        Color(const char* prefix, const ImVec4& v);
    void        Color(const char* prefix, unsigned int v);

    // Logging
    void        LogButtons();
    void        LogToTTY(int max_depth = -1);
    void        LogToFile(int max_depth = -1, const char* filename = NULL);
    void        LogToClipboard(int max_depth = -1);

    // Utilities
    void        SetNewWindowDefaultPos(const ImVec2& pos);                          // set position of window that do
    bool        IsHovered();                                                        // was the last item active area hovered by mouse?
    ImVec2      GetItemBoxMin();                                                    // get bounding box of last item
    ImVec2      GetItemBoxMax();                                                    // get bounding box of last item
    bool        IsClipped(const ImVec2& item_size);                                 // to perform coarse clipping on user's side (as an optimisation)
    bool        IsKeyPressed(int key_index, bool repeat = true);                    // key_index into the keys_down[512] array, imgui doesn't know the semantic of each entry
    bool        IsMouseClicked(int button, bool repeat = false);
    bool        IsMouseDoubleClicked(int button);
    bool        IsMouseHoveringBox(const ImVec2& box_min, const ImVec2& box_max);
    ImVec2      GetMousePos();
    float       GetTime();
    int         GetFrameCount();
    const char* GetStyleColorName(ImGuiCol idx);
    void        GetDefaultFontData(const void** fnt_data, unsigned int* fnt_size, const void** png_data, unsigned int* png_size);
    ImVec2      CalcTextSize(const char* text, const char* text_end = NULL, const bool hide_text_after_hash = true);

}; // namespace ImGui

// Flags for ImGui::Begin()
enum ImGuiWindowFlags_
{
    // Default: 0
    ImGuiWindowFlags_ShowBorders            = 1 << 0,
    ImGuiWindowFlags_NoTitleBar             = 1 << 1,
    ImGuiWindowFlags_NoResize               = 1 << 2,
    ImGuiWindowFlags_NoMove                 = 1 << 3,
    ImGuiWindowFlags_NoScrollbar            = 1 << 4,
    ImGuiWindowFlags_ChildWindow            = 1 << 5,   // For internal use by BeginChild()
    ImGuiWindowFlags_ChildWindowAutoFitX    = 1 << 6,   // For internal use by BeginChild()
    ImGuiWindowFlags_ChildWindowAutoFitY    = 1 << 7,   // For internal use by BeginChild()
    ImGuiWindowFlags_ComboBox               = 1 << 8,   // For internal use by ComboBox()
    ImGuiWindowFlags_Tooltip                = 1 << 9,   // For internal use by Render() when using Tooltip
};

// Flags for ImGui::InputText()
enum ImGuiInputTextFlags_
{
    // Default: 0
    ImGuiInputTextFlags_CharsDecimal        = 1 << 0,   // Allow 0123456789.+-*/
    ImGuiInputTextFlags_CharsHexadecimal    = 1 << 1,   // Allow 0123456789ABCDEFabcdef
    ImGuiInputTextFlags_AutoSelectAll       = 1 << 2,   // Select entire text when first taking focus
    ImGuiInputTextFlags_EnterReturnsTrue    = 1 << 3,   // Return 'true' when Enter is pressed (as opposed to when the value was modified)
    //ImGuiInputTextFlags_AlignCenter       = 1 << 3,
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
    ImGuiKey_COUNT,
};

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
    ImGuiCol_COUNT,
};

enum ImGuiColorEditMode_
{
    ImGuiColorEditMode_UserSelect = -1,
    ImGuiColorEditMode_RGB = 0,
    ImGuiColorEditMode_HSV = 1,
    ImGuiColorEditMode_HEX = 2,
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

    ImGuiStyle();
};

// This is where your app communicate with ImGui. Call ImGui::GetIO() to access.
// Read 'Programmer guide' section in .cpp file for general usage.
struct ImGuiIO
{
    // Settings (fill once)                 // Default value:
    ImVec2      DisplaySize;                // <unset>                  // Display size, in pixels. For clamping windows positions.
    float       DeltaTime;                  // = 1.0f/60.0f             // Time elapsed since last frame, in seconds.
    float       IniSavingRate;              // = 5.0f                   // Maximum time between saving .ini file, in seconds. Set to a negative value to disable .ini saving.
    const char* IniFilename;                // = "imgui.ini"            // Absolute path to .ini file.
    const char* LogFilename;                // = "imgui_log.txt"        // Absolute path to .log file.
    float       MouseDoubleClickTime;       // = 0.30f                  // Time for a double-click, in seconds.
    float       MouseDoubleClickMaxDist;    // = 6.0f                   // Distance threshold to stay in to validate a double-click, in pixels.
    int         KeyMap[ImGuiKey_COUNT];     // <unset>                  // Map of indices into the KeysDown[512] entries array
    ImFont      Font;                       // <auto>                   // Gets passed to text functions. Typedef ImFont to the type you want (ImBitmapFont* or your own font).
	float		FontYOffset;				// = 0.0f					// Offset font rendering by xx pixels in Y axis.
    ImVec2      FontTexUvForWhite;          // = (0.0f,0.0f)            // Font texture must have a white pixel at this UV coordinate. Adjust if you are using custom texture.
    bool        FontAllowScaling;           // = false                  // Set to allow scaling text with CTRL+Wheel.
    float       PixelCenterOffset;          // = 0.0f                   // Try to set to 0.5f or 0.375f if rendering is blurry

    // Settings - Rendering function (REQUIRED)
    // See example code if you are unsure of how to implement this.
    void        (*RenderDrawListsFn)(ImDrawList** const draw_lists, int count);

    // Settings - Clipboard Support
    // Override to provide your clipboard handlers.
    // On Windows architecture, defaults to use the native Win32 clipboard, otherwise default to use a ImGui private clipboard. 
    // NB- for SetClipboardTextFn, the string is *NOT* zero-terminated at 'text_end'
    const char* (*GetClipboardTextFn)();                                        
    void        (*SetClipboardTextFn)(const char* text, const char* text_end);

    // Input - Fill before calling NewFrame()
    ImVec2      MousePos;                   // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
    bool        MouseDown[5];               // Mouse buttons. ImGui itself only uses button 0 (left button) but you can use others as storage for convenience.
    int         MouseWheel;                 // Mouse wheel: -1,0,+1
    bool        KeyCtrl;                    // Keyboard modifier pressed: Control
    bool        KeyShift;                   // Keyboard modifier pressed: Shift
    bool        KeysDown[512];              // Keyboard keys that are pressed (in whatever order user naturally has access to keyboard data)
    char        InputCharacters[16];        // List of characters input (translated by user from keypress+keyboard state). Fill using AddInputCharacter() helper.

    // Output - Retrieve after calling NewFrame(), you can use them to discard inputs or hide them from the rest of your application
    bool        WantCaptureMouse;           // ImGui is using your mouse input (= window is being hovered or widget is active).
    bool        WantCaptureKeyboard;        // imGui is using your keyboard input (= widget is active).

    // Function
    void        AddInputCharacter(char c);  // Helper to add a new character into InputCharacters[]

    // [Internal] ImGui will maintain those fields for you
    ImVec2      MousePosPrev;
    ImVec2      MouseDelta;
    bool        MouseClicked[5];
    ImVec2      MouseClickedPos[5];
    float       MouseClickedTime[5];
    bool        MouseDoubleClicked[5];
    float       MouseDownTime[5];
    float       KeysDownTime[512];

    ImGuiIO();
};

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

// Helper: execute a block of code once a frame only
// Usage: if (IMGUI_ONCE_UPON_A_FRAME) {/*do something once a frame*/)
#define IMGUI_ONCE_UPON_A_FRAME         static ImGuiOncePerFrame im = ImGuiOncePerFrame()
struct ImGuiOncePerFrame
{
    ImGuiOncePerFrame() : LastFrame(-1) {}
    operator bool() const { return TryIsNewFrame(); }
private:
    mutable int LastFrame;
    bool        TryIsNewFrame() const   { const int current_frame = ImGui::GetFrameCount(); if (LastFrame == current_frame) return false; LastFrame = current_frame; return true; }
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
        void split(char separator, ImVector<TextRange>& out);
    };

    char                InputBuf[256];
    ImVector<TextRange> Filters;
    int                 CountGrep;

    ImGuiTextFilter();
    void Clear() { InputBuf[0] = 0; Build(); }
    void Draw(const char* label = "Filter (inc,-exc)", float width = -1.0f);    // Helper calling InputText+Build
    bool PassFilter(const char* val) const;
    bool IsActive() const { return !Filters.empty(); }
    void Build();
};

// Helper: Text buffer for logging/accumulating text
struct ImGuiTextBuffer
{
    ImVector<char>      Buf;

    ImGuiTextBuffer()   { Buf.push_back(0); }
    const char*         begin() const { return Buf.begin(); }
    const char*         end() const { return Buf.end()-1; }
    size_t              size() const { return Buf.size()-1; }
    bool                empty() { return Buf.empty(); }
    void                clear() { Buf.clear(); Buf.push_back(0); }
    void                append(const char* fmt, ...);
};

// Helper: Key->value storage
// - Store collapse state for a tree
// - Store color edit options, etc.
// Typically you don't have to worry about this since a storage is held within each Window.
// Declare your own storage if you want to manipulate the open/close state of a particular sub-tree in your interface.
struct ImGuiStorage
{
    struct Pair { ImU32 key; int val; };
    ImVector<Pair>  Data;

    void    Clear();
    int     GetInt(ImU32 key, int default_val = 0);
    void    SetInt(ImU32 key, int val);
    void    SetAllInt(int val);

    int*    Find(ImU32 key);
    void    Insert(ImU32 key, int val);
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

// sizeof() == 20
struct ImDrawVert
{
    ImVec2  pos;
    ImVec2  uv;
    ImU32   col;
};

// Draw command list
// User is responsible for providing a renderer for this in ImGuiIO::RenderDrawListFn
struct ImDrawList
{
    // This is what you have to render
    ImVector<ImDrawCmd>     commands;           // commands
    ImVector<ImDrawVert>    vtx_buffer;         // each command consume ImDrawCmd::vtx_count of those

    // [Internal to ImGui]
    ImVector<ImVec4>        clip_rect_stack;    // [internal] clip rect stack while building the command-list (so text command can perform clipping early on)
    ImDrawVert*             vtx_write;          // [internal] point within vtx_buffer after each add command (to avoid using the ImVector<> operators too much)

    ImDrawList() { Clear(); }

    void Clear();
    void PushClipRect(const ImVec4& clip_rect);
    void PopClipRect();
    void ReserveVertices(unsigned int vtx_count);
    void AddVtx(const ImVec2& pos, ImU32 col);
    void AddVtxLine(const ImVec2& a, const ImVec2& b, ImU32 col);

    // Primitives
    void AddLine(const ImVec2& a, const ImVec2& b, ImU32 col);
    void AddRect(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding = 0.0f, int rounding_corners=0x0F);
    void AddRectFilled(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding = 0.0f, int rounding_corners=0x0F);
    void AddTriangleFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col);
    void AddCircle(const ImVec2& centre, float radius, ImU32 col, int num_segments = 12);
    void AddCircleFilled(const ImVec2& centre, float radius, ImU32 col, int num_segments = 12);
    void AddArc(const ImVec2& center, float rad, ImU32 col, int a_min, int a_max, bool tris=false, const ImVec2& third_point_offset = ImVec2(0,0));
    void AddText(ImFont font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end);
};

// Optional bitmap font data loader & renderer into vertices
//  #define ImFont to ImBitmapFont to use
// Using the .fnt format exported by BMFont
//  - tool: http://www.angelcode.com/products/bmfont
//  - file-format: http://www.angelcode.com/products/bmfont/doc/file_format.html
// Assume valid file data (won't handle invalid/malicious data)
// Handle a subset of parameters.
//  - kerning pair are not supported (because ImGui code does per-character CalcTextSize calls, need to turn it into something more stateful to allow kerning)
struct ImBitmapFont
{
#pragma pack(push, 1)
    struct FntInfo
    {
        signed short    FontSize;
        unsigned char   BitField;       // bit 0: smooth, bit 1: unicode, bit 2: italic, bit 3: bold, bit 4: fixedHeight, bits 5-7: reserved
        unsigned char   CharSet;
        unsigned short  StretchH;
        unsigned char   AA;
        unsigned char   PaddingUp, PaddingRight, PaddingDown, PaddingLeft;
        unsigned char   SpacingHoriz, SpacingVert;
        unsigned char   Outline;
        //char          FontName[];
    };

    struct FntCommon
    {
        unsigned short  LineHeight;
        unsigned short  Base;
        unsigned short  ScaleW, ScaleH;
        unsigned short  Pages;
        unsigned char   BitField;
        unsigned char   Channels[4];
    };

    struct FntGlyph
    {
        unsigned int    Id;
        unsigned short  X, Y;
        unsigned short  Width, Height;
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

    unsigned char*          Data;               // Raw data, content of .fnt file
    size_t                  DataSize;           //
    bool                    DataOwned;          // 
    const FntInfo*          Info;               // (point into raw data)
    const FntCommon*        Common;             // (point into raw data)
    const FntGlyph*         Glyphs;             // (point into raw data)
    size_t                  GlyphsCount;        //
    const FntKerning*       Kerning;            // (point into raw data)
    size_t                  KerningCount;       //
    int                     TabCount;           // FIXME: mishandled (add fixed amount instead of aligning to column)
    ImVector<const char*>   Filenames;          // (point into raw data)
    ImVector<int>           IndexLookup;        // (built)

    ImBitmapFont();
    ~ImBitmapFont() { Clear(); }

    bool                    LoadFromMemory(const void* data, size_t data_size);
    bool                    LoadFromFile(const char* filename);
    void                    Clear();
    void                    BuildLookupTable();
    const FntGlyph *        FindGlyph(unsigned short c) const;
    float                   GetFontSize() const { return (float)Info->FontSize; }
	bool					IsLoaded() const { return Info != NULL && Common != NULL && Glyphs != NULL; }

    ImVec2                  CalcTextSize(float size, float max_width, const char* text_begin, const char* text_end, const char** remaining = NULL) const;
    void                    RenderText(float size, ImVec2 pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, ImDrawVert*& out_vertices) const;
};
