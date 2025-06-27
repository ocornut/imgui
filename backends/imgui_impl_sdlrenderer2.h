// dear imgui: Renderer Backend for SDL_Renderer for SDL2
// (Requires: SDL 2.0.17+)

// Note that SDL_Renderer is an _optional_ component of SDL2, which IMHO is now largely obsolete.
// For a multi-platform app consider using other technologies:
// - SDL3+SDL_GPU: SDL_GPU is SDL3 new graphics abstraction API. You will need to update to SDL3.
// - SDL2+DirectX, SDL2+OpenGL, SDL2+Vulkan: combine SDL with dedicated renderers.
// If your application wants to render any non trivial amount of graphics other than UI,
// please be aware that SDL_Renderer currently offers a limited graphic API to the end-user
// and it might be difficult to step out of those boundaries.

// Implemented features:
//  [X] Renderer: User texture binding. Use 'SDL_Texture*' as texture identifier. Read the FAQ about ImTextureID/ImTextureRef!
//  [X] Renderer: Large meshes support (64k+ vertices) even with 16-bit indices (ImGuiBackendFlags_RendererHasVtxOffset).
//  [X] Renderer: Texture updates support for dynamic font atlas (ImGuiBackendFlags_RendererHasTextures).
//  [X] Renderer: Expose selected render state for draw callbacks to use. Access in '(ImGui_ImplXXXX_RenderState*)GetPlatformIO().Renderer_RenderState'.
// Missing features:
//  [ ] Renderer: Multi-viewport support (multiple windows).

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#pragma once
#ifndef IMGUI_DISABLE
#include "imgui.h"      // IMGUI_IMPL_API

struct SDL_Renderer;

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer2_Init(SDL_Renderer* renderer);
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer2_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer2_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer2_RenderDrawData(ImDrawData* draw_data, SDL_Renderer* renderer);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer2_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer2_DestroyDeviceObjects();

// (Advanced) Use e.g. if you need to precisely control the timing of texture updates (e.g. for staged rendering), by setting ImDrawData::Textures = NULL to handle this manually.
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer2_UpdateTexture(ImTextureData* tex);

// [BETA] Selected render state data shared with callbacks.
// This is temporarily stored in GetPlatformIO().Renderer_RenderState during the ImGui_ImplSDLRenderer2_RenderDrawData() call.
// (Please open an issue if you feel you need access to more data)
struct ImGui_ImplSDLRenderer2_RenderState
{
    SDL_Renderer*       Renderer;
};

#endif // #ifndef IMGUI_DISABLE
