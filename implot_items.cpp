// MIT License

// Copyright (c) 2020 Evan Pezent

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// ImPlot v0.14

#include "implot.h"
#include "implot_internal.h"

//-----------------------------------------------------------------------------
// [SECTION] Macros and Defines
//-----------------------------------------------------------------------------

#define SQRT_1_2 0.70710678118f
#define SQRT_3_2 0.86602540378f

#ifndef IMPLOT_NO_FORCE_INLINE
    #ifdef _MSC_VER
        #define IMPLOT_INLINE __forceinline
    #elif defined(__GNUC__)
        #define IMPLOT_INLINE inline __attribute__((__always_inline__))
    #elif defined(__CLANG__)
        #if __has_attribute(__always_inline__)
            #define IMPLOT_INLINE inline __attribute__((__always_inline__))
        #else
            #define IMPLOT_INLINE inline
        #endif
    #else
        #define IMPLOT_INLINE inline
    #endif
#else
    #define IMPLOT_INLINE inline
#endif

#if defined __SSE__ || defined __x86_64__ || defined _M_X64
#ifndef IMGUI_ENABLE_SSE
#include <immintrin.h>
#endif
static IMPLOT_INLINE float  ImInvSqrt(float x) { return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x))); }
#else
static IMPLOT_INLINE float  ImInvSqrt(float x) { return 1.0f / sqrtf(x); }
#endif

#define IMPLOT_NORMALIZE2F_OVER_ZERO(VX,VY) do { float d2 = VX*VX + VY*VY; if (d2 > 0.0f) { float inv_len = ImInvSqrt(d2); VX *= inv_len; VY *= inv_len; } } while (0)

// Support for pre-1.82 versions. Users on 1.82+ can use 0 (default) flags to mean "all corners" but in order to support older versions we are more explicit.
#if (IMGUI_VERSION_NUM < 18102) && !defined(ImDrawFlags_RoundCornersAll)
#define ImDrawFlags_RoundCornersAll ImDrawCornerFlags_All
#endif

//-----------------------------------------------------------------------------
// [SECTION] Template instantiation utility
//-----------------------------------------------------------------------------

// By default, templates are instantiated for `float`, `double`, and for the following integer types, which are defined in imgui.h:
//     signed char         ImS8;   // 8-bit signed integer
//     unsigned char       ImU8;   // 8-bit unsigned integer
//     signed short        ImS16;  // 16-bit signed integer
//     unsigned short      ImU16;  // 16-bit unsigned integer
//     signed int          ImS32;  // 32-bit signed integer == int
//     unsigned int        ImU32;  // 32-bit unsigned integer
//     signed   long long  ImS64;  // 64-bit signed integer
//     unsigned long long  ImU64;  // 64-bit unsigned integer
// (note: this list does *not* include `long`, `unsigned long` and `long double`)
//
// You can customize the supported types by defining IMPLOT_CUSTOM_NUMERIC_TYPES at compile time to define your own type list. 
//    As an example, you could use the compile time define given by the line below in order to support only float and double.
//        -DIMPLOT_CUSTOM_NUMERIC_TYPES="(float)(double)"
//    In order to support all known C++ types, use:
//        -DIMPLOT_CUSTOM_NUMERIC_TYPES="(signed char)(unsigned char)(signed short)(unsigned short)(signed int)(unsigned int)(signed long)(unsigned long)(signed long long)(unsigned long long)(float)(double)(long double)"

#ifdef IMPLOT_CUSTOM_NUMERIC_TYPES
    #define IMPLOT_NUMERIC_TYPES IMPLOT_CUSTOM_NUMERIC_TYPES
#else
    #define IMPLOT_NUMERIC_TYPES (ImS8)(ImU8)(ImS16)(ImU16)(ImS32)(ImU32)(ImS64)(ImU64)(float)(double)
#endif

// CALL_INSTANTIATE_FOR_NUMERIC_TYPES will duplicate the template instantion code `INSTANTIATE_MACRO(T)` on supported types.
#define _CAT(x, y) _CAT_(x, y)
#define _CAT_(x,y) x ## y
#define _INSTANTIATE_FOR_NUMERIC_TYPES(chain) _CAT(_INSTANTIATE_FOR_NUMERIC_TYPES_1 chain, _END)
#define _INSTANTIATE_FOR_NUMERIC_TYPES_1(T) INSTANTIATE_MACRO(T); _INSTANTIATE_FOR_NUMERIC_TYPES_2
#define _INSTANTIATE_FOR_NUMERIC_TYPES_2(T) INSTANTIATE_MACRO(T); _INSTANTIATE_FOR_NUMERIC_TYPES_1
#define _INSTANTIATE_FOR_NUMERIC_TYPES_1_END
#define _INSTANTIATE_FOR_NUMERIC_TYPES_2_END
#define CALL_INSTANTIATE_FOR_NUMERIC_TYPES() _INSTANTIATE_FOR_NUMERIC_TYPES(IMPLOT_NUMERIC_TYPES);

namespace ImPlot {

//-----------------------------------------------------------------------------
// [SECTION] Utils
//-----------------------------------------------------------------------------

// Calc maximum index size of ImDrawIdx
template <typename T>
struct MaxIdx { static const unsigned int Value; };
template <> const unsigned int MaxIdx<unsigned short>::Value = 65535;
template <> const unsigned int MaxIdx<unsigned int>::Value   = 4294967295;

IMPLOT_INLINE void GetLineRenderProps(const ImDrawList& draw_list, float& half_weight, ImVec2& tex_uv0, ImVec2& tex_uv1) {
    const bool aa = ImHasFlag(draw_list.Flags, ImDrawListFlags_AntiAliasedLines) &&
                    ImHasFlag(draw_list.Flags, ImDrawListFlags_AntiAliasedLinesUseTex);
    if (aa) {
        ImVec4 tex_uvs = draw_list._Data->TexUvLines[(int)(half_weight*2)];
        tex_uv0 = ImVec2(tex_uvs.x, tex_uvs.y);
        tex_uv1 = ImVec2(tex_uvs.z, tex_uvs.w);
        half_weight += 1;
    }
    else {
        tex_uv0 = tex_uv1 = draw_list._Data->TexUvWhitePixel;
    }
}

IMPLOT_INLINE void PrimLine(ImDrawList& draw_list, const ImVec2& P1, const ImVec2& P2, float half_weight, ImU32 col, const ImVec2& tex_uv0, const ImVec2 tex_uv1) {
    float dx = P2.x - P1.x;
    float dy = P2.y - P1.y;
    IMPLOT_NORMALIZE2F_OVER_ZERO(dx, dy);
    dx *= half_weight;
    dy *= half_weight;
    draw_list._VtxWritePtr[0].pos.x = P1.x + dy;
    draw_list._VtxWritePtr[0].pos.y = P1.y - dx;
    draw_list._VtxWritePtr[0].uv    = tex_uv0;
    draw_list._VtxWritePtr[0].col   = col;
    draw_list._VtxWritePtr[1].pos.x = P2.x + dy;
    draw_list._VtxWritePtr[1].pos.y = P2.y - dx;
    draw_list._VtxWritePtr[1].uv    = tex_uv0;
    draw_list._VtxWritePtr[1].col   = col;
    draw_list._VtxWritePtr[2].pos.x = P2.x - dy;
    draw_list._VtxWritePtr[2].pos.y = P2.y + dx;
    draw_list._VtxWritePtr[2].uv    = tex_uv1;
    draw_list._VtxWritePtr[2].col   = col;
    draw_list._VtxWritePtr[3].pos.x = P1.x - dy;
    draw_list._VtxWritePtr[3].pos.y = P1.y + dx;
    draw_list._VtxWritePtr[3].uv    = tex_uv1;
    draw_list._VtxWritePtr[3].col   = col;
    draw_list._VtxWritePtr += 4;
    draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx);
    draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 1);
    draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 2);
    draw_list._IdxWritePtr[3] = (ImDrawIdx)(draw_list._VtxCurrentIdx);
    draw_list._IdxWritePtr[4] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 2);
    draw_list._IdxWritePtr[5] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 3);
    draw_list._IdxWritePtr += 6;
    draw_list._VtxCurrentIdx += 4;
}

IMPLOT_INLINE void PrimRectFill(ImDrawList& draw_list, const ImVec2& Pmin, const ImVec2& Pmax, ImU32 col, const ImVec2& uv) {
    draw_list._VtxWritePtr[0].pos   = Pmin;
    draw_list._VtxWritePtr[0].uv    = uv;
    draw_list._VtxWritePtr[0].col   = col;
    draw_list._VtxWritePtr[1].pos   = Pmax;
    draw_list._VtxWritePtr[1].uv    = uv;
    draw_list._VtxWritePtr[1].col   = col;
    draw_list._VtxWritePtr[2].pos.x = Pmin.x;
    draw_list._VtxWritePtr[2].pos.y = Pmax.y;
    draw_list._VtxWritePtr[2].uv    = uv;
    draw_list._VtxWritePtr[2].col   = col;
    draw_list._VtxWritePtr[3].pos.x = Pmax.x;
    draw_list._VtxWritePtr[3].pos.y = Pmin.y;
    draw_list._VtxWritePtr[3].uv    = uv;
    draw_list._VtxWritePtr[3].col   = col;
    draw_list._VtxWritePtr += 4;
    draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx);
    draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 1);
    draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 2);
    draw_list._IdxWritePtr[3] = (ImDrawIdx)(draw_list._VtxCurrentIdx);
    draw_list._IdxWritePtr[4] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 1);
    draw_list._IdxWritePtr[5] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 3);
    draw_list._IdxWritePtr += 6;
    draw_list._VtxCurrentIdx += 4;
}

IMPLOT_INLINE void PrimRectLine(ImDrawList& draw_list, const ImVec2& Pmin, const ImVec2& Pmax, float weight, ImU32 col, const ImVec2& uv) {

    draw_list._VtxWritePtr[0].pos.x = Pmin.x;
    draw_list._VtxWritePtr[0].pos.y = Pmin.y;
    draw_list._VtxWritePtr[0].uv    = uv;
    draw_list._VtxWritePtr[0].col   = col;

    draw_list._VtxWritePtr[1].pos.x = Pmin.x;
    draw_list._VtxWritePtr[1].pos.y = Pmax.y;
    draw_list._VtxWritePtr[1].uv    = uv;
    draw_list._VtxWritePtr[1].col   = col;

    draw_list._VtxWritePtr[2].pos.x = Pmax.x;
    draw_list._VtxWritePtr[2].pos.y = Pmax.y;
    draw_list._VtxWritePtr[2].uv    = uv;
    draw_list._VtxWritePtr[2].col   = col;

    draw_list._VtxWritePtr[3].pos.x = Pmax.x;
    draw_list._VtxWritePtr[3].pos.y = Pmin.y;
    draw_list._VtxWritePtr[3].uv    = uv;
    draw_list._VtxWritePtr[3].col   = col;

    draw_list._VtxWritePtr[4].pos.x = Pmin.x + weight;
    draw_list._VtxWritePtr[4].pos.y = Pmin.y + weight;
    draw_list._VtxWritePtr[4].uv    = uv;
    draw_list._VtxWritePtr[4].col   = col;

    draw_list._VtxWritePtr[5].pos.x = Pmin.x + weight;
    draw_list._VtxWritePtr[5].pos.y = Pmax.y - weight;
    draw_list._VtxWritePtr[5].uv    = uv;
    draw_list._VtxWritePtr[5].col   = col;

    draw_list._VtxWritePtr[6].pos.x = Pmax.x - weight;
    draw_list._VtxWritePtr[6].pos.y = Pmax.y - weight;
    draw_list._VtxWritePtr[6].uv    = uv;
    draw_list._VtxWritePtr[6].col   = col;

    draw_list._VtxWritePtr[7].pos.x = Pmax.x - weight;
    draw_list._VtxWritePtr[7].pos.y = Pmin.y + weight;
    draw_list._VtxWritePtr[7].uv    = uv;
    draw_list._VtxWritePtr[7].col   = col;

    draw_list._VtxWritePtr += 8;

    draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 0);
    draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 1);
    draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 5);
    draw_list._IdxWritePtr += 3;

    draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 0);
    draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 5);
    draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 4);
    draw_list._IdxWritePtr += 3;

    draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 1);
    draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 2);
    draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 6);
    draw_list._IdxWritePtr += 3;

    draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 1);
    draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 6);
    draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 5);
    draw_list._IdxWritePtr += 3;

    draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 2);
    draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 3);
    draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 7);
    draw_list._IdxWritePtr += 3;

    draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 2);
    draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 7);
    draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 6);
    draw_list._IdxWritePtr += 3;

    draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 3);
    draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 0);
    draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 4);
    draw_list._IdxWritePtr += 3;

    draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 3);
    draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 4);
    draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 7);
    draw_list._IdxWritePtr += 3;

    draw_list._VtxCurrentIdx += 8;
}


//-----------------------------------------------------------------------------
// [SECTION] Item Utils
//-----------------------------------------------------------------------------

ImPlotItem* RegisterOrGetItem(const char* label_id, ImPlotItemFlags flags, bool* just_created) {
    ImPlotContext& gp = *GImPlot;
    ImPlotItemGroup& Items = *gp.CurrentItems;
    ImGuiID id = Items.GetItemID(label_id);
    if (just_created != NULL)
        *just_created = Items.GetItem(id) == NULL;
    ImPlotItem* item = Items.GetOrAddItem(id);
    if (item->SeenThisFrame)
        return item;
    item->SeenThisFrame = true;
    int idx = Items.GetItemIndex(item);
    item->ID = id;
    if (!ImHasFlag(flags, ImPlotItemFlags_NoLegend) && ImGui::FindRenderedTextEnd(label_id, NULL) != label_id) {
        Items.Legend.Indices.push_back(idx);
        item->NameOffset = Items.Legend.Labels.size();
        Items.Legend.Labels.append(label_id, label_id + strlen(label_id) + 1);
    }
    else {
        item->Show = true;
    }
    return item;
}

ImPlotItem* GetItem(const char* label_id) {
    ImPlotContext& gp = *GImPlot;
    return gp.CurrentItems->GetItem(label_id);
}

bool IsItemHidden(const char* label_id) {
    ImPlotItem* item = GetItem(label_id);
    return item != NULL && !item->Show;
}

ImPlotItem* GetCurrentItem() {
    ImPlotContext& gp = *GImPlot;
    return gp.CurrentItem;
}

void SetNextLineStyle(const ImVec4& col, float weight) {
    ImPlotContext& gp = *GImPlot;
    gp.NextItemData.Colors[ImPlotCol_Line] = col;
    gp.NextItemData.LineWeight             = weight;
}

void SetNextFillStyle(const ImVec4& col, float alpha) {
    ImPlotContext& gp = *GImPlot;
    gp.NextItemData.Colors[ImPlotCol_Fill] = col;
    gp.NextItemData.FillAlpha              = alpha;
}

void SetNextMarkerStyle(ImPlotMarker marker, float size, const ImVec4& fill, float weight, const ImVec4& outline) {
    ImPlotContext& gp = *GImPlot;
    gp.NextItemData.Marker                          = marker;
    gp.NextItemData.Colors[ImPlotCol_MarkerFill]    = fill;
    gp.NextItemData.MarkerSize                      = size;
    gp.NextItemData.Colors[ImPlotCol_MarkerOutline] = outline;
    gp.NextItemData.MarkerWeight                    = weight;
}

void SetNextErrorBarStyle(const ImVec4& col, float size, float weight) {
    ImPlotContext& gp = *GImPlot;
    gp.NextItemData.Colors[ImPlotCol_ErrorBar] = col;
    gp.NextItemData.ErrorBarSize               = size;
    gp.NextItemData.ErrorBarWeight             = weight;
}

ImVec4 GetLastItemColor() {
    ImPlotContext& gp = *GImPlot;
    if (gp.PreviousItem)
        return ImGui::ColorConvertU32ToFloat4(gp.PreviousItem->Color);
    return ImVec4();
}

void BustItemCache() {
    ImPlotContext& gp = *GImPlot;
    for (int p = 0; p < gp.Plots.GetBufSize(); ++p) {
        ImPlotPlot& plot = *gp.Plots.GetByIndex(p);
        plot.Items.Reset();
    }
    for (int p = 0; p < gp.Subplots.GetBufSize(); ++p) {
        ImPlotSubplot& subplot = *gp.Subplots.GetByIndex(p);
        subplot.Items.Reset();
    }
}

