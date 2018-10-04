// dear imgui: Renderer for Metal
// This needs to be used along with a Platform Binding (e.g. OSX)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'MTLTexture' as ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

@class MTLRenderPassDescriptor;
@protocol MTLDevice, MTLCommandBuffer, MTLRenderCommandEncoder;

IMGUI_IMPL_API bool ImGui_ImplMetal_Init(id<MTLDevice> device);
IMGUI_IMPL_API void ImGui_ImplMetal_Shutdown();
IMGUI_IMPL_API void ImGui_ImplMetal_NewFrame(MTLRenderPassDescriptor *renderPassDescriptor);
IMGUI_IMPL_API void ImGui_ImplMetal_RenderDrawData(ImDrawData* draw_data,
                                                   id<MTLCommandBuffer> commandBuffer,
                                                   id<MTLRenderCommandEncoder> commandEncoder);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool ImGui_ImplMetal_CreateFontsTexture(id<MTLDevice> device);
IMGUI_IMPL_API void ImGui_ImplMetal_DestroyFontsTexture();
IMGUI_IMPL_API bool ImGui_ImplMetal_CreateDeviceObjects(id<MTLDevice> device);
IMGUI_IMPL_API void ImGui_ImplMetal_DestroyDeviceObjects();
