// dear imgui: Renderer Backend for DirectX9
// This needs to be used along with a Platform Backend (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'LPDIRECT3DTEXTURE9' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Renderer: Large meshes support (64k+ vertices) with 16-bit indices.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2024-02-12: DirectX9: Using RGBA format when supported by the driver to avoid CPU side conversion. (#6575)
//  2022-10-11: Using 'nullptr' instead of 'NULL' as per our switch to C++11.
//  2021-06-29: Reorganized backend to pull data from a single structure to facilitate usage with multiple-contexts (all g_XXXX access changed to bd->XXXX).
//  2021-06-25: DirectX9: Explicitly disable texture state stages after >= 1.
//  2021-05-19: DirectX9: Replaced direct access to ImDrawCmd::TextureId with a call to ImDrawCmd::GetTexID(). (will become a requirement)
//  2021-04-23: DirectX9: Explicitly setting up more graphics states to increase compatibility with unusual non-default states.
//  2021-03-18: DirectX9: Calling IDirect3DStateBlock9::Capture() after CreateStateBlock() as a workaround for state restoring issues (see #3857).
//  2021-03-03: DirectX9: Added support for IMGUI_USE_BGRA_PACKED_COLOR in user's imconfig file.
//  2021-02-18: DirectX9: Change blending equation to preserve alpha in output buffer.
//  2019-05-29: DirectX9: Added support for large mesh (64K+ vertices), enable ImGuiBackendFlags_RendererHasVtxOffset flag.
//  2019-04-30: DirectX9: Added support for special ImDrawCallback_ResetRenderState callback to reset render state.
//  2019-03-29: Misc: Fixed erroneous assert in ImGui_ImplDX9_InvalidateDeviceObjects().
//  2019-01-16: Misc: Disabled fog before drawing UI's. Fixes issue #2288.
//  2018-11-30: Misc: Setting up io.BackendRendererName so it can be displayed in the About Window.
//  2018-06-08: Misc: Extracted imgui_impl_dx9.cpp/.h away from the old combined DX9+Win32 example.
//  2018-06-08: DirectX9: Use draw_data->DisplayPos and draw_data->DisplaySize to setup projection matrix and clipping rectangle.
//  2018-05-07: Render: Saving/restoring Transform because they don't seem to be included in the StateBlock. Setting shading mode to Gouraud.
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback and exposed ImGui_ImplDX9_RenderDrawData() in the .h file so you can call it yourself.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_dx9.h"

// DirectX
#include <d3d9.h>

// DirectX data
struct ImGui_ImplDX9_Data
{
    LPDIRECT3DDEVICE9           pd3dDevice;
    LPDIRECT3DVERTEXBUFFER9     pVB;
    LPDIRECT3DINDEXBUFFER9      pIB;
    LPDIRECT3DTEXTURE9          FontTexture;
    int                         VertexBufferSize;
    int                         IndexBufferSize;

    // Direct3D 9 programmable rendering pipeline implementation.
    // Nearly all graphic card released after 2007 support Shader Model 2.
    // So theoretically, this feature is supported on almost all current devices.
    bool                         IsShaderSupported;
    IDirect3DVertexDeclaration9* pInputLayout;
    IDirect3DVertexShader9*      pVertexShader;
    IDirect3DPixelShader9*       pPixelShader;

    ImGui_ImplDX9_Data()        { memset((void*)this, 0, sizeof(*this)); VertexBufferSize = 5000; IndexBufferSize = 10000; }
};

