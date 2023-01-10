// MIT License

// Copyright (c) 2022 Evan Pezent

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

/*

API BREAKING CHANGES
====================
Occasionally introducing changes that are breaking the API. We try to make the breakage minor and easy to fix.
Below is a change-log of API breaking changes only. If you are using one of the functions listed, expect to have to fix some code.
When you are not sure about a old symbol or function name, try using the Search/Find function of your IDE to look for comments or references in all implot files.
You can read releases logs https://github.com/epezent/implot/releases for more details.

- 2022/06/19 (0.14) - The signature of ColormapScale has changed to accommodate a new ImPlotColormapScaleFlags parameter
- 2022/06/17 (0.14) - **IMPORTANT** All PlotX functions now take an ImPlotX_Flags `flags` parameter. Where applicable, it is located before the existing `offset` and `stride` parameters.
                      If you were providing offset and stride values, you will need to update your function call to include a `flags` value. If you fail to do this, you will likely see
                      unexpected results or crashes without a compiler warning since these three are all default args. We apologize for the inconvenience, but this was a necessary evil.
                    - PlotBarsH has been removed; use PlotBars + ImPlotBarsFlags_Horizontal instead
                    - PlotErrorBarsH has been removed; use PlotErrorBars + ImPlotErrorBarsFlags_Horizontal
                    - PlotHistogram/PlotHistogram2D signatures changed; `cumulative`, `density`, and `outliers` options now specified via ImPlotHistogramFlags
                    - PlotPieChart signature changed; `normalize` option now specified via ImPlotPieChartFlags
                    - PlotText signature changes; `vertical` option now specified via `ImPlotTextFlags_Vertical`
                    - `PlotVLines` and `PlotHLines` replaced with `PlotInfLines` (+ ImPlotInfLinesFlags_Horizontal )
                    - arguments of ImPlotGetter have been reversed to be consistent with other API callbacks
                    - SetupAxisScale + ImPlotScale have replaced ImPlotAxisFlags_LogScale and ImPlotAxisFlags_Time flags
                    - ImPlotFormatters should now return an int indicating the size written
                    - the signature of ImPlotGetter has been reversed so that void* user_data is the last argument and consistent with other callbacks
- 2021/10/19 (0.13) - MAJOR API OVERHAUL! See #168 and #272
                    - TRIVIAL RENAME:
                      - ImPlotLimits                              -> ImPlotRect
                      - ImPlotYAxis_                              -> ImAxis_
                      - SetPlotYAxis                              -> SetAxis
                      - BeginDragDropTarget                       -> BeginDragDropTargetPlot
                      - BeginDragDropSource                       -> BeginDragDropSourcePlot
                      - ImPlotFlags_NoMousePos                    -> ImPlotFlags_NoMouseText
                      - SetNextPlotLimits                         -> SetNextAxesLimits
                      - SetMouseTextLocation                      -> SetupMouseText
                    - SIGNATURE MODIFIED:
                      - PixelsToPlot/PlotToPixels                 -> added optional X-Axis arg
                      - GetPlotMousePos                           -> added optional X-Axis arg
                      - GetPlotLimits                             -> added optional X-Axis arg
                      - GetPlotSelection                          -> added optional X-Axis arg
                      - DragLineX/Y/DragPoint                     -> now takes int id; removed labels (render with Annotation/Tag instead)
                    - REPLACED:
                      - IsPlotXAxisHovered/IsPlotXYAxisHovered    -> IsAxisHovered(ImAxis)
                      - BeginDragDropTargetX/BeginDragDropTargetY -> BeginDragDropTargetAxis(ImAxis)
                      - BeginDragDropSourceX/BeginDragDropSourceY -> BeginDragDropSourceAxis(ImAxis)
                      - ImPlotCol_XAxis, ImPlotCol_YAxis1, etc.   -> ImPlotCol_AxisText (push/pop this around SetupAxis to style individual axes)
                      - ImPlotCol_XAxisGrid, ImPlotCol_Y1AxisGrid -> ImPlotCol_AxisGrid (push/pop this around SetupAxis to style individual axes)
                      - SetNextPlotLimitsX/Y                      -> SetNextAxisLimits(ImAxis)
                      - LinkNextPlotLimits                        -> SetNextAxisLinks(ImAxis)
                      - FitNextPlotAxes                           -> SetNextAxisToFit(ImAxis)/SetNextAxesToFit
                      - SetLegendLocation                         -> SetupLegend
                      - ImPlotFlags_NoHighlight                   -> ImPlotLegendFlags_NoHighlight
                      - ImPlotOrientation                         -> ImPlotLegendFlags_Horizontal
                      - Annotate                                  -> Annotation
                    - REMOVED:
                      - GetPlotQuery, SetPlotQuery, IsPlotQueried -> use DragRect
                      - SetNextPlotTicksX, SetNextPlotTicksY      -> use SetupAxisTicks
                      - SetNextPlotFormatX, SetNextPlotFormatY    -> use SetupAxisFormat
                      - AnnotateClamped                           -> use Annotation(bool clamp = true)
                    - OBSOLETED:
                      - BeginPlot (original signature)            -> use simplified signature + Setup API
- 2021/07/30 (0.12) - The offset argument of `PlotXG` functions was been removed. Implement offsetting in your getter callback instead.
- 2021/03/08 (0.9)  - SetColormap and PushColormap(ImVec4*) were removed. Use AddColormap for custom colormap support. LerpColormap was changed to SampleColormap.
                      ShowColormapScale was changed to ColormapScale and requires additional arguments.
- 2021/03/07 (0.9)  - The signature of ShowColormapScale was modified to accept a ImVec2 size.
- 2021/02/28 (0.9)  - BeginLegendDragDropSource was changed to BeginDragDropSourceItem with a number of other drag and drop improvements.
- 2021/01/18 (0.9)  - The default behavior for opening context menus was change from double right-click to single right-click. ImPlotInputMap and related functions were moved
                      to implot_internal.h due to its immaturity.
- 2020/10/16 (0.8)  - ImPlotStyleVar_InfoPadding was changed to ImPlotStyleVar_MousePosPadding
- 2020/09/10 (0.8)  - The single array versions of PlotLine, PlotScatter, PlotStems, and PlotShaded were given additional arguments for x-scale and x0.
- 2020/09/07 (0.8)  - Plotting functions which accept a custom getter function pointer have been post-fixed with a G (e.g. PlotLineG)
- 2020/09/06 (0.7)  - Several flags under ImPlotFlags and ImPlotAxisFlags were inverted (e.g. ImPlotFlags_Legend -> ImPlotFlags_NoLegend) so that the default flagset
                      is simply 0. This more closely matches ImGui's style and makes it easier to enable non-default but commonly used flags (e.g. ImPlotAxisFlags_Time).
- 2020/08/28 (0.5)  - ImPlotMarker_ can no longer be combined with bitwise OR, |. This features caused unecessary slow-down, and almost no one used it.
- 2020/08/25 (0.5)  - ImPlotAxisFlags_Scientific was removed. Logarithmic axes automatically uses scientific notation.
- 2020/08/17 (0.5)  - PlotText was changed so that text is centered horizontally and vertically about the desired point.
- 2020/08/16 (0.5)  - An ImPlotContext must be explicitly created and destroyed now with `CreateContext` and `DestroyContext`. Previously, the context was statically initialized in this source file.
- 2020/06/13 (0.4)  - The flags `ImPlotAxisFlag_Adaptive` and `ImPlotFlags_Cull` were removed. Both are now done internally by default.
- 2020/06/03 (0.3)  - The signature and behavior of PlotPieChart was changed so that data with sum less than 1 can optionally be normalized. The label format can now be specified as well.
- 2020/06/01 (0.3)  - SetPalette was changed to `SetColormap` for consistency with other plotting libraries. `RestorePalette` was removed. Use `SetColormap(ImPlotColormap_Default)`.
- 2020/05/31 (0.3)  - Plot functions taking custom ImVec2* getters were removed. Use the ImPlotPoint* getter versions instead.
- 2020/05/29 (0.3)  - The signature of ImPlotLimits::Contains was changed to take two doubles instead of ImVec2
- 2020/05/16 (0.2)  - All plotting functions were reverted to being prefixed with "Plot" to maintain a consistent VerbNoun style. `Plot` was split into `PlotLine`
                      and `PlotScatter` (however, `PlotLine` can still be used to plot scatter points as `Plot` did before.). `Bar` is not `PlotBars`, to indicate
                      that multiple bars will be plotted.
- 2020/05/13 (0.2)  - `ImMarker` was change to `ImPlotMarker` and `ImAxisFlags` was changed to `ImPlotAxisFlags`.
- 2020/05/11 (0.2)  - `ImPlotFlags_Selection` was changed to `ImPlotFlags_BoxSelect`
- 2020/05/11 (0.2)  - The namespace ImGui:: was replaced with ImPlot::. As a result, the following additional changes were made:
                      - Functions that were prefixed or decorated with the word "Plot" have been truncated. E.g., `ImGui::PlotBars` is now just `ImPlot::Bar`.
                        It should be fairly obvious what was what.
                      - Some functions have been given names that would have otherwise collided with the ImGui namespace. This has been done to maintain a consistent
                        style with ImGui. E.g., 'ImGui::PushPlotStyleVar` is now 'ImPlot::PushStyleVar'.
- 2020/05/10 (0.2)  - The following function/struct names were changes:
                     - ImPlotRange       -> ImPlotLimits
                     - GetPlotRange()    -> GetPlotLimits()
                     - SetNextPlotRange  -> SetNextPlotLimits
                     - SetNextPlotRangeX -> SetNextPlotLimitsX
                     - SetNextPlotRangeY -> SetNextPlotLimitsY
- 2020/05/10 (0.2)  - Plot queries are pixel based by default. Query rects that maintain relative plot position have been removed. This was done to support multi-y-axis.

*/

#include "implot.h"
#include "implot_internal.h"

#include <stdlib.h>

// Support for pre-1.82 versions. Users on 1.82+ can use 0 (default) flags to mean "all corners" but in order to support older versions we are more explicit.
#if (IMGUI_VERSION_NUM < 18102) && !defined(ImDrawFlags_RoundCornersAll)
#define ImDrawFlags_RoundCornersAll ImDrawCornerFlags_All
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wformat-nonliteral"  // warning: format string is not a string literal
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wformat-nonliteral"    // warning: format not a string literal, format string not checked
#endif

// Global plot context
#ifndef GImPlot
ImPlotContext* GImPlot = NULL;
#endif

//-----------------------------------------------------------------------------
// Struct Implementations
//-----------------------------------------------------------------------------

ImPlotInputMap::ImPlotInputMap() {
    ImPlot::MapInputDefault(this);
}

ImPlotStyle::ImPlotStyle() {

    LineWeight         = 1;
    Marker             = ImPlotMarker_None;
    MarkerSize         = 4;
    MarkerWeight       = 1;
    FillAlpha          = 1;
    ErrorBarSize       = 5;
    ErrorBarWeight     = 1.5f;
    DigitalBitHeight   = 8;
    DigitalBitGap      = 4;

    PlotBorderSize     = 1;
    MinorAlpha         = 0.25f;
    MajorTickLen       = ImVec2(10,10);
    MinorTickLen       = ImVec2(5,5);
    MajorTickSize      = ImVec2(1,1);
    MinorTickSize      = ImVec2(1,1);
    MajorGridSize      = ImVec2(1,1);
    MinorGridSize      = ImVec2(1,1);
    PlotPadding        = ImVec2(10,10);
    LabelPadding       = ImVec2(5,5);
    LegendPadding      = ImVec2(10,10);
    LegendInnerPadding = ImVec2(5,5);
    LegendSpacing      = ImVec2(5,0);
    MousePosPadding    = ImVec2(10,10);
    AnnotationPadding  = ImVec2(2,2);
    FitPadding         = ImVec2(0,0);
    PlotDefaultSize    = ImVec2(400,300);
    PlotMinSize        = ImVec2(200,150);

    ImPlot::StyleColorsAuto(this);

    Colormap = ImPlotColormap_Deep;

    UseLocalTime     = false;
    Use24HourClock   = false;
    UseISO8601       = false;
}

//-----------------------------------------------------------------------------
// Style
//-----------------------------------------------------------------------------

namespace ImPlot {

const char* GetStyleColorName(ImPlotCol col) {
    static const char* col_names[ImPlotCol_COUNT] = {
        "Line",
        "Fill",
        "MarkerOutline",
        "MarkerFill",
        "ErrorBar",
        "FrameBg",
        "PlotBg",
        "PlotBorder",
        "LegendBg",
        "LegendBorder",
        "LegendText",
        "TitleText",
        "InlayText",
        "AxisText",
        "AxisGrid",
        "AxisTick",
        "AxisBg",
        "AxisBgHovered",
        "AxisBgActive",
        "Selection",
        "Crosshairs"
    };
    return col_names[col];
}

const char* GetMarkerName(ImPlotMarker marker) {
    switch (marker) {
        case ImPlotMarker_None:     return "None";
        case ImPlotMarker_Circle:   return "Circle";
        case ImPlotMarker_Square:   return "Square";
        case ImPlotMarker_Diamond:  return "Diamond";
        case ImPlotMarker_Up:       return "Up";
        case ImPlotMarker_Down:     return "Down";
        case ImPlotMarker_Left:     return "Left";
        case ImPlotMarker_Right:    return "Right";
        case ImPlotMarker_Cross:    return "Cross";
        case ImPlotMarker_Plus:     return "Plus";
        case ImPlotMarker_Asterisk: return "Asterisk";
        default:                    return "";
    }
}

ImVec4 GetAutoColor(ImPlotCol idx) {
    ImVec4 col(0,0,0,1);
    switch(idx) {
        case ImPlotCol_Line:          return col; // these are plot dependent!
        case ImPlotCol_Fill:          return col; // these are plot dependent!
        case ImPlotCol_MarkerOutline: return col; // these are plot dependent!
        case ImPlotCol_MarkerFill:    return col; // these are plot dependent!
        case ImPlotCol_ErrorBar:      return ImGui::GetStyleColorVec4(ImGuiCol_Text);
        case ImPlotCol_FrameBg:       return ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
        case ImPlotCol_PlotBg:        return ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        case ImPlotCol_PlotBorder:    return ImGui::GetStyleColorVec4(ImGuiCol_Border);
        case ImPlotCol_LegendBg:      return ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);
        case ImPlotCol_LegendBorder:  return GetStyleColorVec4(ImPlotCol_PlotBorder);
        case ImPlotCol_LegendText:    return GetStyleColorVec4(ImPlotCol_InlayText);
        case ImPlotCol_TitleText:     return ImGui::GetStyleColorVec4(ImGuiCol_Text);
        case ImPlotCol_InlayText:     return ImGui::GetStyleColorVec4(ImGuiCol_Text);
        case ImPlotCol_AxisText:      return ImGui::GetStyleColorVec4(ImGuiCol_Text);
        case ImPlotCol_AxisGrid:      return GetStyleColorVec4(ImPlotCol_AxisText) * ImVec4(1,1,1,0.25f);
        case ImPlotCol_AxisTick:      return GetStyleColorVec4(ImPlotCol_AxisGrid);
        case ImPlotCol_AxisBg:        return ImVec4(0,0,0,0);
        case ImPlotCol_AxisBgHovered: return ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
        case ImPlotCol_AxisBgActive:  return ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
        case ImPlotCol_Selection:     return ImVec4(1,1,0,1);
        case ImPlotCol_Crosshairs:    return GetStyleColorVec4(ImPlotCol_PlotBorder);
        default: return col;
    }
}

struct ImPlotStyleVarInfo {
    ImGuiDataType   Type;
    ImU32           Count;
    ImU32           Offset;
    void*           GetVarPtr(ImPlotStyle* style) const { return (void*)((unsigned char*)style + Offset); }
};

static const ImPlotStyleVarInfo GPlotStyleVarInfo[] =
{
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImPlotStyle, LineWeight)         }, // ImPlotStyleVar_LineWeight
    { ImGuiDataType_S32,   1, (ImU32)IM_OFFSETOF(ImPlotStyle, Marker)             }, // ImPlotStyleVar_Marker
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImPlotStyle, MarkerSize)         }, // ImPlotStyleVar_MarkerSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImPlotStyle, MarkerWeight)       }, // ImPlotStyleVar_MarkerWeight
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImPlotStyle, FillAlpha)          }, // ImPlotStyleVar_FillAlpha
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImPlotStyle, ErrorBarSize)       }, // ImPlotStyleVar_ErrorBarSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImPlotStyle, ErrorBarWeight)     }, // ImPlotStyleVar_ErrorBarWeight
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImPlotStyle, DigitalBitHeight)   }, // ImPlotStyleVar_DigitalBitHeight
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImPlotStyle, DigitalBitGap)      }, // ImPlotStyleVar_DigitalBitGap

    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImPlotStyle, PlotBorderSize)     }, // ImPlotStyleVar_PlotBorderSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImPlotStyle, MinorAlpha)         }, // ImPlotStyleVar_MinorAlpha
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, MajorTickLen)       }, // ImPlotStyleVar_MajorTickLen
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, MinorTickLen)       }, // ImPlotStyleVar_MinorTickLen
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, MajorTickSize)      }, // ImPlotStyleVar_MajorTickSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, MinorTickSize)      }, // ImPlotStyleVar_MinorTickSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, MajorGridSize)      }, // ImPlotStyleVar_MajorGridSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, MinorGridSize)      }, // ImPlotStyleVar_MinorGridSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, PlotPadding)        }, // ImPlotStyleVar_PlotPadding
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, LabelPadding)       }, // ImPlotStyleVar_LabelPaddine
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, LegendPadding)      }, // ImPlotStyleVar_LegendPadding
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, LegendInnerPadding) }, // ImPlotStyleVar_LegendInnerPadding
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, LegendSpacing)      }, // ImPlotStyleVar_LegendSpacing

    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, MousePosPadding)    }, // ImPlotStyleVar_MousePosPadding
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, AnnotationPadding)  }, // ImPlotStyleVar_AnnotationPadding
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, FitPadding)         }, // ImPlotStyleVar_FitPadding
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, PlotDefaultSize)    }, // ImPlotStyleVar_PlotDefaultSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImPlotStyle, PlotMinSize)        }  // ImPlotStyleVar_PlotMinSize
};

static const ImPlotStyleVarInfo* GetPlotStyleVarInfo(ImPlotStyleVar idx) {
    IM_ASSERT(idx >= 0 && idx < ImPlotStyleVar_COUNT);
    IM_ASSERT(IM_ARRAYSIZE(GPlotStyleVarInfo) == ImPlotStyleVar_COUNT);
    return &GPlotStyleVarInfo[idx];
}

//-----------------------------------------------------------------------------
// Generic Helpers
//-----------------------------------------------------------------------------

void AddTextVertical(ImDrawList *DrawList, ImVec2 pos, ImU32 col, const char *text_begin, const char* text_end) {
    // the code below is based loosely on ImFont::RenderText
    if (!text_end)
        text_end = text_begin + strlen(text_begin);
    ImGuiContext& g = *GImGui;
    ImFont* font = g.Font;
    // Align to be pixel perfect
    pos.x = IM_FLOOR(pos.x);
    pos.y = IM_FLOOR(pos.y);
    const float scale = g.FontSize / font->FontSize;
    const char* s = text_begin;
    int chars_exp = (int)(text_end - s);
    int chars_rnd = 0;
    const int vtx_count_max = chars_exp * 4;
    const int idx_count_max = chars_exp * 6;
    DrawList->PrimReserve(idx_count_max, vtx_count_max);
    while (s < text_end) {
        unsigned int c = (unsigned int)*s;
        if (c < 0x80) {
            s += 1;
        }
        else {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0) // Malformed UTF-8?
                break;
        }
        const ImFontGlyph * glyph = font->FindGlyph((ImWchar)c);
        if (glyph == NULL) {
            continue;
        }
        DrawList->PrimQuadUV(pos + ImVec2(glyph->Y0, -glyph->X0) * scale, pos + ImVec2(glyph->Y0, -glyph->X1) * scale,
                             pos + ImVec2(glyph->Y1, -glyph->X1) * scale, pos + ImVec2(glyph->Y1, -glyph->X0) * scale,
                             ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V0),
                             ImVec2(glyph->U1, glyph->V1), ImVec2(glyph->U0, glyph->V1),
                             col);
        pos.y -= glyph->AdvanceX * scale;
        chars_rnd++;
    }
    // Give back unused vertices
    int chars_skp = chars_exp-chars_rnd;
    DrawList->PrimUnreserve(chars_skp*6, chars_skp*4);
}

void AddTextCentered(ImDrawList* DrawList, ImVec2 top_center, ImU32 col, const char* text_begin, const char* text_end) {
    float txt_ht = ImGui::GetTextLineHeight();
    const char* title_end = ImGui::FindRenderedTextEnd(text_begin, text_end);
    ImVec2 text_size;
    float  y = 0;
    while (const char* tmp = (const char*)memchr(text_begin, '\n', title_end-text_begin)) {
        text_size = ImGui::CalcTextSize(text_begin,tmp,true);
        DrawList->AddText(ImVec2(top_center.x - text_size.x * 0.5f, top_center.y+y),col,text_begin,tmp);
        text_begin = tmp + 1;
        y += txt_ht;
    }
    text_size = ImGui::CalcTextSize(text_begin,title_end,true);
    DrawList->AddText(ImVec2(top_center.x - text_size.x * 0.5f, top_center.y+y),col,text_begin,title_end);
}

double NiceNum(double x, bool round) {
    double f;
    double nf;
    int expv = (int)floor(ImLog10(x));
    f = x / ImPow(10.0, (double)expv);
    if (round)
        if (f < 1.5)
            nf = 1;
        else if (f < 3)
            nf = 2;
        else if (f < 7)
            nf = 5;
        else
            nf = 10;
    else if (f <= 1)
        nf = 1;
    else if (f <= 2)
        nf = 2;
    else if (f <= 5)
        nf = 5;
    else
        nf = 10;
    return nf * ImPow(10.0, expv);
}

//-----------------------------------------------------------------------------
// Context Utils
//-----------------------------------------------------------------------------

void SetImGuiContext(ImGuiContext* ctx) {
    ImGui::SetCurrentContext(ctx);
}

ImPlotContext* CreateContext() {
    ImPlotContext* ctx = IM_NEW(ImPlotContext)();
    Initialize(ctx);
    if (GImPlot == NULL)
        SetCurrentContext(ctx);
    return ctx;
}

void DestroyContext(ImPlotContext* ctx) {
    if (ctx == NULL)
        ctx = GImPlot;
    if (GImPlot == ctx)
        SetCurrentContext(NULL);
    IM_DELETE(ctx);
}

ImPlotContext* GetCurrentContext() {
    return GImPlot;
}

void SetCurrentContext(ImPlotContext* ctx) {
    GImPlot = ctx;
}

#define IMPLOT_APPEND_CMAP(name, qual) ctx->ColormapData.Append(#name, name, sizeof(name)/sizeof(ImU32), qual)
#define IM_RGB(r,g,b) IM_COL32(r,g,b,255)

void Initialize(ImPlotContext* ctx) {
    ResetCtxForNextPlot(ctx);
    ResetCtxForNextAlignedPlots(ctx);
    ResetCtxForNextSubplot(ctx);

    const ImU32 Deep[]     = {4289753676, 4283598045, 4285048917, 4283584196, 4289950337, 4284512403, 4291005402, 4287401100, 4285839820, 4291671396                        };
    const ImU32 Dark[]     = {4280031972, 4290281015, 4283084621, 4288892568, 4278222847, 4281597951, 4280833702, 4290740727, 4288256409                                    };
    const ImU32 Pastel[]   = {4289639675, 4293119411, 4291161036, 4293184478, 4289124862, 4291624959, 4290631909, 4293712637, 4294111986                                    };
    const ImU32 Paired[]   = {4293119554, 4290017311, 4287291314, 4281114675, 4288256763, 4280031971, 4285513725, 4278222847, 4292260554, 4288298346, 4288282623, 4280834481};
    const ImU32 Viridis[]  = {4283695428, 4285867080, 4287054913, 4287455029, 4287526954, 4287402273, 4286883874, 4285579076, 4283552122, 4280737725, 4280674301            };
    const ImU32 Plasma[]   = {4287039501, 4288480321, 4289200234, 4288941455, 4287638193, 4286072780, 4284638433, 4283139314, 4281771772, 4280667900, 4280416752            };
    const ImU32 Hot[]      = {4278190144, 4278190208, 4278190271, 4278190335, 4278206719, 4278223103, 4278239231, 4278255615, 4283826175, 4289396735, 4294967295            };
    const ImU32 Cool[]     = {4294967040, 4294960666, 4294954035, 4294947661, 4294941030, 4294934656, 4294928025, 4294921651, 4294915020, 4294908646, 4294902015            };
    const ImU32 Pink[]     = {4278190154, 4282532475, 4284308894, 4285690554, 4286879686, 4287870160, 4288794330, 4289651940, 4291685869, 4293392118, 4294967295            };
    const ImU32 Jet[]      = {4289331200, 4294901760, 4294923520, 4294945280, 4294967040, 4289396565, 4283826090, 4278255615, 4278233855, 4278212095, 4278190335            };
    const ImU32 Twilight[] = {IM_RGB(226,217,226),IM_RGB(166,191,202),IM_RGB(109,144,192),IM_RGB(95,88,176),IM_RGB(83,30,124),IM_RGB(47,20,54),IM_RGB(100,25,75),IM_RGB(159,60,80),IM_RGB(192,117,94),IM_RGB(208,179,158),IM_RGB(226,217,226)};
    const ImU32 RdBu[]     = {IM_RGB(103,0,31),IM_RGB(178,24,43),IM_RGB(214,96,77),IM_RGB(244,165,130),IM_RGB(253,219,199),IM_RGB(247,247,247),IM_RGB(209,229,240),IM_RGB(146,197,222),IM_RGB(67,147,195),IM_RGB(33,102,172),IM_RGB(5,48,97)};
    const ImU32 BrBG[]     = {IM_RGB(84,48,5),IM_RGB(140,81,10),IM_RGB(191,129,45),IM_RGB(223,194,125),IM_RGB(246,232,195),IM_RGB(245,245,245),IM_RGB(199,234,229),IM_RGB(128,205,193),IM_RGB(53,151,143),IM_RGB(1,102,94),IM_RGB(0,60,48)};
    const ImU32 PiYG[]     = {IM_RGB(142,1,82),IM_RGB(197,27,125),IM_RGB(222,119,174),IM_RGB(241,182,218),IM_RGB(253,224,239),IM_RGB(247,247,247),IM_RGB(230,245,208),IM_RGB(184,225,134),IM_RGB(127,188,65),IM_RGB(77,146,33),IM_RGB(39,100,25)};
    const ImU32 Spectral[] = {IM_RGB(158,1,66),IM_RGB(213,62,79),IM_RGB(244,109,67),IM_RGB(253,174,97),IM_RGB(254,224,139),IM_RGB(255,255,191),IM_RGB(230,245,152),IM_RGB(171,221,164),IM_RGB(102,194,165),IM_RGB(50,136,189),IM_RGB(94,79,162)};
    const ImU32 Greys[]    = {IM_COL32_WHITE, IM_COL32_BLACK                                                                                                                };

    IMPLOT_APPEND_CMAP(Deep, true);
    IMPLOT_APPEND_CMAP(Dark, true);
    IMPLOT_APPEND_CMAP(Pastel, true);
    IMPLOT_APPEND_CMAP(Paired, true);
    IMPLOT_APPEND_CMAP(Viridis, false);
    IMPLOT_APPEND_CMAP(Plasma, false);
    IMPLOT_APPEND_CMAP(Hot, false);
    IMPLOT_APPEND_CMAP(Cool, false);
    IMPLOT_APPEND_CMAP(Pink, false);
    IMPLOT_APPEND_CMAP(Jet, false);
    IMPLOT_APPEND_CMAP(Twilight, false);
    IMPLOT_APPEND_CMAP(RdBu, false);
    IMPLOT_APPEND_CMAP(BrBG, false);
    IMPLOT_APPEND_CMAP(PiYG, false);
    IMPLOT_APPEND_CMAP(Spectral, false);
    IMPLOT_APPEND_CMAP(Greys, false);
}

void ResetCtxForNextPlot(ImPlotContext* ctx) {
    // end child window if it was made
    if (ctx->ChildWindowMade)
        ImGui::EndChild();
    ctx->ChildWindowMade = false;
    // reset the next plot/item data
    ctx->NextPlotData.Reset();
    ctx->NextItemData.Reset();
    // reset labels
    ctx->Annotations.Reset();
    ctx->Tags.Reset();
    // reset extents/fit
    ctx->OpenContextThisFrame = false;
    // reset digital plot items count
    ctx->DigitalPlotItemCnt = 0;
    ctx->DigitalPlotOffset = 0;
    // nullify plot
    ctx->CurrentPlot  = NULL;
    ctx->CurrentItem  = NULL;
    ctx->PreviousItem = NULL;
}

void ResetCtxForNextAlignedPlots(ImPlotContext* ctx) {
    ctx->CurrentAlignmentH = NULL;
    ctx->CurrentAlignmentV = NULL;
}

void ResetCtxForNextSubplot(ImPlotContext* ctx) {
    ctx->CurrentSubplot      = NULL;
    ctx->CurrentAlignmentH   = NULL;
    ctx->CurrentAlignmentV   = NULL;
}

//-----------------------------------------------------------------------------
// Plot Utils
//-----------------------------------------------------------------------------

ImPlotPlot* GetPlot(const char* title) {
    ImGuiWindow*   Window = GImGui->CurrentWindow;
    const ImGuiID  ID     = Window->GetID(title);
    return GImPlot->Plots.GetByKey(ID);
}

ImPlotPlot* GetCurrentPlot() {
    return GImPlot->CurrentPlot;
}

void BustPlotCache() {
    GImPlot->Plots.Clear();
    GImPlot->Subplots.Clear();
}

//-----------------------------------------------------------------------------
// Legend Utils
//-----------------------------------------------------------------------------

ImVec2 GetLocationPos(const ImRect& outer_rect, const ImVec2& inner_size, ImPlotLocation loc, const ImVec2& pad) {
    ImVec2 pos;
    if (ImHasFlag(loc, ImPlotLocation_West) && !ImHasFlag(loc, ImPlotLocation_East))
        pos.x = outer_rect.Min.x + pad.x;
    else if (!ImHasFlag(loc, ImPlotLocation_West) && ImHasFlag(loc, ImPlotLocation_East))
        pos.x = outer_rect.Max.x - pad.x - inner_size.x;
    else
        pos.x = outer_rect.GetCenter().x - inner_size.x * 0.5f;
    // legend reference point y
    if (ImHasFlag(loc, ImPlotLocation_North) && !ImHasFlag(loc, ImPlotLocation_South))
        pos.y = outer_rect.Min.y + pad.y;
    else if (!ImHasFlag(loc, ImPlotLocation_North) && ImHasFlag(loc, ImPlotLocation_South))
        pos.y = outer_rect.Max.y - pad.y - inner_size.y;
    else
        pos.y = outer_rect.GetCenter().y - inner_size.y * 0.5f;
    pos.x = IM_ROUND(pos.x);
    pos.y = IM_ROUND(pos.y);
    return pos;
}

ImVec2 CalcLegendSize(ImPlotItemGroup& items, const ImVec2& pad, const ImVec2& spacing, bool vertical) {
    // vars
    const int   nItems      = items.GetLegendCount();
    const float txt_ht      = ImGui::GetTextLineHeight();
    const float icon_size   = txt_ht;
    // get label max width
    float max_label_width = 0;
    float sum_label_width = 0;
    for (int i = 0; i < nItems; ++i) {
        const char* label       = items.GetLegendLabel(i);
        const float label_width = ImGui::CalcTextSize(label, NULL, true).x;
        max_label_width         = label_width > max_label_width ? label_width : max_label_width;
        sum_label_width        += label_width;
    }
    // calc legend size
    const ImVec2 legend_size = vertical ?
                               ImVec2(pad.x * 2 + icon_size + max_label_width, pad.y * 2 + nItems * txt_ht + (nItems - 1) * spacing.y) :
                               ImVec2(pad.x * 2 + icon_size * nItems + sum_label_width + (nItems - 1) * spacing.x, pad.y * 2 + txt_ht);
    return legend_size;
}

int LegendSortingComp(const void* _a, const void* _b) {
    ImPlotItemGroup* items = GImPlot->SortItems;
    const int a = *(const int*)_a;
    const int b = *(const int*)_b;
    const char* label_a = items->GetLegendLabel(a);
    const char* label_b = items->GetLegendLabel(b);
    return strcmp(label_a,label_b);
}

bool ShowLegendEntries(ImPlotItemGroup& items, const ImRect& legend_bb, bool hovered, const ImVec2& pad, const ImVec2& spacing, bool vertical, ImDrawList& DrawList) {
    // vars
    const float txt_ht      = ImGui::GetTextLineHeight();
    const float icon_size   = txt_ht;
    const float icon_shrink = 2;
    ImU32 col_txt           = GetStyleColorU32(ImPlotCol_LegendText);
    ImU32  col_txt_dis      = ImAlphaU32(col_txt, 0.25f);
    // render each legend item
    float sum_label_width = 0;
    bool any_item_hovered = false;

    const int num_items = items.GetLegendCount();
    if (num_items < 1)
        return hovered;
    // build render order
    ImVector<int>& indices = GImPlot->TempInt1;
    indices.resize(num_items);
    for (int i = 0; i < num_items; ++i)
        indices[i] = i;
    if (ImHasFlag(items.Legend.Flags, ImPlotLegendFlags_Sort) && num_items > 1) {
        GImPlot->SortItems = &items;
        qsort(indices.Data, num_items, sizeof(int), LegendSortingComp);
    }
    // render
    for (int i = 0; i < num_items; ++i) {
        const int idx           = indices[i];
        ImPlotItem* item        = items.GetLegendItem(idx);
        const char* label       = items.GetLegendLabel(idx);
        const float label_width = ImGui::CalcTextSize(label, NULL, true).x;
        const ImVec2 top_left   = vertical ?
                                  legend_bb.Min + pad + ImVec2(0, i * (txt_ht + spacing.y)) :
                                  legend_bb.Min + pad + ImVec2(i * (icon_size + spacing.x) + sum_label_width, 0);
        sum_label_width        += label_width;
        ImRect icon_bb;
        icon_bb.Min = top_left + ImVec2(icon_shrink,icon_shrink);
        icon_bb.Max = top_left + ImVec2(icon_size - icon_shrink, icon_size - icon_shrink);
        ImRect label_bb;
        label_bb.Min = top_left;
        label_bb.Max = top_left + ImVec2(label_width + icon_size, icon_size);
        ImU32 col_txt_hl;
        ImU32 col_item = ImAlphaU32(item->Color,1);

        ImRect button_bb(icon_bb.Min, label_bb.Max);

        ImGui::KeepAliveID(item->ID);

        bool item_hov = false;
        bool item_hld = false;
        bool item_clk = ImHasFlag(items.Legend.Flags, ImPlotLegendFlags_NoButtons)
                      ? false
                      : ImGui::ButtonBehavior(button_bb, item->ID, &item_hov, &item_hld);

        if (item_clk)
            item->Show = !item->Show;


        const bool can_hover = (item_hov)
                             && (!ImHasFlag(items.Legend.Flags, ImPlotLegendFlags_NoHighlightItem)
                             || !ImHasFlag(items.Legend.Flags, ImPlotLegendFlags_NoHighlightAxis));

        if (can_hover) {
            item->LegendHoverRect.Min = icon_bb.Min;
            item->LegendHoverRect.Max = label_bb.Max;
            item->LegendHovered = true;
            col_txt_hl = ImMixU32(col_txt, col_item, 64);
            any_item_hovered = true;
        }
        else {
            col_txt_hl = ImGui::GetColorU32(col_txt);
        }
        ImU32 col_icon;
        if (item_hld)
            col_icon = item->Show ? ImAlphaU32(col_item,0.5f) : ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.5f);
        else if (item_hov)
            col_icon = item->Show ? ImAlphaU32(col_item,0.75f) : ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.75f);
        else
            col_icon = item->Show ? col_item : col_txt_dis;

        DrawList.AddRectFilled(icon_bb.Min, icon_bb.Max, col_icon);
        const char* text_display_end = ImGui::FindRenderedTextEnd(label, NULL);
        if (label != text_display_end)
            DrawList.AddText(top_left + ImVec2(icon_size, 0), item->Show ? col_txt_hl  : col_txt_dis, label, text_display_end);
    }
    return hovered && !any_item_hovered;
}

