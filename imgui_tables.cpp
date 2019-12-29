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

// Typical call flow: (root level is public API):
// - BeginTable()                               user begin into a table
//    - BeginChild()                            - (if ScrollX/ScrollY is set)
//    - TableBeginInitVisibility()              - lock columns visibility
// - TableSetupColumn()                         user submit columns details (optional)
// - TableAutoHeaders() or TableHeader()        user submit a headers row (optional)
//    - TableSortSpecsClickColumn()
// - TableGetSortSpecs()                        user queries updated sort specs (optional)
// - TableNextRow() / TableNextCell()           user begin into the first row, also automatically called by TableAutoHeaders()
//    - TableUpdateLayout()                     - called by the FIRST call to TableNextRow()
//      - TableUpdateDrawChannels()             - setup ImDrawList channels
//      - TableUpdateBorders()                  - detect hovering columns for resize, ahead of contents submission
//      - TableDrawContextMenu()                - draw right-click context menu
// - [...]                                      user emit contents
// - EndTable()                                 user ends the table
//    - TableDrawBorders()                      - draw outer borders, inner vertical borders
//    - TableDrawMergeChannels()                - merge draw channels if clipping isn't required
//    - TableSetColumnWidth()                   - apply resizing width
//      - TableUpdateColumnsWeightFromWidth()
//      - EndChild()                            - (if ScrollX/ScrollY is set)

// Configuration
static const float TABLE_RESIZE_SEPARATOR_HALF_THICKNESS = 4.0f;    // Extend outside inner borders.
static const float TABLE_RESIZE_SEPARATOR_FEEDBACK_TIMER = 0.06f;   // Delay/timer before making the hover feedback (color+cursor) visible because tables/columns tends to be more cramped.

// Helper
inline ImGuiTableFlags TableFixFlags(ImGuiTableFlags flags)
{
    // Adjust flags: set default sizing policy
    if ((flags & ImGuiTableFlags_SizingPolicyMaskX_) == 0)
        flags |= (flags & ImGuiTableFlags_ScrollX) ? ImGuiTableFlags_SizingPolicyFixedX : ImGuiTableFlags_SizingPolicyStretchX;

    // Adjust flags: MultiSortable automatically enable Sortable
    if (flags & ImGuiTableFlags_MultiSortable)
        flags |= ImGuiTableFlags_Sortable;

    // Adjust flags: disable saved settings if there's nothing to save
    if ((flags & (ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable)) == 0)
        flags |= ImGuiTableFlags_NoSavedSettings;

    // Adjust flags: enforce borders when resizable
    if (flags & ImGuiTableFlags_Resizable)
        flags |= ImGuiTableFlags_BordersV;

    // Adjust flags: disable top rows freezing if there's no scrolling
    // In theory we could want to assert if ScrollFreeze was set without the corresponding scroll flag, but that would hinder demos.
    if ((flags & ImGuiTableFlags_ScrollX) == 0)
        flags &= ~ImGuiTableFlags_ScrollFreezeColumnsMask_;
    if ((flags & ImGuiTableFlags_ScrollY) == 0)
        flags &= ~ImGuiTableFlags_ScrollFreezeRowsMask_;

    // Adjust flags: disable NoHostExtendY if we have any scrolling going on
    if ((flags & ImGuiTableFlags_NoHostExtendY) && (flags & (ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY)) != 0)
        flags &= ~ImGuiTableFlags_NoHostExtendY;

    // Adjust flags: we don't support NoClipX with (FreezeColumns > 0), we could with some work but it doesn't appear to be worth the effort
    if (flags & ImGuiTableFlags_ScrollFreezeColumnsMask_)
        flags &= ~ImGuiTableFlags_NoClipX;

    return flags;
}

// About 'outer_size':
//   The meaning of outer_size needs to differ slightly depending of if we are using ScrollX/ScrollY flags.
//   With ScrollX/ScrollY: using a child window for scrolling:
//   - outer_size.y < 0.0f  ->  bottom align
//   - outer_size.y = 0.0f  ->  bottom align: consistent with BeginChild(), best to preserve (0,0) default arg
//   - outer_size.y > 0.0f  ->  fixed child height
//   Without scrolling, we output table directly in parent window:
//   - outer_size.y < 0.0f  ->  bottom align (will auto extend, unless NoHostExtendV is set)
//   - outer_size.y = 0.0f  ->  zero minimum height (will auto extend, unless NoHostExtendV is set)
//   - outer_size.y > 0.0f  ->  minimum height (will auto extend, unless NoHostExtendV is set)
// About: 'inner_width':
//   With ScrollX:
//   - inner_width  < 0.0f  ->  *illegal* fit in known width (right align from outer_size.x) <-- weird
//   - inner_width  = 0.0f  ->  auto enlarge: *only* fixed size columns, which will take space they need (proportional columns becomes fixed columns) <-- desired default :(
//   - inner_width  > 0.0f  ->  fit in known width: fixed column take space they need if possible (otherwise shrink down), proportional columns share remaining space.
//   Without ScrollX:
//   - inner_width  < 0.0f  ->  fit in known width (right align from outer_size.x) <-- desired default
//   - inner_width  = 0.0f  ->  auto enlarge: will emit contents size in parent window
//   - inner_width  > 0.0f  ->  fit in known width (bypass outer_size.x, permitted but not useful, should instead alter outer_width)
// FIXME-TABLE: This is currently not very useful.
// FIXME-TABLE: Replace enlarge vs fixed width by a flag.
// Even if not really useful, we allow 'inner_scroll_width < outer_size.x' for consistency and to facilitate understanding of what the value does.
bool    ImGui::BeginTable(const char* str_id, int columns_count, ImGuiTableFlags flags, const ImVec2& outer_size, float inner_width)
{
    ImGuiID id = GetID(str_id);
    return BeginTableEx(str_id, id, columns_count, flags, outer_size, inner_width);
}

bool    ImGui::BeginTableEx(const char* name, ImGuiID id, int columns_count, ImGuiTableFlags flags, const ImVec2& outer_size, float inner_width)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* outer_window = GetCurrentWindow();

    // Sanity checks
    IM_ASSERT(columns_count > 0 && columns_count < IMGUI_TABLE_MAX_COLUMNS && "Only 0..63 columns allowed!");
    if (flags & ImGuiTableFlags_ScrollX)
        IM_ASSERT(inner_width >= 0.0f);

    const bool use_child_window = (flags & (ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY)) != 0;
    const ImVec2 avail_size = GetContentRegionAvail();
    ImVec2 actual_outer_size = CalcItemSize(outer_size, ImMax(avail_size.x, 1.0f), use_child_window ? ImMax(avail_size.y, 1.0f) : 0.0f);
    ImRect outer_rect(outer_window->DC.CursorPos, outer_window->DC.CursorPos + actual_outer_size);

    // If an outer size is specified ahead we will be able to early out when not visible. Exact clipping rules may evolve.
    if (use_child_window && IsClippedEx(outer_rect, 0, false))
    {
        ItemSize(outer_rect);
        return false;
    }

    flags = TableFixFlags(flags);
    if (outer_window->Flags & ImGuiWindowFlags_NoSavedSettings)
        flags |= ImGuiTableFlags_NoSavedSettings;

    // Acquire storage for the table
    ImGuiTable* table = g.Tables.GetOrAddByKey(id);
    const ImGuiTableFlags table_last_flags = table->Flags;
    const int instance_no = (table->LastFrameActive != g.FrameCount) ? 0 : table->InstanceNo + 1;
    const ImGuiID instance_id = id + instance_no;
    if (instance_no > 0)
        IM_ASSERT(table->ColumnsCount == columns_count && "BeginTable(): Cannot change columns count mid-frame while preserving same ID");

    // Initialize
    table->ID = id;
    table->Flags = flags;
    table->IsFirstFrame = (table->LastFrameActive == -1);
    table->InstanceNo = (ImS16)instance_no;
    table->LastFrameActive = g.FrameCount;
    table->OuterWindow = table->InnerWindow = outer_window;
    table->ColumnsCount = columns_count;
    table->ColumnsNames.Buf.resize(0);
    table->IsLayoutLocked = false;
    table->InnerWidth = inner_width;
    table->OuterRect = outer_rect;
    table->WorkRect = outer_rect;

    if (use_child_window)
    {
        // Ensure no vertical scrollbar appears if we only want horizontal one, to make flag consistent (we have no other way to disable vertical scrollbar of a window while keeping the horizontal one showing)
        ImVec2 override_content_size(FLT_MAX, FLT_MAX);
        if ((flags & ImGuiTableFlags_ScrollX) && !(flags & ImGuiTableFlags_ScrollY))
            override_content_size.y = FLT_MIN;

        // Ensure specified width (when not specified, Stretched columns will act as if the width == OuterWidth and never lead to any scrolling)
        // We don't handle inner_width < 0.0f, we could potentially use it to right-align based on the right side of the child window work rect,
        // which would require knowing ahead if we are going to have decoration taking horizontal spaces (typically a vertical scrollbar).
        if (inner_width != 0.0f)
            override_content_size.x = inner_width;

        if (override_content_size.x != FLT_MAX || override_content_size.y != FLT_MAX)
            SetNextWindowContentSize(ImVec2(override_content_size.x != FLT_MAX ? override_content_size.x : 0.0f, override_content_size.y != FLT_MAX ? override_content_size.y : 0.0f));

        // Create scrolling region (without border = zero window padding)
        ImGuiWindowFlags child_flags = (flags & ImGuiTableFlags_ScrollX) ? ImGuiWindowFlags_HorizontalScrollbar : ImGuiWindowFlags_None;
        BeginChildEx(name, instance_id, table->OuterRect.GetSize(), false, child_flags);
        table->InnerWindow = g.CurrentWindow;
        table->WorkRect = table->InnerWindow->WorkRect;
        table->OuterRect = table->InnerWindow->Rect();
    }
    else
    {
        // WorkRect.Max will grow as we append contents.
        PushID(instance_id);
    }

    const bool has_cell_padding_x = (flags & (ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV)) != 0;
    ImGuiWindow* inner_window = table->InnerWindow;
    table->CurrentColumn = -1;
    table->CurrentRow = -1;
    table->RowBgColorCounter = 0;
    table->LastRowFlags = ImGuiTableRowFlags_None;

    table->CellPaddingX1 = has_cell_padding_x ? g.Style.CellPadding.x + 1.0f : 0.0f;
    table->CellPaddingX2 = has_cell_padding_x ? g.Style.CellPadding.x : 0.0f;
    table->CellPaddingY = g.Style.CellPadding.y;
    table->CellSpacingX = has_cell_padding_x ? 0.0f : g.Style.CellPadding.x;

    table->HostClipRect = inner_window->ClipRect;
    table->InnerClipRect = (inner_window == outer_window) ? table->WorkRect : inner_window->ClipRect;
    table->InnerClipRect.ClipWith(table->WorkRect);     // We need this to honor inner_width
    table->InnerClipRect.ClipWith(table->HostClipRect);
    table->InnerClipRect.Max.y = (flags & ImGuiTableFlags_NoHostExtendY) ? table->WorkRect.Max.y : inner_window->ClipRect.Max.y;
    table->BackgroundClipRect = table->InnerClipRect;
    table->RowPosY1 = table->RowPosY2 = table->WorkRect.Min.y; // This is needed somehow
    table->RowTextBaseline = 0.0f; // This will be cleared again by TableBeginRow()
    table->FreezeRowsRequest = (ImS8)((flags & ImGuiTableFlags_ScrollFreezeRowsMask_) >> ImGuiTableFlags_ScrollFreezeRowsShift_);
    table->FreezeRowsCount = (inner_window->Scroll.y != 0.0f) ? table->FreezeRowsRequest : 0;
    table->FreezeColumnsRequest = (ImS8)((flags & ImGuiTableFlags_ScrollFreezeColumnsMask_) >> ImGuiTableFlags_ScrollFreezeColumnsShift_);
    table->FreezeColumnsCount = (inner_window->Scroll.x != 0.0f) ? table->FreezeColumnsRequest : 0;
    table->IsFreezeRowsPassed = (table->FreezeRowsCount == 0);
    table->DeclColumnsCount = 0;
    table->LastResizedColumn = table->ResizedColumn;
    table->HoveredColumnBody = -1;
    table->HoveredColumnBorder = -1;
    table->RightMostActiveColumn = -1;
    table->IsFirstFrame = false;

    // FIXME-TABLE FIXME-STYLE: Using opaque colors facilitate overlapping elements of the grid
    //table->BorderOuterColor = GetColorU32(ImGuiCol_Separator, 1.00f);
    //table->BorderInnerColor = GetColorU32(ImGuiCol_Separator, 0.60f);
    table->BorderOuterColor = GetColorU32(ImVec4(0.31f, 0.31f, 0.35f, 1.00f));
    table->BorderInnerColor = GetColorU32(ImVec4(0.23f, 0.23f, 0.25f, 1.00f));
    //table->BorderOuterColor = IM_COL32(255, 0, 0, 255);
    //table->BorderInnerColor = IM_COL32(255, 255, 0, 255);
    table->BorderX1 = table->InnerClipRect.Min.x;// +((table->Flags & ImGuiTableFlags_BordersOuter) ? 0.0f : -1.0f);
    table->BorderX2 = table->InnerClipRect.Max.x;// +((table->Flags & ImGuiTableFlags_BordersOuter) ? 0.0f : +1.0f);

    // Make table current
    g.CurrentTableStack.push_back(ImGuiPtrOrIndex(g.Tables.GetIndex(table)));
    g.CurrentTable = table;
    outer_window->DC.CurrentTable = table;
    if ((table_last_flags & ImGuiTableFlags_Reorderable) && !(flags & ImGuiTableFlags_Reorderable))
        table->IsResetDisplayOrderRequest = true;

    // Clear data if columns count changed
    if (table->Columns.Size != 0 && table->Columns.Size != columns_count)
    {
        table->Columns.resize(0);
        table->DisplayOrder.resize(0);
    }

    // Setup default columns state
    if (table->Columns.Size == 0)
    {
        table->IsFirstFrame = true;
        table->IsSortSpecsDirty = true;
        table->Columns.reserve(columns_count);
        table->DisplayOrder.reserve(columns_count);
        for (int n = 0; n < columns_count; n++)
        {
            ImGuiTableColumn column;
            column.IndexDisplayOrder = (ImS8)n;
            table->Columns.push_back(column);
            table->DisplayOrder.push_back(column.IndexDisplayOrder);
        }
    }

    // Load settings
    if (table->IsFirstFrame || table->IsSettingsRequestLoad)
        TableLoadSettings(table);

    // Handle resizing request
    // (We process this at the first beginning of the frame)
    // FIXME-TABLE: Preserve contents width _while resizing down_ until releasing.
    // FIXME-TABLE: Contains columns if our work area doesn't allow for scrolling.
    if (table->InstanceNo == 0)
    {
        if (table->ResizedColumn != -1 && table->ResizedColumnNextWidth != FLT_MAX)
            TableSetColumnWidth(table, &table->Columns[table->ResizedColumn], table->ResizedColumnNextWidth);
        table->ResizedColumnNextWidth = FLT_MAX;
        table->ResizedColumn = -1;
    }

    // Handle reordering request
    // Note: we don't clear ReorderColumn after handling the request.
    if (table->InstanceNo == 0)
    {
        if (table->HeadHeaderColumn == -1 && table->ReorderColumn != -1)
            table->ReorderColumn = -1;
        table->HeadHeaderColumn = -1;
        if (table->ReorderColumn != -1 && table->ReorderColumnDir != 0)
        {
            IM_ASSERT(table->ReorderColumnDir == -1 || table->ReorderColumnDir == +1);
            IM_ASSERT(table->Flags & ImGuiTableFlags_Reorderable);
            ImGuiTableColumn* dragged_column = &table->Columns[table->ReorderColumn];
            ImGuiTableColumn* target_column = &table->Columns[(table->ReorderColumnDir == -1) ? dragged_column->PrevActiveColumn : dragged_column->NextActiveColumn];
            ImSwap(table->DisplayOrder[dragged_column->IndexDisplayOrder], table->DisplayOrder[target_column->IndexDisplayOrder]);
            ImSwap(dragged_column->IndexDisplayOrder, target_column->IndexDisplayOrder);
            table->ReorderColumnDir = 0;
            table->IsSettingsDirty = true;
        }
    }

    // Handle display order reset request
    if (table->IsResetDisplayOrderRequest)
    {
        for (int n = 0; n < columns_count; n++)
            table->DisplayOrder[n] = table->Columns[n].IndexDisplayOrder = (ImU8)n;
        table->IsResetDisplayOrderRequest = false;
        table->IsSettingsDirty = true;
    }

    TableBeginInitVisibility(table);

    // Grab a copy of window fields we will modify
    table->BackupSkipItems = inner_window->SkipItems;
    table->BackupWorkRect = inner_window->WorkRect;
    table->BackupCursorMaxPos = inner_window->DC.CursorMaxPos;

    if (flags & ImGuiTableFlags_NoClipX)
        table->DrawSplitter.SetCurrentChannel(inner_window->DrawList, 1);
    else
        inner_window->DrawList->PushClipRect(inner_window->ClipRect.Min, inner_window->ClipRect.Max, false);

    return true;
}

