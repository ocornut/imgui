// dear imgui: text selection extension
// Select and copy any text Dear ImGui draws, by screen position, without changing how widgets render it.
// - Click where no widget takes the click to set the anchor (a caret shows at the nearest glyph edge), then
//   Shift+Click or drag to set the other end. Double-Click selects a word, Double-Click+drag extends by words,
//   Shift+Double-Click extends the selection to include the whole word. Triple-Click selects the line (all text on
//   that line of the window), Triple-Click+drag extends by lines, Shift+Triple-Click extends to the whole line.
// - Shift+Alt+Click, or Shift+Alt held while dragging, selects a block: the glyphs inside the rectangle spanned
//   by the two ends, copied as one line per row.
// - Both ends stay inside the container the anchor was set in: its window and, when set in a table, that table. Dragging
//   past the container's edge scrolls it, faster the longer the mouse stays out, and extends the selection to the
//   nearest point inside it. A table's frozen rows and columns come before its scrolling ones.
// - Every glyph of the container drawn between the two ends in reading order is highlighted, with bookend markers on
//   the first and last selected glyphs. Child windows inside the container take part only when they overlap the
//   rectangle spanned by the two ends (selecting in one of two side-by-side children leaves the other out).
// - A word is a run of word characters, or a run of delimiter characters (see TextSelectSetWordDelimiters()).
// - Ctrl+C (Cmd+C on macOS) or Ctrl+Insert (see TextSelectSetCopyShortcut()) copies the selection in reading order: lines from top to bottom,
//   fragments on a line from left to right, wide gaps (e.g. table columns) as tabs, soft-wrapped lines joined by a space
//   (or as given to TextSelectMarkSoftWrap()). The container's rows and columns scrolled out of view are submitted for
//   that one frame (through ImGuiListClipper and tables' box-select unclip support) and copied; hidden columns are not.
// - Ctrl+A (Cmd+A on macOS), with a caret or selection in the hovered or focused window, selects everything in its
//   container; each further press widens it: a table to its window, a child window to its parent, up to the root.
//   TextSelectSetSelectAllShortcut() changes or disables the key.
// - Escape, or a click on a widget other than the container's scrollbars, clears the selection.
// - When io.ConfigWindowsMoveFromTitleBarOnly is off, a click on background farther than the width of an 'M' (in the
//   text's font) from any text on its line clears the selection and a drag there moves the window; closer, it selects.
// - A click on a widget's text stays the widget's click. On a TreeNode or a row-spanning Selectable, dragging it past
//   the drag threshold turns it into a selection instead, unless the widget already used the drag (edited a value,
//   started a drag and drop). Any other widget starts a selection only with Alt held.
// Requires ImDrawListSharedData::TextHook, which ImDrawList::AddText() calls before emitting glyphs.
// Opt-out: PushItemFlag(ImGuiItemFlags_NoTextSelect, true) for text, TextSelectPushDisabledRegion() for an area.

#pragma once

#include "imgui.h"

#ifndef IMGUI_TEXT_SELECT_FLASH_DURATION
#define IMGUI_TEXT_SELECT_FLASH_DURATION 0.20f    // Seconds the highlight takes to fade back from opaque after a copy
#endif

#ifndef IMGUI_TEXT_SELECT_DEFAULT_DELIMITERS
#define IMGUI_TEXT_SELECT_DEFAULT_DELIMITERS "'\"`|:,()[]{}<>"
#endif

struct ImGuiTextSelectCopyLine
{
    ImVec2      Pos;        // Screen position of the line's first copied glyph
    bool        Visible;    // The first glyph was inside its clip rectangle (not scrolled out of view)
    const char* Text;       // Points into the full copied text
    const char* TextEnd;
};

// Called after a copy has set the clipboard, instead of the built-in feedback (the highlight flashes opaque and
// fades back over IMGUI_TEXT_SELECT_FLASH_DURATION). Pointers are valid only during the call.
typedef void (*ImGuiTextSelectCopyFeedbackCallback)(const char* text, const ImGuiTextSelectCopyLine* lines, int lines_count, void* user_data);

namespace ImGui
{
    // Installs into the current context. Safe to call every frame. The state is freed with the context.
    void        TextSelectInstall();
    void        TextSelectSetEnabled(bool enabled);
    void        TextSelectSetCopyFeedbackCallback(ImGuiTextSelectCopyFeedbackCallback callback, void* user_data);
    // ASCII characters that end a word, besides blanks. Other characters, including all non-ASCII, are word characters.
    void        TextSelectSetWordDelimiters(const char* delimiters);
    // Default ImGuiMod_Ctrl | ImGuiKey_A (Cmd+A on macOS with io.ConfigMacOSXBehaviors). Either chord selects; 0 disables
    // it, e.g. (ImGuiMod_Ctrl | ImGuiKey_A, ImGuiMod_Super | ImGuiKey_A) also accepts the other modifier.
    // Default ImGuiMod_Ctrl | ImGuiKey_C and ImGuiMod_Ctrl | ImGuiKey_Insert. Either chord copies; 0 disables it.
    void        TextSelectSetCopyShortcut(ImGuiKeyChord key_chord, ImGuiKeyChord alt_key_chord = 0);
    void        TextSelectSetSelectAllShortcut(ImGuiKeyChord key_chord, ImGuiKeyChord alt_key_chord = 0);

    // Text drawn in between is ignored (as ImGuiItemFlags_NoTextSelect), and clicks on that stretch of the current
    // window, from the cursor at Push to the cursor at Pop, do not set the anchor.
    void        TextSelectPushDisabledRegion();
    void        TextSelectPopDisabledRegion();

    // The next text drawn in the current window copies as this text, once, when any of its glyphs is selected.
    // E.g. the full value of a text drawn truncated. Only needed while TextSelectIsCopying(). NULL clears it, e.g. after
    // drawing in case nothing reached AddText() (a truncation down to just the ellipsis character).
    void        TextSelectSetNextCopyText(const char* text, const char* text_end = NULL);
    // Treats a rectangle as one selectable unit with the given copy text, e.g. for text drawn with
    // ImGuiItemFlags_NoTextSelect or without AddText() (animated digits). Highlights it when selected.
    void        TextSelectAddText(const ImVec2& p_min, const ImVec2& p_max, const char* text, const char* text_end = NULL);

    // The next text drawn in the current window continues the previous line (a soft wrap done by the caller):
    // copies join the two with join (a static string, "" for a word cut in two) instead of a newline.
    void        TextSelectMarkSoftWrap(const char* join = " ");
    // Selects everything in the current window and its child windows, e.g. from a "Select all" menu item.
    void        TextSelectSelectAll();
    // Selects the text inside a rectangle of the current window and copies it on the next frame, with the usual copy
    // feedback, e.g. from a click handler. copy_text, when given, is what reaches the clipboard instead.
    void        TextSelectSelectAndCopy(const ImVec2& p_min, const ImVec2& p_max, const char* copy_text = NULL, const char* copy_text_end = NULL);

    bool        TextSelectHasSelection(bool current_window_only = false);   // current_window_only: in the current window's root
    bool        TextSelectIsCopying();          // True during the frame whose text is being collected for a copy
    void        TextSelectClear();
}
