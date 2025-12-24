// imgui_impl_sdlsurface2.h
// CPU-only SDL_Surface backend for Dear ImGui
#pragma once

#include "imgui.h"
#include <SDL.h>

// Initialize with target SDL_Surface (32-bit RGBA)
IMGUI_API bool ImGui_ImplSDLSurface2_Init(SDL_Surface* surface);
IMGUI_API void ImGui_ImplSDLSurface2_Shutdown();
IMGUI_API void ImGui_ImplSDLSurface2_NewFrame();
IMGUI_API void ImGui_ImplSDLSurface2_RenderDrawData(ImDrawData* draw_data);

IMGUI_API SDL_Surface* ImGui_ImplSDLSurface2_CreateFontAtlasSurface();