//-----------------------------------------------------------------------------
// Locators
//-----------------------------------------------------------------------------

static const float TICK_FILL_X = 0.8f;
static const float TICK_FILL_Y = 1.0f;

void Locator_Default(ImPlotTicker& ticker, const ImPlotRange& range, float pixels, bool vertical, ImPlotFormatter formatter, void* formatter_data) {
    if (range.Min == range.Max)
        return;
    const int nMinor        = 10;
    const int nMajor        = ImMax(2, (int)IM_ROUND(pixels / (vertical ? 300.0f : 400.0f)));
    const double nice_range = NiceNum(range.Size() * 0.99, false);
    const double interval   = NiceNum(nice_range / (nMajor - 1), true);
    const double graphmin   = floor(range.Min / interval) * interval;
    const double graphmax   = ceil(range.Max / interval) * interval;
    bool first_major_set    = false;
    int  first_major_idx    = 0;
    const int idx0 = ticker.TickCount(); // ticker may have user custom ticks
    ImVec2 total_size(0,0);
    for (double major = graphmin; major < graphmax + 0.5 * interval; major += interval) {
        // is this zero? combat zero formatting issues
        if (major-interval < 0 && major+interval > 0)
            major = 0;
        if (range.Contains(major)) {
            if (!first_major_set) {
                first_major_idx = ticker.TickCount();
                first_major_set = true;
            }
            total_size += ticker.AddTick(major, true, 0, true, formatter, formatter_data).LabelSize;
        }
        for (int i = 1; i < nMinor; ++i) {
            double minor = major + i * interval / nMinor;
            if (range.Contains(minor)) {
                total_size += ticker.AddTick(minor, false, 0, true, formatter, formatter_data).LabelSize;
            }
        }
    }
    // prune if necessary
    if ((!vertical && total_size.x > pixels*TICK_FILL_X) || (vertical && total_size.y > pixels*TICK_FILL_Y)) {
        for (int i = first_major_idx-1; i >= idx0; i -= 2)
            ticker.Ticks[i].ShowLabel = false;
        for (int i = first_major_idx+1; i < ticker.TickCount(); i += 2)
            ticker.Ticks[i].ShowLabel = false;
    }
}

bool CalcLogarithmicExponents(const ImPlotRange& range, float pix, bool vertical, int& exp_min, int& exp_max, int& exp_step) {
    if (range.Min * range.Max > 0) {
        const int nMajor = vertical ? ImMax(2, (int)IM_ROUND(pix * 0.02f)) : ImMax(2, (int)IM_ROUND(pix * 0.01f)); // TODO: magic numbers
        double log_min = ImLog10(ImAbs(range.Min));
        double log_max = ImLog10(ImAbs(range.Max));
        double log_a = ImMin(log_min,log_max);
        double log_b = ImMax(log_min,log_max);
        exp_step  = ImMax(1,(int)(log_b - log_a) / nMajor);
        exp_min   = (int)log_a;
        exp_max   = (int)log_b;
        if (exp_step != 1) {
            while(exp_step % 3 != 0)       exp_step++; // make step size multiple of three
            while(exp_min % exp_step != 0) exp_min--;  // decrease exp_min until exp_min + N * exp_step will be 0
        }
        return true;
    }
    return false;
}

void AddTicksLogarithmic(const ImPlotRange& range, int exp_min, int exp_max, int exp_step, ImPlotTicker& ticker, ImPlotFormatter formatter, void* data) {
    const double sign = ImSign(range.Max);
    for (int e = exp_min - exp_step; e < (exp_max + exp_step); e += exp_step) {
        double major1 = sign*ImPow(10, (double)(e));
        double major2 = sign*ImPow(10, (double)(e + 1));
        double interval = (major2 - major1) / 9;
        if (major1 >= (range.Min - DBL_EPSILON) && major1 <= (range.Max + DBL_EPSILON))
            ticker.AddTick(major1, true, 0, true, formatter, data);
        for (int j = 0; j < exp_step; ++j) {
            major1 = sign*ImPow(10, (double)(e+j));
            major2 = sign*ImPow(10, (double)(e+j+1));
            interval = (major2 - major1) / 9;
            for (int i = 1; i < (9 + (int)(j < (exp_step - 1))); ++i) {
                double minor = major1 + i * interval;
                if (minor >= (range.Min - DBL_EPSILON) && minor <= (range.Max + DBL_EPSILON))
                    ticker.AddTick(minor, false, 0, false, formatter, data);
            }
        }
    }
}

void Locator_Log10(ImPlotTicker& ticker, const ImPlotRange& range, float pixels, bool vertical, ImPlotFormatter formatter, void* formatter_data) {
    int exp_min, exp_max, exp_step;
    if (CalcLogarithmicExponents(range, pixels, vertical, exp_min, exp_max, exp_step))
        AddTicksLogarithmic(range, exp_min, exp_max, exp_step, ticker, formatter, formatter_data);
}

float CalcSymLogPixel(double plt, const ImPlotRange& range, float pixels) {
    double scaleToPixels = pixels / range.Size();
    double scaleMin      = TransformForward_SymLog(range.Min,NULL);
    double scaleMax      = TransformForward_SymLog(range.Max,NULL);
    double s             = TransformForward_SymLog(plt, NULL);
    double t             = (s - scaleMin) / (scaleMax - scaleMin);
    plt                  = range.Min + range.Size() * t;

    return (float)(0 + scaleToPixels * (plt - range.Min));
}

void Locator_SymLog(ImPlotTicker& ticker, const ImPlotRange& range, float pixels, bool vertical, ImPlotFormatter formatter, void* formatter_data) {
    if (range.Min >= -1 && range.Max <= 1) {
        Locator_Default(ticker, range, pixels, vertical, formatter, formatter_data);
    }
    else if (range.Min * range.Max < 0) { // cross zero
        const float pix_min = 0;
        const float pix_max = pixels;
        const float pix_p1  = CalcSymLogPixel(1, range, pixels);
        const float pix_n1  = CalcSymLogPixel(-1, range, pixels);
        int exp_min_p, exp_max_p, exp_step_p;
        int exp_min_n, exp_max_n, exp_step_n;
        CalcLogarithmicExponents(ImPlotRange(1,range.Max), ImAbs(pix_max-pix_p1),vertical,exp_min_p,exp_max_p,exp_step_p);
        CalcLogarithmicExponents(ImPlotRange(range.Min,-1),ImAbs(pix_n1-pix_min),vertical,exp_min_n,exp_max_n,exp_step_n);
        int exp_step = ImMax(exp_step_n, exp_step_p);
        ticker.AddTick(0,true,0,true,formatter,formatter_data);
        AddTicksLogarithmic(ImPlotRange(1,range.Max), exp_min_p,exp_max_p,exp_step,ticker,formatter,formatter_data);
        AddTicksLogarithmic(ImPlotRange(range.Min,-1),exp_min_n,exp_max_n,exp_step,ticker,formatter,formatter_data);
    }
    else {
        Locator_Log10(ticker, range, pixels, vertical, formatter, formatter_data);
    }
}

void AddTicksCustom(const double* values, const char* const labels[], int n, ImPlotTicker& ticker, ImPlotFormatter formatter, void* data) {
    for (int i = 0; i < n; ++i) {
        if (labels != NULL)
            ticker.AddTick(values[i], false, 0, true, labels[i]);
        else
            ticker.AddTick(values[i], false, 0, true, formatter, data);
    }
}

//-----------------------------------------------------------------------------
// Time Ticks and Utils
//-----------------------------------------------------------------------------

// this may not be thread safe?
static const double TimeUnitSpans[ImPlotTimeUnit_COUNT] = {
    0.000001,
    0.001,
    1,
    60,
    3600,
    86400,
    2629800,
    31557600
};

inline ImPlotTimeUnit GetUnitForRange(double range) {
    static double cutoffs[ImPlotTimeUnit_COUNT] = {0.001, 1, 60, 3600, 86400, 2629800, 31557600, IMPLOT_MAX_TIME};
    for (int i = 0; i < ImPlotTimeUnit_COUNT; ++i) {
        if (range <= cutoffs[i])
            return (ImPlotTimeUnit)i;
    }
    return ImPlotTimeUnit_Yr;
}

inline int LowerBoundStep(int max_divs, const int* divs, const int* step, int size) {
    if (max_divs < divs[0])
        return 0;
    for (int i = 1; i < size; ++i) {
        if (max_divs < divs[i])
            return step[i-1];
    }
    return step[size-1];
}

inline int GetTimeStep(int max_divs, ImPlotTimeUnit unit) {
    if (unit == ImPlotTimeUnit_Ms || unit == ImPlotTimeUnit_Us) {
        static const int step[] = {500,250,200,100,50,25,20,10,5,2,1};
        static const int divs[] = {2,4,5,10,20,40,50,100,200,500,1000};
        return LowerBoundStep(max_divs, divs, step, 11);
    }
    if (unit == ImPlotTimeUnit_S || unit == ImPlotTimeUnit_Min) {
        static const int step[] = {30,15,10,5,1};
        static const int divs[] = {2,4,6,12,60};
        return LowerBoundStep(max_divs, divs, step, 5);
    }
    else if (unit == ImPlotTimeUnit_Hr) {
        static const int step[] = {12,6,3,2,1};
        static const int divs[] = {2,4,8,12,24};
        return LowerBoundStep(max_divs, divs, step, 5);
    }
    else if (unit == ImPlotTimeUnit_Day) {
        static const int step[] = {14,7,2,1};
        static const int divs[] = {2,4,14,28};
        return LowerBoundStep(max_divs, divs, step, 4);
    }
    else if (unit == ImPlotTimeUnit_Mo) {
        static const int step[] = {6,3,2,1};
        static const int divs[] = {2,4,6,12};
        return LowerBoundStep(max_divs, divs, step, 4);
    }
    return 0;
}

ImPlotTime MkGmtTime(struct tm *ptm) {
    ImPlotTime t;
#ifdef _WIN32
    t.S = _mkgmtime(ptm);
#else
    t.S = timegm(ptm);
#endif
    if (t.S < 0)
        t.S = 0;
    return t;
}

tm* GetGmtTime(const ImPlotTime& t, tm* ptm)
{
#ifdef _WIN32
  if (gmtime_s(ptm, &t.S) == 0)
    return ptm;
  else
    return NULL;
#else
  return gmtime_r(&t.S, ptm);
#endif
}

ImPlotTime MkLocTime(struct tm *ptm) {
    ImPlotTime t;
    t.S = mktime(ptm);
    if (t.S < 0)
        t.S = 0;
    return t;
}

tm* GetLocTime(const ImPlotTime& t, tm* ptm) {
#ifdef _WIN32
  if (localtime_s(ptm, &t.S) == 0)
    return ptm;
  else
    return NULL;
#else
    return localtime_r(&t.S, ptm);
#endif
}

inline ImPlotTime MkTime(struct tm *ptm) {
    if (GetStyle().UseLocalTime)
        return MkLocTime(ptm);
    else
        return MkGmtTime(ptm);
}

inline tm* GetTime(const ImPlotTime& t, tm* ptm) {
    if (GetStyle().UseLocalTime)
        return GetLocTime(t,ptm);
    else
        return GetGmtTime(t,ptm);
}

ImPlotTime MakeTime(int year, int month, int day, int hour, int min, int sec, int us) {
    tm& Tm = GImPlot->Tm;

    int yr = year - 1900;
    if (yr < 0)
        yr = 0;

    sec  = sec + us / 1000000;
    us   = us % 1000000;

    Tm.tm_sec  = sec;
    Tm.tm_min  = min;
    Tm.tm_hour = hour;
    Tm.tm_mday = day;
    Tm.tm_mon  = month;
    Tm.tm_year = yr;

    ImPlotTime t = MkTime(&Tm);

    t.Us = us;
    return t;
}

int GetYear(const ImPlotTime& t) {
    tm& Tm = GImPlot->Tm;
    GetTime(t, &Tm);
    return Tm.tm_year + 1900;
}

ImPlotTime AddTime(const ImPlotTime& t, ImPlotTimeUnit unit, int count) {
    tm& Tm = GImPlot->Tm;
    ImPlotTime t_out = t;
    switch(unit) {
        case ImPlotTimeUnit_Us:  t_out.Us += count;         break;
        case ImPlotTimeUnit_Ms:  t_out.Us += count * 1000;  break;
        case ImPlotTimeUnit_S:   t_out.S  += count;         break;
        case ImPlotTimeUnit_Min: t_out.S  += count * 60;    break;
        case ImPlotTimeUnit_Hr:  t_out.S  += count * 3600;  break;
        case ImPlotTimeUnit_Day: t_out.S  += count * 86400; break;
        case ImPlotTimeUnit_Mo:  for (int i = 0; i < abs(count); ++i) {
                                     GetTime(t_out, &Tm);
                                     if (count > 0)
                                        t_out.S += 86400 * GetDaysInMonth(Tm.tm_year + 1900, Tm.tm_mon);
                                     else if (count < 0)
                                        t_out.S -= 86400 * GetDaysInMonth(Tm.tm_year + 1900 - (Tm.tm_mon == 0 ? 1 : 0), Tm.tm_mon == 0 ? 11 : Tm.tm_mon - 1); // NOT WORKING
                                 }
                                 break;
        case ImPlotTimeUnit_Yr:  for (int i = 0; i < abs(count); ++i) {
                                    if (count > 0)
                                        t_out.S += 86400 * (365 + (int)IsLeapYear(GetYear(t_out)));
                                    else if (count < 0)
                                        t_out.S -= 86400 * (365 + (int)IsLeapYear(GetYear(t_out) - 1));
                                    // this is incorrect if leap year and we are past Feb 28
                                 }
                                 break;
        default:                 break;
    }
    t_out.RollOver();
    return t_out;
}

ImPlotTime FloorTime(const ImPlotTime& t, ImPlotTimeUnit unit) {
    GetTime(t, &GImPlot->Tm);
    switch (unit) {
        case ImPlotTimeUnit_S:   return ImPlotTime(t.S, 0);
        case ImPlotTimeUnit_Ms:  return ImPlotTime(t.S, (t.Us / 1000) * 1000);
        case ImPlotTimeUnit_Us:  return t;
        case ImPlotTimeUnit_Yr:  GImPlot->Tm.tm_mon  = 0; // fall-through
        case ImPlotTimeUnit_Mo:  GImPlot->Tm.tm_mday = 1; // fall-through
        case ImPlotTimeUnit_Day: GImPlot->Tm.tm_hour = 0; // fall-through
        case ImPlotTimeUnit_Hr:  GImPlot->Tm.tm_min  = 0; // fall-through
        case ImPlotTimeUnit_Min: GImPlot->Tm.tm_sec  = 0; break;
        default:                 return t;
    }
    return MkTime(&GImPlot->Tm);
}

ImPlotTime CeilTime(const ImPlotTime& t, ImPlotTimeUnit unit) {
    return AddTime(FloorTime(t, unit), unit, 1);
}

ImPlotTime RoundTime(const ImPlotTime& t, ImPlotTimeUnit unit) {
    ImPlotTime t1 = FloorTime(t, unit);
    ImPlotTime t2 = AddTime(t1,unit,1);
    if (t1.S == t2.S)
        return t.Us - t1.Us < t2.Us - t.Us ? t1 : t2;
    return t.S - t1.S < t2.S - t.S ? t1 : t2;
}

ImPlotTime CombineDateTime(const ImPlotTime& date_part, const ImPlotTime& tod_part) {
    tm& Tm = GImPlot->Tm;
    GetTime(date_part, &GImPlot->Tm);
    int y = Tm.tm_year;
    int m = Tm.tm_mon;
    int d = Tm.tm_mday;
    GetTime(tod_part, &GImPlot->Tm);
    Tm.tm_year = y;
    Tm.tm_mon  = m;
    Tm.tm_mday = d;
    ImPlotTime t = MkTime(&Tm);
    t.Us = tod_part.Us;
    return t;
}

// TODO: allow users to define these
static const char* MONTH_NAMES[]  = {"January","February","March","April","May","June","July","August","September","October","November","December"};
static const char* WD_ABRVS[]     = {"Su","Mo","Tu","We","Th","Fr","Sa"};
static const char* MONTH_ABRVS[]  = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

int FormatTime(const ImPlotTime& t, char* buffer, int size, ImPlotTimeFmt fmt, bool use_24_hr_clk) {
    tm& Tm = GImPlot->Tm;
    GetTime(t, &Tm);
    const int us   = t.Us % 1000;
    const int ms   = t.Us / 1000;
    const int sec  = Tm.tm_sec;
    const int min  = Tm.tm_min;
    if (use_24_hr_clk) {
        const int hr   = Tm.tm_hour;
        switch(fmt) {
            case ImPlotTimeFmt_Us:        return ImFormatString(buffer, size, ".%03d %03d", ms, us);
            case ImPlotTimeFmt_SUs:       return ImFormatString(buffer, size, ":%02d.%03d %03d", sec, ms, us);
            case ImPlotTimeFmt_SMs:       return ImFormatString(buffer, size, ":%02d.%03d", sec, ms);
            case ImPlotTimeFmt_S:         return ImFormatString(buffer, size, ":%02d", sec);
            case ImPlotTimeFmt_MinSMs:    return ImFormatString(buffer, size, ":%02d:%02d.%03d", min, sec, ms);
            case ImPlotTimeFmt_HrMinSMs:  return ImFormatString(buffer, size, "%02d:%02d:%02d.%03d", hr, min, sec, ms);
            case ImPlotTimeFmt_HrMinS:    return ImFormatString(buffer, size, "%02d:%02d:%02d", hr, min, sec);
            case ImPlotTimeFmt_HrMin:     return ImFormatString(buffer, size, "%02d:%02d", hr, min);
            case ImPlotTimeFmt_Hr:        return ImFormatString(buffer, size, "%02d:00", hr);
            default:                      return 0;
        }
    }
    else {
        const char* ap = Tm.tm_hour < 12 ? "am" : "pm";
        const int hr   = (Tm.tm_hour == 0 || Tm.tm_hour == 12) ? 12 : Tm.tm_hour % 12;
        switch(fmt) {
            case ImPlotTimeFmt_Us:        return ImFormatString(buffer, size, ".%03d %03d", ms, us);
            case ImPlotTimeFmt_SUs:       return ImFormatString(buffer, size, ":%02d.%03d %03d", sec, ms, us);
            case ImPlotTimeFmt_SMs:       return ImFormatString(buffer, size, ":%02d.%03d", sec, ms);
            case ImPlotTimeFmt_S:         return ImFormatString(buffer, size, ":%02d", sec);
            case ImPlotTimeFmt_MinSMs:    return ImFormatString(buffer, size, ":%02d:%02d.%03d", min, sec, ms);
            case ImPlotTimeFmt_HrMinSMs:  return ImFormatString(buffer, size, "%d:%02d:%02d.%03d%s", hr, min, sec, ms, ap);
            case ImPlotTimeFmt_HrMinS:    return ImFormatString(buffer, size, "%d:%02d:%02d%s", hr, min, sec, ap);
            case ImPlotTimeFmt_HrMin:     return ImFormatString(buffer, size, "%d:%02d%s", hr, min, ap);
            case ImPlotTimeFmt_Hr:        return ImFormatString(buffer, size, "%d%s", hr, ap);
            default:                      return 0;
        }
    }
}

int FormatDate(const ImPlotTime& t, char* buffer, int size, ImPlotDateFmt fmt, bool use_iso_8601) {
    tm& Tm = GImPlot->Tm;
    GetTime(t, &Tm);
    const int day  = Tm.tm_mday;
    const int mon  = Tm.tm_mon + 1;
    const int year = Tm.tm_year + 1900;
    const int yr   = year % 100;
    if (use_iso_8601) {
        switch (fmt) {
            case ImPlotDateFmt_DayMo:   return ImFormatString(buffer, size, "--%02d-%02d", mon, day);
            case ImPlotDateFmt_DayMoYr: return ImFormatString(buffer, size, "%d-%02d-%02d", year, mon, day);
            case ImPlotDateFmt_MoYr:    return ImFormatString(buffer, size, "%d-%02d", year, mon);
            case ImPlotDateFmt_Mo:      return ImFormatString(buffer, size, "--%02d", mon);
            case ImPlotDateFmt_Yr:      return ImFormatString(buffer, size, "%d", year);
            default:                    return 0;
        }
    }
    else {
        switch (fmt) {
            case ImPlotDateFmt_DayMo:   return ImFormatString(buffer, size, "%d/%d", mon, day);
            case ImPlotDateFmt_DayMoYr: return ImFormatString(buffer, size, "%d/%d/%02d", mon, day, yr);
            case ImPlotDateFmt_MoYr:    return ImFormatString(buffer, size, "%s %d", MONTH_ABRVS[Tm.tm_mon], year);
            case ImPlotDateFmt_Mo:      return ImFormatString(buffer, size, "%s", MONTH_ABRVS[Tm.tm_mon]);
            case ImPlotDateFmt_Yr:      return ImFormatString(buffer, size, "%d", year);
            default:                    return 0;
        }
    }
 }

int FormatDateTime(const ImPlotTime& t, char* buffer, int size, ImPlotDateTimeSpec fmt) {
    int written = 0;
    if (fmt.Date != ImPlotDateFmt_None)
        written += FormatDate(t, buffer, size, fmt.Date, fmt.UseISO8601);
    if (fmt.Time != ImPlotTimeFmt_None) {
        if (fmt.Date != ImPlotDateFmt_None)
            buffer[written++] = ' ';
        written += FormatTime(t, &buffer[written], size - written, fmt.Time, fmt.Use24HourClock);
    }
    return written;
}

inline float GetDateTimeWidth(ImPlotDateTimeSpec fmt) {
    static const ImPlotTime t_max_width = MakeTime(2888, 12, 22, 12, 58, 58, 888888); // best guess at time that maximizes pixel width
    char buffer[32];
    FormatDateTime(t_max_width, buffer, 32, fmt);
    return ImGui::CalcTextSize(buffer).x;
}

inline bool TimeLabelSame(const char* l1, const char* l2) {
    size_t len1 = strlen(l1);
    size_t len2 = strlen(l2);
    size_t n  = len1 < len2 ? len1 : len2;
    return strcmp(l1 + len1 - n, l2 + len2 - n) == 0;
}

static const ImPlotDateTimeSpec TimeFormatLevel0[ImPlotTimeUnit_COUNT] = {
    ImPlotDateTimeSpec(ImPlotDateFmt_None,  ImPlotTimeFmt_Us),
    ImPlotDateTimeSpec(ImPlotDateFmt_None,  ImPlotTimeFmt_SMs),
    ImPlotDateTimeSpec(ImPlotDateFmt_None,  ImPlotTimeFmt_S),
    ImPlotDateTimeSpec(ImPlotDateFmt_None,  ImPlotTimeFmt_HrMin),
    ImPlotDateTimeSpec(ImPlotDateFmt_None,  ImPlotTimeFmt_Hr),
    ImPlotDateTimeSpec(ImPlotDateFmt_DayMo, ImPlotTimeFmt_None),
    ImPlotDateTimeSpec(ImPlotDateFmt_Mo,    ImPlotTimeFmt_None),
    ImPlotDateTimeSpec(ImPlotDateFmt_Yr,    ImPlotTimeFmt_None)
};

static const ImPlotDateTimeSpec TimeFormatLevel1[ImPlotTimeUnit_COUNT] = {
    ImPlotDateTimeSpec(ImPlotDateFmt_None,    ImPlotTimeFmt_HrMin),
    ImPlotDateTimeSpec(ImPlotDateFmt_None,    ImPlotTimeFmt_HrMinS),
    ImPlotDateTimeSpec(ImPlotDateFmt_None,    ImPlotTimeFmt_HrMin),
    ImPlotDateTimeSpec(ImPlotDateFmt_None,    ImPlotTimeFmt_HrMin),
    ImPlotDateTimeSpec(ImPlotDateFmt_DayMoYr, ImPlotTimeFmt_None),
    ImPlotDateTimeSpec(ImPlotDateFmt_DayMoYr, ImPlotTimeFmt_None),
    ImPlotDateTimeSpec(ImPlotDateFmt_Yr,      ImPlotTimeFmt_None),
    ImPlotDateTimeSpec(ImPlotDateFmt_Yr,      ImPlotTimeFmt_None)
};

static const ImPlotDateTimeSpec TimeFormatLevel1First[ImPlotTimeUnit_COUNT] = {
    ImPlotDateTimeSpec(ImPlotDateFmt_DayMoYr, ImPlotTimeFmt_HrMinS),
    ImPlotDateTimeSpec(ImPlotDateFmt_DayMoYr, ImPlotTimeFmt_HrMinS),
    ImPlotDateTimeSpec(ImPlotDateFmt_DayMoYr, ImPlotTimeFmt_HrMin),
    ImPlotDateTimeSpec(ImPlotDateFmt_DayMoYr, ImPlotTimeFmt_HrMin),
    ImPlotDateTimeSpec(ImPlotDateFmt_DayMoYr, ImPlotTimeFmt_None),
    ImPlotDateTimeSpec(ImPlotDateFmt_DayMoYr, ImPlotTimeFmt_None),
    ImPlotDateTimeSpec(ImPlotDateFmt_Yr,      ImPlotTimeFmt_None),
    ImPlotDateTimeSpec(ImPlotDateFmt_Yr,      ImPlotTimeFmt_None)
};

static const ImPlotDateTimeSpec TimeFormatMouseCursor[ImPlotTimeUnit_COUNT] = {
    ImPlotDateTimeSpec(ImPlotDateFmt_None,     ImPlotTimeFmt_Us),
    ImPlotDateTimeSpec(ImPlotDateFmt_None,     ImPlotTimeFmt_SUs),
    ImPlotDateTimeSpec(ImPlotDateFmt_None,     ImPlotTimeFmt_SMs),
    ImPlotDateTimeSpec(ImPlotDateFmt_None,     ImPlotTimeFmt_HrMinS),
    ImPlotDateTimeSpec(ImPlotDateFmt_None,     ImPlotTimeFmt_HrMin),
    ImPlotDateTimeSpec(ImPlotDateFmt_DayMo,    ImPlotTimeFmt_Hr),
    ImPlotDateTimeSpec(ImPlotDateFmt_DayMoYr,  ImPlotTimeFmt_None),
    ImPlotDateTimeSpec(ImPlotDateFmt_MoYr,     ImPlotTimeFmt_None)
};

inline ImPlotDateTimeSpec GetDateTimeFmt(const ImPlotDateTimeSpec* ctx, ImPlotTimeUnit idx) {
    ImPlotStyle& style     = GetStyle();
    ImPlotDateTimeSpec fmt  = ctx[idx];
    fmt.UseISO8601         = style.UseISO8601;
    fmt.Use24HourClock     = style.Use24HourClock;
    return fmt;
}

void Locator_Time(ImPlotTicker& ticker, const ImPlotRange& range, float pixels, bool vertical, ImPlotFormatter formatter, void* formatter_data) {
    IM_ASSERT_USER_ERROR(vertical == false, "Cannot locate Time ticks on vertical axis!");
    (void)vertical;
    // get units for level 0 and level 1 labels
    const ImPlotTimeUnit unit0 = GetUnitForRange(range.Size() / (pixels / 100)); // level = 0 (top)
    const ImPlotTimeUnit unit1 = ImClamp(unit0 + 1, 0, ImPlotTimeUnit_COUNT-1);  // level = 1 (bottom)
    // get time format specs
    const ImPlotDateTimeSpec fmt0 = GetDateTimeFmt(TimeFormatLevel0, unit0);
    const ImPlotDateTimeSpec fmt1 = GetDateTimeFmt(TimeFormatLevel1, unit1);
    const ImPlotDateTimeSpec fmtf = GetDateTimeFmt(TimeFormatLevel1First, unit1);
    // min max times
    const ImPlotTime t_min = ImPlotTime::FromDouble(range.Min);
    const ImPlotTime t_max = ImPlotTime::FromDouble(range.Max);
    // maximum allowable density of labels
    const float max_density = 0.5f;
    // book keeping
    int last_major_offset = -1;
    // formatter data
    Formatter_Time_Data ftd;
    ftd.UserFormatter = formatter;
    ftd.UserFormatterData = formatter_data;
    if (unit0 != ImPlotTimeUnit_Yr) {
        // pixels per major (level 1) division
        const float pix_per_major_div = pixels / (float)(range.Size() / TimeUnitSpans[unit1]);
        // nominal pixels taken up by labels
        const float fmt0_width = GetDateTimeWidth(fmt0);
        const float fmt1_width = GetDateTimeWidth(fmt1);
        const float fmtf_width = GetDateTimeWidth(fmtf);
        // the maximum number of minor (level 0) labels that can fit between major (level 1) divisions
        const int   minor_per_major   = (int)(max_density * pix_per_major_div / fmt0_width);
        // the minor step size (level 0)
        const int step = GetTimeStep(minor_per_major, unit0);
        // generate ticks
        ImPlotTime t1 = FloorTime(ImPlotTime::FromDouble(range.Min), unit1);
        while (t1 < t_max) {
            // get next major
            const ImPlotTime t2 = AddTime(t1, unit1, 1);
            // add major tick
            if (t1 >= t_min && t1 <= t_max) {
                // minor level 0 tick
                ftd.Time = t1; ftd.Spec = fmt0;
                ticker.AddTick(t1.ToDouble(), true, 0, true, Formatter_Time, &ftd);
                // major level 1 tick
                ftd.Time = t1; ftd.Spec = last_major_offset < 0 ? fmtf : fmt1;
                ImPlotTick& tick_maj = ticker.AddTick(t1.ToDouble(), true, 1, true, Formatter_Time, &ftd);
                const char* this_major = ticker.GetText(tick_maj);
                if (last_major_offset >= 0 && TimeLabelSame(ticker.TextBuffer.Buf.Data + last_major_offset, this_major))
                    tick_maj.ShowLabel = false;
                last_major_offset = tick_maj.TextOffset;
            }
            // add minor ticks up until next major
            if (minor_per_major > 1 && (t_min <= t2 && t1 <= t_max)) {
                ImPlotTime t12 = AddTime(t1, unit0, step);
                while (t12 < t2) {
                    float px_to_t2 = (float)((t2 - t12).ToDouble()/range.Size()) * pixels;
                    if (t12 >= t_min && t12 <= t_max) {
                        ftd.Time = t12; ftd.Spec = fmt0;
                        ticker.AddTick(t12.ToDouble(), false, 0, px_to_t2 >= fmt0_width, Formatter_Time, &ftd);
                        if (last_major_offset < 0 && px_to_t2 >= fmt0_width && px_to_t2 >= (fmt1_width + fmtf_width) / 2) {
                            ftd.Time = t12; ftd.Spec = fmtf;
                            ImPlotTick& tick_maj = ticker.AddTick(t12.ToDouble(), true, 1, true, Formatter_Time, &ftd);
                            last_major_offset = tick_maj.TextOffset;
                        }
                    }
                    t12 = AddTime(t12, unit0, step);
                }
            }
            t1 = t2;
        }
    }
    else {
        const ImPlotDateTimeSpec fmty = GetDateTimeFmt(TimeFormatLevel0, ImPlotTimeUnit_Yr);
        const float label_width = GetDateTimeWidth(fmty);
        const int   max_labels  = (int)(max_density * pixels / label_width);
        const int year_min      = GetYear(t_min);
        const int year_max      = GetYear(CeilTime(t_max, ImPlotTimeUnit_Yr));
        const double nice_range = NiceNum((year_max - year_min)*0.99,false);
        const double interval   = NiceNum(nice_range / (max_labels - 1), true);
        const int graphmin      = (int)(floor(year_min / interval) * interval);
        const int graphmax      = (int)(ceil(year_max  / interval) * interval);
        const int step          = (int)interval <= 0 ? 1 : (int)interval;

        for (int y = graphmin; y < graphmax; y += step) {
            ImPlotTime t = MakeTime(y);
            if (t >= t_min && t <= t_max) {
                ftd.Time = t; ftd.Spec = fmty;
                ticker.AddTick(t.ToDouble(), true, 0, true, Formatter_Time, &ftd);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Context Menu
//-----------------------------------------------------------------------------

template <typename F>
bool DragFloat(const char*, F*, float, F, F) {
    return false;
}

template <>
bool DragFloat<double>(const char* label, double* v, float v_speed, double v_min, double v_max) {
    return ImGui::DragScalar(label, ImGuiDataType_Double, v, v_speed, &v_min, &v_max, "%.3f", 1);
}

template <>
bool DragFloat<float>(const char* label, float* v, float v_speed, float v_min, float v_max) {
    return ImGui::DragScalar(label, ImGuiDataType_Float, v, v_speed, &v_min, &v_max, "%.3f", 1);
}

inline void BeginDisabledControls(bool cond) {
    if (cond) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.25f);
    }
}

inline void EndDisabledControls(bool cond) {
    if (cond) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
}

void ShowAxisContextMenu(ImPlotAxis& axis, ImPlotAxis* equal_axis, bool /*time_allowed*/) {

    ImGui::PushItemWidth(75);
    bool always_locked   = axis.IsRangeLocked() || axis.IsAutoFitting();
    bool label           = axis.HasLabel();
    bool grid            = axis.HasGridLines();
    bool ticks           = axis.HasTickMarks();
    bool labels          = axis.HasTickLabels();
    double drag_speed    = (axis.Range.Size() <= DBL_EPSILON) ? DBL_EPSILON * 1.0e+13 : 0.01 * axis.Range.Size(); // recover from almost equal axis limits.

    if (axis.Scale == ImPlotScale_Time) {
        ImPlotTime tmin = ImPlotTime::FromDouble(axis.Range.Min);
        ImPlotTime tmax = ImPlotTime::FromDouble(axis.Range.Max);

        BeginDisabledControls(always_locked);
        ImGui::CheckboxFlags("##LockMin", (unsigned int*)&axis.Flags, ImPlotAxisFlags_LockMin);
        EndDisabledControls(always_locked);
        ImGui::SameLine();
        BeginDisabledControls(axis.IsLockedMin() || always_locked);
        if (ImGui::BeginMenu("Min Time")) {
            if (ShowTimePicker("mintime", &tmin)) {
                if (tmin >= tmax)
                    tmax = AddTime(tmin, ImPlotTimeUnit_S, 1);
                axis.SetRange(tmin.ToDouble(),tmax.ToDouble());
            }
            ImGui::Separator();
            if (ShowDatePicker("mindate",&axis.PickerLevel,&axis.PickerTimeMin,&tmin,&tmax)) {
                tmin = CombineDateTime(axis.PickerTimeMin, tmin);
                if (tmin >= tmax)
                    tmax = AddTime(tmin, ImPlotTimeUnit_S, 1);
                axis.SetRange(tmin.ToDouble(), tmax.ToDouble());
            }
            ImGui::EndMenu();
        }
        EndDisabledControls(axis.IsLockedMin() || always_locked);

        BeginDisabledControls(always_locked);
        ImGui::CheckboxFlags("##LockMax", (unsigned int*)&axis.Flags, ImPlotAxisFlags_LockMax);
        EndDisabledControls(always_locked);
        ImGui::SameLine();
        BeginDisabledControls(axis.IsLockedMax() || always_locked);
        if (ImGui::BeginMenu("Max Time")) {
            if (ShowTimePicker("maxtime", &tmax)) {
                if (tmax <= tmin)
                    tmin = AddTime(tmax, ImPlotTimeUnit_S, -1);
                axis.SetRange(tmin.ToDouble(),tmax.ToDouble());
            }
            ImGui::Separator();
            if (ShowDatePicker("maxdate",&axis.PickerLevel,&axis.PickerTimeMax,&tmin,&tmax)) {
                tmax = CombineDateTime(axis.PickerTimeMax, tmax);
                if (tmax <= tmin)
                    tmin = AddTime(tmax, ImPlotTimeUnit_S, -1);
                axis.SetRange(tmin.ToDouble(), tmax.ToDouble());
            }
            ImGui::EndMenu();
        }
        EndDisabledControls(axis.IsLockedMax() || always_locked);
    }
    else {
        BeginDisabledControls(always_locked);
        ImGui::CheckboxFlags("##LockMin", (unsigned int*)&axis.Flags, ImPlotAxisFlags_LockMin);
        EndDisabledControls(always_locked);
        ImGui::SameLine();
        BeginDisabledControls(axis.IsLockedMin() || always_locked);
        double temp_min = axis.Range.Min;
        if (DragFloat("Min", &temp_min, (float)drag_speed, -HUGE_VAL, axis.Range.Max - DBL_EPSILON)) {
            axis.SetMin(temp_min,true);
            if (equal_axis != NULL)
                equal_axis->SetAspect(axis.GetAspect());
        }
        EndDisabledControls(axis.IsLockedMin() || always_locked);

        BeginDisabledControls(always_locked);
        ImGui::CheckboxFlags("##LockMax", (unsigned int*)&axis.Flags, ImPlotAxisFlags_LockMax);
        EndDisabledControls(always_locked);
        ImGui::SameLine();
        BeginDisabledControls(axis.IsLockedMax() || always_locked);
        double temp_max = axis.Range.Max;
        if (DragFloat("Max", &temp_max, (float)drag_speed, axis.Range.Min + DBL_EPSILON, HUGE_VAL)) {
            axis.SetMax(temp_max,true);
            if (equal_axis != NULL)
                equal_axis->SetAspect(axis.GetAspect());
        }
        EndDisabledControls(axis.IsLockedMax() || always_locked);
    }

    ImGui::Separator();

    ImGui::CheckboxFlags("Auto-Fit",(unsigned int*)&axis.Flags, ImPlotAxisFlags_AutoFit);
    // TODO
    // BeginDisabledControls(axis.IsTime() && time_allowed);
    // ImGui::CheckboxFlags("Log Scale",(unsigned int*)&axis.Flags, ImPlotAxisFlags_LogScale);
    // EndDisabledControls(axis.IsTime() && time_allowed);
    // if (time_allowed) {
    //     BeginDisabledControls(axis.IsLog() || axis.IsSymLog());
    //     ImGui::CheckboxFlags("Time",(unsigned int*)&axis.Flags, ImPlotAxisFlags_Time);
    //     EndDisabledControls(axis.IsLog() || axis.IsSymLog());
    // }
    ImGui::Separator();
    ImGui::CheckboxFlags("Invert",(unsigned int*)&axis.Flags, ImPlotAxisFlags_Invert);
    ImGui::CheckboxFlags("Opposite",(unsigned int*)&axis.Flags, ImPlotAxisFlags_Opposite);
    ImGui::Separator();
    BeginDisabledControls(axis.LabelOffset == -1);
    if (ImGui::Checkbox("Label", &label))
        ImFlipFlag(axis.Flags, ImPlotAxisFlags_NoLabel);
    EndDisabledControls(axis.LabelOffset == -1);
    if (ImGui::Checkbox("Grid Lines", &grid))
        ImFlipFlag(axis.Flags, ImPlotAxisFlags_NoGridLines);
    if (ImGui::Checkbox("Tick Marks", &ticks))
        ImFlipFlag(axis.Flags, ImPlotAxisFlags_NoTickMarks);
    if (ImGui::Checkbox("Tick Labels", &labels))
        ImFlipFlag(axis.Flags, ImPlotAxisFlags_NoTickLabels);

}

bool ShowLegendContextMenu(ImPlotLegend& legend, bool visible) {
    const float s = ImGui::GetFrameHeight();
    bool ret = false;
    if (ImGui::Checkbox("Show",&visible))
        ret = true;
    if (legend.CanGoInside)
        ImGui::CheckboxFlags("Outside",(unsigned int*)&legend.Flags, ImPlotLegendFlags_Outside);
    if (ImGui::RadioButton("H", ImHasFlag(legend.Flags, ImPlotLegendFlags_Horizontal)))
        legend.Flags |= ImPlotLegendFlags_Horizontal;
    ImGui::SameLine();
    if (ImGui::RadioButton("V", !ImHasFlag(legend.Flags, ImPlotLegendFlags_Horizontal)))
        legend.Flags &= ~ImPlotLegendFlags_Horizontal;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2,2));
    if (ImGui::Button("NW",ImVec2(1.5f*s,s))) { legend.Location = ImPlotLocation_NorthWest; } ImGui::SameLine();
    if (ImGui::Button("N", ImVec2(1.5f*s,s))) { legend.Location = ImPlotLocation_North;     } ImGui::SameLine();
    if (ImGui::Button("NE",ImVec2(1.5f*s,s))) { legend.Location = ImPlotLocation_NorthEast; }
    if (ImGui::Button("W", ImVec2(1.5f*s,s))) { legend.Location = ImPlotLocation_West;      } ImGui::SameLine();
    if (ImGui::InvisibleButton("C", ImVec2(1.5f*s,s))) {     } ImGui::SameLine();
    if (ImGui::Button("E", ImVec2(1.5f*s,s))) { legend.Location = ImPlotLocation_East;      }
    if (ImGui::Button("SW",ImVec2(1.5f*s,s))) { legend.Location = ImPlotLocation_SouthWest; } ImGui::SameLine();
    if (ImGui::Button("S", ImVec2(1.5f*s,s))) { legend.Location = ImPlotLocation_South;     } ImGui::SameLine();
    if (ImGui::Button("SE",ImVec2(1.5f*s,s))) { legend.Location = ImPlotLocation_SouthEast; }
    ImGui::PopStyleVar();
    return ret;
}

