// dear imgui: Renderer for DirectX12
// This needs to be used along with a Platform Binding (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'D3D12_GPU_DESCRIPTOR_HANDLE' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Renderer: Multi-viewport support. Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.
//      FIXME: The transition from removing a viewport and moving the window in an existing hosted viewport tends to flicker.
//  [X] Renderer: Support for large meshes (64k+ vertices) with 16-bits indices.
// Missing features, issues:
//  [ ] 64-bit only for now! (Because sizeof(ImTextureId) == sizeof(void*)). See github.com/ocornut/imgui/pull/301

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
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
#include "imgui_impl_dx12.h"

// DirectX
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
#endif

// DirectX data
static ID3D12Device*                g_pd3dDevice = NULL;
static ID3D10Blob*                  g_pVertexShaderBlob = NULL;
static ID3D10Blob*                  g_pPixelShaderBlob = NULL;
static ID3D12RootSignature*         g_pRootSignature = NULL;
static ID3D12PipelineState*         g_pPipelineState = NULL;
static DXGI_FORMAT                  g_RTVFormat = DXGI_FORMAT_UNKNOWN;
static ID3D12Resource*              g_pFontTextureResource = NULL;
static D3D12_CPU_DESCRIPTOR_HANDLE  g_hFontSrvCpuDescHandle = {};
static D3D12_GPU_DESCRIPTOR_HANDLE  g_hFontSrvGpuDescHandle = {};
static ID3D12DescriptorHeap*        g_pd3dSrvDescHeap = NULL;
static UINT                         g_numFramesInFlight = 0;

struct FrameResources
{
    ID3D12Resource*     IndexBuffer;
    ID3D12Resource*     VertexBuffer;
    int                 IndexBufferSize;
    int                 VertexBufferSize;
};

struct FrameContext
{
    ID3D12CommandAllocator*     CommandAllocator;
    ID3D12Resource*             RenderTarget;
    D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetCpuDescriptors;
};

struct ImGuiViewportDataDx12
{
    ID3D12CommandQueue*         CommandQueue;
    ID3D12GraphicsCommandList*  CommandList;
    ID3D12DescriptorHeap*       RtvDescHeap;
    IDXGISwapChain3*            SwapChain;

    ID3D12Fence*                Fence;
    UINT64                      FenceSignaledValue;
    HANDLE                      FenceEvent;

    UINT                        FrameIndex;
    FrameContext*               FrameCtx;
    FrameResources*             Resources;

    ImGuiViewportDataDx12()
    {
        CommandQueue = NULL;
        CommandList = NULL;
        RtvDescHeap = NULL;
        SwapChain = NULL;

        Fence = NULL;
        FenceSignaledValue = 0;
        FenceEvent = NULL;
        FrameIndex = UINT_MAX;
        FrameCtx = new FrameContext[g_numFramesInFlight];
        Resources = new FrameResources[g_numFramesInFlight];

        for (UINT i = 0; i < g_numFramesInFlight; ++i)
        {
            FrameCtx[i].CommandAllocator = NULL;
            FrameCtx[i].RenderTarget = NULL;

            // Create buffers with a default size (they will later be grown as needed)
            Resources[i].IndexBuffer = NULL;
            Resources[i].VertexBuffer = NULL;
            Resources[i].VertexBufferSize = 5000;
            Resources[i].IndexBufferSize = 10000;
        }
    }
    ~ImGuiViewportDataDx12()
    {
        IM_ASSERT(CommandQueue == NULL && CommandList == NULL);
        IM_ASSERT(RtvDescHeap == NULL);
        IM_ASSERT(SwapChain == NULL);
        IM_ASSERT(Fence == NULL);
        IM_ASSERT(FenceEvent == NULL);

        for (UINT i = 0; i < g_numFramesInFlight; ++i)
        {
            IM_ASSERT(FrameCtx[i].CommandAllocator == NULL && FrameCtx[i].RenderTarget == NULL);
            IM_ASSERT(Resources[i].IndexBuffer == NULL && Resources[i].VertexBuffer == NULL);
        }

        delete[] FrameCtx; FrameCtx = NULL;
        delete[] Resources; Resources = NULL;
    }
};

