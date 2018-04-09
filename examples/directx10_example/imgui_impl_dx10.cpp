// ImGui Win32 + DirectX10 binding

// Implemented features:
//  [X] User texture binding. Use 'ID3D10ShaderResourceView*' as ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG 
// (minor and older changes stripped away, please see git history for details)
//  2018-04-09: Misc: Fixed erroneous call to io.Fonts->ClearInputData() + ClearTexData() that was left in DX10 example but removed in 1.47 (Nov 2015) on other back-ends.
//  2018-03-20: Misc: Setup io.BackendFlags ImGuiBackendFlags_HasMouseCursors and ImGuiBackendFlags_HasSetMousePos flags + honor ImGuiConfigFlags_NoMouseCursorChange flag.
//  2018-02-20: Inputs: Added support for mouse cursors (ImGui::GetMouseCursor() value and WM_SETCURSOR message handling).
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback and exposed ImGui_ImplDX10_RenderDrawData() in the .h file so you can call it yourself.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2018-02-06: Inputs: Added mapping for ImGuiKey_Space.
//  2018-02-06: Inputs: Honoring the io.WantSetMousePos by repositioning the mouse (when using navigation and ImGuiConfigFlags_NavEnableSetMousePos is set).
//  2018-01-20: Inputs: Added Horizontal Mouse Wheel support.
//  2018-01-08: Inputs: Added mapping for ImGuiKey_Insert.
//  2018-01-05: Inputs: Added WM_LBUTTONDBLCLK double-click handlers for window classes with the CS_DBLCLKS flag.
//  2017-10-23: Inputs: Added WM_SYSKEYDOWN / WM_SYSKEYUP handlers so e.g. the VK_MENU key can be read.
//  2017-10-23: Inputs: Using Win32 ::SetCapture/::GetCapture() to retrieve mouse positions outside the client area when dragging. 
//  2016-11-12: Inputs: Only call Win32 ::SetCursor(NULL) when io.MouseDrawCursor is set.
//  2016-05-07: DirectX10: Disabling depth-write.

#include "imgui.h"
#include "imgui_impl_dx10.h"

// DirectX
#include <d3d10_1.h>
#include <d3d10.h>
#include <d3dcompiler.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

// Win32 Data
static HWND                     g_hWnd = 0;
static INT64                    g_Time = 0;
static INT64                    g_TicksPerSecond = 0;
static ImGuiMouseCursor         g_LastMouseCursor = ImGuiMouseCursor_COUNT;

// DirectX data
static ID3D10Device*            g_pd3dDevice = NULL;
static ID3D10Buffer*            g_pVB = NULL;
static ID3D10Buffer*            g_pIB = NULL;
static ID3D10Blob *             g_pVertexShaderBlob = NULL;
static ID3D10VertexShader*      g_pVertexShader = NULL;
static ID3D10InputLayout*       g_pInputLayout = NULL;
static ID3D10Buffer*            g_pVertexConstantBuffer = NULL;
static ID3D10Blob *             g_pPixelShaderBlob = NULL;
static ID3D10PixelShader*       g_pPixelShader = NULL;
static ID3D10SamplerState*      g_pFontSampler = NULL;
static ID3D10ShaderResourceView*g_pFontTextureView = NULL;
static ID3D10RasterizerState*   g_pRasterizerState = NULL;
static ID3D10BlendState*        g_pBlendState = NULL;
static ID3D10DepthStencilState* g_pDepthStencilState = NULL;
static int                      g_VertexBufferSize = 5000, g_IndexBufferSize = 10000;

struct VERTEX_CONSTANT_BUFFER
{
    float        mvp[4][4];
};