struct CUSTOMVERTEX
{
    float    pos[3];
    D3DCOLOR col;
    float    uv[2];
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

#ifdef IMGUI_USE_BGRA_PACKED_COLOR
#define IMGUI_COL_TO_DX9_ARGB(_COL)     (_COL)
#else
#define IMGUI_COL_TO_DX9_ARGB(_COL)     (((_COL) & 0xFF00FF00) | (((_COL) & 0xFF0000) >> 16) | (((_COL) & 0xFF) << 16))
#endif

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplDX9_Data* ImGui_ImplDX9_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplDX9_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

// Shared
static D3DMATRIX ImGui_ImplDX9_BuildProjectionMatrix(ImDrawData* draw_data)
{
    // Orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    // Being agnostic of whether <d3dx9.h> or <DirectXMath.h> can be used, we aren't relying on D3DXMatrixIdentity()/D3DXMatrixOrthoOffCenterLH() or DirectX::XMMatrixIdentity()/DirectX::XMMatrixOrthographicOffCenterLH()
    float L = draw_data->DisplayPos.x + 0.5f;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x + 0.5f;
    float T = draw_data->DisplayPos.y + 0.5f;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y + 0.5f;
    const D3DMATRIX mat_projection =
    { { {
        2.0f/(R-L),   0.0f,         0.0f,  0.0f,
        0.0f,         2.0f/(T-B),   0.0f,  0.0f,
        0.0f,         0.0f,         0.5f,  0.0f,
        (L+R)/(L-R),  (T+B)/(B-T),  0.5f,  1.0f,
    } } };
    return mat_projection;
}

static void ImGui_ImplDX9_ResetRenderState(ImDrawData* draw_data);

static void ImGui_ImplDX9_RenderCommandLists(ImDrawData* draw_data)
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();

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
                    ImGui_ImplDX9_ResetRenderState(draw_data);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
                ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
                if (clip_max.x < clip_min.x || clip_max.y < clip_min.y)
                    continue;

                // Apply Scissor/clipping rectangle, Bind texture, Draw
                const RECT rect = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
                bd->pd3dDevice->SetScissorRect(&rect);
                bd->pd3dDevice->SetTexture(0, (IDirect3DTexture9*)pcmd->GetTexID());
                bd->pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, pcmd->VtxOffset + global_vtx_offset, 0, (UINT)cmd_list->VtxBuffer.Size, pcmd->IdxOffset + global_idx_offset, pcmd->ElemCount / 3);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }
}