void ShowSubplotsContextMenu(ImPlotSubplot& subplot) {
    if ((ImGui::BeginMenu("Linking"))) {
        if (ImGui::MenuItem("Link Rows",NULL,ImHasFlag(subplot.Flags, ImPlotSubplotFlags_LinkRows)))
            ImFlipFlag(subplot.Flags, ImPlotSubplotFlags_LinkRows);
        if (ImGui::MenuItem("Link Cols",NULL,ImHasFlag(subplot.Flags, ImPlotSubplotFlags_LinkCols)))
            ImFlipFlag(subplot.Flags, ImPlotSubplotFlags_LinkCols);
        if (ImGui::MenuItem("Link All X",NULL,ImHasFlag(subplot.Flags, ImPlotSubplotFlags_LinkAllX)))
            ImFlipFlag(subplot.Flags, ImPlotSubplotFlags_LinkAllX);
        if (ImGui::MenuItem("Link All Y",NULL,ImHasFlag(subplot.Flags, ImPlotSubplotFlags_LinkAllY)))
            ImFlipFlag(subplot.Flags, ImPlotSubplotFlags_LinkAllY);
        ImGui::EndMenu();
    }
    if ((ImGui::BeginMenu("Settings"))) {
        BeginDisabledControls(!subplot.HasTitle);
        if (ImGui::MenuItem("Title",NULL,subplot.HasTitle && !ImHasFlag(subplot.Flags, ImPlotSubplotFlags_NoTitle)))
            ImFlipFlag(subplot.Flags, ImPlotSubplotFlags_NoTitle);
        EndDisabledControls(!subplot.HasTitle);
        if (ImGui::MenuItem("Resizable",NULL,!ImHasFlag(subplot.Flags, ImPlotSubplotFlags_NoResize)))
            ImFlipFlag(subplot.Flags, ImPlotSubplotFlags_NoResize);
        if (ImGui::MenuItem("Align",NULL,!ImHasFlag(subplot.Flags, ImPlotSubplotFlags_NoAlign)))
            ImFlipFlag(subplot.Flags, ImPlotSubplotFlags_NoAlign);
        if (ImGui::MenuItem("Share Items",NULL,ImHasFlag(subplot.Flags, ImPlotSubplotFlags_ShareItems)))
            ImFlipFlag(subplot.Flags, ImPlotSubplotFlags_ShareItems);
        ImGui::EndMenu();
    }
}

void ShowPlotContextMenu(ImPlotPlot& plot) {
    const bool owns_legend = GImPlot->CurrentItems == &plot.Items;
    const bool equal = ImHasFlag(plot.Flags, ImPlotFlags_Equal);

    char buf[16] = {};

    for (int i = 0; i < IMPLOT_NUM_X_AXES; i++) {
        ImPlotAxis& x_axis = plot.XAxis(i);
        if (!x_axis.Enabled || !x_axis.HasMenus())
            continue;
        ImGui::PushID(i);
        ImFormatString(buf, sizeof(buf) - 1, i == 0 ? "X-Axis" : "X-Axis %d", i + 1);
        if (ImGui::BeginMenu(x_axis.HasLabel() ? plot.GetAxisLabel(x_axis) : buf)) {
            ShowAxisContextMenu(x_axis, equal ? x_axis.OrthoAxis : NULL, false);
            ImGui::EndMenu();
        }
        ImGui::PopID();
    }

    for (int i = 0; i < IMPLOT_NUM_Y_AXES; i++) {
        ImPlotAxis& y_axis = plot.YAxis(i);
        if (!y_axis.Enabled || !y_axis.HasMenus())
            continue;
        ImGui::PushID(i);
        ImFormatString(buf, sizeof(buf) - 1, i == 0 ? "Y-Axis" : "Y-Axis %d", i + 1);
        if (ImGui::BeginMenu(y_axis.HasLabel() ? plot.GetAxisLabel(y_axis) : buf)) {
            ShowAxisContextMenu(y_axis, equal ? y_axis.OrthoAxis : NULL, false);
            ImGui::EndMenu();
        }
        ImGui::PopID();
    }

    ImGui::Separator();
    if (!ImHasFlag(GImPlot->CurrentItems->Legend.Flags, ImPlotLegendFlags_NoMenus)) {
        if ((ImGui::BeginMenu("Legend"))) {
            if (owns_legend) {
                if (ShowLegendContextMenu(plot.Items.Legend, !ImHasFlag(plot.Flags, ImPlotFlags_NoLegend)))
                    ImFlipFlag(plot.Flags, ImPlotFlags_NoLegend);
            }
            else if (GImPlot->CurrentSubplot != NULL) {
                if (ShowLegendContextMenu(GImPlot->CurrentSubplot->Items.Legend, !ImHasFlag(GImPlot->CurrentSubplot->Flags, ImPlotSubplotFlags_NoLegend)))
                    ImFlipFlag(GImPlot->CurrentSubplot->Flags, ImPlotSubplotFlags_NoLegend);
            }
            ImGui::EndMenu();
        }
    }
    if ((ImGui::BeginMenu("Settings"))) {
        if (ImGui::MenuItem("Equal", NULL, ImHasFlag(plot.Flags, ImPlotFlags_Equal)))
            ImFlipFlag(plot.Flags, ImPlotFlags_Equal);
        if (ImGui::MenuItem("Box Select",NULL,!ImHasFlag(plot.Flags, ImPlotFlags_NoBoxSelect)))
            ImFlipFlag(plot.Flags, ImPlotFlags_NoBoxSelect);
        BeginDisabledControls(plot.TitleOffset == -1);
        if (ImGui::MenuItem("Title",NULL,plot.HasTitle()))
            ImFlipFlag(plot.Flags, ImPlotFlags_NoTitle);
        EndDisabledControls(plot.TitleOffset == -1);
        if (ImGui::MenuItem("Mouse Position",NULL,!ImHasFlag(plot.Flags, ImPlotFlags_NoMouseText)))
            ImFlipFlag(plot.Flags, ImPlotFlags_NoMouseText);
        if (ImGui::MenuItem("Crosshairs",NULL,ImHasFlag(plot.Flags, ImPlotFlags_Crosshairs)))
            ImFlipFlag(plot.Flags, ImPlotFlags_Crosshairs);
        ImGui::EndMenu();
    }
    if (GImPlot->CurrentSubplot != NULL && !ImHasFlag(GImPlot->CurrentPlot->Flags, ImPlotSubplotFlags_NoMenus)) {
        ImGui::Separator();
        if ((ImGui::BeginMenu("Subplots"))) {
            ShowSubplotsContextMenu(*GImPlot->CurrentSubplot);
            ImGui::EndMenu();
        }
    }
}

//-----------------------------------------------------------------------------
// Axis Utils
//-----------------------------------------------------------------------------

static inline int AxisPrecision(const ImPlotAxis& axis) {
    const double range = axis.Ticker.TickCount() > 1 ? (axis.Ticker.Ticks[1].PlotPos - axis.Ticker.Ticks[0].PlotPos) : axis.Range.Size();
    return Precision(range);
}

static inline double RoundAxisValue(const ImPlotAxis& axis, double value) {
    return RoundTo(value, AxisPrecision(axis));
}

void LabelAxisValue(const ImPlotAxis& axis, double value, char* buff, int size, bool round) {
    ImPlotContext& gp = *GImPlot;
    // TODO: We shouldn't explicitly check that the axis is Time here. Ideally,
    // Formatter_Time would handle the formatting for us, but the code below
    // needs additional arguments which are not currently available in ImPlotFormatter
    if (axis.Locator == Locator_Time) {
        ImPlotTimeUnit unit = axis.Vertical
                            ? GetUnitForRange(axis.Range.Size() / (gp.CurrentPlot->PlotRect.GetHeight() / 100)) // TODO: magic value!
                            : GetUnitForRange(axis.Range.Size() / (gp.CurrentPlot->PlotRect.GetWidth() / 100)); // TODO: magic value!
        FormatDateTime(ImPlotTime::FromDouble(value), buff, size, GetDateTimeFmt(TimeFormatMouseCursor, unit));
    }
    else {
        if (round)
            value = RoundAxisValue(axis, value);
        axis.Formatter(value, buff, size, axis.FormatterData);
    }
}

void UpdateAxisColors(ImPlotAxis& axis) {
    const ImVec4 col_grid = GetStyleColorVec4(ImPlotCol_AxisGrid);
    axis.ColorMaj         = ImGui::GetColorU32(col_grid);
    axis.ColorMin         = ImGui::GetColorU32(col_grid*ImVec4(1,1,1,GImPlot->Style.MinorAlpha));
    axis.ColorTick        = GetStyleColorU32(ImPlotCol_AxisTick);
    axis.ColorTxt         = GetStyleColorU32(ImPlotCol_AxisText);
    axis.ColorBg          = GetStyleColorU32(ImPlotCol_AxisBg);
    axis.ColorHov         = GetStyleColorU32(ImPlotCol_AxisBgHovered);
    axis.ColorAct         = GetStyleColorU32(ImPlotCol_AxisBgActive);
    // axis.ColorHiLi     = IM_COL32_BLACK_TRANS;
}

void PadAndDatumAxesX(ImPlotPlot& plot, float& pad_T, float& pad_B, ImPlotAlignmentData* align) {

    ImPlotContext& gp = *GImPlot;

    const float T = ImGui::GetTextLineHeight();
    const float P = gp.Style.LabelPadding.y;
    const float K = gp.Style.MinorTickLen.x;

    int   count_T = 0;
    int   count_B = 0;
    float last_T  = plot.AxesRect.Min.y;
    float last_B  = plot.AxesRect.Max.y;

    for (int i = IMPLOT_NUM_X_AXES; i-- > 0;) { // FYI: can iterate forward
        ImPlotAxis& axis = plot.XAxis(i);
        if (!axis.Enabled)
            continue;
        const bool label = axis.HasLabel();
        const bool ticks = axis.HasTickLabels();
        const bool opp   = axis.IsOpposite();
        const bool time  = axis.Scale == ImPlotScale_Time;
        if (opp) {
            if (count_T++ > 0)
                pad_T += K + P;
            if (label)
                pad_T += T + P;
            if (ticks)
                pad_T += ImMax(T, axis.Ticker.MaxSize.y) + P + (time ? T + P : 0);
            axis.Datum1 = plot.CanvasRect.Min.y + pad_T;
            axis.Datum2 = last_T;
            last_T = axis.Datum1;
        }
        else {
            if (count_B++ > 0)
                pad_B += K + P;
            if (label)
                pad_B += T + P;
            if (ticks)
                pad_B += ImMax(T, axis.Ticker.MaxSize.y) + P + (time ? T + P : 0);
            axis.Datum1 = plot.CanvasRect.Max.y - pad_B;
            axis.Datum2 = last_B;
            last_B = axis.Datum1;
        }
    }

    if (align) {
        count_T = count_B = 0;
        float delta_T, delta_B;
        align->Update(pad_T,pad_B,delta_T,delta_B);
        for (int i = IMPLOT_NUM_X_AXES; i-- > 0;) {
            ImPlotAxis& axis = plot.XAxis(i);
            if (!axis.Enabled)
                continue;
            if (axis.IsOpposite()) {
                axis.Datum1 += delta_T;
                axis.Datum2 += count_T++ > 1 ? delta_T : 0;
            }
            else {
                axis.Datum1 -= delta_B;
                axis.Datum2 -= count_B++ > 1 ? delta_B : 0;
            }
        }
    }
}

void PadAndDatumAxesY(ImPlotPlot& plot, float& pad_L, float& pad_R, ImPlotAlignmentData* align) {

    //   [   pad_L   ]                 [   pad_R   ]
    //   .................CanvasRect................
    //   :TPWPK.PTPWP _____PlotRect____ PWPTP.KPWPT:
    //   :A # |- A # |-               -| # A -| # A:
    //   :X   |  X   |                 |   X  |   x:
    //   :I # |- I # |-               -| # I -| # I:
    //   :S   |  S   |                 |   S  |   S:
    //   :3 # |- 0 # |-_______________-| # 1 -| # 2:
    //   :.........................................:
    //
    //   T = text height
    //   P = label padding
    //   K = minor tick length
    //   W = label width

    ImPlotContext& gp = *GImPlot;

    const float T = ImGui::GetTextLineHeight();
    const float P = gp.Style.LabelPadding.x;
    const float K = gp.Style.MinorTickLen.y;

    int   count_L = 0;
    int   count_R = 0;
    float last_L  = plot.AxesRect.Min.x;
    float last_R  = plot.AxesRect.Max.x;

    for (int i = IMPLOT_NUM_Y_AXES; i-- > 0;) { // FYI: can iterate forward
        ImPlotAxis& axis = plot.YAxis(i);
        if (!axis.Enabled)
            continue;
        const bool label = axis.HasLabel();
        const bool ticks = axis.HasTickLabels();
        const bool opp   = axis.IsOpposite();
        if (opp) {
            if (count_R++ > 0)
                pad_R += K + P;
            if (label)
                pad_R += T + P;
            if (ticks)
                pad_R += axis.Ticker.MaxSize.x + P;
            axis.Datum1 = plot.CanvasRect.Max.x - pad_R;
            axis.Datum2 = last_R;
            last_R = axis.Datum1;
        }
        else {
            if (count_L++ > 0)
                pad_L += K + P;
            if (label)
                pad_L += T + P;
            if (ticks)
                pad_L += axis.Ticker.MaxSize.x + P;
            axis.Datum1 = plot.CanvasRect.Min.x + pad_L;
            axis.Datum2 = last_L;
            last_L = axis.Datum1;
        }
    }

    plot.PlotRect.Min.x = plot.CanvasRect.Min.x + pad_L;
    plot.PlotRect.Max.x = plot.CanvasRect.Max.x - pad_R;

    if (align) {
        count_L = count_R = 0;
        float delta_L, delta_R;
        align->Update(pad_L,pad_R,delta_L,delta_R);
        for (int i = IMPLOT_NUM_Y_AXES; i-- > 0;) {
            ImPlotAxis& axis = plot.YAxis(i);
            if (!axis.Enabled)
                continue;
            if (axis.IsOpposite()) {
                axis.Datum1 -= delta_R;
                axis.Datum2 -= count_R++ > 1 ? delta_R : 0;
            }
            else {
                axis.Datum1 += delta_L;
                axis.Datum2 += count_L++ > 1 ? delta_L : 0;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// RENDERING
//-----------------------------------------------------------------------------

static inline void RenderGridLinesX(ImDrawList& DrawList, const ImPlotTicker& ticker, const ImRect& rect, ImU32 col_maj, ImU32 col_min, float size_maj, float size_min) {
    const float density   = ticker.TickCount() / rect.GetWidth();
    ImVec4 col_min4  = ImGui::ColorConvertU32ToFloat4(col_min);
    col_min4.w      *= ImClamp(ImRemap(density, 0.1f, 0.2f, 1.0f, 0.0f), 0.0f, 1.0f);
    col_min = ImGui::ColorConvertFloat4ToU32(col_min4);
    for (int t = 0; t < ticker.TickCount(); t++) {
        const ImPlotTick& xt = ticker.Ticks[t];
        if (xt.PixelPos < rect.Min.x || xt.PixelPos > rect.Max.x)
            continue;
        if (xt.Level == 0) {
            if (xt.Major)
                DrawList.AddLine(ImVec2(xt.PixelPos, rect.Min.y), ImVec2(xt.PixelPos, rect.Max.y), col_maj, size_maj);
            else if (density < 0.2f)
                DrawList.AddLine(ImVec2(xt.PixelPos, rect.Min.y), ImVec2(xt.PixelPos, rect.Max.y), col_min, size_min);
        }
    }
}

static inline void RenderGridLinesY(ImDrawList& DrawList, const ImPlotTicker& ticker, const ImRect& rect, ImU32 col_maj, ImU32 col_min, float size_maj, float size_min) {
    const float density   = ticker.TickCount() / rect.GetHeight();
    ImVec4 col_min4  = ImGui::ColorConvertU32ToFloat4(col_min);
    col_min4.w      *= ImClamp(ImRemap(density, 0.1f, 0.2f, 1.0f, 0.0f), 0.0f, 1.0f);
    col_min = ImGui::ColorConvertFloat4ToU32(col_min4);
    for (int t = 0; t < ticker.TickCount(); t++) {
        const ImPlotTick& yt = ticker.Ticks[t];
        if (yt.PixelPos < rect.Min.y || yt.PixelPos > rect.Max.y)
            continue;
        if (yt.Major)
            DrawList.AddLine(ImVec2(rect.Min.x, yt.PixelPos), ImVec2(rect.Max.x, yt.PixelPos), col_maj, size_maj);
        else if (density < 0.2f)
            DrawList.AddLine(ImVec2(rect.Min.x, yt.PixelPos), ImVec2(rect.Max.x, yt.PixelPos), col_min, size_min);
    }
}

static inline void RenderSelectionRect(ImDrawList& DrawList, const ImVec2& p_min, const ImVec2& p_max, const ImVec4& col) {
    const ImU32 col_bg = ImGui::GetColorU32(col * ImVec4(1,1,1,0.25f));
    const ImU32 col_bd = ImGui::GetColorU32(col);
    DrawList.AddRectFilled(p_min, p_max, col_bg);
    DrawList.AddRect(p_min, p_max, col_bd);
}

//-----------------------------------------------------------------------------
// Input Handling
//-----------------------------------------------------------------------------

static const float MOUSE_CURSOR_DRAG_THRESHOLD = 5.0f;
static const float BOX_SELECT_DRAG_THRESHOLD   = 4.0f;

bool UpdateInput(ImPlotPlot& plot) {

    bool changed = false;

    ImPlotContext& gp = *GImPlot;
    ImGuiIO& IO = ImGui::GetIO();

    // BUTTON STATE -----------------------------------------------------------

    const ImGuiButtonFlags plot_button_flags = ImGuiButtonFlags_AllowItemOverlap
                                             | ImGuiButtonFlags_PressedOnClick
                                             | ImGuiButtonFlags_PressedOnDoubleClick
                                             | ImGuiButtonFlags_MouseButtonLeft
                                             | ImGuiButtonFlags_MouseButtonRight
                                             | ImGuiButtonFlags_MouseButtonMiddle;
    const ImGuiButtonFlags axis_button_flags = ImGuiButtonFlags_FlattenChildren
                                             | plot_button_flags;

    const bool plot_clicked = ImGui::ButtonBehavior(plot.PlotRect,plot.ID,&plot.Hovered,&plot.Held,plot_button_flags);
    ImGui::SetItemAllowOverlap();

    if (plot_clicked) {
        if (!ImHasFlag(plot.Flags, ImPlotFlags_NoBoxSelect) && IO.MouseClicked[gp.InputMap.Select] && ImHasFlag(IO.KeyMods, gp.InputMap.SelectMod)) {
            plot.Selecting   = true;
            plot.SelectStart = IO.MousePos;
            plot.SelectRect  = ImRect(0,0,0,0);
        }
        if (IO.MouseDoubleClicked[gp.InputMap.Fit]) {
            plot.FitThisFrame = true;
            for (int i = 0; i < ImAxis_COUNT; ++i)
                plot.Axes[i].FitThisFrame = true;
        }
    }

    const bool can_pan = IO.MouseDown[gp.InputMap.Pan] && ImHasFlag(IO.KeyMods, gp.InputMap.PanMod);

    plot.Held = plot.Held && can_pan;

    bool x_click[IMPLOT_NUM_X_AXES] = {false};
    bool x_held[IMPLOT_NUM_X_AXES]  = {false};
    bool x_hov[IMPLOT_NUM_X_AXES]   = {false};

    bool y_click[IMPLOT_NUM_Y_AXES] = {false};
    bool y_held[IMPLOT_NUM_Y_AXES]  = {false};
    bool y_hov[IMPLOT_NUM_Y_AXES]   = {false};

    for (int i = 0; i < IMPLOT_NUM_X_AXES; ++i) {
        ImPlotAxis& xax = plot.XAxis(i);
        if (xax.Enabled) {
            ImGui::KeepAliveID(xax.ID);
            x_click[i]  = ImGui::ButtonBehavior(xax.HoverRect,xax.ID,&xax.Hovered,&xax.Held,axis_button_flags);
            if (x_click[i] && IO.MouseDoubleClicked[gp.InputMap.Fit])
                plot.FitThisFrame = xax.FitThisFrame = true;
            xax.Held  = xax.Held && can_pan;
            x_hov[i]  = xax.Hovered || plot.Hovered;
            x_held[i] = xax.Held    || plot.Held;
        }
    }

    for (int i = 0; i < IMPLOT_NUM_Y_AXES; ++i) {
        ImPlotAxis& yax = plot.YAxis(i);
        if (yax.Enabled) {
            ImGui::KeepAliveID(yax.ID);
            y_click[i]  = ImGui::ButtonBehavior(yax.HoverRect,yax.ID,&yax.Hovered,&yax.Held,axis_button_flags);
            if (y_click[i] && IO.MouseDoubleClicked[gp.InputMap.Fit])
                plot.FitThisFrame = yax.FitThisFrame = true;
            yax.Held  = yax.Held && can_pan;
            y_hov[i]  = yax.Hovered || plot.Hovered;
            y_held[i] = yax.Held    || plot.Held;
        }
    }

    // cancel due to DND activity
    if (GImGui->DragDropActive || (IO.KeyMods == gp.InputMap.OverrideMod && gp.InputMap.OverrideMod != 0))
        return false;

    // STATE -------------------------------------------------------------------

    const bool axis_equal      = ImHasFlag(plot.Flags, ImPlotFlags_Equal);

    const bool any_x_hov       = plot.Hovered || AnyAxesHovered(&plot.Axes[ImAxis_X1], IMPLOT_NUM_X_AXES);
    const bool any_x_held      = plot.Held    || AnyAxesHeld(&plot.Axes[ImAxis_X1], IMPLOT_NUM_X_AXES);
    const bool any_y_hov       = plot.Hovered || AnyAxesHovered(&plot.Axes[ImAxis_Y1], IMPLOT_NUM_Y_AXES);
    const bool any_y_held      = plot.Held    || AnyAxesHeld(&plot.Axes[ImAxis_Y1], IMPLOT_NUM_Y_AXES);
    const bool any_hov         = any_x_hov    || any_y_hov;
    const bool any_held        = any_x_held   || any_y_held;

    const ImVec2 select_drag   = ImGui::GetMouseDragDelta(gp.InputMap.Select);
    const ImVec2 pan_drag      = ImGui::GetMouseDragDelta(gp.InputMap.Pan);
    const float select_drag_sq = ImLengthSqr(select_drag);
    const float pan_drag_sq    = ImLengthSqr(pan_drag);
    const bool selecting       = plot.Selecting && select_drag_sq > MOUSE_CURSOR_DRAG_THRESHOLD;
    const bool panning         = any_held       && pan_drag_sq    > MOUSE_CURSOR_DRAG_THRESHOLD;

    // CONTEXT MENU -----------------------------------------------------------

    if (IO.MouseReleased[gp.InputMap.Menu] && !plot.ContextLocked)
        gp.OpenContextThisFrame = true;

    if (selecting || panning)
        plot.ContextLocked = true;
    else if (!(IO.MouseDown[gp.InputMap.Menu] || IO.MouseReleased[gp.InputMap.Menu]))
        plot.ContextLocked = false;

    // DRAG INPUT -------------------------------------------------------------

    if (any_held && !plot.Selecting) {
        int drag_direction = 0;
        for (int i = 0; i < IMPLOT_NUM_X_AXES; i++) {
            ImPlotAxis& x_axis = plot.XAxis(i);
            if (x_held[i] && !x_axis.IsInputLocked()) {
                drag_direction |= (1 << 1);
                bool increasing = x_axis.IsInverted() ? IO.MouseDelta.x > 0 : IO.MouseDelta.x < 0;
                if (IO.MouseDelta.x != 0 && !x_axis.IsPanLocked(increasing)) {
                    const double plot_l = x_axis.PixelsToPlot(plot.PlotRect.Min.x - IO.MouseDelta.x);
                    const double plot_r = x_axis.PixelsToPlot(plot.PlotRect.Max.x - IO.MouseDelta.x);
                    x_axis.SetMin(x_axis.IsInverted() ? plot_r : plot_l);
                    x_axis.SetMax(x_axis.IsInverted() ? plot_l : plot_r);
                    if (axis_equal && x_axis.OrthoAxis != NULL)
                        x_axis.OrthoAxis->SetAspect(x_axis.GetAspect());
                    changed = true;
                }
            }
        }
        for (int i = 0; i < IMPLOT_NUM_Y_AXES; i++) {
            ImPlotAxis& y_axis = plot.YAxis(i);
            if (y_held[i] && !y_axis.IsInputLocked()) {
                drag_direction |= (1 << 2);
                bool increasing = y_axis.IsInverted() ? IO.MouseDelta.y < 0 : IO.MouseDelta.y > 0;
                if (IO.MouseDelta.y != 0 && !y_axis.IsPanLocked(increasing)) {
                    const double plot_t = y_axis.PixelsToPlot(plot.PlotRect.Min.y - IO.MouseDelta.y);
                    const double plot_b = y_axis.PixelsToPlot(plot.PlotRect.Max.y - IO.MouseDelta.y);
                    y_axis.SetMin(y_axis.IsInverted() ? plot_t : plot_b);
                    y_axis.SetMax(y_axis.IsInverted() ? plot_b : plot_t);
                    if (axis_equal && y_axis.OrthoAxis != NULL)
                        y_axis.OrthoAxis->SetAspect(y_axis.GetAspect());
                    changed = true;
                }
            }
        }
        if (IO.MouseDragMaxDistanceSqr[gp.InputMap.Pan] > MOUSE_CURSOR_DRAG_THRESHOLD) {
            switch (drag_direction) {
                case 0        : ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed); break;
                case (1 << 1) : ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);   break;
                case (1 << 2) : ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);   break;
                default       : ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);  break;
            }
        }
    }

    // SCROLL INPUT -----------------------------------------------------------

    if (any_hov && IO.MouseWheel != 0 && ImHasFlag(IO.KeyMods, gp.InputMap.ZoomMod)) {

        float zoom_rate = gp.InputMap.ZoomRate;
        if (IO.MouseWheel > 0)
            zoom_rate = (-zoom_rate) / (1.0f + (2.0f * zoom_rate));
        ImVec2 rect_size = plot.PlotRect.GetSize();
        float tx = ImRemap(IO.MousePos.x, plot.PlotRect.Min.x, plot.PlotRect.Max.x, 0.0f, 1.0f);
        float ty = ImRemap(IO.MousePos.y, plot.PlotRect.Min.y, plot.PlotRect.Max.y, 0.0f, 1.0f);

        for (int i = 0; i < IMPLOT_NUM_X_AXES; i++) {
            ImPlotAxis& x_axis = plot.XAxis(i);
            const bool equal_zoom   = axis_equal && x_axis.OrthoAxis != NULL;
            const bool equal_locked = (equal_zoom != false) && x_axis.OrthoAxis->IsInputLocked();
            if (x_hov[i] && !x_axis.IsInputLocked() && !equal_locked) {
                float correction = (plot.Hovered && equal_zoom) ? 0.5f : 1.0f;
                const double plot_l = x_axis.PixelsToPlot(plot.PlotRect.Min.x - rect_size.x * tx * zoom_rate * correction);
                const double plot_r = x_axis.PixelsToPlot(plot.PlotRect.Max.x + rect_size.x * (1 - tx) * zoom_rate * correction);
                x_axis.SetMin(x_axis.IsInverted() ? plot_r : plot_l);
                x_axis.SetMax(x_axis.IsInverted() ? plot_l : plot_r);
                if (axis_equal && x_axis.OrthoAxis != NULL)
                    x_axis.OrthoAxis->SetAspect(x_axis.GetAspect());
                changed = true;
            }
        }
        for (int i = 0; i < IMPLOT_NUM_Y_AXES; i++) {
            ImPlotAxis& y_axis = plot.YAxis(i);
            const bool equal_zoom   = axis_equal && y_axis.OrthoAxis != NULL;
            const bool equal_locked = equal_zoom && y_axis.OrthoAxis->IsInputLocked();
            if (y_hov[i] && !y_axis.IsInputLocked() && !equal_locked) {
                float correction = (plot.Hovered && equal_zoom) ? 0.5f : 1.0f;
                const double plot_t = y_axis.PixelsToPlot(plot.PlotRect.Min.y - rect_size.y * ty * zoom_rate * correction);
                const double plot_b = y_axis.PixelsToPlot(plot.PlotRect.Max.y + rect_size.y * (1 - ty) * zoom_rate * correction);
                y_axis.SetMin(y_axis.IsInverted() ? plot_t : plot_b);
                y_axis.SetMax(y_axis.IsInverted() ? plot_b : plot_t);
                if (axis_equal && y_axis.OrthoAxis != NULL)
                    y_axis.OrthoAxis->SetAspect(y_axis.GetAspect());
                changed = true;
            }
        }
    }

    // BOX-SELECTION ----------------------------------------------------------

    if (plot.Selecting) {
        const ImVec2 d = plot.SelectStart - IO.MousePos;
        const bool x_can_change = !ImHasFlag(IO.KeyMods,gp.InputMap.SelectHorzMod) && ImFabs(d.x) > 2;
        const bool y_can_change = !ImHasFlag(IO.KeyMods,gp.InputMap.SelectVertMod) && ImFabs(d.y) > 2;
        // confirm
        if (IO.MouseReleased[gp.InputMap.Select]) {
            for (int i = 0; i < IMPLOT_NUM_X_AXES; i++) {
                ImPlotAxis& x_axis = plot.XAxis(i);
                if (!x_axis.IsInputLocked() && x_can_change) {
                    const double p1 = x_axis.PixelsToPlot(plot.SelectStart.x);
                    const double p2 = x_axis.PixelsToPlot(IO.MousePos.x);
                    x_axis.SetMin(ImMin(p1, p2));
                    x_axis.SetMax(ImMax(p1, p2));
                    changed = true;
                }
            }
            for (int i = 0; i < IMPLOT_NUM_Y_AXES; i++) {
                ImPlotAxis& y_axis = plot.YAxis(i);
                if (!y_axis.IsInputLocked() && y_can_change) {
                    const double p1 = y_axis.PixelsToPlot(plot.SelectStart.y);
                    const double p2 = y_axis.PixelsToPlot(IO.MousePos.y);
                    y_axis.SetMin(ImMin(p1, p2));
                    y_axis.SetMax(ImMax(p1, p2));
                    changed = true;
                }
            }
            if (x_can_change || y_can_change || (ImHasFlag(IO.KeyMods,gp.InputMap.SelectHorzMod) && ImHasFlag(IO.KeyMods,gp.InputMap.SelectVertMod)))
                gp.OpenContextThisFrame = false;
            plot.Selected = plot.Selecting = false;
        }
        // cancel
        else if (IO.MouseReleased[gp.InputMap.SelectCancel]) {
            plot.Selected = plot.Selecting = false;
            gp.OpenContextThisFrame = false;
        }
        else if (ImLengthSqr(d) > BOX_SELECT_DRAG_THRESHOLD) {
            // bad selection
            if (plot.IsInputLocked()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
                gp.OpenContextThisFrame = false;
                plot.Selected      = false;
            }
            else {
                // TODO: Handle only min or max locked cases
                const bool full_width  = ImHasFlag(IO.KeyMods, gp.InputMap.SelectHorzMod) || AllAxesInputLocked(&plot.Axes[ImAxis_X1], IMPLOT_NUM_X_AXES);
                const bool full_height = ImHasFlag(IO.KeyMods, gp.InputMap.SelectVertMod) || AllAxesInputLocked(&plot.Axes[ImAxis_Y1], IMPLOT_NUM_Y_AXES);
                plot.SelectRect.Min.x = full_width  ? plot.PlotRect.Min.x : ImMin(plot.SelectStart.x, IO.MousePos.x);
                plot.SelectRect.Max.x = full_width  ? plot.PlotRect.Max.x : ImMax(plot.SelectStart.x, IO.MousePos.x);
                plot.SelectRect.Min.y = full_height ? plot.PlotRect.Min.y : ImMin(plot.SelectStart.y, IO.MousePos.y);
                plot.SelectRect.Max.y = full_height ? plot.PlotRect.Max.y : ImMax(plot.SelectStart.y, IO.MousePos.y);
                plot.SelectRect.Min  -= plot.PlotRect.Min;
                plot.SelectRect.Max  -= plot.PlotRect.Min;
                plot.Selected = true;
            }
        }
        else {
            plot.Selected = false;
        }
    }
    return changed;
}

