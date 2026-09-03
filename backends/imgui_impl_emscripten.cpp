// dear imgui: Platform Backend for Emscripten HTML5
//
// See documentation in imgui_impl_emscripten.h.
//
// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2026-09-03: Updated coding style and simplified key translation table setup.
//  2026-04-02: Inputs: Replaced custom KeyboardEvent.code parser with ImHashStr()/ImGuiStorage lookup to match Dear ImGui backend style.
//  2026-03-31: Emscripten: Added configurable TargetDevicePixelRatio to control how browser device pixels map to Dear ImGui pixels.
//  2026-03-31: Inputs: Added BrowserBack/Forward and F13-F24 key mappings.
//  2026-03-31: Emscripten: Moved cursor state into backend userdata and replaced cursor restore storage with owned C strings.
//  2024-12-09: Inputs: Added special handling for modifier keys to also generate modifier key events.
//  2024-12-08: Inputs: Prevent "Delete" key from getting printed in text input.
//  2024-12-06: Inputs: Added special handling for Tab and Enter event capture.
//  2024-12-06: Inputs: Handle blur and focus events correctly, focusin and focusout aren't enough.
//  2024-12-06: Emscripten: Don't rely on devicePixelRatio for WebGPU framebuffer sizing; CSS->ImGui scaling may still use it.
//  2024-11-22: Initial version by Eugene Hopkinson. (#8178)

#include "imgui.h"
#ifndef IMGUI_DISABLE

#include "imgui_impl_emscripten.h"
#include <emscripten.h>
#include <emscripten/html5.h>

extern ImGuiID ImHashStr(char const* data, size_t data_size = 0, ImGuiID seed = 0); // Declared in imgui_internal.h.

float ImGui_ImplEmscripten_TargetDevicePixelRatio = 1.0f;

// W3C UI Events KeyboardEvent.code translation helpers

static ImGuiKey ImGui_ImplEmscripten_TranslateKey(char const* emscripten_key);
static constexpr ImGuiMouseButton ImGui_ImplEmscripten_TranslateMouseButton(unsigned short emscripten_button) __attribute__((__const__));

namespace emscripten_browser_cursor_internal
{

// Browser cursor helpers, adapted from https://github.com/Armchair-Software/emscripten-browser-cursor

enum class cursor
{
    // General
    cursor_auto,                                                                // The UA will determine the cursor to display based on the current context. E.g., equivalent to text when hovering text.
    cursor_default,                                                             // The platform-dependent default cursor. Typically an arrow.
    none,                                                                       // No cursor is rendered.

    // Links & status
    context_menu,                                                               // cursor slightly obscuring a menu icon - A context menu is available.
    help,                                                                       // cursor next to a question mark - Help information is available.
    pointer,                                                                    // right hand with an index finger pointing up - The cursor is a pointer that indicates a link. Typically an image of a pointing hand.
    progress,                                                                   // cursor and hour glass - The program is busy in the background, but the user can still interact with the interface (in contrast to wait).
    wait,                                                                       // hour glass - The program is busy, and the user can't interact with the interface (in contrast to progress). Sometimes an image of an hourglass or a watch.

    // Selection
    cell,                                                                       // plus symbol - The table cell or set of cells can be selected.
    crosshair,                                                                  // crosshair  - Cross cursor, often used to indicate selection in a bitmap.
    text,                                                                       // vertical i-beam - The text can be selected. Typically the shape of an I-beam.
    vertical_text,                                                              // horizontal i-beam - The vertical text can be selected. Typically the shape of a sideways I-beam.

    // Drag & drop
    alias,                                                                      // cursor next to a folder icon with a curved arrow pointing up and to the right - An alias or shortcut is to be created.
    copy,                                                                       // cursor next to a smaller folder icon with a plus sign - Something is to be copied.
    move,                                                                       // plus sign made of two thin lines, with small arrows facing out - Something is to be moved.
    no_drop,                                                                    // cursor next to circle with a line through it - An item may not be dropped at the current location.
    not_allowed,                                                                // circle with a line through it - The requested action will not be carried out.
    grab,                                                                       // fully opened hand - Something can be grabbed (dragged to be moved).
    grabbing,                                                                   // closed hand - Something is being grabbed (dragged to be moved).

    // Resizing & scrolling
    all_scroll,                                                                 // dot with four triangles around it - Something can be scrolled in any direction (panned).
    col_resize,                                                                 // The item/column can be resized horizontally. Often rendered as arrows pointing left and right with a vertical bar separating them.
    row_resize,                                                                 // The item/row can be resized vertically. Often rendered as arrows pointing up and down with a horizontal bar separating them.
    n_resize,                                                                   // arrow pointing up - Some edge is to be moved. For example, the se-resize cursor is used when the movement starts from the south-east corner of the box.
    e_resize,                                                                   // arrow pointing right
    s_resize,                                                                   // arrow pointing down
    w_resize,                                                                   // arrow pointing left
    ne_resize,                                                                  // arrow pointing top-right
    nw_resize,                                                                  // arrow pointing top-left
    se_resize,                                                                  // arrow pointing bottom-right
    sw_resize,                                                                  // arrow pointing bottom-left
    ew_resize,                                                                  // arrow pointing left and right - Bidirectional resize cursor.
    ns_resize,                                                                  // arrow pointing up and down
    nesw_resize,                                                                // arrow pointing both to the top-right and bottom-left
    nwse_resize,                                                                // arrow pointing both to the top-left and bottom-right

