// ImGui SDL2 binding with OpenGL ES 2
// You can copy and use unmodified imgui_impl_* files in your project. 
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// See main.cpp for an example of using this.
// https://github.com/ocornut/imgui

#ifndef IMGUI_IMPL_SDL_GL2
#define IMGUI_IMPL_SDL_GL2

#include "../../../../../../../imgui.h"

struct SDL_Window;
typedef union SDL_Event SDL_Event;

IMGUI_API bool        ImGui_ImplSdlGLES2_Init(SDL_Window *window);
IMGUI_API void        ImGui_ImplSdlGLES2_Shutdown();
IMGUI_API void        ImGui_ImplSdlGLES2_NewFrame();
IMGUI_API bool        ImGui_ImplSdlGLES2_ProcessEvent(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplSdlGLES2_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplSdlGLES2_CreateDeviceObjects();

#endif // IMGUI_IMPL_SDL_GL2