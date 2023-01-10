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

// We define this so that the demo does not accidentally use deprecated API
#ifndef IMPLOT_DISABLE_OBSOLETE_FUNCTIONS
#define IMPLOT_DISABLE_OBSOLETE_FUNCTIONS
#endif

#include "implot.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _MSC_VER
#define sprintf sprintf_s
#endif

#ifndef PI
#define PI 3.14159265358979323846
#endif

#define CHECKBOX_FLAG(flags, flag) ImGui::CheckboxFlags(#flag, (unsigned int*)&flags, flag)

// Encapsulates examples for customizing ImPlot.
namespace MyImPlot {

// Example for Custom Data and Getters section.
struct Vector2f {
    Vector2f(float _x, float _y) { x = _x; y = _y; }
    float x, y;
};

// Example for Custom Data and Getters section.
struct WaveData {
    double X, Amp, Freq, Offset;
    WaveData(double x, double amp, double freq, double offset) { X = x; Amp = amp; Freq = freq; Offset = offset; }
};
ImPlotPoint SineWave(int idx, void* wave_data);
ImPlotPoint SawWave(int idx, void* wave_data);
ImPlotPoint Spiral(int idx, void* wave_data);

// Example for Tables section.
void Sparkline(const char* id, const float* values, int count, float min_v, float max_v, int offset, const ImVec4& col, const ImVec2& size);

// Example for Custom Plotters and Tooltips section.
void PlotCandlestick(const char* label_id, const double* xs, const double* opens, const double* closes, const double* lows, const double* highs, int count, bool tooltip = true, float width_percent = 0.25f, ImVec4 bullCol = ImVec4(0,1,0,1), ImVec4 bearCol = ImVec4(1,0,0,1));

// Example for Custom Styles section.
void StyleSeaborn();

} // namespace MyImPlot

namespace ImPlot {

template <typename T>
inline T RandomRange(T min, T max) {
    T scale = rand() / (T) RAND_MAX;
    return min + scale * ( max - min );
}

ImVec4 RandomColor() {
    ImVec4 col;
    col.x = RandomRange(0.0f,1.0f);
    col.y = RandomRange(0.0f,1.0f);
    col.z = RandomRange(0.0f,1.0f);
    col.w = 1.0f;
    return col;
}

double RandomGauss() {
	static double V1, V2, S;
	static int phase = 0;
	double X;
	if(phase == 0) {
		do {
			double U1 = (double)rand() / RAND_MAX;
			double U2 = (double)rand() / RAND_MAX;
			V1 = 2 * U1 - 1;
			V2 = 2 * U2 - 1;
			S = V1 * V1 + V2 * V2;
			} while(S >= 1 || S == 0);

		X = V1 * sqrt(-2 * log(S) / S);
	} else
		X = V2 * sqrt(-2 * log(S) / S);
	phase = 1 - phase;
	return X;
}

template <int N>
struct NormalDistribution {
    NormalDistribution(double mean, double sd) {
        for (int i = 0; i < N; ++i)
            Data[i] = RandomGauss()*sd + mean;
    }
    double Data[N];
};

// utility structure for realtime plot
struct ScrollingBuffer {
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 2000) {
        MaxSize = max_size;
        Offset  = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y) {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x,y));
        else {
            Data[Offset] = ImVec2(x,y);
            Offset =  (Offset + 1) % MaxSize;
        }
    }
    void Erase() {
        if (Data.size() > 0) {
            Data.shrink(0);
            Offset  = 0;
        }
    }
};

// utility structure for realtime plot
struct RollingBuffer {
    float Span;
    ImVector<ImVec2> Data;
    RollingBuffer() {
        Span = 10.0f;
        Data.reserve(2000);
    }
    void AddPoint(float x, float y) {
        float xmod = fmodf(x, Span);
        if (!Data.empty() && xmod < Data.back().x)
            Data.shrink(0);
        Data.push_back(ImVec2(xmod, y));
    }
};

// Huge data used by Time Formatting example (~500 MB allocation!)
struct HugeTimeData {
    HugeTimeData(double min) {
        Ts = new double[Size];
        Ys = new double[Size];
        for (int i = 0; i < Size; ++i) {
            Ts[i] = min + i;
            Ys[i] = GetY(Ts[i]);
        }
    }
    ~HugeTimeData() { delete[] Ts;  delete[] Ys; }
    static double GetY(double t) {
        return 0.5 + 0.25 * sin(t/86400/12) +  0.005 * sin(t/3600);
    }
    double* Ts;
    double* Ys;
    static const int Size = 60*60*24*366;
};

//-----------------------------------------------------------------------------
// [SECTION] Demo Functions
//-----------------------------------------------------------------------------

void Demo_Help() {
    ImGui::Text("ABOUT THIS DEMO:");
    ImGui::BulletText("Sections below are demonstrating many aspects of the library.");
    ImGui::BulletText("The \"Tools\" menu above gives access to: Style Editors (ImPlot/ImGui)\n"
                        "and Metrics (general purpose Dear ImGui debugging tool).");
    ImGui::Separator();
    ImGui::Text("PROGRAMMER GUIDE:");
    ImGui::BulletText("See the ShowDemoWindow() code in implot_demo.cpp. <- you are here!");
    ImGui::BulletText("If you see visual artifacts, do one of the following:");
    ImGui::Indent();
    ImGui::BulletText("Handle ImGuiBackendFlags_RendererHasVtxOffset for 16-bit indices in your backend.");
    ImGui::BulletText("Or, enable 32-bit indices in imconfig.h.");
    ImGui::BulletText("Your current configuration is:");
    ImGui::Indent();
    ImGui::BulletText("ImDrawIdx: %d-bit", (int)(sizeof(ImDrawIdx) * 8));
    ImGui::BulletText("ImGuiBackendFlags_RendererHasVtxOffset: %s", (ImGui::GetIO().BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset) ? "True" : "False");
    ImGui::Unindent();
    ImGui::Unindent();
    ImGui::Separator();
    ImGui::Text("USER GUIDE:");
    ShowUserGuide();
}

//-----------------------------------------------------------------------------

void ButtonSelector(const char* label, ImGuiMouseButton* b) {
    ImGui::PushID(label);
    if (ImGui::RadioButton("LMB",*b == ImGuiMouseButton_Left))
        *b = ImGuiMouseButton_Left;
    ImGui::SameLine();
    if (ImGui::RadioButton("RMB",*b == ImGuiMouseButton_Right))
        *b = ImGuiMouseButton_Right;
    ImGui::SameLine();
    if (ImGui::RadioButton("MMB",*b == ImGuiMouseButton_Middle))
        *b = ImGuiMouseButton_Middle;
    ImGui::PopID();
}

void ModSelector(const char* label, int* k) {
    ImGui::PushID(label);
    ImGui::CheckboxFlags("Ctrl", (unsigned int*)k, ImGuiMod_Ctrl); ImGui::SameLine();
    ImGui::CheckboxFlags("Shift", (unsigned int*)k, ImGuiMod_Shift); ImGui::SameLine();
    ImGui::CheckboxFlags("Alt", (unsigned int*)k, ImGuiMod_Alt); ImGui::SameLine();
    ImGui::CheckboxFlags("Super", (unsigned int*)k, ImGuiMod_Super);
    ImGui::PopID();
}

void InputMapping(const char* label, ImGuiMouseButton* b, int* k) {
    ImGui::LabelText("##","%s",label);
    if (b != NULL) {
        ImGui::SameLine(100);
        ButtonSelector(label,b);
    }
    if (k != NULL) {
        ImGui::SameLine(300);
        ModSelector(label,k);
    }
}

void ShowInputMapping() {
    ImPlotInputMap& map = ImPlot::GetInputMap();
    InputMapping("Pan",&map.Pan,&map.PanMod);
    InputMapping("Fit",&map.Fit,NULL);
    InputMapping("Select",&map.Select,&map.SelectMod);
    InputMapping("SelectHorzMod",NULL,&map.SelectHorzMod);
    InputMapping("SelectVertMod",NULL,&map.SelectVertMod);
    InputMapping("SelectCancel",&map.SelectCancel,NULL);
    InputMapping("Menu",&map.Menu,NULL);
    InputMapping("OverrideMod",NULL,&map.OverrideMod);
    InputMapping("ZoomMod",NULL,&map.ZoomMod);
    ImGui::SliderFloat("ZoomRate",&map.ZoomRate,-1,1);
}

