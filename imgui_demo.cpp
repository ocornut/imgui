// dear imgui, v1.90.1 WIP
// (demo code)

// Help:
// - Read FAQ at http://dearimgui.com/faq
// - Call and read ImGui::ShowDemoWindow() in imgui_demo.cpp. All applications in examples/ are doing that.
// - Need help integrating Dear ImGui in your codebase?
//   - Read Getting Started https://github.com/ocornut/imgui/wiki/Getting-Started
//   - Read 'Programmer guide' in imgui.cpp for notes on how to setup Dear ImGui in your codebase.
// Read imgui.cpp for more details, documentation and comments.
// Get the latest version at https://github.com/ocornut/imgui

//---------------------------------------------------
// PLEASE DO NOT REMOVE THIS FILE FROM YOUR PROJECT!
//---------------------------------------------------
// Message to the person tempted to delete this file when integrating Dear ImGui into their codebase:
// Think again! It is the most useful reference code that you and other coders will want to refer to and call.
// Have the ImGui::ShowDemoWindow() function wired in an always-available debug menu of your game/app!
// Also include Metrics! ItemPicker! DebugLog! and other debug features.
// Removing this file from your project is hindering access to documentation for everyone in your team,
// likely leading you to poorer usage of the library.
// Everything in this file will be stripped out by the linker if you don't call ImGui::ShowDemoWindow().
// If you want to link core Dear ImGui in your shipped builds but want a thorough guarantee that the demo will not be
// linked, you can setup your imconfig.h with #define IMGUI_DISABLE_DEMO_WINDOWS and those functions will be empty.
// In another situation, whenever you have Dear ImGui available you probably want this to be available for reference.
// Thank you,
// -Your beloved friend, imgui_demo.cpp (which you won't delete)

//--------------------------------------------
// ABOUT THE MEANING OF THE 'static' KEYWORD:
//--------------------------------------------
// In this demo code, we frequently use 'static' variables inside functions.
// A static variable persists across calls. It is essentially a global variable but declared inside the scope of the function.
// Think of "static int n = 0;" as "global int n = 0;" !
// We do this IN THE DEMO because we want:
// - to gather code and data in the same place.
// - to make the demo source code faster to read, faster to change, smaller in size.
// - it is also a convenient way of storing simple UI related information as long as your function
//   doesn't need to be reentrant or used in multiple threads.
// This might be a pattern you will want to use in your code, but most of the data you would be working
// with in a complex codebase is likely going to be stored outside your functions.

//-----------------------------------------
// ABOUT THE CODING STYLE OF OUR DEMO CODE
//-----------------------------------------
// The Demo code in this file is designed to be easy to copy-and-paste into your application!
// Because of this:
// - We never omit the ImGui:: prefix when calling functions, even though most code here is in the same namespace.
// - We try to declare static variables in the local scope, as close as possible to the code using them.
// - We never use any of the helpers/facilities used internally by Dear ImGui, unless available in the public API.
// - We never use maths operators on ImVec2/ImVec4. For our other sources files we use them, and they are provided
//   by imgui.h using the IMGUI_DEFINE_MATH_OPERATORS define. For your own sources file they are optional
//   and require you either enable those, either provide your own via IM_VEC2_CLASS_EXTRA in imconfig.h.
//   Because we can't assume anything about your support of maths operators, we cannot use them in imgui_demo.cpp.

// Navigating this file:
// - In Visual Studio IDE: CTRL+comma ("Edit.GoToAll") can follow symbols in comments, whereas CTRL+F12 ("Edit.GoToImplementation") cannot.
// - With Visual Assist installed: ALT+G ("VAssistX.GoToImplementation") can also follow symbols in comments.

/*

Index of this file:

// [SECTION] Forward Declarations
// [SECTION] Helpers
// [SECTION] Demo Window / ShowDemoWindow()
// - ShowDemoWindow()
// - sub section: ShowDemoWindowWidgets()
// - sub section: ShowDemoWindowLayout()
// - sub section: ShowDemoWindowPopups()
// - sub section: ShowDemoWindowTables()
// - sub section: ShowDemoWindowInputs()
// [SECTION] About Window / ShowAboutWindow()
// [SECTION] Style Editor / ShowStyleEditor()
// [SECTION] User Guide / ShowUserGuide()
// [SECTION] Example App: Main Menu Bar / ShowExampleAppMainMenuBar()
// [SECTION] Example App: Debug Console / ShowExampleAppConsole()
// [SECTION] Example App: Debug Log / ShowExampleAppLog()
// [SECTION] Example App: Simple Layout / ShowExampleAppLayout()
// [SECTION] Example App: Property Editor / ShowExampleAppPropertyEditor()
// [SECTION] Example App: Long Text / ShowExampleAppLongText()
// [SECTION] Example App: Auto Resize / ShowExampleAppAutoResize()
// [SECTION] Example App: Constrained Resize / ShowExampleAppConstrainedResize()
// [SECTION] Example App: Simple overlay / ShowExampleAppSimpleOverlay()
// [SECTION] Example App: Fullscreen window / ShowExampleAppFullscreen()
// [SECTION] Example App: Manipulating window titles / ShowExampleAppWindowTitles()
// [SECTION] Example App: Custom Rendering using ImDrawList API / ShowExampleAppCustomRendering()
// [SECTION] Example App: Documents Handling / ShowExampleAppDocuments()

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE

// System includes
#include <ctype.h>          // toupper
#include <limits.h>         // INT_MIN, INT_MAX
#include <math.h>           // sqrtf, powf, cosf, sinf, floorf, ceilf
#include <stdio.h>          // vsnprintf, sscanf, printf
#include <stdlib.h>         // NULL, malloc, free, atoi
#include <stdint.h>         // intptr_t
#if !defined(_MSC_VER) || _MSC_VER >= 1800
#include <inttypes.h>       // PRId64/PRIu64, not avail in some MinGW headers.
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)     // condition expression is constant
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#pragma warning (disable: 26451)    // [Static Analyzer] Arithmetic overflow : Using operator 'xxx' on a 4 byte value and then casting the result to an 8 byte value. Cast the value to the wider type before calling operator 'xxx' to avoid overflow(io.2).
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                     // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                           // yes, they are more terse.
#pragma clang diagnostic ignored "-Wdeprecated-declarations"        // warning: 'xx' is deprecated: The POSIX name for this..   // for strdup used in demo code (so user can copy & paste the code)
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"       // warning: cast to 'void *' from smaller integer type
#pragma clang diagnostic ignored "-Wformat-security"                // warning: format string is not a string literal
#pragma clang diagnostic ignored "-Wexit-time-destructors"          // warning: declaration requires an exit-time destructor    // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. ImGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wunused-macros"                  // warning: macro is not used                               // we define snprintf/vsnprintf on Windows so they are available, but not always used.
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                   // some standard header variations use #define NULL 0
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wreserved-id-macro"              // warning: macro name is a reserved identifier
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                  // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"      // warning: cast to pointer from integer of different size
#pragma GCC diagnostic ignored "-Wformat-security"          // warning: format string is not a string literal (potentially insecure)
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wmisleading-indentation"   // [__GNUC__ >= 6] warning: this 'if' clause does not guard this statement      // GCC 6.0+ only. See #883 on GitHub.
#endif

// Play it nice with Windows users (Update: May 2018, Notepad now supports Unix-style carriage returns!)
#ifdef _WIN32
#define IM_NEWLINE  "\r\n"
#else
#define IM_NEWLINE  "\n"
#endif

// Helpers
#if defined(_MSC_VER) && !defined(snprintf)
#define snprintf    _snprintf
#endif
#if defined(_MSC_VER) && !defined(vsnprintf)
#define vsnprintf   _vsnprintf
#endif

// Format specifiers for 64-bit values (hasn't been decently standardized before VS2013)
#if !defined(PRId64) && defined(_MSC_VER)
#define PRId64 "I64d"
#define PRIu64 "I64u"
#elif !defined(PRId64)
#define PRId64 "lld"
#define PRIu64 "llu"
#endif

// Helpers macros
// We normally try to not use many helpers in imgui_demo.cpp in order to make code easier to copy and paste,
// but making an exception here as those are largely simplifying code...
// In other imgui sources we can use nicer internal functions from imgui_internal.h (ImMin/ImMax) but not in the demo.
#define IM_MIN(A, B)            (((A) < (B)) ? (A) : (B))
#define IM_MAX(A, B)            (((A) >= (B)) ? (A) : (B))
#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

// Enforce cdecl calling convention for functions called by the standard library, in case compilation settings changed the default to e.g. __vectorcall
#ifndef IMGUI_CDECL
#ifdef _MSC_VER
#define IMGUI_CDECL __cdecl
#else
#define IMGUI_CDECL
#endif
#endif

//-----------------------------------------------------------------------------
// [SECTION] Forward Declarations, Helpers
//-----------------------------------------------------------------------------

#if !defined(IMGUI_DISABLE_DEMO_WINDOWS)

// Forward Declarations
static void ShowExampleAppMainMenuBar(ImGuiContext* ctx);
static void ShowExampleAppConsole(ImGuiContext* ctx, bool* p_open);
static void ShowExampleAppCustomRendering(ImGuiContext* ctx, bool* p_open);
static void ShowExampleAppDocuments(ImGuiContext* ctx, bool* p_open);
static void ShowExampleAppLog(ImGuiContext* ctx, bool* p_open);
static void ShowExampleAppLayout(ImGuiContext* ctx, bool* p_open);
static void ShowExampleAppPropertyEditor(ImGuiContext* ctx, bool* p_open);
static void ShowExampleAppSimpleOverlay(ImGuiContext* ctx, bool* p_open);
static void ShowExampleAppAutoResize(ImGuiContext* ctx, bool* p_open);
static void ShowExampleAppConstrainedResize(ImGuiContext* ctx, bool* p_open);
static void ShowExampleAppFullscreen(ImGuiContext* ctx, bool* p_open);
static void ShowExampleAppLongText(ImGuiContext* ctx, bool* p_open);
static void ShowExampleAppWindowTitles(ImGuiContext* ctx, bool* p_open);
static void ShowExampleMenuFile(ImGuiContext* ctx);

// We split the contents of the big ShowDemoWindow() function into smaller functions
// (because the link time of very large functions grow non-linearly)
static void ShowDemoWindowWidgets(ImGuiContext* ctx);
static void ShowDemoWindowLayout(ImGuiContext* ctx);
static void ShowDemoWindowPopups(ImGuiContext* ctx);
static void ShowDemoWindowTables(ImGuiContext* ctx);
static void ShowDemoWindowColumns(ImGuiContext* ctx);
static void ShowDemoWindowInputs(ImGuiContext* ctx);

//-----------------------------------------------------------------------------
// [SECTION] Helpers
//-----------------------------------------------------------------------------

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(ImGuiContext* ctx, const char* desc)
{
    ImGui::TextDisabled(ctx, "(?)");
    if (ImGui::BeginItemTooltip(ctx))
    {
        ImGui::PushTextWrapPos(ctx, ImGui::GetFontSize(ctx) * 35.0f);
        ImGui::TextUnformatted(ctx, desc);
        ImGui::PopTextWrapPos(ctx);
        ImGui::EndTooltip(ctx);
    }
}

// Helper to wire demo markers located in code to an interactive browser
typedef void (*ImGuiDemoMarkerCallback)(const char* file, int line, const char* section, void* user_data);
extern ImGuiDemoMarkerCallback      GImGuiDemoMarkerCallback;
extern void*                        GImGuiDemoMarkerCallbackUserData;
ImGuiDemoMarkerCallback             GImGuiDemoMarkerCallback = NULL;
void*                               GImGuiDemoMarkerCallbackUserData = NULL;
#define IMGUI_DEMO_MARKER(section)  do { if (GImGuiDemoMarkerCallback != NULL) GImGuiDemoMarkerCallback(__FILE__, __LINE__, section, GImGuiDemoMarkerCallbackUserData); } while (0)

//-----------------------------------------------------------------------------
// [SECTION] Demo Window / ShowDemoWindow()
//-----------------------------------------------------------------------------
// - ShowDemoWindow()
// - ShowDemoWindowWidgets()
// - ShowDemoWindowLayout()
// - ShowDemoWindowPopups()
// - ShowDemoWindowTables()
// - ShowDemoWindowColumns()
// - ShowDemoWindowInputs()
//-----------------------------------------------------------------------------

// Demonstrate most Dear ImGui features (this is big function!)
// You may execute this function to experiment with the UI and understand what it does.
// You may then search for keywords in the code when you are interested by a specific feature.
void ImGui::ShowDemoWindow(ImGuiContext* ctx, bool* p_open)
{
    // Examples Apps (accessible from the "Examples" menu)
    static bool show_app_main_menu_bar = false;
    static bool show_app_console = false;
    static bool show_app_custom_rendering = false;
    static bool show_app_documents = false;
    static bool show_app_log = false;
    static bool show_app_layout = false;
    static bool show_app_property_editor = false;
    static bool show_app_simple_overlay = false;
    static bool show_app_auto_resize = false;
    static bool show_app_constrained_resize = false;
    static bool show_app_fullscreen = false;
    static bool show_app_long_text = false;
    static bool show_app_window_titles = false;

    if (show_app_main_menu_bar)       ShowExampleAppMainMenuBar(ctx);
    if (show_app_documents)           ShowExampleAppDocuments(ctx, &show_app_documents);
    if (show_app_console)             ShowExampleAppConsole(ctx, &show_app_console);
    if (show_app_custom_rendering)    ShowExampleAppCustomRendering(ctx, &show_app_custom_rendering);
    if (show_app_log)                 ShowExampleAppLog(ctx, &show_app_log);
    if (show_app_layout)              ShowExampleAppLayout(ctx, &show_app_layout);
    if (show_app_property_editor)     ShowExampleAppPropertyEditor(ctx, &show_app_property_editor);
    if (show_app_simple_overlay)      ShowExampleAppSimpleOverlay(ctx, &show_app_simple_overlay);
    if (show_app_auto_resize)         ShowExampleAppAutoResize(ctx, &show_app_auto_resize);
    if (show_app_constrained_resize)  ShowExampleAppConstrainedResize(ctx, &show_app_constrained_resize);
    if (show_app_fullscreen)          ShowExampleAppFullscreen(ctx, &show_app_fullscreen);
    if (show_app_long_text)           ShowExampleAppLongText(ctx, &show_app_long_text);
    if (show_app_window_titles)       ShowExampleAppWindowTitles(ctx, &show_app_window_titles);

    // Dear ImGui Tools (accessible from the "Tools" menu)
    static bool show_tool_metrics = false;
    static bool show_tool_debug_log = false;
    static bool show_tool_id_stack_tool = false;
    static bool show_tool_style_editor = false;
    static bool show_tool_about = false;

    if (show_tool_metrics)
        ImGui::ShowMetricsWindow(ctx, &show_tool_metrics);
    if (show_tool_debug_log)
        ImGui::ShowDebugLogWindow(ctx, &show_tool_debug_log);
    if (show_tool_id_stack_tool)
        ImGui::ShowIDStackToolWindow(ctx, &show_tool_id_stack_tool);
    if (show_tool_style_editor)
    {
        ImGui::Begin(ctx, "Dear ImGui Style Editor", &show_tool_style_editor);
        ImGui::ShowStyleEditor(ctx);
        ImGui::End(ctx);
    }
    if (show_tool_about)
        ImGui::ShowAboutWindow(ctx, &show_tool_about);

    // Demonstrate the various window flags. Typically you would just use the default!
    static bool no_titlebar = false;
    static bool no_scrollbar = false;
    static bool no_menu = false;
    static bool no_move = false;
    static bool no_resize = false;
    static bool no_collapse = false;
    static bool no_close = false;
    static bool no_nav = false;
    static bool no_background = false;
    static bool no_bring_to_front = false;
    static bool unsaved_document = false;

    ImGuiWindowFlags window_flags = 0;
    if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
    if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
    if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
    if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
    if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
    if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
    if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
    if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
    if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (unsaved_document)   window_flags |= ImGuiWindowFlags_UnsavedDocument;
    if (no_close)           p_open = NULL; // Don't pass our bool* to Begin

    // We specify a default position/size in case there's no data in the .ini file.
    // We only do it to make the demo applications a little more welcoming, but typically this isn't required.
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport(ctx);
    ImGui::SetNextWindowPos(ctx, ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ctx, ImVec2(550, 680), ImGuiCond_FirstUseEver);

    // Main body of the Demo window starts here.
    if (!ImGui::Begin(ctx, "Dear ImGui Demo", p_open, window_flags))
    {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End(ctx);
        return;
    }

    // Most "big" widgets share a common width settings by default. See 'Demo->Layout->Widgets Width' for details.
    // e.g. Use 2/3 of the space for widgets and 1/3 for labels (right align)
    //ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.35f);
    // e.g. Leave a fixed amount of width for labels (by passing a negative value), the rest goes to widgets.
    ImGui::PushItemWidth(ctx, ImGui::GetFontSize(ctx) * -12);

    // Menu Bar
    if (ImGui::BeginMenuBar(ctx))
    {
        if (ImGui::BeginMenu(ctx, "Menu"))
        {
            IMGUI_DEMO_MARKER("Menu/File");
            ShowExampleMenuFile(ctx);
            ImGui::EndMenu(ctx);
        }
        if (ImGui::BeginMenu(ctx, "Examples"))
        {
            IMGUI_DEMO_MARKER("Menu/Examples");
            ImGui::MenuItem(ctx, "Main menu bar", NULL, &show_app_main_menu_bar);

            ImGui::SeparatorText(ctx, "Mini apps");
            ImGui::MenuItem(ctx, "Console", NULL, &show_app_console);
            ImGui::MenuItem(ctx, "Custom rendering", NULL, &show_app_custom_rendering);
            ImGui::MenuItem(ctx, "Documents", NULL, &show_app_documents);
            ImGui::MenuItem(ctx, "Log", NULL, &show_app_log);
            ImGui::MenuItem(ctx, "Property editor", NULL, &show_app_property_editor);
            ImGui::MenuItem(ctx, "Simple layout", NULL, &show_app_layout);
            ImGui::MenuItem(ctx, "Simple overlay", NULL, &show_app_simple_overlay);

            ImGui::SeparatorText(ctx, "Concepts");
            ImGui::MenuItem(ctx, "Auto-resizing window", NULL, &show_app_auto_resize);
            ImGui::MenuItem(ctx, "Constrained-resizing window", NULL, &show_app_constrained_resize);
            ImGui::MenuItem(ctx, "Fullscreen window", NULL, &show_app_fullscreen);
            ImGui::MenuItem(ctx, "Long text display", NULL, &show_app_long_text);
            ImGui::MenuItem(ctx, "Manipulating window titles", NULL, &show_app_window_titles);

            ImGui::EndMenu(ctx);
        }
        //if (ImGui::MenuItem("MenuItem")) {} // You can also use MenuItem() inside a menu bar!
        if (ImGui::BeginMenu(ctx, "Tools"))
        {
            IMGUI_DEMO_MARKER("Menu/Tools");
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
            const bool has_debug_tools = true;
#else
            const bool has_debug_tools = false;
#endif
            ImGui::MenuItem(ctx, "Metrics/Debugger", NULL, &show_tool_metrics, has_debug_tools);
            ImGui::MenuItem(ctx, "Debug Log", NULL, &show_tool_debug_log, has_debug_tools);
            ImGui::MenuItem(ctx, "ID Stack Tool", NULL, &show_tool_id_stack_tool, has_debug_tools);
            ImGui::MenuItem(ctx, "Style Editor", NULL, &show_tool_style_editor);
            ImGui::MenuItem(ctx, "About Dear ImGui", NULL, &show_tool_about);
            ImGui::EndMenu(ctx);
        }
        ImGui::EndMenuBar(ctx);
    }

    ImGui::Text(ctx, "dear imgui says hello! (%s) (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
    ImGui::Spacing(ctx);

    IMGUI_DEMO_MARKER("Help");
    if (ImGui::CollapsingHeader(ctx, "Help"))
    {
        ImGui::SeparatorText(ctx, "ABOUT THIS DEMO:");
        ImGui::BulletText(ctx, "Sections below are demonstrating many aspects of the library.");
        ImGui::BulletText(ctx, "The \"Examples\" menu above leads to more demo contents.");
        ImGui::BulletText(ctx, "The \"Tools\" menu above gives access to: About Box, Style Editor,\n"
                          "and Metrics/Debugger (general purpose Dear ImGui debugging tool).");

        ImGui::SeparatorText(ctx, "PROGRAMMER GUIDE:");
        ImGui::BulletText(ctx, "See the ShowDemoWindow() code in imgui_demo.cpp. <- you are here!");
        ImGui::BulletText(ctx, "See comments in imgui.cpp.");
        ImGui::BulletText(ctx, "See example applications in the examples/ folder.");
        ImGui::BulletText(ctx, "Read the FAQ at https://www.dearimgui.com/faq/");
        ImGui::BulletText(ctx, "Set 'io.ConfigFlags |= NavEnableKeyboard' for keyboard controls.");
        ImGui::BulletText(ctx, "Set 'io.ConfigFlags |= NavEnableGamepad' for gamepad controls.");

        ImGui::SeparatorText(ctx, "USER GUIDE:");
        ImGui::ShowUserGuide(ctx);
    }

    IMGUI_DEMO_MARKER("Configuration");
    if (ImGui::CollapsingHeader(ctx, "Configuration"))
    {
        ImGuiIO& io = ImGui::GetIO(ctx);

        if (ImGui::TreeNode(ctx, "Configuration##2"))
        {
            ImGui::SeparatorText(ctx, "General");
            ImGui::CheckboxFlags(ctx, "io.ConfigFlags: NavEnableKeyboard",    &io.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Enable keyboard controls.");
            ImGui::CheckboxFlags(ctx, "io.ConfigFlags: NavEnableGamepad",     &io.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Enable gamepad controls. Require backend to set io.BackendFlags |= ImGuiBackendFlags_HasGamepad.\n\nRead instructions in imgui.cpp for details.");
            ImGui::CheckboxFlags(ctx, "io.ConfigFlags: NavEnableSetMousePos", &io.ConfigFlags, ImGuiConfigFlags_NavEnableSetMousePos);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Instruct navigation to move the mouse cursor. See comment for ImGuiConfigFlags_NavEnableSetMousePos.");
            ImGui::CheckboxFlags(ctx, "io.ConfigFlags: NoMouse",              &io.ConfigFlags, ImGuiConfigFlags_NoMouse);
            if (io.ConfigFlags & ImGuiConfigFlags_NoMouse)
            {
                // The "NoMouse" option can get us stuck with a disabled mouse! Let's provide an alternative way to fix it:
                if (fmodf((float)ImGui::GetTime(ctx), 0.40f) < 0.20f)
                {
                    ImGui::SameLine(ctx);
                    ImGui::Text(ctx, "<<PRESS SPACE TO DISABLE>>");
                }
                if (ImGui::IsKeyPressed(ctx, ImGuiKey_Space))
                    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            }
            ImGui::CheckboxFlags(ctx, "io.ConfigFlags: NoMouseCursorChange", &io.ConfigFlags, ImGuiConfigFlags_NoMouseCursorChange);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Instruct backend to not alter mouse cursor shape and visibility.");
            ImGui::Checkbox(ctx, "io.ConfigInputTrickleEventQueue", &io.ConfigInputTrickleEventQueue);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Enable input queue trickling: some types of events submitted during the same frame (e.g. button down + up) will be spread over multiple frames, improving interactions with low framerates.");
            ImGui::Checkbox(ctx, "io.MouseDrawCursor", &io.MouseDrawCursor);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Instruct Dear ImGui to render a mouse cursor itself. Note that a mouse cursor rendered via your application GPU rendering path will feel more laggy than hardware cursor, but will be more in sync with your other visuals.\n\nSome desktop applications may use both kinds of cursors (e.g. enable software cursor only when resizing/dragging something).");

            ImGui::SeparatorText(ctx, "Widgets");
            ImGui::Checkbox(ctx, "io.ConfigInputTextCursorBlink", &io.ConfigInputTextCursorBlink);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Enable blinking cursor (optional as some users consider it to be distracting).");
            ImGui::Checkbox(ctx, "io.ConfigInputTextEnterKeepActive", &io.ConfigInputTextEnterKeepActive);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Pressing Enter will keep item active and select contents (single-line only).");
            ImGui::Checkbox(ctx, "io.ConfigDragClickToInputText", &io.ConfigDragClickToInputText);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Enable turning DragXXX widgets into text input with a simple mouse click-release (without moving).");
            ImGui::Checkbox(ctx, "io.ConfigWindowsResizeFromEdges", &io.ConfigWindowsResizeFromEdges);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Enable resizing of windows from their edges and from the lower-left corner.\nThis requires (io.BackendFlags & ImGuiBackendFlags_HasMouseCursors) because it needs mouse cursor feedback.");
            ImGui::Checkbox(ctx, "io.ConfigWindowsMoveFromTitleBarOnly", &io.ConfigWindowsMoveFromTitleBarOnly);
            ImGui::Checkbox(ctx, "io.ConfigMacOSXBehaviors", &io.ConfigMacOSXBehaviors);
            ImGui::Text(ctx, "Also see Style->Rendering for rendering options.");

            ImGui::SeparatorText(ctx, "Debug");
            ImGui::BeginDisabled(ctx);
            ImGui::Checkbox(ctx, "io.ConfigDebugBeginReturnValueOnce", &io.ConfigDebugBeginReturnValueOnce); // .
            ImGui::EndDisabled(ctx);
            ImGui::SameLine(ctx); HelpMarker(ctx, "First calls to Begin()/BeginChild() will return false.\n\nTHIS OPTION IS DISABLED because it needs to be set at application boot-time to make sense. Showing the disabled option is a way to make this feature easier to discover");
            ImGui::Checkbox(ctx, "io.ConfigDebugBeginReturnValueLoop", &io.ConfigDebugBeginReturnValueLoop);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Some calls to Begin()/BeginChild() will return false.\n\nWill cycle through window depths then repeat. Windows should be flickering while running.");
            ImGui::Checkbox(ctx, "io.ConfigDebugIgnoreFocusLoss", &io.ConfigDebugIgnoreFocusLoss);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Option to deactivate io.AddFocusEvent(false) handling. May facilitate interactions with a debugger when focus loss leads to clearing inputs data.");
            ImGui::Checkbox(ctx, "io.ConfigDebugIniSettings", &io.ConfigDebugIniSettings);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Option to save .ini data with extra comments (particularly helpful for Docking, but makes saving slower).");

            ImGui::TreePop(ctx);
            ImGui::Spacing(ctx);
        }

        IMGUI_DEMO_MARKER("Configuration/Backend Flags");
        if (ImGui::TreeNode(ctx, "Backend Flags"))
        {
            HelpMarker(ctx, 
                "Those flags are set by the backends (imgui_impl_xxx files) to specify their capabilities.\n"
                "Here we expose them as read-only fields to avoid breaking interactions with your backend.");

            // FIXME: Maybe we need a BeginReadonly() equivalent to keep label bright?
            ImGui::BeginDisabled(ctx);
            ImGui::CheckboxFlags(ctx, "io.BackendFlags: HasGamepad",           &io.BackendFlags, ImGuiBackendFlags_HasGamepad);
            ImGui::CheckboxFlags(ctx, "io.BackendFlags: HasMouseCursors",      &io.BackendFlags, ImGuiBackendFlags_HasMouseCursors);
            ImGui::CheckboxFlags(ctx, "io.BackendFlags: HasSetMousePos",       &io.BackendFlags, ImGuiBackendFlags_HasSetMousePos);
            ImGui::CheckboxFlags(ctx, "io.BackendFlags: RendererHasVtxOffset", &io.BackendFlags, ImGuiBackendFlags_RendererHasVtxOffset);
            ImGui::EndDisabled(ctx);
            ImGui::TreePop(ctx);
            ImGui::Spacing(ctx);
        }

        IMGUI_DEMO_MARKER("Configuration/Style");
        if (ImGui::TreeNode(ctx, "Style"))
        {
            HelpMarker(ctx, "The same contents can be accessed in 'Tools->Style Editor' or by calling the ShowStyleEditor() function.");
            ImGui::ShowStyleEditor(ctx);
            ImGui::TreePop(ctx);
            ImGui::Spacing(ctx);
        }

        IMGUI_DEMO_MARKER("Configuration/Capture, Logging");
        if (ImGui::TreeNode(ctx, "Capture/Logging"))
        {
            HelpMarker(ctx, 
                "The logging API redirects all text output so you can easily capture the content of "
                "a window or a block. Tree nodes can be automatically expanded.\n"
                "Try opening any of the contents below in this window and then click one of the \"Log To\" button.");
            ImGui::LogButtons(ctx);

            HelpMarker(ctx, "You can also call ImGui::LogText() to output directly to the log without a visual output.");
            if (ImGui::Button(ctx, "Copy \"Hello, world!\" to clipboard"))
            {
                ImGui::LogToClipboard(ctx);
                ImGui::LogText(ctx, "Hello, world!");
                ImGui::LogFinish(ctx);
            }
            ImGui::TreePop(ctx);
        }
    }

    IMGUI_DEMO_MARKER("Window options");
    if (ImGui::CollapsingHeader(ctx, "Window options"))
    {
        if (ImGui::BeginTable(ctx, "split", 3))
        {
            ImGui::TableNextColumn(ctx); ImGui::Checkbox(ctx, "No titlebar", &no_titlebar);
            ImGui::TableNextColumn(ctx); ImGui::Checkbox(ctx, "No scrollbar", &no_scrollbar);
            ImGui::TableNextColumn(ctx); ImGui::Checkbox(ctx, "No menu", &no_menu);
            ImGui::TableNextColumn(ctx); ImGui::Checkbox(ctx, "No move", &no_move);
            ImGui::TableNextColumn(ctx); ImGui::Checkbox(ctx, "No resize", &no_resize);
            ImGui::TableNextColumn(ctx); ImGui::Checkbox(ctx, "No collapse", &no_collapse);
            ImGui::TableNextColumn(ctx); ImGui::Checkbox(ctx, "No close", &no_close);
            ImGui::TableNextColumn(ctx); ImGui::Checkbox(ctx, "No nav", &no_nav);
            ImGui::TableNextColumn(ctx); ImGui::Checkbox(ctx, "No background", &no_background);
            ImGui::TableNextColumn(ctx); ImGui::Checkbox(ctx, "No bring to front", &no_bring_to_front);
            ImGui::TableNextColumn(ctx); ImGui::Checkbox(ctx, "Unsaved document", &unsaved_document);
            ImGui::EndTable(ctx);
        }
    }

    // All demo contents
    ShowDemoWindowWidgets(ctx);
    ShowDemoWindowLayout(ctx);
    ShowDemoWindowPopups(ctx);
    ShowDemoWindowTables(ctx);
    ShowDemoWindowInputs(ctx);

    // End of ShowDemoWindow()
    ImGui::PopItemWidth(ctx);
    ImGui::End(ctx);
}

static void ShowDemoWindowWidgets(ImGuiContext* ctx)
{
    IMGUI_DEMO_MARKER("Widgets");
    if (!ImGui::CollapsingHeader(ctx, "Widgets"))
        return;

    static bool disable_all = false; // The Checkbox for that is inside the "Disabled" section at the bottom
    if (disable_all)
        ImGui::BeginDisabled(ctx);

    IMGUI_DEMO_MARKER("Widgets/Basic");
    if (ImGui::TreeNode(ctx, "Basic"))
    {
        ImGui::SeparatorText(ctx, "General");

        IMGUI_DEMO_MARKER("Widgets/Basic/Button");
        static int clicked = 0;
        if (ImGui::Button(ctx, "Button"))
            clicked++;
        if (clicked & 1)
        {
            ImGui::SameLine(ctx);
            ImGui::Text(ctx, "Thanks for clicking me!");
        }

        IMGUI_DEMO_MARKER("Widgets/Basic/Checkbox");
        static bool check = true;
        ImGui::Checkbox(ctx, "checkbox", &check);

        IMGUI_DEMO_MARKER("Widgets/Basic/RadioButton");
        static int e = 0;
        ImGui::RadioButton(ctx, "radio a", &e, 0); ImGui::SameLine(ctx);
        ImGui::RadioButton(ctx, "radio b", &e, 1); ImGui::SameLine(ctx);
        ImGui::RadioButton(ctx, "radio c", &e, 2);

        // Color buttons, demonstrate using PushID() to add unique identifier in the ID stack, and changing style.
        IMGUI_DEMO_MARKER("Widgets/Basic/Buttons (Colored)");
        for (int i = 0; i < 7; i++)
        {
            if (i > 0)
                ImGui::SameLine(ctx);
            ImGui::PushID(ctx, i);
            ImGui::PushStyleColor(ctx, ImGuiCol_Button, (ImVec4)ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ctx, ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ctx, ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(i / 7.0f, 0.8f, 0.8f));
            ImGui::Button(ctx, "Click");
            ImGui::PopStyleColor(ctx, 3);
            ImGui::PopID(ctx);
        }

        // Use AlignTextToFramePadding() to align text baseline to the baseline of framed widgets elements
        // (otherwise a Text+SameLine+Button sequence will have the text a little too high by default!)
        // See 'Demo->Layout->Text Baseline Alignment' for details.
        ImGui::AlignTextToFramePadding(ctx);
        ImGui::Text(ctx, "Hold to repeat:");
        ImGui::SameLine(ctx);

        // Arrow buttons with Repeater
        IMGUI_DEMO_MARKER("Widgets/Basic/Buttons (Repeating)");
        static int counter = 0;
        float spacing = ImGui::GetStyle(ctx).ItemInnerSpacing.x;
        ImGui::PushButtonRepeat(ctx, true);
        if (ImGui::ArrowButton(ctx, "##left", ImGuiDir_Left)) { counter--; }
        ImGui::SameLine(ctx, 0.0f, spacing);
        if (ImGui::ArrowButton(ctx, "##right", ImGuiDir_Right)) { counter++; }
        ImGui::PopButtonRepeat(ctx);
        ImGui::SameLine(ctx);
        ImGui::Text(ctx, "%d", counter);

        ImGui::Button(ctx, "Tooltip");
        ImGui::SetItemTooltip(ctx, "I am a tooltip");

        ImGui::LabelText(ctx, "label", "Value");

        ImGui::SeparatorText(ctx, "Inputs");

        {
            // To wire InputText() with std::string or any other custom string type,
            // see the "Text Input > Resize Callback" section of this demo, and the misc/cpp/imgui_stdlib.h file.
            IMGUI_DEMO_MARKER("Widgets/Basic/InputText");
            static char str0[128] = "Hello, world!";
            ImGui::InputText(ctx, "input text", str0, IM_ARRAYSIZE(str0));
            ImGui::SameLine(ctx); HelpMarker(ctx, 
                "USER:\n"
                "Hold SHIFT or use mouse to select text.\n"
                "CTRL+Left/Right to word jump.\n"
                "CTRL+A or Double-Click to select all.\n"
                "CTRL+X,CTRL+C,CTRL+V clipboard.\n"
                "CTRL+Z,CTRL+Y undo/redo.\n"
                "ESCAPE to revert.\n\n"
                "PROGRAMMER:\n"
                "You can use the ImGuiInputTextFlags_CallbackResize facility if you need to wire InputText() "
                "to a dynamic string type. See misc/cpp/imgui_stdlib.h for an example (this is not demonstrated "
                "in imgui_demo.cpp).");

            static char str1[128] = "";
            ImGui::InputTextWithHint(ctx, "input text (w/ hint)", "enter text here", str1, IM_ARRAYSIZE(str1));

            IMGUI_DEMO_MARKER("Widgets/Basic/InputInt, InputFloat");
            static int i0 = 123;
            ImGui::InputInt(ctx, "input int", &i0);

            static float f0 = 0.001f;
            ImGui::InputFloat(ctx, "input float", &f0, 0.01f, 1.0f, "%.3f");

            static double d0 = 999999.00000001;
            ImGui::InputDouble(ctx, "input double", &d0, 0.01f, 1.0f, "%.8f");

            static float f1 = 1.e10f;
            ImGui::InputFloat(ctx, "input scientific", &f1, 0.0f, 0.0f, "%e");
            ImGui::SameLine(ctx); HelpMarker(ctx, 
                "You can input value using the scientific notation,\n"
                "  e.g. \"1e+8\" becomes \"100000000\".");

            static float vec4a[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
            ImGui::InputFloat3(ctx, "input float3", vec4a);
        }

        ImGui::SeparatorText(ctx, "Drags");

        {
            IMGUI_DEMO_MARKER("Widgets/Basic/DragInt, DragFloat");
            static int i1 = 50, i2 = 42;
            ImGui::DragInt(ctx, "drag int", &i1, 1);
            ImGui::SameLine(ctx); HelpMarker(ctx, 
                "Click and drag to edit value.\n"
                "Hold SHIFT/ALT for faster/slower edit.\n"
                "Double-click or CTRL+click to input value.");

            ImGui::DragInt(ctx, "drag int 0..100", &i2, 1, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp);

            static float f1 = 1.00f, f2 = 0.0067f;
            ImGui::DragFloat(ctx, "drag float", &f1, 0.005f);
            ImGui::DragFloat(ctx, "drag small float", &f2, 0.0001f, 0.0f, 0.0f, "%.06f ns");
        }

        ImGui::SeparatorText(ctx, "Sliders");

        {
            IMGUI_DEMO_MARKER("Widgets/Basic/SliderInt, SliderFloat");
            static int i1 = 0;
            ImGui::SliderInt(ctx, "slider int", &i1, -1, 3);
            ImGui::SameLine(ctx); HelpMarker(ctx, "CTRL+click to input value.");

            static float f1 = 0.123f, f2 = 0.0f;
            ImGui::SliderFloat(ctx, "slider float", &f1, 0.0f, 1.0f, "ratio = %.3f");
            ImGui::SliderFloat(ctx, "slider float (log)", &f2, -10.0f, 10.0f, "%.4f", ImGuiSliderFlags_Logarithmic);

            IMGUI_DEMO_MARKER("Widgets/Basic/SliderAngle");
            static float angle = 0.0f;
            ImGui::SliderAngle(ctx, "slider angle", &angle);

            // Using the format string to display a name instead of an integer.
            // Here we completely omit '%d' from the format string, so it'll only display a name.
            // This technique can also be used with DragInt().
            IMGUI_DEMO_MARKER("Widgets/Basic/Slider (enum)");
            enum Element { Element_Fire, Element_Earth, Element_Air, Element_Water, Element_COUNT };
            static int elem = Element_Fire;
            const char* elems_names[Element_COUNT] = { "Fire", "Earth", "Air", "Water" };
            const char* elem_name = (elem >= 0 && elem < Element_COUNT) ? elems_names[elem] : "Unknown";
            ImGui::SliderInt(ctx, "slider enum", &elem, 0, Element_COUNT - 1, elem_name); // Use ImGuiSliderFlags_NoInput flag to disable CTRL+Click here.
            ImGui::SameLine(ctx); HelpMarker(ctx, "Using the format string parameter to display a name instead of the underlying integer.");
        }

        ImGui::SeparatorText(ctx, "Selectors/Pickers");

        {
            IMGUI_DEMO_MARKER("Widgets/Basic/ColorEdit3, ColorEdit4");
            static float col1[3] = { 1.0f, 0.0f, 0.2f };
            static float col2[4] = { 0.4f, 0.7f, 0.0f, 0.5f };
            ImGui::ColorEdit3(ctx, "color 1", col1);
            ImGui::SameLine(ctx); HelpMarker(ctx, 
                "Click on the color square to open a color picker.\n"
                "Click and hold to use drag and drop.\n"
                "Right-click on the color square to show options.\n"
                "CTRL+click on individual component to input value.\n");

            ImGui::ColorEdit4(ctx, "color 2", col2);
        }

        {
            // Using the _simplified_ one-liner Combo() api here
            // See "Combo" section for examples of how to use the more flexible BeginCombo()/EndCombo() api.
            IMGUI_DEMO_MARKER("Widgets/Basic/Combo");
            const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIIIIII", "JJJJ", "KKKKKKK" };
            static int item_current = 0;
            ImGui::Combo(ctx, "combo", &item_current, items, IM_ARRAYSIZE(items));
            ImGui::SameLine(ctx); HelpMarker(ctx, 
                "Using the simplified one-liner Combo API here.\nRefer to the \"Combo\" section below for an explanation of how to use the more flexible and general BeginCombo/EndCombo API.");
        }

        {
            // Using the _simplified_ one-liner ListBox() api here
            // See "List boxes" section for examples of how to use the more flexible BeginListBox()/EndListBox() api.
            IMGUI_DEMO_MARKER("Widgets/Basic/ListBox");
            const char* items[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange", "Pineapple", "Strawberry", "Watermelon" };
            static int item_current = 1;
            ImGui::ListBox(ctx, "listbox", &item_current, items, IM_ARRAYSIZE(items), 4);
            ImGui::SameLine(ctx); HelpMarker(ctx, 
                "Using the simplified one-liner ListBox API here.\nRefer to the \"List boxes\" section below for an explanation of how to use the more flexible and general BeginListBox/EndListBox API.");
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Tooltips");
    if (ImGui::TreeNode(ctx, "Tooltips"))
    {
        // Tooltips are windows following the mouse. They do not take focus away.
        ImGui::SeparatorText(ctx, "General");

        // Typical use cases:
        // - Short-form (text only):      SetItemTooltip("Hello");
        // - Short-form (any contents):   if (BeginItemTooltip()) { Text("Hello"); EndTooltip(); }

        // - Full-form (text only):       if (IsItemHovered(...)) { SetTooltip("Hello"); }
        // - Full-form (any contents):    if (IsItemHovered(...) && BeginTooltip()) { Text("Hello"); EndTooltip(); }

        HelpMarker(ctx, 
            "Tooltip are typically created by using a IsItemHovered() + SetTooltip() sequence.\n\n"
            "We provide a helper SetItemTooltip() function to perform the two with standards flags.");

        ImVec2 sz = ImVec2(-FLT_MIN, 0.0f);

        ImGui::Button(ctx, "Basic", sz);
        ImGui::SetItemTooltip(ctx, "I am a tooltip");

        ImGui::Button(ctx, "Fancy", sz);
        if (ImGui::BeginItemTooltip(ctx))
        {
            ImGui::Text(ctx, "I am a fancy tooltip");
            static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
            ImGui::PlotLines(ctx, "Curve", arr, IM_ARRAYSIZE(arr));
            ImGui::Text(ctx, "Sin(time) = %f", sinf((float)ImGui::GetTime(ctx)));
            ImGui::EndTooltip(ctx);
        }

        ImGui::SeparatorText(ctx, "Always On");

        // Showcase NOT relying on a IsItemHovered() to emit a tooltip.
        // Here the tooltip is always emitted when 'always_on == true'.
        static int always_on = 0;
        ImGui::RadioButton(ctx, "Off", &always_on, 0);
        ImGui::SameLine(ctx);
        ImGui::RadioButton(ctx, "Always On (Simple)", &always_on, 1);
        ImGui::SameLine(ctx);
        ImGui::RadioButton(ctx, "Always On (Advanced)", &always_on, 2);
        if (always_on == 1)
            ImGui::SetTooltip(ctx, "I am following you around.");
        else if (always_on == 2 && ImGui::BeginTooltip(ctx))
        {
            ImGui::ProgressBar(ctx, sinf((float)ImGui::GetTime(ctx)) * 0.5f + 0.5f, ImVec2(ImGui::GetFontSize(ctx) * 25, 0.0f));
            ImGui::EndTooltip(ctx);
        }

        ImGui::SeparatorText(ctx, "Custom");

        HelpMarker(ctx, 
            "Passing ImGuiHoveredFlags_ForTooltip to IsItemHovered() is the preferred way to standardize"
            "tooltip activation details across your application. You may however decide to use custom"
            "flags for a specific tooltip instance.");

        // The following examples are passed for documentation purpose but may not be useful to most users.
        // Passing ImGuiHoveredFlags_ForTooltip to IsItemHovered() will pull ImGuiHoveredFlags flags values from
        // 'style.HoverFlagsForTooltipMouse' or 'style.HoverFlagsForTooltipNav' depending on whether mouse or gamepad/keyboard is being used.
        // With default settings, ImGuiHoveredFlags_ForTooltip is equivalent to ImGuiHoveredFlags_DelayShort + ImGuiHoveredFlags_Stationary.
        ImGui::Button(ctx, "Manual", sz);
        if (ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip(ctx, "I am a manually emitted tooltip.");

        ImGui::Button(ctx, "DelayNone", sz);
        if (ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_DelayNone))
            ImGui::SetTooltip(ctx, "I am a tooltip with no delay.");

        ImGui::Button(ctx, "DelayShort", sz);
        if (ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_NoSharedDelay))
            ImGui::SetTooltip(ctx, "I am a tooltip with a short delay (%0.2f sec).", ImGui::GetStyle(ctx).HoverDelayShort);

        ImGui::Button(ctx, "DelayLong", sz);
        if (ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
            ImGui::SetTooltip(ctx, "I am a tooltip with a long delay (%0.2f sec).", ImGui::GetStyle(ctx).HoverDelayNormal);

        ImGui::Button(ctx, "Stationary", sz);
        if (ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_Stationary))
            ImGui::SetTooltip(ctx, "I am a tooltip requiring mouse to be stationary before activating.");

        // Using ImGuiHoveredFlags_ForTooltip will pull flags from 'style.HoverFlagsForTooltipMouse' or 'style.HoverFlagsForTooltipNav',
        // which default value include the ImGuiHoveredFlags_AllowWhenDisabled flag.
        // As a result, Set
        ImGui::BeginDisabled(ctx);
        ImGui::Button(ctx, "Disabled item", sz);
        ImGui::EndDisabled(ctx);
        if (ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip(ctx, "I am a a tooltip for a disabled item.");

        ImGui::TreePop(ctx);
    }

    // Testing ImGuiOnceUponAFrame helper.
    //static ImGuiOnceUponAFrame once;
    //for (int i = 0; i < 5; i++)
    //    if (once)
    //        ImGui::Text("This will be displayed only once.");

    IMGUI_DEMO_MARKER("Widgets/Tree Nodes");
    if (ImGui::TreeNode(ctx, "Tree Nodes"))
    {
        IMGUI_DEMO_MARKER("Widgets/Tree Nodes/Basic trees");
        if (ImGui::TreeNode(ctx, "Basic trees"))
        {
            for (int i = 0; i < 5; i++)
            {
                // Use SetNextItemOpen() so set the default state of a node to be open. We could
                // also use TreeNodeEx() with the ImGuiTreeNodeFlags_DefaultOpen flag to achieve the same thing!
                if (i == 0)
                    ImGui::SetNextItemOpen(ctx, true, ImGuiCond_Once);

                if (ImGui::TreeNode(ctx, (void*)(intptr_t)i, "Child %d", i))
                {
                    ImGui::Text(ctx, "blah blah");
                    ImGui::SameLine(ctx);
                    if (ImGui::SmallButton(ctx, "button")) {}
                    ImGui::TreePop(ctx);
                }
            }
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Tree Nodes/Advanced, with Selectable nodes");
        if (ImGui::TreeNode(ctx, "Advanced, with Selectable nodes"))
        {
            HelpMarker(ctx, 
                "This is a more typical looking tree with selectable nodes.\n"
                "Click to select, CTRL+Click to toggle, click on arrows or double-click to open.");
            static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
            static bool align_label_with_current_x_position = false;
            static bool test_drag_and_drop = false;
            ImGui::CheckboxFlags(ctx, "ImGuiTreeNodeFlags_OpenOnArrow",       &base_flags, ImGuiTreeNodeFlags_OpenOnArrow);
            ImGui::CheckboxFlags(ctx, "ImGuiTreeNodeFlags_OpenOnDoubleClick", &base_flags, ImGuiTreeNodeFlags_OpenOnDoubleClick);
            ImGui::CheckboxFlags(ctx, "ImGuiTreeNodeFlags_SpanAvailWidth",    &base_flags, ImGuiTreeNodeFlags_SpanAvailWidth); ImGui::SameLine(ctx); HelpMarker(ctx, "Extend hit area to all available width instead of allowing more items to be laid out after the node.");
            ImGui::CheckboxFlags(ctx, "ImGuiTreeNodeFlags_SpanFullWidth",     &base_flags, ImGuiTreeNodeFlags_SpanFullWidth);
            ImGui::CheckboxFlags(ctx, "ImGuiTreeNodeFlags_SpanAllColumns",    &base_flags, ImGuiTreeNodeFlags_SpanAllColumns); ImGui::SameLine(ctx); HelpMarker(ctx, "For use in Tables only.");
            ImGui::Checkbox(ctx, "Align label with current X position", &align_label_with_current_x_position);
            ImGui::Checkbox(ctx, "Test tree node as drag source", &test_drag_and_drop);
            ImGui::Text(ctx, "Hello!");
            if (align_label_with_current_x_position)
                ImGui::Unindent(ctx, ImGui::GetTreeNodeToLabelSpacing(ctx));

            // 'selection_mask' is dumb representation of what may be user-side selection state.
            //  You may retain selection state inside or outside your objects in whatever format you see fit.
            // 'node_clicked' is temporary storage of what node we have clicked to process selection at the end
            /// of the loop. May be a pointer to your own node type, etc.
            static int selection_mask = (1 << 2);
            int node_clicked = -1;
            for (int i = 0; i < 6; i++)
            {
                // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
                // To alter selection we use IsItemClicked() && !IsItemToggledOpen(), so clicking on an arrow doesn't alter selection.
                ImGuiTreeNodeFlags node_flags = base_flags;
                const bool is_selected = (selection_mask & (1 << i)) != 0;
                if (is_selected)
                    node_flags |= ImGuiTreeNodeFlags_Selected;
                if (i < 3)
                {
                    // Items 0..2 are Tree Node
                    bool node_open = ImGui::TreeNodeEx(ctx, (void*)(intptr_t)i, node_flags, "Selectable Node %d", i);
                    if (ImGui::IsItemClicked(ctx) && !ImGui::IsItemToggledOpen(ctx))
                        node_clicked = i;
                    if (test_drag_and_drop && ImGui::BeginDragDropSource(ctx))
                    {
                        ImGui::SetDragDropPayload(ctx, "_TREENODE", NULL, 0);
                        ImGui::Text(ctx, "This is a drag and drop source");
                        ImGui::EndDragDropSource(ctx);
                    }
                    if (node_open)
                    {
                        ImGui::BulletText(ctx, "Blah blah\nBlah Blah");
                        ImGui::TreePop(ctx);
                    }
                }
                else
                {
                    // Items 3..5 are Tree Leaves
                    // The only reason we use TreeNode at all is to allow selection of the leaf. Otherwise we can
                    // use BulletText() or advance the cursor by GetTreeNodeToLabelSpacing() and call Text().
                    node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; // ImGuiTreeNodeFlags_Bullet
                    ImGui::TreeNodeEx(ctx, (void*)(intptr_t)i, node_flags, "Selectable Leaf %d", i);
                    if (ImGui::IsItemClicked(ctx) && !ImGui::IsItemToggledOpen(ctx))
                        node_clicked = i;
                    if (test_drag_and_drop && ImGui::BeginDragDropSource(ctx))
                    {
                        ImGui::SetDragDropPayload(ctx, "_TREENODE", NULL, 0);
                        ImGui::Text(ctx, "This is a drag and drop source");
                        ImGui::EndDragDropSource(ctx);
                    }
                }
            }
            if (node_clicked != -1)
            {
                // Update selection state
                // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
                if (ImGui::GetIO(ctx).KeyCtrl)
                    selection_mask ^= (1 << node_clicked);          // CTRL+click to toggle
                else //if (!(selection_mask & (1 << node_clicked))) // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
                    selection_mask = (1 << node_clicked);           // Click to single-select
            }
            if (align_label_with_current_x_position)
                ImGui::Indent(ctx, ImGui::GetTreeNodeToLabelSpacing(ctx));
            ImGui::TreePop(ctx);
        }
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Collapsing Headers");
    if (ImGui::TreeNode(ctx, "Collapsing Headers"))
    {
        static bool closable_group = true;
        ImGui::Checkbox(ctx, "Show 2nd header", &closable_group);
        if (ImGui::CollapsingHeader(ctx, "Header", ImGuiTreeNodeFlags_None))
        {
            ImGui::Text(ctx, "IsItemHovered: %d", ImGui::IsItemHovered(ctx));
            for (int i = 0; i < 5; i++)
                ImGui::Text(ctx, "Some content %d", i);
        }
        if (ImGui::CollapsingHeader(ctx, "Header with a close button", &closable_group))
        {
            ImGui::Text(ctx, "IsItemHovered: %d", ImGui::IsItemHovered(ctx));
            for (int i = 0; i < 5; i++)
                ImGui::Text(ctx, "More content %d", i);
        }
        /*
        if (ImGui::CollapsingHeader("Header with a bullet", ImGuiTreeNodeFlags_Bullet))
            ImGui::Text("IsItemHovered: %d", ImGui::IsItemHovered());
        */
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Bullets");
    if (ImGui::TreeNode(ctx, "Bullets"))
    {
        ImGui::BulletText(ctx, "Bullet point 1");
        ImGui::BulletText(ctx, "Bullet point 2\nOn multiple lines");
        if (ImGui::TreeNode(ctx, "Tree node"))
        {
            ImGui::BulletText(ctx, "Another bullet point");
            ImGui::TreePop(ctx);
        }
        ImGui::Bullet(ctx); ImGui::Text(ctx, "Bullet point 3 (two calls)");
        ImGui::Bullet(ctx); ImGui::SmallButton(ctx, "Button");
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Text");
    if (ImGui::TreeNode(ctx, "Text"))
    {
        IMGUI_DEMO_MARKER("Widgets/Text/Colored Text");
        if (ImGui::TreeNode(ctx, "Colorful Text"))
        {
            // Using shortcut. You can use PushStyleColor()/PopStyleColor() for more flexibility.
            ImGui::TextColored(ctx, ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Pink");
            ImGui::TextColored(ctx, ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Yellow");
            ImGui::TextDisabled(ctx, "Disabled");
            ImGui::SameLine(ctx); HelpMarker(ctx, "The TextDisabled color is stored in ImGuiStyle.");
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Text/Word Wrapping");
        if (ImGui::TreeNode(ctx, "Word Wrapping"))
        {
            // Using shortcut. You can use PushTextWrapPos()/PopTextWrapPos() for more flexibility.
            ImGui::TextWrapped(ctx, 
                "This text should automatically wrap on the edge of the window. The current implementation "
                "for text wrapping follows simple rules suitable for English and possibly other languages.");
            ImGui::Spacing(ctx);

            static float wrap_width = 200.0f;
            ImGui::SliderFloat(ctx, "Wrap width", &wrap_width, -20, 600, "%.0f");

            ImDrawList* draw_list = ImGui::GetWindowDrawList(ctx);
            for (int n = 0; n < 2; n++)
            {
                ImGui::Text(ctx, "Test paragraph %d:", n);
                ImVec2 pos = ImGui::GetCursorScreenPos(ctx);
                ImVec2 marker_min = ImVec2(pos.x + wrap_width, pos.y);
                ImVec2 marker_max = ImVec2(pos.x + wrap_width + 10, pos.y + ImGui::GetTextLineHeight(ctx));
                ImGui::PushTextWrapPos(ctx, ImGui::GetCursorPos(ctx).x + wrap_width);
                if (n == 0)
                    ImGui::Text(ctx, "The lazy dog is a good dog. This paragraph should fit within %.0f pixels. Testing a 1 character word. The quick brown fox jumps over the lazy dog.", wrap_width);
                else
                    ImGui::Text(ctx, "aaaaaaaa bbbbbbbb, c cccccccc,dddddddd. d eeeeeeee   ffffffff. gggggggg!hhhhhhhh");

                // Draw actual text bounding box, following by marker of our expected limit (should not overlap!)
                draw_list->AddRect(ImGui::GetItemRectMin(ctx), ImGui::GetItemRectMax(ctx), IM_COL32(255, 255, 0, 255));
                draw_list->AddRectFilled(marker_min, marker_max, IM_COL32(255, 0, 255, 255));
                ImGui::PopTextWrapPos(ctx);
            }

            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Text/UTF-8 Text");
        if (ImGui::TreeNode(ctx, "UTF-8 Text"))
        {
            // UTF-8 test with Japanese characters
            // (Needs a suitable font? Try "Google Noto" or "Arial Unicode". See docs/FONTS.md for details.)
            // - From C++11 you can use the u8"my text" syntax to encode literal strings as UTF-8
            // - For earlier compiler, you may be able to encode your sources as UTF-8 (e.g. in Visual Studio, you
            //   can save your source files as 'UTF-8 without signature').
            // - FOR THIS DEMO FILE ONLY, BECAUSE WE WANT TO SUPPORT OLD COMPILERS, WE ARE *NOT* INCLUDING RAW UTF-8
            //   CHARACTERS IN THIS SOURCE FILE. Instead we are encoding a few strings with hexadecimal constants.
            //   Don't do this in your application! Please use u8"text in any language" in your application!
            // Note that characters values are preserved even by InputText() if the font cannot be displayed,
            // so you can safely copy & paste garbled characters into another application.
            ImGui::TextWrapped(ctx, 
                "CJK text will only appear if the font was loaded with the appropriate CJK character ranges. "
                "Call io.Fonts->AddFontFromFileTTF() manually to load extra character ranges. "
                "Read docs/FONTS.md for details.");
            ImGui::Text(ctx, "Hiragana: \xe3\x81\x8b\xe3\x81\x8d\xe3\x81\x8f\xe3\x81\x91\xe3\x81\x93 (kakikukeko)"); // Normally we would use u8"blah blah" with the proper characters directly in the string.
            ImGui::Text(ctx, "Kanjis: \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e (nihongo)");
            static char buf[32] = "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e";
            //static char buf[32] = u8"NIHONGO"; // <- this is how you would write it with C++11, using real kanjis
            ImGui::InputText(ctx, "UTF-8 input", buf, IM_ARRAYSIZE(buf));
            ImGui::TreePop(ctx);
        }
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Images");
    if (ImGui::TreeNode(ctx, "Images"))
    {
        ImGuiIO& io = ImGui::GetIO(ctx);
        ImGui::TextWrapped(ctx, 
            "Below we are displaying the font texture (which is the only texture we have access to in this demo). "
            "Use the 'ImTextureID' type as storage to pass pointers or identifier to your own texture data. "
            "Hover the texture for a zoomed view!");

        // Below we are displaying the font texture because it is the only texture we have access to inside the demo!
        // Remember that ImTextureID is just storage for whatever you want it to be. It is essentially a value that
        // will be passed to the rendering backend via the ImDrawCmd structure.
        // If you use one of the default imgui_impl_XXXX.cpp rendering backend, they all have comments at the top
        // of their respective source file to specify what they expect to be stored in ImTextureID, for example:
        // - The imgui_impl_dx11.cpp renderer expect a 'ID3D11ShaderResourceView*' pointer
        // - The imgui_impl_opengl3.cpp renderer expect a GLuint OpenGL texture identifier, etc.
        // More:
        // - If you decided that ImTextureID = MyEngineTexture*, then you can pass your MyEngineTexture* pointers
        //   to ImGui::Image(), and gather width/height through your own functions, etc.
        // - You can use ShowMetricsWindow() to inspect the draw data that are being passed to your renderer,
        //   it will help you debug issues if you are confused about it.
        // - Consider using the lower-level ImDrawList::AddImage() API, via ImGui::GetWindowDrawList()->AddImage().
        // - Read https://github.com/ocornut/imgui/blob/master/docs/FAQ.md
        // - Read https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
        ImTextureID my_tex_id = io.Fonts->TexID;
        float my_tex_w = (float)io.Fonts->TexWidth;
        float my_tex_h = (float)io.Fonts->TexHeight;
        {
            static bool use_text_color_for_tint = false;
            ImGui::Checkbox(ctx, "Use Text Color for Tint", &use_text_color_for_tint);
            ImGui::Text(ctx, "%.0fx%.0f", my_tex_w, my_tex_h);
            ImVec2 pos = ImGui::GetCursorScreenPos(ctx);
            ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
            ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
            ImVec4 tint_col = use_text_color_for_tint ? ImGui::GetStyleColorVec4(ctx, ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border_col = ImGui::GetStyleColorVec4(ctx, ImGuiCol_Border);
            ImGui::Image(ctx, my_tex_id, ImVec2(my_tex_w, my_tex_h), uv_min, uv_max, tint_col, border_col);
            if (ImGui::BeginItemTooltip(ctx))
            {
                float region_sz = 32.0f;
                float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
                float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
                float zoom = 4.0f;
                if (region_x < 0.0f) { region_x = 0.0f; }
                else if (region_x > my_tex_w - region_sz) { region_x = my_tex_w - region_sz; }
                if (region_y < 0.0f) { region_y = 0.0f; }
                else if (region_y > my_tex_h - region_sz) { region_y = my_tex_h - region_sz; }
                ImGui::Text(ctx, "Min: (%.2f, %.2f)", region_x, region_y);
                ImGui::Text(ctx, "Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
                ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
                ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
                ImGui::Image(ctx, my_tex_id, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, tint_col, border_col);
                ImGui::EndTooltip(ctx);
            }
        }

        IMGUI_DEMO_MARKER("Widgets/Images/Textured buttons");
        ImGui::TextWrapped(ctx, "And now some textured buttons..");
        static int pressed_count = 0;
        for (int i = 0; i < 8; i++)
        {
            // UV coordinates are often (0.0f, 0.0f) and (1.0f, 1.0f) to display an entire textures.
            // Here are trying to display only a 32x32 pixels area of the texture, hence the UV computation.
            // Read about UV coordinates here: https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
            ImGui::PushID(ctx, i);
            if (i > 0)
                ImGui::PushStyleVar(ctx, ImGuiStyleVar_FramePadding, ImVec2(i - 1.0f, i - 1.0f));
            ImVec2 size = ImVec2(32.0f, 32.0f);                         // Size of the image we want to make visible
            ImVec2 uv0 = ImVec2(0.0f, 0.0f);                            // UV coordinates for lower-left
            ImVec2 uv1 = ImVec2(32.0f / my_tex_w, 32.0f / my_tex_h);    // UV coordinates for (32,32) in our texture
            ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);             // Black background
            ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);           // No tint
            if (ImGui::ImageButton(ctx, "", my_tex_id, size, uv0, uv1, bg_col, tint_col))
                pressed_count += 1;
            if (i > 0)
                ImGui::PopStyleVar(ctx);
            ImGui::PopID(ctx);
            ImGui::SameLine(ctx);
        }
        ImGui::NewLine(ctx);
        ImGui::Text(ctx, "Pressed %d times.", pressed_count);
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Combo");
    if (ImGui::TreeNode(ctx, "Combo"))
    {
        // Combo Boxes are also called "Dropdown" in other systems
        // Expose flags as checkbox for the demo
        static ImGuiComboFlags flags = 0;
        ImGui::CheckboxFlags(ctx, "ImGuiComboFlags_PopupAlignLeft", &flags, ImGuiComboFlags_PopupAlignLeft);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Only makes a difference if the popup is larger than the combo");
        if (ImGui::CheckboxFlags(ctx, "ImGuiComboFlags_NoArrowButton", &flags, ImGuiComboFlags_NoArrowButton))
            flags &= ~ImGuiComboFlags_NoPreview;     // Clear the other flag, as we cannot combine both
        if (ImGui::CheckboxFlags(ctx, "ImGuiComboFlags_NoPreview", &flags, ImGuiComboFlags_NoPreview))
            flags &= ~(ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_WidthFitPreview); // Clear the other flag, as we cannot combine both
        if (ImGui::CheckboxFlags(ctx, "ImGuiComboFlags_WidthFitPreview", &flags, ImGuiComboFlags_WidthFitPreview))
            flags &= ~ImGuiComboFlags_NoPreview;

        // Override default popup height
        if (ImGui::CheckboxFlags(ctx, "ImGuiComboFlags_HeightSmall", &flags, ImGuiComboFlags_HeightSmall))
            flags &= ~(ImGuiComboFlags_HeightMask_ & ~ImGuiComboFlags_HeightSmall);
        if (ImGui::CheckboxFlags(ctx, "ImGuiComboFlags_HeightRegular", &flags, ImGuiComboFlags_HeightRegular))
            flags &= ~(ImGuiComboFlags_HeightMask_ & ~ImGuiComboFlags_HeightRegular);
        if (ImGui::CheckboxFlags(ctx, "ImGuiComboFlags_HeightLargest", &flags, ImGuiComboFlags_HeightLargest))
            flags &= ~(ImGuiComboFlags_HeightMask_ & ~ImGuiComboFlags_HeightLargest);

        // Using the generic BeginCombo() API, you have full control over how to display the combo contents.
        // (your selection data could be an index, a pointer to the object, an id for the object, a flag intrusively
        // stored in the object itself, etc.)
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO" };
        static int item_current_idx = 0; // Here we store our selection data as an index.
        const char* combo_preview_value = items[item_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)
        if (ImGui::BeginCombo(ctx, "combo 1", combo_preview_value, flags))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (item_current_idx == n);
                if (ImGui::Selectable(ctx, items[n], is_selected))
                    item_current_idx = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus(ctx);
            }
            ImGui::EndCombo(ctx);
        }

        ImGui::Spacing(ctx);
        ImGui::SeparatorText(ctx, "One-liner variants");
        HelpMarker(ctx, "Flags above don't apply to this section.");

        // Simplified one-liner Combo() API, using values packed in a single constant string
        // This is a convenience for when the selection set is small and known at compile-time.
        static int item_current_2 = 0;
        ImGui::Combo(ctx, "combo 2 (one-liner)", &item_current_2, "aaaa\0bbbb\0cccc\0dddd\0eeee\0\0");

        // Simplified one-liner Combo() using an array of const char*
        // This is not very useful (may obsolete): prefer using BeginCombo()/EndCombo() for full control.
        static int item_current_3 = -1; // If the selection isn't within 0..count, Combo won't display a preview
        ImGui::Combo(ctx, "combo 3 (array)", &item_current_3, items, IM_ARRAYSIZE(items));

        // Simplified one-liner Combo() using an accessor function
        static int item_current_4 = 0;
        ImGui::Combo(ctx, "combo 4 (function)", &item_current_4, [](void* data, int n) { return ((const char**)data)[n]; }, items, IM_ARRAYSIZE(items));

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/List Boxes");
    if (ImGui::TreeNode(ctx, "List boxes"))
    {
        // BeginListBox() is essentially a thin wrapper to using BeginChild()/EndChild() with the ImGuiChildFlags_FrameStyle flag for stylistic changes + displaying a label.
        // You may be tempted to simply use BeginChild() directly, however note that BeginChild() requires EndChild() to always be called (inconsistent with BeginListBox()/EndListBox()).

        // Using the generic BeginListBox() API, you have full control over how to display the combo contents.
        // (your selection data could be an index, a pointer to the object, an id for the object, a flag intrusively
        // stored in the object itself, etc.)
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO" };
        static int item_current_idx = 0; // Here we store our selection data as an index.
        if (ImGui::BeginListBox(ctx, "listbox 1"))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (item_current_idx == n);
                if (ImGui::Selectable(ctx, items[n], is_selected))
                    item_current_idx = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus(ctx);
            }
            ImGui::EndListBox(ctx);
        }

        // Custom size: use all width, 5 items tall
        ImGui::Text(ctx, "Full-width:");
        if (ImGui::BeginListBox(ctx, "##listbox 2", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing(ctx))))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (item_current_idx == n);
                if (ImGui::Selectable(ctx, items[n], is_selected))
                    item_current_idx = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus(ctx);
            }
            ImGui::EndListBox(ctx);
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Selectables");
    if (ImGui::TreeNode(ctx, "Selectables"))
    {
        // Selectable() has 2 overloads:
        // - The one taking "bool selected" as a read-only selection information.
        //   When Selectable() has been clicked it returns true and you can alter selection state accordingly.
        // - The one taking "bool* p_selected" as a read-write selection information (convenient in some cases)
        // The earlier is more flexible, as in real application your selection may be stored in many different ways
        // and not necessarily inside a bool value (e.g. in flags within objects, as an external list, etc).
        IMGUI_DEMO_MARKER("Widgets/Selectables/Basic");
        if (ImGui::TreeNode(ctx, "Basic"))
        {
            static bool selection[5] = { false, true, false, false };
            ImGui::Selectable(ctx, "1. I am selectable", &selection[0]);
            ImGui::Selectable(ctx, "2. I am selectable", &selection[1]);
            ImGui::Selectable(ctx, "3. I am selectable", &selection[2]);
            if (ImGui::Selectable(ctx, "4. I am double clickable", selection[3], ImGuiSelectableFlags_AllowDoubleClick))
                if (ImGui::IsMouseDoubleClicked(ctx, 0))
                    selection[3] = !selection[3];
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Selectables/Single Selection");
        if (ImGui::TreeNode(ctx, "Selection State: Single Selection"))
        {
            static int selected = -1;
            for (int n = 0; n < 5; n++)
            {
                char buf[32];
                sprintf(buf, "Object %d", n);
                if (ImGui::Selectable(ctx, buf, selected == n))
                    selected = n;
            }
            ImGui::TreePop(ctx);
        }
        IMGUI_DEMO_MARKER("Widgets/Selectables/Multiple Selection");
        if (ImGui::TreeNode(ctx, "Selection State: Multiple Selection"))
        {
            HelpMarker(ctx, "Hold CTRL and click to select multiple items.");
            static bool selection[5] = { false, false, false, false, false };
            for (int n = 0; n < 5; n++)
            {
                char buf[32];
                sprintf(buf, "Object %d", n);
                if (ImGui::Selectable(ctx, buf, selection[n]))
                {
                    if (!ImGui::GetIO(ctx).KeyCtrl)    // Clear selection when CTRL is not held
                        memset(selection, 0, sizeof(selection));
                    selection[n] ^= 1;
                }
            }
            ImGui::TreePop(ctx);
        }
        IMGUI_DEMO_MARKER("Widgets/Selectables/Rendering more items on the same line");
        if (ImGui::TreeNode(ctx, "Rendering more items on the same line"))
        {
            // (1) Using SetNextItemAllowOverlap()
            // (2) Using the Selectable() override that takes "bool* p_selected" parameter, the bool value is toggled automatically.
            static bool selected[3] = { false, false, false };
            ImGui::SetNextItemAllowOverlap(ctx); ImGui::Selectable(ctx, "main.c",    &selected[0]); ImGui::SameLine(ctx); ImGui::SmallButton(ctx, "Link 1");
            ImGui::SetNextItemAllowOverlap(ctx); ImGui::Selectable(ctx, "Hello.cpp", &selected[1]); ImGui::SameLine(ctx); ImGui::SmallButton(ctx, "Link 2");
            ImGui::SetNextItemAllowOverlap(ctx); ImGui::Selectable(ctx, "Hello.h",   &selected[2]); ImGui::SameLine(ctx); ImGui::SmallButton(ctx, "Link 3");
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Selectables/In columns");
        if (ImGui::TreeNode(ctx, "In columns"))
        {
            static bool selected[10] = {};

            if (ImGui::BeginTable(ctx, "split1", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
            {
                for (int i = 0; i < 10; i++)
                {
                    char label[32];
                    sprintf(label, "Item %d", i);
                    ImGui::TableNextColumn(ctx);
                    ImGui::Selectable(ctx, label, &selected[i]); // FIXME-TABLE: Selection overlap
                }
                ImGui::EndTable(ctx);
            }
            ImGui::Spacing(ctx);
            if (ImGui::BeginTable(ctx, "split2", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
            {
                for (int i = 0; i < 10; i++)
                {
                    char label[32];
                    sprintf(label, "Item %d", i);
                    ImGui::TableNextRow(ctx);
                    ImGui::TableNextColumn(ctx);
                    ImGui::Selectable(ctx, label, &selected[i], ImGuiSelectableFlags_SpanAllColumns);
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "Some other contents");
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "123456");
                }
                ImGui::EndTable(ctx);
            }
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Selectables/Grid");
        if (ImGui::TreeNode(ctx, "Grid"))
        {
            static char selected[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };

            // Add in a bit of silly fun...
            const float time = (float)ImGui::GetTime(ctx);
            const bool winning_state = memchr(selected, 0, sizeof(selected)) == NULL; // If all cells are selected...
            if (winning_state)
                ImGui::PushStyleVar(ctx, ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f + 0.5f * cosf(time * 2.0f), 0.5f + 0.5f * sinf(time * 3.0f)));

            for (int y = 0; y < 4; y++)
                for (int x = 0; x < 4; x++)
                {
                    if (x > 0)
                        ImGui::SameLine(ctx);
                    ImGui::PushID(ctx, y * 4 + x);
                    if (ImGui::Selectable(ctx, "Sailor", selected[y][x] != 0, 0, ImVec2(50, 50)))
                    {
                        // Toggle clicked cell + toggle neighbors
                        selected[y][x] ^= 1;
                        if (x > 0) { selected[y][x - 1] ^= 1; }
                        if (x < 3) { selected[y][x + 1] ^= 1; }
                        if (y > 0) { selected[y - 1][x] ^= 1; }
                        if (y < 3) { selected[y + 1][x] ^= 1; }
                    }
                    ImGui::PopID(ctx);
                }

            if (winning_state)
                ImGui::PopStyleVar(ctx);
            ImGui::TreePop(ctx);
        }
        IMGUI_DEMO_MARKER("Widgets/Selectables/Alignment");
        if (ImGui::TreeNode(ctx, "Alignment"))
        {
            HelpMarker(ctx, 
                "By default, Selectables uses style.SelectableTextAlign but it can be overridden on a per-item "
                "basis using PushStyleVar(). You'll probably want to always keep your default situation to "
                "left-align otherwise it becomes difficult to layout multiple items on a same line");
            static bool selected[3 * 3] = { true, false, true, false, true, false, true, false, true };
            for (int y = 0; y < 3; y++)
            {
                for (int x = 0; x < 3; x++)
                {
                    ImVec2 alignment = ImVec2((float)x / 2.0f, (float)y / 2.0f);
                    char name[32];
                    sprintf(name, "(%.1f,%.1f)", alignment.x, alignment.y);
                    if (x > 0) ImGui::SameLine(ctx);
                    ImGui::PushStyleVar(ctx, ImGuiStyleVar_SelectableTextAlign, alignment);
                    ImGui::Selectable(ctx, name, &selected[3 * y + x], ImGuiSelectableFlags_None, ImVec2(80, 80));
                    ImGui::PopStyleVar(ctx);
                }
            }
            ImGui::TreePop(ctx);
        }
        ImGui::TreePop(ctx);
    }

    // To wire InputText() with std::string or any other custom string type,
    // see the "Text Input > Resize Callback" section of this demo, and the misc/cpp/imgui_stdlib.h file.
    IMGUI_DEMO_MARKER("Widgets/Text Input");
    if (ImGui::TreeNode(ctx, "Text Input"))
    {
        IMGUI_DEMO_MARKER("Widgets/Text Input/Multi-line Text Input");
        if (ImGui::TreeNode(ctx, "Multi-line Text Input"))
        {
            // Note: we are using a fixed-sized buffer for simplicity here. See ImGuiInputTextFlags_CallbackResize
            // and the code in misc/cpp/imgui_stdlib.h for how to setup InputText() for dynamically resizing strings.
            static char text[1024 * 16] =
                "/*\n"
                " The Pentium F00F bug, shorthand for F0 0F C7 C8,\n"
                " the hexadecimal encoding of one offending instruction,\n"
                " more formally, the invalid operand with locked CMPXCHG8B\n"
                " instruction bug, is a design flaw in the majority of\n"
                " Intel Pentium, Pentium MMX, and Pentium OverDrive\n"
                " processors (all in the P5 microarchitecture).\n"
                "*/\n\n"
                "label:\n"
                "\tlock cmpxchg8b eax\n";

            static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
            HelpMarker(ctx, "You can use the ImGuiInputTextFlags_CallbackResize facility if you need to wire InputTextMultiline() to a dynamic string type. See misc/cpp/imgui_stdlib.h for an example. (This is not demonstrated in imgui_demo.cpp because we don't want to include <string> in here)");
            ImGui::CheckboxFlags(ctx, "ImGuiInputTextFlags_ReadOnly", &flags, ImGuiInputTextFlags_ReadOnly);
            ImGui::CheckboxFlags(ctx, "ImGuiInputTextFlags_AllowTabInput", &flags, ImGuiInputTextFlags_AllowTabInput);
            ImGui::CheckboxFlags(ctx, "ImGuiInputTextFlags_CtrlEnterForNewLine", &flags, ImGuiInputTextFlags_CtrlEnterForNewLine);
            ImGui::InputTextMultiline(ctx, "##source", text, IM_ARRAYSIZE(text), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight(ctx) * 16), flags);
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Text Input/Filtered Text Input");
        if (ImGui::TreeNode(ctx, "Filtered Text Input"))
        {
            struct TextFilters
            {
                // Modify character input by altering 'data->Eventchar' (ImGuiInputTextFlags_CallbackCharFilter callback)
                static int FilterCasingSwap(ImGuiInputTextCallbackData* data)
                {
                    if (data->EventChar >= 'a' && data->EventChar <= 'z')       { data->EventChar -= 'a' - 'A'; } // Lowercase becomes uppercase
                    else if (data->EventChar >= 'A' && data->EventChar <= 'Z')  { data->EventChar += 'a' - 'A'; } // Uppercase becomes lowercase
                    return 0;
                }

                // Return 0 (pass) if the character is 'i' or 'm' or 'g' or 'u' or 'i', otherwise return 1 (filter out)
                static int FilterImGuiLetters(ImGuiInputTextCallbackData* data)
                {
                    if (data->EventChar < 256 && strchr("imgui", (char)data->EventChar))
                        return 0;
                    return 1;
                }
            };

            static char buf1[32] = ""; ImGui::InputText(ctx, "default",     buf1, 32);
            static char buf2[32] = ""; ImGui::InputText(ctx, "decimal",     buf2, 32, ImGuiInputTextFlags_CharsDecimal);
            static char buf3[32] = ""; ImGui::InputText(ctx, "hexadecimal", buf3, 32, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
            static char buf4[32] = ""; ImGui::InputText(ctx, "uppercase",   buf4, 32, ImGuiInputTextFlags_CharsUppercase);
            static char buf5[32] = ""; ImGui::InputText(ctx, "no blank",    buf5, 32, ImGuiInputTextFlags_CharsNoBlank);
            static char buf6[32] = ""; ImGui::InputText(ctx, "casing swap", buf6, 32, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterCasingSwap); // Use CharFilter callback to replace characters.
            static char buf7[32] = ""; ImGui::InputText(ctx, "\"imgui\"",   buf7, 32, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterImGuiLetters); // Use CharFilter callback to disable some characters.
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Text Input/Password input");
        if (ImGui::TreeNode(ctx, "Password Input"))
        {
            static char password[64] = "password123";
            ImGui::InputText(ctx, "password", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Display all characters as '*'.\nDisable clipboard cut and copy.\nDisable logging.\n");
            ImGui::InputTextWithHint(ctx, "password (w/ hint)", "<password>", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);
            ImGui::InputText(ctx, "password (clear)", password, IM_ARRAYSIZE(password));
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Text Input/Completion, History, Edit Callbacks");
        if (ImGui::TreeNode(ctx, "Completion, History, Edit Callbacks"))
        {
            struct Funcs
            {
                static int MyCallback(ImGuiInputTextCallbackData* data)
                {
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
                    {
                        data->InsertChars(data->CursorPos, "..");
                    }
                    else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
                    {
                        if (data->EventKey == ImGuiKey_UpArrow)
                        {
                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, "Pressed Up!");
                            data->SelectAll();
                        }
                        else if (data->EventKey == ImGuiKey_DownArrow)
                        {
                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, "Pressed Down!");
                            data->SelectAll();
                        }
                    }
                    else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit)
                    {
                        // Toggle casing of first character
                        char c = data->Buf[0];
                        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) data->Buf[0] ^= 32;
                        data->BufDirty = true;

                        // Increment a counter
                        int* p_int = (int*)data->UserData;
                        *p_int = *p_int + 1;
                    }
                    return 0;
                }
            };
            static char buf1[64];
            ImGui::InputText(ctx, "Completion", buf1, 64, ImGuiInputTextFlags_CallbackCompletion, Funcs::MyCallback);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Here we append \"..\" each time Tab is pressed. See 'Examples>Console' for a more meaningful demonstration of using this callback.");

            static char buf2[64];
            ImGui::InputText(ctx, "History", buf2, 64, ImGuiInputTextFlags_CallbackHistory, Funcs::MyCallback);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Here we replace and select text each time Up/Down are pressed. See 'Examples>Console' for a more meaningful demonstration of using this callback.");

            static char buf3[64];
            static int edit_count = 0;
            ImGui::InputText(ctx, "Edit", buf3, 64, ImGuiInputTextFlags_CallbackEdit, Funcs::MyCallback, (void*)&edit_count);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Here we toggle the casing of the first character on every edit + count edits.");
            ImGui::SameLine(ctx); ImGui::Text(ctx, "(%d)", edit_count);

            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Text Input/Resize Callback");
        if (ImGui::TreeNode(ctx, "Resize Callback"))
        {
            // To wire InputText() with std::string or any other custom string type,
            // you can use the ImGuiInputTextFlags_CallbackResize flag + create a custom ImGui::InputText() wrapper
            // using your preferred type. See misc/cpp/imgui_stdlib.h for an implementation of this using std::string.
            HelpMarker(ctx, 
                "Using ImGuiInputTextFlags_CallbackResize to wire your custom string type to InputText().\n\n"
                "See misc/cpp/imgui_stdlib.h for an implementation of this for std::string.");
            struct Funcs
            {
                static int MyResizeCallback(ImGuiInputTextCallbackData* data)
                {
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
                    {
                        ImVector<char>* my_str = (ImVector<char>*)data->UserData;
                        IM_ASSERT(my_str->begin() == data->Buf);
                        my_str->resize(data->BufSize); // NB: On resizing calls, generally data->BufSize == data->BufTextLen + 1
                        data->Buf = my_str->begin();
                    }
                    return 0;
                }

                // Note: Because ImGui:: is a namespace you would typically add your own function into the namespace.
                // For example, you code may declare a function 'ImGui::InputText(const char* label, MyString* my_str)'
                static bool MyInputTextMultiline(ImGuiContext* ctx, const char* label, ImVector<char>* my_str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0)
                {
                    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
                    return ImGui::InputTextMultiline(ctx, label, my_str->begin(), (size_t)my_str->size(), size, flags | ImGuiInputTextFlags_CallbackResize, Funcs::MyResizeCallback, (void*)my_str);
                }
            };

            // For this demo we are using ImVector as a string container.
            // Note that because we need to store a terminating zero character, our size/capacity are 1 more
            // than usually reported by a typical string class.
            static ImVector<char> my_str;
            if (my_str.empty())
                my_str.push_back(0);
            Funcs::MyInputTextMultiline(ctx, "##MyStr", &my_str, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight(ctx) * 16));
            ImGui::Text(ctx, "Data: %p\nSize: %d\nCapacity: %d", (void*)my_str.begin(), my_str.size(), my_str.capacity());
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Text Input/Miscellaneous");
        if (ImGui::TreeNode(ctx, "Miscellaneous"))
        {
            static char buf1[16];
            static ImGuiInputTextFlags flags = ImGuiInputTextFlags_EscapeClearsAll;
            ImGui::CheckboxFlags(ctx, "ImGuiInputTextFlags_EscapeClearsAll", &flags, ImGuiInputTextFlags_EscapeClearsAll);
            ImGui::CheckboxFlags(ctx, "ImGuiInputTextFlags_ReadOnly", &flags, ImGuiInputTextFlags_ReadOnly);
            ImGui::CheckboxFlags(ctx, "ImGuiInputTextFlags_NoUndoRedo", &flags, ImGuiInputTextFlags_NoUndoRedo);
            ImGui::InputText(ctx, "Hello", buf1, IM_ARRAYSIZE(buf1), flags);
            ImGui::TreePop(ctx);
        }

        ImGui::TreePop(ctx);
    }

    // Tabs
    IMGUI_DEMO_MARKER("Widgets/Tabs");
    if (ImGui::TreeNode(ctx, "Tabs"))
    {
        IMGUI_DEMO_MARKER("Widgets/Tabs/Basic");
        if (ImGui::TreeNode(ctx, "Basic"))
        {
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (ImGui::BeginTabBar(ctx, "MyTabBar", tab_bar_flags))
            {
                if (ImGui::BeginTabItem(ctx, "Avocado"))
                {
                    ImGui::Text(ctx, "This is the Avocado tab!\nblah blah blah blah blah");
                    ImGui::EndTabItem(ctx);
                }
                if (ImGui::BeginTabItem(ctx, "Broccoli"))
                {
                    ImGui::Text(ctx, "This is the Broccoli tab!\nblah blah blah blah blah");
                    ImGui::EndTabItem(ctx);
                }
                if (ImGui::BeginTabItem(ctx, "Cucumber"))
                {
                    ImGui::Text(ctx, "This is the Cucumber tab!\nblah blah blah blah blah");
                    ImGui::EndTabItem(ctx);
                }
                ImGui::EndTabBar(ctx);
            }
            ImGui::Separator(ctx);
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Tabs/Advanced & Close Button");
        if (ImGui::TreeNode(ctx, "Advanced & Close Button"))
        {
            // Expose a couple of the available flags. In most cases you may just call BeginTabBar() with no flags (0).
            static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;
            ImGui::CheckboxFlags(ctx, "ImGuiTabBarFlags_Reorderable", &tab_bar_flags, ImGuiTabBarFlags_Reorderable);
            ImGui::CheckboxFlags(ctx, "ImGuiTabBarFlags_AutoSelectNewTabs", &tab_bar_flags, ImGuiTabBarFlags_AutoSelectNewTabs);
            ImGui::CheckboxFlags(ctx, "ImGuiTabBarFlags_TabListPopupButton", &tab_bar_flags, ImGuiTabBarFlags_TabListPopupButton);
            ImGui::CheckboxFlags(ctx, "ImGuiTabBarFlags_NoCloseWithMiddleMouseButton", &tab_bar_flags, ImGuiTabBarFlags_NoCloseWithMiddleMouseButton);
            if ((tab_bar_flags & ImGuiTabBarFlags_FittingPolicyMask_) == 0)
                tab_bar_flags |= ImGuiTabBarFlags_FittingPolicyDefault_;
            if (ImGui::CheckboxFlags(ctx, "ImGuiTabBarFlags_FittingPolicyResizeDown", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyResizeDown))
                tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyResizeDown);
            if (ImGui::CheckboxFlags(ctx, "ImGuiTabBarFlags_FittingPolicyScroll", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyScroll))
                tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyScroll);

            // Tab Bar
            const char* names[4] = { "Artichoke", "Beetroot", "Celery", "Daikon" };
            static bool opened[4] = { true, true, true, true }; // Persistent user state
            for (int n = 0; n < IM_ARRAYSIZE(opened); n++)
            {
                if (n > 0) { ImGui::SameLine(ctx); }
                ImGui::Checkbox(ctx, names[n], &opened[n]);
            }

            // Passing a bool* to BeginTabItem() is similar to passing one to Begin():
            // the underlying bool will be set to false when the tab is closed.
            if (ImGui::BeginTabBar(ctx, "MyTabBar", tab_bar_flags))
            {
                for (int n = 0; n < IM_ARRAYSIZE(opened); n++)
                    if (opened[n] && ImGui::BeginTabItem(ctx, names[n], &opened[n], ImGuiTabItemFlags_None))
                    {
                        ImGui::Text(ctx, "This is the %s tab!", names[n]);
                        if (n & 1)
                            ImGui::Text(ctx, "I am an odd tab.");
                        ImGui::EndTabItem(ctx);
                    }
                ImGui::EndTabBar(ctx);
            }
            ImGui::Separator(ctx);
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Tabs/TabItemButton & Leading-Trailing flags");
        if (ImGui::TreeNode(ctx, "TabItemButton & Leading/Trailing flags"))
        {
            static ImVector<int> active_tabs;
            static int next_tab_id = 0;
            if (next_tab_id == 0) // Initialize with some default tabs
                for (int i = 0; i < 3; i++)
                    active_tabs.push_back(next_tab_id++);

            // TabItemButton() and Leading/Trailing flags are distinct features which we will demo together.
            // (It is possible to submit regular tabs with Leading/Trailing flags, or TabItemButton tabs without Leading/Trailing flags...
            // but they tend to make more sense together)
            static bool show_leading_button = true;
            static bool show_trailing_button = true;
            ImGui::Checkbox(ctx, "Show Leading TabItemButton()", &show_leading_button);
            ImGui::Checkbox(ctx, "Show Trailing TabItemButton()", &show_trailing_button);

            // Expose some other flags which are useful to showcase how they interact with Leading/Trailing tabs
            static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyResizeDown;
            ImGui::CheckboxFlags(ctx, "ImGuiTabBarFlags_TabListPopupButton", &tab_bar_flags, ImGuiTabBarFlags_TabListPopupButton);
            if (ImGui::CheckboxFlags(ctx, "ImGuiTabBarFlags_FittingPolicyResizeDown", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyResizeDown))
                tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyResizeDown);
            if (ImGui::CheckboxFlags(ctx, "ImGuiTabBarFlags_FittingPolicyScroll", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyScroll))
                tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyScroll);

            if (ImGui::BeginTabBar(ctx, "MyTabBar", tab_bar_flags))
            {
                // Demo a Leading TabItemButton(): click the "?" button to open a menu
                if (show_leading_button)
                    if (ImGui::TabItemButton(ctx, "?", ImGuiTabItemFlags_Leading | ImGuiTabItemFlags_NoTooltip))
                        ImGui::OpenPopup(ctx, "MyHelpMenu");
                if (ImGui::BeginPopup(ctx, "MyHelpMenu"))
                {
                    ImGui::Selectable(ctx, "Hello!");
                    ImGui::EndPopup(ctx);
                }

                // Demo Trailing Tabs: click the "+" button to add a new tab (in your app you may want to use a font icon instead of the "+")
                // Note that we submit it before the regular tabs, but because of the ImGuiTabItemFlags_Trailing flag it will always appear at the end.
                if (show_trailing_button)
                    if (ImGui::TabItemButton(ctx, "+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
                        active_tabs.push_back(next_tab_id++); // Add new tab

                // Submit our regular tabs
                for (int n = 0; n < active_tabs.Size; )
                {
                    bool open = true;
                    char name[16];
                    snprintf(name, IM_ARRAYSIZE(name), "%04d", active_tabs[n]);
                    if (ImGui::BeginTabItem(ctx, name, &open, ImGuiTabItemFlags_None))
                    {
                        ImGui::Text(ctx, "This is the %s tab!", name);
                        ImGui::EndTabItem(ctx);
                    }

                    if (!open)
                        active_tabs.erase(active_tabs.Data + n);
                    else
                        n++;
                }

                ImGui::EndTabBar(ctx);
            }
            ImGui::Separator(ctx);
            ImGui::TreePop(ctx);
        }
        ImGui::TreePop(ctx);
    }

    // Plot/Graph widgets are not very good.
    // Consider using a third-party library such as ImPlot: https://github.com/epezent/implot
    // (see others https://github.com/ocornut/imgui/wiki/Useful-Extensions)
    IMGUI_DEMO_MARKER("Widgets/Plotting");
    if (ImGui::TreeNode(ctx, "Plotting"))
    {
        static bool animate = true;
        ImGui::Checkbox(ctx, "Animate", &animate);

        // Plot as lines and plot as histogram
        IMGUI_DEMO_MARKER("Widgets/Plotting/PlotLines, PlotHistogram");
        static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
        ImGui::PlotLines(ctx, "Frame Times", arr, IM_ARRAYSIZE(arr));
        ImGui::PlotHistogram(ctx, "Histogram", arr, IM_ARRAYSIZE(arr), 0, NULL, 0.0f, 1.0f, ImVec2(0, 80.0f));

        // Fill an array of contiguous float values to plot
        // Tip: If your float aren't contiguous but part of a structure, you can pass a pointer to your first float
        // and the sizeof() of your structure in the "stride" parameter.
        static float values[90] = {};
        static int values_offset = 0;
        static double refresh_time = 0.0;
        if (!animate || refresh_time == 0.0)
            refresh_time = ImGui::GetTime(ctx);
        while (refresh_time < ImGui::GetTime(ctx)) // Create data at fixed 60 Hz rate for the demo
        {
            static float phase = 0.0f;
            values[values_offset] = cosf(phase);
            values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
            phase += 0.10f * values_offset;
            refresh_time += 1.0f / 60.0f;
        }

        // Plots can display overlay texts
        // (in this example, we will display an average value)
        {
            float average = 0.0f;
            for (int n = 0; n < IM_ARRAYSIZE(values); n++)
                average += values[n];
            average /= (float)IM_ARRAYSIZE(values);
            char overlay[32];
            sprintf(overlay, "avg %f", average);
            ImGui::PlotLines(ctx, "Lines", values, IM_ARRAYSIZE(values), values_offset, overlay, -1.0f, 1.0f, ImVec2(0, 80.0f));
        }

        // Use functions to generate output
        // FIXME: This is actually VERY awkward because current plot API only pass in indices.
        // We probably want an API passing floats and user provide sample rate/count.
        struct Funcs
        {
            static float Sin(void*, int i) { return sinf(i * 0.1f); }
            static float Saw(void*, int i) { return (i & 1) ? 1.0f : -1.0f; }
        };
        static int func_type = 0, display_count = 70;
        ImGui::SeparatorText(ctx, "Functions");
        ImGui::SetNextItemWidth(ctx, ImGui::GetFontSize(ctx) * 8);
        ImGui::Combo(ctx, "func", &func_type, "Sin\0Saw\0");
        ImGui::SameLine(ctx);
        ImGui::SliderInt(ctx, "Sample count", &display_count, 1, 400);
        float (*func)(void*, int) = (func_type == 0) ? Funcs::Sin : Funcs::Saw;
        ImGui::PlotLines(ctx, "Lines", func, NULL, display_count, 0, NULL, -1.0f, 1.0f, ImVec2(0, 80));
        ImGui::PlotHistogram(ctx, "Histogram", func, NULL, display_count, 0, NULL, -1.0f, 1.0f, ImVec2(0, 80));
        ImGui::Separator(ctx);

        // Animate a simple progress bar
        IMGUI_DEMO_MARKER("Widgets/Plotting/ProgressBar");
        static float progress = 0.0f, progress_dir = 1.0f;
        if (animate)
        {
            progress += progress_dir * 0.4f * ImGui::GetIO(ctx).DeltaTime;
            if (progress >= +1.1f) { progress = +1.1f; progress_dir *= -1.0f; }
            if (progress <= -0.1f) { progress = -0.1f; progress_dir *= -1.0f; }
        }

        // Typically we would use ImVec2(-1.0f,0.0f) or ImVec2(-FLT_MIN,0.0f) to use all available width,
        // or ImVec2(width,0.0f) for a specified width. ImVec2(0.0f,0.0f) uses ItemWidth.
        ImGui::ProgressBar(ctx, progress, ImVec2(0.0f, 0.0f));
        ImGui::SameLine(ctx, 0.0f, ImGui::GetStyle(ctx).ItemInnerSpacing.x);
        ImGui::Text(ctx, "Progress Bar");

        float progress_saturated = IM_CLAMP(progress, 0.0f, 1.0f);
        char buf[32];
        sprintf(buf, "%d/%d", (int)(progress_saturated * 1753), 1753);
        ImGui::ProgressBar(ctx, progress, ImVec2(0.f, 0.f), buf);
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Color");
    if (ImGui::TreeNode(ctx, "Color/Picker Widgets"))
    {
        static ImVec4 color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);

        static bool alpha_preview = true;
        static bool alpha_half_preview = false;
        static bool drag_and_drop = true;
        static bool options_menu = true;
        static bool hdr = false;
        ImGui::SeparatorText(ctx, "Options");
        ImGui::Checkbox(ctx, "With Alpha Preview", &alpha_preview);
        ImGui::Checkbox(ctx, "With Half Alpha Preview", &alpha_half_preview);
        ImGui::Checkbox(ctx, "With Drag and Drop", &drag_and_drop);
        ImGui::Checkbox(ctx, "With Options Menu", &options_menu); ImGui::SameLine(ctx); HelpMarker(ctx, "Right-click on the individual color widget to show options.");
        ImGui::Checkbox(ctx, "With HDR", &hdr); ImGui::SameLine(ctx); HelpMarker(ctx, "Currently all this does is to lift the 0..1 limits on dragging widgets.");
        ImGuiColorEditFlags misc_flags = (hdr ? ImGuiColorEditFlags_HDR : 0) | (drag_and_drop ? 0 : ImGuiColorEditFlags_NoDragDrop) | (alpha_half_preview ? ImGuiColorEditFlags_AlphaPreviewHalf : (alpha_preview ? ImGuiColorEditFlags_AlphaPreview : 0)) | (options_menu ? 0 : ImGuiColorEditFlags_NoOptions);

        IMGUI_DEMO_MARKER("Widgets/Color/ColorEdit");
        ImGui::SeparatorText(ctx, "Inline color editor");
        ImGui::Text(ctx, "Color widget:");
        ImGui::SameLine(ctx); HelpMarker(ctx, 
            "Click on the color square to open a color picker.\n"
            "CTRL+click on individual component to input value.\n");
        ImGui::ColorEdit3(ctx, "MyColor##1", (float*)&color, misc_flags);

        IMGUI_DEMO_MARKER("Widgets/Color/ColorEdit (HSV, with Alpha)");
        ImGui::Text(ctx, "Color widget HSV with Alpha:");
        ImGui::ColorEdit4(ctx, "MyColor##2", (float*)&color, ImGuiColorEditFlags_DisplayHSV | misc_flags);

        IMGUI_DEMO_MARKER("Widgets/Color/ColorEdit (float display)");
        ImGui::Text(ctx, "Color widget with Float Display:");
        ImGui::ColorEdit4(ctx, "MyColor##2f", (float*)&color, ImGuiColorEditFlags_Float | misc_flags);

        IMGUI_DEMO_MARKER("Widgets/Color/ColorButton (with Picker)");
        ImGui::Text(ctx, "Color button with Picker:");
        ImGui::SameLine(ctx); HelpMarker(ctx, 
            "With the ImGuiColorEditFlags_NoInputs flag you can hide all the slider/text inputs.\n"
            "With the ImGuiColorEditFlags_NoLabel flag you can pass a non-empty label which will only "
            "be used for the tooltip and picker popup.");
        ImGui::ColorEdit4(ctx, "MyColor##3", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | misc_flags);

        IMGUI_DEMO_MARKER("Widgets/Color/ColorButton (with custom Picker popup)");
        ImGui::Text(ctx, "Color button with Custom Picker Popup:");

        // Generate a default palette. The palette will persist and can be edited.
        static bool saved_palette_init = true;
        static ImVec4 saved_palette[32] = {};
        if (saved_palette_init)
        {
            for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
            {
                ImGui::ColorConvertHSVtoRGB(n / 31.0f, 0.8f, 0.8f,
                    saved_palette[n].x, saved_palette[n].y, saved_palette[n].z);
                saved_palette[n].w = 1.0f; // Alpha
            }
            saved_palette_init = false;
        }

        static ImVec4 backup_color;
        bool open_popup = ImGui::ColorButton(ctx, "MyColor##3b", color, misc_flags);
        ImGui::SameLine(ctx, 0, ImGui::GetStyle(ctx).ItemInnerSpacing.x);
        open_popup |= ImGui::Button(ctx, "Palette");
        if (open_popup)
        {
            ImGui::OpenPopup(ctx, "mypicker");
            backup_color = color;
        }
        if (ImGui::BeginPopup(ctx, "mypicker"))
        {
            ImGui::Text(ctx, "MY CUSTOM COLOR PICKER WITH AN AMAZING PALETTE!");
            ImGui::Separator(ctx);
            ImGui::ColorPicker4(ctx, "##picker", (float*)&color, misc_flags | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
            ImGui::SameLine(ctx);

            ImGui::BeginGroup(ctx); // Lock X position
            ImGui::Text(ctx, "Current");
            ImGui::ColorButton(ctx, "##current", color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
            ImGui::Text(ctx, "Previous");
            if (ImGui::ColorButton(ctx, "##previous", backup_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
                color = backup_color;
            ImGui::Separator(ctx);
            ImGui::Text(ctx, "Palette");
            for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
            {
                ImGui::PushID(ctx, n);
                if ((n % 8) != 0)
                    ImGui::SameLine(ctx, 0.0f, ImGui::GetStyle(ctx).ItemSpacing.y);

                ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
                if (ImGui::ColorButton(ctx, "##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
                    color = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, color.w); // Preserve alpha!

                // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
                // drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
                if (ImGui::BeginDragDropTarget(ctx))
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ctx, IMGUI_PAYLOAD_TYPE_COLOR_3F))
                        memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 3);
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ctx, IMGUI_PAYLOAD_TYPE_COLOR_4F))
                        memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 4);
                    ImGui::EndDragDropTarget(ctx);
                }

                ImGui::PopID(ctx);
            }
            ImGui::EndGroup(ctx);
            ImGui::EndPopup(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Color/ColorButton (simple)");
        ImGui::Text(ctx, "Color button only:");
        static bool no_border = false;
        ImGui::Checkbox(ctx, "ImGuiColorEditFlags_NoBorder", &no_border);
        ImGui::ColorButton(ctx, "MyColor##3c", *(ImVec4*)&color, misc_flags | (no_border ? ImGuiColorEditFlags_NoBorder : 0), ImVec2(80, 80));

        IMGUI_DEMO_MARKER("Widgets/Color/ColorPicker");
        ImGui::SeparatorText(ctx, "Color picker");
        static bool alpha = true;
        static bool alpha_bar = true;
        static bool side_preview = true;
        static bool ref_color = false;
        static ImVec4 ref_color_v(1.0f, 0.0f, 1.0f, 0.5f);
        static int display_mode = 0;
        static int picker_mode = 0;
        ImGui::Checkbox(ctx, "With Alpha", &alpha);
        ImGui::Checkbox(ctx, "With Alpha Bar", &alpha_bar);
        ImGui::Checkbox(ctx, "With Side Preview", &side_preview);
        if (side_preview)
        {
            ImGui::SameLine(ctx);
            ImGui::Checkbox(ctx, "With Ref Color", &ref_color);
            if (ref_color)
            {
                ImGui::SameLine(ctx);
                ImGui::ColorEdit4(ctx, "##RefColor", &ref_color_v.x, ImGuiColorEditFlags_NoInputs | misc_flags);
            }
        }
        ImGui::Combo(ctx, "Display Mode", &display_mode, "Auto/Current\0None\0RGB Only\0HSV Only\0Hex Only\0");
        ImGui::SameLine(ctx); HelpMarker(ctx, 
            "ColorEdit defaults to displaying RGB inputs if you don't specify a display mode, "
            "but the user can change it with a right-click on those inputs.\n\nColorPicker defaults to displaying RGB+HSV+Hex "
            "if you don't specify a display mode.\n\nYou can change the defaults using SetColorEditOptions().");
        ImGui::SameLine(ctx); HelpMarker(ctx, "When not specified explicitly (Auto/Current mode), user can right-click the picker to change mode.");
        ImGuiColorEditFlags flags = misc_flags;
        if (!alpha)            flags |= ImGuiColorEditFlags_NoAlpha;        // This is by default if you call ColorPicker3() instead of ColorPicker4()
        if (alpha_bar)         flags |= ImGuiColorEditFlags_AlphaBar;
        if (!side_preview)     flags |= ImGuiColorEditFlags_NoSidePreview;
        if (picker_mode == 1)  flags |= ImGuiColorEditFlags_PickerHueBar;
        if (picker_mode == 2)  flags |= ImGuiColorEditFlags_PickerHueWheel;
        if (display_mode == 1) flags |= ImGuiColorEditFlags_NoInputs;       // Disable all RGB/HSV/Hex displays
        if (display_mode == 2) flags |= ImGuiColorEditFlags_DisplayRGB;     // Override display mode
        if (display_mode == 3) flags |= ImGuiColorEditFlags_DisplayHSV;
        if (display_mode == 4) flags |= ImGuiColorEditFlags_DisplayHex;
        ImGui::ColorPicker4(ctx, "MyColor##4", (float*)&color, flags, ref_color ? &ref_color_v.x : NULL);

        ImGui::Text(ctx, "Set defaults in code:");
        ImGui::SameLine(ctx); HelpMarker(ctx, 
            "SetColorEditOptions() is designed to allow you to set boot-time default.\n"
            "We don't have Push/Pop functions because you can force options on a per-widget basis if needed,"
            "and the user can change non-forced ones with the options menu.\nWe don't have a getter to avoid"
            "encouraging you to persistently save values that aren't forward-compatible.");
        if (ImGui::Button(ctx, "Default: Uint8 + HSV + Hue Bar"))
            ImGui::SetColorEditOptions(ctx, ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_PickerHueBar);
        if (ImGui::Button(ctx, "Default: Float + HDR + Hue Wheel"))
            ImGui::SetColorEditOptions(ctx, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_PickerHueWheel);

        // Always both a small version of both types of pickers (to make it more visible in the demo to people who are skimming quickly through it)
        ImGui::Text(ctx, "Both types:");
        float w = (ImGui::GetContentRegionAvail(ctx).x - ImGui::GetStyle(ctx).ItemSpacing.y) * 0.40f;
        ImGui::SetNextItemWidth(ctx, w);
        ImGui::ColorPicker3(ctx, "##MyColor##5", (float*)&color, ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
        ImGui::SameLine(ctx);
        ImGui::SetNextItemWidth(ctx, w);
        ImGui::ColorPicker3(ctx, "##MyColor##6", (float*)&color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);

        // HSV encoded support (to avoid RGB<>HSV round trips and singularities when S==0 or V==0)
        static ImVec4 color_hsv(0.23f, 1.0f, 1.0f, 1.0f); // Stored as HSV!
        ImGui::Spacing(ctx);
        ImGui::Text(ctx, "HSV encoded colors");
        ImGui::SameLine(ctx); HelpMarker(ctx, 
            "By default, colors are given to ColorEdit and ColorPicker in RGB, but ImGuiColorEditFlags_InputHSV"
            "allows you to store colors as HSV and pass them to ColorEdit and ColorPicker as HSV. This comes with the"
            "added benefit that you can manipulate hue values with the picker even when saturation or value are zero.");
        ImGui::Text(ctx, "Color widget with InputHSV:");
        ImGui::ColorEdit4(ctx, "HSV shown as RGB##1", (float*)&color_hsv, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputHSV | ImGuiColorEditFlags_Float);
        ImGui::ColorEdit4(ctx, "HSV shown as HSV##1", (float*)&color_hsv, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputHSV | ImGuiColorEditFlags_Float);
        ImGui::DragFloat4(ctx, "Raw HSV values", (float*)&color_hsv, 0.01f, 0.0f, 1.0f);

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Drag and Slider Flags");
    if (ImGui::TreeNode(ctx, "Drag/Slider Flags"))
    {
        // Demonstrate using advanced flags for DragXXX and SliderXXX functions. Note that the flags are the same!
        static ImGuiSliderFlags flags = ImGuiSliderFlags_None;
        ImGui::CheckboxFlags(ctx, "ImGuiSliderFlags_AlwaysClamp", &flags, ImGuiSliderFlags_AlwaysClamp);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Always clamp value to min/max bounds (if any) when input manually with CTRL+Click.");
        ImGui::CheckboxFlags(ctx, "ImGuiSliderFlags_Logarithmic", &flags, ImGuiSliderFlags_Logarithmic);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Enable logarithmic editing (more precision for small values).");
        ImGui::CheckboxFlags(ctx, "ImGuiSliderFlags_NoRoundToFormat", &flags, ImGuiSliderFlags_NoRoundToFormat);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Disable rounding underlying value to match precision of the format string (e.g. %.3f values are rounded to those 3 digits).");
        ImGui::CheckboxFlags(ctx, "ImGuiSliderFlags_NoInput", &flags, ImGuiSliderFlags_NoInput);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Disable CTRL+Click or Enter key allowing to input text directly into the widget.");

        // Drags
        static float drag_f = 0.5f;
        static int drag_i = 50;
        ImGui::Text(ctx, "Underlying float value: %f", drag_f);
        ImGui::DragFloat(ctx, "DragFloat (0 -> 1)", &drag_f, 0.005f, 0.0f, 1.0f, "%.3f", flags);
        ImGui::DragFloat(ctx, "DragFloat (0 -> +inf)", &drag_f, 0.005f, 0.0f, FLT_MAX, "%.3f", flags);
        ImGui::DragFloat(ctx, "DragFloat (-inf -> 1)", &drag_f, 0.005f, -FLT_MAX, 1.0f, "%.3f", flags);
        ImGui::DragFloat(ctx, "DragFloat (-inf -> +inf)", &drag_f, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f", flags);
        ImGui::DragInt(ctx, "DragInt (0 -> 100)", &drag_i, 0.5f, 0, 100, "%d", flags);

        // Sliders
        static float slider_f = 0.5f;
        static int slider_i = 50;
        ImGui::Text(ctx, "Underlying float value: %f", slider_f);
        ImGui::SliderFloat(ctx, "SliderFloat (0 -> 1)", &slider_f, 0.0f, 1.0f, "%.3f", flags);
        ImGui::SliderInt(ctx, "SliderInt (0 -> 100)", &slider_i, 0, 100, "%d", flags);

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Range Widgets");
    if (ImGui::TreeNode(ctx, "Range Widgets"))
    {
        static float begin = 10, end = 90;
        static int begin_i = 100, end_i = 1000;
        ImGui::DragFloatRange2(ctx, "range float", &begin, &end, 0.25f, 0.0f, 100.0f, "Min: %.1f %%", "Max: %.1f %%", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragIntRange2(ctx, "range int", &begin_i, &end_i, 5, 0, 1000, "Min: %d units", "Max: %d units");
        ImGui::DragIntRange2(ctx, "range int (no bounds)", &begin_i, &end_i, 5, 0, 0, "Min: %d units", "Max: %d units");
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Data Types");
    if (ImGui::TreeNode(ctx, "Data Types"))
    {
        // DragScalar/InputScalar/SliderScalar functions allow various data types
        // - signed/unsigned
        // - 8/16/32/64-bits
        // - integer/float/double
        // To avoid polluting the public API with all possible combinations, we use the ImGuiDataType enum
        // to pass the type, and passing all arguments by pointer.
        // This is the reason the test code below creates local variables to hold "zero" "one" etc. for each type.
        // In practice, if you frequently use a given type that is not covered by the normal API entry points,
        // you can wrap it yourself inside a 1 line function which can take typed argument as value instead of void*,
        // and then pass their address to the generic function. For example:
        //   bool MySliderU64(const char *label, u64* value, u64 min = 0, u64 max = 0, const char* format = "%lld")
        //   {
        //      return SliderScalar(label, ImGuiDataType_U64, value, &min, &max, format);
        //   }

        // Setup limits (as helper variables so we can take their address, as explained above)
        // Note: SliderScalar() functions have a maximum usable range of half the natural type maximum, hence the /2.
        #ifndef LLONG_MIN
        ImS64 LLONG_MIN = -9223372036854775807LL - 1;
        ImS64 LLONG_MAX = 9223372036854775807LL;
        ImU64 ULLONG_MAX = (2ULL * 9223372036854775807LL + 1);
        #endif
        const char    s8_zero  = 0,   s8_one  = 1,   s8_fifty  = 50, s8_min  = -128,        s8_max = 127;
        const ImU8    u8_zero  = 0,   u8_one  = 1,   u8_fifty  = 50, u8_min  = 0,           u8_max = 255;
        const short   s16_zero = 0,   s16_one = 1,   s16_fifty = 50, s16_min = -32768,      s16_max = 32767;
        const ImU16   u16_zero = 0,   u16_one = 1,   u16_fifty = 50, u16_min = 0,           u16_max = 65535;
        const ImS32   s32_zero = 0,   s32_one = 1,   s32_fifty = 50, s32_min = INT_MIN/2,   s32_max = INT_MAX/2,    s32_hi_a = INT_MAX/2 - 100,    s32_hi_b = INT_MAX/2;
        const ImU32   u32_zero = 0,   u32_one = 1,   u32_fifty = 50, u32_min = 0,           u32_max = UINT_MAX/2,   u32_hi_a = UINT_MAX/2 - 100,   u32_hi_b = UINT_MAX/2;
        const ImS64   s64_zero = 0,   s64_one = 1,   s64_fifty = 50, s64_min = LLONG_MIN/2, s64_max = LLONG_MAX/2,  s64_hi_a = LLONG_MAX/2 - 100,  s64_hi_b = LLONG_MAX/2;
        const ImU64   u64_zero = 0,   u64_one = 1,   u64_fifty = 50, u64_min = 0,           u64_max = ULLONG_MAX/2, u64_hi_a = ULLONG_MAX/2 - 100, u64_hi_b = ULLONG_MAX/2;
        const float   f32_zero = 0.f, f32_one = 1.f, f32_lo_a = -10000000000.0f, f32_hi_a = +10000000000.0f;
        const double  f64_zero = 0.,  f64_one = 1.,  f64_lo_a = -1000000000000000.0, f64_hi_a = +1000000000000000.0;

        // State
        static char   s8_v  = 127;
        static ImU8   u8_v  = 255;
        static short  s16_v = 32767;
        static ImU16  u16_v = 65535;
        static ImS32  s32_v = -1;
        static ImU32  u32_v = (ImU32)-1;
        static ImS64  s64_v = -1;
        static ImU64  u64_v = (ImU64)-1;
        static float  f32_v = 0.123f;
        static double f64_v = 90000.01234567890123456789;

        const float drag_speed = 0.2f;
        static bool drag_clamp = false;
        IMGUI_DEMO_MARKER("Widgets/Data Types/Drags");
        ImGui::SeparatorText(ctx, "Drags");
        ImGui::Checkbox(ctx, "Clamp integers to 0..50", &drag_clamp);
        ImGui::SameLine(ctx); HelpMarker(ctx, 
            "As with every widget in dear imgui, we never modify values unless there is a user interaction.\n"
            "You can override the clamping limits by using CTRL+Click to input a value.");
        ImGui::DragScalar(ctx, "drag s8",        ImGuiDataType_S8,     &s8_v,  drag_speed, drag_clamp ? &s8_zero  : NULL, drag_clamp ? &s8_fifty  : NULL);
        ImGui::DragScalar(ctx, "drag u8",        ImGuiDataType_U8,     &u8_v,  drag_speed, drag_clamp ? &u8_zero  : NULL, drag_clamp ? &u8_fifty  : NULL, "%u ms");
        ImGui::DragScalar(ctx, "drag s16",       ImGuiDataType_S16,    &s16_v, drag_speed, drag_clamp ? &s16_zero : NULL, drag_clamp ? &s16_fifty : NULL);
        ImGui::DragScalar(ctx, "drag u16",       ImGuiDataType_U16,    &u16_v, drag_speed, drag_clamp ? &u16_zero : NULL, drag_clamp ? &u16_fifty : NULL, "%u ms");
        ImGui::DragScalar(ctx, "drag s32",       ImGuiDataType_S32,    &s32_v, drag_speed, drag_clamp ? &s32_zero : NULL, drag_clamp ? &s32_fifty : NULL);
        ImGui::DragScalar(ctx, "drag s32 hex",   ImGuiDataType_S32,    &s32_v, drag_speed, drag_clamp ? &s32_zero : NULL, drag_clamp ? &s32_fifty : NULL, "0x%08X");
        ImGui::DragScalar(ctx, "drag u32",       ImGuiDataType_U32,    &u32_v, drag_speed, drag_clamp ? &u32_zero : NULL, drag_clamp ? &u32_fifty : NULL, "%u ms");
        ImGui::DragScalar(ctx, "drag s64",       ImGuiDataType_S64,    &s64_v, drag_speed, drag_clamp ? &s64_zero : NULL, drag_clamp ? &s64_fifty : NULL);
        ImGui::DragScalar(ctx, "drag u64",       ImGuiDataType_U64,    &u64_v, drag_speed, drag_clamp ? &u64_zero : NULL, drag_clamp ? &u64_fifty : NULL);
        ImGui::DragScalar(ctx, "drag float",     ImGuiDataType_Float,  &f32_v, 0.005f,  &f32_zero, &f32_one, "%f");
        ImGui::DragScalar(ctx, "drag float log", ImGuiDataType_Float,  &f32_v, 0.005f,  &f32_zero, &f32_one, "%f", ImGuiSliderFlags_Logarithmic);
        ImGui::DragScalar(ctx, "drag double",    ImGuiDataType_Double, &f64_v, 0.0005f, &f64_zero, NULL,     "%.10f grams");
        ImGui::DragScalar(ctx, "drag double log",ImGuiDataType_Double, &f64_v, 0.0005f, &f64_zero, &f64_one, "0 < %.10f < 1", ImGuiSliderFlags_Logarithmic);

        IMGUI_DEMO_MARKER("Widgets/Data Types/Sliders");
        ImGui::SeparatorText(ctx, "Sliders");
        ImGui::SliderScalar(ctx, "slider s8 full",       ImGuiDataType_S8,     &s8_v,  &s8_min,   &s8_max,   "%d");
        ImGui::SliderScalar(ctx, "slider u8 full",       ImGuiDataType_U8,     &u8_v,  &u8_min,   &u8_max,   "%u");
        ImGui::SliderScalar(ctx, "slider s16 full",      ImGuiDataType_S16,    &s16_v, &s16_min,  &s16_max,  "%d");
        ImGui::SliderScalar(ctx, "slider u16 full",      ImGuiDataType_U16,    &u16_v, &u16_min,  &u16_max,  "%u");
        ImGui::SliderScalar(ctx, "slider s32 low",       ImGuiDataType_S32,    &s32_v, &s32_zero, &s32_fifty,"%d");
        ImGui::SliderScalar(ctx, "slider s32 high",      ImGuiDataType_S32,    &s32_v, &s32_hi_a, &s32_hi_b, "%d");
        ImGui::SliderScalar(ctx, "slider s32 full",      ImGuiDataType_S32,    &s32_v, &s32_min,  &s32_max,  "%d");
        ImGui::SliderScalar(ctx, "slider s32 hex",       ImGuiDataType_S32,    &s32_v, &s32_zero, &s32_fifty, "0x%04X");
        ImGui::SliderScalar(ctx, "slider u32 low",       ImGuiDataType_U32,    &u32_v, &u32_zero, &u32_fifty,"%u");
        ImGui::SliderScalar(ctx, "slider u32 high",      ImGuiDataType_U32,    &u32_v, &u32_hi_a, &u32_hi_b, "%u");
        ImGui::SliderScalar(ctx, "slider u32 full",      ImGuiDataType_U32,    &u32_v, &u32_min,  &u32_max,  "%u");
        ImGui::SliderScalar(ctx, "slider s64 low",       ImGuiDataType_S64,    &s64_v, &s64_zero, &s64_fifty,"%" PRId64);
        ImGui::SliderScalar(ctx, "slider s64 high",      ImGuiDataType_S64,    &s64_v, &s64_hi_a, &s64_hi_b, "%" PRId64);
        ImGui::SliderScalar(ctx, "slider s64 full",      ImGuiDataType_S64,    &s64_v, &s64_min,  &s64_max,  "%" PRId64);
        ImGui::SliderScalar(ctx, "slider u64 low",       ImGuiDataType_U64,    &u64_v, &u64_zero, &u64_fifty,"%" PRIu64 " ms");
        ImGui::SliderScalar(ctx, "slider u64 high",      ImGuiDataType_U64,    &u64_v, &u64_hi_a, &u64_hi_b, "%" PRIu64 " ms");
        ImGui::SliderScalar(ctx, "slider u64 full",      ImGuiDataType_U64,    &u64_v, &u64_min,  &u64_max,  "%" PRIu64 " ms");
        ImGui::SliderScalar(ctx, "slider float low",     ImGuiDataType_Float,  &f32_v, &f32_zero, &f32_one);
        ImGui::SliderScalar(ctx, "slider float low log", ImGuiDataType_Float,  &f32_v, &f32_zero, &f32_one,  "%.10f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderScalar(ctx, "slider float high",    ImGuiDataType_Float,  &f32_v, &f32_lo_a, &f32_hi_a, "%e");
        ImGui::SliderScalar(ctx, "slider double low",    ImGuiDataType_Double, &f64_v, &f64_zero, &f64_one,  "%.10f grams");
        ImGui::SliderScalar(ctx, "slider double low log",ImGuiDataType_Double, &f64_v, &f64_zero, &f64_one,  "%.10f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderScalar(ctx, "slider double high",   ImGuiDataType_Double, &f64_v, &f64_lo_a, &f64_hi_a, "%e grams");

        ImGui::SeparatorText(ctx, "Sliders (reverse)");
        ImGui::SliderScalar(ctx, "slider s8 reverse",    ImGuiDataType_S8,   &s8_v,  &s8_max,    &s8_min,   "%d");
        ImGui::SliderScalar(ctx, "slider u8 reverse",    ImGuiDataType_U8,   &u8_v,  &u8_max,    &u8_min,   "%u");
        ImGui::SliderScalar(ctx, "slider s32 reverse",   ImGuiDataType_S32,  &s32_v, &s32_fifty, &s32_zero, "%d");
        ImGui::SliderScalar(ctx, "slider u32 reverse",   ImGuiDataType_U32,  &u32_v, &u32_fifty, &u32_zero, "%u");
        ImGui::SliderScalar(ctx, "slider s64 reverse",   ImGuiDataType_S64,  &s64_v, &s64_fifty, &s64_zero, "%" PRId64);
        ImGui::SliderScalar(ctx, "slider u64 reverse",   ImGuiDataType_U64,  &u64_v, &u64_fifty, &u64_zero, "%" PRIu64 " ms");

        IMGUI_DEMO_MARKER("Widgets/Data Types/Inputs");
        static bool inputs_step = true;
        ImGui::SeparatorText(ctx, "Inputs");
        ImGui::Checkbox(ctx, "Show step buttons", &inputs_step);
        ImGui::InputScalar(ctx, "input s8",      ImGuiDataType_S8,     &s8_v,  inputs_step ? &s8_one  : NULL, NULL, "%d");
        ImGui::InputScalar(ctx, "input u8",      ImGuiDataType_U8,     &u8_v,  inputs_step ? &u8_one  : NULL, NULL, "%u");
        ImGui::InputScalar(ctx, "input s16",     ImGuiDataType_S16,    &s16_v, inputs_step ? &s16_one : NULL, NULL, "%d");
        ImGui::InputScalar(ctx, "input u16",     ImGuiDataType_U16,    &u16_v, inputs_step ? &u16_one : NULL, NULL, "%u");
        ImGui::InputScalar(ctx, "input s32",     ImGuiDataType_S32,    &s32_v, inputs_step ? &s32_one : NULL, NULL, "%d");
        ImGui::InputScalar(ctx, "input s32 hex", ImGuiDataType_S32,    &s32_v, inputs_step ? &s32_one : NULL, NULL, "%04X");
        ImGui::InputScalar(ctx, "input u32",     ImGuiDataType_U32,    &u32_v, inputs_step ? &u32_one : NULL, NULL, "%u");
        ImGui::InputScalar(ctx, "input u32 hex", ImGuiDataType_U32,    &u32_v, inputs_step ? &u32_one : NULL, NULL, "%08X");
        ImGui::InputScalar(ctx, "input s64",     ImGuiDataType_S64,    &s64_v, inputs_step ? &s64_one : NULL);
        ImGui::InputScalar(ctx, "input u64",     ImGuiDataType_U64,    &u64_v, inputs_step ? &u64_one : NULL);
        ImGui::InputScalar(ctx, "input float",   ImGuiDataType_Float,  &f32_v, inputs_step ? &f32_one : NULL);
        ImGui::InputScalar(ctx, "input double",  ImGuiDataType_Double, &f64_v, inputs_step ? &f64_one : NULL);

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Multi-component Widgets");
    if (ImGui::TreeNode(ctx, "Multi-component Widgets"))
    {
        static float vec4f[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
        static int vec4i[4] = { 1, 5, 100, 255 };

        ImGui::SeparatorText(ctx, "2-wide");
        ImGui::InputFloat2(ctx, "input float2", vec4f);
        ImGui::DragFloat2(ctx, "drag float2", vec4f, 0.01f, 0.0f, 1.0f);
        ImGui::SliderFloat2(ctx, "slider float2", vec4f, 0.0f, 1.0f);
        ImGui::InputInt2(ctx, "input int2", vec4i);
        ImGui::DragInt2(ctx, "drag int2", vec4i, 1, 0, 255);
        ImGui::SliderInt2(ctx, "slider int2", vec4i, 0, 255);

        ImGui::SeparatorText(ctx, "3-wide");
        ImGui::InputFloat3(ctx, "input float3", vec4f);
        ImGui::DragFloat3(ctx, "drag float3", vec4f, 0.01f, 0.0f, 1.0f);
        ImGui::SliderFloat3(ctx, "slider float3", vec4f, 0.0f, 1.0f);
        ImGui::InputInt3(ctx, "input int3", vec4i);
        ImGui::DragInt3(ctx, "drag int3", vec4i, 1, 0, 255);
        ImGui::SliderInt3(ctx, "slider int3", vec4i, 0, 255);

        ImGui::SeparatorText(ctx, "4-wide");
        ImGui::InputFloat4(ctx, "input float4", vec4f);
        ImGui::DragFloat4(ctx, "drag float4", vec4f, 0.01f, 0.0f, 1.0f);
        ImGui::SliderFloat4(ctx, "slider float4", vec4f, 0.0f, 1.0f);
        ImGui::InputInt4(ctx, "input int4", vec4i);
        ImGui::DragInt4(ctx, "drag int4", vec4i, 1, 0, 255);
        ImGui::SliderInt4(ctx, "slider int4", vec4i, 0, 255);

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Vertical Sliders");
    if (ImGui::TreeNode(ctx, "Vertical Sliders"))
    {
        const float spacing = 4;
        ImGui::PushStyleVar(ctx, ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

        static int int_value = 0;
        ImGui::VSliderInt(ctx, "##int", ImVec2(18, 160), &int_value, 0, 5);
        ImGui::SameLine(ctx);

        static float values[7] = { 0.0f, 0.60f, 0.35f, 0.9f, 0.70f, 0.20f, 0.0f };
        ImGui::PushID(ctx, "set1");
        for (int i = 0; i < 7; i++)
        {
            if (i > 0) ImGui::SameLine(ctx);
            ImGui::PushID(ctx, i);
            ImGui::PushStyleColor(ctx, ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(i / 7.0f, 0.5f, 0.5f));
            ImGui::PushStyleColor(ctx, ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(i / 7.0f, 0.6f, 0.5f));
            ImGui::PushStyleColor(ctx, ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(i / 7.0f, 0.7f, 0.5f));
            ImGui::PushStyleColor(ctx, ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(i / 7.0f, 0.9f, 0.9f));
            ImGui::VSliderFloat(ctx, "##v", ImVec2(18, 160), &values[i], 0.0f, 1.0f, "");
            if (ImGui::IsItemActive(ctx) || ImGui::IsItemHovered(ctx))
                ImGui::SetTooltip(ctx, "%.3f", values[i]);
            ImGui::PopStyleColor(ctx, 4);
            ImGui::PopID(ctx);
        }
        ImGui::PopID(ctx);

        ImGui::SameLine(ctx);
        ImGui::PushID(ctx, "set2");
        static float values2[4] = { 0.20f, 0.80f, 0.40f, 0.25f };
        const int rows = 3;
        const ImVec2 small_slider_size(18, (float)(int)((160.0f - (rows - 1) * spacing) / rows));
        for (int nx = 0; nx < 4; nx++)
        {
            if (nx > 0) ImGui::SameLine(ctx);
            ImGui::BeginGroup(ctx);
            for (int ny = 0; ny < rows; ny++)
            {
                ImGui::PushID(ctx, nx * rows + ny);
                ImGui::VSliderFloat(ctx, "##v", small_slider_size, &values2[nx], 0.0f, 1.0f, "");
                if (ImGui::IsItemActive(ctx) || ImGui::IsItemHovered(ctx))
                    ImGui::SetTooltip(ctx, "%.3f", values2[nx]);
                ImGui::PopID(ctx);
            }
            ImGui::EndGroup(ctx);
        }
        ImGui::PopID(ctx);

        ImGui::SameLine(ctx);
        ImGui::PushID(ctx, "set3");
        for (int i = 0; i < 4; i++)
        {
            if (i > 0) ImGui::SameLine(ctx);
            ImGui::PushID(ctx, i);
            ImGui::PushStyleVar(ctx, ImGuiStyleVar_GrabMinSize, 40);
            ImGui::VSliderFloat(ctx, "##v", ImVec2(40, 160), &values[i], 0.0f, 1.0f, "%.2f\nsec");
            ImGui::PopStyleVar(ctx);
            ImGui::PopID(ctx);
        }
        ImGui::PopID(ctx);
        ImGui::PopStyleVar(ctx);
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Drag and drop");
    if (ImGui::TreeNode(ctx, "Drag and Drop"))
    {
        IMGUI_DEMO_MARKER("Widgets/Drag and drop/Standard widgets");
        if (ImGui::TreeNode(ctx, "Drag and drop in standard widgets"))
        {
            // ColorEdit widgets automatically act as drag source and drag target.
            // They are using standardized payload strings IMGUI_PAYLOAD_TYPE_COLOR_3F and IMGUI_PAYLOAD_TYPE_COLOR_4F
            // to allow your own widgets to use colors in their drag and drop interaction.
            // Also see 'Demo->Widgets->Color/Picker Widgets->Palette' demo.
            HelpMarker(ctx, "You can drag from the color squares.");
            static float col1[3] = { 1.0f, 0.0f, 0.2f };
            static float col2[4] = { 0.4f, 0.7f, 0.0f, 0.5f };
            ImGui::ColorEdit3(ctx, "color 1", col1);
            ImGui::ColorEdit4(ctx, "color 2", col2);
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Drag and drop/Copy-swap items");
        if (ImGui::TreeNode(ctx, "Drag and drop to copy/swap items"))
        {
            enum Mode
            {
                Mode_Copy,
                Mode_Move,
                Mode_Swap
            };
            static int mode = 0;
            if (ImGui::RadioButton(ctx, "Copy", mode == Mode_Copy)) { mode = Mode_Copy; } ImGui::SameLine(ctx);
            if (ImGui::RadioButton(ctx, "Move", mode == Mode_Move)) { mode = Mode_Move; } ImGui::SameLine(ctx);
            if (ImGui::RadioButton(ctx, "Swap", mode == Mode_Swap)) { mode = Mode_Swap; }
            static const char* names[9] =
            {
                "Bobby", "Beatrice", "Betty",
                "Brianna", "Barry", "Bernard",
                "Bibi", "Blaine", "Bryn"
            };
            for (int n = 0; n < IM_ARRAYSIZE(names); n++)
            {
                ImGui::PushID(ctx, n);
                if ((n % 3) != 0)
                    ImGui::SameLine(ctx);
                ImGui::Button(ctx, names[n], ImVec2(60, 60));

                // Our buttons are both drag sources and drag targets here!
                if (ImGui::BeginDragDropSource(ctx, ImGuiDragDropFlags_None))
                {
                    // Set payload to carry the index of our item (could be anything)
                    ImGui::SetDragDropPayload(ctx, "DND_DEMO_CELL", &n, sizeof(int));

                    // Display preview (could be anything, e.g. when dragging an image we could decide to display
                    // the filename and a small preview of the image, etc.)
                    if (mode == Mode_Copy) { ImGui::Text(ctx, "Copy %s", names[n]); }
                    if (mode == Mode_Move) { ImGui::Text(ctx, "Move %s", names[n]); }
                    if (mode == Mode_Swap) { ImGui::Text(ctx, "Swap %s", names[n]); }
                    ImGui::EndDragDropSource(ctx);
                }
                if (ImGui::BeginDragDropTarget(ctx))
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ctx, "DND_DEMO_CELL"))
                    {
                        IM_ASSERT(payload->DataSize == sizeof(int));
                        int payload_n = *(const int*)payload->Data;
                        if (mode == Mode_Copy)
                        {
                            names[n] = names[payload_n];
                        }
                        if (mode == Mode_Move)
                        {
                            names[n] = names[payload_n];
                            names[payload_n] = "";
                        }
                        if (mode == Mode_Swap)
                        {
                            const char* tmp = names[n];
                            names[n] = names[payload_n];
                            names[payload_n] = tmp;
                        }
                    }
                    ImGui::EndDragDropTarget(ctx);
                }
                ImGui::PopID(ctx);
            }
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Drag and Drop/Drag to reorder items (simple)");
        if (ImGui::TreeNode(ctx, "Drag to reorder items (simple)"))
        {
            // Simple reordering
            HelpMarker(ctx, 
                "We don't use the drag and drop api at all here! "
                "Instead we query when the item is held but not hovered, and order items accordingly.");
            static const char* item_names[] = { "Item One", "Item Two", "Item Three", "Item Four", "Item Five" };
            for (int n = 0; n < IM_ARRAYSIZE(item_names); n++)
            {
                const char* item = item_names[n];
                ImGui::Selectable(ctx, item);

                if (ImGui::IsItemActive(ctx) && !ImGui::IsItemHovered(ctx))
                {
                    int n_next = n + (ImGui::GetMouseDragDelta(ctx, 0).y < 0.f ? -1 : 1);
                    if (n_next >= 0 && n_next < IM_ARRAYSIZE(item_names))
                    {
                        item_names[n] = item_names[n_next];
                        item_names[n_next] = item;
                        ImGui::ResetMouseDragDelta(ctx);
                    }
                }
            }
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Widgets/Drag and Drop/Tooltip at target location");
        if (ImGui::TreeNode(ctx, "Tooltip at target location"))
        {
            for (int n = 0; n < 2; n++)
            {
                // Drop targets
                ImGui::Button(ctx, n ? "drop here##1" : "drop here##0");
                if (ImGui::BeginDragDropTarget(ctx))
                {
                    ImGuiDragDropFlags drop_target_flags = ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoPreviewTooltip;
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ctx, IMGUI_PAYLOAD_TYPE_COLOR_4F, drop_target_flags))
                    {
                        IM_UNUSED(payload);
                        ImGui::SetMouseCursor(ctx, ImGuiMouseCursor_NotAllowed);
                        ImGui::BeginTooltip(ctx);
                        ImGui::Text(ctx, "Cannot drop here!");
                        ImGui::EndTooltip(ctx);
                    }
                    ImGui::EndDragDropTarget(ctx);
                }

                // Drop source
                static ImVec4 col4 = { 1.0f, 0.0f, 0.2f, 1.0f };
                if (n == 0)
                    ImGui::ColorButton(ctx, "drag me", col4);

            }
            ImGui::TreePop(ctx);
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Querying Item Status (Edited,Active,Hovered etc.)");
    if (ImGui::TreeNode(ctx, "Querying Item Status (Edited/Active/Hovered etc.)"))
    {
        // Select an item type
        const char* item_names[] =
        {
            "Text", "Button", "Button (w/ repeat)", "Checkbox", "SliderFloat", "InputText", "InputTextMultiline", "InputFloat",
            "InputFloat3", "ColorEdit4", "Selectable", "MenuItem", "TreeNode", "TreeNode (w/ double-click)", "Combo", "ListBox"
        };
        static int item_type = 4;
        static bool item_disabled = false;
        ImGui::Combo(ctx, "Item Type", &item_type, item_names, IM_ARRAYSIZE(item_names), IM_ARRAYSIZE(item_names));
        ImGui::SameLine(ctx);
        HelpMarker(ctx, "Testing how various types of items are interacting with the IsItemXXX functions. Note that the bool return value of most ImGui function is generally equivalent to calling ImGui::IsItemHovered().");
        ImGui::Checkbox(ctx, "Item Disabled",  &item_disabled);

        // Submit selected items so we can query their status in the code following it.
        bool ret = false;
        static bool b = false;
        static float col4f[4] = { 1.0f, 0.5, 0.0f, 1.0f };
        static char str[16] = {};
        if (item_disabled)
            ImGui::BeginDisabled(ctx, true);
        if (item_type == 0) { ImGui::Text(ctx, "ITEM: Text"); }                                              // Testing text items with no identifier/interaction
        if (item_type == 1) { ret = ImGui::Button(ctx, "ITEM: Button"); }                                    // Testing button
        if (item_type == 2) { ImGui::PushButtonRepeat(ctx, true); ret = ImGui::Button(ctx, "ITEM: Button"); ImGui::PopButtonRepeat(ctx); } // Testing button (with repeater)
        if (item_type == 3) { ret = ImGui::Checkbox(ctx, "ITEM: Checkbox", &b); }                            // Testing checkbox
        if (item_type == 4) { ret = ImGui::SliderFloat(ctx, "ITEM: SliderFloat", &col4f[0], 0.0f, 1.0f); }   // Testing basic item
        if (item_type == 5) { ret = ImGui::InputText(ctx, "ITEM: InputText", &str[0], IM_ARRAYSIZE(str)); }  // Testing input text (which handles tabbing)
        if (item_type == 6) { ret = ImGui::InputTextMultiline(ctx, "ITEM: InputTextMultiline", &str[0], IM_ARRAYSIZE(str)); } // Testing input text (which uses a child window)
        if (item_type == 7) { ret = ImGui::InputFloat(ctx, "ITEM: InputFloat", col4f, 1.0f); }               // Testing +/- buttons on scalar input
        if (item_type == 8) { ret = ImGui::InputFloat3(ctx, "ITEM: InputFloat3", col4f); }                   // Testing multi-component items (IsItemXXX flags are reported merged)
        if (item_type == 9) { ret = ImGui::ColorEdit4(ctx, "ITEM: ColorEdit4", col4f); }                     // Testing multi-component items (IsItemXXX flags are reported merged)
        if (item_type == 10){ ret = ImGui::Selectable(ctx, "ITEM: Selectable"); }                            // Testing selectable item
        if (item_type == 11){ ret = ImGui::MenuItem(ctx, "ITEM: MenuItem"); }                                // Testing menu item (they use ImGuiButtonFlags_PressedOnRelease button policy)
        if (item_type == 12){ ret = ImGui::TreeNode(ctx, "ITEM: TreeNode"); if (ret) ImGui::TreePop(ctx); }     // Testing tree node
        if (item_type == 13){ ret = ImGui::TreeNodeEx(ctx, "ITEM: TreeNode w/ ImGuiTreeNodeFlags_OpenOnDoubleClick", ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_NoTreePushOnOpen); } // Testing tree node with ImGuiButtonFlags_PressedOnDoubleClick button policy.
        if (item_type == 14){ const char* items[] = { "Apple", "Banana", "Cherry", "Kiwi" }; static int current = 1; ret = ImGui::Combo(ctx, "ITEM: Combo", &current, items, IM_ARRAYSIZE(items)); }
        if (item_type == 15){ const char* items[] = { "Apple", "Banana", "Cherry", "Kiwi" }; static int current = 1; ret = ImGui::ListBox(ctx, "ITEM: ListBox", &current, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items)); }

        bool hovered_delay_none = ImGui::IsItemHovered(ctx);
        bool hovered_delay_stationary = ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_Stationary);
        bool hovered_delay_short = ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_DelayShort);
        bool hovered_delay_normal = ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_DelayNormal);
        bool hovered_delay_tooltip = ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_ForTooltip); // = Normal + Stationary

        // Display the values of IsItemHovered() and other common item state functions.
        // Note that the ImGuiHoveredFlags_XXX flags can be combined.
        // Because BulletText is an item itself and that would affect the output of IsItemXXX functions,
        // we query every state in a single call to avoid storing them and to simplify the code.
        ImGui::BulletText(ctx, 
            "Return value = %d\n"
            "IsItemFocused() = %d\n"
            "IsItemHovered() = %d\n"
            "IsItemHovered(_AllowWhenBlockedByPopup) = %d\n"
            "IsItemHovered(_AllowWhenBlockedByActiveItem) = %d\n"
            "IsItemHovered(_AllowWhenOverlappedByItem) = %d\n"
            "IsItemHovered(_AllowWhenOverlappedByWindow) = %d\n"
            "IsItemHovered(_AllowWhenDisabled) = %d\n"
            "IsItemHovered(_RectOnly) = %d\n"
            "IsItemActive() = %d\n"
            "IsItemEdited() = %d\n"
            "IsItemActivated() = %d\n"
            "IsItemDeactivated() = %d\n"
            "IsItemDeactivatedAfterEdit() = %d\n"
            "IsItemVisible() = %d\n"
            "IsItemClicked() = %d\n"
            "IsItemToggledOpen() = %d\n"
            "GetItemRectMin() = (%.1f, %.1f)\n"
            "GetItemRectMax() = (%.1f, %.1f)\n"
            "GetItemRectSize() = (%.1f, %.1f)",
            ret,
            ImGui::IsItemFocused(ctx),
            ImGui::IsItemHovered(ctx),
            ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_AllowWhenBlockedByPopup),
            ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_AllowWhenBlockedByActiveItem),
            ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_AllowWhenOverlappedByItem),
            ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_AllowWhenOverlappedByWindow),
            ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_AllowWhenDisabled),
            ImGui::IsItemHovered(ctx, ImGuiHoveredFlags_RectOnly),
            ImGui::IsItemActive(ctx),
            ImGui::IsItemEdited(ctx),
            ImGui::IsItemActivated(ctx),
            ImGui::IsItemDeactivated(ctx),
            ImGui::IsItemDeactivatedAfterEdit(ctx),
            ImGui::IsItemVisible(ctx),
            ImGui::IsItemClicked(ctx),
            ImGui::IsItemToggledOpen(ctx),
            ImGui::GetItemRectMin(ctx).x, ImGui::GetItemRectMin(ctx).y,
            ImGui::GetItemRectMax(ctx).x, ImGui::GetItemRectMax(ctx).y,
            ImGui::GetItemRectSize(ctx).x, ImGui::GetItemRectSize(ctx).y
        );
        ImGui::BulletText(ctx, 
            "with Hovering Delay or Stationary test:\n"
            "IsItemHovered() = = %d\n"
            "IsItemHovered(_Stationary) = %d\n"
            "IsItemHovered(_DelayShort) = %d\n"
            "IsItemHovered(_DelayNormal) = %d\n"
            "IsItemHovered(_Tooltip) = %d",
            hovered_delay_none, hovered_delay_stationary, hovered_delay_short, hovered_delay_normal, hovered_delay_tooltip);

        if (item_disabled)
            ImGui::EndDisabled(ctx);

        char buf[1] = "";
        ImGui::InputText(ctx, "unused", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine(ctx);
        HelpMarker(ctx, "This widget is only here to be able to tab-out of the widgets above and see e.g. Deactivated() status.");

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Querying Window Status (Focused,Hovered etc.)");
    if (ImGui::TreeNode(ctx, "Querying Window Status (Focused/Hovered etc.)"))
    {
        static bool embed_all_inside_a_child_window = false;
        ImGui::Checkbox(ctx, "Embed everything inside a child window for testing _RootWindow flag.", &embed_all_inside_a_child_window);
        if (embed_all_inside_a_child_window)
            ImGui::BeginChild(ctx, "outer_child", ImVec2(0, ImGui::GetFontSize(ctx) * 20.0f), ImGuiChildFlags_Border);

        // Testing IsWindowFocused() function with its various flags.
        ImGui::BulletText(ctx, 
            "IsWindowFocused() = %d\n"
            "IsWindowFocused(_ChildWindows) = %d\n"
            "IsWindowFocused(_ChildWindows|_NoPopupHierarchy) = %d\n"
            "IsWindowFocused(_ChildWindows|_RootWindow) = %d\n"
            "IsWindowFocused(_ChildWindows|_RootWindow|_NoPopupHierarchy) = %d\n"
            "IsWindowFocused(_RootWindow) = %d\n"
            "IsWindowFocused(_RootWindow|_NoPopupHierarchy) = %d\n"
            "IsWindowFocused(_AnyWindow) = %d\n",
            ImGui::IsWindowFocused(ctx),
            ImGui::IsWindowFocused(ctx, ImGuiFocusedFlags_ChildWindows),
            ImGui::IsWindowFocused(ctx, ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_NoPopupHierarchy),
            ImGui::IsWindowFocused(ctx, ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_RootWindow),
            ImGui::IsWindowFocused(ctx, ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_RootWindow | ImGuiFocusedFlags_NoPopupHierarchy),
            ImGui::IsWindowFocused(ctx, ImGuiFocusedFlags_RootWindow),
            ImGui::IsWindowFocused(ctx, ImGuiFocusedFlags_RootWindow | ImGuiFocusedFlags_NoPopupHierarchy),
            ImGui::IsWindowFocused(ctx, ImGuiFocusedFlags_AnyWindow));

        // Testing IsWindowHovered() function with its various flags.
        ImGui::BulletText(ctx, 
            "IsWindowHovered() = %d\n"
            "IsWindowHovered(_AllowWhenBlockedByPopup) = %d\n"
            "IsWindowHovered(_AllowWhenBlockedByActiveItem) = %d\n"
            "IsWindowHovered(_ChildWindows) = %d\n"
            "IsWindowHovered(_ChildWindows|_NoPopupHierarchy) = %d\n"
            "IsWindowHovered(_ChildWindows|_RootWindow) = %d\n"
            "IsWindowHovered(_ChildWindows|_RootWindow|_NoPopupHierarchy) = %d\n"
            "IsWindowHovered(_RootWindow) = %d\n"
            "IsWindowHovered(_RootWindow|_NoPopupHierarchy) = %d\n"
            "IsWindowHovered(_ChildWindows|_AllowWhenBlockedByPopup) = %d\n"
            "IsWindowHovered(_AnyWindow) = %d\n"
            "IsWindowHovered(_Stationary) = %d\n",
            ImGui::IsWindowHovered(ctx),
            ImGui::IsWindowHovered(ctx, ImGuiHoveredFlags_AllowWhenBlockedByPopup),
            ImGui::IsWindowHovered(ctx, ImGuiHoveredFlags_AllowWhenBlockedByActiveItem),
            ImGui::IsWindowHovered(ctx, ImGuiHoveredFlags_ChildWindows),
            ImGui::IsWindowHovered(ctx, ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_NoPopupHierarchy),
            ImGui::IsWindowHovered(ctx, ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_RootWindow),
            ImGui::IsWindowHovered(ctx, ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_NoPopupHierarchy),
            ImGui::IsWindowHovered(ctx, ImGuiHoveredFlags_RootWindow),
            ImGui::IsWindowHovered(ctx, ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_NoPopupHierarchy),
            ImGui::IsWindowHovered(ctx, ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup),
            ImGui::IsWindowHovered(ctx, ImGuiHoveredFlags_AnyWindow),
            ImGui::IsWindowHovered(ctx, ImGuiHoveredFlags_Stationary));

        ImGui::BeginChild(ctx, "child", ImVec2(0, 50), ImGuiChildFlags_Border);
        ImGui::Text(ctx, "This is another child window for testing the _ChildWindows flag.");
        ImGui::EndChild(ctx);
        if (embed_all_inside_a_child_window)
            ImGui::EndChild(ctx);

        // Calling IsItemHovered() after begin returns the hovered status of the title bar.
        // This is useful in particular if you want to create a context menu associated to the title bar of a window.
        static bool test_window = false;
        ImGui::Checkbox(ctx, "Hovered/Active tests after Begin() for title bar testing", &test_window);
        if (test_window)
        {
            ImGui::Begin(ctx, "Title bar Hovered/Active tests", &test_window);
            if (ImGui::BeginPopupContextItem(ctx)) // <-- This is using IsItemHovered()
            {
                if (ImGui::MenuItem(ctx, "Close")) { test_window = false; }
                ImGui::EndPopup(ctx);
            }
            ImGui::Text(ctx, 
                "IsItemHovered() after begin = %d (== is title bar hovered)\n"
                "IsItemActive() after begin = %d (== is window being clicked/moved)\n",
                ImGui::IsItemHovered(ctx), ImGui::IsItemActive(ctx));
            ImGui::End(ctx);
        }

        ImGui::TreePop(ctx);
    }

    // Demonstrate BeginDisabled/EndDisabled using a checkbox located at the bottom of the section (which is a bit odd:
    // logically we'd have this checkbox at the top of the section, but we don't want this feature to steal that space)
    if (disable_all)
        ImGui::EndDisabled(ctx);

    IMGUI_DEMO_MARKER("Widgets/Disable Block");
    if (ImGui::TreeNode(ctx, "Disable block"))
    {
        ImGui::Checkbox(ctx, "Disable entire section above", &disable_all);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Demonstrate using BeginDisabled()/EndDisabled() across this section.");
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Widgets/Text Filter");
    if (ImGui::TreeNode(ctx, "Text Filter"))
    {
        // Helper class to easy setup a text filter.
        // You may want to implement a more feature-full filtering scheme in your own application.
        HelpMarker(ctx, "Not a widget per-se, but ImGuiTextFilter is a helper to perform simple filtering on text strings.");
        static ImGuiTextFilter filter;
        ImGui::Text(ctx, "Filter usage:\n"
            "  \"\"         display all lines\n"
            "  \"xxx\"      display lines containing \"xxx\"\n"
            "  \"xxx,yyy\"  display lines containing \"xxx\" or \"yyy\"\n"
            "  \"-xxx\"     hide lines containing \"xxx\"");
        filter.Draw();
        const char* lines[] = { "aaa1.c", "bbb1.c", "ccc1.c", "aaa2.cpp", "bbb2.cpp", "ccc2.cpp", "abc.h", "hello, world" };
        for (int i = 0; i < IM_ARRAYSIZE(lines); i++)
            if (filter.PassFilter(lines[i]))
                ImGui::BulletText(ctx, "%s", lines[i]);
        ImGui::TreePop(ctx);
    }
}

static void ShowDemoWindowLayout(ImGuiContext* ctx)
{
    IMGUI_DEMO_MARKER("Layout");
    if (!ImGui::CollapsingHeader(ctx, "Layout & Scrolling"))
        return;

    IMGUI_DEMO_MARKER("Layout/Child windows");
    if (ImGui::TreeNode(ctx, "Child windows"))
    {
        ImGui::SeparatorText(ctx, "Child windows");

        HelpMarker(ctx, "Use child windows to begin into a self-contained independent scrolling/clipping regions within a host window.");
        static bool disable_mouse_wheel = false;
        static bool disable_menu = false;
        ImGui::Checkbox(ctx, "Disable Mouse Wheel", &disable_mouse_wheel);
        ImGui::Checkbox(ctx, "Disable Menu", &disable_menu);

        // Child 1: no border, enable horizontal scrollbar
        {
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
            if (disable_mouse_wheel)
                window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
            ImGui::BeginChild(ctx, "ChildL", ImVec2(ImGui::GetContentRegionAvail(ctx).x * 0.5f, 260), ImGuiChildFlags_None, window_flags);
            for (int i = 0; i < 100; i++)
                ImGui::Text(ctx, "%04d: scrollable region", i);
            ImGui::EndChild(ctx);
        }

        ImGui::SameLine(ctx);

        // Child 2: rounded border
        {
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
            if (disable_mouse_wheel)
                window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
            if (!disable_menu)
                window_flags |= ImGuiWindowFlags_MenuBar;
            ImGui::PushStyleVar(ctx, ImGuiStyleVar_ChildRounding, 5.0f);
            ImGui::BeginChild(ctx, "ChildR", ImVec2(0, 260), ImGuiChildFlags_Border, window_flags);
            if (!disable_menu && ImGui::BeginMenuBar(ctx))
            {
                if (ImGui::BeginMenu(ctx, "Menu"))
                {
                    ShowExampleMenuFile(ctx);
                    ImGui::EndMenu(ctx);
                }
                ImGui::EndMenuBar(ctx);
            }
            if (ImGui::BeginTable(ctx, "split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
            {
                for (int i = 0; i < 100; i++)
                {
                    char buf[32];
                    sprintf(buf, "%03d", i);
                    ImGui::TableNextColumn(ctx);
                    ImGui::Button(ctx, buf, ImVec2(-FLT_MIN, 0.0f));
                }
                ImGui::EndTable(ctx);
            }
            ImGui::EndChild(ctx);
            ImGui::PopStyleVar(ctx);
        }

        // Child 3: manual-resize
        ImGui::SeparatorText(ctx, "Manual-resize");
        {
            HelpMarker(ctx, "Drag bottom border to resize. Double-click bottom border to auto-fit to vertical contents.");
            ImGui::PushStyleColor(ctx, ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ctx, ImGuiCol_FrameBg));
            if (ImGui::BeginChild(ctx, "ResizableChild", ImVec2(-FLT_MIN, ImGui::GetTextLineHeightWithSpacing(ctx) * 8), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY))
                for (int n = 0; n < 10; n++)
                    ImGui::Text(ctx, "Line %04d", n);
            ImGui::PopStyleColor(ctx);
            ImGui::EndChild(ctx);
        }

        // Child 4: auto-resizing height with a limit
        ImGui::SeparatorText(ctx, "Auto-resize with constraints");
        {
            static int draw_lines = 3;
            static int max_height_in_lines = 10;
            ImGui::SetNextItemWidth(ctx, ImGui::GetFontSize(ctx) * 8);
            ImGui::DragInt(ctx, "Lines Count", &draw_lines, 0.2f);
            ImGui::SetNextItemWidth(ctx, ImGui::GetFontSize(ctx) * 8);
            ImGui::DragInt(ctx, "Max Height (in Lines)", &max_height_in_lines, 0.2f);

            ImGui::SetNextWindowSizeConstraints(ctx, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing(ctx) * 1), ImVec2(FLT_MAX, ImGui::GetTextLineHeightWithSpacing(ctx) * max_height_in_lines));
            if (ImGui::BeginChild(ctx, "ConstrainedChild", ImVec2(-FLT_MIN, 0.0f), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY))
                for (int n = 0; n < draw_lines; n++)
                    ImGui::Text(ctx, "Line %04d", n);
            ImGui::EndChild(ctx);
        }

        ImGui::SeparatorText(ctx, "Misc/Advanced");

        // Demonstrate a few extra things
        // - Changing ImGuiCol_ChildBg (which is transparent black in default styles)
        // - Using SetCursorPos() to position child window (the child window is an item from the POV of parent window)
        //   You can also call SetNextWindowPos() to position the child window. The parent window will effectively
        //   layout from this position.
        // - Using ImGui::GetItemRectMin/Max() to query the "item" state (because the child window is an item from
        //   the POV of the parent window). See 'Demo->Querying Status (Edited/Active/Hovered etc.)' for details.
        {
            static int offset_x = 0;
            static bool override_bg_color = true;
            static ImGuiChildFlags child_flags = ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY;
            ImGui::SetNextItemWidth(ctx, ImGui::GetFontSize(ctx) * 8);
            ImGui::DragInt(ctx, "Offset X", &offset_x, 1.0f, -1000, 1000);
            ImGui::Checkbox(ctx, "Override ChildBg color", &override_bg_color);
            ImGui::CheckboxFlags(ctx, "ImGuiChildFlags_Border", &child_flags, ImGuiChildFlags_Border);
            ImGui::CheckboxFlags(ctx, "ImGuiChildFlags_AlwaysUseWindowPadding", &child_flags, ImGuiChildFlags_AlwaysUseWindowPadding);
            ImGui::CheckboxFlags(ctx, "ImGuiChildFlags_ResizeX", &child_flags, ImGuiChildFlags_ResizeX);
            ImGui::CheckboxFlags(ctx, "ImGuiChildFlags_ResizeY", &child_flags, ImGuiChildFlags_ResizeY);
            ImGui::CheckboxFlags(ctx, "ImGuiChildFlags_FrameStyle", &child_flags, ImGuiChildFlags_FrameStyle);
            ImGui::SameLine(ctx); HelpMarker(ctx, "Style the child window like a framed item: use FrameBg, FrameRounding, FrameBorderSize, FramePadding instead of ChildBg, ChildRounding, ChildBorderSize, WindowPadding.");
            if (child_flags & ImGuiChildFlags_FrameStyle)
                override_bg_color = false;

            ImGui::SetCursorPosX(ctx, ImGui::GetCursorPosX(ctx) + (float)offset_x);
            if (override_bg_color)
                ImGui::PushStyleColor(ctx, ImGuiCol_ChildBg, IM_COL32(255, 0, 0, 100));
            ImGui::BeginChild(ctx, "Red", ImVec2(200, 100), child_flags, ImGuiWindowFlags_None);
            if (override_bg_color)
                ImGui::PopStyleColor(ctx);

            for (int n = 0; n < 50; n++)
                ImGui::Text(ctx, "Some test %d", n);
            ImGui::EndChild(ctx);
            bool child_is_hovered = ImGui::IsItemHovered(ctx);
            ImVec2 child_rect_min = ImGui::GetItemRectMin(ctx);
            ImVec2 child_rect_max = ImGui::GetItemRectMax(ctx);
            ImGui::Text(ctx, "Hovered: %d", child_is_hovered);
            ImGui::Text(ctx, "Rect of child window is: (%.0f,%.0f) (%.0f,%.0f)", child_rect_min.x, child_rect_min.y, child_rect_max.x, child_rect_max.y);
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Layout/Widgets Width");
    if (ImGui::TreeNode(ctx, "Widgets Width"))
    {
        static float f = 0.0f;
        static bool show_indented_items = true;
        ImGui::Checkbox(ctx, "Show indented items", &show_indented_items);

        // Use SetNextItemWidth() to set the width of a single upcoming item.
        // Use PushItemWidth()/PopItemWidth() to set the width of a group of items.
        // In real code use you'll probably want to choose width values that are proportional to your font size
        // e.g. Using '20.0f * GetFontSize()' as width instead of '200.0f', etc.

        ImGui::Text(ctx, "SetNextItemWidth/PushItemWidth(100)");
        ImGui::SameLine(ctx); HelpMarker(ctx, "Fixed width.");
        ImGui::PushItemWidth(ctx, 100);
        ImGui::DragFloat(ctx, "float##1b", &f);
        if (show_indented_items)
        {
            ImGui::Indent(ctx);
            ImGui::DragFloat(ctx, "float (indented)##1b", &f);
            ImGui::Unindent(ctx);
        }
        ImGui::PopItemWidth(ctx);

        ImGui::Text(ctx, "SetNextItemWidth/PushItemWidth(-100)");
        ImGui::SameLine(ctx); HelpMarker(ctx, "Align to right edge minus 100");
        ImGui::PushItemWidth(ctx, -100);
        ImGui::DragFloat(ctx, "float##2a", &f);
        if (show_indented_items)
        {
            ImGui::Indent(ctx);
            ImGui::DragFloat(ctx, "float (indented)##2b", &f);
            ImGui::Unindent(ctx);
        }
        ImGui::PopItemWidth(ctx);

        ImGui::Text(ctx, "SetNextItemWidth/PushItemWidth(GetContentRegionAvail().x * 0.5f)");
        ImGui::SameLine(ctx); HelpMarker(ctx, "Half of available width.\n(~ right-cursor_pos)\n(works within a column set)");
        ImGui::PushItemWidth(ctx, ImGui::GetContentRegionAvail(ctx).x * 0.5f);
        ImGui::DragFloat(ctx, "float##3a", &f);
        if (show_indented_items)
        {
            ImGui::Indent(ctx);
            ImGui::DragFloat(ctx, "float (indented)##3b", &f);
            ImGui::Unindent(ctx);
        }
        ImGui::PopItemWidth(ctx);

        ImGui::Text(ctx, "SetNextItemWidth/PushItemWidth(-GetContentRegionAvail().x * 0.5f)");
        ImGui::SameLine(ctx); HelpMarker(ctx, "Align to right edge minus half");
        ImGui::PushItemWidth(ctx, -ImGui::GetContentRegionAvail(ctx).x * 0.5f);
        ImGui::DragFloat(ctx, "float##4a", &f);
        if (show_indented_items)
        {
            ImGui::Indent(ctx);
            ImGui::DragFloat(ctx, "float (indented)##4b", &f);
            ImGui::Unindent(ctx);
        }
        ImGui::PopItemWidth(ctx);

        // Demonstrate using PushItemWidth to surround three items.
        // Calling SetNextItemWidth() before each of them would have the same effect.
        ImGui::Text(ctx, "SetNextItemWidth/PushItemWidth(-FLT_MIN)");
        ImGui::SameLine(ctx); HelpMarker(ctx, "Align to right edge");
        ImGui::PushItemWidth(ctx, -FLT_MIN);
        ImGui::DragFloat(ctx, "##float5a", &f);
        if (show_indented_items)
        {
            ImGui::Indent(ctx);
            ImGui::DragFloat(ctx, "float (indented)##5b", &f);
            ImGui::Unindent(ctx);
        }
        ImGui::PopItemWidth(ctx);

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Layout/Basic Horizontal Layout");
    if (ImGui::TreeNode(ctx, "Basic Horizontal Layout"))
    {
        ImGui::TextWrapped(ctx, "(Use ImGui::SameLine() to keep adding items to the right of the preceding item)");

        // Text
        IMGUI_DEMO_MARKER("Layout/Basic Horizontal Layout/SameLine");
        ImGui::Text(ctx, "Two items: Hello"); ImGui::SameLine(ctx);
        ImGui::TextColored(ctx, ImVec4(1, 1, 0, 1), "Sailor");

        // Adjust spacing
        ImGui::Text(ctx, "More spacing: Hello"); ImGui::SameLine(ctx, 0, 20);
        ImGui::TextColored(ctx, ImVec4(1, 1, 0, 1), "Sailor");

        // Button
        ImGui::AlignTextToFramePadding(ctx);
        ImGui::Text(ctx, "Normal buttons"); ImGui::SameLine(ctx);
        ImGui::Button(ctx, "Banana"); ImGui::SameLine(ctx);
        ImGui::Button(ctx, "Apple"); ImGui::SameLine(ctx);
        ImGui::Button(ctx, "Corniflower");

        // Button
        ImGui::Text(ctx, "Small buttons"); ImGui::SameLine(ctx);
        ImGui::SmallButton(ctx, "Like this one"); ImGui::SameLine(ctx);
        ImGui::Text(ctx, "can fit within a text block.");

        // Aligned to arbitrary position. Easy/cheap column.
        IMGUI_DEMO_MARKER("Layout/Basic Horizontal Layout/SameLine (with offset)");
        ImGui::Text(ctx, "Aligned");
        ImGui::SameLine(ctx, 150); ImGui::Text(ctx, "x=150");
        ImGui::SameLine(ctx, 300); ImGui::Text(ctx, "x=300");
        ImGui::Text(ctx, "Aligned");
        ImGui::SameLine(ctx, 150); ImGui::SmallButton(ctx, "x=150");
        ImGui::SameLine(ctx, 300); ImGui::SmallButton(ctx, "x=300");

        // Checkbox
        IMGUI_DEMO_MARKER("Layout/Basic Horizontal Layout/SameLine (more)");
        static bool c1 = false, c2 = false, c3 = false, c4 = false;
        ImGui::Checkbox(ctx, "My", &c1); ImGui::SameLine(ctx);
        ImGui::Checkbox(ctx, "Tailor", &c2); ImGui::SameLine(ctx);
        ImGui::Checkbox(ctx, "Is", &c3); ImGui::SameLine(ctx);
        ImGui::Checkbox(ctx, "Rich", &c4);

        // Various
        static float f0 = 1.0f, f1 = 2.0f, f2 = 3.0f;
        ImGui::PushItemWidth(ctx, 80);
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD" };
        static int item = -1;
        ImGui::Combo(ctx, "Combo", &item, items, IM_ARRAYSIZE(items)); ImGui::SameLine(ctx);
        ImGui::SliderFloat(ctx, "X", &f0, 0.0f, 5.0f); ImGui::SameLine(ctx);
        ImGui::SliderFloat(ctx, "Y", &f1, 0.0f, 5.0f); ImGui::SameLine(ctx);
        ImGui::SliderFloat(ctx, "Z", &f2, 0.0f, 5.0f);
        ImGui::PopItemWidth(ctx);

        ImGui::PushItemWidth(ctx, 80);
        ImGui::Text(ctx, "Lists:");
        static int selection[4] = { 0, 1, 2, 3 };
        for (int i = 0; i < 4; i++)
        {
            if (i > 0) ImGui::SameLine(ctx);
            ImGui::PushID(ctx, i);
            ImGui::ListBox(ctx, "", &selection[i], items, IM_ARRAYSIZE(items));
            ImGui::PopID(ctx);
            //ImGui::SetItemTooltip("ListBox %d hovered", i);
        }
        ImGui::PopItemWidth(ctx);

        // Dummy
        IMGUI_DEMO_MARKER("Layout/Basic Horizontal Layout/Dummy");
        ImVec2 button_sz(40, 40);
        ImGui::Button(ctx, "A", button_sz); ImGui::SameLine(ctx);
        ImGui::Dummy(ctx, button_sz); ImGui::SameLine(ctx);
        ImGui::Button(ctx, "B", button_sz);

        // Manually wrapping
        // (we should eventually provide this as an automatic layout feature, but for now you can do it manually)
        IMGUI_DEMO_MARKER("Layout/Basic Horizontal Layout/Manual wrapping");
        ImGui::Text(ctx, "Manual wrapping:");
        ImGuiStyle& style = ImGui::GetStyle(ctx);
        int buttons_count = 20;
        float window_visible_x2 = ImGui::GetWindowPos(ctx).x + ImGui::GetWindowContentRegionMax(ctx).x;
        for (int n = 0; n < buttons_count; n++)
        {
            ImGui::PushID(ctx, n);
            ImGui::Button(ctx, "Box", button_sz);
            float last_button_x2 = ImGui::GetItemRectMax(ctx).x;
            float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x; // Expected position if next button was on same line
            if (n + 1 < buttons_count && next_button_x2 < window_visible_x2)
                ImGui::SameLine(ctx);
            ImGui::PopID(ctx);
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Layout/Groups");
    if (ImGui::TreeNode(ctx, "Groups"))
    {
        HelpMarker(ctx, 
            "BeginGroup() basically locks the horizontal position for new line. "
            "EndGroup() bundles the whole group so that you can use \"item\" functions such as "
            "IsItemHovered()/IsItemActive() or SameLine() etc. on the whole group.");
        ImGui::BeginGroup(ctx);
        {
            ImGui::BeginGroup(ctx);
            ImGui::Button(ctx, "AAA");
            ImGui::SameLine(ctx);
            ImGui::Button(ctx, "BBB");
            ImGui::SameLine(ctx);
            ImGui::BeginGroup(ctx);
            ImGui::Button(ctx, "CCC");
            ImGui::Button(ctx, "DDD");
            ImGui::EndGroup(ctx);
            ImGui::SameLine(ctx);
            ImGui::Button(ctx, "EEE");
            ImGui::EndGroup(ctx);
            ImGui::SetItemTooltip(ctx, "First group hovered");
        }
        // Capture the group size and create widgets using the same size
        ImVec2 size = ImGui::GetItemRectSize(ctx);
        const float values[5] = { 0.5f, 0.20f, 0.80f, 0.60f, 0.25f };
        ImGui::PlotHistogram(ctx, "##values", values, IM_ARRAYSIZE(values), 0, NULL, 0.0f, 1.0f, size);

        ImGui::Button(ctx, "ACTION", ImVec2((size.x - ImGui::GetStyle(ctx).ItemSpacing.x) * 0.5f, size.y));
        ImGui::SameLine(ctx);
        ImGui::Button(ctx, "REACTION", ImVec2((size.x - ImGui::GetStyle(ctx).ItemSpacing.x) * 0.5f, size.y));
        ImGui::EndGroup(ctx);
        ImGui::SameLine(ctx);

        ImGui::Button(ctx, "LEVERAGE\nBUZZWORD", size);
        ImGui::SameLine(ctx);

        if (ImGui::BeginListBox(ctx, "List", size))
        {
            ImGui::Selectable(ctx, "Selected", true);
            ImGui::Selectable(ctx, "Not Selected", false);
            ImGui::EndListBox(ctx);
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Layout/Text Baseline Alignment");
    if (ImGui::TreeNode(ctx, "Text Baseline Alignment"))
    {
        {
            ImGui::BulletText(ctx, "Text baseline:");
            ImGui::SameLine(ctx); HelpMarker(ctx, 
                "This is testing the vertical alignment that gets applied on text to keep it aligned with widgets. "
                "Lines only composed of text or \"small\" widgets use less vertical space than lines with framed widgets.");
            ImGui::Indent(ctx);

            ImGui::Text(ctx, "KO Blahblah"); ImGui::SameLine(ctx);
            ImGui::Button(ctx, "Some framed item"); ImGui::SameLine(ctx);
            HelpMarker(ctx, "Baseline of button will look misaligned with text..");

            // If your line starts with text, call AlignTextToFramePadding() to align text to upcoming widgets.
            // (because we don't know what's coming after the Text() statement, we need to move the text baseline
            // down by FramePadding.y ahead of time)
            ImGui::AlignTextToFramePadding(ctx);
            ImGui::Text(ctx, "OK Blahblah"); ImGui::SameLine(ctx);
            ImGui::Button(ctx, "Some framed item"); ImGui::SameLine(ctx);
            HelpMarker(ctx, "We call AlignTextToFramePadding() to vertically align the text baseline by +FramePadding.y");

            // SmallButton() uses the same vertical padding as Text
            ImGui::Button(ctx, "TEST##1"); ImGui::SameLine(ctx);
            ImGui::Text(ctx, "TEST"); ImGui::SameLine(ctx);
            ImGui::SmallButton(ctx, "TEST##2");

            // If your line starts with text, call AlignTextToFramePadding() to align text to upcoming widgets.
            ImGui::AlignTextToFramePadding(ctx);
            ImGui::Text(ctx, "Text aligned to framed item"); ImGui::SameLine(ctx);
            ImGui::Button(ctx, "Item##1"); ImGui::SameLine(ctx);
            ImGui::Text(ctx, "Item"); ImGui::SameLine(ctx);
            ImGui::SmallButton(ctx, "Item##2"); ImGui::SameLine(ctx);
            ImGui::Button(ctx, "Item##3");

            ImGui::Unindent(ctx);
        }

        ImGui::Spacing(ctx);

        {
            ImGui::BulletText(ctx, "Multi-line text:");
            ImGui::Indent(ctx);
            ImGui::Text(ctx, "One\nTwo\nThree"); ImGui::SameLine(ctx);
            ImGui::Text(ctx, "Hello\nWorld"); ImGui::SameLine(ctx);
            ImGui::Text(ctx, "Banana");

            ImGui::Text(ctx, "Banana"); ImGui::SameLine(ctx);
            ImGui::Text(ctx, "Hello\nWorld"); ImGui::SameLine(ctx);
            ImGui::Text(ctx, "One\nTwo\nThree");

            ImGui::Button(ctx, "HOP##1"); ImGui::SameLine(ctx);
            ImGui::Text(ctx, "Banana"); ImGui::SameLine(ctx);
            ImGui::Text(ctx, "Hello\nWorld"); ImGui::SameLine(ctx);
            ImGui::Text(ctx, "Banana");

            ImGui::Button(ctx, "HOP##2"); ImGui::SameLine(ctx);
            ImGui::Text(ctx, "Hello\nWorld"); ImGui::SameLine(ctx);
            ImGui::Text(ctx, "Banana");
            ImGui::Unindent(ctx);
        }

        ImGui::Spacing(ctx);

        {
            ImGui::BulletText(ctx, "Misc items:");
            ImGui::Indent(ctx);

            // SmallButton() sets FramePadding to zero. Text baseline is aligned to match baseline of previous Button.
            ImGui::Button(ctx, "80x80", ImVec2(80, 80));
            ImGui::SameLine(ctx);
            ImGui::Button(ctx, "50x50", ImVec2(50, 50));
            ImGui::SameLine(ctx);
            ImGui::Button(ctx, "Button()");
            ImGui::SameLine(ctx);
            ImGui::SmallButton(ctx, "SmallButton()");

            // Tree
            const float spacing = ImGui::GetStyle(ctx).ItemInnerSpacing.x;
            ImGui::Button(ctx, "Button##1");
            ImGui::SameLine(ctx, 0.0f, spacing);
            if (ImGui::TreeNode(ctx, "Node##1"))
            {
                // Placeholder tree data
                for (int i = 0; i < 6; i++)
                    ImGui::BulletText(ctx, "Item %d..", i);
                ImGui::TreePop(ctx);
            }

            // Vertically align text node a bit lower so it'll be vertically centered with upcoming widget.
            // Otherwise you can use SmallButton() (smaller fit).
            ImGui::AlignTextToFramePadding(ctx);

            // Common mistake to avoid: if we want to SameLine after TreeNode we need to do it before we add
            // other contents below the node.
            bool node_open = ImGui::TreeNode(ctx, "Node##2");
            ImGui::SameLine(ctx, 0.0f, spacing); ImGui::Button(ctx, "Button##2");
            if (node_open)
            {
                // Placeholder tree data
                for (int i = 0; i < 6; i++)
                    ImGui::BulletText(ctx, "Item %d..", i);
                ImGui::TreePop(ctx);
            }

            // Bullet
            ImGui::Button(ctx, "Button##3");
            ImGui::SameLine(ctx, 0.0f, spacing);
            ImGui::BulletText(ctx, "Bullet text");

            ImGui::AlignTextToFramePadding(ctx);
            ImGui::BulletText(ctx, "Node");
            ImGui::SameLine(ctx, 0.0f, spacing); ImGui::Button(ctx, "Button##4");
            ImGui::Unindent(ctx);
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Layout/Scrolling");
    if (ImGui::TreeNode(ctx, "Scrolling"))
    {
        // Vertical scroll functions
        IMGUI_DEMO_MARKER("Layout/Scrolling/Vertical");
        HelpMarker(ctx, "Use SetScrollHereY() or SetScrollFromPosY() to scroll to a given vertical position.");

        static int track_item = 50;
        static bool enable_track = true;
        static bool enable_extra_decorations = false;
        static float scroll_to_off_px = 0.0f;
        static float scroll_to_pos_px = 200.0f;

        ImGui::Checkbox(ctx, "Decoration", &enable_extra_decorations);

        ImGui::Checkbox(ctx, "Track", &enable_track);
        ImGui::PushItemWidth(ctx, 100);
        ImGui::SameLine(ctx, 140); enable_track |= ImGui::DragInt(ctx, "##item", &track_item, 0.25f, 0, 99, "Item = %d");

        bool scroll_to_off = ImGui::Button(ctx, "Scroll Offset");
        ImGui::SameLine(ctx, 140); scroll_to_off |= ImGui::DragFloat(ctx, "##off", &scroll_to_off_px, 1.00f, 0, FLT_MAX, "+%.0f px");

        bool scroll_to_pos = ImGui::Button(ctx, "Scroll To Pos");
        ImGui::SameLine(ctx, 140); scroll_to_pos |= ImGui::DragFloat(ctx, "##pos", &scroll_to_pos_px, 1.00f, -10, FLT_MAX, "X/Y = %.0f px");
        ImGui::PopItemWidth(ctx);

        if (scroll_to_off || scroll_to_pos)
            enable_track = false;

        ImGuiStyle& style = ImGui::GetStyle(ctx);
        float child_w = (ImGui::GetContentRegionAvail(ctx).x - 4 * style.ItemSpacing.x) / 5;
        if (child_w < 1.0f)
            child_w = 1.0f;
        ImGui::PushID(ctx, "##VerticalScrolling");
        for (int i = 0; i < 5; i++)
        {
            if (i > 0) ImGui::SameLine(ctx);
            ImGui::BeginGroup(ctx);
            const char* names[] = { "Top", "25%", "Center", "75%", "Bottom" };
            ImGui::TextUnformatted(ctx, names[i]);

            const ImGuiWindowFlags child_flags = enable_extra_decorations ? ImGuiWindowFlags_MenuBar : 0;
            const ImGuiID child_id = ImGui::GetID(ctx, (void*)(intptr_t)i);
            const bool child_is_visible = ImGui::BeginChild(ctx, child_id, ImVec2(child_w, 200.0f), ImGuiChildFlags_Border, child_flags);
            if (ImGui::BeginMenuBar(ctx))
            {
                ImGui::TextUnformatted(ctx, "abc");
                ImGui::EndMenuBar(ctx);
            }
            if (scroll_to_off)
                ImGui::SetScrollY(ctx, scroll_to_off_px);
            if (scroll_to_pos)
                ImGui::SetScrollFromPosY(ctx, ImGui::GetCursorStartPos(ctx).y + scroll_to_pos_px, i * 0.25f);
            if (child_is_visible) // Avoid calling SetScrollHereY when running with culled items
            {
                for (int item = 0; item < 100; item++)
                {
                    if (enable_track && item == track_item)
                    {
                        ImGui::TextColored(ctx, ImVec4(1, 1, 0, 1), "Item %d", item);
                        ImGui::SetScrollHereY(ctx, i * 0.25f); // 0.0f:top, 0.5f:center, 1.0f:bottom
                    }
                    else
                    {
                        ImGui::Text(ctx, "Item %d", item);
                    }
                }
            }
            float scroll_y = ImGui::GetScrollY(ctx);
            float scroll_max_y = ImGui::GetScrollMaxY(ctx);
            ImGui::EndChild(ctx);
            ImGui::Text(ctx, "%.0f/%.0f", scroll_y, scroll_max_y);
            ImGui::EndGroup(ctx);
        }
        ImGui::PopID(ctx);

        // Horizontal scroll functions
        IMGUI_DEMO_MARKER("Layout/Scrolling/Horizontal");
        ImGui::Spacing(ctx);
        HelpMarker(ctx, 
            "Use SetScrollHereX() or SetScrollFromPosX() to scroll to a given horizontal position.\n\n"
            "Because the clipping rectangle of most window hides half worth of WindowPadding on the "
            "left/right, using SetScrollFromPosX(+1) will usually result in clipped text whereas the "
            "equivalent SetScrollFromPosY(+1) wouldn't.");
        ImGui::PushID(ctx, "##HorizontalScrolling");
        for (int i = 0; i < 5; i++)
        {
            float child_height = ImGui::GetTextLineHeight(ctx) + style.ScrollbarSize + style.WindowPadding.y * 2.0f;
            ImGuiWindowFlags child_flags = ImGuiWindowFlags_HorizontalScrollbar | (enable_extra_decorations ? ImGuiWindowFlags_AlwaysVerticalScrollbar : 0);
            ImGuiID child_id = ImGui::GetID(ctx, (void*)(intptr_t)i);
            bool child_is_visible = ImGui::BeginChild(ctx, child_id, ImVec2(-100, child_height), ImGuiChildFlags_Border, child_flags);
            if (scroll_to_off)
                ImGui::SetScrollX(ctx, scroll_to_off_px);
            if (scroll_to_pos)
                ImGui::SetScrollFromPosX(ctx, ImGui::GetCursorStartPos(ctx).x + scroll_to_pos_px, i * 0.25f);
            if (child_is_visible) // Avoid calling SetScrollHereY when running with culled items
            {
                for (int item = 0; item < 100; item++)
                {
                    if (item > 0)
                        ImGui::SameLine(ctx);
                    if (enable_track && item == track_item)
                    {
                        ImGui::TextColored(ctx, ImVec4(1, 1, 0, 1), "Item %d", item);
                        ImGui::SetScrollHereX(ctx, i * 0.25f); // 0.0f:left, 0.5f:center, 1.0f:right
                    }
                    else
                    {
                        ImGui::Text(ctx, "Item %d", item);
                    }
                }
            }
            float scroll_x = ImGui::GetScrollX(ctx);
            float scroll_max_x = ImGui::GetScrollMaxX(ctx);
            ImGui::EndChild(ctx);
            ImGui::SameLine(ctx);
            const char* names[] = { "Left", "25%", "Center", "75%", "Right" };
            ImGui::Text(ctx, "%s\n%.0f/%.0f", names[i], scroll_x, scroll_max_x);
            ImGui::Spacing(ctx);
        }
        ImGui::PopID(ctx);

        // Miscellaneous Horizontal Scrolling Demo
        IMGUI_DEMO_MARKER("Layout/Scrolling/Horizontal (more)");
        HelpMarker(ctx, 
            "Horizontal scrolling for a window is enabled via the ImGuiWindowFlags_HorizontalScrollbar flag.\n\n"
            "You may want to also explicitly specify content width by using SetNextWindowContentWidth() before Begin().");
        static int lines = 7;
        ImGui::SliderInt(ctx, "Lines", &lines, 1, 15);
        ImGui::PushStyleVar(ctx, ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleVar(ctx, ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
        ImVec2 scrolling_child_size = ImVec2(0, ImGui::GetFrameHeightWithSpacing(ctx) * 7 + 30);
        ImGui::BeginChild(ctx, "scrolling", scrolling_child_size, ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar);
        for (int line = 0; line < lines; line++)
        {
            // Display random stuff. For the sake of this trivial demo we are using basic Button() + SameLine()
            // If you want to create your own time line for a real application you may be better off manipulating
            // the cursor position yourself, aka using SetCursorPos/SetCursorScreenPos to position the widgets
            // yourself. You may also want to use the lower-level ImDrawList API.
            int num_buttons = 10 + ((line & 1) ? line * 9 : line * 3);
            for (int n = 0; n < num_buttons; n++)
            {
                if (n > 0) ImGui::SameLine(ctx);
                ImGui::PushID(ctx, n + line * 1000);
                char num_buf[16];
                sprintf(num_buf, "%d", n);
                const char* label = (!(n % 15)) ? "FizzBuzz" : (!(n % 3)) ? "Fizz" : (!(n % 5)) ? "Buzz" : num_buf;
                float hue = n * 0.05f;
                ImGui::PushStyleColor(ctx, ImGuiCol_Button, (ImVec4)ImColor::HSV(hue, 0.6f, 0.6f));
                ImGui::PushStyleColor(ctx, ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue, 0.7f, 0.7f));
                ImGui::PushStyleColor(ctx, ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue, 0.8f, 0.8f));
                ImGui::Button(ctx, label, ImVec2(40.0f + sinf((float)(line + n)) * 20.0f, 0.0f));
                ImGui::PopStyleColor(ctx, 3);
                ImGui::PopID(ctx);
            }
        }
        float scroll_x = ImGui::GetScrollX(ctx);
        float scroll_max_x = ImGui::GetScrollMaxX(ctx);
        ImGui::EndChild(ctx);
        ImGui::PopStyleVar(ctx, 2);
        float scroll_x_delta = 0.0f;
        ImGui::SmallButton(ctx, "<<");
        if (ImGui::IsItemActive(ctx))
            scroll_x_delta = -ImGui::GetIO(ctx).DeltaTime * 1000.0f;
        ImGui::SameLine(ctx);
        ImGui::Text(ctx, "Scroll from code"); ImGui::SameLine(ctx);
        ImGui::SmallButton(ctx, ">>");
        if (ImGui::IsItemActive(ctx))
            scroll_x_delta = +ImGui::GetIO(ctx).DeltaTime * 1000.0f;
        ImGui::SameLine(ctx);
        ImGui::Text(ctx, "%.0f/%.0f", scroll_x, scroll_max_x);
        if (scroll_x_delta != 0.0f)
        {
            // Demonstrate a trick: you can use Begin to set yourself in the context of another window
            // (here we are already out of your child window)
            ImGui::BeginChild(ctx, "scrolling");
            ImGui::SetScrollX(ctx, ImGui::GetScrollX(ctx) + scroll_x_delta);
            ImGui::EndChild(ctx);
        }
        ImGui::Spacing(ctx);

        static bool show_horizontal_contents_size_demo_window = false;
        ImGui::Checkbox(ctx, "Show Horizontal contents size demo window", &show_horizontal_contents_size_demo_window);

        if (show_horizontal_contents_size_demo_window)
        {
            static bool show_h_scrollbar = true;
            static bool show_button = true;
            static bool show_tree_nodes = true;
            static bool show_text_wrapped = false;
            static bool show_columns = true;
            static bool show_tab_bar = true;
            static bool show_child = false;
            static bool explicit_content_size = false;
            static float contents_size_x = 300.0f;
            if (explicit_content_size)
                ImGui::SetNextWindowContentSize(ctx, ImVec2(contents_size_x, 0.0f));
            ImGui::Begin(ctx, "Horizontal contents size demo window", &show_horizontal_contents_size_demo_window, show_h_scrollbar ? ImGuiWindowFlags_HorizontalScrollbar : 0);
            IMGUI_DEMO_MARKER("Layout/Scrolling/Horizontal contents size demo window");
            ImGui::PushStyleVar(ctx, ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
            ImGui::PushStyleVar(ctx, ImGuiStyleVar_FramePadding, ImVec2(2, 0));
            HelpMarker(ctx, "Test of different widgets react and impact the work rectangle growing when horizontal scrolling is enabled.\n\nUse 'Metrics->Tools->Show windows rectangles' to visualize rectangles.");
            ImGui::Checkbox(ctx, "H-scrollbar", &show_h_scrollbar);
            ImGui::Checkbox(ctx, "Button", &show_button);            // Will grow contents size (unless explicitly overwritten)
            ImGui::Checkbox(ctx, "Tree nodes", &show_tree_nodes);    // Will grow contents size and display highlight over full width
            ImGui::Checkbox(ctx, "Text wrapped", &show_text_wrapped);// Will grow and use contents size
            ImGui::Checkbox(ctx, "Columns", &show_columns);          // Will use contents size
            ImGui::Checkbox(ctx, "Tab bar", &show_tab_bar);          // Will use contents size
            ImGui::Checkbox(ctx, "Child", &show_child);              // Will grow and use contents size
            ImGui::Checkbox(ctx, "Explicit content size", &explicit_content_size);
            ImGui::Text(ctx, "Scroll %.1f/%.1f %.1f/%.1f", ImGui::GetScrollX(ctx), ImGui::GetScrollMaxX(ctx), ImGui::GetScrollY(ctx), ImGui::GetScrollMaxY(ctx));
            if (explicit_content_size)
            {
                ImGui::SameLine(ctx);
                ImGui::SetNextItemWidth(ctx, 100);
                ImGui::DragFloat(ctx, "##csx", &contents_size_x);
                ImVec2 p = ImGui::GetCursorScreenPos(ctx);
                ImGui::GetWindowDrawList(ctx)->AddRectFilled(p, ImVec2(p.x + 10, p.y + 10), IM_COL32_WHITE);
                ImGui::GetWindowDrawList(ctx)->AddRectFilled(ImVec2(p.x + contents_size_x - 10, p.y), ImVec2(p.x + contents_size_x, p.y + 10), IM_COL32_WHITE);
                ImGui::Dummy(ctx, ImVec2(0, 10));
            }
            ImGui::PopStyleVar(ctx, 2);
            ImGui::Separator(ctx);
            if (show_button)
            {
                ImGui::Button(ctx, "this is a 300-wide button", ImVec2(300, 0));
            }
            if (show_tree_nodes)
            {
                bool open = true;
                if (ImGui::TreeNode(ctx, "this is a tree node"))
                {
                    if (ImGui::TreeNode(ctx, "another one of those tree node..."))
                    {
                        ImGui::Text(ctx, "Some tree contents");
                        ImGui::TreePop(ctx);
                    }
                    ImGui::TreePop(ctx);
                }
                ImGui::CollapsingHeader(ctx, "CollapsingHeader", &open);
            }
            if (show_text_wrapped)
            {
                ImGui::TextWrapped(ctx, "This text should automatically wrap on the edge of the work rectangle.");
            }
            if (show_columns)
            {
                ImGui::Text(ctx, "Tables:");
                if (ImGui::BeginTable(ctx, "table", 4, ImGuiTableFlags_Borders))
                {
                    for (int n = 0; n < 4; n++)
                    {
                        ImGui::TableNextColumn(ctx);
                        ImGui::Text(ctx, "Width %.2f", ImGui::GetContentRegionAvail(ctx).x);
                    }
                    ImGui::EndTable(ctx);
                }
                ImGui::Text(ctx, "Columns:");
                ImGui::Columns(ctx, 4);
                for (int n = 0; n < 4; n++)
                {
                    ImGui::Text(ctx, "Width %.2f", ImGui::GetColumnWidth(ctx));
                    ImGui::NextColumn(ctx);
                }
                ImGui::Columns(ctx, 1);
            }
            if (show_tab_bar && ImGui::BeginTabBar(ctx, "Hello"))
            {
                if (ImGui::BeginTabItem(ctx, "OneOneOne")) { ImGui::EndTabItem(ctx); }
                if (ImGui::BeginTabItem(ctx, "TwoTwoTwo")) { ImGui::EndTabItem(ctx); }
                if (ImGui::BeginTabItem(ctx, "ThreeThreeThree")) { ImGui::EndTabItem(ctx); }
                if (ImGui::BeginTabItem(ctx, "FourFourFour")) { ImGui::EndTabItem(ctx); }
                ImGui::EndTabBar(ctx);
            }
            if (show_child)
            {
                ImGui::BeginChild(ctx, "child", ImVec2(0, 0), ImGuiChildFlags_Border);
                ImGui::EndChild(ctx);
            }
            ImGui::End(ctx);
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Layout/Clipping");
    if (ImGui::TreeNode(ctx, "Clipping"))
    {
        static ImVec2 size(100.0f, 100.0f);
        static ImVec2 offset(30.0f, 30.0f);
        ImGui::DragFloat2(ctx, "size", (float*)&size, 0.5f, 1.0f, 200.0f, "%.0f");
        ImGui::TextWrapped(ctx, "(Click and drag to scroll)");

        HelpMarker(ctx, 
            "(Left) Using ImGui::PushClipRect():\n"
            "Will alter ImGui hit-testing logic + ImDrawList rendering.\n"
            "(use this if you want your clipping rectangle to affect interactions)\n\n"
            "(Center) Using ImDrawList::PushClipRect():\n"
            "Will alter ImDrawList rendering only.\n"
            "(use this as a shortcut if you are only using ImDrawList calls)\n\n"
            "(Right) Using ImDrawList::AddText() with a fine ClipRect:\n"
            "Will alter only this specific ImDrawList::AddText() rendering.\n"
            "This is often used internally to avoid altering the clipping rectangle and minimize draw calls.");

        for (int n = 0; n < 3; n++)
        {
            if (n > 0)
                ImGui::SameLine(ctx);

            ImGui::PushID(ctx, n);
            ImGui::InvisibleButton(ctx, "##canvas", size);
            if (ImGui::IsItemActive(ctx) && ImGui::IsMouseDragging(ctx, ImGuiMouseButton_Left))
            {
                offset.x += ImGui::GetIO(ctx).MouseDelta.x;
                offset.y += ImGui::GetIO(ctx).MouseDelta.y;
            }
            ImGui::PopID(ctx);
            if (!ImGui::IsItemVisible(ctx)) // Skip rendering as ImDrawList elements are not clipped.
                continue;

            const ImVec2 p0 = ImGui::GetItemRectMin(ctx);
            const ImVec2 p1 = ImGui::GetItemRectMax(ctx);
            const char* text_str = "Line 1 hello\nLine 2 clip me!";
            const ImVec2 text_pos = ImVec2(p0.x + offset.x, p0.y + offset.y);
            ImDrawList* draw_list = ImGui::GetWindowDrawList(ctx);
            switch (n)
            {
            case 0:
                ImGui::PushClipRect(ctx, p0, p1, true);
                draw_list->AddRectFilled(p0, p1, IM_COL32(90, 90, 120, 255));
                draw_list->AddText(text_pos, IM_COL32_WHITE, text_str);
                ImGui::PopClipRect(ctx);
                break;
            case 1:
                draw_list->PushClipRect(p0, p1, true);
                draw_list->AddRectFilled(p0, p1, IM_COL32(90, 90, 120, 255));
                draw_list->AddText(text_pos, IM_COL32_WHITE, text_str);
                draw_list->PopClipRect();
                break;
            case 2:
                ImVec4 clip_rect(p0.x, p0.y, p1.x, p1.y); // AddText() takes a ImVec4* here so let's convert.
                draw_list->AddRectFilled(p0, p1, IM_COL32(90, 90, 120, 255));
                draw_list->AddText(ImGui::GetFont(ctx), ImGui::GetFontSize(ctx), text_pos, IM_COL32_WHITE, text_str, NULL, 0.0f, &clip_rect);
                break;
            }
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Layout/Overlap Mode");
    if (ImGui::TreeNode(ctx, "Overlap Mode"))
    {
        static bool enable_allow_overlap = true;

        HelpMarker(ctx, 
            "Hit-testing is by default performed in item submission order, which generally is perceived as 'back-to-front'.\n\n"
            "By using SetNextItemAllowOverlap() you can notify that an item may be overlapped by another. Doing so alters the hovering logic: items using AllowOverlap mode requires an extra frame to accept hovered state.");
        ImGui::Checkbox(ctx, "Enable AllowOverlap", &enable_allow_overlap);

        ImVec2 button1_pos = ImGui::GetCursorScreenPos(ctx);
        ImVec2 button2_pos = ImVec2(button1_pos.x + 50.0f, button1_pos.y + 50.0f);
        if (enable_allow_overlap)
            ImGui::SetNextItemAllowOverlap(ctx);
        ImGui::Button(ctx, "Button 1", ImVec2(80, 80));
        ImGui::SetCursorScreenPos(ctx, button2_pos);
        ImGui::Button(ctx, "Button 2", ImVec2(80, 80));

        // This is typically used with width-spanning items.
        // (note that Selectable() has a dedicated flag ImGuiSelectableFlags_AllowOverlap, which is a shortcut
        // for using SetNextItemAllowOverlap(). For demo purpose we use SetNextItemAllowOverlap() here.)
        if (enable_allow_overlap)
            ImGui::SetNextItemAllowOverlap(ctx);
        ImGui::Selectable(ctx, "Some Selectable", false);
        ImGui::SameLine(ctx);
        ImGui::SmallButton(ctx, "++");

        ImGui::TreePop(ctx);
    }
}

static void ShowDemoWindowPopups(ImGuiContext* ctx)
{
    IMGUI_DEMO_MARKER("Popups");
    if (!ImGui::CollapsingHeader(ctx, "Popups & Modal windows"))
        return;

    // The properties of popups windows are:
    // - They block normal mouse hovering detection outside them. (*)
    // - Unless modal, they can be closed by clicking anywhere outside them, or by pressing ESCAPE.
    // - Their visibility state (~bool) is held internally by Dear ImGui instead of being held by the programmer as
    //   we are used to with regular Begin() calls. User can manipulate the visibility state by calling OpenPopup().
    // (*) One can use IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) to bypass it and detect hovering even
    //     when normally blocked by a popup.
    // Those three properties are connected. The library needs to hold their visibility state BECAUSE it can close
    // popups at any time.

    // Typical use for regular windows:
    //   bool my_tool_is_active = false; if (ImGui::Button("Open")) my_tool_is_active = true; [...] if (my_tool_is_active) Begin("My Tool", &my_tool_is_active) { [...] } End();
    // Typical use for popups:
    //   if (ImGui::Button("Open")) ImGui::OpenPopup("MyPopup"); if (ImGui::BeginPopup("MyPopup") { [...] EndPopup(); }

    // With popups we have to go through a library call (here OpenPopup) to manipulate the visibility state.
    // This may be a bit confusing at first but it should quickly make sense. Follow on the examples below.

    IMGUI_DEMO_MARKER("Popups/Popups");
    if (ImGui::TreeNode(ctx, "Popups"))
    {
        ImGui::TextWrapped(ctx, 
            "When a popup is active, it inhibits interacting with windows that are behind the popup. "
            "Clicking outside the popup closes it.");

        static int selected_fish = -1;
        const char* names[] = { "Bream", "Haddock", "Mackerel", "Pollock", "Tilefish" };
        static bool toggles[] = { true, false, false, false, false };

        // Simple selection popup (if you want to show the current selection inside the Button itself,
        // you may want to build a string using the "###" operator to preserve a constant ID with a variable label)
        if (ImGui::Button(ctx, "Select.."))
            ImGui::OpenPopup(ctx, "my_select_popup");
        ImGui::SameLine(ctx);
        ImGui::TextUnformatted(ctx, selected_fish == -1 ? "<None>" : names[selected_fish]);
        if (ImGui::BeginPopup(ctx, "my_select_popup"))
        {
            ImGui::SeparatorText(ctx, "Aquarium");
            for (int i = 0; i < IM_ARRAYSIZE(names); i++)
                if (ImGui::Selectable(ctx, names[i]))
                    selected_fish = i;
            ImGui::EndPopup(ctx);
        }

        // Showing a menu with toggles
        if (ImGui::Button(ctx, "Toggle.."))
            ImGui::OpenPopup(ctx, "my_toggle_popup");
        if (ImGui::BeginPopup(ctx, "my_toggle_popup"))
        {
            for (int i = 0; i < IM_ARRAYSIZE(names); i++)
                ImGui::MenuItem(ctx, names[i], "", &toggles[i]);
            if (ImGui::BeginMenu(ctx, "Sub-menu"))
            {
                ImGui::MenuItem(ctx, "Click me");
                ImGui::EndMenu(ctx);
            }

            ImGui::Separator(ctx);
            ImGui::Text(ctx, "Tooltip here");
            ImGui::SetItemTooltip(ctx, "I am a tooltip over a popup");

            if (ImGui::Button(ctx, "Stacked Popup"))
                ImGui::OpenPopup(ctx, "another popup");
            if (ImGui::BeginPopup(ctx, "another popup"))
            {
                for (int i = 0; i < IM_ARRAYSIZE(names); i++)
                    ImGui::MenuItem(ctx, names[i], "", &toggles[i]);
                if (ImGui::BeginMenu(ctx, "Sub-menu"))
                {
                    ImGui::MenuItem(ctx, "Click me");
                    if (ImGui::Button(ctx, "Stacked Popup"))
                        ImGui::OpenPopup(ctx, "another popup");
                    if (ImGui::BeginPopup(ctx, "another popup"))
                    {
                        ImGui::Text(ctx, "I am the last one here.");
                        ImGui::EndPopup(ctx);
                    }
                    ImGui::EndMenu(ctx);
                }
                ImGui::EndPopup(ctx);
            }
            ImGui::EndPopup(ctx);
        }

        // Call the more complete ShowExampleMenuFile which we use in various places of this demo
        if (ImGui::Button(ctx, "With a menu.."))
            ImGui::OpenPopup(ctx, "my_file_popup");
        if (ImGui::BeginPopup(ctx, "my_file_popup", ImGuiWindowFlags_MenuBar))
        {
            if (ImGui::BeginMenuBar(ctx))
            {
                if (ImGui::BeginMenu(ctx, "File"))
                {
                    ShowExampleMenuFile(ctx);
                    ImGui::EndMenu(ctx);
                }
                if (ImGui::BeginMenu(ctx, "Edit"))
                {
                    ImGui::MenuItem(ctx, "Dummy");
                    ImGui::EndMenu(ctx);
                }
                ImGui::EndMenuBar(ctx);
            }
            ImGui::Text(ctx, "Hello from popup!");
            ImGui::Button(ctx, "This is a dummy button..");
            ImGui::EndPopup(ctx);
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Popups/Context menus");
    if (ImGui::TreeNode(ctx, "Context menus"))
    {
        HelpMarker(ctx, "\"Context\" functions are simple helpers to associate a Popup to a given Item or Window identifier.");

        // BeginPopupContextItem() is a helper to provide common/simple popup behavior of essentially doing:
        //     if (id == 0)
        //         id = GetItemID(); // Use last item id
        //     if (IsItemHovered() && IsMouseReleased(ImGuiMouseButton_Right))
        //         OpenPopup(id);
        //     return BeginPopup(id);
        // For advanced uses you may want to replicate and customize this code.
        // See more details in BeginPopupContextItem().

        // Example 1
        // When used after an item that has an ID (e.g. Button), we can skip providing an ID to BeginPopupContextItem(),
        // and BeginPopupContextItem() will use the last item ID as the popup ID.
        {
            const char* names[5] = { "Label1", "Label2", "Label3", "Label4", "Label5" };
            static int selected = -1;
            for (int n = 0; n < 5; n++)
            {
                if (ImGui::Selectable(ctx, names[n], selected == n))
                    selected = n;
                if (ImGui::BeginPopupContextItem(ctx)) // <-- use last item id as popup id
                {
                    selected = n;
                    ImGui::Text(ctx, "This a popup for \"%s\"!", names[n]);
                    if (ImGui::Button(ctx, "Close"))
                        ImGui::CloseCurrentPopup(ctx);
                    ImGui::EndPopup(ctx);
                }
                ImGui::SetItemTooltip(ctx, "Right-click to open popup");
            }
        }

        // Example 2
        // Popup on a Text() element which doesn't have an identifier: we need to provide an identifier to BeginPopupContextItem().
        // Using an explicit identifier is also convenient if you want to activate the popups from different locations.
        {
            HelpMarker(ctx, "Text() elements don't have stable identifiers so we need to provide one.");
            static float value = 0.5f;
            ImGui::Text(ctx, "Value = %.3f <-- (1) right-click this text", value);
            if (ImGui::BeginPopupContextItem(ctx, "my popup"))
            {
                if (ImGui::Selectable(ctx, "Set to zero")) value = 0.0f;
                if (ImGui::Selectable(ctx, "Set to PI")) value = 3.1415f;
                ImGui::SetNextItemWidth(ctx, -FLT_MIN);
                ImGui::DragFloat(ctx, "##Value", &value, 0.1f, 0.0f, 0.0f);
                ImGui::EndPopup(ctx);
            }

            // We can also use OpenPopupOnItemClick() to toggle the visibility of a given popup.
            // Here we make it that right-clicking this other text element opens the same popup as above.
            // The popup itself will be submitted by the code above.
            ImGui::Text(ctx, "(2) Or right-click this text");
            ImGui::OpenPopupOnItemClick(ctx, "my popup", ImGuiPopupFlags_MouseButtonRight);

            // Back to square one: manually open the same popup.
            if (ImGui::Button(ctx, "(3) Or click this button"))
                ImGui::OpenPopup(ctx, "my popup");
        }

        // Example 3
        // When using BeginPopupContextItem() with an implicit identifier (NULL == use last item ID),
        // we need to make sure your item identifier is stable.
        // In this example we showcase altering the item label while preserving its identifier, using the ### operator (see FAQ).
        {
            HelpMarker(ctx, "Showcase using a popup ID linked to item ID, with the item having a changing label + stable ID using the ### operator.");
            static char name[32] = "Label1";
            char buf[64];
            sprintf(buf, "Button: %s###Button", name); // ### operator override ID ignoring the preceding label
            ImGui::Button(ctx, buf);
            if (ImGui::BeginPopupContextItem(ctx))
            {
                ImGui::Text(ctx, "Edit name:");
                ImGui::InputText(ctx, "##edit", name, IM_ARRAYSIZE(name));
                if (ImGui::Button(ctx, "Close"))
                    ImGui::CloseCurrentPopup(ctx);
                ImGui::EndPopup(ctx);
            }
            ImGui::SameLine(ctx); ImGui::Text(ctx, "(<-- right-click here)");
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Popups/Modals");
    if (ImGui::TreeNode(ctx, "Modals"))
    {
        ImGui::TextWrapped(ctx, "Modal windows are like popups but the user cannot close them by clicking outside.");

        if (ImGui::Button(ctx, "Delete.."))
            ImGui::OpenPopup(ctx, "Delete?");

        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport(ctx)->GetCenter();
        ImGui::SetNextWindowPos(ctx, center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal(ctx, "Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text(ctx, "All those beautiful files will be deleted.\nThis operation cannot be undone!");
            ImGui::Separator(ctx);

            //static int unused_i = 0;
            //ImGui::Combo("Combo", &unused_i, "Delete\0Delete harder\0");

            static bool dont_ask_me_next_time = false;
            ImGui::PushStyleVar(ctx, ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::Checkbox(ctx, "Don't ask me next time", &dont_ask_me_next_time);
            ImGui::PopStyleVar(ctx);

            if (ImGui::Button(ctx, "OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(ctx); }
            ImGui::SetItemDefaultFocus(ctx);
            ImGui::SameLine(ctx);
            if (ImGui::Button(ctx, "Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(ctx); }
            ImGui::EndPopup(ctx);
        }

        if (ImGui::Button(ctx, "Stacked modals.."))
            ImGui::OpenPopup(ctx, "Stacked 1");
        if (ImGui::BeginPopupModal(ctx, "Stacked 1", NULL, ImGuiWindowFlags_MenuBar))
        {
            if (ImGui::BeginMenuBar(ctx))
            {
                if (ImGui::BeginMenu(ctx, "File"))
                {
                    if (ImGui::MenuItem(ctx, "Some menu item")) {}
                    ImGui::EndMenu(ctx);
                }
                ImGui::EndMenuBar(ctx);
            }
            ImGui::Text(ctx, "Hello from Stacked The First\nUsing style.Colors[ImGuiCol_ModalWindowDimBg] behind it.");

            // Testing behavior of widgets stacking their own regular popups over the modal.
            static int item = 1;
            static float color[4] = { 0.4f, 0.7f, 0.0f, 0.5f };
            ImGui::Combo(ctx, "Combo", &item, "aaaa\0bbbb\0cccc\0dddd\0eeee\0\0");
            ImGui::ColorEdit4(ctx, "color", color);

            if (ImGui::Button(ctx, "Add another modal.."))
                ImGui::OpenPopup(ctx, "Stacked 2");

            // Also demonstrate passing a bool* to BeginPopupModal(), this will create a regular close button which
            // will close the popup. Note that the visibility state of popups is owned by imgui, so the input value
            // of the bool actually doesn't matter here.
            bool unused_open = true;
            if (ImGui::BeginPopupModal(ctx, "Stacked 2", &unused_open))
            {
                ImGui::Text(ctx, "Hello from Stacked The Second!");
                if (ImGui::Button(ctx, "Close"))
                    ImGui::CloseCurrentPopup(ctx);
                ImGui::EndPopup(ctx);
            }

            if (ImGui::Button(ctx, "Close"))
                ImGui::CloseCurrentPopup(ctx);
            ImGui::EndPopup(ctx);
        }

        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Popups/Menus inside a regular window");
    if (ImGui::TreeNode(ctx, "Menus inside a regular window"))
    {
        ImGui::TextWrapped(ctx, "Below we are testing adding menu items to a regular window. It's rather unusual but should work!");
        ImGui::Separator(ctx);

        ImGui::MenuItem(ctx, "Menu item", "CTRL+M");
        if (ImGui::BeginMenu(ctx, "Menu inside a regular window"))
        {
            ShowExampleMenuFile(ctx);
            ImGui::EndMenu(ctx);
        }
        ImGui::Separator(ctx);
        ImGui::TreePop(ctx);
    }
}

// Dummy data structure that we use for the Table demo.
// (pre-C++11 doesn't allow us to instantiate ImVector<MyItem> template if this structure is defined inside the demo function)
namespace
{
// We are passing our own identifier to TableSetupColumn() to facilitate identifying columns in the sorting code.
// This identifier will be passed down into ImGuiTableSortSpec::ColumnUserID.
// But it is possible to omit the user id parameter of TableSetupColumn() and just use the column index instead! (ImGuiTableSortSpec::ColumnIndex)
// If you don't use sorting, you will generally never care about giving column an ID!
enum MyItemColumnID
{
    MyItemColumnID_ID,
    MyItemColumnID_Name,
    MyItemColumnID_Action,
    MyItemColumnID_Quantity,
    MyItemColumnID_Description
};

struct MyItem
{
    int         ID;
    const char* Name;
    int         Quantity;

    // We have a problem which is affecting _only this demo_ and should not affect your code:
    // As we don't rely on std:: or other third-party library to compile dear imgui, we only have reliable access to qsort(),
    // however qsort doesn't allow passing user data to comparing function.
    // As a workaround, we are storing the sort specs in a static/global for the comparing function to access.
    // In your own use case you would probably pass the sort specs to your sorting/comparing functions directly and not use a global.
    // We could technically call ImGui::TableGetSortSpecs() in CompareWithSortSpecs(), but considering that this function is called
    // very often by the sorting algorithm it would be a little wasteful.
    static const ImGuiTableSortSpecs* s_current_sort_specs;

    static void SortWithSortSpecs(ImGuiTableSortSpecs* sort_specs, MyItem* items, int items_count)
    {
        s_current_sort_specs = sort_specs; // Store in variable accessible by the sort function.
        if (items_count > 1)
            qsort(items, (size_t)items_count, sizeof(items[0]), MyItem::CompareWithSortSpecs);
        s_current_sort_specs = NULL;
    }

    // Compare function to be used by qsort()
    static int IMGUI_CDECL CompareWithSortSpecs(const void* lhs, const void* rhs)
    {
        const MyItem* a = (const MyItem*)lhs;
        const MyItem* b = (const MyItem*)rhs;
        for (int n = 0; n < s_current_sort_specs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const ImGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[n];
            int delta = 0;
            switch (sort_spec->ColumnUserID)
            {
            case MyItemColumnID_ID:             delta = (a->ID - b->ID);                break;
            case MyItemColumnID_Name:           delta = (strcmp(a->Name, b->Name));     break;
            case MyItemColumnID_Quantity:       delta = (a->Quantity - b->Quantity);    break;
            case MyItemColumnID_Description:    delta = (strcmp(a->Name, b->Name));     break;
            default: IM_ASSERT(0); break;
            }
            if (delta > 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? +1 : -1;
            if (delta < 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? -1 : +1;
        }

        // qsort() is instable so always return a way to differenciate items.
        // Your own compare function may want to avoid fallback on implicit sort specs e.g. a Name compare if it wasn't already part of the sort specs.
        return (a->ID - b->ID);
    }
};
const ImGuiTableSortSpecs* MyItem::s_current_sort_specs = NULL;
}

// Make the UI compact because there are so many fields
static void PushStyleCompact(ImGuiContext* ctx)
{
    ImGuiStyle& style = ImGui::GetStyle(ctx);
    ImGui::PushStyleVar(ctx, ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, (float)(int)(style.FramePadding.y * 0.60f)));
    ImGui::PushStyleVar(ctx, ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, (float)(int)(style.ItemSpacing.y * 0.60f)));
}

static void PopStyleCompact(ImGuiContext* ctx)
{
    ImGui::PopStyleVar(ctx, 2);
}

// Show a combo box with a choice of sizing policies
static void EditTableSizingFlags(ImGuiContext* ctx, ImGuiTableFlags* p_flags)
{
    struct EnumDesc { ImGuiTableFlags Value; const char* Name; const char* Tooltip; };
    static const EnumDesc policies[] =
    {
        { ImGuiTableFlags_None,               "Default",                            "Use default sizing policy:\n- ImGuiTableFlags_SizingFixedFit if ScrollX is on or if host window has ImGuiWindowFlags_AlwaysAutoResize.\n- ImGuiTableFlags_SizingStretchSame otherwise." },
        { ImGuiTableFlags_SizingFixedFit,     "ImGuiTableFlags_SizingFixedFit",     "Columns default to _WidthFixed (if resizable) or _WidthAuto (if not resizable), matching contents width." },
        { ImGuiTableFlags_SizingFixedSame,    "ImGuiTableFlags_SizingFixedSame",    "Columns are all the same width, matching the maximum contents width.\nImplicitly disable ImGuiTableFlags_Resizable and enable ImGuiTableFlags_NoKeepColumnsVisible." },
        { ImGuiTableFlags_SizingStretchProp,  "ImGuiTableFlags_SizingStretchProp",  "Columns default to _WidthStretch with weights proportional to their widths." },
        { ImGuiTableFlags_SizingStretchSame,  "ImGuiTableFlags_SizingStretchSame",  "Columns default to _WidthStretch with same weights." }
    };
    int idx;
    for (idx = 0; idx < IM_ARRAYSIZE(policies); idx++)
        if (policies[idx].Value == (*p_flags & ImGuiTableFlags_SizingMask_))
            break;
    const char* preview_text = (idx < IM_ARRAYSIZE(policies)) ? policies[idx].Name + (idx > 0 ? strlen("ImGuiTableFlags") : 0) : "";
    if (ImGui::BeginCombo(ctx, "Sizing Policy", preview_text))
    {
        for (int n = 0; n < IM_ARRAYSIZE(policies); n++)
            if (ImGui::Selectable(ctx, policies[n].Name, idx == n))
                *p_flags = (*p_flags & ~ImGuiTableFlags_SizingMask_) | policies[n].Value;
        ImGui::EndCombo(ctx);
    }
    ImGui::SameLine(ctx);
    ImGui::TextDisabled(ctx, "(?)");
    if (ImGui::BeginItemTooltip(ctx))
    {
        ImGui::PushTextWrapPos(ctx, ImGui::GetFontSize(ctx) * 50.0f);
        for (int m = 0; m < IM_ARRAYSIZE(policies); m++)
        {
            ImGui::Separator(ctx);
            ImGui::Text(ctx, "%s:", policies[m].Name);
            ImGui::Separator(ctx);
            ImGui::SetCursorPosX(ctx, ImGui::GetCursorPosX(ctx) + ImGui::GetStyle(ctx).IndentSpacing * 0.5f);
            ImGui::TextUnformatted(ctx, policies[m].Tooltip);
        }
        ImGui::PopTextWrapPos(ctx);
        ImGui::EndTooltip(ctx);
    }
}

static void EditTableColumnsFlags(ImGuiContext* ctx, ImGuiTableColumnFlags* p_flags)
{
    ImGui::CheckboxFlags(ctx, "_Disabled", p_flags, ImGuiTableColumnFlags_Disabled); ImGui::SameLine(ctx); HelpMarker(ctx, "Master disable flag (also hide from context menu)");
    ImGui::CheckboxFlags(ctx, "_DefaultHide", p_flags, ImGuiTableColumnFlags_DefaultHide);
    ImGui::CheckboxFlags(ctx, "_DefaultSort", p_flags, ImGuiTableColumnFlags_DefaultSort);
    if (ImGui::CheckboxFlags(ctx, "_WidthStretch", p_flags, ImGuiTableColumnFlags_WidthStretch))
        *p_flags &= ~(ImGuiTableColumnFlags_WidthMask_ ^ ImGuiTableColumnFlags_WidthStretch);
    if (ImGui::CheckboxFlags(ctx, "_WidthFixed", p_flags, ImGuiTableColumnFlags_WidthFixed))
        *p_flags &= ~(ImGuiTableColumnFlags_WidthMask_ ^ ImGuiTableColumnFlags_WidthFixed);
    ImGui::CheckboxFlags(ctx, "_NoResize", p_flags, ImGuiTableColumnFlags_NoResize);
    ImGui::CheckboxFlags(ctx, "_NoReorder", p_flags, ImGuiTableColumnFlags_NoReorder);
    ImGui::CheckboxFlags(ctx, "_NoHide", p_flags, ImGuiTableColumnFlags_NoHide);
    ImGui::CheckboxFlags(ctx, "_NoClip", p_flags, ImGuiTableColumnFlags_NoClip);
    ImGui::CheckboxFlags(ctx, "_NoSort", p_flags, ImGuiTableColumnFlags_NoSort);
    ImGui::CheckboxFlags(ctx, "_NoSortAscending", p_flags, ImGuiTableColumnFlags_NoSortAscending);
    ImGui::CheckboxFlags(ctx, "_NoSortDescending", p_flags, ImGuiTableColumnFlags_NoSortDescending);
    ImGui::CheckboxFlags(ctx, "_NoHeaderLabel", p_flags, ImGuiTableColumnFlags_NoHeaderLabel);
    ImGui::CheckboxFlags(ctx, "_NoHeaderWidth", p_flags, ImGuiTableColumnFlags_NoHeaderWidth);
    ImGui::CheckboxFlags(ctx, "_PreferSortAscending", p_flags, ImGuiTableColumnFlags_PreferSortAscending);
    ImGui::CheckboxFlags(ctx, "_PreferSortDescending", p_flags, ImGuiTableColumnFlags_PreferSortDescending);
    ImGui::CheckboxFlags(ctx, "_IndentEnable", p_flags, ImGuiTableColumnFlags_IndentEnable); ImGui::SameLine(ctx); HelpMarker(ctx, "Default for column 0");
    ImGui::CheckboxFlags(ctx, "_IndentDisable", p_flags, ImGuiTableColumnFlags_IndentDisable); ImGui::SameLine(ctx); HelpMarker(ctx, "Default for column >0");
    ImGui::CheckboxFlags(ctx, "_AngledHeader", p_flags, ImGuiTableColumnFlags_AngledHeader);
}

static void ShowTableColumnsStatusFlags(ImGuiContext* ctx, ImGuiTableColumnFlags flags)
{
    ImGui::CheckboxFlags(ctx, "_IsEnabled", &flags, ImGuiTableColumnFlags_IsEnabled);
    ImGui::CheckboxFlags(ctx, "_IsVisible", &flags, ImGuiTableColumnFlags_IsVisible);
    ImGui::CheckboxFlags(ctx, "_IsSorted", &flags, ImGuiTableColumnFlags_IsSorted);
    ImGui::CheckboxFlags(ctx, "_IsHovered", &flags, ImGuiTableColumnFlags_IsHovered);
}

static void ShowDemoWindowTables(ImGuiContext* ctx)
{
    //ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    IMGUI_DEMO_MARKER("Tables");
    if (!ImGui::CollapsingHeader(ctx, "Tables & Columns"))
        return;

    // Using those as a base value to create width/height that are factor of the size of our font
    const float TEXT_BASE_WIDTH = ImGui::CalcTextSize(ctx, "A").x;
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing(ctx);

    ImGui::PushID(ctx, "Tables");

    int open_action = -1;
    if (ImGui::Button(ctx, "Expand all"))
        open_action = 1;
    ImGui::SameLine(ctx);
    if (ImGui::Button(ctx, "Collapse all"))
        open_action = 0;
    ImGui::SameLine(ctx);

    // Options
    static bool disable_indent = false;
    ImGui::Checkbox(ctx, "Disable tree indentation", &disable_indent);
    ImGui::SameLine(ctx);
    HelpMarker(ctx, "Disable the indenting of tree nodes so demo tables can use the full window width.");
    ImGui::Separator(ctx);
    if (disable_indent)
        ImGui::PushStyleVar(ctx, ImGuiStyleVar_IndentSpacing, 0.0f);

    // About Styling of tables
    // Most settings are configured on a per-table basis via the flags passed to BeginTable() and TableSetupColumns APIs.
    // There are however a few settings that a shared and part of the ImGuiStyle structure:
    //   style.CellPadding                          // Padding within each cell
    //   style.Colors[ImGuiCol_TableHeaderBg]       // Table header background
    //   style.Colors[ImGuiCol_TableBorderStrong]   // Table outer and header borders
    //   style.Colors[ImGuiCol_TableBorderLight]    // Table inner borders
    //   style.Colors[ImGuiCol_TableRowBg]          // Table row background when ImGuiTableFlags_RowBg is enabled (even rows)
    //   style.Colors[ImGuiCol_TableRowBgAlt]       // Table row background when ImGuiTableFlags_RowBg is enabled (odds rows)

    // Demos
    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Basic");
    if (ImGui::TreeNode(ctx, "Basic"))
    {
        // Here we will showcase three different ways to output a table.
        // They are very simple variations of a same thing!

        // [Method 1] Using TableNextRow() to create a new row, and TableSetColumnIndex() to select the column.
        // In many situations, this is the most flexible and easy to use pattern.
        HelpMarker(ctx, "Using TableNextRow() + calling TableSetColumnIndex() _before_ each cell, in a loop.");
        if (ImGui::BeginTable(ctx, "table1", 3))
        {
            for (int row = 0; row < 4; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Text(ctx, "Row %d Column %d", row, column);
                }
            }
            ImGui::EndTable(ctx);
        }

        // [Method 2] Using TableNextColumn() called multiple times, instead of using a for loop + TableSetColumnIndex().
        // This is generally more convenient when you have code manually submitting the contents of each column.
        HelpMarker(ctx, "Using TableNextRow() + calling TableNextColumn() _before_ each cell, manually.");
        if (ImGui::BeginTable(ctx, "table2", 3))
        {
            for (int row = 0; row < 4; row++)
            {
                ImGui::TableNextRow(ctx);
                ImGui::TableNextColumn(ctx);
                ImGui::Text(ctx, "Row %d", row);
                ImGui::TableNextColumn(ctx);
                ImGui::Text(ctx, "Some contents");
                ImGui::TableNextColumn(ctx);
                ImGui::Text(ctx, "123.456");
            }
            ImGui::EndTable(ctx);
        }

        // [Method 3] We call TableNextColumn() _before_ each cell. We never call TableNextRow(),
        // as TableNextColumn() will automatically wrap around and create new rows as needed.
        // This is generally more convenient when your cells all contains the same type of data.
        HelpMarker(ctx, 
            "Only using TableNextColumn(), which tends to be convenient for tables where every cell contains the same type of contents.\n"
            "This is also more similar to the old NextColumn() function of the Columns API, and provided to facilitate the Columns->Tables API transition.");
        if (ImGui::BeginTable(ctx, "table3", 3))
        {
            for (int item = 0; item < 14; item++)
            {
                ImGui::TableNextColumn(ctx);
                ImGui::Text(ctx, "Item %d", item);
            }
            ImGui::EndTable(ctx);
        }

        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Borders, background");
    if (ImGui::TreeNode(ctx, "Borders, background"))
    {
        // Expose a few Borders related flags interactively
        enum ContentsType { CT_Text, CT_FillButton };
        static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
        static bool display_headers = false;
        static int contents_type = CT_Text;

        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_RowBg", &flags, ImGuiTableFlags_RowBg);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Borders", &flags, ImGuiTableFlags_Borders);
        ImGui::SameLine(ctx); HelpMarker(ctx, "ImGuiTableFlags_Borders\n = ImGuiTableFlags_BordersInnerV\n | ImGuiTableFlags_BordersOuterV\n | ImGuiTableFlags_BordersInnerV\n | ImGuiTableFlags_BordersOuterH");
        ImGui::Indent(ctx);

        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersH", &flags, ImGuiTableFlags_BordersH);
        ImGui::Indent(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersOuterH", &flags, ImGuiTableFlags_BordersOuterH);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersInnerH", &flags, ImGuiTableFlags_BordersInnerH);
        ImGui::Unindent(ctx);

        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersV", &flags, ImGuiTableFlags_BordersV);
        ImGui::Indent(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersOuterV", &flags, ImGuiTableFlags_BordersOuterV);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersInnerV", &flags, ImGuiTableFlags_BordersInnerV);
        ImGui::Unindent(ctx);

        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersOuter", &flags, ImGuiTableFlags_BordersOuter);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersInner", &flags, ImGuiTableFlags_BordersInner);
        ImGui::Unindent(ctx);

        ImGui::AlignTextToFramePadding(ctx); ImGui::Text(ctx, "Cell contents:");
        ImGui::SameLine(ctx); ImGui::RadioButton(ctx, "Text", &contents_type, CT_Text);
        ImGui::SameLine(ctx); ImGui::RadioButton(ctx, "FillButton", &contents_type, CT_FillButton);
        ImGui::Checkbox(ctx, "Display headers", &display_headers);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoBordersInBody", &flags, ImGuiTableFlags_NoBordersInBody); ImGui::SameLine(ctx); HelpMarker(ctx, "Disable vertical borders in columns Body (borders will always appear in Headers");
        PopStyleCompact(ctx);

        if (ImGui::BeginTable(ctx, "table1", 3, flags))
        {
            // Display headers so we can inspect their interaction with borders.
            // (Headers are not the main purpose of this section of the demo, so we are not elaborating on them too much. See other sections for details)
            if (display_headers)
            {
                ImGui::TableSetupColumn(ctx, "One");
                ImGui::TableSetupColumn(ctx, "Two");
                ImGui::TableSetupColumn(ctx, "Three");
                ImGui::TableHeadersRow(ctx);
            }

            for (int row = 0; row < 5; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    char buf[32];
                    sprintf(buf, "Hello %d,%d", column, row);
                    if (contents_type == CT_Text)
                        ImGui::TextUnformatted(ctx, buf);
                    else if (contents_type == CT_FillButton)
                        ImGui::Button(ctx, buf, ImVec2(-FLT_MIN, 0.0f));
                }
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Resizable, stretch");
    if (ImGui::TreeNode(ctx, "Resizable, stretch"))
    {
        // By default, if we don't enable ScrollX the sizing policy for each column is "Stretch"
        // All columns maintain a sizing weight, and they will occupy all available width.
        static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody;
        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Resizable", &flags, ImGuiTableFlags_Resizable);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersV", &flags, ImGuiTableFlags_BordersV);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Using the _Resizable flag automatically enables the _BordersInnerV flag as well, this is why the resize borders are still showing when unchecking this.");
        PopStyleCompact(ctx);

        if (ImGui::BeginTable(ctx, "table1", 3, flags))
        {
            for (int row = 0; row < 5; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Text(ctx, "Hello %d,%d", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Resizable, fixed");
    if (ImGui::TreeNode(ctx, "Resizable, fixed"))
    {
        // Here we use ImGuiTableFlags_SizingFixedFit (even though _ScrollX is not set)
        // So columns will adopt the "Fixed" policy and will maintain a fixed width regardless of the whole available width (unless table is small)
        // If there is not enough available width to fit all columns, they will however be resized down.
        // FIXME-TABLE: Providing a stretch-on-init would make sense especially for tables which don't have saved settings
        HelpMarker(ctx, 
            "Using _Resizable + _SizingFixedFit flags.\n"
            "Fixed-width columns generally makes more sense if you want to use horizontal scrolling.\n\n"
            "Double-click a column border to auto-fit the column to its contents.");
        PushStyleCompact(ctx);
        static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody;
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoHostExtendX", &flags, ImGuiTableFlags_NoHostExtendX);
        PopStyleCompact(ctx);

        if (ImGui::BeginTable(ctx, "table1", 3, flags))
        {
            for (int row = 0; row < 5; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Text(ctx, "Hello %d,%d", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Resizable, mixed");
    if (ImGui::TreeNode(ctx, "Resizable, mixed"))
    {
        HelpMarker(ctx, 
            "Using TableSetupColumn() to alter resizing policy on a per-column basis.\n\n"
            "When combining Fixed and Stretch columns, generally you only want one, maybe two trailing columns to use _WidthStretch.");
        static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

        if (ImGui::BeginTable(ctx, "table1", 3, flags))
        {
            ImGui::TableSetupColumn(ctx, "AAA", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn(ctx, "BBB", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn(ctx, "CCC", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow(ctx);
            for (int row = 0; row < 5; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Text(ctx, "%s %d,%d", (column == 2) ? "Stretch" : "Fixed", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }
        if (ImGui::BeginTable(ctx, "table2", 6, flags))
        {
            ImGui::TableSetupColumn(ctx, "AAA", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn(ctx, "BBB", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn(ctx, "CCC", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn(ctx, "DDD", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(ctx, "EEE", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(ctx, "FFF", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableHeadersRow(ctx);
            for (int row = 0; row < 5; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 6; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Text(ctx, "%s %d,%d", (column >= 3) ? "Stretch" : "Fixed", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Reorderable, hideable, with headers");
    if (ImGui::TreeNode(ctx, "Reorderable, hideable, with headers"))
    {
        HelpMarker(ctx, 
            "Click and drag column headers to reorder columns.\n\n"
            "Right-click on a header to open a context menu.");
        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;
        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Resizable", &flags, ImGuiTableFlags_Resizable);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Reorderable", &flags, ImGuiTableFlags_Reorderable);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Hideable", &flags, ImGuiTableFlags_Hideable);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoBordersInBody", &flags, ImGuiTableFlags_NoBordersInBody);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoBordersInBodyUntilResize", &flags, ImGuiTableFlags_NoBordersInBodyUntilResize); ImGui::SameLine(ctx); HelpMarker(ctx, "Disable vertical borders in columns Body until hovered for resize (borders will always appear in Headers)");
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_HighlightHoveredColumn", &flags, ImGuiTableFlags_HighlightHoveredColumn);
        PopStyleCompact(ctx);

        if (ImGui::BeginTable(ctx, "table1", 3, flags))
        {
            // Submit columns name with TableSetupColumn() and call TableHeadersRow() to create a row with a header in each column.
            // (Later we will show how TableSetupColumn() has other uses, optional flags, sizing weight etc.)
            ImGui::TableSetupColumn(ctx, "One");
            ImGui::TableSetupColumn(ctx, "Two");
            ImGui::TableSetupColumn(ctx, "Three");
            ImGui::TableHeadersRow(ctx);
            for (int row = 0; row < 6; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Text(ctx, "Hello %d,%d", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }

        // Use outer_size.x == 0.0f instead of default to make the table as tight as possible (only valid when no scrolling and no stretch column)
        if (ImGui::BeginTable(ctx, "table2", 3, flags | ImGuiTableFlags_SizingFixedFit, ImVec2(0.0f, 0.0f)))
        {
            ImGui::TableSetupColumn(ctx, "One");
            ImGui::TableSetupColumn(ctx, "Two");
            ImGui::TableSetupColumn(ctx, "Three");
            ImGui::TableHeadersRow(ctx);
            for (int row = 0; row < 6; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Text(ctx, "Fixed %d,%d", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Padding");
    if (ImGui::TreeNode(ctx, "Padding"))
    {
        // First example: showcase use of padding flags and effect of BorderOuterV/BorderInnerV on X padding.
        // We don't expose BorderOuterH/BorderInnerH here because they have no effect on X padding.
        HelpMarker(ctx, 
            "We often want outer padding activated when any using features which makes the edges of a column visible:\n"
            "e.g.:\n"
            "- BorderOuterV\n"
            "- any form of row selection\n"
            "Because of this, activating BorderOuterV sets the default to PadOuterX. Using PadOuterX or NoPadOuterX you can override the default.\n\n"
            "Actual padding values are using style.CellPadding.\n\n"
            "In this demo we don't show horizontal borders to emphasize how they don't affect default horizontal padding.");

        static ImGuiTableFlags flags1 = ImGuiTableFlags_BordersV;
        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_PadOuterX", &flags1, ImGuiTableFlags_PadOuterX);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Enable outer-most padding (default if ImGuiTableFlags_BordersOuterV is set)");
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoPadOuterX", &flags1, ImGuiTableFlags_NoPadOuterX);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Disable outer-most padding (default if ImGuiTableFlags_BordersOuterV is not set)");
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoPadInnerX", &flags1, ImGuiTableFlags_NoPadInnerX);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Disable inner padding between columns (double inner padding if BordersOuterV is on, single inner padding if BordersOuterV is off)");
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersOuterV", &flags1, ImGuiTableFlags_BordersOuterV);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersInnerV", &flags1, ImGuiTableFlags_BordersInnerV);
        static bool show_headers = false;
        ImGui::Checkbox(ctx, "show_headers", &show_headers);
        PopStyleCompact(ctx);

        if (ImGui::BeginTable(ctx, "table_padding", 3, flags1))
        {
            if (show_headers)
            {
                ImGui::TableSetupColumn(ctx, "One");
                ImGui::TableSetupColumn(ctx, "Two");
                ImGui::TableSetupColumn(ctx, "Three");
                ImGui::TableHeadersRow(ctx);
            }

            for (int row = 0; row < 5; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    if (row == 0)
                    {
                        ImGui::Text(ctx, "Avail %.2f", ImGui::GetContentRegionAvail(ctx).x);
                    }
                    else
                    {
                        char buf[32];
                        sprintf(buf, "Hello %d,%d", column, row);
                        ImGui::Button(ctx, buf, ImVec2(-FLT_MIN, 0.0f));
                    }
                    //if (ImGui::TableGetColumnFlags() & ImGuiTableColumnFlags_IsHovered)
                    //    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(0, 100, 0, 255));
                }
            }
            ImGui::EndTable(ctx);
        }

        // Second example: set style.CellPadding to (0.0) or a custom value.
        // FIXME-TABLE: Vertical border effectively not displayed the same way as horizontal one...
        HelpMarker(ctx, "Setting style.CellPadding to (0,0) or a custom value.");
        static ImGuiTableFlags flags2 = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
        static ImVec2 cell_padding(0.0f, 0.0f);
        static bool show_widget_frame_bg = true;

        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Borders", &flags2, ImGuiTableFlags_Borders);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersH", &flags2, ImGuiTableFlags_BordersH);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersV", &flags2, ImGuiTableFlags_BordersV);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersInner", &flags2, ImGuiTableFlags_BordersInner);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersOuter", &flags2, ImGuiTableFlags_BordersOuter);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_RowBg", &flags2, ImGuiTableFlags_RowBg);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Resizable", &flags2, ImGuiTableFlags_Resizable);
        ImGui::Checkbox(ctx, "show_widget_frame_bg", &show_widget_frame_bg);
        ImGui::SliderFloat2(ctx, "CellPadding", &cell_padding.x, 0.0f, 10.0f, "%.0f");
        PopStyleCompact(ctx);

        ImGui::PushStyleVar(ctx, ImGuiStyleVar_CellPadding, cell_padding);
        if (ImGui::BeginTable(ctx, "table_padding_2", 3, flags2))
        {
            static char text_bufs[3 * 5][16]; // Mini text storage for 3x5 cells
            static bool init = true;
            if (!show_widget_frame_bg)
                ImGui::PushStyleColor(ctx, ImGuiCol_FrameBg, 0);
            for (int cell = 0; cell < 3 * 5; cell++)
            {
                ImGui::TableNextColumn(ctx);
                if (init)
                    strcpy(text_bufs[cell], "edit me");
                ImGui::SetNextItemWidth(ctx, -FLT_MIN);
                ImGui::PushID(ctx, cell);
                ImGui::InputText(ctx, "##cell", text_bufs[cell], IM_ARRAYSIZE(text_bufs[cell]));
                ImGui::PopID(ctx);
            }
            if (!show_widget_frame_bg)
                ImGui::PopStyleColor(ctx);
            init = false;
            ImGui::EndTable(ctx);
        }
        ImGui::PopStyleVar(ctx);

        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Explicit widths");
    if (ImGui::TreeNode(ctx, "Sizing policies"))
    {
        static ImGuiTableFlags flags1 = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ContextMenuInBody;
        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Resizable", &flags1, ImGuiTableFlags_Resizable);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoHostExtendX", &flags1, ImGuiTableFlags_NoHostExtendX);
        PopStyleCompact(ctx);

        static ImGuiTableFlags sizing_policy_flags[4] = { ImGuiTableFlags_SizingFixedFit, ImGuiTableFlags_SizingFixedSame, ImGuiTableFlags_SizingStretchProp, ImGuiTableFlags_SizingStretchSame };
        for (int table_n = 0; table_n < 4; table_n++)
        {
            ImGui::PushID(ctx, table_n);
            ImGui::SetNextItemWidth(ctx, TEXT_BASE_WIDTH * 30);
            EditTableSizingFlags(ctx, &sizing_policy_flags[table_n]);

            // To make it easier to understand the different sizing policy,
            // For each policy: we display one table where the columns have equal contents width, and one where the columns have different contents width.
            if (ImGui::BeginTable(ctx, "table1", 3, sizing_policy_flags[table_n] | flags1))
            {
                for (int row = 0; row < 3; row++)
                {
                    ImGui::TableNextRow(ctx);
                    ImGui::TableNextColumn(ctx); ImGui::Text(ctx, "Oh dear");
                    ImGui::TableNextColumn(ctx); ImGui::Text(ctx, "Oh dear");
                    ImGui::TableNextColumn(ctx); ImGui::Text(ctx, "Oh dear");
                }
                ImGui::EndTable(ctx);
            }
            if (ImGui::BeginTable(ctx, "table2", 3, sizing_policy_flags[table_n] | flags1))
            {
                for (int row = 0; row < 3; row++)
                {
                    ImGui::TableNextRow(ctx);
                    ImGui::TableNextColumn(ctx); ImGui::Text(ctx, "AAAA");
                    ImGui::TableNextColumn(ctx); ImGui::Text(ctx, "BBBBBBBB");
                    ImGui::TableNextColumn(ctx); ImGui::Text(ctx, "CCCCCCCCCCCC");
                }
                ImGui::EndTable(ctx);
            }
            ImGui::PopID(ctx);
        }

        ImGui::Spacing(ctx);
        ImGui::TextUnformatted(ctx, "Advanced");
        ImGui::SameLine(ctx);
        HelpMarker(ctx, "This section allows you to interact and see the effect of various sizing policies depending on whether Scroll is enabled and the contents of your columns.");

        enum ContentsType { CT_ShowWidth, CT_ShortText, CT_LongText, CT_Button, CT_FillButton, CT_InputText };
        static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
        static int contents_type = CT_ShowWidth;
        static int column_count = 3;

        PushStyleCompact(ctx);
        ImGui::PushID(ctx, "Advanced");
        ImGui::PushItemWidth(ctx, TEXT_BASE_WIDTH * 30);
        EditTableSizingFlags(ctx, &flags);
        ImGui::Combo(ctx, "Contents", &contents_type, "Show width\0Short Text\0Long Text\0Button\0Fill Button\0InputText\0");
        if (contents_type == CT_FillButton)
        {
            ImGui::SameLine(ctx);
            HelpMarker(ctx, "Be mindful that using right-alignment (e.g. size.x = -FLT_MIN) creates a feedback loop where contents width can feed into auto-column width can feed into contents width.");
        }
        ImGui::DragInt(ctx, "Columns", &column_count, 0.1f, 1, 64, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Resizable", &flags, ImGuiTableFlags_Resizable);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_PreciseWidths", &flags, ImGuiTableFlags_PreciseWidths);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Disable distributing remainder width to stretched columns (width allocation on a 100-wide table with 3 columns: Without this flag: 33,33,34. With this flag: 33,33,33). With larger number of columns, resizing will appear to be less smooth.");
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_ScrollX", &flags, ImGuiTableFlags_ScrollX);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_ScrollY", &flags, ImGuiTableFlags_ScrollY);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoClip", &flags, ImGuiTableFlags_NoClip);
        ImGui::PopItemWidth(ctx);
        ImGui::PopID(ctx);
        PopStyleCompact(ctx);

        if (ImGui::BeginTable(ctx, "table2", column_count, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 7)))
        {
            for (int cell = 0; cell < 10 * column_count; cell++)
            {
                ImGui::TableNextColumn(ctx);
                int column = ImGui::TableGetColumnIndex(ctx);
                int row = ImGui::TableGetRowIndex(ctx);

                ImGui::PushID(ctx, cell);
                char label[32];
                static char text_buf[32] = "";
                sprintf(label, "Hello %d,%d", column, row);
                switch (contents_type)
                {
                case CT_ShortText:  ImGui::TextUnformatted(ctx, label); break;
                case CT_LongText:   ImGui::Text(ctx, "Some %s text %d,%d\nOver two lines..", column == 0 ? "long" : "longeeer", column, row); break;
                case CT_ShowWidth:  ImGui::Text(ctx, "W: %.1f", ImGui::GetContentRegionAvail(ctx).x); break;
                case CT_Button:     ImGui::Button(ctx, label); break;
                case CT_FillButton: ImGui::Button(ctx, label, ImVec2(-FLT_MIN, 0.0f)); break;
                case CT_InputText:  ImGui::SetNextItemWidth(ctx, -FLT_MIN); ImGui::InputText(ctx, "##", text_buf, IM_ARRAYSIZE(text_buf)); break;
                }
                ImGui::PopID(ctx);
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Vertical scrolling, with clipping");
    if (ImGui::TreeNode(ctx, "Vertical scrolling, with clipping"))
    {
        HelpMarker(ctx, "Here we activate ScrollY, which will create a child window container to allow hosting scrollable contents.\n\nWe also demonstrate using ImGuiListClipper to virtualize the submission of many items.");
        static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_ScrollY", &flags, ImGuiTableFlags_ScrollY);
        PopStyleCompact(ctx);

        // When using ScrollX or ScrollY we need to specify a size for our table container!
        // Otherwise by default the table will fit all available space, like a BeginChild() call.
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 8);
        if (ImGui::BeginTable(ctx, "table_scrolly", 3, flags, outer_size))
        {
            ImGui::TableSetupScrollFreeze(ctx, 0, 1); // Make top row always visible
            ImGui::TableSetupColumn(ctx, "One", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn(ctx, "Two", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn(ctx, "Three", ImGuiTableColumnFlags_None);
            ImGui::TableHeadersRow(ctx);

            // Demonstrate using clipper for large vertical lists
            ImGuiListClipper clipper;
            clipper.Begin(1000);
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImGui::TableNextRow(ctx);
                    for (int column = 0; column < 3; column++)
                    {
                        ImGui::TableSetColumnIndex(ctx, column);
                        ImGui::Text(ctx, "Hello %d,%d", column, row);
                    }
                }
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Horizontal scrolling");
    if (ImGui::TreeNode(ctx, "Horizontal scrolling"))
    {
        HelpMarker(ctx, 
            "When ScrollX is enabled, the default sizing policy becomes ImGuiTableFlags_SizingFixedFit, "
            "as automatically stretching columns doesn't make much sense with horizontal scrolling.\n\n"
            "Also note that as of the current version, you will almost always want to enable ScrollY along with ScrollX,"
            "because the container window won't automatically extend vertically to fix contents (this may be improved in future versions).");
        static ImGuiTableFlags flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
        static int freeze_cols = 1;
        static int freeze_rows = 1;

        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Resizable", &flags, ImGuiTableFlags_Resizable);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_ScrollX", &flags, ImGuiTableFlags_ScrollX);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_ScrollY", &flags, ImGuiTableFlags_ScrollY);
        ImGui::SetNextItemWidth(ctx, ImGui::GetFrameHeight(ctx));
        ImGui::DragInt(ctx, "freeze_cols", &freeze_cols, 0.2f, 0, 9, NULL, ImGuiSliderFlags_NoInput);
        ImGui::SetNextItemWidth(ctx, ImGui::GetFrameHeight(ctx));
        ImGui::DragInt(ctx, "freeze_rows", &freeze_rows, 0.2f, 0, 9, NULL, ImGuiSliderFlags_NoInput);
        PopStyleCompact(ctx);

        // When using ScrollX or ScrollY we need to specify a size for our table container!
        // Otherwise by default the table will fit all available space, like a BeginChild() call.
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 8);
        if (ImGui::BeginTable(ctx, "table_scrollx", 7, flags, outer_size))
        {
            ImGui::TableSetupScrollFreeze(ctx, freeze_cols, freeze_rows);
            ImGui::TableSetupColumn(ctx, "Line #", ImGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
            ImGui::TableSetupColumn(ctx, "One");
            ImGui::TableSetupColumn(ctx, "Two");
            ImGui::TableSetupColumn(ctx, "Three");
            ImGui::TableSetupColumn(ctx, "Four");
            ImGui::TableSetupColumn(ctx, "Five");
            ImGui::TableSetupColumn(ctx, "Six");
            ImGui::TableHeadersRow(ctx);
            for (int row = 0; row < 20; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 7; column++)
                {
                    // Both TableNextColumn() and TableSetColumnIndex() return true when a column is visible or performing width measurement.
                    // Because here we know that:
                    // - A) all our columns are contributing the same to row height
                    // - B) column 0 is always visible,
                    // We only always submit this one column and can skip others.
                    // More advanced per-column clipping behaviors may benefit from polling the status flags via TableGetColumnFlags().
                    if (!ImGui::TableSetColumnIndex(ctx, column) && column > 0)
                        continue;
                    if (column == 0)
                        ImGui::Text(ctx, "Line %d", row);
                    else
                        ImGui::Text(ctx, "Hello world %d,%d", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }

        ImGui::Spacing(ctx);
        ImGui::TextUnformatted(ctx, "Stretch + ScrollX");
        ImGui::SameLine(ctx);
        HelpMarker(ctx, 
            "Showcase using Stretch columns + ScrollX together: "
            "this is rather unusual and only makes sense when specifying an 'inner_width' for the table!\n"
            "Without an explicit value, inner_width is == outer_size.x and therefore using Stretch columns + ScrollX together doesn't make sense.");
        static ImGuiTableFlags flags2 = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg | ImGuiTableFlags_ContextMenuInBody;
        static float inner_width = 1000.0f;
        PushStyleCompact(ctx);
        ImGui::PushID(ctx, "flags3");
        ImGui::PushItemWidth(ctx, TEXT_BASE_WIDTH * 30);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_ScrollX", &flags2, ImGuiTableFlags_ScrollX);
        ImGui::DragFloat(ctx, "inner_width", &inner_width, 1.0f, 0.0f, FLT_MAX, "%.1f");
        ImGui::PopItemWidth(ctx);
        ImGui::PopID(ctx);
        PopStyleCompact(ctx);
        if (ImGui::BeginTable(ctx, "table2", 7, flags2, outer_size, inner_width))
        {
            for (int cell = 0; cell < 20 * 7; cell++)
            {
                ImGui::TableNextColumn(ctx);
                ImGui::Text(ctx, "Hello world %d,%d", ImGui::TableGetColumnIndex(ctx), ImGui::TableGetRowIndex(ctx));
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Columns flags");
    if (ImGui::TreeNode(ctx, "Columns flags"))
    {
        // Create a first table just to show all the options/flags we want to make visible in our example!
        const int column_count = 3;
        const char* column_names[column_count] = { "One", "Two", "Three" };
        static ImGuiTableColumnFlags column_flags[column_count] = { ImGuiTableColumnFlags_DefaultSort, ImGuiTableColumnFlags_None, ImGuiTableColumnFlags_DefaultHide };
        static ImGuiTableColumnFlags column_flags_out[column_count] = { 0, 0, 0 }; // Output from TableGetColumnFlags()

        if (ImGui::BeginTable(ctx, "table_columns_flags_checkboxes", column_count, ImGuiTableFlags_None))
        {
            PushStyleCompact(ctx);
            for (int column = 0; column < column_count; column++)
            {
                ImGui::TableNextColumn(ctx);
                ImGui::PushID(ctx, column);
                ImGui::AlignTextToFramePadding(ctx); // FIXME-TABLE: Workaround for wrong text baseline propagation across columns
                ImGui::Text(ctx, "'%s'", column_names[column]);
                ImGui::Spacing(ctx);
                ImGui::Text(ctx, "Input flags:");
                EditTableColumnsFlags(ctx, &column_flags[column]);
                ImGui::Spacing(ctx);
                ImGui::Text(ctx, "Output flags:");
                ImGui::BeginDisabled(ctx);
                ShowTableColumnsStatusFlags(ctx, column_flags_out[column]);
                ImGui::EndDisabled(ctx);
                ImGui::PopID(ctx);
            }
            PopStyleCompact(ctx);
            ImGui::EndTable(ctx);
        }

        // Create the real table we care about for the example!
        // We use a scrolling table to be able to showcase the difference between the _IsEnabled and _IsVisible flags above, otherwise in
        // a non-scrolling table columns are always visible (unless using ImGuiTableFlags_NoKeepColumnsVisible + resizing the parent window down)
        const ImGuiTableFlags flags
            = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY
            | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV
            | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable;
        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 9);
        if (ImGui::BeginTable(ctx, "table_columns_flags", column_count, flags, outer_size))
        {
            bool has_angled_header = false;
            for (int column = 0; column < column_count; column++)
            {
                has_angled_header |= (column_flags[column] & ImGuiTableColumnFlags_AngledHeader) != 0;
                ImGui::TableSetupColumn(ctx, column_names[column], column_flags[column]);
            }
            if (has_angled_header)
                ImGui::TableAngledHeadersRow(ctx);
            ImGui::TableHeadersRow(ctx);
            for (int column = 0; column < column_count; column++)
                column_flags_out[column] = ImGui::TableGetColumnFlags(ctx, column);
            float indent_step = (float)((int)TEXT_BASE_WIDTH / 2);
            for (int row = 0; row < 8; row++)
            {
                ImGui::Indent(ctx, indent_step); // Add some indentation to demonstrate usage of per-column IndentEnable/IndentDisable flags.
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < column_count; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Text(ctx, "%s %s", (column == 0) ? "Indented" : "Hello", ImGui::TableGetColumnName(ctx, column));
                }
            }
            ImGui::Unindent(ctx, indent_step * 8.0f);

            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Columns widths");
    if (ImGui::TreeNode(ctx, "Columns widths"))
    {
        HelpMarker(ctx, "Using TableSetupColumn() to setup default width.");

        static ImGuiTableFlags flags1 = ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBodyUntilResize;
        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Resizable", &flags1, ImGuiTableFlags_Resizable);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoBordersInBodyUntilResize", &flags1, ImGuiTableFlags_NoBordersInBodyUntilResize);
        PopStyleCompact(ctx);
        if (ImGui::BeginTable(ctx, "table1", 3, flags1))
        {
            // We could also set ImGuiTableFlags_SizingFixedFit on the table and all columns will default to ImGuiTableColumnFlags_WidthFixed.
            ImGui::TableSetupColumn(ctx, "one", ImGuiTableColumnFlags_WidthFixed, 100.0f); // Default to 100.0f
            ImGui::TableSetupColumn(ctx, "two", ImGuiTableColumnFlags_WidthFixed, 200.0f); // Default to 200.0f
            ImGui::TableSetupColumn(ctx, "three", ImGuiTableColumnFlags_WidthFixed);       // Default to auto
            ImGui::TableHeadersRow(ctx);
            for (int row = 0; row < 4; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    if (row == 0)
                        ImGui::Text(ctx, "(w: %5.1f)", ImGui::GetContentRegionAvail(ctx).x);
                    else
                        ImGui::Text(ctx, "Hello %d,%d", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }

        HelpMarker(ctx, "Using TableSetupColumn() to setup explicit width.\n\nUnless _NoKeepColumnsVisible is set, fixed columns with set width may still be shrunk down if there's not enough space in the host.");

        static ImGuiTableFlags flags2 = ImGuiTableFlags_None;
        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoKeepColumnsVisible", &flags2, ImGuiTableFlags_NoKeepColumnsVisible);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersInnerV", &flags2, ImGuiTableFlags_BordersInnerV);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersOuterV", &flags2, ImGuiTableFlags_BordersOuterV);
        PopStyleCompact(ctx);
        if (ImGui::BeginTable(ctx, "table2", 4, flags2))
        {
            // We could also set ImGuiTableFlags_SizingFixedFit on the table and all columns will default to ImGuiTableColumnFlags_WidthFixed.
            ImGui::TableSetupColumn(ctx, "", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn(ctx, "", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 15.0f);
            ImGui::TableSetupColumn(ctx, "", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 30.0f);
            ImGui::TableSetupColumn(ctx, "", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 15.0f);
            for (int row = 0; row < 5; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 4; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    if (row == 0)
                        ImGui::Text(ctx, "(w: %5.1f)", ImGui::GetContentRegionAvail(ctx).x);
                    else
                        ImGui::Text(ctx, "Hello %d,%d", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Nested tables");
    if (ImGui::TreeNode(ctx, "Nested tables"))
    {
        HelpMarker(ctx, "This demonstrates embedding a table into another table cell.");

        if (ImGui::BeginTable(ctx, "table_nested1", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
        {
            ImGui::TableSetupColumn(ctx, "A0");
            ImGui::TableSetupColumn(ctx, "A1");
            ImGui::TableHeadersRow(ctx);

            ImGui::TableNextColumn(ctx);
            ImGui::Text(ctx, "A0 Row 0");
            {
                float rows_height = TEXT_BASE_HEIGHT * 2;
                if (ImGui::BeginTable(ctx, "table_nested2", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
                {
                    ImGui::TableSetupColumn(ctx, "B0");
                    ImGui::TableSetupColumn(ctx, "B1");
                    ImGui::TableHeadersRow(ctx);

                    ImGui::TableNextRow(ctx, ImGuiTableRowFlags_None, rows_height);
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "B0 Row 0");
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "B1 Row 0");
                    ImGui::TableNextRow(ctx, ImGuiTableRowFlags_None, rows_height);
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "B0 Row 1");
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "B1 Row 1");

                    ImGui::EndTable(ctx);
                }
            }
            ImGui::TableNextColumn(ctx); ImGui::Text(ctx, "A1 Row 0");
            ImGui::TableNextColumn(ctx); ImGui::Text(ctx, "A0 Row 1");
            ImGui::TableNextColumn(ctx); ImGui::Text(ctx, "A1 Row 1");
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Row height");
    if (ImGui::TreeNode(ctx, "Row height"))
    {
        HelpMarker(ctx, "You can pass a 'min_row_height' to TableNextRow().\n\nRows are padded with 'style.CellPadding.y' on top and bottom, so effectively the minimum row height will always be >= 'style.CellPadding.y * 2.0f'.\n\nWe cannot honor a _maximum_ row height as that would require a unique clipping rectangle per row.");
        if (ImGui::BeginTable(ctx, "table_row_height", 1, ImGuiTableFlags_Borders))
        {
            for (int row = 0; row < 8; row++)
            {
                float min_row_height = (float)(int)(TEXT_BASE_HEIGHT * 0.30f * row);
                ImGui::TableNextRow(ctx, ImGuiTableRowFlags_None, min_row_height);
                ImGui::TableNextColumn(ctx);
                ImGui::Text(ctx, "min_row_height = %.2f", min_row_height);
            }
            ImGui::EndTable(ctx);
        }

        HelpMarker(ctx, "Showcase using SameLine(0,0) to share Current Line Height between cells.\n\nPlease note that Tables Row Height is not the same thing as Current Line Height, as a table cell may contains multiple lines.");
        if (ImGui::BeginTable(ctx, "table_share_lineheight", 2, ImGuiTableFlags_Borders))
        {
            ImGui::TableNextRow(ctx);
            ImGui::TableNextColumn(ctx);
            ImGui::ColorButton(ctx, "##1", ImVec4(0.13f, 0.26f, 0.40f, 1.0f), ImGuiColorEditFlags_None, ImVec2(40, 40));
            ImGui::TableNextColumn(ctx);
            ImGui::Text(ctx, "Line 1");
            ImGui::Text(ctx, "Line 2");

            ImGui::TableNextRow(ctx);
            ImGui::TableNextColumn(ctx);
            ImGui::ColorButton(ctx, "##2", ImVec4(0.13f, 0.26f, 0.40f, 1.0f), ImGuiColorEditFlags_None, ImVec2(40, 40));
            ImGui::TableNextColumn(ctx);
            ImGui::SameLine(ctx, 0.0f, 0.0f); // Reuse line height from previous column
            ImGui::Text(ctx, "Line 1, with SameLine(0,0)");
            ImGui::Text(ctx, "Line 2");

            ImGui::EndTable(ctx);
        }

        HelpMarker(ctx, "Showcase altering CellPadding.y between rows. Note that CellPadding.x is locked for the entire table.");
        if (ImGui::BeginTable(ctx, "table_changing_cellpadding_y", 1, ImGuiTableFlags_Borders))
        {
            ImGuiStyle& style = ImGui::GetStyle(ctx);
            for (int row = 0; row < 8; row++)
            {
                if ((row % 3) == 2)
                    ImGui::PushStyleVar(ctx, ImGuiStyleVar_CellPadding, ImVec2(style.CellPadding.x, 20.0f));
                ImGui::TableNextRow(ctx, ImGuiTableRowFlags_None);
                ImGui::TableNextColumn(ctx);
                ImGui::Text(ctx, "CellPadding.y = %.2f", style.CellPadding.y);
                if ((row % 3) == 2)
                    ImGui::PopStyleVar(ctx);
            }
            ImGui::EndTable(ctx);
        }

        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Outer size");
    if (ImGui::TreeNode(ctx, "Outer size"))
    {
        // Showcasing use of ImGuiTableFlags_NoHostExtendX and ImGuiTableFlags_NoHostExtendY
        // Important to that note how the two flags have slightly different behaviors!
        ImGui::Text(ctx, "Using NoHostExtendX and NoHostExtendY:");
        PushStyleCompact(ctx);
        static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoHostExtendX", &flags, ImGuiTableFlags_NoHostExtendX);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Make outer width auto-fit to columns, overriding outer_size.x value.\n\nOnly available when ScrollX/ScrollY are disabled and Stretch columns are not used.");
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoHostExtendY", &flags, ImGuiTableFlags_NoHostExtendY);
        ImGui::SameLine(ctx); HelpMarker(ctx, "Make outer height stop exactly at outer_size.y (prevent auto-extending table past the limit).\n\nOnly available when ScrollX/ScrollY are disabled. Data below the limit will be clipped and not visible.");
        PopStyleCompact(ctx);

        ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 5.5f);
        if (ImGui::BeginTable(ctx, "table1", 3, flags, outer_size))
        {
            for (int row = 0; row < 10; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "Cell %d,%d", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }
        ImGui::SameLine(ctx);
        ImGui::Text(ctx, "Hello!");

        ImGui::Spacing(ctx);

        ImGui::Text(ctx, "Using explicit size:");
        if (ImGui::BeginTable(ctx, "table2", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(TEXT_BASE_WIDTH * 30, 0.0f)))
        {
            for (int row = 0; row < 5; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "Cell %d,%d", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }
        ImGui::SameLine(ctx);
        if (ImGui::BeginTable(ctx, "table3", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(TEXT_BASE_WIDTH * 30, 0.0f)))
        {
            for (int row = 0; row < 3; row++)
            {
                ImGui::TableNextRow(ctx, 0, TEXT_BASE_HEIGHT * 1.5f);
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "Cell %d,%d", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }

        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Background color");
    if (ImGui::TreeNode(ctx, "Background color"))
    {
        static ImGuiTableFlags flags = ImGuiTableFlags_RowBg;
        static int row_bg_type = 1;
        static int row_bg_target = 1;
        static int cell_bg_type = 1;

        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Borders", &flags, ImGuiTableFlags_Borders);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_RowBg", &flags, ImGuiTableFlags_RowBg);
        ImGui::SameLine(ctx); HelpMarker(ctx, "ImGuiTableFlags_RowBg automatically sets RowBg0 to alternative colors pulled from the Style.");
        ImGui::Combo(ctx, "row bg type", (int*)&row_bg_type, "None\0Red\0Gradient\0");
        ImGui::Combo(ctx, "row bg target", (int*)&row_bg_target, "RowBg0\0RowBg1\0"); ImGui::SameLine(ctx); HelpMarker(ctx, "Target RowBg0 to override the alternating odd/even colors,\nTarget RowBg1 to blend with them.");
        ImGui::Combo(ctx, "cell bg type", (int*)&cell_bg_type, "None\0Blue\0"); ImGui::SameLine(ctx); HelpMarker(ctx, "We are colorizing cells to B1->C2 here.");
        IM_ASSERT(row_bg_type >= 0 && row_bg_type <= 2);
        IM_ASSERT(row_bg_target >= 0 && row_bg_target <= 1);
        IM_ASSERT(cell_bg_type >= 0 && cell_bg_type <= 1);
        PopStyleCompact(ctx);

        if (ImGui::BeginTable(ctx, "table1", 5, flags))
        {
            for (int row = 0; row < 6; row++)
            {
                ImGui::TableNextRow(ctx);

                // Demonstrate setting a row background color with 'ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBgX, ...)'
                // We use a transparent color so we can see the one behind in case our target is RowBg1 and RowBg0 was already targeted by the ImGuiTableFlags_RowBg flag.
                if (row_bg_type != 0)
                {
                    ImU32 row_bg_color = ImGui::GetColorU32(ctx, row_bg_type == 1 ? ImVec4(0.7f, 0.3f, 0.3f, 0.65f) : ImVec4(0.2f + row * 0.1f, 0.2f, 0.2f, 0.65f)); // Flat or Gradient?
                    ImGui::TableSetBgColor(ctx, ImGuiTableBgTarget_RowBg0 + row_bg_target, row_bg_color);
                }

                // Fill cells
                for (int column = 0; column < 5; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Text(ctx, "%c%c", 'A' + row, '0' + column);

                    // Change background of Cells B1->C2
                    // Demonstrate setting a cell background color with 'ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ...)'
                    // (the CellBg color will be blended over the RowBg and ColumnBg colors)
                    // We can also pass a column number as a third parameter to TableSetBgColor() and do this outside the column loop.
                    if (row >= 1 && row <= 2 && column >= 1 && column <= 2 && cell_bg_type == 1)
                    {
                        ImU32 cell_bg_color = ImGui::GetColorU32(ctx, ImVec4(0.3f, 0.3f, 0.7f, 0.65f));
                        ImGui::TableSetBgColor(ctx, ImGuiTableBgTarget_CellBg, cell_bg_color);
                    }
                }
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Tree view");
    if (ImGui::TreeNode(ctx, "Tree view"))
    {
        static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

        static ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_SpanAllColumns;
        ImGui::CheckboxFlags(ctx, "ImGuiTreeNodeFlags_SpanFullWidth", &tree_node_flags, ImGuiTreeNodeFlags_SpanFullWidth);
        ImGui::CheckboxFlags(ctx, "ImGuiTreeNodeFlags_SpanAllColumns", &tree_node_flags, ImGuiTreeNodeFlags_SpanAllColumns);

        HelpMarker(ctx, "See \"Columns flags\" section to configure how indentation is applied to individual columns.");
        if (ImGui::BeginTable(ctx, "3ways", 3, flags))
        {
            // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
            ImGui::TableSetupColumn(ctx, "Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(ctx, "Size", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
            ImGui::TableSetupColumn(ctx, "Type", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 18.0f);
            ImGui::TableHeadersRow(ctx);

            // Simple storage to output a dummy file-system.
            struct MyTreeNode
            {
                const char*     Name;
                const char*     Type;
                int             Size;
                int             ChildIdx;
                int             ChildCount;
                static void DisplayNode(ImGuiContext* ctx, const MyTreeNode* node, const MyTreeNode* all_nodes)
                {
                    ImGui::TableNextRow(ctx);
                    ImGui::TableNextColumn(ctx);
                    const bool is_folder = (node->ChildCount > 0);
                    if (is_folder)
                    {
                        bool open = ImGui::TreeNodeEx(ctx, node->Name, tree_node_flags);
                        ImGui::TableNextColumn(ctx);
                        ImGui::TextDisabled(ctx, "--");
                        ImGui::TableNextColumn(ctx);
                        ImGui::TextUnformatted(ctx, node->Type);
                        if (open)
                        {
                            for (int child_n = 0; child_n < node->ChildCount; child_n++)
                                DisplayNode(ctx, &all_nodes[node->ChildIdx + child_n], all_nodes);
                            ImGui::TreePop(ctx);
                        }
                    }
                    else
                    {
                        ImGui::TreeNodeEx(ctx, node->Name, tree_node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                        ImGui::TableNextColumn(ctx);
                        ImGui::Text(ctx, "%d", node->Size);
                        ImGui::TableNextColumn(ctx);
                        ImGui::TextUnformatted(ctx, node->Type);
                    }
                }
            };
            static const MyTreeNode nodes[] =
            {
                { "Root",                         "Folder",       -1,       1, 3    }, // 0
                { "Music",                        "Folder",       -1,       4, 2    }, // 1
                { "Textures",                     "Folder",       -1,       6, 3    }, // 2
                { "desktop.ini",                  "System file",  1024,    -1,-1    }, // 3
                { "File1_a.wav",                  "Audio file",   123000,  -1,-1    }, // 4
                { "File1_b.wav",                  "Audio file",   456000,  -1,-1    }, // 5
                { "Image001.png",                 "Image file",   203128,  -1,-1    }, // 6
                { "Copy of Image001.png",         "Image file",   203256,  -1,-1    }, // 7
                { "Copy of Image001 (Final2).png","Image file",   203512,  -1,-1    }, // 8
            };

            MyTreeNode::DisplayNode(ctx, &nodes[0], nodes);

            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Item width");
    if (ImGui::TreeNode(ctx, "Item width"))
    {
        HelpMarker(ctx, 
            "Showcase using PushItemWidth() and how it is preserved on a per-column basis.\n\n"
            "Note that on auto-resizing non-resizable fixed columns, querying the content width for e.g. right-alignment doesn't make sense.");
        if (ImGui::BeginTable(ctx, "table_item_width", 3, ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn(ctx, "small");
            ImGui::TableSetupColumn(ctx, "half");
            ImGui::TableSetupColumn(ctx, "right-align");
            ImGui::TableHeadersRow(ctx);

            for (int row = 0; row < 3; row++)
            {
                ImGui::TableNextRow(ctx);
                if (row == 0)
                {
                    // Setup ItemWidth once (instead of setting up every time, which is also possible but less efficient)
                    ImGui::TableSetColumnIndex(ctx, 0);
                    ImGui::PushItemWidth(ctx, TEXT_BASE_WIDTH * 3.0f); // Small
                    ImGui::TableSetColumnIndex(ctx, 1);
                    ImGui::PushItemWidth(ctx, -ImGui::GetContentRegionAvail(ctx).x * 0.5f);
                    ImGui::TableSetColumnIndex(ctx, 2);
                    ImGui::PushItemWidth(ctx, -FLT_MIN); // Right-aligned
                }

                // Draw our contents
                static float dummy_f = 0.0f;
                ImGui::PushID(ctx, row);
                ImGui::TableSetColumnIndex(ctx, 0);
                ImGui::SliderFloat(ctx, "float0", &dummy_f, 0.0f, 1.0f);
                ImGui::TableSetColumnIndex(ctx, 1);
                ImGui::SliderFloat(ctx, "float1", &dummy_f, 0.0f, 1.0f);
                ImGui::TableSetColumnIndex(ctx, 2);
                ImGui::SliderFloat(ctx, "##float2", &dummy_f, 0.0f, 1.0f); // No visible label since right-aligned
                ImGui::PopID(ctx);
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    // Demonstrate using TableHeader() calls instead of TableHeadersRow()
    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Custom headers");
    if (ImGui::TreeNode(ctx, "Custom headers"))
    {
        const int COLUMNS_COUNT = 3;
        if (ImGui::BeginTable(ctx, "table_custom_headers", COLUMNS_COUNT, ImGuiTableFlags_Borders | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
        {
            ImGui::TableSetupColumn(ctx, "Apricot");
            ImGui::TableSetupColumn(ctx, "Banana");
            ImGui::TableSetupColumn(ctx, "Cherry");

            // Dummy entire-column selection storage
            // FIXME: It would be nice to actually demonstrate full-featured selection using those checkbox.
            static bool column_selected[3] = {};

            // Instead of calling TableHeadersRow() we'll submit custom headers ourselves
            ImGui::TableNextRow(ctx, ImGuiTableRowFlags_Headers);
            for (int column = 0; column < COLUMNS_COUNT; column++)
            {
                ImGui::TableSetColumnIndex(ctx, column);
                const char* column_name = ImGui::TableGetColumnName(ctx, column); // Retrieve name passed to TableSetupColumn()
                ImGui::PushID(ctx, column);
                ImGui::PushStyleVar(ctx, ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                ImGui::Checkbox(ctx, "##checkall", &column_selected[column]);
                ImGui::PopStyleVar(ctx);
                ImGui::SameLine(ctx, 0.0f, ImGui::GetStyle(ctx).ItemInnerSpacing.x);
                ImGui::TableHeader(ctx, column_name);
                ImGui::PopID(ctx);
            }

            for (int row = 0; row < 5; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < 3; column++)
                {
                    char buf[32];
                    sprintf(buf, "Cell %d,%d", column, row);
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Selectable(ctx, buf, column_selected[column]);
                }
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    // Demonstrate using ImGuiTableColumnFlags_AngledHeader flag to create angled headers
    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Angled headers");
    if (ImGui::TreeNode(ctx, "Angled headers"))
    {
        const char* column_names[] = { "Track", "cabasa", "ride", "smash", "tom-hi", "tom-mid", "tom-low", "hihat-o", "hihat-c", "snare-s", "snare-c", "clap", "rim", "kick" };
        const int columns_count = IM_ARRAYSIZE(column_names);
        const int rows_count = 12;

        static ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Hideable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_HighlightHoveredColumn;
        static bool bools[columns_count * rows_count] = {}; // Dummy storage selection storage
        static int frozen_cols = 1;
        static int frozen_rows = 2;
        ImGui::CheckboxFlags(ctx, "_ScrollX", &table_flags, ImGuiTableFlags_ScrollX);
        ImGui::CheckboxFlags(ctx, "_ScrollY", &table_flags, ImGuiTableFlags_ScrollY);
        ImGui::CheckboxFlags(ctx, "_NoBordersInBody", &table_flags, ImGuiTableFlags_NoBordersInBody);
        ImGui::CheckboxFlags(ctx, "_HighlightHoveredColumn", &table_flags, ImGuiTableFlags_HighlightHoveredColumn);
        ImGui::SetNextItemWidth(ctx, ImGui::GetFontSize(ctx) * 8);
        ImGui::SliderInt(ctx, "Frozen columns", &frozen_cols, 0, 2);
        ImGui::SetNextItemWidth(ctx, ImGui::GetFontSize(ctx) * 8);
        ImGui::SliderInt(ctx, "Frozen rows", &frozen_rows, 0, 2);

        if (ImGui::BeginTable(ctx, "table_angled_headers", columns_count, table_flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 12)))
        {
            ImGui::TableSetupColumn(ctx, column_names[0], ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoReorder);
            for (int n = 1; n < columns_count; n++)
                ImGui::TableSetupColumn(ctx, column_names[n], ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupScrollFreeze(ctx, frozen_cols, frozen_rows);

            ImGui::TableAngledHeadersRow(ctx); // Draw angled headers for all columns with the ImGuiTableColumnFlags_AngledHeader flag.
            ImGui::TableHeadersRow(ctx);       // Draw remaining headers and allow access to context-menu and other functions.
            for (int row = 0; row < rows_count; row++)
            {
                ImGui::PushID(ctx, row);
                ImGui::TableNextRow(ctx);
                ImGui::TableSetColumnIndex(ctx, 0);
                ImGui::AlignTextToFramePadding(ctx);
                ImGui::Text(ctx, "Track %d", row);
                for (int column = 1; column < columns_count; column++)
                    if (ImGui::TableSetColumnIndex(ctx, column))
                    {
                        ImGui::PushID(ctx, column);
                        ImGui::Checkbox(ctx, "", &bools[row * columns_count + column]);
                        ImGui::PopID(ctx);
                    }
                ImGui::PopID(ctx);
            }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    // Demonstrate creating custom context menus inside columns, while playing it nice with context menus provided by TableHeadersRow()/TableHeader()
    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Context menus");
    if (ImGui::TreeNode(ctx, "Context menus"))
    {
        HelpMarker(ctx, "By default, right-clicking over a TableHeadersRow()/TableHeader() line will open the default context-menu.\nUsing ImGuiTableFlags_ContextMenuInBody we also allow right-clicking over columns body.");
        static ImGuiTableFlags flags1 = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders | ImGuiTableFlags_ContextMenuInBody;

        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_ContextMenuInBody", &flags1, ImGuiTableFlags_ContextMenuInBody);
        PopStyleCompact(ctx);

        // Context Menus: first example
        // [1.1] Right-click on the TableHeadersRow() line to open the default table context menu.
        // [1.2] Right-click in columns also open the default table context menu (if ImGuiTableFlags_ContextMenuInBody is set)
        const int COLUMNS_COUNT = 3;
        if (ImGui::BeginTable(ctx, "table_context_menu", COLUMNS_COUNT, flags1))
        {
            ImGui::TableSetupColumn(ctx, "One");
            ImGui::TableSetupColumn(ctx, "Two");
            ImGui::TableSetupColumn(ctx, "Three");

            // [1.1]] Right-click on the TableHeadersRow() line to open the default table context menu.
            ImGui::TableHeadersRow(ctx);

            // Submit dummy contents
            for (int row = 0; row < 4; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < COLUMNS_COUNT; column++)
                {
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Text(ctx, "Cell %d,%d", column, row);
                }
            }
            ImGui::EndTable(ctx);
        }

        // Context Menus: second example
        // [2.1] Right-click on the TableHeadersRow() line to open the default table context menu.
        // [2.2] Right-click on the ".." to open a custom popup
        // [2.3] Right-click in columns to open another custom popup
        HelpMarker(ctx, "Demonstrate mixing table context menu (over header), item context button (over button) and custom per-colum context menu (over column body).");
        ImGuiTableFlags flags2 = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders;
        if (ImGui::BeginTable(ctx, "table_context_menu_2", COLUMNS_COUNT, flags2))
        {
            ImGui::TableSetupColumn(ctx, "One");
            ImGui::TableSetupColumn(ctx, "Two");
            ImGui::TableSetupColumn(ctx, "Three");

            // [2.1] Right-click on the TableHeadersRow() line to open the default table context menu.
            ImGui::TableHeadersRow(ctx);
            for (int row = 0; row < 4; row++)
            {
                ImGui::TableNextRow(ctx);
                for (int column = 0; column < COLUMNS_COUNT; column++)
                {
                    // Submit dummy contents
                    ImGui::TableSetColumnIndex(ctx, column);
                    ImGui::Text(ctx, "Cell %d,%d", column, row);
                    ImGui::SameLine(ctx);

                    // [2.2] Right-click on the ".." to open a custom popup
                    ImGui::PushID(ctx, row * COLUMNS_COUNT + column);
                    ImGui::SmallButton(ctx, "..");
                    if (ImGui::BeginPopupContextItem(ctx))
                    {
                        ImGui::Text(ctx, "This is the popup for Button(\"..\") in Cell %d,%d", column, row);
                        if (ImGui::Button(ctx, "Close"))
                            ImGui::CloseCurrentPopup(ctx);
                        ImGui::EndPopup(ctx);
                    }
                    ImGui::PopID(ctx);
                }
            }

            // [2.3] Right-click anywhere in columns to open another custom popup
            // (instead of testing for !IsAnyItemHovered() we could also call OpenPopup() with ImGuiPopupFlags_NoOpenOverExistingPopup
            // to manage popup priority as the popups triggers, here "are we hovering a column" are overlapping)
            int hovered_column = -1;
            for (int column = 0; column < COLUMNS_COUNT + 1; column++)
            {
                ImGui::PushID(ctx, column);
                if (ImGui::TableGetColumnFlags(ctx, column) & ImGuiTableColumnFlags_IsHovered)
                    hovered_column = column;
                if (hovered_column == column && !ImGui::IsAnyItemHovered(ctx) && ImGui::IsMouseReleased(ctx, 1))
                    ImGui::OpenPopup(ctx, "MyPopup");
                if (ImGui::BeginPopup(ctx, "MyPopup"))
                {
                    if (column == COLUMNS_COUNT)
                        ImGui::Text(ctx, "This is a custom popup for unused space after the last column.");
                    else
                        ImGui::Text(ctx, "This is a custom popup for Column %d", column);
                    if (ImGui::Button(ctx, "Close"))
                        ImGui::CloseCurrentPopup(ctx);
                    ImGui::EndPopup(ctx);
                }
                ImGui::PopID(ctx);
            }

            ImGui::EndTable(ctx);
            ImGui::Text(ctx, "Hovered column: %d", hovered_column);
        }
        ImGui::TreePop(ctx);
    }

    // Demonstrate creating multiple tables with the same ID
    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Synced instances");
    if (ImGui::TreeNode(ctx, "Synced instances"))
    {
        HelpMarker(ctx, "Multiple tables with the same identifier will share their settings, width, visibility, order etc.");

        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings;
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_ScrollY", &flags, ImGuiTableFlags_ScrollY);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_SizingFixedFit", &flags, ImGuiTableFlags_SizingFixedFit);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_HighlightHoveredColumn", &flags, ImGuiTableFlags_HighlightHoveredColumn);
        for (int n = 0; n < 3; n++)
        {
            char buf[32];
            sprintf(buf, "Synced Table %d", n);
            bool open = ImGui::CollapsingHeader(ctx, buf, ImGuiTreeNodeFlags_DefaultOpen);
            if (open && ImGui::BeginTable(ctx, "Table", 3, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing(ctx) * 5)))
            {
                ImGui::TableSetupColumn(ctx, "One");
                ImGui::TableSetupColumn(ctx, "Two");
                ImGui::TableSetupColumn(ctx, "Three");
                ImGui::TableHeadersRow(ctx);
                const int cell_count = (n == 1) ? 27 : 9; // Make second table have a scrollbar to verify that additional decoration is not affecting column positions.
                for (int cell = 0; cell < cell_count; cell++)
                {
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "this cell %d", cell);
                }
                ImGui::EndTable(ctx);
            }
        }
        ImGui::TreePop(ctx);
    }

    // Demonstrate using Sorting facilities
    // This is a simplified version of the "Advanced" example, where we mostly focus on the code necessary to handle sorting.
    // Note that the "Advanced" example also showcase manually triggering a sort (e.g. if item quantities have been modified)
    static const char* template_items_names[] =
    {
        "Banana", "Apple", "Cherry", "Watermelon", "Grapefruit", "Strawberry", "Mango",
        "Kiwi", "Orange", "Pineapple", "Blueberry", "Plum", "Coconut", "Pear", "Apricot"
    };
    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Sorting");
    if (ImGui::TreeNode(ctx, "Sorting"))
    {
        // Create item list
        static ImVector<MyItem> items;
        if (items.Size == 0)
        {
            items.resize(50, MyItem());
            for (int n = 0; n < items.Size; n++)
            {
                const int template_n = n % IM_ARRAYSIZE(template_items_names);
                MyItem& item = items[n];
                item.ID = n;
                item.Name = template_items_names[template_n];
                item.Quantity = (n * n - n) % 20; // Assign default quantities
            }
        }

        // Options
        static ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
            | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollY;
        PushStyleCompact(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_SortMulti", &flags, ImGuiTableFlags_SortMulti);
        ImGui::SameLine(ctx); HelpMarker(ctx, "When sorting is enabled: hold shift when clicking headers to sort on multiple column. TableGetSortSpecs() may return specs where (SpecsCount > 1).");
        ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_SortTristate", &flags, ImGuiTableFlags_SortTristate);
        ImGui::SameLine(ctx); HelpMarker(ctx, "When sorting is enabled: allow no sorting, disable default sorting. TableGetSortSpecs() may return specs where (SpecsCount == 0).");
        PopStyleCompact(ctx);

        if (ImGui::BeginTable(ctx, "table_sorting", 4, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 15), 0.0f))
        {
            // Declare columns
            // We use the "user_id" parameter of TableSetupColumn() to specify a user id that will be stored in the sort specifications.
            // This is so our sort function can identify a column given our own identifier. We could also identify them based on their index!
            // Demonstrate using a mixture of flags among available sort-related flags:
            // - ImGuiTableColumnFlags_DefaultSort
            // - ImGuiTableColumnFlags_NoSort / ImGuiTableColumnFlags_NoSortAscending / ImGuiTableColumnFlags_NoSortDescending
            // - ImGuiTableColumnFlags_PreferSortAscending / ImGuiTableColumnFlags_PreferSortDescending
            ImGui::TableSetupColumn(ctx, "ID",       ImGuiTableColumnFlags_DefaultSort          | ImGuiTableColumnFlags_WidthFixed,   0.0f, MyItemColumnID_ID);
            ImGui::TableSetupColumn(ctx, "Name",                                                  ImGuiTableColumnFlags_WidthFixed,   0.0f, MyItemColumnID_Name);
            ImGui::TableSetupColumn(ctx, "Action",   ImGuiTableColumnFlags_NoSort               | ImGuiTableColumnFlags_WidthFixed,   0.0f, MyItemColumnID_Action);
            ImGui::TableSetupColumn(ctx, "Quantity", ImGuiTableColumnFlags_PreferSortDescending | ImGuiTableColumnFlags_WidthStretch, 0.0f, MyItemColumnID_Quantity);
            ImGui::TableSetupScrollFreeze(ctx, 0, 1); // Make row always visible
            ImGui::TableHeadersRow(ctx);

            // Sort our data if sort specs have been changed!
            if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs(ctx))
                if (sort_specs->SpecsDirty)
                {
                    MyItem::SortWithSortSpecs(sort_specs, items.Data, items.Size);
                    sort_specs->SpecsDirty = false;
                }

            // Demonstrate using clipper for large vertical lists
            ImGuiListClipper clipper;
            clipper.Begin(items.Size);
            while (clipper.Step())
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                {
                    // Display a data item
                    MyItem* item = &items[row_n];
                    ImGui::PushID(ctx, item->ID);
                    ImGui::TableNextRow(ctx);
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "%04d", item->ID);
                    ImGui::TableNextColumn(ctx);
                    ImGui::TextUnformatted(ctx, item->Name);
                    ImGui::TableNextColumn(ctx);
                    ImGui::SmallButton(ctx, "None");
                    ImGui::TableNextColumn(ctx);
                    ImGui::Text(ctx, "%d", item->Quantity);
                    ImGui::PopID(ctx);
                }
            ImGui::EndTable(ctx);
        }
        ImGui::TreePop(ctx);
    }

    // In this example we'll expose most table flags and settings.
    // For specific flags and settings refer to the corresponding section for more detailed explanation.
    // This section is mostly useful to experiment with combining certain flags or settings with each others.
    //ImGui::SetNextItemOpen(true, ImGuiCond_Once); // [DEBUG]
    if (open_action != -1)
        ImGui::SetNextItemOpen(ctx, open_action != 0);
    IMGUI_DEMO_MARKER("Tables/Advanced");
    if (ImGui::TreeNode(ctx, "Advanced"))
    {
        static ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
            | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
            | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY
            | ImGuiTableFlags_SizingFixedFit;
        static ImGuiTableColumnFlags columns_base_flags = ImGuiTableColumnFlags_None;

        enum ContentsType { CT_Text, CT_Button, CT_SmallButton, CT_FillButton, CT_Selectable, CT_SelectableSpanRow };
        static int contents_type = CT_SelectableSpanRow;
        const char* contents_type_names[] = { "Text", "Button", "SmallButton", "FillButton", "Selectable", "Selectable (span row)" };
        static int freeze_cols = 1;
        static int freeze_rows = 1;
        static int items_count = IM_ARRAYSIZE(template_items_names) * 2;
        static ImVec2 outer_size_value = ImVec2(0.0f, TEXT_BASE_HEIGHT * 12);
        static float row_min_height = 0.0f; // Auto
        static float inner_width_with_scroll = 0.0f; // Auto-extend
        static bool outer_size_enabled = true;
        static bool show_headers = true;
        static bool show_wrapped_text = false;
        //static ImGuiTextFilter filter;
        //ImGui::SetNextItemOpen(true, ImGuiCond_Once); // FIXME-TABLE: Enabling this results in initial clipped first pass on table which tend to affect column sizing
        if (ImGui::TreeNode(ctx, "Options"))
        {
            // Make the UI compact because there are so many fields
            PushStyleCompact(ctx);
            ImGui::PushItemWidth(ctx, TEXT_BASE_WIDTH * 28.0f);

            if (ImGui::TreeNodeEx(ctx, "Features:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Resizable", &flags, ImGuiTableFlags_Resizable);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Reorderable", &flags, ImGuiTableFlags_Reorderable);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Hideable", &flags, ImGuiTableFlags_Hideable);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_Sortable", &flags, ImGuiTableFlags_Sortable);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoSavedSettings", &flags, ImGuiTableFlags_NoSavedSettings);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_ContextMenuInBody", &flags, ImGuiTableFlags_ContextMenuInBody);
                ImGui::TreePop(ctx);
            }

            if (ImGui::TreeNodeEx(ctx, "Decorations:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_RowBg", &flags, ImGuiTableFlags_RowBg);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersV", &flags, ImGuiTableFlags_BordersV);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersOuterV", &flags, ImGuiTableFlags_BordersOuterV);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersInnerV", &flags, ImGuiTableFlags_BordersInnerV);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersH", &flags, ImGuiTableFlags_BordersH);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersOuterH", &flags, ImGuiTableFlags_BordersOuterH);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_BordersInnerH", &flags, ImGuiTableFlags_BordersInnerH);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoBordersInBody", &flags, ImGuiTableFlags_NoBordersInBody); ImGui::SameLine(ctx); HelpMarker(ctx, "Disable vertical borders in columns Body (borders will always appear in Headers");
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoBordersInBodyUntilResize", &flags, ImGuiTableFlags_NoBordersInBodyUntilResize); ImGui::SameLine(ctx); HelpMarker(ctx, "Disable vertical borders in columns Body until hovered for resize (borders will always appear in Headers)");
                ImGui::TreePop(ctx);
            }

            if (ImGui::TreeNodeEx(ctx, "Sizing:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                EditTableSizingFlags(ctx, &flags);
                ImGui::SameLine(ctx); HelpMarker(ctx, "In the Advanced demo we override the policy of each column so those table-wide settings have less effect that typical.");
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoHostExtendX", &flags, ImGuiTableFlags_NoHostExtendX);
                ImGui::SameLine(ctx); HelpMarker(ctx, "Make outer width auto-fit to columns, overriding outer_size.x value.\n\nOnly available when ScrollX/ScrollY are disabled and Stretch columns are not used.");
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoHostExtendY", &flags, ImGuiTableFlags_NoHostExtendY);
                ImGui::SameLine(ctx); HelpMarker(ctx, "Make outer height stop exactly at outer_size.y (prevent auto-extending table past the limit).\n\nOnly available when ScrollX/ScrollY are disabled. Data below the limit will be clipped and not visible.");
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoKeepColumnsVisible", &flags, ImGuiTableFlags_NoKeepColumnsVisible);
                ImGui::SameLine(ctx); HelpMarker(ctx, "Only available if ScrollX is disabled.");
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_PreciseWidths", &flags, ImGuiTableFlags_PreciseWidths);
                ImGui::SameLine(ctx); HelpMarker(ctx, "Disable distributing remainder width to stretched columns (width allocation on a 100-wide table with 3 columns: Without this flag: 33,33,34. With this flag: 33,33,33). With larger number of columns, resizing will appear to be less smooth.");
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoClip", &flags, ImGuiTableFlags_NoClip);
                ImGui::SameLine(ctx); HelpMarker(ctx, "Disable clipping rectangle for every individual columns (reduce draw command count, items will be able to overflow into other columns). Generally incompatible with ScrollFreeze options.");
                ImGui::TreePop(ctx);
            }

            if (ImGui::TreeNodeEx(ctx, "Padding:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_PadOuterX", &flags, ImGuiTableFlags_PadOuterX);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoPadOuterX", &flags, ImGuiTableFlags_NoPadOuterX);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_NoPadInnerX", &flags, ImGuiTableFlags_NoPadInnerX);
                ImGui::TreePop(ctx);
            }

            if (ImGui::TreeNodeEx(ctx, "Scrolling:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_ScrollX", &flags, ImGuiTableFlags_ScrollX);
                ImGui::SameLine(ctx);
                ImGui::SetNextItemWidth(ctx, ImGui::GetFrameHeight(ctx));
                ImGui::DragInt(ctx, "freeze_cols", &freeze_cols, 0.2f, 0, 9, NULL, ImGuiSliderFlags_NoInput);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_ScrollY", &flags, ImGuiTableFlags_ScrollY);
                ImGui::SameLine(ctx);
                ImGui::SetNextItemWidth(ctx, ImGui::GetFrameHeight(ctx));
                ImGui::DragInt(ctx, "freeze_rows", &freeze_rows, 0.2f, 0, 9, NULL, ImGuiSliderFlags_NoInput);
                ImGui::TreePop(ctx);
            }

            if (ImGui::TreeNodeEx(ctx, "Sorting:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_SortMulti", &flags, ImGuiTableFlags_SortMulti);
                ImGui::SameLine(ctx); HelpMarker(ctx, "When sorting is enabled: hold shift when clicking headers to sort on multiple column. TableGetSortSpecs() may return specs where (SpecsCount > 1).");
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_SortTristate", &flags, ImGuiTableFlags_SortTristate);
                ImGui::SameLine(ctx); HelpMarker(ctx, "When sorting is enabled: allow no sorting, disable default sorting. TableGetSortSpecs() may return specs where (SpecsCount == 0).");
                ImGui::TreePop(ctx);
            }

            if (ImGui::TreeNodeEx(ctx, "Headers:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox(ctx, "show_headers", &show_headers);
                ImGui::CheckboxFlags(ctx, "ImGuiTableFlags_HighlightHoveredColumn", &flags, ImGuiTableFlags_HighlightHoveredColumn);
                ImGui::CheckboxFlags(ctx, "ImGuiTableColumnFlags_AngledHeader", &columns_base_flags, ImGuiTableColumnFlags_AngledHeader);
                ImGui::SameLine(ctx); HelpMarker(ctx, "Enable AngledHeader on all columns. Best enabled on selected narrow columns (see \"Angled headers\" section of the demo).");
                ImGui::TreePop(ctx);
            }

            if (ImGui::TreeNodeEx(ctx, "Other:", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox(ctx, "show_wrapped_text", &show_wrapped_text);

                ImGui::DragFloat2(ctx, "##OuterSize", &outer_size_value.x);
                ImGui::SameLine(ctx, 0.0f, ImGui::GetStyle(ctx).ItemInnerSpacing.x);
                ImGui::Checkbox(ctx, "outer_size", &outer_size_enabled);
                ImGui::SameLine(ctx);
                HelpMarker(ctx, "If scrolling is disabled (ScrollX and ScrollY not set):\n"
                    "- The table is output directly in the parent window.\n"
                    "- OuterSize.x < 0.0f will right-align the table.\n"
                    "- OuterSize.x = 0.0f will narrow fit the table unless there are any Stretch columns.\n"
                    "- OuterSize.y then becomes the minimum size for the table, which will extend vertically if there are more rows (unless NoHostExtendY is set).");

                // From a user point of view we will tend to use 'inner_width' differently depending on whether our table is embedding scrolling.
                // To facilitate toying with this demo we will actually pass 0.0f to the BeginTable() when ScrollX is disabled.
                ImGui::DragFloat(ctx, "inner_width (when ScrollX active)", &inner_width_with_scroll, 1.0f, 0.0f, FLT_MAX);

                ImGui::DragFloat(ctx, "row_min_height", &row_min_height, 1.0f, 0.0f, FLT_MAX);
                ImGui::SameLine(ctx); HelpMarker(ctx, "Specify height of the Selectable item.");

                ImGui::DragInt(ctx, "items_count", &items_count, 0.1f, 0, 9999);
                ImGui::Combo(ctx, "items_type (first column)", &contents_type, contents_type_names, IM_ARRAYSIZE(contents_type_names));
                //filter.Draw("filter");
                ImGui::TreePop(ctx);
            }

            ImGui::PopItemWidth(ctx);
            PopStyleCompact(ctx);
            ImGui::Spacing(ctx);
            ImGui::TreePop(ctx);
        }

        // Update item list if we changed the number of items
        static ImVector<MyItem> items;
        static ImVector<int> selection;
        static bool items_need_sort = false;
        if (items.Size != items_count)
        {
            items.resize(items_count, MyItem());
            for (int n = 0; n < items_count; n++)
            {
                const int template_n = n % IM_ARRAYSIZE(template_items_names);
                MyItem& item = items[n];
                item.ID = n;
                item.Name = template_items_names[template_n];
                item.Quantity = (template_n == 3) ? 10 : (template_n == 4) ? 20 : 0; // Assign default quantities
            }
        }

        const ImDrawList* parent_draw_list = ImGui::GetWindowDrawList(ctx);
        const int parent_draw_list_draw_cmd_count = parent_draw_list->CmdBuffer.Size;
        ImVec2 table_scroll_cur, table_scroll_max; // For debug display
        const ImDrawList* table_draw_list = NULL;  // "

        // Submit table
        const float inner_width_to_use = (flags & ImGuiTableFlags_ScrollX) ? inner_width_with_scroll : 0.0f;
        if (ImGui::BeginTable(ctx, "table_advanced", 6, flags, outer_size_enabled ? outer_size_value : ImVec2(0, 0), inner_width_to_use))
        {
            // Declare columns
            // We use the "user_id" parameter of TableSetupColumn() to specify a user id that will be stored in the sort specifications.
            // This is so our sort function can identify a column given our own identifier. We could also identify them based on their index!
            ImGui::TableSetupColumn(ctx, "ID",           columns_base_flags | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, MyItemColumnID_ID);
            ImGui::TableSetupColumn(ctx, "Name",         columns_base_flags | ImGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Name);
            ImGui::TableSetupColumn(ctx, "Action",       columns_base_flags | ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Action);
            ImGui::TableSetupColumn(ctx, "Quantity",     columns_base_flags | ImGuiTableColumnFlags_PreferSortDescending, 0.0f, MyItemColumnID_Quantity);
            ImGui::TableSetupColumn(ctx, "Description",  columns_base_flags | ((flags & ImGuiTableFlags_NoHostExtendX) ? 0 : ImGuiTableColumnFlags_WidthStretch), 0.0f, MyItemColumnID_Description);
            ImGui::TableSetupColumn(ctx, "Hidden",       columns_base_flags |  ImGuiTableColumnFlags_DefaultHide | ImGuiTableColumnFlags_NoSort);
            ImGui::TableSetupScrollFreeze(ctx, freeze_cols, freeze_rows);

            // Sort our data if sort specs have been changed!
            ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs(ctx);
            if (sort_specs && sort_specs->SpecsDirty)
                items_need_sort = true;
            if (sort_specs && items_need_sort && items.Size > 1)
            {
                MyItem::SortWithSortSpecs(sort_specs, items.Data, items.Size);
                sort_specs->SpecsDirty = false;
            }
            items_need_sort = false;

            // Take note of whether we are currently sorting based on the Quantity field,
            // we will use this to trigger sorting when we know the data of this column has been modified.
            const bool sorts_specs_using_quantity = (ImGui::TableGetColumnFlags(ctx, 3) & ImGuiTableColumnFlags_IsSorted) != 0;

            // Show headers
            if (show_headers && (columns_base_flags & ImGuiTableColumnFlags_AngledHeader) != 0)
                ImGui::TableAngledHeadersRow(ctx);
            if (show_headers)
                ImGui::TableHeadersRow(ctx);

            // Show data
            // FIXME-TABLE FIXME-NAV: How we can get decent up/down even though we have the buttons here?
            ImGui::PushButtonRepeat(ctx, true);
#if 1
            // Demonstrate using clipper for large vertical lists
            ImGuiListClipper clipper;
            clipper.Begin(items.Size);
            while (clipper.Step())
            {
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
#else
            // Without clipper
            {
                for (int row_n = 0; row_n < items.Size; row_n++)
#endif
                {
                    MyItem* item = &items[row_n];
                    //if (!filter.PassFilter(item->Name))
                    //    continue;

                    const bool item_is_selected = selection.contains(item->ID);
                    ImGui::PushID(ctx, item->ID);
                    ImGui::TableNextRow(ctx, ImGuiTableRowFlags_None, row_min_height);

                    // For the demo purpose we can select among different type of items submitted in the first column
                    ImGui::TableSetColumnIndex(ctx, 0);
                    char label[32];
                    sprintf(label, "%04d", item->ID);
                    if (contents_type == CT_Text)
                        ImGui::TextUnformatted(ctx, label);
                    else if (contents_type == CT_Button)
                        ImGui::Button(ctx, label);
                    else if (contents_type == CT_SmallButton)
                        ImGui::SmallButton(ctx, label);
                    else if (contents_type == CT_FillButton)
                        ImGui::Button(ctx, label, ImVec2(-FLT_MIN, 0.0f));
                    else if (contents_type == CT_Selectable || contents_type == CT_SelectableSpanRow)
                    {
                        ImGuiSelectableFlags selectable_flags = (contents_type == CT_SelectableSpanRow) ? ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap : ImGuiSelectableFlags_None;
                        if (ImGui::Selectable(ctx, label, item_is_selected, selectable_flags, ImVec2(0, row_min_height)))
                        {
                            if (ImGui::GetIO(ctx).KeyCtrl)
                            {
                                if (item_is_selected)
                                    selection.find_erase_unsorted(item->ID);
                                else
                                    selection.push_back(item->ID);
                            }
                            else
                            {
                                selection.clear();
                                selection.push_back(item->ID);
                            }
                        }
                    }

                    if (ImGui::TableSetColumnIndex(ctx, 1))
                        ImGui::TextUnformatted(ctx, item->Name);

                    // Here we demonstrate marking our data set as needing to be sorted again if we modified a quantity,
                    // and we are currently sorting on the column showing the Quantity.
                    // To avoid triggering a sort while holding the button, we only trigger it when the button has been released.
                    // You will probably need a more advanced system in your code if you want to automatically sort when a specific entry changes.
                    if (ImGui::TableSetColumnIndex(ctx, 2))
                    {
                        if (ImGui::SmallButton(ctx, "Chop")) { item->Quantity += 1; }
                        if (sorts_specs_using_quantity && ImGui::IsItemDeactivated(ctx)) { items_need_sort = true; }
                        ImGui::SameLine(ctx);
                        if (ImGui::SmallButton(ctx, "Eat")) { item->Quantity -= 1; }
                        if (sorts_specs_using_quantity && ImGui::IsItemDeactivated(ctx)) { items_need_sort = true; }
                    }

                    if (ImGui::TableSetColumnIndex(ctx, 3))
                        ImGui::Text(ctx, "%d", item->Quantity);

                    ImGui::TableSetColumnIndex(ctx, 4);
                    if (show_wrapped_text)
                        ImGui::TextWrapped(ctx, "Lorem ipsum dolor sit amet");
                    else
                        ImGui::Text(ctx, "Lorem ipsum dolor sit amet");

                    if (ImGui::TableSetColumnIndex(ctx, 5))
                        ImGui::Text(ctx, "1234");

                    ImGui::PopID(ctx);
                }
            }
            ImGui::PopButtonRepeat(ctx);

            // Store some info to display debug details below
            table_scroll_cur = ImVec2(ImGui::GetScrollX(ctx), ImGui::GetScrollY(ctx));
            table_scroll_max = ImVec2(ImGui::GetScrollMaxX(ctx), ImGui::GetScrollMaxY(ctx));
            table_draw_list = ImGui::GetWindowDrawList(ctx);
            ImGui::EndTable(ctx);
        }
        static bool show_debug_details = false;
        ImGui::Checkbox(ctx, "Debug details", &show_debug_details);
        if (show_debug_details && table_draw_list)
        {
            ImGui::SameLine(ctx, 0.0f, 0.0f);
            const int table_draw_list_draw_cmd_count = table_draw_list->CmdBuffer.Size;
            if (table_draw_list == parent_draw_list)
                ImGui::Text(ctx, ": DrawCmd: +%d (in same window)",
                    table_draw_list_draw_cmd_count - parent_draw_list_draw_cmd_count);
            else
                ImGui::Text(ctx, ": DrawCmd: +%d (in child window), Scroll: (%.f/%.f) (%.f/%.f)",
                    table_draw_list_draw_cmd_count - 1, table_scroll_cur.x, table_scroll_max.x, table_scroll_cur.y, table_scroll_max.y);
        }
        ImGui::TreePop(ctx);
    }

    ImGui::PopID(ctx);

    ShowDemoWindowColumns(ctx);

    if (disable_indent)
        ImGui::PopStyleVar(ctx);
}

// Demonstrate old/legacy Columns API!
// [2020: Columns are under-featured and not maintained. Prefer using the more flexible and powerful BeginTable() API!]
static void ShowDemoWindowColumns(ImGuiContext* ctx)
{
    IMGUI_DEMO_MARKER("Columns (legacy API)");
    bool open = ImGui::TreeNode(ctx, "Legacy Columns API");
    ImGui::SameLine(ctx);
    HelpMarker(ctx, "Columns() is an old API! Prefer using the more flexible and powerful BeginTable() API!");
    if (!open)
        return;

    // Basic columns
    IMGUI_DEMO_MARKER("Columns (legacy API)/Basic");
    if (ImGui::TreeNode(ctx, "Basic"))
    {
        ImGui::Text(ctx, "Without border:");
        ImGui::Columns(ctx, 3, "mycolumns3", false);  // 3-ways, no border
        ImGui::Separator(ctx);
        for (int n = 0; n < 14; n++)
        {
            char label[32];
            sprintf(label, "Item %d", n);
            if (ImGui::Selectable(ctx, label)) {}
            //if (ImGui::Button(label, ImVec2(-FLT_MIN,0.0f))) {}
            ImGui::NextColumn(ctx);
        }
        ImGui::Columns(ctx, 1);
        ImGui::Separator(ctx);

        ImGui::Text(ctx, "With border:");
        ImGui::Columns(ctx, 4, "mycolumns"); // 4-ways, with border
        ImGui::Separator(ctx);
        ImGui::Text(ctx, "ID"); ImGui::NextColumn(ctx);
        ImGui::Text(ctx, "Name"); ImGui::NextColumn(ctx);
        ImGui::Text(ctx, "Path"); ImGui::NextColumn(ctx);
        ImGui::Text(ctx, "Hovered"); ImGui::NextColumn(ctx);
        ImGui::Separator(ctx);
        const char* names[3] = { "One", "Two", "Three" };
        const char* paths[3] = { "/path/one", "/path/two", "/path/three" };
        static int selected = -1;
        for (int i = 0; i < 3; i++)
        {
            char label[32];
            sprintf(label, "%04d", i);
            if (ImGui::Selectable(ctx, label, selected == i, ImGuiSelectableFlags_SpanAllColumns))
                selected = i;
            bool hovered = ImGui::IsItemHovered(ctx);
            ImGui::NextColumn(ctx);
            ImGui::Text(ctx, names[i]); ImGui::NextColumn(ctx);
            ImGui::Text(ctx, paths[i]); ImGui::NextColumn(ctx);
            ImGui::Text(ctx, "%d", hovered); ImGui::NextColumn(ctx);
        }
        ImGui::Columns(ctx, 1);
        ImGui::Separator(ctx);
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Columns (legacy API)/Borders");
    if (ImGui::TreeNode(ctx, "Borders"))
    {
        // NB: Future columns API should allow automatic horizontal borders.
        static bool h_borders = true;
        static bool v_borders = true;
        static int columns_count = 4;
        const int lines_count = 3;
        ImGui::SetNextItemWidth(ctx, ImGui::GetFontSize(ctx) * 8);
        ImGui::DragInt(ctx, "##columns_count", &columns_count, 0.1f, 2, 10, "%d columns");
        if (columns_count < 2)
            columns_count = 2;
        ImGui::SameLine(ctx);
        ImGui::Checkbox(ctx, "horizontal", &h_borders);
        ImGui::SameLine(ctx);
        ImGui::Checkbox(ctx, "vertical", &v_borders);
        ImGui::Columns(ctx, columns_count, NULL, v_borders);
        for (int i = 0; i < columns_count * lines_count; i++)
        {
            if (h_borders && ImGui::GetColumnIndex(ctx) == 0)
                ImGui::Separator(ctx);
            ImGui::Text(ctx, "%c%c%c", 'a' + i, 'a' + i, 'a' + i);
            ImGui::Text(ctx, "Width %.2f", ImGui::GetColumnWidth(ctx));
            ImGui::Text(ctx, "Avail %.2f", ImGui::GetContentRegionAvail(ctx).x);
            ImGui::Text(ctx, "Offset %.2f", ImGui::GetColumnOffset(ctx));
            ImGui::Text(ctx, "Long text that is likely to clip");
            ImGui::Button(ctx, "Button", ImVec2(-FLT_MIN, 0.0f));
            ImGui::NextColumn(ctx);
        }
        ImGui::Columns(ctx, 1);
        if (h_borders)
            ImGui::Separator(ctx);
        ImGui::TreePop(ctx);
    }

    // Create multiple items in a same cell before switching to next column
    IMGUI_DEMO_MARKER("Columns (legacy API)/Mixed items");
    if (ImGui::TreeNode(ctx, "Mixed items"))
    {
        ImGui::Columns(ctx, 3, "mixed");
        ImGui::Separator(ctx);

        ImGui::Text(ctx, "Hello");
        ImGui::Button(ctx, "Banana");
        ImGui::NextColumn(ctx);

        ImGui::Text(ctx, "ImGui");
        ImGui::Button(ctx, "Apple");
        static float foo = 1.0f;
        ImGui::InputFloat(ctx, "red", &foo, 0.05f, 0, "%.3f");
        ImGui::Text(ctx, "An extra line here.");
        ImGui::NextColumn(ctx);

        ImGui::Text(ctx, "Sailor");
        ImGui::Button(ctx, "Corniflower");
        static float bar = 1.0f;
        ImGui::InputFloat(ctx, "blue", &bar, 0.05f, 0, "%.3f");
        ImGui::NextColumn(ctx);

        if (ImGui::CollapsingHeader(ctx, "Category A")) { ImGui::Text(ctx, "Blah blah blah"); } ImGui::NextColumn(ctx);
        if (ImGui::CollapsingHeader(ctx, "Category B")) { ImGui::Text(ctx, "Blah blah blah"); } ImGui::NextColumn(ctx);
        if (ImGui::CollapsingHeader(ctx, "Category C")) { ImGui::Text(ctx, "Blah blah blah"); } ImGui::NextColumn(ctx);
        ImGui::Columns(ctx, 1);
        ImGui::Separator(ctx);
        ImGui::TreePop(ctx);
    }

    // Word wrapping
    IMGUI_DEMO_MARKER("Columns (legacy API)/Word-wrapping");
    if (ImGui::TreeNode(ctx, "Word-wrapping"))
    {
        ImGui::Columns(ctx, 2, "word-wrapping");
        ImGui::Separator(ctx);
        ImGui::TextWrapped(ctx, "The quick brown fox jumps over the lazy dog.");
        ImGui::TextWrapped(ctx, "Hello Left");
        ImGui::NextColumn(ctx);
        ImGui::TextWrapped(ctx, "The quick brown fox jumps over the lazy dog.");
        ImGui::TextWrapped(ctx, "Hello Right");
        ImGui::Columns(ctx, 1);
        ImGui::Separator(ctx);
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Columns (legacy API)/Horizontal Scrolling");
    if (ImGui::TreeNode(ctx, "Horizontal Scrolling"))
    {
        ImGui::SetNextWindowContentSize(ctx, ImVec2(1500.0f, 0.0f));
        ImVec2 child_size = ImVec2(0, ImGui::GetFontSize(ctx) * 20.0f);
        ImGui::BeginChild(ctx, "##ScrollingRegion", child_size, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Columns(ctx, 10);

        // Also demonstrate using clipper for large vertical lists
        int ITEMS_COUNT = 2000;
        ImGuiListClipper clipper;
        clipper.Begin(ITEMS_COUNT);
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                for (int j = 0; j < 10; j++)
                {
                    ImGui::Text(ctx, "Line %d Column %d...", i, j);
                    ImGui::NextColumn(ctx);
                }
        }
        ImGui::Columns(ctx, 1);
        ImGui::EndChild(ctx);
        ImGui::TreePop(ctx);
    }

    IMGUI_DEMO_MARKER("Columns (legacy API)/Tree");
    if (ImGui::TreeNode(ctx, "Tree"))
    {
        ImGui::Columns(ctx, 2, "tree", true);
        for (int x = 0; x < 3; x++)
        {
            bool open1 = ImGui::TreeNode(ctx, (void*)(intptr_t)x, "Node%d", x);
            ImGui::NextColumn(ctx);
            ImGui::Text(ctx, "Node contents");
            ImGui::NextColumn(ctx);
            if (open1)
            {
                for (int y = 0; y < 3; y++)
                {
                    bool open2 = ImGui::TreeNode(ctx, (void*)(intptr_t)y, "Node%d.%d", x, y);
                    ImGui::NextColumn(ctx);
                    ImGui::Text(ctx, "Node contents");
                    if (open2)
                    {
                        ImGui::Text(ctx, "Even more contents");
                        if (ImGui::TreeNode(ctx, "Tree in column"))
                        {
                            ImGui::Text(ctx, "The quick brown fox jumps over the lazy dog");
                            ImGui::TreePop(ctx);
                        }
                    }
                    ImGui::NextColumn(ctx);
                    if (open2)
                        ImGui::TreePop(ctx);
                }
                ImGui::TreePop(ctx);
            }
        }
        ImGui::Columns(ctx, 1);
        ImGui::TreePop(ctx);
    }

    ImGui::TreePop(ctx);
}

static void ShowDemoWindowInputs(ImGuiContext* ctx)
{
    IMGUI_DEMO_MARKER("Inputs & Focus");
    if (ImGui::CollapsingHeader(ctx, "Inputs & Focus"))
    {
        ImGuiIO& io = ImGui::GetIO(ctx);

        // Display inputs submitted to ImGuiIO
        IMGUI_DEMO_MARKER("Inputs & Focus/Inputs");
        ImGui::SetNextItemOpen(ctx, true, ImGuiCond_Once);
        if (ImGui::TreeNode(ctx, "Inputs"))
        {
            HelpMarker(ctx, 
                "This is a simplified view. See more detailed input state:\n"
                "- in 'Tools->Metrics/Debugger->Inputs'.\n"
                "- in 'Tools->Debug Log->IO'.");
            if (ImGui::IsMousePosValid(ctx))
                ImGui::Text(ctx, "Mouse pos: (%g, %g)", io.MousePos.x, io.MousePos.y);
            else
                ImGui::Text(ctx, "Mouse pos: <INVALID>");
            ImGui::Text(ctx, "Mouse delta: (%g, %g)", io.MouseDelta.x, io.MouseDelta.y);
            ImGui::Text(ctx, "Mouse down:");
            for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) if (ImGui::IsMouseDown(ctx, i)) { ImGui::SameLine(ctx); ImGui::Text(ctx, "b%d (%.02f secs)", i, io.MouseDownDuration[i]); }
            ImGui::Text(ctx, "Mouse wheel: %.1f", io.MouseWheel);

            // We iterate both legacy native range and named ImGuiKey ranges, which is a little odd but this allows displaying the data for old/new backends.
            // User code should never have to go through such hoops! You can generally iterate between ImGuiKey_NamedKey_BEGIN and ImGuiKey_NamedKey_END.
#ifdef IMGUI_DISABLE_OBSOLETE_KEYIO
            struct funcs { static bool IsLegacyNativeDupe(ImGuiContext*, ImGuiKey) { return false; } };
            ImGuiKey start_key = ImGuiKey_NamedKey_BEGIN;
#else
            struct funcs { static bool IsLegacyNativeDupe(ImGuiContext* ctx, ImGuiKey key) { return key < 512 && ImGui::GetIO(ctx).KeyMap[key] != -1; } }; // Hide Native<>ImGuiKey duplicates when both exists in the array
            ImGuiKey start_key = (ImGuiKey)0;
#endif
            ImGui::Text(ctx, "Keys down:");         for (ImGuiKey key = start_key; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1)) { if (funcs::IsLegacyNativeDupe(ctx, key) || !ImGui::IsKeyDown(ctx, key)) continue; ImGui::SameLine(ctx); ImGui::Text(ctx, (key < ImGuiKey_NamedKey_BEGIN) ? "\"%s\"" : "\"%s\" %d", ImGui::GetKeyName(ctx, key), key); }
            ImGui::Text(ctx, "Keys mods: %s%s%s%s", io.KeyCtrl ? "CTRL " : "", io.KeyShift ? "SHIFT " : "", io.KeyAlt ? "ALT " : "", io.KeySuper ? "SUPER " : "");
            ImGui::Text(ctx, "Chars queue:");       for (int i = 0; i < io.InputQueueCharacters.Size; i++) { ImWchar c = io.InputQueueCharacters[i]; ImGui::SameLine(ctx);  ImGui::Text(ctx, "\'%c\' (0x%04X)", (c > ' ' && c <= 255) ? (char)c : '?', c); } // FIXME: We should convert 'c' to UTF-8 here but the functions are not public.

            ImGui::TreePop(ctx);
        }

        // Display ImGuiIO output flags
        IMGUI_DEMO_MARKER("Inputs & Focus/Outputs");
        ImGui::SetNextItemOpen(ctx, true, ImGuiCond_Once);
        if (ImGui::TreeNode(ctx, "Outputs"))
        {
            HelpMarker(ctx, 
                "The value of io.WantCaptureMouse and io.WantCaptureKeyboard are normally set by Dear ImGui "
                "to instruct your application of how to route inputs. Typically, when a value is true, it means "
                "Dear ImGui wants the corresponding inputs and we expect the underlying application to ignore them.\n\n"
                "The most typical case is: when hovering a window, Dear ImGui set io.WantCaptureMouse to true, "
                "and underlying application should ignore mouse inputs (in practice there are many and more subtle "
                "rules leading to how those flags are set).");
            ImGui::Text(ctx, "io.WantCaptureMouse: %d", io.WantCaptureMouse);
            ImGui::Text(ctx, "io.WantCaptureMouseUnlessPopupClose: %d", io.WantCaptureMouseUnlessPopupClose);
            ImGui::Text(ctx, "io.WantCaptureKeyboard: %d", io.WantCaptureKeyboard);
            ImGui::Text(ctx, "io.WantTextInput: %d", io.WantTextInput);
            ImGui::Text(ctx, "io.WantSetMousePos: %d", io.WantSetMousePos);
            ImGui::Text(ctx, "io.NavActive: %d, io.NavVisible: %d", io.NavActive, io.NavVisible);

            IMGUI_DEMO_MARKER("Inputs & Focus/Outputs/WantCapture override");
            if (ImGui::TreeNode(ctx, "WantCapture override"))
            {
                HelpMarker(ctx, 
                    "Hovering the colored canvas will override io.WantCaptureXXX fields.\n"
                    "Notice how normally (when set to none), the value of io.WantCaptureKeyboard would be false when hovering and true when clicking.");
                static int capture_override_mouse = -1;
                static int capture_override_keyboard = -1;
                const char* capture_override_desc[] = { "None", "Set to false", "Set to true" };
                ImGui::SetNextItemWidth(ctx, ImGui::GetFontSize(ctx) * 15);
                ImGui::SliderInt(ctx, "SetNextFrameWantCaptureMouse() on hover", &capture_override_mouse, -1, +1, capture_override_desc[capture_override_mouse + 1], ImGuiSliderFlags_AlwaysClamp);
                ImGui::SetNextItemWidth(ctx, ImGui::GetFontSize(ctx) * 15);
                ImGui::SliderInt(ctx, "SetNextFrameWantCaptureKeyboard() on hover", &capture_override_keyboard, -1, +1, capture_override_desc[capture_override_keyboard + 1], ImGuiSliderFlags_AlwaysClamp);

                ImGui::ColorButton(ctx, "##panel", ImVec4(0.7f, 0.1f, 0.7f, 1.0f), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(128.0f, 96.0f)); // Dummy item
                if (ImGui::IsItemHovered(ctx) && capture_override_mouse != -1)
                    ImGui::SetNextFrameWantCaptureMouse(ctx, capture_override_mouse == 1);
                if (ImGui::IsItemHovered(ctx) && capture_override_keyboard != -1)
                    ImGui::SetNextFrameWantCaptureKeyboard(ctx, capture_override_keyboard == 1);

                ImGui::TreePop(ctx);
            }
            ImGui::TreePop(ctx);
        }

        // Display mouse cursors
        IMGUI_DEMO_MARKER("Inputs & Focus/Mouse Cursors");
        if (ImGui::TreeNode(ctx, "Mouse Cursors"))
        {
            const char* mouse_cursors_names[] = { "Arrow", "TextInput", "ResizeAll", "ResizeNS", "ResizeEW", "ResizeNESW", "ResizeNWSE", "Hand", "NotAllowed" };
            IM_ASSERT(IM_ARRAYSIZE(mouse_cursors_names) == ImGuiMouseCursor_COUNT);

            ImGuiMouseCursor current = ImGui::GetMouseCursor(ctx);
            ImGui::Text(ctx, "Current mouse cursor = %d: %s", current, mouse_cursors_names[current]);
            ImGui::BeginDisabled(ctx, true);
            ImGui::CheckboxFlags(ctx, "io.BackendFlags: HasMouseCursors", &io.BackendFlags, ImGuiBackendFlags_HasMouseCursors);
            ImGui::EndDisabled(ctx);

            ImGui::Text(ctx, "Hover to see mouse cursors:");
            ImGui::SameLine(ctx); HelpMarker(ctx, 
                "Your application can render a different mouse cursor based on what ImGui::GetMouseCursor() returns. "
                "If software cursor rendering (io.MouseDrawCursor) is set ImGui will draw the right cursor for you, "
                "otherwise your backend needs to handle it.");
            for (int i = 0; i < ImGuiMouseCursor_COUNT; i++)
            {
                char label[32];
                sprintf(label, "Mouse cursor %d: %s", i, mouse_cursors_names[i]);
                ImGui::Bullet(ctx); ImGui::Selectable(ctx, label, false);
                if (ImGui::IsItemHovered(ctx))
                    ImGui::SetMouseCursor(ctx, i);
            }
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Inputs & Focus/Tabbing");
        if (ImGui::TreeNode(ctx, "Tabbing"))
        {
            ImGui::Text(ctx, "Use TAB/SHIFT+TAB to cycle through keyboard editable fields.");
            static char buf[32] = "hello";
            ImGui::InputText(ctx, "1", buf, IM_ARRAYSIZE(buf));
            ImGui::InputText(ctx, "2", buf, IM_ARRAYSIZE(buf));
            ImGui::InputText(ctx, "3", buf, IM_ARRAYSIZE(buf));
            ImGui::PushTabStop(ctx, false);
            ImGui::InputText(ctx, "4 (tab skip)", buf, IM_ARRAYSIZE(buf));
            ImGui::SameLine(ctx); HelpMarker(ctx, "Item won't be cycled through when using TAB or Shift+Tab.");
            ImGui::PopTabStop(ctx);
            ImGui::InputText(ctx, "5", buf, IM_ARRAYSIZE(buf));
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Inputs & Focus/Focus from code");
        if (ImGui::TreeNode(ctx, "Focus from code"))
        {
            bool focus_1 = ImGui::Button(ctx, "Focus on 1"); ImGui::SameLine(ctx);
            bool focus_2 = ImGui::Button(ctx, "Focus on 2"); ImGui::SameLine(ctx);
            bool focus_3 = ImGui::Button(ctx, "Focus on 3");
            int has_focus = 0;
            static char buf[128] = "click on a button to set focus";

            if (focus_1) ImGui::SetKeyboardFocusHere(ctx);
            ImGui::InputText(ctx, "1", buf, IM_ARRAYSIZE(buf));
            if (ImGui::IsItemActive(ctx)) has_focus = 1;

            if (focus_2) ImGui::SetKeyboardFocusHere(ctx);
            ImGui::InputText(ctx, "2", buf, IM_ARRAYSIZE(buf));
            if (ImGui::IsItemActive(ctx)) has_focus = 2;

            ImGui::PushTabStop(ctx, false);
            if (focus_3) ImGui::SetKeyboardFocusHere(ctx);
            ImGui::InputText(ctx, "3 (tab skip)", buf, IM_ARRAYSIZE(buf));
            if (ImGui::IsItemActive(ctx)) has_focus = 3;
            ImGui::SameLine(ctx); HelpMarker(ctx, "Item won't be cycled through when using TAB or Shift+Tab.");
            ImGui::PopTabStop(ctx);

            if (has_focus)
                ImGui::Text(ctx, "Item with focus: %d", has_focus);
            else
                ImGui::Text(ctx, "Item with focus: <none>");

            // Use >= 0 parameter to SetKeyboardFocusHere() to focus an upcoming item
            static float f3[3] = { 0.0f, 0.0f, 0.0f };
            int focus_ahead = -1;
            if (ImGui::Button(ctx, "Focus on X")) { focus_ahead = 0; } ImGui::SameLine(ctx);
            if (ImGui::Button(ctx, "Focus on Y")) { focus_ahead = 1; } ImGui::SameLine(ctx);
            if (ImGui::Button(ctx, "Focus on Z")) { focus_ahead = 2; }
            if (focus_ahead != -1) ImGui::SetKeyboardFocusHere(ctx, focus_ahead);
            ImGui::SliderFloat3(ctx, "Float3", &f3[0], 0.0f, 1.0f);

            ImGui::TextWrapped(ctx, "NB: Cursor & selection are preserved when refocusing last used item in code.");
            ImGui::TreePop(ctx);
        }

        IMGUI_DEMO_MARKER("Inputs & Focus/Dragging");
        if (ImGui::TreeNode(ctx, "Dragging"))
        {
            ImGui::TextWrapped(ctx, "You can use ImGui::GetMouseDragDelta(0) to query for the dragged amount on any widget.");
            for (int button = 0; button < 3; button++)
            {
                ImGui::Text(ctx, "IsMouseDragging(%d):", button);
                ImGui::Text(ctx, "  w/ default threshold: %d,", ImGui::IsMouseDragging(ctx, button));
                ImGui::Text(ctx, "  w/ zero threshold: %d,", ImGui::IsMouseDragging(ctx, button, 0.0f));
                ImGui::Text(ctx, "  w/ large threshold: %d,", ImGui::IsMouseDragging(ctx, button, 20.0f));
            }

            ImGui::Button(ctx, "Drag Me");
            if (ImGui::IsItemActive(ctx))
                ImGui::GetForegroundDrawList(ctx)->AddLine(io.MouseClickedPos[0], io.MousePos, ImGui::GetColorU32(ctx, ImGuiCol_Button), 4.0f); // Draw a line between the button and the mouse cursor

            // Drag operations gets "unlocked" when the mouse has moved past a certain threshold
            // (the default threshold is stored in io.MouseDragThreshold). You can request a lower or higher
            // threshold using the second parameter of IsMouseDragging() and GetMouseDragDelta().
            ImVec2 value_raw = ImGui::GetMouseDragDelta(ctx, 0, 0.0f);
            ImVec2 value_with_lock_threshold = ImGui::GetMouseDragDelta(ctx, 0);
            ImVec2 mouse_delta = io.MouseDelta;
            ImGui::Text(ctx, "GetMouseDragDelta(0):");
            ImGui::Text(ctx, "  w/ default threshold: (%.1f, %.1f)", value_with_lock_threshold.x, value_with_lock_threshold.y);
            ImGui::Text(ctx, "  w/ zero threshold: (%.1f, %.1f)", value_raw.x, value_raw.y);
            ImGui::Text(ctx, "io.MouseDelta: (%.1f, %.1f)", mouse_delta.x, mouse_delta.y);
            ImGui::TreePop(ctx);
        }
    }
}

//-----------------------------------------------------------------------------
// [SECTION] About Window / ShowAboutWindow()
// Access from Dear ImGui Demo -> Tools -> About
//-----------------------------------------------------------------------------

void ImGui::ShowAboutWindow(ImGuiContext* ctx, bool* p_open)
{
    if (!ImGui::Begin(ctx, "About Dear ImGui", p_open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End(ctx);
        return;
    }
    IMGUI_DEMO_MARKER("Tools/About Dear ImGui");
    ImGui::Text(ctx, "Dear ImGui %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
    ImGui::Separator(ctx);
    ImGui::Text(ctx, "By Omar Cornut and all Dear ImGui contributors.");
    ImGui::Text(ctx, "Dear ImGui is licensed under the MIT License, see LICENSE for more information.");
    ImGui::Text(ctx, "If your company uses this, please consider sponsoring the project!");

    static bool show_config_info = false;
    ImGui::Checkbox(ctx, "Config/Build Information", &show_config_info);
    if (show_config_info)
    {
        ImGuiIO& io = ImGui::GetIO(ctx);
        ImGuiStyle& style = ImGui::GetStyle(ctx);

        bool copy_to_clipboard = ImGui::Button(ctx, "Copy to clipboard");
        ImVec2 child_size = ImVec2(0, ImGui::GetTextLineHeightWithSpacing(ctx) * 18);
        ImGui::BeginChild(ctx, ImGui::GetID(ctx, "cfg_infos"), child_size, ImGuiChildFlags_FrameStyle);
        if (copy_to_clipboard)
        {
            ImGui::LogToClipboard(ctx);
            ImGui::LogText(ctx, "```\n"); // Back quotes will make text appears without formatting when pasting on GitHub
        }

        ImGui::Text(ctx, "Dear ImGui %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
        ImGui::Separator(ctx);
        ImGui::Text(ctx, "sizeof(size_t): %d, sizeof(ImDrawIdx): %d, sizeof(ImDrawVert): %d", (int)sizeof(size_t), (int)sizeof(ImDrawIdx), (int)sizeof(ImDrawVert));
        ImGui::Text(ctx, "define: __cplusplus=%d", (int)__cplusplus);
#ifdef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        ImGui::Text(ctx, "define: IMGUI_DISABLE_OBSOLETE_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_OBSOLETE_KEYIO
        ImGui::Text(ctx, "define: IMGUI_DISABLE_OBSOLETE_KEYIO");
#endif
#ifdef IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
        ImGui::Text(ctx, "define: IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
        ImGui::Text(ctx, "define: IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_WIN32_FUNCTIONS
        ImGui::Text(ctx, "define: IMGUI_DISABLE_WIN32_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS
        ImGui::Text(ctx, "define: IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_MATH_FUNCTIONS
        ImGui::Text(ctx, "define: IMGUI_DISABLE_DEFAULT_MATH_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS
        ImGui::Text(ctx, "define: IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_FILE_FUNCTIONS
        ImGui::Text(ctx, "define: IMGUI_DISABLE_FILE_FUNCTIONS");
#endif
#ifdef IMGUI_DISABLE_DEFAULT_ALLOCATORS
        ImGui::Text(ctx, "define: IMGUI_DISABLE_DEFAULT_ALLOCATORS");
#endif
#ifdef IMGUI_USE_BGRA_PACKED_COLOR
        ImGui::Text(ctx, "define: IMGUI_USE_BGRA_PACKED_COLOR");
#endif
#ifdef _WIN32
        ImGui::Text(ctx, "define: _WIN32");
#endif
#ifdef _WIN64
        ImGui::Text(ctx, "define: _WIN64");
#endif
#ifdef __linux__
        ImGui::Text(ctx, "define: __linux__");
#endif
#ifdef __APPLE__
        ImGui::Text(ctx, "define: __APPLE__");
#endif
#ifdef _MSC_VER
        ImGui::Text(ctx, "define: _MSC_VER=%d", _MSC_VER);
#endif
#ifdef _MSVC_LANG
        ImGui::Text(ctx, "define: _MSVC_LANG=%d", (int)_MSVC_LANG);
#endif
#ifdef __MINGW32__
        ImGui::Text(ctx, "define: __MINGW32__");
#endif
#ifdef __MINGW64__
        ImGui::Text(ctx, "define: __MINGW64__");
#endif
#ifdef __GNUC__
        ImGui::Text(ctx, "define: __GNUC__=%d", (int)__GNUC__);
#endif
#ifdef __clang_version__
        ImGui::Text(ctx, "define: __clang_version__=%s", __clang_version__);
#endif
#ifdef __EMSCRIPTEN__
        ImGui::Text(ctx, "define: __EMSCRIPTEN__");
#endif
        ImGui::Separator(ctx);
        ImGui::Text(ctx, "io.BackendPlatformName: %s", io.BackendPlatformName ? io.BackendPlatformName : "NULL");
        ImGui::Text(ctx, "io.BackendRendererName: %s", io.BackendRendererName ? io.BackendRendererName : "NULL");
        ImGui::Text(ctx, "io.ConfigFlags: 0x%08X", io.ConfigFlags);
        if (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard)        ImGui::Text(ctx, " NavEnableKeyboard");
        if (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad)         ImGui::Text(ctx, " NavEnableGamepad");
        if (io.ConfigFlags & ImGuiConfigFlags_NavEnableSetMousePos)     ImGui::Text(ctx, " NavEnableSetMousePos");
        if (io.ConfigFlags & ImGuiConfigFlags_NavNoCaptureKeyboard)     ImGui::Text(ctx, " NavNoCaptureKeyboard");
        if (io.ConfigFlags & ImGuiConfigFlags_NoMouse)                  ImGui::Text(ctx, " NoMouse");
        if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)      ImGui::Text(ctx, " NoMouseCursorChange");
        if (io.MouseDrawCursor)                                         ImGui::Text(ctx, "io.MouseDrawCursor");
        if (io.ConfigMacOSXBehaviors)                                   ImGui::Text(ctx, "io.ConfigMacOSXBehaviors");
        if (io.ConfigInputTextCursorBlink)                              ImGui::Text(ctx, "io.ConfigInputTextCursorBlink");
        if (io.ConfigWindowsResizeFromEdges)                            ImGui::Text(ctx, "io.ConfigWindowsResizeFromEdges");
        if (io.ConfigWindowsMoveFromTitleBarOnly)                       ImGui::Text(ctx, "io.ConfigWindowsMoveFromTitleBarOnly");
        if (io.ConfigMemoryCompactTimer >= 0.0f)                        ImGui::Text(ctx, "io.ConfigMemoryCompactTimer = %.1f", io.ConfigMemoryCompactTimer);
        ImGui::Text(ctx, "io.BackendFlags: 0x%08X", io.BackendFlags);
        if (io.BackendFlags & ImGuiBackendFlags_HasGamepad)             ImGui::Text(ctx, " HasGamepad");
        if (io.BackendFlags & ImGuiBackendFlags_HasMouseCursors)        ImGui::Text(ctx, " HasMouseCursors");
        if (io.BackendFlags & ImGuiBackendFlags_HasSetMousePos)         ImGui::Text(ctx, " HasSetMousePos");
        if (io.BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset)   ImGui::Text(ctx, " RendererHasVtxOffset");
        ImGui::Separator(ctx);
        ImGui::Text(ctx, "io.Fonts: %d fonts, Flags: 0x%08X, TexSize: %d,%d", io.Fonts->Fonts.Size, io.Fonts->Flags, io.Fonts->TexWidth, io.Fonts->TexHeight);
        ImGui::Text(ctx, "io.DisplaySize: %.2f,%.2f", io.DisplaySize.x, io.DisplaySize.y);
        ImGui::Text(ctx, "io.DisplayFramebufferScale: %.2f,%.2f", io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        ImGui::Separator(ctx);
        ImGui::Text(ctx, "style.WindowPadding: %.2f,%.2f", style.WindowPadding.x, style.WindowPadding.y);
        ImGui::Text(ctx, "style.WindowBorderSize: %.2f", style.WindowBorderSize);
        ImGui::Text(ctx, "style.FramePadding: %.2f,%.2f", style.FramePadding.x, style.FramePadding.y);
        ImGui::Text(ctx, "style.FrameRounding: %.2f", style.FrameRounding);
        ImGui::Text(ctx, "style.FrameBorderSize: %.2f", style.FrameBorderSize);
        ImGui::Text(ctx, "style.ItemSpacing: %.2f,%.2f", style.ItemSpacing.x, style.ItemSpacing.y);
        ImGui::Text(ctx, "style.ItemInnerSpacing: %.2f,%.2f", style.ItemInnerSpacing.x, style.ItemInnerSpacing.y);

        if (copy_to_clipboard)
        {
            ImGui::LogText(ctx, "\n```\n");
            ImGui::LogFinish(ctx);
        }
        ImGui::EndChild(ctx);
    }
    ImGui::End(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] Style Editor / ShowStyleEditor()
//-----------------------------------------------------------------------------
// - ShowFontSelector()
// - ShowStyleSelector()
// - ShowStyleEditor()
//-----------------------------------------------------------------------------

// Demo helper function to select among loaded fonts.
// Here we use the regular BeginCombo()/EndCombo() api which is the more flexible one.
void ImGui::ShowFontSelector(ImGuiContext* ctx, const char* label)
{
    ImGuiIO& io = ImGui::GetIO(ctx);
    ImFont* font_current = ImGui::GetFont(ctx);
    if (ImGui::BeginCombo(ctx, label, font_current->GetDebugName()))
    {
        for (ImFont* font : io.Fonts->Fonts)
        {
            ImGui::PushID(ctx, (void*)font);
            if (ImGui::Selectable(ctx, font->GetDebugName(), font == font_current))
                io.FontDefault = font;
            ImGui::PopID(ctx);
        }
        ImGui::EndCombo(ctx);
    }
    ImGui::SameLine(ctx);
    HelpMarker(ctx, 
        "- Load additional fonts with io.Fonts->AddFontFromFileTTF().\n"
        "- The font atlas is built when calling io.Fonts->GetTexDataAsXXXX() or io.Fonts->Build().\n"
        "- Read FAQ and docs/FONTS.md for more details.\n"
        "- If you need to add/remove fonts at runtime (e.g. for DPI change), do it before calling NewFrame().");
}

// Demo helper function to select among default colors. See ShowStyleEditor() for more advanced options.
// Here we use the simplified Combo() api that packs items into a single literal string.
// Useful for quick combo boxes where the choices are known locally.
bool ImGui::ShowStyleSelector(ImGuiContext* ctx, const char* label)
{
    static int style_idx = -1;
    if (ImGui::Combo(ctx, label, &style_idx, "Dark\0Light\0Classic\0"))
    {
        switch (style_idx)
        {
        case 0: ImGui::StyleColorsDark(ctx); break;
        case 1: ImGui::StyleColorsLight(ctx); break;
        case 2: ImGui::StyleColorsClassic(ctx); break;
        }
        return true;
    }
    return false;
}

void ImGui::ShowStyleEditor(ImGuiContext* ctx, ImGuiStyle* ref)
{
    IMGUI_DEMO_MARKER("Tools/Style Editor");
    // You can pass in a reference ImGuiStyle structure to compare to, revert to and save to
    // (without a reference style pointer, we will use one compared locally as a reference)
    ImGuiStyle& style = ImGui::GetStyle(ctx);
    static ImGuiStyle ref_saved_style;

    // Default to using internal storage as reference
    static bool init = true;
    if (init && ref == NULL)
        ref_saved_style = style;
    init = false;
    if (ref == NULL)
        ref = &ref_saved_style;

    ImGui::PushItemWidth(ctx, ImGui::GetWindowWidth(ctx) * 0.50f);

    if (ImGui::ShowStyleSelector(ctx, "Colors##Selector"))
        ref_saved_style = style;
    ImGui::ShowFontSelector(ctx, "Fonts##Selector");

    // Simplified Settings (expose floating-pointer border sizes as boolean representing 0.0f or 1.0f)
    if (ImGui::SliderFloat(ctx, "FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f"))
        style.GrabRounding = style.FrameRounding; // Make GrabRounding always the same value as FrameRounding
    { bool border = (style.WindowBorderSize > 0.0f); if (ImGui::Checkbox(ctx, "WindowBorder", &border)) { style.WindowBorderSize = border ? 1.0f : 0.0f; } }
    ImGui::SameLine(ctx);
    { bool border = (style.FrameBorderSize > 0.0f);  if (ImGui::Checkbox(ctx, "FrameBorder",  &border)) { style.FrameBorderSize  = border ? 1.0f : 0.0f; } }
    ImGui::SameLine(ctx);
    { bool border = (style.PopupBorderSize > 0.0f);  if (ImGui::Checkbox(ctx, "PopupBorder",  &border)) { style.PopupBorderSize  = border ? 1.0f : 0.0f; } }

    // Save/Revert button
    if (ImGui::Button(ctx, "Save Ref"))
        *ref = ref_saved_style = style;
    ImGui::SameLine(ctx);
    if (ImGui::Button(ctx, "Revert Ref"))
        style = *ref;
    ImGui::SameLine(ctx);
    HelpMarker(ctx, 
        "Save/Revert in local non-persistent storage. Default Colors definition are not affected. "
        "Use \"Export\" below to save them somewhere.");

    ImGui::Separator(ctx);

    if (ImGui::BeginTabBar(ctx, "##tabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem(ctx, "Sizes"))
        {
            ImGui::SeparatorText(ctx, "Main");
            ImGui::SliderFloat2(ctx, "WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2(ctx, "FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2(ctx, "ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2(ctx, "ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2(ctx, "TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
            ImGui::SliderFloat(ctx, "IndentSpacing", &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
            ImGui::SliderFloat(ctx, "ScrollbarSize", &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
            ImGui::SliderFloat(ctx, "GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");

            ImGui::SeparatorText(ctx, "Borders");
            ImGui::SliderFloat(ctx, "WindowBorderSize", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat(ctx, "ChildBorderSize", &style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat(ctx, "PopupBorderSize", &style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat(ctx, "FrameBorderSize", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat(ctx, "TabBorderSize", &style.TabBorderSize, 0.0f, 1.0f, "%.0f");
            ImGui::SliderFloat(ctx, "TabBarBorderSize", &style.TabBarBorderSize, 0.0f, 2.0f, "%.0f");

            ImGui::SeparatorText(ctx, "Rounding");
            ImGui::SliderFloat(ctx, "WindowRounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(ctx, "ChildRounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(ctx, "FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(ctx, "PopupRounding", &style.PopupRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(ctx, "ScrollbarRounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(ctx, "GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
            ImGui::SliderFloat(ctx, "TabRounding", &style.TabRounding, 0.0f, 12.0f, "%.0f");

            ImGui::SeparatorText(ctx, "Tables");
            ImGui::SliderFloat2(ctx, "CellPadding", (float*)&style.CellPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderAngle(ctx, "TableAngledHeadersAngle", &style.TableAngledHeadersAngle, -50.0f, +50.0f);

            ImGui::SeparatorText(ctx, "Widgets");
            ImGui::SliderFloat2(ctx, "WindowTitleAlign", (float*)&style.WindowTitleAlign, 0.0f, 1.0f, "%.2f");
            int window_menu_button_position = style.WindowMenuButtonPosition + 1;
            if (ImGui::Combo(ctx, "WindowMenuButtonPosition", (int*)&window_menu_button_position, "None\0Left\0Right\0"))
                style.WindowMenuButtonPosition = window_menu_button_position - 1;
            ImGui::Combo(ctx, "ColorButtonPosition", (int*)&style.ColorButtonPosition, "Left\0Right\0");
            ImGui::SliderFloat2(ctx, "ButtonTextAlign", (float*)&style.ButtonTextAlign, 0.0f, 1.0f, "%.2f");
            ImGui::SameLine(ctx); HelpMarker(ctx, "Alignment applies when a button is larger than its text content.");
            ImGui::SliderFloat2(ctx, "SelectableTextAlign", (float*)&style.SelectableTextAlign, 0.0f, 1.0f, "%.2f");
            ImGui::SameLine(ctx); HelpMarker(ctx, "Alignment applies when a selectable is larger than its text content.");
            ImGui::SliderFloat(ctx, "SeparatorTextBorderSize", &style.SeparatorTextBorderSize, 0.0f, 10.0f, "%.0f");
            ImGui::SliderFloat2(ctx, "SeparatorTextAlign", (float*)&style.SeparatorTextAlign, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat2(ctx, "SeparatorTextPadding", (float*)&style.SeparatorTextPadding, 0.0f, 40.0f, "%.0f");
            ImGui::SliderFloat(ctx, "LogSliderDeadzone", &style.LogSliderDeadzone, 0.0f, 12.0f, "%.0f");

            ImGui::SeparatorText(ctx, "Tooltips");
            for (int n = 0; n < 2; n++)
                if (ImGui::TreeNodeEx(ctx, n == 0 ? "HoverFlagsForTooltipMouse" : "HoverFlagsForTooltipNav"))
                {
                    ImGuiHoveredFlags* p = (n == 0) ? &style.HoverFlagsForTooltipMouse : &style.HoverFlagsForTooltipNav;
                    ImGui::CheckboxFlags(ctx, "ImGuiHoveredFlags_DelayNone", p, ImGuiHoveredFlags_DelayNone);
                    ImGui::CheckboxFlags(ctx, "ImGuiHoveredFlags_DelayShort", p, ImGuiHoveredFlags_DelayShort);
                    ImGui::CheckboxFlags(ctx, "ImGuiHoveredFlags_DelayNormal", p, ImGuiHoveredFlags_DelayNormal);
                    ImGui::CheckboxFlags(ctx, "ImGuiHoveredFlags_Stationary", p, ImGuiHoveredFlags_Stationary);
                    ImGui::CheckboxFlags(ctx, "ImGuiHoveredFlags_NoSharedDelay", p, ImGuiHoveredFlags_NoSharedDelay);
                    ImGui::TreePop(ctx);
                }

            ImGui::SeparatorText(ctx, "Misc");
            ImGui::SliderFloat2(ctx, "DisplaySafeAreaPadding", (float*)&style.DisplaySafeAreaPadding, 0.0f, 30.0f, "%.0f"); ImGui::SameLine(ctx); HelpMarker(ctx, "Adjust if you cannot see the edges of your screen (e.g. on a TV where scaling has not been configured).");

            ImGui::EndTabItem(ctx);
        }

        if (ImGui::BeginTabItem(ctx, "Colors"))
        {
            static int output_dest = 0;
            static bool output_only_modified = true;
            if (ImGui::Button(ctx, "Export"))
            {
                if (output_dest == 0)
                    ImGui::LogToClipboard(ctx);
                else
                    ImGui::LogToTTY(ctx);
                ImGui::LogText(ctx, "ImVec4* colors = ImGui::GetStyle().Colors;" IM_NEWLINE);
                for (int i = 0; i < ImGuiCol_COUNT; i++)
                {
                    const ImVec4& col = style.Colors[i];
                    const char* name = ImGui::GetStyleColorName(i);
                    if (!output_only_modified || memcmp(&col, &ref->Colors[i], sizeof(ImVec4)) != 0)
                        ImGui::LogText(ctx, "colors[ImGuiCol_%s]%*s= ImVec4(%.2ff, %.2ff, %.2ff, %.2ff);" IM_NEWLINE,
                            name, 23 - (int)strlen(name), "", col.x, col.y, col.z, col.w);
                }
                ImGui::LogFinish(ctx);
            }
            ImGui::SameLine(ctx); ImGui::SetNextItemWidth(ctx, 120); ImGui::Combo(ctx, "##output_type", &output_dest, "To Clipboard\0To TTY\0");
            ImGui::SameLine(ctx); ImGui::Checkbox(ctx, "Only Modified Colors", &output_only_modified);

            static ImGuiTextFilter filter;
            filter.Draw("Filter colors", ImGui::GetFontSize(ctx) * 16);

            static ImGuiColorEditFlags alpha_flags = 0;
            if (ImGui::RadioButton(ctx, "Opaque", alpha_flags == ImGuiColorEditFlags_None))             { alpha_flags = ImGuiColorEditFlags_None; } ImGui::SameLine(ctx);
            if (ImGui::RadioButton(ctx, "Alpha",  alpha_flags == ImGuiColorEditFlags_AlphaPreview))     { alpha_flags = ImGuiColorEditFlags_AlphaPreview; } ImGui::SameLine(ctx);
            if (ImGui::RadioButton(ctx, "Both",   alpha_flags == ImGuiColorEditFlags_AlphaPreviewHalf)) { alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf; } ImGui::SameLine(ctx);
            HelpMarker(ctx, 
                "In the color list:\n"
                "Left-click on color square to open color picker,\n"
                "Right-click to open edit options menu.");

            ImGui::SetNextWindowSizeConstraints(ctx, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing(ctx) * 10), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::BeginChild(ctx, "##colors", ImVec2(0, 0), ImGuiChildFlags_Border, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NavFlattened);
            ImGui::PushItemWidth(ctx, ImGui::GetFontSize(ctx) * -12);
            for (int i = 0; i < ImGuiCol_COUNT; i++)
            {
                const char* name = ImGui::GetStyleColorName(i);
                if (!filter.PassFilter(name))
                    continue;
                ImGui::PushID(ctx, i);
                ImGui::ColorEdit4(ctx, "##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | alpha_flags);
                if (memcmp(&style.Colors[i], &ref->Colors[i], sizeof(ImVec4)) != 0)
                {
                    // Tips: in a real user application, you may want to merge and use an icon font into the main font,
                    // so instead of "Save"/"Revert" you'd use icons!
                    // Read the FAQ and docs/FONTS.md about using icon fonts. It's really easy and super convenient!
                    ImGui::SameLine(ctx, 0.0f, style.ItemInnerSpacing.x); if (ImGui::Button(ctx, "Save")) { ref->Colors[i] = style.Colors[i]; }
                    ImGui::SameLine(ctx, 0.0f, style.ItemInnerSpacing.x); if (ImGui::Button(ctx, "Revert")) { style.Colors[i] = ref->Colors[i]; }
                }
                ImGui::SameLine(ctx, 0.0f, style.ItemInnerSpacing.x);
                ImGui::TextUnformatted(ctx, name);
                ImGui::PopID(ctx);
            }
            ImGui::PopItemWidth(ctx);
            ImGui::EndChild(ctx);

            ImGui::EndTabItem(ctx);
        }

        if (ImGui::BeginTabItem(ctx, "Fonts"))
        {
            ImGuiIO& io = ImGui::GetIO(ctx);
            ImFontAtlas* atlas = io.Fonts;
            HelpMarker(ctx, "Read FAQ and docs/FONTS.md for details on font loading.");
            ImGui::ShowFontAtlas(ctx, atlas);

            // Post-baking font scaling. Note that this is NOT the nice way of scaling fonts, read below.
            // (we enforce hard clamping manually as by default DragFloat/SliderFloat allows CTRL+Click text to get out of bounds).
            const float MIN_SCALE = 0.3f;
            const float MAX_SCALE = 2.0f;
            HelpMarker(ctx, 
                "Those are old settings provided for convenience.\n"
                "However, the _correct_ way of scaling your UI is currently to reload your font at the designed size, "
                "rebuild the font atlas, and call style.ScaleAllSizes() on a reference ImGuiStyle structure.\n"
                "Using those settings here will give you poor quality results.");
            static float window_scale = 1.0f;
            ImGui::PushItemWidth(ctx, ImGui::GetFontSize(ctx) * 8);
            if (ImGui::DragFloat(ctx, "window scale", &window_scale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp)) // Scale only this window
                ImGui::SetWindowFontScale(ctx, window_scale);
            ImGui::DragFloat(ctx, "global scale", &io.FontGlobalScale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp); // Scale everything
            ImGui::PopItemWidth(ctx);

            ImGui::EndTabItem(ctx);
        }

        if (ImGui::BeginTabItem(ctx, "Rendering"))
        {
            ImGui::Checkbox(ctx, "Anti-aliased lines", &style.AntiAliasedLines);
            ImGui::SameLine(ctx);
            HelpMarker(ctx, "When disabling anti-aliasing lines, you'll probably want to disable borders in your style as well.");

            ImGui::Checkbox(ctx, "Anti-aliased lines use texture", &style.AntiAliasedLinesUseTex);
            ImGui::SameLine(ctx);
            HelpMarker(ctx, "Faster lines using texture data. Require backend to render with bilinear filtering (not point/nearest filtering).");

            ImGui::Checkbox(ctx, "Anti-aliased fill", &style.AntiAliasedFill);
            ImGui::PushItemWidth(ctx, ImGui::GetFontSize(ctx) * 8);
            ImGui::DragFloat(ctx, "Curve Tessellation Tolerance", &style.CurveTessellationTol, 0.02f, 0.10f, 10.0f, "%.2f");
            if (style.CurveTessellationTol < 0.10f) style.CurveTessellationTol = 0.10f;

            // When editing the "Circle Segment Max Error" value, draw a preview of its effect on auto-tessellated circles.
            ImGui::DragFloat(ctx, "Circle Tessellation Max Error", &style.CircleTessellationMaxError , 0.005f, 0.10f, 5.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            const bool show_samples = ImGui::IsItemActive(ctx);
            if (show_samples)
                ImGui::SetNextWindowPos(ctx, ImGui::GetCursorScreenPos(ctx));
            if (show_samples && ImGui::BeginTooltip(ctx))
            {
                ImGui::TextUnformatted(ctx, "(R = radius, N = number of segments)");
                ImGui::Spacing(ctx);
                ImDrawList* draw_list = ImGui::GetWindowDrawList(ctx);
                const float min_widget_width = ImGui::CalcTextSize(ctx, "N: MMM\nR: MMM").x;
                for (int n = 0; n < 8; n++)
                {
                    const float RAD_MIN = 5.0f;
                    const float RAD_MAX = 70.0f;
                    const float rad = RAD_MIN + (RAD_MAX - RAD_MIN) * (float)n / (8.0f - 1.0f);

                    ImGui::BeginGroup(ctx);

                    ImGui::Text(ctx, "R: %.f\nN: %d", rad, draw_list->_CalcCircleAutoSegmentCount(rad));

                    const float canvas_width = IM_MAX(min_widget_width, rad * 2.0f);
                    const float offset_x     = floorf(canvas_width * 0.5f);
                    const float offset_y     = floorf(RAD_MAX);

                    const ImVec2 p1 = ImGui::GetCursorScreenPos(ctx);
                    draw_list->AddCircle(ImVec2(p1.x + offset_x, p1.y + offset_y), rad, ImGui::GetColorU32(ctx, ImGuiCol_Text));
                    ImGui::Dummy(ctx, ImVec2(canvas_width, RAD_MAX * 2));

                    /*
                    const ImVec2 p2 = ImGui::GetCursorScreenPos();
                    draw_list->AddCircleFilled(ImVec2(p2.x + offset_x, p2.y + offset_y), rad, ImGui::GetColorU32(ImGuiCol_Text));
                    ImGui::Dummy(ImVec2(canvas_width, RAD_MAX * 2));
                    */

                    ImGui::EndGroup(ctx);
                    ImGui::SameLine(ctx);
                }
                ImGui::EndTooltip(ctx);
            }
            ImGui::SameLine(ctx);
            HelpMarker(ctx, "When drawing circle primitives with \"num_segments == 0\" tesselation will be calculated automatically.");

            ImGui::DragFloat(ctx, "Global Alpha", &style.Alpha, 0.005f, 0.20f, 1.0f, "%.2f"); // Not exposing zero here so user doesn't "lose" the UI (zero alpha clips all widgets). But application code could have a toggle to switch between zero and non-zero.
            ImGui::DragFloat(ctx, "Disabled Alpha", &style.DisabledAlpha, 0.005f, 0.0f, 1.0f, "%.2f"); ImGui::SameLine(ctx); HelpMarker(ctx, "Additional alpha multiplier for disabled items (multiply over current value of Alpha).");
            ImGui::PopItemWidth(ctx);

            ImGui::EndTabItem(ctx);
        }

        ImGui::EndTabBar(ctx);
    }

    ImGui::PopItemWidth(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] User Guide / ShowUserGuide()
//-----------------------------------------------------------------------------

void ImGui::ShowUserGuide(ImGuiContext* ctx)
{
    ImGuiIO& io = ImGui::GetIO(ctx);
    ImGui::BulletText(ctx, "Double-click on title bar to collapse window.");
    ImGui::BulletText(ctx, 
        "Click and drag on lower corner to resize window\n"
        "(double-click to auto fit window to its contents).");
    ImGui::BulletText(ctx, "CTRL+Click on a slider or drag box to input value as text.");
    ImGui::BulletText(ctx, "TAB/SHIFT+TAB to cycle through keyboard editable fields.");
    ImGui::BulletText(ctx, "CTRL+Tab to select a window.");
    if (io.FontAllowUserScaling)
        ImGui::BulletText(ctx, "CTRL+Mouse Wheel to zoom window contents.");
    ImGui::BulletText(ctx, "While inputing text:\n");
    ImGui::Indent(ctx);
    ImGui::BulletText(ctx, "CTRL+Left/Right to word jump.");
    ImGui::BulletText(ctx, "CTRL+A or double-click to select all.");
    ImGui::BulletText(ctx, "CTRL+X/C/V to use clipboard cut/copy/paste.");
    ImGui::BulletText(ctx, "CTRL+Z,CTRL+Y to undo/redo.");
    ImGui::BulletText(ctx, "ESCAPE to revert.");
    ImGui::Unindent(ctx);
    ImGui::BulletText(ctx, "With keyboard navigation enabled:");
    ImGui::Indent(ctx);
    ImGui::BulletText(ctx, "Arrow keys to navigate.");
    ImGui::BulletText(ctx, "Space to activate a widget.");
    ImGui::BulletText(ctx, "Return to input text into a widget.");
    ImGui::BulletText(ctx, "Escape to deactivate a widget, close popup, exit child window.");
    ImGui::BulletText(ctx, "Alt to jump to the menu layer of a window.");
    ImGui::Unindent(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Main Menu Bar / ShowExampleAppMainMenuBar()
//-----------------------------------------------------------------------------
// - ShowExampleAppMainMenuBar()
// - ShowExampleMenuFile()
//-----------------------------------------------------------------------------

// Demonstrate creating a "main" fullscreen menu bar and populating it.
// Note the difference between BeginMainMenuBar() and BeginMenuBar():
// - BeginMenuBar() = menu-bar inside current window (which needs the ImGuiWindowFlags_MenuBar flag!)
// - BeginMainMenuBar() = helper to create menu-bar-sized window at the top of the main viewport + call BeginMenuBar() into it.
static void ShowExampleAppMainMenuBar(ImGuiContext* ctx)
{
    if (ImGui::BeginMainMenuBar(ctx))
    {
        if (ImGui::BeginMenu(ctx, "File"))
        {
            ShowExampleMenuFile(ctx);
            ImGui::EndMenu(ctx);
        }
        if (ImGui::BeginMenu(ctx, "Edit"))
        {
            if (ImGui::MenuItem(ctx, "Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem(ctx, "Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator(ctx);
            if (ImGui::MenuItem(ctx, "Cut", "CTRL+X")) {}
            if (ImGui::MenuItem(ctx, "Copy", "CTRL+C")) {}
            if (ImGui::MenuItem(ctx, "Paste", "CTRL+V")) {}
            ImGui::EndMenu(ctx);
        }
        ImGui::EndMainMenuBar(ctx);
    }
}

// Note that shortcuts are currently provided for display only
// (future version will add explicit flags to BeginMenu() to request processing shortcuts)
static void ShowExampleMenuFile(ImGuiContext* ctx)
{
    IMGUI_DEMO_MARKER("Examples/Menu");
    ImGui::MenuItem(ctx, "(demo menu)", NULL, false, false);
    if (ImGui::MenuItem(ctx, "New")) {}
    if (ImGui::MenuItem(ctx, "Open", "Ctrl+O")) {}
    if (ImGui::BeginMenu(ctx, "Open Recent"))
    {
        ImGui::MenuItem(ctx, "fish_hat.c");
        ImGui::MenuItem(ctx, "fish_hat.inl");
        ImGui::MenuItem(ctx, "fish_hat.h");
        if (ImGui::BeginMenu(ctx, "More.."))
        {
            ImGui::MenuItem(ctx, "Hello");
            ImGui::MenuItem(ctx, "Sailor");
            if (ImGui::BeginMenu(ctx, "Recurse.."))
            {
                ShowExampleMenuFile(ctx);
                ImGui::EndMenu(ctx);
            }
            ImGui::EndMenu(ctx);
        }
        ImGui::EndMenu(ctx);
    }
    if (ImGui::MenuItem(ctx, "Save", "Ctrl+S")) {}
    if (ImGui::MenuItem(ctx, "Save As..")) {}

    ImGui::Separator(ctx);
    IMGUI_DEMO_MARKER("Examples/Menu/Options");
    if (ImGui::BeginMenu(ctx, "Options"))
    {
        static bool enabled = true;
        ImGui::MenuItem(ctx, "Enabled", "", &enabled);
        ImGui::BeginChild(ctx, "child", ImVec2(0, 60), ImGuiChildFlags_Border);
        for (int i = 0; i < 10; i++)
            ImGui::Text(ctx, "Scrolling Text %d", i);
        ImGui::EndChild(ctx);
        static float f = 0.5f;
        static int n = 0;
        ImGui::SliderFloat(ctx, "Value", &f, 0.0f, 1.0f);
        ImGui::InputFloat(ctx, "Input", &f, 0.1f);
        ImGui::Combo(ctx, "Combo", &n, "Yes\0No\0Maybe\0\0");
        ImGui::EndMenu(ctx);
    }

    IMGUI_DEMO_MARKER("Examples/Menu/Colors");
    if (ImGui::BeginMenu(ctx, "Colors"))
    {
        float sz = ImGui::GetTextLineHeight(ctx);
        for (int i = 0; i < ImGuiCol_COUNT; i++)
        {
            const char* name = ImGui::GetStyleColorName((ImGuiCol)i);
            ImVec2 p = ImGui::GetCursorScreenPos(ctx);
            ImGui::GetWindowDrawList(ctx)->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), ImGui::GetColorU32(ctx, (ImGuiCol)i));
            ImGui::Dummy(ctx, ImVec2(sz, sz));
            ImGui::SameLine(ctx);
            ImGui::MenuItem(ctx, name);
        }
        ImGui::EndMenu(ctx);
    }

    // Here we demonstrate appending again to the "Options" menu (which we already created above)
    // Of course in this demo it is a little bit silly that this function calls BeginMenu("Options") twice.
    // In a real code-base using it would make senses to use this feature from very different code locations.
    if (ImGui::BeginMenu(ctx, "Options")) // <-- Append!
    {
        IMGUI_DEMO_MARKER("Examples/Menu/Append to an existing menu");
        static bool b = true;
        ImGui::Checkbox(ctx, "SomeOption", &b);
        ImGui::EndMenu(ctx);
    }

    if (ImGui::BeginMenu(ctx, "Disabled", false)) // Disabled
    {
        IM_ASSERT(0);
    }
    if (ImGui::MenuItem(ctx, "Checked", NULL, true)) {}
    ImGui::Separator(ctx);
    if (ImGui::MenuItem(ctx, "Quit", "Alt+F4")) {}
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Debug Console / ShowExampleAppConsole()
//-----------------------------------------------------------------------------

// Demonstrate creating a simple console window, with scrolling, filtering, completion and history.
// For the console example, we are using a more C++ like approach of declaring a class to hold both data and functions.
struct ExampleAppConsole
{
    char                  InputBuf[256];
    ImVector<char*>       Items;
    ImVector<const char*> Commands;
    ImVector<char*>       History;
    int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter       Filter;
    bool                  AutoScroll;
    bool                  ScrollToBottom;

    ExampleAppConsole()
    {
        IMGUI_DEMO_MARKER("Examples/Console");
        ClearLog();
        memset(InputBuf, 0, sizeof(InputBuf));
        HistoryPos = -1;

        // "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
        Commands.push_back("HELP");
        Commands.push_back("HISTORY");
        Commands.push_back("CLEAR");
        Commands.push_back("CLASSIFY");
        AutoScroll = true;
        ScrollToBottom = false;
        AddLog("Welcome to Dear ImGui!");
    }
    ~ExampleAppConsole()
    {
        ClearLog();
        for (int i = 0; i < History.Size; i++)
            free(History[i]);
    }

    // Portable helpers
    static int   Stricmp(const char* s1, const char* s2)         { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
    static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
    static char* Strdup(const char* s)                           { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
    static void  Strtrim(char* s)                                { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

    void    ClearLog()
    {
        for (int i = 0; i < Items.Size; i++)
            free(Items[i]);
        Items.clear();
    }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        // FIXME-OPT
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf)-1] = 0;
        va_end(args);
        Items.push_back(Strdup(buf));
    }

    void    Draw(ImGuiContext* ctx, const char* title, bool* p_open)
    {
        ImGui::SetNextWindowSize(ctx, ImVec2(520, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(ctx, title, p_open))
        {
            ImGui::End(ctx);
            return;
        }

        // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
        // So e.g. IsItemHovered() will return true when hovering the title bar.
        // Here we create a context menu only available from the title bar.
        if (ImGui::BeginPopupContextItem(ctx))
        {
            if (ImGui::MenuItem(ctx, "Close Console"))
                *p_open = false;
            ImGui::EndPopup(ctx);
        }

        ImGui::TextWrapped(ctx, 
            "This example implements a console with basic coloring, completion (TAB key) and history (Up/Down keys). A more elaborate "
            "implementation may want to store entries along with extra data such as timestamp, emitter, etc.");
        ImGui::TextWrapped(ctx, "Enter 'HELP' for help.");

        // TODO: display items starting from the bottom

        if (ImGui::SmallButton(ctx, "Add Debug Text"))  { AddLog("%d some text", Items.Size); AddLog("some more text"); AddLog("display very important message here!"); }
        ImGui::SameLine(ctx);
        if (ImGui::SmallButton(ctx, "Add Debug Error")) { AddLog("[error] something went wrong"); }
        ImGui::SameLine(ctx);
        if (ImGui::SmallButton(ctx, "Clear"))           { ClearLog(); }
        ImGui::SameLine(ctx);
        bool copy_to_clipboard = ImGui::SmallButton(ctx, "Copy");
        //static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

        ImGui::Separator(ctx);

        // Options menu
        if (ImGui::BeginPopup(ctx, "Options"))
        {
            ImGui::Checkbox(ctx, "Auto-scroll", &AutoScroll);
            ImGui::EndPopup(ctx);
        }

        // Options, Filter
        if (ImGui::Button(ctx, "Options"))
            ImGui::OpenPopup(ctx, "Options");
        ImGui::SameLine(ctx);
        Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        ImGui::Separator(ctx);

        // Reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve = ImGui::GetStyle(ctx).ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(ctx);
        if (ImGui::BeginChild(ctx, "ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (ImGui::BeginPopupContextWindow(ctx))
            {
                if (ImGui::Selectable(ctx, "Clear")) ClearLog();
                ImGui::EndPopup(ctx);
            }

            // Display every line as a separate entry so we can change their color or add custom widgets.
            // If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
            // NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
            // to only process visible items. The clipper will automatically measure the height of your first item and then
            // "seek" to display only items in the visible area.
            // To use the clipper we can replace your standard loop:
            //      for (int i = 0; i < Items.Size; i++)
            //   With:
            //      ImGuiListClipper clipper;
            //      clipper.Begin(Items.Size);
            //      while (clipper.Step())
            //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            // - That your items are evenly spaced (same height)
            // - That you have cheap random access to your elements (you can access them given their index,
            //   without processing all the ones before)
            // You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
            // We would need random-access on the post-filtered list.
            // A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
            // or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
            // and appending newly elements as they are inserted. This is left as a task to the user until we can manage
            // to improve this example code!
            // If your items are of variable height:
            // - Split them into same height items would be simpler and facilitate random-seeking into your list.
            // - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
            ImGui::PushStyleVar(ctx, ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
            if (copy_to_clipboard)
                ImGui::LogToClipboard(ctx);
            for (const char* item : Items)
            {
                if (!Filter.PassFilter(item))
                    continue;

                // Normally you would store more information in your item than just a string.
                // (e.g. make Items[] an array of structure, store color/type etc.)
                ImVec4 color;
                bool has_color = false;
                if (strstr(item, "[error]")) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
                else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
                if (has_color)
                    ImGui::PushStyleColor(ctx, ImGuiCol_Text, color);
                ImGui::TextUnformatted(ctx, item);
                if (has_color)
                    ImGui::PopStyleColor(ctx);
            }
            if (copy_to_clipboard)
                ImGui::LogFinish(ctx);

            // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
            // Using a scrollbar or mouse-wheel will take away from the bottom edge.
            if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY(ctx) >= ImGui::GetScrollMaxY(ctx)))
                ImGui::SetScrollHereY(ctx, 1.0f);
            ScrollToBottom = false;

            ImGui::PopStyleVar(ctx);
        }
        ImGui::EndChild(ctx);
        ImGui::Separator(ctx);

        // Command-line
        bool reclaim_focus = false;
        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
        if (ImGui::InputText(ctx, "Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
        {
            char* s = InputBuf;
            Strtrim(s);
            if (s[0])
                ExecCommand(s);
            strcpy(s, "");
            reclaim_focus = true;
        }

        // Auto-focus on window apparition
        ImGui::SetItemDefaultFocus(ctx);
        if (reclaim_focus)
            ImGui::SetKeyboardFocusHere(ctx, -1); // Auto focus previous widget

        ImGui::End(ctx);
    }

    void    ExecCommand(const char* command_line)
    {
        AddLog("# %s\n", command_line);

        // Insert into history. First find match and delete it so it can be pushed to the back.
        // This isn't trying to be smart or optimal.
        HistoryPos = -1;
        for (int i = History.Size - 1; i >= 0; i--)
            if (Stricmp(History[i], command_line) == 0)
            {
                free(History[i]);
                History.erase(History.begin() + i);
                break;
            }
        History.push_back(Strdup(command_line));

        // Process command
        if (Stricmp(command_line, "CLEAR") == 0)
        {
            ClearLog();
        }
        else if (Stricmp(command_line, "HELP") == 0)
        {
            AddLog("Commands:");
            for (int i = 0; i < Commands.Size; i++)
                AddLog("- %s", Commands[i]);
        }
        else if (Stricmp(command_line, "HISTORY") == 0)
        {
            int first = History.Size - 10;
            for (int i = first > 0 ? first : 0; i < History.Size; i++)
                AddLog("%3d: %s\n", i, History[i]);
        }
        else
        {
            AddLog("Unknown command: '%s'\n", command_line);
        }

        // On command input, we scroll to bottom even if AutoScroll==false
        ScrollToBottom = true;
    }

    // In C++11 you'd be better off using lambdas for this sort of forwarding callbacks
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        ExampleAppConsole* console = (ExampleAppConsole*)data->UserData;
        return console->TextEditCallback(data);
    }

    int     TextEditCallback(ImGuiInputTextCallbackData* data)
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
                    if (c == ' ' || c == '\t' || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                // Build a list of candidates
                ImVector<const char*> candidates;
                for (int i = 0; i < Commands.Size; i++)
                    if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
                        candidates.push_back(Commands[i]);

                if (candidates.Size == 0)
                {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
                }
                else if (candidates.Size == 1)
                {
                    // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
                    data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                }
                else
                {
                    // Multiple matches. Complete as much as we can..
                    // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
                    int match_len = (int)(word_end - word_start);
                    for (;;)
                    {
                        int c = 0;
                        bool all_candidates_matches = true;
                        for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c == 0 || c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

                    if (match_len > 0)
                    {
                        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                    }

                    // List matches
                    AddLog("Possible matches:\n");
                    for (int i = 0; i < candidates.Size; i++)
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
                        HistoryPos = History.Size - 1;
                    else if (HistoryPos > 0)
                        HistoryPos--;
                }
                else if (data->EventKey == ImGuiKey_DownArrow)
                {
                    if (HistoryPos != -1)
                        if (++HistoryPos >= History.Size)
                            HistoryPos = -1;
                }

                // A better implementation would preserve the data on the current input line along with cursor position.
                if (prev_history_pos != HistoryPos)
                {
                    const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, history_str);
                }
            }
        }
        return 0;
    }
};

static void ShowExampleAppConsole(ImGuiContext* ctx, bool* p_open)
{
    static ExampleAppConsole console;
    console.Draw(ctx, "Example: Console", p_open);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Debug Log / ShowExampleAppLog()
//-----------------------------------------------------------------------------

// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
struct ExampleAppLog
{
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool                AutoScroll;  // Keep scrolling if already at the bottom.

    ExampleAppLog()
    {
        AutoScroll = true;
        Clear();
    }

    void    Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void    Draw(ImGuiContext* ctx, const char* title, bool* p_open = NULL)
    {
        if (!ImGui::Begin(ctx, title, p_open))
        {
            ImGui::End(ctx);
            return;
        }

        // Options menu
        if (ImGui::BeginPopup(ctx, "Options"))
        {
            ImGui::Checkbox(ctx, "Auto-scroll", &AutoScroll);
            ImGui::EndPopup(ctx);
        }

        // Main window
        if (ImGui::Button(ctx, "Options"))
            ImGui::OpenPopup(ctx, "Options");
        ImGui::SameLine(ctx);
        bool clear = ImGui::Button(ctx, "Clear");
        ImGui::SameLine(ctx);
        bool copy = ImGui::Button(ctx, "Copy");
        ImGui::SameLine(ctx);
        Filter.Draw("Filter", -100.0f);

        ImGui::Separator(ctx);

        if (ImGui::BeginChild(ctx, "scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (clear)
                Clear();
            if (copy)
                ImGui::LogToClipboard(ctx);

            ImGui::PushStyleVar(ctx, ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            const char* buf = Buf.begin();
            const char* buf_end = Buf.end();
            if (Filter.IsActive())
            {
                // In this example we don't use the clipper when Filter is enabled.
                // This is because we don't have random access to the result of our filter.
                // A real application processing logs with ten of thousands of entries may want to store the result of
                // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
                for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
                {
                    const char* line_start = buf + LineOffsets[line_no];
                    const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    if (Filter.PassFilter(line_start, line_end))
                        ImGui::TextUnformatted(ctx, line_start, line_end);
                }
            }
            else
            {
                // The simplest and easy way to display the entire buffer:
                //   ImGui::TextUnformatted(buf_begin, buf_end);
                // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
                // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
                // within the visible area.
                // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
                // on your side is recommended. Using ImGuiListClipper requires
                // - A) random access into your data
                // - B) items all being the  same height,
                // both of which we can handle since we have an array pointing to the beginning of each line of text.
                // When using the filter (in the block of code above) we don't have random access into the data to display
                // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
                // it possible (and would be recommended if you want to search through tens of thousands of entries).
                ImGuiListClipper clipper;
                clipper.Begin(LineOffsets.Size);
                while (clipper.Step())
                {
                    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                    {
                        const char* line_start = buf + LineOffsets[line_no];
                        const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                        ImGui::TextUnformatted(ctx, line_start, line_end);
                    }
                }
                clipper.End();
            }
            ImGui::PopStyleVar(ctx);

            // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
            // Using a scrollbar or mouse-wheel will take away from the bottom edge.
            if (AutoScroll && ImGui::GetScrollY(ctx) >= ImGui::GetScrollMaxY(ctx))
                ImGui::SetScrollHereY(ctx, 1.0f);
        }
        ImGui::EndChild(ctx);
        ImGui::End(ctx);
    }
};

// Demonstrate creating a simple log window with basic filtering.
static void ShowExampleAppLog(ImGuiContext* ctx, bool* p_open)
{
    static ExampleAppLog log;

    // For the demo: add a debug button _BEFORE_ the normal log window contents
    // We take advantage of a rarely used feature: multiple calls to Begin()/End() are appending to the _same_ window.
    // Most of the contents of the window will be added by the log.Draw() call.
    ImGui::SetNextWindowSize(ctx, ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin(ctx, "Example: Log", p_open);
    IMGUI_DEMO_MARKER("Examples/Log");
    if (ImGui::SmallButton(ctx, "[Debug] Add 5 entries"))
    {
        static int counter = 0;
        const char* categories[3] = { "info", "warn", "error" };
        const char* words[] = { "Bumfuzzled", "Cattywampus", "Snickersnee", "Abibliophobia", "Absquatulate", "Nincompoop", "Pauciloquent" };
        for (int n = 0; n < 5; n++)
        {
            const char* category = categories[counter % IM_ARRAYSIZE(categories)];
            const char* word = words[counter % IM_ARRAYSIZE(words)];
            log.AddLog("[%05d] [%s] Hello, current time is %.1f, here's a word: '%s'\n",
                ImGui::GetFrameCount(ctx), category, ImGui::GetTime(ctx), word);
            counter++;
        }
    }
    ImGui::End(ctx);

    // Actually call in the regular Log helper (which will Begin() into the same window as we just did)
    log.Draw(ctx, "Example: Log", p_open);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Simple Layout / ShowExampleAppLayout()
//-----------------------------------------------------------------------------

// Demonstrate create a window with multiple child windows.
static void ShowExampleAppLayout(ImGuiContext* ctx, bool* p_open)
{
    ImGui::SetNextWindowSize(ctx, ImVec2(500, 440), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(ctx, "Example: Simple layout", p_open, ImGuiWindowFlags_MenuBar))
    {
        IMGUI_DEMO_MARKER("Examples/Simple layout");
        if (ImGui::BeginMenuBar(ctx))
        {
            if (ImGui::BeginMenu(ctx, "File"))
            {
                if (ImGui::MenuItem(ctx, "Close", "Ctrl+W")) { *p_open = false; }
                ImGui::EndMenu(ctx);
            }
            ImGui::EndMenuBar(ctx);
        }

        // Left
        static int selected = 0;
        {
            ImGui::BeginChild(ctx, "left pane", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
            for (int i = 0; i < 100; i++)
            {
                // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
                char label[128];
                sprintf(label, "MyObject %d", i);
                if (ImGui::Selectable(ctx, label, selected == i))
                    selected = i;
            }
            ImGui::EndChild(ctx);
        }
        ImGui::SameLine(ctx);

        // Right
        {
            ImGui::BeginGroup(ctx);
            ImGui::BeginChild(ctx, "item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing(ctx))); // Leave room for 1 line below us
            ImGui::Text(ctx, "MyObject: %d", selected);
            ImGui::Separator(ctx);
            if (ImGui::BeginTabBar(ctx, "##Tabs", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem(ctx, "Description"))
                {
                    ImGui::TextWrapped(ctx, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. ");
                    ImGui::EndTabItem(ctx);
                }
                if (ImGui::BeginTabItem(ctx, "Details"))
                {
                    ImGui::Text(ctx, "ID: 0123456789");
                    ImGui::EndTabItem(ctx);
                }
                ImGui::EndTabBar(ctx);
            }
            ImGui::EndChild(ctx);
            if (ImGui::Button(ctx, "Revert")) {}
            ImGui::SameLine(ctx);
            if (ImGui::Button(ctx, "Save")) {}
            ImGui::EndGroup(ctx);
        }
    }
    ImGui::End(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Property Editor / ShowExampleAppPropertyEditor()
//-----------------------------------------------------------------------------

static void ShowPlaceholderObject(ImGuiContext* ctx, const char* prefix, int uid)
{
    // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
    ImGui::PushID(ctx, uid);

    // Text and Tree nodes are less high than framed widgets, using AlignTextToFramePadding() we add vertical spacing to make the tree lines equal high.
    ImGui::TableNextRow(ctx);
    ImGui::TableSetColumnIndex(ctx, 0);
    ImGui::AlignTextToFramePadding(ctx);
    bool node_open = ImGui::TreeNode(ctx, "Object", "%s_%u", prefix, uid);
    ImGui::TableSetColumnIndex(ctx, 1);
    ImGui::Text(ctx, "my sailor is rich");

    if (node_open)
    {
        static float placeholder_members[8] = { 0.0f, 0.0f, 1.0f, 3.1416f, 100.0f, 999.0f };
        for (int i = 0; i < 8; i++)
        {
            ImGui::PushID(ctx, i); // Use field index as identifier.
            if (i < 2)
            {
                ShowPlaceholderObject(ctx, "Child", 424242);
            }
            else
            {
                // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
                ImGui::TableNextRow(ctx);
                ImGui::TableSetColumnIndex(ctx, 0);
                ImGui::AlignTextToFramePadding(ctx);
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
                ImGui::TreeNodeEx(ctx, "Field", flags, "Field_%d", i);

                ImGui::TableSetColumnIndex(ctx, 1);
                ImGui::SetNextItemWidth(ctx, -FLT_MIN);
                if (i >= 5)
                    ImGui::InputFloat(ctx, "##value", &placeholder_members[i], 1.0f);
                else
                    ImGui::DragFloat(ctx, "##value", &placeholder_members[i], 0.01f);
                ImGui::NextColumn(ctx);
            }
            ImGui::PopID(ctx);
        }
        ImGui::TreePop(ctx);
    }
    ImGui::PopID(ctx);
}

// Demonstrate create a simple property editor.
// This demo is a bit lackluster nowadays, would be nice to improve.
static void ShowExampleAppPropertyEditor(ImGuiContext* ctx, bool* p_open)
{
    ImGui::SetNextWindowSize(ctx, ImVec2(430, 450), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(ctx, "Example: Property editor", p_open))
    {
        ImGui::End(ctx);
        return;
    }

    IMGUI_DEMO_MARKER("Examples/Property Editor");
    HelpMarker(ctx, 
        "This example shows how you may implement a property editor using two columns.\n"
        "All objects/fields data are dummies here.\n");

    ImGui::PushStyleVar(ctx, ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    if (ImGui::BeginTable(ctx, "##split", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupScrollFreeze(ctx, 0, 1);
        ImGui::TableSetupColumn(ctx, "Object");
        ImGui::TableSetupColumn(ctx, "Contents");
        ImGui::TableHeadersRow(ctx);

        // Iterate placeholder objects (all the same data)
        for (int obj_i = 0; obj_i < 4; obj_i++)
            ShowPlaceholderObject(ctx, "Object", obj_i);

        ImGui::EndTable(ctx);
    }
    ImGui::PopStyleVar(ctx);
    ImGui::End(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Long Text / ShowExampleAppLongText()
//-----------------------------------------------------------------------------

// Demonstrate/test rendering huge amount of text, and the incidence of clipping.
static void ShowExampleAppLongText(ImGuiContext* ctx, bool* p_open)
{
    ImGui::SetNextWindowSize(ctx, ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(ctx, "Example: Long text display", p_open))
    {
        ImGui::End(ctx);
        return;
    }
    IMGUI_DEMO_MARKER("Examples/Long text display");

    static int test_type = 0;
    static ImGuiTextBuffer log;
    static int lines = 0;
    ImGui::Text(ctx, "Printing unusually long amount of text.");
    ImGui::Combo(ctx, "Test type", &test_type,
        "Single call to TextUnformatted()\0"
        "Multiple calls to Text(), clipped\0"
        "Multiple calls to Text(), not clipped (slow)\0");
    ImGui::Text(ctx, "Buffer contents: %d lines, %d bytes", lines, log.size());
    if (ImGui::Button(ctx, "Clear")) { log.clear(); lines = 0; }
    ImGui::SameLine(ctx);
    if (ImGui::Button(ctx, "Add 1000 lines"))
    {
        for (int i = 0; i < 1000; i++)
            log.appendf("%i The quick brown fox jumps over the lazy dog\n", lines + i);
        lines += 1000;
    }
    ImGui::BeginChild(ctx, "Log");
    switch (test_type)
    {
    case 0:
        // Single call to TextUnformatted() with a big buffer
        ImGui::TextUnformatted(ctx, log.begin(), log.end());
        break;
    case 1:
        {
            // Multiple calls to Text(), manually coarsely clipped - demonstrate how to use the ImGuiListClipper helper.
            ImGui::PushStyleVar(ctx, ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGuiListClipper clipper;
            clipper.Begin(lines);
            while (clipper.Step())
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                    ImGui::Text(ctx, "%i The quick brown fox jumps over the lazy dog", i);
            ImGui::PopStyleVar(ctx);
            break;
        }
    case 2:
        // Multiple calls to Text(), not clipped (slow)
        ImGui::PushStyleVar(ctx, ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        for (int i = 0; i < lines; i++)
            ImGui::Text(ctx, "%i The quick brown fox jumps over the lazy dog", i);
        ImGui::PopStyleVar(ctx);
        break;
    }
    ImGui::EndChild(ctx);
    ImGui::End(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Auto Resize / ShowExampleAppAutoResize()
//-----------------------------------------------------------------------------

// Demonstrate creating a window which gets auto-resized according to its content.
static void ShowExampleAppAutoResize(ImGuiContext* ctx, bool* p_open)
{
    if (!ImGui::Begin(ctx, "Example: Auto-resizing window", p_open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End(ctx);
        return;
    }
    IMGUI_DEMO_MARKER("Examples/Auto-resizing window");

    static int lines = 10;
    ImGui::TextUnformatted(ctx, 
        "Window will resize every-frame to the size of its content.\n"
        "Note that you probably don't want to query the window size to\n"
        "output your content because that would create a feedback loop.");
    ImGui::SliderInt(ctx, "Number of lines", &lines, 1, 20);
    for (int i = 0; i < lines; i++)
        ImGui::Text(ctx, "%*sThis is line %d", i * 4, "", i); // Pad with space to extend size horizontally
    ImGui::End(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Constrained Resize / ShowExampleAppConstrainedResize()
//-----------------------------------------------------------------------------

// Demonstrate creating a window with custom resize constraints.
// Note that size constraints currently don't work on a docked window (when in 'docking' branch)
static void ShowExampleAppConstrainedResize(ImGuiContext* ctx, bool* p_open)
{
    struct CustomConstraints
    {
        // Helper functions to demonstrate programmatic constraints
        // FIXME: This doesn't take account of decoration size (e.g. title bar), library should make this easier.
        // FIXME: None of the three demos works consistently when resizing from borders.
        static void AspectRatio(ImGuiSizeCallbackData* data)
        {
            float aspect_ratio = *(float*)data->UserData;
            data->DesiredSize.y = (float)(int)(data->DesiredSize.x / aspect_ratio);
        }
        static void Square(ImGuiSizeCallbackData* data)
        {
            data->DesiredSize.x = data->DesiredSize.y = IM_MAX(data->DesiredSize.x, data->DesiredSize.y);
        }
        static void Step(ImGuiSizeCallbackData* data)
        {
            float step = *(float*)data->UserData;
            data->DesiredSize = ImVec2((int)(data->DesiredSize.x / step + 0.5f) * step, (int)(data->DesiredSize.y / step + 0.5f) * step);
        }
    };

    const char* test_desc[] =
    {
        "Between 100x100 and 500x500",
        "At least 100x100",
        "Resize vertical + lock current width",
        "Resize horizontal + lock current height",
        "Width Between 400 and 500",
        "Height at least 400",
        "Custom: Aspect Ratio 16:9",
        "Custom: Always Square",
        "Custom: Fixed Steps (100)",
    };

    // Options
    static bool auto_resize = false;
    static bool window_padding = true;
    static int type = 6; // Aspect Ratio
    static int display_lines = 10;

    // Submit constraint
    float aspect_ratio = 16.0f / 9.0f;
    float fixed_step = 100.0f;
    if (type == 0) ImGui::SetNextWindowSizeConstraints(ctx, ImVec2(100, 100), ImVec2(500, 500));         // Between 100x100 and 500x500
    if (type == 1) ImGui::SetNextWindowSizeConstraints(ctx, ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX)); // Width > 100, Height > 100
    if (type == 2) ImGui::SetNextWindowSizeConstraints(ctx, ImVec2(-1, 0),    ImVec2(-1, FLT_MAX));      // Resize vertical + lock current width
    if (type == 3) ImGui::SetNextWindowSizeConstraints(ctx, ImVec2(0, -1),    ImVec2(FLT_MAX, -1));      // Resize horizontal + lock current height
    if (type == 4) ImGui::SetNextWindowSizeConstraints(ctx, ImVec2(400, -1),  ImVec2(500, -1));          // Width Between and 400 and 500
    if (type == 5) ImGui::SetNextWindowSizeConstraints(ctx, ImVec2(-1, 500),  ImVec2(-1, FLT_MAX));      // Height at least 400
    if (type == 6) ImGui::SetNextWindowSizeConstraints(ctx, ImVec2(0, 0),     ImVec2(FLT_MAX, FLT_MAX), CustomConstraints::AspectRatio, (void*)&aspect_ratio);   // Aspect ratio
    if (type == 7) ImGui::SetNextWindowSizeConstraints(ctx, ImVec2(0, 0),     ImVec2(FLT_MAX, FLT_MAX), CustomConstraints::Square);                              // Always Square
    if (type == 8) ImGui::SetNextWindowSizeConstraints(ctx, ImVec2(0, 0),     ImVec2(FLT_MAX, FLT_MAX), CustomConstraints::Step, (void*)&fixed_step);            // Fixed Step

    // Submit window
    if (!window_padding)
        ImGui::PushStyleVar(ctx, ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const ImGuiWindowFlags window_flags = auto_resize ? ImGuiWindowFlags_AlwaysAutoResize : 0;
    const bool window_open = ImGui::Begin(ctx, "Example: Constrained Resize", p_open, window_flags);
    if (!window_padding)
        ImGui::PopStyleVar(ctx);
    if (window_open)
    {
        IMGUI_DEMO_MARKER("Examples/Constrained Resizing window");
        if (ImGui::GetIO(ctx).KeyShift)
        {
            // Display a dummy viewport (in your real app you would likely use ImageButton() to display a texture.
            ImVec2 avail_size = ImGui::GetContentRegionAvail(ctx);
            ImVec2 pos = ImGui::GetCursorScreenPos(ctx);
            ImGui::ColorButton(ctx, "viewport", ImVec4(0.5f, 0.2f, 0.5f, 1.0f), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, avail_size);
            ImGui::SetCursorScreenPos(ctx, ImVec2(pos.x + 10, pos.y + 10));
            ImGui::Text(ctx, "%.2f x %.2f", avail_size.x, avail_size.y);
        }
        else
        {
            ImGui::Text(ctx, "(Hold SHIFT to display a dummy viewport)");
            if (ImGui::Button(ctx, "Set 200x200")) { ImGui::SetWindowSize(ctx, ImVec2(200, 200)); } ImGui::SameLine(ctx);
            if (ImGui::Button(ctx, "Set 500x500")) { ImGui::SetWindowSize(ctx, ImVec2(500, 500)); } ImGui::SameLine(ctx);
            if (ImGui::Button(ctx, "Set 800x200")) { ImGui::SetWindowSize(ctx, ImVec2(800, 200)); }
            ImGui::SetNextItemWidth(ctx, ImGui::GetFontSize(ctx) * 20);
            ImGui::Combo(ctx, "Constraint", &type, test_desc, IM_ARRAYSIZE(test_desc));
            ImGui::SetNextItemWidth(ctx, ImGui::GetFontSize(ctx) * 20);
            ImGui::DragInt(ctx, "Lines", &display_lines, 0.2f, 1, 100);
            ImGui::Checkbox(ctx, "Auto-resize", &auto_resize);
            ImGui::Checkbox(ctx, "Window padding", &window_padding);
            for (int i = 0; i < display_lines; i++)
                ImGui::Text(ctx, "%*sHello, sailor! Making this line long enough for the example.", i * 4, "");
        }
    }
    ImGui::End(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Simple overlay / ShowExampleAppSimpleOverlay()
//-----------------------------------------------------------------------------

// Demonstrate creating a simple static window with no decoration
// + a context-menu to choose which corner of the screen to use.
static void ShowExampleAppSimpleOverlay(ImGuiContext* ctx, bool* p_open)
{
    static int location = 0;
    ImGuiIO& io = ImGui::GetIO(ctx);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (location >= 0)
    {
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport(ctx);
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(ctx, window_pos, ImGuiCond_Always, window_pos_pivot);
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    else if (location == -2)
    {
        // Center window
        ImGui::SetNextWindowPos(ctx, ImGui::GetMainViewport(ctx)->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(ctx, 0.35f); // Transparent background
    if (ImGui::Begin(ctx, "Example: Simple overlay", p_open, window_flags))
    {
        IMGUI_DEMO_MARKER("Examples/Simple Overlay");
        ImGui::Text(ctx, "Simple overlay\n" "(right-click to change position)");
        ImGui::Separator(ctx);
        if (ImGui::IsMousePosValid(ctx))
            ImGui::Text(ctx, "Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
        else
            ImGui::Text(ctx, "Mouse Position: <invalid>");
        if (ImGui::BeginPopupContextWindow(ctx))
        {
            if (ImGui::MenuItem(ctx, "Custom",       NULL, location == -1)) location = -1;
            if (ImGui::MenuItem(ctx, "Center",       NULL, location == -2)) location = -2;
            if (ImGui::MenuItem(ctx, "Top-left",     NULL, location == 0)) location = 0;
            if (ImGui::MenuItem(ctx, "Top-right",    NULL, location == 1)) location = 1;
            if (ImGui::MenuItem(ctx, "Bottom-left",  NULL, location == 2)) location = 2;
            if (ImGui::MenuItem(ctx, "Bottom-right", NULL, location == 3)) location = 3;
            if (p_open && ImGui::MenuItem(ctx, "Close")) *p_open = false;
            ImGui::EndPopup(ctx);
        }
    }
    ImGui::End(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Fullscreen window / ShowExampleAppFullscreen()
//-----------------------------------------------------------------------------

// Demonstrate creating a window covering the entire screen/viewport
static void ShowExampleAppFullscreen(ImGuiContext* ctx, bool* p_open)
{
    static bool use_work_area = true;
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

    // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
    // Based on your use case you may want one or the other.
    const ImGuiViewport* viewport = ImGui::GetMainViewport(ctx);
    ImGui::SetNextWindowPos(ctx, use_work_area ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(ctx, use_work_area ? viewport->WorkSize : viewport->Size);

    if (ImGui::Begin(ctx, "Example: Fullscreen window", p_open, flags))
    {
        ImGui::Checkbox(ctx, "Use work area instead of main area", &use_work_area);
        ImGui::SameLine(ctx);
        HelpMarker(ctx, "Main Area = entire viewport,\nWork Area = entire viewport minus sections used by the main menu bars, task bars etc.\n\nEnable the main-menu bar in Examples menu to see the difference.");

        ImGui::CheckboxFlags(ctx, "ImGuiWindowFlags_NoBackground", &flags, ImGuiWindowFlags_NoBackground);
        ImGui::CheckboxFlags(ctx, "ImGuiWindowFlags_NoDecoration", &flags, ImGuiWindowFlags_NoDecoration);
        ImGui::Indent(ctx);
        ImGui::CheckboxFlags(ctx, "ImGuiWindowFlags_NoTitleBar", &flags, ImGuiWindowFlags_NoTitleBar);
        ImGui::CheckboxFlags(ctx, "ImGuiWindowFlags_NoCollapse", &flags, ImGuiWindowFlags_NoCollapse);
        ImGui::CheckboxFlags(ctx, "ImGuiWindowFlags_NoScrollbar", &flags, ImGuiWindowFlags_NoScrollbar);
        ImGui::Unindent(ctx);

        if (p_open && ImGui::Button(ctx, "Close this window"))
            *p_open = false;
    }
    ImGui::End(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Manipulating Window Titles / ShowExampleAppWindowTitles()
//-----------------------------------------------------------------------------

// Demonstrate the use of "##" and "###" in identifiers to manipulate ID generation.
// This applies to all regular items as well.
// Read FAQ section "How can I have multiple widgets with the same label?" for details.
static void ShowExampleAppWindowTitles(ImGuiContext* ctx, bool*)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport(ctx);
    const ImVec2 base_pos = viewport->Pos;

    // By default, Windows are uniquely identified by their title.
    // You can use the "##" and "###" markers to manipulate the display/ID.

    // Using "##" to display same title but have unique identifier.
    ImGui::SetNextWindowPos(ctx, ImVec2(base_pos.x + 100, base_pos.y + 100), ImGuiCond_FirstUseEver);
    ImGui::Begin(ctx, "Same title as another window##1");
    IMGUI_DEMO_MARKER("Examples/Manipulating window titles");
    ImGui::Text(ctx, "This is window 1.\nMy title is the same as window 2, but my identifier is unique.");
    ImGui::End(ctx);

    ImGui::SetNextWindowPos(ctx, ImVec2(base_pos.x + 100, base_pos.y + 200), ImGuiCond_FirstUseEver);
    ImGui::Begin(ctx, "Same title as another window##2");
    ImGui::Text(ctx, "This is window 2.\nMy title is the same as window 1, but my identifier is unique.");
    ImGui::End(ctx);

    // Using "###" to display a changing title but keep a static identifier "AnimatedTitle"
    char buf[128];
    sprintf(buf, "Animated title %c %d###AnimatedTitle", "|/-\\"[(int)(ImGui::GetTime(ctx) / 0.25f) & 3], ImGui::GetFrameCount(ctx));
    ImGui::SetNextWindowPos(ctx, ImVec2(base_pos.x + 100, base_pos.y + 300), ImGuiCond_FirstUseEver);
    ImGui::Begin(ctx, buf);
    ImGui::Text(ctx, "This window has a changing title.");
    ImGui::End(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Custom Rendering using ImDrawList API / ShowExampleAppCustomRendering()
//-----------------------------------------------------------------------------

// Demonstrate using the low-level ImDrawList to draw custom shapes.
static void ShowExampleAppCustomRendering(ImGuiContext* ctx, bool* p_open)
{
    if (!ImGui::Begin(ctx, "Example: Custom rendering", p_open))
    {
        ImGui::End(ctx);
        return;
    }
    IMGUI_DEMO_MARKER("Examples/Custom Rendering");

    // Tip: If you do a lot of custom rendering, you probably want to use your own geometrical types and benefit of
    // overloaded operators, etc. Define IM_VEC2_CLASS_EXTRA in imconfig.h to create implicit conversions between your
    // types and ImVec2/ImVec4. Dear ImGui defines overloaded operators but they are internal to imgui.cpp and not
    // exposed outside (to avoid messing with your types) In this example we are not using the maths operators!

    if (ImGui::BeginTabBar(ctx, "##TabBar"))
    {
        if (ImGui::BeginTabItem(ctx, "Primitives"))
        {
            ImGui::PushItemWidth(ctx, -ImGui::GetFontSize(ctx) * 15);
            ImDrawList* draw_list = ImGui::GetWindowDrawList(ctx);

            // Draw gradients
            // (note that those are currently exacerbating our sRGB/Linear issues)
            // Calling ImGui::GetColorU32() multiplies the given colors by the current Style Alpha, but you may pass the IM_COL32() directly as well..
            ImGui::Text(ctx, "Gradients");
            ImVec2 gradient_size = ImVec2(ImGui::CalcItemWidth(ctx), ImGui::GetFrameHeight(ctx));
            {
                ImVec2 p0 = ImGui::GetCursorScreenPos(ctx);
                ImVec2 p1 = ImVec2(p0.x + gradient_size.x, p0.y + gradient_size.y);
                ImU32 col_a = ImGui::GetColorU32(ctx, IM_COL32(0, 0, 0, 255));
                ImU32 col_b = ImGui::GetColorU32(ctx, IM_COL32(255, 255, 255, 255));
                draw_list->AddRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
                ImGui::InvisibleButton(ctx, "##gradient1", gradient_size);
            }
            {
                ImVec2 p0 = ImGui::GetCursorScreenPos(ctx);
                ImVec2 p1 = ImVec2(p0.x + gradient_size.x, p0.y + gradient_size.y);
                ImU32 col_a = ImGui::GetColorU32(ctx, IM_COL32(0, 255, 0, 255));
                ImU32 col_b = ImGui::GetColorU32(ctx, IM_COL32(255, 0, 0, 255));
                draw_list->AddRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
                ImGui::InvisibleButton(ctx, "##gradient2", gradient_size);
            }

            // Draw a bunch of primitives
            ImGui::Text(ctx, "All primitives");
            static float sz = 36.0f;
            static float thickness = 3.0f;
            static int ngon_sides = 6;
            static bool circle_segments_override = false;
            static int circle_segments_override_v = 12;
            static bool curve_segments_override = false;
            static int curve_segments_override_v = 8;
            static ImVec4 colf = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
            ImGui::DragFloat(ctx, "Size", &sz, 0.2f, 2.0f, 100.0f, "%.0f");
            ImGui::DragFloat(ctx, "Thickness", &thickness, 0.05f, 1.0f, 8.0f, "%.02f");
            ImGui::SliderInt(ctx, "N-gon sides", &ngon_sides, 3, 12);
            ImGui::Checkbox(ctx, "##circlesegmentoverride", &circle_segments_override);
            ImGui::SameLine(ctx, 0.0f, ImGui::GetStyle(ctx).ItemInnerSpacing.x);
            circle_segments_override |= ImGui::SliderInt(ctx, "Circle segments override", &circle_segments_override_v, 3, 40);
            ImGui::Checkbox(ctx, "##curvessegmentoverride", &curve_segments_override);
            ImGui::SameLine(ctx, 0.0f, ImGui::GetStyle(ctx).ItemInnerSpacing.x);
            curve_segments_override |= ImGui::SliderInt(ctx, "Curves segments override", &curve_segments_override_v, 3, 40);
            ImGui::ColorEdit4(ctx, "Color", &colf.x);

            const ImVec2 p = ImGui::GetCursorScreenPos(ctx);
            const ImU32 col = ImColor(colf);
            const float spacing = 10.0f;
            const ImDrawFlags corners_tl_br = ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight;
            const float rounding = sz / 5.0f;
            const int circle_segments = circle_segments_override ? circle_segments_override_v : 0;
            const int curve_segments = curve_segments_override ? curve_segments_override_v : 0;
            float x = p.x + 4.0f;
            float y = p.y + 4.0f;
            for (int n = 0; n < 2; n++)
            {
                // First line uses a thickness of 1.0f, second line uses the configurable thickness
                float th = (n == 0) ? 1.0f : thickness;
                draw_list->AddNgon(ImVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, col, ngon_sides, th);                 x += sz + spacing;  // N-gon
                draw_list->AddCircle(ImVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, col, circle_segments, th);          x += sz + spacing;  // Circle
                draw_list->AddEllipse(ImVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, sz*0.3f, col, -0.3f, circle_segments, th); x += sz + spacing;	// Ellipse
                draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col, 0.0f, ImDrawFlags_None, th);          x += sz + spacing;  // Square
                draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col, rounding, ImDrawFlags_None, th);      x += sz + spacing;  // Square with all rounded corners
                draw_list->AddRect(ImVec2(x, y), ImVec2(x + sz, y + sz), col, rounding, corners_tl_br, th);         x += sz + spacing;  // Square with two rounded corners
                draw_list->AddTriangle(ImVec2(x+sz*0.5f,y), ImVec2(x+sz, y+sz-0.5f), ImVec2(x, y+sz-0.5f), col, th);x += sz + spacing;  // Triangle
                //draw_list->AddTriangle(ImVec2(x+sz*0.2f,y), ImVec2(x, y+sz-0.5f), ImVec2(x+sz*0.4f, y+sz-0.5f), col, th);x+= sz*0.4f + spacing; // Thin triangle
                draw_list->AddLine(ImVec2(x, y), ImVec2(x + sz, y), col, th);                                       x += sz + spacing;  // Horizontal line (note: drawing a filled rectangle will be faster!)
                draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + sz), col, th);                                       x += spacing;       // Vertical line (note: drawing a filled rectangle will be faster!)
                draw_list->AddLine(ImVec2(x, y), ImVec2(x + sz, y + sz), col, th);                                  x += sz + spacing;  // Diagonal line

                // Quadratic Bezier Curve (3 control points)
                ImVec2 cp3[3] = { ImVec2(x, y + sz * 0.6f), ImVec2(x + sz * 0.5f, y - sz * 0.4f), ImVec2(x + sz, y + sz) };
                draw_list->AddBezierQuadratic(cp3[0], cp3[1], cp3[2], col, th, curve_segments); x += sz + spacing;

                // Cubic Bezier Curve (4 control points)
                ImVec2 cp4[4] = { ImVec2(x, y), ImVec2(x + sz * 1.3f, y + sz * 0.3f), ImVec2(x + sz - sz * 1.3f, y + sz - sz * 0.3f), ImVec2(x + sz, y + sz) };
                draw_list->AddBezierCubic(cp4[0], cp4[1], cp4[2], cp4[3], col, th, curve_segments);

                x = p.x + 4;
                y += sz + spacing;
            }
            draw_list->AddNgonFilled(ImVec2(x + sz * 0.5f, y + sz * 0.5f), sz * 0.5f, col, ngon_sides);             x += sz + spacing;  // N-gon
            draw_list->AddCircleFilled(ImVec2(x + sz * 0.5f, y + sz * 0.5f), sz * 0.5f, col, circle_segments);      x += sz + spacing;  // Circle
            draw_list->AddEllipseFilled(ImVec2(x + sz * 0.5f, y + sz * 0.5f), sz * 0.5f, sz * 0.3f, col, -0.3f, circle_segments); x += sz + spacing;// Ellipse
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col);                                    x += sz + spacing;  // Square
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col, 10.0f);                             x += sz + spacing;  // Square with all rounded corners
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + sz), col, 10.0f, corners_tl_br);              x += sz + spacing;  // Square with two rounded corners
            draw_list->AddTriangleFilled(ImVec2(x+sz*0.5f,y), ImVec2(x+sz, y+sz-0.5f), ImVec2(x, y+sz-0.5f), col);  x += sz + spacing;  // Triangle
            //draw_list->AddTriangleFilled(ImVec2(x+sz*0.2f,y), ImVec2(x, y+sz-0.5f), ImVec2(x+sz*0.4f, y+sz-0.5f), col); x += sz*0.4f + spacing; // Thin triangle
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + sz, y + thickness), col);                             x += sz + spacing;  // Horizontal line (faster than AddLine, but only handle integer thickness)
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + thickness, y + sz), col);                             x += spacing * 2.0f;// Vertical line (faster than AddLine, but only handle integer thickness)
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + 1, y + 1), col);                                      x += sz;            // Pixel (faster than AddLine)
            draw_list->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + sz, y + sz), IM_COL32(0, 0, 0, 255), IM_COL32(255, 0, 0, 255), IM_COL32(255, 255, 0, 255), IM_COL32(0, 255, 0, 255));

            ImGui::Dummy(ctx, ImVec2((sz + spacing) * 11.2f, (sz + spacing) * 3.0f));
            ImGui::PopItemWidth(ctx);
            ImGui::EndTabItem(ctx);
        }

        if (ImGui::BeginTabItem(ctx, "Canvas"))
        {
            static ImVector<ImVec2> points;
            static ImVec2 scrolling(0.0f, 0.0f);
            static bool opt_enable_grid = true;
            static bool opt_enable_context_menu = true;
            static bool adding_line = false;

            ImGui::Checkbox(ctx, "Enable grid", &opt_enable_grid);
            ImGui::Checkbox(ctx, "Enable context menu", &opt_enable_context_menu);
            ImGui::Text(ctx, "Mouse Left: drag to add lines,\nMouse Right: drag to scroll, click for context menu.");

            // Typically you would use a BeginChild()/EndChild() pair to benefit from a clipping region + own scrolling.
            // Here we demonstrate that this can be replaced by simple offsetting + custom drawing + PushClipRect/PopClipRect() calls.
            // To use a child window instead we could use, e.g:
            //      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));      // Disable padding
            //      ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(50, 50, 50, 255));  // Set a background color
            //      ImGui::BeginChild("canvas", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Border, ImGuiWindowFlags_NoMove);
            //      ImGui::PopStyleColor();
            //      ImGui::PopStyleVar();
            //      [...]
            //      ImGui::EndChild();

            // Using InvisibleButton() as a convenience 1) it will advance the layout cursor and 2) allows us to use IsItemHovered()/IsItemActive()
            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos(ctx);      // ImDrawList API uses screen coordinates!
            ImVec2 canvas_sz = ImGui::GetContentRegionAvail(ctx);   // Resize canvas to what's available
            if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
            if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
            ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

            // Draw border and background color
            ImGuiIO& io = ImGui::GetIO(ctx);
            ImDrawList* draw_list = ImGui::GetWindowDrawList(ctx);
            draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
            draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

            // This will catch our interactions
            ImGui::InvisibleButton(ctx, "canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool is_hovered = ImGui::IsItemHovered(ctx); // Hovered
            const bool is_active = ImGui::IsItemActive(ctx);   // Held
            const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
            const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

            // Add first and second point
            if (is_hovered && !adding_line && ImGui::IsMouseClicked(ctx, ImGuiMouseButton_Left))
            {
                points.push_back(mouse_pos_in_canvas);
                points.push_back(mouse_pos_in_canvas);
                adding_line = true;
            }
            if (adding_line)
            {
                points.back() = mouse_pos_in_canvas;
                if (!ImGui::IsMouseDown(ctx, ImGuiMouseButton_Left))
                    adding_line = false;
            }

            // Pan (we use a zero mouse threshold when there's no context menu)
            // You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
            const float mouse_threshold_for_pan = opt_enable_context_menu ? -1.0f : 0.0f;
            if (is_active && ImGui::IsMouseDragging(ctx, ImGuiMouseButton_Right, mouse_threshold_for_pan))
            {
                scrolling.x += io.MouseDelta.x;
                scrolling.y += io.MouseDelta.y;
            }

            // Context menu (under default mouse threshold)
            ImVec2 drag_delta = ImGui::GetMouseDragDelta(ctx, ImGuiMouseButton_Right);
            if (opt_enable_context_menu && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
                ImGui::OpenPopupOnItemClick(ctx, "context", ImGuiPopupFlags_MouseButtonRight);
            if (ImGui::BeginPopup(ctx, "context"))
            {
                if (adding_line)
                    points.resize(points.size() - 2);
                adding_line = false;
                if (ImGui::MenuItem(ctx, "Remove one", NULL, false, points.Size > 0)) { points.resize(points.size() - 2); }
                if (ImGui::MenuItem(ctx, "Remove all", NULL, false, points.Size > 0)) { points.clear(); }
                ImGui::EndPopup(ctx);
            }

            // Draw grid + all lines in the canvas
            draw_list->PushClipRect(canvas_p0, canvas_p1, true);
            if (opt_enable_grid)
            {
                const float GRID_STEP = 64.0f;
                for (float x = fmodf(scrolling.x, GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
                    draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(200, 200, 200, 40));
                for (float y = fmodf(scrolling.y, GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
                    draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(200, 200, 200, 40));
            }
            for (int n = 0; n < points.Size; n += 2)
                draw_list->AddLine(ImVec2(origin.x + points[n].x, origin.y + points[n].y), ImVec2(origin.x + points[n + 1].x, origin.y + points[n + 1].y), IM_COL32(255, 255, 0, 255), 2.0f);
            draw_list->PopClipRect();

            ImGui::EndTabItem(ctx);
        }

        if (ImGui::BeginTabItem(ctx, "BG/FG draw lists"))
        {
            static bool draw_bg = true;
            static bool draw_fg = true;
            ImGui::Checkbox(ctx, "Draw in Background draw list", &draw_bg);
            ImGui::SameLine(ctx); HelpMarker(ctx, "The Background draw list will be rendered below every Dear ImGui windows.");
            ImGui::Checkbox(ctx, "Draw in Foreground draw list", &draw_fg);
            ImGui::SameLine(ctx); HelpMarker(ctx, "The Foreground draw list will be rendered over every Dear ImGui windows.");
            ImVec2 window_pos = ImGui::GetWindowPos(ctx);
            ImVec2 window_size = ImGui::GetWindowSize(ctx);
            ImVec2 window_center = ImVec2(window_pos.x + window_size.x * 0.5f, window_pos.y + window_size.y * 0.5f);
            if (draw_bg)
                ImGui::GetBackgroundDrawList(ctx)->AddCircle(window_center, window_size.x * 0.6f, IM_COL32(255, 0, 0, 200), 0, 10 + 4);
            if (draw_fg)
                ImGui::GetForegroundDrawList(ctx)->AddCircle(window_center, window_size.y * 0.6f, IM_COL32(0, 255, 0, 200), 0, 10);
            ImGui::EndTabItem(ctx);
        }

        // Demonstrate out-of-order rendering via channels splitting
        // We use functions in ImDrawList as each draw list contains a convenience splitter,
        // but you can also instantiate your own ImDrawListSplitter if you need to nest them.
        if (ImGui::BeginTabItem(ctx, "Draw Channels"))
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList(ctx);
            {
                ImGui::Text(ctx, "Blue shape is drawn first: appears in back");
                ImGui::Text(ctx, "Red shape is drawn after: appears in front");
                ImVec2 p0 = ImGui::GetCursorScreenPos(ctx);
                draw_list->AddRectFilled(ImVec2(p0.x, p0.y), ImVec2(p0.x + 50, p0.y + 50), IM_COL32(0, 0, 255, 255)); // Blue
                draw_list->AddRectFilled(ImVec2(p0.x + 25, p0.y + 25), ImVec2(p0.x + 75, p0.y + 75), IM_COL32(255, 0, 0, 255)); // Red
                ImGui::Dummy(ctx, ImVec2(75, 75));
            }
            ImGui::Separator(ctx);
            {
                ImGui::Text(ctx, "Blue shape is drawn first, into channel 1: appears in front");
                ImGui::Text(ctx, "Red shape is drawn after, into channel 0: appears in back");
                ImVec2 p1 = ImGui::GetCursorScreenPos(ctx);

                // Create 2 channels and draw a Blue shape THEN a Red shape.
                // You can create any number of channels. Tables API use 1 channel per column in order to better batch draw calls.
                draw_list->ChannelsSplit(2);
                draw_list->ChannelsSetCurrent(1);
                draw_list->AddRectFilled(ImVec2(p1.x, p1.y), ImVec2(p1.x + 50, p1.y + 50), IM_COL32(0, 0, 255, 255)); // Blue
                draw_list->ChannelsSetCurrent(0);
                draw_list->AddRectFilled(ImVec2(p1.x + 25, p1.y + 25), ImVec2(p1.x + 75, p1.y + 75), IM_COL32(255, 0, 0, 255)); // Red

                // Flatten/reorder channels. Red shape is in channel 0 and it appears below the Blue shape in channel 1.
                // This works by copying draw indices only (vertices are not copied).
                draw_list->ChannelsMerge();
                ImGui::Dummy(ctx, ImVec2(75, 75));
                ImGui::Text(ctx, "After reordering, contents of channel 0 appears below channel 1.");
            }
            ImGui::EndTabItem(ctx);
        }

        ImGui::EndTabBar(ctx);
    }

    ImGui::End(ctx);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Documents Handling / ShowExampleAppDocuments()
//-----------------------------------------------------------------------------

// Simplified structure to mimic a Document model
struct MyDocument
{
    const char* Name;       // Document title
    bool        Open;       // Set when open (we keep an array of all available documents to simplify demo code!)
    bool        OpenPrev;   // Copy of Open from last update.
    bool        Dirty;      // Set when the document has been modified
    bool        WantClose;  // Set when the document
    ImVec4      Color;      // An arbitrary variable associated to the document

    MyDocument(const char* name, bool open = true, const ImVec4& color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f))
    {
        Name = name;
        Open = OpenPrev = open;
        Dirty = false;
        WantClose = false;
        Color = color;
    }
    void DoOpen()       { Open = true; }
    void DoQueueClose() { WantClose = true; }
    void DoForceClose() { Open = false; Dirty = false; }
    void DoSave()       { Dirty = false; }

    // Display placeholder contents for the Document
    static void DisplayContents(ImGuiContext* ctx, MyDocument* doc)
    {
        ImGui::PushID(ctx, doc);
        ImGui::Text(ctx, "Document \"%s\"", doc->Name);
        ImGui::PushStyleColor(ctx, ImGuiCol_Text, doc->Color);
        ImGui::TextWrapped(ctx, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
        ImGui::PopStyleColor(ctx);
        if (ImGui::Button(ctx, "Modify", ImVec2(100, 0)))
            doc->Dirty = true;
        ImGui::SameLine(ctx);
        if (ImGui::Button(ctx, "Save", ImVec2(100, 0)))
            doc->DoSave();
        ImGui::ColorEdit3(ctx, "color", &doc->Color.x);  // Useful to test drag and drop and hold-dragged-to-open-tab behavior.
        ImGui::PopID(ctx);
    }

    // Display context menu for the Document
    static void DisplayContextMenu(ImGuiContext* ctx, MyDocument* doc)
    {
        if (!ImGui::BeginPopupContextItem(ctx))
            return;

        char buf[256];
        sprintf(buf, "Save %s", doc->Name);
        if (ImGui::MenuItem(ctx, buf, "CTRL+S", false, doc->Open))
            doc->DoSave();
        if (ImGui::MenuItem(ctx, "Close", "CTRL+W", false, doc->Open))
            doc->DoQueueClose();
        ImGui::EndPopup(ctx);
    }
};

struct ExampleAppDocuments
{
    ImVector<MyDocument> Documents;

    ExampleAppDocuments()
    {
        Documents.push_back(MyDocument("Lettuce",             true,  ImVec4(0.4f, 0.8f, 0.4f, 1.0f)));
        Documents.push_back(MyDocument("Eggplant",            true,  ImVec4(0.8f, 0.5f, 1.0f, 1.0f)));
        Documents.push_back(MyDocument("Carrot",              true,  ImVec4(1.0f, 0.8f, 0.5f, 1.0f)));
        Documents.push_back(MyDocument("Tomato",              false, ImVec4(1.0f, 0.3f, 0.4f, 1.0f)));
        Documents.push_back(MyDocument("A Rather Long Title", false));
        Documents.push_back(MyDocument("Some Document",       false));
    }
};

// [Optional] Notify the system of Tabs/Windows closure that happened outside the regular tab interface.
// If a tab has been closed programmatically (aka closed from another source such as the Checkbox() in the demo,
// as opposed to clicking on the regular tab closing button) and stops being submitted, it will take a frame for
// the tab bar to notice its absence. During this frame there will be a gap in the tab bar, and if the tab that has
// disappeared was the selected one, the tab bar will report no selected tab during the frame. This will effectively
// give the impression of a flicker for one frame.
// We call SetTabItemClosed() to manually notify the Tab Bar or Docking system of removed tabs to avoid this glitch.
// Note that this completely optional, and only affect tab bars with the ImGuiTabBarFlags_Reorderable flag.
static void NotifyOfDocumentsClosedElsewhere(ImGuiContext* ctx, ExampleAppDocuments& app)
{
    for (MyDocument& doc : app.Documents)
    {
        if (!doc.Open && doc.OpenPrev)
            ImGui::SetTabItemClosed(ctx, doc.Name);
        doc.OpenPrev = doc.Open;
    }
}

void ShowExampleAppDocuments(ImGuiContext* ctx, bool* p_open)
{
    static ExampleAppDocuments app;

    // Options
    static bool opt_reorderable = true;
    static ImGuiTabBarFlags opt_fitting_flags = ImGuiTabBarFlags_FittingPolicyDefault_;

    bool window_contents_visible = ImGui::Begin(ctx, "Example: Documents", p_open, ImGuiWindowFlags_MenuBar);
    if (!window_contents_visible)
    {
        ImGui::End(ctx);
        return;
    }

    // Menu
    if (ImGui::BeginMenuBar(ctx))
    {
        if (ImGui::BeginMenu(ctx, "File"))
        {
            int open_count = 0;
            for (MyDocument& doc : app.Documents)
                open_count += doc.Open ? 1 : 0;

            if (ImGui::BeginMenu(ctx, "Open", open_count < app.Documents.Size))
            {
                for (MyDocument& doc : app.Documents)
                    if (!doc.Open && ImGui::MenuItem(ctx, doc.Name))
                        doc.DoOpen();
                ImGui::EndMenu(ctx);
            }
            if (ImGui::MenuItem(ctx, "Close All Documents", NULL, false, open_count > 0))
                for (MyDocument& doc : app.Documents)
                    doc.DoQueueClose();
            if (ImGui::MenuItem(ctx, "Exit", "Ctrl+F4") && p_open)
                *p_open = false;
            ImGui::EndMenu(ctx);
        }
        ImGui::EndMenuBar(ctx);
    }

    // [Debug] List documents with one checkbox for each
    for (int doc_n = 0; doc_n < app.Documents.Size; doc_n++)
    {
        MyDocument& doc = app.Documents[doc_n];
        if (doc_n > 0)
            ImGui::SameLine(ctx);
        ImGui::PushID(ctx, &doc);
        if (ImGui::Checkbox(ctx, doc.Name, &doc.Open))
            if (!doc.Open)
                doc.DoForceClose();
        ImGui::PopID(ctx);
    }

    ImGui::Separator(ctx);

    // About the ImGuiWindowFlags_UnsavedDocument / ImGuiTabItemFlags_UnsavedDocument flags.
    // They have multiple effects:
    // - Display a dot next to the title.
    // - Tab is selected when clicking the X close button.
    // - Closure is not assumed (will wait for user to stop submitting the tab).
    //   Otherwise closure is assumed when pressing the X, so if you keep submitting the tab may reappear at end of tab bar.
    //   We need to assume closure by default otherwise waiting for "lack of submission" on the next frame would leave an empty
    //   hole for one-frame, both in the tab-bar and in tab-contents when closing a tab/window.
    //   The rarely used SetTabItemClosed() function is a way to notify of programmatic closure to avoid the one-frame hole.

    // Submit Tab Bar and Tabs
    {
        ImGuiTabBarFlags tab_bar_flags = (opt_fitting_flags) | (opt_reorderable ? ImGuiTabBarFlags_Reorderable : 0);
        if (ImGui::BeginTabBar(ctx, "##tabs", tab_bar_flags))
        {
            if (opt_reorderable)
                NotifyOfDocumentsClosedElsewhere(ctx, app);

            // [DEBUG] Stress tests
            //if ((ImGui::GetFrameCount() % 30) == 0) docs[1].Open ^= 1;            // [DEBUG] Automatically show/hide a tab. Test various interactions e.g. dragging with this on.
            //if (ImGui::GetIO().KeyCtrl) ImGui::SetTabItemSelected(docs[1].Name);  // [DEBUG] Test SetTabItemSelected(), probably not very useful as-is anyway..

            // Submit Tabs
            for (MyDocument& doc : app.Documents)
            {
                if (!doc.Open)
                    continue;

                ImGuiTabItemFlags tab_flags = (doc.Dirty ? ImGuiTabItemFlags_UnsavedDocument : 0);
                bool visible = ImGui::BeginTabItem(ctx, doc.Name, &doc.Open, tab_flags);

                // Cancel attempt to close when unsaved add to save queue so we can display a popup.
                if (!doc.Open && doc.Dirty)
                {
                    doc.Open = true;
                    doc.DoQueueClose();
                }

                MyDocument::DisplayContextMenu(ctx, &doc);
                if (visible)
                {
                    MyDocument::DisplayContents(ctx, &doc);
                    ImGui::EndTabItem(ctx);
                }
            }

            ImGui::EndTabBar(ctx);
        }
    }

    // Update closing queue
    static ImVector<MyDocument*> close_queue;
    if (close_queue.empty())
    {
        // Close queue is locked once we started a popup
        for (MyDocument& doc : app.Documents)
            if (doc.WantClose)
            {
                doc.WantClose = false;
                close_queue.push_back(&doc);
            }
    }

    // Display closing confirmation UI
    if (!close_queue.empty())
    {
        int close_queue_unsaved_documents = 0;
        for (int n = 0; n < close_queue.Size; n++)
            if (close_queue[n]->Dirty)
                close_queue_unsaved_documents++;

        if (close_queue_unsaved_documents == 0)
        {
            // Close documents when all are unsaved
            for (int n = 0; n < close_queue.Size; n++)
                close_queue[n]->DoForceClose();
            close_queue.clear();
        }
        else
        {
            if (!ImGui::IsPopupOpen(ctx, "Save?"))
                ImGui::OpenPopup(ctx, "Save?");
            if (ImGui::BeginPopupModal(ctx, "Save?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text(ctx, "Save change to the following items?");
                float item_height = ImGui::GetTextLineHeightWithSpacing(ctx);
                if (ImGui::BeginChild(ctx, ImGui::GetID(ctx, "frame"), ImVec2(-FLT_MIN, 6.25f * item_height), ImGuiChildFlags_FrameStyle))
                {
                    for (int n = 0; n < close_queue.Size; n++)
                        if (close_queue[n]->Dirty)
                            ImGui::Text(ctx, "%s", close_queue[n]->Name);
                }
                ImGui::EndChild(ctx);

                ImVec2 button_size(ImGui::GetFontSize(ctx) * 7.0f, 0.0f);
                if (ImGui::Button(ctx, "Yes", button_size))
                {
                    for (int n = 0; n < close_queue.Size; n++)
                    {
                        if (close_queue[n]->Dirty)
                            close_queue[n]->DoSave();
                        close_queue[n]->DoForceClose();
                    }
                    close_queue.clear();
                    ImGui::CloseCurrentPopup(ctx);
                }
                ImGui::SameLine(ctx);
                if (ImGui::Button(ctx, "No", button_size))
                {
                    for (int n = 0; n < close_queue.Size; n++)
                        close_queue[n]->DoForceClose();
                    close_queue.clear();
                    ImGui::CloseCurrentPopup(ctx);
                }
                ImGui::SameLine(ctx);
                if (ImGui::Button(ctx, "Cancel", button_size))
                {
                    close_queue.clear();
                    ImGui::CloseCurrentPopup(ctx);
                }
                ImGui::EndPopup(ctx);
            }
        }
    }

    ImGui::End(ctx);
}

// End of Demo code
#else

void ImGui::ShowAboutWindow(ImGuiContext*, bool*) {}
void ImGui::ShowDemoWindow(ImGuiContext*, bool*) {}
void ImGui::ShowUserGuide(ImGuiContext*) {}
void ImGui::ShowStyleEditor(ImGuiContext*, ImGuiStyle*) {}

#endif

#endif // #ifndef IMGUI_DISABLE
