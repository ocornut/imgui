struct		SDL_Window;

bool        ImGui_ImplSdl_Init(SDL_Window *window);
void        ImGui_ImplSdl_Shutdown();
bool        ImGui_ImplSdl_NewFrame(SDL_Window *window);

void        ImGui_ImplSdl_InvalidateDeviceObjects();
bool        ImGui_ImplSdl_CreateDeviceObjects();

void		ImGui_ImplSdl_KeyCallback(int key, int, bool down);
void        ImGui_ImplSdl_CharCallback(unsigned int c);
