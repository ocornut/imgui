// dear imgui: Renderer Backend for DirectX12, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.dx12;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <stddef.h>     // offsetof

// DirectX (same as imgui_impl_dx12.h, imgui_impl_dx12.cpp)
#include <dxgiformat.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <d3dcompiler.h>

export module imgui.impl.dx12;

import std.compat;
import imgui;

#include "imgui_impl_dx12.h"

module :private;

#include "imgui_impl_dx12.cpp"