//-----------------------------------------------------------------------------
// Next Plot Data (Legacy)
//-----------------------------------------------------------------------------

void ApplyNextPlotData(ImAxis idx) {
    ImPlotContext& gp = *GImPlot;
    ImPlotPlot& plot  = *GImPlot->CurrentPlot;
    ImPlotAxis& axis  = plot.Axes[idx];
    if (!axis.Enabled)
        return;
    double*     npd_lmin = gp.NextPlotData.LinkedMin[idx];
    double*     npd_lmax = gp.NextPlotData.LinkedMax[idx];
    bool        npd_rngh = gp.NextPlotData.HasRange[idx];
    ImPlotCond  npd_rngc = gp.NextPlotData.RangeCond[idx];
    ImPlotRange     npd_rngv = gp.NextPlotData.Range[idx];
    axis.LinkedMin = npd_lmin;
    axis.LinkedMax = npd_lmax;
    axis.PullLinks();
    if (npd_rngh) {
        if (!plot.Initialized || npd_rngc == ImPlotCond_Always)
            axis.SetRange(npd_rngv);
    }
    axis.HasRange         = npd_rngh;
    axis.RangeCond        = npd_rngc;
}

//-----------------------------------------------------------------------------
// Setup
//-----------------------------------------------------------------------------

void SetupAxis(ImAxis idx, const char* label, ImPlotAxisFlags flags) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                         "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    // get plot and axis
    ImPlotPlot& plot = *GImPlot->CurrentPlot;
    ImPlotAxis& axis = plot.Axes[idx];
    // set ID
    axis.ID = plot.ID + idx + 1;
    // check and set flags
    if (plot.JustCreated || flags != axis.PreviousFlags)
        axis.Flags = flags;
    axis.PreviousFlags = flags;
    // enable axis
    axis.Enabled = true;
    // set label
    plot.SetAxisLabel(axis,label);
    // cache colors
    UpdateAxisColors(axis);
}

void SetupAxisLimits(ImAxis idx, double min_lim, double max_lim, ImPlotCond cond) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                         "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");    // get plot and axis
    ImPlotPlot& plot = *GImPlot->CurrentPlot;
    ImPlotAxis& axis = plot.Axes[idx];
    IM_ASSERT_USER_ERROR(axis.Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    if (!plot.Initialized || cond == ImPlotCond_Always)
        axis.SetRange(min_lim, max_lim);
    axis.HasRange  = true;
    axis.RangeCond = cond;
}

void SetupAxisFormat(ImAxis idx, const char* fmt) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                         "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    ImPlotPlot& plot = *GImPlot->CurrentPlot;
    ImPlotAxis& axis = plot.Axes[idx];
    IM_ASSERT_USER_ERROR(axis.Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    axis.HasFormatSpec = fmt != NULL;
    if (fmt != NULL)
        ImStrncpy(axis.FormatSpec,fmt,sizeof(axis.FormatSpec));
}

void SetupAxisLinks(ImAxis idx, double* min_lnk, double* max_lnk) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                         "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    ImPlotPlot& plot = *GImPlot->CurrentPlot;
    ImPlotAxis& axis = plot.Axes[idx];
    IM_ASSERT_USER_ERROR(axis.Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    axis.LinkedMin = min_lnk;
    axis.LinkedMax = max_lnk;
    axis.PullLinks();
}

void SetupAxisFormat(ImAxis idx, ImPlotFormatter formatter, void* data) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                         "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    ImPlotPlot& plot = *GImPlot->CurrentPlot;
    ImPlotAxis& axis = plot.Axes[idx];
    IM_ASSERT_USER_ERROR(axis.Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    axis.Formatter = formatter;
    axis.FormatterData = data;
}

void SetupAxisTicks(ImAxis idx, const double* values, int n_ticks, const char* const labels[], bool show_default) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                        "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    ImPlotPlot& plot = *GImPlot->CurrentPlot;
    ImPlotAxis& axis = plot.Axes[idx];
    IM_ASSERT_USER_ERROR(axis.Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    axis.ShowDefaultTicks = show_default;
    AddTicksCustom(values,
                  labels,
                  n_ticks,
                  axis.Ticker,
                  axis.Formatter ? axis.Formatter : Formatter_Default,
                  (axis.Formatter && axis.FormatterData) ? axis.FormatterData : axis.HasFormatSpec ? axis.FormatSpec : (void*)IMPLOT_LABEL_FORMAT);
}

void SetupAxisTicks(ImAxis idx, double v_min, double v_max, int n_ticks, const char* const labels[], bool show_default) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                         "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    n_ticks = n_ticks < 2 ? 2 : n_ticks;
    FillRange(GImPlot->TempDouble1, n_ticks, v_min, v_max);
    SetupAxisTicks(idx, GImPlot->TempDouble1.Data, n_ticks, labels, show_default);
}

void SetupAxisScale(ImAxis idx, ImPlotScale scale) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                        "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    ImPlotPlot& plot = *GImPlot->CurrentPlot;
    ImPlotAxis& axis = plot.Axes[idx];
    IM_ASSERT_USER_ERROR(axis.Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    axis.Scale = scale;
    switch (scale)
    {
    case ImPlotScale_Time:
        axis.TransformForward = NULL;
        axis.TransformInverse = NULL;
        axis.TransformData    = NULL;
        axis.Locator          = Locator_Time;
        axis.ConstraintRange  = ImPlotRange(IMPLOT_MIN_TIME, IMPLOT_MAX_TIME);
        axis.Ticker.Levels    = 2;
        break;
    case ImPlotScale_Log10:
        axis.TransformForward = TransformForward_Log10;
        axis.TransformInverse = TransformInverse_Log10;
        axis.TransformData    = NULL;
        axis.Locator          = Locator_Log10;
        axis.ConstraintRange  = ImPlotRange(DBL_MIN, INFINITY);
        break;
    case ImPlotScale_SymLog:
        axis.TransformForward = TransformForward_SymLog;
        axis.TransformInverse = TransformInverse_SymLog;
        axis.TransformData    = NULL;
        axis.Locator          = Locator_SymLog;
        axis.ConstraintRange  = ImPlotRange(-INFINITY, INFINITY);
        break;
    default:
        axis.TransformForward = NULL;
        axis.TransformInverse = NULL;
        axis.TransformData    = NULL;
        axis.Locator          = NULL;
        axis.ConstraintRange  = ImPlotRange(-INFINITY, INFINITY);
        break;
    }
}

void SetupAxisScale(ImAxis idx, ImPlotTransform fwd, ImPlotTransform inv, void* data) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                        "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    ImPlotPlot& plot = *GImPlot->CurrentPlot;
    ImPlotAxis& axis = plot.Axes[idx];
    IM_ASSERT_USER_ERROR(axis.Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    axis.Scale = IMPLOT_AUTO;
    axis.TransformForward = fwd;
    axis.TransformInverse = inv;
    axis.TransformData = data;
}

void SetupAxisLimitsConstraints(ImAxis idx, double v_min, double v_max) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                        "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    ImPlotPlot& plot = *GImPlot->CurrentPlot;
    ImPlotAxis& axis = plot.Axes[idx];
    IM_ASSERT_USER_ERROR(axis.Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    axis.ConstraintRange.Min = v_min;
    axis.ConstraintRange.Max = v_max;
}

void SetupAxisZoomConstraints(ImAxis idx, double z_min, double z_max) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                        "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    ImPlotPlot& plot = *GImPlot->CurrentPlot;
    ImPlotAxis& axis = plot.Axes[idx];
    IM_ASSERT_USER_ERROR(axis.Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    axis.ConstraintZoom.Min = z_min;
    axis.ConstraintZoom.Max = z_max;
}

void SetupAxes(const char* x_label, const char* y_label, ImPlotAxisFlags x_flags, ImPlotAxisFlags y_flags) {
    SetupAxis(ImAxis_X1, x_label, x_flags);
    SetupAxis(ImAxis_Y1, y_label, y_flags);
}

void SetupAxesLimits(double x_min, double x_max, double y_min, double y_max, ImPlotCond cond) {
    SetupAxisLimits(ImAxis_X1, x_min, x_max, cond);
    SetupAxisLimits(ImAxis_Y1, y_min, y_max, cond);
}

void SetupLegend(ImPlotLocation location, ImPlotLegendFlags flags) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                         "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    IM_ASSERT_USER_ERROR(GImPlot->CurrentItems != NULL,
                         "SetupLegend() needs to be called within an itemized context!");
    ImPlotLegend& legend = GImPlot->CurrentItems->Legend;
    // check and set location
    if (location != legend.PreviousLocation)
        legend.Location = location;
    legend.PreviousLocation = location;
    // check and set flags
    if (flags != legend.PreviousFlags)
        legend.Flags = flags;
    legend.PreviousFlags = flags;
}

void SetupMouseText(ImPlotLocation location, ImPlotMouseTextFlags flags) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL && !GImPlot->CurrentPlot->SetupLocked,
                         "Setup needs to be called after BeginPlot and before any setup locking functions (e.g. PlotX)!");
    GImPlot->CurrentPlot->MouseTextLocation = location;
    GImPlot->CurrentPlot->MouseTextFlags = flags;
}

//-----------------------------------------------------------------------------
// SetNext
//-----------------------------------------------------------------------------

void SetNextAxisLimits(ImAxis axis, double v_min, double v_max, ImPlotCond cond) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot == NULL, "SetNextAxisLimits() needs to be called before BeginPlot()!");
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    gp.NextPlotData.HasRange[axis]  = true;
    gp.NextPlotData.RangeCond[axis] = cond;
    gp.NextPlotData.Range[axis].Min = v_min;
    gp.NextPlotData.Range[axis].Max = v_max;
}

void SetNextAxisLinks(ImAxis axis, double* link_min, double* link_max) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot == NULL, "SetNextAxisLinks() needs to be called before BeginPlot()!");
    gp.NextPlotData.LinkedMin[axis] = link_min;
    gp.NextPlotData.LinkedMax[axis] = link_max;
}

void SetNextAxisToFit(ImAxis axis) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot == NULL, "SetNextAxisToFit() needs to be called before BeginPlot()!");
    gp.NextPlotData.Fit[axis] = true;
}

void SetNextAxesLimits(double x_min, double x_max, double y_min, double y_max, ImPlotCond cond) {
    SetNextAxisLimits(ImAxis_X1, x_min, x_max, cond);
    SetNextAxisLimits(ImAxis_Y1, y_min, y_max, cond);
}

void SetNextAxesToFit() {
    for (int i = 0; i < ImAxis_COUNT; ++i)
        SetNextAxisToFit(i);
}

//-----------------------------------------------------------------------------
// BeginPlot
//-----------------------------------------------------------------------------

bool BeginPlot(const char* title_id, const ImVec2& size, ImPlotFlags flags) {
    IM_ASSERT_USER_ERROR(GImPlot != NULL, "No current context. Did you call ImPlot::CreateContext() or ImPlot::SetCurrentContext()?");
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot == NULL, "Mismatched BeginPlot()/EndPlot()!");

    // FRONT MATTER -----------------------------------------------------------

    if (GImPlot->CurrentSubplot != NULL)
        ImGui::PushID(GImPlot->CurrentSubplot->CurrentIdx);

    // get globals
    ImPlotContext& gp        = *GImPlot;
    ImGuiContext &G          = *GImGui;
    ImGuiWindow* Window      = G.CurrentWindow;

    // skip if needed
    if (Window->SkipItems && !gp.CurrentSubplot) {
        ResetCtxForNextPlot(GImPlot);
        return false;
    }

    // ID and age (TODO: keep track of plot age in frames)
    const ImGuiID ID         = Window->GetID(title_id);
    const bool just_created  = gp.Plots.GetByKey(ID) == NULL;
    gp.CurrentPlot           = gp.Plots.GetOrAddByKey(ID);

    ImPlotPlot &plot         = *gp.CurrentPlot;
    plot.ID                  = ID;
    plot.Items.ID            = ID - 1;
    plot.JustCreated         = just_created;
    plot.SetupLocked         = false;

    // check flags
    if (plot.JustCreated)
        plot.Flags = flags;
    else if (flags != plot.PreviousFlags)
        plot.Flags = flags;
    plot.PreviousFlags = flags;

    // setup default axes
    if (plot.JustCreated) {
        SetupAxis(ImAxis_X1);
        SetupAxis(ImAxis_Y1);
    }

    // reset axes
    for (int i = 0; i < ImAxis_COUNT; ++i) {
        plot.Axes[i].Reset();
        UpdateAxisColors(plot.Axes[i]);
    }
    // ensure first axes enabled
    plot.Axes[ImAxis_X1].Enabled = true;
    plot.Axes[ImAxis_Y1].Enabled = true;
    // set initial axes
    plot.CurrentX = ImAxis_X1;
    plot.CurrentY = ImAxis_Y1;

    // process next plot data (legacy)
    for (int i = 0; i < ImAxis_COUNT; ++i)
        ApplyNextPlotData(i);

    // capture scroll with a child region
    if (!ImHasFlag(plot.Flags, ImPlotFlags_NoChild)) {
        ImVec2 child_size;
        if (gp.CurrentSubplot != NULL)
            child_size = gp.CurrentSubplot->CellSize;
        else
            child_size = ImVec2(size.x == 0 ? gp.Style.PlotDefaultSize.x : size.x, size.y == 0 ? gp.Style.PlotDefaultSize.y : size.y);
        ImGui::BeginChild(title_id, child_size, false, ImGuiWindowFlags_NoScrollbar);
        Window = ImGui::GetCurrentWindow();
        Window->ScrollMax.y = 1.0f;
        gp.ChildWindowMade = true;
    }
    else {
        gp.ChildWindowMade = false;
    }

    // clear text buffers
    plot.ClearTextBuffer();
    plot.SetTitle(title_id);

    // set frame size
    ImVec2 frame_size;
    if (gp.CurrentSubplot != NULL)
        frame_size = gp.CurrentSubplot->CellSize;
    else
        frame_size = ImGui::CalcItemSize(size, gp.Style.PlotDefaultSize.x, gp.Style.PlotDefaultSize.y);

    if (frame_size.x < gp.Style.PlotMinSize.x && (size.x < 0.0f || gp.CurrentSubplot != NULL))
        frame_size.x = gp.Style.PlotMinSize.x;
    if (frame_size.y < gp.Style.PlotMinSize.y && (size.y < 0.0f || gp.CurrentSubplot != NULL))
        frame_size.y = gp.Style.PlotMinSize.y;

    plot.FrameRect = ImRect(Window->DC.CursorPos, Window->DC.CursorPos + frame_size);
    ImGui::ItemSize(plot.FrameRect);
    if (!ImGui::ItemAdd(plot.FrameRect, plot.ID, &plot.FrameRect) && !gp.CurrentSubplot) {
        ResetCtxForNextPlot(GImPlot);
        return false;
    }

    // setup items (or dont)
    if (gp.CurrentItems == NULL)
        gp.CurrentItems = &plot.Items;

    return true;
}

//-----------------------------------------------------------------------------
// SetupFinish
//-----------------------------------------------------------------------------

void SetupFinish() {
    IM_ASSERT_USER_ERROR(GImPlot != NULL, "No current context. Did you call ImPlot::CreateContext() or ImPlot::SetCurrentContext()?");
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "SetupFinish needs to be called after BeginPlot!");

    ImPlotContext& gp       = *GImPlot;
    ImGuiContext& G         = *GImGui;
    ImDrawList& DrawList    = *G.CurrentWindow->DrawList;
    const ImGuiStyle& Style = G.Style;

    ImPlotPlot &plot  = *gp.CurrentPlot;

    // lock setup
    plot.SetupLocked = true;

    // finalize axes and set default formatter/locator
    for (int i = 0; i < ImAxis_COUNT; ++i) {
        ImPlotAxis& axis = plot.Axes[i];
        if (axis.Enabled) {
            axis.Constrain();
            if (!plot.Initialized && axis.CanInitFit())
                plot.FitThisFrame = axis.FitThisFrame = true;
        }
        if (axis.Formatter == NULL) {
            axis.Formatter = Formatter_Default;
            if (axis.HasFormatSpec)
                axis.FormatterData = axis.FormatSpec;
            else
                axis.FormatterData = (void*)IMPLOT_LABEL_FORMAT;
        }
        if (axis.Locator == NULL) {
            axis.Locator = Locator_Default;
        }
    }

    // setup NULL orthogonal axes
    const bool axis_equal = ImHasFlag(plot.Flags, ImPlotFlags_Equal);
    for (int ix = ImAxis_X1, iy = ImAxis_Y1; ix < ImAxis_Y1 || iy < ImAxis_COUNT; ++ix, ++iy) {
        ImPlotAxis& x_axis = plot.Axes[ix];
        ImPlotAxis& y_axis = plot.Axes[iy];
        if (x_axis.Enabled && y_axis.Enabled) {
            if (x_axis.OrthoAxis == NULL)
                x_axis.OrthoAxis = &y_axis;
            if (y_axis.OrthoAxis == NULL)
                y_axis.OrthoAxis = &x_axis;
        }
        else if (x_axis.Enabled)
        {
            if (x_axis.OrthoAxis == NULL && !axis_equal)
                x_axis.OrthoAxis = &plot.Axes[ImAxis_Y1];
        }
        else if (y_axis.Enabled) {
            if (y_axis.OrthoAxis == NULL && !axis_equal)
                y_axis.OrthoAxis = &plot.Axes[ImAxis_X1];
        }
    }

    // canvas/axes bb
    plot.CanvasRect = ImRect(plot.FrameRect.Min + gp.Style.PlotPadding, plot.FrameRect.Max - gp.Style.PlotPadding);
    plot.AxesRect   = plot.FrameRect;

    // outside legend adjustments
    if (!ImHasFlag(plot.Flags, ImPlotFlags_NoLegend) && plot.Items.GetLegendCount() > 0 && ImHasFlag(plot.Items.Legend.Flags, ImPlotLegendFlags_Outside)) {
        ImPlotLegend& legend = plot.Items.Legend;
        const bool horz = ImHasFlag(legend.Flags, ImPlotLegendFlags_Horizontal);
        const ImVec2 legend_size = CalcLegendSize(plot.Items, gp.Style.LegendInnerPadding, gp.Style.LegendSpacing, !horz);
        const bool west = ImHasFlag(legend.Location, ImPlotLocation_West) && !ImHasFlag(legend.Location, ImPlotLocation_East);
        const bool east = ImHasFlag(legend.Location, ImPlotLocation_East) && !ImHasFlag(legend.Location, ImPlotLocation_West);
        const bool north = ImHasFlag(legend.Location, ImPlotLocation_North) && !ImHasFlag(legend.Location, ImPlotLocation_South);
        const bool south = ImHasFlag(legend.Location, ImPlotLocation_South) && !ImHasFlag(legend.Location, ImPlotLocation_North);
        if ((west && !horz) || (west && horz && !north && !south)) {
            plot.CanvasRect.Min.x += (legend_size.x + gp.Style.LegendPadding.x);
            plot.AxesRect.Min.x   += (legend_size.x + gp.Style.PlotPadding.x);
        }
        if ((east && !horz) || (east && horz && !north && !south)) {
            plot.CanvasRect.Max.x -= (legend_size.x + gp.Style.LegendPadding.x);
            plot.AxesRect.Max.x   -= (legend_size.x + gp.Style.PlotPadding.x);
        }
        if ((north && horz) || (north && !horz && !west && !east)) {
            plot.CanvasRect.Min.y += (legend_size.y + gp.Style.LegendPadding.y);
            plot.AxesRect.Min.y   += (legend_size.y + gp.Style.PlotPadding.y);
        }
        if ((south && horz) || (south && !horz && !west && !east)) {
            plot.CanvasRect.Max.y -= (legend_size.y + gp.Style.LegendPadding.y);
            plot.AxesRect.Max.y   -= (legend_size.y + gp.Style.PlotPadding.y);
        }
    }

    // plot bb
    float pad_top = 0, pad_bot = 0, pad_left = 0, pad_right = 0;

    // (0) calc top padding form title
    ImVec2 title_size(0.0f, 0.0f);
    if (plot.HasTitle())
         title_size = ImGui::CalcTextSize(plot.GetTitle(), NULL, true);
    if (title_size.x > 0) {
        pad_top += title_size.y + gp.Style.LabelPadding.y;
        plot.AxesRect.Min.y += gp.Style.PlotPadding.y + pad_top;
    }

    // (1) calc addition top padding and bot padding
    PadAndDatumAxesX(plot,pad_top,pad_bot,gp.CurrentAlignmentH);

    const float plot_height = plot.CanvasRect.GetHeight() - pad_top - pad_bot;

    // (2) get y tick labels (needed for left/right pad)
    for (int i = 0; i < IMPLOT_NUM_Y_AXES; i++) {
        ImPlotAxis& axis = plot.YAxis(i);
        if (axis.WillRender() && axis.ShowDefaultTicks) {
            axis.Locator(axis.Ticker, axis.Range, plot_height, true, axis.Formatter, axis.FormatterData);
        }
    }

    // (3) calc left/right pad
    PadAndDatumAxesY(plot,pad_left,pad_right,gp.CurrentAlignmentV);

    const float plot_width = plot.CanvasRect.GetWidth() - pad_left - pad_right;

    // (4) get x ticks
    for (int i = 0; i < IMPLOT_NUM_X_AXES; i++) {
        ImPlotAxis& axis = plot.XAxis(i);
        if (axis.WillRender() && axis.ShowDefaultTicks) {
            axis.Locator(axis.Ticker, axis.Range, plot_width, false, axis.Formatter, axis.FormatterData);
        }
    }

    // (5) calc plot bb
    plot.PlotRect = ImRect(plot.CanvasRect.Min + ImVec2(pad_left, pad_top), plot.CanvasRect.Max - ImVec2(pad_right, pad_bot));

    // HOVER------------------------------------------------------------

    // axes hover rect, pixel ranges
    for (int i = 0; i < IMPLOT_NUM_X_AXES; ++i) {
        ImPlotAxis& xax = plot.XAxis(i);
        xax.HoverRect   = ImRect(ImVec2(plot.PlotRect.Min.x, ImMin(xax.Datum1,xax.Datum2)),
                                 ImVec2(plot.PlotRect.Max.x, ImMax(xax.Datum1,xax.Datum2)));
        xax.PixelMin    = xax.IsInverted() ? plot.PlotRect.Max.x : plot.PlotRect.Min.x;
        xax.PixelMax    = xax.IsInverted() ? plot.PlotRect.Min.x : plot.PlotRect.Max.x;
        xax.UpdateTransformCache();
    }

    for (int i = 0; i < IMPLOT_NUM_Y_AXES; ++i) {
        ImPlotAxis& yax = plot.YAxis(i);
        yax.HoverRect   = ImRect(ImVec2(ImMin(yax.Datum1,yax.Datum2),plot.PlotRect.Min.y),
                                 ImVec2(ImMax(yax.Datum1,yax.Datum2),plot.PlotRect.Max.y));
        yax.PixelMin    = yax.IsInverted() ? plot.PlotRect.Min.y : plot.PlotRect.Max.y;
        yax.PixelMax    = yax.IsInverted() ? plot.PlotRect.Max.y : plot.PlotRect.Min.y;
        yax.UpdateTransformCache();
    }
    // Equal axis constraint. Must happen after we set Pixels
    // constrain equal axes for primary x and y if not approximately equal
    // constrains x to y since x pixel size depends on y labels width, and causes feedback loops in opposite case
    if (axis_equal) {
        for (int i = 0; i < IMPLOT_NUM_X_AXES; ++i) {
            ImPlotAxis& x_axis = plot.XAxis(i);
            if (x_axis.OrthoAxis == NULL)
                continue;
            double xar = x_axis.GetAspect();
            double yar = x_axis.OrthoAxis->GetAspect();
            // edge case: user has set x range this frame, so fit y to x so that we honor their request for x range
            // NB: because of feedback across several frames, the user's x request may not be perfectly honored
            if (x_axis.HasRange)
                x_axis.OrthoAxis->SetAspect(xar);
            else if (!ImAlmostEqual(xar,yar) && !x_axis.OrthoAxis->IsInputLocked())
                 x_axis.SetAspect(yar);
        }
    }

    // INPUT ------------------------------------------------------------------
    if (!ImHasFlag(plot.Flags, ImPlotFlags_NoInputs))
        UpdateInput(plot);

    // fit from FitNextPlotAxes or auto fit
    for (int i = 0; i < ImAxis_COUNT; ++i) {
        if (gp.NextPlotData.Fit[i] || plot.Axes[i].IsAutoFitting()) {
            plot.FitThisFrame = true;
            plot.Axes[i].FitThisFrame = true;
        }
    }

    // RENDER -----------------------------------------------------------------

    const float txt_height = ImGui::GetTextLineHeight();

    // render frame
    if (!ImHasFlag(plot.Flags, ImPlotFlags_NoFrame))
        ImGui::RenderFrame(plot.FrameRect.Min, plot.FrameRect.Max, GetStyleColorU32(ImPlotCol_FrameBg), true, Style.FrameRounding);

    // grid bg
    DrawList.AddRectFilled(plot.PlotRect.Min, plot.PlotRect.Max, GetStyleColorU32(ImPlotCol_PlotBg));

    // transform ticks
    for (int i = 0; i < ImAxis_COUNT; i++) {
        ImPlotAxis& axis = plot.Axes[i];
        if (axis.WillRender()) {
            for (int t = 0; t < axis.Ticker.TickCount(); t++) {
                ImPlotTick& tk = axis.Ticker.Ticks[t];
                tk.PixelPos = IM_ROUND(axis.PlotToPixels(tk.PlotPos));
            }
        }
    }

    // render grid (background)
    for (int i = 0; i < IMPLOT_NUM_X_AXES; i++) {
        ImPlotAxis& x_axis = plot.XAxis(i);
        if (x_axis.Enabled && x_axis.HasGridLines() && !x_axis.IsForeground())
            RenderGridLinesX(DrawList, x_axis.Ticker, plot.PlotRect, x_axis.ColorMaj, x_axis.ColorMin, gp.Style.MajorGridSize.x, gp.Style.MinorGridSize.x);
    }
    for (int i = 0; i < IMPLOT_NUM_Y_AXES; i++) {
        ImPlotAxis& y_axis = plot.YAxis(i);
        if (y_axis.Enabled && y_axis.HasGridLines() && !y_axis.IsForeground())
            RenderGridLinesY(DrawList, y_axis.Ticker, plot.PlotRect,  y_axis.ColorMaj, y_axis.ColorMin, gp.Style.MajorGridSize.y, gp.Style.MinorGridSize.y);
    }

    // render x axis button, label, tick labels
    for (int i = 0; i < IMPLOT_NUM_X_AXES; i++) {
        ImPlotAxis& ax = plot.XAxis(i);
        if (!ax.Enabled)
            continue;
        if ((ax.Hovered || ax.Held) && !plot.Held && !ImHasFlag(ax.Flags, ImPlotAxisFlags_NoHighlight))
            DrawList.AddRectFilled(ax.HoverRect.Min, ax.HoverRect.Max, ax.Held ? ax.ColorAct : ax.ColorHov);
        else if (ax.ColorHiLi != IM_COL32_BLACK_TRANS) {
            DrawList.AddRectFilled(ax.HoverRect.Min, ax.HoverRect.Max, ax.ColorHiLi);
            ax.ColorHiLi = IM_COL32_BLACK_TRANS;
        }
        else if (ax.ColorBg != IM_COL32_BLACK_TRANS) {
            DrawList.AddRectFilled(ax.HoverRect.Min, ax.HoverRect.Max, ax.ColorBg);
        }
        const ImPlotTicker& tkr = ax.Ticker;
        const bool opp = ax.IsOpposite();
        if (ax.HasLabel()) {
            const char* label        = plot.GetAxisLabel(ax);
            const ImVec2 label_size  = ImGui::CalcTextSize(label);
            const float label_offset = (ax.HasTickLabels() ? tkr.MaxSize.y + gp.Style.LabelPadding.y : 0.0f)
                                     + (tkr.Levels - 1) * (txt_height + gp.Style.LabelPadding.y)
                                     + gp.Style.LabelPadding.y;
            const ImVec2 label_pos(plot.PlotRect.GetCenter().x - label_size.x * 0.5f,
                                   opp ? ax.Datum1 - label_offset - label_size.y : ax.Datum1 + label_offset);
            DrawList.AddText(label_pos, ax.ColorTxt, label);
        }
        if (ax.HasTickLabels()) {
            for (int j = 0; j < tkr.TickCount(); ++j) {
                const ImPlotTick& tk = tkr.Ticks[j];
                const float datum = ax.Datum1 + (opp ? (-gp.Style.LabelPadding.y -txt_height -tk.Level * (txt_height + gp.Style.LabelPadding.y))
                                                     : gp.Style.LabelPadding.y + tk.Level * (txt_height + gp.Style.LabelPadding.y));
                if (tk.ShowLabel && tk.PixelPos >= plot.PlotRect.Min.x - 1 && tk.PixelPos <= plot.PlotRect.Max.x + 1) {
                    ImVec2 start(tk.PixelPos - 0.5f * tk.LabelSize.x, datum);
                    DrawList.AddText(start, ax.ColorTxt, tkr.GetText(j));
                }
            }
        }
    }

    // render y axis button, label, tick labels
    for (int i = 0; i < IMPLOT_NUM_Y_AXES; i++) {
        ImPlotAxis& ax = plot.YAxis(i);
        if (!ax.Enabled)
            continue;
        if ((ax.Hovered || ax.Held) && !plot.Held && !ImHasFlag(ax.Flags, ImPlotAxisFlags_NoHighlight))
            DrawList.AddRectFilled(ax.HoverRect.Min, ax.HoverRect.Max, ax.Held ? ax.ColorAct : ax.ColorHov);
        else if (ax.ColorHiLi != IM_COL32_BLACK_TRANS) {
            DrawList.AddRectFilled(ax.HoverRect.Min, ax.HoverRect.Max, ax.ColorHiLi);
            ax.ColorHiLi = IM_COL32_BLACK_TRANS;
        }
        else if (ax.ColorBg != IM_COL32_BLACK_TRANS) {
            DrawList.AddRectFilled(ax.HoverRect.Min, ax.HoverRect.Max, ax.ColorBg);
        }
        const ImPlotTicker& tkr = ax.Ticker;
        const bool opp = ax.IsOpposite();
        if (ax.HasLabel()) {
            const char* label        = plot.GetAxisLabel(ax);
            const ImVec2 label_size  = CalcTextSizeVertical(label);
            const float label_offset = (ax.HasTickLabels() ? tkr.MaxSize.x + gp.Style.LabelPadding.x : 0.0f)
                                     + gp.Style.LabelPadding.x;
            const ImVec2 label_pos(opp ? ax.Datum1 + label_offset : ax.Datum1 - label_offset - label_size.x,
                                   plot.PlotRect.GetCenter().y + label_size.y * 0.5f);
            AddTextVertical(&DrawList, label_pos, ax.ColorTxt, label);
        }
        if (ax.HasTickLabels()) {
            for (int j = 0; j < tkr.TickCount(); ++j) {
                const ImPlotTick& tk = tkr.Ticks[j];
                const float datum = ax.Datum1 + (opp ? gp.Style.LabelPadding.x : (-gp.Style.LabelPadding.x - tk.LabelSize.x));
                if (tk.ShowLabel && tk.PixelPos >= plot.PlotRect.Min.y - 1 && tk.PixelPos <= plot.PlotRect.Max.y + 1) {
                    ImVec2 start(datum, tk.PixelPos - 0.5f * tk.LabelSize.y);
                    DrawList.AddText(start, ax.ColorTxt, tkr.GetText(j));
                }
            }
        }
    }


    // clear legend (TODO: put elsewhere)
    plot.Items.Legend.Reset();
    // push ID to set item hashes (NB: !!!THIS PROBABLY NEEDS TO BE IN BEGIN PLOT!!!!)
    ImGui::PushOverrideID(gp.CurrentItems->ID);
}

//-----------------------------------------------------------------------------
// EndPlot()
//-----------------------------------------------------------------------------

