// dear imgui: compile/run test for the C++20 named module 'imgui' (see imgui.cppm).
// This translation unit only imports modules: no macros are available, so it uses the constexpr alternatives.
// See test_module_macros.cpp for a translation unit that also includes imgui_macros.h.

import std;
import imgui;

#if defined(IMGUI_VERSION) || defined(IM_COL32) || defined(IM_ASSERT) || defined(IMGUI_EXPORT)
#error "Macros must not leak through 'import imgui;'"
#endif

// Constexpr alternatives to macros
static_assert(ImGuiVersionNum >= 19200);
static_assert(ImGuiVersion[0] != 0);
static_assert(ImCol32(255, 255, 255, 255) == ImCol32_White);
static_assert(ImCol32(0, 0, 0, 255) == ImCol32_Black);
static_assert(ImCol32_BlackTrans == 0);
static_assert(((ImCol32(0x12, 0x34, 0x56, 0x78) >> ImCol32_GShift) & 0xFF) == 0x34);
static_assert((ImCol32_White & ImCol32_AMask) == ImCol32_AMask);
static_assert(ImUnicodeCodepoint_Invalid == 0xFFFD);
static_assert(std::string_view(ImGuiPayloadType_Color4F) == "_COL4F");
constexpr int ints[] = { 1, 2, 3 };
static_assert(ImCountOf(ints) == 3);

// Math operators are exported
static_assert(requires(ImVec2 a, ImVec2 b) { a + b; a * 2.0f; a += b; });

// Exported types/enums
static_assert(std::is_enum_v<ImGuiWindowFlags_>);
static_assert(sizeof(ImDrawIdx) == 2 || sizeof(ImDrawIdx) == 4);
static_assert(std::is_same_v<decltype(ImGui::GetIO()), ImGuiIO&>);

// Internal types stay incomplete (imgui_internal.h is not exported)
template<typename T> concept IsComplete = requires { sizeof(T); };
static_assert(!IsComplete<ImGuiContext>);

int TestModuleWithMacros(); // test_module_macros.cpp

static void RenderTextures(ImDrawData* draw_data)
{
    // Minimal renderer: acknowledge texture requests (same as imgui_impl_null.cpp)
    if (draw_data->Textures != nullptr)
        for (ImTextureData* tex : *draw_data->Textures)
        {
            if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates)
                tex->SetStatus(ImTextureStatus_OK);
            else if (tex->Status == ImTextureStatus_WantDestroy)
                tex->SetStatus(ImTextureStatus_Destroyed);
        }
}

int main()
{
    if (!ImGui::CheckVersion())
        return 1;

    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;
    io.IniFilename = nullptr;

    int vtx_count = 0;
    for (int n = 0; n < 5; n++)
    {
        io.DisplaySize = ImVec2(1280, 720);
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();

        static float f = 0.5f;
        static std::array<char, 64> buf = { "Hello, module!" };
        ImGui::Begin("Module test");
        ImGui::Text("Dear ImGui %s (%d)", ImGui::GetVersion(), ImGuiVersionNum);
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::InputText("text", buf.data(), buf.size());
        if (ImGui::BeginTable("table", 2))
        {
            ImGui::TableNextColumn(); ImGui::TextUnformatted("A");
            ImGui::TableNextColumn(); ImGui::TextUnformatted("B");
            ImGui::EndTable();
        }
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 p = ImGui::GetCursorScreenPos();
        draw_list->AddRect(p, p + ImVec2(50, 50) * 2.0f, ImCol32(255, 0, 0, 255));
        ImGui::End();
        ImGui::ShowDemoWindow(nullptr);

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        RenderTextures(draw_data);
        vtx_count = draw_data->TotalVtxCount;
    }

    ImVector<int> v;
    v.push_back(1);
    v.push_back(2);

    const int macros_result = TestModuleWithMacros();

    ImGui::DestroyContext(ctx);

    std::println("imgui {}: vtx_count={} vector_size={} macros_result={}", ImGuiVersion, vtx_count, v.Size, macros_result);
    return (vtx_count > 0 && v.Size == 2 && macros_result == 0) ? 0 : 1;
}
