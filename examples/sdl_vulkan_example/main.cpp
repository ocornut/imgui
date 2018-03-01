// ImGui - standalone example application for SDL2 + Vulkan
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include "imgui.h"
#include "../imgui_impl_sdl2.h"
#include "../imgui_impl_vulkan.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

// FIXME-VULKAN: Resizing with IMGUI_UNLIMITED_FRAME_RATE triggers errors from the validation layer.
#define IMGUI_MAX_POSSIBLE_BACK_BUFFERS 16
#define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

static VkAllocationCallbacks*   g_Allocator = NULL;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkSurfaceKHR             g_Surface = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static VkSwapchainKHR           g_Swapchain = VK_NULL_HANDLE;
static VkRenderPass             g_RenderPass = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = (uint32_t)-1;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;

static VkSurfaceFormatKHR       g_SurfaceFormat;
static VkPresentModeKHR         g_PresentMode;

static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static int                      fb_width, fb_height;
static uint32_t                 g_BackbufferIndices[IMGUI_VK_QUEUED_FRAMES];    // keep track of recently rendered swapchain frame indices
static uint32_t                 g_BackBufferCount = 0;
static VkImage                  g_BackBuffer[IMGUI_MAX_POSSIBLE_BACK_BUFFERS] = {};
static VkImageView              g_BackBufferView[IMGUI_MAX_POSSIBLE_BACK_BUFFERS] = {};
static VkFramebuffer            g_Framebuffer[IMGUI_MAX_POSSIBLE_BACK_BUFFERS] = {};

static uint32_t                 g_FrameIndex = 0;
static VkCommandPool            g_CommandPool[IMGUI_VK_QUEUED_FRAMES];
static VkCommandBuffer          g_CommandBuffer[IMGUI_VK_QUEUED_FRAMES];
static VkFence                  g_Fence[IMGUI_VK_QUEUED_FRAMES];
static VkSemaphore              g_PresentCompleteSemaphore[IMGUI_VK_QUEUED_FRAMES];
static VkSemaphore              g_RenderCompleteSemaphore[IMGUI_VK_QUEUED_FRAMES];

static VkClearValue             g_ClearValue = {};

static void check_vk_result(VkResult err)
{
    if (err == 0) return;
    printf("VkResult %d\n", err);
    if (err < 0)
        abort();
}

static void CreateOrResizeSwapChainAndFrameBuffer(int w, int h)
{
    VkResult err;
    VkSwapchainKHR old_swapchain = g_Swapchain;
    err = vkDeviceWaitIdle(g_Device);
    check_vk_result(err);

    // Destroy old Framebuffer
    for (uint32_t i = 0; i < g_BackBufferCount; i++)
    {
        if (g_BackBufferView[i])
            vkDestroyImageView(g_Device, g_BackBufferView[i], g_Allocator);
        if (g_Framebuffer[i])
            vkDestroyFramebuffer(g_Device, g_Framebuffer[i], g_Allocator);
    }
    if (g_RenderPass)
        vkDestroyRenderPass(g_Device, g_RenderPass, g_Allocator);

    // Create Swapchain
    {
        VkSwapchainCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = g_Surface;
        info.imageFormat = g_SurfaceFormat.format;
        info.imageColorSpace = g_SurfaceFormat.colorSpace;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
        info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = g_PresentMode;
        info.clipped = VK_TRUE;
        info.oldSwapchain = old_swapchain;
        VkSurfaceCapabilitiesKHR cap;
        err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_PhysicalDevice, g_Surface, &cap);
        check_vk_result(err);
        if (cap.maxImageCount > 0)
            info.minImageCount = (cap.minImageCount + 2 < cap.maxImageCount) ? (cap.minImageCount + 2) : cap.maxImageCount;
        else
            info.minImageCount = cap.minImageCount + 2;

        if (cap.currentExtent.width == 0xffffffff)
        {
            info.imageExtent.width = fb_width = w;
            info.imageExtent.height = fb_height = h;
        }
        else
        {
            info.imageExtent.width = fb_width = cap.currentExtent.width;
            info.imageExtent.height = fb_height = cap.currentExtent.height;
        }
        err = vkCreateSwapchainKHR(g_Device, &info, g_Allocator, &g_Swapchain);
        check_vk_result(err);
        err = vkGetSwapchainImagesKHR(g_Device, g_Swapchain, &g_BackBufferCount, NULL);
        check_vk_result(err);
        err = vkGetSwapchainImagesKHR(g_Device, g_Swapchain, &g_BackBufferCount, g_BackBuffer);
        check_vk_result(err);
    }
    if (old_swapchain)
        vkDestroySwapchainKHR(g_Device, old_swapchain, g_Allocator);

    // Create the Render Pass
    {
        VkAttachmentDescription attachment = {};
        attachment.format = g_SurfaceFormat.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;
        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        err = vkCreateRenderPass(g_Device, &info, g_Allocator, &g_RenderPass);
        check_vk_result(err);
    }

    // Create The Image Views
    {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = g_SurfaceFormat.format;
        info.components.r = VK_COMPONENT_SWIZZLE_R;
        info.components.g = VK_COMPONENT_SWIZZLE_G;
        info.components.b = VK_COMPONENT_SWIZZLE_B;
        info.components.a = VK_COMPONENT_SWIZZLE_A;
        VkImageSubresourceRange image_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        info.subresourceRange = image_range;
        for (uint32_t i = 0; i < g_BackBufferCount; i++)
        {
            info.image = g_BackBuffer[i];
            err = vkCreateImageView(g_Device, &info, g_Allocator, &g_BackBufferView[i]);
            check_vk_result(err);
        }
    }

    // Create Framebuffer
    {
        VkImageView attachment[1];
        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = g_RenderPass;
        info.attachmentCount = 1;
        info.pAttachments = attachment;
        info.width = fb_width;
        info.height = fb_height;
        info.layers = 1;
        for (uint32_t i = 0; i < g_BackBufferCount; i++)
        {
            attachment[0] = g_BackBufferView[i];
            err = vkCreateFramebuffer(g_Device, &info, g_Allocator, &g_Framebuffer[i]);
            check_vk_result(err);
        }
    }
}

