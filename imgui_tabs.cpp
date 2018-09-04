// dear imgui tabs
// experimental/WIP feature, API will be reworked.

#define _CRT_SECURE_NO_WARNINGS
#include "imgui_tabs.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_PLACEMENT_NEW
#include <imgui_internal.h>

#define TAB_SMOOTH_DRAG 0   // This work nicely but has overlapping issues (maybe render dragged tab separately, at end)

// Basic keyed storage, slow/amortized insertion, O(Log N) queries over a dense/hot buffer
template<typename T>
struct ImGuiBigStorage
{
    ImGuiStorage    Map;    // ID->Index
    ImVector<T>     Data;

    ~ImGuiBigStorage() { Clear(); }
    void Clear() 
    { 
        for (int n = 0; n < Data.Size; n++) 
            Data[n].~T(); 
        Map.Clear(); 
        Data.clear(); 
    }
    T*  GetOrCreateByKey(ImGuiID key)
    {
        int* p_idx = Map.GetIntRef(key, -1);
        if (*p_idx == -1)
        {
            *p_idx = Data.Size;
            Data.resize(Data.Size + 1);
            IM_PLACEMENT_NEW(&Data[*p_idx]) T();
        }
        return &Data[*p_idx];
    }
    T*  GetByKey(ImGuiID key)
    {
        int idx = Map.GetInt(key, -1);
        return (idx != -1) ? &Data[idx] : NULL;
    }
};

// sizeof() = 60~
struct ImGuiTabItem
{
    ImGuiID         Id;
    int             GlobalIndex;        // Index in ctx.Tabs[] array, this is the way to uniquely reference to a Tab (instead of using a pointer)
    int             CurrentOrder;       // Index for display. Include hidden tabs. Assigned in TabBarLayout() when expected this frame
    int             CurrentOrderVisible;// Index for display. Include hidden tabs. Assigned in TabBarLayout() when expected this frame
    int             LastFrameVisible;
    int             LastFrameSelected;  // This allows us to rebuild an ordered list of the last activated tabs with little maintenance and zero cost on activation.
    float           OffsetAnim;         // Position relative to beginning of tab
    float           OffsetTarget;       // Position relative to beginning of tab
    float           WidthContents;      // Width of actual contents, stored during TabItem() call
    float           WidthAnim;          // Width currently displayed (animating toward TabWidthDesired)
    float           WidthTarget;        // Width calculated by tab bar
    float           AppearAnim;
    bool            SkipOffsetAnim;
    bool            SkipAppearAnim;
    char            DebugName[16];

    ImGuiTabItem() 
    { 
        Id = 0; 
        GlobalIndex = -1;
        CurrentOrder = -1;
        CurrentOrderVisible = -1;
        LastFrameVisible = LastFrameSelected -1;
        OffsetAnim = OffsetTarget = 0.0f; 
        WidthContents = WidthAnim = WidthTarget = 0.0f;
        AppearAnim = 1.0f;
        SkipOffsetAnim = SkipAppearAnim = false;
        memset(DebugName, 0, sizeof(DebugName));
    }
};

// sizeof() = 96~112 bytes, +3 allocs
struct ImGuiTabBar
{
    ImGuiID             Id;
    ImGuiID             CurrSelectedTabId, NextSelectedTabId;
    int                 CurrTabCount, NextTabCount;
    int                 CurrVisibleCount;
    int                 LastFrameVisible, CurrFrameVisible;
    ImRect              BarRect;
    ImRect              ContentsRect;
    float               OffsetMax;
    float               ScrollingAnim;
    float               ScrollingTarget;
    ImGuiTabBarFlags    Flags;
    ImVector<int>       TabsOrder;
    int                 ReorderRequestTabIdx;
    int                 ReorderRequestDir;
    bool                WantLayout;
    bool                CurrOrderInsideTabsIsValid;
    bool                CurrSelectedTabIdIsAlive;

    ImGuiTabBar()
    {
        Id = 0;
        CurrSelectedTabId = NextSelectedTabId = 0;
        CurrTabCount = NextTabCount = 0;
        CurrVisibleCount = 0;
        LastFrameVisible = CurrFrameVisible = -1;
        OffsetMax = 0.0f;
        ScrollingAnim = ScrollingTarget = 0.0f;
        Flags = ImGuiTabBarFlags_None;
        ReorderRequestTabIdx = -1;
        ReorderRequestDir = 0;
        WantLayout = false;
        CurrOrderInsideTabsIsValid = false;
        CurrSelectedTabIdIsAlive = false;
    }
};

