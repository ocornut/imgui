// dear imgui, v1.93.0 WIP
// (macros)

// Version, compile-time configuration and all preprocessor macros of the public API.
// - This is included by imgui.h: you don't need to include it yourself when using imgui.h.

#pragma once

// Library Version
// (Integer encoded as XYYZZ for use in #if preprocessor conditionals, e.g. '#if IMGUI_VERSION_NUM >= 12345')
#define IMGUI_VERSION       "1.93.0 WIP"
#define IMGUI_VERSION_NUM   19297
#define IMGUI_HAS_TABLE             // Added BeginTable() - from IMGUI_VERSION_NUM >= 18000
#define IMGUI_HAS_TEXTURES          // Added ImGuiBackendFlags_RendererHasTextures - from IMGUI_VERSION_NUM >= 19198

// Configuration file with compile-time options
// (edit imconfig.h or '#define IMGUI_USER_CONFIG "myfilename.h" from your build system)
#ifdef IMGUI_USER_CONFIG
#include IMGUI_USER_CONFIG
#endif
#include "imconfig.h"

#ifndef IMGUI_DISABLE

//-----------------------------------------------------------------------------
// [SECTION] Attributes and helper macros
//-----------------------------------------------------------------------------

// Define attributes of all API symbols declarations (e.g. for DLL under Windows)
// IMGUI_API is used for core imgui functions, IMGUI_IMPL_API is used for the default backends files (imgui_impl_xxx.h)
// Using dear imgui via a shared library is not recommended: we don't guarantee backward nor forward ABI compatibility + this is a call-heavy library and function call overhead adds up.
#ifndef IMGUI_API
#define IMGUI_API
#endif
#ifndef IMGUI_IMPL_API
#define IMGUI_IMPL_API              IMGUI_API
#endif

// Constexpr alternatives to some of the macros below (e.g. ImCol32() for IM_COL32()) are declared in imgui.h ('Constexpr alternatives to macros' section) when compiling as C++17 or later.
// (C++17 is required for 'inline' variables)
#if (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L
#define IMGUI_HAS_CONSTEXPR
#endif

// Helper Macros
// (note: compiling with NDEBUG will usually strip out assert() to nothing, which is NOT recommended because we use asserts to notify of programmer mistakes.)
#ifndef IM_ASSERT
#include <assert.h>
#define IM_ASSERT(_EXPR)            assert(_EXPR)                               // You can override the default assert handler by editing imconfig.h
#endif
#define IM_COUNTOF(_ARR)            ((int)(sizeof(_ARR) / sizeof(*(_ARR))))     // Size of a static C-style array. Don't use on pointers!
#define IM_UNUSED(_VAR)             ((void)(_VAR))                              // Used to silence "unused variable warnings". Often useful as asserts may be stripped out from final builds.
#define IM_STRINGIFY_HELPER(_EXPR)  #_EXPR
#define IM_STRINGIFY(_EXPR)         IM_STRINGIFY_HELPER(_EXPR)                  // Preprocessor idiom to stringify e.g. an integer or a macro.

// Check that version and structures layouts are matching between compiled imgui code and caller. Read comments above DebugCheckVersionAndDataLayout() for details.
#define IMGUI_CHECKVERSION()        ImGui::DebugCheckVersionAndDataLayout(IMGUI_VERSION, sizeof(ImGuiIO), sizeof(ImGuiStyle), sizeof(ImVec2), sizeof(ImVec4), sizeof(ImDrawVert), sizeof(ImDrawIdx))

// Helper Macros - IM_FMTARGS, IM_FMTLIST: Apply printf-style warnings to our formatting functions.
// (MSVC provides an equivalent mechanism via SAL Annotations but it requires the macros in a different
//  location. e.g. #include <sal.h> + void myprintf(_Printf_format_string_ const char* format, ...),
//  and only works when using Code Analysis, rather than just normal compiling).
// (see https://github.com/ocornut/imgui/issues/8871 for a patch to enable this for MSVC's Code Analysis)
#if !defined(IMGUI_USE_STB_SPRINTF) && defined(__MINGW32__) && !defined(__clang__)
#define IM_FMTARGS(FMT)             __attribute__((format(gnu_printf, FMT, FMT+1)))
#define IM_FMTLIST(FMT)             __attribute__((format(gnu_printf, FMT, 0)))
#elif !defined(IMGUI_USE_STB_SPRINTF) && (defined(__clang__) || defined(__GNUC__))
#define IM_FMTARGS(FMT)             __attribute__((format(printf, FMT, FMT+1)))
#define IM_FMTLIST(FMT)             __attribute__((format(printf, FMT, 0)))
#else
#define IM_FMTARGS(FMT)
#define IM_FMTLIST(FMT)
#endif

// Disable some of MSVC most aggressive Debug runtime checks in function header/footer (used in some simple/low-level functions)
#if defined(_MSC_VER) && !defined(__clang__)  && !defined(__INTEL_COMPILER) && !defined(IMGUI_DEBUG_PARANOID)
#define IM_MSVC_RUNTIME_CHECKS_OFF      __pragma(runtime_checks("",off))     __pragma(check_stack(off)) __pragma(strict_gs_check(push,off))
#define IM_MSVC_RUNTIME_CHECKS_RESTORE  __pragma(runtime_checks("",restore)) __pragma(check_stack())    __pragma(strict_gs_check(pop))
#else
#define IM_MSVC_RUNTIME_CHECKS_OFF
#define IM_MSVC_RUNTIME_CHECKS_RESTORE
#endif

