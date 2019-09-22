// dear imgui: Renderer for modern OpenGL with shaders / programmatic pipeline
// - Desktop GL: 3.x 4.x
// - Embedded GL: ES 2.0 (WebGL 1.0), ES 3.0 (WebGL 2.0)
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.
//  [x] Renderer: Desktop GL only: Support for large meshes (64k+ vertices) with 16-bits indices.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.

// About GLSL version:
//  The 'glsl_version' initialization parameter should be NULL (default) or a "#version XXX" string.
//  On computer platform the GLSL version default to "#version 130". On OpenGL ES 3 platform it defaults to "#version 300 es"
//  Only override if your GL version doesn't handle this GLSL version. See GLSL version table at the top of imgui_impl_opengl3.cpp.

#pragma once

// Specific OpenGL versions
//#define IMGUI_IMPL_OPENGL_ES2     // Auto-detected on Emscripten
//#define IMGUI_IMPL_OPENGL_ES3     // Auto-detected on iOS/Android

// Set default OpenGL3 loader to be gl3w
#if !defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)     \
 && !defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)     \
 && !defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)     \
 && !defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
    // to avoid problem with non-clang compilers not having this macro.
    #if defined(__has_include)
        // check if the header exists, then automatically define the macros ..
        // check if glew gl loader exists, use it
        #if __has_include(<GL/glew.h>)
            // make sure we don't have another gl loader headers ....
            // that means that the user might have used one and initialized it
            // while we use the other ....
            // so we make sure there is only one gl loader header
            // in this case, we need to only have glew headers and not glad !
            #if __has_include(<glad/glad.h>)
                #error you have multiple headers in your system (GLEW + GLAD), remove one of them or use '#define IMGUI_IMPL_OPENGL_LOADER_GLEW'
            #else
                #define IMGUI_IMPL_OPENGL_LOADER_GLEW
            #endif
        #elif __has_include(<glad/glad.h>)
            // make sure we don't have another gl loader headers ....
            // that means that the user might have used one and initialized it
            // while we use the other ....
            // so we make sure there is only one gl loader header
            // in this case, we need to only have glad headers and not glew !
            #if __has_include(<GL/glew.h>)
                 #error you have multiple headers in your system (GLEW + GLAD), remove one of them or use '#define IMGUI_IMPL_OPENGL_LOADER_GLAD'
            #else
                #define IMGUI_IMPL_OPENGL_LOADER_GLAD
            #endif
        #else
            #define IMGUI_IMPL_OPENGL_LOADER_GL3W
        #endif
    #else
        #define IMGUI_IMPL_OPENGL_LOADER_GL3W
    #endif
#endif

IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_Init(const char* glsl_version = NULL);
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_DestroyDeviceObjects();