void Demo_Config() {
    ImGui::ShowFontSelector("Font");
    ImGui::ShowStyleSelector("ImGui Style");
    ImPlot::ShowStyleSelector("ImPlot Style");
    ImPlot::ShowColormapSelector("ImPlot Colormap");
    ImPlot::ShowInputMapSelector("Input Map");
    ImGui::Separator();
    ImGui::Checkbox("Use Local Time", &ImPlot::GetStyle().UseLocalTime);
    ImGui::Checkbox("Use ISO 8601", &ImPlot::GetStyle().UseISO8601);
    ImGui::Checkbox("Use 24 Hour Clock", &ImPlot::GetStyle().Use24HourClock);
    ImGui::Separator();
    if (ImPlot::BeginPlot("Preview")) {
        static double now = (double)time(0);
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
        ImPlot::SetupAxisLimits(ImAxis_X1, now, now + 24*3600);
        for (int i = 0; i < 10; ++i) {
            double x[2] = {now, now + 24*3600};
            double y[2] = {0,i/9.0};
            ImGui::PushID(i);
            ImPlot::PlotLine("##Line",x,y,2);
            ImGui::PopID();
        }
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_LinePlots() {
    static float xs1[1001], ys1[1001];
    for (int i = 0; i < 1001; ++i) {
        xs1[i] = i * 0.001f;
        ys1[i] = 0.5f + 0.5f * sinf(50 * (xs1[i] + (float)ImGui::GetTime() / 10));
    }
    static double xs2[20], ys2[20];
    for (int i = 0; i < 20; ++i) {
        xs2[i] = i * 1/19.0f;
        ys2[i] = xs2[i] * xs2[i];
    }
    if (ImPlot::BeginPlot("Line Plots")) {
        ImPlot::SetupAxes("x","y");
        ImPlot::PlotLine("f(x)", xs1, ys1, 1001);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
        ImPlot::PlotLine("g(x)", xs2, ys2, 20,ImPlotLineFlags_Segments);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_FilledLinePlots() {
    static double xs1[101], ys1[101], ys2[101], ys3[101];
    srand(0);
    for (int i = 0; i < 101; ++i) {
        xs1[i] = (float)i;
        ys1[i] = RandomRange(400.0,450.0);
        ys2[i] = RandomRange(275.0,350.0);
        ys3[i] = RandomRange(150.0,225.0);
    }
    static bool show_lines = true;
    static bool show_fills = true;
    static float fill_ref = 0;
    static int shade_mode = 0;
    static ImPlotShadedFlags flags = 0;
    ImGui::Checkbox("Lines",&show_lines); ImGui::SameLine();
    ImGui::Checkbox("Fills",&show_fills);
    if (show_fills) {
        ImGui::SameLine();
        if (ImGui::RadioButton("To -INF",shade_mode == 0))
            shade_mode = 0;
        ImGui::SameLine();
        if (ImGui::RadioButton("To +INF",shade_mode == 1))
            shade_mode = 1;
        ImGui::SameLine();
        if (ImGui::RadioButton("To Ref",shade_mode == 2))
            shade_mode = 2;
        if (shade_mode == 2) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::DragFloat("##Ref",&fill_ref, 1, -100, 500);
        }
    }

    if (ImPlot::BeginPlot("Stock Prices")) {
        ImPlot::SetupAxes("Days","Price");
        ImPlot::SetupAxesLimits(0,100,0,500);
        if (show_fills) {
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::PlotShaded("Stock 1", xs1, ys1, 101, shade_mode == 0 ? -INFINITY : shade_mode == 1 ? INFINITY : fill_ref, flags);
            ImPlot::PlotShaded("Stock 2", xs1, ys2, 101, shade_mode == 0 ? -INFINITY : shade_mode == 1 ? INFINITY : fill_ref, flags);
            ImPlot::PlotShaded("Stock 3", xs1, ys3, 101, shade_mode == 0 ? -INFINITY : shade_mode == 1 ? INFINITY : fill_ref, flags);
            ImPlot::PopStyleVar();
        }
        if (show_lines) {
            ImPlot::PlotLine("Stock 1", xs1, ys1, 101);
            ImPlot::PlotLine("Stock 2", xs1, ys2, 101);
            ImPlot::PlotLine("Stock 3", xs1, ys3, 101);
        }
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_ShadedPlots() {
    static float xs[1001], ys[1001], ys1[1001], ys2[1001], ys3[1001], ys4[1001];
    srand(0);
    for (int i = 0; i < 1001; ++i) {
        xs[i]  = i * 0.001f;
        ys[i]  = 0.25f + 0.25f * sinf(25 * xs[i]) * sinf(5 * xs[i]) + RandomRange(-0.01f, 0.01f);
        ys1[i] = ys[i] + RandomRange(0.1f, 0.12f);
        ys2[i] = ys[i] - RandomRange(0.1f, 0.12f);
        ys3[i] = 0.75f + 0.2f * sinf(25 * xs[i]);
        ys4[i] = 0.75f + 0.1f * cosf(25 * xs[i]);
    }
    static float alpha = 0.25f;
    ImGui::DragFloat("Alpha",&alpha,0.01f,0,1);

    if (ImPlot::BeginPlot("Shaded Plots")) {
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, alpha);
        ImPlot::PlotShaded("Uncertain Data",xs,ys1,ys2,1001);
        ImPlot::PlotLine("Uncertain Data", xs, ys, 1001);
        ImPlot::PlotShaded("Overlapping",xs,ys3,ys4,1001);
        ImPlot::PlotLine("Overlapping",xs,ys3,1001);
        ImPlot::PlotLine("Overlapping",xs,ys4,1001);
        ImPlot::PopStyleVar();
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_ScatterPlots() {
    srand(0);
    static float xs1[100], ys1[100];
    for (int i = 0; i < 100; ++i) {
        xs1[i] = i * 0.01f;
        ys1[i] = xs1[i] + 0.1f * ((float)rand() / (float)RAND_MAX);
    }
    static float xs2[50], ys2[50];
    for (int i = 0; i < 50; i++) {
        xs2[i] = 0.25f + 0.2f * ((float)rand() / (float)RAND_MAX);
        ys2[i] = 0.75f + 0.2f * ((float)rand() / (float)RAND_MAX);
    }

    if (ImPlot::BeginPlot("Scatter Plot")) {
        ImPlot::PlotScatter("Data 1", xs1, ys1, 100);
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Square, 6, ImPlot::GetColormapColor(1), IMPLOT_AUTO, ImPlot::GetColormapColor(1));
        ImPlot::PlotScatter("Data 2", xs2, ys2, 50);
        ImPlot::PopStyleVar();
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_StairstepPlots() {
    static float ys1[21], ys2[21];
    for (int i = 0; i < 21; ++i) {
        ys1[i] = 0.75f + 0.2f * sinf(10 * i * 0.05f);
        ys2[i] = 0.25f + 0.2f * sinf(10 * i * 0.05f);
    }
    static ImPlotStairsFlags flags = 0;
    CHECKBOX_FLAG(flags, ImPlotStairsFlags_Shaded);
    if (ImPlot::BeginPlot("Stairstep Plot")) {
        ImPlot::SetupAxes("x","f(x)");
        ImPlot::SetupAxesLimits(0,1,0,1);

        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.5f,0.5f,0.5f,1.0f));
        ImPlot::PlotLine("##1",ys1,21,0.05f);
        ImPlot::PlotLine("##2",ys2,21,0.05f);
        ImPlot::PopStyleColor();

        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.25f);
        ImPlot::PlotStairs("Post Step (default)", ys1, 21, 0.05f, 0, flags);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.25f);
        ImPlot::PlotStairs("Pre Step", ys2, 21, 0.05f, 0, flags|ImPlotStairsFlags_PreStep);

        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_BarPlots() {
    static ImS8  data[10] = {1,2,3,4,5,6,7,8,9,10};
    if (ImPlot::BeginPlot("Bar Plot")) {
        ImPlot::PlotBars("Vertical",data,10,0.7,1);
        ImPlot::PlotBars("Horizontal",data,10,0.4,1,ImPlotBarsFlags_Horizontal);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_BarGroups() {
    static ImS8  data[30] = {83, 67, 23, 89, 83, 78, 91, 82, 85, 90,  // midterm
                             80, 62, 56, 99, 55, 78, 88, 78, 90, 100, // final
                             80, 69, 52, 92, 72, 78, 75, 76, 89, 95}; // course

    static const char*  ilabels[]   = {"Midterm Exam","Final Exam","Course Grade"};
    static const char*  glabels[]   = {"S1","S2","S3","S4","S5","S6","S7","S8","S9","S10"};
    static const double positions[] = {0,1,2,3,4,5,6,7,8,9};

    static int items  = 3;
    static int groups = 10;
    static float size = 0.67f;

    static ImPlotBarGroupsFlags flags = 0;
    static bool horz = false;

    ImGui::CheckboxFlags("Stacked", (unsigned int*)&flags, ImPlotBarGroupsFlags_Stacked);
    ImGui::SameLine();
    ImGui::Checkbox("Horizontal",&horz);

    ImGui::SliderInt("Items",&items,1,3);
    ImGui::SliderFloat("Size",&size,0,1);

    if (ImPlot::BeginPlot("Bar Group")) {
        ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);
        if (horz) {
            ImPlot::SetupAxes("Score","Student",ImPlotAxisFlags_AutoFit,ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxisTicks(ImAxis_Y1,positions, groups, glabels);
            ImPlot::PlotBarGroups(ilabels,data,items,groups,size,0,flags|ImPlotBarGroupsFlags_Horizontal);
        }
        else {
            ImPlot::SetupAxes("Student","Score",ImPlotAxisFlags_AutoFit,ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxisTicks(ImAxis_X1,positions, groups, glabels);
            ImPlot::PlotBarGroups(ilabels,data,items,groups,size,0,flags);
        }
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_BarStacks() {

    static ImPlotColormap Liars = -1;
    if (Liars == -1) {
        static const ImU32 Liars_Data[6] = { 4282515870, 4282609140, 4287357182, 4294630301, 4294945280, 4294921472 };
        Liars = ImPlot::AddColormap("Liars", Liars_Data, 6);
    }

    static bool diverging = true;
    ImGui::Checkbox("Diverging",&diverging);

    static const char* politicians[] = {"Trump","Bachman","Cruz","Gingrich","Palin","Santorum","Walker","Perry","Ryan","McCain","Rubio","Romney","Rand Paul","Christie","Biden","Kasich","Sanders","J Bush","H Clinton","Obama"};
    static int data_reg[] = {18,26,7,14,10,8,6,11,4,4,3,8,6,8,6,5,0,3,1,2,                // Pants on Fire
                             43,36,30,21,30,27,25,17,11,22,15,16,16,17,12,12,14,6,13,12,  // False
                             16,13,28,22,15,21,15,18,30,17,24,18,13,10,14,15,17,22,14,12, // Mostly False
                             17,10,13,25,12,22,19,26,23,17,22,27,20,26,29,17,18,22,21,27, // Half True
                             5,7,16,10,10,12,23,13,17,20,22,16,23,19,20,26,36,29,27,26,   // Mostly True
                             1,8,6,8,23,10,12,15,15,20,14,15,22,20,19,25,15,18,24,21};    // True
    static const char* labels_reg[] = {"Pants on Fire","False","Mostly False","Half True","Mostly True","True"};


    static int data_div[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                         // Pants on Fire (dummy, to order legend logically)
                             0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                         // False         (dummy, to order legend logically)
                             0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                         // Mostly False  (dummy, to order legend logically)
                             -16,-13,-28,-22,-15,-21,-15,-18,-30,-17,-24,-18,-13,-10,-14,-15,-17,-22,-14,-12, // Mostly False
                             -43,-36,-30,-21,-30,-27,-25,-17,-11,-22,-15,-16,-16,-17,-12,-12,-14,-6,-13,-12,  // False
                             -18,-26,-7,-14,-10,-8,-6,-11,-4,-4,-3,-8,-6,-8,-6,-5,0,-3,-1,-2,                 // Pants on Fire
                             17,10,13,25,12,22,19,26,23,17,22,27,20,26,29,17,18,22,21,27,                     // Half True
                             5,7,16,10,10,12,23,13,17,20,22,16,23,19,20,26,36,29,27,26,                       // Mostly True
                             1,8,6,8,23,10,12,15,15,20,14,15,22,20,19,25,15,18,24,21};                        // True
    static const char* labels_div[] = {"Pants on Fire","False","Mostly False","Mostly False","False","Pants on Fire","Half True","Mostly True","True"};

    ImPlot::PushColormap(Liars);
    if (ImPlot::BeginPlot("PolitiFact: Who Lies More?",ImVec2(-1,400),ImPlotFlags_NoMouseText)) {
        ImPlot::SetupLegend(ImPlotLocation_South, ImPlotLegendFlags_Outside|ImPlotLegendFlags_Horizontal);
        ImPlot::SetupAxes(NULL,NULL,ImPlotAxisFlags_AutoFit|ImPlotAxisFlags_NoDecorations,ImPlotAxisFlags_AutoFit|ImPlotAxisFlags_Invert);
        ImPlot::SetupAxisTicks(ImAxis_Y1,0,19,20,politicians,false);
        if (diverging)
            ImPlot::PlotBarGroups(labels_div,data_div,9,20,0.75,0,ImPlotBarGroupsFlags_Stacked|ImPlotBarGroupsFlags_Horizontal);
        else
            ImPlot::PlotBarGroups(labels_reg,data_reg,6,20,0.75,0,ImPlotBarGroupsFlags_Stacked|ImPlotBarGroupsFlags_Horizontal);
        ImPlot::EndPlot();
    }
    ImPlot::PopColormap();
}

//-----------------------------------------------------------------------------

void Demo_ErrorBars() {
    static float xs[5]    = {1,2,3,4,5};
    static float bar[5]   = {1,2,5,3,4};
    static float lin1[5]  = {8,8,9,7,8};
    static float lin2[5]  = {6,7,6,9,6};
    static float err1[5]  = {0.2f, 0.4f, 0.2f, 0.6f, 0.4f};
    static float err2[5]  = {0.4f, 0.2f, 0.4f, 0.8f, 0.6f};
    static float err3[5]  = {0.09f, 0.14f, 0.09f, 0.12f, 0.16f};
    static float err4[5]  = {0.02f, 0.08f, 0.15f, 0.05f, 0.2f};


    if (ImPlot::BeginPlot("##ErrorBars")) {
        ImPlot::SetupAxesLimits(0, 6, 0, 10);
        ImPlot::PlotBars("Bar", xs, bar, 5, 0.5f);
        ImPlot::PlotErrorBars("Bar", xs, bar, err1, 5);
        ImPlot::SetNextErrorBarStyle(ImPlot::GetColormapColor(1), 0);
        ImPlot::PlotErrorBars("Line", xs, lin1, err1, err2, 5);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Square);
        ImPlot::PlotLine("Line", xs, lin1, 5);
        ImPlot::PushStyleColor(ImPlotCol_ErrorBar, ImPlot::GetColormapColor(2));
        ImPlot::PlotErrorBars("Scatter", xs, lin2, err2, 5);
        ImPlot::PlotErrorBars("Scatter", xs, lin2,  err3, err4, 5, ImPlotErrorBarsFlags_Horizontal);
        ImPlot::PopStyleColor();
        ImPlot::PlotScatter("Scatter", xs, lin2, 5);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_StemPlots() {
    static double xs[51], ys1[51], ys2[51];
    for (int i = 0; i < 51; ++i) {
        xs[i] = i * 0.02;
        ys1[i] = 1.0 + 0.5 * sin(25*xs[i])*cos(2*xs[i]);
        ys2[i] = 0.5 + 0.25  * sin(10*xs[i]) * sin(xs[i]);
    }
    if (ImPlot::BeginPlot("Stem Plots")) {
        ImPlot::SetupAxisLimits(ImAxis_X1,0,1.0);
        ImPlot::SetupAxisLimits(ImAxis_Y1,0,1.6);
        ImPlot::PlotStems("Stems 1",xs,ys1,51);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
        ImPlot::PlotStems("Stems 2", xs, ys2,51);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_InfiniteLines() {
    static double vals[] = {0.25, 0.5, 0.75};
    if (ImPlot::BeginPlot("##Infinite")) {
        ImPlot::SetupAxes(NULL,NULL,ImPlotAxisFlags_NoInitialFit,ImPlotAxisFlags_NoInitialFit);
        ImPlot::PlotInfLines("Vertical",vals,3);
        ImPlot::PlotInfLines("Horizontal",vals,3,ImPlotInfLinesFlags_Horizontal);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_PieCharts() {
    static const char* labels1[]    = {"Frogs","Hogs","Dogs","Logs"};
    static float data1[]            = {0.15f,  0.30f,  0.2f, 0.05f};
    static ImPlotPieChartFlags flags = 0;
    ImGui::SetNextItemWidth(250);
    ImGui::DragFloat4("Values", data1, 0.01f, 0, 1);
    if ((data1[0] + data1[1] + data1[2] + data1[3]) < 1) {
        ImGui::SameLine();
        CHECKBOX_FLAG(flags,ImPlotPieChartFlags_Normalize);
    }

    if (ImPlot::BeginPlot("##Pie1", ImVec2(250,250), ImPlotFlags_Equal | ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, 1, 0, 1);
        ImPlot::PlotPieChart(labels1, data1, 4, 0.5, 0.5, 0.4, "%.2f", 90, flags);
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    static const char* labels2[]   = {"A","B","C","D","E"};
    static int data2[]             = {1,1,2,3,5};

    ImPlot::PushColormap(ImPlotColormap_Pastel);
    if (ImPlot::BeginPlot("##Pie2", ImVec2(250,250), ImPlotFlags_Equal | ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, 1, 0, 1);
        ImPlot::PlotPieChart(labels2, data2, 5, 0.5, 0.5, 0.4, "%.0f", 180, flags);
        ImPlot::EndPlot();
    }
    ImPlot::PopColormap();
}

//-----------------------------------------------------------------------------

void Demo_Heatmaps() {
    static float values1[7][7]  = {{0.8f, 2.4f, 2.5f, 3.9f, 0.0f, 4.0f, 0.0f},
                                    {2.4f, 0.0f, 4.0f, 1.0f, 2.7f, 0.0f, 0.0f},
                                    {1.1f, 2.4f, 0.8f, 4.3f, 1.9f, 4.4f, 0.0f},
                                    {0.6f, 0.0f, 0.3f, 0.0f, 3.1f, 0.0f, 0.0f},
                                    {0.7f, 1.7f, 0.6f, 2.6f, 2.2f, 6.2f, 0.0f},
                                    {1.3f, 1.2f, 0.0f, 0.0f, 0.0f, 3.2f, 5.1f},
                                    {0.1f, 2.0f, 0.0f, 1.4f, 0.0f, 1.9f, 6.3f}};
    static float scale_min       = 0;
    static float scale_max       = 6.3f;
    static const char* xlabels[] = {"C1","C2","C3","C4","C5","C6","C7"};
    static const char* ylabels[] = {"R1","R2","R3","R4","R5","R6","R7"};

    static ImPlotColormap map = ImPlotColormap_Viridis;
    if (ImPlot::ColormapButton(ImPlot::GetColormapName(map),ImVec2(225,0),map)) {
        map = (map + 1) % ImPlot::GetColormapCount();
        // We bust the color cache of our plots so that item colors will
        // resample the new colormap in the event that they have already
        // been created. See documentation in implot.h.
        BustColorCache("##Heatmap1");
        BustColorCache("##Heatmap2");
    }

    ImGui::SameLine();
    ImGui::LabelText("##Colormap Index", "%s", "Change Colormap");
    ImGui::SetNextItemWidth(225);
    ImGui::DragFloatRange2("Min / Max",&scale_min, &scale_max, 0.01f, -20, 20);

    static ImPlotHeatmapFlags hm_flags = 0;

    ImGui::CheckboxFlags("Column Major", (unsigned int*)&hm_flags, ImPlotHeatmapFlags_ColMajor);

    static ImPlotAxisFlags axes_flags = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks;

    ImPlot::PushColormap(map);

    if (ImPlot::BeginPlot("##Heatmap1",ImVec2(225,225),ImPlotFlags_NoLegend|ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes(NULL, NULL, axes_flags, axes_flags);
        ImPlot::SetupAxisTicks(ImAxis_X1,0 + 1.0/14.0, 1 - 1.0/14.0, 7, xlabels);
        ImPlot::SetupAxisTicks(ImAxis_Y1,1 - 1.0/14.0, 0 + 1.0/14.0, 7, ylabels);
        ImPlot::PlotHeatmap("heat",values1[0],7,7,scale_min,scale_max,"%g",ImPlotPoint(0,0),ImPlotPoint(1,1),hm_flags);
        ImPlot::EndPlot();
    }
    ImGui::SameLine();
    ImPlot::ColormapScale("##HeatScale",scale_min, scale_max, ImVec2(60,225));

    ImGui::SameLine();

    const int size = 80;
    static double values2[size*size];
    srand((unsigned int)(ImGui::GetTime()*1000000));
    for (int i = 0; i < size*size; ++i)
        values2[i] = RandomRange(0.0,1.0);

    if (ImPlot::BeginPlot("##Heatmap2",ImVec2(225,225))) {
        ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(-1,1,-1,1);
        ImPlot::PlotHeatmap("heat1",values2,size,size,0,1,NULL);
        ImPlot::PlotHeatmap("heat2",values2,size,size,0,1,NULL, ImPlotPoint(-1,-1), ImPlotPoint(0,0));
        ImPlot::EndPlot();
    }
    ImPlot::PopColormap();

}

//-----------------------------------------------------------------------------

void Demo_Histogram() {
    static ImPlotHistogramFlags hist_flags = ImPlotHistogramFlags_Density;
    static int  bins       = 50;
    static double mu       = 5;
    static double sigma    = 2;
    ImGui::SetNextItemWidth(200);
    if (ImGui::RadioButton("Sqrt",bins==ImPlotBin_Sqrt))       { bins = ImPlotBin_Sqrt;    } ImGui::SameLine();
    if (ImGui::RadioButton("Sturges",bins==ImPlotBin_Sturges)) { bins = ImPlotBin_Sturges; } ImGui::SameLine();
    if (ImGui::RadioButton("Rice",bins==ImPlotBin_Rice))       { bins = ImPlotBin_Rice;    } ImGui::SameLine();
    if (ImGui::RadioButton("Scott",bins==ImPlotBin_Scott))     { bins = ImPlotBin_Scott;   } ImGui::SameLine();
    if (ImGui::RadioButton("N Bins",bins>=0))                  { bins = 50;                }
    if (bins>=0) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::SliderInt("##Bins", &bins, 1, 100);
    }
    ImGui::CheckboxFlags("Horizontal", (unsigned int*)&hist_flags, ImPlotHistogramFlags_Horizontal);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Density", (unsigned int*)&hist_flags, ImPlotHistogramFlags_Density);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Cumulative", (unsigned int*)&hist_flags, ImPlotHistogramFlags_Cumulative);

    static bool range = false;
    ImGui::Checkbox("Range", &range);
    static float rmin = -3;
    static float rmax = 13;
    if (range) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::DragFloat2("##Range",&rmin,0.1f,-3,13);
        ImGui::SameLine();
        ImGui::CheckboxFlags("Exclude Outliers", (unsigned int*)&hist_flags, ImPlotHistogramFlags_NoOutliers);
    }
    static NormalDistribution<10000> dist(mu, sigma);
    static double x[100];
    static double y[100];
    if (hist_flags & ImPlotHistogramFlags_Density) {
        for (int i = 0; i < 100; ++i) {
            x[i] = -3 + 16 * (double)i/99.0;
            y[i] = exp( - (x[i]-mu)*(x[i]-mu) / (2*sigma*sigma)) / (sigma * sqrt(2*3.141592653589793238));
        }
        if (hist_flags & ImPlotHistogramFlags_Cumulative) {
            for (int i = 1; i < 100; ++i)
                y[i] += y[i-1];
            for (int i = 0; i < 100; ++i)
                y[i] /= y[99];
        }
    }

    if (ImPlot::BeginPlot("##Histograms")) {
        ImPlot::SetupAxes(NULL,NULL,ImPlotAxisFlags_AutoFit,ImPlotAxisFlags_AutoFit);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL,0.5f);
        ImPlot::PlotHistogram("Empirical", dist.Data, 10000, bins, 1.0, range ? ImPlotRange(rmin,rmax) : ImPlotRange(), hist_flags);
        if ((hist_flags & ImPlotHistogramFlags_Density) && !(hist_flags & ImPlotHistogramFlags_NoOutliers)) {
            if (hist_flags & ImPlotHistogramFlags_Horizontal)
                ImPlot::PlotLine("Theoretical",y,x,100);
            else
                ImPlot::PlotLine("Theoretical",x,y,100);
        }
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_Histogram2D() {
    static int count     = 50000;
    static int xybins[2] = {100,100};

    static ImPlotHistogramFlags hist_flags = 0;

    ImGui::SliderInt("Count",&count,100,100000);
    ImGui::SliderInt2("Bins",xybins,1,500);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Density", (unsigned int*)&hist_flags, ImPlotHistogramFlags_Density);

    static NormalDistribution<100000> dist1(1, 2);
    static NormalDistribution<100000> dist2(1, 1);
    double max_count = 0;
    ImPlotAxisFlags flags = ImPlotAxisFlags_AutoFit|ImPlotAxisFlags_Foreground;
    ImPlot::PushColormap("Hot");
    if (ImPlot::BeginPlot("##Hist2D",ImVec2(ImGui::GetContentRegionAvail().x-100-ImGui::GetStyle().ItemSpacing.x,0))) {
        ImPlot::SetupAxes(NULL, NULL, flags, flags);
        ImPlot::SetupAxesLimits(-6,6,-6,6);
        max_count = ImPlot::PlotHistogram2D("Hist2D",dist1.Data,dist2.Data,count,xybins[0],xybins[1],ImPlotRect(-6,6,-6,6), hist_flags);
        ImPlot::EndPlot();
    }
    ImGui::SameLine();
    ImPlot::ColormapScale(hist_flags & ImPlotHistogramFlags_Density ? "Density" : "Count",0,max_count,ImVec2(100,0));
    ImPlot::PopColormap();
}

//-----------------------------------------------------------------------------

void Demo_DigitalPlots() {
    ImGui::BulletText("Digital plots do not respond to Y drag and zoom, so that");
    ImGui::Indent();
    ImGui::Text("you can drag analog plots over the rising/falling digital edge.");
    ImGui::Unindent();

    static bool paused = false;
    static ScrollingBuffer dataDigital[2];
    static ScrollingBuffer dataAnalog[2];
    static bool showDigital[2] = {true, false};
    static bool showAnalog[2] = {true, false};

    char label[32];
    ImGui::Checkbox("digital_0", &showDigital[0]); ImGui::SameLine();
    ImGui::Checkbox("digital_1", &showDigital[1]); ImGui::SameLine();
    ImGui::Checkbox("analog_0",  &showAnalog[0]);  ImGui::SameLine();
    ImGui::Checkbox("analog_1",  &showAnalog[1]);

    static float t = 0;
    if (!paused) {
        t += ImGui::GetIO().DeltaTime;
        //digital signal values
        if (showDigital[0])
            dataDigital[0].AddPoint(t, sinf(2*t) > 0.45);
        if (showDigital[1])
            dataDigital[1].AddPoint(t, sinf(2*t) < 0.45);
        //Analog signal values
        if (showAnalog[0])
            dataAnalog[0].AddPoint(t, sinf(2*t));
        if (showAnalog[1])
            dataAnalog[1].AddPoint(t, cosf(2*t));
    }
    if (ImPlot::BeginPlot("##Digital")) {
        ImPlot::SetupAxisLimits(ImAxis_X1, t - 10.0, t, paused ? ImGuiCond_Once : ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1);
        for (int i = 0; i < 2; ++i) {
            if (showDigital[i] && dataDigital[i].Data.size() > 0) {
                snprintf(label, sizeof(label), "digital_%d", i);
                ImPlot::PlotDigital(label, &dataDigital[i].Data[0].x, &dataDigital[i].Data[0].y, dataDigital[i].Data.size(), 0, dataDigital[i].Offset, 2 * sizeof(float));
            }
        }
        for (int i = 0; i < 2; ++i) {
            if (showAnalog[i]) {
                snprintf(label, sizeof(label), "analog_%d", i);
                if (dataAnalog[i].Data.size() > 0)
                    ImPlot::PlotLine(label, &dataAnalog[i].Data[0].x, &dataAnalog[i].Data[0].y, dataAnalog[i].Data.size(), 0, dataAnalog[i].Offset, 2 * sizeof(float));
            }
        }
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_Images() {
    ImGui::BulletText("Below we are displaying the font texture, which is the only texture we have\naccess to in this demo.");
    ImGui::BulletText("Use the 'ImTextureID' type as storage to pass pointers or identifiers to your\nown texture data.");
    ImGui::BulletText("See ImGui Wiki page 'Image Loading and Displaying Examples'.");
    static ImVec2 bmin(0,0);
    static ImVec2 bmax(1,1);
    static ImVec2 uv0(0,0);
    static ImVec2 uv1(1,1);
    static ImVec4 tint(1,1,1,1);
    ImGui::SliderFloat2("Min", &bmin.x, -2, 2, "%.1f");
    ImGui::SliderFloat2("Max", &bmax.x, -2, 2, "%.1f");
    ImGui::SliderFloat2("UV0", &uv0.x, -2, 2, "%.1f");
    ImGui::SliderFloat2("UV1", &uv1.x, -2, 2, "%.1f");
    ImGui::ColorEdit4("Tint",&tint.x);
    if (ImPlot::BeginPlot("##image")) {
        ImPlot::PlotImage("my image",ImGui::GetIO().Fonts->TexID, bmin, bmax, uv0, uv1, tint);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_RealtimePlots() {
    ImGui::BulletText("Move your mouse to change the data!");
    ImGui::BulletText("This example assumes 60 FPS. Higher FPS requires larger buffer size.");
    static ScrollingBuffer sdata1, sdata2;
    static RollingBuffer   rdata1, rdata2;
    ImVec2 mouse = ImGui::GetMousePos();
    static float t = 0;
    t += ImGui::GetIO().DeltaTime;
    sdata1.AddPoint(t, mouse.x * 0.0005f);
    rdata1.AddPoint(t, mouse.x * 0.0005f);
    sdata2.AddPoint(t, mouse.y * 0.0005f);
    rdata2.AddPoint(t, mouse.y * 0.0005f);

    static float history = 10.0f;
    ImGui::SliderFloat("History",&history,1,30,"%.1f s");
    rdata1.Span = history;
    rdata2.Span = history;

    static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

    if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1,150))) {
        ImPlot::SetupAxes(NULL, NULL, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1,t - history, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1,0,1);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL,0.5f);
        ImPlot::PlotShaded("Mouse X", &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), -INFINITY, 0, sdata1.Offset, 2 * sizeof(float));
        ImPlot::PlotLine("Mouse Y", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), 0, sdata2.Offset, 2*sizeof(float));
        ImPlot::EndPlot();
    }
    if (ImPlot::BeginPlot("##Rolling", ImVec2(-1,150))) {
        ImPlot::SetupAxes(NULL, NULL, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1,0,history, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1,0,1);
        ImPlot::PlotLine("Mouse X", &rdata1.Data[0].x, &rdata1.Data[0].y, rdata1.Data.size(), 0, 0, 2 * sizeof(float));
        ImPlot::PlotLine("Mouse Y", &rdata2.Data[0].x, &rdata2.Data[0].y, rdata2.Data.size(), 0, 0, 2 * sizeof(float));
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_MarkersAndText() {
    static float mk_size = ImPlot::GetStyle().MarkerSize;
    static float mk_weight = ImPlot::GetStyle().MarkerWeight;
    ImGui::DragFloat("Marker Size",&mk_size,0.1f,2.0f,10.0f,"%.2f px");
    ImGui::DragFloat("Marker Weight", &mk_weight,0.05f,0.5f,3.0f,"%.2f px");

    if (ImPlot::BeginPlot("##MarkerStyles", ImVec2(-1,0), ImPlotFlags_CanvasOnly)) {

        ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, 10, 0, 12);

        ImS8 xs[2] = {1,4};
        ImS8 ys[2] = {10,11};

        // filled markers
        for (int m = 0; m < ImPlotMarker_COUNT; ++m) {
            ImGui::PushID(m);
            ImPlot::SetNextMarkerStyle(m, mk_size, IMPLOT_AUTO_COL, mk_weight);
            ImPlot::PlotLine("##Filled", xs, ys, 2);
            ImGui::PopID();
            ys[0]--; ys[1]--;
        }
        xs[0] = 6; xs[1] = 9; ys[0] = 10; ys[1] = 11;
        // open markers
        for (int m = 0; m < ImPlotMarker_COUNT; ++m) {
            ImGui::PushID(m);
            ImPlot::SetNextMarkerStyle(m, mk_size, ImVec4(0,0,0,0), mk_weight);
            ImPlot::PlotLine("##Open", xs, ys, 2);
            ImGui::PopID();
            ys[0]--; ys[1]--;
        }

        ImPlot::PlotText("Filled Markers", 2.5f, 6.0f);
        ImPlot::PlotText("Open Markers",   7.5f, 6.0f);

        ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(1,0,1,1));
        ImPlot::PlotText("Vertical Text", 5.0f, 6.0f, ImVec2(0,0), ImPlotTextFlags_Vertical);
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_NaNValues() {

    static bool include_nan = true;
    static ImPlotLineFlags flags = 0;

    float data1[5] = {0.0f,0.25f,0.5f,0.75f,1.0f};
    float data2[5] = {0.0f,0.25f,0.5f,0.75f,1.0f};

    if (include_nan)
        data1[2] = NAN;

    ImGui::Checkbox("Include NaN",&include_nan);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Skip NaN", (unsigned int*)&flags, ImPlotLineFlags_SkipNaN);

    if (ImPlot::BeginPlot("##NaNValues")) {
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Square);
        ImPlot::PlotLine("line", data1, data2, 5, flags);
        ImPlot::PlotBars("bars", data1, 5);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_LogScale() {
    static double xs[1001], ys1[1001], ys2[1001], ys3[1001];
    for (int i = 0; i < 1001; ++i) {
        xs[i]  = i*0.1f;
        ys1[i] = sin(xs[i]) + 1;
        ys2[i] = log(xs[i]);
        ys3[i] = pow(10.0, xs[i]);
    }
    if (ImPlot::BeginPlot("Log Plot", ImVec2(-1,0))) {
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
        ImPlot::SetupAxesLimits(0.1, 100, 0, 10);
        ImPlot::PlotLine("f(x) = x",        xs, xs,  1001);
        ImPlot::PlotLine("f(x) = sin(x)+1", xs, ys1, 1001);
        ImPlot::PlotLine("f(x) = log(x)",   xs, ys2, 1001);
        ImPlot::PlotLine("f(x) = 10^x",     xs, ys3, 21);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_SymmetricLogScale() {
    static double xs[1001], ys1[1001], ys2[1001];
    for (int i = 0; i < 1001; ++i) {
        xs[i]  = i*0.1f-50;
        ys1[i] = sin(xs[i]);
        ys2[i] = i*0.002 - 1;
    }
    if (ImPlot::BeginPlot("SymLog Plot", ImVec2(-1,0))) {
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_SymLog);
        ImPlot::PlotLine("f(x) = a*x+b",xs,ys2,1001);
        ImPlot::PlotLine("f(x) = sin(x)",xs,ys1,1001);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_TimeScale() {

    static double t_min = 1609459200; // 01/01/2021 @ 12:00:00am (UTC)
    static double t_max = 1640995200; // 01/01/2022 @ 12:00:00am (UTC)

    ImGui::BulletText("When ImPlotAxisFlags_Time is enabled on the X-Axis, values are interpreted as\n"
                        "UNIX timestamps in seconds and axis labels are formated as date/time.");
    ImGui::BulletText("By default, labels are in UTC time but can be set to use local time instead.");

    ImGui::Checkbox("Local Time",&ImPlot::GetStyle().UseLocalTime);
    ImGui::SameLine();
    ImGui::Checkbox("ISO 8601",&ImPlot::GetStyle().UseISO8601);
    ImGui::SameLine();
    ImGui::Checkbox("24 Hour Clock",&ImPlot::GetStyle().Use24HourClock);

    static HugeTimeData* data = NULL;
    if (data == NULL) {
        ImGui::SameLine();
        if (ImGui::Button("Generate Huge Data (~500MB!)")) {
            static HugeTimeData sdata(t_min);
            data = &sdata;
        }
    }

    if (ImPlot::BeginPlot("##Time", ImVec2(-1,0))) {
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
        ImPlot::SetupAxesLimits(t_min,t_max,0,1);
        if (data != NULL) {
            // downsample our data
            int downsample = (int)ImPlot::GetPlotLimits().X.Size() / 1000 + 1;
            int start = (int)(ImPlot::GetPlotLimits().X.Min - t_min);
            start = start < 0 ? 0 : start > HugeTimeData::Size - 1 ? HugeTimeData::Size - 1 : start;
            int end = (int)(ImPlot::GetPlotLimits().X.Max - t_min) + 1000;
            end = end < 0 ? 0 : end > HugeTimeData::Size - 1 ? HugeTimeData::Size - 1 : end;
            int size = (end - start)/downsample;
            // plot it
            ImPlot::PlotLine("Time Series", &data->Ts[start], &data->Ys[start], size, 0, 0, sizeof(double)*downsample);
        }
        // plot time now
        double t_now = (double)time(0);
        double y_now = HugeTimeData::GetY(t_now);
        ImPlot::PlotScatter("Now",&t_now,&y_now,1);
        ImPlot::Annotation(t_now,y_now,ImPlot::GetLastItemColor(),ImVec2(10,10),false,"Now");
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

static inline double TransformForward_Sqrt(double v, void*) {
    return sqrt(v);
}

static inline double TransformInverse_Sqrt(double v, void*) {
    return v*v;
}

void Demo_CustomScale() {
    static float v[100];
    for (int i = 0; i < 100; ++i) {
        v[i] = i*0.01f;
    }
    if (ImPlot::BeginPlot("Sqrt")) {
        ImPlot::SetupAxis(ImAxis_X1, "Linear");
        ImPlot::SetupAxis(ImAxis_Y1, "Sqrt");
        ImPlot::SetupAxisScale(ImAxis_Y1, TransformForward_Sqrt, TransformInverse_Sqrt);
        ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0, INFINITY);
        ImPlot::PlotLine("##data",v,v,100);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_MultipleAxes() {
    static float xs[1001], xs2[1001], ys1[1001], ys2[1001], ys3[1001];
    for (int i = 0; i < 1001; ++i) {
        xs[i]  = (i*0.1f);
        xs2[i] = xs[i] + 10.0f;
        ys1[i] = sinf(xs[i]) * 3 + 1;
        ys2[i] = cosf(xs[i]) * 0.2f + 0.5f;
        ys3[i] = sinf(xs[i]+0.5f) * 100 + 200;
    }

    static bool x2_axis = true;
    static bool y2_axis = true;
    static bool y3_axis = true;

    ImGui::Checkbox("X-Axis 2", &x2_axis);
    ImGui::SameLine();
    ImGui::Checkbox("Y-Axis 2", &y2_axis);
    ImGui::SameLine();
    ImGui::Checkbox("Y-Axis 3", &y3_axis);

    ImGui::BulletText("You can drag axes to the opposite side of the plot.");
    ImGui::BulletText("Hover over legend items to see which axis they are plotted on.");

    if (ImPlot::BeginPlot("Multi-Axis Plot", ImVec2(-1,0))) {
        ImPlot::SetupAxes("X-Axis 1", "Y-Axis 1");
        ImPlot::SetupAxesLimits(0, 100, 0, 10);
        if (x2_axis) {
            ImPlot::SetupAxis(ImAxis_X2, "X-Axis 2",ImPlotAxisFlags_AuxDefault);
            ImPlot::SetupAxisLimits(ImAxis_X2, 0, 100);
        }
        if (y2_axis) {
            ImPlot::SetupAxis(ImAxis_Y2, "Y-Axis 2",ImPlotAxisFlags_AuxDefault);
            ImPlot::SetupAxisLimits(ImAxis_Y2, 0, 1);
        }
        if (y3_axis) {
            ImPlot::SetupAxis(ImAxis_Y3, "Y-Axis 3",ImPlotAxisFlags_AuxDefault);
            ImPlot::SetupAxisLimits(ImAxis_Y3, 0, 300);
        }

        ImPlot::PlotLine("f(x) = x", xs, xs, 1001);
        if (x2_axis) {
            ImPlot::SetAxes(ImAxis_X2, ImAxis_Y1);
            ImPlot::PlotLine("f(x) = sin(x)*3+1", xs2, ys1, 1001);
        }
        if (y2_axis) {
            ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
            ImPlot::PlotLine("f(x) = cos(x)*.2+.5", xs, ys2, 1001);
        }
        if (y3_axis) {
            ImPlot::SetAxes(ImAxis_X2, ImAxis_Y3);
            ImPlot::PlotLine("f(x) = sin(x+.5)*100+200 ", xs2, ys3, 1001);
        }
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_LinkedAxes() {
    static ImPlotRect lims(0,1,0,1);
    static bool linkx = true, linky = true;
    int data[2] = {0,1};
    ImGui::Checkbox("Link X", &linkx);
    ImGui::SameLine();
    ImGui::Checkbox("Link Y", &linky);

    ImGui::DragScalarN("Limits",ImGuiDataType_Double,&lims.X.Min,4,0.01f);

    if (BeginAlignedPlots("AlignedGroup")) {
        if (ImPlot::BeginPlot("Plot A")) {
            ImPlot::SetupAxisLinks(ImAxis_X1, linkx ? &lims.X.Min : NULL, linkx ? &lims.X.Max : NULL);
            ImPlot::SetupAxisLinks(ImAxis_Y1, linky ? &lims.Y.Min : NULL, linky ? &lims.Y.Max : NULL);
            ImPlot::PlotLine("Line",data,2);
            ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("Plot B")) {
            ImPlot::SetupAxisLinks(ImAxis_X1, linkx ? &lims.X.Min : NULL, linkx ? &lims.X.Max : NULL);
            ImPlot::SetupAxisLinks(ImAxis_Y1, linky ? &lims.Y.Min : NULL, linky ? &lims.Y.Max : NULL);
            ImPlot::PlotLine("Line",data,2);
            ImPlot::EndPlot();
        }
        ImPlot::EndAlignedPlots();
    }
}

//-----------------------------------------------------------------------------

void Demo_AxisConstraints() {
    static float constraints[4] = {-10,10,1,20};
    static ImPlotAxisFlags flags;
    ImGui::DragFloat2("Limits Constraints", &constraints[0], 0.01f);
    ImGui::DragFloat2("Zoom Constraints", &constraints[2], 0.01f);
    CHECKBOX_FLAG(flags, ImPlotAxisFlags_PanStretch);
    if (ImPlot::BeginPlot("##AxisConstraints",ImVec2(-1,0))) {
        ImPlot::SetupAxes("X","Y",flags,flags);
        ImPlot::SetupAxesLimits(-1,1,-1,1);
        ImPlot::SetupAxisLimitsConstraints(ImAxis_X1,constraints[0], constraints[1]);
        ImPlot::SetupAxisZoomConstraints(ImAxis_X1,constraints[2], constraints[3]);
        ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1,constraints[0], constraints[1]);
        ImPlot::SetupAxisZoomConstraints(ImAxis_Y1,constraints[2], constraints[3]);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_EqualAxes() {
    ImGui::BulletText("Equal constraint applies to axis pairs (e.g ImAxis_X1/Y1, ImAxis_X2/Y2)");
    static double xs1[360], ys1[360];
    for (int i = 0; i < 360; ++i) {
        double angle = i * 2 * PI / 359.0;
        xs1[i] = cos(angle); ys1[i] = sin(angle);
    }
    float xs2[] = {-1,0,1,0,-1};
    float ys2[] = {0,1,0,-1,0};
    if (ImPlot::BeginPlot("##EqualAxes",ImVec2(-1,0),ImPlotFlags_Equal)) {
        ImPlot::SetupAxis(ImAxis_X2, NULL, ImPlotAxisFlags_AuxDefault);
        ImPlot::SetupAxis(ImAxis_Y2, NULL, ImPlotAxisFlags_AuxDefault);
        ImPlot::PlotLine("Circle",xs1,ys1,360);
        ImPlot::SetAxes(ImAxis_X2, ImAxis_Y2);
        ImPlot::PlotLine("Diamond",xs2,ys2,5);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_AutoFittingData() {
    ImGui::BulletText("The Y-axis has been configured to auto-fit to only the data visible in X-axis range.");
    ImGui::BulletText("Zoom and pan the X-axis. Disable Stems to see a difference in fit.");
    ImGui::BulletText("If ImPlotAxisFlags_RangeFit is disabled, the axis will fit ALL data.");

    static ImPlotAxisFlags xflags = ImPlotAxisFlags_None;
    static ImPlotAxisFlags yflags = ImPlotAxisFlags_AutoFit|ImPlotAxisFlags_RangeFit;

    ImGui::TextUnformatted("X: "); ImGui::SameLine();
    ImGui::CheckboxFlags("ImPlotAxisFlags_AutoFit##X", (unsigned int*)&xflags, ImPlotAxisFlags_AutoFit); ImGui::SameLine();
    ImGui::CheckboxFlags("ImPlotAxisFlags_RangeFit##X", (unsigned int*)&xflags, ImPlotAxisFlags_RangeFit);

    ImGui::TextUnformatted("Y: "); ImGui::SameLine();
    ImGui::CheckboxFlags("ImPlotAxisFlags_AutoFit##Y", (unsigned int*)&yflags, ImPlotAxisFlags_AutoFit); ImGui::SameLine();
    ImGui::CheckboxFlags("ImPlotAxisFlags_RangeFit##Y", (unsigned int*)&yflags, ImPlotAxisFlags_RangeFit);

    static double data[101];
    srand(0);
    for (int i = 0; i < 101; ++i)
        data[i] = 1 + sin(i/10.0f);

    if (ImPlot::BeginPlot("##DataFitting")) {
        ImPlot::SetupAxes("X","Y",xflags,yflags);
        ImPlot::PlotLine("Line",data,101);
        ImPlot::PlotStems("Stems",data,101);
        ImPlot::EndPlot();
    };
}

//-----------------------------------------------------------------------------

ImPlotPoint SinewaveGetter(int i, void* data) {
    float f = *(float*)data;
    return ImPlotPoint(i,sinf(f*i));
}

void Demo_SubplotsSizing() {

    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;
    ImGui::CheckboxFlags("ImPlotSubplotFlags_NoResize", (unsigned int*)&flags, ImPlotSubplotFlags_NoResize);
    ImGui::CheckboxFlags("ImPlotSubplotFlags_NoTitle", (unsigned int*)&flags, ImPlotSubplotFlags_NoTitle);

    static int rows = 3;
    static int cols = 3;
    ImGui::SliderInt("Rows",&rows,1,5);
    ImGui::SliderInt("Cols",&cols,1,5);
    static float rratios[] = {5,1,1,1,1,1};
    static float cratios[] = {5,1,1,1,1,1};
    ImGui::DragScalarN("Row Ratios",ImGuiDataType_Float,rratios,rows,0.01f,0);
    ImGui::DragScalarN("Col Ratios",ImGuiDataType_Float,cratios,cols,0.01f,0);
    if (ImPlot::BeginSubplots("My Subplots", rows, cols, ImVec2(-1,400), flags, rratios, cratios)) {
        for (int i = 0; i < rows*cols; ++i) {
            if (ImPlot::BeginPlot("",ImVec2(),ImPlotFlags_NoLegend)) {
                ImPlot::SetupAxes(NULL,NULL,ImPlotAxisFlags_NoDecorations,ImPlotAxisFlags_NoDecorations);
                float fi = 0.01f * (i+1);
                ImPlot::SetNextLineStyle(SampleColormap((float)i/(float)(rows*cols-1),ImPlotColormap_Jet));
                ImPlot::PlotLineG("data",SinewaveGetter,&fi,1000);
                ImPlot::EndPlot();
            }
        }
        ImPlot::EndSubplots();
    }
}

//-----------------------------------------------------------------------------

void Demo_SubplotItemSharing() {
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_ShareItems;
    ImGui::CheckboxFlags("ImPlotSubplotFlags_ShareItems", (unsigned int*)&flags, ImPlotSubplotFlags_ShareItems);
    ImGui::CheckboxFlags("ImPlotSubplotFlags_ColMajor", (unsigned int*)&flags, ImPlotSubplotFlags_ColMajor);
    ImGui::BulletText("Drag and drop items from the legend onto plots (except for 'common')");
    static int rows = 2;
    static int cols = 3;
    static int id[] = {0,1,2,3,4,5};
    static int curj = -1;
    if (ImPlot::BeginSubplots("##ItemSharing", rows, cols, ImVec2(-1,400), flags)) {
        for (int i = 0; i < rows*cols; ++i) {
            if (ImPlot::BeginPlot("")) {
                float fc = 0.01f;
                ImPlot::PlotLineG("common",SinewaveGetter,&fc,1000);
                for (int j = 0; j < 6; ++j) {
                    if (id[j] == i) {
                        char label[8];
                        float fj = 0.01f * (j+2);
                        snprintf(label, sizeof(label), "data%d", j);
                        ImPlot::PlotLineG(label,SinewaveGetter,&fj,1000);
                        if (ImPlot::BeginDragDropSourceItem(label)) {
                            curj = j;
                            ImGui::SetDragDropPayload("MY_DND",NULL,0);
                            ImPlot::ItemIcon(GetLastItemColor()); ImGui::SameLine();
                            ImGui::TextUnformatted(label);
                            ImPlot::EndDragDropSource();
                        }
                    }
                }
                if (ImPlot::BeginDragDropTargetPlot()) {
                    if (ImGui::AcceptDragDropPayload("MY_DND"))
                        id[curj] = i;
                    ImPlot::EndDragDropTarget();
                }
                ImPlot::EndPlot();
            }
        }
        ImPlot::EndSubplots();
    }
}

//-----------------------------------------------------------------------------

void Demo_SubplotAxisLinking() {
    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_LinkRows | ImPlotSubplotFlags_LinkCols;
    ImGui::CheckboxFlags("ImPlotSubplotFlags_LinkRows", (unsigned int*)&flags, ImPlotSubplotFlags_LinkRows);
    ImGui::CheckboxFlags("ImPlotSubplotFlags_LinkCols", (unsigned int*)&flags, ImPlotSubplotFlags_LinkCols);
    ImGui::CheckboxFlags("ImPlotSubplotFlags_LinkAllX", (unsigned int*)&flags, ImPlotSubplotFlags_LinkAllX);
    ImGui::CheckboxFlags("ImPlotSubplotFlags_LinkAllY", (unsigned int*)&flags, ImPlotSubplotFlags_LinkAllY);

    static int rows = 2;
    static int cols = 2;
    if (ImPlot::BeginSubplots("##AxisLinking", rows, cols, ImVec2(-1,400), flags)) {
        for (int i = 0; i < rows*cols; ++i) {
            if (ImPlot::BeginPlot("")) {
                ImPlot::SetupAxesLimits(0,1000,-1,1);
                float fc = 0.01f;
                ImPlot::PlotLineG("common",SinewaveGetter,&fc,1000);
                ImPlot::EndPlot();
            }
        }
        ImPlot::EndSubplots();
    }
}

//-----------------------------------------------------------------------------

void Demo_LegendOptions() {
    static ImPlotLocation loc = ImPlotLocation_East;
    ImGui::CheckboxFlags("North", (unsigned int*)&loc, ImPlotLocation_North); ImGui::SameLine();
    ImGui::CheckboxFlags("South", (unsigned int*)&loc, ImPlotLocation_South); ImGui::SameLine();
    ImGui::CheckboxFlags("West",  (unsigned int*)&loc, ImPlotLocation_West);  ImGui::SameLine();
    ImGui::CheckboxFlags("East",  (unsigned int*)&loc, ImPlotLocation_East);

    static ImPlotLegendFlags flags = 0;

    CHECKBOX_FLAG(flags, ImPlotLegendFlags_Horizontal);
    CHECKBOX_FLAG(flags, ImPlotLegendFlags_Outside);
    CHECKBOX_FLAG(flags, ImPlotLegendFlags_Sort);

    ImGui::SliderFloat2("LegendPadding", (float*)&GetStyle().LegendPadding, 0.0f, 20.0f, "%.0f");
    ImGui::SliderFloat2("LegendInnerPadding", (float*)&GetStyle().LegendInnerPadding, 0.0f, 10.0f, "%.0f");
    ImGui::SliderFloat2("LegendSpacing", (float*)&GetStyle().LegendSpacing, 0.0f, 5.0f, "%.0f");

    if (ImPlot::BeginPlot("##Legend",ImVec2(-1,0))) {
        ImPlot::SetupLegend(loc, flags);
        static MyImPlot::WaveData data1(0.001, 0.2, 4, 0.2);
        static MyImPlot::WaveData data2(0.001, 0.2, 4, 0.4);
        static MyImPlot::WaveData data3(0.001, 0.2, 4, 0.6);
        static MyImPlot::WaveData data4(0.001, 0.2, 4, 0.8);
        static MyImPlot::WaveData data5(0.001, 0.2, 4, 1.0);

        ImPlot::PlotLineG("Item B", MyImPlot::SawWave, &data1, 1000);         // "Item B" added to legend
        ImPlot::PlotLineG("Item A##IDText", MyImPlot::SawWave, &data2, 1000);  // "Item A" added to legend, text after ## used for ID only
        ImPlot::PlotLineG("##NotListed", MyImPlot::SawWave, &data3, 1000);     // plotted, but not added to legend
        ImPlot::PlotLineG("Item C", MyImPlot::SawWave, &data4, 1000);         // "Item C" added to legend
        ImPlot::PlotLineG("Item C", MyImPlot::SawWave,  &data5, 1000);         // combined with previous "Item C"

        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_DragPoints() {
    ImGui::BulletText("Click and drag each point.");
    static ImPlotDragToolFlags flags = ImPlotDragToolFlags_None;
    ImGui::CheckboxFlags("NoCursors", (unsigned int*)&flags, ImPlotDragToolFlags_NoCursors); ImGui::SameLine();
    ImGui::CheckboxFlags("NoFit", (unsigned int*)&flags, ImPlotDragToolFlags_NoFit); ImGui::SameLine();
    ImGui::CheckboxFlags("NoInput", (unsigned int*)&flags, ImPlotDragToolFlags_NoInputs);
    ImPlotAxisFlags ax_flags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks;
    if (ImPlot::BeginPlot("##Bezier",ImVec2(-1,0),ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes(0,0,ax_flags,ax_flags);
        ImPlot::SetupAxesLimits(0,1,0,1);
        static ImPlotPoint P[] = {ImPlotPoint(.05f,.05f), ImPlotPoint(0.2,0.4),  ImPlotPoint(0.8,0.6),  ImPlotPoint(.95f,.95f)};

        ImPlot::DragPoint(0,&P[0].x,&P[0].y, ImVec4(0,0.9f,0,1),4,flags);
        ImPlot::DragPoint(1,&P[1].x,&P[1].y, ImVec4(1,0.5f,1,1),4,flags);
        ImPlot::DragPoint(2,&P[2].x,&P[2].y, ImVec4(0,0.5f,1,1),4,flags);
        ImPlot::DragPoint(3,&P[3].x,&P[3].y, ImVec4(0,0.9f,0,1),4,flags);

        static ImPlotPoint B[100];
        for (int i = 0; i < 100; ++i) {
            double t  = i / 99.0;
            double u  = 1 - t;
            double w1 = u*u*u;
            double w2 = 3*u*u*t;
            double w3 = 3*u*t*t;
            double w4 = t*t*t;
            B[i] = ImPlotPoint(w1*P[0].x + w2*P[1].x + w3*P[2].x + w4*P[3].x, w1*P[0].y + w2*P[1].y + w3*P[2].y + w4*P[3].y);
        }


        ImPlot::SetNextLineStyle(ImVec4(1,0.5f,1,1));
        ImPlot::PlotLine("##h1",&P[0].x, &P[0].y, 2, 0, 0, sizeof(ImPlotPoint));
        ImPlot::SetNextLineStyle(ImVec4(0,0.5f,1,1));
        ImPlot::PlotLine("##h2",&P[2].x, &P[2].y, 2, 0, 0, sizeof(ImPlotPoint));
        ImPlot::SetNextLineStyle(ImVec4(0,0.9f,0,1), 2);
        ImPlot::PlotLine("##bez",&B[0].x, &B[0].y, 100, 0, 0, sizeof(ImPlotPoint));

        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_DragLines() {
    ImGui::BulletText("Click and drag the horizontal and vertical lines.");
    static double x1 = 0.2;
    static double x2 = 0.8;
    static double y1 = 0.25;
    static double y2 = 0.75;
    static double f = 0.1;
    static ImPlotDragToolFlags flags = ImPlotDragToolFlags_None;
    ImGui::CheckboxFlags("NoCursors", (unsigned int*)&flags, ImPlotDragToolFlags_NoCursors); ImGui::SameLine();
    ImGui::CheckboxFlags("NoFit", (unsigned int*)&flags, ImPlotDragToolFlags_NoFit); ImGui::SameLine();
    ImGui::CheckboxFlags("NoInput", (unsigned int*)&flags, ImPlotDragToolFlags_NoInputs);
    if (ImPlot::BeginPlot("##lines",ImVec2(-1,0))) {
        ImPlot::SetupAxesLimits(0,1,0,1);
        ImPlot::DragLineX(0,&x1,ImVec4(1,1,1,1),1,flags);
        ImPlot::DragLineX(1,&x2,ImVec4(1,1,1,1),1,flags);
        ImPlot::DragLineY(2,&y1,ImVec4(1,1,1,1),1,flags);
        ImPlot::DragLineY(3,&y2,ImVec4(1,1,1,1),1,flags);
        double xs[1000], ys[1000];
        for (int i = 0; i < 1000; ++i) {
            xs[i] = (x2+x1)/2+fabs(x2-x1)*(i/1000.0f - 0.5f);
            ys[i] = (y1+y2)/2+fabs(y2-y1)/2*sin(f*i/10);
        }
        ImPlot::PlotLine("Interactive Data", xs, ys, 1000);
        ImPlot::DragLineY(120482,&f,ImVec4(1,0.5f,1,1),1,flags);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_DragRects() {

    static float x_data[512];
    static float y_data1[512];
    static float y_data2[512];
    static float y_data3[512];
    static float sampling_freq = 44100;
    static float freq = 500;
    for (size_t i = 0; i < 512; ++i) {
        const float t = i / sampling_freq;
        x_data[i] = t;
        const float arg = 2 * 3.14f * freq * t;
        y_data1[i] = sinf(arg);
        y_data2[i] = y_data1[i] * -0.6f + sinf(2 * arg) * 0.4f;
        y_data3[i] = y_data2[i] * -0.6f + sinf(3 * arg) * 0.4f;
    }
    ImGui::BulletText("Click and drag the edges, corners, and center of the rect.");
    static ImPlotRect rect(0.0025,0.0045,0,0.5);
    static ImPlotDragToolFlags flags = ImPlotDragToolFlags_None;
    ImGui::CheckboxFlags("NoCursors", (unsigned int*)&flags, ImPlotDragToolFlags_NoCursors); ImGui::SameLine();
    ImGui::CheckboxFlags("NoFit", (unsigned int*)&flags, ImPlotDragToolFlags_NoFit); ImGui::SameLine();
    ImGui::CheckboxFlags("NoInput", (unsigned int*)&flags, ImPlotDragToolFlags_NoInputs);

    if (ImPlot::BeginPlot("##Main",ImVec2(-1,150))) {
        ImPlot::SetupAxes(NULL,NULL,ImPlotAxisFlags_NoTickLabels,ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxesLimits(0,0.01,-1,1);
        ImPlot::PlotLine("Signal 1", x_data, y_data1, 512);
        ImPlot::PlotLine("Signal 2", x_data, y_data2, 512);
        ImPlot::PlotLine("Signal 3", x_data, y_data3, 512);
        ImPlot::DragRect(0,&rect.X.Min,&rect.Y.Min,&rect.X.Max,&rect.Y.Max,ImVec4(1,0,1,1),flags);
        ImPlot::EndPlot();
    }
    if (ImPlot::BeginPlot("##rect",ImVec2(-1,150), ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes(NULL,NULL,ImPlotAxisFlags_NoDecorations,ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(rect.X.Min, rect.X.Max, rect.Y.Min, rect.Y.Max, ImGuiCond_Always);
        ImPlot::PlotLine("Signal 1", x_data, y_data1, 512);
        ImPlot::PlotLine("Signal 2", x_data, y_data2, 512);
        ImPlot::PlotLine("Signal 3", x_data, y_data3, 512);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

ImPlotPoint FindCentroid(const ImVector<ImPlotPoint>& data, const ImPlotRect& bounds, int& cnt) {
    cnt = 0;
    ImPlotPoint avg;
    ImPlotRect bounds_fixed;
    bounds_fixed.X.Min = bounds.X.Min < bounds.X.Max ? bounds.X.Min : bounds.X.Max;
    bounds_fixed.X.Max = bounds.X.Min < bounds.X.Max ? bounds.X.Max : bounds.X.Min;
    bounds_fixed.Y.Min = bounds.Y.Min < bounds.Y.Max ? bounds.Y.Min : bounds.Y.Max;
    bounds_fixed.Y.Max = bounds.Y.Min < bounds.Y.Max ? bounds.Y.Max : bounds.Y.Min;
    for (int i = 0; i < data.size(); ++i) {
        if (bounds_fixed.Contains(data[i].x, data[i].y)) {
            avg.x += data[i].x;
            avg.y += data[i].y;
            cnt++;
        }
    }
    if (cnt > 0) {
        avg.x = avg.x / cnt;
        avg.y = avg.y / cnt;
    }
    return avg;
}

//-----------------------------------------------------------------------------

void Demo_Querying() {
    static ImVector<ImPlotPoint> data;
    static ImVector<ImPlotRect> rects;
    static ImPlotRect limits, select;
    static bool init = true;
    if (init) {
        for (int i = 0; i < 50; ++i)
        {
            double x = RandomRange(0.1, 0.9);
            double y = RandomRange(0.1, 0.9);
            data.push_back(ImPlotPoint(x,y));
        }
        init = false;
    }

    ImGui::BulletText("Box select and left click mouse to create a new query rect.");
    ImGui::BulletText("Ctrl + click in the plot area to draw points.");

    if (ImGui::Button("Clear Queries"))
        rects.shrink(0);

    if (ImPlot::BeginPlot("##Centroid")) {
        ImPlot::SetupAxesLimits(0,1,0,1);
        if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(0) && ImGui::GetIO().KeyCtrl) {
            ImPlotPoint pt = ImPlot::GetPlotMousePos();
            data.push_back(pt);
        }
        ImPlot::PlotScatter("Points", &data[0].x, &data[0].y, data.size(), 0, 0, 2 * sizeof(double));
        if (ImPlot::IsPlotSelected()) {
            select = ImPlot::GetPlotSelection();
            int cnt;
            ImPlotPoint centroid = FindCentroid(data,select,cnt);
            if (cnt > 0) {
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Square,6);
                ImPlot::PlotScatter("Centroid", &centroid.x, &centroid.y, 1);
            }
            if (ImGui::IsMouseClicked(ImPlot::GetInputMap().SelectCancel)) {
                CancelPlotSelection();
                rects.push_back(select);
            }
        }
        for (int i = 0; i < rects.size(); ++i) {
            int cnt;
            ImPlotPoint centroid = FindCentroid(data,rects[i],cnt);
            if (cnt > 0) {
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Square,6);
                ImPlot::PlotScatter("Centroid", &centroid.x, &centroid.y, 1);
            }
            ImPlot::DragRect(i,&rects[i].X.Min,&rects[i].Y.Min,&rects[i].X.Max,&rects[i].Y.Max,ImVec4(1,0,1,1));
        }
        limits  = ImPlot::GetPlotLimits();
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_Annotations() {
    static bool clamp = false;
    ImGui::Checkbox("Clamp",&clamp);
    if (ImPlot::BeginPlot("##Annotations")) {
        ImPlot::SetupAxesLimits(0,2,0,1);
        static float p[] = {0.25f, 0.25f, 0.75f, 0.75f, 0.25f};
        ImPlot::PlotScatter("##Points",&p[0],&p[1],4);
        ImVec4 col = GetLastItemColor();
        ImPlot::Annotation(0.25,0.25,col,ImVec2(-15,15),clamp,"BL");
        ImPlot::Annotation(0.75,0.25,col,ImVec2(15,15),clamp,"BR");
        ImPlot::Annotation(0.75,0.75,col,ImVec2(15,-15),clamp,"TR");
        ImPlot::Annotation(0.25,0.75,col,ImVec2(-15,-15),clamp,"TL");
        ImPlot::Annotation(0.5,0.5,col,ImVec2(0,0),clamp,"Center");

        ImPlot::Annotation(1.25,0.75,ImVec4(0,1,0,1),ImVec2(0,0),clamp);

        float bx[] = {1.2f,1.5f,1.8f};
        float by[] = {0.25f, 0.5f, 0.75f};
        ImPlot::PlotBars("##Bars",bx,by,3,0.2);
        for (int i = 0; i < 3; ++i)
            ImPlot::Annotation(bx[i],by[i],ImVec4(0,0,0,0),ImVec2(0,-5),clamp,"B[%d]=%.2f",i,by[i]);
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_Tags() {
    static bool show = true;
    ImGui::Checkbox("Show Tags",&show);
    if (ImPlot::BeginPlot("##Tags")) {
        ImPlot::SetupAxis(ImAxis_X2);
        ImPlot::SetupAxis(ImAxis_Y2);
        if (show) {
            ImPlot::TagX(0.25, ImVec4(1,1,0,1));
            ImPlot::TagY(0.75, ImVec4(1,1,0,1));
            static double drag_tag = 0.25;
            ImPlot::DragLineY(0,&drag_tag,ImVec4(1,0,0,1),1,ImPlotDragToolFlags_NoFit);
            ImPlot::TagY(drag_tag, ImVec4(1,0,0,1), "Drag");
            SetAxes(ImAxis_X2, ImAxis_Y2);
            ImPlot::TagX(0.5, ImVec4(0,1,1,1), "%s", "MyTag");
            ImPlot::TagY(0.5, ImVec4(0,1,1,1), "Tag: %d", 42);
        }
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_DragAndDrop() {
    ImGui::BulletText("Drag/drop items from the left column.");
    ImGui::BulletText("Drag/drop items between plots.");
    ImGui::Indent();
    ImGui::BulletText("Plot 1 Targets: Plot, Y-Axes, Legend");
    ImGui::BulletText("Plot 1 Sources: Legend Item Labels");
    ImGui::BulletText("Plot 2 Targets: Plot, X-Axis, Y-Axis");
    ImGui::BulletText("Plot 2 Sources: Plot, X-Axis, Y-Axis (hold Ctrl)");
    ImGui::Unindent();

    // convenience struct to manage DND items; do this however you like
    struct MyDndItem {
        int              Idx;
        int              Plt;
        ImAxis           Yax;
        char             Label[16];
        ImVector<ImVec2> Data;
        ImVec4           Color;
        MyDndItem()        {
            static int i = 0;
            Idx = i++;
            Plt = 0;
            Yax = ImAxis_Y1;
            snprintf(Label, sizeof(Label), "%02d Hz", Idx+1);
            Color = RandomColor();
            Data.reserve(1001);
            for (int k = 0; k < 1001; ++k) {
                float t = k * 1.0f / 999;
                Data.push_back(ImVec2(t, 0.5f + 0.5f * sinf(2*3.14f*t*(Idx+1))));
            }
        }
        void Reset() { Plt = 0; Yax = ImAxis_Y1; }
    };

    const int         k_dnd = 20;
    static MyDndItem  dnd[k_dnd];
    static MyDndItem* dndx = NULL; // for plot 2
    static MyDndItem* dndy = NULL; // for plot 2

    // child window to serve as initial source for our DND items
    ImGui::BeginChild("DND_LEFT",ImVec2(100,400));
    if (ImGui::Button("Reset Data")) {
        for (int k = 0; k < k_dnd; ++k)
            dnd[k].Reset();
        dndx = dndy = NULL;
    }
    for (int k = 0; k < k_dnd; ++k) {
        if (dnd[k].Plt > 0)
            continue;
        ImPlot::ItemIcon(dnd[k].Color); ImGui::SameLine();
        ImGui::Selectable(dnd[k].Label, false, 0, ImVec2(100, 0));
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("MY_DND", &k, sizeof(int));
            ImPlot::ItemIcon(dnd[k].Color); ImGui::SameLine();
            ImGui::TextUnformatted(dnd[k].Label);
            ImGui::EndDragDropSource();
        }
    }
    ImGui::EndChild();
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
            int i = *(int*)payload->Data; dnd[i].Reset();
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    ImGui::BeginChild("DND_RIGHT",ImVec2(-1,400));
    // plot 1 (time series)
    ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoHighlight;
    if (ImPlot::BeginPlot("##DND1", ImVec2(-1,195))) {
        ImPlot::SetupAxis(ImAxis_X1, NULL, flags|ImPlotAxisFlags_Lock);
        ImPlot::SetupAxis(ImAxis_Y1, "[drop here]", flags);
        ImPlot::SetupAxis(ImAxis_Y2, "[drop here]", flags|ImPlotAxisFlags_Opposite);
        ImPlot::SetupAxis(ImAxis_Y3, "[drop here]", flags|ImPlotAxisFlags_Opposite);

        for (int k = 0; k < k_dnd; ++k) {
            if (dnd[k].Plt == 1 && dnd[k].Data.size() > 0) {
                ImPlot::SetAxis(dnd[k].Yax);
                ImPlot::SetNextLineStyle(dnd[k].Color);
                ImPlot::PlotLine(dnd[k].Label, &dnd[k].Data[0].x, &dnd[k].Data[0].y, dnd[k].Data.size(), 0, 0, 2 * sizeof(float));
                // allow legend item labels to be DND sources
                if (ImPlot::BeginDragDropSourceItem(dnd[k].Label)) {
                    ImGui::SetDragDropPayload("MY_DND", &k, sizeof(int));
                    ImPlot::ItemIcon(dnd[k].Color); ImGui::SameLine();
                    ImGui::TextUnformatted(dnd[k].Label);
                    ImPlot::EndDragDropSource();
                }
            }
        }
        // allow the main plot area to be a DND target
        if (ImPlot::BeginDragDropTargetPlot()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dnd[i].Plt = 1; dnd[i].Yax = ImAxis_Y1;
            }
            ImPlot::EndDragDropTarget();
        }
        // allow each y-axis to be a DND target
        for (int y = ImAxis_Y1; y <= ImAxis_Y3; ++y) {
            if (ImPlot::BeginDragDropTargetAxis(y)) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                    int i = *(int*)payload->Data; dnd[i].Plt = 1; dnd[i].Yax = y;
                }
                ImPlot::EndDragDropTarget();
            }
        }
        // allow the legend to be a DND target
        if (ImPlot::BeginDragDropTargetLegend()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dnd[i].Plt = 1; dnd[i].Yax = ImAxis_Y1;
            }
            ImPlot::EndDragDropTarget();
        }
        ImPlot::EndPlot();
    }
    // plot 2 (Lissajous)
    if (ImPlot::BeginPlot("##DND2", ImVec2(-1,195))) {
        ImPlot::PushStyleColor(ImPlotCol_AxisBg, dndx != NULL ? dndx->Color : ImPlot::GetStyle().Colors[ImPlotCol_AxisBg]);
        ImPlot::SetupAxis(ImAxis_X1, dndx == NULL ? "[drop here]" : dndx->Label, flags);
        ImPlot::PushStyleColor(ImPlotCol_AxisBg, dndy != NULL ? dndy->Color : ImPlot::GetStyle().Colors[ImPlotCol_AxisBg]);
        ImPlot::SetupAxis(ImAxis_Y1, dndy == NULL ? "[drop here]" : dndy->Label, flags);
        ImPlot::PopStyleColor(2);
        if (dndx != NULL && dndy != NULL) {
            ImVec4 mixed((dndx->Color.x + dndy->Color.x)/2,(dndx->Color.y + dndy->Color.y)/2,(dndx->Color.z + dndy->Color.z)/2,(dndx->Color.w + dndy->Color.w)/2);
            ImPlot::SetNextLineStyle(mixed);
            ImPlot::PlotLine("##dndxy", &dndx->Data[0].y, &dndy->Data[0].y, dndx->Data.size(), 0, 0, 2 * sizeof(float));
        }
        // allow the x-axis to be a DND target
        if (ImPlot::BeginDragDropTargetAxis(ImAxis_X1)) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dndx = &dnd[i];
            }
            ImPlot::EndDragDropTarget();
        }
        // allow the x-axis to be a DND source
        if (dndx != NULL && ImPlot::BeginDragDropSourceAxis(ImAxis_X1)) {
            ImGui::SetDragDropPayload("MY_DND", &dndx->Idx, sizeof(int));
            ImPlot::ItemIcon(dndx->Color); ImGui::SameLine();
            ImGui::TextUnformatted(dndx->Label);
            ImPlot::EndDragDropSource();
        }
        // allow the y-axis to be a DND target
        if (ImPlot::BeginDragDropTargetAxis(ImAxis_Y1)) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dndy = &dnd[i];
            }
            ImPlot::EndDragDropTarget();
        }
        // allow the y-axis to be a DND source
        if (dndy != NULL && ImPlot::BeginDragDropSourceAxis(ImAxis_Y1)) {
            ImGui::SetDragDropPayload("MY_DND", &dndy->Idx, sizeof(int));
            ImPlot::ItemIcon(dndy->Color); ImGui::SameLine();
            ImGui::TextUnformatted(dndy->Label);
            ImPlot::EndDragDropSource();
        }
        // allow the plot area to be a DND target
        if (ImPlot::BeginDragDropTargetPlot()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND")) {
                int i = *(int*)payload->Data; dndx = dndy = &dnd[i];
            }
        }
        // allow the plot area to be a DND source
        if (ImPlot::BeginDragDropSourcePlot()) {
            ImGui::TextUnformatted("Yes, you can\ndrag this!");
            ImPlot::EndDragDropSource();
        }
        ImPlot::EndPlot();
    }
    ImGui::EndChild();
}

//-----------------------------------------------------------------------------

void Demo_Tables() {
#ifdef IMGUI_HAS_TABLE
    static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                                   ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;
    static bool anim = true;
    static int offset = 0;
    ImGui::BulletText("Plots can be used inside of ImGui tables as another means of creating subplots.");
    ImGui::Checkbox("Animate",&anim);
    if (anim)
        offset = (offset + 1) % 100;
    if (ImGui::BeginTable("##table", 3, flags, ImVec2(-1,0))) {
        ImGui::TableSetupColumn("Electrode", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Voltage", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("EMG Signal");
        ImGui::TableHeadersRow();
        ImPlot::PushColormap(ImPlotColormap_Cool);
        for (int row = 0; row < 10; row++) {
            ImGui::TableNextRow();
            static float data[100];
            srand(row);
            for (int i = 0; i < 100; ++i)
                data[i] = RandomRange(0.0f,10.0f);
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("EMG %d", row);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f V", data[offset]);
            ImGui::TableSetColumnIndex(2);
            ImGui::PushID(row);
            MyImPlot::Sparkline("##spark",data,100,0,11.0f,offset,ImPlot::GetColormapColor(row),ImVec2(-1, 35));
            ImGui::PopID();
        }
        ImPlot::PopColormap();
        ImGui::EndTable();
    }
#else
    ImGui::BulletText("You need to merge the ImGui 'tables' branch for this section.");
#endif
}

//-----------------------------------------------------------------------------

void Demo_OffsetAndStride() {
    static const int k_circles    = 11;
    static const int k_points_per = 50;
    static const int k_size       = 2 * k_points_per * k_circles;
    static double interleaved_data[k_size];
    for (int p = 0; p < k_points_per; ++p) {
        for (int c = 0; c < k_circles; ++c) {
            double r = (double)c / (k_circles - 1) * 0.2 + 0.2;
            interleaved_data[p*2*k_circles + 2*c + 0] = 0.5 + r * cos((double)p/k_points_per * 6.28);
            interleaved_data[p*2*k_circles + 2*c + 1] = 0.5 + r * sin((double)p/k_points_per * 6.28);
        }
    }
    static int offset = 0;
    ImGui::BulletText("Offsetting is useful for realtime plots (see above) and circular buffers.");
    ImGui::BulletText("Striding is useful for interleaved data (e.g. audio) or plotting structs.");
    ImGui::BulletText("Here, all circle data is stored in a single interleaved buffer:");
    ImGui::BulletText("[c0.x0 c0.y0 ... cn.x0 cn.y0 c0.x1 c0.y1 ... cn.x1 cn.y1 ... cn.xm cn.ym]");
    ImGui::BulletText("The offset value indicates which circle point index is considered the first.");
    ImGui::BulletText("Offsets can be negative and/or larger than the actual data count.");
    ImGui::SliderInt("Offset", &offset, -2*k_points_per, 2*k_points_per);
    if (ImPlot::BeginPlot("##strideoffset",ImVec2(-1,0),ImPlotFlags_Equal)) {
        ImPlot::PushColormap(ImPlotColormap_Jet);
        char buff[32];
        for (int c = 0; c < k_circles; ++c) {
            snprintf(buff, sizeof(buff), "Circle %d", c);
            ImPlot::PlotLine(buff, &interleaved_data[c*2 + 0], &interleaved_data[c*2 + 1], k_points_per, 0, offset, 2*k_circles*sizeof(double));
        }
        ImPlot::EndPlot();
        ImPlot::PopColormap();
    }
    // offset++; uncomment for animation!
}

//-----------------------------------------------------------------------------

void Demo_CustomDataAndGetters() {
    ImGui::BulletText("You can plot custom structs using the stride feature.");
    ImGui::BulletText("Most plotters can also be passed a function pointer for getting data.");
    ImGui::Indent();
        ImGui::BulletText("You can optionally pass user data to be given to your getter function.");
        ImGui::BulletText("C++ lambdas can be passed as function pointers as well!");
    ImGui::Unindent();

    MyImPlot::Vector2f vec2_data[2] = { MyImPlot::Vector2f(0,0), MyImPlot::Vector2f(1,1) };

    if (ImPlot::BeginPlot("##Custom Data")) {

        // custom structs using stride example:
        ImPlot::PlotLine("Vector2f", &vec2_data[0].x, &vec2_data[0].y, 2, 0, 0, sizeof(MyImPlot::Vector2f) /* or sizeof(float) * 2 */);

        // custom getter example 1:
        ImPlot::PlotLineG("Spiral", MyImPlot::Spiral, NULL, 1000);

        // custom getter example 2:
        static MyImPlot::WaveData data1(0.001, 0.2, 2, 0.75);
        static MyImPlot::WaveData data2(0.001, 0.2, 4, 0.25);
        ImPlot::PlotLineG("Waves", MyImPlot::SineWave, &data1, 1000);
        ImPlot::PlotLineG("Waves", MyImPlot::SawWave, &data2, 1000);
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
        ImPlot::PlotShadedG("Waves", MyImPlot::SineWave, &data1, MyImPlot::SawWave, &data2, 1000);
        ImPlot::PopStyleVar();

        // you can also pass C++ lambdas:
        // auto lamda = [](void* data, int idx) { ... return ImPlotPoint(x,y); };
        // ImPlot::PlotLine("My Lambda", lambda, data, 1000);

        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

int MetricFormatter(double value, char* buff, int size, void* data) {
    const char* unit = (const char*)data;
    static double v[]      = {1000000000,1000000,1000,1,0.001,0.000001,0.000000001};
    static const char* p[] = {"G","M","k","","m","u","n"};
    if (value == 0) {
        return snprintf(buff,size,"0 %s", unit);
    }
    for (int i = 0; i < 7; ++i) {
        if (fabs(value) >= v[i]) {
            return snprintf(buff,size,"%g %s%s",value/v[i],p[i],unit);
        }
    }
    return snprintf(buff,size,"%g %s%s",value/v[6],p[6],unit);
}

void Demo_TickLabels()  {
    static bool custom_fmt    = true;
    static bool custom_ticks  = false;
    static bool custom_labels = true;
    ImGui::Checkbox("Show Custom Format", &custom_fmt);
    ImGui::SameLine();
    ImGui::Checkbox("Show Custom Ticks", &custom_ticks);
    if (custom_ticks) {
        ImGui::SameLine();
        ImGui::Checkbox("Show Custom Labels", &custom_labels);
    }
    const double pi = 3.14;
    const char* pi_str[] = {"PI"};
    static double yticks[] = {100,300,700,900};
    static const char*  ylabels[] = {"One","Three","Seven","Nine"};
    static double yticks_aux[] = {0.2,0.4,0.6};
    static const char* ylabels_aux[] = {"A","B","C","D","E","F"};
    if (ImPlot::BeginPlot("##Ticks")) {
        ImPlot::SetupAxesLimits(2.5,5,0,1000);
        ImPlot::SetupAxis(ImAxis_Y2, NULL, ImPlotAxisFlags_AuxDefault);
        ImPlot::SetupAxis(ImAxis_Y3, NULL, ImPlotAxisFlags_AuxDefault);
        if (custom_fmt) {
            ImPlot::SetupAxisFormat(ImAxis_X1, "%g ms");
            ImPlot::SetupAxisFormat(ImAxis_Y1, MetricFormatter, (void*)"Hz");
            ImPlot::SetupAxisFormat(ImAxis_Y2, "%g dB");
            ImPlot::SetupAxisFormat(ImAxis_Y3, MetricFormatter, (void*)"m");
        }
        if (custom_ticks) {
            ImPlot::SetupAxisTicks(ImAxis_X1, &pi,1,custom_labels ? pi_str : NULL, true);
            ImPlot::SetupAxisTicks(ImAxis_Y1, yticks, 4, custom_labels ? ylabels : NULL, false);
            ImPlot::SetupAxisTicks(ImAxis_Y2, yticks_aux, 3, custom_labels ? ylabels_aux : NULL, false);
            ImPlot::SetupAxisTicks(ImAxis_Y3, 0, 1, 6, custom_labels ? ylabels_aux : NULL, false);
        }
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_CustomStyles() {
    ImPlot::PushColormap(ImPlotColormap_Deep);
    // normally you wouldn't change the entire style each frame
    ImPlotStyle backup = ImPlot::GetStyle();
    MyImPlot::StyleSeaborn();
    if (ImPlot::BeginPlot("seaborn style")) {
        ImPlot::SetupAxes( "x-axis", "y-axis");
        ImPlot::SetupAxesLimits(-0.5f, 9.5f, 0, 10);
        unsigned int lin[10] = {8,8,9,7,8,8,8,9,7,8};
        unsigned int bar[10] = {1,2,5,3,4,1,2,5,3,4};
        unsigned int dot[10] = {7,6,6,7,8,5,6,5,8,7};
        ImPlot::PlotBars("Bars", bar, 10, 0.5f);
        ImPlot::PlotLine("Line", lin, 10);
        ImPlot::NextColormapColor(); // skip green
        ImPlot::PlotScatter("Scatter", dot, 10);
        ImPlot::EndPlot();
    }
    ImPlot::GetStyle() = backup;
    ImPlot::PopColormap();
}

//-----------------------------------------------------------------------------

void Demo_CustomRendering() {
    if (ImPlot::BeginPlot("##CustomRend")) {
        ImVec2 cntr = ImPlot::PlotToPixels(ImPlotPoint(0.5f,  0.5f));
        ImVec2 rmin = ImPlot::PlotToPixels(ImPlotPoint(0.25f, 0.75f));
        ImVec2 rmax = ImPlot::PlotToPixels(ImPlotPoint(0.75f, 0.25f));
        ImPlot::PushPlotClipRect();
        ImPlot::GetPlotDrawList()->AddCircleFilled(cntr,20,IM_COL32(255,255,0,255),20);
        ImPlot::GetPlotDrawList()->AddRect(rmin, rmax, IM_COL32(128,0,255,255));
        ImPlot::PopPlotClipRect();
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_LegendPopups() {
    ImGui::BulletText("You can implement legend context menus to inject per-item controls and widgets.");
    ImGui::BulletText("Right click the legend label/icon to edit custom item attributes.");

    static float  frequency = 0.1f;
    static float  amplitude = 0.5f;
    static ImVec4 color     = ImVec4(1,1,0,1);
    static float  alpha     = 1.0f;
    static bool   line      = false;
    static float  thickness = 1;
    static bool   markers   = false;
    static bool   shaded    = false;

    static float vals[101];
    for (int i = 0; i < 101; ++i)
        vals[i] = amplitude * sinf(frequency * i);

    if (ImPlot::BeginPlot("Right Click the Legend")) {
        ImPlot::SetupAxesLimits(0,100,-1,1);
        // rendering logic
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, alpha);
        if (!line) {
            ImPlot::SetNextFillStyle(color);
            ImPlot::PlotBars("Right Click Me", vals, 101);
        }
        else {
            if (markers) ImPlot::SetNextMarkerStyle(ImPlotMarker_Square);
            ImPlot::SetNextLineStyle(color, thickness);
            ImPlot::PlotLine("Right Click Me", vals, 101);
            if (shaded) ImPlot::PlotShaded("Right Click Me",vals,101);
        }
        ImPlot::PopStyleVar();
        // custom legend context menu
        if (ImPlot::BeginLegendPopup("Right Click Me")) {
            ImGui::SliderFloat("Frequency",&frequency,0,1,"%0.2f");
            ImGui::SliderFloat("Amplitude",&amplitude,0,1,"%0.2f");
            ImGui::Separator();
            ImGui::ColorEdit3("Color",&color.x);
            ImGui::SliderFloat("Transparency",&alpha,0,1,"%.2f");
            ImGui::Checkbox("Line Plot", &line);
            if (line) {
                ImGui::SliderFloat("Thickness", &thickness, 0, 5);
                ImGui::Checkbox("Markers", &markers);
                ImGui::Checkbox("Shaded",&shaded);
            }
            ImPlot::EndLegendPopup();
        }
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Demo_ColormapWidgets() {
    static int cmap = ImPlotColormap_Viridis;

    if (ImPlot::ColormapButton("Button",ImVec2(0,0),cmap)) {
        cmap = (cmap + 1) % ImPlot::GetColormapCount();
    }

    static float t = 0.5f;
    static ImVec4 col;
    ImGui::ColorButton("##Display",col,ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine();
    ImPlot::ColormapSlider("Slider", &t, &col, "%.3f", cmap);

    ImPlot::ColormapIcon(cmap); ImGui::SameLine(); ImGui::Text("Icon");

    static ImPlotColormapScaleFlags flags = 0;
    static float scale[2] = {0, 100};
    ImPlot::ColormapScale("Scale",scale[0],scale[1],ImVec2(0,0),"%g dB",flags,cmap);
    ImGui::InputFloat2("Scale",scale);
    CHECKBOX_FLAG(flags, ImPlotColormapScaleFlags_NoLabel);
    CHECKBOX_FLAG(flags, ImPlotColormapScaleFlags_Opposite);
    CHECKBOX_FLAG(flags, ImPlotColormapScaleFlags_Invert);
}

//-----------------------------------------------------------------------------

void Demo_CustomPlottersAndTooltips()  {
    ImGui::BulletText("You can create custom plotters or extend ImPlot using implot_internal.h.");
    double dates[]  = {1546300800,1546387200,1546473600,1546560000,1546819200,1546905600,1546992000,1547078400,1547164800,1547424000,1547510400,1547596800,1547683200,1547769600,1547942400,1548028800,1548115200,1548201600,1548288000,1548374400,1548633600,1548720000,1548806400,1548892800,1548979200,1549238400,1549324800,1549411200,1549497600,1549584000,1549843200,1549929600,1550016000,1550102400,1550188800,1550361600,1550448000,1550534400,1550620800,1550707200,1550793600,1551052800,1551139200,1551225600,1551312000,1551398400,1551657600,1551744000,1551830400,1551916800,1552003200,1552262400,1552348800,1552435200,1552521600,1552608000,1552867200,1552953600,1553040000,1553126400,1553212800,1553472000,1553558400,1553644800,1553731200,1553817600,1554076800,1554163200,1554249600,1554336000,1554422400,1554681600,1554768000,1554854400,1554940800,1555027200,1555286400,1555372800,1555459200,1555545600,1555632000,1555891200,1555977600,1556064000,1556150400,1556236800,1556496000,1556582400,1556668800,1556755200,1556841600,1557100800,1557187200,1557273600,1557360000,1557446400,1557705600,1557792000,1557878400,1557964800,1558051200,1558310400,1558396800,1558483200,1558569600,1558656000,1558828800,1558915200,1559001600,1559088000,1559174400,1559260800,1559520000,1559606400,1559692800,1559779200,1559865600,1560124800,1560211200,1560297600,1560384000,1560470400,1560729600,1560816000,1560902400,1560988800,1561075200,1561334400,1561420800,1561507200,1561593600,1561680000,1561939200,1562025600,1562112000,1562198400,1562284800,1562544000,1562630400,1562716800,1562803200,1562889600,1563148800,1563235200,1563321600,1563408000,1563494400,1563753600,1563840000,1563926400,1564012800,1564099200,1564358400,1564444800,1564531200,1564617600,1564704000,1564963200,1565049600,1565136000,1565222400,1565308800,1565568000,1565654400,1565740800,1565827200,1565913600,1566172800,1566259200,1566345600,1566432000,1566518400,1566777600,1566864000,1566950400,1567036800,1567123200,1567296000,1567382400,1567468800,1567555200,1567641600,1567728000,1567987200,1568073600,1568160000,1568246400,1568332800,1568592000,1568678400,1568764800,1568851200,1568937600,1569196800,1569283200,1569369600,1569456000,1569542400,1569801600,1569888000,1569974400,1570060800,1570147200,1570406400,1570492800,1570579200,1570665600,1570752000,1571011200,1571097600,1571184000,1571270400,1571356800,1571616000,1571702400,1571788800,1571875200,1571961600};
    double opens[]  = {1284.7,1319.9,1318.7,1328,1317.6,1321.6,1314.3,1325,1319.3,1323.1,1324.7,1321.3,1323.5,1322,1281.3,1281.95,1311.1,1315,1314,1313.1,1331.9,1334.2,1341.3,1350.6,1349.8,1346.4,1343.4,1344.9,1335.6,1337.9,1342.5,1337,1338.6,1337,1340.4,1324.65,1324.35,1349.5,1371.3,1367.9,1351.3,1357.8,1356.1,1356,1347.6,1339.1,1320.6,1311.8,1314,1312.4,1312.3,1323.5,1319.1,1327.2,1332.1,1320.3,1323.1,1328,1330.9,1338,1333,1335.3,1345.2,1341.1,1332.5,1314,1314.4,1310.7,1314,1313.1,1315,1313.7,1320,1326.5,1329.2,1314.2,1312.3,1309.5,1297.4,1293.7,1277.9,1295.8,1295.2,1290.3,1294.2,1298,1306.4,1299.8,1302.3,1297,1289.6,1302,1300.7,1303.5,1300.5,1303.2,1306,1318.7,1315,1314.5,1304.1,1294.7,1293.7,1291.2,1290.2,1300.4,1284.2,1284.25,1301.8,1295.9,1296.2,1304.4,1323.1,1340.9,1341,1348,1351.4,1351.4,1343.5,1342.3,1349,1357.6,1357.1,1354.7,1361.4,1375.2,1403.5,1414.7,1433.2,1438,1423.6,1424.4,1418,1399.5,1435.5,1421.25,1434.1,1412.4,1409.8,1412.2,1433.4,1418.4,1429,1428.8,1420.6,1441,1460.4,1441.7,1438.4,1431,1439.3,1427.4,1431.9,1439.5,1443.7,1425.6,1457.5,1451.2,1481.1,1486.7,1512.1,1515.9,1509.2,1522.3,1513,1526.6,1533.9,1523,1506.3,1518.4,1512.4,1508.8,1545.4,1537.3,1551.8,1549.4,1536.9,1535.25,1537.95,1535.2,1556,1561.4,1525.6,1516.4,1507,1493.9,1504.9,1506.5,1513.1,1506.5,1509.7,1502,1506.8,1521.5,1529.8,1539.8,1510.9,1511.8,1501.7,1478,1485.4,1505.6,1511.6,1518.6,1498.7,1510.9,1510.8,1498.3,1492,1497.7,1484.8,1494.2,1495.6,1495.6,1487.5,1491.1,1495.1,1506.4};
    double highs[]  = {1284.75,1320.6,1327,1330.8,1326.8,1321.6,1326,1328,1325.8,1327.1,1326,1326,1323.5,1322.1,1282.7,1282.95,1315.8,1316.3,1314,1333.2,1334.7,1341.7,1353.2,1354.6,1352.2,1346.4,1345.7,1344.9,1340.7,1344.2,1342.7,1342.1,1345.2,1342,1350,1324.95,1330.75,1369.6,1374.3,1368.4,1359.8,1359,1357,1356,1353.4,1340.6,1322.3,1314.1,1316.1,1312.9,1325.7,1323.5,1326.3,1336,1332.1,1330.1,1330.4,1334.7,1341.1,1344.2,1338.8,1348.4,1345.6,1342.8,1334.7,1322.3,1319.3,1314.7,1316.6,1316.4,1315,1325.4,1328.3,1332.2,1329.2,1316.9,1312.3,1309.5,1299.6,1296.9,1277.9,1299.5,1296.2,1298.4,1302.5,1308.7,1306.4,1305.9,1307,1297.2,1301.7,1305,1305.3,1310.2,1307,1308,1319.8,1321.7,1318.7,1316.2,1305.9,1295.8,1293.8,1293.7,1304.2,1302,1285.15,1286.85,1304,1302,1305.2,1323,1344.1,1345.2,1360.1,1355.3,1363.8,1353,1344.7,1353.6,1358,1373.6,1358.2,1369.6,1377.6,1408.9,1425.5,1435.9,1453.7,1438,1426,1439.1,1418,1435,1452.6,1426.65,1437.5,1421.5,1414.1,1433.3,1441.3,1431.4,1433.9,1432.4,1440.8,1462.3,1467,1443.5,1444,1442.9,1447,1437.6,1440.8,1445.7,1447.8,1458.2,1461.9,1481.8,1486.8,1522.7,1521.3,1521.1,1531.5,1546.1,1534.9,1537.7,1538.6,1523.6,1518.8,1518.4,1514.6,1540.3,1565,1554.5,1556.6,1559.8,1541.9,1542.9,1540.05,1558.9,1566.2,1561.9,1536.2,1523.8,1509.1,1506.2,1532.2,1516.6,1519.7,1515,1519.5,1512.1,1524.5,1534.4,1543.3,1543.3,1542.8,1519.5,1507.2,1493.5,1511.4,1525.8,1522.2,1518.8,1515.3,1518,1522.3,1508,1501.5,1503,1495.5,1501.1,1497.9,1498.7,1492.1,1499.4,1506.9,1520.9};
    double lows[]   = {1282.85,1315,1318.7,1309.6,1317.6,1312.9,1312.4,1319.1,1319,1321,1318.1,1321.3,1319.9,1312,1280.5,1276.15,1308,1309.9,1308.5,1312.3,1329.3,1333.1,1340.2,1347,1345.9,1338,1340.8,1335,1332,1337.9,1333,1336.8,1333.2,1329.9,1340.4,1323.85,1324.05,1349,1366.3,1351.2,1349.1,1352.4,1350.7,1344.3,1338.9,1316.3,1308.4,1306.9,1309.6,1306.7,1312.3,1315.4,1319,1327.2,1317.2,1320,1323,1328,1323,1327.8,1331.7,1335.3,1336.6,1331.8,1311.4,1310,1309.5,1308,1310.6,1302.8,1306.6,1313.7,1320,1322.8,1311,1312.1,1303.6,1293.9,1293.5,1291,1277.9,1294.1,1286,1289.1,1293.5,1296.9,1298,1299.6,1292.9,1285.1,1288.5,1296.3,1297.2,1298.4,1298.6,1302,1300.3,1312,1310.8,1301.9,1292,1291.1,1286.3,1289.2,1289.9,1297.4,1283.65,1283.25,1292.9,1295.9,1290.8,1304.2,1322.7,1336.1,1341,1343.5,1345.8,1340.3,1335.1,1341.5,1347.6,1352.8,1348.2,1353.7,1356.5,1373.3,1398,1414.7,1427,1416.4,1412.7,1420.1,1396.4,1398.8,1426.6,1412.85,1400.7,1406,1399.8,1404.4,1415.5,1417.2,1421.9,1415,1413.7,1428.1,1434,1435.7,1427.5,1429.4,1423.9,1425.6,1427.5,1434.8,1422.3,1412.1,1442.5,1448.8,1468.2,1484.3,1501.6,1506.2,1498.6,1488.9,1504.5,1518.3,1513.9,1503.3,1503,1506.5,1502.1,1503,1534.8,1535.3,1541.4,1528.6,1525.6,1535.25,1528.15,1528,1542.6,1514.3,1510.7,1505.5,1492.1,1492.9,1496.8,1493.1,1503.4,1500.9,1490.7,1496.3,1505.3,1505.3,1517.9,1507.4,1507.1,1493.3,1470.5,1465,1480.5,1501.7,1501.4,1493.3,1492.1,1505.1,1495.7,1478,1487.1,1480.8,1480.6,1487,1488.3,1484.8,1484,1490.7,1490.4,1503.1};
    double closes[] = {1283.35,1315.3,1326.1,1317.4,1321.5,1317.4,1323.5,1319.2,1321.3,1323.3,1319.7,1325.1,1323.6,1313.8,1282.05,1279.05,1314.2,1315.2,1310.8,1329.1,1334.5,1340.2,1340.5,1350,1347.1,1344.3,1344.6,1339.7,1339.4,1343.7,1337,1338.9,1340.1,1338.7,1346.8,1324.25,1329.55,1369.6,1372.5,1352.4,1357.6,1354.2,1353.4,1346,1341,1323.8,1311.9,1309.1,1312.2,1310.7,1324.3,1315.7,1322.4,1333.8,1319.4,1327.1,1325.8,1330.9,1325.8,1331.6,1336.5,1346.7,1339.2,1334.7,1313.3,1316.5,1312.4,1313.4,1313.3,1312.2,1313.7,1319.9,1326.3,1331.9,1311.3,1313.4,1309.4,1295.2,1294.7,1294.1,1277.9,1295.8,1291.2,1297.4,1297.7,1306.8,1299.4,1303.6,1302.2,1289.9,1299.2,1301.8,1303.6,1299.5,1303.2,1305.3,1319.5,1313.6,1315.1,1303.5,1293,1294.6,1290.4,1291.4,1302.7,1301,1284.15,1284.95,1294.3,1297.9,1304.1,1322.6,1339.3,1340.1,1344.9,1354,1357.4,1340.7,1342.7,1348.2,1355.1,1355.9,1354.2,1362.1,1360.1,1408.3,1411.2,1429.5,1430.1,1426.8,1423.4,1425.1,1400.8,1419.8,1432.9,1423.55,1412.1,1412.2,1412.8,1424.9,1419.3,1424.8,1426.1,1423.6,1435.9,1440.8,1439.4,1439.7,1434.5,1436.5,1427.5,1432.2,1433.3,1441.8,1437.8,1432.4,1457.5,1476.5,1484.2,1519.6,1509.5,1508.5,1517.2,1514.1,1527.8,1531.2,1523.6,1511.6,1515.7,1515.7,1508.5,1537.6,1537.2,1551.8,1549.1,1536.9,1529.4,1538.05,1535.15,1555.9,1560.4,1525.5,1515.5,1511.1,1499.2,1503.2,1507.4,1499.5,1511.5,1513.4,1515.8,1506.2,1515.1,1531.5,1540.2,1512.3,1515.2,1506.4,1472.9,1489,1507.9,1513.8,1512.9,1504.4,1503.9,1512.8,1500.9,1488.7,1497.6,1483.5,1494,1498.3,1494.1,1488.1,1487.5,1495.7,1504.7,1505.3};
    static bool tooltip = true;
    ImGui::Checkbox("Show Tooltip", &tooltip);
    ImGui::SameLine();
    static ImVec4 bullCol = ImVec4(0.000f, 1.000f, 0.441f, 1.000f);
    static ImVec4 bearCol = ImVec4(0.853f, 0.050f, 0.310f, 1.000f);
    ImGui::SameLine(); ImGui::ColorEdit4("##Bull", &bullCol.x, ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine(); ImGui::ColorEdit4("##Bear", &bearCol.x, ImGuiColorEditFlags_NoInputs);
    ImPlot::GetStyle().UseLocalTime = false;

    if (ImPlot::BeginPlot("Candlestick Chart",ImVec2(-1,0))) {
        ImPlot::SetupAxes(NULL,NULL,0,ImPlotAxisFlags_AutoFit|ImPlotAxisFlags_RangeFit);
        ImPlot::SetupAxesLimits(1546300800, 1571961600, 1250, 1600);
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
        ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 1546300800, 1571961600);
        ImPlot::SetupAxisZoomConstraints(ImAxis_X1, 60*60*24*14, 1571961600-1546300800);
        ImPlot::SetupAxisFormat(ImAxis_Y1, "$%.0f");
        MyImPlot::PlotCandlestick("GOOGL",dates, opens, closes, lows, highs, 218, tooltip, 0.25f, bullCol, bearCol);
        ImPlot::EndPlot();
    }
    }

//-----------------------------------------------------------------------------
// DEMO WINDOW
//-----------------------------------------------------------------------------

void DemoHeader(const char* label, void(*demo)()) {
    if (ImGui::TreeNodeEx(label)) {
        demo();
        ImGui::TreePop();
    }
}

void ShowDemoWindow(bool* p_open) {
    static bool show_implot_metrics      = false;
    static bool show_implot_style_editor = false;
    static bool show_imgui_metrics       = false;
    static bool show_imgui_style_editor  = false;
    static bool show_imgui_demo          = false;

    if (show_implot_metrics) {
        ImPlot::ShowMetricsWindow(&show_implot_metrics);
    }
    if (show_implot_style_editor) {
        ImGui::SetNextWindowSize(ImVec2(415,762), ImGuiCond_Appearing);
        ImGui::Begin("Style Editor (ImPlot)", &show_implot_style_editor);
        ImPlot::ShowStyleEditor();
        ImGui::End();
    }
    if (show_imgui_style_editor) {
        ImGui::Begin("Style Editor (ImGui)", &show_imgui_style_editor);
        ImGui::ShowStyleEditor();
        ImGui::End();
    }
    if (show_imgui_metrics) {
        ImGui::ShowMetricsWindow(&show_imgui_metrics);
    }
    if (show_imgui_demo) {
        ImGui::ShowDemoWindow(&show_imgui_demo);
    }
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 750), ImGuiCond_FirstUseEver);
    ImGui::Begin("ImPlot Demo", p_open, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("Metrics",      NULL, &show_implot_metrics);
            ImGui::MenuItem("Style Editor", NULL, &show_implot_style_editor);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Metrics",       NULL, &show_imgui_metrics);
            ImGui::MenuItem("ImGui Style Editor",  NULL, &show_imgui_style_editor);
            ImGui::MenuItem("ImGui Demo",          NULL, &show_imgui_demo);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    //-------------------------------------------------------------------------
    ImGui::Text("ImPlot says hello. (%s)", IMPLOT_VERSION);
    // display warning about 16-bit indices
    static bool showWarning = sizeof(ImDrawIdx)*8 == 16 && (ImGui::GetIO().BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset) == false;
    if (showWarning) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,0,1));
        ImGui::TextWrapped("WARNING: ImDrawIdx is 16-bit and ImGuiBackendFlags_RendererHasVtxOffset is false. Expect visual glitches and artifacts! See README for more information.");
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();

    if (ImGui::BeginTabBar("ImPlotDemoTabs")) {
        if (ImGui::BeginTabItem("Plots")) {
            DemoHeader("Line Plots", Demo_LinePlots);
            DemoHeader("Filled Line Plots", Demo_FilledLinePlots);
            DemoHeader("Shaded Plots##", Demo_ShadedPlots);
            DemoHeader("Scatter Plots", Demo_ScatterPlots);
            DemoHeader("Realtime Plots", Demo_RealtimePlots);
            DemoHeader("Stairstep Plots", Demo_StairstepPlots);
            DemoHeader("Bar Plots", Demo_BarPlots);
            DemoHeader("Bar Groups", Demo_BarGroups);
            DemoHeader("Bar Stacks", Demo_BarStacks);
            DemoHeader("Error Bars", Demo_ErrorBars);
            DemoHeader("Stem Plots##", Demo_StemPlots);
            DemoHeader("Infinite Lines", Demo_InfiniteLines);
            DemoHeader("Pie Charts", Demo_PieCharts);
            DemoHeader("Heatmaps", Demo_Heatmaps);
            DemoHeader("Histogram", Demo_Histogram);
            DemoHeader("Histogram 2D", Demo_Histogram2D);
            DemoHeader("Digital Plots", Demo_DigitalPlots);
            DemoHeader("Images", Demo_Images);
            DemoHeader("Markers and Text", Demo_MarkersAndText);
            DemoHeader("NaN Values", Demo_NaNValues);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Subplots")) {
            DemoHeader("Sizing", Demo_SubplotsSizing);
            DemoHeader("Item Sharing", Demo_SubplotItemSharing);
            DemoHeader("Axis Linking", Demo_SubplotAxisLinking);
            DemoHeader("Tables", Demo_Tables);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Axes")) {
            DemoHeader("Log Scale", Demo_LogScale);
            DemoHeader("Symmetric Log Scale", Demo_SymmetricLogScale);
            DemoHeader("Time Scale", Demo_TimeScale);
            DemoHeader("Custom Scale", Demo_CustomScale);
            DemoHeader("Multiple Axes", Demo_MultipleAxes);
            DemoHeader("Tick Labels", Demo_TickLabels);
            DemoHeader("Linked Axes", Demo_LinkedAxes);
            DemoHeader("Axis Constraints", Demo_AxisConstraints);
            DemoHeader("Equal Axes", Demo_EqualAxes);
            DemoHeader("Auto-Fitting Data", Demo_AutoFittingData);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Tools")) {
            DemoHeader("Offset and Stride", Demo_OffsetAndStride);
            DemoHeader("Drag Points", Demo_DragPoints);
            DemoHeader("Drag Lines", Demo_DragLines);
            DemoHeader("Drag Rects", Demo_DragRects);
            DemoHeader("Querying", Demo_Querying);
            DemoHeader("Annotations", Demo_Annotations);
            DemoHeader("Tags", Demo_Tags);
            DemoHeader("Drag and Drop", Demo_DragAndDrop);
            DemoHeader("Legend Options", Demo_LegendOptions);
            DemoHeader("Legend Popups", Demo_LegendPopups);
            DemoHeader("Colormap Widgets", Demo_ColormapWidgets);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Custom")) {
            DemoHeader("Custom Styles", Demo_CustomStyles);
            DemoHeader("Custom Data and Getters", Demo_CustomDataAndGetters);
            DemoHeader("Custom Rendering", Demo_CustomRendering);
            DemoHeader("Custom Plotters and Tooltips", Demo_CustomPlottersAndTooltips);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Config")) {
            Demo_Config();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Help")) {
            Demo_Help();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

} // namespace ImPlot

namespace MyImPlot {

ImPlotPoint SineWave(int idx, void* data) {
    WaveData* wd = (WaveData*)data;
    double x = idx * wd->X;
    return ImPlotPoint(x, wd->Offset + wd->Amp * sin(2 * 3.14 * wd->Freq * x));
}

ImPlotPoint SawWave(int idx, void* data) {
    WaveData* wd = (WaveData*)data;
    double x = idx * wd->X;
    return ImPlotPoint(x, wd->Offset + wd->Amp * (-2 / 3.14 * atan(cos(3.14 * wd->Freq * x) / sin(3.14 * wd->Freq * x))));
}

ImPlotPoint Spiral(int idx, void*) {
    float r = 0.9f;            // outer radius
    float a = 0;               // inner radius
    float b = 0.05f;           // increment per rev
    float n = (r - a) / b;     // number  of revolutions
    double th = 2 * n * 3.14;  // angle
    float Th = float(th * idx / (1000 - 1));
    return ImPlotPoint(0.5f+(a + b*Th / (2.0f * (float) 3.14))*cos(Th),
                       0.5f + (a + b*Th / (2.0f * (float)3.14))*sin(Th));
}

void Sparkline(const char* id, const float* values, int count, float min_v, float max_v, int offset, const ImVec4& col, const ImVec2& size) {
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0,0));
    if (ImPlot::BeginPlot(id,size,ImPlotFlags_CanvasOnly|ImPlotFlags_NoChild)) {
        ImPlot::SetupAxes(0,0,ImPlotAxisFlags_NoDecorations,ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, count - 1, min_v, max_v, ImGuiCond_Always);
        ImPlot::SetNextLineStyle(col);
        ImPlot::SetNextFillStyle(col, 0.25);
        ImPlot::PlotLine(id, values, count, 1, 0, ImPlotLineFlags_Shaded, offset);
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleVar();
}

void StyleSeaborn() {

    ImPlotStyle& style              = ImPlot::GetStyle();

    ImVec4* colors                  = style.Colors;
    colors[ImPlotCol_Line]          = IMPLOT_AUTO_COL;
    colors[ImPlotCol_Fill]          = IMPLOT_AUTO_COL;
    colors[ImPlotCol_MarkerOutline] = IMPLOT_AUTO_COL;
    colors[ImPlotCol_MarkerFill]    = IMPLOT_AUTO_COL;
    colors[ImPlotCol_ErrorBar]      = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_FrameBg]       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImPlotCol_PlotBg]        = ImVec4(0.92f, 0.92f, 0.95f, 1.00f);
    colors[ImPlotCol_PlotBorder]    = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImPlotCol_LegendBg]      = ImVec4(0.92f, 0.92f, 0.95f, 1.00f);
    colors[ImPlotCol_LegendBorder]  = ImVec4(0.80f, 0.81f, 0.85f, 1.00f);
    colors[ImPlotCol_LegendText]    = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_TitleText]     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_InlayText]     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_AxisText]      = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImPlotCol_AxisGrid]      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImPlotCol_AxisBgHovered]   = ImVec4(0.92f, 0.92f, 0.95f, 1.00f);
    colors[ImPlotCol_AxisBgActive]    = ImVec4(0.92f, 0.92f, 0.95f, 0.75f);
    colors[ImPlotCol_Selection]     = ImVec4(1.00f, 0.65f, 0.00f, 1.00f);
    colors[ImPlotCol_Crosshairs]    = ImVec4(0.23f, 0.10f, 0.64f, 0.50f);

    style.LineWeight       = 1.5;
    style.Marker           = ImPlotMarker_None;
    style.MarkerSize       = 4;
    style.MarkerWeight     = 1;
    style.FillAlpha        = 1.0f;
    style.ErrorBarSize     = 5;
    style.ErrorBarWeight   = 1.5f;
    style.DigitalBitHeight = 8;
    style.DigitalBitGap    = 4;
    style.PlotBorderSize   = 0;
    style.MinorAlpha       = 1.0f;
    style.MajorTickLen     = ImVec2(0,0);
    style.MinorTickLen     = ImVec2(0,0);
    style.MajorTickSize    = ImVec2(0,0);
    style.MinorTickSize    = ImVec2(0,0);
    style.MajorGridSize    = ImVec2(1.2f,1.2f);
    style.MinorGridSize    = ImVec2(1.2f,1.2f);
    style.PlotPadding      = ImVec2(12,12);
    style.LabelPadding     = ImVec2(5,5);
    style.LegendPadding    = ImVec2(5,5);
    style.MousePosPadding  = ImVec2(5,5);
    style.PlotMinSize      = ImVec2(300,225);
}

} // namespaece MyImPlot

// WARNING:
//
// You can use "implot_internal.h" to build custom plotting fuctions or extend ImPlot.
// However, note that forward compatibility of this file is not guaranteed and the
// internal API is subject to change. At some point we hope to bring more of this
// into the public API and expose the necessary building blocks to fully support
// custom plotters. For now, proceed at your own risk!

#include "implot_internal.h"

namespace MyImPlot {

template <typename T>
int BinarySearch(const T* arr, int l, int r, T x) {
    if (r >= l) {
        int mid = l + (r - l) / 2;
        if (arr[mid] == x)
            return mid;
        if (arr[mid] > x)
            return BinarySearch(arr, l, mid - 1, x);
        return BinarySearch(arr, mid + 1, r, x);
    }
    return -1;
}

void PlotCandlestick(const char* label_id, const double* xs, const double* opens, const double* closes, const double* lows, const double* highs, int count, bool tooltip, float width_percent, ImVec4 bullCol, ImVec4 bearCol) {

    // get ImGui window DrawList
    ImDrawList* draw_list = ImPlot::GetPlotDrawList();
    // calc real value width
    double half_width = count > 1 ? (xs[1] - xs[0]) * width_percent : width_percent;

    // custom tool
    if (ImPlot::IsPlotHovered() && tooltip) {
        ImPlotPoint mouse   = ImPlot::GetPlotMousePos();
        mouse.x             = ImPlot::RoundTime(ImPlotTime::FromDouble(mouse.x), ImPlotTimeUnit_Day).ToDouble();
        float  tool_l       = ImPlot::PlotToPixels(mouse.x - half_width * 1.5, mouse.y).x;
        float  tool_r       = ImPlot::PlotToPixels(mouse.x + half_width * 1.5, mouse.y).x;
        float  tool_t       = ImPlot::GetPlotPos().y;
        float  tool_b       = tool_t + ImPlot::GetPlotSize().y;
        ImPlot::PushPlotClipRect();
        draw_list->AddRectFilled(ImVec2(tool_l, tool_t), ImVec2(tool_r, tool_b), IM_COL32(128,128,128,64));
        ImPlot::PopPlotClipRect();
        // find mouse location index
        int idx = BinarySearch(xs, 0, count - 1, mouse.x);
        // render tool tip (won't be affected by plot clip rect)
        if (idx != -1) {
            ImGui::BeginTooltip();
            char buff[32];
            ImPlot::FormatDate(ImPlotTime::FromDouble(xs[idx]),buff,32,ImPlotDateFmt_DayMoYr,ImPlot::GetStyle().UseISO8601);
            ImGui::Text("Day:   %s",  buff);
            ImGui::Text("Open:  $%.2f", opens[idx]);
            ImGui::Text("Close: $%.2f", closes[idx]);
            ImGui::Text("Low:   $%.2f", lows[idx]);
            ImGui::Text("High:  $%.2f", highs[idx]);
            ImGui::EndTooltip();
        }
    }

    // begin plot item
    if (ImPlot::BeginItem(label_id)) {
        // override legend icon color
        ImPlot::GetCurrentItem()->Color = IM_COL32(64,64,64,255);
        // fit data if requested
        if (ImPlot::FitThisFrame()) {
            for (int i = 0; i < count; ++i) {
                ImPlot::FitPoint(ImPlotPoint(xs[i], lows[i]));
                ImPlot::FitPoint(ImPlotPoint(xs[i], highs[i]));
            }
        }
        // render data
        for (int i = 0; i < count; ++i) {
            ImVec2 open_pos  = ImPlot::PlotToPixels(xs[i] - half_width, opens[i]);
            ImVec2 close_pos = ImPlot::PlotToPixels(xs[i] + half_width, closes[i]);
            ImVec2 low_pos   = ImPlot::PlotToPixels(xs[i], lows[i]);
            ImVec2 high_pos  = ImPlot::PlotToPixels(xs[i], highs[i]);
            ImU32 color      = ImGui::GetColorU32(opens[i] > closes[i] ? bearCol : bullCol);
            draw_list->AddLine(low_pos, high_pos, color);
            draw_list->AddRectFilled(open_pos, close_pos, color);
        }

        // end plot item
        ImPlot::EndItem();
    }
}

} // namespace MyImplot