void ImGui::TableBeginInitVisibility(ImGuiTable* table)
{
    // Setup and lock Active state and order
    table->ColumnsActiveCount = 0;
    table->IsDefaultDisplayOrder = true;
    ImGuiTableColumn* last_active_column = NULL;
    bool want_column_auto_fit = false;
    for (int order_n = 0; order_n < table->ColumnsCount; order_n++)
    {
        const int column_n = table->DisplayOrder[order_n];
        if (column_n != order_n)
            table->IsDefaultDisplayOrder = false;
        ImGuiTableColumn* column = &table->Columns[column_n];
        column->NameOffset = -1;
        if (!(table->Flags & ImGuiTableFlags_Hideable) || (column->Flags & ImGuiTableColumnFlags_NoHide))
            column->NextIsActive = true;
        if (column->IsActive != column->NextIsActive)
        {
            column->IsActive = column->NextIsActive;
            table->IsSettingsDirty = true;
            if (!column->IsActive && column->SortOrder != -1)
                table->IsSortSpecsDirty = true;
        }
        if (column->SortOrder > 0 && !(table->Flags & ImGuiTableFlags_MultiSortable))
            table->IsSortSpecsDirty = true;
        if (column->AutoFitQueue != 0x00)
            want_column_auto_fit = true;

        ImU64 index_mask = (ImU64)1 << column_n;
        ImU64 display_order_mask = (ImU64)1 << column->IndexDisplayOrder;
        if (column->IsActive)
        {
            column->PrevActiveColumn = column->NextActiveColumn = -1;
            if (last_active_column)
            {
                last_active_column->NextActiveColumn = (ImS8)column_n;
                column->PrevActiveColumn = (ImS8)table->Columns.index_from_ptr(last_active_column);
            }
            column->IndexWithinActiveSet = (ImS8)table->ColumnsActiveCount;
            table->ColumnsActiveCount++;
            table->ActiveMaskByIndex |= index_mask;
            table->ActiveMaskByDisplayOrder |= display_order_mask;
            last_active_column = column;
        }
        else
        {
            column->IndexWithinActiveSet = -1;
            table->ActiveMaskByIndex &= ~index_mask;
            table->ActiveMaskByDisplayOrder &= ~display_order_mask;
        }
        IM_ASSERT(column->IndexWithinActiveSet <= column->IndexDisplayOrder);
    }
    table->VisibleMaskByIndex = table->ActiveMaskByIndex; // Columns will be masked out by TableUpdateLayout() when Clipped
    table->RightMostActiveColumn = (ImS8)(last_active_column ? table->Columns.index_from_ptr(last_active_column) : -1);

    // Disable child window clipping while fitting columns. This is not strictly necessary but makes it possible to avoid
    // the column fitting to wait until the first visible frame of the child container (may or not be a good thing).
    if (want_column_auto_fit && table->OuterWindow != table->InnerWindow)
        table->InnerWindow->SkipItems = false;
}

void ImGui::TableUpdateDrawChannels(ImGuiTable* table)
{
    // Allocate draw channels.
    // - We allocate them following the storage order instead of the display order so reordering won't needlessly increase overall dormant memory cost
    // - We isolate headers draw commands in their own channels instead of just altering clip rects. This is in order to facilitate merging of draw commands.
    // - After crossing FreezeRowsCount, all columns see their current draw channel increased.
    // - We only use the dummy draw channel so we can push a null clipping rectangle into it without affecting other channels, while simplifying per-row/per-cell overhead. It will be empty and discarded when merged.
    // Draw channel allocation (before merging):
    // - NoClip                       --> 1+1 channels: background + foreground (same clip rect == 1 draw call)
    // - Clip                         --> 1+N channels
    // - FreezeRows || FreezeColumns  --> 1+N*2 (unless scrolling value is zero)
    // - FreezeRows && FreezeColunns  --> 2+N*2 (unless scrolling value is zero)
    const int freeze_row_multiplier = (table->FreezeRowsCount > 0) ? 2 : 1;
    const int channels_for_row = (table->Flags & ImGuiTableFlags_NoClipX) ? 1 : table->ColumnsActiveCount;
    const int channels_for_background = 1;
    const int channels_for_dummy = (table->ColumnsActiveCount < table->ColumnsCount || table->VisibleMaskByIndex != table->ActiveMaskByIndex) ? +1 : 0;
    const int channels_total = channels_for_background + (channels_for_row * freeze_row_multiplier) + channels_for_dummy;
    table->DrawSplitter.Split(table->InnerWindow->DrawList, channels_total);
    table->DummyDrawChannel = channels_for_dummy ? (ImS8)(channels_total - 1) : -1;

    int draw_channel_current = 1;
    for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
    {
        ImGuiTableColumn* column = &table->Columns[column_n];
        if (!column->IsClipped)
        {
            column->DrawChannelRowsBeforeFreeze = (ImS8)(draw_channel_current);
            column->DrawChannelRowsAfterFreeze = (ImS8)(draw_channel_current + (table->FreezeRowsCount > 0 ? channels_for_row : 0));
            if (!(table->Flags & ImGuiTableFlags_NoClipX))
                draw_channel_current++;
        }
        else
        {
            column->DrawChannelRowsBeforeFreeze = column->DrawChannelRowsAfterFreeze = table->DummyDrawChannel;
        }
        column->DrawChannelCurrent = column->DrawChannelRowsBeforeFreeze;
    }
}

// Adjust flags: default width mode + weighted columns are not allowed when auto extending
static ImGuiTableColumnFlags TableFixColumnFlags(ImGuiTable* table, ImGuiTableColumnFlags flags)
{
    // Sizing Policy
    if ((flags & ImGuiTableColumnFlags_WidthMask_) == 0)
    {
        if (table->Flags & ImGuiTableFlags_SizingPolicyFixedX)
            flags |= ((table->Flags & ImGuiTableFlags_Resizable) && !(flags & ImGuiTableColumnFlags_NoResize)) ? ImGuiTableColumnFlags_WidthFixed : ImGuiTableColumnFlags_WidthAlwaysAutoResize;
        else
            flags |= ImGuiTableColumnFlags_WidthStretch;
    }
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiTableColumnFlags_WidthMask_)); // Check that only 1 of each set is used.
    if ((flags & ImGuiTableColumnFlags_WidthAlwaysAutoResize))// || ((flags & ImGuiTableColumnFlags_WidthStretch) && (table->Flags & ImGuiTableFlags_SizingPolicyStretchX)))
        flags |= ImGuiTableColumnFlags_NoResize;
    //if ((flags & ImGuiTableColumnFlags_WidthStretch) && (table->Flags & ImGuiTableFlags_SizingPolicyFixedX))
    //    flags = (flags & ~ImGuiTableColumnFlags_WidthMask_) | ImGuiTableColumnFlags_WidthFixed;

    // Sorting
    if ((flags & ImGuiTableColumnFlags_NoSortAscending) && (flags & ImGuiTableColumnFlags_NoSortDescending))
        flags |= ImGuiTableColumnFlags_NoSort;

    // Alignment
    //if ((flags & ImGuiTableColumnFlags_AlignMask_) == 0)
    //    flags |= ImGuiTableColumnFlags_AlignCenter;
    //IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiTableColumnFlags_AlignMask_)); // Check that only 1 of each set is used.

    return flags;
}

static void TableFixColumnSortDirection(ImGuiTableColumn* column)
{
    // Handle NoSortAscending/NoSortDescending
    if (column->SortDirection == ImGuiSortDirection_Ascending && (column->Flags & ImGuiTableColumnFlags_NoSortAscending))
        column->SortDirection = ImGuiSortDirection_Descending;
    else if (column->SortDirection == ImGuiSortDirection_Descending && (column->Flags & ImGuiTableColumnFlags_NoSortDescending))
        column->SortDirection = ImGuiSortDirection_Ascending;
}

static float TableGetMinColumnWidth()
{
    ImGuiContext& g = *GImGui;
    // return g.Style.ColumnsMinSpacing;
    return g.Style.FramePadding.x * 3.0f;
}

