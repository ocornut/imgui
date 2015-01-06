// ImGui - standalone example application for DirectX 11

#include <windows.h>
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../shared/stb_image.h"    // for .png loading
#include "../../imgui.h"

// DirectX 11
#include <d3d11.h>
#include <d3dcompiler.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strdup

static HWND                     hWnd;
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceImmediateContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11Buffer*            g_pVB = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView;

static ID3D10Blob *             g_pVertexShaderBlob = NULL;
static ID3D11VertexShader*      g_pVertexShader = NULL;
static ID3D11InputLayout*       g_pInputLayout = NULL;
static ID3D11Buffer*            g_pVertexConstantBuffer = NULL;

static ID3D10Blob *             g_pPixelShaderBlob = NULL;
static ID3D11PixelShader*       g_pPixelShader = NULL;

static ID3D11ShaderResourceView*g_pFontTextureView = NULL;
static ID3D11SamplerState*      g_pFontSampler = NULL;
static ID3D11BlendState*        g_blendState = NULL;

struct CUSTOMVERTEX
{
    float        pos[2];
    float        uv[2];
    unsigned int col;
};

struct VERTEX_CONSTANT_BUFFER
{
    float        mvp[4][4];
};

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
// - try adjusting ImGui::GetIO().PixelCenterOffset to 0.5f or 0.375f
static void ImImpl_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
    size_t total_vtx_count = 0;
    for (int n = 0; n < cmd_lists_count; n++)
        total_vtx_count += cmd_lists[n]->vtx_buffer.size();
    if (total_vtx_count == 0)
        return;

    // Copy and convert all vertices into a single contiguous buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (g_pd3dDeviceImmediateContext->Map(g_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) != S_OK)
        return;
    CUSTOMVERTEX* vtx_dst = (CUSTOMVERTEX*)mappedResource.pData;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        const ImDrawVert* vtx_src = &cmd_list->vtx_buffer[0];
        for (size_t i = 0; i < cmd_list->vtx_buffer.size(); i++)
        {
            vtx_dst->pos[0] = vtx_src->pos.x;
            vtx_dst->pos[1] = vtx_src->pos.y;
            vtx_dst->uv[0] = vtx_src->uv.x;
            vtx_dst->uv[1] = vtx_src->uv.y;
            vtx_dst->col = vtx_src->col;
            vtx_dst++;
            vtx_src++;
        }
    }
    g_pd3dDeviceImmediateContext->Unmap(g_pVB, 0);

    // Setup orthographic projection matrix into our constant buffer
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (g_pd3dDeviceImmediateContext->Map(g_pVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) != S_OK)
            return;

        VERTEX_CONSTANT_BUFFER* pConstantBuffer = (VERTEX_CONSTANT_BUFFER*)mappedResource.pData;
        const float L = 0.0f;
        const float R = ImGui::GetIO().DisplaySize.x;
        const float B = ImGui::GetIO().DisplaySize.y;
        const float T = 0.0f;
        const float mvp[4][4] = 
        {
            { 2.0f/(R-L),   0.0f,           0.0f,       0.0f},
            { 0.0f,         2.0f/(T-B),     0.0f,       0.0f,},
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
        };
        memcpy(&pConstantBuffer->mvp, mvp, sizeof(mvp));
        g_pd3dDeviceImmediateContext->Unmap(g_pVertexConstantBuffer, 0);
    }
    
    // Setup viewport
    {
        D3D11_VIEWPORT vp;
        memset(&vp, 0, sizeof(D3D11_VIEWPORT));
        vp.Width = ImGui::GetIO().DisplaySize.x;
        vp.Height = ImGui::GetIO().DisplaySize.y;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        g_pd3dDeviceImmediateContext->RSSetViewports(1, &vp);
    }

    // Bind shader and vertex buffers
    unsigned int stride = sizeof(CUSTOMVERTEX);
    unsigned int offset = 0;
    g_pd3dDeviceImmediateContext->IASetInputLayout(g_pInputLayout);
    g_pd3dDeviceImmediateContext->IASetVertexBuffers(0, 1, &g_pVB, &stride, &offset);
    g_pd3dDeviceImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    g_pd3dDeviceImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
    g_pd3dDeviceImmediateContext->VSSetConstantBuffers(0, 1, &g_pVertexConstantBuffer);

    g_pd3dDeviceImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
    g_pd3dDeviceImmediateContext->PSSetShaderResources(0, 1, &g_pFontTextureView);
    g_pd3dDeviceImmediateContext->PSSetSamplers(0, 1, &g_pFontSampler);

    // Setup render state
    const float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    g_pd3dDeviceImmediateContext->OMSetBlendState(g_blendState, blendFactor, 0xffffffff);

    // Render command lists
    int vtx_offset = 0;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        // Render command list
        const ImDrawList* cmd_list = cmd_lists[n];
        for (size_t cmd_i = 0; cmd_i < cmd_list->commands.size(); cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->commands[cmd_i];
            const D3D11_RECT r = { (LONG)pcmd->clip_rect.x, (LONG)pcmd->clip_rect.y, (LONG)pcmd->clip_rect.z, (LONG)pcmd->clip_rect.w };
            g_pd3dDeviceImmediateContext->RSSetScissorRects(1, &r); 
            g_pd3dDeviceImmediateContext->Draw(pcmd->vtx_count, vtx_offset);
            vtx_offset += pcmd->vtx_count;
        }
    }

    // Restore modified state
    g_pd3dDeviceImmediateContext->IASetInputLayout(NULL);
    g_pd3dDeviceImmediateContext->PSSetShader(NULL, NULL, 0);
    g_pd3dDeviceImmediateContext->VSSetShader(NULL, NULL, 0);
}

