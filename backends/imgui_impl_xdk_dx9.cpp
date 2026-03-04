// dear imgui: Renderer Backend for DirectX9 on Xbox 360
// This needs to be used along with the XDK and the xbox plataform code

// Implemented features:
//  [X] Renderer: User texture binding. Use 'LPDIRECT3DTEXTURE9' as texture identifier. Read the FAQ about ImTextureID/ImTextureRef!
//  [X] Renderer: Large meshes support (64k+ vertices) even with 16-bit indices (ImGuiBackendFlags_RendererHasVtxOffset).
//  [X] Renderer: Texture updates support for dynamic font atlas (ImGuiBackendFlags_RendererHasTextures).
//  [X] Renderer: IMGUI_USE_BGRA_PACKED_COLOR support, as this is the optimal color encoding for DirectX9.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2026-03-03: Basic support Xbox 360

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_xdk_dx9.h"

// DirectX

#ifndef _XBOX
    #error "This headers requires de xbox 360 XDK"
#else 
    #include <xtl.h>
#endif

#include <d3d9.h>
#include <d3dx9.h>

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wold-style-cast"   // warning: use of old-style cast                    // yes, they are more terse.
#pragma clang diagnostic ignored "-Wsign-conversion"  // warning: implicit conversion changes signedness
#endif

// --- SHADERS -------------------------------------------------------
const char* g_szVertexShaderPgr =
"float4x4 ProjectionMatrix;"
"struct VS_INPUT"
"{"
"    float2 pos : POSITION0;"
"    float4 col : COLOR0;"
"    float2 uv  : TEXCOORD0;"
"};"
"struct PS_INPUT"
"{"
"    float4 pos : POSITION0;"
"    float4 col : COLOR0;"
"    float2 uv  : TEXCOORD0;"
"};"
"PS_INPUT main(VS_INPUT input)"
"{"
"    PS_INPUT output;"
"    output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));"
"    output.col = input.col;"
"    output.uv  = input.uv;"
"    return output;"
"}";

const char* g_szPixelShaderPgr =
"sampler2D sampler0 : register(s0);"
"struct PS_INPUT"
"{"
"    float4 pos: POSITION0;"
"    float4 col: COLOR0;"
"    float2 uv:  TEXCOORD0;"
"};"
"float4 main(PS_INPUT input) : COLOR0"
"{"
"    return tex2D(sampler0, input.uv) * input.col;"
"}";

// DirectX data
struct ImGui_ImplDX9_Data
{
    LPDIRECT3DDEVICE9           pd3dDevice;
    LPDIRECT3DVERTEXBUFFER9     pVB;
    LPDIRECT3DINDEXBUFFER9      pIB;
    D3DVertexShader*            pVertexShader; // Vertex Shader
    D3DPixelShader*             pPixelShader;  // Pixel Shader
    D3DVertexDeclaration*       pVertexDecl;   // Vertex format decl
    
    // TODO:
    int                         VertexBufferSize;
    int                         IndexBufferSize;
    bool                        HasRgbaSupport;

    ImGui_ImplDX9_Data()        { memset((void*)this, 0, sizeof(*this)); VertexBufferSize = 5000; IndexBufferSize = 10000; }
};

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

// Functions
static void ImGui_ImplDX9_SetupRenderState(ImDrawData* draw_data)
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();

    // Setup viewport
    D3DVIEWPORT9 vp;
    vp.X = vp.Y = 0;
    vp.Width = (DWORD)draw_data->DisplaySize.x;
    vp.Height = (DWORD)draw_data->DisplaySize.y;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;

    LPDIRECT3DDEVICE9 device = bd->pd3dDevice;
    device->SetViewport(&vp);

    device->SetVertexShader(bd->pVertexShader);
    device->SetPixelShader(bd->pPixelShader);
    device->SetVertexDeclaration(bd->pVertexDecl);

    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
    device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

    // Setup orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    // Being agnostic of whether <d3dx9.h> or <DirectXMath.h> can be used, we aren't relying on D3DXMatrixIdentity()/D3DXMatrixOrthoOffCenterLH() or DirectX::XMMatrixIdentity()/DirectX::XMMatrixOrthographicOffCenterLH()
    {
        float L = draw_data->DisplayPos.x + 0.5f;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x + 0.5f;
        float T = draw_data->DisplayPos.y + 0.5f;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y + 0.5f;
        D3DMATRIX mat_projection =
        { { {
            2.0f/(R-L),   0.0f,         0.0f,  0.0f,
            0.0f,         2.0f/(T-B),   0.0f,  0.0f,
            0.0f,         0.0f,         0.5f,  0.0f,
            (L+R)/(L-R),  (T+B)/(B-T),  0.5f,  1.0f
        } } };
        device->SetVertexShaderConstantF(0, (float*)&mat_projection, 4);
    }
}

