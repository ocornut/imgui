// dear imgui: compile test for the C++20 named module 'imgui' (see imgui.cppm), using the macros from imgui_macros.h.
// Modules don't export macros: include imgui_macros.h (with the same configuration as the module) before importing.

#include "imgui_macros.h"
import std;
import imgui;

// Macros and constexpr alternatives must agree
static_assert(IMGUI_VERSION_NUM == ImGuiVersionNum);
static_assert(IM_COL32(1, 2, 3, 4) == ImCol32(1, 2, 3, 4));
static_assert(IM_COL32_WHITE == ImCol32_White);
static_assert(IM_UNICODE_CODEPOINT_MAX == ImUnicodeCodepoint_Max);

struct TestObject
{
    int Value;
    TestObject(int value) : Value(value) {}
};

int TestModuleWithMacros()
{
    if (!IMGUI_CHECKVERSION())
        return 1;

    // IM_NEW/IM_DELETE rely on the exported ImNewWrapper placement new and IM_DELETE template
    TestObject* obj = IM_NEW(TestObject)(42);
    IM_ASSERT(obj->Value == 42);
    IM_DELETE(obj);

    const char* names[] = { "a", "b", "c", "d" };
    IM_UNUSED(names);
    static_assert(IM_COUNTOF(names) == 4);

    ImGui::NewFrame();
    ImGui::Begin("Macros");
    ImGui::GetWindowDrawList()->AddLine(ImVec2(0, 0), ImVec2(10, 10), IM_COL32_WHITE);
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        const float col[4] = { 1, 0, 0, 1 };
        ImGui::SetDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F, col, sizeof(col));
        ImGui::EndDragDropSource();
    }
    ImGui::End();
    ImGui::EndFrame();
    return 0;
}
