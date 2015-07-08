// ImGui SDL2 binding with OpenGL
// https://github.com/ocornut/imgui

struct SDL_Window;
typedef union SDL_Event SDL_Event;

bool        ImGui_ImplSdl_Init(SDL_Window *window);
void        ImGui_ImplSdl_Shutdown();
void        ImGui_ImplSdl_NewFrame(SDL_Window *window);
bool        ImGui_ImplSdl_ProcessEvent(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
void        ImGui_ImplSdl_InvalidateDeviceObjects();
bool        ImGui_ImplSdl_CreateDeviceObjects();
