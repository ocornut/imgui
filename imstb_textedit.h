// [DEAR IMGUI]
// This is a slightly modified version of stb_textedit.h 1.14.
// Those changes would need to be pushed into nothings/stb:
// - Fix in stb_textedit_discard_redo (see https://github.com/nothings/stb/issues/321)
// - Fix in stb_textedit_find_charpos to handle last line (see https://github.com/ocornut/imgui/issues/6000 + #6783)
// Grep for [DEAR IMGUI] to find the changes.
// - Also renamed macros used or defined outside of IMSTB_TEXTEDIT_IMPLEMENTATION block from STB_TEXTEDIT_* to IMSTB_TEXTEDIT_*

// stb_textedit.h - v1.14  - public domain - Sean Barrett
// Development of this library was sponsored by RAD Game Tools
//
// This C header file implements the guts of a multi-line text-editing
// widget; you implement display, word-wrapping, and low-level string
// insertion/deletion, and stb_textedit will map user inputs into
// insertions & deletions, plus updates to the cursor position,
// selection state, and undo state.
//
// It is intended for use in games and other systems that need to build
// their own custom widgets and which do not have heavy text-editing
// requirements (this library is not recommended for use for editing large
// texts, as its performance does not scale and it has limited undo).
//
// Non-trivial behaviors are modelled after Windows text controls.
//
//
// LICENSE
//
// See end of file for license information.
//
//
// DEPENDENCIES
//
// Uses the C runtime function 'memmove', which you can override
// by defining IMSTB_TEXTEDIT_memmove before the implementation.
// Uses no other functions. Performs no runtime allocations.
//
//
// VERSION HISTORY
//
//   1.14 (2021-07-11) page up/down, various fixes
//   1.13 (2019-02-07) fix bug in undo size management
//   1.12 (2018-01-29) user can change STB_TEXTEDIT_KEYTYPE, fix redo to avoid crash
//   1.11 (2017-03-03) fix HOME on last line, dragging off single-line textfield
//   1.10 (2016-10-25) supress warnings about casting away const with -Wcast-qual
//   1.9  (2016-08-27) customizable move-by-word
//   1.8  (2016-04-02) better keyboard handling when mouse button is down
//   1.7  (2015-09-13) change y range handling in case baseline is non-0
//   1.6  (2015-04-15) allow STB_TEXTEDIT_memmove
//   1.5  (2014-09-10) add support for secondary keys for OS X
//   1.4  (2014-08-17) fix signed/unsigned warnings
//   1.3  (2014-06-19) fix mouse clicking to round to nearest char boundary
//   1.2  (2014-05-27) fix some RAD types that had crept into the new code
//   1.1  (2013-12-15) move-by-word (requires STB_TEXTEDIT_IS_SPACE )
//   1.0  (2012-07-26) improve documentation, initial public release
//   0.3  (2012-02-24) bugfixes, single-line mode; insert mode
//   0.2  (2011-11-28) fixes to undo/redo
//   0.1  (2010-07-08) initial version
//
// ADDITIONAL CONTRIBUTORS
//
//   Ulf Winklemann: move-by-word in 1.1
//   Fabian Giesen: secondary key inputs in 1.5
//   Martins Mozeiko: STB_TEXTEDIT_memmove in 1.6
//   Louis Schnellbach: page up/down in 1.14
//
//   Bugfixes:
//      Scott Graham
//      Daniel Keller
//      Omar Cornut
//      Dan Thompson
//
// USAGE
//
// This file behaves differently depending on what symbols you define
// before including it.
//
//
// Header-file mode:
//
//   If you do not define STB_TEXTEDIT_IMPLEMENTATION before including this,
//   it will operate in "header file" mode. In this mode, it declares a
//   single public symbol, STB_TexteditState, which encapsulates the current
//   state of a text widget (except for the string, which you will store
//   separately).
//
//   To compile in this mode, you must define STB_TEXTEDIT_CHARTYPE to a
//   primitive type that defines a single character (e.g. char, wchar_t, etc).
//
//   To save space or increase undo-ability, you can optionally define the
//   following things that are used by the undo system:
//
//      STB_TEXTEDIT_POSITIONTYPE         small int type encoding a valid cursor position
//      STB_TEXTEDIT_UNDOSTATECOUNT       the number of undo states to allow
//      STB_TEXTEDIT_UNDOCHARCOUNT        the number of characters to store in the undo buffer
//
//   If you don't define these, they are set to permissive types and
//   moderate sizes. The undo system does no memory allocations, so
//   it grows STB_TexteditState by the worst-case storage which is (in bytes):
//
//        [4 + 3 * sizeof(STB_TEXTEDIT_POSITIONTYPE)] * STB_TEXTEDIT_UNDOSTATECOUNT
//      +          sizeof(STB_TEXTEDIT_CHARTYPE)      * STB_TEXTEDIT_UNDOCHARCOUNT
//
//
// Implementation mode:
//
//   If you define STB_TEXTEDIT_IMPLEMENTATION before including this, it
//   will compile the implementation of the text edit widget, depending
//   on a large number of symbols which must be defined before the include.
//
//   The implementation is defined only as static functions. You will then
//   need to provide your own APIs in the same file which will access the
//   static functions.
//
//   The basic concept is that you provide a "string" object which
//   behaves like an array of characters. stb_textedit uses indices to
//   refer to positions in the string, implicitly representing positions
//   in the displayed textedit. This is true for both plain text and
//   rich text; even with rich text stb_truetype interacts with your
//   code as if there was an array of all the displayed characters.
//
// Symbols that must be the same in header-file and implementation mode:
//
//     STB_TEXTEDIT_CHARTYPE             the character type
//     STB_TEXTEDIT_POSITIONTYPE         small type that is a valid cursor position
//     STB_TEXTEDIT_UNDOSTATECOUNT       the number of undo states to allow
//     STB_TEXTEDIT_UNDOCHARCOUNT        the number of characters to store in the undo buffer
//
// Symbols you must define for implementation mode:
//
//    STB_TEXTEDIT_STRING               the type of object representing a string being edited,
//                                      typically this is a wrapper object with other data you need
//
//    STB_TEXTEDIT_STRINGLEN(obj)       the length of the string (ideally O(1))
//    STB_TEXTEDIT_LAYOUTROW(&r,obj,n)  returns the results of laying out a line of characters
//                                        starting from character #n (see discussion below)
//    STB_TEXTEDIT_GETWIDTH(obj,n,i)    returns the pixel delta from the xpos of the i'th character
//                                        to the xpos of the i+1'th char for a line of characters
//                                        starting at character #n (i.e. accounts for kerning
//                                        with previous char)
//    STB_TEXTEDIT_KEYTOTEXT(k)         maps a keyboard input to an insertable character
//                                        (return type is int, -1 means not valid to insert)
//    STB_TEXTEDIT_GETCHAR(obj,i)       returns the i'th character of obj, 0-based
//    STB_TEXTEDIT_NEWLINE              the character returned by _GETCHAR() we recognize
//                                        as manually wordwrapping for end-of-line positioning
//
//    STB_TEXTEDIT_DELETECHARS(obj,i,n)      delete n characters starting at i
//    STB_TEXTEDIT_INSERTCHARS(obj,i,c*,n)   insert n characters at i (pointed to by STB_TEXTEDIT_CHARTYPE*)
//
//    STB_TEXTEDIT_K_SHIFT       a power of two that is or'd in to a keyboard input to represent the shift key
//
//    STB_TEXTEDIT_K_LEFT        keyboard input to move cursor left
//    STB_TEXTEDIT_K_RIGHT       keyboard input to move cursor right
//    STB_TEXTEDIT_K_UP          keyboard input to move cursor up
//    STB_TEXTEDIT_K_DOWN        keyboard input to move cursor down
//    STB_TEXTEDIT_K_PGUP        keyboard input to move cursor up a page
//    STB_TEXTEDIT_K_PGDOWN      keyboard input to move cursor down a page
//    STB_TEXTEDIT_K_LINESTART   keyboard input to move cursor to start of line  // e.g. HOME
//    STB_TEXTEDIT_K_LINEEND     keyboard input to move cursor to end of line    // e.g. END
//    STB_TEXTEDIT_K_TEXTSTART   keyboard input to move cursor to start of text  // e.g. ctrl-HOME
//    STB_TEXTEDIT_K_TEXTEND     keyboard input to move cursor to end of text    // e.g. ctrl-END
//    STB_TEXTEDIT_K_DELETE      keyboard input to delete selection or character under cursor
//    STB_TEXTEDIT_K_BACKSPACE   keyboard input to delete selection or character left of cursor
//    STB_TEXTEDIT_K_UNDO        keyboard input to perform undo
//    STB_TEXTEDIT_K_REDO        keyboard input to perform redo
//
// Optional:
//    STB_TEXTEDIT_K_INSERT              keyboard input to toggle insert mode
//    STB_TEXTEDIT_IS_SPACE(ch)          true if character is whitespace (e.g. 'isspace'),
//                                          required for default WORDLEFT/WORDRIGHT handlers
//    STB_TEXTEDIT_MOVEWORDLEFT(obj,i)   custom handler for WORDLEFT, returns index to move cursor to
//    STB_TEXTEDIT_MOVEWORDRIGHT(obj,i)  custom handler for WORDRIGHT, returns index to move cursor to
//    STB_TEXTEDIT_K_WORDLEFT            keyboard input to move cursor left one word // e.g. ctrl-LEFT
//    STB_TEXTEDIT_K_WORDRIGHT           keyboard input to move cursor right one word // e.g. ctrl-RIGHT
//    STB_TEXTEDIT_K_LINESTART2          secondary keyboard input to move cursor to start of line
//    STB_TEXTEDIT_K_LINEEND2            secondary keyboard input to move cursor to end of line
//    STB_TEXTEDIT_K_TEXTSTART2          secondary keyboard input to move cursor to start of text
//    STB_TEXTEDIT_K_TEXTEND2            secondary keyboard input to move cursor to end of text
//
// Keyboard input must be encoded as a single integer value; e.g. a character code
// and some bitflags that represent shift states. to simplify the interface, SHIFT must
// be a bitflag, so we can test the shifted state of cursor movements to allow selection,
// i.e. (STB_TEXTEDIT_K_RIGHT|STB_TEXTEDIT_K_SHIFT) should be shifted right-arrow.
//
// You can encode other things, such as CONTROL or ALT, in additional bits, and
// then test for their presence in e.g. STB_TEXTEDIT_K_WORDLEFT. For example,
// my Windows implementations add an additional CONTROL bit, and an additional KEYDOWN
// bit. Then all of the STB_TEXTEDIT_K_ values bitwise-or in the KEYDOWN bit,
// and I pass both WM_KEYDOWN and WM_CHAR events to the "key" function in the
// API below. The control keys will only match WM_KEYDOWN events because of the
// keydown bit I add, and STB_TEXTEDIT_KEYTOTEXT only tests for the KEYDOWN
// bit so it only decodes WM_CHAR events.
//
// STB_TEXTEDIT_LAYOUTROW returns information about the shape of one displayed
// row of characters assuming they start on the i'th character--the width and
// the height and the number of characters consumed. This allows this library
// to traverse the entire layout incrementally. You need to compute word-wrapping
// here.
//
// Each textfield keeps its own insert mode state, which is not how normal
// applications work. To keep an app-wide insert mode, update/copy the
// "insert_mode" field of STB_TexteditState before/after calling API functions.
//
// API
//
//    void stb_textedit_initialize_state(STB_TexteditState *state, int is_single_line)
//
//    void stb_textedit_click(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, float x, float y)
//    void stb_textedit_drag(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, float x, float y)
//    int  stb_textedit_cut(STB_TEXTEDIT_STRING *str, STB_TexteditState *state)
//    int  stb_textedit_paste(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, STB_TEXTEDIT_CHARTYPE *text, int len)
//    void stb_textedit_key(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, STB_TEXEDIT_KEYTYPE key)
//
//    Each of these functions potentially updates the string and updates the
//    state.
//
//      initialize_state:
//          set the textedit state to a known good default state when initially
//          constructing the textedit.
//
//      click:
//          call this with the mouse x,y on a mouse down; it will update the cursor
//          and reset the selection start/end to the cursor point. the x,y must
//          be relative to the text widget, with (0,0) being the top left.
//
//      drag:
//          call this with the mouse x,y on a mouse drag/up; it will update the
//          cursor and the selection end point
//
//      cut:
//          call this to delete the current selection; returns true if there was
//          one. you should FIRST copy the current selection to the system paste buffer.
//          (To copy, just copy the current selection out of the string yourself.)
//
//      paste:
//          call this to paste text at the current cursor point or over the current
//          selection if there is one.
//
//      key:
//          call this for keyboard inputs sent to the textfield. you can use it
//          for "key down" events or for "translated" key events. if you need to
//          do both (as in Win32), or distinguish Unicode characters from control
//          inputs, set a high bit to distinguish the two; then you can define the
//          various definitions like STB_TEXTEDIT_K_LEFT have the is-key-event bit
//          set, and make STB_TEXTEDIT_KEYTOCHAR check that the is-key-event bit is
//          clear. STB_TEXTEDIT_KEYTYPE defaults to int, but you can #define it to
//          anything other type you wante before including.
//
//
//   When rendering, you can read the cursor position and selection state from
//   the STB_TexteditState.
//
//
// Notes:
//
// This is designed to be usable in IMGUI, so it allows for the possibility of
// running in an IMGUI that has NOT cached the multi-line layout. For this
// reason, it provides an interface that is compatible with computing the
// layout incrementally--we try to make sure we make as few passes through
// as possible. (For example, to locate the mouse pointer in the text, we
// could define functions that return the X and Y positions of characters
// and binary search Y and then X, but if we're doing dynamic layout this
// will run the layout algorithm many times, so instead we manually search
// forward in one pass. Similar logic applies to e.g. up-arrow and
// down-arrow movement.)
//
// If it's run in a widget that *has* cached the layout, then this is less
// efficient, but it's not horrible on modern computers. But you wouldn't
// want to edit million-line files with it.


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////
////   Header-file mode
////
////

