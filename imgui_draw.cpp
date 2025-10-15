// dear imgui, v1.92.5 WIP
// (drawing and font code)

/*

Index of this file:

// [SECTION] STB libraries implementation
// [SECTION] Style functions
// [SECTION] ImDrawList
// [SECTION] ImTriangulator, ImDrawList concave polygon fill
// [SECTION] ImDrawListSplitter
// [SECTION] ImDrawData
// [SECTION] Helpers ShadeVertsXXX functions
// [SECTION] ImFontConfig
// [SECTION] ImFontAtlas, ImFontAtlasBuilder
// [SECTION] ImFontAtlas: backend for stb_truetype
// [SECTION] ImFontAtlas: glyph ranges helpers
// [SECTION] ImFontGlyphRangesBuilder
// [SECTION] ImFont
// [SECTION] ImGui Internal Render Helpers
// [SECTION] Decompression code
// [SECTION] Default font data (ProggyClean.ttf)

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_internal.h"
#ifdef IMGUI_ENABLE_FREETYPE
#include "misc/freetype/imgui_freetype.h"
#endif

#include <stdio.h>      // vsnprintf, sscanf, printf
#include <stdint.h>     // intptr_t

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)     // condition expression is constant
#pragma warning (disable: 4505)     // unreferenced local function has been removed (stb stuff)
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#pragma warning (disable: 26451)    // [Static Analyzer] Arithmetic overflow : Using operator 'xxx' on a 4 byte value and then casting the result to a 8 byte value. Cast the value to the wider type before calling operator 'xxx' to avoid overflow(io.2).
#pragma warning (disable: 26812)    // [Static Analyzer] The enum type 'xxx' is unscoped. Prefer 'enum class' over 'enum' (Enum.3).
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                      // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"                    // warning: comparing floating point with == or != is unsafe // storing and comparing against same constants ok.
#pragma clang diagnostic ignored "-Wglobal-constructors"            // warning: declaration requires a global destructor         // similar to above, not sure what the exact difference is.
#pragma clang diagnostic ignored "-Wsign-conversion"                // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                    // some standard header variations use #define NULL 0
#pragma clang diagnostic ignored "-Wcomma"                          // warning: possible misuse of comma operator here
#pragma clang diagnostic ignored "-Wreserved-id-macro"              // warning: macro name is a reserved identifier
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#pragma clang diagnostic ignored "-Wreserved-identifier"            // warning: identifier '_Xxx' is reserved because it starts with '_' followed by a capital letter
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"            // warning: 'xxx' is an unsafe pointer used for buffer access
#pragma clang diagnostic ignored "-Wnontrivial-memaccess"           // warning: first argument in call to 'memset' is a pointer to non-trivially copyable type
#pragma clang diagnostic ignored "-Wcast-qual"                      // warning: cast from 'const xxxx *' to 'xxx *' drops const qualifier
#pragma clang diagnostic ignored "-Wswitch-default"                 // warning: 'switch' missing 'default' label
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                          // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wunused-function"                  // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wfloat-equal"                      // warning: comparing floating-point with '==' or '!=' is unsafe
#pragma GCC diagnostic ignored "-Wdouble-promotion"                 // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"                       // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wstack-protector"                  // warning: stack protector not protecting local variables: variable length buffer
#pragma GCC diagnostic ignored "-Wstrict-overflow"                  // warning: assuming signed overflow does not occur when simplifying division / ..when changing X +- C1 cmp C2 to X cmp C2 -+ C1
#pragma GCC diagnostic ignored "-Wclass-memaccess"                  // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#pragma GCC diagnostic ignored "-Wcast-qual"                        // warning: cast from type 'const xxxx *' to type 'xxxx *' casts away qualifiers
#endif

//-------------------------------------------------------------------------
// [SECTION] STB libraries implementation (for stb_truetype and stb_rect_pack)
//-------------------------------------------------------------------------

// Compile time options:
//#define IMGUI_STB_NAMESPACE           ImStb
//#define IMGUI_STB_TRUETYPE_FILENAME   "my_folder/stb_truetype.h"
//#define IMGUI_STB_RECT_PACK_FILENAME  "my_folder/stb_rect_pack.h"
//#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION

#ifdef IMGUI_STB_NAMESPACE
namespace IMGUI_STB_NAMESPACE
{
#endif

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4456)                             // declaration of 'xx' hides previous local declaration
#pragma warning (disable: 6011)                             // (stb_rectpack) Dereferencing NULL pointer 'cur->next'.
#pragma warning (disable: 6385)                             // (stb_truetype) Reading invalid data from 'buffer':  the readable size is '_Old_3`kernel_width' bytes, but '3' bytes may be read.
#pragma warning (disable: 28182)                            // (stb_rectpack) Dereferencing NULL pointer. 'cur' contains the same NULL value as 'cur->next' did.
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"        // warning: 'xxxx' defined but not used
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"              // warning: comparison is always true due to limited range of data type [-Wtype-limits]
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"     // warning: this statement may fall through
#endif

#ifndef STB_RECT_PACK_IMPLEMENTATION                        // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION          // in case the user already have an implementation in another compilation unit
#define STBRP_STATIC
#define STBRP_ASSERT(x)     do { IM_ASSERT(x); } while (0)
#define STBRP_SORT          ImQsort
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#ifdef IMGUI_STB_RECT_PACK_FILENAME
#include IMGUI_STB_RECT_PACK_FILENAME
#else
#include "imstb_rectpack.h"
#endif
#endif

#ifdef  IMGUI_ENABLE_STB_TRUETYPE
#ifndef STB_TRUETYPE_IMPLEMENTATION                         // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION           // in case the user already have an implementation in another compilation unit
#define STBTT_malloc(x,u)   ((void)(u), IM_ALLOC(x))
#define STBTT_free(x,u)     ((void)(u), IM_FREE(x))
#define STBTT_assert(x)     do { IM_ASSERT(x); } while(0)
#define STBTT_fmod(x,y)     ImFmod(x,y)
#define STBTT_sqrt(x)       ImSqrt(x)
#define STBTT_pow(x,y)      ImPow(x,y)
#define STBTT_fabs(x)       ImFabs(x)
#define STBTT_ifloor(x)     ((int)ImFloor(x))
#define STBTT_iceil(x)      ((int)ImCeil(x))
#define STBTT_strlen(x)     ImStrlen(x)
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#else
#define STBTT_DEF extern
#endif
#ifdef IMGUI_STB_TRUETYPE_FILENAME
#include IMGUI_STB_TRUETYPE_FILENAME
#else
#include "imstb_truetype.h"
#endif
#endif
#endif // IMGUI_ENABLE_STB_TRUETYPE

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#ifdef IMGUI_STB_NAMESPACE
} // namespace ImStb
using namespace IMGUI_STB_NAMESPACE;
#endif

//-----------------------------------------------------------------------------
// [SECTION] Style functions
//-----------------------------------------------------------------------------

void ImGui::StyleColorsDark(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_InputTextCursor]        = colors[ImGuiCol_Text];
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabSelected]            = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline]    = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed]              = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected]      = ImLerp(colors[ImGuiCol_TabSelected],  colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink]               = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_TreeLines]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_UnsavedMarker]          = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_NavCursor]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void ImGui::StyleColorsClassic(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.85f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
    colors[ImGuiCol_Border]                 = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.27f, 0.27f, 0.54f, 0.83f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.40f, 0.61f, 0.62f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.40f, 0.48f, 0.71f, 0.79f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.46f, 0.54f, 0.80f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
    colors[ImGuiCol_Separator]              = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
    colors[ImGuiCol_InputTextCursor]        = colors[ImGuiCol_Text];
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabSelected]            = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline]    = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed]              = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected]      = ImLerp(colors[ImGuiCol_TabSelected],  colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.53f, 0.53f, 0.87f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    colors[ImGuiCol_TextLink]               = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    colors[ImGuiCol_TreeLines]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_UnsavedMarker]          = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_NavCursor]              = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

// Those light colors are better suited with a thicker font than the default one + FrameBorder
void ImGui::StyleColorsLight(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_InputTextCursor]        = colors[ImGuiCol_Text];
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.90f);
    colors[ImGuiCol_TabSelected]            = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline]    = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed]              = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected]      = ImLerp(colors[ImGuiCol_TabSelected],  colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.26f, 0.59f, 1.00f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
    colors[ImGuiCol_TextLink]               = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_TreeLines]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_UnsavedMarker]          = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavCursor]              = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawList
//-----------------------------------------------------------------------------

ImDrawListSharedData::ImDrawListSharedData()
{
    memset(this, 0, sizeof(*this));
    InitialFringeScale = 1.0f;
    for (int i = 0; i < IM_ARRAYSIZE(ArcFastVtx); i++)
    {
        const float a = ((float)i * 2 * IM_PI) / (float)IM_ARRAYSIZE(ArcFastVtx);
        ArcFastVtx[i] = ImVec2(ImCos(a), ImSin(a));
    }
    ArcFastRadiusCutoff = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC_R(IM_DRAWLIST_ARCFAST_SAMPLE_MAX, CircleSegmentMaxError);
}

ImDrawListSharedData::~ImDrawListSharedData()
{
    IM_ASSERT(DrawLists.Size == 0);
}

void ImDrawListSharedData::SetCircleTessellationMaxError(float max_error)
{
    if (CircleSegmentMaxError == max_error)
        return;

    IM_ASSERT(max_error > 0.0f);
    CircleSegmentMaxError = max_error;
    for (int i = 0; i < IM_ARRAYSIZE(CircleSegmentCounts); i++)
    {
        const float radius = (float)i;
        CircleSegmentCounts[i] = (ImU8)((i > 0) ? IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, CircleSegmentMaxError) : IM_DRAWLIST_ARCFAST_SAMPLE_MAX);
    }
    ArcFastRadiusCutoff = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC_R(IM_DRAWLIST_ARCFAST_SAMPLE_MAX, CircleSegmentMaxError);
}

ImDrawList::ImDrawList(ImDrawListSharedData* shared_data)
{
    memset(this, 0, sizeof(*this));
    _SetDrawListSharedData(shared_data);
}

ImDrawList::~ImDrawList()
{
    _ClearFreeMemory();
    _SetDrawListSharedData(NULL);
}

void ImDrawList::_SetDrawListSharedData(ImDrawListSharedData* data)
{
    if (_Data != NULL)
        _Data->DrawLists.find_erase_unsorted(this);
    _Data = data;
    if (_Data != NULL)
        _Data->DrawLists.push_back(this);
}

// Initialize before use in a new frame. We always have a command ready in the buffer.
// In the majority of cases, you would want to call PushClipRect() and PushTexture() after this.
void ImDrawList::_ResetForNewFrame()
{
    // Verify that the ImDrawCmd fields we want to memcmp() are contiguous in memory.
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, ClipRect) == 0);
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, TexRef) == sizeof(ImVec4));
    IM_STATIC_ASSERT(offsetof(ImDrawCmd, VtxOffset) == sizeof(ImVec4) + sizeof(ImTextureRef));
    if (_Splitter._Count > 1)
        _Splitter.Merge(this);

    CmdBuffer.resize(0);
    IdxBuffer.resize(0);
    VtxBuffer.resize(0);
    Flags = _Data->InitialFlags;
    memset(&_CmdHeader, 0, sizeof(_CmdHeader));
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.resize(0);
    _TextureStack.resize(0);
    _CallbacksDataBuf.resize(0);
    _Path.resize(0);
    _Splitter.Clear();
    CmdBuffer.push_back(ImDrawCmd());
    _FringeScale = _Data->InitialFringeScale;
}

void ImDrawList::_ClearFreeMemory()
{
    CmdBuffer.clear();
    IdxBuffer.clear();
    VtxBuffer.clear();
    Flags = ImDrawListFlags_None;
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.clear();
    _TextureStack.clear();
    _CallbacksDataBuf.clear();
    _Path.clear();
    _Splitter.ClearFreeMemory();
}

// Note: For multi-threaded rendering, consider using `imgui_threaded_rendering` from https://github.com/ocornut/imgui_club
ImDrawList* ImDrawList::CloneOutput() const
{
    ImDrawList* dst = IM_NEW(ImDrawList(NULL));
    dst->CmdBuffer = CmdBuffer;
    dst->IdxBuffer = IdxBuffer;
    dst->VtxBuffer = VtxBuffer;
    dst->Flags = Flags;
    return dst;
}

void ImDrawList::AddDrawCmd()
{
    ImDrawCmd draw_cmd;
    draw_cmd.ClipRect = _CmdHeader.ClipRect;    // Same as calling ImDrawCmd_HeaderCopy()
    draw_cmd.TexRef = _CmdHeader.TexRef;
    draw_cmd.VtxOffset = _CmdHeader.VtxOffset;
    draw_cmd.IdxOffset = IdxBuffer.Size;

    IM_ASSERT(draw_cmd.ClipRect.x <= draw_cmd.ClipRect.z && draw_cmd.ClipRect.y <= draw_cmd.ClipRect.w);
    CmdBuffer.push_back(draw_cmd);
}

// Pop trailing draw command (used before merging or presenting to user)
// Note that this leaves the ImDrawList in a state unfit for further commands, as most code assume that CmdBuffer.Size > 0 && CmdBuffer.back().UserCallback == NULL
void ImDrawList::_PopUnusedDrawCmd()
{
    while (CmdBuffer.Size > 0)
    {
        ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
        if (curr_cmd->ElemCount != 0 || curr_cmd->UserCallback != NULL)
            return;// break;
        CmdBuffer.pop_back();
    }
}

void ImDrawList::AddCallback(ImDrawCallback callback, void* userdata, size_t userdata_size)
{
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    IM_ASSERT(callback != NULL);
    IM_ASSERT(curr_cmd->UserCallback == NULL);
    if (curr_cmd->ElemCount != 0)
    {
        AddDrawCmd();
        curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    }

    curr_cmd->UserCallback = callback;
    if (userdata_size == 0)
    {
        // Store user data directly in command (no indirection)
        curr_cmd->UserCallbackData = userdata;
        curr_cmd->UserCallbackDataSize = 0;
        curr_cmd->UserCallbackDataOffset = -1;
    }
    else
    {
        // Copy and store user data in a buffer
        IM_ASSERT(userdata != NULL);
        IM_ASSERT(userdata_size < (1u << 31));
        curr_cmd->UserCallbackData = NULL; // Will be resolved during Render()
        curr_cmd->UserCallbackDataSize = (int)userdata_size;
        curr_cmd->UserCallbackDataOffset = _CallbacksDataBuf.Size;
        _CallbacksDataBuf.resize(_CallbacksDataBuf.Size + (int)userdata_size);
        memcpy(_CallbacksDataBuf.Data + (size_t)curr_cmd->UserCallbackDataOffset, userdata, userdata_size);
    }

    AddDrawCmd(); // Force a new command after us (see comment below)
}

// Compare ClipRect, TexRef and VtxOffset with a single memcmp()
#define ImDrawCmd_HeaderSize                            (offsetof(ImDrawCmd, VtxOffset) + sizeof(unsigned int))
#define ImDrawCmd_HeaderCompare(CMD_LHS, CMD_RHS)       (memcmp(CMD_LHS, CMD_RHS, ImDrawCmd_HeaderSize))    // Compare ClipRect, TexRef, VtxOffset
#define ImDrawCmd_HeaderCopy(CMD_DST, CMD_SRC)          (memcpy(CMD_DST, CMD_SRC, ImDrawCmd_HeaderSize))    // Copy ClipRect, TexRef, VtxOffset
#define ImDrawCmd_AreSequentialIdxOffset(CMD_0, CMD_1)  (CMD_0->IdxOffset + CMD_0->ElemCount == CMD_1->IdxOffset)

// Try to merge two last draw commands
void ImDrawList::_TryMergeDrawCmds()
{
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (ImDrawCmd_HeaderCompare(curr_cmd, prev_cmd) == 0 && ImDrawCmd_AreSequentialIdxOffset(prev_cmd, curr_cmd) && curr_cmd->UserCallback == NULL && prev_cmd->UserCallback == NULL)
    {
        prev_cmd->ElemCount += curr_cmd->ElemCount;
        CmdBuffer.pop_back();
    }
}

// Our scheme may appears a bit unusual, basically we want the most-common calls AddLine AddRect etc. to not have to perform any check so we always have a command ready in the stack.
// The cost of figuring out if a new command has to be added or if we can merge is paid in those Update** functions only.
void ImDrawList::_OnChangedClipRect()
{
    // If current command is used with different settings we need to add a new command
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount != 0 && memcmp(&curr_cmd->ClipRect, &_CmdHeader.ClipRect, sizeof(ImVec4)) != 0)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (curr_cmd->ElemCount == 0 && CmdBuffer.Size > 1 && ImDrawCmd_HeaderCompare(&_CmdHeader, prev_cmd) == 0 && ImDrawCmd_AreSequentialIdxOffset(prev_cmd, curr_cmd) && prev_cmd->UserCallback == NULL)
    {
        CmdBuffer.pop_back();
        return;
    }
    curr_cmd->ClipRect = _CmdHeader.ClipRect;
}

void ImDrawList::_OnChangedTexture()
{
    // If current command is used with different settings we need to add a new command
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount != 0 && curr_cmd->TexRef != _CmdHeader.TexRef)
    {
        AddDrawCmd();
        return;
    }

    // Unlike other _OnChangedXXX functions this may be called by ImFontAtlasUpdateDrawListsTextures() in more locations so we need to handle this case.
    if (curr_cmd->UserCallback != NULL)
        return;

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (curr_cmd->ElemCount == 0 && CmdBuffer.Size > 1 && ImDrawCmd_HeaderCompare(&_CmdHeader, prev_cmd) == 0 && ImDrawCmd_AreSequentialIdxOffset(prev_cmd, curr_cmd) && prev_cmd->UserCallback == NULL)
    {
        CmdBuffer.pop_back();
        return;
    }
    curr_cmd->TexRef = _CmdHeader.TexRef;
}

void ImDrawList::_OnChangedVtxOffset()
{
    // We don't need to compare curr_cmd->VtxOffset != _CmdHeader.VtxOffset because we know it'll be different at the time we call this.
    _VtxCurrentIdx = 0;
    IM_ASSERT_PARANOID(CmdBuffer.Size > 0);
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    //IM_ASSERT(curr_cmd->VtxOffset != _CmdHeader.VtxOffset); // See #3349
    if (curr_cmd->ElemCount != 0)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);
    curr_cmd->VtxOffset = _CmdHeader.VtxOffset;
}

int ImDrawList::_CalcCircleAutoSegmentCount(float radius) const
{
    // Automatic segment count
    const int radius_idx = (int)(radius + 0.999999f); // ceil to never reduce accuracy
    if (radius_idx >= 0 && radius_idx < IM_ARRAYSIZE(_Data->CircleSegmentCounts))
        return _Data->CircleSegmentCounts[radius_idx]; // Use cached value
    else
        return IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, _Data->CircleSegmentMaxError);
}

// Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
void ImDrawList::PushClipRect(const ImVec2& cr_min, const ImVec2& cr_max, bool intersect_with_current_clip_rect)
{
    ImVec4 cr(cr_min.x, cr_min.y, cr_max.x, cr_max.y);
    if (intersect_with_current_clip_rect)
    {
        ImVec4 current = _CmdHeader.ClipRect;
        if (cr.x < current.x) cr.x = current.x;
        if (cr.y < current.y) cr.y = current.y;
        if (cr.z > current.z) cr.z = current.z;
        if (cr.w > current.w) cr.w = current.w;
    }
    cr.z = ImMax(cr.x, cr.z);
    cr.w = ImMax(cr.y, cr.w);

    _ClipRectStack.push_back(cr);
    _CmdHeader.ClipRect = cr;
    _OnChangedClipRect();
}

void ImDrawList::PushClipRectFullScreen()
{
    PushClipRect(ImVec2(_Data->ClipRectFullscreen.x, _Data->ClipRectFullscreen.y), ImVec2(_Data->ClipRectFullscreen.z, _Data->ClipRectFullscreen.w));
}

void ImDrawList::PopClipRect()
{
    _ClipRectStack.pop_back();
    _CmdHeader.ClipRect = (_ClipRectStack.Size == 0) ? _Data->ClipRectFullscreen : _ClipRectStack.Data[_ClipRectStack.Size - 1];
    _OnChangedClipRect();
}

void ImDrawList::PushTexture(ImTextureRef tex_ref)
{
    _TextureStack.push_back(tex_ref);
    _CmdHeader.TexRef = tex_ref;
    if (tex_ref._TexData != NULL)
        IM_ASSERT(tex_ref._TexData->WantDestroyNextFrame == false);
    _OnChangedTexture();
}

void ImDrawList::PopTexture()
{
    _TextureStack.pop_back();
    _CmdHeader.TexRef = (_TextureStack.Size == 0) ? ImTextureRef() : _TextureStack.Data[_TextureStack.Size - 1];
    _OnChangedTexture();
}

// This is used by ImGui::PushFont()/PopFont(). It works because we never use _TextureIdStack[] elsewhere than in PushTexture()/PopTexture().
void ImDrawList::_SetTexture(ImTextureRef tex_ref)
{
    if (_CmdHeader.TexRef == tex_ref)
        return;
    _CmdHeader.TexRef = tex_ref;
    _TextureStack.back() = tex_ref;
    _OnChangedTexture();
}

// Reserve space for a number of vertices and indices.
// You must finish filling your reserved data before calling PrimReserve() again, as it may reallocate or
// submit the intermediate results. PrimUnreserve() can be used to release unused allocations.
void ImDrawList::PrimReserve(int idx_count, int vtx_count)
{
    // Large mesh support (when enabled)
    IM_ASSERT_PARANOID(idx_count >= 0 && vtx_count >= 0);
    if (sizeof(ImDrawIdx) == 2 && (_VtxCurrentIdx + vtx_count >= (1 << 16)) && (Flags & ImDrawListFlags_AllowVtxOffset))
    {
        // FIXME: In theory we should be testing that vtx_count <64k here.
        // In practice, RenderText() relies on reserving ahead for a worst case scenario so it is currently useful for us
        // to not make that check until we rework the text functions to handle clipping and large horizontal lines better.
        _CmdHeader.VtxOffset = VtxBuffer.Size;
        _OnChangedVtxOffset();
    }

    ImDrawCmd* draw_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd->ElemCount += idx_count;

    int vtx_buffer_old_size = VtxBuffer.Size;
    VtxBuffer.resize(vtx_buffer_old_size + vtx_count);
    _VtxWritePtr = VtxBuffer.Data + vtx_buffer_old_size;

    int idx_buffer_old_size = IdxBuffer.Size;
    IdxBuffer.resize(idx_buffer_old_size + idx_count);
    _IdxWritePtr = IdxBuffer.Data + idx_buffer_old_size;
}

// Release the number of reserved vertices/indices from the end of the last reservation made with PrimReserve().
void ImDrawList::PrimUnreserve(int idx_count, int vtx_count)
{
    IM_ASSERT_PARANOID(idx_count >= 0 && vtx_count >= 0);

    ImDrawCmd* draw_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd->ElemCount -= idx_count;
    VtxBuffer.shrink(VtxBuffer.Size - vtx_count);
    IdxBuffer.shrink(IdxBuffer.Size - idx_count);
}

// Fully unrolled with inline call to keep our debug builds decently fast.
void ImDrawList::PrimRect(const ImVec2& a, const ImVec2& c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv(_Data->TexUvWhitePixel);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimRectUV(const ImVec2& a, const ImVec2& c, const ImVec2& uv_a, const ImVec2& uv_c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimQuadUV(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, ImU32 col)
{
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

// On AddPolyline() and AddConvexPolyFilled() we intentionally avoid using ImVec2 and superfluous function calls to optimize debug/non-inlined builds.
// - Those macros expects l-values and need to be used as their own statement.
// - Those macros are intentionally not surrounded by the 'do {} while (0)' idiom because even that translates to runtime with debug compilers.
#define IM_NORMALIZE2F_OVER_ZERO(VX,VY)     { float d2 = VX*VX + VY*VY; if (d2 > 0.0f) { float inv_len = ImRsqrt(d2); VX *= inv_len; VY *= inv_len; } } (void)0
#define IM_FIXNORMAL2F_MAX_INVLEN2          100.0f // 500.0f (see #4053, #3366)
#define IM_FIXNORMAL2F(VX,VY)               { float d2 = VX*VX + VY*VY; if (d2 > 0.000001f) { float inv_len2 = 1.0f / d2; if (inv_len2 > IM_FIXNORMAL2F_MAX_INVLEN2) inv_len2 = IM_FIXNORMAL2F_MAX_INVLEN2; VX *= inv_len2; VY *= inv_len2; } } (void)0

// TODO: Thickness anti-aliased lines cap are missing their AA fringe.
// We avoid using the ImVec2 math operators here to reduce cost to a minimum for debug/non-inlined builds.
void ImDrawList::AddPolyline(const ImVec2* points, const int points_count, ImU32 col, ImDrawFlags flags, float thickness)
{
    if (points_count < 2 || (col & IM_COL32_A_MASK) == 0)
        return;

    const bool closed = (flags & ImDrawFlags_Closed) != 0;
    const ImVec2 opaque_uv = _Data->TexUvWhitePixel;
    const int count = closed ? points_count : points_count - 1; // The number of line segments we need to draw
    const bool thick_line = (thickness > _FringeScale);

    if (Flags & ImDrawListFlags_AntiAliasedLines)
    {
        // Anti-aliased stroke
        const float AA_SIZE = _FringeScale;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;

        // Thicknesses <1.0 should behave like thickness 1.0
        thickness = ImMax(thickness, 1.0f);
        const int integer_thickness = (int)thickness;
        const float fractional_thickness = thickness - integer_thickness;

        // Do we want to draw this line using a texture?
        // - For now, only draw integer-width lines using textures to avoid issues with the way scaling occurs, could be improved.
        // - If AA_SIZE is not 1.0f we cannot use the texture path.
        const bool use_texture = (Flags & ImDrawListFlags_AntiAliasedLinesUseTex) && (integer_thickness < IM_DRAWLIST_TEX_LINES_WIDTH_MAX) && (fractional_thickness <= 0.00001f) && (AA_SIZE == 1.0f);

        // We should never hit this, because NewFrame() doesn't set ImDrawListFlags_AntiAliasedLinesUseTex unless ImFontAtlasFlags_NoBakedLines is off
        IM_ASSERT_PARANOID(!use_texture || !(_Data->Font->ContainerAtlas->Flags & ImFontAtlasFlags_NoBakedLines));

        const int idx_count = use_texture ? (count * 6) : (thick_line ? count * 18 : count * 12);
        const int vtx_count = use_texture ? (points_count * 2) : (thick_line ? points_count * 4 : points_count * 3);
        PrimReserve(idx_count, vtx_count);

        // Temporary buffer
        // The first <points_count> items are normals at each line point, then after that there are either 2 or 4 temp points for each line point
        _Data->TempBuffer.reserve_discard(points_count * ((use_texture || !thick_line) ? 3 : 5));
        ImVec2* temp_normals = _Data->TempBuffer.Data;
        ImVec2* temp_points = temp_normals + points_count;

        // Calculate normals (tangents) for each line segment
        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
            float dx = points[i2].x - points[i1].x;
            float dy = points[i2].y - points[i1].y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i1].x = dy;
            temp_normals[i1].y = -dx;
        }
        if (!closed)
            temp_normals[points_count - 1] = temp_normals[points_count - 2];

        // If we are drawing a one-pixel-wide line without a texture, or a textured line of any width, we only need 2 or 3 vertices per point
        if (use_texture || !thick_line)
        {
            // [PATH 1] Texture-based lines (thick or non-thick)
            // [PATH 2] Non texture-based lines (non-thick)

            // The width of the geometry we need to draw - this is essentially <thickness> pixels for the line itself, plus "one pixel" for AA.
            // - In the texture-based path, we don't use AA_SIZE here because the +1 is tied to the generated texture
            //   (see ImFontAtlasBuildRenderLinesTexData() function), and so alternate values won't work without changes to that code.
            // - In the non texture-based paths, we would allow AA_SIZE to potentially be != 1.0f with a patch (e.g. fringe_scale patch to
            //   allow scaling geometry while preserving one-screen-pixel AA fringe).
            const float half_draw_size = use_texture ? ((thickness * 0.5f) + 1) : AA_SIZE;

            // If line is not closed, the first and last points need to be generated differently as there are no normals to blend
            if (!closed)
            {
                temp_points[0] = points[0] + temp_normals[0] * half_draw_size;
                temp_points[1] = points[0] - temp_normals[0] * half_draw_size;
                temp_points[(points_count-1)*2+0] = points[points_count-1] + temp_normals[points_count-1] * half_draw_size;
                temp_points[(points_count-1)*2+1] = points[points_count-1] - temp_normals[points_count-1] * half_draw_size;
            }

            // Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
            // This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx; // Vertex index for start of line segment
            for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
            {
                const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1; // i2 is the second point of the line segment
                const unsigned int idx2 = ((i1 + 1) == points_count) ? _VtxCurrentIdx : (idx1 + (use_texture ? 2 : 3)); // Vertex index for end of segment

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F(dm_x, dm_y);
                dm_x *= half_draw_size; // dm_x, dm_y are offset to the outer edge of the AA area
                dm_y *= half_draw_size;

                // Add temporary vertices for the outer edges
                ImVec2* out_vtx = &temp_points[i2 * 2];
                out_vtx[0].x = points[i2].x + dm_x;
                out_vtx[0].y = points[i2].y + dm_y;
                out_vtx[1].x = points[i2].x - dm_x;
                out_vtx[1].y = points[i2].y - dm_y;

                if (use_texture)
                {
                    // Add indices for two triangles
                    _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 1); // Right tri
                    _IdxWritePtr[3] = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[4] = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 0); // Left tri
                    _IdxWritePtr += 6;
                }
                else
                {
                    // Add indexes for four triangles
                    _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 2); // Right tri 1
                    _IdxWritePtr[3] = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[4] = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 0); // Right tri 2
                    _IdxWritePtr[6] = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[7] = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[8] = (ImDrawIdx)(idx1 + 0); // Left tri 1
                    _IdxWritePtr[9] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1); // Left tri 2
                    _IdxWritePtr += 12;
                }

                idx1 = idx2;
            }

            // Add vertices for each point on the line
            if (use_texture)
            {
                // If we're using textures we only need to emit the left/right edge vertices
                ImVec4 tex_uvs = _Data->TexUvLines[integer_thickness];
                /*if (fractional_thickness != 0.0f) // Currently always zero when use_texture==false!
                {
                    const ImVec4 tex_uvs_1 = _Data->TexUvLines[integer_thickness + 1];
                    tex_uvs.x = tex_uvs.x + (tex_uvs_1.x - tex_uvs.x) * fractional_thickness; // inlined ImLerp()
                    tex_uvs.y = tex_uvs.y + (tex_uvs_1.y - tex_uvs.y) * fractional_thickness;
                    tex_uvs.z = tex_uvs.z + (tex_uvs_1.z - tex_uvs.z) * fractional_thickness;
                    tex_uvs.w = tex_uvs.w + (tex_uvs_1.w - tex_uvs.w) * fractional_thickness;
                }*/
                ImVec2 tex_uv0(tex_uvs.x, tex_uvs.y);
                ImVec2 tex_uv1(tex_uvs.z, tex_uvs.w);
                for (int i = 0; i < points_count; i++)
                {
                    _VtxWritePtr[0].pos = temp_points[i * 2 + 0]; _VtxWritePtr[0].uv = tex_uv0; _VtxWritePtr[0].col = col; // Left-side outer edge
                    _VtxWritePtr[1].pos = temp_points[i * 2 + 1]; _VtxWritePtr[1].uv = tex_uv1; _VtxWritePtr[1].col = col; // Right-side outer edge
                    _VtxWritePtr += 2;
                }
            }
            else
            {
                // If we're not using a texture, we need the center vertex as well
                for (int i = 0; i < points_count; i++)
                {
                    _VtxWritePtr[0].pos = points[i];              _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;       // Center of line
                    _VtxWritePtr[1].pos = temp_points[i * 2 + 0]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col_trans; // Left-side outer edge
                    _VtxWritePtr[2].pos = temp_points[i * 2 + 1]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col_trans; // Right-side outer edge
                    _VtxWritePtr += 3;
                }
            }
        }
        else
        {
            // [PATH 2] Non texture-based lines (thick): we need to draw the solid line core and thus require four vertices per point
            const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;

            // If line is not closed, the first and last points need to be generated differently as there are no normals to blend
            if (!closed)
            {
                const int points_last = points_count - 1;
                temp_points[0] = points[0] + temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[1] = points[0] + temp_normals[0] * (half_inner_thickness);
                temp_points[2] = points[0] - temp_normals[0] * (half_inner_thickness);
                temp_points[3] = points[0] - temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[points_last * 4 + 0] = points[points_last] + temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
                temp_points[points_last * 4 + 1] = points[points_last] + temp_normals[points_last] * (half_inner_thickness);
                temp_points[points_last * 4 + 2] = points[points_last] - temp_normals[points_last] * (half_inner_thickness);
                temp_points[points_last * 4 + 3] = points[points_last] - temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
            }

            // Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
            // This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx; // Vertex index for start of line segment
            for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
            {
                const int i2 = (i1 + 1) == points_count ? 0 : (i1 + 1); // i2 is the second point of the line segment
                const unsigned int idx2 = (i1 + 1) == points_count ? _VtxCurrentIdx : (idx1 + 4); // Vertex index for end of segment

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F(dm_x, dm_y);
                float dm_out_x = dm_x * (half_inner_thickness + AA_SIZE);
                float dm_out_y = dm_y * (half_inner_thickness + AA_SIZE);
                float dm_in_x = dm_x * half_inner_thickness;
                float dm_in_y = dm_y * half_inner_thickness;

                // Add temporary vertices
                ImVec2* out_vtx = &temp_points[i2 * 4];
                out_vtx[0].x = points[i2].x + dm_out_x;
                out_vtx[0].y = points[i2].y + dm_out_y;
                out_vtx[1].x = points[i2].x + dm_in_x;
                out_vtx[1].y = points[i2].y + dm_in_y;
                out_vtx[2].x = points[i2].x - dm_in_x;
                out_vtx[2].y = points[i2].y - dm_in_y;
                out_vtx[3].x = points[i2].x - dm_out_x;
                out_vtx[3].y = points[i2].y - dm_out_y;

                // Add indexes
                _IdxWritePtr[0]  = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[1]  = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[2]  = (ImDrawIdx)(idx1 + 2);
                _IdxWritePtr[3]  = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[4]  = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[5]  = (ImDrawIdx)(idx2 + 1);
                _IdxWritePtr[6]  = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[7]  = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[8]  = (ImDrawIdx)(idx1 + 0);
                _IdxWritePtr[9]  = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1);
                _IdxWritePtr[12] = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[13] = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[14] = (ImDrawIdx)(idx1 + 3);
                _IdxWritePtr[15] = (ImDrawIdx)(idx1 + 3); _IdxWritePtr[16] = (ImDrawIdx)(idx2 + 3); _IdxWritePtr[17] = (ImDrawIdx)(idx2 + 2);
                _IdxWritePtr += 18;

                idx1 = idx2;
            }

            // Add vertices
            for (int i = 0; i < points_count; i++)
            {
                _VtxWritePtr[0].pos = temp_points[i * 4 + 0]; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col_trans;
                _VtxWritePtr[1].pos = temp_points[i * 4 + 1]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
                _VtxWritePtr[2].pos = temp_points[i * 4 + 2]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
                _VtxWritePtr[3].pos = temp_points[i * 4 + 3]; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col_trans;
                _VtxWritePtr += 4;
            }
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // [PATH 4] Non texture-based, Non anti-aliased lines
        const int idx_count = count * 6;
        const int vtx_count = count * 4;    // FIXME-OPT: Not sharing edges
        PrimReserve(idx_count, vtx_count);

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
            const ImVec2& p1 = points[i1];
            const ImVec2& p2 = points[i2];

            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            dx *= (thickness * 0.5f);
            dy *= (thickness * 0.5f);

            _VtxWritePtr[0].pos.x = p1.x + dy; _VtxWritePtr[0].pos.y = p1.y - dx; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr[1].pos.x = p2.x + dy; _VtxWritePtr[1].pos.y = p2.y - dx; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
            _VtxWritePtr[2].pos.x = p2.x - dy; _VtxWritePtr[2].pos.y = p2.y + dx; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
            _VtxWritePtr[3].pos.x = p1.x - dy; _VtxWritePtr[3].pos.y = p1.y + dx; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col;
            _VtxWritePtr += 4;

            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + 1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + 2);
            _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx + 2); _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx + 3);
            _IdxWritePtr += 6;
            _VtxCurrentIdx += 4;
        }
    }
}

// - We intentionally avoid using ImVec2 and its math operators here to reduce cost to a minimum for debug/non-inlined builds.
// - Filled shapes must always use clockwise winding order. The anti-aliasing fringe depends on it. Counter-clockwise shapes will have "inward" anti-aliasing.
void ImDrawList::AddConvexPolyFilled(const ImVec2* points, const int points_count, ImU32 col)
{
    if (points_count < 3 || (col & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;

    if (Flags & ImDrawListFlags_AntiAliasedFill)
    {
        // Anti-aliased Fill
        const float AA_SIZE = _FringeScale;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;
        const int idx_count = (points_count - 2)*3 + points_count * 6;
        const int vtx_count = (points_count * 2);
        PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx + 1;
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + ((i - 1) << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx + (i << 1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        _Data->TempBuffer.reserve_discard(points_count);
        ImVec2* temp_normals = _Data->TempBuffer.Data;
        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            float dx = p1.x - p0.x;
            float dy = p1.y - p0.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i0].x = dy;
            temp_normals[i0].y = -dx;
        }

        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            float dm_x = (n0.x + n1.x) * 0.5f;
            float dm_y = (n0.y + n1.y) * 0.5f;
            IM_FIXNORMAL2F(dm_x, dm_y);
            dm_x *= AA_SIZE * 0.5f;
            dm_y *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos.x = (points[i1].x - dm_x); _VtxWritePtr[0].pos.y = (points[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos.x = (points[i1].x + dm_x); _VtxWritePtr[1].pos.y = (points[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (i0 << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx + (i1 << 1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count - 2)*3;
        const int vtx_count = points_count;
        PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + i - 1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + i);
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

void ImDrawList::_PathArcToFastEx(const ImVec2& center, float radius, int a_min_sample, int a_max_sample, int a_step)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }

    // Calculate arc auto segment step size
    if (a_step <= 0)
        a_step = IM_DRAWLIST_ARCFAST_SAMPLE_MAX / _CalcCircleAutoSegmentCount(radius);

    // Make sure we never do steps larger than one quarter of the circle
    a_step = ImClamp(a_step, 1, IM_DRAWLIST_ARCFAST_TABLE_SIZE / 4);

    const int sample_range = ImAbs(a_max_sample - a_min_sample);
    const int a_next_step = a_step;

    int samples = sample_range + 1;
    bool extra_max_sample = false;
    if (a_step > 1)
    {
        samples            = sample_range / a_step + 1;
        const int overstep = sample_range % a_step;

        if (overstep > 0)
        {
            extra_max_sample = true;
            samples++;

            // When we have overstep to avoid awkwardly looking one long line and one tiny one at the end,
            // distribute first step range evenly between them by reducing first step size.
            if (sample_range > 0)
                a_step -= (a_step - overstep) / 2;
        }
    }

    _Path.resize(_Path.Size + samples);
    ImVec2* out_ptr = _Path.Data + (_Path.Size - samples);

    int sample_index = a_min_sample;
    if (sample_index < 0 || sample_index >= IM_DRAWLIST_ARCFAST_SAMPLE_MAX)
    {
        sample_index = sample_index % IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        if (sample_index < 0)
            sample_index += IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
    }

    if (a_max_sample >= a_min_sample)
    {
        for (int a = a_min_sample; a <= a_max_sample; a += a_step, sample_index += a_step, a_step = a_next_step)
        {
            // a_step is clamped to IM_DRAWLIST_ARCFAST_SAMPLE_MAX, so we have guaranteed that it will not wrap over range twice or more
            if (sample_index >= IM_DRAWLIST_ARCFAST_SAMPLE_MAX)
                sample_index -= IM_DRAWLIST_ARCFAST_SAMPLE_MAX;

            const ImVec2 s = _Data->ArcFastVtx[sample_index];
            out_ptr->x = center.x + s.x * radius;
            out_ptr->y = center.y + s.y * radius;
            out_ptr++;
        }
    }
    else
    {
        for (int a = a_min_sample; a >= a_max_sample; a -= a_step, sample_index -= a_step, a_step = a_next_step)
        {
            // a_step is clamped to IM_DRAWLIST_ARCFAST_SAMPLE_MAX, so we have guaranteed that it will not wrap over range twice or more
            if (sample_index < 0)
                sample_index += IM_DRAWLIST_ARCFAST_SAMPLE_MAX;

            const ImVec2 s = _Data->ArcFastVtx[sample_index];
            out_ptr->x = center.x + s.x * radius;
            out_ptr->y = center.y + s.y * radius;
            out_ptr++;
        }
    }

    if (extra_max_sample)
    {
        int normalized_max_sample = a_max_sample % IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        if (normalized_max_sample < 0)
            normalized_max_sample += IM_DRAWLIST_ARCFAST_SAMPLE_MAX;

        const ImVec2 s = _Data->ArcFastVtx[normalized_max_sample];
        out_ptr->x = center.x + s.x * radius;
        out_ptr->y = center.y + s.y * radius;
        out_ptr++;
    }

    IM_ASSERT_PARANOID(_Path.Data + _Path.Size == out_ptr);
}

void ImDrawList::_PathArcToN(const ImVec2& center, float radius, float a_min, float a_max, int num_segments)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }

    // Note that we are adding a point at both a_min and a_max.
    // If you are trying to draw a full closed circle you don't want the overlapping points!
    _Path.reserve(_Path.Size + (num_segments + 1));
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        _Path.push_back(ImVec2(center.x + ImCos(a) * radius, center.y + ImSin(a) * radius));
    }
}

// 0: East, 3: South, 6: West, 9: North, 12: East
void ImDrawList::PathArcToFast(const ImVec2& center, float radius, int a_min_of_12, int a_max_of_12)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }
    _PathArcToFastEx(center, radius, a_min_of_12 * IM_DRAWLIST_ARCFAST_SAMPLE_MAX / 12, a_max_of_12 * IM_DRAWLIST_ARCFAST_SAMPLE_MAX / 12, 0);
}

void ImDrawList::PathArcTo(const ImVec2& center, float radius, float a_min, float a_max, int num_segments)
{
    if (radius < 0.5f)
    {
        _Path.push_back(center);
        return;
    }

    if (num_segments > 0)
    {
        _PathArcToN(center, radius, a_min, a_max, num_segments);
        return;
    }

    // Automatic segment count
    if (radius <= _Data->ArcFastRadiusCutoff)
    {
        const bool a_is_reverse = a_max < a_min;

        // We are going to use precomputed values for mid samples.
        // Determine first and last sample in lookup table that belong to the arc.
        const float a_min_sample_f = IM_DRAWLIST_ARCFAST_SAMPLE_MAX * a_min / (IM_PI * 2.0f);
        const float a_max_sample_f = IM_DRAWLIST_ARCFAST_SAMPLE_MAX * a_max / (IM_PI * 2.0f);

        const int a_min_sample = a_is_reverse ? (int)ImFloor(a_min_sample_f) : (int)ImCeil(a_min_sample_f);
        const int a_max_sample = a_is_reverse ? (int)ImCeil(a_max_sample_f) : (int)ImFloor(a_max_sample_f);
        const int a_mid_samples = a_is_reverse ? ImMax(a_min_sample - a_max_sample, 0) : ImMax(a_max_sample - a_min_sample, 0);

        const float a_min_segment_angle = a_min_sample * IM_PI * 2.0f / IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        const float a_max_segment_angle = a_max_sample * IM_PI * 2.0f / IM_DRAWLIST_ARCFAST_SAMPLE_MAX;
        const bool a_emit_start = ImAbs(a_min_segment_angle - a_min) >= 1e-5f;
        const bool a_emit_end = ImAbs(a_max - a_max_segment_angle) >= 1e-5f;

        _Path.reserve(_Path.Size + (a_mid_samples + 1 + (a_emit_start ? 1 : 0) + (a_emit_end ? 1 : 0)));
        if (a_emit_start)
            _Path.push_back(ImVec2(center.x + ImCos(a_min) * radius, center.y + ImSin(a_min) * radius));
        if (a_mid_samples > 0)
            _PathArcToFastEx(center, radius, a_min_sample, a_max_sample, 0);
        if (a_emit_end)
            _Path.push_back(ImVec2(center.x + ImCos(a_max) * radius, center.y + ImSin(a_max) * radius));
    }
    else
    {
        const float arc_length = ImAbs(a_max - a_min);
        const int circle_segment_count = _CalcCircleAutoSegmentCount(radius);
        const int arc_segment_count = ImMax((int)ImCeil(circle_segment_count * arc_length / (IM_PI * 2.0f)), (int)(2.0f * IM_PI / arc_length));
        _PathArcToN(center, radius, a_min, a_max, arc_segment_count);
    }
}

void ImDrawList::PathEllipticalArcTo(const ImVec2& center, const ImVec2& radius, float rot, float a_min, float a_max, int num_segments)
{
    if (num_segments <= 0)
        num_segments = _CalcCircleAutoSegmentCount(ImMax(radius.x, radius.y)); // A bit pessimistic, maybe there's a better computation to do here.

    _Path.reserve(_Path.Size + (num_segments + 1));

    const float cos_rot = ImCos(rot);
    const float sin_rot = ImSin(rot);
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        ImVec2 point(ImCos(a) * radius.x, ImSin(a) * radius.y);
        const ImVec2 rel((point.x * cos_rot) - (point.y * sin_rot), (point.x * sin_rot) + (point.y * cos_rot));
        point.x = rel.x + center.x;
        point.y = rel.y + center.y;
        _Path.push_back(point);
    }
}

ImVec2 ImBezierCubicCalc(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, float t)
{
    float u = 1.0f - t;
    float w1 = u * u * u;
    float w2 = 3 * u * u * t;
    float w3 = 3 * u * t * t;
    float w4 = t * t * t;
    return ImVec2(w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x, w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y);
}

ImVec2 ImBezierQuadraticCalc(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, float t)
{
    float u = 1.0f - t;
    float w1 = u * u;
    float w2 = 2 * u * t;
    float w3 = t * t;
    return ImVec2(w1 * p1.x + w2 * p2.x + w3 * p3.x, w1 * p1.y + w2 * p2.y + w3 * p3.y);
}

// Closely mimics ImBezierCubicClosestPointCasteljau() in imgui.cpp
static void PathBezierCubicCurveToCasteljau(ImVector<ImVec2>* path, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level)
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = (x2 - x4) * dy - (y2 - y4) * dx;
    float d3 = (x3 - x4) * dy - (y3 - y4) * dx;
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy))
    {
        path->push_back(ImVec2(x4, y4));
    }
    else if (level < 10)
    {
        float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
        float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
        float x34 = (x3 + x4) * 0.5f, y34 = (y3 + y4) * 0.5f;
        float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
        float x234 = (x23 + x34) * 0.5f, y234 = (y23 + y34) * 0.5f;
        float x1234 = (x123 + x234) * 0.5f, y1234 = (y123 + y234) * 0.5f;
        PathBezierCubicCurveToCasteljau(path, x1, y1, x12, y12, x123, y123, x1234, y1234, tess_tol, level + 1);
        PathBezierCubicCurveToCasteljau(path, x1234, y1234, x234, y234, x34, y34, x4, y4, tess_tol, level + 1);
    }
}

static void PathBezierQuadraticCurveToCasteljau(ImVector<ImVec2>* path, float x1, float y1, float x2, float y2, float x3, float y3, float tess_tol, int level)
{
    float dx = x3 - x1, dy = y3 - y1;
    float det = (x2 - x3) * dy - (y2 - y3) * dx;
    if (det * det * 4.0f < tess_tol * (dx * dx + dy * dy))
    {
        path->push_back(ImVec2(x3, y3));
    }
    else if (level < 10)
    {
        float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
        float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
        float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
        PathBezierQuadraticCurveToCasteljau(path, x1, y1, x12, y12, x123, y123, tess_tol, level + 1);
        PathBezierQuadraticCurveToCasteljau(path, x123, y123, x23, y23, x3, y3, tess_tol, level + 1);
    }
}

void ImDrawList::PathBezierCubicCurveTo(const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, int num_segments)
{
    ImVec2 p1 = _Path.back();
    if (num_segments == 0)
    {
        IM_ASSERT(_Data->CurveTessellationTol > 0.0f);
        PathBezierCubicCurveToCasteljau(&_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, _Data->CurveTessellationTol, 0); // Auto-tessellated
    }
    else
    {
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step++)
            _Path.push_back(ImBezierCubicCalc(p1, p2, p3, p4, t_step * i_step));
    }
}

void ImDrawList::PathBezierQuadraticCurveTo(const ImVec2& p2, const ImVec2& p3, int num_segments)
{
    ImVec2 p1 = _Path.back();
    if (num_segments == 0)
    {
        IM_ASSERT(_Data->CurveTessellationTol > 0.0f);
        PathBezierQuadraticCurveToCasteljau(&_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, _Data->CurveTessellationTol, 0);// Auto-tessellated
    }
    else
    {
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step++)
            _Path.push_back(ImBezierQuadraticCalc(p1, p2, p3, t_step * i_step));
    }
}

static inline ImDrawFlags FixRectCornerFlags(ImDrawFlags flags)
{
    /*
    IM_STATIC_ASSERT(ImDrawFlags_RoundCornersTopLeft == (1 << 4));
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    // Obsoleted in 1.82 (from February 2021). This code was stripped/simplified and mostly commented in 1.90 (from September 2023)
    // - Legacy Support for hard coded ~0 (used to be a suggested equivalent to ImDrawCornerFlags_All)
    if (flags == ~0)                    { return ImDrawFlags_RoundCornersAll; }
    // - Legacy Support for hard coded 0x01 to 0x0F (matching 15 out of 16 old flags combinations). Read details in older version of this code.
    if (flags >= 0x01 && flags <= 0x0F) { return (flags << 4); }
    // We cannot support hard coded 0x00 with 'float rounding > 0.0f' --> replace with ImDrawFlags_RoundCornersNone or use 'float rounding = 0.0f'
#endif
    */
    // If this assert triggers, please update your code replacing hardcoded values with new ImDrawFlags_RoundCorners* values.
    // Note that ImDrawFlags_Closed (== 0x01) is an invalid flag for AddRect(), AddRectFilled(), PathRect() etc. anyway.
    // See details in 1.82 Changelog as well as 2021/03/12 and 2023/09/08 entries in "API BREAKING CHANGES" section.
    IM_ASSERT((flags & 0x0F) == 0 && "Misuse of legacy hardcoded ImDrawCornerFlags values!");

    if ((flags & ImDrawFlags_RoundCornersMask_) == 0)
        flags |= ImDrawFlags_RoundCornersAll;

    return flags;
}

void ImDrawList::PathRect(const ImVec2& a, const ImVec2& b, float rounding, ImDrawFlags flags)
{
    if (rounding >= 0.5f)
    {
        flags = FixRectCornerFlags(flags);
        rounding = ImMin(rounding, ImFabs(b.x - a.x) * (((flags & ImDrawFlags_RoundCornersTop) == ImDrawFlags_RoundCornersTop) || ((flags & ImDrawFlags_RoundCornersBottom) == ImDrawFlags_RoundCornersBottom) ? 0.5f : 1.0f) - 1.0f);
        rounding = ImMin(rounding, ImFabs(b.y - a.y) * (((flags & ImDrawFlags_RoundCornersLeft) == ImDrawFlags_RoundCornersLeft) || ((flags & ImDrawFlags_RoundCornersRight) == ImDrawFlags_RoundCornersRight) ? 0.5f : 1.0f) - 1.0f);
    }
    if (rounding < 0.5f || (flags & ImDrawFlags_RoundCornersMask_) == ImDrawFlags_RoundCornersNone)
    {
        PathLineTo(a);
        PathLineTo(ImVec2(b.x, a.y));
        PathLineTo(b);
        PathLineTo(ImVec2(a.x, b.y));
    }
    else
    {
        const float rounding_tl = (flags & ImDrawFlags_RoundCornersTopLeft)     ? rounding : 0.0f;
        const float rounding_tr = (flags & ImDrawFlags_RoundCornersTopRight)    ? rounding : 0.0f;
        const float rounding_br = (flags & ImDrawFlags_RoundCornersBottomRight) ? rounding : 0.0f;
        const float rounding_bl = (flags & ImDrawFlags_RoundCornersBottomLeft)  ? rounding : 0.0f;
        PathArcToFast(ImVec2(a.x + rounding_tl, a.y + rounding_tl), rounding_tl, 6, 9);
        PathArcToFast(ImVec2(b.x - rounding_tr, a.y + rounding_tr), rounding_tr, 9, 12);
        PathArcToFast(ImVec2(b.x - rounding_br, b.y - rounding_br), rounding_br, 0, 3);
        PathArcToFast(ImVec2(a.x + rounding_bl, b.y - rounding_bl), rounding_bl, 3, 6);
    }
}

void ImDrawList::AddLine(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    PathLineTo(p1 + ImVec2(0.5f, 0.5f));
    PathLineTo(p2 + ImVec2(0.5f, 0.5f));
    PathStroke(col, 0, thickness);
}

// p_min = upper-left, p_max = lower-right
// Note we don't render 1 pixels sized rectangles properly.
void ImDrawList::AddRect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags flags, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (Flags & ImDrawListFlags_AntiAliasedLines)
        PathRect(p_min + ImVec2(0.50f, 0.50f), p_max - ImVec2(0.50f, 0.50f), rounding, flags);
    else
        PathRect(p_min + ImVec2(0.50f, 0.50f), p_max - ImVec2(0.49f, 0.49f), rounding, flags); // Better looking lower-right corner and rounded non-AA shapes.
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddRectFilled(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags flags)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (rounding < 0.5f || (flags & ImDrawFlags_RoundCornersMask_) == ImDrawFlags_RoundCornersNone)
    {
        PrimReserve(6, 4);
        PrimRect(p_min, p_max, col);
    }
    else
    {
        PathRect(p_min, p_max, rounding, flags);
        PathFillConvex(col);
    }
}

// p_min = upper-left, p_max = lower-right
void ImDrawList::AddRectFilledMultiColor(const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left)
{
    if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;
    PrimReserve(6, 4);
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 1)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2));
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 3));
    PrimWriteVtx(p_min, uv, col_upr_left);
    PrimWriteVtx(ImVec2(p_max.x, p_min.y), uv, col_upr_right);
    PrimWriteVtx(p_max, uv, col_bot_right);
    PrimWriteVtx(ImVec2(p_min.x, p_max.y), uv, col_bot_left);
}

void ImDrawList::AddQuad(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathLineTo(p4);
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddQuadFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathLineTo(p4);
    PathFillConvex(col);
}

void ImDrawList::AddTriangle(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddTriangleFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathFillConvex(col);
}

void ImDrawList::AddCircle(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || radius < 0.5f)
        return;

    if (num_segments <= 0)
    {
        // Use arc with automatic segment count
        _PathArcToFastEx(center, radius - 0.5f, 0, IM_DRAWLIST_ARCFAST_SAMPLE_MAX, 0);
        _Path.Size--;
    }
    else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp(num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX);

        // Because we are filling a closed shape we remove 1 from the count of segments/points
        const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
        PathArcTo(center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    }

    PathStroke(col, ImDrawFlags_Closed, thickness);
}

void ImDrawList::AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || radius < 0.5f)
        return;

    if (num_segments <= 0)
    {
        // Use arc with automatic segment count
        _PathArcToFastEx(center, radius, 0, IM_DRAWLIST_ARCFAST_SAMPLE_MAX, 0);
        _Path.Size--;
    }
    else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp(num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX);

        // Because we are filling a closed shape we remove 1 from the count of segments/points
        const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
        PathArcTo(center, radius, 0.0f, a_max, num_segments - 1);
    }

    PathFillConvex(col);
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgon(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    PathStroke(col, ImDrawFlags_Closed, thickness);
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgonFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(center, radius, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

// Ellipse
void ImDrawList::AddEllipse(const ImVec2& center, const ImVec2& radius, ImU32 col, float rot, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (num_segments <= 0)
        num_segments = _CalcCircleAutoSegmentCount(ImMax(radius.x, radius.y)); // A bit pessimistic, maybe there's a better computation to do here.

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathEllipticalArcTo(center, radius, rot, 0.0f, a_max, num_segments - 1);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddEllipseFilled(const ImVec2& center, const ImVec2& radius, ImU32 col, float rot, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (num_segments <= 0)
        num_segments = _CalcCircleAutoSegmentCount(ImMax(radius.x, radius.y)); // A bit pessimistic, maybe there's a better computation to do here.

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathEllipticalArcTo(center, radius, rot, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

// Cubic Bezier takes 4 controls points
void ImDrawList::AddBezierCubic(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathBezierCubicCurveTo(p2, p3, p4, num_segments);
    PathStroke(col, 0, thickness);
}

// Quadratic Bezier takes 3 controls points
void ImDrawList::AddBezierQuadratic(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathBezierQuadraticCurveTo(p2, p3, num_segments);
    PathStroke(col, 0, thickness);
}

void ImDrawList::AddText(ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width, const ImVec4* cpu_fine_clip_rect)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    // Accept null ranges
    if (text_begin == text_end || text_begin[0] == 0)
        return;
    // No need to strlen() here: font->RenderText() will do it and may early out.

    // Pull default font/size from the shared ImDrawListSharedData instance
    if (font == NULL)
        font = _Data->Font;
    if (font_size == 0.0f)
        font_size = _Data->FontSize;

    ImVec4 clip_rect = _CmdHeader.ClipRect;
    if (cpu_fine_clip_rect)
    {
        clip_rect.x = ImMax(clip_rect.x, cpu_fine_clip_rect->x);
        clip_rect.y = ImMax(clip_rect.y, cpu_fine_clip_rect->y);
        clip_rect.z = ImMin(clip_rect.z, cpu_fine_clip_rect->z);
        clip_rect.w = ImMin(clip_rect.w, cpu_fine_clip_rect->w);
    }
    font->RenderText(this, font_size, pos, col, clip_rect, text_begin, text_end, wrap_width, (cpu_fine_clip_rect != NULL) ? ImDrawTextFlags_CpuFineClip : ImDrawTextFlags_None);
}

void ImDrawList::AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end)
{
    AddText(_Data->Font, _Data->FontSize, pos, col, text_begin, text_end);
}

void ImDrawList::AddImage(ImTextureRef tex_ref, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = tex_ref != _CmdHeader.TexRef;
    if (push_texture_id)
        PushTexture(tex_ref);

    PrimReserve(6, 4);
    PrimRectUV(p_min, p_max, uv_min, uv_max, col);

    if (push_texture_id)
        PopTexture();
}

void ImDrawList::AddImageQuad(ImTextureRef tex_ref, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& uv1, const ImVec2& uv2, const ImVec2& uv3, const ImVec2& uv4, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = tex_ref != _CmdHeader.TexRef;
    if (push_texture_id)
        PushTexture(tex_ref);

    PrimReserve(6, 4);
    PrimQuadUV(p1, p2, p3, p4, uv1, uv2, uv3, uv4, col);

    if (push_texture_id)
        PopTexture();
}

void ImDrawList::AddImageRounded(ImTextureRef tex_ref, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, float rounding, ImDrawFlags flags)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    flags = FixRectCornerFlags(flags);
    if (rounding < 0.5f || (flags & ImDrawFlags_RoundCornersMask_) == ImDrawFlags_RoundCornersNone)
    {
        AddImage(tex_ref, p_min, p_max, uv_min, uv_max, col);
        return;
    }

    const bool push_texture_id = tex_ref != _CmdHeader.TexRef;
    if (push_texture_id)
        PushTexture(tex_ref);

    int vert_start_idx = VtxBuffer.Size;
    PathRect(p_min, p_max, rounding, flags);
    PathFillConvex(col);
    int vert_end_idx = VtxBuffer.Size;
    ImGui::ShadeVertsLinearUV(this, vert_start_idx, vert_end_idx, p_min, p_max, uv_min, uv_max, true);

    if (push_texture_id)
        PopTexture();
}

//-----------------------------------------------------------------------------
// [SECTION] ImTriangulator, ImDrawList concave polygon fill
//-----------------------------------------------------------------------------
// Triangulate concave polygons. Based on "Triangulation by Ear Clipping" paper, O(N^2) complexity.
// Reference: https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
// Provided as a convenience for user but not used by main library.
//-----------------------------------------------------------------------------
// - ImTriangulator [Internal]
// - AddConcavePolyFilled()
//-----------------------------------------------------------------------------

enum ImTriangulatorNodeType
{
    ImTriangulatorNodeType_Convex,
    ImTriangulatorNodeType_Ear,
    ImTriangulatorNodeType_Reflex
};

struct ImTriangulatorNode
{
    ImTriangulatorNodeType  Type;
    int                     Index;
    ImVec2                  Pos;
    ImTriangulatorNode*     Next;
    ImTriangulatorNode*     Prev;

    void    Unlink()        { Next->Prev = Prev; Prev->Next = Next; }
};

struct ImTriangulatorNodeSpan
{
    ImTriangulatorNode**    Data = NULL;
    int                     Size = 0;

    void    push_back(ImTriangulatorNode* node) { Data[Size++] = node; }
    void    find_erase_unsorted(int idx)        { for (int i = Size - 1; i >= 0; i--) if (Data[i]->Index == idx) { Data[i] = Data[Size - 1]; Size--; return; } }
};

struct ImTriangulator
{
    static int EstimateTriangleCount(int points_count)      { return (points_count < 3) ? 0 : points_count - 2; }
    static int EstimateScratchBufferSize(int points_count)  { return sizeof(ImTriangulatorNode) * points_count + sizeof(ImTriangulatorNode*) * points_count * 2; }

    void    Init(const ImVec2* points, int points_count, void* scratch_buffer);
    void    GetNextTriangle(unsigned int out_triangle[3]);     // Return relative indexes for next triangle

    // Internal functions
    void    BuildNodes(const ImVec2* points, int points_count);
    void    BuildReflexes();
    void    BuildEars();
    void    FlipNodeList();
    bool    IsEar(int i0, int i1, int i2, const ImVec2& v0, const ImVec2& v1, const ImVec2& v2) const;
    void    ReclassifyNode(ImTriangulatorNode* node);

    // Internal members
    int                     _TrianglesLeft = 0;
    ImTriangulatorNode*     _Nodes = NULL;
    ImTriangulatorNodeSpan  _Ears;
    ImTriangulatorNodeSpan  _Reflexes;
};

// Distribute storage for nodes, ears and reflexes.
// FIXME-OPT: if everything is convex, we could report it to caller and let it switch to an convex renderer
// (this would require first building reflexes to bail to convex if empty, without even building nodes)
void ImTriangulator::Init(const ImVec2* points, int points_count, void* scratch_buffer)
{
    IM_ASSERT(scratch_buffer != NULL && points_count >= 3);
    _TrianglesLeft = EstimateTriangleCount(points_count);
    _Nodes         = (ImTriangulatorNode*)scratch_buffer;                          // points_count x Node
    _Ears.Data     = (ImTriangulatorNode**)(_Nodes + points_count);                // points_count x Node*
    _Reflexes.Data = (ImTriangulatorNode**)(_Nodes + points_count) + points_count; // points_count x Node*
    BuildNodes(points, points_count);
    BuildReflexes();
    BuildEars();
}

void ImTriangulator::BuildNodes(const ImVec2* points, int points_count)
{
    for (int i = 0; i < points_count; i++)
    {
        _Nodes[i].Type = ImTriangulatorNodeType_Convex;
        _Nodes[i].Index = i;
        _Nodes[i].Pos = points[i];
        _Nodes[i].Next = _Nodes + i + 1;
        _Nodes[i].Prev = _Nodes + i - 1;
    }
    _Nodes[0].Prev = _Nodes + points_count - 1;
    _Nodes[points_count - 1].Next = _Nodes;
}

void ImTriangulator::BuildReflexes()
{
    ImTriangulatorNode* n1 = _Nodes;
    for (int i = _TrianglesLeft; i >= 0; i--, n1 = n1->Next)
    {
        if (ImTriangleIsClockwise(n1->Prev->Pos, n1->Pos, n1->Next->Pos))
            continue;
        n1->Type = ImTriangulatorNodeType_Reflex;
        _Reflexes.push_back(n1);
    }
}

void ImTriangulator::BuildEars()
{
    ImTriangulatorNode* n1 = _Nodes;
    for (int i = _TrianglesLeft; i >= 0; i--, n1 = n1->Next)
    {
        if (n1->Type != ImTriangulatorNodeType_Convex)
            continue;
        if (!IsEar(n1->Prev->Index, n1->Index, n1->Next->Index, n1->Prev->Pos, n1->Pos, n1->Next->Pos))
            continue;
        n1->Type = ImTriangulatorNodeType_Ear;
        _Ears.push_back(n1);
    }
}

void ImTriangulator::GetNextTriangle(unsigned int out_triangle[3])
{
    if (_Ears.Size == 0)
    {
        FlipNodeList();

        ImTriangulatorNode* node = _Nodes;
        for (int i = _TrianglesLeft; i >= 0; i--, node = node->Next)
            node->Type = ImTriangulatorNodeType_Convex;
        _Reflexes.Size = 0;
        BuildReflexes();
        BuildEars();

        // If we still don't have ears, it means geometry is degenerated.
        if (_Ears.Size == 0)
        {
            // Return first triangle available, mimicking the behavior of convex fill.
            IM_ASSERT(_TrianglesLeft > 0); // Geometry is degenerated
            _Ears.Data[0] = _Nodes;
            _Ears.Size    = 1;
        }
    }

    ImTriangulatorNode* ear = _Ears.Data[--_Ears.Size];
    out_triangle[0] = ear->Prev->Index;
    out_triangle[1] = ear->Index;
    out_triangle[2] = ear->Next->Index;

    ear->Unlink();
    if (ear == _Nodes)
        _Nodes = ear->Next;

    ReclassifyNode(ear->Prev);
    ReclassifyNode(ear->Next);
    _TrianglesLeft--;
}

void ImTriangulator::FlipNodeList()
{
    ImTriangulatorNode* prev = _Nodes;
    ImTriangulatorNode* temp = _Nodes;
    ImTriangulatorNode* current = _Nodes->Next;
    prev->Next = prev;
    prev->Prev = prev;
    while (current != _Nodes)
    {
        temp = current->Next;

        current->Next = prev;
        prev->Prev = current;
        _Nodes->Next = current;
        current->Prev = _Nodes;

        prev = current;
        current = temp;
    }
    _Nodes = prev;
}

// A triangle is an ear is no other vertex is inside it. We can test reflexes vertices only (see reference algorithm)
bool ImTriangulator::IsEar(int i0, int i1, int i2, const ImVec2& v0, const ImVec2& v1, const ImVec2& v2) const
{
    ImTriangulatorNode** p_end = _Reflexes.Data + _Reflexes.Size;
    for (ImTriangulatorNode** p = _Reflexes.Data; p < p_end; p++)
    {
        ImTriangulatorNode* reflex = *p;
        if (reflex->Index != i0 && reflex->Index != i1 && reflex->Index != i2)
            if (ImTriangleContainsPoint(v0, v1, v2, reflex->Pos))
                return false;
    }
    return true;
}

void ImTriangulator::ReclassifyNode(ImTriangulatorNode* n1)
{
    // Classify node
    ImTriangulatorNodeType type;
    const ImTriangulatorNode* n0 = n1->Prev;
    const ImTriangulatorNode* n2 = n1->Next;
    if (!ImTriangleIsClockwise(n0->Pos, n1->Pos, n2->Pos))
        type = ImTriangulatorNodeType_Reflex;
    else if (IsEar(n0->Index, n1->Index, n2->Index, n0->Pos, n1->Pos, n2->Pos))
        type = ImTriangulatorNodeType_Ear;
    else
        type = ImTriangulatorNodeType_Convex;

    // Update lists when a type changes
    if (type == n1->Type)
        return;
    if (n1->Type == ImTriangulatorNodeType_Reflex)
        _Reflexes.find_erase_unsorted(n1->Index);
    else if (n1->Type == ImTriangulatorNodeType_Ear)
        _Ears.find_erase_unsorted(n1->Index);
    if (type == ImTriangulatorNodeType_Reflex)
        _Reflexes.push_back(n1);
    else if (type == ImTriangulatorNodeType_Ear)
        _Ears.push_back(n1);
    n1->Type = type;
}

// Use ear-clipping algorithm to triangulate a simple polygon (no self-interaction, no holes).
// (Reminder: we don't perform any coarse clipping/culling in ImDrawList layer!
// It is up to caller to ensure not making costly calls that will be outside of visible area.
// As concave fill is noticeably more expensive than other primitives, be mindful of this...
// Caller can build AABB of points, and avoid filling if 'draw_list->_CmdHeader.ClipRect.Overlays(points_bb) == false')
void ImDrawList::AddConcavePolyFilled(const ImVec2* points, const int points_count, ImU32 col)
{
    if (points_count < 3 || (col & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;
    ImTriangulator triangulator;
    unsigned int triangle[3];
    if (Flags & ImDrawListFlags_AntiAliasedFill)
    {
        // Anti-aliased Fill
        const float AA_SIZE = _FringeScale;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;
        const int idx_count = (points_count - 2) * 3 + points_count * 6;
        const int vtx_count = (points_count * 2);
        PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx + 1;

        _Data->TempBuffer.reserve_discard((ImTriangulator::EstimateScratchBufferSize(points_count) + sizeof(ImVec2)) / sizeof(ImVec2));
        triangulator.Init(points, points_count, _Data->TempBuffer.Data);
        while (triangulator._TrianglesLeft > 0)
        {
            triangulator.GetNextTriangle(triangle);
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (triangle[0] << 1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (triangle[1] << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx + (triangle[2] << 1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        _Data->TempBuffer.reserve_discard(points_count);
        ImVec2* temp_normals = _Data->TempBuffer.Data;
        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            float dx = p1.x - p0.x;
            float dy = p1.y - p0.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i0].x = dy;
            temp_normals[i0].y = -dx;
        }

        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            float dm_x = (n0.x + n1.x) * 0.5f;
            float dm_y = (n0.y + n1.y) * 0.5f;
            IM_FIXNORMAL2F(dm_x, dm_y);
            dm_x *= AA_SIZE * 0.5f;
            dm_y *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos.x = (points[i1].x - dm_x); _VtxWritePtr[0].pos.y = (points[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos.x = (points[i1].x + dm_x); _VtxWritePtr[1].pos.y = (points[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (i0 << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx + (i1 << 1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count - 2) * 3;
        const int vtx_count = points_count;
        PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        _Data->TempBuffer.reserve_discard((ImTriangulator::EstimateScratchBufferSize(points_count) + sizeof(ImVec2)) / sizeof(ImVec2));
        triangulator.Init(points, points_count, _Data->TempBuffer.Data);
        while (triangulator._TrianglesLeft > 0)
        {
            triangulator.GetNextTriangle(triangle);
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx + triangle[0]); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + triangle[1]); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + triangle[2]);
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawListSplitter
//-----------------------------------------------------------------------------
// FIXME: This may be a little confusing, trying to be a little too low-level/optimal instead of just doing vector swap..
//-----------------------------------------------------------------------------

void ImDrawListSplitter::ClearFreeMemory()
{
    for (int i = 0; i < _Channels.Size; i++)
    {
        if (i == _Current)
            memset(&_Channels[i], 0, sizeof(_Channels[i]));  // Current channel is a copy of CmdBuffer/IdxBuffer, don't destruct again
        _Channels[i]._CmdBuffer.clear();
        _Channels[i]._IdxBuffer.clear();
    }
    _Current = 0;
    _Count = 1;
    _Channels.clear();
}

void ImDrawListSplitter::Split(ImDrawList* draw_list, int channels_count)
{
    IM_UNUSED(draw_list);
    IM_ASSERT(_Current == 0 && _Count <= 1 && "Nested channel splitting is not supported. Please use separate instances of ImDrawListSplitter.");
    int old_channels_count = _Channels.Size;
    if (old_channels_count < channels_count)
    {
        _Channels.reserve(channels_count); // Avoid over reserving since this is likely to stay stable
        _Channels.resize(channels_count);
    }
    _Count = channels_count;

    // Channels[] (24/32 bytes each) hold storage that we'll swap with draw_list->_CmdBuffer/_IdxBuffer
    // The content of Channels[0] at this point doesn't matter. We clear it to make state tidy in a debugger but we don't strictly need to.
    // When we switch to the next channel, we'll copy draw_list->_CmdBuffer/_IdxBuffer into Channels[0] and then Channels[1] into draw_list->CmdBuffer/_IdxBuffer
    memset(&_Channels[0], 0, sizeof(ImDrawChannel));
    for (int i = 1; i < channels_count; i++)
    {
        if (i >= old_channels_count)
        {
            IM_PLACEMENT_NEW(&_Channels[i]) ImDrawChannel();
        }
        else
        {
            _Channels[i]._CmdBuffer.resize(0);
            _Channels[i]._IdxBuffer.resize(0);
        }
    }
}

void ImDrawListSplitter::Merge(ImDrawList* draw_list)
{
    // Note that we never use or rely on _Channels.Size because it is merely a buffer that we never shrink back to 0 to keep all sub-buffers ready for use.
    if (_Count <= 1)
        return;

    SetCurrentChannel(draw_list, 0);
    draw_list->_PopUnusedDrawCmd();

    // Calculate our final buffer sizes. Also fix the incorrect IdxOffset values in each command.
    int new_cmd_buffer_count = 0;
    int new_idx_buffer_count = 0;
    ImDrawCmd* last_cmd = (_Count > 0 && draw_list->CmdBuffer.Size > 0) ? &draw_list->CmdBuffer.back() : NULL;
    int idx_offset = last_cmd ? last_cmd->IdxOffset + last_cmd->ElemCount : 0;
    for (int i = 1; i < _Count; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (ch._CmdBuffer.Size > 0 && ch._CmdBuffer.back().ElemCount == 0 && ch._CmdBuffer.back().UserCallback == NULL) // Equivalent of PopUnusedDrawCmd()
            ch._CmdBuffer.pop_back();

        if (ch._CmdBuffer.Size > 0 && last_cmd != NULL)
        {
            // Do not include ImDrawCmd_AreSequentialIdxOffset() in the compare as we rebuild IdxOffset values ourselves.
            // Manipulating IdxOffset (e.g. by reordering draw commands like done by RenderDimmedBackgroundBehindWindow()) is not supported within a splitter.
            ImDrawCmd* next_cmd = &ch._CmdBuffer[0];
            if (ImDrawCmd_HeaderCompare(last_cmd, next_cmd) == 0 && last_cmd->UserCallback == NULL && next_cmd->UserCallback == NULL)
            {
                // Merge previous channel last draw command with current channel first draw command if matching.
                last_cmd->ElemCount += next_cmd->ElemCount;
                idx_offset += next_cmd->ElemCount;
                ch._CmdBuffer.erase(ch._CmdBuffer.Data); // FIXME-OPT: Improve for multiple merges.
            }
        }
        if (ch._CmdBuffer.Size > 0)
            last_cmd = &ch._CmdBuffer.back();
        new_cmd_buffer_count += ch._CmdBuffer.Size;
        new_idx_buffer_count += ch._IdxBuffer.Size;
        for (int cmd_n = 0; cmd_n < ch._CmdBuffer.Size; cmd_n++)
        {
            ch._CmdBuffer.Data[cmd_n].IdxOffset = idx_offset;
            idx_offset += ch._CmdBuffer.Data[cmd_n].ElemCount;
        }
    }
    draw_list->CmdBuffer.resize(draw_list->CmdBuffer.Size + new_cmd_buffer_count);
    draw_list->IdxBuffer.resize(draw_list->IdxBuffer.Size + new_idx_buffer_count);

    // Write commands and indices in order (they are fairly small structures, we don't copy vertices only indices)
    ImDrawCmd* cmd_write = draw_list->CmdBuffer.Data + draw_list->CmdBuffer.Size - new_cmd_buffer_count;
    ImDrawIdx* idx_write = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size - new_idx_buffer_count;
    for (int i = 1; i < _Count; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (int sz = ch._CmdBuffer.Size) { memcpy(cmd_write, ch._CmdBuffer.Data, sz * sizeof(ImDrawCmd)); cmd_write += sz; }
        if (int sz = ch._IdxBuffer.Size) { memcpy(idx_write, ch._IdxBuffer.Data, sz * sizeof(ImDrawIdx)); idx_write += sz; }
    }
    draw_list->_IdxWritePtr = idx_write;

    // Ensure there's always a non-callback draw command trailing the command-buffer
    if (draw_list->CmdBuffer.Size == 0 || draw_list->CmdBuffer.back().UserCallback != NULL)
        draw_list->AddDrawCmd();

    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = &draw_list->CmdBuffer.Data[draw_list->CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount == 0)
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TexRef, VtxOffset
    else if (ImDrawCmd_HeaderCompare(curr_cmd, &draw_list->_CmdHeader) != 0)
        draw_list->AddDrawCmd();

    _Count = 1;
}

void ImDrawListSplitter::SetCurrentChannel(ImDrawList* draw_list, int idx)
{
    IM_ASSERT(idx >= 0 && idx < _Count);
    if (_Current == idx)
        return;

    // Overwrite ImVector (12/16 bytes), four times. This is merely a silly optimization instead of doing .swap()
    memcpy(&_Channels.Data[_Current]._CmdBuffer, &draw_list->CmdBuffer, sizeof(draw_list->CmdBuffer));
    memcpy(&_Channels.Data[_Current]._IdxBuffer, &draw_list->IdxBuffer, sizeof(draw_list->IdxBuffer));
    _Current = idx;
    memcpy(&draw_list->CmdBuffer, &_Channels.Data[idx]._CmdBuffer, sizeof(draw_list->CmdBuffer));
    memcpy(&draw_list->IdxBuffer, &_Channels.Data[idx]._IdxBuffer, sizeof(draw_list->IdxBuffer));
    draw_list->_IdxWritePtr = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size;

    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = (draw_list->CmdBuffer.Size == 0) ? NULL : &draw_list->CmdBuffer.Data[draw_list->CmdBuffer.Size - 1];
    if (curr_cmd == NULL)
        draw_list->AddDrawCmd();
    else if (curr_cmd->ElemCount == 0)
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TexRef, VtxOffset
    else if (ImDrawCmd_HeaderCompare(curr_cmd, &draw_list->_CmdHeader) != 0)
        draw_list->AddDrawCmd();
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawData
//-----------------------------------------------------------------------------

void ImDrawData::Clear()
{
    Valid = false;
    CmdListsCount = TotalIdxCount = TotalVtxCount = 0;
    CmdLists.resize(0); // The ImDrawList are NOT owned by ImDrawData but e.g. by ImGuiContext, so we don't clear them.
    DisplayPos = DisplaySize = FramebufferScale = ImVec2(0.0f, 0.0f);
    OwnerViewport = NULL;
    Textures = NULL;
}

// Important: 'out_list' is generally going to be draw_data->CmdLists, but may be another temporary list
// as long at it is expected that the result will be later merged into draw_data->CmdLists[].
void ImGui::AddDrawListToDrawDataEx(ImDrawData* draw_data, ImVector<ImDrawList*>* out_list, ImDrawList* draw_list)
{
    if (draw_list->CmdBuffer.Size == 0)
        return;
    if (draw_list->CmdBuffer.Size == 1 && draw_list->CmdBuffer[0].ElemCount == 0 && draw_list->CmdBuffer[0].UserCallback == NULL)
        return;

    // Draw list sanity check. Detect mismatch between PrimReserve() calls and incrementing _VtxCurrentIdx, _VtxWritePtr etc.
    // May trigger for you if you are using PrimXXX functions incorrectly.
    IM_ASSERT(draw_list->VtxBuffer.Size == 0 || draw_list->_VtxWritePtr == draw_list->VtxBuffer.Data + draw_list->VtxBuffer.Size);
    IM_ASSERT(draw_list->IdxBuffer.Size == 0 || draw_list->_IdxWritePtr == draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size);
    if (!(draw_list->Flags & ImDrawListFlags_AllowVtxOffset))
        IM_ASSERT((int)draw_list->_VtxCurrentIdx == draw_list->VtxBuffer.Size);

    // Check that draw_list doesn't use more vertices than indexable (default ImDrawIdx = unsigned short = 2 bytes = 64K vertices per ImDrawList = per window)
    // If this assert triggers because you are drawing lots of stuff manually:
    // - First, make sure you are coarse clipping yourself and not trying to draw many things outside visible bounds.
    //   Be mindful that the lower-level ImDrawList API doesn't filter vertices. Use the Metrics/Debugger window to inspect draw list contents.
    // - If you want large meshes with more than 64K vertices, you can either:
    //   (A) Handle the ImDrawCmd::VtxOffset value in your renderer backend, and set 'io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset'.
    //       Most example backends already support this from 1.71. Pre-1.71 backends won't.
    //       Some graphics API such as GL ES 1/2 don't have a way to offset the starting vertex so it is not supported for them.
    //   (B) Or handle 32-bit indices in your renderer backend, and uncomment '#define ImDrawIdx unsigned int' line in imconfig.h.
    //       Most example backends already support this. For example, the OpenGL example code detect index size at compile-time:
    //         glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
    //       Your own engine or render API may use different parameters or function calls to specify index sizes.
    //       2 and 4 bytes indices are generally supported by most graphics API.
    // - If for some reason neither of those solutions works for you, a workaround is to call BeginChild()/EndChild() before reaching
    //   the 64K limit to split your draw commands in multiple draw lists.
    if (sizeof(ImDrawIdx) == 2)
        IM_ASSERT(draw_list->_VtxCurrentIdx < (1 << 16) && "Too many vertices in ImDrawList using 16-bit indices. Read comment above");

    // Resolve callback data pointers
    if (draw_list->_CallbacksDataBuf.Size > 0)
        for (ImDrawCmd& cmd : draw_list->CmdBuffer)
            if (cmd.UserCallback != NULL && cmd.UserCallbackDataOffset != -1 && cmd.UserCallbackDataSize > 0)
                cmd.UserCallbackData = draw_list->_CallbacksDataBuf.Data + cmd.UserCallbackDataOffset;

    // Add to output list + records state in ImDrawData
    out_list->push_back(draw_list);
    draw_data->CmdListsCount++;
    draw_data->TotalVtxCount += draw_list->VtxBuffer.Size;
    draw_data->TotalIdxCount += draw_list->IdxBuffer.Size;
}

void ImDrawData::AddDrawList(ImDrawList* draw_list)
{
    IM_ASSERT(CmdLists.Size == CmdListsCount);
    draw_list->_PopUnusedDrawCmd();
    ImGui::AddDrawListToDrawDataEx(this, &CmdLists, draw_list);
}

// For backward compatibility: convert all buffers from indexed to de-indexed, in case you cannot render indexed. Note: this is slow and most likely a waste of resources. Always prefer indexed rendering!
void ImDrawData::DeIndexAllBuffers()
{
    ImVector<ImDrawVert> new_vtx_buffer;
    TotalVtxCount = TotalIdxCount = 0;
    for (ImDrawList* draw_list : CmdLists)
    {
        if (draw_list->IdxBuffer.empty())
            continue;
        new_vtx_buffer.resize(draw_list->IdxBuffer.Size);
        for (int j = 0; j < draw_list->IdxBuffer.Size; j++)
            new_vtx_buffer[j] = draw_list->VtxBuffer[draw_list->IdxBuffer[j]];
        draw_list->VtxBuffer.swap(new_vtx_buffer);
        draw_list->IdxBuffer.resize(0);
        TotalVtxCount += draw_list->VtxBuffer.Size;
    }
}

// Helper to scale the ClipRect field of each ImDrawCmd.
// Use if your final output buffer is at a different scale than draw_data->DisplaySize,
// or if there is a difference between your window resolution and framebuffer resolution.
void ImDrawData::ScaleClipRects(const ImVec2& fb_scale)
{
    for (ImDrawList* draw_list : CmdLists)
        for (ImDrawCmd& cmd : draw_list->CmdBuffer)
            cmd.ClipRect = ImVec4(cmd.ClipRect.x * fb_scale.x, cmd.ClipRect.y * fb_scale.y, cmd.ClipRect.z * fb_scale.x, cmd.ClipRect.w * fb_scale.y);
}

//-----------------------------------------------------------------------------
// [SECTION] Helpers ShadeVertsXXX functions
//-----------------------------------------------------------------------------

// Generic linear color gradient, write to RGB fields, leave A untouched.
void ImGui::ShadeVertsLinearColorGradientKeepAlpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1)
{
    ImVec2 gradient_extent = gradient_p1 - gradient_p0;
    float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    const int col0_r = (int)(col0 >> IM_COL32_R_SHIFT) & 0xFF;
    const int col0_g = (int)(col0 >> IM_COL32_G_SHIFT) & 0xFF;
    const int col0_b = (int)(col0 >> IM_COL32_B_SHIFT) & 0xFF;
    const int col_delta_r = ((int)(col1 >> IM_COL32_R_SHIFT) & 0xFF) - col0_r;
    const int col_delta_g = ((int)(col1 >> IM_COL32_G_SHIFT) & 0xFF) - col0_g;
    const int col_delta_b = ((int)(col1 >> IM_COL32_B_SHIFT) & 0xFF) - col0_b;
    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        float d = ImDot(vert->pos - gradient_p0, gradient_extent);
        float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);
        int r = (int)(col0_r + col_delta_r * t);
        int g = (int)(col0_g + col_delta_g * t);
        int b = (int)(col0_b + col_delta_b * t);
        vert->col = (r << IM_COL32_R_SHIFT) | (g << IM_COL32_G_SHIFT) | (b << IM_COL32_B_SHIFT) | (vert->col & IM_COL32_A_MASK);
    }
}

// Distribute UV over (a, b) rectangle
void ImGui::ShadeVertsLinearUV(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, bool clamp)
{
    const ImVec2 size = b - a;
    const ImVec2 uv_size = uv_b - uv_a;
    const ImVec2 scale = ImVec2(
        size.x != 0.0f ? (uv_size.x / size.x) : 0.0f,
        size.y != 0.0f ? (uv_size.y / size.y) : 0.0f);

    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    if (clamp)
    {
        const ImVec2 min = ImMin(uv_a, uv_b);
        const ImVec2 max = ImMax(uv_a, uv_b);
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = ImClamp(uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale), min, max);
    }
    else
    {
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale);
    }
}

void ImGui::ShadeVertsTransformPos(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, const ImVec2& pivot_in, float cos_a, float sin_a, const ImVec2& pivot_out)
{
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
        vertex->pos = ImRotate(vertex->pos- pivot_in, cos_a, sin_a) + pivot_out;
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontConfig
//-----------------------------------------------------------------------------

// FIXME-NEWATLAS: Oversample specification could be more dynamic. For now, favoring automatic selection.
ImFontConfig::ImFontConfig()
{
    memset(this, 0, sizeof(*this));
    FontDataOwnedByAtlas = true;
    OversampleH = 0; // Auto == 1 or 2 depending on size
    OversampleV = 0; // Auto == 1
    GlyphMaxAdvanceX = FLT_MAX;
    RasterizerMultiply = 1.0f;
    RasterizerDensity = 1.0f;
    EllipsisChar = 0;
}

//-----------------------------------------------------------------------------
// [SECTION] ImTextureData
//-----------------------------------------------------------------------------
// - ImTextureData::Create()
// - ImTextureData::DestroyPixels()
//-----------------------------------------------------------------------------

int ImTextureDataGetFormatBytesPerPixel(ImTextureFormat format)
{
    switch (format)
    {
    case ImTextureFormat_Alpha8: return 1;
    case ImTextureFormat_RGBA32: return 4;
    }
    IM_ASSERT(0);
    return 0;
}

const char* ImTextureDataGetStatusName(ImTextureStatus status)
{
    switch (status)
    {
    case ImTextureStatus_OK: return "OK";
    case ImTextureStatus_Destroyed: return "Destroyed";
    case ImTextureStatus_WantCreate: return "WantCreate";
    case ImTextureStatus_WantUpdates: return "WantUpdates";
    case ImTextureStatus_WantDestroy: return "WantDestroy";
    }
    return "N/A";
}

const char* ImTextureDataGetFormatName(ImTextureFormat format)
{
    switch (format)
    {
    case ImTextureFormat_Alpha8: return "Alpha8";
    case ImTextureFormat_RGBA32: return "RGBA32";
    }
    return "N/A";
}

void ImTextureData::Create(ImTextureFormat format, int w, int h)
{
    IM_ASSERT(Status == ImTextureStatus_Destroyed);
    DestroyPixels();
    Format = format;
    Status = ImTextureStatus_WantCreate;
    Width = w;
    Height = h;
    BytesPerPixel = ImTextureDataGetFormatBytesPerPixel(format);
    UseColors = false;
    Pixels = (unsigned char*)IM_ALLOC(Width * Height * BytesPerPixel);
    IM_ASSERT(Pixels != NULL);
    memset(Pixels, 0, Width * Height * BytesPerPixel);
    UsedRect.x = UsedRect.y = UsedRect.w = UsedRect.h = 0;
    UpdateRect.x = UpdateRect.y = (unsigned short)~0;
    UpdateRect.w = UpdateRect.h = 0;
}

void ImTextureData::DestroyPixels()
{
    if (Pixels)
        IM_FREE(Pixels);
    Pixels = NULL;
    UseColors = false;
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontAtlas, ImFontAtlasBuilder
//-----------------------------------------------------------------------------
// - Default texture data encoded in ASCII
// - ImFontAtlas()
// - ImFontAtlas::Clear()
// - ImFontAtlas::CompactCache()
// - ImFontAtlas::ClearInputData()
// - ImFontAtlas::ClearTexData()
// - ImFontAtlas::ClearFonts()
//-----------------------------------------------------------------------------
// - ImFontAtlasUpdateNewFrame()
// - ImFontAtlasTextureBlockConvert()
// - ImFontAtlasTextureBlockPostProcess()
// - ImFontAtlasTextureBlockPostProcessMultiply()
// - ImFontAtlasTextureBlockFill()
// - ImFontAtlasTextureBlockCopy()
// - ImFontAtlasTextureBlockQueueUpload()
//-----------------------------------------------------------------------------
// - ImFontAtlas::GetTexDataAsAlpha8() [legacy]
// - ImFontAtlas::GetTexDataAsRGBA32() [legacy]
// - ImFontAtlas::Build() [legacy]
//-----------------------------------------------------------------------------
// - ImFontAtlas::AddFont()
// - ImFontAtlas::AddFontDefault()
// - ImFontAtlas::AddFontFromFileTTF()
// - ImFontAtlas::AddFontFromMemoryTTF()
// - ImFontAtlas::AddFontFromMemoryCompressedTTF()
// - ImFontAtlas::AddFontFromMemoryCompressedBase85TTF()
// - ImFontAtlas::RemoveFont()
// - ImFontAtlasBuildNotifySetFont()
//-----------------------------------------------------------------------------
// - ImFontAtlas::AddCustomRect()
// - ImFontAtlas::RemoveCustomRect()
// - ImFontAtlas::GetCustomRect()
// - ImFontAtlas::AddCustomRectFontGlyph() [legacy]
// - ImFontAtlas::AddCustomRectFontGlyphForSize() [legacy]
// - ImFontAtlasGetMouseCursorTexData()
//-----------------------------------------------------------------------------
// - ImFontAtlasBuildMain()
// - ImFontAtlasBuildSetupFontLoader()
// - ImFontAtlasBuildPreloadAllGlyphRanges()
// - ImFontAtlasBuildUpdatePointers()
// - ImFontAtlasBuildRenderBitmapFromString()
// - ImFontAtlasBuildUpdateBasicTexData()
// - ImFontAtlasBuildUpdateLinesTexData()
// - ImFontAtlasBuildAddFont()
// - ImFontAtlasBuildSetupFontBakedEllipsis()
// - ImFontAtlasBuildSetupFontBakedBlanks()
// - ImFontAtlasBuildSetupFontBakedFallback()
// - ImFontAtlasBuildSetupFontSpecialGlyphs()
// - ImFontAtlasBuildDiscardBakes()
// - ImFontAtlasBuildDiscardFontBakedGlyph()
// - ImFontAtlasBuildDiscardFontBaked()
// - ImFontAtlasBuildDiscardFontBakes()
//-----------------------------------------------------------------------------
// - ImFontAtlasAddDrawListSharedData()
// - ImFontAtlasRemoveDrawListSharedData()
// - ImFontAtlasUpdateDrawListsTextures()
// - ImFontAtlasUpdateDrawListsSharedData()
//-----------------------------------------------------------------------------
// - ImFontAtlasBuildSetTexture()
// - ImFontAtlasBuildAddTexture()
// - ImFontAtlasBuildMakeSpace()
// - ImFontAtlasBuildRepackTexture()
// - ImFontAtlasBuildGrowTexture()
// - ImFontAtlasBuildRepackOrGrowTexture()
// - ImFontAtlasBuildGetTextureSizeEstimate()
// - ImFontAtlasBuildCompactTexture()
// - ImFontAtlasBuildInit()
// - ImFontAtlasBuildDestroy()
//-----------------------------------------------------------------------------
// - ImFontAtlasPackInit()
// - ImFontAtlasPackAllocRectEntry()
// - ImFontAtlasPackReuseRectEntry()
// - ImFontAtlasPackDiscardRect()
// - ImFontAtlasPackAddRect()
// - ImFontAtlasPackGetRect()
//-----------------------------------------------------------------------------
// - ImFontBaked_BuildGrowIndex()
// - ImFontBaked_BuildLoadGlyph()
// - ImFontBaked_BuildLoadGlyphAdvanceX()
// - ImFontAtlasDebugLogTextureRequests()
//-----------------------------------------------------------------------------
// - ImFontAtlasGetFontLoaderForStbTruetype()
//-----------------------------------------------------------------------------

// A work of art lies ahead! (. = white layer, X = black layer, others are blank)
// The 2x2 white texels on the top left are the ones we'll use everywhere in Dear ImGui to render filled shapes.
// (This is used when io.MouseDrawCursor = true)
const int FONT_ATLAS_DEFAULT_TEX_DATA_W = 122; // Actual texture will be 2 times that + 1 spacing.
const int FONT_ATLAS_DEFAULT_TEX_DATA_H = 27;
static const char FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[FONT_ATLAS_DEFAULT_TEX_DATA_W * FONT_ATLAS_DEFAULT_TEX_DATA_H + 1] =
{
    "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX-     XX          - XX       XX "
    "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X-    X..X         -X..X     X..X"
    "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X-    X..X         -X...X   X...X"
    "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X-    X..X         - X...X X...X "
    "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X-    X..X         -  X...X...X  "
    "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X-    X..XXX       -   X.....X   "
    "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX-    X..X..XXX    -    X...X    "
    "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      -    X..X..X..XX  -     X.X     "
    "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       -    X..X..X..X.X -    X...X    "
    "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        -XXX X..X..X..X..X-   X.....X   "
    "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         -X..XX........X..X-  X...X...X  "
    "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          -X...X...........X- X...X X...X "
    "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           - X..............X-X...X   X...X"
    "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            -  X.............X-X..X     X..X"
    "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           -  X.............X- XX       XX "
    "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          -   X............X--------------"
    "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          -   X...........X -             "
    "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       -------------------------------------    X..........X -             "
    "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           -    X..........X -             "
    "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           -     X........X  -             "
    "      X..X  -       -  X...X  -         X...X         -  X..X           X..X  -           -     X........X  -             "
    "       XX   -       -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           -     XXXXXXXXXX  -             "
    "-------------       -    X    -           X           -X.....................X-           -------------------             "
    "                    ----------------------------------- X...XXXXXXXXXXXXX...X -                                           "
    "                                                      -  X..X           X..X  -                                           "
    "                                                      -   X.X           X.X   -                                           "
    "                                                      -    XX           XX    -                                           "
};

static const ImVec2 FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[ImGuiMouseCursor_COUNT][3] =
{
    // Pos ........ Size ......... Offset ......
    { ImVec2( 0,3), ImVec2(12,19), ImVec2( 0, 0) }, // ImGuiMouseCursor_Arrow
    { ImVec2(13,0), ImVec2( 7,16), ImVec2( 1, 8) }, // ImGuiMouseCursor_TextInput
    { ImVec2(31,0), ImVec2(23,23), ImVec2(11,11) }, // ImGuiMouseCursor_ResizeAll
    { ImVec2(21,0), ImVec2( 9,23), ImVec2( 4,11) }, // ImGuiMouseCursor_ResizeNS
    { ImVec2(55,18),ImVec2(23, 9), ImVec2(11, 4) }, // ImGuiMouseCursor_ResizeEW
    { ImVec2(73,0), ImVec2(17,17), ImVec2( 8, 8) }, // ImGuiMouseCursor_ResizeNESW
    { ImVec2(55,0), ImVec2(17,17), ImVec2( 8, 8) }, // ImGuiMouseCursor_ResizeNWSE
    { ImVec2(91,0), ImVec2(17,22), ImVec2( 5, 0) }, // ImGuiMouseCursor_Hand
    { ImVec2(0,3),  ImVec2(12,19), ImVec2(0, 0) },  // ImGuiMouseCursor_Wait       // Arrow + custom code in ImGui::RenderMouseCursor()
    { ImVec2(0,3),  ImVec2(12,19), ImVec2(0, 0) },  // ImGuiMouseCursor_Progress   // Arrow + custom code in ImGui::RenderMouseCursor()
    { ImVec2(109,0),ImVec2(13,15), ImVec2( 6, 7) }, // ImGuiMouseCursor_NotAllowed
};

#define IM_FONTGLYPH_INDEX_UNUSED           ((ImU16)-1) // 0xFFFF
#define IM_FONTGLYPH_INDEX_NOT_FOUND        ((ImU16)-2) // 0xFFFE

ImFontAtlas::ImFontAtlas()
{
    memset(this, 0, sizeof(*this));
    TexDesiredFormat = ImTextureFormat_RGBA32;
    TexGlyphPadding = 1;
    TexMinWidth = 512;
    TexMinHeight = 128;
    TexMaxWidth = 8192;
    TexMaxHeight = 8192;
    TexRef._TexID = ImTextureID_Invalid;
    RendererHasTextures = false; // Assumed false by default, as apps can call e.g Atlas::Build() after backend init and before ImGui can update.
    TexNextUniqueID = 1;
    FontNextUniqueID = 1;
    Builder = NULL;
}

ImFontAtlas::~ImFontAtlas()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    RendererHasTextures = false; // Full Clear() is supported, but ClearTexData() only isn't.
    ClearFonts();
    ClearTexData();
    TexList.clear_delete();
    TexData = NULL;
}

void ImFontAtlas::Clear()
{
    bool backup_renderer_has_textures = RendererHasTextures;
    RendererHasTextures = false; // Full Clear() is supported, but ClearTexData() only isn't.
    ClearFonts();
    ClearTexData();
    RendererHasTextures = backup_renderer_has_textures;
}

void ImFontAtlas::CompactCache()
{
    ImFontAtlasTextureCompact(this);
}

void ImFontAtlas::SetFontLoader(const ImFontLoader* font_loader)
{
    ImFontAtlasBuildSetupFontLoader(this, font_loader);
}

void ImFontAtlas::ClearInputData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");

    for (ImFont* font : Fonts)
        ImFontAtlasFontDestroyOutput(this, font);
    for (ImFontConfig& font_cfg : Sources)
        ImFontAtlasFontDestroySourceData(this, &font_cfg);
    for (ImFont* font : Fonts)
    {
        // When clearing this we lose access to the font name and other information used to build the font.
        font->Sources.clear();
        font->Flags |= ImFontFlags_NoLoadGlyphs;
    }
    Sources.clear();
}

// Clear CPU-side copy of the texture data.
void ImFontAtlas::ClearTexData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    IM_ASSERT(RendererHasTextures == false && "Not supported for dynamic atlases, but you may call Clear().");
    for (ImTextureData* tex : TexList)
        tex->DestroyPixels();
    //Locked = true; // Hoped to be able to lock this down but some reload patterns may not be happy with it.
}

void ImFontAtlas::ClearFonts()
{
    // FIXME-NEWATLAS: Illegal to remove currently bound font.
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    ImFontAtlasBuildDestroy(this);
    ClearInputData();
    Fonts.clear_delete();
    TexIsBuilt = false;
    for (ImDrawListSharedData* shared_data : DrawListSharedDatas)
        if (shared_data->FontAtlas == this)
        {
            shared_data->Font = NULL;
            shared_data->FontScale = shared_data->FontSize = 0.0f;
        }
}

static void ImFontAtlasBuildUpdateRendererHasTexturesFromContext(ImFontAtlas* atlas)
{
    // [LEGACY] Copy back the ImGuiBackendFlags_RendererHasTextures flag from ImGui context.
    // - This is the 1% exceptional case where that dependency if useful, to bypass an issue where otherwise at the
    //   time of an early call to Build(), it would be impossible for us to tell if the backend supports texture update.
    // - Without this hack, we would have quite a pitfall as many legacy codebases have an early call to Build().
    //   Whereas conversely, the portion of people using ImDrawList without ImGui is expected to be pathologically rare.
    for (ImDrawListSharedData* shared_data : atlas->DrawListSharedDatas)
        if (ImGuiContext* imgui_ctx = shared_data->Context)
        {
            atlas->RendererHasTextures = (imgui_ctx->IO.BackendFlags & ImGuiBackendFlags_RendererHasTextures) != 0;
            break;
        }
}

// Called by NewFrame() for atlases owned by a context.
// If you manually manage font atlases, you'll need to call this yourself.
// - 'frame_count' needs to be provided because we can gc/prioritize baked fonts based on their age.
// - 'frame_count' may not match those of all imgui contexts using this atlas, as contexts may be updated as different frequencies. But generally you can use ImGui::GetFrameCount() on one of your context.
void ImFontAtlasUpdateNewFrame(ImFontAtlas* atlas, int frame_count, bool renderer_has_textures)
{
    IM_ASSERT(atlas->Builder == NULL || atlas->Builder->FrameCount < frame_count); // Protection against being called twice.
    atlas->RendererHasTextures = renderer_has_textures;

    // Check that font atlas was built or backend support texture reload in which case we can build now
    if (atlas->RendererHasTextures)
    {
        atlas->TexIsBuilt = true;
        if (atlas->Builder == NULL) // This will only happen if fonts were not already loaded.
            ImFontAtlasBuildMain(atlas);
    }
    // Legacy backend
    if (!atlas->RendererHasTextures)
        IM_ASSERT_USER_ERROR(atlas->TexIsBuilt, "Backend does not support ImGuiBackendFlags_RendererHasTextures, and font atlas is not built! Update backend OR make sure you called ImGui_ImplXXXX_NewFrame() function for renderer backend, which should call io.Fonts->GetTexDataAsRGBA32() / GetTexDataAsAlpha8().");
    if (atlas->TexIsBuilt && atlas->Builder->PreloadedAllGlyphsRanges)
        IM_ASSERT_USER_ERROR(atlas->RendererHasTextures == false, "Called ImFontAtlas::Build() before ImGuiBackendFlags_RendererHasTextures got set! With new backends: you don't need to call Build().");

    // Clear BakedCurrent cache, this is important because it ensure the uncached path gets taken once.
    // We also rely on ImFontBaked* pointers never crossing frames.
    ImFontAtlasBuilder* builder = atlas->Builder;
    builder->FrameCount = frame_count;
    for (ImFont* font : atlas->Fonts)
        font->LastBaked = NULL;

    // Garbage collect BakedPool
    if (builder->BakedDiscardedCount > 0)
    {
        int dst_n = 0, src_n = 0;
        for (; src_n < builder->BakedPool.Size; src_n++)
        {
            ImFontBaked* p_src = &builder->BakedPool[src_n];
            if (p_src->WantDestroy)
                continue;
            ImFontBaked* p_dst = &builder->BakedPool[dst_n++];
            if (p_dst == p_src)
                continue;
            memcpy(p_dst, p_src, sizeof(ImFontBaked));
            builder->BakedMap.SetVoidPtr(p_dst->BakedId, p_dst);
        }
        IM_ASSERT(dst_n + builder->BakedDiscardedCount == src_n);
        builder->BakedPool.Size -= builder->BakedDiscardedCount;
        builder->BakedDiscardedCount = 0;
    }

    // Update texture status
    for (int tex_n = 0; tex_n < atlas->TexList.Size; tex_n++)
    {
        ImTextureData* tex = atlas->TexList[tex_n];
        bool remove_from_list = false;
        if (tex->Status == ImTextureStatus_OK)
        {
            tex->Updates.resize(0);
            tex->UpdateRect.x = tex->UpdateRect.y = (unsigned short)~0;
            tex->UpdateRect.w = tex->UpdateRect.h = 0;
        }
        if (tex->Status == ImTextureStatus_WantCreate && atlas->RendererHasTextures)
            IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == NULL && "Backend set texture's TexID/BackendUserData but did not update Status to OK.");

        // Request destroy
        // - Keep bool to true in order to differentiate a planned destroy vs a destroy decided by the backend.
        // - We don't destroy pixels right away, as backend may have an in-flight copy from RAM.
        if (tex->WantDestroyNextFrame && tex->Status != ImTextureStatus_Destroyed && tex->Status != ImTextureStatus_WantDestroy)
        {
            IM_ASSERT(tex->Status == ImTextureStatus_OK || tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates);
            tex->Status = ImTextureStatus_WantDestroy;
        }

        // If a texture has never reached the backend, they don't need to know about it.
        // (note: backends between 1.92.0 and 1.92.4 could set an already destroyed texture to ImTextureStatus_WantDestroy
        //  when invalidating graphics objects twice, which would previously remove it from the list and crash.)
        if (tex->Status == ImTextureStatus_WantDestroy && tex->TexID == ImTextureID_Invalid && tex->BackendUserData == NULL)
            tex->Status = ImTextureStatus_Destroyed;

        // Process texture being destroyed
        if (tex->Status == ImTextureStatus_Destroyed)
        {
            IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == NULL && "Backend set texture Status to Destroyed but did not clear TexID/BackendUserData!");
            if (tex->WantDestroyNextFrame)
                remove_from_list = true; // Destroy was scheduled by us
            else
                tex->Status = ImTextureStatus_WantCreate; // Destroy was done was backend: recreate it (e.g. freed resources mid-run)
        }

        // The backend may need defer destroying by a few frames, to handle texture used by previous in-flight rendering.
        // We allow the texture staying in _WantDestroy state and increment a counter which the backend can use to take its decision.
        if (tex->Status == ImTextureStatus_WantDestroy)
            tex->UnusedFrames++;

        // Destroy and remove
        if (remove_from_list)
        {
            IM_ASSERT(atlas->TexData != tex);
            tex->DestroyPixels();
            IM_DELETE(tex);
            atlas->TexList.erase(atlas->TexList.begin() + tex_n);
            tex_n--;
        }
    }
}

void ImFontAtlasTextureBlockConvert(const unsigned char* src_pixels, ImTextureFormat src_fmt, int src_pitch, unsigned char* dst_pixels, ImTextureFormat dst_fmt, int dst_pitch, int w, int h)
{
    IM_ASSERT(src_pixels != NULL && dst_pixels != NULL);
    if (src_fmt == dst_fmt)
    {
        int line_sz = w * ImTextureDataGetFormatBytesPerPixel(src_fmt);
        for (int ny = h; ny > 0; ny--, src_pixels += src_pitch, dst_pixels += dst_pitch)
            memcpy(dst_pixels, src_pixels, line_sz);
    }
    else if (src_fmt == ImTextureFormat_Alpha8 && dst_fmt == ImTextureFormat_RGBA32)
    {
        for (int ny = h; ny > 0; ny--, src_pixels += src_pitch, dst_pixels += dst_pitch)
        {
            const ImU8* src_p = (const ImU8*)src_pixels;
            ImU32* dst_p = (ImU32*)(void*)dst_pixels;
            for (int nx = w; nx > 0; nx--)
                *dst_p++ = IM_COL32(255, 255, 255, (unsigned int)(*src_p++));
        }
    }
    else if (src_fmt == ImTextureFormat_RGBA32 && dst_fmt == ImTextureFormat_Alpha8)
    {
        for (int ny = h; ny > 0; ny--, src_pixels += src_pitch, dst_pixels += dst_pitch)
        {
            const ImU32* src_p = (const ImU32*)(void*)src_pixels;
            ImU8* dst_p = (ImU8*)dst_pixels;
            for (int nx = w; nx > 0; nx--)
                *dst_p++ = ((*src_p++) >> IM_COL32_A_SHIFT) & 0xFF;
        }
    }
    else
    {
        IM_ASSERT(0);
    }
}

// Source buffer may be written to (used for in-place mods).
// Post-process hooks may eventually be added here.
void ImFontAtlasTextureBlockPostProcess(ImFontAtlasPostProcessData* data)
{
    // Multiply operator (legacy)
    if (data->FontSrc->RasterizerMultiply != 1.0f)
        ImFontAtlasTextureBlockPostProcessMultiply(data, data->FontSrc->RasterizerMultiply);
}

void ImFontAtlasTextureBlockPostProcessMultiply(ImFontAtlasPostProcessData* data, float multiply_factor)
{
    unsigned char* pixels = (unsigned char*)data->Pixels;
    int pitch = data->Pitch;
    if (data->Format == ImTextureFormat_Alpha8)
    {
        for (int ny = data->Height; ny > 0; ny--, pixels += pitch)
        {
            ImU8* p = (ImU8*)pixels;
            for (int nx = data->Width; nx > 0; nx--, p++)
            {
                unsigned int v = ImMin((unsigned int)(*p * multiply_factor), (unsigned int)255);
                *p = (unsigned char)v;
            }
        }
    }
    else if (data->Format == ImTextureFormat_RGBA32) //-V547
    {
        for (int ny = data->Height; ny > 0; ny--, pixels += pitch)
        {
            ImU32* p = (ImU32*)(void*)pixels;
            for (int nx = data->Width; nx > 0; nx--, p++)
            {
                unsigned int a = ImMin((unsigned int)(((*p >> IM_COL32_A_SHIFT) & 0xFF) * multiply_factor), (unsigned int)255);
                *p = IM_COL32((*p >> IM_COL32_R_SHIFT) & 0xFF, (*p >> IM_COL32_G_SHIFT) & 0xFF, (*p >> IM_COL32_B_SHIFT) & 0xFF, a);
            }
        }
    }
    else
    {
        IM_ASSERT(0);
    }
}

// Fill with single color. We don't use this directly but it is convenient for anyone working on uploading custom rects.
void ImFontAtlasTextureBlockFill(ImTextureData* dst_tex, int dst_x, int dst_y, int w, int h, ImU32 col)
{
    if (dst_tex->Format == ImTextureFormat_Alpha8)
    {
        ImU8 col_a = (col >> IM_COL32_A_SHIFT) & 0xFF;
        for (int y = 0; y < h; y++)
            memset((ImU8*)dst_tex->GetPixelsAt(dst_x, dst_y + y), col_a, w);
    }
    else
    {
        for (int y = 0; y < h; y++)
        {
            ImU32* p = (ImU32*)(void*)dst_tex->GetPixelsAt(dst_x, dst_y + y);
            for (int x = w; x > 0; x--, p++)
                *p = col;
        }
    }
}

// Copy block from one texture to another
void ImFontAtlasTextureBlockCopy(ImTextureData* src_tex, int src_x, int src_y, ImTextureData* dst_tex, int dst_x, int dst_y, int w, int h)
{
    IM_ASSERT(src_tex->Pixels != NULL && dst_tex->Pixels != NULL);
    IM_ASSERT(src_tex->Format == dst_tex->Format);
    IM_ASSERT(src_x >= 0 && src_x + w <= src_tex->Width);
    IM_ASSERT(src_y >= 0 && src_y + h <= src_tex->Height);
    IM_ASSERT(dst_x >= 0 && dst_x + w <= dst_tex->Width);
    IM_ASSERT(dst_y >= 0 && dst_y + h <= dst_tex->Height);
    for (int y = 0; y < h; y++)
        memcpy(dst_tex->GetPixelsAt(dst_x, dst_y + y), src_tex->GetPixelsAt(src_x, src_y + y), w * dst_tex->BytesPerPixel);
}

// Queue texture block update for renderer backend
void ImFontAtlasTextureBlockQueueUpload(ImFontAtlas* atlas, ImTextureData* tex, int x, int y, int w, int h)
{
    IM_ASSERT(tex->Status != ImTextureStatus_WantDestroy && tex->Status != ImTextureStatus_Destroyed);
    IM_ASSERT(x >= 0 && x <= 0xFFFF && y >= 0 && y <= 0xFFFF && w >= 0 && x + w <= 0x10000 && h >= 0 && y + h <= 0x10000);
    IM_UNUSED(atlas);

    ImTextureRect req = { (unsigned short)x, (unsigned short)y, (unsigned short)w, (unsigned short)h };
    int new_x1 = ImMax(tex->UpdateRect.w == 0 ? 0 : tex->UpdateRect.x + tex->UpdateRect.w, req.x + req.w);
    int new_y1 = ImMax(tex->UpdateRect.h == 0 ? 0 : tex->UpdateRect.y + tex->UpdateRect.h, req.y + req.h);
    tex->UpdateRect.x = ImMin(tex->UpdateRect.x, req.x);
    tex->UpdateRect.y = ImMin(tex->UpdateRect.y, req.y);
    tex->UpdateRect.w = (unsigned short)(new_x1 - tex->UpdateRect.x);
    tex->UpdateRect.h = (unsigned short)(new_y1 - tex->UpdateRect.y);
    tex->UsedRect.x = ImMin(tex->UsedRect.x, req.x);
    tex->UsedRect.y = ImMin(tex->UsedRect.y, req.y);
    tex->UsedRect.w = (unsigned short)(ImMax(tex->UsedRect.x + tex->UsedRect.w, req.x + req.w) - tex->UsedRect.x);
    tex->UsedRect.h = (unsigned short)(ImMax(tex->UsedRect.y + tex->UsedRect.h, req.y + req.h) - tex->UsedRect.y);
    atlas->TexIsBuilt = false;

    // No need to queue if status is == ImTextureStatus_WantCreate
    if (tex->Status == ImTextureStatus_OK || tex->Status == ImTextureStatus_WantUpdates)
    {
        tex->Status = ImTextureStatus_WantUpdates;
        tex->Updates.push_back(req);
    }
}

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
static void GetTexDataAsFormat(ImFontAtlas* atlas, ImTextureFormat format, unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    ImTextureData* tex = atlas->TexData;
    if (!atlas->TexIsBuilt || tex == NULL || tex->Pixels == NULL || atlas->TexDesiredFormat != format)
    {
        atlas->TexDesiredFormat = format;
        atlas->Build();
        tex = atlas->TexData;
    }
    if (out_pixels) { *out_pixels = (unsigned char*)tex->Pixels; };
    if (out_width) { *out_width = tex->Width; };
    if (out_height) { *out_height = tex->Height; };
    if (out_bytes_per_pixel) { *out_bytes_per_pixel = tex->BytesPerPixel; }
}

void ImFontAtlas::GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    GetTexDataAsFormat(this, ImTextureFormat_Alpha8, out_pixels, out_width, out_height, out_bytes_per_pixel);
}

void ImFontAtlas::GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    GetTexDataAsFormat(this, ImTextureFormat_RGBA32, out_pixels, out_width, out_height, out_bytes_per_pixel);
}

bool ImFontAtlas::Build()
{
    ImFontAtlasBuildMain(this);
    return true;
}
#endif // #ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS

ImFont* ImFontAtlas::AddFont(const ImFontConfig* font_cfg_in)
{
    // Sanity Checks
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    IM_ASSERT((font_cfg_in->FontData != NULL && font_cfg_in->FontDataSize > 0) || (font_cfg_in->FontLoader != NULL));
    //IM_ASSERT(font_cfg_in->SizePixels > 0.0f && "Is ImFontConfig struct correctly initialized?");
    IM_ASSERT(font_cfg_in->RasterizerDensity > 0.0f && "Is ImFontConfig struct correctly initialized?");
    if (font_cfg_in->GlyphOffset.x != 0.0f || font_cfg_in->GlyphOffset.y != 0.0f || font_cfg_in->GlyphMinAdvanceX != 0.0f || font_cfg_in->GlyphMaxAdvanceX != FLT_MAX)
        IM_ASSERT(font_cfg_in->SizePixels != 0.0f && "Specifying glyph offset/advances requires a reference size to base it on.");

    // Lazily create builder on the first call to AddFont
    if (Builder == NULL)
        ImFontAtlasBuildInit(this);

    // Create new font
    ImFont* font;
    if (!font_cfg_in->MergeMode)
    {
        font = IM_NEW(ImFont)();
        font->FontId = FontNextUniqueID++;
        font->Flags = font_cfg_in->Flags;
        font->LegacySize = font_cfg_in->SizePixels;
        font->CurrentRasterizerDensity = font_cfg_in->RasterizerDensity;
        Fonts.push_back(font);
    }
    else
    {
        IM_ASSERT(Fonts.Size > 0 && "Cannot use MergeMode for the first font"); // When using MergeMode make sure that a font has already been added before. You can use ImGui::GetIO().Fonts->AddFontDefault() to add the default imgui font.
        font = font_cfg_in->DstFont ? font_cfg_in->DstFont : Fonts.back();
    }

    // Add to list
    Sources.push_back(*font_cfg_in);
    ImFontConfig* font_cfg = &Sources.back();
    if (font_cfg->DstFont == NULL)
        font_cfg->DstFont = font;
    font->Sources.push_back(font_cfg);
    ImFontAtlasBuildUpdatePointers(this); // Pointers to Sources are otherwise dangling after we called Sources.push_back().

    if (font_cfg->FontDataOwnedByAtlas == false)
    {
        font_cfg->FontDataOwnedByAtlas = true;
        font_cfg->FontData = ImMemdup(font_cfg->FontData, (size_t)font_cfg->FontDataSize);
    }

    // Sanity check
    // We don't round cfg.SizePixels yet as relative size of merged fonts are used afterwards.
    if (font_cfg->GlyphExcludeRanges != NULL)
    {
        int size = 0;
        for (const ImWchar* p = font_cfg->GlyphExcludeRanges; p[0] != 0; p++, size++) {}
        IM_ASSERT((size & 1) == 0 && "GlyphExcludeRanges[] size must be multiple of two!");
        IM_ASSERT((size <= 64) && "GlyphExcludeRanges[] size must be small!");
        font_cfg->GlyphExcludeRanges = (ImWchar*)ImMemdup(font_cfg->GlyphExcludeRanges, sizeof(font_cfg->GlyphExcludeRanges[0]) * (size + 1));
    }
    if (font_cfg->FontLoader != NULL)
    {
        IM_ASSERT(font_cfg->FontLoader->FontBakedLoadGlyph != NULL);
        IM_ASSERT(font_cfg->FontLoader->LoaderInit == NULL && font_cfg->FontLoader->LoaderShutdown == NULL); // FIXME-NEWATLAS: Unsupported yet.
    }
    IM_ASSERT(font_cfg->FontLoaderData == NULL);

    if (!ImFontAtlasFontSourceInit(this, font_cfg))
    {
        // Rollback (this is a fragile/rarely exercised code-path. TestSuite's "misc_atlas_add_invalid_font" aim to test this)
        ImFontAtlasFontDestroySourceData(this, font_cfg);
        Sources.pop_back();
        font->Sources.pop_back();
        if (!font_cfg->MergeMode)
        {
            IM_DELETE(font);
            Fonts.pop_back();
        }
        return NULL;
    }
    ImFontAtlasFontSourceAddToFont(this, font, font_cfg);

    return font;
}

// Default font TTF is compressed with stb_compress then base85 encoded (see misc/fonts/binary_to_compressed_c.cpp for encoder)
static unsigned int stb_decompress_length(const unsigned char* input);
static unsigned int stb_decompress(unsigned char* output, const unsigned char* input, unsigned int length);
static unsigned int Decode85Byte(char c)                                    { return c >= '\\' ? c-36 : c-35; }
static void         Decode85(const unsigned char* src, unsigned char* dst)
{
    while (*src)
    {
        unsigned int tmp = Decode85Byte(src[0]) + 85 * (Decode85Byte(src[1]) + 85 * (Decode85Byte(src[2]) + 85 * (Decode85Byte(src[3]) + 85 * Decode85Byte(src[4]))));
        dst[0] = ((tmp >> 0) & 0xFF); dst[1] = ((tmp >> 8) & 0xFF); dst[2] = ((tmp >> 16) & 0xFF); dst[3] = ((tmp >> 24) & 0xFF);   // We can't assume little-endianness.
        src += 5;
        dst += 4;
    }
}
#ifndef IMGUI_DISABLE_DEFAULT_FONT
static const char* GetDefaultCompressedFontDataTTF(int* out_size);
#endif

// Load embedded ProggyClean.ttf at size 13, disable oversampling
// If you want a similar font which may be better scaled, consider using ProggyVector from the same author!
ImFont* ImFontAtlas::AddFontDefault(const ImFontConfig* font_cfg_template)
{
#ifndef IMGUI_DISABLE_DEFAULT_FONT
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (!font_cfg_template)
    {
        font_cfg.OversampleH = font_cfg.OversampleV = 1;
        font_cfg.PixelSnapH = true;
    }
    if (font_cfg.SizePixels <= 0.0f)
        font_cfg.SizePixels = 13.0f * 1.0f;
    if (font_cfg.Name[0] == '\0')
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "ProggyClean.ttf");
    font_cfg.EllipsisChar = (ImWchar)0x0085;
    font_cfg.GlyphOffset.y += 1.0f * IM_TRUNC(font_cfg.SizePixels / 13.0f);  // Add +1 offset per 13 units

    int ttf_compressed_size = 0;
    const char* ttf_compressed = GetDefaultCompressedFontDataTTF(&ttf_compressed_size);
    return AddFontFromMemoryCompressedTTF(ttf_compressed, ttf_compressed_size, font_cfg.SizePixels, &font_cfg);
#else
    IM_ASSERT(0 && "AddFontDefault() disabled in this build.");
    IM_UNUSED(font_cfg_template);
    return NULL;
#endif // #ifndef IMGUI_DISABLE_DEFAULT_FONT
}

ImFont* ImFontAtlas::AddFontFromFileTTF(const char* filename, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    size_t data_size = 0;
    void* data = ImFileLoadToMemory(filename, "rb", &data_size, 0);
    if (!data)
    {
        if (font_cfg_template == NULL || (font_cfg_template->Flags & ImFontFlags_NoLoadError) == 0)
        {
            IMGUI_DEBUG_LOG("While loading '%s'\n", filename);
            IM_ASSERT_USER_ERROR(0, "Could not load font file!");
        }
        return NULL;
    }
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (font_cfg.Name[0] == '\0')
    {
        // Store a short copy of filename into into the font name for convenience
        const char* p;
        for (p = filename + ImStrlen(filename); p > filename && p[-1] != '/' && p[-1] != '\\'; p--) {}
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "%s", p);
    }
    return AddFontFromMemoryTTF(data, (int)data_size, size_pixels, &font_cfg, glyph_ranges);
}

// NB: Transfer ownership of 'ttf_data' to ImFontAtlas, unless font_cfg_template->FontDataOwnedByAtlas == false. Owned TTF buffer will be deleted after Build().
ImFont* ImFontAtlas::AddFontFromMemoryTTF(void* font_data, int font_data_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    IM_ASSERT(font_data_size > 100 && "Incorrect value for font_data_size!"); // Heuristic to prevent accidentally passing a wrong value to font_data_size.
    font_cfg.FontData = font_data;
    font_cfg.FontDataSize = font_data_size;
    font_cfg.SizePixels = size_pixels > 0.0f ? size_pixels : font_cfg.SizePixels;
    if (glyph_ranges)
        font_cfg.GlyphRanges = glyph_ranges;
    return AddFont(&font_cfg);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedTTF(const void* compressed_ttf_data, int compressed_ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    const unsigned int buf_decompressed_size = stb_decompress_length((const unsigned char*)compressed_ttf_data);
    unsigned char* buf_decompressed_data = (unsigned char*)IM_ALLOC(buf_decompressed_size);
    stb_decompress(buf_decompressed_data, (const unsigned char*)compressed_ttf_data, (unsigned int)compressed_ttf_size);

    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    font_cfg.FontDataOwnedByAtlas = true;
    return AddFontFromMemoryTTF(buf_decompressed_data, (int)buf_decompressed_size, size_pixels, &font_cfg, glyph_ranges);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedBase85TTF(const char* compressed_ttf_data_base85, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges)
{
    int compressed_ttf_size = (((int)ImStrlen(compressed_ttf_data_base85) + 4) / 5) * 4;
    void* compressed_ttf = IM_ALLOC((size_t)compressed_ttf_size);
    Decode85((const unsigned char*)compressed_ttf_data_base85, (unsigned char*)compressed_ttf);
    ImFont* font = AddFontFromMemoryCompressedTTF(compressed_ttf, compressed_ttf_size, size_pixels, font_cfg, glyph_ranges);
    IM_FREE(compressed_ttf);
    return font;
}

// On font removal we need to remove references (otherwise we could queue removal?)
// We allow old_font == new_font which forces updating all values (e.g. sizes)
static void ImFontAtlasBuildNotifySetFont(ImFontAtlas* atlas, ImFont* old_font, ImFont* new_font)
{
    for (ImDrawListSharedData* shared_data : atlas->DrawListSharedDatas)
    {
        if (shared_data->Font == old_font)
            shared_data->Font = new_font;
        if (ImGuiContext* ctx = shared_data->Context)
        {
            if (ctx->IO.FontDefault == old_font)
                ctx->IO.FontDefault = new_font;
            if (ctx->Font == old_font)
            {
                ImGuiContext* curr_ctx = ImGui::GetCurrentContext();
                bool need_bind_ctx = ctx != curr_ctx;
                if (need_bind_ctx)
                    ImGui::SetCurrentContext(ctx);
                ImGui::SetCurrentFont(new_font, ctx->FontSizeBase, ctx->FontSize);
                if (need_bind_ctx)
                    ImGui::SetCurrentContext(curr_ctx);
            }
            for (ImFontStackData& font_stack_data : ctx->FontStack)
                if (font_stack_data.Font == old_font)
                    font_stack_data.Font = new_font;
        }
    }
}

void ImFontAtlas::RemoveFont(ImFont* font)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas!");
    font->ClearOutputData();

    ImFontAtlasFontDestroyOutput(this, font);
    for (ImFontConfig* src : font->Sources)
        ImFontAtlasFontDestroySourceData(this, src);
    for (int src_n = 0; src_n < Sources.Size; src_n++)
        if (Sources[src_n].DstFont == font)
            Sources.erase(&Sources[src_n--]);

    bool removed = Fonts.find_erase(font);
    IM_ASSERT(removed);
    IM_UNUSED(removed);

    ImFontAtlasBuildUpdatePointers(this);

    font->ContainerAtlas = NULL;
    IM_DELETE(font);

    // Notify external systems
    ImFont* new_current_font = Fonts.empty() ? NULL : Fonts[0];
    ImFontAtlasBuildNotifySetFont(this, font, new_current_font);
}

// At it is common to do an AddCustomRect() followed by a GetCustomRect(), we provide an optional 'ImFontAtlasRect* out_r = NULL' argument to retrieve the info straight away.
ImFontAtlasRectId ImFontAtlas::AddCustomRect(int width, int height, ImFontAtlasRect* out_r)
{
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);

    if (Builder == NULL)
        ImFontAtlasBuildInit(this);

    ImFontAtlasRectId r_id = ImFontAtlasPackAddRect(this, width, height);
    if (r_id == ImFontAtlasRectId_Invalid)
        return ImFontAtlasRectId_Invalid;
    if (out_r != NULL)
        GetCustomRect(r_id, out_r);

    if (RendererHasTextures)
    {
        ImTextureRect* r = ImFontAtlasPackGetRect(this, r_id);
        ImFontAtlasTextureBlockQueueUpload(this, TexData, r->x, r->y, r->w, r->h);
    }
    return r_id;
}

void ImFontAtlas::RemoveCustomRect(ImFontAtlasRectId id)
{
    if (ImFontAtlasPackGetRectSafe(this, id) == NULL)
        return;
    ImFontAtlasPackDiscardRect(this, id);
}

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
// This API does not make sense anymore with scalable fonts.
// - Prefer adding a font source (ImFontConfig) using a custom/procedural loader.
// - You may use ImFontFlags_LockBakedSizes to limit an existing font to known baked sizes:
//     ImFont* myfont = io.Fonts->AddFontFromFileTTF(....);
//     myfont->GetFontBaked(16.0f);
//     myfont->Flags |= ImFontFlags_LockBakedSizes;
ImFontAtlasRectId ImFontAtlas::AddCustomRectFontGlyph(ImFont* font, ImWchar codepoint, int width, int height, float advance_x, const ImVec2& offset)
{
    float font_size = font->LegacySize;
    return AddCustomRectFontGlyphForSize(font, font_size, codepoint, width, height, advance_x, offset);
}
// FIXME: we automatically set glyph.Colored=true by default.
// If you need to alter this, you can write 'font->Glyphs.back()->Colored' after calling AddCustomRectFontGlyph().
ImFontAtlasRectId ImFontAtlas::AddCustomRectFontGlyphForSize(ImFont* font, float font_size, ImWchar codepoint, int width, int height, float advance_x, const ImVec2& offset)
{
#ifdef IMGUI_USE_WCHAR32
    IM_ASSERT(codepoint <= IM_UNICODE_CODEPOINT_MAX);
#endif
    IM_ASSERT(font != NULL);
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);

    ImFontBaked* baked = font->GetFontBaked(font_size);

    ImFontAtlasRectId r_id = ImFontAtlasPackAddRect(this, width, height);
    if (r_id == ImFontAtlasRectId_Invalid)
        return ImFontAtlasRectId_Invalid;
    ImTextureRect* r = ImFontAtlasPackGetRect(this, r_id);
    if (RendererHasTextures)
        ImFontAtlasTextureBlockQueueUpload(this, TexData, r->x, r->y, r->w, r->h);

    if (baked->IsGlyphLoaded(codepoint))
        ImFontAtlasBakedDiscardFontGlyph(this, font, baked, baked->FindGlyph(codepoint));

    ImFontGlyph glyph;
    glyph.Codepoint = codepoint;
    glyph.AdvanceX = advance_x;
    glyph.X0 = offset.x;
    glyph.Y0 = offset.y;
    glyph.X1 = offset.x + r->w;
    glyph.Y1 = offset.y + r->h;
    glyph.Visible = true;
    glyph.Colored = true; // FIXME: Arbitrary
    glyph.PackId = r_id;
    ImFontAtlasBakedAddFontGlyph(this, baked, font->Sources[0], &glyph);
    return r_id;
}
#endif // #ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS

bool ImFontAtlas::GetCustomRect(ImFontAtlasRectId id, ImFontAtlasRect* out_r) const
{
    ImTextureRect* r = ImFontAtlasPackGetRectSafe((ImFontAtlas*)this, id);
    if (r == NULL)
        return false;
    IM_ASSERT(TexData->Width > 0 && TexData->Height > 0);   // Font atlas needs to be built before we can calculate UV coordinates
    if (out_r == NULL)
        return true;
    out_r->x = r->x;
    out_r->y = r->y;
    out_r->w = r->w;
    out_r->h = r->h;
    out_r->uv0 = ImVec2((float)(r->x), (float)(r->y)) * TexUvScale;
    out_r->uv1 = ImVec2((float)(r->x + r->w), (float)(r->y + r->h)) * TexUvScale;
    return true;
}

bool ImFontAtlasGetMouseCursorTexData(ImFontAtlas* atlas, ImGuiMouseCursor cursor_type, ImVec2* out_offset, ImVec2* out_size, ImVec2 out_uv_border[2], ImVec2 out_uv_fill[2])
{
    if (cursor_type <= ImGuiMouseCursor_None || cursor_type >= ImGuiMouseCursor_COUNT)
        return false;
    if (atlas->Flags & ImFontAtlasFlags_NoMouseCursors)
        return false;

    ImTextureRect* r = ImFontAtlasPackGetRect(atlas, atlas->Builder->PackIdMouseCursors);
    ImVec2 pos = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][0] + ImVec2((float)r->x, (float)r->y);
    ImVec2 size = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][1];
    *out_size = size;
    *out_offset = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][2];
    out_uv_border[0] = (pos) * atlas->TexUvScale;
    out_uv_border[1] = (pos + size) * atlas->TexUvScale;
    pos.x += FONT_ATLAS_DEFAULT_TEX_DATA_W + 1;
    out_uv_fill[0] = (pos) * atlas->TexUvScale;
    out_uv_fill[1] = (pos + size) * atlas->TexUvScale;
    return true;
}

// When atlas->RendererHasTextures = true, this is only called if no font were loaded.
void ImFontAtlasBuildMain(ImFontAtlas* atlas)
{
    IM_ASSERT(!atlas->Locked && "Cannot modify a locked ImFontAtlas!");
    if (atlas->TexData && atlas->TexData->Format != atlas->TexDesiredFormat)
        ImFontAtlasBuildClear(atlas);

    if (atlas->Builder == NULL)
        ImFontAtlasBuildInit(atlas);

    // Default font is none are specified
    if (atlas->Sources.Size == 0)
        atlas->AddFontDefault();

    // [LEGACY] For backends not supporting RendererHasTextures: preload all glyphs
    ImFontAtlasBuildUpdateRendererHasTexturesFromContext(atlas);
    if (atlas->RendererHasTextures == false) // ~ImGuiBackendFlags_RendererHasTextures
        ImFontAtlasBuildLegacyPreloadAllGlyphRanges(atlas);
    atlas->TexIsBuilt = true;
}

void ImFontAtlasBuildGetOversampleFactors(ImFontConfig* src, ImFontBaked* baked, int* out_oversample_h, int* out_oversample_v)
{
    // Automatically disable horizontal oversampling over size 36
    const float raster_size = baked->Size * baked->RasterizerDensity * src->RasterizerDensity;
    *out_oversample_h = (src->OversampleH != 0) ? src->OversampleH : (raster_size > 36.0f || src->PixelSnapH) ? 1 : 2;
    *out_oversample_v = (src->OversampleV != 0) ? src->OversampleV : 1;
}

// Setup main font loader for the atlas
// Every font source (ImFontConfig) will use this unless ImFontConfig::FontLoader specify a custom loader.
void ImFontAtlasBuildSetupFontLoader(ImFontAtlas* atlas, const ImFontLoader* font_loader)
{
    if (atlas->FontLoader == font_loader)
        return;
    IM_ASSERT(!atlas->Locked && "Cannot modify a locked ImFontAtlas!");

    for (ImFont* font : atlas->Fonts)
        ImFontAtlasFontDestroyOutput(atlas, font);
    if (atlas->Builder && atlas->FontLoader && atlas->FontLoader->LoaderShutdown)
        atlas->FontLoader->LoaderShutdown(atlas);

    atlas->FontLoader = font_loader;
    atlas->FontLoaderName = font_loader ? font_loader->Name : "NULL";
    IM_ASSERT(atlas->FontLoaderData == NULL);

    if (atlas->Builder && atlas->FontLoader && atlas->FontLoader->LoaderInit)
        atlas->FontLoader->LoaderInit(atlas);
    for (ImFont* font : atlas->Fonts)
        ImFontAtlasFontInitOutput(atlas, font);
    for (ImFont* font : atlas->Fonts)
        for (ImFontConfig* src : font->Sources)
            ImFontAtlasFontSourceAddToFont(atlas, font, src);
}

// Preload all glyph ranges for legacy backends.
// This may lead to multiple texture creation which might be a little slower than before.
void ImFontAtlasBuildLegacyPreloadAllGlyphRanges(ImFontAtlas* atlas)
{
    atlas->Builder->PreloadedAllGlyphsRanges = true;
    for (ImFont* font : atlas->Fonts)
    {
        ImFontBaked* baked = font->GetFontBaked(font->LegacySize);
        if (font->FallbackChar != 0)
            baked->FindGlyph(font->FallbackChar);
        if (font->EllipsisChar != 0)
            baked->FindGlyph(font->EllipsisChar);
        for (ImFontConfig* src : font->Sources)
        {
            const ImWchar* ranges = src->GlyphRanges ? src->GlyphRanges : atlas->GetGlyphRangesDefault();
            for (; ranges[0]; ranges += 2)
                for (unsigned int c = ranges[0]; c <= ranges[1] && c <= IM_UNICODE_CODEPOINT_MAX; c++) //-V560
                    baked->FindGlyph((ImWchar)c);
        }
    }
}

// FIXME: May make ImFont::Sources a ImSpan<> and move ownership to ImFontAtlas
void ImFontAtlasBuildUpdatePointers(ImFontAtlas* atlas)
{
    for (ImFont* font : atlas->Fonts)
        font->Sources.resize(0);
    for (ImFontConfig& src : atlas->Sources)
        src.DstFont->Sources.push_back(&src);
}

// Render a white-colored bitmap encoded in a string
void ImFontAtlasBuildRenderBitmapFromString(ImFontAtlas* atlas, int x, int y, int w, int h, const char* in_str, char in_marker_char)
{
    ImTextureData* tex = atlas->TexData;
    IM_ASSERT(x >= 0 && x + w <= tex->Width);
    IM_ASSERT(y >= 0 && y + h <= tex->Height);

    switch (tex->Format)
    {
    case ImTextureFormat_Alpha8:
    {
        ImU8* out_p = (ImU8*)tex->GetPixelsAt(x, y);
        for (int off_y = 0; off_y < h; off_y++, out_p += tex->Width, in_str += w)
            for (int off_x = 0; off_x < w; off_x++)
                out_p[off_x] = (in_str[off_x] == in_marker_char) ? 0xFF : 0x00;
        break;
    }
    case ImTextureFormat_RGBA32:
    {
        ImU32* out_p = (ImU32*)tex->GetPixelsAt(x, y);
        for (int off_y = 0; off_y < h; off_y++, out_p += tex->Width, in_str += w)
            for (int off_x = 0; off_x < w; off_x++)
                out_p[off_x] = (in_str[off_x] == in_marker_char) ? IM_COL32_WHITE : IM_COL32_BLACK_TRANS;
        break;
    }
    }
}

static void ImFontAtlasBuildUpdateBasicTexData(ImFontAtlas* atlas)
{
    // Pack and store identifier so we can refresh UV coordinates on texture resize.
    // FIXME-NEWATLAS: User/custom rects where user code wants to store UV coordinates will need to do the same thing.
    ImFontAtlasBuilder* builder = atlas->Builder;
    ImVec2i pack_size = (atlas->Flags & ImFontAtlasFlags_NoMouseCursors) ? ImVec2i(2, 2) : ImVec2i(FONT_ATLAS_DEFAULT_TEX_DATA_W * 2 + 1, FONT_ATLAS_DEFAULT_TEX_DATA_H);

    ImFontAtlasRect r;
    bool add_and_draw = (atlas->GetCustomRect(builder->PackIdMouseCursors, &r) == false);
    if (add_and_draw)
    {
        builder->PackIdMouseCursors = atlas->AddCustomRect(pack_size.x, pack_size.y, &r);
        IM_ASSERT(builder->PackIdMouseCursors != ImFontAtlasRectId_Invalid);

        // Draw to texture
        if (atlas->Flags & ImFontAtlasFlags_NoMouseCursors)
        {
            // 2x2 white pixels
            ImFontAtlasBuildRenderBitmapFromString(atlas, r.x, r.y, 2, 2, "XX" "XX", 'X');
        }
        else
        {
            // 2x2 white pixels + mouse cursors
            const int x_for_white = r.x;
            const int x_for_black = r.x + FONT_ATLAS_DEFAULT_TEX_DATA_W + 1;
            ImFontAtlasBuildRenderBitmapFromString(atlas, x_for_white, r.y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, '.');
            ImFontAtlasBuildRenderBitmapFromString(atlas, x_for_black, r.y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, 'X');
        }
    }

    // Refresh UV coordinates
    atlas->TexUvWhitePixel = ImVec2((r.x + 0.5f) * atlas->TexUvScale.x, (r.y + 0.5f) * atlas->TexUvScale.y);
}

static void ImFontAtlasBuildUpdateLinesTexData(ImFontAtlas* atlas)
{
    if (atlas->Flags & ImFontAtlasFlags_NoBakedLines)
        return;

    // Pack and store identifier so we can refresh UV coordinates on texture resize.
    ImTextureData* tex = atlas->TexData;
    ImFontAtlasBuilder* builder = atlas->Builder;

    ImFontAtlasRect r;
    bool add_and_draw = atlas->GetCustomRect(builder->PackIdLinesTexData, &r) == false;
    if (add_and_draw)
    {
        ImVec2i pack_size = ImVec2i(IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 2, IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 1);
        builder->PackIdLinesTexData = atlas->AddCustomRect(pack_size.x, pack_size.y, &r);
        IM_ASSERT(builder->PackIdLinesTexData != ImFontAtlasRectId_Invalid);
    }

    // Register texture region for thick lines
    // The +2 here is to give space for the end caps, whilst height +1 is to accommodate the fact we have a zero-width row
    // This generates a triangular shape in the texture, with the various line widths stacked on top of each other to allow interpolation between them
    for (int n = 0; n < IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 1; n++) // +1 because of the zero-width row
    {
        // Each line consists of at least two empty pixels at the ends, with a line of solid pixels in the middle
        const int y = n;
        const int line_width = n;
        const int pad_left = (r.w - line_width) / 2;
        const int pad_right = r.w - (pad_left + line_width);
        IM_ASSERT(pad_left + line_width + pad_right == r.w && y < r.h); // Make sure we're inside the texture bounds before we start writing pixels

        // Write each slice
        if (add_and_draw && tex->Format == ImTextureFormat_Alpha8)
        {
            ImU8* write_ptr = (ImU8*)tex->GetPixelsAt(r.x, r.y + y);
            for (int i = 0; i < pad_left; i++)
                *(write_ptr + i) = 0x00;

            for (int i = 0; i < line_width; i++)
                *(write_ptr + pad_left + i) = 0xFF;

            for (int i = 0; i < pad_right; i++)
                *(write_ptr + pad_left + line_width + i) = 0x00;
        }
        else if (add_and_draw && tex->Format == ImTextureFormat_RGBA32)
        {
            ImU32* write_ptr = (ImU32*)(void*)tex->GetPixelsAt(r.x, r.y + y);
            for (int i = 0; i < pad_left; i++)
                *(write_ptr + i) = IM_COL32(255, 255, 255, 0);

            for (int i = 0; i < line_width; i++)
                *(write_ptr + pad_left + i) = IM_COL32_WHITE;

            for (int i = 0; i < pad_right; i++)
                *(write_ptr + pad_left + line_width + i) = IM_COL32(255, 255, 255, 0);
        }

        // Refresh UV coordinates
        ImVec2 uv0 = ImVec2((float)(r.x + pad_left - 1), (float)(r.y + y)) * atlas->TexUvScale;
        ImVec2 uv1 = ImVec2((float)(r.x + pad_left + line_width + 1), (float)(r.y + y + 1)) * atlas->TexUvScale;
        float half_v = (uv0.y + uv1.y) * 0.5f; // Calculate a constant V in the middle of the row to avoid sampling artifacts
        atlas->TexUvLines[n] = ImVec4(uv0.x, half_v, uv1.x, half_v);
    }
}

//-----------------------------------------------------------------------------------------------------------------------------

// Was tempted to lazily init FontSrc but wouldn't save much + makes it more complicated to detect invalid data at AddFont()
bool ImFontAtlasFontInitOutput(ImFontAtlas* atlas, ImFont* font)
{
    bool ret = true;
    for (ImFontConfig* src : font->Sources)
        if (!ImFontAtlasFontSourceInit(atlas, src))
            ret = false;
    IM_ASSERT(ret); // Unclear how to react to this meaningfully. Assume that result will be same as initial AddFont() call.
    return ret;
}

// Keep source/input FontData
void ImFontAtlasFontDestroyOutput(ImFontAtlas* atlas, ImFont* font)
{
    font->ClearOutputData();
    for (ImFontConfig* src : font->Sources)
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        if (loader && loader->FontSrcDestroy != NULL)
            loader->FontSrcDestroy(atlas, src);
    }
}

//-----------------------------------------------------------------------------------------------------------------------------

bool ImFontAtlasFontSourceInit(ImFontAtlas* atlas, ImFontConfig* src)
{
    const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
    if (loader->FontSrcInit != NULL && !loader->FontSrcInit(atlas, src))
        return false;
    return true;
}

void ImFontAtlasFontSourceAddToFont(ImFontAtlas* atlas, ImFont* font, ImFontConfig* src)
{
    if (src->MergeMode == false)
    {
        font->ClearOutputData();
        //font->FontSize = src->SizePixels;
        font->ContainerAtlas = atlas;
        IM_ASSERT(font->Sources[0] == src);
    }
    atlas->TexIsBuilt = false; // For legacy backends
    ImFontAtlasBuildSetupFontSpecialGlyphs(atlas, font, src);
}

void ImFontAtlasFontDestroySourceData(ImFontAtlas* atlas, ImFontConfig* src)
{
    IM_UNUSED(atlas);
    if (src->FontDataOwnedByAtlas)
        IM_FREE(src->FontData);
    src->FontData = NULL;
    if (src->GlyphExcludeRanges)
        IM_FREE((void*)src->GlyphExcludeRanges);
    src->GlyphExcludeRanges = NULL;
}

// Create a compact, baked "..." if it doesn't exist, by using the ".".
// This may seem overly complicated right now but the point is to exercise and improve a technique which should be increasingly used.
// FIXME-NEWATLAS: This borrows too much from FontLoader's FontLoadGlyph() handlers and suggest that we should add further helpers.
static ImFontGlyph* ImFontAtlasBuildSetupFontBakedEllipsis(ImFontAtlas* atlas, ImFontBaked* baked)
{
    ImFont* font = baked->ContainerFont;
    IM_ASSERT(font->EllipsisChar != 0);

    const ImFontGlyph* dot_glyph = baked->FindGlyphNoFallback((ImWchar)'.');
    if (dot_glyph == NULL)
        dot_glyph = baked->FindGlyphNoFallback((ImWchar)0xFF0E);
    if (dot_glyph == NULL)
        return NULL;
    ImFontAtlasRectId dot_r_id = dot_glyph->PackId; // Deep copy to avoid invalidation of glyphs and rect pointers
    ImTextureRect* dot_r = ImFontAtlasPackGetRect(atlas, dot_r_id);
    const int dot_spacing = 1;
    const float dot_step = (dot_glyph->X1 - dot_glyph->X0) + dot_spacing;

    ImFontAtlasRectId pack_id = ImFontAtlasPackAddRect(atlas, (dot_r->w * 3 + dot_spacing * 2), dot_r->h);
    ImTextureRect* r = ImFontAtlasPackGetRect(atlas, pack_id);

    ImFontGlyph glyph_in = {};
    ImFontGlyph* glyph = &glyph_in;
    glyph->Codepoint = font->EllipsisChar;
    glyph->AdvanceX = ImMax(dot_glyph->AdvanceX, dot_glyph->X0 + dot_step * 3.0f - dot_spacing); // FIXME: Slightly odd for normally mono-space fonts but since this is used for trailing contents.
    glyph->X0 = dot_glyph->X0;
    glyph->Y0 = dot_glyph->Y0;
    glyph->X1 = dot_glyph->X0 + dot_step * 3 - dot_spacing;
    glyph->Y1 = dot_glyph->Y1;
    glyph->Visible = true;
    glyph->PackId = pack_id;
    glyph = ImFontAtlasBakedAddFontGlyph(atlas, baked, NULL, glyph);
    dot_glyph = NULL; // Invalidated

    // Copy to texture, post-process and queue update for backend
    // FIXME-NEWATLAS-V2: Dot glyph is already post-processed as this point, so this would damage it.
    dot_r = ImFontAtlasPackGetRect(atlas, dot_r_id);
    ImTextureData* tex = atlas->TexData;
    for (int n = 0; n < 3; n++)
        ImFontAtlasTextureBlockCopy(tex, dot_r->x, dot_r->y, tex, r->x + (dot_r->w + dot_spacing) * n, r->y, dot_r->w, dot_r->h);
    ImFontAtlasTextureBlockQueueUpload(atlas, tex, r->x, r->y, r->w, r->h);

    return glyph;
}

// Load fallback in order to obtain its index
// (this is called from in hot-path so we avoid extraneous parameters to minimize impact on code size)
static void ImFontAtlasBuildSetupFontBakedFallback(ImFontBaked* baked)
{
    IM_ASSERT(baked->FallbackGlyphIndex == -1);
    IM_ASSERT(baked->FallbackAdvanceX == 0.0f);
    ImFont* font = baked->ContainerFont;
    ImFontGlyph* fallback_glyph = NULL;
    if (font->FallbackChar != 0)
        fallback_glyph = baked->FindGlyphNoFallback(font->FallbackChar);
    if (fallback_glyph == NULL)
    {
        ImFontGlyph* space_glyph = baked->FindGlyphNoFallback((ImWchar)' ');
        ImFontGlyph glyph;
        glyph.Codepoint = 0;
        glyph.AdvanceX = space_glyph ? space_glyph->AdvanceX : IM_ROUND(baked->Size * 0.40f);
        fallback_glyph = ImFontAtlasBakedAddFontGlyph(font->ContainerAtlas, baked, NULL, &glyph);
    }
    baked->FallbackGlyphIndex = baked->Glyphs.index_from_ptr(fallback_glyph); // Storing index avoid need to update pointer on growth and simplify inner loop code
    baked->FallbackAdvanceX = fallback_glyph->AdvanceX;
}

static void ImFontAtlasBuildSetupFontBakedBlanks(ImFontAtlas* atlas, ImFontBaked* baked)
{
    // Mark space as always hidden (not strictly correct/necessary. but some e.g. icons fonts don't have a space. it tends to look neater in previews)
    ImFontGlyph* space_glyph = baked->FindGlyphNoFallback((ImWchar)' ');
    if (space_glyph != NULL)
        space_glyph->Visible = false;

    // Setup Tab character.
    // FIXME: Needs proper TAB handling but it needs to be contextualized (or we could arbitrary say that each string starts at "column 0" ?)
    if (baked->FindGlyphNoFallback('\t') == NULL && space_glyph != NULL)
    {
        ImFontGlyph tab_glyph;
        tab_glyph.Codepoint = '\t';
        tab_glyph.AdvanceX = space_glyph->AdvanceX * IM_TABSIZE;
        ImFontAtlasBakedAddFontGlyph(atlas, baked, NULL, &tab_glyph);
    }
}

// Load/identify special glyphs
// (note that this is called again for fonts with MergeMode)
void ImFontAtlasBuildSetupFontSpecialGlyphs(ImFontAtlas* atlas, ImFont* font, ImFontConfig* src)
{
    IM_UNUSED(atlas);
    IM_ASSERT(font->Sources.contains(src));

    // Find Fallback character. Actual glyph loaded in GetFontBaked().
    const ImWchar fallback_chars[] = { font->FallbackChar, (ImWchar)IM_UNICODE_CODEPOINT_INVALID, (ImWchar)'?', (ImWchar)' ' };
    if (font->FallbackChar == 0)
        for (ImWchar candidate_char : fallback_chars)
            if (candidate_char != 0 && font->IsGlyphInFont(candidate_char))
            {
                font->FallbackChar = (ImWchar)candidate_char;
                break;
            }

    // Setup Ellipsis character. It is required for rendering elided text. We prefer using U+2026 (horizontal ellipsis).
    // However some old fonts may contain ellipsis at U+0085. Here we auto-detect most suitable ellipsis character.
    // FIXME: Note that 0x2026 is rarely included in our font ranges. Because of this we are more likely to use three individual dots.
    const ImWchar ellipsis_chars[] = { src->EllipsisChar, (ImWchar)0x2026, (ImWchar)0x0085 };
    if (font->EllipsisChar == 0)
        for (ImWchar candidate_char : ellipsis_chars)
            if (candidate_char != 0 && font->IsGlyphInFont(candidate_char))
            {
                font->EllipsisChar = candidate_char;
                break;
            }
    if (font->EllipsisChar == 0)
    {
        font->EllipsisChar = 0x0085;
        font->EllipsisAutoBake = true;
    }
}

void ImFontAtlasBakedDiscardFontGlyph(ImFontAtlas* atlas, ImFont* font, ImFontBaked* baked, ImFontGlyph* glyph)
{
    if (glyph->PackId != ImFontAtlasRectId_Invalid)
    {
        ImFontAtlasPackDiscardRect(atlas, glyph->PackId);
        glyph->PackId = ImFontAtlasRectId_Invalid;
    }
    ImWchar c = (ImWchar)glyph->Codepoint;
    IM_ASSERT(font->FallbackChar != c && font->EllipsisChar != c); // Unsupported for simplicity
    IM_ASSERT(glyph >= baked->Glyphs.Data && glyph < baked->Glyphs.Data + baked->Glyphs.Size);
    IM_UNUSED(font);
    baked->IndexLookup[c] = IM_FONTGLYPH_INDEX_UNUSED;
    baked->IndexAdvanceX[c] = baked->FallbackAdvanceX;
}

ImFontBaked* ImFontAtlasBakedAdd(ImFontAtlas* atlas, ImFont* font, float font_size, float font_rasterizer_density, ImGuiID baked_id)
{
    IMGUI_DEBUG_LOG_FONT("[font] Created baked %.2fpx\n", font_size);
    ImFontBaked* baked = atlas->Builder->BakedPool.push_back(ImFontBaked());
    baked->Size = font_size;
    baked->RasterizerDensity = font_rasterizer_density;
    baked->BakedId = baked_id;
    baked->ContainerFont = font;
    baked->LastUsedFrame = atlas->Builder->FrameCount;

    // Initialize backend data
    size_t loader_data_size = 0;
    for (ImFontConfig* src : font->Sources) // Cannot easily be cached as we allow changing backend
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        loader_data_size += loader->FontBakedSrcLoaderDataSize;
    }
    baked->FontLoaderDatas = (loader_data_size > 0) ? IM_ALLOC(loader_data_size) : NULL;
    char* loader_data_p = (char*)baked->FontLoaderDatas;
    for (ImFontConfig* src : font->Sources)
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        if (loader->FontBakedInit)
            loader->FontBakedInit(atlas, src, baked, loader_data_p);
        loader_data_p += loader->FontBakedSrcLoaderDataSize;
    }

    ImFontAtlasBuildSetupFontBakedBlanks(atlas, baked);
    return baked;
}

// FIXME-OPT: This is not a fast query. Adding a BakedCount field in Font might allow to take a shortcut for the most common case.
ImFontBaked* ImFontAtlasBakedGetClosestMatch(ImFontAtlas* atlas, ImFont* font, float font_size, float font_rasterizer_density)
{
    ImFontAtlasBuilder* builder = atlas->Builder;
    for (int step_n = 0; step_n < 2; step_n++)
    {
        ImFontBaked* closest_larger_match = NULL;
        ImFontBaked* closest_smaller_match = NULL;
        for (int baked_n = 0; baked_n < builder->BakedPool.Size; baked_n++)
        {
            ImFontBaked* baked = &builder->BakedPool[baked_n];
            if (baked->ContainerFont != font || baked->WantDestroy)
                continue;
            if (step_n == 0 && baked->RasterizerDensity != font_rasterizer_density) // First try with same density
                continue;
            if (baked->Size > font_size && (closest_larger_match == NULL || baked->Size < closest_larger_match->Size))
                closest_larger_match = baked;
            if (baked->Size < font_size && (closest_smaller_match == NULL || baked->Size > closest_smaller_match->Size))
                closest_smaller_match = baked;
        }
        if (closest_larger_match)
            if (closest_smaller_match == NULL || (closest_larger_match->Size >= font_size * 2.0f && closest_smaller_match->Size > font_size * 0.5f))
                return closest_larger_match;
        if (closest_smaller_match)
            return closest_smaller_match;
    }
    return NULL;
}

void ImFontAtlasBakedDiscard(ImFontAtlas* atlas, ImFont* font, ImFontBaked* baked)
{
    ImFontAtlasBuilder* builder = atlas->Builder;
    IMGUI_DEBUG_LOG_FONT("[font] Discard baked %.2f for \"%s\"\n", baked->Size, font->GetDebugName());

    for (ImFontGlyph& glyph : baked->Glyphs)
        if (glyph.PackId != ImFontAtlasRectId_Invalid)
            ImFontAtlasPackDiscardRect(atlas, glyph.PackId);

    char* loader_data_p = (char*)baked->FontLoaderDatas;
    for (ImFontConfig* src : font->Sources)
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        if (loader->FontBakedDestroy)
            loader->FontBakedDestroy(atlas, src, baked, loader_data_p);
        loader_data_p += loader->FontBakedSrcLoaderDataSize;
    }
    if (baked->FontLoaderDatas)
    {
        IM_FREE(baked->FontLoaderDatas);
        baked->FontLoaderDatas = NULL;
    }
    builder->BakedMap.SetVoidPtr(baked->BakedId, NULL);
    builder->BakedDiscardedCount++;
    baked->ClearOutputData();
    baked->WantDestroy = true;
    font->LastBaked = NULL;
}

// use unused_frames==0 to discard everything.
void ImFontAtlasFontDiscardBakes(ImFontAtlas* atlas, ImFont* font, int unused_frames)
{
    if (ImFontAtlasBuilder* builder = atlas->Builder) // This can be called from font destructor
        for (int baked_n = 0; baked_n < builder->BakedPool.Size; baked_n++)
        {
            ImFontBaked* baked = &builder->BakedPool[baked_n];
            if (baked->LastUsedFrame + unused_frames > atlas->Builder->FrameCount)
                continue;
            if (baked->ContainerFont != font || baked->WantDestroy)
                continue;
            ImFontAtlasBakedDiscard(atlas, font, baked);
        }
}

// use unused_frames==0 to discard everything.
void ImFontAtlasBuildDiscardBakes(ImFontAtlas* atlas, int unused_frames)
{
    ImFontAtlasBuilder* builder = atlas->Builder;
    for (int baked_n = 0; baked_n < builder->BakedPool.Size; baked_n++)
    {
        ImFontBaked* baked = &builder->BakedPool[baked_n];
        if (baked->LastUsedFrame + unused_frames > atlas->Builder->FrameCount)
            continue;
        if (baked->WantDestroy || (baked->ContainerFont->Flags & ImFontFlags_LockBakedSizes))
            continue;
        ImFontAtlasBakedDiscard(atlas, baked->ContainerFont, baked);
    }
}

// Those functions are designed to facilitate changing the underlying structures for ImFontAtlas to store an array of ImDrawListSharedData*
void ImFontAtlasAddDrawListSharedData(ImFontAtlas* atlas, ImDrawListSharedData* data)
{
    IM_ASSERT(!atlas->DrawListSharedDatas.contains(data));
    atlas->DrawListSharedDatas.push_back(data);
}

void ImFontAtlasRemoveDrawListSharedData(ImFontAtlas* atlas, ImDrawListSharedData* data)
{
    IM_ASSERT(atlas->DrawListSharedDatas.contains(data));
    atlas->DrawListSharedDatas.find_erase(data);
}

// Update texture identifier in all active draw lists
void ImFontAtlasUpdateDrawListsTextures(ImFontAtlas* atlas, ImTextureRef old_tex, ImTextureRef new_tex)
{
    for (ImDrawListSharedData* shared_data : atlas->DrawListSharedDatas)
        for (ImDrawList* draw_list : shared_data->DrawLists)
        {
            // Replace in command-buffer
            // (there is not need to replace in ImDrawListSplitter: current channel is in ImDrawList's CmdBuffer[],
            //  other channels will be on SetCurrentChannel() which already needs to compare CmdHeader anyhow)
            if (draw_list->CmdBuffer.Size > 0 && draw_list->_CmdHeader.TexRef == old_tex)
                draw_list->_SetTexture(new_tex);

            // Replace in stack
            for (ImTextureRef& stacked_tex : draw_list->_TextureStack)
                if (stacked_tex == old_tex)
                    stacked_tex = new_tex;
        }
}

// Update texture coordinates in all draw list shared context
// FIXME-NEWATLAS FIXME-OPT: Doesn't seem necessary to update for all, only one bound to current context?
void ImFontAtlasUpdateDrawListsSharedData(ImFontAtlas* atlas)
{
    for (ImDrawListSharedData* shared_data : atlas->DrawListSharedDatas)
        if (shared_data->FontAtlas == atlas)
        {
            shared_data->TexUvWhitePixel = atlas->TexUvWhitePixel;
            shared_data->TexUvLines = atlas->TexUvLines;
        }
}

// Set current texture. This is mostly called from AddTexture() + to handle a failed resize.
static void ImFontAtlasBuildSetTexture(ImFontAtlas* atlas, ImTextureData* tex)
{
    ImTextureRef old_tex_ref = atlas->TexRef;
    atlas->TexData = tex;
    atlas->TexUvScale = ImVec2(1.0f / tex->Width, 1.0f / tex->Height);
    atlas->TexRef._TexData = tex;
    //atlas->TexRef._TexID = tex->TexID; // <-- We intentionally don't do that. It would be misleading and betray promise that both fields aren't set.
    ImFontAtlasUpdateDrawListsTextures(atlas, old_tex_ref, atlas->TexRef);
}

// Create a new texture, discard previous one
ImTextureData* ImFontAtlasTextureAdd(ImFontAtlas* atlas, int w, int h)
{
    ImTextureData* old_tex = atlas->TexData;
    ImTextureData* new_tex;

    // FIXME: Cannot reuse texture because old UV may have been used already (unless we remap UV).
    /*if (old_tex != NULL && old_tex->Status == ImTextureStatus_WantCreate)
    {
        // Reuse texture not yet used by backend.
        IM_ASSERT(old_tex->TexID == ImTextureID_Invalid && old_tex->BackendUserData == NULL);
        old_tex->DestroyPixels();
        old_tex->Updates.clear();
        new_tex = old_tex;
        old_tex = NULL;
    }
    else*/
    {
        // Add new
        new_tex = IM_NEW(ImTextureData)();
        new_tex->UniqueID = atlas->TexNextUniqueID++;
        atlas->TexList.push_back(new_tex);
    }
    if (old_tex != NULL)
    {
        // Queue old as to destroy next frame
        old_tex->WantDestroyNextFrame = true;
        IM_ASSERT(old_tex->Status == ImTextureStatus_OK || old_tex->Status == ImTextureStatus_WantCreate || old_tex->Status == ImTextureStatus_WantUpdates);
    }

    new_tex->Create(atlas->TexDesiredFormat, w, h);
    atlas->TexIsBuilt = false;

    ImFontAtlasBuildSetTexture(atlas, new_tex);

    return new_tex;
}

#if 0
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"
static void ImFontAtlasDebugWriteTexToDisk(ImTextureData* tex, const char* description)
{
    ImGuiContext& g = *GImGui;
    char buf[128];
    ImFormatString(buf, IM_ARRAYSIZE(buf), "[%05d] Texture #%03d - %s.png", g.FrameCount, tex->UniqueID, description);
    stbi_write_png(buf, tex->Width, tex->Height, tex->BytesPerPixel, tex->Pixels, tex->GetPitch()); // tex->BytesPerPixel is technically not component, but ok for the formats we support.
}
#endif

void ImFontAtlasTextureRepack(ImFontAtlas* atlas, int w, int h)
{
    ImFontAtlasBuilder* builder = atlas->Builder;
    builder->LockDisableResize = true;

    ImTextureData* old_tex = atlas->TexData;
    ImTextureData* new_tex = ImFontAtlasTextureAdd(atlas, w, h);
    new_tex->UseColors = old_tex->UseColors;
    IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: resize+repack %dx%d => Texture #%03d: %dx%d\n", old_tex->UniqueID, old_tex->Width, old_tex->Height, new_tex->UniqueID, new_tex->Width, new_tex->Height);
    //for (int baked_n = 0; baked_n < builder->BakedPool.Size; baked_n++)
    //    IMGUI_DEBUG_LOG_FONT("[font] - Baked %.2fpx, %d glyphs, want_destroy=%d\n", builder->BakedPool[baked_n].FontSize, builder->BakedPool[baked_n].Glyphs.Size, builder->BakedPool[baked_n].WantDestroy);
    //IMGUI_DEBUG_LOG_FONT("[font] - Old packed rects: %d, area %d px\n", builder->RectsPackedCount, builder->RectsPackedSurface);
    //ImFontAtlasDebugWriteTexToDisk(old_tex, "Before Pack");

    // Repack, lose discarded rectangle, copy pixels
    // FIXME-NEWATLAS: This is unstable because packing order is based on RectsIndex
    // FIXME-NEWATLAS-V2: Repacking in batch would be beneficial to packing heuristic, and fix stability.
    // FIXME-NEWATLAS-TESTS: Test calling RepackTexture with size too small to fits existing rects.
    ImFontAtlasPackInit(atlas);
    ImVector<ImTextureRect> old_rects;
    ImVector<ImFontAtlasRectEntry> old_index = builder->RectsIndex;
    old_rects.swap(builder->Rects);

    for (ImFontAtlasRectEntry& index_entry : builder->RectsIndex)
    {
        if (index_entry.IsUsed == false)
            continue;
        ImTextureRect& old_r = old_rects[index_entry.TargetIndex];
        if (old_r.w == 0 && old_r.h == 0)
            continue;
        ImFontAtlasRectId new_r_id = ImFontAtlasPackAddRect(atlas, old_r.w, old_r.h, &index_entry);
        if (new_r_id == ImFontAtlasRectId_Invalid)
        {
            // Undo, grow texture and try repacking again.
            // FIXME-NEWATLAS-TESTS: This is a very rarely exercised path! It needs to be automatically tested properly.
            IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: resize failed. Will grow.\n", new_tex->UniqueID);
            new_tex->WantDestroyNextFrame = true;
            builder->Rects.swap(old_rects);
            builder->RectsIndex = old_index;
            ImFontAtlasBuildSetTexture(atlas, old_tex);
            ImFontAtlasTextureGrow(atlas, w, h); // Recurse
            return;
        }
        IM_ASSERT(ImFontAtlasRectId_GetIndex(new_r_id) == builder->RectsIndex.index_from_ptr(&index_entry));
        ImTextureRect* new_r = ImFontAtlasPackGetRect(atlas, new_r_id);
        ImFontAtlasTextureBlockCopy(old_tex, old_r.x, old_r.y, new_tex, new_r->x, new_r->y, new_r->w, new_r->h);
    }
    IM_ASSERT(old_rects.Size == builder->Rects.Size + builder->RectsDiscardedCount);
    builder->RectsDiscardedCount = 0;
    builder->RectsDiscardedSurface = 0;

    // Patch glyphs UV
    for (int baked_n = 0; baked_n < builder->BakedPool.Size; baked_n++)
        for (ImFontGlyph& glyph : builder->BakedPool[baked_n].Glyphs)
            if (glyph.PackId != ImFontAtlasRectId_Invalid)
            {
                ImTextureRect* r = ImFontAtlasPackGetRect(atlas, glyph.PackId);
                glyph.U0 = (r->x) * atlas->TexUvScale.x;
                glyph.V0 = (r->y) * atlas->TexUvScale.y;
                glyph.U1 = (r->x + r->w) * atlas->TexUvScale.x;
                glyph.V1 = (r->y + r->h) * atlas->TexUvScale.y;
            }

    // Update other cached UV
    ImFontAtlasBuildUpdateLinesTexData(atlas);
    ImFontAtlasBuildUpdateBasicTexData(atlas);

    builder->LockDisableResize = false;
    ImFontAtlasUpdateDrawListsSharedData(atlas);
    //ImFontAtlasDebugWriteTexToDisk(new_tex, "After Pack");
}

void ImFontAtlasTextureGrow(ImFontAtlas* atlas, int old_tex_w, int old_tex_h)
{
    //ImFontAtlasDebugWriteTexToDisk(atlas->TexData, "Before Grow");
    ImFontAtlasBuilder* builder = atlas->Builder;
    if (old_tex_w == -1)
        old_tex_w = atlas->TexData->Width;
    if (old_tex_h == -1)
        old_tex_h = atlas->TexData->Height;

    // FIXME-NEWATLAS-V2: What to do when reaching limits exposed by backend?
    // FIXME-NEWATLAS-V2: Does ImFontAtlasFlags_NoPowerOfTwoHeight makes sense now? Allow 'lock' and 'compact' operations?
    IM_ASSERT(ImIsPowerOfTwo(old_tex_w) && ImIsPowerOfTwo(old_tex_h));
    IM_ASSERT(ImIsPowerOfTwo(atlas->TexMinWidth) && ImIsPowerOfTwo(atlas->TexMaxWidth) && ImIsPowerOfTwo(atlas->TexMinHeight) && ImIsPowerOfTwo(atlas->TexMaxHeight));

    // Grow texture so it follows roughly a square.
    // - Grow height before width, as width imply more packing nodes.
    // - Caller should be taking account of RectsDiscardedSurface and may not need to grow.
    int new_tex_w = (old_tex_h <= old_tex_w) ? old_tex_w : old_tex_w * 2;
    int new_tex_h = (old_tex_h <= old_tex_w) ? old_tex_h * 2 : old_tex_h;

    // Handle minimum size first (for pathologically large packed rects)
    const int pack_padding = atlas->TexGlyphPadding;
    new_tex_w = ImMax(new_tex_w, ImUpperPowerOfTwo(builder->MaxRectSize.x + pack_padding));
    new_tex_h = ImMax(new_tex_h, ImUpperPowerOfTwo(builder->MaxRectSize.y + pack_padding));
    new_tex_w = ImClamp(new_tex_w, atlas->TexMinWidth, atlas->TexMaxWidth);
    new_tex_h = ImClamp(new_tex_h, atlas->TexMinHeight, atlas->TexMaxHeight);
    if (new_tex_w == old_tex_w && new_tex_h == old_tex_h)
        return;

    ImFontAtlasTextureRepack(atlas, new_tex_w, new_tex_h);
}

void ImFontAtlasTextureMakeSpace(ImFontAtlas* atlas)
{
    // Can some baked contents be ditched?
    //IMGUI_DEBUG_LOG_FONT("[font] ImFontAtlasBuildMakeSpace()\n");
    ImFontAtlasBuilder* builder = atlas->Builder;
    ImFontAtlasBuildDiscardBakes(atlas, 2);

    // Currently using a heuristic for repack without growing.
    if (builder->RectsDiscardedSurface < builder->RectsPackedSurface * 0.20f)
        ImFontAtlasTextureGrow(atlas);
    else
        ImFontAtlasTextureRepack(atlas, atlas->TexData->Width, atlas->TexData->Height);
}

ImVec2i ImFontAtlasTextureGetSizeEstimate(ImFontAtlas* atlas)
{
    int min_w = ImUpperPowerOfTwo(atlas->TexMinWidth);
    int min_h = ImUpperPowerOfTwo(atlas->TexMinHeight);
    if (atlas->Builder == NULL || atlas->TexData == NULL || atlas->TexData->Status == ImTextureStatus_WantDestroy)
        return ImVec2i(min_w, min_h);

    ImFontAtlasBuilder* builder = atlas->Builder;
    min_w = ImMax(ImUpperPowerOfTwo(builder->MaxRectSize.x), min_w);
    min_h = ImMax(ImUpperPowerOfTwo(builder->MaxRectSize.y), min_h);
    const int surface_approx = builder->RectsPackedSurface - builder->RectsDiscardedSurface; // Expected surface after repack
    const int surface_sqrt = (int)sqrtf((float)surface_approx);

    int new_tex_w;
    int new_tex_h;
    if (min_w >= min_h)
    {
        new_tex_w = ImMax(min_w, ImUpperPowerOfTwo(surface_sqrt));
        new_tex_h = ImMax(min_h, (int)((surface_approx + new_tex_w - 1) / new_tex_w));
        if ((atlas->Flags & ImFontAtlasFlags_NoPowerOfTwoHeight) == 0)
            new_tex_h = ImUpperPowerOfTwo(new_tex_h);
    }
    else
    {
        new_tex_h = ImMax(min_h, ImUpperPowerOfTwo(surface_sqrt));
        if ((atlas->Flags & ImFontAtlasFlags_NoPowerOfTwoHeight) == 0)
            new_tex_h = ImUpperPowerOfTwo(new_tex_h);
        new_tex_w = ImMax(min_w, (int)((surface_approx + new_tex_h - 1) / new_tex_h));
    }

    IM_ASSERT(ImIsPowerOfTwo(new_tex_w) && ImIsPowerOfTwo(new_tex_h));
    return ImVec2i(new_tex_w, new_tex_h);
}

// Clear all output. Invalidates all AddCustomRect() return values!
void ImFontAtlasBuildClear(ImFontAtlas* atlas)
{
    ImVec2i new_tex_size = ImFontAtlasTextureGetSizeEstimate(atlas);
    ImFontAtlasBuildDestroy(atlas);
    ImFontAtlasTextureAdd(atlas, new_tex_size.x, new_tex_size.y);
    ImFontAtlasBuildInit(atlas);
    for (ImFontConfig& src : atlas->Sources)
        ImFontAtlasFontSourceInit(atlas, &src);
    for (ImFont* font : atlas->Fonts)
        for (ImFontConfig* src : font->Sources)
            ImFontAtlasFontSourceAddToFont(atlas, font, src);
}

// You should not need to call this manually!
// If you think you do, let us know and we can advise about policies auto-compact.
void ImFontAtlasTextureCompact(ImFontAtlas* atlas)
{
    ImFontAtlasBuilder* builder = atlas->Builder;
    ImFontAtlasBuildDiscardBakes(atlas, 1);

    ImTextureData* old_tex = atlas->TexData;
    ImVec2i old_tex_size = ImVec2i(old_tex->Width, old_tex->Height);
    ImVec2i new_tex_size = ImFontAtlasTextureGetSizeEstimate(atlas);
    if (builder->RectsDiscardedCount == 0 && new_tex_size.x == old_tex_size.x && new_tex_size.y == old_tex_size.y)
        return;

    ImFontAtlasTextureRepack(atlas, new_tex_size.x, new_tex_size.y);
}

// Start packing over current empty texture
void ImFontAtlasBuildInit(ImFontAtlas* atlas)
{
    // Select Backend
    // - Note that we do not reassign to atlas->FontLoader, since it is likely to point to static data which
    //   may mess with some hot-reloading schemes. If you need to assign to this (for dynamic selection) AND are
    //   using a hot-reloading scheme that messes up static data, store your own instance of FontLoader somewhere
    //   and point to it instead of pointing directly to return value of the GetFontLoaderXXX functions.
    if (atlas->FontLoader == NULL)
    {
#ifdef IMGUI_ENABLE_FREETYPE
        atlas->SetFontLoader(ImGuiFreeType::GetFontLoader());
#elif defined(IMGUI_ENABLE_STB_TRUETYPE)
        atlas->SetFontLoader(ImFontAtlasGetFontLoaderForStbTruetype());
#else
        IM_ASSERT(0); // Invalid Build function
#endif
    }

    // Create initial texture size
    if (atlas->TexData == NULL || atlas->TexData->Pixels == NULL)
        ImFontAtlasTextureAdd(atlas, ImUpperPowerOfTwo(atlas->TexMinWidth), ImUpperPowerOfTwo(atlas->TexMinHeight));

    atlas->Builder = IM_NEW(ImFontAtlasBuilder)();
    if (atlas->FontLoader->LoaderInit)
        atlas->FontLoader->LoaderInit(atlas);

    ImFontAtlasBuildUpdateRendererHasTexturesFromContext(atlas);

    ImFontAtlasPackInit(atlas);

    // Add required texture data
    ImFontAtlasBuildUpdateLinesTexData(atlas);
    ImFontAtlasBuildUpdateBasicTexData(atlas);

    // Register fonts
    ImFontAtlasBuildUpdatePointers(atlas);

    // Update UV coordinates etc. stored in bound ImDrawListSharedData instance
    ImFontAtlasUpdateDrawListsSharedData(atlas);

    //atlas->TexIsBuilt = true;
}

// Destroy builder and all cached glyphs. Do not destroy actual fonts.
void ImFontAtlasBuildDestroy(ImFontAtlas* atlas)
{
    for (ImFont* font : atlas->Fonts)
        ImFontAtlasFontDestroyOutput(atlas, font);
    if (atlas->Builder && atlas->FontLoader && atlas->FontLoader->LoaderShutdown)
    {
        atlas->FontLoader->LoaderShutdown(atlas);
        IM_ASSERT(atlas->FontLoaderData == NULL);
    }
    IM_DELETE(atlas->Builder);
    atlas->Builder = NULL;
}

void ImFontAtlasPackInit(ImFontAtlas * atlas)
{
    ImTextureData* tex = atlas->TexData;
    ImFontAtlasBuilder* builder = atlas->Builder;

    // In theory we could decide to reduce the number of nodes, e.g. halve them, and waste a little texture space, but it doesn't seem worth it.
    const int pack_node_count = tex->Width / 2;
    builder->PackNodes.resize(pack_node_count);
    IM_STATIC_ASSERT(sizeof(stbrp_context) <= sizeof(stbrp_context_opaque));
    stbrp_init_target((stbrp_context*)(void*)&builder->PackContext, tex->Width, tex->Height, builder->PackNodes.Data, builder->PackNodes.Size);
    builder->RectsPackedSurface = builder->RectsPackedCount = 0;
    builder->MaxRectSize = ImVec2i(0, 0);
    builder->MaxRectBounds = ImVec2i(0, 0);
}

// This is essentially a free-list pattern, it may be nice to wrap it into a dedicated type.
static ImFontAtlasRectId ImFontAtlasPackAllocRectEntry(ImFontAtlas* atlas, int rect_idx)
{
    ImFontAtlasBuilder* builder = (ImFontAtlasBuilder*)atlas->Builder;
    int index_idx;
    ImFontAtlasRectEntry* index_entry;
    if (builder->RectsIndexFreeListStart < 0)
    {
        builder->RectsIndex.resize(builder->RectsIndex.Size + 1);
        index_idx = builder->RectsIndex.Size - 1;
        index_entry = &builder->RectsIndex[index_idx];
        memset(index_entry, 0, sizeof(*index_entry));
    }
    else
    {
        index_idx = builder->RectsIndexFreeListStart;
        index_entry = &builder->RectsIndex[index_idx];
        IM_ASSERT(index_entry->IsUsed == false && index_entry->Generation > 0); // Generation is incremented during DiscardRect
        builder->RectsIndexFreeListStart = index_entry->TargetIndex;
    }
    index_entry->TargetIndex = rect_idx;
    index_entry->IsUsed = 1;
    return ImFontAtlasRectId_Make(index_idx, index_entry->Generation);
}

// Overwrite existing entry
static ImFontAtlasRectId ImFontAtlasPackReuseRectEntry(ImFontAtlas* atlas, ImFontAtlasRectEntry* index_entry)
{
    IM_ASSERT(index_entry->IsUsed);
    index_entry->TargetIndex = atlas->Builder->Rects.Size - 1;
    int index_idx = atlas->Builder->RectsIndex.index_from_ptr(index_entry);
    return ImFontAtlasRectId_Make(index_idx, index_entry->Generation);
}

// This is expected to be called in batches and followed by a repack
void ImFontAtlasPackDiscardRect(ImFontAtlas* atlas, ImFontAtlasRectId id)
{
    IM_ASSERT(id != ImFontAtlasRectId_Invalid);

    ImTextureRect* rect = ImFontAtlasPackGetRect(atlas, id);
    if (rect == NULL)
        return;

    ImFontAtlasBuilder* builder = atlas->Builder;
    int index_idx = ImFontAtlasRectId_GetIndex(id);
    ImFontAtlasRectEntry* index_entry = &builder->RectsIndex[index_idx];
    IM_ASSERT(index_entry->IsUsed && index_entry->TargetIndex >= 0);
    index_entry->IsUsed = false;
    index_entry->TargetIndex = builder->RectsIndexFreeListStart;
    index_entry->Generation++;
    if (index_entry->Generation == 0)
        index_entry->Generation++; // Keep non-zero on overflow

    const int pack_padding = atlas->TexGlyphPadding;
    builder->RectsIndexFreeListStart = index_idx;
    builder->RectsDiscardedCount++;
    builder->RectsDiscardedSurface += (rect->w + pack_padding) * (rect->h + pack_padding);
    rect->w = rect->h = 0; // Clear rectangle so it won't be packed again
}

// Important: Calling this may recreate a new texture and therefore change atlas->TexData
// FIXME-NEWFONTS: Expose other glyph padding settings for custom alteration (e.g. drop shadows). See #7962
ImFontAtlasRectId ImFontAtlasPackAddRect(ImFontAtlas* atlas, int w, int h, ImFontAtlasRectEntry* overwrite_entry)
{
    IM_ASSERT(w > 0 && w <= 0xFFFF);
    IM_ASSERT(h > 0 && h <= 0xFFFF);

    ImFontAtlasBuilder* builder = (ImFontAtlasBuilder*)atlas->Builder;
    const int pack_padding = atlas->TexGlyphPadding;
    builder->MaxRectSize.x = ImMax(builder->MaxRectSize.x, w);
    builder->MaxRectSize.y = ImMax(builder->MaxRectSize.y, h);

    // Pack
    ImTextureRect r = { 0, 0, (unsigned short)w, (unsigned short)h };
    for (int attempts_remaining = 3; attempts_remaining >= 0; attempts_remaining--)
    {
        // Try packing
        stbrp_rect pack_r = {};
        pack_r.w = w + pack_padding;
        pack_r.h = h + pack_padding;
        stbrp_pack_rects((stbrp_context*)(void*)&builder->PackContext, &pack_r, 1);
        r.x = (unsigned short)pack_r.x;
        r.y = (unsigned short)pack_r.y;
        if (pack_r.was_packed)
            break;

        // If we ran out of attempts, return fallback
        if (attempts_remaining == 0 || builder->LockDisableResize)
        {
            IMGUI_DEBUG_LOG_FONT("[font] Failed packing %dx%d rectangle. Returning fallback.\n", w, h);
            return ImFontAtlasRectId_Invalid;
        }

        // Resize or repack atlas! (this should be a rare event)
        ImFontAtlasTextureMakeSpace(atlas);
    }

    builder->MaxRectBounds.x = ImMax(builder->MaxRectBounds.x, r.x + r.w + pack_padding);
    builder->MaxRectBounds.y = ImMax(builder->MaxRectBounds.y, r.y + r.h + pack_padding);
    builder->RectsPackedCount++;
    builder->RectsPackedSurface += (w + pack_padding) * (h + pack_padding);

    builder->Rects.push_back(r);
    if (overwrite_entry != NULL)
        return ImFontAtlasPackReuseRectEntry(atlas, overwrite_entry); // Write into an existing entry instead of adding one (used during repack)
    else
        return ImFontAtlasPackAllocRectEntry(atlas, builder->Rects.Size - 1);
}

// Generally for non-user facing functions: assert on invalid ID.
ImTextureRect* ImFontAtlasPackGetRect(ImFontAtlas* atlas, ImFontAtlasRectId id)
{
    IM_ASSERT(id != ImFontAtlasRectId_Invalid);
    int index_idx = ImFontAtlasRectId_GetIndex(id);
    ImFontAtlasBuilder* builder = (ImFontAtlasBuilder*)atlas->Builder;
    ImFontAtlasRectEntry* index_entry = &builder->RectsIndex[index_idx];
    IM_ASSERT(index_entry->Generation == ImFontAtlasRectId_GetGeneration(id));
    IM_ASSERT(index_entry->IsUsed);
    return &builder->Rects[index_entry->TargetIndex];
}

// For user-facing functions: return NULL on invalid ID.
// Important: return pointer is valid until next call to AddRect(), e.g. FindGlyph(), CalcTextSize() can all potentially invalidate previous pointers.
ImTextureRect* ImFontAtlasPackGetRectSafe(ImFontAtlas* atlas, ImFontAtlasRectId id)
{
    if (id == ImFontAtlasRectId_Invalid)
        return NULL;
    int index_idx = ImFontAtlasRectId_GetIndex(id);
    if (atlas->Builder == NULL)
        ImFontAtlasBuildInit(atlas);
    ImFontAtlasBuilder* builder = (ImFontAtlasBuilder*)atlas->Builder;
    IM_MSVC_WARNING_SUPPRESS(28182); // Static Analysis false positive "warning C28182: Dereferencing NULL pointer 'builder'"
    if (index_idx >= builder->RectsIndex.Size)
        return NULL;
    ImFontAtlasRectEntry* index_entry = &builder->RectsIndex[index_idx];
    if (index_entry->Generation != ImFontAtlasRectId_GetGeneration(id) || !index_entry->IsUsed)
        return NULL;
    return &builder->Rects[index_entry->TargetIndex];
}

// Important! This assume by ImFontConfig::GlyphExcludeRanges[] is a SMALL ARRAY (e.g. <10 entries)
// Use "Input Glyphs Overlap Detection Tool" to display a list of glyphs provided by multiple sources in order to set this array up.
static bool ImFontAtlasBuildAcceptCodepointForSource(ImFontConfig* src, ImWchar codepoint)
{
    if (const ImWchar* exclude_list = src->GlyphExcludeRanges)
        for (; exclude_list[0] != 0; exclude_list += 2)
            if (codepoint >= exclude_list[0] && codepoint <= exclude_list[1])
                return false;
    return true;
}

static void ImFontBaked_BuildGrowIndex(ImFontBaked* baked, int new_size)
{
    IM_ASSERT(baked->IndexAdvanceX.Size == baked->IndexLookup.Size);
    if (new_size <= baked->IndexLookup.Size)
        return;
    baked->IndexAdvanceX.resize(new_size, -1.0f);
    baked->IndexLookup.resize(new_size, IM_FONTGLYPH_INDEX_UNUSED);
}

static void ImFontAtlas_FontHookRemapCodepoint(ImFontAtlas* atlas, ImFont* font, ImWchar* c)
{
    IM_UNUSED(atlas);
    if (font->RemapPairs.Data.Size != 0)
        *c = (ImWchar)font->RemapPairs.GetInt((ImGuiID)*c, (int)*c);
}

static ImFontGlyph* ImFontBaked_BuildLoadGlyph(ImFontBaked* baked, ImWchar codepoint, float* only_load_advance_x)
{
    ImFont* font = baked->ContainerFont;
    ImFontAtlas* atlas = font->ContainerAtlas;
    if (atlas->Locked || (font->Flags & ImFontFlags_NoLoadGlyphs))
    {
        // Lazily load fallback glyph
        if (baked->FallbackGlyphIndex == -1 && baked->LoadNoFallback == 0)
            ImFontAtlasBuildSetupFontBakedFallback(baked);
        return NULL;
    }

    // User remapping hooks
    ImWchar src_codepoint = codepoint;
    ImFontAtlas_FontHookRemapCodepoint(atlas, font, &codepoint);

    //char utf8_buf[5];
    //IMGUI_DEBUG_LOG("[font] BuildLoadGlyph U+%04X (%s)\n", (unsigned int)codepoint, ImTextCharToUtf8(utf8_buf, (unsigned int)codepoint));

    // Special hook
    // FIXME-NEWATLAS: it would be nicer if this used a more standardized way of hooking
    if (codepoint == font->EllipsisChar && font->EllipsisAutoBake)
        if (ImFontGlyph* glyph = ImFontAtlasBuildSetupFontBakedEllipsis(atlas, baked))
            return glyph;

    // Call backend
    char* loader_user_data_p = (char*)baked->FontLoaderDatas;
    int src_n = 0;
    for (ImFontConfig* src : font->Sources)
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        if (!src->GlyphExcludeRanges || ImFontAtlasBuildAcceptCodepointForSource(src, codepoint))
        {
            if (only_load_advance_x == NULL)
            {
                ImFontGlyph glyph_buf;
                if (loader->FontBakedLoadGlyph(atlas, src, baked, loader_user_data_p, codepoint, &glyph_buf, NULL))
                {
                    // FIXME: Add hooks for e.g. #7962
                    glyph_buf.Codepoint = src_codepoint;
                    glyph_buf.SourceIdx = src_n;
                    return ImFontAtlasBakedAddFontGlyph(atlas, baked, src, &glyph_buf);
                }
            }
            else
            {
                // Special mode but only loading glyphs metrics. Will rasterize and pack later.
                if (loader->FontBakedLoadGlyph(atlas, src, baked, loader_user_data_p, codepoint, NULL, only_load_advance_x))
                {
                    ImFontAtlasBakedAddFontGlyphAdvancedX(atlas, baked, src, codepoint, *only_load_advance_x);
                    return NULL;
                }
            }
        }
        loader_user_data_p += loader->FontBakedSrcLoaderDataSize;
        src_n++;
    }

    // Lazily load fallback glyph
    if (baked->LoadNoFallback)
        return NULL;
    if (baked->FallbackGlyphIndex == -1)
        ImFontAtlasBuildSetupFontBakedFallback(baked);

    // Mark index as not found, so we don't attempt the search twice
    ImFontBaked_BuildGrowIndex(baked, codepoint + 1);
    baked->IndexAdvanceX[codepoint] = baked->FallbackAdvanceX;
    baked->IndexLookup[codepoint] = IM_FONTGLYPH_INDEX_NOT_FOUND;
    return NULL;
}

static float ImFontBaked_BuildLoadGlyphAdvanceX(ImFontBaked* baked, ImWchar codepoint)
{
    if (baked->Size >= IMGUI_FONT_SIZE_THRESHOLD_FOR_LOADADVANCEXONLYMODE || baked->LoadNoRenderOnLayout)
    {
        // First load AdvanceX value used by CalcTextSize() API then load the rest when loaded by drawing API.
        float only_advance_x = 0.0f;
        ImFontGlyph* glyph = ImFontBaked_BuildLoadGlyph(baked, (ImWchar)codepoint, &only_advance_x);
        return glyph ? glyph->AdvanceX : only_advance_x;
    }
    else
    {
        ImFontGlyph* glyph = ImFontBaked_BuildLoadGlyph(baked, (ImWchar)codepoint, NULL);
        return glyph ? glyph->AdvanceX : baked->FallbackAdvanceX;
    }
}

// The point of this indirection is to not be inlined in debug mode in order to not bloat inner loop.b
IM_MSVC_RUNTIME_CHECKS_OFF
static float BuildLoadGlyphGetAdvanceOrFallback(ImFontBaked* baked, unsigned int codepoint)
{
    return ImFontBaked_BuildLoadGlyphAdvanceX(baked, (ImWchar)codepoint);
}
IM_MSVC_RUNTIME_CHECKS_RESTORE

#ifndef IMGUI_DISABLE_DEBUG_TOOLS
void ImFontAtlasDebugLogTextureRequests(ImFontAtlas* atlas)
{
    // [DEBUG] Log texture update requests
    ImGuiContext& g = *GImGui;
    IM_UNUSED(g);
    for (ImTextureData* tex : atlas->TexList)
    {
        if ((g.IO.BackendFlags & ImGuiBackendFlags_RendererHasTextures) == 0)
            IM_ASSERT(tex->Updates.Size == 0);
        if (tex->Status == ImTextureStatus_WantCreate)
            IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: create %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
        else if (tex->Status == ImTextureStatus_WantDestroy)
            IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: destroy %dx%d, texid=0x%" IM_PRIX64 ", backend_data=%p\n", tex->UniqueID, tex->Width, tex->Height, IM_TEXTUREID_TO_U64(tex->TexID), tex->BackendUserData);
        else if (tex->Status == ImTextureStatus_WantUpdates)
        {
            IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: update %d regions, texid=0x%" IM_PRIX64 ", backend_data=0x%" IM_PRIX64 "\n", tex->UniqueID, tex->Updates.Size, IM_TEXTUREID_TO_U64(tex->TexID), (ImU64)(intptr_t)tex->BackendUserData);
            for (const ImTextureRect& r : tex->Updates)
            {
                IM_UNUSED(r);
                IM_ASSERT(r.x >= 0 && r.y >= 0);
                IM_ASSERT(r.x + r.w <= tex->Width && r.y + r.h <= tex->Height); // In theory should subtract PackPadding but it's currently part of atlas and mid-frame change would wreck assert.
                //IMGUI_DEBUG_LOG_FONT("[font] Texture #%03d: update (% 4d..%-4d)->(% 4d..%-4d), texid=0x%" IM_PRIX64 ", backend_data=0x%" IM_PRIX64 "\n", tex->UniqueID, r.x, r.y, r.x + r.w, r.y + r.h, IM_TEXTUREID_TO_U64(tex->TexID), (ImU64)(intptr_t)tex->BackendUserData);
            }
        }
    }
}
#endif

//-------------------------------------------------------------------------
// [SECTION] ImFontAtlas: backend for stb_truetype
//-------------------------------------------------------------------------
// (imstb_truetype.h in included near the top of this file, when IMGUI_ENABLE_STB_TRUETYPE is set)
//-------------------------------------------------------------------------

#ifdef IMGUI_ENABLE_STB_TRUETYPE

// One for each ConfigData
struct ImGui_ImplStbTrueType_FontSrcData
{
    stbtt_fontinfo  FontInfo;
    float           ScaleFactor;
};

static bool ImGui_ImplStbTrueType_FontSrcInit(ImFontAtlas* atlas, ImFontConfig* src)
{
    IM_UNUSED(atlas);

    ImGui_ImplStbTrueType_FontSrcData* bd_font_data = IM_NEW(ImGui_ImplStbTrueType_FontSrcData);
    IM_ASSERT(src->FontLoaderData == NULL);

    // Initialize helper structure for font loading and verify that the TTF/OTF data is correct
    const int font_offset = stbtt_GetFontOffsetForIndex((unsigned char*)src->FontData, src->FontNo);
    if (font_offset < 0)
    {
        IM_DELETE(bd_font_data);
        IM_ASSERT_USER_ERROR(0, "stbtt_GetFontOffsetForIndex(): FontData is incorrect, or FontNo cannot be found.");
        return false;
    }
    if (!stbtt_InitFont(&bd_font_data->FontInfo, (unsigned char*)src->FontData, font_offset))
    {
        IM_DELETE(bd_font_data);
        IM_ASSERT_USER_ERROR(0, "stbtt_InitFont(): failed to parse FontData. It is correct and complete? Check FontDataSize.");
        return false;
    }
    src->FontLoaderData = bd_font_data;

    const float ref_size = src->DstFont->Sources[0]->SizePixels;
    if (src->MergeMode && src->SizePixels == 0.0f)
        src->SizePixels = ref_size;

    if (src->SizePixels >= 0.0f)
        bd_font_data->ScaleFactor = stbtt_ScaleForPixelHeight(&bd_font_data->FontInfo, 1.0f);
    else
        bd_font_data->ScaleFactor = stbtt_ScaleForMappingEmToPixels(&bd_font_data->FontInfo, 1.0f);
    if (src->MergeMode && src->SizePixels != 0.0f && ref_size != 0.0f)
        bd_font_data->ScaleFactor *= src->SizePixels / ref_size; // FIXME-NEWATLAS: Should tidy up that a bit

    return true;
}

static void ImGui_ImplStbTrueType_FontSrcDestroy(ImFontAtlas* atlas, ImFontConfig* src)
{
    IM_UNUSED(atlas);
    ImGui_ImplStbTrueType_FontSrcData* bd_font_data = (ImGui_ImplStbTrueType_FontSrcData*)src->FontLoaderData;
    IM_DELETE(bd_font_data);
    src->FontLoaderData = NULL;
}

static bool ImGui_ImplStbTrueType_FontSrcContainsGlyph(ImFontAtlas* atlas, ImFontConfig* src, ImWchar codepoint)
{
    IM_UNUSED(atlas);

    ImGui_ImplStbTrueType_FontSrcData* bd_font_data = (ImGui_ImplStbTrueType_FontSrcData*)src->FontLoaderData;
    IM_ASSERT(bd_font_data != NULL);

    int glyph_index = stbtt_FindGlyphIndex(&bd_font_data->FontInfo, (int)codepoint);
    return glyph_index != 0;
}

static bool ImGui_ImplStbTrueType_FontBakedInit(ImFontAtlas* atlas, ImFontConfig* src, ImFontBaked* baked, void*)
{
    IM_UNUSED(atlas);

    ImGui_ImplStbTrueType_FontSrcData* bd_font_data = (ImGui_ImplStbTrueType_FontSrcData*)src->FontLoaderData;
    if (src->MergeMode == false)
    {
        // FIXME-NEWFONTS: reevaluate how to use sizing metrics
        // FIXME-NEWFONTS: make use of line gap value
        float scale_for_layout = bd_font_data->ScaleFactor * baked->Size;
        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&bd_font_data->FontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);
        baked->Ascent = ImCeil(unscaled_ascent * scale_for_layout);
        baked->Descent = ImFloor(unscaled_descent * scale_for_layout);
    }
    return true;
}

static bool ImGui_ImplStbTrueType_FontBakedLoadGlyph(ImFontAtlas* atlas, ImFontConfig* src, ImFontBaked* baked, void*, ImWchar codepoint, ImFontGlyph* out_glyph, float* out_advance_x)
{
    // Search for first font which has the glyph
    ImGui_ImplStbTrueType_FontSrcData* bd_font_data = (ImGui_ImplStbTrueType_FontSrcData*)src->FontLoaderData;
    IM_ASSERT(bd_font_data);
    int glyph_index = stbtt_FindGlyphIndex(&bd_font_data->FontInfo, (int)codepoint);
    if (glyph_index == 0)
        return false;

    // Fonts unit to pixels
    int oversample_h, oversample_v;
    ImFontAtlasBuildGetOversampleFactors(src, baked, &oversample_h, &oversample_v);
    const float scale_for_layout = bd_font_data->ScaleFactor * baked->Size;
    const float rasterizer_density = src->RasterizerDensity * baked->RasterizerDensity;
    const float scale_for_raster_x = bd_font_data->ScaleFactor * baked->Size * rasterizer_density * oversample_h;
    const float scale_for_raster_y = bd_font_data->ScaleFactor * baked->Size * rasterizer_density * oversample_v;

    // Obtain size and advance
    int x0, y0, x1, y1;
    int advance, lsb;
    stbtt_GetGlyphBitmapBoxSubpixel(&bd_font_data->FontInfo, glyph_index, scale_for_raster_x, scale_for_raster_y, 0, 0, &x0, &y0, &x1, &y1);
    stbtt_GetGlyphHMetrics(&bd_font_data->FontInfo, glyph_index, &advance, &lsb);

    // Load metrics only mode
    if (out_advance_x != NULL)
    {
        IM_ASSERT(out_glyph == NULL);
        *out_advance_x = advance * scale_for_layout;
        return true;
    }

    // Prepare glyph
    out_glyph->Codepoint = codepoint;
    out_glyph->AdvanceX = advance * scale_for_layout;

    // Pack and retrieve position inside texture atlas
    // (generally based on stbtt_PackFontRangesRenderIntoRects)
    const bool is_visible = (x0 != x1 && y0 != y1);
    if (is_visible)
    {
        const int w = (x1 - x0 + oversample_h - 1);
        const int h = (y1 - y0 + oversample_v - 1);
        ImFontAtlasRectId pack_id = ImFontAtlasPackAddRect(atlas, w, h);
        if (pack_id == ImFontAtlasRectId_Invalid)
        {
            // Pathological out of memory case (TexMaxWidth/TexMaxHeight set too small?)
            IM_ASSERT(pack_id != ImFontAtlasRectId_Invalid && "Out of texture memory.");
            return false;
        }
        ImTextureRect* r = ImFontAtlasPackGetRect(atlas, pack_id);

        // Render
        stbtt_GetGlyphBitmapBox(&bd_font_data->FontInfo, glyph_index, scale_for_raster_x, scale_for_raster_y, &x0, &y0, &x1, &y1);
        ImFontAtlasBuilder* builder = atlas->Builder;
        builder->TempBuffer.resize(w * h * 1);
        unsigned char* bitmap_pixels = builder->TempBuffer.Data;
        memset(bitmap_pixels, 0, w * h * 1);

        // Render with oversampling
        // (those functions conveniently assert if pixels are not cleared, which is another safety layer)
        float sub_x, sub_y;
        stbtt_MakeGlyphBitmapSubpixelPrefilter(&bd_font_data->FontInfo, bitmap_pixels, w, h, w,
            scale_for_raster_x, scale_for_raster_y, 0, 0, oversample_h, oversample_v, &sub_x, &sub_y, glyph_index);

        const float ref_size = baked->ContainerFont->Sources[0]->SizePixels;
        const float offsets_scale = (ref_size != 0.0f) ? (baked->Size / ref_size) : 1.0f;
        float font_off_x = (src->GlyphOffset.x * offsets_scale);
        float font_off_y = (src->GlyphOffset.y * offsets_scale);
        if (src->PixelSnapH) // Snap scaled offset. This is to mitigate backward compatibility issues for GlyphOffset, but a better design would be welcome.
            font_off_x = IM_ROUND(font_off_x);
        if (src->PixelSnapV)
            font_off_y = IM_ROUND(font_off_y);
        font_off_x += sub_x;
        font_off_y += sub_y + IM_ROUND(baked->Ascent);
        float recip_h = 1.0f / (oversample_h * rasterizer_density);
        float recip_v = 1.0f / (oversample_v * rasterizer_density);

        // Register glyph
        // r->x r->y are coordinates inside texture (in pixels)
        // glyph.X0, glyph.Y0 are drawing coordinates from base text position, and accounting for oversampling.
        out_glyph->X0 = x0 * recip_h + font_off_x;
        out_glyph->Y0 = y0 * recip_v + font_off_y;
        out_glyph->X1 = (x0 + (int)r->w) * recip_h + font_off_x;
        out_glyph->Y1 = (y0 + (int)r->h) * recip_v + font_off_y;
        out_glyph->Visible = true;
        out_glyph->PackId = pack_id;
        ImFontAtlasBakedSetFontGlyphBitmap(atlas, baked, src, out_glyph, r, bitmap_pixels, ImTextureFormat_Alpha8, w);
    }

    return true;
}

const ImFontLoader* ImFontAtlasGetFontLoaderForStbTruetype()
{
    static ImFontLoader loader;
    loader.Name = "stb_truetype";
    loader.FontSrcInit = ImGui_ImplStbTrueType_FontSrcInit;
    loader.FontSrcDestroy = ImGui_ImplStbTrueType_FontSrcDestroy;
    loader.FontSrcContainsGlyph = ImGui_ImplStbTrueType_FontSrcContainsGlyph;
    loader.FontBakedInit = ImGui_ImplStbTrueType_FontBakedInit;
    loader.FontBakedDestroy = NULL;
    loader.FontBakedLoadGlyph = ImGui_ImplStbTrueType_FontBakedLoadGlyph;
    return &loader;
}

#endif // IMGUI_ENABLE_STB_TRUETYPE

//-------------------------------------------------------------------------
// [SECTION] ImFontAtlas: glyph ranges helpers
//-------------------------------------------------------------------------
// - GetGlyphRangesDefault()
// Obsolete functions since 1.92:
// - GetGlyphRangesGreek()
// - GetGlyphRangesKorean()
// - GetGlyphRangesChineseFull()
// - GetGlyphRangesChineseSimplifiedCommon()
// - GetGlyphRangesJapanese()
// - GetGlyphRangesCyrillic()
// - GetGlyphRangesThai()
// - GetGlyphRangesVietnamese()
//-----------------------------------------------------------------------------

// Retrieve list of range (2 int per range, values are inclusive)
const ImWchar*   ImFontAtlas::GetGlyphRangesDefault()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0,
    };
    return &ranges[0];
}

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
const ImWchar*   ImFontAtlas::GetGlyphRangesGreek()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0370, 0x03FF, // Greek and Coptic
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesKorean()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3131, 0x3163, // Korean alphabets
        0xAC00, 0xD7A3, // Korean characters
        0xFFFD, 0xFFFD, // Invalid
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesChineseFull()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0xFFFD, 0xFFFD, // Invalid
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    return &ranges[0];
}

static void UnpackAccumulativeOffsetsIntoRanges(int base_codepoint, const short* accumulative_offsets, int accumulative_offsets_count, ImWchar* out_ranges)
{
    for (int n = 0; n < accumulative_offsets_count; n++, out_ranges += 2)
    {
        out_ranges[0] = out_ranges[1] = (ImWchar)(base_codepoint + accumulative_offsets[n]);
        base_codepoint += accumulative_offsets[n];
    }
    out_ranges[0] = 0;
}

const ImWchar*  ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon()
{
    // Store 2500 regularly used characters for Simplified Chinese.
    // Sourced from https://zh.wiktionary.org/wiki/%E9%99%84%E5%BD%95:%E7%8E%B0%E4%BB%A3%E6%B1%89%E8%AF%AD%E5%B8%B8%E7%94%A8%E5%AD%97%E8%A1%A8
    // This table covers 97.97% of all characters used during the month in July, 1987.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,3,2,1,2,2,1,1,1,1,1,5,2,1,2,3,3,3,2,2,4,1,1,1,2,1,5,2,3,1,2,1,2,1,1,2,1,1,2,2,1,4,1,1,1,1,5,10,1,2,19,2,1,2,1,2,1,2,1,2,
        1,5,1,6,3,2,1,2,2,1,1,1,4,8,5,1,1,4,1,1,3,1,2,1,5,1,2,1,1,1,10,1,1,5,2,4,6,1,4,2,2,2,12,2,1,1,6,1,1,1,4,1,1,4,6,5,1,4,2,2,4,10,7,1,1,4,2,4,
        2,1,4,3,6,10,12,5,7,2,14,2,9,1,1,6,7,10,4,7,13,1,5,4,8,4,1,1,2,28,5,6,1,1,5,2,5,20,2,2,9,8,11,2,9,17,1,8,6,8,27,4,6,9,20,11,27,6,68,2,2,1,1,
        1,2,1,2,2,7,6,11,3,3,1,1,3,1,2,1,1,1,1,1,3,1,1,8,3,4,1,5,7,2,1,4,4,8,4,2,1,2,1,1,4,5,6,3,6,2,12,3,1,3,9,2,4,3,4,1,5,3,3,1,3,7,1,5,1,1,1,1,2,
        3,4,5,2,3,2,6,1,1,2,1,7,1,7,3,4,5,15,2,2,1,5,3,22,19,2,1,1,1,1,2,5,1,1,1,6,1,1,12,8,2,9,18,22,4,1,1,5,1,16,1,2,7,10,15,1,1,6,2,4,1,2,4,1,6,
        1,1,3,2,4,1,6,4,5,1,2,1,1,2,1,10,3,1,3,2,1,9,3,2,5,7,2,19,4,3,6,1,1,1,1,1,4,3,2,1,1,1,2,5,3,1,1,1,2,2,1,1,2,1,1,2,1,3,1,1,1,3,7,1,4,1,1,2,1,
        1,2,1,2,4,4,3,8,1,1,1,2,1,3,5,1,3,1,3,4,6,2,2,14,4,6,6,11,9,1,15,3,1,28,5,2,5,5,3,1,3,4,5,4,6,14,3,2,3,5,21,2,7,20,10,1,2,19,2,4,28,28,2,3,
        2,1,14,4,1,26,28,42,12,40,3,52,79,5,14,17,3,2,2,11,3,4,6,3,1,8,2,23,4,5,8,10,4,2,7,3,5,1,1,6,3,1,2,2,2,5,28,1,1,7,7,20,5,3,29,3,17,26,1,8,4,
        27,3,6,11,23,5,3,4,6,13,24,16,6,5,10,25,35,7,3,2,3,3,14,3,6,2,6,1,4,2,3,8,2,1,1,3,3,3,4,1,1,13,2,2,4,5,2,1,14,14,1,2,2,1,4,5,2,3,1,14,3,12,
        3,17,2,16,5,1,2,1,8,9,3,19,4,2,2,4,17,25,21,20,28,75,1,10,29,103,4,1,2,1,1,4,2,4,1,2,3,24,2,2,2,1,1,2,1,3,8,1,1,1,2,1,1,3,1,1,1,6,1,5,3,1,1,
        1,3,4,1,1,5,2,1,5,6,13,9,16,1,1,1,1,3,2,3,2,4,5,2,5,2,2,3,7,13,7,2,2,1,1,1,1,2,3,3,2,1,6,4,9,2,1,14,2,14,2,1,18,3,4,14,4,11,41,15,23,15,23,
        176,1,3,4,1,1,1,1,5,3,1,2,3,7,3,1,1,2,1,2,4,4,6,2,4,1,9,7,1,10,5,8,16,29,1,1,2,2,3,1,3,5,2,4,5,4,1,1,2,2,3,3,7,1,6,10,1,17,1,44,4,6,2,1,1,6,
        5,4,2,10,1,6,9,2,8,1,24,1,2,13,7,8,8,2,1,4,1,3,1,3,3,5,2,5,10,9,4,9,12,2,1,6,1,10,1,1,7,7,4,10,8,3,1,13,4,3,1,6,1,3,5,2,1,2,17,16,5,2,16,6,
        1,4,2,1,3,3,6,8,5,11,11,1,3,3,2,4,6,10,9,5,7,4,7,4,7,1,1,4,2,1,3,6,8,7,1,6,11,5,5,3,24,9,4,2,7,13,5,1,8,82,16,61,1,1,1,4,2,2,16,10,3,8,1,1,
        6,4,2,1,3,1,1,1,4,3,8,4,2,2,1,1,1,1,1,6,3,5,1,1,4,6,9,2,1,1,1,2,1,7,2,1,6,1,5,4,4,3,1,8,1,3,3,1,3,2,2,2,2,3,1,6,1,2,1,2,1,3,7,1,8,2,1,2,1,5,
        2,5,3,5,10,1,2,1,1,3,2,5,11,3,9,3,5,1,1,5,9,1,2,1,5,7,9,9,8,1,3,3,3,6,8,2,3,2,1,1,32,6,1,2,15,9,3,7,13,1,3,10,13,2,14,1,13,10,2,1,3,10,4,15,
        2,15,15,10,1,3,9,6,9,32,25,26,47,7,3,2,3,1,6,3,4,3,2,8,5,4,1,9,4,2,2,19,10,6,2,3,8,1,2,2,4,2,1,9,4,4,4,6,4,8,9,2,3,1,1,1,1,3,5,5,1,3,8,4,6,
        2,1,4,12,1,5,3,7,13,2,5,8,1,6,1,2,5,14,6,1,5,2,4,8,15,5,1,23,6,62,2,10,1,1,8,1,2,2,10,4,2,2,9,2,1,1,3,2,3,1,5,3,3,2,1,3,8,1,1,1,11,3,1,1,4,
        3,7,1,14,1,2,3,12,5,2,5,1,6,7,5,7,14,11,1,3,1,8,9,12,2,1,11,8,4,4,2,6,10,9,13,1,1,3,1,5,1,3,2,4,4,1,18,2,3,14,11,4,29,4,2,7,1,3,13,9,2,2,5,
        3,5,20,7,16,8,5,72,34,6,4,22,12,12,28,45,36,9,7,39,9,191,1,1,1,4,11,8,4,9,2,3,22,1,1,1,1,4,17,1,7,7,1,11,31,10,2,4,8,2,3,2,1,4,2,16,4,32,2,
        3,19,13,4,9,1,5,2,14,8,1,1,3,6,19,6,5,1,16,6,2,10,8,5,1,2,3,1,5,5,1,11,6,6,1,3,3,2,6,3,8,1,1,4,10,7,5,7,7,5,8,9,2,1,3,4,1,1,3,1,3,3,2,6,16,
        1,4,6,3,1,10,6,1,3,15,2,9,2,10,25,13,9,16,6,2,2,10,11,4,3,9,1,2,6,6,5,4,30,40,1,10,7,12,14,33,6,3,6,7,3,1,3,1,11,14,4,9,5,12,11,49,18,51,31,
        140,31,2,2,1,5,1,8,1,10,1,4,4,3,24,1,10,1,3,6,6,16,3,4,5,2,1,4,2,57,10,6,22,2,22,3,7,22,6,10,11,36,18,16,33,36,2,5,5,1,1,1,4,10,1,4,13,2,7,
        5,2,9,3,4,1,7,43,3,7,3,9,14,7,9,1,11,1,1,3,7,4,18,13,1,14,1,3,6,10,73,2,2,30,6,1,11,18,19,13,22,3,46,42,37,89,7,3,16,34,2,2,3,9,1,7,1,1,1,2,
        2,4,10,7,3,10,3,9,5,28,9,2,6,13,7,3,1,3,10,2,7,2,11,3,6,21,54,85,2,1,4,2,2,1,39,3,21,2,2,5,1,1,1,4,1,1,3,4,15,1,3,2,4,4,2,3,8,2,20,1,8,7,13,
        4,1,26,6,2,9,34,4,21,52,10,4,4,1,5,12,2,11,1,7,2,30,12,44,2,30,1,1,3,6,16,9,17,39,82,2,2,24,7,1,7,3,16,9,14,44,2,1,2,1,2,3,5,2,4,1,6,7,5,3,
        2,6,1,11,5,11,2,1,18,19,8,1,3,24,29,2,1,3,5,2,2,1,13,6,5,1,46,11,3,5,1,1,5,8,2,10,6,12,6,3,7,11,2,4,16,13,2,5,1,1,2,2,5,2,28,5,2,23,10,8,4,
        4,22,39,95,38,8,14,9,5,1,13,5,4,3,13,12,11,1,9,1,27,37,2,5,4,4,63,211,95,2,2,2,1,3,5,2,1,1,2,2,1,1,1,3,2,4,1,2,1,1,5,2,2,1,1,2,3,1,3,1,1,1,
        3,1,4,2,1,3,6,1,1,3,7,15,5,3,2,5,3,9,11,4,2,22,1,6,3,8,7,1,4,28,4,16,3,3,25,4,4,27,27,1,4,1,2,2,7,1,3,5,2,28,8,2,14,1,8,6,16,25,3,3,3,14,3,
        3,1,1,2,1,4,6,3,8,4,1,1,1,2,3,6,10,6,2,3,18,3,2,5,5,4,3,1,5,2,5,4,23,7,6,12,6,4,17,11,9,5,1,1,10,5,12,1,1,11,26,33,7,3,6,1,17,7,1,5,12,1,11,
        2,4,1,8,14,17,23,1,2,1,7,8,16,11,9,6,5,2,6,4,16,2,8,14,1,11,8,9,1,1,1,9,25,4,11,19,7,2,15,2,12,8,52,7,5,19,2,16,4,36,8,1,16,8,24,26,4,6,2,9,
        5,4,36,3,28,12,25,15,37,27,17,12,59,38,5,32,127,1,2,9,17,14,4,1,2,1,1,8,11,50,4,14,2,19,16,4,17,5,4,5,26,12,45,2,23,45,104,30,12,8,3,10,2,2,
        3,3,1,4,20,7,2,9,6,15,2,20,1,3,16,4,11,15,6,134,2,5,59,1,2,2,2,1,9,17,3,26,137,10,211,59,1,2,4,1,4,1,1,1,2,6,2,3,1,1,2,3,2,3,1,3,4,4,2,3,3,
        1,4,3,1,7,2,2,3,1,2,1,3,3,3,2,2,3,2,1,3,14,6,1,3,2,9,6,15,27,9,34,145,1,1,2,1,1,1,1,2,1,1,1,1,2,2,2,3,1,2,1,1,1,2,3,5,8,3,5,2,4,1,3,2,2,2,12,
        4,1,1,1,10,4,5,1,20,4,16,1,15,9,5,12,2,9,2,5,4,2,26,19,7,1,26,4,30,12,15,42,1,6,8,172,1,1,4,2,1,1,11,2,2,4,2,1,2,1,10,8,1,2,1,4,5,1,2,5,1,8,
        4,1,3,4,2,1,6,2,1,3,4,1,2,1,1,1,1,12,5,7,2,4,3,1,1,1,3,3,6,1,2,2,3,3,3,2,1,2,12,14,11,6,6,4,12,2,8,1,7,10,1,35,7,4,13,15,4,3,23,21,28,52,5,
        26,5,6,1,7,10,2,7,53,3,2,1,1,1,2,163,532,1,10,11,1,3,3,4,8,2,8,6,2,2,23,22,4,2,2,4,2,1,3,1,3,3,5,9,8,2,1,2,8,1,10,2,12,21,20,15,105,2,3,1,1,
        3,2,3,1,1,2,5,1,4,15,11,19,1,1,1,1,5,4,5,1,1,2,5,3,5,12,1,2,5,1,11,1,1,15,9,1,4,5,3,26,8,2,1,3,1,1,15,19,2,12,1,2,5,2,7,2,19,2,20,6,26,7,5,
        2,2,7,34,21,13,70,2,128,1,1,2,1,1,2,1,1,3,2,2,2,15,1,4,1,3,4,42,10,6,1,49,85,8,1,2,1,1,4,4,2,3,6,1,5,7,4,3,211,4,1,2,1,2,5,1,2,4,2,2,6,5,6,
        10,3,4,48,100,6,2,16,296,5,27,387,2,2,3,7,16,8,5,38,15,39,21,9,10,3,7,59,13,27,21,47,5,21,6
    };
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0xFFFD, 0xFFFD  // Invalid
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00) * 2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesJapanese()
{
    // 2999 ideograms code points for Japanese
    // - 2136 Joyo (meaning "for regular use" or "for common use") Kanji code points
    // - 863 Jinmeiyo (meaning "for personal name") Kanji code points
    // - Sourced from official information provided by the government agencies of Japan:
    //   - List of Joyo Kanji by the Agency for Cultural Affairs
    //     - https://www.bunka.go.jp/kokugo_nihongo/sisaku/joho/joho/kijun/naikaku/kanji/
    //   - List of Jinmeiyo Kanji by the Ministry of Justice
    //     - http://www.moj.go.jp/MINJI/minji86.html
    //   - Available under the terms of the Creative Commons Attribution 4.0 International (CC BY 4.0).
    //     - https://creativecommons.org/licenses/by/4.0/legalcode
    // - You can generate this code by the script at:
    //   - https://github.com/vaiorabbit/everyday_use_kanji
    // - References:
    //   - List of Joyo Kanji
    //     - (Wikipedia) https://en.wikipedia.org/wiki/List_of_j%C5%8Dy%C5%8D_kanji
    //   - List of Jinmeiyo Kanji
    //     - (Wikipedia) https://en.wikipedia.org/wiki/Jinmeiy%C5%8D_kanji
    // - Missing 1 Joyo Kanji: U+20B9F (Kun'yomi: Shikaru, On'yomi: Shitsu,shichi), see https://github.com/ocornut/imgui/pull/3627 for details.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,3,3,2,2,1,5,3,5,7,5,6,1,2,1,7,2,6,3,1,8,1,1,4,1,1,18,2,11,2,6,2,1,2,1,5,1,2,1,3,1,2,1,2,3,3,1,1,2,3,1,1,1,12,7,9,1,4,5,1,
        1,2,1,10,1,1,9,2,2,4,5,6,9,3,1,1,1,1,9,3,18,5,2,2,2,2,1,6,3,7,1,1,1,1,2,2,4,2,1,23,2,10,4,3,5,2,4,10,2,4,13,1,6,1,9,3,1,1,6,6,7,6,3,1,2,11,3,
        2,2,3,2,15,2,2,5,4,3,6,4,1,2,5,2,12,16,6,13,9,13,2,1,1,7,16,4,7,1,19,1,5,1,2,2,7,7,8,2,6,5,4,9,18,7,4,5,9,13,11,8,15,2,1,1,1,2,1,2,2,1,2,2,8,
        2,9,3,3,1,1,4,4,1,1,1,4,9,1,4,3,5,5,2,7,5,3,4,8,2,1,13,2,3,3,1,14,1,1,4,5,1,3,6,1,5,2,1,1,3,3,3,3,1,1,2,7,6,6,7,1,4,7,6,1,1,1,1,1,12,3,3,9,5,
        2,6,1,5,6,1,2,3,18,2,4,14,4,1,3,6,1,1,6,3,5,5,3,2,2,2,2,12,3,1,4,2,3,2,3,11,1,7,4,1,2,1,3,17,1,9,1,24,1,1,4,2,2,4,1,2,7,1,1,1,3,1,2,2,4,15,1,
        1,2,1,1,2,1,5,2,5,20,2,5,9,1,10,8,7,6,1,1,1,1,1,1,6,2,1,2,8,1,1,1,1,5,1,1,3,1,1,1,1,3,1,1,12,4,1,3,1,1,1,1,1,10,3,1,7,5,13,1,2,3,4,6,1,1,30,
        2,9,9,1,15,38,11,3,1,8,24,7,1,9,8,10,2,1,9,31,2,13,6,2,9,4,49,5,2,15,2,1,10,2,1,1,1,2,2,6,15,30,35,3,14,18,8,1,16,10,28,12,19,45,38,1,3,2,3,
        13,2,1,7,3,6,5,3,4,3,1,5,7,8,1,5,3,18,5,3,6,1,21,4,24,9,24,40,3,14,3,21,3,2,1,2,4,2,3,1,15,15,6,5,1,1,3,1,5,6,1,9,7,3,3,2,1,4,3,8,21,5,16,4,
        5,2,10,11,11,3,6,3,2,9,3,6,13,1,2,1,1,1,1,11,12,6,6,1,4,2,6,5,2,1,1,3,3,6,13,3,1,1,5,1,2,3,3,14,2,1,2,2,2,5,1,9,5,1,1,6,12,3,12,3,4,13,2,14,
        2,8,1,17,5,1,16,4,2,2,21,8,9,6,23,20,12,25,19,9,38,8,3,21,40,25,33,13,4,3,1,4,1,2,4,1,2,5,26,2,1,1,2,1,3,6,2,1,1,1,1,1,1,2,3,1,1,1,9,2,3,1,1,
        1,3,6,3,2,1,1,6,6,1,8,2,2,2,1,4,1,2,3,2,7,3,2,4,1,2,1,2,2,1,1,1,1,1,3,1,2,5,4,10,9,4,9,1,1,1,1,1,1,5,3,2,1,6,4,9,6,1,10,2,31,17,8,3,7,5,40,1,
        7,7,1,6,5,2,10,7,8,4,15,39,25,6,28,47,18,10,7,1,3,1,1,2,1,1,1,3,3,3,1,1,1,3,4,2,1,4,1,3,6,10,7,8,6,2,2,1,3,3,2,5,8,7,9,12,2,15,1,1,4,1,2,1,1,
        1,3,2,1,3,3,5,6,2,3,2,10,1,4,2,8,1,1,1,11,6,1,21,4,16,3,1,3,1,4,2,3,6,5,1,3,1,1,3,3,4,6,1,1,10,4,2,7,10,4,7,4,2,9,4,3,1,1,1,4,1,8,3,4,1,3,1,
        6,1,4,2,1,4,7,2,1,8,1,4,5,1,1,2,2,4,6,2,7,1,10,1,1,3,4,11,10,8,21,4,6,1,3,5,2,1,2,28,5,5,2,3,13,1,2,3,1,4,2,1,5,20,3,8,11,1,3,3,3,1,8,10,9,2,
        10,9,2,3,1,1,2,4,1,8,3,6,1,7,8,6,11,1,4,29,8,4,3,1,2,7,13,1,4,1,6,2,6,12,12,2,20,3,2,3,6,4,8,9,2,7,34,5,1,18,6,1,1,4,4,5,7,9,1,2,2,4,3,4,1,7,
        2,2,2,6,2,3,25,5,3,6,1,4,6,7,4,2,1,4,2,13,6,4,4,3,1,5,3,4,4,3,2,1,1,4,1,2,1,1,3,1,11,1,6,3,1,7,3,6,2,8,8,6,9,3,4,11,3,2,10,12,2,5,11,1,6,4,5,
        3,1,8,5,4,6,6,3,5,1,1,3,2,1,2,2,6,17,12,1,10,1,6,12,1,6,6,19,9,6,16,1,13,4,4,15,7,17,6,11,9,15,12,6,7,2,1,2,2,15,9,3,21,4,6,49,18,7,3,2,3,1,
        6,8,2,2,6,2,9,1,3,6,4,4,1,2,16,2,5,2,1,6,2,3,5,3,1,2,5,1,2,1,9,3,1,8,6,4,8,11,3,1,1,1,1,3,1,13,8,4,1,3,2,2,1,4,1,11,1,5,2,1,5,2,5,8,6,1,1,7,
        4,3,8,3,2,7,2,1,5,1,5,2,4,7,6,2,8,5,1,11,4,5,3,6,18,1,2,13,3,3,1,21,1,1,4,1,4,1,1,1,8,1,2,2,7,1,2,4,2,2,9,2,1,1,1,4,3,6,3,12,5,1,1,1,5,6,3,2,
        4,8,2,2,4,2,7,1,8,9,5,2,3,2,1,3,2,13,7,14,6,5,1,1,2,1,4,2,23,2,1,1,6,3,1,4,1,15,3,1,7,3,9,14,1,3,1,4,1,1,5,8,1,3,8,3,8,15,11,4,14,4,4,2,5,5,
        1,7,1,6,14,7,7,8,5,15,4,8,6,5,6,2,1,13,1,20,15,11,9,2,5,6,2,11,2,6,2,5,1,5,8,4,13,19,25,4,1,1,11,1,34,2,5,9,14,6,2,2,6,1,1,14,1,3,14,13,1,6,
        12,21,14,14,6,32,17,8,32,9,28,1,2,4,11,8,3,1,14,2,5,15,1,1,1,1,3,6,4,1,3,4,11,3,1,1,11,30,1,5,1,4,1,5,8,1,1,3,2,4,3,17,35,2,6,12,17,3,1,6,2,
        1,1,12,2,7,3,3,2,1,16,2,8,3,6,5,4,7,3,3,8,1,9,8,5,1,2,1,3,2,8,1,2,9,12,1,1,2,3,8,3,24,12,4,3,7,5,8,3,3,3,3,3,3,1,23,10,3,1,2,2,6,3,1,16,1,16,
        22,3,10,4,11,6,9,7,7,3,6,2,2,2,4,10,2,1,1,2,8,7,1,6,4,1,3,3,3,5,10,12,12,2,3,12,8,15,1,1,16,6,6,1,5,9,11,4,11,4,2,6,12,1,17,5,13,1,4,9,5,1,11,
        2,1,8,1,5,7,28,8,3,5,10,2,17,3,38,22,1,2,18,12,10,4,38,18,1,4,44,19,4,1,8,4,1,12,1,4,31,12,1,14,7,75,7,5,10,6,6,13,3,2,11,11,3,2,5,28,15,6,18,
        18,5,6,4,3,16,1,7,18,7,36,3,5,3,1,7,1,9,1,10,7,2,4,2,6,2,9,7,4,3,32,12,3,7,10,2,23,16,3,1,12,3,31,4,11,1,3,8,9,5,1,30,15,6,12,3,2,2,11,19,9,
        14,2,6,2,3,19,13,17,5,3,3,25,3,14,1,1,1,36,1,3,2,19,3,13,36,9,13,31,6,4,16,34,2,5,4,2,3,3,5,1,1,1,4,3,1,17,3,2,3,5,3,1,3,2,3,5,6,3,12,11,1,3,
        1,2,26,7,12,7,2,14,3,3,7,7,11,25,25,28,16,4,36,1,2,1,6,2,1,9,3,27,17,4,3,4,13,4,1,3,2,2,1,10,4,2,4,6,3,8,2,1,18,1,1,24,2,2,4,33,2,3,63,7,1,6,
        40,7,3,4,4,2,4,15,18,1,16,1,1,11,2,41,14,1,3,18,13,3,2,4,16,2,17,7,15,24,7,18,13,44,2,2,3,6,1,1,7,5,1,7,1,4,3,3,5,10,8,2,3,1,8,1,1,27,4,2,1,
        12,1,2,1,10,6,1,6,7,5,2,3,7,11,5,11,3,6,6,2,3,15,4,9,1,1,2,1,2,11,2,8,12,8,5,4,2,3,1,5,2,2,1,14,1,12,11,4,1,11,17,17,4,3,2,5,5,7,3,1,5,9,9,8,
        2,5,6,6,13,13,2,1,2,6,1,2,2,49,4,9,1,2,10,16,7,8,4,3,2,23,4,58,3,29,1,14,19,19,11,11,2,7,5,1,3,4,6,2,18,5,12,12,17,17,3,3,2,4,1,6,2,3,4,3,1,
        1,1,1,5,1,1,9,1,3,1,3,6,1,8,1,1,2,6,4,14,3,1,4,11,4,1,3,32,1,2,4,13,4,1,2,4,2,1,3,1,11,1,4,2,1,4,4,6,3,5,1,6,5,7,6,3,23,3,5,3,5,3,3,13,3,9,10,
        1,12,10,2,3,18,13,7,160,52,4,2,2,3,2,14,5,4,12,4,6,4,1,20,4,11,6,2,12,27,1,4,1,2,2,7,4,5,2,28,3,7,25,8,3,19,3,6,10,2,2,1,10,2,5,4,1,3,4,1,5,
        3,2,6,9,3,6,2,16,3,3,16,4,5,5,3,2,1,2,16,15,8,2,6,21,2,4,1,22,5,8,1,1,21,11,2,1,11,11,19,13,12,4,2,3,2,3,6,1,8,11,1,4,2,9,5,2,1,11,2,9,1,1,2,
        14,31,9,3,4,21,14,4,8,1,7,2,2,2,5,1,4,20,3,3,4,10,1,11,9,8,2,1,4,5,14,12,14,2,17,9,6,31,4,14,1,20,13,26,5,2,7,3,6,13,2,4,2,19,6,2,2,18,9,3,5,
        12,12,14,4,6,2,3,6,9,5,22,4,5,25,6,4,8,5,2,6,27,2,35,2,16,3,7,8,8,6,6,5,9,17,2,20,6,19,2,13,3,1,1,1,4,17,12,2,14,7,1,4,18,12,38,33,2,10,1,1,
        2,13,14,17,11,50,6,33,20,26,74,16,23,45,50,13,38,33,6,6,7,4,4,2,1,3,2,5,8,7,8,9,3,11,21,9,13,1,3,10,6,7,1,2,2,18,5,5,1,9,9,2,68,9,19,13,2,5,
        1,4,4,7,4,13,3,9,10,21,17,3,26,2,1,5,2,4,5,4,1,7,4,7,3,4,2,1,6,1,1,20,4,1,9,2,2,1,3,3,2,3,2,1,1,1,20,2,3,1,6,2,3,6,2,4,8,1,3,2,10,3,5,3,4,4,
        3,4,16,1,6,1,10,2,4,2,1,1,2,10,11,2,2,3,1,24,31,4,10,10,2,5,12,16,164,15,4,16,7,9,15,19,17,1,2,1,1,5,1,1,1,1,1,3,1,4,3,1,3,1,3,1,2,1,1,3,3,7,
        2,8,1,2,2,2,1,3,4,3,7,8,12,92,2,10,3,1,3,14,5,25,16,42,4,7,7,4,2,21,5,27,26,27,21,25,30,31,2,1,5,13,3,22,5,6,6,11,9,12,1,5,9,7,5,5,22,60,3,5,
        13,1,1,8,1,1,3,3,2,1,9,3,3,18,4,1,2,3,7,6,3,1,2,3,9,1,3,1,3,2,1,3,1,1,1,2,1,11,3,1,6,9,1,3,2,3,1,2,1,5,1,1,4,3,4,1,2,2,4,4,1,7,2,1,2,2,3,5,13,
        18,3,4,14,9,9,4,16,3,7,5,8,2,6,48,28,3,1,1,4,2,14,8,2,9,2,1,15,2,4,3,2,10,16,12,8,7,1,1,3,1,1,1,2,7,4,1,6,4,38,39,16,23,7,15,15,3,2,12,7,21,
        37,27,6,5,4,8,2,10,8,8,6,5,1,2,1,3,24,1,16,17,9,23,10,17,6,1,51,55,44,13,294,9,3,6,2,4,2,2,15,1,1,1,13,21,17,68,14,8,9,4,1,4,9,3,11,7,1,1,1,
        5,6,3,2,1,1,1,2,3,8,1,2,2,4,1,5,5,2,1,4,3,7,13,4,1,4,1,3,1,1,1,5,5,10,1,6,1,5,2,1,5,2,4,1,4,5,7,3,18,2,9,11,32,4,3,3,2,4,7,11,16,9,11,8,13,38,
        32,8,4,2,1,1,2,1,2,4,4,1,1,1,4,1,21,3,11,1,16,1,1,6,1,3,2,4,9,8,57,7,44,1,3,3,13,3,10,1,1,7,5,2,7,21,47,63,3,15,4,7,1,16,1,1,2,8,2,3,42,15,4,
        1,29,7,22,10,3,78,16,12,20,18,4,67,11,5,1,3,15,6,21,31,32,27,18,13,71,35,5,142,4,10,1,2,50,19,33,16,35,37,16,19,27,7,1,133,19,1,4,8,7,20,1,4,
        4,1,10,3,1,6,1,2,51,5,40,15,24,43,22928,11,1,13,154,70,3,1,1,7,4,10,1,2,1,1,2,1,2,1,2,2,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,
        3,2,1,1,1,1,2,1,1,
    };
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0xFFFD, 0xFFFD  // Invalid
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00)*2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesCyrillic()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesThai()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x2010, 0x205E, // Punctuations
        0x0E00, 0x0E7F, // Thai
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesVietnamese()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x0102, 0x0103,
        0x0110, 0x0111,
        0x0128, 0x0129,
        0x0168, 0x0169,
        0x01A0, 0x01A1,
        0x01AF, 0x01B0,
        0x1EA0, 0x1EF9,
        0,
    };
    return &ranges[0];
}
#endif // #ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//-----------------------------------------------------------------------------
// [SECTION] ImFontGlyphRangesBuilder
//-----------------------------------------------------------------------------

void ImFontGlyphRangesBuilder::AddText(const char* text, const char* text_end)
{
    while (text_end ? (text < text_end) : *text)
    {
        unsigned int c = 0;
        int c_len = ImTextCharFromUtf8(&c, text, text_end);
        text += c_len;
        if (c_len == 0)
            break;
        AddChar((ImWchar)c);
    }
}

void ImFontGlyphRangesBuilder::AddRanges(const ImWchar* ranges)
{
    for (; ranges[0]; ranges += 2)
        for (unsigned int c = ranges[0]; c <= ranges[1] && c <= IM_UNICODE_CODEPOINT_MAX; c++) //-V560
            AddChar((ImWchar)c);
}

void ImFontGlyphRangesBuilder::BuildRanges(ImVector<ImWchar>* out_ranges)
{
    const int max_codepoint = IM_UNICODE_CODEPOINT_MAX;
    for (int n = 0; n <= max_codepoint; n++)
        if (GetBit(n))
        {
            out_ranges->push_back((ImWchar)n);
            while (n < max_codepoint && GetBit(n + 1))
                n++;
            out_ranges->push_back((ImWchar)n);
        }
    out_ranges->push_back(0);
}

//-----------------------------------------------------------------------------
// [SECTION] ImFont
//-----------------------------------------------------------------------------

ImFontBaked::ImFontBaked()
{
    memset(this, 0, sizeof(*this));
    FallbackGlyphIndex = -1;
}

void ImFontBaked::ClearOutputData()
{
    FallbackAdvanceX = 0.0f;
    Glyphs.clear();
    IndexAdvanceX.clear();
    IndexLookup.clear();
    FallbackGlyphIndex = -1;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
}

ImFont::ImFont()
{
    memset(this, 0, sizeof(*this));
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    Scale = 1.0f;
#endif
}

ImFont::~ImFont()
{
    ClearOutputData();
}

void ImFont::ClearOutputData()
{
    if (ImFontAtlas* atlas = ContainerAtlas)
        ImFontAtlasFontDiscardBakes(atlas, this, 0);
    FallbackChar = EllipsisChar = 0;
    memset(Used8kPagesMap, 0, sizeof(Used8kPagesMap));
    LastBaked = NULL;
}

// API is designed this way to avoid exposing the 8K page size
// e.g. use with IsGlyphRangeUnused(0, 255)
bool ImFont::IsGlyphRangeUnused(unsigned int c_begin, unsigned int c_last)
{
    unsigned int page_begin = (c_begin / 8192);
    unsigned int page_last = (c_last / 8192);
    for (unsigned int page_n = page_begin; page_n <= page_last; page_n++)
        if ((page_n >> 3) < sizeof(Used8kPagesMap))
            if (Used8kPagesMap[page_n >> 3] & (1 << (page_n & 7)))
                return false;
    return true;
}

// x0/y0/x1/y1 are offset from the character upper-left layout position, in pixels. Therefore x0/y0 are often fairly close to zero.
// Not to be mistaken with texture coordinates, which are held by u0/v0/u1/v1 in normalized format (0.0..1.0 on each texture axis).
// - 'src' is not necessarily == 'this->Sources' because multiple source fonts+configs can be used to build one target font.
ImFontGlyph* ImFontAtlasBakedAddFontGlyph(ImFontAtlas* atlas, ImFontBaked* baked, ImFontConfig* src, const ImFontGlyph* in_glyph)
{
    int glyph_idx = baked->Glyphs.Size;
    baked->Glyphs.push_back(*in_glyph);
    ImFontGlyph* glyph = &baked->Glyphs[glyph_idx];
    IM_ASSERT(baked->Glyphs.Size < 0xFFFE); // IndexLookup[] hold 16-bit values and -1/-2 are reserved.

    // Set UV from packed rectangle
    if (glyph->PackId != ImFontAtlasRectId_Invalid)
    {
        ImTextureRect* r = ImFontAtlasPackGetRect(atlas, glyph->PackId);
        IM_ASSERT(glyph->U0 == 0.0f && glyph->V0 == 0.0f && glyph->U1 == 0.0f && glyph->V1 == 0.0f);
        glyph->U0 = (r->x) * atlas->TexUvScale.x;
        glyph->V0 = (r->y) * atlas->TexUvScale.y;
        glyph->U1 = (r->x + r->w) * atlas->TexUvScale.x;
        glyph->V1 = (r->y + r->h) * atlas->TexUvScale.y;
        baked->MetricsTotalSurface += r->w * r->h;
    }

    if (src != NULL)
    {
        // Clamp & recenter if needed
        const float ref_size = baked->ContainerFont->Sources[0]->SizePixels;
        const float offsets_scale = (ref_size != 0.0f) ? (baked->Size / ref_size) : 1.0f;
        float advance_x = ImClamp(glyph->AdvanceX, src->GlyphMinAdvanceX * offsets_scale, src->GlyphMaxAdvanceX * offsets_scale);
        if (advance_x != glyph->AdvanceX)
        {
            float char_off_x = src->PixelSnapH ? ImTrunc((advance_x - glyph->AdvanceX) * 0.5f) : (advance_x - glyph->AdvanceX) * 0.5f;
            glyph->X0 += char_off_x;
            glyph->X1 += char_off_x;
        }

        // Snap to pixel
        if (src->PixelSnapH)
            advance_x = IM_ROUND(advance_x);

        // Bake spacing
        glyph->AdvanceX = advance_x + src->GlyphExtraAdvanceX;
    }
    if (glyph->Colored)
        atlas->TexPixelsUseColors = atlas->TexData->UseColors = true;

    // Update lookup tables
    const int codepoint = glyph->Codepoint;
    ImFontBaked_BuildGrowIndex(baked, codepoint + 1);
    baked->IndexAdvanceX[codepoint] = glyph->AdvanceX;
    baked->IndexLookup[codepoint] = (ImU16)glyph_idx;
    const int page_n = codepoint / 8192;
    baked->ContainerFont->Used8kPagesMap[page_n >> 3] |= 1 << (page_n & 7);

    return glyph;
}

// FIXME: Code is duplicated with code above.
void ImFontAtlasBakedAddFontGlyphAdvancedX(ImFontAtlas* atlas, ImFontBaked* baked, ImFontConfig* src, ImWchar codepoint, float advance_x)
{
    IM_UNUSED(atlas);
    if (src != NULL)
    {
        // Clamp & recenter if needed
        const float ref_size = baked->ContainerFont->Sources[0]->SizePixels;
        const float offsets_scale = (ref_size != 0.0f) ? (baked->Size / ref_size) : 1.0f;
        advance_x = ImClamp(advance_x, src->GlyphMinAdvanceX * offsets_scale, src->GlyphMaxAdvanceX * offsets_scale);

        // Snap to pixel
        if (src->PixelSnapH)
            advance_x = IM_ROUND(advance_x);

        // Bake spacing
        advance_x += src->GlyphExtraAdvanceX;
    }

    ImFontBaked_BuildGrowIndex(baked, codepoint + 1);
    baked->IndexAdvanceX[codepoint] = advance_x;
}

// Copy to texture, post-process and queue update for backend
void ImFontAtlasBakedSetFontGlyphBitmap(ImFontAtlas* atlas, ImFontBaked* baked, ImFontConfig* src, ImFontGlyph* glyph, ImTextureRect* r, const unsigned char* src_pixels, ImTextureFormat src_fmt, int src_pitch)
{
    ImTextureData* tex = atlas->TexData;
    IM_ASSERT(r->x + r->w <= tex->Width && r->y + r->h <= tex->Height);
    ImFontAtlasTextureBlockConvert(src_pixels, src_fmt, src_pitch, (unsigned char*)tex->GetPixelsAt(r->x, r->y), tex->Format, tex->GetPitch(), r->w, r->h);
    ImFontAtlasPostProcessData pp_data = { atlas, baked->ContainerFont, src, baked, glyph, tex->GetPixelsAt(r->x, r->y), tex->Format, tex->GetPitch(), r->w, r->h };
    ImFontAtlasTextureBlockPostProcess(&pp_data);
    ImFontAtlasTextureBlockQueueUpload(atlas, tex, r->x, r->y, r->w, r->h);
}

void ImFont::AddRemapChar(ImWchar from_codepoint, ImWchar to_codepoint)
{
    RemapPairs.SetInt((ImGuiID)from_codepoint, (int)to_codepoint);
}

// Find glyph, load if necessary, return fallback if missing
ImFontGlyph* ImFontBaked::FindGlyph(ImWchar c)
{
    if (c < (size_t)IndexLookup.Size) IM_LIKELY
    {
        const int i = (int)IndexLookup.Data[c];
        if (i == IM_FONTGLYPH_INDEX_NOT_FOUND)
            return &Glyphs.Data[FallbackGlyphIndex];
        if (i != IM_FONTGLYPH_INDEX_UNUSED)
            return &Glyphs.Data[i];
    }
    ImFontGlyph* glyph = ImFontBaked_BuildLoadGlyph(this, c, NULL);
    return glyph ? glyph : &Glyphs.Data[FallbackGlyphIndex];
}

// Attempt to load but when missing, return NULL instead of FallbackGlyph
ImFontGlyph* ImFontBaked::FindGlyphNoFallback(ImWchar c)
{
    if (c < (size_t)IndexLookup.Size) IM_LIKELY
    {
        const int i = (int)IndexLookup.Data[c];
        if (i == IM_FONTGLYPH_INDEX_NOT_FOUND)
            return NULL;
        if (i != IM_FONTGLYPH_INDEX_UNUSED)
            return &Glyphs.Data[i];
    }
    LoadNoFallback = true; // This is actually a rare call, not done in hot-loop, so we prioritize not adding extra cruft to ImFontBaked_BuildLoadGlyph() call sites.
    ImFontGlyph* glyph = ImFontBaked_BuildLoadGlyph(this, c, NULL);
    LoadNoFallback = false;
    return glyph;
}

bool ImFontBaked::IsGlyphLoaded(ImWchar c)
{
    if (c < (size_t)IndexLookup.Size) IM_LIKELY
    {
        const int i = (int)IndexLookup.Data[c];
        if (i == IM_FONTGLYPH_INDEX_NOT_FOUND)
            return false;
        if (i != IM_FONTGLYPH_INDEX_UNUSED)
            return true;
    }
    return false;
}

// This is not fast query
bool ImFont::IsGlyphInFont(ImWchar c)
{
    ImFontAtlas* atlas = ContainerAtlas;
    ImFontAtlas_FontHookRemapCodepoint(atlas, this, &c);
    for (ImFontConfig* src : Sources)
    {
        const ImFontLoader* loader = src->FontLoader ? src->FontLoader : atlas->FontLoader;
        if (loader->FontSrcContainsGlyph != NULL && loader->FontSrcContainsGlyph(atlas, src, c))
            return true;
    }
    return false;
}

// This is manually inlined in CalcTextSizeA() and CalcWordWrapPosition(), with a non-inline call to BuildLoadGlyphGetAdvanceOrFallback().
IM_MSVC_RUNTIME_CHECKS_OFF
float ImFontBaked::GetCharAdvance(ImWchar c)
{
    if ((int)c < IndexAdvanceX.Size)
    {
        // Missing glyphs fitting inside index will have stored FallbackAdvanceX already.
        const float x = IndexAdvanceX.Data[c];
        if (x >= 0.0f)
            return x;
    }
    return ImFontBaked_BuildLoadGlyphAdvanceX(this, c);
}
IM_MSVC_RUNTIME_CHECKS_RESTORE

ImGuiID ImFontAtlasBakedGetId(ImGuiID font_id, float baked_size, float rasterizer_density)
{
    struct { ImGuiID FontId; float BakedSize; float RasterizerDensity; } hashed_data;
    hashed_data.FontId = font_id;
    hashed_data.BakedSize = baked_size;
    hashed_data.RasterizerDensity = rasterizer_density;
    return ImHashData(&hashed_data, sizeof(hashed_data));
}

// ImFontBaked pointers are valid for the entire frame but shall never be kept between frames.
ImFontBaked* ImFont::GetFontBaked(float size, float density)
{
    ImFontBaked* baked = LastBaked;

    // Round font size
    // - ImGui::PushFont() will already round, but other paths calling GetFontBaked() directly also needs it (e.g. ImFontAtlasBuildPreloadAllGlyphRanges)
    size = ImGui::GetRoundedFontSize(size);

    if (density < 0.0f)
        density = CurrentRasterizerDensity;
    if (baked && baked->Size == size && baked->RasterizerDensity == density)
        return baked;

    ImFontAtlas* atlas = ContainerAtlas;
    ImFontAtlasBuilder* builder = atlas->Builder;
    baked = ImFontAtlasBakedGetOrAdd(atlas, this, size, density);
    if (baked == NULL)
        return NULL;
    baked->LastUsedFrame = builder->FrameCount;
    LastBaked = baked;
    return baked;
}

ImFontBaked* ImFontAtlasBakedGetOrAdd(ImFontAtlas* atlas, ImFont* font, float font_size, float font_rasterizer_density)
{
    // FIXME-NEWATLAS: Design for picking a nearest size based on some criteria?
    // FIXME-NEWATLAS: Altering font density won't work right away.
    IM_ASSERT(font_size > 0.0f && font_rasterizer_density > 0.0f);
    ImGuiID baked_id = ImFontAtlasBakedGetId(font->FontId, font_size, font_rasterizer_density);
    ImFontAtlasBuilder* builder = atlas->Builder;
    ImFontBaked** p_baked_in_map = (ImFontBaked**)builder->BakedMap.GetVoidPtrRef(baked_id);
    ImFontBaked* baked = *p_baked_in_map;
    if (baked != NULL)
    {
        IM_ASSERT(baked->Size == font_size && baked->ContainerFont == font && baked->BakedId == baked_id);
        return baked;
    }

    // If atlas is locked, find closest match
    // FIXME-OPT: This is not an optimal query.
    if ((font->Flags & ImFontFlags_LockBakedSizes) || atlas->Locked)
    {
        baked = ImFontAtlasBakedGetClosestMatch(atlas, font, font_size, font_rasterizer_density);
        if (baked != NULL)
            return baked;
        if (atlas->Locked)
        {
            IM_ASSERT(!atlas->Locked && "Cannot use dynamic font size with a locked ImFontAtlas!"); // Locked because rendering backend does not support ImGuiBackendFlags_RendererHasTextures!
            return NULL;
        }
    }

    // Create new
    baked = ImFontAtlasBakedAdd(atlas, font, font_size, font_rasterizer_density, baked_id);
    *p_baked_in_map = baked; // To avoid 'builder->BakedMap.SetVoidPtr(baked_id, baked);' while we can.
    return baked;
}

// Trim trailing space and find beginning of next line
const char* ImTextCalcWordWrapNextLineStart(const char* text, const char* text_end, ImDrawTextFlags flags)
{
    if ((flags & ImDrawTextFlags_WrapKeepBlanks) == 0)
        while (text < text_end && ImCharIsBlankA(*text))
            text++;
    if (*text == '\n')
        text++;
    return text;
}

// Simple word-wrapping for English, not full-featured. Please submit failing cases!
// This will return the next location to wrap from. If no wrapping if necessary, this will fast-forward to e.g. text_end.
// FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)
const char* ImFontCalcWordWrapPositionEx(ImFont* font, float size, const char* text, const char* text_end, float wrap_width, ImDrawTextFlags flags)
{
    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    ImFontBaked* baked = font->GetFontBaked(size);
    const float scale = size / baked->Size;

    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;
    wrap_width /= scale; // We work with unscaled widths to avoid scaling every characters

    const char* word_end = text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = text;
    IM_ASSERT(text_end != NULL);
    while (s < text_end)
    {
        unsigned int c = (unsigned int)*s;
        const char* next_s;
        if (c < 0x80)
            next_s = s + 1;
        else
            next_s = s + ImTextCharFromUtf8(&c, s, text_end);

        if (c < 32)
        {
            if (c == '\n')
                return s; // Direct return, skip "Wrap_width is too small to fit anything" path.
            if (c == '\r')
            {
                s = next_s;
                continue;
            }
        }

        // Optimized inline version of 'float char_width = GetCharAdvance((ImWchar)c);'
        float char_width = (c < (unsigned int)baked->IndexAdvanceX.Size) ? baked->IndexAdvanceX.Data[c] : -1.0f;
        if (char_width < 0.0f)
            char_width = BuildLoadGlyphGetAdvanceOrFallback(baked, c);

        if (ImCharIsBlankW(c))
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end = s;
            }
            blank_width += char_width;
            inside_word = false;
        }
        else
        {
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            }
            else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                if ((flags & ImDrawTextFlags_WrapKeepBlanks) && line_width <= wrap_width)
                    prev_word_end = s;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = (c != '.' && c != ',' && c != ';' && c != '!' && c != '?' && c != '\"' && c != 0x3001 && c != 0x3002);
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width > wrap_width)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < wrap_width)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
    // +1 may not be a character start point in UTF-8 but it's ok because caller loops use (text >= word_wrap_eol).
    if (s == text && text < text_end)
        return s + ImTextCountUtf8BytesFromChar(s, text_end);
    return s;
}

const char* ImFont::CalcWordWrapPosition(float size, const char* text, const char* text_end, float wrap_width)
{
    return ImFontCalcWordWrapPositionEx(this, size, text, text_end, wrap_width, ImDrawTextFlags_None);
}

ImVec2 ImFontCalcTextSizeEx(ImFont* font, float size, float max_width, float wrap_width, const char* text_begin, const char* text_end_display, const char* text_end, const char** out_remaining, ImVec2* out_offset, ImDrawTextFlags flags)
{
    if (!text_end)
        text_end = text_begin + ImStrlen(text_begin); // FIXME-OPT: Need to avoid this.
    if (!text_end_display)
        text_end_display = text_end;

    ImFontBaked* baked = font->GetFontBaked(size);
    const float line_height = size;
    const float scale = line_height / baked->Size;

    ImVec2 text_size = ImVec2(0, 0);
    float line_width = 0.0f;

    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    const char* s = text_begin;
    while (s < text_end_display)
    {
        // Word-wrapping
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
                word_wrap_eol = ImFontCalcWordWrapPositionEx(font, size, s, text_end, wrap_width - line_width, flags);

            if (s >= word_wrap_eol)
            {
                if (text_size.x < line_width)
                    text_size.x = line_width;
                text_size.y += line_height;
                line_width = 0.0f;
                s = ImTextCalcWordWrapNextLineStart(s, text_end, flags); // Wrapping skips upcoming blanks
                if (flags & ImDrawTextFlags_StopOnNewLine)
                    break;
                word_wrap_eol = NULL;
                continue;
            }
        }

        // Decode and advance source
        const char* prev_s = s;
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
            s += 1;
        else
            s += ImTextCharFromUtf8(&c, s, text_end);

        if (c == '\n')
        {
            text_size.x = ImMax(text_size.x, line_width);
            text_size.y += line_height;
            line_width = 0.0f;
            if (flags & ImDrawTextFlags_StopOnNewLine)
                break;
            continue;
        }
        if (c == '\r')
            continue;

        // Optimized inline version of 'float char_width = GetCharAdvance((ImWchar)c);'
        float char_width = (c < (unsigned int)baked->IndexAdvanceX.Size) ? baked->IndexAdvanceX.Data[c] : -1.0f;
        if (char_width < 0.0f)
            char_width = BuildLoadGlyphGetAdvanceOrFallback(baked, c);
        char_width *= scale;

        if (line_width + char_width >= max_width)
        {
            s = prev_s;
            break;
        }

        line_width += char_width;
    }

    if (text_size.x < line_width)
        text_size.x = line_width;

    if (out_offset != NULL)
        *out_offset = ImVec2(line_width, text_size.y + line_height);  // offset allow for the possibility of sitting after a trailing \n

    if (line_width > 0 || text_size.y == 0.0f)                        // whereas size.y will ignore the trailing \n
        text_size.y += line_height;

    if (out_remaining != NULL)
        *out_remaining = s;

    return text_size;
}

ImVec2 ImFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** out_remaining)
{
    return ImFontCalcTextSizeEx(this, size, max_width, wrap_width, text_begin, text_end, text_end, out_remaining, NULL, ImDrawTextFlags_None);
}

// Note: as with every ImDrawList drawing function, this expects that the font atlas texture is bound.
void ImFont::RenderChar(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, ImWchar c, const ImVec4* cpu_fine_clip)
{
    ImFontBaked* baked = GetFontBaked(size);
    const ImFontGlyph* glyph = baked->FindGlyph(c);
    if (!glyph || !glyph->Visible)
        return;
    if (glyph->Colored)
        col |= ~IM_COL32_A_MASK;
    float scale = (size >= 0.0f) ? (size / baked->Size) : 1.0f;
    float x = IM_TRUNC(pos.x);
    float y = IM_TRUNC(pos.y);

    float x1 = x + glyph->X0 * scale;
    float x2 = x + glyph->X1 * scale;
    if (cpu_fine_clip && (x1 > cpu_fine_clip->z || x2 < cpu_fine_clip->x))
        return;
    float y1 = y + glyph->Y0 * scale;
    float y2 = y + glyph->Y1 * scale;
    float u1 = glyph->U0;
    float v1 = glyph->V0;
    float u2 = glyph->U1;
    float v2 = glyph->V1;

    // Always CPU fine clip. Code extracted from RenderText().
    // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
    if (cpu_fine_clip != NULL)
    {
        if (x1 < cpu_fine_clip->x) { u1 = u1 + (1.0f - (x2 - cpu_fine_clip->x) / (x2 - x1)) * (u2 - u1); x1 = cpu_fine_clip->x; }
        if (y1 < cpu_fine_clip->y) { v1 = v1 + (1.0f - (y2 - cpu_fine_clip->y) / (y2 - y1)) * (v2 - v1); y1 = cpu_fine_clip->y; }
        if (x2 > cpu_fine_clip->z) { u2 = u1 + ((cpu_fine_clip->z - x1) / (x2 - x1)) * (u2 - u1); x2 = cpu_fine_clip->z; }
        if (y2 > cpu_fine_clip->w) { v2 = v1 + ((cpu_fine_clip->w - y1) / (y2 - y1)) * (v2 - v1); y2 = cpu_fine_clip->w; }
        if (y1 >= y2)
            return;
    }
    draw_list->PrimReserve(6, 4);
    draw_list->PrimRectUV(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(u1, v1), ImVec2(u2, v2), col);
}

// Note: as with every ImDrawList drawing function, this expects that the font atlas texture is bound.
// DO NOT CALL DIRECTLY THIS WILL CHANGE WILDLY IN 2025-2025. Use ImDrawList::AddText().
void ImFont::RenderText(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, float wrap_width, ImDrawTextFlags flags)
{
    // Align to be pixel perfect
begin:
    float x = IM_TRUNC(pos.x);
    float y = IM_TRUNC(pos.y);
    if (y > clip_rect.w)
        return;

    if (!text_end)
        text_end = text_begin + ImStrlen(text_begin); // ImGui:: functions generally already provides a valid text_end, so this is merely to handle direct calls.

    const float line_height = size;
    ImFontBaked* baked = GetFontBaked(size);

    const float scale = size / baked->Size;
    const float origin_x = x;
    const bool word_wrap_enabled = (wrap_width > 0.0f);

    // Fast-forward to first visible line
    const char* s = text_begin;
    if (y + line_height < clip_rect.y)
        while (y + line_height < clip_rect.y && s < text_end)
        {
            const char* line_end = (const char*)ImMemchr(s, '\n', text_end - s);
            if (word_wrap_enabled)
            {
                // FIXME-OPT: This is not optimal as do first do a search for \n before calling CalcWordWrapPosition().
                // If the specs for CalcWordWrapPosition() were reworked to optionally return on \n we could combine both.
                // However it is still better than nothing performing the fast-forward!
                s = ImFontCalcWordWrapPositionEx(this, size, s, line_end ? line_end : text_end, wrap_width, flags);
                s = ImTextCalcWordWrapNextLineStart(s, text_end, flags);
            }
            else
            {
                s = line_end ? line_end + 1 : text_end;
            }
            y += line_height;
        }

    // For large text, scan for the last visible line in order to avoid over-reserving in the call to PrimReserve()
    // Note that very large horizontal line will still be affected by the issue (e.g. a one megabyte string buffer without a newline will likely crash atm)
    if (text_end - s > 10000 && !word_wrap_enabled)
    {
        const char* s_end = s;
        float y_end = y;
        while (y_end < clip_rect.w && s_end < text_end)
        {
            s_end = (const char*)ImMemchr(s_end, '\n', text_end - s_end);
            s_end = s_end ? s_end + 1 : text_end;
            y_end += line_height;
        }
        text_end = s_end;
    }
    if (s == text_end)
        return;

    // Reserve vertices for remaining worse case (over-reserving is useful and easily amortized)
    const int vtx_count_max = (int)(text_end - s) * 4;
    const int idx_count_max = (int)(text_end - s) * 6;
    const int idx_expected_size = draw_list->IdxBuffer.Size + idx_count_max;
    draw_list->PrimReserve(idx_count_max, vtx_count_max);
    ImDrawVert*  vtx_write = draw_list->_VtxWritePtr;
    ImDrawIdx*   idx_write = draw_list->_IdxWritePtr;
    unsigned int vtx_index = draw_list->_VtxCurrentIdx;
    const int cmd_count = draw_list->CmdBuffer.Size;
    const bool cpu_fine_clip = (flags & ImDrawTextFlags_CpuFineClip) != 0;

    const ImU32 col_untinted = col | ~IM_COL32_A_MASK;
    const char* word_wrap_eol = NULL;

    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
                word_wrap_eol = ImFontCalcWordWrapPositionEx(this, size, s, text_end, wrap_width - (x - origin_x), flags);

            if (s >= word_wrap_eol)
            {
                x = origin_x;
                y += line_height;
                if (y > clip_rect.w)
                    break; // break out of main loop
                word_wrap_eol = NULL;
                s = ImTextCalcWordWrapNextLineStart(s, text_end, flags); // Wrapping skips upcoming blanks
                continue;
            }
        }

        // Decode and advance source
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
            s += 1;
        else
            s += ImTextCharFromUtf8(&c, s, text_end);

        if (c < 32)
        {
            if (c == '\n')
            {
                x = origin_x;
                y += line_height;
                if (y > clip_rect.w)
                    break; // break out of main loop
                continue;
            }
            if (c == '\r')
                continue;
        }

        const ImFontGlyph* glyph = baked->FindGlyph((ImWchar)c);
        //if (glyph == NULL)
        //    continue;

        float char_width = glyph->AdvanceX * scale;
        if (glyph->Visible)
        {
            // We don't do a second finer clipping test on the Y axis as we've already skipped anything before clip_rect.y and exit once we pass clip_rect.w
            float x1 = x + glyph->X0 * scale;
            float x2 = x + glyph->X1 * scale;
            float y1 = y + glyph->Y0 * scale;
            float y2 = y + glyph->Y1 * scale;
            if (x1 <= clip_rect.z && x2 >= clip_rect.x)
            {
                // Render a character
                float u1 = glyph->U0;
                float v1 = glyph->V0;
                float u2 = glyph->U1;
                float v2 = glyph->V1;

                // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
                if (cpu_fine_clip)
                {
                    if (x1 < clip_rect.x)
                    {
                        u1 = u1 + (1.0f - (x2 - clip_rect.x) / (x2 - x1)) * (u2 - u1);
                        x1 = clip_rect.x;
                    }
                    if (y1 < clip_rect.y)
                    {
                        v1 = v1 + (1.0f - (y2 - clip_rect.y) / (y2 - y1)) * (v2 - v1);
                        y1 = clip_rect.y;
                    }
                    if (x2 > clip_rect.z)
                    {
                        u2 = u1 + ((clip_rect.z - x1) / (x2 - x1)) * (u2 - u1);
                        x2 = clip_rect.z;
                    }
                    if (y2 > clip_rect.w)
                    {
                        v2 = v1 + ((clip_rect.w - y1) / (y2 - y1)) * (v2 - v1);
                        y2 = clip_rect.w;
                    }
                    if (y1 >= y2)
                    {
                        x += char_width;
                        continue;
                    }
                }

                // Support for untinted glyphs
                ImU32 glyph_col = glyph->Colored ? col_untinted : col;

                // We are NOT calling PrimRectUV() here because non-inlined causes too much overhead in a debug builds. Inlined here:
                {
                    vtx_write[0].pos.x = x1; vtx_write[0].pos.y = y1; vtx_write[0].col = glyph_col; vtx_write[0].uv.x = u1; vtx_write[0].uv.y = v1;
                    vtx_write[1].pos.x = x2; vtx_write[1].pos.y = y1; vtx_write[1].col = glyph_col; vtx_write[1].uv.x = u2; vtx_write[1].uv.y = v1;
                    vtx_write[2].pos.x = x2; vtx_write[2].pos.y = y2; vtx_write[2].col = glyph_col; vtx_write[2].uv.x = u2; vtx_write[2].uv.y = v2;
                    vtx_write[3].pos.x = x1; vtx_write[3].pos.y = y2; vtx_write[3].col = glyph_col; vtx_write[3].uv.x = u1; vtx_write[3].uv.y = v2;
                    idx_write[0] = (ImDrawIdx)(vtx_index); idx_write[1] = (ImDrawIdx)(vtx_index + 1); idx_write[2] = (ImDrawIdx)(vtx_index + 2);
                    idx_write[3] = (ImDrawIdx)(vtx_index); idx_write[4] = (ImDrawIdx)(vtx_index + 2); idx_write[5] = (ImDrawIdx)(vtx_index + 3);
                    vtx_write += 4;
                    vtx_index += 4;
                    idx_write += 6;
                }
            }
        }
        x += char_width;
    }

    // Edge case: calling RenderText() with unloaded glyphs triggering texture change. It doesn't happen via ImGui:: calls because CalcTextSize() is always used.
    if (cmd_count != draw_list->CmdBuffer.Size) //-V547
    {
        IM_ASSERT(draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ElemCount == 0);
        draw_list->CmdBuffer.pop_back();
        draw_list->PrimUnreserve(idx_count_max, vtx_count_max);
        draw_list->AddDrawCmd();
        //IMGUI_DEBUG_LOG("RenderText: cancel and retry to missing glyphs.\n"); // [DEBUG]
        //draw_list->AddRectFilled(pos, pos + ImVec2(10, 10), IM_COL32(255, 0, 0, 255)); // [DEBUG]
        goto begin;
        //RenderText(draw_list, size, pos, col, clip_rect, text_begin, text_end, wrap_width, cpu_fine_clip); // FIXME-OPT: Would a 'goto begin' be better for code-gen?
        //return;
    }

    // Give back unused vertices (clipped ones, blanks) ~ this is essentially a PrimUnreserve() action.
    draw_list->VtxBuffer.Size = (int)(vtx_write - draw_list->VtxBuffer.Data); // Same as calling shrink()
    draw_list->IdxBuffer.Size = (int)(idx_write - draw_list->IdxBuffer.Data);
    draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ElemCount -= (idx_expected_size - draw_list->IdxBuffer.Size);
    draw_list->_VtxWritePtr = vtx_write;
    draw_list->_IdxWritePtr = idx_write;
    draw_list->_VtxCurrentIdx = vtx_index;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGui Internal Render Helpers
//-----------------------------------------------------------------------------
// Vaguely redesigned to stop accessing ImGui global state:
// - RenderArrow()
// - RenderBullet()
// - RenderCheckMark()
// - RenderArrowPointingAt()
// - RenderRectFilledRangeH()
// - RenderRectFilledWithHole()
//-----------------------------------------------------------------------------
// Function in need of a redesign (legacy mess)
// - RenderColorRectWithAlphaCheckerboard()
//-----------------------------------------------------------------------------

// Render an arrow aimed to be aligned with text (p_min is a position in the same space text would be positioned). To e.g. denote expanded/collapsed state
void ImGui::RenderArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float scale)
{
    const float h = draw_list->_Data->FontSize * 1.00f;
    float r = h * 0.40f * scale;
    ImVec2 center = pos + ImVec2(h * 0.50f, h * 0.50f * scale);

    ImVec2 a, b, c;
    switch (dir)
    {
    case ImGuiDir_Up:
    case ImGuiDir_Down:
        if (dir == ImGuiDir_Up) r = -r;
        a = ImVec2(+0.000f, +0.750f) * r;
        b = ImVec2(-0.866f, -0.750f) * r;
        c = ImVec2(+0.866f, -0.750f) * r;
        break;
    case ImGuiDir_Left:
    case ImGuiDir_Right:
        if (dir == ImGuiDir_Left) r = -r;
        a = ImVec2(+0.750f, +0.000f) * r;
        b = ImVec2(-0.750f, +0.866f) * r;
        c = ImVec2(-0.750f, -0.866f) * r;
        break;
    case ImGuiDir_None:
    case ImGuiDir_COUNT:
        IM_ASSERT(0);
        break;
    }
    draw_list->AddTriangleFilled(center + a, center + b, center + c, col);
}

void ImGui::RenderBullet(ImDrawList* draw_list, ImVec2 pos, ImU32 col)
{
    // FIXME-OPT: This should be baked in font now that it's easier.
    float font_size = draw_list->_Data->FontSize;
    draw_list->AddCircleFilled(pos, font_size * 0.20f, col, (font_size < 22) ? 8 : (font_size < 40) ? 12 : 0); // Hardcode optimal/nice tessellation threshold
}

void ImGui::RenderCheckMark(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float sz)
{
    float thickness = ImMax(sz / 5.0f, 1.0f);
    sz -= thickness * 0.5f;
    pos += ImVec2(thickness * 0.25f, thickness * 0.25f);

    float third = sz / 3.0f;
    float bx = pos.x + third;
    float by = pos.y + sz - third * 0.5f;
    draw_list->PathLineTo(ImVec2(bx - third, by - third));
    draw_list->PathLineTo(ImVec2(bx, by));
    draw_list->PathLineTo(ImVec2(bx + third * 2.0f, by - third * 2.0f));
    draw_list->PathStroke(col, 0, thickness);
}

// Render an arrow. 'pos' is position of the arrow tip. half_sz.x is length from base to tip. half_sz.y is length on each side.
void ImGui::RenderArrowPointingAt(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 col)
{
    switch (direction)
    {
    case ImGuiDir_Left:  draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Right: draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_Up:    draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Down:  draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_None: case ImGuiDir_COUNT: break; // Fix warnings
    }
}

static inline float ImAcos01(float x)
{
    if (x <= 0.0f) return IM_PI * 0.5f;
    if (x >= 1.0f) return 0.0f;
    return ImAcos(x);
    //return (-0.69813170079773212f * x * x - 0.87266462599716477f) * x + 1.5707963267948966f; // Cheap approximation, may be enough for what we do.
}

// FIXME: Cleanup and move code to ImDrawList.
void ImGui::RenderRectFilledRangeH(ImDrawList* draw_list, const ImRect& rect, ImU32 col, float x_start_norm, float x_end_norm, float rounding)
{
    if (x_end_norm == x_start_norm)
        return;
    if (x_start_norm > x_end_norm)
        ImSwap(x_start_norm, x_end_norm);

    ImVec2 p0 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_start_norm), rect.Min.y);
    ImVec2 p1 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_end_norm), rect.Max.y);
    if (rounding == 0.0f)
    {
        draw_list->AddRectFilled(p0, p1, col, 0.0f);
        return;
    }

    rounding = ImClamp(ImMin((rect.Max.x - rect.Min.x) * 0.5f, (rect.Max.y - rect.Min.y) * 0.5f) - 1.0f, 0.0f, rounding);
    const float inv_rounding = 1.0f / rounding;
    const float arc0_b = ImAcos01(1.0f - (p0.x - rect.Min.x) * inv_rounding);
    const float arc0_e = ImAcos01(1.0f - (p1.x - rect.Min.x) * inv_rounding);
    const float half_pi = IM_PI * 0.5f; // We will == compare to this because we know this is the exact value ImAcos01 can return.
    const float x0 = ImMax(p0.x, rect.Min.x + rounding);
    if (arc0_b == arc0_e)
    {
        draw_list->PathLineTo(ImVec2(x0, p1.y));
        draw_list->PathLineTo(ImVec2(x0, p0.y));
    }
    else if (arc0_b == 0.0f && arc0_e == half_pi)
    {
        draw_list->PathArcToFast(ImVec2(x0, p1.y - rounding), rounding, 3, 6); // BL
        draw_list->PathArcToFast(ImVec2(x0, p0.y + rounding), rounding, 6, 9); // TR
    }
    else
    {
        draw_list->PathArcTo(ImVec2(x0, p1.y - rounding), rounding, IM_PI - arc0_e, IM_PI - arc0_b); // BL
        draw_list->PathArcTo(ImVec2(x0, p0.y + rounding), rounding, IM_PI + arc0_b, IM_PI + arc0_e); // TR
    }
    if (p1.x > rect.Min.x + rounding)
    {
        const float arc1_b = ImAcos01(1.0f - (rect.Max.x - p1.x) * inv_rounding);
        const float arc1_e = ImAcos01(1.0f - (rect.Max.x - p0.x) * inv_rounding);
        const float x1 = ImMin(p1.x, rect.Max.x - rounding);
        if (arc1_b == arc1_e)
        {
            draw_list->PathLineTo(ImVec2(x1, p0.y));
            draw_list->PathLineTo(ImVec2(x1, p1.y));
        }
        else if (arc1_b == 0.0f && arc1_e == half_pi)
        {
            draw_list->PathArcToFast(ImVec2(x1, p0.y + rounding), rounding, 9, 12); // TR
            draw_list->PathArcToFast(ImVec2(x1, p1.y - rounding), rounding, 0, 3);  // BR
        }
        else
        {
            draw_list->PathArcTo(ImVec2(x1, p0.y + rounding), rounding, -arc1_e, -arc1_b); // TR
            draw_list->PathArcTo(ImVec2(x1, p1.y - rounding), rounding, +arc1_b, +arc1_e); // BR
        }
    }
    draw_list->PathFillConvex(col);
}

void ImGui::RenderRectFilledWithHole(ImDrawList* draw_list, const ImRect& outer, const ImRect& inner, ImU32 col, float rounding)
{
    const bool fill_L = (inner.Min.x > outer.Min.x);
    const bool fill_R = (inner.Max.x < outer.Max.x);
    const bool fill_U = (inner.Min.y > outer.Min.y);
    const bool fill_D = (inner.Max.y < outer.Max.y);
    if (fill_L) draw_list->AddRectFilled(ImVec2(outer.Min.x, inner.Min.y), ImVec2(inner.Min.x, inner.Max.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_U ? 0 : ImDrawFlags_RoundCornersTopLeft)    | (fill_D ? 0 : ImDrawFlags_RoundCornersBottomLeft));
    if (fill_R) draw_list->AddRectFilled(ImVec2(inner.Max.x, inner.Min.y), ImVec2(outer.Max.x, inner.Max.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_U ? 0 : ImDrawFlags_RoundCornersTopRight)   | (fill_D ? 0 : ImDrawFlags_RoundCornersBottomRight));
    if (fill_U) draw_list->AddRectFilled(ImVec2(inner.Min.x, outer.Min.y), ImVec2(inner.Max.x, inner.Min.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_L ? 0 : ImDrawFlags_RoundCornersTopLeft)    | (fill_R ? 0 : ImDrawFlags_RoundCornersTopRight));
    if (fill_D) draw_list->AddRectFilled(ImVec2(inner.Min.x, inner.Max.y), ImVec2(inner.Max.x, outer.Max.y), col, rounding, ImDrawFlags_RoundCornersNone | (fill_L ? 0 : ImDrawFlags_RoundCornersBottomLeft) | (fill_R ? 0 : ImDrawFlags_RoundCornersBottomRight));
    if (fill_L && fill_U) draw_list->AddRectFilled(ImVec2(outer.Min.x, outer.Min.y), ImVec2(inner.Min.x, inner.Min.y), col, rounding, ImDrawFlags_RoundCornersTopLeft);
    if (fill_R && fill_U) draw_list->AddRectFilled(ImVec2(inner.Max.x, outer.Min.y), ImVec2(outer.Max.x, inner.Min.y), col, rounding, ImDrawFlags_RoundCornersTopRight);
    if (fill_L && fill_D) draw_list->AddRectFilled(ImVec2(outer.Min.x, inner.Max.y), ImVec2(inner.Min.x, outer.Max.y), col, rounding, ImDrawFlags_RoundCornersBottomLeft);
    if (fill_R && fill_D) draw_list->AddRectFilled(ImVec2(inner.Max.x, inner.Max.y), ImVec2(outer.Max.x, outer.Max.y), col, rounding, ImDrawFlags_RoundCornersBottomRight);
}

// Helper for ColorPicker4()
// NB: This is rather brittle and will show artifact when rounding this enabled if rounded corners overlap multiple cells. Caller currently responsible for avoiding that.
// Spent a non reasonable amount of time trying to getting this right for ColorButton with rounding+anti-aliasing+ImGuiColorEditFlags_HalfAlphaPreview flag + various grid sizes and offsets, and eventually gave up... probably more reasonable to disable rounding altogether.
// FIXME: uses ImGui::GetColorU32
void ImGui::RenderColorRectWithAlphaCheckerboard(ImDrawList* draw_list, ImVec2 p_min, ImVec2 p_max, ImU32 col, float grid_step, ImVec2 grid_off, float rounding, ImDrawFlags flags)
{
    if ((flags & ImDrawFlags_RoundCornersMask_) == 0)
        flags = ImDrawFlags_RoundCornersDefault_;
    if (((col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT) < 0xFF)
    {
        ImU32 col_bg1 = GetColorU32(ImAlphaBlendColors(IM_COL32(204, 204, 204, 255), col));
        ImU32 col_bg2 = GetColorU32(ImAlphaBlendColors(IM_COL32(128, 128, 128, 255), col));
        draw_list->AddRectFilled(p_min, p_max, col_bg1, rounding, flags);

        int yi = 0;
        for (float y = p_min.y + grid_off.y; y < p_max.y; y += grid_step, yi++)
        {
            float y1 = ImClamp(y, p_min.y, p_max.y), y2 = ImMin(y + grid_step, p_max.y);
            if (y2 <= y1)
                continue;
            for (float x = p_min.x + grid_off.x + (yi & 1) * grid_step; x < p_max.x; x += grid_step * 2.0f)
            {
                float x1 = ImClamp(x, p_min.x, p_max.x), x2 = ImMin(x + grid_step, p_max.x);
                if (x2 <= x1)
                    continue;
                ImDrawFlags cell_flags = ImDrawFlags_RoundCornersNone;
                if (y1 <= p_min.y) { if (x1 <= p_min.x) cell_flags |= ImDrawFlags_RoundCornersTopLeft; if (x2 >= p_max.x) cell_flags |= ImDrawFlags_RoundCornersTopRight; }
                if (y2 >= p_max.y) { if (x1 <= p_min.x) cell_flags |= ImDrawFlags_RoundCornersBottomLeft; if (x2 >= p_max.x) cell_flags |= ImDrawFlags_RoundCornersBottomRight; }

                // Combine flags
                cell_flags = (flags == ImDrawFlags_RoundCornersNone || cell_flags == ImDrawFlags_RoundCornersNone) ? ImDrawFlags_RoundCornersNone : (cell_flags & flags);
                draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col_bg2, rounding, cell_flags);
            }
        }
    }
    else
    {
        draw_list->AddRectFilled(p_min, p_max, col, rounding, flags);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Decompression code
//-----------------------------------------------------------------------------
// Compressed with stb_compress() then converted to a C array and encoded as base85.
// Use the program in misc/fonts/binary_to_compressed_c.cpp to create the array from a TTF file.
// The purpose of encoding as base85 instead of "0x00,0x01,..." style is only save on _source code_ size.
// Decompression from stb.h (public domain) by Sean Barrett https://github.com/nothings/stb/blob/master/stb.h
//-----------------------------------------------------------------------------

static unsigned int stb_decompress_length(const unsigned char *input)
{
    return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

static unsigned char *stb__barrier_out_e, *stb__barrier_out_b;
static const unsigned char *stb__barrier_in_b;
static unsigned char *stb__dout;
static void stb__match(const unsigned char *data, unsigned int length)
{
    // INVERSE of memmove... write each byte before copying the next...
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_out_b) { stb__dout = stb__barrier_out_e+1; return; }
    while (length--) *stb__dout++ = *data++;
}

static void stb__lit(const unsigned char *data, unsigned int length)
{
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_in_b) { stb__dout = stb__barrier_out_e+1; return; }
    memcpy(stb__dout, data, length);
    stb__dout += length;
}

#define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
#define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
#define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

static const unsigned char *stb_decompress_token(const unsigned char *i)
{
    if (*i >= 0x20) { // use fewer if's for cases that expand small
        if (*i >= 0x80)       stb__match(stb__dout-i[1]-1, i[0] - 0x80 + 1), i += 2;
        else if (*i >= 0x40)  stb__match(stb__dout-(stb__in2(0) - 0x4000 + 1), i[2]+1), i += 3;
        else /* *i >= 0x20 */ stb__lit(i+1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
    } else { // more ifs for cases that expand large, since overhead is amortized
        if (*i >= 0x18)       stb__match(stb__dout-(stb__in3(0) - 0x180000 + 1), i[3]+1), i += 4;
        else if (*i >= 0x10)  stb__match(stb__dout-(stb__in3(0) - 0x100000 + 1), stb__in2(3)+1), i += 5;
        else if (*i >= 0x08)  stb__lit(i+2, stb__in2(0) - 0x0800 + 1), i += 2 + (stb__in2(0) - 0x0800 + 1);
        else if (*i == 0x07)  stb__lit(i+3, stb__in2(1) + 1), i += 3 + (stb__in2(1) + 1);
        else if (*i == 0x06)  stb__match(stb__dout-(stb__in3(1)+1), i[4]+1), i += 5;
        else if (*i == 0x04)  stb__match(stb__dout-(stb__in3(1)+1), stb__in2(4)+1), i += 6;
    }
    return i;
}

static unsigned int stb_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
    const unsigned long ADLER_MOD = 65521;
    unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
    unsigned long blocklen = buflen % 5552;

    unsigned long i;
    while (buflen) {
        for (i=0; i + 7 < blocklen; i += 8) {
            s1 += buffer[0], s2 += s1;
            s1 += buffer[1], s2 += s1;
            s1 += buffer[2], s2 += s1;
            s1 += buffer[3], s2 += s1;
            s1 += buffer[4], s2 += s1;
            s1 += buffer[5], s2 += s1;
            s1 += buffer[6], s2 += s1;
            s1 += buffer[7], s2 += s1;

            buffer += 8;
        }

        for (; i < blocklen; ++i)
            s1 += *buffer++, s2 += s1;

        s1 %= ADLER_MOD, s2 %= ADLER_MOD;
        buflen -= blocklen;
        blocklen = 5552;
    }
    return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static unsigned int stb_decompress(unsigned char *output, const unsigned char *i, unsigned int /*length*/)
{
    if (stb__in4(0) != 0x57bC0000) return 0;
    if (stb__in4(4) != 0)          return 0; // error! stream is > 4GB
    const unsigned int olen = stb_decompress_length(i);
    stb__barrier_in_b = i;
    stb__barrier_out_e = output + olen;
    stb__barrier_out_b = output;
    i += 16;

    stb__dout = output;
    for (;;) {
        const unsigned char *old_i = i;
        i = stb_decompress_token(i);
        if (i == old_i) {
            if (*i == 0x05 && i[1] == 0xfa) {
                IM_ASSERT(stb__dout == output + olen);
                if (stb__dout != output + olen) return 0;
                if (stb_adler32(1, output, olen) != (unsigned int) stb__in4(2))
                    return 0;
                return olen;
            } else {
                IM_ASSERT(0); /* NOTREACHED */
                return 0;
            }
        }
        IM_ASSERT(stb__dout <= output + olen);
        if (stb__dout > output + olen)
            return 0;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Default font data (ProggyClean.ttf)
//-----------------------------------------------------------------------------
// ProggyClean.ttf
// Copyright (c) 2004, 2005 Tristan Grimmer
// MIT license (see License.txt in http://www.proggyfonts.net/index.php?menu=download)
// Download and more information at http://www.proggyfonts.net or http://upperboundsinteractive.com/fonts.php
// If you want a similar font which may be better scaled, consider using ProggyVector from the same author!
//-----------------------------------------------------------------------------

#ifndef IMGUI_DISABLE_DEFAULT_FONT

// File: 'ProggyClean.ttf' (41208 bytes)
// Exported using binary_to_compressed_c.exe -u8 "ProggyClean.ttf" proggy_clean_ttf
static const unsigned int proggy_clean_ttf_compressed_size = 9583;
static const unsigned char proggy_clean_ttf_compressed_data[9583] =
{
    87,188,0,0,0,0,0,0,0,0,160,248,0,4,0,0,55,0,1,0,0,0,12,0,128,0,3,0,64,79,83,47,50,136,235,116,144,0,0,1,72,130,21,44,78,99,109,97,112,2,18,35,117,0,0,3,160,130,19,36,82,99,118,116,
    32,130,23,130,2,33,4,252,130,4,56,2,103,108,121,102,18,175,137,86,0,0,7,4,0,0,146,128,104,101,97,100,215,145,102,211,130,27,32,204,130,3,33,54,104,130,16,39,8,66,1,195,0,0,1,4,130,
    15,59,36,104,109,116,120,138,0,126,128,0,0,1,152,0,0,2,6,108,111,99,97,140,115,176,216,0,0,5,130,30,41,2,4,109,97,120,112,1,174,0,218,130,31,32,40,130,16,44,32,110,97,109,101,37,89,
    187,150,0,0,153,132,130,19,44,158,112,111,115,116,166,172,131,239,0,0,155,36,130,51,44,210,112,114,101,112,105,2,1,18,0,0,4,244,130,47,32,8,132,203,46,1,0,0,60,85,233,213,95,15,60,
    245,0,3,8,0,131,0,34,183,103,119,130,63,43,0,0,189,146,166,215,0,0,254,128,3,128,131,111,130,241,33,2,0,133,0,32,1,130,65,38,192,254,64,0,0,3,128,131,16,130,5,32,1,131,7,138,3,33,2,
    0,130,17,36,1,1,0,144,0,130,121,130,23,38,2,0,8,0,64,0,10,130,9,32,118,130,9,130,6,32,0,130,59,33,1,144,131,200,35,2,188,2,138,130,16,32,143,133,7,37,1,197,0,50,2,0,131,0,33,4,9,131,
    5,145,3,43,65,108,116,115,0,64,0,0,32,172,8,0,131,0,35,5,0,1,128,131,77,131,3,33,3,128,191,1,33,1,128,130,184,35,0,0,128,0,130,3,131,11,32,1,130,7,33,0,128,131,1,32,1,136,9,32,0,132,
    15,135,5,32,1,131,13,135,27,144,35,32,1,149,25,131,21,32,0,130,0,32,128,132,103,130,35,132,39,32,0,136,45,136,97,133,17,130,5,33,0,0,136,19,34,0,128,1,133,13,133,5,32,128,130,15,132,
    131,32,3,130,5,32,3,132,27,144,71,32,0,133,27,130,29,130,31,136,29,131,63,131,3,65,63,5,132,5,132,205,130,9,33,0,0,131,9,137,119,32,3,132,19,138,243,130,55,32,1,132,35,135,19,131,201,
    136,11,132,143,137,13,130,41,32,0,131,3,144,35,33,128,0,135,1,131,223,131,3,141,17,134,13,136,63,134,15,136,53,143,15,130,96,33,0,3,131,4,130,3,34,28,0,1,130,5,34,0,0,76,130,17,131,
    9,36,28,0,4,0,48,130,17,46,8,0,8,0,2,0,0,0,127,0,255,32,172,255,255,130,9,34,0,0,129,132,9,130,102,33,223,213,134,53,132,22,33,1,6,132,6,64,4,215,32,129,165,216,39,177,0,1,141,184,
    1,255,133,134,45,33,198,0,193,1,8,190,244,1,28,1,158,2,20,2,136,2,252,3,20,3,88,3,156,3,222,4,20,4,50,4,80,4,98,4,162,5,22,5,102,5,188,6,18,6,116,6,214,7,56,7,126,7,236,8,78,8,108,
    8,150,8,208,9,16,9,74,9,136,10,22,10,128,11,4,11,86,11,200,12,46,12,130,12,234,13,94,13,164,13,234,14,80,14,150,15,40,15,176,16,18,16,116,16,224,17,82,17,182,18,4,18,110,18,196,19,
    76,19,172,19,246,20,88,20,174,20,234,21,64,21,128,21,166,21,184,22,18,22,126,22,198,23,52,23,142,23,224,24,86,24,186,24,238,25,54,25,150,25,212,26,72,26,156,26,240,27,92,27,200,28,
    4,28,76,28,150,28,234,29,42,29,146,29,210,30,64,30,142,30,224,31,36,31,118,31,166,31,166,32,16,130,1,52,46,32,138,32,178,32,200,33,20,33,116,33,152,33,238,34,98,34,134,35,12,130,1,
    33,128,35,131,1,60,152,35,176,35,216,36,0,36,74,36,104,36,144,36,174,37,6,37,96,37,130,37,248,37,248,38,88,38,170,130,1,8,190,216,39,64,39,154,40,10,40,104,40,168,41,14,41,32,41,184,
    41,248,42,54,42,96,42,96,43,2,43,42,43,94,43,172,43,230,44,32,44,52,44,154,45,40,45,92,45,120,45,170,45,232,46,38,46,166,47,38,47,182,47,244,48,94,48,200,49,62,49,180,50,30,50,158,
    51,30,51,130,51,238,52,92,52,206,53,58,53,134,53,212,54,38,54,114,54,230,55,118,55,216,56,58,56,166,57,18,57,116,57,174,58,46,58,154,59,6,59,124,59,232,60,58,60,150,61,34,61,134,61,
    236,62,86,62,198,63,42,63,154,64,18,64,106,64,208,65,54,65,162,66,8,66,64,66,122,66,184,66,240,67,98,67,204,68,42,68,138,68,238,69,88,69,182,69,226,70,84,70,180,71,20,71,122,71,218,
    72,84,72,198,73,64,0,36,70,21,8,8,77,3,0,7,0,11,0,15,0,19,0,23,0,27,0,31,0,35,0,39,0,43,0,47,0,51,0,55,0,59,0,63,0,67,0,71,0,75,0,79,0,83,0,87,0,91,0,95,0,99,0,103,0,107,0,111,0,115,
    0,119,0,123,0,127,0,131,0,135,0,139,0,143,0,0,17,53,51,21,49,150,3,32,5,130,23,32,33,130,3,211,7,151,115,32,128,133,0,37,252,128,128,2,128,128,190,5,133,74,32,4,133,6,206,5,42,0,7,
    1,128,0,0,2,0,4,0,0,65,139,13,37,0,1,53,51,21,7,146,3,32,3,130,19,32,1,141,133,32,3,141,14,131,13,38,255,0,128,128,0,6,1,130,84,35,2,128,4,128,140,91,132,89,32,51,65,143,6,139,7,33,
    1,0,130,57,32,254,130,3,32,128,132,4,32,4,131,14,138,89,35,0,0,24,0,130,0,33,3,128,144,171,66,55,33,148,115,65,187,19,32,5,130,151,143,155,163,39,32,1,136,182,32,253,134,178,132,7,
    132,200,145,17,32,3,65,48,17,165,17,39,0,0,21,0,128,255,128,3,65,175,17,65,3,27,132,253,131,217,139,201,155,233,155,27,131,67,131,31,130,241,33,255,0,131,181,137,232,132,15,132,4,138,
    247,34,255,0,128,179,238,32,0,130,0,32,20,65,239,48,33,0,19,67,235,10,32,51,65,203,14,65,215,11,32,7,154,27,135,39,32,33,130,35,33,128,128,130,231,32,253,132,231,32,128,132,232,34,
    128,128,254,133,13,136,8,32,253,65,186,5,130,36,130,42,176,234,133,231,34,128,0,0,66,215,44,33,0,1,68,235,6,68,211,19,32,49,68,239,14,139,207,139,47,66,13,7,32,51,130,47,33,1,0,130,
    207,35,128,128,1,0,131,222,131,5,130,212,130,6,131,212,32,0,130,10,133,220,130,233,130,226,32,254,133,255,178,233,39,3,1,128,3,0,2,0,4,68,15,7,68,99,12,130,89,130,104,33,128,4,133,
    93,130,10,38,0,0,11,1,0,255,0,68,63,16,70,39,9,66,215,8,32,7,68,77,6,68,175,14,32,29,68,195,6,132,7,35,2,0,128,255,131,91,132,4,65,178,5,141,111,67,129,23,165,135,140,107,142,135,33,
    21,5,69,71,6,131,7,33,1,0,140,104,132,142,130,4,137,247,140,30,68,255,12,39,11,0,128,0,128,3,0,3,69,171,15,67,251,7,65,15,8,66,249,11,65,229,7,67,211,7,66,13,7,35,1,128,128,254,133,
    93,32,254,131,145,132,4,132,18,32,2,151,128,130,23,34,0,0,9,154,131,65,207,8,68,107,15,68,51,7,32,7,70,59,7,135,121,130,82,32,128,151,111,41,0,0,4,0,128,255,0,1,128,1,137,239,33,0,
    37,70,145,10,65,77,10,65,212,14,37,0,0,0,5,0,128,66,109,5,70,123,10,33,0,19,72,33,18,133,237,70,209,11,33,0,2,130,113,137,119,136,115,33,1,0,133,43,130,5,34,0,0,10,69,135,6,70,219,
    13,66,155,7,65,9,12,66,157,11,66,9,11,32,7,130,141,132,252,66,151,9,137,9,66,15,30,36,0,20,0,128,0,130,218,71,11,42,68,51,8,65,141,7,73,19,15,69,47,23,143,39,66,81,7,32,1,66,55,6,34,
    1,128,128,68,25,5,69,32,6,137,6,136,25,32,254,131,42,32,3,66,88,26,148,26,32,0,130,0,32,14,164,231,70,225,12,66,233,7,67,133,19,71,203,15,130,161,32,255,130,155,32,254,139,127,134,
    12,164,174,33,0,15,164,159,33,59,0,65,125,20,66,25,7,32,5,68,191,6,66,29,7,144,165,65,105,9,35,128,128,255,0,137,2,133,182,164,169,33,128,128,197,171,130,155,68,235,7,32,21,70,77,19,
    66,21,10,68,97,8,66,30,5,66,4,43,34,0,17,0,71,19,41,65,253,20,71,25,23,65,91,15,65,115,7,34,2,128,128,66,9,8,130,169,33,1,0,66,212,13,132,28,72,201,43,35,0,0,0,18,66,27,38,76,231,5,
    68,157,20,135,157,32,7,68,185,13,65,129,28,66,20,5,32,253,66,210,11,65,128,49,133,61,32,0,65,135,6,74,111,37,72,149,12,66,203,19,65,147,19,68,93,7,68,85,8,76,4,5,33,255,0,133,129,34,
    254,0,128,68,69,8,181,197,34,0,0,12,65,135,32,65,123,20,69,183,27,133,156,66,50,5,72,87,10,67,137,32,33,0,19,160,139,78,251,13,68,55,20,67,119,19,65,91,36,69,177,15,32,254,143,16,65,
    98,53,32,128,130,0,32,0,66,43,54,70,141,23,66,23,15,131,39,69,47,11,131,15,70,129,19,74,161,9,36,128,255,0,128,254,130,153,65,148,32,67,41,9,34,0,0,4,79,15,5,73,99,10,71,203,8,32,3,
    72,123,6,72,43,8,32,2,133,56,131,99,130,9,34,0,0,6,72,175,5,73,159,14,144,63,135,197,132,189,133,66,33,255,0,73,6,7,70,137,12,35,0,0,0,10,130,3,73,243,25,67,113,12,65,73,7,69,161,7,
    138,7,37,21,2,0,128,128,254,134,3,73,116,27,33,128,128,130,111,39,12,0,128,1,0,3,128,2,72,219,21,35,43,0,47,0,67,47,20,130,111,33,21,1,68,167,13,81,147,8,133,230,32,128,77,73,6,32,
    128,131,142,134,18,130,6,32,255,75,18,12,131,243,37,128,0,128,3,128,3,74,231,21,135,123,32,29,134,107,135,7,32,21,74,117,7,135,7,134,96,135,246,74,103,23,132,242,33,0,10,67,151,28,
    67,133,20,66,141,11,131,11,32,3,77,71,6,32,128,130,113,32,1,81,4,6,134,218,66,130,24,131,31,34,0,26,0,130,0,77,255,44,83,15,11,148,155,68,13,7,32,49,78,231,18,79,7,11,73,243,11,32,
    33,65,187,10,130,63,65,87,8,73,239,19,35,0,128,1,0,131,226,32,252,65,100,6,32,128,139,8,33,1,0,130,21,32,253,72,155,44,73,255,20,32,128,71,67,8,81,243,39,67,15,20,74,191,23,68,121,
    27,32,1,66,150,6,32,254,79,19,11,131,214,32,128,130,215,37,2,0,128,253,0,128,136,5,65,220,24,147,212,130,210,33,0,24,72,219,42,84,255,13,67,119,16,69,245,19,72,225,19,65,3,15,69,93,
    19,131,55,132,178,71,115,14,81,228,6,142,245,33,253,0,132,43,172,252,65,16,11,75,219,8,65,219,31,66,223,24,75,223,10,33,29,1,80,243,10,66,175,8,131,110,134,203,133,172,130,16,70,30,
    7,164,183,130,163,32,20,65,171,48,65,163,36,65,143,23,65,151,19,65,147,13,65,134,17,133,17,130,216,67,114,5,164,217,65,137,12,72,147,48,79,71,19,74,169,22,80,251,8,65,173,7,66,157,
    15,74,173,15,32,254,65,170,8,71,186,45,72,131,6,77,143,40,187,195,152,179,65,123,38,68,215,57,68,179,15,65,85,7,69,187,14,32,21,66,95,15,67,19,25,32,1,83,223,6,32,2,76,240,7,77,166,
    43,65,8,5,130,206,32,0,67,39,54,143,167,66,255,19,82,193,11,151,47,85,171,5,67,27,17,132,160,69,172,11,69,184,56,66,95,6,33,12,1,130,237,32,2,68,179,27,68,175,16,80,135,15,72,55,7,
    71,87,12,73,3,12,132,12,66,75,32,76,215,5,169,139,147,135,148,139,81,12,12,81,185,36,75,251,7,65,23,27,76,215,9,87,165,12,65,209,15,72,157,7,65,245,31,32,128,71,128,6,32,1,82,125,5,
    34,0,128,254,131,169,32,254,131,187,71,180,9,132,27,32,2,88,129,44,32,0,78,47,40,65,79,23,79,171,14,32,21,71,87,8,72,15,14,65,224,33,130,139,74,27,62,93,23,7,68,31,7,75,27,7,139,15,
    74,3,7,74,23,27,65,165,11,65,177,15,67,123,5,32,1,130,221,32,252,71,96,5,74,12,12,133,244,130,25,34,1,0,128,130,2,139,8,93,26,8,65,9,32,65,57,14,140,14,32,0,73,79,67,68,119,11,135,
    11,32,51,90,75,14,139,247,65,43,7,131,19,139,11,69,159,11,65,247,6,36,1,128,128,253,0,90,71,9,33,1,0,132,14,32,128,89,93,14,69,133,6,130,44,131,30,131,6,65,20,56,33,0,16,72,179,40,
    75,47,12,65,215,19,74,95,19,65,43,11,131,168,67,110,5,75,23,17,69,106,6,75,65,5,71,204,43,32,0,80,75,47,71,203,15,159,181,68,91,11,67,197,7,73,101,13,68,85,6,33,128,128,130,214,130,
    25,32,254,74,236,48,130,194,37,0,18,0,128,255,128,77,215,40,65,139,64,32,51,80,159,10,65,147,39,130,219,84,212,43,130,46,75,19,97,74,33,11,65,201,23,65,173,31,33,1,0,79,133,6,66,150,
    5,67,75,48,85,187,6,70,207,37,32,71,87,221,13,73,163,14,80,167,15,132,15,83,193,19,82,209,8,78,99,9,72,190,11,77,110,49,89,63,5,80,91,35,99,63,32,70,235,23,81,99,10,69,148,10,65,110,
    36,32,0,65,99,47,95,219,11,68,171,51,66,87,7,72,57,7,74,45,17,143,17,65,114,50,33,14,0,65,111,40,159,195,98,135,15,35,7,53,51,21,100,78,9,95,146,16,32,254,82,114,6,32,128,67,208,37,
    130,166,99,79,58,32,17,96,99,14,72,31,19,72,87,31,82,155,7,67,47,14,32,21,131,75,134,231,72,51,17,72,78,8,133,8,80,133,6,33,253,128,88,37,9,66,124,36,72,65,12,134,12,71,55,43,66,139,
    27,85,135,10,91,33,12,65,35,11,66,131,11,71,32,8,90,127,6,130,244,71,76,11,168,207,33,0,12,66,123,32,32,0,65,183,15,68,135,11,66,111,7,67,235,11,66,111,15,32,254,97,66,12,160,154,67,
    227,52,80,33,15,87,249,15,93,45,31,75,111,12,93,45,11,77,99,9,160,184,81,31,12,32,15,98,135,30,104,175,7,77,249,36,69,73,15,78,5,12,32,254,66,151,19,34,128,128,4,87,32,12,149,35,133,
    21,96,151,31,32,19,72,35,5,98,173,15,143,15,32,21,143,99,158,129,33,0,0,65,35,52,65,11,15,147,15,98,75,11,33,1,0,143,151,132,15,32,254,99,200,37,132,43,130,4,39,0,10,0,128,1,128,3,
    0,104,151,14,97,187,20,69,131,15,67,195,11,87,227,7,33,128,128,132,128,33,254,0,68,131,9,65,46,26,42,0,0,0,7,0,0,255,128,3,128,0,88,223,15,33,0,21,89,61,22,66,209,12,65,2,12,37,0,2,
    1,0,3,128,101,83,8,36,0,1,53,51,29,130,3,34,21,1,0,66,53,8,32,0,68,215,6,100,55,25,107,111,9,66,193,11,72,167,8,73,143,31,139,31,33,1,0,131,158,32,254,132,5,33,253,128,65,16,9,133,
    17,89,130,25,141,212,33,0,0,93,39,8,90,131,25,93,39,14,66,217,6,106,179,8,159,181,71,125,15,139,47,138,141,87,11,14,76,23,14,65,231,26,140,209,66,122,8,81,179,5,101,195,26,32,47,74,
    75,13,69,159,11,83,235,11,67,21,16,136,167,131,106,130,165,130,15,32,128,101,90,24,134,142,32,0,65,103,51,108,23,11,101,231,15,75,173,23,74,237,23,66,15,6,66,46,17,66,58,17,65,105,
    49,66,247,55,71,179,12,70,139,15,86,229,7,84,167,15,32,1,95,72,12,89,49,6,33,128,128,65,136,38,66,30,9,32,0,100,239,7,66,247,29,70,105,20,65,141,19,69,81,15,130,144,32,128,83,41,5,
    32,255,131,177,68,185,5,133,126,65,97,37,32,0,130,0,33,21,0,130,55,66,195,28,67,155,13,34,79,0,83,66,213,13,73,241,19,66,59,19,65,125,11,135,201,66,249,16,32,128,66,44,11,66,56,17,
    68,143,8,68,124,38,67,183,12,96,211,9,65,143,29,112,171,5,32,0,68,131,63,34,33,53,51,71,121,11,32,254,98,251,16,32,253,74,231,10,65,175,37,133,206,37,0,0,8,1,0,0,107,123,11,113,115,
    9,33,0,1,130,117,131,3,73,103,7,66,51,18,66,44,5,133,75,70,88,5,32,254,65,39,12,68,80,9,34,12,0,128,107,179,28,68,223,6,155,111,86,147,15,32,2,131,82,141,110,33,254,0,130,15,32,4,103,
    184,15,141,35,87,176,5,83,11,5,71,235,23,114,107,11,65,189,16,70,33,15,86,153,31,135,126,86,145,30,65,183,41,32,0,130,0,32,10,65,183,24,34,35,0,39,67,85,9,65,179,15,143,15,33,1,0,65,
    28,17,157,136,130,123,32,20,130,3,32,0,97,135,24,115,167,19,80,71,12,32,51,110,163,14,78,35,19,131,19,155,23,77,229,8,78,9,17,151,17,67,231,46,94,135,8,73,31,31,93,215,56,82,171,25,
    72,77,8,162,179,169,167,99,131,11,69,85,19,66,215,15,76,129,13,68,115,22,72,79,35,67,113,5,34,0,0,19,70,31,46,65,89,52,73,223,15,85,199,33,95,33,8,132,203,73,29,32,67,48,16,177,215,
    101,13,15,65,141,43,69,141,15,75,89,5,70,0,11,70,235,21,178,215,36,10,0,128,0,0,71,207,24,33,0,19,100,67,6,80,215,11,66,67,7,80,43,12,71,106,7,80,192,5,65,63,5,66,217,26,33,0,13,156,
    119,68,95,5,72,233,12,134,129,85,81,11,76,165,20,65,43,8,73,136,8,75,10,31,38,128,128,0,0,0,13,1,130,4,32,3,106,235,29,114,179,12,66,131,23,32,7,77,133,6,67,89,12,131,139,116,60,9,
    89,15,37,32,0,74,15,7,103,11,22,65,35,5,33,55,0,93,81,28,67,239,23,78,85,5,107,93,14,66,84,17,65,193,26,74,183,10,66,67,34,143,135,79,91,15,32,7,117,111,8,75,56,9,84,212,9,154,134,
    32,0,130,0,32,18,130,3,70,171,41,83,7,16,70,131,19,84,191,15,84,175,19,84,167,30,84,158,12,154,193,68,107,15,33,0,0,65,79,42,65,71,7,73,55,7,118,191,16,83,180,9,32,255,76,166,9,154,
    141,32,0,130,0,69,195,52,65,225,15,151,15,75,215,31,80,56,10,68,240,17,100,32,9,70,147,39,65,93,12,71,71,41,92,85,15,84,135,23,78,35,15,110,27,10,84,125,8,107,115,29,136,160,38,0,0,
    14,0,128,255,0,82,155,24,67,239,8,119,255,11,69,131,11,77,29,6,112,31,8,134,27,105,203,8,32,2,75,51,11,75,195,12,74,13,29,136,161,37,128,0,0,0,11,1,130,163,82,115,8,125,191,17,69,35,
    12,74,137,15,143,15,32,1,65,157,12,136,12,161,142,65,43,40,65,199,6,65,19,24,102,185,11,76,123,11,99,6,12,135,12,32,254,130,8,161,155,101,23,9,39,8,0,0,1,128,3,128,2,78,63,17,72,245,
    12,67,41,11,90,167,9,32,128,97,49,9,32,128,109,51,14,132,97,81,191,8,130,97,125,99,12,121,35,9,127,75,15,71,79,12,81,151,23,87,97,7,70,223,15,80,245,16,105,97,15,32,254,113,17,6,32,
    128,130,8,105,105,8,76,122,18,65,243,21,74,63,7,38,4,1,0,255,0,2,0,119,247,28,133,65,32,255,141,91,35,0,0,0,16,67,63,36,34,59,0,63,77,59,9,119,147,11,143,241,66,173,15,66,31,11,67,
    75,8,81,74,16,32,128,131,255,87,181,42,127,43,5,34,255,128,2,120,235,11,37,19,0,23,0,0,37,109,191,14,118,219,7,127,43,14,65,79,14,35,0,0,0,3,73,91,5,130,5,38,3,0,7,0,11,0,0,70,205,
    11,88,221,12,32,0,73,135,7,87,15,22,73,135,10,79,153,15,97,71,19,65,49,11,32,1,131,104,121,235,11,80,65,11,142,179,144,14,81,123,46,32,1,88,217,5,112,5,8,65,201,15,83,29,15,122,147,
    11,135,179,142,175,143,185,67,247,39,66,199,7,35,5,0,128,3,69,203,15,123,163,12,67,127,7,130,119,71,153,10,141,102,70,175,8,32,128,121,235,30,136,89,100,191,11,116,195,11,111,235,15,
    72,39,7,32,2,97,43,5,132,5,94,67,8,131,8,125,253,10,32,3,65,158,16,146,16,130,170,40,0,21,0,128,0,0,3,128,5,88,219,15,24,64,159,32,135,141,65,167,15,68,163,10,97,73,49,32,255,82,58,
    7,93,80,8,97,81,16,24,67,87,52,34,0,0,5,130,231,33,128,2,80,51,13,65,129,8,113,61,6,132,175,65,219,5,130,136,77,152,17,32,0,95,131,61,70,215,6,33,21,51,90,53,10,78,97,23,105,77,31,
    65,117,7,139,75,24,68,195,9,24,64,22,9,33,0,128,130,11,33,128,128,66,25,5,121,38,5,134,5,134,45,66,40,36,66,59,18,34,128,0,0,66,59,81,135,245,123,103,19,120,159,19,77,175,12,33,255,
    0,87,29,10,94,70,21,66,59,54,39,3,1,128,3,0,2,128,4,24,65,7,15,66,47,7,72,98,12,37,0,0,0,3,1,0,24,65,55,21,131,195,32,1,67,178,6,33,4,0,77,141,8,32,6,131,47,74,67,16,24,69,3,20,24,
    65,251,7,133,234,130,229,94,108,17,35,0,0,6,0,141,175,86,59,5,162,79,85,166,8,70,112,13,32,13,24,64,67,26,24,71,255,7,123,211,12,80,121,11,69,215,15,66,217,11,69,71,10,131,113,132,
    126,119,90,9,66,117,19,132,19,32,0,130,0,24,64,47,59,33,7,0,73,227,5,68,243,15,85,13,12,76,37,22,74,254,15,130,138,33,0,4,65,111,6,137,79,65,107,16,32,1,77,200,6,34,128,128,3,75,154,
    12,37,0,16,0,0,2,0,104,115,36,140,157,68,67,19,68,51,15,106,243,15,134,120,70,37,10,68,27,10,140,152,65,121,24,32,128,94,155,7,67,11,8,24,74,11,25,65,3,12,83,89,18,82,21,37,67,200,
    5,130,144,24,64,172,12,33,4,0,134,162,74,80,14,145,184,32,0,130,0,69,251,20,32,19,81,243,5,82,143,8,33,5,53,89,203,5,133,112,79,109,15,33,0,21,130,71,80,175,41,36,75,0,79,0,83,121,
    117,9,87,89,27,66,103,11,70,13,15,75,191,11,135,67,87,97,20,109,203,5,69,246,8,108,171,5,78,195,38,65,51,13,107,203,11,77,3,17,24,75,239,17,65,229,28,79,129,39,130,175,32,128,123,253,
    7,132,142,24,65,51,15,65,239,41,36,128,128,0,0,13,65,171,5,66,163,28,136,183,118,137,11,80,255,15,67,65,7,74,111,8,32,0,130,157,32,253,24,76,35,10,103,212,5,81,175,9,69,141,7,66,150,
    29,131,158,24,75,199,28,124,185,7,76,205,15,68,124,14,32,3,123,139,16,130,16,33,128,128,108,199,6,33,0,3,65,191,35,107,11,6,73,197,11,24,70,121,15,83,247,15,24,70,173,23,69,205,14,
    32,253,131,140,32,254,136,4,94,198,9,32,3,78,4,13,66,127,13,143,13,32,0,130,0,33,16,0,24,69,59,39,109,147,12,76,253,19,24,69,207,15,69,229,15,130,195,71,90,10,139,10,130,152,73,43,
    40,91,139,10,65,131,37,35,75,0,79,0,84,227,12,143,151,68,25,15,80,9,23,95,169,11,34,128,2,128,112,186,5,130,6,83,161,19,76,50,6,130,37,65,145,44,110,83,5,32,16,67,99,6,71,67,15,76,
    55,17,140,215,67,97,23,76,69,15,77,237,11,104,211,23,77,238,11,65,154,43,33,0,10,83,15,28,83,13,20,67,145,19,67,141,14,97,149,21,68,9,15,86,251,5,66,207,5,66,27,37,82,1,23,127,71,12,
    94,235,10,110,175,24,98,243,15,132,154,132,4,24,66,69,10,32,4,67,156,43,130,198,35,2,1,0,4,75,27,9,69,85,9,95,240,7,32,128,130,35,32,28,66,43,40,24,82,63,23,83,123,12,72,231,15,127,
    59,23,116,23,19,117,71,7,24,77,99,15,67,111,15,71,101,8,36,2,128,128,252,128,127,60,11,32,1,132,16,130,18,141,24,67,107,9,32,3,68,194,15,175,15,38,0,11,0,128,1,128,2,80,63,25,32,0,
    24,65,73,11,69,185,15,83,243,16,32,0,24,81,165,8,130,86,77,35,6,155,163,88,203,5,24,66,195,30,70,19,19,24,80,133,15,32,1,75,211,8,32,254,108,133,8,79,87,20,65,32,9,41,0,0,7,0,128,0,
    0,2,128,2,68,87,15,66,1,16,92,201,16,24,76,24,17,133,17,34,128,0,30,66,127,64,34,115,0,119,73,205,9,66,43,11,109,143,15,24,79,203,11,90,143,15,131,15,155,31,65,185,15,86,87,11,35,128,
    128,253,0,69,7,6,130,213,33,1,0,119,178,15,142,17,66,141,74,83,28,6,36,7,0,0,4,128,82,39,18,76,149,12,67,69,21,32,128,79,118,15,32,0,130,0,32,8,131,206,32,2,79,83,9,100,223,14,102,
    113,23,115,115,7,24,65,231,12,130,162,32,4,68,182,19,130,102,93,143,8,69,107,29,24,77,255,12,143,197,72,51,7,76,195,15,132,139,85,49,15,130,152,131,18,71,81,23,70,14,11,36,0,10,0,128,
    2,69,59,9,89,151,15,66,241,11,76,165,12,71,43,15,75,49,13,65,12,23,132,37,32,0,179,115,130,231,95,181,16,132,77,32,254,67,224,8,65,126,20,79,171,8,32,2,89,81,5,75,143,6,80,41,8,34,
    2,0,128,24,81,72,9,32,0,130,0,35,17,0,0,255,77,99,39,95,65,36,67,109,15,24,69,93,11,77,239,5,95,77,23,35,128,1,0,128,24,86,7,8,132,167,32,2,69,198,41,130,202,33,0,26,120,75,44,24,89,
    51,15,71,243,12,70,239,11,24,84,3,11,66,7,11,71,255,10,32,21,69,155,35,88,151,12,32,128,74,38,10,65,210,8,74,251,5,65,226,5,75,201,13,32,3,65,9,41,146,41,40,0,0,0,9,1,0,1,0,2,91,99,
    19,32,35,106,119,13,70,219,15,83,239,12,137,154,32,2,67,252,19,36,128,0,0,4,1,130,196,32,2,130,8,91,107,8,32,0,135,81,24,73,211,8,132,161,73,164,13,36,0,8,0,128,2,105,123,26,139,67,
    76,99,15,34,1,0,128,135,76,83,156,20,92,104,8,67,251,30,24,86,47,27,123,207,12,24,86,7,15,71,227,8,32,4,65,20,20,131,127,32,0,130,123,32,0,71,223,26,32,19,90,195,22,71,223,15,84,200,
    6,32,128,133,241,24,84,149,9,67,41,25,36,0,0,0,22,0,88,111,49,32,87,66,21,5,77,3,27,123,75,7,71,143,19,135,183,71,183,19,130,171,74,252,5,131,5,89,87,17,32,1,132,18,130,232,68,11,10,
    33,1,128,70,208,16,66,230,18,147,18,130,254,223,255,75,27,23,65,59,15,135,39,155,255,34,128,128,254,104,92,8,33,0,128,65,32,11,65,1,58,33,26,0,130,0,72,71,18,78,55,17,76,11,19,86,101,
    12,75,223,11,89,15,11,24,76,87,15,75,235,15,131,15,72,95,7,85,71,11,72,115,11,73,64,6,34,1,128,128,66,215,9,34,128,254,128,134,14,33,128,255,67,102,5,32,0,130,16,70,38,11,66,26,57,
    88,11,8,24,76,215,34,78,139,7,95,245,7,32,7,24,73,75,23,32,128,131,167,130,170,101,158,9,82,49,22,118,139,6,32,18,67,155,44,116,187,9,108,55,14,80,155,23,66,131,15,93,77,10,131,168,
    32,128,73,211,12,24,75,187,22,32,4,96,71,20,67,108,19,132,19,120,207,8,32,5,76,79,15,66,111,21,66,95,8,32,3,190,211,111,3,8,211,212,32,20,65,167,44,34,75,0,79,97,59,13,32,33,112,63,
    10,65,147,19,69,39,19,143,39,24,66,71,9,130,224,65,185,43,94,176,12,65,183,24,71,38,8,24,72,167,7,65,191,38,136,235,24,96,167,12,65,203,62,115,131,13,65,208,42,175,235,67,127,6,32,
    4,76,171,29,114,187,5,32,71,65,211,5,65,203,68,72,51,8,164,219,32,0,172,214,71,239,58,78,3,27,66,143,15,77,19,15,147,31,35,33,53,51,21,66,183,10,173,245,66,170,30,150,30,34,0,0,23,
    80,123,54,76,1,16,73,125,15,82,245,11,167,253,24,76,85,12,70,184,5,32,254,131,185,37,254,0,128,1,0,128,133,16,117,158,18,92,27,38,65,3,17,130,251,35,17,0,128,254,24,69,83,39,140,243,
    121,73,19,109,167,7,81,41,15,24,95,175,12,102,227,15,121,96,11,24,95,189,7,32,3,145,171,154,17,24,77,47,9,33,0,5,70,71,37,68,135,7,32,29,117,171,11,69,87,15,24,79,97,19,24,79,149,23,
    131,59,32,1,75,235,5,72,115,11,72,143,7,132,188,71,27,46,131,51,32,0,69,95,6,175,215,32,21,131,167,81,15,19,151,191,151,23,131,215,71,43,5,32,254,24,79,164,24,74,109,8,77,166,13,65,
    176,26,88,162,5,98,159,6,171,219,120,247,6,79,29,8,99,169,10,103,59,19,65,209,35,131,35,91,25,19,112,94,15,83,36,8,173,229,33,20,0,88,75,43,71,31,12,65,191,71,33,1,0,130,203,32,254,
    131,4,68,66,7,67,130,6,104,61,13,173,215,38,13,1,0,0,0,2,128,67,111,28,74,129,16,104,35,19,79,161,16,87,14,7,138,143,132,10,67,62,36,114,115,5,162,151,67,33,16,108,181,15,143,151,67,
    5,5,24,100,242,15,170,153,34,0,0,14,65,51,34,32,55,79,75,9,32,51,74,7,10,65,57,38,132,142,32,254,72,0,14,139,163,32,128,80,254,8,67,158,21,65,63,7,32,4,72,227,27,95,155,12,67,119,19,
    124,91,24,149,154,72,177,34,97,223,8,155,151,24,108,227,15,88,147,16,72,117,19,68,35,11,92,253,15,70,199,15,24,87,209,17,32,2,87,233,7,32,1,24,88,195,10,119,24,8,32,3,81,227,24,65,
    125,21,35,128,128,0,25,76,59,48,24,90,187,9,97,235,12,66,61,11,91,105,19,24,79,141,11,24,79,117,15,24,79,129,27,90,53,13,130,13,32,253,131,228,24,79,133,40,69,70,8,66,137,31,65,33,
    19,96,107,8,68,119,29,66,7,5,68,125,16,65,253,19,65,241,27,24,90,179,13,24,79,143,18,33,128,128,130,246,32,254,130,168,68,154,36,77,51,9,97,47,5,167,195,32,21,131,183,78,239,27,155,
    195,78,231,14,201,196,77,11,6,32,5,73,111,37,97,247,12,77,19,31,155,207,78,215,19,162,212,69,17,14,66,91,19,80,143,57,78,203,39,159,215,32,128,93,134,8,24,80,109,24,66,113,15,169,215,
    66,115,6,32,4,69,63,33,32,0,101,113,7,86,227,35,143,211,36,49,53,51,21,1,77,185,14,65,159,28,69,251,34,67,56,8,33,9,0,24,107,175,25,90,111,12,110,251,11,119,189,24,119,187,34,87,15,
    9,32,4,66,231,37,90,39,7,66,239,8,84,219,15,69,105,23,24,85,27,27,87,31,11,33,1,128,76,94,6,32,1,85,241,7,33,128,128,106,48,10,33,128,128,69,136,11,133,13,24,79,116,49,84,236,8,24,
    91,87,9,32,5,165,255,69,115,12,66,27,15,159,15,24,72,247,12,74,178,5,24,80,64,15,33,0,128,143,17,77,89,51,130,214,24,81,43,7,170,215,74,49,8,159,199,143,31,139,215,69,143,5,32,254,
    24,81,50,35,181,217,84,123,70,143,195,159,15,65,187,16,66,123,7,65,175,15,65,193,29,68,207,39,79,27,5,70,131,6,32,4,68,211,33,33,67,0,83,143,14,159,207,143,31,140,223,33,0,128,24,80,
    82,14,24,93,16,23,32,253,65,195,5,68,227,40,133,214,107,31,7,32,5,67,115,27,87,9,8,107,31,43,66,125,6,32,0,103,177,23,131,127,72,203,36,32,0,110,103,8,155,163,73,135,6,32,19,24,112,
    99,10,65,71,11,73,143,19,143,31,126,195,5,24,85,21,9,24,76,47,14,32,254,24,93,77,36,68,207,11,39,25,0,0,255,128,3,128,4,66,51,37,95,247,13,82,255,24,76,39,19,147,221,66,85,27,24,118,
    7,8,24,74,249,12,76,74,8,91,234,8,67,80,17,131,222,33,253,0,121,30,44,73,0,16,69,15,6,32,0,65,23,38,69,231,12,65,179,6,98,131,16,86,31,27,24,108,157,14,80,160,8,24,65,46,17,33,4,0,
    96,2,18,144,191,65,226,8,68,19,5,171,199,80,9,15,180,199,67,89,5,32,255,24,79,173,28,174,201,24,79,179,50,32,1,24,122,5,10,82,61,10,180,209,83,19,8,32,128,24,80,129,27,111,248,43,131,
    71,24,115,103,8,67,127,41,78,213,24,100,247,19,66,115,39,75,107,5,32,254,165,219,78,170,40,24,112,163,49,32,1,97,203,6,65,173,64,32,0,83,54,7,133,217,88,37,12,32,254,131,28,33,128,
    3,67,71,44,84,183,6,32,5,69,223,33,96,7,7,123,137,16,192,211,24,112,14,9,32,255,67,88,29,68,14,10,84,197,38,33,0,22,116,47,50,32,87,106,99,9,116,49,15,89,225,15,97,231,23,70,41,19,
    82,85,8,93,167,6,32,253,132,236,108,190,7,89,251,5,116,49,58,33,128,128,131,234,32,15,24,74,67,38,70,227,24,24,83,45,23,89,219,12,70,187,12,89,216,19,32,2,69,185,24,141,24,70,143,66,
    24,82,119,56,78,24,10,32,253,133,149,132,6,24,106,233,7,69,198,48,178,203,81,243,12,68,211,15,106,255,23,66,91,15,69,193,7,100,39,10,24,83,72,16,176,204,33,19,0,88,207,45,68,21,12,
    68,17,10,65,157,53,68,17,6,32,254,92,67,10,65,161,25,69,182,43,24,118,91,47,69,183,18,181,209,111,253,12,89,159,8,66,112,12,69,184,45,35,0,0,0,9,24,80,227,26,73,185,16,118,195,15,131,
    15,33,1,0,65,59,15,66,39,27,160,111,66,205,12,148,111,143,110,33,128,128,156,112,24,81,199,8,75,199,23,66,117,20,155,121,32,254,68,126,12,72,213,29,134,239,149,123,89,27,16,148,117,
    65,245,8,24,71,159,14,141,134,134,28,73,51,55,109,77,15,105,131,11,68,67,11,76,169,27,107,209,12,102,174,8,32,128,72,100,18,116,163,56,79,203,11,75,183,44,85,119,19,71,119,23,151,227,
    32,1,93,27,8,65,122,5,77,102,8,110,120,20,66,23,8,66,175,17,66,63,12,133,12,79,35,8,74,235,33,67,149,16,69,243,15,78,57,15,69,235,16,67,177,7,151,192,130,23,67,84,29,141,192,174,187,
    77,67,15,69,11,12,159,187,77,59,10,199,189,24,70,235,50,96,83,19,66,53,23,105,65,19,77,47,12,163,199,66,67,37,78,207,50,67,23,23,174,205,67,228,6,71,107,13,67,22,14,66,85,11,83,187,
    38,124,47,49,95,7,19,66,83,23,67,23,19,24,96,78,17,80,101,16,71,98,40,33,0,7,88,131,22,24,89,245,12,84,45,12,102,213,5,123,12,9,32,2,126,21,14,43,255,0,128,128,0,0,20,0,128,255,128,
    3,126,19,39,32,75,106,51,7,113,129,15,24,110,135,19,126,47,15,115,117,11,69,47,11,32,2,109,76,9,102,109,9,32,128,75,2,10,130,21,32,254,69,47,6,32,3,94,217,47,32,0,65,247,10,69,15,46,
    65,235,31,65,243,15,101,139,10,66,174,14,65,247,16,72,102,28,69,17,14,84,243,9,165,191,88,47,48,66,53,12,32,128,71,108,6,203,193,32,17,75,187,42,73,65,16,65,133,52,114,123,9,167,199,
    69,21,37,86,127,44,75,171,11,180,197,78,213,12,148,200,81,97,46,24,95,243,9,32,4,66,75,33,113,103,9,87,243,36,143,225,24,84,27,31,90,145,8,148,216,67,49,5,24,84,34,14,75,155,27,67,
    52,13,140,13,36,0,20,0,128,255,24,135,99,46,88,59,43,155,249,80,165,7,136,144,71,161,23,32,253,132,33,32,254,88,87,44,136,84,35,128,0,0,21,81,103,5,94,47,44,76,51,12,143,197,151,15,
    65,215,31,24,64,77,13,65,220,20,65,214,14,71,4,40,65,213,13,32,0,130,0,35,21,1,2,0,135,0,34,36,0,72,134,10,36,1,0,26,0,130,134,11,36,2,0,14,0,108,134,11,32,3,138,23,32,4,138,11,34,
    5,0,20,134,33,34,0,0,6,132,23,32,1,134,15,32,18,130,25,133,11,37,1,0,13,0,49,0,133,11,36,2,0,7,0,38,134,11,36,3,0,17,0,45,134,11,32,4,138,35,36,5,0,10,0,62,134,23,32,6,132,23,36,3,
    0,1,4,9,130,87,131,167,133,11,133,167,133,11,133,167,133,11,37,3,0,34,0,122,0,133,11,133,167,133,11,133,167,133,11,133,167,34,50,0,48,130,1,34,52,0,47,134,5,8,49,49,0,53,98,121,32,
    84,114,105,115,116,97,110,32,71,114,105,109,109,101,114,82,101,103,117,108,97,114,84,84,88,32,80,114,111,103,103,121,67,108,101,97,110,84,84,50,48,48,52,47,130,2,53,49,53,0,98,0,121,
    0,32,0,84,0,114,0,105,0,115,0,116,0,97,0,110,130,15,32,71,132,15,36,109,0,109,0,101,130,9,32,82,130,5,36,103,0,117,0,108,130,29,32,114,130,43,34,84,0,88,130,35,32,80,130,25,34,111,
    0,103,130,1,34,121,0,67,130,27,32,101,132,59,32,84,130,31,33,0,0,65,155,9,34,20,0,0,65,11,6,130,8,135,2,33,1,1,130,9,8,120,1,1,2,1,3,1,4,1,5,1,6,1,7,1,8,1,9,1,10,1,11,1,12,1,13,1,14,
    1,15,1,16,1,17,1,18,1,19,1,20,1,21,1,22,1,23,1,24,1,25,1,26,1,27,1,28,1,29,1,30,1,31,1,32,0,3,0,4,0,5,0,6,0,7,0,8,0,9,0,10,0,11,0,12,0,13,0,14,0,15,0,16,0,17,0,18,0,19,0,20,0,21,0,
    22,0,23,0,24,0,25,0,26,0,27,0,28,0,29,0,30,0,31,130,187,8,66,33,0,34,0,35,0,36,0,37,0,38,0,39,0,40,0,41,0,42,0,43,0,44,0,45,0,46,0,47,0,48,0,49,0,50,0,51,0,52,0,53,0,54,0,55,0,56,0,
    57,0,58,0,59,0,60,0,61,0,62,0,63,0,64,0,65,0,66,130,243,9,75,68,0,69,0,70,0,71,0,72,0,73,0,74,0,75,0,76,0,77,0,78,0,79,0,80,0,81,0,82,0,83,0,84,0,85,0,86,0,87,0,88,0,89,0,90,0,91,0,
    92,0,93,0,94,0,95,0,96,0,97,1,33,1,34,1,35,1,36,1,37,1,38,1,39,1,40,1,41,1,42,1,43,1,44,1,45,1,46,1,47,1,48,1,49,1,50,1,51,1,52,1,53,1,54,1,55,1,56,1,57,1,58,1,59,1,60,1,61,1,62,1,
    63,1,64,1,65,0,172,0,163,0,132,0,133,0,189,0,150,0,232,0,134,0,142,0,139,0,157,0,169,0,164,0,239,0,138,0,218,0,131,0,147,0,242,0,243,0,141,0,151,0,136,0,195,0,222,0,241,0,158,0,170,
    0,245,0,244,0,246,0,162,0,173,0,201,0,199,0,174,0,98,0,99,0,144,0,100,0,203,0,101,0,200,0,202,0,207,0,204,0,205,0,206,0,233,0,102,0,211,0,208,0,209,0,175,0,103,0,240,0,145,0,214,0,
    212,0,213,0,104,0,235,0,237,0,137,0,106,0,105,0,107,0,109,0,108,0,110,0,160,0,111,0,113,0,112,0,114,0,115,0,117,0,116,0,118,0,119,0,234,0,120,0,122,0,121,0,123,0,125,0,124,0,184,0,
    161,0,127,0,126,0,128,0,129,0,236,0,238,0,186,14,117,110,105,99,111,100,101,35,48,120,48,48,48,49,141,14,32,50,141,14,32,51,141,14,32,52,141,14,32,53,141,14,32,54,141,14,32,55,141,
    14,32,56,141,14,32,57,141,14,32,97,141,14,32,98,141,14,32,99,141,14,32,100,141,14,32,101,141,14,32,102,140,14,33,49,48,141,14,141,239,32,49,141,239,32,49,141,239,32,49,141,239,32,49,
    141,239,32,49,141,239,32,49,141,239,32,49,141,239,32,49,141,239,32,49,141,239,32,49,141,239,32,49,141,239,32,49,141,239,32,49,141,239,45,49,102,6,100,101,108,101,116,101,4,69,117,114,
    111,140,236,32,56,141,236,32,56,141,236,32,56,141,236,32,56,141,236,32,56,141,236,32,56,141,236,32,56,141,236,32,56,141,236,32,56,141,236,32,56,141,236,32,56,141,236,32,56,141,236,
    32,56,141,236,32,56,141,236,32,56,65,220,13,32,57,65,220,13,32,57,141,239,32,57,141,239,32,57,141,239,32,57,141,239,32,57,141,239,32,57,141,239,32,57,141,239,32,57,141,239,32,57,141,
    239,32,57,141,239,32,57,141,239,32,57,141,239,32,57,141,239,32,57,141,239,35,57,102,0,0,5,250,72,249,98,247,
};

static const char* GetDefaultCompressedFontDataTTF(int* out_size)
{
    *out_size = proggy_clean_ttf_compressed_size;
    return (const char*)proggy_clean_ttf_compressed_data;
}
#endif // #ifndef IMGUI_DISABLE_DEFAULT_FONT

#endif // #ifndef IMGUI_DISABLE