// Layout columns for the frame
// Runs on the first call to TableNextRow(), to give a chance for TableSetupColumn() to be called first.
// FIXME-TABLE: Our width (and therefore our WorkRect) will be minimal in the first frame for WidthAlwaysAutoResize columns,
// increase feedback side-effect with widgets relying on WorkRect.Max.x. Maybe provide a default distribution for WidthAlwaysAutoResize columns?
void    ImGui::TableUpdateLayout(ImGuiTable* table)
{
    IM_ASSERT(table->IsLayoutLocked == false);

    // Compute offset, clip rect for the frame
    const ImRect work_rect = table->WorkRect;
    const float padding_auto_x = table->CellPaddingX1; // Can't make auto padding larger than what WorkRect knows about so right-alignment matches.
    const float min_column_width = TableGetMinColumnWidth();

    int count_fixed = 0;
    float width_fixed = 0.0f;
    float total_weights = 0.0f;
    table->LeftMostStretchedColumnDisplayOrder = -1;
    for (int order_n = 0; order_n < table->ColumnsCount; order_n++)
    {
        if (!(table->ActiveMaskByDisplayOrder & ((ImU64)1 << order_n)))
            continue;
        const int column_n = table->DisplayOrder[order_n];
        ImGuiTableColumn* column = &table->Columns[column_n];

        // Adjust flags: default width mode + weighted columns are not allowed when auto extending
        // FIXME-TABLE: Clarify why we need to do this again here and not just in TableSetupColumn()
        column->Flags = TableFixColumnFlags(table, column->FlagsIn);

        // We have a unusual edge case where if the user doesn't call TableGetSortSpecs() but has sorting enabled
        // or varying sorting flags, we still want the sorting arrows to honor those flags.
        if (table->Flags & ImGuiTableFlags_Sortable)
            TableFixColumnSortDirection(column);

        if (column->Flags & (ImGuiTableColumnFlags_WidthAlwaysAutoResize | ImGuiTableColumnFlags_WidthFixed))
        {
            // Latch initial size for fixed columns
            count_fixed += 1;
            const bool init_size = (column->AutoFitQueue != 0x00) || (column->Flags & ImGuiTableColumnFlags_WidthAlwaysAutoResize);
            if (init_size)
            {
                // Combine width from regular rows + width from headers unless requested not to
                float width_request = (float)ImMax(column->ContentWidthRowsFrozen, column->ContentWidthRowsUnfrozen);
                if (!(table->Flags & ImGuiTableFlags_NoHeadersWidth) && !(column->Flags & ImGuiTableColumnFlags_NoHeaderWidth))
                    width_request = ImMax(width_request, (float)column->ContentWidthHeadersDesired);
                column->WidthRequested = ImMax(width_request + padding_auto_x, min_column_width);

                // FIXME-TABLE: Increase minimum size during init frame so avoid biasing auto-fitting widgets (e.g. TextWrapped) too much.
                // Otherwise what tends to happen is that TextWrapped would output a very large height (= first frame scrollbar display very off + clipper would skip lots of items)
                // This is merely making the side-effect less extreme, but doesn't properly fixes it.
                if (column->AutoFitQueue > 0x01 && table->IsFirstFrame)
                    column->WidthRequested = ImMax(column->WidthRequested, min_column_width * 4.0f);
            }
            width_fixed += column->WidthRequested;
        }
        else
        {
            IM_ASSERT(column->Flags & ImGuiTableColumnFlags_WidthStretch);
            IM_ASSERT(column->ResizeWeight > 0.0f);
            total_weights += column->ResizeWeight;
            if (table->LeftMostStretchedColumnDisplayOrder == -1)
                table->LeftMostStretchedColumnDisplayOrder = (ImS8)column->IndexDisplayOrder;
        }
    }

    // Layout
    const float width_spacings = table->CellSpacingX * (table->ColumnsActiveCount - 1);
    float width_avail;
    if ((table->Flags & ImGuiTableFlags_ScrollX) && (table->InnerWidth == 0.0f))
        width_avail = table->InnerClipRect.GetWidth() - width_spacings - 1.0f;
    else
        width_avail = work_rect.GetWidth() - width_spacings - 1.0f; // Remove -1.0f to cancel out the +1.0f we are doing in EndTable() to make last column line visible
    const float width_avail_for_stretched_columns = width_avail - width_fixed;
    float width_remaining_for_stretched_columns = width_avail_for_stretched_columns;

    // Apply final width based on requested widths
    // Mark some columns as not resizable
    int count_resizable = 0;
    table->ColumnsTotalWidth = width_spacings;
    for (int order_n = 0; order_n < table->ColumnsCount; order_n++)
    {
        if (!(table->ActiveMaskByDisplayOrder & ((ImU64)1 << order_n)))
            continue;
        ImGuiTableColumn* column = &table->Columns[table->DisplayOrder[order_n]];

        // Allocate width for stretched/weighted columns
        if (column->Flags & ImGuiTableColumnFlags_WidthStretch)
        {
            float weight_ratio = column->ResizeWeight / total_weights;
            column->WidthRequested = IM_FLOOR(ImMax(width_avail_for_stretched_columns * weight_ratio, min_column_width) + 0.01f);
            width_remaining_for_stretched_columns -= column->WidthRequested;

            // [Resize Rule 2] Resizing from right-side of a weighted column before a fixed column froward sizing to left-side of fixed column
            // We also need to copy the NoResize flag..
            if (column->NextActiveColumn != -1)
                if (ImGuiTableColumn* next_column = &table->Columns[column->NextActiveColumn])
                    if (next_column->Flags & ImGuiTableColumnFlags_WidthFixed)
                        column->Flags |= (next_column->Flags & ImGuiTableColumnFlags_NoDirectResize_);
        }

        // [Resize Rule 1] The right-most active column is not resizable if there is at least one Stretch column (see comments in TableResizeColumn().)
        if (column->NextActiveColumn == -1 && table->LeftMostStretchedColumnDisplayOrder != -1)
            column->Flags |= ImGuiTableColumnFlags_NoDirectResize_;

        if (!(column->Flags & ImGuiTableColumnFlags_NoResize))
            count_resizable++;

        // Assign final width, record width in case we will need to shrink
        column->WidthGiven = ImFloor(ImMax(column->WidthRequested, min_column_width));
        table->ColumnsTotalWidth += column->WidthGiven;
    }

#if 0
    const float width_excess = table->ColumnsTotalWidth - work_rect.GetWidth();
    if ((table->Flags & ImGuiTableFlags_SizingPolicyStretchX) && width_excess > 0.0f)
    {
        // Shrink widths when the total does not fit
        // FIXME-TABLE: This is working but confuses/conflicts with manual resizing.
        // FIXME-TABLE: Policy to shrink down below below ideal/requested width if there's no room?
        g.ShrinkWidthBuffer.resize(table->ColumnsActiveCount);
        for (int order_n = 0, active_n = 0; order_n < table->ColumnsCount; order_n++)
        {
            if (!(table->ActiveMaskByDisplayOrder & ((ImU64)1 << order_n)))
                continue;
            const int column_n = table->DisplayOrder[order_n];
            g.ShrinkWidthBuffer[active_n].Index = column_n;
            g.ShrinkWidthBuffer[active_n].Width = table->Columns[column_n].WidthGiven;
            active_n++;
        }
        ShrinkWidths(g.ShrinkWidthBuffer.Data, g.ShrinkWidthBuffer.Size, width_excess);
        for (int n = 0; n < g.ShrinkWidthBuffer.Size; n++)
            table->Columns[g.ShrinkWidthBuffer.Data[n].Index].WidthGiven = ImMax(g.ShrinkWidthBuffer.Data[n].Width, min_column_size);
        // FIXME: Need to alter table->ColumnsTotalWidth
    }
    else
#endif

    // Redistribute remainder width due to rounding (remainder width is < 1.0f * number of Stretch column)
    // Using right-to-left distribution (more likely to match resizing cursor), could be adjusted depending where the mouse cursor is and/or relative weights.
    // FIXME-TABLE: May be simpler to store floating width and floor final positions only
    // FIXME-TABLE: Make it optional? User might prefer to preserve pixel perfect same size?
    if (width_remaining_for_stretched_columns >= 1.0f)
        for (int order_n = table->ColumnsCount - 1; total_weights > 0.0f && width_remaining_for_stretched_columns >= 1.0f && order_n >= 0; order_n--)
        {
            if (!(table->ActiveMaskByDisplayOrder & ((ImU64)1 << order_n)))
                continue;
            ImGuiTableColumn* column = &table->Columns[table->DisplayOrder[order_n]];
            if (!(column->Flags & ImGuiTableColumnFlags_WidthStretch))
                continue;
            column->WidthRequested += 1.0f;
            column->WidthGiven += 1.0f;
            width_remaining_for_stretched_columns -= 1.0f;
        }

    // Setup final position, offset and clipping rectangles
    int active_n = 0;
    float offset_x = (table->FreezeColumnsCount > 0) ? table->OuterRect.Min.x : work_rect.Min.x;
    ImRect host_clip_rect = table->InnerClipRect;
    for (int order_n = 0; order_n < table->ColumnsCount; order_n++)
    {
        const int column_n = table->DisplayOrder[order_n];
        ImGuiTableColumn* column = &table->Columns[column_n];

        if (table->FreezeColumnsCount > 0 && table->FreezeColumnsCount == active_n)
            offset_x += work_rect.Min.x - table->OuterRect.Min.x;

        if (!(table->ActiveMaskByDisplayOrder & ((ImU64)1 << order_n)))
        {
            // Hidden column: clear a few fields and we are done with it for the remainder of the function.
            // We set a zero-width clip rect however we pay attention to set Min.y/Max.y properly to not interfere with the clipper.
            column->MinX = column->MaxX = offset_x;
            column->StartXRows = column->StartXHeaders = offset_x;
            column->WidthGiven = 0.0f;
            column->ClipRect.Min.x = offset_x;
            column->ClipRect.Min.y = work_rect.Min.y;
            column->ClipRect.Max.x = offset_x;
            column->ClipRect.Max.y = FLT_MAX;
            column->ClipRect.ClipWithFull(host_clip_rect);
            column->IsClipped = true;
            continue;
        }

        // If horizontal scrolling if disabled, we apply a final lossless shrinking of columns in order to make sure they are all visible.
        // Because of this we also know that all of the columns will always fit in table->WorkRect and therefore in table->InnerRect (because ScrollX is off)
        if (!(table->Flags & ImGuiTableFlags_ScrollX))
        {
            float max_x = table->WorkRect.Max.x - (table->ColumnsActiveCount - (column->IndexWithinActiveSet + 1)) * min_column_width;
            if (offset_x + column->WidthGiven > max_x)
                column->WidthGiven = ImMax(max_x - offset_x, min_column_width);
        }

        column->MinX = offset_x;
        column->MaxX = column->MinX + column->WidthGiven;

        //// A one pixel padding on the right side makes clipping more noticeable and contents look less cramped.
        column->ClipRect.Min.x = column->MinX;
        column->ClipRect.Min.y = work_rect.Min.y;
        column->ClipRect.Max.x = column->MaxX;// -1.0f;
        column->ClipRect.Max.y = FLT_MAX;
        column->ClipRect.ClipWithFull(host_clip_rect);
        
        column->IsClipped = (column->ClipRect.Max.x <= column->ClipRect.Min.x) && (column->AutoFitQueue & 1) == 0 && (column->CannotSkipItemsQueue & 1) == 0;
        if (column->IsClipped)
        {
            // Columns with the _WidthAlwaysAutoResize sizing policy will never be updated then.
            table->VisibleMaskByIndex &= ~((ImU64)1 << column_n);
        }
        else
        {
            // Starting cursor position
            column->StartXRows = column->StartXHeaders = column->MinX + table->CellPaddingX1;

            // Alignment
            // FIXME-TABLE: This align based on the whole column width, not per-cell, and therefore isn't useful in many cases.
            // (To be able to honor this we might be able to store a log of cells width, per row, for visible rows, but nav/programmatic scroll would have visible artifacts.)
            //if (column->Flags & ImGuiTableColumnFlags_AlignRight)
            //    column->StartXRows = ImMax(column->StartXRows, column->MaxX - column->WidthContent[0]);
            //else if (column->Flags & ImGuiTableColumnFlags_AlignCenter)
            //    column->StartXRows = ImLerp(column->StartXRows, ImMax(column->StartXRows, column->MaxX - column->WidthContent[0]), 0.5f);

            // Reset content width variables
            const float initial_max_pos_x = column->MinX + table->CellPaddingX1;
            column->ContentMaxPosRowsFrozen = column->ContentMaxPosRowsUnfrozen = initial_max_pos_x;
            column->ContentMaxPosHeadersUsed = column->ContentMaxPosHeadersDesired = initial_max_pos_x;
        }

        // Don't decrement auto-fit counters until container window got a chance to submit its items
        if (table->BackupSkipItems == false)
        {
            column->AutoFitQueue >>= 1;
            column->CannotSkipItemsQueue >>= 1;
        }

        if (active_n < table->FreezeColumnsCount)
            host_clip_rect.Min.x = ImMax(host_clip_rect.Min.x, column->MaxX + 2.0f);

        offset_x += column->WidthGiven + table->CellSpacingX;
        active_n++;
    }

    // Clear Resizable flag if none of our column are actually resizable (either via an explicit _NoResize flag, either because of using _WidthAlwaysAutoResize/_WidthStretch)
    // This will hide the resizing option from the context menu.
    if (count_resizable == 0 && (table->Flags & ImGuiTableFlags_Resizable))
        table->Flags &= ~ImGuiTableFlags_Resizable;

    // Allocate draw channels
    TableUpdateDrawChannels(table);

    // Borders
    if (table->Flags & ImGuiTableFlags_Resizable)
        TableUpdateBorders(table);

    // Reset fields after we used them in TableSetupResize()
    table->LastFirstRowHeight = 0.0f;
    table->IsLayoutLocked = true;
    table->IsUsingHeaders = false;

    // Context menu
    if (table->IsContextPopupOpen && table->InstanceNo == table->InstanceInteracted)
    {
        if (BeginPopup("##TableContextMenu"))
        {
            TableDrawContextMenu(table, table->ContextPopupColumn);
            EndPopup();
        }
        else
        {
            table->IsContextPopupOpen = false;
        }
    }
}

// Process interaction on resizing borders. Actual size change will be applied in EndTable()
// - Set table->HoveredColumnBorder with a short delay/timer to reduce feedback noise
// - Submit ahead of table contents and header, use ImGuiButtonFlags_AllowItemOverlap to prioritize widgets overlapping the same area.
void    ImGui::TableUpdateBorders(ImGuiTable* table)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(table->Flags & ImGuiTableFlags_Resizable);

    // At this point OuterRect height may be zero or under actual final height, so we rely on temporal coherency and use
    // the final height from last frame. Because this is only affecting _interaction_ with columns, it is not really problematic.
    // (whereas the actual visual will be displayed in EndTable() and using the current frame height)
    // Actual columns highlight/render will be performed in EndTable() and not be affected.
    const bool borders_full_height = (table->IsUsingHeaders == false) || (table->Flags & ImGuiTableFlags_BordersFullHeight);
    const float hit_half_width = TABLE_RESIZE_SEPARATOR_HALF_THICKNESS;
    const float hit_y1 = table->OuterRect.Min.y;
    const float hit_y2_full = ImMax(table->OuterRect.Max.y, hit_y1 + table->LastOuterHeight);
    const float hit_y2 = borders_full_height ? hit_y2_full : (hit_y1 + table->LastFirstRowHeight);
    const float mouse_x_hover_body = (g.IO.MousePos.y >= hit_y1 && g.IO.MousePos.y < hit_y2_full) ? g.IO.MousePos.x : FLT_MAX;

    for (int order_n = 0; order_n < table->ColumnsCount; order_n++)
    {
        if (!(table->ActiveMaskByDisplayOrder & ((ImU64)1 << order_n)))
            continue;

        const int column_n = table->DisplayOrder[order_n];
        ImGuiTableColumn* column = &table->Columns[column_n];

        // Detect hovered column:
        // - we perform an unusually low-level check here.. not using IsMouseHoveringRect() to avoid touch padding.
        // - we don't care about the full set of IsItemHovered() feature either.
        if (mouse_x_hover_body >= column->MinX && mouse_x_hover_body < column->MaxX)
            table->HoveredColumnBody = (ImS8)column_n;

        if (column->Flags & (ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoDirectResize_))
            continue;

        ImGuiID column_id = table->ID + (table->InstanceNo * table->ColumnsCount) + column_n;
        ImRect hit_rect(column->MaxX - hit_half_width, hit_y1, column->MaxX + hit_half_width, hit_y2);
        //GetForegroundDrawList()->AddRect(hit_rect.Min, hit_rect.Max, IM_COL32(255, 0, 0, 100));
        KeepAliveID(column_id);

        bool hovered = false, held = false;
        bool pressed = ButtonBehavior(hit_rect, column_id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_AllowItemOverlap | ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnDoubleClick);
        if (pressed && IsMouseDoubleClicked(0) && !(column->Flags & ImGuiTableColumnFlags_WidthStretch))
        {
            // FIXME-TABLE: Double-clicking on column edge could auto-fit weighted column?
            TableSetColumnAutofit(table, column_n);
            ClearActiveID();
            held = hovered = false;
        }
        if (held)
        {
            table->ResizedColumn = (ImS8)column_n;
            table->InstanceInteracted = table->InstanceNo;
        }
        if ((hovered && g.HoveredIdTimer > TABLE_RESIZE_SEPARATOR_FEEDBACK_TIMER) || held)
        {
            table->HoveredColumnBorder = (ImS8)column_n;
            SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
    }
}