// Render function.
void ImGui_ImplDX9_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized (never true on xbox)
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    LPDIRECT3DDEVICE9 device = bd->pd3dDevice;

    // Catch up with texture updates. Most of the times, the list will have 1 element with an OK status, aka nothing to do.
    // (This almost always points to ImGui::GetPlatformIO().Textures[] but is part of ImDrawData to allow overriding or disabling texture updates).
    if (draw_data->Textures != nullptr)
        IM_FOREACH(ImTextureData* tex , *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_ImplDX9_UpdateTexture(tex);

    // Create and grow buffers if needed
    if (!bd->pVB || bd->VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (bd->pVB) { bd->pVB->Release(); bd->pVB = nullptr; }
        bd->VertexBufferSize = draw_data->TotalVtxCount + 5000;
        HRESULT hr = device->CreateVertexBuffer(
            bd->VertexBufferSize * sizeof(ImDrawVert), 
            D3DUSAGE_WRITEONLY, 
            0,
            D3DPOOL_DEFAULT, // D3DUSAGE_DYNAMIC is not supported on Xbox 360
            &bd->pVB, nullptr);
        if (FAILED(hr))
        {
            OutputDebugString("CreateVertexBuffer failed");
            return;
        }
    }
    if (!bd->pIB || bd->IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (bd->pIB) { bd->pIB->Release(); bd->pIB = nullptr; }
        bd->IndexBufferSize = draw_data->TotalIdxCount + 10000;
        HRESULT hr = device->CreateIndexBuffer(
            bd->IndexBufferSize * sizeof(ImDrawIdx), 
            D3DUSAGE_WRITEONLY, // D3DUSAGE_DYNAMIC is not supported on Xbox 360
            sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32, 
            D3DPOOL_DEFAULT, &bd->pIB, 
            nullptr);
        if (FAILED(hr))
        {
            OutputDebugString("CreateIndexBuffer failed");
            return;
        }
    }

    // Backup the DX9 state
    // Xbox 360 XDK uses D3DStateBlock* instead of IDirect3DStateBlock9*
    D3DStateBlock* state_block = nullptr;
    if (device->CreateStateBlock(D3DSBT_ALL, &state_block) < 0)
        return;
    if (state_block->Capture() < 0)
    {
        state_block->Release();
        return;
    }

    // NOTE: GetTransform/SetTransform are NOT available on Xbox 360.
    // The projection matrix is handled entirely via SetVertexShaderConstantF
    // inside ImGui_ImplDX9_SetupRenderState — no backup/restore needed here.

    // Allocate buffers
    // D3DLOCK_NOOVERWRITE is only valid on dynamic buffers.
    // Since we recreate the buffer when it grows and overwrite from offset 0
    // every frame, lock with 0 flags is correct here.
    // Xbox 360: Lock() does NOT support range locking — OffsetToLock and SizeToLock MUST be 0.
    // On PC these args are an optimization hint; on 360 passing non-zero values triggers
    // ERR[D3D]: "Range locking is not implemented" and a forced trap in debug builds.
    // In release the validation is silently skipped but behavior is undefined.
    // We always lock the full buffer (0, 0) and write from the beginning each frame,
    // which is safe because we recreate the buffer whenever it needs to grow.

    // Copy vertices directly as ImDrawVert — no struct conversion needed.
    // Only the color channel order needs fixing: ImGui outputs RGBA, Xenos expects BGRA.
    // We swap R and B per vertex. If IMGUI_USE_BGRA_PACKED_COLOR is defined in imconfig.h,
    // ImGui generates BGRA directly and the swap below becomes a no-op (identity macro).
    ImDrawVert* vtx_dst;
    ImDrawIdx* idx_dst;
    if (bd->pVB->Lock(0, 0, (void**)&vtx_dst, 0) < 0)
    {
        state_block->Release();
        return;
    }
    if (bd->pIB->Lock(0, 0, (void**)&idx_dst, 0) < 0)
    {
        bd->pVB->Unlock();
        state_block->Release();
        return;
    }
    IM_FOREACH(const ImDrawList* draw_list , draw_data->CmdLists)
    {
        const ImDrawVert* vtx_src = draw_list->VtxBuffer.Data;
        for (int i = 0; i < draw_list->VtxBuffer.Size; i++)
        {
            *vtx_dst = *vtx_src;
            vtx_dst->col = IMGUI_COL_TO_DX9_ARGB(vtx_src->col); // RGBA → BGRA
            vtx_dst++;
            vtx_src++;
        }
        memcpy(idx_dst, draw_list->IdxBuffer.Data, draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        idx_dst += draw_list->IdxBuffer.Size;
    }
    bd->pVB->Unlock();
    bd->pIB->Unlock();
    device->SetStreamSource(0, bd->pVB, 0, sizeof(ImDrawVert));
    device->SetIndices(bd->pIB);

    // Setup desired DX state
    ImGui_ImplDX9_SetupRenderState(draw_data);

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    IM_FOREACH(const ImDrawList* draw_list , draw_data->CmdLists)
    {
        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplDX9_SetupRenderState(draw_data);
                else
                    pcmd->UserCallback(draw_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
                ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle
                const RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
                device->SetScissorRect(&r);

                // Bind texture, Draw
                const LPDIRECT3DTEXTURE9 texture = (LPDIRECT3DTEXTURE9)pcmd->GetTexID();
                device->SetTexture(0, texture);
                device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, pcmd->VtxOffset + global_vtx_offset, 0, (UINT)draw_list->VtxBuffer.Size, pcmd->IdxOffset + global_idx_offset, pcmd->ElemCount / 3);
            }
        }
        global_idx_offset += draw_list->IdxBuffer.Size;
        global_vtx_offset += draw_list->VtxBuffer.Size;
    }

    // NOTE: Transform restore is intentionally omitted — Xbox 360 does not support
    // Get/SetTransform. The projection matrix is managed via vertex shader constants
    // (SetVertexShaderConstantF), which are saved and restored by the StateBlock above.

    // Restore the DX9 state
    state_block->Apply();
    state_block->Release();
}

