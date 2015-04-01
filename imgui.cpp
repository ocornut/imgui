// ImGui library v1.38 WIP
// See ImGui::ShowTestWindow() for sample code.
// Read 'Programmer guide' below for notes on how to setup ImGui in your codebase.
// Get latest version at https://github.com/ocornut/imgui
// Developed by Omar Cornut and contributors.

/*

 Index
 - MISSION STATEMENT
 - END-USER GUIDE
 - PROGRAMMER GUIDE (read me!)
 - API BREAKING CHANGES (read me when you update!)
 - FREQUENTLY ASKED QUESTIONS (FAQ), TIPS
   - Can I have multiple widgets with the same label? (Yes)
   - Why is my text output blurry?
   - How can I load a different font than the default? 
   - How can I load multiple fonts?
   - How can I display and input Chinese, Japanese, Korean characters?
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
 - portable, minimize dependencies, run on target (consoles, phones, etc.)
 - efficient runtime (NB- we do allocate when "growing" content - creating a window / opening a tree node for the first time, etc. - but a typical frame won't allocate anything)
 - read about immediate-mode gui principles @ http://mollyrocket.com/861, http://mollyrocket.com/forums/index.html

 Designed for developers and content-creators, not the typical end-user! Some of the weaknesses includes:
 - doesn't look fancy, doesn't animate
 - limited layout features, intricate layouts are typically crafted in code
 - occasionally uses statically sized buffers for string manipulations - won't crash, but some very long pieces of text may be clipped. functions like ImGui::TextUnformatted() don't have such restriction.


 END-USER GUIDE
 ==============

 - double-click title bar to collapse window
 - click upper right corner to close a window, available when 'bool* p_opened' is passed to ImGui::Begin()
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

 - read the FAQ below this section!
 - your code creates the UI, if your code doesn't run the UI is gone! == very dynamic UI, no construction/destructions steps, less data retention on your side, no state duplication, less sync, less bugs.
 - call and read ImGui::ShowTestWindow() for sample code demonstrating most features.
 - see examples/ folder for standalone sample applications. e.g. examples/opengl_example/
 - customization: PushStyleColor()/PushStyleVar() or the style editor to tweak the look of the interface (e.g. if you want a more compact UI or a different color scheme).

 - getting started:
   - initialisation: call ImGui::GetIO() to retrieve the ImGuiIO structure and fill the 'Settings' data.
   - every frame: 
      1/ in your mainloop or right after you got your keyboard/mouse info, call ImGui::GetIO() and fill the 'Input' data, then call ImGui::NewFrame().
      2/ use any ImGui function you want between NewFrame() and Render()
      3/ ImGui::Render() to render all the accumulated command-lists. it will call your RenderDrawListFn handler that you set in the IO structure.
   - all rendering information are stored into command-lists until ImGui::Render() is called. 
   - ImGui never touches or know about your GPU state. the only function that knows about GPU is the RenderDrawListFn handler that you must provide.
   - effectively it means you can create widgets at any time in your code, regardless of "update" vs "render" considerations.
   - refer to the examples applications in the examples/ folder for instruction on how to setup your code.
   - a typical application skeleton may be:

        // Application init
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x = 1920.0f;
        io.DisplaySize.y = 1280.0f;
        io.DeltaTime = 1.0f/60.0f;
        io.IniFilename = "imgui.ini";
        // TODO: Fill others settings of the io structure

        // Load texture
        unsigned char* pixels;
        int width, height, bytes_per_pixels;
        io.Fonts->GetTexDataAsRGBA32(pixels, &width, &height, &bytes_per_pixels);
        // TODO: copy texture to graphics memory. 
        // TODO: store your texture pointer/identifier in 'io.Fonts->TexID'

        // Application main loop
        while (true)
        {
            // 1) get low-level input
            // e.g. on Win32, GetKeyboardState(), or poll your events, etc.
            
            // 2) TODO: fill all fields of IO structure and call NewFrame
            ImGuiIO& io = ImGui::GetIO();
            io.MousePos = mouse_pos;
            io.MouseDown[0] = mouse_button_0;
            io.KeysDown[i] = ...
            ImGui::NewFrame();

            // 3) most of your application code here - you can use any of ImGui::* functions at any point in the frame
            ImGui::Begin("My window");
            ImGui::Text("Hello, world.");
            ImGui::End();
            GameUpdate();
            GameRender();

            // 4) render & swap video buffers
            ImGui::Render();
            // swap video buffer, etc.
        }

   - after calling ImGui::NewFrame() you can read back 'io.WantCaptureMouse' and 'io.WantCaptureKeyboard' to tell if ImGui 
     wants to use your inputs. if it does you can discard/hide the inputs from the rest of your application.

 API BREAKING CHANGES
 ====================

 Occasionally introducing changes that are breaking the API. The breakage are generally minor and easy to fix.
 Here is a change-log of API breaking changes, if you are using one of the functions listed, expect to have to fix some code.
 
 - 2015/03/17 (1.36) - renamed GetItemRectMin()/GetItemRectMax()/IsMouseHoveringBox() to GetItemRectMin()/GetItemRectMax()/IsMouseHoveringRect(). Kept inline redirection function (will obsolete).
 - 2015/03/15 (1.36) - renamed style.TreeNodeSpacing to style.IndentSpacing, ImGuiStyleVar_TreeNodeSpacing to ImGuiStyleVar_IndentSpacing
 - 2015/03/13 (1.36) - renamed GetWindowIsFocused() to IsWindowFocused(). Kept inline redirection function (will obsolete).
 - 2015/03/08 (1.35) - renamed style.ScrollBarWidth to style.ScrollbarWidth
 - 2015/02/27 (1.34) - renamed OpenNextNode(bool) to SetNextTreeNodeOpened(bool, ImGuiSetCond). Kept inline redirection function (will obsolete).
 - 2015/02/27 (1.34) - renamed ImGuiSetCondition_*** to ImGuiSetCond_***, and _FirstUseThisSession becomes _Once.
 - 2015/02/11 (1.32) - changed text input callback ImGuiTextEditCallback return type from void-->int. reserved for future use, return 0 for now.
 - 2015/02/10 (1.32) - renamed GetItemWidth() to CalcItemWidth() to clarify its evolving behavior
 - 2015/02/08 (1.31) - renamed GetTextLineSpacing() to GetTextLineHeightWithSpacing()
 - 2015/02/01 (1.31) - removed IO.MemReallocFn (unused)
 - 2015/01/19 (1.30) - renamed ImGuiStorage::GetIntPtr()/GetFloatPtr() to GetIntRef()/GetIntRef() because Ptr was conflicting with actual pointer storage functions.
 - 2015/01/11 (1.30) - big font/image API change! now loads TTF file. allow for multiple fonts. no need for a PNG loader.
              (1.30) - removed GetDefaultFontData(). uses io.Fonts->GetTextureData*() API to retrieve uncompressed pixels.
                       this sequence:
                           const void* png_data;
                           unsigned int png_size;
                           ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size);
                           // <Copy to GPU>
                       became:
                           unsigned char* pixels;
                           int width, height;
                           io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
                           // <Copy to GPU>
                           io.Fonts->TexID = (your_texture_identifier);
                       you now have much more flexibility to load multiple TTF fonts and manage the texture buffer for internal needs.
                       it is now recommended your sample the font texture with bilinear interpolation.
              (1.30) - added texture identifier in ImDrawCmd passed to your render function (we can now render images). make sure to set io.Fonts->TexID.
              (1.30) - removed IO.PixelCenterOffset (unnecessary, can be handled in user projection matrix)
              (1.30) - removed ImGui::IsItemFocused() in favor of ImGui::IsItemActive() which handles all widgets
 - 2014/12/10 (1.18) - removed SetNewWindowDefaultPos() in favor of new generic API SetNextWindowPos(pos, ImGuiSetCondition_FirstUseEver)
 - 2014/11/28 (1.17) - moved IO.Font*** options to inside the IO.Font-> structure (FontYOffset, FontTexUvForWhite, FontBaseScale, FontFallbackGlyph)
 - 2014/11/26 (1.17) - reworked syntax of IMGUI_ONCE_UPON_A_FRAME helper macro to increase compiler compatibility
 - 2014/11/07 (1.15) - renamed IsHovered() to IsItemHovered()
 - 2014/10/02 (1.14) - renamed IMGUI_INCLUDE_IMGUI_USER_CPP to IMGUI_INCLUDE_IMGUI_USER_INL and imgui_user.cpp to imgui_user.inl (more IDE friendly)
 - 2014/09/25 (1.13) - removed 'text_end' parameter from IO.SetClipboardTextFn (the string is now always zero-terminated for simplicity)
 - 2014/09/24 (1.12) - renamed SetFontScale() to SetWindowFontScale()
 - 2014/09/24 (1.12) - moved IM_MALLOC/IM_REALLOC/IM_FREE preprocessor defines to IO.MemAllocFn/IO.MemReallocFn/IO.MemFreeFn
 - 2014/08/30 (1.09) - removed IO.FontHeight (now computed automatically)
 - 2014/08/30 (1.09) - moved IMGUI_FONT_TEX_UV_FOR_WHITE preprocessor define to IO.FontTexUvForWhite
 - 2014/08/28 (1.09) - changed the behavior of IO.PixelCenterOffset following various rendering fixes


 FREQUENTLY ASKED QUESTIONS (FAQ), TIPS
 ======================================

 Q: Can I have multiple widgets with the same label? 
 A: Yes. A primer on the use of labels/IDs in ImGui..
 
   - Interactive widgets require state to be carried over multiple frames (most typically ImGui often needs to remember what is the "active" widget).
     to do so they need an unique ID. unique ID are typically derived from a string label, an integer index or a pointer.
     
       Button("OK");        // Label = "OK",     ID = hash of "OK"
       Button("Cancel");    // Label = "Cancel", ID = hash of "Cancel"

   - Elements that are not clickable, such as Text() items don't need an ID.

   - ID are uniquely scoped within windows, tree nodes, etc. so no conflict can happen if you have two buttons called "OK" in two different windows
     or in two different locations of a tree.

   - if you have a same ID twice in the same location, you'll have a conflict:

       Button("OK");
       Button("OK");        // ID collision! Both buttons will be treated as the same.

     Fear not! this is easy to solve and there are many ways to solve it!

   - when passing a label you can optionally specify extra unique ID information within string itself. This helps solving the simpler collision cases.
     use "##" to pass a complement to the ID that won't be visible to the end-user:

       Button("Play##0");   // Label = "Play",   ID = hash of "Play##0"
       Button("Play##1");   // Label = "Play",   ID = hash of "Play##1" (different from above)

   - occasionally (rarely) you might want change a label while preserving a constant ID. This allows you to animate labels.
     use "###" to pass a label that isn't part of ID:

       Button("Hello###ID"; // Label = "Hello",  ID = hash of "ID"
       Button("World###ID"; // Label = "World",  ID = hash of "ID" (same as above)

   - use PushID() / PopID() to create scopes and avoid ID conflicts within the same Window.
     this is the most convenient way of distinguish ID if you are iterating and creating many UI elements.
     you can push a pointer, a string or an integer value. remember that ID are formed from the addition of everything in the ID stack!

       for (int i = 0; i < 100; i++)
       {
         PushID(i);
         Button("Click");   // Label = "Click",  ID = hash of integer + "label" (unique)
         PopID();
       }

       for (int i = 0; i < 100; i++)
       {
         MyObject* obj = Objects[i];
         PushID(obj);
         Button("Click");   // Label = "Click",  ID = hash of pointer + "label" (unique)
         PopID();
       }

       for (int i = 0; i < 100; i++)
       {
         MyObject* obj = Objects[i];
         PushID(obj->Name);
         Button("Click");   // Label = "Click",  ID = hash of string + "label" (unique)
         PopID();
       }

   - more example showing that you can stack multiple prefixes into the ID stack:

       Button("Click");     // Label = "Click",  ID = hash of "Click"
       PushID("node");
       Button("Click");     // Label = "Click",  ID = hash of "node" + "Click"
         PushID(my_ptr);
           Button("Click"); // Label = "Click",  ID = hash of "node" + ptr + "Click"
         PopID();
       PopID();

   - tree nodes implicitly creates a scope for you by calling PushID().

       Button("Click");     // Label = "Click",  ID = hash of "Click"
       if (TreeNode("node"))
       {
         Button("Click");   // Label = "Click",  ID = hash of "node" + "Click"
         TreePop();
       }

   - when working with trees, ID are used to preserve the opened/closed state of each tree node.
     depending on your use cases you may want to use strings, indices or pointers as ID. 
      e.g. when displaying a single object that may change over time (1-1 relationship), using a static string as ID will preserve your node open/closed state when the targeted object change.
      e.g. when displaying a list of objects, using indices or pointers as ID will preserve the node open/closed state differently. experiment and see what makes more sense!

 Q: Why is my text output blurry?
 A: In your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)

 Q: How can I load a different font than the default? (default is an embedded version of ProggyClean.ttf, rendered at size 13)
 A: Use the font atlas to load the TTF file you want:

     io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels);
     io.Fonts->GetTexDataAsRGBA32() or GetTexDataAsAlpha8()

 Q: How can I load multiple fonts?
 A: Use the font atlas to pack them into a single texture:

     ImFont* font0 = io.Fonts->AddFontDefault();
     ImFont* font1 = io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels);
     ImFont* font2 = io.Fonts->AddFontFromFileTTF("myfontfile2.ttf", size_in_pixels);
     io.Fonts->GetTexDataAsRGBA32() or GetTexDataAsAlpha8()
     // the first loaded font gets used by default
     // use ImGui::PushFont()/ImGui::PopFont() to change the font at runtime

 Q: How can I render and input Chinese, Japanese, Korean characters?
 A: When loading a font, pass custom Unicode ranges to specify the glyphs to load. ImGui will support UTF-8 encoding across the board.
    Character input depends on you passing the right character code to io.AddInputCharacter(). The example applications do that.

     io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels, io.Fonts->GetGlyphRangesJapanese());  // Load Japanese characters
     io.Fonts->GetTexDataAsRGBA32() or GetTexDataAsAlpha8()
     io.ImeWindowHandle = MY_HWND;      // To input using Microsoft IME, give ImGui the hwnd of your application

 - tip: the construct 'IMGUI_ONCE_UPON_A_FRAME { ... }' will run the block of code only once a frame. You can use it to quickly add custom UI in the middle of a deep nested inner loop in your code.
 - tip: you can create widgets without a Begin()/End() block, they will go in an implicit window called "Debug"
 - tip: you can call Begin() multiple times with the same name during the same frame, it will keep appending to the same window.
 - tip: you can call Render() multiple times (e.g for VR renders).
 - tip: call and read the ShowTestWindow() code for more example of how to use ImGui!


 ISSUES & TODO-LIST
 ==================

 - misc: merge or clarify ImVec4 vs ImRect?
 - window: add horizontal scroll
 - window: fix resize grip rendering scaling along with Rounding style setting
 - window: autofit feedback loop when user relies on any dynamic layout (window width multiplier, column). maybe just clearly drop manual autofit?
 - window: add a way for very transient windows (non-saved, temporary overlay over hundreds of objects) to "clean" up from the global window list. 
 - window: allow resizing of child windows (possibly given min/max for each axis?)
 - window: background options for child windows, border option (disable rounding)
 - window: resizing from any sides? + mouse cursor directives for app.
 - widgets: display mode: widget-label, label-widget (aligned on column or using fixed size), label-newline-tab-widget etc.
 - widgets: clean up widgets internal toward exposing everything.
 - main: considering adding EndFrame()/Init(). some constructs are awkward in the implementation because of the lack of them.
 - main: IsItemHovered() make it more consistent for various type of widgets, widgets with multiple components, etc. also effectively IsHovered() region sometimes differs from hot region, e.g tree nodes
 - main: IsItemHovered() info stored in a stack? so that 'if TreeNode() { Text; TreePop; } if IsHovered' return the hover state of the TreeNode?
!- input number: large int not reliably supported because of int<>float conversions.
 - input number: optional range min/max for Input*() functions
 - input number: holding [-]/[+] buttons could increase the step speed non-linearly (or user-controlled)
 - input number: use mouse wheel to step up/down
 - input number: non-decimal input.
 - text: proper alignment options
 - layout: horizontal layout helper (github issue #97)
 - layout: more generic alignment state (left/right/centered) for single items?
 - layout: clean up the InputFloatN/SliderFloatN/ColorEdit4 layout code. item width should include frame padding.
 - columns: separator function or parameter that works within the column (currently Separator() bypass all columns)
 - columns: declare column set (each column: fixed size, %, fill, distribute default size among fills)
 - columns: columns header to act as button (~sort op) and allow resize/reorder
 - columns: user specify columns size
 - columns: tree node example, removing the last NextColumn() makes a padding difference (it should not)
 - combo: turn child handling code into pop up helper
 - combo: contents should extends to fit label if combo widget is small
 - listbox: multiple selection
 - listbox: user may want to initial scroll to focus on the one selected value?
 ! menubar, menus
 - tabs
 - gauge: various forms of gauge/loading bars widgets
 - color: better color editor.
 - plot: make it easier for user to draw extra stuff into the graph (e.g: draw basis, highlight certain points, 2d plots, multiple plots)
 - plot: "smooth" automatic scale over time, user give an input 0.0(full user scale) 1.0(full derived from value)
 - plot: add a helper e.g. Plot(char* label, float value, float time_span=2.0f) that stores values and Plot them for you - probably another function name. and/or automatically allow to plot ANY displayed value (more reliance on stable ID)
 - file selection widget -> build the tool in our codebase to improve model-dialog idioms
 - slider: allow using the [-]/[+] buttons used by InputFloat()/InputInt()
 - slider: initial absolute click is imprecise. change to relative movement slider (same as scrollbar).
 - slider: add dragging-based widgets to edit values with mouse (on 2 axises), saving screen real-estate.
 - text edit: clean up the mess caused by converting UTF-8 <> wchar. the code is rather inefficient right now.
 - text edit: centered text for slider as input text so it matches typical positioning.
 - text edit: flag to disable live update of the user buffer. 
 - text edit: field resize behavior - field could stretch when being edited? hover tooltip shows more text?
 - text edit: add multi-line text edit
 - tree: add treenode/treepush int variants? because (void*) cast from int warns on some platforms/settings
 - settings: write more decent code to allow saving/loading new fields
 - settings: api for per-tool simple persistent data (bool,int,float,columns sizes,etc.) in .ini file
 ! style: store rounded corners in texture to use 1 quad per corner (filled and wireframe). so rounding have minor cost.
 - style: checkbox: padding for "active" color should be a multiplier of the 
 - style: colorbox not always square?
 - text: simple markup language for color change?
 - log: LogButtons() options for specifying depth and/or hiding depth slider
 - log: have more control over the log scope (e.g. stop logging when leaving current tree node scope)
 - log: be able to log anything (e.g. right-click on a window/tree-node, shows context menu? log into tty/file/clipboard)
 - log: let user copy any window content to clipboard easily (CTRL+C on windows? while moving it? context menu?). code is commented because it fails with multiple Begin/End pairs.
 - filters: set a current filter that tree node can automatically query to hide themselves
 - filters: handle wildcards (with implicit leading/trailing *), regexps
 - shortcuts: add a shortcut api, e.g. parse "&Save" and/or "Save (CTRL+S)", pass in to widgets or provide simple ways to use (button=activate, input=focus)
 ! keyboard: tooltip & combo boxes are messing up / not honoring keyboard tabbing
 - keyboard: full keyboard navigation and focus.
 - input: rework IO to be able to pass actual events to fix temporal aliasing issues.
 - input: support track pad style scrolling & slider edit.
 - tooltip: move to fit within screen (e.g. when mouse cursor is right of the screen).
 - portability: big-endian test/support (github issue #81)
 - misc: mark printf compiler attributes on relevant functions
 - misc: provide a way to compile out the entire implementation while providing a dummy API (e.g. #define IMGUI_DUMMY_IMPL)
 - misc: double-clicking on title bar to minimize isn't consistent, perhaps move to single-click on left-most collapse icon?
 - style editor: have a more global HSV setter (e.g. alter hue on all elements). consider replacing active/hovered by offset in HSV space?
 - style editor: color child window height expressed in multiple of line height.
 - optimization/render: use indexed rendering to reduce vertex data cost (e.g. for remote/networked imgui)
 - optimization/render: merge command-lists with same clip-rect into one even if they aren't sequential? (as long as in-between clip rectangle don't overlap)?
 - optimization: turn some the various stack vectors into statically-sized arrays
 - optimization: better clipping for multi-component widgets
*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#include <ctype.h>      // toupper, isprint
#include <math.h>       // sqrtf, fabsf, fmodf, powf, cosf, sinf, floorf, ceilf
#include <stdint.h>     // intptr_t
#include <stdio.h>      // vsnprintf, sscanf
#include <new>          // new (ptr)

#ifdef _MSC_VER
#pragma warning (disable: 4505) // unreferenced local function has been removed (stb stuff)
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

// Clang warnings with -Weverything
#ifdef __clang__
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning : use of old-style cast                              // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"            // warning : comparing floating point with == or != is unsafe   // storing and comparing against same constants ok.
#pragma clang diagnostic ignored "-Wformat-nonliteral"      // warning : format string is not a string literal              // passing non-literal to vsnformat(). yes, user passing incorrect format strings can crash the code.
#pragma clang diagnostic ignored "-Wexit-time-destructors"  // warning : declaration requires an exit-time destructor       // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. ImGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wglobal-constructors"    // warning : declaration requires a global destructor           // similar to above, not sure what the exact difference it.
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning : implicit conversion changes signedness             // 
#pragma clang diagnostic ignored "-Wmissing-noreturn"       // warning : function xx could be declared with attribute 'noreturn' warning    // GetDefaultFontData() asserts which some implementation makes it never return.
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#endif

//-------------------------------------------------------------------------
// STB libraries implementation
//-------------------------------------------------------------------------

struct ImGuiTextEditState;

//#define IMGUI_STB_NAMESPACE     ImStb
//#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION

#ifdef IMGUI_STB_NAMESPACE
namespace IMGUI_STB_NAMESPACE
{
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

#define STBRP_ASSERT(x)    IM_ASSERT(x)
#ifndef IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#include "stb_rect_pack.h"

#define STBTT_malloc(x,u)  ((void)(u), ImGui::MemAlloc(x))
#define STBTT_free(x,u)    ((void)(u), ImGui::MemFree(x))
#define STBTT_assert(x)    IM_ASSERT(x)
#ifndef IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#endif
#include "stb_truetype.h"

#undef STB_TEXTEDIT_STRING
#undef STB_TEXTEDIT_CHARTYPE
#define STB_TEXTEDIT_STRING    ImGuiTextEditState
#define STB_TEXTEDIT_CHARTYPE  ImWchar
#include "stb_textedit.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef IMGUI_STB_NAMESPACE
} // namespace ImStb
using namespace IMGUI_STB_NAMESPACE;
#endif

//-------------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------------

struct ImRect;
struct ImGuiColMod;
struct ImGuiStyleMod;
struct ImGuiDrawContext;
struct ImGuiTextEditState;
struct ImGuiIniData;
struct ImGuiState;
struct ImGuiWindow;

static bool         ButtonBehavior(const ImRect& bb, ImGuiID id, bool* out_hovered, bool* out_held, bool allow_key_modifiers, bool repeat = false, bool pressed_on_click = false);
static void         LogText(const ImVec2& ref_pos, const char* text, const char* text_end = NULL);

static void         RenderText(ImVec2 pos, const char* text, const char* text_end = NULL, bool hide_text_after_hash = true);
static void         RenderTextWrapped(ImVec2 pos, const char* text, const char* text_end, float wrap_width);
static void         RenderTextClipped(ImVec2 pos, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& clip_max);
static void         RenderFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border = true, float rounding = 0.0f);
static void         RenderCollapseTriangle(ImVec2 p_min, bool opened, float scale = 1.0f, bool shadow = false);

static void         SetFont(ImFont* font);
static bool         ItemAdd(const ImRect& bb, const ImGuiID* id);
static void         ItemSize(ImVec2 size, float text_offset_y = 0.0f);
static void         ItemSize(const ImRect& bb, float text_offset_y = 0.0f);
static void         PushColumnClipRect(int column_index = -1);
static bool         IsClipped(const ImRect& bb);

static bool         IsMouseHoveringRect(const ImRect& bb);
static bool         IsKeyPressedMap(ImGuiKey key, bool repeat = true);

static void         Scrollbar(ImGuiWindow* window);
static bool         CloseWindowButton(bool* p_opened = NULL);
static void         FocusWindow(ImGuiWindow* window);
static ImGuiWindow* FindHoveredWindow(ImVec2 pos, bool excluding_childs);

// Helpers: String
static int          ImStricmp(const char* str1, const char* str2);
static int          ImStrnicmp(const char* str1, const char* str2, int count);
static char*        ImStrdup(const char *str);
static size_t       ImStrlenW(const ImWchar* str);
static const char*  ImStristr(const char* haystack, const char* needle, const char* needle_end);
static size_t       ImFormatString(char* buf, size_t buf_size, const char* fmt, ...);
static size_t       ImFormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args);

// Helpers: Misc
static ImU32        ImHash(const void* data, size_t data_size, ImU32 seed);
static bool         ImLoadFileToMemory(const char* filename, const char* file_open_mode, void** out_file_data, size_t* out_file_size, size_t padding_bytes = 0);
static inline int   ImUpperPowerOfTwo(int v) { v--; v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; v++; return v; }
static inline bool  ImCharIsSpace(int c) { return c == ' ' || c == '\t' || c == 0x3000; }

// Helpers: UTF-8 <> wchar
static int          ImTextCharToUtf8(char* buf, size_t buf_size, unsigned int in_char);                                // return output UTF-8 bytes count
static ptrdiff_t    ImTextStrToUtf8(char* buf, size_t buf_size, const ImWchar* in_text, const ImWchar* in_text_end);   // return output UTF-8 bytes count
static int          ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end);          // return input UTF-8 bytes count
static ptrdiff_t    ImTextStrFromUtf8(ImWchar* buf, size_t buf_size, const char* in_text, const char* in_text_end, const char** in_remaining = NULL);   // return input UTF-8 bytes count
static int          ImTextCountCharsFromUtf8(const char* in_text, const char* in_text_end);                            // return number of UTF-8 code-points (NOT bytes count)
static int          ImTextCountUtf8BytesFromStr(const ImWchar* in_text, const ImWchar* in_text_end);                   // return number of bytes to express string as UTF-8 code-points

//-----------------------------------------------------------------------------
// Platform dependent default implementations
//-----------------------------------------------------------------------------

static const char*  GetClipboardTextFn_DefaultImpl();
static void         SetClipboardTextFn_DefaultImpl(const char* text);
static void         ImeSetInputScreenPosFn_DefaultImpl(int x, int y);

//-----------------------------------------------------------------------------
// User facing structures
//-----------------------------------------------------------------------------

ImGuiStyle::ImGuiStyle()
{
    Alpha                   = 1.0f;             // Global alpha applies to everything in ImGui
    WindowPadding           = ImVec2(8,8);      // Padding within a window
    WindowMinSize           = ImVec2(32,32);    // Minimum window size
    WindowRounding          = 9.0f;             // Radius of window corners rounding. Set to 0.0f to have rectangular windows
    ChildWindowRounding     = 0.0f;             // Radius of child window corners rounding. Set to 0.0f to have rectangular windows
    FramePadding            = ImVec2(4,3);      // Padding within a framed rectangle (used by most widgets)
    FrameRounding           = 0.0f;             // Radius of frame corners rounding. Set to 0.0f to have rectangular frames (used by most widgets).
    ItemSpacing             = ImVec2(8,4);      // Horizontal and vertical spacing between widgets/lines
    ItemInnerSpacing        = ImVec2(4,4);      // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)
    TouchExtraPadding       = ImVec2(0,0);      // Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!
    AutoFitPadding          = ImVec2(8,8);      // Extra space after auto-fit (double-clicking on resize grip)
    WindowFillAlphaDefault  = 0.70f;            // Default alpha of window background, if not specified in ImGui::Begin()
    IndentSpacing           = 22.0f;            // Horizontal spacing when e.g. entering a tree node
    ColumnsMinSpacing       = 6.0f;             // Minimum horizontal spacing between two columns
    ScrollbarWidth          = 16.0f;            // Width of the vertical scrollbar
    GrabMinSize             = 10.0f;            // Minimum width/height of a slider or scrollbar grab
    DisplaySafeAreaPadding  = ImVec2(22,22);    // Window positions are clamped to be visible within the display area. If you cannot see the edge of your screen (e.g. on a TV) increase the safe area padding

    Colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    Colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    Colors[ImGuiCol_ChildWindowBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    Colors[ImGuiCol_Border]                 = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
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
    Colors[ImGuiCol_CheckActive]            = ImVec4(0.65f, 0.50f, 0.50f, 0.55f);
    Colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    Colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    Colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    Colors[ImGuiCol_Button]                 = ImVec4(0.67f, 0.40f, 0.40f, 0.60f);
    Colors[ImGuiCol_ButtonHovered]          = ImVec4(0.67f, 0.40f, 0.40f, 1.00f);
    Colors[ImGuiCol_ButtonActive]           = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    Colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
    Colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    Colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
    Colors[ImGuiCol_Column]                 = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    Colors[ImGuiCol_ColumnHovered]          = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
    Colors[ImGuiCol_ColumnActive]           = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
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

// Statically allocated font atlas. This is merely a maneuver to keep ImFontAtlas definition at the bottom of the .h file (otherwise it'd be inside ImGuiIO)
// Also we wouldn't be able to new() one at this point, before users may define IO.MemAllocFn.
static ImFontAtlas GDefaultFontAtlas;

ImGuiIO::ImGuiIO()
{
    // Most fields are initialized with zero
    memset(this, 0, sizeof(*this));

    DisplaySize = ImVec2(-1.0f, -1.0f);
    DeltaTime = 1.0f/60.0f;
    IniSavingRate = 5.0f;
    IniFilename = "imgui.ini";
    LogFilename = "imgui_log.txt";
    Fonts = &GDefaultFontAtlas;
    FontGlobalScale = 1.0f;
    MousePos = ImVec2(-1,-1);
    MousePosPrev = ImVec2(-1,-1);
    MouseDoubleClickTime = 0.30f;
    MouseDoubleClickMaxDist = 6.0f;
    MouseDragThreshold = 6.0f;
    UserData = NULL;

    // User functions
    RenderDrawListsFn = NULL;
    MemAllocFn = malloc;
    MemFreeFn = free;
    GetClipboardTextFn = GetClipboardTextFn_DefaultImpl;   // Platform dependent default implementations
    SetClipboardTextFn = SetClipboardTextFn_DefaultImpl;
    ImeSetInputScreenPosFn = ImeSetInputScreenPosFn_DefaultImpl;
}

// Pass in translated ASCII characters for text input.
// - with glfw you can get those from the callback set in glfwSetCharCallback()
// - on Windows you can get those using ToAscii+keyboard state, or via the WM_CHAR message
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

// Play it nice with Windows users. Notepad in 2015 still doesn't display text data with Unix-style \n.
#ifdef _MSC_VER
#define STR_NEWLINE "\r\n"
#else
#define STR_NEWLINE "\n"
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
static inline int    ImClamp(int v, int mn, int mx)                             { return (v < mn) ? mn : (v > mx) ? mx : v; }
static inline float  ImClamp(float v, float mn, float mx)                       { return (v < mn) ? mn : (v > mx) ? mx : v; }
static inline ImVec2 ImClamp(const ImVec2& f, const ImVec2& mn, ImVec2 mx)      { return ImVec2(ImClamp(f.x,mn.x,mx.x), ImClamp(f.y,mn.y,mx.y)); }
static inline float  ImSaturate(float f)                                        { return (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f; }
static inline float  ImLerp(float a, float b, float t)                          { return a + (b - a) * t; }
static inline ImVec2 ImLerp(const ImVec2& a, const ImVec2& b, const ImVec2& t)  { return ImVec2(a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y); }
static inline float  ImLengthSqr(const ImVec2& lhs)                             { return lhs.x*lhs.x + lhs.y*lhs.y; }

static int ImStricmp(const char* str1, const char* str2)
{
    int d;
    while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; }
    return d;
}

static int ImStrnicmp(const char* str1, const char* str2, int count)
{
    int d = 0;
    while (count > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; count--; }
    return d;
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
    while (*str++) n++;
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

// Pass data_size==0 for zero-terminated string
// Try to replace with FNV1a hash?
static ImU32 ImHash(const void* data, size_t data_size, ImU32 seed = 0) 
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

    seed = ~seed;
    ImU32 crc = seed; 
    const unsigned char* current = (const unsigned char*)data;

    if (data_size > 0)
    {
        // Known size
        while (data_size--) 
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ *current++]; 
    }
    else
    {
        // Zero-terminated string
        while (unsigned char c = *current++)
        {
            // We support a syntax of "label###id" where only "###id" is included in the hash, and only "label" gets displayed.
            // Because this syntax is rarely used we are optimizing for the common case.
            // - If we reach ### in the string we discard the hash so far and reset to the seed. 
            // - We don't do 'current += 2; continue;' after handling ### to keep the code smaller.
            if (c == '#' && current[0] == '#' && current[1] == '#')
                crc = seed;

            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        }
    }
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

ImU32 ImGui::ColorConvertFloat4ToU32(const ImVec4& in)
{
    ImU32 out  = ((ImU32)(ImSaturate(in.x)*255.f));
    out |= ((ImU32)(ImSaturate(in.y)*255.f) << 8);
    out |= ((ImU32)(ImSaturate(in.z)*255.f) << 16);
    out |= ((ImU32)(ImSaturate(in.w)*255.f) << 24);
    return out;
}

// Convert rgb floats ([0-1],[0-1],[0-1]) to hsv floats ([0-1],[0-1],[0-1]), from Foley & van Dam p592
// Optimized http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
void ImGui::ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v)
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
void ImGui::ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b)
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

// Load file content into memory
// Memory allocated with ImGui::MemAlloc(), must be freed by user using ImGui::MemFree()
static bool ImLoadFileToMemory(const char* filename, const char* file_open_mode, void** out_file_data, size_t* out_file_size, size_t padding_bytes)
{
    IM_ASSERT(filename && file_open_mode && out_file_data && out_file_size);
    *out_file_data = NULL;
    *out_file_size = 0;

    FILE* f;
    if ((f = fopen(filename, file_open_mode)) == NULL)
        return false;

    long file_size_signed;
    if (fseek(f, 0, SEEK_END) || (file_size_signed = ftell(f)) == -1 || fseek(f, 0, SEEK_SET))
    {
        fclose(f);
        return false;
    }

    size_t file_size = (size_t)file_size_signed;
    void* file_data = ImGui::MemAlloc(file_size + padding_bytes);
    if (file_data == NULL)
    {
        fclose(f);
        return false;
    }
    if (fread(file_data, 1, file_size, f) != file_size)
    {
        fclose(f);
        ImGui::MemFree(file_data);
        return false;
    }
    if (padding_bytes > 0)
        memset((void *)(((char*)file_data) + file_size), 0, padding_bytes);

    fclose(f);
    *out_file_data = file_data;
    *out_file_size = file_size;

    return true;
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

struct ImRect           // 2D axis aligned bounding-box
{
    ImVec2      Min;
    ImVec2      Max;

    ImRect()                                        { Min = ImVec2(FLT_MAX,FLT_MAX); Max = ImVec2(-FLT_MAX,-FLT_MAX); }
    ImRect(const ImVec2& min, const ImVec2& max)    { Min = min; Max = max; }
    ImRect(const ImVec4& v)                         { Min.x = v.x; Min.y = v.y; Max.x = v.z; Max.y = v.w; }
    ImRect(float x1, float y1, float x2, float y2)  { Min.x = x1; Min.y = y1; Max.x = x2; Max.y = y2; }

    ImVec2      GetCenter() const                   { return Min + (Max-Min)*0.5f; }
    ImVec2      GetSize() const                     { return Max-Min; }
    float       GetWidth() const                    { return (Max-Min).x; }
    float       GetHeight() const                   { return (Max-Min).y; }
    ImVec2      GetTL() const                       { return Min; }
    ImVec2      GetTR() const                       { return ImVec2(Max.x,Min.y); }
    ImVec2      GetBL() const                       { return ImVec2(Min.x,Max.y); }
    ImVec2      GetBR() const                       { return Max; }
    bool        Contains(const ImVec2& p) const     { return p.x >= Min.x && p.y >= Min.y && p.x < Max.x && p.y < Max.y; }
    bool        Contains(const ImRect& r) const     { return r.Min.x >= Min.x && r.Min.y >= Min.y && r.Max.x < Max.x && r.Max.y < Max.y; }
    bool        Overlaps(const ImRect& r) const     { return r.Min.y < Max.y && r.Max.y > Min.y && r.Min.x < Max.x && r.Max.x > Min.x; }
    void        Add(const ImVec2& rhs)              { Min.x = ImMin(Min.x, rhs.x); Min.y = ImMin(Min.y, rhs.y); Max.x = ImMax(Max.x, rhs.x); Max.y = ImMax(Max.x, rhs.x); }
    void        Add(const ImRect& rhs)              { Min.x = ImMin(Min.x, rhs.Min.x); Min.y = ImMin(Min.y, rhs.Min.y); Max.x = ImMax(Max.x, rhs.Max.x); Max.y = ImMax(Max.y, rhs.Max.y); }
    void        Expand(const float amount)          { Min.x -= amount; Min.y -= amount; Max.x += amount; Max.y += amount; }
    void        Expand(const ImVec2& amount)        { Min -= amount; Max += amount; }
    void        Clip(const ImRect& clip)            { Min.x = ImMax(Min.x, clip.Min.x); Min.y = ImMax(Min.y, clip.Min.y); Max.x = ImMin(Max.x, clip.Max.x); Max.y = ImMin(Max.y, clip.Max.y); }
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
typedef ImRect ImGuiAabb;   // FIXME-OBSOLETE

struct ImGuiGroupData
{
    ImVec2 BackupCursorPos;
    ImVec2 BackupCursorMaxPos;
    float  BackupColumnsStartX;
    float  BackupCurrentLineHeight;
    float  BackupCurrentLineTextBaseOffset;
    float  BackupLogLinePosY;
};

// Temporary per-window data, reset at the beginning of the frame
struct ImGuiDrawContext
{
    ImVec2                  CursorPos;
    ImVec2                  CursorPosPrevLine;
    ImVec2                  CursorStartPos;
    ImVec2                  CursorMaxPos;        // Implicitly calculate the size of our contents, always extending. Saved into window->SizeContents at the end of the frame
    float                   CurrentLineHeight;
    float                   CurrentLineTextBaseOffset;
    float                   PrevLineHeight;
    float                   PrevLineTextBaseOffset;
    float                   LogLinePosY;
    int                     TreeDepth;
    ImGuiID                 LastItemID;
    ImRect                  LastItemRect;
    bool                    LastItemHoveredAndUsable;
    bool                    LastItemHoveredRect;
    ImVector<ImGuiWindow*>  ChildWindows;
    ImVector<bool>          AllowKeyboardFocus;
    ImVector<float>         ItemWidth;           // 0.0: default, >0.0: width in pixels, <0.0: align xx pixels to the right of window
    ImVector<float>         TextWrapPos;
    ImVector<ImGuiGroupData> GroupStack;
    ImGuiColorEditMode      ColorEditMode;
    ImGuiStorage*           StateStorage;

    float                   ColumnsStartX;       // Indentation / start position from left of window (increased by TreePush/TreePop, etc.)
    float                   ColumnsOffsetX;      // Offset to the current column (if ColumnsCurrent > 0). FIXME: This and the above should be a stack to allow use cases like Tree->Column->Tree. Need revamp columns API.
    int                     ColumnsCurrent;
    int                     ColumnsCount;
    ImVec2                  ColumnsStartPos;
    float                   ColumnsCellMinY;
    float                   ColumnsCellMaxY;
    bool                    ColumnsShowBorders;
    ImGuiID                 ColumnsSetID;
    ImVector<float>         ColumnsOffsetsT;     // Columns offset normalized 0.0 (far left) -> 1.0 (far right)

    ImGuiDrawContext()
    {
        CursorPos = CursorPosPrevLine = CursorStartPos = CursorMaxPos = ImVec2(0.0f, 0.0f);
        CurrentLineHeight = PrevLineHeight = 0.0f;
        CurrentLineTextBaseOffset = PrevLineTextBaseOffset = 0.0f;
        LogLinePosY = -1.0f;
        TreeDepth = 0;
        LastItemID = 0;
        LastItemRect = ImRect(0.0f,0.0f,0.0f,0.0f);
        LastItemHoveredAndUsable = LastItemHoveredRect = false;
        ColorEditMode = ImGuiColorEditMode_RGB;
        StateStorage = NULL;

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

// Internal state of the currently focused/edited text input box
struct ImGuiTextEditState
{
    ImGuiID             Id;                             // widget id owning the text state
    ImWchar             Text[1024];                     // edit buffer, we need to persist but can't guarantee the persistence of the user-provided buffer. so we copy into own buffer.
    char                InitialText[1024*3+1];          // backup of end-user buffer at the time of focus (in UTF-8, unaltered)
    size_t              CurLenA, CurLenW;               // we need to maintain our buffer length in both UTF-8 and wchar format.
    size_t              BufSizeA;                       // end-user buffer size, <= 1024 (or increase above)
    float               Width;                          // widget width
    float               ScrollX;
    STB_TexteditState   StbState;
    float               CursorAnim;
    ImVec2              InputCursorScreenPos;           // Cursor position in screen space to be used by IME callback.
    bool                SelectedAllMouseLock;
    ImFont*             Font;
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
    static const char*      GetTextPointerClippedA(ImFont* font, float font_size, const char* text, float width, ImVec2* out_text_size = NULL);
    static const ImWchar*   GetTextPointerClippedW(ImFont* font, float font_size, const ImWchar* text, float width, ImVec2* out_text_size = NULL);
    static void             RenderTextScrolledClipped(ImFont* font, float font_size, const char* text, ImVec2 pos_base, float width, float scroll_x);
};

// Data saved in imgui.ini file
struct ImGuiIniData
{
    char*   Name;
    ImGuiID ID;
    ImVec2  Pos;
    ImVec2  Size;
    bool    Collapsed;

    ImGuiIniData() { memset(this, 0, sizeof(*this)); }
    ~ImGuiIniData() { if (Name) { ImGui::MemFree(Name); Name = NULL; } }
};

struct ImGuiMouseCursorData
{
    ImGuiMouseCursor    Type;
    ImVec2              Offset;
    ImVec2              Size;
    ImVec2              TexUvMin[2];
    ImVec2              TexUvMax[2];
};

// Main state for ImGui
struct ImGuiState
{
    bool                    Initialized;
    ImGuiIO                 IO;
    ImGuiStyle              Style;
    ImFont*                 Font;                               // (Shortcut) == FontStack.empty() ? IO.Font : FontStack.back()
    float                   FontSize;                           // (Shortcut) == FontBaseSize * g.CurrentWindow->FontWindowScale == window->FontSize()
    float                   FontBaseSize;                       // (Shortcut) == IO.FontGlobalScale * Font->Scale * Font->FontSize. Size of characters.
    ImVec2                  FontTexUvWhitePixel;                // (Shortcut) == Font->TexUvForWhite

    float                   Time;
    int                     FrameCount;
    int                     FrameCountRendered;
    ImVector<ImGuiWindow*>  Windows;
    ImVector<ImGuiWindow*>  WindowsSortBuffer;
    ImGuiWindow*            CurrentWindow;                      // Being drawn into
    ImVector<ImGuiWindow*>  CurrentWindowStack;
    ImGuiWindow*            FocusedWindow;                      // Will catch keyboard inputs
    ImGuiWindow*            HoveredWindow;                      // Will catch mouse inputs
    ImGuiWindow*            HoveredRootWindow;                  // Will catch mouse inputs (for focus/move only)
    ImGuiID                 HoveredId;
    ImGuiID                 ActiveId;
    ImGuiID                 ActiveIdPreviousFrame;
    bool                    ActiveIdIsAlive;
    bool                    ActiveIdIsFocusedOnly;              // Set only by active widget. Denote focus but no active interaction.
    ImGuiWindow*            MovedWindow;                        // Track the child window we clicked on to move a window. Only valid if ActiveID is the "#MOVE" identifier of a window.
    float                   SettingsDirtyTimer;
    ImVector<ImGuiIniData*> Settings;
    ImVector<ImGuiColMod>   ColorModifiers;
    ImVector<ImGuiStyleMod> StyleModifiers;
    ImVector<ImFont*>       FontStack;

    ImVec2                  SetNextWindowPosVal;
    ImGuiSetCond            SetNextWindowPosCond;
    ImVec2                  SetNextWindowSizeVal;
    ImGuiSetCond            SetNextWindowSizeCond;
    bool                    SetNextWindowCollapsedVal;
    ImGuiSetCond            SetNextWindowCollapsedCond;
    bool                    SetNextWindowFocus;
    bool                    SetNextTreeNodeOpenedVal;
    ImGuiSetCond            SetNextTreeNodeOpenedCond;

    // Render
    ImVector<ImDrawList*>   RenderDrawLists[3];

    // Mouse cursor
    ImGuiMouseCursor        MouseCursor;
    ImDrawList              MouseCursorDrawList;                // Optional software render of mouse cursors, if io.MouseDrawCursor is set
    ImGuiMouseCursorData    MouseCursorData[ImGuiMouseCursor_Count_];

    // Widget state
    ImGuiTextEditState      InputTextState;
    ImGuiID                 SliderAsInputTextId;
    ImGuiStorage            ColorEditModeStorage;               // for user selection
    ImGuiID                 ActiveComboID;
    float                   ScrollbarClickDeltaToGrabCenter;    // distance between mouse and center of grab box, normalized in parent space
    char                    Tooltip[1024];
    char*                   PrivateClipboard;                   // if no custom clipboard handler is defined

    // Logging
    bool                    LogEnabled;
    FILE*                   LogFile;
    ImGuiTextBuffer*        LogClipboard;                       // pointer so our GImGui static constructor doesn't call heap allocators.
    int                     LogStartDepth;
    int                     LogAutoExpandMaxDepth;

    // Misc
    float                   FramerateSecPerFrame[120];          // calculate estimate of framerate for user
    int                     FramerateSecPerFrameIdx;
    float                   FramerateSecPerFrameAccum;
    char                    TempBuffer[1024*3+1];               // temporary text buffer

    ImGuiState()
    {
        Initialized = false;
        Font = NULL;
        FontBaseSize = FontSize = 0.0f;
        FontTexUvWhitePixel = ImVec2(0.0f, 0.0f);

        Time = 0.0f;
        FrameCount = 0;
        FrameCountRendered = -1;
        CurrentWindow = NULL;
        FocusedWindow = NULL;
        HoveredWindow = NULL;
        HoveredRootWindow = NULL;
        HoveredId = 0;
        ActiveId = 0;
        ActiveIdPreviousFrame = 0;
        ActiveIdIsAlive = false;
        ActiveIdIsFocusedOnly = false;
        MovedWindow = NULL;
        SettingsDirtyTimer = 0.0f;

        SetNextWindowPosVal = ImVec2(0.0f, 0.0f);
        SetNextWindowPosCond = 0;
        SetNextWindowSizeVal = ImVec2(0.0f, 0.0f);
        SetNextWindowSizeCond = 0;
        SetNextWindowCollapsedVal = false;
        SetNextWindowCollapsedCond = 0;
        SetNextWindowFocus = false;
        SetNextTreeNodeOpenedVal = false;
        SetNextTreeNodeOpenedCond = 0;

        SliderAsInputTextId = 0;
        ActiveComboID = 0;
        ScrollbarClickDeltaToGrabCenter = 0.0f;
        memset(Tooltip, 0, sizeof(Tooltip));
        PrivateClipboard = NULL;

        MouseCursor = ImGuiMouseCursor_Arrow;

        LogEnabled = false;
        LogFile = NULL;
        LogClipboard = NULL;
        LogStartDepth = 0;
        LogAutoExpandMaxDepth = 2;

        memset(FramerateSecPerFrame, 0, sizeof(FramerateSecPerFrame));
        FramerateSecPerFrameIdx = 0;
        FramerateSecPerFrameAccum = 0.0f;
    }
};

static ImGuiState   GImDefaultState;                            // Internal state storage
static ImGuiState*  GImGui = &GImDefaultState;                  // We access everything through this pointer. NB: this pointer is always assumed to be != NULL

struct ImGuiWindow
{
    char*                   Name;
    ImGuiID                 ID;
    ImGuiWindowFlags        Flags;
    ImVec2                  PosFloat;
    ImVec2                  Pos;                                // Position rounded-up to nearest pixel
    ImVec2                  Size;                               // Current size (==SizeFull or collapsed title bar size)
    ImVec2                  SizeFull;                           // Size when non collapsed
    ImVec2                  SizeContents;                       // Size of contents (== extents reach of the drawing cursor) from previous frame
    ImGuiID                 MoveID;                             // == window->GetID("#MOVE")
    float                   ScrollY;
    float                   NextScrollY;
    bool                    ScrollbarY;
    bool                    Visible;                            // Set to true on Begin()
    bool                    WasVisible;
    bool                    Accessed;                           // Set to true when any widget access the current window
    bool                    Collapsed;                          // Set when collapsing window to become only title-bar
    bool                    SkipItems;                          // == Visible && !Collapsed
    int                     AutoFitFrames;
    bool                    AutoFitOnlyGrows;
    int                     SetWindowPosAllowFlags;             // bit ImGuiSetCond_*** specify if SetWindowPos() call will succeed with this particular flag. 
    int                     SetWindowSizeAllowFlags;            // bit ImGuiSetCond_*** specify if SetWindowSize() call will succeed with this particular flag. 
    int                     SetWindowCollapsedAllowFlags;       // bit ImGuiSetCond_*** specify if SetWindowCollapsed() call will succeed with this particular flag. 

    ImGuiDrawContext        DC;                                 // Temporary per-window data, reset at the beginning of the frame
    ImVector<ImGuiID>       IDStack;                            // ID stack. ID are hashes seeded with the value at the top of the stack
    ImVector<ImVec4>        ClipRectStack;                      // Scissoring / clipping rectangle. x1, y1, x2, y2.
    ImRect                  ClippedRect;                        // = ClipRectStack.front() after setup in Begin()
    int                     LastFrameDrawn;
    float                   ItemWidthDefault;
    ImGuiStorage            StateStorage;
    float                   FontWindowScale;                    // Scale multiplier per-window
    ImDrawList*             DrawList;
    ImGuiWindow*            RootWindow;

    // Focus
    int                     FocusIdxAllCounter;                 // Start at -1 and increase as assigned via FocusItemRegister()
    int                     FocusIdxTabCounter;                 // (same, but only count widgets which you can Tab through)
    int                     FocusIdxAllRequestCurrent;          // Item being requested for focus
    int                     FocusIdxTabRequestCurrent;          // Tab-able item being requested for focus
    int                     FocusIdxAllRequestNext;             // Item being requested for focus, for next update (relies on layout to be stable between the frame pressing TAB and the next frame)
    int                     FocusIdxTabRequestNext;             // "

public:
    ImGuiWindow(const char* name);
    ~ImGuiWindow();

    ImGuiID     GetID(const char* str);
    ImGuiID     GetID(const void* ptr);

    bool        FocusItemRegister(bool is_active, bool tab_stop = true);      // Return true if focus is requested
    void        FocusItemUnregister();

    ImRect      Rect() const                            { return ImRect(Pos, Pos+Size); }
    float       CalcFontSize() const                    { return GImGui->FontBaseSize * FontWindowScale; }
    float       TitleBarHeight() const                  { return (Flags & ImGuiWindowFlags_NoTitleBar) ? 0 : CalcFontSize() + GImGui->Style.FramePadding.y * 2.0f; }
    ImRect      TitleBarRect() const                    { return ImRect(Pos, Pos + ImVec2(SizeFull.x, TitleBarHeight())); }
    ImVec2      WindowPadding() const                   { return ((Flags & ImGuiWindowFlags_ChildWindow) && !(Flags & ImGuiWindowFlags_ShowBorders)) ? ImVec2(0,0) : GImGui->Style.WindowPadding; }
    ImU32       Color(ImGuiCol idx, float a=1.f) const  { ImVec4 c = GImGui->Style.Colors[idx]; c.w *= GImGui->Style.Alpha * a; return ImGui::ColorConvertFloat4ToU32(c); }
    ImU32       Color(const ImVec4& col) const          { ImVec4 c = col; c.w *= GImGui->Style.Alpha; return ImGui::ColorConvertFloat4ToU32(c); }
};

static inline ImGuiWindow* GetCurrentWindow()
{
    // If this ever crash it probably means that ImGui::NewFrame() hasn't been called. We should always have a CurrentWindow in the stack (there is an implicit "Debug" window)
    ImGuiState& g = *GImGui;
    g.CurrentWindow->Accessed = true;
    return g.CurrentWindow;
}

static inline void SetCurrentWindow(ImGuiWindow* window)
{
    ImGuiState& g = *GImGui;
    g.CurrentWindow = window;
    if (window)
        g.FontSize = window->CalcFontSize();
}

static inline ImGuiWindow* GetParentWindow()
{
    ImGuiState& g = *GImGui;
    IM_ASSERT(g.CurrentWindowStack.size() >= 2);
    return g.CurrentWindowStack[g.CurrentWindowStack.size() - 2];
}

static void SetActiveId(ImGuiID id) 
{
    ImGuiState& g = *GImGui;
    g.ActiveId = id; 
    g.ActiveIdIsFocusedOnly = false;
}

static void RegisterAliveId(ImGuiID id)
{
    ImGuiState& g = *GImGui;
    if (g.ActiveId == id)
        g.ActiveIdIsAlive = true;
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

int ImGuiStorage::GetInt(ImU32 key, int default_val) const
{
    ImVector<Pair>::iterator it = LowerBound(const_cast<ImVector<ImGuiStorage::Pair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_i;
}

float ImGuiStorage::GetFloat(ImU32 key, float default_val) const
{
    ImVector<Pair>::iterator it = LowerBound(const_cast<ImVector<ImGuiStorage::Pair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_f;
}

void* ImGuiStorage::GetVoidPtr(ImGuiID key) const
{
    ImVector<Pair>::iterator it = LowerBound(const_cast<ImVector<ImGuiStorage::Pair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return NULL;
    return it->val_p;
}

// References are only valid until a new value is added to the storage. Calling a Set***() function or a Get***Ref() function invalidates the pointer.
int* ImGuiStorage::GetIntRef(ImGuiID key, int default_val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, Pair(key, default_val));
    return &it->val_i;
}

float* ImGuiStorage::GetFloatRef(ImGuiID key, float default_val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, Pair(key, default_val));
    return &it->val_f;
}

// FIXME-OPT: Wasting CPU because all SetInt() are preceeded by GetInt() calls so we should have the result from lower_bound already in place.
// However we only use SetInt() on explicit user action (so that's maximum once a frame) so the optimisation isn't much needed.
void ImGuiStorage::SetInt(ImU32 key, int val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, Pair(key, val));
        return;
    }
    it->val_i = val;
}

void ImGuiStorage::SetFloat(ImU32 key, float val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, Pair(key, val));
        return;
    }
    it->val_f = val;
}

void ImGuiStorage::SetVoidPtr(ImU32 key, void* val)
{
    ImVector<Pair>::iterator it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, Pair(key, val));
        return;
    }
    it->val_p = val;
}

void ImGuiStorage::SetAllInt(int v)
{
    for (size_t i = 0; i < Data.size(); i++)
        Data[i].val_i = v;
}

//-----------------------------------------------------------------------------

// Helper: Parse and apply text filters. In format "aaaaa[,bbbb][,ccccc]"
ImGuiTextFilter::ImGuiTextFilter(const char* default_filter)
{
    if (default_filter)
    {
        ImFormatString(InputBuf, IM_ARRAYSIZE(InputBuf), "%s", default_filter);
        Build();
    }
    else
    {
        InputBuf[0] = 0;
        CountGrep = 0;
    }
}

void ImGuiTextFilter::Draw(const char* label, float width)
{
    if (width > 0.0f)
        ImGui::PushItemWidth(width);
    ImGui::InputText(label, InputBuf, IM_ARRAYSIZE(InputBuf));
    if (width > 0.0f)
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

// On some platform vsnprintf() takes va_list by reference and modifies it. 
// va_copy is the 'correct' way to copy a va_list but Visual Studio prior to 2013 doesn't have it.
#ifndef va_copy
#define va_copy(dest, src) (dest = src)
#endif

// Helper: Text buffer for logging/accumulating text
void ImGuiTextBuffer::appendv(const char* fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    int len = vsnprintf(NULL, 0, fmt, args);         // FIXME-OPT: could do a first pass write attempt, likely successful on first pass.
    if (len <= 0)
        return;

    const size_t write_off = Buf.size();
    const size_t needed_sz = write_off + (size_t)len;
    if (write_off + (size_t)len >= Buf.capacity())
    {
        const size_t double_capacity = Buf.capacity() * 2;
        Buf.reserve(needed_sz > double_capacity ? needed_sz : double_capacity);
    }

    Buf.resize(needed_sz);
    ImFormatStringV(&Buf[write_off] - 1, (size_t)len+1, fmt, args_copy);
}

void ImGuiTextBuffer::append(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    appendv(fmt, args);
    va_end(args);
}

//-----------------------------------------------------------------------------

ImGuiWindow::ImGuiWindow(const char* name)
{
    Name = ImStrdup(name);
    ID = ImHash(name, 0); 
    IDStack.push_back(ID);
    MoveID = GetID("#MOVE");

    Flags = 0;
    PosFloat = Pos = ImVec2(0.0f, 0.0f);
    Size = SizeFull = ImVec2(0.0f, 0.0f);
    SizeContents = ImVec2(0.0f, 0.0f);
    ScrollY = 0.0f;
    NextScrollY = 0.0f;
    ScrollbarY = false;
    Visible = WasVisible = false;
    Accessed = false;
    Collapsed = false;
    SkipItems = false;
    AutoFitFrames = -1;
    AutoFitOnlyGrows = false;
    SetWindowPosAllowFlags = SetWindowSizeAllowFlags = SetWindowCollapsedAllowFlags = ImGuiSetCond_Always | ImGuiSetCond_Once | ImGuiSetCond_FirstUseEver;

    LastFrameDrawn = -1;
    ItemWidthDefault = 0.0f;
    FontWindowScale = 1.0f;

    DrawList = (ImDrawList*)ImGui::MemAlloc(sizeof(ImDrawList));
    new(DrawList) ImDrawList();
    RootWindow = NULL;

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
    ImGuiID seed = IDStack.back();
    const ImGuiID id = ImHash(str, 0, seed);
    RegisterAliveId(id);
    return id;
}

ImGuiID ImGuiWindow::GetID(const void* ptr)
{
    ImGuiID seed = IDStack.back();
    const ImGuiID id = ImHash(&ptr, sizeof(void*), seed);
    RegisterAliveId(id);
    return id;
}

bool ImGuiWindow::FocusItemRegister(bool is_active, bool tab_stop)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    const bool allow_keyboard_focus = window->DC.AllowKeyboardFocus.back();
    FocusIdxAllCounter++;
    if (allow_keyboard_focus)
        FocusIdxTabCounter++;

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

static inline void AddDrawListToRenderList(ImVector<ImDrawList*>& out_render_list, ImDrawList* draw_list)
{
    if (!draw_list->commands.empty() && !draw_list->vtx_buffer.empty())
    {
        if (draw_list->commands.back().vtx_count == 0)
            draw_list->commands.pop_back();
        out_render_list.push_back(draw_list);
    }
}

static void AddWindowToRenderList(ImVector<ImDrawList*>& out_render_list, ImGuiWindow* window)
{
    AddDrawListToRenderList(out_render_list, window->DrawList);
    for (size_t i = 0; i < window->DC.ChildWindows.size(); i++)
    {
        ImGuiWindow* child = window->DC.ChildWindows[i];
        if (child->Visible)                 // clipped children may have been marked not Visible
            AddWindowToRenderList(out_render_list, child);
    }
}

//-----------------------------------------------------------------------------

void* ImGui::MemAlloc(size_t sz)
{
    return GImGui->IO.MemAllocFn(sz);
}

void ImGui::MemFree(void* ptr)
{
    return GImGui->IO.MemFreeFn(ptr);
}
    
static ImGuiIniData* FindWindowSettings(const char* name)
{
    ImGuiState& g = *GImGui;
    ImGuiID id = ImHash(name, 0);
    for (size_t i = 0; i != g.Settings.size(); i++)
    {
        ImGuiIniData* ini = g.Settings[i];
        if (ini->ID == id)
            return ini;
    }
    return NULL;
}

static ImGuiIniData* AddWindowSettings(const char* name)
{
    ImGuiIniData* ini = (ImGuiIniData*)ImGui::MemAlloc(sizeof(ImGuiIniData));
    new(ini) ImGuiIniData();
    ini->Name = ImStrdup(name);
    ini->ID = ImHash(name, 0);
    ini->Collapsed = false;
    ini->Pos = ImVec2(FLT_MAX,FLT_MAX);
    ini->Size = ImVec2(0,0);
    GImGui->Settings.push_back(ini);
    return ini;
}

// Zero-tolerance, poor-man .ini parsing
// FIXME: Write something less rubbish
static void LoadSettings()
{
    ImGuiState& g = *GImGui;
    const char* filename = g.IO.IniFilename;
    if (!filename)
        return;

    char* file_data;
    size_t file_size;
    if (!ImLoadFileToMemory(filename, "rb", (void**)&file_data, &file_size, 1))
        return;

    ImGuiIniData* settings = NULL;
    const char* buf_end = file_data + file_size;
    for (const char* line_start = file_data; line_start < buf_end; )
    {
        const char* line_end = line_start;
        while (line_end < buf_end && *line_end != '\n' && *line_end != '\r')
            line_end++;
        
        if (line_start[0] == '[' && line_end > line_start && line_end[-1] == ']')
        {
            char name[64];
            ImFormatString(name, IM_ARRAYSIZE(name), "%.*s", line_end-line_start-2, line_start+1);
            settings = FindWindowSettings(name);
            if (!settings)
                settings = AddWindowSettings(name);
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

    ImGui::MemFree(file_data);
}

static void SaveSettings()
{
    ImGuiState& g = *GImGui;
    const char* filename = g.IO.IniFilename;
    if (!filename)
        return;

    // Gather data from windows that were active during this session
    for (size_t i = 0; i != g.Windows.size(); i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Flags & ImGuiWindowFlags_NoSavedSettings)
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
        const char* name = settings->Name;
        if (const char* p = strstr(name, "###"))  // Skip to the "###" marker if any. We don't skip past to match the behavior of GetID()
            name = p;
        fprintf(f, "[%s]\n", name);
        fprintf(f, "Pos=%d,%d\n", (int)settings->Pos.x, (int)settings->Pos.y);
        fprintf(f, "Size=%d,%d\n", (int)settings->Size.x, (int)settings->Size.y);
        fprintf(f, "Collapsed=%d\n", settings->Collapsed);
        fprintf(f, "\n");
    }

    fclose(f);
}

static void MarkSettingsDirty()
{
    ImGuiState& g = *GImGui;

    if (g.SettingsDirtyTimer <= 0.0f)
        g.SettingsDirtyTimer = g.IO.IniSavingRate;
}

const char* ImGui::GetVersion()
{
    return IMGUI_VERSION;
}

// Internal state access - if you want to share ImGui state between modules (e.g. DLL) or allocate it yourself
// Note that we still point to some static data and members (such as GFontAtlas), so the state instance you end up using will point to the static data within its module
void* ImGui::GetInternalState()
{
    return GImGui;
}

size_t ImGui::GetInternalStateSize()
{
    return sizeof(ImGuiState);
}

void ImGui::SetInternalState(void* state, bool construct)
{
    if (construct)
        new (state) ImGuiState();
    
    GImGui = (ImGuiState*)state;
}

ImGuiIO& ImGui::GetIO()
{
    return GImGui->IO;
}

ImGuiStyle& ImGui::GetStyle()
{
    return GImGui->Style;
}

void ImGui::NewFrame()
{
    ImGuiState& g = *GImGui;

    // Check user data
    IM_ASSERT(g.IO.DeltaTime > 0.0f);
    IM_ASSERT(g.IO.DisplaySize.x >= 0.0f && g.IO.DisplaySize.y >= 0.0f);
    IM_ASSERT(g.IO.RenderDrawListsFn != NULL);       // Must be implemented
    IM_ASSERT(g.IO.Fonts->Fonts.size() > 0);         // Font Atlas not created. Did you call io.Fonts->GetTexDataAsRGBA32 / GetTexDataAsAlpha8 ?
    IM_ASSERT(g.IO.Fonts->Fonts[0]->IsLoaded());     // Font Atlas not created. Did you call io.Fonts->GetTexDataAsRGBA32 / GetTexDataAsAlpha8 ?

    if (!g.Initialized)
    {
        // Initialize on first frame
        g.LogClipboard = (ImGuiTextBuffer*)ImGui::MemAlloc(sizeof(ImGuiTextBuffer));
        new(g.LogClipboard) ImGuiTextBuffer();

        IM_ASSERT(g.Settings.empty());
        LoadSettings();
        g.Initialized = true;
    }

    SetFont(g.IO.Fonts->Fonts[0]);

    g.Time += g.IO.DeltaTime;
    g.FrameCount += 1;
    g.Tooltip[0] = '\0';

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
                if (ImLengthSqr(g.IO.MousePos - g.IO.MouseClickedPos[i]) < g.IO.MouseDoubleClickMaxDist * g.IO.MouseDoubleClickMaxDist)
                    g.IO.MouseDoubleClicked[i] = true;
                g.IO.MouseClickedTime[i] = -FLT_MAX;    // so the third click isn't turned into a double-click
            }
            else
            {
                g.IO.MouseClickedTime[i] = g.Time;
            }
            g.IO.MouseClickedPos[i] = g.IO.MousePos;
            g.IO.MouseDragMaxDistanceSqr[i] = 0.0f;
        }
        else if (g.IO.MouseDown[i])
        {
            g.IO.MouseDragMaxDistanceSqr[i] = ImMax(g.IO.MouseDragMaxDistanceSqr[i], ImLengthSqr(g.IO.MousePos - g.IO.MouseClickedPos[i]));
        }
    }
    for (size_t i = 0; i < IM_ARRAYSIZE(g.IO.KeysDown); i++)
        g.IO.KeysDownTime[i] = g.IO.KeysDown[i] ? (g.IO.KeysDownTime[i] < 0.0f ? 0.0f : g.IO.KeysDownTime[i] + g.IO.DeltaTime) : -1.0f;

    // Calculate frame-rate for the user, as a purely luxurious feature
    g.FramerateSecPerFrameAccum += g.IO.DeltaTime - g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx];
    g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx] = g.IO.DeltaTime;
    g.FramerateSecPerFrameIdx = (g.FramerateSecPerFrameIdx + 1) % IM_ARRAYSIZE(g.FramerateSecPerFrame);
    g.IO.Framerate = 1.0f / (g.FramerateSecPerFrameAccum / (float)IM_ARRAYSIZE(g.FramerateSecPerFrame));

    // Clear reference to active widget if the widget isn't alive anymore
    g.HoveredId = 0;
    if (!g.ActiveIdIsAlive && g.ActiveIdPreviousFrame == g.ActiveId && g.ActiveId != 0)
        SetActiveId(0);
    g.ActiveIdPreviousFrame = g.ActiveId;
    g.ActiveIdIsAlive = false;
    if (!g.ActiveId)
        g.MovedWindow = NULL;

    // Delay saving settings so we don't spam disk too much
    if (g.SettingsDirtyTimer > 0.0f)
    {
        g.SettingsDirtyTimer -= g.IO.DeltaTime;
        if (g.SettingsDirtyTimer <= 0.0f)
            SaveSettings();
    }

    // Find the window we are hovering. Child windows can extend beyond the limit of their parent so we need to derive HoveredRootWindow from HoveredWindow
    g.HoveredWindow = FindHoveredWindow(g.IO.MousePos, false);
    if (g.HoveredWindow && (g.HoveredWindow->Flags & ImGuiWindowFlags_ChildWindow))
        g.HoveredRootWindow = g.HoveredWindow->RootWindow;
    else
        g.HoveredRootWindow = FindHoveredWindow(g.IO.MousePos, true);

    // Are we using inputs? Tell user so they can capture/discard the inputs away from the rest of their application.
    // When clicking outside of a window we assume the click is owned by the application and won't request capture.
    int mouse_earliest_button_down = -1;
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.MouseDown); i++)
    {
        if (g.IO.MouseClicked[i])
            g.IO.MouseDownOwned[i] = (g.HoveredWindow != NULL);
        if (g.IO.MouseDown[i])
            if (mouse_earliest_button_down == -1 || g.IO.MouseClickedTime[mouse_earliest_button_down] > g.IO.MouseClickedTime[i])
                mouse_earliest_button_down = i;
    }
    bool mouse_owned_by_application = mouse_earliest_button_down != -1 && !g.IO.MouseDownOwned[mouse_earliest_button_down];
    g.IO.WantCaptureMouse = (!mouse_owned_by_application && g.HoveredWindow != NULL) || (g.ActiveId != 0);
    g.IO.WantCaptureKeyboard = (g.ActiveId != 0);
    g.MouseCursor = ImGuiMouseCursor_Arrow;

    // If mouse was first clicked outside of ImGui bounds we also cancel out hovering.
    if (mouse_owned_by_application)
        g.HoveredWindow = g.HoveredRootWindow = NULL;

    // Scale & Scrolling
    if (g.HoveredWindow && g.IO.MouseWheel != 0.0f)
    {
        ImGuiWindow* window = g.HoveredWindow;
        if (g.IO.KeyCtrl)
        {
            if (g.IO.FontAllowUserScaling)
            {
                // Zoom / Scale window
                float new_font_scale = ImClamp(window->FontWindowScale + g.IO.MouseWheel * 0.10f, 0.50f, 2.50f);
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
            if (!(window->Flags & ImGuiWindowFlags_NoScrollWithMouse))
            {
                const int scroll_lines = (window->Flags & ImGuiWindowFlags_ComboBox) ? 3 : 5;
                window->NextScrollY -= g.IO.MouseWheel * window->CalcFontSize() * scroll_lines;
            }
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
        window->WasVisible = window->Visible;
        window->Visible = false;
        window->Accessed = false;
    }

    // No window should be open at the beginning of the frame.
    // But in order to allow the user to call NewFrame() multiple times without calling Render(), we are doing an explicit clear.
    g.CurrentWindowStack.resize(0);

    // Create implicit window - we will only render it if the user has added something to it.
    ImGui::SetNextWindowSize(ImVec2(400,400), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Debug");
}

// NB: behavior of ImGui after Shutdown() is not tested/guaranteed at the moment. This function is merely here to free heap allocations.
void ImGui::Shutdown()
{
    ImGuiState& g = *GImGui;
    if (!g.Initialized)
        return;

    SaveSettings();

    for (size_t i = 0; i < g.Windows.size(); i++)
    {
        g.Windows[i]->~ImGuiWindow();
        ImGui::MemFree(g.Windows[i]);
    }
    g.Windows.clear();
    g.WindowsSortBuffer.clear();
    g.CurrentWindowStack.clear();
    g.FocusedWindow = NULL;
    g.HoveredWindow = NULL;
    g.HoveredRootWindow = NULL;
    for (size_t i = 0; i < g.Settings.size(); i++)
    {
        g.Settings[i]->~ImGuiIniData();
        ImGui::MemFree(g.Settings[i]);
    }
    g.Settings.clear();
    g.ColorModifiers.clear();
    g.StyleModifiers.clear();
    g.FontStack.clear();
    for (size_t i = 0; i < IM_ARRAYSIZE(g.RenderDrawLists); i++)
        g.RenderDrawLists[i].clear();
    g.MouseCursorDrawList.ClearFreeMemory();
    g.ColorEditModeStorage.Clear();
    if (g.PrivateClipboard)
    {
        ImGui::MemFree(g.PrivateClipboard);
        g.PrivateClipboard = NULL;
    }

    if (g.LogFile && g.LogFile != stdout)
    {
        fclose(g.LogFile);
        g.LogFile = NULL;
    }
    if (g.LogClipboard)
    {
        g.LogClipboard->~ImGuiTextBuffer();
        ImGui::MemFree(g.LogClipboard);
    }

    g.IO.Fonts->Clear();

    g.Initialized = false;
}

// FIXME: Add a more explicit sort order in the window structure.
static int ChildWindowComparer(const void* lhs, const void* rhs)
{
    const ImGuiWindow* a = *(const ImGuiWindow**)lhs;
    const ImGuiWindow* b = *(const ImGuiWindow**)rhs;
    if (int d = (a->Flags & ImGuiWindowFlags_Popup) - (b->Flags & ImGuiWindowFlags_Popup))
        return d;
    if (int d = (a->Flags & ImGuiWindowFlags_Tooltip) - (b->Flags & ImGuiWindowFlags_Tooltip))
        return d;
    if (int d = (a->Flags & ImGuiWindowFlags_ComboBox) - (b->Flags & ImGuiWindowFlags_ComboBox))
        return d;
    return 0;
}

static void AddWindowToSortedBuffer(ImVector<ImGuiWindow*>& out_sorted_windows, ImGuiWindow* window)
{
    out_sorted_windows.push_back(window);
    if (window->Visible)
    {
        const size_t count = window->DC.ChildWindows.size();
        if (count > 1)
            qsort(window->DC.ChildWindows.begin(), count, sizeof(ImGuiWindow*), ChildWindowComparer);
        for (size_t i = 0; i < count; i++)
        {
            ImGuiWindow* child = window->DC.ChildWindows[i];
            if (child->Visible)
                AddWindowToSortedBuffer(out_sorted_windows, child);
        }
    }
}

static void PushClipRect(const ImVec4& clip_rect, bool clipped = true)
{
    ImGuiWindow* window = GetCurrentWindow();

    ImVec4 cr = clip_rect;
    if (clipped && !window->ClipRectStack.empty())
    {
        // Clip with existing clip rect
        const ImVec4 cur_cr = window->ClipRectStack.back();
        cr = ImVec4(ImMax(cr.x, cur_cr.x), ImMax(cr.y, cur_cr.y), ImMin(cr.z, cur_cr.z), ImMin(cr.w, cur_cr.w));
    }
    cr.z = ImMax(cr.x, cr.z);
    cr.w = ImMax(cr.y, cr.w);

    IM_ASSERT(cr.x <= cr.z && cr.y <= cr.w);
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
    ImGuiState& g = *GImGui;
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

        // Select window for move/focus when we're done with all our widgets (we use the root window ID here)
        if (g.ActiveId == 0 && g.HoveredId == 0 && g.HoveredRootWindow != NULL && g.IO.MouseClicked[0])
        {
            IM_ASSERT(g.MovedWindow == NULL);
            g.MovedWindow = g.HoveredWindow;
            SetActiveId(g.HoveredRootWindow->MoveID);
        }

        // Sort the window list so that all child windows are after their parent
        // We cannot do that on FocusWindow() because childs may not exist yet
        g.WindowsSortBuffer.resize(0);
        g.WindowsSortBuffer.reserve(g.Windows.size());
        for (size_t i = 0; i != g.Windows.size(); i++)
        {
            ImGuiWindow* window = g.Windows[i];
            if (window->Flags & ImGuiWindowFlags_ChildWindow)           // if a child is visible its parent will add it
                if (window->Visible)
                    continue;
            AddWindowToSortedBuffer(g.WindowsSortBuffer, window);
        }
        IM_ASSERT(g.Windows.size() == g.WindowsSortBuffer.size());    // we done something wrong
        g.Windows.swap(g.WindowsSortBuffer);

        // Clear data for next frame
        g.IO.MouseWheel = 0.0f;
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
        for (size_t i = 0; i < IM_ARRAYSIZE(g.RenderDrawLists); i++)
            g.RenderDrawLists[i].resize(0);
        for (size_t i = 0; i != g.Windows.size(); i++)
        {
            ImGuiWindow* window = g.Windows[i];
            if (window->Visible && (window->Flags & (ImGuiWindowFlags_ChildWindow)) == 0)
            {
                // FIXME: Generalize this with a proper layering system so we can stack.
                if (window->Flags & ImGuiWindowFlags_Popup)
                    AddWindowToRenderList(g.RenderDrawLists[1], window);
                else if (window->Flags & ImGuiWindowFlags_Tooltip)
                    AddWindowToRenderList(g.RenderDrawLists[2], window);
                else
                    AddWindowToRenderList(g.RenderDrawLists[0], window);
            }
        }

        // Flatten layers
        size_t n = g.RenderDrawLists[0].size();
        size_t flattened_size = n;
        for (size_t i = 1; i < IM_ARRAYSIZE(g.RenderDrawLists); i++)
            flattened_size += g.RenderDrawLists[i].size();
        g.RenderDrawLists[0].resize(flattened_size);
        for (size_t i = 1; i < IM_ARRAYSIZE(g.RenderDrawLists); i++)
        {
            ImVector<ImDrawList*>& layer = g.RenderDrawLists[i];
            if (!layer.empty())
            {
                memcpy(&g.RenderDrawLists[0][n], &layer[0], layer.size() * sizeof(ImDrawList*));
                n += layer.size();
            }
        }

        if (g.IO.MouseDrawCursor)
        {
            const ImGuiMouseCursorData& cursor_data = g.MouseCursorData[g.MouseCursor];
            const ImVec2 pos = g.IO.MousePos - cursor_data.Offset;
            const ImVec2 size = cursor_data.Size;
            const ImTextureID tex_id = g.IO.Fonts->TexID;
            g.MouseCursorDrawList.Clear();
            g.MouseCursorDrawList.PushTextureID(tex_id);
            g.MouseCursorDrawList.AddImage(tex_id, pos+ImVec2(1,0), pos+ImVec2(1,0) + size, cursor_data.TexUvMin[1], cursor_data.TexUvMax[1], 0x30000000); // Shadow
            g.MouseCursorDrawList.AddImage(tex_id, pos+ImVec2(2,0), pos+ImVec2(2,0) + size, cursor_data.TexUvMin[1], cursor_data.TexUvMax[1], 0x30000000); // Shadow
            g.MouseCursorDrawList.AddImage(tex_id, pos,             pos + size,             cursor_data.TexUvMin[1], cursor_data.TexUvMax[1], 0xFF000000); // Black border
            g.MouseCursorDrawList.AddImage(tex_id, pos,             pos + size,             cursor_data.TexUvMin[0], cursor_data.TexUvMax[0], 0xFFFFFFFF); // White fill
            g.MouseCursorDrawList.PopTextureID();
            AddDrawListToRenderList(g.RenderDrawLists[0], &g.MouseCursorDrawList);
        }

        // Render
        if (!g.RenderDrawLists[0].empty())
            g.IO.RenderDrawListsFn(&g.RenderDrawLists[0][0], (int)g.RenderDrawLists[0].size());
    }
}

// Find the optional ## from which we stop displaying text.
static const char*  FindTextDisplayEnd(const char* text, const char* text_end = NULL)
{
    const char* text_display_end = text;
    if (!text_end)
        text_end = (const char*)-1;
    while (text_display_end < text_end && *text_display_end != '\0' && (text_display_end[0] != '#' || text_display_end[1] != '#'))
        text_display_end++;
    return text_display_end;
}

// Pass text data straight to log (without being displayed)
void ImGui::LogText(const char* fmt, ...)
{
    ImGuiState& g = *GImGui;
    if (!g.LogEnabled)
        return;

    va_list args;
    va_start(args, fmt);
    if (g.LogFile)
    {
        vfprintf(g.LogFile, fmt, args);
    }
    else
    {
        g.LogClipboard->appendv(fmt, args);
    }
    va_end(args);
}

// Internal version that takes a position to decide on newline placement and pad items according to their depth.
// We split text into individual lines to add current tree level padding
static void LogText(const ImVec2& ref_pos, const char* text, const char* text_end)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    if (!text_end)
        text_end = FindTextDisplayEnd(text, text_end);

    const bool log_new_line = ref_pos.y > window->DC.LogLinePosY+1;
    window->DC.LogLinePosY = ref_pos.y;

    const char* text_remaining = text;
    if (g.LogStartDepth > window->DC.TreeDepth)  // Re-adjust padding if we have popped out of our starting depth
        g.LogStartDepth = window->DC.TreeDepth;
    const int tree_depth = (window->DC.TreeDepth - g.LogStartDepth);
    for (;;)
    {
        // Split the string. Each new line (after a '\n') is followed by spacing corresponding to the current depth of our log entry.
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
            if (log_new_line || !is_first_line)
                ImGui::LogText(STR_NEWLINE "%*s%.*s", tree_depth*4, "", char_count, text_remaining);
            else
                ImGui::LogText(" %.*s", char_count, text_remaining);
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

// Internal ImGui functions to render text
// RenderText***() functions calls ImDrawList::AddText() calls ImBitmapFont::RenderText()
static void RenderText(ImVec2 pos, const char* text, const char* text_end, bool hide_text_after_hash)
{
    ImGuiState& g = *GImGui;
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
        window->DrawList->AddText(g.Font, g.FontSize, pos, window->Color(ImGuiCol_Text), text, text_display_end);

        // Log as text
        if (g.LogEnabled)
            LogText(pos, text, text_display_end);
    }
}

static void RenderTextWrapped(ImVec2 pos, const char* text, const char* text_end, float wrap_width)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    if (!text_end)
        text_end = text + strlen(text); // FIXME-OPT

    const int text_len = (int)(text_end - text);
    if (text_len > 0)
    {
        // Render
        window->DrawList->AddText(g.Font, g.FontSize, pos, window->Color(ImGuiCol_Text), text, text_end, wrap_width);

        // Log as text
        if (g.LogEnabled)
            LogText(pos, text, text_end);
    }
}

static void RenderTextClipped(ImVec2 pos, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& clip_max)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    // Hide anything after a '##' string
    const char* text_display_end = FindTextDisplayEnd(text, text_end);
    const int text_len = (int)(text_display_end - text);
    if (text_len > 0)
    {
        const ImVec2 text_size = text_size_if_known ? *text_size_if_known : ImGui::CalcTextSize(text, text_display_end, false, 0.0f);

        // Perform CPU side clipping for single clipped element to avoid using scissor state
        const bool need_clipping = (pos.x + text_size.x >= clip_max.x) || (pos.y + text_size.y >= clip_max.y);

        // Render
        window->DrawList->AddText(g.Font, g.FontSize, pos, window->Color(ImGuiCol_Text), text, text_display_end, 0.0f, need_clipping ? &clip_max : NULL);

        // Log as text
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
        // FIXME: This is the best I've found that works on multiple renderer/back ends. Bit dodgy.
        window->DrawList->AddRect(p_min+ImVec2(1.5f,1.5f), p_max+ImVec2(1,1), window->Color(ImGuiCol_BorderShadow), rounding);
        window->DrawList->AddRect(p_min+ImVec2(0.5f,0.5f), p_max+ImVec2(0,0), window->Color(ImGuiCol_Border), rounding);
    }
}

// Render a triangle to denote expanded/collapsed state
static void RenderCollapseTriangle(ImVec2 p_min, bool opened, float scale, bool shadow)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    const float h = g.FontSize * 1.00f;
    const float r = h * 0.40f * scale;
    ImVec2 center = p_min + ImVec2(h*0.50f, h*0.50f*scale);

    ImVec2 a, b, c;
    if (opened)
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
    window->DrawList->AddTriangleFilled(a, b, c, window->Color(ImGuiCol_Text));
}

// Calculate text size. Text can be multi-line. Optionally ignore text after a ## marker.
// CalcTextSize("") should return ImVec2(0.0f, GImGui->FontSize)
ImVec2 ImGui::CalcTextSize(const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width)
{
    ImGuiState& g = *GImGui;

    const char* text_display_end;
    if (hide_text_after_double_hash)
        text_display_end = FindTextDisplayEnd(text, text_end);      // Hide anything after a '##' string
    else
        text_display_end = text_end;

    ImFont* font = g.Font;
    const float font_size = g.FontSize;
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, wrap_width, text, text_display_end, NULL);

    // Cancel out character spacing for the last character of a line (it is baked into glyph->XAdvance field)
    const float font_scale = font_size / font->FontSize; 
    const float character_spacing_x = 1.0f * font_scale;
    if (text_size.x > 0.0f)
        text_size.x -= character_spacing_x;

    return text_size;
}

// Helper to calculate coarse clipping of large list of evenly sized items. 
// If you are displaying thousands of items and you have a random access to the list, you can perform clipping yourself to save on CPU.
// {
//    float item_height = ImGui::GetTextLineHeightWithSpacing();
//    int display_start, display_end;
//    ImGui::CalcListClipping(count, item_height, &display_start, &display_end);            // calculate how many to clip/display
//    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (display_start) * item_height);         // advance cursor
//    for (int i = display_start; i < display_end; i++)                                     // display only visible items
//        // TODO: display visible item
//    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (count - display_end) * item_height);   // advance cursor
// }
void ImGui::CalcListClipping(int items_count, float items_height, int* out_items_display_start, int* out_items_display_end)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    if (g.LogEnabled)
    {
        // If logging is active, do not perform any clipping
        *out_items_display_start = 0;
        *out_items_display_end = items_count;
        return;
    }

    const ImVec2 pos = window->DC.CursorPos;
    const ImVec4 clip_rect = window->ClipRectStack.back();
    const float clip_y1 = clip_rect.y;
    const float clip_y2 = clip_rect.w;

    int start = (int)((clip_y1 - pos.y) / items_height);
    int end = (int)((clip_y2 - pos.y) / items_height);
    start = ImClamp(start, 0, items_count);
    end = ImClamp(end + 1, start, items_count);

    *out_items_display_start = start;
    *out_items_display_end = end;
}

// Find window given position, search front-to-back
static ImGuiWindow* FindHoveredWindow(ImVec2 pos, bool excluding_childs)
{
    ImGuiState& g = *GImGui;
    for (int i = (int)g.Windows.size()-1; i >= 0; i--)
    {
        ImGuiWindow* window = g.Windows[(size_t)i];
        if (!window->Visible)
            continue;
        if (excluding_childs && (window->Flags & ImGuiWindowFlags_ChildWindow) != 0)
            continue;

        // Using the clipped AABB so a child window will typically be clipped by its parent.
        ImRect bb(window->ClippedRect.Min - g.Style.TouchExtraPadding, window->ClippedRect.Max + g.Style.TouchExtraPadding);
        if (bb.Contains(pos))
            return window;
    }
    return NULL;
}

// Test if mouse cursor is hovering given rectangle
// NB- Rectangle is clipped by our current clip setting
// NB- Expand the rectangle to be generous on imprecise inputs systems (g.Style.TouchExtraPadding)
static bool IsMouseHoveringRect(const ImRect& rect)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    // Clip
    ImRect rect_clipped = rect;
    if (!window->ClipRectStack.empty())
    {
        const ImVec4 clip_rect = window->ClipRectStack.back();
        rect_clipped.Clip(ImRect(ImVec2(clip_rect.x, clip_rect.y), ImVec2(clip_rect.z, clip_rect.w)));
    }

    // Expand for touch input
    const ImRect rect_for_touch(rect_clipped.Min - g.Style.TouchExtraPadding, rect_clipped.Max + g.Style.TouchExtraPadding);
    return rect_for_touch.Contains(g.IO.MousePos);
}

bool ImGui::IsMouseHoveringRect(const ImVec2& rect_min, const ImVec2& rect_max)
{
    return IsMouseHoveringRect(ImRect(rect_min, rect_max));
}

bool ImGui::IsMouseHoveringWindow()
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    return g.HoveredWindow == window;
}

bool ImGui::IsMouseHoveringAnyWindow()
{
    ImGuiState& g = *GImGui;
    return g.HoveredWindow != NULL;
}

bool ImGui::IsPosHoveringAnyWindow(const ImVec2& pos)
{
    return FindHoveredWindow(pos, false) != NULL;
}

static bool IsKeyPressedMap(ImGuiKey key, bool repeat)
{
    ImGuiState& g = *GImGui;
    const int key_index = g.IO.KeyMap[key];
    return ImGui::IsKeyPressed(key_index, repeat);
}

bool ImGui::IsKeyPressed(int key_index, bool repeat)
{
    ImGuiState& g = *GImGui;
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
    ImGuiState& g = *GImGui;
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
    ImGuiState& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseDoubleClicked[button];
}

bool ImGui::IsMouseDragging(int button, float lock_threshold)
{
    ImGuiState& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (lock_threshold < 0.0f)
        lock_threshold = g.IO.MouseDragThreshold;
    return g.IO.MouseDragMaxDistanceSqr[button] >= lock_threshold * lock_threshold;
}

ImVec2 ImGui::GetMousePos()
{
    return GImGui->IO.MousePos;
}

ImVec2 ImGui::GetMouseDragDelta(int button, float lock_threshold)
{
    ImGuiState& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (lock_threshold < 0.0f)
        lock_threshold = g.IO.MouseDragThreshold;
    if (g.IO.MouseDown[button])
        if (g.IO.MouseDragMaxDistanceSqr[button] >= lock_threshold * lock_threshold)
            return g.IO.MousePos - g.IO.MouseClickedPos[button];     // Assume we can only get active with left-mouse button (at the moment).
    return ImVec2(0.0f, 0.0f);
}

ImGuiMouseCursor ImGui::GetMouseCursor()
{
    return GImGui->MouseCursor;
}

void ImGui::SetMouseCursor(ImGuiMouseCursor cursor_type)
{
    GImGui->MouseCursor = cursor_type;
}

bool ImGui::IsItemHovered()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.LastItemHoveredAndUsable;
}

bool ImGui::IsItemHoveredRect()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.LastItemHoveredRect;
}

bool ImGui::IsItemActive()
{
    ImGuiState& g = *GImGui;
    if (g.ActiveId)
    {
        ImGuiWindow* window = GetCurrentWindow();
        return g.ActiveId == window->DC.LastItemID;
    }
    return false;
}

bool ImGui::IsAnyItemActive()
{
    ImGuiState& g = *GImGui;
    return g.ActiveId != 0;
}

ImVec2 ImGui::GetItemRectMin()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.LastItemRect.Min;
}

ImVec2 ImGui::GetItemRectMax()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.LastItemRect.Max;
}

ImVec2 ImGui::GetItemRectSize()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.LastItemRect.GetSize();
}

ImVec2 ImGui::CalcItemRectClosestPoint(const ImVec2& pos, bool on_edge, float outward)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImRect rect = window->DC.LastItemRect;
    rect.Expand(outward);
    return rect.GetClosestPoint(pos, on_edge);
}

// Tooltip is stored and turned into a BeginTooltip()/EndTooltip() sequence at the end of the frame. Each call override previous value.
void ImGui::SetTooltipV(const char* fmt, va_list args)
{
    ImGuiState& g = *GImGui;
    ImFormatStringV(g.Tooltip, IM_ARRAYSIZE(g.Tooltip), fmt, args);
}

void ImGui::SetTooltip(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SetTooltipV(fmt, args);
    va_end(args);
}

float ImGui::GetTime()
{
    return GImGui->Time;
}

int ImGui::GetFrameCount()
{
    return GImGui->FrameCount;
}

void ImGui::BeginTooltip()
{
    ImGuiState& g = *GImGui;
    ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("##Tooltip", NULL, ImVec2(0,0), g.Style.Colors[ImGuiCol_TooltipBg].w, flags);
}

void ImGui::EndTooltip()
{
    IM_ASSERT(GetCurrentWindow()->Flags & ImGuiWindowFlags_Tooltip);
    ImGui::End();
}

void ImGui::BeginPopup(bool* p_opened)
{
    IM_ASSERT(p_opened != NULL);    // Must provide a bool at the moment

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_Popup|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("##Popup", p_opened, flags);
}

void ImGui::EndPopup()
{
    IM_ASSERT(GetCurrentWindow()->Flags & ImGuiWindowFlags_Popup);
    ImGui::End();
    ImGui::PopStyleVar();
}

bool ImGui::BeginChild(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_ChildWindow;

    const ImVec2 content_max = window->Pos + ImGui::GetContentRegionMax();
    const ImVec2 cursor_pos = window->Pos + ImGui::GetCursorPos();
    ImVec2 size = size_arg;
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

    const float alpha = 1.0f;
    bool ret = ImGui::Begin(title, NULL, size, alpha, flags);

    if (!(window->Flags & ImGuiWindowFlags_ShowBorders))
        g.CurrentWindow->Flags &= ~ImGuiWindowFlags_ShowBorders;

    return ret;
}

bool ImGui::BeginChild(ImGuiID id, const ImVec2& size, bool border, ImGuiWindowFlags extra_flags)
{
    char str_id[32];
    ImFormatString(str_id, IM_ARRAYSIZE(str_id), "child_%x", id);
    bool ret = ImGui::BeginChild(str_id, size, border, extra_flags);
    return ret;
}

void ImGui::EndChild()
{
    ImGuiWindow* window = GetCurrentWindow();

    IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);
    if (window->Flags & ImGuiWindowFlags_ComboBox)
    {
        ImGui::End();
    }
    else
    {
        // When using auto-filling child window, we don't provide full width/height to ItemSize so that it doesn't feed back into automatic size-fitting.
        ImGuiState& g = *GImGui;
        ImVec2 sz = ImGui::GetWindowSize();
        if (window->Flags & ImGuiWindowFlags_ChildWindowAutoFitX)
            sz.x = ImMax(g.Style.WindowMinSize.x, sz.x - g.Style.AutoFitPadding.x);
        if (window->Flags & ImGuiWindowFlags_ChildWindowAutoFitY)
            sz.y = ImMax(g.Style.WindowMinSize.y, sz.y - g.Style.AutoFitPadding.y);
        
        ImGui::End();

        window = GetCurrentWindow();
        ImRect bb(window->DC.CursorPos, window->DC.CursorPos + sz);
        ItemSize(sz);
        ItemAdd(bb, NULL);
    }
}

// Helper to create a child window / scrolling region that looks like a normal widget frame.
void ImGui::BeginChildFrame(ImGuiID id, const ImVec2& size)
{
    ImGuiState& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, style.Colors[ImGuiCol_FrameBg]);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, style.FrameRounding);
    ImGui::BeginChild(id, size);
}

void ImGui::EndChildFrame()
{
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

static ImGuiWindow* FindWindowByName(const char* name)
{
    // FIXME-OPT: Store sorted hashes -> pointers.
    ImGuiState& g = *GImGui;
    ImGuiID id = ImHash(name, 0);
    for (size_t i = 0; i < g.Windows.size(); i++)
        if (g.Windows[i]->ID == id)
            return g.Windows[i];
    return NULL;
}

static ImGuiWindow* CreateNewWindow(const char* name, ImVec2 size, ImGuiWindowFlags flags)
{
    ImGuiState& g = *GImGui;

    // Create window the first time
    ImGuiWindow* window = (ImGuiWindow*)ImGui::MemAlloc(sizeof(ImGuiWindow));
    new(window) ImGuiWindow(name);
    window->Flags = flags;

    if (flags & ImGuiWindowFlags_NoSavedSettings)
    {
        // User can disable loading and saving of settings. Tooltip and child windows also don't store settings.
        window->Size = window->SizeFull = size;
    }
    else
    {
        // Retrieve settings from .ini file
        // Use SetWindowPos() or SetNextWindowPos() with the appropriate condition flag to change the initial position of a window.
        window->PosFloat = ImVec2(60, 60);
        window->Pos = ImVec2((float)(int)window->PosFloat.x, (float)(int)window->PosFloat.y);

        ImGuiIniData* settings = FindWindowSettings(name);
        if (!settings)
        {
            settings = AddWindowSettings(name);
        }
        else
        {
            window->SetWindowPosAllowFlags &= ~ImGuiSetCond_FirstUseEver;
            window->SetWindowSizeAllowFlags &= ~ImGuiSetCond_FirstUseEver;
            window->SetWindowCollapsedAllowFlags &= ~ImGuiSetCond_FirstUseEver;
        }

        if (settings->Pos.x != FLT_MAX)
        {
            window->PosFloat = settings->Pos;
            window->Pos = ImVec2((float)(int)window->PosFloat.x, (float)(int)window->PosFloat.y);
            window->Collapsed = settings->Collapsed;
        }

        if (ImLengthSqr(settings->Size) > 0.00001f && !(flags & ImGuiWindowFlags_NoResize))
            size = settings->Size;
        window->Size = window->SizeFull = size;
    }

    if ((flags & ImGuiWindowFlags_AlwaysAutoResize) != 0)
    {
        window->AutoFitFrames = 2;
        window->AutoFitOnlyGrows = false;
    }
    else if (ImLengthSqr(window->Size) < 0.00001f)
    {
        window->AutoFitFrames = 2;
        window->AutoFitOnlyGrows = true;
    }

    g.Windows.push_back(window);
    return window;
}

// Push a new ImGui window to add widgets to. 
// - 'size' for a regular window denote the initial size for first-time creation (no saved data) and isn't that useful. Use SetNextWindowSize() prior to calling Begin() for more flexible window manipulation.
// - A default window called "Debug" is automatically stacked at the beginning of every frame so you can use widgets without explicitly calling a Begin/End pair.
// - Begin/End can be called multiple times during the frame with the same window name to append content.
// - The window name is used as a unique identifier to preserve window information across frames (and save rudimentary information to the .ini file). 
//   You can use the "##" or "###" markers to use the same label with different id, or same id with different label. See documentation at the top of this file.
// - Return false when window is collapsed, so you can early out in your code. You always need to call ImGui::End() even if false is returned.
// - Passing 'bool* p_opened' displays a Close button on the upper-right corner of the window, the pointed value will be set to false when the button is pressed.
// - Passing non-zero 'size' is roughly equivalent to calling SetNextWindowSize(size, ImGuiSetCond_FirstUseEver) prior to calling Begin().
bool ImGui::Begin(const char* name, bool* p_opened, ImGuiWindowFlags flags)
{
    return ImGui::Begin(name, p_opened, ImVec2(0.f, 0.f), -1.0f, flags);
}

bool ImGui::Begin(const char* name, bool* p_opened, const ImVec2& size_on_first_use, float bg_alpha, ImGuiWindowFlags flags)
{
    ImGuiState& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    IM_ASSERT(g.Initialized);                       // Forgot to call ImGui::NewFrame()
    IM_ASSERT(name != NULL);                        // Must pass a name

    // Find or create
    bool window_is_new = false;
    ImGuiWindow* window = FindWindowByName(name);
    if (!window)
    {
        window = CreateNewWindow(name, size_on_first_use, flags);
        window_is_new = true;
    }
    window->Flags = (ImGuiWindowFlags)flags;

    // Add to stack
    g.CurrentWindowStack.push_back(window);
    SetCurrentWindow(window);

    // Process SetNextWindow***() calls
    bool window_pos_set_by_api = false;
    if (g.SetNextWindowPosCond)
    {
        const ImVec2 backup_cursor_pos = window->DC.CursorPos;                  // FIXME: not sure of the exact reason of this anymore :( need to look into that.
        ImGui::SetWindowPos(g.SetNextWindowPosVal, g.SetNextWindowPosCond);
        window->DC.CursorPos = backup_cursor_pos;
        window_pos_set_by_api = true;
        g.SetNextWindowPosCond = 0;
    }
    if (g.SetNextWindowSizeCond)
    {
        ImGui::SetWindowSize(g.SetNextWindowSizeVal, g.SetNextWindowSizeCond);
        g.SetNextWindowSizeCond = 0;
    }
    if (g.SetNextWindowCollapsedCond)
    {
        ImGui::SetWindowCollapsed(g.SetNextWindowCollapsedVal, g.SetNextWindowCollapsedCond);
        g.SetNextWindowCollapsedCond = 0;
    }
    if (g.SetNextWindowFocus)
    {
        ImGui::SetWindowFocus();
        g.SetNextWindowFocus = false;
    }

    // Find parent
    ImGuiWindow* parent_window = (flags & ImGuiWindowFlags_ChildWindow) != 0 ? g.CurrentWindowStack[g.CurrentWindowStack.size()-2] : NULL;

    // Update known root window (if we are a child window, otherwise window == window->RootWindow)
    size_t root_idx = g.CurrentWindowStack.size() - 1;
    while (root_idx > 0)
    {
        if ((g.CurrentWindowStack[root_idx]->Flags & ImGuiWindowFlags_ChildWindow) == 0)
            break;
        root_idx--;
    }
    window->RootWindow = g.CurrentWindowStack[root_idx];

    // Default alpha
    if (bg_alpha < 0.0f)
        bg_alpha = style.WindowFillAlphaDefault;

    // When reusing window again multiple times a frame, just append content (don't need to setup again)
    const int current_frame = ImGui::GetFrameCount();
    const bool first_begin_of_the_frame = (window->LastFrameDrawn != current_frame);
    if (first_begin_of_the_frame)
    {
        window->DrawList->Clear();
        window->Visible = true;

        // New windows appears in front
        if (!(flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Tooltip))
        {
            if (window->LastFrameDrawn < current_frame - 1)
            {
                FocusWindow(window);

                // Popup position themselves when they first appear
                if (flags & ImGuiWindowFlags_Popup)
                {
                    if (!window_pos_set_by_api)
                        window->PosFloat = g.IO.MousePos;
                }
            }
        }

        window->LastFrameDrawn = current_frame;
        window->ClipRectStack.resize(0);

        // Reset contents size for auto-fitting
        window->SizeContents = window_is_new ? ImVec2(0.0f, 0.0f) : window->DC.CursorMaxPos - window->Pos;
        window->SizeContents.y += window->ScrollY;

        if (flags & ImGuiWindowFlags_ChildWindow)
        {
            parent_window->DC.ChildWindows.push_back(window);
            window->Pos = window->PosFloat = parent_window->DC.CursorPos;
            window->SizeFull = size_on_first_use;
        }
    }

    // Setup texture
    window->DrawList->PushTextureID(g.Font->ContainerAtlas->TexID);

    // Setup outer clipping rectangle
    if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_ComboBox))
        PushClipRect(parent_window->ClipRectStack.back());
    else if (g.IO.DisplayVisibleMin.x != g.IO.DisplayVisibleMax.x && g.IO.DisplayVisibleMin.y != g.IO.DisplayVisibleMax.y)
        PushClipRect(ImVec4(g.IO.DisplayVisibleMin.x, g.IO.DisplayVisibleMin.y, g.IO.DisplayVisibleMax.x, g.IO.DisplayVisibleMax.y));
    else
        PushClipRect(ImVec4(0.0f, 0.0f, g.IO.DisplaySize.x, g.IO.DisplaySize.y));

    // Setup and draw window
    if (first_begin_of_the_frame)
    {
        // Reset ID stack
        window->IDStack.resize(1);

        // Move window (at the beginning of the frame to avoid input lag or sheering). Only valid for root windows.
        RegisterAliveId(window->MoveID);
        if (g.ActiveId == window->MoveID)
        {
            if (g.IO.MouseDown[0])
            {
                if (!(window->Flags & ImGuiWindowFlags_NoMove))
                {
                    window->PosFloat += g.IO.MouseDelta;
                    if (!(window->Flags & ImGuiWindowFlags_NoSavedSettings))
                        MarkSettingsDirty();
                }
                IM_ASSERT(g.MovedWindow != NULL);
                FocusWindow(g.MovedWindow);
            }
            else
            {
                SetActiveId(0);
                g.MovedWindow = NULL;   // Not strictly necessary but doing it for sanity.
            }
        }

        // Tooltips always follows mouse
        if (!window_pos_set_by_api && (window->Flags & ImGuiWindowFlags_Tooltip) != 0)
        {
            window->PosFloat = g.IO.MousePos + ImVec2(32,16) - style.FramePadding*2;
        }

        // Clamp into view
        if (!(window->Flags & ImGuiWindowFlags_ChildWindow) && !(window->Flags & ImGuiWindowFlags_Tooltip))
        {
            if (window->AutoFitFrames <= 0 && g.IO.DisplaySize.x > 0.0f && g.IO.DisplaySize.y > 0.0f) // Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
            {
                ImVec2 clip_min = style.DisplaySafeAreaPadding;
                ImVec2 clip_max = g.IO.DisplaySize - style.DisplaySafeAreaPadding;
                window->PosFloat = ImMax(window->PosFloat + window->Size, clip_min) - window->Size;
                window->PosFloat = ImMin(window->PosFloat, clip_max);
            }
            window->SizeFull = ImMax(window->SizeFull, style.WindowMinSize);
        }
        window->Pos = ImVec2((float)(int)window->PosFloat.x, (float)(int)window->PosFloat.y);

        // Default item width. Make it proportional to window size if window manually resizes
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

        ImRect title_bar_rect = window->TitleBarRect();

        // Apply and ImClamp scrolling
        window->ScrollY = window->NextScrollY;
        window->ScrollY = ImMax(window->ScrollY, 0.0f);
        if (!window->Collapsed && !window->SkipItems)
            window->ScrollY = ImMin(window->ScrollY, ImMax(0.0f, window->SizeContents.y - window->SizeFull.y));
        window->NextScrollY = window->ScrollY;

        // At this point we don't have a clipping rectangle setup yet, so we can test and draw in title bar
        // Collapse window by double-clicking on title bar
        if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
        {
            if (!(window->Flags & ImGuiWindowFlags_NoCollapse) && g.HoveredWindow == window && IsMouseHoveringRect(title_bar_rect) && g.IO.MouseDoubleClicked[0])
            {
                window->Collapsed = !window->Collapsed;
                if (!(window->Flags & ImGuiWindowFlags_NoSavedSettings))
                    MarkSettingsDirty();
                FocusWindow(window);
            }
        }
        else
        {
            window->Collapsed = false;
        }

        // Calculate auto-fit size
        ImVec2 size_auto_fit;
        if ((window->Flags & ImGuiWindowFlags_Tooltip) != 0)
        {
            // Tooltip always resize. We keep the spacing symmetric on both axises for aesthetic purpose.
            size_auto_fit = window->SizeContents + style.WindowPadding - ImVec2(0.0f, style.ItemSpacing.y);
        }
        else
        {
            size_auto_fit = ImClamp(window->SizeContents + style.AutoFitPadding, style.WindowMinSize, ImMax(style.WindowMinSize, g.IO.DisplaySize - style.AutoFitPadding));
            if (size_auto_fit.y < window->SizeContents.y + style.AutoFitPadding.y)
                size_auto_fit.x += style.ScrollbarWidth;
        }

        const float window_rounding = (window->Flags & ImGuiWindowFlags_ChildWindow) ? style.ChildWindowRounding : style.WindowRounding;
        if (window->Collapsed)
        {
            // We still process initial auto-fit on collapsed windows to get a window width
            // But otherwise we don't honor ImGuiWindowFlags_AlwaysAutoResize when collapsed.
            if (window->AutoFitFrames > 0)
            {
                window->SizeFull = window->AutoFitOnlyGrows ? ImMax(window->SizeFull, size_auto_fit) : size_auto_fit;
                title_bar_rect = window->TitleBarRect();
            }
        
            // Draw title bar only
            window->Size = title_bar_rect.GetSize();
            window->DrawList->AddRectFilled(title_bar_rect.GetTL(), title_bar_rect.GetBR(), window->Color(ImGuiCol_TitleBgCollapsed), window_rounding);
            if (window->Flags & ImGuiWindowFlags_ShowBorders)
            {
                window->DrawList->AddRect(title_bar_rect.GetTL()+ImVec2(1,1), title_bar_rect.GetBR()+ImVec2(1,1), window->Color(ImGuiCol_BorderShadow), window_rounding);
                window->DrawList->AddRect(title_bar_rect.GetTL(), title_bar_rect.GetBR(), window->Color(ImGuiCol_Border), window_rounding);
            }
        }
        else
        {
            ImU32 resize_col = 0;
            if ((window->Flags & ImGuiWindowFlags_Tooltip) != 0)
            {
                window->Size = window->SizeFull = size_auto_fit;
            }
            else
            {
                if ((window->Flags & ImGuiWindowFlags_AlwaysAutoResize) != 0)
                {
                    // Don't continuously mark settings as dirty, the size of the window doesn't need to be stored.
                    window->SizeFull = size_auto_fit;
                }
                else if (window->AutoFitFrames > 0)
                {
                    // Auto-fit only grows during the first few frames
                    window->SizeFull = window->AutoFitOnlyGrows ? ImMax(window->SizeFull, size_auto_fit) : size_auto_fit;
                    if (!(window->Flags & ImGuiWindowFlags_NoSavedSettings))
                        MarkSettingsDirty();
                }
                else if (!(window->Flags & ImGuiWindowFlags_NoResize))
                {
                    // Manual resize grip
                    const ImRect resize_rect(window->Rect().GetBR()-ImVec2(14,14), window->Rect().GetBR());
                    const ImGuiID resize_id = window->GetID("#RESIZE");
                    bool hovered, held;
                    ButtonBehavior(resize_rect, resize_id, &hovered, &held, true);
                    resize_col = window->Color(held ? ImGuiCol_ResizeGripActive : hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip);

                    if (hovered || held)
                        g.MouseCursor = ImGuiMouseCursor_ResizeNWSE;

                    if (g.HoveredWindow == window && held && g.IO.MouseDoubleClicked[0])
                    {
                        // Manual auto-fit when double-clicking
                        window->SizeFull = size_auto_fit;
                        if (!(window->Flags & ImGuiWindowFlags_NoSavedSettings))
                            MarkSettingsDirty();
                        SetActiveId(0);
                    }
                    else if (held)
                    {
                        // Resize
                        window->SizeFull = ImMax(window->SizeFull + g.IO.MouseDelta, style.WindowMinSize);
                        if (!(window->Flags & ImGuiWindowFlags_NoSavedSettings))
                            MarkSettingsDirty();
                    }
                }

                // Update rectangle immediately so that rendering right below us isn't one frame late
                window->Size = window->SizeFull;
                title_bar_rect = window->TitleBarRect();
            }

            // Scrollbar
            window->ScrollbarY = (window->SizeContents.y > window->Size.y) && !(window->Flags & ImGuiWindowFlags_NoScrollbar);

            // Window background
            if (bg_alpha > 0.0f)
            {
                if ((window->Flags & ImGuiWindowFlags_ComboBox) != 0)
                    window->DrawList->AddRectFilled(window->Pos, window->Pos+window->Size, window->Color(ImGuiCol_ComboBg, bg_alpha), window_rounding);
                else if ((window->Flags & ImGuiWindowFlags_Tooltip) != 0)
                    window->DrawList->AddRectFilled(window->Pos, window->Pos+window->Size, window->Color(ImGuiCol_TooltipBg, bg_alpha), window_rounding);
                else if ((window->Flags & ImGuiWindowFlags_ChildWindow) != 0)
                    window->DrawList->AddRectFilled(window->Pos, window->Pos+window->Size-ImVec2(window->ScrollbarY?style.ScrollbarWidth:0.0f,0.0f), window->Color(ImGuiCol_ChildWindowBg, bg_alpha), window_rounding, window->ScrollbarY ? (1|8) : (0xF));
                else
                    window->DrawList->AddRectFilled(window->Pos, window->Pos+window->Size, window->Color(ImGuiCol_WindowBg, bg_alpha), window_rounding);
            }

            // Title bar
            if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
                window->DrawList->AddRectFilled(title_bar_rect.GetTL(), title_bar_rect.GetBR(), window->Color(ImGuiCol_TitleBg), window_rounding, 1|2);

            // Borders
            if (window->Flags & ImGuiWindowFlags_ShowBorders)
            {
                window->DrawList->AddRect(window->Pos+ImVec2(1,1), window->Pos+window->Size+ImVec2(1,1), window->Color(ImGuiCol_BorderShadow), window_rounding);
                window->DrawList->AddRect(window->Pos, window->Pos+window->Size, window->Color(ImGuiCol_Border), window_rounding);
                if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
                    window->DrawList->AddLine(title_bar_rect.GetBL(), title_bar_rect.GetBR(), window->Color(ImGuiCol_Border));
            }

            // Scrollbar
            if (window->ScrollbarY)
                Scrollbar(window);

            // Render resize grip
            // (after the input handling so we don't have a frame of latency)
            if (!(window->Flags & ImGuiWindowFlags_NoResize))
            {
                const float r = window_rounding;
                const ImVec2 br = window->Rect().GetBR();
                if (r == 0.0f)
                {
                    window->DrawList->AddTriangleFilled(br, br-ImVec2(0,14), br-ImVec2(14,0), resize_col);
                }
                else
                {
                    // FIXME: We should draw 4 triangles and decide on a size that's not dependent on the rounding size (previously used 18)
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
        window->DC.CursorMaxPos = window->DC.CursorStartPos;
        window->DC.CurrentLineHeight = window->DC.PrevLineHeight = 0.0f;
        window->DC.CurrentLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
        window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f;
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
        window->DC.GroupStack.resize(0);

        if (window->AutoFitFrames > 0)
            window->AutoFitFrames--;

        // Title bar
        if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
        {
            if (p_opened != NULL)
                CloseWindowButton(p_opened);

            ImVec2 text_min = window->Pos + style.FramePadding;
            if (!(window->Flags & ImGuiWindowFlags_NoCollapse))
            {
                RenderCollapseTriangle(window->Pos + style.FramePadding, !window->Collapsed, 1.0f, true);
                text_min.x += g.FontSize + style.ItemInnerSpacing.x;
            }

            const ImVec2 text_size = CalcTextSize(name, NULL, true);
            const ImVec2 text_max = window->Pos + ImVec2(window->Size.x - (p_opened ? (title_bar_rect.GetHeight()-3) : style.FramePadding.x), style.FramePadding.y*2 + text_size.y);
            RenderTextClipped(text_min, name, NULL, &text_size, text_max);
        }
        if (window->Flags & ImGuiWindowFlags_Popup)
        {
            if (p_opened)
            {
                if (g.IO.MouseClicked[0] && (!g.HoveredWindow || g.HoveredWindow->RootWindow != window))
                    *p_opened = false;
                else if (!g.FocusedWindow)
                    *p_opened = false;
                else if (g.FocusedWindow->RootWindow != window)// && !(g.FocusedWindow->RootWindow->Flags & ImGuiWindowFlags_Tooltip))
                    *p_opened = false;
            }
        }

        // Save clipped aabb so we can access it in constant-time in FindHoveredWindow()
        window->ClippedRect = window->Rect();
        window->ClippedRect.Clip(window->ClipRectStack.front());

        // Pressing CTRL+C while holding on a window copy its content to the clipboard
        // This works but 1. doesn't handle multiple Begin/End pairs, 2. recursing into another Begin/End pair - so we need to work that out and add better logging scope.
        // Maybe we can support CTRL+C on every element?
        /*
        if (g.ActiveId == move_id)
            if (g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_C))
                ImGui::LogToClipboard();
        */
    }

    // Inner clipping rectangle
    // We set this up after processing the resize grip so that our clip rectangle doesn't lag by a frame
    // Note that if our window is collapsed we will end up with a null clipping rectangle which is the correct behavior.
    const ImRect title_bar_rect = window->TitleBarRect();
    ImVec4 clip_rect(title_bar_rect.Min.x+0.5f+window->WindowPadding().x*0.5f, title_bar_rect.Max.y+0.5f, window->Rect().Max.x+0.5f-window->WindowPadding().x*0.5f, window->Rect().Max.y-1.5f);
    if (window->ScrollbarY)
        clip_rect.z -= style.ScrollbarWidth;
    PushClipRect(clip_rect);

    // Clear 'accessed' flag last thing
    if (first_begin_of_the_frame)
        window->Accessed = false;

    // Child window can be out of sight and have "negative" clip windows.
    // Mark them as collapsed so commands are skipped earlier (we can't manually collapse because they have no title bar).
    if (flags & ImGuiWindowFlags_ChildWindow)
    {
        IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0);
        window->Collapsed = parent_window && parent_window->Collapsed;

        const ImVec4 clip_rect_t = window->ClipRectStack.back();
        window->Collapsed |= (clip_rect_t.x >= clip_rect_t.z || clip_rect_t.y >= clip_rect_t.w);

        // We also hide the window from rendering because we've already added its border to the command list.
        // (we could perform the check earlier in the function but it is simpler at this point)
        if (window->Collapsed)
            window->Visible = false;
    }
    if (style.Alpha <= 0.0f)
        window->Visible = false;

    // Return false if we don't intend to display anything to allow user to perform an early out optimization
    window->SkipItems = (window->Collapsed || !window->Visible) && window->AutoFitFrames == 0;
    return !window->SkipItems;
}

void ImGui::End()
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImGui::Columns(1, "#CloseColumns");
    PopClipRect();   // inner window clip rectangle
    PopClipRect();   // outer window clip rectangle
    window->DrawList->PopTextureID();

    // Stop logging
    if (!(window->Flags & ImGuiWindowFlags_ChildWindow))    // FIXME: add more options for scope of logging
        ImGui::LogFinish();

    // Pop
    // NB: we don't clear 'window->RootWindow'. The pointer is allowed to live until the next call to Begin().
    g.CurrentWindowStack.pop_back();
    SetCurrentWindow(g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back());
}

// Vertical scrollbar
// The entire piece of code below is rather confusing because:
// - We handle absolute seeking (when first clicking outside the grab) and relative manipulation (afterward or when clicking inside the grab)
// - We store values as ratio and in a form that allows the window content to change while we are holding on a scrollbar
static void Scrollbar(ImGuiWindow* window)
{
    ImGuiState& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID("#SCROLLY");

    // Render background
    ImRect bb(window->Rect().Max.x - style.ScrollbarWidth, window->Pos.y + window->TitleBarHeight()+1, window->Rect().Max.x, window->Rect().Max.y-1);
    window->DrawList->AddRectFilled(bb.Min, bb.Max, window->Color(ImGuiCol_ScrollbarBg));
    bb.Expand(-3.0f);
    const float scrollbar_height = bb.GetHeight();

    // The grabable box size generally represent the amount visible (vs the total scrollable amount)
    // But we maintain a minimum size in pixel to allow for the user to still aim inside.
    const float grab_h_pixels = ImMax(style.GrabMinSize, scrollbar_height * ImSaturate(window->Size.y / ImMax(window->SizeContents.y, window->Size.y)));
    const float grab_h_norm = grab_h_pixels / scrollbar_height;

    // Handle input right away. None of the code of Begin() is relying on scrolling position before calling Scrollbar().
    bool held = false;
    bool hovered = false;
    const bool previously_held = (g.ActiveId == id);
    ButtonBehavior(bb, id, &hovered, &held, true);

    const float scroll_max = ImMax(1.0f, window->SizeContents.y - window->Size.y);
    float scroll_ratio = ImSaturate(window->ScrollY / scroll_max);
    float grab_y_norm = scroll_ratio * (scrollbar_height - grab_h_pixels) / scrollbar_height;
    if (held)
    {
        const float clicked_y_norm = ImSaturate((g.IO.MousePos.y - bb.Min.y) / scrollbar_height);     // Click position in scrollbar space (0.0f->1.0f)
        g.HoveredId = id;

        bool seek_absolute = false;
        if (!previously_held)
        {
            // On initial click calculate the distance between mouse and the center of the grab
            if (clicked_y_norm >= grab_y_norm && clicked_y_norm <= grab_y_norm + grab_h_norm)
            {
                g.ScrollbarClickDeltaToGrabCenter = clicked_y_norm - grab_y_norm - grab_h_norm*0.5f;
            }
            else
            {
                seek_absolute = true;
                g.ScrollbarClickDeltaToGrabCenter = 0;
            }
        }

        // Apply scroll
        const float scroll_y_norm = ImSaturate((clicked_y_norm - g.ScrollbarClickDeltaToGrabCenter - grab_h_norm*0.5f) / (1.0f - grab_h_norm));
        window->ScrollY = (float)(int)(0.5f + scroll_y_norm * (window->SizeContents.y - window->Size.y));
        window->NextScrollY = window->ScrollY;

        // Update values for rendering
        scroll_ratio = ImSaturate(window->ScrollY / scroll_max);
        grab_y_norm = scroll_ratio * (scrollbar_height - grab_h_pixels) / scrollbar_height;

        // Update distance to grab now that we have seeked and saturated
        if (seek_absolute)
            g.ScrollbarClickDeltaToGrabCenter = clicked_y_norm - grab_y_norm - grab_h_norm*0.5f;
    }

    // Render
    const ImU32 grab_col = window->Color(held ? ImGuiCol_ScrollbarGrabActive : hovered ? ImGuiCol_ScrollbarGrabHovered : ImGuiCol_ScrollbarGrab);
    window->DrawList->AddRectFilled(ImVec2(bb.Min.x, ImLerp(bb.Min.y, bb.Max.y, grab_y_norm)), ImVec2(bb.Max.x, ImLerp(bb.Min.y, bb.Max.y, grab_y_norm) + grab_h_pixels), grab_col);
}

// Moving window to front of display (which happens to be back of our sorted list)
static void FocusWindow(ImGuiWindow* window)
{
    ImGuiState& g = *GImGui;

    // Always mark the window we passed as focused. This is used for keyboard interactions such as tabbing.
    g.FocusedWindow = window;

    // And move its root window to the top of the pile 
    if (window->RootWindow)
        window = window->RootWindow;

    if (g.Windows.back() == window)
        return;

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
    window->DC.ItemWidth.push_back(item_width == 0.0f ? window->ItemWidthDefault : item_width);
}

void ImGui::PopItemWidth()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ItemWidth.pop_back();
}

float ImGui::CalcItemWidth()
{
    ImGuiWindow* window = GetCurrentWindow();
    float w = window->DC.ItemWidth.back();
    if (w < 0.0f)
    {
        // Align to a right-side limit. We include 1 frame padding in the calculation because this is how the width is always used (we add 2 frame padding to it), but we could move that responsibility to the widget as well.
        ImGuiState& g = *GImGui;
        w = -w;
        float width_to_right_edge = window->Pos.x + ImGui::GetContentRegionMax().x - window->DC.CursorPos.x;
        w = ImMax(1.0f, width_to_right_edge - w - g.Style.FramePadding.x * 2.0f);
    }
    w = (float)(int)w;
    return w;
}

static void SetFont(ImFont* font)
{
    ImGuiState& g = *GImGui;

    IM_ASSERT(font && font->IsLoaded());
    IM_ASSERT(font->Scale > 0.0f);
    g.Font = font;
    g.FontBaseSize = g.IO.FontGlobalScale * g.Font->FontSize * g.Font->Scale;
    g.FontSize = g.CurrentWindow ? g.CurrentWindow->CalcFontSize() : 0.0f;
    g.FontTexUvWhitePixel = g.Font->ContainerAtlas->TexUvWhitePixel;
}

void ImGui::PushFont(ImFont* font)
{
    ImGuiState& g = *GImGui;

    if (!font)
        font = g.IO.Fonts->Fonts[0];

    SetFont(font);
    g.FontStack.push_back(font);
    g.CurrentWindow->DrawList->PushTextureID(font->ContainerAtlas->TexID);
}

void  ImGui::PopFont()
{
    ImGuiState& g = *GImGui;

    g.CurrentWindow->DrawList->PopTextureID();
    g.FontStack.pop_back();
    SetFont(g.FontStack.empty() ? g.IO.Fonts->Fonts[0] : g.FontStack.back());
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
    ImGuiState& g = *GImGui;

    ImGuiColMod backup;
    backup.Col = idx;
    backup.PreviousValue = g.Style.Colors[idx];
    g.ColorModifiers.push_back(backup);
    g.Style.Colors[idx] = col;
}

void ImGui::PopStyleColor(int count)
{
    ImGuiState& g = *GImGui;

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
    ImGuiState& g = *GImGui;
    switch (idx)
    {
    case ImGuiStyleVar_Alpha: return &g.Style.Alpha;
    case ImGuiStyleVar_WindowRounding: return &g.Style.WindowRounding;
    case ImGuiStyleVar_ChildWindowRounding: return &g.Style.ChildWindowRounding;
    case ImGuiStyleVar_FrameRounding: return &g.Style.FrameRounding;
    case ImGuiStyleVar_IndentSpacing: return &g.Style.IndentSpacing;
    case ImGuiStyleVar_GrabMinSize: return &g.Style.GrabMinSize;
    }
    return NULL;
}

static ImVec2* GetStyleVarVec2Addr(ImGuiStyleVar idx)
{
    ImGuiState& g = *GImGui;
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
    ImGuiState& g = *GImGui;

    float* pvar = GetStyleVarFloatAddr(idx);
    IM_ASSERT(pvar != NULL); // Called function with wrong-type? Variable is not a float.
    ImGuiStyleMod backup;
    backup.Var = idx;
    backup.PreviousValue = ImVec2(*pvar, 0.0f);
    g.StyleModifiers.push_back(backup);
    *pvar = val;
}


void ImGui::PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
{
    ImGuiState& g = *GImGui;

    ImVec2* pvar = GetStyleVarVec2Addr(idx);
    IM_ASSERT(pvar != NULL); // Called function with wrong-type? Variable is not a ImVec2.
    ImGuiStyleMod backup;
    backup.Var = idx;
    backup.PreviousValue = *pvar;
    g.StyleModifiers.push_back(backup);
    *pvar = val;
}

void ImGui::PopStyleVar(int count)
{
    ImGuiState& g = *GImGui;

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

const char* ImGui::GetStyleColName(ImGuiCol idx)
{
    // Create switch-case from enum with regexp: ImGuiCol_{.*}, --> case ImGuiCol_\1: return "\1";
    switch (idx)
    {
    case ImGuiCol_Text: return "Text";
    case ImGuiCol_WindowBg: return "WindowBg";
    case ImGuiCol_ChildWindowBg: return "ChildWindowBg";
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
    case ImGuiCol_CheckMark: return "CheckMark";
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

bool ImGui::IsWindowFocused()
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    return g.FocusedWindow == window;
}

bool ImGui::IsRootWindowFocused()
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* root_window = GetCurrentWindow()->RootWindow;
    return g.FocusedWindow == root_window;
}

bool ImGui::IsRootWindowOrAnyChildFocused()
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* root_window = GetCurrentWindow()->RootWindow;
    return g.FocusedWindow->RootWindow == root_window;
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

static void SetWindowPos(ImGuiWindow* window, const ImVec2& pos, ImGuiSetCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowPosAllowFlags & cond) == 0)
        return;
    window->SetWindowPosAllowFlags &= ~(ImGuiSetCond_Once | ImGuiSetCond_FirstUseEver);

    // Set
    const ImVec2 old_pos = window->Pos;
    window->PosFloat = pos;
    window->Pos = ImVec2((float)(int)window->PosFloat.x, (float)(int)window->PosFloat.y);
    window->DC.CursorPos += (window->Pos - old_pos);    // As we happen to move the window while it is being appended to (which is a bad idea - will smear) let's at least offset the cursor
    window->DC.CursorMaxPos += (window->Pos - old_pos); // And more importantly we need to adjust this so size calculation doesn't get affected.
}

void ImGui::SetWindowPos(const ImVec2& pos, ImGuiSetCond cond)
{
    ImGuiWindow* window = GetCurrentWindow();
    SetWindowPos(window, pos, cond);
}

void ImGui::SetWindowPos(const char* name, const ImVec2& pos, ImGuiSetCond cond)
{
    ImGuiWindow* window = FindWindowByName(name);
    if (window)
        SetWindowPos(window, pos, cond);
}

ImVec2 ImGui::GetWindowSize()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->Size;
}

static void SetWindowSize(ImGuiWindow* window, const ImVec2& size, ImGuiSetCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowSizeAllowFlags & cond) == 0)
        return;
    window->SetWindowSizeAllowFlags &= ~(ImGuiSetCond_Once | ImGuiSetCond_FirstUseEver);

    // Set
    if (ImLengthSqr(size) > 0.00001f)
    {
        window->SizeFull = size;
        window->AutoFitFrames = 0;
    }
    else
    {
        // Autofit
        window->AutoFitFrames = 2;
        window->AutoFitOnlyGrows = false;
    }
}

void ImGui::SetWindowSize(const ImVec2& size, ImGuiSetCond cond)
{
    ImGuiWindow* window = GetCurrentWindow();
    SetWindowSize(window, size, cond);
}

void ImGui::SetWindowSize(const char* name, const ImVec2& size, ImGuiSetCond cond)
{
    ImGuiWindow* window = FindWindowByName(name);
    if (window)
        SetWindowSize(window, size, cond);
}

static void SetWindowCollapsed(ImGuiWindow* window, bool collapsed, ImGuiSetCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowCollapsedAllowFlags & cond) == 0)
        return;
    window->SetWindowCollapsedAllowFlags &= ~(ImGuiSetCond_Once | ImGuiSetCond_FirstUseEver);

    // Set
    window->Collapsed = collapsed;
}

void ImGui::SetWindowCollapsed(bool collapsed, ImGuiSetCond cond)
{
    ImGuiWindow* window = GetCurrentWindow();
    SetWindowCollapsed(window, collapsed, cond);
}

bool ImGui::GetWindowCollapsed()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->Collapsed;
}

void ImGui::SetWindowCollapsed(const char* name, bool collapsed, ImGuiSetCond cond)
{
    ImGuiWindow* window = FindWindowByName(name);
    if (window)
        SetWindowCollapsed(window, collapsed, cond);
}

void ImGui::SetWindowFocus()
{
    ImGuiWindow* window = GetCurrentWindow();
    FocusWindow(window);
}

void ImGui::SetWindowFocus(const char* name)
{
    ImGuiWindow* window = FindWindowByName(name);
    if (window)
        FocusWindow(window);
}

void ImGui::SetNextWindowPos(const ImVec2& pos, ImGuiSetCond cond)
{
    ImGuiState& g = *GImGui;
    g.SetNextWindowPosVal = pos;
    g.SetNextWindowPosCond = cond ? cond : ImGuiSetCond_Always;
}

void ImGui::SetNextWindowSize(const ImVec2& size, ImGuiSetCond cond)
{
    ImGuiState& g = *GImGui;
    g.SetNextWindowSizeVal = size;
    g.SetNextWindowSizeCond = cond ? cond : ImGuiSetCond_Always;
}

void ImGui::SetNextWindowCollapsed(bool collapsed, ImGuiSetCond cond)
{
    ImGuiState& g = *GImGui;
    g.SetNextWindowCollapsedVal = collapsed;
    g.SetNextWindowCollapsedCond = cond ? cond : ImGuiSetCond_Always;
}

void ImGui::SetNextWindowFocus()
{
    ImGuiState& g = *GImGui;
    g.SetNextWindowFocus = true;
}

ImVec2 ImGui::GetContentRegionMax()
{
    ImGuiWindow* window = GetCurrentWindow();
    ImVec2 window_padding = window->WindowPadding();
    ImVec2 mx = window->Size - window_padding;
    if (window->DC.ColumnsCount != 1)
    {
        mx.x = ImGui::GetColumnOffset(window->DC.ColumnsCurrent + 1);
        mx.x -= window_padding.x;
    }
    else
    {
        if (window->ScrollbarY)
            mx.x -= GImGui->Style.ScrollbarWidth;
    }
    return mx;
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
        m.x -= GImGui->Style.ScrollbarWidth;
    return m;
}

float ImGui::GetTextLineHeight()
{
    ImGuiState& g = *GImGui;
    return g.FontSize;
}

float ImGui::GetTextLineHeightWithSpacing()
{
    ImGuiState& g = *GImGui;
    return g.FontSize + g.Style.ItemSpacing.y;
}

ImDrawList* ImGui::GetWindowDrawList()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DrawList;
}

ImFont* ImGui::GetWindowFont()
{
    ImGuiState& g = *GImGui;
    return g.Font;
}

float ImGui::GetWindowFontSize()
{
    ImGuiState& g = *GImGui;
    return g.FontSize;
}

void ImGui::SetWindowFontScale(float scale)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->FontWindowScale = scale;
    g.FontSize = window->CalcFontSize();
}

// NB: internally we store CursorPos in absolute screen coordinates because it is more convenient.
// Conversion happens as we pass the value to user, but it makes our naming convention dodgy. May want to rename 'DC.CursorPos'.
ImVec2 ImGui::GetCursorPos()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.CursorPos - window->Pos;
}

float ImGui::GetCursorPosX()
{
    return ImGui::GetCursorPos().x;
}

float ImGui::GetCursorPosY()
{
    return ImGui::GetCursorPos().y;
}

void ImGui::SetCursorPos(const ImVec2& pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos = window->Pos + pos;
    window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
}

void ImGui::SetCursorPosX(float x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.x = window->Pos.x + x;
    window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPos.x);
}

void ImGui::SetCursorPosY(float y)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.y = window->Pos.y + y;
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, window->DC.CursorPos.y);
}

ImVec2 ImGui::GetCursorScreenPos()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.CursorPos;
}

void ImGui::SetCursorScreenPos(const ImVec2& screen_pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos = screen_pos;
}

float ImGui::GetScrollPosY()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->ScrollY;
}

float ImGui::GetScrollMaxY()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->SizeContents.y - window->SizeFull.y;
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

void ImGui::SetStateStorage(ImGuiStorage* tree)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.StateStorage = tree ? tree : &window->StateStorage;
}

ImGuiStorage* ImGui::GetStateStorage()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.StateStorage;
}

void ImGui::TextV(const char* fmt, va_list args)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const char* text_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
    TextUnformatted(g.TempBuffer, text_end);
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
    ImGuiState& g = *GImGui;
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
        // From this point we will only compute the width of lines that are visible. Optimization only available when word-wrapping is disabled. 
        // We also don't vertically center the text within the line full height, which is unlikely to matter because we are likely the biggest and only item on the line.
        const char* line = text;
        const float line_height = ImGui::GetTextLineHeight();
        const ImVec2 text_pos = window->DC.CursorPos + ImVec2(0.0f, window->DC.CurrentLineTextBaseOffset);
        const ImVec4 clip_rect = window->ClipRectStack.back();
        ImVec2 text_size(0,0);

        if (text_pos.y <= clip_rect.w)
        {
            ImVec2 pos = text_pos;

            // Lines to skip (can't skip when logging text)
            if (!g.LogEnabled)
            {
                int lines_skippable = (int)((clip_rect.y - text_pos.y) / line_height) - 1;
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
                ImRect line_rect(pos, pos + ImVec2(ImGui::GetWindowWidth(), line_height));
                while (line < text_end)
                {
                    const char* line_end = strchr(line, '\n');
                    if (IsClipped(line_rect))
                        break;

                    const ImVec2 line_size = CalcTextSize(line, line_end, false);
                    text_size.x = ImMax(text_size.x, line_size.x);
                    RenderText(pos, line, line_end, false);
                    if (!line_end)
                        line_end = text_end;
                    line = line_end + 1;
                    line_rect.Min.y += line_height;
                    line_rect.Max.y += line_height;
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

            text_size.y += (pos - text_pos).y;
        }

        ImRect bb(text_pos, text_pos + text_size);
        ItemSize(bb);
        ItemAdd(bb, NULL);
    }
    else
    {
        const float wrap_width = wrap_enabled ? CalcWrapWidthForPos(window->DC.CursorPos, wrap_pos_x) : 0.0f;
        const ImVec2 text_size = CalcTextSize(text_begin, text_end, false, wrap_width);

        // Account of baseline offset
        ImVec2 text_pos = window->DC.CursorPos;
        text_pos.y += window->DC.CurrentLineTextBaseOffset;

        ImRect bb(text_pos, text_pos + text_size);
        ItemSize(bb.GetSize());
        if (!ItemAdd(bb, NULL))
            return;

        // Render (we don't hide text after ## in this end-user function)
        RenderTextWrapped(bb.Min, text_begin, text_end, wrap_width);
    }
}

void ImGui::AlignFirstTextHeightToWidgets()
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    // Declare a dummy item size to that upcoming items that are smaller will center-align on the newly expanded line height.
    ItemSize(ImVec2(0, g.FontSize + g.Style.FramePadding.y*2), g.Style.FramePadding.y);
    ImGui::SameLine(0, 0);
}

// Add a label+text combo aligned to other label+value widgets
void ImGui::LabelTextV(const char* label, const char* fmt, va_list args)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const ImGuiStyle& style = g.Style;
    const float w = ImGui::CalcItemWidth();

    const char* value_text_begin = &g.TempBuffer[0];
    const char* value_text_end = value_text_begin + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect value_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w + style.FramePadding.x*2, label_size.y + style.FramePadding.y*2));
    const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w + style.FramePadding.x*2 + (label_size.x > 0.0f ? style.ItemInnerSpacing.x : 0.0f), style.FramePadding.y*2) + label_size);
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, NULL))
        return;

    // Render
    RenderTextClipped(ImVec2(value_bb.Min.x, value_bb.Min.y + style.FramePadding.y), value_text_begin, value_text_end, NULL, value_bb.Max);
    RenderText(ImVec2(value_bb.Max.x + style.ItemInnerSpacing.x, value_bb.Min.y + style.FramePadding.y), label);
}

void ImGui::LabelText(const char* label, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LabelTextV(label, fmt, args);
    va_end(args);
}

static inline bool IsWindowContentHoverable(ImGuiWindow* window)
{
    ImGuiState& g = *GImGui;

    ImGuiWindow* focused_window = g.FocusedWindow;
    if (focused_window && (focused_window->Flags & ImGuiWindowFlags_Popup) != 0 && focused_window->WasVisible && focused_window != window)
        return false;

    return true;
}

static bool IsHovered(const ImRect& bb, ImGuiID id)
{
    ImGuiState& g = *GImGui;
    if (g.HoveredId == 0)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (g.HoveredRootWindow == window->RootWindow)
        {
            if ((g.ActiveId == 0 || g.ActiveId == id || g.ActiveIdIsFocusedOnly) && IsMouseHoveringRect(bb))
                if (IsWindowContentHoverable(g.HoveredRootWindow))
                    return true;
        }
    }
    return false;
}

static bool ButtonBehavior(const ImRect& bb, ImGuiID id, bool* out_hovered, bool* out_held, bool allow_key_modifiers, bool repeat, bool pressed_on_click)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    const bool hovered = IsHovered(bb, id);
    bool pressed = false;
    if (hovered)
    {
        g.HoveredId = id;
        if (allow_key_modifiers || (!g.IO.KeyCtrl && !g.IO.KeyShift && !g.IO.KeyAlt))
        {
            if (g.IO.MouseClicked[0])
            {
                if (pressed_on_click)
                {
                    pressed = true;
                    SetActiveId(0);
                }
                else
                {
                    SetActiveId(id);
                }
                FocusWindow(window);
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
            SetActiveId(0);
        }
    }

    if (out_hovered) *out_hovered = hovered;
    if (out_held) *out_held = held;

    return pressed;
}

bool ImGui::Button(const char* label, const ImVec2& size_arg, bool repeat_when_held)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const ImVec2 size(size_arg.x != 0.0f ? size_arg.x : (label_size.x + style.FramePadding.x*2), size_arg.y != 0.0f ? size_arg.y : (label_size.y + style.FramePadding.y*2));
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, &id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, true, repeat_when_held);

    // Render
    const ImU32 col = window->Color((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

    const ImVec2 off = ImVec2(ImMax(0.0f, size.x - label_size.x) * 0.5f, ImMax(0.0f, size.y - label_size.y) * 0.5f); // Center (only applies if we explicitly gave a size bigger than the text size, which isn't the common path)
    RenderTextClipped(bb.Min + off, label, NULL, &label_size, bb.Max);                          // Render clip (only applies if we explicitly gave a size smaller than the text size, which isn't the commmon path)

    return pressed;
}

// Small buttons fits within text without additional spacing.
bool ImGui::SmallButton(const char* label)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 text_pos = window->DC.CursorPos;
    text_pos.y += window->DC.CurrentLineTextBaseOffset;
    ImRect bb(text_pos, text_pos + label_size + ImVec2(style.FramePadding.x*2,0));
    ItemSize(bb);
    if (!ItemAdd(bb, &id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, true);

    // Render
    const ImU32 col = window->Color((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    RenderFrame(bb.Min, bb.Max, col);
    RenderText(bb.Min + ImVec2(style.FramePadding.x,0), label);

    return pressed;
}

// Tip: use ImGui::PushID()/PopID() to push indices or pointers in the ID stack.
// Then you can keep 'str_id' empty or the same for all your buttons (instead of creating a string based on a non-string id)
bool ImGui::InvisibleButton(const char* str_id, const ImVec2& size)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(str_id);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(bb);
    if (!ItemAdd(bb, &id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, true);

    return pressed;
}

// Upper-right button to close a window.
static bool CloseWindowButton(bool* p_opened)
{
    ImGuiWindow* window = GetCurrentWindow();

    const ImGuiID id = window->GetID("#CLOSE");
    const float size = window->TitleBarHeight() - 4.0f;
    const ImRect bb(window->Rect().GetTR() + ImVec2(-3.0f-size,2.0f), window->Rect().GetTR() + ImVec2(-3.0f,2.0f+size));

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, true);

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

    if (p_opened != NULL && pressed)
        *p_opened = !*p_opened;

    return pressed;
}

void ImGui::Image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    if (border_col.w > 0.0f)
        bb.Max += ImVec2(2,2);
    ItemSize(bb);
    if (!ItemAdd(bb, NULL))
        return;

    if (border_col.w > 0.0f)
    {
        window->DrawList->AddRect(bb.Min, bb.Max, window->Color(border_col), 0.0f);
        window->DrawList->AddImage(user_texture_id, bb.Min+ImVec2(1,1), bb.Max-ImVec2(1,1), uv0, uv1, window->Color(tint_col));
    }
    else
    {
        window->DrawList->AddImage(user_texture_id, bb.Min, bb.Max, uv0, uv1, window->Color(tint_col));
    }
}

// frame_padding < 0: uses FramePadding from style (default)
// frame_padding = 0: no framing
// frame_padding > 0: set framing size
// The color used are the button colors.
bool ImGui::ImageButton(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;

    // Default to using texture ID as ID. User can still push string/integer prefixes.
    // We could hash the size/uv to create a unique ID but that would prevent the user from animating buttons.
    ImGui::PushID((void *)user_texture_id);
    const ImGuiID id = window->GetID("#image");
    ImGui::PopID();

    const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : style.FramePadding;
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding*2);
    const ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size); 
    ItemSize(bb);
    if (!ItemAdd(bb, &id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, true);

    // Render
    const ImU32 col = window->Color((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    if (padding.x > 0.0f || padding.y > 0.0f)
        RenderFrame(bb.Min, bb.Max, col);
    if (bg_col.w > 0.0f)
        window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, window->Color(bg_col));
    window->DrawList->AddImage(user_texture_id, image_bb.Min, image_bb.Max, uv0, uv1, window->Color(tint_col));

    return pressed;
}

// Start logging ImGui output to TTY
void ImGui::LogToTTY(int max_depth)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (g.LogEnabled)
        return;

    g.LogEnabled = true;
    g.LogFile = stdout;
    g.LogStartDepth = window->DC.TreeDepth;
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

// Start logging ImGui output to given file
void ImGui::LogToFile(int max_depth, const char* filename)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (g.LogEnabled)
        return;
    if (!filename)
    {
        filename = g.IO.LogFilename;
        if (!filename)
            return;
    }

    g.LogFile = fopen(filename, "ab");
    if (!g.LogFile)
    {
        IM_ASSERT(g.LogFile != NULL); // Consider this an error
        return;
    }
    g.LogEnabled = true;
    g.LogStartDepth = window->DC.TreeDepth;
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

// Start logging ImGui output to clipboard
void ImGui::LogToClipboard(int max_depth)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiState& g = *GImGui;
    if (g.LogEnabled)
        return;

    g.LogEnabled = true;
    g.LogFile = NULL;
    g.LogStartDepth = window->DC.TreeDepth;
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

void ImGui::LogFinish()
{
    ImGuiState& g = *GImGui;
    if (!g.LogEnabled)
        return;

    ImGui::LogText(STR_NEWLINE);
    g.LogEnabled = false;
    if (g.LogFile != NULL)
    {
        if (g.LogFile == stdout)
            fflush(g.LogFile);
        else
            fclose(g.LogFile);
        g.LogFile = NULL;
    }
    if (g.LogClipboard->size() > 1)
    {
        if (g.IO.SetClipboardTextFn)
            g.IO.SetClipboardTextFn(g.LogClipboard->begin());
        g.LogClipboard->clear();
    }
}

// Helper to display logging buttons
void ImGui::LogButtons()
{
    ImGuiState& g = *GImGui;

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

bool ImGui::CollapsingHeader(const char* label, const char* str_id, bool display_frame, bool default_open)
{
    ImGuiState& g = *GImGui;
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

    // We only write to the tree storage if the user clicks (or explicitely use SetNextTreeNode*** functions)
    ImGuiStorage* storage = window->DC.StateStorage;
    bool opened;
    if (g.SetNextTreeNodeOpenedCond != 0)
    {
        if (g.SetNextTreeNodeOpenedCond & ImGuiSetCond_Always)
        {
            opened = g.SetNextTreeNodeOpenedVal;
            storage->SetInt(id, opened);
        }
        else
        {
            // We treat ImGuiSetCondition_Once and ImGuiSetCondition_FirstUseEver the same because tree node state are not saved persistently.
            const int stored_value = storage->GetInt(id, -1);
            if (stored_value == -1)
            {
                opened = g.SetNextTreeNodeOpenedVal;
                storage->SetInt(id, opened);
            }
            else
            {
                opened = stored_value != 0;
            }
        }
        g.SetNextTreeNodeOpenedCond = 0;
    }
    else
    {
        opened = storage->GetInt(id, default_open) != 0;
    }

    // Framed header expand a little outside the default padding
    const ImVec2 window_padding = window->WindowPadding();
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 pos_min = window->DC.CursorPos;
    const ImVec2 pos_max = window->Pos + GetContentRegionMax();
    ImRect bb = ImRect(pos_min, ImVec2(pos_max.x, pos_min.y + label_size.y));
    if (display_frame)
    {
        bb.Min.x -= window_padding.x*0.5f - 1;
        bb.Max.x += window_padding.x*0.5f - 1;
        bb.Max.y += style.FramePadding.y * 2;
    }

    // FIXME: we don't provide our width so that it doesn't get feed back into AutoFit. Should manage that better so we can still hover without extending ContentsSize
    const ImRect text_bb(bb.Min, bb.Min + ImVec2(g.FontSize + style.FramePadding.x*2*2,0) + label_size);
    ItemSize(ImVec2(text_bb.GetSize().x, bb.GetSize().y), display_frame ? style.FramePadding.y : 0.0f);

    // When logging is enabled, if automatically expand tree nodes (but *NOT* collapsing headers.. seems like sensible behavior).
    // NB- If we are above max depth we still allow manually opened nodes to be logged.
    if (g.LogEnabled && !display_frame && window->DC.TreeDepth < g.LogAutoExpandMaxDepth)
        opened = true;

    if (!ItemAdd(bb, &id))
        return opened;

    bool hovered, held;
    bool pressed = ButtonBehavior(display_frame ? bb : text_bb, id, &hovered, &held, false);
    if (pressed)
    {
        opened = !opened;
        storage->SetInt(id, opened);
    }

    // Render
    const ImU32 col = window->Color((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
    if (display_frame)
    {
        // Framed type
        RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
        RenderCollapseTriangle(bb.Min + style.FramePadding, opened, 1.0f, true);
        if (g.LogEnabled)
        {
            // NB: '##' is normally used to hide text (as a library-wide feature), so we need to specify the text range to make sure the ## aren't stripped out here.
            const char log_prefix[] = "\n##";
            LogText(bb.Min + style.FramePadding, log_prefix, log_prefix+3);
        }
        RenderText(bb.Min + style.FramePadding + ImVec2(g.FontSize + style.FramePadding.x*2,0), label);
        if (g.LogEnabled)
        {
            const char log_suffix[] = "##";
            LogText(bb.Min + style.FramePadding, log_suffix, log_suffix+2);
        }
    }
    else
    {
        // Unframed typed for tree nodes
        if ((held && hovered) || hovered)
            RenderFrame(bb.Min, bb.Max, col, false);
        RenderCollapseTriangle(bb.Min + ImVec2(style.FramePadding.x, g.FontSize*0.15f), opened, 0.70f, false);
        if (g.LogEnabled)
            LogText(bb.Min, ">");
        RenderText(bb.Min + ImVec2(g.FontSize + style.FramePadding.x*2,0), label);
    }

    return opened;
}

void ImGui::Bullet()
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const ImGuiStyle& style = g.Style;
    const float line_height = g.FontSize;
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(line_height, line_height));
    ItemSize(bb);
    if (!ItemAdd(bb, NULL))
        return;

    // Render
    const float bullet_size = line_height*0.15f;
    window->DrawList->AddCircleFilled(bb.Min + ImVec2(style.FramePadding.x + line_height*0.5f, line_height*0.5f), bullet_size, window->Color(ImGuiCol_Text));

    // Stay on same line
    ImGui::SameLine(0, -1);
}

// Text with a little bullet aligned to the typical tree node.
void ImGui::BulletTextV(const char* fmt, va_list args)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;
    
    const char* text_begin = g.TempBuffer;
    const char* text_end = text_begin + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);

    const ImGuiStyle& style = g.Style;
    const float line_height = g.FontSize;
    const ImVec2 label_size = CalcTextSize(text_begin, text_end, true);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(line_height + (label_size.x > 0.0f ? (style.FramePadding.x*2) : 0.0f),0) + label_size);  // Empty text doesn't add padding
    ItemSize(bb);
    if (!ItemAdd(bb, NULL))
        return;

    // Render
    const float bullet_size = line_height*0.15f;
    window->DrawList->AddCircleFilled(bb.Min + ImVec2(style.FramePadding.x + line_height*0.5f, line_height*0.5f), bullet_size, window->Color(ImGuiCol_Text));
    RenderText(bb.Min+ImVec2(g.FontSize + style.FramePadding.x*2,0), text_begin, text_end);
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
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);

    if (!str_id || !str_id[0])
        str_id = fmt;

    ImGui::PushID(str_id);
    const bool opened = ImGui::CollapsingHeader(g.TempBuffer, "", false);
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
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);

    if (!ptr_id)
        ptr_id = fmt;

    ImGui::PushID(ptr_id);
    const bool opened = ImGui::CollapsingHeader(g.TempBuffer, "", false);
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

void ImGui::SetNextTreeNodeOpened(bool opened, ImGuiSetCond cond)
{
    ImGuiState& g = *GImGui;
    g.SetNextTreeNodeOpenedVal = opened;
    g.SetNextTreeNodeOpenedCond = cond ? cond : ImGuiSetCond_Always;
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

ImGuiID ImGui::GetID(const char* str_id)
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->GetID(str_id);
}

ImGuiID ImGui::GetID(const void* ptr_id)
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->GetID(ptr_id);
}

// User can input math operators (e.g. +100) to edit a numerical values.
// NB: only call right after InputText because we are using its InitialValue storage
static void ApplyNumericalTextInput(const char* buf, float *v)
{
    while (ImCharIsSpace(*buf))
        buf++;

    // We don't support '-' op because it would conflict with inputing negative value.
    // Instead you can use +-100 to subtract from an existing value
    char op = buf[0];
    if (op == '+' || op == '*' || op == '/')
    {
        buf++;
        while (ImCharIsSpace(*buf))
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
        if (sscanf(GImGui->InputTextState.InitialText, "%f", &ref_v) < 1)
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

// Create text input in place of a slider (when CTRL+Clicking on slider)
static bool SliderFloatAsInputText(const char* label, float* v, ImGuiID id, int decimal_precision)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    char text_buf[64];
    ImFormatString(text_buf, IM_ARRAYSIZE(text_buf), "%.*f", decimal_precision, *v);

    SetActiveId(g.SliderAsInputTextId);
    g.HoveredId = 0;

    // Our replacement widget will override the focus ID (registered previously to allow for a TAB focus to happen)
    window->FocusItemUnregister();

    bool value_changed = ImGui::InputText(label, text_buf, IM_ARRAYSIZE(text_buf), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
    if (g.SliderAsInputTextId == 0)
    {
        // First frame
        IM_ASSERT(g.ActiveId == id);    // InputText ID expected to match the Slider ID (else we'd need to store them both, which is also possible)
        g.SliderAsInputTextId = g.ActiveId;
        g.HoveredId = id;
    }
    else if (g.ActiveId != g.SliderAsInputTextId)
    {
        // Release
        g.SliderAsInputTextId = 0;
    }
    if (value_changed)
    {
        ApplyNumericalTextInput(text_buf, v);
    }
    return value_changed;
}

// Parse display precision back from the display format string
static void ParseFormat(const char* fmt, int& decimal_precision)
{
    while ((fmt = strchr(fmt, '%')) != NULL)
    {
        fmt++;
        if (fmt[0] == '%') { fmt++; continue; } // Ignore "%%"
        while (*fmt >= '0' && *fmt <= '9')
            fmt++;
        if (*fmt == '.')
        {
            decimal_precision = atoi(fmt + 1);
            if (decimal_precision < 0 || decimal_precision > 10)
                decimal_precision = 3;
        }
        break;
    }
}

static bool SliderBehavior(const ImRect& frame_bb, const ImRect& slider_bb, ImGuiID id, float* v, float v_min, float v_max, float power, int decimal_precision, bool horizontal)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    const ImGuiStyle& style = g.Style;

    // Draw frame
    RenderFrame(frame_bb.Min, frame_bb.Max, window->Color(ImGuiCol_FrameBg), true, style.FrameRounding);

    const bool is_finite = (v_min != -FLT_MAX && v_min != FLT_MAX && v_max != -FLT_MAX && v_max != FLT_MAX);
    const bool is_non_linear = fabsf(power - 1.0f) > 0.0001f;

    const float slider_sz = horizontal ? slider_bb.GetWidth() : slider_bb.GetHeight();
    float grab_sz;
    if (decimal_precision > 0 || !is_finite)
        grab_sz = style.GrabMinSize;
    else
        grab_sz = ImMax(1.0f * (slider_sz / (v_max-v_min+1.0f)), style.GrabMinSize);    // Integer sliders, if possible have the grab size represent 1 unit
    const float slider_usable_sz = slider_sz - grab_sz;
    const float slider_usable_pos_min = (horizontal ? slider_bb.Min.x : slider_bb.Min.y) + grab_sz*0.5f;
    const float slider_usable_pos_max = (horizontal ? slider_bb.Max.x : slider_bb.Max.y) - grab_sz*0.5f;

    bool value_changed = false;

    if (is_finite)
    {
        // For logarithmic sliders that cross over sign boundary we want the exponential increase to be symmetric around 0.0f
        float linear_zero_pos = 0.0f;   // 0.0->1.0f
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

        // Process clicking on the slider
        if (g.ActiveId == id)
        {
            if (g.IO.MouseDown[0])
            {
                const float mouse_abs_pos = horizontal ? g.IO.MousePos.x : g.IO.MousePos.y;
                float normalized_pos = ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f);
                if (!horizontal)
                    normalized_pos = 1.0f - normalized_pos;

                float new_value;
                if (is_non_linear)
                {
                    // Account for logarithmic scale on both sides of the zero
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
                }
                else
                {
                    // Linear slider
                    new_value = ImLerp(v_min, v_max, normalized_pos);
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
            else
            {
                SetActiveId(0);
            }
        }

        // Calculate slider grab positioning
        float grab_t;
        if (is_non_linear)
        {
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
        }
        else
        {
            // Linear slider
            grab_t = (ImClamp(*v, v_min, v_max) - v_min) / (v_max - v_min);
        }

        // Draw
        if (!horizontal)
            grab_t = 1.0f - grab_t;
        const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
        ImRect grab_bb;
        if (horizontal)
            grab_bb = ImRect(ImVec2(grab_pos - grab_sz*0.5f, frame_bb.Min.y + 2.0f), ImVec2(grab_pos + grab_sz*0.5f, frame_bb.Max.y - 2.0f));
        else
            grab_bb = ImRect(ImVec2(frame_bb.Min.x + 2.0f, grab_pos - grab_sz*0.5f), ImVec2(frame_bb.Max.x - 2.0f, grab_pos + grab_sz*0.5f));
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, window->Color(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab));
    }

    return value_changed;
}

// Use power!=1.0 for logarithmic sliders.
// Adjust display_format to decorate the value with a prefix or a suffix.
//   "%.3f"         1.234
//   "%5.2f secs"   01.23 secs
//   "Gold: %.0f"   Gold: 1
bool ImGui::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* display_format, float power)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = ImGui::CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y) + style.FramePadding*2.0f);
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    // NB- we don't call ItemSize() yet becausae we may turn into a text edit box below
    if (!ItemAdd(total_bb, &id))
    {
        ItemSize(total_bb, style.FramePadding.y);
        return false;
    }

    const bool hovered = IsHovered(frame_bb, id);
    if (hovered)
        g.HoveredId = id;

    if (!display_format)
        display_format = "%.3f";
    int decimal_precision = 3;
    ParseFormat(display_format, decimal_precision);

    const bool tab_focus_requested = window->FocusItemRegister(g.ActiveId == id);
    const bool is_finite = (v_min != -FLT_MAX && v_min != FLT_MAX && v_max != -FLT_MAX && v_max != FLT_MAX);

    // Tabbing or CTRL-clicking through slider turns into an input box
    bool start_text_input = false;
    if (tab_focus_requested || (hovered && g.IO.MouseClicked[0]))
    {
        SetActiveId(id);
        FocusWindow(window);

        const bool is_ctrl_down = g.IO.KeyCtrl;
        if (tab_focus_requested || is_ctrl_down || !is_finite)
        {
            start_text_input = true;
            g.SliderAsInputTextId = 0;
        }
    }
    if (start_text_input || (g.ActiveId == id && id == g.SliderAsInputTextId))
        return SliderFloatAsInputText(label, v, id, decimal_precision);

    ItemSize(total_bb, style.FramePadding.y);

    // Actual slider behavior + render grab
    bool value_changed = SliderBehavior(frame_bb, inner_bb, id, v, v_min, v_max, power, decimal_precision, true);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    char* value_buf_end = value_buf + ImFormatString(value_buf, IM_ARRAYSIZE(value_buf), display_format, *v);
    const ImVec2 value_text_size = CalcTextSize(value_buf, value_buf_end, true);
    RenderTextClipped(ImVec2(ImMax(frame_bb.Min.x + style.FramePadding.x, inner_bb.GetCenter().x - value_text_size.x*0.5f), frame_bb.Min.y + style.FramePadding.y), value_buf, value_buf_end, &value_text_size, frame_bb.Max);

    RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);

    return value_changed;
}

bool ImGui::VSliderFloat(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* display_format, float power)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
    const ImRect slider_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(frame_bb, &id))
        return false;

    const bool hovered = IsHovered(frame_bb, id);
    if (hovered)
        g.HoveredId = id;

    if (!display_format)
        display_format = "%.3f";
    int decimal_precision = 3;
    ParseFormat(display_format, decimal_precision);

    if (hovered && g.IO.MouseClicked[0])
    {
        SetActiveId(id);
        FocusWindow(window);
    }

    // Actual slider behavior + render grab
    bool value_changed = SliderBehavior(frame_bb, slider_bb, id, v, v_min, v_max, power, decimal_precision, false);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    // For the vertical slider we allow centered text to overlap the frame padding
    char value_buf[64];
    char* value_buf_end = value_buf + ImFormatString(value_buf, IM_ARRAYSIZE(value_buf), display_format, *v);
    const ImVec2 value_text_size = CalcTextSize(value_buf, value_buf_end, true);
    RenderTextClipped(ImVec2(ImMax(frame_bb.Min.x, slider_bb.GetCenter().x - value_text_size.x*0.5f), frame_bb.Min.y + style.FramePadding.y), value_buf, value_buf_end, &value_text_size, frame_bb.Max);

    if (label_size.x > 0.0f)
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

bool ImGui::VSliderInt(const char* label, const ImVec2& size, int* v, int v_min, int v_max, const char* display_format)
{
    if (!display_format)
        display_format = "%.0f";
    float v_f = (float)*v;
    bool value_changed = ImGui::VSliderFloat(label, size, &v_f, (float)v_min, (float)v_max, display_format, 1.0f);
    *v = (int)v_f;
    return value_changed;
}

// Add multiple sliders on 1 line for compact edition of multiple components
static bool SliderFloatN(const char* label, float v[3], int components, float v_min, float v_max, const char* display_format, float power)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const float w_full = ImGui::CalcItemWidth();
    const float w_item_one  = ImMax(1.0f, (float)(int)((w_full - (style.FramePadding.x*2.0f + style.ItemInnerSpacing.x)*(components-1)) / (float)components));
    const float w_item_last = ImMax(1.0f, (float)(int)(w_full - (w_item_one + style.FramePadding.x*2.0f + style.ItemInnerSpacing.x)*(components-1)));

    bool value_changed = false;
    ImGui::BeginGroup();
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
        ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
        ImGui::PopID();
    }
    ImGui::PopItemWidth();
    ImGui::PopID();

    ImGui::TextUnformatted(label, FindTextDisplayEnd(label));
    ImGui::EndGroup();

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

static bool SliderIntN(const char* label, int v[3], int components, int v_min, int v_max, const char* display_format)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const float w_full = ImGui::CalcItemWidth();
    const float w_item_one  = ImMax(1.0f, (float)(int)((w_full - (style.FramePadding.x*2.0f + style.ItemInnerSpacing.x)*(components-1)) / (float)components));
    const float w_item_last = ImMax(1.0f, (float)(int)(w_full - (w_item_one + style.FramePadding.x*2.0f + style.ItemInnerSpacing.x)*(components-1)));

    bool value_changed = false;
    ImGui::BeginGroup();
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
        value_changed |= ImGui::SliderInt("##v", &v[i], v_min, v_max, display_format);
        ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
        ImGui::PopID();
    }
    ImGui::PopItemWidth();
    ImGui::PopID();

    ImGui::TextUnformatted(label, FindTextDisplayEnd(label));
    ImGui::EndGroup();

    return value_changed;
}

bool ImGui::SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* display_format)
{
    return SliderIntN(label, v, 2, v_min, v_max, display_format);
}

bool ImGui::SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* display_format)
{
    return SliderIntN(label, v, 3, v_min, v_max, display_format);
}

bool ImGui::SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* display_format)
{
    return SliderIntN(label, v, 4, v_min, v_max, display_format);
}

enum ImGuiPlotType
{
    ImGuiPlotType_Lines,
    ImGuiPlotType_Histogram
};

static void Plot(ImGuiPlotType plot_type, const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const ImGuiStyle& style = g.Style;

    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    if (graph_size.x == 0.0f)
        graph_size.x = ImGui::CalcItemWidth() + (style.FramePadding.x * 2);
    if (graph_size.y == 0.0f)
        graph_size.y = label_size.y + (style.FramePadding.y * 2);

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y));
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, NULL))
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

    RenderFrame(frame_bb.Min, frame_bb.Max, window->Color(ImGuiCol_FrameBg), true, style.FrameRounding);

    int res_w = ImMin((int)graph_size.x, values_count);
    if (plot_type == ImGuiPlotType_Lines)
        res_w -= 1;

    // Tooltip on hover
    int v_hovered = -1;
    if (IsMouseHoveringRect(inner_bb))
    {
        const float t = ImClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
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
            window->DrawList->AddLine(ImLerp(inner_bb.Min, inner_bb.Max, p0), ImLerp(inner_bb.Min, inner_bb.Max, p1), v_hovered == v_idx ? col_hovered : col_base);
        else if (plot_type == ImGuiPlotType_Histogram)
            window->DrawList->AddRectFilled(ImLerp(inner_bb.Min, inner_bb.Max, p0), ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(p1.x, 1.0f))+ImVec2(-1,0), v_hovered == v_idx ? col_hovered : col_base);

        t0 = t1;
        p0 = p1;
    }

    // Text overlay
    if (overlay_text)
        RenderText(ImVec2(inner_bb.GetCenter().x - ImGui::CalcTextSize(overlay_text, NULL, true).x*0.5f, frame_bb.Min.y + style.FramePadding.y), overlay_text);

    RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);
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
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const ImRect check_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(label_size.y + style.FramePadding.y*2, label_size.y + style.FramePadding.y*2));
    ItemSize(check_bb, style.FramePadding.y);

    ImRect total_bb = check_bb;
    if (label_size.x > 0)
        SameLine(0, (int)style.ItemInnerSpacing.x);
    const ImRect text_bb(window->DC.CursorPos + ImVec2(0,style.FramePadding.y), window->DC.CursorPos + ImVec2(0,style.FramePadding.y) + label_size);
    if (label_size.x > 0)
    {
        ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
        total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));
    }

    if (!ItemAdd(total_bb, &id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(total_bb, id, &hovered, &held, true);
    if (pressed)
        *v = !(*v);

    RenderFrame(check_bb.Min, check_bb.Max, window->Color((held && hovered) ? ImGuiCol_CheckActive : hovered ? ImGuiCol_CheckHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
    if (*v)
    {
        const float check_sz = ImMin(check_bb.GetWidth(), check_bb.GetHeight());
        const float pad = check_sz < 8.0f ? 1.0f : check_sz < 13.0f ? 2.0f : 3.0f;
        window->DrawList->AddRectFilled(check_bb.Min+ImVec2(pad,pad), check_bb.Max-ImVec2(pad,pad), window->Color(ImGuiCol_CheckMark), style.FrameRounding);
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
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const ImRect check_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(label_size.y + style.FramePadding.y*2-1, label_size.y + style.FramePadding.y*2-1));
    ItemSize(check_bb, style.FramePadding.y);

    ImRect total_bb = check_bb;
    if (label_size.x > 0)
        SameLine(0, (int)style.ItemInnerSpacing.x);
    const ImRect text_bb(window->DC.CursorPos + ImVec2(0, style.FramePadding.y), window->DC.CursorPos + ImVec2(0, style.FramePadding.y) + label_size);
    if (label_size.x > 0)
    {
        ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
        total_bb.Add(text_bb);
    }

    if (!ItemAdd(total_bb, &id))
        return false;

    ImVec2 center = check_bb.GetCenter();
    center.x = (float)(int)center.x + 0.5f;
    center.y = (float)(int)center.y + 0.5f;
    const float radius = check_bb.GetHeight() * 0.5f;

    bool hovered, held;
    bool pressed = ButtonBehavior(total_bb, id, &hovered, &held, true);

    window->DrawList->AddCircleFilled(center, radius, window->Color((held && hovered) ? ImGuiCol_CheckActive : hovered ? ImGuiCol_CheckHovered : ImGuiCol_FrameBg), 16);
    if (active)
    {
        const float check_sz = ImMin(check_bb.GetWidth(), check_bb.GetHeight());
        const float pad = check_sz < 8.0f ? 1.0f : check_sz < 13.0f ? 2.0f : 3.0f;
        window->DrawList->AddCircleFilled(center, radius-pad, window->Color(ImGuiCol_CheckMark), 16);
    }

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

static bool is_separator(unsigned int c)                                                          { return c==',' || c==';' || c=='(' || c==')' || c=='{' || c=='}' || c=='[' || c==']' || c=='|'; }
#define STB_TEXTEDIT_IS_SPACE(CH)                                                                 ( ImCharIsSpace((unsigned int)CH) || is_separator((unsigned int)CH) )
static void STB_TEXTEDIT_DELETECHARS(STB_TEXTEDIT_STRING* obj, int pos, int n)
{
    ImWchar* dst = obj->Text + pos;

    // We maintain our buffer length in both UTF-8 and wchar formats
    obj->CurLenA -= ImTextCountUtf8BytesFromStr(dst, dst + n);
    obj->CurLenW -= n;

    // Offset remaining text
    const ImWchar* src = obj->Text + pos + n; 
    while (ImWchar c = *src++)
        *dst++ = c; 
    *dst = '\0';
}

static bool STB_TEXTEDIT_INSERTCHARS(STB_TEXTEDIT_STRING* obj, int pos, const ImWchar* new_text, int new_text_len)
{
    const size_t text_len = obj->CurLenW;
    if ((size_t)new_text_len + text_len + 1 > IM_ARRAYSIZE(obj->Text))
        return false;

    const int new_text_len_utf8 = ImTextCountUtf8BytesFromStr(new_text, new_text + new_text_len);
    if ((size_t)new_text_len_utf8 + obj->CurLenA + 1 > obj->BufSizeA)
        return false;

    if (pos != (int)text_len)
        memmove(obj->Text + (size_t)pos + new_text_len, obj->Text + (size_t)pos, (text_len - (size_t)pos) * sizeof(ImWchar));
    memcpy(obj->Text + (size_t)pos, new_text, (size_t)new_text_len * sizeof(ImWchar));

    obj->CurLenW += new_text_len;
    obj->CurLenA += new_text_len_utf8;
    obj->Text[obj->CurLenW] = '\0';

    return true;
}

// We don't use an enum so we can build even with conflicting symbols (if another user of stb_textedit.h leak their STB_TEXTEDIT_K_* symbols)
#define STB_TEXTEDIT_K_LEFT         0x10000 // keyboard input to move cursor left
#define STB_TEXTEDIT_K_RIGHT        0x10001 // keyboard input to move cursor right
#define STB_TEXTEDIT_K_UP           0x10002 // keyboard input to move cursor up
#define STB_TEXTEDIT_K_DOWN         0x10003 // keyboard input to move cursor down
#define STB_TEXTEDIT_K_LINESTART    0x10004 // keyboard input to move cursor to start of line
#define STB_TEXTEDIT_K_LINEEND      0x10005 // keyboard input to move cursor to end of line
#define STB_TEXTEDIT_K_TEXTSTART    0x10006 // keyboard input to move cursor to start of text
#define STB_TEXTEDIT_K_TEXTEND      0x10007 // keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_DELETE       0x10008 // keyboard input to delete selection or character under cursor
#define STB_TEXTEDIT_K_BACKSPACE    0x10009 // keyboard input to delete selection or character left of cursor
#define STB_TEXTEDIT_K_UNDO         0x1000A // keyboard input to perform undo
#define STB_TEXTEDIT_K_REDO         0x1000B // keyboard input to perform redo
#define STB_TEXTEDIT_K_WORDLEFT     0x1000C // keyboard input to move cursor left one word
#define STB_TEXTEDIT_K_WORDRIGHT    0x1000D // keyboard input to move cursor right one word
#define STB_TEXTEDIT_K_SHIFT        0x20000

#ifdef IMGUI_STB_NAMESPACE
namespace IMGUI_STB_NAMESPACE
{
#endif
#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb_textedit.h"
#ifdef IMGUI_STB_NAMESPACE
}
#endif

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

    // If widget became bigger than text (because of a resize), reset horizontal scrolling
    if (ScrollX > 0.0f)
    {
        const float text_width = cursor_offset_x + Font->CalcTextSizeW(FontSize, FLT_MAX, Text+StbState.cursor, NULL, NULL).x;
        if (text_width < Width)
        {
            ScrollX = 0.0f;
            return;
        }
    }

    if (cursor_offset_x < ScrollX)
        ScrollX = ImMax(0.0f, cursor_offset_x - scroll_x_increment);    
    else if (cursor_offset_x - Width >= ScrollX)
        ScrollX = cursor_offset_x - Width + scroll_x_increment;
}

ImVec2 ImGuiTextEditState::CalcDisplayOffsetFromCharIdx(int i) const
{
    const ImWchar* text_start = GetTextPointerClippedW(Font, FontSize, Text, ScrollX, NULL);
    const ImWchar* text_end = (Text+i >= text_start) ? Text+i : text_start;                    // Clip if requested character is outside of display
    IM_ASSERT(text_end >= text_start);

    const ImVec2 offset = Font->CalcTextSizeW(FontSize, Width+1, text_start, text_end, NULL);
    return offset;
}

// [Static]
const char* ImGuiTextEditState::GetTextPointerClippedA(ImFont* font, float font_size, const char* text, float width, ImVec2* out_text_size)
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
const ImWchar* ImGuiTextEditState::GetTextPointerClippedW(ImFont* font, float font_size, const ImWchar* text, float width, ImVec2* out_text_size)
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
void ImGuiTextEditState::RenderTextScrolledClipped(ImFont* font, float font_size, const char* buf, ImVec2 pos, float width, float scroll_x)
{
    ImGuiWindow* window = GetCurrentWindow();
    const ImU32 font_color = window->Color(ImGuiCol_Text);
    //window->DrawList->AddLine(pos, pos+ImVec2(width,0), 0xFF00FFFF);

    // Determine start and end of visible string
    // FIXME-OPT: This is pretty slow for what it does.
    const char* text_start = scroll_x <= 0.0f ? buf : GetTextPointerClippedA(font, font_size, buf, scroll_x, NULL);
    const char* text_end = GetTextPointerClippedA(font, font_size, text_start, width + 1, NULL); // +1 to allow character spacing to fit outside the allowed width
    window->DrawList->AddText(font, font_size, pos, font_color, text_start, text_end);

    // Log as text
    if (GImGui->LogEnabled)
        LogText(pos, buf, NULL);
}

bool ImGui::InputFloat(const char* label, float *v, float step, float step_fast, int decimal_precision, ImGuiInputTextFlags extra_flags)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const float w = ImGui::CalcItemWidth();
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y) + style.FramePadding*2.0f);

    ImGui::BeginGroup();
    ImGui::PushID(label);
    const ImVec2 button_sz = ImVec2(g.FontSize, g.FontSize) + style.FramePadding * 2;
    if (step > 0.0f)
        ImGui::PushItemWidth(ImMax(1.0f, w - (button_sz.x + style.ItemInnerSpacing.x)*2));

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
        ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
        if (ImGui::Button("-", button_sz, true))
        {
            *v -= g.IO.KeyCtrl && step_fast > 0.0f ? step_fast : step;
            value_changed = true;
        }
        ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
        if (ImGui::Button("+", button_sz, true))
        {
            *v += g.IO.KeyCtrl && step_fast > 0.0f ? step_fast : step;
            value_changed = true;
        }
    }

    ImGui::PopID();

    if (label_size.x > 0)
    {
        ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
        RenderText(ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y + style.FramePadding.y), label);
        ItemSize(label_size, style.FramePadding.y);
    }

    ImGui::EndGroup();

    return value_changed;
}

bool ImGui::InputInt(const char* label, int *v, int step, int step_fast, ImGuiInputTextFlags extra_flags)
{
    float f = (float)*v;
    const bool value_changed = ImGui::InputFloat(label, &f, (float)step, (float)step_fast, 0, extra_flags);
    if (value_changed)
        *v = (int)f;
    return value_changed;
}

// Public API to manipulate UTF-8 text
// We expose UTF-8 to the user (unlike the STB_TEXTEDIT_* functions which are manipulating wchar)
void ImGuiTextEditCallbackData::DeleteChars(int pos, int bytes_count)
{
    char* dst = Buf + pos;
    const char* src = Buf + pos + bytes_count;
    while (char c = *src++)
        *dst++ = c;
    *dst = '\0';

    BufDirty = true;
    if (CursorPos + bytes_count >= pos)
        CursorPos -= bytes_count;
    else if (CursorPos >= pos)
        CursorPos = pos;
    SelectionStart = SelectionEnd = CursorPos;
}

void ImGuiTextEditCallbackData::InsertChars(int pos, const char* new_text, const char* new_text_end)
{
    const size_t text_len = strlen(Buf);
    if (!new_text_end)
        new_text_end = new_text + strlen(new_text);
    const size_t new_text_len = (size_t)(new_text_end - new_text);

    if (new_text_len + text_len + 1 >= BufSize)
        return;

    size_t upos = (size_t)pos;
    if (text_len != upos)
        memmove(Buf + upos + new_text_len, Buf + upos, text_len - upos);
    memcpy(Buf + upos, new_text, new_text_len * sizeof(char));
    Buf[text_len + new_text_len] = '\0';

    BufDirty = true;
    if (CursorPos >= pos)
        CursorPos += (int)new_text_len;
    SelectionStart = SelectionEnd = CursorPos;
}

// Return false to discard a character.
static bool InputTextFilterCharacter(unsigned int* p_char, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void* user_data)
{
    unsigned int c = *p_char;

    if (c < 128 && c != ' ' && !isprint((int)(c & 0xFF)))
        return false;

    if (c >= 0xE000 && c <= 0xF8FF) // Filter private Unicode range. I don't imagine anybody would want to input them. GLFW on OSX seems to send private characters for special keys like arrow keys.
        return false;

    if (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank))
    {
        if (flags & ImGuiInputTextFlags_CharsDecimal)
            if (!(c >= '0' && c <= '9') && (c != '.') && (c != '-') && (c != '+') && (c != '*') && (c != '/'))
                return false;

        if (flags & ImGuiInputTextFlags_CharsHexadecimal)
            if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'))
                return false;

        if (flags & ImGuiInputTextFlags_CharsUppercase)
            if (c >= 'a' && c <= 'z')
                *p_char = (c += (unsigned int)('A'-'a'));

        if (flags & ImGuiInputTextFlags_CharsNoBlank)
            if (ImCharIsSpace(c))
                return false;
    }

    if (flags & ImGuiInputTextFlags_CallbackCharFilter)
    {
        ImGuiTextEditCallbackData callback_data;
        memset(&callback_data, 0, sizeof(ImGuiTextEditCallbackData));
        callback_data.EventFlag = ImGuiInputTextFlags_CallbackCharFilter; 
        callback_data.EventChar = (ImWchar)c;
        callback_data.Flags = flags;
        callback_data.UserData = user_data;
        if (callback(&callback_data) != 0)
            return false;
        *p_char = callback_data.EventChar;
        if (!callback_data.EventChar)
            return false;
    }

    return true;
}

// Edit a string of text
bool ImGui::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void* user_data)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;

    const ImGuiID id = window->GetID(label);
    const float w = ImGui::CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y) + style.FramePadding*2.0f);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? (style.ItemInnerSpacing.x + label_size.x) : 0.0f, 0.0f));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, &id))
        return false;

    // NB: we are only allowed to access 'edit_state' if we are the active widget.
    ImGuiTextEditState& edit_state = g.InputTextState;

    const bool is_ctrl_down = io.KeyCtrl;
    const bool is_shift_down = io.KeyShift;
    const bool focus_requested = window->FocusItemRegister(g.ActiveId == id, (flags & ImGuiInputTextFlags_CallbackCompletion) == 0);    // Using completion callback disable keyboard tabbing
    const bool focus_requested_by_code = focus_requested && (window->FocusIdxAllCounter == window->FocusIdxAllRequestCurrent);
    const bool focus_requested_by_tab = focus_requested && !focus_requested_by_code;

    const bool hovered = IsHovered(frame_bb, id);
    if (hovered)
    {
        g.HoveredId = id;
        g.MouseCursor = ImGuiMouseCursor_TextInput;
    }
    const bool user_clicked = hovered && io.MouseClicked[0];

    bool select_all = (g.ActiveId != id) && (flags & ImGuiInputTextFlags_AutoSelectAll) != 0;
    if (focus_requested || user_clicked)
    {
        if (g.ActiveId != id)
        {
            // Start edition
            // Take a copy of the initial buffer value (both in original UTF-8 format and converted to wchar)
            // From the moment we focused we are ignoring the content of 'buf'
            ImFormatString(edit_state.InitialText, IM_ARRAYSIZE(edit_state.InitialText), "%s", buf);
            const char* buf_end = NULL;
            edit_state.CurLenW = ImTextStrFromUtf8(edit_state.Text, IM_ARRAYSIZE(edit_state.Text), buf, NULL, &buf_end);
            edit_state.CurLenA = buf_end - buf; // We can't get the result from ImFormatString() above because it is not UTF-8 aware. Here we'll cut off malformed UTF-8.
            edit_state.Width = w + style.FramePadding.x;
            edit_state.InputCursorScreenPos = ImVec2(-1.f,-1.f);
            edit_state.CursorAnimReset();

            if (edit_state.Id != id)
            {
                edit_state.Id = id;
                edit_state.ScrollX = 0.0f;
                stb_textedit_initialize_state(&edit_state.StbState, true); 
                if (focus_requested_by_code)
                    select_all = true;
            }
            else
            {
                // Recycle existing cursor/selection/undo stack but clamp position
                // Note a single mouse click will override the cursor/position immediately by calling stb_textedit_click handler.
                edit_state.StbState.cursor = ImMin(edit_state.StbState.cursor, (int)edit_state.CurLenW);
                edit_state.StbState.select_start = ImMin(edit_state.StbState.select_start, (int)edit_state.CurLenW);
                edit_state.StbState.select_end = ImMin(edit_state.StbState.select_end, (int)edit_state.CurLenW);
            }
            if (focus_requested_by_tab || (user_clicked && is_ctrl_down))
                select_all = true;
        }
        SetActiveId(id);
        FocusWindow(window);
    }
    else if (io.MouseClicked[0])
    {
        // Release focus when we click outside
        if (g.ActiveId == id)
        {
            SetActiveId(0);
        }
    }

    // Although we are active we don't prevent mouse from hovering other elements unless we are interacting right now with the widget.
    // Down the line we should have a cleaner concept of focused vs active in the library.
    if (g.ActiveId == id)
        g.ActiveIdIsFocusedOnly = !io.MouseDown[0];

    bool value_changed = false;
    bool cancel_edit = false;
    bool enter_pressed = false;
    if (g.ActiveId == id)
    //if (edit_state.Id == id)  // Works, but double-click to select-all sets cursors to end which in turn tends to scroll toward the right when shrinking widget.
    {
        // Update some data if we are active or last active
        edit_state.Width = w + style.FramePadding.x;
        edit_state.BufSizeA = buf_size;
        edit_state.Font = g.Font;
        edit_state.FontSize = g.FontSize;
        edit_state.UpdateScrollOffset();
    }
    if (g.ActiveId == id)
    {
        // Edit in progress
        const float mx = g.IO.MousePos.x - frame_bb.Min.x - style.FramePadding.x;
        const float my = g.FontSize*0.5f;   // Flatten mouse because we are doing a single-line edit

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

        if (g.IO.InputCharacters[0])
        {
            // Process text input (before we check for Return because using some IME will effectively send a Return?)
            for (int n = 0; n < IM_ARRAYSIZE(g.IO.InputCharacters) && g.IO.InputCharacters[n]; n++)
            {
                unsigned int c = (unsigned int)g.IO.InputCharacters[n];
                if (c)
                {
                    // Insert character if they pass filtering
                    if (!InputTextFilterCharacter(&c, flags, callback, user_data))
                        continue;
                    edit_state.OnKeyPressed((int)c);
                }
            }

            // Consume characters
            memset(g.IO.InputCharacters, 0, sizeof(g.IO.InputCharacters));
        }

        const int k_mask = (is_shift_down ? STB_TEXTEDIT_K_SHIFT : 0);
        if (IsKeyPressedMap(ImGuiKey_LeftArrow))                { edit_state.OnKeyPressed(is_ctrl_down ? STB_TEXTEDIT_K_WORDLEFT | k_mask : STB_TEXTEDIT_K_LEFT | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_RightArrow))          { edit_state.OnKeyPressed(is_ctrl_down ? STB_TEXTEDIT_K_WORDRIGHT | k_mask  : STB_TEXTEDIT_K_RIGHT | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_Home))                { edit_state.OnKeyPressed(is_ctrl_down ? STB_TEXTEDIT_K_TEXTSTART | k_mask : STB_TEXTEDIT_K_LINESTART | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_End))                 { edit_state.OnKeyPressed(is_ctrl_down ? STB_TEXTEDIT_K_TEXTEND | k_mask : STB_TEXTEDIT_K_LINEEND | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_Delete))              { edit_state.OnKeyPressed(STB_TEXTEDIT_K_DELETE | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_Backspace))           { edit_state.OnKeyPressed(STB_TEXTEDIT_K_BACKSPACE | k_mask); }
        else if (IsKeyPressedMap(ImGuiKey_Enter))               { SetActiveId(0); enter_pressed = true; }
        else if (IsKeyPressedMap(ImGuiKey_Escape))              { SetActiveId(0); cancel_edit = true; }
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
                const int ie = edit_state.HasSelection() ? ImMax(edit_state.StbState.select_start, edit_state.StbState.select_end) : (int)edit_state.CurLenW;
                ImTextStrToUtf8(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), edit_state.Text+ib, edit_state.Text+ie);
                g.IO.SetClipboardTextFn(g.TempBuffer);
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
                    const size_t clipboard_len = strlen(clipboard);
                    ImWchar* clipboard_filtered = (ImWchar*)ImGui::MemAlloc((clipboard_len+1) * sizeof(ImWchar));
                    int clipboard_filtered_len = 0;
                    for (const char* s = clipboard; *s; )
                    {
                        unsigned int c;
                        s += ImTextCharFromUtf8(&c, s, NULL);
                        if (c == 0)
                            break;
                        if (c >= 0x10000)
                            continue;
                        if (!InputTextFilterCharacter(&c, flags, callback, user_data))
                            continue;
                        clipboard_filtered[clipboard_filtered_len++] = (ImWchar)c;
                    }
                    clipboard_filtered[clipboard_filtered_len] = 0;
                    if (clipboard_filtered_len > 0) // If everything was filtered, ignore the pasting operation
                        stb_textedit_paste(&edit_state, &edit_state.StbState, clipboard_filtered, clipboard_filtered_len);
                    ImGui::MemFree(clipboard_filtered);
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
            ImTextStrToUtf8(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), edit_state.Text, NULL);

            // User callback
            if ((flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways)) != 0)
            {
                IM_ASSERT(callback != NULL);

                // The reason we specify the usage semantic (Completion/History) is that Completion needs to disable keyboard TABBING at the moment.
                ImGuiInputTextFlags event_flag = 0;
                ImGuiKey event_key = ImGuiKey_COUNT;
                if ((flags & ImGuiInputTextFlags_CallbackCompletion) != 0 && IsKeyPressedMap(ImGuiKey_Tab))
                {
                    event_flag = ImGuiInputTextFlags_CallbackCompletion;
                    event_key = ImGuiKey_Tab;
                }
                else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_UpArrow))
                {
                    event_flag = ImGuiInputTextFlags_CallbackHistory;
                    event_key = ImGuiKey_UpArrow;
                }
                else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_DownArrow))
                {
                    event_flag = ImGuiInputTextFlags_CallbackHistory;
                    event_key = ImGuiKey_DownArrow;
                }

                if (event_key != ImGuiKey_COUNT || (flags & ImGuiInputTextFlags_CallbackAlways) != 0)
                {
                    ImGuiTextEditCallbackData callback_data;
                    callback_data.EventFlag = event_flag; 
                    callback_data.EventKey = event_key;
                    callback_data.Buf = g.TempBuffer;
                    callback_data.BufSize = edit_state.BufSizeA;
                    callback_data.BufDirty = false;
                    callback_data.Flags = flags;
                    callback_data.UserData = user_data;

                    // We have to convert from position from wchar to UTF-8 positions
                    const int utf8_cursor_pos = callback_data.CursorPos = ImTextCountUtf8BytesFromStr(edit_state.Text, edit_state.Text + edit_state.StbState.cursor);
                    const int utf8_selection_start = callback_data.SelectionStart = ImTextCountUtf8BytesFromStr(edit_state.Text, edit_state.Text + edit_state.StbState.select_start);
                    const int utf8_selection_end = callback_data.SelectionEnd = ImTextCountUtf8BytesFromStr(edit_state.Text, edit_state.Text + edit_state.StbState.select_end);

                    // Call user code
                    callback(&callback_data);

                    // Read back what user may have modified
                    IM_ASSERT(callback_data.Buf == g.TempBuffer);              // Invalid to modify those fields
                    IM_ASSERT(callback_data.BufSize == edit_state.BufSizeA);
                    IM_ASSERT(callback_data.Flags == flags);
                    if (callback_data.CursorPos != utf8_cursor_pos)            edit_state.StbState.cursor = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.CursorPos);
                    if (callback_data.SelectionStart != utf8_selection_start)  edit_state.StbState.select_start = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionStart);
                    if (callback_data.SelectionEnd != utf8_selection_end)      edit_state.StbState.select_end = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionEnd);
                    if (callback_data.BufDirty)
                    {
                        ImTextStrFromUtf8(edit_state.Text, IM_ARRAYSIZE(edit_state.Text), g.TempBuffer, NULL);
                        edit_state.CursorAnimReset();
                    }
                }
            }

            if (strcmp(g.TempBuffer, buf) != 0)
            {
                ImFormatString(buf, buf_size, "%s", g.TempBuffer);
                value_changed = true;
            }
        }
    }
    
    RenderFrame(frame_bb.Min, frame_bb.Max, window->Color(ImGuiCol_FrameBg), true, style.FrameRounding);

    const ImVec2 font_off_up = ImVec2(0.0f, g.FontSize+1.0f);    // FIXME: those offsets are part of the style or font API
    const ImVec2 font_off_dn = ImVec2(0.0f, 2.0f);

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

    //const float render_scroll_x = (g.ActiveId == id) ? edit_state.ScrollX : 0.0f;
    const float render_scroll_x = (edit_state.Id == id) ? edit_state.ScrollX : 0.0f;
    ImGuiTextEditState::RenderTextScrolledClipped(g.Font, g.FontSize, buf, frame_bb.Min + style.FramePadding, w + style.FramePadding.x, render_scroll_x);

    if (g.ActiveId == id)
    {
        const ImVec2 cursor_pos = frame_bb.Min + style.FramePadding + edit_state.CalcDisplayOffsetFromCharIdx(edit_state.StbState.cursor);

        // Draw blinking cursor
        if (g.InputTextState.CursorIsVisible())
            window->DrawList->AddRect(cursor_pos - font_off_up + ImVec2(0,2), cursor_pos + font_off_dn - ImVec2(0,3), window->Color(ImGuiCol_Text));
        
        // Notify OS of text input position for advanced IME
        if (io.ImeSetInputScreenPosFn && ImLengthSqr(edit_state.InputCursorScreenPos - cursor_pos) > 0.0001f)
            io.ImeSetInputScreenPosFn((int)cursor_pos.x - 1, (int)(cursor_pos.y - g.FontSize));   // -1 x offset so that Windows IME can cover our cursor. Bit of an extra nicety.

        edit_state.InputCursorScreenPos = cursor_pos;
    }

    if (label_size.x > 0)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    if ((flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0)
        return enter_pressed;
    else
        return value_changed;
}

static bool InputFloatN(const char* label, float* v, int components, int decimal_precision)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const float w_full = ImGui::CalcItemWidth();
    const float w_item_one  = ImMax(1.0f, (float)(int)((w_full - (style.FramePadding.x*2.0f + style.ItemInnerSpacing.x) * (components-1)) / (float)components));
    const float w_item_last = ImMax(1.0f, (float)(int)(w_full - (w_item_one + style.FramePadding.x*2.0f + style.ItemInnerSpacing.x) * (components-1)));

    bool value_changed = false;
    ImGui::BeginGroup();
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
        ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
        ImGui::PopID();
    }
    ImGui::PopItemWidth();
    ImGui::PopID();

    window->DC.CurrentLineTextBaseOffset = ImMax(window->DC.CurrentLineTextBaseOffset, style.FramePadding.y);
    ImGui::TextUnformatted(label, FindTextDisplayEnd(label));
    ImGui::EndGroup();

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

static bool InputIntN(const char* label, int* v, int components)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const float w_full = ImGui::CalcItemWidth();
    const float w_item_one  = ImMax(1.0f, (float)(int)((w_full - (style.FramePadding.x*2.0f + style.ItemInnerSpacing.x) * (components-1)) / (float)components));
    const float w_item_last = ImMax(1.0f, (float)(int)(w_full - (w_item_one + style.FramePadding.x*2.0f + style.ItemInnerSpacing.x) * (components-1)));

    bool value_changed = false;
    ImGui::BeginGroup();
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
        value_changed |= ImGui::InputInt("##v", &v[i], 0, 0);
        ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
        ImGui::PopID();
    }
    ImGui::PopItemWidth();
    ImGui::PopID();

    ImGui::TextUnformatted(label, FindTextDisplayEnd(label));
    ImGui::EndGroup();

    return value_changed;
}

bool ImGui::InputInt2(const char* label, int v[2])
{
    return InputIntN(label, v, 2);
}

bool ImGui::InputInt3(const char* label, int v[3])
{
    return InputIntN(label, v, 3);
}

bool ImGui::InputInt4(const char* label, int v[4])
{
    return InputIntN(label, v, 4);
}

static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
    const char** items = (const char**)data;
    if (out_text)
        *out_text = items[idx];
    return true;
}

static bool Items_SingleStringGetter(void* data, int idx, const char** out_text)
{
    // FIXME-OPT: we could pre-compute the indices to fasten this. But only 1 active combo means the waste is limited.
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

// Combo box helper allowing to pass an array of strings.
bool ImGui::Combo(const char* label, int* current_item, const char** items, int items_count, int height_in_items)
{
    const bool value_changed = Combo(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
    return value_changed;
}

// Combo box helper allowing to pass all items in a single string.
bool ImGui::Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
{
    int items_count = 0;
    const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this
    while (*p)
    {
        p += strlen(p) + 1;
        items_count++;
    }
    bool value_changed = Combo(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
    return value_changed;
}

// Combo box function.
bool ImGui::Combo(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int height_in_items)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = ImGui::CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y) + style.FramePadding*2.0f);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(style.ItemInnerSpacing.x + label_size.x,0));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, &id))
        return false;

    const float arrow_size = (g.FontSize + style.FramePadding.x * 2.0f);
    const bool hovered = IsHovered(frame_bb, id);

    bool value_changed = false;
    const ImRect value_bb(frame_bb.Min, frame_bb.Max - ImVec2(arrow_size, 0.0f));
    RenderFrame(frame_bb.Min, frame_bb.Max, window->Color(ImGuiCol_FrameBg), true, style.FrameRounding);
    RenderFrame(ImVec2(frame_bb.Max.x-arrow_size, frame_bb.Min.y), frame_bb.Max, window->Color(hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button), true, style.FrameRounding);	// FIXME-ROUNDING
    RenderCollapseTriangle(ImVec2(frame_bb.Max.x-arrow_size, frame_bb.Min.y) + style.FramePadding, true);

    if (*current_item >= 0 && *current_item < items_count)
    {
        const char* item_text;
        if (items_getter(data, *current_item, &item_text))
            RenderTextClipped(frame_bb.Min + style.FramePadding, item_text, NULL, NULL, value_bb.Max);
    }

    if (label_size.x > 0)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    ImGui::PushID((int)id);
    bool menu_toggled = false;
    if (hovered)
    {
        g.HoveredId = id;
        if (g.IO.MouseClicked[0])
        {
            menu_toggled = true;
            g.ActiveComboID = (g.ActiveComboID == id) ? 0 : id;
            if (g.ActiveComboID)
                FocusWindow(window);
        }
    }
    
    if (g.ActiveComboID == id)
    {
        // Size default to hold ~7 items
        if (height_in_items < 0)
            height_in_items = 7;

        const ImVec2 backup_pos = ImGui::GetCursorPos();
        const float popup_off_x = 0.0f;//style.ItemInnerSpacing.x;
        const float popup_height = (label_size.y + style.ItemSpacing.y) * ImMin(items_count, height_in_items) + style.WindowPadding.y;
        const ImRect popup_rect(ImVec2(frame_bb.Min.x+popup_off_x, frame_bb.Max.y), ImVec2(frame_bb.Max.x+popup_off_x, frame_bb.Max.y + popup_height));
        ImGui::SetCursorPos(popup_rect.Min - window->Pos);

        const ImGuiWindowFlags flags = ImGuiWindowFlags_ComboBox | ((window->Flags & ImGuiWindowFlags_ShowBorders) ? ImGuiWindowFlags_ShowBorders : 0);
        ImGui::BeginChild("#ComboBox", popup_rect.GetSize(), false, flags);
        ImGui::Spacing();

        bool combo_item_active = false;
        combo_item_active |= (g.ActiveId == GetCurrentWindow()->GetID("#SCROLLY"));

        // Display items
        for (int i = 0; i < items_count; i++)
        {
            ImGui::PushID((void*)(intptr_t)i);
            const bool item_selected = (i == *current_item);
            const char* item_text;
            if (!items_getter(data, i, &item_text))
                item_text = "*Unknown item*";
            if (ImGui::Selectable(item_text, item_selected))
            {
                SetActiveId(0);
                g.ActiveComboID = 0;
                value_changed = true;
                *current_item = i;
            }
            if (item_selected && menu_toggled)
                ImGui::SetScrollPosHere();
            combo_item_active |= ImGui::IsItemActive();
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::SetCursorPos(backup_pos);
        
        if (!combo_item_active && g.ActiveId != 0)
            g.ActiveComboID = 0;
    }

    ImGui::PopID();

    return value_changed;
}

// Tip: pass an empty label (e.g. "##dummy") then you can use the space to draw other text or image.
// But you need to make sure the ID is unique, e.g. enclose calls in PushID/PopID.
bool ImGui::Selectable(const char* label, bool selected, const ImVec2& size_arg)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    
    const float w = ImMax(label_size.x, window->Pos.x + ImGui::GetContentRegionMax().x - style.AutoFitPadding.x - window->DC.CursorPos.x);
    const ImVec2 size(size_arg.x != 0.0f ? size_arg.x : w, size_arg.y != 0.0f ? size_arg.y : label_size.y);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(bb);

    // Selectables are meant to be tightly packed together. So for both rendering and collision we extend to compensate for spacing.
    ImRect bb_with_spacing = bb;
    const float spacing_L = (float)(int)(style.ItemSpacing.x * 0.5f);
    const float spacing_U = (float)(int)(style.ItemSpacing.y * 0.5f);
    const float spacing_R = style.ItemSpacing.x - spacing_L;
    const float spacing_D = style.ItemSpacing.y - spacing_U;
    bb_with_spacing.Min.x -= spacing_L;
    bb_with_spacing.Min.y -= spacing_U;
    bb_with_spacing.Max.x += spacing_R;
    bb_with_spacing.Max.y += spacing_D;
    if (!ItemAdd(bb_with_spacing, &id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb_with_spacing, id, &hovered, &held, true, false, false);

    // Render
    if (hovered || selected)
    {
        const ImU32 col = window->Color((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        RenderFrame(bb_with_spacing.Min, bb_with_spacing.Max, col, false, style.FrameRounding);
    }

    //const ImVec2 off = ImVec2(ImMax(0.0f, size.x - text_size.x) * 0.5f, ImMax(0.0f, size.y - text_size.y) * 0.5f);
    RenderTextClipped(bb.Min, label, NULL, &label_size, bb_with_spacing.Max);

    return pressed;
}

bool ImGui::Selectable(const char* label, bool* p_selected, const ImVec2& size_arg)
{
    if (ImGui::Selectable(label, *p_selected, size_arg))
    {
        *p_selected = !*p_selected;
        return true;
    }
    return false;
}

// Helper to calculate the size of a listbox and display a label on the right.
// Tip: To have a list filling the entire window width, PushItemWidth(-1) and pass an empty label "##empty"
bool ImGui::ListBoxHeader(const char* label, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();

    const ImGuiStyle& style = ImGui::GetStyle();
    const ImGuiID id = ImGui::GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    // Size default to hold ~7 items. Fractional number of items helps seeing that we can scroll down/up without looking at scrollbar.
    ImVec2 size;
    size.x = (size_arg.x != 0.0f) ? (size_arg.x) : ImGui::CalcItemWidth() + style.FramePadding.x * 2.0f;
    size.y = (size_arg.y != 0.0f) ? (size_arg.y) : ImGui::GetTextLineHeightWithSpacing() * 7.4f + style.ItemSpacing.y;
    const ImVec2 frame_size = ImVec2(size.x, ImMax(size.y, label_size.y));
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    const ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    window->DC.LastItemRect = bb;

    ImGui::BeginGroup();
    if (label_size.x > 0)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    ImGui::BeginChildFrame(id, frame_bb.GetSize());
    return true;
}

bool ImGui::ListBoxHeader(const char* label, int items_count, int height_in_items)
{
    // Size default to hold ~7 items. Fractional number of items helps seeing that we can scroll down/up without looking at scrollbar.
    // However we don't add +0.40f if items_count <= height_in_items. It is slightly dodgy, because it means a dynamic list of items will make the widget resize occasionally when it crosses that size.
    // I am expecting that someone will come and complain about this behavior in a remote future, then we can advise on a better solution.
    if (height_in_items < 0)
        height_in_items = ImMin(items_count, 7);
    float height_in_items_f = height_in_items < items_count ? (height_in_items + 0.40f) : (height_in_items + 0.00f);

    // We include ItemSpacing.y so that a list sized for the exact number of items doesn't make a scrollbar appears. We could also enforce that by passing a flag to BeginChild().
    ImVec2 size;
    size.x = 0.0f;
    size.y = ImGui::GetTextLineHeightWithSpacing() * height_in_items_f + ImGui::GetStyle().ItemSpacing.y;
    return ImGui::ListBoxHeader(label, size);
}

void ImGui::ListBoxFooter()
{
    ImGuiWindow* parent_window = GetParentWindow();
    const ImRect bb = parent_window->DC.LastItemRect;
    const ImGuiStyle& style = ImGui::GetStyle();
    
    ImGui::EndChildFrame();

    // Redeclare item size so that it includes the label (we have stored the full size in LastItemRect)
    // We call SameLine() to restore DC.CurrentLine* data
    ImGui::SameLine();
    parent_window->DC.CursorPos = bb.Min;
    ItemSize(bb, style.FramePadding.y);
    ImGui::EndGroup();
}

bool ImGui::ListBox(const char* label, int* current_item, const char** items, int items_count, int height_items)
{
    const bool value_changed = ListBox(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_items);
    return value_changed;
}

bool ImGui::ListBox(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int height_in_items)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    if (!ImGui::ListBoxHeader(label, items_count, height_in_items))
        return false;

    bool value_changed = false;
    for (int i = 0; i < items_count; i++)
    {
        const bool item_selected = (i == *current_item);
        const char* item_text;
        if (!items_getter(data, i, &item_text))
            item_text = "*Unknown item*";

        ImGui::PushID(i);
        if (ImGui::Selectable(item_text, item_selected))
        {
            *current_item = i;
            value_changed = true;
        }
        ImGui::PopID();
    }

    ImGui::ListBoxFooter();
    return value_changed;
}

// A little colored square. Return true when clicked.
bool ImGui::ColorButton(const ImVec4& col, bool small_height, bool outline_border)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID("#colorbutton");
    const float square_size = g.FontSize;
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(square_size + style.FramePadding.x*2, square_size + (small_height ? 0 : style.FramePadding.y*2)));
    ItemSize(bb, small_height ? 0.0f : style.FramePadding.y);
    if (!ItemAdd(bb, &id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, true);
    RenderFrame(bb.Min, bb.Max, window->Color(col), outline_border, style.FrameRounding);

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
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w_full = ImGui::CalcItemWidth();
    const float square_sz = (g.FontSize + style.FramePadding.x * 2.0f);

    ImGuiColorEditMode edit_mode = window->DC.ColorEditMode;
    if (edit_mode == ImGuiColorEditMode_UserSelect || edit_mode == ImGuiColorEditMode_UserSelectShowButton)
        edit_mode = g.ColorEditModeStorage.GetInt(id, 0) % 3;

    float fx = col[0];
    float fy = col[1];
    float fz = col[2];
    float fw = col[3];
    const ImVec4 col_display(fx, fy, fz, 1.0f);

    if (edit_mode == ImGuiColorEditMode_HSV)
        ImGui::ColorConvertRGBtoHSV(fx, fy, fz, fx, fy, fz);

    int ix = (int)(fx * 255.0f + 0.5f);
    int iy = (int)(fy * 255.0f + 0.5f);
    int iz = (int)(fz * 255.0f + 0.5f);
    int iw = (int)(fw * 255.0f + 0.5f);

    int components = alpha ? 4 : 3;
    bool value_changed = false;

    ImGui::BeginGroup();
    ImGui::PushID(label);

    bool hsv = (edit_mode == 1);
    switch (edit_mode)
    {
    case ImGuiColorEditMode_RGB:
    case ImGuiColorEditMode_HSV:
        {
            // 0: RGB 0..255 Sliders
            // 1: HSV 0..255 Sliders
            const float w_items_all = w_full - (square_sz + style.ItemInnerSpacing.x);
            const float w_item_one  = ImMax(1.0f, (float)(int)((w_items_all - (style.FramePadding.x*2.0f + style.ItemInnerSpacing.x) * (components-1)) / (float)components));
            const float w_item_last = ImMax(1.0f, (float)(int)(w_items_all - (w_item_one + style.FramePadding.x*2.0f + style.ItemInnerSpacing.x) * (components-1)));

            ImGui::PushItemWidth(w_item_one);
            value_changed |= ImGui::SliderInt("##X", &ix, 0, 255, hsv ? "H:%3.0f" : "R:%3.0f");
            ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
            value_changed |= ImGui::SliderInt("##Y", &iy, 0, 255, hsv ? "S:%3.0f" : "G:%3.0f");
            ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
            if (alpha)
            {
                value_changed |= ImGui::SliderInt("##Z", &iz, 0, 255, hsv ? "V:%3.0f" : "B:%3.0f");
                ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
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
                ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ix, iy, iz, iw);
            else
                ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ix, iy, iz);
            ImGui::PushItemWidth(w_slider_all - style.ItemInnerSpacing.x);
            value_changed |= ImGui::InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::PopItemWidth();
            char* p = buf;
            while (*p == '#' || ImCharIsSpace(*p)) 
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

    ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
    if (ImGui::ColorButton(col_display))
    {
        // Don't set local copy of 'edit_mode' right away!
        g.ColorEditModeStorage.SetInt(id, (edit_mode + 1) % 3);
    }

    if (window->DC.ColorEditMode == ImGuiColorEditMode_UserSelectShowButton)
    {
        ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
        const char* button_titles[3] = { "RGB", "HSV", "HEX" };
        if (ImGui::Button(button_titles[edit_mode]))
        {
            // Don't set local copy of 'edit_mode' right away!
            g.ColorEditModeStorage.SetInt(id, (edit_mode + 1) % 3);
        }
        ImGui::SameLine();
    }
    else
    {
        ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
    }

    ImGui::TextUnformatted(label, FindTextDisplayEnd(label));

    // Convert back
    fx = ix / 255.0f;
    fy = iy / 255.0f;
    fz = iz / 255.0f;
    fw = iw / 255.0f;
    if (edit_mode == 1)
        ImGui::ColorConvertHSVtoRGB(fx, fy, fz, fx, fy, fz);

    if (value_changed)
    {
        col[0] = fx;
        col[1] = fy;
        col[2] = fz;
        if (alpha)
            col[3] = fw;
    }

    ImGui::PopID();
    ImGui::EndGroup();

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

    const ImRect bb(ImVec2(window->Pos.x, window->DC.CursorPos.y), ImVec2(window->Pos.x + window->Size.x, window->DC.CursorPos.y));
    ItemSize(ImVec2(0.0f, bb.GetSize().y)); // NB: we don't provide our width so that it doesn't get feed back into AutoFit
    if (!ItemAdd(bb, NULL))
    {
        if (window->DC.ColumnsCount > 1)
            PushColumnClipRect();
        return;
    }

    window->DrawList->AddLine(bb.Min, bb.Max, window->Color(ImGuiCol_Border));

    ImGuiState& g = *GImGui;
    if (g.LogEnabled)
        ImGui::LogText(STR_NEWLINE "--------------------------------");

    if (window->DC.ColumnsCount > 1)
    {
        PushColumnClipRect();
        window->DC.ColumnsCellMinY = window->DC.CursorPos.y;
    }
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
static void ItemSize(ImVec2 size, float text_offset_y)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    // Always align ourselves on pixel boundaries
    const float line_height = ImMax(window->DC.CurrentLineHeight, size.y);
    const float text_base_offset = ImMax(window->DC.CurrentLineTextBaseOffset, text_offset_y);
    window->DC.CursorPosPrevLine = ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y);
    window->DC.CursorPos = ImVec2((float)(int)(window->Pos.x + window->DC.ColumnsStartX + window->DC.ColumnsOffsetX), (float)(int)(window->DC.CursorPos.y + line_height + g.Style.ItemSpacing.y));
    window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPosPrevLine.x);
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, window->DC.CursorPos.y);

    //window->DrawList->AddCircle(window->DC.CursorMaxPos, 3.0f, 0xFF0000FF, 4); // Debug

    window->DC.PrevLineHeight = line_height;
    window->DC.PrevLineTextBaseOffset = text_base_offset;
    window->DC.CurrentLineHeight = window->DC.CurrentLineTextBaseOffset = 0.0f;
}

static inline void ItemSize(const ImRect& bb, float text_offset_y)
{ 
    ItemSize(bb.GetSize(), text_offset_y); 
}

static bool IsClipped(const ImRect& bb)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    if (!bb.Overlaps(ImRect(window->ClipRectStack.back())) && !g.LogEnabled)
        return true;
    return false;
}

bool ImGui::IsClipped(const ImVec2& item_size)
{
    ImGuiWindow* window = GetCurrentWindow();
    return IsClipped(ImRect(window->DC.CursorPos, window->DC.CursorPos + item_size));
}

static bool ItemAdd(const ImRect& bb, const ImGuiID* id)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.LastItemID = id ? *id : 0;
    window->DC.LastItemRect = bb;
    if (IsClipped(bb))
    {
        window->DC.LastItemHoveredAndUsable = window->DC.LastItemHoveredRect = false;
        return false;
    }

    // This is a sensible default, but widgets are free to override it after calling ItemAdd()
    ImGuiState& g = *GImGui;
    if (IsMouseHoveringRect(bb))
    {
        // Matching the behavior of IsHovered() but ignore if ActiveId==window->MoveID (we clicked on the window background)
        // So that clicking on items with no active id such as Text() still returns true with IsItemHovered()
        window->DC.LastItemHoveredRect = true;
        window->DC.LastItemHoveredAndUsable = false;
        if (g.ActiveId == 0 || (id && g.ActiveId == *id) || g.ActiveIdIsFocusedOnly || (g.ActiveId == window->MoveID))
            if (IsWindowContentHoverable(window))
                window->DC.LastItemHoveredAndUsable = true;
    }
    else
    {
        window->DC.LastItemHoveredAndUsable = window->DC.LastItemHoveredRect = false;
    }

    return true;
}

void ImGui::BeginGroup()
{
    ImGuiWindow* window = GetCurrentWindow();

    window->DC.GroupStack.resize(window->DC.GroupStack.size() + 1);
    ImGuiGroupData& group_data = window->DC.GroupStack.back();
    group_data.BackupCursorPos = window->DC.CursorPos;
    group_data.BackupCursorMaxPos = window->DC.CursorMaxPos;
    group_data.BackupColumnsStartX = window->DC.ColumnsStartX;
    group_data.BackupCurrentLineHeight = window->DC.CurrentLineHeight;
    group_data.BackupCurrentLineTextBaseOffset = window->DC.CurrentLineTextBaseOffset;
    group_data.BackupLogLinePosY = window->DC.LogLinePosY;

    window->DC.ColumnsStartX = window->DC.CursorPos.x - window->Pos.x;
    window->DC.CursorMaxPos = window->DC.CursorPos;
    window->DC.CurrentLineHeight = 0.0f;
    window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f;
}

void ImGui::EndGroup()
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiStyle& style = ImGui::GetStyle();

    IM_ASSERT(!window->DC.GroupStack.empty());

    ImGuiGroupData& group_data = window->DC.GroupStack.back();

    ImRect group_bb(group_data.BackupCursorPos, window->DC.CursorMaxPos);
    group_bb.Max.y -= style.ItemSpacing.y;      // Cancel out last vertical spacing because we are adding one ourselves.
    group_bb.Max = ImMax(group_bb.Min, group_bb.Max);

    window->DC.CursorPos = group_data.BackupCursorPos;
    window->DC.CursorMaxPos = ImMax(group_data.BackupCursorMaxPos, window->DC.CursorMaxPos);
    window->DC.CurrentLineHeight = group_data.BackupCurrentLineHeight;
    window->DC.CurrentLineTextBaseOffset = group_data.BackupCurrentLineTextBaseOffset;      // FIXME: Ideally we'll grab the base offset from the first line of the group.
    window->DC.ColumnsStartX = group_data.BackupColumnsStartX;
    window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f;

    ItemSize(group_bb.GetSize(), group_data.BackupCurrentLineTextBaseOffset);
    ItemAdd(group_bb, NULL);

    window->DC.GroupStack.pop_back();

    //window->DrawList->AddRect(group_bb.Min, group_bb.Max, 0xFFFF00FF);   // Debug
}

// Gets back to previous line and continue with horizontal layout
//      column_x == 0   : follow on previous item
//      columm_x != 0   : align to specified column
//      spacing_w < 0   : use default spacing if column_x==0, no spacing if column_x!=0
//      spacing_w >= 0  : enforce spacing
void ImGui::SameLine(int column_x, int spacing_w)
{
    ImGuiState& g = *GImGui;
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
    window->DC.CurrentLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
    window->DC.CursorPos = ImVec2(x, y);
}

void ImGui::NextColumn()
{
    ImGuiState& g = *GImGui;
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
        window->DC.CurrentLineTextBaseOffset = 0.0f;

        PushColumnClipRect();
        ImGui::PushItemWidth(ImGui::GetColumnWidth() * 0.65f);
    }
}

int ImGui::GetColumnIndex()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.ColumnsCurrent;
}

int ImGui::GetColumnsCount()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DC.ColumnsCount;
}

float ImGui::GetColumnOffset(int column_index)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (column_index < 0)
        column_index = window->DC.ColumnsCurrent;

    // Read from cache
    IM_ASSERT(column_index < (int)window->DC.ColumnsOffsetsT.size());
    const float t = window->DC.ColumnsOffsetsT[column_index];

    const float min_x = window->DC.ColumnsStartX;
    const float max_x = window->Size.x - (g.Style.ScrollbarWidth);// - window->WindowPadding().x;
    const float offset = min_x + t * (max_x - min_x);
    return offset;
}

void ImGui::SetColumnOffset(int column_index, float offset)
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (column_index < 0)
        column_index = window->DC.ColumnsCurrent;

    IM_ASSERT(column_index < (int)window->DC.ColumnsOffsetsT.size());
    const ImGuiID column_id = window->DC.ColumnsSetID + ImGuiID(column_index);

    const float min_x = window->DC.ColumnsStartX;
    const float max_x = window->Size.x - (g.Style.ScrollbarWidth);// - window->WindowPadding().x;
    const float t = (offset - min_x) / (max_x - min_x);
    window->DC.StateStorage->SetFloat(column_id, t);
    window->DC.ColumnsOffsetsT[column_index] = t;
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
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

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
    if (window->DC.ColumnsCount != columns_count && window->DC.ColumnsCount != 1 && window->DC.ColumnsShowBorders && !window->SkipItems)
    {
        const float y1 = window->DC.ColumnsStartPos.y;
        const float y2 = window->DC.CursorPos.y;
        for (int i = 1; i < window->DC.ColumnsCount; i++)
        {
            float x = window->Pos.x + GetColumnOffset(i);
            
            const ImGuiID column_id = window->DC.ColumnsSetID + ImGuiID(i);
            const ImRect column_rect(ImVec2(x-4,y1),ImVec2(x+4,y2));

            if (IsClipped(column_rect))
                continue;

            bool hovered, held;
            ButtonBehavior(column_rect, column_id, &hovered, &held, true);
            if (hovered || held)
                g.MouseCursor = ImGuiMouseCursor_ResizeEW;

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
        // Cache column offsets
        window->DC.ColumnsOffsetsT.resize((size_t)columns_count + 1);
        for (int column_index = 0; column_index < columns_count + 1; column_index++)
        {
            const ImGuiID column_id = window->DC.ColumnsSetID + ImGuiID(column_index);
            RegisterAliveId(column_id);
            const float default_t = column_index / (float)window->DC.ColumnsCount;
            const float t = window->DC.StateStorage->GetFloat(column_id, default_t);      // Cheaply store our floating point value inside the integer (could store an union into the map?)
            window->DC.ColumnsOffsetsT[column_index] = t;
        }

        PushColumnClipRect();
        ImGui::PushItemWidth(ImGui::GetColumnWidth() * 0.65f);
    }
    else
    {
        window->DC.ColumnsOffsetsT.resize(2);
        window->DC.ColumnsOffsetsT[0] = 0.0f;
        window->DC.ColumnsOffsetsT[1] = 1.0f;
    }
}


inline void ImGui::Indent()
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ColumnsStartX += g.Style.IndentSpacing;
    window->DC.CursorPos.x = window->Pos.x + window->DC.ColumnsStartX + window->DC.ColumnsOffsetX;
}

inline void ImGui::Unindent()
{
    ImGuiState& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ColumnsStartX -= g.Style.IndentSpacing;
    window->DC.CursorPos.x = window->Pos.x + window->DC.ColumnsStartX + window->DC.ColumnsOffsetX;
}

void ImGui::TreePush(const char* str_id)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGui::Indent();
    window->DC.TreeDepth++;
    PushID(str_id ? str_id : "#TreePush");
}

void ImGui::TreePush(const void* ptr_id)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGui::Indent();
    window->DC.TreeDepth++;
    PushID(ptr_id ? ptr_id : (const void*)"#TreePush");
}

void ImGui::TreePop()
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGui::Unindent();
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
        ImFormatString(fmt, IM_ARRAYSIZE(fmt), "%%s: %s", float_format);
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

static ImVec4 GNullClipRect(-9999.0f,-9999.0f, +9999.0f, +9999.0f);

void ImDrawList::Clear()
{
    commands.resize(0);
    vtx_buffer.resize(0);
    vtx_write = NULL;
    clip_rect_stack.resize(0);
    texture_id_stack.resize(0);
}

void ImDrawList::ClearFreeMemory()
{
    commands.clear();
    vtx_buffer.clear();
    vtx_write = NULL;
    clip_rect_stack.clear();
    texture_id_stack.clear();
}

void ImDrawList::AddDrawCmd()
{
    ImDrawCmd draw_cmd;
    draw_cmd.vtx_count = 0;
    draw_cmd.clip_rect = clip_rect_stack.empty() ? GNullClipRect : clip_rect_stack.back();
    draw_cmd.texture_id = texture_id_stack.empty() ? NULL : texture_id_stack.back();
    draw_cmd.user_callback = NULL;
    draw_cmd.user_callback_data = NULL;

    IM_ASSERT(draw_cmd.clip_rect.x <= draw_cmd.clip_rect.z && draw_cmd.clip_rect.y <= draw_cmd.clip_rect.w);
    commands.push_back(draw_cmd);
}

void ImDrawList::AddCallback(ImDrawCallback callback, void* callback_data)
{
    ImDrawCmd* current_cmd = commands.empty() ? NULL : &commands.back();
    if (!current_cmd || current_cmd->vtx_count != 0 || current_cmd->user_callback != NULL)
    {
        AddDrawCmd();
        current_cmd = &commands.back();
    }
    current_cmd->user_callback = callback;
    current_cmd->user_callback_data = callback_data;

    // Force a new command after us
    // We function this way so that the most common calls (AddLine, AddRect..) always have a command to add to without doing any check.
    AddDrawCmd();
}

void ImDrawList::UpdateClipRect()
{
    ImDrawCmd* current_cmd = commands.empty() ? NULL : &commands.back();
    if (!current_cmd || (current_cmd->vtx_count != 0) || current_cmd->user_callback != NULL)
    {
        AddDrawCmd();
    }
    else
    {
        current_cmd->clip_rect = clip_rect_stack.empty() ? GNullClipRect : clip_rect_stack.back();
    }
}

// Scissoring. The values in clip_rect are x1, y1, x2, y2.
void ImDrawList::PushClipRect(const ImVec4& clip_rect)
{
    clip_rect_stack.push_back(clip_rect);
    UpdateClipRect();
}

void ImDrawList::PushClipRectFullScreen()
{
    PushClipRect(GNullClipRect);

    // This would be more correct but we're not supposed to access ImGuiState from here?
    //ImGuiState& g = *GImGui;
    //if (g.IO.DisplayVisibleMin.x != g.IO.DisplayVisibleMax.x && g.IO.DisplayVisibleMin.y != g.IO.DisplayVisibleMax.y)
    //    PushClipRect(ImVec4(g.IO.DisplayVisibleMin.x, g.IO.DisplayVisibleMin.y, g.IO.DisplayVisibleMax.x, g.IO.DisplayVisibleMax.y));
    //else
    //    PushClipRect(ImVec4(0.0f, 0.0f, g.IO.DisplaySize.x, g.IO.DisplaySize.y));
}

void ImDrawList::PopClipRect()
{
    IM_ASSERT(clip_rect_stack.size() > 0);
    clip_rect_stack.pop_back();
    UpdateClipRect();
}

void ImDrawList::UpdateTextureID()
{
    ImDrawCmd* current_cmd = commands.empty() ? NULL : &commands.back();
    const ImTextureID texture_id = texture_id_stack.empty() ? NULL : texture_id_stack.back();
    if (!current_cmd || (current_cmd->vtx_count != 0 && current_cmd->texture_id != texture_id) || current_cmd->user_callback != NULL)
    {
        AddDrawCmd();
    }
    else
    {
        current_cmd->texture_id = texture_id;
    }
}

void ImDrawList::PushTextureID(const ImTextureID& texture_id)
{
    texture_id_stack.push_back(texture_id);
    UpdateTextureID();
}

void ImDrawList::PopTextureID()
{
    IM_ASSERT(texture_id_stack.size() > 0);
    texture_id_stack.pop_back();
    UpdateTextureID();
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
    vtx_write->uv = GImGui->FontTexUvWhitePixel;
    vtx_write++;
}

void ImDrawList::AddVtxUV(const ImVec2& pos, ImU32 col, const ImVec2& uv)
{
    vtx_write->pos = pos;
    vtx_write->col = col;
    vtx_write->uv = uv;
    vtx_write++;
}

// NB: memory should be reserved for 6 vertices by the caller.
void ImDrawList::AddVtxLine(const ImVec2& a, const ImVec2& b, ImU32 col, float thickness)
{
    const float inv_length = 1.0f / sqrtf(ImLengthSqr(b - a));
    const ImVec2 hn = (b - a) * (thickness * 0.5f * inv_length);// half normal
    const ImVec2 hp0 = ImVec2(+hn.y, -hn.x);                    // half perpendiculars + user offset
    const ImVec2 hp1 = ImVec2(-hn.y, +hn.x);

    // Two triangles makes up one line. Using triangles allows us to reduce amount of draw calls.
    AddVtx(a + hp0, col);
    AddVtx(b + hp0, col);
    AddVtx(a + hp1, col);
    AddVtx(b + hp0, col);
    AddVtx(b + hp1, col);
    AddVtx(a + hp1, col);
}

void ImDrawList::AddLine(const ImVec2& a, const ImVec2& b, ImU32 col, float thickness)
{
    if ((col >> 24) == 0)
        return;

    ReserveVertices(6);
    AddVtxLine(a, b, col, thickness);
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

    ReserveVertices(3);
    AddVtx(a, col);
    AddVtx(b, col);
    AddVtx(c, col);
}

void ImDrawList::AddCircle(const ImVec2& centre, float radius, ImU32 col, int num_segments)
{
    if ((col >> 24) == 0)
        return;

    ReserveVertices((unsigned int)num_segments*6);
    const float a_step = 2*PI/(float)num_segments;
    float a0 = 0.0f;
    for (int i = 0; i < num_segments; i++)
    {
        const float a1 = (i + 1) == num_segments ? 0.0f : a0 + a_step;
        AddVtxLine(centre + ImVec2(cosf(a0), sinf(a0))*radius, centre + ImVec2(cosf(a1), sinf(a1))*radius, col);
        a0 = a1;
    }
}

void ImDrawList::AddCircleFilled(const ImVec2& centre, float radius, ImU32 col, int num_segments)
{
    if ((col >> 24) == 0)
        return;

    ReserveVertices((unsigned int)num_segments*3);
    const float a_step = 2*PI/(float)num_segments;
    float a0 = 0.0f;
    for (int i = 0; i < num_segments; i++)
    {
        const float a1 = (i + 1) == num_segments ? 0.0f : a0 + a_step;
        AddVtx(centre + ImVec2(cosf(a0), sinf(a0))*radius, col);
        AddVtx(centre + ImVec2(cosf(a1), sinf(a1))*radius, col);
        AddVtx(centre, col);
        a0 = a1;
    }
}

void ImDrawList::AddText(const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width, const ImVec2* cpu_clip_max)
{
    if ((col >> 24) == 0)
        return;

    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);

    IM_ASSERT(font->ContainerAtlas->TexID == texture_id_stack.back());  // Use high-level ImGui::PushFont() or low-level ImDrawList::PushTextureId() to change font.

    // reserve vertices for worse case
    const unsigned int char_count = (unsigned int)(text_end - text_begin);
    const unsigned int vtx_count_max = char_count * 6;
    const size_t vtx_begin = vtx_buffer.size();
    ReserveVertices(vtx_count_max);

    font->RenderText(font_size, pos, col, clip_rect_stack.back(), text_begin, text_end, vtx_write, wrap_width, cpu_clip_max);

    // give back unused vertices
    vtx_buffer.resize((size_t)(vtx_write - &vtx_buffer.front()));
    const size_t vtx_count = vtx_buffer.size() - vtx_begin;
    commands.back().vtx_count -= (unsigned int)(vtx_count_max - vtx_count);
    vtx_write -= (vtx_count_max - vtx_count);
}

void ImDrawList::AddImage(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv0, const ImVec2& uv1, ImU32 col)
{
    if ((col >> 24) == 0)
        return;

    // FIXME-OPT: This is wasting draw calls.
    const bool push_texture_id = texture_id_stack.empty() || user_texture_id != texture_id_stack.back();
    if (push_texture_id)
        PushTextureID(user_texture_id);

    ReserveVertices(6);
    AddVtxUV(ImVec2(a.x,a.y), col, uv0);
    AddVtxUV(ImVec2(b.x,a.y), col, ImVec2(uv1.x,uv0.y));
    AddVtxUV(ImVec2(b.x,b.y), col, uv1);
    AddVtxUV(ImVec2(a.x,a.y), col, ImVec2(uv0.x,uv0.y));
    AddVtxUV(ImVec2(b.x,b.y), col, uv1);
    AddVtxUV(ImVec2(a.x,b.y), col, ImVec2(uv0.x,uv1.y));

    if (push_texture_id)
        PopTextureID();
}

//-----------------------------------------------------------------------------
// ImFontAtlias
//-----------------------------------------------------------------------------

struct ImFontAtlas::ImFontAtlasData
{
    // Input
    ImFont*             OutFont;        // Load into this font
    void*               TTFData;        // TTF data, we own the memory
    size_t              TTFDataSize;    // TTF data size, in bytes
    float               SizePixels;     // Desired output size, in pixels
    const ImWchar*      GlyphRanges;    // List of Unicode range (2 value per range, values are inclusive, zero-terminated list)
    int                 FontNo;         // Index of font within .TTF file (0)

    // Temporary Build Data
    stbtt_fontinfo      FontInfo;
    stbrp_rect*         Rects;
    stbtt_pack_range*   Ranges;
    int                 RangesCount;
};

ImFontAtlas::ImFontAtlas()
{
    TexID = NULL;
    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
    TexWidth = TexHeight = 0;
    TexUvWhitePixel = ImVec2(0, 0);
}

ImFontAtlas::~ImFontAtlas()
{
    Clear();
}

void    ImFontAtlas::ClearInputData()
{
    for (size_t i = 0; i < InputData.size(); i++)
    {
        if (InputData[i]->TTFData)
            ImGui::MemFree(InputData[i]->TTFData);
        ImGui::MemFree(InputData[i]);
    }
    InputData.clear();
}

void    ImFontAtlas::ClearTexData()
{
    if (TexPixelsAlpha8)
        ImGui::MemFree(TexPixelsAlpha8);
    if (TexPixelsRGBA32)
        ImGui::MemFree(TexPixelsRGBA32);
    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
}

void    ImFontAtlas::Clear()
{
    ClearInputData(); 
    ClearTexData();
    for (size_t i = 0; i < Fonts.size(); i++)
    {
        Fonts[i]->~ImFont();
        ImGui::MemFree(Fonts[i]);
    }
    Fonts.clear();
}

void ImGui::GetDefaultFontData(const void** fnt_data, unsigned int* fnt_size, const void** png_data, unsigned int* png_size)
{
    printf("GetDefaultFontData() is obsoleted in ImGui 1.30.\n");
    printf("Please use ImGui::GetIO().Fonts->GetTexDataAsRGBA32() or GetTexDataAsAlpha8() functions to retrieve uncompressed texture data.\n");
    if (fnt_data) *fnt_data = NULL;
    if (fnt_size) *fnt_size = 0;
    if (png_data) *png_data = NULL;
    if (png_size) *png_size = 0;
    IM_ASSERT(false);
}

void    ImFontAtlas::GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Lazily build
    if (TexPixelsAlpha8 == NULL)
    {
        if (InputData.empty())
            AddFontDefault();
        Build();
    }

    *out_pixels = TexPixelsAlpha8;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 1;
}

void    ImFontAtlas::GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Lazily convert to RGBA32 format
    // Although it is likely to be the most commonly used format, our font rendering is 8 bpp
    if (!TexPixelsRGBA32)
    {
        unsigned char* pixels;
        GetTexDataAsAlpha8(&pixels, NULL, NULL);
        TexPixelsRGBA32 = (unsigned int*)ImGui::MemAlloc((size_t)(TexWidth * TexHeight * 4));
        const unsigned char* src = pixels;
        unsigned int* dst = TexPixelsRGBA32;
        for (int n = TexWidth * TexHeight; n > 0; n--)
            *dst++ = ((unsigned int)(*src++) << 24) | 0x00FFFFFF;
    }

    *out_pixels = (unsigned char*)TexPixelsRGBA32;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 4;
}

static void GetDefaultCompressedFontDataTTF(const void** ttf_compressed_data, unsigned int* ttf_compressed_size);
static unsigned int stb_decompress_length(unsigned char *input);
static unsigned int stb_decompress(unsigned char *output, unsigned char *i, unsigned int length);

// Load embedded ProggyClean.ttf at size 13
ImFont* ImFontAtlas::AddFontDefault()
{
    // Get compressed data
    unsigned int ttf_compressed_size;
    const void* ttf_compressed;
    GetDefaultCompressedFontDataTTF(&ttf_compressed, &ttf_compressed_size);

    // Decompress
    const size_t buf_decompressed_size = stb_decompress_length((unsigned char*)ttf_compressed);
    unsigned char* buf_decompressed = (unsigned char *)ImGui::MemAlloc(buf_decompressed_size);
    stb_decompress(buf_decompressed, (unsigned char*)ttf_compressed, ttf_compressed_size);

    // Add
    ImFont* font = AddFontFromMemoryTTF(buf_decompressed, buf_decompressed_size, 13.0f, GetGlyphRangesDefault(), 0);
    font->DisplayOffset.y += 1;
    return font;
}

ImFont* ImFontAtlas::AddFontFromFileTTF(const char* filename, float size_pixels, const ImWchar* glyph_ranges, int font_no)
{
    void* data = NULL;
    size_t data_size = 0;
    if (!ImLoadFileToMemory(filename, "rb", (void**)&data, &data_size))
    {
        IM_ASSERT(0); // Could not load file.
        return NULL;
    }

    // Add
    ImFont* font = AddFontFromMemoryTTF(data, data_size, size_pixels, glyph_ranges, font_no);
    return font;
}

// NB: ownership of 'data' is given to ImFontAtlas which will clear it.
ImFont* ImFontAtlas::AddFontFromMemoryTTF(void* in_ttf_data, size_t in_ttf_data_size, float size_pixels, const ImWchar* glyph_ranges, int font_no)
{
    // Create new font
    ImFont* font = (ImFont*)ImGui::MemAlloc(sizeof(ImFont));
    new (font) ImFont();
    Fonts.push_back(font);

    // Add to build list
    ImFontAtlasData* data = (ImFontAtlasData*)ImGui::MemAlloc(sizeof(ImFontAtlasData));
    memset(data, 0, sizeof(ImFontAtlasData));
    data->OutFont = font;
    data->TTFData = in_ttf_data;
    data->TTFDataSize = in_ttf_data_size;
    data->SizePixels = size_pixels;
    data->GlyphRanges = glyph_ranges;
    data->FontNo = font_no;
    InputData.push_back(data);

    // Invalidate texture
    ClearTexData();

    return font;
}

bool    ImFontAtlas::Build()
{
    IM_ASSERT(InputData.size() > 0);

    TexID = NULL;
    TexWidth = TexHeight = 0;
    TexUvWhitePixel = ImVec2(0, 0);
    ClearTexData();

    // Initialize font information early (so we can error without any cleanup) + count glyphs
    int total_glyph_count = 0;
    int total_glyph_range_count = 0;
    for (size_t input_i = 0; input_i < InputData.size(); input_i++)
    {
        ImFontAtlasData& data = *InputData[input_i];
        IM_ASSERT(data.OutFont && !data.OutFont->IsLoaded());
        const int font_offset = stbtt_GetFontOffsetForIndex((unsigned char*)data.TTFData, data.FontNo);
        IM_ASSERT(font_offset >= 0);
        if (!stbtt_InitFont(&data.FontInfo, (unsigned char*)data.TTFData, font_offset)) 
            return false;

        if (!data.GlyphRanges)
            data.GlyphRanges = GetGlyphRangesDefault();
        for (const ImWchar* in_range = data.GlyphRanges; in_range[0] && in_range[1]; in_range += 2)
        {
            total_glyph_count += (in_range[1] - in_range[0]) + 1;
            total_glyph_range_count++;
        }
    }

    // Start packing
    TexWidth = (total_glyph_count > 1000) ? 1024 : 512;  // Width doesn't actually matters.
    TexHeight = 0;
    const int max_tex_height = 1024*32;
    stbtt_pack_context spc;
    int ret = stbtt_PackBegin(&spc, NULL, TexWidth, max_tex_height, 0, 1, NULL);
    IM_ASSERT(ret);
    stbtt_PackSetOversampling(&spc, 1, 1);

    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    ImVector<stbrp_rect> extra_rects;
    RenderCustomTexData(0, &extra_rects);
    stbrp_pack_rects((stbrp_context*)spc.pack_info, &extra_rects[0], (int)extra_rects.size());
    for (size_t i = 0; i < extra_rects.size(); i++)
        if (extra_rects[i].was_packed)
            TexHeight = ImMax(TexHeight, extra_rects[i].y + extra_rects[i].h);

    // Allocate packing character data and flag packed characters buffer as non-packed (x0=y0=x1=y1=0)
    int buf_packedchars_n = 0, buf_rects_n = 0, buf_ranges_n = 0;
    stbtt_packedchar* buf_packedchars = (stbtt_packedchar*)ImGui::MemAlloc(total_glyph_count * sizeof(stbtt_packedchar));
    stbrp_rect* buf_rects = (stbrp_rect*)ImGui::MemAlloc(total_glyph_count * sizeof(stbrp_rect));
    stbtt_pack_range* buf_ranges = (stbtt_pack_range*)ImGui::MemAlloc(total_glyph_range_count * sizeof(stbtt_pack_range));
    memset(buf_packedchars, 0, total_glyph_count * sizeof(stbtt_packedchar));
    memset(buf_rects, 0, total_glyph_count * sizeof(stbrp_rect));              // Unnecessary but let's clear this for the sake of sanity.
    memset(buf_ranges, 0, total_glyph_range_count * sizeof(stbtt_pack_range));

    // First font pass: pack all glyphs (no rendering at this point, we are working with glyph sizes only)
    for (size_t input_i = 0; input_i < InputData.size(); input_i++)
    {
        ImFontAtlasData& data = *InputData[input_i];

        // Setup ranges
        int glyph_count = 0;
        int glyph_ranges_count = 0;
        for (const ImWchar* in_range = data.GlyphRanges; in_range[0] && in_range[1]; in_range += 2)
        {
            glyph_count += (in_range[1] - in_range[0]) + 1;
            glyph_ranges_count++;
        }
        data.Ranges = buf_ranges + buf_ranges_n;
        data.RangesCount = glyph_ranges_count;
        buf_ranges_n += glyph_ranges_count;
        for (int i = 0; i < glyph_ranges_count; i++)
        {
            const ImWchar* in_range = &data.GlyphRanges[i * 2];
            stbtt_pack_range& range = data.Ranges[i];
            range.font_size = data.SizePixels;
            range.first_unicode_char_in_range = in_range[0];
            range.num_chars_in_range = (in_range[1] - in_range[0]) + 1;
            range.chardata_for_range = buf_packedchars + buf_packedchars_n;
            buf_packedchars_n += range.num_chars_in_range;
        }

        // Pack
        data.Rects = buf_rects + buf_rects_n;
        buf_rects_n += glyph_count;
        const int n = stbtt_PackFontRangesGatherRects(&spc, &data.FontInfo, data.Ranges, data.RangesCount, data.Rects);
        stbrp_pack_rects((stbrp_context*)spc.pack_info, data.Rects, n);

        // Extend texture height
        for (int i = 0; i < n; i++)
            if (data.Rects[i].was_packed)
                TexHeight = ImMax(TexHeight, data.Rects[i].y + data.Rects[i].h);
    }
    IM_ASSERT(buf_rects_n == total_glyph_count);
    IM_ASSERT(buf_packedchars_n == total_glyph_count);
    IM_ASSERT(buf_ranges_n == total_glyph_range_count);

    // Create texture
    TexHeight = ImUpperPowerOfTwo(TexHeight);
    TexPixelsAlpha8 = (unsigned char*)ImGui::MemAlloc(TexWidth * TexHeight);
    memset(TexPixelsAlpha8, 0, TexWidth * TexHeight);
    spc.pixels = TexPixelsAlpha8;
    spc.height = TexHeight;

    // Second pass: render characters
    for (size_t input_i = 0; input_i < InputData.size(); input_i++)
    {
        ImFontAtlasData& data = *InputData[input_i];
        ret = stbtt_PackFontRangesRenderIntoRects(&spc, &data.FontInfo, data.Ranges, data.RangesCount, data.Rects);
        data.Rects = NULL;
    }

    // End packing
    stbtt_PackEnd(&spc);
    ImGui::MemFree(buf_rects);
    buf_rects = NULL;

    // Third pass: setup ImFont and glyphs for runtime
    for (size_t input_i = 0; input_i < InputData.size(); input_i++)
    {
        ImFontAtlasData& data = *InputData[input_i];
        data.OutFont->ContainerAtlas = this;
        data.OutFont->FontSize = data.SizePixels;

        const float font_scale = stbtt_ScaleForPixelHeight(&data.FontInfo, data.SizePixels);
        int font_ascent, font_descent, font_line_gap;
        stbtt_GetFontVMetrics(&data.FontInfo, &font_ascent, &font_descent, &font_line_gap);

        const float uv_scale_x = 1.0f / TexWidth;
        const float uv_scale_y = 1.0f / TexHeight;
        const int character_spacing_x = 1;
        for (int i = 0; i < data.RangesCount; i++)
        {
            stbtt_pack_range& range = data.Ranges[i];
            for (int char_idx = 0; char_idx < range.num_chars_in_range; char_idx += 1)
            {
                const int codepoint = range.first_unicode_char_in_range + char_idx;
                const stbtt_packedchar& pc = range.chardata_for_range[char_idx];
                if (!pc.x0 && !pc.x1 && !pc.y0 && !pc.y1)
                    continue;

                data.OutFont->Glyphs.resize(data.OutFont->Glyphs.size() + 1);
                ImFont::Glyph& glyph = data.OutFont->Glyphs.back();
                glyph.Codepoint = (ImWchar)codepoint;
                glyph.Width = (signed short)pc.x1 - pc.x0 + 1;
                glyph.Height = (signed short)pc.y1 - pc.y0 + 1;
                glyph.XOffset = (signed short)(pc.xoff);
                glyph.YOffset = (signed short)(pc.yoff + (int)(font_ascent * font_scale));
                glyph.XAdvance = (signed short)(pc.xadvance + character_spacing_x);  // Bake spacing into XAdvance
                glyph.U0 = ((float)pc.x0 - 0.5f) * uv_scale_x;
                glyph.V0 = ((float)pc.y0 - 0.5f) * uv_scale_y;
                glyph.U1 = ((float)pc.x0 - 0.5f + glyph.Width) * uv_scale_x;
                glyph.V1 = ((float)pc.y0 - 0.5f + glyph.Height) * uv_scale_y;
            }
        }

        data.OutFont->BuildLookupTable();
    }

    // Cleanup temporaries
    ImGui::MemFree(buf_packedchars);
    ImGui::MemFree(buf_ranges);
    buf_packedchars = NULL;
    buf_ranges = NULL;
    ClearInputData();

    // Render into our custom data block
    RenderCustomTexData(1, &extra_rects);

    return true;
}

void ImFontAtlas::RenderCustomTexData(int pass, void* p_rects)
{
    // . = white layer, X = black layer, others are blank
    const int TEX_DATA_W = 90;
    const int TEX_DATA_H = 27;
    const char texture_data[TEX_DATA_W*TEX_DATA_H+1] =
    {
        "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX"
        "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X"
        "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X"
        "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X"
        "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X"
        "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X"
        "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX"
        "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      "
        "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       "
        "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        "
        "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         "
        "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          "
        "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           "
        "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            "
        "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           "
        "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          "
        "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          "
        "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       ------------------------------------"
        "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           "
        "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           "
        "      X..X          -  X...X  -         X...X         -  X..X           X..X  -           "
        "       XX           -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           "
        "------------        -    X    -           X           -X.....................X-           "
        "                    ----------------------------------- X...XXXXXXXXXXXXX...X -           "
        "                                                      -  X..X           X..X  -           "
        "                                                      -   X.X           X.X   -           "
        "                                                      -    XX           XX    -           "
    };

    ImVector<stbrp_rect>& rects = *(ImVector<stbrp_rect>*)p_rects;
    if (pass == 0)
    {
        stbrp_rect r = { 0 };
        r.w = (TEX_DATA_W*2)+1;
        r.h = TEX_DATA_H+1;
        rects.push_back(r);
    }
    else if (pass == 1)
    {
        // Copy pixels
        const stbrp_rect& r = rects[0];
        for (int y = 0, n = 0; y < TEX_DATA_H; y++)
            for (int x = 0; x < TEX_DATA_W; x++, n++)
            {
                const int offset0 = (int)(r.x + x) + (int)(r.y + y) * TexWidth;
                const int offset1 = offset0 + 1 + TEX_DATA_W;
                TexPixelsAlpha8[offset0] = texture_data[n] == '.' ? 0xFF : 0x00;
                TexPixelsAlpha8[offset1] = texture_data[n] == 'X' ? 0xFF : 0x00;
            }
        const ImVec2 tex_uv_scale(1.0f / TexWidth, 1.0f / TexHeight);
        TexUvWhitePixel = ImVec2(r.x + 0.5f, r.y + 0.5f) * tex_uv_scale;

        const ImVec2 cursor_datas[ImGuiMouseCursor_Count_][3] =
        {
            // Pos ........ Size ......... Offset ......
            { ImVec2(0,3),  ImVec2(12,19), ImVec2( 0, 0) }, // ImGuiMouseCursor_Arrow
            { ImVec2(13,0), ImVec2(7,16),  ImVec2( 4, 8) }, // ImGuiMouseCursor_TextInput
            { ImVec2(31,0), ImVec2(23,23), ImVec2(11,11) }, // ImGuiMouseCursor_Move
            { ImVec2(21,0), ImVec2( 9,23), ImVec2( 5,11) }, // ImGuiMouseCursor_ResizeNS
            { ImVec2(55,18),ImVec2(23, 9), ImVec2(11, 5) }, // ImGuiMouseCursor_ResizeEW
            { ImVec2(73,0), ImVec2(17,17), ImVec2( 9, 9) }, // ImGuiMouseCursor_ResizeNESW
            { ImVec2(55,0), ImVec2(17,17), ImVec2( 9, 9) }, // ImGuiMouseCursor_ResizeNWSE
        };

        for (int type = 0; type < ImGuiMouseCursor_Count_; type++)
        {
            ImGuiMouseCursorData& cursor_data = GImGui->MouseCursorData[type];
            ImVec2 pos = cursor_datas[type][0] + ImVec2((float)r.x, (float)r.y);
            const ImVec2 size = cursor_datas[type][1];
            cursor_data.Type = type;
            cursor_data.Size = size;
            cursor_data.Offset = cursor_datas[type][2];
            cursor_data.TexUvMin[0] = (pos) * tex_uv_scale;
            cursor_data.TexUvMax[0] = (pos + size) * tex_uv_scale;
            pos.x += TEX_DATA_W+1;
            cursor_data.TexUvMin[1] = (pos) * tex_uv_scale;
            cursor_data.TexUvMax[1] = (pos + size) * tex_uv_scale;
        }
    }
}

//-----------------------------------------------------------------------------
// ImFont
//-----------------------------------------------------------------------------

ImFont::ImFont()
{
    Scale = 1.0f;
    FallbackChar = (ImWchar)'?';
    Clear();
}

void    ImFont::Clear()
{
    FontSize = 0.0f;
    DisplayOffset = ImVec2(-0.5f, 0.5f);
    ContainerAtlas = NULL;
    Glyphs.clear();
    FallbackGlyph = NULL;
    FallbackXAdvance = 0.0f;
    IndexXAdvance.clear();
    IndexLookup.clear();
}

// Retrieve list of range (2 int per range, values are inclusive)
const ImWchar*   ImFontAtlas::GetGlyphRangesDefault()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesChinese()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesJapanese()
{
    // Store the 1946 ideograms code points as successive offsets from the initial unicode codepoint 0x4E00. Each offset has an implicit +1.
    // This encoding helps us reduce the source code size.
    static const short offsets_from_0x4E00[] = 
    {
        -1,0,1,3,0,0,0,0,1,0,5,1,1,0,7,4,6,10,0,1,9,9,7,1,3,19,1,10,7,1,0,1,0,5,1,0,6,4,2,6,0,0,12,6,8,0,3,5,0,1,0,9,0,0,8,1,1,3,4,5,13,0,0,8,2,17,
        4,3,1,1,9,6,0,0,0,2,1,3,2,22,1,9,11,1,13,1,3,12,0,5,9,2,0,6,12,5,3,12,4,1,2,16,1,1,4,6,5,3,0,6,13,15,5,12,8,14,0,0,6,15,3,6,0,18,8,1,6,14,1,
        5,4,12,24,3,13,12,10,24,0,0,0,1,0,1,1,2,9,10,2,2,0,0,3,3,1,0,3,8,0,3,2,4,4,1,6,11,10,14,6,15,3,4,15,1,0,0,5,2,2,0,0,1,6,5,5,6,0,3,6,5,0,0,1,0,
        11,2,2,8,4,7,0,10,0,1,2,17,19,3,0,2,5,0,6,2,4,4,6,1,1,11,2,0,3,1,2,1,2,10,7,6,3,16,0,8,24,0,0,3,1,1,3,0,1,6,0,0,0,2,0,1,5,15,0,1,0,0,2,11,19,
        1,4,19,7,6,5,1,0,0,0,0,5,1,0,1,9,0,0,5,0,2,0,1,0,3,0,11,3,0,2,0,0,0,0,0,9,3,6,4,12,0,14,0,0,29,10,8,0,14,37,13,0,31,16,19,0,8,30,1,20,8,3,48,
        21,1,0,12,0,10,44,34,42,54,11,18,82,0,2,1,2,12,1,0,6,2,17,2,12,7,0,7,17,4,2,6,24,23,8,23,39,2,16,23,1,0,5,1,2,15,14,5,6,2,11,0,8,6,2,2,2,14,
        20,4,15,3,4,11,10,10,2,5,2,1,30,2,1,0,0,22,5,5,0,3,1,5,4,1,0,0,2,2,21,1,5,1,2,16,2,1,3,4,0,8,4,0,0,5,14,11,2,16,1,13,1,7,0,22,15,3,1,22,7,14,
        22,19,11,24,18,46,10,20,64,45,3,2,0,4,5,0,1,4,25,1,0,0,2,10,0,0,0,1,0,1,2,0,0,9,1,2,0,0,0,2,5,2,1,1,5,5,8,1,1,1,5,1,4,9,1,3,0,1,0,1,1,2,0,0,
        2,0,1,8,22,8,1,0,0,0,0,4,2,1,0,9,8,5,0,9,1,30,24,2,6,4,39,0,14,5,16,6,26,179,0,2,1,1,0,0,0,5,2,9,6,0,2,5,16,7,5,1,1,0,2,4,4,7,15,13,14,0,0,
        3,0,1,0,0,0,2,1,6,4,5,1,4,9,0,3,1,8,0,0,10,5,0,43,0,2,6,8,4,0,2,0,0,9,6,0,9,3,1,6,20,14,6,1,4,0,7,2,3,0,2,0,5,0,3,1,0,3,9,7,0,3,4,0,4,9,1,6,0,
        9,0,0,2,3,10,9,28,3,6,2,4,1,2,32,4,1,18,2,0,3,1,5,30,10,0,2,2,2,0,7,9,8,11,10,11,7,2,13,7,5,10,0,3,40,2,0,1,6,12,0,4,5,1,5,11,11,21,4,8,3,7,
        8,8,33,5,23,0,0,19,8,8,2,3,0,6,1,1,1,5,1,27,4,2,5,0,3,5,6,3,1,0,3,1,12,5,3,3,2,0,7,7,2,1,0,4,0,1,1,2,0,10,10,6,2,5,9,7,5,15,15,21,6,11,5,20,
        4,3,5,5,2,5,0,2,1,0,1,7,28,0,9,0,5,12,5,5,18,30,0,12,3,3,21,16,25,32,9,3,14,11,24,5,66,9,1,2,0,5,9,1,5,1,8,0,8,3,3,0,1,15,1,4,8,1,2,7,0,7,2,
        8,3,7,5,3,7,10,2,1,0,0,2,25,0,6,4,0,10,0,4,2,4,1,12,5,38,4,0,4,1,10,5,9,4,0,14,4,2,5,18,20,21,1,3,0,5,0,7,0,3,7,1,3,1,1,8,1,0,0,0,3,2,5,2,11,
        6,0,13,1,3,9,1,12,0,16,6,2,1,0,2,1,12,6,13,11,2,0,28,1,7,8,14,13,8,13,0,2,0,5,4,8,10,2,37,42,19,6,6,7,4,14,11,18,14,80,7,6,0,4,72,12,36,27,
        7,7,0,14,17,19,164,27,0,5,10,7,3,13,6,14,0,2,2,5,3,0,6,13,0,0,10,29,0,4,0,3,13,0,3,1,6,51,1,5,28,2,0,8,0,20,2,4,0,25,2,10,13,10,0,16,4,0,1,0,
        2,1,7,0,1,8,11,0,0,1,2,7,2,23,11,6,6,4,16,2,2,2,0,22,9,3,3,5,2,0,15,16,21,2,9,20,15,15,5,3,9,1,0,0,1,7,7,5,4,2,2,2,38,24,14,0,0,15,5,6,24,14,
        5,5,11,0,21,12,0,3,8,4,11,1,8,0,11,27,7,2,4,9,21,59,0,1,39,3,60,62,3,0,12,11,0,3,30,11,0,13,88,4,15,5,28,13,1,4,48,17,17,4,28,32,46,0,16,0,
        18,11,1,8,6,38,11,2,6,11,38,2,0,45,3,11,2,7,8,4,30,14,17,2,1,1,65,18,12,16,4,2,45,123,12,56,33,1,4,3,4,7,0,0,0,3,2,0,16,4,2,4,2,0,7,4,5,2,26,
        2,25,6,11,6,1,16,2,6,17,77,15,3,35,0,1,0,5,1,0,38,16,6,3,12,3,3,3,0,9,3,1,3,5,2,9,0,18,0,25,1,3,32,1,72,46,6,2,7,1,3,14,17,0,28,1,40,13,0,20,
        15,40,6,38,24,12,43,1,1,9,0,12,6,0,6,2,4,19,3,7,1,48,0,9,5,0,5,6,9,6,10,15,2,11,19,3,9,2,0,1,10,1,27,8,1,3,6,1,14,0,26,0,27,16,3,4,9,6,2,23,
        9,10,5,25,2,1,6,1,1,48,15,9,15,14,3,4,26,60,29,13,37,21,1,6,4,0,2,11,22,23,16,16,2,2,1,3,0,5,1,6,4,0,0,4,0,0,8,3,0,2,5,0,7,1,7,3,13,2,4,10,
        3,0,2,31,0,18,3,0,12,10,4,1,0,7,5,7,0,5,4,12,2,22,10,4,2,15,2,8,9,0,23,2,197,51,3,1,1,4,13,4,3,21,4,19,3,10,5,40,0,4,1,1,10,4,1,27,34,7,21,
        2,17,2,9,6,4,2,3,0,4,2,7,8,2,5,1,15,21,3,4,4,2,2,17,22,1,5,22,4,26,7,0,32,1,11,42,15,4,1,2,5,0,19,3,1,8,6,0,10,1,9,2,13,30,8,2,24,17,19,1,4,
        4,25,13,0,10,16,11,39,18,8,5,30,82,1,6,8,18,77,11,13,20,75,11,112,78,33,3,0,0,60,17,84,9,1,1,12,30,10,49,5,32,158,178,5,5,6,3,3,1,3,1,4,7,6,
        19,31,21,0,2,9,5,6,27,4,9,8,1,76,18,12,1,4,0,3,3,6,3,12,2,8,30,16,2,25,1,5,5,4,3,0,6,10,2,3,1,0,5,1,19,3,0,8,1,5,2,6,0,0,0,19,1,2,0,5,1,2,5,
        1,3,7,0,4,12,7,3,10,22,0,9,5,1,0,2,20,1,1,3,23,30,3,9,9,1,4,191,14,3,15,6,8,50,0,1,0,0,4,0,0,1,0,2,4,2,0,2,3,0,2,0,2,2,8,7,0,1,1,1,3,3,17,11,
        91,1,9,3,2,13,4,24,15,41,3,13,3,1,20,4,125,29,30,1,0,4,12,2,21,4,5,5,19,11,0,13,11,86,2,18,0,7,1,8,8,2,2,22,1,2,6,5,2,0,1,2,8,0,2,0,5,2,1,0,
        2,10,2,0,5,9,2,1,2,0,1,0,4,0,0,10,2,5,3,0,6,1,0,1,4,4,33,3,13,17,3,18,6,4,7,1,5,78,0,4,1,13,7,1,8,1,0,35,27,15,3,0,0,0,1,11,5,41,38,15,22,6,
        14,14,2,1,11,6,20,63,5,8,27,7,11,2,2,40,58,23,50,54,56,293,8,8,1,5,1,14,0,1,12,37,89,8,8,8,2,10,6,0,0,0,4,5,2,1,0,1,1,2,7,0,3,3,0,4,6,0,3,2,
        19,3,8,0,0,0,4,4,16,0,4,1,5,1,3,0,3,4,6,2,17,10,10,31,6,4,3,6,10,126,7,3,2,2,0,9,0,0,5,20,13,0,15,0,6,0,2,5,8,64,50,3,2,12,2,9,0,0,11,8,20,
        109,2,18,23,0,0,9,61,3,0,28,41,77,27,19,17,81,5,2,14,5,83,57,252,14,154,263,14,20,8,13,6,57,39,38,
    };
    static int ranges_unpacked = false;
    static ImWchar ranges[8 + IM_ARRAYSIZE(offsets_from_0x4E00)*2 + 1] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
    };
    if (!ranges_unpacked)
    {
        // Unpack
        int codepoint = 0x4e00;
        ImWchar* dst = &ranges[8];
        for (int n = 0; n < IM_ARRAYSIZE(offsets_from_0x4E00); n++, dst += 2)
            dst[0] = dst[1] = (ImWchar)(codepoint += (offsets_from_0x4E00[n] + 1));
        dst[0] = 0;
        ranges_unpacked = true;
    }
    return &ranges[0];
}

void ImFont::BuildLookupTable()
{
    int max_codepoint = 0;
    for (size_t i = 0; i != Glyphs.size(); i++)
        max_codepoint = ImMax(max_codepoint, (int)Glyphs[i].Codepoint);

    IndexXAdvance.clear();
    IndexXAdvance.resize((size_t)max_codepoint + 1);
    IndexLookup.clear();
    IndexLookup.resize((size_t)max_codepoint + 1);
    for (size_t i = 0; i < (size_t)max_codepoint + 1; i++)
    {
        IndexXAdvance[i] = -1.0f;
        IndexLookup[i] = -1;
    }
    for (size_t i = 0; i < Glyphs.size(); i++)
    {
        const size_t codepoint = (int)Glyphs[i].Codepoint;
        IndexXAdvance[codepoint] = Glyphs[i].XAdvance;
        IndexLookup[codepoint] = (int)i;
    }

    // Create a glyph to handle TAB
    // FIXME: Needs proper TAB handling but it needs to be contextualized (can arbitrary say that each string starts at "column 0"
    if (FindGlyph((unsigned short)' '))
    {
        if (Glyphs.back().Codepoint != '\t')   // So we can call this function multiple times
            Glyphs.resize(Glyphs.size() + 1);
        ImFont::Glyph& tab_glyph = Glyphs.back();
        tab_glyph = *FindGlyph((unsigned short)' ');
        tab_glyph.Codepoint = '\t';
        tab_glyph.XAdvance *= 4;
        IndexXAdvance[(size_t)tab_glyph.Codepoint] = (float)tab_glyph.XAdvance;
        IndexLookup[(size_t)tab_glyph.Codepoint] = (int)(Glyphs.size()-1);
    }

    FallbackGlyph = NULL;
    FallbackGlyph = FindGlyph(FallbackChar);
    FallbackXAdvance = FallbackGlyph ? FallbackGlyph->XAdvance : 0.0f;
    for (size_t i = 0; i < (size_t)max_codepoint + 1; i++)
        if (IndexXAdvance[i] < 0.0f)
            IndexXAdvance[i] = FallbackXAdvance;
}

void ImFont::SetFallbackChar(ImWchar c)
{
    FallbackChar = c;
    BuildLookupTable();
}

const ImFont::Glyph* ImFont::FindGlyph(unsigned short c) const
{
    if (c < (int)IndexLookup.size())
    {
        const int i = IndexLookup[c];
        if (i != -1)
            return &Glyphs[i];
    }
    return FallbackGlyph;
}

// Convert UTF-8 to 32-bits character, process single character input.
// Based on stb_from_utf8() from github.com/nothings/stb/
// We handle UTF-8 decoding error by skipping forward.
static int ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end)
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
        *out_char = 0;
        if (in_text_end && in_text_end - (const char*)str < 2) return 0; 
        if (*str < 0xc2) return 0;
        c = (unsigned int)((*str++ & 0x1f) << 6);
        if ((*str & 0xc0) != 0x80) return 0;
        c += (*str++ & 0x3f);
        *out_char = c;
        return 2;
    }
    if ((*str & 0xf0) == 0xe0) 
    {
        *out_char = 0;
        if (in_text_end && in_text_end - (const char*)str < 3) return 0;
        if (*str == 0xe0 && (str[1] < 0xa0 || str[1] > 0xbf)) return 0;
        if (*str == 0xed && str[1] > 0x9f) return 0; // str[1] < 0x80 is checked below
        c = (unsigned int)((*str++ & 0x0f) << 12);
        if ((*str & 0xc0) != 0x80) return 0;
        c += (unsigned int)((*str++ & 0x3f) << 6);
        if ((*str & 0xc0) != 0x80) return 0;
        c += (*str++ & 0x3f);
        *out_char = c;
        return 3;
    }
    if ((*str & 0xf8) == 0xf0) 
    {
        *out_char = 0;
        if (in_text_end && in_text_end - (const char*)str < 4) return 0;
        if (*str > 0xf4) return 0;
        if (*str == 0xf0 && (str[1] < 0x90 || str[1] > 0xbf)) return 0;
        if (*str == 0xf4 && str[1] > 0x8f) return 0; // str[1] < 0x80 is checked below
        c = (unsigned int)((*str++ & 0x07) << 18);
        if ((*str & 0xc0) != 0x80) return 0;
        c += (unsigned int)((*str++ & 0x3f) << 12);
        if ((*str & 0xc0) != 0x80) return 0;
        c += (unsigned int)((*str++ & 0x3f) << 6);
        if ((*str & 0xc0) != 0x80) return 0;
        c += (*str++ & 0x3f);
        // utf-8 encodings of values used in surrogate pairs are invalid
        if ((c & 0xFFFFF800) == 0xD800) return 0;
        *out_char = c;
        return 4;
    }
    *out_char = 0;
    return 0;
}

static ptrdiff_t ImTextStrFromUtf8(ImWchar* buf, size_t buf_size, const char* in_text, const char* in_text_end, const char** in_text_remaining)
{
    ImWchar* buf_out = buf;
    ImWchar* buf_end = buf + buf_size;
    while (buf_out < buf_end-1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        if (c == 0)
            break;
        if (c < 0x10000)    // FIXME: Losing characters that don't fit in 2 bytes
            *buf_out++ = (ImWchar)c;
    }
    *buf_out = 0;
    if (in_text_remaining)
        *in_text_remaining = in_text;
    return buf_out - buf;
}

static int ImTextCountCharsFromUtf8(const char* in_text, const char* in_text_end)
{
    int char_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        if (c == 0)
            break;
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

static int ImTextCountUtf8BytesFromStr(const ImWchar* in_text, const ImWchar* in_text_end)
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

const char* ImFont::CalcWordWrapPositionA(float scale, const char* text, const char* text_end, float wrap_width) const
{
    // Simple word-wrapping for English, not full-featured. Please submit failing cases!
    // FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;

    const char* word_end = text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = text;
    while (s < text_end)
    {
        unsigned int c = (unsigned int)*s;
        const char* next_s;
        if (c < 0x80)
            next_s = s + 1;
        else
            next_s = s + ImTextCharFromUtf8(&c, s, text_end);
        if (c == 0)
            break;

        if (c == '\n')
        {
            line_width = word_width = blank_width = 0.0f;
            inside_word = true;
            s = next_s;
            continue;
        }

        const float char_width = ((size_t)c < IndexXAdvance.size()) ? IndexXAdvance[(size_t)c] * scale : FallbackXAdvance;
        if (ImCharIsSpace(c))
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

ImVec2 ImFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** remaining) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin); // FIXME-OPT: Need to avoid this.

    const float scale = size / FontSize;
    const float line_height = FontSize * scale;

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
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - line_width);
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
                    if (ImCharIsSpace(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source (handle unlikely UTF-8 decoding failure by skipping to the next byte)
        const char* prev_s = s;
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        }
        else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0)
                break;
        }
        
        if (c == '\n')
        {
            text_size.x = ImMax(text_size.x, line_width);
            text_size.y += line_height;
            line_width = 0.0f;
            continue;
        }
        
        const float char_width = ((size_t)c < IndexXAdvance.size()) ? IndexXAdvance[(size_t)c] * scale : FallbackXAdvance;
        if (line_width + char_width >= max_width)
        {
            s = prev_s;
            break;
        }

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

ImVec2 ImFont::CalcTextSizeW(float size, float max_width, const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining) const
{
    if (!text_end)
        text_end = text_begin + ImStrlenW(text_begin);

    const float scale = size / FontSize;
    const float line_height = FontSize * scale;

    ImVec2 text_size = ImVec2(0,0);
    float line_width = 0.0f;

    const ImWchar* s = text_begin;
    while (s < text_end)
    {
        const unsigned int c = (unsigned int)(*s++);

        if (c == '\n')
        {
            text_size.x = ImMax(text_size.x, line_width);
            text_size.y += line_height;
            line_width = 0.0f;
            continue;
        }
        
        const float char_width = ((size_t)c < IndexXAdvance.size()) ? IndexXAdvance[(size_t)c] * scale : FallbackXAdvance;
        if (line_width + char_width >= max_width)
        {
            s--;
            break;
        }

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

void ImFont::RenderText(float size, ImVec2 pos, ImU32 col, const ImVec4& clip_rect_ref, const char* text_begin, const char* text_end, ImDrawVert*& out_vertices, float wrap_width, const ImVec2* cpu_clip_max) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin);

    const float scale = size / FontSize;
    const float line_height = FontSize * scale;

    // Align to be pixel perfect
    pos.x = (float)(int)pos.x + DisplayOffset.x;
    pos.y = (float)(int)pos.y + DisplayOffset.y;

    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    ImVec4 clip_rect = clip_rect_ref;
    if (cpu_clip_max)
    {
        clip_rect.z = ImMin(clip_rect.z, cpu_clip_max->x);
        clip_rect.w = ImMin(clip_rect.w, cpu_clip_max->y);
    }
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
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - pos.x));
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                x = pos.x;
                y += line_height;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (ImCharIsSpace(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source (handle unlikely UTF-8 decoding failure by skipping to the next byte)
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        }
        else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0)
                break;
        }

        if (c == '\n')
        {
            x = pos.x;
            y += line_height;
            continue;
        }

        float char_width = 0.0f;
        if (const Glyph* glyph = FindGlyph((unsigned short)c))
        {
            char_width = glyph->XAdvance * scale;
            if (c != ' ' && c != '\t')
            {
                // Clipping on Y is more likely
                float y1 = (float)(y + glyph->YOffset * scale);
                float y2 = (float)(y1 + glyph->Height * scale);
                if (y1 <= clip_rect.w && y2 >= clip_rect.y)
                {
                    float x1 = (float)(x + glyph->XOffset * scale);
                    float x2 = (float)(x1 + glyph->Width * scale);
                    if (x1 <= clip_rect.z && x2 >= clip_rect.x)
                    {
                        // Render a character
                        float u1 = glyph->U0;
                        float v1 = glyph->V0;
                        float u2 = glyph->U1;
                        float v2 = glyph->V1;

                        // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quad and in the "max" direction (bottom-right)
                        if (cpu_clip_max)
                        {
                            if (x2 > cpu_clip_max->x)
                            {
                                const float clip_tx = (cpu_clip_max->x - x1) / (x2 - x1);
                                x2 = cpu_clip_max->x;
                                u2 = u1 + clip_tx * (u2 - u1);
                            }
                            if (y2 > cpu_clip_max->y)
                            {
                                const float clip_ty = (cpu_clip_max->y - y1) / (y2 - y1);
                                y2 = cpu_clip_max->y;
                                v2 = v1 + clip_ty * (v2 - v1);
                            }
                        }

                        out_vertices[0].pos = ImVec2(x1, y1);
                        out_vertices[0].uv  = ImVec2(u1, v1);
                        out_vertices[0].col = col;

                        out_vertices[1].pos = ImVec2(x2, y1);
                        out_vertices[1].uv  = ImVec2(u2, v1);
                        out_vertices[1].col = col;

                        out_vertices[2].pos = ImVec2(x2, y2);
                        out_vertices[2].uv  = ImVec2(u2, v2);
                        out_vertices[2].col = col;

                        out_vertices[3] = out_vertices[0];
                        out_vertices[4] = out_vertices[2];

                        out_vertices[5].pos = ImVec2(x1, y2);
                        out_vertices[5].uv  = ImVec2(u1, v2);
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

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Win32 API clipboard implementation
static const char* GetClipboardTextFn_DefaultImpl()
{
    static char* buf_local = NULL;
    if (buf_local)
    {
        ImGui::MemFree(buf_local);
        buf_local = NULL;
    }
    if (!OpenClipboard(NULL)) 
        return NULL;
    HANDLE wbuf_handle = GetClipboardData(CF_UNICODETEXT); 
    if (wbuf_handle == NULL)
        return NULL;
    if (ImWchar* wbuf_global = (ImWchar*)GlobalLock(wbuf_handle))
    {
        int buf_len = ImTextCountUtf8BytesFromStr(wbuf_global, NULL) + 1;
        buf_local = (char*)ImGui::MemAlloc(buf_len * sizeof(char));
        ImTextStrToUtf8(buf_local, buf_len, wbuf_global, NULL);
    }
    GlobalUnlock(wbuf_handle); 
    CloseClipboard(); 
    return buf_local;
}

// Win32 API clipboard implementation
static void SetClipboardTextFn_DefaultImpl(const char* text)
{
    if (!OpenClipboard(NULL))
        return;

    const int wbuf_length = ImTextCountCharsFromUtf8(text, NULL) + 1;
    HGLOBAL wbuf_handle = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wbuf_length * sizeof(ImWchar)); 
    if (wbuf_handle == NULL)
        return;
    ImWchar* wbuf_global = (ImWchar*)GlobalLock(wbuf_handle); 
    ImTextStrFromUtf8(wbuf_global, wbuf_length, text, NULL);
    GlobalUnlock(wbuf_handle); 
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, wbuf_handle);
    CloseClipboard();
}

#else

// Local ImGui-only clipboard implementation, if user hasn't defined better clipboard handlers
static const char* GetClipboardTextFn_DefaultImpl()
{
    return GImGui->PrivateClipboard;
}

// Local ImGui-only clipboard implementation, if user hasn't defined better clipboard handlers
static void SetClipboardTextFn_DefaultImpl(const char* text)
{
    ImGuiState& g = *GImGui;
    if (g.PrivateClipboard)
    {
        ImGui::MemFree(g.PrivateClipboard);
        g.PrivateClipboard = NULL;
    }
    const char* text_end = text + strlen(text);
    g.PrivateClipboard = (char*)ImGui::MemAlloc((size_t)(text_end - text) + 1);
    memcpy(g.PrivateClipboard, text, (size_t)(text_end - text));
    g.PrivateClipboard[(size_t)(text_end - text)] = 0;
}

#endif

#if defined(_MSC_VER) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS)

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <Imm.h>
#pragma comment(lib, "imm32")

static void ImeSetInputScreenPosFn_DefaultImpl(int x, int y)
{
    // Notify OS Input Method Editor of text input position
    if (HWND hwnd = (HWND)GImGui->IO.ImeWindowHandle)
        if (HIMC himc = ImmGetContext(hwnd))
        {
            COMPOSITIONFORM cf;
            cf.ptCurrentPos.x = x;
            cf.ptCurrentPos.y = y;
            cf.dwStyle = CFS_FORCE_POSITION;
            ImmSetCompositionWindow(himc, &cf);
        }
}

#else

static void ImeSetInputScreenPosFn_DefaultImpl(int, int)
{
}

#endif

#ifdef IMGUI_DISABLE_TEST_WINDOWS

void ImGui::ShowUserGuide() {}
void ImGui::ShowStyleEditor(ImGuiStyle*) {}
void ImGui::ShowTestWindow(bool*) {}

#else

//-----------------------------------------------------------------------------
// HELP
//-----------------------------------------------------------------------------

void ImGui::ShowUserGuide()
{
    ImGuiState& g = *GImGui;

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
    ImGuiState& g = *GImGui;
    ImGuiStyle& style = g.Style;

    const ImGuiStyle def; // Default style

    if (ImGui::Button("Revert Style"))
        g.Style = ref ? *ref : def;
    if (ref)
    {
        ImGui::SameLine();
        if (ImGui::Button("Save Style"))
            *ref = g.Style;
    }

    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.55f);

    if (ImGui::TreeNode("Sizes"))
    {
        ImGui::SliderFloat("Alpha", &style.Alpha, 0.20f, 1.0f, "%.2f");                 // Not exposing zero here so user doesn't "lose" the UI. But application code could have a toggle to switch between zero and non-zero.
        ImGui::SliderFloat2("WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 16.0f, "%.0f");
        ImGui::SliderFloat("ChildWindowRounding", &style.ChildWindowRounding, 0.0f, 16.0f, "%.0f");
        ImGui::SliderFloat2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 16.0f, "%.0f");
        ImGui::SliderFloat2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat2("ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat2("TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
        ImGui::SliderFloat("IndentSpacing", &style.IndentSpacing, 0.0f, 20.0f, "%.0f");
        ImGui::SliderFloat("ScrollBarWidth", &style.ScrollbarWidth, 1.0f, 20.0f, "%.0f");
        ImGui::SliderFloat("GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Colors"))
    {
        static int output_dest = 0;
        static bool output_only_modified = false;
        if (ImGui::Button("Output Colors"))
        {
            if (output_dest == 0)
                ImGui::LogToClipboard();
            else
                ImGui::LogToTTY();
            ImGui::LogText("ImGuiStyle& style = ImGui::GetStyle();" STR_NEWLINE);
            for (int i = 0; i < ImGuiCol_COUNT; i++)
            {
                const ImVec4& col = style.Colors[i];
                const char* name = ImGui::GetStyleColName(i);
                if (!output_only_modified || memcmp(&col, (ref ? &ref->Colors[i] : &def.Colors[i]), sizeof(ImVec4)) != 0)
                    ImGui::LogText("style.Colors[ImGuiCol_%s]%*s= ImVec4(%.2ff, %.2ff, %.2ff, %.2ff);" STR_NEWLINE, name, 22 - strlen(name), "", col.x, col.y, col.z, col.w);
            }
            ImGui::LogFinish();
        }
        ImGui::SameLine(); ImGui::PushItemWidth(150); ImGui::Combo("##output_type", &output_dest, "To Clipboard\0To TTY"); ImGui::PopItemWidth();
        ImGui::SameLine(); ImGui::Checkbox("Only Modified Fields", &output_only_modified);

        static ImGuiColorEditMode edit_mode = ImGuiColorEditMode_RGB;
        ImGui::RadioButton("RGB", &edit_mode, ImGuiColorEditMode_RGB);
        ImGui::SameLine();
        ImGui::RadioButton("HSV", &edit_mode, ImGuiColorEditMode_HSV);
        ImGui::SameLine();
        ImGui::RadioButton("HEX", &edit_mode, ImGuiColorEditMode_HEX);
        //ImGui::Text("Tip: Click on colored square to change edit mode.");

        static ImGuiTextFilter filter;
        filter.Draw("Filter colors", 200);

        ImGui::BeginChild("#colors", ImVec2(0, 300), true);

        ImGui::ColorEditMode(edit_mode);
        for (int i = 0; i < ImGuiCol_COUNT; i++)
        {
            const char* name = ImGui::GetStyleColName(i);
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

static void ShowExampleAppConsole(bool* opened);
static void ShowExampleAppLongText(bool* opened);
static void ShowExampleAppAutoResize(bool* opened);
static void ShowExampleAppFixedOverlay(bool* opened);
static void ShowExampleAppManipulatingWindowTitle(bool* opened);
static void ShowExampleAppCustomRendering(bool* opened);

// Demonstrate ImGui features (unfortunately this makes this function a little bloated!)
void ImGui::ShowTestWindow(bool* opened)
{
    // Examples apps
    static bool show_app_console = false;
    static bool show_app_long_text = false;
    static bool show_app_auto_resize = false;
    static bool show_app_fixed_overlay = false;
    static bool show_app_custom_rendering = false;
    static bool show_app_manipulating_window_title = false;
    if (show_app_console)
        ShowExampleAppConsole(&show_app_console);
    if (show_app_long_text)
        ShowExampleAppLongText(&show_app_long_text);
    if (show_app_auto_resize)
        ShowExampleAppAutoResize(&show_app_auto_resize);
    if (show_app_fixed_overlay)
        ShowExampleAppFixedOverlay(&show_app_fixed_overlay);
    if (show_app_manipulating_window_title)
        ShowExampleAppManipulatingWindowTitle(&show_app_manipulating_window_title);
    if (show_app_custom_rendering)
        ShowExampleAppCustomRendering(&show_app_custom_rendering);

    static bool no_titlebar = false;
    static bool no_border = true;
    static bool no_resize = false;
    static bool no_move = false;
    static bool no_scrollbar = false;
    static bool no_collapse = false;
    static float bg_alpha = 0.65f;

    // Demonstrate the various window flags. Typically you would just use the default.
    ImGuiWindowFlags window_flags = 0;
    if (no_titlebar)  window_flags |= ImGuiWindowFlags_NoTitleBar;
    if (!no_border)   window_flags |= ImGuiWindowFlags_ShowBorders;
    if (no_resize)    window_flags |= ImGuiWindowFlags_NoResize;
    if (no_move)      window_flags |= ImGuiWindowFlags_NoMove;
    if (no_scrollbar) window_flags |= ImGuiWindowFlags_NoScrollbar;
    if (no_collapse)  window_flags |= ImGuiWindowFlags_NoCollapse;
    if (!ImGui::Begin("ImGui Test", opened, ImVec2(550,680), bg_alpha, window_flags))
    {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }
    
    //ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);    // 2/3 of the space for widget and 1/3 for labels
    ImGui::PushItemWidth(-140);                                 // Right align, keep 140 pixels for labels

    ImGui::Text("ImGui says hello.");
    //ImGui::Text("MousePos (%g, %g)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
    //ImGui::Text("MouseWheel %d", ImGui::GetIO().MouseWheel);
    //ImGui::Text("KeyMods %s%s%s", ImGui::GetIO().KeyCtrl ? "CTRL" : "", ImGui::GetIO().KeyShift ? "SHIFT" : "", ImGui::GetIO().KeyAlt? "ALT" : "");
    //ImGui::Text("WantCaptureMouse: %d", ImGui::GetIO().WantCaptureMouse);
    //ImGui::Text("WantCaptureKeyboard: %d", ImGui::GetIO().WantCaptureKeyboard);

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
        ImGui::Checkbox("no scrollbar", &no_scrollbar); ImGui::SameLine(300);
        ImGui::Checkbox("no collapse", &no_collapse);
        ImGui::SliderFloat("bg alpha", &bg_alpha, 0.0f, 1.0f);

        if (ImGui::TreeNode("Style"))
        {
            ImGui::ShowStyleEditor();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Fonts"))
        {
            ImGui::TextWrapped("Tip: Load fonts with GetIO().Fonts->AddFontFromFileTTF().");
            for (size_t i = 0; i < ImGui::GetIO().Fonts->Fonts.size(); i++)
            {
                ImFont* font = ImGui::GetIO().Fonts->Fonts[i];
                ImGui::BulletText("Font %d: %.2f pixels, %d glyphs", i, font->FontSize, font->Glyphs.size());
                ImGui::TreePush((void*)i);
                ImGui::PushFont(font);
                ImGui::Text("The quick brown fox jumps over the lazy dog");
                ImGui::PopFont();
                if (i > 0 && ImGui::Button("Set as default"))
                {
                    ImGui::GetIO().Fonts->Fonts[i] = ImGui::GetIO().Fonts->Fonts[0];
                    ImGui::GetIO().Fonts->Fonts[0] = font;
                }
                ImGui::SliderFloat("font scale", &font->Scale, 0.3f, 2.0f, "%.1f");             // scale only this font
                ImGui::TreePop();
            }
            static float window_scale = 1.0f;
            ImGui::SliderFloat("this window scale", &window_scale, 0.3f, 2.0f, "%.1f");                   // scale only this window
            ImGui::SliderFloat("global scale", &ImGui::GetIO().FontGlobalScale, 0.3f, 2.0f, "%.1f"); // scale everything
            ImGui::SetWindowFontScale(window_scale);
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
            ImGui::Bullet(); ImGui::Text("Bullet point 3 (two calls)");
            ImGui::Bullet(); ImGui::SmallButton("Button 1");
            ImGui::Bullet(); ImGui::SmallButton("Button 2");
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
            ImGui::TextWrapped("This text should automatically wrap on the edge of the window. The current implementation for text wrapping follows simple rules that works for English and possibly other languages.");
            ImGui::Spacing();

            static float wrap_width = 200.0f;
            ImGui::SliderFloat("Wrap width", &wrap_width, -20, 600, "%.0f");

            ImGui::Text("Test paragraph 1:");
            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetCursorScreenPos() + ImVec2(wrap_width, 0.0f), ImGui::GetCursorScreenPos() + ImVec2(wrap_width+10, ImGui::GetTextLineHeight()), 0xFFFF00FF);
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_width);
            ImGui::Text("lazy dog. This paragraph is made to fit within %.0f pixels. The quick brown fox jumps over the lazy dog.", wrap_width);
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0xFF00FFFF);
            ImGui::PopTextWrapPos();

            ImGui::Text("Test paragraph 2:");
            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetCursorScreenPos() + ImVec2(wrap_width, 0.0f), ImGui::GetCursorScreenPos() + ImVec2(wrap_width+10, ImGui::GetTextLineHeight()), 0xFFFF00FF);
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_width);
            ImGui::Text("aaaaaaaa bbbbbbbb, cccccccc,dddddddd. eeeeeeee   ffffffff. gggggggg!hhhhhhhh");
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0xFF00FFFF);
            ImGui::PopTextWrapPos();

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("UTF-8 Text"))
        {
            // UTF-8 test with Japanese characters
            // (needs a suitable font, try Arial Unicode or M+ fonts http://mplus-fonts.sourceforge.jp/mplus-outline-fonts/index-en.html)
            // Most compiler appears to support UTF-8 in source code (with Visual Studio you need to save your file as 'UTF-8 without signature')
            // However for the sake for maximum portability here we are *not* including raw UTF-8 character in this source file, instead we encode the string with hexadecimal constants.
            // In your own application be reasonable and use UTF-8 in source or retrieve the data from file system!
            // Note that characters values are preserved even if the font cannot be displayed, so you can safely copy & paste garbled characters into another application.
            ImGui::TextWrapped("CJK text will only appears if the font was loaded with the appropriate CJK character ranges. Call io.Font->LoadFromFileTTF() manually to load extra character ranges.");
            ImGui::Text("Hiragana: \xe3\x81\x8b\xe3\x81\x8d\xe3\x81\x8f\xe3\x81\x91\xe3\x81\x93 (kakikukeko)");
            ImGui::Text("Kanjis: \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e (nihongo)");
            static char buf[32] = "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e";
            ImGui::InputText("UTF-8 input", buf, IM_ARRAYSIZE(buf));
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Clipping"))
        {
            static ImVec2 size(80, 20);
            ImGui::TextWrapped("On a per-widget basis we are occasionally clipping text if it won't fit in its frame.");
            ImGui::SliderFloat2("size", (float*)&size, 5.0f, 200.0f);
            ImGui::Button("Line 1 hello\nLine 2 clip me!", size);
            ImGui::TextWrapped("Otherwise we are doing coarser clipping + passing a scissor rectangle to the renderer. The system is designed to try minimizing both execution and rendering cost.");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Images"))
        {
            ImGui::TextWrapped("Below we are displaying the font texture (which is the only texture we have access to in this demo). Use the 'ImTextureID' type as storage to pass pointers or identifier to your own texture data. Hover the texture for a zoomed view!");
            ImVec2 tex_screen_pos = ImGui::GetCursorScreenPos();
            float tex_w = (float)ImGui::GetIO().Fonts->TexWidth;
            float tex_h = (float)ImGui::GetIO().Fonts->TexHeight;
            ImTextureID tex_id = ImGui::GetIO().Fonts->TexID;
            ImGui::Image(tex_id, ImVec2(tex_w, tex_h), ImVec2(0,0), ImVec2(1,1), ImColor(255,255,255,255), ImColor(255,255,255,128));
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                float focus_sz = 32.0f;
                float focus_x = ImClamp(ImGui::GetMousePos().x - tex_screen_pos.x - focus_sz * 0.5f, 0.0f, tex_w - focus_sz);
                float focus_y = ImClamp(ImGui::GetMousePos().y - tex_screen_pos.y - focus_sz * 0.5f, 0.0f, tex_h - focus_sz);
                ImGui::Text("Min: (%.2f, %.2f)", focus_x, focus_y);
                ImGui::Text("Max: (%.2f, %.2f)", focus_x + focus_sz, focus_y + focus_sz);
                ImVec2 uv0 = ImVec2((focus_x) / tex_w, (focus_y) / tex_h);
                ImVec2 uv1 = ImVec2((focus_x + focus_sz) / tex_w, (focus_y + focus_sz) / tex_h);
                ImGui::Image(tex_id, ImVec2(128,128), uv0, uv1, ImColor(255,255,255,255), ImColor(255,255,255,128));
                ImGui::EndTooltip();
            }
            ImGui::TextWrapped("And now some textured buttons..");
            static int pressed_count = 0;
            for (int i = 0; i < 8; i++)
            {
                if (i > 0)
                    ImGui::SameLine();
                ImGui::PushID(i);
                int frame_padding = -1 + i;     // -1 padding uses default padding
                if (ImGui::ImageButton(tex_id, ImVec2(32,32), ImVec2(0,0), ImVec2(32.0f/tex_w,32/tex_h), frame_padding))
                    pressed_count += 1;
                ImGui::PopID();
            }
            ImGui::Text("Pressed %d times.", pressed_count);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Selectables"))
        {
            if (ImGui::TreeNode("Basic"))
            {
                static bool selected[3] = { false, true, false };
                ImGui::Selectable("1. I am selectable", &selected[0]);
                ImGui::Selectable("2. I am selectable", &selected[1]);
                ImGui::Text("3. I am not selectable");
                ImGui::Selectable("4. I am selectable", &selected[2]);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Rendering more text into the same block"))
            {
                static bool selected[3] = { false, false, false };
                ImGui::Selectable("main.c", &selected[0]);    ImGui::SameLine(300); ImGui::Text(" 2,345 bytes");
                ImGui::Selectable("Hello.cpp", &selected[1]); ImGui::SameLine(300); ImGui::Text("12,345 bytes");
                ImGui::Selectable("Hello.h", &selected[2]);   ImGui::SameLine(300); ImGui::Text(" 2,345 bytes");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Grid"))
            {
                static bool selected[16] = { true, false, false, false, false, true, false, false, false, false, true, false, false, false, false, true };
                for (int i = 0; i < 16; i++)
                {
                    ImGui::PushID(i);
                    if (ImGui::Selectable("Me", &selected[i], ImVec2(50,50)))
                    {
                        int x = i % 4, y = i / 4;
                        if (x > 0) selected[i - 1] ^= 1;
                        if (x < 3) selected[i + 1] ^= 1;
                        if (y > 0) selected[i - 4] ^= 1;
                        if (y < 3) selected[i + 4] ^= 1;
                    }
                    if ((i % 4) < 3) ImGui::SameLine();
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Popup"))
        {
            static bool popup_open = false;
            static int selected_fish = -1;
            const char* fishes[] = { "Bream", "Haddock", "Mackerel", "Pollock", "Tilefish" };
            if (ImGui::Button("Select.."))
                popup_open = true;
            ImGui::SameLine();
            ImGui::Text(selected_fish == -1 ? "<None>" : fishes[selected_fish]);
            if (popup_open)
            {
                ImGui::BeginPopup(&popup_open);
                ImGui::Text("Aquarium");
                ImGui::Separator();
                for (int i = 0; i < IM_ARRAYSIZE(fishes); i++)
                {
                    if (ImGui::Selectable(fishes[i], false))
                    {
                        selected_fish = i;
                        popup_open = false;
                    }
                }
                ImGui::EndPopup();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Filtered Text Input"))
        {
            static char buf1[64] = ""; ImGui::InputText("default", buf1, 64);
            static char buf2[64] = ""; ImGui::InputText("decimal", buf2, 64, ImGuiInputTextFlags_CharsDecimal);
            static char buf3[64] = ""; ImGui::InputText("hexadecimal", buf3, 64, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
            static char buf4[64] = ""; ImGui::InputText("uppercase", buf4, 64, ImGuiInputTextFlags_CharsUppercase);
            static char buf5[64] = ""; ImGui::InputText("no blank", buf5, 64, ImGuiInputTextFlags_CharsNoBlank);
            struct TextFilters { static int FilterImGuiLetters(ImGuiTextEditCallbackData* data) { if (data->EventChar < 256 && strchr("imgui", (char)data->EventChar)) return 0; return 1; } };
            static char buf6[64] = ""; ImGui::InputText("\"imgui\" letters", buf6, 64, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterImGuiLetters);
            ImGui::TreePop();
        }

        static bool check = true;
        ImGui::Checkbox("checkbox", &check);

        static int e = 0;
        ImGui::RadioButton("radio a", &e, 0); ImGui::SameLine();
        ImGui::RadioButton("radio b", &e, 1); ImGui::SameLine();
        ImGui::RadioButton("radio c", &e, 2);

        // Color buttons, demonstrate using PushID() to add unique identifier in the ID stack, and changing style.
        for (int i = 0; i < 7; i++)
        {
            if (i > 0) ImGui::SameLine();
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(i/7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(i/7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(i/7.0f, 0.8f, 0.8f));
            ImGui::Button("Click");
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }

        ImGui::Text("Hover over me");
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

        // Testing IMGUI_ONCE_UPON_A_FRAME macro
        //for (int i = 0; i < 5; i++)
        //{
        //  IMGUI_ONCE_UPON_A_FRAME
        //  {
        //      ImGui::Text("This will be displayed only once.");
        //  }
        //}

        ImGui::Separator();

        ImGui::LabelText("label", "Value");

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

        static float vec4a[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
        //ImGui::InputFloat2("input float2", vec4a);
        ImGui::InputFloat3("input float3", vec4a);
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

        static float col1[3] = { 1.0f,0.0f,0.2f };
        static float col2[4] = { 0.4f,0.7f,0.0f,0.5f };
        ImGui::ColorEdit3("color 1", col1);
        ImGui::ColorEdit4("color 2", col2);

        const char* listbox_items[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange", "Pineapple", "Strawberry", "Watermelon" };
        static int listbox_item_current = 1;
        ImGui::ListBox("listbox\n(single select)", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items), 4);

        //static int listbox_item_current2 = 2;
        //ImGui::PushItemWidth(-1);
        //ImGui::ListBox("##listbox2", &listbox_item_current2, listbox_items, IM_ARRAYSIZE(listbox_items), 4);
        //ImGui::PopItemWidth();

        if (ImGui::TreeNode("Multi-component Widgets"))
        {
            ImGui::Unindent();

            static float vec4f[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
            static int vec4i[4] = { 1, 5, 100, 255 };

            ImGui::InputFloat2("input float2", vec4f);
            ImGui::SliderFloat2("slider float2", vec4f, 0.0f, 1.0f);
            ImGui::InputInt2("input int2", vec4i);
            ImGui::SliderInt2("slider int2", vec4i, 0, 255);

            ImGui::InputFloat3("input float3", vec4f);
            ImGui::SliderFloat3("slider float3", vec4f, 0.0f, 1.0f);
            ImGui::InputInt3("input int3", vec4i);
            ImGui::SliderInt3("slider int3", vec4i, 0, 255);

            ImGui::InputFloat4("input float4", vec4f);
            ImGui::SliderFloat4("slider float4", vec4f, 0.0f, 1.0f);
            ImGui::InputInt4("input int4", vec4i);
            ImGui::SliderInt4("slider int4", vec4i, 0, 255);

            ImGui::Indent();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Vertical Sliders"))
        {
            ImGui::Unindent();
            const float spacing = 4;
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

            static int int_value = 0;
            ImGui::VSliderInt("##int", ImVec2(18,160), &int_value, 0, 5);
            ImGui::SameLine();

            static float values[7] = { 0.0f, 0.60f, 0.35f, 0.9f, 0.70f, 0.20f, 0.0f };
            ImGui::PushID("set1");
            for (int i = 0; i < 7; i++)
            {
                if (i > 0) ImGui::SameLine();
                ImGui::PushID(i);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor::HSV(i/7.0f, 0.5f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor::HSV(i/7.0f, 0.9f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImColor::HSV(i/7.0f, 1.0f, 1.0f));
                ImGui::VSliderFloat("##v", ImVec2(18,160), &values[i], 0.0f, 1.0f, "");
                if (ImGui::IsItemActive() || ImGui::IsItemHovered())
                    ImGui::SetTooltip("%.3f", values[i]);
                ImGui::PopStyleColor(3);
                ImGui::PopID();
            }
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::PushID("set2");
            static float values2[4] = { 0.20f, 0.80f, 0.40f, 0.25f };
            const int rows = 3;
            const ImVec2 small_slider_size(18, (160.0f-(rows-1)*spacing)/rows);
            for (int nx = 0; nx < 4; nx++)
            {
                if (nx > 0) ImGui::SameLine();
                ImGui::BeginGroup();
                for (int ny = 0; ny < rows; ny++)
                {
                    ImGui::PushID(nx*rows+ny);
                    ImGui::VSliderFloat("##v", small_slider_size, &values2[nx], 0.0f, 1.0f, "");
                    if (ImGui::IsItemActive() || ImGui::IsItemHovered())
                        ImGui::SetTooltip("%.3f", values2[nx]);
                    ImGui::PopID();
                }
                ImGui::EndGroup();
            }
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::PushID("set3");
            for (int i = 0; i < 4; i++)
            {
                if (i > 0) ImGui::SameLine();
                ImGui::PushID(i);
                ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 40);
                ImGui::VSliderFloat("##v", ImVec2(40,160), &values[i], 0.0f, 1.0f, "%.2f");
                ImGui::PopStyleVar();
                ImGui::PopID();
            }
            ImGui::PopID();

            ImGui::Indent();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Dragging"))
        {
            // You can use ImGui::GetItemActiveDragDelta() to query for the dragged amount on any widget.
            static ImVec2 value_raw(0.0f, 0.0f);
            static ImVec2 value_with_lock_threshold(0.0f, 0.0f);
            ImGui::Button("Drag Me");
            if (ImGui::IsItemActive())
            {
                value_raw = ImGui::GetMouseDragDelta(0, 0.0f);
                value_with_lock_threshold = ImGui::GetMouseDragDelta(0);
                //ImGui::SetTooltip("Delta: %.1f, %.1f", value.x, value.y);

                // Draw a line between the button and the mouse cursor
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                draw_list->PushClipRectFullScreen();
                draw_list->AddLine(ImGui::CalcItemRectClosestPoint(ImGui::GetIO().MousePos, true, -2.0f), ImGui::GetIO().MousePos, ImColor(ImGui::GetStyle().Colors[ImGuiCol_Button]), 4.0f);
                draw_list->PopClipRect();
            }
            ImGui::SameLine(); ImGui::Text("Raw (%.1f, %.1f), WithLockThresold (%.1f, %.1f)", value_raw.x, value_raw.y, value_with_lock_threshold.x, value_with_lock_threshold.y);
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("Graphs widgets"))
    {
        static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
        ImGui::PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr));

        static bool pause;
        static ImVector<float> values; if (values.empty()) { values.resize(90); memset(&values.front(), 0, values.size()*sizeof(float)); } 
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
        ImGui::PlotLines("##Graph", &values.front(), (int)values.size(), (int)values_offset, "avg 0.0", -1.0f, 1.0f, ImVec2(0,80));
        ImGui::SameLine(0, (int)ImGui::GetStyle().ItemInnerSpacing.x); 
        ImGui::BeginGroup();
        ImGui::Text("Graph");
        ImGui::Checkbox("pause", &pause);
        ImGui::EndGroup();
        ImGui::PlotHistogram("Histogram", arr, IM_ARRAYSIZE(arr), 0, NULL, 0.0f, 1.0f, ImVec2(0,80));
    }

    if (ImGui::CollapsingHeader("Layout"))
    {
        if (ImGui::TreeNode("Widgets Alignment"))
        {
            static float f = 0.0f;
            ImGui::Text("Fixed: 100 pixels");
            ImGui::PushItemWidth(100);
            ImGui::InputFloat("float##1", &f);
            ImGui::PopItemWidth();

            ImGui::Text("Proportional: 50%% of window width");
            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
            ImGui::InputFloat("float##2", &f);
            ImGui::PopItemWidth();

            ImGui::Text("Right-aligned: Leave 100 pixels for label");
            ImGui::PushItemWidth(-100);
            ImGui::InputFloat("float##3", &f);
            ImGui::PopItemWidth();

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Basic Horizontal Layout"))
        {
            ImGui::TextWrapped("(Use ImGui::SameLine() to keep adding items to the right of the preceeding item)");

            // Text
            ImGui::Text("Two items: Hello");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1,1,0,1), "World");

            // Adjust spacing
            ImGui::Text("More spacing: Hello");
            ImGui::SameLine(0, 20);
            ImGui::TextColored(ImVec4(1,1,0,1), "World");

            // Button
            ImGui::AlignFirstTextHeightToWidgets();
            ImGui::Text("Normal buttons"); ImGui::SameLine();
            ImGui::Button("Banana"); ImGui::SameLine();
            ImGui::Button("Apple"); ImGui::SameLine();
            ImGui::Button("Corniflower");

            // Button
            ImGui::Text("Small buttons"); ImGui::SameLine();
            ImGui::SmallButton("Like this one"); ImGui::SameLine();
            ImGui::Text("can fit within a text block.");

            // Aligned to arbitrary position. Easy/cheap column.
            ImGui::Text("Aligned"); 
            ImGui::SameLine(150); ImGui::Text("x=150");
            ImGui::SameLine(300); ImGui::Text("x=300");
            ImGui::Text("Aligned");
            ImGui::SameLine(150); ImGui::SmallButton("x=150");
            ImGui::SameLine(300); ImGui::SmallButton("x=300");

            // Checkbox
            static bool c1=false,c2=false,c3=false,c4=false;
            ImGui::Checkbox("My", &c1); ImGui::SameLine();
            ImGui::Checkbox("Tailor", &c2); ImGui::SameLine();
            ImGui::Checkbox("Is", &c3); ImGui::SameLine();
            ImGui::Checkbox("Rich", &c4);

            // Various
            static float f0=1.0f, f1=2.0f, f2=3.0f;
            ImGui::PushItemWidth(80);
            const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD" };
            static int item = -1;
            ImGui::Combo("Combo", &item, items, IM_ARRAYSIZE(items)); ImGui::SameLine();
            ImGui::SliderFloat("X", &f0, 0.0f,5.0f); ImGui::SameLine();
            ImGui::SliderFloat("Y", &f1, 0.0f,5.0f); ImGui::SameLine();
            ImGui::SliderFloat("Z", &f2, 0.0f,5.0f); 
            ImGui::PopItemWidth();

            ImGui::PushItemWidth(80);
            ImGui::Text("Lists:");
            static int selection[4] = { 0, 1, 2, 3 };
            for (int i = 0; i < 4; i++)
            {
                if (i > 0) ImGui::SameLine();
                ImGui::PushID(i);
                ImGui::ListBox("", &selection[i], items, IM_ARRAYSIZE(items));
                ImGui::PopID();
                //if (ImGui::IsItemHovered()) ImGui::SetTooltip("ListBox %d hovered", i); 
            }
            ImGui::PopItemWidth();

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Groups"))
        {
            ImGui::TextWrapped("(Using ImGui::BeginGroup()/EndGroup() to layout items)");

            ImVec2 size;
            ImGui::BeginGroup();
            {
                ImGui::BeginGroup();
                ImGui::Button("AAA");
                ImGui::SameLine();
                ImGui::Button("BBB");
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Button("CCC");
                ImGui::Button("DDD");
                ImGui::EndGroup();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Group hovered");
                ImGui::SameLine();
                ImGui::Button("EEE");
                ImGui::EndGroup();

                // Capture the group size and create widgets using the same size
                size = ImGui::GetItemRectSize();
                const float values[5] = { 0.5f, 0.20f, 0.80f, 0.60f, 0.25f };
                ImGui::PlotHistogram("##values", values, IM_ARRAYSIZE(values), 0, NULL, 0.0f, 1.0f, size);
            }
            ImGui::Button("ACTION", ImVec2((size.x - ImGui::GetStyle().ItemSpacing.x)*0.5f,size.y));
            ImGui::SameLine();
            ImGui::Button("REACTION", ImVec2((size.x - ImGui::GetStyle().ItemSpacing.x)*0.5f,size.y));
            ImGui::EndGroup();
            ImGui::SameLine();

            ImGui::Button("LEVERAGE\nBUZZWORD", size);
            ImGui::SameLine();

            ImGui::ListBoxHeader("List", size);
            ImGui::Selectable("Selected", true);
            ImGui::Selectable("Not Selected", false);
            ImGui::ListBoxFooter();

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Text Baseline Alignment"))
        {
            ImGui::TextWrapped("(This is testing the vertical alignment that occurs on text to keep it at the same baseline as widgets. Lines only composed of text or \"small\" widgets fit in less vertical spaces than lines with normal widgets)"); 

            ImGui::Text("One\nTwo\nThree"); ImGui::SameLine();            
            ImGui::Text("Hello\nWorld"); ImGui::SameLine();
            ImGui::Text("Banana");

            ImGui::Text("Banana"); ImGui::SameLine();
            ImGui::Text("Hello\nWorld"); ImGui::SameLine();
            ImGui::Text("One\nTwo\nThree");

            ImGui::Button("HOP"); ImGui::SameLine();
            ImGui::Text("Banana"); ImGui::SameLine();
            ImGui::Text("Hello\nWorld"); ImGui::SameLine();
            ImGui::Text("Banana");

            ImGui::Button("HOP"); ImGui::SameLine();
            ImGui::Text("Hello\nWorld"); ImGui::SameLine();
            ImGui::Text("Banana");

            ImGui::Button("TEST"); ImGui::SameLine();
            ImGui::Text("TEST"); ImGui::SameLine();
            ImGui::SmallButton("TEST");

            ImGui::AlignFirstTextHeightToWidgets(); // If your line starts with text, call this to align it to upcoming widgets.
            ImGui::Text("Text aligned to Widget"); ImGui::SameLine();
            ImGui::Button("Widget"); ImGui::SameLine();
            ImGui::Text("Widget"); ImGui::SameLine();
            ImGui::SmallButton("Widget");

            ImGui::TreePop();
        }
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
        ImGui::BeginChild("Sub1", ImVec2(ImGui::GetWindowWidth() * 0.5f,300));
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

        ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
        ImGui::BeginChild("Sub2", ImVec2(0,300), true);
        ImGui::Text("With border");
        ImGui::Columns(2);
        for (int i = 0; i < 100; i++)
        {
            if (i == 50)
                ImGui::NextColumn();
            char buf[32];
            ImFormatString(buf, IM_ARRAYSIZE(buf), "%08x", i*5731);
            ImGui::Button(buf);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    if (ImGui::CollapsingHeader("Columns"))
    {
        // Basic columns
        ImGui::Text("Basic:");
        ImGui::Columns(4, "mycolumns");
        ImGui::Separator();
        ImGui::Text("ID"); ImGui::NextColumn();
        ImGui::Text("Name"); ImGui::NextColumn();
        ImGui::Text("Path"); ImGui::NextColumn();
        ImGui::Text("Flags"); ImGui::NextColumn();
        ImGui::Separator();
        const char* names[3] = { "Robert", "Stephanie", "C64" };
        const char* paths[3] = { "/path/robert", "/path/stephanie", "/path/computer" };
        for (int i = 0; i < 3; i++)
        {
            ImGui::Text("%04d", i); ImGui::NextColumn();
            ImGui::Text(names[i]); ImGui::NextColumn();
            ImGui::Text(paths[i]); ImGui::NextColumn(); 
            ImGui::Text("...."); ImGui::NextColumn();
        }
        ImGui::Columns(1);

        ImGui::Separator();
        ImGui::Spacing();

        // Scrolling columns
		/*
        ImGui::Text("Scrolling:");
        ImGui::BeginChild("##header", ImVec2(0, ImGui::GetTextLineHeightWithSpacing()+ImGui::GetStyle().ItemSpacing.y));
        ImGui::Columns(3);
        ImGui::Text("ID"); ImGui::NextColumn();
        ImGui::Text("Name"); ImGui::NextColumn();
        ImGui::Text("Path"); ImGui::NextColumn();
        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::EndChild();
        ImGui::BeginChild("##scrollingregion", ImVec2(0, 60));
        ImGui::Columns(3);
        for (int i = 0; i < 10; i++)
        {
            ImGui::Text("%04d", i); ImGui::NextColumn();
            ImGui::Text("Foobar"); ImGui::NextColumn();
            ImGui::Text("/path/foobar/%04d/", i); ImGui::NextColumn();
        }
        ImGui::Columns(1);
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::Spacing();
		*/

        // Create multiple items in a same cell before switching to next column
        ImGui::Text("Mixed items:");
        ImGui::Columns(3, "mixed");
        ImGui::Separator();

        static int e = 0;
        ImGui::Text("Hello"); 
        ImGui::Button("Banana");
        ImGui::RadioButton("radio a", &e, 0); 
        ImGui::NextColumn();

        ImGui::Text("ImGui"); 
        ImGui::Button("Apple");
        ImGui::RadioButton("radio b", &e, 1);
        static float foo = 1.0f;
        ImGui::InputFloat("red", &foo, 0.05f, 0, 3); 
        ImGui::Text("An extra line here.");
        ImGui::NextColumn();
        
        ImGui::Text("World!");
        ImGui::Button("Corniflower");
        ImGui::RadioButton("radio c", &e, 2);
        static float bar = 1.0f;
        ImGui::InputFloat("blue", &bar, 0.05f, 0, 3); 
        ImGui::NextColumn();

        if (ImGui::CollapsingHeader("Category A")) ImGui::Text("Blah blah blah"); ImGui::NextColumn();
        if (ImGui::CollapsingHeader("Category B")) ImGui::Text("Blah blah blah"); ImGui::NextColumn();
        if (ImGui::CollapsingHeader("Category C")) ImGui::Text("Blah blah blah"); ImGui::NextColumn();
        ImGui::Columns(1);

        ImGui::Separator();
        ImGui::Spacing();

        // Tree items
        ImGui::Text("Tree items:");
        ImGui::Columns(2, "tree items");
        ImGui::Separator();
        if (ImGui::TreeNode("Hello")) { ImGui::BulletText("World"); ImGui::TreePop(); } ImGui::NextColumn();
        if (ImGui::TreeNode("Bonjour")) { ImGui::BulletText("Monde"); ImGui::TreePop(); } ImGui::NextColumn();
        ImGui::Columns(1);

        ImGui::Separator();
        ImGui::Spacing();

        // Word-wrapping
        ImGui::Text("Word-wrapping:");
        ImGui::Columns(2, "word-wrapping");
        ImGui::Separator();
        ImGui::TextWrapped("The quick brown fox jumps over the lazy dog.");
        ImGui::Text("Hello Left");
        ImGui::NextColumn();
        ImGui::TextWrapped("The quick brown fox jumps over the lazy dog.");
        ImGui::Text("Hello Right");
        ImGui::Columns(1);

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::TreeNode("Inside a tree.."))
        {
            if (ImGui::TreeNode("node 1 (with borders)"))
            {
                ImGui::Columns(4);
                for (int i = 0; i < 8; i++)
                {
                    ImGui::Text("%c%c%c", 'a'+i, 'a'+i, 'a'+i);
                    ImGui::NextColumn();
                }
                ImGui::Columns(1);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("node 2 (without borders)"))
            {
                ImGui::Columns(4, NULL, false);
                for (int i = 0; i < 8; i++)
                {
                    ImGui::Text("%c%c%c", 'a'+i, 'a'+i, 'a'+i);
                    ImGui::NextColumn();
                }
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

    if (ImGui::CollapsingHeader("Keyboard, Mouse & Focus"))
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
            if (ImGui::IsItemActive()) has_focus = 1;
            
            if (focus_2) ImGui::SetKeyboardFocusHere();
            ImGui::InputText("2", buf, IM_ARRAYSIZE(buf));
            if (ImGui::IsItemActive()) has_focus = 2;

            ImGui::PushAllowKeyboardFocus(false);
            if (focus_3) ImGui::SetKeyboardFocusHere();
            ImGui::InputText("3 (tab skip)", buf, IM_ARRAYSIZE(buf));
            if (ImGui::IsItemActive()) has_focus = 3;
            ImGui::PopAllowKeyboardFocus();
            if (has_focus)
                ImGui::Text("Item with focus: %d", has_focus);
            else 
                ImGui::Text("Item with focus: <none>");
            ImGui::TextWrapped("Cursor & selection are preserved when refocusing last used item in code.");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Mouse cursors"))
        {
            ImGui::TextWrapped("(Your application can render a different mouse cursor based on what ImGui::GetMouseCursor() returns. You can also set io.MouseDrawCursor to ask ImGui to render the cursor for you in software)");
            ImGui::Checkbox("io.MouseDrawCursor", &ImGui::GetIO().MouseDrawCursor);
            ImGui::Text("Hover to see mouse cursors:");
            for (int i = 0; i < ImGuiMouseCursor_Count_; i++)
            {
                char label[32];
                sprintf(label, "Mouse cursor %d", i);
                ImGui::Bullet(); ImGui::Selectable(label, false); 
                if (ImGui::IsItemHovered()) 
                    ImGui::SetMouseCursor(i);
            }
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("App Examples"))
    {
        ImGui::Checkbox("Console", &show_app_console);
        ImGui::Checkbox("Long text display", &show_app_long_text);
        ImGui::Checkbox("Auto-resizing window", &show_app_auto_resize);
        ImGui::Checkbox("Simple overlay", &show_app_fixed_overlay);
        ImGui::Checkbox("Manipulating window title", &show_app_manipulating_window_title);
        ImGui::Checkbox("Custom rendering", &show_app_custom_rendering);
    }

    ImGui::End();
}

static void ShowExampleAppAutoResize(bool* opened)
{
    if (!ImGui::Begin("Example: Auto-Resizing Window", opened, ImGuiWindowFlags_AlwaysAutoResize))
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

static void ShowExampleAppFixedOverlay(bool* opened)
{
    ImGui::SetNextWindowPos(ImVec2(10,10));
    if (!ImGui::Begin("Example: Fixed Overlay", opened, ImVec2(0,0), 0.3f, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Simple overlay\non the top-left side of the screen.");
    ImGui::Separator();
    ImGui::Text("Mouse Position: (%.1f,%.1f)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y); 

    ImGui::End();
}

static void ShowExampleAppManipulatingWindowTitle(bool* opened)
{
    (void)opened;

    // By default, Windows are uniquely identified by their title.
    // You can use the "##" and "###" markers to manipulate the display/ID. Read FAQ at the top of this file!

    // Using "##" to display same title but have unique identifier.
    ImGui::SetNextWindowPos(ImVec2(100,100), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Same title as another window##1");
    ImGui::Text("This is window 1.\nMy title is the same as window 2, but my identifier is unique.");
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(100,200), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Same title as another window##2");
    ImGui::Text("This is window 2.\nMy title is the same as window 1, but my identifier is unique.");
    ImGui::End();

    // Using "###" to display a changing title but keep a static identifier "AnimatedTitle"
    char buf[128];
    ImFormatString(buf, IM_ARRAYSIZE(buf), "Animated title %c %d##AnimatedTitle", "|/-\\"[(int)(ImGui::GetTime()/0.25f)&3], rand());
    ImGui::SetNextWindowPos(ImVec2(100,300), ImGuiSetCond_FirstUseEver);
    ImGui::Begin(buf);
    ImGui::Text("This window has a changing title.");
    ImGui::End();
}

static void ShowExampleAppCustomRendering(bool* opened)
{
    ImGui::SetNextWindowSize(ImVec2(300,350), ImGuiSetCond_FirstUseEver);
    if (!ImGui::Begin("Example: Custom Rendering", opened))
    {
        ImGui::End();
        return;
    }

    // Tip: If you do a lot of custom rendering, you probably want to use your own geometrical types and benefit of overloaded operators, etc.
    // Define IM_VEC2_CLASS_EXTRA in imconfig.h to create implicit conversions between your types and ImVec2/ImVec4.
    // ImGui defines overloaded operators but they are internal to imgui.cpp and not exposed outside (to avoid messing with your types)
    // In this example we aren't using the operators.

    static ImVector<ImVec2> points;
    static bool adding_line = false;
    if (ImGui::Button("Clear")) points.clear();
    if (points.size() >= 2) { ImGui::SameLine(); if (ImGui::Button("Undo")) { points.pop_back(); points.pop_back(); } }
    ImGui::Text("Left-click and drag to add lines");
    ImGui::Text("Right-click to undo");

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Here we are using InvisibleButton() as a convenience to 1) advance the cursor and 2) allows us to use IsItemHovered()
    // However you can draw directly and poll mouse/keyboard by yourself. You can manipulate the cursor using GetCursorPos() and SetCursorPos().
    // If you only use the ImDrawList API, you can notify the owner window of its extends by using SetCursorPos(max).
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
    ImVec2 canvas_size = ImVec2(ImMax(50.0f,ImGui::GetWindowContentRegionMax().x-ImGui::GetCursorPos().x), ImMax(50.0f,ImGui::GetWindowContentRegionMax().y-ImGui::GetCursorPos().y));    // Resize canvas what's available
    draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 0xFFFFFFFF);
    bool adding_preview = false;
    ImGui::InvisibleButton("canvas", canvas_size);
    if (ImGui::IsItemHovered())
    {
        ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - canvas_pos.x, ImGui::GetIO().MousePos.y - canvas_pos.y);
        if (!adding_line && ImGui::GetIO().MouseClicked[0])
        {
            points.push_back(mouse_pos_in_canvas);
            adding_line = true;
        }
        if (adding_line)
        {
            adding_preview = true;
            points.push_back(mouse_pos_in_canvas);
            if (!ImGui::GetIO().MouseDown[0])
                adding_line = adding_preview = false;
        }
        if (ImGui::GetIO().MouseClicked[1] && !points.empty())
        {
            adding_line = false;
            points.pop_back();
            points.pop_back();
        }
    }
    draw_list->PushClipRect(ImVec4(canvas_pos.x, canvas_pos.y, canvas_pos.x+canvas_size.x, canvas_pos.y+canvas_size.y));      // clip lines within the canvas (if we resize it, etc.)
    for (int i = 0; i < (int)points.size() - 1; i += 2)
        draw_list->AddLine(ImVec2(canvas_pos.x + points[i].x, canvas_pos.y + points[i].y), ImVec2(canvas_pos.x + points[i+1].x, canvas_pos.y + points[i+1].y), 0xFF00FFFF);
    draw_list->PopClipRect();
    if (adding_preview)
        points.pop_back();
    ImGui::End();
}

struct ExampleAppConsole
{
    char                  InputBuf[256];
    ImVector<char*>       Items;
    bool                  ScrollToBottom;
    ImVector<char*>       History;
    int                   HistoryPos;    // -1: new line, 0..History.size()-1 browsing history.
    ImVector<const char*> Commands;

    ExampleAppConsole()
    {
        ClearLog();
        HistoryPos = -1;
        Commands.push_back("HELP");
        Commands.push_back("HISTORY");
        Commands.push_back("CLEAR");
        Commands.push_back("CLASSIFY");  // "classify" is here to provide an example of "C"+[tab] completing to "CL" and displaying matches.
    }
    ~ExampleAppConsole()
    {
        ClearLog();
        for (size_t i = 0; i < Items.size(); i++) 
            ImGui::MemFree(History[i]); 
    }

    void    ClearLog()
    {
        for (size_t i = 0; i < Items.size(); i++) 
            ImGui::MemFree(Items[i]); 
        Items.clear();
        ScrollToBottom = true;
    }

    void    AddLog(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        ImFormatStringV(buf, IM_ARRAYSIZE(buf), fmt, args);
        va_end(args);
        Items.push_back(ImStrdup(buf));
        ScrollToBottom = true;
    }

    void    Run(const char* title, bool* opened)
    {
        ImGui::SetNextWindowSize(ImVec2(520,600), ImGuiSetCond_FirstUseEver);
        if (!ImGui::Begin(title, opened))
        {
            ImGui::End();
            return;
        }

        ImGui::TextWrapped("This example implements a console with basic coloring, completion and history. A more elaborate implementation may want to store entries along with extra data such as timestamp, emitter, etc.");
        ImGui::TextWrapped("Enter 'HELP' for help, press TAB to use text completion.");

        // TODO: display from bottom
        // TODO: clip manually

        if (ImGui::SmallButton("Add Dummy Text")) { AddLog("%d some text", Items.size()); AddLog("some more text"); AddLog("display very important message here!"); } ImGui::SameLine(); 
        if (ImGui::SmallButton("Add Dummy Error")) AddLog("[error] something went wrong"); ImGui::SameLine(); 
        if (ImGui::SmallButton("Clear")) ClearLog();
        //static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

        ImGui::Separator();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
        static ImGuiTextFilter filter;
        filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        //if (ImGui::IsItemHovered()) ImGui::SetKeyboardFocusHere(-1); // Auto focus on hover
        ImGui::PopStyleVar();
        ImGui::Separator();

        // Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
        // NB- if you have thousands of entries this approach may be too inefficient. You can seek and display only the lines that are visible - CalcListClipping() is a helper to compute this information.
        // If your items are of variable size you may want to implement code similar to what CalcListClipping() does. Or split your data into fixed height items to allow random-seeking into your list.
        ImGui::BeginChild("ScrollingRegion", ImVec2(0,-ImGui::GetTextLineHeightWithSpacing()*2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1)); // Tighten spacing
        for (size_t i = 0; i < Items.size(); i++)
        {
            const char* item = Items[i];
            if (!filter.PassFilter(item))
                continue;
            ImVec4 col(1,1,1,1); // A better implement may store a type per-item. For the sample let's just parse the text.
            if (strstr(item, "[error]")) col = ImVec4(1.0f,0.4f,0.4f,1.0f);
            else if (strncmp(item, "# ", 2) == 0) col = ImVec4(1.0f,0.8f,0.6f,1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::TextUnformatted(item);
            ImGui::PopStyleColor();
        }
        if (ScrollToBottom)
            ImGui::SetScrollPosHere();
        ScrollToBottom = false;
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        // Command-line
        if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCompletion|ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
        {
            char* input_end = InputBuf+strlen(InputBuf);
            while (input_end > InputBuf && input_end[-1] == ' ') input_end--; *input_end = 0;
            if (InputBuf[0])
                ExecCommand(InputBuf);
            strcpy(InputBuf, "");
        }

        // Demonstrate keeping auto focus on the input box
        if (ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
            ImGui::SetKeyboardFocusHere(-1); // Auto focus

        ImGui::End();
    }

    void    ExecCommand(const char* command_line)
    {
        AddLog("# %s\n", command_line);

        // Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
        HistoryPos = -1;
        for (int i = (int)History.size()-1; i >= 0; i--)
            if (ImStricmp(History[i], command_line) == 0)
            {
                ImGui::MemFree(History[i]);
                History.erase(History.begin() + i);
                break;
            }
        History.push_back(ImStrdup(command_line));

        // Process command
        if (ImStricmp(command_line, "CLEAR") == 0)
        {
            ClearLog();
        }
        else if (ImStricmp(command_line, "HELP") == 0)
        {
            AddLog("Commands:");
            for (size_t i = 0; i < Commands.size(); i++)
                AddLog("- %s", Commands[i]);
        }
        else if (ImStricmp(command_line, "HISTORY") == 0)
        {
            for (size_t i = History.size() >= 10 ? History.size() - 10 : 0; i < History.size(); i++)
                AddLog("%3d: %s\n", i, History[i]);
        }
        else
        {
            AddLog("Unknown command: '%s'\n", command_line);
        }
    }

    static int TextEditCallbackStub(ImGuiTextEditCallbackData* data)
    {
        ExampleAppConsole* console = (ExampleAppConsole*)data->UserData;
        return console->TextEditCallback(data);
    }

    int     TextEditCallback(ImGuiTextEditCallbackData* data)
    {
        //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
        switch (data->EventFlag)
        {
        case ImGuiInputTextFlags_CallbackCompletion:
            {
                // Example of TEXT COMPLETION

                // Locate beginning of current word
                const char* word_end = data->Buf + data->CursorPos;
                const char* word_start = word_end;
                while (word_start > data->Buf)
                {
                    const char c = word_start[-1];
                    if (ImCharIsSpace(c) || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                // Build a list of candidates
                ImVector<const char*> candidates;
                for (size_t i = 0; i < Commands.size(); i++)
                    if (ImStrnicmp(Commands[i], word_start, (int)(word_end-word_start)) == 0)
                        candidates.push_back(Commands[i]);

                if (candidates.size() == 0)
                {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", word_end-word_start, word_start);
                }
                else if (candidates.size() == 1)
                {
                    // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
                    data->DeleteChars((int)(word_start-data->Buf), (int)(word_end-word_start));
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                }
                else
                {
                    // Multiple matches. Complete as much as we can, so inputing "C" will complete to "CL" and display "CLEAR" and "CLASSIFY"
                    int match_len = (int)(word_end - word_start);
                    for (;;)
                    {
                        int c = 0;
                        bool all_candidates_matches = true;
                        for (size_t i = 0; i < candidates.size() && all_candidates_matches; i++)
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

                    if (match_len > 0)
                    {
                        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end-word_start));
                        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                    }

                    // List matches
                    AddLog("Possible matches:\n");
                    for (size_t i = 0; i < candidates.size(); i++)
                        AddLog("- %s\n", candidates[i]);
                }

                break;
            }
        case ImGuiInputTextFlags_CallbackHistory:
            {
                // Example of HISTORY
                const int prev_history_pos = HistoryPos;
                if (data->EventKey == ImGuiKey_UpArrow)
                {
                    if (HistoryPos == -1)
                        HistoryPos = (int)(History.size() - 1);
                    else if (HistoryPos > 0)
                        HistoryPos--;
                }
                else if (data->EventKey == ImGuiKey_DownArrow)
                {
                    if (HistoryPos != -1)
                        if (++HistoryPos >= (int)History.size())
                            HistoryPos = -1;
                }

                // A better implementation would preserve the data on the current input line along with cursor position.
                if (prev_history_pos != HistoryPos)
                {
                    ImFormatString(data->Buf, data->BufSize, "%s", (HistoryPos >= 0) ? History[HistoryPos] : "");
                    data->BufDirty = true;
                    data->CursorPos = data->SelectionStart = data->SelectionEnd = (int)strlen(data->Buf);
                }
            }
        }
        return 0;
    }
};

static void ShowExampleAppConsole(bool* opened)
{
    static ExampleAppConsole console;
    console.Run("Example: Console", opened);
}

static void ShowExampleAppLongText(bool* opened)
{
    ImGui::SetNextWindowSize(ImVec2(520,600), ImGuiSetCond_FirstUseEver);
    if (!ImGui::Begin("Example: Long text display", opened))
    {
        ImGui::End();
        return;
    }

    static int test_type = 0;
    static ImGuiTextBuffer log;
    static int lines = 0;
    ImGui::Text("Printing unusually long amount of text.");
    ImGui::Combo("Test type", &test_type, "Single call to TextUnformatted()\0Multiple calls to Text(), clipped manually\0Multiple calls to Text(), not clipped"); 
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
    switch (test_type)
    {
    case 0:
        // Single call to TextUnformatted() with a big buffer
        ImGui::TextUnformatted(log.begin(), log.end());
        break;
    case 1:
        // Multiple calls to Text(), manually coarsely clipped - demonstrate how to use the CalcListClipping() helper.
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));
        int display_start, display_end;
        ImGui::CalcListClipping(lines, ImGui::GetTextLineHeight(), &display_start, &display_end);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (display_start) * ImGui::GetTextLineHeight());
        for (int i = display_start; i < display_end; i++)
            ImGui::Text("%i The quick brown fox jumps over the lazy dog\n", i);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (lines - display_end) * ImGui::GetTextLineHeight());
        ImGui::PopStyleVar();
        break;
    case 2:
        // Multiple calls to Text(), not clipped
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));
        for (int i = 0; i < lines; i++)
            ImGui::Text("%i The quick brown fox jumps over the lazy dog\n", i);
        ImGui::PopStyleVar();
        break;
    }
    ImGui::EndChild();
    ImGui::End();
}

// End of Sample code
#endif

//-----------------------------------------------------------------------------
// FONT DATA
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// ProggyClean.ttf
// Copyright (c) 2004, 2005 Tristan Grimmer
// MIT license (see License.txt in http://www.upperbounds.net/download/ProggyClean.ttf.zip)
// Download and more information at http://upperbounds.net
//-----------------------------------------------------------------------------
// Compressed with stb_compress() then converted to a C array.
// Decompressor from stb.h (public domain) by Sean Barrett
// https://github.com/nothings/stb/blob/master/stb.h
//-----------------------------------------------------------------------------

static unsigned int stb_decompress_length(unsigned char *input)
{
    return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

static unsigned char *stb__barrier, *stb__barrier2, *stb__barrier3, *stb__barrier4;

static unsigned char *stb__dout;
static void stb__match(unsigned char *data, unsigned int length)
{
    // INVERSE of memmove... write each byte before copying the next...
    IM_ASSERT (stb__dout + length <= stb__barrier);
    if (stb__dout + length > stb__barrier) { stb__dout += length; return; }
    if (data < stb__barrier4) { stb__dout = stb__barrier+1; return; }
    while (length--) *stb__dout++ = *data++;
}

static void stb__lit(unsigned char *data, unsigned int length)
{
    IM_ASSERT (stb__dout + length <= stb__barrier);
    if (stb__dout + length > stb__barrier) { stb__dout += length; return; }
    if (data < stb__barrier2) { stb__dout = stb__barrier+1; return; }
    memcpy(stb__dout, data, length);
    stb__dout += length;
}

#define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
#define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
#define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

static unsigned char *stb_decompress_token(unsigned char *i)
{
    if (*i >= 0x20) { // use fewer if's for cases that expand small
        if (*i >= 0x80)       stb__match(stb__dout-i[1]-1, i[0] - 0x80 + 1), i += 2;
        else if (*i >= 0x40)  stb__match(stb__dout-(stb__in2(0) - 0x4000 + 1), i[2]+1), i += 3;
        else /* *i >= 0x20 */ stb__lit(i+1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
    } else { // more ifs for cases that expand large, since overhead is amortized
        if (*i >= 0x18)       stb__match(stb__dout-(stb__in3(0) - 0x180000 + 1), i[3]+1), i += 4;
        else if (*i >= 0x10)  stb__match(stb__dout-(stb__in3(0) - 0x100000 + 1), stb__in2(3)+1), i += 5;
        else if (*i >= 0x08)  stb__lit(i+2, stb__in2(0) - 0x0800 + 1), i += 2 + (stb__in2(0) - 0x0800 + 1);
        else if (*i == 0x07)  stb__lit(i+3, stb__in2(1) + 1), i += 3 + (stb__in2(1) + 1);
        else if (*i == 0x06)  stb__match(stb__dout-(stb__in3(1)+1), i[4]+1), i += 5;
        else if (*i == 0x04)  stb__match(stb__dout-(stb__in3(1)+1), stb__in2(4)+1), i += 6;
    }
    return i;
}

static unsigned int stb_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
    const unsigned long ADLER_MOD = 65521;
    unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
    unsigned long blocklen, i;

    blocklen = buflen % 5552;
    while (buflen) {
        for (i=0; i + 7 < blocklen; i += 8) {
            s1 += buffer[0], s2 += s1;
            s1 += buffer[1], s2 += s1;
            s1 += buffer[2], s2 += s1;
            s1 += buffer[3], s2 += s1;
            s1 += buffer[4], s2 += s1;
            s1 += buffer[5], s2 += s1;
            s1 += buffer[6], s2 += s1;
            s1 += buffer[7], s2 += s1;

            buffer += 8;
        }

        for (; i < blocklen; ++i)
            s1 += *buffer++, s2 += s1;

        s1 %= ADLER_MOD, s2 %= ADLER_MOD;
        buflen -= blocklen;
        blocklen = 5552;
    }
    return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static unsigned int stb_decompress(unsigned char *output, unsigned char *i, unsigned int length)
{
    unsigned int olen;
    if (stb__in4(0) != 0x57bC0000) return 0;
    if (stb__in4(4) != 0)          return 0; // error! stream is > 4GB
    olen = stb_decompress_length(i);
    stb__barrier2 = i;
    stb__barrier3 = i+length;
    stb__barrier = output + olen;
    stb__barrier4 = output;
    i += 16;

    stb__dout = output;
    for (;;) {
        unsigned char *old_i = i;
        i = stb_decompress_token(i);
        if (i == old_i) {
            if (*i == 0x05 && i[1] == 0xfa) {
                IM_ASSERT(stb__dout == output + olen);
                if (stb__dout != output + olen) return 0;
                if (stb_adler32(1, output, olen) != (unsigned int) stb__in4(2))
                    return 0;
                return olen;
            } else {
                IM_ASSERT(0); /* NOTREACHED */
                return 0;
            }
        }
        IM_ASSERT(stb__dout <= output + olen); 
        if (stb__dout > output + olen)
            return 0;
    }
}

static const unsigned int proggy_clean_ttf_compressed_size = 9583;
static const unsigned int proggy_clean_ttf_compressed_data[9584/4] =
{
    0x0000bc57, 0x00000000, 0xf8a00000, 0x00000400, 0x00010037, 0x000c0000, 0x00030080, 0x2f534f40, 0x74eb8832, 0x01000090, 0x2c158248, 0x616d634e, 
    0x23120270, 0x03000075, 0x241382a0, 0x74766352, 0x82178220, 0xfc042102, 0x02380482, 0x66796c67, 0x5689af12, 0x04070000, 0x80920000, 0x64616568, 
    0xd36691d7, 0xcc201b82, 0x36210382, 0x27108268, 0xc3014208, 0x04010000, 0x243b0f82, 0x78746d68, 0x807e008a, 0x98010000, 0x06020000, 0x61636f6c, 
    0xd8b0738c, 0x82050000, 0x0402291e, 0x7078616d, 0xda00ae01, 0x28201f82, 0x202c1082, 0x656d616e, 0x96bb5925, 0x84990000, 0x9e2c1382, 0x74736f70, 
    0xef83aca6, 0x249b0000, 0xd22c3382, 0x70657270, 0x12010269, 0xf4040000, 0x08202f82, 0x012ecb84, 0x553c0000, 0x0f5fd5e9, 0x0300f53c, 0x00830008, 
    0x7767b722, 0x002b3f82, 0xa692bd00, 0xfe0000d7, 0x83800380, 0x21f1826f, 0x00850002, 0x41820120, 0x40fec026, 0x80030000, 0x05821083, 0x07830120, 
    0x0221038a, 0x24118200, 0x90000101, 0x82798200, 0x00022617, 0x00400008, 0x2009820a, 0x82098276, 0x82002006, 0x9001213b, 0x0223c883, 0x828a02bc, 
    0x858f2010, 0xc5012507, 0x00023200, 0x04210083, 0x91058309, 0x6c412b03, 0x40007374, 0xac200000, 0x00830008, 0x01000523, 0x834d8380, 0x80032103, 
    0x012101bf, 0x23b88280, 0x00800000, 0x0b830382, 0x07820120, 0x83800021, 0x88012001, 0x84002009, 0x2005870f, 0x870d8301, 0x2023901b, 0x83199501, 
    0x82002015, 0x84802000, 0x84238267, 0x88002027, 0x8561882d, 0x21058211, 0x13880000, 0x01800022, 0x05850d85, 0x0f828020, 0x03208384, 0x03200582, 
    0x47901b84, 0x1b850020, 0x1f821d82, 0x3f831d88, 0x3f410383, 0x84058405, 0x210982cd, 0x09830000, 0x03207789, 0xf38a1384, 0x01203782, 0x13872384, 
    0x0b88c983, 0x0d898f84, 0x00202982, 0x23900383, 0x87008021, 0x83df8301, 0x86118d03, 0x863f880d, 0x8f35880f, 0x2160820f, 0x04830300, 0x1c220382, 
    0x05820100, 0x4c000022, 0x09831182, 0x04001c24, 0x11823000, 0x0800082e, 0x00000200, 0xff007f00, 0xffffac20, 0x00220982, 0x09848100, 0xdf216682, 
    0x843586d5, 0x06012116, 0x04400684, 0xa58120d7, 0x00b127d8, 0x01b88d01, 0x2d8685ff, 0xc100c621, 0xf4be0801, 0x9e011c01, 0x88021402, 0x1403fc02, 
    0x9c035803, 0x1404de03, 0x50043204, 0xa2046204, 0x66051605, 0x1206bc05, 0xd6067406, 0x7e073807, 0x4e08ec07, 0x96086c08, 0x1009d008, 0x88094a09, 
    0x800a160a, 0x560b040b, 0x2e0cc80b, 0xea0c820c, 0xa40d5e0d, 0x500eea0d, 0x280f960e, 0x1210b00f, 0xe0107410, 0xb6115211, 0x6e120412, 0x4c13c412, 
    0xf613ac13, 0xae145814, 0x4015ea14, 0xa6158015, 0x1216b815, 0xc6167e16, 0x8e173417, 0x5618e017, 0xee18ba18, 0x96193619, 0x481ad419, 0xf01a9c1a, 
    0xc81b5c1b, 0x4c1c041c, 0xea1c961c, 0x921d2a1d, 0x401ed21d, 0xe01e8e1e, 0x761f241f, 0xa61fa61f, 0x01821020, 0x8a202e34, 0xc820b220, 0x74211421, 
    0xee219821, 0x86226222, 0x01820c23, 0x83238021, 0x23983c01, 0x24d823b0, 0x244a2400, 0x24902468, 0x250625ae, 0x25822560, 0x26f825f8, 0x82aa2658, 
    0xd8be0801, 0x9a274027, 0x68280a28, 0x0e29a828, 0xb8292029, 0x362af829, 0x602a602a, 0x2a2b022b, 0xac2b5e2b, 0x202ce62b, 0x9a2c342c, 0x5c2d282d, 
    0xaa2d782d, 0x262ee82d, 0x262fa62e, 0xf42fb62f, 0xc8305e30, 0xb4313e31, 0x9e321e32, 0x82331e33, 0x5c34ee33, 0x3a35ce34, 0xd4358635, 0x72362636, 
    0x7637e636, 0x3a38d837, 0x1239a638, 0xae397439, 0x9a3a2e3a, 0x7c3b063b, 0x3a3ce83b, 0x223d963c, 0xec3d863d, 0xc63e563e, 0x9a3f2a3f, 0x6a401240, 
    0x3641d040, 0x0842a241, 0x7a424042, 0xf042b842, 0xcc436243, 0x8a442a44, 0x5845ee44, 0xe245b645, 0xb4465446, 0x7a471447, 0x5448da47, 0x4049c648, 
    0x15462400, 0x034d0808, 0x0b000700, 0x13000f00, 0x1b001700, 0x23001f00, 0x2b002700, 0x33002f00, 0x3b003700, 0x43003f00, 0x4b004700, 0x53004f00, 
    0x5b005700, 0x63005f00, 0x6b006700, 0x73006f00, 0x7b007700, 0x83007f00, 0x8b008700, 0x00008f00, 0x15333511, 0x20039631, 0x20178205, 0xd3038221, 
    0x20739707, 0x25008580, 0x028080fc, 0x05be8080, 0x04204a85, 0x05ce0685, 0x0107002a, 0x02000080, 0x00000400, 0x250d8b41, 0x33350100, 0x03920715, 
    0x13820320, 0x858d0120, 0x0e8d0320, 0xff260d83, 0x00808000, 0x54820106, 0x04800223, 0x845b8c80, 0x41332059, 0x078b068f, 0x82000121, 0x82fe2039, 
    0x84802003, 0x83042004, 0x23598a0e, 0x00180000, 0x03210082, 0x42ab9080, 0x73942137, 0x2013bb41, 0x8f978205, 0x2027a39b, 0x20b68801, 0x84b286fd, 
    0x91c88407, 0x41032011, 0x11a51130, 0x15000027, 0x80ff8000, 0x11af4103, 0x841b0341, 0x8bd983fd, 0x9be99bc9, 0x8343831b, 0x21f1821f, 0xb58300ff, 
    0x0f84e889, 0xf78a0484, 0x8000ff22, 0x0020eeb3, 0x14200082, 0x2130ef41, 0xeb431300, 0x4133200a, 0xd7410ecb, 0x9a07200b, 0x2027871b, 0x21238221, 
    0xe7828080, 0xe784fd20, 0xe8848020, 0xfe808022, 0x08880d85, 0xba41fd20, 0x82248205, 0x85eab02a, 0x008022e7, 0x2cd74200, 0x44010021, 0xd34406eb, 
    0x44312013, 0xcf8b0eef, 0x0d422f8b, 0x82332007, 0x0001212f, 0x8023cf82, 0x83000180, 0x820583de, 0x830682d4, 0x820020d4, 0x82dc850a, 0x20e282e9, 
    0xb2ff85fe, 0x010327e9, 0x02000380, 0x0f440400, 0x0c634407, 0x68825982, 0x85048021, 0x260a825d, 0x010b0000, 0x4400ff00, 0x2746103f, 0x08d74209, 
    0x4d440720, 0x0eaf4406, 0xc3441d20, 0x23078406, 0xff800002, 0x04845b83, 0x8d05b241, 0x1781436f, 0x6b8c87a5, 0x1521878e, 0x06474505, 0x01210783, 
    0x84688c00, 0x8904828e, 0x441e8cf7, 0x0b270cff, 0x80008000, 0x45030003, 0xfb430fab, 0x080f4107, 0x410bf942, 0xd34307e5, 0x070d4207, 0x80800123, 
    0x205d85fe, 0x849183fe, 0x20128404, 0x82809702, 0x00002217, 0x41839a09, 0x6b4408cf, 0x0733440f, 0x3b460720, 0x82798707, 0x97802052, 0x0000296f, 
    0xff800004, 0x01800100, 0x0021ef89, 0x0a914625, 0x410a4d41, 0x00250ed4, 0x00050000, 0x056d4280, 0x210a7b46, 0x21481300, 0x46ed8512, 0x00210bd1, 
    0x89718202, 0x21738877, 0x2b850001, 0x00220582, 0x87450a00, 0x0ddb4606, 0x41079b42, 0x9d420c09, 0x0b09420b, 0x8d820720, 0x9742fc84, 0x42098909, 
    0x00241e0f, 0x00800014, 0x0b47da82, 0x0833442a, 0x49078d41, 0x2f450f13, 0x42278f17, 0x01200751, 0x22063742, 0x44808001, 0x20450519, 0x88068906, 
    0x83fe2019, 0x4203202a, 0x1a941a58, 0x00820020, 0xe7a40e20, 0x420ce146, 0x854307e9, 0x0fcb4713, 0xff20a182, 0xfe209b82, 0x0c867f8b, 0x0021aea4, 
    0x219fa40f, 0x7d41003b, 0x07194214, 0xbf440520, 0x071d4206, 0x6941a590, 0x80802309, 0x028900ff, 0xa9a4b685, 0xc5808021, 0x449b82ab, 0x152007eb, 
    0x42134d46, 0x61440a15, 0x051e4208, 0x222b0442, 0x47001100, 0xfd412913, 0x17194714, 0x410f5b41, 0x02220773, 0x09428080, 0x21a98208, 0xd4420001, 
    0x481c840d, 0x00232bc9, 0x42120000, 0xe74c261b, 0x149d4405, 0x07209d87, 0x410db944, 0x14421c81, 0x42fd2005, 0x80410bd2, 0x203d8531, 0x06874100, 
    0x48256f4a, 0xcb420c95, 0x13934113, 0x44075d44, 0x044c0855, 0x00ff2105, 0xfe228185, 0x45448000, 0x22c5b508, 0x410c0000, 0x7b412087, 0x1bb74514, 
    0x32429c85, 0x0a574805, 0x21208943, 0x8ba01300, 0x440dfb4e, 0x77431437, 0x245b4113, 0x200fb145, 0x41108ffe, 0x80203562, 0x00200082, 0x46362b42, 
    0x1742178d, 0x4527830f, 0x0f830b2f, 0x4a138146, 0x802409a1, 0xfe8000ff, 0x94419982, 0x09294320, 0x04000022, 0x49050f4f, 0xcb470a63, 0x48032008, 
    0x2b48067b, 0x85022008, 0x82638338, 0x00002209, 0x05af4806, 0x900e9f49, 0x84c5873f, 0x214285bd, 0x064900ff, 0x0c894607, 0x00000023, 0x4903820a, 
    0x714319f3, 0x0749410c, 0x8a07a145, 0x02152507, 0xfe808000, 0x74490386, 0x8080211b, 0x0c276f82, 0x00018000, 0x48028003, 0x2b2315db, 0x43002f00, 
    0x6f82142f, 0x44011521, 0x93510da7, 0x20e68508, 0x06494d80, 0x8e838020, 0x06821286, 0x124bff20, 0x25f3830c, 0x03800080, 0xe74a0380, 0x207b8715, 
    0x876b861d, 0x4a152007, 0x07870775, 0xf6876086, 0x8417674a, 0x0a0021f2, 0x431c9743, 0x8d421485, 0x200b830b, 0x06474d03, 0x71828020, 0x04510120, 
    0x42da8606, 0x1f831882, 0x001a0022, 0xff4d0082, 0x0b0f532c, 0x0d449b94, 0x4e312007, 0x074f12e7, 0x0bf3490b, 0xbb412120, 0x413f820a, 0xef490857, 
    0x80002313, 0xe2830001, 0x6441fc20, 0x8b802006, 0x00012108, 0xfd201582, 0x492c9b48, 0x802014ff, 0x51084347, 0x0f4327f3, 0x17bf4a14, 0x201b7944, 
    0x06964201, 0x134ffe20, 0x20d6830b, 0x25d78280, 0xfd800002, 0x05888000, 0x9318dc41, 0x21d282d4, 0xdb481800, 0x0dff542a, 0x45107743, 0xe14813f5, 
    0x0f034113, 0x83135d45, 0x47b28437, 0xe4510e73, 0x21f58e06, 0x2b8400fd, 0x1041fcac, 0x08db4b0b, 0x421fdb41, 0xdf4b18df, 0x011d210a, 0x420af350, 
    0x6e8308af, 0xac85cb86, 0x1e461082, 0x82b7a407, 0x411420a3, 0xa34130ab, 0x178f4124, 0x41139741, 0x86410d93, 0x82118511, 0x057243d8, 0x8941d9a4, 
    0x3093480c, 0x4a13474f, 0xfb5016a9, 0x07ad4108, 0x4a0f9d42, 0xfe200fad, 0x4708aa41, 0x83482dba, 0x288f4d06, 0xb398c3bb, 0x44267b41, 0xb34439d7, 
    0x0755410f, 0x200ebb45, 0x0f5f4215, 0x20191343, 0x06df5301, 0xf04c0220, 0x2ba64d07, 0x82050841, 0x430020ce, 0xa78f3627, 0x5213ff42, 0x2f970bc1, 
    0x4305ab55, 0xa084111b, 0x450bac45, 0x5f4238b8, 0x010c2106, 0x0220ed82, 0x441bb344, 0x875010af, 0x0737480f, 0x490c5747, 0x0c840c03, 0x4c204b42, 
    0x8ba905d7, 0x8b948793, 0x510c0c51, 0xfb4b24b9, 0x1b174107, 0x5709d74c, 0xd1410ca5, 0x079d480f, 0x201ff541, 0x06804780, 0x7d520120, 0x80002205, 
    0x20a983fe, 0x47bb83fe, 0x1b8409b4, 0x81580220, 0x4e00202c, 0x4f41282f, 0x0eab4f17, 0x57471520, 0x0e0f4808, 0x8221e041, 0x3e1b4a8b, 0x4407175d, 
    0x1b4b071f, 0x4a0f8b07, 0x174a0703, 0x0ba5411b, 0x430fb141, 0x0120057b, 0xfc20dd82, 0x4a056047, 0xf4850c0c, 0x01221982, 0x02828000, 0x1a5d088b, 
    0x20094108, 0x8c0e3941, 0x4900200e, 0x7744434f, 0x200b870b, 0x0e4b5a33, 0x2b41f78b, 0x8b138307, 0x0b9f450b, 0x2406f741, 0xfd808001, 0x09475a00, 
    0x84000121, 0x5980200e, 0x85450e5d, 0x832c8206, 0x4106831e, 0x00213814, 0x28b34810, 0x410c2f4b, 0x5f4a13d7, 0x0b2b4113, 0x6e43a883, 0x11174b05, 
    0x4b066a45, 0xcc470541, 0x5000202b, 0xcb472f4b, 0x44b59f0f, 0xc5430b5b, 0x0d654907, 0x21065544, 0xd6828080, 0xfe201982, 0x8230ec4a, 0x120025c2, 
    0x80ff8000, 0x4128d74d, 0x3320408b, 0x410a9f50, 0xdb822793, 0x822bd454, 0x61134b2e, 0x410b214a, 0xad4117c9, 0x0001211f, 0x4206854f, 0x4b430596, 
    0x06bb5530, 0x2025cf46, 0x0ddd5747, 0x500ea349, 0x0f840fa7, 0x5213c153, 0x634e08d1, 0x0bbe4809, 0x59316e4d, 0x5b50053f, 0x203f6323, 0x5117eb46, 
    0x94450a63, 0x246e410a, 0x63410020, 0x0bdb5f2f, 0x4233ab44, 0x39480757, 0x112d4a07, 0x7241118f, 0x000e2132, 0x9f286f41, 0x0f8762c3, 0x33350723, 
    0x094e6415, 0x2010925f, 0x067252fe, 0xd0438020, 0x63a68225, 0x11203a4f, 0x480e6360, 0x5748131f, 0x079b521f, 0x200e2f43, 0x864b8315, 0x113348e7, 
    0x85084e48, 0x06855008, 0x5880fd21, 0x7c420925, 0x0c414824, 0x37470c86, 0x1b8b422b, 0x5b0a8755, 0x23410c21, 0x0b83420b, 0x5a082047, 0xf482067f, 
    0xa80b4c47, 0x0c0021cf, 0x20207b42, 0x0fb74100, 0x420b8744, 0xeb43076f, 0x0f6f420b, 0x4261fe20, 0x439aa00c, 0x215034e3, 0x0ff9570f, 0x4b1f2d5d, 
    0x2d5d0c6f, 0x09634d0b, 0x1f51b8a0, 0x620f200c, 0xaf681e87, 0x24f94d07, 0x4e0f4945, 0xfe200c05, 0x22139742, 0x57048080, 0x23950c20, 0x97601585, 
    0x4813201f, 0xad620523, 0x200f8f0f, 0x9e638f15, 0x00002181, 0x41342341, 0x0f930f0b, 0x210b4b62, 0x978f0001, 0xfe200f84, 0x8425c863, 0x2704822b, 
    0x80000a00, 0x00038001, 0x610e9768, 0x834514bb, 0x0bc3430f, 0x2107e357, 0x80848080, 0x4400fe21, 0x2e410983, 0x00002a1a, 0x00000700, 0x800380ff, 
    0x0fdf5800, 0x59150021, 0xd142163d, 0x0c02410c, 0x01020025, 0x65800300, 0x00240853, 0x1d333501, 0x15220382, 0x35420001, 0x44002008, 0x376406d7, 
    0x096f6b19, 0x480bc142, 0x8f4908a7, 0x211f8b1f, 0x9e830001, 0x0584fe20, 0x4180fd21, 0x11850910, 0x8d198259, 0x000021d4, 0x5a08275d, 0x275d1983, 
    0x06d9420e, 0x9f08b36a, 0x0f7d47b5, 0x8d8a2f8b, 0x4c0e0b57, 0xe7410e17, 0x42d18c1a, 0xb351087a, 0x1ac36505, 0x4b4a2f20, 0x0b9f450d, 0x430beb53, 
    0xa7881015, 0xa5826a83, 0x80200f82, 0x86185a65, 0x4100208e, 0x176c3367, 0x0fe7650b, 0x4a17ad4b, 0x0f4217ed, 0x112e4206, 0x41113a42, 0xf7423169, 
    0x0cb34737, 0x560f8b46, 0xa75407e5, 0x5f01200f, 0x31590c48, 0x80802106, 0x42268841, 0x0020091e, 0x4207ef64, 0x69461df7, 0x138d4114, 0x820f5145, 
    0x53802090, 0xff200529, 0xb944b183, 0x417e8505, 0x00202561, 0x15210082, 0x42378200, 0x9b431cc3, 0x004f220d, 0x0dd54253, 0x4213f149, 0x7d41133b, 
    0x42c9870b, 0x802010f9, 0x420b2c42, 0x8f441138, 0x267c4408, 0x600cb743, 0x8f4109d3, 0x05ab701d, 0x83440020, 0x3521223f, 0x0b794733, 0xfb62fe20, 
    0x4afd2010, 0xaf410ae7, 0x25ce8525, 0x01080000, 0x7b6b0000, 0x0973710b, 0x82010021, 0x49038375, 0x33420767, 0x052c4212, 0x58464b85, 0x41fe2005, 
    0x50440c27, 0x000c2209, 0x1cb36b80, 0x9b06df44, 0x0f93566f, 0x52830220, 0xfe216e8d, 0x200f8200, 0x0fb86704, 0xb057238d, 0x050b5305, 0x7217eb47, 
    0xbd410b6b, 0x0f214610, 0x871f9956, 0x1e91567e, 0x2029b741, 0x20008200, 0x18b7410a, 0x27002322, 0x41095543, 0x0f8f0fb3, 0x41000121, 0x889d111c, 
    0x14207b82, 0x00200382, 0x73188761, 0x475013a7, 0x6e33200c, 0x234e0ea3, 0x9b138313, 0x08e54d17, 0x9711094e, 0x2ee74311, 0x4908875e, 0xd75d1f1f, 
    0x19ab5238, 0xa2084d48, 0x63a7a9b3, 0x55450b83, 0x0fd74213, 0x440d814c, 0x4f481673, 0x05714323, 0x13000022, 0x412e1f46, 0xdf493459, 0x21c7550f, 
    0x8408215f, 0x201d49cb, 0xb1103043, 0x0f0d65d7, 0x452b8d41, 0x594b0f8d, 0x0b004605, 0xb215eb46, 0x000a24d7, 0x47000080, 0x002118cf, 0x06436413, 
    0x420bd750, 0x2b500743, 0x076a470c, 0x4105c050, 0xd942053f, 0x0d00211a, 0x5f44779c, 0x0ce94805, 0x51558186, 0x14a54c0b, 0x49082b41, 0x0a4b0888, 
    0x8080261f, 0x0d000000, 0x20048201, 0x1deb6a03, 0x420cb372, 0x07201783, 0x4306854d, 0x8b830c59, 0x59093c74, 0x0020250f, 0x67070f4a, 0x2341160b, 
    0x00372105, 0x431c515d, 0x554e17ef, 0x0e5d6b05, 0x41115442, 0xb74a1ac1, 0x2243420a, 0x5b4f878f, 0x7507200f, 0x384b086f, 0x09d45409, 0x0020869a, 
    0x12200082, 0xab460382, 0x10075329, 0x54138346, 0xaf540fbf, 0x1ea75413, 0x9a0c9e54, 0x0f6b44c1, 0x41000021, 0x47412a4f, 0x07374907, 0x5310bf76, 
    0xff2009b4, 0x9a09a64c, 0x8200208d, 0x34c34500, 0x970fe141, 0x1fd74b0f, 0x440a3850, 0x206411f0, 0x27934609, 0x470c5d41, 0x555c2947, 0x1787540f, 
    0x6e0f234e, 0x7d540a1b, 0x1d736b08, 0x0026a088, 0x80000e00, 0x9b5200ff, 0x08ef4318, 0x450bff77, 0x1d4d0b83, 0x081f7006, 0xcb691b86, 0x4b022008, 
    0xc34b0b33, 0x1d0d4a0c, 0x8025a188, 0x0b000000, 0x52a38201, 0xbf7d0873, 0x0c234511, 0x8f0f894a, 0x4101200f, 0x0c880c9d, 0x2b418ea1, 0x06c74128, 
    0x66181341, 0x7b4c0bb9, 0x0c06630b, 0xfe200c87, 0x9ba10882, 0x27091765, 0x01000008, 0x02800380, 0x48113f4e, 0x29430cf5, 0x09a75a0b, 0x31618020, 
    0x6d802009, 0x61840e33, 0x8208bf51, 0x0c637d61, 0x7f092379, 0x4f470f4b, 0x1797510c, 0x46076157, 0xf5500fdf, 0x0f616910, 0x1171fe20, 0x82802006, 
    0x08696908, 0x41127a4c, 0x3f4a15f3, 0x01042607, 0x0200ff00, 0x1cf77700, 0xff204185, 0x00235b8d, 0x43100000, 0x3b22243f, 0x3b4d3f00, 0x0b937709, 
    0xad42f18f, 0x0b1f420f, 0x51084b43, 0x8020104a, 0xb557ff83, 0x052b7f2a, 0x0280ff22, 0x250beb78, 0x00170013, 0xbf6d2500, 0x07db760e, 0x410e2b7f, 
    0x00230e4f, 0x49030000, 0x0582055b, 0x07000326, 0x00000b00, 0x580bcd46, 0x00200cdd, 0x57078749, 0x8749160f, 0x0f994f0a, 0x41134761, 0x01200b31, 
    0xeb796883, 0x0b41500b, 0x0e90b38e, 0x202e7b51, 0x05d95801, 0x41080570, 0x1d530fc9, 0x0b937a0f, 0xaf8eb387, 0xf743b98f, 0x07c74227, 0x80000523, 
    0x0fcb4503, 0x430ca37b, 0x7782077f, 0x8d0a9947, 0x08af4666, 0xeb798020, 0x6459881e, 0xc3740bbf, 0x0feb6f0b, 0x20072748, 0x052b6102, 0x435e0584, 
    0x7d088308, 0x03200afd, 0x92109e41, 0x28aa8210, 0x80001500, 0x80030000, 0x0fdb5805, 0x209f4018, 0xa7418d87, 0x0aa3440f, 0x20314961, 0x073a52ff, 
    0x6108505d, 0x43181051, 0x00223457, 0xe7820500, 0x50028021, 0x81410d33, 0x063d7108, 0xdb41af84, 0x4d888205, 0x00201198, 0x463d835f, 0x152106d7, 
    0x0a355a33, 0x6917614e, 0x75411f4d, 0x184b8b07, 0x1809c344, 0x21091640, 0x0b828000, 0x42808021, 0x26790519, 0x86058605, 0x2428422d, 0x22123b42, 
    0x42000080, 0xf587513b, 0x7813677b, 0xaf4d139f, 0x00ff210c, 0x5e0a1d57, 0x3b421546, 0x01032736, 0x02000380, 0x41180480, 0x2f420f07, 0x0c624807, 
    0x00000025, 0x18000103, 0x83153741, 0x430120c3, 0x042106b2, 0x088d4d00, 0x2f830620, 0x1810434a, 0x18140345, 0x8507fb41, 0x5ee582ea, 0x0023116c, 
    0x8d000600, 0x053b56af, 0xa6554fa2, 0x0d704608, 0x40180d20, 0x47181a43, 0xd37b07ff, 0x0b79500c, 0x420fd745, 0x47450bd9, 0x8471830a, 0x095a777e, 
    0x84137542, 0x82002013, 0x2f401800, 0x0007213b, 0x4405e349, 0x0d550ff3, 0x16254c0c, 0x820ffe4a, 0x0400218a, 0x89066f41, 0x106b414f, 0xc84d0120, 
    0x80802206, 0x0c9a4b03, 0x00100025, 0x68000200, 0x9d8c2473, 0x44134344, 0xf36a0f33, 0x4678860f, 0x1b440a25, 0x41988c0a, 0x80201879, 0x43079b5e, 
    0x4a18080b, 0x0341190b, 0x1259530c, 0x43251552, 0x908205c8, 0x0cac4018, 0x86000421, 0x0e504aa2, 0x0020b891, 0xfb450082, 0x51132014, 0x8f5205f3, 
    0x35052108, 0x8505cb59, 0x0f6d4f70, 0x82150021, 0x29af5047, 0x4f004b24, 0x75795300, 0x1b595709, 0x460b6742, 0xbf4b0f0d, 0x5743870b, 0xcb6d1461, 
    0x08f64505, 0x4e05ab6c, 0x334126c3, 0x0bcb6b0d, 0x1811034d, 0x4111ef4b, 0x814f1ce5, 0x20af8227, 0x07fd7b80, 0x41188e84, 0xef410f33, 0x80802429, 
    0x410d0000, 0xa34205ab, 0x76b7881c, 0xff500b89, 0x0741430f, 0x20086f4a, 0x209d8200, 0x234c18fd, 0x05d4670a, 0x4509af51, 0x9642078d, 0x189e831d, 
    0x7c1cc74b, 0xcd4c07b9, 0x0e7c440f, 0x8b7b0320, 0x21108210, 0xc76c8080, 0x03002106, 0x6b23bf41, 0xc549060b, 0x7946180b, 0x0ff7530f, 0x17ad4618, 
    0x200ecd45, 0x208c83fd, 0x5e0488fe, 0x032009c6, 0x420d044e, 0x0d8f0d7f, 0x00820020, 0x18001021, 0x6d273b45, 0xfd4c0c93, 0xcf451813, 0x0fe5450f, 
    0x5a47c382, 0x820a8b0a, 0x282b4998, 0x410a8b5b, 0x4b232583, 0x54004f00, 0x978f0ce3, 0x500f1944, 0xa95f1709, 0x0280220b, 0x05ba7080, 0xa1530682, 
    0x06324c13, 0x91412582, 0x05536e2c, 0x63431020, 0x0f434706, 0x8c11374c, 0x176143d7, 0x4d0f454c, 0xd3680bed, 0x0bee4d17, 0x212b9a41, 0x0f530a00, 
    0x140d531c, 0x43139143, 0x95610e8d, 0x0f094415, 0x4205fb56, 0x1b4205cf, 0x17015225, 0x5e0c477f, 0xaf6e0aeb, 0x0ff36218, 0x04849a84, 0x0a454218, 
    0x9c430420, 0x23c6822b, 0x04000102, 0x45091b4b, 0xf05f0955, 0x82802007, 0x421c2023, 0x5218282b, 0x7b53173f, 0x0fe7480c, 0x74173b7f, 0x47751317, 
    0x634d1807, 0x0f6f430f, 0x24086547, 0xfc808002, 0x0b3c7f80, 0x10840120, 0x188d1282, 0x20096b43, 0x0fc24403, 0x00260faf, 0x0180000b, 0x3f500280, 
    0x18002019, 0x450b4941, 0xf3530fb9, 0x18002010, 0x8208a551, 0x06234d56, 0xcb58a39b, 0xc3421805, 0x1313461e, 0x0f855018, 0xd34b0120, 0x6cfe2008, 
    0x574f0885, 0x09204114, 0x07000029, 0x00008000, 0x44028002, 0x01420f57, 0x10c95c10, 0x11184c18, 0x80221185, 0x7f421e00, 0x00732240, 0x09cd4977, 
    0x6d0b2b42, 0x4f180f8f, 0x8f5a0bcb, 0x9b0f830f, 0x0fb9411f, 0x230b5756, 0x00fd8080, 0x82060745, 0x000121d5, 0x8e0fb277, 0x4a8d4211, 0x24061c53, 
    0x04000007, 0x12275280, 0x430c954c, 0x80201545, 0x200f764f, 0x20008200, 0x20ce8308, 0x09534f02, 0x660edf64, 0x73731771, 0xe7411807, 0x20a2820c, 
    0x13b64404, 0x8f5d6682, 0x1d6b4508, 0x0cff4d18, 0x3348c58f, 0x0fc34c07, 0x31558b84, 0x8398820f, 0x17514712, 0x240b0e46, 0x80000a00, 0x093b4502, 
    0x420f9759, 0xa54c0bf1, 0x0f2b470c, 0x410d314b, 0x2584170c, 0x73b30020, 0xb55fe782, 0x204d8410, 0x08e043fe, 0x4f147e41, 0x022008ab, 0x4b055159, 
    0x2950068f, 0x00022208, 0x48511880, 0x82002009, 0x00112300, 0x634dff00, 0x24415f27, 0x180f6d43, 0x4d0b5d45, 0x4d5f05ef, 0x01802317, 0x56188000, 
    0xa7840807, 0xc6450220, 0x21ca8229, 0x4b781a00, 0x3359182c, 0x0cf3470f, 0x180bef46, 0x420b0354, 0xff470b07, 0x4515200a, 0x9758239b, 0x4a80200c, 
    0xd2410a26, 0x05fb4a08, 0x4b05e241, 0x03200dc9, 0x92290941, 0x00002829, 0x00010900, 0x5b020001, 0x23201363, 0x460d776a, 0xef530fdb, 0x209a890c, 
    0x13fc4302, 0x00008024, 0xc4820104, 0x08820220, 0x20086b5b, 0x18518700, 0x8408d349, 0x0da449a1, 0x00080024, 0x7b690280, 0x4c438b1a, 0x01220f63, 
    0x4c878000, 0x5c149c53, 0xfb430868, 0x2f56181e, 0x0ccf7b1b, 0x0f075618, 0x2008e347, 0x14144104, 0x00207f83, 0x00207b82, 0x201adf47, 0x16c35a13, 
    0x540fdf47, 0x802006c8, 0x5418f185, 0x29430995, 0x00002419, 0x58001600, 0x5720316f, 0x4d051542, 0x4b7b1b03, 0x138f4707, 0xb747b787, 0x4aab8213, 
    0x058305fc, 0x20115759, 0x82128401, 0x0a0b44e8, 0x46800121, 0xe64210d0, 0x82129312, 0x4bffdffe, 0x3b41171b, 0x9b27870f, 0x808022ff, 0x085c68fe, 
    0x41800021, 0x01410b20, 0x001a213a, 0x47480082, 0x11374e12, 0x56130b4c, 0xdf4b0c65, 0x0b0f590b, 0x0f574c18, 0x830feb4b, 0x075f480f, 0x480b4755, 
    0x40490b73, 0x80012206, 0x09d74280, 0x80fe8022, 0x80210e86, 0x056643ff, 0x10820020, 0x420b2646, 0x0b58391a, 0xd74c1808, 0x078b4e22, 0x2007f55f, 
    0x4b491807, 0x83802017, 0x65aa82a7, 0x3152099e, 0x068b7616, 0x9b431220, 0x09bb742c, 0x500e376c, 0x8342179b, 0x0a4d5d0f, 0x8020a883, 0x180cd349, 
    0x2016bb4b, 0x14476004, 0x84136c43, 0x08cf7813, 0x4f4c0520, 0x156f420f, 0x20085f42, 0x6fd3be03, 0xd4d30803, 0xa7411420, 0x004b222c, 0x0d3b614f, 
    0x3f702120, 0x1393410a, 0x8f132745, 0x47421827, 0x41e08209, 0xb05e2bb9, 0x18b7410c, 0x18082647, 0x4107a748, 0xeb8826bf, 0x0ca76018, 0x733ecb41, 
    0xd0410d83, 0x43ebaf2a, 0x0420067f, 0x721dab4c, 0x472005bb, 0x4105d341, 0x334844cb, 0x20dba408, 0x47d6ac00, 0x034e3aef, 0x0f8f421b, 0x930f134d, 
    0x3521231f, 0xb7421533, 0x42f5ad0a, 0x1e961eaa, 0x17000022, 0x4c367b50, 0x7d491001, 0x0bf5520f, 0x4c18fda7, 0xb8460c55, 0x83fe2005, 0x00fe25b9, 
    0x80000180, 0x9e751085, 0x261b5c12, 0x82110341, 0x001123fb, 0x4518fe80, 0xf38c2753, 0x6d134979, 0x295107a7, 0xaf5f180f, 0x0fe3660c, 0x180b6079, 
    0x2007bd5f, 0x9aab9103, 0x2f4d1811, 0x05002109, 0x44254746, 0x1d200787, 0x450bab75, 0x4f180f57, 0x4f181361, 0x3b831795, 0xeb4b0120, 0x0b734805, 
    0x84078f48, 0x2e1b47bc, 0x00203383, 0xaf065f45, 0x831520d7, 0x130f51a7, 0x1797bf97, 0x2b47d783, 0x18fe2005, 0x4a18a44f, 0xa64d086d, 0x1ab0410d, 
    0x6205a258, 0xdbab069f, 0x4f06f778, 0xa963081d, 0x133b670a, 0x8323d141, 0x13195b23, 0x530f5e70, 0xe5ad0824, 0x58001421, 0x1f472b4b, 0x47bf410c, 
    0x82000121, 0x83fe20cb, 0x07424404, 0x68068243, 0xd7ad0d3d, 0x00010d26, 0x80020000, 0x4a1c6f43, 0x23681081, 0x10a14f13, 0x8a070e57, 0x430a848f, 
    0x7372243e, 0x4397a205, 0xb56c1021, 0x43978f0f, 0x64180505, 0x99aa0ff2, 0x0e000022, 0x20223341, 0x094b4f37, 0x074a3320, 0x2639410a, 0xfe208e84, 
    0x8b0e0048, 0x508020a3, 0x9e4308fe, 0x073f4115, 0xe3480420, 0x0c9b5f1b, 0x7c137743, 0x9a95185b, 0x6122b148, 0x979b08df, 0x0fe36c18, 0x48109358, 
    0x23441375, 0x0ffd5c0b, 0x180fc746, 0x2011d157, 0x07e95702, 0x58180120, 0x18770ac3, 0x51032008, 0x7d4118e3, 0x80802315, 0x3b4c1900, 0xbb5a1830, 
    0x0ceb6109, 0x5b0b3d42, 0x4f181369, 0x4f180b8d, 0x4f180f75, 0x355a1b81, 0x200d820d, 0x18e483fd, 0x4528854f, 0x89420846, 0x1321411f, 0x44086b60, 
    0x07421d77, 0x107d4405, 0x4113fd41, 0x5a181bf1, 0x4f180db3, 0x8021128f, 0x20f68280, 0x44a882fe, 0x334d249a, 0x052f6109, 0x1520c3a7, 0xef4eb783, 
    0x4ec39b1b, 0xc4c90ee7, 0x20060b4d, 0x256f4905, 0x4d0cf761, 0xcf9b1f13, 0xa213d74e, 0x0e1145d4, 0x50135b42, 0xcb4e398f, 0x20d79f27, 0x08865d80, 
    0x186d5018, 0xa90f7142, 0x067342d7, 0x3f450420, 0x65002021, 0xe3560771, 0x24d38f23, 0x15333531, 0x0eb94d01, 0x451c9f41, 0x384322fb, 0x00092108, 
    0x19af6b18, 0x6e0c6f5a, 0xbd770bfb, 0x22bb7718, 0x20090f57, 0x25e74204, 0x4207275a, 0xdb5408ef, 0x1769450f, 0x1b1b5518, 0x210b1f57, 0x5e4c8001, 
    0x55012006, 0x802107f1, 0x0a306a80, 0x45808021, 0x0d850b88, 0x31744f18, 0x1808ec54, 0x2009575b, 0x45ffa505, 0x1b420c73, 0x180f9f0f, 0x4a0cf748, 
    0x501805b2, 0x00210f40, 0x4d118f80, 0xd6823359, 0x072b5118, 0x314ad7aa, 0x8fc79f08, 0x45d78b1f, 0xfe20058f, 0x23325118, 0x7b54d9b5, 0x9fc38f46, 
    0x10bb410f, 0x41077b42, 0xc1410faf, 0x27cf441d, 0x46051b4f, 0x04200683, 0x2121d344, 0x8f530043, 0x8fcf9f0e, 0x21df8c1f, 0x50188000, 0x5d180e52, 
    0xfd201710, 0x4405c341, 0xd68528e3, 0x20071f6b, 0x1b734305, 0x6b080957, 0x7d422b1f, 0x67002006, 0x7f8317b1, 0x2024cb48, 0x08676e00, 0x8749a39b, 
    0x18132006, 0x410a6370, 0x8f490b47, 0x7e1f8f13, 0x551805c3, 0x4c180915, 0xfe200e2f, 0x244d5d18, 0x270bcf44, 0xff000019, 0x04800380, 0x5f253342, 
    0xff520df7, 0x13274c18, 0x5542dd93, 0x0776181b, 0xf94a1808, 0x084a4c0c, 0x4308ea5b, 0xde831150, 0x7900fd21, 0x00492c1e, 0x060f4510, 0x17410020, 
    0x0ce74526, 0x6206b341, 0x1f561083, 0x9d6c181b, 0x08a0500e, 0x112e4118, 0x60000421, 0xbf901202, 0x4408e241, 0xc7ab0513, 0xb40f0950, 0x055943c7, 
    0x4f18ff20, 0xc9ae1cad, 0x32b34f18, 0x7a180120, 0x3d520a05, 0x53d1b40a, 0x80200813, 0x1b815018, 0x832bf86f, 0x67731847, 0x297f4308, 0x6418d54e, 
    0x734213f7, 0x056b4b27, 0xdba5fe20, 0x1828aa4e, 0x2031a370, 0x06cb6101, 0x2040ad41, 0x07365300, 0x2558d985, 0x83fe200c, 0x0380211c, 0x542c4743, 
    0x052006b7, 0x6021df45, 0x897b0707, 0x18d3c010, 0x20090e70, 0x1d5843ff, 0x540a0e44, 0x002126c5, 0x322f7416, 0x636a5720, 0x0f317409, 0x610fe159, 
    0x294617e7, 0x08555213, 0x2006a75d, 0x6cec84fd, 0xfb5907be, 0x3a317405, 0x83808021, 0x180f20ea, 0x4626434a, 0x531818e3, 0xdb59172d, 0x0cbb460c, 
    0x2013d859, 0x18b94502, 0x8f46188d, 0x77521842, 0x0a184e38, 0x9585fd20, 0x6a180684, 0xc64507e9, 0x51cbb230, 0xd3440cf3, 0x17ff6a0f, 0x450f5b42, 
    0x276407c1, 0x4853180a, 0x21ccb010, 0xcf580013, 0x0c15442d, 0x410a1144, 0x1144359d, 0x5cfe2006, 0xa1410a43, 0x2bb64519, 0x2f5b7618, 0xb512b745, 
    0x0cfd6fd1, 0x42089f59, 0xb8450c70, 0x0000232d, 0x50180900, 0xb9491ae3, 0x0fc37610, 0x01210f83, 0x0f3b4100, 0xa01b2742, 0x0ccd426f, 0x6e8f6f94, 
    0x9c808021, 0xc7511870, 0x17c74b08, 0x9b147542, 0x44fe2079, 0xd5480c7e, 0x95ef861d, 0x101b597b, 0xf5417594, 0x9f471808, 0x86868d0e, 0x3733491c, 
    0x690f4d6d, 0x43440b83, 0x1ba94c0b, 0x660cd16b, 0x802008ae, 0x74126448, 0xcb4f38a3, 0x2cb74b0b, 0x47137755, 0xe3971777, 0x1b5d0120, 0x057a4108, 
    0x6e08664d, 0x17421478, 0x11af4208, 0x850c3f42, 0x08234f0c, 0x4321eb4a, 0xf3451095, 0x0f394e0f, 0x4310eb45, 0xc09707b1, 0x54431782, 0xaec08d1d, 
    0x0f434dbb, 0x9f0c0b45, 0x0a3b4dbb, 0x4618bdc7, 0x536032eb, 0x17354213, 0x4d134169, 0xc7a30c2f, 0x4e254342, 0x174332cf, 0x43cdae17, 0x6b4706e4, 
    0x0e16430d, 0x530b5542, 0x2f7c26bb, 0x13075f31, 0x43175342, 0x60181317, 0x6550114e, 0x28624710, 0x58070021, 0x59181683, 0x2d540cf5, 0x05d5660c, 
    0x20090c7b, 0x0e157e02, 0x8000ff2b, 0x14000080, 0x80ff8000, 0x27137e03, 0x336a4b20, 0x0f817107, 0x13876e18, 0x730f2f7e, 0x2f450b75, 0x6d02200b, 
    0x6d66094c, 0x4b802009, 0x15820a02, 0x2f45fe20, 0x5e032006, 0x00202fd9, 0x450af741, 0xeb412e0f, 0x0ff3411f, 0x420a8b65, 0xf7410eae, 0x1c664810, 
    0x540e1145, 0xbfa509f3, 0x42302f58, 0x80200c35, 0xcb066c47, 0x4b1120c1, 0x41492abb, 0x34854110, 0xa7097b72, 0x251545c7, 0x4b2c7f56, 0xc5b40bab, 
    0x940cd54e, 0x2e6151c8, 0x09f35f18, 0x4b420420, 0x09677121, 0x8f24f357, 0x1b5418e1, 0x08915a1f, 0x3143d894, 0x22541805, 0x1b9b4b0e, 0x8c0d3443, 
    0x1400240d, 0x18ff8000, 0x582e6387, 0xf99b2b3b, 0x8807a550, 0x17a14790, 0x2184fd20, 0x5758fe20, 0x2354882c, 0x15000080, 0x5e056751, 0x334c2c2f, 
    0x97c58f0c, 0x1fd7410f, 0x0d4d4018, 0x4114dc41, 0x04470ed6, 0x0dd54128, 0x00820020, 0x02011523, 0x22008700, 0x86480024, 0x0001240a, 0x8682001a, 
    0x0002240b, 0x866c000e, 0x8a03200b, 0x8a042017, 0x0005220b, 0x22218614, 0x84060000, 0x86012017, 0x8212200f, 0x250b8519, 0x000d0001, 0x0b850031, 
    0x07000224, 0x0b862600, 0x11000324, 0x0b862d00, 0x238a0420, 0x0a000524, 0x17863e00, 0x17840620, 0x01000324, 0x57820904, 0x0b85a783, 0x0b85a785, 
    0x0b85a785, 0x22000325, 0x85007a00, 0x85a7850b, 0x85a7850b, 0x22a7850b, 0x82300032, 0x00342201, 0x0805862f, 0x35003131, 0x54207962, 0x74736972, 
    0x47206e61, 0x6d6d6972, 0x65527265, 0x616c7567, 0x58545472, 0x6f725020, 0x43796767, 0x6e61656c, 0x30325454, 0x822f3430, 0x35313502, 0x79006200, 
    0x54002000, 0x69007200, 0x74007300, 0x6e006100, 0x47200f82, 0x6d240f84, 0x65006d00, 0x52200982, 0x67240582, 0x6c007500, 0x72201d82, 0x54222b82, 
    0x23825800, 0x19825020, 0x67006f22, 0x79220182, 0x1b824300, 0x3b846520, 0x1f825420, 0x41000021, 0x1422099b, 0x0b410000, 0x87088206, 0x01012102, 
    0x78080982, 0x01020101, 0x01040103, 0x01060105, 0x01080107, 0x010a0109, 0x010c010b, 0x010e010d, 0x0110010f, 0x01120111, 0x01140113, 0x01160115, 
    0x01180117, 0x011a0119, 0x011c011b, 0x011e011d, 0x0020011f, 0x00040003, 0x00060005, 0x00080007, 0x000a0009, 0x000c000b, 0x000e000d, 0x0010000f, 
    0x00120011, 0x00140013, 0x00160015, 0x00180017, 0x001a0019, 0x001c001b, 0x001e001d, 0x08bb821f, 0x22002142, 0x24002300, 0x26002500, 0x28002700, 
    0x2a002900, 0x2c002b00, 0x2e002d00, 0x30002f00, 0x32003100, 0x34003300, 0x36003500, 0x38003700, 0x3a003900, 0x3c003b00, 0x3e003d00, 0x40003f00, 
    0x42004100, 0x4b09f382, 0x00450044, 0x00470046, 0x00490048, 0x004b004a, 0x004d004c, 0x004f004e, 0x00510050, 0x00530052, 0x00550054, 0x00570056, 
    0x00590058, 0x005b005a, 0x005d005c, 0x005f005e, 0x01610060, 0x01220121, 0x01240123, 0x01260125, 0x01280127, 0x012a0129, 0x012c012b, 0x012e012d, 
    0x0130012f, 0x01320131, 0x01340133, 0x01360135, 0x01380137, 0x013a0139, 0x013c013b, 0x013e013d, 0x0140013f, 0x00ac0041, 0x008400a3, 0x00bd0085, 
    0x00e80096, 0x008e0086, 0x009d008b, 0x00a400a9, 0x008a00ef, 0x008300da, 0x00f20093, 0x008d00f3, 0x00880097, 0x00de00c3, 0x009e00f1, 0x00f500aa, 
    0x00f600f4, 0x00ad00a2, 0x00c700c9, 0x006200ae, 0x00900063, 0x00cb0064, 0x00c80065, 0x00cf00ca, 0x00cd00cc, 0x00e900ce, 0x00d30066, 0x00d100d0, 
    0x006700af, 0x009100f0, 0x00d400d6, 0x006800d5, 0x00ed00eb, 0x006a0089, 0x006b0069, 0x006c006d, 0x00a0006e, 0x0071006f, 0x00720070, 0x00750073, 
    0x00760074, 0x00ea0077, 0x007a0078, 0x007b0079, 0x007c007d, 0x00a100b8, 0x007e007f, 0x00810080, 0x00ee00ec, 0x6e750eba, 0x646f6369, 0x78302365, 
    0x31303030, 0x32200e8d, 0x33200e8d, 0x34200e8d, 0x35200e8d, 0x36200e8d, 0x37200e8d, 0x38200e8d, 0x39200e8d, 0x61200e8d, 0x62200e8d, 0x63200e8d, 
    0x64200e8d, 0x65200e8d, 0x66200e8d, 0x31210e8c, 0x8d0e8d30, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 
    0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x66312def, 0x6c656406, 0x04657465, 0x6f727545, 0x3820ec8c, 0x3820ec8d, 
    0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 
    0x3820ec8d, 0x200ddc41, 0x0ddc4139, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 
    0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0x00663923, 0x48fa0500, 0x00f762f9, 
};

static void GetDefaultCompressedFontDataTTF(const void** ttf_compressed_data, unsigned int* ttf_compressed_size)
{
    *ttf_compressed_data = proggy_clean_ttf_compressed_data;
    *ttf_compressed_size = proggy_clean_ttf_compressed_size;
}

//-----------------------------------------------------------------------------

//---- Include imgui_user.inl at the end of imgui.cpp
//---- So you can include code that extends ImGui using its private data/functions.
#ifdef IMGUI_INCLUDE_IMGUI_USER_INL
#include "imgui_user.inl"
#endif

//-----------------------------------------------------------------------------
