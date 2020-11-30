// dear imgui, v1.80 WIP
// (tables and columns code)

/*
 *
 * Index of this file:
 *
 * // [SECTION] Widgets: BeginTable, EndTable, etc.
 * // [SECTION] Widgets: Columns, BeginColumns, EndColumns, etc.
 *
 */

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)     // condition expression is constant
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#if defined(_MSC_VER) && _MSC_VER >= 1922 // MSVC 2019 16.2 or later
#pragma warning (disable: 5054)     // operator '|': deprecated between enumerations of different types
#endif
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                      // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"                    // warning: comparing floating point with == or != is unsafe // storing and comparing against same constants (typically 0.0f) is ok.
#pragma clang diagnostic ignored "-Wsign-conversion"                // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                    // some standard header variations use #define NULL 0
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wenum-enum-conversion"           // warning: bitwise operation between different enumeration types ('XXXFlags_' and 'XXXFlagsPrivate_')
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"// warning: bitwise operation between different enumeration types ('XXXFlags_' and 'XXXFlagsPrivate_') is deprecated
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                  // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wclass-memaccess"          // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif


//-----------------------------------------------------------------------------
// [SECTION] Widgets: BeginTable, EndTable, etc.
//-----------------------------------------------------------------------------


//-------------------------------------------------------------------------
// [SECTION] Widgets: Columns, BeginColumns, EndColumns, etc.
// (This is a legacy API, prefer using BeginTable/EndTable!)
//-------------------------------------------------------------------------
// - SetWindowClipRectBeforeSetChannel() [Internal]
// - GetColumnIndex()
// - GetColumnsCount()
// - GetColumnOffset()
// - GetColumnWidth()
// - SetColumnOffset()
// - SetColumnWidth()
// - PushColumnClipRect() [Internal]
// - PushColumnsBackground() [Internal]
// - PopColumnsBackground() [Internal]
// - FindOrCreateColumns() [Internal]
// - GetColumnsID() [Internal]
// - BeginColumns()
// - NextColumn()
// - EndColumns()
// - Columns()
//-------------------------------------------------------------------------

// [Internal] Small optimization to avoid calls to PopClipRect/SetCurrentChannel/PushClipRect in sequences,
// they would meddle many times with the underlying ImDrawCmd.
// Instead, we do a preemptive overwrite of clipping rectangle _without_ altering the command-buffer and let
// the subsequent single call to SetCurrentChannel() does it things once.
void ImGui::SetWindowClipRectBeforeSetChannel(ImGuiWindow* window, const ImRect& clip_rect)
{
    ImVec4 clip_rect_vec4 = clip_rect.ToVec4();
    window->ClipRect = clip_rect;
    window->DrawList->_CmdHeader.ClipRect = clip_rect_vec4;
    window->DrawList->_ClipRectStack.Data[window->DrawList->_ClipRectStack.Size - 1] = clip_rect_vec4;
}

int ImGui::GetColumnIndex()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CurrentColumns ? window->DC.CurrentColumns->Current : 0;
}

int ImGui::GetColumnsCount()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CurrentColumns ? window->DC.CurrentColumns->Count : 1;
}

float ImGui::GetColumnOffsetFromNorm(const ImGuiOldColumns* columns, float offset_norm)
{
    return offset_norm * (columns->OffMaxX - columns->OffMinX);
}

float ImGui::GetColumnNormFromOffset(const ImGuiOldColumns* columns, float offset)
{
    return offset / (columns->OffMaxX - columns->OffMinX);
}

static const float COLUMNS_HIT_RECT_HALF_WIDTH = 4.0f;

