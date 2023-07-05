#pragma once

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_uwp.h>
#include "Common\DeviceResources.h"

// Renders Direct2D and 3D content on the screen.
namespace example_uwp_directx11
{
	class example_uwp_directx11Main : public DX::IDeviceNotify
	{
	public:
		example_uwp_directx11Main(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~example_uwp_directx11Main();
		void CreateWindowSizeDependentResources();
		bool Render();

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

        ImGuiIO io;
        bool show_demo_window = true;
        bool show_another_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	};
}
