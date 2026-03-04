// dear imgui: Platform Backend for Xbox 360 (XDK)
// Modeled after imgui_impl_win32.cpp — adapted for the console.
//
// KEY DIFFERENCES from Win32:
//   - No HWND / no message loop / no WndProcHandler
//   - No mouse (gamepad acts as the only navigation device)
//   - XInput is statically linked in the XDK (no dynamic DLL loading)
//   - DisplaySize is fixed — the caller sets it once in Init
//   - No DPI, no clipboard, no OS cursor shapes

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_xbox_360.h"

#ifndef _XBOX
    #error "This file requires the Xbox 360 XDK"
#endif
#include <xtl.h>   // Umbrella header: XInput, QueryPerformanceCounter, etc.

// ------------------------------------------------------------------------------
// Internal backend state
// ------------------------------------------------------------------------------

struct ImGui_ImplXDK_Data
{
    // Timing
    LARGE_INTEGER   TicksPerSecond;
    LARGE_INTEGER   LastTime;

    // Gamepad
    int             PrimaryPort;            // which controller drives ImGui nav
    DWORD           LastPacketNumber;       // skip update when packet unchanged
    bool            HasGamepad;             // was a gamepad detected last frame

    // System keyboard (XShowKeyboardUI) — optional text input
    bool            KeyboardOpen;           // is the system keyboard currently open
    XOVERLAPPED     KeyboardOverlapped;     // async operation handle
    wchar_t         KeyboardResult[512];    // output buffer for keyboard UI

    ImGui_ImplXDK_Data()    { memset((void*)this, 0, sizeof(*this)); PrimaryPort = 0; }
};

static ImGui_ImplXDK_Data* ImGui_ImplXDK_GetBackendData()
{
    return ImGui::GetCurrentContext()
        ? (ImGui_ImplXDK_Data*)ImGui::GetIO().BackendPlatformUserData
        : nullptr;
}

// ------------------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------------------

