// dear imgui: Renderer Backend for WebGPU, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.wgpu;'
// Define one of IMGUI_IMPL_WEBGPU_BACKEND_DAWN, IMGUI_IMPL_WEBGPU_BACKEND_WGPU, IMGUI_IMPL_WEBGPU_BACKEND_WGVK when compiling this file (see imgui_impl_wgpu.h).

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <limits.h>     // UINT_MAX

// WebGPU (same as imgui_impl_wgpu.h, imgui_impl_wgpu.cpp)
#if defined(__EMSCRIPTEN__) && !defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN) && !defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
#include <emscripten/version.h>
#endif
#include <webgpu/webgpu.h>
#if defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU) && !defined(__EMSCRIPTEN__)
#include <webgpu/wgpu.h>
#endif
#if defined(__APPLE__) && !defined(__EMSCRIPTEN__)
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#include <Cocoa/Cocoa.h>
#include <QuartzCore/CAMetalLayer.h>
#endif
#endif

export module imgui.impl.wgpu;

import std.compat;
import imgui;

#include "imgui_impl_wgpu.h"

module :private;

#include "imgui_impl_wgpu.cpp"
