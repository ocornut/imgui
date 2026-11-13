// dear imgui: stack layouts
// (stack layout code)

/*

Index of this file:

// [SECTION] Commentary
// [SECTION] Header mess
// [SECTION] Stack layout: Forward declarations
// [SECTION] Stack layout: flags, enums, data structures
// [SECTION] Stack Layout: Context forward declarations
// [SECTION] Stack Layout: Internal code forward declarations
// [SECTION] Stack Layout: Context
// [SECTION] Stack Layout: Internal code
// [SECTION] Stack Layout: Internal API
// [SECTION] Stack Layout: Public API

*/

// Navigating this file:
// - In Visual Studio IDE: CTRL+comma ("Edit.NavigateTo") can follow symbols in comments, whereas CTRL+F12 ("Edit.GoToImplementation") cannot.
// - With Visual Assist installed: ALT+G ("VAssistX.GoToImplementation") can also follow symbols in comments.

//-----------------------------------------------------------------------------
// [SECTION] Commentary
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Typical call flow: (root level is generally public API):
//-----------------------------------------------------------------------------
//
// - BeginHorizontal/BeginVertical()            user begin into a layout
// - [...]                                      user emit contents
// -  | Spring()                                - separate widget groups in layout
// -  | SuspendLayout()                         - stop any layout operations
// -  | ResumeLayout()                          - resume layout operations
//-----------------------------------------------------------------------------
// - EndHorizontal/EndVertical()                user ends the layout
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// [SECTION] Header mess
//-----------------------------------------------------------------------------

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_internal.h"

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                      // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"                    // warning: comparing floating point with == or != is unsafe // storing and comparing against same constants (typically 0.0f) is ok.
#pragma clang diagnostic ignored "-Wsign-conversion"                // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"            // warning: 'xxx' is an unsafe pointer used for buffer access
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                          // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wfloat-equal"                      // warning: comparing floating-point with '==' or '!=' is unsafe
#pragma GCC diagnostic ignored "-Wconversion"                       // warning: conversion to 'xxxx' from 'xxxx' may change value
#pragma GCC diagnostic ignored "-Wsign-conversion"                  // warning: conversion to 'xxxx' from 'xxxx' may change the sign of the result
#endif

namespace ImGuiStackLayout
{

//-----------------------------------------------------------------------------
// [SECTION] Stack layout: Forward declarations
//-----------------------------------------------------------------------------

// Use your programming IDE "Go to definition" facility on the names of the center columns to find the actual flags/enum lists.
typedef int ImGuiLayoutItemType;        // -> enum ImGuiLayoutItemType_     // Enum: Item or Spring


//-----------------------------------------------------------------------------
// [SECTION] Stack layout: flags, enums, data structures
//-----------------------------------------------------------------------------

enum ImGuiLayoutItemType_
{
    ImGuiLayoutItemType_Item,
    ImGuiLayoutItemType_Spring
};

// sizeof() == 48
struct ImGuiLayoutItem
{
    ImGuiLayoutItemType     Type;               // Type of an item
    ImRect                  MeasuredBounds;

    float                   SpringWeight;       // Weight of a spring
    float                   SpringSpacing;      // Spring spacing
    float                   SpringSize;         // Calculated spring size

    float                   CurrentAlign;
    float                   CurrentAlignOffset;

    int                     VertexIndexBegin;
    int                     VertexIndexEnd;

    ImGuiLayoutItem(ImGuiLayoutItemType type)
    {
        Type = type;
        MeasuredBounds = ImRect(0, 0, 0, 0);    // FIXME: @thedmd are you sure the default ImRect value FLT_MAX,FLT_MAX,-FLT_MAX,-FLT_MAX aren't enough here?
        SpringWeight = 1.0f;
        SpringSpacing = -1.0f;
        SpringSize = 0.0f;
        CurrentAlign = 0.0f;
        CurrentAlignOffset = 0.0f;
        VertexIndexBegin = VertexIndexEnd = 0;
    }
};

struct ImGuiLayout
{
    ImGuiID                     Id;
    ImGuiLayoutType             Type;
    int                         LastFrameUsed;
    float                       LastTimeUsed;
    int                         InstanceCount;      // Further uses of the ID in the current frame.
    int                         IDStackSize;        // Size of the ID stack before BeginLayout() pushed the ID.
    int                         GroupStackSize;     // Size of the group stack when BeginLayout() was called.
    ImVec2                      Size;               // Size passed to BeginLayout
    ImVec2                      CurrentSize;        // Bounds of layout known at the beginning the frame.
    ImVec2                      MinimumSize;        // Minimum possible size when springs are collapsed.
    ImVec2                      MeasuredSize;       // Measured size with springs expanded.

    ImVector<ImGuiLayoutItem>   Items;
    int                         CurrentItemIndex;
    int                         ParentItemIndex;
    ImGuiLayout*                Parent;
    ImGuiLayout*                FirstChild;
    ImGuiLayout*                NextSibling;
    float                       Align;              // Current item alignment.
    ImVec2                      StartPos;           // Initial cursor position when BeginLayout is called.
    ImVec2                      StartCursorMaxPos;  // Maximum cursor position when BeginLayout is called.

    // Line state of the enclosing flow, restored by EndLayout() as EndGroup() does.
    ImVec2                      BackupCursorPosPrevLine;
    ImVec1                      BackupIndent;
    ImVec2                      BackupCurrLineSize;
    float                       BackupCurrLineTextBaseOffset;
    bool                        BackupIsSameLine;
    ImGuiLayoutType             BackupLayoutType;

    // Last item when the current layout item began, to tell whether an item was submitted inside it.
    ImGuiID                     ItemBeginLastItemId;
    ImRect                      ItemBeginLastItemRect;

    bool                        ClipRectPushed;

    // Root layouts: how far corrections can move items, and the clip rect widened by it while items are submitted.
    ImVec2                      SpringSizeInTree;   // Spring sizes of the layout and its nested layouts at the end of the previous frame, per axis.
    bool                        WideClipRectPushed;
    int                         WideClipRectCmdIndex;
    ImVec4                      WideClipRect;
    ImVec4                      NarrowClipRect;
    ImGuiLayoutDebugFlags       DebugFlags;

    ImGuiLayout(ImGuiID id, ImGuiLayoutType type)
    {
        Id = id;
        Type = type;
        LastFrameUsed = -1;
        LastTimeUsed = -1.0f;
        InstanceCount = 0;
        IDStackSize = 0;
        GroupStackSize = 0;
        Size = CurrentSize = MinimumSize = MeasuredSize = ImVec2(0, 0);
        CurrentItemIndex = 0;
        ParentItemIndex = 0;
        Parent = FirstChild = NextSibling = NULL;
        Align = -1.0f;
        StartPos = ImVec2(0, 0);
        StartCursorMaxPos = ImVec2(0, 0);
        BackupCursorPosPrevLine = BackupCurrLineSize = ImVec2(0, 0);
        BackupCurrLineTextBaseOffset = 0.0f;
        BackupIsSameLine = false;
        BackupLayoutType = ImGuiLayoutType_Vertical;
        ItemBeginLastItemId = 0;
        ClipRectPushed = false;
        SpringSizeInTree = ImVec2(0, 0);
        WideClipRectPushed = false;
        WideClipRectCmdIndex = 0;
        WideClipRect = NarrowClipRect = ImVec4(0, 0, 0, 0);
        DebugFlags = ImGuiLayoutDebugFlags_None;
    }
};

struct ImGuiLayoutSuspendBackup
{
    ImGuiLayoutType         LayoutType;
    ImVec2                  CursorMaxPos;
    int                     IDStackSize;        // Size of the ID stack before SuspendLayout() pushed the current ID again.
};

struct ImGuiLayoutWindowState
{
    ImGuiWindow*            Window;
    ImGuiLayout*            CurrentLayout;
    ImVector<ImGuiLayout*>  LayoutStack;
    ImVector<ImGuiLayoutSuspendBackup> SuspendStack;
    ImGuiStorage            Layouts;

