// dear imgui: Platform Backend for GLFW
// This needs to be used along with a Renderer (e.g. OpenGL3, Vulkan, WebGPU..)
// (Info: GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (Requires: GLFW 3.0+. Prefer GLFW 3.3+/3.4+ for full feature support.)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Mouse support. Can discriminate Mouse/TouchScreen/Pen (Windows only).
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy GLFW_KEY_* values are obsolete since 1.87 and not supported since 1.91.5]
//  [X] Platform: Gamepad support. Enable with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors) with GLFW 3.1+. Resizing cursors requires GLFW 3.4+! Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [X] Platform: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.
//  [X] Multiple Dear ImGui contexts support.
// Missing features or Issues:
//  [ ] Platform: Touch events are only correctly identified as Touch on Windows. This create issues with some interactions. GLFW doesn't provide a way to identify touch inputs from mouse inputs, we cannot call io.AddMouseSourceEvent() to identify the source. We provide a Windows-specific workaround.
//  [ ] Platform: Missing ImGuiMouseCursor_Wait and ImGuiMouseCursor_Progress cursors.
//  [ ] Platform: Multi-viewport: Missing ImGuiBackendFlags_HasParentViewport support. The viewport->ParentViewportID field is ignored, and therefore io.ConfigViewportsNoDefaultParent has no effect either.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// About Emscripten support:
// - Emscripten provides its own GLFW (3.2.1) implementation (syntax: "-sUSE_GLFW=3"), but Joystick is broken and several features are not supported (multiple windows, clipboard, timer, etc.)
// - A third-party Emscripten GLFW (3.4.0) implementation (syntax: "--use-port=contrib.glfw3") fixes the Joystick issue and implements all relevant features for the browser.
// See https://github.com/pongasoft/emscripten-glfw/blob/master/docs/Comparison.md for details.

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2025-XX-XX: Platform: Added support for multiple windows via the ImGuiPlatformIO interface.
//  2025-12-12: Added IMGUI_IMPL_GLFW_DISABLE_X11 / IMGUI_IMPL_GLFW_DISABLE_WAYLAND to forcefully disable either.
//  2025-12-10: Avoid repeated glfwSetCursor()/glfwSetInputMode() calls when unnecessary. Lowers overhead for very high framerates (e.g. 10k+ FPS).
//  2025-11-06: Lower minimum requirement to GLFW 3.0. Though a recent version e.g GLFW 3.4 is highly recommended.
//  2025-09-18: Call platform_io.ClearPlatformHandlers() on shutdown.
//  2025-09-15: Content Scales are always reported as 1.0 on Wayland. FramebufferScale are always reported as 1.0 on X11. (#8920, #8921)
//  2025-09-10: [Docking] Improve multi-viewport behavior in tiling WMs on X11 via the ImGui_ImplGlfw_SetWindowFloating() function. Note: using GLFW backend on Linux/BSD etc. requires linking with -lX11. (#8884, #8474, #8289)
//  2025-07-08: Made ImGui_ImplGlfw_GetContentScaleForWindow(), ImGui_ImplGlfw_GetContentScaleForMonitor() helpers return 1.0f on Emscripten and Android platforms, matching macOS logic. (#8742, #8733)
//  2025-06-18: Added support for multiple Dear ImGui contexts. (#8676, #8239, #8069)
//  2025-06-11: Added ImGui_ImplGlfw_GetContentScaleForWindow(GLFWwindow* window) and ImGui_ImplGlfw_GetContentScaleForMonitor(GLFWmonitor* monitor) helper to facilitate making DPI-aware apps.
//  2025-05-15: [Docking] Add Platform_GetWindowFramebufferScale() handler, to allow varying Retina display density on multiple monitors.
//  2025-04-26: [Docking] Disable multi-viewports under Wayland. (#8587)
//  2025-03-10: Map GLFW_KEY_WORLD_1 and GLFW_KEY_WORLD_2 into ImGuiKey_Oem102.
//  2025-03-03: Fixed clipboard handler assertion when using GLFW <= 3.2.1 compiled with asserts enabled.
//  2025-02-21: [Docking] Update monitors and work areas information every frame, as the later may change regardless of monitor changes. (#8415)
//  2024-11-05: [Docking] Added Linux workaround for spurious mouse up events emitted while dragging and creating new viewport. (#3158, #7733, #7922)
//  2024-08-22: Moved some OS/backend related function pointers from ImGuiIO to ImGuiPlatformIO:
//               - io.GetClipboardTextFn    -> platform_io.Platform_GetClipboardTextFn
//               - io.SetClipboardTextFn    -> platform_io.Platform_SetClipboardTextFn
//               - io.PlatformOpenInShellFn -> platform_io.Platform_OpenInShellFn
//  2024-07-31: Added ImGui_ImplGlfw_Sleep() helper function for usage by our examples app, since GLFW doesn't provide one.
//  2024-07-08: *BREAKING* Renamed ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback to ImGui_ImplGlfw_InstallEmscriptenCallbacks(), added GLFWWindow* parameter.
//  2024-07-08: Emscripten: Added support for GLFW3 contrib port (GLFW 3.4.0 features + bug fixes): to enable, replace -sUSE_GLFW=3 with --use-port=contrib.glfw3 (requires emscripten 3.1.59+) (https://github.com/pongasoft/emscripten-glfw)
//  2024-07-02: Emscripten: Added io.PlatformOpenInShellFn() handler for Emscripten versions.
//  2023-12-19: Emscripten: Added ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback() to register canvas selector and auto-resize GLFW window.
//  2023-10-05: Inputs: Added support for extra ImGuiKey values: F13 to F24 function keys.
//  2023-07-18: Inputs: Revert ignoring mouse data on GLFW_CURSOR_DISABLED as it can be used differently. User may set ImGuiConfigFLags_NoMouse if desired. (#5625, #6609)
//  2023-06-12: Accept glfwGetTime() not returning a monotonically increasing value. This seems to happens on some Windows setup when peripherals disconnect, and is likely to also happen on browser + Emscripten. (#6491)
//  2023-04-04: Inputs: Added support for io.AddMouseSourceEvent() to discriminate ImGuiMouseSource_Mouse/ImGuiMouseSource_TouchScreen/ImGuiMouseSource_Pen on Windows ONLY, using a custom WndProc hook. (#2702)
//  2023-03-16: Inputs: Fixed key modifiers handling on secondary viewports (docking branch). Broken on 2023/01/04. (#6248, #6034)
//  2023-03-14: Emscripten: Avoid using glfwGetError() and glfwGetGamepadState() which are not correctly implemented in Emscripten emulation. (#6240)
//  2023-02-03: Emscripten: Registering custom low-level mouse wheel handler to get more accurate scrolling impulses on Emscripten. (#4019, #6096)
//  2023-01-18: Handle unsupported glfwGetVideoMode() call on e.g. Emscripten.
//  2023-01-04: Inputs: Fixed mods state on Linux when using Alt-GR text input (e.g. German keyboard layout), could lead to broken text input. Revert a 2022/01/17 change were we resumed using mods provided by GLFW, turns out they were faulty.
//  2022-11-22: Perform a dummy glfwGetError() read to cancel missing names with glfwGetKeyName(). (#5908)
//  2022-10-18: Perform a dummy glfwGetError() read to cancel missing mouse cursors errors. Using GLFW_VERSION_COMBINED directly. (#5785)
//  2022-10-11: Using 'nullptr' instead of 'NULL' as per our switch to C++11.
//  2022-09-26: Inputs: Renamed ImGuiKey_ModXXX introduced in 1.87 to ImGuiMod_XXX (old names still supported).
//  2022-09-01: Inputs: Honor GLFW_CURSOR_DISABLED by not setting mouse position *EDIT* Reverted 2023-07-18.
//  2022-04-30: Inputs: Fixed ImGui_ImplGlfw_TranslateUntranslatedKey() for lower case letters on OSX.
//  2022-03-23: Inputs: Fixed a regression in 1.87 which resulted in keyboard modifiers events being reported incorrectly on Linux/X11.
//  2022-02-07: Added ImGui_ImplGlfw_InstallCallbacks()/ImGui_ImplGlfw_RestoreCallbacks() helpers to facilitate user installing callbacks after initializing backend.
//  2022-01-26: Inputs: replaced short-lived io.AddKeyModsEvent() (added two weeks ago) with io.AddKeyEvent() using ImGuiKey_ModXXX flags. Sorry for the confusion.
//  2021-01-20: Inputs: calling new io.AddKeyAnalogEvent() for gamepad support, instead of writing directly to io.NavInputs[].
//  2022-01-17: Inputs: calling new io.AddMousePosEvent(), io.AddMouseButtonEvent(), io.AddMouseWheelEvent() API (1.87+).
//  2022-01-17: Inputs: always update key mods next and before key event (not in NewFrame) to fix input queue with very low framerates.
//  2022-01-12: *BREAKING CHANGE*: Now using glfwSetCursorPosCallback(). If you called ImGui_ImplGlfw_InitXXX() with install_callbacks = false, you MUST install glfwSetCursorPosCallback() and forward it to the backend via ImGui_ImplGlfw_CursorPosCallback().
//  2022-01-10: Inputs: calling new io.AddKeyEvent(), io.AddKeyModsEvent() + io.SetKeyEventNativeData() API (1.87+). Support for full ImGuiKey range.
//  2022-01-05: Inputs: Converting GLFW untranslated keycodes back to translated keycodes (in the ImGui_ImplGlfw_KeyCallback() function) in order to match the behavior of every other backend, and facilitate the use of GLFW with lettered-shortcuts API.
//  2021-08-17: *BREAKING CHANGE*: Now using glfwSetWindowFocusCallback() to calling io.AddFocusEvent(). If you called ImGui_ImplGlfw_InitXXX() with install_callbacks = false, you MUST install glfwSetWindowFocusCallback() and forward it to the backend via ImGui_ImplGlfw_WindowFocusCallback().
//  2021-07-29: *BREAKING CHANGE*: Now using glfwSetCursorEnterCallback(). MousePos is correctly reported when the host platform window is hovered but not focused. If you called ImGui_ImplGlfw_InitXXX() with install_callbacks = false, you MUST install glfwSetWindowFocusCallback() callback and forward it to the backend via ImGui_ImplGlfw_CursorEnterCallback().
//  2021-06-29: Reorganized backend to pull data from a single structure to facilitate usage with multiple-contexts (all g_XXXX access changed to bd->XXXX).
//  2020-01-17: Inputs: Disable error callback while assigning mouse cursors because some X11 setup don't have them and it generates errors.
//  2019-12-05: Inputs: Added support for new mouse cursors added in GLFW 3.4+ (resizing cursors, not allowed cursor).
//  2019-10-18: Misc: Previously installed user callbacks are now restored on shutdown.
//  2019-07-21: Inputs: Added mapping for ImGuiKey_KeyPadEnter.
//  2019-05-11: Inputs: Don't filter value from character callback before calling AddInputCharacter().
//  2019-03-12: Misc: Preserve DisplayFramebufferScale when main window is minimized.
//  2018-11-30: Misc: Setting up io.BackendPlatformName so it can be displayed in the About Window.
//  2018-11-07: Inputs: When installing our GLFW callbacks, we save user's previously installed ones - if any - and chain call them.
//  2018-08-01: Inputs: Workaround for Emscripten which doesn't seem to handle focus related calls.
//  2018-06-29: Inputs: Added support for the ImGuiMouseCursor_Hand cursor.
//  2018-06-08: Misc: Extracted imgui_impl_glfw.cpp/.h away from the old combined GLFW+OpenGL/Vulkan examples.
//  2018-03-20: Misc: Setup io.BackendFlags ImGuiBackendFlags_HasMouseCursors flag + honor ImGuiConfigFlags_NoMouseCursorChange flag.
//  2018-02-20: Inputs: Added support for mouse cursors (ImGui::GetMouseCursor() value, passed to glfwSetCursor()).
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2018-02-06: Inputs: Added mapping for ImGuiKey_Space.
//  2018-01-25: Inputs: Added gamepad support if ImGuiConfigFlags_NavEnableGamepad is set.
//  2018-01-25: Inputs: Honoring the io.WantSetMousePos by repositioning the mouse (when using navigation and ImGuiConfigFlags_NavMoveMouse is set).
//  2018-01-20: Inputs: Added Horizontal Mouse Wheel support.
//  2018-01-18: Inputs: Added mapping for ImGuiKey_Insert.
//  2017-08-25: Inputs: MousePos set to -FLT_MAX,-FLT_MAX when mouse is unavailable/missing (instead of -1,-1).
//  2016-10-15: Misc: Added a void* user_data parameter to Clipboard function handlers.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_glfw.h"