template<typename T>
static void SafeRelease(T*& res)
{
    if (res)
        res->Release();
    res = NULL;
}

struct VERTEX_CONSTANT_BUFFER
{
    float   mvp[4][4];
};

// Forward Declarations
static void ImGui_ImplDX12_InitPlatformInterface();
static void ImGui_ImplDX12_ShutdownPlatformInterface();

static void ImGui_ImplDX12_SetupRenderState(ImDrawData* draw_data, ID3D12GraphicsCommandList* ctx, FrameResources* fr)
{
    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
    VERTEX_CONSTANT_BUFFER vertex_constant_buffer;
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
    ctx->SetPipelineState(g_pPipelineState);
    ctx->SetGraphicsRootSignature(g_pRootSignature);
    ctx->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);

    // Setup blend factor
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    ctx->OMSetBlendFactor(blend_factor);
}

// Render function
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
void ImGui_ImplDX12_RenderDrawData(ImDrawData* draw_data, ID3D12GraphicsCommandList* ctx)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    ImGuiViewportDataDx12* render_data = (ImGuiViewportDataDx12*)draw_data->OwnerViewport->RendererUserData;
    render_data->FrameIndex++;
    FrameResources* fr = &render_data->Resources[render_data->FrameIndex % g_numFramesInFlight];

    // Create and grow vertex/index buffers if needed
    if (fr->VertexBuffer == NULL || fr->VertexBufferSize < draw_data->TotalVtxCount)
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
        if (g_pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&fr->VertexBuffer)) < 0)
            return;
    }
    if (fr->IndexBuffer == NULL || fr->IndexBufferSize < draw_data->TotalIdxCount)
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
        if (g_pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&fr->IndexBuffer)) < 0)
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
            if (pcmd->UserCallback != NULL)
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
                // Apply Scissor, Bind texture, Draw
                const D3D12_RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
                ctx->SetGraphicsRootDescriptorTable(1, *(D3D12_GPU_DESCRIPTOR_HANDLE*)&pcmd->TextureId);
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

        ID3D12Resource* pTexture = NULL;
        g_pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&pTexture));

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

        ID3D12Resource* uploadBuffer = NULL;
        HRESULT hr = g_pd3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer));
        IM_ASSERT(SUCCEEDED(hr));

        void* mapped = NULL;
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

        ID3D12Fence* fence = NULL;
        hr = g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        IM_ASSERT(SUCCEEDED(hr));

        HANDLE event = CreateEvent(0, 0, 0, 0);
        IM_ASSERT(event != NULL);

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 1;

        ID3D12CommandQueue* cmdQueue = NULL;
        hr = g_pd3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));
        IM_ASSERT(SUCCEEDED(hr));

        ID3D12CommandAllocator* cmdAlloc = NULL;
        hr = g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
        IM_ASSERT(SUCCEEDED(hr));

        ID3D12GraphicsCommandList* cmdList = NULL;
        hr = g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc, NULL, IID_PPV_ARGS(&cmdList));
        IM_ASSERT(SUCCEEDED(hr));

        cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);
        cmdList->ResourceBarrier(1, &barrier);

        hr = cmdList->Close();
        IM_ASSERT(SUCCEEDED(hr));

        cmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*) &cmdList);
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
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, g_hFontSrvCpuDescHandle);
        SafeRelease(g_pFontTextureResource);
        g_pFontTextureResource = pTexture;
    }

    // Store our identifier
    static_assert(sizeof(ImTextureID) >= sizeof(g_hFontSrvGpuDescHandle.ptr), "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
    io.Fonts->TexID = (ImTextureID)g_hFontSrvGpuDescHandle.ptr;
}

