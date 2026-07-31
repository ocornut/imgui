// dear imgui: Platform Backend for Emscripten HTML5
//
// See documentation in imgui_impl_emscripten.h.
//
// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
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

float ImGui_ImplEmscripten_TargetDevicePixelRatio{1.0f};

namespace {

// W3C UI Events KeyboardEvent.code translation helpers

ImGuiKey translate_key(char const* emscripten_key);
constexpr ImGuiMouseButton translate_mousebutton(unsigned short emscripten_button) __attribute__((__const__));

} // anonymous namespace

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

void set(cursor new_cursor);                                                    // set a new cursor from a cursor enum
char* get_string();                                                             // read the current cursor setting as an owned string, caller must free()
void set(char const* new_cursor);                                               // set the cursor from an arbitrary string

} // namespace emscripten_browser_cursor_internal

namespace {

struct ImGui_ImplEmscripten_Data
{
    float CssToImGuiScale{1.0f};
    emscripten_browser_cursor_internal::cursor CurrentCursor{emscripten_browser_cursor_internal::cursor::invalid};
    char* CursorToRestore{nullptr};
    bool LastMouseDrawCursor{false};
    bool LastNoMouseCursorChange{false};
};

float get_target_device_pixel_ratio()
{
    IM_ASSERT(ImGui_ImplEmscripten_TargetDevicePixelRatio > 0.0f && "ImGui_ImplEmscripten_TargetDevicePixelRatio must be positive.");
    return ImGui_ImplEmscripten_TargetDevicePixelRatio > 0.0f ? ImGui_ImplEmscripten_TargetDevicePixelRatio : 1.0f;
}

void update_display_properties(ImGuiIO& io, ImGui_ImplEmscripten_Data* bd, float css_width, float css_height)
{
    float const target_device_pixel_ratio{get_target_device_pixel_ratio()};
    float const css_to_imgui_scale{static_cast<float>(emscripten_get_device_pixel_ratio()) / target_device_pixel_ratio};
    bd->CssToImGuiScale = css_to_imgui_scale;
    io.DisplaySize.x = css_width * css_to_imgui_scale;
    io.DisplaySize.y = css_height * css_to_imgui_scale;
    io.DisplayFramebufferScale = ImVec2(target_device_pixel_ratio, target_device_pixel_ratio);
}

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
ImGui_ImplEmscripten_Data* ImGui_ImplEmscripten_GetBackendData()
{
    return ImGui::GetCurrentContext() ? static_cast<ImGui_ImplEmscripten_Data*>(ImGui::GetIO().BackendPlatformUserData) : nullptr;
}

} // anonymous namespace

