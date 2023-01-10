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

// Table of Contents:
//
// [SECTION] Macros and Defines
// [SECTION] Enums and Types
// [SECTION] Callbacks
// [SECTION] Contexts
// [SECTION] Begin/End Plot
// [SECTION] Begin/End Subplot
// [SECTION] Setup
// [SECTION] SetNext
// [SECTION] Plot Items
// [SECTION] Plot Tools
// [SECTION] Plot Utils
// [SECTION] Legend Utils
// [SECTION] Drag and Drop
// [SECTION] Styling
// [SECTION] Colormaps
// [SECTION] Input Mapping
// [SECTION] Miscellaneous
// [SECTION] Demo
// [SECTION] Obsolete API

#pragma once
#include "imgui.h"

//-----------------------------------------------------------------------------
// [SECTION] Macros and Defines
//-----------------------------------------------------------------------------

// Define attributes of all API symbols declarations (e.g. for DLL under Windows)
// Using ImPlot via a shared library is not recommended, because we don't guarantee
// backward nor forward ABI compatibility and also function call overhead. If you
// do use ImPlot as a DLL, be sure to call SetImGuiContext (see Miscellanous section).
#ifndef IMPLOT_API
#define IMPLOT_API
#endif

// ImPlot version string.
#define IMPLOT_VERSION "0.14"
// Indicates variable should deduced automatically.
#define IMPLOT_AUTO -1
// Special color used to indicate that a color should be deduced automatically.
#define IMPLOT_AUTO_COL ImVec4(0,0,0,-1)
// Macro for templated plotting functions; keeps header clean.
#define IMPLOT_TMP template <typename T> IMPLOT_API

//-----------------------------------------------------------------------------
// [SECTION] Enums and Types
//-----------------------------------------------------------------------------

// Forward declarations
struct ImPlotContext;             // ImPlot context (opaque struct, see implot_internal.h)

// Enums/Flags
typedef int ImAxis;                   // -> enum ImAxis_
typedef int ImPlotFlags;              // -> enum ImPlotFlags_
typedef int ImPlotAxisFlags;          // -> enum ImPlotAxisFlags_
typedef int ImPlotSubplotFlags;       // -> enum ImPlotSubplotFlags_
typedef int ImPlotLegendFlags;        // -> enum ImPlotLegendFlags_
typedef int ImPlotMouseTextFlags;     // -> enum ImPlotMouseTextFlags_
typedef int ImPlotDragToolFlags;      // -> ImPlotDragToolFlags_
typedef int ImPlotColormapScaleFlags; // -> ImPlotColormapScaleFlags_

typedef int ImPlotItemFlags;          // -> ImPlotItemFlags_
typedef int ImPlotLineFlags;          // -> ImPlotLineFlags_
typedef int ImPlotScatterFlags;       // -> ImPlotScatterFlags
typedef int ImPlotStairsFlags;        // -> ImPlotStairsFlags_
typedef int ImPlotShadedFlags;        // -> ImPlotShadedFlags_
typedef int ImPlotBarsFlags;          // -> ImPlotBarsFlags_
typedef int ImPlotBarGroupsFlags;     // -> ImPlotBarGroupsFlags_
typedef int ImPlotErrorBarsFlags;     // -> ImPlotErrorBarsFlags_
typedef int ImPlotStemsFlags;         // -> ImPlotStemsFlags_
typedef int ImPlotInfLinesFlags;      // -> ImPlotInfLinesFlags_
typedef int ImPlotPieChartFlags;      // -> ImPlotPieChartFlags_
typedef int ImPlotHeatmapFlags;       // -> ImPlotHeatmapFlags_
typedef int ImPlotHistogramFlags;     // -> ImPlotHistogramFlags_
typedef int ImPlotDigitalFlags;       // -> ImPlotDigitalFlags_
typedef int ImPlotImageFlags;         // -> ImPlotImageFlags_
typedef int ImPlotTextFlags;          // -> ImPlotTextFlags_
typedef int ImPlotDummyFlags;         // -> ImPlotDummyFlags_

typedef int ImPlotCond;               // -> enum ImPlotCond_
typedef int ImPlotCol;                // -> enum ImPlotCol_
typedef int ImPlotStyleVar;           // -> enum ImPlotStyleVar_
typedef int ImPlotScale;              // -> enum ImPlotScale_
typedef int ImPlotMarker;             // -> enum ImPlotMarker_
typedef int ImPlotColormap;           // -> enum ImPlotColormap_
typedef int ImPlotLocation;           // -> enum ImPlotLocation_
typedef int ImPlotBin;                // -> enum ImPlotBin_

// Axis indices. The values assigned may change; NEVER hardcode these.
enum ImAxis_ {
    // horizontal axes
    ImAxis_X1 = 0, // enabled by default
    ImAxis_X2,     // disabled by default
    ImAxis_X3,     // disabled by default
    // vertical axes
    ImAxis_Y1,     // enabled by default
    ImAxis_Y2,     // disabled by default
    ImAxis_Y3,     // disabled by default
    // bookeeping
    ImAxis_COUNT
};

// Options for plots (see BeginPlot).
enum ImPlotFlags_ {
    ImPlotFlags_None          = 0,       // default
    ImPlotFlags_NoTitle       = 1 << 0,  // the plot title will not be displayed (titles are also hidden if preceeded by double hashes, e.g. "##MyPlot")
    ImPlotFlags_NoLegend      = 1 << 1,  // the legend will not be displayed
    ImPlotFlags_NoMouseText   = 1 << 2,  // the mouse position, in plot coordinates, will not be displayed inside of the plot
    ImPlotFlags_NoInputs      = 1 << 3,  // the user will not be able to interact with the plot
    ImPlotFlags_NoMenus       = 1 << 4,  // the user will not be able to open context menus
    ImPlotFlags_NoBoxSelect   = 1 << 5,  // the user will not be able to box-select
    ImPlotFlags_NoChild       = 1 << 6,  // a child window region will not be used to capture mouse scroll (can boost performance for single ImGui window applications)
    ImPlotFlags_NoFrame       = 1 << 7,  // the ImGui frame will not be rendered
    ImPlotFlags_Equal         = 1 << 8,  // x and y axes pairs will be constrained to have the same units/pixel
    ImPlotFlags_Crosshairs    = 1 << 9,  // the default mouse cursor will be replaced with a crosshair when hovered
    ImPlotFlags_CanvasOnly    = ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText
};

// Options for plot axes (see SetupAxis).
enum ImPlotAxisFlags_ {
    ImPlotAxisFlags_None          = 0,       // default
    ImPlotAxisFlags_NoLabel       = 1 << 0,  // the axis label will not be displayed (axis labels are also hidden if the supplied string name is NULL)
    ImPlotAxisFlags_NoGridLines   = 1 << 1,  // no grid lines will be displayed
    ImPlotAxisFlags_NoTickMarks   = 1 << 2,  // no tick marks will be displayed
    ImPlotAxisFlags_NoTickLabels  = 1 << 3,  // no text labels will be displayed
    ImPlotAxisFlags_NoInitialFit  = 1 << 4,  // axis will not be initially fit to data extents on the first rendered frame
    ImPlotAxisFlags_NoMenus       = 1 << 5,  // the user will not be able to open context menus with right-click
    ImPlotAxisFlags_NoSideSwitch  = 1 << 6,  // the user will not be able to switch the axis side by dragging it
    ImPlotAxisFlags_NoHighlight   = 1 << 7,  // the axis will not have its background highlighted when hovered or held
    ImPlotAxisFlags_Opposite      = 1 << 8,  // axis ticks and labels will be rendered on the conventionally opposite side (i.e, right or top)
    ImPlotAxisFlags_Foreground    = 1 << 9,  // grid lines will be displayed in the foreground (i.e. on top of data) instead of the background
    ImPlotAxisFlags_Invert        = 1 << 10, // the axis will be inverted
    ImPlotAxisFlags_AutoFit       = 1 << 11, // axis will be auto-fitting to data extents
    ImPlotAxisFlags_RangeFit      = 1 << 12, // axis will only fit points if the point is in the visible range of the **orthogonal** axis
    ImPlotAxisFlags_PanStretch    = 1 << 13, // panning in a locked or constrained state will cause the axis to stretch if possible
    ImPlotAxisFlags_LockMin       = 1 << 14, // the axis minimum value will be locked when panning/zooming
    ImPlotAxisFlags_LockMax       = 1 << 15, // the axis maximum value will be locked when panning/zooming
    ImPlotAxisFlags_Lock          = ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax,
    ImPlotAxisFlags_NoDecorations = ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels,
    ImPlotAxisFlags_AuxDefault    = ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_Opposite
};

// Options for subplots (see BeginSubplot)
enum ImPlotSubplotFlags_ {
    ImPlotSubplotFlags_None        = 0,       // default
    ImPlotSubplotFlags_NoTitle     = 1 << 0,  // the subplot title will not be displayed (titles are also hidden if preceeded by double hashes, e.g. "##MySubplot")
    ImPlotSubplotFlags_NoLegend    = 1 << 1,  // the legend will not be displayed (only applicable if ImPlotSubplotFlags_ShareItems is enabled)
    ImPlotSubplotFlags_NoMenus     = 1 << 2,  // the user will not be able to open context menus with right-click
    ImPlotSubplotFlags_NoResize    = 1 << 3,  // resize splitters between subplot cells will be not be provided
    ImPlotSubplotFlags_NoAlign     = 1 << 4,  // subplot edges will not be aligned vertically or horizontally
    ImPlotSubplotFlags_ShareItems  = 1 << 5,  // items across all subplots will be shared and rendered into a single legend entry
    ImPlotSubplotFlags_LinkRows    = 1 << 6,  // link the y-axis limits of all plots in each row (does not apply to auxiliary axes)
    ImPlotSubplotFlags_LinkCols    = 1 << 7,  // link the x-axis limits of all plots in each column (does not apply to auxiliary axes)
    ImPlotSubplotFlags_LinkAllX    = 1 << 8,  // link the x-axis limits in every plot in the subplot (does not apply to auxiliary axes)
    ImPlotSubplotFlags_LinkAllY    = 1 << 9,  // link the y-axis limits in every plot in the subplot (does not apply to auxiliary axes)
    ImPlotSubplotFlags_ColMajor    = 1 << 10  // subplots are added in column major order instead of the default row major order
};

