// dear imgui: Renderer Backend for DirectX12
// This needs to be used along with a Platform Backend (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'D3D12_GPU_DESCRIPTOR_HANDLE' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Renderer: Large meshes support (64k+ vertices) with 16-bit indices.
//  [X] Renderer: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.
//      FIXME: The transition from removing a viewport and moving the window in an existing hosted viewport tends to flicker.

// Important: to compile on 32-bit systems, this backend requires code to be compiled with '#define ImTextureID ImU64'.
// This is because we need ImTextureID to carry a 64-bit value and by default ImTextureID is defined as void*.
// To build this on 32-bit systems:
// - [Solution 1] IDE/msbuild: in "Properties/C++/Preprocessor Definitions" add 'ImTextureID=ImU64' (this is what we do in the 'example_win32_direct12/example_win32_direct12.vcxproj' project file)
// - [Solution 2] IDE/msbuild: in "Properties/C++/Preprocessor Definitions" add 'IMGUI_USER_CONFIG="my_imgui_config.h"' and inside 'my_imgui_config.h' add '#define ImTextureID ImU64' and as many other options as you like.
// - [Solution 3] IDE/msbuild: edit imconfig.h and add '#define ImTextureID ImU64' (prefer solution 2 to create your own config file!)
// - [Solution 4] command-line: add '/D ImTextureID=ImU64' to your cl.exe command-line (this is what we do in the example_win32_direct12/build_win32.bat file)

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2024-XX-XX: Platform: Added support for multiple windows via the ImGuiPlatformIO interface.
//  2022-10-11: Using 'nullptr' instead of 'NULL' as per our switch to C++11.
//  2021-06-29: Reorganized backend to pull data from a single structure to facilitate usage with multiple-contexts (all g_XXXX access changed to bd->XXXX).
//  2021-05-19: DirectX12: Replaced direct access to ImDrawCmd::TextureId with a call to ImDrawCmd::GetTexID(). (will become a requirement)
//  2021-02-18: DirectX12: Change blending equation to preserve alpha in output buffer.
//  2021-01-11: DirectX12: Improve Windows 7 compatibility (for D3D12On7) by loading d3d12.dll dynamically.
//  2020-09-16: DirectX12: Avoid rendering calls with zero-sized scissor rectangle since it generates a validation layer warning.
//  2020-09-08: DirectX12: Clarified support for building on 32-bit systems by redefining ImTextureID.
//  2019-10-18: DirectX12: *BREAKING CHANGE* Added extra ID3D12DescriptorHeap parameter to ImGui_ImplDX12_Init() function.
//  2019-05-29: DirectX12: Added support for large mesh (64K+ vertices), enable ImGuiBackendFlags_RendererHasVtxOffset flag.
//  2019-04-30: DirectX12: Added support for special ImDrawCallback_ResetRenderState callback to reset render state.
//  2019-03-29: Misc: Various minor tidying up.
//  2018-12-03: Misc: Added #pragma comment statement to automatically link with d3dcompiler.lib when using D3DCompile().
//  2018-11-30: Misc: Setting up io.BackendRendererName so it can be displayed in the About Window.
//  2018-06-12: DirectX12: Moved the ID3D12GraphicsCommandList* parameter from NewFrame() to RenderDrawData().
//  2018-06-08: Misc: Extracted imgui_impl_dx12.cpp/.h away from the old combined DX12+Win32 example.
//  2018-06-08: DirectX12: Use draw_data->DisplayPos and draw_data->DisplaySize to setup projection matrix and clipping rectangle (to ease support for future multi-viewport).
//  2018-02-22: Merged into master with all Win32 code synchronized to other examples.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_dx12.h"

// DirectX
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
#endif

// DirectX data
struct ImGui_ImplDX12_Data
{
    ID3D12Device*               pd3dDevice;
    ID3D12RootSignature*        pRootSignature;
    ID3D12PipelineState*        pPipelineState;
    DXGI_FORMAT                 RTVFormat;
    ID3D12Resource*             pFontTextureResource;
    D3D12_CPU_DESCRIPTOR_HANDLE hFontSrvCpuDescHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE hFontSrvGpuDescHandle;
    ID3D12DescriptorHeap*       pd3dSrvDescHeap;
    UINT                        numFramesInFlight;

