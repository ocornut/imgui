// dear imgui: header-only (non-module) test for imgui_internal.h.
// imgui_internal.h must work on its own, including after imgui_macros.h (which defines IMGUI_VERSION without the declarations of imgui.h).

#include "imgui_macros.h"
#include "imgui_internal.h"
#include <stdio.h>

int main()
{
    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(ctx);            // imgui_internal.h API
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;   // (no renderer: textures are never uploaded)

    ImGuiWindow* window = nullptr;
    for (int frame = 0; frame < 2; frame++)     // new windows are hidden on their first frame
    {
        ImGui::NewFrame();
        ImGui::Begin("Test");
        window = ImGui::GetCurrentWindow();
        ImGui::Text("Hello");
        ImGui::End();
        ImGui::Render();
    }

    const bool ok = window != nullptr && ImGui::GetDrawData()->TotalVtxCount > 0 && ctx->FrameCount == 2;
    printf("window=%p vtx=%d frame=%d\n", (void*)window, ImGui::GetDrawData()->TotalVtxCount, ctx->FrameCount);
    ImGui::DestroyContext(ctx);
    return ok ? 0 : 1;
}
