// dear imgui: Renderer Backend for DirectX9
// This needs to be used along with a Platform Backend (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'LPDIRECT3DTEXTURE9' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Renderer: Support for large meshes (64k+ vertices) with 16-bit indices.

// You can copy and use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
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
#include "imgui_impl_dx9.h"

// Direct3D9
#include <d3d9.h>
#define SAFE_RELEASE(X) { if (X) { (X)->Release(); (X) = NULL; } }
#ifdef IMGUI_USE_BGRA_PACKED_COLOR
#define IMGUI_COL_TO_DX9_ARGB(_COL) (_COL)
#else
#define IMGUI_COL_TO_DX9_ARGB(_COL) (((_COL) & 0xFF00FF00) | (((_COL) & 0xFF0000) >> 16) | (((_COL) & 0xFF) << 16))
#endif

// Direct3D9 fixed rendering pipeline
struct CUSTOMVERTEX
{
    float    pos[3];
    D3DCOLOR col;
    float    uv[2];
};
static const DWORD D3DFVF_CUSTOMVERTEX = (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);

// Direct3D9 programmable rendering pipeline
static const D3DVERTEXELEMENT9 g_InputLayoutData[4] = {
    // Input vertex layout of default ImDrawVert
    { 0,  0, D3DDECLTYPE_FLOAT2  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0,  8, D3DDECLTYPE_FLOAT2  , D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR   , 0 },
    D3DDECL_END(),
};
static const BYTE g_VertexShaderData[] = {
    // HLSL source code
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
    // Precompile with Visual Studio 2019 HLSL compiler, vs_2_0
       0,   2, 254, 255, 254, 255, 
      30,   0,  67,  84,  65,  66, 
      28,   0,   0,   0,  75,   0, 
       0,   0,   0,   2, 254, 255, 
       1,   0,   0,   0,  28,   0, 
       0,   0,   0,   1,   0,   0, 
      68,   0,   0,   0,  48,   0, 
       0,   0,   2,   0,   0,   0, 
       4,   0,   2,   0,  52,   0, 
       0,   0,   0,   0,   0,   0, 
     109, 118, 112,   0,   3,   0, 
       3,   0,   4,   0,   4,   0, 
       1,   0,   0,   0,   0,   0, 
       0,   0, 118, 115,  95,  50, 
      95,  48,   0,  77, 105,  99, 
     114, 111, 115, 111, 102, 116, 
      32,  40,  82,  41,  32,  72, 
      76,  83,  76,  32,  83, 104, 
      97, 100, 101, 114,  32,  67, 
     111, 109, 112, 105, 108, 101, 
     114,  32,  49,  48,  46,  49, 
       0, 171,  31,   0,   0,   2, 
       0,   0,   0, 128,   0,   0, 
      15, 144,  31,   0,   0,   2, 
       5,   0,   0, 128,   1,   0, 
      15, 144,  31,   0,   0,   2, 
      10,   0,   0, 128,   2,   0, 
      15, 144,   5,   0,   0,   3, 
       0,   0,  15, 128,   0,   0, 
      85, 144,   1,   0, 228, 160, 
       4,   0,   0,   4,   0,   0, 
      15, 128,   0,   0, 228, 160, 
       0,   0,   0, 144,   0,   0, 
     228, 128,   2,   0,   0,   3, 
       0,   0,  15, 192,   0,   0, 
     228, 128,   3,   0, 228, 160, 
       1,   0,   0,   2,   0,   0, 
       3, 224,   1,   0, 228, 144, 
       1,   0,   0,   2,   0,   0, 
      15, 208,   2,   0, 228, 144, 
     255, 255,   0,   0
};
static const BYTE g_PixelShaderData[] = {
    // HLSL source code
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
    // Precompile with Visual Studio 2019 HLSL compiler, ps_2_0
       0,   2, 255, 255, 254, 255, 
      31,   0,  67,  84,  65,  66, 
      28,   0,   0,   0,  79,   0, 
       0,   0,   0,   2, 255, 255, 
       1,   0,   0,   0,  28,   0, 
       0,   0,   0,   1,   0,   0, 
      72,   0,   0,   0,  48,   0, 
       0,   0,   3,   0,   0,   0, 
       1,   0,   2,   0,  56,   0, 
       0,   0,   0,   0,   0,   0, 
     116, 101, 120,  48,   0, 171, 
     171, 171,   4,   0,  12,   0, 
       1,   0,   1,   0,   1,   0, 
       0,   0,   0,   0,   0,   0, 
     112, 115,  95,  50,  95,  48, 
       0,  77, 105,  99, 114, 111, 
     115, 111, 102, 116,  32,  40, 
      82,  41,  32,  72,  76,  83, 
      76,  32,  83, 104,  97, 100, 
     101, 114,  32,  67, 111, 109, 
     112, 105, 108, 101, 114,  32, 
      49,  48,  46,  49,   0, 171, 
      31,   0,   0,   2,   0,   0, 
       0, 128,   0,   0,   3, 176, 
      31,   0,   0,   2,   0,   0, 
       0, 128,   0,   0,  15, 144, 
      31,   0,   0,   2,   0,   0, 
       0, 144,   0,   8,  15, 160, 
      66,   0,   0,   3,   0,   0, 
      15, 128,   0,   0, 228, 176, 
       0,   8, 228, 160,   5,   0, 
       0,   3,   0,   0,  15, 128, 
       0,   0, 228, 128,   0,   0, 
     228, 144,   1,   0,   0,   2, 
       0,   8,  15, 128,   0,   0, 
     228, 128, 255, 255,   0,   0
};