void BustColorCache(const char* plot_title_id) {
    ImPlotContext& gp = *GImPlot;
    if (plot_title_id == NULL) {
        BustItemCache();
    }
    else {
        ImGuiID id = ImGui::GetCurrentWindow()->GetID(plot_title_id);
        ImPlotPlot* plot = gp.Plots.GetByKey(id);
        if (plot != NULL)
            plot->Items.Reset();
        else {
            ImPlotSubplot* subplot = gp.Subplots.GetByKey(id);
            if (subplot != NULL)
                subplot->Items.Reset();
        }
    }
}

//-----------------------------------------------------------------------------
// [SECTION] BeginItem / EndItem
//-----------------------------------------------------------------------------

static const float ITEM_HIGHLIGHT_LINE_SCALE = 2.0f;
static const float ITEM_HIGHLIGHT_MARK_SCALE = 1.25f;

// Begins a new item. Returns false if the item should not be plotted.
bool BeginItem(const char* label_id, ImPlotItemFlags flags, ImPlotCol recolor_from) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "PlotX() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    bool just_created;
    ImPlotItem* item = RegisterOrGetItem(label_id, flags, &just_created);
    // set current item
    gp.CurrentItem = item;
    ImPlotNextItemData& s = gp.NextItemData;
    // set/override item color
    if (recolor_from != -1) {
        if (!IsColorAuto(s.Colors[recolor_from]))
            item->Color = ImGui::ColorConvertFloat4ToU32(s.Colors[recolor_from]);
        else if (!IsColorAuto(gp.Style.Colors[recolor_from]))
            item->Color = ImGui::ColorConvertFloat4ToU32(gp.Style.Colors[recolor_from]);
        else if (just_created)
            item->Color = NextColormapColorU32();
    }
    else if (just_created) {
        item->Color = NextColormapColorU32();
    }
    // hide/show item
    if (gp.NextItemData.HasHidden) {
        if (just_created || gp.NextItemData.HiddenCond == ImGuiCond_Always)
            item->Show = !gp.NextItemData.Hidden;
    }
    if (!item->Show) {
        // reset next item data
        gp.NextItemData.Reset();
        gp.PreviousItem = item;
        gp.CurrentItem  = NULL;
        return false;
    }
    else {
        ImVec4 item_color = ImGui::ColorConvertU32ToFloat4(item->Color);
        // stage next item colors
        s.Colors[ImPlotCol_Line]           = IsColorAuto(s.Colors[ImPlotCol_Line])          ? ( IsColorAuto(ImPlotCol_Line)           ? item_color                 : gp.Style.Colors[ImPlotCol_Line]          ) : s.Colors[ImPlotCol_Line];
        s.Colors[ImPlotCol_Fill]           = IsColorAuto(s.Colors[ImPlotCol_Fill])          ? ( IsColorAuto(ImPlotCol_Fill)           ? item_color                 : gp.Style.Colors[ImPlotCol_Fill]          ) : s.Colors[ImPlotCol_Fill];
        s.Colors[ImPlotCol_MarkerOutline]  = IsColorAuto(s.Colors[ImPlotCol_MarkerOutline]) ? ( IsColorAuto(ImPlotCol_MarkerOutline)  ? s.Colors[ImPlotCol_Line]   : gp.Style.Colors[ImPlotCol_MarkerOutline] ) : s.Colors[ImPlotCol_MarkerOutline];
        s.Colors[ImPlotCol_MarkerFill]     = IsColorAuto(s.Colors[ImPlotCol_MarkerFill])    ? ( IsColorAuto(ImPlotCol_MarkerFill)     ? s.Colors[ImPlotCol_Line]   : gp.Style.Colors[ImPlotCol_MarkerFill]    ) : s.Colors[ImPlotCol_MarkerFill];
        s.Colors[ImPlotCol_ErrorBar]       = IsColorAuto(s.Colors[ImPlotCol_ErrorBar])      ? ( GetStyleColorVec4(ImPlotCol_ErrorBar)                                                                         ) : s.Colors[ImPlotCol_ErrorBar];
        // stage next item style vars
        s.LineWeight         = s.LineWeight       < 0 ? gp.Style.LineWeight       : s.LineWeight;
        s.Marker             = s.Marker           < 0 ? gp.Style.Marker           : s.Marker;
        s.MarkerSize         = s.MarkerSize       < 0 ? gp.Style.MarkerSize       : s.MarkerSize;
        s.MarkerWeight       = s.MarkerWeight     < 0 ? gp.Style.MarkerWeight     : s.MarkerWeight;
        s.FillAlpha          = s.FillAlpha        < 0 ? gp.Style.FillAlpha        : s.FillAlpha;
        s.ErrorBarSize       = s.ErrorBarSize     < 0 ? gp.Style.ErrorBarSize     : s.ErrorBarSize;
        s.ErrorBarWeight     = s.ErrorBarWeight   < 0 ? gp.Style.ErrorBarWeight   : s.ErrorBarWeight;
        s.DigitalBitHeight   = s.DigitalBitHeight < 0 ? gp.Style.DigitalBitHeight : s.DigitalBitHeight;
        s.DigitalBitGap      = s.DigitalBitGap    < 0 ? gp.Style.DigitalBitGap    : s.DigitalBitGap;
        // apply alpha modifier(s)
        s.Colors[ImPlotCol_Fill].w       *= s.FillAlpha;
        s.Colors[ImPlotCol_MarkerFill].w *= s.FillAlpha; // TODO: this should be separate, if it at all
        // apply highlight mods
        if (item->LegendHovered) {
            if (!ImHasFlag(gp.CurrentItems->Legend.Flags, ImPlotLegendFlags_NoHighlightItem)) {
                s.LineWeight   *= ITEM_HIGHLIGHT_LINE_SCALE;
                s.MarkerSize   *= ITEM_HIGHLIGHT_MARK_SCALE;
                s.MarkerWeight *= ITEM_HIGHLIGHT_LINE_SCALE;
                // TODO: how to highlight fills?
            }
            if (!ImHasFlag(gp.CurrentItems->Legend.Flags, ImPlotLegendFlags_NoHighlightAxis)) {
                if (gp.CurrentPlot->EnabledAxesX() > 1)
                    gp.CurrentPlot->Axes[gp.CurrentPlot->CurrentX].ColorHiLi = item->Color;
                if (gp.CurrentPlot->EnabledAxesY() > 1)
                    gp.CurrentPlot->Axes[gp.CurrentPlot->CurrentY].ColorHiLi = item->Color;
            }
        }
        // set render flags
        s.RenderLine       = s.Colors[ImPlotCol_Line].w          > 0 && s.LineWeight > 0;
        s.RenderFill       = s.Colors[ImPlotCol_Fill].w          > 0;
        s.RenderMarkerFill = s.Colors[ImPlotCol_MarkerFill].w    > 0;
        s.RenderMarkerLine = s.Colors[ImPlotCol_MarkerOutline].w > 0 && s.MarkerWeight > 0;
        // push rendering clip rect
        PushPlotClipRect();
        return true;
    }
}

// Ends an item (call only if BeginItem returns true)
void EndItem() {
    ImPlotContext& gp = *GImPlot;
    // pop rendering clip rect
    PopPlotClipRect();
    // reset next item data
    gp.NextItemData.Reset();
    // set current item
    gp.PreviousItem = gp.CurrentItem;
    gp.CurrentItem  = NULL;
}

//-----------------------------------------------------------------------------
// [SECTION] Indexers
//-----------------------------------------------------------------------------

template <typename T>
IMPLOT_INLINE T IndexData(const T* data, int idx, int count, int offset, int stride) {
    const int s = ((offset == 0) << 0) | ((stride == sizeof(T)) << 1);
    switch (s) {
        case 3 : return data[idx];
        case 2 : return data[(offset + idx) % count];
        case 1 : return *(const T*)(const void*)((const unsigned char*)data + (size_t)((idx) ) * stride);
        case 0 : return *(const T*)(const void*)((const unsigned char*)data + (size_t)((offset + idx) % count) * stride);
        default: return T(0);
    }
}

template <typename T>
struct IndexerIdx {
    IndexerIdx(const T* data, int count, int offset = 0, int stride = sizeof(T)) :
        Data(data),
        Count(count),
        Offset(count ? ImPosMod(offset, count) : 0),
        Stride(stride)
    { }
    template <typename I> IMPLOT_INLINE double operator()(I idx) const {
        return (double)IndexData(Data, idx, Count, Offset, Stride);
    }
    const T* Data;
    int Count;
    int Offset;
    int Stride;
};

template <typename _Indexer1, typename _Indexer2>
struct IndexerAdd {
    IndexerAdd(const _Indexer1& indexer1, const _Indexer2& indexer2, double scale1 = 1, double scale2 = 1)
        : Indexer1(indexer1),
          Indexer2(indexer2),
          Scale1(scale1),
          Scale2(scale2),
          Count(ImMin(Indexer1.Count, Indexer2.Count))
    { }
    template <typename I> IMPLOT_INLINE double operator()(I idx) const {
        return Scale1 * Indexer1(idx) + Scale2 * Indexer2(idx);
    }
    const _Indexer1& Indexer1;
    const _Indexer2& Indexer2;
    double Scale1;
    double Scale2;
    int Count;
};

struct IndexerLin {
    IndexerLin(double m, double b) : M(m), B(b) { }
    template <typename I> IMPLOT_INLINE double operator()(I idx) const {
        return M * idx + B;
    }
    const double M;
    const double B;
};

struct IndexerConst {
    IndexerConst(double ref) : Ref(ref) { }
    template <typename I> IMPLOT_INLINE double operator()(I) const { return Ref; }
    const double Ref;
};

//-----------------------------------------------------------------------------
// [SECTION] Getters
//-----------------------------------------------------------------------------

template <typename _IndexerX, typename _IndexerY>
struct GetterXY {
    GetterXY(_IndexerX x, _IndexerY y, int count) : IndxerX(x), IndxerY(y), Count(count) { }
    template <typename I> IMPLOT_INLINE ImPlotPoint operator()(I idx) const {
        return ImPlotPoint(IndxerX(idx),IndxerY(idx));
    }
    const _IndexerX IndxerX;
    const _IndexerY IndxerY;
    const int Count;
};

/// Interprets a user's function pointer as ImPlotPoints
struct GetterFuncPtr {
    GetterFuncPtr(ImPlotGetter getter, void* data, int count) :
        Getter(getter),
        Data(data),
        Count(count)
    { }
    template <typename I> IMPLOT_INLINE ImPlotPoint operator()(I idx) const {
        return Getter(idx, Data);
    }
    ImPlotGetter Getter;
    void* const Data;
    const int Count;
};

template <typename _Getter>
struct GetterOverrideX {
    GetterOverrideX(_Getter getter, double x) : Getter(getter), X(x), Count(getter.Count) { }
    template <typename I> IMPLOT_INLINE ImPlotPoint operator()(I idx) const {
        ImPlotPoint p = Getter(idx);
        p.x = X;
        return p;
    }
    const _Getter Getter;
    const double X;
    const int Count;
};

template <typename _Getter>
struct GetterOverrideY {
    GetterOverrideY(_Getter getter, double y) : Getter(getter), Y(y), Count(getter.Count) { }
    template <typename I> IMPLOT_INLINE ImPlotPoint operator()(I idx) const {
        ImPlotPoint p = Getter(idx);
        p.y = Y;
        return p;
    }
    const _Getter Getter;
    const double Y;
    const int Count;
};

template <typename _Getter>
struct GetterLoop {
    GetterLoop(_Getter getter) : Getter(getter), Count(getter.Count + 1) { }
    template <typename I> IMPLOT_INLINE ImPlotPoint operator()(I idx) const {
        idx = idx % (Count - 1);
        return Getter(idx);
    }
    const _Getter Getter;
    const int Count;
};

template <typename T>
struct GetterError {
    GetterError(const T* xs, const T* ys, const T* neg, const T* pos, int count, int offset, int stride) :
        Xs(xs),
        Ys(ys),
        Neg(neg),
        Pos(pos),
        Count(count),
        Offset(count ? ImPosMod(offset, count) : 0),
        Stride(stride)
    { }
    template <typename I> IMPLOT_INLINE ImPlotPointError operator()(I idx) const {
        return ImPlotPointError((double)IndexData(Xs,  idx, Count, Offset, Stride),
                                (double)IndexData(Ys,  idx, Count, Offset, Stride),
                                (double)IndexData(Neg, idx, Count, Offset, Stride),
                                (double)IndexData(Pos, idx, Count, Offset, Stride));
    }
    const T* const Xs;
    const T* const Ys;
    const T* const Neg;
    const T* const Pos;
    const int Count;
    const int Offset;
    const int Stride;
};

//-----------------------------------------------------------------------------
// [SECTION] Fitters
//-----------------------------------------------------------------------------

template <typename _Getter1>
struct Fitter1 {
    Fitter1(const _Getter1& getter) : Getter(getter) { }
    void Fit(ImPlotAxis& x_axis, ImPlotAxis& y_axis) const {
        for (int i = 0; i < Getter.Count; ++i) {
            ImPlotPoint p = Getter(i);
            x_axis.ExtendFitWith(y_axis, p.x, p.y);
            y_axis.ExtendFitWith(x_axis, p.y, p.x);
        }
    }
    const _Getter1& Getter;
};

template <typename _Getter1>
struct FitterX {
    FitterX(const _Getter1& getter) : Getter(getter) { }
    void Fit(ImPlotAxis& x_axis, ImPlotAxis&) const {
        for (int i = 0; i < Getter.Count; ++i) {
            ImPlotPoint p = Getter(i);
            x_axis.ExtendFit(p.x);
        }
    }
    const _Getter1& Getter;
};

template <typename _Getter1>
struct FitterY {
    FitterY(const _Getter1& getter) : Getter(getter) { }
    void Fit(ImPlotAxis&, ImPlotAxis& y_axis) const {
        for (int i = 0; i < Getter.Count; ++i) {
            ImPlotPoint p = Getter(i);
            y_axis.ExtendFit(p.y);
        }
    }
    const _Getter1& Getter;
};

template <typename _Getter1, typename _Getter2>
struct Fitter2 {
    Fitter2(const _Getter1& getter1, const _Getter2& getter2) : Getter1(getter1), Getter2(getter2) { }
    void Fit(ImPlotAxis& x_axis, ImPlotAxis& y_axis) const {
        for (int i = 0; i < Getter1.Count; ++i) {
            ImPlotPoint p = Getter1(i);
            x_axis.ExtendFitWith(y_axis, p.x, p.y);
            y_axis.ExtendFitWith(x_axis, p.y, p.x);
        }
        for (int i = 0; i < Getter2.Count; ++i) {
            ImPlotPoint p = Getter2(i);
            x_axis.ExtendFitWith(y_axis, p.x, p.y);
            y_axis.ExtendFitWith(x_axis, p.y, p.x);
        }
    }
    const _Getter1& Getter1;
    const _Getter2& Getter2;
};

template <typename _Getter1, typename _Getter2>
struct FitterBarV {
    FitterBarV(const _Getter1& getter1, const _Getter2& getter2, double width) :
        Getter1(getter1),
        Getter2(getter2),
        HalfWidth(width*0.5)
    { }
    void Fit(ImPlotAxis& x_axis, ImPlotAxis& y_axis) const {
        int count = ImMin(Getter1.Count, Getter2.Count);
        for (int i = 0; i < count; ++i) {
            ImPlotPoint p1 = Getter1(i); p1.x -= HalfWidth;
            ImPlotPoint p2 = Getter2(i); p2.x += HalfWidth;
            x_axis.ExtendFitWith(y_axis, p1.x, p1.y);
            y_axis.ExtendFitWith(x_axis, p1.y, p1.x);
            x_axis.ExtendFitWith(y_axis, p2.x, p2.y);
            y_axis.ExtendFitWith(x_axis, p2.y, p2.x);
        }
    }
    const _Getter1& Getter1;
    const _Getter2& Getter2;
    const double    HalfWidth;
};

