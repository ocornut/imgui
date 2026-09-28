// dear imgui: Renderer Backend for SDL_Renderer for SDL2, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.sdlrenderer2;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <stddef.h>     // offsetof

// SDL (same as imgui_impl_sdlrenderer2.cpp)
#include <SDL_render.h>
#include <SDL_version.h>

export module imgui.impl.sdlrenderer2;

import std.compat;
import imgui;

#include "imgui_impl_sdlrenderer2.h"

module :private;

#include "imgui_impl_sdlrenderer2.cpp"
