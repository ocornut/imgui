// ImGui - standalone example application for Glfw + Vulkan, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>

#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui_impl_glfw_vulkan.h"

#define IMGUI_MAX_POSSIBLE_BACK_BUFFERS 16
#define IMGUI_UNLIMITED_FRAME_RATE
//#ifdef _DEBUG
//#define IMGUI_VULKAN_DEBUG_REPORT
//#endif

static VkAllocationCallbacks*   g_Allocator = NULL;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkSurfaceKHR             g_Surface = VK_NULL_HANDLE;
static VkPhysicalDevice         g_Gpu = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static VkSwapchainKHR           g_Swapchain = VK_NULL_HANDLE;
static VkRenderPass             g_RenderPass = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = 0;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT g_Debug_Report = VK_NULL_HANDLE;

static VkSurfaceFormatKHR       g_SurfaceFormat;
static VkImageSubresourceRange  g_ImageRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
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

static void resize_vulkan(GLFWwindow* /*window*/, int w, int h)
{
    VkResult err;
    VkSwapchainKHR old_swapchain = g_Swapchain;
    err = vkDeviceWaitIdle(g_Device);
    check_vk_result(err);

    // Destroy old Framebuffer:
    for (uint32_t i = 0; i < g_BackBufferCount; i++)
        if (g_BackBufferView[i])
            vkDestroyImageView(g_Device, g_BackBufferView[i], g_Allocator);
    for (uint32_t i = 0; i < g_BackBufferCount; i++)
        if (g_Framebuffer[i])
            vkDestroyFramebuffer(g_Device, g_Framebuffer[i], g_Allocator);
    if (g_RenderPass)
        vkDestroyRenderPass(g_Device, g_RenderPass, g_Allocator);

    // Create Swapchain:
    {
        VkSwapchainCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = g_Surface;
        info.imageFormat = g_SurfaceFormat.format;
        info.imageColorSpace = g_SurfaceFormat.colorSpace;
        info.imageArrayLayers = 1;
        info.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = g_PresentMode;
        info.clipped = VK_TRUE;
        info.oldSwapchain = old_swapchain;
        VkSurfaceCapabilitiesKHR cap;
        err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_Gpu, g_Surface, &cap);
        check_vk_result(err);
        if (cap.maxImageCount > 0)
            info.minImageCount = (cap.minImageCount + 2 < cap.maxImageCount) ? (cap.minImageCount + 2) : cap.maxImageCount;
        else
            info.minImageCount = cap.minImageCount + 2;

        if (cap.currentExtent.width == 0xffffffff)
        {
            fb_width = w;
            fb_height = h;
            info.imageExtent.width = fb_width;
            info.imageExtent.height = fb_height;
        }
        else
        {
            fb_width = cap.currentExtent.width;
            fb_height = cap.currentExtent.height;
            info.imageExtent.width = fb_width;
            info.imageExtent.height = fb_height;
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

    // Create the Render Pass:
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
        info.subresourceRange = g_ImageRange;
        for (uint32_t i = 0; i < g_BackBufferCount; i++)
        {
            info.image = g_BackBuffer[i];
            err = vkCreateImageView(g_Device, &info, g_Allocator, &g_BackBufferView[i]);
            check_vk_result(err);
        }
    }

    // Create Framebuffer:
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
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    printf("[vulkan] ObjectType: %i\nMessage: %s\n\n", objectType, pMessage );
    return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT

static void setup_vulkan(GLFWwindow* window)
{
    VkResult err;

    // Create Vulkan Instance
    {
        uint32_t extensions_count;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledExtensionCount = extensions_count;
        create_info.ppEnabledExtensionNames = glfw_extensions;

#ifdef IMGUI_VULKAN_DEBUG_REPORT
        // enabling multiple validation layers grouped as lunarg standard validation
        const char* layers[] = {"VK_LAYER_LUNARG_standard_validation"};
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;

        // need additional storage for char pointer to debug report extension
        const char** extensions = (const char**)malloc(sizeof(const char*) * (extensions_count + 1));
        for (size_t i = 0; i < extensions_count; i++)
            extensions[i] = glfw_extensions[i];
        extensions[ extensions_count ] = "VK_EXT_debug_report";
        create_info.enabledExtensionCount = extensions_count+1;
        create_info.ppEnabledExtensionNames = extensions;
#endif // IMGUI_VULKAN_DEBUG_REPORT

        err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        check_vk_result(err);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
        free(extensions);

        // create the debug report callback
        VkDebugReportCallbackCreateInfoEXT debug_report_ci ={};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = NULL;
        
        // get the proc address of the function pointer, required for used extensions
        PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = 
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");

        err = vkCreateDebugReportCallbackEXT( g_Instance, &debug_report_ci, g_Allocator, &g_Debug_Report );
        check_vk_result(err);
#endif // IMGUI_VULKAN_DEBUG_REPORT
    }

    // Create Window Surface
    {
        err = glfwCreateWindowSurface(g_Instance, window, g_Allocator, &g_Surface);
        check_vk_result(err);
    }

    // Get GPU
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
        g_Gpu = gpus[0];
        free(gpus);
    }

    // Get queue
    {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(g_Gpu, &count, NULL);
        VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
        vkGetPhysicalDeviceQueueFamilyProperties(g_Gpu, &count, queues);
        for (uint32_t i = 0; i < count; i++)
        {
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                g_QueueFamily = i;
                break;
            }
        }
        free(queues);
    }

    // Check for WSI support
    {
        VkBool32 res;
        vkGetPhysicalDeviceSurfaceSupportKHR(g_Gpu, g_QueueFamily, g_Surface, &res);
        if (res != VK_TRUE)
        {
            fprintf(stderr, "Error no WSI support on physical device 0\n");
            exit(-1);
        }
    }

    // Get Surface Format
    {
        // Per Spec Format and View Format are expected to be the same unless VK_IMAGE_CREATE_MUTABLE_BIT was set at image creation
        // Assuming that the default behavior is without setting this bit, there is no need for separate Spawchain image and image view format
        // additionally several new color spaces were introduced with Vulkan Spec v1.0.40
        // hence we must make sure that a format with the mostly available color space, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, is found and used
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(g_Gpu, g_Surface, &count, NULL);
        VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(g_Gpu, g_Surface, &count, formats);

        // first check if only one format, VK_FORMAT_UNDEFINED, is available, which would imply that any format is available
        if (count == 1)
        {
            if( formats[0].format == VK_FORMAT_UNDEFINED )
            {
                g_SurfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
                g_SurfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            }
            else
            {   // no point in searching another format
                g_SurfaceFormat = formats[0];
            }
        }
        else
        {
            // request several formats, the first found will be used 
            VkFormat requestSurfaceImageFormat[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
            VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            bool requestedFound = false;
            for (size_t i = 0; i < sizeof(requestSurfaceImageFormat) / sizeof(requestSurfaceImageFormat[0]); i++)
            {
                if( requestedFound ) {
                    break;
                }
                for (uint32_t j = 0; j < count; j++)
                {
                    if (formats[j].format == requestSurfaceImageFormat[i] && formats[j].colorSpace == requestSurfaceColorSpace)
                    {
                        g_SurfaceFormat = formats[j];
                        requestedFound = true;
                    }
                }
            }

            // if none of the requested image formats could be found, use the first available
            if (!requestedFound)
                g_SurfaceFormat = formats[0];
        }
        free(formats);
    }


    // Get Present Mode
    {
        // Requst a certain mode and confirm that it is available. If not use VK_PRESENT_MODE_FIFO_KHR which is mandatory
#ifdef IMGUI_UNLIMITED_FRAME_RATE
        g_PresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
#else
        g_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
#endif
        uint32_t count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(g_Gpu, g_Surface, &count, nullptr);
        VkPresentModeKHR* presentModes = (VkPresentModeKHR*)malloc(sizeof(VkQueueFamilyProperties) * count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(g_Gpu, g_Surface, &count, presentModes);
        bool presentModeAvailable = false;
        for (size_t i = 0; i < count; i++) 
        {
            if (presentModes[i] == g_PresentMode)
            {
                presentModeAvailable = true;
                break;
            }
        }
        if( !presentModeAvailable )
            g_PresentMode = VK_PRESENT_MODE_FIFO_KHR;   // always available
    }


    // Create Logical Device
    {
        int device_extension_count = 1;
        const char* device_extensions[] = {"VK_KHR_swapchain"};
        const uint32_t queue_index = 0;
        const uint32_t queue_count = 1;
        const float queue_priority[] = {1.0f};
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = queue_count;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info)/sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = device_extension_count;
        create_info.ppEnabledExtensionNames = device_extensions;
        err = vkCreateDevice(g_Gpu, &create_info, g_Allocator, &g_Device);
        check_vk_result(err);
        vkGetDeviceQueue(g_Device, g_QueueFamily, queue_index, &g_Queue);
    }

    // Create Framebuffers
    {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        resize_vulkan(window, w, h);
        glfwSetFramebufferSizeCallback(window, resize_vulkan);
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
    // get the proc address of the function pointer, required for used extensions
    auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT(g_Instance, g_Debug_Report, g_Allocator);
#endif // IMGUI_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

static void frame_begin()
{
    VkResult err;
    while (true)
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

    VkSwapchainKHR swapchains[1] = {g_Swapchain};
    uint32_t indices[1] = {g_BackbufferIndices[PresentIndex]};
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

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui Vulkan example", NULL, NULL);

    // Setup Vulkan
    if (!glfwVulkanSupported())
    {
        printf("GLFW: Vulkan Not Supported\n");
        return 1;
    }
    setup_vulkan(window);

    // Setup ImGui binding
    ImGui_ImplGlfwVulkan_Init_Data init_data = {};
    init_data.allocator = g_Allocator;
    init_data.gpu = g_Gpu;
    init_data.device = g_Device;
    init_data.render_pass = g_RenderPass;
    init_data.pipeline_cache = g_PipelineCache;
    init_data.descriptor_pool = g_DescriptorPool;
    init_data.check_vk_result = check_vk_result;
    ImGui_ImplGlfwVulkan_Init(window, true, &init_data);

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

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

        ImGui_ImplGlfwVulkan_CreateFontsTexture(g_CommandBuffer[g_FrameIndex]);

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
        ImGui_ImplGlfwVulkan_InvalidateFontUploadObjects();
    }

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);

    // When IMGUI_UNLIMITED_FRAME_RATE is defined we render into latest image acquired from the swapchain but we display the image which was rendered before.
    // Hence we must render once and increase the g_FrameIndex without presenting, which we do before entering the render loop.
    // This is also the reason why frame_end() is split into frame_end() and frame_present(), the later one not being called here.
#ifdef IMGUI_UNLIMITED_FRAME_RATE
    ImGui_ImplGlfwVulkan_NewFrame();
    frame_begin();
    ImGui_ImplGlfwVulkan_Render(g_CommandBuffer[g_FrameIndex]);
    frame_end();
    g_FrameIndex = (g_FrameIndex + 1) % IMGUI_VK_QUEUED_FRAMES;
#endif // IMGUI_UNLIMITED_FRAME_RATE

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfwVulkan_NewFrame();

        // 1. Show a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        {
            static float f = 0.0f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            if (ImGui::Button("Test Window")) show_test_window ^= 1;
            if (ImGui::Button("Another Window")) show_another_window ^= 1;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
            ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiCond_FirstUseEver);
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }

        g_ClearValue.color.float32[0] = clear_color.x;
        g_ClearValue.color.float32[1] = clear_color.y;
        g_ClearValue.color.float32[2] = clear_color.z;
        g_ClearValue.color.float32[3] = clear_color.w;

        frame_begin();
        ImGui_ImplGlfwVulkan_Render(g_CommandBuffer[g_FrameIndex]);
        frame_end();
        frame_present();
    }

    // Cleanup
    VkResult err = vkDeviceWaitIdle(g_Device);
    check_vk_result(err);
    ImGui_ImplGlfwVulkan_Shutdown();
    cleanup_vulkan();
    glfwTerminate();

    return 0;
}
