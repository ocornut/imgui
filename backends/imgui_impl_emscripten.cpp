// dear imgui: Platform Backend for Emscripten HTML5
//
// See documentation in imgui_impl_emscripten.h.
//
// Copyright 2024 Eugene Hopkinson

#include "imgui.h"
#ifndef IMGUI_DISABLE

#include "imgui_impl_emscripten.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>

namespace {

/// A map of HTML5 key names to imgui keys
static const std::unordered_map<std::string, ImGuiKey> key_translate_lookup{
  // main character keys
  {"Backquote",            ImGuiKey_GraveAccent},
  {"Backslash",            ImGuiKey_Backslash},
  {"BracketLeft",          ImGuiKey_LeftBracket},
  {"BracketRight",         ImGuiKey_RightBracket},
  {"Comma",                ImGuiKey_Comma},
  {"Digit0",               ImGuiKey_0},
  {"Digit1",               ImGuiKey_1},
  {"Digit2",               ImGuiKey_2},
  {"Digit3",               ImGuiKey_3},
  {"Digit4",               ImGuiKey_4},
  {"Digit5",               ImGuiKey_5},
  {"Digit6",               ImGuiKey_6},
  {"Digit7",               ImGuiKey_7},
  {"Digit8",               ImGuiKey_8},
  {"Digit9",               ImGuiKey_9},
  {"Equal",                ImGuiKey_Equal},
  {"IntlBackslash",        ImGuiKey_Backslash},                                 // Mapping to generic backslash
  {"IntlRo",               ImGuiKey_Slash},                                     // Closest match for non-standard layouts
  {"IntlYen",              ImGuiKey_Backslash},                                 // Closest match for non-standard layouts
  {"KeyA",                 ImGuiKey_A},
  {"KeyB",                 ImGuiKey_B},
  {"KeyC",                 ImGuiKey_C},
  {"KeyD",                 ImGuiKey_D},
  {"KeyE",                 ImGuiKey_E},
  {"KeyF",                 ImGuiKey_F},
  {"KeyG",                 ImGuiKey_G},
  {"KeyH",                 ImGuiKey_H},
  {"KeyI",                 ImGuiKey_I},
  {"KeyJ",                 ImGuiKey_J},
  {"KeyK",                 ImGuiKey_K},
  {"KeyL",                 ImGuiKey_L},
  {"KeyM",                 ImGuiKey_M},
  {"KeyN",                 ImGuiKey_N},
  {"KeyO",                 ImGuiKey_O},
  {"KeyP",                 ImGuiKey_P},
  {"KeyQ",                 ImGuiKey_Q},
  {"KeyR",                 ImGuiKey_R},
  {"KeyS",                 ImGuiKey_S},
  {"KeyT",                 ImGuiKey_T},
  {"KeyU",                 ImGuiKey_U},
  {"KeyV",                 ImGuiKey_V},
  {"KeyW",                 ImGuiKey_W},
  {"KeyX",                 ImGuiKey_X},
  {"KeyY",                 ImGuiKey_Y},
  {"KeyZ",                 ImGuiKey_Z},
  {"Minus",                ImGuiKey_Minus},
  {"Period",               ImGuiKey_Period},
  {"Quote",                ImGuiKey_Apostrophe},
  {"Semicolon",            ImGuiKey_Semicolon},
  {"Slash",                ImGuiKey_Slash},

  // control keys
  {"AltLeft",              ImGuiKey_LeftAlt},
  {"AltRight",             ImGuiKey_RightAlt},
  {"Backspace",            ImGuiKey_Backspace},
  {"CapsLock",             ImGuiKey_CapsLock},
  {"ContextMenu",          ImGuiKey_Menu},
  {"ControlLeft",          ImGuiKey_LeftCtrl},
  {"ControlRight",         ImGuiKey_RightCtrl},
  {"Enter",                ImGuiKey_Enter},
  {"MetaLeft",             ImGuiKey_LeftSuper},
  {"MetaRight",            ImGuiKey_RightSuper},
  {"ShiftLeft",            ImGuiKey_LeftShift},
  {"ShiftRight",           ImGuiKey_RightShift},
  {"Space",                ImGuiKey_Space},
  {"Tab",                  ImGuiKey_Tab},

  // navigation key group
  {"Delete",               ImGuiKey_Delete},
  {"End",                  ImGuiKey_End},
  //{"Help",                 ImGuiKey_PrintScreen},                               // Best approximation
  {"Home",                 ImGuiKey_Home},
  {"Insert",               ImGuiKey_Insert},
  {"PageDown",             ImGuiKey_PageDown},
  {"PageUp",               ImGuiKey_PageUp},

  // arrow key group
  {"ArrowDown",            ImGuiKey_DownArrow},
  {"ArrowLeft",            ImGuiKey_LeftArrow},
  {"ArrowRight",           ImGuiKey_RightArrow},
  {"ArrowUp",              ImGuiKey_UpArrow},

  // number pad group
  {"NumLock",              ImGuiKey_NumLock},
  {"Numpad0",              ImGuiKey_Keypad0},
  {"Numpad1",              ImGuiKey_Keypad1},
  {"Numpad2",              ImGuiKey_Keypad2},
  {"Numpad3",              ImGuiKey_Keypad3},
  {"Numpad4",              ImGuiKey_Keypad4},
  {"Numpad5",              ImGuiKey_Keypad5},
  {"Numpad6",              ImGuiKey_Keypad6},
  {"Numpad7",              ImGuiKey_Keypad7},
  {"Numpad8",              ImGuiKey_Keypad8},
  {"Numpad9",              ImGuiKey_Keypad9},
  {"NumpadAdd",            ImGuiKey_KeypadAdd},
  {"NumpadBackspace",      ImGuiKey_Backspace},                                 // No direct mapping; backspace functionality
  //{"NumpadClear",          ImGuiKey_KeypadClear},                               // Custom-defined if needed
  //{"NumpadClearEntry",     ImGuiKey_KeypadClear},                               // Custom-defined if needed
  {"NumpadComma",          ImGuiKey_KeypadDecimal},                             // Closest match
  {"NumpadDecimal",        ImGuiKey_KeypadDecimal},
  {"NumpadDivide",         ImGuiKey_KeypadDivide},
  {"NumpadEnter",          ImGuiKey_KeypadEnter},
  {"NumpadEqual",          ImGuiKey_KeypadEqual},
  {"NumpadHash",           ImGuiKey_Backslash},                                 // Mapped to # on UK keyboard
  //{"NumpadMemoryAdd",      ImGuiKey_None},                                      // No defined mapping
  //{"NumpadMemoryClear",    ImGuiKey_None},                                      // No defined mapping
  //{"NumpadMemoryRecall",   ImGuiKey_None},                                      // No defined mapping
  //{"NumpadMemoryStore",    ImGuiKey_None},                                      // No defined mapping
  //{"NumpadMemorySubtract", ImGuiKey_None},                                      // No defined mapping
  {"NumpadMultiply",       ImGuiKey_KeypadMultiply},
  {"NumpadParenLeft",      ImGuiKey_LeftBracket},                               // Closest available
  {"NumpadParenRight",     ImGuiKey_RightBracket},                              // Closest available
  {"NumpadStar",           ImGuiKey_KeypadMultiply},                            // Same as multiply
  {"NumpadSubtract",       ImGuiKey_KeypadSubtract},

  // top row key groups
  {"Escape",               ImGuiKey_Escape},
  {"F1",                   ImGuiKey_F1},
  {"F2",                   ImGuiKey_F2},
  {"F3",                   ImGuiKey_F3},
  {"F4",                   ImGuiKey_F4},
  {"F5",                   ImGuiKey_F5},
  {"F6",                   ImGuiKey_F6},
  {"F6",                   ImGuiKey_F6},
  {"F7",                   ImGuiKey_F7},
  {"F8",                   ImGuiKey_F8},
  {"F9",                   ImGuiKey_F9},
  {"F10",                  ImGuiKey_F10},
  {"F11",                  ImGuiKey_F11},
  {"F12",                  ImGuiKey_F12},
  //{"Fn",                   ImGuiKey_None},                                      // No direct mapping
  //{"FnLock",               ImGuiKey_None},                                      // No direct mapping
  {"PrintScreen",          ImGuiKey_PrintScreen},
  {"ScrollLock",           ImGuiKey_ScrollLock},
  {"Pause",                ImGuiKey_Pause},
};

ImGuiKey translate_key(char const* emscripten_key) __attribute__((__const__));
ImGuiKey translate_key(char const* emscripten_key) {
  /// Translate an emscripten-provided browser string describing a keycode to an imgui key code
  if(auto it{key_translate_lookup.find(emscripten_key)}; it != key_translate_lookup.end()) {
    return it->second;
  }
  return ImGuiKey_None;
}

constexpr ImGuiMouseButton translate_mousebutton(unsigned short emscripten_button) __attribute__((__const__));
constexpr ImGuiMouseButton translate_mousebutton(unsigned short emscripten_button) {
  /// Translate an emscripten-provided integer describing a mouse button to an imgui mouse button
  if(emscripten_button == 1) return ImGuiMouseButton_Middle;                    // 1 = middle mouse button
  if(emscripten_button == 2) return ImGuiMouseButton_Right;                     // 2 = right mouse button
  if(emscripten_button > ImGuiMouseButton_COUNT) return ImGuiMouseButton_Middle; // treat any weird clicks on unexpected buttons (button 6 upwards) as middle mouse
  return emscripten_button;                                                     // any other button translates 1:1
}

} // anonymous namespace

