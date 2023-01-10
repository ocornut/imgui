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

// You may use this file to debug, understand or extend ImPlot features but we
// don't provide any guarantee of forward compatibility!

//-----------------------------------------------------------------------------
// [SECTION] Header Mess
//-----------------------------------------------------------------------------

#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <time.h>
#include "imgui_internal.h"

#ifndef IMPLOT_VERSION
#error Must include implot.h before implot_internal.h
#endif


// Support for pre-1.84 versions. ImPool's GetSize() -> GetBufSize()
#if (IMGUI_VERSION_NUM < 18303)
#define GetBufSize GetSize
#endif

//-----------------------------------------------------------------------------
// [SECTION] Constants
//-----------------------------------------------------------------------------

// Constants can be changed unless stated otherwise. We may move some of these
// to ImPlotStyleVar_ over time.

// Mimimum allowable timestamp value 01/01/1970 @ 12:00am (UTC) (DO NOT DECREASE THIS)
#define IMPLOT_MIN_TIME  0
// Maximum allowable timestamp value 01/01/3000 @ 12:00am (UTC) (DO NOT INCREASE THIS)
#define IMPLOT_MAX_TIME  32503680000
// Default label format for axis labels
#define IMPLOT_LABEL_FORMAT "%g"
// Max character size for tick labels
#define IMPLOT_LABEL_MAX_SIZE 32

//-----------------------------------------------------------------------------
// [SECTION] Macros
//-----------------------------------------------------------------------------

#define IMPLOT_NUM_X_AXES ImAxis_Y1
#define IMPLOT_NUM_Y_AXES (ImAxis_COUNT - IMPLOT_NUM_X_AXES)

// Split ImU32 color into RGB components [0 255]
#define IM_COL32_SPLIT_RGB(col,r,g,b) \
    ImU32 r = ((col >> IM_COL32_R_SHIFT) & 0xFF); \
    ImU32 g = ((col >> IM_COL32_G_SHIFT) & 0xFF); \
    ImU32 b = ((col >> IM_COL32_B_SHIFT) & 0xFF);

//-----------------------------------------------------------------------------
// [SECTION] Forward Declarations
//-----------------------------------------------------------------------------

struct ImPlotTick;
struct ImPlotAxis;
struct ImPlotAxisColor;
struct ImPlotItem;
struct ImPlotLegend;
struct ImPlotPlot;
struct ImPlotNextPlotData;
struct ImPlotTicker;

//-----------------------------------------------------------------------------
// [SECTION] Context Pointer
//-----------------------------------------------------------------------------

#ifndef GImPlot
extern IMPLOT_API ImPlotContext* GImPlot; // Current implicit context pointer
#endif

//-----------------------------------------------------------------------------
// [SECTION] Generic Helpers
//-----------------------------------------------------------------------------

// Computes the common (base-10) logarithm
static inline float  ImLog10(float x)  { return log10f(x); }
static inline double ImLog10(double x) { return log10(x);  }
static inline float  ImSinh(float x)   { return sinhf(x);  }
static inline double ImSinh(double x)  { return sinh(x);   }
static inline float  ImAsinh(float x)  { return asinhf(x); }
static inline double ImAsinh(double x) { return asinh(x);  }
// Returns true if a flag is set
template <typename TSet, typename TFlag>
static inline bool ImHasFlag(TSet set, TFlag flag) { return (set & flag) == flag; }
// Flips a flag in a flagset
template <typename TSet, typename TFlag>
static inline void ImFlipFlag(TSet& set, TFlag flag) { ImHasFlag(set, flag) ? set &= ~flag : set |= flag; }
// Linearly remaps x from [x0 x1] to [y0 y1].
template <typename T>
static inline T ImRemap(T x, T x0, T x1, T y0, T y1) { return y0 + (x - x0) * (y1 - y0) / (x1 - x0); }
// Linear rempas x from [x0 x1] to [0 1]
template <typename T>
static inline T ImRemap01(T x, T x0, T x1) { return (x - x0) / (x1 - x0); }
// Returns always positive modulo (assumes r != 0)
static inline int ImPosMod(int l, int r) { return (l % r + r) % r; }
// Returns true if val is NAN
static inline bool ImNan(double val) { return isnan(val); }
// Returns true if val is NAN or INFINITY
static inline bool ImNanOrInf(double val) { return !(val >= -DBL_MAX && val <= DBL_MAX) || ImNan(val); }
// Turns NANs to 0s
static inline double ImConstrainNan(double val) { return ImNan(val) ? 0 : val; }
// Turns infinity to floating point maximums
static inline double ImConstrainInf(double val) { return val >= DBL_MAX ?  DBL_MAX : val <= -DBL_MAX ? - DBL_MAX : val; }
// Turns numbers less than or equal to 0 to 0.001 (sort of arbitrary, is there a better way?)
static inline double ImConstrainLog(double val) { return val <= 0 ? 0.001f : val; }
// Turns numbers less than 0 to zero
static inline double ImConstrainTime(double val) { return val < IMPLOT_MIN_TIME ? IMPLOT_MIN_TIME : (val > IMPLOT_MAX_TIME ? IMPLOT_MAX_TIME : val); }
// True if two numbers are approximately equal using units in the last place.
static inline bool ImAlmostEqual(double v1, double v2, int ulp = 2) { return ImAbs(v1-v2) < DBL_EPSILON * ImAbs(v1+v2) * ulp || ImAbs(v1-v2) < DBL_MIN; }
// Finds min value in an unsorted array
template <typename T>
static inline T ImMinArray(const T* values, int count) { T m = values[0]; for (int i = 1; i < count; ++i) { if (values[i] < m) { m = values[i]; } } return m; }
// Finds the max value in an unsorted array
template <typename T>
static inline T ImMaxArray(const T* values, int count) { T m = values[0]; for (int i = 1; i < count; ++i) { if (values[i] > m) { m = values[i]; } } return m; }
// Finds the min and max value in an unsorted array
template <typename T>
static inline void ImMinMaxArray(const T* values, int count, T* min_out, T* max_out) {
    T Min = values[0]; T Max = values[0];
    for (int i = 1; i < count; ++i) {
        if (values[i] < Min) { Min = values[i]; }
        if (values[i] > Max) { Max = values[i]; }
    }
    *min_out = Min; *max_out = Max;
}
// Finds the sim of an array
template <typename T>
static inline T ImSum(const T* values, int count) {
    T sum  = 0;
    for (int i = 0; i < count; ++i)
        sum += values[i];
    return sum;
}
// Finds the mean of an array
template <typename T>
static inline double ImMean(const T* values, int count) {
    double den = 1.0 / count;
    double mu  = 0;
    for (int i = 0; i < count; ++i)
        mu += (double)values[i] * den;
    return mu;
}
// Finds the sample standard deviation of an array
template <typename T>
static inline double ImStdDev(const T* values, int count) {
    double den = 1.0 / (count - 1.0);
    double mu  = ImMean(values, count);
    double x   = 0;
    for (int i = 0; i < count; ++i)
        x += ((double)values[i] - mu) * ((double)values[i] - mu) * den;
    return sqrt(x);
}
// Mix color a and b by factor s in [0 256]
static inline ImU32 ImMixU32(ImU32 a, ImU32 b, ImU32 s) {
#ifdef IMPLOT_MIX64
    const ImU32 af = 256-s;
    const ImU32 bf = s;
    const ImU64 al = (a & 0x00ff00ff) | (((ImU64)(a & 0xff00ff00)) << 24);
    const ImU64 bl = (b & 0x00ff00ff) | (((ImU64)(b & 0xff00ff00)) << 24);
    const ImU64 mix = (al * af + bl * bf);
    return ((mix >> 32) & 0xff00ff00) | ((mix & 0xff00ff00) >> 8);
#else
    const ImU32 af = 256-s;
    const ImU32 bf = s;
    const ImU32 al = (a & 0x00ff00ff);
    const ImU32 ah = (a & 0xff00ff00) >> 8;
    const ImU32 bl = (b & 0x00ff00ff);
    const ImU32 bh = (b & 0xff00ff00) >> 8;
    const ImU32 ml = (al * af + bl * bf);
    const ImU32 mh = (ah * af + bh * bf);
    return (mh & 0xff00ff00) | ((ml & 0xff00ff00) >> 8);
#endif
}

// Lerp across an array of 32-bit collors given t in [0.0 1.0]
static inline ImU32 ImLerpU32(const ImU32* colors, int size, float t) {
    int i1 = (int)((size - 1 ) * t);
    int i2 = i1 + 1;
    if (i2 == size || size == 1)
        return colors[i1];
    float den = 1.0f / (size - 1);
    float t1 = i1 * den;
    float t2 = i2 * den;
    float tr = ImRemap01(t, t1, t2);
    return ImMixU32(colors[i1], colors[i2], (ImU32)(tr*256));
}

