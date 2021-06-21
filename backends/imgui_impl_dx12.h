// dear imgui: Renderer Backend for DirectX12
// This needs to be used along with a Platform Backend (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'D3D12_GPU_DESCRIPTOR_HANDLE' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Renderer: Support for large meshes (64k+ vertices) with 16-bit indices.

// Important: to compile on 32-bit systems, this backend requires code to be compiled with '#define ImTextureID ImU64'.
// This is because we need ImTextureID to carry a 64-bit value and by default ImTextureID is defined as void*.
// This define is set in the example .vcxproj file and need to be replicated in your app or by adding it to your imconfig.h file.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this. 
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4471) // a forward declaration of an unscoped enumeration must have an underlying type
#endif

enum DXGI_FORMAT;
struct ID3D12Device;
struct ID3D12DescriptorHeap;
struct ID3D12GraphicsCommandList;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

// cmd_list is the command list that the implementation will use to render imgui draw lists.
// Before calling the render function, caller must prepare cmd_list by resetting it and setting the appropriate
// render target and descriptor heap that contains font_srv_cpu_desc_handle/font_srv_gpu_desc_handle.
// font_srv_cpu_desc_handle and font_srv_gpu_desc_handle are handles to a single SRV descriptor to use for the internal font texture.
IMGUI_IMPL_API bool     ImGui_ImplDX12_Init(ID3D12Device* device, int num_frames_in_flight, DXGI_FORMAT rtv_format, ID3D12DescriptorHeap* cbv_srv_heap,
                                            D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle);
IMGUI_IMPL_API void     ImGui_ImplDX12_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplDX12_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplDX12_RenderDrawData(ImDrawData* draw_data, ID3D12GraphicsCommandList* graphics_command_list);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API void     ImGui_ImplDX12_InvalidateDeviceObjects();
IMGUI_IMPL_API bool     ImGui_ImplDX12_CreateDeviceObjects();

#ifdef _MSC_VER
#pragma warning (pop)
#endif

