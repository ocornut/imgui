// dear imgui: Renderer Backend for modern OpenGL with shaders / programmatic pipeline, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.opengl3;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <stddef.h>     // offsetof
#include <stdio.h>      // stderr

// GL ES auto-detection (same as imgui_impl_opengl3.h)
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3)
#if (defined(__APPLE__) && (TARGET_OS_IOS || TARGET_OS_TV)) || (defined(__ANDROID__))
#define IMGUI_IMPL_OPENGL_ES3
#elif defined(__EMSCRIPTEN__) || defined(__amigaos4__)
#define IMGUI_IMPL_OPENGL_ES2
#endif
#endif

// GL includes (same as imgui_impl_opengl3.cpp)
#if defined(IMGUI_IMPL_OPENGL_ES2)
#if (defined(__APPLE__) && (TARGET_OS_IOS || TARGET_OS_TV))
#include <OpenGLES/ES2/gl.h>
#else
#include <GLES2/gl2.h>
#endif
#if defined(__EMSCRIPTEN__)
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <GLES2/gl2ext.h>
#endif
#elif defined(IMGUI_IMPL_OPENGL_ES3)
#if (defined(__APPLE__) && (TARGET_OS_IOS || TARGET_OS_TV))
#include <OpenGLES/ES3/gl.h>
#else
#include <GLES3/gl3.h>
#endif
#elif !defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
#define IMGL3W_IMPL
#define IMGUI_IMPL_OPENGL_LOADER_IMGL3W
#include "imgui_impl_opengl3_loader.h"
#endif

export module imgui.impl.opengl3;

import std.compat;
import imgui;

#include "imgui_impl_opengl3.h"

module :private;

#include "imgui_impl_opengl3.cpp"
