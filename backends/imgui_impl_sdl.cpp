// dear imgui: Platform Backend for SDL2
// This needs to be used along with a Renderer (e.g. DirectX11, OpenGL3, Vulkan..)
// (Info: SDL2 is a cross-platform general purpose library for handling windows, inputs, graphics context creation, etc.)
// (Prefer SDL 2.0.5+ for full feature support.)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy SDL_SCANCODE_* values will also be supported unless IMGUI_DISABLE_OBSOLETE_KEYIO is set]
//  [X] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [X] Platform: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.
// Missing features:
//  [ ] Platform: SDL2 handling of IME under Windows appears to be broken and it explicitly disable the regular Windows IME. You can restore Windows IME by compiling SDL with SDL_DISABLE_WINDOWS_IME.
//  [ ] Platform: Multi-viewport + Minimized windows seems to break mouse wheel events (at least under Windows).

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2022-XX-XX: Platform: Added support for multiple windows via the ImGuiPlatformIO interface.
//  2022-03-22: Inputs: Fix mouse position issues when dragging outside of boundaries. SDL_CaptureMouse() erroneously still gives out LEAVE events when hovering OS decorations.
//  2022-03-22: Inputs: Added support for extra mouse buttons (SDL_BUTTON_X1/SDL_BUTTON_X2).
//  2022-02-04: Added SDL_Renderer* parameter to ImGui_ImplSDL2_InitForSDLRenderer(), so we can use SDL_GetRendererOutputSize() instead of SDL_GL_GetDrawableSize() when bound to a SDL_Renderer.
//  2022-01-26: Inputs: replaced short-lived io.AddKeyModsEvent() (added two weeks ago) with io.AddKeyEvent() using ImGuiKey_ModXXX flags. Sorry for the confusion.
//  2021-01-20: Inputs: calling new io.AddKeyAnalogEvent() for gamepad support, instead of writing directly to io.NavInputs[].
//  2022-01-17: Inputs: calling new io.AddMousePosEvent(), io.AddMouseButtonEvent(), io.AddMouseWheelEvent() API (1.87+).
//  2022-01-17: Inputs: always update key mods next and before key event (not in NewFrame) to fix input queue with very low framerates.
//  2022-01-12: Update mouse inputs using SDL_MOUSEMOTION/SDL_WINDOWEVENT_LEAVE + fallback to provide it when focused but not hovered/captured. More standard and will allow us to pass it to future input queue API.
//  2022-01-12: Maintain our own copy of MouseButtonsDown mask instead of using ImGui::IsAnyMouseDown() which will be obsoleted.
//  2022-01-10: Inputs: calling new io.AddKeyEvent(), io.AddKeyModsEvent() + io.SetKeyEventNativeData() API (1.87+). Support for full ImGuiKey range.
//  2021-08-17: Calling io.AddFocusEvent() on SDL_WINDOWEVENT_FOCUS_GAINED/SDL_WINDOWEVENT_FOCUS_LOST.
//  2021-07-29: Inputs: MousePos is correctly reported when the host platform window is hovered but not focused (using SDL_GetMouseFocus() + SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, requires SDL 2.0.5+)
//  2021-06:29: *BREAKING CHANGE* Removed 'SDL_Window* window' parameter to ImGui_ImplSDL2_NewFrame() which was unnecessary.
//  2021-06-29: Reorganized backend to pull data from a single structure to facilitate usage with multiple-contexts (all g_XXXX access changed to bd->XXXX).
//  2021-03-22: Rework global mouse pos availability check listing supported platforms explicitly, effectively fixing mouse access on Raspberry Pi. (#2837, #3950)
//  2020-05-25: Misc: Report a zero display-size when window is minimized, to be consistent with other backends.
//  2020-02-20: Inputs: Fixed mapping for ImGuiKey_KeyPadEnter (using SDL_SCANCODE_KP_ENTER instead of SDL_SCANCODE_RETURN2).
//  2019-12-17: Inputs: On Wayland, use SDL_GetMouseState (because there is no global mouse state).
//  2019-12-05: Inputs: Added support for ImGuiMouseCursor_NotAllowed mouse cursor.
//  2019-07-21: Inputs: Added mapping for ImGuiKey_KeyPadEnter.
//  2019-04-23: Inputs: Added support for SDL_GameController (if ImGuiConfigFlags_NavEnableGamepad is set by user application).
//  2019-03-12: Misc: Preserve DisplayFramebufferScale when main window is minimized.
//  2018-12-21: Inputs: Workaround for Android/iOS which don't seem to handle focus related calls.
//  2018-11-30: Misc: Setting up io.BackendPlatformName so it can be displayed in the About Window.
//  2018-11-14: Changed the signature of ImGui_ImplSDL2_ProcessEvent() to take a 'const SDL_Event*'.
//  2018-08-01: Inputs: Workaround for Emscripten which doesn't seem to handle focus related calls.
//  2018-06-29: Inputs: Added support for the ImGuiMouseCursor_Hand cursor.
//  2018-06-08: Misc: Extracted imgui_impl_sdl.cpp/.h away from the old combined SDL2+OpenGL/Vulkan examples.
//  2018-06-08: Misc: ImGui_ImplSDL2_InitForOpenGL() now takes a SDL_GLContext parameter.
//  2018-05-09: Misc: Fixed clipboard paste memory leak (we didn't call SDL_FreeMemory on the data returned by SDL_GetClipboardText).
//  2018-03-20: Misc: Setup io.BackendFlags ImGuiBackendFlags_HasMouseCursors flag + honor ImGuiConfigFlags_NoMouseCursorChange flag.
//  2018-02-16: Inputs: Added support for mouse cursors, honoring ImGui::GetMouseCursor() value.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2018-02-06: Inputs: Added mapping for ImGuiKey_Space.
//  2018-02-05: Misc: Using SDL_GetPerformanceCounter() instead of SDL_GetTicks() to be able to handle very high framerate (1000+ FPS).
//  2018-02-05: Inputs: Keyboard mapping is using scancodes everywhere instead of a confusing mixture of keycodes and scancodes.
//  2018-01-20: Inputs: Added Horizontal Mouse Wheel support.
//  2018-01-19: Inputs: When available (SDL 2.0.4+) using SDL_CaptureMouse() to retrieve coordinates outside of client area when dragging. Otherwise (SDL 2.0.3 and before) testing for SDL_WINDOW_INPUT_FOCUS instead of SDL_WINDOW_MOUSE_FOCUS.
//  2018-01-18: Inputs: Added mapping for ImGuiKey_Insert.
//  2017-08-25: Inputs: MousePos set to -FLT_MAX,-FLT_MAX when mouse is unavailable/missing (instead of -1,-1).
//  2016-10-15: Misc: Added a void* user_data parameter to Clipboard function handlers.