#ifndef INCLUDE_IMSTB_TEXTEDIT_H
#define INCLUDE_IMSTB_TEXTEDIT_H

////////////////////////////////////////////////////////////////////////
//
//     STB_TexteditState
//
// Definition of STB_TexteditState which you should store
// per-textfield; it includes cursor position, selection state,
// and undo state.
//

#ifndef IMSTB_TEXTEDIT_UNDOSTATECOUNT
#define IMSTB_TEXTEDIT_UNDOSTATECOUNT   99
#endif
#ifndef IMSTB_TEXTEDIT_UNDOCHARCOUNT
#define IMSTB_TEXTEDIT_UNDOCHARCOUNT   999
#endif
#ifndef IMSTB_TEXTEDIT_CHARTYPE
#define IMSTB_TEXTEDIT_CHARTYPE        int
#endif
#ifndef IMSTB_TEXTEDIT_POSITIONTYPE
#define IMSTB_TEXTEDIT_POSITIONTYPE    int
#endif

typedef struct
{
   // private data
   IMSTB_TEXTEDIT_POSITIONTYPE  where;
   IMSTB_TEXTEDIT_POSITIONTYPE  insert_length;
   IMSTB_TEXTEDIT_POSITIONTYPE  delete_length;
   int                        char_storage;
} StbUndoRecord;