void ImGui_ImplEmscripten_Init()
{
    // Initialise the Emscripten backend, setting input callbacks
    ImGuiIO& io{ImGui::GetIO()};
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");
    ImGui_ImplEmscripten_Data* bd{IM_NEW(ImGui_ImplEmscripten_Data)()};
    io.BackendPlatformUserData = static_cast<void*>(bd);
    io.BackendPlatformName = "imgui_impl_emscripten";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

    // set up initial display size values
    update_display_properties(
        io,
        bd,
        static_cast<float>(EM_ASM_INT(return window.innerWidth;)),
        static_cast<float>(EM_ASM_INT(return window.innerHeight;))
    );

    emscripten_set_mousemove_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenMouseEvent const* mouse_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_MOUSEMOVE
            ImGui_ImplEmscripten_Data* bd{ImGui_ImplEmscripten_GetBackendData()};
            float const css_to_imgui_scale{bd ? bd->CssToImGuiScale : 1.0f};
            ImGui::GetIO().AddMousePosEvent(
                static_cast<float>(mouse_event->clientX) * css_to_imgui_scale,
                static_cast<float>(mouse_event->clientY) * css_to_imgui_scale
            );
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_mousedown_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenMouseEvent const* mouse_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_MOUSEDOWN
            ImGui::GetIO().AddMouseButtonEvent(translate_mousebutton(mouse_event->button), true); // translated button, down
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_mouseup_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenMouseEvent const* mouse_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_MOUSEUP
            ImGui::GetIO().AddMouseButtonEvent(translate_mousebutton(mouse_event->button), false); // translated button, up
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_mouseenter_callback(
        EMSCRIPTEN_EVENT_TARGET_DOCUMENT,                                       // target - WINDOW doesn't produce mouseenter events
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenMouseEvent const* mouse_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_MOUSEENTER
            ImGui_ImplEmscripten_Data* bd{ImGui_ImplEmscripten_GetBackendData()};
            float const css_to_imgui_scale{bd ? bd->CssToImGuiScale : 1.0f};
            ImGui::GetIO().AddMousePosEvent(
                static_cast<float>(mouse_event->clientX) * css_to_imgui_scale,
                static_cast<float>(mouse_event->clientY) * css_to_imgui_scale
            );
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_mouseleave_callback(
        EMSCRIPTEN_EVENT_TARGET_DOCUMENT,                                       // target - WINDOW doesn't produce mouseenter events
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenMouseEvent const* /*mouse_event*/, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_MOUSELEAVE
            ImGuiIO& io{ImGui::GetIO()};
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
            float scale{1.0f};
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
            ImGuiIO& io{ImGui::GetIO()};
            io.AddMouseWheelEvent(
                -static_cast<float>(wheel_event->deltaX) * scale,
                -static_cast<float>(wheel_event->deltaY) * scale
            );
            return io.WantCaptureMouse;                                         // consume the event when imgui wants to capture mouse input
        }
    );
    emscripten_set_keydown_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenKeyboardEvent const* key_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_KEYDOWN
            const ImGuiKey key{translate_key(key_event->code)};
            ImGuiIO& io{ImGui::GetIO()};
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
            const ImGuiKey key{translate_key(key_event->code)};
            ImGuiIO& io{ImGui::GetIO()};
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
            ImGuiIO& io{ImGui::GetIO()};
            io.AddInputCharactersUTF8(key_event->key);
            return io.WantCaptureKeyboard;                                      // the event was consumed only if imgui wants to capture the keyboard
        }
    );
    emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenUiEvent const* event, void* /*data*/) { // event_type == EMSCRIPTEN_EVENT_RESIZE
            ImGuiIO& io{ImGui::GetIO()};
            ImGui_ImplEmscripten_Data* bd{ImGui_ImplEmscripten_GetBackendData()};
            if (bd != nullptr) update_display_properties(io, bd, static_cast<float>(event->windowInnerWidth), static_cast<float>(event->windowInnerHeight));
            return true;                                                        // the event was consumed
        }
    );
    emscripten_set_blur_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenFocusEvent const* /*event*/, void* /*data*/) { // event_type == EMSCRIPTEN_EVENT_BLUR
            ImGuiIO& io{ImGui::GetIO()};
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
            ImGuiIO& io{ImGui::GetIO()};
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
            ImGuiIO& io{ImGui::GetIO()};
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
            ImGuiIO& io{ImGui::GetIO()};
            io.AddFocusEvent(false);
            io.ClearInputKeys();                                                // clear pending input keys on focus loss - for example if you press tab to cycle to another part of the UI
            return true;                                                        // the event was consumed
        }
    );

    // TODO: touch events
}

void ImGui_ImplEmscripten_Shutdown()
{
    ImGui_ImplEmscripten_Data* bd{ImGui_ImplEmscripten_GetBackendData()};
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
        emscripten_browser_cursor_internal::set(bd->CursorToRestore);           // restore the previous cursor state if imgui still owns the cursor on shutdown
        free(bd->CursorToRestore);
    }

    ImGuiIO& io{ImGui::GetIO()};
    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_HasMouseCursors;
    IM_DELETE(bd);
}