static float GetDraggedColumnOffset(ImGuiOldColumns* columns, int column_index)
{
    // Active (dragged) column always follow mouse. The reason we need this is that dragging a column to the right edge of an auto-resizing
    // window creates a feedback loop because we store normalized positions. So while dragging we enforce absolute positioning.
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(column_index > 0); // We are not supposed to drag column 0.
    IM_ASSERT(g.ActiveId == columns->ID + ImGuiID(column_index));

    float x = g.IO.MousePos.x - g.ActiveIdClickOffset.x + COLUMNS_HIT_RECT_HALF_WIDTH - window->Pos.x;
    x = ImMax(x, ImGui::GetColumnOffset(column_index - 1) + g.Style.ColumnsMinSpacing);
    if ((columns->Flags & ImGuiOldColumnFlags_NoPreserveWidths))
        x = ImMin(x, ImGui::GetColumnOffset(column_index + 1) - g.Style.ColumnsMinSpacing);

    return x;
}

float ImGui::GetColumnOffset(int column_index)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiOldColumns* columns = window->DC.CurrentColumns;
    if (columns == NULL)
        return 0.0f;

    if (column_index < 0)
        column_index = columns->Current;
    IM_ASSERT(column_index < columns->Columns.Size);

    const float t = columns->Columns[column_index].OffsetNorm;
    const float x_offset = ImLerp(columns->OffMinX, columns->OffMaxX, t);
    return x_offset;
}

static float GetColumnWidthEx(ImGuiOldColumns* columns, int column_index, bool before_resize = false)
{
    if (column_index < 0)
        column_index = columns->Current;

    float offset_norm;
    if (before_resize)
        offset_norm = columns->Columns[column_index + 1].OffsetNormBeforeResize - columns->Columns[column_index].OffsetNormBeforeResize;
    else
        offset_norm = columns->Columns[column_index + 1].OffsetNorm - columns->Columns[column_index].OffsetNorm;
    return ImGui::GetColumnOffsetFromNorm(columns, offset_norm);
}

float ImGui::GetColumnWidth(int column_index)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiOldColumns* columns = window->DC.CurrentColumns;
    if (columns == NULL)
        return GetContentRegionAvail().x;

    if (column_index < 0)
        column_index = columns->Current;
    return GetColumnOffsetFromNorm(columns, columns->Columns[column_index + 1].OffsetNorm - columns->Columns[column_index].OffsetNorm);
}

void ImGui::SetColumnOffset(int column_index, float offset)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiOldColumns* columns = window->DC.CurrentColumns;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    IM_ASSERT(column_index < columns->Columns.Size);

    const bool preserve_width = !(columns->Flags & ImGuiOldColumnFlags_NoPreserveWidths) && (column_index < columns->Count - 1);
    const float width = preserve_width ? GetColumnWidthEx(columns, column_index, columns->IsBeingResized) : 0.0f;

    if (!(columns->Flags & ImGuiOldColumnFlags_NoForceWithinWindow))
        offset = ImMin(offset, columns->OffMaxX - g.Style.ColumnsMinSpacing * (columns->Count - column_index));
    columns->Columns[column_index].OffsetNorm = GetColumnNormFromOffset(columns, offset - columns->OffMinX);

    if (preserve_width)
        SetColumnOffset(column_index + 1, offset + ImMax(g.Style.ColumnsMinSpacing, width));
}

void ImGui::SetColumnWidth(int column_index, float width)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiOldColumns* columns = window->DC.CurrentColumns;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    SetColumnOffset(column_index + 1, GetColumnOffset(column_index) + width);
}

void ImGui::PushColumnClipRect(int column_index)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiOldColumns* columns = window->DC.CurrentColumns;
    if (column_index < 0)
        column_index = columns->Current;

    ImGuiOldColumnData* column = &columns->Columns[column_index];
    PushClipRect(column->ClipRect.Min, column->ClipRect.Max, false);
}

// Get into the columns background draw command (which is generally the same draw command as before we called BeginColumns)
void ImGui::PushColumnsBackground()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiOldColumns* columns = window->DC.CurrentColumns;
    if (columns->Count == 1)
        return;

    // Optimization: avoid SetCurrentChannel() + PushClipRect()
    columns->HostBackupClipRect = window->ClipRect;
    SetWindowClipRectBeforeSetChannel(window, columns->HostInitialClipRect);
    columns->Splitter.SetCurrentChannel(window->DrawList, 0);
}