void EndPlot() {
    IM_ASSERT_USER_ERROR(GImPlot != NULL, "No current context. Did you call ImPlot::CreateContext() or ImPlot::SetCurrentContext()?");
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "Mismatched BeginPlot()/EndPlot()!");

    SetupLock();

    ImPlotContext& gp     = *GImPlot;
    ImGuiContext &G       = *GImGui;
    ImPlotPlot &plot      = *gp.CurrentPlot;
    ImGuiWindow * Window  = G.CurrentWindow;
    ImDrawList & DrawList = *Window->DrawList;
    const ImGuiIO &   IO  = ImGui::GetIO();

    // FINAL RENDER -----------------------------------------------------------

    const bool render_border  = gp.Style.PlotBorderSize > 0 && gp.Style.Colors[ImPlotCol_PlotBorder].w > 0;
    const bool any_x_held = plot.Held    || AnyAxesHeld(&plot.Axes[ImAxis_X1], IMPLOT_NUM_X_AXES);
    const bool any_y_held = plot.Held    || AnyAxesHeld(&plot.Axes[ImAxis_Y1], IMPLOT_NUM_Y_AXES);

    ImGui::PushClipRect(plot.FrameRect.Min, plot.FrameRect.Max, true);

    // render grid (foreground)
    for (int i = 0; i < IMPLOT_NUM_X_AXES; i++) {
        ImPlotAxis& x_axis = plot.XAxis(i);
        if (x_axis.Enabled && x_axis.HasGridLines() && x_axis.IsForeground())
            RenderGridLinesX(DrawList, x_axis.Ticker, plot.PlotRect, x_axis.ColorMaj, x_axis.ColorMin, gp.Style.MajorGridSize.x, gp.Style.MinorGridSize.x);
    }
    for (int i = 0; i < IMPLOT_NUM_Y_AXES; i++) {
        ImPlotAxis& y_axis = plot.YAxis(i);
        if (y_axis.Enabled && y_axis.HasGridLines() && y_axis.IsForeground())
            RenderGridLinesY(DrawList, y_axis.Ticker, plot.PlotRect,  y_axis.ColorMaj, y_axis.ColorMin, gp.Style.MajorGridSize.y, gp.Style.MinorGridSize.y);
    }


    // render title
    if (plot.HasTitle()) {
        ImU32 col = GetStyleColorU32(ImPlotCol_TitleText);
        AddTextCentered(&DrawList,ImVec2(plot.PlotRect.GetCenter().x, plot.CanvasRect.Min.y),col,plot.GetTitle());
    }

    // render x ticks
    int count_B = 0, count_T = 0;
    for (int i = 0; i < IMPLOT_NUM_X_AXES; i++) {
        const ImPlotAxis& ax = plot.XAxis(i);
        if (!ax.Enabled)
            continue;
        const ImPlotTicker& tkr = ax.Ticker;
        const bool opp = ax.IsOpposite();
        const bool aux = ((opp && count_T > 0)||(!opp && count_B > 0));
        if (ax.HasTickMarks()) {
            const float direction = opp ? 1.0f : -1.0f;
            for (int j = 0; j < tkr.TickCount(); ++j) {
                const ImPlotTick& tk = tkr.Ticks[j];
                if (tk.Level != 0 || tk.PixelPos < plot.PlotRect.Min.x || tk.PixelPos > plot.PlotRect.Max.x)
                    continue;
                const ImVec2 start(tk.PixelPos, ax.Datum1);
                const float len = (!aux && tk.Major) ? gp.Style.MajorTickLen.x  : gp.Style.MinorTickLen.x;
                const float thk = (!aux && tk.Major) ? gp.Style.MajorTickSize.x : gp.Style.MinorTickSize.x;
                DrawList.AddLine(start, start + ImVec2(0,direction*len), ax.ColorTick, thk);
            }
            if (aux || !render_border)
                DrawList.AddLine(ImVec2(plot.PlotRect.Min.x,ax.Datum1), ImVec2(plot.PlotRect.Max.x,ax.Datum1), ax.ColorTick, gp.Style.MinorTickSize.x);
        }
        count_B += !opp;
        count_T +=  opp;
    }

    // render y ticks
    int count_L = 0, count_R = 0;
    for (int i = 0; i < IMPLOT_NUM_Y_AXES; i++) {
        const ImPlotAxis& ax = plot.YAxis(i);
        if (!ax.Enabled)
            continue;
        const ImPlotTicker& tkr = ax.Ticker;
        const bool opp = ax.IsOpposite();
        const bool aux = ((opp && count_R > 0)||(!opp && count_L > 0));
        if (ax.HasTickMarks()) {
            const float direction = opp ? -1.0f : 1.0f;
            for (int j = 0; j < tkr.TickCount(); ++j) {
                const ImPlotTick& tk = tkr.Ticks[j];
                if (tk.Level != 0 || tk.PixelPos < plot.PlotRect.Min.y || tk.PixelPos > plot.PlotRect.Max.y)
                    continue;
                const ImVec2 start(ax.Datum1, tk.PixelPos);
                const float len = (!aux && tk.Major) ? gp.Style.MajorTickLen.y  : gp.Style.MinorTickLen.y;
                const float thk = (!aux && tk.Major) ? gp.Style.MajorTickSize.y : gp.Style.MinorTickSize.y;
                DrawList.AddLine(start, start + ImVec2(direction*len,0), ax.ColorTick, thk);
            }
            if (aux || !render_border)
                DrawList.AddLine(ImVec2(ax.Datum1, plot.PlotRect.Min.y), ImVec2(ax.Datum1, plot.PlotRect.Max.y), ax.ColorTick, gp.Style.MinorTickSize.y);
        }
        count_L += !opp;
        count_R +=  opp;
    }
    ImGui::PopClipRect();

    // render annotations
    PushPlotClipRect();
    for (int i = 0; i < gp.Annotations.Size; ++i) {
        const char* txt       = gp.Annotations.GetText(i);
        ImPlotAnnotation& an  = gp.Annotations.Annotations[i];
        const ImVec2 txt_size = ImGui::CalcTextSize(txt);
        const ImVec2 size     = txt_size + gp.Style.AnnotationPadding * 2;
        ImVec2 pos            = an.Pos;
        if (an.Offset.x == 0)
            pos.x -= size.x / 2;
        else if (an.Offset.x > 0)
            pos.x += an.Offset.x;
        else
            pos.x -= size.x - an.Offset.x;
        if (an.Offset.y == 0)
            pos.y -= size.y / 2;
        else if (an.Offset.y > 0)
            pos.y += an.Offset.y;
        else
            pos.y -= size.y - an.Offset.y;
        if (an.Clamp)
            pos = ClampLabelPos(pos, size, plot.PlotRect.Min, plot.PlotRect.Max);
        ImRect rect(pos,pos+size);
        if (an.Offset.x != 0 || an.Offset.y != 0) {
            ImVec2 corners[4] = {rect.GetTL(), rect.GetTR(), rect.GetBR(), rect.GetBL()};
            int min_corner = 0;
            float min_len = FLT_MAX;
            for (int c = 0; c < 4; ++c) {
                float len = ImLengthSqr(an.Pos - corners[c]);
                if (len < min_len) {
                    min_corner = c;
                    min_len = len;
                }
            }
            DrawList.AddLine(an.Pos, corners[min_corner], an.ColorBg);
        }
        DrawList.AddRectFilled(rect.Min, rect.Max, an.ColorBg);
        DrawList.AddText(pos + gp.Style.AnnotationPadding, an.ColorFg, txt);
    }

    // render selection
    if (plot.Selected)
        RenderSelectionRect(DrawList, plot.SelectRect.Min + plot.PlotRect.Min, plot.SelectRect.Max + plot.PlotRect.Min, GetStyleColorVec4(ImPlotCol_Selection));

    // render crosshairs
    if (ImHasFlag(plot.Flags, ImPlotFlags_Crosshairs) && plot.Hovered && !(any_x_held || any_y_held) && !plot.Selecting && !plot.Items.Legend.Hovered) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        ImVec2 xy = IO.MousePos;
        ImVec2 h1(plot.PlotRect.Min.x, xy.y);
        ImVec2 h2(xy.x - 5, xy.y);
        ImVec2 h3(xy.x + 5, xy.y);
        ImVec2 h4(plot.PlotRect.Max.x, xy.y);
        ImVec2 v1(xy.x, plot.PlotRect.Min.y);
        ImVec2 v2(xy.x, xy.y - 5);
        ImVec2 v3(xy.x, xy.y + 5);
        ImVec2 v4(xy.x, plot.PlotRect.Max.y);
        ImU32 col = GetStyleColorU32(ImPlotCol_Crosshairs);
        DrawList.AddLine(h1, h2, col);
        DrawList.AddLine(h3, h4, col);
        DrawList.AddLine(v1, v2, col);
        DrawList.AddLine(v3, v4, col);
    }

    // render mouse pos
    if (!ImHasFlag(plot.Flags, ImPlotFlags_NoMouseText) && (plot.Hovered || ImHasFlag(plot.MouseTextFlags, ImPlotMouseTextFlags_ShowAlways))) {

        const bool no_aux = ImHasFlag(plot.MouseTextFlags, ImPlotMouseTextFlags_NoAuxAxes);
        const bool no_fmt = ImHasFlag(plot.MouseTextFlags, ImPlotMouseTextFlags_NoFormat);

        ImGuiTextBuffer& builder = gp.MousePosStringBuilder;
        builder.Buf.shrink(0);
        char buff[IMPLOT_LABEL_MAX_SIZE];

        const int num_x = no_aux ? 1 : IMPLOT_NUM_X_AXES;
        for (int i = 0; i < num_x; ++i) {
            ImPlotAxis& x_axis = plot.XAxis(i);
            if (!x_axis.Enabled)
                continue;
            if (i > 0)
                builder.append(", (");
            double v = x_axis.PixelsToPlot(IO.MousePos.x);
            if (no_fmt)
                Formatter_Default(v,buff,IMPLOT_LABEL_MAX_SIZE,(void*)IMPLOT_LABEL_FORMAT);
            else
                LabelAxisValue(x_axis,v,buff,IMPLOT_LABEL_MAX_SIZE,true);
            builder.append(buff);
            if (i > 0)
                builder.append(")");
        }
        builder.append(", ");
        const int num_y = no_aux ? 1 : IMPLOT_NUM_Y_AXES;
        for (int i = 0; i < num_y; ++i) {
            ImPlotAxis& y_axis = plot.YAxis(i);
            if (!y_axis.Enabled)
                continue;
            if (i > 0)
                builder.append(", (");
            double v = y_axis.PixelsToPlot(IO.MousePos.y);
            if (no_fmt)
                Formatter_Default(v,buff,IMPLOT_LABEL_MAX_SIZE,(void*)IMPLOT_LABEL_FORMAT);
            else
                LabelAxisValue(y_axis,v,buff,IMPLOT_LABEL_MAX_SIZE,true);
            builder.append(buff);
            if (i > 0)
                builder.append(")");
        }

        if (!builder.empty()) {
            const ImVec2 size = ImGui::CalcTextSize(builder.c_str());
            const ImVec2 pos = GetLocationPos(plot.PlotRect, size, plot.MouseTextLocation, gp.Style.MousePosPadding);
            DrawList.AddText(pos, GetStyleColorU32(ImPlotCol_InlayText), builder.c_str());
        }
    }
    PopPlotClipRect();

    // axis side switch
    if (!plot.Held) {
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        ImRect trigger_rect = plot.PlotRect;
        trigger_rect.Expand(-10);
        for (int i = 0; i < IMPLOT_NUM_X_AXES; ++i) {
            ImPlotAxis& x_axis = plot.XAxis(i);
            if (ImHasFlag(x_axis.Flags, ImPlotAxisFlags_NoSideSwitch))
                continue;
            if (x_axis.Held && plot.PlotRect.Contains(mouse_pos)) {
                const bool opp = ImHasFlag(x_axis.Flags, ImPlotAxisFlags_Opposite);
                if (!opp) {
                    ImRect rect(plot.PlotRect.Min.x - 5, plot.PlotRect.Min.y - 5,
                                plot.PlotRect.Max.x + 5, plot.PlotRect.Min.y + 5);
                    if (mouse_pos.y < plot.PlotRect.Max.y - 10)
                        DrawList.AddRectFilled(rect.Min, rect.Max, x_axis.ColorHov);
                    if (rect.Contains(mouse_pos))
                        x_axis.Flags |= ImPlotAxisFlags_Opposite;
                }
                else {
                    ImRect rect(plot.PlotRect.Min.x - 5, plot.PlotRect.Max.y - 5,
                                plot.PlotRect.Max.x + 5, plot.PlotRect.Max.y + 5);
                    if (mouse_pos.y > plot.PlotRect.Min.y + 10)
                        DrawList.AddRectFilled(rect.Min, rect.Max, x_axis.ColorHov);
                    if (rect.Contains(mouse_pos))
                        x_axis.Flags &= ~ImPlotAxisFlags_Opposite;
                }
            }
        }
        for (int i = 0; i < IMPLOT_NUM_Y_AXES; ++i) {
            ImPlotAxis& y_axis = plot.YAxis(i);
            if (ImHasFlag(y_axis.Flags, ImPlotAxisFlags_NoSideSwitch))
                continue;
            if (y_axis.Held && plot.PlotRect.Contains(mouse_pos)) {
                const bool opp = ImHasFlag(y_axis.Flags, ImPlotAxisFlags_Opposite);
                if (!opp) {
                    ImRect rect(plot.PlotRect.Max.x - 5, plot.PlotRect.Min.y - 5,
                                plot.PlotRect.Max.x + 5, plot.PlotRect.Max.y + 5);
                    if (mouse_pos.x > plot.PlotRect.Min.x + 10)
                        DrawList.AddRectFilled(rect.Min, rect.Max, y_axis.ColorHov);
                    if (rect.Contains(mouse_pos))
                        y_axis.Flags |= ImPlotAxisFlags_Opposite;
                }
                else {
                    ImRect rect(plot.PlotRect.Min.x - 5, plot.PlotRect.Min.y - 5,
                                plot.PlotRect.Min.x + 5, plot.PlotRect.Max.y + 5);
                    if (mouse_pos.x < plot.PlotRect.Max.x - 10)
                        DrawList.AddRectFilled(rect.Min, rect.Max, y_axis.ColorHov);
                    if (rect.Contains(mouse_pos))
                        y_axis.Flags &= ~ImPlotAxisFlags_Opposite;
                }
            }
        }
    }

    // reset legend hovers
    plot.Items.Legend.Hovered = false;
    for (int i = 0; i < plot.Items.GetItemCount(); ++i)
        plot.Items.GetItemByIndex(i)->LegendHovered = false;
    // render legend
    if (!ImHasFlag(plot.Flags, ImPlotFlags_NoLegend) && plot.Items.GetLegendCount() > 0) {
        ImPlotLegend& legend = plot.Items.Legend;
        const bool   legend_out  = ImHasFlag(legend.Flags, ImPlotLegendFlags_Outside);
        const bool   legend_horz = ImHasFlag(legend.Flags, ImPlotLegendFlags_Horizontal);
        const ImVec2 legend_size = CalcLegendSize(plot.Items, gp.Style.LegendInnerPadding, gp.Style.LegendSpacing, !legend_horz);
        const ImVec2 legend_pos  = GetLocationPos(legend_out ? plot.FrameRect : plot.PlotRect,
                                                  legend_size,
                                                  legend.Location,
                                                  legend_out ? gp.Style.PlotPadding : gp.Style.LegendPadding);
        legend.Rect = ImRect(legend_pos, legend_pos + legend_size);
        // test hover
        legend.Hovered = ImGui::IsWindowHovered() && legend.Rect.Contains(IO.MousePos);

        if (legend_out)
            ImGui::PushClipRect(plot.FrameRect.Min, plot.FrameRect.Max, true);
        else
            PushPlotClipRect();
        ImU32  col_bg      = GetStyleColorU32(ImPlotCol_LegendBg);
        ImU32  col_bd      = GetStyleColorU32(ImPlotCol_LegendBorder);
        DrawList.AddRectFilled(legend.Rect.Min, legend.Rect.Max, col_bg);
        DrawList.AddRect(legend.Rect.Min, legend.Rect.Max, col_bd);
        bool legend_contextable = ShowLegendEntries(plot.Items, legend.Rect, legend.Hovered, gp.Style.LegendInnerPadding, gp.Style.LegendSpacing, !legend_horz, DrawList)
                                && !ImHasFlag(legend.Flags, ImPlotLegendFlags_NoMenus);

        // main ctx menu
        if (gp.OpenContextThisFrame && legend_contextable && !ImHasFlag(plot.Flags, ImPlotFlags_NoMenus))
            ImGui::OpenPopup("##LegendContext");
        ImGui::PopClipRect();
        if (ImGui::BeginPopup("##LegendContext")) {
            ImGui::Text("Legend"); ImGui::Separator();
            if (ShowLegendContextMenu(legend, !ImHasFlag(plot.Flags, ImPlotFlags_NoLegend)))
                ImFlipFlag(plot.Flags, ImPlotFlags_NoLegend);
            ImGui::EndPopup();
        }
    }
    else {
        plot.Items.Legend.Rect = ImRect();
    }

    // render border
    if (render_border)
        DrawList.AddRect(plot.PlotRect.Min, plot.PlotRect.Max, GetStyleColorU32(ImPlotCol_PlotBorder), 0, ImDrawFlags_RoundCornersAll, gp.Style.PlotBorderSize);

    // render tags
    for (int i = 0; i < gp.Tags.Size; ++i) {
        ImPlotTag& tag  = gp.Tags.Tags[i];
        ImPlotAxis& axis = plot.Axes[tag.Axis];
        if (!axis.Enabled || !axis.Range.Contains(tag.Value))
            continue;
        const char* txt = gp.Tags.GetText(i);
        ImVec2 text_size = ImGui::CalcTextSize(txt);
        ImVec2 size = text_size + gp.Style.AnnotationPadding * 2;
        ImVec2 pos;
        axis.Ticker.OverrideSizeLate(size);
        float pix = IM_ROUND(axis.PlotToPixels(tag.Value));
        if (axis.Vertical) {
            if (axis.IsOpposite()) {
                pos = ImVec2(axis.Datum1 + gp.Style.LabelPadding.x, pix - size.y * 0.5f);
                DrawList.AddTriangleFilled(ImVec2(axis.Datum1,pix), pos, pos + ImVec2(0,size.y), tag.ColorBg);
            }
            else {
                pos = ImVec2(axis.Datum1 - size.x - gp.Style.LabelPadding.x, pix - size.y * 0.5f);
                DrawList.AddTriangleFilled(pos + ImVec2(size.x,0), ImVec2(axis.Datum1,pix), pos+size, tag.ColorBg);
            }
        }
        else {
            if (axis.IsOpposite()) {
                pos = ImVec2(pix - size.x * 0.5f, axis.Datum1 - size.y - gp.Style.LabelPadding.y );
                DrawList.AddTriangleFilled(pos + ImVec2(0,size.y), pos + size, ImVec2(pix,axis.Datum1), tag.ColorBg);
            }
            else {
                pos = ImVec2(pix - size.x * 0.5f, axis.Datum1 + gp.Style.LabelPadding.y);
                DrawList.AddTriangleFilled(pos, ImVec2(pix,axis.Datum1), pos + ImVec2(size.x, 0), tag.ColorBg);
            }
        }
        DrawList.AddRectFilled(pos,pos+size,tag.ColorBg);
        DrawList.AddText(pos+gp.Style.AnnotationPadding,tag.ColorFg,txt);
    }

    // FIT DATA --------------------------------------------------------------
    const bool axis_equal = ImHasFlag(plot.Flags, ImPlotFlags_Equal);
    if (plot.FitThisFrame) {
        for (int i = 0; i < IMPLOT_NUM_X_AXES; i++) {
            ImPlotAxis& x_axis = plot.XAxis(i);
            if (x_axis.FitThisFrame) {
                x_axis.ApplyFit(gp.Style.FitPadding.x);
                if (axis_equal && x_axis.OrthoAxis != NULL) {
                    double aspect = x_axis.GetAspect();
                    ImPlotAxis& y_axis = *x_axis.OrthoAxis;
                    if (y_axis.FitThisFrame) {
                        y_axis.ApplyFit(gp.Style.FitPadding.y);
                        y_axis.FitThisFrame = false;
                        aspect = ImMax(aspect, y_axis.GetAspect());
                    }
                    x_axis.SetAspect(aspect);
                    y_axis.SetAspect(aspect);
                }
            }
        }
        for (int i = 0; i < IMPLOT_NUM_Y_AXES; i++) {
            ImPlotAxis& y_axis = plot.YAxis(i);
            if (y_axis.FitThisFrame) {
                y_axis.ApplyFit(gp.Style.FitPadding.y);
                if (axis_equal && y_axis.OrthoAxis != NULL) {
                    double aspect = y_axis.GetAspect();
                    ImPlotAxis& x_axis = *y_axis.OrthoAxis;
                    if (x_axis.FitThisFrame) {
                        x_axis.ApplyFit(gp.Style.FitPadding.x);
                        x_axis.FitThisFrame = false;
                        aspect = ImMax(x_axis.GetAspect(), aspect);
                    }
                    x_axis.SetAspect(aspect);
                    y_axis.SetAspect(aspect);
                }
            }
        }
        plot.FitThisFrame = false;
    }

    // CONTEXT MENUS -----------------------------------------------------------

    ImGui::PushOverrideID(plot.ID);

    const bool can_ctx = gp.OpenContextThisFrame &&
                         !ImHasFlag(plot.Flags, ImPlotFlags_NoMenus) &&
                         !plot.Items.Legend.Hovered;



    // main ctx menu
    if (can_ctx && plot.Hovered)
        ImGui::OpenPopup("##PlotContext");
    if (ImGui::BeginPopup("##PlotContext")) {
        ShowPlotContextMenu(plot);
        ImGui::EndPopup();
    }

    // axes ctx menus
    for (int i = 0; i < IMPLOT_NUM_X_AXES; ++i) {
        ImGui::PushID(i);
        ImPlotAxis& x_axis = plot.XAxis(i);
        if (can_ctx && x_axis.Hovered && x_axis.HasMenus())
            ImGui::OpenPopup("##XContext");
        if (ImGui::BeginPopup("##XContext")) {
            ImGui::Text(x_axis.HasLabel() ? plot.GetAxisLabel(x_axis) :  i == 0 ? "X-Axis" : "X-Axis %d", i + 1);
            ImGui::Separator();
            ShowAxisContextMenu(x_axis, axis_equal ? x_axis.OrthoAxis : NULL, true);
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    for (int i = 0; i < IMPLOT_NUM_Y_AXES; ++i) {
        ImGui::PushID(i);
        ImPlotAxis& y_axis = plot.YAxis(i);
        if (can_ctx && y_axis.Hovered && y_axis.HasMenus())
            ImGui::OpenPopup("##YContext");
        if (ImGui::BeginPopup("##YContext")) {
            ImGui::Text(y_axis.HasLabel() ? plot.GetAxisLabel(y_axis) : i == 0 ? "Y-Axis" : "Y-Axis %d", i + 1);
            ImGui::Separator();
            ShowAxisContextMenu(y_axis, axis_equal ? y_axis.OrthoAxis : NULL, false);
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    ImGui::PopID();

    // LINKED AXES ------------------------------------------------------------

    for (int i = 0; i < ImAxis_COUNT; ++i)
        plot.Axes[i].PushLinks();


    // CLEANUP ----------------------------------------------------------------

    // remove items
    if (gp.CurrentItems == &plot.Items)
        gp.CurrentItems = NULL;
    // reset the plot items for the next frame
    for (int i = 0; i < plot.Items.GetItemCount(); ++i) {
        plot.Items.GetItemByIndex(i)->SeenThisFrame = false;
    }

    // mark the plot as initialized, i.e. having made it through one frame completely
    plot.Initialized = true;
    // Pop ImGui::PushID at the end of BeginPlot
    ImGui::PopID();
    // Reset context for next plot
    ResetCtxForNextPlot(GImPlot);

    // setup next subplot
    if (gp.CurrentSubplot != NULL) {
        ImGui::PopID();
        SubplotNextCell();
    }
}

//-----------------------------------------------------------------------------
// BEGIN/END SUBPLOT
//-----------------------------------------------------------------------------

static const float SUBPLOT_BORDER_SIZE             = 1.0f;
static const float SUBPLOT_SPLITTER_HALF_THICKNESS = 4.0f;
static const float SUBPLOT_SPLITTER_FEEDBACK_TIMER = 0.06f;

void SubplotSetCell(int row, int col) {
    ImPlotContext& gp      = *GImPlot;
    ImPlotSubplot& subplot = *gp.CurrentSubplot;
    if (row >= subplot.Rows || col >= subplot.Cols)
        return;
    float xoff = 0;
    float yoff = 0;
    for (int c = 0; c < col; ++c)
        xoff += subplot.ColRatios[c];
    for (int r = 0; r < row; ++r)
        yoff += subplot.RowRatios[r];
    const ImVec2 grid_size = subplot.GridRect.GetSize();
    ImVec2 cpos            = subplot.GridRect.Min + ImVec2(xoff*grid_size.x,yoff*grid_size.y);
    cpos.x = IM_ROUND(cpos.x);
    cpos.y = IM_ROUND(cpos.y);
    ImGui::GetCurrentWindow()->DC.CursorPos =  cpos;
    // set cell size
    subplot.CellSize.x = IM_ROUND(subplot.GridRect.GetWidth()  * subplot.ColRatios[col]);
    subplot.CellSize.y = IM_ROUND(subplot.GridRect.GetHeight() * subplot.RowRatios[row]);
    // setup links
    const bool lx = ImHasFlag(subplot.Flags, ImPlotSubplotFlags_LinkAllX);
    const bool ly = ImHasFlag(subplot.Flags, ImPlotSubplotFlags_LinkAllY);
    const bool lr = ImHasFlag(subplot.Flags, ImPlotSubplotFlags_LinkRows);
    const bool lc = ImHasFlag(subplot.Flags, ImPlotSubplotFlags_LinkCols);

    SetNextAxisLinks(ImAxis_X1, lx ? &subplot.ColLinkData[0].Min : lc ? &subplot.ColLinkData[col].Min : NULL,
                                lx ? &subplot.ColLinkData[0].Max : lc ? &subplot.ColLinkData[col].Max : NULL);
    SetNextAxisLinks(ImAxis_Y1, ly ? &subplot.RowLinkData[0].Min : lr ? &subplot.RowLinkData[row].Min : NULL,
                                ly ? &subplot.RowLinkData[0].Max : lr ? &subplot.RowLinkData[row].Max : NULL);
    // setup alignment
    if (!ImHasFlag(subplot.Flags, ImPlotSubplotFlags_NoAlign)) {
        gp.CurrentAlignmentH = &subplot.RowAlignmentData[row];
        gp.CurrentAlignmentV = &subplot.ColAlignmentData[col];
    }
    // set idx
    if (ImHasFlag(subplot.Flags, ImPlotSubplotFlags_ColMajor))
        subplot.CurrentIdx = col * subplot.Rows + row;
    else
        subplot.CurrentIdx = row * subplot.Cols + col;
}

void SubplotSetCell(int idx) {
    ImPlotContext& gp      = *GImPlot;
    ImPlotSubplot& subplot = *gp.CurrentSubplot;
    if (idx >= subplot.Rows * subplot.Cols)
        return;
    int row = 0, col = 0;
    if (ImHasFlag(subplot.Flags, ImPlotSubplotFlags_ColMajor)) {
        row = idx % subplot.Rows;
        col = idx / subplot.Rows;
    }
    else {
        row = idx / subplot.Cols;
        col = idx % subplot.Cols;
    }
    return SubplotSetCell(row, col);
}

void SubplotNextCell() {
    ImPlotContext& gp      = *GImPlot;
    ImPlotSubplot& subplot = *gp.CurrentSubplot;
    SubplotSetCell(++subplot.CurrentIdx);
}

bool BeginSubplots(const char* title, int rows, int cols, const ImVec2& size, ImPlotSubplotFlags flags, float* row_sizes, float* col_sizes) {
    IM_ASSERT_USER_ERROR(rows > 0 && cols > 0, "Invalid sizing arguments!");
    IM_ASSERT_USER_ERROR(GImPlot != NULL, "No current context. Did you call ImPlot::CreateContext() or ImPlot::SetCurrentContext()?");
    IM_ASSERT_USER_ERROR(GImPlot->CurrentSubplot == NULL, "Mismatched BeginSubplots()/EndSubplots()!");
    ImPlotContext& gp = *GImPlot;
    ImGuiContext &G = *GImGui;
    ImGuiWindow * Window = G.CurrentWindow;
    if (Window->SkipItems)
        return false;
    const ImGuiID ID = Window->GetID(title);
    bool just_created = gp.Subplots.GetByKey(ID) == NULL;
    gp.CurrentSubplot = gp.Subplots.GetOrAddByKey(ID);
    ImPlotSubplot& subplot = *gp.CurrentSubplot;
    subplot.ID       = ID;
    subplot.Items.ID = ID - 1;
    subplot.HasTitle = ImGui::FindRenderedTextEnd(title, NULL) != title;
    // push ID
    ImGui::PushID(ID);

    if (just_created)
        subplot.Flags = flags;
    else if (flags != subplot.PreviousFlags)
        subplot.Flags = flags;
    subplot.PreviousFlags = flags;

    // check for change in rows and cols
    if (subplot.Rows != rows || subplot.Cols != cols) {
        subplot.RowAlignmentData.resize(rows);
        subplot.RowLinkData.resize(rows);
        subplot.RowRatios.resize(rows);
        for (int r = 0; r < rows; ++r) {
            subplot.RowAlignmentData[r].Reset();
            subplot.RowLinkData[r] = ImPlotRange(0,1);
            subplot.RowRatios[r] = 1.0f / rows;
        }
        subplot.ColAlignmentData.resize(cols);
        subplot.ColLinkData.resize(cols);
        subplot.ColRatios.resize(cols);
        for (int c = 0; c < cols; ++c) {
            subplot.ColAlignmentData[c].Reset();
            subplot.ColLinkData[c] = ImPlotRange(0,1);
            subplot.ColRatios[c] = 1.0f / cols;
        }
    }
    // check incoming size requests
    float row_sum = 0, col_sum = 0;
    if (row_sizes != NULL) {
        row_sum = ImSum(row_sizes, rows);
        for (int r = 0; r < rows; ++r)
            subplot.RowRatios[r] = row_sizes[r] / row_sum;
    }
    if (col_sizes != NULL) {
        col_sum = ImSum(col_sizes, cols);
        for (int c = 0; c < cols; ++c)
            subplot.ColRatios[c] = col_sizes[c] / col_sum;
    }
    subplot.Rows = rows;
    subplot.Cols = cols;

    // calc plot frame sizes
    ImVec2 title_size(0.0f, 0.0f);
    if (!ImHasFlag(subplot.Flags, ImPlotSubplotFlags_NoTitle))
         title_size = ImGui::CalcTextSize(title, NULL, true);
    const float pad_top = title_size.x > 0.0f ? title_size.y + gp.Style.LabelPadding.y : 0;
    const ImVec2 half_pad = gp.Style.PlotPadding/2;
    const ImVec2 frame_size = ImGui::CalcItemSize(size, gp.Style.PlotDefaultSize.x, gp.Style.PlotDefaultSize.y);
    subplot.FrameRect = ImRect(Window->DC.CursorPos, Window->DC.CursorPos + frame_size);
    subplot.GridRect.Min = subplot.FrameRect.Min + half_pad + ImVec2(0,pad_top);
    subplot.GridRect.Max = subplot.FrameRect.Max - half_pad;
    subplot.FrameHovered = subplot.FrameRect.Contains(ImGui::GetMousePos()) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

    // outside legend adjustments (TODO: make function)
    const bool share_items = ImHasFlag(subplot.Flags, ImPlotSubplotFlags_ShareItems);
    if (share_items)
        gp.CurrentItems = &subplot.Items;
    if (share_items && !ImHasFlag(subplot.Flags, ImPlotSubplotFlags_NoLegend) && subplot.Items.GetLegendCount() > 0) {
        ImPlotLegend& legend = subplot.Items.Legend;
        const bool horz = ImHasFlag(legend.Flags, ImPlotLegendFlags_Horizontal);
        const ImVec2 legend_size = CalcLegendSize(subplot.Items, gp.Style.LegendInnerPadding, gp.Style.LegendSpacing, !horz);
        const bool west = ImHasFlag(legend.Location, ImPlotLocation_West) && !ImHasFlag(legend.Location, ImPlotLocation_East);
        const bool east = ImHasFlag(legend.Location, ImPlotLocation_East) && !ImHasFlag(legend.Location, ImPlotLocation_West);
        const bool north = ImHasFlag(legend.Location, ImPlotLocation_North) && !ImHasFlag(legend.Location, ImPlotLocation_South);
        const bool south = ImHasFlag(legend.Location, ImPlotLocation_South) && !ImHasFlag(legend.Location, ImPlotLocation_North);
        if ((west && !horz) || (west && horz && !north && !south))
            subplot.GridRect.Min.x += (legend_size.x + gp.Style.LegendPadding.x);
        if ((east && !horz) || (east && horz && !north && !south))
            subplot.GridRect.Max.x -= (legend_size.x + gp.Style.LegendPadding.x);
        if ((north && horz) || (north && !horz && !west && !east))
            subplot.GridRect.Min.y += (legend_size.y + gp.Style.LegendPadding.y);
        if ((south && horz) || (south && !horz && !west && !east))
            subplot.GridRect.Max.y -= (legend_size.y + gp.Style.LegendPadding.y);
    }

    // render single background frame
    ImGui::RenderFrame(subplot.FrameRect.Min, subplot.FrameRect.Max, GetStyleColorU32(ImPlotCol_FrameBg), true, ImGui::GetStyle().FrameRounding);
    // render title
    if (title_size.x > 0.0f && !ImHasFlag(subplot.Flags, ImPlotFlags_NoTitle)) {
        const ImU32 col = GetStyleColorU32(ImPlotCol_TitleText);
        AddTextCentered(ImGui::GetWindowDrawList(),ImVec2(subplot.GridRect.GetCenter().x, subplot.GridRect.Min.y - pad_top + half_pad.y),col,title);
    }

    // render splitters
    if (!ImHasFlag(subplot.Flags, ImPlotSubplotFlags_NoResize)) {
        ImDrawList& DrawList = *ImGui::GetWindowDrawList();
        const ImU32 hov_col = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_SeparatorHovered]);
        const ImU32 act_col = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_SeparatorActive]);
        float xpos = subplot.GridRect.Min.x;
        float ypos = subplot.GridRect.Min.y;
        int separator = 1;
        // bool pass = false;
        for (int r = 0; r < subplot.Rows-1; ++r) {
            ypos += subplot.RowRatios[r] * subplot.GridRect.GetHeight();
            const ImGuiID sep_id = subplot.ID + separator;
            ImGui::KeepAliveID(sep_id);
            const ImRect sep_bb = ImRect(subplot.GridRect.Min.x, ypos-SUBPLOT_SPLITTER_HALF_THICKNESS, subplot.GridRect.Max.x, ypos+SUBPLOT_SPLITTER_HALF_THICKNESS);
            bool sep_hov = false, sep_hld = false;
            const bool sep_clk = ImGui::ButtonBehavior(sep_bb, sep_id, &sep_hov, &sep_hld, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_AllowItemOverlap | ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnDoubleClick);
            if ((sep_hov && G.HoveredIdTimer > SUBPLOT_SPLITTER_FEEDBACK_TIMER) || sep_hld) {
                if (sep_clk && ImGui::IsMouseDoubleClicked(0)) {
                    float p = (subplot.RowRatios[r] + subplot.RowRatios[r+1])/2;
                    subplot.RowRatios[r] = subplot.RowRatios[r+1] = p;
                }
                if (sep_clk) {
                    subplot.TempSizes[0] = subplot.RowRatios[r];
                    subplot.TempSizes[1] = subplot.RowRatios[r+1];
                }
                if (sep_hld) {
                    float dp = ImGui::GetMouseDragDelta(0).y  / subplot.GridRect.GetHeight();
                    if (subplot.TempSizes[0] + dp > 0.1f && subplot.TempSizes[1] - dp > 0.1f) {
                        subplot.RowRatios[r]   = subplot.TempSizes[0] + dp;
                        subplot.RowRatios[r+1] = subplot.TempSizes[1] - dp;
                    }
                }
                DrawList.AddLine(ImVec2(IM_ROUND(subplot.GridRect.Min.x),IM_ROUND(ypos)),
                                 ImVec2(IM_ROUND(subplot.GridRect.Max.x),IM_ROUND(ypos)),
                                 sep_hld ? act_col : hov_col, SUBPLOT_BORDER_SIZE);
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }
            separator++;
        }
        for (int c = 0; c < subplot.Cols-1; ++c) {
            xpos += subplot.ColRatios[c] * subplot.GridRect.GetWidth();
            const ImGuiID sep_id = subplot.ID + separator;
            ImGui::KeepAliveID(sep_id);
            const ImRect sep_bb = ImRect(xpos-SUBPLOT_SPLITTER_HALF_THICKNESS, subplot.GridRect.Min.y, xpos+SUBPLOT_SPLITTER_HALF_THICKNESS, subplot.GridRect.Max.y);
            bool sep_hov = false, sep_hld = false;
            const bool sep_clk = ImGui::ButtonBehavior(sep_bb, sep_id, &sep_hov, &sep_hld, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_AllowItemOverlap | ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnDoubleClick);
            if ((sep_hov && G.HoveredIdTimer > SUBPLOT_SPLITTER_FEEDBACK_TIMER) || sep_hld) {
                if (sep_clk && ImGui::IsMouseDoubleClicked(0)) {
                    float p = (subplot.ColRatios[c] + subplot.ColRatios[c+1])/2;
                    subplot.ColRatios[c] = subplot.ColRatios[c+1] = p;
                }
                if (sep_clk) {
                    subplot.TempSizes[0] = subplot.ColRatios[c];
                    subplot.TempSizes[1] = subplot.ColRatios[c+1];
                }
                if (sep_hld) {
                    float dp = ImGui::GetMouseDragDelta(0).x / subplot.GridRect.GetWidth();
                    if (subplot.TempSizes[0] + dp > 0.1f && subplot.TempSizes[1] - dp > 0.1f) {
                        subplot.ColRatios[c]   = subplot.TempSizes[0] + dp;
                        subplot.ColRatios[c+1] = subplot.TempSizes[1] - dp;
                    }
                }
                DrawList.AddLine(ImVec2(IM_ROUND(xpos),IM_ROUND(subplot.GridRect.Min.y)),
                                 ImVec2(IM_ROUND(xpos),IM_ROUND(subplot.GridRect.Max.y)),
                                 sep_hld ? act_col : hov_col, SUBPLOT_BORDER_SIZE);
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
            separator++;
        }
    }

    // set outgoing sizes
    if (row_sizes != NULL) {
        for (int r = 0; r < rows; ++r)
            row_sizes[r] = subplot.RowRatios[r] * row_sum;
    }
    if (col_sizes != NULL) {
        for (int c = 0; c < cols; ++c)
            col_sizes[c] = subplot.ColRatios[c] * col_sum;
    }

    // push styling
    PushStyleColor(ImPlotCol_FrameBg, IM_COL32_BLACK_TRANS);
    PushStyleVar(ImPlotStyleVar_PlotPadding, half_pad);
    PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);

    // set initial cursor pos
    Window->DC.CursorPos = subplot.GridRect.Min;
    // begin alignments
    for (int r = 0; r < subplot.Rows; ++r)
        subplot.RowAlignmentData[r].Begin();
    for (int c = 0; c < subplot.Cols; ++c)
        subplot.ColAlignmentData[c].Begin();
    // clear legend data
    subplot.Items.Legend.Reset();
    // Setup first subplot
    SubplotSetCell(0,0);
    return true;
}

