// dear imgui: Platform Backend for Emscripten
// This needs to be used along with a Renderer (e.g. OpenGL3)

// Implemented features:
//  [X] Platform: Clipboard support (with IMGUI_IMPL_EMSCRIPTEN_ENABLE_CLIPBOARD).
//  [X] Platform: Mouse support. Can discriminate Mouse/TouchScreen/Pen (with IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENTS).
//  [X] Platform: Keyboard support.
//  [X] Platform: Gamepad support.
//  [X] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors). Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Configuration flags to add in your imconfig file:
// #define IMGUI_IMPL_EMSCRIPTEN_ENABLE_CLIPBOARD   // Enable clipboard support, requires "Module['_ImGui_ImplEmscripten_ClipboardPasteCallback']"
// #define IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENTS     // Handle inputs using pointer events, requires "Module['_ImGui_ImplEmscripten_PointerEvent']" and "touch-action: none" to properly accept touch inputs.

#include "imgui.h"
#ifndef IMGUI_DISABLE

#include <emscripten/html5.h>

struct ImGui_ImplEmscripten_BackendData
{
    const char* ClipboardTextData;
    char* TargetId;
    double Time;
    int Width;
    int Height;
    ImGuiMouseCursor LastMouseCursor;
    int GamepadsCount;
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplEmscripten_BackendData* ImGui_ImplEmscripten_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplEmscripten_BackendData*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}
static ImGui_ImplEmscripten_BackendData* ImGui_ImplEmscripten_GetBackendData(ImGuiIO& io)
{
    return (ImGui_ImplEmscripten_BackendData*)io.BackendPlatformUserData;
}

// Functions
static bool ImGui_ImplEmscripten_UpdateMouseCursor(ImGuiIO& io, ImGuiMouseCursor imgui_cursor)
{
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    ImGui_ImplEmscripten_BackendData* bd = ImGui_ImplEmscripten_GetBackendData(io);

    const char* name = NULL;
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        name = "none";
    }
    else
    {
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow: name = "default"; break;
        case ImGuiMouseCursor_TextInput: name = "text"; break;
        case ImGuiMouseCursor_ResizeAll: name = "move"; break;
        case ImGuiMouseCursor_ResizeNS: name = "ns-resize"; break;
        case ImGuiMouseCursor_ResizeEW: name = "ew-resize"; break;
        case ImGuiMouseCursor_ResizeNESW: name = "nesw-resize"; break;
        case ImGuiMouseCursor_ResizeNWSE: name = "nwse-resize"; break;
        case ImGuiMouseCursor_Hand: name = "pointer"; break;
        case ImGuiMouseCursor_Wait: name = "wait"; break;
        case ImGuiMouseCursor_Progress: name = "progress"; break;
        case ImGuiMouseCursor_NotAllowed: name = "not-allowed"; break;
        }
    }

    if (name)
    {
        EM_ASM({
            document.querySelector(UTF8ToString($0)).style.cursor = UTF8ToString($1);
        }, bd->TargetId, name);

        return true;
    }

    return false;
}