template <typename _Getter1, typename _Getter2>
struct FitterBarH {
    FitterBarH(const _Getter1& getter1, const _Getter2& getter2, double height) :
        Getter1(getter1),
        Getter2(getter2),
        HalfHeight(height*0.5)
    { }
    void Fit(ImPlotAxis& x_axis, ImPlotAxis& y_axis) const {
        int count = ImMin(Getter1.Count, Getter2.Count);
        for (int i = 0; i < count; ++i) {
            ImPlotPoint p1 = Getter1(i); p1.y -= HalfHeight;
            ImPlotPoint p2 = Getter2(i); p2.y += HalfHeight;
            x_axis.ExtendFitWith(y_axis, p1.x, p1.y);
            y_axis.ExtendFitWith(x_axis, p1.y, p1.x);
            x_axis.ExtendFitWith(y_axis, p2.x, p2.y);
            y_axis.ExtendFitWith(x_axis, p2.y, p2.x);
        }
    }
    const _Getter1& Getter1;
    const _Getter2& Getter2;
    const double    HalfHeight;
};

struct FitterRect {
    FitterRect(const ImPlotPoint& pmin, const ImPlotPoint& pmax) :
        Pmin(pmin),
        Pmax(pmax)
    { }
    FitterRect(const ImPlotRect& rect) :
        FitterRect(rect.Min(), rect.Max())
    { }
    void Fit(ImPlotAxis& x_axis, ImPlotAxis& y_axis) const {
        x_axis.ExtendFitWith(y_axis, Pmin.x, Pmin.y);
        y_axis.ExtendFitWith(x_axis, Pmin.y, Pmin.x);
        x_axis.ExtendFitWith(y_axis, Pmax.x, Pmax.y);
        y_axis.ExtendFitWith(x_axis, Pmax.y, Pmax.x);
    }
    const ImPlotPoint Pmin;
    const ImPlotPoint Pmax;
};

//-----------------------------------------------------------------------------
// [SECTION] Transformers
//-----------------------------------------------------------------------------

struct Transformer1 {
    Transformer1(double pixMin, double pltMin, double pltMax, double m, double scaMin, double scaMax, ImPlotTransform fwd, void* data) :
        ScaMin(scaMin),
        ScaMax(scaMax),
        PltMin(pltMin),
        PltMax(pltMax),
        PixMin(pixMin),
        M(m),
        TransformFwd(fwd),
        TransformData(data)
    { }

    template <typename T> IMPLOT_INLINE float operator()(T p) const {
        if (TransformFwd != NULL) {
            double s = TransformFwd(p, TransformData);
            double t = (s - ScaMin) / (ScaMax - ScaMin);
            p = PltMin + (PltMax - PltMin) * t;
        }
        return (float)(PixMin + M * (p - PltMin));
    }

    double ScaMin, ScaMax, PltMin, PltMax, PixMin, M;
    ImPlotTransform TransformFwd;
    void*           TransformData;
};

struct Transformer2 {
    Transformer2(const ImPlotAxis& x_axis, const ImPlotAxis& y_axis) :
        Tx(x_axis.PixelMin,
           x_axis.Range.Min,
           x_axis.Range.Max,
           x_axis.ScaleToPixel,
           x_axis.ScaleMin,
           x_axis.ScaleMax,
           x_axis.TransformForward,
           x_axis.TransformData),
        Ty(y_axis.PixelMin,
           y_axis.Range.Min,
           y_axis.Range.Max,
           y_axis.ScaleToPixel,
           y_axis.ScaleMin,
           y_axis.ScaleMax,
           y_axis.TransformForward,
           y_axis.TransformData)
    { }

    Transformer2(const ImPlotPlot& plot) :
        Transformer2(plot.Axes[plot.CurrentX], plot.Axes[plot.CurrentY])
    { }

    Transformer2() :
        Transformer2(*GImPlot->CurrentPlot)
    { }

    template <typename P> IMPLOT_INLINE ImVec2 operator()(const P& plt) const {
        ImVec2 out;
        out.x = Tx(plt.x);
        out.y = Ty(plt.y);
        return out;
    }

    template <typename T> IMPLOT_INLINE ImVec2 operator()(T x, T y) const {
        ImVec2 out;
        out.x = Tx(x);
        out.y = Ty(y);
        return out;
    }

    Transformer1 Tx;
    Transformer1 Ty;
};

//-----------------------------------------------------------------------------
// [SECTION] Renderers
//-----------------------------------------------------------------------------

struct RendererBase {
    RendererBase(int prims, int idx_consumed, int vtx_consumed) :
        Prims(prims),
        IdxConsumed(idx_consumed),
        VtxConsumed(vtx_consumed)
    { }
    const int Prims;
    Transformer2 Transformer;
    const int IdxConsumed;
    const int VtxConsumed;
};

template <class _Getter>
struct RendererLineStrip : RendererBase {
    RendererLineStrip(const _Getter& getter, ImU32 col, float weight) :
        RendererBase(getter.Count - 1, 6, 4),
        Getter(getter),
        Col(col),
        HalfWeight(ImMax(1.0f,weight)*0.5f)
    {
        P1 = this->Transformer(Getter(0));
    }
    void Init(ImDrawList& draw_list) const {
        GetLineRenderProps(draw_list, HalfWeight, UV0, UV1);
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 P2 = this->Transformer(Getter(prim + 1));
        if (!cull_rect.Overlaps(ImRect(ImMin(P1, P2), ImMax(P1, P2)))) {
            P1 = P2;
            return false;
        }
        PrimLine(draw_list,P1,P2,HalfWeight,Col,UV0,UV1);
        P1 = P2;
        return true;
    }
    const _Getter& Getter;
    const ImU32 Col;
    mutable float HalfWeight;
    mutable ImVec2 P1;
    mutable ImVec2 UV0;
    mutable ImVec2 UV1;
};

template <class _Getter>
struct RendererLineStripSkip : RendererBase {
    RendererLineStripSkip(const _Getter& getter, ImU32 col, float weight) :
        RendererBase(getter.Count - 1, 6, 4),
        Getter(getter),
        Col(col),
        HalfWeight(ImMax(1.0f,weight)*0.5f)
    {
        P1 = this->Transformer(Getter(0));
    }
    void Init(ImDrawList& draw_list) const {
        GetLineRenderProps(draw_list, HalfWeight, UV0, UV1);
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 P2 = this->Transformer(Getter(prim + 1));
        if (!cull_rect.Overlaps(ImRect(ImMin(P1, P2), ImMax(P1, P2)))) {
            if (!ImNan(P2.x) && !ImNan(P2.y))
                P1 = P2;
            return false;
        }
        PrimLine(draw_list,P1,P2,HalfWeight,Col,UV0,UV1);
        if (!ImNan(P2.x) && !ImNan(P2.y))
            P1 = P2;
        return true;
    }
    const _Getter& Getter;
    const ImU32 Col;
    mutable float HalfWeight;
    mutable ImVec2 P1;
    mutable ImVec2 UV0;
    mutable ImVec2 UV1;
};

template <class _Getter>
struct RendererLineSegments1 : RendererBase {
    RendererLineSegments1(const _Getter& getter, ImU32 col, float weight) :
        RendererBase(getter.Count / 2, 6, 4),
        Getter(getter),
        Col(col),
        HalfWeight(ImMax(1.0f,weight)*0.5f)
    { }
    void Init(ImDrawList& draw_list) const {
        GetLineRenderProps(draw_list, HalfWeight, UV0, UV1);
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 P1 = this->Transformer(Getter(prim*2+0));
        ImVec2 P2 = this->Transformer(Getter(prim*2+1));
        if (!cull_rect.Overlaps(ImRect(ImMin(P1, P2), ImMax(P1, P2))))
            return false;
        PrimLine(draw_list,P1,P2,HalfWeight,Col,UV0,UV1);
        return true;
    }
    const _Getter& Getter;
    const ImU32 Col;
    mutable float HalfWeight;
    mutable ImVec2 UV0;
    mutable ImVec2 UV1;
};

template <class _Getter1, class _Getter2>
struct RendererLineSegments2 : RendererBase {
    RendererLineSegments2(const _Getter1& getter1, const _Getter2& getter2, ImU32 col, float weight) :
        RendererBase(ImMin(getter1.Count, getter1.Count), 6, 4),
        Getter1(getter1),
        Getter2(getter2),
        Col(col),
        HalfWeight(ImMax(1.0f,weight)*0.5f)
    {}
    void Init(ImDrawList& draw_list) const {
        GetLineRenderProps(draw_list, HalfWeight, UV0, UV1);
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 P1 = this->Transformer(Getter1(prim));
        ImVec2 P2 = this->Transformer(Getter2(prim));
        if (!cull_rect.Overlaps(ImRect(ImMin(P1, P2), ImMax(P1, P2))))
            return false;
        PrimLine(draw_list,P1,P2,HalfWeight,Col,UV0,UV1);
        return true;
    }
    const _Getter1& Getter1;
    const _Getter2& Getter2;
    const ImU32 Col;
    mutable float HalfWeight;
    mutable ImVec2 UV0;
    mutable ImVec2 UV1;
};

template <class _Getter1, class _Getter2>
struct RendererBarsFillV : RendererBase {
    RendererBarsFillV(const _Getter1& getter1, const _Getter2& getter2, ImU32 col, double width) :
        RendererBase(ImMin(getter1.Count, getter1.Count), 6, 4),
        Getter1(getter1),
        Getter2(getter2),
        Col(col),
        HalfWidth(width/2)
    {}
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImPlotPoint p1 = Getter1(prim);
        ImPlotPoint p2 = Getter2(prim);
        p1.x += HalfWidth;
        p2.x -= HalfWidth;
        ImVec2 P1 = this->Transformer(p1);
        ImVec2 P2 = this->Transformer(p2);
        float width_px = ImAbs(P1.x-P2.x);
        if (width_px < 1.0f) {
            P1.x += P1.x > P2.x ? (1-width_px) / 2 : (width_px-1) / 2;
            P2.x += P2.x > P1.x ? (1-width_px) / 2 : (width_px-1) / 2;
        }
        ImVec2 PMin = ImMin(P1, P2);
        ImVec2 PMax = ImMax(P1, P2);
        if (!cull_rect.Overlaps(ImRect(PMin, PMax)))
            return false;
        PrimRectFill(draw_list,PMin,PMax,Col,UV);
        return true;
    }
    const _Getter1& Getter1;
    const _Getter2& Getter2;
    const ImU32 Col;
    const double HalfWidth;
    mutable ImVec2 UV;
};

template <class _Getter1, class _Getter2>
struct RendererBarsFillH : RendererBase {
    RendererBarsFillH(const _Getter1& getter1, const _Getter2& getter2, ImU32 col, double height) :
        RendererBase(ImMin(getter1.Count, getter1.Count), 6, 4),
        Getter1(getter1),
        Getter2(getter2),
        Col(col),
        HalfHeight(height/2)
    {}
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImPlotPoint p1 = Getter1(prim);
        ImPlotPoint p2 = Getter2(prim);
        p1.y += HalfHeight;
        p2.y -= HalfHeight;
        ImVec2 P1 = this->Transformer(p1);
        ImVec2 P2 = this->Transformer(p2);
        float height_px = ImAbs(P1.y-P2.y);
        if (height_px < 1.0f) {
            P1.y += P1.y > P2.y ? (1-height_px) / 2 : (height_px-1) / 2;
            P2.y += P2.y > P1.y ? (1-height_px) / 2 : (height_px-1) / 2;
        }
        ImVec2 PMin = ImMin(P1, P2);
        ImVec2 PMax = ImMax(P1, P2);
        if (!cull_rect.Overlaps(ImRect(PMin, PMax)))
            return false;
        PrimRectFill(draw_list,PMin,PMax,Col,UV);
        return true;
    }
    const _Getter1& Getter1;
    const _Getter2& Getter2;
    const ImU32 Col;
    const double HalfHeight;
    mutable ImVec2 UV;
};

template <class _Getter1, class _Getter2>
struct RendererBarsLineV : RendererBase {
    RendererBarsLineV(const _Getter1& getter1, const _Getter2& getter2, ImU32 col, double width, float weight) :
        RendererBase(ImMin(getter1.Count, getter1.Count), 24, 8),
        Getter1(getter1),
        Getter2(getter2),
        Col(col),
        HalfWidth(width/2),
        Weight(weight)
    {}
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImPlotPoint p1 = Getter1(prim);
        ImPlotPoint p2 = Getter2(prim);
        p1.x += HalfWidth;
        p2.x -= HalfWidth;
        ImVec2 P1 = this->Transformer(p1);
        ImVec2 P2 = this->Transformer(p2);
        float width_px = ImAbs(P1.x-P2.x);
        if (width_px < 1.0f) {
            P1.x += P1.x > P2.x ? (1-width_px) / 2 : (width_px-1) / 2;
            P2.x += P2.x > P1.x ? (1-width_px) / 2 : (width_px-1) / 2;
        }
        ImVec2 PMin = ImMin(P1, P2);
        ImVec2 PMax = ImMax(P1, P2);
        if (!cull_rect.Overlaps(ImRect(PMin, PMax)))
            return false;
        PrimRectLine(draw_list,PMin,PMax,Weight,Col,UV);
        return true;
    }
    const _Getter1& Getter1;
    const _Getter2& Getter2;
    const ImU32 Col;
    const double HalfWidth;
    const float Weight;
    mutable ImVec2 UV;
};

template <class _Getter1, class _Getter2>
struct RendererBarsLineH : RendererBase {
    RendererBarsLineH(const _Getter1& getter1, const _Getter2& getter2, ImU32 col, double height, float weight) :
        RendererBase(ImMin(getter1.Count, getter1.Count), 24, 8),
        Getter1(getter1),
        Getter2(getter2),
        Col(col),
        HalfHeight(height/2),
        Weight(weight)
    {}
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImPlotPoint p1 = Getter1(prim);
        ImPlotPoint p2 = Getter2(prim);
        p1.y += HalfHeight;
        p2.y -= HalfHeight;
        ImVec2 P1 = this->Transformer(p1);
        ImVec2 P2 = this->Transformer(p2);
        float height_px = ImAbs(P1.y-P2.y);
        if (height_px < 1.0f) {
            P1.y += P1.y > P2.y ? (1-height_px) / 2 : (height_px-1) / 2;
            P2.y += P2.y > P1.y ? (1-height_px) / 2 : (height_px-1) / 2;
        }
        ImVec2 PMin = ImMin(P1, P2);
        ImVec2 PMax = ImMax(P1, P2);
        if (!cull_rect.Overlaps(ImRect(PMin, PMax)))
            return false;
        PrimRectLine(draw_list,PMin,PMax,Weight,Col,UV);
        return true;
    }
    const _Getter1& Getter1;
    const _Getter2& Getter2;
    const ImU32 Col;
    const double HalfHeight;
    const float Weight;
    mutable ImVec2 UV;
};


template <class _Getter>
struct RendererStairsPre : RendererBase {
    RendererStairsPre(const _Getter& getter, ImU32 col, float weight) :
        RendererBase(getter.Count - 1, 12, 8),
        Getter(getter),
        Col(col),
        HalfWeight(ImMax(1.0f,weight)*0.5f)
    {
        P1 = this->Transformer(Getter(0));
    }
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 P2 = this->Transformer(Getter(prim + 1));
        if (!cull_rect.Overlaps(ImRect(ImMin(P1, P2), ImMax(P1, P2)))) {
            P1 = P2;
            return false;
        }
        PrimRectFill(draw_list, ImVec2(P1.x - HalfWeight, P1.y), ImVec2(P1.x + HalfWeight, P2.y), Col, UV);
        PrimRectFill(draw_list, ImVec2(P1.x, P2.y + HalfWeight), ImVec2(P2.x, P2.y - HalfWeight), Col, UV);
        P1 = P2;
        return true;
    }
    const _Getter& Getter;
    const ImU32 Col;
    mutable float HalfWeight;
    mutable ImVec2 P1;
    mutable ImVec2 UV;
};