bool    ImGui_ImplDX12_CreateDeviceObjects()
{
    if (!g_pd3dDevice)
        return false;
    if (g_pPipelineState)
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

        ID3DBlob* blob = NULL;
        if (D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, NULL) != S_OK)
            return false;

        g_pd3dDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&g_pRootSignature));
        blob->Release();
    }

    // By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
    // If you would like to use this DX12 sample code but remove this dependency you can:
    //  1) compile once, save the compiled shader blobs into a file or source code and pass them to CreateVertexShader()/CreatePixelShader() [preferred solution]
    //  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL.
    // See https://github.com/ocornut/imgui/pull/638 for sources and details.

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.NodeMask = 1;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.pRootSignature = g_pRootSignature;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = g_RTVFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

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

        D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &g_pVertexShaderBlob, NULL);
        if (g_pVertexShaderBlob == NULL) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
            return false;
        psoDesc.VS = { g_pVertexShaderBlob->GetBufferPointer(), g_pVertexShaderBlob->GetBufferSize() };

        // Create the input layout
        static D3D12_INPUT_ELEMENT_DESC local_layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

        D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &g_pPixelShaderBlob, NULL);
        if (g_pPixelShaderBlob == NULL)  // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
            return false;
        psoDesc.PS = { g_pPixelShaderBlob->GetBufferPointer(), g_pPixelShaderBlob->GetBufferSize() };
    }

    // Create the blending setup
    {
        D3D12_BLEND_DESC& desc = psoDesc.BlendState;
        desc.AlphaToCoverageEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
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

    if (g_pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pPipelineState)) != S_OK)
        return false;

    ImGui_ImplDX12_CreateFontsTexture();

    return true;
}

void    ImGui_ImplDX12_InvalidateDeviceObjects()
{
    if (!g_pd3dDevice)
        return;

    SafeRelease(g_pVertexShaderBlob);
    SafeRelease(g_pPixelShaderBlob);
    SafeRelease(g_pRootSignature);
    SafeRelease(g_pPipelineState);
    SafeRelease(g_pFontTextureResource);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->TexID = NULL; // We copied g_pFontTextureView to io.Fonts->TexID so let's clear that as well.
}

bool ImGui_ImplDX12_Init(ID3D12Device* device, int num_frames_in_flight, DXGI_FORMAT rtv_format, ID3D12DescriptorHeap* cbv_srv_heap,
                         D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle)
{
    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_dx12";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional) // FIXME-VIEWPORT: Actually unfinished..

    g_pd3dDevice = device;
    g_RTVFormat = rtv_format;
    g_hFontSrvCpuDescHandle = font_srv_cpu_desc_handle;
    g_hFontSrvGpuDescHandle = font_srv_gpu_desc_handle;
    g_numFramesInFlight = num_frames_in_flight;
    g_pd3dSrvDescHeap = cbv_srv_heap;

    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->RendererUserData = IM_NEW(ImGuiViewportDataDx12)();

    // Setup back-end capabilities flags
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;    // We can create multi-viewports on the Renderer side (optional)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        ImGui_ImplDX12_InitPlatformInterface();

    return true;
}

void ImGui_ImplDX12_Shutdown()
{
    ImGui_ImplDX12_ShutdownPlatformInterface();
    ImGui_ImplDX12_InvalidateDeviceObjects();

    g_pd3dDevice = NULL;
    g_hFontSrvCpuDescHandle.ptr = 0;
    g_hFontSrvGpuDescHandle.ptr = 0;
    g_numFramesInFlight = 0;
    g_pd3dSrvDescHeap = NULL;

    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    if (ImGuiViewportDataDx12* data = (ImGuiViewportDataDx12*)main_viewport->RendererUserData)
        IM_DELETE(data);
    main_viewport->RendererUserData = NULL;
}