// Clamp a float to [0, 1].
static inline float ImSaturate(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// Map a digital button bit to an ImGuiKey analog event.
#define MAP_BUTTON(KEY, MASK)  \
    io.AddKeyEvent((KEY), ((gamepad.wButtons & (MASK)) != 0));

// Map an analog value in range [V0, V1] to an ImGuiKey analog event.
// Values outside the range are clamped/saturated.
#define MAP_ANALOG(KEY, VALUE, V0, V1) \
    { float vn = (float)((int)(VALUE) - (int)(V0)) / (float)((int)(V1) - (int)(V0)); \
      io.AddKeyAnalogEvent((KEY), vn > 0.10f, ImSaturate(vn)); }

// ------------------------------------------------------------------------------
// Gamepad update
// ------------------------------------------------------------------------------

static void ImGui_ImplXDK_UpdateGamepad(ImGuiIO& io)
{
    ImGui_ImplXDK_Data* bd = ImGui_ImplXDK_GetBackendData();

    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(state));

    // Only poll the primary port; extend to XUSER_MAX_COUNT if you need
    // multi-controller support (e.g. for split-screen UIs).
    if (XInputGetState(bd->PrimaryPort, &state) != ERROR_SUCCESS)
    {
        // Controller disconnected — clear gamepad flag so ImGui stops nav
        if (bd->HasGamepad)
        {
            io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
            bd->HasGamepad = false;
        }
        return;
    }

    // Packet number unchanged → state did not change, skip re-submitting events.
    // This matches the Win32 backend optimization (XInputPacketNumber check).
    if (bd->HasGamepad && bd->LastPacketNumber == state.dwPacketNumber)
        return;

    bd->HasGamepad       = true;
    bd->LastPacketNumber = state.dwPacketNumber;
    io.BackendFlags     |= ImGuiBackendFlags_HasGamepad;

    XINPUT_GAMEPAD& gamepad = state.Gamepad;

    // -- Face buttons ----------------------------------------------------------
    MAP_BUTTON(ImGuiKey_GamepadFaceDown,  XINPUT_GAMEPAD_A);           // Confirm
    MAP_BUTTON(ImGuiKey_GamepadFaceRight, XINPUT_GAMEPAD_B);           // Cancel / back
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft,  XINPUT_GAMEPAD_X);
    MAP_BUTTON(ImGuiKey_GamepadFaceUp,    XINPUT_GAMEPAD_Y);

    // -- D-Pad (discrete navigation) -------------------------------------------
    MAP_BUTTON(ImGuiKey_GamepadDpadUp,    XINPUT_GAMEPAD_DPAD_UP);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown,  XINPUT_GAMEPAD_DPAD_DOWN);
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft,  XINPUT_GAMEPAD_DPAD_LEFT);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight, XINPUT_GAMEPAD_DPAD_RIGHT);

    // -- Bumpers ---------------------------------------------------------------
    MAP_BUTTON(ImGuiKey_GamepadL1, XINPUT_GAMEPAD_LEFT_SHOULDER);
    MAP_BUTTON(ImGuiKey_GamepadR1, XINPUT_GAMEPAD_RIGHT_SHOULDER);

    // -- Thumbstick clicks -----------------------------------------------------
    MAP_BUTTON(ImGuiKey_GamepadL3, XINPUT_GAMEPAD_LEFT_THUMB);
    MAP_BUTTON(ImGuiKey_GamepadR3, XINPUT_GAMEPAD_RIGHT_THUMB);

    // -- Start / Back ----------------------------------------------------------
    MAP_BUTTON(ImGuiKey_GamepadStart, XINPUT_GAMEPAD_START);
    MAP_BUTTON(ImGuiKey_GamepadBack,  XINPUT_GAMEPAD_BACK);

    // -- Triggers (0–255, threshold defined by XDK constant) -------------------
    MAP_ANALOG(ImGuiKey_GamepadL2, gamepad.bLeftTrigger,  XINPUT_GAMEPAD_TRIGGER_THRESHOLD, 255);
    MAP_ANALOG(ImGuiKey_GamepadR2, gamepad.bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD, 255);

    // -- Left stick (navigation between items, same role as D-Pad but analog) --
    // sThumbLX/LY range: -32768 to +32767
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,  gamepad.sThumbLX, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,  -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight, gamepad.sThumbLX, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,  +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,    gamepad.sThumbLY, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,  +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,  gamepad.sThumbLY, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,  -32768);

    // -- Right stick (scroll inside panels) ------------------------------------
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft,  gamepad.sThumbRX, -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight, gamepad.sThumbRX, +XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp,    gamepad.sThumbRY, +XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown,  gamepad.sThumbRY, -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, -32768);
}

#undef MAP_BUTTON
#undef MAP_ANALOG

// ------------------------------------------------------------------------------
// Keyboard update (USB keyboard / Chatpad via XInputGetKeystroke)
// ------------------------------------------------------------------------------
// XInputGetKeystroke returns one keystroke event per call.
// We drain the queue completely every frame.
//
// VK_* codes on the 360 are the same Win32 values, so the mapping table
// is straightforward. We only need the subset that makes sense for a UI:
// printable characters + editing keys (backspace, enter, arrows, etc.).

