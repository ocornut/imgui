// dear imgui: Renderer Backend for SDL using SDL_Renderer
// (Requires: SDL 2.0.17+)


#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

struct SDL_Renderer;

IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer_Init(SDL_Renderer *renderer);
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_DestroyDeviceObjects();
