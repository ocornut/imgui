// ImGui SDL binding with Vulkan + shaders
// FIXME: Changes of ImTextureID aren't supported by this binding! Please, someone add it!

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 5 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXX_CreateFontsTexture(), ImGui_ImplXXXX_NewFrame(), ImGui_ImplXXXX_Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

struct SDL_Window;

#define IMGUI_VK_QUEUED_FRAMES 2

struct ImGui_ImplSdlVulkan_Init_Data
{
    VkAllocationCallbacks* allocator;
    VkPhysicalDevice       gpu;
    VkDevice               device;
    VkRenderPass           render_pass;
    VkPipelineCache        pipeline_cache;
    VkDescriptorPool       descriptor_pool;
    void (*check_vk_result)(VkResult err);
};

IMGUI_API bool        ImGui_ImplSdlVulkan_Init(SDL_Window* window, bool install_callbacks, ImGui_ImplSdlVulkan_Init_Data *init_data);
IMGUI_API void        ImGui_ImplSdlVulkan_Shutdown();
IMGUI_API void        ImGui_ImplSdlVulkan_NewFrame(SDL_Window* window);
IMGUI_API bool        ImGui_ImplSdlVulkan_ProcessEvent(SDL_Event* event);
IMGUI_API void        ImGui_ImplSdlVulkan_Render(VkCommandBuffer command_buffer);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplSdlVulkan_InvalidateFontUploadObjects();
IMGUI_API void        ImGui_ImplSdlVulkan_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplSdlVulkan_CreateFontsTexture(VkCommandBuffer command_buffer);
IMGUI_API bool        ImGui_ImplSdlVulkan_CreateDeviceObjects();