// Clang warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning: use of old-style cast
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wexit-time-destructors"  // warning: declaration requires an exit-time destructor     // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. ImGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wglobal-constructors"    // warning: declaration requires a global destructor         // similar to above, not sure what the exact difference is.
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wfloat-equal"              // warning: comparing floating-point with '==' or '!=' is unsafe
#endif

// GLFW
#if !defined(IMGUI_IMPL_GLFW_DISABLE_X11) && (defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__))
#define GLFW_HAS_X11        1
#else
#define GLFW_HAS_X11        0
#endif
#if !defined(IMGUI_IMPL_GLFW_DISABLE_WAYLAND) && (defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__))
#define GLFW_HAS_WAYLAND    1
#else
#define GLFW_HAS_WAYLAND    0
#endif
#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#ifndef GLFW_EXPOSE_NATIVE_WIN32    // for glfwGetWin32Window()
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
#elif defined(__APPLE__)
#ifndef GLFW_EXPOSE_NATIVE_COCOA    // for glfwGetCocoaWindow()
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>
#elif GLFW_HAS_X11
#ifndef GLFW_EXPOSE_NATIVE_X11      // for glfwGetX11Display(), glfwGetX11Window() on Freedesktop (Linux, BSD, etc.)
#define GLFW_EXPOSE_NATIVE_X11
#include <X11/Xatom.h>
#endif
#include <GLFW/glfw3native.h>
#undef Status                   // X11 headers are leaking this.
#endif

#ifndef _WIN32
#include <unistd.h>             // for usleep()
#endif
#include <stdio.h>              // for snprintf()

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#ifdef EMSCRIPTEN_USE_PORT_CONTRIB_GLFW3
#include <GLFW/emscripten_glfw3.h>
#else
#define EMSCRIPTEN_USE_EMBEDDED_GLFW3
#endif
#endif

// We gather version tests as define in order to easily see which features are version-dependent.
#define GLFW_VERSION_COMBINED           (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 + GLFW_VERSION_REVISION)
#define GLFW_HAS_CREATECURSOR           (GLFW_VERSION_COMBINED >= 3100) // 3.1+ glfwCreateCursor()
#define GLFW_HAS_WINDOW_TOPMOST         (GLFW_VERSION_COMBINED >= 3200) // 3.2+ GLFW_FLOATING
#define GLFW_HAS_WINDOW_HOVERED         (GLFW_VERSION_COMBINED >= 3300) // 3.3+ GLFW_HOVERED
#define GLFW_HAS_WINDOW_ALPHA           (GLFW_VERSION_COMBINED >= 3300) // 3.3+ glfwSetWindowOpacity
#define GLFW_HAS_PER_MONITOR_DPI        (GLFW_VERSION_COMBINED >= 3300) // 3.3+ glfwGetMonitorContentScale
#if defined(__EMSCRIPTEN__) || defined(__SWITCH__)                      // no Vulkan support in GLFW for Emscripten or homebrew Nintendo Switch
#define GLFW_HAS_VULKAN                 (0)
#else
#define GLFW_HAS_VULKAN                 (GLFW_VERSION_COMBINED >= 3200) // 3.2+ glfwCreateWindowSurface
#endif
#define GLFW_HAS_FOCUS_WINDOW           (GLFW_VERSION_COMBINED >= 3200) // 3.2+ glfwFocusWindow
#define GLFW_HAS_FOCUS_ON_SHOW          (GLFW_VERSION_COMBINED >= 3300) // 3.3+ GLFW_FOCUS_ON_SHOW
#define GLFW_HAS_MONITOR_WORK_AREA      (GLFW_VERSION_COMBINED >= 3300) // 3.3+ glfwGetMonitorWorkarea
#define GLFW_HAS_OSX_WINDOW_POS_FIX     (GLFW_VERSION_COMBINED >= 3301) // 3.3.1+ Fixed: Resizing window repositions it on MacOS #1553
#ifdef GLFW_RESIZE_NESW_CURSOR          // Let's be nice to people who pulled GLFW between 2019-04-16 (3.4 define) and 2019-11-29 (cursors defines) // FIXME: Remove when GLFW 3.4 is released?
#define GLFW_HAS_NEW_CURSORS            (GLFW_VERSION_COMBINED >= 3400) // 3.4+ GLFW_RESIZE_ALL_CURSOR, GLFW_RESIZE_NESW_CURSOR, GLFW_RESIZE_NWSE_CURSOR, GLFW_NOT_ALLOWED_CURSOR
#else
#define GLFW_HAS_NEW_CURSORS            (0)
#endif
#ifdef GLFW_MOUSE_PASSTHROUGH           // Let's be nice to people who pulled GLFW between 2019-04-16 (3.4 define) and 2020-07-17 (passthrough)
#define GLFW_HAS_MOUSE_PASSTHROUGH      (GLFW_VERSION_COMBINED >= 3400) // 3.4+ GLFW_MOUSE_PASSTHROUGH
#else
#define GLFW_HAS_MOUSE_PASSTHROUGH      (0)
#endif
#define GLFW_HAS_GAMEPAD_API            (GLFW_VERSION_COMBINED >= 3300) // 3.3+ glfwGetGamepadState() new api
#define GLFW_HAS_GETKEYNAME             (GLFW_VERSION_COMBINED >= 3200) // 3.2+ glfwGetKeyName()
#define GLFW_HAS_GETERROR               (GLFW_VERSION_COMBINED >= 3300) // 3.3+ glfwGetError()
#define GLFW_HAS_GETPLATFORM            (GLFW_VERSION_COMBINED >= 3400) // 3.4+ glfwGetPlatform()

// Map GLFWWindow* to ImGuiContext*.
// - Would be simpler if we could use glfwSetWindowUserPointer()/glfwGetWindowUserPointer(), but this is a single and shared resource.
// - Would be simpler if we could use e.g. std::map<> as well. But we don't.
// - This is not particularly optimized as we expect size to be small and queries to be rare.
struct ImGui_ImplGlfw_WindowToContext { GLFWwindow* Window; ImGuiContext* Context; };
static ImVector<ImGui_ImplGlfw_WindowToContext> g_ContextMap;
static void ImGui_ImplGlfw_ContextMap_Add(GLFWwindow* window, ImGuiContext* ctx) { g_ContextMap.push_back(ImGui_ImplGlfw_WindowToContext{ window, ctx }); }
static void ImGui_ImplGlfw_ContextMap_Remove(GLFWwindow* window)                 { for (ImGui_ImplGlfw_WindowToContext& entry : g_ContextMap) if (entry.Window == window) { g_ContextMap.erase_unsorted(&entry); if (g_ContextMap.empty()) g_ContextMap.clear(); return; } }
static ImGuiContext* ImGui_ImplGlfw_ContextMap_Get(GLFWwindow* window)           { for (ImGui_ImplGlfw_WindowToContext& entry : g_ContextMap) if (entry.Window == window) return entry.Context; return nullptr; }

enum GlfwClientApi
{
    GlfwClientApi_OpenGL,
    GlfwClientApi_Vulkan,
    GlfwClientApi_Unknown,  // Anything else fits here.
};

// GLFW data
struct ImGui_ImplGlfw_Data
{
    ImGuiContext*           Context;
    GLFWwindow*             Window;
    GlfwClientApi           ClientApi;
    double                  Time;
    GLFWwindow*             MouseWindow;
#if GLFW_HAS_CREATECURSOR
    GLFWcursor*             MouseCursors[ImGuiMouseCursor_COUNT];
    GLFWcursor*             LastMouseCursor;
#endif
    bool                    MouseIgnoreButtonUpWaitForFocusLoss;
    bool                    MouseIgnoreButtonUp;
    ImVec2                  LastValidMousePos;
    GLFWwindow*             KeyOwnerWindows[GLFW_KEY_LAST];
    bool                    IsWayland;
    bool                    InstalledCallbacks;
    bool                    CallbacksChainForAllWindows;
    char                    BackendPlatformName[32];
#ifdef EMSCRIPTEN_USE_EMBEDDED_GLFW3
    const char*             CanvasSelector;
#endif

    // Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
    GLFWwindowfocusfun      PrevUserCallbackWindowFocus;
    GLFWcursorposfun        PrevUserCallbackCursorPos;
    GLFWcursorenterfun      PrevUserCallbackCursorEnter;
    GLFWmousebuttonfun      PrevUserCallbackMousebutton;
    GLFWscrollfun           PrevUserCallbackScroll;
    GLFWkeyfun              PrevUserCallbackKey;
    GLFWcharfun             PrevUserCallbackChar;
    GLFWmonitorfun          PrevUserCallbackMonitor;
#ifdef _WIN32
    WNDPROC                 PrevWndProc;
#endif

