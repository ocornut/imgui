// dear imgui: helpers for the differential tests (see imgui_add_test(... DIFF ...) in CMakeLists.txt).
// Each test is built twice, with 'import imgui;' and with '#include "imgui.h"' (IMGUI_TEST_HEADER), runs the same scripted frames,
// and prints a hash of everything rendered (draw data, texture uploads): the outputs of both builds must match.
// Included after the imports/includes: uses no other header.

#pragma once

static const int TestDiff_FrameCount = 120;

// FNV-1a hash
struct TestHash
{
    unsigned long long  Value = 14695981039346656037ull;
    int                 VtxCount = 0;
    void Add(const void* data, size_t size) { const unsigned char* p = (const unsigned char*)data; for (size_t n = 0; n < size; n++) Value = (Value ^ p[n]) * 1099511628211ull; }
    template<typename T> void Add(const T& v) { Add(&v, sizeof(v)); }
    void Print(const char* name) const { printf("%s: frames=%d vertices=%d hash=%08x%08x\n", name, TestDiff_FrameCount, VtxCount, (unsigned int)(Value >> 32), (unsigned int)Value); }
};

static void TestDiff_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::GetPlatformIO().Platform_OpenInShellFn = [](ImGuiContext*, const char*) { return false; };   // scripted clicks may hit links
}

// Call after the backends' NewFrame(): scripted input (mouse moves and clicks, keyboard navigation and text), fixed time step
static void TestDiff_NewFrame(int frame)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;
    io.AddMousePosEvent((float)(frame * 37 % 1000), (float)(frame * 23 % 700));
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, frame % 10 == 5);
    io.AddKeyEvent(ImGuiKey_DownArrow, frame % 4 == 1);
    io.AddKeyEvent(ImGuiKey_Space, frame % 12 == 7);
    if (frame % 8 == 3)
        io.AddInputCharacter('a' + frame % 26);
}

static void TestDiff_ShowWindows()
{
    ImGui::ShowDemoWindow();
    ImGui::ShowDebugLogWindow();
    ImGui::ShowIDStackToolWindow();
    ImGui::Begin("Style Editor");
    ImGui::ShowStyleEditor();
    ImGui::End();
    // Not compared: ShowMetricsWindow() displays pointers, which differ between runs, and ShowAboutWindow() displays build
    // information, which differs between builds (e.g. like imgui_single_file.h, imgui.cppm compiles imgui_demo.cpp after
    // imgui_internal.h, which defines IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS when IMGUI_DISABLE_FILE_FUNCTIONS is defined).
}

// Call after ImGui::Render(), before the renderer backend processes the texture requests
static void TestDiff_HashDrawData(TestHash& hash, const ImDrawData* draw_data)
{
    hash.Add(draw_data->DisplayPos);
    hash.Add(draw_data->DisplaySize);
    hash.VtxCount += draw_data->TotalVtxCount;
    for (const ImDrawList* draw_list : draw_data->CmdLists)
    {
        hash.Add(draw_list->VtxBuffer.Data, (size_t)draw_list->VtxBuffer.size_in_bytes());
        hash.Add(draw_list->IdxBuffer.Data, (size_t)draw_list->IdxBuffer.size_in_bytes());
        for (const ImDrawCmd& cmd : draw_list->CmdBuffer)
        {
            hash.Add(cmd.ClipRect);
            hash.Add(cmd.VtxOffset);
            hash.Add(cmd.IdxOffset);
            hash.Add(cmd.ElemCount);
            hash.Add(cmd.TexRef._TexData ? cmd.TexRef._TexData->UniqueID : -1);  // (not the texture pointer/identifier)
        }
    }
    if (draw_data->Textures != nullptr)
        for (const ImTextureData* tex : *draw_data->Textures)
        {
            hash.Add(tex->UniqueID);
            hash.Add(tex->Status);
            if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates)
            {
                hash.Add(tex->Format);
                hash.Add(tex->Width);
                hash.Add(tex->Height);
                hash.Add(tex->Pixels, (size_t)tex->GetSizeInBytes());
            }
        }
}
