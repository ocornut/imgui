// dear imgui: Platform Backend for Windows (standard windows API for 32-bits AND 64-bits applications), as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.win32;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <float.h>      // FLT_MAX

// Windows (same as imgui_impl_win32.cpp)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <dwmapi.h>
#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
#include <xinput.h>
#endif

export module imgui.impl.win32;

import std.compat;
import imgui;

#include "imgui_impl_win32.h"

module :private;

#include "imgui_impl_win32.cpp"