    ImGuiLayoutWindowState()
    {
        Window            = NULL;
        CurrentLayout     = NULL;
    }
};

struct ImGuiLayoutContextState
{
    ImGuiStorage            WindowStates;       // ImGuiLayoutWindowState* by window ID
    ImGuiLayoutWindowState* LastWindowState;
    ImGuiLayoutDebugFlags   NextDebugFlags;

    ImGuiLayoutContextState()
    {
        LastWindowState = NULL;
        NextDebugFlags = ImGuiLayoutDebugFlags_None;
    }
};


//-----------------------------------------------------------------------------
// [SECTION] Stack Layout: Context forward declarations
//-----------------------------------------------------------------------------

// The state of a context is the UserData of its Shutdown hook, found by the hook owner.
static ImGuiID                  GetHookOwnerID();
static ImGuiLayoutContextState* FindContextState(ImGuiContext& g);
static ImGuiLayoutContextState* GetContextState(ImGuiContext& g);
static void                     NewFramePreHook(ImGuiContext* ctx, ImGuiContextHook* hook);
static void                     ShutdownHook(ImGuiContext* ctx, ImGuiContextHook* hook);

static ImGuiLayoutWindowState*  GetCurrentWindowLayoutState();
static void                     DestroyWindowLayoutState(ImGuiLayoutWindowState* window_state);


//-----------------------------------------------------------------------------
// [SECTION] Stack Layout: Internal code forward declarations
//-----------------------------------------------------------------------------

// Stack Layout
static ImGuiLayout*     FindLayout(ImGuiID id, ImGuiLayoutType type);
static ImGuiLayout*     CreateNewLayout(ImGuiID id, ImGuiLayoutType type, ImVec2 size);
static void             BeginLayout(ImGuiID id, ImGuiLayoutType type, ImVec2 size, float align);
static void             EndLayout(ImGuiLayoutWindowState* window_state);
static ImVec2           CalculateLayoutSize(ImGuiLayout& layout, bool collapse_springs);
static void             PushLayout(ImGuiLayout* layout);
static void             PopLayout(ImGuiLayout* layout);
static void             BalanceLayoutSprings(ImGuiLayout& layout);
static ImVec2           BalanceLayoutItemAlignment(ImGuiLayout& layout, ImGuiLayoutItem& item);
static void             BalanceLayoutItemsAlignment(ImGuiLayout& layout);
static bool             HasAnyNonZeroSpring(ImGuiLayout& layout);
static void             BalanceChildLayouts(ImGuiLayout& layout);
static void             BeginLayoutClipRect(ImGuiLayout& layout, bool measured);
static void             EndLayoutClipRect(ImGuiLayout& layout);
static void             BeginLayoutWideClipRect(ImGuiLayout& layout);
static void             EndLayoutWideClipRect(ImGuiLayout& layout);
static ImVec2           CalculateSpringSizeInTree(const ImGuiLayout& layout);
static ImGuiLayoutItem* GenerateLayoutItem(ImGuiLayout& layout, ImGuiLayoutItemType type);
static float            CalculateLayoutItemAlignmentOffset(ImGuiLayout& layout, ImGuiLayoutItem& item);
static void             TranslateLayoutItem(ImGuiLayoutItem& item, const ImVec2& offset);
static void             SignedIndent(float indent);
static void             MarkLayoutLastItem(ImGuiLayout& layout);
static void             BeginLayoutItem(ImGuiLayout& layout);
static void             EndLayoutItem(ImGuiLayout& layout);
static void             AddLayoutSpring(ImGuiLayout& layout, float weight, float spacing);
static void             DebugDrawLayout(const ImGuiLayout& layout);


//-----------------------------------------------------------------------------
// [SECTION] Stack Layout: Context
//-----------------------------------------------------------------------------

static ImGuiID GetHookOwnerID()
{
    static const ImGuiID owner_id = ImHashStr("ImGuiStackLayout");
    return owner_id;
}

static ImGuiLayoutContextState* FindContextState(ImGuiContext& g)
{
    const ImGuiID owner_id = GetHookOwnerID();
    for (ImGuiContextHook& hook : g.Hooks)
        if (hook.Owner == owner_id && hook.Type == ImGuiContextHookType_Shutdown)
            return (ImGuiLayoutContextState*)hook.UserData;
    return NULL;
}

static ImGuiLayoutContextState* GetContextState(ImGuiContext& g)
{
    if (ImGuiLayoutContextState* state = FindContextState(g))
        return state;

    ImGuiLayoutContextState* state = IM_NEW(ImGuiLayoutContextState);

    ImGuiContextHook hook;
    hook.Owner    = GetHookOwnerID();
    hook.UserData = state;
    hook.Type     = ImGuiContextHookType_NewFramePre;
    hook.Callback = NewFramePreHook;
    ImGui::AddContextHook(&g, &hook);
    hook.Type     = ImGuiContextHookType_Shutdown;
    hook.Callback = ShutdownHook;
    ImGui::AddContextHook(&g, &hook);

    return state;
}

static void NewFramePreHook(ImGuiContext* ctx, ImGuiContextHook* hook)
{
    ImGuiContext& g = *ctx;
    ImGuiLayoutContextState* state = (ImGuiLayoutContextState*)hook->UserData;

    // Same policy as windows and tables: free layouts unused for io.ConfigMemoryCompactTimer, all of a compacted window.
    const bool gc_all = (g.GcCompactAll || g.IO.ConfigMemoryCompactTimer < 0.0f);
    const float memory_compact_start_time = gc_all ? FLT_MAX : (float)g.Time - g.IO.ConfigMemoryCompactTimer;

    ImVector<ImGuiStoragePair>& window_states = state->WindowStates.Data;
    int window_states_kept = 0;
    for (ImGuiStoragePair& window_pair : window_states)
    {
        ImGuiLayoutWindowState* window_state = (ImGuiLayoutWindowState*)window_pair.val_p;

        // Error recovery in End() closes layouts left open, unless disabled.
        if (window_state->LayoutStack.Size > 0)
        {
            IM_ASSERT_USER_ERROR(0, "Missing EndHorizontal(), EndVertical() or ResumeLayout() at the end of the frame!");
            window_state->LayoutStack.resize(0);
            window_state->SuspendStack.resize(0);
            window_state->CurrentLayout = NULL;
        }

        if (window_state->Window->MemoryCompacted)
        {
            DestroyWindowLayoutState(window_state);
            continue;
        }

        ImVector<ImGuiStoragePair>& layouts = window_state->Layouts.Data;
        int layouts_kept = 0;
        for (ImGuiStoragePair& layout_pair : layouts)
        {
            ImGuiLayout* layout = (ImGuiLayout*)layout_pair.val_p;
            if (layout->LastFrameUsed != g.FrameCount && layout->LastTimeUsed < memory_compact_start_time)
                IM_DELETE(layout);
            else
                layouts[layouts_kept++] = layout_pair;
        }
        layouts.resize(layouts_kept);

        window_states[window_states_kept++] = window_pair;
    }
    window_states.resize(window_states_kept);
    state->LastWindowState = NULL;
}

static void ShutdownHook(ImGuiContext* ctx, ImGuiContextHook* hook)
{
    IM_UNUSED(ctx);
    ImGuiLayoutContextState* state = (ImGuiLayoutContextState*)hook->UserData;
    for (ImGuiStoragePair& window_pair : state->WindowStates.Data)
        DestroyWindowLayoutState((ImGuiLayoutWindowState*)window_pair.val_p);
    IM_DELETE(state);
    hook->UserData = NULL;
}

static ImGuiLayoutWindowState* GetCurrentWindowLayoutState()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiLayoutContextState* state = GetContextState(g);
    if (state->LastWindowState != NULL && state->LastWindowState->Window == window)
        return state->LastWindowState;

