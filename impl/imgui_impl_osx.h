// dear imgui: Platform Binding for OSX / Cocoa
// This needs to be used along with a Renderer (e.g. OpenGL2, OpenGL3, Vulkan, Metal..)
// [BETA] Beta bindings, not well tested. If you want a portable application, prefer using the Glfw or SDL platform bindings on Mac.

// Issues:
// [ ] Platform: Keys are all generally very broken. Best using [event keycode] and not [event characters]..
// [ ] Platform: Mouse cursor shapes and visibility are not supported (see end of https://github.com/glfw/glfw/issues/427)

@class NSEvent;
@class NSView;

IMGUI_API bool        ImGui_ImplOSX_Init();
IMGUI_API void        ImGui_ImplOSX_Shutdown();
IMGUI_API void        ImGui_ImplOSX_NewFrame(NSView *_Nonnull view);
IMGUI_API bool        ImGui_ImplOSX_HandleEvent(NSEvent *_Nonnull event, NSView *_Nullable view);