// Render function
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
void ImGui_ImplDX10_RenderDrawData(ImDrawData* draw_data)
{
    ID3D10Device* ctx = g_pd3dDevice;

    // Create and grow vertex/index buffers if needed
    if (!g_pVB || g_VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }
        g_VertexBufferSize = draw_data->TotalVtxCount + 5000;
        D3D10_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D10_BUFFER_DESC));
        desc.Usage = D3D10_USAGE_DYNAMIC;
        desc.ByteWidth = g_VertexBufferSize * sizeof(ImDrawVert);
        desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        if (ctx->CreateBuffer(&desc, NULL, &g_pVB) < 0)
            return;
    }

    if (!g_pIB || g_IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
        g_IndexBufferSize = draw_data->TotalIdxCount + 10000;
        D3D10_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D10_BUFFER_DESC));
        desc.Usage = D3D10_USAGE_DYNAMIC;
        desc.ByteWidth = g_IndexBufferSize * sizeof(ImDrawIdx);
        desc.BindFlags = D3D10_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
        if (ctx->CreateBuffer(&desc, NULL, &g_pIB) < 0)
            return;
    }

    // Copy and convert all vertices into a single contiguous buffer
    ImDrawVert* vtx_dst = NULL;
    ImDrawIdx* idx_dst = NULL;
    g_pVB->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&vtx_dst);
    g_pIB->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&idx_dst);
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    g_pVB->Unmap();
    g_pIB->Unmap();

    // Setup orthographic projection matrix into our constant buffer
    {
        void* mapped_resource;
        if (g_pVertexConstantBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
            return;
        VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)mapped_resource;
        const float L = 0.0f;
        const float R = ImGui::GetIO().DisplaySize.x;
        const float B = ImGui::GetIO().DisplaySize.y;
        const float T = 0.0f;
        const float mvp[4][4] =
        {
            { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
        };
        memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
        g_pVertexConstantBuffer->Unmap();
    }

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
        D3D10_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
        ID3D10Buffer*               IndexBuffer, *VertexBuffer, *VSConstantBuffer;
        UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
        DXGI_FORMAT                 IndexBufferFormat;
        ID3D10InputLayout*          InputLayout;
    };
    BACKUP_DX10_STATE old;
    old.ScissorRectsCount = old.ViewportsCount = D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->RSGetScissorRects(&old.ScissorRectsCount, old.ScissorRects);
    ctx->RSGetViewports(&old.ViewportsCount, old.Viewports);
    ctx->RSGetState(&old.RS);
    ctx->OMGetBlendState(&old.BlendState, old.BlendFactor, &old.SampleMask);
    ctx->OMGetDepthStencilState(&old.DepthStencilState, &old.StencilRef);
    ctx->PSGetShaderResources(0, 1, &old.PSShaderResource);
    ctx->PSGetSamplers(0, 1, &old.PSSampler);
    ctx->PSGetShader(&old.PS);
    ctx->VSGetShader(&old.VS);
    ctx->VSGetConstantBuffers(0, 1, &old.VSConstantBuffer);
    ctx->IAGetPrimitiveTopology(&old.PrimitiveTopology);
    ctx->IAGetIndexBuffer(&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
    ctx->IAGetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
    ctx->IAGetInputLayout(&old.InputLayout);

    // Setup viewport
    D3D10_VIEWPORT vp;
    memset(&vp, 0, sizeof(D3D10_VIEWPORT));
    vp.Width = (UINT)ImGui::GetIO().DisplaySize.x;
    vp.Height = (UINT)ImGui::GetIO().DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0;
    ctx->RSSetViewports(1, &vp);

    // Bind shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    ctx->IASetInputLayout(g_pInputLayout);
    ctx->IASetVertexBuffers(0, 1, &g_pVB, &stride, &offset);
    ctx->IASetIndexBuffer(g_pIB, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_pVertexShader);
    ctx->VSSetConstantBuffers(0, 1, &g_pVertexConstantBuffer);
    ctx->PSSetShader(g_pPixelShader);
    ctx->PSSetSamplers(0, 1, &g_pFontSampler);

    // Setup render state
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    ctx->OMSetBlendState(g_pBlendState, blend_factor, 0xffffffff);
    ctx->OMSetDepthStencilState(g_pDepthStencilState, 0);
    ctx->RSSetState(g_pRasterizerState);

    // Render command lists
    int vtx_offset = 0;
    int idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                const D3D10_RECT r = { (LONG)pcmd->ClipRect.x, (LONG)pcmd->ClipRect.y, (LONG)pcmd->ClipRect.z, (LONG)pcmd->ClipRect.w };
                ctx->PSSetShaderResources(0, 1, (ID3D10ShaderResourceView**)&pcmd->TextureId);
                ctx->RSSetScissorRects(1, &r);
                ctx->DrawIndexed(pcmd->ElemCount, idx_offset, vtx_offset);
            }
            idx_offset += pcmd->ElemCount;
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
    }

    // Restore modified DX state
    ctx->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
    ctx->RSSetViewports(old.ViewportsCount, old.Viewports);
    ctx->RSSetState(old.RS); if (old.RS) old.RS->Release();
    ctx->OMSetBlendState(old.BlendState, old.BlendFactor, old.SampleMask); if (old.BlendState) old.BlendState->Release();
    ctx->OMSetDepthStencilState(old.DepthStencilState, old.StencilRef); if (old.DepthStencilState) old.DepthStencilState->Release();
    ctx->PSSetShaderResources(0, 1, &old.PSShaderResource); if (old.PSShaderResource) old.PSShaderResource->Release();
    ctx->PSSetSamplers(0, 1, &old.PSSampler); if (old.PSSampler) old.PSSampler->Release();
    ctx->PSSetShader(old.PS); if (old.PS) old.PS->Release();
    ctx->VSSetShader(old.VS); if (old.VS) old.VS->Release();
    ctx->VSSetConstantBuffers(0, 1, &old.VSConstantBuffer); if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
    ctx->IASetPrimitiveTopology(old.PrimitiveTopology);
    ctx->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset); if (old.IndexBuffer) old.IndexBuffer->Release();
    ctx->IASetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset); if (old.VertexBuffer) old.VertexBuffer->Release();
    ctx->IASetInputLayout(old.InputLayout); if (old.InputLayout) old.InputLayout->Release();
}

