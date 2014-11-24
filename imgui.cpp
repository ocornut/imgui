// ImGui library v1.17 wip
// See ImGui::ShowTestWindow() for sample code.
// Read 'Programmer guide' below for notes on how to setup ImGui in your codebase.
// Get latest version at https://github.com/ocornut/imgui
// Developed by Omar Cornut and contributors.

/*

 Index
 - MISSION STATEMENT
 - END-USER GUIDE
 - PROGRAMMER GUIDE
 - TROUBLESHOOTING & FREQUENTLY ASKED QUESTIONS
 - API BREAKING CHANGES
 - ISSUES & TODO-LIST
 - CODE
 - SAMPLE CODE
 - FONT DATA

 
 MISSION STATEMENT
 =================

 - easy to use to create code-driven and data-driven tools
 - easy to use to create ad hoc short-lived tools and long-lived, more elaborate tools
 - easy to hack and improve
 - minimize screen real-estate usage
 - minimize setup and maintenance
 - minimize state storage on user side
 - portable, minimize dependencies, run on target (consoles, etc.)
 - efficient runtime (NB- we do allocate when "growing" content - creating a window / opening a tree node for the first time, etc. - but a typical frame won't allocate anything)
 - read about immediate-mode GUI principles @ http://mollyrocket.com/861, http://mollyrocket.com/forums/index.html

 Designed for developers and content-creators, not the typical end-user! Some of the weaknesses includes:
 - doesn't look fancy, doesn't animate
 - limited layout features, intricate layouts are typically crafted in code
 - occasionally use statically sized buffers for string manipulations - won't crash, but some long text may be clipped
 
 END-USER GUIDE
 ==============

 - double-click title bar to collapse window
 - click upper right corner to close a window, available when 'bool* open' is passed to ImGui::Begin()
 - click and drag on lower right corner to resize window
 - click and drag on any empty space to move window
 - double-click/double-tap on lower right corner grip to auto-fit to content
 - TAB/SHIFT+TAB to cycle through keyboard editable fields
 - use mouse wheel to scroll
 - use CTRL+mouse wheel to zoom window contents (if IO.FontAllowScaling is true)
 - CTRL+Click on a slider to input value as text
 - text editor:
   - Hold SHIFT or use mouse to select text.
   - CTRL+Left/Right to word jump
   - CTRL+Shift+Left/Right to select words
   - CTRL+A our Double-Click to select all
   - CTRL+X,CTRL+C,CTRL+V to use OS clipboard
   - CTRL+Z,CTRL+Y to undo/redo
   - ESCAPE to revert text to its original value
   - You can apply arithmetic operators +,*,/ on numerical values. Use +- to subtract (because - would set a negative value!)

 PROGRAMMER GUIDE
 ================

 - your code creates the UI, if your code doesn't run the UI is gone! == dynamic UI, no construction step, less data retention on your side, no state duplication, less sync, less errors.
 - see ImGui::ShowTestWindow() for user-side sample code
 - see examples/ folder for standalone sample applications.

 - getting started:
   - initialisation: call ImGui::GetIO() and fill the 'Settings' data.
   - every frame: 
      1/ in your mainloop or right after you got your keyboard/mouse info, call ImGui::GetIO() and fill the 'Input' data, then call ImGui::NewFrame().
      2/ use any ImGui function you want between NewFrame() and Render()
      3/ ImGui::Render() to render all the accumulated command-lists. it will call your RenderDrawListFn handler that you set in the IO structure.
   - all rendering information are stored into command-lists until ImGui::Render() is called.
   - effectively it means you can create widgets at any time in your code, regardless of "update" vs "render" considerations.
   - refer to the examples applications in the examples/ folder for instruction on how to setup your code.
   - a typical application skeleton may be:

        // Application init
        // TODO: Fill all settings fields of the io structure
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x = 1920.0f;
        io.DisplaySize.y = 1280.0f;
        io.DeltaTime = 1.0f/60.0f;
        io.IniFilename = "imgui.ini";

        // Application main loop
        while (true)
        {
            // 1/ get low-level input
            // e.g. on Win32, GetKeyboardState(), or poll your events, etc.
            
            // 2/ TODO: Fill all 'Input' fields of io structure and call NewFrame
            ImGuiIO& io = ImGui::GetIO();
            io.MousePos = ...
            io.KeysDown[i] = ...
            ImGui::NewFrame();

            // 3/ most of your application code here - you can use any of ImGui::* functions between NewFrame() and Render() calls
            GameUpdate();
            GameRender();

            // 4/ render & swap video buffers
            ImGui::Render();
            // swap video buffer, etc.
        }

 TROUBLESHOOTING & FREQUENTLY ASKED QUESTIONS
 ============================================

 - if text or lines are blurry when integrating ImGui in your engine:
   - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
   - try adjusting ImGui::GetIO().PixelCenterOffset to 0.5f or 0.375f

 - if you can only see text but no solid shapes or lines:
   - make sure io.FontTexUvForWhite is set to the texture coordinates of a pure white pixel in your texture. 
     (this is done for you if you are using the default font)
     (ImGui is using this texture coordinate to draw solid objects so text and solid draw calls can be merged into one.)

 - if you want to use a different font than the default:
   - create bitmap font data using BMFont, make sure that BMFont is exporting the .fnt file in Binary mode.
       io.Font = new ImBitmapFont();
       io.Font->LoadFromFile("path_to_your_fnt_file.fnt");
   - load your texture yourself. texture *MUST* have white pixel at UV coordinate io.FontTexUvForWhite. This is used to draw all solid shapes.
   - the extra_fonts/ folder provides examples of using external fonts.

 - if you are confused about the meaning or use of ID in ImGui:
   - some widgets requires state to be carried over multiple frames (most typically ImGui often wants remember what is the "active" widget).
     to do so they need an unique ID. unique ID are typically derived from a string label, an indice or a pointer.
     when you call Button("OK") the button shows "OK" and also use "OK" as an ID.
   - ID are uniquely scoped within Windows so no conflict can happen if you have two buttons called "OK" in two different Windows.
     within a same Window, use PushID() / PopID() to easily create scopes and avoid ID conflicts. 
     so if you have a loop creating "multiple" items, you can use PushID() / PopID() with the index of each item, or their pointer, etc.
     some functions like TreeNode() implicitly creates a scope for you by calling PushID()
   - when dealing with trees, ID are important because you want to preserve the opened/closed state of tree nodes.
     depending on your use cases you may want to use strings, indices or pointers as ID. experiment and see what makes more sense!
      e.g. When displaying a single object, using a static string as ID will preserve your node open/closed state when the targeted object change
      e.g. When displaying a list of objects, using indices or pointers as ID will preserve the node open/closed state per object
   - when passing a label you can optionally specify extra unique ID information within the same string using "##". This helps solving the simpler collision cases.
      e.g. "Label" display "Label" and uses "Label" as ID
      e.g. "Label##Foobar" display "Label" and uses "Label##Foobar" as ID
      e.g. "##Foobar" display an empty label and uses "##Foobar" as ID
   - read articles about the imgui principles (see web links) to understand the requirement and use of ID.

 - tip: the construct 'if (IMGUI_ONCE_UPON_A_FRAME)' will evaluate to true only once a frame, you can use it to add custom UI in the middle of a deep nested inner loop in your code.
 - tip: you can call Render() multiple times (e.g for VR renders), up to you to communicate the extra state to your RenderDrawListFn function.
 - tip: you can create widgets without a Begin()/End() block, they will go in an implicit window called "Debug"
 - tip: read the ShowTestWindow() code for more example of how to use ImGui!

 API BREAKING CHANGES
 ====================

  - 2014/11/07 (1.15) renamed IsHovered() to IsItemHovered()
  - 2014/10/02 (1.14) renamed IMGUI_INCLUDE_IMGUI_USER_CPP to IMGUI_INCLUDE_IMGUI_USER_INL and imgui_user.cpp to imgui_user.inl (more IDE friendly)
  - 2014/09/25 (1.13) removed 'text_end' parameter from IO.SetClipboardTextFn (the string is now always zero-terminated for simplicity)
  - 2014/09/24 (1.12) renamed SetFontScale() to SetWindowFontScale()
  - 2014/09/24 (1.12) moved IM_MALLOC/IM_REALLOC/IM_FREE preprocessor defines to IO.MemAllocFn/IO.MemReallocFn/IO.MemFreeFn
  - 2014/08/30 (1.09) removed IO.FontHeight (now computed automatically)
  - 2014/08/30 (1.09) moved IMGUI_FONT_TEX_UV_FOR_WHITE preprocessor define to IO.FontTexUvForWhite
  - 2014/08/28 (1.09) changed the behavior of IO.PixelCenterOffset following various rendering fixes

 ISSUES & TODO-LIST
 ==================

 - misc: merge or clarify ImVec4 / ImGuiAabb, they are essentially duplicate containers
 - window: autofit is losing its purpose when user relies on any dynamic layout (window width multiplier, column). maybe just discard autofit?
 - window: add horizontal scroll
 - window: fix resize grip rendering scaling along with Rounding style setting
 - widgets: switching from "widget-label" to "label-widget" would make it more convenient to integrate widgets in trees
 - widgets: clip text? hover clipped text shows it in a tooltip or in-place overlay
 - main: make IsHovered() more consistent for various type of widgets, widgets with multiple components, etc. also effectively IsHovered() region sometimes differs from hot region, e.g tree nodes
 - main: make IsHovered() info stored in a stack? so that 'if TreeNode() { Text; TreePop; } if IsHovered' return the hover state of the TreeNode?
 - scrollbar: use relative mouse movement when first-clicking inside of scroll grab box.
 - scrollbar: make the grab visible and a minimum size for long scroll regions
 - input number: optional range min/max
 - input number: holding [-]/[+] buttons should increase the step non-linearly
 - input number: use mouse wheel to step up/down
 - layout: clean up the InputFloatN/SliderFloatN/ColorEdit4 horrible layout code. item width should include frame padding, then we can have a generic horizontal layout helper.
 - columns: declare column set (each column: fixed size, %, fill, distribute default size among fills)
 - columns: columns header to act as button (~sort op) and allow resize/reorder
 - columns: user specify columns size
 - combo: turn child handling code into popup helper
 - list selection, concept of a selectable "block" (that can be multiple widgets)
 - menubar, menus
 - plot: make it easier for user to draw into the graph (e.g: draw basis, highlight certain points, 2d plots, multiple plots)
 - plot: "smooth" automatic scale, user give an input 0.0(full user scale) 1.0(full derived from value)
 - plot: add a helper e.g. Plot(char* label, float value, float time_span=2.0f) that stores values and Plot them for you - probably another function name. and/or automatically allow to plot ANY displayed value (more reliance on stable ID)
 - file selection widget -> build the tool in our codebase to improve model-dialog idioms (may or not lead to ImGui changes)
 - slider: allow using the [-]/[+] buttons used by InputFloat()/InputInt()
 - slider: initial absolute click is imprecise. change to relative movement slider? hide mouse cursor, allow more precise input using less screen-space.
 - text edit: clean up the horrible mess caused by converting UTF-8 <> wchar
 - text edit: centered text for slider or input text to it matches typical positioning.
 - text edit: flag to disable live update of the user buffer. 
 - text edit: field resize behavior - field could stretch when being edited? hover tooltip shows more text?
 - text edit: pasting text into a number box should filter the characters the same way direct input does
 - text edit: add multi-line text edit
 - settings: write more decent code to allow saving/loading new fields
 - settings: api for per-tool simple persistent data (bool,int,float) in .ini file
 - log: be able to right-click and log a window or tree-node into tty/file/clipboard?
 - filters: set a current filter that tree node can automatically query to hide themselves
 - filters: handle wildcards (with implicit leading/trailing *), regexps
 - shortcuts: add a shortcut api, e.g. parse "&Save" and/or "Save (CTRL+S)", pass in to widgets or provide simple ways to use (button=activate, input=focus)
 ! keyboard: tooltip & combo boxes are messing up / not honoring keyboard tabbing
 - keyboard: full keyboard navigation and focus.
 - input: rework IO to be able to pass actual events to fix temporal aliasing issues.
 - input: support track pad style scrolling & slider edit.
 - tooltip: move to fit within screen (e.g. when mouse cursor is right of the screen).
 - clipboard: automatically transform \n into \n\r or equivalent for higher compability on windows
 - portability: big-endian test/support (github issue #81)
 - misc: provide a way to compile out the entire implementation while providing a dummy API (e.g. #define IMGUI_DUMMY_IMPL
 - misc: not thread-safe
 - misc: double-clicking on title bar to minimize isn't consistent, perhaps move to single-click on left-most collapse icon?
 - style editor: add a button to output C code.
 - optimization/render: use indexed rendering
 - optimization/render: move clip-rect to vertex data? would allow merging all commands
 - optimization/render: merge command-lists with same clip-rect into one even if they aren't sequential? (as long as in-between clip rectangle don't overlap)?
 - optimization/render: font exported by bmfont is not tight fit on vertical axis, incur unneeded pixel-shading cost.
 - optimization: turn some the various stack vectors into statically-sized arrays
 - optimization: better clipping for multi-component widgets
 - optimization: specialize for height based clipping first (assume widgets never go up + height tests before width tests?)
*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#include <ctype.h>      // toupper
#include <math.h>       // sqrtf
#include <stdint.h>     // intptr_t
#include <stdio.h>      // vsnprintf
#include <string.h>     // memset
#include <new>          // new (ptr)

#ifdef _MSC_VER
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning : use of old-style cast                              // yes, they are more terse and not scary looking.
#pragma clang diagnostic ignored "-Wfloat-equal"            // warning : comparing floating point with == or != is unsafe   // storing and comparing against same constants ok.
#pragma clang diagnostic ignored "-Wformat-nonliteral"      // warning : format string is not a string literal              // passing non-literal to vsnformat(). yes, user passing incorrect format strings can crash the code, thank you.
#pragma clang diagnostic ignored "-Wexit-time-destructors"  // warning : declaration requires an exit-time destructor       // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. ImGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wglobal-constructors"    // warning : declaration requires a global destructor           // similar to above, not sure what the exact difference it.
#endif

//-------------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------------

static bool         ButtonBehaviour(const ImGuiAabb& bb, const ImGuiID& id, bool* out_hovered, bool* out_held, bool allow_key_modifiers, bool repeat = false);
static void         LogText(const ImVec2& ref_pos, const char* text, const char* text_end = NULL);

static void         RenderText(ImVec2 pos, const char* text, const char* text_end = NULL, bool hide_text_after_hash = true, float wrap_width = 0.0f);
static void         RenderFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border = true, float rounding = 0.0f);
static void         RenderCollapseTriangle(ImVec2 p_min, bool open, float scale = 1.0f, bool shadow = false);

static void         ItemSize(ImVec2 size, ImVec2* adjust_start_offset = NULL);
static void         ItemSize(const ImGuiAabb& aabb, ImVec2* adjust_start_offset = NULL);
static void         PushColumnClipRect(int column_index = -1);
static bool         IsClipped(const ImGuiAabb& aabb);
static bool         ClipAdvance(const ImGuiAabb& aabb);

static bool         IsMouseHoveringBox(const ImGuiAabb& box);
static bool         IsKeyPressedMap(ImGuiKey key, bool repeat = true);

static bool         CloseWindowButton(bool* open = NULL);
static void         FocusWindow(ImGuiWindow* window);
static ImGuiWindow* FindWindow(const char* name);
static ImGuiWindow* FindHoveredWindow(ImVec2 pos, bool excluding_childs);

//-----------------------------------------------------------------------------
// Platform dependent default implementations
//-----------------------------------------------------------------------------

static const char*  GetClipboardTextFn_DefaultImpl();
static void         SetClipboardTextFn_DefaultImpl(const char* text);

//-----------------------------------------------------------------------------
// User facing structures
//-----------------------------------------------------------------------------

ImGuiStyle::ImGuiStyle()
{
    Alpha                   = 1.0f;             // Global alpha applies to everything in ImGui
    WindowPadding           = ImVec2(8,8);      // Padding within a window
    WindowMinSize           = ImVec2(48,48);    // Minimum window size
    FramePadding            = ImVec2(5,4);      // Padding within a framed rectangle (used by most widgets)
    ItemSpacing             = ImVec2(10,5);     // Horizontal and vertical spacing between widgets/lines
    ItemInnerSpacing        = ImVec2(5,5);      // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)
    TouchExtraPadding       = ImVec2(0,0);      // Expand bounding box for touch-based system where touch position is not accurate enough (unnecessary for mouse inputs). Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget running. So dont grow this too much!
    AutoFitPadding          = ImVec2(8,8);      // Extra space after auto-fit (double-clicking on resize grip)
    WindowFillAlphaDefault  = 0.70f;            // Default alpha of window background, if not specified in ImGui::Begin()
    WindowRounding          = 10.0f;            // Radius of window corners rounding. Set to 0.0f to have rectangular windows
    TreeNodeSpacing         = 22.0f;            // Horizontal spacing when entering a tree node
    ColumnsMinSpacing       = 6.0f;             // Minimum horizontal spacing between two columns
    ScrollBarWidth          = 16.0f;            // Width of the vertical scroll bar

    Colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    Colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    Colors[ImGuiCol_Border]                 = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    Colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
    Colors[ImGuiCol_FrameBg]                = ImVec4(0.80f, 0.80f, 0.80f, 0.30f);   // Background of checkbox, radio button, plot, slider, text input
    Colors[ImGuiCol_TitleBg]                = ImVec4(0.50f, 0.50f, 1.00f, 0.45f);
    Colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    Colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.40f, 0.40f, 0.80f, 0.15f);
    Colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
    Colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
    Colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.80f, 0.50f, 0.50f, 0.40f);
    Colors[ImGuiCol_ComboBg]                = ImVec4(0.20f, 0.20f, 0.20f, 0.99f);
    Colors[ImGuiCol_CheckHovered]           = ImVec4(0.60f, 0.40f, 0.40f, 0.45f);
    Colors[ImGuiCol_CheckActive]            = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    Colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    Colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    Colors[ImGuiCol_Button]                 = ImVec4(0.67f, 0.40f, 0.40f, 0.60f);
    Colors[ImGuiCol_ButtonHovered]          = ImVec4(0.67f, 0.40f, 0.40f, 1.00f);
    Colors[ImGuiCol_ButtonActive]           = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    Colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
    Colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    Colors[ImGuiCol_HeaderActive]           = ImVec4(0.60f, 0.60f, 0.80f, 1.00f);
    Colors[ImGuiCol_Column]                 = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    Colors[ImGuiCol_ColumnHovered]          = ImVec4(0.60f, 0.40f, 0.40f, 1.00f);
    Colors[ImGuiCol_ColumnActive]           = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    Colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    Colors[ImGuiCol_ResizeGripHovered]      = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
    Colors[ImGuiCol_ResizeGripActive]       = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    Colors[ImGuiCol_CloseButton]            = ImVec4(0.50f, 0.50f, 0.90f, 0.50f);
    Colors[ImGuiCol_CloseButtonHovered]     = ImVec4(0.70f, 0.70f, 0.90f, 0.60f);
    Colors[ImGuiCol_CloseButtonActive]      = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    Colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    Colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    Colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    Colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    Colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    Colors[ImGuiCol_TooltipBg]              = ImVec4(0.05f, 0.05f, 0.10f, 0.90f);
}

ImGuiIO::ImGuiIO()
{
    memset(this, 0, sizeof(*this));
    DeltaTime = 1.0f/60.0f;
    IniSavingRate = 5.0f;
    IniFilename = "imgui.ini";
    LogFilename = "imgui_log.txt";
    Font = NULL;
    FontYOffset = 0.0f;
    FontTexUvForWhite = ImVec2(0.0f,0.0f);
    FontBaseScale = 1.0f;
    FontAllowUserScaling = false;
    FontFallbackGlyph = (ImWchar)'?';
    PixelCenterOffset = 0.0f;
    MousePos = ImVec2(-1,-1);
    MousePosPrev = ImVec2(-1,-1);
    MouseDoubleClickTime = 0.30f;
    MouseDoubleClickMaxDist = 6.0f;
    UserData = NULL;

    // User functions
    RenderDrawListsFn = NULL;
    MemAllocFn = malloc;
    MemReallocFn = realloc;
    MemFreeFn = free;
    GetClipboardTextFn = GetClipboardTextFn_DefaultImpl;   // Platform dependant default implementations
    SetClipboardTextFn = SetClipboardTextFn_DefaultImpl;
    ImeSetInputScreenPosFn = NULL;
}

// Pass in translated ASCII characters for text input.
// - with glfw you can get those from the callback set in glfwSetCharCallback()
// - on Windows you can get those using ToAscii+keyboard state, or via the VM_CHAR message
static size_t ImStrlenW(const ImWchar* str);
void ImGuiIO::AddInputCharacter(ImWchar c)
{
    const size_t n = ImStrlenW(InputCharacters);
    if (n + 1 < sizeof(InputCharacters) / sizeof(InputCharacters[0]))
    {
        InputCharacters[n] = c;
        InputCharacters[n+1] = 0;
    }
}

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

#define IM_ARRAYSIZE(_ARR)          ((int)(sizeof(_ARR)/sizeof(*_ARR)))

#undef PI
const float PI = 3.14159265358979323846f;

#ifdef INT_MAX
#define IM_INT_MAX INT_MAX
#else
#define IM_INT_MAX 2147483647
#endif

// Math bits
// We are keeping those static in the .cpp file so as not to leak them outside, in the case the user has implicit cast operators between ImVec2 and its own types.
static inline ImVec2 operator*(const ImVec2& lhs, const float rhs)              { return ImVec2(lhs.x*rhs, lhs.y*rhs); }
//static inline ImVec2 operator/(const ImVec2& lhs, const float rhs)            { return ImVec2(lhs.x/rhs, lhs.y/rhs); }
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x+rhs.x, lhs.y+rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)            { return ImVec2(lhs.x-rhs.x, lhs.y-rhs.y); }
static inline ImVec2 operator*(const ImVec2& lhs, const ImVec2 rhs)             { return ImVec2(lhs.x*rhs.x, lhs.y*rhs.y); }
static inline ImVec2 operator/(const ImVec2& lhs, const ImVec2 rhs)             { return ImVec2(lhs.x/rhs.x, lhs.y/rhs.y); }
static inline ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static inline ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static inline ImVec2& operator*=(ImVec2& lhs, const float rhs)                  { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
//static inline ImVec2& operator/=(ImVec2& lhs, const float rhs)                { lhs.x /= rhs; lhs.y /= rhs; return lhs; }

static inline int    ImMin(int lhs, int rhs)                                    { return lhs < rhs ? lhs : rhs; }
static inline int    ImMax(int lhs, int rhs)                                    { return lhs >= rhs ? lhs : rhs; }
static inline float  ImMin(float lhs, float rhs)                                { return lhs < rhs ? lhs : rhs; }
static inline float  ImMax(float lhs, float rhs)                                { return lhs >= rhs ? lhs : rhs; }
static inline ImVec2 ImMin(const ImVec2& lhs, const ImVec2& rhs)                { return ImVec2(ImMin(lhs.x,rhs.x), ImMin(lhs.y,rhs.y)); }
static inline ImVec2 ImMax(const ImVec2& lhs, const ImVec2& rhs)                { return ImVec2(ImMax(lhs.x,rhs.x), ImMax(lhs.y,rhs.y)); }
static inline float  ImClamp(float f, float mn, float mx)                       { return (f < mn) ? mn : (f > mx) ? mx : f; }
static inline ImVec2 ImClamp(const ImVec2& f, const ImVec2& mn, ImVec2 mx)      { return ImVec2(ImClamp(f.x,mn.x,mx.x), ImClamp(f.y,mn.y,mx.y)); }
static inline float  ImSaturate(float f)                                        { return (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f; }
static inline float  ImLerp(float a, float b, float t)                          { return a + (b - a) * t; }
//static inline ImVec2 ImLerp(const ImVec2& a, const ImVec2& b, float t)        { return a + (b - a) * t; }
static inline ImVec2 ImLerp(const ImVec2& a, const ImVec2& b, const ImVec2& t)  { return ImVec2(a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y); }
static inline float  ImLength(const ImVec2& lhs)                                { return sqrtf(lhs.x*lhs.x + lhs.y*lhs.y); }

static int           ImTextCharToUtf8(char* buf, size_t buf_size, unsigned int in_char);                                // return output UTF-8 bytes count
static ptrdiff_t     ImTextStrToUtf8(char* buf, size_t buf_size, const ImWchar* in_text, const ImWchar* in_text_end);   // return output UTF-8 bytes count
static int           ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end);          // return input UTF-8 bytes count
static ptrdiff_t     ImTextStrFromUtf8(ImWchar* buf, size_t buf_size, const char* in_text, const char* in_text_end);    // return input UTF-8 bytes count
static int           ImTextCountCharsFromUtf8(const char* in_text, const char* in_text_end);                            // return number of UTF-8 code-points (NOT bytes count)
static int           ImTextCountUtf8BytesFromWchar(const ImWchar* in_text, const ImWchar* in_text_end);                 // return number of bytes to express string as UTF-8 code-points

static int ImStricmp(const char* str1, const char* str2)
{
    int d;
    while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; }
    return d;
}

static int ImStrnicmp(const char* str1, const char* str2, int count)
{
    int d;
    while (count > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; count--; }
    return (count == 0) ? 0 : d;
}

static char* ImStrdup(const char *str)
{
    char *buff = (char*)ImGui::MemAlloc(strlen(str) + 1);
    IM_ASSERT(buff);
    strcpy(buff, str);
    return buff;
}

static size_t ImStrlenW(const ImWchar* str)
{
    size_t n = 0;
    while (*str++)
        n++;
    return n;
}

static const char* ImStristr(const char* haystack, const char* needle, const char* needle_end)
{
    if (!needle_end)
        needle_end = needle + strlen(needle);

    const char un0 = (char)toupper(*needle);
    while (*haystack)
    {
        if (toupper(*haystack) == un0)
        {
            const char* b = needle + 1;
            for (const char* a = haystack + 1; b < needle_end; a++, b++)
                if (toupper(*a) != toupper(*b))
                    break;
            if (b == needle_end)
                return haystack;
        }
        haystack++;
    }
    return NULL;
}

static ImU32 crc32(const void* data, size_t data_size, ImU32 seed = 0) 
{ 
    static ImU32 crc32_lut[256] = { 0 };
    if (!crc32_lut[1])
    {
        const ImU32 polynomial = 0xEDB88320;
        for (ImU32 i = 0; i < 256; i++) 
        { 
            ImU32 crc = i; 
            for (ImU32 j = 0; j < 8; j++) 
                crc = (crc >> 1) ^ (ImU32(-int(crc & 1)) & polynomial); 
            crc32_lut[i] = crc; 
        }
    }
    ImU32 crc = ~seed; 
    const unsigned char* current = (const unsigned char*)data; 
    while (data_size--) 
        crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ *current++]; 
    return ~crc; 
} 

static size_t ImFormatString(char* buf, size_t buf_size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buf, buf_size, fmt, args);
    va_end(args);
    buf[buf_size-1] = 0;
    return (w == -1) ? buf_size : (size_t)w;
}

static size_t ImFormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args)
{
    int w = vsnprintf(buf, buf_size, fmt, args);
    buf[buf_size-1] = 0;
    return (w == -1) ? buf_size : (size_t)w;
}

static ImU32 ImConvertColorFloat4ToU32(const ImVec4& in)
{
    ImU32 out  = ((ImU32)(ImSaturate(in.x)*255.f));
    out |= ((ImU32)(ImSaturate(in.y)*255.f) << 8);
    out |= ((ImU32)(ImSaturate(in.z)*255.f) << 16);
    out |= ((ImU32)(ImSaturate(in.w)*255.f) << 24);
    return out;
}

// Convert rgb floats ([0-1],[0-1],[0-1]) to hsv floats ([0-1],[0-1],[0-1]), from Foley & van Dam p592
// Optimized http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
static void ImConvertColorRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v)
{
    float K = 0.f;
    if (g < b)
    {
        const float tmp = g; g = b; b = tmp;
        K = -1.f;
    }
    if (r < g)
    {
        const float tmp = r; r = g; g = tmp;
        K = -2.f / 6.f - K;
    }

    const float chroma = r - (g < b ? g : b);
    out_h = fabsf(K + (g - b) / (6.f * chroma + 1e-20f));
    out_s = chroma / (r + 1e-20f);
    out_v = r;
}

// Convert hsv floats ([0-1],[0-1],[0-1]) to rgb floats ([0-1],[0-1],[0-1]), from Foley & van Dam p593
// also http://en.wikipedia.org/wiki/HSL_and_HSV
static void ImConvertColorHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b)
{   
    if (s == 0.0f)
    {
        // gray
        out_r = out_g = out_b = v;
        return;
    }

    h = fmodf(h, 1.0f) / (60.0f/360.0f);
    int   i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i)
    {
    case 0: out_r = v; out_g = t; out_b = p; break;
    case 1: out_r = q; out_g = v; out_b = p; break;
    case 2: out_r = p; out_g = v; out_b = t; break;
    case 3: out_r = p; out_g = q; out_b = v; break;
    case 4: out_r = t; out_g = p; out_b = v; break;
    case 5: default: out_r = v; out_g = p; out_b = q; break;
    }
}

//-----------------------------------------------------------------------------

struct ImGuiColMod       // Color modifier, backup of modified data so we can restore it
{
    ImGuiCol    Col;
    ImVec4      PreviousValue;
};

struct ImGuiStyleMod    // Style modifier, backup of modified data so we can restore it
{
    ImGuiStyleVar Var;
    ImVec2      PreviousValue;
};

struct ImGuiAabb    // 2D axis aligned bounding-box
{
    ImVec2      Min;
    ImVec2      Max;

    ImGuiAabb()                                         { Min = ImVec2(FLT_MAX,FLT_MAX); Max = ImVec2(-FLT_MAX,-FLT_MAX); }
    ImGuiAabb(const ImVec2& min, const ImVec2& max)     { Min = min; Max = max; }
    ImGuiAabb(const ImVec4& v)                          { Min.x = v.x; Min.y = v.y; Max.x = v.z; Max.y = v.w; }
    ImGuiAabb(float x1, float y1, float x2, float y2)   { Min.x = x1; Min.y = y1; Max.x = x2; Max.y = y2; }

    ImVec2      GetCenter() const                       { return Min + (Max-Min)*0.5f; }
    ImVec2      GetSize() const                         { return Max-Min; }
    float       GetWidth() const                        { return (Max-Min).x; }
    float       GetHeight() const                       { return (Max-Min).y; }
    ImVec2      GetTL() const                           { return Min; }
    ImVec2      GetTR() const                           { return ImVec2(Max.x,Min.y); }
    ImVec2      GetBL() const                           { return ImVec2(Min.x,Max.y); }
    ImVec2      GetBR() const                           { return Max; }
    bool        Contains(ImVec2 p) const                { return p.x >= Min.x && p.y >= Min.y && p.x <= Max.x && p.y <= Max.y; }
    bool        Contains(const ImGuiAabb& r) const      { return r.Min.x >= Min.x && r.Min.y >= Min.y && r.Max.x <= Max.x && r.Max.y <= Max.y; }
    bool        Overlaps(const ImGuiAabb& r) const      { return r.Min.y <= Max.y && r.Max.y >= Min.y && r.Min.x <= Max.x && r.Max.x >= Min.x; }
    void        Expand(ImVec2 sz)                       { Min -= sz; Max += sz; }
    void        Clip(const ImGuiAabb& clip)             { Min.x = ImMax(Min.x, clip.Min.x); Min.y = ImMax(Min.y, clip.Min.y); Max.x = ImMin(Max.x, clip.Max.x); Max.y = ImMin(Max.y, clip.Max.y); }
};

// Temporary per-window data, reset at the beginning of the frame
struct ImGuiDrawContext
{
    ImVec2                  CursorPos;
    ImVec2                  CursorPosPrevLine;
    ImVec2                  CursorStartPos;
    float                   CurrentLineHeight;
    float                   PrevLineHeight;
    float                   LogLineHeight;
    int                     TreeDepth;
    ImGuiAabb               LastItemAabb;
    bool                    LastItemHovered;
    bool                    LastItemFocused;
    ImVector<ImGuiWindow*>  ChildWindows;
    ImVector<bool>          AllowKeyboardFocus;
    ImVector<float>         ItemWidth;
    ImVector<float>         TextWrapPos;
    ImGuiColorEditMode      ColorEditMode;
    ImGuiStorage*           StateStorage;
    int                     OpenNextNode;

    float                   ColumnsStartX;       // Start position from left of window (increased by TreePush/TreePop, etc.)
    float                   ColumnsOffsetX;      // Offset to the current column (if ColumnsCurrent > 0). FIXME: This and the above should be a stack to allow use cases like Tree->Column->Tree. Need revamp columns API.
    int                     ColumnsCurrent;
    int                     ColumnsCount;
    ImVec2                  ColumnsStartPos;
    float                   ColumnsCellMinY;
    float                   ColumnsCellMaxY;
    bool                    ColumnsShowBorders;
    ImGuiID                 ColumnsSetID;

    ImGuiDrawContext()
    {
        CursorPos = CursorPosPrevLine = CursorStartPos = ImVec2(0.0f, 0.0f);
        CurrentLineHeight = PrevLineHeight = 0.0f;
        LogLineHeight = -1.0f;
        TreeDepth = 0;
        LastItemAabb = ImGuiAabb(0.0f,0.0f,0.0f,0.0f);
        LastItemHovered = false;
        LastItemFocused = true;
        StateStorage = NULL;
        OpenNextNode = -1;

        ColumnsStartX = 0.0f;
        ColumnsOffsetX = 0.0f;
        ColumnsCurrent = 0;
        ColumnsCount = 1;
        ColumnsStartPos = ImVec2(0.0f, 0.0f);
        ColumnsCellMinY = ColumnsCellMaxY = 0.0f;
        ColumnsShowBorders = true;
        ColumnsSetID = 0;
    }
};

struct ImGuiTextEditState;
#define STB_TEXTEDIT_STRING ImGuiTextEditState
#define STB_TEXTEDIT_CHARTYPE ImWchar
#include "stb_textedit.h"

// Internal state of the currently focused/edited text input box
struct ImGuiTextEditState
{
    ImWchar             Text[1024];                     // edit buffer, we need to persist but can't guarantee the persistence of the user-provided buffer. so we copy into own buffer.
    char                InitialText[1024*3+1];          // backup of end-user buffer at the time of focus (in UTF-8, unconverted)
    size_t              BufSize;                        // end-user buffer size, <= 1024 (or increase above)
    float               Width;                          // widget width
    float               ScrollX;
    STB_TexteditState   StbState;
    float               CursorAnim;
    ImVec2              LastCursorPos;                  // Cursor position in screen space to be used by IME callback.
    bool                SelectedAllMouseLock;
    ImFont              Font;
    float               FontSize;

    ImGuiTextEditState()                                { memset(this, 0, sizeof(*this)); }

    void                CursorAnimReset()               { CursorAnim = -0.30f; }                                                // After a user-input the cursor stays on for a while without blinking
    bool                CursorIsVisible() const         { return CursorAnim <= 0.0f || fmodf(CursorAnim, 1.20f) <= 0.80f; }     // Blinking
    bool                HasSelection() const            { return StbState.select_start != StbState.select_end; }
    void                SelectAll()                     { StbState.select_start = 0; StbState.select_end = (int)ImStrlenW(Text); StbState.cursor = StbState.select_end; StbState.has_preferred_x = false; }

    void                OnKeyPressed(int key);
    void                UpdateScrollOffset();
    ImVec2              CalcDisplayOffsetFromCharIdx(int i) const;

    // Static functions because they are used to render non-focused instances of a text input box
    static const char*      GetTextPointerClippedA(ImFont font, float font_size, const char* text, float width, ImVec2* out_text_size = NULL);
    static const ImWchar*   GetTextPointerClippedW(ImFont font, float font_size, const ImWchar* text, float width, ImVec2* out_text_size = NULL);
    static void             RenderTextScrolledClipped(ImFont font, float font_size, const char* text, ImVec2 pos_base, float width, float scroll_x);
};

struct ImGuiIniData
{
    char*   Name;
    ImVec2  Pos;
    ImVec2  Size;
    bool    Collapsed;

    ImGuiIniData() { memset(this, 0, sizeof(*this)); }
    ~ImGuiIniData() { if (Name) { ImGui::MemFree(Name); Name = NULL; } }
};

struct ImGuiState
{
    bool                    Initialized;
    ImGuiIO                 IO;
    ImGuiStyle              Style;
    float                   FontSize;                           // == IO.FontBaseScale * IO.Font->GetFontSize(). Vertical distance between two lines of text, aka == CalcTextSize(" ").y

    float                   Time;
    int                     FrameCount;
    int                     FrameCountRendered;
    ImVector<ImGuiWindow*>  Windows;
    ImGuiWindow*            CurrentWindow;                      // Being drawn into
    ImVector<ImGuiWindow*>  CurrentWindowStack;
    ImGuiWindow*            FocusedWindow;                      // Will catch keyboard inputs
    ImGuiWindow*            HoveredWindow;                      // Will catch mouse inputs
    ImGuiWindow*            HoveredWindowExcludingChilds;       // Will catch mouse inputs (for focus/move only)
    ImGuiID                 HoveredId;
    ImGuiID                 ActiveId;
    ImGuiID                 ActiveIdPreviousFrame;
    bool                    ActiveIdIsAlive;
    float                   SettingsDirtyTimer;
    ImVector<ImGuiIniData*> Settings;
    ImVec2                  NewWindowDefaultPos;
    ImVector<ImGuiColMod>   ColorModifiers;
    ImVector<ImGuiStyleMod> StyleModifiers;

    // Render
    ImVector<ImDrawList*>   RenderDrawLists;

    // Widget state
    ImGuiTextEditState      InputTextState;
    ImGuiID                 SliderAsInputTextId;
    ImGuiStorage            ColorEditModeStorage;               // for user selection
    ImGuiID                 ActiveComboID;
    char                    Tooltip[1024];
    char*                   PrivateClipboard;                   // if no custom clipboard handler is defined

    // Logging
    bool                    LogEnabled;
    FILE*                   LogFile;
    ImGuiTextBuffer*        LogClipboard;                       // pointer so our GImGui static constructor doesn't call heap allocators.
    int                     LogAutoExpandMaxDepth;

    ImGuiState()
    {
        Initialized = false;
        Time = 0.0f;
        FrameCount = 0;
        FrameCountRendered = -1;
        CurrentWindow = NULL;
        FocusedWindow = NULL;
        HoveredWindow = NULL;
        HoveredWindowExcludingChilds = NULL;
        ActiveIdIsAlive = false;
        SettingsDirtyTimer = 0.0f;
        NewWindowDefaultPos = ImVec2(60, 60);
        SliderAsInputTextId = 0;
        ActiveComboID = 0;
        memset(Tooltip, 0, sizeof(Tooltip));
        PrivateClipboard = NULL;
        LogEnabled = false;
        LogFile = NULL;
        LogAutoExpandMaxDepth = 2;
        LogClipboard = NULL;
    }
};

static ImGuiState   GImGui;

struct ImGuiWindow
{
    char*                   Name;
    ImGuiID                 ID;
    ImGuiWindowFlags        Flags;
    ImVec2                  PosFloat;
    ImVec2                  Pos;                                // Position rounded-up to nearest pixel
    ImVec2                  Size;                               // Current size (==SizeFull or collapsed title bar size)
    ImVec2                  SizeFull;                           // Size when non collapsed
    ImVec2                  SizeContentsFit;                    // Size of contents (extents reach by the drawing cursor) - may not fit within Size.
    float                   ScrollY;
    float                   NextScrollY;
    bool                    ScrollbarY;
    bool                    Visible;                            // Set to true on Begin()
    bool                    Accessed;                           // Set to true when any widget access the current window
    bool                    Collapsed;                          // Set when collapsing window to become only title-bar
    bool                    SkipItems;                          // == Visible && !Collapsed
    int                     AutoFitFrames;
    bool                    AutoFitOnlyGrows;

    ImGuiDrawContext        DC;
    ImVector<ImGuiID>       IDStack;
    ImVector<ImVec4>        ClipRectStack;
    int                     LastFrameDrawn;
    float                   ItemWidthDefault;
    ImGuiStorage            StateStorage;
    float                   FontWindowScale;                    // Scale multipler per-window
    ImDrawList*             DrawList;

    // Focus
    int                     FocusIdxAllCounter;                 // Start at -1 and increase as assigned via FocusItemRegister()
    int                     FocusIdxTabCounter;                 // (same, but only count widgets which you can Tab through)
    int                     FocusIdxAllRequestCurrent;          // Item being requested for focus
    int                     FocusIdxTabRequestCurrent;          // Tab-able item being requested for focus
    int                     FocusIdxAllRequestNext;             // Item being requested for focus, for next update (relies on layout to be stable between the frame pressing TAB and the next frame)
    int                     FocusIdxTabRequestNext;             // "

public:
    ImGuiWindow(const char* name, ImVec2 default_pos, ImVec2 default_size);
    ~ImGuiWindow();

    ImGuiID     GetID(const char* str);
    ImGuiID     GetID(const void* ptr);

    void        AddToRenderList();
    bool        FocusItemRegister(bool is_active, bool tab_stop = true);      // Return true if focus is requested
    void        FocusItemUnregister();

    ImGuiAabb   Aabb() const                            { return ImGuiAabb(Pos, Pos+Size); }
    ImFont      Font() const                            { return GImGui.IO.Font; }
    float       FontSize() const                        { return GImGui.FontSize * FontWindowScale; }
    ImVec2      CursorPos() const                       { return DC.CursorPos; }
    float       TitleBarHeight() const                  { return (Flags & ImGuiWindowFlags_NoTitleBar) ? 0 : FontSize() + GImGui.Style.FramePadding.y * 2.0f; }
    ImGuiAabb   TitleBarAabb() const                    { return ImGuiAabb(Pos, Pos + ImVec2(SizeFull.x, TitleBarHeight())); }
    ImVec2      WindowPadding() const                   { return ((Flags & ImGuiWindowFlags_ChildWindow) && !(Flags & ImGuiWindowFlags_ShowBorders)) ? ImVec2(1,1) : GImGui.Style.WindowPadding; }
    ImU32       Color(ImGuiCol idx, float a=1.f) const  { ImVec4 c = GImGui.Style.Colors[idx]; c.w *= GImGui.Style.Alpha * a; return ImConvertColorFloat4ToU32(c); }
    ImU32       Color(const ImVec4& col) const          { ImVec4 c = col; c.w *= GImGui.Style.Alpha; return ImConvertColorFloat4ToU32(c); }
};

static ImGuiWindow* GetCurrentWindow()
{
    IM_ASSERT(GImGui.CurrentWindow != NULL);    // ImGui::NewFrame() hasn't been called yet?
    GImGui.CurrentWindow->Accessed = true;
    return GImGui.CurrentWindow;
}

static void RegisterAliveId(const ImGuiID& id)
{
    if (GImGui.ActiveId == id)
        GImGui.ActiveIdIsAlive = true;
}

//-----------------------------------------------------------------------------

// Helper: Key->value storage
void ImGuiStorage::Clear()
{
    Data.clear();
}

// std::lower_bound but without the bullshit
static ImVector<ImGuiStorage::Pair>::iterator LowerBound(ImVector<ImGuiStorage::Pair>& data, ImU32 key)
{
    ImVector<ImGuiStorage::Pair>::iterator first = data.begin();
    ImVector<ImGuiStorage::Pair>::iterator last = data.end();
    int count = (int)(last - first);
    while (count > 0)
    {
        int count2 = count / 2;
        ImVector<ImGuiStorage::Pair>::iterator mid = first + count2;
        if (mid->key < key)
        {
            first = ++mid;
            count -= count2 + 1;
        }
        else
        {
            count = count2;
        }
    }
    return first;
}

int* ImGuiStorage::Find(ImU32 key)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end())
        return NULL;
    if (it->key != key)
        return NULL;
    return &it->val;
}

int ImGuiStorage::GetInt(ImU32 key, int default_val)
{
    int* pval = Find(key);
    if (!pval)
        return default_val;
    return *pval;
}

// FIXME-OPT: We are wasting time because all SetInt() are preceeded by GetInt() calls so we should have the result from lower_bound already in place.
// However we only use SetInt() on explicit user action (so that's maximum once a frame) so the optimisation isn't much needed.
void ImGuiStorage::SetInt(ImU32 key, int val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it != Data.end() && it->key == key)
    {
        it->val = val;
    }
    else
    {
        Pair pair_key;
        pair_key.key = key;
        pair_key.val = val;
        Data.insert(it, pair_key);
    }
}

void ImGuiStorage::SetAllInt(int v)
{
    for (size_t i = 0; i < Data.size(); i++)
        Data[i].val = v;
}

//-----------------------------------------------------------------------------

// Helper: Parse and apply text filters. In format "aaaaa[,bbbb][,ccccc]"
ImGuiTextFilter::ImGuiTextFilter()
{
    InputBuf[0] = 0;
    CountGrep = 0;
}

void ImGuiTextFilter::Draw(const char* label, float width)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (width < 0.0f)
    {
        ImVec2 label_size = ImGui::CalcTextSize(label, NULL);
        width = ImMax(window->Pos.x + ImGui::GetContentRegionMax().x - window->DC.CursorPos.x - (label_size.x + GImGui.Style.ItemSpacing.x*4), 10.0f);
    }
    ImGui::PushItemWidth(width);
    ImGui::InputText(label, InputBuf, IM_ARRAYSIZE(InputBuf));
    ImGui::PopItemWidth();
    Build();
}

void ImGuiTextFilter::TextRange::split(char separator, ImVector<TextRange>& out)
{
    out.resize(0);
    const char* wb = b;
    const char* we = wb;
    while (we < e)
    {
        if (*we == separator)
        {
            out.push_back(TextRange(wb, we));
            wb = we + 1;
        }
        we++;
    }
    if (wb != we)
        out.push_back(TextRange(wb, we));
}

void ImGuiTextFilter::Build()
{
    Filters.resize(0);
    TextRange input_range(InputBuf, InputBuf+strlen(InputBuf));
    input_range.split(',', Filters);

    CountGrep = 0;
    for (size_t i = 0; i != Filters.size(); i++)
    {
        Filters[i].trim_blanks();
        if (Filters[i].empty())
            continue;
        if (Filters[i].front() != '-')
            CountGrep += 1;
    }
}

bool ImGuiTextFilter::PassFilter(const char* val) const
{
    if (Filters.empty())
        return true;

    if (val == NULL)
        val = "";

    for (size_t i = 0; i != Filters.size(); i++)
    {
        const TextRange& f = Filters[i];
        if (f.empty())
            continue;
        if (f.front() == '-')
        {
            // Subtract
            if (ImStristr(val, f.begin()+1, f.end()) != NULL)
                return false;
        }
        else
        {
            // Grep
            if (ImStristr(val, f.begin(), f.end()) != NULL)
                return true;
        }
    }

    // Implicit * grep
    if (CountGrep == 0)
        return true;

    return false;
}

//-----------------------------------------------------------------------------

// Helper: Text buffer for logging/accumulating text
void ImGuiTextBuffer::append(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len <= 0)
        return;

    const size_t write_off = Buf.size();
    if (write_off + (size_t)len >= Buf.capacity())
        Buf.reserve(Buf.capacity() * 2);

    Buf.resize(write_off + (size_t)len);

    va_start(args, fmt);
    ImFormatStringV(&Buf[write_off] - 1, (size_t)len+1, fmt, args);
    va_end(args);
}

//-----------------------------------------------------------------------------

ImGuiWindow::ImGuiWindow(const char* name, ImVec2 default_pos, ImVec2 default_size)
{
    Name = ImStrdup(name);
    ID = GetID(name); 
    IDStack.push_back(ID);

    PosFloat = default_pos;
    Pos = ImVec2((float)(int)PosFloat.x, (float)(int)PosFloat.y);
    Size = SizeFull = default_size;
    SizeContentsFit = ImVec2(0.0f, 0.0f);
    ScrollY = 0.0f;
    NextScrollY = 0.0f;
    ScrollbarY = false;
    Visible = false;
    Accessed = false;
    Collapsed = false;
    SkipItems = false;
    AutoFitFrames = -1;
    AutoFitOnlyGrows = false;
    LastFrameDrawn = -1;
    ItemWidthDefault = 0.0f;
    FontWindowScale = 1.0f;

    if (ImLength(Size) < 0.001f)
    {
        AutoFitFrames = 2;
        AutoFitOnlyGrows = true;
    }

    DrawList = (ImDrawList*)ImGui::MemAlloc(sizeof(ImDrawList));
    new(DrawList) ImDrawList();

    FocusIdxAllCounter = FocusIdxTabCounter = -1;
    FocusIdxAllRequestCurrent = FocusIdxTabRequestCurrent = IM_INT_MAX;
    FocusIdxAllRequestNext = FocusIdxTabRequestNext = IM_INT_MAX;
}

ImGuiWindow::~ImGuiWindow()
{
    DrawList->~ImDrawList();
    ImGui::MemFree(DrawList);
    DrawList = NULL;
    ImGui::MemFree(Name);
    Name = NULL;
}

ImGuiID ImGuiWindow::GetID(const char* str)
{
    const ImGuiID seed = IDStack.empty() ? 0 : IDStack.back();
    const ImGuiID id = crc32(str, strlen(str), seed); // FIXME-OPT: crc32 function/variant should handle zero-terminated strings
    RegisterAliveId(id);
    return id;
}

ImGuiID ImGuiWindow::GetID(const void* ptr)
{
    const ImGuiID seed = IDStack.empty() ? 0 : IDStack.back();
    const ImGuiID id = crc32(&ptr, sizeof(void*), seed);
    RegisterAliveId(id);
    return id;
}

bool ImGuiWindow::FocusItemRegister(bool is_active, bool tab_stop)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    const bool allow_keyboard_focus = window->DC.AllowKeyboardFocus.back();
    FocusIdxAllCounter++;
    if (allow_keyboard_focus)
        FocusIdxTabCounter++;

    if (is_active)
        window->DC.LastItemFocused = true;

    // Process keyboard input at this point: TAB, Shift-TAB switch focus
    // We can always TAB out of a widget that doesn't allow tabbing in.
    if (tab_stop && FocusIdxAllRequestNext == IM_INT_MAX && FocusIdxTabRequestNext == IM_INT_MAX && is_active && IsKeyPressedMap(ImGuiKey_Tab))
    {
        // Modulo on index will be applied at the end of frame once we've got the total counter of items.
        FocusIdxTabRequestNext = FocusIdxTabCounter + (g.IO.KeyShift ? (allow_keyboard_focus ? -1 : 0) : +1);
    }

    if (FocusIdxAllCounter == FocusIdxAllRequestCurrent)
        return true;

    if (allow_keyboard_focus)
        if (FocusIdxTabCounter == FocusIdxTabRequestCurrent)
            return true;

    return false;
}

void ImGuiWindow::FocusItemUnregister()
{
    FocusIdxAllCounter--;
    FocusIdxTabCounter--;
}

void ImGuiWindow::AddToRenderList()
{
    ImGuiState& g = GImGui;

    if (!DrawList->commands.empty() && !DrawList->vtx_buffer.empty())
    {
        if (DrawList->commands.back().vtx_count == 0)
            DrawList->commands.pop_back();
        g.RenderDrawLists.push_back(DrawList);
    }
    for (size_t i = 0; i < DC.ChildWindows.size(); i++)
    {
        ImGuiWindow* child = DC.ChildWindows[i];
        if (child->Visible)                 // clipped childs may have been marked not Visible
            child->AddToRenderList();
    }
}

//-----------------------------------------------------------------------------

void* ImGui::MemAlloc(size_t sz)
{
    return GImGui.IO.MemAllocFn(sz);
}

void ImGui::MemFree(void* ptr)
{
    return GImGui.IO.MemFreeFn(ptr);
}

void* ImGui::MemRealloc(void* ptr, size_t sz)
{
    return GImGui.IO.MemReallocFn(ptr, sz);
}
    
static ImGuiIniData* FindWindowSettings(const char* name)
{
    ImGuiState& g = GImGui;

    for (size_t i = 0; i != g.Settings.size(); i++)
    {
        ImGuiIniData* ini = g.Settings[i];
        if (ImStricmp(ini->Name, name) == 0)
            return ini;
    }
    ImGuiIniData* ini = (ImGuiIniData*)ImGui::MemAlloc(sizeof(ImGuiIniData));
    new(ini) ImGuiIniData();
    ini->Name = ImStrdup(name);
    ini->Collapsed = false;
    ini->Pos = ImVec2(FLT_MAX,FLT_MAX);
    ini->Size = ImVec2(0,0);
    g.Settings.push_back(ini);
    return ini;
}

// Zero-tolerance, poor-man .ini parsing
// FIXME: Write something less rubbish
static void LoadSettings()
{
    ImGuiState& g = GImGui;
    const char* filename = g.IO.IniFilename;
    if (!filename)
        return;

    // Load file
    FILE* f;
    if ((f = fopen(filename, "rt")) == NULL)
        return;
    if (fseek(f, 0, SEEK_END)) 
    {
       fclose(f); 
       return; 
    }
    const long f_size_signed = ftell(f);
    if (f_size_signed == -1) 
    {
       fclose(f); 
       return; 
    }
    size_t f_size = (size_t)f_size_signed;
    if (fseek(f, 0, SEEK_SET)) 
    {
       fclose(f); 
       return; 
    }
    char* f_data = (char*)ImGui::MemAlloc(f_size+1);
    f_size = fread(f_data, 1, f_size, f); // Text conversion alter read size so let's not be fussy about return value
    fclose(f);
    if (f_size == 0)
    {
        ImGui::MemFree(f_data);
        return;
    }
    f_data[f_size] = 0;

    ImGuiIniData* settings = NULL;
    const char* buf_end = f_data + f_size;
    for (const char* line_start = f_data; line_start < buf_end; )
    {
        const char* line_end = line_start;
        while (line_end < buf_end && *line_end != '\n' && *line_end != '\r')
            line_end++;
        
        if (line_start[0] == '[' && line_end > line_start && line_end[-1] == ']')
        {
            char name[64];
            ImFormatString(name, IM_ARRAYSIZE(name), "%.*s", line_end-line_start-2, line_start+1);
            settings = FindWindowSettings(name);
        }
        else if (settings)
        {
            float x, y;
            int i;
            if (sscanf(line_start, "Pos=%f,%f", &x, &y) == 2)
                settings->Pos = ImVec2(x, y);
            else if (sscanf(line_start, "Size=%f,%f", &x, &y) == 2)
                settings->Size = ImMax(ImVec2(x, y), g.Style.WindowMinSize);
            else if (sscanf(line_start, "Collapsed=%d", &i) == 1)
                settings->Collapsed = (i != 0);
        }

        line_start = line_end+1;
    }

    ImGui::MemFree(f_data);
}

static void SaveSettings()
{
    ImGuiState& g = GImGui;
    const char* filename = g.IO.IniFilename;
    if (!filename)
        return;

    // Gather data from windows that were active during this session
    for (size_t i = 0; i != g.Windows.size(); i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip))
            continue;
        ImGuiIniData* settings = FindWindowSettings(window->Name);
        settings->Pos = window->Pos;
        settings->Size = window->SizeFull;
        settings->Collapsed = window->Collapsed;
    }

    // Write .ini file
    // If a window wasn't opened in this session we preserve its settings
    FILE* f = fopen(filename, "wt");
    if (!f)
        return;
    for (size_t i = 0; i != g.Settings.size(); i++)
    {
        const ImGuiIniData* settings = g.Settings[i];
        if (settings->Pos.x == FLT_MAX)
            continue;
        fprintf(f, "[%s]\n", settings->Name);
        fprintf(f, "Pos=%d,%d\n", (int)settings->Pos.x, (int)settings->Pos.y);
        fprintf(f, "Size=%d,%d\n", (int)settings->Size.x, (int)settings->Size.y);
        fprintf(f, "Collapsed=%d\n", settings->Collapsed);
        fprintf(f, "\n");
    }

    fclose(f);
}

static void MarkSettingsDirty()
{
    ImGuiState& g = GImGui;

    if (g.SettingsDirtyTimer <= 0.0f)
        g.SettingsDirtyTimer = g.IO.IniSavingRate;
}

ImGuiIO& ImGui::GetIO()
{
    return GImGui.IO;
}

ImGuiStyle& ImGui::GetStyle()
{
    return GImGui.Style;
}

void ImGui::NewFrame()
{
    ImGuiState& g = GImGui;

    // Check user inputs
    IM_ASSERT(g.IO.DeltaTime > 0.0f);
    IM_ASSERT(g.IO.DisplaySize.x > 0.0f && g.IO.DisplaySize.y > 0.0f);
    IM_ASSERT(g.IO.RenderDrawListsFn != NULL);  // Must be implemented
    IM_ASSERT(g.IO.FontBaseScale > 0.0f);

    if (!g.Initialized)
    {
        // Initialize on first frame
        g.LogClipboard = (ImGuiTextBuffer*)ImGui::MemAlloc(sizeof(ImGuiTextBuffer));
        new(g.LogClipboard) ImGuiTextBuffer();

        IM_ASSERT(g.Settings.empty());
        LoadSettings();
        if (!g.IO.Font)
        {
            // Default font
            const void* fnt_data;
            unsigned int fnt_size;
            ImGui::GetDefaultFontData(&fnt_data, &fnt_size, NULL, NULL);
            g.IO.Font = (ImBitmapFont*)ImGui::MemAlloc(sizeof(ImBitmapFont));
            new(g.IO.Font) ImBitmapFont();
            g.IO.Font->LoadFromMemory(fnt_data, fnt_size);
            IM_ASSERT(g.IO.Font->IsLoaded());       // Font failed to load
            g.IO.FontYOffset = +1;
        }
        g.Initialized = true;
    }
    IM_ASSERT(g.IO.Font && g.IO.Font->IsLoaded());  // Font not loaded

    g.Time += g.IO.DeltaTime;
    g.FrameCount += 1;
    g.Tooltip[0] = '\0';
    g.FontSize = g.IO.FontBaseScale * g.IO.Font->GetFontSize();

    // Update inputs state
    if (g.IO.MousePos.x < 0 && g.IO.MousePos.y < 0)
        g.IO.MousePos = ImVec2(-9999.0f, -9999.0f);
    if ((g.IO.MousePos.x < 0 && g.IO.MousePos.y < 0) || (g.IO.MousePosPrev.x < 0 && g.IO.MousePosPrev.y < 0))   // if mouse just appeared or disappeared (negative coordinate) we cancel out movement in MouseDelta
        g.IO.MouseDelta = ImVec2(0.0f, 0.0f);
    else
        g.IO.MouseDelta = g.IO.MousePos - g.IO.MousePosPrev;
    g.IO.MousePosPrev = g.IO.MousePos;
    for (size_t i = 0; i < IM_ARRAYSIZE(g.IO.MouseDown); i++)
    {
        g.IO.MouseDownTime[i] = g.IO.MouseDown[i] ? (g.IO.MouseDownTime[i] < 0.0f ? 0.0f : g.IO.MouseDownTime[i] + g.IO.DeltaTime) : -1.0f;
        g.IO.MouseClicked[i] = (g.IO.MouseDownTime[i] == 0.0f);
        g.IO.MouseDoubleClicked[i] = false;
        if (g.IO.MouseClicked[i])
        {
            if (g.Time - g.IO.MouseClickedTime[i] < g.IO.MouseDoubleClickTime)
            {
                if (ImLength(g.IO.MousePos - g.IO.MouseClickedPos[i]) < g.IO.MouseDoubleClickMaxDist)
                    g.IO.MouseDoubleClicked[i] = true;
                g.IO.MouseClickedTime[i] = -FLT_MAX;    // so the third click isn't turned into a double-click
            }
            else
            {
                g.IO.MouseClickedTime[i] = g.Time;
                g.IO.MouseClickedPos[i] = g.IO.MousePos;
            }
        }
    }
    for (size_t i = 0; i < IM_ARRAYSIZE(g.IO.KeysDown); i++)
        g.IO.KeysDownTime[i] = g.IO.KeysDown[i] ? (g.IO.KeysDownTime[i] < 0.0f ? 0.0f : g.IO.KeysDownTime[i] + g.IO.DeltaTime) : -1.0f;

    // Clear reference to active widget if the widget isn't alive anymore
    g.HoveredId = 0;
    if (!g.ActiveIdIsAlive && g.ActiveIdPreviousFrame == g.ActiveId && g.ActiveId != 0)
        g.ActiveId = 0;
    g.ActiveIdPreviousFrame = g.ActiveId;
    g.ActiveIdIsAlive = false;

    // Delay saving settings so we don't spam disk too much
    if (g.SettingsDirtyTimer > 0.0f)
    {
        g.SettingsDirtyTimer -= g.IO.DeltaTime;
        if (g.SettingsDirtyTimer <= 0.0f)
            SaveSettings();
    }

    g.HoveredWindow = FindHoveredWindow(g.IO.MousePos, false);
    g.HoveredWindowExcludingChilds = FindHoveredWindow(g.IO.MousePos, true);

    // Are we using inputs? Tell user so they can capture/discard them.
    g.IO.WantCaptureMouse = (g.HoveredWindow != NULL) || (g.ActiveId != 0);
    g.IO.WantCaptureKeyboard = (g.ActiveId != 0);

    // Scale & Scrolling
    if (g.HoveredWindow && g.IO.MouseWheel != 0)
    {
        ImGuiWindow* window = g.HoveredWindow;
        const int mouse_wheel_dir = g.IO.MouseWheel > 0 ? +1 : -1;
        if (g.IO.KeyCtrl)
        {
            if (g.IO.FontAllowUserScaling)
            {
                // Zoom / Scale window
                float new_font_scale = ImClamp(window->FontWindowScale + mouse_wheel_dir * 0.10f, 0.50f, 2.50f);
                float scale = new_font_scale / window->FontWindowScale;
                window->FontWindowScale = new_font_scale;

                const ImVec2 offset = window->Size * (1.0f - scale) * (g.IO.MousePos - window->Pos) / window->Size;
                window->Pos += offset;
                window->PosFloat += offset;
                window->Size *= scale;
                window->SizeFull *= scale;
            }
        }
        else
        {
            // Scroll
            const int scroll_lines = (window->Flags & ImGuiWindowFlags_ComboBox) ? 3 : 5;
            window->NextScrollY -= mouse_wheel_dir * window->FontSize() * scroll_lines;
        }
    }

    // Pressing TAB activate widget focus
    // NB: Don't discard FocusedWindow if it isn't active, so that a window that go on/off programatically won't lose its keyboard focus.
    if (g.ActiveId == 0 && g.FocusedWindow != NULL && g.FocusedWindow->Visible && IsKeyPressedMap(ImGuiKey_Tab, false))
    {
        g.FocusedWindow->FocusIdxTabRequestNext = 0;
    }

    // Mark all windows as not visible
    for (size_t i = 0; i != g.Windows.size(); i++)
    {
        ImGuiWindow* window = g.Windows[i];
        window->Visible = false;
        window->Accessed = false;
    }

    // No window should be open at the beginning of the frame.
    // But in order to allow the user to call NewFrame() multiple times without calling Render(), we are doing an explicit clear.
    g.CurrentWindowStack.clear();

    // Create implicit window - we will only render it if the user has added something to it.
    ImGui::Begin("Debug", NULL, ImVec2(400,400));
}

// NB: behaviour of ImGui after Shutdown() is not tested/guaranteed at the moment. This function is merely here to free heap allocations.
void ImGui::Shutdown()
{
    ImGuiState& g = GImGui;
    if (!g.Initialized)
        return;

    SaveSettings();

    for (size_t i = 0; i < g.Windows.size(); i++)
    {
        g.Windows[i]->~ImGuiWindow();
        ImGui::MemFree(g.Windows[i]);
    }
    g.Windows.clear();
    g.CurrentWindowStack.clear();
    g.RenderDrawLists.clear();
    g.FocusedWindow = NULL;
    g.HoveredWindow = NULL;
    g.HoveredWindowExcludingChilds = NULL;
    for (size_t i = 0; i < g.Settings.size(); i++)
    {
        g.Settings[i]->~ImGuiIniData();
        ImGui::MemFree(g.Settings[i]);
    }
    g.Settings.clear();
    g.ColorEditModeStorage.Clear();
    if (g.LogFile && g.LogFile != stdout)
    {
        fclose(g.LogFile);
        g.LogFile = NULL;
    }
    if (g.IO.Font)
    {
        g.IO.Font->~ImBitmapFont();
        ImGui::MemFree(g.IO.Font);
        g.IO.Font = NULL;
    }

    if (g.PrivateClipboard)
    {
        ImGui::MemFree(g.PrivateClipboard);
        g.PrivateClipboard = NULL;
    }

    if (g.LogClipboard)
    {
        g.LogClipboard->~ImGuiTextBuffer();
        ImGui::MemFree(g.LogClipboard);
    }

    g.Initialized = false;
}

static void AddWindowToSortedBuffer(ImGuiWindow* window, ImVector<ImGuiWindow*>& sorted_windows)
{
    sorted_windows.push_back(window);
    if (window->Visible)
    {
        for (size_t i = 0; i < window->DC.ChildWindows.size(); i++)
        {
            ImGuiWindow* child = window->DC.ChildWindows[i];
            if (child->Visible)
                AddWindowToSortedBuffer(child, sorted_windows);
        }
    }
}

static void PushClipRect(const ImVec4& clip_rect, bool clipped = true)
{
    ImGuiWindow* window = GetCurrentWindow();

    ImVec4 cr = clip_rect;
    if (clipped && !window->ClipRectStack.empty())
    {
        // Clip to new clip rect
        const ImVec4 cur_cr = window->ClipRectStack.back();
        cr = ImVec4(ImMax(cr.x, cur_cr.x), ImMax(cr.y, cur_cr.y), ImMin(cr.z, cur_cr.z), ImMin(cr.w, cur_cr.w));
    }

    window->ClipRectStack.push_back(cr);
    window->DrawList->PushClipRect(cr);
}

static void PopClipRect()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->ClipRectStack.pop_back();
    window->DrawList->PopClipRect();
}

void ImGui::Render()
{
    ImGuiState& g = GImGui;
    IM_ASSERT(g.Initialized);                       // Forgot to call ImGui::NewFrame()

    const bool first_render_of_the_frame = (g.FrameCountRendered != g.FrameCount);
    g.FrameCountRendered = g.FrameCount;
    
    if (first_render_of_the_frame)
    {
        // Hide implicit window if it hasn't been used
        IM_ASSERT(g.CurrentWindowStack.size() == 1);    // Mismatched Begin/End 
        if (g.CurrentWindow && !g.CurrentWindow->Accessed)
            g.CurrentWindow->Visible = false;
        ImGui::End();

        // Sort the window list so that all child windows are after their parent
        // We cannot do that on FocusWindow() because childs may not exist yet
        ImVector<ImGuiWindow*> sorted_windows;
        sorted_windows.reserve(g.Windows.size());
        for (size_t i = 0; i != g.Windows.size(); i++)
        {
            ImGuiWindow* window = g.Windows[i];
            if (window->Flags & ImGuiWindowFlags_ChildWindow)           // if a child is visible its parent will add it
                if (window->Visible)
                    continue;
            AddWindowToSortedBuffer(window, sorted_windows);
        }
        IM_ASSERT(g.Windows.size() == sorted_windows.size());           // We done something wrong
        g.Windows.swap(sorted_windows);

        // Clear data for next frame
        g.IO.MouseWheel = 0;
        memset(g.IO.InputCharacters, 0, sizeof(g.IO.InputCharacters));
    }

    // Skip render altogether if alpha is 0.0
    // Note that vertex buffers have been created, so it is best practice that you don't call Begin/End in the first place.
    if (g.Style.Alpha > 0.0f)
    {
        // Render tooltip
        if (g.Tooltip[0])
        {
            // Use a dummy window to render the tooltip
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(g.Tooltip);
            ImGui::EndTooltip();
        }

        // Gather windows to render
        g.RenderDrawLists.resize(0);
        for (size_t i = 0; i != g.Windows.size(); i++)
        {
            ImGuiWindow* window = g.Windows[i];
            if (window->Visible && (window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip)) == 0)
                window->AddToRenderList();
        }
        for (size_t i = 0; i != g.Windows.size(); i++)
        {
            ImGuiWindow* window = g.Windows[i];
            if (window->Visible && (window->Flags & ImGuiWindowFlags_Tooltip))
                window->AddToRenderList();
        }

        // Render
        if (!g.RenderDrawLists.empty())
            g.IO.RenderDrawListsFn(&g.RenderDrawLists[0], (int)g.RenderDrawLists.size());
        g.RenderDrawLists.resize(0);
    }
}

// Find the optional ## from which we stop displaying text.
static const char*  FindTextDisplayEnd(const char* text, const char* text_end = NULL)
{
    const char* text_display_end = text;
    while ((!text_end || text_display_end < text_end) && *text_display_end != '\0' && (text_display_end[0] != '#' || text_display_end[1] != '#'))
        text_display_end++;
    return text_display_end;
}

// Log ImGui display into text output (tty or file or clipboard)
static void LogText(const ImVec2& ref_pos, const char* text, const char* text_end)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    if (!text_end)
        text_end = FindTextDisplayEnd(text, text_end);

    const bool log_new_line = ref_pos.y > window->DC.LogLineHeight+1;
    window->DC.LogLineHeight = ref_pos.y;

    const char* text_remaining = text;
    const int tree_depth = window->DC.TreeDepth;
    while (true)
    {
        const char* line_end = text_remaining;
        while (line_end < text_end)
            if (*line_end == '\n')
                break;
            else
                line_end++;
        if (line_end >= text_end)
            line_end = NULL;

        const bool is_first_line = (text == text_remaining);
        bool is_last_line = false;
        if (line_end == NULL)
        {
            is_last_line = true;
            line_end = text_end;
        }
        if (line_end != NULL && !(is_last_line && (line_end - text_remaining)==0))
        {
            const int char_count = (int)(line_end - text_remaining);
            if (g.LogFile)
            {
                if (log_new_line || !is_first_line)
                    fprintf(g.LogFile, "\n%*s%.*s", tree_depth*4, "", char_count, text_remaining);
                else
                    fprintf(g.LogFile, " %.*s", char_count, text_remaining);
            }
            else
            {
                if (log_new_line || !is_first_line)
                    g.LogClipboard->append("\n%*s%.*s", tree_depth*4, "", char_count, text_remaining);
                else
                    g.LogClipboard->append(" %.*s", char_count, text_remaining);
            }
        }

        if (is_last_line)
            break;
        text_remaining = line_end + 1;
    }
}

static float CalcWrapWidthForPos(const ImVec2& pos, float wrap_pos_x)
{
    if (wrap_pos_x < 0.0f)
        return 0.0f;

    ImGuiWindow* window = GetCurrentWindow();
    if (wrap_pos_x == 0.0f)
        wrap_pos_x = ImGui::GetContentRegionMax().x;
    if (wrap_pos_x > 0.0f)
        wrap_pos_x += window->Pos.x; // wrap_pos_x is provided is window local space
    
    const float wrap_width = wrap_pos_x > 0.0f ? ImMax(wrap_pos_x - pos.x, 0.00001f) : 0.0f;
    return wrap_width;
}

// Internal ImGui function to render text (called from ImGui::Text(), ImGui::TextUnformatted(), etc.)
// RenderText() calls ImDrawList::AddText() calls ImBitmapFont::RenderText()
static void RenderText(ImVec2 pos, const char* text, const char* text_end, bool hide_text_after_hash, float wrap_width)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    // Hide anything after a '##' string
    const char* text_display_end;
    if (hide_text_after_hash)
    {
        text_display_end = FindTextDisplayEnd(text, text_end);
    }
    else
    {
        if (!text_end)
            text_end = text + strlen(text); // FIXME-OPT
        text_display_end = text_end;
    }

    const int text_len = (int)(text_display_end - text);
    if (text_len > 0)
    {
        // Render
        window->DrawList->AddText(window->Font(), window->FontSize(), pos, window->Color(ImGuiCol_Text), text, text + text_len, wrap_width);

        // Log as text. We split text into individual lines to add current tree level padding
        if (g.LogEnabled)
            LogText(pos, text, text_display_end);
    }
}

// Render a rectangle shaped with optional rounding and borders
static void RenderFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border, float rounding)
{
    ImGuiWindow* window = GetCurrentWindow();

    window->DrawList->AddRectFilled(p_min, p_max, fill_col, rounding);
    if (border && (window->Flags & ImGuiWindowFlags_ShowBorders))
    {
        // FIXME: This is the best I've found that works on multiple renderer/back ends. Rather dodgy.
        const float offset = GImGui.IO.PixelCenterOffset;
        window->DrawList->AddRect(p_min+ImVec2(1.5f-offset,1.5f-offset), p_max+ImVec2(1.0f-offset*2,1.0f-offset*2), window->Color(ImGuiCol_BorderShadow), rounding);
        window->DrawList->AddRect(p_min+ImVec2(0.5f-offset,0.5f-offset), p_max+ImVec2(0.0f-offset*2,0.0f-offset*2), window->Color(ImGuiCol_Border), rounding);
    }
}

// Render a triangle to denote expanded/collapsed state
static void RenderCollapseTriangle(ImVec2 p_min, bool open, float scale, bool shadow)
{
    ImGuiWindow* window = GetCurrentWindow();

    const float h = window->FontSize() * 1.00f;
    const float r = h * 0.40f * scale;
    ImVec2 center = p_min + ImVec2(h*0.50f, h*0.50f*scale);

    ImVec2 a, b, c;
    if (open)
    {
        center.y -= r*0.25f;
        a = center + ImVec2(0,1)*r;
        b = center + ImVec2(-0.866f,-0.5f)*r;
        c = center + ImVec2(0.866f,-0.5f)*r;
    }
    else
    {
        a = center + ImVec2(1,0)*r;
        b = center + ImVec2(-0.500f,0.866f)*r;
        c = center + ImVec2(-0.500f,-0.866f)*r;
    }
    
    if (shadow && (window->Flags & ImGuiWindowFlags_ShowBorders) != 0)
        window->DrawList->AddTriangleFilled(a+ImVec2(2,2), b+ImVec2(2,2), c+ImVec2(2,2), window->Color(ImGuiCol_BorderShadow));
    window->DrawList->AddTriangleFilled(a, b, c, window->Color(ImGuiCol_Border));
}

// Calculate text size. Text can be multi-line. Optionally ignore text after a ## marker.
// CalcTextSize("") should return ImVec2(0.0f, GImGui.FontSize)
ImVec2 ImGui::CalcTextSize(const char* text, const char* text_end, bool hide_text_after_hash, float wrap_width)
{
    ImGuiWindow* window = GetCurrentWindow();

    const char* text_display_end;
    if (hide_text_after_hash)
        text_display_end = FindTextDisplayEnd(text, text_end);      // Hide anything after a '##' string
    else
        text_display_end = text_end;

    const ImVec2 text_size = window->Font()->CalcTextSizeA(window->FontSize(), FLT_MAX, wrap_width, text, text_display_end, NULL);
    return text_size;
}

// Find window given position, search front-to-back
static ImGuiWindow* FindHoveredWindow(ImVec2 pos, bool excluding_childs)
{
    ImGuiState& g = GImGui;
    for (int i = (int)g.Windows.size()-1; i >= 0; i--)
    {
        ImGuiWindow* window = g.Windows[(size_t)i];
        if (!window->Visible)
            continue;
        if (excluding_childs && (window->Flags & ImGuiWindowFlags_ChildWindow) != 0)
            continue;
        ImGuiAabb bb(window->Pos - g.Style.TouchExtraPadding, window->Pos+window->Size + g.Style.TouchExtraPadding);
        if (bb.Contains(pos))
            return window;
    }
    return NULL;
}

// Test if mouse cursor is hovering given aabb
// NB- Box is clipped by our current clip setting
// NB- Expand the aabb to be generous on imprecise inputs systems (g.Style.TouchExtraPadding)
static bool IsMouseHoveringBox(const ImGuiAabb& box)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    // Clip
    ImGuiAabb box_clipped = box;
    if (!window->ClipRectStack.empty())
    {
        const ImVec4 clip_rect = window->ClipRectStack.back();
        box_clipped.Clip(ImGuiAabb(ImVec2(clip_rect.x, clip_rect.y), ImVec2(clip_rect.z, clip_rect.w)));
    }

    // Expand for touch input
    const ImGuiAabb box_for_touch(box_clipped.Min - g.Style.TouchExtraPadding, box_clipped.Max + g.Style.TouchExtraPadding);
    return box_for_touch.Contains(g.IO.MousePos);
}

bool ImGui::IsMouseHoveringBox(const ImVec2& box_min, const ImVec2& box_max)
{
    return IsMouseHoveringBox(ImGuiAabb(box_min, box_max));
}

bool ImGui::IsMouseHoveringWindow()
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    return g.HoveredWindow == window;
}

bool ImGui::IsMouseHoveringAnyWindow()
{
    ImGuiState& g = GImGui;
    return g.HoveredWindow != NULL;
}

bool ImGui::IsPosHoveringAnyWindow(const ImVec2& pos)
{
    return FindHoveredWindow(pos, false) != NULL;
}

static bool IsKeyPressedMap(ImGuiKey key, bool repeat)
{
    ImGuiState& g = GImGui;
    const int key_index = g.IO.KeyMap[key];
    return ImGui::IsKeyPressed(key_index, repeat);
}

bool ImGui::IsKeyPressed(int key_index, bool repeat)
{
    ImGuiState& g = GImGui;
    IM_ASSERT(key_index >= 0 && key_index < IM_ARRAYSIZE(g.IO.KeysDown));
    const float t = g.IO.KeysDownTime[key_index];
    if (t == 0.0f)
        return true;

    // FIXME: Repeat rate should be provided elsewhere?
    const float KEY_REPEAT_DELAY = 0.250f;
    const float KEY_REPEAT_RATE = 0.020f;
    if (repeat && t > KEY_REPEAT_DELAY)
        if ((fmodf(t - KEY_REPEAT_DELAY, KEY_REPEAT_RATE) > KEY_REPEAT_RATE*0.5f) != (fmodf(t - KEY_REPEAT_DELAY - g.IO.DeltaTime, KEY_REPEAT_RATE) > KEY_REPEAT_RATE*0.5f))
            return true;

    return false;
}

bool ImGui::IsMouseClicked(int button, bool repeat)
{
    ImGuiState& g = GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    const float t = g.IO.MouseDownTime[button];
    if (t == 0.0f)
        return true;

    // FIXME: Repeat rate should be provided elsewhere?
    const float MOUSE_REPEAT_DELAY = 0.250f;
    const float MOUSE_REPEAT_RATE = 0.020f;
    if (repeat && t > MOUSE_REPEAT_DELAY)
        if ((fmodf(t - MOUSE_REPEAT_DELAY, MOUSE_REPEAT_RATE) > MOUSE_REPEAT_RATE*0.5f) != (fmodf(t - MOUSE_REPEAT_DELAY - g.IO.DeltaTime, MOUSE_REPEAT_RATE) > MOUSE_REPEAT_RATE*0.5f))
            return true;

    return false;
}

bool ImGui::IsMouseDoubleClicked(int button)
{
    ImGuiState& g = GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseDoubleClicked[button];
}

ImVec2 ImGui::GetMousePos()
{
    return GImGui.IO.MousePos;
}

bool ImGui::IsItemHovered()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.LastItemHovered;
}

bool ImGui::IsItemFocused()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.LastItemFocused;
}

ImVec2 ImGui::GetItemBoxMin()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.LastItemAabb.Min;
}

ImVec2 ImGui::GetItemBoxMax()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.LastItemAabb.Max;
}

// Tooltip is stored and turned into a BeginTooltip()/EndTooltip() sequence at the end of the frame. Each call override previous value.
void ImGui::SetTooltipV(const char* fmt, va_list args)
{
    ImGuiState& g = GImGui;
    ImFormatStringV(g.Tooltip, IM_ARRAYSIZE(g.Tooltip), fmt, args);
}

void ImGui::SetTooltip(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SetTooltipV(fmt, args);
    va_end(args);
}

// Position new window if they don't have position setting in the .ini file. Rarely useful (used by the sample applications).
void ImGui::SetNewWindowDefaultPos(const ImVec2& pos)
{
    ImGuiState& g = GImGui;
    g.NewWindowDefaultPos = pos;
}

float ImGui::GetTime()
{
    return GImGui.Time;
}

int ImGui::GetFrameCount()
{
    return GImGui.FrameCount;
}

static ImGuiWindow* FindWindow(const char* name)
{
    ImGuiState& g = GImGui;
    for (size_t i = 0; i != g.Windows.size(); i++)
        if (strcmp(g.Windows[i]->Name, name) == 0)
            return g.Windows[i];
    return NULL;
}

void ImGui::BeginTooltip()
{
    ImGui::Begin("##Tooltip", NULL, ImVec2(0,0), 0.9f, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_Tooltip);
}

void ImGui::EndTooltip()
{
    IM_ASSERT(GetCurrentWindow()->Flags & ImGuiWindowFlags_Tooltip);
    ImGui::End();
}

void ImGui::BeginChild(const char* str_id, ImVec2 size, bool border, ImGuiWindowFlags extra_flags)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_ChildWindow;

    const ImVec2 content_max = window->Pos + ImGui::GetContentRegionMax();
    const ImVec2 cursor_pos = window->Pos + ImGui::GetCursorPos();
    if (size.x <= 0.0f)
    {
        if (size.x == 0.0f)
            flags |= ImGuiWindowFlags_ChildWindowAutoFitX;
        size.x = ImMax(content_max.x - cursor_pos.x, g.Style.WindowMinSize.x) - fabsf(size.x);
    }
    if (size.y <= 0.0f)
    {
        if (size.y == 0.0f)
            flags |= ImGuiWindowFlags_ChildWindowAutoFitY;
        size.y = ImMax(content_max.y - cursor_pos.y, g.Style.WindowMinSize.y) - fabsf(size.y);
    }
    if (border)
        flags |= ImGuiWindowFlags_ShowBorders;
    flags |= extra_flags;

    char title[256];
    ImFormatString(title, IM_ARRAYSIZE(title), "%s.%s", window->Name, str_id);

    const float alpha = (flags & ImGuiWindowFlags_ComboBox) ? 1.0f : 0.0f;
    ImGui::Begin(title, NULL, size, alpha, flags);

    if (!(window->Flags & ImGuiWindowFlags_ShowBorders))
        g.CurrentWindow->Flags &= ~ImGuiWindowFlags_ShowBorders;
}

void ImGui::EndChild()
{
    ImGuiWindow* window = GetCurrentWindow();

    if (window->Flags & ImGuiWindowFlags_ComboBox)
    {
        ImGui::End();
    }
    else
    {
        // When using auto-filling child window, we don't provide the width/height to ItemSize so that it doesn't feed back into automatic size-fitting.
        ImVec2 sz = ImGui::GetWindowSize();
        if (window->Flags & ImGuiWindowFlags_ChildWindowAutoFitX)
            sz.x = 0;
        if (window->Flags & ImGuiWindowFlags_ChildWindowAutoFitY)
            sz.y = 0;
        
        ImGui::End();
        ItemSize(sz);
    }
}

// Push a new ImGui window to add widgets to. This can be called multiple times with the same window to append contents
bool ImGui::Begin(const char* name, bool* open, ImVec2 size, float fill_alpha, ImGuiWindowFlags flags)
{
    ImGuiState& g = GImGui;
    const ImGuiStyle& style = g.Style;
    IM_ASSERT(g.Initialized);                       // Forgot to call ImGui::NewFrame()

    ImGuiWindow* window = FindWindow(name);
    if (!window)
    {
        // Create window the first time, and load settings
        if (flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip))
        {
            // Tooltip and child windows don't store settings
            window = (ImGuiWindow*)ImGui::MemAlloc(sizeof(ImGuiWindow));
            new(window) ImGuiWindow(name, ImVec2(0,0), size);
        }
        else
        {
            // Normal windows store settings in .ini file
            ImGuiIniData* settings = FindWindowSettings(name);
            if (settings && ImLength(settings->Size) > 0.0f && !(flags & ImGuiWindowFlags_NoResize))// && ImLengthsize) == 0.0f)
                size = settings->Size;

            window = (ImGuiWindow*)ImGui::MemAlloc(sizeof(ImGuiWindow));
            new(window) ImGuiWindow(name, g.NewWindowDefaultPos, size);

            if (settings->Pos.x != FLT_MAX)
            {
                window->PosFloat = settings->Pos;
                window->Pos = ImVec2((float)(int)window->PosFloat.x, (float)(int)window->PosFloat.y);
                window->Collapsed = settings->Collapsed;
            }
        }
        g.Windows.push_back(window);
    }
    window->Flags = (ImGuiWindowFlags)flags;

    g.CurrentWindowStack.push_back(window);
    g.CurrentWindow = window;

    // Default alpha
    if (fill_alpha < 0.0f)
        fill_alpha = style.WindowFillAlphaDefault;

    // When reusing window again multiple times a frame, just append content (don't need to setup again)
    const int current_frame = ImGui::GetFrameCount();
    const bool first_begin_of_the_frame = (window->LastFrameDrawn != current_frame);
    if (first_begin_of_the_frame)
    {
        window->DrawList->Clear();
        window->Visible = true;

        // New windows appears in front
        if (window->LastFrameDrawn < current_frame - 1)
        {
            FocusWindow(window);
            if ((window->Flags & ImGuiWindowFlags_Tooltip) != 0)
            {
                // Hide for 1 frame while resizing
                window->AutoFitFrames = 2;
                window->AutoFitOnlyGrows = false;
                window->Visible = false;
            }
        }

        window->LastFrameDrawn = current_frame;
        window->ClipRectStack.resize(0);

        if (flags & ImGuiWindowFlags_ChildWindow)
        {
            ImGuiWindow* parent_window = g.CurrentWindowStack[g.CurrentWindowStack.size()-2];
            parent_window->DC.ChildWindows.push_back(window);
            window->Pos = window->PosFloat = parent_window->DC.CursorPos;
            window->SizeFull = size;
        }

        // Outer clipping rectangle
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_ComboBox))
            PushClipRect(g.CurrentWindowStack[g.CurrentWindowStack.size()-2]->ClipRectStack.back());
        else
            PushClipRect(ImVec4(0.0f, 0.0f, g.IO.DisplaySize.x, g.IO.DisplaySize.y));

        // Seed ID stack with our window pointer
        window->IDStack.resize(0);
        ImGui::PushID(window);

        // Move window (at the beginning of the frame to avoid lag)
        const ImGuiID move_id = window->GetID("#MOVE");
        RegisterAliveId(move_id);
        if (g.ActiveId == move_id)
        {
            if (g.IO.MouseDown[0])
            {
                if (!(window->Flags & ImGuiWindowFlags_NoMove))
                {
                    window->PosFloat += g.IO.MouseDelta;
                    MarkSettingsDirty();
                }
                FocusWindow(window);
            }
            else
            {
                g.ActiveId = 0;
            }
        }

        // Tooltips always follow mouse
        if ((window->Flags & ImGuiWindowFlags_Tooltip) != 0)
        {
            window->PosFloat = g.IO.MousePos + ImVec2(32,16) - g.Style.FramePadding*2;
        }

        // Clamp into view
        if (!(window->Flags & ImGuiWindowFlags_ChildWindow))
        {
            const ImVec2 pad = ImVec2(window->FontSize()*2.0f, window->FontSize()*2.0f);
            window->PosFloat = ImMax(window->PosFloat + window->Size, pad) - window->Size;
            window->PosFloat = ImMin(window->PosFloat, ImVec2(g.IO.DisplaySize.x, g.IO.DisplaySize.y) - pad);
            window->SizeFull = ImMax(window->SizeFull, pad);
        }
        window->Pos = ImVec2((float)(int)window->PosFloat.x, (float)(int)window->PosFloat.y);

        // Default item width
        if (window->Size.x > 0.0f && !(window->Flags & ImGuiWindowFlags_Tooltip) && !(window->Flags & ImGuiWindowFlags_AlwaysAutoResize))
            window->ItemWidthDefault = (float)(int)(window->Size.x * 0.65f);
        else
            window->ItemWidthDefault = 200.0f;

        // Prepare for focus requests
        if (window->FocusIdxAllRequestNext == IM_INT_MAX || window->FocusIdxAllCounter == -1)
            window->FocusIdxAllRequestCurrent = IM_INT_MAX;
        else
            window->FocusIdxAllRequestCurrent = (window->FocusIdxAllRequestNext + (window->FocusIdxAllCounter+1)) % (window->FocusIdxAllCounter+1);
        if (window->FocusIdxTabRequestNext == IM_INT_MAX || window->FocusIdxTabCounter == -1)
            window->FocusIdxTabRequestCurrent = IM_INT_MAX;
        else
            window->FocusIdxTabRequestCurrent = (window->FocusIdxTabRequestNext + (window->FocusIdxTabCounter+1)) % (window->FocusIdxTabCounter+1);
        window->FocusIdxAllCounter = window->FocusIdxTabCounter = -1;
        window->FocusIdxAllRequestNext = window->FocusIdxTabRequestNext = IM_INT_MAX;

        ImGuiAabb title_bar_aabb = window->TitleBarAabb();

        // Apply and ImClamp scrolling
        window->ScrollY = window->NextScrollY;
        window->ScrollY = ImMax(window->ScrollY, 0.0f);
        if (!window->Collapsed && !window->SkipItems)
            window->ScrollY = ImMin(window->ScrollY, ImMax(0.0f, (float)window->SizeContentsFit.y - window->SizeFull.y));
        window->NextScrollY = window->ScrollY;

        // At this point we don't have a clipping rectangle setup yet, so we can test and draw in title bar
        // Collapse window by double-clicking on title bar
        if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
        {
            if (g.HoveredWindow == window && IsMouseHoveringBox(title_bar_aabb) && g.IO.MouseDoubleClicked[0])
            {
                window->Collapsed = !window->Collapsed;
                MarkSettingsDirty();
                FocusWindow(window);
            }
        }
        else
        {
            window->Collapsed = false;
        }

        if (window->Collapsed)
        {
            // Draw title bar only
            window->Size = title_bar_aabb.GetSize();
            window->DrawList->AddRectFilled(title_bar_aabb.GetTL(), title_bar_aabb.GetBR(), window->Color(ImGuiCol_TitleBgCollapsed), g.Style.WindowRounding);
            if (window->Flags & ImGuiWindowFlags_ShowBorders)
            {
                window->DrawList->AddRect(title_bar_aabb.GetTL()+ImVec2(1,1), title_bar_aabb.GetBR()+ImVec2(1,1), window->Color(ImGuiCol_BorderShadow), g.Style.WindowRounding);
                window->DrawList->AddRect(title_bar_aabb.GetTL(), title_bar_aabb.GetBR(), window->Color(ImGuiCol_Border), g.Style.WindowRounding);
            }
        }
        else
        {
            window->Size = window->SizeFull;

            ImU32 resize_col = 0;
            if ((window->Flags & ImGuiWindowFlags_Tooltip) != 0)
            {
                // Tooltip always resize
                if (window->AutoFitFrames > 0)
                {
                    window->SizeFull = window->SizeContentsFit + g.Style.WindowPadding - ImVec2(0.0f, g.Style.ItemSpacing.y);
                }
            }
            else
            {
                const ImVec2 size_auto_fit = ImClamp(window->SizeContentsFit + style.AutoFitPadding, style.WindowMinSize, g.IO.DisplaySize - style.AutoFitPadding);
                if ((window->Flags & ImGuiWindowFlags_AlwaysAutoResize) != 0)
                {
                    // Don't continously mark settings as dirty, the size of the window doesn't need to be stored.
                    window->SizeFull = size_auto_fit;
                }
                else if (window->AutoFitFrames > 0)
                {
                    // Auto-fit only grows during the first few frames
                    if (window->AutoFitOnlyGrows)
                        window->SizeFull = ImMax(window->SizeFull, size_auto_fit);
                    else
                        window->SizeFull = size_auto_fit;
                    MarkSettingsDirty();
                }
                else if (!(window->Flags & ImGuiWindowFlags_NoResize))
                {
                    // Resize grip
                    const ImGuiAabb resize_aabb(window->Aabb().GetBR()-ImVec2(18,18), window->Aabb().GetBR());
                    const ImGuiID resize_id = window->GetID("#RESIZE");
                    bool hovered, held;
                    ButtonBehaviour(resize_aabb, resize_id, &hovered, &held, true);
                    resize_col = window->Color(held ? ImGuiCol_ResizeGripActive : hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip);

                    if (g.HoveredWindow == window && held && g.IO.MouseDoubleClicked[0])
                    {
                        // Manual auto-fit
                        window->SizeFull = size_auto_fit;
                        window->Size = window->SizeFull;
                        MarkSettingsDirty();
                    }
                    else if (held)
                    {
                        // Resize
                        window->SizeFull = ImMax(window->SizeFull + g.IO.MouseDelta, style.WindowMinSize);
                        window->Size = window->SizeFull;
                        MarkSettingsDirty();
                    }
                }

                // Update aabb immediately so that the rendering below isn't one frame late
                title_bar_aabb = window->TitleBarAabb();
            }

            // Title bar + Window box
            if (fill_alpha > 0.0f)
            {
                if ((window->Flags & ImGuiWindowFlags_ComboBox) != 0)
                    window->DrawList->AddRectFilled(window->Pos, window->Pos+window->Size, window->Color(ImGuiCol_ComboBg, fill_alpha), 0);
                else
                    window->DrawList->AddRectFilled(window->Pos, window->Pos+window->Size, window->Color(ImGuiCol_WindowBg, fill_alpha), g.Style.WindowRounding);
            }

            if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
                window->DrawList->AddRectFilled(title_bar_aabb.GetTL(), title_bar_aabb.GetBR(), window->Color(ImGuiCol_TitleBg), g.Style.WindowRounding, 1|2);

            // Borders
            if (window->Flags & ImGuiWindowFlags_ShowBorders)
            {
                const float rounding = (window->Flags & ImGuiWindowFlags_ComboBox) ? 0.0f : g.Style.WindowRounding;
                window->DrawList->AddRect(window->Pos+ImVec2(1,1), window->Pos+window->Size+ImVec2(1,1), window->Color(ImGuiCol_BorderShadow), rounding);
                window->DrawList->AddRect(window->Pos, window->Pos+window->Size, window->Color(ImGuiCol_Border), rounding);
                if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
                    window->DrawList->AddLine(title_bar_aabb.GetBL(), title_bar_aabb.GetBR(), window->Color(ImGuiCol_Border));
            }

            // Scrollbar
            window->ScrollbarY = (window->SizeContentsFit.y > window->Size.y) && !(window->Flags & ImGuiWindowFlags_NoScrollbar);
            if (window->ScrollbarY)
            {
                ImGuiAabb scrollbar_bb(window->Aabb().Max.x - style.ScrollBarWidth, title_bar_aabb.Max.y+1, window->Aabb().Max.x, window->Aabb().Max.y-1);
                //window->DrawList->AddLine(scrollbar_bb.GetTL(), scrollbar_bb.GetBL(), g.Colors[ImGuiCol_Border]);
                window->DrawList->AddRectFilled(scrollbar_bb.Min, scrollbar_bb.Max, window->Color(ImGuiCol_ScrollbarBg));
                scrollbar_bb.Expand(ImVec2(-3,-3));

                const float grab_size_y_norm = ImSaturate(window->Size.y / ImMax(window->SizeContentsFit.y, window->Size.y));
                const float grab_size_y = scrollbar_bb.GetHeight() * grab_size_y_norm;

                // Handle input right away (none of the code above is relying on scrolling position)
                bool held = false;
                bool hovered = false;
                if (grab_size_y_norm < 1.0f)
                {
                    const ImGuiID scrollbar_id = window->GetID("#SCROLLY");
                    ButtonBehaviour(scrollbar_bb, scrollbar_id, &hovered, &held, true);
                    if (held)
                    {
                        g.HoveredId = scrollbar_id;
                        const float pos_y_norm = ImSaturate((g.IO.MousePos.y - (scrollbar_bb.Min.y + grab_size_y*0.5f)) / (scrollbar_bb.GetHeight() - grab_size_y)) * (1.0f - grab_size_y_norm);
                        window->ScrollY = pos_y_norm * window->SizeContentsFit.y;
                        window->NextScrollY = window->ScrollY;
                    }
                }

                // Normalized height of the grab
                const float pos_y_norm = ImSaturate(window->ScrollY / ImMax(0.0f, window->SizeContentsFit.y));
                const ImU32 grab_col = window->Color(held ? ImGuiCol_ScrollbarGrabActive : hovered ? ImGuiCol_ScrollbarGrabHovered : ImGuiCol_ScrollbarGrab);
                window->DrawList->AddRectFilled(
                    ImVec2(scrollbar_bb.Min.x, ImLerp(scrollbar_bb.Min.y, scrollbar_bb.Max.y, pos_y_norm)), 
                    ImVec2(scrollbar_bb.Max.x, ImLerp(scrollbar_bb.Min.y, scrollbar_bb.Max.y, pos_y_norm + grab_size_y_norm)), grab_col);
            }

            // Render resize grip
            // (after the input handling so we don't have a frame of latency)
            if (!(window->Flags & ImGuiWindowFlags_NoResize))
            {
                const float r = style.WindowRounding;
                const ImVec2 br = window->Aabb().GetBR();
                if (r == 0.0f)
                {
                    window->DrawList->AddTriangleFilled(br, br-ImVec2(0,14), br-ImVec2(14,0), resize_col);
                }
                else
                {
                    // FIXME: We should draw 4 triangles and decide on a size that's not dependant on the rounding size (previously used 18)
                    window->DrawList->AddArc(br - ImVec2(r,r), r, resize_col, 6, 9, true);
                    window->DrawList->AddTriangleFilled(br+ImVec2(0,-2*r),br+ImVec2(0,-r),br+ImVec2(-r,-r), resize_col);
                    window->DrawList->AddTriangleFilled(br+ImVec2(-r,-r), br+ImVec2(-r,0),br+ImVec2(-2*r,0), resize_col);
                }
            }
        }

        // Setup drawing context
        window->DC.ColumnsStartX = window->WindowPadding().x;
        window->DC.ColumnsOffsetX = 0.0f;
        window->DC.CursorStartPos = window->Pos + ImVec2(window->DC.ColumnsStartX + window->DC.ColumnsOffsetX, window->TitleBarHeight() + window->WindowPadding().y) - ImVec2(0.0f, window->ScrollY);
        window->DC.CursorPos = window->DC.CursorStartPos;
        window->DC.CursorPosPrevLine = window->DC.CursorPos;
        window->DC.CurrentLineHeight = window->DC.PrevLineHeight = 0.0f;
        window->DC.LogLineHeight = window->DC.CursorPos.y - 9999.0f;
        window->DC.ChildWindows.resize(0);
        window->DC.ItemWidth.resize(0); 
        window->DC.ItemWidth.push_back(window->ItemWidthDefault);
        window->DC.AllowKeyboardFocus.resize(0);
        window->DC.AllowKeyboardFocus.push_back(true);
        window->DC.TextWrapPos.resize(0);
        window->DC.TextWrapPos.push_back(-1.0f); // disabled
        window->DC.ColorEditMode = ImGuiColorEditMode_UserSelect;
        window->DC.ColumnsCurrent = 0;
        window->DC.ColumnsCount = 1;
        window->DC.ColumnsStartPos = window->DC.CursorPos;
        window->DC.ColumnsCellMinY = window->DC.ColumnsCellMaxY = window->DC.ColumnsStartPos.y;
        window->DC.TreeDepth = 0;
        window->DC.StateStorage = &window->StateStorage;
        window->DC.OpenNextNode = -1;

        // Reset contents size for auto-fitting
        window->SizeContentsFit = ImVec2(0.0f, 0.0f);
        if (window->AutoFitFrames > 0)
            window->AutoFitFrames--;

        // Title bar
        if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
        {
            RenderCollapseTriangle(window->Pos + style.FramePadding, !window->Collapsed, 1.0f, true);
            if (open)
                CloseWindowButton(open);

            const ImVec2 text_size = CalcTextSize(name);
            const ImVec2 text_min = window->Pos + style.FramePadding + ImVec2(window->FontSize() + style.ItemInnerSpacing.x, 0.0f);
            const ImVec2 text_max = window->Pos + ImVec2(window->Size.x - (open ? (title_bar_aabb.GetHeight()-3) : style.FramePadding.x), style.FramePadding.y + text_size.y);
            const bool clip_title = text_size.x > (text_max.x - text_min.x);    // only push a clip rectangle if we need to, because it may turn into a separate draw call
            if (clip_title)
                PushClipRect(ImVec4(text_min.x, text_min.y, text_max.x, text_max.y));
            RenderText(text_min, name);
            if (clip_title)
                PopClipRect();
        }
    }
    else
    {
        // Outer clipping rectangle
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_ComboBox))
        {
            ImGuiWindow* parent_window = g.CurrentWindowStack[g.CurrentWindowStack.size()-2];
            PushClipRect(parent_window->ClipRectStack.back());
        }
        else
        {
            PushClipRect(ImVec4(0.0f, 0.0f, g.IO.DisplaySize.x, g.IO.DisplaySize.y));
        }
    }

    // Inner clipping rectangle
    // We set this up after processing the resize grip so that our clip rectangle doesn't lag by a frame
    const ImGuiAabb title_bar_aabb = window->TitleBarAabb();
    ImVec4 clip_rect(title_bar_aabb.Min.x+0.5f+window->WindowPadding().x*0.5f, title_bar_aabb.Max.y+0.5f, window->Aabb().Max.x+0.5f-window->WindowPadding().x*0.5f, window->Aabb().Max.y-1.5f);
    if (window->ScrollbarY)
        clip_rect.z -= g.Style.ScrollBarWidth;
    PushClipRect(clip_rect);

    // Clear 'accessed' flag last thing
    if (first_begin_of_the_frame)
        window->Accessed = false;

    // Child window can be out of sight and have "negative" clip windows.
    // Mark them as collapsed so commands are skipped earlier (we can't manually collapse because they have no title bar).
    if (flags & ImGuiWindowFlags_ChildWindow)
    {
        IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0);
        const ImVec4 clip_rect_t = window->ClipRectStack.back();
        window->Collapsed = (clip_rect_t.x >= clip_rect_t.z || clip_rect_t.y >= clip_rect_t.w);

        // We also hide the window from rendering because we've already added its border to the command list.
        // (we could perform the check earlier in the function but it is simpler at this point)
        if (window->Collapsed)
            window->Visible = false;
    }
    if (g.Style.Alpha <= 0.0f)
        window->Visible = false;

    // Return false if we don't intend to display anything to allow user to perform an early out optimisation
    window->SkipItems = window->Collapsed || (!window->Visible && window->AutoFitFrames == 0);
    return !window->SkipItems;
}

void ImGui::End()
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImGui::Columns(1, "#CloseColumns");
    PopClipRect();   // inner window clip rectangle
    PopClipRect();   // outer window clip rectangle

    // Select window for move/focus when we're done with all our widgets (we only consider non-childs windows here)
    const ImGuiAabb bb(window->Pos, window->Pos+window->Size);
    if (g.ActiveId == 0 && g.HoveredId == 0 && g.HoveredWindowExcludingChilds == window && IsMouseHoveringBox(bb) && g.IO.MouseClicked[0])
        g.ActiveId = window->GetID("#MOVE");

    // Stop logging
    if (!(window->Flags & ImGuiWindowFlags_ChildWindow))    // FIXME: add more options for scope of logging
    {
        g.LogEnabled = false;
        if (g.LogFile != NULL)
        {
            fprintf(g.LogFile, "\n");
            if (g.LogFile == stdout)
                fflush(g.LogFile);
            else
                fclose(g.LogFile);
            g.LogFile = NULL;
        }
        if (g.LogClipboard->size() > 1)
        {
            g.LogClipboard->append("\n");
            if (g.IO.SetClipboardTextFn)
                g.IO.SetClipboardTextFn(g.LogClipboard->begin());
            g.LogClipboard->clear();
        }
    }

    // Pop
    g.CurrentWindowStack.pop_back();
    g.CurrentWindow = g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back();
}

// Moving window to front
static void FocusWindow(ImGuiWindow* window)
{
    ImGuiState& g = GImGui;
    g.FocusedWindow = window;

    for (size_t i = 0; i < g.Windows.size(); i++)
        if (g.Windows[i] == window)
        {
            g.Windows.erase(g.Windows.begin() + i);
            break;
        }
    g.Windows.push_back(window);
}

void ImGui::PushItemWidth(float item_width)
{
    ImGuiWindow* window = GetCurrentWindow();
    item_width = (float)(int)item_width;
    window->DC.ItemWidth.push_back(item_width > 0.0f ? item_width : window->ItemWidthDefault);
}

void ImGui::PopItemWidth()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ItemWidth.pop_back();
}

float ImGui::GetItemWidth()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.ItemWidth.back();
}

void ImGui::PushAllowKeyboardFocus(bool allow_keyboard_focus)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.AllowKeyboardFocus.push_back(allow_keyboard_focus);
}

void ImGui::PopAllowKeyboardFocus()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.AllowKeyboardFocus.pop_back();
}

void ImGui::PushTextWrapPos(float wrap_x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.TextWrapPos.push_back(wrap_x);
}

void ImGui::PopTextWrapPos()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.TextWrapPos.pop_back();
}

void ImGui::PushStyleColor(ImGuiCol idx, const ImVec4& col)
{
    ImGuiState& g = GImGui;

    ImGuiColMod backup;
    backup.Col = idx;
    backup.PreviousValue = g.Style.Colors[idx];
    g.ColorModifiers.push_back(backup);
    g.Style.Colors[idx] = col;
}

void ImGui::PopStyleColor(int count)
{
    ImGuiState& g = GImGui;

    while (count > 0)
    {
        ImGuiColMod& backup = g.ColorModifiers.back();
        g.Style.Colors[backup.Col] = backup.PreviousValue;
        g.ColorModifiers.pop_back();
        count--;
    }
}

static float* GetStyleVarFloatAddr(ImGuiStyleVar idx)
{
    ImGuiState& g = GImGui;
    switch (idx)
    {
    case ImGuiStyleVar_Alpha: return &g.Style.Alpha;
    case ImGuiStyleVar_TreeNodeSpacing: return &g.Style.TreeNodeSpacing;
    case ImGuiStyleVar_ColumnsMinSpacing: return &g.Style.ColumnsMinSpacing;
    }
    return NULL;
}

static ImVec2* GetStyleVarVec2Addr(ImGuiStyleVar idx)
{
    ImGuiState& g = GImGui;
    switch (idx)
    {
    case ImGuiStyleVar_WindowPadding: return &g.Style.WindowPadding;
    case ImGuiStyleVar_FramePadding: return &g.Style.FramePadding;
    case ImGuiStyleVar_ItemSpacing: return &g.Style.ItemSpacing;
    case ImGuiStyleVar_ItemInnerSpacing: return &g.Style.ItemInnerSpacing;
    }
    return NULL;
}

void ImGui::PushStyleVar(ImGuiStyleVar idx, float val)
{
    ImGuiState& g = GImGui;

    float* pvar = GetStyleVarFloatAddr(idx);
    IM_ASSERT(pvar != NULL); // Called wrong function?
    ImGuiStyleMod backup;
    backup.Var = idx;
    backup.PreviousValue = ImVec2(*pvar, 0.0f);
    g.StyleModifiers.push_back(backup);
    *pvar = val;
}


void ImGui::PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
{
    ImGuiState& g = GImGui;

    ImVec2* pvar = GetStyleVarVec2Addr(idx);
    IM_ASSERT(pvar != NULL); // Called wrong function?
    ImGuiStyleMod backup;
    backup.Var = idx;
    backup.PreviousValue = *pvar;
    g.StyleModifiers.push_back(backup);
    *pvar = val;
}

void ImGui::PopStyleVar(int count)
{
    ImGuiState& g = GImGui;

    while (count > 0)
    {
        ImGuiStyleMod& backup = g.StyleModifiers.back();
        if (float* pvar_f = GetStyleVarFloatAddr(backup.Var))
            *pvar_f = backup.PreviousValue.x;
        else if (ImVec2* pvar_v = GetStyleVarVec2Addr(backup.Var))
            *pvar_v = backup.PreviousValue;
        g.StyleModifiers.pop_back();
        count--;
    }
}

const char* ImGui::GetStyleColorName(ImGuiCol idx)
{
    // Create switch-case from enum with regexp: ImGuiCol_{.*}, --> case ImGuiCol_\1: return "\1";
    switch (idx)
    {
    case ImGuiCol_Text: return "Text";
    case ImGuiCol_WindowBg: return "WindowBg";
    case ImGuiCol_Border: return "Border";
    case ImGuiCol_BorderShadow: return "BorderShadow";
    case ImGuiCol_FrameBg: return "FrameBg";
    case ImGuiCol_TitleBg: return "TitleBg";
    case ImGuiCol_TitleBgCollapsed: return "TitleBgCollapsed";
    case ImGuiCol_ScrollbarBg: return "ScrollbarBg";
    case ImGuiCol_ScrollbarGrab: return "ScrollbarGrab";
    case ImGuiCol_ScrollbarGrabHovered: return "ScrollbarGrabHovered";
    case ImGuiCol_ScrollbarGrabActive: return "ScrollbarGrabActive";
    case ImGuiCol_ComboBg: return "ComboBg";
    case ImGuiCol_CheckHovered: return "CheckHovered";
    case ImGuiCol_CheckActive: return "CheckActive";
    case ImGuiCol_SliderGrab: return "SliderGrab";
    case ImGuiCol_SliderGrabActive: return "SliderGrabActive";
    case ImGuiCol_Button: return "Button";
    case ImGuiCol_ButtonHovered: return "ButtonHovered";
    case ImGuiCol_ButtonActive: return "ButtonActive";
    case ImGuiCol_Header: return "Header";
    case ImGuiCol_HeaderHovered: return "HeaderHovered";
    case ImGuiCol_HeaderActive: return "HeaderActive";
    case ImGuiCol_Column: return "Column";
    case ImGuiCol_ColumnHovered: return "ColumnHovered";
    case ImGuiCol_ColumnActive: return "ColumnActive";
    case ImGuiCol_ResizeGrip: return "ResizeGrip";
    case ImGuiCol_ResizeGripHovered: return "ResizeGripHovered";
    case ImGuiCol_ResizeGripActive: return "ResizeGripActive";
    case ImGuiCol_CloseButton: return "CloseButton";
    case ImGuiCol_CloseButtonHovered: return "CloseButtonHovered";
    case ImGuiCol_CloseButtonActive: return "CloseButtonActive";
    case ImGuiCol_PlotLines: return "PlotLines";
    case ImGuiCol_PlotLinesHovered: return "PlotLinesHovered";
    case ImGuiCol_PlotHistogram: return "PlotHistogram";
    case ImGuiCol_PlotHistogramHovered: return "PlotHistogramHovered";
    case ImGuiCol_TextSelectedBg: return "TextSelectedBg";
    case ImGuiCol_TooltipBg: return "TooltipBg";
    }
    IM_ASSERT(0);
    return "Unknown";
}

bool ImGui::GetWindowIsFocused()
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    return g.FocusedWindow == window;
}

float ImGui::GetWindowWidth()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->Size.x;
}

ImVec2 ImGui::GetWindowPos()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->Pos;
}

void ImGui::SetWindowPos(const ImVec2& pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    const ImVec2 old_pos = window->Pos;
    window->PosFloat = pos;
    window->Pos = ImVec2((float)(int)window->PosFloat.x, (float)(int)window->PosFloat.y);

    // If we happen to move the window while it is being appended to (which is a bad idea - will smear) let's at least offset the cursor
    window->DC.CursorPos += (window->Pos - old_pos);
}

ImVec2 ImGui::GetWindowSize()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->Size;
}

void ImGui::SetWindowSize(const ImVec2& size)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (ImLength(size) < 0.001f)
    {
        window->AutoFitFrames = 2;
        window->AutoFitOnlyGrows = false;
    }
    else
    {
        window->SizeFull = size;
    }
}

ImVec2 ImGui::GetContentRegionMax()
{
    ImGuiWindow* window = GetCurrentWindow();

    ImVec2 m = window->Size - window->WindowPadding();
    if (window->DC.ColumnsCount != 1)
    {
        m.x = GetColumnOffset(window->DC.ColumnsCurrent + 1);
        m.x -= GImGui.Style.WindowPadding.x;
    }
    else
    {
        if (window->ScrollbarY)
            m.x -= GImGui.Style.ScrollBarWidth;
    }

    return m;
}

ImVec2 ImGui::GetWindowContentRegionMin()
{
    ImGuiWindow* window = GetCurrentWindow();
    return ImVec2(0, window->TitleBarHeight()) + window->WindowPadding();
}

ImVec2 ImGui::GetWindowContentRegionMax()
{
    ImGuiWindow* window = GetCurrentWindow();
    ImVec2 m = window->Size - window->WindowPadding();
    if (window->ScrollbarY)
        m.x -= GImGui.Style.ScrollBarWidth;
    return m;
}

float ImGui::GetTextLineHeight()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->FontSize();
}

float ImGui::GetTextLineSpacing()
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    return window->FontSize() + g.Style.ItemSpacing.y;
}

ImDrawList* ImGui::GetWindowDrawList()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DrawList;
}

ImFont ImGui::GetWindowFont()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->Font();
}

float ImGui::GetWindowFontSize()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->FontSize();
}

void ImGui::SetWindowFontScale(float scale)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->FontWindowScale = scale;
}

ImVec2 ImGui::GetCursorPos()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.CursorPos - window->Pos;
}

void ImGui::SetCursorPos(const ImVec2& pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos = window->Pos + pos;
}

void ImGui::SetCursorPosX(float x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.x = window->Pos.x + x;
}

void ImGui::SetCursorPosY(float y)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.y = window->Pos.y + y;
}

ImVec2 ImGui::GetCursorScreenPos()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.CursorPos;
}

void ImGui::SetScrollPosHere()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->NextScrollY = (window->DC.CursorPos.y + window->ScrollY) - (window->Pos.y + window->SizeFull.y * 0.5f) - (window->TitleBarHeight() + window->WindowPadding().y);
}

void ImGui::SetKeyboardFocusHere(int offset)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->FocusIdxAllRequestNext = window->FocusIdxAllCounter + 1 + offset;
    window->FocusIdxTabRequestNext = IM_INT_MAX;
}

void ImGui::SetTreeStateStorage(ImGuiStorage* tree)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.StateStorage = tree ? tree : &window->StateStorage;
}

ImGuiStorage* ImGui::GetTreeStateStorage()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.StateStorage;
}

void ImGui::TextV(const char* fmt, va_list args)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    static char buf[1024];
    const char* text_end = buf + ImFormatStringV(buf, IM_ARRAYSIZE(buf), fmt, args);
    TextUnformatted(buf, text_end);
}

void ImGui::Text(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextV(fmt, args);
    va_end(args);
}

void ImGui::TextColoredV(const ImVec4& col, const char* fmt, va_list args)
{
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    TextV(fmt, args);
    ImGui::PopStyleColor();
}

void ImGui::TextColored(const ImVec4& col, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextColoredV(col, fmt, args);
    va_end(args);
}

void ImGui::TextWrappedV(const char* fmt, va_list args)
{
    ImGui::PushTextWrapPos(0.0f);
    TextV(fmt, args);
    ImGui::PopTextWrapPos();
}

void ImGui::TextWrapped(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextWrappedV(fmt, args);
    va_end(args);
}

void ImGui::TextUnformatted(const char* text, const char* text_end)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    IM_ASSERT(text != NULL);
    const char* text_begin = text;
    if (text_end == NULL)
        text_end = text + strlen(text); // FIXME-OPT

    const float wrap_pos_x = window->DC.TextWrapPos.back();
    const bool wrap_enabled = wrap_pos_x >= 0.0f;
    if (text_end - text > 2000 && !wrap_enabled)
    {
        // Long text!
        // Perform manual coarse clipping to optimize for long multi-line text
        // From this point we will only compute the width of lines that are visible.
        // Optimization only available when word-wrapping is disabled.
        const char* line = text;
        const float line_height = ImGui::GetTextLineHeight();
        const ImVec2 start_pos = window->DC.CursorPos;
        const ImVec4 clip_rect = window->ClipRectStack.back();
        ImVec2 text_size(0,0);

        if (start_pos.y <= clip_rect.w)
        {
            ImVec2 pos = start_pos;

            // Lines to skip (can't skip when logging text)
            if (!g.LogEnabled)
            {
                int lines_skippable = (int)((clip_rect.y - start_pos.y) / line_height) - 1;
                if (lines_skippable > 0)
                {
                    int lines_skipped = 0;
                    while (line < text_end && lines_skipped <= lines_skippable)
                    {
                        const char* line_end = strchr(line, '\n');
                        line = line_end + 1;
                        lines_skipped++;
                    }
                    pos.y += lines_skipped * line_height;
                }
            }

            // Lines to render
            if (line < text_end)
            {
                ImGuiAabb line_box(pos, pos + ImVec2(ImGui::GetWindowWidth(), line_height));
                while (line < text_end)
                {
                    const char* line_end = strchr(line, '\n');
                    if (IsClipped(line_box))
                        break;

                    const ImVec2 line_size = CalcTextSize(line, line_end, false);
                    text_size.x = ImMax(text_size.x, line_size.x);
                    RenderText(pos, line, line_end, false);
                    if (!line_end)
                        line_end = text_end;
                    line = line_end + 1;
                    line_box.Min.y += line_height;
                    line_box.Max.y += line_height;
                    pos.y += line_height;
                }

                // Count remaining lines
                int lines_skipped = 0;
                while (line < text_end)
                {
                    const char* line_end = strchr(line, '\n');
                    if (!line_end)
                        line_end = text_end;
                    line = line_end + 1;
                    lines_skipped++;
                }
                pos.y += lines_skipped * line_height;
            }

            text_size.y += (pos - start_pos).y;
        }
        const ImGuiAabb bb(window->DC.CursorPos, window->DC.CursorPos + text_size);
        ItemSize(bb);
        ClipAdvance(bb);
    }
    else
    {
        const float wrap_width = wrap_enabled ? CalcWrapWidthForPos(window->DC.CursorPos, wrap_pos_x) : 0.0f;
        const ImVec2 text_size = CalcTextSize(text_begin, text_end, false, wrap_width);
        ImGuiAabb bb(window->DC.CursorPos, window->DC.CursorPos + text_size);
        ItemSize(bb.GetSize(), &bb.Min);

        if (ClipAdvance(bb))
            return;

        // Render
        // We don't hide text after ## in this end-user function.
        RenderText(bb.Min, text_begin, text_end, false, wrap_width);
    }
}

void ImGui::AlignFirstTextHeightToWidgets()
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    // Declare a dummy item size to that upcoming items that are smaller will center-align on the newly expanded line height.
    ItemSize(ImVec2(0, window->FontSize() + g.Style.FramePadding.y*2));
    ImGui::SameLine(0, 0);
}

// Add a label+text combo aligned to other label+value widgets
void ImGui::LabelTextV(const char* label, const char* fmt, va_list args)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;
    const ImGuiStyle& style = g.Style;
    const float w = window->DC.ItemWidth.back();

    static char buf[1024];
    const char* text_begin = &buf[0];
    const char* text_end = text_begin + ImFormatStringV(buf, IM_ARRAYSIZE(buf), fmt, args);

    const ImVec2 text_size = CalcTextSize(label);
    const ImGuiAabb value_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w + style.FramePadding.x*2, text_size.y));
    const ImGuiAabb bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w + style.FramePadding.x*2 + style.ItemInnerSpacing.x, 0.0f) + text_size);
    ItemSize(bb);

    if (ClipAdvance(value_bb))
        return;

    // Render
    RenderText(value_bb.Min, text_begin, text_end);
    RenderText(ImVec2(value_bb.Max.x + style.ItemInnerSpacing.x, value_bb.Min.y), label);
}

void ImGui::LabelText(const char* label, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LabelTextV(label, fmt, args);
    va_end(args);
}

static bool ButtonBehaviour(const ImGuiAabb& bb, const ImGuiID& id, bool* out_hovered, bool* out_held, bool allow_key_modifiers, bool repeat)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    const bool hovered = (g.HoveredWindow == window) && (g.HoveredId == 0) && IsMouseHoveringBox(bb);
    bool pressed = false;
    if (hovered)
    {
        g.HoveredId = id;
        if (allow_key_modifiers || (!g.IO.KeyCtrl && !g.IO.KeyShift))
        {
            if (g.IO.MouseClicked[0])
            {
                g.ActiveId = id;
            }
            else if (repeat && g.ActiveId && ImGui::IsMouseClicked(0, true))
            {
                pressed = true;
            }
        }
    }

    bool held = false;
    if (g.ActiveId == id)
    {
        if (g.IO.MouseDown[0])
        {
            held = true;
        }
        else
        {
            if (hovered)
                pressed = true;
            g.ActiveId = 0;
        }
    }

    if (out_hovered) *out_hovered = hovered;
    if (out_held) *out_held = held;

    return pressed;
}

bool ImGui::Button(const char* label, ImVec2 size, bool repeat_when_held)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 text_size = CalcTextSize(label);
    if (size.x == 0.0f)
        size.x = text_size.x;
    if (size.y == 0.0f)
        size.y = text_size.y;

    const ImGuiAabb bb(window->DC.CursorPos, window->DC.CursorPos+size + style.FramePadding*2.0f);
    ItemSize(bb);

    if (ClipAdvance(bb))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehaviour(bb, id, &hovered, &held, true, repeat_when_held);

    // Render
    const ImU32 col = window->Color((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    RenderFrame(bb.Min, bb.Max, col);

    if (size.x < text_size.x || size.y < text_size.y)
        PushClipRect(ImVec4(bb.Min.x+style.FramePadding.x, bb.Min.y+style.FramePadding.y, bb.Max.x, bb.Max.y-style.FramePadding.y));        // Allow extra to draw over the horizontal padding to make it visible that text doesn't fit
    const ImVec2 off = ImVec2(ImMax(0.0f, size.x - text_size.x) * 0.5f, ImMax(0.0f, size.y - text_size.y) * 0.5f);
    RenderText(bb.Min + style.FramePadding + off, label);
    if (size.x < text_size.x || size.y < text_size.y)
        PopClipRect();

    return pressed;
}

// Small buttons fits within text without additional spacing.
bool ImGui::SmallButton(const char* label)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImGuiAabb bb(window->DC.CursorPos, window->DC.CursorPos+CalcTextSize(label) + ImVec2(style.FramePadding.x*2,0));
    ItemSize(bb);

    if (ClipAdvance(bb))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehaviour(bb, id, &hovered, &held, true);

    // Render
    const ImU32 col = window->Color((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    RenderFrame(bb.Min, bb.Max, col);
    RenderText(bb.Min + ImVec2(style.FramePadding.x,0), label);

    return pressed;
}

// Upper-right button to close a window.
static bool CloseWindowButton(bool* open)
{
    ImGuiWindow* window = GetCurrentWindow();

    const ImGuiID id = window->GetID("##CLOSE");
    const float size = window->TitleBarHeight() - 4.0f;
    const ImGuiAabb bb(window->Aabb().GetTR() + ImVec2(-3.0f-size,2.0f), window->Aabb().GetTR() + ImVec2(-3.0f,2.0f+size));

    bool hovered, held;
    bool pressed = ButtonBehaviour(bb, id, &hovered, &held, true);

    // Render
    const ImU32 col = window->Color((held && hovered) ? ImGuiCol_CloseButtonActive : hovered ? ImGuiCol_CloseButtonHovered : ImGuiCol_CloseButton);
    const ImVec2 center = bb.GetCenter();
    window->DrawList->AddCircleFilled(center, ImMax(2.0f,size*0.5f), col, 16);

    const float cross_extent = (size * 0.5f * 0.7071f) - 1.0f;
    if (hovered)
    {
        window->DrawList->AddLine(center + ImVec2(+cross_extent,+cross_extent), center + ImVec2(-cross_extent,-cross_extent), window->Color(ImGuiCol_Text));
        window->DrawList->AddLine(center + ImVec2(+cross_extent,-cross_extent), center + ImVec2(-cross_extent,+cross_extent), window->Color(ImGuiCol_Text));
    }

    if (open != NULL && pressed)
        *open = !*open;

    return pressed;
}

// Start logging ImGui output to TTY
void ImGui::LogToTTY(int max_depth)
{
    ImGuiState& g = GImGui;
    if (g.LogEnabled)
        return;
    g.LogEnabled = true;
    g.LogFile = stdout;
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

// Start logging ImGui output to given file
void ImGui::LogToFile(int max_depth, const char* filename)
{
    ImGuiState& g = GImGui;
    if (g.LogEnabled)
        return;
    if (!filename)
        filename = g.IO.LogFilename;
    g.LogEnabled = true;
    g.LogFile = fopen(filename, "at");
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

// Start logging ImGui output to clipboard
void ImGui::LogToClipboard(int max_depth)
{
    ImGuiState& g = GImGui;
    if (g.LogEnabled)
        return;
    g.LogEnabled = true;
    g.LogFile = NULL;
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

// Helper to display logging buttons
void ImGui::LogButtons()
{
    ImGuiState& g = GImGui;

    ImGui::PushID("LogButtons");
    const bool log_to_tty = ImGui::Button("Log To TTY");
    ImGui::SameLine();      
    const bool log_to_file = ImGui::Button("Log To File");
    ImGui::SameLine();
    const bool log_to_clipboard = ImGui::Button("Log To Clipboard");
    ImGui::SameLine();

    ImGui::PushItemWidth(80.0f);
    ImGui::PushAllowKeyboardFocus(false);
    ImGui::SliderInt("Depth", &g.LogAutoExpandMaxDepth, 0, 9, NULL);
    ImGui::PopAllowKeyboardFocus();
    ImGui::PopItemWidth();
    ImGui::PopID();

    // Start logging at the end of the function so that the buttons don't appear in the log
    if (log_to_tty)
        LogToTTY(g.LogAutoExpandMaxDepth);
    if (log_to_file)
        LogToFile(g.LogAutoExpandMaxDepth, g.IO.LogFilename);
    if (log_to_clipboard)
        LogToClipboard(g.LogAutoExpandMaxDepth);
}

bool ImGui::CollapsingHeader(const char* label, const char* str_id, const bool display_frame, const bool default_open)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;

    IM_ASSERT(str_id != NULL || label != NULL);
    if (str_id == NULL)
        str_id = label;
    if (label == NULL)
        label = str_id;
    const ImGuiID id = window->GetID(str_id);

    // We only write to the tree storage if the user clicks
    ImGuiStorage* tree = window->DC.StateStorage;
    bool opened;
    if (window->DC.OpenNextNode != -1)
    {
        opened = window->DC.OpenNextNode > 0;
        tree->SetInt(id, opened);
        window->DC.OpenNextNode = -1;
    }
    else
    {
        opened = tree->GetInt(id, default_open) != 0;
    }

    // Framed header expand a little outside the default padding
    const ImVec2 window_padding = window->WindowPadding();
    const ImVec2 text_size = CalcTextSize(label);
    const ImVec2 pos_min = window->DC.CursorPos;
    const ImVec2 pos_max = window->Pos + GetContentRegionMax();
    ImGuiAabb bb = ImGuiAabb(pos_min, ImVec2(pos_max.x, pos_min.y + text_size.y));
    if (display_frame)
    {
        bb.Min.x -= window_padding.x*0.5f - 1;
        bb.Max.x += window_padding.x*0.5f - 1;
        bb.Max.y += style.FramePadding.y * 2;
    }

    const ImGuiAabb text_bb(bb.Min, bb.Min + ImVec2(window->FontSize() + style.FramePadding.x*2*2,0) + text_size);
    ItemSize(ImVec2(text_bb.GetSize().x, bb.GetSize().y));  // NB: we don't provide our width so that it doesn't get feed back into AutoFit

    // When logging is enabled, if automatically expand tree nodes (but *NOT* collapsing headers.. seems like sensible behaviour).
    // NB- If we are above max depth we still allow manually opened nodes to be logged.
    if (!display_frame) 
        if (g.LogEnabled && window->DC.TreeDepth < g.LogAutoExpandMaxDepth)
            opened = true;

    if (ClipAdvance(bb))
        return opened;

    bool hovered, held;
    bool pressed = ButtonBehaviour(display_frame ? bb : text_bb, id, &hovered, &held, false);
    if (pressed)
    {
        opened = !opened;
        tree->SetInt(id, opened);
    }

    // Render
    const ImU32 col = window->Color((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
    if (display_frame)
    {
        // Framed type
        RenderFrame(bb.Min, bb.Max, col, true);
        RenderCollapseTriangle(bb.Min + style.FramePadding, opened, 1.0f, true);
        RenderText(bb.Min + style.FramePadding + ImVec2(window->FontSize() + style.FramePadding.x*2,0), label);
    }
    else
    {
        // Unframed typed for tree nodes
        if ((held && hovered) || hovered)
            RenderFrame(bb.Min, bb.Max, col, false);
        RenderCollapseTriangle(bb.Min + ImVec2(style.FramePadding.x, window->FontSize()*0.15f), opened, 0.70f, false);
        RenderText(bb.Min + ImVec2(window->FontSize() + style.FramePadding.x*2,0), label);
    }

    return opened;
}

// Text with a little bullet aligned to the typical tree node.
void ImGui::BulletTextV(const char* fmt, va_list args)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    static char buf[1024];
    const char* text_begin = buf;
    const char* text_end = text_begin + ImFormatStringV(buf, IM_ARRAYSIZE(buf), fmt, args);

    const float line_height = window->FontSize();
    const ImVec2 text_size = CalcTextSize(text_begin, text_end);
    const ImGuiAabb bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(line_height + (text_size.x > 0.0f ? (g.Style.FramePadding.x*2) : 0.0f),0) + text_size);  // Empty text doesn't add padding
    ItemSize(bb);

    if (ClipAdvance(bb))
        return;

    // Render
    const float bullet_size = line_height*0.15f;
    window->DrawList->AddCircleFilled(bb.Min + ImVec2(g.Style.FramePadding.x + line_height*0.5f, line_height*0.5f), bullet_size, window->Color(ImGuiCol_Text));
    RenderText(bb.Min+ImVec2(window->FontSize()+g.Style.FramePadding.x*2,0), text_begin, text_end);
}

void ImGui::BulletText(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    BulletTextV(fmt, args);
    va_end(args);
}

// If returning 'true' the node is open and the user is responsible for calling TreePop
bool ImGui::TreeNodeV(const char* str_id, const char* fmt, va_list args)
{
    static char buf[1024];
    ImFormatStringV(buf, IM_ARRAYSIZE(buf), fmt, args);

    if (!str_id || !str_id[0])
        str_id = fmt;

    ImGui::PushID(str_id);
    const bool opened = ImGui::CollapsingHeader(buf, "", false);        // do not add to the ID so that TreeNodeSetOpen can access
    ImGui::PopID();

    if (opened)
        ImGui::TreePush(str_id);

    return opened;
}

bool ImGui::TreeNode(const char* str_id, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool s = TreeNodeV(str_id, fmt, args);
    va_end(args);
    return s;
}

// If returning 'true' the node is open and the user is responsible for calling TreePop
bool ImGui::TreeNodeV(const void* ptr_id, const char* fmt, va_list args)
{
    static char buf[1024];
    ImFormatStringV(buf, IM_ARRAYSIZE(buf), fmt, args);

    if (!ptr_id)
        ptr_id = fmt;

    ImGui::PushID(ptr_id);
    const bool opened = ImGui::CollapsingHeader(buf, "", false);
    ImGui::PopID();

    if (opened)
        ImGui::TreePush(ptr_id);

    return opened;
}

bool ImGui::TreeNode(const void* ptr_id, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool s = TreeNodeV(ptr_id, fmt, args);
    va_end(args);
    return s;
}

bool ImGui::TreeNode(const char* str_label_id)
{
    return TreeNode(str_label_id, "%s", str_label_id);
}

void ImGui::OpenNextNode(bool open)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.OpenNextNode = open ? 1 : 0;
}

void ImGui::PushID(const char* str_id)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->IDStack.push_back(window->GetID(str_id));
}

void ImGui::PushID(const void* ptr_id)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->IDStack.push_back(window->GetID(ptr_id));
}

void ImGui::PushID(const int int_id)
{
    const void* ptr_id = (void*)(intptr_t)int_id;
    ImGuiWindow* window = GetCurrentWindow();
    window->IDStack.push_back(window->GetID(ptr_id));
}

void ImGui::PopID()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->IDStack.pop_back();
}

// User can input math operators (e.g. +100) to edit a numerical values.
// NB: only call right after InputText because we are using its InitialValue storage
static void ApplyNumericalTextInput(const char* buf, float *v)
{
    while (*buf == ' ' || *buf == '\t')
        buf++;

    // We don't support '-' op because it would conflict with inputing negative value.
    // Instead you can use +-100 to subtract from an existing value
    char op = buf[0];
    if (op == '+' || op == '*' || op == '/')
    {
        buf++;
        while (*buf == ' ' || *buf == '\t')
            buf++;
    }
    else
    {
        op = 0;
    }
    if (!buf[0])
        return;

    float ref_v = *v;
    if (op)
        if (sscanf(GImGui.InputTextState.InitialText, "%f", &ref_v) < 1)
            return;

    float op_v = 0.0f;
    if (sscanf(buf, "%f", &op_v) < 1)
        return;

    if (op == '+')
        *v = ref_v + op_v;
    else if (op == '*')
        *v = ref_v * op_v;
    else if (op == '/')
    {
        if (op_v == 0.0f)
            return;
        *v = ref_v / op_v;
    }
    else
        *v = op_v;
}

// Use power!=1.0 for logarithmic sliders.
// Adjust display_format to decorate the value with a prefix or a suffix.
bool ImGui::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* display_format, float power)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = window->DC.ItemWidth.back();

    if (!display_format)
        display_format = "%.3f";

    // Parse display precision back from the display format string
    int decimal_precision = 3;
    if (const char* p = strchr(display_format, '%'))
    {
        p++;
        while (*p >= '0' && *p <= '9')
            p++;
        if (*p == '.')
        {
            decimal_precision = atoi(p+1);
            if (decimal_precision < 0 || decimal_precision > 10)
                decimal_precision = 3;
        }
    }

    const ImVec2 text_size = CalcTextSize(label);
    const ImGuiAabb frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, text_size.y) + style.FramePadding*2.0f);
    const ImGuiAabb slider_bb(frame_bb.Min+g.Style.FramePadding, frame_bb.Max-g.Style.FramePadding);
    const ImGuiAabb bb(frame_bb.Min, frame_bb.Max + ImVec2(style.ItemInnerSpacing.x + text_size.x, 0.0f));

    if (IsClipped(slider_bb))
    {
        // NB- we don't use ClipAdvance() in the if() statement because we don't want to submit ItemSize() because we may change into a text edit later which may submit an ItemSize itself
        ItemSize(bb);
        return false;
    }

    const bool tab_focus_requested = window->FocusItemRegister(g.ActiveId == id);

    const bool is_unbound = v_min == -FLT_MAX || v_min == FLT_MAX || v_max == -FLT_MAX || v_max == FLT_MAX;

    const float grab_size_in_units = 1.0f;                                                              // In 'v' units. Probably needs to be parametrized, based on a 'v_step' value? decimal precision?
    float grab_size_in_pixels;
    if (decimal_precision > 0 || is_unbound)
        grab_size_in_pixels = 10.0f;
    else
        grab_size_in_pixels = ImMax(grab_size_in_units * (w / (v_max-v_min+1.0f)), 8.0f);               // Integer sliders
    const float slider_effective_w = slider_bb.GetWidth() - grab_size_in_pixels;
    const float slider_effective_x1 = slider_bb.Min.x + grab_size_in_pixels*0.5f;
    const float slider_effective_x2 = slider_bb.Max.x - grab_size_in_pixels*0.5f;

    // For logarithmic sliders that cross over sign boundary we want the exponential increase to be symmetric around 0.0f
    float linear_zero_pos = 0.0f;   // 0.0->1.0f
    if (!is_unbound)
    {
        if (v_min * v_max < 0.0f)
        {
            // Different sign
            const float linear_dist_min_to_0 = powf(fabsf(0.0f - v_min), 1.0f/power);
            const float linear_dist_max_to_0 = powf(fabsf(v_max - 0.0f), 1.0f/power);
            linear_zero_pos = linear_dist_min_to_0 / (linear_dist_min_to_0+linear_dist_max_to_0);
        }
        else
        {
            // Same sign
            linear_zero_pos = v_min < 0.0f ? 1.0f : 0.0f;
        }
    }

    const bool hovered = (g.HoveredWindow == window) && (g.HoveredId == 0) && IsMouseHoveringBox(slider_bb);
    if (hovered)
        g.HoveredId = id;

    bool start_text_input = false;
    if (tab_focus_requested || (hovered && g.IO.MouseClicked[0]))
    {
        g.ActiveId = id;

        const bool is_ctrl_down = g.IO.KeyCtrl;
        if (tab_focus_requested || is_ctrl_down || is_unbound)
        {
            start_text_input = true;
            g.SliderAsInputTextId = 0;
        }
    }

    // Tabbing or CTRL-clicking through slider turns into an input box
    bool value_changed = false;
    if (start_text_input || (g.ActiveId == id && id == g.SliderAsInputTextId))
    {
        char text_buf[64];
        ImFormatString(text_buf, IM_ARRAYSIZE(text_buf), "%.*f", decimal_precision, *v);

        g.ActiveId = g.SliderAsInputTextId;
        g.HoveredId = 0;
        window->FocusItemUnregister();      // Our replacement slider will override the focus ID (registered previously to allow for a TAB focus to happen)
        value_changed = ImGui::InputText(label, text_buf, IM_ARRAYSIZE(text_buf), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
        if (g.SliderAsInputTextId == 0)
        {
            // First frame
            IM_ASSERT(g.ActiveId == id);    // InputText ID should match the Slider ID (else we'd need to store them both which is also possible)
            g.SliderAsInputTextId = g.ActiveId;
            g.ActiveId = id;
            g.HoveredId = id;
        }
        else
        {
            if (g.ActiveId == g.SliderAsInputTextId)
                g.ActiveId = id;
            else
                g.ActiveId = g.SliderAsInputTextId = 0;
        }
        if (value_changed)
        {
            ApplyNumericalTextInput(text_buf, v);
        }
        return value_changed;
    }

    ItemSize(bb);
    RenderFrame(frame_bb.Min, frame_bb.Max, window->Color(ImGuiCol_FrameBg));

	// Process clicking on the slider
    if (g.ActiveId == id)
    {
        if (g.IO.MouseDown[0])
        {
            if (!is_unbound)
            {
                const float normalized_pos = ImClamp((g.IO.MousePos.x - slider_effective_x1) / slider_effective_w, 0.0f, 1.0f);
                
                // Linear slider
                //float new_value = ImLerp(v_min, v_max, normalized_pos);

                // Account for logarithmic scale on both sides of the zero
                float new_value;
                if (normalized_pos < linear_zero_pos)
                {
                    // Negative: rescale to the negative range before powering
                    float a = 1.0f - (normalized_pos / linear_zero_pos);
                    a = powf(a, power);
                    new_value = ImLerp(ImMin(v_max,0.f), v_min, a);
                }
                else
                {
                    // Positive: rescale to the positive range before powering
                    float a;
                    if (fabsf(linear_zero_pos - 1.0f) > 1.e-6)
                        a = (normalized_pos - linear_zero_pos) / (1.0f - linear_zero_pos);
                    else
                        a = normalized_pos;
                    a = powf(a, power);
                    new_value = ImLerp(ImMax(v_min,0.0f), v_max, a);
                }

                // Round past decimal precision
                //    0->1, 1->0.1, 2->0.01, etc.
                // So when our value is 1.99999 with a precision of 0.001 we'll end up rounding to 2.0
                const float min_step = 1.0f / powf(10.0f, (float)decimal_precision);
                const float remainder = fmodf(new_value, min_step);
                if (remainder <= min_step*0.5f)
                    new_value -= remainder;
                else
                    new_value += (min_step - remainder);

                if (*v != new_value)
                {
                    *v = new_value;
                    value_changed = true;
                }
            }
        }
        else
        {
            g.ActiveId = 0;
        }
    }

    if (!is_unbound)
    {
        // Linear slider
        // const float grab_t = (ImClamp(*v, v_min, v_max) - v_min) / (v_max - v_min);

        // Calculate slider grab positioning
        float grab_t;
        float v_clamped = ImClamp(*v, v_min, v_max);
        if (v_clamped < 0.0f)
        {
            const float f = 1.0f - (v_clamped - v_min) / (ImMin(0.0f,v_max) - v_min);
            grab_t = (1.0f - powf(f, 1.0f/power)) * linear_zero_pos;
        }
        else
        {
            const float f = (v_clamped - ImMax(0.0f,v_min)) / (v_max - ImMax(0.0f,v_min));
            grab_t = linear_zero_pos + powf(f, 1.0f/power) * (1.0f - linear_zero_pos);
        }

        // Draw
        const float grab_x = ImLerp(slider_effective_x1, slider_effective_x2, grab_t);
        const ImGuiAabb grab_bb(ImVec2(grab_x-grab_size_in_pixels*0.5f,frame_bb.Min.y+2.0f), ImVec2(grab_x+grab_size_in_pixels*0.5f,frame_bb.Max.y-2.0f));
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, window->Color(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab));
    }

    // Draw value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    ImFormatString(value_buf, IM_ARRAYSIZE(value_buf), display_format, *v);
    RenderText(ImVec2(slider_bb.GetCenter().x-CalcTextSize(value_buf).x*0.5f, frame_bb.Min.y + style.FramePadding.y), value_buf);

    RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, slider_bb.Min.y), label);

    return value_changed;
}

bool ImGui::SliderAngle(const char* label, float* v, float v_degrees_min, float v_degrees_max)
{
    float v_deg = *v * 360.0f / (2*PI);
    bool value_changed = ImGui::SliderFloat(label, &v_deg, v_degrees_min, v_degrees_max, "%.0f deg", 1.0f);
    *v = v_deg * (2*PI) / 360.0f;
    return value_changed;
}

bool ImGui::SliderInt(const char* label, int* v, int v_min, int v_max, const char* display_format)
{
    if (!display_format)
        display_format = "%.0f";
    float v_f = (float)*v;
    bool value_changed = ImGui::SliderFloat(label, &v_f, (float)v_min, (float)v_max, display_format, 1.0f);
    *v = (int)v_f;
    return value_changed;
}

// Add multiple sliders on 1 line for compact edition of multiple components
static bool SliderFloatN(const char* label, float v[3], int components, float v_min, float v_max, const char* display_format, float power)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const float w_full = window->DC.ItemWidth.back();
    const float w_item_one  = ImMax(1.0f, (float)(int)((w_full - (style.FramePadding.x*2.0f+style.ItemInnerSpacing.x)*(components-1)) / (float)components));
    const float w_item_last = ImMax(1.0f, (float)(int)(w_full - (w_item_one+style.FramePadding.x*2.0f+style.ItemInnerSpacing.x)*(components-1)));

    bool value_changed = false;
    ImGui::PushID(label);
    ImGui::PushItemWidth(w_item_one);
    for (int i = 0; i < components; i++)
    {
        ImGui::PushID(i);
        if (i + 1 == components)
        {
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(w_item_last);
        }
        value_changed |= ImGui::SliderFloat("##v", &v[i], v_min, v_max, display_format, power);
        ImGui::SameLine(0, 0);
        ImGui::PopID();
    }
    ImGui::PopItemWidth();
    ImGui::PopID();

    ImGui::TextUnformatted(label, FindTextDisplayEnd(label));

    return value_changed;
}

bool ImGui::SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* display_format, float power)
{
    return SliderFloatN(label, v, 2, v_min, v_max, display_format, power);
}

bool ImGui::SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* display_format, float power)
{
    return SliderFloatN(label, v, 3, v_min, v_max, display_format, power);
}

bool ImGui::SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* display_format, float power)
{
    return SliderFloatN(label, v, 4, v_min, v_max, display_format, power);
}

enum ImGuiPlotType
{
    ImGuiPlotType_Lines,
    ImGuiPlotType_Histogram
};

static void Plot(ImGuiPlotType plot_type, const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const ImGuiStyle& style = g.Style;

    const ImVec2 text_size = ImGui::CalcTextSize(label);
    if (graph_size.x == 0.0f)
        graph_size.x = window->DC.ItemWidth.back();
    if (graph_size.y == 0.0f)
        graph_size.y = text_size.y;

    const ImGuiAabb frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y) + style.FramePadding*2.0f);
    const ImGuiAabb graph_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImGuiAabb bb(frame_bb.Min, frame_bb.Max + ImVec2(style.ItemInnerSpacing.x + text_size.x,0));
    ItemSize(bb);

    if (ClipAdvance(bb))
        return;

    // Determine scale from values if not specified
    if (scale_min == FLT_MAX || scale_max == FLT_MAX)
    {
        float v_min = FLT_MAX;
        float v_max = -FLT_MAX;
        for (int i = 0; i < values_count; i++)
        {
            const float v = values_getter(data, i);
            v_min = ImMin(v_min, v);
            v_max = ImMax(v_max, v);
        }
        if (scale_min == FLT_MAX)
            scale_min = v_min;
        if (scale_max == FLT_MAX)
            scale_max = v_max;
    }

    RenderFrame(frame_bb.Min, frame_bb.Max, window->Color(ImGuiCol_FrameBg));

    int res_w = ImMin((int)graph_size.x, values_count);
    if (plot_type == ImGuiPlotType_Lines)
        res_w -= 1;

    // Tooltip on hover
    int v_hovered = -1;
    if (IsMouseHoveringBox(graph_bb))
    {
        const float t = ImClamp((g.IO.MousePos.x - graph_bb.Min.x) / (graph_bb.Max.x - graph_bb.Min.x), 0.0f, 0.9999f);
        const int v_idx = (int)(t * (values_count + ((plot_type == ImGuiPlotType_Lines) ? -1 : 0)));
        IM_ASSERT(v_idx >= 0 && v_idx < values_count);
        
        const float v0 = values_getter(data, (v_idx + values_offset) % values_count);
        const float v1 = values_getter(data, (v_idx + 1 + values_offset) % values_count);
        if (plot_type == ImGuiPlotType_Lines)
            ImGui::SetTooltip("%d: %8.4g\n%d: %8.4g", v_idx, v0, v_idx+1, v1);
        else if (plot_type == ImGuiPlotType_Histogram)
            ImGui::SetTooltip("%d: %8.4g", v_idx, v0);
        v_hovered = v_idx;
    }

    const float t_step = 1.0f / (float)res_w;

    float v0 = values_getter(data, (0 + values_offset) % values_count);
    float t0 = 0.0f;
    ImVec2 p0 = ImVec2( t0, 1.0f - ImSaturate((v0 - scale_min) / (scale_max - scale_min)) );

    const ImU32 col_base = window->Color((plot_type == ImGuiPlotType_Lines) ? ImGuiCol_PlotLines : ImGuiCol_PlotHistogram);
    const ImU32 col_hovered = window->Color((plot_type == ImGuiPlotType_Lines) ? ImGuiCol_PlotLinesHovered : ImGuiCol_PlotHistogramHovered);

    for (int n = 0; n < res_w; n++)
    {
        const float t1 = t0 + t_step;
        const int v_idx = (int)(t0 * values_count);
        IM_ASSERT(v_idx >= 0 && v_idx < values_count);
        const float v1 = values_getter(data, (v_idx + values_offset + 1) % values_count);
        const ImVec2 p1 = ImVec2( t1, 1.0f - ImSaturate((v1 - scale_min) / (scale_max - scale_min)) );

        // NB- Draw calls are merged together by the DrawList system.
        if (plot_type == ImGuiPlotType_Lines)
            window->DrawList->AddLine(ImLerp(graph_bb.Min, graph_bb.Max, p0), ImLerp(graph_bb.Min, graph_bb.Max, p1), v_hovered == v_idx ? col_hovered : col_base);
        else if (plot_type == ImGuiPlotType_Histogram)
            window->DrawList->AddRectFilled(ImLerp(graph_bb.Min, graph_bb.Max, p0), ImLerp(graph_bb.Min, graph_bb.Max, ImVec2(p1.x, 1.0f))+ImVec2(-1,0), v_hovered == v_idx ? col_hovered : col_base);

        t0 = t1;
        p0 = p1;
    }

    // Text overlay
    if (overlay_text)
        RenderText(ImVec2(graph_bb.GetCenter().x - ImGui::CalcTextSize(overlay_text).x*0.5f, frame_bb.Min.y + style.FramePadding.y), overlay_text);

    RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, graph_bb.Min.y), label);
}

struct ImGuiPlotArrayGetterData
{
    const float* Values;
    size_t Stride;

    ImGuiPlotArrayGetterData(const float* values, size_t stride) { Values = values; Stride = stride; }
};

static float Plot_ArrayGetter(void* data, int idx)
{
    ImGuiPlotArrayGetterData* plot_data = (ImGuiPlotArrayGetterData*)data;
    const float v = *(float*)(void*)((unsigned char*)plot_data->Values + (size_t)idx * plot_data->Stride);
    return v;
}

void ImGui::PlotLines(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size, size_t stride)
{
    ImGuiPlotArrayGetterData data(values, stride);
    Plot(ImGuiPlotType_Lines, label, &Plot_ArrayGetter, (void*)&data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

void ImGui::PlotLines(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
{
    Plot(ImGuiPlotType_Lines, label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

void ImGui::PlotHistogram(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size, size_t stride)
{
    ImGuiPlotArrayGetterData data(values, stride);
    Plot(ImGuiPlotType_Histogram, label, &Plot_ArrayGetter, (void*)&data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

void ImGui::PlotHistogram(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
{
    Plot(ImGuiPlotType_Histogram, label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

bool ImGui::Checkbox(const char* label, bool* v)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 text_size = CalcTextSize(label);

    const ImGuiAabb check_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(text_size.y + style.FramePadding.y*2, text_size.y + style.FramePadding.y*2));
    ItemSize(check_bb);
    SameLine(0, (int)g.Style.ItemInnerSpacing.x);

    const ImGuiAabb text_bb(window->DC.CursorPos + ImVec2(0,style.FramePadding.y), window->DC.CursorPos + ImVec2(0,style.FramePadding.y) + text_size);
    ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()));
    const ImGuiAabb total_bb(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));

    if (ClipAdvance(total_bb))
        return false;

    const bool hovered = (g.HoveredWindow == window) && (g.HoveredId == 0) && IsMouseHoveringBox(total_bb);
    const bool pressed = hovered && g.IO.MouseClicked[0];
    if (hovered)
        g.HoveredId = id;
    if (pressed)
    {
        *v = !(*v);
        g.ActiveId = 0; // Clear focus
    }

    RenderFrame(check_bb.Min, check_bb.Max, window->Color(hovered ? ImGuiCol_CheckHovered : ImGuiCol_FrameBg));
    if (*v)
    {
        window->DrawList->AddRectFilled(check_bb.Min+ImVec2(3,3), check_bb.Max-ImVec2(3,3), window->Color(ImGuiCol_CheckActive));
    }

    if (g.LogEnabled)
        LogText(text_bb.GetTL(), *v ? "[x]" : "[ ]");
    RenderText(text_bb.GetTL(), label);

    return pressed;
}

bool ImGui::CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value)
{
    bool v = (*flags & flags_value) ? true : false;
    bool pressed = ImGui::Checkbox(label, &v);
    if (v)
        *flags |= flags_value;
    else
        *flags &= ~flags_value;
    return pressed;
}

bool ImGui::RadioButton(const char* label, bool active)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 text_size = CalcTextSize(label);

    const ImGuiAabb check_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(text_size.y + style.FramePadding.y*2-1, text_size.y + style.FramePadding.y*2-1));
    ItemSize(check_bb);
    SameLine(0, (int)style.ItemInnerSpacing.x);

    const ImGuiAabb text_bb(window->DC.CursorPos + ImVec2(0, style.FramePadding.y), window->DC.CursorPos + ImVec2(0, style.FramePadding.y) + text_size);
    ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()));
    const ImGuiAabb total_bb(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));

    if (ClipAdvance(total_bb))
        return false;

    ImVec2 center = check_bb.GetCenter();
    center.x = (float)(int)center.x + 0.5f;
    center.y = (float)(int)center.y + 0.5f;
    const float radius = check_bb.GetHeight() * 0.5f;

    const bool hovered = (g.HoveredWindow == window) && (g.HoveredId == 0) && IsMouseHoveringBox(total_bb);
    const bool pressed = hovered && g.IO.MouseClicked[0];
    if (hovered)
        g.HoveredId = id;

    window->DrawList->AddCircleFilled(center, radius, window->Color(hovered ? ImGuiCol_CheckHovered : ImGuiCol_FrameBg), 16);
    if (active)
        window->DrawList->AddCircleFilled(center, radius-3.0f, window->Color(ImGuiCol_CheckActive), 16);

    if (window->Flags & ImGuiWindowFlags_ShowBorders)
    {
        window->DrawList->AddCircle(center+ImVec2(1,1), radius, window->Color(ImGuiCol_BorderShadow), 16);
        window->DrawList->AddCircle(center, radius, window->Color(ImGuiCol_Border), 16);
    }

    if (g.LogEnabled)
        LogText(text_bb.GetTL(), active ? "(x)" : "( )");
    RenderText(text_bb.GetTL(), label);

    return pressed;
}

bool ImGui::RadioButton(const char* label, int* v, int v_button)
{
    const bool pressed = ImGui::RadioButton(label, *v == v_button);
    if (pressed)
    {
        *v = v_button;
    }
    return pressed;
}

// Wrapper for stb_textedit.h to edit text (our wrapper is for: statically sized buffer, single-line, wchar characters. InputText converts between UTF-8 and wchar)
static int     STB_TEXTEDIT_STRINGLEN(const STB_TEXTEDIT_STRING* obj)                             { return (int)ImStrlenW(obj->Text); }
static ImWchar STB_TEXTEDIT_GETCHAR(const STB_TEXTEDIT_STRING* obj, int idx)                      { return obj->Text[idx]; }
static float   STB_TEXTEDIT_GETWIDTH(STB_TEXTEDIT_STRING* obj, int line_start_idx, int char_idx)  { (void)line_start_idx; return obj->Font->CalcTextSizeW(obj->FontSize, FLT_MAX, &obj->Text[char_idx], &obj->Text[char_idx]+1, NULL).x; }
static int     STB_TEXTEDIT_KEYTOTEXT(int key)                                                    { return key >= 0x10000 ? 0 : key; }
static ImWchar STB_TEXTEDIT_NEWLINE = '\n';
static void    STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, STB_TEXTEDIT_STRING* obj, int line_start_idx)
{
    const ImWchar* text_remaining = NULL;
    const ImVec2 size = obj->Font->CalcTextSizeW(obj->FontSize, FLT_MAX, obj->Text + line_start_idx, NULL, &text_remaining);
    r->x0 = 0.0f;
    r->x1 = size.x;
    r->baseline_y_delta = size.y;
    r->ymin = 0.0f;
    r->ymax = size.y;
    r->num_chars = (int)(text_remaining - (obj->Text + line_start_idx));
}

static bool is_white(unsigned int c)                                                              { return c==0 || c==' ' || c=='\t' || c=='\r' || c=='\n'; }
static bool is_separator(unsigned int c)                                                          { return c==',' || c==';' || c=='(' || c==')' || c=='{' || c=='}' || c=='[' || c==']' || c=='|'; }
static bool STB_TEXTEDIT_IS_SPACE(ImWchar c)													  { return is_white((unsigned int)c) || is_separator((unsigned int)c); }
static void STB_TEXTEDIT_DELETECHARS(STB_TEXTEDIT_STRING* obj, int pos, int n)                    { ImWchar* dst = obj->Text+pos; const ImWchar* src = obj->Text+pos+n; while (ImWchar c = *src++) *dst++ = c; *dst = '\0'; }
static bool STB_TEXTEDIT_INSERTCHARS(STB_TEXTEDIT_STRING* obj, int pos, const ImWchar* new_text, int new_text_len)
{
    const size_t text_len = ImStrlenW(obj->Text);
    if (new_text_len + text_len + 1 >= obj->BufSize)
        return false;

    if (pos != text_len)
        memmove(obj->Text + (size_t)pos + new_text_len, obj->Text + (size_t)pos, (text_len - (size_t)pos) * sizeof(ImWchar));
    memcpy(obj->Text + (size_t)pos, new_text, (size_t)new_text_len * sizeof(ImWchar));
    obj->Text[text_len + (size_t)new_text_len] = '\0';

    return true;
}

enum
{
    STB_TEXTEDIT_K_LEFT = 1 << 16,  // keyboard input to move cursor left
    STB_TEXTEDIT_K_RIGHT,           // keyboard input to move cursor right
    STB_TEXTEDIT_K_UP,              // keyboard input to move cursor up
    STB_TEXTEDIT_K_DOWN,            // keyboard input to move cursor down
    STB_TEXTEDIT_K_LINESTART,       // keyboard input to move cursor to start of line
    STB_TEXTEDIT_K_LINEEND,         // keyboard input to move cursor to end of line
    STB_TEXTEDIT_K_TEXTSTART,       // keyboard input to move cursor to start of text
    STB_TEXTEDIT_K_TEXTEND,         // keyboard input to move cursor to end of text
    STB_TEXTEDIT_K_DELETE,          // keyboard input to delete selection or character under cursor
    STB_TEXTEDIT_K_BACKSPACE,       // keyboard input to delete selection or character left of cursor
    STB_TEXTEDIT_K_UNDO,            // keyboard input to perform undo
    STB_TEXTEDIT_K_REDO,            // keyboard input to perform redo
    STB_TEXTEDIT_K_WORDLEFT,        // keyboard input to move cursor left one word
    STB_TEXTEDIT_K_WORDRIGHT,       // keyboard input to move cursor right one word
    STB_TEXTEDIT_K_SHIFT = 1 << 17
};

#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb_textedit.h"

void ImGuiTextEditState::OnKeyPressed(int key)
{ 
    stb_textedit_key(this, &StbState, key); 
    CursorAnimReset(); 
}

void ImGuiTextEditState::UpdateScrollOffset()
{
    // Scroll in chunks of quarter width
    const float scroll_x_increment = Width * 0.25f;
    const float cursor_offset_x = Font->CalcTextSizeW(FontSize, FLT_MAX, Text, Text+StbState.cursor, NULL).x;
    if (ScrollX > cursor_offset_x)
        ScrollX = ImMax(0.0f, cursor_offset_x - scroll_x_increment);    
    else if (ScrollX < cursor_offset_x - Width)
        ScrollX = cursor_offset_x - Width + scroll_x_increment;
}

ImVec2 ImGuiTextEditState::CalcDisplayOffsetFromCharIdx(int i) const
{
    const ImWchar* text_start = GetTextPointerClippedW(Font, FontSize, Text, ScrollX, NULL);
    const ImWchar* text_end = (Text+i >= text_start) ? Text+i : text_start;                    // Clip if requested character is outside of display
    IM_ASSERT(text_end >= text_start);

    const ImVec2 offset = Font->CalcTextSizeW(FontSize, Width, text_start, text_end, NULL);
    return offset;
}

// [Static]
const char* ImGuiTextEditState::GetTextPointerClippedA(ImFont font, float font_size, const char* text, float width, ImVec2* out_text_size)
{
    if (width <= 0.0f)
        return text;

    const char* text_clipped_end = NULL;
    const ImVec2 text_size = font->CalcTextSizeA(font_size, width, 0.0f, text, NULL, &text_clipped_end);
    if (out_text_size)
        *out_text_size = text_size;
    return text_clipped_end;
}

// [Static]
const ImWchar* ImGuiTextEditState::GetTextPointerClippedW(ImFont font, float font_size, const ImWchar* text, float width, ImVec2* out_text_size)
{
    if (width <= 0.0f)
        return text;

    const ImWchar* text_clipped_end = NULL;
    const ImVec2 text_size = font->CalcTextSizeW(font_size, width, text, NULL, &text_clipped_end);
    if (out_text_size)
        *out_text_size = text_size;
    return text_clipped_end;
}

// [Static]
void ImGuiTextEditState::RenderTextScrolledClipped(ImFont font, float font_size, const char* buf, ImVec2 pos, float width, float scroll_x)
{
    // NB- We start drawing at character boundary
    ImVec2 text_size;
    const char* text_start = GetTextPointerClippedA(font, font_size, buf, scroll_x, NULL);
    const char* text_end = GetTextPointerClippedA(font, font_size, text_start, width, &text_size);

    // Draw a little clip symbol if we've got text on either left or right of the box
    const char symbol_c = '~';
    const float symbol_w = font_size*0.40f;     // FIXME: compute correct width
    const float clip_begin = (text_start > buf && text_start < text_end) ? symbol_w : 0.0f;
    const float clip_end = (text_end[0] != '\0' && text_end > text_start) ? symbol_w : 0.0f;

    // Draw text
    RenderText(pos+ImVec2(clip_begin,0), text_start+(clip_begin>0.0f?1:0), text_end-(clip_end>0.0f?1:0), false);//, &text_params_with_clipping);

    // Draw the clip symbol
    const char s[2] = {symbol_c,'\0'};
    if (clip_begin > 0.0f)
        RenderText(pos, s);
    if (clip_end > 0.0f)
        RenderText(pos+ImVec2(width-clip_end,0.0f), s);
}

bool ImGui::InputFloat(const char* label, float *v, float step, float step_fast, int decimal_precision, ImGuiInputTextFlags extra_flags)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const float w = window->DC.ItemWidth.back();
    const ImVec2 text_size = CalcTextSize(label);
    const ImGuiAabb frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, text_size.y) + style.FramePadding*2.0f);

    ImGui::PushID(label);
    const float button_sz = window->FontSize();
    if (step > 0.0f)
        ImGui::PushItemWidth(ImMax(1.0f, window->DC.ItemWidth.back() - (button_sz+g.Style.FramePadding.x*2.0f+g.Style.ItemInnerSpacing.x)*2));

    char buf[64];
    if (decimal_precision < 0)
        ImFormatString(buf, IM_ARRAYSIZE(buf), "%f", *v);       // Ideally we'd have a minimum decimal precision of 1 to visually denote that it is a float, while hiding non-significant digits?
    else
        ImFormatString(buf, IM_ARRAYSIZE(buf), "%.*f", decimal_precision, *v);
    bool value_changed = false;
    const ImGuiInputTextFlags flags = extra_flags | (ImGuiInputTextFlags_CharsDecimal|ImGuiInputTextFlags_AutoSelectAll);
    if (ImGui::InputText("", buf, IM_ARRAYSIZE(buf), flags))
    {
        ApplyNumericalTextInput(buf, v);
        value_changed = true;
    }

    // Step buttons
    if (step > 0.0f)
    {
        ImGui::PopItemWidth();
        ImGui::SameLine(0, 0);
        if (ImGui::Button("-", ImVec2(button_sz,button_sz), true))
        {
            *v -= g.IO.KeyCtrl && step_fast > 0.0f ? step_fast : step;
            value_changed = true;
        }
        ImGui::SameLine(0, (int)g.Style.ItemInnerSpacing.x);
        if (ImGui::Button("+", ImVec2(button_sz,button_sz), true))
        {
            *v += g.IO.KeyCtrl && step_fast > 0.0f ? step_fast : step;
            value_changed = true;
        }
    }

    ImGui::PopID();

    RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + g.Style.FramePadding.y), label);

    return value_changed;
}

bool ImGui::InputInt(const char* label, int *v, int step, int step_fast, ImGuiInputTextFlags extra_flags)
{
    float f = (float)*v;
    const bool value_changed = ImGui::InputFloat(label, &f, (float)step, (float)step_fast, 0, extra_flags);
    *v = (int)f;
    return value_changed;
}

// Public API to manipulate UTF-8 text
// We expose UTF-8 to the user (unlike the STB_TEXTEDIT_* functions which are manipulating wchar)
void ImGuiTextEditCallbackData::DeleteChars(size_t pos, size_t bytes_count)
{
    char* dst = Buf + pos;
    const char* src = Buf + pos + bytes_count;
    while (char c = *src++)
        *dst++ = c;
    *dst = '\0';

    BufDirty = true;
    if ((size_t)CursorPos + bytes_count >= pos)
        CursorPos -= bytes_count;
    else if ((size_t)CursorPos >= pos)
        CursorPos = pos;
    SelectionStart = SelectionEnd = CursorPos;
}

void ImGuiTextEditCallbackData::InsertChars(size_t pos, const char* new_text, const char* new_text_end)
{
    const size_t text_len = strlen(Buf);
    if (!new_text_end)
        new_text_end = new_text + strlen(new_text);
    const size_t new_text_len = new_text_end - new_text;

    if (new_text_len + text_len + 1 >= BufSize)
        return;

    if (text_len != pos)
        memmove(Buf + pos + new_text_len, Buf + pos, text_len - pos);
    memcpy(Buf + pos, new_text, new_text_len * sizeof(char));
    Buf[text_len + new_text_len] = '\0';

    BufDirty = true;
    if ((size_t)CursorPos >= pos)
        CursorPos += new_text_len;
    SelectionStart = SelectionEnd = CursorPos;
}

// Edit a string of text
bool ImGui::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, void (*callback)(ImGuiTextEditCallbackData*), void* user_data)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;

    const ImGuiID id = window->GetID(label);
    const float w = window->DC.ItemWidth.back();

    const ImVec2 text_size = CalcTextSize(label);
    const ImGuiAabb frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, text_size.y) + style.FramePadding*2.0f);
    const ImGuiAabb bb(frame_bb.Min, frame_bb.Max + ImVec2(style.ItemInnerSpacing.x + text_size.x, 0.0f));
    ItemSize(bb);

    if (ClipAdvance(frame_bb))
        return false;

    // NB: we are only allowed to access it if we are the active widget.
    ImGuiTextEditState& edit_state = g.InputTextState;

    const bool is_ctrl_down = io.KeyCtrl;
    const bool is_shift_down = io.KeyShift;
    const bool tab_focus_requested = window->FocusItemRegister(g.ActiveId == id, (flags & ImGuiInputTextFlags_CallbackCompletion) == 0);	// Using completion callback disable keyboard tabbing
    //const bool align_center = (bool)(flags & ImGuiInputTextFlags_AlignCenter);    // FIXME: Unsupported

    const bool hovered = (g.HoveredWindow == window) && (g.HoveredId == 0) && IsMouseHoveringBox(frame_bb);
    if (hovered)
        g.HoveredId = id;

    bool select_all = (g.ActiveId != id) && (flags & ImGuiInputTextFlags_AutoSelectAll) != 0;
    if (tab_focus_requested || (hovered && io.MouseClicked[0]))
    {
        if (g.ActiveId != id)
        {
            // Start edition
            // Take a copy of the initial buffer value (both in original UTF-8 format and converted to wchar)
            ImFormatString(edit_state.InitialText, IM_ARRAYSIZE(edit_state.InitialText), "%s", buf);
            ImTextStrFromUtf8(edit_state.Text, IM_ARRAYSIZE(edit_state.Text), buf, NULL);
            edit_state.ScrollX = 0.0f;
            edit_state.Width = w;
            stb_textedit_initialize_state(&edit_state.StbState, true); 
            edit_state.CursorAnimReset();
            edit_state.LastCursorPos = ImVec2(-1.f,-1.f);

            if (tab_focus_requested || is_ctrl_down)
                select_all = true;
        }
        g.ActiveId = id;
    }
    else if (io.MouseClicked[0])
    {
        // Release focus when we click outside
        if (g.ActiveId == id)
        {
            g.ActiveId = 0;
        }
    }

    bool value_changed = false;
    bool cancel_edit = false;
    bool enter_pressed = false;
    static char text_tmp_utf8[IM_ARRAYSIZE(edit_state.InitialText)];
    if (g.ActiveId == id)
    {
        // Edit in progress
        edit_state.BufSize = buf_size < IM_ARRAYSIZE(edit_state.Text) ? buf_size : IM_ARRAYSIZE(edit_state.Text);
        edit_state.Font = window->Font();
        edit_state.FontSize = window->FontSize();
    
        const float mx = g.IO.MousePos.x - frame_bb.Min.x - style.FramePadding.x;
        const float my = window->FontSize()*0.5f;   // Flatten mouse because we are doing a single-line edit

        edit_state.UpdateScrollOffset();
        if (select_all || (hovered && io.MouseDoubleClicked[0]))
        {
            edit_state.SelectAll();
            edit_state.SelectedAllMouseLock = true;
        }
        else if (io.MouseClicked[0] && !edit_state.SelectedAllMouseLock)
        {
            stb_textedit_click(&edit_state, &edit_state.StbState, mx + edit_state.ScrollX, my);
            edit_state.CursorAnimReset();

        }
        else if (io.MouseDown[0] && !edit_state.SelectedAllMouseLock)
        {
            stb_textedit_drag(&edit_state, &edit_state.StbState, mx + edit_state.ScrollX, my);
            edit_state.CursorAnimReset();
        }
        if (edit_state.SelectedAllMouseLock && !io.MouseDown[0])
             edit_state.SelectedAllMouseLock = false;

        const int k_mask = (is_shift_down ? STB_TEXTEDIT_K_SHIFT : 0);
		if (IsKeyPressedMap(ImGuiKey_LeftArrow))                { edit_state.OnKeyPressed(is_ctrl_down ? STB_TEXTEDIT_K_WORDLEFT | k_mask : STB_TEXTEDIT_K_LEFT | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_RightArrow))          { edit_state.OnKeyPressed(is_ctrl_down ? STB_TEXTEDIT_K_WORDRIGHT | k_mask  : STB_TEXTEDIT_K_RIGHT | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_Home))                { edit_state.OnKeyPressed(is_ctrl_down ? STB_TEXTEDIT_K_TEXTSTART | k_mask : STB_TEXTEDIT_K_LINESTART | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_End))                 { edit_state.OnKeyPressed(is_ctrl_down ? STB_TEXTEDIT_K_TEXTEND | k_mask : STB_TEXTEDIT_K_LINEEND | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_Delete))              { edit_state.OnKeyPressed(STB_TEXTEDIT_K_DELETE | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_Backspace))           { edit_state.OnKeyPressed(STB_TEXTEDIT_K_BACKSPACE | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_Enter))               { g.ActiveId = 0; enter_pressed = true; }
        else if (IsKeyPressedMap(ImGuiKey_Escape))              { g.ActiveId = 0; cancel_edit = true; }
		else if (is_ctrl_down && IsKeyPressedMap(ImGuiKey_Z))   { edit_state.OnKeyPressed(STB_TEXTEDIT_K_UNDO); }
		else if (is_ctrl_down && IsKeyPressedMap(ImGuiKey_Y))   { edit_state.OnKeyPressed(STB_TEXTEDIT_K_REDO); }
		else if (is_ctrl_down && IsKeyPressedMap(ImGuiKey_A))   { edit_state.SelectAll(); }
        else if (is_ctrl_down && (IsKeyPressedMap(ImGuiKey_X) || IsKeyPressedMap(ImGuiKey_C)))
        {
            // Cut, Copy
            const bool cut = IsKeyPressedMap(ImGuiKey_X);
            if (cut && !edit_state.HasSelection())
                edit_state.SelectAll();

            if (g.IO.SetClipboardTextFn)
            {
                const int ib = edit_state.HasSelection() ? ImMin(edit_state.StbState.select_start, edit_state.StbState.select_end) : 0;
                const int ie = edit_state.HasSelection() ? ImMax(edit_state.StbState.select_start, edit_state.StbState.select_end) : (int)ImStrlenW(edit_state.Text);
                ImTextStrToUtf8(text_tmp_utf8, IM_ARRAYSIZE(text_tmp_utf8), edit_state.Text+ib, edit_state.Text+ie);
                g.IO.SetClipboardTextFn(text_tmp_utf8);
            }

            if (cut)
                stb_textedit_cut(&edit_state, &edit_state.StbState);
        }
        else if (is_ctrl_down && IsKeyPressedMap(ImGuiKey_V))
        {
            // Paste
            if (g.IO.GetClipboardTextFn)
            {
                if (const char* clipboard = g.IO.GetClipboardTextFn())
                {
                    // Remove new-line from pasted buffer
                    size_t clipboard_len = strlen(clipboard);
                    ImWchar* clipboard_filtered = (ImWchar*)ImGui::MemAlloc((clipboard_len+1) * sizeof(ImWchar));
                    int clipboard_filtered_len = 0;
                    for (const char* s = clipboard; *s; )
                    {
                        unsigned int c;
                        const int bytes_count = ImTextCharFromUtf8(&c, s, NULL);
                        if (bytes_count <= 0)
                            break;
                        s += bytes_count;
                        if (c == '\n' || c == '\r')
                            continue;
                        if (c >= 0x10000)
                            continue;
                        clipboard_filtered[clipboard_filtered_len++] = (ImWchar)c;
                    }
                    clipboard_filtered[clipboard_filtered_len] = 0;
                    stb_textedit_paste(&edit_state, &edit_state.StbState, clipboard_filtered, clipboard_filtered_len);
                    ImGui::MemFree(clipboard_filtered);
                }
            }
        }
        else if (g.IO.InputCharacters[0])
        {
            // Text input
            for (int n = 0; n < IM_ARRAYSIZE(g.IO.InputCharacters) && g.IO.InputCharacters[n]; n++)
            {
                const ImWchar c = g.IO.InputCharacters[n];
                if (c)
                {
                    // Filter
                    if (c < 256 && !isprint((char)(c & 0xFF)) && c != ' ')
                        continue;
                    if (flags & ImGuiInputTextFlags_CharsDecimal)
                        if (!(c >= '0' && c <= '9') && (c != '.') && (c != '-') && (c != '+') && (c != '*') && (c != '/'))
                            continue;
                    if (flags & ImGuiInputTextFlags_CharsHexadecimal)
                        if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'))
                            continue;

                    // Insert character!
                    edit_state.OnKeyPressed(c);
                }
            }
        }

        edit_state.CursorAnim += g.IO.DeltaTime;
        edit_state.UpdateScrollOffset();

        if (cancel_edit)
        {
            // Restore initial value
            ImFormatString(buf, buf_size, "%s", edit_state.InitialText);
            value_changed = true;
        }
        else
        {
            // Apply new value immediately - copy modified buffer back
            // Note that as soon as we can focus into the input box, the in-widget value gets priority over any underlying modification of the input buffer
            // FIXME: We actually always render 'buf' in RenderTextScrolledClipped
            // FIXME-OPT: CPU waste to do this every time the widget is active, should mark dirty state from the stb_textedit callbacks
            ImTextStrToUtf8(text_tmp_utf8, IM_ARRAYSIZE(text_tmp_utf8), edit_state.Text, NULL);

            // User callback
            if ((flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways)) != 0)
            {
                IM_ASSERT(callback != NULL);

                // The reason we specify the usage semantic (Completion/History) is that Completion needs to disable keyboard TABBING at the moment.
                ImGuiKey event_key = ImGuiKey_COUNT;
                if ((flags & ImGuiInputTextFlags_CallbackCompletion) != 0 && IsKeyPressedMap(ImGuiKey_Tab))
                    event_key = ImGuiKey_Tab;
                else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_UpArrow))
                    event_key = ImGuiKey_UpArrow;
                else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_DownArrow))
                    event_key = ImGuiKey_DownArrow;

                if (event_key != ImGuiKey_COUNT || (flags & ImGuiInputTextFlags_CallbackAlways) != 0)
                {
                    ImGuiTextEditCallbackData callback_data;
                    callback_data.EventKey = event_key;
                    callback_data.Buf = text_tmp_utf8;
                    callback_data.BufSize = edit_state.BufSize;
                    callback_data.BufDirty = false;
                    callback_data.Flags = flags;
					callback_data.UserData = user_data;

                    // We have to convert from position from wchar to UTF-8 positions
                    const int utf8_cursor_pos = callback_data.CursorPos = ImTextCountUtf8BytesFromWchar(edit_state.Text, edit_state.Text + edit_state.StbState.cursor);
                    const int utf8_selection_start = callback_data.SelectionStart = ImTextCountUtf8BytesFromWchar(edit_state.Text, edit_state.Text + edit_state.StbState.select_start);
                    const int utf8_selection_end = callback_data.SelectionEnd = ImTextCountUtf8BytesFromWchar(edit_state.Text, edit_state.Text + edit_state.StbState.select_end);

					// Call user code
					callback(&callback_data);

                    // Read back what user may have modified
                    IM_ASSERT(callback_data.Buf == text_tmp_utf8);             // Invalid to modify those fields
                    IM_ASSERT(callback_data.BufSize == edit_state.BufSize);
                    IM_ASSERT(callback_data.Flags == flags);
                    if (callback_data.CursorPos != utf8_cursor_pos)            edit_state.StbState.cursor = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.CursorPos);
                    if (callback_data.SelectionStart != utf8_selection_start)  edit_state.StbState.select_start = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionStart);
                    if (callback_data.SelectionEnd != utf8_selection_end)      edit_state.StbState.select_end = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionEnd);
                    if (callback_data.BufDirty)
                    {
                        ImTextStrFromUtf8(edit_state.Text, IM_ARRAYSIZE(edit_state.Text), text_tmp_utf8, NULL);
                        edit_state.CursorAnimReset();
                    }
                }
            }

            if (strcmp(text_tmp_utf8, buf) != 0)
            {
                ImFormatString(buf, buf_size, "%s", text_tmp_utf8);
                value_changed = true;
            }
        }
    }
    
    RenderFrame(frame_bb.Min, frame_bb.Max, window->Color(ImGuiCol_FrameBg), true);

    const ImVec2 font_off_up = ImVec2(0.0f,window->FontSize()+1.0f);    // FIXME: those offsets are part of the style or font API
    const ImVec2 font_off_dn = ImVec2(0.0f,2.0f);

    if (g.ActiveId == id)
    {
        // Draw selection
        const int select_begin_idx = edit_state.StbState.select_start;
        const int select_end_idx = edit_state.StbState.select_end;
        if (select_begin_idx != select_end_idx)
        {
            const ImVec2 select_begin_pos = frame_bb.Min + style.FramePadding + edit_state.CalcDisplayOffsetFromCharIdx(ImMin(select_begin_idx,select_end_idx));
            const ImVec2 select_end_pos = frame_bb.Min + style.FramePadding + edit_state.CalcDisplayOffsetFromCharIdx(ImMax(select_begin_idx,select_end_idx));
            window->DrawList->AddRectFilled(select_begin_pos - font_off_up, select_end_pos + font_off_dn, window->Color(ImGuiCol_TextSelectedBg));
        }
    }

    // FIXME: 'align_center' unsupported
    ImGuiTextEditState::RenderTextScrolledClipped(window->Font(), window->FontSize(), buf, frame_bb.Min + style.FramePadding, w, (g.ActiveId == id) ? edit_state.ScrollX : 0.0f);

    if (g.ActiveId == id)
    {
        const ImVec2 cursor_pos = frame_bb.Min + style.FramePadding + edit_state.CalcDisplayOffsetFromCharIdx(edit_state.StbState.cursor);

        // Draw blinking cursor
        if (g.InputTextState.CursorIsVisible())
            window->DrawList->AddRect(cursor_pos - font_off_up + ImVec2(0,2), cursor_pos + font_off_dn - ImVec2(0,3), window->Color(ImGuiCol_Text));
        
        // Notify OS of text input position
        if (io.ImeSetInputScreenPosFn && ImLength(edit_state.LastCursorPos - cursor_pos) > 0.01f)
            io.ImeSetInputScreenPosFn((int)cursor_pos.x - 1, (int)(cursor_pos.y - window->FontSize()));   // -1 x offset so that Windows IME can cover our cursor. Bit of an extra nicety.

        edit_state.LastCursorPos = cursor_pos;
    }

    RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    if ((flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0)
        return enter_pressed;
    else
        return value_changed;
}

static bool InputFloatN(const char* label, float* v, int components, int decimal_precision)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const float w_full = window->DC.ItemWidth.back();
    const float w_item_one  = ImMax(1.0f, (float)(int)((w_full - (style.FramePadding.x*2.0f+style.ItemInnerSpacing.x) * (components-1)) / (float)components));
    const float w_item_last = ImMax(1.0f, (float)(int)(w_full - (w_item_one+style.FramePadding.x*2.0f+style.ItemInnerSpacing.x) * (components-1)));

    bool value_changed = false;
    ImGui::PushID(label);
    ImGui::PushItemWidth(w_item_one);
    for (int i = 0; i < components; i++)
    {
        ImGui::PushID(i);
        if (i + 1 == components)
        {
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(w_item_last);
        }
        value_changed |= ImGui::InputFloat("##v", &v[i], 0, 0, decimal_precision);
        ImGui::SameLine(0, 0);
        ImGui::PopID();
    }
    ImGui::PopItemWidth();
    ImGui::PopID();

    ImGui::TextUnformatted(label, FindTextDisplayEnd(label));

    return value_changed;
}

bool ImGui::InputFloat2(const char* label, float v[2], int decimal_precision)
{
    return InputFloatN(label, v, 2, decimal_precision);
}

bool ImGui::InputFloat3(const char* label, float v[3], int decimal_precision)
{
    return InputFloatN(label, v, 3, decimal_precision);
}

bool ImGui::InputFloat4(const char* label, float v[4], int decimal_precision)
{
    return InputFloatN(label, v, 4, decimal_precision);
}

static bool Combo_ArrayGetter(void* data, int idx, const char** out_text)
{
    const char** items = (const char**)data;
    if (out_text)
        *out_text = items[idx];
    return true;
}

// Combo box helper allowing to pass an array of strings.
bool ImGui::Combo(const char* label, int* current_item, const char** items, int items_count, int popup_height_items)
{
    const bool value_changed = Combo(label, current_item, Combo_ArrayGetter, (void*)items, items_count, popup_height_items);
    return value_changed;
}

static bool Combo_StringListGetter(void* data, int idx, const char** out_text)
{
    // FIXME-OPT: we could precompute the indices to fasten this. But only 1 active combo means the waste is limited.
    const char* items_separated_by_zeros = (const char*)data;
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
        if (idx == items_count)
            break;
        p += strlen(p) + 1;
        items_count++;
    }
    if (!*p)
        return false;
    if (out_text)
        *out_text = p;
    return true;
}

// Combo box helper allowing to pass all items in a single string.
bool ImGui::Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_height_items)
{
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
        p += strlen(p) + 1;
        items_count++;
    }
    bool value_changed = Combo(label, current_item, Combo_StringListGetter, (void*)items_separated_by_zeros, items_count, popup_height_items);
    return value_changed;
}

// Combo box function.
bool ImGui::Combo(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_height_items)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 text_size = CalcTextSize(label);
    const float arrow_size = (window->FontSize() + style.FramePadding.x * 2.0f);
    const ImGuiAabb frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(window->DC.ItemWidth.back(), text_size.y) + style.FramePadding*2.0f);
    const ImGuiAabb bb(frame_bb.Min, frame_bb.Max + ImVec2(style.ItemInnerSpacing.x + text_size.x,0));

    if (ClipAdvance(frame_bb))
        return false;

    const bool hovered = (g.HoveredWindow == window) && (g.HoveredId == 0) && IsMouseHoveringBox(bb);

    bool value_changed = false;
    ItemSize(frame_bb);
    RenderFrame(frame_bb.Min, frame_bb.Max, window->Color(ImGuiCol_FrameBg));
    RenderFrame(ImVec2(frame_bb.Max.x-arrow_size, frame_bb.Min.y), frame_bb.Max, window->Color(hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button));
    RenderCollapseTriangle(ImVec2(frame_bb.Max.x-arrow_size, frame_bb.Min.y) + style.FramePadding, true);

    if (*current_item >= 0 && *current_item < items_count)
    {
        const char* item_text;
        if (items_getter(data, *current_item, &item_text))
            RenderText(frame_bb.Min + style.FramePadding, item_text, NULL, false);
    }

    ImGui::SameLine(0, (int)g.Style.ItemInnerSpacing.x);
    ImGui::TextUnformatted(label, FindTextDisplayEnd(label));

    ImGui::PushID((int)id);
    bool menu_toggled = false;
    if (hovered)
    {
        g.HoveredId = id;
        if (g.IO.MouseClicked[0])
        {
            menu_toggled = true;
            g.ActiveComboID = (g.ActiveComboID == id) ? 0 : id;
        }
    }
    
    if (g.ActiveComboID == id)
    {
        const ImVec2 backup_pos = ImGui::GetCursorPos();
        const float popup_off_x = 0.0f;//g.Style.ItemInnerSpacing.x;
        const float popup_height = (text_size.y + g.Style.ItemSpacing.y) * ImMin(items_count, popup_height_items) + g.Style.WindowPadding.y;
        const ImGuiAabb popup_aabb(ImVec2(frame_bb.Min.x+popup_off_x, frame_bb.Max.y), ImVec2(frame_bb.Max.x+popup_off_x, frame_bb.Max.y + popup_height));
        ImGui::SetCursorPos(popup_aabb.Min - window->Pos);

        const ImGuiWindowFlags flags = ImGuiWindowFlags_ComboBox | ((window->Flags & ImGuiWindowFlags_ShowBorders) ? ImGuiWindowFlags_ShowBorders : 0);
        ImGui::BeginChild("#ComboBox", popup_aabb.GetSize(), false, flags);
        ImGuiWindow* child_window = GetCurrentWindow();
        ImGui::Spacing();

        bool combo_item_active = false;
        combo_item_active |= (g.ActiveId == child_window->GetID("#SCROLLY"));

        // Display items
        for (int item_idx = 0; item_idx < items_count; item_idx++)
        {
            const float item_h = child_window->FontSize();
            const float spacing_up = (float)(int)(g.Style.ItemSpacing.y/2);
            const float spacing_dn = g.Style.ItemSpacing.y - spacing_up;
            const ImGuiAabb item_aabb(ImVec2(popup_aabb.Min.x, child_window->DC.CursorPos.y - spacing_up), ImVec2(popup_aabb.Max.x, child_window->DC.CursorPos.y + item_h + spacing_dn));
            const ImGuiID item_id = child_window->GetID((void*)(intptr_t)item_idx);

            bool item_hovered, item_held;
            bool item_pressed = ButtonBehaviour(item_aabb, item_id, &item_hovered, &item_held, true);
            bool item_selected = (item_idx == *current_item);

            if (item_hovered || item_selected)
            {
                const ImU32 col = window->Color((item_held && item_hovered) ? ImGuiCol_HeaderActive : item_hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
                RenderFrame(item_aabb.Min, item_aabb.Max, col, false);
            }

            const char* item_text;
            if (!items_getter(data, item_idx, &item_text))
                item_text = "*Unknown item*";
            ImGui::Text("%s", item_text);
            
            if (item_selected)
            {
                if (menu_toggled)
                    ImGui::SetScrollPosHere();
            }
            if (item_pressed)
            {
                g.ActiveId = 0;
                g.ActiveComboID = 0;
                value_changed = true;
                *current_item = item_idx;
            }

            combo_item_active |= (g.ActiveId == item_id);
        }
        ImGui::EndChild();
        ImGui::SetCursorPos(backup_pos);
        
        if (!combo_item_active && g.ActiveId != 0)
            g.ActiveComboID = 0;
    }

    ImGui::PopID();

    return value_changed;
}

// A little colored square. Return true when clicked.
bool ImGui::ColorButton(const ImVec4& col, bool small_height, bool outline_border)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const float square_size = window->FontSize();
    const ImGuiAabb bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(square_size + style.FramePadding.x*2, square_size + (small_height ? 0 : style.FramePadding.y*2)));
    ItemSize(bb);

    if (ClipAdvance(bb))
        return false;

    const bool hovered = (g.HoveredWindow == window) && (g.HoveredId == 0) && IsMouseHoveringBox(bb);
    const bool pressed = hovered && g.IO.MouseClicked[0];
    RenderFrame(bb.Min, bb.Max, window->Color(col), outline_border);

    if (hovered)
    {
        int ix = (int)(col.x * 255.0f + 0.5f);
        int iy = (int)(col.y * 255.0f + 0.5f);
        int iz = (int)(col.z * 255.0f + 0.5f);
        int iw = (int)(col.w * 255.0f + 0.5f);
        ImGui::SetTooltip("Color:\n(%.2f,%.2f,%.2f,%.2f)\n#%02X%02X%02X%02X", col.x, col.y, col.z, col.w, ix, iy, iz, iw);
    }

    return pressed;
}

bool ImGui::ColorEdit3(const char* label, float col[3])
{
    float col4[4];
    col4[0] = col[0];
    col4[1] = col[1];
    col4[2] = col[2];
    col4[3] = 1.0f;
    const bool value_changed = ImGui::ColorEdit4(label, col4, false);
    col[0] = col4[0];
    col[1] = col4[1];
    col[2] = col4[2];
    return value_changed;
}

// Edit colors components (each component in 0.0f..1.0f range
// Use CTRL-Click to input value and TAB to go to next item.
bool ImGui::ColorEdit4(const char* label, float col[4], bool alpha)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w_full = window->DC.ItemWidth.back();
    const float square_sz = (window->FontSize() + style.FramePadding.x * 2.0f);

    ImGuiColorEditMode edit_mode = window->DC.ColorEditMode;
    if (edit_mode == ImGuiColorEditMode_UserSelect)
        edit_mode = g.ColorEditModeStorage.GetInt(id, 0) % 3;

    float fx = col[0];
    float fy = col[1];
    float fz = col[2];
    float fw = col[3];
    const ImVec4 col_display(fx, fy, fz, 1.0f);

    if (edit_mode == ImGuiColorEditMode_HSV)
        ImConvertColorRGBtoHSV(fx, fy, fz, fx, fy, fz);

    int ix = (int)(fx * 255.0f + 0.5f);
    int iy = (int)(fy * 255.0f + 0.5f);
    int iz = (int)(fz * 255.0f + 0.5f);
    int iw = (int)(fw * 255.0f + 0.5f);

    int components = alpha ? 4 : 3;
    bool value_changed = false;

    ImGui::PushID(label);

    bool hsv = (edit_mode == 1);
    switch (edit_mode)
    {
    case ImGuiColorEditMode_RGB:
    case ImGuiColorEditMode_HSV:
        {
            // 0: RGB 0..255
            // 1: HSV 0.255 Sliders
            const float w_items_all = w_full - (square_sz + style.ItemInnerSpacing.x);
            const float w_item_one  = ImMax(1.0f, (float)(int)((w_items_all - (style.FramePadding.x*2.0f+style.ItemInnerSpacing.x) * (components-1)) / (float)components));
            const float w_item_last = ImMax(1.0f, (float)(int)(w_items_all - (w_item_one+style.FramePadding.x*2.0f+style.ItemInnerSpacing.x) * (components-1)));

            ImGui::PushItemWidth(w_item_one);
            value_changed |= ImGui::SliderInt("##X", &ix, 0, 255, hsv ? "H:%3.0f" : "R:%3.0f");
            ImGui::SameLine(0, 0);
            value_changed |= ImGui::SliderInt("##Y", &iy, 0, 255, hsv ? "S:%3.0f" : "G:%3.0f");
            ImGui::SameLine(0, 0);
            if (alpha)
            {
                value_changed |= ImGui::SliderInt("##Z", &iz, 0, 255, hsv ? "V:%3.0f" : "B:%3.0f");
                ImGui::SameLine(0, 0);
                ImGui::PushItemWidth(w_item_last);
                value_changed |= ImGui::SliderInt("##W", &iw, 0, 255, "A:%3.0f");
            }
            else
            {
                ImGui::PushItemWidth(w_item_last);
                value_changed |= ImGui::SliderInt("##Z", &iz, 0, 255, hsv ? "V:%3.0f" : "B:%3.0f");
            }
            ImGui::PopItemWidth();
            ImGui::PopItemWidth();
        }
        break;
    case ImGuiColorEditMode_HEX:
        {
            // 2: RGB Hexadecimal
            const float w_slider_all = w_full - square_sz;
            char buf[64];
            if (alpha)
                sprintf(buf, "#%02X%02X%02X%02X", ix, iy, iz, iw);
            else
                sprintf(buf, "#%02X%02X%02X", ix, iy, iz);
            ImGui::PushItemWidth(w_slider_all - g.Style.ItemInnerSpacing.x);
            value_changed |= ImGui::InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::PopItemWidth();
            char* p = buf;
            while (*p == '#' || *p == ' ' || *p == '\t') 
                p++;

            // Treat at unsigned (%X is unsigned)
            ix = iy = iz = iw = 0;
            if (alpha)
                sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&ix, (unsigned int*)&iy, (unsigned int*)&iz, (unsigned int*)&iw);
            else
                sscanf(p, "%02X%02X%02X", (unsigned int*)&ix, (unsigned int*)&iy, (unsigned int*)&iz);
        }
        break;
    }

    ImGui::SameLine(0, 0);
    ImGui::ColorButton(col_display);

    if (window->DC.ColorEditMode == ImGuiColorEditMode_UserSelect)
    {
        ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
        const char* button_titles[3] = { "RGB", "HSV", "HEX" };
        if (ImGui::Button(button_titles[edit_mode]))
        {
            // Don't set local copy of 'edit_mode' right away!
            g.ColorEditModeStorage.SetInt(id, (edit_mode + 1) % 3);
        }
    }

    ImGui::SameLine();
    ImGui::TextUnformatted(label, FindTextDisplayEnd(label));

    // Convert back
    fx = ix / 255.0f;
    fy = iy / 255.0f;
    fz = iz / 255.0f;
    fw = iw / 255.0f;
    if (edit_mode == 1)
        ImConvertColorHSVtoRGB(fx, fy, fz, fx, fy, fz);

    if (value_changed)
    {
        col[0] = fx;
        col[1] = fy;
        col[2] = fz;
        if (alpha)
            col[3] = fw;
    }

    ImGui::PopID();

    return value_changed;
}

void ImGui::ColorEditMode(ImGuiColorEditMode mode)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ColorEditMode = mode;
}

// Horizontal separating line.
void ImGui::Separator()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    if (window->DC.ColumnsCount > 1)
        PopClipRect();

    const ImGuiAabb bb(ImVec2(window->Pos.x, window->DC.CursorPos.y), ImVec2(window->Pos.x + window->Size.x, window->DC.CursorPos.y));
    ItemSize(ImVec2(0.0f, bb.GetSize().y)); // NB: we don't provide our width so that it doesn't get feed back into AutoFit

    if (ClipAdvance(bb))
    {
        if (window->DC.ColumnsCount > 1)
            PushColumnClipRect();
        return;
    }

    window->DrawList->AddLine(bb.Min, bb.Max, window->Color(ImGuiCol_Border));

    if (window->DC.ColumnsCount > 1)
        PushColumnClipRect();
}

// A little vertical spacing.
void ImGui::Spacing()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ItemSize(ImVec2(0,0));
}

// Advance cursor given item size.
static void ItemSize(ImVec2 size, ImVec2* adjust_start_offset)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const float line_height = ImMax(window->DC.CurrentLineHeight, size.y);
    if (adjust_start_offset)
        adjust_start_offset->y = adjust_start_offset->y + (line_height - size.y) * 0.5f;

    // Always align ourselves on pixel boundaries
    window->DC.CursorPosPrevLine = ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y);
    window->DC.CursorPos = ImVec2((float)(int)(window->Pos.x + window->DC.ColumnsStartX + window->DC.ColumnsOffsetX), (float)(int)(window->DC.CursorPos.y + line_height + g.Style.ItemSpacing.y));

    window->SizeContentsFit = ImMax(window->SizeContentsFit, ImVec2(window->DC.CursorPosPrevLine.x, window->DC.CursorPos.y) - window->Pos + ImVec2(0.0f, window->ScrollY));

    window->DC.PrevLineHeight = line_height;
    window->DC.CurrentLineHeight = 0.0f;
}

static void ItemSize(const ImGuiAabb& aabb, ImVec2* adjust_start_offset) 
{ 
    ItemSize(aabb.GetSize(), adjust_start_offset); 
}

void ImGui::NextColumn()
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    if (window->DC.ColumnsCount > 1)
    {
        ImGui::PopItemWidth();
        PopClipRect();

        window->DC.ColumnsCellMaxY = ImMax(window->DC.ColumnsCellMaxY, window->DC.CursorPos.y);
        if (++window->DC.ColumnsCurrent < window->DC.ColumnsCount)
        {
            window->DC.ColumnsOffsetX = ImGui::GetColumnOffset(window->DC.ColumnsCurrent) - window->DC.ColumnsStartX + g.Style.ItemSpacing.x;
        }
        else
        {
            window->DC.ColumnsCurrent = 0;
            window->DC.ColumnsOffsetX = 0.0f;
            window->DC.ColumnsCellMinY = window->DC.ColumnsCellMaxY;
        }
        window->DC.CursorPos.x = (float)(int)(window->Pos.x + window->DC.ColumnsStartX + window->DC.ColumnsOffsetX);
        window->DC.CursorPos.y = window->DC.ColumnsCellMinY;
        window->DC.CurrentLineHeight = 0.0f;

        PushColumnClipRect();
        ImGui::PushItemWidth(ImGui::GetColumnWidth() * 0.65f);
    }
}

static bool IsClipped(const ImGuiAabb& bb)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    if (!bb.Overlaps(ImGuiAabb(window->ClipRectStack.back())) && !g.LogEnabled)
        return true;
    return false;
}

bool ImGui::IsClipped(const ImVec2& item_size)
{
    ImGuiWindow* window = GetCurrentWindow();
    return IsClipped(ImGuiAabb(window->DC.CursorPos, window->DC.CursorPos + item_size));
}

static bool ClipAdvance(const ImGuiAabb& bb)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.LastItemAabb = bb;
    window->DC.LastItemFocused = false;
    if (IsClipped(bb))
    {
        window->DC.LastItemHovered = false;
        return true;
    }
    window->DC.LastItemHovered = IsMouseHoveringBox(bb);     // this is a sensible default but widgets are free to override it after calling ClipAdvance
    return false;
}

// Gets back to previous line and continue with horizontal layout
//      column_x == 0   : follow on previous item
//      columm_x != 0   : align to specified column
//      spacing_w < 0   : use default spacing if column_x==0, no spacing if column_x!=0
//      spacing_w >= 0  : enforce spacing
void ImGui::SameLine(int column_x, int spacing_w)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;
    
    float x, y;
    if (column_x != 0)
    {
        if (spacing_w < 0) spacing_w = 0;
        x = window->Pos.x + (float)column_x + (float)spacing_w;
        y = window->DC.CursorPosPrevLine.y;
    }
    else
    {
        if (spacing_w < 0) spacing_w = (int)g.Style.ItemSpacing.x;
        x = window->DC.CursorPosPrevLine.x + (float)spacing_w;
        y = window->DC.CursorPosPrevLine.y;
    }
    window->DC.CurrentLineHeight = window->DC.PrevLineHeight;
    window->DC.CursorPos = ImVec2(x, y);
}

float ImGui::GetColumnOffset(int column_index)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (column_index < 0)
        column_index = window->DC.ColumnsCurrent;

    const ImGuiID column_id = window->DC.ColumnsSetID + ImGuiID(column_index);
    RegisterAliveId(column_id);
    const float default_t = column_index / (float)window->DC.ColumnsCount;
    const float t = (float)window->StateStorage.GetInt(column_id, (int)(default_t * 8192)) / 8192;      // Cheaply store our floating point value inside the integer (could store an union into the map?)

    const float offset = window->DC.ColumnsStartX + t * (window->Size.x - g.Style.ScrollBarWidth - window->DC.ColumnsStartX);
    return offset;
}

void ImGui::SetColumnOffset(int column_index, float offset)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (column_index < 0)
        column_index = window->DC.ColumnsCurrent;

    const ImGuiID column_id = window->DC.ColumnsSetID + ImGuiID(column_index);
    const float t = (offset - window->DC.ColumnsStartX) / (window->Size.x - g.Style.ScrollBarWidth - window->DC.ColumnsStartX);
    window->StateStorage.SetInt(column_id, (int)(t*8192));
}

float ImGui::GetColumnWidth(int column_index)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (column_index < 0)
        column_index = window->DC.ColumnsCurrent;

    const float w = GetColumnOffset(column_index+1) - GetColumnOffset(column_index);
    return w;
}

static void PushColumnClipRect(int column_index)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (column_index < 0)
        column_index = window->DC.ColumnsCurrent;

    const float x1 = window->Pos.x + ImGui::GetColumnOffset(column_index) - 1;
    const float x2 = window->Pos.x + ImGui::GetColumnOffset(column_index+1) - 1;
    PushClipRect(ImVec4(x1,-FLT_MAX,x2,+FLT_MAX));
}

void ImGui::Columns(int columns_count, const char* id, bool border)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    if (window->DC.ColumnsCount != 1)
    {
        if (window->DC.ColumnsCurrent != 0)
            ItemSize(ImVec2(0,0));   // Advance to column 0
        ImGui::PopItemWidth();
        PopClipRect();

        window->DC.ColumnsCellMaxY = ImMax(window->DC.ColumnsCellMaxY, window->DC.CursorPos.y);
        window->DC.CursorPos.y = window->DC.ColumnsCellMaxY;
    }

    // Draw columns borders and handle resize at the time of "closing" a columns set
    if (window->DC.ColumnsCount != columns_count && window->DC.ColumnsCount != 1 && window->DC.ColumnsShowBorders)
    {
        const float y1 = window->DC.ColumnsStartPos.y;
        const float y2 = window->DC.CursorPos.y;
        for (int i = 1; i < window->DC.ColumnsCount; i++)
        {
            float x = window->Pos.x + GetColumnOffset(i);
            
            const ImGuiID column_id = window->DC.ColumnsSetID + ImGuiID(i);
            const ImGuiAabb column_aabb(ImVec2(x-4,y1),ImVec2(x+4,y2));

            if (IsClipped(column_aabb))
                continue;

            bool hovered, held;
            ButtonBehaviour(column_aabb, column_id, &hovered, &held, true);

            // Draw before resize so our items positioning are in sync with the line being drawn
            const ImU32 col = window->Color(held ? ImGuiCol_ColumnActive : hovered ? ImGuiCol_ColumnHovered : ImGuiCol_Column);
            const float xi = (float)(int)x;
            window->DrawList->AddLine(ImVec2(xi, y1), ImVec2(xi, y2), col);

            if (held)
            {
                x -= window->Pos.x;
                x = ImClamp(x + g.IO.MouseDelta.x, ImGui::GetColumnOffset(i-1)+g.Style.ColumnsMinSpacing, ImGui::GetColumnOffset(i+1)-g.Style.ColumnsMinSpacing);
                SetColumnOffset(i, x);
                x += window->Pos.x;
            }
        }
    }

    // Set state for first column
    window->DC.ColumnsSetID = window->GetID(id ? id : "");
    window->DC.ColumnsCurrent = 0;
    window->DC.ColumnsCount = columns_count;
    window->DC.ColumnsShowBorders = border;
    window->DC.ColumnsStartPos = window->DC.CursorPos;
    window->DC.ColumnsCellMinY = window->DC.ColumnsCellMaxY = window->DC.CursorPos.y;
    window->DC.ColumnsOffsetX = 0.0f;
    window->DC.CursorPos.x = (float)(int)(window->Pos.x + window->DC.ColumnsStartX + window->DC.ColumnsOffsetX);

    if (window->DC.ColumnsCount != 1)
    {
        PushColumnClipRect();
        ImGui::PushItemWidth(ImGui::GetColumnWidth() * 0.65f);
    }
}

void ImGui::TreePush(const char* str_id)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ColumnsStartX += g.Style.TreeNodeSpacing;
    window->DC.CursorPos.x = window->Pos.x + window->DC.ColumnsStartX + window->DC.ColumnsOffsetX;
    window->DC.TreeDepth++;
    PushID(str_id ? str_id : "#TreePush");
}

void ImGui::TreePush(const void* ptr_id)
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ColumnsStartX += g.Style.TreeNodeSpacing;
    window->DC.CursorPos.x = window->Pos.x + window->DC.ColumnsStartX + window->DC.ColumnsOffsetX;
    window->DC.TreeDepth++;
    PushID(ptr_id ? ptr_id : (const void*)"#TreePush");
}

void ImGui::TreePop()
{
    ImGuiState& g = GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ColumnsStartX -= g.Style.TreeNodeSpacing;
    window->DC.CursorPos.x = window->Pos.x + window->DC.ColumnsStartX + window->DC.ColumnsOffsetX;
    window->DC.TreeDepth--;
    PopID();
}

void ImGui::Value(const char* prefix, bool b)
{
    ImGui::Text("%s: %s", prefix, (b ? "true" : "false"));
}

void ImGui::Value(const char* prefix, int v)
{
    ImGui::Text("%s: %d", prefix, v);
}

void ImGui::Value(const char* prefix, unsigned int v)
{
    ImGui::Text("%s: %d", prefix, v);
}

void ImGui::Value(const char* prefix, float v, const char* float_format)
{
    if (float_format)
    {
        char fmt[64];
        sprintf(fmt, "%%s: %s", float_format);
        ImGui::Text(fmt, prefix, v);
    }
    else
    {
        ImGui::Text("%s: %.3f", prefix, v);
    }
}

void ImGui::Color(const char* prefix, const ImVec4& v)
{
    ImGui::Text("%s: (%.2f,%.2f,%.2f,%.2f)", prefix, v.x, v.y, v.z, v.w);
    ImGui::SameLine();
    ImGui::ColorButton(v, true);
}

void ImGui::Color(const char* prefix, unsigned int v)
{
    ImGui::Text("%s: %08X", prefix, v);
    ImGui::SameLine();

    ImVec4 col;
    col.x = (float)((v >> 0) & 0xFF) / 255.0f;
    col.y = (float)((v >> 8) & 0xFF) / 255.0f;
    col.z = (float)((v >> 16) & 0xFF) / 255.0f;
    col.w = (float)((v >> 24) & 0xFF) / 255.0f;
    ImGui::ColorButton(col, true);
}

//-----------------------------------------------------------------------------
// ImDrawList
//-----------------------------------------------------------------------------

void ImDrawList::Clear()
{
    commands.resize(0);
    vtx_buffer.resize(0);
    vtx_write = NULL;
    clip_rect_stack.resize(0);
}

void ImDrawList::PushClipRect(const ImVec4& clip_rect)
{
    if (!commands.empty() && commands.back().vtx_count == 0)
    {
        // Reuse empty command because high-level clipping may have discarded the other vertices already
        commands.back().clip_rect = clip_rect;
    }
    else
    {
        ImDrawCmd draw_cmd;
        draw_cmd.vtx_count = 0;
        draw_cmd.clip_rect = clip_rect;
        commands.push_back(draw_cmd);
    }
    clip_rect_stack.push_back(clip_rect);
}

void ImDrawList::PopClipRect()
{
    clip_rect_stack.pop_back();
    const ImVec4 clip_rect = clip_rect_stack.empty() ? ImVec4(-9999.0f,-9999.0f, +9999.0f, +9999.0f) : clip_rect_stack.back();
    if (!commands.empty() && commands.back().vtx_count == 0)
    {
        // Reuse empty command because high-level clipping may have discarded the other vertices already
        commands.back().clip_rect = clip_rect;
    }
    else
    {
        ImDrawCmd draw_cmd;
        draw_cmd.vtx_count = 0;
        draw_cmd.clip_rect = clip_rect;
        commands.push_back(draw_cmd);
    }
}

void ImDrawList::ReserveVertices(unsigned int vtx_count)
{
    if (vtx_count > 0)
    {
        ImDrawCmd& draw_cmd = commands.back();
        draw_cmd.vtx_count += vtx_count;
        vtx_buffer.resize(vtx_buffer.size() + vtx_count);
        vtx_write = &vtx_buffer[vtx_buffer.size() - vtx_count];
    }
}

void ImDrawList::AddVtx(const ImVec2& pos, ImU32 col)
{
    vtx_write->pos = pos;
    vtx_write->col = col;
    vtx_write->uv = GImGui.IO.FontTexUvForWhite;
    vtx_write++;
}

void ImDrawList::AddVtxLine(const ImVec2& a, const ImVec2& b, ImU32 col)
{
    const float offset = GImGui.IO.PixelCenterOffset;
    const ImVec2 hn = (b - a) * (0.50f / ImLength(b - a));     // half normal
    const ImVec2 hp0 = ImVec2(offset + hn.y, offset - hn.x);   // half perpendiculars + user offset
    const ImVec2 hp1 = ImVec2(offset - hn.y, offset + hn.x);

    // Two triangles makes up one line. Using triangles allows us to reduce amount of draw calls.
    AddVtx(a + hp0, col);
    AddVtx(b + hp0, col);
    AddVtx(a + hp1, col);
    AddVtx(b + hp0, col);
    AddVtx(b + hp1, col);
    AddVtx(a + hp1, col);
}

void ImDrawList::AddLine(const ImVec2& a, const ImVec2& b, ImU32 col)
{
    if ((col >> 24) == 0)
        return;

    ReserveVertices(6);
    AddVtxLine(a, b, col);
}

void ImDrawList::AddArc(const ImVec2& center, float rad, ImU32 col, int a_min, int a_max, bool tris, const ImVec2& third_point_offset)
{
    if ((col >> 24) == 0)
        return;

    static ImVec2 circle_vtx[12];
    static bool circle_vtx_builds = false;
    if (!circle_vtx_builds)
    {
        for (int i = 0; i < IM_ARRAYSIZE(circle_vtx); i++)
        {
            const float a = ((float)i / (float)IM_ARRAYSIZE(circle_vtx)) * 2*PI;
            circle_vtx[i].x = cosf(a + PI);
            circle_vtx[i].y = sinf(a + PI);
        }
        circle_vtx_builds = true;
    }
    
    if (tris)
    {
        ReserveVertices((unsigned int)(a_max-a_min) * 3);
        for (int a = a_min; a < a_max; a++)
        {
            AddVtx(center + circle_vtx[a % IM_ARRAYSIZE(circle_vtx)] * rad, col);
            AddVtx(center + circle_vtx[(a+1) % IM_ARRAYSIZE(circle_vtx)] * rad, col);
            AddVtx(center + third_point_offset, col);
        }
    }
    else
    {
        ReserveVertices((unsigned int)(a_max-a_min) * 6);
        for (int a = a_min; a < a_max; a++)
            AddVtxLine(center + circle_vtx[a % IM_ARRAYSIZE(circle_vtx)] * rad, center + circle_vtx[(a+1) % IM_ARRAYSIZE(circle_vtx)] * rad, col);
    }
}

void ImDrawList::AddRect(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners)
{
    if ((col >> 24) == 0)
        return;

    float r = rounding;
    r = ImMin(r, fabsf(b.x-a.x) * ( ((rounding_corners&(1|2))==(1|2)) || ((rounding_corners&(4|8))==(4|8)) ? 0.5f : 1.0f ));
    r = ImMin(r, fabsf(b.y-a.y) * ( ((rounding_corners&(1|8))==(1|8)) || ((rounding_corners&(2|4))==(2|4)) ? 0.5f : 1.0f ));

    if (r == 0.0f || rounding_corners == 0)
    {
        ReserveVertices(4*6);
        AddVtxLine(ImVec2(a.x,a.y), ImVec2(b.x,a.y), col);
        AddVtxLine(ImVec2(b.x,a.y), ImVec2(b.x,b.y), col);
        AddVtxLine(ImVec2(b.x,b.y), ImVec2(a.x,b.y), col);
        AddVtxLine(ImVec2(a.x,b.y), ImVec2(a.x,a.y), col);
    }
    else
    {
        ReserveVertices(4*6);
        AddVtxLine(ImVec2(a.x + ((rounding_corners & 1)?r:0), a.y), ImVec2(b.x - ((rounding_corners & 2)?r:0), a.y), col);
        AddVtxLine(ImVec2(b.x, a.y + ((rounding_corners & 2)?r:0)), ImVec2(b.x, b.y - ((rounding_corners & 4)?r:0)), col);
        AddVtxLine(ImVec2(b.x - ((rounding_corners & 4)?r:0), b.y), ImVec2(a.x + ((rounding_corners & 8)?r:0), b.y), col);
        AddVtxLine(ImVec2(a.x, b.y - ((rounding_corners & 8)?r:0)), ImVec2(a.x, a.y + ((rounding_corners & 1)?r:0)), col);

        if (rounding_corners & 1) AddArc(ImVec2(a.x+r,a.y+r), r, col, 0, 3);
        if (rounding_corners & 2) AddArc(ImVec2(b.x-r,a.y+r), r, col, 3, 6);
        if (rounding_corners & 4) AddArc(ImVec2(b.x-r,b.y-r), r, col, 6, 9);
        if (rounding_corners & 8) AddArc(ImVec2(a.x+r,b.y-r), r, col, 9, 12);
    }
}

void ImDrawList::AddRectFilled(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners)
{
    if ((col >> 24) == 0)
        return;

    float r = rounding;
    r = ImMin(r, fabsf(b.x-a.x) * ( ((rounding_corners&(1|2))==(1|2)) || ((rounding_corners&(4|8))==(4|8)) ? 0.5f : 1.0f ));
    r = ImMin(r, fabsf(b.y-a.y) * ( ((rounding_corners&(1|8))==(1|8)) || ((rounding_corners&(2|4))==(2|4)) ? 0.5f : 1.0f ));

    if (r == 0.0f || rounding_corners == 0)
    {
        // Use triangle so we can merge more draw calls together (at the cost of extra vertices)
        ReserveVertices(6);
        AddVtx(ImVec2(a.x,a.y), col);
        AddVtx(ImVec2(b.x,a.y), col);
        AddVtx(ImVec2(b.x,b.y), col);
        AddVtx(ImVec2(a.x,a.y), col);
        AddVtx(ImVec2(b.x,b.y), col);
        AddVtx(ImVec2(a.x,b.y), col);
    }
    else
    {
        ReserveVertices(6+6*2);
        AddVtx(ImVec2(a.x+r,a.y), col);
        AddVtx(ImVec2(b.x-r,a.y), col);
        AddVtx(ImVec2(b.x-r,b.y), col);
        AddVtx(ImVec2(a.x+r,a.y), col);
        AddVtx(ImVec2(b.x-r,b.y), col);
        AddVtx(ImVec2(a.x+r,b.y), col);
        
        float top_y = (rounding_corners & 1) ? a.y+r : a.y;
        float bot_y = (rounding_corners & 8) ? b.y-r : b.y;
        AddVtx(ImVec2(a.x,top_y), col);
        AddVtx(ImVec2(a.x+r,top_y), col);
        AddVtx(ImVec2(a.x+r,bot_y), col);
        AddVtx(ImVec2(a.x,top_y), col);
        AddVtx(ImVec2(a.x+r,bot_y), col);
        AddVtx(ImVec2(a.x,bot_y), col);

        top_y = (rounding_corners & 2) ? a.y+r : a.y;
        bot_y = (rounding_corners & 4) ? b.y-r : b.y;
        AddVtx(ImVec2(b.x-r,top_y), col);
        AddVtx(ImVec2(b.x,top_y), col);
        AddVtx(ImVec2(b.x,bot_y), col);
        AddVtx(ImVec2(b.x-r,top_y), col);
        AddVtx(ImVec2(b.x,bot_y), col);
        AddVtx(ImVec2(b.x-r,bot_y), col);

        if (rounding_corners & 1) AddArc(ImVec2(a.x+r,a.y+r), r, col, 0, 3, true);
        if (rounding_corners & 2) AddArc(ImVec2(b.x-r,a.y+r), r, col, 3, 6, true);
        if (rounding_corners & 4) AddArc(ImVec2(b.x-r,b.y-r), r, col, 6, 9, true);
        if (rounding_corners & 8) AddArc(ImVec2(a.x+r,b.y-r), r, col, 9, 12,true);
    }
}

void ImDrawList::AddTriangleFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col)
{
    if ((col >> 24) == 0)
        return;

    const ImVec2 offset(GImGui.IO.PixelCenterOffset,GImGui.IO.PixelCenterOffset);

    ReserveVertices(3);
    AddVtx(a + offset, col);
    AddVtx(b + offset, col);
    AddVtx(c + offset, col);
}

void ImDrawList::AddCircle(const ImVec2& centre, float radius, ImU32 col, int num_segments)
{
    if ((col >> 24) == 0)
        return;

    const ImVec2 offset(GImGui.IO.PixelCenterOffset,GImGui.IO.PixelCenterOffset);

    ReserveVertices((unsigned int)num_segments*6);
    const float a_step = 2*PI/(float)num_segments;
    float a0 = 0.0f;
    for (int i = 0; i < num_segments; i++)
    {
        const float a1 = (i + 1) == num_segments ? 0.0f : a0 + a_step;
        AddVtxLine(centre + offset + ImVec2(cosf(a0), sinf(a0))*radius, centre + ImVec2(cosf(a1), sinf(a1))*radius, col);
        a0 = a1;
    }
}

void ImDrawList::AddCircleFilled(const ImVec2& centre, float radius, ImU32 col, int num_segments)
{
    if ((col >> 24) == 0)
        return;

    const ImVec2 offset(GImGui.IO.PixelCenterOffset,GImGui.IO.PixelCenterOffset);

    ReserveVertices((unsigned int)num_segments*3);
    const float a_step = 2*PI/(float)num_segments;
    float a0 = 0.0f;
    for (int i = 0; i < num_segments; i++)
    {
        const float a1 = (i + 1) == num_segments ? 0.0f : a0 + a_step;
        AddVtx(centre + offset + ImVec2(cosf(a0), sinf(a0))*radius, col);
        AddVtx(centre + offset + ImVec2(cosf(a1), sinf(a1))*radius, col);
        AddVtx(centre + offset, col);
        a0 = a1;
    }
}

void ImDrawList::AddText(ImFont font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width)
{
    if ((col >> 24) == 0)
        return;

    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin); // FIXME-OPT

    // reserve vertices for worse case
    const unsigned int char_count = (unsigned int)(text_end - text_begin);
    const unsigned int vtx_count_max = char_count * 6;
    const size_t vtx_begin = vtx_buffer.size();
    ReserveVertices(vtx_count_max);

    font->RenderText(font_size, pos, col, clip_rect_stack.back(), text_begin, text_end, vtx_write, wrap_width);

    // give back unused vertices
    vtx_buffer.resize((size_t)(vtx_write - &vtx_buffer.front()));
    const size_t vtx_count = vtx_buffer.size() - vtx_begin;
    commands.back().vtx_count -= (unsigned int)(vtx_count_max - vtx_count);
    vtx_write -= (vtx_count_max - vtx_count);
}

//-----------------------------------------------------------------------------
// ImBitmapFont
//-----------------------------------------------------------------------------

ImBitmapFont::ImBitmapFont()
{
    Data = NULL;
    DataSize = 0;
    DataOwned = false;
    Info = NULL;
    Common = NULL;
    Glyphs = NULL;
    GlyphsCount = 0;
    Kerning = NULL;
    KerningCount = 0;
    TabCount = 4;
}

void    ImBitmapFont::Clear()
{
    if (Data && DataOwned)
        ImGui::MemFree(Data);
    Data = NULL;
    DataOwned = false;
    Info = NULL;
    Common = NULL;
    Glyphs = NULL;
    GlyphsCount = 0;
    Filenames.clear();
    IndexLookup.clear();
}

bool    ImBitmapFont::LoadFromFile(const char* filename)
{
    IM_ASSERT(!IsLoaded());     // Call Clear()

    // Load file
    FILE* f;
    if ((f = fopen(filename, "rb")) == NULL)
        return false;
    if (fseek(f, 0, SEEK_END)) 
    {
        fclose(f);
        return false;
    }
    const long f_size = ftell(f);
    if (f_size == -1)
    {
        fclose(f);
        return false;
    }
    DataSize = (size_t)f_size;
    if (fseek(f, 0, SEEK_SET)) 
    {
        fclose(f);
        return false;
    }
    if ((Data = (unsigned char*)ImGui::MemAlloc(DataSize)) == NULL)
    {
        fclose(f);
        return false;
    }
    if (fread(Data, 1, DataSize, f) != DataSize)
    {
        fclose(f);
        ImGui::MemFree(Data);
        return false;
    }
    fclose(f);
    DataOwned = true;
    return LoadFromMemory(Data, DataSize);
}

bool    ImBitmapFont::LoadFromMemory(const void* data, size_t data_size)
{
    IM_ASSERT(!IsLoaded());         // Call Clear()

    Data = (unsigned char*)data;
    DataSize = data_size;

    // Parse data
    if (DataSize < 4 || Data[0] != 'B' || Data[1] != 'M' || Data[2] != 'F' || Data[3] != 0x03)
        return false;
    for (const unsigned char* p = Data+4; p < Data + DataSize; )
    {
        const unsigned char block_type = *(unsigned char*)p;
        p += sizeof(unsigned char);
        ImU32 block_size;   // use memcpy to read 4-byte because they may be unaligned. This seems to break when compiling for Emscripten.
        memcpy(&block_size, p, sizeof(ImU32));
        p += sizeof(ImU32);

        switch (block_type)
        {
        case 1:
            IM_ASSERT(Info == NULL);
            Info = (FntInfo*)p;
            break;
        case 2:
            IM_ASSERT(Common == NULL);
            Common = (FntCommon*)p;
            break;
        case 3:
            for (const unsigned char* s = p; s < p+block_size && s < Data+DataSize; s = s + strlen((const char*)s) + 1)
                Filenames.push_back((const char*)s);
            break;
        case 4:
            IM_ASSERT(Glyphs == NULL && GlyphsCount == 0);
            Glyphs = (FntGlyph*)p;
            GlyphsCount = block_size / sizeof(FntGlyph);
            break;
        default:
            IM_ASSERT(Kerning == NULL && KerningCount == 0);
            Kerning = (FntKerning*)p;
            KerningCount = block_size / sizeof(FntKerning);
            break;
        }
        p += block_size;
    }

    BuildLookupTable();
    return true;
}

void ImBitmapFont::BuildLookupTable()
{
    ImU32 max_c = 0;
    for (size_t i = 0; i != GlyphsCount; i++)
        if (max_c < Glyphs[i].Id)
            max_c = Glyphs[i].Id;

    IndexLookup.clear();
    IndexLookup.resize(max_c + 1);
    for (size_t i = 0; i < IndexLookup.size(); i++)
        IndexLookup[i] = -1;
    for (size_t i = 0; i < GlyphsCount; i++)
        IndexLookup[Glyphs[i].Id] = (int)i;
}

const ImBitmapFont::FntGlyph* ImBitmapFont::FindGlyph(unsigned short c, const ImBitmapFont::FntGlyph* fallback) const
{
    if (c < (int)IndexLookup.size())
    {
        const int i = IndexLookup[c];
        if (i >= 0 && i < (int)GlyphsCount)
            return &Glyphs[i];
    }
    return fallback;
}

// Convert UTF-8 to 32-bits character, process single character input.
// Based on stb_from_utf8() from github.com/nothings/stb/
static int ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end)
{
    if (*in_text != 0)
    {
        unsigned int c = (unsigned int)-1;
        const unsigned char* str = (const unsigned char*)in_text;
        if (!(*str & 0x80))
        {
            c = (unsigned int)(*str++);
            *out_char = c;
            return 1;
        }
        if ((*str & 0xe0) == 0xc0) 
        {
            if (in_text_end && in_text_end - (const char*)str < 2) return -1;
            if (*str < 0xc2) return -1;
            c = (unsigned int)((*str++ & 0x1f) << 6);
            if ((*str & 0xc0) != 0x80) return -1;
            c += (*str++ & 0x3f);
            *out_char = c;
            return 2;
        }
        if ((*str & 0xf0) == 0xe0) 
        {
            if (in_text_end && in_text_end - (const char*)str < 3) return -1;
            if (*str == 0xe0 && (str[1] < 0xa0 || str[1] > 0xbf)) return -1;
            if (*str == 0xed && str[1] > 0x9f) return -1; // str[1] < 0x80 is checked below
            c = (unsigned int)((*str++ & 0x0f) << 12);
            if ((*str & 0xc0) != 0x80) return -1;
            c += (unsigned int)((*str++ & 0x3f) << 6);
            if ((*str & 0xc0) != 0x80) return -1;
            c += (*str++ & 0x3f);
            *out_char = c;
            return 3;
        }
        if ((*str & 0xf8) == 0xf0) 
        {
            if (in_text_end && in_text_end - (const char*)str < 4) return -1;
            if (*str > 0xf4) return -1;
            if (*str == 0xf0 && (str[1] < 0x90 || str[1] > 0xbf)) return -1;
            if (*str == 0xf4 && str[1] > 0x8f) return -1; // str[1] < 0x80 is checked below
            c = (unsigned int)((*str++ & 0x07) << 18);
            if ((*str & 0xc0) != 0x80) return -1;
            c += (unsigned int)((*str++ & 0x3f) << 12);
            if ((*str & 0xc0) != 0x80) return -1;
            c += (unsigned int)((*str++ & 0x3f) << 6);
            if ((*str & 0xc0) != 0x80) return -1;
            c += (*str++ & 0x3f);
            // utf-8 encodings of values used in surrogate pairs are invalid
            if ((c & 0xFFFFF800) == 0xD800) return -1;
            *out_char = c;
            return 4;
        }
    }
    *out_char = 0;
    return 0;
}

static ptrdiff_t ImTextStrFromUtf8(ImWchar* buf, size_t buf_size, const char* in_text, const char* in_text_end)
{
    ImWchar* buf_out = buf;
    ImWchar* buf_end = buf + buf_size;
    while (buf_out < buf_end-1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        if (c < 0x10000)    // FIXME: Losing characters that don't fit in 2 bytes
            *buf_out++ = (ImWchar)c;
    }
    *buf_out = 0;
    return buf_out - buf;
}

static int ImTextCountCharsFromUtf8(const char* in_text, const char* in_text_end)
{
    int char_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        if (c < 0x10000)
            char_count++;
    }
    return char_count;
}

// Based on stb_to_utf8() from github.com/nothings/stb/
static int ImTextCharToUtf8(char* buf, size_t buf_size, unsigned int c)
{
    if (c)
    {
        size_t i = 0;
        size_t n = buf_size;
        if (c < 0x80) 
        {
            if (i+1 > n) return 0;
            buf[i++] = (char)c;
            return 1;
        } 
        else if (c < 0x800) 
        {
            if (i+2 > n) return 0;
            buf[i++] = (char)(0xc0 + (c >> 6));
            buf[i++] = (char)(0x80 + (c & 0x3f));
            return 2;
        }
        else if (c >= 0xdc00 && c < 0xe000)
        {
            return 0;
        } 
        else if (c >= 0xd800 && c < 0xdc00) 
        {
            if (i+4 > n) return 0;
            buf[i++] = (char)(0xf0 + (c >> 18));
            buf[i++] = (char)(0x80 + ((c >> 12) & 0x3f));
            buf[i++] = (char)(0x80 + ((c >> 6) & 0x3f));
            buf[i++] = (char)(0x80 + ((c ) & 0x3f));
            return 4;
        }
        //else if (c < 0x10000)
        {
            if (i+3 > n) return 0;
            buf[i++] = (char)(0xe0 + (c >> 12));
            buf[i++] = (char)(0x80 + ((c>> 6) & 0x3f));
            buf[i++] = (char)(0x80 + ((c ) & 0x3f));
            return 3;
        }
    }
    return 0;
}

static ptrdiff_t ImTextStrToUtf8(char* buf, size_t buf_size, const ImWchar* in_text, const ImWchar* in_text_end)
{
    char* buf_out = buf;
    const char* buf_end = buf + buf_size;
    while (buf_out < buf_end-1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        buf_out += ImTextCharToUtf8(buf_out, (uintptr_t)(buf_end-buf_out-1), (unsigned int)*in_text);
        in_text++;
    }
    *buf_out = 0;
    return buf_out - buf;
}

static int ImTextCountUtf8BytesFromWchar(const ImWchar* in_text, const ImWchar* in_text_end)
{
    int bytes_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text)
    {
        char dummy[5]; // FIXME-OPT
        bytes_count += ImTextCharToUtf8(dummy, 5, (unsigned int)*in_text);
        in_text++;
    }
    return bytes_count;
}

const char* ImBitmapFont::CalcWordWrapPositionA(float scale, const char* text, const char* text_end, float wrap_width, const FntGlyph* fallback_glyph) const
{
    // Simple word-wrapping for English, not full-featured. Please submit failing cases!
    // FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world"
    // -->
    // "Hello"
    // "world"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width
    // --> 
    //  "The tr"
    //  "opical"
    //  "fish"

    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;

    const char* word_end = text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = text;
    while (s < text_end)
    {
        unsigned int c;
        const int bytes_count = ImTextCharFromUtf8(&c, s, text_end);
        const char* next_s = s + (bytes_count > 0 ? bytes_count : 1);

        if (c == '\n')
        {
            line_width = word_width = blank_width = 0.0f;
            inside_word = true;
            s = next_s;
            continue;
        }

        float char_width = 0.0f;
        if (c == '\t')
        {
            if (const FntGlyph* glyph = FindGlyph((unsigned short)' '))
                char_width = (glyph->XAdvance + Info->SpacingHoriz) * 4 * scale;
        }
        else
        {
            if (const FntGlyph* glyph = FindGlyph((unsigned short)c, fallback_glyph))
                char_width = (glyph->XAdvance + Info->SpacingHoriz) * scale;
        }

        if (c == ' ' || c == '\t')
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
            }
            blank_width += char_width;
            inside_word = false;
        }
        else
        {
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            }
            else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = !(c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width >= wrap_width)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < wrap_width)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    return s;
}

ImVec2 ImBitmapFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** remaining) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin); // FIXME-OPT

    const float scale = size / (float)Info->FontSize;
    const float line_height = (float)Info->FontSize * scale;
    const FntGlyph* fallback_glyph = FindGlyph(GImGui.IO.FontFallbackGlyph);

    ImVec2 text_size = ImVec2(0,0);
    float line_width = 0.0f;

    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    const char* s = text_begin;
    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - line_width, fallback_glyph);
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                if (text_size.x < line_width)
                    text_size.x = line_width;
                text_size.y += line_height;
                line_width = 0.0f;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (c == ' ' || c == '\t') { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source (handle unlikely UTF-8 decoding failure by skipping to the next byte)
        unsigned int c;
        const int bytes_count = ImTextCharFromUtf8(&c, s, text_end);
        s += bytes_count > 0 ? bytes_count : 1;
        
        if (c == '\n')
        {
            if (text_size.x < line_width)
                text_size.x = line_width;
            text_size.y += line_height;
            line_width = 0.0f;
            continue;
        }
        
        float char_width = 0.0f;
        if (c == '\t')
        {
            // FIXME: Better TAB handling
            if (const FntGlyph* glyph = FindGlyph((unsigned short)' '))
                char_width = (glyph->XAdvance + Info->SpacingHoriz) * 4 * scale;
        }
        else if (const FntGlyph* glyph = FindGlyph((unsigned short)c, fallback_glyph))
        {
            char_width = (glyph->XAdvance + Info->SpacingHoriz) * scale;
        }

        if (line_width + char_width >= max_width)
            break;

        line_width += char_width;
    }

    if (line_width > 0 || text_size.y == 0.0f)
    {
        if (text_size.x < line_width)
            text_size.x = line_width;
        text_size.y += line_height;
    }

    if (remaining)
        *remaining = s;

    return text_size;
}

ImVec2 ImBitmapFont::CalcTextSizeW(float size, float max_width, const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining) const
{
    if (!text_end)
        text_end = text_begin + ImStrlenW(text_begin);

    const float scale = size / (float)Info->FontSize;
    const float line_height = (float)Info->FontSize * scale;
    const FntGlyph* fallback_glyph = FindGlyph(GImGui.IO.FontFallbackGlyph);

    ImVec2 text_size = ImVec2(0,0);
    float line_width = 0.0f;

    const ImWchar* s = text_begin;
    while (s < text_end)
    {
        const unsigned int c = (unsigned int)(*s++);

        if (c == '\n')
        {
            if (text_size.x < line_width)
                text_size.x = line_width;
            text_size.y += line_height;
            line_width = 0.0f;
            continue;
        }
        
        float char_width = 0.0f;
        if (c == '\t')
        {
            // FIXME: Better TAB handling
            if (const FntGlyph* glyph = FindGlyph((unsigned short)' '))
                char_width = (glyph->XAdvance + Info->SpacingHoriz) * 4 * scale;
        }
        else
        {
            if (const FntGlyph* glyph = FindGlyph((unsigned short)c, fallback_glyph))
                char_width = (glyph->XAdvance + Info->SpacingHoriz) * scale;
        }

        if (line_width + char_width >= max_width)
            break;

        line_width += char_width;
    }

    if (line_width > 0 || text_size.y == 0.0f)
    {
        if (text_size.x < line_width)
            text_size.x = line_width;
        text_size.y += line_height;
    }

    if (remaining)
        *remaining = s;

    return text_size;
}

void ImBitmapFont::RenderText(float size, ImVec2 pos, ImU32 col, const ImVec4& clip_rect_ref, const char* text_begin, const char* text_end, ImDrawVert*& out_vertices, float wrap_width) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin);

    const float line_height = (float)Info->FontSize;
    const float scale = size / (float)Info->FontSize;
    const float tex_scale_x = 1.0f / (float)Common->ScaleW;
    const float tex_scale_y = 1.0f / (float)(Common->ScaleH);
    const float outline = (float)Info->Outline;
    const FntGlyph* fallback_glyph = FindGlyph(GImGui.IO.FontFallbackGlyph);

    // Align to be pixel perfect
    pos.x = (float)(int)pos.x;
    pos.y = (float)(int)pos.y + GImGui.IO.FontYOffset;

    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    const ImVec4 clip_rect = clip_rect_ref;
    float x = pos.x;
    float y = pos.y;

    const char* s = text_begin;
    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - pos.x), fallback_glyph);
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                x = pos.x;
                y += line_height * scale;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (c == ' ' || c == '\t') { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source (handle unlikely UTF-8 decoding failure by skipping to the next byte)
        unsigned int c;
        const int bytes_count = ImTextCharFromUtf8(&c, s, text_end);
        s += bytes_count > 0 ? bytes_count : 1;

        if (c == '\n')
        {
            x = pos.x;
            y += line_height * scale;
            continue;
        }

        float char_width = 0.0f;
        if (c == '\t')
        {
            // FIXME: Better TAB handling
            if (const FntGlyph* glyph = FindGlyph((unsigned short)' '))
                char_width += (glyph->XAdvance + Info->SpacingHoriz) * 4 * scale;
        }
        else if (const FntGlyph* glyph = FindGlyph((unsigned short)c, fallback_glyph))
        {
            char_width = (glyph->XAdvance + Info->SpacingHoriz) * scale;
            if (c != ' ')
            {
                // Clipping on Y is more likely
                const float y1 = (float)(y + (glyph->YOffset + outline*2) * scale);
                const float y2 = (float)(y1 + glyph->Height * scale);
                if (y1 <= clip_rect.w && y2 >= clip_rect.y)
                {
                    const float x1 = (float)(x + (glyph->XOffset + outline) * scale);
                    const float x2 = (float)(x1 + glyph->Width * scale);
                    if (x1 <= clip_rect.z && x2 >= clip_rect.x)
                    {
                        // Render a character
                        const float s1 = (glyph->X) * tex_scale_x;
                        const float t1 = (glyph->Y) * tex_scale_y;
                        const float s2 = (glyph->X + glyph->Width) * tex_scale_x;
                        const float t2 = (glyph->Y + glyph->Height) * tex_scale_y;

                        out_vertices[0].pos = ImVec2(x1, y1);
                        out_vertices[0].uv  = ImVec2(s1, t1);
                        out_vertices[0].col = col;

                        out_vertices[1].pos = ImVec2(x2, y1);
                        out_vertices[1].uv  = ImVec2(s2, t1);
                        out_vertices[1].col = col;

                        out_vertices[2].pos = ImVec2(x2, y2);
                        out_vertices[2].uv  = ImVec2(s2, t2);
                        out_vertices[2].col = col;

                        out_vertices[3] = out_vertices[0];
                        out_vertices[4] = out_vertices[2];

                        out_vertices[5].pos = ImVec2(x1, y2);
                        out_vertices[5].uv  = ImVec2(s1, t2);
                        out_vertices[5].col = col;

                        out_vertices += 6;
                    }
                }
            }
        }

        x += char_width;
    }
}

//-----------------------------------------------------------------------------
// PLATFORM DEPENDANT HELPERS
//-----------------------------------------------------------------------------

#if defined(_MSC_VER) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCS)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Win32 API clipboard implementation
static const char*  GetClipboardTextFn_DefaultImpl()
{
    static char* buf_local = NULL;
    if (buf_local)
    {
        ImGui::MemFree(buf_local);
        buf_local = NULL;
    }
    if (!OpenClipboard(NULL)) 
        return NULL;
    HANDLE buf_handle = GetClipboardData(CF_TEXT); 
    if (buf_handle == NULL)
        return NULL;
    if (char* buf_global = (char*)GlobalLock(buf_handle))
        buf_local = ImStrdup(buf_global);
    GlobalUnlock(buf_handle); 
    CloseClipboard(); 
    return buf_local;
}

// Win32 API clipboard implementation
static void SetClipboardTextFn_DefaultImpl(const char* text)
{
    if (!OpenClipboard(NULL))
        return;
    const char* text_end = text + strlen(text);
    const int buf_length = (int)(text_end - text) + 1;
    HGLOBAL buf_handle = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)buf_length * sizeof(char)); 
    if (buf_handle == NULL)
        return;
    char* buf_global = (char *)GlobalLock(buf_handle); 
    memcpy(buf_global, text, (size_t)(text_end - text));
    buf_global[text_end - text] = 0;
    GlobalUnlock(buf_handle); 
    EmptyClipboard();
    SetClipboardData(CF_TEXT, buf_handle);
    CloseClipboard();
}

#else

// Local ImGui-only clipboard implementation, if user hasn't defined better clipboard handlers
static const char*  GetClipboardTextFn_DefaultImpl()
{
    return GImGui.PrivateClipboard;
}

// Local ImGui-only clipboard implementation, if user hasn't defined better clipboard handlers
static void SetClipboardTextFn_DefaultImpl(const char* text)
{
    if (GImGui.PrivateClipboard)
    {
        ImGui::MemFree(GImGui.PrivateClipboard);
        GImGui.PrivateClipboard = NULL;
    }
    const char* text_end = text + strlen(text);
    GImGui.PrivateClipboard = (char*)ImGui::MemAlloc((size_t)(text_end - text) + 1);
    memcpy(GImGui.PrivateClipboard, text, (size_t)(text_end - text));
    GImGui.PrivateClipboard[(size_t)(text_end - text)] = 0;
}

#endif

//-----------------------------------------------------------------------------
// HELP
//-----------------------------------------------------------------------------

void ImGui::ShowUserGuide()
{
    ImGuiState& g = GImGui;

    ImGui::BulletText("Double-click on title bar to collapse window.");
    ImGui::BulletText("Click and drag on lower right corner to resize window.");
    ImGui::BulletText("Click and drag on any empty space to move window.");
    ImGui::BulletText("Mouse Wheel to scroll.");
    if (g.IO.FontAllowUserScaling)
        ImGui::BulletText("CTRL+Mouse Wheel to zoom window contents.");
    ImGui::BulletText("TAB/SHIFT+TAB to cycle through keyboard editable fields.");
    ImGui::BulletText("CTRL+Click on a slider to input text.");
    ImGui::BulletText(
        "While editing text:\n"
        "- Hold SHIFT or use mouse to select text\n"
        "- CTRL+Left/Right to word jump\n"
        "- CTRL+A select all\n"
        "- CTRL+X,CTRL+C,CTRL+V clipboard\n"
        "- CTRL+Z,CTRL+Y undo/redo\n"
        "- ESCAPE to revert\n"
        "- You can apply arithmetic operators +,*,/ on numerical values.\n"
        "  Use +- to subtract.\n");
}

void ImGui::ShowStyleEditor(ImGuiStyle* ref)
{
    ImGuiState& g = GImGui;
    ImGuiStyle& style = g.Style;

    const ImGuiStyle def;

    if (ImGui::Button("Revert Style"))
        g.Style = ref ? *ref : def;
    if (ref)
    {
        ImGui::SameLine();
        if (ImGui::Button("Save Style"))
            *ref = g.Style;
    }

    ImGui::PushItemWidth(ImGui::GetWindowWidth()*0.55f);

    if (ImGui::TreeNode("Sizes"))
    {
        ImGui::SliderFloat("Alpha", &style.Alpha, 0.20f, 1.0f, "%.2f");                 // Not exposing zero here so user doesn't "lose" the UI. But application code could have a toggle to switch between zero and non-zero.
        ImGui::SliderFloat2("WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 16.0f, "%.0f");
        ImGui::SliderFloat2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat2("ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat2("TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat("TreeNodeSpacing", &style.TreeNodeSpacing, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat("ScrollBarWidth", &style.ScrollBarWidth, 0.0f, 20.0f, "%.0f");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Colors"))
    {
        static ImGuiColorEditMode edit_mode = ImGuiColorEditMode_RGB;
        ImGui::RadioButton("RGB", &edit_mode, ImGuiColorEditMode_RGB);
        ImGui::SameLine();
        ImGui::RadioButton("HSV", &edit_mode, ImGuiColorEditMode_HSV);
        ImGui::SameLine();
        ImGui::RadioButton("HEX", &edit_mode, ImGuiColorEditMode_HEX);

        static ImGuiTextFilter filter;
        filter.Draw("Filter colors", 200);

        ImGui::BeginChild("#colors", ImVec2(0, 300), true);

        ImGui::ColorEditMode(edit_mode);
        for (int i = 0; i < ImGuiCol_COUNT; i++)
        {
            const char* name = GetStyleColorName(i);
            if (!filter.PassFilter(name))
                continue;
            ImGui::PushID(i);
            ImGui::ColorEdit4(name, (float*)&style.Colors[i], true);
            if (memcmp(&style.Colors[i], (ref ? &ref->Colors[i] : &def.Colors[i]), sizeof(ImVec4)) != 0)
            {
                ImGui::SameLine(); if (ImGui::Button("Revert")) style.Colors[i] = ref ? ref->Colors[i] : def.Colors[i];
                if (ref) { ImGui::SameLine(); if (ImGui::Button("Save")) ref->Colors[i] = style.Colors[i]; }
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::TreePop();
    }

    ImGui::PopItemWidth();
}

//-----------------------------------------------------------------------------
// SAMPLE CODE
//-----------------------------------------------------------------------------

static void ShowExampleAppConsole(bool* open);
static void ShowExampleAppLongText(bool* open);
static void ShowExampleAppAutoResize(bool* open);

// Demonstrate ImGui features (unfortunately this makes this function a little bloated!)
void ImGui::ShowTestWindow(bool* open)
{
    static bool no_titlebar = false;
    static bool no_border = true;
    static bool no_resize = false;
    static bool no_move = false;
    static bool no_scrollbar = false;
    static float fill_alpha = 0.65f;

    const ImGuiWindowFlags layout_flags = (no_titlebar ? ImGuiWindowFlags_NoTitleBar : 0) | (no_border ? 0 : ImGuiWindowFlags_ShowBorders) | (no_resize ? ImGuiWindowFlags_NoResize : 0) | (no_move ? ImGuiWindowFlags_NoMove : 0) | (no_scrollbar ? ImGuiWindowFlags_NoScrollbar : 0);
    ImGui::Begin("ImGui Test", open, ImVec2(550,680), fill_alpha, layout_flags);
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);

    ImGui::Text("ImGui says hello.");
    //ImGui::Text("MousePos (%g, %g)", g.IO.MousePos.x, g.IO.MousePos.y);
    //ImGui::Text("MouseWheel %d", g.IO.MouseWheel);

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Help"))
    {
        ImGui::TextWrapped("This window is being created by the ShowTestWindow() function. Please refer to the code for programming reference.\n\nUser Guide:");
        ImGui::ShowUserGuide();
    }

    if (ImGui::CollapsingHeader("Window options"))
    {
        ImGui::Checkbox("no titlebar", &no_titlebar); ImGui::SameLine(150);
        ImGui::Checkbox("no border", &no_border); ImGui::SameLine(300);
        ImGui::Checkbox("no resize", &no_resize); 
        ImGui::Checkbox("no move", &no_move); ImGui::SameLine(150);
        ImGui::Checkbox("no scrollbar", &no_scrollbar);
        ImGui::SliderFloat("fill alpha", &fill_alpha, 0.0f, 1.0f);
        if (ImGui::TreeNode("Style Editor"))
        {
            ImGui::ShowStyleEditor();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Logging"))
        {
            ImGui::LogButtons();
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("Widgets"))
    {
        static bool a=false;
        if (ImGui::Button("Button")) { printf("Clicked\n"); a ^= 1; }
        if (a)
        {
            ImGui::SameLine(); 
            ImGui::Text("Thanks for clicking me!");
        }

        if (ImGui::TreeNode("Tree"))
        {
            for (size_t i = 0; i < 5; i++)
            {
                if (ImGui::TreeNode((void*)i, "Child %d", i))
                {
                    ImGui::Text("blah blah");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("print"))
                        printf("Child %d pressed", (int)i);
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Bullets"))
        {
            ImGui::BulletText("Bullet point 1");
            ImGui::BulletText("Bullet point 2\nOn multiple lines");
            ImGui::BulletText("Bullet point 3");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Colored Text"))
        {
            // Using shortcut. You can use PushStyleColor()/PopStyleColor() for more flexibility.
            ImGui::TextColored(ImVec4(1.0f,0.0f,1.0f,1.0f), "Pink");
            ImGui::TextColored(ImVec4(1.0f,1.0f,0.0f,1.0f), "Yellow");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Word Wrapping"))
        {
            // Using shortcut. You can use PushTextWrapPos()/PopTextWrapPos() for more flexibility.
            ImGui::TextWrapped("This is a long paragraph. The text should automatically wrap on the edge of the window. The current implementation follows simple rules that works for English and possibly other languages.");
            ImGui::Spacing();

            static float wrap_width = 200.0f;
            ImGui::SliderFloat("Wrap width", &wrap_width, -20, 600, "%.0f");

            ImGui::Text("Test paragraph 1:");
            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetCursorScreenPos() + ImVec2(wrap_width, 0.0f), ImGui::GetCursorScreenPos() + ImVec2(wrap_width+10, ImGui::GetTextLineHeight()), 0xFFFF00FF);
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_width);
            ImGui::Text("lazy dog. This paragraph is made to fit within %.0f pixels. The quick brown fox jumps over the lazy dog.", wrap_width);
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemBoxMin(), ImGui::GetItemBoxMax(), 0xFF00FFFF);
            ImGui::PopTextWrapPos();

            ImGui::Text("Test paragraph 2:");
            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetCursorScreenPos() + ImVec2(wrap_width, 0.0f), ImGui::GetCursorScreenPos() + ImVec2(wrap_width+10, ImGui::GetTextLineHeight()), 0xFFFF00FF);
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_width);
            ImGui::Text("aaaaaaaa bbbbbbbb, cccccccc,dddddddd. eeeeeeee   ffffffff. gggggggg!hhhhhhhh");
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemBoxMin(), ImGui::GetItemBoxMax(), 0xFF00FFFF);
            ImGui::PopTextWrapPos();

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("UTF-8 Text"))
        {
            // UTF-8 test (need a suitable font, try extra_fonts/mplus* files for example)
            // Most compiler appears to support UTF-8 in source code (with Visual Studio you need to save your file as 'UTF-8 without signature')
            // However for the sake for maximum portability here we are *not* including raw UTF-8 character in this source file, instead we encode the string with hexadecimal constants.
            // In your own application please be reasonable and use UTF-8 in the source or get the data from external files! :)
            ImGui::TextWrapped("(CJK text will only appears if the font supports it. Please check in the extra_fonts/ folder if you intend to use non-ASCII characters. Note that characters values are preserved even if the font cannot be displayed, so you can safely copy & paste garbled characters.)");
            ImGui::Text("Hiragana: \xe3\x81\x8b\xe3\x81\x8d\xe3\x81\x8f\xe3\x81\x91\xe3\x81\x93 (kakikukeko)");
            ImGui::Text("Kanjis: \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e (nihongo)");
            static char buf[32] = "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e";
            ImGui::InputText("UTF-8 input", buf, IM_ARRAYSIZE(buf));
            ImGui::TreePop();
        }

        static bool check = true;
        ImGui::Checkbox("checkbox", &check);

        static int e = 0;
        ImGui::RadioButton("radio a", &e, 0); ImGui::SameLine();
        ImGui::RadioButton("radio b", &e, 1); ImGui::SameLine();
        ImGui::RadioButton("radio c", &e, 2);

        ImGui::Text("Hover me");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("I am a tooltip");

        ImGui::SameLine();
        ImGui::Text("- or me");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("I am a fancy tooltip");
            static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
            ImGui::PlotLines("Curve", arr, IM_ARRAYSIZE(arr));
            ImGui::EndTooltip();
        }

        ImGui::Separator();
        ImGui::Text("^ Horizontal separator");

        static int item = 1;
        ImGui::Combo("combo", &item, "aaaa\0bbbb\0cccc\0dddd\0eeee\0\0");

        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK" };
        static int item2 = -1;
        ImGui::Combo("combo scroll", &item2, items, IM_ARRAYSIZE(items));

        static char str0[128] = "Hello, world!";
        static int i0=123;
        static float f0=0.001f;
        ImGui::InputText("string", str0, IM_ARRAYSIZE(str0));
        ImGui::InputInt("input int", &i0);
        ImGui::InputFloat("input float", &f0, 0.01f, 1.0f);

        //static float vec2a[3] = { 0.10f, 0.20f };
        //ImGui::InputFloat2("input float2", vec2a);

        static float vec3a[3] = { 0.10f, 0.20f, 0.30f };
        ImGui::InputFloat3("input float3", vec3a);

        //static float vec4a[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
        //ImGui::InputFloat4("input float4", vec4a);

        static int i1=0;
        static int i2=42;
        ImGui::SliderInt("int 0..3", &i1, 0, 3);
        ImGui::SliderInt("int -100..100", &i2, -100, 100);

        static float f1=1.123f;
        static float f2=0;
        static float f3=0;
        static float f4=123456789.0f;
        ImGui::SliderFloat("float", &f1, 0.0f, 2.0f);
        ImGui::SliderFloat("log float", &f2, 0.0f, 10.0f, "%.4f", 2.0f);
        ImGui::SliderFloat("signed log float", &f3, -10.0f, 10.0f, "%.4f", 3.0f);
        ImGui::SliderFloat("unbound float", &f4, -FLT_MAX, FLT_MAX, "%.4f");
        static float angle = 0.0f;
        ImGui::SliderAngle("angle", &angle);

        //static float vec2b[3] = { 0.10f, 0.20f };
        //ImGui::SliderFloat2("slider float2", vec2b, 0.0f, 1.0f);

        static float vec3b[3] = { 0.10f, 0.20f, 0.30f };
        ImGui::SliderFloat3("slider float3", vec3b, 0.0f, 1.0f);

        //static float vec4b[4] = { 0.10f, 0.20f, 0.30f, 0.40f };
        //ImGui::SliderFloat4("slider float4", vec4b, 0.0f, 1.0f);

        static float col1[3] = { 1.0f,0.0f,0.2f };
        static float col2[4] = { 0.4f,0.7f,0.0f,0.5f };
        ImGui::ColorEdit3("color 1", col1);
        ImGui::ColorEdit4("color 2", col2);
    }

    if (ImGui::CollapsingHeader("Graphs widgets"))
    {
        static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
        ImGui::PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr));

        static bool pause;
        static ImVector<float> values; if (values.empty()) { values.resize(100); memset(&values.front(), 0, values.size()*sizeof(float)); } 
        static size_t values_offset = 0; 
        if (!pause) 
        { 
            // create dummy data at fixed 60 hz rate
            static float refresh_time = -1.0f;
            if (ImGui::GetTime() > refresh_time + 1.0f/60.0f)
            {
                refresh_time = ImGui::GetTime();
                static float phase = 0.0f;
                values[values_offset] = cosf(phase); 
                values_offset = (values_offset+1)%values.size(); 
                phase += 0.10f*values_offset; 
            }
        }
        ImGui::PlotLines("Frame Times", &values.front(), (int)values.size(), (int)values_offset, "avg 0.0", -1.0f, 1.0f, ImVec2(0,70));

        ImGui::SameLine(); ImGui::Checkbox("pause", &pause);
        ImGui::PlotHistogram("Histogram", arr, IM_ARRAYSIZE(arr), 0, NULL, 0.0f, 1.0f, ImVec2(0,70));
    }

    if (ImGui::CollapsingHeader("Widgets on same line"))
    {
        // Text
        ImGui::Text("Hello");
        ImGui::SameLine();
        ImGui::Text("World");

        // Button
        if (ImGui::Button("Banana")) printf("Pressed!\n");
        ImGui::SameLine();
        ImGui::Button("Apple");
        ImGui::SameLine();
        ImGui::Button("Corniflower");

        // Button
        ImGui::SmallButton("Banana");
        ImGui::SameLine();
        ImGui::SmallButton("Apple");
        ImGui::SameLine();
        ImGui::SmallButton("Corniflower");
        ImGui::SameLine();
        ImGui::Text("Small buttons fit in a text block");

        // Checkbox
        static bool c1=false,c2=false,c3=false,c4=false;
        ImGui::Checkbox("My", &c1);
        ImGui::SameLine();
        ImGui::Checkbox("Tailor", &c2);
        ImGui::SameLine();
        ImGui::Checkbox("Is", &c3);
        ImGui::SameLine();
        ImGui::Checkbox("Rich", &c4);

        // SliderFloat
        static float f0=1.0f, f1=2.0f, f2=3.0f;
        ImGui::PushItemWidth(80);
        ImGui::SliderFloat("f0", &f0, 0.0f,5.0f);
        ImGui::SameLine();
        ImGui::SliderFloat("f1", &f1, 0.0f,5.0f);
        ImGui::SameLine();
        ImGui::SliderFloat("f2", &f2, 0.0f,5.0f);

        // InputText
        static char s0[128] = "one", s1[128] = "two", s2[128] = "three";
        ImGui::InputText("s0", s0, 128);
        ImGui::SameLine();
        ImGui::InputText("s1", s1, 128);
        ImGui::SameLine();
        ImGui::InputText("s2", s2, 128);

        // LabelText
        ImGui::LabelText("l0", "one");
        ImGui::SameLine();
        ImGui::LabelText("l0", "two");
        ImGui::SameLine();
        ImGui::LabelText("l0", "three");
        ImGui::PopItemWidth();
    }

    if (ImGui::CollapsingHeader("Child regions"))
    {
        ImGui::Text("Without border");
        static int line = 50;
        bool goto_line = ImGui::Button("Goto");
        ImGui::SameLine(); 
        ImGui::PushItemWidth(100);
        goto_line |= ImGui::InputInt("##Line", &line, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopItemWidth();
        ImGui::BeginChild("Sub1", ImVec2(ImGui::GetWindowWidth()*0.5f,300));
        for (int i = 0; i < 100; i++)
        {
            ImGui::Text("%04d: scrollable region", i);
            if (goto_line && line == i)
                ImGui::SetScrollPosHere();
        }
        if (goto_line && line >= 100)
            ImGui::SetScrollPosHere();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Sub2", ImVec2(0,300), true);
        ImGui::Text("With border");
        ImGui::Columns(2);
        for (int i = 0; i < 100; i++)
        {
            char buf[32];
            ImFormatString(buf, IM_ARRAYSIZE(buf), "%08x", i*5731);
            ImGui::Button(buf);
            ImGui::NextColumn();
        }
        ImGui::EndChild();
    }

    if (ImGui::CollapsingHeader("Columns"))
    {
        ImGui::Columns(4, "data", true);
        ImGui::Text("ID"); ImGui::NextColumn();
        ImGui::Text("Name"); ImGui::NextColumn();
        ImGui::Text("Path"); ImGui::NextColumn();
        ImGui::Text("Flags"); ImGui::NextColumn();
        ImGui::Separator();

        ImGui::Text("0000"); ImGui::NextColumn();
        ImGui::Text("Robert"); ImGui::NextColumn();
        ImGui::Text("/path/robert"); ImGui::NextColumn();
        ImGui::Text("...."); ImGui::NextColumn();

        ImGui::Text("0001"); ImGui::NextColumn();
        ImGui::Text("Stephanie"); ImGui::NextColumn();
        ImGui::Text("/path/stephanie"); ImGui::NextColumn();
        ImGui::Text("line 1"); ImGui::Text("line 2"); ImGui::NextColumn(); // two lines, two items

        ImGui::Text("0002"); ImGui::NextColumn();
        ImGui::Text("C64"); ImGui::NextColumn();
        ImGui::Text("/path/computer"); ImGui::NextColumn();
        ImGui::Text("...."); ImGui::NextColumn();
        ImGui::Columns(1);

        ImGui::Separator();

        ImGui::Columns(3, "mixed");

        // Create multiple items in a same cell because switching to next column
        static int e = 0;
        ImGui::Text("Hello"); 
        ImGui::Button("Banana");
        ImGui::RadioButton("radio a", &e, 0); 
        ImGui::NextColumn();

        ImGui::Text("ImGui"); 
        ImGui::Button("Apple");
        ImGui::RadioButton("radio b", &e, 1);
        ImGui::Text("An extra line here.");
        ImGui::NextColumn();
        
        ImGui::Text("World!");
        ImGui::Button("Corniflower");
        ImGui::RadioButton("radio c", &e, 2);
        ImGui::NextColumn();

        if (ImGui::CollapsingHeader("Category A")) ImGui::Text("Blah blah blah"); ImGui::NextColumn();
        if (ImGui::CollapsingHeader("Category B")) ImGui::Text("Blah blah blah"); ImGui::NextColumn();
        if (ImGui::CollapsingHeader("Category C")) ImGui::Text("Blah blah blah"); ImGui::NextColumn();

        ImGui::Columns(1);

        ImGui::Separator();

        ImGui::Columns(2, "multiple components");
        static float foo = 1.0f;
        ImGui::InputFloat("red", &foo, 0.05f, 0, 3); ImGui::NextColumn();
        static float bar = 1.0f;
        ImGui::InputFloat("blue", &bar, 0.05f, 0, 3); ImGui::NextColumn();
        ImGui::Columns(1);

        ImGui::Separator();
        
        ImGui::Columns(2, "word wrapping");
        ImGui::TextWrapped("The quick brown fox jumps over the lazy dog.");
        ImGui::Text("Hello Left");
        ImGui::NextColumn();
        ImGui::TextWrapped("The quick brown fox jumps over the lazy dog.");
        ImGui::Text("Hello Right");
        ImGui::Columns(1);

        ImGui::Separator();

        if (ImGui::TreeNode("Inside a tree.."))
        {
            if (ImGui::TreeNode("node 1 (with borders)"))
            {
                ImGui::Columns(4);
                ImGui::Text("aaa"); ImGui::NextColumn();
                ImGui::Text("bbb"); ImGui::NextColumn();
                ImGui::Text("ccc"); ImGui::NextColumn();
                ImGui::Text("ddd"); ImGui::NextColumn();
                ImGui::Text("eee"); ImGui::NextColumn();
                ImGui::Text("fff"); ImGui::NextColumn();
                ImGui::Text("ggg"); ImGui::NextColumn();
                ImGui::Text("hhh"); ImGui::NextColumn();
                ImGui::Columns(1);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("node 2 (without borders)"))
            {
                ImGui::Columns(4, NULL, false);
                ImGui::Text("aaa"); ImGui::NextColumn();
                ImGui::Text("bbb"); ImGui::NextColumn();
                ImGui::Text("ccc"); ImGui::NextColumn();
                ImGui::Text("ddd"); ImGui::NextColumn();
                ImGui::Text("eee"); ImGui::NextColumn();
                ImGui::Text("fff"); ImGui::NextColumn();
                ImGui::Text("ggg"); ImGui::NextColumn();
                ImGui::Text("hhh"); ImGui::NextColumn();
                ImGui::Columns(1);
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("Filtering"))
    {
        static ImGuiTextFilter filter;
        ImGui::Text("Filter usage:\n"
                    "  \"\"         display all lines\n"
                    "  \"xxx\"      display lines containing \"xxx\"\n"
                    "  \"xxx,yyy\"  display lines containing \"xxx\" or \"yyy\"\n"
                    "  \"-xxx\"     hide lines containing \"xxx\"");
        filter.Draw();
        const char* lines[] = { "aaa1.c", "bbb1.c", "ccc1.c", "aaa2.cpp", "bbb2.cpp", "ccc2.cpp", "abc.h", "hello, world" };
        for (size_t i = 0; i < IM_ARRAYSIZE(lines); i++)
            if (filter.PassFilter(lines[i]))
                ImGui::BulletText("%s", lines[i]);
    }

    if (ImGui::CollapsingHeader("Keyboard & Focus"))
    {
        if (ImGui::TreeNode("Tabbing"))
        {
            ImGui::Text("Use TAB/SHIFT+TAB to cycle thru keyboard editable fields.");
            static char buf[32] = "dummy";
            ImGui::InputText("1", buf, IM_ARRAYSIZE(buf));
            ImGui::InputText("2", buf, IM_ARRAYSIZE(buf));
            ImGui::InputText("3", buf, IM_ARRAYSIZE(buf));
            ImGui::PushAllowKeyboardFocus(false);
            ImGui::InputText("4 (tab skip)", buf, IM_ARRAYSIZE(buf));
            //ImGui::SameLine(); ImGui::Text("(?)"); if (ImGui::IsHovered()) ImGui::SetTooltip("Use ImGui::PushAllowKeyboardFocus(bool)\nto disable tabbing through certain widgets.");
            ImGui::PopAllowKeyboardFocus();
            ImGui::InputText("5", buf, IM_ARRAYSIZE(buf));
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Focus from code"))
        {
            bool focus_1 = ImGui::Button("Focus on 1"); ImGui::SameLine();
            bool focus_2 = ImGui::Button("Focus on 2"); ImGui::SameLine();
            bool focus_3 = ImGui::Button("Focus on 3");
            int has_focus = 0;
            static char buf[128] = "click on a button to set focus";
            
            if (focus_1) ImGui::SetKeyboardFocusHere();
            ImGui::InputText("1", buf, IM_ARRAYSIZE(buf));
            if (ImGui::IsItemFocused()) has_focus = 1;
            
            if (focus_2) ImGui::SetKeyboardFocusHere();
            ImGui::InputText("2", buf, IM_ARRAYSIZE(buf));
            if (ImGui::IsItemFocused()) has_focus = 2;

            ImGui::PushAllowKeyboardFocus(false);
            if (focus_3) ImGui::SetKeyboardFocusHere();
            ImGui::InputText("3 (tab skip)", buf, IM_ARRAYSIZE(buf));
            if (ImGui::IsItemFocused()) has_focus = 3;
            ImGui::PopAllowKeyboardFocus();
            if (has_focus)
                ImGui::Text("Item with focus: %d", has_focus);
            else 
                ImGui::Text("Item with focus: <none>");
            ImGui::TreePop();
        }
    }

    static bool show_app_console = false;
    static bool show_app_long_text = false;
    static bool show_app_auto_resize = false;
    if (ImGui::CollapsingHeader("App Examples"))
    {
        ImGui::Checkbox("Console", &show_app_console);
        ImGui::Checkbox("Long text display", &show_app_long_text);
        ImGui::Checkbox("Auto-resizing window", &show_app_auto_resize);
    }
    if (show_app_console)
        ShowExampleAppConsole(&show_app_console);
    if (show_app_long_text)
        ShowExampleAppLongText(&show_app_long_text);
    if (show_app_auto_resize)
        ShowExampleAppAutoResize(&show_app_auto_resize);

    ImGui::End();
}

static void ShowExampleAppAutoResize(bool* open)
{
    if (!ImGui::Begin("Example: Auto-Resizing Window", open, ImVec2(0,0), -1.0f, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    static int lines = 10;
    ImGui::TextWrapped("Window will resize every-frame to the size of its content. Note that you don't want to query the window size to output your content because that would create a feedback loop.");
    ImGui::SliderInt("Number of lines", &lines, 1, 20);
    for (int i = 0; i < lines; i++)
        ImGui::Text("%*sThis is line %d", i*4, "", i); // Pad with space to extend size horizontally

    ImGui::End();
}

struct ExampleAppConsole
{
    ImVector<char*> Items;
    bool			NewItems;

    void	Clear()
    {
        for (size_t i = 0; i < Items.size(); i++) 
            ImGui::MemFree(Items[i]); 
        Items.clear();
        NewItems = true;
    }

    void	AddLog(const char* fmt, ...)
    {
        char buf[512];
        va_list args;
        va_start(args, fmt);
        ImFormatStringV(buf, IM_ARRAYSIZE(buf), fmt, args);
        va_end(args);
        Items.push_back(ImStrdup(buf));
        NewItems = true;
    }

    void	TextEditCallback(ImGuiTextEditCallbackData* data)
    {
		//AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
        switch (data->EventKey)
        {
        case ImGuiKey_Tab:
            {
                // Example of TEXT COMPLETION

                // Locate beginning of current word
                const char* word_end = data->Buf + data->CursorPos;
                const char* word_start = word_end;
                while (word_start > data->Buf)
                {
                    const char c = word_start[-1];
                    if (c == ' ' || c == '\t' || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                // Build a list of candidates
                const char* commands[] = { "HELP", "CLEAR", "CLASSIFY" };
                ImVector<const char*> candidates;
                for (size_t i = 0; i < IM_ARRAYSIZE(commands); i++)
                    if (ImStrnicmp(commands[i], word_start, word_end-word_start) == 0)
                        candidates.push_back(commands[i]);

                if (candidates.size() == 0)
                {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", word_end-word_start, word_start);
                }
                else if (candidates.size() == 1)
                {
                    // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
                    data->DeleteChars(word_start - data->Buf, word_end-word_start);
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                }
                else
                {
                    // Multiple matches. Complete as much as we can, so inputing "C" will complete to "CL" and display "CLEAR" and "CLASSIFY"
                    int match_len = word_end - word_start;
                    while (true)
                    {
                        char c;
                        bool all_candidates_matches = true;
                        for (size_t i = 0; i < candidates.size() && all_candidates_matches; i++)
                        {
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        }
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

					if (match_len > 0)
					{
						data->DeleteChars(word_start - data->Buf, word_end-word_start);
						data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
					}

                    // List matches
                    AddLog("Possible matches:\n");
                    for (size_t i = 0; i < candidates.size(); i++)
                        AddLog("- %s\n", candidates[i]);
                }

                break;
            }
        }
    }
};

static void ShowExampleAppConsole_TextEditCallback(ImGuiTextEditCallbackData* data)
{
    ExampleAppConsole* console = (ExampleAppConsole*)data->UserData;
    console->TextEditCallback(data);
}

static void ShowExampleAppConsole(bool* open)
{
    if (!ImGui::Begin("Example: Console", open, ImVec2(520,600)))
    {
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("This example implement a simple console. A more elaborate implementation may want to store individual entries along with extra data such as timestamp, emitter, etc.");
	ImGui::TextWrapped("Press TAB to use text completion.");

    // TODO: display from bottom
    // TODO: clip manually
    // TODO: history

    static ExampleAppConsole console;
    static char input[256] = "";

    if (ImGui::SmallButton("Add Dummy Text")) console.AddLog("some text\nsome more text");
    ImGui::SameLine(); 
    if (ImGui::SmallButton("Add Dummy Error")) console.AddLog("[error] something went wrong");
    ImGui::SameLine(); 
    if (ImGui::SmallButton("Clear all")) console.Clear();
    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
    static ImGuiTextFilter filter;
    filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
    if (ImGui::IsItemHovered()) ImGui::SetKeyboardFocusHere(-1); // Auto focus on hover
    ImGui::PopStyleVar();
    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0,-ImGui::GetTextLineSpacing()*2));

    // Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
    // NB- if you have lots of text this approach may be too inefficient. You can seek and display only the lines that are on display using a technique similar to what TextUnformatted() does,
    // or faster if your entries are already stored into a table.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1)); // tighten spacing
    ImGui::GetStyle().ItemSpacing.y = 1; // tighten spacing
    for (size_t i = 0; i < console.Items.size(); i++)
    {
        const char* item = console.Items[i];
        if (!filter.PassFilter(item))
            continue;
        ImVec4 col(1,1,1,1);
        if (strstr(item, "[error]")) col = ImVec4(1.0f,0.4f,0.4f,1.0f);
        else if (strncmp(item, "# ", 2) == 0) col = ImVec4(1.0f,0.8f,0.6f,1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(item);
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar();
    if (console.NewItems)
    {
        ImGui::SetScrollPosHere();
        console.NewItems = false;
    }
    ImGui::EndChild();

    ImGui::Separator();
    if (ImGui::InputText("Input", input, IM_ARRAYSIZE(input), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCompletion, &ShowExampleAppConsole_TextEditCallback, (void*)&console))
    {
        const char* input_trimmed_end = input+strlen(input);
        while (input_trimmed_end > input && input_trimmed_end[-1] == ' ')
            input_trimmed_end--;
        if (input_trimmed_end > input)
        {
            console.AddLog("# %s\n", input);
            console.AddLog("Unknown command: '%.*s'\n", input_trimmed_end-input, input);	// NB: we don't actually handle any command in this sample code
        }
        strcpy(input, "");
    }
    if (ImGui::IsItemHovered()) ImGui::SetKeyboardFocusHere(-1); // Auto focus on hover

    ImGui::End();
}

static void ShowExampleAppLongText(bool* open)
{
    if (!ImGui::Begin("Example: Long text display", open, ImVec2(520,600)))
    {
        ImGui::End();
        return;
    }

    static ImGuiTextBuffer log;
    static int lines = 0;
    ImGui::Text("Printing unusually long amount of text.");
    ImGui::Text("Buffer contents: %d lines, %d bytes", lines, log.size());
    if (ImGui::Button("Clear")) { log.clear(); lines = 0; }
    ImGui::SameLine();
    if (ImGui::Button("Add 1000 lines"))
    {
        for (int i = 0; i < 1000; i++)
            log.append("%i The quick brown fox jumps over the lazy dog\n", lines+i);
        lines += 1000;
    }
    ImGui::BeginChild("Log");
    ImGui::TextUnformatted(log.begin(), log.end());
    ImGui::EndChild();

    ImGui::End();
}

// End of Sample code

//-----------------------------------------------------------------------------
// Font data
// Bitmap exported from proggy_clean.fon (c) by Tristan Grimmer http://upperbounds.net/
// Also available on unofficial ProggyFonts mirror http://www.proggyfonts.net
//-----------------------------------------------------------------------------
/*
// Copyright (c) 2004, 2005 Tristan Grimmer

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
// copies of the Software, and to permit persons to whom the Software is 
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
// SOFTWARE.
*/
//-----------------------------------------------------------------------------
// Exported with bmfont (www.angelcode.com/products/bmfont), size 13, no anti-aliasing
// We are using bmfont format and you can load your own font from a file by setting up ImGui::GetIO().Font
// PNG reduced in size with pngout.exe
// Manually converted to C++ array using the following program:
/*
static void binary_to_c(const char* name_in, const char* symbol)
{
    FILE* fi = fopen(name_in, "rb"); fseek(fi, 0, SEEK_END); long sz = ftell(fi); fseek(fi, 0, SEEK_SET);
    fprintf(stdout, "static const unsigned int %s_size = %d;\n", symbol, sz);
    fprintf(stdout, "static const unsigned int %s_data[%d/4] =\n{", symbol, ((sz+3)/4)*4);
    int column = 0;
    for (unsigned int data = 0; fread(&data, 1, 4, fi); data = 0)
        if ((column++ % 12) == 0)
            fprintf(stdout, "\n    0x%08x, ", data);
        else
            fprintf(stdout, "0x%08x, ", data);
    fprintf(stdout, "\n};\n\n");
    fclose(fi);
}

int main(int argc, char** argv)
{
    binary_to_c("proggy_clean_13.fnt", "proggy_clean_13_fnt");
    binary_to_c("proggy_clean_13.png", "proggy_clean_13_png");
    return 1;
}
*/
//-----------------------------------------------------------------------------