    ImGui_ImplDX12_Data()       { memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplDX12_Data* ImGui_ImplDX12_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplDX12_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

// Buffers used during the rendering of a frame
struct ImGui_ImplDX12_RenderBuffers
{
    ID3D12Resource*     IndexBuffer;
    ID3D12Resource*     VertexBuffer;
    int                 IndexBufferSize;
    int                 VertexBufferSize;
};

// Buffers used for secondary viewports created by the multi-viewports systems
struct ImGui_ImplDX12_FrameContext
{
    ID3D12CommandAllocator*         CommandAllocator;
    ID3D12Resource*                 RenderTarget;
    D3D12_CPU_DESCRIPTOR_HANDLE     RenderTargetCpuDescriptors;
};

// Helper structure we store in the void* RendererUserData field of each ImGuiViewport to easily retrieve our backend data.
// Main viewport created by application will only use the Resources field.
// Secondary viewports created by this backend will use all the fields (including Window fields),
struct ImGui_ImplDX12_ViewportData
{
    // Window
    ID3D12CommandQueue*             CommandQueue;
    ID3D12GraphicsCommandList*      CommandList;
    ID3D12DescriptorHeap*           RtvDescHeap;
    IDXGISwapChain3*                SwapChain;
    ID3D12Fence*                    Fence;
    UINT64                          FenceSignaledValue;
    HANDLE                          FenceEvent;
    UINT                            NumFramesInFlight;
    ImGui_ImplDX12_FrameContext*    FrameCtx;

    // Render buffers
    UINT                            FrameIndex;
    ImGui_ImplDX12_RenderBuffers*   FrameRenderBuffers;

    ImGui_ImplDX12_ViewportData(UINT num_frames_in_flight)
    {
        CommandQueue = nullptr;
        CommandList = nullptr;
        RtvDescHeap = nullptr;
        SwapChain = nullptr;
        Fence = nullptr;
        FenceSignaledValue = 0;
        FenceEvent = nullptr;
        NumFramesInFlight = num_frames_in_flight;
        FrameCtx = new ImGui_ImplDX12_FrameContext[NumFramesInFlight];
        FrameIndex = UINT_MAX;
        FrameRenderBuffers = new ImGui_ImplDX12_RenderBuffers[NumFramesInFlight];

        for (UINT i = 0; i < NumFramesInFlight; ++i)
        {
            FrameCtx[i].CommandAllocator = nullptr;
            FrameCtx[i].RenderTarget = nullptr;

            // Create buffers with a default size (they will later be grown as needed)
            FrameRenderBuffers[i].IndexBuffer = nullptr;
            FrameRenderBuffers[i].VertexBuffer = nullptr;
            FrameRenderBuffers[i].VertexBufferSize = 5000;
            FrameRenderBuffers[i].IndexBufferSize = 10000;
        }
    }
    ~ImGui_ImplDX12_ViewportData()
    {
        IM_ASSERT(CommandQueue == nullptr && CommandList == nullptr);
        IM_ASSERT(RtvDescHeap == nullptr);
        IM_ASSERT(SwapChain == nullptr);
        IM_ASSERT(Fence == nullptr);
        IM_ASSERT(FenceEvent == nullptr);

        for (UINT i = 0; i < NumFramesInFlight; ++i)
        {
            IM_ASSERT(FrameCtx[i].CommandAllocator == nullptr && FrameCtx[i].RenderTarget == nullptr);
            IM_ASSERT(FrameRenderBuffers[i].IndexBuffer == nullptr && FrameRenderBuffers[i].VertexBuffer == nullptr);
        }

        delete[] FrameCtx; FrameCtx = nullptr;
        delete[] FrameRenderBuffers; FrameRenderBuffers = nullptr;
    }
};

struct VERTEX_CONSTANT_BUFFER_DX12
{
    float   mvp[4][4];
};

// Forward Declarations
static void ImGui_ImplDX12_InitPlatformInterface();
static void ImGui_ImplDX12_ShutdownPlatformInterface();

// Functions
static void ImGui_ImplDX12_SetupRenderState(ImDrawData* draw_data, ID3D12GraphicsCommandList* ctx, ImGui_ImplDX12_RenderBuffers* fr)
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();

    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
    VERTEX_CONSTANT_BUFFER_DX12 vertex_constant_buffer;
    {
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
        };
        memcpy(&vertex_constant_buffer.mvp, mvp, sizeof(mvp));
    }

    // Setup viewport
    D3D12_VIEWPORT vp;
    memset(&vp, 0, sizeof(D3D12_VIEWPORT));
    vp.Width = draw_data->DisplaySize.x;
    vp.Height = draw_data->DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0.0f;
    ctx->RSSetViewports(1, &vp);