    ImGui_ImplGlfw_Data()   { memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// - Because glfwPollEvents() process all windows and some events may be called outside of it, you will need to register your own callbacks
//   (passing install_callbacks=false in ImGui_ImplGlfw_InitXXX functions), set the current dear imgui context and then call our callbacks.
// - Otherwise we may need to store a GLFWWindow* -> ImGuiContext* map and handle this in the backend, adding a little bit of extra complexity to it.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
namespace ImGui { extern ImGuiIO& GetIO(ImGuiContext*); }
static ImGui_ImplGlfw_Data* ImGui_ImplGlfw_GetBackendData()
{
    // Get data for current context
    return ImGui::GetCurrentContext() ? (ImGui_ImplGlfw_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}
static ImGui_ImplGlfw_Data* ImGui_ImplGlfw_GetBackendData(GLFWwindow* window)
{
    // Get data for a given GLFW window, regardless of current context (since GLFW events are sent together)
    ImGuiContext* ctx = ImGui_ImplGlfw_ContextMap_Get(window);
    return (ImGui_ImplGlfw_Data*)ImGui::GetIO(ctx).BackendPlatformUserData;
}

// Forward Declarations
static void ImGui_ImplGlfw_UpdateMonitors();
static void ImGui_ImplGlfw_InitMultiViewportSupport();
static void ImGui_ImplGlfw_ShutdownMultiViewportSupport();

// Functions
static bool ImGui_ImplGlfw_IsWayland()
{
#if !GLFW_HAS_WAYLAND
    return false;
#elif GLFW_HAS_GETPLATFORM
    return glfwGetPlatform() == GLFW_PLATFORM_WAYLAND;
#else
    const char* version = glfwGetVersionString();
    if (strstr(version, "Wayland") == NULL) // e.g. Ubuntu 22.04 ships with GLFW 3.3.6 compiled without Wayland
        return false;
#ifdef GLFW_EXPOSE_NATIVE_X11
    if (glfwGetX11Display() != NULL)
        return false;
#endif
    return true;
#endif
}

// Not static to allow third-party code to use that if they want to (but undocumented)
ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int keycode, int scancode);
ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int keycode, int scancode)
{
    IM_UNUSED(scancode);
    switch (keycode)
    {
        case GLFW_KEY_TAB: return ImGuiKey_Tab;
        case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
        case GLFW_KEY_UP: return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
        case GLFW_KEY_HOME: return ImGuiKey_Home;
        case GLFW_KEY_END: return ImGuiKey_End;
        case GLFW_KEY_INSERT: return ImGuiKey_Insert;
        case GLFW_KEY_DELETE: return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE: return ImGuiKey_Space;
        case GLFW_KEY_ENTER: return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA: return ImGuiKey_Comma;
        case GLFW_KEY_MINUS: return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD: return ImGuiKey_Period;
        case GLFW_KEY_SLASH: return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
        case GLFW_KEY_WORLD_1: return ImGuiKey_Oem102;
        case GLFW_KEY_WORLD_2: return ImGuiKey_Oem102;
        case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
        case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU: return ImGuiKey_Menu;
        case GLFW_KEY_0: return ImGuiKey_0;
        case GLFW_KEY_1: return ImGuiKey_1;
        case GLFW_KEY_2: return ImGuiKey_2;
        case GLFW_KEY_3: return ImGuiKey_3;
        case GLFW_KEY_4: return ImGuiKey_4;
        case GLFW_KEY_5: return ImGuiKey_5;
        case GLFW_KEY_6: return ImGuiKey_6;
        case GLFW_KEY_7: return ImGuiKey_7;
        case GLFW_KEY_8: return ImGuiKey_8;
        case GLFW_KEY_9: return ImGuiKey_9;
        case GLFW_KEY_A: return ImGuiKey_A;
        case GLFW_KEY_B: return ImGuiKey_B;
        case GLFW_KEY_C: return ImGuiKey_C;
        case GLFW_KEY_D: return ImGuiKey_D;
        case GLFW_KEY_E: return ImGuiKey_E;
        case GLFW_KEY_F: return ImGuiKey_F;
        case GLFW_KEY_G: return ImGuiKey_G;
        case GLFW_KEY_H: return ImGuiKey_H;
        case GLFW_KEY_I: return ImGuiKey_I;
        case GLFW_KEY_J: return ImGuiKey_J;
        case GLFW_KEY_K: return ImGuiKey_K;
        case GLFW_KEY_L: return ImGuiKey_L;
        case GLFW_KEY_M: return ImGuiKey_M;
        case GLFW_KEY_N: return ImGuiKey_N;
        case GLFW_KEY_O: return ImGuiKey_O;
        case GLFW_KEY_P: return ImGuiKey_P;
        case GLFW_KEY_Q: return ImGuiKey_Q;
        case GLFW_KEY_R: return ImGuiKey_R;
        case GLFW_KEY_S: return ImGuiKey_S;
        case GLFW_KEY_T: return ImGuiKey_T;
        case GLFW_KEY_U: return ImGuiKey_U;
        case GLFW_KEY_V: return ImGuiKey_V;
        case GLFW_KEY_W: return ImGuiKey_W;
        case GLFW_KEY_X: return ImGuiKey_X;
        case GLFW_KEY_Y: return ImGuiKey_Y;
        case GLFW_KEY_Z: return ImGuiKey_Z;
        case GLFW_KEY_F1: return ImGuiKey_F1;
        case GLFW_KEY_F2: return ImGuiKey_F2;
        case GLFW_KEY_F3: return ImGuiKey_F3;
        case GLFW_KEY_F4: return ImGuiKey_F4;
        case GLFW_KEY_F5: return ImGuiKey_F5;
        case GLFW_KEY_F6: return ImGuiKey_F6;
        case GLFW_KEY_F7: return ImGuiKey_F7;
        case GLFW_KEY_F8: return ImGuiKey_F8;
        case GLFW_KEY_F9: return ImGuiKey_F9;
        case GLFW_KEY_F10: return ImGuiKey_F10;
        case GLFW_KEY_F11: return ImGuiKey_F11;
        case GLFW_KEY_F12: return ImGuiKey_F12;
        case GLFW_KEY_F13: return ImGuiKey_F13;
        case GLFW_KEY_F14: return ImGuiKey_F14;
        case GLFW_KEY_F15: return ImGuiKey_F15;
        case GLFW_KEY_F16: return ImGuiKey_F16;
        case GLFW_KEY_F17: return ImGuiKey_F17;
        case GLFW_KEY_F18: return ImGuiKey_F18;
        case GLFW_KEY_F19: return ImGuiKey_F19;
        case GLFW_KEY_F20: return ImGuiKey_F20;
        case GLFW_KEY_F21: return ImGuiKey_F21;
        case GLFW_KEY_F22: return ImGuiKey_F22;
        case GLFW_KEY_F23: return ImGuiKey_F23;
        case GLFW_KEY_F24: return ImGuiKey_F24;
        default: return ImGuiKey_None;
    }
}

// X11 does not include current pressed/released modifier key in 'mods' flags submitted by GLFW
// See https://github.com/ocornut/imgui/issues/6034 and https://github.com/glfw/glfw/issues/1630
static void ImGui_ImplGlfw_UpdateKeyModifiers(ImGuiIO& io, GLFWwindow* window)
{
    io.AddKeyEvent(ImGuiMod_Ctrl,  (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Shift, (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)   == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT)   == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Alt,   (glfwGetKey(window, GLFW_KEY_LEFT_ALT)     == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_ALT)     == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Super, (glfwGetKey(window, GLFW_KEY_LEFT_SUPER)   == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_SUPER)   == GLFW_PRESS));
}

static bool ImGui_ImplGlfw_ShouldChainCallback(ImGui_ImplGlfw_Data* bd, GLFWwindow* window)
{
    return bd->CallbacksChainForAllWindows ? true : (window == bd->Window);
}

void ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData(window);
    if (bd->PrevUserCallbackMousebutton != nullptr && ImGui_ImplGlfw_ShouldChainCallback(bd, window))
        bd->PrevUserCallbackMousebutton(window, button, action, mods);

    // Workaround for Linux: ignore mouse up events which are following an focus loss following a viewport creation
    if (bd->MouseIgnoreButtonUp && action == GLFW_RELEASE)
        return;

    ImGuiIO& io = ImGui::GetIO(bd->Context);
    ImGui_ImplGlfw_UpdateKeyModifiers(io, window);
    if (button >= 0 && button < ImGuiMouseButton_COUNT)
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
}

void ImGui_ImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData(window);
    if (bd->PrevUserCallbackScroll != nullptr && ImGui_ImplGlfw_ShouldChainCallback(bd, window))
        bd->PrevUserCallbackScroll(window, xoffset, yoffset);

#ifdef EMSCRIPTEN_USE_EMBEDDED_GLFW3
    // Ignore GLFW events: will be processed in ImGui_ImplEmscripten_WheelCallback().
    return;
#endif

    ImGuiIO& io = ImGui::GetIO(bd->Context);
    io.AddMouseWheelEvent((float)xoffset, (float)yoffset);
}

// FIXME: should this be baked into ImGui_ImplGlfw_KeyToImGuiKey()? then what about the values passed to io.SetKeyEventNativeData()?
static int ImGui_ImplGlfw_TranslateUntranslatedKey(int key, int scancode)
{
#if GLFW_HAS_GETKEYNAME && !defined(EMSCRIPTEN_USE_EMBEDDED_GLFW3)
    // GLFW 3.1+ attempts to "untranslate" keys, which goes the opposite of what every other framework does, making using lettered shortcuts difficult.
    // (It had reasons to do so: namely GLFW is/was more likely to be used for WASD-type game controls rather than lettered shortcuts, but IHMO the 3.1 change could have been done differently)
    // See https://github.com/glfw/glfw/issues/1502 for details.
    // Adding a workaround to undo this (so our keys are translated->untranslated->translated, likely a lossy process).
    // This won't cover edge cases but this is at least going to cover common cases.
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_EQUAL)
        return key;
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(nullptr);
    const char* key_name = glfwGetKeyName(key, scancode);
    glfwSetErrorCallback(prev_error_callback);
#if GLFW_HAS_GETERROR && !defined(EMSCRIPTEN_USE_EMBEDDED_GLFW3) // Eat errors (see #5908)
    (void)glfwGetError(nullptr);
#endif
    if (key_name && key_name[0] != 0 && key_name[1] == 0)
    {
        const char char_names[] = "`-=[]\\,;\'./";
        const int char_keys[] = { GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_MINUS, GLFW_KEY_EQUAL, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_BACKSLASH, GLFW_KEY_COMMA, GLFW_KEY_SEMICOLON, GLFW_KEY_APOSTROPHE, GLFW_KEY_PERIOD, GLFW_KEY_SLASH, 0 };
        IM_ASSERT(IM_ARRAYSIZE(char_names) == IM_ARRAYSIZE(char_keys));
        if (key_name[0] >= '0' && key_name[0] <= '9')               { key = GLFW_KEY_0 + (key_name[0] - '0'); }
        else if (key_name[0] >= 'A' && key_name[0] <= 'Z')          { key = GLFW_KEY_A + (key_name[0] - 'A'); }
        else if (key_name[0] >= 'a' && key_name[0] <= 'z')          { key = GLFW_KEY_A + (key_name[0] - 'a'); }
        else if (const char* p = strchr(char_names, key_name[0]))   { key = char_keys[p - char_names]; }
    }
    // if (action == GLFW_PRESS) printf("key %d scancode %d name '%s'\n", key, scancode, key_name);
