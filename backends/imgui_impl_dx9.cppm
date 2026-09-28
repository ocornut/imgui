// dear imgui: Renderer Backend for DirectX9, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.dx9;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"

// DirectX (same as imgui_impl_dx9.cpp)
#include <d3d9.h>

export module imgui.impl.dx9;

import std.compat;
import imgui;

#include "imgui_impl_dx9.h"

module :private;

#include "imgui_impl_dx9.cpp"