// Set alpha channel of 32-bit color from float in range [0.0 1.0]
static inline ImU32 ImAlphaU32(ImU32 col, float alpha) {
    return col & ~((ImU32)((1.0f-alpha)*255)<<IM_COL32_A_SHIFT);
}

// Returns true of two ranges overlap
template <typename T>
static inline bool ImOverlaps(T min_a, T max_a, T min_b, T max_b) {
    return min_a <= max_b && min_b <= max_a;
}

//-----------------------------------------------------------------------------
// [SECTION] ImPlot Enums
//-----------------------------------------------------------------------------

typedef int ImPlotTimeUnit;    // -> enum ImPlotTimeUnit_
typedef int ImPlotDateFmt;     // -> enum ImPlotDateFmt_
typedef int ImPlotTimeFmt;     // -> enum ImPlotTimeFmt_

enum ImPlotTimeUnit_ {
    ImPlotTimeUnit_Us,  // microsecond
    ImPlotTimeUnit_Ms,  // millisecond
    ImPlotTimeUnit_S,   // second
    ImPlotTimeUnit_Min, // minute
    ImPlotTimeUnit_Hr,  // hour
    ImPlotTimeUnit_Day, // day
    ImPlotTimeUnit_Mo,  // month
    ImPlotTimeUnit_Yr,  // year
    ImPlotTimeUnit_COUNT
};

enum ImPlotDateFmt_ {              // default        [ ISO 8601     ]
    ImPlotDateFmt_None = 0,
    ImPlotDateFmt_DayMo,           // 10/3           [ --10-03      ]
    ImPlotDateFmt_DayMoYr,         // 10/3/91        [ 1991-10-03   ]
    ImPlotDateFmt_MoYr,            // Oct 1991       [ 1991-10      ]
    ImPlotDateFmt_Mo,              // Oct            [ --10         ]
    ImPlotDateFmt_Yr               // 1991           [ 1991         ]
};

enum ImPlotTimeFmt_ {              // default        [ 24 Hour Clock ]
    ImPlotTimeFmt_None = 0,
    ImPlotTimeFmt_Us,              // .428 552       [ .428 552     ]
    ImPlotTimeFmt_SUs,             // :29.428 552    [ :29.428 552  ]
    ImPlotTimeFmt_SMs,             // :29.428        [ :29.428      ]
    ImPlotTimeFmt_S,               // :29            [ :29          ]
    ImPlotTimeFmt_MinSMs,          // 21:29.428      [ 21:29.428    ]
    ImPlotTimeFmt_HrMinSMs,        // 7:21:29.428pm  [ 19:21:29.428 ]
    ImPlotTimeFmt_HrMinS,          // 7:21:29pm      [ 19:21:29     ]
    ImPlotTimeFmt_HrMin,           // 7:21pm         [ 19:21        ]
    ImPlotTimeFmt_Hr               // 7pm            [ 19:00        ]
};

//-----------------------------------------------------------------------------
// [SECTION] Callbacks
//-----------------------------------------------------------------------------

typedef void (*ImPlotLocator)(ImPlotTicker& ticker, const ImPlotRange& range, float pixels, bool vertical, ImPlotFormatter formatter, void* formatter_data);

//-----------------------------------------------------------------------------
// [SECTION] Structs
//-----------------------------------------------------------------------------

// Combined date/time format spec
struct ImPlotDateTimeSpec {
    ImPlotDateTimeSpec() {}
    ImPlotDateTimeSpec(ImPlotDateFmt date_fmt, ImPlotTimeFmt time_fmt, bool use_24_hr_clk = false, bool use_iso_8601 = false) {
        Date           = date_fmt;
        Time           = time_fmt;
        UseISO8601     = use_iso_8601;
        Use24HourClock = use_24_hr_clk;
    }
    ImPlotDateFmt Date;
    ImPlotTimeFmt Time;
    bool UseISO8601;
    bool Use24HourClock;
};

// Two part timestamp struct.
struct ImPlotTime {
    time_t S;  // second part
    int    Us; // microsecond part
    ImPlotTime() { S = 0; Us = 0; }
    ImPlotTime(time_t s, int us = 0) { S  = s + us / 1000000; Us = us % 1000000; }
    void RollOver() { S  = S + Us / 1000000;  Us = Us % 1000000; }
    double ToDouble() const { return (double)S + (double)Us / 1000000.0; }
    static ImPlotTime FromDouble(double t) { return ImPlotTime((time_t)t, (int)(t * 1000000 - floor(t) * 1000000)); }
};

static inline ImPlotTime operator+(const ImPlotTime& lhs, const ImPlotTime& rhs)
{ return ImPlotTime(lhs.S + rhs.S, lhs.Us + rhs.Us); }
static inline ImPlotTime operator-(const ImPlotTime& lhs, const ImPlotTime& rhs)
{ return ImPlotTime(lhs.S - rhs.S, lhs.Us - rhs.Us); }
static inline bool operator==(const ImPlotTime& lhs, const ImPlotTime& rhs)
{ return lhs.S == rhs.S && lhs.Us == rhs.Us; }
static inline bool operator<(const ImPlotTime& lhs, const ImPlotTime& rhs)
{ return lhs.S == rhs.S ? lhs.Us < rhs.Us : lhs.S < rhs.S; }
static inline bool operator>(const ImPlotTime& lhs, const ImPlotTime& rhs)
{ return rhs < lhs; }
static inline bool operator<=(const ImPlotTime& lhs, const ImPlotTime& rhs)
{ return lhs < rhs || lhs == rhs; }
static inline bool operator>=(const ImPlotTime& lhs, const ImPlotTime& rhs)
{ return lhs > rhs || lhs == rhs; }

// Colormap data storage
struct ImPlotColormapData {
    ImVector<ImU32> Keys;
    ImVector<int>   KeyCounts;
    ImVector<int>   KeyOffsets;
    ImVector<ImU32> Tables;
    ImVector<int>   TableSizes;
    ImVector<int>   TableOffsets;
    ImGuiTextBuffer Text;
    ImVector<int>   TextOffsets;
    ImVector<bool>  Quals;
    ImGuiStorage    Map;
    int             Count;

    ImPlotColormapData() { Count = 0; }

    int Append(const char* name, const ImU32* keys, int count, bool qual) {
        if (GetIndex(name) != -1)
            return -1;
        KeyOffsets.push_back(Keys.size());
        KeyCounts.push_back(count);
        Keys.reserve(Keys.size()+count);
        for (int i = 0; i < count; ++i)
            Keys.push_back(keys[i]);
        TextOffsets.push_back(Text.size());
        Text.append(name, name + strlen(name) + 1);
        Quals.push_back(qual);
        ImGuiID id = ImHashStr(name);
        int idx = Count++;
        Map.SetInt(id,idx);
        _AppendTable(idx);
        return idx;
    }

    void _AppendTable(ImPlotColormap cmap) {
        int key_count     = GetKeyCount(cmap);
        const ImU32* keys = GetKeys(cmap);
        int off = Tables.size();
        TableOffsets.push_back(off);
        if (IsQual(cmap)) {
            Tables.reserve(key_count);
            for (int i = 0; i < key_count; ++i)
                Tables.push_back(keys[i]);
            TableSizes.push_back(key_count);
        }
        else {
            int max_size = 255 * (key_count-1) + 1;
            Tables.reserve(off + max_size);
            // ImU32 last = keys[0];
            // Tables.push_back(last);
            // int n = 1;
            for (int i = 0; i < key_count-1; ++i) {
                for (int s = 0; s < 255; ++s) {
                    ImU32 a = keys[i];
                    ImU32 b = keys[i+1];
                    ImU32 c = ImMixU32(a,b,s);
                    // if (c != last) {
                        Tables.push_back(c);
                        // last = c;
                        // n++;
                    // }
                }
            }
            ImU32 c = keys[key_count-1];
            // if (c != last) {
                Tables.push_back(c);
                // n++;
            // }
            // TableSizes.push_back(n);
            TableSizes.push_back(max_size);
        }
    }

    void RebuildTables() {
        Tables.resize(0);
        TableSizes.resize(0);
        TableOffsets.resize(0);
        for (int i = 0; i < Count; ++i)
            _AppendTable(i);
    }

    inline bool           IsQual(ImPlotColormap cmap) const                      { return Quals[cmap];                                             }
    inline const char*    GetName(ImPlotColormap cmap) const                     { return cmap < Count ? Text.Buf.Data + TextOffsets[cmap] : NULL; }
    inline ImPlotColormap GetIndex(const char* name) const                       { ImGuiID key = ImHashStr(name); return Map.GetInt(key,-1);       }

    inline const ImU32*   GetKeys(ImPlotColormap cmap) const                     { return &Keys[KeyOffsets[cmap]];                                 }
    inline int            GetKeyCount(ImPlotColormap cmap) const                 { return KeyCounts[cmap];                                         }
    inline ImU32          GetKeyColor(ImPlotColormap cmap, int idx) const        { return Keys[KeyOffsets[cmap]+idx];                              }
    inline void           SetKeyColor(ImPlotColormap cmap, int idx, ImU32 value) { Keys[KeyOffsets[cmap]+idx] = value; RebuildTables();            }

