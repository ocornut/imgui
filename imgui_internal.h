// dear imgui, v1.50
// (internals)

// You may use this file to debug, understand or extend ImGui features but we don't provide any guarantee of forward compatibility!
// Implement maths operators for ImVec2 (disabled by default to not collide with using IM_VEC2_CLASS_EXTRA along with your own math types+operators)
//   #define IMGUI_DEFINE_MATH_OPERATORS

#pragma once

#ifndef IMGUI_VERSION
#error Must include imgui.h before imgui_internal.h
#endif

#include <stdio.h>      // FILE*
#include <math.h>       // sqrtf, fabsf, fmodf, powf, floorf, ceilf, cosf, sinf

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4251) // class 'xxx' needs to have dll-interface to be used by clients of struct 'xxx' // when IMGUI_API is set to__declspec(dllexport)
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"        // for stb_textedit.h
#pragma clang diagnostic ignored "-Wmissing-prototypes"     // for stb_textedit.h
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------

struct ImRect;
struct ImGuiColMod;
struct ImGuiStyleMod;
struct ImGuiGroupData;
struct ImGuiSimpleColumns;
struct ImGuiDrawContext;
struct ImGuiTextEditState;
struct ImGuiIniData;
struct ImGuiMouseCursorData;
struct ImGuiPopupRef;
struct ImGuiWindow;

typedef int ImGuiLayoutType;      // enum ImGuiLayoutType_
typedef int ImGuiButtonFlags;     // enum ImGuiButtonFlags_
typedef int ImGuiTreeNodeFlags;   // enum ImGuiTreeNodeFlags_
typedef int ImGuiSliderFlags;     // enum ImGuiSliderFlags_

//-------------------------------------------------------------------------
// STB libraries
//-------------------------------------------------------------------------

namespace ImGuiStb
{

#undef STB_TEXTEDIT_STRING
#undef STB_TEXTEDIT_CHARTYPE
#define STB_TEXTEDIT_STRING             ImGuiTextEditState
#define STB_TEXTEDIT_CHARTYPE           ImWchar
#define STB_TEXTEDIT_GETWIDTH_NEWLINE   -1.0f
#include "stb_textedit.h"

} // namespace ImGuiStb

//-----------------------------------------------------------------------------
// Context
//-----------------------------------------------------------------------------

#ifndef GImGui
extern IMGUI_API ImGuiContext* GImGui;  // Current implicit ImGui context pointer
#endif

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

#define IM_ARRAYSIZE(_ARR)      ((int)(sizeof(_ARR)/sizeof(*_ARR)))
#define IM_PI                   3.14159265358979323846f
#define IM_OFFSETOF(_TYPE,_ELM) ((size_t)&(((_TYPE*)0)->_ELM))

// Helpers: UTF-8 <> wchar
IMGUI_API int           ImTextStrToUtf8(char* buf, int buf_size, const ImWchar* in_text, const ImWchar* in_text_end);      // return output UTF-8 bytes count
IMGUI_API int           ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end);          // return input UTF-8 bytes count
IMGUI_API int           ImTextStrFromUtf8(ImWchar* buf, int buf_size, const char* in_text, const char* in_text_end, const char** in_remaining = NULL);   // return input UTF-8 bytes count
IMGUI_API int           ImTextCountCharsFromUtf8(const char* in_text, const char* in_text_end);                            // return number of UTF-8 code-points (NOT bytes count)
IMGUI_API int           ImTextCountUtf8BytesFromStr(const ImWchar* in_text, const ImWchar* in_text_end);                   // return number of bytes to express string as UTF-8 code-points

// Helpers: Misc
IMGUI_API ImU32         ImHash(const void* data, int data_size, ImU32 seed = 0);    // Pass data_size==0 for zero-terminated strings
IMGUI_API void*         ImFileLoadToMemory(const char* filename, const char* file_open_mode, int* out_file_size = NULL, int padding_bytes = 0);
IMGUI_API FILE*         ImFileOpen(const char* filename, const char* file_open_mode);         
IMGUI_API bool          ImIsPointInTriangle(const ImVec2& p, const ImVec2& a, const ImVec2& b, const ImVec2& c);
static inline bool      ImCharIsSpace(int c)            { return c == ' ' || c == '\t' || c == 0x3000; }
static inline int       ImUpperPowerOfTwo(int v)        { v--; v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; v++; return v; }

// Helpers: String
IMGUI_API int           ImStricmp(const char* str1, const char* str2);
IMGUI_API int           ImStrnicmp(const char* str1, const char* str2, int count);
IMGUI_API char*         ImStrdup(const char* str);
IMGUI_API int           ImStrlenW(const ImWchar* str);
IMGUI_API const ImWchar*ImStrbolW(const ImWchar* buf_mid_line, const ImWchar* buf_begin); // Find beginning-of-line
IMGUI_API const char*   ImStristr(const char* haystack, const char* haystack_end, const char* needle, const char* needle_end);
IMGUI_API int           ImFormatString(char* buf, int buf_size, const char* fmt, ...) IM_PRINTFARGS(3);
IMGUI_API int           ImFormatStringV(char* buf, int buf_size, const char* fmt, va_list args);

