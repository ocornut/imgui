// ImGui SDL2 binding with OpenGL3
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

struct SDL_Window;
typedef union SDL_Event SDL_Event;

IMGUI_API bool        ImGui_ImplSdlGL3_Init(SDL_Window* window);
IMGUI_API void        ImGui_ImplSdlGL3_Shutdown();
IMGUI_API void        ImGui_ImplSdlGL3_NewFrame(SDL_Window* window);
IMGUI_API void        ImGui_ImplSdlGL3_RenderDrawData(ImDrawData* draw_data);
IMGUI_API bool        ImGui_ImplSdlGL3_ProcessEvent(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplSdlGL3_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplSdlGL3_CreateDeviceObjects();