    // Zooming
    zoom_in,                                                                    // magnifying glass with a plus sign - Something can be zoomed (magnified) in or out.
    zoom_out,

    // Special invalid value
    invalid
};

} // namespace emscripten_browser_cursor_internal

static void ImGui_ImplEmscripten_SetBrowserCursor(emscripten_browser_cursor_internal::cursor new_cursor); // set a new cursor from a cursor enum
static char* ImGui_ImplEmscripten_GetBrowserCursor();                           // read the current cursor setting as an owned string, caller must free()
static void ImGui_ImplEmscripten_SetBrowserCursor(char const* new_cursor);      // set the cursor from an arbitrary string

struct ImGui_ImplEmscripten_Data
{
    float CssToImGuiScale = 1.0f;
    emscripten_browser_cursor_internal::cursor CurrentCursor = emscripten_browser_cursor_internal::cursor::invalid;
    char* CursorToRestore = nullptr;
    bool LastMouseDrawCursor = false;
    bool LastNoMouseCursorChange = false;
};

static float ImGui_ImplEmscripten_GetTargetDevicePixelRatio()
{
    IM_ASSERT(ImGui_ImplEmscripten_TargetDevicePixelRatio > 0.0f && "ImGui_ImplEmscripten_TargetDevicePixelRatio must be positive.");
    return ImGui_ImplEmscripten_TargetDevicePixelRatio > 0.0f ? ImGui_ImplEmscripten_TargetDevicePixelRatio : 1.0f;
}

static void ImGui_ImplEmscripten_UpdateDisplayProperties(ImGuiIO& io, ImGui_ImplEmscripten_Data* bd, float css_width, float css_height)
{
    float const target_device_pixel_ratio = ImGui_ImplEmscripten_GetTargetDevicePixelRatio();
    float const css_to_imgui_scale = (float)emscripten_get_device_pixel_ratio() / target_device_pixel_ratio;
    bd->CssToImGuiScale = css_to_imgui_scale;
    io.DisplaySize.x = css_width * css_to_imgui_scale;
    io.DisplaySize.y = css_height * css_to_imgui_scale;
    io.DisplayFramebufferScale = ImVec2(target_device_pixel_ratio, target_device_pixel_ratio);
}

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
static ImGui_ImplEmscripten_Data* ImGui_ImplEmscripten_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplEmscripten_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

