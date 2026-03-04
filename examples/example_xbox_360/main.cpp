#include <xtl.h>
#include <stdio.h>

#define IMGUI_USE_BGRA_PACKED_COLOR

#include "imgui_impl_xdk_dx9.h"
#include "imgui_impl_xbox_360.h"

static Direct3D* g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static bool                     g_DeviceLost = false;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

BOOL   g_bWidescreen = TRUE;

bool CreateDeviceD3D();
void CleanupDeviceD3D();

void __cdecl main()
{
    OutputDebugString("Starting Dear ImGui Direct3D9 Example for Xbox 360...\n");

    if (!CreateDeviceD3D())
    {
        OutputDebugString("Failed to create Direct3D9 device!\n");
        CleanupDeviceD3D();
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2(
        (float)g_d3dpp.BackBufferWidth,
        (float)g_d3dpp.BackBufferHeight
    );

    {
        char buf[64];
        sprintf(buf, "DisplaySize: %.0f x %.0f\n", io.DisplaySize.x, io.DisplaySize.y);
    }

    ImGui::StyleColorsDark();

    // FIX: plataforma (input) antes do renderer
    if (!ImGui_ImplXbox360_Init())
    {
        OutputDebugString("Failed to initialize Xbox 360 platform backend!\n");
        CleanupDeviceD3D();
        return;
    }

    if (!ImGui_ImplDX9_Init(g_pd3dDevice))
    {
        OutputDebugString("Failed to initialize DX9 renderer backend!\n");
        CleanupDeviceD3D();
        return;
    }

    bool  show_demo_window = true;
    bool  show_another_window = false;
    bool  xbox_menu = false;
    bool  metrics = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    for (;;)
    {
        ImGui_ImplXbox360_NewFrame();   // 1. input + delta time
        ImGui_ImplDX9_NewFrame();       // 2. renderer (no-op mas necessário)
        ImGui::NewFrame();              // 3. começa o frame ImGui

        if (show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }
        
        {
            static float f = 0.0f;
            static int   counter = 0;

            ImGui::Begin("Hello, world!");
            ImGui::Text("This is some useful text.");
            ImGui::Checkbox("Demo Window", &show_demo_window);
            ImGui::Checkbox("Another Window", &show_another_window);
            ImGui::Checkbox("Xbox Menu", &xbox_menu);
            ImGui::Checkbox("Metrics", &metrics);
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            if (ImGui::Button("Button"))
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        if (xbox_menu)
        {
            ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
            ImGui::Begin("Xbox Especific features (Xbox menu)");
            {
                ImGui::BeginGroup();
                ImGui::Text("Show xbox input");
                ImGui::SameLine();
                ImGui::Spacing();
                ImGui::SameLine();
                ImGui::Button("Open");
                if (ImGui::IsItemActivated())
                {
                    ImGui_ImplXbox360_OpenKeyboard(L"Xbox keyboard", L"Test input", L"hello");
                }
                ImGui::EndGroup();
            }
            ImGui::End();
        }

        if (metrics)
        {
            ImGui::ShowMetricsWindow();
        }

        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA(
            (int)(clear_color.x * clear_color.w * 255.0f),
            (int)(clear_color.y * clear_color.w * 255.0f),
            (int)(clear_color.z * clear_color.w * 255.0f),
            (int)(clear_color.w * 255.0f)
        );
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);

        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            if (io.WantTextInput)
            {
                // [Test feature]
                // If the user wants text input, we can optionally open the system keyboard.
                // This is a convenient way to get text input on console without having to
                // implement your own on-screen keyboard. The downside is that it is a
                // modal operation and doesn't allow for per-character input events.
                //
                // You may want to customize this behavior by e.g. only opening the system
                // keyboard when a specific widget is active, or by implementing your own
                // non-modal keyboard UI that submits character events directly to ImGui.
                ImGui_ImplXbox360_OpenKeyboard(L"Input", L"Enter text:", L"");
            }
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }

        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
        if (result == D3DERR_DEVICELOST)
            g_DeviceLost = true;
    }

    if (g_pd3dDevice)
    {
        g_pd3dDevice->BlockUntilIdle();
    }
    CleanupDeviceD3D();
    OutputDebugString("Exiting through main()...\n");
}

bool CreateDeviceD3D()
{
    // FIX: atribui ao global g_pD3D, não a uma variável local perdida
    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_pD3D)
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));

    XVIDEO_MODE VideoMode;
    XGetVideoMode(&VideoMode);
    g_bWidescreen = VideoMode.fIsWideScreen;

    g_d3dpp.BackBufferWidth = min(VideoMode.dwDisplayWidth, 1280);
    g_d3dpp.BackBufferHeight = min(VideoMode.dwDisplayHeight, 720);
    g_d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    g_d3dpp.BackBufferCount = 1;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    if (FAILED(g_pD3D->CreateDevice(
        0, D3DDEVTYPE_HAL, NULL,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &g_d3dpp, &g_pd3dDevice)))
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplXbox360_Shutdown();
    ImGui::DestroyContext();

    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release();       g_pD3D = NULL; }
}