    ImGuiLayoutWindowState* window_state = (ImGuiLayoutWindowState*)state->WindowStates.GetVoidPtr(window->ID);
    if (window_state == NULL)
    {
        window_state = IM_NEW(ImGuiLayoutWindowState);
        window_state->Window = window;
        state->WindowStates.SetVoidPtr(window->ID, window_state);
    }
    state->LastWindowState = window_state;
    return window_state;
}

static void DestroyWindowLayoutState(ImGuiLayoutWindowState* window_state)
{
    for (ImGuiStoragePair& layout_pair : window_state->Layouts.Data)
        IM_DELETE((ImGuiLayout*)layout_pair.val_p);
    IM_DELETE(window_state);
}


//-----------------------------------------------------------------------------
// [SECTION] Stack Layout: Internal code
//-----------------------------------------------------------------------------

static ImGuiLayout* FindLayout(ImGuiID id, ImGuiLayoutType type)
{
    IM_ASSERT(type == ImGuiLayoutType_Horizontal || type == ImGuiLayoutType_Vertical);

    ImGuiLayoutWindowState* window_state = GetCurrentWindowLayoutState();
    ImGuiLayout* layout = (ImGuiLayout*)window_state->Layouts.GetVoidPtr(id);
    if (!layout)
        return NULL;

    if (layout->Type != type)
    {
        layout->Type = type;
        layout->MinimumSize = ImVec2(0.0f, 0.0f);
        layout->Items.clear();
    }

    return layout;
}

static ImGuiLayout* CreateNewLayout(ImGuiID id, ImGuiLayoutType type, ImVec2 size)
{
    IM_ASSERT(type == ImGuiLayoutType_Horizontal || type == ImGuiLayoutType_Vertical);

    ImGuiLayoutWindowState* window_state = GetCurrentWindowLayoutState();

    ImGuiLayout* layout = IM_NEW(ImGuiLayout)(id, type);
    layout->Size = size;

    window_state->Layouts.SetVoidPtr(id, layout);

    return layout;
}

static void BeginLayout(ImGuiID id, ImGuiLayoutType type, ImVec2 size, float align)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (size.x < 0.0f || size.y < 0.0f)
        size = ImGui::CalcItemSize(size, 0.0f, 0.0f);

    const int id_stack_size = window->IDStack.Size;
    ImGui::PushOverrideID(id);

    // Find or create
    ImGuiLayout* layout = FindLayout(id, type);
    if (!layout)
        layout = CreateNewLayout(id, type, size);

    // Each further use of the ID in a frame gets a layout of its own, as table instances do.
    if (layout->LastFrameUsed == g.FrameCount)
    {
        IM_ASSERT_USER_ERROR(0, "BeginHorizontal()/BeginVertical() with an ID already used in this frame! Use PushID() to make IDs unique.");
        const int instance_no = ++layout->InstanceCount;
        const ImGuiID instance_id = ImHashData(&instance_no, sizeof(instance_no), id);
        ImGuiLayout* instance = FindLayout(instance_id, type);
        layout = instance ? instance : CreateNewLayout(instance_id, type, size);
    }
    else
    {
        layout->InstanceCount = 0;
    }

    const bool measured = (layout->LastFrameUsed == g.FrameCount - 1);
    layout->LastFrameUsed = g.FrameCount;
    layout->LastTimeUsed = (float)g.Time;
    layout->IDStackSize = id_stack_size;
    layout->GroupStackSize = g.GroupStack.Size;

    PushLayout(layout);

    ImGuiLayoutContextState* context_state = GetContextState(g);
    layout->DebugFlags = context_state->NextDebugFlags | (layout->Parent ? layout->Parent->DebugFlags : ImGuiLayoutDebugFlags_None);
    context_state->NextDebugFlags = ImGuiLayoutDebugFlags_None;

    if (layout->Size.x != size.x || layout->Size.y != size.y)
        layout->Size = size;

    if (align < 0.0f)
        layout->Align = -1.0f;
    else
        layout->Align = ImClamp(align, 0.0f, 1.0f);

    // Start capture
    layout->CurrentItemIndex = 0;

    layout->CurrentSize.x = layout->Size.x > 0.0f ? layout->Size.x : layout->MinimumSize.x;
    layout->CurrentSize.y = layout->Size.y > 0.0f ? layout->Size.y : layout->MinimumSize.y;

    // The layout is a block starting at the cursor, like a group: save the line it is on.
    layout->StartPos = window->DC.CursorPos;
    layout->StartCursorMaxPos = window->DC.CursorMaxPos;
    layout->BackupCursorPosPrevLine = window->DC.CursorPosPrevLine;
    layout->BackupIndent = window->DC.Indent;
    layout->BackupCurrLineSize = window->DC.CurrLineSize;
    layout->BackupCurrLineTextBaseOffset = window->DC.CurrLineTextBaseOffset;
    layout->BackupIsSameLine = window->DC.IsSameLine;
    layout->BackupLayoutType = window->DC.LayoutType;

    // ItemSize() flows items along the layout axis. New lines start at the layout's left edge.
    window->DC.LayoutType = type;
    window->DC.Indent.x = layout->StartPos.x - window->Pos.x - window->DC.ColumnsOffset.x;
    window->DC.CurrLineSize = ImVec2(0.0f, 0.0f);

    if (layout->Parent == NULL)
        BeginLayoutWideClipRect(*layout);
    BeginLayoutClipRect(*layout, measured);

    BeginLayoutItem(*layout);
}