namespace {

void restore_cursor_if_necessary(ImGui_ImplEmscripten_Data* bd)
{
    if (bd->CursorToRestore == nullptr) return;
    emscripten_browser_cursor_internal::set(bd->CursorToRestore);               // restore the previous cursor state when leaving imgui cursor ownership
    free(bd->CursorToRestore);
    bd->CursorToRestore = nullptr;
    bd->CurrentCursor = emscripten_browser_cursor_internal::cursor::invalid;    // select an unused value for current cursor to force a set next time
}

void set_cursor_if_necessary(ImGui_ImplEmscripten_Data* bd, emscripten_browser_cursor_internal::cursor new_cursor)
{
    if (new_cursor == bd->CurrentCursor) return;                                // don't do anything if the current cursor is already set
    if (bd->CursorToRestore == nullptr) bd->CursorToRestore = emscripten_browser_cursor_internal::get_string(); // back up the existing cursor state when first taking cursor ownership
    bd->CurrentCursor = new_cursor;
    emscripten_browser_cursor_internal::set(new_cursor);
}

void update_cursor(ImGui_ImplEmscripten_Data* bd)
{
    // Sync any cursor changes due to ImGui to the browser's cursor
    ImGuiIO& io{ImGui::GetIO()};
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

        set_cursor_if_necessary(bd, emscripten_browser_cursor_internal::cursor::none); // hide the cursor for the entire window if imgui is handling cursor drawing - not just when imgui wants to capture the mouse
        bd->LastMouseDrawCursor = true;
    }
    else
    {
        if (bd->LastMouseDrawCursor)
        {
            restore_cursor_if_necessary(bd);
            bd->LastMouseDrawCursor = false;
        }
    }

    if (io.WantCaptureMouse)                                                    // mouse is hovering over the gui
    {
        switch (ImGui::GetMouseCursor())
        {
        case ImGuiMouseCursor_None:
            set_cursor_if_necessary(bd, emscripten_browser_cursor_internal::cursor::none);
            break;
        case ImGuiMouseCursor_Arrow:
            set_cursor_if_necessary(bd, emscripten_browser_cursor_internal::cursor::cursor_default);
            break;
        case ImGuiMouseCursor_TextInput:                                        // When hovering over InputText, etc.
            set_cursor_if_necessary(bd, emscripten_browser_cursor_internal::cursor::text);
            break;
        case ImGuiMouseCursor_ResizeAll:                                        // (Unused by Dear ImGui functions)
            set_cursor_if_necessary(bd, emscripten_browser_cursor_internal::cursor::move);
            break;
        case ImGuiMouseCursor_ResizeNS:                                         // When hovering over a horizontal border
            set_cursor_if_necessary(bd, emscripten_browser_cursor_internal::cursor::ns_resize);
            break;
        case ImGuiMouseCursor_ResizeEW:                                         // When hovering over a vertical border or a column
            set_cursor_if_necessary(bd, emscripten_browser_cursor_internal::cursor::ew_resize);
            break;
        case ImGuiMouseCursor_ResizeNESW:                                       // When hovering over the bottom-left corner of a window
            set_cursor_if_necessary(bd, emscripten_browser_cursor_internal::cursor::nesw_resize);
            break;
        case ImGuiMouseCursor_ResizeNWSE:                                       // When hovering over the bottom-right corner of a window
            set_cursor_if_necessary(bd, emscripten_browser_cursor_internal::cursor::nwse_resize);
            break;
        case ImGuiMouseCursor_Hand:                                             // (Unused by Dear ImGui functions. Use for e.g. hyperlinks)
            set_cursor_if_necessary(bd, emscripten_browser_cursor_internal::cursor::pointer);
            break;
        case ImGuiMouseCursor_NotAllowed:                                       // When hovering something with disallowed interaction. Usually a crossed circle.
            set_cursor_if_necessary(bd, emscripten_browser_cursor_internal::cursor::not_allowed);
            break;
        }
    }
    else                                                                        // mouse is away from the gui, hovering over some other part of the viewport
    {
        restore_cursor_if_necessary(bd);
    }
}

} // anonymous namespace

void ImGui_ImplEmscripten_NewFrame()
{
    ImGui_ImplEmscripten_Data* bd{ImGui_ImplEmscripten_GetBackendData()};
    IM_ASSERT(bd != nullptr && "Context or backend not initialized? Did you call ImGui_ImplEmscripten_Init()?");

    // Update any state that needs to be polled
    update_cursor(bd);
}