void EndSubplots() {
    IM_ASSERT_USER_ERROR(GImPlot != NULL, "No current context. Did you call ImPlot::CreateContext() or ImPlot::SetCurrentContext()?");
    IM_ASSERT_USER_ERROR(GImPlot->CurrentSubplot != NULL, "Mismatched BeginSubplots()/EndSubplots()!");
    ImPlotContext& gp = *GImPlot;
    ImPlotSubplot& subplot = *GImPlot->CurrentSubplot;
    // set alignments
    for (int r = 0; r < subplot.Rows; ++r)
        subplot.RowAlignmentData[r].End();
    for (int c = 0; c < subplot.Cols; ++c)
        subplot.ColAlignmentData[c].End();
    // pop styling
    PopStyleColor();
    PopStyleVar();
    PopStyleVar();
    ImGui::PopStyleVar();
    // legend
    subplot.Items.Legend.Hovered = false;
    for (int i = 0; i < subplot.Items.GetItemCount(); ++i)
        subplot.Items.GetItemByIndex(i)->LegendHovered = false;
    // render legend
    const bool share_items = ImHasFlag(subplot.Flags, ImPlotSubplotFlags_ShareItems);
    ImDrawList& DrawList = *ImGui::GetWindowDrawList();
    if (share_items && !ImHasFlag(subplot.Flags, ImPlotSubplotFlags_NoLegend) && subplot.Items.GetLegendCount() > 0) {
        const bool   legend_horz = ImHasFlag(subplot.Items.Legend.Flags, ImPlotLegendFlags_Horizontal);
        const ImVec2 legend_size = CalcLegendSize(subplot.Items, gp.Style.LegendInnerPadding, gp.Style.LegendSpacing, !legend_horz);
        const ImVec2 legend_pos  = GetLocationPos(subplot.FrameRect, legend_size, subplot.Items.Legend.Location, gp.Style.PlotPadding);
        subplot.Items.Legend.Rect = ImRect(legend_pos, legend_pos + legend_size);
        subplot.Items.Legend.Hovered = subplot.FrameHovered && subplot.Items.Legend.Rect.Contains(ImGui::GetIO().MousePos);
        ImGui::PushClipRect(subplot.FrameRect.Min, subplot.FrameRect.Max, true);
        ImU32  col_bg      = GetStyleColorU32(ImPlotCol_LegendBg);
        ImU32  col_bd      = GetStyleColorU32(ImPlotCol_LegendBorder);
        DrawList.AddRectFilled(subplot.Items.Legend.Rect.Min, subplot.Items.Legend.Rect.Max, col_bg);
        DrawList.AddRect(subplot.Items.Legend.Rect.Min, subplot.Items.Legend.Rect.Max, col_bd);
        bool legend_contextable = ShowLegendEntries(subplot.Items, subplot.Items.Legend.Rect, subplot.Items.Legend.Hovered, gp.Style.LegendInnerPadding, gp.Style.LegendSpacing, !legend_horz, DrawList)
                                && !ImHasFlag(subplot.Items.Legend.Flags, ImPlotLegendFlags_NoMenus);
        if (legend_contextable && !ImHasFlag(subplot.Flags, ImPlotSubplotFlags_NoMenus) && ImGui::GetIO().MouseReleased[gp.InputMap.Menu])
            ImGui::OpenPopup("##LegendContext");
        ImGui::PopClipRect();
        if (ImGui::BeginPopup("##LegendContext")) {
            ImGui::Text("Legend"); ImGui::Separator();
            if (ShowLegendContextMenu(subplot.Items.Legend, !ImHasFlag(subplot.Flags, ImPlotFlags_NoLegend)))
                ImFlipFlag(subplot.Flags, ImPlotFlags_NoLegend);
            ImGui::EndPopup();
        }
    }
    else {
        subplot.Items.Legend.Rect = ImRect();
    }
    // remove items
    if (gp.CurrentItems == &subplot.Items)
        gp.CurrentItems = NULL;
    // reset the plot items for the next frame (TODO: put this elswhere)
    for (int i = 0; i < subplot.Items.GetItemCount(); ++i) {
        subplot.Items.GetItemByIndex(i)->SeenThisFrame = false;
    }
    // pop id
    ImGui::PopID();
    // set DC back correctly
    GImGui->CurrentWindow->DC.CursorPos = subplot.FrameRect.Min;
    ImGui::Dummy(subplot.FrameRect.GetSize());
    ResetCtxForNextSubplot(GImPlot);

}

//-----------------------------------------------------------------------------
// [SECTION] Plot Utils
//-----------------------------------------------------------------------------

void SetAxis(ImAxis axis) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "SetAxis() needs to be called between BeginPlot() and EndPlot()!");
    IM_ASSERT_USER_ERROR(axis >= ImAxis_X1 && axis < ImAxis_COUNT, "Axis index out of bounds!");
    IM_ASSERT_USER_ERROR(gp.CurrentPlot->Axes[axis].Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    SetupLock();
    if (axis < ImAxis_Y1)
        gp.CurrentPlot->CurrentX = axis;
    else
        gp.CurrentPlot->CurrentY = axis;
}

void SetAxes(ImAxis x_idx, ImAxis y_idx) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "SetAxes() needs to be called between BeginPlot() and EndPlot()!");
    IM_ASSERT_USER_ERROR(x_idx >= ImAxis_X1 && x_idx < ImAxis_Y1, "X-Axis index out of bounds!");
    IM_ASSERT_USER_ERROR(y_idx >= ImAxis_Y1 && y_idx < ImAxis_COUNT, "Y-Axis index out of bounds!");
    IM_ASSERT_USER_ERROR(gp.CurrentPlot->Axes[x_idx].Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    IM_ASSERT_USER_ERROR(gp.CurrentPlot->Axes[y_idx].Enabled, "Axis is not enabled! Did you forget to call SetupAxis()?");
    SetupLock();
    gp.CurrentPlot->CurrentX = x_idx;
    gp.CurrentPlot->CurrentY = y_idx;
}

ImPlotPoint PixelsToPlot(float x, float y, ImAxis x_idx, ImAxis y_idx) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "PixelsToPlot() needs to be called between BeginPlot() and EndPlot()!");
    IM_ASSERT_USER_ERROR(x_idx == IMPLOT_AUTO || (x_idx >= ImAxis_X1 && x_idx < ImAxis_Y1),    "X-Axis index out of bounds!");
    IM_ASSERT_USER_ERROR(y_idx == IMPLOT_AUTO || (y_idx >= ImAxis_Y1 && y_idx < ImAxis_COUNT), "Y-Axis index out of bounds!");
    SetupLock();
    ImPlotPlot& plot   = *gp.CurrentPlot;
    ImPlotAxis& x_axis = x_idx == IMPLOT_AUTO ? plot.Axes[plot.CurrentX] : plot.Axes[x_idx];
    ImPlotAxis& y_axis = y_idx == IMPLOT_AUTO ? plot.Axes[plot.CurrentY] : plot.Axes[y_idx];
    return ImPlotPoint( x_axis.PixelsToPlot(x), y_axis.PixelsToPlot(y) );
}

ImPlotPoint PixelsToPlot(const ImVec2& pix, ImAxis x_idx, ImAxis y_idx) {
    return PixelsToPlot(pix.x, pix.y, x_idx, y_idx);
}

ImVec2 PlotToPixels(double x, double y, ImAxis x_idx, ImAxis y_idx) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "PlotToPixels() needs to be called between BeginPlot() and EndPlot()!");
    IM_ASSERT_USER_ERROR(x_idx == IMPLOT_AUTO || (x_idx >= ImAxis_X1 && x_idx < ImAxis_Y1),    "X-Axis index out of bounds!");
    IM_ASSERT_USER_ERROR(y_idx == IMPLOT_AUTO || (y_idx >= ImAxis_Y1 && y_idx < ImAxis_COUNT), "Y-Axis index out of bounds!");
    SetupLock();
    ImPlotPlot& plot = *gp.CurrentPlot;
    ImPlotAxis& x_axis = x_idx == IMPLOT_AUTO ? plot.Axes[plot.CurrentX] : plot.Axes[x_idx];
    ImPlotAxis& y_axis = y_idx == IMPLOT_AUTO ? plot.Axes[plot.CurrentY] : plot.Axes[y_idx];
    return ImVec2( x_axis.PlotToPixels(x), y_axis.PlotToPixels(y) );
}

ImVec2 PlotToPixels(const ImPlotPoint& plt, ImAxis x_idx, ImAxis y_idx) {
    return PlotToPixels(plt.x, plt.y, x_idx, y_idx);
}

ImVec2 GetPlotPos() {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "GetPlotPos() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    return gp.CurrentPlot->PlotRect.Min;
}

ImVec2 GetPlotSize() {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "GetPlotSize() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    return gp.CurrentPlot->PlotRect.GetSize();
}

ImPlotPoint GetPlotMousePos(ImAxis x_idx, ImAxis y_idx) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "GetPlotMousePos() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    return PixelsToPlot(ImGui::GetMousePos(), x_idx, y_idx);
}

ImPlotRect GetPlotLimits(ImAxis x_idx, ImAxis y_idx) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "GetPlotLimits() needs to be called between BeginPlot() and EndPlot()!");
    IM_ASSERT_USER_ERROR(x_idx == IMPLOT_AUTO || (x_idx >= ImAxis_X1 && x_idx < ImAxis_Y1),    "X-Axis index out of bounds!");
    IM_ASSERT_USER_ERROR(y_idx == IMPLOT_AUTO || (y_idx >= ImAxis_Y1 && y_idx < ImAxis_COUNT), "Y-Axis index out of bounds!");
    SetupLock();
    ImPlotPlot& plot = *gp.CurrentPlot;
    ImPlotAxis& x_axis = x_idx == IMPLOT_AUTO ? plot.Axes[plot.CurrentX] : plot.Axes[x_idx];
    ImPlotAxis& y_axis = y_idx == IMPLOT_AUTO ? plot.Axes[plot.CurrentY] : plot.Axes[y_idx];
    ImPlotRect limits;
    limits.X = x_axis.Range;
    limits.Y = y_axis.Range;
    return limits;
}

bool IsPlotHovered() {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "IsPlotHovered() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    return gp.CurrentPlot->Hovered;
}

bool IsAxisHovered(ImAxis axis) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "IsPlotXAxisHovered() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    return gp.CurrentPlot->Axes[axis].Hovered;
}

bool IsSubplotsHovered() {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentSubplot != NULL, "IsSubplotsHovered() needs to be called between BeginSubplots() and EndSubplots()!");
    return gp.CurrentSubplot->FrameHovered;
}

bool IsPlotSelected() {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "IsPlotSelected() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    return gp.CurrentPlot->Selected;
}

ImPlotRect GetPlotSelection(ImAxis x_idx, ImAxis y_idx) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "GetPlotSelection() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    ImPlotPlot& plot = *gp.CurrentPlot;
    if (!plot.Selected)
        return ImPlotRect(0,0,0,0);
    ImPlotPoint p1 = PixelsToPlot(plot.SelectRect.Min + plot.PlotRect.Min, x_idx, y_idx);
    ImPlotPoint p2 = PixelsToPlot(plot.SelectRect.Max + plot.PlotRect.Min, x_idx, y_idx);
    ImPlotRect result;
    result.X.Min = ImMin(p1.x, p2.x);
    result.X.Max = ImMax(p1.x, p2.x);
    result.Y.Min = ImMin(p1.y, p2.y);
    result.Y.Max = ImMax(p1.y, p2.y);
    return result;
}

void CancelPlotSelection() {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "CancelPlotSelection() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    ImPlotPlot& plot = *gp.CurrentPlot;
    if (plot.Selected)
        plot.Selected = plot.Selecting = false;
}

void HideNextItem(bool hidden, ImPlotCond cond) {
    ImPlotContext& gp = *GImPlot;
    gp.NextItemData.HasHidden  = true;
    gp.NextItemData.Hidden     = hidden;
    gp.NextItemData.HiddenCond = cond;
}

//-----------------------------------------------------------------------------
// [SECTION] Plot Tools
//-----------------------------------------------------------------------------

void Annotation(double x, double y, const ImVec4& col, const ImVec2& offset, bool clamp, bool round) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "Annotation() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    char x_buff[IMPLOT_LABEL_MAX_SIZE];
    char y_buff[IMPLOT_LABEL_MAX_SIZE];
    ImPlotAxis& x_axis = gp.CurrentPlot->Axes[gp.CurrentPlot->CurrentX];
    ImPlotAxis& y_axis = gp.CurrentPlot->Axes[gp.CurrentPlot->CurrentY];
    LabelAxisValue(x_axis, x, x_buff, sizeof(x_buff), round);
    LabelAxisValue(y_axis, y, y_buff, sizeof(y_buff), round);
    Annotation(x,y,col,offset,clamp,"%s, %s",x_buff,y_buff);
}

void AnnotationV(double x, double y, const ImVec4& col, const ImVec2& offset, bool clamp, const char* fmt, va_list args) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "Annotation() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    ImVec2 pos = PlotToPixels(x,y,IMPLOT_AUTO,IMPLOT_AUTO);
    ImU32  bg  = ImGui::GetColorU32(col);
    ImU32  fg  = col.w == 0 ? GetStyleColorU32(ImPlotCol_InlayText) : CalcTextColor(col);
    gp.Annotations.AppendV(pos, offset, bg, fg, clamp, fmt, args);
}

void Annotation(double x, double y, const ImVec4& col, const ImVec2& offset, bool clamp, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    AnnotationV(x,y,col,offset,clamp,fmt,args);
    va_end(args);
}

void TagV(ImAxis axis, double v, const ImVec4& col, const char* fmt, va_list args) {
    ImPlotContext& gp = *GImPlot;
    SetupLock();
    ImU32 bg = ImGui::GetColorU32(col);
    ImU32 fg = col.w == 0 ? GetStyleColorU32(ImPlotCol_AxisText) : CalcTextColor(col);
    gp.Tags.AppendV(axis,v,bg,fg,fmt,args);
}

void Tag(ImAxis axis, double v, const ImVec4& col, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TagV(axis,v,col,fmt,args);
    va_end(args);
}

void Tag(ImAxis axis, double v, const ImVec4& color, bool round) {
    ImPlotContext& gp = *GImPlot;
    SetupLock();
    char buff[IMPLOT_LABEL_MAX_SIZE];
    ImPlotAxis& ax = gp.CurrentPlot->Axes[axis];
    LabelAxisValue(ax, v, buff, sizeof(buff), round);
    Tag(axis,v,color,"%s",buff);
}

IMPLOT_API void TagX(double x, const ImVec4& color, bool round) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "TagX() needs to be called between BeginPlot() and EndPlot()!");
    Tag(GImPlot->CurrentPlot->CurrentX, x, color, round);
}

IMPLOT_API void TagX(double x, const ImVec4& color, const char* fmt, ...) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "TagX() needs to be called between BeginPlot() and EndPlot()!");
    va_list args;
    va_start(args, fmt);
    TagV(GImPlot->CurrentPlot->CurrentX,x,color,fmt,args);
    va_end(args);
}

IMPLOT_API void TagXV(double x, const ImVec4& color, const char* fmt, va_list args) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "TagX() needs to be called between BeginPlot() and EndPlot()!");
    TagV(GImPlot->CurrentPlot->CurrentX, x, color, fmt, args);
}

IMPLOT_API void TagY(double y, const ImVec4& color, bool round) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "TagY() needs to be called between BeginPlot() and EndPlot()!");
    Tag(GImPlot->CurrentPlot->CurrentY, y, color, round);
}

IMPLOT_API void TagY(double y, const ImVec4& color, const char* fmt, ...) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "TagY() needs to be called between BeginPlot() and EndPlot()!");
    va_list args;
    va_start(args, fmt);
    TagV(GImPlot->CurrentPlot->CurrentY,y,color,fmt,args);
    va_end(args);
}

IMPLOT_API void TagYV(double y, const ImVec4& color, const char* fmt, va_list args) {
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "TagY() needs to be called between BeginPlot() and EndPlot()!");
    TagV(GImPlot->CurrentPlot->CurrentY, y, color, fmt, args);
}

static const float DRAG_GRAB_HALF_SIZE = 4.0f;

bool DragPoint(int n_id, double* x, double* y, const ImVec4& col, float radius, ImPlotDragToolFlags flags) {
    ImGui::PushID("#IMPLOT_DRAG_POINT");
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "DragPoint() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();

    if (!ImHasFlag(flags,ImPlotDragToolFlags_NoFit) && FitThisFrame()) {
        FitPoint(ImPlotPoint(*x,*y));
    }

    const bool input = !ImHasFlag(flags, ImPlotDragToolFlags_NoInputs);
    const bool show_curs = !ImHasFlag(flags, ImPlotDragToolFlags_NoCursors);
    const bool no_delay = !ImHasFlag(flags, ImPlotDragToolFlags_Delayed);
    const float grab_half_size = ImMax(DRAG_GRAB_HALF_SIZE, radius);
    const ImVec4 color = IsColorAuto(col) ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : col;
    const ImU32 col32 = ImGui::ColorConvertFloat4ToU32(color);

    ImVec2 pos = PlotToPixels(*x,*y,IMPLOT_AUTO,IMPLOT_AUTO);
    const ImGuiID id = ImGui::GetCurrentWindow()->GetID(n_id);
    ImRect rect(pos.x-grab_half_size,pos.y-grab_half_size,pos.x+grab_half_size,pos.y+grab_half_size);
    bool hovered = false, held = false;

    ImGui::KeepAliveID(id);
    if (input)
        ImGui::ButtonBehavior(rect,id,&hovered,&held);

    bool dragging = false;
    if (held && ImGui::IsMouseDragging(0)) {
        *x = ImPlot::GetPlotMousePos(IMPLOT_AUTO,IMPLOT_AUTO).x;
        *y = ImPlot::GetPlotMousePos(IMPLOT_AUTO,IMPLOT_AUTO).y;
        dragging = true;
    }

    PushPlotClipRect();
    ImDrawList& DrawList = *GetPlotDrawList();
    if ((hovered || held) && show_curs)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    if (dragging && no_delay)
        pos = PlotToPixels(*x,*y,IMPLOT_AUTO,IMPLOT_AUTO);
    DrawList.AddCircleFilled(pos, radius, col32);
    PopPlotClipRect();

    ImGui::PopID();
    return dragging;
}

bool DragLineX(int n_id, double* value, const ImVec4& col, float thickness, ImPlotDragToolFlags flags) {
    // ImGui::PushID("#IMPLOT_DRAG_LINE_X");
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "DragLineX() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();

    if (!ImHasFlag(flags,ImPlotDragToolFlags_NoFit) && FitThisFrame()) {
        FitPointX(*value);
    }

    const bool input = !ImHasFlag(flags, ImPlotDragToolFlags_NoInputs);
    const bool show_curs = !ImHasFlag(flags, ImPlotDragToolFlags_NoCursors);
    const bool no_delay = !ImHasFlag(flags, ImPlotDragToolFlags_Delayed);
    const float grab_half_size = ImMax(DRAG_GRAB_HALF_SIZE, thickness/2);
    float yt = gp.CurrentPlot->PlotRect.Min.y;
    float yb = gp.CurrentPlot->PlotRect.Max.y;
    float x  = IM_ROUND(PlotToPixels(*value,0,IMPLOT_AUTO,IMPLOT_AUTO).x);
    const ImGuiID id = ImGui::GetCurrentWindow()->GetID(n_id);
    ImRect rect(x-grab_half_size,yt,x+grab_half_size,yb);
    bool hovered = false, held = false;

    ImGui::KeepAliveID(id);
    if (input)
        ImGui::ButtonBehavior(rect,id,&hovered,&held);

    if ((hovered || held) && show_curs)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    float len = gp.Style.MajorTickLen.x;
    ImVec4 color = IsColorAuto(col) ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : col;
    ImU32 col32 = ImGui::ColorConvertFloat4ToU32(color);

    bool dragging = false;
    if (held && ImGui::IsMouseDragging(0)) {
        *value = ImPlot::GetPlotMousePos(IMPLOT_AUTO,IMPLOT_AUTO).x;
        dragging = true;
    }

    PushPlotClipRect();
    ImDrawList& DrawList = *GetPlotDrawList();
    if (dragging && no_delay)
        x  = IM_ROUND(PlotToPixels(*value,0,IMPLOT_AUTO,IMPLOT_AUTO).x);
    DrawList.AddLine(ImVec2(x,yt), ImVec2(x,yb),     col32,   thickness);
    DrawList.AddLine(ImVec2(x,yt), ImVec2(x,yt+len), col32, 3*thickness);
    DrawList.AddLine(ImVec2(x,yb), ImVec2(x,yb-len), col32, 3*thickness);
    PopPlotClipRect();

    // ImGui::PopID();
    return dragging;
}

bool DragLineY(int n_id, double* value, const ImVec4& col, float thickness, ImPlotDragToolFlags flags) {
    ImGui::PushID("#IMPLOT_DRAG_LINE_Y");
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "DragLineY() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();

    if (!ImHasFlag(flags,ImPlotDragToolFlags_NoFit) && FitThisFrame()) {
        FitPointY(*value);
    }

    const bool input = !ImHasFlag(flags, ImPlotDragToolFlags_NoInputs);
    const bool show_curs = !ImHasFlag(flags, ImPlotDragToolFlags_NoCursors);
    const bool no_delay = !ImHasFlag(flags, ImPlotDragToolFlags_Delayed);
    const float grab_half_size = ImMax(DRAG_GRAB_HALF_SIZE, thickness/2);
    float xl = gp.CurrentPlot->PlotRect.Min.x;
    float xr = gp.CurrentPlot->PlotRect.Max.x;
    float y  = IM_ROUND(PlotToPixels(0, *value,IMPLOT_AUTO,IMPLOT_AUTO).y);

    const ImGuiID id = ImGui::GetCurrentWindow()->GetID(n_id);
    ImRect rect(xl,y-grab_half_size,xr,y+grab_half_size);
    bool hovered = false, held = false;

    ImGui::KeepAliveID(id);
    if (input)
        ImGui::ButtonBehavior(rect,id,&hovered,&held);

    if ((hovered || held) && show_curs)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

    float len = gp.Style.MajorTickLen.y;
    ImVec4 color = IsColorAuto(col) ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : col;
    ImU32 col32 = ImGui::ColorConvertFloat4ToU32(color);

    bool dragging = false;
    if (held && ImGui::IsMouseDragging(0)) {
        *value = ImPlot::GetPlotMousePos(IMPLOT_AUTO,IMPLOT_AUTO).y;
        dragging = true;
    }

    PushPlotClipRect();
    ImDrawList& DrawList = *GetPlotDrawList();
    if (dragging && no_delay)
        y  = IM_ROUND(PlotToPixels(0, *value,IMPLOT_AUTO,IMPLOT_AUTO).y);
    DrawList.AddLine(ImVec2(xl,y), ImVec2(xr,y),     col32,   thickness);
    DrawList.AddLine(ImVec2(xl,y), ImVec2(xl+len,y), col32, 3*thickness);
    DrawList.AddLine(ImVec2(xr,y), ImVec2(xr-len,y), col32, 3*thickness);
    PopPlotClipRect();

    ImGui::PopID();
    return dragging;
}

bool DragRect(int n_id, double* x_min, double* y_min, double* x_max, double* y_max, const ImVec4& col, ImPlotDragToolFlags flags) {
    ImGui::PushID("#IMPLOT_DRAG_RECT");
    IM_ASSERT_USER_ERROR(GImPlot->CurrentPlot != NULL, "DragRect() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();

    if (!ImHasFlag(flags,ImPlotDragToolFlags_NoFit) && FitThisFrame()) {
        FitPoint(ImPlotPoint(*x_min,*y_min));
        FitPoint(ImPlotPoint(*x_max,*y_max));
    }

    const bool input = !ImHasFlag(flags, ImPlotDragToolFlags_NoInputs);
    const bool show_curs = !ImHasFlag(flags, ImPlotDragToolFlags_NoCursors);
    const bool no_delay = !ImHasFlag(flags, ImPlotDragToolFlags_Delayed);
    bool    h[] = {true,false,true,false};
    double* x[] = {x_min,x_max,x_max,x_min};
    double* y[] = {y_min,y_min,y_max,y_max};
    ImVec2 p[4];
    for (int i = 0; i < 4; ++i)
        p[i] = PlotToPixels(*x[i],*y[i],IMPLOT_AUTO,IMPLOT_AUTO);
    ImVec2 pc = PlotToPixels((*x_min+*x_max)/2,(*y_min+*y_max)/2,IMPLOT_AUTO,IMPLOT_AUTO);
    ImRect rect(ImMin(p[0],p[2]),ImMax(p[0],p[2]));
    ImRect rect_grab = rect; rect_grab.Expand(DRAG_GRAB_HALF_SIZE);

    ImGuiMouseCursor cur[4];
    if (show_curs) {
        cur[0] = (rect.Min.x == p[0].x && rect.Min.y == p[0].y) || (rect.Max.x == p[0].x && rect.Max.y == p[0].y) ? ImGuiMouseCursor_ResizeNWSE : ImGuiMouseCursor_ResizeNESW;
        cur[1] = cur[0] == ImGuiMouseCursor_ResizeNWSE ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE;
        cur[2] = cur[1] == ImGuiMouseCursor_ResizeNWSE ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE;
        cur[3] = cur[2] == ImGuiMouseCursor_ResizeNWSE ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE;
    }

    ImVec4 color = IsColorAuto(col) ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : col;
    ImU32 col32 = ImGui::ColorConvertFloat4ToU32(color);
    color.w *= 0.25f;
    ImU32 col32_a = ImGui::ColorConvertFloat4ToU32(color);
    const ImGuiID id = ImGui::GetCurrentWindow()->GetID(n_id);

    bool dragging = false;
    bool hovered = false, held = false;
    ImRect b_rect(pc.x-DRAG_GRAB_HALF_SIZE,pc.y-DRAG_GRAB_HALF_SIZE,pc.x+DRAG_GRAB_HALF_SIZE,pc.y+DRAG_GRAB_HALF_SIZE);

    ImGui::KeepAliveID(id);
    if (input)
        ImGui::ButtonBehavior(b_rect,id,&hovered,&held);

    if ((hovered || held) && show_curs)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    if (held && ImGui::IsMouseDragging(0)) {
        for (int i = 0; i < 4; ++i) {
            ImPlotPoint pp = PixelsToPlot(p[i] + ImGui::GetIO().MouseDelta,IMPLOT_AUTO,IMPLOT_AUTO);
            *y[i] = pp.y;
            *x[i] = pp.x;
        }
        dragging = true;
    }

    for (int i = 0; i < 4; ++i) {
        // points
        b_rect = ImRect(p[i].x-DRAG_GRAB_HALF_SIZE,p[i].y-DRAG_GRAB_HALF_SIZE,p[i].x+DRAG_GRAB_HALF_SIZE,p[i].y+DRAG_GRAB_HALF_SIZE);
        ImGuiID p_id = id + i + 1;
        ImGui::KeepAliveID(p_id);
        if (input)
            ImGui::ButtonBehavior(b_rect,p_id,&hovered,&held);
        if ((hovered || held) && show_curs)
            ImGui::SetMouseCursor(cur[i]);

        if (held && ImGui::IsMouseDragging(0)) {
            *x[i] = ImPlot::GetPlotMousePos(IMPLOT_AUTO,IMPLOT_AUTO).x;
            *y[i] = ImPlot::GetPlotMousePos(IMPLOT_AUTO,IMPLOT_AUTO).y;
            dragging = true;
        }

        // edges
        ImVec2 e_min = ImMin(p[i],p[(i+1)%4]);
        ImVec2 e_max = ImMax(p[i],p[(i+1)%4]);
        b_rect = h[i] ? ImRect(e_min.x + DRAG_GRAB_HALF_SIZE, e_min.y - DRAG_GRAB_HALF_SIZE, e_max.x - DRAG_GRAB_HALF_SIZE, e_max.y + DRAG_GRAB_HALF_SIZE)
                    : ImRect(e_min.x - DRAG_GRAB_HALF_SIZE, e_min.y + DRAG_GRAB_HALF_SIZE, e_max.x + DRAG_GRAB_HALF_SIZE, e_max.y - DRAG_GRAB_HALF_SIZE);
        ImGuiID e_id = id + i + 5;
        ImGui::KeepAliveID(e_id);
        if (input)
            ImGui::ButtonBehavior(b_rect,e_id,&hovered,&held);
        if ((hovered || held) && show_curs)
            h[i] ? ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS) : ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (held && ImGui::IsMouseDragging(0)) {
            if (h[i])
                *y[i] = ImPlot::GetPlotMousePos(IMPLOT_AUTO,IMPLOT_AUTO).y;
            else
                *x[i] = ImPlot::GetPlotMousePos(IMPLOT_AUTO,IMPLOT_AUTO).x;
            dragging = true;
        }
        if (hovered && ImGui::IsMouseDoubleClicked(0))
        {
            ImPlotRect b = GetPlotLimits(IMPLOT_AUTO,IMPLOT_AUTO);
            if (h[i])
                *y[i] = ((y[i] == y_min && *y_min < *y_max) || (y[i] == y_max && *y_max < *y_min)) ? b.Y.Min : b.Y.Max;
            else
                *x[i] = ((x[i] == x_min && *x_min < *x_max) || (x[i] == x_max && *x_max < *x_min)) ? b.X.Min : b.X.Max;
            dragging = true;
        }
    }


    PushPlotClipRect();
    ImDrawList& DrawList = *GetPlotDrawList();
    if (dragging && no_delay) {
        for (int i = 0; i < 4; ++i)
            p[i] = PlotToPixels(*x[i],*y[i],IMPLOT_AUTO,IMPLOT_AUTO);
        pc = PlotToPixels((*x_min+*x_max)/2,(*y_min+*y_max)/2,IMPLOT_AUTO,IMPLOT_AUTO);
        rect = ImRect(ImMin(p[0],p[2]),ImMax(p[0],p[2]));
    }
    DrawList.AddRectFilled(rect.Min, rect.Max, col32_a);
    DrawList.AddRect(rect.Min, rect.Max, col32);
    if (input && (dragging || rect_grab.Contains(ImGui::GetMousePos()))) {
        DrawList.AddCircleFilled(pc,DRAG_GRAB_HALF_SIZE,col32);
        for (int i = 0; i < 4; ++i)
            DrawList.AddCircleFilled(p[i],DRAG_GRAB_HALF_SIZE,col32);
    }
    PopPlotClipRect();
    ImGui::PopID();
    return dragging;
}

bool DragRect(int id, ImPlotRect* bounds, const ImVec4& col, ImPlotDragToolFlags flags) {
    return DragRect(id, &bounds->X.Min, &bounds->Y.Min,&bounds->X.Max, &bounds->Y.Max, col, flags);
}

//-----------------------------------------------------------------------------
// [SECTION] Legend Utils and Tools
//-----------------------------------------------------------------------------

bool IsLegendEntryHovered(const char* label_id) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentItems != NULL, "IsPlotItemHighlight() needs to be called within an itemized context!");
    SetupLock();
    ImGuiID id = ImGui::GetIDWithSeed(label_id, NULL, gp.CurrentItems->ID);
    ImPlotItem* item = gp.CurrentItems->GetItem(id);
    return item && item->LegendHovered;
}

bool BeginLegendPopup(const char* label_id, ImGuiMouseButton mouse_button) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentItems != NULL, "BeginLegendPopup() needs to be called within an itemized context!");
    SetupLock();
    ImGuiWindow* window = GImGui->CurrentWindow;
    if (window->SkipItems)
        return false;
    ImGuiID id = ImGui::GetIDWithSeed(label_id, NULL, gp.CurrentItems->ID);
    if (ImGui::IsMouseReleased(mouse_button)) {
        ImPlotItem* item = gp.CurrentItems->GetItem(id);
        if (item && item->LegendHovered)
            ImGui::OpenPopupEx(id);
    }
    return ImGui::BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
}

void EndLegendPopup() {
    SetupLock();
    ImGui::EndPopup();
}

void ShowAltLegend(const char* title_id, bool vertical, const ImVec2 size, bool interactable) {
    ImPlotContext& gp    = *GImPlot;
    ImGuiContext &G      = *GImGui;
    ImGuiWindow * Window = G.CurrentWindow;
    if (Window->SkipItems)
        return;
    ImDrawList &DrawList = *Window->DrawList;
    ImPlotPlot* plot = GetPlot(title_id);
    ImVec2 legend_size;
    ImVec2 default_size = gp.Style.LegendPadding * 2;
    if (plot != NULL) {
        legend_size  = CalcLegendSize(plot->Items, gp.Style.LegendInnerPadding, gp.Style.LegendSpacing, vertical);
        default_size = legend_size + gp.Style.LegendPadding * 2;
    }
    ImVec2 frame_size = ImGui::CalcItemSize(size, default_size.x, default_size.y);
    ImRect bb_frame = ImRect(Window->DC.CursorPos, Window->DC.CursorPos + frame_size);
    ImGui::ItemSize(bb_frame);
    if (!ImGui::ItemAdd(bb_frame, 0, &bb_frame))
        return;
    ImGui::RenderFrame(bb_frame.Min, bb_frame.Max, GetStyleColorU32(ImPlotCol_FrameBg), true, G.Style.FrameRounding);
    DrawList.PushClipRect(bb_frame.Min, bb_frame.Max, true);
    if (plot != NULL) {
        const ImVec2 legend_pos  = GetLocationPos(bb_frame, legend_size, 0, gp.Style.LegendPadding);
        const ImRect legend_bb(legend_pos, legend_pos + legend_size);
        interactable = interactable && bb_frame.Contains(ImGui::GetIO().MousePos);
        // render legend box
        ImU32  col_bg      = GetStyleColorU32(ImPlotCol_LegendBg);
        ImU32  col_bd      = GetStyleColorU32(ImPlotCol_LegendBorder);
        DrawList.AddRectFilled(legend_bb.Min, legend_bb.Max, col_bg);
        DrawList.AddRect(legend_bb.Min, legend_bb.Max, col_bd);
        // render entries
        ShowLegendEntries(plot->Items, legend_bb, interactable, gp.Style.LegendInnerPadding, gp.Style.LegendSpacing, vertical, DrawList);
    }
    DrawList.PopClipRect();
}

//-----------------------------------------------------------------------------
// [SECTION] Drag and Drop Utils
//-----------------------------------------------------------------------------

bool BeginDragDropTargetPlot() {
    SetupLock();
    ImRect rect = GImPlot->CurrentPlot->PlotRect;
    return ImGui::BeginDragDropTargetCustom(rect, GImPlot->CurrentPlot->ID);
}

bool BeginDragDropTargetAxis(ImAxis axis) {
    SetupLock();
    ImPlotPlot& plot = *GImPlot->CurrentPlot;
    ImPlotAxis& ax = plot.Axes[axis];
    ImRect rect = ax.HoverRect;
    rect.Expand(-3.5f);
    return ImGui::BeginDragDropTargetCustom(rect, ax.ID);
}

bool BeginDragDropTargetLegend() {
    SetupLock();
    ImPlotItemGroup& items = *GImPlot->CurrentItems;
    ImRect rect = items.Legend.Rect;
    return ImGui::BeginDragDropTargetCustom(rect, items.ID);
}

void EndDragDropTarget() {
    SetupLock();
	ImGui::EndDragDropTarget();
}