#include "imgui.h"
#include "imgui_impl_sdl.h"

// SDL
// (the multi-viewports feature requires SDL features supported from SDL 2.0.4+. SDL 2.0.5+ is highly recommended)
#include <SDL.h>
#include <SDL_syswm.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if SDL_VERSION_ATLEAST(2,0,4) && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && !(defined(__APPLE__) && TARGET_OS_IOS) && !defined(__amigaos4__)
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    1
#else
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    0
#endif
#define SDL_HAS_MOUSE_FOCUS_CLICKTHROUGH    SDL_VERSION_ATLEAST(2,0,5)
#define SDL_HAS_WINDOW_ALPHA                SDL_VERSION_ATLEAST(2,0,5)
#define SDL_HAS_ALWAYS_ON_TOP               SDL_VERSION_ATLEAST(2,0,5)
#define SDL_HAS_USABLE_DISPLAY_BOUNDS       SDL_VERSION_ATLEAST(2,0,5)
#define SDL_HAS_PER_MONITOR_DPI             SDL_VERSION_ATLEAST(2,0,4)
#define SDL_HAS_VULKAN                      SDL_VERSION_ATLEAST(2,0,6)
#if !SDL_HAS_VULKAN
static const Uint32 SDL_WINDOW_VULKAN = 0x10000000;
#endif

// SDL Data
struct ImGui_ImplSDL2_Data
{
    SDL_Window*     Window;
    SDL_Renderer*   Renderer;
    Uint64          Time;
    Uint32          MouseWindowID;
    int             MouseButtonsDown;
    SDL_Cursor*     MouseCursors[ImGuiMouseCursor_COUNT];
    int             PendingMouseLeaveFrame;
    char*           ClipboardTextData;
    bool            MouseCanUseGlobalState;
    bool            UseVulkan;

    ImGui_ImplSDL2_Data()   { memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
static ImGui_ImplSDL2_Data* ImGui_ImplSDL2_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplSDL2_Data*)ImGui::GetIO().BackendPlatformUserData : NULL;
}

// Forward Declarations
static void ImGui_ImplSDL2_UpdateMonitors();
static void ImGui_ImplSDL2_InitPlatformInterface(SDL_Window* window, void* sdl_gl_context);
static void ImGui_ImplSDL2_ShutdownPlatformInterface();

// Functions
static const char* ImGui_ImplSDL2_GetClipboardText(void*)
{
    ImGui_ImplSDL2_Data* bd = ImGui_ImplSDL2_GetBackendData();
    if (bd->ClipboardTextData)
        SDL_free(bd->ClipboardTextData);
    bd->ClipboardTextData = SDL_GetClipboardText();
    return bd->ClipboardTextData;
}

static void ImGui_ImplSDL2_SetClipboardText(void*, const char* text)
{
    SDL_SetClipboardText(text);
}

