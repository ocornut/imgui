// ImGui Renderer for: px_render 
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)
// (Note: We are using GL3W as a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc..)

// Implemented features:
//  [X] Renderer: User texture binding. Use Pointer to Texture Object ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

namespace px_render {
  struct RenderContext;
  struct DisplayList;
}

IMGUI_IMPL_API void     ImGui_Impl_pxrender_Init(px_render::RenderContext *ctx);
IMGUI_IMPL_API void     ImGui_Impl_pxrender_Shutdown();
IMGUI_IMPL_API void     ImGui_Impl_pxrender_RenderDrawData(ImDrawData* draw_data, px_render::DisplayList *dl_output);