static ImGuiKey ImGui_ImplXDK_VKeyToImGuiKey(WORD vk)
{
    switch (vk)
    {
    case VK_BACK:       return ImGuiKey_Backspace;
    case VK_TAB:        return ImGuiKey_Tab;
    case VK_RETURN:     return ImGuiKey_Enter;
    case VK_ESCAPE:     return ImGuiKey_Escape;
    case VK_SPACE:      return ImGuiKey_Space;
    case VK_LEFT:       return ImGuiKey_LeftArrow;
    case VK_RIGHT:      return ImGuiKey_RightArrow;
    case VK_UP:         return ImGuiKey_UpArrow;
    case VK_DOWN:       return ImGuiKey_DownArrow;
    case VK_HOME:       return ImGuiKey_Home;
    case VK_END:        return ImGuiKey_End;
    case VK_DELETE:     return ImGuiKey_Delete;
    case VK_INSERT:     return ImGuiKey_Insert;
    case VK_PRIOR:      return ImGuiKey_PageUp;
    case VK_NEXT:       return ImGuiKey_PageDown;
    case VK_SHIFT:
    case VK_LSHIFT:     return ImGuiKey_LeftShift;
    case VK_RSHIFT:     return ImGuiKey_RightShift;
    case VK_CONTROL:
    case VK_LCONTROL:   return ImGuiKey_LeftCtrl;
    case VK_RCONTROL:   return ImGuiKey_RightCtrl;
    case VK_MENU:
    case VK_LMENU:      return ImGuiKey_LeftAlt;
    case VK_RMENU:      return ImGuiKey_RightAlt;
    default:            break;
    }
    // Alphanumeric: VK codes for '0'-'9' and 'A'-'Z' are their ASCII values.
    if (vk >= '0' && vk <= '9') return (ImGuiKey)(ImGuiKey_0 + (vk - '0'));
    if (vk >= 'A' && vk <= 'Z') return (ImGuiKey)(ImGuiKey_A + (vk - 'A'));
    return ImGuiKey_None;
}

static void ImGui_ImplXDK_UpdateKeyboard(ImGuiIO& io)
{
    // Drain all pending keystroke events from the XInput queue.
    // XUSER_INDEX_ANY polls all connected ports simultaneously.
    XINPUT_KEYSTROKE ks;
    while (XInputGetKeystroke(XUSER_INDEX_ANY, 0, &ks) == ERROR_SUCCESS)
    {
        const bool is_down = (ks.Flags & XINPUT_KEYSTROKE_KEYDOWN) != 0;
        const bool is_up   = (ks.Flags & XINPUT_KEYSTROKE_KEYUP)   != 0;

        // Submit key state to ImGui
        ImGuiKey key = ImGui_ImplXDK_VKeyToImGuiKey(ks.VirtualKey);
        if (key != ImGuiKey_None)
        {
            if (is_down) io.AddKeyEvent(key, true);
            if (is_up)   io.AddKeyEvent(key, false);
        }

        // Submit printable characters on key-down (keystroke.Unicode is
        // already the final character — modifier keys applied by XDK).
        if (is_down && ks.Unicode != 0)
            io.AddInputCharacterUTF16(ks.Unicode);
    }
}

// ------------------------------------------------------------------------------
// System keyboard (XShowKeyboardUI)
// ------------------------------------------------------------------------------

static void ImGui_ImplXDK_UpdateSystemKeyboard(ImGuiIO& io)
{
    ImGui_ImplXDK_Data* bd = ImGui_ImplXDK_GetBackendData();
    if (!bd->KeyboardOpen)
        return;

    // Poll the async operation
    if (!XHasOverlappedIoCompleted(&bd->KeyboardOverlapped))
        return;

    // Done — inject the result into ImGui's input stream
    DWORD result = XGetOverlappedResult(&bd->KeyboardOverlapped, nullptr, FALSE);
    if (result == ERROR_SUCCESS && bd->KeyboardResult[0] != L'\0')
    {
        // Inject every UTF-16 character from the keyboard result
        for (const wchar_t* p = bd->KeyboardResult; *p; ++p)
            io.AddInputCharacterUTF16((unsigned short)*p);
    }

    // Reset state
    ZeroMemory(&bd->KeyboardOverlapped, sizeof(bd->KeyboardOverlapped));
    ZeroMemory(bd->KeyboardResult, sizeof(bd->KeyboardResult));
    bd->KeyboardOpen = false;
}

// ------------------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------------------

