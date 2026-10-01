// dear imgui: compile/link test for the C++20 backend modules that need a GPU/display to run:
// 'imgui.impl.opengl2', 'imgui.impl.opengl3', 'imgui.impl.glut', 'imgui.impl.glfw', 'imgui.impl.vulkan'.
// Checks that their API is exported, usable from an importer, and links (not run).

#include <GLFW/glfw3.h>         // application-side platform headers
#include <vulkan/vulkan.h>
import std;
import imgui;
import imgui.impl.opengl2;
import imgui.impl.opengl3;
import imgui.impl.glut;
import imgui.impl.glfw;
import imgui.impl.vulkan;

// Exported structs are complete types
static_assert(sizeof(ImGui_ImplVulkan_InitInfo) > 0);
static_assert(sizeof(ImGui_ImplVulkanH_Window) > 0);
static_assert(sizeof(ImGui_ImplOpenGL3_RenderState) > 0);
static_assert(std::is_same_v<decltype(&ImGui_ImplGlfw_InitForOpenGL), bool (*)(GLFWwindow*, bool)>);

void RenderFrameOpenGL3(GLFWwindow* window)
{
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RenderFrameOpenGL2()
{
    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL2_Init();
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

void RenderFrameVulkan(VkInstance instance, VkDevice device, VkQueue queue, VkCommandBuffer command_buffer)
{
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_3;
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
