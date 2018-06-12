// ImGui Renderer for: OpenGL3 (modern OpenGL with shaders / programmatic pipeline)
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)
// (Note: We are using GL3W as a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc..)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// About GLSL version:
// The 'glsl_version' initialization parameter defaults to "#version 150" if NULL.
// Only override if your GL version doesn't handle this GLSL version. Keep NULL if unsure!

IMGUI_API bool  ImGui_ImplOpenGL3_Init(const char* glsl_version = "#version 150");
IMGUI_API void  ImGui_ImplOpenGL3_Shutdown();
IMGUI_API void  ImGui_ImplOpenGL3_NewFrame();
IMGUI_API void  ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_API bool  ImGui_ImplOpenGL3_CreateFontsTexture();
IMGUI_API void  ImGui_ImplOpenGL3_DestroyFontsTexture();
IMGUI_API bool  ImGui_ImplOpenGL3_CreateDeviceObjects();
IMGUI_API void  ImGui_ImplOpenGL3_DestroyDeviceObjects();