    inline const ImU32*   GetTable(ImPlotColormap cmap) const                    { return &Tables[TableOffsets[cmap]];                             }
    inline int            GetTableSize(ImPlotColormap cmap) const                { return TableSizes[cmap];                                        }
    inline ImU32          GetTableColor(ImPlotColormap cmap, int idx) const      { return Tables[TableOffsets[cmap]+idx];                          }

    inline ImU32 LerpTable(ImPlotColormap cmap, float t) const {
        int off = TableOffsets[cmap];
        int siz = TableSizes[cmap];
        int idx = Quals[cmap] ? ImClamp((int)(siz*t),0,siz-1) : (int)((siz - 1) * t + 0.5f);
        return Tables[off + idx];
    }
};

// ImPlotPoint with positive/negative error values
struct ImPlotPointError {
    double X, Y, Neg, Pos;
    ImPlotPointError(double x, double y, double neg, double pos) {
        X = x; Y = y; Neg = neg; Pos = pos;
    }
};

// Interior plot label/annotation
struct ImPlotAnnotation {
    ImVec2 Pos;
    ImVec2 Offset;
    ImU32  ColorBg;
    ImU32  ColorFg;
    int    TextOffset;
    bool   Clamp;
    ImPlotAnnotation() {
        ColorBg = ColorFg = 0;
        TextOffset = 0;
        Clamp = false;
    }
};

// Collection of plot labels
struct ImPlotAnnotationCollection {

    ImVector<ImPlotAnnotation> Annotations;
    ImGuiTextBuffer            TextBuffer;
    int                        Size;

    ImPlotAnnotationCollection() { Reset(); }

    void AppendV(const ImVec2& pos, const ImVec2& off, ImU32 bg, ImU32 fg, bool clamp, const char* fmt,  va_list args) IM_FMTLIST(7) {
        ImPlotAnnotation an;
        an.Pos = pos; an.Offset = off;
        an.ColorBg = bg; an.ColorFg = fg;
        an.TextOffset = TextBuffer.size();
        an.Clamp = clamp;
        Annotations.push_back(an);
        TextBuffer.appendfv(fmt, args);
        const char nul[] = "";
        TextBuffer.append(nul,nul+1);
        Size++;
    }

    void Append(const ImVec2& pos, const ImVec2& off, ImU32 bg, ImU32 fg, bool clamp, const char* fmt,  ...) IM_FMTARGS(7) {
        va_list args;
        va_start(args, fmt);
        AppendV(pos, off, bg, fg, clamp, fmt, args);
        va_end(args);
    }

    const char* GetText(int idx) {
        return TextBuffer.Buf.Data + Annotations[idx].TextOffset;
    }

    void Reset() {
        Annotations.shrink(0);
        TextBuffer.Buf.shrink(0);
        Size = 0;
    }
};

struct ImPlotTag {
    ImAxis Axis;
    double Value;
    ImU32  ColorBg;
    ImU32  ColorFg;
    int    TextOffset;
};

struct ImPlotTagCollection {

    ImVector<ImPlotTag> Tags;
    ImGuiTextBuffer     TextBuffer;
    int                 Size;

    ImPlotTagCollection() { Reset(); }

    void AppendV(ImAxis axis, double value, ImU32 bg, ImU32 fg, const char* fmt, va_list args) IM_FMTLIST(6) {
        ImPlotTag tag;
        tag.Axis = axis;
        tag.Value = value;
        tag.ColorBg = bg;
        tag.ColorFg = fg;
        tag.TextOffset = TextBuffer.size();
        Tags.push_back(tag);
        TextBuffer.appendfv(fmt, args);
        const char nul[] = "";
        TextBuffer.append(nul,nul+1);
        Size++;
    }

    void Append(ImAxis axis, double value, ImU32 bg, ImU32 fg, const char* fmt, ...) IM_FMTARGS(6) {
        va_list args;
        va_start(args, fmt);
        AppendV(axis, value, bg, fg, fmt, args);
        va_end(args);
    }

    const char* GetText(int idx) {
        return TextBuffer.Buf.Data + Tags[idx].TextOffset;
    }

    void Reset() {
        Tags.shrink(0);
        TextBuffer.Buf.shrink(0);
        Size = 0;
    }
};

// Tick mark info
struct ImPlotTick
{
    double PlotPos;
    float  PixelPos;
    ImVec2 LabelSize;
    int    TextOffset;
    bool   Major;
    bool   ShowLabel;
    int    Level;
    int    Idx;

    ImPlotTick(double value, bool major, int level, bool show_label) {
        PixelPos     = 0;
        PlotPos      = value;
        Major        = major;
        ShowLabel    = show_label;
        Level        = level;
        TextOffset   = -1;
    }
};

// Collection of ticks
struct ImPlotTicker {
    ImVector<ImPlotTick> Ticks;
    ImGuiTextBuffer      TextBuffer;
    ImVec2               MaxSize;
    ImVec2               LateSize;
    int                  Levels;

    ImPlotTicker() {
        Reset();
    }

    ImPlotTick& AddTick(double value, bool major, int level, bool show_label, const char* label) {
        ImPlotTick tick(value, major, level, show_label);
        if (show_label && label != NULL) {
            tick.TextOffset = TextBuffer.size();
            TextBuffer.append(label, label + strlen(label) + 1);
            tick.LabelSize = ImGui::CalcTextSize(TextBuffer.Buf.Data + tick.TextOffset);
        }
        return AddTick(tick);
    }

    ImPlotTick& AddTick(double value, bool major, int level, bool show_label, ImPlotFormatter formatter, void* data) {
        ImPlotTick tick(value, major, level, show_label);
        if (show_label && formatter != NULL) {
            char buff[IMPLOT_LABEL_MAX_SIZE];
            tick.TextOffset = TextBuffer.size();
            formatter(tick.PlotPos, buff, sizeof(buff), data);
            TextBuffer.append(buff, buff + strlen(buff) + 1);
            tick.LabelSize = ImGui::CalcTextSize(TextBuffer.Buf.Data + tick.TextOffset);
        }
        return AddTick(tick);
    }

    inline ImPlotTick& AddTick(ImPlotTick tick) {
        if (tick.ShowLabel) {
            MaxSize.x     =  tick.LabelSize.x > MaxSize.x ? tick.LabelSize.x : MaxSize.x;
            MaxSize.y     =  tick.LabelSize.y > MaxSize.y ? tick.LabelSize.y : MaxSize.y;
        }
        tick.Idx = Ticks.size();
        Ticks.push_back(tick);
        return Ticks.back();
    }

    const char* GetText(int idx) const {
        return TextBuffer.Buf.Data + Ticks[idx].TextOffset;
    }

    const char* GetText(const ImPlotTick& tick) {
        return GetText(tick.Idx);
    }

    void OverrideSizeLate(const ImVec2& size) {
        LateSize.x = size.x > LateSize.x ? size.x : LateSize.x;
        LateSize.y = size.y > LateSize.y ? size.y : LateSize.y;
    }

    void Reset() {
        Ticks.shrink(0);
        TextBuffer.Buf.shrink(0);
        MaxSize = LateSize;
        LateSize = ImVec2(0,0);
        Levels = 1;
    }

    int TickCount() const {
        return Ticks.Size;
    }
};

// Axis state information that must persist after EndPlot
struct ImPlotAxis
{
    ImGuiID              ID;
    ImPlotAxisFlags      Flags;
    ImPlotAxisFlags      PreviousFlags;
    ImPlotRange          Range;
    ImPlotCond           RangeCond;
    ImPlotScale          Scale;
    ImPlotRange          FitExtents;
    ImPlotAxis*          OrthoAxis;
    ImPlotRange          ConstraintRange;
    ImPlotRange          ConstraintZoom;

    ImPlotTicker         Ticker;
    ImPlotFormatter      Formatter;
    void*                FormatterData;
    char                 FormatSpec[16];
    ImPlotLocator        Locator;

    double*              LinkedMin;
    double*              LinkedMax;

    int                  PickerLevel;
    ImPlotTime           PickerTimeMin, PickerTimeMax;

    ImPlotTransform      TransformForward;
    ImPlotTransform      TransformInverse;
    void*                TransformData;
    float                PixelMin, PixelMax;
    double               ScaleMin, ScaleMax;
    double               ScaleToPixel;
    float                Datum1, Datum2;

    ImRect               HoverRect;
    int                  LabelOffset;
    ImU32                ColorMaj, ColorMin, ColorTick, ColorTxt, ColorBg, ColorHov, ColorAct, ColorHiLi;

    bool                 Enabled;
    bool                 Vertical;
    bool                 FitThisFrame;
    bool                 HasRange;
    bool                 HasFormatSpec;
    bool                 ShowDefaultTicks;
    bool                 Hovered;
    bool                 Held;