// Programmable render pipeline
static bool ImGui_ImplDX9WithShader_CreateDeviceObjects()
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    
    D3DCAPS9 caps; ZeroMemory(&caps, sizeof(D3DCAPS9));
    if (D3D_OK != bd->pd3dDevice->GetDeviceCaps(&caps))
        return false;
    if (caps.VertexShaderVersion < D3DVS_VERSION(2,0) || caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
        return false;
    
    // Input layout of default ImDrawVert
    static const D3DVERTEXELEMENT9 InputLayout[4] = {
        { 0,  0, D3DDECLTYPE_FLOAT2  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0,  8, D3DDECLTYPE_FLOAT2  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR   , 0 },
        D3DDECL_END(),
    };
    if (D3D_OK != bd->pd3dDevice->CreateVertexDeclaration(InputLayout, &bd->pInputLayout))
        return false;
    
    // Shaders
    /*
        float4x4 mvp : register(c0);
        struct VS_Input
        {
            float2 pos : POSITION0;
            float2 uv  : TEXCOORD0;
            float4 col : COLOR0;
        };
        struct VS_Output
        {
            float4 pos : POSITION;
            float2 uv  : TEXCOORD0;
            float4 col : COLOR0;
        };
        VS_Output main(VS_Input input)
        {
            VS_Output output;
            output.pos = mul(mvp, float4(input.pos.x, input.pos.y, 0.0f, 1.0f));
            output.uv = input.uv;
            output.col = input.col;
            return output;
        };
    */
    // Precompile with Visual Studio HLSL compiler, vs_2_0
    static const BYTE VertexShaderByteCode[244] = { 0, 2, 254, 255, 254, 255, 30, 0, 67, 84, 65, 66, 28, 0, 0, 0, 75, 0, 0, 0, 0, 2, 254, 255, 1, 0, 0, 0, 28, 0, 0, 0, 0, 1, 0, 0, 68, 0, 0, 0, 48, 0, 0, 0, 2, 0, 0, 0, 4, 0, 2, 0, 52, 0, 0, 0, 0, 0, 0, 0, 109, 118, 112, 0, 3, 0, 3, 0, 4, 0, 4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 118, 115, 95, 50, 95, 48, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 171, 31, 0, 0, 2, 0, 0, 0, 128, 0, 0, 15, 144, 31, 0, 0, 2, 5, 0, 0, 128, 1, 0, 15, 144, 31, 0, 0, 2, 10, 0, 0, 128, 2, 0, 15, 144, 5, 0, 0, 3, 0, 0, 15, 128, 0, 0, 85, 144, 1, 0, 228, 160, 4, 0, 0, 4, 0, 0, 15, 128, 0, 0, 228, 160, 0, 0, 0, 144, 0, 0, 228, 128, 2, 0, 0, 3, 0, 0, 15, 192, 0, 0, 228, 128, 3, 0, 228, 160, 1, 0, 0, 2, 0, 0, 3, 224, 1, 0, 228, 144, 1, 0, 0, 2, 0, 0, 15, 208, 2, 0, 228, 144, 255, 255, 0, 0 };
    /*
        sampler tex0 : register(s0);
        struct PS_Input
        {
            float4 pos : VPOS;
            float2 uv  : TEXCOORD0;
            float4 col : COLOR0;
        };
        struct PS_Output
        {
            float4 col : COLOR;
        };
        PS_Output main(PS_Input input)
        {
            PS_Output output;
            output.col = input.col * tex2D(tex0, input.uv);
            return output;
        };
    */
    // Precompile with Visual Studio HLSL compiler, ps_2_0
    static const BYTE PixelShaderByteCode[216] = { 0, 2, 255, 255, 254, 255, 31, 0, 67, 84, 65, 66, 28, 0, 0, 0, 79, 0, 0, 0, 0, 2, 255, 255, 1, 0, 0, 0, 28, 0, 0, 0, 0, 1, 0, 0, 72, 0, 0, 0, 48, 0, 0, 0, 3, 0, 0, 0, 1, 0, 2, 0, 56, 0, 0, 0, 0, 0, 0, 0, 116, 101, 120, 48, 0, 171, 171, 171, 4, 0, 12, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 112, 115, 95, 50, 95, 48, 0, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 40, 82, 41, 32, 72, 76, 83, 76, 32, 83, 104, 97, 100, 101, 114, 32, 67, 111, 109, 112, 105, 108, 101, 114, 32, 49, 48, 46, 49, 0, 171, 31, 0, 0, 2, 0, 0, 0, 128, 0, 0, 3, 176, 31, 0, 0, 2, 0, 0, 0, 128, 0, 0, 15, 144, 31, 0, 0, 2, 0, 0, 0, 144, 0, 8, 15, 160, 66, 0, 0, 3, 0, 0, 15, 128, 0, 0, 228, 176, 0, 8, 228, 160, 5, 0, 0, 3, 0, 0, 15, 128, 0, 0, 228, 128, 0, 0, 228, 144, 1, 0, 0, 2, 0, 8, 15, 128, 0, 0, 228, 128, 255, 255, 0, 0 };
    if (D3D_OK != bd->pd3dDevice->CreateVertexShader((DWORD*)VertexShaderByteCode, &bd->pVertexShader))
        return false;
    if (D3D_OK != bd->pd3dDevice->CreatePixelShader((DWORD*)PixelShaderByteCode, &bd->pPixelShader))
        return false;
    
    bd->IsShaderSupported = true;
    ImGui::GetIO().BackendRendererName = "imgui_impl_dx9 (shader)";
    return true;
}

static void ImGui_ImplDX9WithShader_InvalidateDeviceObjects()
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    if (bd->pInputLayout) { bd->pInputLayout->Release(); bd->pInputLayout = nullptr; }
    if (bd->pVertexShader) { bd->pVertexShader->Release(); bd->pVertexShader = nullptr; }
    if (bd->pPixelShader) { bd->pPixelShader->Release(); bd->pPixelShader = nullptr; }
    bd->IsShaderSupported = false;
}