void    ImGui::EndTable()
{
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;
    IM_ASSERT(table != NULL && "Only call EndTable() is BeginTable() returns true!");

    const ImGuiTableFlags flags = table->Flags;
    ImGuiWindow* inner_window = table->InnerWindow;
    ImGuiWindow* outer_window = table->OuterWindow;
    IM_ASSERT(inner_window == g.CurrentWindow);
    IM_ASSERT(outer_window == inner_window || outer_window == inner_window->ParentWindow);

    if (table->IsInsideRow)
        TableEndRow(table);

    // Finalize table height
    inner_window->SkipItems = table->BackupSkipItems;
    inner_window->DC.CursorMaxPos = table->BackupCursorMaxPos;
    if (inner_window != outer_window)
    {
        table->OuterRect.Max.y = ImMax(table->OuterRect.Max.y, inner_window->Pos.y + inner_window->Size.y);
        inner_window->DC.CursorMaxPos.y = table->RowPosY2;
    }
    else if (!(flags & ImGuiTableFlags_NoHostExtendY))
    {
        table->OuterRect.Max.y = ImMax(table->OuterRect.Max.y, inner_window->DC.CursorPos.y);
        inner_window->DC.CursorMaxPos.y = table->RowPosY2;
    }
    table->WorkRect.Max.y = ImMax(table->WorkRect.Max.y, table->OuterRect.Max.y);
    table->LastOuterHeight = table->OuterRect.GetHeight();

    // Store content width reference for each column
    float max_pos_x = inner_window->DC.CursorMaxPos.x;
    for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
    {
        ImGuiTableColumn* column = &table->Columns[column_n];

        // Store content width (for both Headers and Rows)
        //float ref_x = column->MinX;
        float ref_x_rows = column->StartXRows - table->CellPaddingX1;
        float ref_x_headers = column->StartXHeaders - table->CellPaddingX1;
        column->ContentWidthRowsFrozen = (ImS16)ImMax(0.0f, column->ContentMaxPosRowsFrozen - ref_x_rows);
        column->ContentWidthRowsUnfrozen = (ImS16)ImMax(0.0f, column->ContentMaxPosRowsUnfrozen - ref_x_rows);
        column->ContentWidthHeadersUsed = (ImS16)ImMax(0.0f, column->ContentMaxPosHeadersUsed - ref_x_headers);
        column->ContentWidthHeadersDesired = (ImS16)ImMax(0.0f, column->ContentMaxPosHeadersDesired - ref_x_headers);

        if (table->ActiveMaskByIndex & ((ImU64)1 << column_n))
            max_pos_x = ImMax(max_pos_x, column->MaxX);
    }

    // Add an extra 1 pixel so we can see the last column vertical line if it lies on the right-most edge.
    inner_window->DC.CursorMaxPos.x = max_pos_x + 1;

    if (!(flags & ImGuiTableFlags_NoClipX))
        inner_window->DrawList->PopClipRect();
    inner_window->ClipRect = inner_window->DrawList->_ClipRectStack.back();

    // Draw borders
    if ((flags & ImGuiTableFlags_Borders) != 0)
        TableDrawBorders(table);

    // Flatten channels and merge draw calls
    table->DrawSplitter.SetCurrentChannel(inner_window->DrawList, 0);
    TableDrawMergeChannels(table);

    // When releasing a column being resized, scroll to keep the resulting column in sight
    const float min_column_width = TableGetMinColumnWidth();
    if (!(table->Flags & ImGuiTableFlags_ScrollX) && inner_window != outer_window)
    {
        inner_window->Scroll.x = 0.0f;
    }
    else if (table->LastResizedColumn != -1 && table->ResizedColumn == -1 && inner_window->ScrollbarX)
    {
        ImGuiTableColumn* column = &table->Columns[table->LastResizedColumn];
        if (column->MaxX < table->InnerClipRect.Min.x)
            SetScrollFromPosX(inner_window, column->MaxX - inner_window->Pos.x - min_column_width, 1.0f);
        else if (column->MaxX > table->InnerClipRect.Max.x)
            SetScrollFromPosX(inner_window, column->MaxX - inner_window->Pos.x + min_column_width, 1.0f);
    }

    // Apply resizing/dragging at the end of the frame
    if (table->ResizedColumn != -1)
    {
        ImGuiTableColumn* column = &table->Columns[table->ResizedColumn];
        const float new_x2 = (g.IO.MousePos.x - g.ActiveIdClickOffset.x + TABLE_RESIZE_SEPARATOR_HALF_THICKNESS);
        const float new_width = ImFloor(new_x2 - column->MinX);
        table->ResizedColumnNextWidth = new_width;
    }

    // Layout in outer window
    inner_window->WorkRect = table->BackupWorkRect;
    inner_window->SkipItems = table->BackupSkipItems;
    outer_window->DC.CursorPos = table->OuterRect.Min;
    outer_window->DC.ColumnsOffset.x = 0.0f;
    if (inner_window != outer_window)
    {
        // Override EndChild's ItemSize with our own to enable auto-resize on the X axis when possible
        float backup_outer_cursor_pos_x = outer_window->DC.CursorPos.x;
        EndChild();
        outer_window->DC.CursorMaxPos.x = backup_outer_cursor_pos_x + table->ColumnsTotalWidth + 1.0f + inner_window->ScrollbarSizes.x;
    }
    else
    {
        PopID();
        ImVec2 item_size = table->OuterRect.GetSize();
        item_size.x = table->ColumnsTotalWidth;
        ItemSize(item_size);
    }

    // Save settings
    if (table->IsSettingsDirty)
        TableSaveSettings(table);

    // Clear or restore current table, if any
    IM_ASSERT(g.CurrentWindow == outer_window);
    IM_ASSERT(g.CurrentTable == table);
    outer_window->DC.CurrentTable = NULL;
    g.CurrentTableStack.pop_back();
    g.CurrentTable = g.CurrentTableStack.Size ? g.Tables.GetByIndex(g.CurrentTableStack.back().Index) : NULL;
}

void ImGui::TableDrawBorders(ImGuiTable* table)
{
    ImGuiWindow* inner_window = table->InnerWindow;
    ImGuiWindow* outer_window = table->OuterWindow;
    table->DrawSplitter.SetCurrentChannel(inner_window->DrawList, 0);
    if (inner_window->Hidden || !table->HostClipRect.Overlaps(table->InnerClipRect))
        return;

    // Draw inner border and resizing feedback
    const float draw_y1 = table->OuterRect.Min.y;
    float draw_y2_base = (table->FreezeRowsCount >= 1 ? table->OuterRect.Min.y : table->WorkRect.Min.y) + table->LastFirstRowHeight;
    float draw_y2_full = table->OuterRect.Max.y;
    ImU32 border_base_col;
    if (!table->IsUsingHeaders || (table->Flags & ImGuiTableFlags_BordersFullHeight))
    {
        draw_y2_base = draw_y2_full;
        border_base_col = table->BorderInnerColor;
    }
    else
    {
        border_base_col = table->BorderOuterColor;
    }

    if (table->Flags & ImGuiTableFlags_BordersV)
    {
        const bool draw_left_most_border = (table->Flags & ImGuiTableFlags_BordersOuter) == 0;
        if (draw_left_most_border)
            inner_window->DrawList->AddLine(ImVec2(table->OuterRect.Min.x, draw_y1), ImVec2(table->OuterRect.Min.x, draw_y2_base), border_base_col, 1.0f);

        for (int order_n = 0; order_n < table->ColumnsCount; order_n++)
        {
            if (!(table->ActiveMaskByDisplayOrder & ((ImU64)1 << order_n)))
                continue;

            const int column_n = table->DisplayOrder[order_n];
            ImGuiTableColumn* column = &table->Columns[column_n];
            const bool is_hovered = (table->HoveredColumnBorder == column_n);
            const bool is_resized = (table->ResizedColumn == column_n) && (table->InstanceInteracted == table->InstanceNo);
            const bool draw_right_border = (column->MaxX <= table->InnerClipRect.Max.x) || (is_resized || is_hovered);
            if (draw_right_border && column->MaxX > column->ClipRect.Min.x) // FIXME-TABLE FIXME-STYLE: Assume BorderSize==1, this is problematic if we want to increase the border size..
            {
                // Draw in outer window so right-most column won't be clipped
                // Always draw full height border when:
                // - not using headers
                // - user specify ImGuiTableFlags_BordersFullHeight
                // - being interacted with
                // - on the delimitation of frozen column scrolling
                const ImU32 col = is_resized ? GetColorU32(ImGuiCol_SeparatorActive) : is_hovered ? GetColorU32(ImGuiCol_SeparatorHovered) : border_base_col;
                float draw_y2 = draw_y2_base;
                if (is_hovered || is_resized || (table->FreezeColumnsCount != -1 && table->FreezeColumnsCount == order_n + 1))
                    draw_y2 = draw_y2_full;
                inner_window->DrawList->AddLine(ImVec2(column->MaxX, draw_y1), ImVec2(column->MaxX, draw_y2), col, 1.0f);
            }
        }
    }

    // Draw outer border
    if (table->Flags & ImGuiTableFlags_BordersOuter)
    {
        // Display outer border offset by 1 which is a simple way to display it without adding an extra draw call
        // (Without the offset, in outer_window it would be rendered behind cells, because child windows are above their parent.
        // In inner_window, it won't reach out over scrollbars. Another weird solution would be to display part of it in inner window,
        // and the part that's over scrollbars in the outer window..)
        // Either solution currently won't allow us to use a larger border size: the border would clipped.
        ImRect outer_border = table->OuterRect;
        if (inner_window != outer_window)
            outer_border.Expand(1.0f);
        outer_window->DrawList->AddRect(outer_border.Min, outer_border.Max, table->BorderOuterColor); // IM_COL32(255, 0, 0, 255));
    }
    else if (table->Flags & ImGuiTableFlags_BordersH)
    {
        // Draw bottom-most border
        const float border_y = table->RowPosY2;
        if (border_y >= table->BackgroundClipRect.Min.y && border_y < table->BackgroundClipRect.Max.y)
            inner_window->DrawList->AddLine(ImVec2(table->BorderX1, border_y), ImVec2(table->BorderX2, border_y), table->BorderOuterColor);
    }
}

static void TableUpdateColumnsWeightFromWidth(ImGuiTable* table)
{
    IM_ASSERT(table->LeftMostStretchedColumnDisplayOrder != -1);

    // Measure existing quantity
    float visible_weight = 0.0f;
    float visible_width = 0.0f;
    for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
    {
        ImGuiTableColumn* column = &table->Columns[column_n];
        if (!column->IsActive || !(column->Flags & ImGuiTableColumnFlags_WidthStretch))
            continue;
        visible_weight += column->ResizeWeight;
        visible_width += column->WidthRequested;
    }
    IM_ASSERT(visible_weight > 0.0f && visible_width > 0.0f);

    // Apply new weights
    for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
    {
        ImGuiTableColumn* column = &table->Columns[column_n];
        if (!column->IsActive || !(column->Flags & ImGuiTableColumnFlags_WidthStretch))
            continue;
        column->ResizeWeight = (column->WidthRequested + 0.0f) / visible_width;
    }
}

void ImGui::TableSetColumnWidth(ImGuiTable* table, ImGuiTableColumn* column_0, float column_0_width)
{
    // Constraints
    float min_width = TableGetMinColumnWidth();
    float max_width_0 = FLT_MAX;
    if (!(table->Flags & ImGuiTableFlags_ScrollX))
        max_width_0 = (table->WorkRect.Max.x - column_0->MinX) - (table->ColumnsActiveCount - (column_0->IndexWithinActiveSet + 1)) * min_width;
    column_0_width = ImClamp(column_0_width, min_width, max_width_0);

    // Compare both requested and actual given width to avoid overwriting requested width when column is stuck (minimum size, bounded)
    if (column_0->WidthGiven == column_0_width || column_0->WidthRequested == column_0_width)
        return;

    ImGuiTableColumn* column_1 = (column_0->NextActiveColumn != -1) ? &table->Columns[column_0->NextActiveColumn] : NULL;

    // In this surprisingly not simple because of how we support mixing Fixed and Stretch columns.
    // When forwarding resize from Wn| to Fn+1| we need to be considerate of the _NoResize flag on Fn+1.
    // FIXME-TABLE: Find a way to rewrite all of this so interactions feel more consistent for the user.
    // Scenarios:
    // - F1 F2 F3  resize from F1| or F2|   --> ok: alter ->WidthRequested of Fixed column. Subsequent columns will be offset.
    // - F1 F2 F3  resize from F3|          --> ok: alter ->WidthRequested of Fixed column. If active, ScrollX extent can be altered.
    // - F1 F2 W3  resize from F1| or F2|   --> ok: alter ->WidthRequested of Fixed column. If active, ScrollX extent can be altered, but it doesn't make much sense as the Weighted column will always be minimal size.
    // - F1 F2 W3  resize from W3|          --> ok: no-op (disabled by Resize Rule 1)
    // - W1 W2 W3  resize from W1| or W2|   --> FIXME
    // - W1 W2 W3  resize from W3|          --> ok: no-op (disabled by Resize Rule 1)
    // - W1 F2 F3  resize from F3|          --> ok: no-op (disabled by Resize Rule 1)
    // - W1 F2     resize from F2|          --> ok: no-op (disabled by Resize Rule 1)
    // - W1 W2 F3  resize from W1| or W2|   --> ok
    // - W1 F2 W3  resize from W1| or F2|   --> FIXME
    // - F1 W2 F3  resize from W2|          --> ok
    // - W1 F2 F3  resize from W1|          --> ok: equivalent to resizing |F2. F3 will not move. (forwarded by Resize Rule 2)
    // - W1 F2 F3  resize from F2|          --> FIXME should resize F2, F3 and not have effect on W1 (Stretch columns are _before_ the Fixed column).

    // Rules:
    // - [Resize Rule 1] Can't resize from right of right-most visible column if there is any Stretch column. Implemented in TableUpdateLayout().
    // - [Resize Rule 2] Resizing from right-side of a Stretch column before a fixed column froward sizing to left-side of fixed column.
    // - [Resize Rule 3] If we are are followed by a fixed column and we have a Stretch column before, we need to ensure that our left border won't move.

    if (column_0->Flags & ImGuiTableColumnFlags_WidthFixed)
    {
        // [Resize Rule 3] If we are are followed by a fixed column and we have a Stretch column before, we need to
        // ensure that our left border won't move, which we can do by making sure column_a/column_b resizes cancels each others.
        if (column_1 && (column_1->Flags & ImGuiTableColumnFlags_WidthFixed))
            if (table->LeftMostStretchedColumnDisplayOrder != -1 && table->LeftMostStretchedColumnDisplayOrder < column_0->IndexDisplayOrder)
            {
                // (old_a + old_b == new_a + new_b) --> (new_a == old_a + old_b - new_b)
                float column_1_width = ImMax(column_1->WidthRequested - (column_0_width - column_0->WidthRequested), min_width);
                column_0_width = column_0->WidthRequested + column_1->WidthRequested - column_1_width;
                column_1->WidthRequested = column_1_width;
            }

        // Apply
        //IMGUI_DEBUG_LOG("TableSetColumnWidth(%d, %.1f->%.1f)\n", column_0_idx, column_0->WidthRequested, column_0_width);
        column_0->WidthRequested = column_0_width;
    }
    else if (column_0->Flags & ImGuiTableColumnFlags_WidthStretch)
    {
        // [Resize Rule 2]
        if (column_1 && (column_1->Flags & ImGuiTableColumnFlags_WidthFixed))
        {
            float off = (column_0->WidthGiven - column_0_width);
            float column_1_width = column_1->WidthGiven + off;
            column_1->WidthRequested = ImMax(min_width, column_1_width);
            return;
        }

        // (old_a + old_b == new_a + new_b) --> (new_a == old_a + old_b - new_b)
        float column_1_width = ImMax(column_1->WidthRequested - (column_0_width - column_0->WidthRequested), min_width);
        column_0_width = column_0->WidthRequested + column_1->WidthRequested - column_1_width;
        column_1->WidthRequested = column_1_width;
        column_0->WidthRequested = column_0_width;
        TableUpdateColumnsWeightFromWidth(table);
    }
    table->IsSettingsDirty = true;
}