#ifdef IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguemnts
    printf("[vulkan] ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT

static void setup_vulkan(SDL_Window* window, const char** extensions, uint32_t extensions_count)
{
    VkResult err;

    // Create Vulkan Instance
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledExtensionCount = extensions_count;
        create_info.ppEnabledExtensionNames = extensions;

#ifdef IMGUI_VULKAN_DEBUG_REPORT
        // Enabling multiple validation layers grouped as LunarG standard validation
        const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;

        // Enable debug report extension (we need additional storage, so we duplicate the user array to add our new extension to it)
        const char** extensions_ext = (const char**)malloc(sizeof(const char*) * (extensions_count + 1));
        memcpy(extensions_ext, extensions, extensions_count * sizeof(const char*));
        extensions_ext[extensions_count] = "VK_EXT_debug_report";
        create_info.enabledExtensionCount = extensions_count + 1;
        create_info.ppEnabledExtensionNames = extensions_ext;
#endif // IMGUI_VULKAN_DEBUG_REPORT

        err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        check_vk_result(err);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
        free(extensions_ext);

        // Get the function pointer (required for any extensions)
        auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
        IM_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

        // Setup the debug report callback
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = NULL;
        err = vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
        check_vk_result(err);
#endif // IMGUI_VULKAN_DEBUG_REPORT
    }

    // Create Window Surface (with SDL)
    {
        SDL_bool result = SDL_Vulkan_CreateSurface(window, g_Instance, &g_Surface);
        if (result == 0) 
        {
            printf("failed to create Vulkan surface\n");
            abort();
        }
    }

    // Select GPU
    {
        uint32_t gpu_count;
        err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, NULL);
        check_vk_result(err);

        VkPhysicalDevice* gpus = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpu_count);
        err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus);
        check_vk_result(err);

        // If a number >1 of GPUs got reported, you should find the best fit GPU for your purpose
        // e.g. VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU if available, or with the greatest memory available, etc.
        // for sake of simplicity we'll just take the first one, assuming it has a graphics queue family.
        g_PhysicalDevice = gpus[0];
        free(gpus);
    }

    // Select graphics queue family
    {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, NULL);
        VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
        vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues);
        for (uint32_t i = 0; i < count; i++)
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                g_QueueFamily = i;
                break;
            }
        free(queues);
        IM_ASSERT(g_QueueFamily != -1);
    }

    // Check for WSI support
    {
        VkBool32 res;
        vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, g_Surface, &res);
        if (res != VK_TRUE)
        {
            fprintf(stderr, "Error no WSI support on physical device 0\n");
            exit(-1);
        }
    }

    // Get Surface Format
    {
        const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
        const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        g_SurfaceFormat = ImGui_ImplVulkan_SelectSurfaceFormat(g_PhysicalDevice, g_Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);
    }


    // Get Present Mode
    {
#ifdef IMGUI_UNLIMITED_FRAME_RATE
        VkPresentModeKHR present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
#else
        VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
#endif
        g_PresentMode = ImGui_ImplVulkan_SelectPresentMode(g_PhysicalDevice, g_Surface, &present_mode, 1);
    }


    // Create Logical Device (with 1 queue)
    {
        int device_extension_count = 1;
        const char* device_extensions[] = { "VK_KHR_swapchain" };
        const float queue_priority[] = { 1.0f };
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = device_extension_count;
        create_info.ppEnabledExtensionNames = device_extensions;
        err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
        check_vk_result(err);
        vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
    }


    // Create Framebuffers
    {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        CreateOrResizeSwapChainAndFrameBuffer(w, h);
    }


    // Create Command Buffers
    for (int i = 0; i < IMGUI_VK_QUEUED_FRAMES; i++)
    {
        {
            VkCommandPoolCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            info.queueFamilyIndex = g_QueueFamily;
            err = vkCreateCommandPool(g_Device, &info, g_Allocator, &g_CommandPool[i]);
            check_vk_result(err);
        }
        {
            VkCommandBufferAllocateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            info.commandPool = g_CommandPool[i];
            info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            info.commandBufferCount = 1;
            err = vkAllocateCommandBuffers(g_Device, &info, &g_CommandBuffer[i]);
            check_vk_result(err);
        }
        {
            VkFenceCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            err = vkCreateFence(g_Device, &info, g_Allocator, &g_Fence[i]);
            check_vk_result(err);
        }
        {
            VkSemaphoreCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            err = vkCreateSemaphore(g_Device, &info, g_Allocator, &g_PresentCompleteSemaphore[i]);
            check_vk_result(err);
            err = vkCreateSemaphore(g_Device, &info, g_Allocator, &g_RenderCompleteSemaphore[i]);
            check_vk_result(err);
        }
    }

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize pool_size[11] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * 11;
        pool_info.poolSizeCount = 11;
        pool_info.pPoolSizes = pool_size;
        err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
        check_vk_result(err);
    }
}