static bool ArrowButton(ImGuiID id, ImGuiDir dir, ImVec2 padding, ImGuiButtonFlags flags = 0)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    const ImGuiStyle& style = g.Style;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImVec2(g.FontSize + padding.x * 2.0f, g.FontSize + padding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

    const ImU32 col = ImGui::GetColorU32((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    ImGui::RenderNavHighlight(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
    ImGui::RenderArrow(bb.Min + padding, dir, 1.0f);

    return pressed;
}

// FIXME: Helper for external extensions to handle multiple-context. 
// Possibly: require user to call setcontext functions on various subsystems, or have hooks in main SetCurrentContext()
// Possibly: have a way for each extension to register themselves globally, so a void* can be stored in ImGuiContext that can be accessed in constant-time from a handle(=index).
struct ImGuiTabsContext
{
    ImGuiBigStorage<ImGuiTabItem>   Tabs;
    ImGuiBigStorage<ImGuiTabBar>    TabBars;
    ImVector<ImGuiTabBar*>          CurrentTabBar;

    ImGuiTabsContext() { }

    ImGuiTabItem* GetTabByIndex(int index)
    {
        return &Tabs.Data[index];
    }
    ImGuiTabItem* GetTabByOrder(ImGuiTabBar* tab_bar, int order)
    {
        return &Tabs.Data[tab_bar->TabsOrder.Data[order]];
    }
};

static ImGuiTabsContext GTabs;

//-----------------------------------------------------------------------------
// TABS
//-----------------------------------------------------------------------------

#define TAB_UNFOCUSED_LERP 1

enum
{
#if TAB_UNFOCUSED_LERP
    ImGuiCol_Tab = ImGuiCol_Header,
#else
    ImGuiCol_Tab = ImGuiCol_WindowBg,
#endif
    ImGuiCol_TabHovered = ImGuiCol_HeaderHovered,
    ImGuiCol_TabActive = ImGuiCol_HeaderActive,
#if TAB_UNFOCUSED_LERP
    ImGuiCol_TabUnfocused = ImGuiCol_COUNT + 100,
#else
    ImGuiCol_TabUnfocused = ImGuiCol_TitleBgCollapsed
#endif
};

static ImU32   TabGetColorU32(int idx)
{
#if TAB_UNFOCUSED_LERP
    if (idx == ImGuiCol_TabUnfocused)
        return ImGui::GetColorU32(ImLerp(ImGui::GetStyleColorVec4(ImGuiCol_Tab), ImGui::GetStyleColorVec4(ImGuiCol_TabHovered), 0.60f));
#endif
    return ImGui::GetColorU32(idx);
}

// FIXME: flags can be removed once we move border to style
static void RenderTabBackground(ImDrawList* draw_list, const ImRect& bb, ImU32 col)
{
    ImGuiContext& g = *GImGui;
    const float rounding = ImMin(g.FontSize * 0.35f, bb.GetWidth() * 0.5f);
    draw_list->PathLineTo(ImVec2(bb.Min.x, bb.Max.y));
    draw_list->PathArcToFast(ImVec2(bb.Min.x + rounding, bb.Min.y + rounding), rounding, 6, 9);
    draw_list->PathArcToFast(ImVec2(bb.Max.x - rounding, bb.Min.y + rounding), rounding, 9, 12);
    draw_list->PathLineTo(ImVec2(bb.Max.x, bb.Max.y));
    draw_list->AddConvexPolyFilled(draw_list->_Path.Data, draw_list->_Path.Size, col);
    if (g.Style.FrameBorderSize > 0.0f)
        draw_list->AddPolyline(draw_list->_Path.Data, draw_list->_Path.Size, ImGui::GetColorU32(ImGuiCol_Border), false, g.Style.FrameBorderSize);
    draw_list->PathClear();
}

void    ImGui::BeginTabBar(const char* str_id, ImGuiTabBarFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiWindow* window = g.CurrentWindow;

    ImGuiTabsContext& ctx = GTabs;
    IM_ASSERT(ctx.CurrentTabBar.empty());                   // Cannot recurse BeginTabBar()

    // Flags
    if ((flags & ImGuiTabBarFlags_SizingPolicyMask_) == 0)
        flags |= ImGuiTabBarFlags_SizingPolicyDefault_;

    PushID(str_id);
    const ImGuiID id = window->GetID("");
    ImGuiTabBar* tab_bar = ctx.TabBars.GetOrCreateByKey(id);
    IM_ASSERT(tab_bar->CurrFrameVisible != g.FrameCount);   // Cannot call multiple times in the same frame 

    ctx.CurrentTabBar.push_back(tab_bar);
    tab_bar->Id = id;
    tab_bar->Flags = flags;
    tab_bar->BarRect = ImRect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(GetContentRegionAvailWidth(), g.FontSize + style.FramePadding.y * 2.0f));
    tab_bar->WantLayout = true; // Layout will be done on the first call to ItemTab()
    tab_bar->LastFrameVisible = tab_bar->CurrFrameVisible;
    tab_bar->CurrFrameVisible = g.FrameCount;

    // Clear current tab information if it wasn't submitted this frame
    if (!tab_bar->CurrSelectedTabIdIsAlive)
        tab_bar->CurrSelectedTabId = 0;

    ItemSize(tab_bar->BarRect);
    ItemAdd(tab_bar->BarRect, 0);
    window->DC.LastItemId = id; // We don't want Nav but for drag and drop we need an item id
    tab_bar->ContentsRect = ImRect(window->DC.CursorPos, window->DC.CursorPos + GetContentRegionAvail());

    // Draw separator
#if 1
    bool unfocused = !g.NavWindow || g.NavWindow->RootWindowForTitleBarHighlight != window->RootWindow;
#else
    bool unfocused = false;
#endif
    const ImU32 col = TabGetColorU32(unfocused ? ImGuiCol_TabUnfocused : ImGuiCol_TabActive);
    window->DrawList->AddLine(ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y - style.ItemSpacing.y), ImVec2(window->Pos.x + window->Size.x, window->DC.CursorPos.y - style.ItemSpacing.y), col, 1.0f);
}

static int TabBarFindClosestVisibleInDirection(ImGuiTabBar* tab_bar, int order, int dir)
{
    ImGuiTabsContext& ctx = GTabs;
    for (order += dir; order >= 0 && order < tab_bar->TabsOrder.Size; order += dir)
        if (ctx.GetTabByOrder(tab_bar, order)->LastFrameVisible == tab_bar->LastFrameVisible)
            return order;
    return -1;
}

static void TabBarScrollClamp(ImGuiTabBar* tab_bar, float& scrolling)
{
    scrolling = ImMin(scrolling, tab_bar->OffsetMax - tab_bar->BarRect.GetWidth());
    scrolling = ImMax(scrolling, 0.0f);
}

static void TabBarScrollToTab(ImGuiTabBar* tab_bar, ImGuiTabItem* tab)
{
    ImGuiContext& g = *GImGui;
    float margin = g.FontSize * 1.0f; // When to scroll to make Tab N+1 visible always make a bit of N visible to suggest more scrolling area (since we don't have a scrollbar)
    float tab_x1 = tab->OffsetTarget + (tab->CurrentOrderVisible > 0 ? -margin : 0.0f);
    float tab_x2 = tab->OffsetTarget + tab->WidthTarget + (tab->CurrentOrderVisible + 1 < tab_bar->CurrVisibleCount ? margin : 0.0f);
    if (tab_bar->ScrollingTarget > tab_x1)
        tab_bar->ScrollingTarget = tab_x1;
    if (tab_bar->ScrollingTarget + tab_bar->BarRect.GetWidth() < tab_x2)
        tab_bar->ScrollingTarget = tab_x2 - tab_bar->BarRect.GetWidth();
}

static void TabBarQueueChangeTabOrder(ImGuiTabBar* tab_bar, const ImGuiTabItem* tab, int dir)
{
    IM_ASSERT(dir == -1 || dir == +1);
    IM_ASSERT(tab_bar->ReorderRequestTabIdx == -1);
    tab_bar->ReorderRequestTabIdx = tab->GlobalIndex;
    tab_bar->ReorderRequestDir = dir;
}

static ImVec2 TabBarCalcTabBaseSize(const ImVec2& label_size, bool* p_open)
{
    ImGuiContext& g = *GImGui;
    ImVec2 size = ImVec2(label_size.x + g.Style.FramePadding.x * 2.0f, label_size.y + g.Style.FramePadding.y * 2.0f);
    if (p_open != NULL)
        size.x += (g.Style.ItemInnerSpacing.x) + (g.FontSize + g.Style.FramePadding.y * 2.0f); // We use Y intentionally to fit the close button circle.
    return size;
}

// This is called only once a frame before by the first call to ItemTab()
// The reason we're not calling it in BeginTabBar() is to leave a chance to the user to call the SetTabItemClosed() functions.
static void TabBarLayout(ImGuiTabBar* tab_bar)
{
    ImGuiContext& g = *GImGui;
    ImGuiTabsContext& ctx = GTabs;
    ImGuiWindow* window = g.CurrentWindow;
    tab_bar->WantLayout = false;

    // Setup next selected tab
    ImGuiTabItem* scroll_track_selected_tab = NULL;
    if (tab_bar->NextSelectedTabId)
    {
        tab_bar->CurrSelectedTabId = tab_bar->NextSelectedTabId;
        tab_bar->NextSelectedTabId = 0;
        scroll_track_selected_tab = ctx.Tabs.GetByKey(tab_bar->CurrSelectedTabId);
    }
    tab_bar->CurrSelectedTabIdIsAlive = false;

    tab_bar->CurrTabCount = tab_bar->NextTabCount;
    tab_bar->NextTabCount = 0;

    // Process order change request (we could probably process it when requested but it's just saner to do it in a single spot).
    if (tab_bar->ReorderRequestTabIdx != -1)
    {
        IM_ASSERT(!(tab_bar->Flags & ImGuiTabBarFlags_NoReorder));
        ImGuiTabItem* tab1 = ctx.GetTabByIndex(tab_bar->ReorderRequestTabIdx);
        int tab2_order = TabBarFindClosestVisibleInDirection(tab_bar, tab1->CurrentOrder, tab_bar->ReorderRequestDir);
        if (tab2_order != -1)
        {
            ImGuiTabItem* tab2 = ctx.GetTabByOrder(tab_bar, tab2_order);
            ImSwap(tab_bar->TabsOrder.Data[tab1->CurrentOrder], tab_bar->TabsOrder.Data[tab2_order]);
            ImSwap(tab1->CurrentOrder, tab2->CurrentOrder);
            tab1->SkipOffsetAnim = true;
#if !TAB_SMOOTH_DRAG
            tab2->SkipOffsetAnim = true;
#endif
            if (tab1->Id == tab_bar->CurrSelectedTabId)
                scroll_track_selected_tab = tab1;
        }
        tab_bar->ReorderRequestTabIdx = -1;
    }

    // During layout we will search for those infos
    int selected_order = -1;
    ImGuiTabItem* most_recent_selected_tab = NULL;

    // Layout all active tabs
    const float tab_width_equal = (tab_bar->CurrTabCount > 0) ? (float)(int)((tab_bar->BarRect.GetWidth() - (tab_bar->CurrTabCount - 1) * g.Style.ItemInnerSpacing.x) / tab_bar->CurrTabCount) : 0.0f;
    float offset_x = 0.0f;
    int tab_order_visible_n = 0;
    for (int tab_order_n = 0; tab_order_n < tab_bar->TabsOrder.Size; tab_order_n++)
    {
        ImGuiTabItem* tab = ctx.GetTabByOrder(tab_bar, tab_order_n);
        tab->CurrentOrder = tab_order_n;
        tab->CurrentOrderVisible = -1;
        tab->OffsetTarget = offset_x;       // We set the offset even for invisible tabs, so can they readily reappear from here this frame, if needed.
        if (tab->LastFrameVisible != tab_bar->LastFrameVisible)
            continue;

        tab->CurrentOrderVisible = tab_order_visible_n++;
        if (tab->Id == tab_bar->CurrSelectedTabId)
            selected_order = tab->CurrentOrder;
        if (most_recent_selected_tab == NULL || most_recent_selected_tab->LastFrameSelected < tab->LastFrameSelected)
            most_recent_selected_tab = tab;
        if (scroll_track_selected_tab == NULL && g.NavJustMovedToId == tab->Id)
            scroll_track_selected_tab = tab;
        if (tab_bar->Flags & ImGuiTabBarFlags_SizingPolicyEqual)
        {
            const float TAB_MAX_WIDTH = g.FontSize * 13.0f;
            tab->WidthTarget = ImClamp(tab_width_equal, 0.0f, TAB_MAX_WIDTH);
        }
        else if (tab_bar->Flags & ImGuiTabBarFlags_SizingPolicyFit)
        {
            const float TAB_MAX_WIDTH = FLT_MAX;// 100.0f; 
            tab->WidthTarget = ImMin(tab->WidthContents, TAB_MAX_WIDTH);
        }
        
        if (tab->SkipOffsetAnim)
        {
            tab->OffsetAnim = tab->OffsetTarget;
            tab->SkipOffsetAnim = false;
        }

        offset_x += tab->WidthTarget + g.Style.ItemInnerSpacing.x;
    }
    tab_bar->OffsetMax = ImMax(offset_x - g.Style.ItemInnerSpacing.x, 0.0f);
    tab_bar->CurrVisibleCount = tab_order_visible_n;
    tab_bar->CurrOrderInsideTabsIsValid = true;

    // Horizontal scrolling buttons
    // FIXME: This is not satisfying but I'll leave the polish for later.
    const float scrolling_speed = g.IO.DeltaTime * g.FontSize * 70.0f;
    if (tab_bar->OffsetMax > tab_bar->BarRect.GetWidth())
    {
        const ImVec2 backup_main_cursor_pos = window->DC.CursorPos;
        float buttons_width = g.FontSize * 2.0f + g.Style.ItemInnerSpacing.x;

        window->DC.CursorPos = ImVec2(tab_bar->BarRect.Max.x - buttons_width, tab_bar->BarRect.Min.y);
#if 0
        // Continuous scroll
        int scrolling_dir = 0;
        ArrowButton(window->GetID("##<"), ImGuiDir_Left, ImVec2(0.0f, g.Style.FramePadding.y));
        if (ImGui::IsItemActive())
            scrolling_dir = -1;
        ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
        ArrowButton(window->GetID("##>"), ImGuiDir_Right, ImVec2(0.0f, g.Style.FramePadding.y));
        if (ImGui::IsItemActive())
            scrolling_dir = +1;
        if (scrolling_dir != 0)
        {
            tab_bar->ScrollingAnim += scrolling_speed * scrolling_dir;
            tab_bar->ScrollingTarget += scrolling_speed * scrolling_dir;
        }
#else
        // Navigate tab by tab
        const float backup_repeat_delay = g.IO.KeyRepeatDelay;
        const float backup_repeat_rate = g.IO.KeyRepeatRate;
        int select_dir = 0;
        g.IO.KeyRepeatDelay = 0.250f;
        g.IO.KeyRepeatRate = 0.200f;
        if (ArrowButton(window->GetID("##<"), ImGuiDir_Left, ImVec2(0.0f, g.Style.FramePadding.y), ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_Repeat))
            select_dir = -1;
        ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
        if (ArrowButton(window->GetID("##>"), ImGuiDir_Right, ImVec2(0.0f, g.Style.FramePadding.y), ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_Repeat))
            select_dir = +1;
        g.IO.KeyRepeatRate = backup_repeat_rate;
        g.IO.KeyRepeatDelay = backup_repeat_delay;
        if (select_dir != 0)
        {
            int tab2_order = TabBarFindClosestVisibleInDirection(tab_bar, selected_order, select_dir);
            if (tab2_order != -1)
            {
                scroll_track_selected_tab = ctx.GetTabByOrder(tab_bar, tab2_order);
                tab_bar->CurrSelectedTabId = scroll_track_selected_tab->Id;
            }
        }
#endif
        window->DC.CursorPos = backup_main_cursor_pos;
        tab_bar->BarRect.Max.x -= buttons_width + g.Style.ItemInnerSpacing.x;
    }
    TabBarScrollClamp(tab_bar, tab_bar->ScrollingAnim);
    TabBarScrollClamp(tab_bar, tab_bar->ScrollingTarget);
    tab_bar->ScrollingAnim = ImLinearSweep(tab_bar->ScrollingAnim, tab_bar->ScrollingTarget, scrolling_speed);

    // If we have lost the selected tab, select the next most recently active one.
    if (tab_bar->CurrSelectedTabId == 0 && tab_bar->NextSelectedTabId == 0 && most_recent_selected_tab != NULL)
    {
        tab_bar->CurrSelectedTabId = most_recent_selected_tab->Id;
        scroll_track_selected_tab = most_recent_selected_tab;
    }

    if (scroll_track_selected_tab)
        TabBarScrollToTab(tab_bar, scroll_track_selected_tab);
}

// We have to essentially avoid logic here since docked tab will be submitted after the fact.
void    ImGui::EndTabBar()
{
    ImGuiTabsContext& ctx = GTabs;
    IM_ASSERT(!ctx.CurrentTabBar.empty());      // Mismatched BeginTabBar/EndTabBar

    PopID();
    ctx.CurrentTabBar.pop_back();
}

// Not sure this is really useful...
void    ImGui::SetTabItemSelected(const char* label)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiTabsContext& ctx = GTabs;
    IM_ASSERT(!ctx.CurrentTabBar.empty());  // Needs to be called between BeginTabBar() and EndTabBar()
    ImGuiTabBar* tab_bar = ctx.CurrentTabBar.back();

    const ImGuiID id = window->GetID(label);
    IM_ASSERT(tab_bar->WantLayout);         // Needs to be called between BeginTabBar() and before the first call to TabItem()
    tab_bar->NextSelectedTabId = id;
}