// FIXME: Lookup complex keys in the hash map
// Not using charCode/keyCode because they're deprecated, tho SDL still uses them.
static ImGuiKey ImGui_ImplEmscripten_KeyToImGuiKey(const EM_UTF8(&key)[32])
{
    if (key[1] == '\0')
    {
        char c = key[0];
        if (c >= 'a' && c <= 'z')
            return (ImGuiKey)((c - 'a') + ImGuiKey_A);
        else if (c >= '0' && c <= '9')
            return (ImGuiKey)((c - '0') + ImGuiKey_0);
        else
        {
            switch (c)
            {
            case '\'': return ImGuiKey_Apostrophe;
            case ',': return ImGuiKey_Comma;
            case '-': return ImGuiKey_Minus;
            case '.': return ImGuiKey_Period;
            case '/': return ImGuiKey_Slash;
            case ';': return ImGuiKey_Semicolon;
            case '=': return ImGuiKey_Equal;
            case '[': return ImGuiKey_LeftBracket;
            case '\\': return ImGuiKey_Backslash;
            case ']': return ImGuiKey_RightBracket;
            case '`': return ImGuiKey_GraveAccent;
            case ' ': return ImGuiKey_Space;
            case 0x09: return ImGuiKey_Tab;
            case '+': return ImGuiKey_KeypadAdd;
            case '*': return ImGuiKey_KeypadMultiply;
            }
        }
    }
    else
    {
        char c0 = key[0];
        char c1 = key[1];
        char c2 = key[2];
        if (c0 == 'F' && c1 != '\0' && (c2 == '\0' || key[3] == '\0'))
        {
            // F1-F9
            if (c2 == '\0' && (c1 >= '1' && c1 <= '9'))
                return (ImGuiKey)((c1 - '1') + ImGuiKey_F1);
            else if (c1 == '1' && (c2 >= '0' && c2 <= '9')) // F10-F19
                return (ImGuiKey)((c2 - '0') + ImGuiKey_F10);
            else if (c1 == '2' && (c2 >= '0' && c2 <= '4')) // F20-F24
                return (ImGuiKey)((c2 - '0') + ImGuiKey_F20);
        }

        // Slowest path
        if (!strcmp(key, "Tab"))
            return ImGuiKey_Tab;
        else if (!strcmp(key, "ArrowLeft"))
            return ImGuiKey_LeftArrow;
        else if (!strcmp(key, "ArrowRight"))
            return ImGuiKey_RightArrow;
        else if (!strcmp(key, "ArrowUp"))
            return ImGuiKey_UpArrow;
        else if (!strcmp(key, "ArrowDown"))
            return ImGuiKey_DownArrow;
        else if (!strcmp(key, "PageUp"))
            return ImGuiKey_PageUp;
        else if (!strcmp(key, "PageDown"))
            return ImGuiKey_PageDown;
        else if (!strcmp(key, "Home"))
            return ImGuiKey_Home;
        else if (!strcmp(key, "End"))
            return ImGuiKey_End;
        else if (!strcmp(key, "Insert"))
            return ImGuiKey_Insert;
        else if (!strcmp(key, "Delete"))
            return ImGuiKey_Delete;
        else if (!strcmp(key, "Backspace"))
            return ImGuiKey_Backspace;
        else if (!strcmp(key, "Space"))
            return ImGuiKey_Space;
        else if (!strcmp(key, "Enter"))
            return ImGuiKey_Enter;
        else if (!strcmp(key, "Escape"))
            return ImGuiKey_Escape;
        else if (!strcmp(key, "ContextMenu"))
            return ImGuiKey_Menu;
        else if (!strcmp(key, "CapsLock"))
            return ImGuiKey_CapsLock;
        else if (!strcmp(key, "ScrollLock"))
            return ImGuiKey_ScrollLock;
        else if (!strcmp(key, "NumLock"))
            return ImGuiKey_NumLock;
        else if (!strcmp(key, "PrintScreen"))
            return ImGuiKey_PrintScreen;
        else if (!strcmp(key, "Pause"))
            return ImGuiKey_Pause;
    }

    return ImGuiKey_None;
}

static bool ImGui_ImplEmscripten_KeyCallback(int type, const EmscriptenKeyboardEvent* event, void* user_data)
{
    ImGuiIO& io = ImGui::GetIO();

    switch (type)
    {
    case EMSCRIPTEN_EVENT_KEYDOWN:
    case EMSCRIPTEN_EVENT_KEYUP:
    {
        if (event->repeat)
        {
            if (event->key[1] == '\0')
                io.AddInputCharacter(event->key[0]);
            return false;
        }

        io.AddKeyEvent(ImGuiMod_Ctrl, event->ctrlKey);
        io.AddKeyEvent(ImGuiMod_Shift, event->shiftKey);
        io.AddKeyEvent(ImGuiMod_Alt, event->altKey);
        io.AddKeyEvent(ImGuiMod_Super, event->metaKey);

        ImGuiKey key = ImGui_ImplEmscripten_KeyToImGuiKey(event->key);
        if (key != ImGuiKey_None)
        {
            bool down = type == EMSCRIPTEN_EVENT_KEYDOWN;
            if (down && event->key[1] == '\0')
                io.AddInputCharacter(event->key[0]);
            io.AddKeyEvent(key, down);
        }

        // Without this those keys won't be dispatched with "keyup"
        if (key == ImGuiKey_Tab || key == ImGuiKey_Backspace)
            return true;

        return false;
    }
    }

    return false;
}