template <class _Getter>
struct RendererStairsPost : RendererBase {
    RendererStairsPost(const _Getter& getter, ImU32 col, float weight) :
        RendererBase(getter.Count - 1, 12, 8),
        Getter(getter),
        Col(col),
        HalfWeight(ImMax(1.0f,weight) * 0.5f)
    {
        P1 = this->Transformer(Getter(0));
    }
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 P2 = this->Transformer(Getter(prim + 1));
        if (!cull_rect.Overlaps(ImRect(ImMin(P1, P2), ImMax(P1, P2)))) {
            P1 = P2;
            return false;
        }
        PrimRectFill(draw_list, ImVec2(P1.x, P1.y + HalfWeight), ImVec2(P2.x, P1.y - HalfWeight), Col, UV);
        PrimRectFill(draw_list, ImVec2(P2.x - HalfWeight, P2.y), ImVec2(P2.x + HalfWeight, P1.y), Col, UV);
        P1 = P2;
        return true;
    }
    const _Getter& Getter;
    const ImU32 Col;
    mutable float HalfWeight;
    mutable ImVec2 P1;
    mutable ImVec2 UV;
};

template <class _Getter>
struct RendererStairsPreShaded : RendererBase {
    RendererStairsPreShaded(const _Getter& getter, ImU32 col) :
        RendererBase(getter.Count - 1, 6, 4),
        Getter(getter),
        Col(col)
    {
        P1 = this->Transformer(Getter(0));
        Y0 = this->Transformer(ImPlotPoint(0,0)).y;
    }
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 P2 = this->Transformer(Getter(prim + 1));
        ImVec2 PMin(ImMin(P1.x, P2.x), ImMin(Y0, P2.y));
        ImVec2 PMax(ImMax(P1.x, P2.x), ImMax(Y0, P2.y));
        if (!cull_rect.Overlaps(ImRect(PMin, PMax))) {
            P1 = P2;
            return false;
        }
        PrimRectFill(draw_list, PMin, PMax, Col, UV);
        P1 = P2;
        return true;
    }
    const _Getter& Getter;
    const ImU32 Col;
    float Y0;
    mutable ImVec2 P1;
    mutable ImVec2 UV;
};

template <class _Getter>
struct RendererStairsPostShaded : RendererBase {
    RendererStairsPostShaded(const _Getter& getter, ImU32 col) :
        RendererBase(getter.Count - 1, 6, 4),
        Getter(getter),
        Col(col)
    {
        P1 = this->Transformer(Getter(0));
        Y0 = this->Transformer(ImPlotPoint(0,0)).y;
    }
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 P2 = this->Transformer(Getter(prim + 1));
        ImVec2 PMin(ImMin(P1.x, P2.x), ImMin(P1.y, Y0));
        ImVec2 PMax(ImMax(P1.x, P2.x), ImMax(P1.y, Y0));
        if (!cull_rect.Overlaps(ImRect(PMin, PMax))) {
            P1 = P2;
            return false;
        }
        PrimRectFill(draw_list, PMin, PMax, Col, UV);
        P1 = P2;
        return true;
    }
    const _Getter& Getter;
    const ImU32 Col;
    float Y0;
    mutable ImVec2 P1;
    mutable ImVec2 UV;
};



template <class _Getter1, class _Getter2>
struct RendererShaded : RendererBase {
    RendererShaded(const _Getter1& getter1, const _Getter2& getter2, ImU32 col) :
        RendererBase(ImMin(getter1.Count, getter2.Count) - 1, 6, 5),
        Getter1(getter1),
        Getter2(getter2),
        Col(col)
    {
        P11 = this->Transformer(Getter1(0));
        P12 = this->Transformer(Getter2(0));
    }
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 P21 = this->Transformer(Getter1(prim+1));
        ImVec2 P22 = this->Transformer(Getter2(prim+1));
        ImRect rect(ImMin(ImMin(ImMin(P11,P12),P21),P22), ImMax(ImMax(ImMax(P11,P12),P21),P22));
        if (!cull_rect.Overlaps(rect)) {
            P11 = P21;
            P12 = P22;
            return false;
        }
        const int intersect = (P11.y > P12.y && P22.y > P21.y) || (P12.y > P11.y && P21.y > P22.y);
        ImVec2 intersection = Intersection(P11,P21,P12,P22);
        draw_list._VtxWritePtr[0].pos = P11;
        draw_list._VtxWritePtr[0].uv  = UV;
        draw_list._VtxWritePtr[0].col = Col;
        draw_list._VtxWritePtr[1].pos = P21;
        draw_list._VtxWritePtr[1].uv  = UV;
        draw_list._VtxWritePtr[1].col = Col;
        draw_list._VtxWritePtr[2].pos = intersection;
        draw_list._VtxWritePtr[2].uv  = UV;
        draw_list._VtxWritePtr[2].col = Col;
        draw_list._VtxWritePtr[3].pos = P12;
        draw_list._VtxWritePtr[3].uv  = UV;
        draw_list._VtxWritePtr[3].col = Col;
        draw_list._VtxWritePtr[4].pos = P22;
        draw_list._VtxWritePtr[4].uv  = UV;
        draw_list._VtxWritePtr[4].col = Col;
        draw_list._VtxWritePtr += 5;
        draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx);
        draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 1 + intersect);
        draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 3);
        draw_list._IdxWritePtr[3] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 1);
        draw_list._IdxWritePtr[4] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 4);
        draw_list._IdxWritePtr[5] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 3 - intersect);
        draw_list._IdxWritePtr += 6;
        draw_list._VtxCurrentIdx += 5;
        P11 = P21;
        P12 = P22;
        return true;
    }
    const _Getter1& Getter1;
    const _Getter2& Getter2;
    const ImU32 Col;
    mutable ImVec2 P11;
    mutable ImVec2 P12;
    mutable ImVec2 UV;
};

struct RectC {
    ImPlotPoint Pos;
    ImPlotPoint HalfSize;
    ImU32 Color;
};

template <typename _Getter>
struct RendererRectC : RendererBase {
    RendererRectC(const _Getter& getter) :
        RendererBase(getter.Count, 6, 4),
        Getter(getter)
    {}
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        RectC rect = Getter(prim);
        ImVec2 P1 = this->Transformer(rect.Pos.x - rect.HalfSize.x , rect.Pos.y - rect.HalfSize.y);
        ImVec2 P2 = this->Transformer(rect.Pos.x + rect.HalfSize.x , rect.Pos.y + rect.HalfSize.y);
        if ((rect.Color & IM_COL32_A_MASK) == 0 || !cull_rect.Overlaps(ImRect(ImMin(P1, P2), ImMax(P1, P2))))
            return false;
        PrimRectFill(draw_list,P1,P2,rect.Color,UV);
        return true;
    }
    const _Getter& Getter;
    mutable ImVec2 UV;
};

//-----------------------------------------------------------------------------
// [SECTION] RenderPrimitives
//-----------------------------------------------------------------------------

/// Renders primitive shapes in bulk as efficiently as possible.
template <class _Renderer>
void RenderPrimitivesEx(const _Renderer& renderer, ImDrawList& draw_list, const ImRect& cull_rect) {
    unsigned int prims        = renderer.Prims;
    unsigned int prims_culled = 0;
    unsigned int idx          = 0;
    renderer.Init(draw_list);
    while (prims) {
        // find how many can be reserved up to end of current draw command's limit
        unsigned int cnt = ImMin(prims, (MaxIdx<ImDrawIdx>::Value - draw_list._VtxCurrentIdx) / renderer.VtxConsumed);
        // make sure at least this many elements can be rendered to avoid situations where at the end of buffer this slow path is not taken all the time
        if (cnt >= ImMin(64u, prims)) {
            if (prims_culled >= cnt)
                prims_culled -= cnt; // reuse previous reservation
            else {
                // add more elements to previous reservation
                draw_list.PrimReserve((cnt - prims_culled) * renderer.IdxConsumed, (cnt - prims_culled) * renderer.VtxConsumed);
                prims_culled = 0;
            }
        }
        else
        {
            if (prims_culled > 0) {
                draw_list.PrimUnreserve(prims_culled * renderer.IdxConsumed, prims_culled * renderer.VtxConsumed);
                prims_culled = 0;
            }
            cnt = ImMin(prims, (MaxIdx<ImDrawIdx>::Value - 0/*draw_list._VtxCurrentIdx*/) / renderer.VtxConsumed);
            // reserve new draw command
            draw_list.PrimReserve(cnt * renderer.IdxConsumed, cnt * renderer.VtxConsumed);
        }
        prims -= cnt;
        for (unsigned int ie = idx + cnt; idx != ie; ++idx) {
            if (!renderer.Render(draw_list, cull_rect, idx))
                prims_culled++;
        }
    }
    if (prims_culled > 0)
        draw_list.PrimUnreserve(prims_culled * renderer.IdxConsumed, prims_culled * renderer.VtxConsumed);
}

template <template <class> class _Renderer, class _Getter, typename ...Args>
void RenderPrimitives1(const _Getter& getter, Args... args) {
    ImDrawList& draw_list = *GetPlotDrawList();
    const ImRect& cull_rect = GetCurrentPlot()->PlotRect;
    RenderPrimitivesEx(_Renderer<_Getter>(getter,args...), draw_list, cull_rect);
}

template <template <class,class> class _Renderer, class _Getter1, class _Getter2, typename ...Args>
void RenderPrimitives2(const _Getter1& getter1, const _Getter2& getter2, Args... args) {
    ImDrawList& draw_list = *GetPlotDrawList();
    const ImRect& cull_rect = GetCurrentPlot()->PlotRect;
    RenderPrimitivesEx(_Renderer<_Getter1,_Getter2>(getter1,getter2,args...), draw_list, cull_rect);
}

//-----------------------------------------------------------------------------
// [SECTION] Markers
//-----------------------------------------------------------------------------

template <class _Getter>
struct RendererMarkersFill : RendererBase {
    RendererMarkersFill(const _Getter& getter, const ImVec2* marker, int count, float size, ImU32 col) :
        RendererBase(getter.Count, (count-2)*3, count),
        Getter(getter),
        Marker(marker),
        Count(count),
        Size(size),
        Col(col)
    { }
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 p = this->Transformer(Getter(prim));
        if (p.x >= cull_rect.Min.x && p.y >= cull_rect.Min.y && p.x <= cull_rect.Max.x && p.y <= cull_rect.Max.y) {
            for (int i = 0; i < Count; i++) {
                draw_list._VtxWritePtr[0].pos.x = p.x + Marker[i].x * Size;
                draw_list._VtxWritePtr[0].pos.y = p.y + Marker[i].y * Size;
                draw_list._VtxWritePtr[0].uv = UV;
                draw_list._VtxWritePtr[0].col = Col;
                draw_list._VtxWritePtr++;
            }
            for (int i = 2; i < Count; i++) {
                draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx);
                draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + i - 1);
                draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + i);
                draw_list._IdxWritePtr += 3;
            }
            draw_list._VtxCurrentIdx += (ImDrawIdx)Count;
            return true;
        }
        return false;
    }
    const _Getter& Getter;
    const ImVec2* Marker;
    const int Count;
    const float Size;
    const ImU32 Col;
    mutable ImVec2 UV;
};


template <class _Getter>
struct RendererMarkersLine : RendererBase {
    RendererMarkersLine(const _Getter& getter, const ImVec2* marker, int count, float size, float weight, ImU32 col) :
        RendererBase(getter.Count, count/2*6, count/2*4),
        Getter(getter),
        Marker(marker),
        Count(count),
        HalfWeight(ImMax(1.0f,weight)*0.5f),
        Size(size),
        Col(col)
    { }
    void Init(ImDrawList& draw_list) const {
        GetLineRenderProps(draw_list, HalfWeight, UV0, UV1);
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 p = this->Transformer(Getter(prim));
        if (p.x >= cull_rect.Min.x && p.y >= cull_rect.Min.y && p.x <= cull_rect.Max.x && p.y <= cull_rect.Max.y) {
            for (int i = 0; i < Count; i = i + 2) {
                ImVec2 p1(p.x + Marker[i].x * Size, p.y + Marker[i].y * Size);
                ImVec2 p2(p.x + Marker[i+1].x * Size, p.y + Marker[i+1].y * Size);
                PrimLine(draw_list, p1, p2, HalfWeight, Col, UV0, UV1);
            }
            return true;
        }
        return false;
    }
    const _Getter& Getter;
    const ImVec2* Marker;
    const int Count;
    mutable float HalfWeight;
    const float Size;
    const ImU32 Col;
    mutable ImVec2 UV0;
    mutable ImVec2 UV1;
};

static const ImVec2 MARKER_FILL_CIRCLE[10]  = {ImVec2(1.0f, 0.0f), ImVec2(0.809017f, 0.58778524f),ImVec2(0.30901697f, 0.95105654f),ImVec2(-0.30901703f, 0.9510565f),ImVec2(-0.80901706f, 0.5877852f),ImVec2(-1.0f, 0.0f),ImVec2(-0.80901694f, -0.58778536f),ImVec2(-0.3090171f, -0.9510565f),ImVec2(0.30901712f, -0.9510565f),ImVec2(0.80901694f, -0.5877853f)};
static const ImVec2 MARKER_FILL_SQUARE[4]   = {ImVec2(SQRT_1_2,SQRT_1_2), ImVec2(SQRT_1_2,-SQRT_1_2), ImVec2(-SQRT_1_2,-SQRT_1_2), ImVec2(-SQRT_1_2,SQRT_1_2)};
static const ImVec2 MARKER_FILL_DIAMOND[4]  = {ImVec2(1, 0), ImVec2(0, -1), ImVec2(-1, 0), ImVec2(0, 1)};
static const ImVec2 MARKER_FILL_UP[3]       = {ImVec2(SQRT_3_2,0.5f),ImVec2(0,-1),ImVec2(-SQRT_3_2,0.5f)};
static const ImVec2 MARKER_FILL_DOWN[3]     = {ImVec2(SQRT_3_2,-0.5f),ImVec2(0,1),ImVec2(-SQRT_3_2,-0.5f)};
static const ImVec2 MARKER_FILL_LEFT[3]     = {ImVec2(-1,0), ImVec2(0.5, SQRT_3_2), ImVec2(0.5, -SQRT_3_2)};
static const ImVec2 MARKER_FILL_RIGHT[3]    = {ImVec2(1,0), ImVec2(-0.5, SQRT_3_2), ImVec2(-0.5, -SQRT_3_2)};

static const ImVec2 MARKER_LINE_CIRCLE[20]  = {
    ImVec2(1.0f, 0.0f),
    ImVec2(0.809017f, 0.58778524f),
    ImVec2(0.809017f, 0.58778524f),
    ImVec2(0.30901697f, 0.95105654f),
    ImVec2(0.30901697f, 0.95105654f),
    ImVec2(-0.30901703f, 0.9510565f),
    ImVec2(-0.30901703f, 0.9510565f),
    ImVec2(-0.80901706f, 0.5877852f),
    ImVec2(-0.80901706f, 0.5877852f),
    ImVec2(-1.0f, 0.0f),
    ImVec2(-1.0f, 0.0f),
    ImVec2(-0.80901694f, -0.58778536f),
    ImVec2(-0.80901694f, -0.58778536f),
    ImVec2(-0.3090171f, -0.9510565f),
    ImVec2(-0.3090171f, -0.9510565f),
    ImVec2(0.30901712f, -0.9510565f),
    ImVec2(0.30901712f, -0.9510565f),
    ImVec2(0.80901694f, -0.5877853f),
    ImVec2(0.80901694f, -0.5877853f),
    ImVec2(1.0f, 0.0f)
};
static const ImVec2 MARKER_LINE_SQUARE[8]   = {ImVec2(SQRT_1_2,SQRT_1_2), ImVec2(SQRT_1_2,-SQRT_1_2), ImVec2(SQRT_1_2,-SQRT_1_2), ImVec2(-SQRT_1_2,-SQRT_1_2), ImVec2(-SQRT_1_2,-SQRT_1_2), ImVec2(-SQRT_1_2,SQRT_1_2), ImVec2(-SQRT_1_2,SQRT_1_2), ImVec2(SQRT_1_2,SQRT_1_2)};
static const ImVec2 MARKER_LINE_DIAMOND[8]  = {ImVec2(1, 0), ImVec2(0, -1), ImVec2(0, -1), ImVec2(-1, 0), ImVec2(-1, 0), ImVec2(0, 1), ImVec2(0, 1), ImVec2(1, 0)};
static const ImVec2 MARKER_LINE_UP[6]       = {ImVec2(SQRT_3_2,0.5f), ImVec2(0,-1),ImVec2(0,-1),ImVec2(-SQRT_3_2,0.5f),ImVec2(-SQRT_3_2,0.5f),ImVec2(SQRT_3_2,0.5f)};
static const ImVec2 MARKER_LINE_DOWN[6]     = {ImVec2(SQRT_3_2,-0.5f),ImVec2(0,1),ImVec2(0,1),ImVec2(-SQRT_3_2,-0.5f), ImVec2(-SQRT_3_2,-0.5f), ImVec2(SQRT_3_2,-0.5f)};
static const ImVec2 MARKER_LINE_LEFT[6]     = {ImVec2(-1,0), ImVec2(0.5, SQRT_3_2),  ImVec2(0.5, SQRT_3_2),  ImVec2(0.5, -SQRT_3_2) , ImVec2(0.5, -SQRT_3_2) , ImVec2(-1,0) };
static const ImVec2 MARKER_LINE_RIGHT[6]    = {ImVec2(1,0),  ImVec2(-0.5, SQRT_3_2), ImVec2(-0.5, SQRT_3_2), ImVec2(-0.5, -SQRT_3_2), ImVec2(-0.5, -SQRT_3_2), ImVec2(1,0) };
static const ImVec2 MARKER_LINE_ASTERISK[6] = {ImVec2(-SQRT_3_2, -0.5f), ImVec2(SQRT_3_2, 0.5f),  ImVec2(-SQRT_3_2, 0.5f), ImVec2(SQRT_3_2, -0.5f), ImVec2(0, -1), ImVec2(0, 1)};
static const ImVec2 MARKER_LINE_PLUS[4]     = {ImVec2(-1, 0), ImVec2(1, 0), ImVec2(0, -1), ImVec2(0, 1)};
static const ImVec2 MARKER_LINE_CROSS[4]    = {ImVec2(-SQRT_1_2,-SQRT_1_2),ImVec2(SQRT_1_2,SQRT_1_2),ImVec2(SQRT_1_2,-SQRT_1_2),ImVec2(-SQRT_1_2,SQRT_1_2)};

