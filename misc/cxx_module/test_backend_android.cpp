// dear imgui: compile/link test for the C++20 backend modules for Android:
// 'imgui.impl.android', 'imgui.impl.opengl3' (OpenGL ES 3), 'imgui.impl.vulkan'.
// Checks that their API is exported, usable from an importer, and links (not run).

#include <android/native_window.h>  // application-side platform headers
#include <android/input.h>
#include <vulkan/vulkan.h>
import std;
import imgui;
import imgui.impl.android;
import imgui.impl.opengl3;
import imgui.impl.vulkan;

static_assert(std::is_same_v<decltype(&ImGui_ImplAndroid_HandleInputEvent), std::int32_t (*)(const AInputEvent*)>);
static_assert(sizeof(ImGui_ImplVulkan_InitInfo) > 0);

void RenderFrameOpenGL3(ANativeWindow* window, const AInputEvent* input_event)
{
    ImGui_ImplAndroid_Init(window);
    ImGui_ImplOpenGL3_Init("#version 300 es");
    ImGui_ImplAndroid_HandleInputEvent(input_event);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RenderFrameVulkan(VkInstance instance, VkDevice device, VkQueue queue, VkCommandBuffer command_buffer)
{
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_1;
    init_info.Instance = instance;
    init_info.PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(instance);
    init_info.Device = device;
    init_info.Queue = queue;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
}

int main()
{
    return 0;
}
