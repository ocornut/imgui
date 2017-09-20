// ImGui Platform Binding for: Gtk+ 3.x
// This needs to be used along with the OpenGL3 Renderer

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [X] Platform: Keyboard arrays indexed using GDK_KEY_* codes, e.g. ImGui::IsKeyPressed(GDK_KEY_space).

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

typedef struct _GtkWidget GtkWidget;
typedef union  _GdkEvent  GdkEvent;

IMGUI_API bool          ImGui_ImplGtk3_Init(GtkWidget* gl_area, bool install_callbacks);
IMGUI_API void          ImGui_ImplGtk3_HandleEvent(GdkEvent* event);

IMGUI_API void          ImGui_ImplGtk3_Shutdown();
IMGUI_API void          ImGui_ImplGtk3_NewFrame();