static void EndLayout(ImGuiLayoutWindowState* window_state)
{
    ImGuiLayout* layout = window_state->CurrentLayout;
    ImGuiWindow* window = window_state->Window;

    EndLayoutItem(*layout);

    if (layout->CurrentItemIndex < layout->Items.Size)
        layout->Items.resize(layout->CurrentItemIndex);

    PopLayout(layout);

    const bool auto_width  = layout->Size.x <= 0.0f;
    const bool auto_height = layout->Size.y <= 0.0f;

    ImVec2 new_size = layout->Size;
    if (auto_width)
        new_size.x = layout->CurrentSize.x;
    if (auto_height)
        new_size.y = layout->CurrentSize.y;

    ImVec2 new_minimum_size = CalculateLayoutSize(*layout, true);

    if (new_minimum_size.x != layout->MinimumSize.x || new_minimum_size.y != layout->MinimumSize.y)
    {
        layout->MinimumSize = new_minimum_size;

        // Shrink
        if (auto_width)
            new_size.x = new_minimum_size.x;
        if (auto_height)
            new_size.y = new_minimum_size.y;
    }

    if (!auto_width)
        new_size.x = layout->Size.x;
    if (!auto_height)
        new_size.y = layout->Size.y;

    layout->CurrentSize = new_size;

    ImVec2 measured_size = new_size;
    // Inflate to parent size only when there are springs that need the free space.
    if ((auto_width || auto_height) && layout->Parent && HasAnyNonZeroSpring(*layout))
    {
        if (layout->Type == ImGuiLayoutType_Horizontal && auto_width && layout->Parent->CurrentSize.x > 0)
            layout->CurrentSize.x = layout->Parent->CurrentSize.x;
        else if (layout->Type == ImGuiLayoutType_Vertical && auto_height && layout->Parent->CurrentSize.y > 0)
            layout->CurrentSize.y = layout->Parent->CurrentSize.y;

        BalanceLayoutSprings(*layout);

        measured_size = layout->CurrentSize;
    }

    layout->CurrentSize = new_size;
    layout->MeasuredSize = measured_size;

    IM_ASSERT_USER_ERROR(window->IDStack.Size == layout->IDStackSize + 1, "Mismatching PushID()/PopID() inside a layout!");
    while (window->IDStack.Size > layout->IDStackSize)
        ImGui::PopID();

    EndLayoutClipRect(*layout);
    EndLayoutWideClipRect(*layout);

    // Restore the enclosing line, then submit the layout as one item, as EndGroup() does.
    window->DC.CursorPos = layout->StartPos;
    window->DC.CursorPosPrevLine = layout->BackupCursorPosPrevLine;
    window->DC.CursorMaxPos = layout->StartCursorMaxPos;
    window->DC.Indent = layout->BackupIndent;
    window->DC.CurrLineSize = layout->BackupCurrLineSize;
    window->DC.CurrLineTextBaseOffset = layout->BackupCurrLineTextBaseOffset;
    window->DC.IsSameLine = layout->BackupIsSameLine;
    window->DC.LayoutType = layout->BackupLayoutType;
    ImGui::ItemSize(new_size);
    // The item rect includes the inflation to the parent, which GetItemRect*() users draw with, except inside a group
    // opened in the parent: EndGroup() would include it, feeding the parent's size back into itself.
    const bool in_group_of_parent = layout->Parent != NULL && layout->GroupStackSize > layout->Parent->GroupStackSize;
    ImGui::ItemAdd(ImRect(layout->StartPos, layout->StartPos + (in_group_of_parent ? new_size : measured_size)), 0);

    // The parent measures this layout by the size given to ItemSize(), without the inflation in the item rect.
    if (window_state->CurrentLayout)
        MarkLayoutLastItem(*window_state->CurrentLayout);

    if (layout->Parent == NULL)
    {
        BalanceChildLayouts(*layout);
        layout->SpringSizeInTree = CalculateSpringSizeInTree(*layout);
    }

    if (layout->DebugFlags != ImGuiLayoutDebugFlags_None)
        DebugDrawLayout(*layout);

    //window->DrawList->AddRect(layout->StartPos, layout->StartPos + measured_size, IM_COL32(0,255,0,255));           // [DEBUG]
    //window->DrawList->AddRect(window->DC.LastItemRect.Min, window->DC.LastItemRect.Max, IM_COL32(255,255,0,255));   // [DEBUG]
}

static ImVec2 CalculateLayoutSize(ImGuiLayout& layout, bool collapse_springs)
{
    ImVec2 bounds = ImVec2(0.0f, 0.0f);

    if (layout.Type == ImGuiLayoutType_Vertical)
    {
        for (int i = 0; i < layout.Items.Size; i++)
        {
            ImGuiLayoutItem& item = layout.Items[i];
            ImVec2 item_size = item.MeasuredBounds.GetSize();

            if (item.Type == ImGuiLayoutItemType_Item)
            {
                bounds.x  = ImMax(bounds.x, item_size.x);
                bounds.y += item_size.y;
            }
            else
            {
                bounds.y += ImFloor(item.SpringSpacing);

                if (!collapse_springs)
                    bounds.y += item.SpringSize;
            }
        }
    }
    else
    {
        for (int i = 0; i < layout.Items.Size; i++)
        {
            ImGuiLayoutItem& item = layout.Items[i];
            ImVec2 item_size = item.MeasuredBounds.GetSize();

            if (item.Type == ImGuiLayoutItemType_Item)
            {
                bounds.x += item_size.x;
                bounds.y  = ImMax(bounds.y, item_size.y);
            }
            else
            {
                bounds.x += ImFloor(item.SpringSpacing);

                if (!collapse_springs)
                    bounds.x += item.SpringSize;
            }
        }
    }

    return bounds;
}

static void PushLayout(ImGuiLayout* layout)
{
    ImGuiLayoutWindowState* window_state = GetCurrentWindowLayoutState();

    if (layout)
    {
        layout->Parent = window_state->CurrentLayout;
        if (layout->Parent != NULL)
            layout->ParentItemIndex = layout->Parent->CurrentItemIndex;
        if (window_state->CurrentLayout)
        {
            layout->NextSibling = window_state->CurrentLayout->FirstChild;
            layout->FirstChild  = NULL;
            window_state->CurrentLayout->FirstChild = layout;
        }
        else
        {
            layout->NextSibling = NULL;
            layout->FirstChild  = NULL;
        }
    }

    window_state->LayoutStack.push_back(layout);
    window_state->CurrentLayout = layout;
}

static void PopLayout(ImGuiLayout* layout)
{
    ImGuiLayoutWindowState* window_state = GetCurrentWindowLayoutState();

    IM_ASSERT(!window_state->LayoutStack.empty());
    IM_ASSERT(window_state->LayoutStack.back() == layout);
    IM_UNUSED(layout);

    window_state->LayoutStack.pop_back();
    window_state->CurrentLayout = window_state->LayoutStack.empty() ? NULL : window_state->LayoutStack.back();
}

