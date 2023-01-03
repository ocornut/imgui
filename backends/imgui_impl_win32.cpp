// dear imgui: Platform Backend for Windows (standard windows API for 32-bits AND 64-bits applications)
// This needs to be used along with a Renderer (e.g. DirectX11, OpenGL3, Vulkan..)

// Implemented features:
//  [X] Platform: Clipboard support (for Win32 this is actually part of core dear imgui)
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy VK_* values will also be supported unless IMGUI_DISABLE_OBSOLETE_KEYIO is set]
//  [X] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [X] Platform: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h> // GET_X_LPARAM(), GET_Y_LPARAM()
#include <tchar.h>
#include <dwmapi.h>

// Configuration flags to add in your imconfig.h file:
//#define IMGUI_IMPL_WIN32_DISABLE_GAMEPAD              // Disable gamepad support. This was meaningful before <1.81 but we now load XInput dynamically so the option is now less relevant.

// Using XInput for gamepad (will load DLL dynamically)
#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
#include <xinput.h>
typedef DWORD (WINAPI *PFN_XInputGetCapabilities)(DWORD, DWORD, XINPUT_CAPABILITIES*);
typedef DWORD (WINAPI *PFN_XInputGetState)(DWORD, XINPUT_STATE*);
#endif

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2023-XX-XX: Platform: Added support for multiple windows via the ImGuiPlatformIO interface.
//  2022-10-11: Using 'nullptr' instead of 'NULL' as per our switch to C++11.
//  2022-09-28: Inputs: Convert WM_CHAR values with MultiByteToWideChar() when window class was registered as MBCS (not Unicode).
//  2022-09-26: Inputs: Renamed ImGuiKey_ModXXX introduced in 1.87 to ImGuiMod_XXX (old names still supported).
//  2022-01-26: Inputs: replaced short-lived io.AddKeyModsEvent() (added two weeks ago) with io.AddKeyEvent() using ImGuiKey_ModXXX flags. Sorry for the confusion.
//  2021-01-20: Inputs: calling new io.AddKeyAnalogEvent() for gamepad support, instead of writing directly to io.NavInputs[].
//  2022-01-17: Inputs: calling new io.AddMousePosEvent(), io.AddMouseButtonEvent(), io.AddMouseWheelEvent() API (1.87+).
//  2022-01-17: Inputs: always update key mods next and before a key event (not in NewFrame) to fix input queue with very low framerates.
//  2022-01-12: Inputs: Update mouse inputs using WM_MOUSEMOVE/WM_MOUSELEAVE + fallback to provide it when focused but not hovered/captured. More standard and will allow us to pass it to future input queue API.
//  2022-01-12: Inputs: Maintain our own copy of MouseButtonsDown mask instead of using ImGui::IsAnyMouseDown() which will be obsoleted.
//  2022-01-10: Inputs: calling new io.AddKeyEvent(), io.AddKeyModsEvent() + io.SetKeyEventNativeData() API (1.87+). Support for full ImGuiKey range.
//  2021-12-16: Inputs: Fill VK_LCONTROL/VK_RCONTROL/VK_LSHIFT/VK_RSHIFT/VK_LMENU/VK_RMENU for completeness.
//  2021-08-17: Calling io.AddFocusEvent() on WM_SETFOCUS/WM_KILLFOCUS messages.
//  2021-08-02: Inputs: Fixed keyboard modifiers being reported when host window doesn't have focus.
//  2021-07-29: Inputs: MousePos is correctly reported when the host platform window is hovered but not focused (using TrackMouseEvent() to receive WM_MOUSELEAVE events).
//  2021-06-29: Reorganized backend to pull data from a single structure to facilitate usage with multiple-contexts (all g_XXXX access changed to bd->XXXX).
//  2021-06-08: Fixed ImGui_ImplWin32_EnableDpiAwareness() and ImGui_ImplWin32_GetDpiScaleForMonitor() to handle Windows 8.1/10 features without a manifest (per-monitor DPI, and properly calls SetProcessDpiAwareness() on 8.1).
//  2021-03-23: Inputs: Clearing keyboard down array when losing focus (WM_KILLFOCUS).
//  2021-02-18: Added ImGui_ImplWin32_EnableAlphaCompositing(). Non Visual Studio users will need to link with dwmapi.lib (MinGW/gcc: use -ldwmapi).
//  2021-02-17: Fixed ImGui_ImplWin32_EnableDpiAwareness() attempting to get SetProcessDpiAwareness from shcore.dll on Windows 8 whereas it is only supported on Windows 8.1.
//  2021-01-25: Inputs: Dynamically loading XInput DLL.
//  2020-12-04: Misc: Fixed setting of io.DisplaySize to invalid/uninitialized data when after hwnd has been closed.
//  2020-03-03: Inputs: Calling AddInputCharacterUTF16() to support surrogate pairs leading to codepoint >= 0x10000 (for more complete CJK inputs)
//  2020-02-17: Added ImGui_ImplWin32_EnableDpiAwareness(), ImGui_ImplWin32_GetDpiScaleForHwnd(), ImGui_ImplWin32_GetDpiScaleForMonitor() helper functions.
//  2020-01-14: Inputs: Added support for #define IMGUI_IMPL_WIN32_DISABLE_GAMEPAD/IMGUI_IMPL_WIN32_DISABLE_LINKING_XINPUT.
//  2019-12-05: Inputs: Added support for ImGuiMouseCursor_NotAllowed mouse cursor.
//  2019-05-11: Inputs: Don't filter value from WM_CHAR before calling AddInputCharacter().
//  2019-01-17: Misc: Using GetForegroundWindow()+IsChild() instead of GetActiveWindow() to be compatible with windows created in a different thread or parent.
//  2019-01-17: Inputs: Added support for mouse buttons 4 and 5 via WM_XBUTTON* messages.
//  2019-01-15: Inputs: Added support for XInput gamepads (if ImGuiConfigFlags_NavEnableGamepad is set by user application).
//  2018-11-30: Misc: Setting up io.BackendPlatformName so it can be displayed in the About Window.
//  2018-06-29: Inputs: Added support for the ImGuiMouseCursor_Hand cursor.
//  2018-06-10: Inputs: Fixed handling of mouse wheel messages to support fine position messages (typically sent by track-pads).
//  2018-06-08: Misc: Extracted imgui_impl_win32.cpp/.h away from the old combined DX9/DX10/DX11/DX12 examples.
//  2018-03-20: Misc: Setup io.BackendFlags ImGuiBackendFlags_HasMouseCursors and ImGuiBackendFlags_HasSetMousePos flags + honor ImGuiConfigFlags_NoMouseCursorChange flag.
//  2018-02-20: Inputs: Added support for mouse cursors (ImGui::GetMouseCursor() value and WM_SETCURSOR message handling).
//  2018-02-06: Inputs: Added mapping for ImGuiKey_Space.
//  2018-02-06: Inputs: Honoring the io.WantSetMousePos by repositioning the mouse (when using navigation and ImGuiConfigFlags_NavMoveMouse is set).
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2018-01-20: Inputs: Added Horizontal Mouse Wheel support.
//  2018-01-08: Inputs: Added mapping for ImGuiKey_Insert.
//  2018-01-05: Inputs: Added WM_LBUTTONDBLCLK double-click handlers for window classes with the CS_DBLCLKS flag.
//  2017-10-23: Inputs: Added WM_SYSKEYDOWN / WM_SYSKEYUP handlers so e.g. the VK_MENU key can be read.
//  2017-10-23: Inputs: Using Win32 ::SetCapture/::GetCapture() to retrieve mouse positions outside the client area when dragging.
//  2016-11-12: Inputs: Only call Win32 ::SetCursor(nullptr) when io.MouseDrawCursor is set.