template <typename _Getter>
void RenderMarkers(const _Getter& getter, ImPlotMarker marker, float size, bool rend_fill, ImU32 col_fill, bool rend_line, ImU32 col_line, float weight) {
    if (rend_fill) {
        switch (marker) {
            case ImPlotMarker_Circle  : RenderPrimitives1<RendererMarkersFill>(getter,MARKER_FILL_CIRCLE,10,size,col_fill); break;
            case ImPlotMarker_Square  : RenderPrimitives1<RendererMarkersFill>(getter,MARKER_FILL_SQUARE, 4,size,col_fill); break;
            case ImPlotMarker_Diamond : RenderPrimitives1<RendererMarkersFill>(getter,MARKER_FILL_DIAMOND,4,size,col_fill); break;
            case ImPlotMarker_Up      : RenderPrimitives1<RendererMarkersFill>(getter,MARKER_FILL_UP,     3,size,col_fill); break;
            case ImPlotMarker_Down    : RenderPrimitives1<RendererMarkersFill>(getter,MARKER_FILL_DOWN,   3,size,col_fill); break;
            case ImPlotMarker_Left    : RenderPrimitives1<RendererMarkersFill>(getter,MARKER_FILL_LEFT,   3,size,col_fill); break;
            case ImPlotMarker_Right   : RenderPrimitives1<RendererMarkersFill>(getter,MARKER_FILL_RIGHT,  3,size,col_fill); break;
        }
    }
    if (rend_line) {
        switch (marker) {
            case ImPlotMarker_Circle    : RenderPrimitives1<RendererMarkersLine>(getter,MARKER_LINE_CIRCLE, 20,size,weight,col_line); break;
            case ImPlotMarker_Square    : RenderPrimitives1<RendererMarkersLine>(getter,MARKER_LINE_SQUARE,  8,size,weight,col_line); break;
            case ImPlotMarker_Diamond   : RenderPrimitives1<RendererMarkersLine>(getter,MARKER_LINE_DIAMOND, 8,size,weight,col_line); break;
            case ImPlotMarker_Up        : RenderPrimitives1<RendererMarkersLine>(getter,MARKER_LINE_UP,      6,size,weight,col_line); break;
            case ImPlotMarker_Down      : RenderPrimitives1<RendererMarkersLine>(getter,MARKER_LINE_DOWN,    6,size,weight,col_line); break;
            case ImPlotMarker_Left      : RenderPrimitives1<RendererMarkersLine>(getter,MARKER_LINE_LEFT,    6,size,weight,col_line); break;
            case ImPlotMarker_Right     : RenderPrimitives1<RendererMarkersLine>(getter,MARKER_LINE_RIGHT,   6,size,weight,col_line); break;
            case ImPlotMarker_Asterisk  : RenderPrimitives1<RendererMarkersLine>(getter,MARKER_LINE_ASTERISK,6,size,weight,col_line); break;
            case ImPlotMarker_Plus      : RenderPrimitives1<RendererMarkersLine>(getter,MARKER_LINE_PLUS,    4,size,weight,col_line); break;
            case ImPlotMarker_Cross     : RenderPrimitives1<RendererMarkersLine>(getter,MARKER_LINE_CROSS,   4,size,weight,col_line); break;
        }
    }
}

//-----------------------------------------------------------------------------
// [SECTION] PlotLine
//-----------------------------------------------------------------------------

template <typename _Getter>
void PlotLineEx(const char* label_id, const _Getter& getter, ImPlotLineFlags flags) {
    if (BeginItemEx(label_id, Fitter1<_Getter>(getter), flags, ImPlotCol_Line)) {
        const ImPlotNextItemData& s = GetItemData();
        if (getter.Count > 1) {
            if (ImHasFlag(flags, ImPlotLineFlags_Shaded) && s.RenderFill) {
                const ImU32 col_fill = ImGui::GetColorU32(s.Colors[ImPlotCol_Fill]);
                GetterOverrideY<_Getter> getter2(getter, 0);
                RenderPrimitives2<RendererShaded>(getter,getter2,col_fill);
            }
            if (s.RenderLine) {
                const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_Line]);
                if (ImHasFlag(flags,ImPlotLineFlags_Segments)) {
                    RenderPrimitives1<RendererLineSegments1>(getter,col_line,s.LineWeight);
                }
                else if (ImHasFlag(flags, ImPlotLineFlags_Loop)) {
                    if (ImHasFlag(flags, ImPlotLineFlags_SkipNaN))
                        RenderPrimitives1<RendererLineStripSkip>(GetterLoop<_Getter>(getter),col_line,s.LineWeight);
                    else
                        RenderPrimitives1<RendererLineStrip>(GetterLoop<_Getter>(getter),col_line,s.LineWeight);
                }
                else {
                    if (ImHasFlag(flags, ImPlotLineFlags_SkipNaN))
                        RenderPrimitives1<RendererLineStripSkip>(getter,col_line,s.LineWeight);
                    else
                        RenderPrimitives1<RendererLineStrip>(getter,col_line,s.LineWeight);
                }
            }
        }
        // render markers
        if (s.Marker != ImPlotMarker_None) {
            if (ImHasFlag(flags, ImPlotLineFlags_NoClip)) {
                PopPlotClipRect();
                PushPlotClipRect(s.MarkerSize);
            }
            const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_MarkerOutline]);
            const ImU32 col_fill = ImGui::GetColorU32(s.Colors[ImPlotCol_MarkerFill]);
            RenderMarkers<_Getter>(getter, s.Marker, s.MarkerSize, s.RenderMarkerFill, col_fill, s.RenderMarkerLine, col_line, s.MarkerWeight);
        }
        EndItem();
    }
}

template <typename T>
void PlotLine(const char* label_id, const T* values, int count, double xscale, double x0, ImPlotLineFlags flags, int offset, int stride) {
    GetterXY<IndexerLin,IndexerIdx<T>> getter(IndexerLin(xscale,x0),IndexerIdx<T>(values,count,offset,stride),count);
    PlotLineEx(label_id, getter, flags);
}

template <typename T>
void PlotLine(const char* label_id, const T* xs, const T* ys, int count, ImPlotLineFlags flags, int offset, int stride) {
    GetterXY<IndexerIdx<T>,IndexerIdx<T>> getter(IndexerIdx<T>(xs,count,offset,stride),IndexerIdx<T>(ys,count,offset,stride),count);
    PlotLineEx(label_id, getter, flags);
}

#define INSTANTIATE_MACRO(T) \
    template IMPLOT_API void PlotLine<T> (const char* label_id, const T* values, int count, double xscale, double x0, ImPlotLineFlags flags, int offset, int stride); \
    template IMPLOT_API void PlotLine<T>(const char* label_id, const T* xs, const T* ys, int count, ImPlotLineFlags flags, int offset, int stride);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

// custom
void PlotLineG(const char* label_id, ImPlotGetter getter_func, void* data, int count, ImPlotLineFlags flags) {
    GetterFuncPtr getter(getter_func,data, count);
    PlotLineEx(label_id, getter, flags);
}

//-----------------------------------------------------------------------------
// [SECTION] PlotScatter
//-----------------------------------------------------------------------------

template <typename Getter>
void PlotScatterEx(const char* label_id, const Getter& getter, ImPlotScatterFlags flags) {
    if (BeginItemEx(label_id, Fitter1<Getter>(getter), flags, ImPlotCol_MarkerOutline)) {
        const ImPlotNextItemData& s = GetItemData();
        ImPlotMarker marker = s.Marker == ImPlotMarker_None ? ImPlotMarker_Circle: s.Marker;
        if (marker != ImPlotMarker_None) {
            if (ImHasFlag(flags,ImPlotScatterFlags_NoClip)) {
                PopPlotClipRect();
                PushPlotClipRect(s.MarkerSize);
            }
            const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_MarkerOutline]);
            const ImU32 col_fill = ImGui::GetColorU32(s.Colors[ImPlotCol_MarkerFill]);
            RenderMarkers<Getter>(getter, marker, s.MarkerSize, s.RenderMarkerFill, col_fill, s.RenderMarkerLine, col_line, s.MarkerWeight);
        }
        EndItem();
    }
}

template <typename T>
void PlotScatter(const char* label_id, const T* values, int count, double xscale, double x0, ImPlotScatterFlags flags, int offset, int stride) {
    GetterXY<IndexerLin,IndexerIdx<T>> getter(IndexerLin(xscale,x0),IndexerIdx<T>(values,count,offset,stride),count);
    PlotScatterEx(label_id, getter, flags);
}

template <typename T>
void PlotScatter(const char* label_id, const T* xs, const T* ys, int count, ImPlotScatterFlags flags, int offset, int stride) {
    GetterXY<IndexerIdx<T>,IndexerIdx<T>> getter(IndexerIdx<T>(xs,count,offset,stride),IndexerIdx<T>(ys,count,offset,stride),count);
    return PlotScatterEx(label_id, getter, flags);
}

#define INSTANTIATE_MACRO(T) \
    template IMPLOT_API void PlotScatter<T>(const char* label_id, const T* values, int count, double xscale, double x0, ImPlotScatterFlags flags, int offset, int stride); \
    template IMPLOT_API void PlotScatter<T>(const char* label_id, const T* xs, const T* ys, int count, ImPlotScatterFlags flags, int offset, int stride);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

// custom
void PlotScatterG(const char* label_id, ImPlotGetter getter_func, void* data, int count, ImPlotScatterFlags flags) {
    GetterFuncPtr getter(getter_func,data, count);
    return PlotScatterEx(label_id, getter, flags);
}

//-----------------------------------------------------------------------------
// [SECTION] PlotStairs
//-----------------------------------------------------------------------------

template <typename Getter>
void PlotStairsEx(const char* label_id, const Getter& getter, ImPlotStairsFlags flags) {
    if (BeginItemEx(label_id, Fitter1<Getter>(getter), flags, ImPlotCol_Line)) {
        const ImPlotNextItemData& s = GetItemData();
        if (getter.Count > 1 ) {
            if (s.RenderFill && ImHasFlag(flags,ImPlotStairsFlags_Shaded)) {
                const ImU32 col_fill = ImGui::GetColorU32(s.Colors[ImPlotCol_Fill]);
                if (ImHasFlag(flags, ImPlotStairsFlags_PreStep))
                    RenderPrimitives1<RendererStairsPreShaded>(getter,col_fill);
                else
                    RenderPrimitives1<RendererStairsPostShaded>(getter,col_fill);
            }
            if (s.RenderLine) {
                const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_Line]);
                if (ImHasFlag(flags, ImPlotStairsFlags_PreStep))
                    RenderPrimitives1<RendererStairsPre>(getter,col_line,s.LineWeight);
                else
                    RenderPrimitives1<RendererStairsPost>(getter,col_line,s.LineWeight);
            }
        }
        // render markers
        if (s.Marker != ImPlotMarker_None) {
            PopPlotClipRect();
            PushPlotClipRect(s.MarkerSize);
            const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_MarkerOutline]);
            const ImU32 col_fill = ImGui::GetColorU32(s.Colors[ImPlotCol_MarkerFill]);
            RenderMarkers<Getter>(getter, s.Marker, s.MarkerSize, s.RenderMarkerFill, col_fill, s.RenderMarkerLine, col_line, s.MarkerWeight);
        }
        EndItem();
    }
}

template <typename T>
void PlotStairs(const char* label_id, const T* values, int count, double xscale, double x0, ImPlotStairsFlags flags, int offset, int stride) {
    GetterXY<IndexerLin,IndexerIdx<T>> getter(IndexerLin(xscale,x0),IndexerIdx<T>(values,count,offset,stride),count);
    PlotStairsEx(label_id, getter, flags);
}

template <typename T>
void PlotStairs(const char* label_id, const T* xs, const T* ys, int count, ImPlotStairsFlags flags, int offset, int stride) {
    GetterXY<IndexerIdx<T>,IndexerIdx<T>> getter(IndexerIdx<T>(xs,count,offset,stride),IndexerIdx<T>(ys,count,offset,stride),count);
    return PlotStairsEx(label_id, getter, flags);
}

#define INSTANTIATE_MACRO(T) \
    template IMPLOT_API void PlotStairs<T> (const char* label_id, const T* values, int count, double xscale, double x0, ImPlotStairsFlags flags, int offset, int stride); \
    template IMPLOT_API void PlotStairs<T>(const char* label_id, const T* xs, const T* ys, int count, ImPlotStairsFlags flags, int offset, int stride);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

// custom
void PlotStairsG(const char* label_id, ImPlotGetter getter_func, void* data, int count, ImPlotStairsFlags flags) {
    GetterFuncPtr getter(getter_func,data, count);
    return PlotStairsEx(label_id, getter, flags);
}

//-----------------------------------------------------------------------------
// [SECTION] PlotShaded
//-----------------------------------------------------------------------------

template <typename Getter1, typename Getter2>
void PlotShadedEx(const char* label_id, const Getter1& getter1, const Getter2& getter2, ImPlotShadedFlags flags) {
    if (BeginItemEx(label_id, Fitter2<Getter1,Getter2>(getter1,getter2), flags, ImPlotCol_Fill)) {
        const ImPlotNextItemData& s = GetItemData();
        if (s.RenderFill) {
            const ImU32 col = ImGui::GetColorU32(s.Colors[ImPlotCol_Fill]);
            RenderPrimitives2<RendererShaded>(getter1,getter2,col);
        }
        EndItem();
    }
}

template <typename T>
void PlotShaded(const char* label_id, const T* values, int count, double y_ref, double xscale, double x0, ImPlotShadedFlags flags, int offset, int stride) {
    if (!(y_ref > -DBL_MAX))
        y_ref = GetPlotLimits(IMPLOT_AUTO,IMPLOT_AUTO).Y.Min;
    if (!(y_ref < DBL_MAX))
        y_ref = GetPlotLimits(IMPLOT_AUTO,IMPLOT_AUTO).Y.Max;
    GetterXY<IndexerLin,IndexerIdx<T>> getter1(IndexerLin(xscale,x0),IndexerIdx<T>(values,count,offset,stride),count);
    GetterXY<IndexerLin,IndexerConst>  getter2(IndexerLin(xscale,x0),IndexerConst(y_ref),count);
    PlotShadedEx(label_id, getter1, getter2, flags);
}