static ImGuiKey ImGui_ImplSDL2_KeycodeToImGuiKey(int keycode)
{
    switch (keycode)
    {
        case SDLK_TAB: return ImGuiKey_Tab;
        case SDLK_LEFT: return ImGuiKey_LeftArrow;
        case SDLK_RIGHT: return ImGuiKey_RightArrow;
        case SDLK_UP: return ImGuiKey_UpArrow;
        case SDLK_DOWN: return ImGuiKey_DownArrow;
        case SDLK_PAGEUP: return ImGuiKey_PageUp;
        case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
        case SDLK_HOME: return ImGuiKey_Home;
        case SDLK_END: return ImGuiKey_End;
        case SDLK_INSERT: return ImGuiKey_Insert;
        case SDLK_DELETE: return ImGuiKey_Delete;
        case SDLK_BACKSPACE: return ImGuiKey_Backspace;
        case SDLK_SPACE: return ImGuiKey_Space;
        case SDLK_RETURN: return ImGuiKey_Enter;
        case SDLK_ESCAPE: return ImGuiKey_Escape;
        case SDLK_QUOTE: return ImGuiKey_Apostrophe;
        case SDLK_COMMA: return ImGuiKey_Comma;
        case SDLK_MINUS: return ImGuiKey_Minus;
        case SDLK_PERIOD: return ImGuiKey_Period;
        case SDLK_SLASH: return ImGuiKey_Slash;
        case SDLK_SEMICOLON: return ImGuiKey_Semicolon;
        case SDLK_EQUALS: return ImGuiKey_Equal;
        case SDLK_LEFTBRACKET: return ImGuiKey_LeftBracket;
        case SDLK_BACKSLASH: return ImGuiKey_Backslash;
        case SDLK_RIGHTBRACKET: return ImGuiKey_RightBracket;
        case SDLK_BACKQUOTE: return ImGuiKey_GraveAccent;
        case SDLK_CAPSLOCK: return ImGuiKey_CapsLock;
        case SDLK_SCROLLLOCK: return ImGuiKey_ScrollLock;
        case SDLK_NUMLOCKCLEAR: return ImGuiKey_NumLock;
        case SDLK_PRINTSCREEN: return ImGuiKey_PrintScreen;
        case SDLK_PAUSE: return ImGuiKey_Pause;
        case SDLK_KP_0: return ImGuiKey_Keypad0;
        case SDLK_KP_1: return ImGuiKey_Keypad1;
        case SDLK_KP_2: return ImGuiKey_Keypad2;
        case SDLK_KP_3: return ImGuiKey_Keypad3;
        case SDLK_KP_4: return ImGuiKey_Keypad4;
        case SDLK_KP_5: return ImGuiKey_Keypad5;
        case SDLK_KP_6: return ImGuiKey_Keypad6;
        case SDLK_KP_7: return ImGuiKey_Keypad7;
        case SDLK_KP_8: return ImGuiKey_Keypad8;
        case SDLK_KP_9: return ImGuiKey_Keypad9;
        case SDLK_KP_PERIOD: return ImGuiKey_KeypadDecimal;
        case SDLK_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case SDLK_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case SDLK_KP_MINUS: return ImGuiKey_KeypadSubtract;
        case SDLK_KP_PLUS: return ImGuiKey_KeypadAdd;
        case SDLK_KP_ENTER: return ImGuiKey_KeypadEnter;
        case SDLK_KP_EQUALS: return ImGuiKey_KeypadEqual;
        case SDLK_LCTRL: return ImGuiKey_LeftCtrl;
        case SDLK_LSHIFT: return ImGuiKey_LeftShift;
        case SDLK_LALT: return ImGuiKey_LeftAlt;
        case SDLK_LGUI: return ImGuiKey_LeftSuper;
        case SDLK_RCTRL: return ImGuiKey_RightCtrl;
        case SDLK_RSHIFT: return ImGuiKey_RightShift;
        case SDLK_RALT: return ImGuiKey_RightAlt;
        case SDLK_RGUI: return ImGuiKey_RightSuper;
        case SDLK_APPLICATION: return ImGuiKey_Menu;
        case SDLK_0: return ImGuiKey_0;
        case SDLK_1: return ImGuiKey_1;
        case SDLK_2: return ImGuiKey_2;
        case SDLK_3: return ImGuiKey_3;
        case SDLK_4: return ImGuiKey_4;
        case SDLK_5: return ImGuiKey_5;
        case SDLK_6: return ImGuiKey_6;
        case SDLK_7: return ImGuiKey_7;
        case SDLK_8: return ImGuiKey_8;
        case SDLK_9: return ImGuiKey_9;
        case SDLK_a: return ImGuiKey_A;
        case SDLK_b: return ImGuiKey_B;
        case SDLK_c: return ImGuiKey_C;
        case SDLK_d: return ImGuiKey_D;
        case SDLK_e: return ImGuiKey_E;
        case SDLK_f: return ImGuiKey_F;
        case SDLK_g: return ImGuiKey_G;
        case SDLK_h: return ImGuiKey_H;
        case SDLK_i: return ImGuiKey_I;
        case SDLK_j: return ImGuiKey_J;
        case SDLK_k: return ImGuiKey_K;
        case SDLK_l: return ImGuiKey_L;
        case SDLK_m: return ImGuiKey_M;
        case SDLK_n: return ImGuiKey_N;
        case SDLK_o: return ImGuiKey_O;
        case SDLK_p: return ImGuiKey_P;
        case SDLK_q: return ImGuiKey_Q;
        case SDLK_r: return ImGuiKey_R;
        case SDLK_s: return ImGuiKey_S;
        case SDLK_t: return ImGuiKey_T;
        case SDLK_u: return ImGuiKey_U;
        case SDLK_v: return ImGuiKey_V;
        case SDLK_w: return ImGuiKey_W;
        case SDLK_x: return ImGuiKey_X;
        case SDLK_y: return ImGuiKey_Y;
        case SDLK_z: return ImGuiKey_Z;
        case SDLK_F1: return ImGuiKey_F1;
        case SDLK_F2: return ImGuiKey_F2;
        case SDLK_F3: return ImGuiKey_F3;
        case SDLK_F4: return ImGuiKey_F4;
        case SDLK_F5: return ImGuiKey_F5;
        case SDLK_F6: return ImGuiKey_F6;
        case SDLK_F7: return ImGuiKey_F7;
        case SDLK_F8: return ImGuiKey_F8;
        case SDLK_F9: return ImGuiKey_F9;
        case SDLK_F10: return ImGuiKey_F10;
        case SDLK_F11: return ImGuiKey_F11;
        case SDLK_F12: return ImGuiKey_F12;
    }
    return ImGuiKey_None;
}

