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

static VkAllocationCallbacks*   g_Allocator = NULL;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkSurfaceKHR             g_Surface = VK_NULL_HANDLE;
static VkPhysicalDevice         g_Gpu = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static VkSwapchainKHR           g_Swapchain = VK_NULL_HANDLE;
static VkRenderPass             g_RenderPass = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = 0;
static VkQueue                  g_Queue = VK_NULL_HANDLE;

static VkFormat                 g_Format = VK_FORMAT_B8G8R8A8_UNORM;
static VkColorSpaceKHR          g_ColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
static VkImageSubresourceRange  g_ImageRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static int                      fb_width, fb_height;
static uint32_t                 g_BackBufferIndex = 0;
static uint32_t                 g_BackBufferCount = 0;
static VkImage                  g_BackBuffer[IMGUI_MAX_POSSIBLE_BACK_BUFFERS] = {};
static VkImageView              g_BackBufferView[IMGUI_MAX_POSSIBLE_BACK_BUFFERS] = {};
static VkFramebuffer            g_Framebuffer[IMGUI_MAX_POSSIBLE_BACK_BUFFERS] = {};

static uint32_t                 g_FrameIndex = 0;
static VkCommandPool            g_CommandPool[IMGUI_VK_QUEUED_FRAMES];
static VkCommandBuffer          g_CommandBuffer[IMGUI_VK_QUEUED_FRAMES];
static VkFence                  g_Fence[IMGUI_VK_QUEUED_FRAMES];
static VkSemaphore              g_Semaphore[IMGUI_VK_QUEUED_FRAMES];

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
    for (uint32_t i=0; i<g_BackBufferCount; i++)
        if (g_BackBufferView[i])
            vkDestroyImageView(g_Device, g_BackBufferView[i], g_Allocator);
     for(uint32_t i=0; i<g_BackBufferCount; i++)
        if (g_Framebuffer[i])
            vkDestroyFramebuffer(g_Device, g_Framebuffer[i], g_Allocator);
    if (g_RenderPass)
        vkDestroyRenderPass(g_Device, g_RenderPass, g_Allocator);

    // Create Swapchain:
    {
        VkSwapchainCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = g_Surface;
        info.imageFormat = g_Format;
        info.imageColorSpace = g_ColorSpace;
        info.imageArrayLayers = 1;
        info.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        info.clipped = VK_TRUE;
        info.oldSwapchain = old_swapchain;
        VkSurfaceCapabilitiesKHR cap;
        err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_Gpu, g_Surface, &cap);
        check_vk_result(err);
        info.minImageCount = (cap.minImageCount + 2 < cap.maxImageCount) ? (cap.minImageCount + 2) : cap.maxImageCount;
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
        attachment.format = g_Format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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
        info.format = g_Format;
        info.components.r = VK_COMPONENT_SWIZZLE_R;
        info.components.g = VK_COMPONENT_SWIZZLE_G;
        info.components.b = VK_COMPONENT_SWIZZLE_B;
        info.components.a = VK_COMPONENT_SWIZZLE_A;
        info.subresourceRange = g_ImageRange;
        for (uint32_t i = 0; i<g_BackBufferCount; i++)
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
        for (uint32_t i = 0; i<g_BackBufferCount; i++)
        {
            attachment[0] = g_BackBufferView[i];
            err = vkCreateFramebuffer(g_Device, &info, g_Allocator, &g_Framebuffer[i]);
            check_vk_result(err);
        }
    }
}

static void setup_vulkan(GLFWwindow* window)
{
    VkResult err;

    // Create Vulkan Instance
    {
        uint32_t glfw_extensions_count;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledExtensionCount = glfw_extensions_count;
        create_info.ppEnabledExtensionNames = glfw_extensions;
        err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        check_vk_result(err);
    }

    // Create Window Surface
    {
        err = glfwCreateWindowSurface(g_Instance, window, g_Allocator, &g_Surface);
        check_vk_result(err);
    }
    
    // Get GPU
    {
        uint32_t count = 1;
        err = vkEnumeratePhysicalDevices(g_Instance, &count, &g_Gpu);
        check_vk_result(err);
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
    for (int i=0; i<IMGUI_VK_QUEUED_FRAMES; i++)
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
            err = vkCreateSemaphore(g_Device, &info, g_Allocator, &g_Semaphore[i]);
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
        vkDestroySemaphore(g_Device, g_Semaphore[i], g_Allocator);
    }
    for (uint32_t i = 0; i < g_BackBufferCount; i++)
    {
        vkDestroyImageView(g_Device, g_BackBufferView[i], g_Allocator);
        vkDestroyFramebuffer(g_Device, g_Framebuffer[i], g_Allocator);
    }
    vkDestroyRenderPass(g_Device, g_RenderPass, g_Allocator);
    vkDestroySwapchainKHR(g_Device, g_Swapchain, g_Allocator);
    vkDestroySurfaceKHR(g_Instance, g_Surface, g_Allocator);
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
        err = vkAcquireNextImageKHR(g_Device, g_Swapchain, UINT64_MAX, g_Semaphore[g_FrameIndex], VK_NULL_HANDLE, &g_BackBufferIndex);
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
        info.framebuffer = g_Framebuffer[g_BackBufferIndex];
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
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = g_BackBuffer[g_BackBufferIndex];
        barrier.subresourceRange = g_ImageRange;
        vkCmdPipelineBarrier(g_CommandBuffer[g_FrameIndex], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
    }
    {
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &g_Semaphore[g_FrameIndex];
        info.commandBufferCount = 1;
        info.pCommandBuffers = &g_CommandBuffer[g_FrameIndex];

        err = vkEndCommandBuffer(g_CommandBuffer[g_FrameIndex]);
        check_vk_result(err);
        err = vkResetFences(g_Device, 1, &g_Fence[g_FrameIndex]);
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &info, g_Fence[g_FrameIndex]);
        check_vk_result(err);
    }
    {
        VkResult res;
        VkSwapchainKHR swapchains[1] = {g_Swapchain};
        uint32_t indices[1] = {g_BackBufferIndex};
        VkPresentInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.swapchainCount = 1;
        info.pSwapchains = swapchains;
        info.pImageIndices = indices;
        info.pResults = &res;
        err = vkQueuePresentKHR(g_Queue, &info);
        check_vk_result(err);
        check_vk_result(res);
    }
    g_FrameIndex = (g_FrameIndex) % IMGUI_VK_QUEUED_FRAMES;
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
            ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }

        g_ClearValue.color.float32[0] = clear_color.x;
        g_ClearValue.color.float32[1] = clear_color.y;
        g_ClearValue.color.float32[2] = clear_color.z;
        g_ClearValue.color.float32[3] = clear_color.w;

        frame_begin();
        ImGui_ImplGlfwVulkan_Render(g_CommandBuffer[g_FrameIndex]);
        frame_end();
    }

    // Cleanup
    VkResult err = vkDeviceWaitIdle(g_Device);
    check_vk_result(err);
    ImGui_ImplGlfwVulkan_Shutdown();
    cleanup_vulkan();
    glfwTerminate();

    return 0;
}