HRESULT InitDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = (UINT)ImGui::GetIO().DisplaySize.x;
        sd.BufferDesc.Height = (UINT)ImGui::GetIO().DisplaySize.y;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceImmediateContext) != S_OK)
        return E_FAIL;

    // Setup rasterizer
    {
        D3D11_RASTERIZER_DESC RSDesc;
        memset(&RSDesc, 0, sizeof(D3D11_RASTERIZER_DESC));
        RSDesc.FillMode = D3D11_FILL_SOLID;
        RSDesc.CullMode = D3D11_CULL_NONE;
        RSDesc.FrontCounterClockwise = FALSE;
        RSDesc.DepthBias = 0;
        RSDesc.SlopeScaledDepthBias = 0.0f;
        RSDesc.DepthBiasClamp = 0;
        RSDesc.DepthClipEnable = TRUE;
        RSDesc.ScissorEnable = TRUE;
        RSDesc.AntialiasedLineEnable = FALSE;
        RSDesc.MultisampleEnable = (sd.SampleDesc.Count > 1) ? TRUE : FALSE;

        ID3D11RasterizerState* pRState = NULL;
        g_pd3dDevice->CreateRasterizerState(&RSDesc, &pRState);
        g_pd3dDeviceImmediateContext->RSSetState(pRState);
        pRState->Release();
    }

    // Create the render target
    {
        ID3D11Texture2D* pBackBuffer;               
        D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
        ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
        render_target_view_desc.Format = sd.BufferDesc.Format;
        render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &g_mainRenderTargetView);
        g_pd3dDeviceImmediateContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        pBackBuffer->Release();
    }

    // Create the vertex shader
    {
        static const char* vertexShader = 
            "cbuffer vertexBuffer : register(c0) \
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
            return E_FAIL;
        if (g_pd3dDevice->CreateVertexShader((DWORD*)g_pVertexShaderBlob->GetBufferPointer(), g_pVertexShaderBlob->GetBufferSize(), NULL, &g_pVertexShader) != S_OK)
            return E_FAIL;

        // Create the input layout
        D3D11_INPUT_ELEMENT_DESC localLayout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (size_t)(&((CUSTOMVERTEX*)0)->pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, (size_t)(&((CUSTOMVERTEX*)0)->col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, (size_t)(&((CUSTOMVERTEX*)0)->uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        if (g_pd3dDevice->CreateInputLayout(localLayout, 3, g_pVertexShaderBlob->GetBufferPointer(), g_pVertexShaderBlob->GetBufferSize(), &g_pInputLayout) != S_OK)
            return E_FAIL;

        // Create the constant buffer
        {
            D3D11_BUFFER_DESC cbDesc;
            cbDesc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER);
            cbDesc.Usage = D3D11_USAGE_DYNAMIC;
            cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            cbDesc.MiscFlags = 0;
            g_pd3dDevice->CreateBuffer(&cbDesc, NULL, &g_pVertexConstantBuffer);
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
                float4 out_col = texture0.Sample(sampler0, input.uv);\
                return input.col * out_col;\
            }";

        D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &g_pPixelShaderBlob, NULL);
        if (g_pPixelShaderBlob == NULL)  // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
            return E_FAIL;
        if (g_pd3dDevice->CreatePixelShader((DWORD*)g_pPixelShaderBlob->GetBufferPointer(), g_pPixelShaderBlob->GetBufferSize(), NULL, &g_pPixelShader) != S_OK)
            return E_FAIL;
    }

    // Create the blending setup
    {
        D3D11_BLEND_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.AlphaToCoverageEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        g_pd3dDevice->CreateBlendState(&desc, &g_blendState);
    }

    return S_OK;
}