static bool ImGui_ImplWin32_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    ImGuiMouseCursor imgui_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
    }
    else
    {
        // Hardware cursor type
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        }
        ::SetCursor(::LoadCursor(NULL, win32_cursor));
    }
    return true;
}

// Allow compilation with old Windows SDK. MinGW doesn't have default _WIN32_WINNT/WINVER versions.
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

// Process Win32 mouse/keyboard inputs. 
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
// PS: In this Win32 handler, we use the capture API (GetCapture/SetCapture/ReleaseCapture) to be able to read mouse coordinations when dragging mouse outside of our window bounds.
// PS: We treat DBLCLK messages as regular mouse down messages, so this code will work on windows classes that have the CS_DBLCLKS flag set. Our own example app code doesn't set this flag.
IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() == NULL)
        return 0;

    ImGuiIO& io = ImGui::GetIO();
    switch (msg)
    {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    {
        int button = 0;
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) button = 0;
        if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) button = 1;
        if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) button = 2;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
            ::SetCapture(hwnd);
        io.MouseDown[button] = true;
        return 0;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    {
        int button = 0;
        if (msg == WM_LBUTTONUP) button = 0;
        if (msg == WM_RBUTTONUP) button = 1;
        if (msg == WM_MBUTTONUP) button = 2;
        io.MouseDown[button] = false;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
            ::ReleaseCapture();
        return 0;
    }
    case WM_MOUSEWHEEL:
        io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
        return 0;
    case WM_MOUSEHWHEEL:
        io.MouseWheelH += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
        return 0;
    case WM_MOUSEMOVE:
        io.MousePos.x = (signed short)(lParam);
        io.MousePos.y = (signed short)(lParam >> 16);
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam < 256)
            io.KeysDown[wParam] = 1;
        return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wParam < 256)
            io.KeysDown[wParam] = 0;
        return 0;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000)
            io.AddInputCharacter((unsigned short)wParam);
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && ImGui_ImplWin32_UpdateMouseCursor())
            return 1;
        return 0;
    }
    return 0;
}

static void ImGui_ImplDX10_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();

    // Build
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Create DX10 texture
    {
        D3D10_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D10_USAGE_DEFAULT;
        desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D10Texture2D *pTexture = NULL;
        D3D10_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = pixels;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

        // Create texture view
        D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
        ZeroMemory(&srv_desc, sizeof(srv_desc));
        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srv_desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = desc.MipLevels;
        srv_desc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srv_desc, &g_pFontTextureView);
        pTexture->Release();
    }

    // Store our identifier
    io.Fonts->TexID = (void *)g_pFontTextureView;

    // Create texture sampler
    {
        D3D10_SAMPLER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D10_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
        desc.MipLODBias = 0.f;
        desc.ComparisonFunc = D3D10_COMPARISON_ALWAYS;
        desc.MinLOD = 0.f;
        desc.MaxLOD = 0.f;
        g_pd3dDevice->CreateSamplerState(&desc, &g_pFontSampler);
    }
}

