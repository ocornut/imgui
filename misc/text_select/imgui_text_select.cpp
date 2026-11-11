// dear imgui: text selection extension
// See imgui_text_select.h

#include "imgui_text_select.h"
#include "imgui_internal.h"
#include <string.h>

struct ImGuiTextSelectFragment
{
    float       Y0, Y1, X0, X1;     // In the container's local coordinates
    ImVec2      ScreenPos;
    float       SpaceWidth;
    const char* SoftWrapJoin;   // Starts a line produced by word wrapping, not by '\n': copied after the previous line (static string). NULL otherwise
    bool        Visible;
    int         Order;          // Submission order, to keep sorting stable
    int         TextOffset;
    int         TextLen;
};

struct ImGuiTextSelectRegion
{
    ImGuiWindow*    Window;
    float           StartY;
};

struct ImGuiTextSelectDisabledRect
{
    ImGuiWindow*    RootWindow;
    ImRect          Rect;
};

// One end of a selection: a point (Begin == End) or a word on one line, in local coordinates (TextSelectScreenToLocal()).
// The anchor's window and table are the container: both ends and every selected glyph stay inside it.
struct ImGuiTextSelectRange
{
    ImGuiID         WindowId = 0;
    ImVec2          Begin, End;
    ImGuiID         TableId = 0;
    bool            MenuBar = false;
    ImGuiWindow*    Window = NULL;  // Resolved each frame
};

// A glyph edge to draw a caret or bookend at, at the end of the frame once every text run has been seen.
struct ImGuiTextSelectMark
{
    bool            Valid = false;
    float           X0, X1, Y0, Y1;
    ImDrawList*     DrawList;
    ImVec4          ClipRect;
};

struct ImGuiTextSelectState
{
    ImGuiContext*   Ctx = NULL;
    bool            Enabled = true;
    ImGuiTextSelectCopyFeedbackCallback CopyFeedback = NULL;
    void*           CopyFeedbackUserData = NULL;
    ImGuiKeyChord   SelectAllChords[2] = { ImGuiMod_Ctrl | ImGuiKey_A, 0 };
    ImGuiKeyChord   CopyChords[3] = { ImGuiMod_Ctrl | ImGuiKey_C, ImGuiMod_Ctrl | ImGuiKey_Insert, 0 };
    double          FlashStartTime = -1.0;  // Built-in copy feedback: the highlight turns opaque and fades back
    bool            Delimiters[128] = {};

    ImGuiTextSelectRange Anchor;
    ImGuiTextSelectRange Focus;
    bool            Block = false;
    int             DragUnit = 0;   // 0: point, 1: word (double-click), 2: line (triple-click)
    bool            Dragging = false;
    ImGuiID         DragId = 0;
    ImVec2          ScrollAccum;
    ImVec2          ScrollTime;

    // Resolved at NewFrame
    ImGuiWindow*    RootWindow = NULL;
    float           LineTolerance = 0.0f;
    bool            Active = false;     // Non-empty selection
    bool            WantCaret = false;  // Anchor without a selection, in the focused root window
    bool            Copying = false;
    bool            OwnsLog = false;            // The copy frame started imgui's logging (LogToBuffer()) and finishes it
    bool            BoxSelectUnclip = false;
    ImGuiBoxSelectState BoxSelectBackup;
    bool            ClickPending = false;
    bool            ClickOnExcludedText = false;
    bool            AltClick = false;   // Alt+Click: taken before widgets see it, so it always selects
    bool            AltMouseLocked = false;

    // Word/line lookup under the mouse, on click and drag frames
    bool            Probe = false;
    ImGuiWindow*    ProbeWindow = NULL;
    ImGuiTextSelectRange ProbeContainer;
    ImVec2          ProbePos;
    bool            ProbeLineFound = false;
    float           ProbeLineY0 = 0.0f, ProbeLineY1 = 0.0f;
    bool            ProbeWordFound = false;
    float           ProbeWordX0 = 0.0f, ProbeWordX1 = 0.0f;
    bool            ProbeLineExtentFound = false;   // Non-blank glyphs on the mouse's line, across all runs in the window
    float           ProbeLineX0 = 0.0f, ProbeLineX1 = 0.0f;
    float           ProbeTextDistEm = FLT_MAX;
    bool            ProbeGlyphHit = false;      // The mouse is over a selectable glyph
    ImGuiID         ProbeGlyphItemId = 0;       // Last item when that glyph was drawn, e.g. the Selectable owning its label
    bool            ProbeGlyphStealable = false;  // That glyph belongs to a TreeNode or row-spanning Selectable: a drag on it may select
    bool            ProbeUnitHit = false;       // The mouse is over a TextSelectAddText() unit: a point in it snaps to its nearest edge
    float           ProbeUnitX0 = 0.0f, ProbeUnitX1 = 0.0f;

    // A click on a stealable widget's text: a drag past the threshold turns it into a selection, unless the widget used the drag
    bool            StealPending = false;
    ImGuiID         StealItemId = 0;
    bool            StealExtend = false;
    ImGuiTextSelectRange StealRange;

    ImGuiTextSelectMark Caret;
    float               CaretDist = FLT_MAX;
    ImGuiTextSelectMark First;
    ImGuiTextSelectMark Last;

    ImVector<ImGuiTextSelectRegion>         Regions;
    ImVector<ImGuiTextSelectDisabledRect>   DisabledRects;
    ImVector<ImGuiTextSelectDisabledRect>   PrevDisabledRects;

    ImGuiWindow*    NextCopyTextWindow = NULL;
    bool            CopyRequested = false;      // TextSelectSelectAndCopy(): copy on the next frame without a key press
    bool            SelectionSetByCall = false; // TextSelectSelectAndCopy() this frame: the widget click that called it keeps the selection
    ImGuiTextBuffer RequestedCopyText;          // Replaces the copied text when not empty
    ImGuiWindow*    SoftWrapWindow = NULL;
    const char*     SoftWrapJoin = " ";
    ImGuiTextBuffer NextCopyText;

    ImVector<ImGuiTextSelectFragment>   Fragments;
    ImGuiTextBuffer                     FragmentText;
    ImGuiTextBuffer                     CopyText;
    ImVector<int>                       CopyLineOffsets;
    ImVector<ImGuiTextSelectCopyLine>   CopyLines;
};

static void TextSelectHook(ImDrawList* draw_list, ImFont* font, float font_size, const ImVec2& pos, const ImVec4& clip_rect, const char* text_begin, const char* text_end, float wrap_width, void* user_data);

static ImGuiTextSelectState* TextSelectGetState()
{
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (ctx == NULL || ctx->DrawListSharedData.TextHook != TextSelectHook)
        return NULL;
    return (ImGuiTextSelectState*)ctx->DrawListSharedData.TextHookUserData;
}

static void TextSelectSetDelimiters(ImGuiTextSelectState* st, const char* delimiters)
{
    memset(st->Delimiters, 0, sizeof(st->Delimiters));
    for (const char* p = delimiters; *p; p++)
        if ((unsigned char)*p < 128)
            st->Delimiters[(unsigned char)*p] = true;
}

// 0: blank, 1: word character, 2: delimiter. A run of the same class is one word.
static int TextSelectCharClass(const ImGuiTextSelectState* st, unsigned int c)
{
    if (c <= ' ')
        return 0;
    return (c < 128 && st->Delimiters[c]) ? 2 : 1;
}

static void TextSelectClearState(ImGuiTextSelectState* st)
{
    st->Anchor = ImGuiTextSelectRange();
    st->Focus = ImGuiTextSelectRange();
    st->Block = false;
    st->RootWindow = NULL;
    st->Active = st->WantCaret = false;
}

// Local = screen - Pos + Scroll, without the scroll in the menu bar and on an axis inside a table's frozen rows or columns
// (the window's DecoInnerSize): frozen rows then come before every scrolling row, frozen columns before every scrolling column.
static ImVec2 TextSelectScreenToLocal(const ImGuiWindow* window, const ImVec2& p, bool menu_bar)
{
    const float x = p.x - window->Pos.x, y = p.y - window->Pos.y;
    if (menu_bar)
        return ImVec2(x, y);
    return ImVec2(x >= window->DecoInnerSizeX1 ? x + window->Scroll.x : x, y >= window->DecoInnerSizeY1 ? y + window->Scroll.y : y);
}

static ImVec2 TextSelectLocalToScreen(const ImGuiWindow* window, const ImVec2& l, bool frozen_x, bool frozen_y)
{
    return ImVec2(l.x + window->Pos.x - (frozen_x ? 0.0f : window->Scroll.x), l.y + window->Pos.y - (frozen_y ? 0.0f : window->Scroll.y));
}

static ImVec2 TextSelectLocalToScreen(const ImGuiWindow* window, const ImVec2& l, bool menu_bar)
{
    return TextSelectLocalToScreen(window, l, menu_bar || l.x < window->DecoInnerSizeX1, menu_bar || l.y < window->DecoInnerSizeY1);
}