void ImGui_ImplEmscripten_Init()
{
    // Initialise the Emscripten backend, setting input callbacks
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");
    ImGui_ImplEmscripten_Data* bd = IM_NEW(ImGui_ImplEmscripten_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_emscripten";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

    // set up initial display size values
    ImGui_ImplEmscripten_UpdateDisplayProperties(
        io,
        bd,
        (float)EM_ASM_INT(return window.innerWidth;),
        (float)EM_ASM_INT(return window.innerHeight;)
    );

    emscripten_set_mousemove_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenMouseEvent const* mouse_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_MOUSEMOVE
            ImGui_ImplEmscripten_Data* bd = ImGui_ImplEmscripten_GetBackendData();
            float const css_to_imgui_scale = bd ? bd->CssToImGuiScale : 1.0f;
            ImGui::GetIO().AddMousePosEvent(
                (float)mouse_event->clientX * css_to_imgui_scale,
                (float)mouse_event->clientY * css_to_imgui_scale
            );
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_mousedown_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenMouseEvent const* mouse_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_MOUSEDOWN
            ImGui::GetIO().AddMouseButtonEvent(ImGui_ImplEmscripten_TranslateMouseButton(mouse_event->button), true); // translated button, down
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_mouseup_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenMouseEvent const* mouse_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_MOUSEUP
            ImGui::GetIO().AddMouseButtonEvent(ImGui_ImplEmscripten_TranslateMouseButton(mouse_event->button), false); // translated button, up
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_mouseenter_callback(
        EMSCRIPTEN_EVENT_TARGET_DOCUMENT,                                       // target - WINDOW doesn't produce mouseenter events
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenMouseEvent const* mouse_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_MOUSEENTER
            ImGui_ImplEmscripten_Data* bd = ImGui_ImplEmscripten_GetBackendData();
            float const css_to_imgui_scale = bd ? bd->CssToImGuiScale : 1.0f;
            ImGui::GetIO().AddMousePosEvent(
                (float)mouse_event->clientX * css_to_imgui_scale,
                (float)mouse_event->clientY * css_to_imgui_scale
            );
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_mouseleave_callback(
        EMSCRIPTEN_EVENT_TARGET_DOCUMENT,                                       // target - WINDOW doesn't produce mouseenter events
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenMouseEvent const* /*mouse_event*/, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_MOUSELEAVE
            ImGuiIO& io = ImGui::GetIO();
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);                            // cursor is not in the window
            io.ClearInputKeys();                                                // clear pending input keys on mouse exit
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_wheel_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenWheelEvent const* wheel_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_WHEEL
            float scale = 1.0f;
            switch (wheel_event->deltaMode)
            {
            case DOM_DELTA_PIXEL:                                               // scrolling in pixels
                scale = 1.0f / 100.0f;
                break;
            case DOM_DELTA_LINE:                                                // scrolling by lines
                scale = 1.0f / 3.0f;
                break;
            case DOM_DELTA_PAGE:                                                // scrolling by pages
                scale = 80.0f;
                break;
            }
            // TODO: make scrolling speeds configurable
            ImGuiIO& io = ImGui::GetIO();
            io.AddMouseWheelEvent(
                -(float)wheel_event->deltaX * scale,
                -(float)wheel_event->deltaY * scale
            );
            return io.WantCaptureMouse;                                         // consume the event when imgui wants to capture mouse input
        }
    );
    emscripten_set_keydown_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenKeyboardEvent const* key_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_KEYDOWN
            const ImGuiKey key = ImGui_ImplEmscripten_TranslateKey(key_event->code);
            ImGuiIO& io = ImGui::GetIO();
            io.AddKeyEvent(key, true);
            switch (key)                                                        // special cases for certain key events
            {
            case ImGuiKey_LeftCtrl:                                             // additional events for modifier keys
            case ImGuiKey_RightCtrl:
                io.AddKeyEvent(ImGuiMod_Ctrl, true);
                break;
            case ImGuiKey_LeftShift:
            case ImGuiKey_RightShift:
                io.AddKeyEvent(ImGuiMod_Shift, true);
                break;
            case ImGuiKey_LeftAlt:
            case ImGuiKey_RightAlt:
                io.AddKeyEvent(ImGuiMod_Alt, true);
                break;
            case ImGuiKey_LeftSuper:
            case ImGuiKey_RightSuper:
                io.AddKeyEvent(ImGuiMod_Super, true);
                break;
            // TODO: case ImGuiKey_Menu: do we want to do anything with this?
            case ImGuiKey_Tab:                                                  // consuming tab prevents the user tabbing to other parts of the browser interface outside the window content
                return io.WantCaptureKeyboard;                                  // the event was consumed only if imgui wants to capture the keyboard
            case ImGuiKey_Enter:                                                // consuming enter prevents the word "Enter" appearing in text input via the keypress callback
            case ImGuiKey_Delete:                                               // consuming enter prevents the word "Delete" appearing in text input via the keypress callback
                return io.WantTextInput;                                        // the event was consumed only if we're currently accepting text input
            default:
                break;
            }
            return false;                                                       // if no special handling, the event was not consumed
        }
    );
    emscripten_set_keyup_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenKeyboardEvent const* key_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_KEYUP
            const ImGuiKey key = ImGui_ImplEmscripten_TranslateKey(key_event->code);
            ImGuiIO& io = ImGui::GetIO();
            io.AddKeyEvent(key, false);
            switch (key)                                                        // special cases for certain key events
            {
            case ImGuiKey_LeftCtrl:                                             // additional events for modifier keys
            case ImGuiKey_RightCtrl:
                io.AddKeyEvent(ImGuiMod_Ctrl, false);
                break;
            case ImGuiKey_LeftShift:
            case ImGuiKey_RightShift:
                io.AddKeyEvent(ImGuiMod_Shift, false);
                break;
            case ImGuiKey_LeftAlt:
            case ImGuiKey_RightAlt:
                io.AddKeyEvent(ImGuiMod_Alt, false);
                break;
            case ImGuiKey_LeftSuper:
            case ImGuiKey_RightSuper:
                io.AddKeyEvent(ImGuiMod_Super, false);
                break;
            default:
                break;
            }
            return false;                                                       // the event was not consumed
        }
    );
    emscripten_set_keypress_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenKeyboardEvent const* key_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_KEYPRESS
            ImGuiIO& io = ImGui::GetIO();
            io.AddInputCharactersUTF8(key_event->key);
            return io.WantCaptureKeyboard;                                      // the event was consumed only if imgui wants to capture the keyboard
        }
    );
    emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenUiEvent const* event, void* /*data*/) { // event_type == EMSCRIPTEN_EVENT_RESIZE
            ImGuiIO& io = ImGui::GetIO();
            ImGui_ImplEmscripten_Data* bd = ImGui_ImplEmscripten_GetBackendData();
            if (bd != nullptr) ImGui_ImplEmscripten_UpdateDisplayProperties(io, bd, (float)event->windowInnerWidth, (float)event->windowInnerHeight);
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_blur_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenFocusEvent const* /*event*/, void* /*data*/) { // event_type == EMSCRIPTEN_EVENT_BLUR
            ImGuiIO& io = ImGui::GetIO();
            io.AddFocusEvent(false);
            io.ClearInputKeys();                                                // clear pending input keys on focus loss
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_focus_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenFocusEvent const* /*event*/, void* /*data*/) { // event_type == EMSCRIPTEN_EVENT_FOCUS
            ImGuiIO& io = ImGui::GetIO();
            io.AddFocusEvent(true);
            io.ClearInputKeys();                                                // clear pending input keys on focus gain - for example if you press tab to cycle back into the browser window
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_focusin_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenFocusEvent const* /*event*/, void* /*data*/) { // event_type == EMSCRIPTEN_EVENT_FOCUSIN
            ImGuiIO& io = ImGui::GetIO();
            io.AddFocusEvent(true);
            io.ClearInputKeys();                                                // clear pending input keys on focus gain
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_focusout_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenFocusEvent const* /*event*/, void* /*data*/) { // event_type == EMSCRIPTEN_EVENT_FOCUSOUT
            ImGuiIO& io = ImGui::GetIO();
            io.AddFocusEvent(false);
            io.ClearInputKeys();                                                // clear pending input keys on focus loss - for example if you press tab to cycle to another part of the UI
            return true;                                                        // the event was consumed
        }
    );

    // TODO: touch events
}