namespace emscripten_browser_cursor_internal {

////////////////////////////////// Interface ///////////////////////////////////

enum class cursor {
  // General
  cursor_auto,                                                                  // The UA will determine the cursor to display based on the current context. E.g., equivalent to text when hovering text.
  cursor_default,                                                               // The platform-dependent default cursor. Typically an arrow.
  none,                                                                         // No cursor is rendered.

  // Links & status
  context_menu,                                                                 // cursor slightly obscuring a menu icon - A context menu is available.
  help,                                                                         // cursor next to a question mark - Help information is available.
  pointer,                                                                      // right hand with an index finger pointing up - The cursor is a pointer that indicates a link. Typically an image of a pointing hand.
  progress,                                                                     // cursor and hour glass - The program is busy in the background, but the user can still interact with the interface (in contrast to wait).
  wait,                                                                         // hour glass - The program is busy, and the user can't interact with the interface (in contrast to progress). Sometimes an image of an hourglass or a watch.

  // Selection
  cell,                                                                         // plus symbol - The table cell or set of cells can be selected.
  crosshair,                                                                    // crosshair  - Cross cursor, often used to indicate selection in a bitmap.
  text,                                                                         // vertical i-beam - The text can be selected. Typically the shape of an I-beam.
  vertical_text,                                                                // horizontal i-beam - The vertical text can be selected. Typically the shape of a sideways I-beam.