// This is call is 100% optional and allow to remove some one-frame glitches when a tab has been unexpectedly removed.
// To use it to need to call the function SetTabItemClosed() after BeginTabBar() and before any call to TabItem()
void    ImGui::SetTabItemClosed(const char* label)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiTabsContext& ctx = GTabs;
    IM_ASSERT(!ctx.CurrentTabBar.empty());  // Needs to be called between BeginTabBar() and EndTabBar()
    ImGuiTabBar* tab_bar = ctx.CurrentTabBar.back();

    const ImGuiID id = window->GetID(label);
    IM_ASSERT(tab_bar->WantLayout);         // Needs to be called between BeginTabBar() and before the first call to TabItem()
    if (ImGuiTabItem* tab = ctx.Tabs.GetByKey(id))
    {
        if (tab->LastFrameVisible == tab_bar->LastFrameVisible)
            tab_bar->NextTabCount--;
        tab->LastFrameVisible = -1;
        if (tab->Id == tab_bar->CurrSelectedTabId)
        {
            tab_bar->CurrSelectedTabId = tab_bar->NextSelectedTabId = 0;
            tab_bar->CurrSelectedTabIdIsAlive = false;
        }
    }
}

bool    ImGui::TabItem(const char* label, bool* p_open, ImGuiTabItemFlags flags)
{
    // Acquire tab bar data
    ImGuiTabsContext& ctx = GTabs;
    IM_ASSERT(!ctx.CurrentTabBar.empty());  // Needs to be called between BeginTabBar() and EndTabBar()
    ImGuiTabBar* tab_bar = ctx.CurrentTabBar.back();
    if (tab_bar->WantLayout)
        TabBarLayout(tab_bar);

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    //if (window->SkipItems) // FIXME-OPT
    //    return false;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    // If the user called us with *p_open == false, we early out and don't render. We make a dummy call to ItemAdd() so that attempts to use a contextual popup menu with an implicit ID won't use an older ID.
    if (p_open && !*p_open)
    {
        PushItemFlag(ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus, true);
        ItemAdd(ImRect(), id);
        PopItemFlag();
        return false;
    }
    tab_bar->NextTabCount++;

    // Acquire tab data
    ImGuiTabItem* tab = ctx.Tabs.GetOrCreateByKey(id);
    if (tab->Id == 0)
    {
        // Initialize tab
        tab->Id = id;
        tab->GlobalIndex = ctx.Tabs.Data.Size - 1;
        tab_bar->TabsOrder.push_back(tab->GlobalIndex);
    }
    ImStrncpy(tab->DebugName, label, IM_ARRAYSIZE(tab->DebugName));

    const bool tab_bar_appearing = (tab_bar->LastFrameVisible + 1 < g.FrameCount);
    const bool tab_appearing = (tab->LastFrameVisible + 1 < g.FrameCount);
    const bool tab_selected = (tab_bar->CurrSelectedTabId == id);

    // Position newly appearing tab at the end of the tab list
    if (tab_appearing && !tab_bar_appearing && !(tab_bar->Flags & ImGuiTabBarFlags_NoResetOrderOnAppearing))
    {
        tab->OffsetTarget = tab_bar->OffsetMax + g.Style.ItemInnerSpacing.x;
        if (tab->CurrentOrder != -1 && tab_bar->TabsOrder.back() != tab->GlobalIndex)
        {
            // Move tab to end of the list
            IM_ASSERT(tab_bar->TabsOrder[tab->CurrentOrder] == tab->GlobalIndex);
            memmove(&tab_bar->TabsOrder[tab->CurrentOrder], &tab_bar->TabsOrder[tab->CurrentOrder + 1], (tab_bar->TabsOrder.Size - tab->CurrentOrder - 1) * sizeof(tab_bar->TabsOrder[0]));
            tab_bar->TabsOrder.back() = tab->GlobalIndex;
            tab_bar->CurrOrderInsideTabsIsValid = false;
        }
    }

    // Update selected tab
    if (tab_appearing && !(tab_bar->Flags & ImGuiTabBarFlags_NoSelectionOnAppearing) && tab_bar->NextSelectedTabId == 0)
        tab_bar->NextSelectedTabId = id;  // New tabs gets activated
    if (tab_bar->CurrSelectedTabId == id)
    {
        tab_bar->CurrSelectedTabIdIsAlive = true;
        tab->LastFrameSelected = g.FrameCount;
    }

    // Backup current layout position
    const ImVec2 backup_main_cursor_pos = window->DC.CursorPos;

    // Calculate tab contents size
    ImVec2 label_size = CalcTextSize(label, NULL, true);
    ImVec2 size = TabBarCalcTabBaseSize(label_size, p_open);
    tab->WidthContents = size.x;

    // Animate
    {
        // If Tab just reappeared we'll animate vertically only
        if (tab_appearing && !tab_bar_appearing)
        {
            tab->OffsetAnim = tab->OffsetTarget;
            tab->AppearAnim = 0.0f;
        }
        // If Tab Bar just reappeared we aren't animating at all.
        if (tab_appearing || tab_bar_appearing || tab->WidthAnim == 0.0f)
        {
            tab->OffsetAnim = tab->OffsetTarget;
            tab->WidthAnim = tab->WidthTarget;
        }
        if (tab_bar_appearing || tab->SkipOffsetAnim)
        {
            tab->AppearAnim = 1.0f;
            tab->SkipOffsetAnim = false;
        }

        const float speed_x = (tab_bar->Flags & ImGuiTabBarFlags_NoAnim) ? (FLT_MAX) : (g.FontSize * 80.0f * g.IO.DeltaTime);
        const float speed_y = (tab_bar->Flags & ImGuiTabBarFlags_NoAnim) ? (FLT_MAX) : (g.FontSize * 0.80f * g.IO.DeltaTime);
        tab->OffsetAnim = ImLinearSweep(tab->OffsetAnim, tab->OffsetTarget, speed_x);
        tab->WidthAnim = ImLinearSweep(tab->WidthAnim, tab->WidthTarget, speed_x);
        tab->AppearAnim = ImLinearSweep(tab->AppearAnim, 1.0f, speed_y);
    }

    tab->LastFrameVisible = g.FrameCount;
    size.x = tab->WidthAnim;

    // Layout
    window->DC.CursorPos = tab_bar->BarRect.Min + ImVec2((float)(int)tab->OffsetAnim - tab_bar->ScrollingAnim, 0.0f);
    ImVec2 pos = window->DC.CursorPos;

    ImRect bb(pos, pos + size);
    bool want_clip_rect = (bb.Max.x >= tab_bar->BarRect.Max.x) || (tab->AppearAnim < 1.0f);
    if (want_clip_rect)
        PushClipRect(ImVec2(bb.Min.x, bb.Min.y - 1), ImVec2(tab_bar->BarRect.Max.x, bb.Max.y), true);

    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id))
    {
        if (want_clip_rect)
            PopClipRect();
        window->DC.CursorPos = backup_main_cursor_pos;
        return tab_selected && !tab_appearing;
    }

    // Click to Select a tab
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnDragDropHold | ImGuiButtonFlags_AllowItemOverlap);
    hovered |= (g.HoveredId == id);
    if (pressed || (flags & ImGuiTabItemFlags_SetSelected))
        tab_bar->NextSelectedTabId = id;
    //if (tab_is_selected)
    //    hovered = true;

    // Allow the close button to overlap unless we are dragging (in which case we don't want any overlapping tabs to be hovered)
    if (!held)
        SetItemAllowOverlap();

    // Drag and drop
    if (held && !tab_appearing && IsMouseDragging())
    {
        // Re-order local tabs
        if (!(tab_bar->Flags & ImGuiTabBarFlags_NoReorder))
        {
            // CurrentOrderInsideTabsIsValid is only going to be invalid when a new tab appeared this frame and was pushed to the end of the list, and wasn't already at the end of the internal list.
            // It will prevent changing tab order during this frame, but it can be triggered on the next frame. With the mouse movement required to move tabs, it's unlikely someone will ever even notice.
            // The correct solution would be to refresh the tab order at least for the selected tab. Probably not worth bothering.
            if (tab_bar->CurrOrderInsideTabsIsValid)
            {
                // VisualStudio style
                const float anim_dx = (tab->OffsetTarget - tab->OffsetAnim); // Interaction always operated on target positions, ignoring animations!
                if (g.IO.MouseDelta.x < 0.0f && g.IO.MousePos.x < bb.Min.x + anim_dx)
                    TabBarQueueChangeTabOrder(tab_bar, tab, -1);
                else if (g.IO.MouseDelta.x > 0.0f && g.IO.MousePos.x > bb.Max.x + anim_dx)
                    TabBarQueueChangeTabOrder(tab_bar, tab, +1);

                // Live translate display (like Sublime). Need to comment 'tab2->SkipLayoutAnim = true' in EndTabBar() too.
                #if TAB_SMOOTH_DRAG
                    bb.Translate(ImVec2((g.IO.MousePos.x - g.ActiveIdClickOffset.x) - bb.Min.x, 0.0f));
                #endif
            }
        }
    }

    bool just_closed = false;

    // Render
    ImDrawList* draw_list = window->DrawList;
    const float close_button_sz = g.FontSize * 0.5f;
    if (!tab_appearing)
    {
        // Render: very small offset to make selected tab stick out
        bb.Min.y += tab_selected ? -1.0f : 0.0f;

        // Render: offset vertically + clipping when animating (we don't have enough CPU clipping primitives to clip the CloseButton, so this temporarily adds 1 draw call)
        if (tab->AppearAnim < 1.0f)
            bb.Translate(ImVec2(0.0f, (float)(int)((1.0f - tab->AppearAnim) * size.y)));

#if 1
        bool unfocused = (tab_selected) && (!g.NavWindow || g.NavWindow->RootWindowForTitleBarHighlight != window->RootWindow);
#else
        bool unfocused = false;
#endif

        // Render tab shape
        const ImU32 col = TabGetColorU32((hovered && held) ? ImGuiCol_TabActive : hovered ? ImGuiCol_TabHovered : tab_selected ? (unfocused ? ImGuiCol_TabUnfocused : ImGuiCol_TabActive) : ImGuiCol_Tab);
        RenderTabBackground(draw_list, bb, col);

        // Render text label (with clipping + alpha gradient) + unsaved marker
        const char* TAB_UNSAVED_MARKER = "*";
        ImRect text_clip_bb(bb.Min + style.FramePadding, bb.Max);
        text_clip_bb.Max.x -= g.Style.ItemInnerSpacing.x;
        float text_gradient_extent = g.FontSize * 1.5f;
        if (flags & ImGuiTabItemFlags_UnsavedDocument)
        {
            text_clip_bb.Max.x -= CalcTextSize(TAB_UNSAVED_MARKER, NULL, false).x;
            ImVec2 unsaved_marker_pos(ImMin(bb.Min.x + style.FramePadding.x + label_size.x + 1, text_clip_bb.Max.x), bb.Min.y + style.FramePadding.y + (float)(int)(-g.FontSize * 0.25f));
            RenderTextClipped(unsaved_marker_pos, bb.Max - style.FramePadding, TAB_UNSAVED_MARKER, NULL, NULL);
        }

        // Close Button
        bool close_button_visible = false;
        if (p_open != NULL)
        {
            // We are relying on a subtle and confusing distinction between 'hovered' and 'g.HoveredId' which happens because we are using ImGuiButtonFlags_AllowOverlapMode + SetItemAllowOverlap()
            //  'hovered' will be true when hovering the Tab but NOT when hovering the close button
            //  'g.HoveredId==id' will be true when hovering the Tab including when hovering the close button
            //  'g.ActiveId==close_button_id' will be true when we are holding on the close button, in which case both hovered booleans are false
            const ImGuiID close_button_id = window->GetID((void*)(intptr_t)(id + 1));
            const bool hovered_unblocked = IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
            const bool hovered_regular = g.HoveredId == id || g.HoveredId == close_button_id;
            if (hovered_regular || g.ActiveId == close_button_id)
            {
                close_button_visible = true;
                ImGuiItemHoveredDataBackup last_item_backup;
                last_item_backup.Backup();
                if (CloseButton(close_button_id, ImVec2(bb.Max.x - style.FramePadding.x - close_button_sz, bb.Min.y + style.FramePadding.y + close_button_sz), close_button_sz))
                    just_closed = true;
                last_item_backup.Restore();

                // Close with middle mouse button
                if (!(tab_bar->Flags & ImGuiTabBarFlags_NoCloseWithMiddleMouseButton) && IsMouseClicked(2))
                    just_closed = true;
            }

            // Select with right mouse button. This is so the common idiom for context menu automatically highlight the current widget.
            if (IsMouseClicked(1) && hovered_unblocked)
                tab_bar->NextSelectedTabId = id;
        }
        if (close_button_visible)
        {
            float dx = close_button_sz + style.FramePadding.x * 1.0f;    // Because we fade clipped label we don't need FramePadding * 2;
            text_clip_bb.Max.x -= dx;
            text_gradient_extent = ImMax(0.0f, text_gradient_extent - dx);
        }

        // Text with alpha fade if it doesn't fit
        // FIXME: Move into fancy RenderText* helpers.
        int vert_start_idx = draw_list->_VtxCurrentIdx;
        RenderTextClipped(text_clip_bb.Min, text_clip_bb.Max, label, NULL, &label_size, ImVec2(0.0f, 0.0f));
        if (text_clip_bb.GetWidth() < label_size.x)
            ShadeVertsLinearAlphaGradientForLeftToRightText(draw_list, vert_start_idx, draw_list->_VtxCurrentIdx, text_clip_bb.Max.x - text_gradient_extent, text_clip_bb.Max.x);
    }

    // Process close
    if (just_closed)
    {
        *p_open = false;
        if (tab_selected && !(flags & ImGuiTabItemFlags_UnsavedDocument))
        {
            // This will remove a frame of lag for selecting another tab on closure.
            // However we don't run it in the case where the 'Unsaved' flag is set, so user gets a chance to fully undo the closure
            tab->LastFrameVisible = -1;
            tab_bar->NextSelectedTabId = 0;
            tab_bar->CurrSelectedTabIdIsAlive = false;
        }
        else if (!tab_selected && (flags & ImGuiTabItemFlags_UnsavedDocument))
        {
            // Actually select before expecting closure
            tab_bar->NextSelectedTabId = id;
        }
    }

    // Restore main window position so user can draw there
    if (want_clip_rect)
        PopClipRect();
    window->DC.CursorPos = backup_main_cursor_pos;

    return tab_selected && !tab_appearing;
}