#else
    IM_UNUSED(scancode);
#endif
    return key;
}

void ImGui_ImplGlfw_KeyCallback(GLFWwindow* window, int keycode, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData(window);
    if (bd->PrevUserCallbackKey != nullptr && ImGui_ImplGlfw_ShouldChainCallback(bd, window))
        bd->PrevUserCallbackKey(window, keycode, scancode, action, mods);

    if (action != GLFW_PRESS && action != GLFW_RELEASE)
        return;

    ImGuiIO& io = ImGui::GetIO(bd->Context);
    ImGui_ImplGlfw_UpdateKeyModifiers(io, window);

    if (keycode >= 0 && keycode < IM_ARRAYSIZE(bd->KeyOwnerWindows))
        bd->KeyOwnerWindows[keycode] = (action == GLFW_PRESS) ? window : nullptr;

    keycode = ImGui_ImplGlfw_TranslateUntranslatedKey(keycode, scancode);

    ImGuiKey imgui_key = ImGui_ImplGlfw_KeyToImGuiKey(keycode, scancode);
    io.AddKeyEvent(imgui_key, (action == GLFW_PRESS));
    io.SetKeyEventNativeData(imgui_key, keycode, scancode); // To support legacy indexing (<1.87 user code)
}

void ImGui_ImplGlfw_WindowFocusCallback(GLFWwindow* window, int focused)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData(window);
    if (bd->PrevUserCallbackWindowFocus != nullptr && ImGui_ImplGlfw_ShouldChainCallback(bd, window))
        bd->PrevUserCallbackWindowFocus(window, focused);

    // Workaround for Linux: when losing focus with MouseIgnoreButtonUpWaitForFocusLoss set, we will temporarily ignore subsequent Mouse Up events
    bd->MouseIgnoreButtonUp = (bd->MouseIgnoreButtonUpWaitForFocusLoss && focused == 0);
    bd->MouseIgnoreButtonUpWaitForFocusLoss = false;

    ImGuiIO& io = ImGui::GetIO(bd->Context);
    io.AddFocusEvent(focused != 0);
}

void ImGui_ImplGlfw_CursorPosCallback(GLFWwindow* window, double x, double y)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData(window);
    if (bd->PrevUserCallbackCursorPos != nullptr && ImGui_ImplGlfw_ShouldChainCallback(bd, window))
        bd->PrevUserCallbackCursorPos(window, x, y);

    ImGuiIO& io = ImGui::GetIO(bd->Context);
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        int window_x, window_y;
        glfwGetWindowPos(window, &window_x, &window_y);
        x += window_x;
        y += window_y;
    }
    io.AddMousePosEvent((float)x, (float)y);
    bd->LastValidMousePos = ImVec2((float)x, (float)y);
}

// Workaround: X11 seems to send spurious Leave/Enter events which would make us lose our position,
// so we back it up and restore on Leave/Enter (see https://github.com/ocornut/imgui/issues/4984)
void ImGui_ImplGlfw_CursorEnterCallback(GLFWwindow* window, int entered)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData(window);
    if (bd->PrevUserCallbackCursorEnter != nullptr && ImGui_ImplGlfw_ShouldChainCallback(bd, window))
        bd->PrevUserCallbackCursorEnter(window, entered);

    ImGuiIO& io = ImGui::GetIO(bd->Context);
    if (entered)
    {
        bd->MouseWindow = window;
        io.AddMousePosEvent(bd->LastValidMousePos.x, bd->LastValidMousePos.y);
    }
    else if (!entered && bd->MouseWindow == window)
    {
        bd->LastValidMousePos = io.MousePos;
        bd->MouseWindow = nullptr;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }
}

void ImGui_ImplGlfw_CharCallback(GLFWwindow* window, unsigned int c)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData(window);
    if (bd->PrevUserCallbackChar != nullptr && ImGui_ImplGlfw_ShouldChainCallback(bd, window))
        bd->PrevUserCallbackChar(window, c);

    ImGuiIO& io = ImGui::GetIO(bd->Context);
    io.AddInputCharacter(c);
}

void ImGui_ImplGlfw_MonitorCallback(GLFWmonitor*, int)
{
    // This function is technically part of the API even if we stopped using the callback, so leaving it around.
}

#ifdef EMSCRIPTEN_USE_EMBEDDED_GLFW3
static EM_BOOL ImGui_ImplEmscripten_WheelCallback(int, const EmscriptenWheelEvent* ev, void* user_data)
{
    // Mimic Emscripten_HandleWheel() in SDL.
    // Corresponding equivalent in GLFW JS emulation layer has incorrect quantizing preventing small values. See #6096
    ImGui_ImplGlfw_Data* bd = (ImGui_ImplGlfw_Data*)user_data;
    float multiplier = 0.0f;
    if (ev->deltaMode == DOM_DELTA_PIXEL)       { multiplier = 1.0f / 100.0f; } // 100 pixels make up a step.
    else if (ev->deltaMode == DOM_DELTA_LINE)   { multiplier = 1.0f / 3.0f; }   // 3 lines make up a step.
    else if (ev->deltaMode == DOM_DELTA_PAGE)   { multiplier = 80.0f; }         // A page makes up 80 steps.
    float wheel_x = ev->deltaX * -multiplier;
    float wheel_y = ev->deltaY * -multiplier;
    ImGuiIO& io = ImGui::GetIO(bd->Context);
    io.AddMouseWheelEvent(wheel_x, wheel_y);
    //IMGUI_DEBUG_LOG("[Emsc] mode %d dx: %.2f, dy: %.2f, dz: %.2f --> feed %.2f %.2f\n", (int)ev->deltaMode, ev->deltaX, ev->deltaY, ev->deltaZ, wheel_x, wheel_y);
    return EM_TRUE;
}
#endif

#ifdef _WIN32
static LRESULT CALLBACK ImGui_ImplGlfw_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

void ImGui_ImplGlfw_InstallCallbacks(GLFWwindow* window)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData(window);
    IM_ASSERT(bd->InstalledCallbacks == false && "Callbacks already installed!");
    IM_ASSERT(bd->Window == window);

    bd->PrevUserCallbackWindowFocus = glfwSetWindowFocusCallback(window, ImGui_ImplGlfw_WindowFocusCallback);
    bd->PrevUserCallbackCursorEnter = glfwSetCursorEnterCallback(window, ImGui_ImplGlfw_CursorEnterCallback);
    bd->PrevUserCallbackCursorPos = glfwSetCursorPosCallback(window, ImGui_ImplGlfw_CursorPosCallback);
    bd->PrevUserCallbackMousebutton = glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
    bd->PrevUserCallbackScroll = glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
    bd->PrevUserCallbackKey = glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
    bd->PrevUserCallbackChar = glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
    bd->PrevUserCallbackMonitor = glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);
    bd->InstalledCallbacks = true;
}

void ImGui_ImplGlfw_RestoreCallbacks(GLFWwindow* window)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData(window);
    IM_ASSERT(bd->InstalledCallbacks == true && "Callbacks not installed!");
    IM_ASSERT(bd->Window == window);

    glfwSetWindowFocusCallback(window, bd->PrevUserCallbackWindowFocus);
    glfwSetCursorEnterCallback(window, bd->PrevUserCallbackCursorEnter);
    glfwSetCursorPosCallback(window, bd->PrevUserCallbackCursorPos);
    glfwSetMouseButtonCallback(window, bd->PrevUserCallbackMousebutton);
    glfwSetScrollCallback(window, bd->PrevUserCallbackScroll);
    glfwSetKeyCallback(window, bd->PrevUserCallbackKey);
    glfwSetCharCallback(window, bd->PrevUserCallbackChar);
    glfwSetMonitorCallback(bd->PrevUserCallbackMonitor);
    bd->InstalledCallbacks = false;
    bd->PrevUserCallbackWindowFocus = nullptr;
    bd->PrevUserCallbackCursorEnter = nullptr;
    bd->PrevUserCallbackCursorPos = nullptr;
    bd->PrevUserCallbackMousebutton = nullptr;
    bd->PrevUserCallbackScroll = nullptr;
    bd->PrevUserCallbackKey = nullptr;
    bd->PrevUserCallbackChar = nullptr;
    bd->PrevUserCallbackMonitor = nullptr;
}

// Set to 'true' to enable chaining installed callbacks for all windows (including secondary viewports created by backends or by user).
// This is 'false' by default meaning we only chain callbacks for the main viewport.
// We cannot set this to 'true' by default because user callbacks code may be not testing the 'window' parameter of their callback.
// If you set this to 'true' your user callback code will need to make sure you are testing the 'window' parameter.
void ImGui_ImplGlfw_SetCallbacksChainForAllWindows(bool chain_for_all_windows)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    bd->CallbacksChainForAllWindows = chain_for_all_windows;
}

#ifdef __EMSCRIPTEN__
#if EMSCRIPTEN_USE_PORT_CONTRIB_GLFW3 >= 34020240817
void ImGui_ImplGlfw_EmscriptenOpenURL(const char* url) { if (url) emscripten::glfw3::OpenURL(url); }
#else
EM_JS(void, ImGui_ImplGlfw_EmscriptenOpenURL, (const char* url), { url = url ? UTF8ToString(url) : null; if (url) window.open(url, '_blank'); });
#endif
#endif

static bool ImGui_ImplGlfw_Init(GLFWwindow* window, bool install_callbacks, GlfwClientApi client_api)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");
    //printf("GLFW_VERSION: %d.%d.%d (%d)", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION, GLFW_VERSION_COMBINED);

    // Setup backend capabilities flags
    ImGui_ImplGlfw_Data* bd = IM_NEW(ImGui_ImplGlfw_Data)();
    snprintf(bd->BackendPlatformName, sizeof(bd->BackendPlatformName), "imgui_impl_glfw (%d)", GLFW_VERSION_COMBINED);
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = bd->BackendPlatformName;
#if GLFW_HAS_CREATECURSOR
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
#endif
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)

    bool has_viewports = false;
#ifndef __EMSCRIPTEN__
    has_viewports = true;
#if GLFW_HAS_GETPLATFORM
    if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND)
        has_viewports = false;
#endif
    if (has_viewports)
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;  // We can create multi-viewports on the Platform side (optional)
#endif
#if GLFW_HAS_MOUSE_PASSTHROUGH || GLFW_HAS_WINDOW_HOVERED
    io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can call io.AddMouseViewportEvent() with correct data (optional)