    // Bind shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    D3D12_VERTEX_BUFFER_VIEW vbv;
    memset(&vbv, 0, sizeof(D3D12_VERTEX_BUFFER_VIEW));
    vbv.BufferLocation = fr->VertexBuffer->GetGPUVirtualAddress() + offset;
    vbv.SizeInBytes = fr->VertexBufferSize * stride;
    vbv.StrideInBytes = stride;
    ctx->IASetVertexBuffers(0, 1, &vbv);
    D3D12_INDEX_BUFFER_VIEW ibv;
    memset(&ibv, 0, sizeof(D3D12_INDEX_BUFFER_VIEW));
    ibv.BufferLocation = fr->IndexBuffer->GetGPUVirtualAddress();
    ibv.SizeInBytes = fr->IndexBufferSize * sizeof(ImDrawIdx);
    ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    ctx->IASetIndexBuffer(&ibv);
    ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->SetPipelineState(bd->pPipelineState);
    ctx->SetGraphicsRootSignature(bd->pRootSignature);
    ctx->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);

    // Setup blend factor
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    ctx->OMSetBlendFactor(blend_factor);
}

template<typename T>
static inline void SafeRelease(T*& res)
{
    if (res)
        res->Release();
    res = nullptr;
}

// Render function
void ImGui_ImplDX12_RenderDrawData(ImDrawData* draw_data, ID3D12GraphicsCommandList* ctx)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    ImGui_ImplDX12_ViewportData* vd = (ImGui_ImplDX12_ViewportData*)draw_data->OwnerViewport->RendererUserData;
    vd->FrameIndex++;
    ImGui_ImplDX12_RenderBuffers* fr = &vd->FrameRenderBuffers[vd->FrameIndex % bd->numFramesInFlight];

    // Create and grow vertex/index buffers if needed
    if (fr->VertexBuffer == nullptr || fr->VertexBufferSize < draw_data->TotalVtxCount)
    {
        SafeRelease(fr->VertexBuffer);
        fr->VertexBufferSize = draw_data->TotalVtxCount + 5000;
        D3D12_HEAP_PROPERTIES props;
        memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        D3D12_RESOURCE_DESC desc;
        memset(&desc, 0, sizeof(D3D12_RESOURCE_DESC));
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = fr->VertexBufferSize * sizeof(ImDrawVert);
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (bd->pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&fr->VertexBuffer)) < 0)
            return;
    }
    if (fr->IndexBuffer == nullptr || fr->IndexBufferSize < draw_data->TotalIdxCount)
    {
        SafeRelease(fr->IndexBuffer);
        fr->IndexBufferSize = draw_data->TotalIdxCount + 10000;
        D3D12_HEAP_PROPERTIES props;
        memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        D3D12_RESOURCE_DESC desc;
        memset(&desc, 0, sizeof(D3D12_RESOURCE_DESC));
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = fr->IndexBufferSize * sizeof(ImDrawIdx);
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (bd->pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&fr->IndexBuffer)) < 0)
            return;
    }

    // Upload vertex/index data into a single contiguous GPU buffer
    void* vtx_resource, *idx_resource;
    D3D12_RANGE range;
    memset(&range, 0, sizeof(D3D12_RANGE));
    if (fr->VertexBuffer->Map(0, &range, &vtx_resource) != S_OK)
        return;
    if (fr->IndexBuffer->Map(0, &range, &idx_resource) != S_OK)
        return;
    ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource;
    ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    fr->VertexBuffer->Unmap(0, &range);
    fr->IndexBuffer->Unmap(0, &range);

    // Setup desired DX state
    ImGui_ImplDX12_SetupRenderState(draw_data, ctx, fr);

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplDX12_SetupRenderState(draw_data, ctx, fr);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
                ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply Scissor/clipping rectangle, Bind texture, Draw
                const D3D12_RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
                D3D12_GPU_DESCRIPTOR_HANDLE texture_handle = {};
                texture_handle.ptr = (UINT64)pcmd->GetTexID();
                ctx->SetGraphicsRootDescriptorTable(1, texture_handle);
                ctx->RSSetScissorRects(1, &r);
                ctx->DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }
}

