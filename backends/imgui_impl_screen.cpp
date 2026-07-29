// dear imgui: Platform Backend for QNX Screen
// This needs to be used along with a Renderer Backend (e.g. OpenGL3, Vulkan).

// Implemented features:
//  [X] Platform: Keyboard support. Uses io.AddKeyEvent() and native QNX key symbols.
//  [X] Platform: Mouse support. Handles position, three buttons and horizontal/vertical wheels.
//  [X] Platform: Touch support. Maps the first active touch contact to the primary mouse button.
//  [X] Platform: Focus and window-size updates.
// Missing features or Issues:
//  [ ] Platform: Clipboard support.
//  [ ] Platform: Gamepad support.
//  [ ] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors).
//  [ ] Platform: Multi-viewport support.

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2026-07-29: Reworked keyboard handling from the proven qterm Screen path: authoritative modifier masks,
//              complete keypad/Hyper mapping, and QNX/X11 keysym-to-Unicode conversion.
//  2026-07-29: Fixed keyboard handling: use KEY_CAP for physical keys, SYM for text, and honor Screen validity flags.
//  2026-07-28: Initial QNX Screen backend.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_screen.h"

#include <float.h>
#include <stdint.h>
#include <string.h>
#include <sys/keycodes.h>
#include <time.h>

#include "imgui_impl_screen_keysym2ucs.inl"

struct ImGui_ImplScreen_Data
{
    screen_context_t Context;
    screen_window_t  Window;
    uint64_t         TimeNanoseconds;
    int              MouseButtons;
    int              ActiveTouchId;
    bool             LeftSuper;
    bool             RightSuper;

    ImGui_ImplScreen_Data()
    {
        memset(this, 0, sizeof(*this));
        ActiveTouchId = -1;
    }
};