static bool TextSelectInMenuBar(const ImGuiWindow* window, const ImVec2& pos)
{
    return (window->Flags & ImGuiWindowFlags_MenuBar) != 0 && window->MenuBarRect().Contains(pos);
}

static void TextSelectFrozenText(ImGuiContext& g, const ImGuiWindow* window, bool* frozen_x, bool* frozen_y)
{
    const ImGuiTable* table = g.CurrentTable;
    *frozen_x = *frozen_y = window->DC.MenuBarAppending;
    if (*frozen_x || table == NULL || table->InnerWindow != window)
        return;
    *frozen_y = !table->IsUnfrozenRows;
    *frozen_x = table->CurrentColumn >= 0 && table->Columns[table->CurrentColumn].DisplayOrder < table->FreezeColumnsCount;
}

static bool TextSelectInDisabledRect(const ImVector<ImGuiTextSelectDisabledRect>& rects, const ImGuiWindow* hovered, const ImVec2& pos)
{
    if (hovered == NULL)
        return false;
    for (const ImGuiTextSelectDisabledRect& r : rects)
        if (r.RootWindow == hovered->RootWindow && r.Rect.Contains(pos))
            return true;
    return false;
}

static bool TextSelectOnTitleBar(const ImGuiWindow* window, const ImVec2& pos)
{
    const ImGuiWindow* root = window->RootWindow;
    return (root->Flags & ImGuiWindowFlags_NoTitleBar) == 0 && root->TitleBarRect().Contains(pos);
}

// Reading order. Points within LineTolerance vertically are on the same line.
static bool TextSelectPointLess(const ImGuiTextSelectState* st, const ImVec2& a, const ImVec2& b)
{
    if (ImFabs(a.y - b.y) < st->LineTolerance)
        return a.x < b.x;
    return a.y < b.y;
}

// In local coordinates
static void TextSelectGetRange(const ImGuiTextSelectState* st, ImVec2* out_begin, ImVec2* out_end)
{
    const ImVec2 a0 = st->Anchor.Begin, a1 = st->Anchor.End, f0 = st->Focus.Begin, f1 = st->Focus.End;
    if (st->Block)
    {
        *out_begin = ImMin(ImMin(a0, a1), ImMin(f0, f1));
        *out_end = ImMax(ImMax(a0, a1), ImMax(f0, f1));
        return;
    }
    *out_begin = TextSelectPointLess(st, f0, a0) ? f0 : a0;
    *out_end = TextSelectPointLess(st, a1, f1) ? f1 : a1;
}

// Text in a frozen row or column does not follow the scroll, so it sees the range at a different screen position
static void TextSelectGetScreenRange(const ImGuiTextSelectState* st, const ImGuiWindow* window, ImVec2* out_begin, ImVec2* out_end)
{
    ImVec2 begin, end;
    TextSelectGetRange(st, &begin, &end);
    const ImGuiWindow* container = st->Anchor.Window;
    if (window != container)
    {
        *out_begin = TextSelectLocalToScreen(container, begin, st->Anchor.MenuBar);
        *out_end = TextSelectLocalToScreen(container, end, st->Anchor.MenuBar);
        return;
    }
    bool frozen_x, frozen_y;
    TextSelectFrozenText(*st->Ctx, window, &frozen_x, &frozen_y);
    *out_begin = TextSelectLocalToScreen(container, begin, frozen_x, frozen_y);
    *out_end = TextSelectLocalToScreen(container, end, frozen_x, frozen_y);
}

static ImVec2 TextSelectToLocalOffset(const ImGuiTextSelectState* st, const ImGuiWindow* window, const ImVec2& pos)
{
    const ImGuiWindow* container = st->Anchor.Window;
    if (window != container)
    {
        const ImVec2 local = TextSelectScreenToLocal(container, pos, st->Anchor.MenuBar);
        return ImVec2(local.x - pos.x, local.y - pos.y);
    }
    bool frozen_x, frozen_y;
    TextSelectFrozenText(*st->Ctx, window, &frozen_x, &frozen_y);
    return ImVec2((frozen_x ? 0.0f : window->Scroll.x) - window->Pos.x, (frozen_y ? 0.0f : window->Scroll.y) - window->Pos.y);
}

static ImGuiTable* TextSelectFindTableAt(ImGuiContext& g, const ImGuiWindow* window, const ImVec2& pos)
{
    ImGuiTable* best = NULL;
    for (int n = 0; n < g.Tables.GetMapSize(); n++)
        if (ImGuiTable* table = g.Tables.TryGetMapData(n))
            if (table->InnerWindow == window && table->LastFrameActive == g.FrameCount && table->OuterRect.Contains(pos) && (best == NULL || table->OuterRect.GetArea() < best->OuterRect.GetArea()))
                best = table;
    return best;
}

static void TextSelectSetContainerTable(ImGuiContext& g, ImGuiTextSelectRange* r, const ImVec2& pos)
{
    ImGuiTable* table = TextSelectFindTableAt(g, r->Window, pos);
    r->TableId = table != NULL ? table->ID : 0;
}

static ImRect TextSelectContainerRect(const ImGuiTextSelectRange& container)
{
    if (container.MenuBar)
        return container.Window->MenuBarRect();
    ImRect r = container.Window->InnerClipRect;
    const ImGuiTable* table = container.TableId != 0 ? ImGui::TableFindByID(container.TableId) : NULL;
    if (table == NULL)
        return r;
    r.ClipWithFull(table->OuterRect);
    return r;
}

static ImVec2 TextSelectClampToContainer(const ImGuiTextSelectRange& container, const ImVec2& pos)
{
    const ImRect r = TextSelectContainerRect(container);
    return ImVec2(ImClamp(pos.x, r.Min.x, ImMax(r.Min.x, r.Max.x - 1.0f)), ImClamp(pos.y, r.Min.y, ImMax(r.Min.y, r.Max.y - 1.0f)));
}

static bool TextSelectInContainer(ImGuiContext& g, const ImGuiTextSelectRange& container, ImGuiWindow* window)
{
    if (container.Window == NULL || !ImGui::IsWindowChildOf(window, container.Window, false, false))
        return false;
    if (window == container.Window && window->DC.MenuBarAppending != container.MenuBar)
        return false;
    const ImGuiTable* current = g.CurrentTable;
    if (current != NULL && current->InnerWindow == window && current->CurrentColumn >= 0 && !current->Columns[current->CurrentColumn].IsEnabled)
        return false;
    if (container.TableId == 0)
        return true;
    for (int n = g.TablesTempDataStacked - 1; n >= 0; n--)
    {
        const ImGuiTable* table = g.Tables.GetByIndex(g.TablesTempData[n].TableIndex);
        if (table->ID == container.TableId)
            return true;
    }
    return false;
}

// A glyph is selected when it lies after the begin point and before the end point in reading order.
// The line containing a point (y0 <= point.y < y1) compares the glyph center against the point's x.
// Child windows inside the container take part only when their rectangle overlaps the rectangle spanned by the two
// selection ends, so a selection inside one of two side-by-side children leaves the other one out.
static bool TextSelectChildrenOverlap(const ImGuiWindow* window, const ImGuiWindow* container, const ImVec2& sel_begin, const ImVec2& sel_end)
{
    const ImRect sel(ImMin(sel_begin.x, sel_end.x) - 1.0f, ImMin(sel_begin.y, sel_end.y) - 1.0f, ImMax(sel_begin.x, sel_end.x) + 1.0f, ImMax(sel_begin.y, sel_end.y) + 1.0f);
    for (const ImGuiWindow* w = window; w != NULL && w != container; w = w->ParentWindow)
        if (!w->OuterRectClipped.Overlaps(sel))
            return false;
    return true;
}

static bool TextSelectIsSelected(bool block, const ImVec2& sel_begin, const ImVec2& sel_end, float center_x, float y0, float y1)
{
    if (block)
        return center_x >= sel_begin.x && center_x < sel_end.x && y1 > sel_begin.y && y0 <= sel_end.y;
    const bool after_begin = (y1 <= sel_begin.y) ? false : (y0 > sel_begin.y || center_x >= sel_begin.x);
    const bool before_end = (y0 > sel_end.y) ? false : (y1 <= sel_end.y || center_x < sel_end.x);
    return after_begin && before_end;
}

static ImU32 TextSelectHighlightColor(ImGuiTextSelectState* st)
{
    ImGuiContext& g = *st->Ctx;
    ImVec4 col = g.Style.Colors[ImGuiCol_TextSelectedBg];
    if (st->FlashStartTime >= 0.0)
    {
        const float t = (float)((g.Time - st->FlashStartTime) / IMGUI_TEXT_SELECT_FLASH_DURATION);
        if (t >= 1.0f)
            st->FlashStartTime = -1.0;
        else
            col.w = ImLerp(1.0f, col.w, ImMax(t, 0.0f));
    }
    col.w *= g.Style.Alpha;
    return ImGui::ColorConvertFloat4ToU32(col);
}