static void ImGui_ImplDX12_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    {
        D3D12_HEAP_PROPERTIES props;
        memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
        props.Type = D3D12_HEAP_TYPE_DEFAULT;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment = 0;
        desc.Width = width;
        desc.Height = height;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ID3D12Resource* pTexture = nullptr;
        bd->pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pTexture));

        UINT uploadPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
        UINT uploadSize = height * uploadPitch;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = uploadSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        ID3D12Resource* uploadBuffer = nullptr;
        HRESULT hr = bd->pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
        IM_ASSERT(SUCCEEDED(hr));

        void* mapped = nullptr;
        D3D12_RANGE range = { 0, uploadSize };
        hr = uploadBuffer->Map(0, &range, &mapped);
        IM_ASSERT(SUCCEEDED(hr));
        for (int y = 0; y < height; y++)
            memcpy((void*) ((uintptr_t) mapped + y * uploadPitch), pixels + y * width * 4, width * 4);
        uploadBuffer->Unmap(0, &range);

        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.pResource = uploadBuffer;
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srcLocation.PlacedFootprint.Footprint.Width = width;
        srcLocation.PlacedFootprint.Footprint.Height = height;
        srcLocation.PlacedFootprint.Footprint.Depth = 1;
        srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.pResource = pTexture;
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = 0;

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = pTexture;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        ID3D12Fence* fence = nullptr;
        hr = bd->pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        IM_ASSERT(SUCCEEDED(hr));

        HANDLE event = CreateEvent(0, 0, 0, 0);
        IM_ASSERT(event != nullptr);

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 1;

        ID3D12CommandQueue* cmdQueue = nullptr;
        hr = bd->pd3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));
        IM_ASSERT(SUCCEEDED(hr));

        ID3D12CommandAllocator* cmdAlloc = nullptr;
        hr = bd->pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
        IM_ASSERT(SUCCEEDED(hr));

        ID3D12GraphicsCommandList* cmdList = nullptr;
        hr = bd->pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc, nullptr, IID_PPV_ARGS(&cmdList));
        IM_ASSERT(SUCCEEDED(hr));

        cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
        cmdList->ResourceBarrier(1, &barrier);

        hr = cmdList->Close();
        IM_ASSERT(SUCCEEDED(hr));

        cmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&cmdList);
        hr = cmdQueue->Signal(fence, 1);
        IM_ASSERT(SUCCEEDED(hr));

        fence->SetEventOnCompletion(1, event);
        WaitForSingleObject(event, INFINITE);

        cmdList->Release();
        cmdAlloc->Release();
        cmdQueue->Release();
        CloseHandle(event);
        fence->Release();
        uploadBuffer->Release();

        // Create texture view
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        bd->pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, bd->hFontSrvCpuDescHandle);
        SafeRelease(bd->pFontTextureResource);
        bd->pFontTextureResource = pTexture;
    }

    // Store our identifier
    // READ THIS IF THE STATIC_ASSERT() TRIGGERS:
    // - Important: to compile on 32-bit systems, this backend requires code to be compiled with '#define ImTextureID ImU64'.
    // - This is because we need ImTextureID to carry a 64-bit value and by default ImTextureID is defined as void*.
    // [Solution 1] IDE/msbuild: in "Properties/C++/Preprocessor Definitions" add 'ImTextureID=ImU64' (this is what we do in the 'example_win32_direct12/example_win32_direct12.vcxproj' project file)
    // [Solution 2] IDE/msbuild: in "Properties/C++/Preprocessor Definitions" add 'IMGUI_USER_CONFIG="my_imgui_config.h"' and inside 'my_imgui_config.h' add '#define ImTextureID ImU64' and as many other options as you like.
    // [Solution 3] IDE/msbuild: edit imconfig.h and add '#define ImTextureID ImU64' (prefer solution 2 to create your own config file!)
    // [Solution 4] command-line: add '/D ImTextureID=ImU64' to your cl.exe command-line (this is what we do in the example_win32_direct12/build_win32.bat file)
    static_assert(sizeof(ImTextureID) >= sizeof(bd->hFontSrvGpuDescHandle.ptr), "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
    io.Fonts->SetTexID((ImTextureID)bd->hFontSrvGpuDescHandle.ptr);
}

