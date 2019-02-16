// dear imgui: Renderer for GLES2 (or WebGL if run with Emscripten toolchain)
// This needs to be used along with a Platform Binding (e.g. SDL, emscripten..)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#pragma once

IMGUI_IMPL_API bool     ImGui_ImplGLES2_Init();
IMGUI_IMPL_API void     ImGui_ImplGLES2_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplGLES2_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplGLES2_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplGLES2_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplGLES2_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplGLES2_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplGLES2_DestroyDeviceObjects();
