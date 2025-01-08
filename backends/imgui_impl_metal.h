// dear imgui: Renderer Backend for Metal
// This needs to be used along with a Platform Backend (e.g. OSX)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'MTLTexture' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Renderer: Large meshes support (64k+ vertices) even with 16-bit indices (ImGuiBackendFlags_RendererHasVtxOffset).

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"      // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

//-----------------------------------------------------------------------------
// ObjC API
//-----------------------------------------------------------------------------

#ifdef __OBJC__

@class MTLRenderPassDescriptor;
@protocol MTLDevice, MTLCommandBuffer, MTLRenderCommandEncoder;

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool ImGui_ImplMetal_Init(id<MTLDevice> device);
IMGUI_IMPL_API void ImGui_ImplMetal_Shutdown();
IMGUI_IMPL_API void ImGui_ImplMetal_NewFrame(MTLRenderPassDescriptor* renderPassDescriptor);
IMGUI_IMPL_API void ImGui_ImplMetal_RenderDrawData(ImDrawData* drawData,
                                                   id<MTLCommandBuffer> commandBuffer,
                                                   id<MTLRenderCommandEncoder> commandEncoder);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool ImGui_ImplMetal_CreateFontsTexture(id<MTLDevice> device);
IMGUI_IMPL_API void ImGui_ImplMetal_DestroyFontsTexture();
IMGUI_IMPL_API bool ImGui_ImplMetal_CreateDeviceObjects(id<MTLDevice> device);
IMGUI_IMPL_API void ImGui_ImplMetal_DestroyDeviceObjects();

#endif

//-----------------------------------------------------------------------------
// C++ API
//-----------------------------------------------------------------------------

// Enable Metal C++ binding support with '#define IMGUI_IMPL_METAL_CPP' in your imconfig.h file
// More info about using Metal from C++: https://developer.apple.com/metal/cpp/

#ifdef IMGUI_IMPL_METAL_CPP
#include <Metal/Metal.hpp>
#ifndef __OBJC__

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool ImGui_ImplMetal_Init(MTL::Device* device);
IMGUI_IMPL_API void ImGui_ImplMetal_Shutdown();
IMGUI_IMPL_API void ImGui_ImplMetal_NewFrame(MTL::RenderPassDescriptor* renderPassDescriptor);
IMGUI_IMPL_API void ImGui_ImplMetal_RenderDrawData(ImDrawData* draw_data,
                                                   MTL::CommandBuffer* commandBuffer,
                                                   MTL::RenderCommandEncoder* commandEncoder);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool ImGui_ImplMetal_CreateFontsTexture(MTL::Device* device);
IMGUI_IMPL_API void ImGui_ImplMetal_DestroyFontsTexture();
IMGUI_IMPL_API bool ImGui_ImplMetal_CreateDeviceObjects(MTL::Device* device);
IMGUI_IMPL_API void ImGui_ImplMetal_DestroyDeviceObjects();

#endif
#endif

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
