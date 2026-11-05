// Dear ImGui: standalone example application for FreeType variable fonts
// This example demonstrates variable font axis support (weight, width, slant, grade, italic).
// Requires: IMGUI_ENABLE_FREETYPE defined, FreeType library linked.

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_freetype.h"
#include <d3d11.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Known variable fonts to search for in Windows fonts directory
// Prioritize fonts with weight axis (wght) for better demo experience
static const char* g_KnownVariableFonts[] = {
    "SegoeUI-VF.ttf",           // Windows 11 (wght, wdth, slnt, ital)
    "bahnschrift.ttf",          // Windows 10+ (wght: 300-700) - best for testing!
    "CascadiaCode.ttf",         // Windows Terminal (wght: 200-700)
    "CascadiaMono.ttf",         // Windows Terminal mono (wght: 200-700)
    "Inter-Variable.ttf",       // Popular variable font (wght, wdth, slnt)
    "RobotoFlex-VariableFont.ttf", // Google (wght, wdth, slnt, GRAD, opsz)
    "SitkaVF.ttf",              // Windows 10+ (opsz only - no wght)
    nullptr
};

// Find first available variable font
static bool FindVariableFont(char* outPath, size_t outPathSize)
{
    char fontsDir[MAX_PATH];
    if (GetWindowsDirectoryA(fontsDir, MAX_PATH) == 0)
        strcpy(fontsDir, "C:\\Windows");
    strcat(fontsDir, "\\Fonts\\");

    for (int i = 0; g_KnownVariableFonts[i] != nullptr; i++)
    {
        char testPath[MAX_PATH];
        snprintf(testPath, sizeof(testPath), "%s%s", fontsDir, g_KnownVariableFonts[i]);

        FILE* f = fopen(testPath, "rb");
        if (f)
        {
            fclose(f);
            strncpy(outPath, testPath, outPathSize - 1);
            outPath[outPathSize - 1] = '\0';
            return true;
        }
    }
    return false;
}

// Maximum number of variable font axes we support
#define MAX_FONT_AXES 16

// Variable font state
struct VariableFontState
{
    char    FontPath[512] = "";
    float   SizePixels = 32.0f;
    char    SampleText[128] = "\xc3\x89\xc3\xa7\xc3\xbc\xc3\xb1 \xc3\x85gjpqy";  // "Éçüñ Ågjpqy" in UTF-8

    // Dynamic axis data (queried from font)
    ImFontVarAxisInfo   AxisInfo[MAX_FONT_AXES];    // Axis metadata from font
    float               AxisValues[MAX_FONT_AXES];  // Current axis values
    int                 AxisCount = 0;              // Number of axes in font

    // FreeType loader flags
    unsigned int FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoHinting | ImGuiFreeTypeLoaderFlags_Bitmap;

    ImFont* UiFont = nullptr;       // Fixed-size font for UI controls
    ImFont* PreviewFont = nullptr;  // Variable-size font for preview
    bool    NeedsRebuild = true;
    bool    NeedsAxisQuery = true;  // Query axes on next font load
    char    StatusMessage[256] = "";

    void ResetAxesToDefaults()
    {
        for (int i = 0; i < AxisCount; i++)
            AxisValues[i] = AxisInfo[i].Default;
    }
};

static VariableFontState g_FontState;

// Check if file exists
static bool FileExists(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (f) { fclose(f); return true; }
    return false;
}