    ImPlotAxis() {
        ID               = 0;
        Flags            = PreviousFlags = ImPlotAxisFlags_None;
        Range.Min        = 0;
        Range.Max        = 1;
        Scale            = ImPlotScale_Linear;
        TransformForward = TransformInverse = NULL;
        TransformData    = NULL;
        FitExtents.Min   = HUGE_VAL;
        FitExtents.Max   = -HUGE_VAL;
        OrthoAxis        = NULL;
        ConstraintRange  = ImPlotRange(-INFINITY,INFINITY);
        ConstraintZoom   = ImPlotRange(DBL_MIN,INFINITY);
        LinkedMin        = LinkedMax = NULL;
        PickerLevel      = 0;
        Datum1           = Datum2 = 0;
        PixelMin         = PixelMax = 0;
        LabelOffset      = -1;
        ColorMaj         = ColorMin = ColorTick = ColorTxt = ColorBg = ColorHov = ColorAct = 0;
        ColorHiLi        = IM_COL32_BLACK_TRANS;
        Formatter        = NULL;
        FormatterData    = NULL;
        Locator          = NULL;
        Enabled          = Hovered = Held = FitThisFrame = HasRange = HasFormatSpec = false;
        ShowDefaultTicks = true;
    }

    inline void Reset() {
        Enabled          = false;
        Scale            = ImPlotScale_Linear;
        TransformForward = TransformInverse = NULL;
        TransformData    = NULL;
        LabelOffset      = -1;
        HasFormatSpec    = false;
        Formatter        = NULL;
        FormatterData    = NULL;
        Locator          = NULL;
        ShowDefaultTicks = true;
        FitThisFrame     = false;
        FitExtents.Min   = HUGE_VAL;
        FitExtents.Max   = -HUGE_VAL;
        OrthoAxis        = NULL;
        ConstraintRange  = ImPlotRange(-INFINITY,INFINITY);
        ConstraintZoom   = ImPlotRange(DBL_MIN,INFINITY);
        Ticker.Reset();
    }

    inline bool SetMin(double _min, bool force=false) {
        if (!force && IsLockedMin())
            return false;
        _min = ImConstrainNan(ImConstrainInf(_min));
        if (_min < ConstraintRange.Min)
            _min = ConstraintRange.Min;
        double z = Range.Max - _min;
        if (z < ConstraintZoom.Min)
            _min = Range.Max - ConstraintZoom.Min;
        if (z > ConstraintZoom.Max)
            _min = Range.Max - ConstraintZoom.Max;
        if (_min >= Range.Max)
            return false;
        Range.Min = _min;
        PickerTimeMin = ImPlotTime::FromDouble(Range.Min);
        UpdateTransformCache();
        return true;
    };

    inline bool SetMax(double _max, bool force=false) {
        if (!force && IsLockedMax())
            return false;
        _max = ImConstrainNan(ImConstrainInf(_max));
        if (_max > ConstraintRange.Max)
            _max = ConstraintRange.Max;
        double z = _max - Range.Min;
        if (z < ConstraintZoom.Min)
            _max = Range.Min + ConstraintZoom.Min;
        if (z > ConstraintZoom.Max)
            _max = Range.Min + ConstraintZoom.Max;
        if (_max <= Range.Min)
            return false;
        Range.Max = _max;
        PickerTimeMax = ImPlotTime::FromDouble(Range.Max);
        UpdateTransformCache();
        return true;
    };

    inline void SetRange(double v1, double v2) {
        Range.Min = ImMin(v1,v2);
        Range.Max = ImMax(v1,v2);
        Constrain();
        PickerTimeMin = ImPlotTime::FromDouble(Range.Min);
        PickerTimeMax = ImPlotTime::FromDouble(Range.Max);
        UpdateTransformCache();
    }

    inline void SetRange(const ImPlotRange& range) {
        SetRange(range.Min, range.Max);
    }

    inline void SetAspect(double unit_per_pix) {
        double new_size = unit_per_pix * PixelSize();
        double delta    = (new_size - Range.Size()) * 0.5;
        if (IsLocked())
            return;
        else if (IsLockedMin() && !IsLockedMax())
            SetRange(Range.Min, Range.Max  + 2*delta);
        else if (!IsLockedMin() && IsLockedMax())
            SetRange(Range.Min - 2*delta, Range.Max);
        else
            SetRange(Range.Min - delta, Range.Max + delta);
    }

    inline float PixelSize() const { return ImAbs(PixelMax - PixelMin); }

    inline double GetAspect() const { return Range.Size() / PixelSize(); }

    inline void Constrain() {
        Range.Min = ImConstrainNan(ImConstrainInf(Range.Min));
        Range.Max = ImConstrainNan(ImConstrainInf(Range.Max));
        if (Range.Min < ConstraintRange.Min)
            Range.Min = ConstraintRange.Min;
        if (Range.Max > ConstraintRange.Max)
            Range.Max = ConstraintRange.Max;
        double z = Range.Size();
        if (z < ConstraintZoom.Min) {
            double delta = (ConstraintZoom.Min - z) * 0.5;
            Range.Min -= delta;
            Range.Max += delta;
        }
        if (z > ConstraintZoom.Max) {
            double delta = (z - ConstraintZoom.Max) * 0.5;
            Range.Min += delta;
            Range.Max -= delta;
        }
        if (Range.Max <= Range.Min)
            Range.Max = Range.Min + DBL_EPSILON;
    }

    inline void UpdateTransformCache() {
        ScaleToPixel = (PixelMax - PixelMin) / Range.Size();
        if (TransformForward != NULL) {
            ScaleMin = TransformForward(Range.Min, TransformData);
            ScaleMax = TransformForward(Range.Max, TransformData);
        }
        else {
            ScaleMin = Range.Min;
            ScaleMax = Range.Max;
        }
    }

    inline float PlotToPixels(double plt) const {
        if (TransformForward != NULL) {
            double s = TransformForward(plt, TransformData);
            double t = (s - ScaleMin) / (ScaleMax - ScaleMin);
            plt      = Range.Min + Range.Size() * t;
        }
        return (float)(PixelMin + ScaleToPixel * (plt - Range.Min));
    }


    inline double PixelsToPlot(float pix) const {
        double plt = (pix - PixelMin) / ScaleToPixel + Range.Min;
        if (TransformInverse != NULL) {
            double t = (plt - Range.Min) / Range.Size();
            double s = t * (ScaleMax - ScaleMin) + ScaleMin;
            plt = TransformInverse(s, TransformData);
        }
        return plt;
    }

    inline void ExtendFit(double v) {
        if (!ImNanOrInf(v) && v >= ConstraintRange.Min && v <= ConstraintRange.Max) {
            FitExtents.Min = v < FitExtents.Min ? v : FitExtents.Min;
            FitExtents.Max = v > FitExtents.Max ? v : FitExtents.Max;
        }
    }

    inline void ExtendFitWith(ImPlotAxis& alt, double v, double v_alt) {
        if (ImHasFlag(Flags, ImPlotAxisFlags_RangeFit) && !alt.Range.Contains(v_alt))
            return;
        if (!ImNanOrInf(v) && v >= ConstraintRange.Min && v <= ConstraintRange.Max) {
            FitExtents.Min = v < FitExtents.Min ? v : FitExtents.Min;
            FitExtents.Max = v > FitExtents.Max ? v : FitExtents.Max;
        }
    }

    inline void ApplyFit(float padding) {
        const double ext_size = FitExtents.Size() * 0.5;
        FitExtents.Min -= ext_size * padding;
        FitExtents.Max += ext_size * padding;
        if (!IsLockedMin() && !ImNanOrInf(FitExtents.Min))
            Range.Min = FitExtents.Min;
        if (!IsLockedMax() && !ImNanOrInf(FitExtents.Max))
            Range.Max = FitExtents.Max;
        if (ImAlmostEqual(Range.Min, Range.Max))  {
            Range.Max += 0.5;
            Range.Min -= 0.5;
        }
        Constrain();
        UpdateTransformCache();
    }

