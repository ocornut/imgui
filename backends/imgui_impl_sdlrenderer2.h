// dear imgui: Renderer Backend for SDL_Renderer for SDL2
// (Requires: SDL 2.0.17+)

// Note how SDL_Renderer is an _optional_ component of SDL2.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.
// If your application will want to render any non trivial amount of graphics other than UI,
// please be aware that SDL_Renderer currently offers a limited graphic API to the end-user and
// it might be difficult to step out of those boundaries.

// Implemented features:
//  [X] Renderer: User texture binding. Use 'SDL_Texture*' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Renderer: Large meshes support (64k+ vertices) with 16-bit indices.
// Missing features:
//  [ ] Renderer: Multi-viewport support (multiple windows).

#pragma once
#ifndef IMGUI_DISABLE
#include "imgui.h"      // IMGUI_IMPL_API

struct SDL_Renderer;

IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer2_Init(SDL_Renderer* renderer);
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer2_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer2_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer2_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer2_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer2_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer2_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer2_DestroyDeviceObjects();

#endif // #ifndef IMGUI_DISABLE
