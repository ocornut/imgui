// dear imgui: Platform Backend for SDL3, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.sdl3;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <float.h>      // FLT_MAX

// SDL (same as imgui_impl_sdl3.cpp)
#include <SDL3/SDL.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

export module imgui.impl.sdl3;

import std.compat;
import imgui;

#include "imgui_impl_sdl3.h"

module :private;

#include "imgui_impl_sdl3.cpp"