// Direct3D9 data
static IDirect3DDevice9*            g_pd3dDevice       = NULL;
static IDirect3DTexture9*           g_pFontTexture     = NULL;
static IDirect3DVertexBuffer9*      g_pVB              = NULL;
static IDirect3DIndexBuffer9*       g_pIB              = NULL;
static int                          g_VertexBufferSize = 4096;
static int                          g_IndexBufferSize  = 8192;
// Direct3D9 programmable rendering pipeline data
static bool                         g_IsShaderPipeline = false;
static IDirect3DVertexDeclaration9* g_pInputLayout     = NULL;
static IDirect3DVertexShader9*      g_pVertexShader    = NULL;
static IDirect3DPixelShader9*       g_pPixelShader     = NULL;

// Setup render state
static void ImGui_ImplDX9_SetupRenderState(ImDrawData* draw_data)
{
    IDirect3DDevice9* ctx = g_pd3dDevice;
    
    // Setup orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
    // DisplayPos is (0,0) for single viewport apps.
    // Being agnostic of <DirectXMath.h> can be used, we aren't relying on DirectX::XMMatrixIdentity()/DirectX::XMMatrixOrthographicOffCenterLH()
    // D3DX, <d3dx9.h> and D3DXMatrixIdentity()/D3DXMatrixOrthoOffCenterLH() is not recommand. https://walbourn.github.io/where-is-the-directx-sdk-2021-edition/
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
    
    // [IA Stage] & [VS Stage] & [PS Stage]
    
    ctx->SetStreamSourceFreq(0, 1); // no instantiated drawing
    ctx->SetIndices(g_pIB);
    if (!g_IsShaderPipeline)
    {
        // Vertex buffer
        ctx->SetFVF(D3DFVF_CUSTOMVERTEX);
        ctx->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
        // Transforms
        const D3DMATRIX mat_identity =
        { { {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        } } };
        ctx->SetTransform(D3DTS_WORLD, &mat_identity);
        ctx->SetTransform(D3DTS_VIEW, &mat_identity);
        ctx->SetTransform(D3DTS_PROJECTION, &mat_projection);
        // texture stage state
        ctx->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        ctx->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        ctx->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        ctx->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        ctx->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        ctx->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        // No shaders
        ctx->SetVertexShader(NULL);
        ctx->SetPixelShader(NULL);
    }
    else
    {
        // Vertex buffer
        ctx->SetVertexDeclaration(g_pInputLayout);
        ctx->SetStreamSource(0, g_pVB, 0, sizeof(ImDrawVert));
        // Constant buffer
        ctx->SetVertexShaderConstantF(0, (float*)&mat_projection, 4); // float4x4 matrix
        // Shaders
        ctx->SetVertexShader(g_pVertexShader);
        ctx->SetPixelShader(g_pPixelShader);
    }
    // Sampler state
    ctx->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    ctx->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    ctx->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    ctx->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    
    // [RS Stage]
    
    // Rasterizer state
    ctx->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    ctx->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    ctx->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    ctx->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
    ctx->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
    // Setup viewport
    const D3DVIEWPORT9 viewport = { 0, 0, (DWORD)draw_data->DisplaySize.x, (DWORD)draw_data->DisplaySize.y, 0.0f, 1.0f };
    ctx->SetViewport(&viewport);
    
    // [OM Stage]
    
    // Depth stencil state
    ctx->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    ctx->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    // Blend state
    ctx->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    ctx->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    ctx->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    ctx->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    ctx->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ctx->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);
    ctx->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
    ctx->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
    const DWORD mask = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;
    ctx->SetRenderState(D3DRS_COLORWRITEENABLE, mask);
    
    // [Fixed Pipeline]
    
    // disable these features, especially lighting
    ctx->SetRenderState(D3DRS_CLIPPING, FALSE);
    ctx->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    ctx->SetRenderState(D3DRS_LIGHTING, FALSE);
    ctx->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
    ctx->SetRenderState(D3DRS_FOGENABLE, FALSE);
    ctx->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
}