template <typename T>
void PlotShaded(const char* label_id, const T* xs, const T* ys, int count, double y_ref, ImPlotShadedFlags flags, int offset, int stride) {
    if (ImHasFlag(flags, ImPlotShadedFlags_Vertical)) {
        if (y_ref == -HUGE_VAL)
            y_ref = GetPlotLimits(IMPLOT_AUTO, IMPLOT_AUTO).X.Min;
        if (y_ref == HUGE_VAL)
            y_ref = GetPlotLimits(IMPLOT_AUTO, IMPLOT_AUTO).X.Max;
        GetterXY<IndexerIdx<T>, IndexerIdx<T>> getter1(IndexerIdx<T>(xs, count, offset, stride), IndexerIdx<T>(ys, count, offset, stride), count);
        GetterXY<IndexerConst, IndexerIdx<T>>  getter2(IndexerConst(y_ref), IndexerIdx<T>(ys, count, offset, stride), count);
        PlotShadedEx(label_id, getter1, getter2, flags);
    }
    else {
        if (y_ref == -HUGE_VAL)
            y_ref = GetPlotLimits(IMPLOT_AUTO, IMPLOT_AUTO).Y.Min;
        if (y_ref == HUGE_VAL)
            y_ref = GetPlotLimits(IMPLOT_AUTO, IMPLOT_AUTO).Y.Max;
        GetterXY<IndexerIdx<T>, IndexerIdx<T>> getter1(IndexerIdx<T>(xs, count, offset, stride), IndexerIdx<T>(ys, count, offset, stride), count);
        GetterXY<IndexerIdx<T>, IndexerConst>  getter2(IndexerIdx<T>(xs, count, offset, stride), IndexerConst(y_ref), count);
        PlotShadedEx(label_id, getter1, getter2, flags);
    }
}


template <typename T>
void PlotShaded(const char* label_id, const T* xs, const T* ys1, const T* ys2, int count, ImPlotShadedFlags flags, int offset, int stride) {
    GetterXY<IndexerIdx<T>,IndexerIdx<T>> getter1(IndexerIdx<T>(xs,count,offset,stride),IndexerIdx<T>(ys1,count,offset,stride),count);
    GetterXY<IndexerIdx<T>,IndexerIdx<T>> getter2(IndexerIdx<T>(xs,count,offset,stride),IndexerIdx<T>(ys2,count,offset,stride),count);
    PlotShadedEx(label_id, getter1, getter2, flags);
}

#define INSTANTIATE_MACRO(T) \
    template IMPLOT_API void PlotShaded<T>(const char* label_id, const T* values, int count, double y_ref, double xscale, double x0, ImPlotShadedFlags flags, int offset, int stride); \
    template IMPLOT_API void PlotShaded<T>(const char* label_id, const T* xs, const T* ys, int count, double y_ref, ImPlotShadedFlags flags, int offset, int stride); \
    template IMPLOT_API void PlotShaded<T>(const char* label_id, const T* xs, const T* ys1, const T* ys2, int count, ImPlotShadedFlags flags, int offset, int stride);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

// custom
void PlotShadedG(const char* label_id, ImPlotGetter getter_func1, void* data1, ImPlotGetter getter_func2, void* data2, int count, ImPlotShadedFlags flags) {
    GetterFuncPtr getter1(getter_func1, data1, count);
    GetterFuncPtr getter2(getter_func2, data2, count);
    PlotShadedEx(label_id, getter1, getter2, flags);
}

//-----------------------------------------------------------------------------
// [SECTION] PlotBars
//-----------------------------------------------------------------------------

template <typename Getter1, typename Getter2>
void PlotBarsVEx(const char* label_id, const Getter1& getter1, const Getter2 getter2, double width, ImPlotBarsFlags flags) {
    if (BeginItemEx(label_id, FitterBarV<Getter1,Getter2>(getter1,getter2,width), flags, ImPlotCol_Fill)) {
        const ImPlotNextItemData& s = GetItemData();
        const ImU32 col_fill = ImGui::GetColorU32(s.Colors[ImPlotCol_Fill]);
        const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_Line]);
        bool rend_fill = s.RenderFill;
        bool rend_line = s.RenderLine;
        if (rend_fill) {
            RenderPrimitives2<RendererBarsFillV>(getter1,getter2,col_fill,width);
            if (rend_line && col_fill == col_line)
                rend_line = false;
        }
        if (rend_line) {
            RenderPrimitives2<RendererBarsLineV>(getter1,getter2,col_line,width,s.LineWeight);
        }
        EndItem();
    }
}

template <typename Getter1, typename Getter2>
void PlotBarsHEx(const char* label_id, const Getter1& getter1, const Getter2& getter2, double height, ImPlotBarsFlags flags) {
    if (BeginItemEx(label_id, FitterBarH<Getter1,Getter2>(getter1,getter2,height), flags, ImPlotCol_Fill)) {
        const ImPlotNextItemData& s = GetItemData();
        const ImU32 col_fill = ImGui::GetColorU32(s.Colors[ImPlotCol_Fill]);
        const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_Line]);
        bool rend_fill = s.RenderFill;
        bool rend_line = s.RenderLine;
        if (rend_fill) {
            RenderPrimitives2<RendererBarsFillH>(getter1,getter2,col_fill,height);
            if (rend_line && col_fill == col_line)
                rend_line = false;
        }
        if (rend_line) {
            RenderPrimitives2<RendererBarsLineH>(getter1,getter2,col_line,height,s.LineWeight);
        }
        EndItem();
    }
}

template <typename T>
void PlotBars(const char* label_id, const T* values, int count, double bar_size, double shift, ImPlotBarsFlags flags, int offset, int stride) {
    if (ImHasFlag(flags, ImPlotBarsFlags_Horizontal)) {
        GetterXY<IndexerIdx<T>,IndexerLin> getter1(IndexerIdx<T>(values,count,offset,stride),IndexerLin(1.0,shift),count);
        GetterXY<IndexerConst,IndexerLin>  getter2(IndexerConst(0),IndexerLin(1.0,shift),count);
        PlotBarsHEx(label_id, getter1, getter2, bar_size, flags);
    }
    else {
        GetterXY<IndexerLin,IndexerIdx<T>> getter1(IndexerLin(1.0,shift),IndexerIdx<T>(values,count,offset,stride),count);
        GetterXY<IndexerLin,IndexerConst>  getter2(IndexerLin(1.0,shift),IndexerConst(0),count);
        PlotBarsVEx(label_id, getter1, getter2, bar_size, flags);
    }
}

template <typename T>
void PlotBars(const char* label_id, const T* xs, const T* ys, int count, double bar_size, ImPlotBarsFlags flags, int offset, int stride) {
    if (ImHasFlag(flags, ImPlotBarsFlags_Horizontal)) {
        GetterXY<IndexerIdx<T>,IndexerIdx<T>> getter1(IndexerIdx<T>(xs,count,offset,stride),IndexerIdx<T>(ys,count,offset,stride),count);
        GetterXY<IndexerConst, IndexerIdx<T>> getter2(IndexerConst(0),IndexerIdx<T>(ys,count,offset,stride),count);
        PlotBarsHEx(label_id, getter1, getter2, bar_size, flags);
    }
    else {
        GetterXY<IndexerIdx<T>,IndexerIdx<T>> getter1(IndexerIdx<T>(xs,count,offset,stride),IndexerIdx<T>(ys,count,offset,stride),count);
        GetterXY<IndexerIdx<T>,IndexerConst>  getter2(IndexerIdx<T>(xs,count,offset,stride),IndexerConst(0),count);
        PlotBarsVEx(label_id, getter1, getter2, bar_size, flags);
    }
}

#define INSTANTIATE_MACRO(T) \
    template IMPLOT_API void PlotBars<T>(const char* label_id, const T* values, int count, double bar_size, double shift, ImPlotBarsFlags flags, int offset, int stride); \
    template IMPLOT_API void PlotBars<T>(const char* label_id, const T* xs, const T* ys, int count, double bar_size, ImPlotBarsFlags flags, int offset, int stride);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

void PlotBarsG(const char* label_id, ImPlotGetter getter_func, void* data, int count, double bar_size, ImPlotBarsFlags flags) {
    if (ImHasFlag(flags, ImPlotBarsFlags_Horizontal)) {
        GetterFuncPtr getter1(getter_func, data, count);
        GetterOverrideX<GetterFuncPtr> getter2(getter1,0);
        PlotBarsHEx(label_id, getter1, getter2, bar_size, flags);
    }
    else {
        GetterFuncPtr getter1(getter_func, data, count);
        GetterOverrideY<GetterFuncPtr> getter2(getter1,0);
        PlotBarsVEx(label_id, getter1, getter2, bar_size, flags);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] PlotBarGroups
//-----------------------------------------------------------------------------

template <typename T>
void PlotBarGroups(const char* const label_ids[], const T* values, int item_count, int group_count, double group_size, double shift, ImPlotBarGroupsFlags flags) {
    const bool horz = ImHasFlag(flags, ImPlotBarGroupsFlags_Horizontal);
    const bool stack = ImHasFlag(flags, ImPlotBarGroupsFlags_Stacked);
    if (stack) {
        SetupLock();
        GImPlot->TempDouble1.resize(4*group_count);
        double* temp = GImPlot->TempDouble1.Data;
        double* neg =      &temp[0];
        double* pos =      &temp[group_count];
        double* curr_min = &temp[group_count*2];
        double* curr_max = &temp[group_count*3];
        for (int g = 0; g < group_count*2; ++g)
            temp[g] = 0;
        if (horz) {
            for (int i = 0; i < item_count; ++i) {
                if (!IsItemHidden(label_ids[i])) {
                    for (int g = 0; g < group_count; ++g) {
                        double v = (double)values[i*group_count+g];
                        if (v > 0) {
                            curr_min[g] = pos[g];
                            curr_max[g] = curr_min[g] + v;
                            pos[g]      += v;
                        }
                        else {
                            curr_max[g] = neg[g];
                            curr_min[g] = curr_max[g] + v;
                            neg[g]      += v;
                        }
                    }
                }
                GetterXY<IndexerIdx<double>,IndexerLin> getter1(IndexerIdx<double>(curr_min,group_count),IndexerLin(1.0,shift),group_count);
                GetterXY<IndexerIdx<double>,IndexerLin> getter2(IndexerIdx<double>(curr_max,group_count),IndexerLin(1.0,shift),group_count);
                PlotBarsHEx(label_ids[i],getter1,getter2,group_size,0);
            }
        }
        else {
            for (int i = 0; i < item_count; ++i) {
                if (!IsItemHidden(label_ids[i])) {
                    for (int g = 0; g < group_count; ++g) {
                        double v = (double)values[i*group_count+g];
                        if (v > 0) {
                            curr_min[g] = pos[g];
                            curr_max[g] = curr_min[g] + v;
                            pos[g]      += v;
                        }
                        else {
                            curr_max[g] = neg[g];
                            curr_min[g] = curr_max[g] + v;
                            neg[g]      += v;
                        }
                    }
                }
                GetterXY<IndexerLin,IndexerIdx<double>> getter1(IndexerLin(1.0,shift),IndexerIdx<double>(curr_min,group_count),group_count);
                GetterXY<IndexerLin,IndexerIdx<double>> getter2(IndexerLin(1.0,shift),IndexerIdx<double>(curr_max,group_count),group_count);
                PlotBarsVEx(label_ids[i],getter1,getter2,group_size,0);
            }
        }
    }
    else {
        const double subsize = group_size / item_count;
        if (horz) {
            for (int i = 0; i < item_count; ++i) {
                const double subshift = (i+0.5)*subsize - group_size/2;
                PlotBars(label_ids[i],&values[i*group_count],group_count,subsize,subshift+shift,ImPlotBarsFlags_Horizontal);
            }
        }
        else {
            for (int i = 0; i < item_count; ++i) {
                const double subshift = (i+0.5)*subsize - group_size/2;
                PlotBars(label_ids[i],&values[i*group_count],group_count,subsize,subshift+shift);
            }
        }
    }
}

#define INSTANTIATE_MACRO(T) template IMPLOT_API void PlotBarGroups<T>(const char* const label_ids[], const T* values, int items, int groups, double width, double shift, ImPlotBarGroupsFlags flags);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

//-----------------------------------------------------------------------------
// [SECTION] PlotErrorBars
//-----------------------------------------------------------------------------

template <typename _GetterPos, typename _GetterNeg>
void PlotErrorBarsVEx(const char* label_id, const _GetterPos& getter_pos, const _GetterNeg& getter_neg, ImPlotErrorBarsFlags flags) {
    if (BeginItemEx(label_id, Fitter2<_GetterPos,_GetterNeg>(getter_pos, getter_neg), flags, IMPLOT_AUTO)) {
        const ImPlotNextItemData& s = GetItemData();
        ImDrawList& draw_list = *GetPlotDrawList();
        const ImU32 col = ImGui::GetColorU32(s.Colors[ImPlotCol_ErrorBar]);
        const bool rend_whisker  = s.ErrorBarSize > 0;
        const float half_whisker = s.ErrorBarSize * 0.5f;
        for (int i = 0; i < getter_pos.Count; ++i) {
            ImVec2 p1 = PlotToPixels(getter_neg(i),IMPLOT_AUTO,IMPLOT_AUTO);
            ImVec2 p2 = PlotToPixels(getter_pos(i),IMPLOT_AUTO,IMPLOT_AUTO);
            draw_list.AddLine(p1,p2,col, s.ErrorBarWeight);
            if (rend_whisker) {
                draw_list.AddLine(p1 - ImVec2(half_whisker, 0), p1 + ImVec2(half_whisker, 0), col, s.ErrorBarWeight);
                draw_list.AddLine(p2 - ImVec2(half_whisker, 0), p2 + ImVec2(half_whisker, 0), col, s.ErrorBarWeight);
            }
        }
        EndItem();
    }
}

template <typename _GetterPos, typename _GetterNeg>
void PlotErrorBarsHEx(const char* label_id, const _GetterPos& getter_pos, const _GetterNeg& getter_neg, ImPlotErrorBarsFlags flags) {
    if (BeginItemEx(label_id, Fitter2<_GetterPos,_GetterNeg>(getter_pos, getter_neg), flags, IMPLOT_AUTO)) {
        const ImPlotNextItemData& s = GetItemData();
        ImDrawList& draw_list = *GetPlotDrawList();
        const ImU32 col = ImGui::GetColorU32(s.Colors[ImPlotCol_ErrorBar]);
        const bool rend_whisker  = s.ErrorBarSize > 0;
        const float half_whisker = s.ErrorBarSize * 0.5f;
        for (int i = 0; i < getter_pos.Count; ++i) {
            ImVec2 p1 = PlotToPixels(getter_neg(i),IMPLOT_AUTO,IMPLOT_AUTO);
            ImVec2 p2 = PlotToPixels(getter_pos(i),IMPLOT_AUTO,IMPLOT_AUTO);
            draw_list.AddLine(p1, p2, col, s.ErrorBarWeight);
            if (rend_whisker) {
                draw_list.AddLine(p1 - ImVec2(0, half_whisker), p1 + ImVec2(0, half_whisker), col, s.ErrorBarWeight);
                draw_list.AddLine(p2 - ImVec2(0, half_whisker), p2 + ImVec2(0, half_whisker), col, s.ErrorBarWeight);
            }
        }
        EndItem();
    }
}

template <typename T>
void PlotErrorBars(const char* label_id, const T* xs, const T* ys, const T* err, int count, ImPlotErrorBarsFlags flags, int offset, int stride) {
    PlotErrorBars(label_id, xs, ys, err, err, count, flags, offset, stride);
}

template <typename T>
void PlotErrorBars(const char* label_id, const T* xs, const T* ys, const T* neg, const T* pos, int count, ImPlotErrorBarsFlags flags, int offset, int stride) {
    IndexerIdx<T> indexer_x(xs, count,offset,stride);
    IndexerIdx<T> indexer_y(ys, count,offset,stride);
    IndexerIdx<T> indexer_n(neg,count,offset,stride);
    IndexerIdx<T> indexer_p(pos,count,offset,stride);
    GetterError<T> getter(xs, ys, neg, pos, count, offset, stride);
    if (ImHasFlag(flags, ImPlotErrorBarsFlags_Horizontal)) {
        IndexerAdd<IndexerIdx<T>,IndexerIdx<T>> indexer_xp(indexer_x, indexer_p, 1,  1);
        IndexerAdd<IndexerIdx<T>,IndexerIdx<T>> indexer_xn(indexer_x, indexer_n, 1, -1);
        GetterXY<IndexerAdd<IndexerIdx<T>,IndexerIdx<T>>,IndexerIdx<T>> getter_p(indexer_xp, indexer_y, count);
        GetterXY<IndexerAdd<IndexerIdx<T>,IndexerIdx<T>>,IndexerIdx<T>> getter_n(indexer_xn, indexer_y, count);
        PlotErrorBarsHEx(label_id, getter_p, getter_n, flags);
    }
    else {
        IndexerAdd<IndexerIdx<T>,IndexerIdx<T>> indexer_yp(indexer_y, indexer_p, 1,  1);
        IndexerAdd<IndexerIdx<T>,IndexerIdx<T>> indexer_yn(indexer_y, indexer_n, 1, -1);
        GetterXY<IndexerIdx<T>,IndexerAdd<IndexerIdx<T>,IndexerIdx<T>>> getter_p(indexer_x, indexer_yp, count);
        GetterXY<IndexerIdx<T>,IndexerAdd<IndexerIdx<T>,IndexerIdx<T>>> getter_n(indexer_x, indexer_yn, count);
        PlotErrorBarsVEx(label_id, getter_p, getter_n, flags);
    }
}

#define INSTANTIATE_MACRO(T) \
    template IMPLOT_API void PlotErrorBars<T>(const char* label_id, const T* xs, const T* ys, const T* err, int count, ImPlotErrorBarsFlags flags, int offset, int stride); \
    template IMPLOT_API void PlotErrorBars<T>(const char* label_id, const T* xs, const T* ys, const T* neg, const T* pos, int count, ImPlotErrorBarsFlags flags, int offset, int stride);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

//-----------------------------------------------------------------------------
// [SECTION] PlotStems
//-----------------------------------------------------------------------------

template <typename _GetterM, typename _GetterB>
void PlotStemsEx(const char* label_id, const _GetterM& get_mark, const _GetterB& get_base, ImPlotStemsFlags flags) {
    if (BeginItemEx(label_id, Fitter2<_GetterM,_GetterB>(get_mark,get_base), flags, ImPlotCol_Line)) {
        const ImPlotNextItemData& s = GetItemData();
        // render stems
        if (s.RenderLine) {
            const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_Line]);
            RenderPrimitives2<RendererLineSegments2>(get_mark, get_base, col_line, s.LineWeight);
        }
        // render markers
        if (s.Marker != ImPlotMarker_None) {
            PopPlotClipRect();
            PushPlotClipRect(s.MarkerSize);
            const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_MarkerOutline]);
            const ImU32 col_fill = ImGui::GetColorU32(s.Colors[ImPlotCol_MarkerFill]);
            RenderMarkers<_GetterM>(get_mark, s.Marker, s.MarkerSize, s.RenderMarkerFill, col_fill, s.RenderMarkerLine, col_line, s.MarkerWeight);
        }
        EndItem();
    }
}