static bool ImGui_ImplDX9_CheckFormatSupport(LPDIRECT3DDEVICE9 pDevice, D3DFORMAT format)
{
    LPDIRECT3D9 pd3d = nullptr;
    if (pDevice->GetDirect3D(&pd3d) != D3D_OK)
        return false;
    D3DDEVICE_CREATION_PARAMETERS param = {};
    D3DDISPLAYMODE mode = {};
    if (pDevice->GetCreationParameters(&param) != D3D_OK || pDevice->GetDisplayMode(0, &mode) != D3D_OK)
    {
        pd3d->Release();
        return false;
    }
    // Xbox 360: D3DUSAGE_DYNAMIC is not supported, check without it.
    // We keep D3DUSAGE_QUERY_FILTER and D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING.
    bool support = (pd3d->CheckDeviceFormat(param.AdapterOrdinal, 
            param.DeviceType, 
            mode.Format, 
            D3DUSAGE_QUERY_FILTER | D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
            D3DRTYPE_TEXTURE, format)) == D3D_OK;
    pd3d->Release();
    return support;
}

static HRESULT CompileShaders(
    D3DDevice* pd3dDevice,
    ImGui_ImplDX9_Data* const data
) 
{
    ID3DXBuffer* pVertexShaderCode;
    ID3DXBuffer* pVertexErrorMsg;
    // compile shaders
    HRESULT hr = D3DXCompileShader(
        g_szVertexShaderPgr,
        (UINT)strlen(g_szVertexShaderPgr),
        NULL, NULL,
        "main", "vs_3_0",
        0,
        &pVertexShaderCode,
        &pVertexErrorMsg,
        NULL
    );

    if (FAILED(hr))
    {
        if (pVertexErrorMsg)
        {
            OutputDebugString("D3DXCompileShader for vs_3_0 failed: ");
            OutputDebugString((char*)pVertexErrorMsg->GetBufferPointer());
            return E_FAIL;
        }
    }

    // create vertex shader
    pd3dDevice->CreateVertexShader(
        (DWORD*)pVertexShaderCode->GetBufferPointer(),
        &data->pVertexShader
    );

    // compile pixel shader
    ID3DXBuffer* pPixelShaderCode;
    ID3DXBuffer* pPixelErrorMsg;
    hr = D3DXCompileShader(
        g_szPixelShaderPgr,
        (UINT)strlen(g_szPixelShaderPgr),
        NULL, NULL,
        "main", "ps_3_0",
        0,
        &pPixelShaderCode,
        &pPixelErrorMsg,
        NULL
    );

    if (FAILED(hr))
    {
        if (pPixelErrorMsg)
        {
            OutputDebugString("D3DXCompileShader for ps_3_0 failed: ");
            OutputDebugString((char*)pPixelErrorMsg->GetBufferPointer());
            return E_FAIL;
        }
    }

    // create pixel shader
    pd3dDevice->CreatePixelShader(
        (DWORD*)pPixelShaderCode->GetBufferPointer(), 
        &data->pPixelShader
    );

    return S_OK;
}

