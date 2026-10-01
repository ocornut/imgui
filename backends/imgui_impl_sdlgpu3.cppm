// dear imgui: Renderer Backend for SDL_GPU (SDL3), as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.sdlgpu3;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <stddef.h>     // offsetof

// SDL (same as imgui_impl_sdlgpu3.h)
#include <SDL3/SDL_gpu.h>

export module imgui.impl.sdlgpu3;

import std.compat;
import imgui;

#include "imgui_impl_sdlgpu3.h"

module :private;

#include "imgui_impl_sdlgpu3.cpp"