static void ImGui_ImplSDL2_UpdateKeyModifiers(SDL_Keymod sdl_key_mods)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiKey_ModCtrl, (sdl_key_mods & KMOD_CTRL) != 0);
    io.AddKeyEvent(ImGuiKey_ModShift, (sdl_key_mods & KMOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiKey_ModAlt, (sdl_key_mods & KMOD_ALT) != 0);
    io.AddKeyEvent(ImGuiKey_ModSuper, (sdl_key_mods & KMOD_GUI) != 0);
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
// If you have multiple SDL events and some of them are not meant to be used by dear imgui, you may need to filter events based on their windowID field.
bool ImGui_ImplSDL2_ProcessEvent(const SDL_Event* event)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL2_Data* bd = ImGui_ImplSDL2_GetBackendData();

    switch (event->type)
    {
        case SDL_MOUSEMOTION:
        {
            ImVec2 mouse_pos((float)event->motion.x, (float)event->motion.y);
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                int window_x, window_y;
                SDL_GetWindowPosition(SDL_GetWindowFromID(event->motion.windowID), &window_x, &window_y);
                mouse_pos.x += window_x;
                mouse_pos.y += window_y;
            }
            io.AddMousePosEvent(mouse_pos.x, mouse_pos.y);
            return true;
        }
        case SDL_MOUSEWHEEL:
        {
            float wheel_x = (event->wheel.x > 0) ? 1.0f : (event->wheel.x < 0) ? -1.0f : 0.0f;
            float wheel_y = (event->wheel.y > 0) ? 1.0f : (event->wheel.y < 0) ? -1.0f : 0.0f;
            io.AddMouseWheelEvent(wheel_x, wheel_y);
            return true;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        {
            int mouse_button = -1;
            if (event->button.button == SDL_BUTTON_LEFT) { mouse_button = 0; }
            if (event->button.button == SDL_BUTTON_RIGHT) { mouse_button = 1; }
            if (event->button.button == SDL_BUTTON_MIDDLE) { mouse_button = 2; }
            if (event->button.button == SDL_BUTTON_X1) { mouse_button = 3; }
            if (event->button.button == SDL_BUTTON_X2) { mouse_button = 4; }
            if (mouse_button == -1)
                break;
            io.AddMouseButtonEvent(mouse_button, (event->type == SDL_MOUSEBUTTONDOWN));
            bd->MouseButtonsDown = (event->type == SDL_MOUSEBUTTONDOWN) ? (bd->MouseButtonsDown | (1 << mouse_button)) : (bd->MouseButtonsDown & ~(1 << mouse_button));
            return true;
        }
        case SDL_TEXTINPUT:
        {
            io.AddInputCharactersUTF8(event->text.text);
            return true;
        }
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            ImGui_ImplSDL2_UpdateKeyModifiers((SDL_Keymod)event->key.keysym.mod);
            ImGuiKey key = ImGui_ImplSDL2_KeycodeToImGuiKey(event->key.keysym.sym);
            io.AddKeyEvent(key, (event->type == SDL_KEYDOWN));
            io.SetKeyEventNativeData(key, event->key.keysym.sym, event->key.keysym.scancode, event->key.keysym.scancode); // To support legacy indexing (<1.87 user code). Legacy backend uses SDLK_*** as indices to IsKeyXXX() functions.
            return true;
        }
        case SDL_WINDOWEVENT:
        {
            // - When capturing mouse, SDL will send a bunch of conflicting LEAVE/ENTER event on every mouse move, but the final ENTER tends to be right.
            // - However we won't get a correct LEAVE event for a captured window.
            // - In some cases, when detaching a window from main viewport SDL may send SDL_WINDOWEVENT_ENTER one frame too late,
            //   causing SDL_WINDOWEVENT_LEAVE on previous frame to interrupt drag operation by clear mouse position. This is why
            //   we delay process the SDL_WINDOWEVENT_LEAVE events by one frame. See issue #5012 for details.
            Uint8 window_event = event->window.event;
            if (window_event == SDL_WINDOWEVENT_ENTER)
            {
                bd->MouseWindowID = event->window.windowID;
                bd->PendingMouseLeaveFrame = 0;
            }
            if (window_event == SDL_WINDOWEVENT_LEAVE)
                bd->PendingMouseLeaveFrame = ImGui::GetFrameCount() + 1;
            if (window_event == SDL_WINDOWEVENT_FOCUS_GAINED)
                io.AddFocusEvent(true);
            else if (window_event == SDL_WINDOWEVENT_FOCUS_LOST)
                io.AddFocusEvent(false);
            if (window_event == SDL_WINDOWEVENT_CLOSE || window_event == SDL_WINDOWEVENT_MOVED || window_event == SDL_WINDOWEVENT_RESIZED)
                if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void*)SDL_GetWindowFromID(event->window.windowID)))
                {
                    if (window_event == SDL_WINDOWEVENT_CLOSE)
                        viewport->PlatformRequestClose = true;
                    if (window_event == SDL_WINDOWEVENT_MOVED)
                        viewport->PlatformRequestMove = true;
                    if (window_event == SDL_WINDOWEVENT_RESIZED)
                        viewport->PlatformRequestResize = true;
                    return true;
                }
            return true;
        }
    }
    return false;
}

static bool ImGui_ImplSDL2_Init(SDL_Window* window, SDL_Renderer* renderer, void* sdl_gl_context)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == NULL && "Already initialized a platform backend!");

    // Check and store if we are on a SDL backend that supports global mouse position
    // ("wayland" and "rpi" don't support it, but we chose to use a white-list instead of a black-list)
    bool mouse_can_use_global_state = false;
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    const char* sdl_backend = SDL_GetCurrentVideoDriver();
    const char* global_mouse_whitelist[] = { "windows", "cocoa", "x11", "DIVE", "VMAN" };
    for (int n = 0; n < IM_ARRAYSIZE(global_mouse_whitelist); n++)
        if (strncmp(sdl_backend, global_mouse_whitelist[n], strlen(global_mouse_whitelist[n])) == 0)
            mouse_can_use_global_state = true;
