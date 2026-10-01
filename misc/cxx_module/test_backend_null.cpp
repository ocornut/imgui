// dear imgui: run test for the C++20 backend module 'imgui.impl.null' (see backends/imgui_impl_null.cppm).
// Differential test (see test_diff.h): the header build (IMGUI_TEST_HEADER) and the module build must print the same output.

#ifdef IMGUI_TEST_HEADER
#include "imgui.h"
#include "imgui_impl_null.h"
#include <stdio.h>
#else
import std.compat;
import imgui;
import imgui.impl.null;
#endif
#include "test_diff.h"

int main()
{
    ImGui::CreateContext();
    TestDiff_Init();
    if (!ImGui_ImplNull_Init())
        return 1;

    TestHash hash;
    for (int frame = 0; frame < TestDiff_FrameCount; frame++)
    {
        ImGui_ImplNull_NewFrame();
        TestDiff_NewFrame(frame);
        ImGui::NewFrame();
        TestDiff_ShowWindows();
        ImGui::Render();
        TestDiff_HashDrawData(hash, ImGui::GetDrawData());
        ImGui_ImplNullRender_RenderDrawData(ImGui::GetDrawData());
    }

    ImGui_ImplNull_Shutdown();
    ImGui::DestroyContext();
    hash.Print("imgui.impl.null");
    return hash.VtxCount > 0 ? 0 : 1;
}