// Alternative to using a .natstepfilter file or other scripts in misc/debuggers/ to skip debug-stepping selected trivial functions.
// If you get a compiler error or warning related to use, please report it to us!
#if (defined(__clang__) && (__clang_major__ >= 7)) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))
#define IM_NODEBUGSTEP      [[gnu::artificial]]
#elif defined(_MSC_VER) && (_MSC_VER >= 1915)
#define IM_NODEBUGSTEP      __declspec(non_user_code)
#else
#define IM_NODEBUGSTEP
#endif

//-----------------------------------------------------------------------------
// [SECTION] Constants and helpers macros
//-----------------------------------------------------------------------------

// Define this if you need to change the invalid value for your backend.
// - If your backend is using ImTextureID to store an index/offset and you need 0 to be valid, You can add '#define ImTextureID_Invalid ((ImTextureID)-1)' in your imconfig.h file.
// - From 2026/03/12 to 2026/03/19 we experimented with changing to default to -1, but I worried it would cause too many issues in third-party code so it was reverted.
#ifndef ImTextureID_Invalid
#define ImTextureID_Invalid     ((ImTextureID)0)
#endif

// Standard Drag and Drop payload types. You can define you own payload types using short strings. Types starting with '_' are defined by Dear ImGui.
#define IMGUI_PAYLOAD_TYPE_COLOR_3F     "_COL3F"    // float[3]: Standard type for colors, without alpha. User code may use this type.
#define IMGUI_PAYLOAD_TYPE_COLOR_4F     "_COL4F"    // float[4]: Standard type for colors. User code may use this type.

// Debug Logging into ShowDebugLogWindow(), tty and more.
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
#define IMGUI_DEBUG_LOG(...)        ImGui::DebugLog(__VA_ARGS__)
#else
#define IMGUI_DEBUG_LOG(...)        ((void)0)
#endif

// IM_MALLOC(), IM_FREE(), IM_NEW(), IM_PLACEMENT_NEW() (+ IM_DELETE() function in imgui.h)
#define IM_ALLOC(_SIZE)                     ImGui::MemAlloc(_SIZE)
#define IM_FREE(_PTR)                       ImGui::MemFree(_PTR)
#define IM_PLACEMENT_NEW(_PTR)              new(ImNewWrapper(), _PTR)
#define IM_NEW(_TYPE)                       new(ImNewWrapper(), ImGui::MemAlloc(sizeof(_TYPE))) _TYPE

// Helper: Unicode defines
#define IM_UNICODE_CODEPOINT_INVALID 0xFFFD     // Invalid Unicode code point (standard value).
#ifdef IMGUI_USE_WCHAR32
#define IM_UNICODE_CODEPOINT_MAX     0x10FFFF   // Maximum Unicode code point supported by this build.
#else
#define IM_UNICODE_CODEPOINT_MAX     0xFFFF     // Maximum Unicode code point supported by this build.
#endif

// Helpers macros to generate 32-bit encoded colors
// - User can declare their own format by #defining the 5 _SHIFT/_MASK macros in their imconfig file.
// - Any setting other than the default will need custom backend support. The only standard backend that supports anything else than the default is DirectX9.
#ifndef IM_COL32_R_SHIFT
#ifdef IMGUI_USE_BGRA_PACKED_COLOR
#define IM_COL32_R_SHIFT    16
#define IM_COL32_G_SHIFT    8
#define IM_COL32_B_SHIFT    0
#define IM_COL32_A_SHIFT    24
#define IM_COL32_A_MASK     0xFF000000
#else
#define IM_COL32_R_SHIFT    0
#define IM_COL32_G_SHIFT    8
#define IM_COL32_B_SHIFT    16
#define IM_COL32_A_SHIFT    24
#define IM_COL32_A_MASK     0xFF000000
#endif
#endif
#define IM_COL32(R,G,B,A)    (((ImU32)(A)<<IM_COL32_A_SHIFT) | ((ImU32)(B)<<IM_COL32_B_SHIFT) | ((ImU32)(G)<<IM_COL32_G_SHIFT) | ((ImU32)(R)<<IM_COL32_R_SHIFT))
#define IM_COL32_WHITE       IM_COL32(255,255,255,255)  // Opaque white = 0xFFFFFFFF
#define IM_COL32_BLACK       IM_COL32(0,0,0,255)        // Opaque black
#define IM_COL32_BLACK_TRANS IM_COL32(0,0,0,0)          // Transparent black = 0x00000000

// The maximum line width to bake anti-aliased textures for. Build atlas with ImFontAtlasFlags_NoBakedLines to disable baking.
#ifndef IM_DRAWLIST_TEX_LINES_WIDTH_MAX
#define IM_DRAWLIST_TEX_LINES_WIDTH_MAX     (32)
#endif

// An opaque identifier to a rectangle in the atlas (ImFontAtlasRectId). -1 when invalid.
#define ImFontAtlasRectId_Invalid -1

//-----------------------------------------------------------------------------
// [SECTION] Obsolete macros
// (Will be removed! Read 'API BREAKING CHANGES' section in imgui.cpp for details)
//-----------------------------------------------------------------------------

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#define ImDrawCallback_ResetRenderState     (ImDrawCallback)(-8)    // OBSOLETED in 1.92.8: Use ImGui::GetPlatformIO().DrawCallback_ResetRenderState
#endif

#define IM_ARRAYSIZE                IM_COUNTOF                  // RENAMED IN 1.92.6: IM_ARRAYSIZE -> IM_COUNTOF

// RENAMED IMGUI_DISABLE_METRICS_WINDOW > IMGUI_DISABLE_DEBUG_TOOLS in 1.88 (from June 2022)
#ifdef IMGUI_DISABLE_METRICS_WINDOW
#error IMGUI_DISABLE_METRICS_WINDOW was renamed to IMGUI_DISABLE_DEBUG_TOOLS, please use new name.
#endif

#endif // #ifndef IMGUI_DISABLE