static ImGui_ImplScreen_Data* ImGui_ImplScreen_GetBackendData()
{
    return ImGui::GetCurrentContext() != nullptr ? (ImGui_ImplScreen_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

static uint64_t ImGui_ImplScreen_GetTimeNanoseconds()
{
    struct timespec timestamp = {};
    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0)
    {
        return 0;
    }
    return (uint64_t)timestamp.tv_sec * UINT64_C(1000000000) + (uint64_t)timestamp.tv_nsec;
}

static ImGuiKey ImGui_ImplScreen_KeyCodeToImGuiKey(int key_code)
{
    switch (key_code)
    {
        case KEYCODE_TAB:              return ImGuiKey_Tab;
        case KEYCODE_LEFT:             return ImGuiKey_LeftArrow;
        case KEYCODE_RIGHT:            return ImGuiKey_RightArrow;
        case KEYCODE_UP:               return ImGuiKey_UpArrow;
        case KEYCODE_DOWN:             return ImGuiKey_DownArrow;
        case KEYCODE_PG_UP:            return ImGuiKey_PageUp;
        case KEYCODE_PG_DOWN:          return ImGuiKey_PageDown;
        case KEYCODE_HOME:             return ImGuiKey_Home;
        case KEYCODE_END:              return ImGuiKey_End;
        case KEYCODE_INSERT:           return ImGuiKey_Insert;
        case KEYCODE_DELETE:           return ImGuiKey_Delete;
        case KEYCODE_BACKSPACE:        return ImGuiKey_Backspace;
        case KEYCODE_SPACE:            return ImGuiKey_Space;
        case KEYCODE_RETURN:           return ImGuiKey_Enter;
        case KEYCODE_ESCAPE:           return ImGuiKey_Escape;
        case KEYCODE_APOSTROPHE:       return ImGuiKey_Apostrophe;
        case KEYCODE_COMMA:            return ImGuiKey_Comma;
        case KEYCODE_MINUS:            return ImGuiKey_Minus;
        case KEYCODE_PERIOD:           return ImGuiKey_Period;
        case KEYCODE_SLASH:            return ImGuiKey_Slash;
        case KEYCODE_SEMICOLON:        return ImGuiKey_Semicolon;
        case KEYCODE_EQUAL:            return ImGuiKey_Equal;
        case KEYCODE_LEFT_BRACKET:     return ImGuiKey_LeftBracket;
        case KEYCODE_BACK_SLASH:       return ImGuiKey_Backslash;
        case KEYCODE_RIGHT_BRACKET:    return ImGuiKey_RightBracket;
        case KEYCODE_GRAVE:            return ImGuiKey_GraveAccent;
        case KEYCODE_CAPS_LOCK:        return ImGuiKey_CapsLock;
        case KEYCODE_SCROLL_LOCK:      return ImGuiKey_ScrollLock;
        case KEYCODE_NUM_LOCK:         return ImGuiKey_NumLock;
        case KEYCODE_PRINT:            return ImGuiKey_PrintScreen;
        case KEYCODE_PAUSE:            return ImGuiKey_Pause;
        case KEYCODE_KP_DIVIDE:        return ImGuiKey_KeypadDivide;
        case KEYCODE_KP_MULTIPLY:      return ImGuiKey_KeypadMultiply;
        case KEYCODE_KP_MINUS:         return ImGuiKey_KeypadSubtract;
        case KEYCODE_KP_PLUS:          return ImGuiKey_KeypadAdd;
        case KEYCODE_KP_ENTER:         return ImGuiKey_KeypadEnter;
        // QNX reports keypad digits by their navigation key-cap names.
        case KEYCODE_KP_INSERT:        return ImGuiKey_Keypad0;
        case KEYCODE_KP_END:           return ImGuiKey_Keypad1;
        case KEYCODE_KP_DOWN:          return ImGuiKey_Keypad2;
        case KEYCODE_KP_PG_DOWN:       return ImGuiKey_Keypad3;
        case KEYCODE_KP_LEFT:          return ImGuiKey_Keypad4;
        case KEYCODE_KP_FIVE:          return ImGuiKey_Keypad5;
        case KEYCODE_KP_RIGHT:         return ImGuiKey_Keypad6;
        case KEYCODE_KP_HOME:          return ImGuiKey_Keypad7;
        case KEYCODE_KP_UP:            return ImGuiKey_Keypad8;
        case KEYCODE_KP_PG_UP:         return ImGuiKey_Keypad9;
        case KEYCODE_KP_DELETE:        return ImGuiKey_KeypadDecimal;
        case KEYCODE_LEFT_CTRL:        return ImGuiKey_LeftCtrl;
        case KEYCODE_LEFT_SHIFT:       return ImGuiKey_LeftShift;
        case KEYCODE_LEFT_ALT:         return ImGuiKey_LeftAlt;
        case KEYCODE_RIGHT_CTRL:       return ImGuiKey_RightCtrl;
        case KEYCODE_RIGHT_SHIFT:      return ImGuiKey_RightShift;
        case KEYCODE_RIGHT_ALT:        return ImGuiKey_RightAlt;
        case KEYCODE_LEFT_HYPER:       return ImGuiKey_LeftSuper;
        case KEYCODE_RIGHT_HYPER:      return ImGuiKey_RightSuper;
        case KEYCODE_MENU:             return ImGuiKey_Menu;
        case KEYCODE_ZERO:             return ImGuiKey_0;
        case KEYCODE_ONE:              return ImGuiKey_1;
        case KEYCODE_TWO:              return ImGuiKey_2;
        case KEYCODE_THREE:            return ImGuiKey_3;
        case KEYCODE_FOUR:             return ImGuiKey_4;
        case KEYCODE_FIVE:             return ImGuiKey_5;
        case KEYCODE_SIX:              return ImGuiKey_6;
        case KEYCODE_SEVEN:            return ImGuiKey_7;
        case KEYCODE_EIGHT:            return ImGuiKey_8;
        case KEYCODE_NINE:             return ImGuiKey_9;
        case KEYCODE_A:                return ImGuiKey_A;
        case KEYCODE_B:                return ImGuiKey_B;
        case KEYCODE_C:                return ImGuiKey_C;
        case KEYCODE_D:                return ImGuiKey_D;
        case KEYCODE_E:                return ImGuiKey_E;
        case KEYCODE_F:                return ImGuiKey_F;
        case KEYCODE_G:                return ImGuiKey_G;
        case KEYCODE_H:                return ImGuiKey_H;
        case KEYCODE_I:                return ImGuiKey_I;
        case KEYCODE_J:                return ImGuiKey_J;
        case KEYCODE_K:                return ImGuiKey_K;
        case KEYCODE_L:                return ImGuiKey_L;
        case KEYCODE_M:                return ImGuiKey_M;
        case KEYCODE_N:                return ImGuiKey_N;
        case KEYCODE_O:                return ImGuiKey_O;
        case KEYCODE_P:                return ImGuiKey_P;
        case KEYCODE_Q:                return ImGuiKey_Q;
        case KEYCODE_R:                return ImGuiKey_R;
        case KEYCODE_S:                return ImGuiKey_S;
        case KEYCODE_T:                return ImGuiKey_T;
        case KEYCODE_U:                return ImGuiKey_U;
        case KEYCODE_V:                return ImGuiKey_V;
        case KEYCODE_W:                return ImGuiKey_W;
        case KEYCODE_X:                return ImGuiKey_X;
        case KEYCODE_Y:                return ImGuiKey_Y;
        case KEYCODE_Z:                return ImGuiKey_Z;
        case KEYCODE_F1:               return ImGuiKey_F1;
        case KEYCODE_F2:               return ImGuiKey_F2;
        case KEYCODE_F3:               return ImGuiKey_F3;
        case KEYCODE_F4:               return ImGuiKey_F4;
        case KEYCODE_F5:               return ImGuiKey_F5;
        case KEYCODE_F6:               return ImGuiKey_F6;
        case KEYCODE_F7:               return ImGuiKey_F7;
        case KEYCODE_F8:               return ImGuiKey_F8;
        case KEYCODE_F9:               return ImGuiKey_F9;
        case KEYCODE_F10:              return ImGuiKey_F10;
        case KEYCODE_F11:              return ImGuiKey_F11;
        case KEYCODE_F12:              return ImGuiKey_F12;
        default:                       return ImGuiKey_None;
    }
}

static void ImGui_ImplScreen_UpdateKeyModifiers(ImGuiIO& io, ImGui_ImplScreen_Data* bd, int modifiers, ImGuiKey key, bool down)
{
    // SCREEN_PROPERTY_MODIFIERS is the authoritative state for Shift/Ctrl/Alt.
    // Tracking those keys locally loses sticky modifiers and can become stale if
    // a key transition is consumed elsewhere. qterm uses this same Screen state.
    if (key == ImGuiKey_LeftSuper)
    {
        bd->LeftSuper = down;
    }
    else if (key == ImGuiKey_RightSuper)
    {
        bd->RightSuper = down;
    }

    io.AddKeyEvent(ImGuiMod_Ctrl, (modifiers & KEYMOD_CTRL) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (modifiers & KEYMOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (modifiers & KEYMOD_ALT) != 0);
    io.AddKeyEvent(ImGuiMod_Super, bd->LeftSuper || bd->RightSuper);
}

static void ImGui_ImplScreen_ClearModifierState(ImGuiIO& io, ImGui_ImplScreen_Data* bd)
{
    bd->LeftSuper = false;
    bd->RightSuper = false;
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    io.AddKeyEvent(ImGuiMod_Shift, false);
    io.AddKeyEvent(ImGuiMod_Alt, false);
    io.AddKeyEvent(ImGuiMod_Super, false);
}

static bool ImGui_ImplScreen_IsTextCodePoint(unsigned int code_point)
{
    // At this point the QNX keysym has already been converted to Unicode.
    // Do not reject Unicode private-use characters: a user keymap may emit one.
    const bool surrogate = code_point >= 0xd800u && code_point <= 0xdfffu;
    const bool control = code_point < 0x20u || (code_point >= 0x7fu && code_point <= 0x9fu);
    return code_point <= IM_UNICODE_CODEPOINT_MAX && !surrogate && !control;
}

static bool ImGui_ImplScreen_ProcessKeyboardEvent(ImGui_ImplScreen_Data* bd, screen_event_t event)
{
    int flags = 0;
    int modifiers = 0;
    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_FLAGS, &flags) != 0 ||
        screen_get_event_property_iv(event, SCREEN_PROPERTY_MODIFIERS, &modifiers) != 0)
    {
        return false;
    }

    // KEY_CAP is the unmodified key identity used for ImGuiKey events. SYM is
    // the layout/modifier-translated QNX keysym used for text input.
    int key_cap = 0;
    int key_sym = 0;
    int scan_code = 0;
    const bool cap_valid = (flags & SCREEN_FLAG_CAP_VALID) != 0;
    const bool sym_valid = (flags & SCREEN_FLAG_SYM_VALID) != 0;
    const bool scan_valid = (flags & SCREEN_FLAG_SCAN_VALID) != 0;
    const bool down = (flags & (SCREEN_FLAG_KEY_DOWN | SCREEN_FLAG_KEY_REPEAT)) != 0;

    if (cap_valid && screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_CAP, &key_cap) != 0)
    {
        return false;
    }
    if (sym_valid && screen_get_event_property_iv(event, SCREEN_PROPERTY_SYM, &key_sym) != 0)
    {
        return false;
    }
    if (scan_valid && screen_get_event_property_iv(event, SCREEN_PROPERTY_SCAN, &scan_code) != 0)
    {
        scan_code = 0;
    }

    const ImGuiKey key = cap_valid ? ImGui_ImplScreen_KeyCodeToImGuiKey(key_cap) : ImGuiKey_None;
    ImGuiIO& io = ImGui::GetIO();

    // Modifier events must precede the ordinary key event in Dear ImGui's queue.
    // Unlike qterm, we still submit the individual modifier key because ImGui
    // exposes Left/Right Ctrl, Shift, Alt and Super as named keys.
    ImGui_ImplScreen_UpdateKeyModifiers(io, bd, modifiers, key, down);

    if (key != ImGuiKey_None)
    {
        io.AddKeyEvent(key, down);
        io.SetKeyEventNativeData(key, key_cap, scan_code);
    }

    bool text_added = false;
    if (down && sym_valid && key_sym >= 0)
    {
        const unsigned int keysym = (unsigned int)key_sym;
        // QNX special/function symbols begin at KEYCODE_PC_KEYS. Printable
        // symbols below that boundary use X11 keysym encoding. Also accept the
        // standard directly encoded UCS keysym form (0x01000000 | codepoint).
        if (keysym < (unsigned int)KEYCODE_PC_KEYS || (keysym & 0xff000000u) == 0x01000000u)
        {
            const unsigned int code_point = ImGui_ImplScreen_KeySymToUnicode(keysym);
            if (ImGui_ImplScreen_IsTextCodePoint(code_point))
            {
                io.AddInputCharacter(code_point);
                text_added = true;
            }
        }
    }

    return key != ImGuiKey_None || text_added || cap_valid || sym_valid;
}

