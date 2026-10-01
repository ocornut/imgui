// dear imgui: Platform Backend for GLUT/FreeGLUT, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.glut;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"

// GLUT (same as imgui_impl_glut.cpp)
#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

export module imgui.impl.glut;

import std.compat;
import imgui;

#include "imgui_impl_glut.h"

module :private;

#include "imgui_impl_glut.cpp"