static void TextSelectSetMark(ImGuiTextSelectMark* mark, ImDrawList* draw_list, const ImVec4& clip, float x0, float x1, float y0, float y1)
{
    mark->Valid = true;
    mark->X0 = x0; mark->X1 = x1; mark->Y0 = y0; mark->Y1 = y1;
    mark->DrawList = draw_list;
    mark->ClipRect = clip;
}

static void TextSelectEmit(ImGuiTextSelectState* st, ImDrawList* draw_list, const ImVec4& clip, const char* text, const char* text_end, float x0, float x1, float y0, float y1, float space_width, const char* soft_wrap_join, const ImVec2& to_local)
{
    const ImVec2 r_min(ImMax(x0, clip.x), ImMax(y0, clip.y));
    const ImVec2 r_max(ImMin(x1, clip.z), ImMin(y1, clip.w));
    const bool visible = r_min.x < r_max.x && r_min.y < r_max.y;
    if (visible)
    {
        draw_list->AddRectFilled(r_min, r_max, TextSelectHighlightColor(st));
        const ImGuiTextSelectMark& first = st->First;
        if (!first.Valid || y1 <= first.Y0 || (y0 < first.Y1 && x0 < first.X0))
            TextSelectSetMark(&st->First, draw_list, clip, x0, x1, y0, y1);
        const ImGuiTextSelectMark& last = st->Last;
        if (!last.Valid || y0 >= last.Y1 || (y1 > last.Y0 && x1 > last.X1))
            TextSelectSetMark(&st->Last, draw_list, clip, x0, x1, y0, y1);
    }
    if (!st->Copying || text == NULL)
        return;
    ImGuiTextSelectFragment f;
    f.Y0 = y0 + to_local.y; f.Y1 = y1 + to_local.y; f.X0 = x0 + to_local.x; f.X1 = x1 + to_local.x;
    f.ScreenPos = ImVec2(x0, y0);
    f.SpaceWidth = space_width;
    f.SoftWrapJoin = soft_wrap_join;
    f.Visible = visible;
    f.Order = st->Fragments.Size;
    f.TextOffset = st->FragmentText.size();
    f.TextLen = (int)(text_end - text);
    st->FragmentText.append(text, text_end);
    st->Fragments.push_back(f);
}

static void TextSelectNoteExcludedClick(ImGuiTextSelectState* st, ImFont* font, float font_size, const ImVec2& pos, const ImVec4& clip, const char* text_begin, const char* text_end, float wrap_width)
{
    ImGuiContext& g = *st->Ctx;
    if (!st->ClickPending || st->ClickOnExcludedText || g.CurrentWindow == NULL || g.CurrentWindow != g.HoveredWindow)
        return;
    const ImVec2 size = font->CalcTextSizeA(font_size, FLT_MAX, wrap_width, text_begin, text_end);
    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    bb.ClipWith(ImRect(clip.x, clip.y, clip.z, clip.w));
    if (bb.Contains(g.IO.MousePos))
        st->ClickOnExcludedText = true;
}

// Lays the run out as ImFont::RenderText() does, visiting only the lines one of the three tasks needs:
// highlight/copy (lines inside the selection), caret (the anchor's line), probe (the mouse's line).
static void TextSelectHook(ImDrawList* draw_list, ImFont* font, float font_size, const ImVec2& pos, const ImVec4& clip, const char* text_begin, const char* text_end, float wrap_width, void* user_data)
{
    ImGuiTextSelectState* st = (ImGuiTextSelectState*)user_data;
    ImGuiContext& g = *st->Ctx;
    ImGuiWindow* window = g.CurrentWindow;
    const bool has_copy_text = st->NextCopyTextWindow != NULL && st->NextCopyTextWindow == window;
    if (has_copy_text)
        st->NextCopyTextWindow = NULL;
    const char* soft_wrap_first = st->SoftWrapWindow != NULL && st->SoftWrapWindow == window ? st->SoftWrapJoin : NULL;
    if (soft_wrap_first != NULL)
        st->SoftWrapWindow = NULL;
    if (window == NULL || draw_list != window->DrawList || (TextSelectOnTitleBar(window, pos) && clip.y < window->InnerClipRect.Min.y))
        return;
    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);
    if (g.CurrentItemFlags & ImGuiItemFlags_NoTextSelect)
    {
        TextSelectNoteExcludedClick(st, font, font_size, pos, clip, text_begin, text_end, wrap_width);
        return;
    }

    bool do_select = st->Active && TextSelectInContainer(g, st->Anchor, window);
    const bool do_caret = st->WantCaret && window == st->Anchor.Window;
    const bool do_probe = st->Probe && window == st->ProbeWindow && (st->ProbeContainer.Window == NULL || TextSelectInContainer(g, st->ProbeContainer, window));
    if (!do_select && !do_caret && !do_probe)
        return;

    ImVec2 sel_begin(FLT_MAX, FLT_MAX), sel_end(-FLT_MAX, -FLT_MAX);
    if (do_select)
    {
        TextSelectGetScreenRange(st, window, &sel_begin, &sel_end);
        do_select = TextSelectChildrenOverlap(window, st->Anchor.Window, sel_begin, sel_end);
        if (!do_select && !do_caret && !do_probe)
            return;
    }
    const ImVec2 caret_pos = do_caret ? TextSelectLocalToScreen(st->Anchor.Window, st->Anchor.Begin, st->Anchor.MenuBar) : ImVec2(0.0f, 0.0f);
    const ImVec2 probe_pos = st->ProbePos;
    const ImVec2 to_local = (do_select && st->Copying) ? TextSelectToLocalOffset(st, window, pos) : ImVec2(0.0f, 0.0f);
    const bool unclipped = st->Copying && window == st->Anchor.Window;
    float copy_x0 = clip.x, copy_x1 = clip.z;
    if (unclipped)
    {
        // An edge the clip shares with the window's clip rectangle was cut by scrolling, not by the widget
        if (clip.x <= window->ClipRect.Min.x + 0.5f)
            copy_x0 = -FLT_MAX;
        if (clip.z >= window->ClipRect.Max.x - 0.5f)
            copy_x1 = FLT_MAX;
    }
    if (unclipped && st->Anchor.TableId != 0 && g.CurrentTable != NULL && g.CurrentTable->CurrentColumn >= 0)
    {
        // A column's ClipRect is its cell span cut by the visible area: an edge the clip reaches was cut by scrolling
        const ImGuiTableColumn& column = g.CurrentTable->Columns[g.CurrentTable->CurrentColumn];
        if (clip.x <= column.ClipRect.Min.x + 0.5f)
            copy_x0 = column.MinX;
        if (clip.z >= column.ClipRect.Max.x - 0.5f)
            copy_x1 = column.MaxX;
    }

    float max_y = -FLT_MAX;
    if (do_select)
        max_y = unclipped ? sel_end.y : ImMin(sel_end.y, clip.w);
    if (do_caret)
        max_y = ImMax(max_y, caret_pos.y);
    if (do_probe)
        max_y = ImMax(max_y, probe_pos.y);

    const bool snap = (draw_list->Flags & ImDrawFlags_TextNoPixelSnap) == 0;
    const float origin_x = snap ? IM_TRUNC(pos.x) : pos.x;
    float x = origin_x;
    float y = snap ? IM_TRUNC(pos.y) : pos.y;
    ImFontBaked* baked = font->GetFontBaked(font_size);
    const float scale = font_size / baked->Size;
    const float space_width = baked->GetCharAdvance((ImWchar)' ') * scale;
    const float em_width = baked->GetCharAdvance((ImWchar)'M') * scale;
    const bool word_wrap = wrap_width > 0.0f;
    bool copy_text_emitted = false;

    const char* soft_wrapped = soft_wrap_first;
    const char* span_begin = NULL;
    const char* span_end = NULL;
    float span_x0 = 0.0f, span_x1 = 0.0f;
    auto flush = [&]()
    {
        if (span_begin == NULL)
            return;
        const char* line_start = span_x0 == origin_x ? soft_wrapped : NULL;
        if (has_copy_text)
        {
            TextSelectEmit(st, draw_list, clip, copy_text_emitted ? NULL : st->NextCopyText.begin(), st->NextCopyText.end(), span_x0, span_x1, y, y + font_size, space_width, line_start, to_local);
            copy_text_emitted = true;
        }
        else
        {
            TextSelectEmit(st, draw_list, clip, span_begin, span_end, span_x0, span_x1, y, y + font_size, space_width, line_start, to_local);
        }
        span_begin = NULL;
    };

    // Per-line task flags, set at each line start
    bool line_select = false, line_caret = false, line_probe = false;
    int run_class = -1;
    float run_x0 = 0.0f;
    bool probe_hit = false;
    bool probe_run = false;
    float probe_run_x0 = 0.0f, probe_run_x1 = 0.0f;
    auto end_line = [&]()
    {
        flush();
        if (probe_run)
            st->ProbeTextDistEm = ImMin(st->ProbeTextDistEm, ImMax(ImMax(probe_run_x0 - probe_pos.x, probe_pos.x - probe_run_x1), 0.0f) / em_width);
        probe_run = false;
        if (probe_hit && !st->ProbeWordFound)
        {
            st->ProbeWordFound = true;
            st->ProbeWordX1 = x;
        }
        probe_hit = false;
        run_class = -1;
    };
    auto begin_line = [&]()
    {
        const float y1 = y + font_size;
        line_select = do_select && y1 > sel_begin.y && y <= sel_end.y && (unclipped || (y1 >= clip.y && y <= clip.w));
        line_caret = do_caret && y <= caret_pos.y && caret_pos.y < y1;
        line_probe = do_probe && y <= probe_pos.y && probe_pos.y < y1;
        if (line_probe && !st->ProbeLineFound)
        {
            st->ProbeLineFound = true;
            st->ProbeLineY0 = y;
            st->ProbeLineY1 = y1;
        }
    };

    const char* s = text_begin;
    const char* word_wrap_eol = NULL;
    begin_line();
    while (s < text_end)
    {
        if (y > max_y)
            break;
        if (word_wrap)
        {
            if (word_wrap_eol == NULL)
                word_wrap_eol = ImFontCalcWordWrapPositionEx(font, font_size, s, text_end, wrap_width - (x - origin_x));
            if (s >= word_wrap_eol)
            {
                end_line();
                x = origin_x;
                y += font_size;
                // The wrap position is also where a '\n' ends the line: that one is not a soft wrap
                const char* next_line = ImTextCalcWordWrapNextLineStart(s, text_end);
                soft_wrapped = ImMemchr(s, '\n', next_line - s) != NULL ? NULL : " ";
                word_wrap_eol = NULL;
                s = next_line;
                begin_line();
                continue;
            }
        }
        if (x == origin_x && !line_select && !line_caret && !line_probe)
        {
            if (word_wrap)
            {
                s = word_wrap_eol;
            }
            else
            {
                const char* line_end = (const char*)ImMemchr(s, '\n', text_end - s);
                s = line_end ? line_end + 1 : text_end;
                y += font_size;
                soft_wrapped = NULL;
                begin_line();
            }
            continue;
        }

        const char* char_begin = s;
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
            s += 1;
        else
            s += ImTextCharFromUtf8(&c, s, text_end);
        if (c == '\n')
        {
            end_line();
            x = origin_x;
            y += font_size;
            soft_wrapped = NULL;
            begin_line();
            continue;
        }
        if (c == '\r')
            continue;

        const float advance = baked->GetCharAdvance((ImWchar)c) * scale;
        if (line_select)
        {
            const bool visible_x = x + advance > copy_x0 && x < copy_x1;
            if (visible_x && TextSelectIsSelected(st->Block, sel_begin, sel_end, x + advance * 0.5f, y, y + font_size))
            {
                if (span_begin == NULL)
                {
                    span_begin = char_begin;
                    span_x0 = x;
                }
                span_end = s;
                span_x1 = x + advance;
            }
            else
            {
                flush();
            }
        }
        if (line_caret)
        {
            const float d0 = ImFabs(caret_pos.x - x);
            const float d1 = ImFabs(caret_pos.x - (x + advance));
            if (ImMin(d0, d1) < st->CaretDist)
            {
                st->CaretDist = ImMin(d0, d1);
                const float cx = d0 <= d1 ? x : x + advance;
                TextSelectSetMark(&st->Caret, draw_list, clip, cx, cx, y, y + font_size);
            }
        }
        if (line_probe && c > ' ')
        {
            st->ProbeLineX0 = st->ProbeLineExtentFound ? ImMin(st->ProbeLineX0, x) : x;
            st->ProbeLineX1 = st->ProbeLineExtentFound ? ImMax(st->ProbeLineX1, x + advance) : x + advance;
            st->ProbeLineExtentFound = true;
            probe_run_x0 = probe_run ? probe_run_x0 : x;
            probe_run_x1 = x + advance;
            probe_run = true;
        }
        if (line_probe && !st->ProbeWordFound)
        {
            const int cls = TextSelectCharClass(st, c);
            if (cls != run_class)
            {
                if (probe_hit)
                {
                    st->ProbeWordFound = true;
                    st->ProbeWordX1 = x;
                    probe_hit = false;
                }
                run_class = cls;
                run_x0 = x;
            }
            if (!st->ProbeWordFound && !probe_hit && probe_pos.x >= x && probe_pos.x < x + advance)
            {
                probe_hit = true;
                st->ProbeGlyphHit = true;
                st->ProbeGlyphItemId = g.LastItemData.ID;
                // Only TreeNode() and a row-spanning Selectable() set these: buttons, sliders, tabs and headers must keep their drags
                st->ProbeGlyphStealable = (g.LastItemData.StatusFlags & (ImGuiItemStatusFlags_HasDisplayRect | ImGuiItemStatusFlags_HasClipRect)) != 0;
                st->ProbeWordX0 = run_x0;
                st->ProbeLineY0 = y;
                st->ProbeLineY1 = y + font_size;
            }
        }
        x += advance;
    }
    end_line();
}