// Rebuild font with current settings
static void RebuildFont(VariableFontState& state)
{
    ImGuiIO& io = ImGui::GetIO();

    // Check if font file exists
    bool fileExists = FileExists(state.FontPath);

    // Query axes if needed (new font loaded)
    if (state.NeedsAxisQuery)
    {
        if (fileExists)
        {
            state.AxisCount = ImGuiFreeType::GetFontAxes(state.FontPath, state.AxisInfo, MAX_FONT_AXES);
            if (state.AxisCount < 0)
                state.AxisCount = 0;
            state.ResetAxesToDefaults();
        }
        else
        {
            state.AxisCount = 0;
        }
        state.NeedsAxisQuery = false;
    }

    // Clear existing fonts
    io.Fonts->Clear();

    // Always add a default UI font first (ensures we have something valid)
    state.UiFont = io.Fonts->AddFontDefault();

    // Try to load the requested font if file exists
    if (fileExists)
    {
        // Build axis array from current values
        static ImFontConfigVarAxis s_previewAxes[MAX_FONT_AXES];
        for (int i = 0; i < state.AxisCount; i++)
            s_previewAxes[i] = ImFontConfigVarAxis(state.AxisInfo[i].Tag, state.AxisValues[i]);

        ImFontConfig previewConfig;
        previewConfig.FontLoaderFlags = state.FontLoaderFlags;
        previewConfig.VarAxes.Axes = s_previewAxes;
        previewConfig.VarAxes.AxesCount = state.AxisCount;

        state.PreviewFont = io.Fonts->AddFontFromFileTTF(state.FontPath, state.SizePixels, &previewConfig);

        if (state.PreviewFont)
        {
            const char* filename = strrchr(state.FontPath, '\\');
            if (!filename) filename = strrchr(state.FontPath, '/');
            filename = filename ? filename + 1 : state.FontPath;

            snprintf(state.StatusMessage, sizeof(state.StatusMessage),
                     "Loaded: %s @ %.0fpx (%d axes)", filename, state.SizePixels, state.AxisCount);
        }
        else
        {
            state.PreviewFont = state.UiFont;
            snprintf(state.StatusMessage, sizeof(state.StatusMessage),
                     "Invalid font file: %s", state.FontPath);
        }
    }
    else
    {
        state.PreviewFont = state.UiFont;
        snprintf(state.StatusMessage, sizeof(state.StatusMessage),
                 "File not found: %s", state.FontPath);
    }

    // Rebuild font atlas
    io.Fonts->Build();
    state.NeedsRebuild = false;
}