#endif

    bd->Context = ImGui::GetCurrentContext();
    bd->Window = window;
    bd->Time = 0.0;
    bd->IsWayland = ImGui_ImplGlfw_IsWayland();
    ImGui_ImplGlfw_ContextMap_Add(window, bd->Context);

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
#if GLFW_VERSION_COMBINED < 3300
    platform_io.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text) { glfwSetClipboardString(ImGui_ImplGlfw_GetBackendData()->Window, text); };
    platform_io.Platform_GetClipboardTextFn = [](ImGuiContext*) { return glfwGetClipboardString(ImGui_ImplGlfw_GetBackendData()->Window); };
#else
    platform_io.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text) { glfwSetClipboardString(nullptr, text); };
    platform_io.Platform_GetClipboardTextFn = [](ImGuiContext*) { return glfwGetClipboardString(nullptr); };
#endif

#ifdef __EMSCRIPTEN__
    platform_io.Platform_OpenInShellFn = [](ImGuiContext*, const char* url) { ImGui_ImplGlfw_EmscriptenOpenURL(url); return true; };
#endif

    // Create mouse cursors
    // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
    // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
    // Missing cursors will return nullptr and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
#if GLFW_HAS_CREATECURSOR
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(nullptr);
    bd->MouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
    glfwSetErrorCallback(prev_error_callback);
#endif
#if GLFW_HAS_GETERROR && !defined(__EMSCRIPTEN__) // Eat errors (see #5908)
    (void)glfwGetError(nullptr);
#endif

    // Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
    if (install_callbacks)
        ImGui_ImplGlfw_InstallCallbacks(window);

    // Update monitor a first time during init
    // (note: monitor callback are broken in GLFW 3.2 and earlier, see github.com/glfw/glfw/issues/784)
    ImGui_ImplGlfw_UpdateMonitors();
    glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);

    // Set platform dependent data in viewport
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void*)bd->Window;
#ifdef _WIN32
    main_viewport->PlatformHandleRaw = glfwGetWin32Window(bd->Window);
#elif defined(__APPLE__)
    main_viewport->PlatformHandleRaw = (void*)glfwGetCocoaWindow(bd->Window);
#else
    IM_UNUSED(main_viewport);
#endif
    if (has_viewports)
        ImGui_ImplGlfw_InitMultiViewportSupport();

    // Windows: register a WndProc hook so we can intercept some messages.
#ifdef _WIN32
    HWND hwnd = (HWND)main_viewport->PlatformHandleRaw;
    ::SetPropA(hwnd, "IMGUI_BACKEND_DATA", bd);
    bd->PrevWndProc = (WNDPROC)::GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
    IM_ASSERT(bd->PrevWndProc != nullptr);
    ::SetWindowLongPtrW((HWND)main_viewport->PlatformHandleRaw, GWLP_WNDPROC, (LONG_PTR)ImGui_ImplGlfw_WndProc);
#endif

    // Emscripten: the same application can run on various platforms, so we detect the Apple platform at runtime
    // to override io.ConfigMacOSXBehaviors from its default (which is always false in Emscripten).
#ifdef __EMSCRIPTEN__
#if EMSCRIPTEN_USE_PORT_CONTRIB_GLFW3 >= 34020240817
    if (emscripten::glfw3::IsRuntimePlatformApple())
    {
        io.ConfigMacOSXBehaviors = true;

        // Due to how the browser (poorly) handles the Meta Key, this line essentially disables repeats when used.
        // This means that Meta + V only registers a single key-press, even if the keys are held.
        // This is a compromise for dealing with this issue in ImGui since ImGui implements key repeat itself.
        // See https://github.com/pongasoft/emscripten-glfw/blob/v3.4.0.20240817/docs/Usage.md#the-problem-of-the-super-key
        emscripten::glfw3::SetSuperPlusKeyTimeouts(10, 10);
    }
#endif
#endif

    bd->ClientApi = client_api;
    return true;
}

bool ImGui_ImplGlfw_InitForOpenGL(GLFWwindow* window, bool install_callbacks)
{
    return ImGui_ImplGlfw_Init(window, install_callbacks, GlfwClientApi_OpenGL);
}

bool ImGui_ImplGlfw_InitForVulkan(GLFWwindow* window, bool install_callbacks)
{
    return ImGui_ImplGlfw_Init(window, install_callbacks, GlfwClientApi_Vulkan);
}

bool ImGui_ImplGlfw_InitForOther(GLFWwindow* window, bool install_callbacks)
{
    return ImGui_ImplGlfw_Init(window, install_callbacks, GlfwClientApi_Unknown);
}

void ImGui_ImplGlfw_Shutdown()
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");

    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    ImGui_ImplGlfw_ShutdownMultiViewportSupport();
    if (bd->InstalledCallbacks)
        ImGui_ImplGlfw_RestoreCallbacks(bd->Window);
#ifdef EMSCRIPTEN_USE_EMBEDDED_GLFW3
    if (bd->CanvasSelector)
        emscripten_set_wheel_callback(bd->CanvasSelector, nullptr, false, nullptr);
#endif
#if GLFW_HAS_CREATECURSOR
    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        glfwDestroyCursor(bd->MouseCursors[cursor_n]);
#endif
    // Windows: restore our WndProc hook
#ifdef _WIN32
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ::SetPropA((HWND)main_viewport->PlatformHandleRaw, "IMGUI_BACKEND_DATA", nullptr);
    ::SetWindowLongPtrW((HWND)main_viewport->PlatformHandleRaw, GWLP_WNDPROC, (LONG_PTR)bd->PrevWndProc);
    bd->PrevWndProc = nullptr;
#endif

    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos | ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_PlatformHasViewports | ImGuiBackendFlags_HasMouseHoveredViewport);
    platform_io.ClearPlatformHandlers();
    ImGui_ImplGlfw_ContextMap_Remove(bd->Window);
    IM_DELETE(bd);
}

static void ImGui_ImplGlfw_UpdateMouseData()
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    ImGuiID mouse_viewport_id = 0;
    const ImVec2 mouse_pos_prev = io.MousePos;
    for (int n = 0; n < platform_io.Viewports.Size; n++)
    {
        ImGuiViewport* viewport = platform_io.Viewports[n];
        GLFWwindow* window = (GLFWwindow*)viewport->PlatformHandle;

#ifdef EMSCRIPTEN_USE_EMBEDDED_GLFW3
        const bool is_window_focused = true;
#else
        const bool is_window_focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
#endif
        if (is_window_focused)
        {
            // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when io.ConfigNavMoveSetMousePos is enabled by user)
            // When multi-viewports are enabled, all Dear ImGui positions are same as OS positions.
            if (io.WantSetMousePos)
                glfwSetCursorPos(window, (double)(mouse_pos_prev.x - viewport->Pos.x), (double)(mouse_pos_prev.y - viewport->Pos.y));

            // (Optional) Fallback to provide mouse position when focused (ImGui_ImplGlfw_CursorPosCallback already provides this when hovered or captured)
            if (bd->MouseWindow == nullptr)
            {
                double mouse_x, mouse_y;
                glfwGetCursorPos(window, &mouse_x, &mouse_y);
                if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                {
                    // Single viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
                    // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
                    int window_x, window_y;
                    glfwGetWindowPos(window, &window_x, &window_y);
                    mouse_x += window_x;
                    mouse_y += window_y;
                }
                bd->LastValidMousePos = ImVec2((float)mouse_x, (float)mouse_y);
                io.AddMousePosEvent((float)mouse_x, (float)mouse_y);
            }
        }

        // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the viewport the OS mouse cursor is hovering.
        // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, Dear imGui will ignore this field and infer the information using its flawed heuristic.
        // - [X] GLFW >= 3.3 backend ON WINDOWS ONLY does correctly ignore viewports with the _NoInputs flag (since we implement hit via our WndProc hook)
        //       On other platforms we rely on the library fallbacking to its own search when reporting a viewport with _NoInputs flag.
        // - [!] GLFW <= 3.2 backend CANNOT correctly ignore viewports with the _NoInputs flag, and CANNOT reported Hovered Viewport because of mouse capture.
        //       Some backend are not able to handle that correctly. If a backend report an hovered viewport that has the _NoInputs flag (e.g. when dragging a window
        //       for docking, the viewport has the _NoInputs flag in order to allow us to find the viewport under), then Dear ImGui is forced to ignore the value reported
        //       by the backend, and use its flawed heuristic to guess the viewport behind.
        // - [X] GLFW backend correctly reports this regardless of another viewport behind focused and dragged from (we need this to find a useful drag and drop target).
        // FIXME: This is currently only correct on Win32. See what we do below with the WM_NCHITTEST, missing an equivalent for other systems.
        // See https://github.com/glfw/glfw/issues/1236 if you want to help in making this a GLFW feature.
#if GLFW_HAS_MOUSE_PASSTHROUGH
        const bool window_no_input = (viewport->Flags & ImGuiViewportFlags_NoInputs) != 0;
        glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, window_no_input);
#endif
#if GLFW_HAS_MOUSE_PASSTHROUGH || GLFW_HAS_WINDOW_HOVERED
        if (glfwGetWindowAttrib(window, GLFW_HOVERED))
            mouse_viewport_id = viewport->ID;
#else
        // We cannot use bd->MouseWindow maintained from CursorEnter/Leave callbacks, because it is locked to the window capturing mouse.
#endif
    }

    if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
        io.AddMouseViewportEvent(mouse_viewport_id);
}

static void ImGui_ImplGlfw_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(bd->Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int n = 0; n < platform_io.Viewports.Size; n++)
    {
        GLFWwindow* window = (GLFWwindow*)platform_io.Viewports[n]->PlatformHandle;
        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
        {
            if (bd->LastMouseCursor != nullptr)
            {
                // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                bd->LastMouseCursor = nullptr;
            }
        }
        else
        {
            // Show OS mouse cursor
            // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
#if GLFW_HAS_CREATECURSOR
            GLFWcursor* cursor = bd->MouseCursors[imgui_cursor] ? bd->MouseCursors[imgui_cursor] : bd->MouseCursors[ImGuiMouseCursor_Arrow];
            if (bd->LastMouseCursor != cursor)
            {
                glfwSetCursor(window, cursor);
                bd->LastMouseCursor = cursor;
            }
#endif
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

// Update gamepad inputs
static inline float Saturate(float v) { return v < 0.0f ? 0.0f : v  > 1.0f ? 1.0f : v; }
static void ImGui_ImplGlfw_UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0) // FIXME: Technically feeding gamepad shouldn't depend on this now that they are regular inputs, but see #8075
        return;

    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
#if GLFW_HAS_GAMEPAD_API && !defined(EMSCRIPTEN_USE_EMBEDDED_GLFW3)
    GLFWgamepadstate gamepad;
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepad))
        return;
    #define MAP_BUTTON(KEY_NO, BUTTON_NO, _UNUSED)          do { io.AddKeyEvent(KEY_NO, gamepad.buttons[BUTTON_NO] != 0); } while (0)
    #define MAP_ANALOG(KEY_NO, AXIS_NO, _UNUSED, V0, V1)    do { float v = gamepad.axes[AXIS_NO]; v = (v - V0) / (V1 - V0); io.AddKeyAnalogEvent(KEY_NO, v > 0.10f, Saturate(v)); } while (0)