static bool ImGui_ImplDX9WithShader_SetRenderState(ImDrawData* draw_data)
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    if (!bd->IsShaderSupported)
        return false;
    
    IDirect3DDevice9* ctx = bd->pd3dDevice;
    D3DMATRIX mat_projection = ImGui_ImplDX9_BuildProjectionMatrix(draw_data);
    D3DVIEWPORT9 viewport = { 0, 0, (DWORD)draw_data->DisplaySize.x, (DWORD)draw_data->DisplaySize.y, 0.0f, 1.0f };
    
    // [IA Stage]
    ctx->SetVertexDeclaration(bd->pInputLayout);
    ctx->SetStreamSource(0, bd->pVB, 0, sizeof(ImDrawVert));
    ctx->SetStreamSourceFreq(0, 1); // no instantiated drawing
    ctx->SetIndices(bd->pIB);
    // [VS Stage]
    ctx->SetVertexShaderConstantF(0, (float*)&mat_projection, 4); // constant buffer: float4x4 matrix
    ctx->SetVertexShader(bd->pVertexShader);
    // [RS Stage]
    ctx->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID); // rasterizer state
    ctx->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    ctx->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    ctx->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
    ctx->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
    ctx->SetViewport(&viewport);
    // [PS Stage]
    ctx->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP); // sampler state
    ctx->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    ctx->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
    ctx->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    ctx->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    ctx->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    ctx->SetPixelShader(bd->pPixelShader);
    // [OM Stage]
    ctx->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE); // depth stencil state
    ctx->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    ctx->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); // blend state
    ctx->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    ctx->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    ctx->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    ctx->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ctx->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);
    ctx->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
    ctx->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
    ctx->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    
    // [Fixed Pipeline] disable these features, especially lighting
    ctx->SetRenderState(D3DRS_CLIPPING, FALSE);
    ctx->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    ctx->SetRenderState(D3DRS_LIGHTING, FALSE);
    ctx->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    ctx->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
    ctx->SetRenderState(D3DRS_FOGENABLE, FALSE);
    ctx->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
    ctx->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    
    return true;
}

static bool ImGui_ImplDX9WithShader_UploadBuffers(ImDrawData* draw_data)
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    
    // Create or resize buffers if needed
    const DWORD usage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
    const D3DFORMAT format = sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32;
    if (!bd->pVB || bd->VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (bd->pVB) { bd->pVB->Release(); bd->pVB = nullptr; }
        while (bd->VertexBufferSize < draw_data->TotalVtxCount) { bd->VertexBufferSize = draw_data->TotalVtxCount + 5000; }
        if (D3D_OK != bd->pd3dDevice->CreateVertexBuffer(bd->VertexBufferSize * sizeof(ImDrawVert), usage, 0, D3DPOOL_DEFAULT, &bd->pVB, nullptr))
            return false;
    }
    if (!bd->pIB || bd->IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (bd->pIB) { bd->pIB->Release(); bd->pIB = nullptr; }
        while (bd->IndexBufferSize < draw_data->TotalIdxCount) { bd->IndexBufferSize = draw_data->TotalIdxCount + 10000; }
        if (D3D_OK != bd->pd3dDevice->CreateIndexBuffer(bd->IndexBufferSize * sizeof(ImDrawIdx), usage, format, D3DPOOL_DEFAULT, &bd->pIB, nullptr))
            return false;
    }
    
    // Copy and convert all vertices into a single contiguous buffer, convert colors to DX9 default format.
    // FIXME-OPT: This is a minor waste of resource, the ideal is to:
    //  1) to avoid repacking colors:   #define IMGUI_USE_BGRA_PACKED_COLOR
    
    // Copy vertex buffer
    ImDrawVert* vtx_dst = nullptr;
    if (D3D_OK != bd->pVB->Lock(0, (UINT)(draw_data->TotalVtxCount * sizeof(ImDrawVert)), (void**)&vtx_dst, D3DLOCK_DISCARD))
        return false;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
#ifndef IMGUI_USE_BGRA_PACKED_COLOR
        ImDrawVert* vtx_src = cmd_list->VtxBuffer.Data;
        for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
        {
            vtx_dst->pos = vtx_src->pos;
            vtx_dst->uv = vtx_src->uv;
            vtx_dst->col = IMGUI_COL_TO_DX9_ARGB(vtx_src->col);
            vtx_src++;
            vtx_dst++;
        }
#else
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        vtx_dst += cmd_list->VtxBuffer.Size;
#endif
    }
    if (D3D_OK != bd->pVB->Unlock())
        return false;
    
    // Copy index buffer
    ImDrawIdx* idx_dst = nullptr;
    if (D3D_OK != bd->pIB->Lock(0, (UINT)(draw_data->TotalIdxCount * sizeof(ImDrawIdx)), (void**)&idx_dst, D3DLOCK_DISCARD))
        return false;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    if (D3D_OK != bd->pIB->Unlock())
        return false;
    
    return true;
}