    inline bool HasLabel()          const { return LabelOffset != -1 && !ImHasFlag(Flags, ImPlotAxisFlags_NoLabel);                          }
    inline bool HasGridLines()      const { return !ImHasFlag(Flags, ImPlotAxisFlags_NoGridLines);                                           }
    inline bool HasTickLabels()     const { return !ImHasFlag(Flags, ImPlotAxisFlags_NoTickLabels);                                          }
    inline bool HasTickMarks()      const { return !ImHasFlag(Flags, ImPlotAxisFlags_NoTickMarks);                                           }
    inline bool WillRender()        const { return Enabled && (HasGridLines() || HasTickLabels() || HasTickMarks());                         }
    inline bool IsOpposite()        const { return ImHasFlag(Flags, ImPlotAxisFlags_Opposite);                                               }
    inline bool IsInverted()        const { return ImHasFlag(Flags, ImPlotAxisFlags_Invert);                                                 }
    inline bool IsForeground()      const { return ImHasFlag(Flags, ImPlotAxisFlags_Foreground);                                             }
    inline bool IsAutoFitting()     const { return ImHasFlag(Flags, ImPlotAxisFlags_AutoFit);                                                }
    inline bool CanInitFit()        const { return !ImHasFlag(Flags, ImPlotAxisFlags_NoInitialFit) && !HasRange && !LinkedMin && !LinkedMax; }
    inline bool IsRangeLocked()     const { return HasRange && RangeCond == ImPlotCond_Always;                                               }
    inline bool IsLockedMin()       const { return !Enabled || IsRangeLocked() || ImHasFlag(Flags, ImPlotAxisFlags_LockMin);                 }
    inline bool IsLockedMax()       const { return !Enabled || IsRangeLocked() || ImHasFlag(Flags, ImPlotAxisFlags_LockMax);                 }
    inline bool IsLocked()          const { return IsLockedMin() && IsLockedMax();                                                           }
    inline bool IsInputLockedMin()  const { return IsLockedMin() || IsAutoFitting();                                                         }
    inline bool IsInputLockedMax()  const { return IsLockedMax() || IsAutoFitting();                                                         }
    inline bool IsInputLocked()     const { return IsLocked()    || IsAutoFitting();                                                         }
    inline bool HasMenus()          const { return !ImHasFlag(Flags, ImPlotAxisFlags_NoMenus);                                               }

    inline bool IsPanLocked(bool increasing) {
        if (ImHasFlag(Flags, ImPlotAxisFlags_PanStretch)) {
            return IsInputLocked();
        }
        else {
            if (IsLockedMin() || IsLockedMax() || IsAutoFitting())
                return false;
            if (increasing)
                return Range.Max == ConstraintRange.Max;
            else
                return Range.Min == ConstraintRange.Min;
        }
    }

    void PushLinks() {
        if (LinkedMin) { *LinkedMin = Range.Min; }
        if (LinkedMax) { *LinkedMax = Range.Max; }
    }

    void PullLinks() {
        if (LinkedMin) { SetMin(*LinkedMin,true); }
        if (LinkedMax) { SetMax(*LinkedMax,true); }
    }
};

// Align plots group data
struct ImPlotAlignmentData {
    bool  Vertical;
    float PadA;
    float PadB;
    float PadAMax;
    float PadBMax;
    ImPlotAlignmentData() {
        Vertical    = true;
        PadA = PadB = PadAMax = PadBMax = 0;
    }
    void Begin() { PadAMax = PadBMax = 0; }
    void Update(float& pad_a, float& pad_b, float& delta_a, float& delta_b) {
        float bak_a = pad_a; float bak_b = pad_b;
        if (PadAMax < pad_a) { PadAMax = pad_a; }
        if (PadBMax < pad_b) { PadBMax = pad_b; }
        if (pad_a < PadA)    { pad_a = PadA; delta_a = pad_a - bak_a; } else { delta_a = 0; }
        if (pad_b < PadB)    { pad_b = PadB; delta_b = pad_b - bak_b; } else { delta_b = 0; }
    }
    void End()   { PadA = PadAMax; PadB = PadBMax;      }
    void Reset() { PadA = PadB = PadAMax = PadBMax = 0; }
};

// State information for Plot items
struct ImPlotItem
{
    ImGuiID      ID;
    ImU32        Color;
    ImRect       LegendHoverRect;
    int          NameOffset;
    bool         Show;
    bool         LegendHovered;
    bool         SeenThisFrame;

    ImPlotItem() {
        ID            = 0;
        Color         = IM_COL32_WHITE;
        NameOffset    = -1;
        Show          = true;
        SeenThisFrame = false;
        LegendHovered = false;
    }

    ~ImPlotItem() { ID = 0; }
};

// Holds Legend state
struct ImPlotLegend
{
    ImPlotLegendFlags Flags;
    ImPlotLegendFlags PreviousFlags;
    ImPlotLocation    Location;
    ImPlotLocation    PreviousLocation;
    ImVector<int>     Indices;
    ImGuiTextBuffer   Labels;
    ImRect            Rect;
    bool              Hovered;
    bool              Held;
    bool              CanGoInside;

    ImPlotLegend() {
        Flags        = PreviousFlags = ImPlotLegendFlags_None;
        CanGoInside  = true;
        Hovered      = Held = false;
        Location     = PreviousLocation = ImPlotLocation_NorthWest;
    }

    void Reset() { Indices.shrink(0); Labels.Buf.shrink(0); }
};

// Holds Items and Legend data
struct ImPlotItemGroup
{
    ImGuiID            ID;
    ImPlotLegend       Legend;
    ImPool<ImPlotItem> ItemPool;
    int                ColormapIdx;

    ImPlotItemGroup() { ID = 0; ColormapIdx = 0; }

    int         GetItemCount() const             { return ItemPool.GetBufSize();                                 }
    ImGuiID     GetItemID(const char*  label_id) { return ImGui::GetID(label_id); /* GetIDWithSeed */            }
    ImPlotItem* GetItem(ImGuiID id)              { return ItemPool.GetByKey(id);                                 }
    ImPlotItem* GetItem(const char* label_id)    { return GetItem(GetItemID(label_id));                          }
    ImPlotItem* GetOrAddItem(ImGuiID id)         { return ItemPool.GetOrAddByKey(id);                            }
    ImPlotItem* GetItemByIndex(int i)            { return ItemPool.GetByIndex(i);                                }
    int         GetItemIndex(ImPlotItem* item)   { return ItemPool.GetIndex(item);                               }
    int         GetLegendCount() const           { return Legend.Indices.size();                                 }
    ImPlotItem* GetLegendItem(int i)             { return ItemPool.GetByIndex(Legend.Indices[i]);                }
    const char* GetLegendLabel(int i)            { return Legend.Labels.Buf.Data + GetLegendItem(i)->NameOffset; }
    void        Reset()                          { ItemPool.Clear(); Legend.Reset(); ColormapIdx = 0;            }
};

// Holds Plot state information that must persist after EndPlot
struct ImPlotPlot
{
    ImGuiID              ID;
    ImPlotFlags          Flags;
    ImPlotFlags          PreviousFlags;
    ImPlotLocation       MouseTextLocation;
    ImPlotMouseTextFlags MouseTextFlags;
    ImPlotAxis           Axes[ImAxis_COUNT];
    ImGuiTextBuffer      TextBuffer;
    ImPlotItemGroup      Items;
    ImAxis               CurrentX;
    ImAxis               CurrentY;
    ImRect               FrameRect;
    ImRect               CanvasRect;
    ImRect               PlotRect;
    ImRect               AxesRect;
    ImRect               SelectRect;
    ImVec2               SelectStart;
    int                  TitleOffset;
    bool                 JustCreated;
    bool                 Initialized;
    bool                 SetupLocked;
    bool                 FitThisFrame;
    bool                 Hovered;
    bool                 Held;
    bool                 Selecting;
    bool                 Selected;
    bool                 ContextLocked;

    ImPlotPlot() {
        Flags             = PreviousFlags = ImPlotFlags_None;
        for (int i = 0; i < IMPLOT_NUM_X_AXES; ++i)
            XAxis(i).Vertical = false;
        for (int i = 0; i < IMPLOT_NUM_Y_AXES; ++i)
            YAxis(i).Vertical = true;
        SelectStart       = ImVec2(0,0);
        CurrentX          = ImAxis_X1;
        CurrentY          = ImAxis_Y1;
        MouseTextLocation  = ImPlotLocation_South | ImPlotLocation_East;
        MouseTextFlags     = ImPlotMouseTextFlags_None;
        TitleOffset       = -1;
        JustCreated       = true;
        Initialized = SetupLocked = FitThisFrame = false;
        Hovered = Held = Selected = Selecting = ContextLocked = false;
    }

    inline bool IsInputLocked() const {
        for (int i = 0; i < IMPLOT_NUM_X_AXES; ++i) {
            if (!XAxis(i).IsInputLocked())
                return false;
        }
        for (int i = 0; i < IMPLOT_NUM_Y_AXES; ++i) {
            if (!YAxis(i).IsInputLocked())
                return false;
        }
        return true;
    }

    inline void ClearTextBuffer() { TextBuffer.Buf.shrink(0); }

    inline void SetTitle(const char* title) {
        if (title && ImGui::FindRenderedTextEnd(title, NULL) != title) {
            TitleOffset = TextBuffer.size();
            TextBuffer.append(title, title + strlen(title) + 1);
        }
        else {
            TitleOffset = -1;
        }
    }
    inline bool HasTitle() const { return TitleOffset != -1 && !ImHasFlag(Flags, ImPlotFlags_NoTitle); }
    inline const char* GetTitle() const { return TextBuffer.Buf.Data + TitleOffset; }

    inline       ImPlotAxis& XAxis(int i)       { return Axes[ImAxis_X1 + i]; }
    inline const ImPlotAxis& XAxis(int i) const { return Axes[ImAxis_X1 + i]; }
    inline       ImPlotAxis& YAxis(int i)       { return Axes[ImAxis_Y1 + i]; }
    inline const ImPlotAxis& YAxis(int i) const { return Axes[ImAxis_Y1 + i]; }