#ifndef IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENTS
static bool ImGui_ImplEmscripten_MouseCallback(int type, const EmscriptenMouseEvent* event, void* user_data)
{
    ImGuiIO& io = ImGui::GetIO();

    switch (type)
    {
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
    case EMSCRIPTEN_EVENT_MOUSEUP:
    {
        unsigned short button = event->button;
        switch (button)
        {
        case 0: button = 0; break;
        case 1: button = 2; break;
        case 2: button = 1; break;
        default: return false;
        }

        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
        io.AddMouseButtonEvent(button, type == EMSCRIPTEN_EVENT_MOUSEDOWN);
        return false;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
    {
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
        io.AddMousePosEvent((float)event->clientX, (float)event->clientY);
        break;
    }
    }
    return false;
}
#endif

static bool ImGui_ImplEmscripten_WheelCallback(int type, const EmscriptenWheelEvent* event, void* user_data)
{
    if (type == EMSCRIPTEN_EVENT_WHEEL)
    {
        ImGuiIO& io = ImGui::GetIO();

        switch (event->deltaMode)
        {
        case DOM_DELTA_PIXEL:
        {
            io.AddMouseWheelEvent(-(float)event->deltaX / 100.f, -(float)event->deltaY / 100.f);
            break;
        }
        }
    }

    return false;
}

static bool ImGui_ImplEmscripten_FocusCallback(int type, const EmscriptenFocusEvent* event, void* user_data)
{
    ImGuiIO& io = ImGui::GetIO();

    switch (type)
    {
    case EMSCRIPTEN_EVENT_FOCUSIN:
    case EMSCRIPTEN_EVENT_FOCUSOUT:
    {
        io.AddFocusEvent(type == EMSCRIPTEN_EVENT_FOCUSIN);
        break;
    }
    }

    return false;
}

#ifdef IMGUI_IMPL_EMSCRIPTEN_ENABLE_CLIPBOARD
static const char* ImGui_ImplEmscripten_GetClipboardText(ImGuiContext*)
{
    ImGui_ImplEmscripten_BackendData* bd = ImGui_ImplEmscripten_GetBackendData();
    return bd->ClipboardTextData;
}

static void ImGui_ImplEmscripten_SetClipboardText(ImGuiContext*, const char* text)
{
    ImGui_ImplEmscripten_BackendData* bd = ImGui_ImplEmscripten_GetBackendData();
    if (bd->ClipboardTextData)
        free((void*)bd->ClipboardTextData);
    bd->ClipboardTextData = strdup(text);

    if (bd->ClipboardTextData)
    {
        EM_ASM({
            try { navigator.clipboard.writeText(UTF8ToString($0)); }
            catch (err) { console.error(err); }
        }, bd->ClipboardTextData);
    }
}

extern "C"
{

EMSCRIPTEN_KEEPALIVE
void ImGui_ImplEmscripten_ClipboardPasteCallback(const char* text)
{
    ImGui_ImplEmscripten_BackendData* bd = ImGui_ImplEmscripten_GetBackendData();
    if (bd->ClipboardTextData)
        free((void*)bd->ClipboardTextData);
    bd->ClipboardTextData = strdup(text);
}

};

static void ImGui_ImplEmscripten_SetupClipboardCallbacks(ImGui_ImplEmscripten_BackendData* bd)
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_SetClipboardTextFn = ImGui_ImplEmscripten_SetClipboardText;
    platform_io.Platform_GetClipboardTextFn = ImGui_ImplEmscripten_GetClipboardText;

    EM_ASM({
        const el = document.querySelector(UTF8ToString($0));

        document.addEventListener('paste', (ev) => {
            Module['_ImGui_ImplEmscripten_ClipboardPasteCallback'](stringToNewUTF8(ev.clipboardData.getData('text/plain')));
        });
    }, bd->TargetId);
}
#endif