// Columns where the contents didn't stray off their local clip rectangle can be merged into a same draw command.
// To achieve this we merge their clip rect and make them contiguous in the channel list so they can be merged.
// So here we'll reorder the draw cmd which can be merged, by arranging them into a maximum of 4 distinct groups:
//
//   1 group:               2 groups:              2 groups:              4 groups:
//   [ 0. ] no freeze       [ 0. ] row freeze      [ 01 ] col freeze      [ 01 ] row+col freeze
//   [ .. ]  or no scroll   [ 1. ]  and v-scroll   [ .. ]  and h-scroll   [ 23 ]  and v+h-scroll
//
// Each column itself can use 1 channel (row freeze disabled) or 2 channels (row freeze enabled).
// When the contents of a column didn't stray off its limit, we move its channels into the corresponding group
// based on its position (within frozen rows/columns set or not).
// At the end of the operation our 1-4 groups will each have a ImDrawCmd using the same ClipRect, and they will be merged by the DrawSplitter.Merge() call.
//
// Column channels will not be merged into one of the 1-4 groups in the following cases:
// - The contents stray off its clipping rectangle (we only compare the MaxX value, not the MinX value).
//   Direct ImDrawList calls won't be noticed so if you use them make sure the ImGui:: bounds matches, by e.g. calling SetCursorScreenPos().
// - The channel uses more than one draw command itself (we drop all our merging stuff here.. we could do better but it's going to be rare)
//
// This function is particularly tricky to understand.. take a breath.
void    ImGui::TableDrawMergeChannels(ImGuiTable* table)
{
    ImGuiContext& g = *GImGui;
    ImDrawListSplitter* splitter = &table->DrawSplitter;
    const bool is_frozen_v = (table->FreezeRowsCount > 0);
    const bool is_frozen_h = (table->FreezeColumnsCount > 0);

    int merge_set_mask = 0;
    int merge_set_channels_count[4] = { 0 };
    ImU64 merge_set_channels_mask[4] = { 0 };
    ImRect merge_set_clip_rect[4];
    for (int n = 0; n < IM_ARRAYSIZE(merge_set_clip_rect); n++)
        merge_set_clip_rect[n] = ImVec4(+FLT_MAX, +FLT_MAX, -FLT_MAX, -FLT_MAX);
    bool merge_set_all_fit_within_inner_rect = (table->Flags & ImGuiTableFlags_NoHostExtendY) == 0;

    // 1. Scan channels and take note of those who can be merged
    for (int order_n = 0; order_n < table->ColumnsCount; order_n++)
    {
        if (!(table->ActiveMaskByDisplayOrder & ((ImU64)1 << order_n)))
            continue;
        const int column_n = table->DisplayOrder[order_n];
        ImGuiTableColumn* column = &table->Columns[column_n];

        const int merge_set_sub_count = is_frozen_v ? 2 : 1;
        for (int merge_set_sub_n = 0; merge_set_sub_n < merge_set_sub_count; merge_set_sub_n++)
        {
            const int channel_no = (merge_set_sub_n == 0) ? column->DrawChannelRowsBeforeFreeze : column->DrawChannelRowsAfterFreeze;

            // Don't attempt to merge if there are multiple calls within the column
            ImDrawChannel* src_channel = &splitter->_Channels[channel_no];
            if (src_channel->_CmdBuffer.Size > 0 && src_channel->_CmdBuffer.back().ElemCount == 0)
                src_channel->_CmdBuffer.pop_back();
            if (src_channel->_CmdBuffer.Size != 1)
                continue;

            // Find out the width of this merge set and check if it will fit in our column.
            float width_contents;
            if (merge_set_sub_count == 1)   // No row freeze (same as testing !is_frozen_v)
                width_contents = ImMax(column->ContentWidthRowsUnfrozen, column->ContentWidthHeadersUsed);
            else if (merge_set_sub_n == 0)  // Row freeze: use width before freeze
                width_contents = ImMax(column->ContentWidthRowsFrozen, column->ContentWidthHeadersUsed);
            else                            // Row freeze: use width after freeze
                width_contents = column->ContentWidthRowsUnfrozen;
            if (width_contents > column->WidthGiven && !(column->Flags & ImGuiTableColumnFlags_NoClipX))
                continue;

            const int dst_merge_set_n = (is_frozen_h && column_n < table->FreezeColumnsCount ? 0 : 2) + (is_frozen_v ? merge_set_sub_n : 1);
            IM_ASSERT(merge_set_channels_count[dst_merge_set_n] < (int)sizeof(merge_set_channels_mask[dst_merge_set_n]) * 8);
            merge_set_mask |= (1 << dst_merge_set_n);
            merge_set_channels_mask[dst_merge_set_n] |= (ImU64)1 << channel_no;
            merge_set_channels_count[dst_merge_set_n]++;
            merge_set_clip_rect[dst_merge_set_n].Add(src_channel->_CmdBuffer[0].ClipRect);

            // If we end with a single set and hosted by the outer window, we'll attempt to merge our draw command with
            // the existing outer window command. But we can only do so if our columns all fit within the expected clip rect,
            // otherwise clipping will be incorrect when ScrollX is disabled.
            // FIXME-TABLE FIXME-WORKRECT: We are wasting a merge opportunity on tables without scrolling if column don't fit within host clip rect, solely because of the half-padding difference between window->WorkRect and window->InnerClipRect

            // 2019/10/22: (1) This is breaking table_2_draw_calls but I cannot seem to repro what it is attempting to fix...
            // cf git fce2e8dc "Fixed issue with clipping when outerwindow==innerwindow / support ScrollH without ScrollV."
            // 2019/10/22: (2) Clamping code in TableUpdateLayout() seemingly made this not necessary...
#if 0
            if (column->MinX < table->InnerClipRect.Min.x || column->MaxX > table->InnerClipRect.Max.x)
                merge_set_all_fit_within_inner_rect = false;
#endif
        }

        // Invalidate current draw channel (we don't clear DrawChannelBeforeRowFreeze/DrawChannelAfterRowFreeze solely to facilitate debugging)
        column->DrawChannelCurrent = -1;
    }

    // 2. Rewrite channel list in our preferred order
    if (merge_set_mask != 0)
    {
        // Use shared temporary storage so the allocation gets amortized
        g.DrawChannelsTempMergeBuffer.resize(splitter->_Count - 1);
        ImDrawChannel* dst_tmp = g.DrawChannelsTempMergeBuffer.Data;
        ImU64 remaining_mask = ((splitter->_Count < 64) ? ((ImU64)1 << splitter->_Count) - 1 : ~(ImU64)0) & ~1;
        const bool may_extend_clip_rect_to_host_rect = ImIsPowerOfTwo(merge_set_mask);
        for (int merge_set_n = 0; merge_set_n < 4; merge_set_n++)
            if (merge_set_channels_count[merge_set_n])
            {
                ImU64 merge_channels_mask = merge_set_channels_mask[merge_set_n];
                ImRect merge_clip_rect = merge_set_clip_rect[merge_set_n];
                if (may_extend_clip_rect_to_host_rect)
                {
                    //GetOverlayDrawList()->AddRect(table->HostClipRect.Min, table->HostClipRect.Max, IM_COL32(255, 0, 0, 200), 0.0f, ~0, 3.0f);
                    //GetOverlayDrawList()->AddRect(table->InnerClipRect.Min, table->InnerClipRect.Max, IM_COL32(0, 255, 0, 200), 0.0f, ~0, 1.0f);
                    //GetOverlayDrawList()->AddRect(merge_clip_rect.Min, merge_clip_rect.Max, IM_COL32(255, 0, 0, 200), 0.0f, ~0, 2.0f);
                    merge_clip_rect.Add(merge_set_all_fit_within_inner_rect ? table->HostClipRect : table->InnerClipRect);
                    //GetOverlayDrawList()->AddRect(merge_clip_rect.Min, merge_clip_rect.Max, IM_COL32(0, 255, 0, 200));
                }
                remaining_mask &= ~merge_channels_mask;
                for (int n = 0; n < splitter->_Count && merge_channels_mask != 0; n++)
                {
                    // Copy + overwrite new clip rect
                    const ImU64 n_mask = (ImU64)1 << n;
                    if ((merge_channels_mask & n_mask) == 0)
                        continue;
                    ImDrawChannel* channel = &splitter->_Channels[n];
                    IM_ASSERT(channel->_CmdBuffer.Size == 1 && merge_clip_rect.Contains(ImRect(channel->_CmdBuffer[0].ClipRect)));
                    channel->_CmdBuffer[0].ClipRect = *(ImVec4*)&merge_clip_rect;
                    memcpy(dst_tmp++, channel, sizeof(ImDrawChannel));
                    merge_channels_mask &= ~n_mask;
                }
            }

        // Append channels that we didn't reorder at the end of the list
        for (int n = 0; n < splitter->_Count && remaining_mask != 0; n++)
        {
            const ImU64 n_mask = (ImU64)1 << n;
            if ((remaining_mask & n_mask) == 0)
                continue;
            ImDrawChannel* channel = &splitter->_Channels[n];
            memcpy(dst_tmp++, channel, sizeof(ImDrawChannel));
            remaining_mask &= ~n_mask;
        }
        IM_ASSERT(dst_tmp == g.DrawChannelsTempMergeBuffer.Data + g.DrawChannelsTempMergeBuffer.Size);
        memcpy(splitter->_Channels.Data + 1, g.DrawChannelsTempMergeBuffer.Data, (splitter->_Count - 1) * sizeof(ImDrawChannel));
    }

    // 3. Actually merge (channels using the same clip rect will be contiguous and naturally merged)
    splitter->Merge(table->InnerWindow->DrawList);
}

// We use a default parameter of 'init_width_or_weight == -1'
//  ImGuiTableColumnFlags_WidthFixed,    width  <= 0 --> init width == auto
//  ImGuiTableColumnFlags_WidthFixed,    width  >  0 --> init width == manual
//  ImGuiTableColumnFlags_WidthStretch,  weight <  0 --> init weight == 1.0f
//  ImGuiTableColumnFlags_WidthStretch,  weight >= 0 --> init weight == custom
// Use a different API?
void    ImGui::TableSetupColumn(const char* label, ImGuiTableColumnFlags flags, float init_width_or_weight, ImGuiID user_id)
{
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;
    IM_ASSERT(table != NULL && "Need to call TableSetupColumn() after BeginTable()!");
    IM_ASSERT(!table->IsLayoutLocked && "Need to call call TableSetupColumn() before first row!");
    IM_ASSERT(table->DeclColumnsCount >= 0 && table->DeclColumnsCount < table->ColumnsCount && "Called TableSetupColumn() too many times!");

    ImGuiTableColumn* column = &table->Columns[table->DeclColumnsCount];
    table->DeclColumnsCount++;

    // When passing a width automatically enforce WidthFixed policy (vs TableFixColumnFlags would default to WidthAlwaysAutoResize)
    // (we write down to FlagsIn which is a little misleading, another solution would be to pass init_width_or_weight to TableFixColumnFlags)
    if ((flags & ImGuiTableColumnFlags_WidthMask_) == 0)
        if ((table->Flags & ImGuiTableFlags_SizingPolicyFixedX) && (init_width_or_weight > 0.0f))
            flags |= ImGuiTableColumnFlags_WidthFixed;

    column->UserID = user_id;
    column->FlagsIn = flags;
    column->Flags = TableFixColumnFlags(table, column->FlagsIn);
    flags = column->Flags;

    // Initialize defaults
    if (table->IsFirstFrame && !table->IsSettingsLoaded)
    {
        // Init width or weight
        // Disable auto-fit if a default fixed width has been specified
        if ((flags & ImGuiTableColumnFlags_WidthFixed) && init_width_or_weight > 0.0f)
        {
            column->WidthRequested = init_width_or_weight;
            column->AutoFitQueue = 0x00;
        }
        if (flags & ImGuiTableColumnFlags_WidthStretch)
        {
            IM_ASSERT(init_width_or_weight < 0.0f || init_width_or_weight > 0.0f);
            column->ResizeWeight = (init_width_or_weight < 0.0f ? 1.0f : init_width_or_weight);
        }
        else
        {
            column->ResizeWeight = 1.0f;
        }

        // Init default visibility/sort state
        if (flags & ImGuiTableColumnFlags_DefaultHide)
            column->IsActive = column->NextIsActive = false;
        if (flags & ImGuiTableColumnFlags_DefaultSort)
            column->SortOrder = 0; // Multiple columns using _DefaultSort will be reordered when building the sort specs.
    }

    // Store name (append with zero-terminator in contiguous buffer)
    IM_ASSERT(column->NameOffset == -1);
    if (label != NULL)
    {
        column->NameOffset = (ImS16)table->ColumnsNames.size();
        table->ColumnsNames.append(label, label + strlen(label) + 1);
    }
}

// Starts into the first cell of a new row
void    ImGui::TableNextRow(ImGuiTableRowFlags row_flags, float min_row_height)
{
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;

    if (table->CurrentRow == -1)
        TableUpdateLayout(table);
    else if (table->IsInsideRow)
        TableEndRow(table);

    table->LastRowFlags = table->RowFlags;
    table->RowFlags = row_flags;
    TableBeginRow(table);

    // We honor min_height requested by user, but cannot guarantee per-row maximum height as that would essentially require a unique clipping rectangle per-cell.
    table->RowPosY2 += min_row_height;

    TableBeginCell(table, 0);
}

// [Internal]
void    ImGui::TableBeginRow(ImGuiTable* table)
{
    ImGuiWindow* window = table->InnerWindow;
    IM_ASSERT(!table->IsInsideRow);

    // New row
    table->CurrentRow++;
    table->CurrentColumn = -1;
    table->RowBgColor = IM_COL32_DISABLE;
    table->IsInsideRow = true;

    // Begin frozen rows
    float next_y1 = table->RowPosY2;
    if (table->CurrentRow == 0 && table->FreezeRowsCount > 0)
        next_y1 = window->DC.CursorPos.y = table->OuterRect.Min.y;

    table->RowPosY1 = table->RowPosY2 = next_y1;
    table->RowTextBaseline = 0.0f;
    window->DC.CursorMaxPos.y = next_y1;

    // Making the header BG color non-transparent will allow us to overlay it multiple times when handling smooth dragging.
    if (table->RowFlags & ImGuiTableRowFlags_Headers)
    {
        table->RowBgColor = GetColorU32(ImGuiCol_TableHeaderBg);
        if (table->CurrentRow == 0)
            table->IsUsingHeaders = true;
    }
}

