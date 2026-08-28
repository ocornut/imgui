// dear imgui: single-file wrapper include
// We use this to validate compiling all *.cpp files in a same compilation unit.
// Users of that technique (also called "Unity builds") can generally provide this themselves easily.

// Do this:
//    #define IMGUI_IMPLEMENTATION
//    #include "imgui_single_file.h"
// In *one* C++ file to create the implementation.
// Using this in your project will leak the contents of imgui_internal.h and ImVec2 operators in this compilation unit.

#if defined(IMGUI_IMPLEMENTATION) && !defined(IMGUI_DEFINE_MATH_OPERATORS)
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "../../imgui.h"
#ifdef IMGUI_ENABLE_FREETYPE
#include "../../misc/freetype/imgui_freetype.h"
#endif

#ifdef IMGUI_IMPLEMENTATION
#include "../../imgui.cpp"
#include "../../imgui_demo.cpp"
#include "../../imgui_draw.cpp"
#include "../../imgui_tables.cpp"
#include "../../imgui_widgets.cpp"
#ifdef IMGUI_ENABLE_FREETYPE
#include "../../misc/freetype/imgui_freetype.cpp"
#endif
#endif
