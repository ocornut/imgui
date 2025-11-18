// dear imgui: Null Platform+Renderer Backends
// This is designed if you need to use a blind Dear Imgui context with no input and no output.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2025-11-17: Initial version.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_null.h"

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning: use of old-style cast                            // yes, they are more terse.
#endif

IMGUI_IMPL_API bool ImGui_ImplNull_Init()
{
    ImGui_ImplNullPlatform_Init();
    ImGui_ImplNullRender_Init();
    return true;
}

IMGUI_IMPL_API void ImGui_ImplNull_Shutdown()
{
    ImGui_ImplNullRender_Shutdown();
    ImGui_ImplNullPlatform_Shutdown();
}

IMGUI_IMPL_API void ImGui_ImplNull_NewFrame()
{
    ImGui_ImplNullPlatform_NewFrame();
    ImGui_ImplNullRender_NewFrame();
}

IMGUI_IMPL_API bool ImGui_ImplNullPlatform_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    return true;
}

IMGUI_IMPL_API void ImGui_ImplNullPlatform_Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags &= ~ImGuiBackendFlags_HasMouseCursors;
}

IMGUI_IMPL_API void ImGui_ImplNullPlatform_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);
    io.DeltaTime = 1.0f / 60.0f;
}

IMGUI_IMPL_API bool ImGui_ImplNullRender_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    return true;
}

IMGUI_IMPL_API void ImGui_ImplNullRender_Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasTextures;
}

IMGUI_IMPL_API void ImGui_ImplNullRender_NewFrame()
{
}

static void ImGui_ImplNullRender_UpdateTexture(ImTextureData* tex)
{
    if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantDestroy)
        tex->SetStatus(ImTextureStatus_OK);
    if (tex->Status == ImTextureStatus_WantDestroy)
    {
        tex->SetTexID(ImTextureID_Invalid);
        tex->SetStatus(ImTextureStatus_Destroyed);
    }
}

IMGUI_IMPL_API void ImGui_ImplNullRender_RenderDrawData(ImDrawData* draw_data)
{
    if (draw_data->Textures != nullptr)
        for (ImTextureData* tex : *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_ImplNullRender_UpdateTexture(tex);
}

#endif // #ifndef IMGUI_DISABLE
