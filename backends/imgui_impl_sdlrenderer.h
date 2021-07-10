// dear imgui: Renderer Backend for SDL using SDL_Renderer


#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

#include "SDL.h"

IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer_Init(SDL_Window *window);
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplSDLRenderer_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplSDLRenderer_DestroyFontsTexture();