bool BeginDragDropSourcePlot(ImGuiDragDropFlags flags) {
    SetupLock();
    ImPlotPlot* plot = GImPlot->CurrentPlot;
    if (GImGui->IO.KeyMods == GImPlot->InputMap.OverrideMod || GImGui->DragDropPayload.SourceId == plot->ID)
        return ImGui::ItemAdd(plot->PlotRect, plot->ID) && ImGui::BeginDragDropSource(flags);
    return false;
}

bool BeginDragDropSourceAxis(ImAxis idx, ImGuiDragDropFlags flags) {
    SetupLock();
    ImPlotAxis& axis = GImPlot->CurrentPlot->Axes[idx];
    if (GImGui->IO.KeyMods == GImPlot->InputMap.OverrideMod || GImGui->DragDropPayload.SourceId == axis.ID)
        return ImGui::ItemAdd(axis.HoverRect, axis.ID) && ImGui::BeginDragDropSource(flags);
    return false;
}

bool BeginDragDropSourceItem(const char* label_id, ImGuiDragDropFlags flags) {
    SetupLock();
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentItems != NULL, "BeginDragDropSourceItem() needs to be called within an itemized context!");
    ImGuiID item_id = ImGui::GetIDWithSeed(label_id, NULL, gp.CurrentItems->ID);
    ImPlotItem* item = gp.CurrentItems->GetItem(item_id);
    if (item != NULL) {
        return ImGui::ItemAdd(item->LegendHoverRect, item->ID) && ImGui::BeginDragDropSource(flags);
    }
    return false;
}

void EndDragDropSource() {
    SetupLock();
    ImGui::EndDragDropSource();
}

//-----------------------------------------------------------------------------
// [SECTION] Aligned Plots
//-----------------------------------------------------------------------------

bool BeginAlignedPlots(const char* group_id, bool vertical) {
    IM_ASSERT_USER_ERROR(GImPlot != NULL, "No current context. Did you call ImPlot::CreateContext() or ImPlot::SetCurrentContext()?");
    IM_ASSERT_USER_ERROR(GImPlot->CurrentAlignmentH == NULL && GImPlot->CurrentAlignmentV == NULL, "Mismatched BeginAlignedPlots()/EndAlignedPlots()!");
    ImPlotContext& gp = *GImPlot;
    ImGuiContext &G = *GImGui;
    ImGuiWindow * Window = G.CurrentWindow;
    if (Window->SkipItems)
        return false;
    const ImGuiID ID = Window->GetID(group_id);
    ImPlotAlignmentData* alignment = gp.AlignmentData.GetOrAddByKey(ID);
    if (vertical)
        gp.CurrentAlignmentV = alignment;
    else
        gp.CurrentAlignmentH = alignment;
    if (alignment->Vertical != vertical)
        alignment->Reset();
    alignment->Vertical = vertical;
    alignment->Begin();
    return true;
}

void EndAlignedPlots() {
    IM_ASSERT_USER_ERROR(GImPlot != NULL, "No current context. Did you call ImPlot::CreateContext() or ImPlot::SetCurrentContext()?");
    IM_ASSERT_USER_ERROR(GImPlot->CurrentAlignmentH != NULL || GImPlot->CurrentAlignmentV != NULL, "Mismatched BeginAlignedPlots()/EndAlignedPlots()!");
    ImPlotContext& gp = *GImPlot;
    ImPlotAlignmentData* alignment = gp.CurrentAlignmentH != NULL ? gp.CurrentAlignmentH : (gp.CurrentAlignmentV != NULL ? gp.CurrentAlignmentV : NULL);
    if (alignment)
        alignment->End();
    ResetCtxForNextAlignedPlots(GImPlot);
}

//-----------------------------------------------------------------------------
// [SECTION] Plot and Item Styling
//-----------------------------------------------------------------------------

ImPlotStyle& GetStyle() {
    IM_ASSERT_USER_ERROR(GImPlot != NULL, "No current context. Did you call ImPlot::CreateContext() or ImPlot::SetCurrentContext()?");
    ImPlotContext& gp = *GImPlot;
    return gp.Style;
}

void PushStyleColor(ImPlotCol idx, ImU32 col) {
    ImPlotContext& gp = *GImPlot;
    ImGuiColorMod backup;
    backup.Col = (ImGuiCol)idx;
    backup.BackupValue = gp.Style.Colors[idx];
    gp.ColorModifiers.push_back(backup);
    gp.Style.Colors[idx] = ImGui::ColorConvertU32ToFloat4(col);
}

void PushStyleColor(ImPlotCol idx, const ImVec4& col) {
    ImPlotContext& gp = *GImPlot;
    ImGuiColorMod backup;
    backup.Col = (ImGuiCol)idx;
    backup.BackupValue = gp.Style.Colors[idx];
    gp.ColorModifiers.push_back(backup);
    gp.Style.Colors[idx] = col;
}

void PopStyleColor(int count) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(count <= gp.ColorModifiers.Size, "You can't pop more modifiers than have been pushed!");
    while (count > 0)
    {
        ImGuiColorMod& backup = gp.ColorModifiers.back();
        gp.Style.Colors[backup.Col] = backup.BackupValue;
        gp.ColorModifiers.pop_back();
        count--;
    }
}

void PushStyleVar(ImPlotStyleVar idx, float val) {
    ImPlotContext& gp = *GImPlot;
    const ImPlotStyleVarInfo* var_info = GetPlotStyleVarInfo(idx);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1) {
        float* pvar = (float*)var_info->GetVarPtr(&gp.Style);
        gp.StyleModifiers.push_back(ImGuiStyleMod((ImGuiStyleVar)idx, *pvar));
        *pvar = val;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() float variant but variable is not a float!");
}

void PushStyleVar(ImPlotStyleVar idx, int val) {
    ImPlotContext& gp = *GImPlot;
    const ImPlotStyleVarInfo* var_info = GetPlotStyleVarInfo(idx);
    if (var_info->Type == ImGuiDataType_S32 && var_info->Count == 1) {
        int* pvar = (int*)var_info->GetVarPtr(&gp.Style);
        gp.StyleModifiers.push_back(ImGuiStyleMod((ImGuiStyleVar)idx, *pvar));
        *pvar = val;
        return;
    }
    else if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1) {
        float* pvar = (float*)var_info->GetVarPtr(&gp.Style);
        gp.StyleModifiers.push_back(ImGuiStyleMod((ImGuiStyleVar)idx, *pvar));
        *pvar = (float)val;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() int variant but variable is not a int!");
}

void PushStyleVar(ImPlotStyleVar idx, const ImVec2& val)
{
    ImPlotContext& gp = *GImPlot;
    const ImPlotStyleVarInfo* var_info = GetPlotStyleVarInfo(idx);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 2)
    {
        ImVec2* pvar = (ImVec2*)var_info->GetVarPtr(&gp.Style);
        gp.StyleModifiers.push_back(ImGuiStyleMod((ImGuiStyleVar)idx, *pvar));
        *pvar = val;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() ImVec2 variant but variable is not a ImVec2!");
}

void PopStyleVar(int count) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(count <= gp.StyleModifiers.Size, "You can't pop more modifiers than have been pushed!");
    while (count > 0) {
        ImGuiStyleMod& backup = gp.StyleModifiers.back();
        const ImPlotStyleVarInfo* info = GetPlotStyleVarInfo(backup.VarIdx);
        void* data = info->GetVarPtr(&gp.Style);
        if (info->Type == ImGuiDataType_Float && info->Count == 1) {
            ((float*)data)[0] = backup.BackupFloat[0];
        }
        else if (info->Type == ImGuiDataType_Float && info->Count == 2) {
             ((float*)data)[0] = backup.BackupFloat[0];
             ((float*)data)[1] = backup.BackupFloat[1];
        }
        else if (info->Type == ImGuiDataType_S32 && info->Count == 1) {
            ((int*)data)[0] = backup.BackupInt[0];
        }
        gp.StyleModifiers.pop_back();
        count--;
    }
}

//------------------------------------------------------------------------------
// [Section] Colormaps
//------------------------------------------------------------------------------

ImPlotColormap AddColormap(const char* name, const ImVec4* colormap, int size, bool qual) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(size > 1, "The colormap size must be greater than 1!");
    IM_ASSERT_USER_ERROR(gp.ColormapData.GetIndex(name) == -1, "The colormap name has already been used!");
    ImVector<ImU32> buffer;
    buffer.resize(size);
    for (int i = 0; i < size; ++i)
        buffer[i] = ImGui::ColorConvertFloat4ToU32(colormap[i]);
    return gp.ColormapData.Append(name, buffer.Data, size, qual);
}

ImPlotColormap AddColormap(const char* name, const ImU32*  colormap, int size, bool qual) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(size > 1, "The colormap size must be greater than 1!");
    IM_ASSERT_USER_ERROR(gp.ColormapData.GetIndex(name) == -1, "The colormap name has already be used!");
    return gp.ColormapData.Append(name, colormap, size, qual);
}

int GetColormapCount() {
    ImPlotContext& gp = *GImPlot;
    return gp.ColormapData.Count;
}

const char* GetColormapName(ImPlotColormap colormap) {
    ImPlotContext& gp = *GImPlot;
    return gp.ColormapData.GetName(colormap);
}

ImPlotColormap GetColormapIndex(const char* name) {
    ImPlotContext& gp = *GImPlot;
    return gp.ColormapData.GetIndex(name);
}

void PushColormap(ImPlotColormap colormap) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(colormap >= 0 && colormap < gp.ColormapData.Count, "The colormap index is invalid!");
    gp.ColormapModifiers.push_back(gp.Style.Colormap);
    gp.Style.Colormap = colormap;
}

void PushColormap(const char* name) {
    ImPlotContext& gp = *GImPlot;
    ImPlotColormap idx = gp.ColormapData.GetIndex(name);
    IM_ASSERT_USER_ERROR(idx != -1, "The colormap name is invalid!");
    PushColormap(idx);
}

void PopColormap(int count) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(count <= gp.ColormapModifiers.Size, "You can't pop more modifiers than have been pushed!");
    while (count > 0) {
        const ImPlotColormap& backup = gp.ColormapModifiers.back();
        gp.Style.Colormap     = backup;
        gp.ColormapModifiers.pop_back();
        count--;
    }
}

ImU32 NextColormapColorU32() {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentItems != NULL, "NextColormapColor() needs to be called between BeginPlot() and EndPlot()!");
    int idx = gp.CurrentItems->ColormapIdx % gp.ColormapData.GetKeyCount(gp.Style.Colormap);
    ImU32 col  = gp.ColormapData.GetKeyColor(gp.Style.Colormap, idx);
    gp.CurrentItems->ColormapIdx++;
    return col;
}

ImVec4 NextColormapColor() {
    return ImGui::ColorConvertU32ToFloat4(NextColormapColorU32());
}

int GetColormapSize(ImPlotColormap cmap) {
    ImPlotContext& gp = *GImPlot;
    cmap = cmap == IMPLOT_AUTO ? gp.Style.Colormap : cmap;
    IM_ASSERT_USER_ERROR(cmap >= 0 && cmap < gp.ColormapData.Count, "Invalid colormap index!");
    return gp.ColormapData.GetKeyCount(cmap);
}

ImU32 GetColormapColorU32(int idx, ImPlotColormap cmap) {
    ImPlotContext& gp = *GImPlot;
    cmap = cmap == IMPLOT_AUTO ? gp.Style.Colormap : cmap;
    IM_ASSERT_USER_ERROR(cmap >= 0 && cmap < gp.ColormapData.Count, "Invalid colormap index!");
    idx = idx % gp.ColormapData.GetKeyCount(cmap);
    return gp.ColormapData.GetKeyColor(cmap, idx);
}

ImVec4 GetColormapColor(int idx, ImPlotColormap cmap) {
    return ImGui::ColorConvertU32ToFloat4(GetColormapColorU32(idx,cmap));
}

ImU32  SampleColormapU32(float t, ImPlotColormap cmap) {
    ImPlotContext& gp = *GImPlot;
    cmap = cmap == IMPLOT_AUTO ? gp.Style.Colormap : cmap;
    IM_ASSERT_USER_ERROR(cmap >= 0 && cmap < gp.ColormapData.Count, "Invalid colormap index!");
    return gp.ColormapData.LerpTable(cmap, t);
}

ImVec4 SampleColormap(float t, ImPlotColormap cmap) {
    return ImGui::ColorConvertU32ToFloat4(SampleColormapU32(t,cmap));
}

void RenderColorBar(const ImU32* colors, int size, ImDrawList& DrawList, const ImRect& bounds, bool vert, bool reversed, bool continuous) {
    const int n = continuous ? size - 1 : size;
    ImU32 col1, col2;
    if (vert) {
        const float step = bounds.GetHeight() / n;
        ImRect rect(bounds.Min.x, bounds.Min.y, bounds.Max.x, bounds.Min.y + step);
        for (int i = 0; i < n; ++i) {
            if (reversed) {
                col1 = colors[size-i-1];
                col2 = continuous ? colors[size-i-2] : col1;
            }
            else {
                col1 = colors[i];
                col2 = continuous ? colors[i+1] : col1;
            }
            DrawList.AddRectFilledMultiColor(rect.Min, rect.Max, col1, col1, col2, col2);
            rect.TranslateY(step);
        }
    }
    else {
        const float step = bounds.GetWidth() / n;
        ImRect rect(bounds.Min.x, bounds.Min.y, bounds.Min.x + step, bounds.Max.y);
        for (int i = 0; i < n; ++i) {
            if (reversed) {
                col1 = colors[size-i-1];
                col2 = continuous ? colors[size-i-2] : col1;
            }
            else {
                col1 = colors[i];
                col2 = continuous ? colors[i+1] : col1;
            }
            DrawList.AddRectFilledMultiColor(rect.Min, rect.Max, col1, col2, col2, col1);
            rect.TranslateX(step);
        }
    }
}

void ColormapScale(const char* label, double scale_min, double scale_max, const ImVec2& size, const char* format, ImPlotColormapScaleFlags flags, ImPlotColormap cmap) {
    ImGuiContext &G      = *GImGui;
    ImGuiWindow * Window = G.CurrentWindow;
    if (Window->SkipItems)
        return;

    const ImGuiID ID = Window->GetID(label);
    ImVec2 label_size(0,0);
    if (!ImHasFlag(flags, ImPlotColormapScaleFlags_NoLabel)) {
        label_size = ImGui::CalcTextSize(label,NULL,true);
    }

    ImPlotContext& gp = *GImPlot;
    cmap = cmap == IMPLOT_AUTO ? gp.Style.Colormap : cmap;
    IM_ASSERT_USER_ERROR(cmap >= 0 && cmap < gp.ColormapData.Count, "Invalid colormap index!");

    ImVec2 frame_size  = ImGui::CalcItemSize(size, 0, gp.Style.PlotDefaultSize.y);
    if (frame_size.y < gp.Style.PlotMinSize.y && size.y < 0.0f)
        frame_size.y = gp.Style.PlotMinSize.y;

    ImPlotRange range(ImMin(scale_min,scale_max), ImMax(scale_min,scale_max));
    gp.CTicker.Reset();
    Locator_Default(gp.CTicker, range, frame_size.y, true, Formatter_Default, (void*)format);

    const bool rend_label = label_size.x > 0;
    const float txt_off   = gp.Style.LabelPadding.x;
    const float pad       = txt_off + gp.CTicker.MaxSize.x + (rend_label ? txt_off + label_size.y : 0);
    float bar_w           = 20;
    if (frame_size.x == 0)
        frame_size.x = bar_w + pad + 2 * gp.Style.PlotPadding.x;
    else {
        bar_w = frame_size.x - (pad + 2 * gp.Style.PlotPadding.x);
        if (bar_w < gp.Style.MajorTickLen.y)
            bar_w = gp.Style.MajorTickLen.y;
    }

    ImDrawList &DrawList = *Window->DrawList;
    ImRect bb_frame = ImRect(Window->DC.CursorPos, Window->DC.CursorPos + frame_size);
    ImGui::ItemSize(bb_frame);
    if (!ImGui::ItemAdd(bb_frame, ID, &bb_frame))
        return;

    ImGui::RenderFrame(bb_frame.Min, bb_frame.Max, GetStyleColorU32(ImPlotCol_FrameBg), true, G.Style.FrameRounding);

    const bool opposite = ImHasFlag(flags, ImPlotColormapScaleFlags_Opposite);
    const bool inverted = ImHasFlag(flags, ImPlotColormapScaleFlags_Invert);
    const bool reversed = scale_min > scale_max;

    float bb_grad_shift = opposite ? pad : 0;
    ImRect bb_grad(bb_frame.Min + gp.Style.PlotPadding + ImVec2(bb_grad_shift, 0),
                   bb_frame.Min + ImVec2(bar_w + gp.Style.PlotPadding.x + bb_grad_shift,
                                         frame_size.y - gp.Style.PlotPadding.y));

    ImGui::PushClipRect(bb_frame.Min, bb_frame.Max, true);
    const ImU32 col_text = ImGui::GetColorU32(ImGuiCol_Text);

    const bool invert_scale = inverted ? (reversed ? false : true) : (reversed ? true : false);
    const float y_min = invert_scale ? bb_grad.Max.y : bb_grad.Min.y;
    const float y_max = invert_scale ? bb_grad.Min.y : bb_grad.Max.y;

    RenderColorBar(gp.ColormapData.GetKeys(cmap), gp.ColormapData.GetKeyCount(cmap), DrawList, bb_grad, true, !inverted, !gp.ColormapData.IsQual(cmap));
    for (int i = 0; i < gp.CTicker.TickCount(); ++i) {
        const double y_pos_plt = gp.CTicker.Ticks[i].PlotPos;
        const float y_pos = ImRemap((float)y_pos_plt, (float)range.Max, (float)range.Min, y_min, y_max);
        const float tick_width = gp.CTicker.Ticks[i].Major ? gp.Style.MajorTickLen.y : gp.Style.MinorTickLen.y;
        const float tick_thick = gp.CTicker.Ticks[i].Major ? gp.Style.MajorTickSize.y : gp.Style.MinorTickSize.y;
        const float tick_t     = (float)((y_pos_plt - scale_min) / (scale_max - scale_min));
        const ImU32 tick_col = CalcTextColor(GImPlot->ColormapData.LerpTable(cmap,tick_t));
        if (y_pos < bb_grad.Max.y - 2 && y_pos > bb_grad.Min.y + 2) {
            DrawList.AddLine(opposite ? ImVec2(bb_grad.Min.x+1, y_pos) : ImVec2(bb_grad.Max.x-1, y_pos),
                             opposite ? ImVec2(bb_grad.Min.x + tick_width, y_pos) : ImVec2(bb_grad.Max.x - tick_width, y_pos),
                             tick_col,
                             tick_thick);
        }
        const float txt_x = opposite ? bb_grad.Min.x - txt_off - gp.CTicker.Ticks[i].LabelSize.x : bb_grad.Max.x + txt_off;
        const float txt_y = y_pos - gp.CTicker.Ticks[i].LabelSize.y * 0.5f;
        DrawList.AddText(ImVec2(txt_x, txt_y), col_text, gp.CTicker.GetText(i));
    }

    if (rend_label) {
        const float pos_x = opposite ? bb_frame.Min.x + gp.Style.PlotPadding.x : bb_grad.Max.x + 2 * txt_off + gp.CTicker.MaxSize.x;
        const float pos_y = bb_grad.GetCenter().y + label_size.x * 0.5f;
        const char* label_end = ImGui::FindRenderedTextEnd(label);
        AddTextVertical(&DrawList,ImVec2(pos_x,pos_y),col_text,label,label_end);
    }
    DrawList.AddRect(bb_grad.Min, bb_grad.Max, GetStyleColorU32(ImPlotCol_PlotBorder));
    ImGui::PopClipRect();
}

bool ColormapSlider(const char* label, float* t, ImVec4* out, const char* format, ImPlotColormap cmap) {
    *t = ImClamp(*t,0.0f,1.0f);
    ImGuiContext &G      = *GImGui;
    ImGuiWindow * Window = G.CurrentWindow;
    if (Window->SkipItems)
        return false;
    ImPlotContext& gp = *GImPlot;
    cmap = cmap == IMPLOT_AUTO ? gp.Style.Colormap : cmap;
    IM_ASSERT_USER_ERROR(cmap >= 0 && cmap < gp.ColormapData.Count, "Invalid colormap index!");
    const ImU32* keys  = GImPlot->ColormapData.GetKeys(cmap);
    const int    count = GImPlot->ColormapData.GetKeyCount(cmap);
    const bool   qual  = GImPlot->ColormapData.IsQual(cmap);
    const ImVec2 pos  = ImGui::GetCurrentWindow()->DC.CursorPos;
    const float w     = ImGui::CalcItemWidth();
    const float h     = ImGui::GetFrameHeight();
    const ImRect rect = ImRect(pos.x,pos.y,pos.x+w,pos.y+h);
    RenderColorBar(keys,count,*ImGui::GetWindowDrawList(),rect,false,false,!qual);
    const ImU32 grab = CalcTextColor(GImPlot->ColormapData.LerpTable(cmap,*t));
    // const ImU32 text = CalcTextColor(GImPlot->ColormapData.LerpTable(cmap,0.5f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,IM_COL32_BLACK_TRANS);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,IM_COL32_BLACK_TRANS);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(1,1,1,0.1f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,grab);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, grab);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize,2);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,0);
    const bool changed = ImGui::SliderFloat(label,t,0,1,format);
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(2);
    if (out != NULL)
        *out = ImGui::ColorConvertU32ToFloat4(GImPlot->ColormapData.LerpTable(cmap,*t));
    return changed;
}

bool ColormapButton(const char* label, const ImVec2& size_arg, ImPlotColormap cmap) {
    ImGuiContext &G      = *GImGui;
    const ImGuiStyle& style = G.Style;
    ImGuiWindow * Window = G.CurrentWindow;
    if (Window->SkipItems)
        return false;
    ImPlotContext& gp = *GImPlot;
    cmap = cmap == IMPLOT_AUTO ? gp.Style.Colormap : cmap;
    IM_ASSERT_USER_ERROR(cmap >= 0 && cmap < gp.ColormapData.Count, "Invalid colormap index!");
    const ImU32* keys  = GImPlot->ColormapData.GetKeys(cmap);
    const int    count = GImPlot->ColormapData.GetKeyCount(cmap);
    const bool   qual  = GImPlot->ColormapData.IsQual(cmap);
    const ImVec2 pos  = ImGui::GetCurrentWindow()->DC.CursorPos;
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);
    const ImRect rect = ImRect(pos.x,pos.y,pos.x+size.x,pos.y+size.y);
    RenderColorBar(keys,count,*ImGui::GetWindowDrawList(),rect,false,false,!qual);
    const ImU32 text = CalcTextColor(GImPlot->ColormapData.LerpTable(cmap,G.Style.ButtonTextAlign.x));
    ImGui::PushStyleColor(ImGuiCol_Button,IM_COL32_BLACK_TRANS);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(1,1,1,0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(1,1,1,0.2f));
    ImGui::PushStyleColor(ImGuiCol_Text,text);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,0);
    const bool pressed = ImGui::Button(label,size);
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(1);
    return pressed;
}

//-----------------------------------------------------------------------------
// [Section] Miscellaneous
//-----------------------------------------------------------------------------

ImPlotInputMap& GetInputMap() {
    IM_ASSERT_USER_ERROR(GImPlot != NULL, "No current context. Did you call ImPlot::CreateContext() or ImPlot::SetCurrentContext()?");
    ImPlotContext& gp = *GImPlot;
    return gp.InputMap;
}

void MapInputDefault(ImPlotInputMap* dst) {
    ImPlotInputMap& map = dst ? *dst : GetInputMap();
    map.Pan             = ImGuiMouseButton_Left;
    map.PanMod          = ImGuiMod_None;
    map.Fit             = ImGuiMouseButton_Left;
    map.Menu            = ImGuiMouseButton_Right;
    map.Select          = ImGuiMouseButton_Right;
    map.SelectMod       = ImGuiMod_None;
    map.SelectCancel    = ImGuiMouseButton_Left;
    map.SelectHorzMod   = ImGuiMod_Alt;
    map.SelectVertMod   = ImGuiMod_Shift;
    map.OverrideMod     = ImGuiMod_Ctrl;
    map.ZoomMod         = ImGuiMod_None;
    map.ZoomRate        = 0.1f;
}

void MapInputReverse(ImPlotInputMap* dst) {
    ImPlotInputMap& map = dst ? *dst : GetInputMap();
    map.Pan             = ImGuiMouseButton_Right;
    map.PanMod          = ImGuiMod_None;
    map.Fit             = ImGuiMouseButton_Left;
    map.Menu            = ImGuiMouseButton_Right;
    map.Select          = ImGuiMouseButton_Left;
    map.SelectMod       = ImGuiMod_None;
    map.SelectCancel    = ImGuiMouseButton_Right;
    map.SelectHorzMod   = ImGuiMod_Alt;
    map.SelectVertMod   = ImGuiMod_Shift;
    map.OverrideMod     = ImGuiMod_Ctrl;
    map.ZoomMod         = ImGuiMod_None;
    map.ZoomRate        = 0.1f;
}

//-----------------------------------------------------------------------------
// [Section] Miscellaneous
//-----------------------------------------------------------------------------

void ItemIcon(const ImVec4& col) {
    ItemIcon(ImGui::ColorConvertFloat4ToU32(col));
}

void ItemIcon(ImU32 col) {
    const float txt_size = ImGui::GetTextLineHeight();
    ImVec2 size(txt_size-4,txt_size);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 pos = window->DC.CursorPos;
    ImGui::GetWindowDrawList()->AddRectFilled(pos + ImVec2(0,2), pos + size - ImVec2(0,2), col);
    ImGui::Dummy(size);
}

void ColormapIcon(ImPlotColormap cmap) {
    ImPlotContext& gp = *GImPlot;
    const float txt_size = ImGui::GetTextLineHeight();
    ImVec2 size(txt_size-4,txt_size);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 pos = window->DC.CursorPos;
    ImRect rect(pos+ImVec2(0,2),pos+size-ImVec2(0,2));
    ImDrawList& DrawList = *ImGui::GetWindowDrawList();
    RenderColorBar(gp.ColormapData.GetKeys(cmap),gp.ColormapData.GetKeyCount(cmap),DrawList,rect,false,false,!gp.ColormapData.IsQual(cmap));
    ImGui::Dummy(size);
}

ImDrawList* GetPlotDrawList() {
    return ImGui::GetWindowDrawList();
}

void PushPlotClipRect(float expand) {
    ImPlotContext& gp = *GImPlot;
    IM_ASSERT_USER_ERROR(gp.CurrentPlot != NULL, "PushPlotClipRect() needs to be called between BeginPlot() and EndPlot()!");
    SetupLock();
    ImRect rect = gp.CurrentPlot->PlotRect;
    rect.Expand(expand);
    ImGui::PushClipRect(rect.Min, rect.Max, true);
}

void PopPlotClipRect() {
    SetupLock();
    ImGui::PopClipRect();
}

static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool ShowStyleSelector(const char* label)
{
    static int style_idx = -1;
    if (ImGui::Combo(label, &style_idx, "Auto\0Classic\0Dark\0Light\0"))
    {
        switch (style_idx)
        {
        case 0: StyleColorsAuto(); break;
        case 1: StyleColorsClassic(); break;
        case 2: StyleColorsDark(); break;
        case 3: StyleColorsLight(); break;
        }
        return true;
    }
    return false;
}

bool ShowColormapSelector(const char* label) {
    ImPlotContext& gp = *GImPlot;
    bool set = false;
    if (ImGui::BeginCombo(label, gp.ColormapData.GetName(gp.Style.Colormap))) {
        for (int i = 0; i < gp.ColormapData.Count; ++i) {
            const char* name = gp.ColormapData.GetName(i);
            if (ImGui::Selectable(name, gp.Style.Colormap == i)) {
                gp.Style.Colormap = i;
                ImPlot::BustItemCache();
                set = true;
            }
        }
        ImGui::EndCombo();
    }
    return set;
}

bool ShowInputMapSelector(const char* label) {
    static int map_idx = -1;
    if (ImGui::Combo(label, &map_idx, "Default\0Reversed\0"))
    {
        switch (map_idx)
        {
        case 0: MapInputDefault(); break;
        case 1: MapInputReverse(); break;
        }
        return true;
    }
    return false;
}