#endif

    // Setup backend capabilities flags
    ImGui_ImplSDL2_Data* bd = IM_NEW(ImGui_ImplSDL2_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_sdl";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;            // We can honor io.WantSetMousePos requests (optional, rarely used)
    if (mouse_can_use_global_state)
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;  // We can create multi-viewports on the Platform side (optional)

    // SDL on Linux/OSX doesn't report events for unfocused windows (see https://github.com/ocornut/imgui/issues/4960)
#ifndef __APPLE__
    if (mouse_can_use_global_state)
        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;// We can call io.AddMouseViewportEvent() with correct data (optional)
#endif

    bd->Window = window;
    bd->Renderer = renderer;
    bd->MouseCanUseGlobalState = mouse_can_use_global_state;

    io.SetClipboardTextFn = ImGui_ImplSDL2_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplSDL2_GetClipboardText;
    io.ClipboardUserData = NULL;

    // Load mouse cursors
    bd->MouseCursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    bd->MouseCursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    bd->MouseCursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    bd->MouseCursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);

    // Set platform dependent data in viewport
    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void*)window;
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info))
    {
#ifdef _WIN32
        main_viewport->PlatformHandleRaw = (void*)info.info.win.window;
#elif defined(__APPLE__)
        main_viewport->PlatformHandleRaw = (void*)info.info.cocoa.window;
#endif
    }

    // Set SDL hint to receive mouse click events on window focus, otherwise SDL doesn't emit the event.
    // Without this, when clicking to gain focus, our widgets wouldn't activate even though they showed as hovered.
    // (This is unfortunately a global SDL setting, so enabling it might have a side-effect on your application.
    // It is unlikely to make a difference, but if your app absolutely needs to ignore the initial on-focus click:
    // you can ignore SDL_MOUSEBUTTONDOWN events coming right after a SDL_WINDOWEVENT_FOCUS_GAINED)
#if SDL_HAS_MOUSE_FOCUS_CLICKTHROUGH
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif

    // Update monitors
    ImGui_ImplSDL2_UpdateMonitors();

    // We need SDL_CaptureMouse(), SDL_GetGlobalMouseState() from SDL 2.0.4+ to support multiple viewports.
    // We left the call to ImGui_ImplSDL2_InitPlatformInterface() outside of #ifdef to avoid unused-function warnings.
    if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) && (io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports))
        ImGui_ImplSDL2_InitPlatformInterface(window, sdl_gl_context);

    return true;
}

bool ImGui_ImplSDL2_InitForOpenGL(SDL_Window* window, void* sdl_gl_context)
{
    return ImGui_ImplSDL2_Init(window, NULL, sdl_gl_context);
}

bool ImGui_ImplSDL2_InitForVulkan(SDL_Window* window)
{
#if !SDL_HAS_VULKAN
    IM_ASSERT(0 && "Unsupported");
#endif
    if (!ImGui_ImplSDL2_Init(window, NULL, NULL))
        return false;
    ImGui_ImplSDL2_Data* bd = ImGui_ImplSDL2_GetBackendData();
    bd->UseVulkan = true;
    return true;
}

bool ImGui_ImplSDL2_InitForD3D(SDL_Window* window)
{
#if !defined(_WIN32)
    IM_ASSERT(0 && "Unsupported");
#endif
    return ImGui_ImplSDL2_Init(window, NULL, NULL);
}

bool ImGui_ImplSDL2_InitForMetal(SDL_Window* window)
{
    return ImGui_ImplSDL2_Init(window, NULL, NULL);
}

bool ImGui_ImplSDL2_InitForSDLRenderer(SDL_Window* window, SDL_Renderer* renderer)
{
    return ImGui_ImplSDL2_Init(window, renderer, NULL);
}