// Forward Declarations
static void ImGui_ImplWin32_InitPlatformInterface();
static void ImGui_ImplWin32_ShutdownPlatformInterface();
static void ImGui_ImplWin32_UpdateMonitors();

struct ImGui_ImplWin32_Data
{
    HWND                        hWnd;
    HWND                        MouseHwnd;
    bool                        MouseTracked;
    int                         MouseButtonsDown;
    INT64                       Time;
    INT64                       TicksPerSecond;
    ImGuiMouseCursor            LastMouseCursor;
    bool                        WantUpdateMonitors;

#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
    bool                        HasGamepad;
    bool                        WantUpdateHasGamepad;
    HMODULE                     XInputDLL;
    PFN_XInputGetCapabilities   XInputGetCapabilities;
    PFN_XInputGetState          XInputGetState;
#endif

    ImGui_ImplWin32_Data()      { memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
static ImGui_ImplWin32_Data* ImGui_ImplWin32_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplWin32_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

// Functions
bool    ImGui_ImplWin32_Init(void* hwnd)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    INT64 perf_frequency, perf_counter;
    if (!::QueryPerformanceFrequency((LARGE_INTEGER*)&perf_frequency))
        return false;
    if (!::QueryPerformanceCounter((LARGE_INTEGER*)&perf_counter))
        return false;

    // Setup backend capabilities flags
    ImGui_ImplWin32_Data* bd = IM_NEW(ImGui_ImplWin32_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_win32";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can call io.AddMouseViewportEvent() with correct data (optional)

    bd->hWnd = (HWND)hwnd;
    bd->WantUpdateMonitors = true;
    bd->TicksPerSecond = perf_frequency;
    bd->Time = perf_counter;
    bd->LastMouseCursor = ImGuiMouseCursor_COUNT;

    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = main_viewport->PlatformHandleRaw = (void*)bd->hWnd;
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        ImGui_ImplWin32_InitPlatformInterface();

    // Dynamically load XInput library
#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
    bd->WantUpdateHasGamepad = true;
    const char* xinput_dll_names[] =
    {
        "xinput1_4.dll",   // Windows 8+
        "xinput1_3.dll",   // DirectX SDK
        "xinput9_1_0.dll", // Windows Vista, Windows 7
        "xinput1_2.dll",   // DirectX SDK
        "xinput1_1.dll"    // DirectX SDK
    };
    for (int n = 0; n < IM_ARRAYSIZE(xinput_dll_names); n++)
        if (HMODULE dll = ::LoadLibraryA(xinput_dll_names[n]))
        {
            bd->XInputDLL = dll;
            bd->XInputGetCapabilities = (PFN_XInputGetCapabilities)::GetProcAddress(dll, "XInputGetCapabilities");
            bd->XInputGetState = (PFN_XInputGetState)::GetProcAddress(dll, "XInputGetState");
            break;
        }
#endif // IMGUI_IMPL_WIN32_DISABLE_GAMEPAD

    return true;
}

void    ImGui_ImplWin32_Shutdown()
{
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplWin32_ShutdownPlatformInterface();

    // Unload XInput library
#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
    if (bd->XInputDLL)
        ::FreeLibrary(bd->XInputDLL);
#endif // IMGUI_IMPL_WIN32_DISABLE_GAMEPAD

    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    IM_DELETE(bd);
}

static bool ImGui_ImplWin32_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(nullptr);
    }
    else
    {
        // Show OS mouse cursor
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
        case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
        }
        ::SetCursor(::LoadCursor(nullptr, win32_cursor));
    }
    return true;
}

static bool IsVkDown(int vk)
{
    return (::GetKeyState(vk) & 0x8000) != 0;
}

static void ImGui_ImplWin32_AddKeyEvent(ImGuiKey key, bool down, int native_keycode, int native_scancode = -1)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(key, down);
    io.SetKeyEventNativeData(key, native_keycode, native_scancode); // To support legacy indexing (<1.87 user code)
    IM_UNUSED(native_scancode);
}

static void ImGui_ImplWin32_ProcessKeyEventsWorkarounds()
{
    // Left & right Shift keys: when both are pressed together, Windows tend to not generate the WM_KEYUP event for the first released one.
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && !IsVkDown(VK_LSHIFT))
        ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftShift, false, VK_LSHIFT);
    if (ImGui::IsKeyDown(ImGuiKey_RightShift) && !IsVkDown(VK_RSHIFT))
        ImGui_ImplWin32_AddKeyEvent(ImGuiKey_RightShift, false, VK_RSHIFT);

    // Sometimes WM_KEYUP for Win key is not passed down to the app (e.g. for Win+V on some setups, according to GLFW).
    if (ImGui::IsKeyDown(ImGuiKey_LeftSuper) && !IsVkDown(VK_LWIN))
        ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftSuper, false, VK_LWIN);
    if (ImGui::IsKeyDown(ImGuiKey_RightSuper) && !IsVkDown(VK_RWIN))
        ImGui_ImplWin32_AddKeyEvent(ImGuiKey_RightSuper, false, VK_RWIN);
}

static void ImGui_ImplWin32_UpdateKeyModifiers()
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, IsVkDown(VK_CONTROL));
    io.AddKeyEvent(ImGuiMod_Shift, IsVkDown(VK_SHIFT));
    io.AddKeyEvent(ImGuiMod_Alt, IsVkDown(VK_MENU));
    io.AddKeyEvent(ImGuiMod_Super, IsVkDown(VK_APPS));
}

