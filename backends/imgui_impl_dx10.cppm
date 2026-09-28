// dear imgui: Renderer Backend for DirectX10, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.dx10;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <stddef.h>     // offsetof

// DirectX (same as imgui_impl_dx10.cpp)
#include <d3d10_1.h>
#include <d3d10.h>
#include <d3dcompiler.h>

export module imgui.impl.dx10;

import std.compat;
import imgui;

#include "imgui_impl_dx10.h"

module :private;

#include "imgui_impl_dx10.cpp"