#else
    int axes_count = 0, buttons_count = 0;
    const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
    const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
    if (axes_count == 0 || buttons_count == 0)
        return;
    #define MAP_BUTTON(KEY_NO, _UNUSED, BUTTON_NO)          do { io.AddKeyEvent(KEY_NO, (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS)); } while (0)
    #define MAP_ANALOG(KEY_NO, _UNUSED, AXIS_NO, V0, V1)    do { float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; v = (v - V0) / (V1 - V0); io.AddKeyAnalogEvent(KEY_NO, v > 0.10f, Saturate(v)); } while (0)
#endif
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    MAP_BUTTON(ImGuiKey_GamepadStart,       GLFW_GAMEPAD_BUTTON_START,          7);
    MAP_BUTTON(ImGuiKey_GamepadBack,        GLFW_GAMEPAD_BUTTON_BACK,           6);
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft,    GLFW_GAMEPAD_BUTTON_X,              2);     // Xbox X, PS Square
    MAP_BUTTON(ImGuiKey_GamepadFaceRight,   GLFW_GAMEPAD_BUTTON_B,              1);     // Xbox B, PS Circle
    MAP_BUTTON(ImGuiKey_GamepadFaceUp,      GLFW_GAMEPAD_BUTTON_Y,              3);     // Xbox Y, PS Triangle
    MAP_BUTTON(ImGuiKey_GamepadFaceDown,    GLFW_GAMEPAD_BUTTON_A,              0);     // Xbox A, PS Cross
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft,    GLFW_GAMEPAD_BUTTON_DPAD_LEFT,      13);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight,   GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,     11);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp,      GLFW_GAMEPAD_BUTTON_DPAD_UP,        10);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown,    GLFW_GAMEPAD_BUTTON_DPAD_DOWN,      12);
    MAP_BUTTON(ImGuiKey_GamepadL1,          GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,    4);
    MAP_BUTTON(ImGuiKey_GamepadR1,          GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,   5);
    MAP_ANALOG(ImGuiKey_GamepadL2,          GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,     4,      -0.75f,  +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadR2,          GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER,    5,      -0.75f,  +1.0f);
    MAP_BUTTON(ImGuiKey_GamepadL3,          GLFW_GAMEPAD_BUTTON_LEFT_THUMB,     8);
    MAP_BUTTON(ImGuiKey_GamepadR3,          GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,    9);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,  GLFW_GAMEPAD_AXIS_LEFT_X,           0,      -0.25f,  -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight, GLFW_GAMEPAD_AXIS_LEFT_X,           0,      +0.25f,  +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,    GLFW_GAMEPAD_AXIS_LEFT_Y,           1,      -0.25f,  -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,  GLFW_GAMEPAD_AXIS_LEFT_Y,           1,      +0.25f,  +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft,  GLFW_GAMEPAD_AXIS_RIGHT_X,          2,      -0.25f,  -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight, GLFW_GAMEPAD_AXIS_RIGHT_X,          2,      +0.25f,  +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp,    GLFW_GAMEPAD_AXIS_RIGHT_Y,          3,      -0.25f,  -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown,  GLFW_GAMEPAD_AXIS_RIGHT_Y,          3,      +0.25f,  +1.0f);
    #undef MAP_BUTTON
    #undef MAP_ANALOG
}

static void ImGui_ImplGlfw_UpdateMonitors()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    int monitors_count = 0;
    GLFWmonitor** glfw_monitors = glfwGetMonitors(&monitors_count);
    if (monitors_count == 0) // Preserve existing monitor list if there are none. Happens on macOS sleeping (#5683)
        return;

    platform_io.Monitors.resize(0);
    for (int n = 0; n < monitors_count; n++)
    {
        ImGuiPlatformMonitor monitor;
        int x, y;
        glfwGetMonitorPos(glfw_monitors[n], &x, &y);
        const GLFWvidmode* vid_mode = glfwGetVideoMode(glfw_monitors[n]);
        if (vid_mode == nullptr)
            continue; // Failed to get Video mode (e.g. Emscripten does not support this function)
        monitor.MainPos = monitor.WorkPos = ImVec2((float)x, (float)y);
        monitor.MainSize = monitor.WorkSize = ImVec2((float)vid_mode->width, (float)vid_mode->height);
#if GLFW_HAS_MONITOR_WORK_AREA
        int w, h;
        glfwGetMonitorWorkarea(glfw_monitors[n], &x, &y, &w, &h);
        if (w > 0 && h > 0) // Workaround a small GLFW issue reporting zero on monitor changes: https://github.com/glfw/glfw/pull/1761
        {
            monitor.WorkPos = ImVec2((float)x, (float)y);
            monitor.WorkSize = ImVec2((float)w, (float)h);
        }
#endif
        float scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfw_monitors[n]);
        if (scale == 0.0f)
            continue; // Some accessibility applications are declaring virtual monitors with a DPI of 0, see #7902.
        monitor.DpiScale = scale;
        monitor.PlatformHandle = (void*)glfw_monitors[n]; // [...] GLFW doc states: "guaranteed to be valid only until the monitor configuration changes"
        platform_io.Monitors.push_back(monitor);
    }
}

// - On Windows the process needs to be marked DPI-aware!! SDL2 doesn't do it by default. You can call ::SetProcessDPIAware() or call ImGui_ImplWin32_EnableDpiAwareness() from Win32 backend.
// - Apple platforms use FramebufferScale so we always return 1.0f.
// - Some accessibility applications are declaring virtual monitors with a DPI of 0.0f, see #7902. We preserve this value for caller to handle.
float ImGui_ImplGlfw_GetContentScaleForWindow(GLFWwindow* window)
{
#if GLFW_HAS_WAYLAND
    if (ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData(window))
        if (bd->IsWayland)
            return 1.0f;
#endif
#if GLFW_HAS_PER_MONITOR_DPI && !(defined(__APPLE__) || defined(__EMSCRIPTEN__) || defined(__ANDROID__))
    float x_scale, y_scale;
    glfwGetWindowContentScale(window, &x_scale, &y_scale);
    return x_scale;
#else
    IM_UNUSED(window);
    return 1.0f;
#endif
}

float ImGui_ImplGlfw_GetContentScaleForMonitor(GLFWmonitor* monitor)
{
#if GLFW_HAS_WAYLAND
    if (ImGui_ImplGlfw_IsWayland()) // We can't access our bd->IsWayland cache for a monitor.
        return 1.0f;
#endif
#if GLFW_HAS_PER_MONITOR_DPI && !(defined(__APPLE__) || defined(__EMSCRIPTEN__) || defined(__ANDROID__))
    float x_scale, y_scale;
    glfwGetMonitorContentScale(monitor, &x_scale, &y_scale);
    return x_scale;
#else
    IM_UNUSED(monitor);
    return 1.0f;
#endif
}

static void ImGui_ImplGlfw_GetWindowSizeAndFramebufferScale(GLFWwindow* window, ImVec2* out_size, ImVec2* out_framebuffer_scale)
{
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &display_w, &display_h);
    float fb_scale_x = (w > 0) ? (float)display_w / (float)w : 1.0f;
    float fb_scale_y = (h > 0) ? (float)display_h / (float)h : 1.0f;
#if GLFW_HAS_WAYLAND
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData(window);
    if (!bd->IsWayland)
        fb_scale_x = fb_scale_y = 1.0f;
#endif
    if (out_size != nullptr)
        *out_size = ImVec2((float)w, (float)h);
    if (out_framebuffer_scale != nullptr)
        *out_framebuffer_scale = ImVec2(fb_scale_x, fb_scale_y);
}

void ImGui_ImplGlfw_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplGlfw_InitForXXX()?");

    // Setup main viewport size (every frame to accommodate for window resizing)
    ImGui_ImplGlfw_GetWindowSizeAndFramebufferScale(bd->Window, &io.DisplaySize, &io.DisplayFramebufferScale);
    ImGui_ImplGlfw_UpdateMonitors();

    // Setup time step
    // (Accept glfwGetTime() not returning a monotonically increasing value. Seems to happens on disconnecting peripherals and probably on VMs and Emscripten, see #6491, #6189, #6114, #3644)
    double current_time = glfwGetTime();
    if (current_time <= bd->Time)
        current_time = bd->Time + 0.00001f;
    io.DeltaTime = bd->Time > 0.0 ? (float)(current_time - bd->Time) : (float)(1.0f / 60.0f);
    bd->Time = current_time;

    bd->MouseIgnoreButtonUp = false;
    ImGui_ImplGlfw_UpdateMouseData();
    ImGui_ImplGlfw_UpdateMouseCursor();

    // Update game controllers (if enabled and available)
    ImGui_ImplGlfw_UpdateGamepads();
}