// This code supports multi-viewports (multiple OS Windows mapped into different Dear ImGui viewports)
// Because of that, it is a little more complicated than your typical single-viewport binding code!
static void ImGui_ImplWin32_UpdateMouseData()
{
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(bd->hWnd != 0);

    POINT mouse_screen_pos;
    bool has_mouse_screen_pos = ::GetCursorPos(&mouse_screen_pos) != 0;

    HWND focused_window = ::GetForegroundWindow();
    const bool is_app_focused = (focused_window && (focused_window == bd->hWnd || ::IsChild(focused_window, bd->hWnd) || ImGui::FindViewportByPlatformHandle((void*)focused_window)));
    if (is_app_focused)
    {
        // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        // When multi-viewports are enabled, all Dear ImGui positions are same as OS positions.
        if (io.WantSetMousePos)
        {
            POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
            if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0)
                ::ClientToScreen(focused_window, &pos);
            ::SetCursorPos(pos.x, pos.y);
        }

        // (Optional) Fallback to provide mouse position when focused (WM_MOUSEMOVE already provides this when hovered or captured)
        if (!io.WantSetMousePos && !bd->MouseTracked && has_mouse_screen_pos)
        {
            // Single viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
            // (This is the position you can get with ::GetCursorPos() + ::ScreenToClient() or WM_MOUSEMOVE.)
            // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
            // (This is the position you can get with ::GetCursorPos() or WM_MOUSEMOVE + ::ClientToScreen(). In theory adding viewport->Pos to a client position would also be the same.)
            POINT mouse_pos = mouse_screen_pos;
            if (!(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
                ::ScreenToClient(bd->hWnd, &mouse_pos);
            io.AddMousePosEvent((float)mouse_pos.x, (float)mouse_pos.y);
        }
    }

    // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the viewport the OS mouse cursor is hovering.
    // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, Dear imGui will ignore this field and infer the information using its flawed heuristic.
    // - [X] Win32 backend correctly ignore viewports with the _NoInputs flag (here using ::WindowFromPoint with WM_NCHITTEST + HTTRANSPARENT in WndProc does that)
    //       Some backend are not able to handle that correctly. If a backend report an hovered viewport that has the _NoInputs flag (e.g. when dragging a window
    //       for docking, the viewport has the _NoInputs flag in order to allow us to find the viewport under), then Dear ImGui is forced to ignore the value reported
    //       by the backend, and use its flawed heuristic to guess the viewport behind.
    // - [X] Win32 backend correctly reports this regardless of another viewport behind focused and dragged from (we need this to find a useful drag and drop target).
    ImGuiID mouse_viewport_id = 0;
    if (has_mouse_screen_pos)
        if (HWND hovered_hwnd = ::WindowFromPoint(mouse_screen_pos))
            if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void*)hovered_hwnd))
                mouse_viewport_id = viewport->ID;
    io.AddMouseViewportEvent(mouse_viewport_id);
}

// Gamepad navigation mapping
static void ImGui_ImplWin32_UpdateGamepads()
{
#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
    //if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0) // FIXME: Technically feeding gamepad shouldn't depend on this now that they are regular inputs.
    //    return;

    // Calling XInputGetState() every frame on disconnected gamepads is unfortunately too slow.
    // Instead we refresh gamepad availability by calling XInputGetCapabilities() _only_ after receiving WM_DEVICECHANGE.
    if (bd->WantUpdateHasGamepad)
    {
        XINPUT_CAPABILITIES caps = {};
        bd->HasGamepad = bd->XInputGetCapabilities ? (bd->XInputGetCapabilities(0, XINPUT_FLAG_GAMEPAD, &caps) == ERROR_SUCCESS) : false;
        bd->WantUpdateHasGamepad = false;
    }

    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    XINPUT_STATE xinput_state;
    XINPUT_GAMEPAD& gamepad = xinput_state.Gamepad;
    if (!bd->HasGamepad || bd->XInputGetState == nullptr || bd->XInputGetState(0, &xinput_state) != ERROR_SUCCESS)
        return;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    #define IM_SATURATE(V)                      (V < 0.0f ? 0.0f : V > 1.0f ? 1.0f : V)
    #define MAP_BUTTON(KEY_NO, BUTTON_ENUM)     { io.AddKeyEvent(KEY_NO, (gamepad.wButtons & BUTTON_ENUM) != 0); }
    #define MAP_ANALOG(KEY_NO, VALUE, V0, V1)   { float vn = (float)(VALUE - V0) / (float)(V1 - V0); io.AddKeyAnalogEvent(KEY_NO, vn > 0.10f, IM_SATURATE(vn)); }
    MAP_BUTTON(ImGuiKey_GamepadStart,           XINPUT_GAMEPAD_START);
    MAP_BUTTON(ImGuiKey_GamepadBack,            XINPUT_GAMEPAD_BACK);
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft,        XINPUT_GAMEPAD_X);
    MAP_BUTTON(ImGuiKey_GamepadFaceRight,       XINPUT_GAMEPAD_B);
    MAP_BUTTON(ImGuiKey_GamepadFaceUp,          XINPUT_GAMEPAD_Y);
    MAP_BUTTON(ImGuiKey_GamepadFaceDown,        XINPUT_GAMEPAD_A);
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft,        XINPUT_GAMEPAD_DPAD_LEFT);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight,       XINPUT_GAMEPAD_DPAD_RIGHT);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp,          XINPUT_GAMEPAD_DPAD_UP);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown,        XINPUT_GAMEPAD_DPAD_DOWN);
    MAP_BUTTON(ImGuiKey_GamepadL1,              XINPUT_GAMEPAD_LEFT_SHOULDER);
    MAP_BUTTON(ImGuiKey_GamepadR1,              XINPUT_GAMEPAD_RIGHT_SHOULDER);
    MAP_ANALOG(ImGuiKey_GamepadL2,              gamepad.bLeftTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD, 255);
    MAP_ANALOG(ImGuiKey_GamepadR2,              gamepad.bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD, 255);
    MAP_BUTTON(ImGuiKey_GamepadL3,              XINPUT_GAMEPAD_LEFT_THUMB);
    MAP_BUTTON(ImGuiKey_GamepadR3,              XINPUT_GAMEPAD_RIGHT_THUMB);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,      gamepad.sThumbLX, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight,     gamepad.sThumbLX, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,        gamepad.sThumbLY, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,      gamepad.sThumbLY, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft,      gamepad.sThumbRX, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight,     gamepad.sThumbRX, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp,        gamepad.sThumbRY, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown,      gamepad.sThumbRY, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
    #undef MAP_BUTTON
    #undef MAP_ANALOG
#endif // #ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
}

static BOOL CALLBACK ImGui_ImplWin32_UpdateMonitors_EnumFunc(HMONITOR monitor, HDC, LPRECT, LPARAM)
{
    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);
    if (!::GetMonitorInfo(monitor, &info))
        return TRUE;
    ImGuiPlatformMonitor imgui_monitor;
    imgui_monitor.MainPos = ImVec2((float)info.rcMonitor.left, (float)info.rcMonitor.top);
    imgui_monitor.MainSize = ImVec2((float)(info.rcMonitor.right - info.rcMonitor.left), (float)(info.rcMonitor.bottom - info.rcMonitor.top));
    imgui_monitor.WorkPos = ImVec2((float)info.rcWork.left, (float)info.rcWork.top);
    imgui_monitor.WorkSize = ImVec2((float)(info.rcWork.right - info.rcWork.left), (float)(info.rcWork.bottom - info.rcWork.top));
    imgui_monitor.DpiScale = ImGui_ImplWin32_GetDpiScaleForMonitor(monitor);
    ImGuiPlatformIO& io = ImGui::GetPlatformIO();
    if (info.dwFlags & MONITORINFOF_PRIMARY)
        io.Monitors.push_front(imgui_monitor);
    else
        io.Monitors.push_back(imgui_monitor);
    return TRUE;
}

static void ImGui_ImplWin32_UpdateMonitors()
{
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
    ImGui::GetPlatformIO().Monitors.resize(0);
    ::EnumDisplayMonitors(nullptr, nullptr, ImGui_ImplWin32_UpdateMonitors_EnumFunc, 0);
    bd->WantUpdateMonitors = false;
}

