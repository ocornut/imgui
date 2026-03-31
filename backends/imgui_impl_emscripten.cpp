// dear imgui: Platform Backend for Emscripten HTML5
//
// See documentation in imgui_impl_emscripten.h.
//
// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2026-03-31: Emscripten: Reworked KeyboardEvent.code translation to avoid std::unordered_map and added BrowserBack/Forward and F13-F24 mappings.
//  2026-03-31: Emscripten: Moved cursor state into backend userdata and replaced cursor restore storage with owned C strings.
//  2024-12-09: Inputs: Added special handling for modifier keys to also generate modifier key events.
//  2024-12-08: Inputs: Prevent "Delete" key from getting printed in text input.
//  2024-12-06: Inputs: Added special handling for Tab and Enter event capture.
//  2024-12-06: Inputs: Handle blur and focus events correctly, focusin and focusout aren't enough.
//  2024-12-06: Emscripten: Don't use devicePixelRatio as it's not needed with WebGPU.
//  2024-11-22: Initial version by Eugene Hopkinson. (#8178)

#include "imgui.h"
#ifndef IMGUI_DISABLE

#include "imgui_impl_emscripten.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>
#include <string.h>

namespace {

// W3C UI Events KeyboardEvent.code translation helpers

ImGuiKey translate_numpad_key(char const* emscripten_key, size_t key_len) __attribute__((__pure__));
ImGuiKey translate_key(char const* emscripten_key) __attribute__((__pure__));
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
    emscripten_browser_cursor_internal::cursor CurrentCursor;
    char* CursorToRestore;