typedef struct
{
   // private data
   StbUndoRecord          undo_rec [IMSTB_TEXTEDIT_UNDOSTATECOUNT];
   IMSTB_TEXTEDIT_CHARTYPE  undo_char[IMSTB_TEXTEDIT_UNDOCHARCOUNT];
   short undo_point, redo_point;
   int undo_char_point, redo_char_point;
} StbUndoState;

typedef struct
{
   /////////////////////
   //
   // public data
   //

   int cursor;
   // position of the text cursor within the string

   int select_start;          // selection start point
   int select_end;
   // selection start and end point in characters; if equal, no selection.
   // note that start may be less than or greater than end (e.g. when
   // dragging the mouse, start is where the initial click was, and you
   // can drag in either direction)

   unsigned char insert_mode;
   // each textfield keeps its own insert mode state. to keep an app-wide
   // insert mode, copy this value in/out of the app state

   int row_count_per_page;
   // page size in number of row.
   // this value MUST be set to >0 for pageup or pagedown in multilines documents.

   /////////////////////
   //
   // private data
   //
   unsigned char cursor_at_end_of_line; // not implemented yet
   unsigned char initialized;
   unsigned char has_preferred_x;
   unsigned char single_line;
   unsigned char padding1, padding2, padding3;
   float preferred_x; // this determines where the cursor up/down tries to seek to along x
   StbUndoState undostate;
} STB_TexteditState;


////////////////////////////////////////////////////////////////////////
//
//     StbTexteditRow
//
// Result of layout query, used by stb_textedit to determine where
// the text in each row is.

// result of layout query
typedef struct
{
   float x0,x1;             // starting x location, end x location (allows for align=right, etc)
   float baseline_y_delta;  // position of baseline relative to previous row's baseline
   float ymin,ymax;         // height of row above and below baseline
   int num_chars;
} StbTexteditRow;
#endif //INCLUDE_IMSTB_TEXTEDIT_H


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////
////   Implementation mode
////
////


// implementation isn't include-guarded, since it might have indirectly
// included just the "header" portion
#ifdef IMSTB_TEXTEDIT_IMPLEMENTATION

#ifndef IMSTB_TEXTEDIT_memmove
#include <string.h>
#define IMSTB_TEXTEDIT_memmove memmove
#endif


/////////////////////////////////////////////////////////////////////////////
//
//      Mouse input handling
//

// traverse the layout to locate the nearest character to a display position
static int stb_text_locate_coord(IMSTB_TEXTEDIT_STRING *str, float x, float y)
{
   StbTexteditRow r;
   int n = STB_TEXTEDIT_STRINGLEN(str);
   float base_y = 0, prev_x;
   int i=0, k;

   r.x0 = r.x1 = 0;
   r.ymin = r.ymax = 0;
   r.num_chars = 0;

   // search rows to find one that straddles 'y'
   while (i < n) {
      STB_TEXTEDIT_LAYOUTROW(&r, str, i);
      if (r.num_chars <= 0)
         return n;

      if (i==0 && y < base_y + r.ymin)
         return 0;

      if (y < base_y + r.ymax)
         break;

      i += r.num_chars;
      base_y += r.baseline_y_delta;
   }

   // below all text, return 'after' last character
   if (i >= n)
      return n;

   // check if it's before the beginning of the line
   if (x < r.x0)
      return i;

   // check if it's before the end of the line
   if (x < r.x1) {
      // search characters in row for one that straddles 'x'
      prev_x = r.x0;
      for (k=0; k < r.num_chars; ++k) {
         float w = STB_TEXTEDIT_GETWIDTH(str, i, k);
         if (x < prev_x+w) {
            if (x < prev_x+w/2)
               return k+i;
            else
               return k+i+1;
         }
         prev_x += w;
      }
      // shouldn't happen, but if it does, fall through to end-of-line case
   }

   // if the last character is a newline, return that. otherwise return 'after' the last character
   if (STB_TEXTEDIT_GETCHAR(str, i+r.num_chars-1) == STB_TEXTEDIT_NEWLINE)
      return i+r.num_chars-1;
   else
      return i+r.num_chars;
}

// API click: on mouse down, move the cursor to the clicked location, and reset the selection
static void stb_textedit_click(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state, float x, float y)
{
   // In single-line mode, just always make y = 0. This lets the drag keep working if the mouse
   // goes off the top or bottom of the text
   if( state->single_line )
   {
      StbTexteditRow r;
      STB_TEXTEDIT_LAYOUTROW(&r, str, 0);
      y = r.ymin;
   }

   state->cursor = stb_text_locate_coord(str, x, y);
   state->select_start = state->cursor;
   state->select_end = state->cursor;
   state->has_preferred_x = 0;
}

