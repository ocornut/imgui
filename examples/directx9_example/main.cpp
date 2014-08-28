#include <windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "../../imgui.h"

#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strdup

static HWND hWnd;
static LPDIRECT3D9             g_pD3D = NULL;       // Used to create the D3DDevice
static LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // Our rendering device
static LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL;        // Buffer to hold vertices
static LPDIRECT3DTEXTURE9      g_pTexture = NULL;   // Our texture
struct CUSTOMVERTEX
{
    D3DXVECTOR3 position;
    D3DCOLOR    color;
    float       tu, tv;
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

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
    CUSTOMVERTEX* vtx_dst;
    if (g_pVB->Lock(0, total_vtx_count, (void**)&vtx_dst, D3DLOCK_DISCARD) < 0)
        return;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        const ImDrawVert* vtx_src = &cmd_list->vtx_buffer[0];
        for (size_t i = 0; i < cmd_list->vtx_buffer.size(); i++)
        {
            vtx_dst->position.x = vtx_src->pos.x;
            vtx_dst->position.y = vtx_src->pos.y;
            vtx_dst->position.z = 0.0f;
            vtx_dst->color = (vtx_src->col & 0xFF00FF00) | ((vtx_src->col & 0xFF0000)>>16) | ((vtx_src->col & 0xFF) << 16);     // RGBA --> ARGB for DirectX9
            vtx_dst->tu = vtx_src->uv.x;
            vtx_dst->tv = vtx_src->uv.y;
            vtx_dst++;
            vtx_src++;
        }
    }
    g_pVB->Unlock();

    g_pd3dDevice->SetStreamSource( 0, g_pVB, 0, sizeof( CUSTOMVERTEX ) );
    g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );

    // Setup render state: alpha-blending, no face culling, no depth testing
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, false );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, false );
    g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
    g_pd3dDevice->SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
    g_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE, false );
    g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    g_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, true );

    // Setup texture
    g_pd3dDevice->SetTexture( 0, g_pTexture );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

    // Setup orthographic projection matrix
    D3DXMATRIXA16 mat;
    D3DXMatrixIdentity(&mat);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &mat);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &mat);
    D3DXMatrixOrthoOffCenterLH(&mat, 0.5f, ImGui::GetIO().DisplaySize.x+0.5f, ImGui::GetIO().DisplaySize.y+0.5f, 0.5f, -1.0f, +1.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &mat);

    // Render command lists
    int vtx_offset = 0;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        // Render command list
        const ImDrawList* cmd_list = cmd_lists[n];
        const ImDrawCmd* pcmd_end = cmd_list->commands.end();
        for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
        {
            const RECT r = { (LONG)pcmd->clip_rect.x, (LONG)pcmd->clip_rect.y, (LONG)pcmd->clip_rect.z, (LONG)pcmd->clip_rect.w };
            g_pd3dDevice->SetScissorRect(&r);
            g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, vtx_offset, pcmd->vtx_count/3);
            vtx_offset += pcmd->vtx_count;
        }
    }
}

HRESULT InitD3D(HWND hWnd)
{
    if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
        return E_FAIL;

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    // Create the D3DDevice
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice) < 0)
        return E_FAIL;

    return S_OK;
}

void Cleanup()
{
    if (g_pTexture != NULL)
        g_pTexture->Release();

    if (g_pd3dDevice != NULL)
        g_pd3dDevice->Release();

    if (g_pD3D != NULL)
        g_pD3D->Release();
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        // Mouse wheel: -1,0,+1
        io.MouseWheel = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1 : -1;
        return true;
    case WM_MOUSEMOVE:
        // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
        io.MousePos.x = (signed short)(lParam);
        io.MousePos.y = (signed short)(lParam >> 16); 
        return true;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 1 && wParam < 256)
            io.AddInputCharacter((char)wParam);
        return true;
    case WM_DESTROY:
        {
            Cleanup();
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void InitImGui()
{
    RECT rect;
    GetClientRect(hWnd, &rect);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));  // Display size, in pixels. For clamping windows positions.
    io.DeltaTime = 1.0f/60.0f;                                                                  // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
    io.PixelCenterOffset = 0.0f;                                                                // Align Direct3D Texels
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;                                                           // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
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
    if (g_pd3dDevice->CreateVertexBuffer(10000 * sizeof(CUSTOMVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVB, NULL) < 0)
    {
        IM_ASSERT(0);
        return;
    }

    // Load font texture
    const void* png_data;
    unsigned int png_size;
    ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size);
    if (D3DXCreateTextureFromFileInMemory(g_pd3dDevice, png_data, png_size, &g_pTexture) < 0)
    {
        IM_ASSERT(0);
        return;
    }
}

INT64 ticks_per_second = 0;
INT64 time = 0;

void UpdateImGui()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup timestep
    INT64 current_time;
    QueryPerformanceCounter((LARGE_INTEGER *)&current_time); 
    io.DeltaTime = (float)(current_time - time) / ticks_per_second;
    time = current_time;

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
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
    RegisterClassEx(&wc);

    // Create the application's window
    hWnd = CreateWindow(L"ImGui Example", L"ImGui DirectX9 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    if (!QueryPerformanceFrequency((LARGE_INTEGER *)&ticks_per_second))
        return 1;
    if (!QueryPerformanceCounter((LARGE_INTEGER *)&time))
        return 1;

    // Initialize Direct3D
    if (InitD3D(hWnd) < 0)
    {
        if (g_pVB)
            g_pVB->Release();
        UnregisterClass(L"ImGui Example", wc.hInstance);
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

        // Create a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        static bool show_test_window = true;
        static bool show_another_window = false;
        static float f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        show_test_window ^= ImGui::Button("Test Window");
        show_another_window ^= ImGui::Button("Another Window");

        // Calculate and show framerate
        static float ms_per_frame[120] = { 0 };
        static int ms_per_frame_idx = 0;
        static float ms_per_frame_accum = 0.0f;
        ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
        ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
        ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];
        ms_per_frame_idx = (ms_per_frame_idx + 1) % 120;
        const float ms_per_frame_avg = ms_per_frame_accum / 120;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_per_frame_avg, 1000.0f / ms_per_frame_avg);

        // Show the ImGui test window
        // Most of user example code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNewWindowDefaultPos(ImVec2(650, 20));     // Normally user code doesn't need/want to call it because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
            ImGui::ShowTestWindow(&show_test_window);
        }

        // Show another simple window
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window, ImVec2(200,100));
            ImGui::Text("Hello");
            ImGui::End();
        }

        // Rendering
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(204, 153, 153), 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            g_pd3dDevice->EndScene();
        }
        g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    }

    ImGui::Shutdown();

    if (g_pVB)
        g_pVB->Release();

    UnregisterClass(L"ImGui Example", wc.hInstance);
    return 0;
}