// GLFW doesn't provide a portable sleep function
void ImGui_ImplGlfw_Sleep(int milliseconds)
{
#ifdef _WIN32
    ::Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

#ifdef EMSCRIPTEN_USE_EMBEDDED_GLFW3
static EM_BOOL ImGui_ImplGlfw_OnCanvasSizeChange(int event_type, const EmscriptenUiEvent* event, void* user_data)
{
    ImGui_ImplGlfw_Data* bd = (ImGui_ImplGlfw_Data*)user_data;
    double canvas_width, canvas_height;
    emscripten_get_element_css_size(bd->CanvasSelector, &canvas_width, &canvas_height);
    glfwSetWindowSize(bd->Window, (int)canvas_width, (int)canvas_height);
    return true;
}

static EM_BOOL ImGui_ImplEmscripten_FullscreenChangeCallback(int event_type, const EmscriptenFullscreenChangeEvent* event, void* user_data)
{
    ImGui_ImplGlfw_Data* bd = (ImGui_ImplGlfw_Data*)user_data;
    double canvas_width, canvas_height;
    emscripten_get_element_css_size(bd->CanvasSelector, &canvas_width, &canvas_height);
    glfwSetWindowSize(bd->Window, (int)canvas_width, (int)canvas_height);
    return true;
}

// 'canvas_selector' is a CSS selector. The event listener is applied to the first element that matches the query.
// STRING MUST PERSIST FOR THE APPLICATION DURATION. PLEASE USE A STRING LITERAL OR ENSURE POINTER WILL STAY VALID.
void ImGui_ImplGlfw_InstallEmscriptenCallbacks(GLFWwindow*, const char* canvas_selector)
{
    IM_ASSERT(canvas_selector != nullptr);
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplGlfw_InitForXXX()?");

    bd->CanvasSelector = canvas_selector;
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, bd, false, ImGui_ImplGlfw_OnCanvasSizeChange);
    emscripten_set_fullscreenchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, bd, false, ImGui_ImplEmscripten_FullscreenChangeCallback);

    // Change the size of the GLFW window according to the size of the canvas
    ImGui_ImplGlfw_OnCanvasSizeChange(EMSCRIPTEN_EVENT_RESIZE, {}, bd);

    // Register Emscripten Wheel callback to workaround issue in Emscripten GLFW Emulation (#6096)
    // We intentionally do not check 'if (install_callbacks)' here, as some users may set it to false and call GLFW callback themselves.
    // FIXME: May break chaining in case user registered their own Emscripten callback?
    emscripten_set_wheel_callback(bd->CanvasSelector, bd, false, ImGui_ImplEmscripten_WheelCallback);
}
#elif defined(EMSCRIPTEN_USE_PORT_CONTRIB_GLFW3)
// When using --use-port=contrib.glfw3 for the GLFW implementation, you can override the behavior of this call
// by invoking emscripten_glfw_make_canvas_resizable afterward.
// See https://github.com/pongasoft/emscripten-glfw/blob/master/docs/Usage.md#how-to-make-the-canvas-resizable-by-the-user for an explanation
void ImGui_ImplGlfw_InstallEmscriptenCallbacks(GLFWwindow* window, const char* canvas_selector)
{
  GLFWwindow* w = (GLFWwindow*)(EM_ASM_INT({ return Module.glfwGetWindow(UTF8ToString($0)); }, canvas_selector));
  IM_ASSERT(window == w); // Sanity check
  IM_UNUSED(w);
  emscripten_glfw_make_canvas_resizable(window, "window", nullptr);
}
#endif // #ifdef EMSCRIPTEN_USE_PORT_CONTRIB_GLFW3


//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

// Helper structure we store in the void* PlatformUserData field of each ImGuiViewport to easily retrieve our backend data.
struct ImGui_ImplGlfw_ViewportData
{
    GLFWwindow* Window;             // Stored in ImGuiViewport::PlatformHandle
    bool        WindowOwned;
    int         IgnoreWindowPosEventFrame;
    int         IgnoreWindowSizeEventFrame;
#ifdef _WIN32
    WNDPROC     PrevWndProc;
#endif

    ImGui_ImplGlfw_ViewportData()  { memset((void*)this, 0, sizeof(*this)); IgnoreWindowSizeEventFrame = IgnoreWindowPosEventFrame = -1; }
    ~ImGui_ImplGlfw_ViewportData() { IM_ASSERT(Window == nullptr); }
};

static void ImGui_ImplGlfw_WindowCloseCallback(GLFWwindow* window)
{
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window))
        viewport->PlatformRequestClose = true;
}

// GLFW may dispatch window pos/size events after calling glfwSetWindowPos()/glfwSetWindowSize().
// However: depending on the platform the callback may be invoked at different time:
// - on Windows it appears to be called within the glfwSetWindowPos()/glfwSetWindowSize() call
// - on Linux it is queued and invoked during glfwPollEvents()
// Because the event doesn't always fire on glfwSetWindowXXX() we use a frame counter tag to only
// ignore recent glfwSetWindowXXX() calls.
static void ImGui_ImplGlfw_WindowPosCallback(GLFWwindow* window, int, int)
{
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window))
    {
        if (ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData)
        {
            bool ignore_event = (ImGui::GetFrameCount() <= vd->IgnoreWindowPosEventFrame + 1);
            //data->IgnoreWindowPosEventFrame = -1;
            if (ignore_event)
                return;
        }
        viewport->PlatformRequestMove = true;
    }
}

static void ImGui_ImplGlfw_WindowSizeCallback(GLFWwindow* window, int, int)
{
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window))
    {
        if (ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData)
        {
            bool ignore_event = (ImGui::GetFrameCount() <= vd->IgnoreWindowSizeEventFrame + 1);
            //data->IgnoreWindowSizeEventFrame = -1;
            if (ignore_event)
                return;
        }
        viewport->PlatformRequestResize = true;
    }
}

#if !defined(__APPLE__) && !defined(_WIN32) && !defined(__EMSCRIPTEN__) && GLFW_HAS_GETPLATFORM
#define IMGUI_GLFW_HAS_SETWINDOWFLOATING
static void ImGui_ImplGlfw_SetWindowFloating(GLFWwindow* window)
{
#ifdef GLFW_EXPOSE_NATIVE_X11
    if (glfwGetPlatform() == GLFW_PLATFORM_X11)
    {
        Display* display = glfwGetX11Display();
        Window xwindow = glfwGetX11Window(window);
        Atom wm_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
        Atom wm_type_dialog = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
        XChangeProperty(display, xwindow, wm_type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&wm_type_dialog, 1);
        XSetWindowAttributes attrs;
        attrs.override_redirect = False;
        XChangeWindowAttributes(display, xwindow, CWOverrideRedirect, &attrs);
        XFlush(display);
    }
#endif // GLFW_EXPOSE_NATIVE_X11
#ifdef GLFW_EXPOSE_NATIVE_WAYLAND
    // FIXME: Help needed, see #8884, #8474 for discussions about this.
#endif // GLFW_EXPOSE_NATIVE_X11
}
#endif // IMGUI_GLFW_HAS_SETWINDOWFLOATING

static void ImGui_ImplGlfw_CreateWindow(ImGuiViewport* viewport)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    ImGui_ImplGlfw_ViewportData* vd = IM_NEW(ImGui_ImplGlfw_ViewportData)();
    viewport->PlatformUserData = vd;

    // Workaround for Linux: ignore mouse up events corresponding to losing focus of the previously focused window (#7733, #3158, #7922)
#ifdef __linux__
    bd->MouseIgnoreButtonUpWaitForFocusLoss = true;
#endif

    // GLFW 3.2 unfortunately always set focus on glfwCreateWindow() if GLFW_VISIBLE is set, regardless of GLFW_FOCUSED
    // With GLFW 3.3, the hint GLFW_FOCUS_ON_SHOW fixes this problem
    glfwWindowHint(GLFW_VISIBLE, false);
    glfwWindowHint(GLFW_FOCUSED, false);
#if GLFW_HAS_FOCUS_ON_SHOW
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, false);
#endif
    glfwWindowHint(GLFW_DECORATED, (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? false : true);
#if GLFW_HAS_WINDOW_TOPMOST
    glfwWindowHint(GLFW_FLOATING, (viewport->Flags & ImGuiViewportFlags_TopMost) ? true : false);
#endif
    GLFWwindow* share_window = (bd->ClientApi == GlfwClientApi_OpenGL) ? bd->Window : nullptr;
    vd->Window = glfwCreateWindow((int)viewport->Size.x, (int)viewport->Size.y, "No Title Yet", nullptr, share_window);
    vd->WindowOwned = true;
    ImGui_ImplGlfw_ContextMap_Add(vd->Window, bd->Context);
    viewport->PlatformHandle = (void*)vd->Window;
#ifdef IMGUI_GLFW_HAS_SETWINDOWFLOATING
    ImGui_ImplGlfw_SetWindowFloating(vd->Window);
#endif
#ifdef _WIN32
    viewport->PlatformHandleRaw = glfwGetWin32Window(vd->Window);
    ::SetPropA((HWND)viewport->PlatformHandleRaw, "IMGUI_BACKEND_DATA", bd);
#elif defined(__APPLE__)
    viewport->PlatformHandleRaw = (void*)glfwGetCocoaWindow(vd->Window);
#endif
    glfwSetWindowPos(vd->Window, (int)viewport->Pos.x, (int)viewport->Pos.y);

    // Install GLFW callbacks for secondary viewports
    glfwSetWindowFocusCallback(vd->Window, ImGui_ImplGlfw_WindowFocusCallback);
    glfwSetCursorEnterCallback(vd->Window, ImGui_ImplGlfw_CursorEnterCallback);
    glfwSetCursorPosCallback(vd->Window, ImGui_ImplGlfw_CursorPosCallback);
    glfwSetMouseButtonCallback(vd->Window, ImGui_ImplGlfw_MouseButtonCallback);
    glfwSetScrollCallback(vd->Window, ImGui_ImplGlfw_ScrollCallback);
    glfwSetKeyCallback(vd->Window, ImGui_ImplGlfw_KeyCallback);
    glfwSetCharCallback(vd->Window, ImGui_ImplGlfw_CharCallback);
    glfwSetWindowCloseCallback(vd->Window, ImGui_ImplGlfw_WindowCloseCallback);
    glfwSetWindowPosCallback(vd->Window, ImGui_ImplGlfw_WindowPosCallback);
    glfwSetWindowSizeCallback(vd->Window, ImGui_ImplGlfw_WindowSizeCallback);
    if (bd->ClientApi == GlfwClientApi_OpenGL)
    {
        glfwMakeContextCurrent(vd->Window);
        glfwSwapInterval(0);
    }
}

static void ImGui_ImplGlfw_DestroyWindow(ImGuiViewport* viewport)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    if (ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData)
    {
        if (vd->WindowOwned)
        {
#if !GLFW_HAS_MOUSE_PASSTHROUGH && GLFW_HAS_WINDOW_HOVERED && defined(_WIN32)
            HWND hwnd = (HWND)viewport->PlatformHandleRaw;
            ::RemovePropA(hwnd, "IMGUI_VIEWPORT");
#endif

            // Release any keys that were pressed in the window being destroyed and are still held down,
            // because we will not receive any release events after window is destroyed.
            for (int i = 0; i < IM_ARRAYSIZE(bd->KeyOwnerWindows); i++)
                if (bd->KeyOwnerWindows[i] == vd->Window)
                    ImGui_ImplGlfw_KeyCallback(vd->Window, i, 0, GLFW_RELEASE, 0); // Later params are only used for main viewport, on which this function is never called.

            ImGui_ImplGlfw_ContextMap_Remove(vd->Window);
            glfwDestroyWindow(vd->Window);
        }
        vd->Window = nullptr;
        IM_DELETE(vd);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = nullptr;
}

static void ImGui_ImplGlfw_ShowWindow(ImGuiViewport* viewport)
{
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;

#if defined(_WIN32)
    // GLFW hack: Hide icon from task bar
    HWND hwnd = (HWND)viewport->PlatformHandleRaw;
    if (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)
    {
        LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
        ex_style &= ~WS_EX_APPWINDOW;
        ex_style |= WS_EX_TOOLWINDOW;
        ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    }

    // GLFW hack: install WndProc for mouse source event and WM_NCHITTEST message handler.
    ::SetPropA(hwnd, "IMGUI_VIEWPORT", viewport);
    vd->PrevWndProc = (WNDPROC)::GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
    ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)ImGui_ImplGlfw_WndProc);