// API drag: on mouse drag, move the cursor and selection endpoint to the clicked location
static void stb_textedit_drag(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state, float x, float y)
{
   int p = 0;

   // In single-line mode, just always make y = 0. This lets the drag keep working if the mouse
   // goes off the top or bottom of the text
   if( state->single_line )
   {
      StbTexteditRow r;
      STB_TEXTEDIT_LAYOUTROW(&r, str, 0);
      y = r.ymin;
   }

   if (state->select_start == state->select_end)
      state->select_start = state->cursor;

   p = stb_text_locate_coord(str, x, y);
   state->cursor = state->select_end = p;
}

/////////////////////////////////////////////////////////////////////////////
//
//      Keyboard input handling
//

// forward declarations
static void stb_text_undo(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state);
static void stb_text_redo(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state);
static void stb_text_makeundo_delete(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state, int where, int length);
static void stb_text_makeundo_insert(STB_TexteditState *state, int where, int length);
static void stb_text_makeundo_replace(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state, int where, int old_length, int new_length);

typedef struct
{
   float x,y;    // position of n'th character
   float height; // height of line
   int first_char, length; // first char of row, and length
   int prev_first;  // first char of previous row
} StbFindState;

// find the x/y location of a character, and remember info about the previous row in
// case we get a move-up event (for page up, we'll have to rescan)
static void stb_textedit_find_charpos(StbFindState *find, IMSTB_TEXTEDIT_STRING *str, int n, int single_line)
{
   StbTexteditRow r;
   int prev_start = 0;
   int z = STB_TEXTEDIT_STRINGLEN(str);
   int i=0, first;

   if (n == z && single_line) {
      // special case if it's at the end (may not be needed?)
      STB_TEXTEDIT_LAYOUTROW(&r, str, 0);
      find->y = 0;
      find->first_char = 0;
      find->length = z;
      find->height = r.ymax - r.ymin;
      find->x = r.x1;
      return;
   }

   // search rows to find the one that straddles character n
   find->y = 0;

   for(;;) {
      STB_TEXTEDIT_LAYOUTROW(&r, str, i);
      if (n < i + r.num_chars)
         break;
      if (i + r.num_chars == z && z > 0 && STB_TEXTEDIT_GETCHAR(str, z - 1) != STB_TEXTEDIT_NEWLINE)  // [DEAR IMGUI] special handling for last line
         break;   // [DEAR IMGUI]
      prev_start = i;
      i += r.num_chars;
      find->y += r.baseline_y_delta;
      if (i == z) // [DEAR IMGUI]
      {
         r.num_chars = 0; // [DEAR IMGUI]
         break;   // [DEAR IMGUI]
      }
   }

   find->first_char = first = i;
   find->length = r.num_chars;
   find->height = r.ymax - r.ymin;
   find->prev_first = prev_start;

   // now scan to find xpos
   find->x = r.x0;
   for (i=0; first+i < n; ++i)
      find->x += STB_TEXTEDIT_GETWIDTH(str, first, i);
}

#define STB_TEXT_HAS_SELECTION(s)   ((s)->select_start != (s)->select_end)

// make the selection/cursor state valid if client altered the string
static void stb_textedit_clamp(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state)
{
   int n = STB_TEXTEDIT_STRINGLEN(str);
   if (STB_TEXT_HAS_SELECTION(state)) {
      if (state->select_start > n) state->select_start = n;
      if (state->select_end   > n) state->select_end = n;
      // if clamping forced them to be equal, move the cursor to match
      if (state->select_start == state->select_end)
         state->cursor = state->select_start;
   }
   if (state->cursor > n) state->cursor = n;
}

// delete characters while updating undo
static void stb_textedit_delete(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state, int where, int len)
{
   stb_text_makeundo_delete(str, state, where, len);
   STB_TEXTEDIT_DELETECHARS(str, where, len);
   state->has_preferred_x = 0;
}

// delete the section
static void stb_textedit_delete_selection(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state)
{
   stb_textedit_clamp(str, state);
   if (STB_TEXT_HAS_SELECTION(state)) {
      if (state->select_start < state->select_end) {
         stb_textedit_delete(str, state, state->select_start, state->select_end - state->select_start);
         state->select_end = state->cursor = state->select_start;
      } else {
         stb_textedit_delete(str, state, state->select_end, state->select_start - state->select_end);
         state->select_start = state->cursor = state->select_end;
      }
      state->has_preferred_x = 0;
   }
}

// canoncialize the selection so start <= end
static void stb_textedit_sortselection(STB_TexteditState *state)
{
   if (state->select_end < state->select_start) {
      int temp = state->select_end;
      state->select_end = state->select_start;
      state->select_start = temp;
   }
}

// move cursor to first character of selection
static void stb_textedit_move_to_first(STB_TexteditState *state)
{
   if (STB_TEXT_HAS_SELECTION(state)) {
      stb_textedit_sortselection(state);
      state->cursor = state->select_start;
      state->select_end = state->select_start;
      state->has_preferred_x = 0;
   }
}

// move cursor to last character of selection
static void stb_textedit_move_to_last(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state)
{
   if (STB_TEXT_HAS_SELECTION(state)) {
      stb_textedit_sortselection(state);
      stb_textedit_clamp(str, state);
      state->cursor = state->select_end;
      state->select_start = state->select_end;
      state->has_preferred_x = 0;
   }
}

#ifdef STB_TEXTEDIT_IS_SPACE
static int is_word_boundary( IMSTB_TEXTEDIT_STRING *str, int idx )
{
   return idx > 0 ? (STB_TEXTEDIT_IS_SPACE( STB_TEXTEDIT_GETCHAR(str,idx-1) ) && !STB_TEXTEDIT_IS_SPACE( STB_TEXTEDIT_GETCHAR(str, idx) ) ) : 1;
}

#ifndef STB_TEXTEDIT_MOVEWORDLEFT
static int stb_textedit_move_to_word_previous( IMSTB_TEXTEDIT_STRING *str, int c )
{
   --c; // always move at least one character
   while( c >= 0 && !is_word_boundary( str, c ) )
      --c;

   if( c < 0 )
      c = 0;

   return c;
}
#define STB_TEXTEDIT_MOVEWORDLEFT stb_textedit_move_to_word_previous
#endif

#ifndef STB_TEXTEDIT_MOVEWORDRIGHT
static int stb_textedit_move_to_word_next( IMSTB_TEXTEDIT_STRING *str, int c )
{
   const int len = STB_TEXTEDIT_STRINGLEN(str);
   ++c; // always move at least one character
   while( c < len && !is_word_boundary( str, c ) )
      ++c;

   if( c > len )
      c = len;

   return c;
}
#define STB_TEXTEDIT_MOVEWORDRIGHT stb_textedit_move_to_word_next
#endif

#endif

// update selection and cursor to match each other
static void stb_textedit_prep_selection_at_cursor(STB_TexteditState *state)
{
   if (!STB_TEXT_HAS_SELECTION(state))
      state->select_start = state->select_end = state->cursor;
   else
      state->cursor = state->select_end;
}