// Options for legends (see SetupLegend)
enum ImPlotLegendFlags_ {
    ImPlotLegendFlags_None            = 0,      // default
    ImPlotLegendFlags_NoButtons       = 1 << 0, // legend icons will not function as hide/show buttons
    ImPlotLegendFlags_NoHighlightItem = 1 << 1, // plot items will not be highlighted when their legend entry is hovered
    ImPlotLegendFlags_NoHighlightAxis = 1 << 2, // axes will not be highlighted when legend entries are hovered (only relevant if x/y-axis count > 1)
    ImPlotLegendFlags_NoMenus         = 1 << 3, // the user will not be able to open context menus with right-click
    ImPlotLegendFlags_Outside         = 1 << 4, // legend will be rendered outside of the plot area
    ImPlotLegendFlags_Horizontal      = 1 << 5, // legend entries will be displayed horizontally
    ImPlotLegendFlags_Sort            = 1 << 6, // legend entries will be displayed in alphabetical order
};

// Options for mouse hover text (see SetupMouseText)
enum ImPlotMouseTextFlags_ {
    ImPlotMouseTextFlags_None        = 0,      // default
    ImPlotMouseTextFlags_NoAuxAxes   = 1 << 0, // only show the mouse position for primary axes
    ImPlotMouseTextFlags_NoFormat    = 1 << 1, // axes label formatters won't be used to render text
    ImPlotMouseTextFlags_ShowAlways  = 1 << 2, // always display mouse position even if plot not hovered
};

// Options for DragPoint, DragLine, DragRect
enum ImPlotDragToolFlags_ {
    ImPlotDragToolFlags_None      = 0,      // default
    ImPlotDragToolFlags_NoCursors = 1 << 0, // drag tools won't change cursor icons when hovered or held
    ImPlotDragToolFlags_NoFit     = 1 << 1, // the drag tool won't be considered for plot fits
    ImPlotDragToolFlags_NoInputs  = 1 << 2, // lock the tool from user inputs
    ImPlotDragToolFlags_Delayed   = 1 << 3, // tool rendering will be delayed one frame; useful when applying position-constraints
};

// Flags for ColormapScale
enum ImPlotColormapScaleFlags_ {
    ImPlotColormapScaleFlags_None     = 0,      // default
    ImPlotColormapScaleFlags_NoLabel  = 1 << 0, // the colormap axis label will not be displayed
    ImPlotColormapScaleFlags_Opposite = 1 << 1, // render the colormap label and tick labels on the opposite side
    ImPlotColormapScaleFlags_Invert   = 1 << 2, // invert the colormap bar and axis scale (this only affects rendering; if you only want to reverse the scale mapping, make scale_min > scale_max)
};

// Flags for ANY PlotX function
enum ImPlotItemFlags_ {
    ImPlotItemFlags_None     = 0,
    ImPlotItemFlags_NoLegend = 1 << 0, // the item won't have a legend entry displayed
    ImPlotItemFlags_NoFit    = 1 << 1, // the item won't be considered for plot fits
};

// Flags for PlotLine
enum ImPlotLineFlags_ {
    ImPlotLineFlags_None        = 0,       // default
    ImPlotLineFlags_Segments    = 1 << 10, // a line segment will be rendered from every two consecutive points
    ImPlotLineFlags_Loop        = 1 << 11, // the last and first point will be connected to form a closed loop
    ImPlotLineFlags_SkipNaN     = 1 << 12, // NaNs values will be skipped instead of rendered as missing data
    ImPlotLineFlags_NoClip      = 1 << 13, // markers (if displayed) on the edge of a plot will not be clipped
    ImPlotLineFlags_Shaded      = 1 << 14, // a filled region between the line and horizontal origin will be rendered; use PlotShaded for more advanced cases
};

// Flags for PlotScatter
enum ImPlotScatterFlags_ {
    ImPlotScatterFlags_None   = 0,       // default
    ImPlotScatterFlags_NoClip = 1 << 10, // markers on the edge of a plot will not be clipped
};

// Flags for PlotStairs
enum ImPlotStairsFlags_ {
    ImPlotStairsFlags_None     = 0,       // default
    ImPlotStairsFlags_PreStep  = 1 << 10, // the y value is continued constantly to the left from every x position, i.e. the interval (x[i-1], x[i]] has the value y[i]
    ImPlotStairsFlags_Shaded   = 1 << 11  // a filled region between the stairs and horizontal origin will be rendered; use PlotShaded for more advanced cases
};

// Flags for PlotShaded (placeholder)
enum ImPlotShadedFlags_ {
    ImPlotShadedFlags_None = 0, // default
    ImPlotShadedFlags_Vertical = 1 << 10, // Self-implemented, help plot shaded line on the y-axis
};

// Flags for PlotBars
enum ImPlotBarsFlags_ {
    ImPlotBarsFlags_None         = 0,       // default
    ImPlotBarsFlags_Horizontal   = 1 << 10, // bars will be rendered horizontally on the current y-axis
};

// Flags for PlotBarGroups
enum ImPlotBarGroupsFlags_ {
    ImPlotBarGroupsFlags_None        = 0,       // default
    ImPlotBarGroupsFlags_Horizontal  = 1 << 10, // bar groups will be rendered horizontally on the current y-axis
    ImPlotBarGroupsFlags_Stacked     = 1 << 11, // items in a group will be stacked on top of each other
};

// Flags for PlotErrorBars
enum ImPlotErrorBarsFlags_ {
    ImPlotErrorBarsFlags_None       = 0,       // default
    ImPlotErrorBarsFlags_Horizontal = 1 << 10, // error bars will be rendered horizontally on the current y-axis
};

// Flags for PlotStems
enum ImPlotStemsFlags_ {
    ImPlotStemsFlags_None       = 0,       // default
    ImPlotStemsFlags_Horizontal = 1 << 10, // stems will be rendered horizontally on the current y-axis
};

// Flags for PlotInfLines
enum ImPlotInfLinesFlags_ {
    ImPlotInfLinesFlags_None       = 0,      // default
    ImPlotInfLinesFlags_Horizontal = 1 << 10 // lines will be rendered horizontally on the current y-axis
};

// Flags for PlotPieChart
enum ImPlotPieChartFlags_ {
    ImPlotPieChartFlags_None      = 0,      // default
    ImPlotPieChartFlags_Normalize = 1 << 10 // force normalization of pie chart values (i.e. always make a full circle if sum < 0)
};

// Flags for PlotHeatmap
enum ImPlotHeatmapFlags_ {
    ImPlotHeatmapFlags_None     = 0,       // default
    ImPlotHeatmapFlags_ColMajor = 1 << 10, // data will be read in column major order
};

// Flags for PlotHistogram and PlotHistogram2D
enum ImPlotHistogramFlags_ {
    ImPlotHistogramFlags_None       = 0,       // default
    ImPlotHistogramFlags_Horizontal = 1 << 10, // histogram bars will be rendered horizontally (not supported by PlotHistogram2D)
    ImPlotHistogramFlags_Cumulative = 1 << 11, // each bin will contain its count plus the counts of all previous bins (not supported by PlotHistogram2D)
    ImPlotHistogramFlags_Density    = 1 << 12, // counts will be normalized, i.e. the PDF will be visualized, or the CDF will be visualized if Cumulative is also set
    ImPlotHistogramFlags_NoOutliers = 1 << 13, // exclude values outside the specifed histogram range from the count toward normalizing and cumulative counts
    ImPlotHistogramFlags_ColMajor   = 1 << 14  // data will be read in column major order (not supported by PlotHistogram)
};

// Flags for PlotDigital (placeholder)
enum ImPlotDigitalFlags_ {
    ImPlotDigitalFlags_None = 0 // default
};

// Flags for PlotImage (placeholder)
enum ImPlotImageFlags_ {
    ImPlotImageFlags_None = 0 // default
};

// Flags for PlotText
enum ImPlotTextFlags_ {
    ImPlotTextFlags_None     = 0,       // default
    ImPlotTextFlags_Vertical = 1 << 10  // text will be rendered vertically
};

// Flags for PlotDummy (placeholder)
enum ImPlotDummyFlags_ {
    ImPlotDummyFlags_None = 0 // default
};

// Represents a condition for SetupAxisLimits etc. (same as ImGuiCond, but we only support a subset of those enums)
enum ImPlotCond_
{
    ImPlotCond_None   = ImGuiCond_None,    // No condition (always set the variable), same as _Always
    ImPlotCond_Always = ImGuiCond_Always,  // No condition (always set the variable)
    ImPlotCond_Once   = ImGuiCond_Once,    // Set the variable once per runtime session (only the first call will succeed)
};

// Plot styling colors.
enum ImPlotCol_ {
    // item styling colors
    ImPlotCol_Line,          // plot line/outline color (defaults to next unused color in current colormap)
    ImPlotCol_Fill,          // plot fill color for bars (defaults to the current line color)
    ImPlotCol_MarkerOutline, // marker outline color (defaults to the current line color)
    ImPlotCol_MarkerFill,    // marker fill color (defaults to the current line color)
    ImPlotCol_ErrorBar,      // error bar color (defaults to ImGuiCol_Text)
    // plot styling colors
    ImPlotCol_FrameBg,       // plot frame background color (defaults to ImGuiCol_FrameBg)
    ImPlotCol_PlotBg,        // plot area background color (defaults to ImGuiCol_WindowBg)
    ImPlotCol_PlotBorder,    // plot area border color (defaults to ImGuiCol_Border)
    ImPlotCol_LegendBg,      // legend background color (defaults to ImGuiCol_PopupBg)
    ImPlotCol_LegendBorder,  // legend border color (defaults to ImPlotCol_PlotBorder)
    ImPlotCol_LegendText,    // legend text color (defaults to ImPlotCol_InlayText)
    ImPlotCol_TitleText,     // plot title text color (defaults to ImGuiCol_Text)
    ImPlotCol_InlayText,     // color of text appearing inside of plots (defaults to ImGuiCol_Text)
    ImPlotCol_AxisText,      // axis label and tick lables color (defaults to ImGuiCol_Text)
    ImPlotCol_AxisGrid,      // axis grid color (defaults to 25% ImPlotCol_AxisText)
    ImPlotCol_AxisTick,      // axis tick color (defaults to AxisGrid)
    ImPlotCol_AxisBg,        // background color of axis hover region (defaults to transparent)
    ImPlotCol_AxisBgHovered, // axis hover color (defaults to ImGuiCol_ButtonHovered)
    ImPlotCol_AxisBgActive,  // axis active color (defaults to ImGuiCol_ButtonActive)
    ImPlotCol_Selection,     // box-selection color (defaults to yellow)
    ImPlotCol_Crosshairs,    // crosshairs color (defaults to ImPlotCol_PlotBorder)
    ImPlotCol_COUNT
};