int main(int, char**)
{
    // Find a suitable variable font
    if (!FindVariableFont(g_FontState.FontPath, sizeof(g_FontState.FontPath)))
    {
        // Fallback if no variable font found
        strcpy(g_FontState.FontPath, "C:\\Windows\\Fonts\\segoeui.ttf");
    }

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui FreeType Variable Font Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui FreeType Variable Font Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Rebuild font if needed (must be done before NewFrame)
        if (g_FontState.NeedsRebuild)
        {
            RebuildFont(g_FontState);
            ImGui_ImplDX11_InvalidateDeviceObjects();
        }

        // Start frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Get display size for window positioning
        ImVec2 displaySize = io.DisplaySize;
        float padding = 10.0f;
        float halfWidth = (displaySize.x - padding * 3) / 2;
        float windowHeight = displaySize.y - padding * 2;

        // Variable Font Control Panel (left half)
        ImGui::SetNextWindowPos(ImVec2(padding, padding), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(halfWidth, windowHeight), ImGuiCond_Always);
        ImGui::Begin("Variable Font Controls", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        {
            ImGui::Text("FreeType Variable Font Demo");
            ImGui::Separator();

            // Font path
            ImGui::Text("Font Path:");
            ImGui::SetNextItemWidth(-60);
            if (ImGui::InputText("##FontPath", g_FontState.FontPath, sizeof(g_FontState.FontPath), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                g_FontState.NeedsAxisQuery = true;
                g_FontState.NeedsRebuild = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Load"))
            {
                g_FontState.NeedsAxisQuery = true;
                g_FontState.NeedsRebuild = true;
            }

            ImGui::Separator();

            // Size (always available)
            ImGui::SetNextItemWidth(200);
            if (ImGui::SliderFloat("Size (px)", &g_FontState.SizePixels, 8.0f, 72.0f, "%.0f"))
                g_FontState.NeedsRebuild = true;

            // Sample text for metrics visualization (under Size)
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Sample:"); ImGui::SameLine();
            ImGui::SetNextItemWidth(140);
            ImGui::InputText("##SampleText", g_FontState.SampleText, sizeof(g_FontState.SampleText));

            // FreeType loader flags (two-column layout)
            ImGui::Separator();
            ImGui::Text("FreeType Loader Flags:");
            {
                bool edited = false;
                if (ImGui::BeginTable("##FontLoaderFlags", 2))
                {
                    ImGui::TableNextColumn(); edited |= ImGui::CheckboxFlags("NoHinting",     &g_FontState.FontLoaderFlags, ImGuiFreeTypeLoaderFlags_NoHinting);
                    ImGui::TableNextColumn(); edited |= ImGui::CheckboxFlags("Bold",          &g_FontState.FontLoaderFlags, ImGuiFreeTypeLoaderFlags_Bold);
                    ImGui::TableNextColumn(); edited |= ImGui::CheckboxFlags("NoAutoHint",    &g_FontState.FontLoaderFlags, ImGuiFreeTypeLoaderFlags_NoAutoHint);
                    ImGui::TableNextColumn(); edited |= ImGui::CheckboxFlags("Oblique",       &g_FontState.FontLoaderFlags, ImGuiFreeTypeLoaderFlags_Oblique);
                    ImGui::TableNextColumn(); edited |= ImGui::CheckboxFlags("ForceAutoHint", &g_FontState.FontLoaderFlags, ImGuiFreeTypeLoaderFlags_ForceAutoHint);
                    ImGui::TableNextColumn(); edited |= ImGui::CheckboxFlags("Monochrome",    &g_FontState.FontLoaderFlags, ImGuiFreeTypeLoaderFlags_Monochrome);
                    ImGui::TableNextColumn(); edited |= ImGui::CheckboxFlags("LightHinting",  &g_FontState.FontLoaderFlags, ImGuiFreeTypeLoaderFlags_LightHinting);
                    ImGui::TableNextColumn(); edited |= ImGui::CheckboxFlags("LoadColor",     &g_FontState.FontLoaderFlags, ImGuiFreeTypeLoaderFlags_LoadColor);
                    ImGui::TableNextColumn(); edited |= ImGui::CheckboxFlags("MonoHinting",   &g_FontState.FontLoaderFlags, ImGuiFreeTypeLoaderFlags_MonoHinting);
                    ImGui::TableNextColumn(); edited |= ImGui::CheckboxFlags("Bitmap",        &g_FontState.FontLoaderFlags, ImGuiFreeTypeLoaderFlags_Bitmap);
                    ImGui::EndTable();
                }
                if (edited)
                    g_FontState.NeedsRebuild = true;
            }

            // Dynamic axis sliders
            if (g_FontState.AxisCount > 0)
            {
                ImGui::Separator();
                ImGui::Text("Variable Font Axes (%d):", g_FontState.AxisCount);

                if (ImGui::BeginTable("##AxisSliders", 3, ImGuiTableFlags_SizingFixedFit))
                {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                    ImGui::TableSetupColumn("Slider", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, 25.0f);

                    for (int i = 0; i < g_FontState.AxisCount; i++)
                    {
                        const ImFontVarAxisInfo& axis = g_FontState.AxisInfo[i];

                        // Format axis tag as 4 characters for display
                        char tagStr[8];
                        tagStr[0] = (char)((axis.Tag >> 24) & 0xFF);
                        tagStr[1] = (char)((axis.Tag >> 16) & 0xFF);
                        tagStr[2] = (char)((axis.Tag >> 8) & 0xFF);
                        tagStr[3] = (char)(axis.Tag & 0xFF);
                        tagStr[4] = '\0';

                        ImGui::TableNextRow();

                        // Column 1: Name (tag)
                        ImGui::TableNextColumn();
                        ImGui::Text("%s (%s)", axis.Name[0] ? axis.Name : "???", tagStr);

                        // Column 2: Slider
                        ImGui::TableNextColumn();
                        char sliderId[32];
                        snprintf(sliderId, sizeof(sliderId), "##axis%d", i);
                        ImGui::SetNextItemWidth(-FLT_MIN);  // Fill column width
                        if (ImGui::SliderFloat(sliderId, &g_FontState.AxisValues[i], axis.Minimum, axis.Maximum, "%.0f"))
                            g_FontState.NeedsRebuild = true;

                        // Column 3: Reset button
                        ImGui::TableNextColumn();
                        char resetLabel[32];
                        snprintf(resetLabel, sizeof(resetLabel), "R##%d", i);
                        if (ImGui::SmallButton(resetLabel))
                        {
                            g_FontState.AxisValues[i] = axis.Default;
                            g_FontState.NeedsRebuild = true;
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Reset to default: %.0f", axis.Default);
                    }
                    ImGui::EndTable();
                }
            }
            else
            {
                ImGui::Separator();
                ImGui::TextDisabled("No variable font axes found.");
                ImGui::TextDisabled("This font may not be a variable font.");
            }

            ImGui::Separator();

            // Reset button
            if (ImGui::Button("Reset to Defaults", ImVec2(200, 0)))
            {
                g_FontState.SizePixels = 32.0f;
                g_FontState.ResetAxesToDefaults();
                g_FontState.NeedsRebuild = true;
            }

            // Status
            ImGui::Separator();
            ImGui::TextWrapped("Status: %s", g_FontState.StatusMessage);
        }
        ImGui::End();

        // Font Preview Window (right half)
        ImGui::SetNextWindowPos(ImVec2(padding * 2 + halfWidth, padding), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(halfWidth, windowHeight), ImGuiCond_Always);
        ImGui::Begin("Font Preview", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        {
            // Show current axis values compactly (with wrapping)
            ImGui::TextDisabled("%.0fpx", g_FontState.SizePixels);
            for (int i = 0; i < g_FontState.AxisCount; i++)
            {
                char tagStr[8];
                tagStr[0] = (char)((g_FontState.AxisInfo[i].Tag >> 24) & 0xFF);
                tagStr[1] = (char)((g_FontState.AxisInfo[i].Tag >> 16) & 0xFF);
                tagStr[2] = (char)((g_FontState.AxisInfo[i].Tag >> 8) & 0xFF);
                tagStr[3] = (char)(g_FontState.AxisInfo[i].Tag & 0xFF);
                tagStr[4] = '\0';

                // Calculate text width to check if it fits on current line
                char axisText[32];
                snprintf(axisText, sizeof(axisText), "%s=%.0f", tagStr, g_FontState.AxisValues[i]);
                float textWidth = ImGui::CalcTextSize(axisText).x;

                // Try to put on same line, wrap if doesn't fit
                ImGui::SameLine();
                if (ImGui::GetContentRegionAvail().x < textWidth)
                    ImGui::NewLine();

                ImGui::TextDisabled("%s", axisText);
            }

            ImGui::Separator();

            // Visual metrics diagram
            if (g_FontState.PreviewFont)
            {
                ImFont* font = g_FontState.PreviewFont;
                ImFontBaked* baked = font->GetFontBaked(g_FontState.SizePixels);
                if (!baked)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Font not baked at this size");
                }
                else
                {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    ImVec2 canvasPos = ImGui::GetCursorScreenPos();

                    // Get sample text and first character for metrics
                    const char* sampleText = g_FontState.SampleText;
                    if (sampleText[0] == '\0') sampleText = "A";  // Fallback

                    // Decode first UTF-8 character (simple inline decoder)
                    ImWchar firstChar = 0;
                    unsigned char c = (unsigned char)sampleText[0];
                    if (c < 0x80) {
                        firstChar = c;
                    } else if ((c & 0xE0) == 0xC0) {
                        firstChar = ((c & 0x1F) << 6) | (sampleText[1] & 0x3F);
                    } else if ((c & 0xF0) == 0xE0) {
                        firstChar = ((c & 0x0F) << 12) | ((sampleText[1] & 0x3F) << 6) | (sampleText[2] & 0x3F);
                    } else {
                        firstChar = '?';  // Fallback for 4-byte sequences or invalid
                    }

                    // Font metrics
                    // In ImGui/FreeType: Ascent is positive, Descent is negative
                    float fontSize = baked->Size;           // Em height (requested size)
                    float ascent = baked->Ascent;           // Positive distance above baseline
                    float descent = baked->Descent;         // Negative distance below baseline
                    float cellHeight = ascent - descent;    // Total cell height (ascent + |descent|)
                    float lineHeight = fontSize;            // Em height = line height in ImGui
                    float internalLeading = cellHeight - lineHeight;  // Positive = cell extends beyond em square
                    float leadingTop = internalLeading / 2.0f;
                    float leadingBottom = internalLeading - leadingTop;
                    float baselineFromTop = leadingTop + ascent;  // Where baseline sits from top of line

                    // Layout - left margin for vertical measurements
                    float leftMargin = 50.0f;  // Space for 3 vertical measurements on left
                    float topPadding = 20.0f;
                    float bottomPadding = 44.0f;  // Space for horizontal metrics (24px) + 20px gap to legend

                    // Visual height is the max of cell height and line height
                    float visualHeight = (cellHeight > lineHeight) ? cellHeight : lineHeight;
                    float diagramHeight = visualHeight + topPadding + bottomPadding;
                    float diagramWidth = ImGui::GetContentRegionAvail().x;

                    ImGui::Dummy(ImVec2(diagramWidth, diagramHeight));

                    // Y positions - anchor from visual top
                    // When internalLeading <= 0: line bounds contain text bounds (yLineTop is visual top)
                    // When internalLeading > 0: text bounds extend beyond line bounds (yAscent is visual top)
                    float yVisualTop = canvasPos.y + topPadding;
                    float yLineTop, yAscent, yBaseline, yDescent, yLineBottom, yVisualBottom;
                    if (internalLeading <= 0) {
                        // Normal case: text fits within line height
                        yLineTop = yVisualTop;
                        yAscent = yLineTop - leadingTop;  // leadingTop is negative, so ascent is below line top
                        yBaseline = yAscent + ascent;
                        yDescent = yBaseline - descent;
                        yLineBottom = yDescent - leadingBottom;  // leadingBottom is negative
                        yVisualBottom = yLineBottom;
                    } else {
                        // Text extends beyond em square - anchor from ascent
                        yAscent = yVisualTop;
                        yLineTop = yAscent + leadingTop;  // leadingTop is positive, so line top is below ascent
                        yBaseline = yAscent + ascent;
                        yDescent = yBaseline - descent;
                        yLineBottom = yDescent - leadingBottom;  // leadingBottom is positive, so line bottom is above descent
                        yVisualBottom = yDescent;
                    }

                    // X position - text starts after left margin for measurements
                    float xTextStart = canvasPos.x + leftMargin;

                    // Colors
                    ImU32 colBlue = IM_COL32(66, 135, 245, 255);      // Ascent
                    ImU32 colGreen = IM_COL32(87, 200, 87, 255);      // Baseline
                    ImU32 colOrange = IM_COL32(235, 160, 60, 255);    // Descent
                    ImU32 colPurple = IM_COL32(180, 100, 220, 255);   // Text width
                    ImU32 colCyan = IM_COL32(80, 200, 220, 255);      // Character width
                    ImU32 colRed = IM_COL32(220, 80, 80, 255);        // Line height
                    ImU32 colText = IM_COL32(255, 255, 255, 255);

                    // Helper lambda to draw dashed horizontal line
                    auto drawDashedLineH = [&](float x1, float x2, float y, ImU32 col, float thickness = 1.0f) {
                        float dashLen = 6.0f, gapLen = 4.0f;
                        float x = x1;
                        while (x < x2) {
                            float endX = (x + dashLen < x2) ? x + dashLen : x2;
                            drawList->AddLine(ImVec2(x, y), ImVec2(endX, y), col, thickness);
                            x += dashLen + gapLen;
                        }
                    };

                    // Helper lambda to draw dashed vertical line
                    auto drawDashedLineV = [&](float x, float y1, float y2, ImU32 col, float thickness = 1.0f) {
                        float dashLen = 6.0f, gapLen = 4.0f;
                        float y = y1;
                        while (y < y2) {
                            float endY = (y + dashLen < y2) ? y + dashLen : y2;
                            drawList->AddLine(ImVec2(x, y), ImVec2(x, endY), col, thickness);
                            y += dashLen + gapLen;
                        }
                    };

                    // Calculate text width first (needed for line bounds)
                    ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, sampleText);
                    float textEndX = xTextStart + textSize.x;

                    // Get first character glyph metrics
                    float firstCharAdvance = 0.0f;
                    if (ImFontGlyph* glyph = baked->FindGlyph(firstChar))
                        firstCharAdvance = glyph->AdvanceX;

                    // Draw background - only within text width
                    drawList->AddRectFilled(ImVec2(xTextStart, yAscent), ImVec2(textEndX, yDescent), IM_COL32(35, 35, 35, 255));
                    // Leading areas (top and bottom)
                    drawList->AddRectFilled(ImVec2(xTextStart, yLineTop), ImVec2(textEndX, yAscent), IM_COL32(50, 35, 35, 255));
                    drawList->AddRectFilled(ImVec2(xTextStart, yDescent), ImVec2(textEndX, yLineBottom), IM_COL32(50, 35, 35, 255));

                    // Draw horizontal metric lines (dashed, 2px) - only within text width
                    drawDashedLineH(xTextStart, textEndX, yAscent, colBlue, 2.0f);      // Ascent
                    drawDashedLineH(xTextStart, textEndX, yBaseline, colGreen, 2.0f);   // Baseline
                    drawDashedLineH(xTextStart, textEndX, yDescent, colOrange, 2.0f);   // Descent

                    // Draw sample text
                    font->RenderText(drawList, fontSize, ImVec2(xTextStart, yAscent), colText,
                                     ImVec4(0, 0, 10000, 10000), sampleText, nullptr);

                    // Draw leading/line height lines LAST (1px, on top of others)
                    drawDashedLineH(xTextStart, textEndX, yLineTop, colRed, 1.0f);      // Line height top
                    drawDashedLineH(xTextStart, textEndX, yLineBottom, colRed, 1.0f);   // Line height bottom

                    // Vertical measurements to the LEFT of text (2px uniform thickness)
                    float xMeasure1 = canvasPos.x + 10.0f;   // Line height
                    float xMeasure2 = canvasPos.x + 22.0f;   // Cell height
                    float xMeasure3 = canvasPos.x + 34.0f;   // Ascent/Descent

                    ImU32 colYellow = IM_COL32(220, 200, 80, 255);  // Cell height color

                    // Line Height measurement (line top to line bottom)
                    drawList->AddLine(ImVec2(xMeasure1, yLineTop), ImVec2(xMeasure1, yLineBottom), colRed, 2.0f);
                    drawList->AddLine(ImVec2(xMeasure1 - 3, yLineTop), ImVec2(xMeasure1 + 3, yLineTop), colRed, 2.0f);
                    drawList->AddLine(ImVec2(xMeasure1 - 3, yLineBottom), ImVec2(xMeasure1 + 3, yLineBottom), colRed, 2.0f);

                    // Cell Height measurement (ascent to descent)
                    drawList->AddLine(ImVec2(xMeasure2, yAscent), ImVec2(xMeasure2, yDescent), colYellow, 2.0f);
                    drawList->AddLine(ImVec2(xMeasure2 - 3, yAscent), ImVec2(xMeasure2 + 3, yAscent), colYellow, 2.0f);
                    drawList->AddLine(ImVec2(xMeasure2 - 3, yDescent), ImVec2(xMeasure2 + 3, yDescent), colYellow, 2.0f);

                    // Ascent measurement (ascent line to baseline)
                    drawList->AddLine(ImVec2(xMeasure3, yAscent), ImVec2(xMeasure3, yBaseline), colBlue, 2.0f);
                    drawList->AddLine(ImVec2(xMeasure3 - 3, yAscent), ImVec2(xMeasure3 + 3, yAscent), colBlue, 2.0f);
                    drawList->AddLine(ImVec2(xMeasure3 - 3, yBaseline), ImVec2(xMeasure3 + 3, yBaseline), colBlue, 2.0f);

                    // Descent measurement (baseline to descent line)
                    drawList->AddLine(ImVec2(xMeasure3, yBaseline), ImVec2(xMeasure3, yDescent), colOrange, 2.0f);
                    drawList->AddLine(ImVec2(xMeasure3 - 3, yDescent), ImVec2(xMeasure3 + 3, yDescent), colOrange, 2.0f);

                    // Horizontal measurements below the text (use visual bottom to handle negative leading)
                    float yWidthLine = yVisualBottom + 12.0f;
                    float yCharLine = yWidthLine + 12.0f;

                    // Text width line (purple, 2px)
                    drawList->AddLine(ImVec2(xTextStart, yWidthLine), ImVec2(textEndX, yWidthLine), colPurple, 2.0f);
                    drawList->AddLine(ImVec2(xTextStart, yWidthLine - 4), ImVec2(xTextStart, yWidthLine + 4), colPurple, 2.0f);
                    drawList->AddLine(ImVec2(textEndX, yWidthLine - 4), ImVec2(textEndX, yWidthLine + 4), colPurple, 2.0f);

                    // First character AdvanceX line (cyan, 2px)
                    float charEndX = xTextStart + firstCharAdvance;
                    drawList->AddLine(ImVec2(xTextStart, yCharLine), ImVec2(charEndX, yCharLine), colCyan, 2.0f);
                    drawList->AddLine(ImVec2(xTextStart, yCharLine - 4), ImVec2(xTextStart, yCharLine + 4), colCyan, 2.0f);
                    drawList->AddLine(ImVec2(charEndX, yCharLine - 4), ImVec2(charEndX, yCharLine + 4), colCyan, 2.0f);

                    // Legend with values (below the diagram) - 3 equal columns using table

                    if (ImGui::BeginTable("##MetricsLegend", 3, ImGuiTableFlags_SizingStretchSame))
                    {
                        // Row 1: Line Height | Ascent | Width
                        ImGui::TableNextColumn();
                        ImGui::TextColored(ImVec4(0.86f, 0.31f, 0.31f, 1.0f), "---"); ImGui::SameLine();
                        ImGui::Text("Line Height %.0fpx", lineHeight);
                        ImGui::TableNextColumn();
                        ImGui::TextColored(ImVec4(0.26f, 0.53f, 0.96f, 1.0f), "---"); ImGui::SameLine();
                        ImGui::Text("Ascent %.0fpx", ascent);
                        ImGui::TableNextColumn();
                        ImGui::TextColored(ImVec4(0.71f, 0.39f, 0.86f, 1.0f), "---"); ImGui::SameLine();
                        ImGui::Text("Width %.0fpx", textSize.x);

                        // Row 2: Leading | Baseline | Advance
                        ImGui::TableNextColumn();
                        ImGui::TextColored(ImVec4(0.86f, 0.31f, 0.31f, 1.0f), "---"); ImGui::SameLine();
                        ImGui::Text("Leading %.1fpx", internalLeading);
                        ImGui::TableNextColumn();
                        ImGui::TextColored(ImVec4(0.34f, 0.78f, 0.34f, 1.0f), "---"); ImGui::SameLine();
                        ImGui::Text("Baseline %.0fpx", baselineFromTop);
                        ImGui::TableNextColumn();
                        ImGui::TextColored(ImVec4(0.31f, 0.78f, 0.86f, 1.0f), "---"); ImGui::SameLine();
                        ImGui::Text("Advance %.0fpx", firstCharAdvance);

                        // Row 3: Cell Height | Descent | (empty)
                        ImGui::TableNextColumn();
                        ImGui::TextColored(ImVec4(0.86f, 0.78f, 0.31f, 1.0f), "---"); ImGui::SameLine();
                        ImGui::Text("Cell Height %.0fpx", cellHeight);
                        ImGui::TableNextColumn();
                        ImGui::TextColored(ImVec4(0.92f, 0.63f, 0.24f, 1.0f), "---"); ImGui::SameLine();
                        ImGui::Text("Descent %.0fpx", -descent);
                        ImGui::TableNextColumn();
                        // Empty cell

                        ImGui::EndTable();
                    }
                } // end else (baked)
            }

            ImGui::Separator();

            // Push preview font for sample text
            if (g_FontState.PreviewFont)
                ImGui::PushFont(g_FontState.PreviewFont);

            ImGui::TextWrapped("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
            ImGui::TextWrapped("abcdefghijklmnopqrstuvwxyz");
            ImGui::TextWrapped("0123456789");

            ImGui::Separator();
            ImGui::TextWrapped("The quick brown fox jumps over the lazy dog.");

            if (g_FontState.PreviewFont)
                ImGui::PopFont();
        }
        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
