// dear imgui: Renderer Backend for DirectX10
// This needs to be used along with a Platform Backend (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'ID3D10ShaderResourceView*' as texture identifier. Read the FAQ about ImTextureID/ImTextureRef!
//  [X] Renderer: Large meshes support (64k+ vertices) even with 16-bit indices (ImGuiBackendFlags_RendererHasVtxOffset).
//  [X] Renderer: Texture updates support for dynamic font atlas (ImGuiBackendFlags_RendererHasTextures).

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2026-01-19: DirectX11: Added 'SamplerNearest' in ImGui_ImplDX11_RenderState. Renamed 'SamplerDefault' to 'SamplerLinear'. 
//  2025-09-18: Call platform_io.ClearRendererHandlers() on shutdown.
//  2025-06-11: DirectX10: Added support for ImGuiBackendFlags_RendererHasTextures, for dynamic font atlas.
//  2025-05-07: DirectX10: Honor draw_data->FramebufferScale to allow for custom backends and experiment using it (consistently with other renderer backends, even though in normal condition it is not set under Windows).
//  2025-01-06: DirectX10: Expose selected render state in ImGui_ImplDX10_RenderState, which you can access in 'void* platform_io.Renderer_RenderState' during draw callbacks.
//  2024-10-07: DirectX10: Changed default texture sampler to Clamp instead of Repeat/Wrap.
//  2022-10-11: Using 'nullptr' instead of 'NULL' as per our switch to C++11.
//  2021-06-29: Reorganized backend to pull data from a single structure to facilitate usage with multiple-contexts (all g_XXXX access changed to bd->XXXX).
//  2021-05-19: DirectX10: Replaced direct access to ImDrawCmd::TextureId with a call to ImDrawCmd::GetTexID(). (will become a requirement)
//  2021-02-18: DirectX10: Change blending equation to preserve alpha in output buffer.
//  2019-07-21: DirectX10: Backup, clear and restore Geometry Shader is any is bound when calling ImGui_ImplDX10_RenderDrawData().
//  2019-05-29: DirectX10: Added support for large mesh (64K+ vertices), enable ImGuiBackendFlags_RendererHasVtxOffset flag.
//  2019-04-30: DirectX10: Added support for special ImDrawCallback_ResetRenderState callback to reset render state.
//  2018-12-03: Misc: Added #pragma comment statement to automatically link with d3dcompiler.lib when using D3DCompile().
//  2018-11-30: Misc: Setting up io.BackendRendererName so it can be displayed in the About Window.
//  2018-07-13: DirectX10: Fixed unreleased resources in Init and Shutdown functions.
//  2018-06-08: Misc: Extracted imgui_impl_dx10.cpp/.h away from the old combined DX10+Win32 example.
//  2018-06-08: DirectX10: Use draw_data->DisplayPos and draw_data->DisplaySize to setup projection matrix and clipping rectangle.
//  2018-04-09: Misc: Fixed erroneous call to io.Fonts->ClearInputData() + ClearTexData() that was left in DX10 example but removed in 1.47 (Nov 2015) on other backends.
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback and exposed ImGui_ImplDX10_RenderDrawData() in the .h file so you can call it yourself.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2016-05-07: DirectX10: Disabling depth-write.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_dx10.h"

// DirectX
#include <stdio.h>
#include <d3d10_1.h>
#include <d3d10.h>
#include <d3dcompiler.h>
#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning: implicit conversion changes signedness
#endif

// DirectX10 data
struct ImGui_ImplDX10_Texture
{
    ID3D10Texture2D*            pTexture;
    ID3D10ShaderResourceView*   pTextureView;
};

struct ImGui_ImplDX10_Data
{
    ID3D10Device*               pd3dDevice;
    IDXGIFactory*               pFactory;
    ID3D10Buffer*               pVB;
    ID3D10Buffer*               pIB;
    ID3D10VertexShader*         pVertexShader;
    ID3D10InputLayout*          pInputLayout;
    ID3D10Buffer*               pVertexConstantBuffer;
    ID3D10PixelShader*          pPixelShader;
    ID3D10SamplerState*         pTexSamplerLinear;
    ID3D10SamplerState*         pTexSamplerNearest;
    ID3D10RasterizerState*      pRasterizerState;
    ID3D10BlendState*           pBlendState;
    ID3D10DepthStencilState*    pDepthStencilState;
    int                         VertexBufferSize;
    int                         IndexBufferSize;