static int IMGUI_CDECL TextSelectCompareByLine(const void* lhs, const void* rhs)
{
    const ImGuiTextSelectFragment* a = (const ImGuiTextSelectFragment*)lhs;
    const ImGuiTextSelectFragment* b = (const ImGuiTextSelectFragment*)rhs;
    const float ca = a->Y0 + a->Y1, cb = b->Y0 + b->Y1;
    if (ca != cb)
        return ca < cb ? -1 : 1;
    return a->Order - b->Order;
}

static int IMGUI_CDECL TextSelectCompareByX(const void* lhs, const void* rhs)
{
    const ImGuiTextSelectFragment* a = (const ImGuiTextSelectFragment*)lhs;
    const ImGuiTextSelectFragment* b = (const ImGuiTextSelectFragment*)rhs;
    if (a->X0 != b->X0)
        return a->X0 < b->X0 ? -1 : 1;
    return a->Order - b->Order;
}

static void TextSelectFinishCopy(ImGuiTextSelectState* st)
{
    ImVector<ImGuiTextSelectFragment>& frags = st->Fragments;
    const char* src = st->FragmentText.begin();
    ImGuiTextBuffer& out = st->CopyText;
    out.clear();
    st->CopyLineOffsets.resize(0);
    st->CopyLines.resize(0);
    if (frags.Size > 0)
        ImQsort(frags.Data, (size_t)frags.Size, sizeof(ImGuiTextSelectFragment), TextSelectCompareByLine);

    float prev_line_bottom = 0.0f;
    for (int line_start = 0; line_start < frags.Size; )
    {
        const float line_bottom = frags[line_start].Y1;
        int line_end = line_start + 1;
        while (line_end < frags.Size && (frags[line_end].Y0 + frags[line_end].Y1) * 0.5f < line_bottom)
            line_end++;
        ImQsort(frags.Data + line_start, (size_t)(line_end - line_start), sizeof(ImGuiTextSelectFragment), TextSelectCompareByX);

        const ImGuiTextSelectFragment& first = frags[line_start];
        if (line_start > 0)
        {
            if (first.SoftWrapJoin != NULL && !st->Block)
                out.append(first.SoftWrapJoin);
            else
                out.append(first.Y0 - prev_line_bottom > first.Y1 - first.Y0 ? "\n\n" : "\n");
        }
        const int line_offset = out.size();
        const ImGuiTextSelectFragment* prev = NULL;
        for (int n = line_start; n < line_end; n++)
        {
            const ImGuiTextSelectFragment& f = frags[n];
            if (prev != NULL)
            {
                // Same text drawn twice at an overlapping position (e.g. shadowed or faux-bold text)
                if (f.X0 < prev->X1 && f.TextLen == prev->TextLen && memcmp(src + f.TextOffset, src + prev->TextOffset, (size_t)f.TextLen) == 0)
                    continue;
                const float gap = f.X0 - prev->X1;
                if (gap > f.SpaceWidth * 3.0f)
                    out.append("\t");
                else if (gap > f.SpaceWidth * 0.5f)
                    out.append(" ");
            }
            out.append(src + f.TextOffset, src + f.TextOffset + f.TextLen);
            prev = &f;
        }
        while (out.size() > line_offset && (out.Buf[out.size() - 1] == ' ' || out.Buf[out.size() - 1] == '\t'))
        {
            out.Buf.pop_back();
            out.Buf.back() = 0;
        }
        st->CopyLineOffsets.push_back(line_offset);
        ImGuiTextSelectCopyLine line;
        line.Pos = first.ScreenPos;
        line.Visible = first.Visible;
        line.Text = line.TextEnd = NULL;
        st->CopyLines.push_back(line);
        prev_line_bottom = line_bottom;
        line_start = line_end;
    }
    frags.resize(0);
    st->FragmentText.clear();
    if (!st->RequestedCopyText.empty())
    {
        // The highlighted lines only place the feedback; the clipboard gets the requested text as one line
        if (st->CopyLines.Size > 1)
            st->CopyLines.resize(1);
        out.clear();
        out.append(st->RequestedCopyText.begin(), st->RequestedCopyText.end());
        st->RequestedCopyText.clear();
        st->CopyLineOffsets.resize(0);
        if (st->CopyLines.Size == 1)
            st->CopyLineOffsets.push_back(0);
    }
    if (out.empty())
        return;

    // Resolve line pointers once the buffer no longer grows
    for (int n = 0; n < st->CopyLines.Size; n++)
    {
        st->CopyLines[n].Text = out.begin() + st->CopyLineOffsets[n];
        st->CopyLines[n].TextEnd = (n + 1 < st->CopyLines.Size) ? out.begin() + st->CopyLineOffsets[n + 1] - 1 : out.end();
    }
    ImGui::SetClipboardText(out.c_str());
    if (st->CopyFeedback != NULL)
    {
        // The callback runs in EndFrame() where the current window is the implicit fallback window: writing to it
        // (e.g. via GetCurrentWindow()) would show it, and as a new viewport it would take the platform focus.
        ImGuiWindow* fallback_window = st->Ctx->CurrentWindow;
        const bool fallback_write_accessed = fallback_window ? fallback_window->WriteAccessed : false;
        st->CopyFeedback(out.c_str(), st->CopyLines.Data, st->CopyLines.Size, st->CopyFeedbackUserData);
        if (fallback_window)
            fallback_window->WriteAccessed = fallback_write_accessed;
    }
    else
        st->FlashStartTime = st->Ctx->Time;
}