void CleanupDevice()
{
    if (g_pd3dDeviceImmediateContext) g_pd3dDeviceImmediateContext->ClearState();

    // InitImGui
    if (g_pFontSampler) g_pFontSampler->Release();
    if (g_pFontTextureView) g_pFontTextureView->Release();
    if (g_pVB) g_pVB->Release();

    // InitDeviceD3D
    if (g_blendState) g_blendState->Release(); 
    if (g_pPixelShader) g_pPixelShader->Release();
    if (g_pPixelShaderBlob) g_pPixelShaderBlob->Release();
    if (g_pVertexConstantBuffer) g_pVertexConstantBuffer->Release();
    if (g_pInputLayout) g_pInputLayout->Release();
    if (g_pVertexShader) g_pVertexShader->Release();
    if (g_pVertexShaderBlob) g_pVertexShaderBlob->Release();
    if (g_mainRenderTargetView) g_mainRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pd3dDeviceImmediateContext) g_pd3dDeviceImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
        io.MousePos.x = (signed short)(lParam);
        io.MousePos.y = (signed short)(lParam >> 16); 
        return true;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000)
            io.AddInputCharacter((unsigned short)wParam);
        return true;
    case WM_DESTROY:
        CleanupDevice();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void InitImGui()
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    int display_w = (int)(rect.right - rect.left);
    int display_h = (int)(rect.bottom - rect.top);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)display_w, (float)display_h);    // Display size, in pixels. For clamping windows positions.
    io.DeltaTime = 1.0f/60.0f;                                      // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our time step is variable)
    io.PixelCenterOffset = 0.0f;                                    // Align Direct3D Texels
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;                               // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_UP;
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
    io.RenderDrawListsFn = ImImpl_RenderDrawLists;

    // Create the vertex buffer
    {
        D3D11_BUFFER_DESC bufferDesc;
        memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.ByteWidth = 10000 * sizeof(CUSTOMVERTEX);
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bufferDesc.MiscFlags = 0;
        if (g_pd3dDevice->CreateBuffer(&bufferDesc, NULL, &g_pVB) < 0)
        {
            IM_ASSERT(0);
            return;
        }
    }

    // Load font texture
    // Default font (embedded in code)
    const void* png_data;
    unsigned int png_size;
    ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size);
    int tex_x, tex_y, tex_comp;
    void* tex_data = stbi_load_from_memory((const unsigned char*)png_data, (int)png_size, &tex_x, &tex_y, &tex_comp, 0);
    IM_ASSERT(tex_data != NULL);

    {
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = tex_x;
        desc.Height = tex_y;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D *pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = tex_data;
        subResource.SysMemPitch = tex_x * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &g_pFontTextureView);
        pTexture->Release();
    }

    // Create texture sampler
    {
        D3D11_SAMPLER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MipLODBias = 0.f;
        desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        desc.MinLOD = 0.f;
        desc.MaxLOD = 0.f;
        g_pd3dDevice->CreateSamplerState(&desc, &g_pFontSampler);
    }
}

INT64 ticks_per_second = 0;
INT64 last_time = 0;

void UpdateImGui()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup time step
    INT64 current_time;
    QueryPerformanceCounter((LARGE_INTEGER *)&current_time); 
    io.DeltaTime = (float)(current_time - last_time) / ticks_per_second;
    last_time = current_time;

    // Setup inputs
    // (we already got mouse position, buttons, wheel from the window message callback)
    BYTE keystate[256];
    GetKeyboardState(keystate);
    for (int i = 0; i < 256; i++)
        io.KeysDown[i] = (keystate[i] & 0x80) != 0;
    io.KeyCtrl = (keystate[VK_CONTROL] & 0x80) != 0;
    io.KeyShift = (keystate[VK_SHIFT] & 0x80) != 0;
    // io.MousePos : filled by WM_MOUSEMOVE event
    // io.MouseDown : filled by WM_*BUTTON* events
    // io.MouseWheel : filled by WM_MOUSEWHEEL events

    // Start the frame
    ImGui::NewFrame();
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int)
{
    // Register the window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, "ImGui Example", NULL };
    RegisterClassEx(&wc);

    // Create the application's window
    hWnd = CreateWindow("ImGui Example", "ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    if (!QueryPerformanceFrequency((LARGE_INTEGER *)&ticks_per_second))
        return 1;
    if (!QueryPerformanceCounter((LARGE_INTEGER *)&last_time))
        return 1;

    // Initialize Direct3D
    if (InitDeviceD3D(hWnd) < 0)
    {
        CleanupDevice();
        UnregisterClass("ImGui Example", wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    InitImGui();

    // Enter the message loop
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
        
        UpdateImGui();

        static bool show_test_window = true;
        static bool show_another_window = false;

        // 1. Show a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        {
            static float f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            show_test_window ^= ImGui::Button("Test Window");
            show_another_window ^= ImGui::Button("Another Window");

            // Calculate and show frame rate
            static float ms_per_frame[120] = { 0 };
            static int ms_per_frame_idx = 0;
            static float ms_per_frame_accum = 0.0f;
            ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
            ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
            ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];
            ms_per_frame_idx = (ms_per_frame_idx + 1) % 120;
            const float ms_per_frame_avg = ms_per_frame_accum / 120;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_per_frame_avg, 1000.0f / ms_per_frame_avg);
        }

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window, ImVec2(200,100));
            ImGui::Text("Hello");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCondition_FirstUseEver);     // Normally user code doesn't need/want to call it because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
            ImGui::ShowTestWindow(&show_test_window);
        }

        // Rendering
        float clearColor[4] = { 204 / 255.f, 153 / 255.f, 153 / 255.f };
        g_pd3dDeviceImmediateContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
        ImGui::Render();
        g_pSwapChain->Present(0, 0);
    }

    ImGui::Shutdown();
    UnregisterClass("ImGui Example", wc.hInstance);

    return 0;
}
