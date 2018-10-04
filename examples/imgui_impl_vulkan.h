// dear imgui: Renderer for Vulkan
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// Missing features:
//  [ ] Renderer: User texture binding. Changes of ImTextureID aren't supported by this binding! See https://github.com/ocornut/imgui/pull/914

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// The aim of imgui_impl_vulkan.h/.cpp is to be usable in your engine without any modification. 
// IF YOU FEEL YOU NEED TO MAKE ANY CHANGE TO THIS CODE, please share them and your feedback at https://github.com/ocornut/imgui/

#include <vulkan/vulkan.h>

#define IMGUI_VK_QUEUED_FRAMES      2

// Please zero-clear before use.
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

// Called by user code
IMGUI_IMPL_API bool     ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass);
IMGUI_IMPL_API void     ImGui_ImplVulkan_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplVulkan_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer command_buffer);
IMGUI_IMPL_API bool     ImGui_ImplVulkan_CreateFontsTexture(VkCommandBuffer command_buffer);
IMGUI_IMPL_API void     ImGui_ImplVulkan_InvalidateFontUploadObjects();

// Called by ImGui_ImplVulkan_Init() might be useful elsewhere.
IMGUI_IMPL_API bool     ImGui_ImplVulkan_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplVulkan_InvalidateDeviceObjects();


//-------------------------------------------------------------------------
// Internal / Miscellaneous Vulkan Helpers
//-------------------------------------------------------------------------
// You probably do NOT need to use or care about those functions. 
// Those functions only exist because:
//   1) they facilitate the readability and maintenance of the multiple main.cpp examples files.
//   2) the upcoming multi-viewport feature will need them internally.
// Generally we avoid exposing any kind of superfluous high-level helpers in the bindings, 
// but it is too much code to duplicate everywhere so we exceptionally expose them.
// Your application/engine will likely already have code to setup all that stuff (swap chain, render pass, frame buffers, etc.).
// You may read this code to learn about Vulkan, but it is recommended you use you own custom tailored code to do equivalent work.
// (those functions do not interact with any of the state used by the regular ImGui_ImplVulkan_XXX functions)
//-------------------------------------------------------------------------

struct ImGui_ImplVulkanH_FrameData;
struct ImGui_ImplVulkanH_WindowData;

IMGUI_IMPL_API void                 ImGui_ImplVulkanH_CreateWindowDataCommandBuffers(VkPhysicalDevice physical_device, VkDevice device, uint32_t queue_family, ImGui_ImplVulkanH_WindowData* wd, const VkAllocationCallbacks* allocator);
IMGUI_IMPL_API void                 ImGui_ImplVulkanH_CreateWindowDataSwapChainAndFramebuffer(VkPhysicalDevice physical_device, VkDevice device, ImGui_ImplVulkanH_WindowData* wd, const VkAllocationCallbacks* allocator, int w, int h);
IMGUI_IMPL_API void                 ImGui_ImplVulkanH_DestroyWindowData(VkInstance instance, VkDevice device, ImGui_ImplVulkanH_WindowData* wd, const VkAllocationCallbacks* allocator);
IMGUI_IMPL_API VkSurfaceFormatKHR   ImGui_ImplVulkanH_SelectSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkFormat* request_formats, int request_formats_count, VkColorSpaceKHR request_color_space);
IMGUI_IMPL_API VkPresentModeKHR     ImGui_ImplVulkanH_SelectPresentMode(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkPresentModeKHR* request_modes, int request_modes_count);
IMGUI_IMPL_API int                  ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(VkPresentModeKHR present_mode);

// Helper structure to hold the data needed by one rendering frame
struct ImGui_ImplVulkanH_FrameData
{
    uint32_t            BackbufferIndex;        // Keep track of recently rendered swapchain frame indices
    VkCommandPool       CommandPool;
    VkCommandBuffer     CommandBuffer;
    VkFence             Fence;
    VkSemaphore         ImageAcquiredSemaphore;
    VkSemaphore         RenderCompleteSemaphore;

    IMGUI_IMPL_API ImGui_ImplVulkanH_FrameData();
};

// Helper structure to hold the data needed by one rendering context into one OS window
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

