// dear imgui: compile/link test for the C++20 backend modules for Windows:
// 'imgui.impl.win32', 'imgui.impl.dx9', 'imgui.impl.dx10', 'imgui.impl.dx11', 'imgui.impl.dx12', 'imgui.impl.opengl2', 'imgui.impl.opengl3'.
// Checks that their API is exported, usable from an importer, and links (not run).

#include <windows.h>            // application-side platform headers
#include <d3d9.h>
#include <d3d10_1.h>
#include <d3d10.h>
#include <d3d11.h>
#include <d3d12.h>
import std;
import imgui;
import imgui.impl.win32;
import imgui.impl.dx9;
import imgui.impl.dx10;
import imgui.impl.dx11;
import imgui.impl.dx12;
import imgui.impl.opengl2;
import imgui.impl.opengl3;

static_assert(sizeof(ImGui_ImplDX12_InitInfo) > 0);
static_assert(std::is_same_v<decltype(&ImGui_ImplWin32_WndProcHandler), LRESULT (*)(HWND, UINT, WPARAM, LPARAM)>);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void RenderFrameDX9(HWND hwnd, IDirect3DDevice9* device)
{
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(device);
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void RenderFrameDX10(ID3D10Device* device)
{
    ImGui_ImplDX10_Init(device);
    ImGui_ImplDX10_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
}

void RenderFrameDX11(ID3D11Device* device, ID3D11DeviceContext* device_context)
{
    ImGui_ImplDX11_Init(device, device_context);
    ImGui_ImplDX11_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void RenderFrameDX12(ID3D12Device* device, ID3D12CommandQueue* command_queue, ID3D12DescriptorHeap* srv_heap, ID3D12GraphicsCommandList* command_list)
{
    ImGui_ImplDX12_InitInfo init_info;
    init_info.Device = device;
    init_info.CommandQueue = command_queue;
    init_info.NumFramesInFlight = 2;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    init_info.SrvDescriptorHeap = srv_heap;
    ImGui_ImplDX12_Init(&init_info);
    ImGui_ImplDX12_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_list);
}

void RenderFrameOpenGL(HWND hwnd)
{
    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL2_Init();
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

int main()
{
    ImGui_ImplWin32_EnableDpiAwareness();
    return 0;
}