// [Internal]
void    ImGui::TableEndRow(ImGuiTable* table)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(window == table->InnerWindow);
    IM_ASSERT(table->IsInsideRow);

    TableEndCell(table);

    table->RowPosY2 += table->CellPaddingY;

    // Position cursor at the bottom of our row so it can be used for e.g. clipping calculation.
    // However it is likely that the next call to TableBeginCell() will reposition the cursor to take account of vertical padding.
    window->DC.CursorPos.y = table->RowPosY2;

    // Row background fill
    const float bg_y1 = table->RowPosY1;
    const float bg_y2 = table->RowPosY2;

    if (table->CurrentRow == 0)
        table->LastFirstRowHeight = bg_y2 - bg_y1;

    if (table->CurrentRow >= 0 && bg_y2 >= table->InnerClipRect.Min.y && bg_y1 <= table->InnerClipRect.Max.y)
    {
        // Decide of background color for the row
        ImU32 bg_col = 0;
        if (table->RowBgColor != IM_COL32_DISABLE)
            bg_col = table->RowBgColor;
        else if (table->Flags & ImGuiTableFlags_RowBg)
            bg_col = GetColorU32((table->RowBgColorCounter & 1) ? ImGuiCol_TableRowBgAlt : ImGuiCol_TableRowBg);

        // Decide of separating border color
        ImU32 border_col = 0;
        if (table->CurrentRow != 0 || table->InnerWindow == table->OuterWindow)
        {
            if (table->Flags & ImGuiTableFlags_BordersH)
            {
                if (table->CurrentRow == 0 && table->InnerWindow == table->OuterWindow)
                    border_col = table->BorderOuterColor;
                else if (!(table->LastRowFlags & ImGuiTableRowFlags_Headers))
                    border_col = table->BorderInnerColor;
            }
            else
            {
                if (table->RowFlags & ImGuiTableRowFlags_Headers)
                    border_col = table->BorderOuterColor;
            }
        }

        if (bg_col != 0 || border_col != 0)
            table->DrawSplitter.SetCurrentChannel(window->DrawList, 0);

        // Draw background
        // We soft/cpu clip this so all backgrounds and borders can share the same clipping rectangle
        if (bg_col)
        {
            ImRect bg_rect(table->WorkRect.Min.x, bg_y1, table->WorkRect.Max.x, bg_y2);
            bg_rect.ClipWith(table->BackgroundClipRect);
            if (bg_rect.Min.y < bg_rect.Max.y)
                window->DrawList->AddRectFilledMultiColor(bg_rect.Min, bg_rect.Max, bg_col, bg_col, bg_col, bg_col);
        }

        // Draw top border
        const float border_y = bg_y1;
        if (border_col && border_y >= table->BackgroundClipRect.Min.y && border_y < table->BackgroundClipRect.Max.y)
            window->DrawList->AddLine(ImVec2(table->BorderX1, border_y), ImVec2(table->BorderX2, border_y), border_col);
    }

    const bool unfreeze_rows = (table->CurrentRow + 1 == table->FreezeRowsCount && table->FreezeRowsCount > 0);

    // Draw bottom border (always strong)
    const bool draw_separating_border = unfreeze_rows || (table->RowFlags & ImGuiTableRowFlags_Headers);
    if (draw_separating_border)
        if (bg_y2 >= table->BackgroundClipRect.Min.y && bg_y2 < table->BackgroundClipRect.Max.y)
            window->DrawList->AddLine(ImVec2(table->BorderX1, bg_y2), ImVec2(table->BorderX2, bg_y2), table->BorderOuterColor);

    // End frozen rows (when we are past the last frozen row line, teleport cursor and alter clipping rectangle)
    // We need to do that in TableEndRow() instead of TableBeginRow() so the list clipper can mark end of row and get the new cursor position.
    if (unfreeze_rows)
    {
        IM_ASSERT(table->IsFreezeRowsPassed == false);
        table->IsFreezeRowsPassed = true;
        table->DrawSplitter.SetCurrentChannel(window->DrawList, 0);

        ImRect r;
        r.Min.x = table->InnerClipRect.Min.x;
        r.Min.y = ImMax(table->RowPosY2 + 1, window->InnerClipRect.Min.y);
        r.Max.x = table->InnerClipRect.Max.x;
        r.Max.y = window->InnerClipRect.Max.y;
        table->BackgroundClipRect = r;

        float row_height = table->RowPosY2 - table->RowPosY1;
        table->RowPosY2 = window->DC.CursorPos.y = table->WorkRect.Min.y + table->RowPosY2 - table->OuterRect.Min.y;
        table->RowPosY1 = table->RowPosY2 - row_height;
        for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
        {
            ImGuiTableColumn* column = &table->Columns[column_n];
            column->DrawChannelCurrent = column->DrawChannelRowsAfterFreeze;
            column->ClipRect.Min.y = r.Min.y;
        }
    }

    if (!(table->RowFlags & ImGuiTableRowFlags_Headers))
        table->RowBgColorCounter++;
    table->IsInsideRow = false;
}

// [Internal] Called by TableNextRow()TableNextCell()!
// This is called a lot, so we need to be mindful of unnecessary overhead.
void    ImGui::TableBeginCell(ImGuiTable* table, int column_no)
{
    table->CurrentColumn = column_no;
    ImGuiTableColumn* column = &table->Columns[column_no];
    ImGuiWindow* window = table->InnerWindow;

    const float start_x = (table->RowFlags & ImGuiTableRowFlags_Headers) ? column->StartXHeaders : column->StartXRows;

    window->DC.LastItemId = 0;
    window->DC.CursorPos = ImVec2(start_x, table->RowPosY1 + table->CellPaddingY);
    window->DC.CursorMaxPos.x = window->DC.CursorPos.x;
    window->DC.ColumnsOffset.x = start_x - window->Pos.x - window->DC.Indent.x; // FIXME-WORKRECT // FIXME-TABLE: Recurse
    window->DC.CurrLineTextBaseOffset = table->RowTextBaseline;

    window->WorkRect.Min.y = window->DC.CursorPos.y;
    window->WorkRect.Min.x = column->MinX + table->CellPaddingX1;
    window->WorkRect.Max.x = column->MaxX - table->CellPaddingX2;

    // To allow ImGuiListClipper to function we propagate our row height
    if (!column->IsActive)
        window->DC.CursorPos.y = ImMax(window->DC.CursorPos.y, table->RowPosY2);

    // FIXME-COLUMNS: Setup baseline, preserve across columns (how can we obtain first line baseline tho..)
    // window->DC.CurrLineTextBaseOffset = ImMax(window->DC.CurrLineTextBaseOffset, g.Style.FramePadding.y);

    window->SkipItems = column->IsClipped ? true : table->BackupSkipItems;
    if (table->Flags & ImGuiTableFlags_NoClipX)
    {
        table->DrawSplitter.SetCurrentChannel(window->DrawList, 1);
    }
    else
    {
        table->DrawSplitter.SetCurrentChannel(window->DrawList, column->DrawChannelCurrent);
        //window->ClipRect = column->ClipRect;
        //IM_ASSERT(column->ClipRect.Max.x > column->ClipRect.Min.x && column->ClipRect.Max.y > column->ClipRect.Min.y);
        //window->DrawList->_ClipRectStack.back() = ImVec4(column->ClipRect.Min.x, column->ClipRect.Min.y, column->ClipRect.Max.x, column->ClipRect.Max.y);
        //window->DrawList->UpdateClipRect();
        window->DrawList->PopClipRect();
        window->DrawList->PushClipRect(column->ClipRect.Min, column->ClipRect.Max, false);
        //IMGUI_DEBUG_LOG("%d (%.0f,%.0f)(%.0f,%.0f)\n", column_no, column->ClipRect.Min.x, column->ClipRect.Min.y, column->ClipRect.Max.x, column->ClipRect.Max.y);
        window->ClipRect = window->DrawList->_ClipRectStack.back();
    }
}

// [Internal] Called by TableNextRow()TableNextCell()!
void    ImGui::TableEndCell(ImGuiTable* table)
{
    ImGuiTableColumn* column = &table->Columns[table->CurrentColumn];
    ImGuiWindow* window = table->InnerWindow;

    // Report maximum position so we can infer content size per column.
    float* p_max_pos_x;
    if (table->RowFlags & ImGuiTableRowFlags_Headers)
        p_max_pos_x = &column->ContentMaxPosHeadersUsed;  // Useful in case user submit contents in header row that is not a TableHeader() call
    else
        p_max_pos_x = table->IsFreezeRowsPassed ? &column->ContentMaxPosRowsUnfrozen : &column->ContentMaxPosRowsFrozen;
    *p_max_pos_x = ImMax(*p_max_pos_x, window->DC.CursorMaxPos.x);
    table->RowPosY2 = ImMax(table->RowPosY2, window->DC.CursorMaxPos.y);

    // Propagate text baseline for the entire row
    // FIXME-TABLE: Here we propagate text baseline from the last line of the cell.. instead of the first one.
    table->RowTextBaseline = ImMax(table->RowTextBaseline, window->DC.PrevLineTextBaseOffset);
}

// Append into the next cell
// FIXME-TABLE: Wrapping to next row should be optional?
bool    ImGui::TableNextCell()
{
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;

    if (table->CurrentColumn != -1 && table->CurrentColumn + 1 < table->ColumnsCount)
    {
        TableEndCell(table);
        TableBeginCell(table, table->CurrentColumn + 1);
    }
    else
    {
        TableNextRow();
    }

    int column_n = table->CurrentColumn;
    return (table->VisibleMaskByIndex & ((ImU64)1 << column_n)) != 0;
}

const char*   ImGui::TableGetColumnName(int column_n)
{
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;
    if (!table)
        return NULL;
    if (column_n < 0)
        column_n = table->CurrentColumn;
    return TableGetColumnName(table, column_n);
}

bool    ImGui::TableGetColumnIsVisible(int column_n)
{
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;
    if (!table)
        return false;
    if (column_n < 0)
        column_n = table->CurrentColumn;
    return (table->VisibleMaskByIndex & ((ImU64)1 << column_n)) != 0;
}

int     ImGui::TableGetColumnIndex()
{
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;
    if (!table)
        return 0;
    return table->CurrentColumn;
}

bool    ImGui::TableSetColumnIndex(int column_idx)
{
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;
    if (!table)
        return false;

    if (table->CurrentColumn != column_idx)
    {
        if (table->CurrentColumn != -1)
            TableEndCell(table);
        IM_ASSERT(column_idx >= 0 && table->ColumnsCount);
        TableBeginCell(table, column_idx);
    }

    return (table->VisibleMaskByIndex & ((ImU64)1 << column_idx)) != 0;
}

ImRect  ImGui::TableGetCellRect()
{
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;
    ImGuiTableColumn* column = &table->Columns[table->CurrentColumn];
    return ImRect(column->MinX, table->RowPosY1, column->MaxX, table->RowPosY2);
}

const char* ImGui::TableGetColumnName(ImGuiTable* table, int column_no)
{
    ImGuiTableColumn* column = &table->Columns[column_no];
    if (column->NameOffset == -1)
        return NULL;
    return &table->ColumnsNames.Buf[column->NameOffset];
}

void    ImGui::TableSetColumnAutofit(ImGuiTable* table, int column_no)
{
    // Disable clipping then auto-fit, will take 2 frames 
    // (we don't take a shortcut for unclipped columns to reduce inconsistencies when e.g. resizing multiple columns)
    ImGuiTableColumn* column = &table->Columns[column_no];
    column->CannotSkipItemsQueue = (1 << 0);
    column->AutoFitQueue = (1 << 1);
}

void    ImGui::PushTableBackground()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiTable* table = g.CurrentTable;
    table->DrawSplitter.SetCurrentChannel(window->DrawList, 0);
    PushClipRect(table->HostClipRect.Min, table->HostClipRect.Max, false);
}

void    ImGui::PopTableBackground()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiTable* table = g.CurrentTable;
    ImGuiTableColumn* column = &table->Columns[table->CurrentColumn];
    table->DrawSplitter.SetCurrentChannel(window->DrawList, column->DrawChannelCurrent);
    PopClipRect();
}

// Output context menu into current window (generally a popup)
// FIXME-TABLE: Ideally this should be writable by the user. Full programmatic access to that data?
void    ImGui::TableDrawContextMenu(ImGuiTable* table, int selected_column_n)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    bool want_separator = false;
    selected_column_n  = ImClamp(selected_column_n, -1, table->ColumnsCount - 1);

    // Sizing
    if (table->Flags & ImGuiTableFlags_Resizable)
    {
        if (ImGuiTableColumn* selected_column = (selected_column_n != -1) ? &table->Columns[selected_column_n] : NULL)
        {
            const bool can_resize = !(selected_column->Flags & (ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthStretch)) && selected_column->IsActive;
            if (MenuItem("Size column to fit", NULL, false, can_resize))
                TableSetColumnAutofit(table, selected_column_n);
        }

        if (MenuItem("Size all columns to fit", NULL))
        {
            for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
            {
                ImGuiTableColumn* column = &table->Columns[column_n];
                if (column->IsActive)
                    TableSetColumnAutofit(table, column_n);
            }
        }
        want_separator = true;
    }

    // Ordering
    if (table->Flags & ImGuiTableFlags_Reorderable)
    {
        if (MenuItem("Reset order", NULL, false, !table->IsDefaultDisplayOrder))
            table->IsResetDisplayOrderRequest = true;
        want_separator = true;
    }

    // Hiding / Visibility
    if (table->Flags & ImGuiTableFlags_Hideable)
    {
        if (want_separator)
            Separator();
        want_separator = false;

        PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
        for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
        {
            ImGuiTableColumn* column = &table->Columns[column_n];
            const char* name = TableGetColumnName(table, column_n);
            if (name == NULL)
                name = "<Unknown>";

            // Make sure we can't hide the last active column
            bool menu_item_active = (column->Flags & ImGuiTableColumnFlags_NoHide) ? false : true;
            if (column->IsActive && table->ColumnsActiveCount <= 1)
                menu_item_active = false;
            if (MenuItem(name, NULL, column->IsActive, menu_item_active))
                column->NextIsActive = !column->IsActive;
        }
        PopItemFlag();
    }
}