static bool ImGui_ImplScreen_ProcessPointerEvent(ImGui_ImplScreen_Data* bd, screen_event_t event)
{
    int position[2] = {};
    int buttons = 0;
    int wheel_vertical = 0;
    int wheel_horizontal = 0;

    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, position) != 0 &&
        screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, position) != 0)
    {
        return false;
    }
    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_BUTTONS, &buttons) != 0)
    {
        buttons = bd->MouseButtons;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    io.AddMousePosEvent((float)position[0], (float)position[1]);

    const int changed_buttons = buttons ^ bd->MouseButtons;
    if ((changed_buttons & SCREEN_LEFT_MOUSE_BUTTON) != 0)
    {
        io.AddMouseButtonEvent(0, (buttons & SCREEN_LEFT_MOUSE_BUTTON) != 0);
    }
    if ((changed_buttons & SCREEN_RIGHT_MOUSE_BUTTON) != 0)
    {
        io.AddMouseButtonEvent(1, (buttons & SCREEN_RIGHT_MOUSE_BUTTON) != 0);
    }
    if ((changed_buttons & SCREEN_MIDDLE_MOUSE_BUTTON) != 0)
    {
        io.AddMouseButtonEvent(2, (buttons & SCREEN_MIDDLE_MOUSE_BUTTON) != 0);
    }
    bd->MouseButtons = buttons;

    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_MOUSE_WHEEL, &wheel_vertical) != 0)
    {
        wheel_vertical = 0;
    }
    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_MOUSE_HORIZONTAL_WHEEL, &wheel_horizontal) != 0)
    {
        wheel_horizontal = 0;
    }
    if (wheel_vertical != 0 || wheel_horizontal != 0)
    {
        // Screen: up/left are negative. Dear ImGui: up/left are positive.
        io.AddMouseWheelEvent((float)-wheel_horizontal, (float)-wheel_vertical);
    }
    return true;
}