namespace emscripten_browser_cursor_internal
{

char* get_string()
{
    // Return the current cursor setting as a newly-allocated string, caller must free it.
    return reinterpret_cast<char*>(EM_ASM_PTR(
        return stringToNewUTF8(document.body.style.cursor);
    ));
}

void set(cursor new_cursor)
{
    // Set the cursor according to the given enum
    // Note, implementations omitted for cursors not used by imgui.  For full implementation, use https://github.com/Armchair-Software/emscripten-browser-cursor
    switch (new_cursor)
    {
    case cursor::none:
        EM_ASM(document.body.style.cursor = 'none';);
        break;
    case cursor::cursor_default:
    default:
        EM_ASM(document.body.style.cursor = 'default';);
        break;
    case cursor::pointer:
        EM_ASM(document.body.style.cursor = 'pointer';);
        break;
    case cursor::text:
        EM_ASM(document.body.style.cursor = 'text';);
        break;
    case cursor::move:
        EM_ASM(document.body.style.cursor = 'move';);
        break;
    case cursor::not_allowed:
        EM_ASM(document.body.style.cursor = 'not-allowed';);
        break;
    case cursor::ew_resize:
        EM_ASM(document.body.style.cursor = 'ew-resize';);
        break;
    case cursor::ns_resize:
        EM_ASM(document.body.style.cursor = 'ns-resize';);
        break;
    case cursor::nesw_resize:
        EM_ASM(document.body.style.cursor = 'nesw-resize';);
        break;
    case cursor::nwse_resize:
        EM_ASM(document.body.style.cursor = 'nwse-resize';);
        break;
    }
}

void set(char const* new_cursor)
{
    // Set the cursor from an arbitrary string
    EM_ASM({
        document.body.style.cursor = UTF8ToString($0);
    }, new_cursor);
}

} // namespace emscripten_browser_cursor_internal