static void BalanceLayoutSprings(ImGuiLayout& layout)
{
    // Accumulate amount of occupied space and springs weights
    float total_spring_weight = 0.0f;

    int last_spring_item_index = -1;
    for (int i = 0; i < layout.Items.Size; i++)
    {
        ImGuiLayoutItem& item = layout.Items[i];
        if (item.Type == ImGuiLayoutItemType_Spring)
        {
            total_spring_weight += item.SpringWeight;
            last_spring_item_index = i;
        }
    }

    // Determine occupied space and available space depending on layout type
    const bool  is_horizontal   = (layout.Type == ImGuiLayoutType_Horizontal);
    const bool  is_auto_sized   = ((is_horizontal ? layout.Size.x : layout.Size.y) <= 0.0f) && (layout.Parent == NULL);
    const float occupied_space  = is_horizontal ? layout.MinimumSize.x : layout.MinimumSize.y;
    const float available_space = is_auto_sized ? occupied_space : (is_horizontal ? layout.CurrentSize.x : layout.CurrentSize.y);
    const float free_space      = ImMax(available_space - occupied_space, 0.0f);

    // Springs that change size move the items after them, to avoid one frame visual bugs. Offsets accumulate, so each
    // item is translated once.
    ImVec2 offset        = ImVec2(0.0f, 0.0f);
    float span_start     = 0.0f;
    float current_weight = 0.0f;
    for (int i = 0; i < layout.Items.Size; i++)
    {
        ImGuiLayoutItem& item = layout.Items[i];
        if (item.Type != ImGuiLayoutItemType_Spring)
        {
            if (offset.x != 0.0f || offset.y != 0.0f)
            {
                TranslateLayoutItem(item, offset);
                item.MeasuredBounds.Min += offset;
                item.MeasuredBounds.Max += offset;
            }
            continue;
        }

        float last_spring_size = item.SpringSize;

        if (free_space > 0.0f && total_spring_weight > 0.0f)
        {
            float next_weight = current_weight + item.SpringWeight;
            float span_end    = ImFloor((i == last_spring_item_index) ? free_space : (free_space * next_weight / total_spring_weight));
            float spring_size = span_end - span_start;
            item.SpringSize   = spring_size;
            span_start        = span_end;
            current_weight    = next_weight;
        }
        else
        {
            item.SpringSize = 0.0f;
        }

        item.MeasuredBounds.Min += offset;
        item.MeasuredBounds.Max += offset;
        if (last_spring_size != item.SpringSize)
        {
            float difference = item.SpringSize - last_spring_size;
            ImVec2 spring_offset = is_horizontal ? ImVec2(difference, 0.0f) : ImVec2(0.0f, difference);
            item.MeasuredBounds.Max += spring_offset;
            offset += spring_offset;
        }
    }
}

static ImVec2 BalanceLayoutItemAlignment(ImGuiLayout& layout, ImGuiLayoutItem& item)
{
    // Fixup item alignment if necessary.
    ImVec2 position_correction = ImVec2(0.0f, 0.0f);
    if (item.CurrentAlign > 0.0f)
    {
        float item_align_offset = CalculateLayoutItemAlignmentOffset(layout, item);
        if (item.CurrentAlignOffset != item_align_offset)
        {
            float offset = item_align_offset - item.CurrentAlignOffset;

            if (layout.Type == ImGuiLayoutType_Horizontal)
                position_correction.y = offset;
            else
                position_correction.x = offset;

            TranslateLayoutItem(item, position_correction);

            item.CurrentAlignOffset = item_align_offset;
        }
    }

    return position_correction;
}

static void BalanceLayoutItemsAlignment(ImGuiLayout& layout)
{
    for (int i = 0; i < layout.Items.Size; ++i)
    {
        ImGuiLayoutItem& item = layout.Items[i];
        BalanceLayoutItemAlignment(layout, item);
    }
}

static bool HasAnyNonZeroSpring(ImGuiLayout& layout)
{
    for (int i = 0; i < layout.Items.Size; ++i)
    {
        ImGuiLayoutItem& item = layout.Items[i];
        if (item.Type != ImGuiLayoutItemType_Spring)
            continue;
        if (item.SpringWeight > 0)
            return true;
    }
    return false;
}

static void BalanceChildLayouts(ImGuiLayout& layout)
{
    for (ImGuiLayout* child = layout.FirstChild; child != NULL; child = child->NextSibling)
    {
        //ImVec2 child_layout_size = child->CurrentSize;

        // Propagate layout size down to child layouts.
        //
        // TODO: Distribution assume inner layout is only
        //       element inside parent item and assigns
        //       all available space to it.
        //
        //       Investigate how to split space between
        //       adjacent layouts.
        //
        //       Investigate how to measure non-layout items
        //       to treat them as fixed size blocks.
        //
        if (child->Type == ImGuiLayoutType_Horizontal && child->Size.x <= 0.0f)
            child->CurrentSize.x = layout.CurrentSize.x;
        else if (child->Type == ImGuiLayoutType_Vertical && child->Size.y <= 0.0f)
            child->CurrentSize.y = layout.CurrentSize.y;

        BalanceChildLayouts(*child);

        //child->CurrentSize = child_layout_size;

        if (HasAnyNonZeroSpring(*child))
        {
            // Expand item measured bounds to make alignment correct.
            ImGuiLayoutItem& item = layout.Items[child->ParentItemIndex];

            if (child->Type == ImGuiLayoutType_Horizontal && child->Size.x <= 0.0f)
                item.MeasuredBounds.Max.x = ImMax(item.MeasuredBounds.Max.x, item.MeasuredBounds.Min.x + layout.CurrentSize.x);
            else if (child->Type == ImGuiLayoutType_Vertical && child->Size.y <= 0.0f)
                item.MeasuredBounds.Max.y = ImMax(item.MeasuredBounds.Max.y, item.MeasuredBounds.Min.y + layout.CurrentSize.y);
        }
    }

    BalanceLayoutSprings(layout);
    BalanceLayoutItemsAlignment(layout);
}

// Only fixed-size axes clip, and only when the contents measured in the previous frame overflow them.
static void BeginLayoutClipRect(ImGuiLayout& layout, bool measured)
{
    const bool clip_x = layout.Size.x > 0.0f && (!measured || layout.MinimumSize.x > layout.Size.x);
    const bool clip_y = layout.Size.y > 0.0f && (!measured || layout.MinimumSize.y > layout.Size.y);
    layout.ClipRectPushed = clip_x || clip_y;
    if (!layout.ClipRectPushed)
        return;

    ImVec2 clip_rect_min(clip_x ? layout.StartPos.x : -FLT_MAX, clip_y ? layout.StartPos.y : -FLT_MAX);
    ImVec2 clip_rect_max(clip_x ? layout.StartPos.x + layout.Size.x : FLT_MAX, clip_y ? layout.StartPos.y + layout.Size.y : FLT_MAX);
    ImGui::PushClipRect(clip_rect_min, clip_rect_max, true);
}

static void EndLayoutClipRect(ImGuiLayout& layout)
{
    if (layout.ClipRectPushed)
        ImGui::PopClipRect();
}

// Until the root layout corrects them, items can land too far by up to the spring sizes of the tree in the previous
// frame. The clip rect is widened by that much toward the far side meanwhile, so that ItemAdd() and text rendering do
// not cull them. Not toward the near side: hover tests use it too, and items scrolled under the title bar would take
// its clicks.
static void BeginLayoutWideClipRect(ImGuiLayout& layout)
{
    layout.WideClipRectPushed = layout.SpringSizeInTree.x > 0.0f || layout.SpringSizeInTree.y > 0.0f;
    if (!layout.WideClipRectPushed)
        return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 margin = layout.SpringSizeInTree;
    layout.NarrowClipRect = draw_list->_CmdHeader.ClipRect;
    ImGui::PushClipRect(ImVec2(layout.NarrowClipRect.x, layout.NarrowClipRect.y), ImVec2(layout.NarrowClipRect.z + margin.x, layout.NarrowClipRect.w + margin.y), false);
    layout.WideClipRect = draw_list->_CmdHeader.ClipRect;
    layout.WideClipRectCmdIndex = draw_list->CmdBuffer.Size - 1;
}