static const unsigned int proggy_clean_13_png_size = 1557;
static const unsigned int proggy_clean_13_png_data[1560/4] =
{
    0x474e5089, 0x0a1a0a0d, 0x0d000000, 0x52444849, 0x00010000, 0x80000000, 0x00000308, 0x476bd300, 0x00000038, 0x544c5006, 0x00000045, 0xa5ffffff, 
    0x00dd9fd9, 0x74010000, 0x00534e52, 0x66d8e640, 0xbd050000, 0x54414449, 0x9bed5e78, 0x30e36e51, 0xeef5440c, 0x31fde97f, 0x584ec0f0, 0x681ace39, 
    0xca120e6b, 0x1c5a28a6, 0xc5d98a89, 0x1a3d602e, 0x323c0043, 0xf6bc9e68, 0xbe3ad62c, 0x3d60260f, 0x82d60096, 0xe0bfc707, 0xfb9bf1d1, 0xbf0267ac, 
    0x1600260f, 0x061229c0, 0x0000c183, 0x37162c58, 0xdfa088fc, 0xde7d5704, 0x77fcbb80, 0x48e5c3f1, 0x73d8b8f8, 0xc4af7802, 0x1ca111ad, 0x0001ed7a, 
    0x76eda3ef, 0xb78d3e00, 0x801c7203, 0x0215c0b1, 0x0410b044, 0xa85100d4, 0x07627ec7, 0x0cf83fa8, 0x94001a22, 0xf87347f1, 0xdcb5cfc1, 0x1c3880cc, 
    0xd4e034ca, 0xfa928d9d, 0xb0167e31, 0x325cc570, 0x4bbd584b, 0xbd4e6574, 0x70bae084, 0xf0c0008a, 0x3f601ddb, 0x0bba506a, 0xa58a0082, 0x5b46946e, 
    0x720a4ccd, 0xdfaaed39, 0x25dc8042, 0x7ee403f4, 0x2ad69cc9, 0x6c4b3009, 0x429037ed, 0x0293f875, 0x1a69dced, 0xab120198, 0x61c01d88, 0xcf2e43dc, 
    0xfc3c00ef, 0xc049a270, 0xdbbea582, 0x0d592601, 0xc3c9a8dd, 0x5013d143, 0x19a47bbb, 0xf89253dd, 0x0a9901dc, 0x38900ecd, 0xb2dec9d7, 0xc2b91230, 
    0xb8e0106f, 0x976404cb, 0x5d83c3f3, 0x6e8086fd, 0x5c9ab007, 0xf50354f6, 0xe7e72002, 0x4bc870ca, 0xab49736f, 0xc137c6e0, 0xa9aa6ff3, 0xbff84f2f, 
    0x673e6e20, 0xf6e3c7e0, 0x618fe05a, 0x39ca2a00, 0x93ca03b4, 0x3a9d2728, 0xbbebba41, 0xce0e3681, 0x6e29ec05, 0x111eca83, 0xfdfe7ec1, 0xa7c8a75b, 
    0xac6bc3ab, 0x72a5bc25, 0x9f612c1c, 0x378ec05e, 0x7202b157, 0x789e5a82, 0x5256bc0e, 0xcb900996, 0x10721105, 0x00823ce0, 0x69ab59fb, 0x39c72084, 
    0xf5e37b25, 0xd1794700, 0x538d0637, 0x9a2bff4f, 0xce0d43a4, 0xa6da7ed2, 0xd7095132, 0xf5ad6232, 0x9aaa8e9c, 0xd8d1d3ed, 0x058940a1, 0x21f00d64, 
    0x89a5c9de, 0x021b3f24, 0x77a97aac, 0x714be65a, 0x5e2d57ae, 0x27e3610f, 0x28809288, 0x36b9559f, 0xd00e347a, 0x0094e385, 0x565d034d, 0x7f52d5f2, 
    0x9aea81de, 0x5e804909, 0x010d7f0a, 0x8f0d3fb1, 0xbbce23bc, 0x375e85ac, 0x01fa03b9, 0xc0526c3a, 0xf7866870, 0x9d46d804, 0x158ebf64, 0x7bd534c5, 
    0xd80cf202, 0x410ee80f, 0x79419915, 0x74a844ae, 0x94119881, 0xcbbcc0fc, 0xa263d471, 0x013d0269, 0x67f6a0f8, 0x3e4474d0, 0xd1e50cb5, 0x56fd0e60, 
    0xc4c0fd4c, 0x940629ff, 0xe18a7a16, 0xcca0330f, 0xb8ed50b7, 0x6935778b, 0x3735c791, 0x3909eb94, 0x0be36620, 0x0ac0d7aa, 0xefe942c9, 0xf0092727, 
    0x5c020ee2, 0x0246da53, 0xa24be8bc, 0xa891ab94, 0xd012c7e2, 0x9c115954, 0xde0dac8e, 0x555dc022, 0x59e84f77, 0xbed2cf80, 0xe9af2cda, 0x4b600716, 
    0x8955bd80, 0x7098c3f3, 0x25a8466a, 0x4ddbf26a, 0x5f554753, 0xf4890f28, 0x886a27ab, 0x54a00413, 0x0a157ca9, 0x52909a80, 0x7122a312, 0x0024a75c, 
    0xe6d72935, 0xecde29cf, 0x025de009, 0x7995a6aa, 0x4a180491, 0x013df0d8, 0xe009edba, 0xd40019dc, 0x45b36b2a, 0x0122eb0d, 0x6e80a79f, 0x746590f5, 
    0xd1a6dd49, 0xc05954b6, 0x83d4b957, 0xa00fe5b1, 0x59695ad7, 0xcff8433d, 0x44a0f340, 0xdd226c73, 0x5537f08c, 0xe1e89c32, 0x431056af, 0x233eb000, 
    0x60773f40, 0xed7e490a, 0xc160091f, 0x12829db5, 0x43fbe6cf, 0x0a6b26c2, 0xd5f0f35a, 0xfc09fda8, 0x73525f8c, 0x2ea38cf9, 0x32bc410b, 0x94a60a22, 
    0x1f62a42b, 0x5f290034, 0x07beaa91, 0x1e8ccb40, 0x17d6b0f9, 0xa2a017c9, 0x4c79a610, 0xa1de6525, 0xe975029f, 0xe063585f, 0x6246cfbb, 0x04acad44, 
    0xe6a05138, 0xd03d8434, 0xc9950013, 0x5d4c809e, 0xfd26932d, 0x739213ac, 0xe260d8ef, 0xe4164617, 0x16fc60aa, 0x1d0b21e7, 0x445004b4, 0x13fd1b59, 
    0x56b0f804, 0xaa936a3a, 0x335459c1, 0xb37f8caa, 0x06b68e03, 0x14d5eb01, 0x8300c78c, 0x9674792a, 0x20ba791b, 0x4d88024d, 0xef747354, 0x451e673e, 
    0xc4dafc9a, 0xe53b9cd1, 0x32b4011a, 0x3d702c0f, 0x09bc0b40, 0x220d277d, 0x47eb7809, 0x8a946500, 0x7a28c4bd, 0x96e00f99, 0xc04365da, 0x05edcf46, 
    0x7dee2c27, 0xe6020b7f, 0x159ecedf, 0xcbdb00ff, 0x516bb9e3, 0xd0716161, 0xeba75956, 0xf17fc22b, 0x5c578beb, 0xfe474a09, 0xc1750a87, 0xe384c189, 
    0x5df54e26, 0xa6f76b79, 0xd4b172be, 0x3e8d5ceb, 0x832d90ec, 0x180368e7, 0x354c724d, 0x1a8b1412, 0x8de07be9, 0xaf009efe, 0x4616c621, 0x2860eb01, 
    0x244f1404, 0xc3de724b, 0x6497a802, 0xab2f4419, 0x4e02910d, 0xe3ecf410, 0x7a6404a8, 0x8c72b112, 0xde5bc706, 0xd4f8ffe9, 0x50176344, 0x7b49fe7d, 
    0x02c1d88c, 0x25634a40, 0x194804f7, 0x03b76d84, 0x392bde58, 0xdeebad27, 0xc160c021, 0xa97a72db, 0xa8040b83, 0x78804f3e, 0x046b9433, 0x178cc824, 
    0x62800897, 0x7010370b, 0x21cfe7e4, 0x8053ec40, 0xf9d60526, 0xae9d353f, 0x069b40c7, 0x80496f14, 0x57e682b3, 0x6e0273e0, 0x974e2e28, 0x60ab7c3d, 
    0x2025ba33, 0x507b3a8c, 0x12b70173, 0xd095c400, 0xee012d96, 0x6e194c9a, 0xe5933f89, 0x43b70102, 0xf30306aa, 0xc5802189, 0x53c077c3, 0x86029009, 
    0xa0c1e780, 0xa4c04c1f, 0x93dbd580, 0xf8149809, 0x06021893, 0x3060c183, 0x83060c18, 0x183060c1, 0xc183060c, 0x0c183060, 0x60c18306, 0xfe0c1830, 
    0x0cb69501, 0x7a40d9df, 0x000000dd, 0x4e454900, 0x6042ae44, 0x00000082, 
};