  // Drag & drop
  alias,                                                                        // cursor next to a folder icon with a curved arrow pointing up and to the right - An alias or shortcut is to be created.
  copy,                                                                         // cursor next to a smaller folder icon with a plus sign - Something is to be copied.
  move,                                                                         // plus sign made of two thin lines, with small arrows facing out - Something is to be moved.
  no_drop,                                                                      // cursor next to circle with a line through it - An item may not be dropped at the current location.
  not_allowed,                                                                  // Circle with a line through it - The requested action will not be carried out.
  grab,                                                                         // fully opened hand - Something can be grabbed (dragged to be moved).
  grabbing,                                                                     // closed hand - Something is being grabbed (dragged to be moved).

  // Resizing & scrolling
  all_scroll,                                                                   // dot with four triangles around it - Something can be scrolled in any direction (panned).
  col_resize,                                                                   // The item/column can be resized horizontally. Often rendered as arrows pointing left and right with a vertical bar separating them.
  row_resize,                                                                   // The item/row can be resized vertically. Often rendered as arrows pointing up and down with a horizontal bar separating them.
  n_resize,                                                                     // arrow pointing up - Some edge is to be moved. For example, the se-resize cursor is used when the movement starts from the south-east corner of the box.
  e_resize,                                                                     // arrow pointing right
  s_resize,                                                                     // arrow pointing down
  w_resize,                                                                     // arrow pointing left
  ne_resize,                                                                    // arrow pointing top-right
  nw_resize,                                                                    // arrow pointing top-left
  se_resize,                                                                    // arrow pointing bottom-right
  sw_resize,                                                                    // arrow pointing bottom-left
  ew_resize,                                                                    // arrow pointing left and right - Bidirectional resize cursor.
  ns_resize,                                                                    // arrow pointing up and down
  nesw_resize,                                                                  // arrow pointing both to the top-right and bottom-left
  nwse_resize,                                                                  // arrow pointing both to the top-left and bottom-right

  // Zooming
  zoom_in,                                                                      // magnifying glass with a plus sign - Something can be zoomed (magnified) in or out.
  zoom_out,