void ImGui_ImplSDL2_Shutdown()
{
    ImGui_ImplSDL2_Data* bd = ImGui_ImplSDL2_GetBackendData();
    IM_ASSERT(bd != NULL && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDL2_ShutdownPlatformInterface();

    if (bd->ClipboardTextData)
        SDL_free(bd->ClipboardTextData);
    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        SDL_FreeCursor(bd->MouseCursors[cursor_n]);

    io.BackendPlatformName = NULL;
    io.BackendPlatformUserData = NULL;
    IM_DELETE(bd);
}

// This code is incredibly messy because some of the functions we need for full viewport support are not available in SDL < 2.0.4.
static void ImGui_ImplSDL2_UpdateMouseData()
{
    ImGui_ImplSDL2_Data* bd = ImGui_ImplSDL2_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();

    // We forward mouse input when hovered or captured (via SDL_MOUSEMOTION) or when focused (below)
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    // SDL_CaptureMouse() let the OS know e.g. that our imgui drag outside the SDL window boundaries shouldn't e.g. trigger other operations outside
    SDL_CaptureMouse(bd->MouseButtonsDown != 0 ? SDL_TRUE : SDL_FALSE);
    SDL_Window* focused_window = SDL_GetKeyboardFocus();
    const bool is_app_focused = (focused_window && (bd->Window == focused_window || ImGui::FindViewportByPlatformHandle((void*)focused_window)));
#else
    SDL_Window* focused_window = bd->Window;
    const bool is_app_focused = (SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_INPUT_FOCUS) != 0; // SDL 2.0.3 and non-windowed systems: single-viewport only
#endif

    if (is_app_focused)
    {
        // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if (io.WantSetMousePos)
        {
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                SDL_WarpMouseGlobal((int)io.MousePos.x, (int)io.MousePos.y);
            else
#endif
                SDL_WarpMouseInWindow(bd->Window, (int)io.MousePos.x, (int)io.MousePos.y);
        }

        // (Optional) Fallback to provide mouse position when focused (SDL_MOUSEMOTION already provides this when hovered or captured)
        if (bd->MouseCanUseGlobalState && bd->MouseButtonsDown == 0)
        {
            // Single-viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
            // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
            int mouse_x, mouse_y, window_x, window_y;
            SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
            if (!(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
            {
                SDL_GetWindowPosition(focused_window, &window_x, &window_y);
                mouse_x -= window_x;
                mouse_y -= window_y;
            }
            io.AddMousePosEvent((float)mouse_x, (float)mouse_y);
        }
    }

    // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the viewport the OS mouse cursor is hovering.
    // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, Dear imGui will ignore this field and infer the information using its flawed heuristic.
    // - [!] SDL backend does NOT correctly ignore viewports with the _NoInputs flag.
    //       Some backend are not able to handle that correctly. If a backend report an hovered viewport that has the _NoInputs flag (e.g. when dragging a window
    //       for docking, the viewport has the _NoInputs flag in order to allow us to find the viewport under), then Dear ImGui is forced to ignore the value reported
    //       by the backend, and use its flawed heuristic to guess the viewport behind.
    // - [X] SDL backend correctly reports this regardless of another viewport behind focused and dragged from (we need this to find a useful drag and drop target).
    if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
    {
        ImGuiID mouse_viewport_id = 0;
        if (SDL_Window* sdl_mouse_window = SDL_GetWindowFromID(bd->MouseWindowID))
            if (ImGuiViewport* mouse_viewport = ImGui::FindViewportByPlatformHandle((void*)sdl_mouse_window))
                mouse_viewport_id = mouse_viewport->ID;
        io.AddMouseViewportEvent(mouse_viewport_id);
    }
}

static void ImGui_ImplSDL2_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;
    ImGui_ImplSDL2_Data* bd = ImGui_ImplSDL2_GetBackendData();

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        SDL_ShowCursor(SDL_FALSE);
    }
    else
    {
        // Show OS mouse cursor
        SDL_SetCursor(bd->MouseCursors[imgui_cursor] ? bd->MouseCursors[imgui_cursor] : bd->MouseCursors[ImGuiMouseCursor_Arrow]);
        SDL_ShowCursor(SDL_TRUE);
    }
}

static void ImGui_ImplSDL2_UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    // Get gamepad
    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    SDL_GameController* game_controller = SDL_GameControllerOpen(0);
    if (!game_controller)
        return;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    // Update gamepad inputs
    #define IM_SATURATE(V)                      (V < 0.0f ? 0.0f : V > 1.0f ? 1.0f : V)
    #define MAP_BUTTON(KEY_NO, BUTTON_NO)       { io.AddKeyEvent(KEY_NO, SDL_GameControllerGetButton(game_controller, BUTTON_NO) != 0); }
    #define MAP_ANALOG(KEY_NO, AXIS_NO, V0, V1) { float vn = (float)(SDL_GameControllerGetAxis(game_controller, AXIS_NO) - V0) / (float)(V1 - V0); vn = IM_SATURATE(vn); io.AddKeyAnalogEvent(KEY_NO, vn > 0.1f, vn); }
    const int thumb_dead_zone = 8000;           // SDL_gamecontroller.h suggests using this value.
    MAP_BUTTON(ImGuiKey_GamepadStart,           SDL_CONTROLLER_BUTTON_START);
    MAP_BUTTON(ImGuiKey_GamepadBack,            SDL_CONTROLLER_BUTTON_BACK);
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft,        SDL_CONTROLLER_BUTTON_X);              // Xbox X, PS Square
    MAP_BUTTON(ImGuiKey_GamepadFaceRight,       SDL_CONTROLLER_BUTTON_B);              // Xbox B, PS Circle
    MAP_BUTTON(ImGuiKey_GamepadFaceUp,          SDL_CONTROLLER_BUTTON_Y);              // Xbox Y, PS Triangle
    MAP_BUTTON(ImGuiKey_GamepadFaceDown,        SDL_CONTROLLER_BUTTON_A);              // Xbox A, PS Cross
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft,        SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight,       SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp,          SDL_CONTROLLER_BUTTON_DPAD_UP);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown,        SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    MAP_BUTTON(ImGuiKey_GamepadL1,              SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    MAP_BUTTON(ImGuiKey_GamepadR1,              SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    MAP_ANALOG(ImGuiKey_GamepadL2,              SDL_CONTROLLER_AXIS_TRIGGERLEFT,  0.0f, 32767);
    MAP_ANALOG(ImGuiKey_GamepadR2,              SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 0.0f, 32767);
    MAP_BUTTON(ImGuiKey_GamepadL3,              SDL_CONTROLLER_BUTTON_LEFTSTICK);
    MAP_BUTTON(ImGuiKey_GamepadR3,              SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,      SDL_CONTROLLER_AXIS_LEFTX,  -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight,     SDL_CONTROLLER_AXIS_LEFTX,  +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,        SDL_CONTROLLER_AXIS_LEFTY,  -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,      SDL_CONTROLLER_AXIS_LEFTY,  +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft,      SDL_CONTROLLER_AXIS_RIGHTX, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight,     SDL_CONTROLLER_AXIS_RIGHTX, +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp,        SDL_CONTROLLER_AXIS_RIGHTY, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown,      SDL_CONTROLLER_AXIS_RIGHTY, +thumb_dead_zone, +32767);
    #undef MAP_BUTTON
    #undef MAP_ANALOG
}

// FIXME-PLATFORM: SDL doesn't have an event to notify the application of display/monitor changes
static void ImGui_ImplSDL2_UpdateMonitors()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Monitors.resize(0);
    int display_count = SDL_GetNumVideoDisplays();
    for (int n = 0; n < display_count; n++)
    {
        // Warning: the validity of monitor DPI information on Windows depends on the application DPI awareness settings, which generally needs to be set in the manifest or at runtime.
        ImGuiPlatformMonitor monitor;
        SDL_Rect r;
        SDL_GetDisplayBounds(n, &r);
        monitor.MainPos = monitor.WorkPos = ImVec2((float)r.x, (float)r.y);
        monitor.MainSize = monitor.WorkSize = ImVec2((float)r.w, (float)r.h);
#if SDL_HAS_USABLE_DISPLAY_BOUNDS
        SDL_GetDisplayUsableBounds(n, &r);
        monitor.WorkPos = ImVec2((float)r.x, (float)r.y);
        monitor.WorkSize = ImVec2((float)r.w, (float)r.h);
#endif
#if SDL_HAS_PER_MONITOR_DPI
        // FIXME-VIEWPORT: On MacOS SDL reports actual monitor DPI scale, ignoring OS configuration. We may want to set
        //  DpiScale to cocoa_window.backingScaleFactor here.
        float dpi = 0.0f;
        if (!SDL_GetDisplayDPI(n, &dpi, NULL, NULL))
            monitor.DpiScale = dpi / 96.0f;
#endif
        platform_io.Monitors.push_back(monitor);
    }
}