void    ImGui_ImplWin32_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplWin32_Init()?");

    // Setup display size (every frame to accommodate for window resizing)
    RECT rect = { 0, 0, 0, 0 };
    ::GetClientRect(bd->hWnd, &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
    if (bd->WantUpdateMonitors)
        ImGui_ImplWin32_UpdateMonitors();

    // Setup time step
    INT64 current_time = 0;
    ::QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
    io.DeltaTime = (float)(current_time - bd->Time) / bd->TicksPerSecond;
    bd->Time = current_time;

    // Update OS mouse position
    ImGui_ImplWin32_UpdateMouseData();

    // Process workarounds for known Windows key handling issues
    ImGui_ImplWin32_ProcessKeyEventsWorkarounds();

    // Update OS mouse cursor with the cursor requested by imgui
    ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (bd->LastMouseCursor != mouse_cursor)
    {
        bd->LastMouseCursor = mouse_cursor;
        ImGui_ImplWin32_UpdateMouseCursor();
    }

    // Update game controllers (if enabled and available)
    ImGui_ImplWin32_UpdateGamepads();
}

// There is no distinct VK_xxx for keypad enter, instead it is VK_RETURN + KF_EXTENDED, we assign it an arbitrary value to make code more readable (VK_ codes go up to 255)
#define IM_VK_KEYPAD_ENTER      (VK_RETURN + 256)

// Map VK_xxx to ImGuiKey_xxx.
static ImGuiKey ImGui_ImplWin32_VirtualKeyToImGuiKey(WPARAM wParam)
{
    switch (wParam)
    {
        case VK_TAB: return ImGuiKey_Tab;
        case VK_LEFT: return ImGuiKey_LeftArrow;
        case VK_RIGHT: return ImGuiKey_RightArrow;
        case VK_UP: return ImGuiKey_UpArrow;
        case VK_DOWN: return ImGuiKey_DownArrow;
        case VK_PRIOR: return ImGuiKey_PageUp;
        case VK_NEXT: return ImGuiKey_PageDown;
        case VK_HOME: return ImGuiKey_Home;
        case VK_END: return ImGuiKey_End;
        case VK_INSERT: return ImGuiKey_Insert;
        case VK_DELETE: return ImGuiKey_Delete;
        case VK_BACK: return ImGuiKey_Backspace;
        case VK_SPACE: return ImGuiKey_Space;
        case VK_RETURN: return ImGuiKey_Enter;
        case VK_ESCAPE: return ImGuiKey_Escape;
        case VK_OEM_7: return ImGuiKey_Apostrophe;
        case VK_OEM_COMMA: return ImGuiKey_Comma;
        case VK_OEM_MINUS: return ImGuiKey_Minus;
        case VK_OEM_PERIOD: return ImGuiKey_Period;
        case VK_OEM_2: return ImGuiKey_Slash;
        case VK_OEM_1: return ImGuiKey_Semicolon;
        case VK_OEM_PLUS: return ImGuiKey_Equal;
        case VK_OEM_4: return ImGuiKey_LeftBracket;
        case VK_OEM_5: return ImGuiKey_Backslash;
        case VK_OEM_6: return ImGuiKey_RightBracket;
        case VK_OEM_3: return ImGuiKey_GraveAccent;
        case VK_CAPITAL: return ImGuiKey_CapsLock;
        case VK_SCROLL: return ImGuiKey_ScrollLock;
        case VK_NUMLOCK: return ImGuiKey_NumLock;
        case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
        case VK_PAUSE: return ImGuiKey_Pause;
        case VK_NUMPAD0: return ImGuiKey_Keypad0;
        case VK_NUMPAD1: return ImGuiKey_Keypad1;
        case VK_NUMPAD2: return ImGuiKey_Keypad2;
        case VK_NUMPAD3: return ImGuiKey_Keypad3;
        case VK_NUMPAD4: return ImGuiKey_Keypad4;
        case VK_NUMPAD5: return ImGuiKey_Keypad5;
        case VK_NUMPAD6: return ImGuiKey_Keypad6;
        case VK_NUMPAD7: return ImGuiKey_Keypad7;
        case VK_NUMPAD8: return ImGuiKey_Keypad8;
        case VK_NUMPAD9: return ImGuiKey_Keypad9;
        case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
        case VK_DIVIDE: return ImGuiKey_KeypadDivide;
        case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case VK_ADD: return ImGuiKey_KeypadAdd;
        case IM_VK_KEYPAD_ENTER: return ImGuiKey_KeypadEnter;
        case VK_LSHIFT: return ImGuiKey_LeftShift;
        case VK_LCONTROL: return ImGuiKey_LeftCtrl;
        case VK_LMENU: return ImGuiKey_LeftAlt;
        case VK_LWIN: return ImGuiKey_LeftSuper;
        case VK_RSHIFT: return ImGuiKey_RightShift;
        case VK_RCONTROL: return ImGuiKey_RightCtrl;
        case VK_RMENU: return ImGuiKey_RightAlt;
        case VK_RWIN: return ImGuiKey_RightSuper;
        case VK_APPS: return ImGuiKey_Menu;
        case '0': return ImGuiKey_0;
        case '1': return ImGuiKey_1;
        case '2': return ImGuiKey_2;
        case '3': return ImGuiKey_3;
        case '4': return ImGuiKey_4;
        case '5': return ImGuiKey_5;
        case '6': return ImGuiKey_6;
        case '7': return ImGuiKey_7;
        case '8': return ImGuiKey_8;
        case '9': return ImGuiKey_9;
        case 'A': return ImGuiKey_A;
        case 'B': return ImGuiKey_B;
        case 'C': return ImGuiKey_C;
        case 'D': return ImGuiKey_D;
        case 'E': return ImGuiKey_E;
        case 'F': return ImGuiKey_F;
        case 'G': return ImGuiKey_G;
        case 'H': return ImGuiKey_H;
        case 'I': return ImGuiKey_I;
        case 'J': return ImGuiKey_J;
        case 'K': return ImGuiKey_K;
        case 'L': return ImGuiKey_L;
        case 'M': return ImGuiKey_M;
        case 'N': return ImGuiKey_N;
        case 'O': return ImGuiKey_O;
        case 'P': return ImGuiKey_P;
        case 'Q': return ImGuiKey_Q;
        case 'R': return ImGuiKey_R;
        case 'S': return ImGuiKey_S;
        case 'T': return ImGuiKey_T;
        case 'U': return ImGuiKey_U;
        case 'V': return ImGuiKey_V;
        case 'W': return ImGuiKey_W;
        case 'X': return ImGuiKey_X;
        case 'Y': return ImGuiKey_Y;
        case 'Z': return ImGuiKey_Z;
        case VK_F1: return ImGuiKey_F1;
        case VK_F2: return ImGuiKey_F2;
        case VK_F3: return ImGuiKey_F3;
        case VK_F4: return ImGuiKey_F4;
        case VK_F5: return ImGuiKey_F5;
        case VK_F6: return ImGuiKey_F6;
        case VK_F7: return ImGuiKey_F7;
        case VK_F8: return ImGuiKey_F8;
        case VK_F9: return ImGuiKey_F9;
        case VK_F10: return ImGuiKey_F10;
        case VK_F11: return ImGuiKey_F11;
        case VK_F12: return ImGuiKey_F12;
        default: return ImGuiKey_None;
    }
}