    ImGui_ImplEmscripten_Data()
        : CurrentCursor{emscripten_browser_cursor_internal::cursor::invalid}
        , CursorToRestore{nullptr}
    {
    }
};

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
    io.DisplaySize.x = emscripten::val::global("window")["innerWidth"].as<float>();
    io.DisplaySize.y = emscripten::val::global("window")["innerHeight"].as<float>();

    emscripten_set_mousemove_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,                                         // target
        nullptr,                                                                // userData
        false,                                                                  // useCapture
        [](int /*event_type*/, EmscriptenMouseEvent const* mouse_event, void* /*data*/) { // callback, event_type == EMSCRIPTEN_EVENT_MOUSEMOVE
            ImGui::GetIO().AddMousePosEvent(
                static_cast<float>(mouse_event->clientX),
                static_cast<float>(mouse_event->clientY)
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
            ImGui::GetIO().AddMousePosEvent(
                static_cast<float>(mouse_event->clientX),
                static_cast<float>(mouse_event->clientY)
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
            ImGui::GetIO().AddMouseWheelEvent(
                -static_cast<float>(wheel_event->deltaX) * scale,
                -static_cast<float>(wheel_event->deltaY) * scale
            );
            return false;                                                       // the event was not consumed
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
            io.DisplaySize.x = static_cast<float>(event->windowInnerWidth);
            io.DisplaySize.y = static_cast<float>(event->windowInnerHeight);
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

void set_cursor_if_necessary(emscripten_browser_cursor_internal::cursor& current_cursor, emscripten_browser_cursor_internal::cursor new_cursor)
{
    if (new_cursor == current_cursor) return;                                   // don't do anything if the current cursor is already set
    current_cursor = new_cursor;
    emscripten_browser_cursor_internal::set(new_cursor);
}

void update_cursor(ImGui_ImplEmscripten_Data* bd)
{
    // Sync any cursor changes due to ImGui to the browser's cursor
    if (ImGui::GetIO().WantCaptureMouse)                                        // mouse is hovering over the gui
    {
        if (bd->CursorToRestore == nullptr)
        {
            bd->CursorToRestore = emscripten_browser_cursor_internal::get_string(); // back up the existing cursor state when entering the imgui capture space
        }

        switch (ImGui::GetMouseCursor())
        {
        case ImGuiMouseCursor_None:
            set_cursor_if_necessary(bd->CurrentCursor, emscripten_browser_cursor_internal::cursor::none);
            break;
        case ImGuiMouseCursor_Arrow:
            set_cursor_if_necessary(bd->CurrentCursor, emscripten_browser_cursor_internal::cursor::cursor_default);
            break;
        case ImGuiMouseCursor_TextInput:                                        // When hovering over InputText, etc.
            set_cursor_if_necessary(bd->CurrentCursor, emscripten_browser_cursor_internal::cursor::text);
            break;
        case ImGuiMouseCursor_ResizeAll:                                        // (Unused by Dear ImGui functions)
            set_cursor_if_necessary(bd->CurrentCursor, emscripten_browser_cursor_internal::cursor::move);
            break;
        case ImGuiMouseCursor_ResizeNS:                                         // When hovering over a horizontal border
            set_cursor_if_necessary(bd->CurrentCursor, emscripten_browser_cursor_internal::cursor::ns_resize);
            break;
        case ImGuiMouseCursor_ResizeEW:                                         // When hovering over a vertical border or a column
            set_cursor_if_necessary(bd->CurrentCursor, emscripten_browser_cursor_internal::cursor::ew_resize);
            break;
        case ImGuiMouseCursor_ResizeNESW:                                       // When hovering over the bottom-left corner of a window
            set_cursor_if_necessary(bd->CurrentCursor, emscripten_browser_cursor_internal::cursor::nesw_resize);
            break;
        case ImGuiMouseCursor_ResizeNWSE:                                       // When hovering over the bottom-right corner of a window
            set_cursor_if_necessary(bd->CurrentCursor, emscripten_browser_cursor_internal::cursor::nwse_resize);
            break;
        case ImGuiMouseCursor_Hand:                                             // (Unused by Dear ImGui functions. Use for e.g. hyperlinks)
            set_cursor_if_necessary(bd->CurrentCursor, emscripten_browser_cursor_internal::cursor::pointer);
            break;
        case ImGuiMouseCursor_NotAllowed:                                       // When hovering something with disallowed interaction. Usually a crossed circle.
            set_cursor_if_necessary(bd->CurrentCursor, emscripten_browser_cursor_internal::cursor::not_allowed);
            break;
        }
    }
    else                                                                        // mouse is away from the gui, hovering over some other part of the viewport
    {
        if (bd->CursorToRestore != nullptr)
        {
            emscripten_browser_cursor_internal::set(bd->CursorToRestore);       // restore the previous cursor state when leaving the imgui capture space
            free(bd->CursorToRestore);
            bd->CursorToRestore = nullptr;
            bd->CurrentCursor = emscripten_browser_cursor_internal::cursor::invalid; // select an unused value for current cursor to force a set next time set_cursor_if_necessary is called
        }
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

IM_STATIC_ASSERT(static_cast<int>(ImGuiKey_Z) == static_cast<int>(ImGuiKey_A) + 25);
IM_STATIC_ASSERT(static_cast<int>(ImGuiKey_9) == static_cast<int>(ImGuiKey_0) + 9);
IM_STATIC_ASSERT(static_cast<int>(ImGuiKey_F12) == static_cast<int>(ImGuiKey_F1) + 11);
IM_STATIC_ASSERT(static_cast<int>(ImGuiKey_F24) == static_cast<int>(ImGuiKey_F1) + 23);
IM_STATIC_ASSERT(static_cast<int>(ImGuiKey_Keypad9) == static_cast<int>(ImGuiKey_Keypad0) + 9);

constexpr ImGuiMouseButton translate_mousebutton(unsigned short emscripten_button)
{
    // Translate an emscripten-provided integer describing a mouse button to an imgui mouse button
    if (emscripten_button == 1) return ImGuiMouseButton_Middle;                 // 1 = middle mouse button
    if (emscripten_button == 2) return ImGuiMouseButton_Right;                  // 2 = right mouse button
    if (emscripten_button >= ImGuiMouseButton_COUNT) return ImGuiMouseButton_Middle; // treat any weird clicks on unexpected buttons (button 6 upwards) as middle mouse
    return emscripten_button;                                                   // any other button translates 1:1
}

ImGuiKey translate_numpad_key(char const* emscripten_key, size_t key_len)
{
    // W3C UI Events, "Numpad section" code values.
    // These optional W3C numpad codes currently have no ImGuiKey equivalent:
    // "NumpadClear", "NumpadClearEntry", "NumpadMemoryAdd", "NumpadMemoryClear",
    // "NumpadMemoryRecall", "NumpadMemoryStore", "NumpadMemorySubtract"
    switch (emscripten_key[6])
    {
    case 'A': return ImGuiKey_KeypadAdd;                                        // "NumpadAdd"
    case 'B': return ImGuiKey_Backspace;                                        // "NumpadBackspace": closest editing equivalent
    case 'C':
        switch (key_len)
        {
        case 11:
            if (emscripten_key[7] == 'o') return ImGuiKey_KeypadDecimal;        // "NumpadComma": Dear ImGui only exposes KeypadDecimal
            // if (emscripten_key[7] == 'l') return ImGuiKey_None;                // "NumpadClear"
            break;
        // case 16: return ImGuiKey_None;                                         // "NumpadClearEntry"
        }
        break;
    case 'D':
        switch (key_len)
        {
        case 12: return ImGuiKey_KeypadDivide;                                  // "NumpadDivide"
        case 13: return ImGuiKey_KeypadDecimal;                                 // "NumpadDecimal"
        }
        break;
    case 'E':
        switch (emscripten_key[7])
        {
        case 'n': return ImGuiKey_KeypadEnter;                                  // "NumpadEnter"
        case 'q': return ImGuiKey_KeypadEqual;                                  // "NumpadEqual"
        }
        break;
    case 'H': return ImGuiKey_Backslash;                                        // "NumpadHash": closest printable equivalent for telephone-style '#'
    case 'M':
        switch (key_len)
        {
        case 14: return ImGuiKey_KeypadMultiply;                                // "NumpadMultiply"
        // case 15: return ImGuiKey_None;                                         // "NumpadMemoryAdd"
        case 17:
            // switch (emscripten_key[12])
            // {
            // case 'C': return ImGuiKey_None;                                    // "NumpadMemoryClear"
            // case 'S': return ImGuiKey_None;                                    // "NumpadMemoryStore"
            // }
            break;
        // case 18: return ImGuiKey_None;                                         // "NumpadMemoryRecall"
        }
        break;
    case 'P':
        switch (key_len)
        {
        case 15: return ImGuiKey_LeftBracket;                                   // "NumpadParenLeft": closest available bracket key
        case 16: return ImGuiKey_RightBracket;                                  // "NumpadParenRight": closest available bracket key
        }
        break;
    case 'S':
        switch (key_len)
        {
        case 10: return ImGuiKey_KeypadMultiply;                                // "NumpadStar": phone/remote '*' behaves like keypad multiply
        case 14: return ImGuiKey_KeypadSubtract;                                // "NumpadSubtract"
        // case 20: return ImGuiKey_None;                                         // "NumpadMemorySubtract"
        }
        break;
    default: return static_cast<ImGuiKey>(ImGuiKey_Keypad0 + (emscripten_key[6] - '0')); // Remaining canonical "Numpad..." codes here are "Numpad0" .. "Numpad9"
    }
    return ImGuiKey_None;
}

ImGuiKey translate_key(char const* emscripten_key)
{
    // Translate a W3C KeyboardEvent.code string into an ImGuiKey.
    // Canonical references: W3C UI Events code tables, with MDN used for browser quirks/aliases.
    if (emscripten_key == nullptr || emscripten_key[0] == '\0') return ImGuiKey_None;

    size_t const key_len{strlen(emscripten_key)};

    switch (emscripten_key[0])
    {
    case 'A':
        switch (key_len)
        {
        // case 5:
        //     switch (emscripten_key[1])
        //     {
        //     case 'b': return ImGuiKey_None;                                    // "Abort"
        //     case 'g': return ImGuiKey_None;                                    // "Again"
        //     }
        //     break;
        case 7:
            switch (emscripten_key[1])
            {
            case 'l': return ImGuiKey_LeftAlt;                                  // "AltLeft"
            case 'r': return ImGuiKey_UpArrow;                                  // "ArrowUp"
            }
            break;
        case 8: return ImGuiKey_RightAlt;                                       // "AltRight"
        case 9:
            switch (emscripten_key[5])
            {
            case 'D': return ImGuiKey_DownArrow;                                // "ArrowDown"
            case 'L': return ImGuiKey_LeftArrow;                                // "ArrowLeft"
            }
            break;
        case 10: return ImGuiKey_RightArrow;                                    // "ArrowRight"
        // case 13: return ImGuiKey_None;                                         // "AudioVolumeUp"
        // case 15:
        //     switch (emscripten_key[11])
        //     {
        //     case 'D': return ImGuiKey_None;                                    // "AudioVolumeDown"
        //     case 'M': return ImGuiKey_None;                                    // "AudioVolumeMute"
        //     }
        //     break;
        }
        break;
    case 'B':
        switch (key_len)
        {
        case 9:
            switch (emscripten_key[4])
            {
            case 'q': return ImGuiKey_GraveAccent;                              // "Backquote"
            case 's':
                if (emscripten_key[5] == 'l') return ImGuiKey_Backslash;        // "Backslash": W3C uses this for both US \| and the UK #~ key tucked under Enter
                if (emscripten_key[5] == 'p') return ImGuiKey_Backspace;        // "Backspace"
                break;
            }
            break;
        case 11:
            if (emscripten_key[3] == 'c') return ImGuiKey_LeftBracket;          // "BracketLeft"
            switch (emscripten_key[7])
            {
            case 'B': return ImGuiKey_AppBack;                                  // "BrowserBack": pass through even in browser hosts so the embedding app can decide
            // case 'H': return ImGuiKey_None;                                    // "BrowserHome"
            // case 'S': return ImGuiKey_None;                                    // "BrowserStop"
            }
            break;
        case 12: return ImGuiKey_RightBracket;                                  // "BracketRight"
        // case 13: return ImGuiKey_None;                                         // "BrowserSearch"
        case 14:
            switch (emscripten_key[7])
            {
            case 'F': return ImGuiKey_AppForward;                               // "BrowserForward": pass through even in browser hosts so the embedding app can decide
            // case 'R': return ImGuiKey_None;                                    // "BrowserRefresh"
            }
            break;
        // case 16: return ImGuiKey_None;                                         // "BrowserFavorites"
        }
        break;
    case 'C':
        switch (key_len)
        {
        // case 3: return ImGuiKey_None;                                          // "Cut"
        // case 4: return ImGuiKey_None;                                          // "Copy"
        case 5: return ImGuiKey_Comma;                                          // "Comma"
        // case 7: return ImGuiKey_None;                                          // "Convert"
        case 8: return ImGuiKey_CapsLock;                                       // "CapsLock"
        case 11:
            switch (emscripten_key[4])
            {
            case 'e': return ImGuiKey_Menu;                                     // "ContextMenu"
            case 'r': return ImGuiKey_LeftCtrl;                                 // "ControlLeft"
            }
            break;
        case 12: return ImGuiKey_RightCtrl;                                     // "ControlRight"
        }
        break;
    case 'D':
        if (key_len == 6)
        {
            if (emscripten_key[1] == 'e') return ImGuiKey_Delete;               // "Delete"
            else return static_cast<ImGuiKey>(ImGuiKey_0 + (emscripten_key[5] - '0')); // "Digit0" .. "Digit9"
        }
        break;
    case 'E':
        switch (key_len)
        {
        case 3: return ImGuiKey_End;                                            // "End"
        case 5:
            switch (emscripten_key[1])
            {
            case 'n': return ImGuiKey_Enter;                                    // "Enter"
            case 'q': return ImGuiKey_Equal;                                    // "Equal"
            // case 'j': return ImGuiKey_None;                                    // "Eject"
            }
            break;
        case 6: return ImGuiKey_Escape;                                         // "Escape"
        }
        break;
    case 'F':
        switch (key_len)
        {
        case 2:
            if (emscripten_key[1] == 'n') return ImGuiKey_None;                 // "Fn": no ImGuiKey equivalent
            else return static_cast<ImGuiKey>(ImGuiKey_F1 + (emscripten_key[1] - '1')); // "F1" .. "F9"
        case 3:
            if (emscripten_key[1] == '1') return static_cast<ImGuiKey>(ImGuiKey_F10 + (emscripten_key[2] - '0')); // "F10" .. "F19"
            else return static_cast<ImGuiKey>(ImGuiKey_F20 + (emscripten_key[2] - '0')); // "F20" .. "F24"
        // case 4: return ImGuiKey_None;                                          // "Find"
        // case 6: return ImGuiKey_None;                                          // "FnLock"
        }
        break;
    case 'H':
        switch (key_len)
        {
        case 4:
            switch (emscripten_key[1])
            {
            case 'o': return ImGuiKey_Home;                                     // "Home"
            // case 'e': return ImGuiKey_None;                                    // "Help"
            }
            break;
        // case 5: return ImGuiKey_None;                                          // "Hyper"
        // case 8: return ImGuiKey_None;                                          // "Hiragana"
        }
        break;
    case 'I':
        switch (key_len)
        {
        case 6:
            switch (emscripten_key[2])
            {
            case 's': return ImGuiKey_Insert;                                   // "Insert"
            case 't': return ImGuiKey_Slash;                                    // "IntlRo": closest printable equivalent
            }
            break;
        case 7: return ImGuiKey_Backslash;                                      // "IntlYen": closest printable equivalent
        case 13: return ImGuiKey_Backslash;                                     // "IntlBackslash": preserve original behavior and map to generic backslash
        }
        break;
    case 'K':
        switch (key_len)
        {
        case 4: return static_cast<ImGuiKey>(ImGuiKey_A + (emscripten_key[3] - 'A')); // "KeyA" .. "KeyZ"
        // case 8:
        //     switch (emscripten_key[2])
        //     {
        //     case 'n': return ImGuiKey_None;                                    // "KanaMode"
        //     default: return ImGuiKey_None;                                     // "Katakana"
        //     }
        }
        break;
    // case 'L':
    //     switch (key_len)
    //     {
    //     case 5:
    //         switch (emscripten_key[4])
    //         {
    //         case '1': return ImGuiKey_None;                                    // "Lang1"
    //         case '2': return ImGuiKey_None;                                    // "Lang2"
    //         }
    //         break;
    //     case 10:
    //         switch (emscripten_key[6])
    //         {
    //         case 'A':
    //             switch (emscripten_key[9])
    //             {
    //             case '1': return ImGuiKey_None;                                // "LaunchApp1"
    //             case '2': return ImGuiKey_None;                                // "LaunchApp2"
    //             }
    //             break;
    //         case 'M': return ImGuiKey_None;                                    // "LaunchMail"
    //         }
    //         break;
    //     }
    case 'M':
        switch (key_len)
        {
        case 5: return ImGuiKey_Minus;                                          // "Minus"
        case 8: return ImGuiKey_LeftSuper;                                      // "MetaLeft"
        case 9:
            switch (emscripten_key[3])
            {
            case 'a': return ImGuiKey_RightSuper;                               // "MetaRight"
            default:
                // return ImGuiKey_None;                                          // "MediaStop"
            }
            break;
        // case 11: return ImGuiKey_None;                                         // "MediaSelect"
        // case 14:
        //     switch (emscripten_key[10])
        //     {
        //     case 'N': return ImGuiKey_None;                                    // "MediaTrackNext"
        //     default: return ImGuiKey_None;                                     // "MediaPlayPause"
        //     }
        // case 18: return ImGuiKey_None;                                         // "MediaTrackPrevious"
        }
        break;
    case 'N':
        if (emscripten_key[3] == 'p')
        {
            ImGuiKey const numpad_key{translate_numpad_key(emscripten_key, key_len)};
            if (numpad_key != ImGuiKey_None) return numpad_key;
        }
        if (key_len == 7) return ImGuiKey_NumLock;                              // "NumLock"
        // if (key_len == 10) return ImGuiKey_None;                               // "NonConvert"
        break;
    // case 'O': return ImGuiKey_None;                                            // "Open"
    case 'P':
        switch (key_len)
        {
        case 5:
            if (emscripten_key[2] == 'u') return ImGuiKey_Pause;                // "Pause"
            // if (emscripten_key[2] == 's') return ImGuiKey_None;                // "Paste"
            // if (emscripten_key[1] == 'o') return ImGuiKey_None;                // "Power"
            // if (emscripten_key[1] == 'r') return ImGuiKey_None;                // "Props"
            break;
        case 6:
            switch (emscripten_key[1])
            {
            case 'a': return ImGuiKey_PageUp;                                   // "PageUp"
            case 'e': return ImGuiKey_Period;                                   // "Period"
            }
            break;
        case 8: return ImGuiKey_PageDown;                                       // "PageDown"
        case 11: return ImGuiKey_PrintScreen;                                   // "PrintScreen"
        }
        break;
    case 'Q': return ImGuiKey_Apostrophe;                                       // "Quote"
    // case 'R': return ImGuiKey_None;                                            // "Resume"
    case 'S':
        switch (key_len)
        {
        case 5:
            if (emscripten_key[2] == 'a') return ImGuiKey_Slash;                // "Slash"
            if (emscripten_key[1] == 'p') return ImGuiKey_Space;                // "Space"
            // if (emscripten_key[2] == 'e') return ImGuiKey_None;                // "Sleep"
            // if (emscripten_key[1] == 'u') return ImGuiKey_None;                // "Super"
            break;
        // case 6: return ImGuiKey_None;                                          // "Select"
        // case 7: return ImGuiKey_None;                                          // "Suspend"
        case 9:
            switch (emscripten_key[1])
            {
            case 'e': return ImGuiKey_Semicolon;                                // "Semicolon"
            case 'h': return ImGuiKey_LeftShift;                                // "ShiftLeft"
            }
            break;
        case 10:
            switch (emscripten_key[1])
            {
            case 'c': return ImGuiKey_ScrollLock;                               // "ScrollLock"
            case 'h': return ImGuiKey_RightShift;                               // "ShiftRight"
            }
            break;
        }
        break;
    case 'T':
        if (key_len == 3) return ImGuiKey_Tab;                                  // "Tab"
        // if (key_len == 5) return ImGuiKey_None;                                // "Turbo"
        break;
    // case 'U':
    //     switch (key_len)
    //     {
    //     case 4: return ImGuiKey_None;                                          // "Undo"
    //     case 12: return ImGuiKey_None;                                         // "Unidentified"
    //     }
    // case 'W': return ImGuiKey_None;                                            // "WakeUp"
    }
    return ImGuiKey_None;
}

} // anonymous namespace

#endif // IMGUI_DISABLE