void ImGui_ImplDX12_NewFrame()
{
    if (!g_pPipelineState)
        ImGui_ImplDX12_CreateDeviceObjects();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the back-end to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

static void ImGui_ImplDX12_CreateWindow(ImGuiViewport* viewport)
{
    ImGuiViewportDataDx12* data = IM_NEW(ImGuiViewportDataDx12)();
    viewport->RendererUserData = data;

    // PlatformHandleRaw should always be a HWND, whereas PlatformHandle might be a higher-level handle (e.g. GLFWWindow*, SDL_Window*).
    // Some back-ends will leave PlatformHandleRaw NULL, in which case we assume PlatformHandle will contain the HWND.
    HWND hwnd = viewport->PlatformHandleRaw ? (HWND)viewport->PlatformHandleRaw : (HWND)viewport->PlatformHandle;
    IM_ASSERT(hwnd != 0);

    data->FrameIndex = UINT_MAX;

    // Create command queue.
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    HRESULT res = S_OK;
    res = g_pd3dDevice->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&data->CommandQueue));
    IM_ASSERT(res == S_OK);

    // Create command allocator.
    for (UINT i = 0; i < g_numFramesInFlight; ++i)
    {
        res = g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&data->FrameCtx[i].CommandAllocator));
        IM_ASSERT(res == S_OK);
    }

    // Create command list.
    res = g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, data->FrameCtx[0].CommandAllocator, NULL, IID_PPV_ARGS(&data->CommandList));
    IM_ASSERT(res == S_OK);
    data->CommandList->Close();

    // Create fence.
    res = g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&data->Fence));
    IM_ASSERT(res == S_OK);

    data->FenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    IM_ASSERT(data->FenceEvent != NULL);

    // Create swap chain
    // FIXME-VIEWPORT: May want to copy/inherit swap chain settings from the user/application.
    DXGI_SWAP_CHAIN_DESC1 sd1;
    ZeroMemory(&sd1, sizeof(sd1));
    sd1.BufferCount = g_numFramesInFlight;
    sd1.Width = (UINT)viewport->Size.x;
    sd1.Height = (UINT)viewport->Size.y;
    sd1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd1.SampleDesc.Count = 1;
    sd1.SampleDesc.Quality = 0;
    sd1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    sd1.Scaling = DXGI_SCALING_STRETCH;
    sd1.Stereo = FALSE;

    IDXGIFactory4* dxgi_factory = NULL;
    res = ::CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory));
    IM_ASSERT(res == S_OK);

    IDXGISwapChain1* swap_chain = NULL;
    res = dxgi_factory->CreateSwapChainForHwnd(data->CommandQueue, hwnd, &sd1, NULL, NULL, &swap_chain);
    IM_ASSERT(res == S_OK);

    dxgi_factory->Release();

    // Or swapChain.As(&mSwapChain)
    IM_ASSERT(data->SwapChain == NULL);
    swap_chain->QueryInterface(IID_PPV_ARGS(&data->SwapChain));
    swap_chain->Release();

    // Create the render targets
    if (data->SwapChain)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = g_numFramesInFlight;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;

        IM_ASSERT(g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&data->RtvDescHeap)) == S_OK);

        SIZE_T rtv_descriptor_size = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = data->RtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < g_numFramesInFlight; i++)
        {
            data->FrameCtx[i].RenderTargetCpuDescriptors = rtv_handle;
            rtv_handle.ptr += rtv_descriptor_size;
        }

        ID3D12Resource* back_buffer;
        for (UINT i = 0; i < g_numFramesInFlight; i++)
        {
            IM_ASSERT(data->FrameCtx[i].RenderTarget == NULL);
            data->SwapChain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));
            g_pd3dDevice->CreateRenderTargetView(back_buffer, NULL, data->FrameCtx[i].RenderTargetCpuDescriptors);
            data->FrameCtx[i].RenderTarget = back_buffer;
        }
    }

    for (UINT i = 0; i < g_numFramesInFlight; i++)
    {
        SafeRelease(data->Resources[i].IndexBuffer);
        SafeRelease(data->Resources[i].VertexBuffer);
    }
}