// Allow compilation with old Windows SDK. MinGW doesn't have default _WIN32_WINNT/WINVER versions.
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef DBT_DEVNODES_CHANGED
#define DBT_DEVNODES_CHANGED 0x0007
#endif

// Win32 message handler (process Win32 mouse/keyboard inputs, etc.)
// Call from your application's message handler. Keep calling your message handler unless this function returns TRUE.
// When implementing your own backend, you can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if Dear ImGui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to Dear ImGui, and hide them from your application based on those two flags.
// PS: In this Win32 handler, we use the capture API (GetCapture/SetCapture/ReleaseCapture) to be able to read mouse coordinates when dragging mouse outside of our window bounds.
// PS: We treat DBLCLK messages as regular mouse down messages, so this code will work on windows classes that have the CS_DBLCLKS flag set. Our own example app code doesn't set this flag.
#if 0
// Copy this line into your .cpp file to forward declare the function.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() == nullptr)
        return 0;

    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();

    switch (msg)
    {
    case WM_MOUSEMOVE:
    {
        // We need to call TrackMouseEvent in order to receive WM_MOUSELEAVE events
        bd->MouseHwnd = hwnd;
        if (!bd->MouseTracked)
        {
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            ::TrackMouseEvent(&tme);
            bd->MouseTracked = true;
        }
        POINT mouse_pos = { (LONG)GET_X_LPARAM(lParam), (LONG)GET_Y_LPARAM(lParam) };
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            ::ClientToScreen(hwnd, &mouse_pos);
        io.AddMousePosEvent((float)mouse_pos.x, (float)mouse_pos.y);
        break;
    }
    case WM_MOUSELEAVE:
        if (bd->MouseHwnd == hwnd)
            bd->MouseHwnd = nullptr;
        bd->MouseTracked = false;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        break;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
    {
        int button = 0;
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
        if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
        if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
        if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
        if (bd->MouseButtonsDown == 0 && ::GetCapture() == nullptr)
            ::SetCapture(hwnd);
        bd->MouseButtonsDown |= 1 << button;
        io.AddMouseButtonEvent(button, true);
        return 0;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
    {
        int button = 0;
        if (msg == WM_LBUTTONUP) { button = 0; }
        if (msg == WM_RBUTTONUP) { button = 1; }
        if (msg == WM_MBUTTONUP) { button = 2; }
        if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
        bd->MouseButtonsDown &= ~(1 << button);
        if (bd->MouseButtonsDown == 0 && ::GetCapture() == hwnd)
            ::ReleaseCapture();
        io.AddMouseButtonEvent(button, false);
        return 0;
    }
    case WM_MOUSEWHEEL:
        io.AddMouseWheelEvent(0.0f, (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
        return 0;
    case WM_MOUSEHWHEEL:
        io.AddMouseWheelEvent((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, 0.0f);
        return 0;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        const bool is_key_down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
        if (wParam < 256)
        {
            // Submit modifiers
            ImGui_ImplWin32_UpdateKeyModifiers();

            // Obtain virtual key code
            // (keypad enter doesn't have its own... VK_RETURN with KF_EXTENDED flag means keypad enter, see IM_VK_KEYPAD_ENTER definition for details, it is mapped to ImGuiKey_KeyPadEnter.)
            int vk = (int)wParam;
            if ((wParam == VK_RETURN) && (HIWORD(lParam) & KF_EXTENDED))
                vk = IM_VK_KEYPAD_ENTER;

            // Submit key event
            const ImGuiKey key = ImGui_ImplWin32_VirtualKeyToImGuiKey(vk);
            const int scancode = (int)LOBYTE(HIWORD(lParam));
            if (key != ImGuiKey_None)
                ImGui_ImplWin32_AddKeyEvent(key, is_key_down, vk, scancode);

            // Submit individual left/right modifier events
            if (vk == VK_SHIFT)
            {
                // Important: Shift keys tend to get stuck when pressed together, missing key-up events are corrected in ImGui_ImplWin32_ProcessKeyEventsWorkarounds()
                if (IsVkDown(VK_LSHIFT) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftShift, is_key_down, VK_LSHIFT, scancode); }
                if (IsVkDown(VK_RSHIFT) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_RightShift, is_key_down, VK_RSHIFT, scancode); }
            }
            else if (vk == VK_CONTROL)
            {
                if (IsVkDown(VK_LCONTROL) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftCtrl, is_key_down, VK_LCONTROL, scancode); }
                if (IsVkDown(VK_RCONTROL) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_RightCtrl, is_key_down, VK_RCONTROL, scancode); }
            }
            else if (vk == VK_MENU)
            {
                if (IsVkDown(VK_LMENU) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftAlt, is_key_down, VK_LMENU, scancode); }
                if (IsVkDown(VK_RMENU) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_RightAlt, is_key_down, VK_RMENU, scancode); }
            }
        }
        return 0;
    }
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        io.AddFocusEvent(msg == WM_SETFOCUS);
        return 0;
    case WM_CHAR:
        if (::IsWindowUnicode(hwnd))
        {
            // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
            if (wParam > 0 && wParam < 0x10000)
                io.AddInputCharacterUTF16((unsigned short)wParam);
        }
        else
        {
            wchar_t wch = 0;
            ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)&wParam, 1, &wch, 1);
            io.AddInputCharacter(wch);
        }
        return 0;
    case WM_SETCURSOR:
        // This is required to restore cursor when transitioning from e.g resize borders to client area.
        if (LOWORD(lParam) == HTCLIENT && ImGui_ImplWin32_UpdateMouseCursor())
            return 1;
        return 0;
    case WM_DEVICECHANGE:
#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
        if ((UINT)wParam == DBT_DEVNODES_CHANGED)
            bd->WantUpdateHasGamepad = true;
#endif
        return 0;
    case WM_DISPLAYCHANGE:
        bd->WantUpdateMonitors = true;
        return 0;
    }
    return 0;
}


//--------------------------------------------------------------------------------------------------------
// DPI-related helpers (optional)
//--------------------------------------------------------------------------------------------------------
// - Use to enable DPI awareness without having to create an application manifest.
// - Your own app may already do this via a manifest or explicit calls. This is mostly useful for our examples/ apps.
// - In theory we could call simple functions from Windows SDK such as SetProcessDPIAware(), SetProcessDpiAwareness(), etc.
//   but most of the functions provided by Microsoft require Windows 8.1/10+ SDK at compile time and Windows 8/10+ at runtime,
//   neither we want to require the user to have. So we dynamically select and load those functions to avoid dependencies.
//---------------------------------------------------------------------------------------------------------
// This is the scheme successfully used by GLFW (from which we borrowed some of the code) and other apps aiming to be highly portable.
// ImGui_ImplWin32_EnableDpiAwareness() is just a helper called by main.cpp, we don't call it automatically.
// If you are trying to implement your own backend for your own engine, you may ignore that noise.
//---------------------------------------------------------------------------------------------------------