void ImGui_ImplEmscripten_Shutdown()
{
    ImGui_ImplEmscripten_Data* bd = ImGui_ImplEmscripten_GetBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");

    // Unset any callbacks set by Init
    emscripten_set_mousemove_callback( EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    emscripten_set_mousedown_callback( EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    emscripten_set_mouseup_callback(   EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    emscripten_set_mouseenter_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, false, nullptr);
    emscripten_set_mouseleave_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, false, nullptr);
    emscripten_set_wheel_callback(     EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    emscripten_set_keydown_callback(   EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    emscripten_set_keyup_callback(     EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    emscripten_set_keypress_callback(  EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    emscripten_set_resize_callback(    EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    emscripten_set_blur_callback(      EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    emscripten_set_focus_callback(     EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    emscripten_set_focusin_callback(   EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    emscripten_set_focusout_callback(  EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
    // TODO: touch events

    if (bd->CursorToRestore != nullptr)
    {
        ImGui_ImplEmscripten_SetBrowserCursor(bd->CursorToRestore);             // restore the previous cursor state if imgui still owns the cursor on shutdown
        free(bd->CursorToRestore);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_HasMouseCursors;
    IM_DELETE(bd);
}

static void ImGui_ImplEmscripten_RestoreMouseCursor(ImGui_ImplEmscripten_Data* bd)
{
    if (bd->CursorToRestore == nullptr) return;
    ImGui_ImplEmscripten_SetBrowserCursor(bd->CursorToRestore);                 // restore the previous cursor state when leaving imgui cursor ownership
    free(bd->CursorToRestore);
    bd->CursorToRestore = nullptr;
    bd->CurrentCursor = emscripten_browser_cursor_internal::cursor::invalid;    // select an unused value for current cursor to force a set next time
}

static void ImGui_ImplEmscripten_SetMouseCursor(ImGui_ImplEmscripten_Data* bd, emscripten_browser_cursor_internal::cursor new_cursor)
{
    if (new_cursor == bd->CurrentCursor) return;                                // don't do anything if the current cursor is already set
    if (bd->CursorToRestore == nullptr) bd->CursorToRestore = ImGui_ImplEmscripten_GetBrowserCursor(); // back up the existing cursor state when first taking cursor ownership
    bd->CurrentCursor = new_cursor;
    ImGui_ImplEmscripten_SetBrowserCursor(new_cursor);
}

static void ImGui_ImplEmscripten_UpdateMouseCursor(ImGui_ImplEmscripten_Data* bd)
{
    // Sync any cursor changes due to ImGui to the browser's cursor
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
    {
        if (!bd->LastNoMouseCursorChange)
        {
            free(bd->CursorToRestore);
            bd->CursorToRestore = nullptr;
            bd->CurrentCursor = emscripten_browser_cursor_internal::cursor::invalid;
            bd->LastMouseDrawCursor = false;
            bd->LastNoMouseCursorChange = true;
        }
        return;
    }
    bd->LastNoMouseCursorChange = false;

    if (io.MouseDrawCursor)
    {
        if (bd->LastMouseDrawCursor) return;

        ImGui_ImplEmscripten_SetMouseCursor(bd, emscripten_browser_cursor_internal::cursor::none); // hide the cursor for the entire window if imgui is handling cursor drawing - not just when imgui wants to capture the mouse
        bd->LastMouseDrawCursor = true;
    }
    else
    {
        if (bd->LastMouseDrawCursor)
        {
            ImGui_ImplEmscripten_RestoreMouseCursor(bd);
            bd->LastMouseDrawCursor = false;
        }
    }

    if (io.WantCaptureMouse)                                                    // mouse is hovering over the gui
    {
        switch (ImGui::GetMouseCursor())
        {
        case ImGuiMouseCursor_None:
            ImGui_ImplEmscripten_SetMouseCursor(bd, emscripten_browser_cursor_internal::cursor::none);
            break;
        case ImGuiMouseCursor_Arrow:
            ImGui_ImplEmscripten_SetMouseCursor(bd, emscripten_browser_cursor_internal::cursor::cursor_default);
            break;
        case ImGuiMouseCursor_TextInput:                                        // When hovering over InputText, etc.
            ImGui_ImplEmscripten_SetMouseCursor(bd, emscripten_browser_cursor_internal::cursor::text);
            break;
        case ImGuiMouseCursor_ResizeAll:                                        // (Unused by Dear ImGui functions)
            ImGui_ImplEmscripten_SetMouseCursor(bd, emscripten_browser_cursor_internal::cursor::move);
            break;
        case ImGuiMouseCursor_ResizeNS:                                         // When hovering over a horizontal border
            ImGui_ImplEmscripten_SetMouseCursor(bd, emscripten_browser_cursor_internal::cursor::ns_resize);
            break;
        case ImGuiMouseCursor_ResizeEW:                                         // When hovering over a vertical border or a column
            ImGui_ImplEmscripten_SetMouseCursor(bd, emscripten_browser_cursor_internal::cursor::ew_resize);
            break;
        case ImGuiMouseCursor_ResizeNESW:                                       // When hovering over the bottom-left corner of a window
            ImGui_ImplEmscripten_SetMouseCursor(bd, emscripten_browser_cursor_internal::cursor::nesw_resize);
            break;
        case ImGuiMouseCursor_ResizeNWSE:                                       // When hovering over the bottom-right corner of a window
            ImGui_ImplEmscripten_SetMouseCursor(bd, emscripten_browser_cursor_internal::cursor::nwse_resize);
            break;
        case ImGuiMouseCursor_Hand:                                             // (Unused by Dear ImGui functions. Use for e.g. hyperlinks)
            ImGui_ImplEmscripten_SetMouseCursor(bd, emscripten_browser_cursor_internal::cursor::pointer);
            break;
        case ImGuiMouseCursor_NotAllowed:                                       // When hovering something with disallowed interaction. Usually a crossed circle.
            ImGui_ImplEmscripten_SetMouseCursor(bd, emscripten_browser_cursor_internal::cursor::not_allowed);
            break;
        }
    }
    else                                                                        // mouse is away from the gui, hovering over some other part of the viewport
    {
        ImGui_ImplEmscripten_RestoreMouseCursor(bd);
    }
}

void ImGui_ImplEmscripten_NewFrame()
{
    ImGui_ImplEmscripten_Data* bd = ImGui_ImplEmscripten_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized? Did you call ImGui_ImplEmscripten_Init()?");

    // Update any state that needs to be polled
    ImGui_ImplEmscripten_UpdateMouseCursor(bd);
}

static char* ImGui_ImplEmscripten_GetBrowserCursor()
{
    // Return the current cursor setting as a newly-allocated string, caller must free it.
    return (char*)EM_ASM_PTR(
        return stringToNewUTF8(document.body.style.cursor);
    );
}

static void ImGui_ImplEmscripten_SetBrowserCursor(emscripten_browser_cursor_internal::cursor new_cursor)
{
    // Set the cursor according to the given enum
    // Note, implementations omitted for cursors not used by imgui.  For full implementation, use https://github.com/Armchair-Software/emscripten-browser-cursor
    switch (new_cursor)
    {
    case emscripten_browser_cursor_internal::cursor::none:
        EM_ASM(document.body.style.cursor = 'none';);
        break;
    case emscripten_browser_cursor_internal::cursor::cursor_default:
    default:
        EM_ASM(document.body.style.cursor = 'default';);
        break;
    case emscripten_browser_cursor_internal::cursor::pointer:
        EM_ASM(document.body.style.cursor = 'pointer';);
        break;
    case emscripten_browser_cursor_internal::cursor::text:
        EM_ASM(document.body.style.cursor = 'text';);
        break;
    case emscripten_browser_cursor_internal::cursor::move:
        EM_ASM(document.body.style.cursor = 'move';);
        break;
    case emscripten_browser_cursor_internal::cursor::not_allowed:
        EM_ASM(document.body.style.cursor = 'not-allowed';);
        break;
    case emscripten_browser_cursor_internal::cursor::ew_resize:
        EM_ASM(document.body.style.cursor = 'ew-resize';);
        break;
    case emscripten_browser_cursor_internal::cursor::ns_resize:
        EM_ASM(document.body.style.cursor = 'ns-resize';);
        break;
    case emscripten_browser_cursor_internal::cursor::nesw_resize:
        EM_ASM(document.body.style.cursor = 'nesw-resize';);
        break;
    case emscripten_browser_cursor_internal::cursor::nwse_resize:
        EM_ASM(document.body.style.cursor = 'nwse-resize';);
        break;
    }
}

static void ImGui_ImplEmscripten_SetBrowserCursor(char const* new_cursor)
{
    // Set the cursor from an arbitrary string
    EM_ASM({
        document.body.style.cursor = UTF8ToString($0);
    }, new_cursor);
}

static constexpr ImGuiMouseButton ImGui_ImplEmscripten_TranslateMouseButton(unsigned short emscripten_button)
{
    // Translate an emscripten-provided integer describing a mouse button to an imgui mouse button
    if (emscripten_button == 1) return ImGuiMouseButton_Middle;                 // 1 = middle mouse button
    if (emscripten_button == 2) return ImGuiMouseButton_Right;                  // 2 = right mouse button
    if (emscripten_button >= ImGuiMouseButton_COUNT) return ImGuiMouseButton_Middle; // treat any weird clicks on unexpected buttons (button 6 upwards) as middle mouse
    return emscripten_button;                                                   // any other button translates 1:1
}

static ImGuiStorage const& ImGui_ImplEmscripten_GetKeyTranslationStorage()
{
    struct ImGui_ImplEmscripten_KeyTranslation
    {
        const char* EmscriptenKey;
        ImGuiKey Key;
    };
    static const ImGui_ImplEmscripten_KeyTranslation key_translations[] =
    {
        // main character keys
        { "Backquote",                 ImGuiKey_GraveAccent        },
        { "Backslash",                 ImGuiKey_Backslash          },
        { "BracketLeft",               ImGuiKey_LeftBracket        },
        { "BracketRight",              ImGuiKey_RightBracket       },
        { "Comma",                     ImGuiKey_Comma              },
        { "Digit0",                    ImGuiKey_0                  },
        { "Digit1",                    ImGuiKey_1                  },
        { "Digit2",                    ImGuiKey_2                  },
        { "Digit3",                    ImGuiKey_3                  },
        { "Digit4",                    ImGuiKey_4                  },
        { "Digit5",                    ImGuiKey_5                  },
        { "Digit6",                    ImGuiKey_6                  },
        { "Digit7",                    ImGuiKey_7                  },
        { "Digit8",                    ImGuiKey_8                  },
        { "Digit9",                    ImGuiKey_9                  },
        { "Equal",                     ImGuiKey_Equal              },
        { "IntlBackslash",             ImGuiKey_Backslash          },           // Mapping to generic backslash
        { "IntlRo",                    ImGuiKey_Slash              },           // Closest match for non-standard layouts
        { "IntlYen",                   ImGuiKey_Backslash          },           // Closest match for non-standard layouts
        { "KeyA",                      ImGuiKey_A                  },
        { "KeyB",                      ImGuiKey_B                  },
        { "KeyC",                      ImGuiKey_C                  },
        { "KeyD",                      ImGuiKey_D                  },
        { "KeyE",                      ImGuiKey_E                  },
        { "KeyF",                      ImGuiKey_F                  },
        { "KeyG",                      ImGuiKey_G                  },
        { "KeyH",                      ImGuiKey_H                  },
        { "KeyI",                      ImGuiKey_I                  },
        { "KeyJ",                      ImGuiKey_J                  },
        { "KeyK",                      ImGuiKey_K                  },
        { "KeyL",                      ImGuiKey_L                  },
        { "KeyM",                      ImGuiKey_M                  },
        { "KeyN",                      ImGuiKey_N                  },
        { "KeyO",                      ImGuiKey_O                  },
        { "KeyP",                      ImGuiKey_P                  },
        { "KeyQ",                      ImGuiKey_Q                  },
        { "KeyR",                      ImGuiKey_R                  },
        { "KeyS",                      ImGuiKey_S                  },
        { "KeyT",                      ImGuiKey_T                  },
        { "KeyU",                      ImGuiKey_U                  },
        { "KeyV",                      ImGuiKey_V                  },
        { "KeyW",                      ImGuiKey_W                  },
        { "KeyX",                      ImGuiKey_X                  },
        { "KeyY",                      ImGuiKey_Y                  },
        { "KeyZ",                      ImGuiKey_Z                  },
        { "Minus",                     ImGuiKey_Minus              },
        { "Period",                    ImGuiKey_Period             },
        { "Quote",                     ImGuiKey_Apostrophe         },
        { "Semicolon",                 ImGuiKey_Semicolon          },
        { "Slash",                     ImGuiKey_Slash              },

        // control keys
        { "AltLeft",                   ImGuiKey_LeftAlt            },
        { "AltRight",                  ImGuiKey_RightAlt           },
        { "Backspace",                 ImGuiKey_Backspace          },
        { "CapsLock",                  ImGuiKey_CapsLock           },
        { "ContextMenu",               ImGuiKey_Menu               },
        { "ControlLeft",               ImGuiKey_LeftCtrl           },
        { "ControlRight",              ImGuiKey_RightCtrl          },
        { "Enter",                     ImGuiKey_Enter              },
        { "MetaLeft",                  ImGuiKey_LeftSuper          },
        { "MetaRight",                 ImGuiKey_RightSuper         },
        { "ShiftLeft",                 ImGuiKey_LeftShift          },
        { "ShiftRight",                ImGuiKey_RightShift         },
        { "Space",                     ImGuiKey_Space              },
        { "Tab",                       ImGuiKey_Tab                },

        // navigation key group
        { "Delete",                    ImGuiKey_Delete             },
        { "End",                       ImGuiKey_End                },
        //{ "Help",                      ImGuiKey_PrintScreen        },           // Best approximation
        { "Home",                      ImGuiKey_Home               },
        { "Insert",                    ImGuiKey_Insert             },
        { "PageDown",                  ImGuiKey_PageDown           },
        { "PageUp",                    ImGuiKey_PageUp             },

        // arrow key group
        { "ArrowDown",                 ImGuiKey_DownArrow          },
        { "ArrowLeft",                 ImGuiKey_LeftArrow          },
        { "ArrowRight",                ImGuiKey_RightArrow         },
        { "ArrowUp",                   ImGuiKey_UpArrow            },

        // browser key group
        { "BrowserBack",               ImGuiKey_AppBack            },           // Pass through so the embedding app can decide
        //{ "BrowserFavorites",          ImGuiKey_None               },           // No direct mapping
        { "BrowserForward",            ImGuiKey_AppForward         },           // Pass through so the embedding app can decide
        //{ "BrowserHome",               ImGuiKey_None               },           // No direct mapping
        //{ "BrowserRefresh",            ImGuiKey_None               },           // No direct mapping
        //{ "BrowserSearch",             ImGuiKey_None               },           // No direct mapping
        //{ "BrowserStop",               ImGuiKey_None               },           // No direct mapping

        // number pad group
        { "NumLock",                   ImGuiKey_NumLock            },
        { "Numpad0",                   ImGuiKey_Keypad0            },
        { "Numpad1",                   ImGuiKey_Keypad1            },
        { "Numpad2",                   ImGuiKey_Keypad2            },
        { "Numpad3",                   ImGuiKey_Keypad3            },
        { "Numpad4",                   ImGuiKey_Keypad4            },
        { "Numpad5",                   ImGuiKey_Keypad5            },
        { "Numpad6",                   ImGuiKey_Keypad6            },
        { "Numpad7",                   ImGuiKey_Keypad7            },
        { "Numpad8",                   ImGuiKey_Keypad8            },
        { "Numpad9",                   ImGuiKey_Keypad9            },
        { "NumpadAdd",                 ImGuiKey_KeypadAdd          },
        { "NumpadBackspace",           ImGuiKey_Backspace          },           // No direct mapping; backspace functionality
        //{ "NumpadClear",               ImGuiKey_None               },           // No defined Dear ImGui mapping
        //{ "NumpadClearEntry",          ImGuiKey_None               },           // No defined Dear ImGui mapping
        { "NumpadComma",               ImGuiKey_KeypadDecimal      },           // Closest match
        { "NumpadDecimal",             ImGuiKey_KeypadDecimal      },
        { "NumpadDivide",              ImGuiKey_KeypadDivide       },
        { "NumpadEnter",               ImGuiKey_KeypadEnter        },
        { "NumpadEqual",               ImGuiKey_KeypadEqual        },
        { "NumpadHash",                ImGuiKey_Backslash          },           // Mapped to generic backslash for telephone-style '#'
        //{ "NumpadMemoryAdd",           ImGuiKey_None               },           // No defined mapping
        //{ "NumpadMemoryClear",         ImGuiKey_None               },           // No defined mapping
        //{ "NumpadMemoryRecall",        ImGuiKey_None               },           // No defined mapping
        //{ "NumpadMemoryStore",         ImGuiKey_None               },           // No defined mapping
        //{ "NumpadMemorySubtract",      ImGuiKey_None               },           // No defined mapping
        { "NumpadMultiply",            ImGuiKey_KeypadMultiply     },
        { "NumpadParenLeft",           ImGuiKey_LeftBracket        },           // Closest available
        { "NumpadParenRight",          ImGuiKey_RightBracket       },           // Closest available
        { "NumpadStar",                ImGuiKey_KeypadMultiply     },           // Same as multiply
        { "NumpadSubtract",            ImGuiKey_KeypadSubtract     },

        // top row key group
        { "Escape",                    ImGuiKey_Escape             },
        { "F1",                        ImGuiKey_F1                 },
        { "F2",                        ImGuiKey_F2                 },
        { "F3",                        ImGuiKey_F3                 },
        { "F4",                        ImGuiKey_F4                 },
        { "F5",                        ImGuiKey_F5                 },
        { "F6",                        ImGuiKey_F6                 },
        { "F7",                        ImGuiKey_F7                 },
        { "F8",                        ImGuiKey_F8                 },
        { "F9",                        ImGuiKey_F9                 },
        { "F10",                       ImGuiKey_F10                },
        { "F11",                       ImGuiKey_F11                },
        { "F12",                       ImGuiKey_F12                },
        { "F13",                       ImGuiKey_F13                },
        { "F14",                       ImGuiKey_F14                },
        { "F15",                       ImGuiKey_F15                },
        { "F16",                       ImGuiKey_F16                },
        { "F17",                       ImGuiKey_F17                },
        { "F18",                       ImGuiKey_F18                },
        { "F19",                       ImGuiKey_F19                },
        { "F20",                       ImGuiKey_F20                },
        { "F21",                       ImGuiKey_F21                },
        { "F22",                       ImGuiKey_F22                },
        { "F23",                       ImGuiKey_F23                },
        { "F24",                       ImGuiKey_F24                },
        //{ "Fn",                        ImGuiKey_None               },           // No direct mapping
        //{ "FnLock",                    ImGuiKey_None               },           // No direct mapping
        { "PrintScreen",               ImGuiKey_PrintScreen        },
        { "ScrollLock",                ImGuiKey_ScrollLock         },
        { "Pause",                     ImGuiKey_Pause              },

        // clipboard/editing keys without direct mapping
        //{ "Abort",                     ImGuiKey_None               },
        //{ "Again",                     ImGuiKey_None               },
        //{ "Convert",                   ImGuiKey_None               },
        //{ "Copy",                      ImGuiKey_None               },
        //{ "Cut",                       ImGuiKey_None               },
        //{ "Find",                      ImGuiKey_None               },
        //{ "Open",                      ImGuiKey_None               },
        //{ "Paste",                     ImGuiKey_None               },
        //{ "Props",                     ImGuiKey_None               },
        //{ "Resume",                    ImGuiKey_None               },
        //{ "Select",                    ImGuiKey_None               },
        //{ "Undo",                      ImGuiKey_None               },

        // IME and international keys without direct mapping
        //{ "Hiragana",                  ImGuiKey_None               },
        //{ "KanaMode",                  ImGuiKey_None               },
        //{ "Katakana",                  ImGuiKey_None               },
        //{ "Lang1",                     ImGuiKey_None               },
        //{ "Lang2",                     ImGuiKey_None               },
        //{ "NonConvert",                ImGuiKey_None               },

        // media and launcher keys without direct mapping
        //{ "AudioVolumeDown",           ImGuiKey_None               },
        //{ "AudioVolumeMute",           ImGuiKey_None               },
        //{ "AudioVolumeUp",             ImGuiKey_None               },
        //{ "LaunchApp1",                ImGuiKey_None               },
        //{ "LaunchApp2",                ImGuiKey_None               },
        //{ "LaunchMail",                ImGuiKey_None               },
        //{ "MediaPlayPause",            ImGuiKey_None               },
        //{ "MediaSelect",               ImGuiKey_None               },
        //{ "MediaStop",                 ImGuiKey_None               },
        //{ "MediaTrackNext",            ImGuiKey_None               },
        //{ "MediaTrackPrevious",        ImGuiKey_None               },

        // system keys without direct mapping
        //{ "Eject",                     ImGuiKey_None               },
        //{ "Hyper",                     ImGuiKey_None               },
        //{ "Power",                     ImGuiKey_None               },
        //{ "Sleep",                     ImGuiKey_None               },
        //{ "Super",                     ImGuiKey_None               },
        //{ "Suspend",                   ImGuiKey_None               },
        //{ "Turbo",                     ImGuiKey_None               },
        //{ "Unidentified",              ImGuiKey_None               },
        //{ "WakeUp",                    ImGuiKey_None               },
    };

    static ImGuiStorage storage;
    static bool is_initialized = false;
    if (is_initialized) return storage;
    is_initialized = true;
    storage.Data.reserve(IM_ARRAYSIZE(key_translations));
    for (int n = 0; n != IM_ARRAYSIZE(key_translations); ++n) {
        storage.Data.push_back(ImGuiStoragePair(ImHashStr(key_translations[n].EmscriptenKey), key_translations[n].Key));
    }
    storage.BuildSortByKey();
    return storage;
}

static ImGuiKey ImGui_ImplEmscripten_TranslateKey(char const* emscripten_key)
{
    // Translate a W3C KeyboardEvent.code string into an ImGuiKey.
    if (emscripten_key == nullptr || emscripten_key[0] == '\0') return ImGuiKey_None;

    ImGuiStorage const& storage = ImGui_ImplEmscripten_GetKeyTranslationStorage();
    return (ImGuiKey)storage.GetInt(ImHashStr(emscripten_key), ImGuiKey_None);
}

#endif // IMGUI_DISABLE