static void TextSelectResolve(ImGuiTextSelectState* st)
{
    ImGuiContext& g = *st->Ctx;
    st->Active = st->WantCaret = false;
    st->RootWindow = NULL;
    if (!st->Enabled || st->Anchor.WindowId == 0)
        return;
    st->Anchor.Window = ImGui::FindWindowByID(st->Anchor.WindowId);
    st->Focus.Window = ImGui::FindWindowByID(st->Focus.WindowId);
    if (st->Anchor.Window == NULL || st->Focus.Window != st->Anchor.Window || (st->Anchor.TableId != 0 && ImGui::TableFindByID(st->Anchor.TableId) == NULL))
    {
        TextSelectClearState(st);
        return;
    }
    st->RootWindow = st->Anchor.Window->RootWindow;
    ImVec2 sel_begin, sel_end;
    TextSelectGetRange(st, &sel_begin, &sel_end);
    st->Active = sel_begin.x != sel_end.x || (!st->Block && sel_begin.y != sel_end.y);
    st->WantCaret = !st->Active && g.NavWindow != NULL && g.NavWindow->RootWindow == st->RootWindow;
}

// ImGuiListClipper and tables also submit the rows and columns inside BoxSelectState.UnclipRect
static void TextSelectBeginCopy(ImGuiTextSelectState* st)
{
    ImGuiContext& g = *st->Ctx;
    st->Copying = true;
    st->Fragments.resize(0);
    st->FragmentText.clear();
    // Log mode unclips items like g.ItemUnclipByLog alone, but also keeps off-screen child windows (e.g. scrolling
    // tables) from hiding, which a table laid out in a hidden child asserts on. Depth 0: no tree node auto-opens.
    st->OwnsLog = !g.LogEnabled;
    if (st->OwnsLog)
        ImGui::LogToBuffer(0);
    ImGuiBoxSelectState* bs = &g.BoxSelectState;
    st->BoxSelectUnclip = !bs->IsActive && !bs->IsStarting && !st->Anchor.MenuBar;
    if (!st->BoxSelectUnclip)
        return;
    st->BoxSelectBackup = *bs;
    ImVec2 sel_begin, sel_end;
    TextSelectGetRange(st, &sel_begin, &sel_end);
    bs->ID = ImHashStr("##TextSelectCopy");
    bs->IsActive = true;
    bs->Window = st->Anchor.Window;
    bs->UnclipMode = true;
    const ImGuiWindow* window = st->Anchor.Window;
    bs->UnclipRect = ImRect(-FLT_MAX, TextSelectLocalToScreen(window, sel_begin, false, false).y, FLT_MAX, TextSelectLocalToScreen(window, sel_end, false, false).y);
}

static void TextSelectEndCopy(ImGuiTextSelectState* st)
{
    ImGuiContext& g = *st->Ctx;
    st->Copying = false;
    if (st->OwnsLog)
        ImGui::LogFinish();
    st->OwnsLog = false;
    if (st->BoxSelectUnclip)
        g.BoxSelectState = st->BoxSelectBackup;
    st->BoxSelectUnclip = false;
}

static const ImGuiTextSelectRange* TextSelectExtendContainer(ImGuiTextSelectState* st)
{
    ImGuiContext& g = *st->Ctx;
    const bool has_anchor = st->RootWindow != NULL;
    if (st->ClickPending)
        return (has_anchor && g.IO.KeyShift && g.HoveredWindow != NULL && g.HoveredWindow->RootWindow == st->RootWindow) ? &st->Anchor : NULL;
    if (st->StealPending)
    {
        if (st->StealExtend && has_anchor)
            return &st->Anchor;
        st->StealRange.Window = ImGui::FindWindowByID(st->StealRange.WindowId);
        return st->StealRange.Window != NULL ? &st->StealRange : NULL;
    }
    return (st->Dragging && has_anchor) ? &st->Anchor : NULL;
}

// Selects all of the anchor's container; when a previous call already did, widens it first: a table to its window, a
// child window to its parent. A drag is clamped to its container, so only this leaves the anchor at -far_away.
static void TextSelectSelectAllInContainer(ImGuiTextSelectState* st)
{
    const float far_away = 1e8f;
    ImGuiTextSelectRange c = st->Anchor;
    if (c.Begin.x == -far_away && st->Focus.End.x == far_away && !c.MenuBar)
    {
        if (c.TableId != 0)
        {
            c.TableId = 0;
        }
        else if (c.Window != c.Window->RootWindow && c.Window->ParentWindow != NULL)
        {
            c.Window = c.Window->ParentWindow;
            c.WindowId = c.Window->ID;
        }
    }
    c.Begin = c.End = ImVec2(-far_away, -far_away);
    st->Anchor = c;
    c.Begin = c.End = ImVec2(far_away, far_away);
    st->Focus = c;
    st->Block = false;
}

template<int N>
static ImGuiKey TextSelectChordPressed(const ImGuiKeyChord (&chords)[N])
{
    for (ImGuiKeyChord chord : chords)
        if (chord != 0 && ImGui::IsKeyChordPressed(chord))
            return (ImGuiKey)(chord & ~ImGuiMod_Mask_);
    return ImGuiKey_None;
}

static void TextSelectNewFrame(ImGuiContext* ctx, ImGuiContextHook* hook)
{
    ImGuiTextSelectState* st = (ImGuiTextSelectState*)hook->UserData;
    ImGuiContext& g = *ctx;
    const ImGuiIO& io = g.IO;
    st->PrevDisabledRects.swap(st->DisabledRects);
    st->DisabledRects.resize(0);
    st->Regions.resize(0);
    st->NextCopyTextWindow = NULL;
    st->SoftWrapWindow = NULL;
    st->Copying = false;
    st->LineTolerance = g.FontSize * 0.5f;
    st->Caret.Valid = st->First.Valid = st->Last.Valid = false;
    st->CaretDist = FLT_MAX;

    if (st->Dragging)
    {
        if (!io.MouseDown[ImGuiMouseButton_Left] || g.ActiveId != st->DragId)
        {
            st->Dragging = false;
            if (g.ActiveId == st->DragId)
                ImGui::ClearActiveID();
        }
        else
        {
            ImGui::KeepAliveID(st->DragId);
        }
    }

    if (st->AltMouseLocked && !io.MouseDown[ImGuiMouseButton_Left])
    {
        st->AltMouseLocked = false;
        if (io.MouseReleased[ImGuiMouseButton_Left])
            ImGui::SetKeyOwner(ImGuiKey_MouseLeft, st->DragId, ImGuiInputFlags_LockThisFrame);
    }

    st->ClickPending = st->Enabled && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    st->ClickOnExcludedText = false;
    // Holding the active id from NewFrame keeps every widget from hovering or activating (see ItemHoverable())
    st->AltClick = st->ClickPending && io.KeyAlt && g.HoveredWindow != NULL && g.ActiveId == 0 && !TextSelectOnTitleBar(g.HoveredWindow, io.MousePos);
    if (st->AltClick)
    {
        st->DragId = ImHashStr("##TextSelectDrag");
        ImGui::SetActiveID(st->DragId, g.HoveredWindow);
        ImGui::SetKeyOwner(ImGuiKey_MouseLeft, st->DragId, ImGuiInputFlags_LockUntilRelease);
        st->AltMouseLocked = true;
    }
    TextSelectResolve(st);
    const ImGuiTextSelectRange* container = TextSelectExtendContainer(st);
    st->ProbeContainer = container != NULL ? *container : ImGuiTextSelectRange();
    st->ProbeWindow = container != NULL ? container->Window : g.HoveredWindow;
    st->ProbePos = container != NULL ? TextSelectClampToContainer(*container, io.MousePos) : io.MousePos;
    st->Probe = st->Enabled && st->ProbeWindow != NULL && (st->ClickPending || st->Dragging || st->StealPending);
    st->ProbeLineFound = st->ProbeWordFound = st->ProbeLineExtentFound = st->ProbeGlyphHit = st->ProbeGlyphStealable = st->ProbeUnitHit = false;
    st->ProbeTextDistEm = FLT_MAX;
    st->ProbeGlyphItemId = 0;

    const ImGuiKey select_all_key = TextSelectChordPressed(st->SelectAllChords);
    if (select_all_key != ImGuiKey_None && st->RootWindow != NULL && !io.WantTextInput)
    {
        const bool owns_window = (g.HoveredWindow != NULL && g.HoveredWindow->RootWindow == st->RootWindow) || (g.NavWindow != NULL && g.NavWindow->RootWindow == st->RootWindow);
        if (owns_window)
        {
            TextSelectSelectAllInContainer(st);
            TextSelectResolve(st);
            // Nobody else acts on this select-all key press
            ImGui::SetKeyOwner(select_all_key, ImGuiKeyOwner_Any, ImGuiInputFlags_LockThisFrame);
        }
    }

    if (st->CopyRequested)
    {
        st->CopyRequested = false;
        if (st->Active)
        {
            TextSelectBeginCopy(st);
            return;
        }
    }
    st->RequestedCopyText.clear();
    const ImGuiKey copy_key = TextSelectChordPressed(st->CopyChords);
    if (copy_key == ImGuiKey_None)
        return;
    if (!st->Active)
    {
        IMGUI_DEBUG_LOG_IO("[textselect] copy key: no selection (anchor %s)\n", st->Anchor.WindowId != 0 ? "set" : "none");
        return;
    }
    const bool owns_window = (g.HoveredWindow != NULL && g.HoveredWindow->RootWindow == st->RootWindow) || (g.NavWindow != NULL && g.NavWindow->RootWindow == st->RootWindow);
    const char* blocked = io.WantTextInput ? "text input active" : !owns_window ? "selection window neither hovered nor focused" : TextSelectInDisabledRect(st->PrevDisabledRects, g.HoveredWindow, io.MousePos) ? "mouse over a disabled region" : NULL;
    IMGUI_DEBUG_LOG_IO("[textselect] copy key: %s (hovered '%s', nav '%s', selection root '%s')\n", blocked ? blocked : "copying", g.HoveredWindow ? g.HoveredWindow->Name : "", g.NavWindow ? g.NavWindow->Name : "", st->RootWindow->Name);
    if (blocked == NULL)
    {
        TextSelectBeginCopy(st);
        // Nobody else acts on this copy key press
        ImGui::SetKeyOwner(copy_key, ImGuiKeyOwner_Any, ImGuiInputFlags_LockThisFrame);
    }
}