template <typename T>
void PlotStems(const char* label_id, const T* values, int count, double ref, double scale, double start, ImPlotStemsFlags flags, int offset, int stride) {
    if (ImHasFlag(flags, ImPlotStemsFlags_Horizontal)) {
        GetterXY<IndexerIdx<T>,IndexerLin> get_mark(IndexerIdx<T>(values,count,offset,stride),IndexerLin(scale,start),count);
        GetterXY<IndexerConst,IndexerLin>  get_base(IndexerConst(ref),IndexerLin(scale,start),count);
        PlotStemsEx(label_id, get_mark, get_base, flags);
    }
    else {
        GetterXY<IndexerLin,IndexerIdx<T>> get_mark(IndexerLin(scale,start),IndexerIdx<T>(values,count,offset,stride),count);
        GetterXY<IndexerLin,IndexerConst>  get_base(IndexerLin(scale,start),IndexerConst(ref),count);
        PlotStemsEx(label_id, get_mark, get_base, flags);
    }
}

template <typename T>
void PlotStems(const char* label_id, const T* xs, const T* ys, int count, double ref, ImPlotStemsFlags flags, int offset, int stride) {
    if (ImHasFlag(flags, ImPlotStemsFlags_Horizontal)) {
        GetterXY<IndexerIdx<T>,IndexerIdx<T>> get_mark(IndexerIdx<T>(xs,count,offset,stride),IndexerIdx<T>(ys,count,offset,stride),count);
        GetterXY<IndexerConst,IndexerIdx<T>>  get_base(IndexerConst(ref),IndexerIdx<T>(ys,count,offset,stride),count);
        PlotStemsEx(label_id, get_mark, get_base, flags);
    }
    else {
        GetterXY<IndexerIdx<T>,IndexerIdx<T>> get_mark(IndexerIdx<T>(xs,count,offset,stride),IndexerIdx<T>(ys,count,offset,stride),count);
        GetterXY<IndexerIdx<T>,IndexerConst>  get_base(IndexerIdx<T>(xs,count,offset,stride),IndexerConst(ref),count);
        PlotStemsEx(label_id, get_mark, get_base, flags);
    }
}

#define INSTANTIATE_MACRO(T) \
    template IMPLOT_API void PlotStems<T>(const char* label_id, const T* values, int count, double ref, double scale, double start, ImPlotStemsFlags flags, int offset, int stride); \
    template IMPLOT_API void PlotStems<T>(const char* label_id, const T* xs, const T* ys, int count, double ref, ImPlotStemsFlags flags, int offset, int stride);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO


//-----------------------------------------------------------------------------
// [SECTION] PlotInfLines
//-----------------------------------------------------------------------------

template <typename T>
void PlotInfLines(const char* label_id, const T* values, int count, ImPlotInfLinesFlags flags, int offset, int stride) {
    const ImPlotRect lims = GetPlotLimits(IMPLOT_AUTO,IMPLOT_AUTO);
    if (ImHasFlag(flags, ImPlotInfLinesFlags_Horizontal)) {
        GetterXY<IndexerConst,IndexerIdx<T>> get_min(IndexerConst(lims.X.Min),IndexerIdx<T>(values,count,offset,stride),count);
        GetterXY<IndexerConst,IndexerIdx<T>> get_max(IndexerConst(lims.X.Max),IndexerIdx<T>(values,count,offset,stride),count);
        if (BeginItemEx(label_id, FitterY<GetterXY<IndexerConst,IndexerIdx<T>>>(get_min), flags, ImPlotCol_Line)) {
            const ImPlotNextItemData& s = GetItemData();
            const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_Line]);
            if (s.RenderLine)
                RenderPrimitives2<RendererLineSegments2>(get_min, get_max, col_line, s.LineWeight);
            EndItem();
        }
    }
    else {
        GetterXY<IndexerIdx<T>,IndexerConst> get_min(IndexerIdx<T>(values,count,offset,stride),IndexerConst(lims.Y.Min),count);
        GetterXY<IndexerIdx<T>,IndexerConst> get_max(IndexerIdx<T>(values,count,offset,stride),IndexerConst(lims.Y.Max),count);
        if (BeginItemEx(label_id, FitterX<GetterXY<IndexerIdx<T>,IndexerConst>>(get_min), flags, ImPlotCol_Line)) {
            const ImPlotNextItemData& s = GetItemData();
            const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_Line]);
            if (s.RenderLine)
                RenderPrimitives2<RendererLineSegments2>(get_min, get_max, col_line, s.LineWeight);
            EndItem();
        }
    }
}
#define INSTANTIATE_MACRO(T) template IMPLOT_API void PlotInfLines<T>(const char* label_id, const T* xs, int count, ImPlotInfLinesFlags flags, int offset, int stride);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

//-----------------------------------------------------------------------------
// [SECTION] PlotPieChart
//-----------------------------------------------------------------------------

IMPLOT_INLINE void RenderPieSlice(ImDrawList& draw_list, const ImPlotPoint& center, double radius, double a0, double a1, ImU32 col) {
    const float resolution = 50 / (2 * IM_PI);
    ImVec2 buffer[52];
    buffer[0] = PlotToPixels(center,IMPLOT_AUTO,IMPLOT_AUTO);
    int n = ImMax(3, (int)((a1 - a0) * resolution));
    double da = (a1 - a0) / (n - 1);
    int i = 0;
    for (; i < n; ++i) {
        double a = a0 + i * da;
        buffer[i + 1] = PlotToPixels(center.x + radius * cos(a), center.y + radius * sin(a),IMPLOT_AUTO,IMPLOT_AUTO);
    }
    buffer[i+1] = buffer[0];
    // fill
    draw_list.AddConvexPolyFilled(buffer, n + 1, col);
    // border (for AA)
    draw_list.AddPolyline(buffer, n + 2, col, 0, 2.0f);
}

template <typename T>
void PlotPieChart(const char* const label_ids[], const T* values, int count, double x, double y, double radius, const char* fmt, double angle0, ImPlotPieChartFlags flags) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "PlotPieChart() needs to be called between BeginPlot() and EndPlot()!");
    ImDrawList & draw_list = *GetPlotDrawList();
    double sum = 0;
    for (int i = 0; i < count; ++i)
        sum += (double)values[i];
    const bool normalize = ImHasFlag(flags,ImPlotPieChartFlags_Normalize) || sum > 1.0;
    ImPlotPoint center(x,y);
    PushPlotClipRect();
    double a0 = angle0 * 2 * IM_PI / 360.0;
    double a1 = angle0 * 2 * IM_PI / 360.0;
    ImPlotPoint Pmin = ImPlotPoint(x-radius,y-radius);
    ImPlotPoint Pmax = ImPlotPoint(x+radius,y+radius);
    for (int i = 0; i < count; ++i) {
        double percent = normalize ? (double)values[i] / sum : (double)values[i];
        a1 = a0 + 2 * IM_PI * percent;
        if (BeginItemEx(label_ids[i], FitterRect(Pmin,Pmax))) {
            ImU32 col = GetCurrentItem()->Color;
            if (percent < 0.5) {
                RenderPieSlice(draw_list, center, radius, a0, a1, col);
            }
            else  {
                RenderPieSlice(draw_list, center, radius, a0, a0 + (a1 - a0) * 0.5, col);
                RenderPieSlice(draw_list, center, radius, a0 + (a1 - a0) * 0.5, a1, col);
            }
            EndItem();
        }
        a0 = a1;
    }
    if (fmt != NULL) {
        a0 = angle0 * 2 * IM_PI / 360.0;
        a1 = angle0 * 2 * IM_PI / 360.0;
        char buffer[32];
        for (int i = 0; i < count; ++i) {
            ImPlotItem* item = GetItem(label_ids[i]);
            double percent = normalize ? (double)values[i] / sum : (double)values[i];
            a1 = a0 + 2 * IM_PI * percent;
            if (item->Show) {
                ImFormatString(buffer, 32, fmt, (double)values[i]);
                ImVec2 size = ImGui::CalcTextSize(buffer);
                double angle = a0 + (a1 - a0) * 0.5;
                ImVec2 pos = PlotToPixels(center.x + 0.5 * radius * cos(angle), center.y + 0.5 * radius * sin(angle),IMPLOT_AUTO,IMPLOT_AUTO);
                ImU32 col  = CalcTextColor(ImGui::ColorConvertU32ToFloat4(item->Color));
                draw_list.AddText(pos - size * 0.5f, col, buffer);
            }
            a0 = a1;
        }
    }
    PopPlotClipRect();
}
#define INSTANTIATE_MACRO(T) template IMPLOT_API void PlotPieChart<T>(const char* const label_ids[], const T* values, int count, double x, double y, double radius, const char* fmt, double angle0, ImPlotPieChartFlags flags);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

//-----------------------------------------------------------------------------
// [SECTION] PlotHeatmap
//-----------------------------------------------------------------------------

template <typename T>
struct GetterHeatmapRowMaj {
    GetterHeatmapRowMaj(const T* values, int rows, int cols, double scale_min, double scale_max, double width, double height, double xref, double yref, double ydir) :
        Values(values),
        Count(rows*cols),
        Rows(rows),
        Cols(cols),
        ScaleMin(scale_min),
        ScaleMax(scale_max),
        Width(width),
        Height(height),
        XRef(xref),
        YRef(yref),
        YDir(ydir),
        HalfSize(Width*0.5, Height*0.5)
    { }
    template <typename I> IMPLOT_INLINE RectC operator()(I idx) const {
        double val = (double)Values[idx];
        const int r = idx / Cols;
        const int c = idx % Cols;
        const ImPlotPoint p(XRef + HalfSize.x + c*Width, YRef + YDir * (HalfSize.y + r*Height));
        RectC rect;
        rect.Pos = p;
        rect.HalfSize = HalfSize;
        const float t = ImClamp((float)ImRemap01(val, ScaleMin, ScaleMax),0.0f,1.0f);
        rect.Color = GImPlot->ColormapData.LerpTable(GImPlot->Style.Colormap, t);
        return rect;
    }
    const T* const Values;
    const int Count, Rows, Cols;
    const double ScaleMin, ScaleMax, Width, Height, XRef, YRef, YDir;
    const ImPlotPoint HalfSize;
};

template <typename T>
struct GetterHeatmapColMaj {
    GetterHeatmapColMaj(const T* values, int rows, int cols, double scale_min, double scale_max, double width, double height, double xref, double yref, double ydir) :
        Values(values),
        Count(rows*cols),
        Rows(rows),
        Cols(cols),
        ScaleMin(scale_min),
        ScaleMax(scale_max),
        Width(width),
        Height(height),
        XRef(xref),
        YRef(yref),
        YDir(ydir),
        HalfSize(Width*0.5, Height*0.5)
    { }
    template <typename I> IMPLOT_INLINE RectC operator()(I idx) const {
        double val = (double)Values[idx];
        const int r = idx % Cols;
        const int c = idx / Cols;
        const ImPlotPoint p(XRef + HalfSize.x + c*Width, YRef + YDir * (HalfSize.y + r*Height));
        RectC rect;
        rect.Pos = p;
        rect.HalfSize = HalfSize;
        const float t = ImClamp((float)ImRemap01(val, ScaleMin, ScaleMax),0.0f,1.0f);
        rect.Color = GImPlot->ColormapData.LerpTable(GImPlot->Style.Colormap, t);
        return rect;
    }
    const T* const Values;
    const int Count, Rows, Cols;
    const double ScaleMin, ScaleMax, Width, Height, XRef, YRef, YDir;
    const ImPlotPoint HalfSize;
};

template <typename T>
void RenderHeatmap(ImDrawList& draw_list, const T* values, int rows, int cols, double scale_min, double scale_max, const char* fmt, const ImPlotPoint& bounds_min, const ImPlotPoint& bounds_max, bool reverse_y, bool col_maj) {
    ImPlotContext& gp = *GImPlot;
    Transformer2 transformer;
    if (scale_min == 0 && scale_max == 0) {
        T temp_min, temp_max;
        ImMinMaxArray(values,rows*cols,&temp_min,&temp_max);
        scale_min = (double)temp_min;
        scale_max = (double)temp_max;
    }
    if (scale_min == scale_max) {
        ImVec2 a = transformer(bounds_min);
        ImVec2 b = transformer(bounds_max);
        ImU32  col = GetColormapColorU32(0,gp.Style.Colormap);
        draw_list.AddRectFilled(a, b, col);
        return;
    }
    const double yref = reverse_y ? bounds_max.y : bounds_min.y;
    const double ydir = reverse_y ? -1 : 1;
    if (col_maj) {
        GetterHeatmapColMaj<T> getter(values, rows, cols, scale_min, scale_max, (bounds_max.x - bounds_min.x) / cols, (bounds_max.y - bounds_min.y) / rows, bounds_min.x, yref, ydir);
        RenderPrimitives1<RendererRectC>(getter);
    }
    else {
        GetterHeatmapRowMaj<T> getter(values, rows, cols, scale_min, scale_max, (bounds_max.x - bounds_min.x) / cols, (bounds_max.y - bounds_min.y) / rows, bounds_min.x, yref, ydir);
        RenderPrimitives1<RendererRectC>(getter);
    }
    // labels
    if (fmt != NULL) {
        const double w = (bounds_max.x - bounds_min.x) / cols;
        const double h = (bounds_max.y - bounds_min.y) / rows;
        const ImPlotPoint half_size(w*0.5,h*0.5);
        int i = 0;
        if (col_maj) {
            for (int c = 0; c < cols; ++c) {
                for (int r = 0; r < rows; ++r) {
                    ImPlotPoint p;
                    p.x = bounds_min.x + 0.5*w + c*w;
                    p.y = yref + ydir * (0.5*h + r*h);
                    ImVec2 px = transformer(p);
                    char buff[32];
                    ImFormatString(buff, 32, fmt, values[i]);
                    ImVec2 size = ImGui::CalcTextSize(buff);
                    double t = ImClamp(ImRemap01((double)values[i], scale_min, scale_max),0.0,1.0);
                    ImVec4 color = SampleColormap((float)t);
                    ImU32 col = CalcTextColor(color);
                    draw_list.AddText(px - size * 0.5f, col, buff);
                    i++;
                }
            }
        }
        else {
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    ImPlotPoint p;
                    p.x = bounds_min.x + 0.5*w + c*w;
                    p.y = yref + ydir * (0.5*h + r*h);
                    ImVec2 px = transformer(p);
                    char buff[32];
                    ImFormatString(buff, 32, fmt, values[i]);
                    ImVec2 size = ImGui::CalcTextSize(buff);
                    double t = ImClamp(ImRemap01((double)values[i], scale_min, scale_max),0.0,1.0);
                    ImVec4 color = SampleColormap((float)t);
                    ImU32 col = CalcTextColor(color);
                    draw_list.AddText(px - size * 0.5f, col, buff);
                    i++;
                }
            }
        }
    }
}