void ImGui_ImplSDL2_NewFrame()
{
    ImGui_ImplSDL2_Data* bd = ImGui_ImplSDL2_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplSDL2_Init()?");
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    SDL_GetWindowSize(bd->Window, &w, &h);
    if (SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_MINIMIZED)
        w = h = 0;
    if (bd->Renderer != NULL)
        SDL_GetRendererOutputSize(bd->Renderer, &display_w, &display_h);
    else
        SDL_GL_GetDrawableSize(bd->Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);

    // Setup time step (we don't use SDL_GetTicks() because it is using millisecond resolution)
    static Uint64 frequency = SDL_GetPerformanceFrequency();
    Uint64 current_time = SDL_GetPerformanceCounter();
    io.DeltaTime = bd->Time > 0 ? (float)((double)(current_time - bd->Time) / frequency) : (float)(1.0f / 60.0f);
    bd->Time = current_time;

    if (bd->PendingMouseLeaveFrame && bd->PendingMouseLeaveFrame >= ImGui::GetFrameCount() && bd->MouseButtonsDown == 0)
    {
        bd->MouseWindowID = 0;
        bd->PendingMouseLeaveFrame = 0;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    ImGui_ImplSDL2_UpdateMouseData();
    ImGui_ImplSDL2_UpdateMouseCursor();

    // Update game controllers (if enabled and available)
    ImGui_ImplSDL2_UpdateGamepads();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

// Helper structure we store in the void* RenderUserData field of each ImGuiViewport to easily retrieve our backend data.
struct ImGui_ImplSDL2_ViewportData
{
    SDL_Window*     Window;
    Uint32          WindowID;
    bool            WindowOwned;
    SDL_GLContext   GLContext;

    ImGui_ImplSDL2_ViewportData() { Window = NULL; WindowID = 0; WindowOwned = false; GLContext = NULL; }
    ~ImGui_ImplSDL2_ViewportData() { IM_ASSERT(Window == NULL && GLContext == NULL); }
};

static void ImGui_ImplSDL2_CreateWindow(ImGuiViewport* viewport)
{
    ImGui_ImplSDL2_Data* bd = ImGui_ImplSDL2_GetBackendData();
    ImGui_ImplSDL2_ViewportData* vd = IM_NEW(ImGui_ImplSDL2_ViewportData)();
    viewport->PlatformUserData = vd;

    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui_ImplSDL2_ViewportData* main_viewport_data = (ImGui_ImplSDL2_ViewportData*)main_viewport->PlatformUserData;

    // Share GL resources with main context
    bool use_opengl = (main_viewport_data->GLContext != NULL);
    SDL_GLContext backup_context = NULL;
    if (use_opengl)
    {
        backup_context = SDL_GL_GetCurrentContext();
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
        SDL_GL_MakeCurrent(main_viewport_data->Window, main_viewport_data->GLContext);
    }

    Uint32 sdl_flags = 0;
    sdl_flags |= use_opengl ? SDL_WINDOW_OPENGL : (bd->UseVulkan ? SDL_WINDOW_VULKAN : 0);
    sdl_flags |= SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_ALLOW_HIGHDPI;
    sdl_flags |= SDL_WINDOW_HIDDEN;
    sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? SDL_WINDOW_BORDERLESS : 0;
    sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? 0 : SDL_WINDOW_RESIZABLE;
#if !defined(_WIN32)
    // See SDL hack in ImGui_ImplSDL2_ShowWindow().
    sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon) ? SDL_WINDOW_SKIP_TASKBAR : 0;
#endif
#if SDL_HAS_ALWAYS_ON_TOP
    sdl_flags |= (viewport->Flags & ImGuiViewportFlags_TopMost) ? SDL_WINDOW_ALWAYS_ON_TOP : 0;
#endif
    vd->Window = SDL_CreateWindow("No Title Yet", (int)viewport->Pos.x, (int)viewport->Pos.y, (int)viewport->Size.x, (int)viewport->Size.y, sdl_flags);
    vd->WindowOwned = true;
    if (use_opengl)
    {
        vd->GLContext = SDL_GL_CreateContext(vd->Window);
        SDL_GL_SetSwapInterval(0);
    }
    if (use_opengl && backup_context)
        SDL_GL_MakeCurrent(vd->Window, backup_context);

    viewport->PlatformHandle = (void*)vd->Window;
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(vd->Window, &info))
    {
#if defined(_WIN32)
        viewport->PlatformHandleRaw = info.info.win.window;
#elif defined(__APPLE__)
        viewport->PlatformHandleRaw = (void*)info.info.cocoa.window;
#endif
    }
}

static void ImGui_ImplSDL2_DestroyWindow(ImGuiViewport* viewport)
{
    if (ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData)
    {
        if (vd->GLContext && vd->WindowOwned)
            SDL_GL_DeleteContext(vd->GLContext);
        if (vd->Window && vd->WindowOwned)
            SDL_DestroyWindow(vd->Window);
        vd->GLContext = NULL;
        vd->Window = NULL;
        IM_DELETE(vd);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = NULL;
}

static void ImGui_ImplSDL2_ShowWindow(ImGuiViewport* viewport)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
#if defined(_WIN32)
    HWND hwnd = (HWND)viewport->PlatformHandleRaw;

    // SDL hack: Hide icon from task bar
    // Note: SDL 2.0.6+ has a SDL_WINDOW_SKIP_TASKBAR flag which is supported under Windows but the way it create the window breaks our seamless transition.
    if (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)
    {
        LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
        ex_style &= ~WS_EX_APPWINDOW;
        ex_style |= WS_EX_TOOLWINDOW;
        ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    }

    // SDL hack: SDL always activate/focus windows :/
    if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
    {
        ::ShowWindow(hwnd, SW_SHOWNA);
        return;
    }
#endif

    SDL_ShowWindow(vd->Window);
}