namespace {

constexpr ImGuiMouseButton translate_mousebutton(unsigned short emscripten_button)
{
    // Translate an emscripten-provided integer describing a mouse button to an imgui mouse button
    if (emscripten_button == 1) return ImGuiMouseButton_Middle;                 // 1 = middle mouse button
    if (emscripten_button == 2) return ImGuiMouseButton_Right;                  // 2 = right mouse button
    if (emscripten_button >= ImGuiMouseButton_COUNT) return ImGuiMouseButton_Middle; // treat any weird clicks on unexpected buttons (button 6 upwards) as middle mouse
    return emscripten_button;                                                   // any other button translates 1:1
}

ImGuiStorage const& get_key_translation_storage()
{
    static ImGuiStorage storage;
    static bool is_initialized{false};
    if (is_initialized) return storage;
    is_initialized = true;
    storage.Data.reserve(128);

    // main character keys
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Backquote"),          ImGuiKey_GraveAccent));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Backslash"),          ImGuiKey_Backslash));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("BracketLeft"),        ImGuiKey_LeftBracket));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("BracketRight"),       ImGuiKey_RightBracket));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Comma"),              ImGuiKey_Comma));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Digit0"),             ImGuiKey_0));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Digit1"),             ImGuiKey_1));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Digit2"),             ImGuiKey_2));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Digit3"),             ImGuiKey_3));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Digit4"),             ImGuiKey_4));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Digit5"),             ImGuiKey_5));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Digit6"),             ImGuiKey_6));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Digit7"),             ImGuiKey_7));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Digit8"),             ImGuiKey_8));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Digit9"),             ImGuiKey_9));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Equal"),              ImGuiKey_Equal));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("IntlBackslash"),      ImGuiKey_Backslash)); // Mapping to generic backslash
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("IntlRo"),             ImGuiKey_Slash)); // Closest match for non-standard layouts
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("IntlYen"),            ImGuiKey_Backslash)); // Closest match for non-standard layouts
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyA"),               ImGuiKey_A));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyB"),               ImGuiKey_B));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyC"),               ImGuiKey_C));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyD"),               ImGuiKey_D));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyE"),               ImGuiKey_E));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyF"),               ImGuiKey_F));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyG"),               ImGuiKey_G));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyH"),               ImGuiKey_H));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyI"),               ImGuiKey_I));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyJ"),               ImGuiKey_J));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyK"),               ImGuiKey_K));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyL"),               ImGuiKey_L));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyM"),               ImGuiKey_M));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyN"),               ImGuiKey_N));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyO"),               ImGuiKey_O));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyP"),               ImGuiKey_P));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyQ"),               ImGuiKey_Q));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyR"),               ImGuiKey_R));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyS"),               ImGuiKey_S));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyT"),               ImGuiKey_T));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyU"),               ImGuiKey_U));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyV"),               ImGuiKey_V));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyW"),               ImGuiKey_W));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyX"),               ImGuiKey_X));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyY"),               ImGuiKey_Y));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("KeyZ"),               ImGuiKey_Z));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Minus"),              ImGuiKey_Minus));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Period"),             ImGuiKey_Period));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Quote"),              ImGuiKey_Apostrophe));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Semicolon"),          ImGuiKey_Semicolon));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Slash"),              ImGuiKey_Slash));

    // control keys
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("AltLeft"),            ImGuiKey_LeftAlt));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("AltRight"),           ImGuiKey_RightAlt));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Backspace"),          ImGuiKey_Backspace));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("CapsLock"),           ImGuiKey_CapsLock));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("ContextMenu"),        ImGuiKey_Menu));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("ControlLeft"),        ImGuiKey_LeftCtrl));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("ControlRight"),       ImGuiKey_RightCtrl));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Enter"),              ImGuiKey_Enter));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("MetaLeft"),           ImGuiKey_LeftSuper));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("MetaRight"),          ImGuiKey_RightSuper));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("ShiftLeft"),          ImGuiKey_LeftShift));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("ShiftRight"),         ImGuiKey_RightShift));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Space"),              ImGuiKey_Space));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Tab"),                ImGuiKey_Tab));

    // navigation key group
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Delete"),             ImGuiKey_Delete));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("End"),                ImGuiKey_End));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Help"),               ImGuiKey_PrintScreen)); // Best approximation
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Home"),               ImGuiKey_Home));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Insert"),             ImGuiKey_Insert));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("PageDown"),           ImGuiKey_PageDown));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("PageUp"),             ImGuiKey_PageUp));

    // arrow key group
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("ArrowDown"),          ImGuiKey_DownArrow));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("ArrowLeft"),          ImGuiKey_LeftArrow));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("ArrowRight"),         ImGuiKey_RightArrow));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("ArrowUp"),            ImGuiKey_UpArrow));

    // browser key group
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("BrowserBack"),        ImGuiKey_AppBack)); // Pass through so the embedding app can decide
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("BrowserFavorites"),   ImGuiKey_None)); // No direct mapping
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("BrowserForward"),     ImGuiKey_AppForward)); // Pass through so the embedding app can decide
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("BrowserHome"),        ImGuiKey_None)); // No direct mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("BrowserRefresh"),     ImGuiKey_None)); // No direct mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("BrowserSearch"),      ImGuiKey_None)); // No direct mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("BrowserStop"),        ImGuiKey_None)); // No direct mapping

    // number pad group
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumLock"),            ImGuiKey_NumLock));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Numpad0"),            ImGuiKey_Keypad0));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Numpad1"),            ImGuiKey_Keypad1));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Numpad2"),            ImGuiKey_Keypad2));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Numpad3"),            ImGuiKey_Keypad3));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Numpad4"),            ImGuiKey_Keypad4));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Numpad5"),            ImGuiKey_Keypad5));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Numpad6"),            ImGuiKey_Keypad6));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Numpad7"),            ImGuiKey_Keypad7));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Numpad8"),            ImGuiKey_Keypad8));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Numpad9"),            ImGuiKey_Keypad9));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadAdd"),          ImGuiKey_KeypadAdd));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadBackspace"),    ImGuiKey_Backspace)); // No direct mapping; backspace functionality
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadClear"),        ImGuiKey_None)); // No defined Dear ImGui mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadClearEntry"),   ImGuiKey_None)); // No defined Dear ImGui mapping
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadComma"),        ImGuiKey_KeypadDecimal)); // Closest match
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadDecimal"),      ImGuiKey_KeypadDecimal));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadDivide"),       ImGuiKey_KeypadDivide));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadEnter"),        ImGuiKey_KeypadEnter));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadEqual"),        ImGuiKey_KeypadEqual));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadHash"),         ImGuiKey_Backslash)); // Mapped to generic backslash for telephone-style '#'
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadMemoryAdd"),    ImGuiKey_None)); // No defined mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadMemoryClear"),  ImGuiKey_None)); // No defined mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadMemoryRecall"), ImGuiKey_None)); // No defined mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadMemoryStore"),  ImGuiKey_None)); // No defined mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadMemorySubtract"), ImGuiKey_None)); // No defined mapping
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadMultiply"),     ImGuiKey_KeypadMultiply));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadParenLeft"),    ImGuiKey_LeftBracket)); // Closest available
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadParenRight"),   ImGuiKey_RightBracket)); // Closest available
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadStar"),         ImGuiKey_KeypadMultiply)); // Same as multiply
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("NumpadSubtract"),     ImGuiKey_KeypadSubtract));

    // top row key group
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Escape"),             ImGuiKey_Escape));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F1"),                 ImGuiKey_F1));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F2"),                 ImGuiKey_F2));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F3"),                 ImGuiKey_F3));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F4"),                 ImGuiKey_F4));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F5"),                 ImGuiKey_F5));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F6"),                 ImGuiKey_F6));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F7"),                 ImGuiKey_F7));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F8"),                 ImGuiKey_F8));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F9"),                 ImGuiKey_F9));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F10"),                ImGuiKey_F10));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F11"),                ImGuiKey_F11));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F12"),                ImGuiKey_F12));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F13"),                ImGuiKey_F13));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F14"),                ImGuiKey_F14));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F15"),                ImGuiKey_F15));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F16"),                ImGuiKey_F16));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F17"),                ImGuiKey_F17));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F18"),                ImGuiKey_F18));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F19"),                ImGuiKey_F19));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F20"),                ImGuiKey_F20));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F21"),                ImGuiKey_F21));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F22"),                ImGuiKey_F22));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F23"),                ImGuiKey_F23));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("F24"),                ImGuiKey_F24));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Fn"),                 ImGuiKey_None)); // No direct mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("FnLock"),             ImGuiKey_None)); // No direct mapping
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("PrintScreen"),        ImGuiKey_PrintScreen));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("ScrollLock"),         ImGuiKey_ScrollLock));
    storage.Data.push_back(ImGuiStoragePair(ImHashStr("Pause"),              ImGuiKey_Pause));

    // clipboard/editing keys without direct mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Abort"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Again"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Convert"),            ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Copy"),               ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Cut"),                ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Find"),               ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Open"),               ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Paste"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Props"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Resume"),             ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Select"),             ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Undo"),               ImGuiKey_None));

    // IME and international keys without direct mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Hiragana"),           ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("KanaMode"),           ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Katakana"),           ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Lang1"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Lang2"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("NonConvert"),         ImGuiKey_None));

    // media and launcher keys without direct mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("AudioVolumeDown"),    ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("AudioVolumeMute"),    ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("AudioVolumeUp"),      ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("LaunchApp1"),         ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("LaunchApp2"),         ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("LaunchMail"),         ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("MediaPlayPause"),     ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("MediaSelect"),        ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("MediaStop"),          ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("MediaTrackNext"),     ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("MediaTrackPrevious"), ImGuiKey_None));

    // system keys without direct mapping
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Eject"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Hyper"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Power"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Sleep"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Super"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Suspend"),            ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Turbo"),              ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("Unidentified"),       ImGuiKey_None));
    //storage.Data.push_back(ImGuiStoragePair(ImHashStr("WakeUp"),             ImGuiKey_None));

    storage.BuildSortByKey();
    return storage;
}

ImGuiKey translate_key(char const* emscripten_key)
{
    // Translate a W3C KeyboardEvent.code string into an ImGuiKey.
    if (emscripten_key == nullptr || emscripten_key[0] == '\0') return ImGuiKey_None;

    ImGuiStorage const& storage{get_key_translation_storage()};
    return static_cast<ImGuiKey>(storage.GetInt(ImHashStr(emscripten_key), ImGuiKey_None));
}

} // anonymous namespace

#endif // IMGUI_DISABLE
