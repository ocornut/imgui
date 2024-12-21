// dear imgui: Renderer Backend for SDL_gpu for SDL3
// (Requires: SDL 3.0.0+)

// (**IMPORTANT: SDL 3.0.0 is NOT YET RELEASED AND CURRENTLY HAS A FAST CHANGING API. THIS CODE BREAKS OFTEN AS SDL3 CHANGES.**)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'SDL_Texture*' as ImTextureID. Read the FAQ about ImTextureID!
//  [ ] Renderer: Large meshes support (64k+ vertices) with 16-bit indices.
// Missing features:
//  [ ] Renderer: Multi-viewport support (multiple windows).

// You can copy and use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#pragma once
#include "imgui.h"  // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

struct SDL_GPUDevice;
struct SDL_Window;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool ImGui_ImplSDLGPU3_Init(SDL_GPUDevice* device, SDL_Window* window);
IMGUI_IMPL_API void ImGui_ImplSDLGPU3_Shutdown();
IMGUI_IMPL_API void ImGui_ImplSDLGPU3_NewFrame();
IMGUI_IMPL_API void ImGui_ImplSDLGPU3_RenderDrawData(ImDrawData* draw_data, SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* render_target);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool ImGui_ImplSDLGPU3_CreateFontsTexture();
IMGUI_IMPL_API void ImGui_ImplSDLGPU3_DestroyFontsTexture();
IMGUI_IMPL_API bool ImGui_ImplSDLGPU3_CreateDeviceObjects();
IMGUI_IMPL_API void ImGui_ImplSDLGPU3_DestroyDeviceObjects();

#endif  // #ifndef IMGUI_DISABLE