static bool ImGui_ImplDX9WithShader_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return true;
    
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();

    if (!ImGui_ImplDX9WithShader_UploadBuffers(draw_data))
        return false;
    
    // Backup the DX9 state
    IDirect3DStateBlock9* d3d9_state_block = nullptr;
    if (D3D_OK != bd->pd3dDevice->CreateStateBlock(D3DSBT_ALL, &d3d9_state_block))
        return false;
    if (D3D_OK != d3d9_state_block->Capture())
    {
        d3d9_state_block->Release();
        return false;
    }
    D3DMATRIX last_float4x4;
    bd->pd3dDevice->GetVertexShaderConstantF(0, (float*)&last_float4x4, 4);
    
    // Setup desired DX state
    ImGui_ImplDX9WithShader_SetRenderState(draw_data);
    
    // Render command lists
    ImGui_ImplDX9_RenderCommandLists(draw_data);
    
    // Restore the DX9 state
    bd->pd3dDevice->SetVertexShaderConstantF(0, (float*)&last_float4x4, 4);
    d3d9_state_block->Apply();
    d3d9_state_block->Release();
    return true;
}

// Functions
static void ImGui_ImplDX9_SetupRenderState(ImDrawData* draw_data)
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();

    // Setup buffers
    bd->pd3dDevice->SetStreamSource(0, bd->pVB, 0, sizeof(CUSTOMVERTEX));
    bd->pd3dDevice->SetIndices(bd->pIB);
    bd->pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

    // Setup viewport
    D3DVIEWPORT9 vp;
    vp.X = vp.Y = 0;
    vp.Width = (DWORD)draw_data->DisplaySize.x;
    vp.Height = (DWORD)draw_data->DisplaySize.y;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    bd->pd3dDevice->SetViewport(&vp);

    // Setup render state: fixed-pipeline, alpha-blending, no face culling, no depth testing, shade mode (for gradient), bilinear sampling.
    bd->pd3dDevice->SetPixelShader(nullptr);
    bd->pd3dDevice->SetVertexShader(nullptr);
    bd->pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    bd->pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    bd->pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    bd->pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    bd->pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    bd->pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    bd->pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    bd->pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    bd->pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    bd->pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    bd->pd3dDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    bd->pd3dDevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
    bd->pd3dDevice->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
    bd->pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    bd->pd3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
    bd->pd3dDevice->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
    bd->pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    bd->pd3dDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    bd->pd3dDevice->SetRenderState(D3DRS_CLIPPING, TRUE);
    bd->pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    bd->pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    bd->pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    bd->pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    bd->pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    bd->pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    bd->pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    bd->pd3dDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    bd->pd3dDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    bd->pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    bd->pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    // Setup orthographic projection matrix
    D3DMATRIX mat_identity = { { { 1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f } } };
    D3DMATRIX mat_projection = ImGui_ImplDX9_BuildProjectionMatrix(draw_data);
    bd->pd3dDevice->SetTransform(D3DTS_WORLD, &mat_identity);
    bd->pd3dDevice->SetTransform(D3DTS_VIEW, &mat_identity);
    bd->pd3dDevice->SetTransform(D3DTS_PROJECTION, &mat_projection);
}

void ImGui_ImplDX9_ResetRenderState(ImDrawData* draw_data)
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    if (bd->IsShaderSupported)
        ImGui_ImplDX9WithShader_SetRenderState(draw_data);
    else
        ImGui_ImplDX9_SetupRenderState(draw_data);
}