bool ImGui_ImplXbox360_Init(int primary_gamepad_port)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    // Validate port
    IM_ASSERT(primary_gamepad_port >= 0 && primary_gamepad_port < XUSER_MAX_COUNT);

    // Setup timing
    LARGE_INTEGER tps, now;
    if (!QueryPerformanceFrequency(&tps) || !QueryPerformanceCounter(&now))
        return false;

    ImGui_ImplXDK_Data* bd = IM_NEW(ImGui_ImplXDK_Data)();
    bd->TicksPerSecond  = tps;
    bd->LastTime        = now;
    bd->PrimaryPort     = primary_gamepad_port;

    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName     = "imgui_impl_xbox_360";

    // Xbox 360 has a gamepad, but no mouse / no cursor shapes.
    // We advertise HasGamepad after the first successful XInputGetState.
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos);

    // Enable gamepad navigation — this is the primary input method on console.
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // DisplaySize must be set by the caller after Init via:
    //   ImGui::GetIO().DisplaySize = ImVec2(1280, 720);
    // It does not change at runtime (no window resizing on console).

    return true;
}

void ImGui_ImplXbox360_Shutdown()
{
    ImGui_ImplXDK_Data* bd = ImGui_ImplXDK_GetBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    io.BackendPlatformUserData = nullptr;
    io.BackendPlatformName     = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    platform_io.ClearPlatformHandlers();
    IM_DELETE(bd);
}

void ImGui_ImplXbox360_NewFrame()
{
    ImGui_ImplXDK_Data* bd = ImGui_ImplXDK_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplXDK_Init()?");
    ImGuiIO& io = ImGui::GetIO();

    // -- Delta time ------------------------------------------------------------
    // The 360 has no variable-refresh or display resize, but we still compute
    // an accurate delta so animations and drag speeds feel correct.
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    io.DeltaTime = (float)(now.QuadPart - bd->LastTime.QuadPart) / (float)bd->TicksPerSecond.QuadPart;
    if (io.DeltaTime <= 0.0f)
        io.DeltaTime = 1.0f / 60.0f; // Safety clamp — should never happen
    bd->LastTime = now;

    // DisplaySize does not change on console — no-op here. The caller sets
    // it once after Init. Reminder:
    //   ImGui::GetIO().DisplaySize = ImVec2(1280, 720);

    // -- Input -----------------------------------------------------------------
    ImGui_ImplXDK_UpdateGamepad(io);
    ImGui_ImplXDK_UpdateKeyboard(io);
    ImGui_ImplXDK_UpdateSystemKeyboard(io);
}

void ImGui_ImplXbox360_OpenKeyboard(const wchar_t* title, const wchar_t* description, const wchar_t* initial_text)
{
    ImGui_ImplXDK_Data* bd = ImGui_ImplXDK_GetBackendData();
    IM_ASSERT(bd != nullptr);

    // Don't open a second keyboard if one is already in progress
    if (bd->KeyboardOpen)
        return;

    ZeroMemory(&bd->KeyboardOverlapped, sizeof(bd->KeyboardOverlapped));
    ZeroMemory(bd->KeyboardResult,      sizeof(bd->KeyboardResult));

    DWORD hr = XShowKeyboardUI(
        bd->PrimaryPort,            // controller port
        VKBD_DEFAULT,               // keyboard layout flags
        initial_text,               // pre-filled text
        title,                      // title shown at top
        description,                // subtitle / hint
        bd->KeyboardResult,         // output buffer (UTF-16)
        ARRAYSIZE(bd->KeyboardResult),
        &bd->KeyboardOverlapped     // async handle — we poll in NewFrame
    );

    // XShowKeyboardUI returns ERROR_IO_PENDING on success (async)
    if (hr == ERROR_IO_PENDING)
        bd->KeyboardOpen = true;
}

#endif // #ifndef IMGUI_DISABLE