    ImGui_ImplDX10_Data()       { memset((void*)this, 0, sizeof(*this)); VertexBufferSize = 5000; IndexBufferSize = 10000; }
};

struct VERTEX_CONSTANT_BUFFER_DX10
{
    float   mvp[4][4];
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplDX10_Data* ImGui_ImplDX10_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplDX10_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

// Functions
static void ImGui_ImplDX10_SetupRenderState(ImDrawData* draw_data, ID3D10Device* device)
{
    ImGui_ImplDX10_Data* bd = ImGui_ImplDX10_GetBackendData();

    // Setup viewport
    D3D10_VIEWPORT vp = {};
    vp.Width = (UINT)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    vp.Height = (UINT)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0;
    device->RSSetViewports(1, &vp);

    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    void* mapped_resource;
    if (bd->pVertexConstantBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, &mapped_resource) == S_OK)
    {
        VERTEX_CONSTANT_BUFFER_DX10* constant_buffer = (VERTEX_CONSTANT_BUFFER_DX10*)mapped_resource;
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
        memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
        bd->pVertexConstantBuffer->Unmap();
    }

    // Setup shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    device->IASetInputLayout(bd->pInputLayout);
    device->IASetVertexBuffers(0, 1, &bd->pVB, &stride, &offset);
    device->IASetIndexBuffer(bd->pIB, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device->VSSetShader(bd->pVertexShader);
    device->VSSetConstantBuffers(0, 1, &bd->pVertexConstantBuffer);
    device->PSSetShader(bd->pPixelShader);
    device->PSSetSamplers(0, 1, &bd->pTexSamplerLinear);
    device->GSSetShader(nullptr);

    // Setup render state
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    device->OMSetBlendState(bd->pBlendState, blend_factor, 0xffffffff);
    device->OMSetDepthStencilState(bd->pDepthStencilState, 0);
    device->RSSetState(bd->pRasterizerState);
}

// Render function
void ImGui_ImplDX10_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    ImGui_ImplDX10_Data* bd = ImGui_ImplDX10_GetBackendData();
    ID3D10Device* device = bd->pd3dDevice;

