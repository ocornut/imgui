// dear imgui
// (C++20 named module interface: 'import imgui;')

// Usage:
// - Compile this file as the (primary) module interface unit of module 'imgui', e.g. with CMake (see misc/cxx_module/CMakeLists.txt):
//     add_library(imgui)
//     target_sources(imgui PUBLIC FILE_SET CXX_MODULES FILES imgui.cppm)
//     set_target_properties(imgui PROPERTIES CXX_SCAN_FOR_MODULES ON CXX_MODULE_STD ON)
//   Do NOT additionally compile imgui.cpp, imgui_demo.cpp, imgui_draw.cpp, imgui_tables.cpp, imgui_widgets.cpp:
//   they are compiled as part of this unit (see the private module fragment at the bottom of this file).
// - Requires C++20 modules + the C++23 standard library modules ('import std.compat;', usable as an extension in C++20 by Clang/MSVC/GCC).
// - Then, in your code:
//     import imgui;
//     ImGui::CreateContext();
//     ImGui::NewFrame(); ...
// - Compile-time configuration (IMGUI_USER_CONFIG, imconfig.h, IMGUI_DISABLE_XXX, ImTextureID etc.) applies when compiling
//   THIS file. Every consumer of the module sees the configuration the module was built with.

// Design:
// - This follows the "ABI-breaking style" described in https://clang.llvm.org/docs/StandardCPlusPlusModules.html#abi-breaking-style
//   (as opposed to an "export-using style" wrapper that would list 'export using ::ImGui::Xxx;' for every symbol):
//   imgui.h is included in the module purview, so its declarations are attached to module 'imgui'.
//   Symbols are mangled differently from a regular (non-module) build of Dear ImGui: don't link both into the same program.
// - IMGUI_CXX_MODULE is defined below, before anything else. It is used by imgui.h and the .cpp files to:
//   - define IMGUI_EXPORT as 'export' and IMGUI_EXPORT_BEGIN/IMGUI_EXPORT_END as 'export {' / '}' (they expand to nothing in regular builds).
//     Namespaces of imgui.h are prefixed by IMGUI_EXPORT; declarations of the global namespace are enclosed in IMGUI_EXPORT_BEGIN/END blocks.
//   - skip including system/standard headers. The C library comes from 'import std.compat;'; the few headers still needed
//     for their macros (and OS headers) are included here in the global module fragment, so they remain attached to the global module.
// - imgui_internal.h is NOT part of the exported interface: it is only included by the .cpp files in the private module fragment.
//   Types that are opaque in imgui.h (e.g. ImGuiContext, ImDrawListSharedData) remain incomplete for importers.
// - Maths operators for ImVec2/ImVec4 (IMGUI_DEFINE_MATH_OPERATORS) are always enabled and exported,
//   since the implementation requires them. (Being found by ADL, they couldn't be hidden from importers anyway.)
// - Macros are not exported by C++20 modules: IMGUI_VERSION, IMGUI_CHECKVERSION(), IM_ASSERT(), IM_COL32(), IM_COUNTOF() etc.
//   are provided by imgui_macros.h, which importers may include (before or after 'import imgui;').
//   Some have exported constexpr alternatives, e.g. ImCol32(), ImCol32_White, ImCountOf(), ImGui::CheckVersion() (see imgui.h).

// Limitations (for now):
// - The private module fragment requires this file to be the only unit of module 'imgui'.
//   Backends are expected to be provided as separate modules that 'import imgui;'.
// - Not supported yet: IMGUI_ENABLE_FREETYPE, IMGUI_ENABLE_TEST_ENGINE, and linking stb_truetype/stb_rect_pack/stb_sprintf
//   implementations from another translation unit (IMGUI_DISABLE_STB_XXX_IMPLEMENTATION, IMGUI_DISABLE_STB_SPRINTF_IMPLEMENTATION),
//   since the corresponding declarations would be attached to module 'imgui' while their definitions are not.

//-----------------------------------------------------------------------------
// [SECTION] Global module fragment
//-----------------------------------------------------------------------------

module;

#define IMGUI_CXX_MODULE 1

// Version, configuration file with compile-time options (imconfig.h) and macros
// (included here rather than from imgui.h, so any user declarations stay attached to the global module)
#include "imgui_macros.h"

#if defined(IMGUI_ENABLE_FREETYPE)
#error "The 'imgui' C++20 module doesn't support IMGUI_ENABLE_FREETYPE yet."
#endif
#if defined(IMGUI_ENABLE_TEST_ENGINE)
#error "The 'imgui' C++20 module doesn't support IMGUI_ENABLE_TEST_ENGINE yet."
#endif

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

// C library functions and types come from 'import std.compat;' below. Modules don't export macros, so we still need:
#include <float.h>      // FLT_MAX, FLT_MIN, DBL_MAX
#include <limits.h>     // INT_MAX, INT_MIN, UINT_MAX
#include <stdarg.h>     // va_start, va_end, va_copy
#include <stddef.h>     // offsetof
#include <stdio.h>      // stdout
#ifndef IMGUI_DISABLE_TIME_FUNCTIONS
#include <time.h>       // localtime_r (POSIX)
#endif

// SSE intrinsics (same condition as in imgui_internal.h)
#if (defined __SSE__ || defined __x86_64__ || defined _M_X64 || (defined(_M_IX86_FP) && (_M_IX86_FP >= 1))) && !defined(IMGUI_DISABLE_SSE) && !defined(_M_ARM64) && !defined(_M_ARM64EC)
#include <immintrin.h>
#if (defined __AVX__ || defined __SSE4_2__)
#include <nmmintrin.h>
#endif
#endif

// [Windows] OS specific includes (see imgui.cpp)
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifndef IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
#include <shellapi.h>   // ShellExecuteW()
#endif
#include <imm.h>        // ImmGetContext() etc.
#endif

// [Apple] OS specific includes (see imgui.cpp)
#if defined(__APPLE__)
#include <TargetConditionals.h>
#if defined(TARGET_OS_OSX) && TARGET_OS_OSX && defined(IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS)
#include <Carbon/Carbon.h>
#endif
#endif

// [POSIX] Default shell functions (see imgui.cpp)
#if !defined(_WIN32) && !defined(IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS) && defined(__has_include)
#if __has_include(<sys/wait.h>) && __has_include(<unistd.h>)
#include <sys/wait.h>
#include <unistd.h>
#endif
#endif

// [Emscripten] (see imgui_demo.cpp)
#ifdef __EMSCRIPTEN__
#include <emscripten/version.h>
#endif

//-----------------------------------------------------------------------------
// [SECTION] Module interface
//-----------------------------------------------------------------------------

export module imgui;

import std.compat;

// Always enable maths operators: the implementation requires them, and imgui.h won't be included again by the .cpp files.
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"

// Internal functions used by backends (declared in imgui_internal.h)
#ifndef IMGUI_DISABLE
export namespace ImGui { IMGUI_API ImGuiIO& GetIO(ImGuiContext* ctx); } // imgui_impl_glfw.cpp
export IMGUI_API ImGuiID ImHashData(const void* data, size_t data_size, ImGuiID seed); // imgui_impl_wgpu.cpp
#endif

//-----------------------------------------------------------------------------
// [SECTION] Private module fragment (implementation)
//-----------------------------------------------------------------------------
// Nothing below is reachable from importers: this includes imgui_internal.h and all file-local (static) helpers.
// (same order as misc/single_file/imgui_single_file.h)

module :private;

#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"
