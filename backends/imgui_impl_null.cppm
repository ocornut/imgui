// dear imgui: Null Platform+Renderer Backends, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.null;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"

export module imgui.impl.null;

import std.compat;
import imgui;

#include "imgui_impl_null.h"

module :private;

#include "imgui_impl_null.cpp"
