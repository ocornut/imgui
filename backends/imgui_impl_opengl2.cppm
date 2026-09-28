// dear imgui: Renderer Backend for OpenGL2 (legacy OpenGL, fixed pipeline), as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.opengl2;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <stddef.h>     // offsetof

// OpenGL (same as imgui_impl_opengl2.cpp)
#if defined(_WIN32) && !defined(APIENTRY)
#define APIENTRY __stdcall
#endif
#if defined(_WIN32) && !defined(WINGDIAPI)
#define WINGDIAPI __declspec(dllimport)
#endif
#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

export module imgui.impl.opengl2;

import std.compat;
import imgui;

#include "imgui_impl_opengl2.h"

module :private;

#include "imgui_impl_opengl2.cpp"
