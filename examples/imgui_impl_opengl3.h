// dear imgui: Renderer for modern OpenGL with shaders / programmatic pipeline
// - Desktop GL: 2.x 3.x 4.x
// - Embedded GL: ES 2.0 (WebGL 1.0), ES 3.0 (WebGL 2.0)
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID!
//  [x] Renderer: Desktop GL only: Support for large meshes (64k+ vertices) with 16-bit indices.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  We will attempt to detect default GL loader based on available header files. See the imgui_impl_opengl3.cpp for details.
//  If auto-detection fails or doesn't select the same GL loader file as used by your application,
//  you are likely to get a crash in ImGui_ImplOpenGL3_Init().
//  You can explicitly select a loader by using '#define IMGUI_IMPL_OPENGL_LOADER_XXX' in imconfig.h or compiler command-line.
//    #define IMGUI_IMPL_OPENGL_LOADER_GL3W
//    #define IMGUI_IMPL_OPENGL_LOADER_GLEW
//    #define IMGUI_IMPL_OPENGL_LOADER_GLAD
//    #define IMGUI_IMPL_OPENGL_LOADER_GLBINDING2
//    #define IMGUI_IMPL_OPENGL_LOADER_GLBINDING3
//    #define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
//  Or you can select select specific Embedded OpenGL versions:
//    #define IMGUI_IMPL_OPENGL_ES2     // Auto-detected on Emscripten
//    #define IMGUI_IMPL_OPENGL_ES3     // Auto-detected on iOS/Android

// About GLSL version:
//  The 'glsl_version' initialization parameter should be NULL (default) or a "#version XXX" string.
//  On computer platform the GLSL version default to "#version 130". On OpenGL ES 3 platform it defaults to "#version 300 es"
//  Only override if your GL version doesn't handle this GLSL version. See GLSL version table at the top of imgui_impl_opengl3.cpp.

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

// Backend API
IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_Init(const char* glsl_version = NULL);
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_DestroyDeviceObjects();

// Attempt to auto-detect the default GL loader based on available header files.
// If auto-detection fails or doesn't select the same GL loader file as used by your application,
// you are likely to get a crash in ImGui_ImplOpenGL3_Init().
// You can explicitly select a loader by using '#define IMGUI_IMPL_OPENGL_LOADER_XXX'
// or '#define IMGUI_IMPL_OPENGL_ESX' in imconfig.h or compiler command-line.
#if !defined(IMGUI_IMPL_OPENGL_LOADER_GL3W) \
 && !defined(IMGUI_IMPL_OPENGL_LOADER_GLEW) \
 && !defined(IMGUI_IMPL_OPENGL_LOADER_GLAD) \
 && !defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2) \
 && !defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3) \
 && !defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM) \
 && !defined(IMGUI_IMPL_OPENGL_ES2) \
 && !defined(IMGUI_IMPL_OPENGL_ES3)
    #if defined(__APPLE__)
    #include "TargetConditionals.h"
    #endif

    // Auto-enable GLES on matching platforms
    #if (defined(__APPLE__) && (TARGET_OS_IOS || TARGET_OS_TV)) || (defined(__ANDROID__))
    #define IMGUI_IMPL_OPENGL_ES3           // iOS, Android  -> GL ES 3, "#version 300 es"
    #elif defined(__EMSCRIPTEN__)
    #define IMGUI_IMPL_OPENGL_ES2           // Emscripten    -> GL ES 2, "#version 100"
    #elif defined(__has_include)
        // Desktop OpenGL
        #if __has_include(<GL/glew.h>)
            #define IMGUI_IMPL_OPENGL_LOADER_GLEW
        #elif __has_include(<glad/glad.h>)
            #define IMGUI_IMPL_OPENGL_LOADER_GLAD
        #elif __has_include(<GL/gl3w.h>)
            #define IMGUI_IMPL_OPENGL_LOADER_GL3W
        #elif __has_include(<glbinding/glbinding.h>)
            #define IMGUI_IMPL_OPENGL_LOADER_GLBINDING3
        #elif __has_include(<glbinding/Binding.h>)
            #define IMGUI_IMPL_OPENGL_LOADER_GLBINDING2
        #else
            #error "Cannot detect OpenGL loader!"
        #endif
    #else
        #define IMGUI_IMPL_OPENGL_LOADER_GL3W       // Default to GL3W
    #endif
#endif