// Commands drawn under the wide clip rect get the original one back, then merge where it made them split.
static void EndLayoutWideClipRect(ImGuiLayout& layout)
{
    if (!layout.WideClipRectPushed)
        return;
    ImGui::PopClipRect();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec4 wide = layout.WideClipRect;
    const ImVec4 narrow = layout.NarrowClipRect;
    for (int n = layout.WideClipRectCmdIndex; n < draw_list->CmdBuffer.Size; n++)
    {
        // Clip rects pushed without intersecting (e.g. full screen ones) are not inside the wide one: they stay.
        ImVec4& clip_rect = draw_list->CmdBuffer[n].ClipRect;
        if (clip_rect.x < wide.x || clip_rect.y < wide.y || clip_rect.z > wide.z || clip_rect.w > wide.w)
            continue;
        clip_rect.x = ImClamp(clip_rect.x, narrow.x, narrow.z);
        clip_rect.y = ImClamp(clip_rect.y, narrow.y, narrow.w);
        clip_rect.z = ImClamp(clip_rect.z, clip_rect.x, narrow.z);
        clip_rect.w = ImClamp(clip_rect.w, clip_rect.y, narrow.w);
    }

    // Same header comparison as ImDrawList::_TryMergeDrawCmds(): clip rect, texture and vertex offset.
    const size_t header_size = offsetof(ImDrawCmd, VtxOffset) + sizeof(unsigned int);
    int dst = ImMax(layout.WideClipRectCmdIndex - 1, 0);
    for (int src = dst + 1; src < draw_list->CmdBuffer.Size; src++)
    {
        ImDrawCmd& prev_cmd = draw_list->CmdBuffer[dst];
        ImDrawCmd& cmd = draw_list->CmdBuffer[src];
        if (memcmp(&prev_cmd, &cmd, header_size) == 0 && prev_cmd.IdxOffset + prev_cmd.ElemCount == cmd.IdxOffset && prev_cmd.UserCallback == NULL && cmd.UserCallback == NULL)
            prev_cmd.ElemCount += cmd.ElemCount;
        else
            draw_list->CmdBuffer[++dst] = cmd;
    }
    draw_list->CmdBuffer.resize(dst + 1);
}

static ImVec2 CalculateSpringSizeInTree(const ImGuiLayout& layout)
{
    ImVec2 size(0.0f, 0.0f);
    const int axis = (layout.Type == ImGuiLayoutType_Horizontal) ? 0 : 1;
    for (const ImGuiLayoutItem& item : layout.Items)
        if (item.Type == ImGuiLayoutItemType_Spring)
            size[axis] += item.SpringSize;
    for (const ImGuiLayout* child = layout.FirstChild; child != NULL; child = child->NextSibling)
        size += CalculateSpringSizeInTree(*child);
    return size;
}

static ImGuiLayoutItem* GenerateLayoutItem(ImGuiLayout& layout, ImGuiLayoutItemType type)
{
    IM_ASSERT(layout.CurrentItemIndex <= layout.Items.Size);

    if (layout.CurrentItemIndex < layout.Items.Size)
    {
        ImGuiLayoutItem& item = layout.Items[layout.CurrentItemIndex];
        if (item.Type != type)
            item = ImGuiLayoutItem(type);
    }
    else
    {
        layout.Items.push_back(ImGuiLayoutItem(type));
    }

    return &layout.Items[layout.CurrentItemIndex];
}

// Calculate how many pixels from top/left layout edge item need to be moved to match
// layout alignment.
static float CalculateLayoutItemAlignmentOffset(ImGuiLayout& layout, ImGuiLayoutItem& item)
{
    if (item.CurrentAlign <= 0.0f)
        return 0.0f;

    ImVec2 item_size = item.MeasuredBounds.GetSize();

    float layout_extent = (layout.Type == ImGuiLayoutType_Horizontal) ? layout.CurrentSize.y : layout.CurrentSize.x;
    float item_extent   = (layout.Type == ImGuiLayoutType_Horizontal) ? item_size.y : item_size.x;

    if (item_extent <= 0/* || layout_extent <= item_extent*/)
        return 0.0f;

    float align_offset = ImFloor(item.CurrentAlign * (layout_extent - item_extent));

    return align_offset;
}

static void TranslateLayoutItem(ImGuiLayoutItem& item, const ImVec2& offset)
{
    if ((offset.x == 0.0f && offset.y == 0.0f) || (item.VertexIndexBegin == item.VertexIndexEnd))
        return;

    //IMGUI_DEBUG_LOG("TranslateLayoutItem by %f,%f\n", offset.x, offset.y);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImDrawVert* begin = draw_list->VtxBuffer.Data + item.VertexIndexBegin;
    ImDrawVert* end   = draw_list->VtxBuffer.Data + item.VertexIndexEnd;

    for (ImDrawVert* vtx = begin; vtx < end; ++vtx)
    {
        vtx->pos.x += offset.x;
        vtx->pos.y += offset.y;
    }
}

static void SignedIndent(float indent)
{
    if (indent > 0.0f)
        ImGui::Indent(indent);
    else if (indent < 0.0f)
        ImGui::Unindent(-indent);
}

// EndLayoutItem() includes the last item rect only if an item was submitted after this.
static void MarkLayoutLastItem(ImGuiLayout& layout)
{
    ImGuiContext& g = *GImGui;
    layout.ItemBeginLastItemId = g.LastItemData.ID;
    layout.ItemBeginLastItemRect = g.LastItemData.Rect;
}

static void BeginLayoutItem(ImGuiLayout& layout)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiLayoutItem& item = *GenerateLayoutItem(layout, ImGuiLayoutItemType_Item);

    item.CurrentAlign = layout.Align;
    if (item.CurrentAlign < 0.0f)
        item.CurrentAlign = ImClamp(g.Style.LayoutAlign, 0.0f, 1.0f);

    // Align item according to data from previous frame.
    // If layout changes in current frame alignment will
    // be corrected in EndLayout() to it visualy coherent.
    item.CurrentAlignOffset = CalculateLayoutItemAlignmentOffset(layout, item);
    if (layout.Type == ImGuiLayoutType_Horizontal)
    {
        window->DC.CursorPos.y = layout.StartPos.y + item.CurrentAlignOffset;
    }
    else if (item.CurrentAlign > 0.0f)
    {
        float new_position = window->DC.CursorPos.x + item.CurrentAlignOffset;

        // Make placement behave like in horizontal case when next
        // widget is placed at very same Y position. This indent
        // make sure for vertical layout placed widgets has same X position.
        SignedIndent(item.CurrentAlignOffset);

        window->DC.CursorPos.x = new_position;
    }

    // Each item starts its own line and is measured like a group, from what its contents extend CursorMaxPos to.
    window->DC.CurrLineSize = ImVec2(0.0f, 0.0f);
    window->DC.IsSameLine = false;
    window->DC.CursorMaxPos = window->DC.CursorPos;
    MarkLayoutLastItem(layout);

    item.MeasuredBounds.Min = item.MeasuredBounds.Max = window->DC.CursorPos;
    item.VertexIndexBegin = item.VertexIndexEnd = window->DrawList->VtxBuffer.Size;
}

