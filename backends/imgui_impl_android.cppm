// dear imgui: Platform Binding for Android native app, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.android;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <float.h>      // FLT_MAX
#include <time.h>       // CLOCK_MONOTONIC

// Android NDK (same as imgui_impl_android.cpp)
#include <android/native_window.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android/log.h>

export module imgui.impl.android;

import std.compat;
import imgui;

#include "imgui_impl_android.h"

module :private;

#include "imgui_impl_android.cpp"
