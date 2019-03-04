// dear imgui: Renderer for modern OpenGL using GLSL shaders / programmable pipeline:
//      Desktop GL:  3.x, 4.x
//      Embedded GL: ES 2.x (WebGL 1.0), ES 3.x (WebGL 2.0)
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)
// (Note: We are using GL3W as a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily.
// Alternatives are GLEW, Glad, etc..)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// About OpenGL function loaders: modern OpenGL doesn't have a standard header file and requires individual function pointers to be loaded manually.
// Helper libraries are often used for this purpose! Here we are supporting a few common ones: gl3w, glew, glad.
// You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2019-02-15: OpenGL: Initial version, based on OpenGL3 implementation

#pragma once

// GL version setup
//
// You should use one of the following #define at compilation time:
// IMGUI_IMPL_OPENGL_3      => GL 3.x       Supported on: Windows, Mac, Linux
// IMGUI_IMPL_OPENGL_4      => GL 4.x       Supported on: Windows, Mac, Linux
// IMGUI_IMPL_OPENGL_ES_2   => GLES 2.0     Supported on: Windows, Mac, Linux, Android, iOS, Emscripten
// IMGUI_IMPL_OPENGL_ES_3   => GLES 3.x     Supported on: Windows, Mac, Linux, Android, iOS, Emscripten
//
// If not specified at compile time, defaults are defined depending on target platform:
// Windows, Mac, Linux => IMGUI_IMPL_OPENGL_3
// Android, iOS        => IMGUI_IMPL_OPENGL_ES_3
// Emscripten          => IMGUI_IMPL_OPENGL_ES_2

// Validity checks and defaults
//
// iOS, Android and Emscripten
//  - Can only use ES2 or ES3 (no GL3 and GL4)
//  - Defaults to ES3 on iOS and Android, and ES2 on Emscripten (equivalent to WebGL1, for wider browser support)
#if (defined(__APPLE__) && TARGET_OS_IOS) || (defined(__ANDROID__)) || (defined(__EMSCRIPTEN__))
 #if defined(IMGUI_IMPL_OPENGL_3) \
  || defined(IMGUI_IMPL_OPENGL_4)
  #error "Desktop OpenGL can't be used on iOS, Android, or Emscripten"
 #elif defined(IMGUI_IMPL_OPENGL_ES_2) \
    || defined(IMGUI_IMPL_OPENGL_ES_3)
    // OK
 #else
  // Defaults
  #if (defined(__APPLE__) && TARGET_OS_IOS) || (defined(__ANDROID__))
   #define IMGUI_IMPL_OPENGL_ES_3
  #else
   #define IMGUI_IMPL_OPENGL_ES_2
  #endif
 #endif
// Windows, Mac, Linux
//  - Can use all versions: GL3, GL4, ES2, ES3
//  - Defaults to GL3
#else
 #if defined(IMGUI_IMPL_OPENGL_3) \
  || defined(IMGUI_IMPL_OPENGL_4) \
  || defined(IMGUI_IMPL_OPENGL_ES_2) \
  || defined(IMGUI_IMPL_OPENGL_ES_3)
  // OK
 #else
  // Defaults
  #define IMGUI_IMPL_OPENGL_3
 #endif
#endif

// Utility defines:
// - IMGUI_IMPL_OPENGL_DESKTOP if either GL3 or GL4
// - IMGUI_IMPL_OPENGL_EMBEDDED if either ES2 or ES3
#if defined(IMGUI_IMPL_OPENGL_3) \
 || defined(IMGUI_IMPL_OPENGL_4)
 #define IMGUI_IMPL_OPENGL_DESKTOP
#elif defined(IMGUI_IMPL_OPENGL_ES_2) \
   || defined(IMGUI_IMPL_OPENGL_ES_3)
 #define IMGUI_IMPL_OPENGL_EMBEDDED
#endif

// Loaders setup for Desktop GL:
// - GL3W, GLEW, GLAD or CUSTOM supported
// - Defaults to GL3W
#if defined(IMGUI_IMPL_OPENGL_DESKTOP)
 #if !defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)     \
  && !defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)     \
  && !defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)     \
  && !defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
   #define IMGUI_IMPL_OPENGL_LOADER_GL3W
 #endif
#endif

IMGUI_IMPL_API bool ImGui_ImplGL_Init(const char* glsl_version = NULL);
IMGUI_IMPL_API void ImGui_ImplGL_Shutdown();
IMGUI_IMPL_API void ImGui_ImplGL_NewFrame();
IMGUI_IMPL_API void ImGui_ImplGL_RenderDrawData(ImDrawData *draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool ImGui_ImplGL_CreateFontsTexture();
IMGUI_IMPL_API void ImGui_ImplGL_DestroyFontsTexture();
IMGUI_IMPL_API bool ImGui_ImplGL_CreateDeviceObjects();
IMGUI_IMPL_API void ImGui_ImplGL_DestroyDeviceObjects();
