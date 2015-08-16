#include "imgui.h"
#include "imgui_impl_dx12.h"

#include <stdint.h>

#include <windows.h>
#include <D3D12.h>
#include <D3DCompiler.h>

ID3D12GraphicsCommandList* g_commandList;
D3D12_CPU_DESCRIPTOR_HANDLE g_renderTarget;
ID3D12Resource* g_uploadBuffer;
ID3D12PipelineState* g_pipelineState;

// Data
static INT64                    g_Time = 0;
static INT64                    g_TicksPerSecond = 0;
static HWND						g_hwnd;


// dxgi, device, cmd queue, cmd allocator
// command lists genereation with vsh,psh
#define IMGUI_GPU_BUFFER_SIZE 1024*1024

// For creation of resources in the GPU
struct HeapProperty
{
    enum Enum
    {
        Default,
        Upload,
        ReadBack,

        Count
    };

    D3D12_HEAP_PROPERTIES m_properties;
    D3D12_RESOURCE_STATES m_state;
};

static const HeapProperty s_heapProperties[] =
{
    { { D3D12_HEAP_TYPE_DEFAULT,  D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 }, D3D12_RESOURCE_STATE_COMMON       },
    { { D3D12_HEAP_TYPE_UPLOAD,   D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 }, D3D12_RESOURCE_STATE_GENERIC_READ },
    { { D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 }, D3D12_RESOURCE_STATE_COPY_DEST    },
};

static ID3D12Resource* createCommittedResource(ID3D12Device* _device, HeapProperty::Enum _heapProperty, D3D12_RESOURCE_DESC* _resourceDesc, D3D12_CLEAR_VALUE* _clearValue)
{
    const HeapProperty& heapProperty = s_heapProperties[_heapProperty];
    ID3D12Resource* resource;
    _device->CreateCommittedResource(&heapProperty.m_properties
                , D3D12_HEAP_FLAG_NONE
                , _resourceDesc
                , heapProperty.m_state
                , _clearValue
                , __uuidof(ID3D12Resource)
                , (void**)&resource
                );

    return resource;
}

static ID3D12Resource* createCommittedResource(ID3D12Device* _device, HeapProperty::Enum _heapProperty, uint64_t _size, D3D12_RESOURCE_FLAGS _flags = D3D12_RESOURCE_FLAG_NONE)
{
    D3D12_RESOURCE_DESC resourceDesc;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width     = _size;
    resourceDesc.Height    = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count   = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags  = _flags;

    return createCommittedResource(_device, _heapProperty, &resourceDesc, NULL);
}

static void setResourceBarrier(ID3D12GraphicsCommandList* _commandList, ID3D12Resource* _resource, D3D12_RESOURCE_STATES _stateBefore, D3D12_RESOURCE_STATES _stateAfter)
{
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = _resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = _stateBefore;
    barrier.Transition.StateAfter  = _stateAfter;
    _commandList->ResourceBarrier(1, &barrier);
}