// The probed line (unit 2) or word (unit 1) under the mouse, else the mouse point, snapped vertically to the probed line.
static ImGuiTextSelectRange TextSelectRangeAtMouse(const ImGuiTextSelectState* st, ImGuiWindow* window, int unit, bool menu_bar)
{
    const ImVec2 mouse = st->ProbePos;
    const float y = st->ProbeLineFound ? (st->ProbeLineY0 + st->ProbeLineY1) * 0.5f : mouse.y;
    ImGuiTextSelectRange r;
    r.WindowId = window->ID;
    r.Window = window;
    r.MenuBar = menu_bar;
    float x0 = mouse.x, x1 = mouse.x;
    if (unit == 2 && st->ProbeLineExtentFound && window == st->ProbeWindow)
    {
        x0 = st->ProbeLineX0;
        x1 = st->ProbeLineX1;
    }
    else if (unit >= 1 && st->ProbeWordFound && window == st->ProbeWindow)
    {
        x0 = st->ProbeWordX0;
        x1 = st->ProbeWordX1;
    }
    else if (st->ProbeUnitHit && window == st->ProbeWindow)
    {
        x0 = x1 = (mouse.x - st->ProbeUnitX0 < st->ProbeUnitX1 - mouse.x) ? st->ProbeUnitX0 : st->ProbeUnitX1;
    }
    r.Begin = TextSelectScreenToLocal(window, ImVec2(x0, y), menu_bar);
    r.End = TextSelectScreenToLocal(window, ImVec2(x1, y), menu_bar);
    return r;
}

static void TextSelectDrawMark(const ImGuiTextSelectMark& mark, float x, ImU32 col, float thickness)
{
    ImDrawList* draw_list = mark.DrawList;
    draw_list->PushClipRect(ImVec2(mark.ClipRect.x, mark.ClipRect.y), ImVec2(mark.ClipRect.z, mark.ClipRect.w), true);
    draw_list->AddRectFilled(ImVec2(x - thickness * 0.5f, mark.Y0), ImVec2(x + thickness * 0.5f, mark.Y1), col);
    draw_list->PopClipRect();
}

static void TextSelectDrawMarks(ImGuiTextSelectState* st)
{
    if (!st->Caret.Valid && !st->First.Valid && !st->Last.Valid)
        return;
    ImGuiContext& g = *st->Ctx;
    ImVec4 col_v = g.Style.Colors[ImGuiCol_TextSelectedBg];
    col_v.w = 1.0f;
    const ImU32 col = ImGui::ColorConvertFloat4ToU32(col_v);
    const float thickness = ImMax(2.0f, IM_TRUNC(g.FontSize / 8.0f));
    if (st->WantCaret && st->Caret.Valid)
        TextSelectDrawMark(st->Caret, st->Caret.X0, col, thickness);
    if (!st->Active)
        return;
    // A bookend only marks a true end of the selection: its point must be within the vertical clip range
    ImVec2 sel_begin, sel_end;
    TextSelectGetRange(st, &sel_begin, &sel_end);
    sel_begin = TextSelectLocalToScreen(st->Anchor.Window, sel_begin, st->Anchor.MenuBar);
    sel_end = TextSelectLocalToScreen(st->Anchor.Window, sel_end, st->Anchor.MenuBar);
    if (st->First.Valid && sel_begin.y >= st->First.ClipRect.y && sel_begin.y <= st->First.ClipRect.w)
        TextSelectDrawMark(st->First, st->First.X0, col, thickness);
    if (st->Last.Valid && sel_end.y >= st->Last.ClipRect.y && sel_end.y <= st->Last.ClipRect.w)
        TextSelectDrawMark(st->Last, st->Last.X1, col, thickness);
}

static bool TextSelectIsFreeClick(const ImGuiTextSelectState* st, ImGuiWindow* hovered)
{
    ImGuiContext& g = *st->Ctx;
    if (hovered == NULL || st->ClickOnExcludedText || TextSelectInDisabledRect(st->DisabledRects, hovered, g.IO.MousePos))
        return false;
    if (st->AltClick)
        return true;
    // Title bars keep moving windows
    return g.ActiveId == 0 && g.HoveredId == 0 && !TextSelectOnTitleBar(hovered, g.IO.MousePos);
}

// With ImGuiWindowBgClickFlags_Move (io.ConfigWindowsMoveFromTitleBarOnly off), a drag on the background moves the window
static bool TextSelectBgDragMovesWindow(const ImGuiWindow* window)
{
    return (window->BgClickFlags & ImGuiWindowBgClickFlags_Move) != 0 && (window->RootWindow->Flags & ImGuiWindowFlags_NoMove) == 0;
}

static void TextSelectStartDrag(ImGuiTextSelectState* st, ImGuiWindow* window, int unit)
{
    st->DragUnit = unit;
    st->ScrollAccum = st->ScrollTime = ImVec2(0.0f, 0.0f);
    st->DragId = ImHashStr("##TextSelectDrag");
    st->Dragging = true;
    ImGui::SetActiveID(st->DragId, window);
    ImGui::FocusWindow(window);
}

static void TextSelectScrollWithDrag(ImGuiTextSelectState* st)
{
    ImGuiContext& g = *st->Ctx;
    ImGuiWindow* window = st->Anchor.Window;
    if (st->Anchor.MenuBar)
        return;
    const ImRect r = TextSelectContainerRect(st->Anchor);
    const ImGuiTable* table = st->Anchor.TableId != 0 ? ImGui::TableFindByID(st->Anchor.TableId) : NULL;
    const bool table_in_window = table != NULL && table->InnerWindow == table->OuterWindow;
    for (int axis = 0; axis < 2; axis++)
    {
        const float mouse = g.IO.MousePos[axis];
        const float dist = (mouse > r.Max[axis]) ? mouse - r.Max[axis] : (mouse < r.Min[axis]) ? mouse - r.Min[axis] : 0.0f;
        const float scroll = window->Scroll[axis];
        bool can_scroll = (dist < 0.0f && scroll > 0.0f) || (dist > 0.0f && scroll < window->ScrollMax[axis]);
        if (table_in_window && ((dist < 0.0f && table->OuterRect.Min[axis] >= r.Min[axis]) || (dist > 0.0f && table->OuterRect.Max[axis] <= r.Max[axis])))
            can_scroll = false;
        if (!can_scroll)
        {
            st->ScrollAccum[axis] = st->ScrollTime[axis] = 0.0f;
            continue;
        }
        // Lines per second: 8 at the edge, x4 farther out, doubling every second the mouse stays out
        st->ScrollTime[axis] += g.IO.DeltaTime;
        const float speed = ImLinearRemapClamp(g.FontSize, g.FontSize * 5.0f, 1.0f, 4.0f, ImAbs(dist)) * ImPow(2.0f, ImMin(st->ScrollTime[axis], 16.0f));
        st->ScrollAccum[axis] += g.FontSize * 8.0f * speed * ImSign(dist) * g.IO.DeltaTime;
        const float step = ImTrunc(st->ScrollAccum[axis]);
        if (step == 0.0f)
            continue;
        if (axis == 0)
            ImGui::SetScrollX(window, scroll + step);
        else
            ImGui::SetScrollY(window, scroll + step);
        st->ScrollAccum[axis] -= step;
    }
}