bool    ImGui_ImplDX12_CreateDeviceObjects()
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    if (!bd || !bd->pd3dDevice)
        return false;
    if (bd->pPipelineState)
        ImGui_ImplDX12_InvalidateDeviceObjects();

    // Create the root signature
    {
        D3D12_DESCRIPTOR_RANGE descRange = {};
        descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descRange.NumDescriptors = 1;
        descRange.BaseShaderRegister = 0;
        descRange.RegisterSpace = 0;
        descRange.OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER param[2] = {};

        param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param[0].Constants.ShaderRegister = 0;
        param[0].Constants.RegisterSpace = 0;
        param[0].Constants.Num32BitValues = 16;
        param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param[1].DescriptorTable.NumDescriptorRanges = 1;
        param[1].DescriptorTable.pDescriptorRanges = &descRange;
        param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling.
        D3D12_STATIC_SAMPLER_DESC staticSampler = {};
        staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.MipLODBias = 0.f;
        staticSampler.MaxAnisotropy = 0;
        staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        staticSampler.MinLOD = 0.f;
        staticSampler.MaxLOD = 0.f;
        staticSampler.ShaderRegister = 0;
        staticSampler.RegisterSpace = 0;
        staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters = _countof(param);
        desc.pParameters = param;
        desc.NumStaticSamplers = 1;
        desc.pStaticSamplers = &staticSampler;
        desc.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        // Load d3d12.dll and D3D12SerializeRootSignature() function address dynamically to facilitate using with D3D12On7.
        // See if any version of d3d12.dll is already loaded in the process. If so, give preference to that.
        static HINSTANCE d3d12_dll = ::GetModuleHandleA("d3d12.dll");
        if (d3d12_dll == nullptr)
        {
            // Attempt to load d3d12.dll from local directories. This will only succeed if
            // (1) the current OS is Windows 7, and
            // (2) there exists a version of d3d12.dll for Windows 7 (D3D12On7) in one of the following directories.
            // See https://github.com/ocornut/imgui/pull/3696 for details.
            const char* localD3d12Paths[] = { ".\\d3d12.dll", ".\\d3d12on7\\d3d12.dll", ".\\12on7\\d3d12.dll" }; // A. current directory, B. used by some games, C. used in Microsoft D3D12On7 sample
            for (int i = 0; i < IM_ARRAYSIZE(localD3d12Paths); i++)
                if ((d3d12_dll = ::LoadLibraryA(localD3d12Paths[i])) != nullptr)
                    break;

            // If failed, we are on Windows >= 10.
            if (d3d12_dll == nullptr)
                d3d12_dll = ::LoadLibraryA("d3d12.dll");

            if (d3d12_dll == nullptr)
                return false;
        }

        PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignatureFn = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)::GetProcAddress(d3d12_dll, "D3D12SerializeRootSignature");
        if (D3D12SerializeRootSignatureFn == nullptr)
            return false;

        ID3DBlob* blob = nullptr;
        if (D3D12SerializeRootSignatureFn(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr) != S_OK)
            return false;

        bd->pd3dDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&bd->pRootSignature));
        blob->Release();
    }

    // By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
    // If you would like to use this DX12 sample code but remove this dependency you can:
    //  1) compile once, save the compiled shader blobs into a file or source code and assign them to psoDesc.VS/PS [preferred solution]
    //  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL.
    // See https://github.com/ocornut/imgui/pull/638 for sources and details.

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.NodeMask = 1;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.pRootSignature = bd->pRootSignature;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = bd->RTVFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    ID3DBlob* vertexShaderBlob;
    ID3DBlob* pixelShaderBlob;

    // Create the vertex shader
    {
        static const char* vertexShader =
            "cbuffer vertexBuffer : register(b0) \
            {\
              float4x4 ProjectionMatrix; \
            };\
            struct VS_INPUT\
            {\
              float2 pos : POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
              PS_INPUT output;\
              output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
              output.col = input.col;\
              output.uv  = input.uv;\
              return output;\
            }";

        if (FAILED(D3DCompile(vertexShader, strlen(vertexShader), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexShaderBlob, nullptr)))
            return false; // NB: Pass ID3DBlob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
        psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };

        // Create the input layout
        static D3D12_INPUT_ELEMENT_DESC local_layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)offsetof(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)offsetof(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)offsetof(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        psoDesc.InputLayout = { local_layout, 3 };
    }

    // Create the pixel shader
    {
        static const char* pixelShader =
            "struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            SamplerState sampler0 : register(s0);\
            Texture2D texture0 : register(t0);\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
              float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
              return out_col; \
            }";

        if (FAILED(D3DCompile(pixelShader, strlen(pixelShader), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &pixelShaderBlob, nullptr)))
        {
            vertexShaderBlob->Release();
            return false; // NB: Pass ID3DBlob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
        }
        psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
    }

    // Create the blending setup
    {
        D3D12_BLEND_DESC& desc = psoDesc.BlendState;
        desc.AlphaToCoverageEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    // Create the rasterizer state
    {
        D3D12_RASTERIZER_DESC& desc = psoDesc.RasterizerState;
        desc.FillMode = D3D12_FILL_MODE_SOLID;
        desc.CullMode = D3D12_CULL_MODE_NONE;
        desc.FrontCounterClockwise = FALSE;
        desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        desc.DepthClipEnable = true;
        desc.MultisampleEnable = FALSE;
        desc.AntialiasedLineEnable = FALSE;
        desc.ForcedSampleCount = 0;
        desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }

    // Create depth-stencil State
    {
        D3D12_DEPTH_STENCIL_DESC& desc = psoDesc.DepthStencilState;
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.BackFace = desc.FrontFace;
    }

    HRESULT result_pipeline_state = bd->pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bd->pPipelineState));
    vertexShaderBlob->Release();
    pixelShaderBlob->Release();
    if (result_pipeline_state != S_OK)
        return false;

    ImGui_ImplDX12_CreateFontsTexture();

    return true;
}