// This is a helper to output TableHeader() calls based on the column names declared in TableSetupColumn().
// The intent is that advanced users would not need to use this helper and may create their own.
void    ImGui::TableAutoHeaders()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    ImGuiTable* table = g.CurrentTable;
    IM_ASSERT(table != NULL && "Need to call TableAutoHeaders() after BeginTable()!");
    IM_ASSERT(table->CurrentRow == -1);

    int open_context_popup = INT_MAX;

    // This for loop is constructed to not make use of internal functions,
    // as this is intended to be a base template to copy and build from.
    TableNextRow(ImGuiTableRowFlags_Headers, GetTextLineHeight());
    const int columns_count = table->ColumnsCount;
    for (int column_n = 0; column_n < columns_count; column_n++)
    {
        if (!TableSetColumnIndex(column_n))
            continue;

        const char* name = TableGetColumnName(column_n);

        // FIXME-TABLE: Test custom user elements
#if 0
        if (column_n < 2)
        {
            static bool b[10] = {};
            PushID(column_n);
            PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            Checkbox("##", &b[column_n]);
            PopStyleVar();
            PopID();
            SameLine(0.0f, g.Style.ItemInnerSpacing.x);
        }
#endif

        // [DEBUG]
        //if (g.IO.KeyCtrl) { static char buf[32]; name = buf; ImGuiTableColumn* c = &table->Columns[column_n]; if (c->Flags & ImGuiTableColumnFlags_WidthStretch) ImFormatString(buf, 32, "%.3f>%.1f", c->ResizeWeight, c->WidthGiven); else ImFormatString(buf, 32, "%.1f", c->WidthGiven); }

        PushID(table->InstanceNo * table->ColumnsCount + column_n); // Allow unnamed labels (generally accidental, but let's behave nicely with them)
        TableHeader(name);
        PopID();

        // We don't use BeginPopupContextItem() because we want the popup to stay up even after the column is hidden
        if (IsMouseReleased(1) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
            open_context_popup = column_n;
    }

    // FIXME-TABLE: This is not user-land code any more...
    window->SkipItems = table->BackupSkipItems;

    // Allow opening popup from the right-most section after the last column
    // FIXME-TABLE: This is not user-land code any more... perhaps instead we should expose hovered column.
    // and allow some sort of row-centric IsItemHovered() for full flexibility?
    const float unused_x1 = (table->RightMostActiveColumn != -1) ? table->Columns[table->RightMostActiveColumn].MaxX : table->WorkRect.Min.x;
    if (unused_x1 < table->WorkRect.Max.x)
    {
        // FIXME: We inherit ClipRect/SkipItem from last submitted column (active or not), let's override
        window->ClipRect = table->InnerClipRect;

        ImVec2 backup_cursor_max_pos = window->DC.CursorMaxPos;
        window->DC.CursorPos = ImVec2(unused_x1, table->RowPosY1);
        ImVec2 size = ImVec2(table->WorkRect.Max.x - window->DC.CursorPos.x, table->RowPosY2 - table->RowPosY1);
        if (size.x > 0.0f && size.y > 0.0f)
        {
            InvisibleButton("##RemainingSpace", size);
            window->DC.CursorPos.y -= g.Style.ItemSpacing.y;
            window->DC.CursorMaxPos = backup_cursor_max_pos;    // Don't feed back into the width of the Header row

            // We don't use BeginPopupContextItem() because we want the popup to stay up even after the column is hidden
            if (IsMouseReleased(1) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                open_context_popup = -1;
        }

        window->ClipRect = window->DrawList->_ClipRectStack.back();
    }

    // Context Menu
    if (open_context_popup != INT_MAX)
    {
        table->IsContextPopupOpen = true;
        table->ContextPopupColumn = (ImS8)open_context_popup;
        table->InstanceInteracted = table->InstanceNo;
        OpenPopup("##TableContextMenu");
    }
}

// Emit a column header (text + optional sort order)
// We cpu-clip text here so that all columns headers can be merged into a same draw call.
// FIXME-TABLE: Should hold a selection state.
void    ImGui::TableHeader(const char* label)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    ImGuiTable* table = g.CurrentTable;
    IM_ASSERT(table != NULL && "Need to call TableAutoHeaders() after BeginTable()!");
    IM_ASSERT(table->CurrentColumn != -1);
    const int column_n = table->CurrentColumn;
    ImGuiTableColumn* column = &table->Columns[column_n];

    float row_height = GetTextLineHeight();
    ImRect cell_r = TableGetCellRect();
    ImRect work_r = cell_r;
    work_r.Min.x = window->DC.CursorPos.x;
    work_r.Max.y = work_r.Min.y + row_height;

    // Label
    if (label == NULL)
        label = "";
    const char* label_end = FindRenderedTextEnd(label);
    ImVec2 label_size = CalcTextSize(label, label_end, true);
    ImVec2 label_pos = window->DC.CursorPos;
    float ellipsis_max = work_r.Max.x;

    // Selectable
    PushID(label);

    // FIXME-TABLE: Fix when padding are disabled.
    //window->DC.CursorPos.x = column->MinX + table->CellPadding.x;

    // Keep header highlighted when context menu is open. (FIXME-TABLE: however we cannot assume the ID of said popup if it has been created by the user...)
    const bool selected = (table->IsContextPopupOpen && table->ContextPopupColumn == column_n && table->InstanceInteracted == table->InstanceNo);
    const bool pressed = Selectable("", selected, ImGuiSelectableFlags_DrawHoveredWhenHeld, ImVec2(0.0f, row_height));
    const bool held = IsItemActive();
    if (held)
        table->HeadHeaderColumn = (ImS8)column_n;
    window->DC.CursorPos.y -= g.Style.ItemSpacing.y * 0.5f;

    // Drag and drop: re-order columns. Frozen columns are not reorderable.
    // FIXME-TABLE: Scroll request while reordering a column and it lands out of the scrolling zone.
    if (held && (table->Flags & ImGuiTableFlags_Reorderable) && IsMouseDragging(0) && !g.DragDropActive)
    {
        // While moving a column it will jump on the other side of the mouse, so we also test for MouseDelta.x
        table->ReorderColumn = (ImS8)column_n;
        table->InstanceInteracted = table->InstanceNo;
        if (g.IO.MouseDelta.x < 0.0f && g.IO.MousePos.x < cell_r.Min.x)
            if (column->PrevActiveColumn != -1 && (column->IndexWithinActiveSet < table->FreezeColumnsRequest) == (table->Columns[column->PrevActiveColumn].IndexWithinActiveSet < table->FreezeColumnsRequest))
                table->ReorderColumnDir = -1;
        if (g.IO.MouseDelta.x > 0.0f && g.IO.MousePos.x > cell_r.Max.x)
            if (column->NextActiveColumn != -1 && (column->IndexWithinActiveSet < table->FreezeColumnsRequest) == (table->Columns[column->NextActiveColumn].IndexWithinActiveSet < table->FreezeColumnsRequest))
                table->ReorderColumnDir = +1;
    }

    // Sort order arrow
    float w_arrow = 0.0f;
    float w_sort_text = 0.0f;
    if ((table->Flags & ImGuiTableFlags_Sortable) && !(column->Flags & ImGuiTableColumnFlags_NoSort))
    {
        const float ARROW_SCALE = 0.75f;
        w_arrow = ImFloor(g.FontSize * ARROW_SCALE + g.Style.FramePadding.x);// table->CellPadding.x);
        if (column->SortOrder != -1)
        {
            w_sort_text = 0.0f;

            char sort_order_suf[8];
            if (column->SortOrder > 0)
            {
                ImFormatString(sort_order_suf, IM_ARRAYSIZE(sort_order_suf), "%d", column->SortOrder + 1);
                w_sort_text = g.Style.ItemInnerSpacing.x + CalcTextSize(sort_order_suf).x;
            }

            float x = ImMax(cell_r.Min.x, work_r.Max.x - w_arrow - w_sort_text);
            ellipsis_max -= w_arrow + w_sort_text;

            float y = label_pos.y;
            ImU32 col = GetColorU32(ImGuiCol_Text);
            if (column->SortOrder > 0)
            {
                PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_Text, 0.70f));
                RenderText(ImVec2(x + g.Style.ItemInnerSpacing.x, y), sort_order_suf);
                PopStyleColor();
                x += w_sort_text;
            }
            RenderArrow(window->DrawList, ImVec2(x, y), col, column->SortDirection == ImGuiSortDirection_Ascending ? ImGuiDir_Down : ImGuiDir_Up, ARROW_SCALE);
        }

        // Handle clicking on column header to adjust Sort Order
        if (pressed && table->ReorderColumn != column_n)
            TableSortSpecsClickColumn(table, column, g.IO.KeyShift);
    }

    // Render clipped label
    // Clipping here ensure that in the majority of situations, all our header cells will be merged into a single draw call.
    //window->DrawList->AddCircleFilled(ImVec2(ellipsis_max, label_pos.y), 40, IM_COL32_WHITE);
    RenderTextEllipsis(window->DrawList, label_pos, ImVec2(ellipsis_max, label_pos.y + row_height + g.Style.FramePadding.y), ellipsis_max, ellipsis_max, label, label_end, &label_size);

    // We feed our unclipped width to the column without writing on CursorMaxPos, so that column is still considering for merging.
    // FIXME-TABLE: Clarify policies of how label width and potential decorations (arrows) fit into auto-resize of the column
    float max_pos_x = label_pos.x + label_size.x + w_sort_text + w_arrow;
    column->ContentMaxPosHeadersUsed = ImMax(column->ContentMaxPosHeadersUsed, work_r.Max.x);// ImMin(max_pos_x, work_r.Max.x));
    column->ContentMaxPosHeadersDesired = ImMax(column->ContentMaxPosHeadersDesired, max_pos_x);

    PopID();
}

void ImGui::TableSortSpecsClickColumn(ImGuiTable* table, ImGuiTableColumn* clicked_column, bool add_to_existing_sort_orders)
{
    if (!(table->Flags & ImGuiTableFlags_MultiSortable))
        add_to_existing_sort_orders = false;

    ImS8 sort_order_max = 0;
    if (add_to_existing_sort_orders)
        for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
            sort_order_max = ImMax(sort_order_max, table->Columns[column_n].SortOrder);

    for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
    {
        ImGuiTableColumn* column = &table->Columns[column_n];
        if (column == clicked_column)
        {
            // Set new sort direction and sort order
            // - If the PreferSortDescending flag is set, we will default to a Descending direction on the first click.
            // - Note that the PreferSortAscending flag is never checked, it is essentially the default and therefore a no-op.
            // - Note that the NoSortAscending/NoSortDescending flags are processed in TableSortSpecsSanitize(), and they may change/revert
            //   the value of SortDirection. We could technically also do it here but it would be unnecessary and duplicate code.
            if (column->SortOrder == -1)
                column->SortDirection = (column->Flags & ImGuiTableColumnFlags_PreferSortDescending) ? (ImS8)ImGuiSortDirection_Descending : (ImU8)(ImGuiSortDirection_Ascending);
            else
                column->SortDirection = (ImU8)((column->SortDirection == ImGuiSortDirection_Ascending) ? ImGuiSortDirection_Descending : ImGuiSortDirection_Ascending);
            if (column->SortOrder == -1 || !add_to_existing_sort_orders)
                column->SortOrder = add_to_existing_sort_orders ? sort_order_max + 1 : 0;
        }
        else
        {
            if (!add_to_existing_sort_orders)
                column->SortOrder = -1;
        }
        TableFixColumnSortDirection(column);
    }
    table->IsSettingsDirty = true;
    table->IsSortSpecsDirty = true;
}

// Return NULL if no sort specs.
// Return ->WantSort == true when the specs have changed since the last query.
const ImGuiTableSortSpecs* ImGui::TableGetSortSpecs()
{
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;
    IM_ASSERT(table != NULL);

    if (!(table->Flags & ImGuiTableFlags_Sortable))
        return NULL;

    // Flatten sort specs into user facing data
    const bool was_dirty = table->IsSortSpecsDirty;
    if (was_dirty)
    {
        TableSortSpecsSanitize(table);

        // Write output
        table->SortSpecsData.resize(table->SortSpecsCount);
        table->SortSpecs.ColumnsMask = 0x00;
        for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
        {
            ImGuiTableColumn* column = &table->Columns[column_n];
            if (column->SortOrder == -1)
                continue;
            ImGuiTableSortSpecsColumn* sort_spec = &table->SortSpecsData[column->SortOrder];
            sort_spec->ColumnUserID = column->UserID;
            sort_spec->ColumnIndex = (ImU8)column_n;
            sort_spec->SortOrder = (ImU8)column->SortOrder;
            sort_spec->SortSign = (column->SortDirection == ImGuiSortDirection_Ascending) ? +1 : -1;
            sort_spec->SortDirection = column->SortDirection;
            table->SortSpecs.ColumnsMask |= (ImU64)1 << column_n;
        }
    }

    // User facing data
    table->SortSpecs.Specs = table->SortSpecsData.Data;
    table->SortSpecs.SpecsCount = table->SortSpecsData.Size;
    table->SortSpecs.SpecsChanged = was_dirty;
    table->IsSortSpecsDirty = false;
    return table->SortSpecs.SpecsCount ? &table->SortSpecs : NULL;
}

bool ImGui::TableGetColumnIsSorted(int column_n)
{
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;
    if (!table)
        return false;
    if (column_n < 0)
        column_n = table->CurrentColumn;
    ImGuiTableColumn* column = &table->Columns[column_n];
    return (column->SortOrder != -1);
}

void ImGui::TableSortSpecsSanitize(ImGuiTable* table)
{
    // Clear SortOrder from hidden column and verify that there's no gap or duplicate.
    int sort_order_count = 0;
    ImU64 sort_order_mask = 0x00;
    for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
    {
        ImGuiTableColumn* column = &table->Columns[column_n];
        if (column->SortOrder != -1 && !column->IsActive)
            column->SortOrder = -1;
        if (column->SortOrder == -1)
            continue;
        sort_order_count++;
        sort_order_mask |= ((ImU64)1 << column->SortOrder);
        IM_ASSERT(sort_order_count < (int)sizeof(sort_order_mask) * 8);
    }

    const bool need_fix_linearize = ((ImU64)1 << sort_order_count) != (sort_order_mask + 1);
    const bool need_fix_single_sort_order = (sort_order_count > 1) && !(table->Flags & ImGuiTableFlags_MultiSortable);
    if (need_fix_linearize || need_fix_single_sort_order)
    {
        ImU64 fixed_mask = 0x00;
        for (int sort_n = 0; sort_n < sort_order_count; sort_n++)
        {
            // Fix: Rewrite sort order fields if needed so they have no gap or duplicate.
            // (e.g. SortOrder 0 disappeared, SortOrder 1..2 exists --> rewrite then as SortOrder 0..1)
            int column_with_smallest_sort_order = -1;
            for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
                if ((fixed_mask & ((ImU64)1 << (ImU64)column_n)) == 0 && table->Columns[column_n].SortOrder != -1)
                    if (column_with_smallest_sort_order == -1 || table->Columns[column_n].SortOrder < table->Columns[column_with_smallest_sort_order].SortOrder)
                        column_with_smallest_sort_order = column_n;
            IM_ASSERT(column_with_smallest_sort_order != -1);
            fixed_mask |= ((ImU64)1 << column_with_smallest_sort_order);
            table->Columns[column_with_smallest_sort_order].SortOrder = (ImS8)sort_n;

            // Fix: Make sure only one column has a SortOrder if ImGuiTableFlags_MultiSortable is not set.
            if (need_fix_single_sort_order)
            {
                for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
                    if (column_n != column_with_smallest_sort_order)
                        table->Columns[column_n].SortOrder = -1;
                break;
            }
        }
    }

    // Fallback default sort order (if no column has the ImGuiTableColumnFlags_DefaultSort flag)
    if (sort_order_count == 0 && table->IsFirstFrame)
        for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
        {
            ImGuiTableColumn* column = &table->Columns[column_n];
            if (!(column->Flags & ImGuiTableColumnFlags_NoSort) && column->IsActive)
            {
                sort_order_count = 1;
                column->SortOrder = 0;
                break;
            }
        }

    table->SortSpecsCount = (ImS8)sort_order_count;
}