// Plot styling variables.
enum ImPlotStyleVar_ {
    // item styling variables
    ImPlotStyleVar_LineWeight,         // float,  plot item line weight in pixels
    ImPlotStyleVar_Marker,             // int,    marker specification
    ImPlotStyleVar_MarkerSize,         // float,  marker size in pixels (roughly the marker's "radius")
    ImPlotStyleVar_MarkerWeight,       // float,  plot outline weight of markers in pixels
    ImPlotStyleVar_FillAlpha,          // float,  alpha modifier applied to all plot item fills
    ImPlotStyleVar_ErrorBarSize,       // float,  error bar whisker width in pixels
    ImPlotStyleVar_ErrorBarWeight,     // float,  error bar whisker weight in pixels
    ImPlotStyleVar_DigitalBitHeight,   // float,  digital channels bit height (at 1) in pixels
    ImPlotStyleVar_DigitalBitGap,      // float,  digital channels bit padding gap in pixels
    // plot styling variables
    ImPlotStyleVar_PlotBorderSize,     // float,  thickness of border around plot area
    ImPlotStyleVar_MinorAlpha,         // float,  alpha multiplier applied to minor axis grid lines
    ImPlotStyleVar_MajorTickLen,       // ImVec2, major tick lengths for X and Y axes
    ImPlotStyleVar_MinorTickLen,       // ImVec2, minor tick lengths for X and Y axes
    ImPlotStyleVar_MajorTickSize,      // ImVec2, line thickness of major ticks
    ImPlotStyleVar_MinorTickSize,      // ImVec2, line thickness of minor ticks
    ImPlotStyleVar_MajorGridSize,      // ImVec2, line thickness of major grid lines
    ImPlotStyleVar_MinorGridSize,      // ImVec2, line thickness of minor grid lines
    ImPlotStyleVar_PlotPadding,        // ImVec2, padding between widget frame and plot area, labels, or outside legends (i.e. main padding)
    ImPlotStyleVar_LabelPadding,       // ImVec2, padding between axes labels, tick labels, and plot edge
    ImPlotStyleVar_LegendPadding,      // ImVec2, legend padding from plot edges
    ImPlotStyleVar_LegendInnerPadding, // ImVec2, legend inner padding from legend edges
    ImPlotStyleVar_LegendSpacing,      // ImVec2, spacing between legend entries
    ImPlotStyleVar_MousePosPadding,    // ImVec2, padding between plot edge and interior info text
    ImPlotStyleVar_AnnotationPadding,  // ImVec2, text padding around annotation labels
    ImPlotStyleVar_FitPadding,         // ImVec2, additional fit padding as a percentage of the fit extents (e.g. ImVec2(0.1f,0.1f) adds 10% to the fit extents of X and Y)
    ImPlotStyleVar_PlotDefaultSize,    // ImVec2, default size used when ImVec2(0,0) is passed to BeginPlot
    ImPlotStyleVar_PlotMinSize,        // ImVec2, minimum size plot frame can be when shrunk
    ImPlotStyleVar_COUNT
};

// Axis scale
enum ImPlotScale_ {
    ImPlotScale_Linear = 0, // default linear scale
    ImPlotScale_Time,       // date/time scale
    ImPlotScale_Log10,      // base 10 logartithmic scale
    ImPlotScale_SymLog,     // symmetric log scale
};

// Marker specifications.
enum ImPlotMarker_ {
    ImPlotMarker_None = -1, // no marker
    ImPlotMarker_Circle,    // a circle marker (default)
    ImPlotMarker_Square,    // a square maker
    ImPlotMarker_Diamond,   // a diamond marker
    ImPlotMarker_Up,        // an upward-pointing triangle marker
    ImPlotMarker_Down,      // an downward-pointing triangle marker
    ImPlotMarker_Left,      // an leftward-pointing triangle marker
    ImPlotMarker_Right,     // an rightward-pointing triangle marker
    ImPlotMarker_Cross,     // a cross marker (not fillable)
    ImPlotMarker_Plus,      // a plus marker (not fillable)
    ImPlotMarker_Asterisk,  // a asterisk marker (not fillable)
    ImPlotMarker_COUNT
};

// Built-in colormaps
enum ImPlotColormap_ {
    ImPlotColormap_Deep     = 0,   // a.k.a. seaborn deep             (qual=true,  n=10) (default)
    ImPlotColormap_Dark     = 1,   // a.k.a. matplotlib "Set1"        (qual=true,  n=9 )
    ImPlotColormap_Pastel   = 2,   // a.k.a. matplotlib "Pastel1"     (qual=true,  n=9 )
    ImPlotColormap_Paired   = 3,   // a.k.a. matplotlib "Paired"      (qual=true,  n=12)
    ImPlotColormap_Viridis  = 4,   // a.k.a. matplotlib "viridis"     (qual=false, n=11)
    ImPlotColormap_Plasma   = 5,   // a.k.a. matplotlib "plasma"      (qual=false, n=11)
    ImPlotColormap_Hot      = 6,   // a.k.a. matplotlib/MATLAB "hot"  (qual=false, n=11)
    ImPlotColormap_Cool     = 7,   // a.k.a. matplotlib/MATLAB "cool" (qual=false, n=11)
    ImPlotColormap_Pink     = 8,   // a.k.a. matplotlib/MATLAB "pink" (qual=false, n=11)
    ImPlotColormap_Jet      = 9,   // a.k.a. MATLAB "jet"             (qual=false, n=11)
    ImPlotColormap_Twilight = 10,  // a.k.a. matplotlib "twilight"    (qual=false, n=11)
    ImPlotColormap_RdBu     = 11,  // red/blue, Color Brewer          (qual=false, n=11)
    ImPlotColormap_BrBG     = 12,  // brown/blue-green, Color Brewer  (qual=false, n=11)
    ImPlotColormap_PiYG     = 13,  // pink/yellow-green, Color Brewer (qual=false, n=11)
    ImPlotColormap_Spectral = 14,  // color spectrum, Color Brewer    (qual=false, n=11)
    ImPlotColormap_Greys    = 15,  // white/black                     (qual=false, n=2 )
};

// Used to position items on a plot (e.g. legends, labels, etc.)
enum ImPlotLocation_ {
    ImPlotLocation_Center    = 0,                                          // center-center
    ImPlotLocation_North     = 1 << 0,                                     // top-center
    ImPlotLocation_South     = 1 << 1,                                     // bottom-center
    ImPlotLocation_West      = 1 << 2,                                     // center-left
    ImPlotLocation_East      = 1 << 3,                                     // center-right
    ImPlotLocation_NorthWest = ImPlotLocation_North | ImPlotLocation_West, // top-left
    ImPlotLocation_NorthEast = ImPlotLocation_North | ImPlotLocation_East, // top-right
    ImPlotLocation_SouthWest = ImPlotLocation_South | ImPlotLocation_West, // bottom-left
    ImPlotLocation_SouthEast = ImPlotLocation_South | ImPlotLocation_East  // bottom-right
};

// Enums for different automatic histogram binning methods (k = bin count or w = bin width)
enum ImPlotBin_ {
    ImPlotBin_Sqrt    = -1, // k = sqrt(n)
    ImPlotBin_Sturges = -2, // k = 1 + log2(n)
    ImPlotBin_Rice    = -3, // k = 2 * cbrt(n)
    ImPlotBin_Scott   = -4, // w = 3.49 * sigma / cbrt(n)
};

// Double precision version of ImVec2 used by ImPlot. Extensible by end users.
struct ImPlotPoint {
    double x, y;
    ImPlotPoint()                         { x = y = 0.0;      }
    ImPlotPoint(double _x, double _y)     { x = _x; y = _y;   }
    ImPlotPoint(const ImVec2& p)          { x = p.x; y = p.y; }
    double  operator[] (size_t idx) const { return (&x)[idx]; }
    double& operator[] (size_t idx)       { return (&x)[idx]; }
#ifdef IMPLOT_POINT_CLASS_EXTRA
    IMPLOT_POINT_CLASS_EXTRA     // Define additional constructors and implicit cast operators in imconfig.h
                                 // to convert back and forth between your math types and ImPlotPoint.
#endif
};

// Range defined by a min/max value.
struct ImPlotRange {
    double Min, Max;
    ImPlotRange()                         { Min = 0; Max = 0;                                         }
    ImPlotRange(double _min, double _max) { Min = _min; Max = _max;                                   }
    bool Contains(double value) const     { return value >= Min && value <= Max;                      }
    double Size() const                   { return Max - Min;                                         }
    double Clamp(double value) const      { return (value < Min) ? Min : (value > Max) ? Max : value; }
};