    // Catch up with texture updates. Most of the times, the list will have 1 element with an OK status, aka nothing to do.
    // (This almost always points to ImGui::GetPlatformIO().Textures[] but is part of ImDrawData to allow overriding or disabling texture updates).
    if (draw_data->Textures != nullptr)
        for (ImTextureData* tex : *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_ImplDX10_UpdateTexture(tex);

    // Create and grow vertex/index buffers if needed
    if (!bd->pVB || bd->VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (bd->pVB) { bd->pVB->Release(); bd->pVB = nullptr; }
        bd->VertexBufferSize = draw_data->TotalVtxCount + 5000;
        D3D10_BUFFER_DESC desc = {};
        desc.Usage = D3D10_USAGE_DYNAMIC;
        desc.ByteWidth = bd->VertexBufferSize * sizeof(ImDrawVert);
        desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        if (device->CreateBuffer(&desc, nullptr, &bd->pVB) < 0)
            return;
    }

    if (!bd->pIB || bd->IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (bd->pIB) { bd->pIB->Release(); bd->pIB = nullptr; }
        bd->IndexBufferSize = draw_data->TotalIdxCount + 10000;
        D3D10_BUFFER_DESC desc = {};
        desc.Usage = D3D10_USAGE_DYNAMIC;
        desc.ByteWidth = bd->IndexBufferSize * sizeof(ImDrawIdx);
        desc.BindFlags = D3D10_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
        if (device->CreateBuffer(&desc, nullptr, &bd->pIB) < 0)
            return;
    }

    // Copy and convert all vertices into a single contiguous buffer
    ImDrawVert* vtx_dst = nullptr;
    ImDrawIdx* idx_dst = nullptr;
    bd->pVB->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&vtx_dst);
    bd->pIB->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&idx_dst);
    for (const ImDrawList* draw_list : draw_data->CmdLists)
    {
        memcpy(vtx_dst, draw_list->VtxBuffer.Data, draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, draw_list->IdxBuffer.Data, draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += draw_list->VtxBuffer.Size;
        idx_dst += draw_list->IdxBuffer.Size;
    }
    bd->pVB->Unmap();
    bd->pIB->Unmap();

    // Backup DX state that will be modified to restore it afterwards (unfortunately this is very ugly looking and verbose. Close your eyes!)
    struct BACKUP_DX10_STATE
    {
        UINT                        ScissorRectsCount, ViewportsCount;
        D3D10_RECT                  ScissorRects[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        D3D10_VIEWPORT              Viewports[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        ID3D10RasterizerState*      RS;
        ID3D10BlendState*           BlendState;
        FLOAT                       BlendFactor[4];
        UINT                        SampleMask;
        UINT                        StencilRef;
        ID3D10DepthStencilState*    DepthStencilState;
        ID3D10ShaderResourceView*   PSShaderResource;
        ID3D10SamplerState*         PSSampler;
        ID3D10PixelShader*          PS;
        ID3D10VertexShader*         VS;
        ID3D10GeometryShader*       GS;
        D3D10_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
        ID3D10Buffer*               IndexBuffer, *VertexBuffer, *VSConstantBuffer;
        UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
        DXGI_FORMAT                 IndexBufferFormat;
        ID3D10InputLayout*          InputLayout;
    };
    BACKUP_DX10_STATE old = {};
    old.ScissorRectsCount = old.ViewportsCount = D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    device->RSGetScissorRects(&old.ScissorRectsCount, old.ScissorRects);
    device->RSGetViewports(&old.ViewportsCount, old.Viewports);
    device->RSGetState(&old.RS);
    device->OMGetBlendState(&old.BlendState, old.BlendFactor, &old.SampleMask);
    device->OMGetDepthStencilState(&old.DepthStencilState, &old.StencilRef);
    device->PSGetShaderResources(0, 1, &old.PSShaderResource);
    device->PSGetSamplers(0, 1, &old.PSSampler);
    device->PSGetShader(&old.PS);
    device->VSGetShader(&old.VS);
    device->VSGetConstantBuffers(0, 1, &old.VSConstantBuffer);
    device->GSGetShader(&old.GS);
    device->IAGetPrimitiveTopology(&old.PrimitiveTopology);
    device->IAGetIndexBuffer(&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
    device->IAGetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
    device->IAGetInputLayout(&old.InputLayout);

    // Setup desired DX state
    ImGui_ImplDX10_SetupRenderState(draw_data, device);
    // Setup render state structure (for callbacks and custom texture bindings)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    ImGui_ImplDX10_RenderState render_state;
    render_state.Device = bd->pd3dDevice;
    render_state.SamplerLinear = bd->pTexSamplerLinear;
    render_state.SamplerNearest = bd->pTexSamplerNearest;
    render_state.VertexConstantBuffer = bd->pVertexConstantBuffer;
    platform_io.Renderer_RenderState = &render_state;

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    ImVec2 clip_scale = draw_data->FramebufferScale;
    for (const ImDrawList* draw_list : draw_data->CmdLists)
    {
        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplDX10_SetupRenderState(draw_data, device);
                else
                    pcmd->UserCallback(draw_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle
                const D3D10_RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
                device->RSSetScissorRects(1, &r);

                // Bind texture, Draw
                ID3D10ShaderResourceView* texture_srv = (ID3D10ShaderResourceView*)pcmd->GetTexID();
                device->PSSetShaderResources(0, 1, &texture_srv);
                device->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
            }
        }
        global_idx_offset += draw_list->IdxBuffer.Size;
        global_vtx_offset += draw_list->VtxBuffer.Size;
    }
    platform_io.Renderer_RenderState = nullptr;

    // Restore modified DX state
    device->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
    device->RSSetViewports(old.ViewportsCount, old.Viewports);
    device->RSSetState(old.RS); if (old.RS) old.RS->Release();
    device->OMSetBlendState(old.BlendState, old.BlendFactor, old.SampleMask); if (old.BlendState) old.BlendState->Release();
    device->OMSetDepthStencilState(old.DepthStencilState, old.StencilRef); if (old.DepthStencilState) old.DepthStencilState->Release();
    device->PSSetShaderResources(0, 1, &old.PSShaderResource); if (old.PSShaderResource) old.PSShaderResource->Release();
    device->PSSetSamplers(0, 1, &old.PSSampler); if (old.PSSampler) old.PSSampler->Release();
    device->PSSetShader(old.PS); if (old.PS) old.PS->Release();
    device->VSSetShader(old.VS); if (old.VS) old.VS->Release();
    device->GSSetShader(old.GS); if (old.GS) old.GS->Release();
    device->VSSetConstantBuffers(0, 1, &old.VSConstantBuffer); if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
    device->IASetPrimitiveTopology(old.PrimitiveTopology);
    device->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset); if (old.IndexBuffer) old.IndexBuffer->Release();
    device->IASetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset); if (old.VertexBuffer) old.VertexBuffer->Release();
    device->IASetInputLayout(old.InputLayout); if (old.InputLayout) old.InputLayout->Release();
}