//-----------------------------------------------------------------------------
// DEMO & DEBUG CODE
//-----------------------------------------------------------------------------

struct MyDocument
{
    const char* Name;
    bool        Open;
    bool        PreviousOpen;
    bool        Dirty;
    bool        WantClose;

    MyDocument()        { Name = NULL; Open = true;  PreviousOpen = false; Dirty = false; WantClose = false;  }
    void DoOpen()       { Open = true; }
    void DoQueueClose() { WantClose = true; }
    void DoForceClose() { Open = false; Dirty = false; }
    void DoSave()       { Dirty = false; }
};

static MyDocument GDocs[6];

static void MyContextMenu(MyDocument& doc)
{
    if (ImGui::BeginPopupContextItem())
    {
        char buf[256];
        sprintf(buf, "Save %s", doc.Name);
        if (ImGui::MenuItem(buf, "CTRL+S", false, doc.Open))
            doc.DoSave();
        if (ImGui::MenuItem("Close", "CTRL+W", false, doc.Open))
            doc.DoQueueClose();
        if (ImGui::MenuItem("Close All Documents")) // FIXME: Missing save hook
            for (int doc_n = 0; doc_n < IM_ARRAYSIZE(GDocs); doc_n++)
                GDocs[doc_n].DoQueueClose();
        ImGui::EndPopup();
    }
}