// Combination of two range limits for X and Y axes. Also an AABB defined by Min()/Max().
struct ImPlotRect {
    ImPlotRange X, Y;
    ImPlotRect()                                                       {                                                               }
    ImPlotRect(double x_min, double x_max, double y_min, double y_max) { X.Min = x_min; X.Max = x_max; Y.Min = y_min; Y.Max = y_max;   }
    bool Contains(const ImPlotPoint& p) const                          { return Contains(p.x, p.y);                                    }
    bool Contains(double x, double y) const                            { return X.Contains(x) && Y.Contains(y);                        }
    ImPlotPoint Size() const                                           { return ImPlotPoint(X.Size(), Y.Size());                       }
    ImPlotPoint Clamp(const ImPlotPoint& p)                            { return Clamp(p.x, p.y);                                       }
    ImPlotPoint Clamp(double x, double y)                              { return ImPlotPoint(X.Clamp(x),Y.Clamp(y));                    }
    ImPlotPoint Min() const                                            { return ImPlotPoint(X.Min, Y.Min);                             }
    ImPlotPoint Max() const                                            { return ImPlotPoint(X.Max, Y.Max);                             }
};

// Plot style structure
struct ImPlotStyle {
    // item styling variables
    float   LineWeight;              // = 1,      item line weight in pixels
    int     Marker;                  // = ImPlotMarker_None, marker specification
    float   MarkerSize;              // = 4,      marker size in pixels (roughly the marker's "radius")
    float   MarkerWeight;            // = 1,      outline weight of markers in pixels
    float   FillAlpha;               // = 1,      alpha modifier applied to plot fills
    float   ErrorBarSize;            // = 5,      error bar whisker width in pixels
    float   ErrorBarWeight;          // = 1.5,    error bar whisker weight in pixels
    float   DigitalBitHeight;        // = 8,      digital channels bit height (at y = 1.0f) in pixels
    float   DigitalBitGap;           // = 4,      digital channels bit padding gap in pixels
    // plot styling variables
    float   PlotBorderSize;          // = 1,      line thickness of border around plot area
    float   MinorAlpha;              // = 0.25    alpha multiplier applied to minor axis grid lines
    ImVec2  MajorTickLen;            // = 10,10   major tick lengths for X and Y axes
    ImVec2  MinorTickLen;            // = 5,5     minor tick lengths for X and Y axes
    ImVec2  MajorTickSize;           // = 1,1     line thickness of major ticks
    ImVec2  MinorTickSize;           // = 1,1     line thickness of minor ticks
    ImVec2  MajorGridSize;           // = 1,1     line thickness of major grid lines
    ImVec2  MinorGridSize;           // = 1,1     line thickness of minor grid lines
    ImVec2  PlotPadding;             // = 10,10   padding between widget frame and plot area, labels, or outside legends (i.e. main padding)
    ImVec2  LabelPadding;            // = 5,5     padding between axes labels, tick labels, and plot edge
    ImVec2  LegendPadding;           // = 10,10   legend padding from plot edges
    ImVec2  LegendInnerPadding;      // = 5,5     legend inner padding from legend edges
    ImVec2  LegendSpacing;           // = 5,0     spacing between legend entries
    ImVec2  MousePosPadding;         // = 10,10   padding between plot edge and interior mouse location text
    ImVec2  AnnotationPadding;       // = 2,2     text padding around annotation labels
    ImVec2  FitPadding;              // = 0,0     additional fit padding as a percentage of the fit extents (e.g. ImVec2(0.1f,0.1f) adds 10% to the fit extents of X and Y)
    ImVec2  PlotDefaultSize;         // = 400,300 default size used when ImVec2(0,0) is passed to BeginPlot
    ImVec2  PlotMinSize;             // = 200,150 minimum size plot frame can be when shrunk
    // style colors
    ImVec4  Colors[ImPlotCol_COUNT]; // Array of styling colors. Indexable with ImPlotCol_ enums.
    // colormap
    ImPlotColormap Colormap;         // The current colormap. Set this to either an ImPlotColormap_ enum or an index returned by AddColormap.
    // settings/flags
    bool    UseLocalTime;            // = false,  axis labels will be formatted for your timezone when ImPlotAxisFlag_Time is enabled
    bool    UseISO8601;              // = false,  dates will be formatted according to ISO 8601 where applicable (e.g. YYYY-MM-DD, YYYY-MM, --MM-DD, etc.)
    bool    Use24HourClock;          // = false,  times will be formatted using a 24 hour clock
    IMPLOT_API ImPlotStyle();
};

// Support for legacy versions
#if (IMGUI_VERSION_NUM < 18716) // Renamed in 1.88
#define ImGuiMod_None       0
#define ImGuiMod_Ctrl       ImGuiKeyModFlags_Ctrl
#define ImGuiMod_Shift      ImGuiKeyModFlags_Shift
#define ImGuiMod_Alt        ImGuiKeyModFlags_Alt
#define ImGuiMod_Super      ImGuiKeyModFlags_Super
#elif (IMGUI_VERSION_NUM < 18823) // Renamed in 1.89, sorry
#define ImGuiMod_None       0
#define ImGuiMod_Ctrl       ImGuiModFlags_Ctrl
#define ImGuiMod_Shift      ImGuiModFlags_Shift
#define ImGuiMod_Alt        ImGuiModFlags_Alt
#define ImGuiMod_Super      ImGuiModFlags_Super
#endif

// Input mapping structure. Default values listed. See also MapInputDefault, MapInputReverse.
struct ImPlotInputMap {
    ImGuiMouseButton Pan;           // LMB    enables panning when held,
    int              PanMod;        // none   optional modifier that must be held for panning/fitting
    ImGuiMouseButton Fit;           // LMB    initiates fit when double clicked
    ImGuiMouseButton Select;        // RMB    begins box selection when pressed and confirms selection when released
    ImGuiMouseButton SelectCancel;  // LMB    cancels active box selection when pressed; cannot be same as Select
    int              SelectMod;     // none   optional modifier that must be held for box selection
    int              SelectHorzMod; // Alt    expands active box selection horizontally to plot edge when held
    int              SelectVertMod; // Shift  expands active box selection vertically to plot edge when held
    ImGuiMouseButton Menu;          // RMB    opens context menus (if enabled) when clicked
    int              OverrideMod;   // Ctrl   when held, all input is ignored; used to enable axis/plots as DND sources
    int              ZoomMod;       // none   optional modifier that must be held for scroll wheel zooming
    float            ZoomRate;      // 0.1f   zoom rate for scroll (e.g. 0.1f = 10% plot range every scroll click); make negative to invert
    IMPLOT_API ImPlotInputMap();
};

//-----------------------------------------------------------------------------
// [SECTION] Callbacks
//-----------------------------------------------------------------------------

// Callback signature for axis tick label formatter.
typedef int (*ImPlotFormatter)(double value, char* buff, int size, void* user_data);

// Callback signature for data getter.
typedef ImPlotPoint (*ImPlotGetter)(int idx, void* user_data);

// Callback signature for axis transform.
typedef double (*ImPlotTransform)(double value, void* user_data);