static void ImGui_ImplDX10_DestroyTexture(ImTextureData* tex)
{
    if (ImGui_ImplDX10_Texture* backend_tex = (ImGui_ImplDX10_Texture*)tex->BackendUserData)
    {
        IM_ASSERT(backend_tex->pTextureView == (ID3D10ShaderResourceView*)(intptr_t)tex->TexID);
        backend_tex->pTextureView->Release();
        backend_tex->pTexture->Release();
        IM_DELETE(backend_tex);

        // Clear identifiers and mark as destroyed (in order to allow e.g. calling InvalidateDeviceObjects while running)
        tex->SetTexID(ImTextureID_Invalid);
        tex->BackendUserData = nullptr;
    }
    tex->SetStatus(ImTextureStatus_Destroyed);
}

void ImGui_ImplDX10_UpdateTexture(ImTextureData* tex)
{
    ImGui_ImplDX10_Data* bd = ImGui_ImplDX10_GetBackendData();
    if (tex->Status == ImTextureStatus_WantCreate)
    {
        // Create and upload new texture to graphics system
        //IMGUI_DEBUG_LOG("UpdateTexture #%03d: WantCreate %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
        IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);
        unsigned int* pixels = (unsigned int*)tex->GetPixels();
        ImGui_ImplDX10_Texture* backend_tex = IM_NEW(ImGui_ImplDX10_Texture)();

        // Create texture
        D3D10_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = (UINT)tex->Width;
        desc.Height = (UINT)tex->Height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D10_USAGE_DEFAULT;
        desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        D3D10_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = pixels;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        bd->pd3dDevice->CreateTexture2D(&desc, &subResource, &backend_tex->pTexture);
        IM_ASSERT(backend_tex->pTexture != nullptr && "Backend failed to create texture!");

        // Create texture view
        D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
        ZeroMemory(&srv_desc, sizeof(srv_desc));
        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srv_desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = desc.MipLevels;
        srv_desc.Texture2D.MostDetailedMip = 0;
        bd->pd3dDevice->CreateShaderResourceView(backend_tex->pTexture, &srv_desc, &backend_tex->pTextureView);
        IM_ASSERT(backend_tex->pTextureView != nullptr && "Backend failed to create texture!");

        // Store identifiers
        tex->SetTexID((ImTextureID)(intptr_t)backend_tex->pTextureView);
        tex->SetStatus(ImTextureStatus_OK);
        tex->BackendUserData = backend_tex;
    }
    else if (tex->Status == ImTextureStatus_WantUpdates)
    {
        // Update selected blocks. We only ever write to textures regions which have never been used before!
        // This backend choose to use tex->Updates[] but you can use tex->UpdateRect to upload a single region.
        ImGui_ImplDX10_Texture* backend_tex = (ImGui_ImplDX10_Texture*)tex->BackendUserData;
        IM_ASSERT(backend_tex->pTextureView == (ID3D10ShaderResourceView*)(intptr_t)tex->TexID);
        for (ImTextureRect& r : tex->Updates)
        {
            D3D10_BOX box = { (UINT)r.x, (UINT)r.y, (UINT)0, (UINT)(r.x + r.w), (UINT)(r.y + r.h), (UINT)1 };
            bd->pd3dDevice->UpdateSubresource(backend_tex->pTexture, 0, &box, tex->GetPixelsAt(r.x, r.y), (UINT)tex->GetPitch(), 0);
        }
        tex->SetStatus(ImTextureStatus_OK);
    }
    if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames > 0)
        ImGui_ImplDX10_DestroyTexture(tex);
}

