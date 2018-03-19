// ImGui Platform Binding for: SDL2
// This needs to be used along with a Renderer (e.g. DirectX11, OpenGL3, Vulkan..)
// (Info: SDL2 is a cross-platform general purpose library for handling windows, inputs, graphics context creation, etc.)

// Implemented features:
//  [X] Multi-viewport windows (when ImGuiConfigFlags_EnableViewports is enabled).

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

struct SDL_Window;
typedef union SDL_Event SDL_Event;

IMGUI_API bool      ImGui_ImplSDL2_Init(SDL_Window* window, void* sdl_gl_context);
IMGUI_API void      ImGui_ImplSDL2_Shutdown();
IMGUI_API void      ImGui_ImplSDL2_NewFrame(SDL_Window* window);
IMGUI_API bool      ImGui_ImplSDL2_ProcessEvent(SDL_Event* event);