namespace ImPlot {

//-----------------------------------------------------------------------------
// [SECTION] Contexts
//-----------------------------------------------------------------------------

// Creates a new ImPlot context. Call this after ImGui::CreateContext.
IMPLOT_API ImPlotContext* CreateContext();
// Destroys an ImPlot context. Call this before ImGui::DestroyContext. NULL = destroy current context.
IMPLOT_API void DestroyContext(ImPlotContext* ctx = NULL);
// Returns the current ImPlot context. NULL if no context has ben set.
IMPLOT_API ImPlotContext* GetCurrentContext();
// Sets the current ImPlot context.
IMPLOT_API void SetCurrentContext(ImPlotContext* ctx);

// Sets the current **ImGui** context. This is ONLY necessary if you are compiling
// ImPlot as a DLL (not recommended) separate from your ImGui compilation. It
// sets the global variable GImGui, which is not shared across DLL boundaries.
// See GImGui documentation in imgui.cpp for more details.
IMPLOT_API void SetImGuiContext(ImGuiContext* ctx);

//-----------------------------------------------------------------------------
// [SECTION] Begin/End Plot
//-----------------------------------------------------------------------------

// Starts a 2D plotting context. If this function returns true, EndPlot() MUST
// be called! You are encouraged to use the following convention:
//
// if (BeginPlot(...)) {
//     PlotLine(...);
//     ...
//     EndPlot();
// }
//
// Important notes:
//
// - #title_id must be unique to the current ImGui ID scope. If you need to avoid ID
//   collisions or don't want to display a title in the plot, use double hashes
//   (e.g. "MyPlot##HiddenIdText" or "##NoTitle").
// - #size is the **frame** size of the plot widget, not the plot area. The default
//   size of plots (i.e. when ImVec2(0,0)) can be modified in your ImPlotStyle.
IMPLOT_API bool BeginPlot(const char* title_id, const ImVec2& size=ImVec2(-1,0), ImPlotFlags flags=0);

// Only call EndPlot() if BeginPlot() returns true! Typically called at the end
// of an if statement conditioned on BeginPlot(). See example above.
IMPLOT_API void EndPlot();

//-----------------------------------------------------------------------------
// [SECTION] Begin/End Subplots
//-----------------------------------------------------------------------------

// Starts a subdivided plotting context. If the function returns true,
// EndSubplots() MUST be called! Call BeginPlot/EndPlot AT MOST [rows*cols]
// times in  between the begining and end of the subplot context. Plots are
// added in row major order.
//
// Example:
//
// if (BeginSubplots("My Subplot",2,3,ImVec2(800,400)) {
//     for (int i = 0; i < 6; ++i) {
//         if (BeginPlot(...)) {
//             ImPlot::PlotLine(...);
//             ...
//             EndPlot();
//         }
//     }
//     EndSubplots();
// }
//
// Produces:
//
// [0] | [1] | [2]
// ----|-----|----
// [3] | [4] | [5]
//
// Important notes:
//
// - #title_id must be unique to the current ImGui ID scope. If you need to avoid ID
//   collisions or don't want to display a title in the plot, use double hashes
//   (e.g. "MySubplot##HiddenIdText" or "##NoTitle").
// - #rows and #cols must be greater than 0.
// - #size is the size of the entire grid of subplots, not the individual plots
// - #row_ratios and #col_ratios must have AT LEAST #rows and #cols elements,
//   respectively. These are the sizes of the rows and columns expressed in ratios.
//   If the user adjusts the dimensions, the arrays are updated with new ratios.
//
// Important notes regarding BeginPlot from inside of BeginSubplots:
//
// - The #title_id parameter of _BeginPlot_ (see above) does NOT have to be
//   unique when called inside of a subplot context. Subplot IDs are hashed
//   for your convenience so you don't have call PushID or generate unique title
//   strings. Simply pass an empty string to BeginPlot unless you want to title
//   each subplot.
// - The #size parameter of _BeginPlot_ (see above) is ignored when inside of a
//   subplot context. The actual size of the subplot will be based on the
//   #size value you pass to _BeginSubplots_ and #row/#col_ratios if provided.

IMPLOT_API bool BeginSubplots(const char* title_id,
                             int rows,
                             int cols,
                             const ImVec2& size,
                             ImPlotSubplotFlags flags = 0,
                             float* row_ratios        = NULL,
                             float* col_ratios        = NULL);

// Only call EndSubplots() if BeginSubplots() returns true! Typically called at the end
// of an if statement conditioned on BeginSublots(). See example above.
IMPLOT_API void EndSubplots();

//-----------------------------------------------------------------------------
// [SECTION] Setup
//-----------------------------------------------------------------------------

// The following API allows you to setup and customize various aspects of the
// current plot. The functions should be called immediately after BeginPlot
// and before any other API calls. Typical usage is as follows:

// if (BeginPlot(...)) {                     1) begin a new plot
//     SetupAxis(ImAxis_X1, "My X-Axis");    2) make Setup calls
//     SetupAxis(ImAxis_Y1, "My Y-Axis");
//     SetupLegend(ImPlotLocation_North);
//     ...
//     SetupFinish();                        3) [optional] explicitly finish setup
//     PlotLine(...);                        4) plot items
//     ...
//     EndPlot();                            5) end the plot
// }
//
// Important notes:
//
// - Always call Setup code at the top of your BeginPlot conditional statement.
// - Setup is locked once you start plotting or explicitly call SetupFinish.
//   Do NOT call Setup code after you begin plotting or after you make
//   any non-Setup API calls (e.g. utils like PlotToPixels also lock Setup)
// - Calling SetupFinish is OPTIONAL, but probably good practice. If you do not
//   call it yourself, then the first subsequent plotting or utility function will
//   call it for you.

// Enables an axis or sets the label and/or flags for an existing axis. Leave #label = NULL for no label.
IMPLOT_API void SetupAxis(ImAxis axis, const char* label=NULL, ImPlotAxisFlags flags=0);
// Sets an axis range limits. If ImPlotCond_Always is used, the axes limits will be locked.
IMPLOT_API void SetupAxisLimits(ImAxis axis, double v_min, double v_max, ImPlotCond cond = ImPlotCond_Once);
// Links an axis range limits to external values. Set to NULL for no linkage. The pointer data must remain valid until EndPlot.
IMPLOT_API void SetupAxisLinks(ImAxis axis, double* link_min, double* link_max);
// Sets the format of numeric axis labels via formater specifier (default="%g"). Formated values will be double (i.e. use %f).
IMPLOT_API void SetupAxisFormat(ImAxis axis, const char* fmt);
// Sets the format of numeric axis labels via formatter callback. Given #value, write a label into #buff. Optionally pass user data.
IMPLOT_API void SetupAxisFormat(ImAxis axis, ImPlotFormatter formatter, void* data=NULL);
// Sets an axis' ticks and optionally the labels. To keep the default ticks, set #keep_default=true.
IMPLOT_API void SetupAxisTicks(ImAxis axis, const double* values, int n_ticks, const char* const labels[]=NULL, bool keep_default=false);
// Sets an axis' ticks and optionally the labels for the next plot. To keep the default ticks, set #keep_default=true.
IMPLOT_API void SetupAxisTicks(ImAxis axis, double v_min, double v_max, int n_ticks, const char* const labels[]=NULL, bool keep_default=false);
// Sets an axis' scale using built-in options.
IMPLOT_API void SetupAxisScale(ImAxis axis, ImPlotScale scale);
// Sets an axis' scale using user supplied forward and inverse transfroms.
IMPLOT_API void SetupAxisScale(ImAxis axis, ImPlotTransform forward, ImPlotTransform inverse, void* data=NULL);
// Sets an axis' limits constraints.
IMPLOT_API void SetupAxisLimitsConstraints(ImAxis axis, double v_min, double v_max);
// Sets an axis' zoom constraints.
IMPLOT_API void SetupAxisZoomConstraints(ImAxis axis, double z_min, double z_max);

// Sets the label and/or flags for primary X and Y axes (shorthand for two calls to SetupAxis).
IMPLOT_API void SetupAxes(const char* x_label, const char* y_label, ImPlotAxisFlags x_flags=0, ImPlotAxisFlags y_flags=0);
// Sets the primary X and Y axes range limits. If ImPlotCond_Always is used, the axes limits will be locked (shorthand for two calls to SetupAxisLimits).
IMPLOT_API void SetupAxesLimits(double x_min, double x_max, double y_min, double y_max, ImPlotCond cond = ImPlotCond_Once);

// Sets up the plot legend.
IMPLOT_API void SetupLegend(ImPlotLocation location, ImPlotLegendFlags flags=0);
// Set the location of the current plot's mouse position text (default = South|East).
IMPLOT_API void SetupMouseText(ImPlotLocation location, ImPlotMouseTextFlags flags=0);

// Explicitly finalize plot setup. Once you call this, you cannot make anymore Setup calls for the current plot!
// Note that calling this function is OPTIONAL; it will be called by the first subsequent setup-locking API call.
IMPLOT_API void SetupFinish();

//-----------------------------------------------------------------------------
// [SECTION] SetNext
//-----------------------------------------------------------------------------

// Though you should default to the `Setup` API above, there are some scenarios
// where (re)configuring a plot or axis before `BeginPlot` is needed (e.g. if
// using a preceding button or slider widget to change the plot limits). In
// this case, you can use the `SetNext` API below. While this is not as feature
// rich as the Setup API, most common needs are provided. These functions can be
// called anwhere except for inside of `Begin/EndPlot`. For example:

// if (ImGui::Button("Center Plot"))
//     ImPlot::SetNextPlotLimits(-1,1,-1,1);
// if (ImPlot::BeginPlot(...)) {
//     ...
//     ImPlot::EndPlot();
// }
//
// Important notes:
//
// - You must still enable non-default axes with SetupAxis for these functions
//   to work properly.

// Sets an upcoming axis range limits. If ImPlotCond_Always is used, the axes limits will be locked.
IMPLOT_API void SetNextAxisLimits(ImAxis axis, double v_min, double v_max, ImPlotCond cond = ImPlotCond_Once);
// Links an upcoming axis range limits to external values. Set to NULL for no linkage. The pointer data must remain valid until EndPlot!
IMPLOT_API void SetNextAxisLinks(ImAxis axis, double* link_min, double* link_max);
// Set an upcoming axis to auto fit to its data.
IMPLOT_API void SetNextAxisToFit(ImAxis axis);

// Sets the upcoming primary X and Y axes range limits. If ImPlotCond_Always is used, the axes limits will be locked (shorthand for two calls to SetupAxisLimits).
IMPLOT_API void SetNextAxesLimits(double x_min, double x_max, double y_min, double y_max, ImPlotCond cond = ImPlotCond_Once);
// Sets all upcoming axes to auto fit to their data.
IMPLOT_API void SetNextAxesToFit();

//-----------------------------------------------------------------------------
// [SECTION] Plot Items
//-----------------------------------------------------------------------------

// The main plotting API is provied below. Call these functions between
// Begin/EndPlot and after any Setup API calls. Each plots data on the current
// x and y axes, which can be changed with `SetAxis/Axes`.
//
// The templated functions are explicitly instantiated in implot_items.cpp.
// They are not intended to be used generically with custom types. You will get
// a linker error if you try! All functions support the following scalar types:
//
// float, double, ImS8, ImU8, ImS16, ImU16, ImS32, ImU32, ImS64, ImU64
//
//
// If you need to plot custom or non-homogenous data you have a few options:
//
// 1. If your data is a simple struct/class (e.g. Vector2f), you can use striding.
//    This is the most performant option if applicable.
//
//    struct Vector2f { float X, Y; };
//    ...
//    Vector2f data[42];
//    ImPlot::PlotLine("line", &data[0].x, &data[0].y, 42, 0, 0, sizeof(Vector2f));
//
// 2. Write a custom getter C function or C++ lambda and pass it and optionally your data to
//    an ImPlot function post-fixed with a G (e.g. PlotScatterG). This has a slight performance
//    cost, but probably not enough to worry about unless your data is very large. Examples:
//
//    ImPlotPoint MyDataGetter(void* data, int idx) {
//        MyData* my_data = (MyData*)data;
//        ImPlotPoint p;
//        p.x = my_data->GetTime(idx);
//        p.y = my_data->GetValue(idx);
//        return p
//    }
//    ...
//    auto my_lambda = [](int idx, void*) {
//        double t = idx / 999.0;
//        return ImPlotPoint(t, 0.5+0.5*std::sin(2*PI*10*t));
//    };
//    ...
//    if (ImPlot::BeginPlot("MyPlot")) {
//        MyData my_data;
//        ImPlot::PlotScatterG("scatter", MyDataGetter, &my_data, my_data.Size());
//        ImPlot::PlotLineG("line", my_lambda, nullptr, 1000);
//        ImPlot::EndPlot();
//    }
//
// NB: All types are converted to double before plotting. You may lose information
// if you try plotting extremely large 64-bit integral types. Proceed with caution!

// Plots a standard 2D line plot.
IMPLOT_TMP void PlotLine(const char* label_id, const T* values, int count, double xscale=1, double xstart=0, ImPlotLineFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_TMP void PlotLine(const char* label_id, const T* xs, const T* ys, int count, ImPlotLineFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_API void PlotLineG(const char* label_id, ImPlotGetter getter, void* data, int count, ImPlotLineFlags flags=0);

// Plots a standard 2D scatter plot. Default marker is ImPlotMarker_Circle.
IMPLOT_TMP void PlotScatter(const char* label_id, const T* values, int count, double xscale=1, double xstart=0, ImPlotScatterFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_TMP void PlotScatter(const char* label_id, const T* xs, const T* ys, int count, ImPlotScatterFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_API void PlotScatterG(const char* label_id, ImPlotGetter getter, void* data, int count, ImPlotScatterFlags flags=0);

// Plots a a stairstep graph. The y value is continued constantly to the right from every x position, i.e. the interval [x[i], x[i+1]) has the value y[i]
IMPLOT_TMP void PlotStairs(const char* label_id, const T* values, int count, double xscale=1, double xstart=0, ImPlotStairsFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_TMP void PlotStairs(const char* label_id, const T* xs, const T* ys, int count, ImPlotStairsFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_API void PlotStairsG(const char* label_id, ImPlotGetter getter, void* data, int count, ImPlotStairsFlags flags=0);

// Plots a shaded (filled) region between two lines, or a line and a horizontal reference. Set yref to +/-INFINITY for infinite fill extents.
IMPLOT_TMP void PlotShaded(const char* label_id, const T* values, int count, double yref=0, double xscale=1, double xstart=0, ImPlotShadedFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_TMP void PlotShaded(const char* label_id, const T* xs, const T* ys, int count, double yref=0, ImPlotShadedFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_TMP void PlotShaded(const char* label_id, const T* xs, const T* ys1, const T* ys2, int count, ImPlotShadedFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_API void PlotShadedG(const char* label_id, ImPlotGetter getter1, void* data1, ImPlotGetter getter2, void* data2, int count, ImPlotShadedFlags flags=0);

// Plots a bar graph. Vertical by default. #bar_size and #shift are in plot units.
IMPLOT_TMP void PlotBars(const char* label_id, const T* values, int count, double bar_size=0.67, double shift=0, ImPlotBarsFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_TMP void PlotBars(const char* label_id, const T* xs, const T* ys, int count, double bar_size, ImPlotBarsFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_API void PlotBarsG(const char* label_id, ImPlotGetter getter, void* data, int count, double bar_size, ImPlotBarsFlags flags=0);

// Plots a group of bars. #values is a row-major matrix with #item_count rows and #group_count cols. #label_ids should have #item_count elements.
IMPLOT_TMP void PlotBarGroups(const char* const label_ids[], const T* values, int item_count, int group_count, double group_size=0.67, double shift=0, ImPlotBarGroupsFlags flags=0);

// Plots vertical error bar. The label_id should be the same as the label_id of the associated line or bar plot.
IMPLOT_TMP void PlotErrorBars(const char* label_id, const T* xs, const T* ys, const T* err, int count, ImPlotErrorBarsFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_TMP void PlotErrorBars(const char* label_id, const T* xs, const T* ys, const T* neg, const T* pos, int count, ImPlotErrorBarsFlags flags=0, int offset=0, int stride=sizeof(T));

// Plots stems. Vertical by default.
IMPLOT_TMP void PlotStems(const char* label_id, const T* values, int count, double ref=0, double scale=1, double start=0, ImPlotStemsFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_TMP void PlotStems(const char* label_id, const T* xs, const T* ys, int count, double ref=0, ImPlotStemsFlags flags=0, int offset=0, int stride=sizeof(T));

// Plots infinite vertical or horizontal lines (e.g. for references or asymptotes).
IMPLOT_TMP void PlotInfLines(const char* label_id, const T* values, int count, ImPlotInfLinesFlags flags=0, int offset=0, int stride=sizeof(T));

// Plots a pie chart. Center and radius are in plot units. #label_fmt can be set to NULL for no labels.
IMPLOT_TMP void PlotPieChart(const char* const label_ids[], const T* values, int count, double x, double y, double radius, const char* label_fmt="%.1f", double angle0=90, ImPlotPieChartFlags flags=0);

// Plots a 2D heatmap chart. Values are expected to be in row-major order by default. Leave #scale_min and scale_max both at 0 for automatic color scaling, or set them to a predefined range. #label_fmt can be set to NULL for no labels.
IMPLOT_TMP void PlotHeatmap(const char* label_id, const T* values, int rows, int cols, double scale_min=0, double scale_max=0, const char* label_fmt="%.1f", const ImPlotPoint& bounds_min=ImPlotPoint(0,0), const ImPlotPoint& bounds_max=ImPlotPoint(1,1), ImPlotHeatmapFlags flags=0);

// Plots a horizontal histogram. #bins can be a positive integer or an ImPlotBin_ method. If #range is left unspecified, the min/max of #values will be used as the range.
// Otherwise, outlier values outside of the range are not binned. The largest bin count or density is returned.
IMPLOT_TMP double PlotHistogram(const char* label_id, const T* values, int count, int bins=ImPlotBin_Sturges, double bar_scale=1.0, ImPlotRange range=ImPlotRange(), ImPlotHistogramFlags flags=0);

// Plots two dimensional, bivariate histogram as a heatmap. #x_bins and #y_bins can be a positive integer or an ImPlotBin. If #range is left unspecified, the min/max of
// #xs an #ys will be used as the ranges. Otherwise, outlier values outside of range are not binned. The largest bin count or density is returned.
IMPLOT_TMP double PlotHistogram2D(const char* label_id, const T* xs, const T* ys, int count, int x_bins=ImPlotBin_Sturges, int y_bins=ImPlotBin_Sturges, ImPlotRect range=ImPlotRect(), ImPlotHistogramFlags flags=0);

// Plots digital data. Digital plots do not respond to y drag or zoom, and are always referenced to the bottom of the plot.
IMPLOT_TMP void PlotDigital(const char* label_id, const T* xs, const T* ys, int count, ImPlotDigitalFlags flags=0, int offset=0, int stride=sizeof(T));
IMPLOT_API void PlotDigitalG(const char* label_id, ImPlotGetter getter, void* data, int count, ImPlotDigitalFlags flags=0);

// Plots an axis-aligned image. #bounds_min/bounds_max are in plot coordinates (y-up) and #uv0/uv1 are in texture coordinates (y-down).
IMPLOT_API void PlotImage(const char* label_id, ImTextureID user_texture_id, const ImPlotPoint& bounds_min, const ImPlotPoint& bounds_max, const ImVec2& uv0=ImVec2(0,0), const ImVec2& uv1=ImVec2(1,1), const ImVec4& tint_col=ImVec4(1,1,1,1), ImPlotImageFlags flags=0);

// Plots a centered text label at point x,y with an optional pixel offset. Text color can be changed with ImPlot::PushStyleColor(ImPlotCol_InlayText, ...).
IMPLOT_API void PlotText(const char* text, double x, double y, const ImVec2& pix_offset=ImVec2(0,0), ImPlotTextFlags flags=0);

// Plots a dummy item (i.e. adds a legend entry colored by ImPlotCol_Line)
IMPLOT_API void PlotDummy(const char* label_id, ImPlotDummyFlags flags=0);

//-----------------------------------------------------------------------------
// [SECTION] Plot Tools
//-----------------------------------------------------------------------------

// The following can be used to render interactive elements and/or annotations.
// Like the item plotting functions above, they apply to the current x and y
// axes, which can be changed with `SetAxis/SetAxes`.

// Shows a draggable point at x,y. #col defaults to ImGuiCol_Text.
IMPLOT_API bool DragPoint(int id, double* x, double* y, const ImVec4& col, float size = 4, ImPlotDragToolFlags flags=0);
// Shows a draggable vertical guide line at an x-value. #col defaults to ImGuiCol_Text.
IMPLOT_API bool DragLineX(int id, double* x, const ImVec4& col, float thickness = 1, ImPlotDragToolFlags flags=0);
// Shows a draggable horizontal guide line at a y-value. #col defaults to ImGuiCol_Text.
IMPLOT_API bool DragLineY(int id, double* y, const ImVec4& col, float thickness = 1, ImPlotDragToolFlags flags=0);
// Shows a draggable and resizeable rectangle.
IMPLOT_API bool DragRect(int id, double* x1, double* y1, double* x2, double* y2, const ImVec4& col, ImPlotDragToolFlags flags=0);

// Shows an annotation callout at a chosen point. Clamping keeps annotations in the plot area. Annotations are always rendered on top.
IMPLOT_API void Annotation(double x, double y, const ImVec4& col, const ImVec2& pix_offset, bool clamp, bool round = false);
IMPLOT_API void Annotation(double x, double y, const ImVec4& col, const ImVec2& pix_offset, bool clamp, const char* fmt, ...)           IM_FMTARGS(6);
IMPLOT_API void AnnotationV(double x, double y, const ImVec4& col, const ImVec2& pix_offset, bool clamp, const char* fmt, va_list args) IM_FMTLIST(6);

// Shows a x-axis tag at the specified coordinate value.
IMPLOT_API void TagX(double x, const ImVec4& col, bool round = false);
IMPLOT_API void TagX(double x, const ImVec4& col, const char* fmt, ...)           IM_FMTARGS(3);
IMPLOT_API void TagXV(double x, const ImVec4& col, const char* fmt, va_list args) IM_FMTLIST(3);

// Shows a y-axis tag at the specified coordinate value.
IMPLOT_API void TagY(double y, const ImVec4& col, bool round = false);
IMPLOT_API void TagY(double y, const ImVec4& col, const char* fmt, ...)           IM_FMTARGS(3);
IMPLOT_API void TagYV(double y, const ImVec4& col, const char* fmt, va_list args) IM_FMTLIST(3);

//-----------------------------------------------------------------------------
// [SECTION] Plot Utils
//-----------------------------------------------------------------------------

// Select which axis/axes will be used for subsequent plot elements.
IMPLOT_API void SetAxis(ImAxis axis);
IMPLOT_API void SetAxes(ImAxis x_axis, ImAxis y_axis);

// Convert pixels to a position in the current plot's coordinate system. Passing IMPLOT_AUTO uses the current axes.
IMPLOT_API ImPlotPoint PixelsToPlot(const ImVec2& pix, ImAxis x_axis = IMPLOT_AUTO, ImAxis y_axis = IMPLOT_AUTO);
IMPLOT_API ImPlotPoint PixelsToPlot(float x, float y, ImAxis x_axis = IMPLOT_AUTO, ImAxis y_axis = IMPLOT_AUTO);

// Convert a position in the current plot's coordinate system to pixels. Passing IMPLOT_AUTO uses the current axes.
IMPLOT_API ImVec2 PlotToPixels(const ImPlotPoint& plt, ImAxis x_axis = IMPLOT_AUTO, ImAxis y_axis = IMPLOT_AUTO);
IMPLOT_API ImVec2 PlotToPixels(double x, double y, ImAxis x_axis = IMPLOT_AUTO, ImAxis y_axis = IMPLOT_AUTO);

// Get the current Plot position (top-left) in pixels.
IMPLOT_API ImVec2 GetPlotPos();
// Get the curent Plot size in pixels.
IMPLOT_API ImVec2 GetPlotSize();

// Returns the mouse position in x,y coordinates of the current plot. Passing IMPLOT_AUTO uses the current axes.
IMPLOT_API ImPlotPoint GetPlotMousePos(ImAxis x_axis = IMPLOT_AUTO, ImAxis y_axis = IMPLOT_AUTO);
// Returns the current plot axis range.
IMPLOT_API ImPlotRect GetPlotLimits(ImAxis x_axis = IMPLOT_AUTO, ImAxis y_axis = IMPLOT_AUTO);

// Returns true if the plot area in the current plot is hovered.
IMPLOT_API bool IsPlotHovered();
// Returns true if the axis label area in the current plot is hovered.
IMPLOT_API bool IsAxisHovered(ImAxis axis);
// Returns true if the bounding frame of a subplot is hovered.
IMPLOT_API bool IsSubplotsHovered();

// Returns true if the current plot is being box selected.
IMPLOT_API bool IsPlotSelected();
// Returns the current plot box selection bounds. Passing IMPLOT_AUTO uses the current axes.
IMPLOT_API ImPlotRect GetPlotSelection(ImAxis x_axis = IMPLOT_AUTO, ImAxis y_axis = IMPLOT_AUTO);
// Cancels a the current plot box selection.
IMPLOT_API void CancelPlotSelection();

// Hides or shows the next plot item (i.e. as if it were toggled from the legend).
// Use ImPlotCond_Always if you need to forcefully set this every frame.
IMPLOT_API void HideNextItem(bool hidden = true, ImPlotCond cond = ImPlotCond_Once);

// Use the following around calls to Begin/EndPlot to align l/r/t/b padding.
// Consider using Begin/EndSubplots first. They are more feature rich and
// accomplish the same behaviour by default. The functions below offer lower
// level control of plot alignment.

// Align axis padding over multiple plots in a single row or column. #group_id must
// be unique. If this function returns true, EndAlignedPlots() must be called.
IMPLOT_API bool BeginAlignedPlots(const char* group_id, bool vertical = true);
// Only call EndAlignedPlots() if BeginAlignedPlots() returns true!
IMPLOT_API void EndAlignedPlots();

//-----------------------------------------------------------------------------
// [SECTION] Legend Utils
//-----------------------------------------------------------------------------

// Begin a popup for a legend entry.
IMPLOT_API bool BeginLegendPopup(const char* label_id, ImGuiMouseButton mouse_button=1);
// End a popup for a legend entry.
IMPLOT_API void EndLegendPopup();
// Returns true if a plot item legend entry is hovered.
IMPLOT_API bool IsLegendEntryHovered(const char* label_id);

//-----------------------------------------------------------------------------
// [SECTION] Drag and Drop
//-----------------------------------------------------------------------------

// Turns the current plot's plotting area into a drag and drop target. Don't forget to call EndDragDropTarget!
IMPLOT_API bool BeginDragDropTargetPlot();
// Turns the current plot's X-axis into a drag and drop target. Don't forget to call EndDragDropTarget!
IMPLOT_API bool BeginDragDropTargetAxis(ImAxis axis);
// Turns the current plot's legend into a drag and drop target. Don't forget to call EndDragDropTarget!
IMPLOT_API bool BeginDragDropTargetLegend();
// Ends a drag and drop target (currently just an alias for ImGui::EndDragDropTarget).
IMPLOT_API void EndDragDropTarget();

// NB: By default, plot and axes drag and drop *sources* require holding the Ctrl modifier to initiate the drag.
// You can change the modifier if desired. If ImGuiMod_None is provided, the axes will be locked from panning.

// Turns the current plot's plotting area into a drag and drop source. You must hold Ctrl. Don't forget to call EndDragDropSource!
IMPLOT_API bool BeginDragDropSourcePlot(ImGuiDragDropFlags flags=0);
// Turns the current plot's X-axis into a drag and drop source. You must hold Ctrl. Don't forget to call EndDragDropSource!
IMPLOT_API bool BeginDragDropSourceAxis(ImAxis axis, ImGuiDragDropFlags flags=0);
// Turns an item in the current plot's legend into drag and drop source. Don't forget to call EndDragDropSource!
IMPLOT_API bool BeginDragDropSourceItem(const char* label_id, ImGuiDragDropFlags flags=0);
// Ends a drag and drop source (currently just an alias for ImGui::EndDragDropSource).
IMPLOT_API void EndDragDropSource();

//-----------------------------------------------------------------------------
// [SECTION] Styling
//-----------------------------------------------------------------------------

// Styling colors in ImPlot works similarly to styling colors in ImGui, but
// with one important difference. Like ImGui, all style colors are stored in an
// indexable array in ImPlotStyle. You can permanently modify these values through
// GetStyle().Colors, or temporarily modify them with Push/Pop functions below.
// However, by default all style colors in ImPlot default to a special color
// IMPLOT_AUTO_COL. The behavior of this color depends upon the style color to
// which it as applied:
//
//     1) For style colors associated with plot items (e.g. ImPlotCol_Line),
//        IMPLOT_AUTO_COL tells ImPlot to color the item with the next unused
//        color in the current colormap. Thus, every item will have a different
//        color up to the number of colors in the colormap, at which point the
//        colormap will roll over. For most use cases, you should not need to
//        set these style colors to anything but IMPLOT_COL_AUTO; you are
//        probably better off changing the current colormap. However, if you
//        need to explicitly color a particular item you may either Push/Pop
//        the style color around the item in question, or use the SetNextXXXStyle
//        API below. If you permanently set one of these style colors to a specific
//        color, or forget to call Pop, then all subsequent items will be styled
//        with the color you set.
//
//     2) For style colors associated with plot styling (e.g. ImPlotCol_PlotBg),
//        IMPLOT_AUTO_COL tells ImPlot to set that color from color data in your
//        **ImGuiStyle**. The ImGuiCol_ that these style colors default to are
//        detailed above, and in general have been mapped to produce plots visually
//        consistent with your current ImGui style. Of course, you are free to
//        manually set these colors to whatever you like, and further can Push/Pop
//        them around individual plots for plot-specific styling (e.g. coloring axes).

// Provides access to plot style structure for permanant modifications to colors, sizes, etc.
IMPLOT_API ImPlotStyle& GetStyle();

// Style plot colors for current ImGui style (default).
IMPLOT_API void StyleColorsAuto(ImPlotStyle* dst = NULL);
// Style plot colors for ImGui "Classic".
IMPLOT_API void StyleColorsClassic(ImPlotStyle* dst = NULL);
// Style plot colors for ImGui "Dark".
IMPLOT_API void StyleColorsDark(ImPlotStyle* dst = NULL);
// Style plot colors for ImGui "Light".
IMPLOT_API void StyleColorsLight(ImPlotStyle* dst = NULL);

// Use PushStyleX to temporarily modify your ImPlotStyle. The modification
// will last until the matching call to PopStyleX. You MUST call a pop for
// every push, otherwise you will leak memory! This behaves just like ImGui.

// Temporarily modify a style color. Don't forget to call PopStyleColor!
IMPLOT_API void PushStyleColor(ImPlotCol idx, ImU32 col);
IMPLOT_API void PushStyleColor(ImPlotCol idx, const ImVec4& col);
// Undo temporary style color modification(s). Undo multiple pushes at once by increasing count.
IMPLOT_API void PopStyleColor(int count = 1);

// Temporarily modify a style variable of float type. Don't forget to call PopStyleVar!
IMPLOT_API void PushStyleVar(ImPlotStyleVar idx, float val);
// Temporarily modify a style variable of int type. Don't forget to call PopStyleVar!
IMPLOT_API void PushStyleVar(ImPlotStyleVar idx, int val);
// Temporarily modify a style variable of ImVec2 type. Don't forget to call PopStyleVar!
IMPLOT_API void PushStyleVar(ImPlotStyleVar idx, const ImVec2& val);
// Undo temporary style variable modification(s). Undo multiple pushes at once by increasing count.
IMPLOT_API void PopStyleVar(int count = 1);

// The following can be used to modify the style of the next plot item ONLY. They do
// NOT require calls to PopStyleX. Leave style attributes you don't want modified to
// IMPLOT_AUTO or IMPLOT_AUTO_COL. Automatic styles will be deduced from the current
// values in your ImPlotStyle or from Colormap data.

// Set the line color and weight for the next item only.
IMPLOT_API void SetNextLineStyle(const ImVec4& col = IMPLOT_AUTO_COL, float weight = IMPLOT_AUTO);
// Set the fill color for the next item only.
IMPLOT_API void SetNextFillStyle(const ImVec4& col = IMPLOT_AUTO_COL, float alpha_mod = IMPLOT_AUTO);
// Set the marker style for the next item only.
IMPLOT_API void SetNextMarkerStyle(ImPlotMarker marker = IMPLOT_AUTO, float size = IMPLOT_AUTO, const ImVec4& fill = IMPLOT_AUTO_COL, float weight = IMPLOT_AUTO, const ImVec4& outline = IMPLOT_AUTO_COL);
// Set the error bar style for the next item only.
IMPLOT_API void SetNextErrorBarStyle(const ImVec4& col = IMPLOT_AUTO_COL, float size = IMPLOT_AUTO, float weight = IMPLOT_AUTO);

// Gets the last item primary color (i.e. its legend icon color)
IMPLOT_API ImVec4 GetLastItemColor();

// Returns the null terminated string name for an ImPlotCol.
IMPLOT_API const char* GetStyleColorName(ImPlotCol idx);
// Returns the null terminated string name for an ImPlotMarker.
IMPLOT_API const char* GetMarkerName(ImPlotMarker idx);

//-----------------------------------------------------------------------------
// [SECTION] Colormaps
//-----------------------------------------------------------------------------

// Item styling is based on colormaps when the relevant ImPlotCol_XXX is set to
// IMPLOT_AUTO_COL (default). Several built-in colormaps are available. You can
// add and then push/pop your own colormaps as well. To permanently set a colormap,
// modify the Colormap index member of your ImPlotStyle.

// Colormap data will be ignored and a custom color will be used if you have done one of the following:
//     1) Modified an item style color in your ImPlotStyle to anything other than IMPLOT_AUTO_COL.
//     2) Pushed an item style color using PushStyleColor().
//     3) Set the next item style with a SetNextXXXStyle function.

// Add a new colormap. The color data will be copied. The colormap can be used by pushing either the returned index or the
// string name with PushColormap. The colormap name must be unique and the size must be greater than 1. You will receive
// an assert otherwise! By default colormaps are considered to be qualitative (i.e. discrete). If you want to create a
// continuous colormap, set #qual=false. This will treat the colors you provide as keys, and ImPlot will build a linearly
// interpolated lookup table. The memory footprint of this table will be exactly ((size-1)*255+1)*4 bytes.
IMPLOT_API ImPlotColormap AddColormap(const char* name, const ImVec4* cols, int size, bool qual=true);
IMPLOT_API ImPlotColormap AddColormap(const char* name, const ImU32*  cols, int size, bool qual=true);

// Returns the number of available colormaps (i.e. the built-in + user-added count).
IMPLOT_API int GetColormapCount();
// Returns a null terminated string name for a colormap given an index. Returns NULL if index is invalid.
IMPLOT_API const char* GetColormapName(ImPlotColormap cmap);
// Returns an index number for a colormap given a valid string name. Returns -1 if name is invalid.
IMPLOT_API ImPlotColormap GetColormapIndex(const char* name);

// Temporarily switch to one of the built-in (i.e. ImPlotColormap_XXX) or user-added colormaps (i.e. a return value of AddColormap). Don't forget to call PopColormap!
IMPLOT_API void PushColormap(ImPlotColormap cmap);
// Push a colormap by string name. Use built-in names such as "Default", "Deep", "Jet", etc. or a string you provided to AddColormap. Don't forget to call PopColormap!
IMPLOT_API void PushColormap(const char* name);
// Undo temporary colormap modification(s). Undo multiple pushes at once by increasing count.
IMPLOT_API void PopColormap(int count = 1);

// Returns the next color from the current colormap and advances the colormap for the current plot.
// Can also be used with no return value to skip colors if desired. You need to call this between Begin/EndPlot!
IMPLOT_API ImVec4 NextColormapColor();

// Colormap utils. If cmap = IMPLOT_AUTO (default), the current colormap is assumed.
// Pass an explicit colormap index (built-in or user-added) to specify otherwise.

// Returns the size of a colormap.
IMPLOT_API int GetColormapSize(ImPlotColormap cmap = IMPLOT_AUTO);
// Returns a color from a colormap given an index >= 0 (modulo will be performed).
IMPLOT_API ImVec4 GetColormapColor(int idx, ImPlotColormap cmap = IMPLOT_AUTO);
// Sample a color from the current colormap given t between 0 and 1.
IMPLOT_API ImVec4 SampleColormap(float t, ImPlotColormap cmap = IMPLOT_AUTO);

// Shows a vertical color scale with linear spaced ticks using the specified color map. Use double hashes to hide label (e.g. "##NoLabel"). If scale_min > scale_max, the scale to color mapping will be reversed.
IMPLOT_API void ColormapScale(const char* label, double scale_min, double scale_max, const ImVec2& size = ImVec2(0,0), const char* format = "%g", ImPlotColormapScaleFlags flags = 0, ImPlotColormap cmap = IMPLOT_AUTO);
// Shows a horizontal slider with a colormap gradient background. Optionally returns the color sampled at t in [0 1].
IMPLOT_API bool ColormapSlider(const char* label, float* t, ImVec4* out = NULL, const char* format = "", ImPlotColormap cmap = IMPLOT_AUTO);
// Shows a button with a colormap gradient brackground.
IMPLOT_API bool ColormapButton(const char* label, const ImVec2& size = ImVec2(0,0), ImPlotColormap cmap = IMPLOT_AUTO);

// When items in a plot sample their color from a colormap, the color is cached and does not change
// unless explicitly overriden. Therefore, if you change the colormap after the item has already been plotted,
// item colors will NOT update. If you need item colors to resample the new colormap, then use this
// function to bust the cached colors. If #plot_title_id is NULL, then every item in EVERY existing plot
// will be cache busted. Otherwise only the plot specified by #plot_title_id will be busted. For the
// latter, this function must be called in the same ImGui ID scope that the plot is in. You should rarely if ever
// need this function, but it is available for applications that require runtime colormap swaps (e.g. Heatmaps demo).
IMPLOT_API void BustColorCache(const char* plot_title_id = NULL);

//-----------------------------------------------------------------------------
// [SECTION] Input Mapping
//-----------------------------------------------------------------------------

// Provides access to input mapping structure for permanant modifications to controls for pan, select, etc.
IMPLOT_API ImPlotInputMap& GetInputMap();

// Default input mapping: pan = LMB drag, box select = RMB drag, fit = LMB double click, context menu = RMB click, zoom = scroll.
IMPLOT_API void MapInputDefault(ImPlotInputMap* dst = NULL);
// Reverse input mapping: pan = RMB drag, box select = LMB drag, fit = LMB double click, context menu = RMB click, zoom = scroll.
IMPLOT_API void MapInputReverse(ImPlotInputMap* dst = NULL);

//-----------------------------------------------------------------------------
// [SECTION] Miscellaneous
//-----------------------------------------------------------------------------

// Render icons similar to those that appear in legends (nifty for data lists).
IMPLOT_API void ItemIcon(const ImVec4& col);
IMPLOT_API void ItemIcon(ImU32 col);
IMPLOT_API void ColormapIcon(ImPlotColormap cmap);

// Get the plot draw list for custom rendering to the current plot area. Call between Begin/EndPlot.
IMPLOT_API ImDrawList* GetPlotDrawList();
// Push clip rect for rendering to current plot area. The rect can be expanded or contracted by #expand pixels. Call between Begin/EndPlot.
IMPLOT_API void PushPlotClipRect(float expand=0);
// Pop plot clip rect. Call between Begin/EndPlot.
IMPLOT_API void PopPlotClipRect();

// Shows ImPlot style selector dropdown menu.
IMPLOT_API bool ShowStyleSelector(const char* label);
// Shows ImPlot colormap selector dropdown menu.
IMPLOT_API bool ShowColormapSelector(const char* label);
// Shows ImPlot input map selector dropdown menu.
IMPLOT_API bool ShowInputMapSelector(const char* label);
// Shows ImPlot style editor block (not a window).
IMPLOT_API void ShowStyleEditor(ImPlotStyle* ref = NULL);
// Add basic help/info block for end users (not a window).
IMPLOT_API void ShowUserGuide();
// Shows ImPlot metrics/debug information window.
IMPLOT_API void ShowMetricsWindow(bool* p_popen = NULL);

//-----------------------------------------------------------------------------
// [SECTION] Demo
//-----------------------------------------------------------------------------

// Shows the ImPlot demo window (add implot_demo.cpp to your sources!)
IMPLOT_API void ShowDemoWindow(bool* p_open = NULL);

}  // namespace ImPlot

//-----------------------------------------------------------------------------
// [SECTION] Obsolete API
//-----------------------------------------------------------------------------

// The following functions will be removed! Keep your copy of implot up to date!
// Occasionally set '#define IMPLOT_DISABLE_OBSOLETE_FUNCTIONS' to stay ahead.
// If you absolutely must use these functions and do not want to receive compiler
// warnings, set '#define IMPLOT_DISABLE_OBSOLETE_WARNINGS'.

#ifndef IMPLOT_DISABLE_OBSOLETE_FUNCTIONS

#ifndef IMPLOT_DISABLE_DEPRECATED_WARNINGS
#if __cplusplus > 201402L
#define IMPLOT_DEPRECATED(method) [[deprecated]] method
#elif defined( __GNUC__ ) && !defined( __INTEL_COMPILER ) && ( __GNUC__ > 3 || ( __GNUC__ == 3 && __GNUC_MINOR__ >= 1 ) )
#define IMPLOT_DEPRECATED(method) method __attribute__( ( deprecated ) )
#elif defined( _MSC_VER )
#define IMPLOT_DEPRECATED(method) __declspec(deprecated) method
#else
#define IMPLOT_DEPRECATED(method) method
#endif
#else
#define IMPLOT_DEPRECATED(method) method
#endif

enum ImPlotFlagsObsolete_ {
    ImPlotFlags_YAxis2 = 1 << 20,
    ImPlotFlags_YAxis3 = 1 << 21,
};

namespace ImPlot {

// OBSOLETED in v0.13 -> PLANNED REMOVAL in v1.0
IMPLOT_DEPRECATED( IMPLOT_API bool BeginPlot(const char* title_id,
                                             const char* x_label,  // = NULL,
                                             const char* y_label,  // = NULL,
                                             const ImVec2& size       = ImVec2(-1,0),
                                             ImPlotFlags flags        = ImPlotFlags_None,
                                             ImPlotAxisFlags x_flags  = 0,
                                             ImPlotAxisFlags y_flags  = 0,
                                             ImPlotAxisFlags y2_flags = ImPlotAxisFlags_AuxDefault,
                                             ImPlotAxisFlags y3_flags = ImPlotAxisFlags_AuxDefault,
                                             const char* y2_label     = NULL,
                                             const char* y3_label     = NULL) );

} // namespace ImPlot

#endif