static void EndLayoutItem(ImGuiLayout& layout)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(layout.CurrentItemIndex < layout.Items.Size);

    ImGuiLayoutItem& item = layout.Items[layout.CurrentItemIndex];

    // Like EndGroup(), include the last item submitted inside, for e.g. EndTable() undershooting with CursorMaxPos.
    item.MeasuredBounds.Max = ImMax(item.MeasuredBounds.Max, window->DC.CursorMaxPos);
    if (g.LastItemData.ID != layout.ItemBeginLastItemId || memcmp(&g.LastItemData.Rect, &layout.ItemBeginLastItemRect, sizeof(ImRect)) != 0)
        item.MeasuredBounds.Max = ImMax(item.MeasuredBounds.Max, g.LastItemData.Rect.Max);

    ImDrawList* draw_list = window->DrawList;
    item.VertexIndexEnd = draw_list->VtxBuffer.Size;

    if (item.CurrentAlign > 0.0f && layout.Type == ImGuiLayoutType_Vertical)
        SignedIndent(-item.CurrentAlignOffset);

    // Fixup item alignment in case item size changed in current frame.
    ImVec2 position_correction = BalanceLayoutItemAlignment(layout, item);

    item.MeasuredBounds.Min += position_correction;
    item.MeasuredBounds.Max += position_correction;

    if (layout.Type == ImGuiLayoutType_Horizontal)
        window->DC.CursorPos.y = layout.StartPos.y;
    else
        window->DC.CursorPos.x = layout.StartPos.x;

    layout.CurrentItemIndex++;
}

static void AddLayoutSpring(ImGuiLayout& layout, float weight, float spacing)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    EndLayoutItem(layout);

    // Undo item padding, spring should consume all space between items.
    ImGuiLayoutItem* previous_item = &layout.Items[layout.CurrentItemIndex - 1];
    if (layout.Type == ImGuiLayoutType_Horizontal)
        window->DC.CursorPos.x = previous_item->MeasuredBounds.Max.x;
    else
        window->DC.CursorPos.y = previous_item->MeasuredBounds.Max.y;
    window->DC.IsSameLine = false;

    previous_item = NULL; // may be invalid after call to GenerateLayoutItem()

    ImGuiLayoutItem* spring_item = GenerateLayoutItem(layout, ImGuiLayoutItemType_Spring);

    spring_item->MeasuredBounds.Min = spring_item->MeasuredBounds.Max = window->DC.CursorPos;

    if (weight < 0.0f)
        weight = 0.0f;

    if (spring_item->SpringWeight != weight)
        spring_item->SpringWeight = weight;

    if (spacing < 0.0f)
    {
        ImVec2 style_spacing = g.Style.ItemSpacing;
        if (layout.Type == ImGuiLayoutType_Horizontal)
            spacing = style_spacing.x;
        else
            spacing = style_spacing.y;
    }

    if (spring_item->SpringSpacing != spacing)
        spring_item->SpringSpacing = spacing;

    // Submitted even when empty, so that GetItemRect*() and IsItem*() after Spring() refer to the spring.
    ImVec2 spring_size, spring_spacing;
    if (layout.Type == ImGuiLayoutType_Horizontal)
    {
        spring_spacing = ImVec2(0.0f, g.Style.ItemSpacing.y);
        spring_size    = ImVec2(spacing + spring_item->SpringSize, layout.CurrentSize.y);
    }
    else
    {
        spring_spacing = ImVec2(g.Style.ItemSpacing.x, 0.0f);
        spring_size    = ImVec2(layout.CurrentSize.x, spacing + spring_item->SpringSize);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImFloor(spring_spacing));
    ImGui::Dummy(ImFloor(spring_size));
    ImGui::PopStyleVar();

    spring_item->MeasuredBounds.Max = spring_item->MeasuredBounds.Min + ImFloor(spring_size);

    layout.CurrentItemIndex++;

    BeginLayoutItem(layout);
}

static void DebugDrawLayout(const ImGuiLayout& layout)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const int axis = (layout.Type == ImGuiLayoutType_Horizontal) ? 0 : 1;
    const ImRect layout_rect(layout.StartPos, layout.StartPos + layout.MeasuredSize);
    for (const ImGuiLayoutItem& item : layout.Items)
    {
        if (item.Type == ImGuiLayoutItemType_Item)
        {
            if (layout.DebugFlags & ImGuiLayoutDebugFlags_DrawItems)
                draw_list->AddRect(item.MeasuredBounds.Min, item.MeasuredBounds.Max, IM_COL32(255, 255, 0, 160));
            continue;
        }
        if ((layout.DebugFlags & ImGuiLayoutDebugFlags_DrawSprings) == 0)
            continue;

        // Along the layout: the spring, then its spacing. Across it: the whole layout.
        ImRect rect = item.MeasuredBounds;
        rect.Min[axis ^ 1] = layout_rect.Min[axis ^ 1];
        rect.Max[axis ^ 1] = layout_rect.Max[axis ^ 1];
        const float length = rect.Max[axis] - rect.Min[axis];
        if (length <= 0.0f)
            continue;
        const float spacing = ImMin(ImFloor(item.SpringSpacing), length);
        ImRect spring_rect = rect, spacing_rect = rect;
        spring_rect.Max[axis] = spacing_rect.Min[axis] = rect.Max[axis] - spacing;

        draw_list->PushClipRect(rect.Min, rect.Max, true);
        draw_list->AddRectFilled(spring_rect.Min, spring_rect.Max, IM_COL32(80, 20, 80, 255));
        draw_list->AddRectFilled(spacing_rect.Min, spacing_rect.Max, IM_COL32(80, 20, 20, 255));

        // Zig-zag along the middle of the spring.
        const float zig_zag_size = 3.0f;
        ImVec2 direction(0.0f, 0.0f);
        direction[axis] = 1.0f;
        ImVec2 normal(-direction.y, direction.x);
        ImVec2 origin = ImFloor(rect.GetCenter());
        origin[axis] = ImFloor(rect.Min[axis]);
        origin += ImVec2(0.5f, 0.5f);
        draw_list->PathLineTo(origin);
        for (float x = zig_zag_size * 0.5f; x <= length - spacing; x += zig_zag_size)
        {
            draw_list->PathLineTo(origin + direction * x + normal * zig_zag_size);
            normal = -normal;
        }
        draw_list->PathStroke(IM_COL32(255, 255, 255, 190), 1.0f);
        draw_list->PopClipRect();
    }
}


//-----------------------------------------------------------------------------
// [SECTION] Stack Layout: Internal API
//-----------------------------------------------------------------------------

