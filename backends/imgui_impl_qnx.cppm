// dear imgui: Platform Backend for QNX Screen, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.qnx;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <float.h>      // FLT_MAX

// QNX (same as imgui_impl_qnx.h, imgui_impl_qnx.cpp)
#include <screen/screen.h>
#include <sys/keycodes.h>

export module imgui.impl.qnx;

import std.compat;
import imgui;

#include "imgui_impl_qnx.h"

module :private;

#include "imgui_impl_qnx.cpp"