static void ImGui_ImplDX12_DestroyRenderBuffers(ImGui_ImplDX12_RenderBuffers* render_buffers)
{
    SafeRelease(render_buffers->IndexBuffer);
    SafeRelease(render_buffers->VertexBuffer);
    render_buffers->IndexBufferSize = render_buffers->VertexBufferSize = 0;
}

void    ImGui_ImplDX12_InvalidateDeviceObjects()
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    if (!bd || !bd->pd3dDevice)
        return;

    ImGuiIO& io = ImGui::GetIO();
    SafeRelease(bd->pRootSignature);
    SafeRelease(bd->pPipelineState);
    SafeRelease(bd->pFontTextureResource);
    io.Fonts->SetTexID(0); // We copied bd->pFontTextureView to io.Fonts->TexID so let's clear that as well.
}

bool ImGui_ImplDX12_Init(ID3D12Device* device, int num_frames_in_flight, DXGI_FORMAT rtv_format, ID3D12DescriptorHeap* cbv_srv_heap,
                         D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplDX12_Data* bd = IM_NEW(ImGui_ImplDX12_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_dx12";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        ImGui_ImplDX12_InitPlatformInterface();

    bd->pd3dDevice = device;
    bd->RTVFormat = rtv_format;
    bd->hFontSrvCpuDescHandle = font_srv_cpu_desc_handle;
    bd->hFontSrvGpuDescHandle = font_srv_gpu_desc_handle;
    bd->numFramesInFlight = num_frames_in_flight;
    bd->pd3dSrvDescHeap = cbv_srv_heap;

    // Create a dummy ImGui_ImplDX12_ViewportData holder for the main viewport,
    // Since this is created and managed by the application, we will only use the ->Resources[] fields.
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->RendererUserData = IM_NEW(ImGui_ImplDX12_ViewportData)(bd->numFramesInFlight);

    return true;
}

void ImGui_ImplDX12_Shutdown()
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    // Manually delete main viewport render resources in-case we haven't initialized for viewports
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    if (ImGui_ImplDX12_ViewportData* vd = (ImGui_ImplDX12_ViewportData*)main_viewport->RendererUserData)
    {
        // We could just call ImGui_ImplDX12_DestroyWindow(main_viewport) as a convenience but that would be misleading since we only use data->Resources[]
        for (UINT i = 0; i < bd->numFramesInFlight; i++)
            ImGui_ImplDX12_DestroyRenderBuffers(&vd->FrameRenderBuffers[i]);
        IM_DELETE(vd);
        main_viewport->RendererUserData = nullptr;
    }

    // Clean up windows and device objects
    ImGui_ImplDX12_ShutdownPlatformInterface();
    ImGui_ImplDX12_InvalidateDeviceObjects();

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasViewports);
    IM_DELETE(bd);
}

void ImGui_ImplDX12_NewFrame()
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplDX12_Init()?");

    if (!bd->pPipelineState)
        ImGui_ImplDX12_CreateDeviceObjects();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

