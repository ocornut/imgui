
IMGUI_API bool  ImGui_ImplOpenGL3_Init();
IMGUI_API void  ImGui_ImplOpenGL3_Shutdown();
IMGUI_API void  ImGui_ImplOpenGL3_NewFrame();
IMGUI_API void  ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_API bool  ImGui_ImplOpenGL3_CreateFontsTexture();
IMGUI_API void  ImGui_ImplOpenGL3_DestroyFontsTexture();
IMGUI_API bool  ImGui_ImplOpenGL3_CreateDeviceObjects();
IMGUI_API void  ImGui_ImplOpenGL3_DestroyDeviceObjects();
