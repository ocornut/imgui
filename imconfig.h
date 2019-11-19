//-----------------------------------------------------------------------------
// COMPILE-TIME OPTIONS FOR DEAR IMGUI
// Runtime options (clipboard callbacks, enabling various features, etc.) can generally be set via the ImGuiIO structure.
// You can use ImGui::SetAllocatorFunctions() before calling ImGui::CreateContext() to rewire memory allocation functions.
//-----------------------------------------------------------------------------
// A) You may edit imconfig.h (and not overwrite it when updating Dear ImGui, or maintain a patch/branch with your modifications to imconfig.h)
// B) or add configuration directives in your own file and compile with #define IMGUI_USER_CONFIG "myfilename.h"
// If you do so you need to make sure that configuration settings are defined consistently _everywhere_ Dear ImGui is used, which include
// the imgui*.cpp files but also _any_ of your code that uses Dear ImGui. This is because some compile-time options have an affect on data structures.
// Defining those options in imconfig.h will ensure every compilation unit gets to see the same data structure layouts.
// Call IMGUI_CHECKVERSION() from your .cpp files to verify that the data structures your files are using are matching the ones imgui.cpp is using.
//-----------------------------------------------------------------------------

#pragma once

//---- Define assertion handler. Defaults to calling assert().
//#define IM_ASSERT(_EXPR)  MyAssert(_EXPR)
//#define IM_ASSERT(_EXPR)  ((void)(_EXPR))     // Disable asserts

//---- Define attributes of all API symbols declarations, e.g. for DLL under Windows
// Using dear imgui via a shared library is not recommended, because of function call overhead and because we don't guarantee backward nor forward ABI compatibility.
//#define IMGUI_API __declspec( dllexport )
//#define IMGUI_API __declspec( dllimport )

//---- Don't define obsolete functions/enums names. Consider enabling from time to time after updating to avoid using soon-to-be obsolete function/names.
//#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//---- Don't implement demo windows functionality (ShowDemoWindow()/ShowStyleEditor()/ShowUserGuide() methods will be empty)
// It is very strongly recommended to NOT disable the demo windows during development. Please read the comments in imgui_demo.cpp.
//#define IMGUI_DISABLE_DEMO_WINDOWS
//#define IMGUI_DISABLE_METRICS_WINDOW

//---- Don't implement some functions to reduce linkage requirements.
//#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS   // [Win32] Don't implement default clipboard handler. Won't use and link with OpenClipboard/GetClipboardData/CloseClipboard etc.
//#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS         // [Win32] Don't implement default IME handler. Won't use and link with ImmGetContext/ImmSetCompositionWindow.
//#define IMGUI_DISABLE_WIN32_FUNCTIONS                     // [Win32] Won't use and link with any Win32 function (clipboard, ime).
//#define IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS      // [OSX] Implement default OSX clipboard handler (need to link with '-framework ApplicationServices', this is why this is not the default).
//#define IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS            // Don't implement ImFormatString/ImFormatStringV so you can implement them yourself (e.g. if you don't want to link with vsnprintf)
//#define IMGUI_DISABLE_DEFAULT_MATH_FUNCTIONS              // Don't implement ImFabs/ImSqrt/ImPow/ImFmod/ImCos/ImSin/ImAcos/ImAtan2 so you can implement them yourself.
//#define IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS              // Don't implement ImFileOpen/ImFileClose/ImFileRead/ImFileWrite so you can implement them yourself if you don't want to link with fopen/fclose/fread/fwrite. This will also disable the LogToTTY() function.
//#define IMGUI_DISABLE_DEFAULT_ALLOCATORS                  // Don't implement default allocators calling malloc()/free() to avoid linking with them. You will need to call ImGui::SetAllocatorFunctions().

//---- Include imgui_user.h at the end of imgui.h as a convenience
//#define IMGUI_INCLUDE_IMGUI_USER_H

//---- Pack colors to BGRA8 instead of RGBA8 (to avoid converting from one to another)
//#define IMGUI_USE_BGRA_PACKED_COLOR

//---- Avoid multiple STB libraries implementations, or redefine path/filenames to prioritize another version
// By default the embedded implementations are declared static and not available outside of imgui cpp files.
//#define IMGUI_STB_TRUETYPE_FILENAME   "my_folder/stb_truetype.h"
//#define IMGUI_STB_RECT_PACK_FILENAME  "my_folder/stb_rect_pack.h"
//#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION

//---- Define constructor and implicit cast operators to convert back<>forth between your math types and ImVec2/ImVec4.
// This will be inlined as part of ImVec2 and ImVec4 class declarations.
/*
#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const MyVec2& f) { x = f.x; y = f.y; }                       \
        operator MyVec2() const { return MyVec2(x,y); }

#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const MyVec4& f) { x = f.x; y = f.y; z = f.z; w = f.w; }     \
        operator MyVec4() const { return MyVec4(x,y,z,w); }
*/

//---- Using 32-bits vertex indices (default is 16-bits) is one way to allow large meshes with more than 64K vertices.
// Your renderer back-end will need to support it (most example renderer back-ends support both 16/32-bits indices).
// Another way to allow large meshes while keeping 16-bits indices is to handle ImDrawCmd::VtxOffset in your renderer.
// Read about ImGuiBackendFlags_RendererHasVtxOffset for details.
//#define ImDrawIdx unsigned int

//---- Override ImDrawCallback signature (will need to modify renderer back-ends accordingly)
//struct ImDrawList;
//struct ImDrawCmd;
//typedef void (*MyImDrawCallback)(const ImDrawList* draw_list, const ImDrawCmd* cmd, void* my_renderer_user_data);
//#define ImDrawCallback MyImDrawCallback

//---- Debug Tools
// Use 'Metrics->Tools->Item Picker' to pick widgets with the mouse and break into them for easy debugging.
//#define IM_DEBUG_BREAK  IM_ASSERT(0)
//#define IM_DEBUG_BREAK  __debugbreak()
// Have the Item Picker break in the ItemAdd() function instead of ItemHoverable() - which is earlier in the code, will catch a few extra items, allow picking items other than Hovered one.
// This adds a small runtime cost which is why it is not enabled by default.
//#define IMGUI_DEBUG_TOOL_ITEM_PICKER_EX

//---- Tip: You can add extra functions within the ImGui:: namespace, here or in your own headers files.
/*
namespace ImGui
{
    void MyFunction(const char* name, const MyMatrix44& v);
}
*/