static void ImGui_ImplDX12_CreateWindow(ImGuiViewport* viewport)
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    ImGui_ImplDX12_ViewportData* vd = IM_NEW(ImGui_ImplDX12_ViewportData)(bd->numFramesInFlight);
    viewport->RendererUserData = vd;

    // PlatformHandleRaw should always be a HWND, whereas PlatformHandle might be a higher-level handle (e.g. GLFWWindow*, SDL_Window*).
    // Some backends will leave PlatformHandleRaw == 0, in which case we assume PlatformHandle will contain the HWND.
    HWND hwnd = viewport->PlatformHandleRaw ? (HWND)viewport->PlatformHandleRaw : (HWND)viewport->PlatformHandle;
    IM_ASSERT(hwnd != 0);

    vd->FrameIndex = UINT_MAX;

    // Create command queue.
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    HRESULT res = S_OK;
    res = bd->pd3dDevice->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&vd->CommandQueue));
    IM_ASSERT(res == S_OK);

    // Create command allocator.
    for (UINT i = 0; i < bd->numFramesInFlight; ++i)
    {
        res = bd->pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&vd->FrameCtx[i].CommandAllocator));
        IM_ASSERT(res == S_OK);
    }

    // Create command list.
    res = bd->pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, vd->FrameCtx[0].CommandAllocator, nullptr, IID_PPV_ARGS(&vd->CommandList));
    IM_ASSERT(res == S_OK);
    vd->CommandList->Close();

    // Create fence.
    res = bd->pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&vd->Fence));
    IM_ASSERT(res == S_OK);

    vd->FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    IM_ASSERT(vd->FenceEvent != nullptr);

    // Create swap chain
    // FIXME-VIEWPORT: May want to copy/inherit swap chain settings from the user/application.
    DXGI_SWAP_CHAIN_DESC1 sd1;
    ZeroMemory(&sd1, sizeof(sd1));
    sd1.BufferCount = bd->numFramesInFlight;
    sd1.Width = (UINT)viewport->Size.x;
    sd1.Height = (UINT)viewport->Size.y;
    sd1.Format = bd->RTVFormat;
    sd1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd1.SampleDesc.Count = 1;
    sd1.SampleDesc.Quality = 0;
    sd1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    sd1.Scaling = DXGI_SCALING_NONE;
    sd1.Stereo = FALSE;

    IDXGIFactory4* dxgi_factory = nullptr;
    res = ::CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory));
    IM_ASSERT(res == S_OK);

    IDXGISwapChain1* swap_chain = nullptr;
    res = dxgi_factory->CreateSwapChainForHwnd(vd->CommandQueue, hwnd, &sd1, nullptr, nullptr, &swap_chain);
    IM_ASSERT(res == S_OK);

    dxgi_factory->Release();

    // Or swapChain.As(&mSwapChain)
    IM_ASSERT(vd->SwapChain == nullptr);
    swap_chain->QueryInterface(IID_PPV_ARGS(&vd->SwapChain));
    swap_chain->Release();

    // Create the render targets
    if (vd->SwapChain)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = bd->numFramesInFlight;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;

        HRESULT hr = bd->pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&vd->RtvDescHeap));
        IM_ASSERT(hr == S_OK);

        SIZE_T rtv_descriptor_size = bd->pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = vd->RtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < bd->numFramesInFlight; i++)
        {
            vd->FrameCtx[i].RenderTargetCpuDescriptors = rtv_handle;
            rtv_handle.ptr += rtv_descriptor_size;
        }

        ID3D12Resource* back_buffer;
        for (UINT i = 0; i < bd->numFramesInFlight; i++)
        {
            IM_ASSERT(vd->FrameCtx[i].RenderTarget == nullptr);
            vd->SwapChain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));
            bd->pd3dDevice->CreateRenderTargetView(back_buffer, nullptr, vd->FrameCtx[i].RenderTargetCpuDescriptors);
            vd->FrameCtx[i].RenderTarget = back_buffer;
        }
    }

    for (UINT i = 0; i < bd->numFramesInFlight; i++)
        ImGui_ImplDX12_DestroyRenderBuffers(&vd->FrameRenderBuffers[i]);
}

static void ImGui_WaitForPendingOperations(ImGui_ImplDX12_ViewportData* vd)
{
    HRESULT hr = S_FALSE;
    if (vd && vd->CommandQueue && vd->Fence && vd->FenceEvent)
    {
        hr = vd->CommandQueue->Signal(vd->Fence, ++vd->FenceSignaledValue);
        IM_ASSERT(hr == S_OK);
        ::WaitForSingleObject(vd->FenceEvent, 0); // Reset any forgotten waits
        hr = vd->Fence->SetEventOnCompletion(vd->FenceSignaledValue, vd->FenceEvent);
        IM_ASSERT(hr == S_OK);
        ::WaitForSingleObject(vd->FenceEvent, INFINITE);
    }
}