// Helpers: Math
// We are keeping those not leaking to the user by default, in the case the user has implicit cast operators between ImVec2 and its own types (when IM_VEC2_CLASS_EXTRA is defined)
#ifdef IMGUI_DEFINE_MATH_OPERATORS
static inline ImVec2 operator*(const ImVec2& lhs, const float rhs)              { return ImVec2(lhs.x*rhs, lhs.y*rhs); }
static inline ImVec2 operator/(const ImVec2& lhs, const float rhs)              { return ImVec2(lhs.x/rhs, lhs.y/rhs); }
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x+rhs.x, lhs.y+rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x-rhs.x, lhs.y-rhs.y); }
static inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x*rhs.x, lhs.y*rhs.y); }
static inline ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x/rhs.x, lhs.y/rhs.y); }
static inline ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static inline ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static inline ImVec2& operator*=(ImVec2& lhs, const float rhs)                  { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static inline ImVec2& operator/=(ImVec2& lhs, const float rhs)                  { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
static inline ImVec4 operator-(const ImVec4& lhs, const ImVec4& rhs)            { return ImVec4(lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z, lhs.w-rhs.w); }
#endif

static inline int    ImMin(int lhs, int rhs)                                    { return lhs < rhs ? lhs : rhs; }
static inline int    ImMax(int lhs, int rhs)                                    { return lhs >= rhs ? lhs : rhs; }
static inline float  ImMin(float lhs, float rhs)                                { return lhs < rhs ? lhs : rhs; }
static inline float  ImMax(float lhs, float rhs)                                { return lhs >= rhs ? lhs : rhs; }
static inline ImVec2 ImMin(const ImVec2& lhs, const ImVec2& rhs)                { return ImVec2(ImMin(lhs.x,rhs.x), ImMin(lhs.y,rhs.y)); }
static inline ImVec2 ImMax(const ImVec2& lhs, const ImVec2& rhs)                { return ImVec2(ImMax(lhs.x,rhs.x), ImMax(lhs.y,rhs.y)); }
static inline int    ImClamp(int v, int mn, int mx)                             { return (v < mn) ? mn : (v > mx) ? mx : v; }
static inline float  ImClamp(float v, float mn, float mx)                       { return (v < mn) ? mn : (v > mx) ? mx : v; }
static inline ImVec2 ImClamp(const ImVec2& f, const ImVec2& mn, ImVec2 mx)      { return ImVec2(ImClamp(f.x,mn.x,mx.x), ImClamp(f.y,mn.y,mx.y)); }
static inline float  ImSaturate(float f)                                        { return (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f; }
static inline float  ImLerp(float a, float b, float t)                          { return a + (b - a) * t; }
static inline ImVec2 ImLerp(const ImVec2& a, const ImVec2& b, const ImVec2& t)  { return ImVec2(a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y); }
static inline float  ImLengthSqr(const ImVec2& lhs)                             { return lhs.x*lhs.x + lhs.y*lhs.y; }
static inline float  ImLengthSqr(const ImVec4& lhs)                             { return lhs.x*lhs.x + lhs.y*lhs.y + lhs.z*lhs.z + lhs.w*lhs.w; }
static inline float  ImInvLength(const ImVec2& lhs, float fail_value)           { float d = lhs.x*lhs.x + lhs.y*lhs.y; if (d > 0.0f) return 1.0f / sqrtf(d); return fail_value; }
static inline float  ImFloor(float f)                                           { return (float)(int)f; }
static inline ImVec2 ImFloor(ImVec2 v)                                          { return ImVec2((float)(int)v.x, (float)(int)v.y); }

// We call C++ constructor on own allocated memory via the placement "new(ptr) Type()" syntax.
// Defining a custom placement new() with a dummy parameter allows us to bypass including <new> which on some platforms complains when user has disabled exceptions.
#ifdef IMGUI_DEFINE_PLACEMENT_NEW
struct ImPlacementNewDummy {};
inline void* operator new(size_t, ImPlacementNewDummy, void* ptr) { return ptr; }
inline void operator delete(void*, ImPlacementNewDummy, void*) {}
#define IM_PLACEMENT_NEW(_PTR)  new(ImPlacementNewDummy(), _PTR)
#endif

//-----------------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------------

enum ImGuiButtonFlags_
{
    ImGuiButtonFlags_Repeat                 = 1 << 0,   // hold to repeat
    ImGuiButtonFlags_PressedOnClickRelease  = 1 << 1,   // (default) return pressed on click+release on same item (default if no PressedOn** flag is set)
    ImGuiButtonFlags_PressedOnClick         = 1 << 2,   // return pressed on click (default requires click+release)
    ImGuiButtonFlags_PressedOnRelease       = 1 << 3,   // return pressed on release (default requires click+release)
    ImGuiButtonFlags_PressedOnDoubleClick   = 1 << 4,   // return pressed on double-click (default requires click+release)
    ImGuiButtonFlags_FlattenChilds          = 1 << 5,   // allow interaction even if a child window is overlapping
    ImGuiButtonFlags_DontClosePopups        = 1 << 6,   // disable automatically closing parent popup on press
    ImGuiButtonFlags_Disabled               = 1 << 7,   // disable interaction
    ImGuiButtonFlags_AlignTextBaseLine      = 1 << 8,   // vertically align button to match text baseline - ButtonEx() only
    ImGuiButtonFlags_NoKeyModifiers         = 1 << 9,   // disable interaction if a key modifier is held
    ImGuiButtonFlags_AllowOverlapMode       = 1 << 10   // require previous frame HoveredId to either match id or be null before being usable
};

enum ImGuiSliderFlags_
{
    ImGuiSliderFlags_Vertical               = 1 << 0
};

enum ImGuiSelectableFlagsPrivate_
{
    // NB: need to be in sync with last value of ImGuiSelectableFlags_
    ImGuiSelectableFlags_Menu               = 1 << 3,
    ImGuiSelectableFlags_MenuItem           = 1 << 4,
    ImGuiSelectableFlags_Disabled           = 1 << 5,
    ImGuiSelectableFlags_DrawFillAvailWidth = 1 << 6
};

// FIXME: this is in development, not exposed/functional as a generic feature yet.
enum ImGuiLayoutType_
{
    ImGuiLayoutType_Vertical,
    ImGuiLayoutType_Horizontal
};

enum ImGuiPlotType
{
    ImGuiPlotType_Lines,
    ImGuiPlotType_Histogram
};

enum ImGuiDataType
{
    ImGuiDataType_Int,
    ImGuiDataType_Float,
    ImGuiDataType_Float2,
};

enum ImGuiCorner
{
    ImGuiCorner_TopLeft     = 1 << 0, // 1
    ImGuiCorner_TopRight    = 1 << 1, // 2
    ImGuiCorner_BottomRight = 1 << 2, // 4
    ImGuiCorner_BottomLeft  = 1 << 3, // 8
    ImGuiCorner_All         = 0x0F
};

// 2D axis aligned bounding-box
// NB: we can't rely on ImVec2 math operators being available here
struct IMGUI_API ImRect
{
    ImVec2      Min;    // Upper-left
    ImVec2      Max;    // Lower-right

    ImRect()                                        : Min(FLT_MAX,FLT_MAX), Max(-FLT_MAX,-FLT_MAX)  {}
    ImRect(const ImVec2& min, const ImVec2& max)    : Min(min), Max(max)                            {}
    ImRect(const ImVec4& v)                         : Min(v.x, v.y), Max(v.z, v.w)                  {}
    ImRect(float x1, float y1, float x2, float y2)  : Min(x1, y1), Max(x2, y2)                      {}

    ImVec2      GetCenter() const               { return ImVec2((Min.x+Max.x)*0.5f, (Min.y+Max.y)*0.5f); }
    ImVec2      GetSize() const                 { return ImVec2(Max.x-Min.x, Max.y-Min.y); }
    float       GetWidth() const                { return Max.x-Min.x; }
    float       GetHeight() const               { return Max.y-Min.y; }
    ImVec2      GetTL() const                   { return Min; }                   // Top-left
    ImVec2      GetTR() const                   { return ImVec2(Max.x, Min.y); }  // Top-right
    ImVec2      GetBL() const                   { return ImVec2(Min.x, Max.y); }  // Bottom-left
    ImVec2      GetBR() const                   { return Max; }                   // Bottom-right
    bool        Contains(const ImVec2& p) const { return p.x >= Min.x     && p.y >= Min.y     && p.x < Max.x     && p.y < Max.y; }
    bool        Contains(const ImRect& r) const { return r.Min.x >= Min.x && r.Min.y >= Min.y && r.Max.x < Max.x && r.Max.y < Max.y; }
    bool        Overlaps(const ImRect& r) const { return r.Min.y < Max.y  && r.Max.y > Min.y  && r.Min.x < Max.x && r.Max.x > Min.x; }
    void        Add(const ImVec2& rhs)          { if (Min.x > rhs.x)     Min.x = rhs.x;     if (Min.y > rhs.y) Min.y = rhs.y;         if (Max.x < rhs.x) Max.x = rhs.x;         if (Max.y < rhs.y) Max.y = rhs.y; }
    void        Add(const ImRect& rhs)          { if (Min.x > rhs.Min.x) Min.x = rhs.Min.x; if (Min.y > rhs.Min.y) Min.y = rhs.Min.y; if (Max.x < rhs.Max.x) Max.x = rhs.Max.x; if (Max.y < rhs.Max.y) Max.y = rhs.Max.y; }
    void        Expand(const float amount)      { Min.x -= amount;   Min.y -= amount;   Max.x += amount;   Max.y += amount; }
    void        Expand(const ImVec2& amount)    { Min.x -= amount.x; Min.y -= amount.y; Max.x += amount.x; Max.y += amount.y; }
    void        Reduce(const ImVec2& amount)    { Min.x += amount.x; Min.y += amount.y; Max.x -= amount.x; Max.y -= amount.y; }
    void        Clip(const ImRect& clip)        { if (Min.x < clip.Min.x) Min.x = clip.Min.x; if (Min.y < clip.Min.y) Min.y = clip.Min.y; if (Max.x > clip.Max.x) Max.x = clip.Max.x; if (Max.y > clip.Max.y) Max.y = clip.Max.y; }
    void        Floor()                         { Min.x = (float)(int)Min.x; Min.y = (float)(int)Min.y; Max.x = (float)(int)Max.x; Max.y = (float)(int)Max.y; }
    ImVec2      GetClosestPoint(ImVec2 p, bool on_edge) const
    {
        if (!on_edge && Contains(p))
            return p;
        if (p.x > Max.x) p.x = Max.x;
        else if (p.x < Min.x) p.x = Min.x;
        if (p.y > Max.y) p.y = Max.y;
        else if (p.y < Min.y) p.y = Min.y;
        return p;
    }
};

// Stacked color modifier, backup of modified data so we can restore it
struct ImGuiColMod
{
    ImGuiCol    Col;
    ImVec4      BackupValue;
};

// Stacked style modifier, backup of modified data so we can restore it. Data type inferred from the variable.
struct ImGuiStyleMod
{
    ImGuiStyleVar   VarIdx;
    union           { int BackupInt[2]; float BackupFloat[2]; };
    ImGuiStyleMod(ImGuiStyleVar idx, int v)     { VarIdx = idx; BackupInt[0] = v; }
    ImGuiStyleMod(ImGuiStyleVar idx, float v)   { VarIdx = idx; BackupFloat[0] = v; }
    ImGuiStyleMod(ImGuiStyleVar idx, ImVec2 v)  { VarIdx = idx; BackupFloat[0] = v.x; BackupFloat[1] = v.y; }
};

// Stacked data for BeginGroup()/EndGroup()
struct ImGuiGroupData
{
    ImVec2      BackupCursorPos;
    ImVec2      BackupCursorMaxPos;
    float       BackupIndentX;
    float       BackupGroupOffsetX;
    float       BackupCurrentLineHeight;
    float       BackupCurrentLineTextBaseOffset;
    float       BackupLogLinePosY;
    bool        BackupActiveIdIsAlive;
    bool        AdvanceCursor;
};

// Per column data for Columns()
struct ImGuiColumnData
{
    float       OffsetNorm;     // Column start offset, normalized 0.0 (far left) -> 1.0 (far right)
    //float     IndentX;
};

// Simple column measurement currently used for MenuItem() only. This is very short-sighted/throw-away code and NOT a generic helper.
struct IMGUI_API ImGuiSimpleColumns
{
    int         Count;
    float       Spacing;
    float       Width, NextWidth;
    float       Pos[8], NextWidths[8];

    ImGuiSimpleColumns();
    void        Update(int count, float spacing, bool clear);
    float       DeclColumns(float w0, float w1, float w2);
    float       CalcExtraSpace(float avail_w);
};

// Internal state of the currently focused/edited text input box
struct IMGUI_API ImGuiTextEditState
{
    ImGuiID             Id;                         // widget id owning the text state
    ImVector<ImWchar>   Text;                       // edit buffer, we need to persist but can't guarantee the persistence of the user-provided buffer. so we copy into own buffer.
    ImVector<char>      InitialText;                // backup of end-user buffer at the time of focus (in UTF-8, unaltered)
    ImVector<char>      TempTextBuffer;
    int                 CurLenA, CurLenW;           // we need to maintain our buffer length in both UTF-8 and wchar format.
    int                 BufSizeA;                   // end-user buffer size
    float               ScrollX;
    ImGuiStb::STB_TexteditState   StbState;
    float               CursorAnim;
    bool                CursorFollow;
    bool                SelectedAllMouseLock;

    ImGuiTextEditState()                            { memset(this, 0, sizeof(*this)); }
    void                CursorAnimReset()           { CursorAnim = -0.30f; }                                   // After a user-input the cursor stays on for a while without blinking
    void                CursorClamp()               { StbState.cursor = ImMin(StbState.cursor, CurLenW); StbState.select_start = ImMin(StbState.select_start, CurLenW); StbState.select_end = ImMin(StbState.select_end, CurLenW); }
    bool                HasSelection() const        { return StbState.select_start != StbState.select_end; }
    void                ClearSelection()            { StbState.select_start = StbState.select_end = StbState.cursor; }
    void                SelectAll()                 { StbState.select_start = 0; StbState.select_end = CurLenW; StbState.cursor = StbState.select_end; StbState.has_preferred_x = false; }
    void                OnKeyPressed(int key);
};

// Data saved in imgui.ini file
struct ImGuiIniData
{
    char*       Name;
    ImGuiID     Id;
    ImVec2      Pos;
    ImVec2      Size;
    bool        Collapsed;
};

// Mouse cursor data (used when io.MouseDrawCursor is set)
struct ImGuiMouseCursorData
{
    ImGuiMouseCursor    Type;
    ImVec2              HotOffset;
    ImVec2              Size;
    ImVec2              TexUvMin[2];
    ImVec2              TexUvMax[2];
};

// Storage for current popup stack
struct ImGuiPopupRef
{
    ImGuiID         PopupId;        // Set on OpenPopup()
    ImGuiWindow*    Window;         // Resolved on BeginPopup() - may stay unresolved if user never calls OpenPopup()
    ImGuiWindow*    ParentWindow;   // Set on OpenPopup()
    ImGuiID         ParentMenuSet;  // Set on OpenPopup()
    ImVec2          MousePosOnOpen; // Copy of mouse position at the time of opening popup

    ImGuiPopupRef(ImGuiID id, ImGuiWindow* parent_window, ImGuiID parent_menu_set, const ImVec2& mouse_pos) { PopupId = id; Window = NULL; ParentWindow = parent_window; ParentMenuSet = parent_menu_set; MousePosOnOpen = mouse_pos; }
};

// Main state for ImGui
struct ImGuiContext
{
    bool                    Initialized;
    ImGuiIO                 IO;
    ImGuiStyle              Style;
    ImFont*                 Font;                               // (Shortcut) == FontStack.empty() ? IO.Font : FontStack.back()
    float                   FontSize;                           // (Shortcut) == FontBaseSize * g.CurrentWindow->FontWindowScale == window->FontSize()
    float                   FontBaseSize;                       // (Shortcut) == IO.FontGlobalScale * Font->Scale * Font->FontSize. Size of characters.
    ImVec2                  FontTexUvWhitePixel;                // (Shortcut) == Font->TexUvWhitePixel

    float                   Time;
    int                     FrameCount;
    int                     FrameCountEnded;
    int                     FrameCountRendered;
    ImVector<ImGuiWindow*>  Windows;
    ImVector<ImGuiWindow*>  WindowsSortBuffer;
    ImGuiWindow*            CurrentWindow;                      // Being drawn into
    ImVector<ImGuiWindow*>  CurrentWindowStack;
    ImGuiWindow*            FocusedWindow;                      // Will catch keyboard inputs
    ImGuiWindow*            HoveredWindow;                      // Will catch mouse inputs
    ImGuiWindow*            HoveredRootWindow;                  // Will catch mouse inputs (for focus/move only)
    ImGuiID                 HoveredId;                          // Hovered widget
    bool                    HoveredIdAllowOverlap;
    ImGuiID                 HoveredIdPreviousFrame;
    ImGuiID                 ActiveId;                           // Active widget
    ImGuiID                 ActiveIdPreviousFrame;
    bool                    ActiveIdIsAlive;
    bool                    ActiveIdIsJustActivated;            // Set at the time of activation for one frame
    bool                    ActiveIdAllowOverlap;               // Set only by active widget
    ImVec2                  ActiveIdClickOffset;                // Clicked offset from upper-left corner, if applicable (currently only set by ButtonBehavior)
    ImGuiWindow*            ActiveIdWindow;
    ImGuiWindow*            MovedWindow;                        // Track the child window we clicked on to move a window.
    ImGuiID                 MovedWindowMoveId;                  // == MovedWindow->RootWindow->MoveId
    ImVector<ImGuiIniData>  Settings;                           // .ini Settings
    float                   SettingsDirtyTimer;                 // Save .ini Settings on disk when time reaches zero
    ImVector<ImGuiColMod>   ColorModifiers;                     // Stack for PushStyleColor()/PopStyleColor()
    ImVector<ImGuiStyleMod> StyleModifiers;                     // Stack for PushStyleVar()/PopStyleVar()
    ImVector<ImFont*>       FontStack;                          // Stack for PushFont()/PopFont()
    ImVector<ImGuiPopupRef> OpenPopupStack;                     // Which popups are open (persistent)
    ImVector<ImGuiPopupRef> CurrentPopupStack;                  // Which level of BeginPopup() we are in (reset every frame)

    // Storage for SetNexWindow** and SetNextTreeNode*** functions
    ImVec2                  SetNextWindowPosVal;
    ImVec2                  SetNextWindowSizeVal;
    ImVec2                  SetNextWindowContentSizeVal;
    bool                    SetNextWindowCollapsedVal;
    ImGuiSetCond            SetNextWindowPosCond;
    ImGuiSetCond            SetNextWindowSizeCond;
    ImGuiSetCond            SetNextWindowContentSizeCond;
    ImGuiSetCond            SetNextWindowCollapsedCond;
    ImRect                  SetNextWindowSizeConstraintRect;           // Valid if 'SetNextWindowSizeConstraint' is true
    ImGuiSizeConstraintCallback SetNextWindowSizeConstraintCallback;
    void*                       SetNextWindowSizeConstraintCallbackUserData;
    bool                    SetNextWindowSizeConstraint;
    bool                    SetNextWindowFocus;
    bool                    SetNextTreeNodeOpenVal;
    ImGuiSetCond            SetNextTreeNodeOpenCond;

    // Render
    ImDrawData              RenderDrawData;                     // Main ImDrawData instance to pass render information to the user
    ImVector<ImDrawList*>   RenderDrawLists[3];
    float                   ModalWindowDarkeningRatio;
    ImDrawList              OverlayDrawList;                    // Optional software render of mouse cursors, if io.MouseDrawCursor is set + a few debug overlays
    ImGuiMouseCursor        MouseCursor;
    ImGuiMouseCursorData    MouseCursorData[ImGuiMouseCursor_Count_];

    // Widget state
    ImGuiTextEditState      InputTextState;
    ImFont                  InputTextPasswordFont;
    ImGuiID                 ScalarAsInputTextId;                // Temporary text input when CTRL+clicking on a slider, etc.
    ImGuiStorage            ColorEditModeStorage;               // Store user selection of color edit mode
    float                   DragCurrentValue;                   // Currently dragged value, always float, not rounded by end-user precision settings
    ImVec2                  DragLastMouseDelta;
    float                   DragSpeedDefaultRatio;              // If speed == 0.0f, uses (max-min) * DragSpeedDefaultRatio
    float                   DragSpeedScaleSlow;
    float                   DragSpeedScaleFast;
    ImVec2                  ScrollbarClickDeltaToGrabCenter;    // Distance between mouse and center of grab box, normalized in parent space. Use storage?
    char                    Tooltip[1024];
    char*                   PrivateClipboard;                   // If no custom clipboard handler is defined
    ImVec2                  OsImePosRequest, OsImePosSet;       // Cursor position request & last passed to the OS Input Method Editor

    // Logging
    bool                    LogEnabled;
    FILE*                   LogFile;                            // If != NULL log to stdout/ file
    ImGuiTextBuffer*        LogClipboard;                       // Else log to clipboard. This is pointer so our GImGui static constructor doesn't call heap allocators.
    int                     LogStartDepth;
    int                     LogAutoExpandMaxDepth;

    // Misc
    float                   FramerateSecPerFrame[120];          // calculate estimate of framerate for user
    int                     FramerateSecPerFrameIdx;
    float                   FramerateSecPerFrameAccum;
    int                     CaptureMouseNextFrame;              // explicit capture via CaptureInputs() sets those flags
    int                     CaptureKeyboardNextFrame;
    char                    TempBuffer[1024*3+1];               // temporary text buffer

    ImGuiContext()
    {
        Initialized = false;
        Font = NULL;
        FontSize = FontBaseSize = 0.0f;
        FontTexUvWhitePixel = ImVec2(0.0f, 0.0f);

        Time = 0.0f;
        FrameCount = 0;
        FrameCountEnded = FrameCountRendered = -1;
        CurrentWindow = NULL;
        FocusedWindow = NULL;
        HoveredWindow = NULL;
        HoveredRootWindow = NULL;
        HoveredId = 0;
        HoveredIdAllowOverlap = false;
        HoveredIdPreviousFrame = 0;
        ActiveId = 0;
        ActiveIdPreviousFrame = 0;
        ActiveIdIsAlive = false;
        ActiveIdIsJustActivated = false;
        ActiveIdAllowOverlap = false;
        ActiveIdClickOffset = ImVec2(-1,-1);
        ActiveIdWindow = NULL;
        MovedWindow = NULL;
        MovedWindowMoveId = 0;
        SettingsDirtyTimer = 0.0f;

        SetNextWindowPosVal = ImVec2(0.0f, 0.0f);
        SetNextWindowSizeVal = ImVec2(0.0f, 0.0f);
        SetNextWindowCollapsedVal = false;
        SetNextWindowPosCond = 0;
        SetNextWindowSizeCond = 0;
        SetNextWindowContentSizeCond = 0;
        SetNextWindowCollapsedCond = 0;
        SetNextWindowSizeConstraintRect = ImRect();
        SetNextWindowSizeConstraintCallback = NULL;
        SetNextWindowSizeConstraintCallbackUserData = NULL;
        SetNextWindowSizeConstraint = false;
        SetNextWindowFocus = false;
        SetNextTreeNodeOpenVal = false;
        SetNextTreeNodeOpenCond = 0;

        ScalarAsInputTextId = 0;
        DragCurrentValue = 0.0f;
        DragLastMouseDelta = ImVec2(0.0f, 0.0f);
        DragSpeedDefaultRatio = 1.0f / 100.0f;
        DragSpeedScaleSlow = 0.01f;
        DragSpeedScaleFast = 10.0f;
        ScrollbarClickDeltaToGrabCenter = ImVec2(0.0f, 0.0f);
        memset(Tooltip, 0, sizeof(Tooltip));
        PrivateClipboard = NULL;
        OsImePosRequest = OsImePosSet = ImVec2(-1.0f, -1.0f);

        ModalWindowDarkeningRatio = 0.0f;
        OverlayDrawList._OwnerName = "##Overlay"; // Give it a name for debugging
        MouseCursor = ImGuiMouseCursor_Arrow;
        memset(MouseCursorData, 0, sizeof(MouseCursorData));

        LogEnabled = false;
        LogFile = NULL;
        LogClipboard = NULL;
        LogStartDepth = 0;
        LogAutoExpandMaxDepth = 2;

        memset(FramerateSecPerFrame, 0, sizeof(FramerateSecPerFrame));
        FramerateSecPerFrameIdx = 0;
        FramerateSecPerFrameAccum = 0.0f;
        CaptureMouseNextFrame = CaptureKeyboardNextFrame = -1;
        memset(TempBuffer, 0, sizeof(TempBuffer));
    }
};

// Transient per-window data, reset at the beginning of the frame
// FIXME: That's theory, in practice the delimitation between ImGuiWindow and ImGuiDrawContext is quite tenuous and could be reconsidered.
struct IMGUI_API ImGuiDrawContext
{
    ImVec2                  CursorPos;
    ImVec2                  CursorPosPrevLine;
    ImVec2                  CursorStartPos;
    ImVec2                  CursorMaxPos;           // Implicitly calculate the size of our contents, always extending. Saved into window->SizeContents at the end of the frame
    float                   CurrentLineHeight;
    float                   CurrentLineTextBaseOffset;
    float                   PrevLineHeight;
    float                   PrevLineTextBaseOffset;
    float                   LogLinePosY;
    int                     TreeDepth;
    ImGuiID                 LastItemId;
    ImRect                  LastItemRect;
    bool                    LastItemHoveredAndUsable;  // Item rectangle is hovered, and its window is currently interactable with (not blocked by a popup preventing access to the window)
    bool                    LastItemHoveredRect;       // Item rectangle is hovered, but its window may or not be currently interactable with (might be blocked by a popup preventing access to the window)
    bool                    MenuBarAppending;
    float                   MenuBarOffsetX;
    ImVector<ImGuiWindow*>  ChildWindows;
    ImGuiStorage*           StateStorage;
    ImGuiLayoutType         LayoutType;

    // We store the current settings outside of the vectors to increase memory locality (reduce cache misses). The vectors are rarely modified. Also it allows us to not heap allocate for short-lived windows which are not using those settings.
    float                   ItemWidth;              // == ItemWidthStack.back(). 0.0: default, >0.0: width in pixels, <0.0: align xx pixels to the right of window
    float                   TextWrapPos;            // == TextWrapPosStack.back() [empty == -1.0f]
    bool                    AllowKeyboardFocus;     // == AllowKeyboardFocusStack.back() [empty == true]
    bool                    ButtonRepeat;           // == ButtonRepeatStack.back() [empty == false]
    ImVector<float>         ItemWidthStack;
    ImVector<float>         TextWrapPosStack;
    ImVector<bool>          AllowKeyboardFocusStack;
    ImVector<bool>          ButtonRepeatStack;
    ImVector<ImGuiGroupData>GroupStack;
    ImGuiColorEditMode      ColorEditMode;
    int                     StackSizesBackup[6];    // Store size of various stacks for asserting

    float                   IndentX;                // Indentation / start position from left of window (increased by TreePush/TreePop, etc.)
    float                   GroupOffsetX;
    float                   ColumnsOffsetX;         // Offset to the current column (if ColumnsCurrent > 0). FIXME: This and the above should be a stack to allow use cases like Tree->Column->Tree. Need revamp columns API.
    int                     ColumnsCurrent;
    int                     ColumnsCount;
    float                   ColumnsMinX;
    float                   ColumnsMaxX;
    float                   ColumnsStartPosY;
    float                   ColumnsCellMinY;
    float                   ColumnsCellMaxY;
    bool                    ColumnsShowBorders;
    ImGuiID                 ColumnsSetId;
    ImVector<ImGuiColumnData> ColumnsData;

    ImGuiDrawContext()
    {
        CursorPos = CursorPosPrevLine = CursorStartPos = CursorMaxPos = ImVec2(0.0f, 0.0f);
        CurrentLineHeight = PrevLineHeight = 0.0f;
        CurrentLineTextBaseOffset = PrevLineTextBaseOffset = 0.0f;
        LogLinePosY = -1.0f;
        TreeDepth = 0;
        LastItemId = 0;
        LastItemRect = ImRect(0.0f,0.0f,0.0f,0.0f);
        LastItemHoveredAndUsable = LastItemHoveredRect = false;
        MenuBarAppending = false;
        MenuBarOffsetX = 0.0f;
        StateStorage = NULL;
        LayoutType = ImGuiLayoutType_Vertical;
        ItemWidth = 0.0f;
        ButtonRepeat = false;
        AllowKeyboardFocus = true;
        TextWrapPos = -1.0f;
        ColorEditMode = ImGuiColorEditMode_RGB;
        memset(StackSizesBackup, 0, sizeof(StackSizesBackup));

        IndentX = 0.0f;
        GroupOffsetX = 0.0f;
        ColumnsOffsetX = 0.0f;
        ColumnsCurrent = 0;
        ColumnsCount = 1;
        ColumnsMinX = ColumnsMaxX = 0.0f;
        ColumnsStartPosY = 0.0f;
        ColumnsCellMinY = ColumnsCellMaxY = 0.0f;
        ColumnsShowBorders = true;
        ColumnsSetId = 0;
    }
};

// Windows data
struct IMGUI_API ImGuiWindow
{
    char*                   Name;
    ImGuiID                 ID;                                 // == ImHash(Name)
    ImGuiWindowFlags        Flags;                              // See enum ImGuiWindowFlags_
    int                     IndexWithinParent;                  // Order within immediate parent window, if we are a child window. Otherwise 0.
    ImVec2                  PosFloat;
    ImVec2                  Pos;                                // Position rounded-up to nearest pixel
    ImVec2                  Size;                               // Current size (==SizeFull or collapsed title bar size)
    ImVec2                  SizeFull;                           // Size when non collapsed
    ImVec2                  SizeContents;                       // Size of contents (== extents reach of the drawing cursor) from previous frame
    ImVec2                  SizeContentsExplicit;               // Size of contents explicitly set by the user via SetNextWindowContentSize()
    ImRect                  ContentsRegionRect;                 // Maximum visible content position in window coordinates. ~~ (SizeContentsExplicit ? SizeContentsExplicit : Size - ScrollbarSizes) - CursorStartPos, per axis
    ImVec2                  WindowPadding;                      // Window padding at the time of begin. We need to lock it, in particular manipulation of the ShowBorder would have an effect
    ImGuiID                 MoveId;                             // == window->GetID("#MOVE")
    ImVec2                  Scroll;
    ImVec2                  ScrollTarget;                       // target scroll position. stored as cursor position with scrolling canceled out, so the highest point is always 0.0f. (FLT_MAX for no change)
    ImVec2                  ScrollTargetCenterRatio;            // 0.0f = scroll so that target position is at top, 0.5f = scroll so that target position is centered
    bool                    ScrollbarX, ScrollbarY;
    ImVec2                  ScrollbarSizes;
    float                   BorderSize;
    bool                    Active;                             // Set to true on Begin()
    bool                    WasActive;
    bool                    Accessed;                           // Set to true when any widget access the current window
    bool                    Collapsed;                          // Set when collapsing window to become only title-bar
    bool                    SkipItems;                          // == Visible && !Collapsed
    int                     BeginCount;                         // Number of Begin() during the current frame (generally 0 or 1, 1+ if appending via multiple Begin/End pairs)
    ImGuiID                 PopupId;                            // ID in the popup stack when this window is used as a popup/menu (because we use generic Name/ID for recycling)
    int                     AutoFitFramesX, AutoFitFramesY;
    bool                    AutoFitOnlyGrows;
    int                     AutoPosLastDirection;
    int                     HiddenFrames;
    int                     SetWindowPosAllowFlags;             // bit ImGuiSetCond_*** specify if SetWindowPos() call will succeed with this particular flag.
    int                     SetWindowSizeAllowFlags;            // bit ImGuiSetCond_*** specify if SetWindowSize() call will succeed with this particular flag.
    int                     SetWindowCollapsedAllowFlags;       // bit ImGuiSetCond_*** specify if SetWindowCollapsed() call will succeed with this particular flag.
    bool                    SetWindowPosCenterWanted;

    ImGuiDrawContext        DC;                                 // Temporary per-window data, reset at the beginning of the frame
    ImVector<ImGuiID>       IDStack;                            // ID stack. ID are hashes seeded with the value at the top of the stack
    ImRect                  ClipRect;                           // = DrawList->clip_rect_stack.back(). Scissoring / clipping rectangle. x1, y1, x2, y2.
    ImRect                  WindowRectClipped;                  // = WindowRect just after setup in Begin(). == window->Rect() for root window.
    int                     LastFrameActive;
    float                   ItemWidthDefault;
    ImGuiSimpleColumns      MenuColumns;                        // Simplified columns storage for menu items
    ImGuiStorage            StateStorage;
    float                   FontWindowScale;                    // Scale multiplier per-window
    ImDrawList*             DrawList;
    ImGuiWindow*            RootWindow;                         // If we are a child window, this is pointing to the first non-child parent window. Else point to ourself.
    ImGuiWindow*            RootNonPopupWindow;                 // If we are a child window, this is pointing to the first non-child non-popup parent window. Else point to ourself.
    ImGuiWindow*            ParentWindow;                       // If we are a child window, this is pointing to our parent window. Else point to NULL.

    // Navigation / Focus
    int                     FocusIdxAllCounter;                 // Start at -1 and increase as assigned via FocusItemRegister()
    int                     FocusIdxTabCounter;                 // (same, but only count widgets which you can Tab through)
    int                     FocusIdxAllRequestCurrent;          // Item being requested for focus
    int                     FocusIdxTabRequestCurrent;          // Tab-able item being requested for focus
    int                     FocusIdxAllRequestNext;             // Item being requested for focus, for next update (relies on layout to be stable between the frame pressing TAB and the next frame)
    int                     FocusIdxTabRequestNext;             // "

public:
    ImGuiWindow(const char* name);
    ~ImGuiWindow();

    ImGuiID     GetID(const char* str, const char* str_end = NULL);
    ImGuiID     GetID(const void* ptr);
    ImGuiID     GetIDNoKeepAlive(const char* str, const char* str_end = NULL);

    ImRect      Rect() const                            { return ImRect(Pos.x, Pos.y, Pos.x+Size.x, Pos.y+Size.y); }
    float       CalcFontSize() const                    { return GImGui->FontBaseSize * FontWindowScale; }
    float       TitleBarHeight() const                  { return (Flags & ImGuiWindowFlags_NoTitleBar) ? 0.0f : CalcFontSize() + GImGui->Style.FramePadding.y * 2.0f; }
    ImRect      TitleBarRect() const                    { return ImRect(Pos, ImVec2(Pos.x + SizeFull.x, Pos.y + TitleBarHeight())); }
    float       MenuBarHeight() const                   { return (Flags & ImGuiWindowFlags_MenuBar) ? CalcFontSize() + GImGui->Style.FramePadding.y * 2.0f : 0.0f; }
    ImRect      MenuBarRect() const                     { float y1 = Pos.y + TitleBarHeight(); return ImRect(Pos.x, y1, Pos.x + SizeFull.x, y1 + MenuBarHeight()); }
};

//-----------------------------------------------------------------------------
// Internal API
// No guarantee of forward compatibility here.
//-----------------------------------------------------------------------------

namespace ImGui
{
    // We should always have a CurrentWindow in the stack (there is an implicit "Debug" window)
    // If this ever crash because g.CurrentWindow is NULL it means that either
    // - ImGui::NewFrame() has never been called, which is illegal.
    // - You are calling ImGui functions after ImGui::Render() and before the next ImGui::NewFrame(), which is also illegal.
    inline    ImGuiWindow*  GetCurrentWindowRead()      { ImGuiContext& g = *GImGui; return g.CurrentWindow; }
    inline    ImGuiWindow*  GetCurrentWindow()          { ImGuiContext& g = *GImGui; g.CurrentWindow->Accessed = true; return g.CurrentWindow; }
    IMGUI_API ImGuiWindow*  GetParentWindow();
    IMGUI_API ImGuiWindow*  FindWindowByName(const char* name);
    IMGUI_API void          FocusWindow(ImGuiWindow* window);

    IMGUI_API void          EndFrame();                 // Ends the ImGui frame. Automatically called by Render()! you most likely don't need to ever call that yourself directly. If you don't need to render you can call EndFrame() but you'll have wasted CPU already. If you don't need to render, don't create any windows instead!

    IMGUI_API void          SetActiveID(ImGuiID id, ImGuiWindow* window);
	IMGUI_API void          ClearActiveID();
    IMGUI_API void          SetHoveredID(ImGuiID id);
    IMGUI_API void          KeepAliveID(ImGuiID id);

    IMGUI_API void          ItemSize(const ImVec2& size, float text_offset_y = 0.0f);
    IMGUI_API void          ItemSize(const ImRect& bb, float text_offset_y = 0.0f);
    IMGUI_API bool          ItemAdd(const ImRect& bb, const ImGuiID* id);
    IMGUI_API bool          IsClippedEx(const ImRect& bb, const ImGuiID* id, bool clip_even_when_logged);
    IMGUI_API bool          IsHovered(const ImRect& bb, ImGuiID id, bool flatten_childs = false);
    IMGUI_API bool          FocusableItemRegister(ImGuiWindow* window, bool is_active, bool tab_stop = true);      // Return true if focus is requested
    IMGUI_API void          FocusableItemUnregister(ImGuiWindow* window);
    IMGUI_API ImVec2        CalcItemSize(ImVec2 size, float default_x, float default_y);
    IMGUI_API float         CalcWrapWidthForPos(const ImVec2& pos, float wrap_pos_x);

    IMGUI_API void          OpenPopupEx(const char* str_id, bool reopen_existing);

    // NB: All position are in absolute pixels coordinates (not window coordinates)
    // FIXME: All those functions are a mess and needs to be refactored into something decent. AVOID USING OUTSIDE OF IMGUI.CPP! NOT FOR PUBLIC CONSUMPTION.
    // We need: a sort of symbol library, preferably baked into font atlas when possible + decent text rendering helpers.
    IMGUI_API void          RenderText(ImVec2 pos, const char* text, const char* text_end = NULL, bool hide_text_after_hash = true);
    IMGUI_API void          RenderTextWrapped(ImVec2 pos, const char* text, const char* text_end, float wrap_width);
    IMGUI_API void          RenderTextClipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& align = ImVec2(0,0), const ImRect* clip_rect = NULL);
    IMGUI_API void          RenderFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border = true, float rounding = 0.0f);
    IMGUI_API void          RenderCollapseTriangle(ImVec2 pos, bool is_open, float scale = 1.0f);
    IMGUI_API void          RenderBullet(ImVec2 pos);
    IMGUI_API void          RenderCheckMark(ImVec2 pos, ImU32 col);
    IMGUI_API const char*   FindRenderedTextEnd(const char* text, const char* text_end = NULL); // Find the optional ## from which we stop displaying text.

    IMGUI_API bool          ButtonBehavior(const ImRect& bb, ImGuiID id, bool* out_hovered, bool* out_held, ImGuiButtonFlags flags = 0);
    IMGUI_API bool          ButtonEx(const char* label, const ImVec2& size_arg = ImVec2(0,0), ImGuiButtonFlags flags = 0);
    IMGUI_API bool          CloseButton(ImGuiID id, const ImVec2& pos, float radius);

    IMGUI_API bool          SliderBehavior(const ImRect& frame_bb, ImGuiID id, float* v, float v_min, float v_max, float power, int decimal_precision, ImGuiSliderFlags flags = 0);
    IMGUI_API bool          SliderFloatN(const char* label, float* v, int components, float v_min, float v_max, const char* display_format, float power);
    IMGUI_API bool          SliderIntN(const char* label, int* v, int components, int v_min, int v_max, const char* display_format);

    IMGUI_API bool          DragBehavior(const ImRect& frame_bb, ImGuiID id, float* v, float v_speed, float v_min, float v_max, int decimal_precision, float power);
    IMGUI_API bool          DragFloatN(const char* label, float* v, int components, float v_speed, float v_min, float v_max, const char* display_format, float power);
    IMGUI_API bool          DragIntN(const char* label, int* v, int components, float v_speed, int v_min, int v_max, const char* display_format);

    IMGUI_API bool          InputTextEx(const char* label, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback = NULL, void* user_data = NULL);
    IMGUI_API bool          InputFloatN(const char* label, float* v, int components, int decimal_precision, ImGuiInputTextFlags extra_flags);
    IMGUI_API bool          InputIntN(const char* label, int* v, int components, ImGuiInputTextFlags extra_flags);
    IMGUI_API bool          InputScalarEx(const char* label, ImGuiDataType data_type, void* data_ptr, void* step_ptr, void* step_fast_ptr, const char* scalar_format, ImGuiInputTextFlags extra_flags);
    IMGUI_API bool          InputScalarAsWidgetReplacement(const ImRect& aabb, const char* label, ImGuiDataType data_type, void* data_ptr, ImGuiID id, int decimal_precision);

    IMGUI_API bool          TreeNodeBehavior(ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end = NULL);
    IMGUI_API bool          TreeNodeBehaviorIsOpen(ImGuiID id, ImGuiTreeNodeFlags flags = 0);                     // Consume previous SetNextTreeNodeOpened() data, if any. May return true when logging
    IMGUI_API void          TreePushRawID(ImGuiID id);

    IMGUI_API void          PlotEx(ImGuiPlotType plot_type, const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size);

    IMGUI_API int           ParseFormatPrecision(const char* fmt, int default_value);
    IMGUI_API float         RoundScalar(float value, int decimal_precision);

} // namespace ImGui

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning (pop)
#endif