#ifdef IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENTS
extern "C"
{

#define IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENT_HAS_POS 1
#define IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENT_HAS_BUTTON 2
#define IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENT_IS_PRIMARY 4

EMSCRIPTEN_KEEPALIVE
void ImGui_ImplEmscripten_PointerEvent(int type, int pointer_type, int flags, int x, int y, bool down, int button)
{
    if (!(flags & IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENT_IS_PRIMARY))
        return;

    ImGuiIO& io = ImGui::GetIO();

    if (type == 5)
    {
        for (int i = 0; i < ImGuiMouseButton_COUNT; i++)
            io.AddMouseButtonEvent((ImGuiMouseButton)i, false);
    }
    else if (pointer_type >= 0 && pointer_type <= 2)
    {
        ImGuiMouseSource source;
        switch (pointer_type)
        {
        case 0: source = ImGuiMouseSource_Mouse; break;
        case 1: source = ImGuiMouseSource_TouchScreen; break;
        case 2: source = ImGuiMouseSource_Pen; break;
        }

        io.AddMouseSourceEvent(source);
        if (flags & IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENT_HAS_POS)
            io.AddMousePosEvent((float)x, (float)y);
        if (flags & IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENT_HAS_BUTTON)
        {
            ImGuiMouseButton imgui_button = ImGuiMouseButton_COUNT;
            switch (button)
            {
            case 0: imgui_button = ImGuiMouseButton_Left; break;
            case 1: imgui_button = ImGuiMouseButton_Middle; break;
            case 2: imgui_button = ImGuiMouseButton_Right; break;
            case 3: imgui_button = 3; break;
            case 4: imgui_button = 4; break;
            }

            if (imgui_button != ImGuiMouseButton_COUNT)
                io.AddMouseButtonEvent(imgui_button, down);
        }
    }
}

};

static void ImGui_ImplEmscripten_SetupPointerEvents(ImGui_ImplEmscripten_BackendData* bd)
{
    EM_ASM({
        function t(name)
        {
            switch (name)
            {
            case "mouse": return 0;
            case "touch": return 1;
            case "pen": return 2;
            default: return -1;
            }
        }

        function p(ev)
        {
            return ev.isPrimary ? 4 : 0;
        }

        const el = document.querySelector(UTF8ToString($0));
        const cb = Module['_ImGui_ImplEmscripten_PointerEvent'];
        el.imguiHandlePointerDown = function(ev) { cb(0, t(ev.pointerType), 1 | 2 | p(ev), ev.clientX, ev.clientY, true, ev.button); };
        el.imguiHandlePointerUp = function(ev) { cb(1, t(ev.pointerType), 1 | 2 | p(ev), ev.clientX, ev.clientY, false, ev.button); };
        el.imguiHandlePointerMove = function(ev) { cb(2, t(ev.pointerType), 1 | p(ev), ev.clientX, ev.clientY, false); };
        el.imguiHandlePointerCancel = function(ev) { cb(3, t(ev.pointerType), 2 | p(ev), 0, 0, false, ev.button); };
        el.imguiHandlePointerEnter = function(ev) { cb(4, t(ev.pointerType), 0 | p(ev), 0, 0, false, -1); };
        el.imguiHandlePointerLeave = function(ev) { cb(5, t(ev.pointerType), 0 | p(ev), 0, 0, false, -1); };

        el.addEventListener('pointerdown', el.imguiHandlePointerDown);
        el.addEventListener('pointerup', el.imguiHandlePointerUp);
        el.addEventListener('pointermove', el.imguiHandlePointerMove);
        el.addEventListener('pointercancel', el.imguiHandlePointerCancel);
        el.addEventListener('pointerenter', el.imguiHandlePointerEnter);
        el.addEventListener('pointerleave', el.imguiHandlePointerLeave);
    }, bd->TargetId);
}

static void ImGui_ImplEmscripten_ClearPointerEvents(ImGui_ImplEmscripten_BackendData* bd)
{
    EM_ASM({
        const el = document.querySelector(UTF8ToString($0));
        el.removeEventListener('pointerdown', el.imguiHandlePointerDown);
        el.removeEventListener('pointerup', el.imguiHandlePointerUp);
        el.removeEventListener('pointermove', el.imguiHandlePointerMove);
        el.removeEventListener('pointercancel', el.imguiHandlePointerCancel);
        el.removeEventListener('pointerenter', el.imguiHandlePointerEnter);
        el.removeEventListener('pointerleave', el.imguiHandlePointerLeave);

        delete el.imguiHandlePointerDown;
        delete el.imguiHandlePointerUp;
        delete el.imguiHandlePointerMove;
        delete el.imguiHandlePointerCancel;
        delete el.imguiHandlePointerEnter;
        delete el.imguiHandlePointerLeave;
    }, bd->TargetId);
}
#endif