// Render function.
void ImGui_ImplDX9_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    // If available, render via programmable render pipeline
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    if (bd->IsShaderSupported)
    {
        ImGui_ImplDX9WithShader_RenderDrawData(draw_data);
        return;
    }

    // Create and grow buffers if needed
    if (!bd->pVB || bd->VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (bd->pVB) { bd->pVB->Release(); bd->pVB = nullptr; }
        bd->VertexBufferSize = draw_data->TotalVtxCount + 5000;
        if (bd->pd3dDevice->CreateVertexBuffer(bd->VertexBufferSize * sizeof(CUSTOMVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &bd->pVB, nullptr) < 0)
            return;
    }
    if (!bd->pIB || bd->IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (bd->pIB) { bd->pIB->Release(); bd->pIB = nullptr; }
        bd->IndexBufferSize = draw_data->TotalIdxCount + 10000;
        if (bd->pd3dDevice->CreateIndexBuffer(bd->IndexBufferSize * sizeof(ImDrawIdx), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32, D3DPOOL_DEFAULT, &bd->pIB, nullptr) < 0)
            return;
    }

    // Backup the DX9 state
    IDirect3DStateBlock9* d3d9_state_block = nullptr;
    if (bd->pd3dDevice->CreateStateBlock(D3DSBT_ALL, &d3d9_state_block) < 0)
        return;
    if (d3d9_state_block->Capture() < 0)
    {
        d3d9_state_block->Release();
        return;
    }

    // Backup the DX9 transform (DX9 documentation suggests that it is included in the StateBlock but it doesn't appear to)
    D3DMATRIX last_world, last_view, last_projection;
    bd->pd3dDevice->GetTransform(D3DTS_WORLD, &last_world);
    bd->pd3dDevice->GetTransform(D3DTS_VIEW, &last_view);
    bd->pd3dDevice->GetTransform(D3DTS_PROJECTION, &last_projection);

    // Allocate buffers
    CUSTOMVERTEX* vtx_dst;
    ImDrawIdx* idx_dst;
    if (bd->pVB->Lock(0, (UINT)(draw_data->TotalVtxCount * sizeof(CUSTOMVERTEX)), (void**)&vtx_dst, D3DLOCK_DISCARD) < 0)
    {
        d3d9_state_block->Release();
        return;
    }
    if (bd->pIB->Lock(0, (UINT)(draw_data->TotalIdxCount * sizeof(ImDrawIdx)), (void**)&idx_dst, D3DLOCK_DISCARD) < 0)
    {
        bd->pVB->Unlock();
        d3d9_state_block->Release();
        return;
    }

    // Copy and convert all vertices into a single contiguous buffer, convert colors to DX9 default format.
    // FIXME-OPT: This is a minor waste of resource, the ideal is to use imconfig.h and
    //  1) to avoid repacking colors:   #define IMGUI_USE_BGRA_PACKED_COLOR
    //  2) to avoid repacking vertices: #define IMGUI_OVERRIDE_DRAWVERT_STRUCT_LAYOUT struct ImDrawVert { ImVec2 pos; float z; ImU32 col; ImVec2 uv; }
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_src = cmd_list->VtxBuffer.Data;
        for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
        {
            vtx_dst->pos[0] = vtx_src->pos.x;
            vtx_dst->pos[1] = vtx_src->pos.y;
            vtx_dst->pos[2] = 0.0f;
            vtx_dst->col = IMGUI_COL_TO_DX9_ARGB(vtx_src->col);
            vtx_dst->uv[0] = vtx_src->uv.x;
            vtx_dst->uv[1] = vtx_src->uv.y;
            vtx_dst++;
            vtx_src++;
        }
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    bd->pVB->Unlock();
    bd->pIB->Unlock();
    
    // Setup desired DX state
    ImGui_ImplDX9_SetupRenderState(draw_data);

    // Render command lists
    ImGui_ImplDX9_RenderCommandLists(draw_data);

    // Restore the DX9 transform
    bd->pd3dDevice->SetTransform(D3DTS_WORLD, &last_world);
    bd->pd3dDevice->SetTransform(D3DTS_VIEW, &last_view);
    bd->pd3dDevice->SetTransform(D3DTS_PROJECTION, &last_projection);

    // Restore the DX9 state
    d3d9_state_block->Apply();
    d3d9_state_block->Release();
}

bool ImGui_ImplDX9_Init(IDirect3DDevice9* device)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplDX9_Data* bd = IM_NEW(ImGui_ImplDX9_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_dx9";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

    bd->pd3dDevice = device;
    bd->pd3dDevice->AddRef();

    return true;
}

void ImGui_ImplDX9_Shutdown()
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplDX9_InvalidateDeviceObjects();
    if (bd->pd3dDevice) { bd->pd3dDevice->Release(); }
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    IM_DELETE(bd);
}