static void cleanup_vulkan()
{
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);
    for (int i = 0; i < IMGUI_VK_QUEUED_FRAMES; i++)
    {
        vkDestroyFence(g_Device, g_Fence[i], g_Allocator);
        vkFreeCommandBuffers(g_Device, g_CommandPool[i], 1, &g_CommandBuffer[i]);
        vkDestroyCommandPool(g_Device, g_CommandPool[i], g_Allocator);
        vkDestroySemaphore(g_Device, g_PresentCompleteSemaphore[i], g_Allocator);
        vkDestroySemaphore(g_Device, g_RenderCompleteSemaphore[i], g_Allocator);
    }
    for (uint32_t i = 0; i < g_BackBufferCount; i++)
    {
        vkDestroyImageView(g_Device, g_BackBufferView[i], g_Allocator);
        vkDestroyFramebuffer(g_Device, g_Framebuffer[i], g_Allocator);
    }
    vkDestroyRenderPass(g_Device, g_RenderPass, g_Allocator);
    vkDestroySwapchainKHR(g_Device, g_Swapchain, g_Allocator);
    vkDestroySurfaceKHR(g_Instance, g_Surface, g_Allocator);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // IMGUI_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

static void frame_begin()
{
    VkResult err;
    for (;;)
    {
        err = vkWaitForFences(g_Device, 1, &g_Fence[g_FrameIndex], VK_TRUE, 100);
        if (err == VK_SUCCESS) break;
        if (err == VK_TIMEOUT) continue;
        check_vk_result(err);
    }
    {
        err = vkAcquireNextImageKHR(g_Device, g_Swapchain, UINT64_MAX, g_PresentCompleteSemaphore[g_FrameIndex], VK_NULL_HANDLE, &g_BackbufferIndices[g_FrameIndex]);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(g_Device, g_CommandPool[g_FrameIndex], 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(g_CommandBuffer[g_FrameIndex], &info);
        check_vk_result(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = g_RenderPass;
        info.framebuffer = g_Framebuffer[g_BackbufferIndices[g_FrameIndex]];
        info.renderArea.extent.width = fb_width;
        info.renderArea.extent.height = fb_height;
        info.clearValueCount = 1;
        info.pClearValues = &g_ClearValue;
        vkCmdBeginRenderPass(g_CommandBuffer[g_FrameIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
    }
}

static void frame_end()
{
    VkResult err;
    vkCmdEndRenderPass(g_CommandBuffer[g_FrameIndex]);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &g_PresentCompleteSemaphore[g_FrameIndex];
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &g_CommandBuffer[g_FrameIndex];
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &g_RenderCompleteSemaphore[g_FrameIndex];

        err = vkEndCommandBuffer(g_CommandBuffer[g_FrameIndex]);
        check_vk_result(err);
        err = vkResetFences(g_Device, 1, &g_Fence[g_FrameIndex]);
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &info, g_Fence[g_FrameIndex]);
        check_vk_result(err);
    }
}

static void frame_present()
{
    VkResult err;
    // If IMGUI_UNLIMITED_FRAME_RATE is defined we present the latest but one frame. Otherwise we present the latest rendered frame
#ifdef IMGUI_UNLIMITED_FRAME_RATE
    uint32_t PresentIndex = (g_FrameIndex + IMGUI_VK_QUEUED_FRAMES - 1) % IMGUI_VK_QUEUED_FRAMES;
#else
    uint32_t PresentIndex = g_FrameIndex;
#endif // IMGUI_UNLIMITED_FRAME_RATE

    VkSwapchainKHR swapchains[1] = { g_Swapchain };
    uint32_t indices[1] = { g_BackbufferIndices[PresentIndex] };
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &g_RenderCompleteSemaphore[PresentIndex];
    info.swapchainCount = 1;
    info.pSwapchains = swapchains;
    info.pImageIndices = indices;
    err = vkQueuePresentKHR(g_Queue, &info);
    check_vk_result(err);

    g_FrameIndex = (g_FrameIndex + 1) % IMGUI_VK_QUEUED_FRAMES;
}

int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return 1;
    }

    // Setup window
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_Window* window = SDL_CreateWindow("ImGui SDL2+Vulkan example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE);

    // Setup Vulkan
    uint32_t extensions_count;
    SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, NULL);
    const char** sdl_extensions = new const char*[extensions_count];
    SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, sdl_extensions);
    setup_vulkan(window, sdl_extensions, extensions_count);
    delete[] sdl_extensions;

    // Setup ImGui binding
    ImGui::CreateContext();

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Allocator = g_Allocator;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.RenderPass = g_RenderPass;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.CheckVkResultFn = check_vk_result;

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplSDL2_Init(window, NULL);

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them. 
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple. 
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Upload Fonts
    {
        VkResult err;
        err = vkResetCommandPool(g_Device, g_CommandPool[g_FrameIndex], 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(g_CommandBuffer[g_FrameIndex], &begin_info);
        check_vk_result(err);

        ImGui_ImplVulkan_CreateFontsTexture(g_CommandBuffer[g_FrameIndex]);

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &g_CommandBuffer[g_FrameIndex];
        err = vkEndCommandBuffer(g_CommandBuffer[g_FrameIndex]);
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &end_info, VK_NULL_HANDLE);
        check_vk_result(err);

        err = vkDeviceWaitIdle(g_Device);
        check_vk_result(err);
        ImGui_ImplVulkan_InvalidateFontUploadObjects();
    }

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // When IMGUI_UNLIMITED_FRAME_RATE is defined we render into latest image acquired from the swapchain but we display the image which was rendered before.
    // Hence we must render once and increase the g_FrameIndex without presenting, which we do before entering the render loop.
    // This is also the reason why frame_end() is split into frame_end() and frame_present(), the later one not being called here.
#ifdef IMGUI_UNLIMITED_FRAME_RATE
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    frame_begin();
    ImGui_ImplVulkan_Render(g_CommandBuffer[g_FrameIndex]);
    frame_end();
    g_FrameIndex = (g_FrameIndex + 1) % IMGUI_VK_QUEUED_FRAMES;
#endif // IMGUI_UNLIMITED_FRAME_RATE

    // Main loop
    bool done = false;
    while (!done)
    {
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED && event.window.windowID == SDL_GetWindowID(window)) 
                CreateOrResizeSwapChainAndFrameBuffer((int)event.window.data1, (int)event.window.data2);
        }
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);

        // 1. Show a simple window.
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
        {
            static float f = 0.0f;
            static int counter = 0;
            ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }

        // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name your windows.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
        if (show_demo_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        // Rendering
        memcpy(&g_ClearValue.color.float32[0], &clear_color, 4 * sizeof(float));
        frame_begin();
        ImGui_ImplVulkan_Render(g_CommandBuffer[g_FrameIndex]);
        frame_end();
        frame_present();
    }

    // Cleanup
    VkResult err = vkDeviceWaitIdle(g_Device);
    check_vk_result(err);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(window);
    cleanup_vulkan();
    SDL_Quit();

    return 0;
}