static bool ImGui_ImplEmscripten_GamepadCallback(int type, const EmscriptenGamepadEvent* event, void* user_data)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplEmscripten_BackendData* bd = ImGui_ImplEmscripten_GetBackendData(io);

    if (type == EMSCRIPTEN_EVENT_GAMEPADCONNECTED)
        bd->GamepadsCount++;
    else if (type == EMSCRIPTEN_EVENT_GAMEPADDISCONNECTED)
        bd->GamepadsCount--;

    if (bd->GamepadsCount > 0)
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    else
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    return false;
}

static void ImGui_ImplEmscripten_UpdateGamepads(ImGuiIO& io)
{
    if (!(io.BackendFlags & ImGuiBackendFlags_HasGamepad))
        return;

    EmscriptenGamepadEvent state;
    if (emscripten_sample_gamepad_data() != EMSCRIPTEN_RESULT_SUCCESS || emscripten_get_gamepad_status(0, &state) != EMSCRIPTEN_RESULT_SUCCESS)
        return;

    #define IM_SATURATE(V)                      (V < 0.0f ? 0.0f : V > 1.0f ? 1.0f : V)
    #define MAP_BUTTON(KEY_NO, BUTTON_IDX)      { io.AddKeyEvent(KEY_NO, state.digitalButton[BUTTON_IDX]); }
    #define MAP_ANALOG(KEY_NO, VALUE, V0, V1)   { float vn = (float)(VALUE - V0) / (float)(V1 - V0); io.AddKeyAnalogEvent(KEY_NO, vn > 0.10f, IM_SATURATE(vn)); }
    MAP_BUTTON(ImGuiKey_GamepadStart,           9);
    MAP_BUTTON(ImGuiKey_GamepadBack,            8);
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft,        2);
    MAP_BUTTON(ImGuiKey_GamepadFaceRight,       1);
    MAP_BUTTON(ImGuiKey_GamepadFaceUp,          3);
    MAP_BUTTON(ImGuiKey_GamepadFaceDown,        0);
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft,        14);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight,       15);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp,          12);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown,        13);
    MAP_BUTTON(ImGuiKey_GamepadL1,              4);
    MAP_BUTTON(ImGuiKey_GamepadR1,              5);
    MAP_ANALOG(ImGuiKey_GamepadL2,              state.analogButton[6], 0.125, 1.0);
    MAP_ANALOG(ImGuiKey_GamepadR2,              state.analogButton[7], 0.125, 1.0);
    MAP_BUTTON(ImGuiKey_GamepadL3,              10);
    MAP_BUTTON(ImGuiKey_GamepadR3,              11);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,      state.axis[0], -0.25, -1.0);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight,     state.axis[0], +0.25, +1.0);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,        state.axis[1], -0.25, -1.0);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,      state.axis[1], +0.25, +1.0);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft,      state.axis[2], -0.25, -1.0);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight,     state.axis[2], +0.25, +1.0);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp,        state.axis[3], -0.25, -1.0);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown,      state.axis[3], +0.25, +1.0);
    #undef MAP_BUTTON
    #undef MAP_ANALOG
    #undef IM_SATURATE
}

bool     ImGui_ImplEmscripten_Init(const char* target_id)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    ImGui_ImplEmscripten_BackendData* bd = IM_NEW(ImGui_ImplEmscripten_BackendData)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_emscripten";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)

    double width, height;
    emscripten_get_element_css_size(target_id, &width, &height);
    bd->Width = width;
    bd->Height = height;
    bd->Time = emscripten_performance_now();
    bd->LastMouseCursor = ImGuiMouseCursor_COUNT;
    bd->ClipboardTextData = nullptr;
    bd->GamepadsCount = 0;

    size_t target_id_size = strlen(target_id);
    bd->TargetId = (char*)IM_ALLOC(target_id_size + 1);
    memcpy(bd->TargetId, target_id, target_id_size + 1);

    io.DisplaySize = { (float)bd->Width, (float)bd->Height };

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, &ImGui_ImplEmscripten_KeyCallback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, &ImGui_ImplEmscripten_KeyCallback);