    inline int EnabledAxesX() {
        int cnt = 0;
        for (int i = 0; i < IMPLOT_NUM_X_AXES; ++i)
            cnt += XAxis(i).Enabled;
        return cnt;
    }

    inline int EnabledAxesY() {
        int cnt = 0;
        for (int i = 0; i < IMPLOT_NUM_Y_AXES; ++i)
            cnt += YAxis(i).Enabled;
        return cnt;
    }

    inline void SetAxisLabel(ImPlotAxis& axis, const char* label) {
        if (label && ImGui::FindRenderedTextEnd(label, NULL) != label) {
            axis.LabelOffset = TextBuffer.size();
            TextBuffer.append(label, label + strlen(label) + 1);
        }
        else {
            axis.LabelOffset = -1;
        }
    }

    inline const char* GetAxisLabel(const ImPlotAxis& axis) const { return TextBuffer.Buf.Data + axis.LabelOffset; }
};

// Holds subplot data that must persist after EndSubplot
struct ImPlotSubplot {
    ImGuiID                       ID;
    ImPlotSubplotFlags            Flags;
    ImPlotSubplotFlags            PreviousFlags;
    ImPlotItemGroup               Items;
    int                           Rows;
    int                           Cols;
    int                           CurrentIdx;
    ImRect                        FrameRect;
    ImRect                        GridRect;
    ImVec2                        CellSize;
    ImVector<ImPlotAlignmentData> RowAlignmentData;
    ImVector<ImPlotAlignmentData> ColAlignmentData;
    ImVector<float>               RowRatios;
    ImVector<float>               ColRatios;
    ImVector<ImPlotRange>         RowLinkData;
    ImVector<ImPlotRange>         ColLinkData;
    float                         TempSizes[2];
    bool                          FrameHovered;
    bool                          HasTitle;

    ImPlotSubplot() {
        ID                          = 0;
        Flags = PreviousFlags       = ImPlotSubplotFlags_None;
        Rows = Cols = CurrentIdx    = 0;
        FrameHovered                = false;
        Items.Legend.Location       = ImPlotLocation_North;
        Items.Legend.Flags          = ImPlotLegendFlags_Horizontal|ImPlotLegendFlags_Outside;
        Items.Legend.CanGoInside    = false;
        TempSizes[0] = TempSizes[1] = 0;
        FrameHovered                = false;
        HasTitle                    = false;
    }
};

// Temporary data storage for upcoming plot
struct ImPlotNextPlotData
{
    ImPlotCond  RangeCond[ImAxis_COUNT];
    ImPlotRange Range[ImAxis_COUNT];
    bool        HasRange[ImAxis_COUNT];
    bool        Fit[ImAxis_COUNT];
    double*     LinkedMin[ImAxis_COUNT];
    double*     LinkedMax[ImAxis_COUNT];

    ImPlotNextPlotData() { Reset(); }

    void Reset() {
        for (int i = 0; i < ImAxis_COUNT; ++i) {
            HasRange[i]                 = false;
            Fit[i]                      = false;
            LinkedMin[i] = LinkedMax[i] = NULL;
        }
    }

};

// Temporary data storage for upcoming item
struct ImPlotNextItemData {
    ImVec4          Colors[5]; // ImPlotCol_Line, ImPlotCol_Fill, ImPlotCol_MarkerOutline, ImPlotCol_MarkerFill, ImPlotCol_ErrorBar
    float           LineWeight;
    ImPlotMarker    Marker;
    float           MarkerSize;
    float           MarkerWeight;
    float           FillAlpha;
    float           ErrorBarSize;
    float           ErrorBarWeight;
    float           DigitalBitHeight;
    float           DigitalBitGap;
    bool            RenderLine;
    bool            RenderFill;
    bool            RenderMarkerLine;
    bool            RenderMarkerFill;
    bool            HasHidden;
    bool            Hidden;
    ImPlotCond      HiddenCond;
    ImPlotNextItemData() { Reset(); }
    void Reset() {
        for (int i = 0; i < 5; ++i)
            Colors[i] = IMPLOT_AUTO_COL;
        LineWeight    = MarkerSize = MarkerWeight = FillAlpha = ErrorBarSize = ErrorBarWeight = DigitalBitHeight = DigitalBitGap = IMPLOT_AUTO;
        Marker        = IMPLOT_AUTO;
        HasHidden     = Hidden = false;
    }
};

// Holds state information that must persist between calls to BeginPlot()/EndPlot()
struct ImPlotContext {
    // Plot States
    ImPool<ImPlotPlot>    Plots;
    ImPool<ImPlotSubplot> Subplots;
    ImPlotPlot*           CurrentPlot;
    ImPlotSubplot*        CurrentSubplot;
    ImPlotItemGroup*      CurrentItems;
    ImPlotItem*           CurrentItem;
    ImPlotItem*           PreviousItem;

    // Tick Marks and Labels
    ImPlotTicker CTicker;

    // Annotation and Tabs
    ImPlotAnnotationCollection Annotations;
    ImPlotTagCollection        Tags;

    // Flags
    bool ChildWindowMade;

    // Style and Colormaps
    ImPlotStyle                 Style;
    ImVector<ImGuiColorMod>     ColorModifiers;
    ImVector<ImGuiStyleMod>     StyleModifiers;
    ImPlotColormapData          ColormapData;
    ImVector<ImPlotColormap>    ColormapModifiers;

    // Time
    tm Tm;

    // Temp data for general use
    ImVector<double>   TempDouble1, TempDouble2;
    ImVector<int>      TempInt1;

    // Misc
    int                DigitalPlotItemCnt;
    int                DigitalPlotOffset;
    ImPlotNextPlotData NextPlotData;
    ImPlotNextItemData NextItemData;
    ImPlotInputMap     InputMap;
    bool               OpenContextThisFrame;
    ImGuiTextBuffer    MousePosStringBuilder;
    ImPlotItemGroup*   SortItems;

    // Align plots
    ImPool<ImPlotAlignmentData> AlignmentData;
    ImPlotAlignmentData*        CurrentAlignmentH;
    ImPlotAlignmentData*        CurrentAlignmentV;
};

//-----------------------------------------------------------------------------
// [SECTION] Internal API
// No guarantee of forward compatibility here!
//-----------------------------------------------------------------------------

