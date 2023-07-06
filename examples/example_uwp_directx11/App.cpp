#include "pch.h"

#include <imgui.h>
#include <imgui_impl_uwp.h>
#include <imgui_impl_dx11.h>

#include <d3d11.h>
#include <dxgi1_4.h>

using namespace winrt;

using namespace Windows;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Windows::UI::Core;

struct App : implements<App, IFrameworkViewSource, IFrameworkView>
{
    bool m_windowClosed = false;
    bool m_windowVisible = true;

    com_ptr<ID3D11Device> m_pd3dDevice;
    com_ptr<ID3D11DeviceContext> m_pd3dDeviceContext;
    com_ptr<IDXGISwapChain1> m_pSwapChain;
    com_ptr<ID3D11RenderTargetView> m_mainRenderTargetView;

    UINT m_ResizeWidth = 0, m_ResizeHeight = 0;
    Windows::Foundation::Size m_OldSize;

    IFrameworkView CreateView()
    {
        return *this;
    }

    void Initialize(CoreApplicationView const &)
    {
    }

    void Load(hstring const&)
    {
    }

    void Uninitialize()
    {
    }

    void Run()
    {
        CoreWindow window = CoreWindow::GetForCurrentThread();
        CoreDispatcher dispatcher = window.Dispatcher();
        window.Activate();

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplUwp_InitForCurrentView();
        ImGui_ImplDX11_Init(m_pd3dDevice.get(), m_pd3dDeviceContext.get());

        // Our state
        bool show_demo_window = true;
        bool show_another_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        while (!m_windowClosed)
        {
            if (m_windowVisible)
            {
                dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

                // Handle window resize (we don't resize directly in the WM_SIZE handler)
                if (m_ResizeWidth != 0 && m_ResizeHeight != 0)
                {
                    CleanupRenderTarget();
                    m_pSwapChain->ResizeBuffers(2, lround(m_ResizeWidth), lround(m_ResizeHeight), DXGI_FORMAT_B8G8R8A8_UNORM, 0);
                    m_ResizeWidth = m_ResizeHeight = 0;
                    CreateRenderTarget();
                }

                // Start the Dear ImGui frame
                ImGui_ImplDX11_NewFrame();
                ImGui_ImplUwp_NewFrame();
                ImGui::NewFrame();

                // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
                if (show_demo_window)
                    ImGui::ShowDemoWindow(&show_demo_window);

                // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
                {
                    static float f = 0.0f;
                    static int counter = 0;

                    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

                    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
                    ImGui::Checkbox("Another Window", &show_another_window);

                    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
                    ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                        counter++;
                    ImGui::SameLine();
                    ImGui::Text("counter = %d", counter);

                    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                    ImGui::End();
                }

                // 3. Show another simple window.
                if (show_another_window)
                {
                    ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                    ImGui::Text("Hello from another window!");
                    if (ImGui::Button("Close Me"))
                        show_another_window = false;
                    ImGui::End();
                }

                // Rendering
                ImGui::Render();

                auto renderTargetView = m_mainRenderTargetView.get();
                const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

                m_pd3dDeviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);
                m_pd3dDeviceContext->ClearRenderTargetView(renderTargetView, clear_color_with_alpha);
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

                m_pSwapChain->Present(1, 0);
            }
            else
            {
                dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
            }
        }

        // Cleanup
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplUwp_Shutdown();
        ImGui::DestroyContext();
    }

    void SetWindow(CoreWindow const & window)
    {
        window.Closed({ this, &App::OnWindowClosed });
        window.SizeChanged({ this, &App::OnSizeChanged });
        window.VisibilityChanged({ this, &App::OnVisibiltyChanged });

        m_ResizeWidth = window.Bounds().Width;
        m_ResizeHeight = window.Bounds().Height;

        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };

        D3D_FEATURE_LEVEL featureLevel;

        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            0,
            0,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            m_pd3dDevice.put(),
            &featureLevel,
            m_pd3dDeviceContext.put()
        ); 

        if (FAILED(hr))
        {
            winrt::check_hresult(
                D3D11CreateDevice(
                    nullptr,
                    D3D_DRIVER_TYPE_WARP,
                    0,
                    0,
                    featureLevels,
                    ARRAYSIZE(featureLevels),
                    D3D11_SDK_VERSION,
                    m_pd3dDevice.put(),
                    &featureLevel,
                    m_pd3dDeviceContext.put()
                )
            );
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

        swapChainDesc.Width = lround(m_ResizeWidth);
        swapChainDesc.Height = lround(m_ResizeHeight);
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.Flags = 0;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        // This sequence obtains the DXGI factory that was used to create the Direct3D device above.
        com_ptr<IDXGIDevice3> dxgiDevice;
        m_pd3dDevice.as(dxgiDevice);

        com_ptr<IDXGIAdapter> dxgiAdapter;
        winrt::check_hresult(
            dxgiDevice->GetAdapter(dxgiAdapter.put())
        );

        com_ptr<IDXGIFactory4> dxgiFactory;
        winrt::check_hresult(
            dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
        );

        winrt::check_hresult(
            dxgiFactory->CreateSwapChainForCoreWindow(
                m_pd3dDevice.get(),
                winrt::get_unknown(window),
                &swapChainDesc,
                nullptr,
                m_pSwapChain.put()
            )
        );

        winrt::check_hresult(
            dxgiDevice->SetMaximumFrameLatency(1)
        );

        CreateRenderTarget();

        m_ResizeWidth = m_ResizeHeight = 0;
    }

    void OnSizeChanged(IInspectable const &, WindowSizeChangedEventArgs const & args)
    {
        auto size = args.Size();

        // Prevent unnecessary resize
        if (size != m_OldSize)
        {
            m_ResizeWidth = size.Width;
            m_ResizeHeight = size.Height;
        }

        m_OldSize = size;
    }

    void OnVisibiltyChanged(IInspectable const&, VisibilityChangedEventArgs const& args)
    {
        m_windowVisible = args.Visible();
    }

    void OnWindowClosed(IInspectable const&, CoreWindowEventArgs const& args)
    {
        m_windowClosed = true;
    }

    void CreateRenderTarget()
    {
        com_ptr<ID3D11Texture2D> pBackBuffer;
        m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(pBackBuffer.put()));
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer.get(), nullptr, m_mainRenderTargetView.put());
    }

    void CleanupRenderTarget()
    {
        if (m_mainRenderTargetView.get()) { m_mainRenderTargetView->Release(); }
    }
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    CoreApplication::Run(make<App>());
}