void ImGui::PopColumnsBackground()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiOldColumns* columns = window->DC.CurrentColumns;
    if (columns->Count == 1)
        return;

    // Optimization: avoid PopClipRect() + SetCurrentChannel()
    SetWindowClipRectBeforeSetChannel(window, columns->HostBackupClipRect);
    columns->Splitter.SetCurrentChannel(window->DrawList, columns->Current + 1);
}

ImGuiOldColumns* ImGui::FindOrCreateColumns(ImGuiWindow* window, ImGuiID id)
{
    // We have few columns per window so for now we don't need bother much with turning this into a faster lookup.
    for (int n = 0; n < window->ColumnsStorage.Size; n++)
        if (window->ColumnsStorage[n].ID == id)
            return &window->ColumnsStorage[n];

    window->ColumnsStorage.push_back(ImGuiOldColumns());
    ImGuiOldColumns* columns = &window->ColumnsStorage.back();
    columns->ID = id;
    return columns;
}

ImGuiID ImGui::GetColumnsID(const char* str_id, int columns_count)
{
    ImGuiWindow* window = GetCurrentWindow();

    // Differentiate column ID with an arbitrary prefix for cases where users name their columns set the same as another widget.
    // In addition, when an identifier isn't explicitly provided we include the number of columns in the hash to make it uniquer.
    PushID(0x11223347 + (str_id ? 0 : columns_count));
    ImGuiID id = window->GetID(str_id ? str_id : "columns");
    PopID();

    return id;
}