static bool ImGui_ImplScreen_ProcessTouchEvent(ImGui_ImplScreen_Data* bd, screen_event_t event, int event_type)
{
    int touch_id = -1;
    int position[2] = {};
    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_ID, &touch_id) != 0)
    {
        return false;
    }

    const bool is_release = event_type == SCREEN_EVENT_MTOUCH_RELEASE;
    if (bd->ActiveTouchId != -1 && touch_id != bd->ActiveTouchId)
    {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);

    if (!is_release)
    {
        if (screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, position) != 0 &&
            screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, position) != 0)
        {
            return false;
        }
        io.AddMousePosEvent((float)position[0], (float)position[1]);
    }

    if (event_type == SCREEN_EVENT_MTOUCH_TOUCH)
    {
        bd->ActiveTouchId = touch_id;
        io.AddMouseButtonEvent(0, true);
    }
    else if (is_release)
    {
        io.AddMouseButtonEvent(0, false);
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        bd->ActiveTouchId = -1;
    }
    return true;
}

bool ImGui_ImplScreen_ProcessEvent(screen_event_t event)
{
    ImGui_ImplScreen_Data* bd = ImGui_ImplScreen_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplScreen_Init()?");
    if (bd == nullptr || event == nullptr)
    {
        return false;
    }

    int event_type = SCREEN_EVENT_NONE;
    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &event_type) != 0 || event_type == SCREEN_EVENT_NONE)
    {
        return false;
    }
    switch (event_type)
    {
        case SCREEN_EVENT_KEYBOARD:
            return ImGui_ImplScreen_ProcessKeyboardEvent(bd, event);
        case SCREEN_EVENT_POINTER:
            return ImGui_ImplScreen_ProcessPointerEvent(bd, event);
        case SCREEN_EVENT_MTOUCH_TOUCH:
        case SCREEN_EVENT_MTOUCH_MOVE:
        case SCREEN_EVENT_MTOUCH_RELEASE:
            return ImGui_ImplScreen_ProcessTouchEvent(bd, event, event_type);
        case SCREEN_EVENT_PROPERTY:
        {
            int property = 0;
            if (screen_get_event_property_iv(event, SCREEN_PROPERTY_NAME, &property) != 0)
            {
                return false;
            }
            if (property == SCREEN_PROPERTY_FOCUS)
            {
                int focused = 0;
                if (screen_get_window_property_iv(bd->Window, SCREEN_PROPERTY_FOCUS, &focused) == 0)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    io.AddFocusEvent(focused != 0);
                    if (focused == 0)
                    {
                        ImGui_ImplScreen_ClearModifierState(io, bd);
                    }
                }
                return true;
            }
            if (property == SCREEN_PROPERTY_SIZE || property == SCREEN_PROPERTY_BUFFER_SIZE || property == SCREEN_PROPERTY_VISIBLE)
            {
                return true;
            }
            return false;
        }
        case SCREEN_EVENT_CLOSE:
        {
            ImGuiIO& io = ImGui::GetIO();
            io.AddFocusEvent(false);
            ImGui_ImplScreen_ClearModifierState(io, bd);
            return true;
        }
        default:
            return false;
    }
}

