struct SDL_Window;
typedef union SDL_Event SDL_Event;

bool        ImGui_ImplSdl_Init(SDL_Window *window);
void        ImGui_ImplSdl_Shutdown();
void        ImGui_ImplSdl_NewFrame(SDL_Window *window);
bool        ImGui_ImplSdl_EventCallback(const SDL_Event& event);

void        ImGui_ImplSdl_InvalidateDeviceObjects();
bool        ImGui_ImplSdl_CreateDeviceObjects();

