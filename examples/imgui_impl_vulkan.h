// ImGui Renderer for: Vulkan
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// Missing features:
//  [ ] Renderer: User texture binding. Changes of ImTextureID aren't supported by this binding! See https://github.com/ocornut/imgui/pull/914

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <vulkan/vulkan.h>

#define IMGUI_VK_QUEUED_FRAMES 2

struct ImGui_ImplVulkan_InitInfo
{
    VkInstance                      Instance;
    VkPhysicalDevice                PhysicalDevice;
    VkDevice                        Device;
    uint32_t                        QueueFamily;
    VkQueue                         Queue;
    VkPipelineCache                 PipelineCache;
    VkDescriptorPool                DescriptorPool;
    const VkAllocationCallbacks*    Allocator;
    void                            (*CheckVkResultFn)(VkResult err);
};

IMGUI_IMPL_API bool     ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass);
IMGUI_IMPL_API void     ImGui_ImplVulkan_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplVulkan_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer command_buffer);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API void     ImGui_ImplVulkan_InvalidateFontUploadObjects();
IMGUI_IMPL_API void     ImGui_ImplVulkan_InvalidateDeviceObjects();
IMGUI_IMPL_API bool     ImGui_ImplVulkan_CreateFontsTexture(VkCommandBuffer command_buffer);
IMGUI_IMPL_API bool     ImGui_ImplVulkan_CreateDeviceObjects();

//-------------------------------------------------------------------------
// Miscellaneous Vulkan Helpers
// Generally we try to NOT provide any kind of superfluous high-level helpers in the examples. 
// But for the upcoming multi-viewport feature, the Vulkan will need this code anyway, so we decided to shared it and use it in the examples' main.cpp
// If your application/engine already has code to create all that data (swap chain, render pass, frame buffers, etc.) you can ignore all of this.
//-------------------------------------------------------------------------
// NB: Those functions do NOT use any of the state used/affected by the regular ImGui_ImplVulkan_XXX functions.
//-------------------------------------------------------------------------

struct ImGui_ImplVulkanH_FrameData;
struct ImGui_ImplVulkanH_WindowData;

IMGUI_IMPL_API void                 ImGui_ImplVulkanH_CreateWindowDataCommandBuffers(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_family, ImGui_ImplVulkanH_WindowData* wd, const VkAllocationCallbacks* allocator);
IMGUI_IMPL_API void                 ImGui_ImplVulkanH_CreateWindowDataSwapChainAndFramebuffer(VkPhysicalDevice physical_device, VkDevice device, ImGui_ImplVulkanH_WindowData* wd, const VkAllocationCallbacks* allocator, int w, int h);
IMGUI_IMPL_API void                 ImGui_ImplVulkanH_DestroyWindowData(VkInstance instance, VkDevice device, ImGui_ImplVulkanH_WindowData* wd, const VkAllocationCallbacks* allocator);
IMGUI_IMPL_API VkSurfaceFormatKHR   ImGui_ImplVulkanH_SelectSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkFormat* request_formats, int request_formats_count, VkColorSpaceKHR request_color_space);
IMGUI_IMPL_API VkPresentModeKHR     ImGui_ImplVulkanH_SelectPresentMode(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkPresentModeKHR* request_modes, int request_modes_count);
IMGUI_IMPL_API int                  ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(VkPresentModeKHR present_mode);

struct ImGui_ImplVulkanH_FrameData
{
    uint32_t            BackbufferIndex;    // keep track of recently rendered swapchain frame indices
    VkCommandPool       CommandPool;
    VkCommandBuffer     CommandBuffer;
    VkFence             Fence;
    VkSemaphore         ImageAcquiredSemaphore;
    VkSemaphore         RenderCompleteSemaphore;

    IMGUI_IMPL_API ImGui_ImplVulkanH_FrameData();
};

struct ImGui_ImplVulkanH_WindowData
{
    int                 Width;
    int                 Height;
    VkSwapchainKHR      Swapchain;
    VkSurfaceKHR        Surface;
    VkSurfaceFormatKHR  SurfaceFormat;
    VkPresentModeKHR    PresentMode;
    VkRenderPass        RenderPass;
    bool                ClearEnable;
    VkClearValue        ClearValue;
    uint32_t            BackBufferCount;
    VkImage             BackBuffer[16];
    VkImageView         BackBufferView[16];
    VkFramebuffer       Framebuffer[16];
    uint32_t            FrameIndex;
    ImGui_ImplVulkanH_FrameData Frames[IMGUI_VK_QUEUED_FRAMES];

    IMGUI_IMPL_API ImGui_ImplVulkanH_WindowData();
};