// Create objects
static bool ImGui_ImplDX9_CreateBuffers(ImDrawData* draw_data)
{
    const DWORD usage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
    const D3DPOOL pool = D3DPOOL_DEFAULT; // D3DPOOL_MANAGED is not recommand
    // Check buffer size
    // #define INCREASE_SIZE(_V, _ADDV, _TARGET) { while ((_V) < (_TARGET)) { (_V) *= 2; } }
    #define INCREASE_SIZE(_V, _ADDV, _TARGET) { while ((_V) < (_TARGET)) { (_V) += (_ADDV); } }
    if (g_VertexBufferSize < draw_data->TotalVtxCount)
    {
        SAFE_RELEASE(g_pVB);
        INCREASE_SIZE(g_VertexBufferSize, 4096, draw_data->TotalVtxCount);
    }
    if (g_IndexBufferSize < draw_data->TotalIdxCount)
    {
        SAFE_RELEASE(g_pIB);
        INCREASE_SIZE(g_IndexBufferSize, 8192, draw_data->TotalIdxCount);
    }
    #undef INCREASE_SIZE
    // Create vertex buffer
    if (!g_pVB)
    {
        if (!g_IsShaderPipeline)
        {
            if (D3D_OK != g_pd3dDevice->CreateVertexBuffer(g_VertexBufferSize * sizeof(CUSTOMVERTEX), usage, D3DFVF_CUSTOMVERTEX, pool, &g_pVB, NULL))
                return false;
        }
        else
        {
            if (D3D_OK != g_pd3dDevice->CreateVertexBuffer(g_VertexBufferSize * sizeof(ImDrawVert), usage, 0, pool, &g_pVB, NULL))
                return false;
        }
    }
    // Create index buffer
    if (!g_pIB)
    {
        IM_ASSERT(sizeof(ImDrawIdx) == 2 || sizeof(ImDrawIdx) == 4);
        const D3DFORMAT format = sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32;
        if (D3D_OK != g_pd3dDevice->CreateIndexBuffer(g_IndexBufferSize * sizeof(ImDrawIdx), usage, format, pool, &g_pIB, NULL))
            return false;
    }
    
    // Copy and convert all vertices into a single contiguous buffer, convert colors to DX9 default format.
    // FIXME-OPT: This is a minor waste of resource, the ideal is to:
    //  1) to avoid repacking colors:   #define IMGUI_USE_BGRA_PACKED_COLOR
    
    // Copy vertex buffer
    if (!g_IsShaderPipeline)
    {
        CUSTOMVERTEX* vtx_dst = NULL;
        if (D3D_OK != g_pVB->Lock(0, (UINT)(draw_data->TotalVtxCount * sizeof(CUSTOMVERTEX)), (void**)&vtx_dst, D3DLOCK_DISCARD))
            return false;
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
                vtx_src++;
                vtx_dst++;
            }
        }
    }
    else
    {
        ImDrawVert* vtx_dst = NULL;
        if (D3D_OK != g_pVB->Lock(0, (UINT)(draw_data->TotalVtxCount * sizeof(ImDrawVert)), (void**)&vtx_dst, D3DLOCK_DISCARD))
            return false;
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
#ifndef IMGUI_USE_BGRA_PACKED_COLOR
            const ImDrawVert* vtx_src = cmd_list->VtxBuffer.Data;
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
    }
    if (D3D_OK != g_pVB->Unlock())
        return false;
    // Copy index buffer
    ImDrawIdx* idx_dst = NULL;
    if (D3D_OK != g_pIB->Lock(0, (UINT)(draw_data->TotalIdxCount * sizeof(ImDrawIdx)), (void**)&idx_dst, D3DLOCK_DISCARD))
        return false;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    if (D3D_OK != g_pIB->Unlock())
        return false;
    
    return true;
}
static bool ImGui_ImplDX9_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height, bytes_per_pixel;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

    // Convert RGBA32 to BGRA32 (because RGBA32 is not well supported by DX9 devices)
#ifndef IMGUI_USE_BGRA_PACKED_COLOR
    if (io.Fonts->TexPixelsUseColors)
    {
        ImU32* dst_start = (ImU32*)ImGui::MemAlloc(width * height * bytes_per_pixel);
        for (ImU32* src = (ImU32*)pixels, *dst = dst_start, *dst_end = dst_start + width * height; dst < dst_end; src++, dst++)
            *dst = IMGUI_COL_TO_DX9_ARGB(*src);
        pixels = (unsigned char*)dst_start;
    }