//-------------------------------------------------------------------------
// TABLE - .ini settings
//-------------------------------------------------------------------------
// [Init] 1: TableSettingsHandler_ReadXXXX()   Load and parse .ini file into TableSettings.
// [Main] 2: TableLoadSettings()               When table is created, bind Table to TableSettings, serialize TableSettings data into Table.
// [Main] 3: TableSaveSettings()               When table properties are modified, serialize Table data into bound or new TableSettings, mark .ini as dirty.
// [Main] 4: TableSettingsHandler_WriteAll()   When .ini file is dirty (which can come from other source), save TableSettings into .ini file.
//-------------------------------------------------------------------------

static ImGuiTableSettings* CreateTableSettings(ImGuiID id, int columns_count)
{
    ImGuiContext& g = *GImGui;
    ImGuiTableSettings* settings = g.SettingsTables.alloc_chunk(sizeof(ImGuiTableSettings) + (size_t)columns_count * sizeof(ImGuiTableColumnSettings));
    IM_PLACEMENT_NEW(settings) ImGuiTableSettings();
    ImGuiTableColumnSettings* settings_column = settings->GetColumnSettings();
    for (int n = 0; n < columns_count; n++, settings_column++)
        IM_PLACEMENT_NEW(settings_column) ImGuiTableColumnSettings();
    settings->ID = id;
    settings->ColumnsCount = settings->ColumnsCountMax = (ImS8)columns_count;
    return settings;
}

static ImGuiTableSettings* FindTableSettingsByID(ImGuiID id)
{
    // FIXME-OPT: Might want to store a lookup map for this?
    ImGuiContext& g = *GImGui;
    for (ImGuiTableSettings* settings = g.SettingsTables.begin(); settings != NULL; settings = g.SettingsTables.next_chunk(settings))
        if (settings->ID == id)
            return settings;
    return NULL;
}

ImGuiTableSettings* ImGui::TableFindSettings(ImGuiTable* table)
{
    if (table->SettingsOffset == -1)
        return NULL;

    ImGuiContext& g = *GImGui;
    ImGuiTableSettings* settings = g.SettingsTables.ptr_from_offset(table->SettingsOffset);
    IM_ASSERT(settings->ID == table->ID);
    if (settings->ColumnsCountMax < table->ColumnsCount)
    {
        settings->ID = 0; // Ditch storage if we won't fit because of a count change
        return NULL;
    }
    return settings;
}

void ImGui::TableSaveSettings(ImGuiTable* table)
{
    table->IsSettingsDirty = false;
    if (table->Flags & ImGuiTableFlags_NoSavedSettings)
        return;

    // Bind or create settings data
    ImGuiContext& g = *GImGui;
    ImGuiTableSettings* settings = TableFindSettings(table);
    if (settings == NULL)
    {
        settings = CreateTableSettings(table->ID, table->ColumnsCount);
        table->SettingsOffset = g.SettingsTables.offset_from_ptr(settings);
    }
    settings->ColumnsCount = (ImS8)table->ColumnsCount;

    // Serialize ImGuiTableSettings/ImGuiTableColumnSettings --> ImGuiTable/ImGuiTableColumn
    IM_ASSERT(settings->ID == table->ID);
    IM_ASSERT(settings->ColumnsCount == table->ColumnsCount && settings->ColumnsCountMax >= settings->ColumnsCount);
    ImGuiTableColumn* column = table->Columns.Data;
    ImGuiTableColumnSettings* column_settings = settings->GetColumnSettings();

    // FIXME-TABLE: Logic to avoid saving default widths?
    settings->SaveFlags = ImGuiTableFlags_Resizable;
    for (int n = 0; n < table->ColumnsCount; n++, column++, column_settings++)
    {
        //column_settings->WidthOrWeight = column->WidthRequested; // FIXME-WIP
        column_settings->Index = (ImS8)n;
        column_settings->DisplayOrder = column->IndexDisplayOrder;
        column_settings->SortOrder = column->SortOrder;
        column_settings->SortDirection = column->SortDirection;
        column_settings->Visible = column->IsActive;

        // We skip saving some data in the .ini file when they are unnecessary to restore our state
        // FIXME-TABLE: We don't have logic to easily compare SortOrder to DefaultSortOrder yet.
        if (column->IndexDisplayOrder != n)
            settings->SaveFlags |= ImGuiTableFlags_Reorderable;;
        if (column_settings->SortOrder != -1)
            settings->SaveFlags |= ImGuiTableFlags_Sortable;
        if (column_settings->Visible != ((column->Flags & ImGuiTableColumnFlags_DefaultHide) == 0))
            settings->SaveFlags |= ImGuiTableFlags_Hideable;
    }
    settings->SaveFlags &= table->Flags;

    MarkIniSettingsDirty();
}

void ImGui::TableLoadSettings(ImGuiTable* table)
{
    ImGuiContext& g = *GImGui;
    table->IsSettingsRequestLoad = false;
    if (table->Flags & ImGuiTableFlags_NoSavedSettings)
        return;

    // Bind settings
    ImGuiTableSettings* settings;
    if (table->SettingsOffset == -1)
    {
        settings = FindTableSettingsByID(table->ID);
        if (settings == NULL)
            return;
        table->SettingsOffset = g.SettingsTables.offset_from_ptr(settings);
    }
    else
    {
        settings = g.SettingsTables.ptr_from_offset(table->SettingsOffset);
    }
    table->IsSettingsLoaded = true;
    settings->SaveFlags = table->Flags;

    // Serialize ImGuiTable/ImGuiTableColumn --> ImGuiTableSettings/ImGuiTableColumnSettings
    ImGuiTableColumnSettings* column_settings = settings->GetColumnSettings();
    for (int data_n = 0; data_n < settings->ColumnsCount; data_n++, column_settings++)
    {
        int column_n = column_settings->Index;
        if (column_n < 0 || column_n >= table->ColumnsCount)
            continue;
        ImGuiTableColumn* column = &table->Columns[column_n];
        //column->WidthRequested = column_settings->WidthOrWeight; // FIXME-WIP
        if (column_settings->DisplayOrder != -1)
            column->IndexDisplayOrder = column_settings->DisplayOrder;
        if (column_settings->SortOrder != -1)
        {
            column->SortOrder = column_settings->SortOrder;
            column->SortDirection = column_settings->SortDirection;
        }
        column->IsActive = column->NextIsActive = column_settings->Visible;
    }

    // FIXME-TABLE: Need to validate .ini data
    for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
        table->DisplayOrder[table->Columns[column_n].IndexDisplayOrder] = (ImU8)column_n;
}

void*   ImGui::TableSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    ImGuiID id = 0;
    int columns_count = 0;
    if (sscanf(name, "0x%08X,%d", &id, &columns_count) < 2)
        return NULL;
    return CreateTableSettings(id, columns_count);
}

void    ImGui::TableSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    // "Column 0  UserID=0x42AD2D21 Width=100 Visible=1 Order=0 Sort=0v"
    ImGuiTableSettings* settings = (ImGuiTableSettings*)entry;
    int column_n = 0, r = 0, n = 0;
    if (sscanf(line, "Column %d%n", &column_n, &r) == 1)        { line = ImStrSkipBlank(line + r); } else { return; }
    if (column_n < 0 || column_n >= settings->ColumnsCount)
        return;

    char c = 0;
    ImGuiTableColumnSettings* column = settings->GetColumnSettings() + column_n;
    column->Index = (ImS8)column_n;
    if (sscanf(line, "UserID=0x%08X%n", (ImU32*)&n, &r) == 1)   { line = ImStrSkipBlank(line + r); column->UserID = (ImGuiID)n; }
    if (sscanf(line, "Width=%d%n", &n, &r) == 1)                { line = ImStrSkipBlank(line + r); /* .. */ settings->SaveFlags |= ImGuiTableFlags_Resizable; }
    if (sscanf(line, "Visible=%d%n", &n, &r) == 1)              { line = ImStrSkipBlank(line + r); column->Visible = (ImU8)n; settings->SaveFlags |= ImGuiTableFlags_Hideable; }
    if (sscanf(line, "Order=%d%n", &n, &r) == 1)                { line = ImStrSkipBlank(line + r); column->DisplayOrder = (ImS8)n; settings->SaveFlags |= ImGuiTableFlags_Reorderable; }
    if (sscanf(line, "Sort=%d%c%n", &n, &c, &r) == 2)           { line = ImStrSkipBlank(line + r); column->SortOrder = (ImS8)n; column->SortDirection = (c == '^') ? ImGuiSortDirection_Descending : ImGuiSortDirection_Ascending; settings->SaveFlags |= ImGuiTableFlags_Sortable; }
}

void    ImGui::TableSettingsHandler_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    ImGuiContext& g = *ctx;
    for (ImGuiTableSettings* settings = g.SettingsTables.begin(); settings != NULL; settings = g.SettingsTables.next_chunk(settings))
    {
        if (settings->ID == 0) // Skip ditched settings
            continue;

        // TableSaveSettings() may clear some of those flags when we establish that the data can be stripped (e.g. Order was unchanged)
        const bool save_size    = (settings->SaveFlags & ImGuiTableFlags_Resizable) != 0;
        const bool save_visible = (settings->SaveFlags & ImGuiTableFlags_Hideable) != 0;
        const bool save_order   = (settings->SaveFlags & ImGuiTableFlags_Reorderable) != 0;
        const bool save_sort    = (settings->SaveFlags & ImGuiTableFlags_Sortable) != 0;
        if (!save_size && !save_visible && !save_order && !save_sort)
            continue;

        buf->reserve(buf->size() + 30 + settings->ColumnsCount * 50); // ballpark reserve
        buf->appendf("[%s][0x%08X,%d]\n", handler->TypeName, settings->ID, settings->ColumnsCount);
        ImGuiTableColumnSettings* column = settings->GetColumnSettings();
        for (int column_n = 0; column_n < settings->ColumnsCount; column_n++, column++)
        {
            // "Column 0  UserID=0x42AD2D21 Width=100 Visible=1 Order=0 Sort=0v"
            if (column->UserID != 0)
                buf->appendf("Column %-2d UserID=%08X", column_n, column->UserID);
            else
                buf->appendf("Column %-2d", column_n);
            if (save_size)                              buf->appendf(" Width=%d", 0);// (int)settings_column->WidthOrWeight);  // FIXME-TABLE
            if (save_visible)                           buf->appendf(" Visible=%d", column->Visible);
            if (save_order)                             buf->appendf(" Order=%d", column->DisplayOrder);
            if (save_sort && column->SortOrder != -1)   buf->appendf(" Sort=%d%c", column->SortOrder, (column->SortDirection == ImGuiSortDirection_Ascending) ? 'v' : '^');
            buf->append("\n");
        }
        buf->append("\n");
    }
}

//-------------------------------------------------------------------------
// TABLE - Debugging
//-------------------------------------------------------------------------
// - DebugNodeTable() [Internal]
//-------------------------------------------------------------------------

#ifndef IMGUI_DISABLE_METRICS_WINDOW
void ImGui::DebugNodeTable(ImGuiTable* table)
{
    char buf[256];
    char* p = buf;
    const char* buf_end = buf + IM_ARRAYSIZE(buf);
    ImFormatString(p, buf_end - p, "Table 0x%08X (%d columns, in '%s')", table->ID, table->ColumnsCount, table->OuterWindow->Name);
    bool open = TreeNode(table, "%s", buf);
    if (IsItemHovered())
        GetForegroundDrawList()->AddRect(table->OuterRect.Min, table->OuterRect.Max, IM_COL32(255, 255, 0, 255));
    if (open)
    {
        for (int n = 0; n < table->ColumnsCount; n++)
        {
            ImGuiTableColumn* column = &table->Columns[n];
            const char* name = TableGetColumnName(table, n);
            BulletText("Column %d order %d name '%s': +%.1f to +%.1f\n"
                "Active: %d, Clipped: %d, DrawChannels: %d,%d\n"
                "WidthGiven/Requested: %.1f/%.1f, Weight: %.2f\n"
                "UserID: 0x%08X, Flags: 0x%04X: %s%s%s%s..",
                n, column->IndexDisplayOrder, name ? name : "NULL", column->MinX - table->WorkRect.Min.x, column->MaxX - table->WorkRect.Min.x,
                column->IsActive, column->IsClipped, column->DrawChannelRowsBeforeFreeze, column->DrawChannelRowsAfterFreeze,
                column->WidthGiven, column->WidthRequested, column->ResizeWeight,
                column->UserID, column->Flags,
                (column->Flags & ImGuiTableColumnFlags_WidthFixed) ? "WidthFixed " : "",
                (column->Flags & ImGuiTableColumnFlags_WidthStretch) ? "WidthStretch " : "",
                (column->Flags & ImGuiTableColumnFlags_WidthAlwaysAutoResize) ? "WidthAlwaysAutoResize " : "",
                (column->Flags & ImGuiTableColumnFlags_NoResize) ? "NoResize " : "");
        }
        ImGuiTableSettings* settings = TableFindSettings(table);
        if (settings && TreeNode("Settings"))
        {
            BulletText("SaveFlags: 0x%08X", settings->SaveFlags);
            BulletText("ColumnsCount: %d (max %d)", settings->ColumnsCount, settings->ColumnsCountMax);
            for (int n = 0; n < settings->ColumnsCount; n++)
            {
                ImGuiTableColumnSettings* column_settings = &settings->GetColumnSettings()[n];
                BulletText("Column %d Order %d SortOrder %d Visible %d UserID 0x%08X WidthOrWeight %.3f",
                    n, column_settings->DisplayOrder, column_settings->SortOrder, column_settings->Visible, column_settings->UserID, column_settings->WidthOrWeight);
            }
            TreePop();
        }
        TreePop();
    }
}
#endif // #ifndef IMGUI_DISABLE_METRICS_WINDOW

//-------------------------------------------------------------------------




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