static bool TextSelectIsContainerScrollbar(const ImGuiTextSelectState* st, ImGuiID id)
{
    if (id == 0 || st->RootWindow == NULL)
        return false;
    for (ImGuiWindow* w = st->Anchor.Window; w != NULL; w = (w == w->RootWindow) ? NULL : w->ParentWindow)
        if (!w->MemoryCompacted && (id == ImGui::GetWindowScrollbarID(w, ImGuiAxis_X) || id == ImGui::GetWindowScrollbarID(w, ImGuiAxis_Y)))
            return true;
    return false;
}

static void TextSelectEndFrame(ImGuiContext* ctx, ImGuiContextHook* hook)
{
    ImGuiTextSelectState* st = (ImGuiTextSelectState*)hook->UserData;
    ImGuiContext& g = *ctx;
    st->Regions.resize(0);
    if (st->Copying)
    {
        TextSelectEndCopy(st);
        TextSelectFinishCopy(st);
    }
    TextSelectDrawMarks(st);
    if (!st->Enabled)
    {
        TextSelectClearState(st);
        return;
    }

    const ImGuiIO& io = g.IO;
    const bool selection_set_by_call = st->SelectionSetByCall;
    st->SelectionSetByCall = false;
    if (st->ClickPending)
    {
        // Widgets activate on click before EndFrame(), window moving starts inside EndFrame(): ActiveId == 0 means no widget took it.
        ImGuiWindow* hovered = g.HoveredWindow;
        st->StealPending = false;
        if (selection_set_by_call)
            return;
        if (!TextSelectIsFreeClick(st, hovered))
        {
            if (st->AltClick && g.ActiveId == st->DragId)
                ImGui::ClearActiveID();
            const ImGuiWindow* anchor_window = st->Anchor.WindowId != 0 ? ImGui::FindWindowByID(st->Anchor.WindowId) : NULL;
            const bool extend = io.KeyShift && hovered != NULL && anchor_window != NULL && anchor_window->RootWindow == hovered->RootWindow;
            if (!io.KeyShift && !TextSelectIsContainerScrollbar(st, g.ActiveId))
                TextSelectClearState(st);
            if (hovered != NULL && g.ActiveId != 0 && st->ProbeGlyphHit && st->ProbeGlyphItemId == g.ActiveId && st->ProbeGlyphStealable && g.InputTextState.ID != g.ActiveId)
            {
                st->StealPending = true;
                st->StealItemId = g.ActiveId;
                st->StealExtend = extend;
                st->StealRange = TextSelectRangeAtMouse(st, hovered, 0, TextSelectInMenuBar(hovered, io.MousePos));
                TextSelectSetContainerTable(g, &st->StealRange, io.MousePos);
            }
            return;
        }
        const int clicks = io.MouseClickedCount[ImGuiMouseButton_Left];
        const int unit = clicks >= 3 ? 2 : clicks == 2 ? 1 : 0;
        const bool extend = st->ProbeContainer.Window != NULL;
        ImGuiTextSelectRange at_mouse = TextSelectRangeAtMouse(st, extend ? st->ProbeContainer.Window : hovered, unit, extend ? st->ProbeContainer.MenuBar : TextSelectInMenuBar(hovered, io.MousePos));
        const bool move_window = !st->AltClick && !st->ProbeGlyphHit && st->ProbeTextDistEm > 1.0f && TextSelectBgDragMovesWindow(hovered);
        if (move_window && !extend)
        {
            TextSelectClearState(st);
            return;
        }
        if (!extend)
        {
            TextSelectSetContainerTable(g, &at_mouse, io.MousePos);
            st->Anchor = at_mouse;
        }
        st->Block = io.KeyShift && io.KeyAlt;
        st->Focus = at_mouse;

        if (!move_window)
            TextSelectStartDrag(st, hovered, unit);
    }
    else if (st->StealPending)
    {
        ImGuiWindow* window = ImGui::FindWindowByID(st->StealRange.WindowId);
        if (!io.MouseDown[ImGuiMouseButton_Left] || g.ActiveId != st->StealItemId || g.DragDropActive || g.ActiveIdHasBeenEditedBefore || window == NULL)
        {
            st->StealPending = false;
        }
        else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            st->StealPending = false;
            st->StealRange.Window = window;
            if (!st->StealExtend || st->Anchor.WindowId == 0)
                st->Anchor = st->StealRange;
            st->Block = io.KeyShift && io.KeyAlt;
            st->Focus = TextSelectRangeAtMouse(st, st->Anchor.Window, 0, st->Anchor.MenuBar);
            TextSelectStartDrag(st, window, 0);
        }
    }
    else if (st->Dragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        st->Block = io.KeyShift && io.KeyAlt;
        if (st->Anchor.Window != NULL && st->RootWindow != NULL)
        {
            st->Focus = TextSelectRangeAtMouse(st, st->Anchor.Window, st->DragUnit, st->Anchor.MenuBar);
            TextSelectScrollWithDrag(st);
        }
    }
    else if (st->Anchor.WindowId != 0 && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Escape, false))
    {
        TextSelectClearState(st);
    }
}

static void TextSelectShutdown(ImGuiContext* ctx, ImGuiContextHook* hook)
{
    ImGuiTextSelectState* st = (ImGuiTextSelectState*)hook->UserData;
    if (ctx->DrawListSharedData.TextHookUserData == st)
    {
        ctx->DrawListSharedData.TextHook = NULL;
        ctx->DrawListSharedData.TextHookUserData = NULL;
        IM_DELETE(st);
    }
}

void ImGui::TextSelectInstall()
{
    ImGuiContext* ctx = GetCurrentContext();
    if (ctx == NULL || ctx->DrawListSharedData.TextHook == TextSelectHook)
        return;
    IM_ASSERT(ctx->DrawListSharedData.TextHook == NULL && "Another text hook is installed");
    ImGuiTextSelectState* st = IM_NEW(ImGuiTextSelectState)();
    st->Ctx = ctx;
    TextSelectSetDelimiters(st, IMGUI_TEXT_SELECT_DEFAULT_DELIMITERS);
    ctx->DrawListSharedData.TextHook = TextSelectHook;
    ctx->DrawListSharedData.TextHookUserData = st;

    ImGuiContextHook hook;
    hook.UserData = st;
    hook.Type = ImGuiContextHookType_NewFramePost;
    hook.Callback = TextSelectNewFrame;
    AddContextHook(ctx, &hook);
    hook.Type = ImGuiContextHookType_EndFramePre;
    hook.Callback = TextSelectEndFrame;
    AddContextHook(ctx, &hook);
    hook.Type = ImGuiContextHookType_Shutdown;
    hook.Callback = TextSelectShutdown;
    AddContextHook(ctx, &hook);
}

void ImGui::TextSelectSetEnabled(bool enabled)
{
    if (ImGuiTextSelectState* st = TextSelectGetState())
        st->Enabled = enabled;
}

void ImGui::TextSelectSetCopyFeedbackCallback(ImGuiTextSelectCopyFeedbackCallback callback, void* user_data)
{
    if (ImGuiTextSelectState* st = TextSelectGetState())
    {
        st->CopyFeedback = callback;
        st->CopyFeedbackUserData = user_data;
    }
}

void ImGui::TextSelectSetCopyShortcut(ImGuiKeyChord key_chord, ImGuiKeyChord alt_key_chord, ImGuiKeyChord alt_key_chord2)
{
    if (ImGuiTextSelectState* st = TextSelectGetState())
    {
        st->CopyChords[0] = key_chord;
        st->CopyChords[1] = alt_key_chord;
        st->CopyChords[2] = alt_key_chord2;
    }
}

void ImGui::TextSelectSetSelectAllShortcut(ImGuiKeyChord key_chord, ImGuiKeyChord alt_key_chord)
{
    if (ImGuiTextSelectState* st = TextSelectGetState())
    {
        st->SelectAllChords[0] = key_chord;
        st->SelectAllChords[1] = alt_key_chord;
    }
}

void ImGui::TextSelectSetWordDelimiters(const char* delimiters)
{
    if (ImGuiTextSelectState* st = TextSelectGetState())
        TextSelectSetDelimiters(st, delimiters ? delimiters : IMGUI_TEXT_SELECT_DEFAULT_DELIMITERS);
}