// Perform our own check with RtlVerifyVersionInfo() instead of using functions from <VersionHelpers.h> as they
// require a manifest to be functional for checks above 8.1. See https://github.com/ocornut/imgui/issues/4200
static BOOL _IsWindowsVersionOrGreater(WORD major, WORD minor, WORD)
{
    typedef LONG(WINAPI* PFN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);
    static PFN_RtlVerifyVersionInfo RtlVerifyVersionInfoFn = nullptr;
	if (RtlVerifyVersionInfoFn == nullptr)
		if (HMODULE ntdllModule = ::GetModuleHandleA("ntdll.dll"))
			RtlVerifyVersionInfoFn = (PFN_RtlVerifyVersionInfo)GetProcAddress(ntdllModule, "RtlVerifyVersionInfo");
    if (RtlVerifyVersionInfoFn == nullptr)
        return FALSE;

    RTL_OSVERSIONINFOEXW versionInfo = { };
    ULONGLONG conditionMask = 0;
    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    versionInfo.dwMajorVersion = major;
	versionInfo.dwMinorVersion = minor;
	VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
	return (RtlVerifyVersionInfoFn(&versionInfo, VER_MAJORVERSION | VER_MINORVERSION, conditionMask) == 0) ? TRUE : FALSE;
}

#define _IsWindowsVistaOrGreater()   _IsWindowsVersionOrGreater(HIBYTE(0x0600), LOBYTE(0x0600), 0) // _WIN32_WINNT_VISTA
#define _IsWindows8OrGreater()       _IsWindowsVersionOrGreater(HIBYTE(0x0602), LOBYTE(0x0602), 0) // _WIN32_WINNT_WIN8
#define _IsWindows8Point1OrGreater() _IsWindowsVersionOrGreater(HIBYTE(0x0603), LOBYTE(0x0603), 0) // _WIN32_WINNT_WINBLUE
#define _IsWindows10OrGreater()      _IsWindowsVersionOrGreater(HIBYTE(0x0A00), LOBYTE(0x0A00), 0) // _WIN32_WINNT_WINTHRESHOLD / _WIN32_WINNT_WIN10

#ifndef DPI_ENUMS_DECLARED
typedef enum { PROCESS_DPI_UNAWARE = 0, PROCESS_SYSTEM_DPI_AWARE = 1, PROCESS_PER_MONITOR_DPI_AWARE = 2 } PROCESS_DPI_AWARENESS;
typedef enum { MDT_EFFECTIVE_DPI = 0, MDT_ANGULAR_DPI = 1, MDT_RAW_DPI = 2, MDT_DEFAULT = MDT_EFFECTIVE_DPI } MONITOR_DPI_TYPE;
#endif
#ifndef _DPI_AWARENESS_CONTEXTS_
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    (DPI_AWARENESS_CONTEXT)-3
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (DPI_AWARENESS_CONTEXT)-4
#endif
typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);                     // Shcore.lib + dll, Windows 8.1+
typedef HRESULT(WINAPI* PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);        // Shcore.lib + dll, Windows 8.1+
typedef DPI_AWARENESS_CONTEXT(WINAPI* PFN_SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT); // User32.lib + dll, Windows 10 v1607+ (Creators Update)

// Helper function to enable DPI awareness without setting up a manifest
void ImGui_ImplWin32_EnableDpiAwareness()
{
    // Make sure monitors will be updated with latest correct scaling
    if (ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData())
        bd->WantUpdateMonitors = true;

    if (_IsWindows10OrGreater())
    {
        static HINSTANCE user32_dll = ::LoadLibraryA("user32.dll"); // Reference counted per-process
        if (PFN_SetThreadDpiAwarenessContext SetThreadDpiAwarenessContextFn = (PFN_SetThreadDpiAwarenessContext)::GetProcAddress(user32_dll, "SetThreadDpiAwarenessContext"))
        {
            SetThreadDpiAwarenessContextFn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }
    }
    if (_IsWindows8Point1OrGreater())
    {
        static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll"); // Reference counted per-process
        if (PFN_SetProcessDpiAwareness SetProcessDpiAwarenessFn = (PFN_SetProcessDpiAwareness)::GetProcAddress(shcore_dll, "SetProcessDpiAwareness"))
        {
            SetProcessDpiAwarenessFn(PROCESS_PER_MONITOR_DPI_AWARE);
            return;
        }
    }
#if _WIN32_WINNT >= 0x0600
    ::SetProcessDPIAware();
#endif
}

#if defined(_MSC_VER) && !defined(NOGDI)
#pragma comment(lib, "gdi32")   // Link with gdi32.lib for GetDeviceCaps(). MinGW will require linking with '-lgdi32'
#endif