#ifdef IMGUI_IMPL_EMSCRIPTEN_ENABLE_CLIPBOARD
    ImGui_ImplEmscripten_SetupClipboardCallbacks(bd);
#endif

#ifndef IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENTS
    emscripten_set_click_callback(target_id, NULL, false, &ImGui_ImplEmscripten_MouseCallback);
    emscripten_set_mousedown_callback(target_id, NULL, false, &ImGui_ImplEmscripten_MouseCallback);
    emscripten_set_mouseup_callback(target_id, NULL, false, &ImGui_ImplEmscripten_MouseCallback);
    emscripten_set_dblclick_callback(target_id, NULL, false, &ImGui_ImplEmscripten_MouseCallback);
    emscripten_set_mousemove_callback(target_id, NULL, false, &ImGui_ImplEmscripten_MouseCallback);
    emscripten_set_mouseenter_callback(target_id, NULL, false, &ImGui_ImplEmscripten_MouseCallback);
    emscripten_set_mouseleave_callback(target_id, NULL, false, &ImGui_ImplEmscripten_MouseCallback);
#else
    ImGui_ImplEmscripten_SetupPointerEvents(bd);
#endif

    emscripten_set_wheel_callback(target_id, NULL, false, &ImGui_ImplEmscripten_WheelCallback);

    emscripten_set_focusin_callback(target_id, NULL, false, &ImGui_ImplEmscripten_FocusCallback);
    emscripten_set_focusout_callback(target_id, NULL, false, &ImGui_ImplEmscripten_FocusCallback);

    emscripten_set_gamepadconnected_callback(NULL, false, &ImGui_ImplEmscripten_GamepadCallback);
    emscripten_set_gamepaddisconnected_callback(NULL, false, &ImGui_ImplEmscripten_GamepadCallback);

    return true;
}

void     ImGui_ImplEmscripten_Shutdown()
{
    ImGui_ImplEmscripten_BackendData* bd = ImGui_ImplEmscripten_GetBackendData();

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, NULL);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, false, NULL);

#ifndef IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENTS
    emscripten_set_click_callback(bd->TargetId, NULL, false, NULL);
    emscripten_set_mousedown_callback(bd->TargetId, NULL, false, NULL);
    emscripten_set_mouseup_callback(bd->TargetId, NULL, false, NULL);
    emscripten_set_dblclick_callback(bd->TargetId, NULL, false, NULL);
    emscripten_set_mousemove_callback(bd->TargetId, NULL, false, NULL);
    emscripten_set_mouseenter_callback(bd->TargetId, NULL, false, NULL);
    emscripten_set_mouseleave_callback(bd->TargetId, NULL, false, NULL);
#else
    ImGui_ImplEmscripten_ClearPointerEvents(bd);
#endif

    emscripten_set_wheel_callback(bd->TargetId, NULL, false, NULL);

    emscripten_set_focusin_callback(bd->TargetId, NULL, false, NULL);
    emscripten_set_focusout_callback(bd->TargetId, NULL, false, NULL);

    emscripten_set_gamepadconnected_callback(NULL, false, NULL);
    emscripten_set_gamepaddisconnected_callback(NULL, false, NULL);

    free((void*)bd->TargetId);
    IM_DELETE(bd);
}

void     ImGui_ImplEmscripten_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplEmscripten_BackendData* bd = ImGui_ImplEmscripten_GetBackendData(io);

    double current_time = emscripten_performance_now();
    io.DeltaTime = (float)((current_time - bd->Time) / 1000.0);
    bd->Time = current_time;

    // Update OS mouse cursor with the cursor requested by imgui
    ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (bd->LastMouseCursor != mouse_cursor)
    {
        bd->LastMouseCursor = mouse_cursor;
        ImGui_ImplEmscripten_UpdateMouseCursor(io, mouse_cursor);
    }

    ImGui_ImplEmscripten_UpdateGamepads(io);
}

void     ImGui_ImplEmscripten_UpdateCanvasSize(int width, int height)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplEmscripten_BackendData* bd = ImGui_ImplEmscripten_GetBackendData(io);

    bd->Width = width;
    bd->Height = height;

    io.DisplaySize = { (float)bd->Width, (float)bd->Height };
}

#endif // #ifndef IMGUI_DISABLE