static void ImGui_ImplDX12_DestroyWindow(ImGuiViewport* viewport)
{
    // The main viewport (owned by the application) will always have RendererUserData == 0 since we didn't create the data for it.
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    if (ImGui_ImplDX12_ViewportData* vd = (ImGui_ImplDX12_ViewportData*)viewport->RendererUserData)
    {
        ImGui_WaitForPendingOperations(vd);

        SafeRelease(vd->CommandQueue);
        SafeRelease(vd->CommandList);
        SafeRelease(vd->SwapChain);
        SafeRelease(vd->RtvDescHeap);
        SafeRelease(vd->Fence);
        ::CloseHandle(vd->FenceEvent);
        vd->FenceEvent = nullptr;

        for (UINT i = 0; i < bd->numFramesInFlight; i++)
        {
            SafeRelease(vd->FrameCtx[i].RenderTarget);
            SafeRelease(vd->FrameCtx[i].CommandAllocator);
            ImGui_ImplDX12_DestroyRenderBuffers(&vd->FrameRenderBuffers[i]);
        }
        IM_DELETE(vd);
    }
    viewport->RendererUserData = nullptr;
}

static void ImGui_ImplDX12_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    ImGui_ImplDX12_ViewportData* vd = (ImGui_ImplDX12_ViewportData*)viewport->RendererUserData;

    ImGui_WaitForPendingOperations(vd);

    for (UINT i = 0; i < bd->numFramesInFlight; i++)
        SafeRelease(vd->FrameCtx[i].RenderTarget);

    if (vd->SwapChain)
    {
        ID3D12Resource* back_buffer = nullptr;
        vd->SwapChain->ResizeBuffers(0, (UINT)size.x, (UINT)size.y, DXGI_FORMAT_UNKNOWN, 0);
        for (UINT i = 0; i < bd->numFramesInFlight; i++)
        {
            vd->SwapChain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));
            bd->pd3dDevice->CreateRenderTargetView(back_buffer, nullptr, vd->FrameCtx[i].RenderTargetCpuDescriptors);
            vd->FrameCtx[i].RenderTarget = back_buffer;
        }
    }
}

static void ImGui_ImplDX12_RenderWindow(ImGuiViewport* viewport, void*)
{
    ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
    ImGui_ImplDX12_ViewportData* vd = (ImGui_ImplDX12_ViewportData*)viewport->RendererUserData;

    ImGui_ImplDX12_FrameContext* frame_context = &vd->FrameCtx[vd->FrameIndex % bd->numFramesInFlight];
    UINT back_buffer_idx = vd->SwapChain->GetCurrentBackBufferIndex();

    const ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = vd->FrameCtx[back_buffer_idx].RenderTarget;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    // Draw
    ID3D12GraphicsCommandList* cmd_list = vd->CommandList;

    frame_context->CommandAllocator->Reset();
    cmd_list->Reset(frame_context->CommandAllocator, nullptr);
    cmd_list->ResourceBarrier(1, &barrier);
    cmd_list->OMSetRenderTargets(1, &vd->FrameCtx[back_buffer_idx].RenderTargetCpuDescriptors, FALSE, nullptr);
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
        cmd_list->ClearRenderTargetView(vd->FrameCtx[back_buffer_idx].RenderTargetCpuDescriptors, (float*)&clear_color, 0, nullptr);
    cmd_list->SetDescriptorHeaps(1, &bd->pd3dSrvDescHeap);

    ImGui_ImplDX12_RenderDrawData(viewport->DrawData, cmd_list);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    cmd_list->ResourceBarrier(1, &barrier);
    cmd_list->Close();

    vd->CommandQueue->Wait(vd->Fence, vd->FenceSignaledValue);
    vd->CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&cmd_list);
    vd->CommandQueue->Signal(vd->Fence, ++vd->FenceSignaledValue);
}

static void ImGui_ImplDX12_SwapBuffers(ImGuiViewport* viewport, void*)
{
    ImGui_ImplDX12_ViewportData* vd = (ImGui_ImplDX12_ViewportData*)viewport->RendererUserData;

    vd->SwapChain->Present(0, 0);
    while (vd->Fence->GetCompletedValue() < vd->FenceSignaledValue)
        ::SwitchToThread();
}

void ImGui_ImplDX12_InitPlatformInterface()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_CreateWindow = ImGui_ImplDX12_CreateWindow;
    platform_io.Renderer_DestroyWindow = ImGui_ImplDX12_DestroyWindow;
    platform_io.Renderer_SetWindowSize = ImGui_ImplDX12_SetWindowSize;
    platform_io.Renderer_RenderWindow = ImGui_ImplDX12_RenderWindow;
    platform_io.Renderer_SwapBuffers = ImGui_ImplDX12_SwapBuffers;
}

void ImGui_ImplDX12_ShutdownPlatformInterface()
{
    ImGui::DestroyPlatformWindows();
}

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