float ImGui_ImplWin32_GetDpiScaleForMonitor(void* monitor)
{
    UINT xdpi = 96, ydpi = 96;
    if (_IsWindows8Point1OrGreater())
    {
		static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll"); // Reference counted per-process
		static PFN_GetDpiForMonitor GetDpiForMonitorFn = nullptr;
		if (GetDpiForMonitorFn == nullptr && shcore_dll != nullptr)
            GetDpiForMonitorFn = (PFN_GetDpiForMonitor)::GetProcAddress(shcore_dll, "GetDpiForMonitor");
		if (GetDpiForMonitorFn != nullptr)
		{
			GetDpiForMonitorFn((HMONITOR)monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
            IM_ASSERT(xdpi == ydpi); // Please contact me if you hit this assert!
			return xdpi / 96.0f;
		}
    }
#ifndef NOGDI
    const HDC dc = ::GetDC(nullptr);
    xdpi = ::GetDeviceCaps(dc, LOGPIXELSX);
    ydpi = ::GetDeviceCaps(dc, LOGPIXELSY);
    IM_ASSERT(xdpi == ydpi); // Please contact me if you hit this assert!
    ::ReleaseDC(nullptr, dc);
#endif
    return xdpi / 96.0f;
}

float ImGui_ImplWin32_GetDpiScaleForHwnd(void* hwnd)
{
    HMONITOR monitor = ::MonitorFromWindow((HWND)hwnd, MONITOR_DEFAULTTONEAREST);
    return ImGui_ImplWin32_GetDpiScaleForMonitor(monitor);
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

// Helper structure we store in the void* RenderUserData field of each ImGuiViewport to easily retrieve our backend data.
struct ImGui_ImplWin32_ViewportData
{
    HWND    Hwnd;
    bool    HwndOwned;
    DWORD   DwStyle;
    DWORD   DwExStyle;

    ImGui_ImplWin32_ViewportData() { Hwnd = nullptr; HwndOwned = false;  DwStyle = DwExStyle = 0; }
    ~ImGui_ImplWin32_ViewportData() { IM_ASSERT(Hwnd == nullptr); }
};

static void ImGui_ImplWin32_GetWin32StyleFromViewportFlags(ImGuiViewportFlags flags, DWORD* out_style, DWORD* out_ex_style)
{
    if (flags & ImGuiViewportFlags_NoDecoration)
        *out_style = WS_POPUP;
    else
        *out_style = WS_OVERLAPPEDWINDOW;

    if (flags & ImGuiViewportFlags_NoTaskBarIcon)
        *out_ex_style = WS_EX_TOOLWINDOW;
    else
        *out_ex_style = WS_EX_APPWINDOW;

    if (flags & ImGuiViewportFlags_TopMost)
        *out_ex_style |= WS_EX_TOPMOST;
}

static void ImGui_ImplWin32_CreateWindow(ImGuiViewport* viewport)
{
    ImGui_ImplWin32_ViewportData* vd = IM_NEW(ImGui_ImplWin32_ViewportData)();
    viewport->PlatformUserData = vd;

    // Select style and parent window
    ImGui_ImplWin32_GetWin32StyleFromViewportFlags(viewport->Flags, &vd->DwStyle, &vd->DwExStyle);
    HWND parent_window = nullptr;
    if (viewport->ParentViewportId != 0)
        if (ImGuiViewport* parent_viewport = ImGui::FindViewportByID(viewport->ParentViewportId))
            parent_window = (HWND)parent_viewport->PlatformHandle;

    // Create window
    RECT rect = { (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)(viewport->Pos.x + viewport->Size.x), (LONG)(viewport->Pos.y + viewport->Size.y) };
    ::AdjustWindowRectEx(&rect, vd->DwStyle, FALSE, vd->DwExStyle);
    vd->Hwnd = ::CreateWindowEx(
        vd->DwExStyle, _T("ImGui Platform"), _T("Untitled"), vd->DwStyle,       // Style, class name, window name
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,    // Window area
        parent_window, nullptr, ::GetModuleHandle(nullptr), nullptr);           // Parent window, Menu, Instance, Param
    vd->HwndOwned = true;
    viewport->PlatformRequestResize = false;
    viewport->PlatformHandle = viewport->PlatformHandleRaw = vd->Hwnd;
}

static void ImGui_ImplWin32_DestroyWindow(ImGuiViewport* viewport)
{
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
    if (ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData)
    {
        if (::GetCapture() == vd->Hwnd)
        {
            // Transfer capture so if we started dragging from a window that later disappears, we'll still receive the MOUSEUP event.
            ::ReleaseCapture();
            ::SetCapture(bd->hWnd);
        }
        if (vd->Hwnd && vd->HwndOwned)
            ::DestroyWindow(vd->Hwnd);
        vd->Hwnd = nullptr;
        IM_DELETE(vd);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = nullptr;
}

static void ImGui_ImplWin32_ShowWindow(ImGuiViewport* viewport)
{
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
        ::ShowWindow(vd->Hwnd, SW_SHOWNA);
    else
        ::ShowWindow(vd->Hwnd, SW_SHOW);
}

static void ImGui_ImplWin32_UpdateWindow(ImGuiViewport* viewport)
{
    // (Optional) Update Win32 style if it changed _after_ creation.
    // Generally they won't change unless configuration flags are changed, but advanced uses (such as manually rewriting viewport flags) make this useful.
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    DWORD new_style;
    DWORD new_ex_style;
    ImGui_ImplWin32_GetWin32StyleFromViewportFlags(viewport->Flags, &new_style, &new_ex_style);

    // Only reapply the flags that have been changed from our point of view (as other flags are being modified by Windows)
    if (vd->DwStyle != new_style || vd->DwExStyle != new_ex_style)
    {
        // (Optional) Update TopMost state if it changed _after_ creation
        bool top_most_changed = (vd->DwExStyle & WS_EX_TOPMOST) != (new_ex_style & WS_EX_TOPMOST);
        HWND insert_after = top_most_changed ? ((viewport->Flags & ImGuiViewportFlags_TopMost) ? HWND_TOPMOST : HWND_NOTOPMOST) : 0;
        UINT swp_flag = top_most_changed ? 0 : SWP_NOZORDER;

        // Apply flags and position (since it is affected by flags)
        vd->DwStyle = new_style;
        vd->DwExStyle = new_ex_style;
        ::SetWindowLong(vd->Hwnd, GWL_STYLE, vd->DwStyle);
        ::SetWindowLong(vd->Hwnd, GWL_EXSTYLE, vd->DwExStyle);
        RECT rect = { (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)(viewport->Pos.x + viewport->Size.x), (LONG)(viewport->Pos.y + viewport->Size.y) };
        ::AdjustWindowRectEx(&rect, vd->DwStyle, FALSE, vd->DwExStyle); // Client to Screen
        ::SetWindowPos(vd->Hwnd, insert_after, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, swp_flag | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        ::ShowWindow(vd->Hwnd, SW_SHOWNA); // This is necessary when we alter the style
        viewport->PlatformRequestMove = viewport->PlatformRequestResize = true;
    }
}

static ImVec2 ImGui_ImplWin32_GetWindowPos(ImGuiViewport* viewport)
{
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    POINT pos = { 0, 0 };
    ::ClientToScreen(vd->Hwnd, &pos);
    return ImVec2((float)pos.x, (float)pos.y);
}

static void ImGui_ImplWin32_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    RECT rect = { (LONG)pos.x, (LONG)pos.y, (LONG)pos.x, (LONG)pos.y };
    ::AdjustWindowRectEx(&rect, vd->DwStyle, FALSE, vd->DwExStyle);
    ::SetWindowPos(vd->Hwnd, NULL, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

static ImVec2 ImGui_ImplWin32_GetWindowSize(ImGuiViewport* viewport)
{
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    RECT rect;
    ::GetClientRect(vd->Hwnd, &rect);
    return ImVec2(float(rect.right - rect.left), float(rect.bottom - rect.top));
}

static void ImGui_ImplWin32_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    RECT rect = { 0, 0, (LONG)size.x, (LONG)size.y };
    ::AdjustWindowRectEx(&rect, vd->DwStyle, FALSE, vd->DwExStyle); // Client to Screen
    ::SetWindowPos(vd->Hwnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
}

static void ImGui_ImplWin32_SetWindowFocus(ImGuiViewport* viewport)
{
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    ::BringWindowToTop(vd->Hwnd);
    ::SetForegroundWindow(vd->Hwnd);
    ::SetFocus(vd->Hwnd);
}

static bool ImGui_ImplWin32_GetWindowFocus(ImGuiViewport* viewport)
{
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    return ::GetForegroundWindow() == vd->Hwnd;
}

static bool ImGui_ImplWin32_GetWindowMinimized(ImGuiViewport* viewport)
{
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    return ::IsIconic(vd->Hwnd) != 0;
}

static void ImGui_ImplWin32_SetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    // ::SetWindowTextA() doesn't properly handle UTF-8 so we explicitely convert our string.
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    int n = ::MultiByteToWideChar(CP_UTF8, 0, title, -1, nullptr, 0);
    ImVector<wchar_t> title_w;
    title_w.resize(n);
    ::MultiByteToWideChar(CP_UTF8, 0, title, -1, title_w.Data, n);
    ::SetWindowTextW(vd->Hwnd, title_w.Data);
}

static void ImGui_ImplWin32_SetWindowAlpha(ImGuiViewport* viewport, float alpha)
{
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    IM_ASSERT(alpha >= 0.0f && alpha <= 1.0f);
    if (alpha < 1.0f)
    {
        DWORD style = ::GetWindowLongW(vd->Hwnd, GWL_EXSTYLE) | WS_EX_LAYERED;
        ::SetWindowLongW(vd->Hwnd, GWL_EXSTYLE, style);
        ::SetLayeredWindowAttributes(vd->Hwnd, 0, (BYTE)(255 * alpha), LWA_ALPHA);
    }
    else
    {
        DWORD style = ::GetWindowLongW(vd->Hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED;
        ::SetWindowLongW(vd->Hwnd, GWL_EXSTYLE, style);
    }
}

static float ImGui_ImplWin32_GetWindowDpiScale(ImGuiViewport* viewport)
{
    ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
    IM_ASSERT(vd->Hwnd != 0);
    return ImGui_ImplWin32_GetDpiScaleForHwnd(vd->Hwnd);
}

// FIXME-DPI: Testing DPI related ideas
static void ImGui_ImplWin32_OnChangedViewport(ImGuiViewport* viewport)
{
    (void)viewport;
#if 0
    ImGuiStyle default_style;
    //default_style.WindowPadding = ImVec2(0, 0);
    //default_style.WindowBorderSize = 0.0f;
    //default_style.ItemSpacing.y = 3.0f;
    //default_style.FramePadding = ImVec2(0, 0);
    default_style.ScaleAllSizes(viewport->DpiScale);
    ImGuiStyle& style = ImGui::GetStyle();
    style = default_style;
#endif
}

static LRESULT CALLBACK ImGui_ImplWin32_WndProcHandler_PlatformWindow(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void*)hWnd))
    {
        switch (msg)
        {
        case WM_CLOSE:
            viewport->PlatformRequestClose = true;
            return 0;
        case WM_MOVE:
            viewport->PlatformRequestMove = true;
            break;
        case WM_SIZE:
            viewport->PlatformRequestResize = true;
            break;
        case WM_MOUSEACTIVATE:
            if (viewport->Flags & ImGuiViewportFlags_NoFocusOnClick)
                return MA_NOACTIVATE;
            break;
        case WM_NCHITTEST:
            // Let mouse pass-through the window. This will allow the backend to call io.AddMouseViewportEvent() correctly. (which is optional).
            // The ImGuiViewportFlags_NoInputs flag is set while dragging a viewport, as want to detect the window behind the one we are dragging.
            // If you cannot easily access those viewport flags from your windowing/event code: you may manually synchronize its state e.g. in
            // your main loop after calling UpdatePlatformWindows(). Iterate all viewports/platform windows and pass the flag to your windowing system.
            if (viewport->Flags & ImGuiViewportFlags_NoInputs)
                return HTTRANSPARENT;
            break;
        }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void ImGui_ImplWin32_InitPlatformInterface()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = ImGui_ImplWin32_WndProcHandler_PlatformWindow;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = ::GetModuleHandle(nullptr);
    wcex.hIcon = nullptr;
    wcex.hCursor = nullptr;
    wcex.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = _T("ImGui Platform");
    wcex.hIconSm = nullptr;
    ::RegisterClassEx(&wcex);

    ImGui_ImplWin32_UpdateMonitors();

    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateWindow = ImGui_ImplWin32_CreateWindow;
    platform_io.Platform_DestroyWindow = ImGui_ImplWin32_DestroyWindow;
    platform_io.Platform_ShowWindow = ImGui_ImplWin32_ShowWindow;
    platform_io.Platform_SetWindowPos = ImGui_ImplWin32_SetWindowPos;
    platform_io.Platform_GetWindowPos = ImGui_ImplWin32_GetWindowPos;
    platform_io.Platform_SetWindowSize = ImGui_ImplWin32_SetWindowSize;
    platform_io.Platform_GetWindowSize = ImGui_ImplWin32_GetWindowSize;
    platform_io.Platform_SetWindowFocus = ImGui_ImplWin32_SetWindowFocus;
    platform_io.Platform_GetWindowFocus = ImGui_ImplWin32_GetWindowFocus;
    platform_io.Platform_GetWindowMinimized = ImGui_ImplWin32_GetWindowMinimized;
    platform_io.Platform_SetWindowTitle = ImGui_ImplWin32_SetWindowTitle;
    platform_io.Platform_SetWindowAlpha = ImGui_ImplWin32_SetWindowAlpha;
    platform_io.Platform_UpdateWindow = ImGui_ImplWin32_UpdateWindow;
    platform_io.Platform_GetWindowDpiScale = ImGui_ImplWin32_GetWindowDpiScale; // FIXME-DPI
    platform_io.Platform_OnChangedViewport = ImGui_ImplWin32_OnChangedViewport; // FIXME-DPI

    // Register main window handle (which is owned by the main application, not by us)
    // This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.) can use same logic for main and secondary viewports.
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
    ImGui_ImplWin32_ViewportData* vd = IM_NEW(ImGui_ImplWin32_ViewportData)();
    vd->Hwnd = bd->hWnd;
    vd->HwndOwned = false;
    main_viewport->PlatformUserData = vd;
    main_viewport->PlatformHandle = (void*)bd->hWnd;
}

static void ImGui_ImplWin32_ShutdownPlatformInterface()
{
    ::UnregisterClass(_T("ImGui Platform"), ::GetModuleHandle(nullptr));
    ImGui::DestroyPlatformWindows();
}

//---------------------------------------------------------------------------------------------------------
// Transparency related helpers (optional)
//--------------------------------------------------------------------------------------------------------

#if defined(_MSC_VER)
#pragma comment(lib, "dwmapi")  // Link with dwmapi.lib. MinGW will require linking with '-ldwmapi'
#endif

// [experimental]
// Borrowed from GLFW's function updateFramebufferTransparency() in src/win32_window.c
// (the Dwm* functions are Vista era functions but we are borrowing logic from GLFW)
void ImGui_ImplWin32_EnableAlphaCompositing(void* hwnd)
{
    if (!_IsWindowsVistaOrGreater())
        return;

    BOOL composition;
    if (FAILED(::DwmIsCompositionEnabled(&composition)) || !composition)
        return;

    BOOL opaque;
    DWORD color;
    if (_IsWindows8OrGreater() || (SUCCEEDED(::DwmGetColorizationColor(&color, &opaque)) && !opaque))
    {
        HRGN region = ::CreateRectRgn(0, 0, -1, -1);
        DWM_BLURBEHIND bb = {};
        bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
        bb.hRgnBlur = region;
        bb.fEnable = TRUE;
        ::DwmEnableBlurBehindWindow((HWND)hwnd, &bb);
        ::DeleteObject(region);
    }
    else
    {
        DWM_BLURBEHIND bb = {};
        bb.dwFlags = DWM_BB_ENABLE;
        ::DwmEnableBlurBehindWindow((HWND)hwnd, &bb);
    }
}

//---------------------------------------------------------------------------------------------------------