#endif
    
    // Upload texture to graphics system
    g_pFontTexture = NULL;
    if (g_pd3dDevice->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_pFontTexture, NULL) < 0)
        return false;
    D3DLOCKED_RECT tex_locked_rect;
    if (g_pFontTexture->LockRect(0, &tex_locked_rect, NULL, 0) != D3D_OK)
        return false;
    for (int y = 0; y < height; y++)
        memcpy((unsigned char*)tex_locked_rect.pBits + tex_locked_rect.Pitch * y, pixels + (width * bytes_per_pixel) * y, (width * bytes_per_pixel));
    g_pFontTexture->UnlockRect(0);
    
    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)g_pFontTexture);

#ifndef IMGUI_USE_BGRA_PACKED_COLOR
    if (io.Fonts->TexPixelsUseColors)
        ImGui::MemFree(pixels);
#endif

    return true;
}
static bool ImGui_ImplDX9_CreateShaderPipeline()
{
    if (D3D_OK != g_pd3dDevice->CreateVertexDeclaration(g_InputLayoutData, &g_pInputLayout))
        return false;
    if (D3D_OK != g_pd3dDevice->CreateVertexShader((DWORD*)g_VertexShaderData, &g_pVertexShader))
        return false;
    if (D3D_OK != g_pd3dDevice->CreatePixelShader((DWORD*)g_PixelShaderData, &g_pPixelShader))
        return false;
    return true;
}

// Render function.
void ImGui_ImplDX9_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;
    
    // Upload vertex & index data
    if (!ImGui_ImplDX9_CreateBuffers(draw_data))
        return;
    
    // Backup the DX9 transform (DX9 documentation suggests that it is included in the StateBlock but it doesn't appear to)
    D3DMATRIX last_world, last_view, last_projection;
    g_pd3dDevice->GetTransform(D3DTS_WORLD, &last_world);
    g_pd3dDevice->GetTransform(D3DTS_VIEW, &last_view);
    g_pd3dDevice->GetTransform(D3DTS_PROJECTION, &last_projection);
    
    // Backup the DX9 state
    IDirect3DStateBlock9* d3d9_state_block = NULL;
    if (D3D_OK != g_pd3dDevice->CreateStateBlock(D3DSBT_ALL, &d3d9_state_block))
        return;
    
    // Setup desired DX state
    ImGui_ImplDX9_SetupRenderState(draw_data);
    
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
                    ImGui_ImplDX9_SetupRenderState(draw_data);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                const RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
                g_pd3dDevice->SetTexture(0, (IDirect3DTexture9*)pcmd->TextureId);
                g_pd3dDevice->SetScissorRect(&r);
                g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, pcmd->VtxOffset + global_vtx_offset, 0, (UINT)cmd_list->VtxBuffer.Size, pcmd->IdxOffset + global_idx_offset, pcmd->ElemCount / 3);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }
    
    // Restore the DX9 transform
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &last_world);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &last_view);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &last_projection);
    
    // Restore the DX9 state
    d3d9_state_block->Apply();
    d3d9_state_block->Release();
}

bool ImGui_ImplDX9_Init(IDirect3DDevice9* device)
{
    IM_ASSERT(device);
    if (!device)
        return false;
    g_pd3dDevice = device;
    g_pd3dDevice->AddRef();
    
    // Setup backend capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_dx9";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    
    return true;
}

void ImGui_ImplDX9_Shutdown()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    SAFE_RELEASE(g_pd3dDevice);
}

bool ImGui_ImplDX9_CreateDeviceObjects()
{
    if (!g_pd3dDevice)
        return false;
    if (!ImGui_ImplDX9_CreateFontsTexture())
        return false;
    ImGui_ImplDX9_CreateShaderPipeline(); // Shader pipeline is optional
    return true;
}

void ImGui_ImplDX9_InvalidateDeviceObjects()
{
    SAFE_RELEASE(g_pFontTexture);
    SAFE_RELEASE(g_pVB);
    SAFE_RELEASE(g_pIB);
    g_VertexBufferSize = 4096;
    g_IndexBufferSize  = 8192;
    g_IsShaderPipeline = false;
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pVertexShader);
    SAFE_RELEASE(g_pPixelShader);
    if (!g_pd3dDevice)
        return;
    ImGui::GetIO().Fonts->SetTexID(NULL); // We copied g_pFontTexture to io.Fonts->TexID so let's clear that as well.
}

void ImGui_ImplDX9_NewFrame()
{
    if (!g_pFontTexture)
        ImGui_ImplDX9_CreateDeviceObjects();
}