void ShowStyleEditor(ImPlotStyle* ref) {
    ImPlotContext& gp = *GImPlot;
    ImPlotStyle& style = GetStyle();
    static ImPlotStyle ref_saved_style;
    // Default to using internal storage as reference
    static bool init = true;
    if (init && ref == NULL)
        ref_saved_style = style;
    init = false;
    if (ref == NULL)
        ref = &ref_saved_style;

    if (ImPlot::ShowStyleSelector("Colors##Selector"))
        ref_saved_style = style;

    // Save/Revert button
    if (ImGui::Button("Save Ref"))
        *ref = ref_saved_style = style;
    ImGui::SameLine();
    if (ImGui::Button("Revert Ref"))
        style = *ref;
    ImGui::SameLine();
    HelpMarker("Save/Revert in local non-persistent storage. Default Colors definition are not affected. "
               "Use \"Export\" below to save them somewhere.");
    if (ImGui::BeginTabBar("##StyleEditor")) {
        if (ImGui::BeginTabItem("Variables")) {
            ImGui::Text("Item Styling");
            ImGui::SliderFloat("LineWeight", &style.LineWeight, 0.0f, 5.0f, "%.1f");
            ImGui::SliderFloat("MarkerSize", &style.MarkerSize, 2.0f, 10.0f, "%.1f");
            ImGui::SliderFloat("MarkerWeight", &style.MarkerWeight, 0.0f, 5.0f, "%.1f");
            ImGui::SliderFloat("FillAlpha", &style.FillAlpha, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("ErrorBarSize", &style.ErrorBarSize, 0.0f, 10.0f, "%.1f");
            ImGui::SliderFloat("ErrorBarWeight", &style.ErrorBarWeight, 0.0f, 5.0f, "%.1f");
            ImGui::SliderFloat("DigitalBitHeight", &style.DigitalBitHeight, 0.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("DigitalBitGap", &style.DigitalBitGap, 0.0f, 20.0f, "%.1f");
            ImGui::Text("Plot Styling");
            ImGui::SliderFloat("PlotBorderSize", &style.PlotBorderSize, 0.0f, 2.0f, "%.0f");
            ImGui::SliderFloat("MinorAlpha", &style.MinorAlpha, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat2("MajorTickLen", (float*)&style.MajorTickLen, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("MinorTickLen", (float*)&style.MinorTickLen, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("MajorTickSize",  (float*)&style.MajorTickSize, 0.0f, 2.0f, "%.1f");
            ImGui::SliderFloat2("MinorTickSize", (float*)&style.MinorTickSize, 0.0f, 2.0f, "%.1f");
            ImGui::SliderFloat2("MajorGridSize", (float*)&style.MajorGridSize, 0.0f, 2.0f, "%.1f");
            ImGui::SliderFloat2("MinorGridSize", (float*)&style.MinorGridSize, 0.0f, 2.0f, "%.1f");
            ImGui::SliderFloat2("PlotDefaultSize", (float*)&style.PlotDefaultSize, 0.0f, 1000, "%.0f");
            ImGui::SliderFloat2("PlotMinSize", (float*)&style.PlotMinSize, 0.0f, 300, "%.0f");
            ImGui::Text("Plot Padding");
            ImGui::SliderFloat2("PlotPadding", (float*)&style.PlotPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("LabelPadding", (float*)&style.LabelPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("LegendPadding", (float*)&style.LegendPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("LegendInnerPadding", (float*)&style.LegendInnerPadding, 0.0f, 10.0f, "%.0f");
            ImGui::SliderFloat2("LegendSpacing", (float*)&style.LegendSpacing, 0.0f, 5.0f, "%.0f");
            ImGui::SliderFloat2("MousePosPadding", (float*)&style.MousePosPadding, 0.0f, 20.0f, "%.0f");
            ImGui::SliderFloat2("AnnotationPadding", (float*)&style.AnnotationPadding, 0.0f, 5.0f, "%.0f");
            ImGui::SliderFloat2("FitPadding", (float*)&style.FitPadding, 0, 0.2f, "%.2f");

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Colors")) {
            static int output_dest = 0;
            static bool output_only_modified = false;

            if (ImGui::Button("Export", ImVec2(75,0))) {
                if (output_dest == 0)
                    ImGui::LogToClipboard();
                else
                    ImGui::LogToTTY();
                ImGui::LogText("ImVec4* colors = ImPlot::GetStyle().Colors;\n");
                for (int i = 0; i < ImPlotCol_COUNT; i++) {
                    const ImVec4& col = style.Colors[i];
                    const char* name = ImPlot::GetStyleColorName(i);
                    if (!output_only_modified || memcmp(&col, &ref->Colors[i], sizeof(ImVec4)) != 0) {
                        if (IsColorAuto(i))
                            ImGui::LogText("colors[ImPlotCol_%s]%*s= IMPLOT_AUTO_COL;\n",name,14 - (int)strlen(name), "");
                        else
                            ImGui::LogText("colors[ImPlotCol_%s]%*s= ImVec4(%.2ff, %.2ff, %.2ff, %.2ff);\n",
                                        name, 14 - (int)strlen(name), "", col.x, col.y, col.z, col.w);
                    }
                }
                ImGui::LogFinish();
            }
            ImGui::SameLine(); ImGui::SetNextItemWidth(120); ImGui::Combo("##output_type", &output_dest, "To Clipboard\0To TTY\0");
            ImGui::SameLine(); ImGui::Checkbox("Only Modified Colors", &output_only_modified);

            static ImGuiTextFilter filter;
            filter.Draw("Filter colors", ImGui::GetFontSize() * 16);

            static ImGuiColorEditFlags alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf;
            if (ImGui::RadioButton("Opaque", alpha_flags == ImGuiColorEditFlags_None))             { alpha_flags = ImGuiColorEditFlags_None; } ImGui::SameLine();
            if (ImGui::RadioButton("Alpha",  alpha_flags == ImGuiColorEditFlags_AlphaPreview))     { alpha_flags = ImGuiColorEditFlags_AlphaPreview; } ImGui::SameLine();
            if (ImGui::RadioButton("Both",   alpha_flags == ImGuiColorEditFlags_AlphaPreviewHalf)) { alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf; } ImGui::SameLine();
            HelpMarker(
                "In the color list:\n"
                "Left-click on colored square to open color picker,\n"
                "Right-click to open edit options menu.");
            ImGui::Separator();
            ImGui::PushItemWidth(-160);
            for (int i = 0; i < ImPlotCol_COUNT; i++) {
                const char* name = ImPlot::GetStyleColorName(i);
                if (!filter.PassFilter(name))
                    continue;
                ImGui::PushID(i);
                ImVec4 temp = GetStyleColorVec4(i);
                const bool is_auto = IsColorAuto(i);
                if (!is_auto)
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.25f);
                if (ImGui::Button("Auto")) {
                    if (is_auto)
                        style.Colors[i] = temp;
                    else
                        style.Colors[i] = IMPLOT_AUTO_COL;
                    BustItemCache();
                }
                if (!is_auto)
                    ImGui::PopStyleVar();
                ImGui::SameLine();
                if (ImGui::ColorEdit4(name, &temp.x, ImGuiColorEditFlags_NoInputs | alpha_flags)) {
                    style.Colors[i] = temp;
                    BustItemCache();
                }
                if (memcmp(&style.Colors[i], &ref->Colors[i], sizeof(ImVec4)) != 0) {
                    ImGui::SameLine(175); if (ImGui::Button("Save")) { ref->Colors[i] = style.Colors[i]; }
                    ImGui::SameLine(); if (ImGui::Button("Revert")) {
                        style.Colors[i] = ref->Colors[i];
                        BustItemCache();
                    }
                }
                ImGui::PopID();
            }
            ImGui::PopItemWidth();
            ImGui::Separator();
            ImGui::Text("Colors that are set to Auto (i.e. IMPLOT_AUTO_COL) will\n"
                        "be automatically deduced from your ImGui style or the\n"
                        "current ImPlot Colormap. If you want to style individual\n"
                        "plot items, use Push/PopStyleColor around its function.");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Colormaps")) {
            static int output_dest = 0;
            if (ImGui::Button("Export", ImVec2(75,0))) {
                if (output_dest == 0)
                    ImGui::LogToClipboard();
                else
                    ImGui::LogToTTY();
                int size = GetColormapSize();
                const char* name = GetColormapName(gp.Style.Colormap);
                ImGui::LogText("static const ImU32 %s_Data[%d] = {\n", name, size);
                for (int i = 0; i < size; ++i) {
                    ImU32 col = GetColormapColorU32(i,gp.Style.Colormap);
                    ImGui::LogText("    %u%s\n", col, i == size - 1 ? "" : ",");
                }
                ImGui::LogText("};\nImPlotColormap %s = ImPlot::AddColormap(\"%s\", %s_Data, %d);", name, name, name, size);
                ImGui::LogFinish();
            }
            ImGui::SameLine(); ImGui::SetNextItemWidth(120); ImGui::Combo("##output_type", &output_dest, "To Clipboard\0To TTY\0");
            ImGui::SameLine();
            static bool edit = false;
            ImGui::Checkbox("Edit Mode",&edit);

            // built-in/added
            ImGui::Separator();
            for (int i = 0; i < gp.ColormapData.Count; ++i) {
                ImGui::PushID(i);
                int size = gp.ColormapData.GetKeyCount(i);
                bool selected = i == gp.Style.Colormap;

                const char* name = GetColormapName(i);
                if (!selected)
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.25f);
                if (ImGui::Button(name, ImVec2(100,0))) {
                    gp.Style.Colormap = i;
                    BustItemCache();
                }
                if (!selected)
                    ImGui::PopStyleVar();
                ImGui::SameLine();
                ImGui::BeginGroup();
                if (edit) {
                    for (int c = 0; c < size; ++c) {
                        ImGui::PushID(c);
                        ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(gp.ColormapData.GetKeyColor(i,c));
                        if (ImGui::ColorEdit4("",&col4.x,ImGuiColorEditFlags_NoInputs)) {
                            ImU32 col32 = ImGui::ColorConvertFloat4ToU32(col4);
                            gp.ColormapData.SetKeyColor(i,c,col32);
                            BustItemCache();
                        }
                        if ((c + 1) % 12 != 0 && c != size -1)
                            ImGui::SameLine();
                        ImGui::PopID();
                    }
                }
                else {
                    if (ImPlot::ColormapButton("##",ImVec2(-1,0),i))
                        edit = true;
                }
                ImGui::EndGroup();
                ImGui::PopID();
            }


            static ImVector<ImVec4> custom;
            if (custom.Size == 0) {
                custom.push_back(ImVec4(1,0,0,1));
                custom.push_back(ImVec4(0,1,0,1));
                custom.push_back(ImVec4(0,0,1,1));
            }
            ImGui::Separator();
            ImGui::BeginGroup();
            static char name[16] = "MyColormap";


            if (ImGui::Button("+", ImVec2((100 - ImGui::GetStyle().ItemSpacing.x)/2,0)))
                custom.push_back(ImVec4(0,0,0,1));
            ImGui::SameLine();
            if (ImGui::Button("-", ImVec2((100 - ImGui::GetStyle().ItemSpacing.x)/2,0)) && custom.Size > 2)
                custom.pop_back();
            ImGui::SetNextItemWidth(100);
            ImGui::InputText("##Name",name,16,ImGuiInputTextFlags_CharsNoBlank);
            static bool qual = true;
            ImGui::Checkbox("Qualitative",&qual);
            if (ImGui::Button("Add", ImVec2(100, 0)) && gp.ColormapData.GetIndex(name)==-1)
                AddColormap(name,custom.Data,custom.Size,qual);

            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            for (int c = 0; c < custom.Size; ++c) {
                ImGui::PushID(c);
                if (ImGui::ColorEdit4("##Col1", &custom[c].x, ImGuiColorEditFlags_NoInputs)) {

                }
                if ((c + 1) % 12 != 0)
                    ImGui::SameLine();
                ImGui::PopID();
            }
            ImGui::EndGroup();


            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void ShowUserGuide() {
        ImGui::BulletText("Left-click drag within the plot area to pan X and Y axes.");
    ImGui::Indent();
        ImGui::BulletText("Left-click drag on axis labels to pan an individual axis.");
    ImGui::Unindent();
    ImGui::BulletText("Scroll in the plot area to zoom both X any Y axes.");
    ImGui::Indent();
        ImGui::BulletText("Scroll on axis labels to zoom an individual axis.");
    ImGui::Unindent();
    ImGui::BulletText("Right-click drag to box select data.");
    ImGui::Indent();
        ImGui::BulletText("Hold Alt to expand box selection horizontally.");
        ImGui::BulletText("Hold Shift to expand box selection vertically.");
        ImGui::BulletText("Left-click while box selecting to cancel the selection.");
    ImGui::Unindent();
    ImGui::BulletText("Double left-click to fit all visible data.");
    ImGui::Indent();
        ImGui::BulletText("Double left-click axis labels to fit the individual axis.");
    ImGui::Unindent();
    ImGui::BulletText("Right-click open the full plot context menu.");
    ImGui::Indent();
        ImGui::BulletText("Right-click axis labels to open an individual axis context menu.");
    ImGui::Unindent();
    ImGui::BulletText("Click legend label icons to show/hide plot items.");
}

void ShowTicksMetrics(const ImPlotTicker& ticker) {
    ImGui::BulletText("Size: %d", ticker.TickCount());
    ImGui::BulletText("MaxSize: [%f,%f]", ticker.MaxSize.x, ticker.MaxSize.y);
}

void ShowAxisMetrics(const ImPlotPlot& plot, const ImPlotAxis& axis) {
    ImGui::BulletText("Label: %s", axis.LabelOffset == -1 ? "[none]" : plot.GetAxisLabel(axis));
    ImGui::BulletText("Flags: 0x%08X", axis.Flags);
    ImGui::BulletText("Range: [%f,%f]",axis.Range.Min, axis.Range.Max);
    ImGui::BulletText("Pixels: %f", axis.PixelSize());
    ImGui::BulletText("Aspect: %f", axis.GetAspect());
    ImGui::BulletText(axis.OrthoAxis == NULL ? "OrtherAxis: NULL" : "OrthoAxis: 0x%08X", axis.OrthoAxis->ID);
    ImGui::BulletText("LinkedMin: %p", (void*)axis.LinkedMin);
    ImGui::BulletText("LinkedMax: %p", (void*)axis.LinkedMax);
    ImGui::BulletText("HasRange: %s", axis.HasRange ? "true" : "false");
    ImGui::BulletText("Hovered: %s", axis.Hovered ? "true" : "false");
    ImGui::BulletText("Held: %s", axis.Held ? "true" : "false");

    if (ImGui::TreeNode("Transform")) {
        ImGui::BulletText("PixelMin: %f", axis.PixelMin);
        ImGui::BulletText("PixelMax: %f", axis.PixelMax);
        ImGui::BulletText("ScaleToPixel: %f", axis.ScaleToPixel);
        ImGui::BulletText("ScaleMax: %f", axis.ScaleMax);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Ticks")) {
        ShowTicksMetrics(axis.Ticker);
        ImGui::TreePop();
    }
}

void ShowMetricsWindow(bool* p_popen) {

    static bool show_plot_rects = false;
    static bool show_axes_rects = false;
    static bool show_axis_rects = false;
    static bool show_canvas_rects = false;
    static bool show_frame_rects = false;
    static bool show_subplot_frame_rects = false;
    static bool show_subplot_grid_rects = false;

    ImDrawList& fg = *ImGui::GetForegroundDrawList();

    ImPlotContext& gp = *GImPlot;
    // ImGuiContext& g = *GImGui;
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("ImPlot Metrics", p_popen);
    ImGui::Text("ImPlot " IMPLOT_VERSION);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("Mouse Position: [%.0f,%.0f]", io.MousePos.x, io.MousePos.y);
    ImGui::Separator();
    if (ImGui::TreeNode("Tools")) {
        if (ImGui::Button("Bust Plot Cache"))
            BustPlotCache();
        ImGui::SameLine();
        if (ImGui::Button("Bust Item Cache"))
            BustItemCache();
        ImGui::Checkbox("Show Frame Rects", &show_frame_rects);
        ImGui::Checkbox("Show Canvas Rects",&show_canvas_rects);
        ImGui::Checkbox("Show Plot Rects",  &show_plot_rects);
        ImGui::Checkbox("Show Axes Rects",  &show_axes_rects);
        ImGui::Checkbox("Show Axis Rects",  &show_axis_rects);
        ImGui::Checkbox("Show Subplot Frame Rects",  &show_subplot_frame_rects);
        ImGui::Checkbox("Show Subplot Grid Rects",  &show_subplot_grid_rects);
        ImGui::TreePop();
    }
    const int n_plots = gp.Plots.GetBufSize();
    const int n_subplots = gp.Subplots.GetBufSize();
    // render rects
    for (int p = 0; p < n_plots; ++p) {
        ImPlotPlot* plot = gp.Plots.GetByIndex(p);
        if (show_frame_rects)
            fg.AddRect(plot->FrameRect.Min, plot->FrameRect.Max, IM_COL32(255,0,255,255));
        if (show_canvas_rects)
            fg.AddRect(plot->CanvasRect.Min, plot->CanvasRect.Max, IM_COL32(0,255,255,255));
        if (show_plot_rects)
            fg.AddRect(plot->PlotRect.Min, plot->PlotRect.Max, IM_COL32(255,255,0,255));
        if (show_axes_rects)
            fg.AddRect(plot->AxesRect.Min, plot->AxesRect.Max, IM_COL32(0,255,128,255));
        if (show_axis_rects) {
            for (int i = 0; i < ImAxis_COUNT; ++i) {
                if (plot->Axes[i].Enabled)
                    fg.AddRect(plot->Axes[i].HoverRect.Min, plot->Axes[i].HoverRect.Max, IM_COL32(0,255,0,255));
            }
        }
    }
    for (int p = 0; p < n_subplots; ++p) {
        ImPlotSubplot* subplot = gp.Subplots.GetByIndex(p);
        if (show_subplot_frame_rects)
            fg.AddRect(subplot->FrameRect.Min, subplot->FrameRect.Max, IM_COL32(255,0,0,255));
        if (show_subplot_grid_rects)
            fg.AddRect(subplot->GridRect.Min, subplot->GridRect.Max, IM_COL32(0,0,255,255));
    }
    if (ImGui::TreeNode("Plots","Plots (%d)", n_plots)) {
        for (int p = 0; p < n_plots; ++p) {
            // plot
            ImPlotPlot& plot = *gp.Plots.GetByIndex(p);
            ImGui::PushID(p);
            if (ImGui::TreeNode("Plot", "Plot [0x%08X]", plot.ID)) {
                int n_items = plot.Items.GetItemCount();
                if (ImGui::TreeNode("Items", "Items (%d)", n_items)) {
                    for (int i = 0; i < n_items; ++i) {
                        ImPlotItem* item = plot.Items.GetItemByIndex(i);
                        ImGui::PushID(i);
                        if (ImGui::TreeNode("Item", "Item [0x%08X]", item->ID)) {
                            ImGui::Bullet(); ImGui::Checkbox("Show", &item->Show);
                            ImGui::Bullet();
                            ImVec4 temp = ImGui::ColorConvertU32ToFloat4(item->Color);
                            if (ImGui::ColorEdit4("Color",&temp.x, ImGuiColorEditFlags_NoInputs))
                                item->Color = ImGui::ColorConvertFloat4ToU32(temp);

                            ImGui::BulletText("NameOffset: %d",item->NameOffset);
                            ImGui::BulletText("Name: %s", item->NameOffset != -1 ? plot.Items.Legend.Labels.Buf.Data + item->NameOffset : "N/A");
                            ImGui::BulletText("Hovered: %s",item->LegendHovered ? "true" : "false");
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                char buff[16];
                for (int i = 0; i < IMPLOT_NUM_X_AXES; ++i) {
                    ImFormatString(buff,16,"X-Axis %d", i+1);
                    if (plot.XAxis(i).Enabled && ImGui::TreeNode(buff, "X-Axis %d [0x%08X]", i+1, plot.XAxis(i).ID)) {
                        ShowAxisMetrics(plot, plot.XAxis(i));
                        ImGui::TreePop();
                    }
                }
                for (int i = 0; i < IMPLOT_NUM_Y_AXES; ++i) {
                    ImFormatString(buff,16,"Y-Axis %d", i+1);
                    if (plot.YAxis(i).Enabled && ImGui::TreeNode(buff, "Y-Axis %d [0x%08X]", i+1, plot.YAxis(i).ID)) {
                        ShowAxisMetrics(plot, plot.YAxis(i));
                        ImGui::TreePop();
                    }
                }
                ImGui::BulletText("Title: %s", plot.HasTitle() ? plot.GetTitle() : "none");
                ImGui::BulletText("Flags: 0x%08X", plot.Flags);
                ImGui::BulletText("Initialized: %s", plot.Initialized ? "true" : "false");
                ImGui::BulletText("Selecting: %s", plot.Selecting ? "true" : "false");
                ImGui::BulletText("Selected: %s", plot.Selected ? "true" : "false");
                ImGui::BulletText("Hovered: %s", plot.Hovered ? "true" : "false");
                ImGui::BulletText("Held: %s", plot.Held ? "true" : "false");
                ImGui::BulletText("LegendHovered: %s", plot.Items.Legend.Hovered ? "true" : "false");
                ImGui::BulletText("ContextLocked: %s", plot.ContextLocked ? "true" : "false");
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Subplots","Subplots (%d)", n_subplots)) {
        for (int p = 0; p < n_subplots; ++p) {
            // plot
            ImPlotSubplot& plot = *gp.Subplots.GetByIndex(p);
            ImGui::PushID(p);
            if (ImGui::TreeNode("Subplot", "Subplot [0x%08X]", plot.ID)) {
                int n_items = plot.Items.GetItemCount();
                if (ImGui::TreeNode("Items", "Items (%d)", n_items)) {
                    for (int i = 0; i < n_items; ++i) {
                        ImPlotItem* item = plot.Items.GetItemByIndex(i);
                        ImGui::PushID(i);
                        if (ImGui::TreeNode("Item", "Item [0x%08X]", item->ID)) {
                            ImGui::Bullet(); ImGui::Checkbox("Show", &item->Show);
                            ImGui::Bullet();
                            ImVec4 temp = ImGui::ColorConvertU32ToFloat4(item->Color);
                            if (ImGui::ColorEdit4("Color",&temp.x, ImGuiColorEditFlags_NoInputs))
                                item->Color = ImGui::ColorConvertFloat4ToU32(temp);

                            ImGui::BulletText("NameOffset: %d",item->NameOffset);
                            ImGui::BulletText("Name: %s", item->NameOffset != -1 ? plot.Items.Legend.Labels.Buf.Data + item->NameOffset : "N/A");
                            ImGui::BulletText("Hovered: %s",item->LegendHovered ? "true" : "false");
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                ImGui::BulletText("Flags: 0x%08X", plot.Flags);
                ImGui::BulletText("FrameHovered: %s", plot.FrameHovered ? "true" : "false");
                ImGui::BulletText("LegendHovered: %s", plot.Items.Legend.Hovered ? "true" : "false");
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Colormaps")) {
        ImGui::BulletText("Colormaps:  %d", gp.ColormapData.Count);
        ImGui::BulletText("Memory: %d bytes", gp.ColormapData.Tables.Size * 4);
        if (ImGui::TreeNode("Data")) {
            for (int m = 0; m < gp.ColormapData.Count; ++m) {
                if (ImGui::TreeNode(gp.ColormapData.GetName(m))) {
                    int count = gp.ColormapData.GetKeyCount(m);
                    int size = gp.ColormapData.GetTableSize(m);
                    bool qual = gp.ColormapData.IsQual(m);
                    ImGui::BulletText("Qualitative: %s", qual ? "true" : "false");
                    ImGui::BulletText("Key Count: %d", count);
                    ImGui::BulletText("Table Size: %d", size);
                    ImGui::Indent();

                    static float t = 0.5;
                    ImVec4 samp;
                    float wid = 32 * 10 - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x;
                    ImGui::SetNextItemWidth(wid);
                    ImPlot::ColormapSlider("##Sample",&t,&samp,"%.3f",m);
                    ImGui::SameLine();
                    ImGui::ColorButton("Sampler",samp);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));
                    for (int c = 0; c < size; ++c) {
                        ImVec4 col = ImGui::ColorConvertU32ToFloat4(gp.ColormapData.GetTableColor(m,c));
                        ImGui::PushID(m*1000+c);
                        ImGui::ColorButton("",col,0,ImVec2(10,10));
                        ImGui::PopID();
                        if ((c + 1) % 32 != 0 && c != size - 1)
                            ImGui::SameLine();
                    }
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                    ImGui::Unindent();
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }
    ImGui::End();
}

bool ShowDatePicker(const char* id, int* level, ImPlotTime* t, const ImPlotTime* t1, const ImPlotTime* t2) {

    ImGui::PushID(id);
    ImGui::BeginGroup();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 col_txt    = style.Colors[ImGuiCol_Text];
    ImVec4 col_dis    = style.Colors[ImGuiCol_TextDisabled];
    ImVec4 col_btn    = style.Colors[ImGuiCol_Button];
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

    const float ht    = ImGui::GetFrameHeight();
    ImVec2 cell_size(ht*1.25f,ht);
    char buff[32];
    bool clk = false;
    tm& Tm = GImPlot->Tm;

    const int min_yr = 1970;
    const int max_yr = 2999;

    // t1 parts
    int t1_mo = 0; int t1_md = 0; int t1_yr = 0;
    if (t1 != NULL) {
        GetTime(*t1,&Tm);
        t1_mo = Tm.tm_mon;
        t1_md = Tm.tm_mday;
        t1_yr = Tm.tm_year + 1900;
    }

     // t2 parts
    int t2_mo = 0; int t2_md = 0; int t2_yr = 0;
    if (t2 != NULL) {
        GetTime(*t2,&Tm);
        t2_mo = Tm.tm_mon;
        t2_md = Tm.tm_mday;
        t2_yr = Tm.tm_year + 1900;
    }

    // day widget
    if (*level == 0) {
        *t = FloorTime(*t, ImPlotTimeUnit_Day);
        GetTime(*t, &Tm);
        const int this_year = Tm.tm_year + 1900;
        const int last_year = this_year - 1;
        const int next_year = this_year + 1;
        const int this_mon  = Tm.tm_mon;
        const int last_mon  = this_mon == 0 ? 11 : this_mon - 1;
        const int next_mon  = this_mon == 11 ? 0 : this_mon + 1;
        const int days_this_mo = GetDaysInMonth(this_year, this_mon);
        const int days_last_mo = GetDaysInMonth(this_mon == 0 ? last_year : this_year, last_mon);
        ImPlotTime t_first_mo = FloorTime(*t,ImPlotTimeUnit_Mo);
        GetTime(t_first_mo,&Tm);
        const int first_wd = Tm.tm_wday;
        // month year
        ImFormatString(buff, 32, "%s %d", MONTH_NAMES[this_mon], this_year);
        if (ImGui::Button(buff))
            *level = 1;
        ImGui::SameLine(5*cell_size.x);
        BeginDisabledControls(this_year <= min_yr && this_mon == 0);
        if (ImGui::ArrowButtonEx("##Up",ImGuiDir_Up,cell_size))
            *t = AddTime(*t, ImPlotTimeUnit_Mo, -1);
        EndDisabledControls(this_year <= min_yr && this_mon == 0);
        ImGui::SameLine();
        BeginDisabledControls(this_year >= max_yr && this_mon == 11);
        if (ImGui::ArrowButtonEx("##Down",ImGuiDir_Down,cell_size))
            *t = AddTime(*t, ImPlotTimeUnit_Mo, 1);
        EndDisabledControls(this_year >= max_yr && this_mon == 11);
        // render weekday abbreviations
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        for (int i = 0; i < 7; ++i) {
            ImGui::Button(WD_ABRVS[i],cell_size);
            if (i != 6) { ImGui::SameLine(); }
        }
        ImGui::PopItemFlag();
        // 0 = last mo, 1 = this mo, 2 = next mo
        int mo = first_wd > 0 ? 0 : 1;
        int day = mo == 1 ? 1 : days_last_mo - first_wd + 1;
        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < 7; ++j) {
                if (mo == 0 && day > days_last_mo) {
                    mo = 1;
                    day = 1;
                }
                else if (mo == 1 && day > days_this_mo) {
                    mo = 2;
                    day = 1;
                }
                const int now_yr = (mo == 0 && this_mon == 0) ? last_year : ((mo == 2 && this_mon == 11) ? next_year : this_year);
                const int now_mo = mo == 0 ? last_mon : (mo == 1 ? this_mon : next_mon);
                const int now_md = day;

                const bool off_mo   = mo == 0 || mo == 2;
                const bool t1_or_t2 = (t1 != NULL && t1_mo == now_mo && t1_yr == now_yr && t1_md == now_md) ||
                                      (t2 != NULL && t2_mo == now_mo && t2_yr == now_yr && t2_md == now_md);

                if (off_mo)
                    ImGui::PushStyleColor(ImGuiCol_Text, col_dis);
                if (t1_or_t2) {
                    ImGui::PushStyleColor(ImGuiCol_Button, col_btn);
                    ImGui::PushStyleColor(ImGuiCol_Text, col_txt);
                }
                ImGui::PushID(i*7+j);
                ImFormatString(buff,32,"%d",day);
                if (now_yr == min_yr-1 || now_yr == max_yr+1) {
                    ImGui::Dummy(cell_size);
                }
                else if (ImGui::Button(buff,cell_size) && !clk) {
                    *t = MakeTime(now_yr, now_mo, now_md);
                    clk = true;
                }
                ImGui::PopID();
                if (t1_or_t2)
                    ImGui::PopStyleColor(2);
                if (off_mo)
                    ImGui::PopStyleColor();
                if (j != 6)
                    ImGui::SameLine();
                day++;
            }
        }
    }
    // month widget
    else if (*level == 1) {
        *t = FloorTime(*t, ImPlotTimeUnit_Mo);
        GetTime(*t, &Tm);
        int this_yr  = Tm.tm_year + 1900;
        ImFormatString(buff, 32, "%d", this_yr);
        if (ImGui::Button(buff))
            *level = 2;
        BeginDisabledControls(this_yr <= min_yr);
        ImGui::SameLine(5*cell_size.x);
        if (ImGui::ArrowButtonEx("##Up",ImGuiDir_Up,cell_size))
            *t = AddTime(*t, ImPlotTimeUnit_Yr, -1);
        EndDisabledControls(this_yr <= min_yr);
        ImGui::SameLine();
        BeginDisabledControls(this_yr >= max_yr);
        if (ImGui::ArrowButtonEx("##Down",ImGuiDir_Down,cell_size))
            *t = AddTime(*t, ImPlotTimeUnit_Yr, 1);
        EndDisabledControls(this_yr >= max_yr);
        // ImGui::Dummy(cell_size);
        cell_size.x *= 7.0f/4.0f;
        cell_size.y *= 7.0f/3.0f;
        int mo = 0;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 4; ++j) {
                const bool t1_or_t2 = (t1 != NULL && t1_yr == this_yr && t1_mo == mo) ||
                                      (t2 != NULL && t2_yr == this_yr && t2_mo == mo);
                if (t1_or_t2)
                    ImGui::PushStyleColor(ImGuiCol_Button, col_btn);
                if (ImGui::Button(MONTH_ABRVS[mo],cell_size) && !clk) {
                    *t = MakeTime(this_yr, mo);
                    *level = 0;
                }
                if (t1_or_t2)
                    ImGui::PopStyleColor();
                if (j != 3)
                    ImGui::SameLine();
                mo++;
            }
        }
    }
    else if (*level == 2) {
        *t = FloorTime(*t, ImPlotTimeUnit_Yr);
        int this_yr = GetYear(*t);
        int yr = this_yr  - this_yr % 20;
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImFormatString(buff,32,"%d-%d",yr,yr+19);
        ImGui::Button(buff);
        ImGui::PopItemFlag();
        ImGui::SameLine(5*cell_size.x);
        BeginDisabledControls(yr <= min_yr);
        if (ImGui::ArrowButtonEx("##Up",ImGuiDir_Up,cell_size))
            *t = MakeTime(yr-20);
        EndDisabledControls(yr <= min_yr);
        ImGui::SameLine();
        BeginDisabledControls(yr + 20 >= max_yr);
        if (ImGui::ArrowButtonEx("##Down",ImGuiDir_Down,cell_size))
            *t = MakeTime(yr+20);
        EndDisabledControls(yr+ 20 >= max_yr);
        // ImGui::Dummy(cell_size);
        cell_size.x *= 7.0f/4.0f;
        cell_size.y *= 7.0f/5.0f;
        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 4; ++j) {
                const bool t1_or_t2 = (t1 != NULL && t1_yr == yr) || (t2 != NULL && t2_yr == yr);
                if (t1_or_t2)
                    ImGui::PushStyleColor(ImGuiCol_Button, col_btn);
                ImFormatString(buff,32,"%d",yr);
                if (yr<1970||yr>3000) {
                    ImGui::Dummy(cell_size);
                }
                else if (ImGui::Button(buff,cell_size)) {
                    *t = MakeTime(yr);
                    *level = 1;
                }
                if (t1_or_t2)
                    ImGui::PopStyleColor();
                if (j != 3)
                    ImGui::SameLine();
                yr++;
            }
        }
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    ImGui::PopID();
    return clk;
}

bool ShowTimePicker(const char* id, ImPlotTime* t) {
    ImGui::PushID(id);
    tm& Tm = GImPlot->Tm;
    GetTime(*t,&Tm);

    static const char* nums[] = { "00","01","02","03","04","05","06","07","08","09",
                                  "10","11","12","13","14","15","16","17","18","19",
                                  "20","21","22","23","24","25","26","27","28","29",
                                  "30","31","32","33","34","35","36","37","38","39",
                                  "40","41","42","43","44","45","46","47","48","49",
                                  "50","51","52","53","54","55","56","57","58","59"};

    static const char* am_pm[] = {"am","pm"};

    bool hour24 = GImPlot->Style.Use24HourClock;

    int hr  = hour24 ? Tm.tm_hour : ((Tm.tm_hour == 0 || Tm.tm_hour == 12) ? 12 : Tm.tm_hour % 12);
    int min = Tm.tm_min;
    int sec = Tm.tm_sec;
    int ap  = Tm.tm_hour < 12 ? 0 : 1;

    bool changed = false;

    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    spacing.x = 0;
    float width    = ImGui::CalcTextSize("888").x;
    float height   = ImGui::GetFrameHeight();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, spacing);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize,2.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));

    ImGui::SetNextItemWidth(width);
    if (ImGui::BeginCombo("##hr",nums[hr],ImGuiComboFlags_NoArrowButton)) {
        const int ia = hour24 ? 0 : 1;
        const int ib = hour24 ? 24 : 13;
        for (int i = ia; i < ib; ++i) {
            if (ImGui::Selectable(nums[i],i==hr)) {
                hr = i;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::Text(":");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    if (ImGui::BeginCombo("##min",nums[min],ImGuiComboFlags_NoArrowButton)) {
        for (int i = 0; i < 60; ++i) {
            if (ImGui::Selectable(nums[i],i==min)) {
                min = i;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::Text(":");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    if (ImGui::BeginCombo("##sec",nums[sec],ImGuiComboFlags_NoArrowButton)) {
        for (int i = 0; i < 60; ++i) {
            if (ImGui::Selectable(nums[i],i==sec)) {
                sec = i;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    if (!hour24) {
        ImGui::SameLine();
        if (ImGui::Button(am_pm[ap],ImVec2(0,height))) {
            ap = 1 - ap;
            changed = true;
        }
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
    ImGui::PopID();

    if (changed) {
        if (!hour24)
            hr = hr % 12 + ap * 12;
        Tm.tm_hour = hr;
        Tm.tm_min  = min;
        Tm.tm_sec  = sec;
        *t = MkTime(&Tm);
    }

    return changed;
}

void StyleColorsAuto(ImPlotStyle* dst) {
    ImPlotStyle* style              = dst ? dst : &ImPlot::GetStyle();
    ImVec4* colors                  = style->Colors;

    style->MinorAlpha               = 0.25f;

    colors[ImPlotCol_Line]          = IMPLOT_AUTO_COL;
    colors[ImPlotCol_Fill]          = IMPLOT_AUTO_COL;
    colors[ImPlotCol_MarkerOutline] = IMPLOT_AUTO_COL;
    colors[ImPlotCol_MarkerFill]    = IMPLOT_AUTO_COL;
    colors[ImPlotCol_ErrorBar]      = IMPLOT_AUTO_COL;
    colors[ImPlotCol_FrameBg]       = IMPLOT_AUTO_COL;
    colors[ImPlotCol_PlotBg]        = IMPLOT_AUTO_COL;
    colors[ImPlotCol_PlotBorder]    = IMPLOT_AUTO_COL;
    colors[ImPlotCol_LegendBg]      = IMPLOT_AUTO_COL;
    colors[ImPlotCol_LegendBorder]  = IMPLOT_AUTO_COL;
    colors[ImPlotCol_LegendText]    = IMPLOT_AUTO_COL;
    colors[ImPlotCol_TitleText]     = IMPLOT_AUTO_COL;
    colors[ImPlotCol_InlayText]     = IMPLOT_AUTO_COL;
    colors[ImPlotCol_PlotBorder]    = IMPLOT_AUTO_COL;
    colors[ImPlotCol_AxisText]      = IMPLOT_AUTO_COL;
    colors[ImPlotCol_AxisGrid]      = IMPLOT_AUTO_COL;
    colors[ImPlotCol_AxisTick]      = IMPLOT_AUTO_COL;
    colors[ImPlotCol_AxisBg]        = IMPLOT_AUTO_COL;
    colors[ImPlotCol_AxisBgHovered] = IMPLOT_AUTO_COL;
    colors[ImPlotCol_AxisBgActive]  = IMPLOT_AUTO_COL;
    colors[ImPlotCol_Selection]     = IMPLOT_AUTO_COL;
    colors[ImPlotCol_Crosshairs]    = IMPLOT_AUTO_COL;
}

void StyleColorsClassic(ImPlotStyle* dst) {
    ImPlotStyle* style              = dst ? dst : &ImPlot::GetStyle();
    ImVec4* colors                  = style->Colors;

    style->MinorAlpha               = 0.5f;

    colors[ImPlotCol_Line]          = IMPLOT_AUTO_COL;
    colors[ImPlotCol_Fill]          = IMPLOT_AUTO_COL;
    colors[ImPlotCol_MarkerOutline] = IMPLOT_AUTO_COL;
    colors[ImPlotCol_MarkerFill]    = IMPLOT_AUTO_COL;
    colors[ImPlotCol_ErrorBar]      = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImPlotCol_FrameBg]       = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
    colors[ImPlotCol_PlotBg]        = ImVec4(0.00f, 0.00f, 0.00f, 0.35f);
    colors[ImPlotCol_PlotBorder]    = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImPlotCol_LegendBg]      = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
    colors[ImPlotCol_LegendBorder]  = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImPlotCol_LegendText]    = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImPlotCol_TitleText]     = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImPlotCol_InlayText]     = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImPlotCol_AxisText]      = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImPlotCol_AxisGrid]      = ImVec4(0.90f, 0.90f, 0.90f, 0.25f);
    colors[ImPlotCol_AxisTick]      = IMPLOT_AUTO_COL; // TODO
    colors[ImPlotCol_AxisBg]        = IMPLOT_AUTO_COL; // TODO
    colors[ImPlotCol_AxisBgHovered] = IMPLOT_AUTO_COL; // TODO
    colors[ImPlotCol_AxisBgActive]  = IMPLOT_AUTO_COL; // TODO
    colors[ImPlotCol_Selection]     = ImVec4(0.97f, 0.97f, 0.39f, 1.00f);
    colors[ImPlotCol_Crosshairs]    = ImVec4(0.50f, 0.50f, 0.50f, 0.75f);
}

void StyleColorsDark(ImPlotStyle* dst) {
    ImPlotStyle* style              = dst ? dst : &ImPlot::GetStyle();
    ImVec4* colors                  = style->Colors;

    style->MinorAlpha               = 0.25f;

    colors[ImPlotCol_Line]          = IMPLOT_AUTO_COL;
    colors[ImPlotCol_Fill]          = IMPLOT_AUTO_COL;
    colors[ImPlotCol_MarkerOutline] = IMPLOT_AUTO_COL;
    colors[ImPlotCol_MarkerFill]    = IMPLOT_AUTO_COL;
    colors[ImPlotCol_ErrorBar]      = IMPLOT_AUTO_COL;
    colors[ImPlotCol_FrameBg]       = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    colors[ImPlotCol_PlotBg]        = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
    colors[ImPlotCol_PlotBorder]    = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImPlotCol_LegendBg]      = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImPlotCol_LegendBorder]  = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImPlotCol_LegendText]    = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImPlotCol_TitleText]     = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImPlotCol_InlayText]     = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImPlotCol_AxisText]      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImPlotCol_AxisGrid]      = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
    colors[ImPlotCol_AxisTick]      = IMPLOT_AUTO_COL; // TODO
    colors[ImPlotCol_AxisBg]        = IMPLOT_AUTO_COL; // TODO
    colors[ImPlotCol_AxisBgHovered] = IMPLOT_AUTO_COL; // TODO
    colors[ImPlotCol_AxisBgActive]  = IMPLOT_AUTO_COL; // TODO
    colors[ImPlotCol_Selection]     = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImPlotCol_Crosshairs]    = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
}

void StyleColorsLight(ImPlotStyle* dst) {
    ImPlotStyle* style              = dst ? dst : &ImPlot::GetStyle();
    ImVec4* colors                  = style->Colors;

    style->MinorAlpha               = 1.0f;

    colors[ImPlotCol_Line]          = IMPLOT_AUTO_COL;
    colors[ImPlotCol_Fill]          = IMPLOT_AUTO_COL;
    colors[ImPlotCol_MarkerOutline] = IMPLOT_AUTO_COL;
    colors[ImPlotCol_MarkerFill]    = IMPLOT_AUTO_COL;
    colors[ImPlotCol_ErrorBar]      = IMPLOT_AUTO_COL;
    colors[ImPlotCol_FrameBg]       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImPlotCol_PlotBg]        = ImVec4(0.42f, 0.57f, 1.00f, 0.13f);
    colors[ImPlotCol_PlotBorder]    = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImPlotCol_LegendBg]      = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImPlotCol_LegendBorder]  = ImVec4(0.82f, 0.82f, 0.82f, 0.80f);
    colors[ImPlotCol_LegendText]    = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_TitleText]     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_InlayText]     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_AxisText]      = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_AxisGrid]      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImPlotCol_AxisTick]      = ImVec4(0.00f, 0.00f, 0.00f, 0.25f);
    colors[ImPlotCol_AxisBg]        = IMPLOT_AUTO_COL; // TODO
    colors[ImPlotCol_AxisBgHovered] = IMPLOT_AUTO_COL; // TODO
    colors[ImPlotCol_AxisBgActive]  = IMPLOT_AUTO_COL; // TODO
    colors[ImPlotCol_Selection]     = ImVec4(0.82f, 0.64f, 0.03f, 1.00f);
    colors[ImPlotCol_Crosshairs]    = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
}

//-----------------------------------------------------------------------------
// [SECTION] Obsolete Functions/Types
//-----------------------------------------------------------------------------

#ifndef IMPLOT_DISABLE_OBSOLETE_FUNCTIONS

bool BeginPlot(const char* title, const char* x_label, const char* y1_label, const ImVec2& size,
               ImPlotFlags flags, ImPlotAxisFlags x_flags, ImPlotAxisFlags y1_flags, ImPlotAxisFlags y2_flags, ImPlotAxisFlags y3_flags,
               const char* y2_label, const char* y3_label)
{
    if (!BeginPlot(title, size, flags))
        return false;
    SetupAxis(ImAxis_X1, x_label, x_flags);
    SetupAxis(ImAxis_Y1, y1_label, y1_flags);
    if (ImHasFlag(flags, ImPlotFlags_YAxis2))
        SetupAxis(ImAxis_Y2, y2_label, y2_flags);
    if (ImHasFlag(flags, ImPlotFlags_YAxis3))
        SetupAxis(ImAxis_Y3, y3_label, y3_flags);
    return true;
}

#endif

}  // namespace ImPlot