bool    ImGui_ImplDX10_CreateDeviceObjects()
{
    if (!g_pd3dDevice)
        return false;
    if (g_pFontSampler)
        ImGui_ImplDX10_InvalidateDeviceObjects();

    // By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
    // If you would like to use this DX11 sample code but remove this dependency you can: 
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

        D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &g_pVertexShaderBlob, NULL);
        if (g_pVertexShaderBlob == NULL) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
            return false;
        if (g_pd3dDevice->CreateVertexShader((DWORD*)g_pVertexShaderBlob->GetBufferPointer(), g_pVertexShaderBlob->GetBufferSize(), &g_pVertexShader) != S_OK)
            return false;

        // Create the input layout
        D3D10_INPUT_ELEMENT_DESC local_layout[] = 
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->pos), D3D10_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->uv),  D3D10_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)(&((ImDrawVert*)0)->col), D3D10_INPUT_PER_VERTEX_DATA, 0 },
        };
        if (g_pd3dDevice->CreateInputLayout(local_layout, 3, g_pVertexShaderBlob->GetBufferPointer(), g_pVertexShaderBlob->GetBufferSize(), &g_pInputLayout) != S_OK)
            return false;

        // Create the constant buffer
        {
            D3D10_BUFFER_DESC desc;
            desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER);
            desc.Usage = D3D10_USAGE_DYNAMIC;
            desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;
            g_pd3dDevice->CreateBuffer(&desc, NULL, &g_pVertexConstantBuffer);
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

        D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &g_pPixelShaderBlob, NULL);
        if (g_pPixelShaderBlob == NULL)  // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
            return false;
        if (g_pd3dDevice->CreatePixelShader((DWORD*)g_pPixelShaderBlob->GetBufferPointer(), g_pPixelShaderBlob->GetBufferSize(), &g_pPixelShader) != S_OK)
            return false;
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
        desc.SrcBlendAlpha = D3D10_BLEND_INV_SRC_ALPHA;
        desc.DestBlendAlpha = D3D10_BLEND_ZERO;
        desc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
        desc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
        g_pd3dDevice->CreateBlendState(&desc, &g_pBlendState);
    }

    // Create the rasterizer state
    {
        D3D10_RASTERIZER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.FillMode = D3D10_FILL_SOLID;
        desc.CullMode = D3D10_CULL_NONE;
        desc.ScissorEnable = true;
        desc.DepthClipEnable = true;
        g_pd3dDevice->CreateRasterizerState(&desc, &g_pRasterizerState);
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
        g_pd3dDevice->CreateDepthStencilState(&desc, &g_pDepthStencilState);
    }

    ImGui_ImplDX10_CreateFontsTexture();

    return true;
}

void    ImGui_ImplDX10_InvalidateDeviceObjects()
{
    if (!g_pd3dDevice)
        return;

    if (g_pFontSampler) { g_pFontSampler->Release(); g_pFontSampler = NULL; }
    if (g_pFontTextureView) { g_pFontTextureView->Release(); g_pFontTextureView = NULL; ImGui::GetIO().Fonts->TexID = NULL; } // We copied g_pFontTextureView to io.Fonts->TexID so let's clear that as well.
    if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
    if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }

    if (g_pBlendState) { g_pBlendState->Release(); g_pBlendState = NULL; }
    if (g_pDepthStencilState) { g_pDepthStencilState->Release(); g_pDepthStencilState = NULL; }
    if (g_pRasterizerState) { g_pRasterizerState->Release(); g_pRasterizerState = NULL; }
    if (g_pPixelShader) { g_pPixelShader->Release(); g_pPixelShader = NULL; }
    if (g_pPixelShaderBlob) { g_pPixelShaderBlob->Release(); g_pPixelShaderBlob = NULL; }
    if (g_pVertexConstantBuffer) { g_pVertexConstantBuffer->Release(); g_pVertexConstantBuffer = NULL; }
    if (g_pInputLayout) { g_pInputLayout->Release(); g_pInputLayout = NULL; }
    if (g_pVertexShader) { g_pVertexShader->Release(); g_pVertexShader = NULL; }
    if (g_pVertexShaderBlob) { g_pVertexShaderBlob->Release(); g_pVertexShaderBlob = NULL; }
}

bool    ImGui_ImplDX10_Init(void* hwnd, ID3D10Device* device)
{
    g_hWnd = (HWND)hwnd;
    g_pd3dDevice = device;

    if (!QueryPerformanceFrequency((LARGE_INTEGER *)&g_TicksPerSecond))
        return false;
    if (!QueryPerformanceCounter((LARGE_INTEGER *)&g_Time))
        return false;

    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;   // We can honor GetMouseCursor() values
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;    // We can honor io.WantSetMousePos requests (optional, rarely used)

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Space] = VK_SPACE;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';

    io.ImeWindowHandle = g_hWnd;

    return true;
}

void ImGui_ImplDX10_Shutdown()
{
    ImGui_ImplDX10_InvalidateDeviceObjects();
    g_pd3dDevice = NULL;
    g_hWnd = (HWND)0;
}

void ImGui_ImplDX10_NewFrame()
{
    if (!g_pFontSampler)
        ImGui_ImplDX10_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    RECT rect;
    GetClientRect(g_hWnd, &rect);
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
    io.KeySuper = false;
    // io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
    // io.MousePos : filled by WM_MOUSEMOVE events
    // io.MouseDown : filled by WM_*BUTTON* events
    // io.MouseWheel : filled by WM_MOUSEWHEEL events

    // Set OS mouse position if requested (only used when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos)
    {
        POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
        ClientToScreen(g_hWnd, &pos);
        SetCursorPos(pos.x, pos.y);
    }

    // Update OS mouse cursor with the cursor requested by imgui
    ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (g_LastMouseCursor != mouse_cursor)
    {
        g_LastMouseCursor = mouse_cursor;
        ImGui_ImplWin32_UpdateMouseCursor();
    }

    // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
    ImGui::NewFrame();
}