void    ImGui::ShowTabsDemo(const char* title, bool* p_open)
{
    ImGui::Begin(title, p_open, ImGuiWindowFlags_MenuBar);

    if (GDocs[0].Name == NULL)
    {
        GDocs[0].Name = "Hello";
        GDocs[1].Name = "Document 2";
        GDocs[2].Name = "Document 3";
        GDocs[3].Name = "Document 4";
        GDocs[4].Name = "A Rather Long Filename";
        GDocs[5].Name = "Document 5";
        GDocs[3].Open = GDocs[4].Open = GDocs[5].Open = false;
    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            int open_count = 0;
            for (int doc_n = 0; doc_n < IM_ARRAYSIZE(GDocs); doc_n++)
                open_count += (int)GDocs[doc_n].Open;

            if (ImGui::BeginMenu("Open", open_count < IM_ARRAYSIZE(GDocs)))
            {
                for (int doc_n = 0; doc_n < IM_ARRAYSIZE(GDocs); doc_n++)
                {
                    MyDocument& doc = GDocs[doc_n];
                    if (!doc.Open)
                        if (ImGui::MenuItem(doc.Name))
                            doc.DoOpen();
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Close All Documents", NULL, false, open_count > 0)) // FIXME: Missing save hook
                for (int doc_n = 0; doc_n < IM_ARRAYSIZE(GDocs); doc_n++)
                    GDocs[doc_n].DoQueueClose();
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Options/Debug
#if 1
    ImGui::AlignTextToFramePadding();
    static bool opt_anim = true;
    static ImGuiTabBarFlags opt_sizing_flags = ImGuiTabBarFlags_SizingPolicyDefault_;
    ImGui::Checkbox("Anim", &opt_anim);
    ImGui::SameLine();
    ImGui::PushItemWidth(80);
    struct 
    {
        ImGuiTabBarFlags Flags;
        const char*      Desc;
    } sizing_options[] =
    {
        { ImGuiTabBarFlags_SizingPolicyFit, "Fit" },
        { ImGuiTabBarFlags_SizingPolicyEqual, "Equal" },
    };
    const char* opt_sizing_desc = NULL;
    for (int n = 0; n < IM_ARRAYSIZE(sizing_options); n++)
        if (opt_sizing_flags & sizing_options[n].Flags)
            opt_sizing_desc = sizing_options[n].Desc;

    if (ImGui::BeginCombo("##sizing", opt_sizing_desc))
    {
        for (int n = 0; n < IM_ARRAYSIZE(sizing_options); n++)
            if (ImGui::MenuItem(sizing_options[n].Desc))
                opt_sizing_flags = sizing_options[n].Flags;
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    for (int doc_n = 0; doc_n < IM_ARRAYSIZE(GDocs); doc_n++)
    {
        MyDocument& doc = GDocs[doc_n];
        if (doc_n > 0) ImGui::SameLine();
        ImGui::PushID(&doc);
        if (ImGui::Checkbox(doc.Name, &doc.Open))
            if (!doc.Open)
                doc.DoForceClose();
        ImGui::PopID();
    }
    ImGui::Separator();
#endif

    // Tabs
    ImGui::BeginTabBar("##tabs", (opt_anim ? 0 : ImGuiTabBarFlags_NoAnim) | (opt_sizing_flags));

#if 1
    // Testing explicit SetTabItemClosed() prepass to fix some flickering glitches
    // Another solution would be to specifically keep rendering a tab as selected for 1 frame when unexpectedly closed, but that would requires users to keep calling TabItem even when the bool is false..
    for (int doc_n = 0; doc_n < IM_ARRAYSIZE(GDocs); doc_n++)
    {
        MyDocument& doc = GDocs[doc_n];
        if (!doc.Open && doc.PreviousOpen)
            ImGui::SetTabItemClosed(doc.Name);
        doc.PreviousOpen = doc.Open;
    }
    //if ((ImGui::GetFrameCount() % 30) == 0) docs[1].Open ^= 1;            // [DEBUG] Automatically show/hide a tab. Test various interactions e.g. dragging with this on.
    //if (ImGui::GetIO().KeyCtrl) ImGui::SetTabItemSelected(docs[1].Name);  // [DEBUG] Test SetTabItemSelected(), probably not very useful as-is anyway..
#endif

    for (int doc_n = 0; doc_n < IM_ARRAYSIZE(GDocs); doc_n++)
    {
        MyDocument& doc = GDocs[doc_n];
        const bool was_open = doc.Open;
        const ImGuiTabItemFlags flags = (doc.Dirty ? ImGuiTabItemFlags_UnsavedDocument : 0);
        //if (doc_n == 1 && ImGui::GetIO().KeyCtrl) flags |= ImGuiTabItemFlags_SetSelected; // [DEBUG]
        const bool selected = ImGui::TabItem(doc.Name, &doc.Open, flags);
        MyContextMenu(doc);

        // Catch attempt to close when unsaved
        if (was_open && !doc.Open && doc.Dirty)
        {
            doc.Open = true;
            doc.DoQueueClose();
        }
        if (!selected)
            continue;

        ImGui::Text("Exciting Document %d", doc_n + 1);
        ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
        switch (doc_n)
        {
        case 0:
            ImGui::Text("This is the HELLO document, for all your HELLO needs!");
            break;
        case 1:
        case 4:
            if (ImGui::Button("Modify", ImVec2(100, 0)))
                doc.Dirty = true;
            ImGui::SameLine();
            if (ImGui::Button("Save", ImVec2(100, 0)))
                doc.DoSave();
            break;
        case 2:
            ImGui::Text("Actually I am a boring document.");
            break;
        }
    }
    ImGui::EndTabBar();

    // Process closing queue
    static ImVector<MyDocument*> close_queue;
    if (close_queue.empty()) // Close queue is locked once we started a popup?
        for (int doc_n = 0; doc_n < IM_ARRAYSIZE(GDocs); doc_n++)
        {
            MyDocument& doc = GDocs[doc_n];
            if (doc.WantClose)
            {
                doc.WantClose = false;
                close_queue.push_back(&doc);
            }
        }
    if (!close_queue.empty())
    {
        int close_queue_unsaved_documents = 0;
        for (int n = 0; n < close_queue.Size; n++)
            if (close_queue[n]->Dirty)
                close_queue_unsaved_documents++;

        if (close_queue_unsaved_documents == 0)
        {
            // Close documents when all are unsaved
            for (int n = 0; n < close_queue.Size; n++)
                close_queue[n]->DoForceClose();
            close_queue.clear();
        }
        else
        {
            if (!ImGui::IsPopupOpen("Save?"))
                ImGui::OpenPopup("Save?");
            if (ImGui::BeginPopup("Save?"))//, NULL, ImGuiWindowFlags_NoResize))
            {
                ImGui::Text("Save change to the following items?");
                ImGui::PushItemWidth(-1.0f);
                ImGui::ListBoxHeader("##", close_queue_unsaved_documents, 6);
                for (int n = 0; n < close_queue.Size; n++)
                    if (close_queue[n]->Dirty)
                        ImGui::Text("%s", close_queue[n]->Name);
                ImGui::ListBoxFooter();

                if (ImGui::Button("Yes", ImVec2(80, 0)))
                {
                    for (int n = 0; n < close_queue.Size; n++)
                    {
                        if (close_queue[n]->Dirty)
                            close_queue[n]->DoSave();
                        close_queue[n]->DoForceClose();
                    }
                    close_queue.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("No", ImVec2(80, 0)))
                {
                    for (int n = 0; n < close_queue.Size; n++)
                        close_queue[n]->DoForceClose();
                    close_queue.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(80, 0)))
                {
                    close_queue.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }

    ImGui::End();
}

void ImGui::ShowTabsDebug()
{
    ImGui::Begin("Tabs Debug");
    ImGuiTabsContext& ctx = GTabs;
    ImGui::Text("%d TabBars, %d Tabs", ctx.TabBars.Data.Size, ctx.Tabs.Data.Size);
    for (int tab_bar_n = 0; tab_bar_n < ctx.TabBars.Data.Size; tab_bar_n++)
    {
        ImGuiTabBar* tab_bar = &ctx.TabBars.Data[tab_bar_n];
        ImGui::Separator();
        ImGui::Text("TabBar 0x%08X", tab_bar->Id);
        ImGui::Text("TabsOrder.Size %d", tab_bar->TabsOrder.Size);

        for (int tab_n = 0; tab_n < tab_bar->TabsOrder.Size; tab_n++)
        {
            const ImGuiTabItem* tab = ctx.GetTabByOrder(tab_bar, tab_n);
            ImGui::PushID(tab);
            if (ImGui::Button("<")) TabBarQueueChangeTabOrder(tab_bar, tab, -1); ImGui::SameLine(0, 2);
            if (ImGui::Button(">")) TabBarQueueChangeTabOrder(tab_bar, tab, +1); ImGui::SameLine();
            if (ImGui::TreeNode((void*)(intptr_t)tab_n, "%02d%c Tab 0x%08X '%s'", tab_n, (tab->Id == tab_bar->CurrSelectedTabId) ? '*' : ' ', tab->Id, tab->DebugName))
            {
                ImGui::Text("GlobalIndex = %d", tab->GlobalIndex);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
}
