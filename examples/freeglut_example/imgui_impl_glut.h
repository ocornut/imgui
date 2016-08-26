// ImGui GLFW binding with freeglut

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

IMGUI_API bool        ImGui_ImplGLUT_Init();
IMGUI_API void        ImGui_ImplGLUT_NewFrame(int w, int h);
IMGUI_API void        ImGui_ImplGLUT_Shutdown();