namespace ImPlot {

//-----------------------------------------------------------------------------
// [SECTION] Context Utils
//-----------------------------------------------------------------------------

// Initializes an ImPlotContext
IMPLOT_API void Initialize(ImPlotContext* ctx);
// Resets an ImPlot context for the next call to BeginPlot
IMPLOT_API void ResetCtxForNextPlot(ImPlotContext* ctx);
// Resets an ImPlot context for the next call to BeginAlignedPlots
IMPLOT_API void ResetCtxForNextAlignedPlots(ImPlotContext* ctx);
// Resets an ImPlot context for the next call to BeginSubplot
IMPLOT_API void ResetCtxForNextSubplot(ImPlotContext* ctx);

//-----------------------------------------------------------------------------
// [SECTION] Plot Utils
//-----------------------------------------------------------------------------

// Gets a plot from the current ImPlotContext
IMPLOT_API ImPlotPlot* GetPlot(const char* title);
// Gets the current plot from the current ImPlotContext
IMPLOT_API ImPlotPlot* GetCurrentPlot();
// Busts the cache for every plot in the current context
IMPLOT_API void BustPlotCache();

// Shows a plot's context menu.
IMPLOT_API void ShowPlotContextMenu(ImPlotPlot& plot);

//-----------------------------------------------------------------------------
// [SECTION] Setup Utils
//-----------------------------------------------------------------------------

// Lock Setup and call SetupFinish if necessary.
static inline void SetupLock() {
    if (!GImPlot->CurrentPlot->SetupLocked)
        SetupFinish();
    GImPlot->CurrentPlot->SetupLocked = true;
}

//-----------------------------------------------------------------------------
// [SECTION] Subplot Utils
//-----------------------------------------------------------------------------

// Advances to next subplot
IMPLOT_API void SubplotNextCell();

// Shows a subplot's context menu.
IMPLOT_API void ShowSubplotsContextMenu(ImPlotSubplot& subplot);

//-----------------------------------------------------------------------------
// [SECTION] Item Utils
//-----------------------------------------------------------------------------

// Begins a new item. Returns false if the item should not be plotted. Pushes PlotClipRect.
IMPLOT_API bool BeginItem(const char* label_id, ImPlotItemFlags flags=0, ImPlotCol recolor_from=IMPLOT_AUTO);

// Same as above but with fitting functionality.
template <typename _Fitter>
bool BeginItemEx(const char* label_id, const _Fitter& fitter, ImPlotItemFlags flags=0, ImPlotCol recolor_from=IMPLOT_AUTO) {
    if (BeginItem(label_id, flags, recolor_from)) {
        ImPlotPlot& plot = *GetCurrentPlot();
        if (plot.FitThisFrame && !ImHasFlag(flags, ImPlotItemFlags_NoFit))
            fitter.Fit(plot.Axes[plot.CurrentX], plot.Axes[plot.CurrentY]);
        return true;
    }
    return false;
}

// Ends an item (call only if BeginItem returns true). Pops PlotClipRect.
IMPLOT_API void EndItem();

// Register or get an existing item from the current plot.
IMPLOT_API ImPlotItem* RegisterOrGetItem(const char* label_id, ImPlotItemFlags flags, bool* just_created = NULL);
// Get a plot item from the current plot.
IMPLOT_API ImPlotItem* GetItem(const char* label_id);
// Gets the current item.
IMPLOT_API ImPlotItem* GetCurrentItem();
// Busts the cache for every item for every plot in the current context.
IMPLOT_API void BustItemCache();

//-----------------------------------------------------------------------------
// [SECTION] Axis Utils
//-----------------------------------------------------------------------------

// Returns true if any enabled axis is locked from user input.
static inline bool AnyAxesInputLocked(ImPlotAxis* axes, int count) {
    for (int i = 0; i < count; ++i) {
        if (axes[i].Enabled && axes[i].IsInputLocked())
            return true;
    }
    return false;
}

// Returns true if all enabled axes are locked from user input.
static inline bool AllAxesInputLocked(ImPlotAxis* axes, int count) {
    for (int i = 0; i < count; ++i) {
        if (axes[i].Enabled && !axes[i].IsInputLocked())
            return false;
    }
    return true;
}

static inline bool AnyAxesHeld(ImPlotAxis* axes, int count) {
    for (int i = 0; i < count; ++i) {
        if (axes[i].Enabled && axes[i].Held)
            return true;
    }
    return false;
}

static inline bool AnyAxesHovered(ImPlotAxis* axes, int count) {
    for (int i = 0; i < count; ++i) {
        if (axes[i].Enabled && axes[i].Hovered)
            return true;
    }
    return false;
}

// Returns true if the user has requested data to be fit.
static inline bool FitThisFrame() {
    return GImPlot->CurrentPlot->FitThisFrame;
}

// Extends the current plot's axes so that it encompasses a vertical line at x
static inline void FitPointX(double x) {
    ImPlotPlot& plot   = *GetCurrentPlot();
    ImPlotAxis& x_axis = plot.Axes[plot.CurrentX];
    x_axis.ExtendFit(x);
}

// Extends the current plot's axes so that it encompasses a horizontal line at y
static inline void FitPointY(double y) {
    ImPlotPlot& plot   = *GetCurrentPlot();
    ImPlotAxis& y_axis = plot.Axes[plot.CurrentY];
    y_axis.ExtendFit(y);
}

// Extends the current plot's axes so that it encompasses point p
static inline void FitPoint(const ImPlotPoint& p) {
    ImPlotPlot& plot   = *GetCurrentPlot();
    ImPlotAxis& x_axis = plot.Axes[plot.CurrentX];
    ImPlotAxis& y_axis = plot.Axes[plot.CurrentY];
    x_axis.ExtendFitWith(y_axis, p.x, p.y);
    y_axis.ExtendFitWith(x_axis, p.y, p.x);
}

// Returns true if two ranges overlap
static inline bool RangesOverlap(const ImPlotRange& r1, const ImPlotRange& r2)
{ return r1.Min <= r2.Max && r2.Min <= r1.Max; }

// Shows an axis's context menu.
IMPLOT_API void ShowAxisContextMenu(ImPlotAxis& axis, ImPlotAxis* equal_axis, bool time_allowed = false);

//-----------------------------------------------------------------------------
// [SECTION] Legend Utils
//-----------------------------------------------------------------------------

// Gets the position of an inner rect that is located inside of an outer rect according to an ImPlotLocation and padding amount.
IMPLOT_API ImVec2 GetLocationPos(const ImRect& outer_rect, const ImVec2& inner_size, ImPlotLocation location, const ImVec2& pad = ImVec2(0,0));
// Calculates the bounding box size of a legend
IMPLOT_API ImVec2 CalcLegendSize(ImPlotItemGroup& items, const ImVec2& pad, const ImVec2& spacing, bool vertical);
// Renders legend entries into a bounding box
IMPLOT_API bool ShowLegendEntries(ImPlotItemGroup& items, const ImRect& legend_bb, bool interactable, const ImVec2& pad, const ImVec2& spacing, bool vertical, ImDrawList& DrawList);
// Shows an alternate legend for the plot identified by #title_id, outside of the plot frame (can be called before or after of Begin/EndPlot but must occur in the same ImGui window!).
IMPLOT_API void ShowAltLegend(const char* title_id, bool vertical = true, const ImVec2 size = ImVec2(0,0), bool interactable = true);
// Shows an legends's context menu.
IMPLOT_API bool ShowLegendContextMenu(ImPlotLegend& legend, bool visible);

//-----------------------------------------------------------------------------
// [SECTION] Label Utils
//-----------------------------------------------------------------------------

// Create a a string label for a an axis value
IMPLOT_API void LabelAxisValue(const ImPlotAxis& axis, double value, char* buff, int size, bool round = false);

//-----------------------------------------------------------------------------
// [SECTION] Styling Utils
//-----------------------------------------------------------------------------

// Get styling data for next item (call between Begin/EndItem)
static inline const ImPlotNextItemData& GetItemData() { return GImPlot->NextItemData; }

// Returns true if a color is set to be automatically determined
static inline bool IsColorAuto(const ImVec4& col) { return col.w == -1; }
// Returns true if a style color is set to be automaticaly determined
static inline bool IsColorAuto(ImPlotCol idx) { return IsColorAuto(GImPlot->Style.Colors[idx]); }
// Returns the automatically deduced style color
IMPLOT_API ImVec4 GetAutoColor(ImPlotCol idx);

// Returns the style color whether it is automatic or custom set
static inline ImVec4 GetStyleColorVec4(ImPlotCol idx) { return IsColorAuto(idx) ? GetAutoColor(idx) : GImPlot->Style.Colors[idx]; }
static inline ImU32  GetStyleColorU32(ImPlotCol idx)  { return ImGui::ColorConvertFloat4ToU32(GetStyleColorVec4(idx)); }

// Draws vertical text. The position is the bottom left of the text rect.
IMPLOT_API void AddTextVertical(ImDrawList *DrawList, ImVec2 pos, ImU32 col, const char* text_begin, const char* text_end = NULL);
// Draws multiline horizontal text centered.
IMPLOT_API void AddTextCentered(ImDrawList* DrawList, ImVec2 top_center, ImU32 col, const char* text_begin, const char* text_end = NULL);
// Calculates the size of vertical text
static inline ImVec2 CalcTextSizeVertical(const char *text) {
    ImVec2 sz = ImGui::CalcTextSize(text);
    return ImVec2(sz.y, sz.x);
}
// Returns white or black text given background color
static inline ImU32 CalcTextColor(const ImVec4& bg) { return (bg.x * 0.299f + bg.y * 0.587f + bg.z * 0.114f) > 0.5f ? IM_COL32_BLACK : IM_COL32_WHITE; }
static inline ImU32 CalcTextColor(ImU32 bg)         { return CalcTextColor(ImGui::ColorConvertU32ToFloat4(bg)); }
// Lightens or darkens a color for hover
static inline ImU32 CalcHoverColor(ImU32 col)       {  return ImMixU32(col, CalcTextColor(col), 32); }

// Clamps a label position so that it fits a rect defined by Min/Max
static inline ImVec2 ClampLabelPos(ImVec2 pos, const ImVec2& size, const ImVec2& Min, const ImVec2& Max) {
    if (pos.x < Min.x)              pos.x = Min.x;
    if (pos.y < Min.y)              pos.y = Min.y;
    if ((pos.x + size.x) > Max.x)   pos.x = Max.x - size.x;
    if ((pos.y + size.y) > Max.y)   pos.y = Max.y - size.y;
    return pos;
}

// Returns a color from the Color map given an index >= 0 (modulo will be performed).
IMPLOT_API ImU32  GetColormapColorU32(int idx, ImPlotColormap cmap);
// Returns the next unused colormap color and advances the colormap. Can be used to skip colors if desired.
IMPLOT_API ImU32  NextColormapColorU32();
// Linearly interpolates a color from the current colormap given t between 0 and 1.
IMPLOT_API ImU32  SampleColormapU32(float t, ImPlotColormap cmap);

// Render a colormap bar
IMPLOT_API void RenderColorBar(const ImU32* colors, int size, ImDrawList& DrawList, const ImRect& bounds, bool vert, bool reversed, bool continuous);

//-----------------------------------------------------------------------------
// [SECTION] Math and Misc Utils
//-----------------------------------------------------------------------------

// Rounds x to powers of 2,5 and 10 for generating axis labels (from Graphics Gems 1 Chapter 11.2)
IMPLOT_API double NiceNum(double x, bool round);
// Computes order of magnitude of double.
static inline int OrderOfMagnitude(double val) { return val == 0 ? 0 : (int)(floor(log10(fabs(val)))); }
// Returns the precision required for a order of magnitude.
static inline int OrderToPrecision(int order) { return order > 0 ? 0 : 1 - order; }
// Returns a floating point precision to use given a value
static inline int Precision(double val) { return OrderToPrecision(OrderOfMagnitude(val)); }
// Round a value to a given precision
static inline double RoundTo(double val, int prec) { double p = pow(10,(double)prec); return floor(val*p+0.5)/p; }

// Returns the intersection point of two lines A and B (assumes they are not parallel!)
static inline ImVec2 Intersection(const ImVec2& a1, const ImVec2& a2, const ImVec2& b1, const ImVec2& b2) {
    float v1 = (a1.x * a2.y - a1.y * a2.x);  float v2 = (b1.x * b2.y - b1.y * b2.x);
    float v3 = ((a1.x - a2.x) * (b1.y - b2.y) - (a1.y - a2.y) * (b1.x - b2.x));
    return ImVec2((v1 * (b1.x - b2.x) - v2 * (a1.x - a2.x)) / v3, (v1 * (b1.y - b2.y) - v2 * (a1.y - a2.y)) / v3);
}

// Fills a buffer with n samples linear interpolated from vmin to vmax
template <typename T>
void FillRange(ImVector<T>& buffer, int n, T vmin, T vmax) {
    buffer.resize(n);
    T step = (vmax - vmin) / (n - 1);
    for (int i = 0; i < n; ++i) {
        buffer[i] = vmin + i * step;
    }
}

// Calculate histogram bin counts and widths
template <typename T>
static inline void CalculateBins(const T* values, int count, ImPlotBin meth, const ImPlotRange& range, int& bins_out, double& width_out) {
    switch (meth) {
        case ImPlotBin_Sqrt:
            bins_out  = (int)ceil(sqrt(count));
            break;
        case ImPlotBin_Sturges:
            bins_out  = (int)ceil(1.0 + log2(count));
            break;
        case ImPlotBin_Rice:
            bins_out  = (int)ceil(2 * cbrt(count));
            break;
        case ImPlotBin_Scott:
            width_out = 3.49 * ImStdDev(values, count) / cbrt(count);
            bins_out  = (int)round(range.Size() / width_out);
            break;
    }
    width_out = range.Size() / bins_out;
}

//-----------------------------------------------------------------------------
// Time Utils
//-----------------------------------------------------------------------------

// Returns true if year is leap year (366 days long)
static inline bool IsLeapYear(int year) {
    return  year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
// Returns the number of days in a month, accounting for Feb. leap years. #month is zero indexed.
static inline int GetDaysInMonth(int year, int month) {
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return  days[month] + (int)(month == 1 && IsLeapYear(year));
}

// Make a UNIX timestamp from a tm struct expressed in UTC time (i.e. GMT timezone).
IMPLOT_API ImPlotTime MkGmtTime(struct tm *ptm);
// Make a tm struct expressed in UTC time (i.e. GMT timezone) from a UNIX timestamp.
IMPLOT_API tm* GetGmtTime(const ImPlotTime& t, tm* ptm);

// Make a UNIX timestamp from a tm struct expressed in local time.
IMPLOT_API ImPlotTime MkLocTime(struct tm *ptm);
// Make a tm struct expressed in local time from a UNIX timestamp.
IMPLOT_API tm* GetLocTime(const ImPlotTime& t, tm* ptm);

// NB: The following functions only work if there is a current ImPlotContext because the
// internal tm struct is owned by the context! They are aware of ImPlotStyle.UseLocalTime.

// Make a timestamp from time components.
// year[1970-3000], month[0-11], day[1-31], hour[0-23], min[0-59], sec[0-59], us[0,999999]
IMPLOT_API ImPlotTime MakeTime(int year, int month = 0, int day = 1, int hour = 0, int min = 0, int sec = 0, int us = 0);
// Get year component from timestamp [1970-3000]
IMPLOT_API int GetYear(const ImPlotTime& t);

// Adds or subtracts time from a timestamp. #count > 0 to add, < 0 to subtract.
IMPLOT_API ImPlotTime AddTime(const ImPlotTime& t, ImPlotTimeUnit unit, int count);
// Rounds a timestamp down to nearest unit.
IMPLOT_API ImPlotTime FloorTime(const ImPlotTime& t, ImPlotTimeUnit unit);
// Rounds a timestamp up to the nearest unit.
IMPLOT_API ImPlotTime CeilTime(const ImPlotTime& t, ImPlotTimeUnit unit);
// Rounds a timestamp up or down to the nearest unit.
IMPLOT_API ImPlotTime RoundTime(const ImPlotTime& t, ImPlotTimeUnit unit);
// Combines the date of one timestamp with the time-of-day of another timestamp.
IMPLOT_API ImPlotTime CombineDateTime(const ImPlotTime& date_part, const ImPlotTime& time_part);

// Formats the time part of timestamp t into a buffer according to #fmt
IMPLOT_API int FormatTime(const ImPlotTime& t, char* buffer, int size, ImPlotTimeFmt fmt, bool use_24_hr_clk);
// Formats the date part of timestamp t into a buffer according to #fmt
IMPLOT_API int FormatDate(const ImPlotTime& t, char* buffer, int size, ImPlotDateFmt fmt, bool use_iso_8601);
// Formats the time and/or date parts of a timestamp t into a buffer according to #fmt
IMPLOT_API int FormatDateTime(const ImPlotTime& t, char* buffer, int size, ImPlotDateTimeSpec fmt);

// Shows a date picker widget block (year/month/day).
// #level = 0 for day, 1 for month, 2 for year. Modified by user interaction.
// #t will be set when a day is clicked and the function will return true.
// #t1 and #t2 are optional dates to highlight.
IMPLOT_API bool ShowDatePicker(const char* id, int* level, ImPlotTime* t, const ImPlotTime* t1 = NULL, const ImPlotTime* t2 = NULL);
// Shows a time picker widget block (hour/min/sec).
// #t will be set when a new hour, minute, or sec is selected or am/pm is toggled, and the function will return true.
IMPLOT_API bool ShowTimePicker(const char* id, ImPlotTime* t);

//-----------------------------------------------------------------------------
// [SECTION] Transforms
//-----------------------------------------------------------------------------

static inline double TransformForward_Log10(double v, void*) {
    v = v <= 0.0 ? DBL_MIN : v;
    return ImLog10(v);
}

static inline double TransformInverse_Log10(double v, void*) {
    return ImPow(10, v);
}

static inline double TransformForward_SymLog(double v, void*) {
    return 2.0 * ImAsinh(v / 2.0);
}

static inline double TransformInverse_SymLog(double v, void*) {
    return 2.0 * ImSinh(v / 2.0);
}

static inline double TransformForward_Logit(double v, void*) {
    v = ImClamp(v, DBL_MIN, 1.0 - DBL_EPSILON);
    return ImLog10(v / (1 - v));
}

static inline double TransformInverse_Logit(double v, void*) {
    return 1.0 / (1.0 + ImPow(10,-v));
}

//-----------------------------------------------------------------------------
// [SECTION] Formatters
//-----------------------------------------------------------------------------

static inline int Formatter_Default(double value, char* buff, int size, void* data) {
    char* fmt = (char*)data;
    return ImFormatString(buff, size, fmt, value);
}

static inline int Formatter_Logit(double value, char* buff, int size, void*) {
    if (value == 0.5)
        return ImFormatString(buff,size,"1/2");
    else if (value < 0.5)
        return ImFormatString(buff,size,"%g", value);
    else
        return ImFormatString(buff,size,"1 - %g", 1 - value);
}

struct Formatter_Time_Data {
    ImPlotTime Time;
    ImPlotDateTimeSpec Spec;
    ImPlotFormatter UserFormatter;
    void* UserFormatterData;
};

static inline int Formatter_Time(double, char* buff, int size, void* data) {
    Formatter_Time_Data* ftd = (Formatter_Time_Data*)data;
    return FormatDateTime(ftd->Time, buff, size, ftd->Spec);
}

//------------------------------------------------------------------------------
// [SECTION] Locator
//------------------------------------------------------------------------------

void Locator_Default(ImPlotTicker& ticker, const ImPlotRange& range, float pixels, bool vertical, ImPlotFormatter formatter, void* formatter_data);
void Locator_Time(ImPlotTicker& ticker, const ImPlotRange& range, float pixels, bool vertical, ImPlotFormatter formatter, void* formatter_data);
void Locator_Log10(ImPlotTicker& ticker, const ImPlotRange& range, float pixels, bool vertical, ImPlotFormatter formatter, void* formatter_data);
void Locator_SymLog(ImPlotTicker& ticker, const ImPlotRange& range, float pixels, bool vertical, ImPlotFormatter formatter, void* formatter_data);

} // namespace ImPlot