bool    ImGui_ImplDX10_CreateDeviceObjects()
{
    ImGui_ImplDX10_Data* bd = ImGui_ImplDX10_GetBackendData();
    if (!bd->pd3dDevice)
        return false;
    ImGui_ImplDX10_InvalidateDeviceObjects();

    // By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
    // If you would like to use this DX10 sample code but remove this dependency you can:
    //  1) compile once, save the compiled shader blobs into a file or source code and pass them to CreateVertexShader()/CreatePixelShader() [preferred solution]
    //  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL.
    // See https://github.com/ocornut/imgui/pull/638 for sources and details.

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

        ID3DBlob* vertexShaderBlob;
        if (FAILED(D3DCompile(vertexShader, strlen(vertexShader), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vertexShaderBlob, nullptr)))
            return false; // NB: Pass ID3DBlob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
        if (bd->pd3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &bd->pVertexShader) != S_OK)
        {
            vertexShaderBlob->Release();
            return false;
        }

        // Create the input layout
        D3D10_INPUT_ELEMENT_DESC local_layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)offsetof(ImDrawVert, pos), D3D10_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)offsetof(ImDrawVert, uv),  D3D10_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)offsetof(ImDrawVert, col), D3D10_INPUT_PER_VERTEX_DATA, 0 },
        };
        if (bd->pd3dDevice->CreateInputLayout(local_layout, 3, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &bd->pInputLayout) != S_OK)
        {
            vertexShaderBlob->Release();
            return false;
        }
        vertexShaderBlob->Release();

        // Create the constant buffer
        {
            D3D10_BUFFER_DESC desc = {};
            desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER_DX10);
            desc.Usage = D3D10_USAGE_DYNAMIC;
            desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;
            bd->pd3dDevice->CreateBuffer(&desc, nullptr, &bd->pVertexConstantBuffer);
        }
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
            sampler sampler0;\
            Texture2D texture0;\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
            float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
            return out_col; \
            }";

        ID3DBlob* pixelShaderBlob;
        if (FAILED(D3DCompile(pixelShader, strlen(pixelShader), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &pixelShaderBlob, nullptr)))
            return false; // NB: Pass ID3DBlob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
        if (bd->pd3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), &bd->pPixelShader) != S_OK)
        {
            pixelShaderBlob->Release();
            return false;
        }
        pixelShaderBlob->Release();
    }

    // Create the blending setup
    {
        D3D10_BLEND_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.AlphaToCoverageEnable = false;
        desc.BlendEnable[0] = true;
        desc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
        desc.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
        desc.BlendOp = D3D10_BLEND_OP_ADD;
        desc.SrcBlendAlpha = D3D10_BLEND_ONE;
        desc.DestBlendAlpha = D3D10_BLEND_INV_SRC_ALPHA;
        desc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
        desc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
        bd->pd3dDevice->CreateBlendState(&desc, &bd->pBlendState);
    }

    // Create the rasterizer state
    {
        D3D10_RASTERIZER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.FillMode = D3D10_FILL_SOLID;
        desc.CullMode = D3D10_CULL_NONE;
        desc.ScissorEnable = true;
        desc.DepthClipEnable = true;
        bd->pd3dDevice->CreateRasterizerState(&desc, &bd->pRasterizerState);
    }

    // Create depth-stencil State
    {
        D3D10_DEPTH_STENCIL_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D10_COMPARISON_ALWAYS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
        desc.BackFace = desc.FrontFace;
        bd->pd3dDevice->CreateDepthStencilState(&desc, &bd->pDepthStencilState);
    }

    // Create texture sampler
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
    {
        D3D10_SAMPLER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
        desc.MipLODBias = 0.f;
        desc.ComparisonFunc = D3D10_COMPARISON_ALWAYS;
        desc.MinLOD = 0.f;
        desc.MaxLOD = 0.f;
        bd->pd3dDevice->CreateSamplerState(&desc, &bd->pTexSamplerLinear);
        desc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
        bd->pd3dDevice->CreateSamplerState(&desc, &bd->pTexSamplerNearest);
    }

    return true;
}