static bool ImGui_ImplDX9_CheckFormatSupport(IDirect3DDevice9* pDevice, D3DFORMAT format)
{
    IDirect3D9* pd3d = nullptr;
    if (pDevice->GetDirect3D(&pd3d) != D3D_OK)
        return false;
    D3DDEVICE_CREATION_PARAMETERS param = {};
    D3DDISPLAYMODE mode = {};
    if (pDevice->GetCreationParameters(&param) != D3D_OK || pDevice->GetDisplayMode(0, &mode) != D3D_OK)
    {
        pd3d->Release();
        return false;
    }
    // Font texture should support linear filter, color blend and write to render-target
    bool support = (pd3d->CheckDeviceFormat(param.AdapterOrdinal, param.DeviceType, mode.Format, D3DUSAGE_DYNAMIC | D3DUSAGE_QUERY_FILTER | D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, D3DRTYPE_TEXTURE, format)) == D3D_OK;
    pd3d->Release();
    return support;
}

static bool ImGui_ImplDX9_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    unsigned char* pixels;
    int width, height, bytes_per_pixel;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

    // Convert RGBA32 to BGRA32 (because RGBA32 is not well supported by DX9 devices)
#ifndef IMGUI_USE_BGRA_PACKED_COLOR
    const bool rgba_support = ImGui_ImplDX9_CheckFormatSupport(bd->pd3dDevice, D3DFMT_A8B8G8R8);
    if (!rgba_support && io.Fonts->TexPixelsUseColors)
    {
        ImU32* dst_start = (ImU32*)ImGui::MemAlloc((size_t)width * height * bytes_per_pixel);
        for (ImU32* src = (ImU32*)pixels, *dst = dst_start, *dst_end = dst_start + (size_t)width * height; dst < dst_end; src++, dst++)
            *dst = IMGUI_COL_TO_DX9_ARGB(*src);
        pixels = (unsigned char*)dst_start;
    }
#else
    const bool rgba_support = false;
#endif

    // Upload texture to graphics system
    bd->FontTexture = nullptr;
    if (bd->pd3dDevice->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, rgba_support ? D3DFMT_A8B8G8R8 : D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &bd->FontTexture, nullptr) < 0)
        return false;
    D3DLOCKED_RECT tex_locked_rect;
    if (bd->FontTexture->LockRect(0, &tex_locked_rect, nullptr, 0) != D3D_OK)
        return false;
    for (int y = 0; y < height; y++)
        memcpy((unsigned char*)tex_locked_rect.pBits + (size_t)tex_locked_rect.Pitch * y, pixels + (size_t)width * bytes_per_pixel * y, (size_t)width * bytes_per_pixel);
    bd->FontTexture->UnlockRect(0);

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)bd->FontTexture);

#ifndef IMGUI_USE_BGRA_PACKED_COLOR
    if (!rgba_support && io.Fonts->TexPixelsUseColors)
        ImGui::MemFree(pixels);
#endif

    return true;
}

bool ImGui_ImplDX9_CreateDeviceObjects()
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    if (!bd || !bd->pd3dDevice)
        return false;
    if (!ImGui_ImplDX9_CreateFontsTexture())
        return false;
    ImGui_ImplDX9WithShader_CreateDeviceObjects();
    return true;
}

void ImGui_ImplDX9_InvalidateDeviceObjects()
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    if (!bd || !bd->pd3dDevice)
        return;
    ImGui_ImplDX9WithShader_InvalidateDeviceObjects();
    if (bd->pVB) { bd->pVB->Release(); bd->pVB = nullptr; }
    if (bd->pIB) { bd->pIB->Release(); bd->pIB = nullptr; }
    if (bd->FontTexture) { bd->FontTexture->Release(); bd->FontTexture = nullptr; ImGui::GetIO().Fonts->SetTexID(0); } // We copied bd->pFontTextureView to io.Fonts->TexID so let's clear that as well.
}

void ImGui_ImplDX9_NewFrame()
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplDX9_Init()?");

    if (!bd->FontTexture)
        ImGui_ImplDX9_CreateDeviceObjects();
}

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