void ImGui_ImplDX12_CreateDeviceObjects(ID3D12Device* _device
        , ID3D12GraphicsCommandList* _commandList
        , ID3D12CommandQueue* _commandQueue
        , ID3D12Fence* _fence
        , D3D12_CPU_DESCRIPTOR_HANDLE _srvHandle
        , ID3D12RootSignature* _rootSignature)

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

    ID3DBlob* vshBlob = 0;
    ID3DBlob* pshBlob = 0;
    ID3DBlob* errorBlob = 0;

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (size_t)(&((ImDrawVert*)0)->pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, (size_t)(&((ImDrawVert*)0)->uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, (size_t)(&((ImDrawVert*)0)->col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };


    D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_5_0", D3DCOMPILE_DEBUG, 0, &vshBlob, &errorBlob);
    D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_5_0", D3DCOMPILE_DEBUG, 0, &pshBlob, &errorBlob);

    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.FrontCounterClockwise = true;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.f;
    rasterizerDesc.SlopeScaledDepthBias = 0.f;
    rasterizerDesc.DepthClipEnable = true;
    rasterizerDesc.MultisampleEnable = false;
    rasterizerDesc.AntialiasedLineEnable = true;
    rasterizerDesc.ForcedSampleCount = 1;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blendDesc;
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].LogicOpEnable = false;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // Create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = _rootSignature;
    psoDesc.VS = { (uint8_t*)(vshBlob->GetBufferPointer()), vshBlob->GetBufferSize() };
    psoDesc.PS = { (uint8_t*)(pshBlob->GetBufferPointer()), pshBlob->GetBufferSize() };
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    _device->CreateGraphicsPipelineState(&psoDesc, __uuidof(ID3D12PipelineState), (void**)&g_pipelineState);

    // Create upload ring buffer, which we'll also use as staging buffer for the texture
    g_uploadBuffer = createCommittedResource(_device, HeapProperty::Upload, IMGUI_GPU_BUFFER_SIZE * 8);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.f;

    // Create fonts texture and SRV descriptor for it
    unsigned char* pixels = 0;
    int width, height;

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Create fonts texture
    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width     = width;
    desc.Height    = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags  = D3D12_RESOURCE_FLAG_NONE;
    ID3D12Resource* fontResource = createCommittedResource(_device, HeapProperty::Default, &desc, 0);
    
    // Upload the fonts texture
    uint32_t subres = 0;
    uint32_t numRows;
    uint64_t rowPitch;
    uint64_t totalBytes;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
    _device->GetCopyableFootprints(&desc
            , subres
            , 1
            , 0
            , &layout
            , &numRows
            , &rowPitch
            , &totalBytes
            );

    uint8_t* mappedBuffer;

    // Upload the font
    g_uploadBuffer->Map(0, NULL, (void**)&mappedBuffer);
    memcpy(mappedBuffer, pixels, (size_t)totalBytes);
    g_uploadBuffer->Unmap(0, NULL);

    D3D12_BOX box;
    box.left   = 0;
    box.top    = 0;
    box.right  = (UINT)desc.Width;
    box.bottom = (UINT)desc.Height;
    box.front  = 0;
    box.back   = 1;

    D3D12_TEXTURE_COPY_LOCATION dst = { fontResource,   D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, { subres } };
    D3D12_TEXTURE_COPY_LOCATION src = { g_uploadBuffer, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,  layout     };
    _commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);
    _commandList->Close();

    ID3D12CommandList* ppCommandLists[] = { _commandList };
    _commandQueue->ExecuteCommandLists(1, ppCommandLists);
    _commandQueue->Signal(_fence, 0);

    _device->CreateShaderResourceView(fontResource, &srvDesc, _srvHandle);
}

void ImGui_ImplDX12_SetRenderData(ID3D12GraphicsCommandList* _commandList, D3D12_CPU_DESCRIPTOR_HANDLE _renderTarget)
{
    g_commandList = _commandList;
    g_renderTarget = _renderTarget;
}