void    ImGui_ImplDX10_InvalidateDeviceObjects()
{
    ImGui_ImplDX10_Data* bd = ImGui_ImplDX10_GetBackendData();
    if (!bd->pd3dDevice)
        return;

    // Destroy all textures
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
        if (tex->RefCount == 1)
            ImGui_ImplDX10_DestroyTexture(tex);
    if (bd->pTexSamplerLinear)      { bd->pTexSamplerLinear->Release(); bd->pTexSamplerLinear = nullptr; }
    if (bd->pTexSamplerNearest)     { bd->pTexSamplerNearest->Release(); bd->pTexSamplerNearest = nullptr; }
    if (bd->pIB)                    { bd->pIB->Release(); bd->pIB = nullptr; }
    if (bd->pVB)                    { bd->pVB->Release(); bd->pVB = nullptr; }
    if (bd->pBlendState)            { bd->pBlendState->Release(); bd->pBlendState = nullptr; }
    if (bd->pDepthStencilState)     { bd->pDepthStencilState->Release(); bd->pDepthStencilState = nullptr; }
    if (bd->pRasterizerState)       { bd->pRasterizerState->Release(); bd->pRasterizerState = nullptr; }
    if (bd->pPixelShader)           { bd->pPixelShader->Release(); bd->pPixelShader = nullptr; }
    if (bd->pVertexConstantBuffer)  { bd->pVertexConstantBuffer->Release(); bd->pVertexConstantBuffer = nullptr; }
    if (bd->pInputLayout)           { bd->pInputLayout->Release(); bd->pInputLayout = nullptr; }
    if (bd->pVertexShader)          { bd->pVertexShader->Release(); bd->pVertexShader = nullptr; }
}

bool    ImGui_ImplDX10_Init(ID3D10Device* device)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplDX10_Data* bd = IM_NEW(ImGui_ImplDX10_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_dx10";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;   // We can honor ImGuiPlatformIO::Textures[] requests during render.

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_TextureMaxWidth = platform_io.Renderer_TextureMaxHeight = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;

    // Get factory from device
    IDXGIDevice* pDXGIDevice = nullptr;
    IDXGIAdapter* pDXGIAdapter = nullptr;
    IDXGIFactory* pFactory = nullptr;
    if (device->QueryInterface(IID_PPV_ARGS(&pDXGIDevice)) == S_OK)
        if (pDXGIDevice->GetParent(IID_PPV_ARGS(&pDXGIAdapter)) == S_OK)
            if (pDXGIAdapter->GetParent(IID_PPV_ARGS(&pFactory)) == S_OK)
            {
                bd->pd3dDevice = device;
                bd->pFactory = pFactory;
            }
    if (pDXGIDevice) pDXGIDevice->Release();
    if (pDXGIAdapter) pDXGIAdapter->Release();
    bd->pd3dDevice->AddRef();

    return true;
}

void ImGui_ImplDX10_Shutdown()
{
    ImGui_ImplDX10_Data* bd = ImGui_ImplDX10_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    ImGui_ImplDX10_InvalidateDeviceObjects();
    if (bd->pFactory) { bd->pFactory->Release(); }
    if (bd->pd3dDevice) { bd->pd3dDevice->Release(); }

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
    platform_io.ClearRendererHandlers();
    IM_DELETE(bd);
}

void ImGui_ImplDX10_NewFrame()
{
    ImGui_ImplDX10_Data* bd = ImGui_ImplDX10_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplDX10_Init()?");

    if (!bd->pVertexShader)
        if (!ImGui_ImplDX10_CreateDeviceObjects())
            IM_ASSERT(0 && "ImGui_ImplDX10_CreateDeviceObjects() failed!");
}

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
