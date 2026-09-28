// dear imgui: Renderer + Platform Backend for Allegro 5, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.allegro5;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <float.h>      // FLT_MAX

// Allegro (same as imgui_impl_allegro5.cpp)
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#ifdef _WIN32
#include <allegro5/allegro_windows.h>
#endif

export module imgui.impl.allegro5;

import std.compat;
import imgui;

#include "imgui_impl_allegro5.h"

module :private;

#include "imgui_impl_allegro5.cpp"