void ImGui::BeginColumns(const char* str_id, int columns_count, ImGuiOldColumnFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    IM_ASSERT(columns_count >= 1);
    IM_ASSERT(window->DC.CurrentColumns == NULL);   // Nested columns are currently not supported

    // Acquire storage for the columns set
    ImGuiID id = GetColumnsID(str_id, columns_count);
    ImGuiOldColumns* columns = FindOrCreateColumns(window, id);
    IM_ASSERT(columns->ID == id);
    columns->Current = 0;
    columns->Count = columns_count;
    columns->Flags = flags;
    window->DC.CurrentColumns = columns;

    columns->HostCursorPosY = window->DC.CursorPos.y;
    columns->HostCursorMaxPosX = window->DC.CursorMaxPos.x;
    columns->HostInitialClipRect = window->ClipRect;
    columns->HostBackupParentWorkRect = window->ParentWorkRect;
    window->ParentWorkRect = window->WorkRect;

    // Set state for first column
    // We aim so that the right-most column will have the same clipping width as other after being clipped by parent ClipRect
    const float column_padding = g.Style.ItemSpacing.x;
    const float half_clip_extend_x = ImFloor(ImMax(window->WindowPadding.x * 0.5f, window->WindowBorderSize));
    const float max_1 = window->WorkRect.Max.x + column_padding - ImMax(column_padding - window->WindowPadding.x, 0.0f);
    const float max_2 = window->WorkRect.Max.x + half_clip_extend_x;
    columns->OffMinX = window->DC.Indent.x - column_padding + ImMax(column_padding - window->WindowPadding.x, 0.0f);
    columns->OffMaxX = ImMax(ImMin(max_1, max_2) - window->Pos.x, columns->OffMinX + 1.0f);
    columns->LineMinY = columns->LineMaxY = window->DC.CursorPos.y;

    // Clear data if columns count changed
    if (columns->Columns.Size != 0 && columns->Columns.Size != columns_count + 1)
        columns->Columns.resize(0);

    // Initialize default widths
    columns->IsFirstFrame = (columns->Columns.Size == 0);
    if (columns->Columns.Size == 0)
    {
        columns->Columns.reserve(columns_count + 1);
        for (int n = 0; n < columns_count + 1; n++)
        {
            ImGuiOldColumnData column;
            column.OffsetNorm = n / (float)columns_count;
            columns->Columns.push_back(column);
        }
    }

    for (int n = 0; n < columns_count; n++)
    {
        // Compute clipping rectangle
        ImGuiOldColumnData* column = &columns->Columns[n];
        float clip_x1 = IM_ROUND(window->Pos.x + GetColumnOffset(n));
        float clip_x2 = IM_ROUND(window->Pos.x + GetColumnOffset(n + 1) - 1.0f);
        column->ClipRect = ImRect(clip_x1, -FLT_MAX, clip_x2, +FLT_MAX);
        column->ClipRect.ClipWithFull(window->ClipRect);
    }

    if (columns->Count > 1)
    {
        columns->Splitter.Split(window->DrawList, 1 + columns->Count);
        columns->Splitter.SetCurrentChannel(window->DrawList, 1);
        PushColumnClipRect(0);
    }

    // We don't generally store Indent.x inside ColumnsOffset because it may be manipulated by the user.
    float offset_0 = GetColumnOffset(columns->Current);
    float offset_1 = GetColumnOffset(columns->Current + 1);
    float width = offset_1 - offset_0;
    PushItemWidth(width * 0.65f);
    window->DC.ColumnsOffset.x = ImMax(column_padding - window->WindowPadding.x, 0.0f);
    window->DC.CursorPos.x = IM_FLOOR(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
    window->WorkRect.Max.x = window->Pos.x + offset_1 - column_padding;
}

void ImGui::NextColumn()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems || window->DC.CurrentColumns == NULL)
        return;

    ImGuiContext& g = *GImGui;
    ImGuiOldColumns* columns = window->DC.CurrentColumns;

    if (columns->Count == 1)
    {
        window->DC.CursorPos.x = IM_FLOOR(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
        IM_ASSERT(columns->Current == 0);
        return;
    }

    // Next column
    if (++columns->Current == columns->Count)
        columns->Current = 0;

    PopItemWidth();

    // Optimization: avoid PopClipRect() + SetCurrentChannel() + PushClipRect()
    // (which would needlessly attempt to update commands in the wrong channel, then pop or overwrite them),
    ImGuiOldColumnData* column = &columns->Columns[columns->Current];
    SetWindowClipRectBeforeSetChannel(window, column->ClipRect);
    columns->Splitter.SetCurrentChannel(window->DrawList, columns->Current + 1);

    const float column_padding = g.Style.ItemSpacing.x;
    columns->LineMaxY = ImMax(columns->LineMaxY, window->DC.CursorPos.y);
    if (columns->Current > 0)
    {
        // Columns 1+ ignore IndentX (by canceling it out)
        // FIXME-COLUMNS: Unnecessary, could be locked?
        window->DC.ColumnsOffset.x = GetColumnOffset(columns->Current) - window->DC.Indent.x + column_padding;
    }
    else
    {
        // New row/line: column 0 honor IndentX.
        window->DC.ColumnsOffset.x = ImMax(column_padding - window->WindowPadding.x, 0.0f);
        columns->LineMinY = columns->LineMaxY;
    }
    window->DC.CursorPos.x = IM_FLOOR(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
    window->DC.CursorPos.y = columns->LineMinY;
    window->DC.CurrLineSize = ImVec2(0.0f, 0.0f);
    window->DC.CurrLineTextBaseOffset = 0.0f;

    // FIXME-COLUMNS: Share code with BeginColumns() - move code on columns setup.
    float offset_0 = GetColumnOffset(columns->Current);
    float offset_1 = GetColumnOffset(columns->Current + 1);
    float width = offset_1 - offset_0;
    PushItemWidth(width * 0.65f);
    window->WorkRect.Max.x = window->Pos.x + offset_1 - column_padding;
}

void ImGui::EndColumns()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiOldColumns* columns = window->DC.CurrentColumns;
    IM_ASSERT(columns != NULL);

    PopItemWidth();
    if (columns->Count > 1)
    {
        PopClipRect();
        columns->Splitter.Merge(window->DrawList);
    }

    const ImGuiOldColumnFlags flags = columns->Flags;
    columns->LineMaxY = ImMax(columns->LineMaxY, window->DC.CursorPos.y);
    window->DC.CursorPos.y = columns->LineMaxY;
    if (!(flags & ImGuiOldColumnFlags_GrowParentContentsSize))
        window->DC.CursorMaxPos.x = columns->HostCursorMaxPosX;  // Restore cursor max pos, as columns don't grow parent

    // Draw columns borders and handle resize
    // The IsBeingResized flag ensure we preserve pre-resize columns width so back-and-forth are not lossy
    bool is_being_resized = false;
    if (!(flags & ImGuiOldColumnFlags_NoBorder) && !window->SkipItems)
    {
        // We clip Y boundaries CPU side because very long triangles are mishandled by some GPU drivers.
        const float y1 = ImMax(columns->HostCursorPosY, window->ClipRect.Min.y);
        const float y2 = ImMin(window->DC.CursorPos.y, window->ClipRect.Max.y);
        int dragging_column = -1;
        for (int n = 1; n < columns->Count; n++)
        {
            ImGuiOldColumnData* column = &columns->Columns[n];
            float x = window->Pos.x + GetColumnOffset(n);
            const ImGuiID column_id = columns->ID + ImGuiID(n);
            const float column_hit_hw = COLUMNS_HIT_RECT_HALF_WIDTH;
            const ImRect column_hit_rect(ImVec2(x - column_hit_hw, y1), ImVec2(x + column_hit_hw, y2));
            KeepAliveID(column_id);
            if (IsClippedEx(column_hit_rect, column_id, false))
                continue;

            bool hovered = false, held = false;
            if (!(flags & ImGuiOldColumnFlags_NoResize))
            {
                ButtonBehavior(column_hit_rect, column_id, &hovered, &held);
                if (hovered || held)
                    g.MouseCursor = ImGuiMouseCursor_ResizeEW;
                if (held && !(column->Flags & ImGuiOldColumnFlags_NoResize))
                    dragging_column = n;
            }

            // Draw column
            const ImU32 col = GetColorU32(held ? ImGuiCol_SeparatorActive : hovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
            const float xi = IM_FLOOR(x);
            window->DrawList->AddLine(ImVec2(xi, y1 + 1.0f), ImVec2(xi, y2), col);
        }

        // Apply dragging after drawing the column lines, so our rendered lines are in sync with how items were displayed during the frame.
        if (dragging_column != -1)
        {
            if (!columns->IsBeingResized)
                for (int n = 0; n < columns->Count + 1; n++)
                    columns->Columns[n].OffsetNormBeforeResize = columns->Columns[n].OffsetNorm;
            columns->IsBeingResized = is_being_resized = true;
            float x = GetDraggedColumnOffset(columns, dragging_column);
            SetColumnOffset(dragging_column, x);
        }
    }
    columns->IsBeingResized = is_being_resized;

    window->WorkRect = window->ParentWorkRect;
    window->ParentWorkRect = columns->HostBackupParentWorkRect;
    window->DC.CurrentColumns = NULL;
    window->DC.ColumnsOffset.x = 0.0f;
    window->DC.CursorPos.x = IM_FLOOR(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
}

void ImGui::Columns(int columns_count, const char* id, bool border)
{
    ImGuiWindow* window = GetCurrentWindow();
    IM_ASSERT(columns_count >= 1);

    ImGuiOldColumnFlags flags = (border ? 0 : ImGuiOldColumnFlags_NoBorder);
    //flags |= ImGuiOldColumnFlags_NoPreserveWidths; // NB: Legacy behavior
    ImGuiOldColumns* columns = window->DC.CurrentColumns;
    if (columns != NULL && columns->Count == columns_count && columns->Flags == flags)
        return;

    if (columns != NULL)
        EndColumns();

    if (columns_count != 1)
        BeginColumns(id, columns_count, flags);
}


//-------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