void ErrorRecoveryTryToRecoverWindowState(const ImGuiErrorRecoveryState* state_in)
{
    // Called for every window, so it creates no state.
    ImGuiContext& g = *GImGui;
    ImGuiLayoutContextState* context_state = FindContextState(g);
    ImGuiLayoutWindowState* state = context_state ? (ImGuiLayoutWindowState*)context_state->WindowStates.GetVoidPtr(g.CurrentWindow->ID) : NULL;
    if (state == NULL)
        return;

    // Layouts and suspensions each push onto the ID stack: those begun after state_in was stored are above its size.
    while (state->LayoutStack.Size > 0) //-V1044
    {
        if (state->CurrentLayout == NULL)
        {
            if (state->SuspendStack.back().IDStackSize < state_in->SizeOfIDStack)
                break;
            IM_ASSERT_USER_ERROR(0, "Missing ResumeLayout()");
            ImGui::ResumeLayout();
        }
        else
        {
            if (state->CurrentLayout->IDStackSize < state_in->SizeOfIDStack)
                break;
            if (state->CurrentLayout->Type == ImGuiLayoutType_Horizontal)
            {
                IM_ASSERT_USER_ERROR(0, "Missing EndHorizontal()");
                ImGui::EndHorizontal();
            }
            else
            {
                IM_ASSERT_USER_ERROR(0, "Missing EndVertical()");
                ImGui::EndVertical();
            }
        }
    }
}


} // namespace ImGuiStackLayout

//-----------------------------------------------------------------------------
// [SECTION] Stack Layout: Public API
//-----------------------------------------------------------------------------

void ImGui::BeginHorizontal(const char* str_id, const ImVec2& size/* = ImVec2(0, 0)*/, float align/* = -1*/)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiStackLayout::BeginLayout(window->GetID(str_id), ImGuiLayoutType_Horizontal, size, align);
}

void ImGui::BeginHorizontal(const void* ptr_id, const ImVec2& size/* = ImVec2(0, 0)*/, float align/* = -1*/)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiStackLayout::BeginLayout(window->GetID(ptr_id), ImGuiLayoutType_Horizontal, size, align);
}

void ImGui::BeginHorizontal(int id, const ImVec2& size/* = ImVec2(0, 0)*/, float align/* = -1*/)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiStackLayout::BeginLayout(window->GetID(id), ImGuiLayoutType_Horizontal, size, align);
}

void ImGui::EndHorizontal()
{
    ImGuiStackLayout::ImGuiLayoutWindowState* window_state = ImGuiStackLayout::GetCurrentWindowLayoutState();
    IM_ASSERT_USER_ERROR_RET(window_state->CurrentLayout != NULL || window_state->LayoutStack.Size == 0, "Calling EndHorizontal() between SuspendLayout() and ResumeLayout()!");
    IM_ASSERT_USER_ERROR_RET(window_state->CurrentLayout != NULL, "Calling EndHorizontal() too many times!");
    IM_ASSERT_USER_ERROR(window_state->CurrentLayout->Type == ImGuiLayoutType_Horizontal, "Calling EndHorizontal() to end a vertical layout!");
    ImGuiStackLayout::EndLayout(window_state);
}

void ImGui::BeginVertical(const char* str_id, const ImVec2& size/* = ImVec2(0, 0)*/, float align/* = -1*/)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiStackLayout::BeginLayout(window->GetID(str_id), ImGuiLayoutType_Vertical, size, align);
}

void ImGui::BeginVertical(const void* ptr_id, const ImVec2& size/* = ImVec2(0, 0)*/, float align/* = -1*/)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiStackLayout::BeginLayout(window->GetID(ptr_id), ImGuiLayoutType_Vertical, size, align);
}

void ImGui::BeginVertical(int id, const ImVec2& size/* = ImVec2(0, 0)*/, float align/* = -1*/)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiStackLayout::BeginLayout(window->GetID(id), ImGuiLayoutType_Vertical, size, align);
}

void ImGui::EndVertical()
{
    ImGuiStackLayout::ImGuiLayoutWindowState* window_state = ImGuiStackLayout::GetCurrentWindowLayoutState();
    IM_ASSERT_USER_ERROR_RET(window_state->CurrentLayout != NULL || window_state->LayoutStack.Size == 0, "Calling EndVertical() between SuspendLayout() and ResumeLayout()!");
    IM_ASSERT_USER_ERROR_RET(window_state->CurrentLayout != NULL, "Calling EndVertical() too many times!");
    IM_ASSERT_USER_ERROR(window_state->CurrentLayout->Type == ImGuiLayoutType_Vertical, "Calling EndVertical() to end a horizontal layout!");
    ImGuiStackLayout::EndLayout(window_state);
}

// Inserts spring separator in layout
//      weight <= 0     : spring will always have zero size
//      weight > 0      : power of current spring
//      spacing < 0     : use default spacing if pos_x == 0, no spacing if pos_x != 0
//      spacing >= 0    : enforce spacing amount
void ImGui::Spring(float weight/* = 1.0f*/, float spacing/* = -1.0f*/)
{
    ImGuiStackLayout::ImGuiLayoutWindowState* window_state = ImGuiStackLayout::GetCurrentWindowLayoutState();
    IM_ASSERT_USER_ERROR_RET(window_state->CurrentLayout != NULL, "Calling Spring() outside of a layout, or between SuspendLayout() and ResumeLayout()!");

    ImGuiStackLayout::AddLayoutSpring(*window_state->CurrentLayout, weight, spacing);
}

void ImGui::SuspendLayout()
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiStackLayout::ImGuiLayoutWindowState* window_state = ImGuiStackLayout::GetCurrentWindowLayoutState();

    // Items in between flow as they would outside of the outermost layout, and are not measured by the layout.
    // Pushing the current ID again leaves item IDs unchanged, and orders the suspension for error recovery.
    ImGuiStackLayout::ImGuiLayoutSuspendBackup backup;
    backup.LayoutType = window->DC.LayoutType;
    backup.CursorMaxPos = window->DC.CursorMaxPos;
    backup.IDStackSize = window->IDStack.Size;
    window_state->SuspendStack.push_back(backup);
    PushOverrideID(window->IDStack.back());
    for (ImGuiStackLayout::ImGuiLayout* layout : window_state->LayoutStack)
    {
        if (layout != NULL)
        {
            window->DC.LayoutType = layout->BackupLayoutType;
            break;
        }
    }

    ImGuiStackLayout::PushLayout(NULL);
}

void ImGui::ResumeLayout()
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiStackLayout::ImGuiLayoutWindowState* window_state = ImGuiStackLayout::GetCurrentWindowLayoutState();
    IM_ASSERT_USER_ERROR_RET(window_state->SuspendStack.Size > 0, "Calling ResumeLayout() without SuspendLayout()!");
    IM_ASSERT_USER_ERROR_RET(window_state->CurrentLayout == NULL, "Missing EndHorizontal() or EndVertical() before ResumeLayout()!");
    ImGuiStackLayout::PopLayout(NULL);

    const ImGuiStackLayout::ImGuiLayoutSuspendBackup& backup = window_state->SuspendStack.back();
    IM_ASSERT_USER_ERROR(window->IDStack.Size == backup.IDStackSize + 1, "Mismatching PushID()/PopID() between SuspendLayout() and ResumeLayout()!");
    while (window->IDStack.Size > backup.IDStackSize)
        PopID();
    window->DC.LayoutType = backup.LayoutType;
    window->DC.CursorMaxPos = backup.CursorMaxPos;
    window_state->SuspendStack.pop_back();
    if (window_state->CurrentLayout)
        ImGuiStackLayout::MarkLayoutLastItem(*window_state->CurrentLayout);
}

void ImGui::SetNextLayoutDebugFlags(ImGuiLayoutDebugFlags flags)
{
    ImGuiStackLayout::GetContextState(*GImGui)->NextDebugFlags = flags;
}

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