void ImGui::TextSelectPushDisabledRegion()
{
    PushItemFlag(ImGuiItemFlags_NoTextSelect, true);
    ImGuiTextSelectState* st = TextSelectGetState();
    if (st == NULL)
        return;
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiTextSelectRegion region;
    region.Window = window;
    region.StartY = window->DC.CursorPos.y;
    st->Regions.push_back(region);
}

void ImGui::TextSelectPopDisabledRegion()
{
    ImGuiTextSelectState* st = TextSelectGetState();
    if (st != NULL && st->Regions.Size > 0)
    {
        const ImGuiTextSelectRegion region = st->Regions.back();
        st->Regions.pop_back();
        ImGuiWindow* window = region.Window;
        ImGuiTextSelectDisabledRect r;
        r.RootWindow = window->RootWindow;
        r.Rect = ImRect(window->OuterRectClipped.Min.x, region.StartY, window->OuterRectClipped.Max.x, ImMax(window->DC.CursorPos.y, window->DC.CursorMaxPos.y));
        r.Rect.ClipWith(window->InnerClipRect);
        st->DisabledRects.push_back(r);
    }
    PopItemFlag();
}

void ImGui::TextSelectSetNextCopyText(const char* text, const char* text_end)
{
    ImGuiTextSelectState* st = TextSelectGetState();
    if (st == NULL || !st->Copying)
        return;
    if (text == NULL)
    {
        st->NextCopyTextWindow = NULL;
        return;
    }
    st->NextCopyText.clear();
    st->NextCopyText.append(text, text_end);
    st->NextCopyTextWindow = st->Ctx->CurrentWindow;
}

static void TextSelectProbeUnit(ImGuiTextSelectState* st, ImGuiWindow* window, const ImVec2& p_min, const ImVec2& p_max)
{
    ImGuiContext& g = *st->Ctx;
    const ImVec4 clip = window->DrawList->_CmdHeader.ClipRect;
    ImRect bb(p_min, p_max);
    bb.ClipWith(ImRect(clip.x, clip.y, clip.z, clip.w));
    if (st->ClickPending && window == g.HoveredWindow && bb.Contains(g.IO.MousePos))
        st->ClickOnExcludedText = false;
    const bool do_probe = st->Probe && window == st->ProbeWindow && (st->ProbeContainer.Window == NULL || TextSelectInContainer(g, st->ProbeContainer, window));
    const ImVec2 pos = st->ProbePos;
    if (!do_probe || pos.y < p_min.y || pos.y >= p_max.y)
        return;
    if (!st->ProbeLineFound)
    {
        st->ProbeLineFound = true;
        st->ProbeLineY0 = p_min.y;
        st->ProbeLineY1 = p_max.y;
    }
    st->ProbeLineX0 = st->ProbeLineExtentFound ? ImMin(st->ProbeLineX0, p_min.x) : p_min.x;
    st->ProbeLineX1 = st->ProbeLineExtentFound ? ImMax(st->ProbeLineX1, p_max.x) : p_max.x;
    st->ProbeLineExtentFound = true;
    st->ProbeTextDistEm = ImMin(st->ProbeTextDistEm, ImMax(ImMax(p_min.x - pos.x, pos.x - p_max.x), 0.0f) / ImGui::CalcTextSize("M").x);
    if (!st->ProbeWordFound && pos.x >= p_min.x && pos.x < p_max.x)
    {
        st->ProbeWordFound = true;
        st->ProbeWordX0 = p_min.x;
        st->ProbeWordX1 = p_max.x;
        st->ProbeGlyphHit = true;
        st->ProbeGlyphItemId = g.LastItemData.ID;
        st->ProbeGlyphStealable = false;
        st->ProbeUnitHit = true;
        st->ProbeUnitX0 = p_min.x;
        st->ProbeUnitX1 = p_max.x;
        st->ProbeLineY0 = p_min.y;
        st->ProbeLineY1 = p_max.y;
    }
}

void ImGui::TextSelectAddText(const ImVec2& p_min, const ImVec2& p_max, const char* text, const char* text_end)
{
    ImGuiTextSelectState* st = TextSelectGetState();
    ImGuiWindow* window = st != NULL ? st->Ctx->CurrentWindow : NULL;
    if (st == NULL || window == NULL)
        return;
    TextSelectProbeUnit(st, window, p_min, p_max);
    if (st->WantCaret && window == st->Anchor.Window)
    {
        const ImVec2 caret_pos = TextSelectLocalToScreen(st->Anchor.Window, st->Anchor.Begin, st->Anchor.MenuBar);
        const float d0 = ImFabs(caret_pos.x - p_min.x);
        const float d1 = ImFabs(caret_pos.x - p_max.x);
        if (p_min.y <= caret_pos.y && caret_pos.y < p_max.y && ImMin(d0, d1) < st->CaretDist)
        {
            st->CaretDist = ImMin(d0, d1);
            const float cx = d0 <= d1 ? p_min.x : p_max.x;
            TextSelectSetMark(&st->Caret, window->DrawList, window->DrawList->_CmdHeader.ClipRect, cx, cx, p_min.y, p_max.y);
        }
    }
    if (!st->Active || !TextSelectInContainer(*st->Ctx, st->Anchor, window))
        return;
    ImVec2 sel_begin, sel_end;
    TextSelectGetScreenRange(st, window, &sel_begin, &sel_end);
    if (!TextSelectChildrenOverlap(window, st->Anchor.Window, sel_begin, sel_end) || !TextSelectIsSelected(st->Block, sel_begin, sel_end, (p_min.x + p_max.x) * 0.5f, p_min.y, p_max.y))
        return;
    if (text_end == NULL)
        text_end = text + strlen(text);
    TextSelectEmit(st, window->DrawList, window->DrawList->_CmdHeader.ClipRect, text, text_end, p_min.x, p_max.x, p_min.y, p_max.y, CalcTextSize(" ").x, NULL, TextSelectToLocalOffset(st, window, p_min));
}

void ImGui::TextSelectMarkSoftWrap(const char* join)
{
    if (ImGuiTextSelectState* st = TextSelectGetState())
    {
        st->SoftWrapWindow = st->Ctx->CurrentWindow;
        st->SoftWrapJoin = join;
    }
}

void ImGui::TextSelectSelectAndCopy(const ImVec2& p_min, const ImVec2& p_max, const char* copy_text, const char* copy_text_end)
{
    ImGuiTextSelectState* st = TextSelectGetState();
    ImGuiWindow* window = st != NULL ? st->Ctx->CurrentWindow : NULL;
    if (window == NULL || !st->Enabled)
        return;
    // Half a line in from top and bottom: an item rect includes padding that reaches into the neighbouring lines
    const float y_inset = ImMin(st->Ctx->FontSize * 0.5f, (p_max.y - p_min.y) * 0.5f);
    const bool menu_bar = window->DC.MenuBarAppending;
    st->Anchor = st->Focus = ImGuiTextSelectRange();
    st->Anchor.WindowId = st->Focus.WindowId = window->ID;
    st->Anchor.Window = st->Focus.Window = window;
    st->Anchor.MenuBar = menu_bar;
    st->Anchor.Begin = st->Anchor.End = TextSelectScreenToLocal(window, ImVec2(p_min.x, p_min.y + y_inset), menu_bar);
    st->Focus.Begin = st->Focus.End = TextSelectScreenToLocal(window, ImVec2(p_max.x, p_max.y - y_inset), menu_bar);
    st->Block = false;
    st->CopyRequested = true;
    st->SelectionSetByCall = true;
    st->RequestedCopyText.clear();
    if (copy_text != NULL)
        st->RequestedCopyText.append(copy_text, copy_text_end);
}

void ImGui::TextSelectSelectAll()
{
    ImGuiTextSelectState* st = TextSelectGetState();
    ImGuiWindow* window = st != NULL ? st->Ctx->CurrentWindow : NULL;
    if (window == NULL)
        return;
    const float far_away = 1e8f;
    st->Anchor = st->Focus = ImGuiTextSelectRange();
    st->Anchor.WindowId = st->Focus.WindowId = window->ID;
    st->Anchor.Window = st->Focus.Window = window;
    st->Anchor.Begin = st->Anchor.End = ImVec2(0.0f, 0.0f);
    st->Focus.Begin = st->Focus.End = ImVec2(far_away, far_away);
    st->Block = false;
}

bool ImGui::TextSelectHasSelection(bool current_window_only)
{
    ImGuiTextSelectState* st = TextSelectGetState();
    if (st == NULL || !st->Active)
        return false;
    const ImGuiWindow* window = st->Ctx->CurrentWindow;
    return !current_window_only || (window != NULL && window->RootWindow == st->RootWindow);
}

bool ImGui::TextSelectIsCopying()
{
    ImGuiTextSelectState* st = TextSelectGetState();
    return st != NULL && st->Copying;
}

void ImGui::TextSelectClear()
{
    if (ImGuiTextSelectState* st = TextSelectGetState())
        TextSelectClearState(st);
}