static ImVec2 ImGui_ImplSDL2_GetWindowPos(ImGuiViewport* viewport)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    int x = 0, y = 0;
    SDL_GetWindowPosition(vd->Window, &x, &y);
    return ImVec2((float)x, (float)y);
}

static void ImGui_ImplSDL2_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    SDL_SetWindowPosition(vd->Window, (int)pos.x, (int)pos.y);
}

static ImVec2 ImGui_ImplSDL2_GetWindowSize(ImGuiViewport* viewport)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    int w = 0, h = 0;
    SDL_GetWindowSize(vd->Window, &w, &h);
    return ImVec2((float)w, (float)h);
}

static void ImGui_ImplSDL2_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    SDL_SetWindowSize(vd->Window, (int)size.x, (int)size.y);
}

static void ImGui_ImplSDL2_SetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    SDL_SetWindowTitle(vd->Window, title);
}

#if SDL_HAS_WINDOW_ALPHA
static void ImGui_ImplSDL2_SetWindowAlpha(ImGuiViewport* viewport, float alpha)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    SDL_SetWindowOpacity(vd->Window, alpha);
}
#endif

static void ImGui_ImplSDL2_SetWindowFocus(ImGuiViewport* viewport)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    SDL_RaiseWindow(vd->Window);
}

static bool ImGui_ImplSDL2_GetWindowFocus(ImGuiViewport* viewport)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    return (SDL_GetWindowFlags(vd->Window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

static bool ImGui_ImplSDL2_GetWindowMinimized(ImGuiViewport* viewport)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    return (SDL_GetWindowFlags(vd->Window) & SDL_WINDOW_MINIMIZED) != 0;
}

static void ImGui_ImplSDL2_RenderWindow(ImGuiViewport* viewport, void*)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    if (vd->GLContext)
        SDL_GL_MakeCurrent(vd->Window, vd->GLContext);
}

static void ImGui_ImplSDL2_SwapBuffers(ImGuiViewport* viewport, void*)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    if (vd->GLContext)
    {
        SDL_GL_MakeCurrent(vd->Window, vd->GLContext);
        SDL_GL_SwapWindow(vd->Window);
    }
}

// Vulkan support (the Vulkan renderer needs to call a platform-side support function to create the surface)
// SDL is graceful enough to _not_ need <vulkan/vulkan.h> so we can safely include this.
#if SDL_HAS_VULKAN
#include <SDL_vulkan.h>
static int ImGui_ImplSDL2_CreateVkSurface(ImGuiViewport* viewport, ImU64 vk_instance, const void* vk_allocator, ImU64* out_vk_surface)
{
    ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData;
    (void)vk_allocator;
    SDL_bool ret = SDL_Vulkan_CreateSurface(vd->Window, (VkInstance)vk_instance, (VkSurfaceKHR*)out_vk_surface);
    return ret ? 0 : 1; // ret ? VK_SUCCESS : VK_NOT_READY
}
#endif // SDL_HAS_VULKAN

static void ImGui_ImplSDL2_InitPlatformInterface(SDL_Window* window, void* sdl_gl_context)
{
    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateWindow = ImGui_ImplSDL2_CreateWindow;
    platform_io.Platform_DestroyWindow = ImGui_ImplSDL2_DestroyWindow;
    platform_io.Platform_ShowWindow = ImGui_ImplSDL2_ShowWindow;
    platform_io.Platform_SetWindowPos = ImGui_ImplSDL2_SetWindowPos;
    platform_io.Platform_GetWindowPos = ImGui_ImplSDL2_GetWindowPos;
    platform_io.Platform_SetWindowSize = ImGui_ImplSDL2_SetWindowSize;
    platform_io.Platform_GetWindowSize = ImGui_ImplSDL2_GetWindowSize;
    platform_io.Platform_SetWindowFocus = ImGui_ImplSDL2_SetWindowFocus;
    platform_io.Platform_GetWindowFocus = ImGui_ImplSDL2_GetWindowFocus;
    platform_io.Platform_GetWindowMinimized = ImGui_ImplSDL2_GetWindowMinimized;
    platform_io.Platform_SetWindowTitle = ImGui_ImplSDL2_SetWindowTitle;
    platform_io.Platform_RenderWindow = ImGui_ImplSDL2_RenderWindow;
    platform_io.Platform_SwapBuffers = ImGui_ImplSDL2_SwapBuffers;
#if SDL_HAS_WINDOW_ALPHA
    platform_io.Platform_SetWindowAlpha = ImGui_ImplSDL2_SetWindowAlpha;
#endif
#if SDL_HAS_VULKAN
    platform_io.Platform_CreateVkSurface = ImGui_ImplSDL2_CreateVkSurface;
#endif

    // Register main window handle (which is owned by the main application, not by us)
    // This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.) can use same logic for main and secondary viewports.
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui_ImplSDL2_ViewportData* vd = IM_NEW(ImGui_ImplSDL2_ViewportData)();
    vd->Window = window;
    vd->WindowID = SDL_GetWindowID(window);
    vd->WindowOwned = false;
    vd->GLContext = sdl_gl_context;
    main_viewport->PlatformUserData = vd;
    main_viewport->PlatformHandle = vd->Window;
}

static void ImGui_ImplSDL2_ShutdownPlatformInterface()
{
    ImGui::DestroyPlatformWindows();
}
