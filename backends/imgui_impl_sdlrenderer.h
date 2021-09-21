// dear imgui: Renderer Backend for SDL_Renderer
// (Requires: SDL 2.0.17+)

// Important to understand: SDL_Renderer is an _optional_ component of SDL. We do not recommend you use SDL_Renderer
// because it provide a rather limited API to the end-user. We provide this backend for the sake of completeness.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

// Implemented features:
//  [X] Renderer: User texture binding. Use 'SDL_Texture*' as ImTextureID. Read the FAQ about ImTextureID!
// Missing features:
//  [ ] Renderer: Support for large meshes (64k+ vertices) with 16-bit indices.

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

struct SDL_Renderer;

IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer_Init(SDL_Renderer* renderer);
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_DestroyDeviceObjects();