static const unsigned int proggy_clean_13_fnt_size = 4647;
static const unsigned int proggy_clean_13_fnt_data[4648/4] =
{
    0x03464d42, 0x00001a01, 0x40000d00, 0x01006400, 0x00000000, 0x50000101, 0x67676f72, 0x656c4379, 0x02006e61, 0x0000000f, 0x000a000d, 0x00800100, 
    0x01000001, 0x03000000, 0x00000016, 0x676f7270, 0x635f7967, 0x6e61656c, 0x5f33315f, 0x6e702e30, 0xd0040067, 0x00000011, 0x2e000000, 0x07000e00, 
    0x00000d00, 0x07000000, 0x010f0000, 0x36000000, 0x05003800, 0x01000d00, 0x07000000, 0x020f0000, 0x86000000, 0x07000e00, 0x00000d00, 0x07000000, 
    0x030f0000, 0x07000000, 0x06001c00, 0x01000d00, 0x07000000, 0x040f0000, 0x15000000, 0x06001c00, 0x01000d00, 0x07000000, 0x050f0000, 0x23000000, 
    0x06001c00, 0x01000d00, 0x07000000, 0x060f0000, 0x31000000, 0x06001c00, 0x01000d00, 0x07000000, 0x070f0000, 0xfc000000, 0x03003800, 0x02000d00, 
    0x07000000, 0x080f0000, 0x54000000, 0x05003800, 0x01000d00, 0x07000000, 0x090f0000, 0x4d000000, 0x06001c00, 0x01000d00, 0x07000000, 0x0a0f0000, 
    0xa8000000, 0x06001c00, 0x01000d00, 0x07000000, 0x0b0f0000, 0x6a000000, 0x04004600, 0x00000d00, 0x07000000, 0x0c0f0000, 0x74000000, 0x04004600, 
    0x00000d00, 0x07000000, 0x0d0f0000, 0x88000000, 0x04004600, 0x03000d00, 0x07000000, 0x0e0f0000, 0x65000000, 0x04004600, 0x03000d00, 0x07000000, 
    0x0f0f0000, 0x36000000, 0x07000e00, 0x00000d00, 0x07000000, 0x100f0000, 0x5a000000, 0x05003800, 0x00000d00, 0x07000000, 0x110f0000, 0x60000000, 
    0x05003800, 0x00000d00, 0x07000000, 0x120f0000, 0xe4000000, 0x03004600, 0x01000d00, 0x07000000, 0x130f0000, 0xe0000000, 0x03004600, 0x01000d00, 
    0x07000000, 0x140f0000, 0x66000000, 0x05003800, 0x00000d00, 0x07000000, 0x150f0000, 0x6c000000, 0x05003800, 0x00000d00, 0x07000000, 0x160f0000, 
    0x72000000, 0x05003800, 0x00000d00, 0x07000000, 0x170f0000, 0xd8000000, 0x03004600, 0x00000d00, 0x07000000, 0x180f0000, 0xcc000000, 0x03004600, 
    0x01000d00, 0x07000000, 0x190f0000, 0xc8000000, 0x03004600, 0x02000d00, 0x07000000, 0x1a0f0000, 0x78000000, 0x05003800, 0x00000d00, 0x07000000, 
    0x1b0f0000, 0x84000000, 0x05003800, 0x00000d00, 0x07000000, 0x1c0f0000, 0x00000000, 0x15000000, 0xf9000d00, 0x070000ff, 0x1d0f0000, 0xb0000000, 
    0x15000000, 0xf9000d00, 0x070000ff, 0x1e0f0000, 0x2c000000, 0x15000000, 0xf9000d00, 0x070000ff, 0x200f0000, 0x9a000000, 0x15000000, 0xf9000d00, 
    0x070000ff, 0x210f0000, 0x0c000000, 0x01005400, 0x03000d00, 0x07000000, 0x220f0000, 0xbc000000, 0x03004600, 0x02000d00, 0x07000000, 0x230f0000, 
    0x4e000000, 0x07000e00, 0x00000d00, 0x07000000, 0x240f0000, 0x8a000000, 0x05003800, 0x01000d00, 0x07000000, 0x250f0000, 0xa6000000, 0x07000e00, 
    0x00000d00, 0x07000000, 0x260f0000, 0xf4000000, 0x06000e00, 0x01000d00, 0x07000000, 0x270f0000, 0x06000000, 0x01005400, 0x03000d00, 0x07000000, 
    0x280f0000, 0xb8000000, 0x03004600, 0x02000d00, 0x07000000, 0x290f0000, 0xb4000000, 0x03004600, 0x02000d00, 0x07000000, 0x2a0f0000, 0x90000000, 
    0x05003800, 0x01000d00, 0x07000000, 0x2b0f0000, 0x96000000, 0x05003800, 0x01000d00, 0x07000000, 0x2c0f0000, 0xe8000000, 0x02004600, 0x01000d00, 
    0x07000000, 0x2d0f0000, 0x9c000000, 0x05003800, 0x01000d00, 0x07000000, 0x2e0f0000, 0x04000000, 0x01005400, 0x02000d00, 0x07000000, 0x2f0f0000, 
    0xa2000000, 0x05003800, 0x01000d00, 0x07000000, 0x300f0000, 0xae000000, 0x05003800, 0x01000d00, 0x07000000, 0x310f0000, 0xd8000000, 0x05003800, 
    0x01000d00, 0x07000000, 0x320f0000, 0xfa000000, 0x05000000, 0x01000d00, 0x07000000, 0x330f0000, 0x31000000, 0x05002a00, 0x01000d00, 0x07000000, 
    0x340f0000, 0x3f000000, 0x06001c00, 0x01000d00, 0x07000000, 0x350f0000, 0x37000000, 0x05002a00, 0x01000d00, 0x07000000, 0x360f0000, 0x3d000000, 
    0x05002a00, 0x01000d00, 0x07000000, 0x370f0000, 0x43000000, 0x05002a00, 0x01000d00, 0x07000000, 0x380f0000, 0x49000000, 0x05002a00, 0x01000d00, 
    0x07000000, 0x390f0000, 0x4f000000, 0x05002a00, 0x01000d00, 0x07000000, 0x3a0f0000, 0x02000000, 0x01005400, 0x03000d00, 0x07000000, 0x3b0f0000, 
    0xfa000000, 0x02004600, 0x01000d00, 0x07000000, 0x3c0f0000, 0x77000000, 0x06001c00, 0x00000d00, 0x07000000, 0x3d0f0000, 0x7e000000, 0x06001c00, 
    0x01000d00, 0x07000000, 0x3e0f0000, 0x85000000, 0x06001c00, 0x01000d00, 0x07000000, 0x3f0f0000, 0x55000000, 0x05002a00, 0x01000d00, 0x07000000, 
    0x400f0000, 0xae000000, 0x07000e00, 0x00000d00, 0x07000000, 0x410f0000, 0xe0000000, 0x06001c00, 0x01000d00, 0x07000000, 0x420f0000, 0xa1000000, 
    0x06001c00, 0x01000d00, 0x07000000, 0x430f0000, 0x5b000000, 0x05002a00, 0x01000d00, 0x07000000, 0x440f0000, 0xaf000000, 0x06001c00, 0x01000d00, 
    0x07000000, 0x450f0000, 0x61000000, 0x05002a00, 0x01000d00, 0x07000000, 0x460f0000, 0x67000000, 0x05002a00, 0x01000d00, 0x07000000, 0x470f0000, 
    0x38000000, 0x06001c00, 0x01000d00, 0x07000000, 0x480f0000, 0x8c000000, 0x06001c00, 0x01000d00, 0x07000000, 0x490f0000, 0xa0000000, 0x03004600, 
    0x02000d00, 0x07000000, 0x4a0f0000, 0x97000000, 0x04004600, 0x01000d00, 0x07000000, 0x4b0f0000, 0xb6000000, 0x06001c00, 0x01000d00, 0x07000000, 
    0x4c0f0000, 0x6d000000, 0x05002a00, 0x01000d00, 0x07000000, 0x4d0f0000, 0x1e000000, 0x07000e00, 0x00000d00, 0x07000000, 0x4e0f0000, 0x23000000, 
    0x06002a00, 0x01000d00, 0x07000000, 0x4f0f0000, 0xed000000, 0x06000e00, 0x01000d00, 0x07000000, 0x500f0000, 0x73000000, 0x05002a00, 0x01000d00, 
    0x07000000, 0x510f0000, 0x00000000, 0x06001c00, 0x01000d00, 0x07000000, 0x520f0000, 0x0e000000, 0x06001c00, 0x01000d00, 0x07000000, 0x530f0000, 
    0x1c000000, 0x06001c00, 0x01000d00, 0x07000000, 0x540f0000, 0x66000000, 0x07000e00, 0x00000d00, 0x07000000, 0x550f0000, 0x2a000000, 0x06001c00, 
    0x01000d00, 0x07000000, 0x560f0000, 0x6e000000, 0x07000e00, 0x00000d00, 0x07000000, 0x570f0000, 0x76000000, 0x07000e00, 0x00000d00, 0x07000000, 
    0x580f0000, 0x46000000, 0x06001c00, 0x01000d00, 0x07000000, 0x590f0000, 0x7e000000, 0x07000e00, 0x00000d00, 0x07000000, 0x5a0f0000, 0x54000000, 
    0x06001c00, 0x01000d00, 0x07000000, 0x5b0f0000, 0x9c000000, 0x03004600, 0x02000d00, 0x07000000, 0x5c0f0000, 0x79000000, 0x05002a00, 0x01000d00, 
    0x07000000, 0x5d0f0000, 0xdc000000, 0x03004600, 0x02000d00, 0x07000000, 0x5e0f0000, 0x7f000000, 0x05002a00, 0x01000d00, 0x07000000, 0x5f0f0000, 
    0xc6000000, 0x07000e00, 0x00000d00, 0x07000000, 0x600f0000, 0xfd000000, 0x02004600, 0x02000d00, 0x07000000, 0x610f0000, 0x85000000, 0x05002a00, 
    0x01000d00, 0x07000000, 0x620f0000, 0x8b000000, 0x05002a00, 0x01000d00, 0x07000000, 0x630f0000, 0x91000000, 0x05002a00, 0x01000d00, 0x07000000, 
    0x640f0000, 0x97000000, 0x05002a00, 0x01000d00, 0x07000000, 0x650f0000, 0x9d000000, 0x05002a00, 0x01000d00, 0x07000000, 0x660f0000, 0xa3000000, 
    0x05002a00, 0x01000d00, 0x07000000, 0x670f0000, 0xa9000000, 0x05002a00, 0x01000d00, 0x07000000, 0x680f0000, 0xaf000000, 0x05002a00, 0x01000d00, 
    0x07000000, 0x690f0000, 0xee000000, 0x02004600, 0x02000d00, 0x07000000, 0x6a0f0000, 0x92000000, 0x04004600, 0x01000d00, 0x07000000, 0x6b0f0000, 
    0xb5000000, 0x05002a00, 0x01000d00, 0x07000000, 0x6c0f0000, 0xfd000000, 0x02002a00, 0x02000d00, 0x07000000, 0x6d0f0000, 0x8e000000, 0x07000e00, 
    0x00000d00, 0x07000000, 0x6e0f0000, 0xbb000000, 0x05002a00, 0x01000d00, 0x07000000, 0x6f0f0000, 0xc1000000, 0x05002a00, 0x01000d00, 0x07000000, 
    0x700f0000, 0xc7000000, 0x05002a00, 0x01000d00, 0x07000000, 0x710f0000, 0xcd000000, 0x05002a00, 0x01000d00, 0x07000000, 0x720f0000, 0xd3000000, 
    0x05002a00, 0x01000d00, 0x07000000, 0x730f0000, 0xd9000000, 0x05002a00, 0x01000d00, 0x07000000, 0x740f0000, 0x7e000000, 0x04004600, 0x02000d00, 
    0x07000000, 0x750f0000, 0xdf000000, 0x05002a00, 0x01000d00, 0x07000000, 0x760f0000, 0xe5000000, 0x05002a00, 0x01000d00, 0x07000000, 0x770f0000, 
    0xbe000000, 0x07000e00, 0x00000d00, 0x07000000, 0x780f0000, 0xeb000000, 0x05002a00, 0x01000d00, 0x07000000, 0x790f0000, 0xf1000000, 0x05002a00, 
    0x01000d00, 0x07000000, 0x7a0f0000, 0xf7000000, 0x05002a00, 0x01000d00, 0x07000000, 0x7b0f0000, 0x00000000, 0x05003800, 0x01000d00, 0x07000000, 
    0x7c0f0000, 0x00000000, 0x01005400, 0x03000d00, 0x07000000, 0x7d0f0000, 0x06000000, 0x05003800, 0x01000d00, 0x07000000, 0x7e0f0000, 0x16000000, 
    0x07000e00, 0x00000d00, 0x07000000, 0x7f0f0000, 0x58000000, 0x15000000, 0xf9000d00, 0x070000ff, 0x810f0000, 0x16000000, 0x15000000, 0xf9000d00, 
    0x070000ff, 0x8d0f0000, 0x00000000, 0x15000e00, 0xf9000d00, 0x070000ff, 0x8f0f0000, 0xc6000000, 0x15000000, 0xf9000d00, 0x070000ff, 0x900f0000, 
    0x6e000000, 0x15000000, 0xf9000d00, 0x070000ff, 0x9d0f0000, 0x84000000, 0x15000000, 0xf9000d00, 0x070000ff, 0xa00f0000, 0xdc000000, 0x15000000, 
    0xf9000d00, 0x070000ff, 0xa10f0000, 0x0a000000, 0x01005400, 0x03000d00, 0x07000000, 0xa20f0000, 0x0c000000, 0x05003800, 0x01000d00, 0x07000000, 
    0xa30f0000, 0x12000000, 0x05003800, 0x01000d00, 0x07000000, 0xa40f0000, 0x96000000, 0x07000e00, 0x00000d00, 0x07000000, 0xa50f0000, 0x5e000000, 
    0x07000e00, 0x00000d00, 0x07000000, 0xa60f0000, 0x08000000, 0x01005400, 0x03000d00, 0x07000000, 0xa70f0000, 0x18000000, 0x05003800, 0x01000d00, 
    0x07000000, 0xa80f0000, 0xac000000, 0x03004600, 0x02000d00, 0x07000000, 0xa90f0000, 0x56000000, 0x07000e00, 0x00000d00, 0x07000000, 0xaa0f0000, 
    0x8d000000, 0x04004600, 0x01000d00, 0x07000000, 0xab0f0000, 0x1e000000, 0x05003800, 0x01000d00, 0x07000000, 0xac0f0000, 0xfb000000, 0x04000e00, 
    0x01000d00, 0x07000000, 0xad0f0000, 0x42000000, 0x15000000, 0xf9000d00, 0x070000ff, 0xae0f0000, 0x3e000000, 0x07000e00, 0x00000d00, 0x07000000, 
    0xaf0f0000, 0x26000000, 0x07000e00, 0x00000d00, 0x07000000, 0xb00f0000, 0x6f000000, 0x04004600, 0x01000d00, 0x07000000, 0xb10f0000, 0x24000000, 
    0x05003800, 0x01000d00, 0x07000000, 0xb20f0000, 0x79000000, 0x04004600, 0x01000d00, 0x07000000, 0xb30f0000, 0x83000000, 0x04004600, 0x01000d00, 
    0x07000000, 0xb40f0000, 0xeb000000, 0x02004600, 0x03000d00, 0x07000000, 0xb50f0000, 0x46000000, 0x07000e00, 0x00000d00, 0x07000000, 0xb60f0000, 
    0xe6000000, 0x06000e00, 0x01000d00, 0x07000000, 0xb70f0000, 0xc0000000, 0x03004600, 0x02000d00, 0x07000000, 0xb80f0000, 0xf7000000, 0x02004600, 
    0x03000d00, 0x07000000, 0xb90f0000, 0xc4000000, 0x03004600, 0x01000d00, 0x07000000, 0xba0f0000, 0x60000000, 0x04004600, 0x01000d00, 0x07000000, 
    0xbb0f0000, 0x2a000000, 0x05003800, 0x01000d00, 0x07000000, 0xbc0f0000, 0x1c000000, 0x06002a00, 0x01000d00, 0x07000000, 0xbd0f0000, 0xc4000000, 
    0x06001c00, 0x01000d00, 0x07000000, 0xbe0f0000, 0x9e000000, 0x07000e00, 0x00000d00, 0x07000000, 0xbf0f0000, 0x30000000, 0x05003800, 0x01000d00, 
    0x07000000, 0xc00f0000, 0x9a000000, 0x06001c00, 0x01000d00, 0x07000000, 0xc10f0000, 0x93000000, 0x06001c00, 0x01000d00, 0x07000000, 0xc20f0000, 
    0x70000000, 0x06001c00, 0x01000d00, 0x07000000, 0xc30f0000, 0x69000000, 0x06001c00, 0x01000d00, 0x07000000, 0xc40f0000, 0x62000000, 0x06001c00, 
    0x01000d00, 0x07000000, 0xc50f0000, 0x5b000000, 0x06001c00, 0x01000d00, 0x07000000, 0xc60f0000, 0xf2000000, 0x07000000, 0x00000d00, 0x07000000, 
    0xc70f0000, 0xbd000000, 0x06001c00, 0x01000d00, 0x07000000, 0xc80f0000, 0x3c000000, 0x05003800, 0x01000d00, 0x07000000, 0xc90f0000, 0x42000000, 
    0x05003800, 0x01000d00, 0x07000000, 0xca0f0000, 0x48000000, 0x05003800, 0x01000d00, 0x07000000, 0xcb0f0000, 0x4e000000, 0x05003800, 0x01000d00, 
    0x07000000, 0xcc0f0000, 0xa4000000, 0x03004600, 0x02000d00, 0x07000000, 0xcd0f0000, 0xb0000000, 0x03004600, 0x02000d00, 0x07000000, 0xce0f0000, 
    0xa8000000, 0x03004600, 0x02000d00, 0x07000000, 0xcf0f0000, 0xfc000000, 0x03001c00, 0x02000d00, 0x07000000, 0xd00f0000, 0xce000000, 0x07000e00, 
    0x00000d00, 0x07000000, 0xd10f0000, 0xcb000000, 0x06001c00, 0x01000d00, 0x07000000, 0xd20f0000, 0xd2000000, 0x06001c00, 0x01000d00, 0x07000000, 
    0xd30f0000, 0xd9000000, 0x06001c00, 0x01000d00, 0x07000000, 0xd40f0000, 0x2a000000, 0x06002a00, 0x01000d00, 0x07000000, 0xd50f0000, 0xe7000000, 
    0x06001c00, 0x01000d00, 0x07000000, 0xd60f0000, 0xee000000, 0x06001c00, 0x01000d00, 0x07000000, 0xd70f0000, 0x7e000000, 0x05003800, 0x01000d00, 
    0x07000000, 0xd80f0000, 0xf5000000, 0x06001c00, 0x01000d00, 0x07000000, 0xd90f0000, 0x00000000, 0x06002a00, 0x01000d00, 0x07000000, 0xda0f0000, 
    0x07000000, 0x06002a00, 0x01000d00, 0x07000000, 0xdb0f0000, 0x0e000000, 0x06002a00, 0x01000d00, 0x07000000, 0xdc0f0000, 0x15000000, 0x06002a00, 
    0x01000d00, 0x07000000, 0xdd0f0000, 0xd6000000, 0x07000e00, 0x00000d00, 0x07000000, 0xde0f0000, 0xa8000000, 0x05003800, 0x01000d00, 0x07000000, 
    0xdf0f0000, 0xde000000, 0x07000e00, 0x00000d00, 0x07000000, 0xe00f0000, 0xb4000000, 0x05003800, 0x01000d00, 0x07000000, 0xe10f0000, 0xba000000, 
    0x05003800, 0x01000d00, 0x07000000, 0xe20f0000, 0xc0000000, 0x05003800, 0x01000d00, 0x07000000, 0xe30f0000, 0xc6000000, 0x05003800, 0x01000d00, 
    0x07000000, 0xe40f0000, 0xcc000000, 0x05003800, 0x01000d00, 0x07000000, 0xe50f0000, 0xd2000000, 0x05003800, 0x01000d00, 0x07000000, 0xe60f0000, 
    0xb6000000, 0x07000e00, 0x00000d00, 0x07000000, 0xe70f0000, 0xde000000, 0x05003800, 0x01000d00, 0x07000000, 0xe80f0000, 0xe4000000, 0x05003800, 
    0x01000d00, 0x07000000, 0xe90f0000, 0xea000000, 0x05003800, 0x01000d00, 0x07000000, 0xea0f0000, 0xf0000000, 0x05003800, 0x01000d00, 0x07000000, 
    0xeb0f0000, 0xf6000000, 0x05003800, 0x01000d00, 0x07000000, 0xec0f0000, 0xf1000000, 0x02004600, 0x02000d00, 0x07000000, 0xed0f0000, 0xf4000000, 
    0x02004600, 0x02000d00, 0x07000000, 0xee0f0000, 0xd0000000, 0x03004600, 0x02000d00, 0x07000000, 0xef0f0000, 0xd4000000, 0x03004600, 0x02000d00, 
    0x07000000, 0xf00f0000, 0x00000000, 0x05004600, 0x01000d00, 0x07000000, 0xf10f0000, 0x06000000, 0x05004600, 0x01000d00, 0x07000000, 0xf20f0000, 
    0x0c000000, 0x05004600, 0x01000d00, 0x07000000, 0xf30f0000, 0x12000000, 0x05004600, 0x01000d00, 0x07000000, 0xf40f0000, 0x18000000, 0x05004600, 
    0x01000d00, 0x07000000, 0xf50f0000, 0x1e000000, 0x05004600, 0x01000d00, 0x07000000, 0xf60f0000, 0x24000000, 0x05004600, 0x01000d00, 0x07000000, 
    0xf70f0000, 0x2a000000, 0x05004600, 0x01000d00, 0x07000000, 0xf80f0000, 0x30000000, 0x05004600, 0x01000d00, 0x07000000, 0xf90f0000, 0x36000000, 
    0x05004600, 0x01000d00, 0x07000000, 0xfa0f0000, 0x3c000000, 0x05004600, 0x01000d00, 0x07000000, 0xfb0f0000, 0x42000000, 0x05004600, 0x01000d00, 
    0x07000000, 0xfc0f0000, 0x48000000, 0x05004600, 0x01000d00, 0x07000000, 0xfd0f0000, 0x4e000000, 0x05004600, 0x01000d00, 0x07000000, 0xfe0f0000, 
    0x54000000, 0x05004600, 0x01000d00, 0x07000000, 0xff0f0000, 0x5a000000, 0x05004600, 0x01000d00, 0x07000000, 0x000f0000, 
};

void ImGui::GetDefaultFontData(const void** fnt_data, unsigned int* fnt_size, const void** png_data, unsigned int* png_size)
{
    if (fnt_data) *fnt_data = (const void*)proggy_clean_13_fnt_data;
    if (fnt_size) *fnt_size = proggy_clean_13_fnt_size;
    if (png_data) *png_data = (const void*)proggy_clean_13_png_data;
    if (png_size) *png_size = proggy_clean_13_png_size;
}

//-----------------------------------------------------------------------------

//---- Include imgui_user.inl at the end of imgui.cpp so you can include code that extends ImGui using its private data/functions.
#ifdef IMGUI_INCLUDE_IMGUI_USER_INL
#include "imgui_user.inl"
#endif

//-----------------------------------------------------------------------------