bool ImGui_ImplScreen_Init(screen_context_t context, screen_window_t window)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");
    IM_ASSERT(context != nullptr && window != nullptr);
    if (context == nullptr || window == nullptr)
    {
        return false;
    }

    ImGui_ImplScreen_Data* bd = IM_NEW(ImGui_ImplScreen_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_screen";

    bd->Context = context;
    bd->Window = window;
    bd->TimeNanoseconds = ImGui_ImplScreen_GetTimeNanoseconds();

    int focused = 0;
    if (screen_get_window_property_iv(window, SCREEN_PROPERTY_FOCUS, &focused) == 0)
    {
        io.AddFocusEvent(focused != 0);
    }
    return true;
}

void ImGui_ImplScreen_Shutdown()
{
    ImGui_ImplScreen_Data* bd = ImGui_ImplScreen_GetBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");
    if (bd == nullptr)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    IM_DELETE(bd);
}

void ImGui_ImplScreen_NewFrame()
{
    ImGui_ImplScreen_Data* bd = ImGui_ImplScreen_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplScreen_Init()?");
    if (bd == nullptr)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    int size[2] = {};
    if (screen_get_window_property_iv(bd->Window, SCREEN_PROPERTY_BUFFER_SIZE, size) != 0 ||
        size[0] <= 0 || size[1] <= 0)
    {
        size[0] = 0;
        size[1] = 0;
        (void)screen_get_window_property_iv(bd->Window, SCREEN_PROPERTY_SIZE, size);
    }
    if (size[0] > 0 && size[1] > 0)
    {
        io.DisplaySize = ImVec2((float)size[0], (float)size[1]);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    }

    const uint64_t current_time = ImGui_ImplScreen_GetTimeNanoseconds();
    if (current_time > bd->TimeNanoseconds && bd->TimeNanoseconds != 0)
    {
        io.DeltaTime = (float)((double)(current_time - bd->TimeNanoseconds) / 1000000000.0);
    }
    else
    {
        io.DeltaTime = 1.0f / 60.0f;
    }
    bd->TimeNanoseconds = current_time;
}

#endif // #ifndef IMGUI_DISABLE