// API cut: delete selection
static int stb_textedit_cut(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state)
{
   if (STB_TEXT_HAS_SELECTION(state)) {
      stb_textedit_delete_selection(str,state); // implicitly clamps
      state->has_preferred_x = 0;
      return 1;
   }
   return 0;
}

// API paste: replace existing selection with passed-in text
static int stb_textedit_paste_internal(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state, IMSTB_TEXTEDIT_CHARTYPE *text, int len)
{
   // if there's a selection, the paste should delete it
   stb_textedit_clamp(str, state);
   stb_textedit_delete_selection(str,state);
   // try to insert the characters
   if (STB_TEXTEDIT_INSERTCHARS(str, state->cursor, text, len)) {
      stb_text_makeundo_insert(state, state->cursor, len);
      state->cursor += len;
      state->has_preferred_x = 0;
      return 1;
   }
   // note: paste failure will leave deleted selection, may be restored with an undo (see https://github.com/nothings/stb/issues/734 for details)
   return 0;
}

#ifndef STB_TEXTEDIT_KEYTYPE
#define STB_TEXTEDIT_KEYTYPE int
#endif

// API key: process a keyboard input
static void stb_textedit_key(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state, STB_TEXTEDIT_KEYTYPE key)
{
retry:
   switch (key) {
      default: {
         int c = STB_TEXTEDIT_KEYTOTEXT(key);
         if (c > 0) {
            IMSTB_TEXTEDIT_CHARTYPE ch = (IMSTB_TEXTEDIT_CHARTYPE) c;

            // can't add newline in single-line mode
            if (c == '\n' && state->single_line)
               break;

            if (state->insert_mode && !STB_TEXT_HAS_SELECTION(state) && state->cursor < STB_TEXTEDIT_STRINGLEN(str)) {
               stb_text_makeundo_replace(str, state, state->cursor, 1, 1);
               STB_TEXTEDIT_DELETECHARS(str, state->cursor, 1);
               if (STB_TEXTEDIT_INSERTCHARS(str, state->cursor, &ch, 1)) {
                  ++state->cursor;
                  state->has_preferred_x = 0;
               }
            } else {
               stb_textedit_delete_selection(str,state); // implicitly clamps
               if (STB_TEXTEDIT_INSERTCHARS(str, state->cursor, &ch, 1)) {
                  stb_text_makeundo_insert(state, state->cursor, 1);
                  ++state->cursor;
                  state->has_preferred_x = 0;
               }
            }
         }
         break;
      }

#ifdef STB_TEXTEDIT_K_INSERT
      case STB_TEXTEDIT_K_INSERT:
         state->insert_mode = !state->insert_mode;
         break;
#endif

      case STB_TEXTEDIT_K_UNDO:
         stb_text_undo(str, state);
         state->has_preferred_x = 0;
         break;

      case STB_TEXTEDIT_K_REDO:
         stb_text_redo(str, state);
         state->has_preferred_x = 0;
         break;

      case STB_TEXTEDIT_K_LEFT:
         // if currently there's a selection, move cursor to start of selection
         if (STB_TEXT_HAS_SELECTION(state))
            stb_textedit_move_to_first(state);
         else
            if (state->cursor > 0)
               --state->cursor;
         state->has_preferred_x = 0;
         break;

      case STB_TEXTEDIT_K_RIGHT:
         // if currently there's a selection, move cursor to end of selection
         if (STB_TEXT_HAS_SELECTION(state))
            stb_textedit_move_to_last(str, state);
         else
            ++state->cursor;
         stb_textedit_clamp(str, state);
         state->has_preferred_x = 0;
         break;

      case STB_TEXTEDIT_K_LEFT | STB_TEXTEDIT_K_SHIFT:
         stb_textedit_clamp(str, state);
         stb_textedit_prep_selection_at_cursor(state);
         // move selection left
         if (state->select_end > 0)
            --state->select_end;
         state->cursor = state->select_end;
         state->has_preferred_x = 0;
         break;

#ifdef STB_TEXTEDIT_MOVEWORDLEFT
      case STB_TEXTEDIT_K_WORDLEFT:
         if (STB_TEXT_HAS_SELECTION(state))
            stb_textedit_move_to_first(state);
         else {
            state->cursor = STB_TEXTEDIT_MOVEWORDLEFT(str, state->cursor);
            stb_textedit_clamp( str, state );
         }
         break;

      case STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT:
         if( !STB_TEXT_HAS_SELECTION( state ) )
            stb_textedit_prep_selection_at_cursor(state);

         state->cursor = STB_TEXTEDIT_MOVEWORDLEFT(str, state->cursor);
         state->select_end = state->cursor;

         stb_textedit_clamp( str, state );
         break;
#endif

#ifdef STB_TEXTEDIT_MOVEWORDRIGHT
      case STB_TEXTEDIT_K_WORDRIGHT:
         if (STB_TEXT_HAS_SELECTION(state))
            stb_textedit_move_to_last(str, state);
         else {
            state->cursor = STB_TEXTEDIT_MOVEWORDRIGHT(str, state->cursor);
            stb_textedit_clamp( str, state );
         }
         break;

      case STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT:
         if( !STB_TEXT_HAS_SELECTION( state ) )
            stb_textedit_prep_selection_at_cursor(state);

         state->cursor = STB_TEXTEDIT_MOVEWORDRIGHT(str, state->cursor);
         state->select_end = state->cursor;

         stb_textedit_clamp( str, state );
         break;
#endif

      case STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_SHIFT:
         stb_textedit_prep_selection_at_cursor(state);
         // move selection right
         ++state->select_end;
         stb_textedit_clamp(str, state);
         state->cursor = state->select_end;
         state->has_preferred_x = 0;
         break;

      case STB_TEXTEDIT_K_DOWN:
      case STB_TEXTEDIT_K_DOWN | STB_TEXTEDIT_K_SHIFT:
      case STB_TEXTEDIT_K_PGDOWN:
      case STB_TEXTEDIT_K_PGDOWN | STB_TEXTEDIT_K_SHIFT: {
         StbFindState find;
         StbTexteditRow row;
         int i, j, sel = (key & STB_TEXTEDIT_K_SHIFT) != 0;
         int is_page = (key & ~STB_TEXTEDIT_K_SHIFT) == STB_TEXTEDIT_K_PGDOWN;
         int row_count = is_page ? state->row_count_per_page : 1;

         if (!is_page && state->single_line) {
            // on windows, up&down in single-line behave like left&right
            key = STB_TEXTEDIT_K_RIGHT | (key & STB_TEXTEDIT_K_SHIFT);
            goto retry;
         }

         if (sel)
            stb_textedit_prep_selection_at_cursor(state);
         else if (STB_TEXT_HAS_SELECTION(state))
            stb_textedit_move_to_last(str, state);

         // compute current position of cursor point
         stb_textedit_clamp(str, state);
         stb_textedit_find_charpos(&find, str, state->cursor, state->single_line);

         for (j = 0; j < row_count; ++j) {
            float x, goal_x = state->has_preferred_x ? state->preferred_x : find.x;
            int start = find.first_char + find.length;

            if (find.length == 0)
               break;

            // [DEAR IMGUI]
            // going down while being on the last line shouldn't bring us to that line end
            if (STB_TEXTEDIT_GETCHAR(str, find.first_char + find.length - 1) != STB_TEXTEDIT_NEWLINE)
               break;

            // now find character position down a row
            state->cursor = start;
            STB_TEXTEDIT_LAYOUTROW(&row, str, state->cursor);
            x = row.x0;
            for (i=0; i < row.num_chars; ++i) {
               float dx = STB_TEXTEDIT_GETWIDTH(str, start, i);
               #ifdef IMSTB_TEXTEDIT_GETWIDTH_NEWLINE
               if (dx == IMSTB_TEXTEDIT_GETWIDTH_NEWLINE)
                  break;
               #endif
               x += dx;
               if (x > goal_x)
                  break;
               ++state->cursor;
            }
            stb_textedit_clamp(str, state);

            state->has_preferred_x = 1;
            state->preferred_x = goal_x;

            if (sel)
               state->select_end = state->cursor;

            // go to next line
            find.first_char = find.first_char + find.length;
            find.length = row.num_chars;
         }
         break;
      }

      case STB_TEXTEDIT_K_UP:
      case STB_TEXTEDIT_K_UP | STB_TEXTEDIT_K_SHIFT:
      case STB_TEXTEDIT_K_PGUP:
      case STB_TEXTEDIT_K_PGUP | STB_TEXTEDIT_K_SHIFT: {
         StbFindState find;
         StbTexteditRow row;
         int i, j, prev_scan, sel = (key & STB_TEXTEDIT_K_SHIFT) != 0;
         int is_page = (key & ~STB_TEXTEDIT_K_SHIFT) == STB_TEXTEDIT_K_PGUP;
         int row_count = is_page ? state->row_count_per_page : 1;

         if (!is_page && state->single_line) {
            // on windows, up&down become left&right
            key = STB_TEXTEDIT_K_LEFT | (key & STB_TEXTEDIT_K_SHIFT);
            goto retry;
         }

         if (sel)
            stb_textedit_prep_selection_at_cursor(state);
         else if (STB_TEXT_HAS_SELECTION(state))
            stb_textedit_move_to_first(state);

         // compute current position of cursor point
         stb_textedit_clamp(str, state);
         stb_textedit_find_charpos(&find, str, state->cursor, state->single_line);

         for (j = 0; j < row_count; ++j) {
            float  x, goal_x = state->has_preferred_x ? state->preferred_x : find.x;

            // can only go up if there's a previous row
            if (find.prev_first == find.first_char)
               break;

            // now find character position up a row
            state->cursor = find.prev_first;
            STB_TEXTEDIT_LAYOUTROW(&row, str, state->cursor);
            x = row.x0;
            for (i=0; i < row.num_chars; ++i) {
               float dx = STB_TEXTEDIT_GETWIDTH(str, find.prev_first, i);
               #ifdef IMSTB_TEXTEDIT_GETWIDTH_NEWLINE
               if (dx == IMSTB_TEXTEDIT_GETWIDTH_NEWLINE)
                  break;
               #endif
               x += dx;
               if (x > goal_x)
                  break;
               ++state->cursor;
            }
            stb_textedit_clamp(str, state);

            state->has_preferred_x = 1;
            state->preferred_x = goal_x;

            if (sel)
               state->select_end = state->cursor;

            // go to previous line
            // (we need to scan previous line the hard way. maybe we could expose this as a new API function?)
            prev_scan = find.prev_first > 0 ? find.prev_first - 1 : 0;
            while (prev_scan > 0 && STB_TEXTEDIT_GETCHAR(str, prev_scan - 1) != STB_TEXTEDIT_NEWLINE)
               --prev_scan;
            find.first_char = find.prev_first;
            find.prev_first = prev_scan;
         }
         break;
      }

      case STB_TEXTEDIT_K_DELETE:
      case STB_TEXTEDIT_K_DELETE | STB_TEXTEDIT_K_SHIFT:
         if (STB_TEXT_HAS_SELECTION(state))
            stb_textedit_delete_selection(str, state);
         else {
            int n = STB_TEXTEDIT_STRINGLEN(str);
            if (state->cursor < n)
               stb_textedit_delete(str, state, state->cursor, 1);
         }
         state->has_preferred_x = 0;
         break;

      case STB_TEXTEDIT_K_BACKSPACE:
      case STB_TEXTEDIT_K_BACKSPACE | STB_TEXTEDIT_K_SHIFT:
         if (STB_TEXT_HAS_SELECTION(state))
            stb_textedit_delete_selection(str, state);
         else {
            stb_textedit_clamp(str, state);
            if (state->cursor > 0) {
               stb_textedit_delete(str, state, state->cursor-1, 1);
               --state->cursor;
            }
         }
         state->has_preferred_x = 0;
         break;

#ifdef STB_TEXTEDIT_K_TEXTSTART2
      case STB_TEXTEDIT_K_TEXTSTART2:
#endif
      case STB_TEXTEDIT_K_TEXTSTART:
         state->cursor = state->select_start = state->select_end = 0;
         state->has_preferred_x = 0;
         break;

#ifdef STB_TEXTEDIT_K_TEXTEND2
      case STB_TEXTEDIT_K_TEXTEND2:
#endif
      case STB_TEXTEDIT_K_TEXTEND:
         state->cursor = STB_TEXTEDIT_STRINGLEN(str);
         state->select_start = state->select_end = 0;
         state->has_preferred_x = 0;
         break;

#ifdef STB_TEXTEDIT_K_TEXTSTART2
      case STB_TEXTEDIT_K_TEXTSTART2 | STB_TEXTEDIT_K_SHIFT:
#endif
      case STB_TEXTEDIT_K_TEXTSTART | STB_TEXTEDIT_K_SHIFT:
         stb_textedit_prep_selection_at_cursor(state);
         state->cursor = state->select_end = 0;
         state->has_preferred_x = 0;
         break;

#ifdef STB_TEXTEDIT_K_TEXTEND2
      case STB_TEXTEDIT_K_TEXTEND2 | STB_TEXTEDIT_K_SHIFT:
#endif
      case STB_TEXTEDIT_K_TEXTEND | STB_TEXTEDIT_K_SHIFT:
         stb_textedit_prep_selection_at_cursor(state);
         state->cursor = state->select_end = STB_TEXTEDIT_STRINGLEN(str);
         state->has_preferred_x = 0;
         break;


#ifdef STB_TEXTEDIT_K_LINESTART2
      case STB_TEXTEDIT_K_LINESTART2:
#endif
      case STB_TEXTEDIT_K_LINESTART:
         stb_textedit_clamp(str, state);
         stb_textedit_move_to_first(state);
         if (state->single_line)
            state->cursor = 0;
         else while (state->cursor > 0 && STB_TEXTEDIT_GETCHAR(str, state->cursor-1) != STB_TEXTEDIT_NEWLINE)
            --state->cursor;
         state->has_preferred_x = 0;
         break;

#ifdef STB_TEXTEDIT_K_LINEEND2
      case STB_TEXTEDIT_K_LINEEND2:
#endif
      case STB_TEXTEDIT_K_LINEEND: {
         int n = STB_TEXTEDIT_STRINGLEN(str);
         stb_textedit_clamp(str, state);
         stb_textedit_move_to_first(state);
         if (state->single_line)
             state->cursor = n;
         else while (state->cursor < n && STB_TEXTEDIT_GETCHAR(str, state->cursor) != STB_TEXTEDIT_NEWLINE)
             ++state->cursor;
         state->has_preferred_x = 0;
         break;
      }

#ifdef STB_TEXTEDIT_K_LINESTART2
      case STB_TEXTEDIT_K_LINESTART2 | STB_TEXTEDIT_K_SHIFT:
#endif
      case STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT:
         stb_textedit_clamp(str, state);
         stb_textedit_prep_selection_at_cursor(state);
         if (state->single_line)
            state->cursor = 0;
         else while (state->cursor > 0 && STB_TEXTEDIT_GETCHAR(str, state->cursor-1) != STB_TEXTEDIT_NEWLINE)
            --state->cursor;
         state->select_end = state->cursor;
         state->has_preferred_x = 0;
         break;

#ifdef STB_TEXTEDIT_K_LINEEND2
      case STB_TEXTEDIT_K_LINEEND2 | STB_TEXTEDIT_K_SHIFT:
#endif
      case STB_TEXTEDIT_K_LINEEND | STB_TEXTEDIT_K_SHIFT: {
         int n = STB_TEXTEDIT_STRINGLEN(str);
         stb_textedit_clamp(str, state);
         stb_textedit_prep_selection_at_cursor(state);
         if (state->single_line)
             state->cursor = n;
         else while (state->cursor < n && STB_TEXTEDIT_GETCHAR(str, state->cursor) != STB_TEXTEDIT_NEWLINE)
            ++state->cursor;
         state->select_end = state->cursor;
         state->has_preferred_x = 0;
         break;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
//
//      Undo processing
//
// @OPTIMIZE: the undo/redo buffer should be circular

static void stb_textedit_flush_redo(StbUndoState *state)
{
   state->redo_point = IMSTB_TEXTEDIT_UNDOSTATECOUNT;
   state->redo_char_point = IMSTB_TEXTEDIT_UNDOCHARCOUNT;
}

// discard the oldest entry in the undo list
static void stb_textedit_discard_undo(StbUndoState *state)
{
   if (state->undo_point > 0) {
      // if the 0th undo state has characters, clean those up
      if (state->undo_rec[0].char_storage >= 0) {
         int n = state->undo_rec[0].insert_length, i;
         // delete n characters from all other records
         state->undo_char_point -= n;
         IMSTB_TEXTEDIT_memmove(state->undo_char, state->undo_char + n, (size_t) (state->undo_char_point*sizeof(IMSTB_TEXTEDIT_CHARTYPE)));
         for (i=0; i < state->undo_point; ++i)
            if (state->undo_rec[i].char_storage >= 0)
               state->undo_rec[i].char_storage -= n; // @OPTIMIZE: get rid of char_storage and infer it
      }
      --state->undo_point;
      IMSTB_TEXTEDIT_memmove(state->undo_rec, state->undo_rec+1, (size_t) (state->undo_point*sizeof(state->undo_rec[0])));
   }
}

// discard the oldest entry in the redo list--it's bad if this
// ever happens, but because undo & redo have to store the actual
// characters in different cases, the redo character buffer can
// fill up even though the undo buffer didn't
static void stb_textedit_discard_redo(StbUndoState *state)
{
   int k = IMSTB_TEXTEDIT_UNDOSTATECOUNT-1;

   if (state->redo_point <= k) {
      // if the k'th undo state has characters, clean those up
      if (state->undo_rec[k].char_storage >= 0) {
         int n = state->undo_rec[k].insert_length, i;
         // move the remaining redo character data to the end of the buffer
         state->redo_char_point += n;
         IMSTB_TEXTEDIT_memmove(state->undo_char + state->redo_char_point, state->undo_char + state->redo_char_point-n, (size_t) ((IMSTB_TEXTEDIT_UNDOCHARCOUNT - state->redo_char_point)*sizeof(IMSTB_TEXTEDIT_CHARTYPE)));
         // adjust the position of all the other records to account for above memmove
         for (i=state->redo_point; i < k; ++i)
            if (state->undo_rec[i].char_storage >= 0)
               state->undo_rec[i].char_storage += n;
      }
      // now move all the redo records towards the end of the buffer; the first one is at 'redo_point'
      // [DEAR IMGUI]
      size_t move_size = (size_t)((IMSTB_TEXTEDIT_UNDOSTATECOUNT - state->redo_point - 1) * sizeof(state->undo_rec[0]));
      const char* buf_begin = (char*)state->undo_rec; (void)buf_begin;
      const char* buf_end   = (char*)state->undo_rec + sizeof(state->undo_rec); (void)buf_end;
      IM_ASSERT(((char*)(state->undo_rec + state->redo_point)) >= buf_begin);
      IM_ASSERT(((char*)(state->undo_rec + state->redo_point + 1) + move_size) <= buf_end);
      IMSTB_TEXTEDIT_memmove(state->undo_rec + state->redo_point+1, state->undo_rec + state->redo_point, move_size);

      // now move redo_point to point to the new one
      ++state->redo_point;
   }
}

static StbUndoRecord *stb_text_create_undo_record(StbUndoState *state, int numchars)
{
   // any time we create a new undo record, we discard redo
   stb_textedit_flush_redo(state);

   // if we have no free records, we have to make room, by sliding the
   // existing records down
   if (state->undo_point == IMSTB_TEXTEDIT_UNDOSTATECOUNT)
      stb_textedit_discard_undo(state);

   // if the characters to store won't possibly fit in the buffer, we can't undo
   if (numchars > IMSTB_TEXTEDIT_UNDOCHARCOUNT) {
      state->undo_point = 0;
      state->undo_char_point = 0;
      return NULL;
   }

   // if we don't have enough free characters in the buffer, we have to make room
   while (state->undo_char_point + numchars > IMSTB_TEXTEDIT_UNDOCHARCOUNT)
      stb_textedit_discard_undo(state);

   return &state->undo_rec[state->undo_point++];
}

static IMSTB_TEXTEDIT_CHARTYPE *stb_text_createundo(StbUndoState *state, int pos, int insert_len, int delete_len)
{
   StbUndoRecord *r = stb_text_create_undo_record(state, insert_len);
   if (r == NULL)
      return NULL;

   r->where = pos;
   r->insert_length = (IMSTB_TEXTEDIT_POSITIONTYPE) insert_len;
   r->delete_length = (IMSTB_TEXTEDIT_POSITIONTYPE) delete_len;

   if (insert_len == 0) {
      r->char_storage = -1;
      return NULL;
   } else {
      r->char_storage = state->undo_char_point;
      state->undo_char_point += insert_len;
      return &state->undo_char[r->char_storage];
   }
}

static void stb_text_undo(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state)
{
   StbUndoState *s = &state->undostate;
   StbUndoRecord u, *r;
   if (s->undo_point == 0)
      return;

   // we need to do two things: apply the undo record, and create a redo record
   u = s->undo_rec[s->undo_point-1];
   r = &s->undo_rec[s->redo_point-1];
   r->char_storage = -1;

   r->insert_length = u.delete_length;
   r->delete_length = u.insert_length;
   r->where = u.where;

   if (u.delete_length) {
      // if the undo record says to delete characters, then the redo record will
      // need to re-insert the characters that get deleted, so we need to store
      // them.

      // there are three cases:
      //    there's enough room to store the characters
      //    characters stored for *redoing* don't leave room for redo
      //    characters stored for *undoing* don't leave room for redo
      // if the last is true, we have to bail

      if (s->undo_char_point + u.delete_length >= IMSTB_TEXTEDIT_UNDOCHARCOUNT) {
         // the undo records take up too much character space; there's no space to store the redo characters
         r->insert_length = 0;
      } else {
         int i;

         // there's definitely room to store the characters eventually
         while (s->undo_char_point + u.delete_length > s->redo_char_point) {
            // should never happen:
            if (s->redo_point == IMSTB_TEXTEDIT_UNDOSTATECOUNT)
               return;
            // there's currently not enough room, so discard a redo record
            stb_textedit_discard_redo(s);
         }
         r = &s->undo_rec[s->redo_point-1];

         r->char_storage = s->redo_char_point - u.delete_length;
         s->redo_char_point = s->redo_char_point - u.delete_length;

         // now save the characters
         for (i=0; i < u.delete_length; ++i)
            s->undo_char[r->char_storage + i] = STB_TEXTEDIT_GETCHAR(str, u.where + i);
      }

      // now we can carry out the deletion
      STB_TEXTEDIT_DELETECHARS(str, u.where, u.delete_length);
   }

   // check type of recorded action:
   if (u.insert_length) {
      // easy case: was a deletion, so we need to insert n characters
      STB_TEXTEDIT_INSERTCHARS(str, u.where, &s->undo_char[u.char_storage], u.insert_length);
      s->undo_char_point -= u.insert_length;
   }

   state->cursor = u.where + u.insert_length;

   s->undo_point--;
   s->redo_point--;
}

static void stb_text_redo(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state)
{
   StbUndoState *s = &state->undostate;
   StbUndoRecord *u, r;
   if (s->redo_point == IMSTB_TEXTEDIT_UNDOSTATECOUNT)
      return;

   // we need to do two things: apply the redo record, and create an undo record
   u = &s->undo_rec[s->undo_point];
   r = s->undo_rec[s->redo_point];

   // we KNOW there must be room for the undo record, because the redo record
   // was derived from an undo record

   u->delete_length = r.insert_length;
   u->insert_length = r.delete_length;
   u->where = r.where;
   u->char_storage = -1;

   if (r.delete_length) {
      // the redo record requires us to delete characters, so the undo record
      // needs to store the characters

      if (s->undo_char_point + u->insert_length > s->redo_char_point) {
         u->insert_length = 0;
         u->delete_length = 0;
      } else {
         int i;
         u->char_storage = s->undo_char_point;
         s->undo_char_point = s->undo_char_point + u->insert_length;

         // now save the characters
         for (i=0; i < u->insert_length; ++i)
            s->undo_char[u->char_storage + i] = STB_TEXTEDIT_GETCHAR(str, u->where + i);
      }

      STB_TEXTEDIT_DELETECHARS(str, r.where, r.delete_length);
   }

   if (r.insert_length) {
      // easy case: need to insert n characters
      STB_TEXTEDIT_INSERTCHARS(str, r.where, &s->undo_char[r.char_storage], r.insert_length);
      s->redo_char_point += r.insert_length;
   }

   state->cursor = r.where + r.insert_length;

   s->undo_point++;
   s->redo_point++;
}

static void stb_text_makeundo_insert(STB_TexteditState *state, int where, int length)
{
   stb_text_createundo(&state->undostate, where, 0, length);
}

static void stb_text_makeundo_delete(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state, int where, int length)
{
   int i;
   IMSTB_TEXTEDIT_CHARTYPE *p = stb_text_createundo(&state->undostate, where, length, 0);
   if (p) {
      for (i=0; i < length; ++i)
         p[i] = STB_TEXTEDIT_GETCHAR(str, where+i);
   }
}

static void stb_text_makeundo_replace(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state, int where, int old_length, int new_length)
{
   int i;
   IMSTB_TEXTEDIT_CHARTYPE *p = stb_text_createundo(&state->undostate, where, old_length, new_length);
   if (p) {
      for (i=0; i < old_length; ++i)
         p[i] = STB_TEXTEDIT_GETCHAR(str, where+i);
   }
}

// reset the state to default
static void stb_textedit_clear_state(STB_TexteditState *state, int is_single_line)
{
   state->undostate.undo_point = 0;
   state->undostate.undo_char_point = 0;
   state->undostate.redo_point = IMSTB_TEXTEDIT_UNDOSTATECOUNT;
   state->undostate.redo_char_point = IMSTB_TEXTEDIT_UNDOCHARCOUNT;
   state->select_end = state->select_start = 0;
   state->cursor = 0;
   state->has_preferred_x = 0;
   state->preferred_x = 0;
   state->cursor_at_end_of_line = 0;
   state->initialized = 1;
   state->single_line = (unsigned char) is_single_line;
   state->insert_mode = 0;
   state->row_count_per_page = 0;
}

// API initialize
static void stb_textedit_initialize_state(STB_TexteditState *state, int is_single_line)
{
   stb_textedit_clear_state(state, is_single_line);
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif

static int stb_textedit_paste(IMSTB_TEXTEDIT_STRING *str, STB_TexteditState *state, IMSTB_TEXTEDIT_CHARTYPE const *ctext, int len)
{
   return stb_textedit_paste_internal(str, state, (IMSTB_TEXTEDIT_CHARTYPE *) ctext, len);
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif//IMSTB_TEXTEDIT_IMPLEMENTATION

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