void ImGui_ImplDX12_RenderDrawLists(ImDrawData* _draw_data)
{
    // Range CPU will read from mapping the upload buffer
    // End < Begin specifies CPU will not read the mapped buffer
    D3D12_RANGE readRange;
    readRange.End = 0;
    readRange.Begin = 1;

    char* mappedBuffer = 0;
    g_uploadBuffer->Map(0, &readRange, (void**)&mappedBuffer);
    if (mappedBuffer == NULL)
        return;

    char* writeCursor = mappedBuffer;

    // Copy the projection matrix at the beginning of the buffer
    {
        float translate = -0.5f * 2.f;
        const float L = 0.f;
        const float R = ImGui::GetIO().DisplaySize.x;
        const float B = ImGui::GetIO().DisplaySize.y;
        const float T = 0.f;
        const float mvp[4][4] = 
        {
            { 2.0f/(R-L),   0.0f,           0.0f,       0.0f},
            { 0.0f,         2.0f/(T-B),     0.0f,       0.0f,},
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
        };

        memcpy(writeCursor, &mvp[0], sizeof(mvp));
        writeCursor += sizeof(mvp);
    }

    // Copy the vertices and indices for each command list
    for (int n = 0; n < _draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = _draw_data->CmdLists[n];
        size_t verticesCount = cmd_list->VtxBuffer.size();
        size_t indicesCount  = cmd_list->IdxBuffer.size();
        size_t verticesSize = verticesCount * sizeof(ImDrawVert);
        size_t indicesSize = indicesCount * sizeof(ImDrawIdx);

        // Copy the vertex data
        memcpy(writeCursor, &cmd_list->VtxBuffer[0], verticesSize);
        writeCursor += verticesSize;

        // Copy the index data
        memcpy(writeCursor, &cmd_list->IdxBuffer[0], indicesSize);
        writeCursor += indicesSize;
    }

    ID3D12GraphicsCommandList* commandList = g_commandList;

    D3D12_VIEWPORT viewport;
    viewport.Width = ImGui::GetIO().DisplaySize.x;
    viewport.Height = ImGui::GetIO().DisplaySize.y;
    viewport.MinDepth = 0.f;
    viewport.MaxDepth = 1.f;
    viewport.TopLeftX = 0.f;
    viewport.TopLeftY = 0.f;

    commandList->RSSetViewports(1, &viewport);
    commandList->OMSetRenderTargets(1, &g_renderTarget, FALSE, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetPipelineState(g_pipelineState);

    D3D12_GPU_VIRTUAL_ADDRESS bufferAddress = g_uploadBuffer->GetGPUVirtualAddress();
    commandList->SetGraphicsRootConstantBufferView(1, bufferAddress);

    uint64_t readCursor = 64; // Our constant buffer takes 64 bytes - one mat4x4

    for (int n = 0; n < _draw_data->CmdListsCount; n++)
    {
        // Render command lists
        int vtx_offset = 0;
        int idx_offset = 0;

        const ImDrawList* cmd_list = _draw_data->CmdLists[n];
        size_t verticesCount = cmd_list->VtxBuffer.size();
        size_t indicesCount  = cmd_list->IdxBuffer.size();
        size_t verticesSize = verticesCount * sizeof(ImDrawVert);
        size_t indicesSize = indicesCount * sizeof(ImDrawIdx);

        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
        vertexBufferView.BufferLocation = bufferAddress + readCursor;
        vertexBufferView.StrideInBytes = sizeof(ImDrawVert);
        vertexBufferView.SizeInBytes = verticesSize;
        readCursor += verticesSize;

        D3D12_INDEX_BUFFER_VIEW indexBufferView;
        indexBufferView.BufferLocation = bufferAddress + readCursor;
        indexBufferView.SizeInBytes = indicesSize;
        indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        readCursor += indicesSize;

        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        commandList->IASetIndexBuffer(&indexBufferView);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                const D3D12_RECT r = { (LONG)pcmd->ClipRect.x, (LONG)pcmd->ClipRect.y, (LONG)pcmd->ClipRect.z, (LONG)pcmd->ClipRect.w };
                commandList->RSSetScissorRects(1, &r);
                commandList->DrawIndexedInstanced(pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
            }
            idx_offset += pcmd->ElemCount;
        }
        vtx_offset += verticesCount;
    }
}


void ImGui_ImplDX12_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    RECT rect;
    GetClientRect(g_hwnd, &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    // Setup time step
    INT64 current_time;
    QueryPerformanceCounter((LARGE_INTEGER *)&current_time); 
    io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
    g_Time = current_time;

    // Read keyboard modifiers inputs
    io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    // io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
    // io.MousePos : filled by WM_MOUSEMOVE events
    // io.MouseDown : filled by WM_*BUTTON* events
    // io.MouseWheel : filled by WM_MOUSEWHEEL events

    // Hide OS mouse cursor if ImGui is drawing it
    SetCursor(io.MouseDrawCursor ? NULL : LoadCursor(NULL, IDC_ARROW));

    // Start the frame
    ImGui::NewFrame();
}

bool ImGui_ImplDX12_Init(void* _hwnd)
{
	ImGuiIO& io = ImGui::GetIO();
	io.KeyMap[ImGuiKey_Tab] = VK_TAB;                              // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
	io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	io.KeyMap[ImGuiKey_Home] = VK_HOME;
	io.KeyMap[ImGuiKey_End] = VK_END;
	io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';

	io.RenderDrawListsFn = ImGui_ImplDX12_RenderDrawLists;
	io.ImeWindowHandle = _hwnd;

	g_hwnd = (HWND)_hwnd;

	if (!QueryPerformanceFrequency((LARGE_INTEGER *)&g_TicksPerSecond))
		return false;
	if (!QueryPerformanceCounter((LARGE_INTEGER *)&g_Time))
		return false;

	return true;
}