bool ImGui_ImplDX9_Init(IDirect3DDevice9* device)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplDX9_Data* bd = IM_NEW(ImGui_ImplDX9_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_xdk_dx9";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;   // We can honor ImGuiPlatformIO::Textures[] requests during render.

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_TextureMaxWidth = platform_io.Renderer_TextureMaxHeight = 4096;

    bd->pd3dDevice = device;
    bd->pd3dDevice->AddRef();
    bd->HasRgbaSupport = ImGui_ImplDX9_CheckFormatSupport(bd->pd3dDevice, D3DFMT_A8B8G8R8);

    HRESULT hr = CompileShaders(device, bd);
    if FAILED(hr)
    {
        return false;
    }

    // Vertex declaration mirrors ImDrawVert memory layout exactly:
    D3DVERTEXELEMENT9 decl[] =
    {
        // Stream,         Offset,                  Type,                    Method,                 Usage,       UsageIndex
        { 0, (WORD)offsetof(ImDrawVert, pos), D3DDECLTYPE_FLOAT2,      D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, (WORD)offsetof(ImDrawVert, uv),  D3DDECLTYPE_FLOAT2,      D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, (WORD)offsetof(ImDrawVert, col), D3DDECLTYPE_D3DCOLOR,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
        D3DDECL_END()
    };
    hr = device->CreateVertexDeclaration(decl, &bd->pVertexDecl);
    if (FAILED(hr))
    {
        OutputDebugString("CreateVertexDeclaration failed!");
    }
    return true;
}

void ImGui_ImplDX9_Shutdown()
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    ImGui_ImplDX9_InvalidateDeviceObjects();
    if (bd->pd3dDevice) { bd->pd3dDevice->Release(); }

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
    platform_io.ClearRendererHandlers();
    IM_DELETE(bd);
}

// Convert RGBA32 to BGRA32 (because RGBA32 is not well supported by DX9 devices)
static void ImGui_ImplDX9_CopyTextureRegion(
    bool tex_use_colors, 
    const ImU32* src, 
    int src_pitch, 
    ImU32* dst, 
    int dst_pitch, 
    int w, int h)
{
#ifndef IMGUI_USE_BGRA_PACKED_COLOR
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    const bool convert_rgba_to_bgra = (!bd->HasRgbaSupport && tex_use_colors);
#else
    const bool convert_rgba_to_bgra = false;
    IM_UNUSED(tex_use_colors);
#endif
    for (int y = 0; y < h; y++)
    {
        const ImU32* src_p = (const ImU32*)(const void*)((const unsigned char*)src + src_pitch * y);
        ImU32* dst_p = (ImU32*)(void*)((unsigned char*)dst + dst_pitch * y);
        if (convert_rgba_to_bgra)
            for (int x = w; x > 0; x--, src_p++, dst_p++) // Convert copy
                *dst_p = IMGUI_COL_TO_DX9_ARGB(*src_p);
        else
            memcpy(dst_p, src_p, w * 4); // Raw copy
    }
}

