// dear imgui: Platform Backend for SDL2, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.sdl2;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <float.h>      // FLT_MAX

// SDL (same as imgui_impl_sdl2.cpp)
#include <SDL.h>
#include <SDL_syswm.h>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten/em_js.h>
#endif
#if SDL_VERSION_ATLEAST(2,0,6)
#include <SDL_vulkan.h>
#endif
#if defined(__APPLE__) && SDL_VERSION_ATLEAST(2,0,14)
#include <SDL_metal.h>
#endif

export module imgui.impl.sdl2;

import std.compat;
import imgui;

#include "imgui_impl_sdl2.h"

module :private;

#include "imgui_impl_sdl2.cpp"