template <typename T>
void PlotHeatmap(const char* label_id, const T* values, int rows, int cols, double scale_min, double scale_max, const char* fmt, const ImPlotPoint& bounds_min, const ImPlotPoint& bounds_max, ImPlotHeatmapFlags flags) {
    if (BeginItemEx(label_id, FitterRect(bounds_min, bounds_max))) {
        ImDrawList& draw_list = *GetPlotDrawList();
        const bool col_maj = ImHasFlag(flags, ImPlotHeatmapFlags_ColMajor);
        RenderHeatmap(draw_list, values, rows, cols, scale_min, scale_max, fmt, bounds_min, bounds_max, true, col_maj);
        EndItem();
    }
}
#define INSTANTIATE_MACRO(T) template IMPLOT_API void PlotHeatmap<T>(const char* label_id, const T* values, int rows, int cols, double scale_min, double scale_max, const char* fmt, const ImPlotPoint& bounds_min, const ImPlotPoint& bounds_max, ImPlotHeatmapFlags flags);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

//-----------------------------------------------------------------------------
// [SECTION] PlotHistogram
//-----------------------------------------------------------------------------

template <typename T>
double PlotHistogram(const char* label_id, const T* values, int count, int bins, double bar_scale, ImPlotRange range, ImPlotHistogramFlags flags) {

    const bool cumulative = ImHasFlag(flags, ImPlotHistogramFlags_Cumulative);
    const bool density    = ImHasFlag(flags, ImPlotHistogramFlags_Density);
    const bool outliers   = !ImHasFlag(flags, ImPlotHistogramFlags_NoOutliers);

    if (count <= 0 || bins == 0)
        return 0;

    if (range.Min == 0 && range.Max == 0) {
        T Min, Max;
        ImMinMaxArray(values, count, &Min, &Max);
        range.Min = (double)Min;
        range.Max = (double)Max;
    }

    double width;
    if (bins < 0)
        CalculateBins(values, count, bins, range, bins, width);
    else
        width = range.Size() / bins;

    ImVector<double>& bin_centers = GImPlot->TempDouble1;
    ImVector<double>& bin_counts  = GImPlot->TempDouble2;
    bin_centers.resize(bins);
    bin_counts.resize(bins);
    int below = 0;

    for (int b = 0; b < bins; ++b) {
        bin_centers[b] = range.Min + b * width + width * 0.5;
        bin_counts[b] = 0;
    }
    int counted = 0;
    double max_count = 0;
    for (int i = 0; i < count; ++i) {
        double val = (double)values[i];
        if (range.Contains(val)) {
            const int b = ImClamp((int)((val - range.Min) / width), 0, bins - 1);
            bin_counts[b] += 1.0;
            if (bin_counts[b] > max_count)
                max_count = bin_counts[b];
            counted++;
        }
        else if (val < range.Min) {
            below++;
        }
    }
    if (cumulative && density) {
        if (outliers)
            bin_counts[0] += below;
        for (int b = 1; b < bins; ++b)
            bin_counts[b] += bin_counts[b-1];
        double scale = 1.0 / (outliers ? count : counted);
        for (int b = 0; b < bins; ++b)
            bin_counts[b] *= scale;
        max_count = bin_counts[bins-1];
    }
    else if (cumulative) {
        if (outliers)
            bin_counts[0] += below;
        for (int b = 1; b < bins; ++b)
            bin_counts[b] += bin_counts[b-1];
        max_count = bin_counts[bins-1];
    }
    else if (density) {
        double scale = 1.0 / ((outliers ? count : counted) * width);
        for (int b = 0; b < bins; ++b)
            bin_counts[b] *= scale;
        max_count *= scale;
    }
    if (ImHasFlag(flags, ImPlotHistogramFlags_Horizontal))
        PlotBars(label_id, &bin_counts.Data[0], &bin_centers.Data[0], bins, bar_scale*width, ImPlotBarsFlags_Horizontal);
    else
        PlotBars(label_id, &bin_centers.Data[0], &bin_counts.Data[0], bins, bar_scale*width);
    return max_count;
}
#define INSTANTIATE_MACRO(T) template IMPLOT_API double PlotHistogram<T>(const char* label_id, const T* values, int count, int bins, double bar_scale, ImPlotRange range, ImPlotHistogramFlags flags);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

//-----------------------------------------------------------------------------
// [SECTION] PlotHistogram2D
//-----------------------------------------------------------------------------

template <typename T>
double PlotHistogram2D(const char* label_id, const T* xs, const T* ys, int count, int x_bins, int y_bins, ImPlotRect range, ImPlotHistogramFlags flags) {

    // const bool cumulative = ImHasFlag(flags, ImPlotHistogramFlags_Cumulative); NOT SUPPORTED
    const bool density  = ImHasFlag(flags, ImPlotHistogramFlags_Density);
    const bool outliers = !ImHasFlag(flags, ImPlotHistogramFlags_NoOutliers);
    const bool col_maj  = ImHasFlag(flags, ImPlotHistogramFlags_ColMajor);

    if (count <= 0 || x_bins == 0 || y_bins == 0)
        return 0;

    if (range.X.Min == 0 && range.X.Max == 0) {
        T Min, Max;
        ImMinMaxArray(xs, count, &Min, &Max);
        range.X.Min = (double)Min;
        range.X.Max = (double)Max;
    }
    if (range.Y.Min == 0 && range.Y.Max == 0) {
        T Min, Max;
        ImMinMaxArray(ys, count, &Min, &Max);
        range.Y.Min = (double)Min;
        range.Y.Max = (double)Max;
    }

    double width, height;
    if (x_bins < 0)
        CalculateBins(xs, count, x_bins, range.X, x_bins, width);
    else
        width = range.X.Size() / x_bins;
    if (y_bins < 0)
        CalculateBins(ys, count, y_bins, range.Y, y_bins, height);
    else
        height = range.Y.Size() / y_bins;

    const int bins = x_bins * y_bins;

    ImVector<double>& bin_counts = GImPlot->TempDouble1;
    bin_counts.resize(bins);

    for (int b = 0; b < bins; ++b)
        bin_counts[b] = 0;

    int counted = 0;
    double max_count = 0;
    for (int i = 0; i < count; ++i) {
        if (range.Contains((double)xs[i], (double)ys[i])) {
            const int xb = ImClamp( (int)((double)(xs[i] - range.X.Min) / width)  , 0, x_bins - 1);
            const int yb = ImClamp( (int)((double)(ys[i] - range.Y.Min) / height) , 0, y_bins - 1);
            const int b  = yb * x_bins + xb;
            bin_counts[b] += 1.0;
            if (bin_counts[b] > max_count)
                max_count = bin_counts[b];
            counted++;
        }
    }
    if (density) {
        double scale = 1.0 / ((outliers ? count : counted) * width * height);
        for (int b = 0; b < bins; ++b)
            bin_counts[b] *= scale;
        max_count *= scale;
    }

    if (BeginItemEx(label_id, FitterRect(range))) {
        ImDrawList& draw_list = *GetPlotDrawList();
        RenderHeatmap(draw_list, &bin_counts.Data[0], y_bins, x_bins, 0, max_count, NULL, range.Min(), range.Max(), false, col_maj);
        EndItem();
    }
    return max_count;
}
#define INSTANTIATE_MACRO(T) template IMPLOT_API double PlotHistogram2D<T>(const char* label_id,   const T*   xs, const T*   ys, int count, int x_bins, int y_bins, ImPlotRect range, ImPlotHistogramFlags flags);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

//-----------------------------------------------------------------------------
// [SECTION] PlotDigital
//-----------------------------------------------------------------------------

// TODO: Make this behave like all the other plot types (.e. not fixed in y axis)

template <typename Getter>
void PlotDigitalEx(const char* label_id, Getter getter, ImPlotDigitalFlags flags) {
    if (BeginItem(label_id, flags, ImPlotCol_Fill)) {
        ImPlotContext& gp = *GImPlot;
        ImDrawList& draw_list = *GetPlotDrawList();
        const ImPlotNextItemData& s = GetItemData();
        if (getter.Count > 1 && s.RenderFill) {
            ImPlotPlot& plot   = *gp.CurrentPlot;
            ImPlotAxis& x_axis = plot.Axes[plot.CurrentX];
            ImPlotAxis& y_axis = plot.Axes[plot.CurrentY];

            int pixYMax = 0;
            ImPlotPoint itemData1 = getter(0);
            for (int i = 0; i < getter.Count; ++i) {
                ImPlotPoint itemData2 = getter(i);
                if (ImNanOrInf(itemData1.y)) {
                    itemData1 = itemData2;
                    continue;
                }
                if (ImNanOrInf(itemData2.y)) itemData2.y = ImConstrainNan(ImConstrainInf(itemData2.y));
                int pixY_0 = (int)(s.LineWeight);
                itemData1.y = ImMax(0.0, itemData1.y);
                float pixY_1_float = s.DigitalBitHeight * (float)itemData1.y;
                int pixY_1 = (int)(pixY_1_float); //allow only positive values
                int pixY_chPosOffset = (int)(ImMax(s.DigitalBitHeight, pixY_1_float) + s.DigitalBitGap);
                pixYMax = ImMax(pixYMax, pixY_chPosOffset);
                ImVec2 pMin = PlotToPixels(itemData1,IMPLOT_AUTO,IMPLOT_AUTO);
                ImVec2 pMax = PlotToPixels(itemData2,IMPLOT_AUTO,IMPLOT_AUTO);
                int pixY_Offset = 0; //20 pixel from bottom due to mouse cursor label
                pMin.y = (y_axis.PixelMin) + ((-gp.DigitalPlotOffset)                   - pixY_Offset);
                pMax.y = (y_axis.PixelMin) + ((-gp.DigitalPlotOffset) - pixY_0 - pixY_1 - pixY_Offset);
                //plot only one rectangle for same digital state
                while (((i+2) < getter.Count) && (itemData1.y == itemData2.y)) {
                    const int in = (i + 1);
                    itemData2 = getter(in);
                    if (ImNanOrInf(itemData2.y)) break;
                    pMax.x = PlotToPixels(itemData2,IMPLOT_AUTO,IMPLOT_AUTO).x;
                    i++;
                }
                //do not extend plot outside plot range
                if (pMin.x < x_axis.PixelMin) pMin.x = x_axis.PixelMin;
                if (pMax.x < x_axis.PixelMin) pMax.x = x_axis.PixelMin;
                if (pMin.x > x_axis.PixelMax) pMin.x = x_axis.PixelMax;
                if (pMax.x > x_axis.PixelMax) pMax.x = x_axis.PixelMax;
                //plot a rectangle that extends up to x2 with y1 height
                if ((pMax.x > pMin.x) && (gp.CurrentPlot->PlotRect.Contains(pMin) || gp.CurrentPlot->PlotRect.Contains(pMax))) {
                    // ImVec4 colAlpha = item->Color;
                    // colAlpha.w = item->Highlight ? 1.0f : 0.9f;
                    draw_list.AddRectFilled(pMin, pMax, ImGui::GetColorU32(s.Colors[ImPlotCol_Fill]));
                }
                itemData1 = itemData2;
            }
            gp.DigitalPlotItemCnt++;
            gp.DigitalPlotOffset += pixYMax;
        }
        EndItem();
    }
}


template <typename T>
void PlotDigital(const char* label_id, const T* xs, const T* ys, int count, ImPlotDigitalFlags flags, int offset, int stride) {
    GetterXY<IndexerIdx<T>,IndexerIdx<T>> getter(IndexerIdx<T>(xs,count,offset,stride),IndexerIdx<T>(ys,count,offset,stride),count);
    return PlotDigitalEx(label_id, getter, flags);
}
#define INSTANTIATE_MACRO(T) template IMPLOT_API void PlotDigital<T>(const char* label_id, const T* xs, const T* ys, int count, ImPlotDigitalFlags flags, int offset, int stride);
CALL_INSTANTIATE_FOR_NUMERIC_TYPES()
#undef INSTANTIATE_MACRO

// custom
void PlotDigitalG(const char* label_id, ImPlotGetter getter_func, void* data, int count, ImPlotDigitalFlags flags) {
    GetterFuncPtr getter(getter_func,data,count);
    return PlotDigitalEx(label_id, getter, flags);
}

//-----------------------------------------------------------------------------
// [SECTION] PlotImage
//-----------------------------------------------------------------------------

void PlotImage(const char* label_id, ImTextureID user_texture_id, const ImPlotPoint& bmin, const ImPlotPoint& bmax, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, ImPlotImageFlags) {
    if (BeginItemEx(label_id, FitterRect(bmin,bmax))) {
        ImU32 tint_col32 = ImGui::ColorConvertFloat4ToU32(tint_col);
        GetCurrentItem()->Color = tint_col32;
        ImDrawList& draw_list = *GetPlotDrawList();
        ImVec2 p1 = PlotToPixels(bmin.x, bmax.y,IMPLOT_AUTO,IMPLOT_AUTO);
        ImVec2 p2 = PlotToPixels(bmax.x, bmin.y,IMPLOT_AUTO,IMPLOT_AUTO);
        PushPlotClipRect();
        draw_list.AddImage(user_texture_id, p1, p2, uv0, uv1, tint_col32);
        PopPlotClipRect();
        EndItem();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] PlotText
//-----------------------------------------------------------------------------

void PlotText(const char* text, double x, double y, const ImVec2& pixel_offset, ImPlotTextFlags flags) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "PlotText() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    ImDrawList & draw_list = *GetPlotDrawList();
    PushPlotClipRect();
    ImU32 colTxt = GetStyleColorU32(ImPlotCol_InlayText);
    if (ImHasFlag(flags,ImPlotTextFlags_Vertical)) {
        ImVec2 siz = CalcTextSizeVertical(text) * 0.5f;
        ImVec2 ctr = siz * 0.5f;
        ImVec2 pos = PlotToPixels(ImPlotPoint(x,y),IMPLOT_AUTO,IMPLOT_AUTO) + ImVec2(-ctr.x, ctr.y) + pixel_offset;
        if (FitThisFrame() && !ImHasFlag(flags, ImPlotItemFlags_NoFit)) {
            FitPoint(PixelsToPlot(pos));
            FitPoint(PixelsToPlot(pos.x + siz.x, pos.y - siz.y));
        }
        AddTextVertical(&draw_list, pos, colTxt, text);
    }
    else {
        ImVec2 siz = ImGui::CalcTextSize(text);
        ImVec2 pos = PlotToPixels(ImPlotPoint(x,y),IMPLOT_AUTO,IMPLOT_AUTO) - siz * 0.5f + pixel_offset;
        if (FitThisFrame() && !ImHasFlag(flags, ImPlotItemFlags_NoFit)) {
            FitPoint(PixelsToPlot(pos));
            FitPoint(PixelsToPlot(pos+siz));
        }
        draw_list.AddText(pos, colTxt, text);
    }
    PopPlotClipRect();
}

//-----------------------------------------------------------------------------
// [SECTION] PlotDummy
//-----------------------------------------------------------------------------

void PlotDummy(const char* label_id, ImPlotDummyFlags flags) {
    if (BeginItem(label_id, flags, ImPlotCol_Line))
        EndItem();
}

} // namespace ImPlot