void ImGui_ImplDX9_UpdateTexture(ImTextureData* tex)
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();

    if (tex->Status == ImTextureStatus_WantCreate)
    {
        // Create and upload new texture to graphics system
        //IMGUI_DEBUG_LOG("UpdateTexture #%03d: WantCreate %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
        IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);
        LPDIRECT3DTEXTURE9 dx_tex = nullptr;
        // Xbox 360: D3DUSAGE_DYNAMIC is not supported. Use 0 (static texture).
        // The texture is written once at creation and updated via LockRect/UnlockRect.
        HRESULT hr = bd->pd3dDevice->CreateTexture(
            tex->Width, tex->Height, 1, 0, 
            bd->HasRgbaSupport ? D3DFMT_LIN_A8B8G8R8 : D3DFMT_LIN_A8R8G8B8, 
            D3DPOOL_DEFAULT,
            &dx_tex, 
            nullptr);
        if (hr < 0)
        {
            IM_ASSERT(hr >= 0 && "Backend failed to create texture!");
            return;
        }

        D3DLOCKED_RECT locked_rect;
        if (dx_tex->LockRect(0, &locked_rect, nullptr, 0) == D3D_OK)
        {
            ImGui_ImplDX9_CopyTextureRegion(tex->UseColors, 
                    (ImU32*)tex->GetPixels(), 
                    tex->Width * 4, 
                    (ImU32*)locked_rect.pBits, 
                    (ImU32)locked_rect.Pitch, 
                    tex->Width, tex->Height);
            dx_tex->UnlockRect(0);
        }

        // Store identifiers
        tex->SetTexID((ImTextureID)(intptr_t)dx_tex);
        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantUpdates)
    {
        // Update selected blocks. We only ever write to textures regions which have never been used before!
        // This backend choose to use tex->Updates[] but you can use tex->UpdateRect to upload a single region.
        LPDIRECT3DTEXTURE9 backend_tex = (LPDIRECT3DTEXTURE9)(intptr_t)tex->TexID;
        // RECT update_rect = { (LONG)tex->UpdateRect.x, (LONG)tex->UpdateRect.y, (LONG)(tex->UpdateRect.x + tex->UpdateRect.w), (LONG)(tex->UpdateRect.y + tex->UpdateRect.h) };
        D3DLOCKED_RECT locked_rect;
        // Xbox 360: tiled textures do NOT support partial LockRect — the rect argument
        // MUST be NULL (full texture lock). Passing a subrect triggers:
        // ERR[D3D]: "Lock rectangle must match the texture dimensions for tiled formats."
        // We lock the full texture and offset into pBits manually using r.x / r.y.
        if (backend_tex->LockRect(0, &locked_rect, NULL, 0) == D3D_OK)
            IM_FOREACH(ImTextureRect& r , tex->Updates)
                ImGui_ImplDX9_CopyTextureRegion(tex->UseColors, (ImU32*)tex->GetPixelsAt(r.x, r.y), tex->Width * 4,
                    (ImU32*)locked_rect.pBits + r.x + r.y * (locked_rect.Pitch / 4), (int)locked_rect.Pitch, r.w, r.h);
        backend_tex->UnlockRect(0);
        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantDestroy)
    {
        if (LPDIRECT3DTEXTURE9 backend_tex = (LPDIRECT3DTEXTURE9)tex->TexID)
        {
            IM_ASSERT(tex->TexID == (ImTextureID)(intptr_t)backend_tex);
            backend_tex->Release();

            // Clear identifiers and mark as destroyed (in order to allow e.g. calling InvalidateDeviceObjects while running)
            tex->SetTexID(ImTextureID_Invalid);
        }
        tex->SetStatus(ImTextureStatus_Destroyed);
    }
}

bool ImGui_ImplDX9_CreateDeviceObjects()
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    if (!bd || !bd->pd3dDevice)
        return false;
    return true;
}

void ImGui_ImplDX9_InvalidateDeviceObjects()
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    if (!bd || !bd->pd3dDevice)
        return;

    // Destroy all textures
    IM_FOREACH(ImTextureData* tex, ImGui::GetPlatformIO().Textures)
        if (tex->RefCount == 1)
        {
            tex->SetStatus(ImTextureStatus_WantDestroy);
            ImGui_ImplDX9_UpdateTexture(tex);
        }
    if (bd->pVB) { bd->pVB->Release(); bd->pVB = nullptr; }
    if (bd->pIB) { bd->pIB->Release(); bd->pIB = nullptr; }
}

void ImGui_ImplDX9_NewFrame()
{
    ImGui_ImplDX9_Data* bd = ImGui_ImplDX9_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplDX9_Init()?");
    IM_UNUSED(bd);
}

//-----------------------------------------------------------------------------

void __cdecl __imgui_xbox_assert_handler(const char* const lpszAssert)
{
    OutputDebugString(lpszAssert);
    WCHAR lpszMsg[255];
    int nWritten = MultiByteToWideChar(
        CP_UTF8, 0, 
        lpszAssert, (int)strlen(lpszAssert), 
        lpszMsg, sizeof(lpszMsg)/sizeof(WCHAR));
    lpszMsg[(nWritten + 1) % 255] = 0;
#if !defined(_PREFAST_) && !defined(NDEBUG)
    _wassert(lpszMsg, _CRT_WIDE(__FILE__), __LINE__);
#else
    __debugbreak();
#endif
}

#endif // #ifndef IMGUI_DISABLE