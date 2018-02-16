
IMGUI_API bool  ImGui_ImplOpenGL2_Init();
IMGUI_API void  ImGui_ImplOpenGL2_Shutdown();
IMGUI_API void  ImGui_ImplOpenGL2_NewFrame();
IMGUI_API void  ImGui_ImplOpenGL2_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_API bool  ImGui_ImplOpenGL2_CreateFontsTexture();
IMGUI_API void  ImGui_ImplOpenGL2_DestroyFontsTexture();
IMGUI_API bool  ImGui_ImplOpenGL2_CreateDeviceObjects();
IMGUI_API void  ImGui_ImplOpenGL2_DestroyDeviceObjects();