  // Special invalid value
  invalid = std::numeric_limits<int>::max()
};

void set(cursor new_cursor);                                                    // set a new cursor from a cursor enum
void unset();                                                                   // clear the current cursor setting

//////////////////////////////// Implementation ////////////////////////////////

bool is_set() {
  /// Returns whether the cursor is currently set
  return EM_ASM_INT(
    return !(!document.body.style.cursor || document.body.style.cursor.length === 0 );
  );
}

std::string get_string() {
  /// Return the current cursor setting as a string
  auto cursor_str_ptr{reinterpret_cast<char*>(EM_ASM_PTR(
    return stringToNewUTF8(document.body.style.cursor);
  ))};
  std::string const cursor_str{cursor_str_ptr};
  free(cursor_str_ptr);
  return cursor_str;
}

void set(cursor new_cursor) {
  /// Set the cursor according to the given enum
  // Note, implementations omitted for cursors not used by imgui.  For full implementation, use https://github.com/Armchair-Software/emscripten-browser-cursor
  switch(new_cursor) {
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

void set(std::string const &new_cursor) {
  /// Set the cursor from an arbitrary string
  EM_ASM({
    document.body.style.cursor = UTF8ToString($0);
  }, new_cursor.c_str());
}

} // namespace emscripten_browser_cursor

namespace {

void update_cursor() {
  /// Sync any cursor changes due to ImGUI to the browser's cursor
  static emscripten_browser_cursor_internal::cursor current_cursor{emscripten_browser_cursor_internal::cursor::invalid};
  static std::optional<std::string> cursor_to_restore;

  auto set_cursor_if_necessary{[&](emscripten_browser_cursor_internal::cursor new_cursor){
    if(new_cursor == current_cursor) return;                                    // don't do anything if the current cursor is already set
    current_cursor = new_cursor;
    emscripten_browser_cursor_internal::set(new_cursor);
  }};

  if(ImGui::GetIO().WantCaptureMouse) {                                         // mouse is hovering over the gui
    if(!cursor_to_restore && emscripten_browser_cursor_internal::is_set()) {
      cursor_to_restore = emscripten_browser_cursor_internal::get_string();     // back up the existing cursor when entering the imgui capture space
    }

    switch(ImGui::GetMouseCursor()) {
    case ImGuiMouseCursor_Arrow:
      set_cursor_if_necessary(emscripten_browser_cursor_internal::cursor::cursor_default);
      break;
    case ImGuiMouseCursor_TextInput:                                            // When hovering over InputText, etc.
      set_cursor_if_necessary(emscripten_browser_cursor_internal::cursor::text);
      break;
    case ImGuiMouseCursor_ResizeAll:                                            // (Unused by Dear ImGui functions)
      set_cursor_if_necessary(emscripten_browser_cursor_internal::cursor::move);
      break;
    case ImGuiMouseCursor_ResizeNS:                                             // When hovering over a horizontal border
      set_cursor_if_necessary(emscripten_browser_cursor_internal::cursor::ns_resize);
      break;
    case ImGuiMouseCursor_ResizeEW:                                             // When hovering over a vertical border or a column
      set_cursor_if_necessary(emscripten_browser_cursor_internal::cursor::ew_resize);
      break;
    case ImGuiMouseCursor_ResizeNESW:                                           // When hovering over the bottom-left corner of a window
      set_cursor_if_necessary(emscripten_browser_cursor_internal::cursor::nesw_resize);
      break;
    case ImGuiMouseCursor_ResizeNWSE:                                           // When hovering over the bottom-right corner of a window
      set_cursor_if_necessary(emscripten_browser_cursor_internal::cursor::nwse_resize);
      break;
    case ImGuiMouseCursor_Hand:                                                 // (Unused by Dear ImGui functions. Use for e.g. hyperlinks)
      set_cursor_if_necessary(emscripten_browser_cursor_internal::cursor::pointer);
      break;
    case ImGuiMouseCursor_NotAllowed:                                           // When hovering something with disallowed interaction. Usually a crossed circle.
      set_cursor_if_necessary(emscripten_browser_cursor_internal::cursor::not_allowed);
      break;
    }
  } else {                                                                      // mouse is away from the gui, hovering over some other part of the viewport
    if(cursor_to_restore) {
      emscripten_browser_cursor_internal::set(*cursor_to_restore);              // restore the previous cursor when leaving the imgui capture space
      cursor_to_restore = std::nullopt;
      current_cursor = emscripten_browser_cursor_internal::cursor::invalid;     // select an unused value for current cursor to force a set next time set_cursor_if_necessary is called
    }
  }
}

} // anonymous namespace

void ImGui_ImplEmscripten_Init() {
  /// Initialise the Emscripten backend, setting input callbacks
  auto &imgui_io{ImGui::GetIO()};
  imgui_io.BackendPlatformName = "imgui_impl_emscripten";
  imgui_io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

  // set up initial display size values
  imgui_io.DisplaySize.x = emscripten::val::global("window")["innerWidth"].as<float>();
  imgui_io.DisplaySize.y = emscripten::val::global("window")["innerHeight"].as<float>();

  emscripten_set_mousemove_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenMouseEvent const *mouse_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_MOUSEMOVE
      ImGui::GetIO().AddMousePosEvent(
        static_cast<float>(mouse_event->clientX),
        static_cast<float>(mouse_event->clientY)
      );
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_mousedown_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenMouseEvent const *mouse_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_MOUSEDOWN
      ImGui::GetIO().AddMouseButtonEvent(translate_mousebutton(mouse_event->button), true); // translated button, down
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_mouseup_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenMouseEvent const *mouse_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_MOUSEUP
      ImGui::GetIO().AddMouseButtonEvent(translate_mousebutton(mouse_event->button), false); // translated button, up
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_mouseenter_callback(
    EMSCRIPTEN_EVENT_TARGET_DOCUMENT,                                           // target - WINDOW doesn't produce mouseenter events
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenMouseEvent const *mouse_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_MOUSEENTER
      ImGui::GetIO().AddMousePosEvent(
        static_cast<float>(mouse_event->clientX),
        static_cast<float>(mouse_event->clientY)
      );
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_mouseleave_callback(
    EMSCRIPTEN_EVENT_TARGET_DOCUMENT,                                           // target - WINDOW doesn't produce mouseenter events
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenMouseEvent const */*mouse_event*/, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_MOUSELEAVE
      auto &imgui_io{ImGui::GetIO()};
      imgui_io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);                            // cursor is not in the window
      imgui_io.ClearInputKeys();                                                // clear pending input keys on mouse exit
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_wheel_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenWheelEvent const *wheel_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_WHEEL
      float scale{1.0f};
      switch(wheel_event->deltaMode) {
      case DOM_DELTA_PIXEL:                                                     // scrolling in pixels
        scale = 1.0f / 100.0f;
        break;
      case DOM_DELTA_LINE:                                                      // scrolling by lines
        scale = 1.0f / 3.0f;
        break;
      case DOM_DELTA_PAGE:                                                      // scrolling by pages
        scale = 80.0f;
        break;
      }
      // TODO: make scrolling speeds configurable
      ImGui::GetIO().AddMouseWheelEvent(
        -static_cast<float>(wheel_event->deltaX) * scale,
        -static_cast<float>(wheel_event->deltaY) * scale
      );
      return false;                                                             // the event was not consumed
    }
  );
  emscripten_set_keydown_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenKeyboardEvent const *key_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_KEYDOWN
      auto const key{translate_key(key_event->code)};
      auto &imgui_io{ImGui::GetIO()};
      imgui_io.AddKeyEvent(key, true);
      switch(key) {                                                             // special cases for certain key events
      case ImGuiKey_LeftCtrl:                                                   // additional events for modifier keys
      case ImGuiKey_RightCtrl:
        imgui_io.AddKeyEvent(ImGuiMod_Ctrl, true);
        break;
      case ImGuiKey_LeftShift:
      case ImGuiKey_RightShift:
        imgui_io.AddKeyEvent(ImGuiMod_Shift, true);
        break;
      case ImGuiKey_LeftAlt:
      case ImGuiKey_RightAlt:
        imgui_io.AddKeyEvent(ImGuiMod_Alt, true);
        break;
      case ImGuiKey_LeftSuper:
      case ImGuiKey_RightSuper:
        imgui_io.AddKeyEvent(ImGuiMod_Super, true);
        break;
      // TODO: case ImGuiKey_Menu: do we want to do anything with this?
      case ImGuiKey_Tab:                                                        // consuming tab prevents the user tabbing to other parts of the browser interface outside the window content
        return imgui_io.WantCaptureKeyboard;                                    // the event was consumed only if imgui wants to capture the keyboard
      case ImGuiKey_Enter:                                                      // consuming enter prevents the word "Enter" appearing in text input via the keypress callback
      case ImGuiKey_Delete:                                                     // consuming enter prevents the word "Delete" appearing in text input via the keypress callback
        return imgui_io.WantTextInput;                                          // the event was consumed only if we're currently accepting text input
      default:
        break;
      }
      return false;                                                             // if no special handling, the event was not consumed
    }
  );
  emscripten_set_keyup_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenKeyboardEvent const *key_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_KEYUP
      auto const key{translate_key(key_event->code)};
      auto &imgui_io{ImGui::GetIO()};
      imgui_io.AddKeyEvent(key, false);
      switch(key) {                                                             // special cases for certain key events
      case ImGuiKey_LeftCtrl:                                                   // additional events for modifier keys
      case ImGuiKey_RightCtrl:
        imgui_io.AddKeyEvent(ImGuiMod_Ctrl, false);
        break;
      case ImGuiKey_LeftShift:
      case ImGuiKey_RightShift:
        imgui_io.AddKeyEvent(ImGuiMod_Shift, false);
        break;
      case ImGuiKey_LeftAlt:
      case ImGuiKey_RightAlt:
        imgui_io.AddKeyEvent(ImGuiMod_Alt, false);
        break;
      case ImGuiKey_LeftSuper:
      case ImGuiKey_RightSuper:
        imgui_io.AddKeyEvent(ImGuiMod_Super, false);
        break;
      default:
        break;
      }
      return false;                                                             // the event was not consumed
    }
  );
  emscripten_set_keypress_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenKeyboardEvent const *key_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_KEYPRESS
      auto &imgui_io{ImGui::GetIO()};
      imgui_io.AddInputCharactersUTF8(key_event->key);
      return imgui_io.WantCaptureKeyboard;                                      // the event was consumed only if imgui wants to capture the keyboard
    }
  );
  emscripten_set_resize_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenUiEvent const *event, void */*data*/) {    // event_type == EMSCRIPTEN_EVENT_RESIZE
      auto &imgui_io{ImGui::GetIO()};
      imgui_io.DisplaySize.x = static_cast<float>(event->windowInnerWidth);
      imgui_io.DisplaySize.y = static_cast<float>(event->windowInnerHeight);
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_blur_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenFocusEvent const */*event*/, void */*data*/) { // event_type == EMSCRIPTEN_EVENT_BLUR
      auto &imgui_io{ImGui::GetIO()};
      imgui_io.AddFocusEvent(false);
      imgui_io.ClearInputKeys();                                                // clear pending input keys on focus gain
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_focus_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenFocusEvent const */*event*/, void */*data*/) { // event_type == EMSCRIPTEN_EVENT_FOCUS
      auto &imgui_io{ImGui::GetIO()};
      imgui_io.AddFocusEvent(true);
      imgui_io.ClearInputKeys();                                                // clear pending input keys on focus loss - for example if you press tab to cycle to another part of the UI
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_focusin_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenFocusEvent const */*event*/, void */*data*/) { // event_type == EMSCRIPTEN_EVENT_FOCUSIN
      auto &imgui_io{ImGui::GetIO()};
      imgui_io.AddFocusEvent(true);
      imgui_io.ClearInputKeys();                                                // clear pending input keys on focus gain
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_focusout_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenFocusEvent const */*event*/, void */*data*/) { // event_type == EMSCRIPTEN_EVENT_FOCUSOUT
      auto &imgui_io{ImGui::GetIO()};
      imgui_io.AddFocusEvent(false);
      imgui_io.ClearInputKeys();                                                // clear pending input keys on focus loss - for example if you press tab to cycle to another part of the UI
      return true;                                                              // the event was consumed
    }
  );

  // TODO: touch events
}

void ImGui_ImplEmscripten_Shutdown() {
  /// Unset any callbacks set by Init
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
  emscripten_set_focusin_callback(   EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
  emscripten_set_focusout_callback(  EMSCRIPTEN_EVENT_TARGET_WINDOW,   nullptr, false, nullptr);
  // TODO: touch events

  auto &imgui_io{ImGui::GetIO()};
  imgui_io.BackendPlatformName = nullptr;
  imgui_io.BackendPlatformUserData = nullptr;
  imgui_io.BackendFlags &= ~ImGuiBackendFlags_HasMouseCursors;
}

void ImGui_ImplEmscripten_NewFrame() {
  /// Update any state that needs to be polled
  update_cursor();
}

#endif // IMGUI_DISABLE
