//-----------------------------------------------------------------------------
// USER IMPLEMENTATION
//-----------------------------------------------------------------------------

#pragma once

//---- Define your own malloc/free/realloc functions if you want to override internal memory allocations for ImGui
//#define IM_MALLOC(_SIZE)			MyMalloc(_SIZE)			// void* MyMalloc(size_t size);
//#define IM_FREE(_PTR)				MyFree(_PTR)			// void MyFree(void *ptr);
//#define IM_REALLOC(_PTR, _SIZE)	MyRealloc(_PTR, _SIZE)	// void* MyRealloc(void *ptr, size_t size);

//---- Define your own ImVector<> type if you don't want to use the provided implementation defined in imgui.h
//#include <vector>
//#define ImVector  std::vector
//#define ImVector  MyVector

//---- Define assertion handler. Defaults to calling assert().
//#define IM_ASSERT(_EXPR)  MyAssert(_EXPR)

//---- Don't implement default clipboard handlers for Windows (so as not to link with OpenClipboard(), etc.)
//#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCS

//---- Include imgui_user.cpp at the end of imgui.cpp so you can include code that extends ImGui using its private data/functions.
//#define IMGUI_INCLUDE_IMGUI_USER_CPP

//---- Define implicit cast operators to convert back<>forth from your math types and ImVec2/ImVec4.
/*
#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const MyVec2& f) { x = f.x; y = f.y; }                       \
        operator MyVec2() const { return MyVec2(x,y); }

#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const MyVec4& f) { x = f.x; y = f.y; z = f.z; w = f.w; }     \
        operator MyVec4() const { return MyVec4(x,y,z,w); }
*/

//---- Freely implement extra functions within the ImGui:: namespace.
//---- e.g. you can create variants of the ImGui::Value() helper for your low-level math types.
/*
namespace ImGui
{
    void    Value(const char* prefix, const MyVec2& v, const char* float_format = NULL);
    void    Value(const char* prefix, const MyVec4& v, const char* float_format = NULL);
};
*/

