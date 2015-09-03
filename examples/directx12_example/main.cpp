/*
 * Copyright 2015 Onat Turkcuoglu. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <windows.h>

#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"

#include <D3D12.h>
#include <DXGI1_4.h>
#include <D3DCompiler.h>

#include "imgui_impl_dx12.h"

bool InitD3D12(ID3D12Device** _device
	, ID3D12CommandQueue** _commandQueue
	, ID3D12CommandAllocator** _commandAllocator
	, IDXGISwapChain3** _swapChain
	, ID3D12DescriptorHeap** _rtvHeap
	, ID3D12DescriptorHeap** _srvHeap
	, ID3D12RootSignature** _rootSignature
	, uint32_t _width
	, uint32_t _height
	, HWND _hwnd)
{
    // Load the DLLs and lookup function pointers
    HMODULE d3d12dll = LoadLibraryA("d3d12.dll");
    if (NULL == d3d12dll)
    {
	return false;
    }

    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12dll, "D3D12CreateDevice");
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12dll, "D3D12GetDebugInterface");
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(d3d12dll, "D3D12SerializeRootSignature");
    if (NULL == D3D12CreateDevice
	    ||  NULL == D3D12GetDebugInterface
	    ||  NULL == D3D12SerializeRootSignature)
    {
	return false;
    }

    ID3D12Debug* debug;
    HRESULT hr = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&debug);

    if (SUCCEEDED(hr) )
    {
	debug->EnableDebugLayer();
    }

    IDXGIFactory2* dxgiFactory;
    // Create DXGI factory to create the swap chain later
    hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, __uuidof(IDXGIFactory2), (void**)&dxgiFactory);
    if (FAILED(hr) )
    {
	return false;
    }

    // Create device
    ID3D12Device* device = 0;
    hr = D3D12CreateDevice(
	    NULL                        // NULL => Default adapter
	    , D3D_FEATURE_LEVEL_11_0    // Feature level
	    , __uuidof(ID3D12Device)
	    , (void**)&device
	    );

    *_device = device;

    // Create command queue
    ID3D12CommandQueue* commandQueue = 0;
    D3D12_COMMAND_QUEUE_DESC queueDesc;
    queueDesc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = 0;
    queueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0; // Single GPU operation
    device->CreateCommandQueue(&queueDesc
	    , __uuidof(ID3D12CommandQueue)
	    , (void**)_commandQueue
	    );

    // Create command allocator
    ID3D12CommandAllocator* commandAllocator = 0;
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT
	    , __uuidof(ID3D12CommandAllocator)
	    , (void**)_commandAllocator
	    );

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferDesc.Width = _width;
    swapChainDesc.BufferDesc.Height = _height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.OutputWindow = _hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.Flags = 0;

    hr = dxgiFactory->CreateSwapChain(*_commandQueue
	    , &swapChainDesc
	    , (IDXGISwapChain**)_swapChain
	    );

    // Create #FRAME_COUNT render target descriptors (stored in descriptor heap)
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescHeap;
    rtvDescHeap.NumDescriptors = FRAME_COUNT;
    rtvDescHeap.Type     = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescHeap.Flags    = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvDescHeap.NodeMask = 0;
    device->CreateDescriptorHeap(&rtvDescHeap
	    , __uuidof(ID3D12DescriptorHeap)
	    , (void**)_rtvHeap
	    );

    // Create one shader resource descriptor for the font texture
    D3D12_DESCRIPTOR_HEAP_DESC srvDescHeap;
    srvDescHeap.NumDescriptors = 1;
    srvDescHeap.Type     = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDescHeap.Flags    = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    srvDescHeap.NodeMask = 0;
    device->CreateDescriptorHeap(&srvDescHeap
	    , __uuidof(ID3D12DescriptorHeap)
	    , (void**)_srvHeap
	    );

    // A root parameter describes one slot of a root signature
    // Parameter types are: DESCRIPTOR_TABLE, 32BIT_CONSTANTS, CBV, SRV, UAV
    //
    // Root Descriptor Table: { uint NumDescriptorRanges, const DescriptorRange* pDescriptorRanges }
    // Root Descriptor:       { uint ShaderRegister, uint RegisterSpace }
    // Root Constants:        { uint ShaderRegister, uint RegisterSpace, uint Num32BitValues

    D3D12_DESCRIPTOR_RANGE descRange[] =
    {
	{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV,            // Range Type
	    1,                                          // Number of descriptors
	    0,                                          // Base shader register
	    0,                                          // Register space
	    D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },     // Offset in descriptors from the start of the root signature
    };

    // This root signature will have two parameters, one descriptor table for SRVs and one constant buffer descriptor
    D3D12_ROOT_PARAMETER rootParameter[] =
    {
	{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, {1,descRange}, D3D12_SHADER_VISIBILITY_ALL },
	{ D3D12_ROOT_PARAMETER_TYPE_CBV, {0,0}, D3D12_SHADER_VISIBILITY_VERTEX },
    };

    rootParameter[1].Descriptor.RegisterSpace = 0;
    rootParameter[1].Descriptor.ShaderRegister = 0;

    // Include a statci sampler
    D3D12_STATIC_SAMPLER_DESC samplerDesc;
    memset(&samplerDesc, 0, sizeof(samplerDesc));
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MipLODBias = 0.f;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplerDesc.MinLOD = 0.f;
    samplerDesc.MaxLOD = 0.f;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Root signature description
    D3D12_ROOT_SIGNATURE_DESC descRootSignature;
    descRootSignature.NumParameters = 2;
    descRootSignature.pParameters   = rootParameter;
    descRootSignature.NumStaticSamplers = 1;
    descRootSignature.pStaticSamplers   = &samplerDesc;
    descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Serialise the root signature into memory
    ID3DBlob* outBlob;
    ID3DBlob* errorBlob;
    D3D12SerializeRootSignature(&descRootSignature
	    , D3D_ROOT_SIGNATURE_VERSION_1
	    , &outBlob
	    , &errorBlob
	    );

    // Create the root signature using the binary blob
    device->CreateRootSignature(0
	    , outBlob->GetBufferPointer()
	    , outBlob->GetBufferSize()
	    , __uuidof(ID3D12RootSignature)
	    , (void**)_rootSignature
	    );

    outBlob->Release();
    return true;
}

LRESULT ImGui_ImplDX12_WndProcHandler(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (msg)
    {
	case WM_LBUTTONDOWN:
	    io.MouseDown[0] = true;
	    return true;
	case WM_LBUTTONUP:
	    io.MouseDown[0] = false;
	    return true;
	case WM_RBUTTONDOWN:
	    io.MouseDown[1] = true;
	    return true;
	case WM_RBUTTONUP:
	    io.MouseDown[1] = false;
	    return true;
	case WM_MOUSEWHEEL:
	    io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
	    return true;
	case WM_MOUSEMOVE:
	    io.MousePos.x = (signed short)(lParam);
	    io.MousePos.y = (signed short)(lParam >> 16);
	    return true;
	case WM_KEYDOWN:
	    if (wParam < 256)
		io.KeysDown[wParam] = 1;
	    return true;
	case WM_KEYUP:
	    if (wParam < 256)
		io.KeysDown[wParam] = 0;
	    return true;
	case WM_CHAR:
	    // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
	    if (wParam > 0 && wParam < 0x10000)
		io.AddInputCharacter((unsigned short)wParam);
	    return true;
    }
    return 0;
}

static bool s_resizeRenderTargets = false;
static UINT s_resizedWidth;  
static UINT s_resizedHeight;  

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplDX12_WndProcHandler(hWnd, msg, wParam, lParam))
	return true;

    switch (msg)
    {
	case WM_SIZE:
	    // Resize
	    s_resizedWidth  = (UINT)LOWORD(lParam);
	    s_resizedHeight = (UINT)HIWORD(lParam);
	    s_resizeRenderTargets = true;
	    return 0;
	case WM_SYSCOMMAND:
	    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
		return 0;
	    break;
	case WM_DESTROY:
	    PostQuitMessage(0);
	    return 0;
	case WM_LBUTTONDOWN:
	    return 1;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void setResourceBarrier(ID3D12GraphicsCommandList* _commandList, ID3D12Resource* _resource, D3D12_RESOURCE_STATES _stateBefore, D3D12_RESOURCE_STATES _stateAfter)
{
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = _resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = _stateBefore;
    barrier.Transition.StateAfter = _stateAfter;
    _commandList->ResourceBarrier(1, &barrier);
}

int main(int, char**)
{
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, L"ImGui Example", NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(L"ImGui Example", L"ImGui DirectX12 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // D3D12 interfaces
    ID3D12Device* device = 0;
    ID3D12CommandQueue* commandQueue = 0;
    ID3D12CommandAllocator* commandAllocator = 0;
    IDXGISwapChain3* swapChain = 0;
    ID3D12DescriptorHeap* rtvHeap = 0;
    ID3D12DescriptorHeap* srvHeap = 0;
    ID3D12GraphicsCommandList* commandList = 0;
    ID3D12RootSignature* rootSignature = 0;
    ID3D12Resource* renderTargets[FRAME_COUNT];

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    ImGui_ImplDX12_Init(hwnd);

    RECT rect;
    GetClientRect(hwnd, &rect);

    InitD3D12(&device
	    , &commandQueue
	    , &commandAllocator
	    , &swapChain
	    , &rtvHeap
	    , &srvHeap
	    , &rootSignature
	    , rect.right - rect.left
	    , rect.bottom - rect.top
	    , hwnd);

    // Initialize render targets
    // Create one RTV for each frame
    uint32_t rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (uint32_t n = 0; n < FRAME_COUNT; n++)
    {
	swapChain->GetBuffer(n
		, __uuidof(ID3D12Resource)
		, (void**)&renderTargets[n]);
	device->CreateRenderTargetView(renderTargets[n]
		, nullptr
		, rtvHandle);
	rtvHandle.ptr += rtvDescriptorSize;
    }

    // Create the command list for uploading the font texture and rendering
    device->CreateCommandList(0
	    , D3D12_COMMAND_LIST_TYPE_DIRECT
	    , commandAllocator
	    , NULL
	    , __uuidof(ID3D12GraphicsCommandList)
	    , (void**)&commandList
	    );

    // Data for sync
    ID3D12Fence* fence;
    uint32_t frameIndex = 0;
    uint32_t fenceValue = 1;

    // Create an event handle to use for frame synchronization.
    HANDLE fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    ImGui_ImplDX12_CreateDeviceObjects(device, commandList, commandQueue, fence, srvHandle, rootSignature);

    // The fence will be signalled when the command queue finishes uploading the fonts texture (see CreateDeviceObjects)
    // which will signal the fenceEvent and execution continues
    fence->SetEventOnCompletion(0, fenceEvent);
    WaitForSingleObject(fenceEvent, INFINITE);

    // Load Fonts
    // (see extra_fonts/README.txt for more details)
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    // Merge glyphs from multiple fonts into one (e.g. combine default font with another with Chinese glyphs, or add icons)
    //ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 };
    //ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/fontawesome-webfont.ttf", 18.0f, &icons_config, icons_ranges);

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_col = ImColor(114, 144, 154);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
	if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	    continue;
	}

	// Resizing logic
	if (s_resizeRenderTargets)
	{
	    for (uint32_t n = 0; n < FRAME_COUNT; n++)
	    {
		renderTargets[n]->Release(); // Have to release the resources before calling ResizeBuffers
	    }
            swapChain->ResizeBuffers(0, s_resizedWidth, s_resizedHeight, DXGI_FORMAT_UNKNOWN, 0);

	    rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	    for (uint32_t n = 0; n < FRAME_COUNT; n++)
	    {
		swapChain->GetBuffer(n
			, __uuidof(ID3D12Resource)
			, (void**)&renderTargets[n]);
		device->CreateRenderTargetView(renderTargets[n]
			, nullptr
			, rtvHandle);
		rtvHandle.ptr += rtvDescriptorSize;
	    }

	    s_resizeRenderTargets = false;
	    frameIndex = 0;
	}


	ImGui_ImplDX12_NewFrame();

	// 1. Show a simple window
	// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
	{
	    static float f = 0.0f;
	    ImGui::Text("Hello, world!");
	    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
	    ImGui::ColorEdit3("clear color", (float*)&clear_col);
	    if (ImGui::Button("Test Window")) show_test_window ^= 1;
	    if (ImGui::Button("Another Window")) show_another_window ^= 1;
	    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}

	// 2. Show another simple window, this time using an explicit Begin/End pair
	if (show_another_window)
	{
	    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
	    ImGui::Begin("Another Window", &show_another_window);
	    ImGui::Text("Hello");
	    ImGui::End();
	}

	// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if (show_test_window)
	{
	    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);     // Normally user code doesn't need/want to call it because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
	    ImGui::ShowTestWindow(&show_test_window);
	}

	// This is where rendering starts
	commandAllocator->Reset();
	commandList->Reset(commandAllocator, 0);
	ID3D12CommandList* ppCommandLists[] = { commandList };

	// Clear render target
	D3D12_CPU_DESCRIPTOR_HANDLE renderTarget = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	{
	    renderTarget.ptr += frameIndex * rtvDescriptorSize;

	    setResourceBarrier(commandList
		    , renderTargets[frameIndex]
		    , D3D12_RESOURCE_STATE_PRESENT
		    , D3D12_RESOURCE_STATE_RENDER_TARGET);

	    // Set necessary state.
	    const float clearColor[] = { 0.8f, 0.8f, 0.8f, 0.8f };
	    commandList->ClearRenderTargetView(renderTarget, clearColor, 0, nullptr);
	    commandList->Close();

	    commandQueue->ExecuteCommandLists(1, ppCommandLists);
	}

	// Setup imgui cmd list and render
	{
	    commandList->Reset(commandAllocator, 0);
	    commandList->SetGraphicsRootSignature(rootSignature);
	    commandList->SetDescriptorHeaps(1, &srvHeap);
	    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
	    commandList->SetGraphicsRootDescriptorTable(0, srvHandle);

	    // Pass command queue, command list and CPU descriptor handle to the render target
	    // to ImGui before rendering
	    ImGui_ImplDX12_SetRenderData(commandList, renderTarget);
	    ImGui::Render();

	    // Closing the command list and executing it is the apps responsibility
	    // ImGui only fills the command list
	}

	setResourceBarrier(commandList
		, renderTargets[frameIndex]
		, D3D12_RESOURCE_STATE_RENDER_TARGET
		, D3D12_RESOURCE_STATE_PRESENT);
	commandList->Close();

	commandQueue->ExecuteCommandLists(1, ppCommandLists);
	commandQueue->Signal(fence, fenceValue);

	// Wait until the resource transition to present is completed
	if (fence->GetCompletedValue() < fenceValue)
	{
	    fence->SetEventOnCompletion(fenceValue, fenceEvent);
	    WaitForSingleObject(fenceEvent, INFINITE);
	}

	// Present
	swapChain->Present(0, DXGI_PRESENT_RESTART);
	frameIndex = swapChain->GetCurrentBackBufferIndex();
	fenceValue++;
    }

    // Shutdown!!
    //
    UnregisterClass(L"ImGui Example", wc.hInstance);
    return 0;
}
