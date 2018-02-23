// ImGui Win32 + DirectX12 binding
// FIXME: 64-bit only for now! (Because sizeof(ImTextureId) == sizeof(void*))

// Implemented features:
//  [X] User texture binding. Use 'D3D12_GPU_DESCRIPTOR_HANDLE' as ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

enum DXGI_FORMAT;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

// cmd_list is the command list that the implementation will use to render imgui draw lists.
// Before calling the render function, caller must prepare cmd_list by resetting it and setting the appropriate 
// render target and descriptor heap that contains font_srv_cpu_desc_handle/font_srv_gpu_desc_handle.
// font_srv_cpu_desc_handle and font_srv_gpu_desc_handle are handles to a single SRV descriptor to use for the internal font texture.
IMGUI_API bool        ImGui_ImplDX12_Init(void* hwnd, int num_frames_in_flight,
                                          ID3D12Device* device,
                                          DXGI_FORMAT rtv_format,
                                          D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle,
                                          D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle);
IMGUI_API void        ImGui_ImplDX12_Shutdown();
IMGUI_API void        ImGui_ImplDX12_NewFrame(ID3D12GraphicsCommandList* cmd_list);
IMGUI_API void        ImGui_ImplDX12_RenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplDX12_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplDX12_CreateDeviceObjects();

// Handler for Win32 messages, update mouse/keyboard data.
// You may or not need this for your implementation, but it can serve as reference for handling inputs.
// Commented out to avoid dragging dependencies on <windows.h> types. You can copy the extern declaration in your code.
/*
IMGUI_API LRESULT   ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
*/