#if !GLFW_HAS_FOCUS_ON_SHOW
    // GLFW hack: GLFW 3.2 has a bug where glfwShowWindow() also activates/focus the window.
    // The fix was pushed to GLFW repository on 2018/01/09 and should be included in GLFW 3.3 via a GLFW_FOCUS_ON_SHOW window attribute.
    // See https://github.com/glfw/glfw/issues/1189
    // FIXME-VIEWPORT: Implement same work-around for Linux/OSX in the meanwhile.
    if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
    {
        ::ShowWindow(hwnd, SW_SHOWNA);
        return;
    }
#endif
#endif

    glfwShowWindow(vd->Window);
}

static ImVec2 ImGui_ImplGlfw_GetWindowPos(ImGuiViewport* viewport)
{
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    int x = 0, y = 0;
    glfwGetWindowPos(vd->Window, &x, &y);
    return ImVec2((float)x, (float)y);
}

static void ImGui_ImplGlfw_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    vd->IgnoreWindowPosEventFrame = ImGui::GetFrameCount();
    glfwSetWindowPos(vd->Window, (int)pos.x, (int)pos.y);
}

static ImVec2 ImGui_ImplGlfw_GetWindowSize(ImGuiViewport* viewport)
{
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    int w = 0, h = 0;
    glfwGetWindowSize(vd->Window, &w, &h);
    return ImVec2((float)w, (float)h);
}

static void ImGui_ImplGlfw_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
#if defined(__APPLE__) && !GLFW_HAS_OSX_WINDOW_POS_FIX
    // Native OS windows are positioned from the bottom-left corner on macOS, whereas on other platforms they are
    // positioned from the upper-left corner. GLFW makes an effort to convert macOS style coordinates, however it
    // doesn't handle it when changing size. We are manually moving the window in order for changes of size to be based
    // on the upper-left corner.
    int x, y, width, height;
    glfwGetWindowPos(vd->Window, &x, &y);
    glfwGetWindowSize(vd->Window, &width, &height);
    glfwSetWindowPos(vd->Window, x, y - height + size.y);
#endif
    vd->IgnoreWindowSizeEventFrame = ImGui::GetFrameCount();
    glfwSetWindowSize(vd->Window, (int)size.x, (int)size.y);
}

static ImVec2 ImGui_ImplGlfw_GetWindowFramebufferScale(ImGuiViewport* viewport)
{
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    ImVec2 framebuffer_scale;
    ImGui_ImplGlfw_GetWindowSizeAndFramebufferScale(vd->Window, nullptr, &framebuffer_scale);
    return framebuffer_scale;
}

static void ImGui_ImplGlfw_SetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    glfwSetWindowTitle(vd->Window, title);
}

static void ImGui_ImplGlfw_SetWindowFocus(ImGuiViewport* viewport)
{
#if GLFW_HAS_FOCUS_WINDOW
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    glfwFocusWindow(vd->Window);
#else
    // FIXME: What are the effect of not having this function? At the moment imgui doesn't actually call SetWindowFocus - we set that up ahead, will answer that question later.
    (void)viewport;
#endif
}

static bool ImGui_ImplGlfw_GetWindowFocus(ImGuiViewport* viewport)
{
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    return glfwGetWindowAttrib(vd->Window, GLFW_FOCUSED) != 0;
}

static bool ImGui_ImplGlfw_GetWindowMinimized(ImGuiViewport* viewport)
{
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    return glfwGetWindowAttrib(vd->Window, GLFW_ICONIFIED) != 0;
}

#if GLFW_HAS_WINDOW_ALPHA
static void ImGui_ImplGlfw_SetWindowAlpha(ImGuiViewport* viewport, float alpha)
{
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    glfwSetWindowOpacity(vd->Window, alpha);
}
#endif

static void ImGui_ImplGlfw_RenderWindow(ImGuiViewport* viewport, void*)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    if (bd->ClientApi == GlfwClientApi_OpenGL)
        glfwMakeContextCurrent(vd->Window);
}

static void ImGui_ImplGlfw_SwapBuffers(ImGuiViewport* viewport, void*)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    if (bd->ClientApi == GlfwClientApi_OpenGL)
    {
        glfwMakeContextCurrent(vd->Window);
        glfwSwapBuffers(vd->Window);
    }
}

//--------------------------------------------------------------------------------------------------------
// Vulkan support (the Vulkan renderer needs to call a platform-side support function to create the surface)
//--------------------------------------------------------------------------------------------------------

// Avoid including <vulkan.h> so we can build without it
#if GLFW_HAS_VULKAN
#ifndef VULKAN_H_
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif
VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)
struct VkAllocationCallbacks;
enum VkResult { VK_RESULT_MAX_ENUM = 0x7FFFFFFF };
#endif // VULKAN_H_
extern "C" { extern GLFWAPI VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface); }
static int ImGui_ImplGlfw_CreateVkSurface(ImGuiViewport* viewport, ImU64 vk_instance, const void* vk_allocator, ImU64* out_vk_surface)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData;
    IM_UNUSED(bd);
    IM_ASSERT(bd->ClientApi == GlfwClientApi_Vulkan);
    VkResult err = glfwCreateWindowSurface((VkInstance)vk_instance, vd->Window, (const VkAllocationCallbacks*)vk_allocator, (VkSurfaceKHR*)out_vk_surface);
    return (int)err;
}
#endif // GLFW_HAS_VULKAN

static void ImGui_ImplGlfw_InitMultiViewportSupport()
{
    // Register platform interface (will be coupled with a renderer interface)
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateWindow = ImGui_ImplGlfw_CreateWindow;
    platform_io.Platform_DestroyWindow = ImGui_ImplGlfw_DestroyWindow;
    platform_io.Platform_ShowWindow = ImGui_ImplGlfw_ShowWindow;
    platform_io.Platform_SetWindowPos = ImGui_ImplGlfw_SetWindowPos;
    platform_io.Platform_GetWindowPos = ImGui_ImplGlfw_GetWindowPos;
    platform_io.Platform_SetWindowSize = ImGui_ImplGlfw_SetWindowSize;
    platform_io.Platform_GetWindowSize = ImGui_ImplGlfw_GetWindowSize;
    platform_io.Platform_GetWindowFramebufferScale = ImGui_ImplGlfw_GetWindowFramebufferScale;
    platform_io.Platform_SetWindowFocus = ImGui_ImplGlfw_SetWindowFocus;
    platform_io.Platform_GetWindowFocus = ImGui_ImplGlfw_GetWindowFocus;
    platform_io.Platform_GetWindowMinimized = ImGui_ImplGlfw_GetWindowMinimized;
    platform_io.Platform_SetWindowTitle = ImGui_ImplGlfw_SetWindowTitle;
    platform_io.Platform_RenderWindow = ImGui_ImplGlfw_RenderWindow;
    platform_io.Platform_SwapBuffers = ImGui_ImplGlfw_SwapBuffers;
#if GLFW_HAS_WINDOW_ALPHA
    platform_io.Platform_SetWindowAlpha = ImGui_ImplGlfw_SetWindowAlpha;
#endif
#if GLFW_HAS_VULKAN
    platform_io.Platform_CreateVkSurface = ImGui_ImplGlfw_CreateVkSurface;
#endif

    // Register main window handle (which is owned by the main application, not by us)
    // This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.) can use same logic for main and secondary viewports.
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui_ImplGlfw_ViewportData* vd = IM_NEW(ImGui_ImplGlfw_ViewportData)();
    vd->Window = bd->Window;
    vd->WindowOwned = false;
    main_viewport->PlatformUserData = vd;
    main_viewport->PlatformHandle = (void*)bd->Window;
}

static void ImGui_ImplGlfw_ShutdownMultiViewportSupport()
{
    ImGui::DestroyPlatformWindows();
}

//-----------------------------------------------------------------------------

// WndProc hook (declared here because we will need access to ImGui_ImplGlfw_ViewportData)
#ifdef _WIN32
static ImGuiMouseSource GetMouseSourceFromMessageExtraInfo()
{
    LPARAM extra_info = ::GetMessageExtraInfo();
    if ((extra_info & 0xFFFFFF80) == 0xFF515700)
        return ImGuiMouseSource_Pen;
    if ((extra_info & 0xFFFFFF80) == 0xFF515780)
        return ImGuiMouseSource_TouchScreen;
    return ImGuiMouseSource_Mouse;
}
static LRESULT CALLBACK ImGui_ImplGlfw_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ImGui_ImplGlfw_Data* bd = (ImGui_ImplGlfw_Data*)::GetPropA(hWnd, "IMGUI_BACKEND_DATA");
    ImGuiIO& io = ImGui::GetIO(bd->Context);

    WNDPROC prev_wndproc = bd->PrevWndProc;
    ImGuiViewport* viewport = (ImGuiViewport*)::GetPropA(hWnd, "IMGUI_VIEWPORT");
    if (viewport != NULL)
        if (ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData)
            prev_wndproc = vd->PrevWndProc;

    switch (msg)
    {
        // GLFW doesn't allow to distinguish Mouse vs TouchScreen vs Pen.
        // Add support for Win32 (based on imgui_impl_win32), because we rely on _TouchScreen info to trickle inputs differently.
    case WM_MOUSEMOVE: case WM_NCMOUSEMOVE:
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_LBUTTONUP:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: case WM_RBUTTONUP:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: case WM_MBUTTONUP:
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: case WM_XBUTTONUP:
        io.AddMouseSourceEvent(GetMouseSourceFromMessageExtraInfo());
        break;

        // We have submitted https://github.com/glfw/glfw/pull/1568 to allow GLFW to support "transparent inputs".
        // In the meanwhile we implement custom per-platform workarounds here (FIXME-VIEWPORT: Implement same work-around for Linux/OSX!)
#if !GLFW_HAS_MOUSE_PASSTHROUGH && GLFW_HAS_WINDOW_HOVERED
    case WM_NCHITTEST:
    {
        // Let mouse pass-through the window. This will allow the backend to call io.AddMouseViewportEvent() properly (which is OPTIONAL).
        // The ImGuiViewportFlags_NoInputs flag is set while dragging a viewport, as want to detect the window behind the one we are dragging.
        // If you cannot easily access those viewport flags from your windowing/event code: you may manually synchronize its state e.g. in
        // your main loop after calling UpdatePlatformWindows(). Iterate all viewports/platform windows and pass the flag to your windowing system.
        if (viewport && (viewport->Flags & ImGuiViewportFlags_NoInputs))
            return HTTRANSPARENT;
        break;
    }
#endif
    default: break;
    }
    return ::CallWindowProcW(prev_wndproc, hWnd, msg, wParam, lParam);
}
#endif // #ifdef _WIN32

//-----------------------------------------------------------------------------

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif // #ifndef IMGUI_DISABLE