static void ImGui_ImplDX12_DestroyWindow(ImGuiViewport* viewport)
{
    // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
    if (ImGuiViewportDataDx12* data = (ImGuiViewportDataDx12*)viewport->RendererUserData)
    {
        SafeRelease(data->CommandQueue);
        SafeRelease(data->CommandList);
        SafeRelease(data->SwapChain);
        SafeRelease(data->RtvDescHeap);
        SafeRelease(data->Fence);
        ::CloseHandle(data->FenceEvent); 
        data->FenceEvent = NULL;

        for (UINT i = 0; i < g_numFramesInFlight; i++)
        {
            SafeRelease(data->FrameCtx[i].RenderTarget);
            SafeRelease(data->FrameCtx[i].CommandAllocator);
            SafeRelease(data->Resources[i].IndexBuffer);
            SafeRelease(data->Resources[i].VertexBuffer);
        }
        IM_DELETE(data);
    }
    viewport->RendererUserData = NULL;
}

static void ImGui_ImplDX12_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGuiViewportDataDx12* data = (ImGuiViewportDataDx12*)viewport->RendererUserData;

    for (UINT i = 0; i < g_numFramesInFlight; i++)
        SafeRelease(data->FrameCtx[i].RenderTarget);

    if (data->SwapChain)
    {
        ID3D12Resource* back_buffer = NULL;
        data->SwapChain->ResizeBuffers(0, (UINT)size.x, (UINT)size.y, DXGI_FORMAT_UNKNOWN, 0);
        for (UINT i = 0; i < g_numFramesInFlight; i++)
        {
            data->SwapChain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));
            g_pd3dDevice->CreateRenderTargetView(back_buffer, NULL, data->FrameCtx[i].RenderTargetCpuDescriptors);
            data->FrameCtx[i].RenderTarget = back_buffer;
        }
    }
}

static void ImGui_ImplDX12_RenderWindow(ImGuiViewport* viewport, void*)
{
    ImGuiViewportDataDx12* data = (ImGuiViewportDataDx12*)viewport->RendererUserData;

    FrameContext* frame_context = &data->FrameCtx[data->FrameIndex % g_numFramesInFlight];
    UINT back_buffer_idx = data->SwapChain->GetCurrentBackBufferIndex();

    const ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = data->FrameCtx[back_buffer_idx].RenderTarget;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    // Draw
    ID3D12GraphicsCommandList* cmd_list = data->CommandList;

    frame_context->CommandAllocator->Reset();
    cmd_list->Reset(frame_context->CommandAllocator, NULL);
    cmd_list->ResourceBarrier(1, &barrier);
    cmd_list->OMSetRenderTargets(1, &data->FrameCtx[back_buffer_idx].RenderTargetCpuDescriptors, FALSE, NULL);
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
        cmd_list->ClearRenderTargetView(data->FrameCtx[back_buffer_idx].RenderTargetCpuDescriptors, (float*)&clear_color, 0, NULL);
    cmd_list->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);

    ImGui_ImplDX12_RenderDrawData(viewport->DrawData, cmd_list);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    cmd_list->ResourceBarrier(1, &barrier);
    cmd_list->Close();

    data->CommandQueue->Wait(data->Fence, data->FenceSignaledValue);
    data->CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&cmd_list);
    data->CommandQueue->Signal(data->Fence, ++data->FenceSignaledValue);
}

static void ImGui_ImplDX12_SwapBuffers(ImGuiViewport* viewport, void*)
{
    ImGuiViewportDataDx12* data = (ImGuiViewportDataDx12*)viewport->RendererUserData;

    data->SwapChain->Present(0, 0);
    while (data->Fence->GetCompletedValue() < data->FenceSignaledValue)
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
