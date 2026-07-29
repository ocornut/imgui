// Dear ImGui: standalone example application for QNX Screen + Vulkan

// Important note to the reader who wishes to integrate imgui_impl_vulkan.cpp/.h in their own engine/app.
// - Common ImGui_ImplVulkan_XXX functions and structures interface with the renderer backend.
// - ImGui_ImplVulkanH_XXX helpers are used by examples and are not intended as an engine abstraction.

#include "imgui.h"
#include "imgui_impl_screen.h"
#include "imgui_impl_vulkan.h"

#include <errno.h>
#include <screen/screen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/keycodes.h>
#include <time.h>
#include <vulkan/vulkan.h>

struct ScreenApp
{
    screen_context_t Context;
    screen_window_t  Window;
    screen_event_t   Event;
    int              WindowSize[2];

    ScreenApp()
    {
        memset(this, 0, sizeof(*this));
    }
};

static VkAllocationCallbacks*   g_Allocator = nullptr;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = UINT32_MAX;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;
static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t                 g_MinImageCount = 2;
static bool                     g_SwapChainRebuild = false;

static void CheckVkResult(VkResult result)
{
    if (result == VK_SUCCESS)
    {
        return;
    }
    fprintf(stderr, "[vulkan] VkResult = %d\n", result);
    if (result < 0)
    {
        abort();
    }
}

static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* name)
{
    for (const VkExtensionProperties& property : properties)
    {
        if (strcmp(property.extensionName, name) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool CreateScreenWindow(ScreenApp* app)
{
    if (screen_create_context(&app->Context, 0) != 0)
    {
        fprintf(stderr, "screen_create_context() failed: %s\n", strerror(errno));
        return false;
    }
    if (screen_create_window(&app->Window, app->Context) != 0)
    {
        fprintf(stderr, "screen_create_window() failed: %s\n", strerror(errno));
        return false;
    }
    if (screen_create_event(&app->Event) != 0)
    {
        fprintf(stderr, "screen_create_event() failed: %s\n", strerror(errno));
        return false;
    }
    if (screen_get_window_property_iv(app->Window, SCREEN_PROPERTY_SIZE, app->WindowSize) != 0)
    {
        fprintf(stderr, "screen_get_window_property_iv(SCREEN_PROPERTY_SIZE) failed: %s\n", strerror(errno));
        return false;
    }
    if (app->WindowSize[0] <= 0 || app->WindowSize[1] <= 0)
    {
        fprintf(stderr, "screen_get_window_property_iv(SCREEN_PROPERTY_SIZE) returned invalid size %dx%d\n",
                app->WindowSize[0], app->WindowSize[1]);
        return false;
    }

    const int usage = SCREEN_USAGE_VULKAN | SCREEN_USAGE_NATIVE;
    const int transparency = SCREEN_TRANSPARENCY_NONE;
    const char* id = "dear-imgui-screen-vulkan";
    (void)screen_set_window_property_cv(app->Window, SCREEN_PROPERTY_ID_STRING, (int)strlen(id) + 1, id);
    if (screen_set_window_property_iv(app->Window, SCREEN_PROPERTY_USAGE, &usage) != 0 ||
        screen_set_window_property_iv(app->Window, SCREEN_PROPERTY_SIZE, app->WindowSize) != 0 ||
        screen_set_window_property_iv(app->Window, SCREEN_PROPERTY_TRANSPARENCY, &transparency) != 0)
    {
        fprintf(stderr, "Failed to configure QNX Screen window: %s\n", strerror(errno));
        return false;
    }
    return true;
}

static bool CreateVulkanInstance()
{
    uint32_t extension_count = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    if (result != VK_SUCCESS)
    {
        CheckVkResult(result);
        return false;
    }

    ImVector<VkExtensionProperties> available_extensions;
    available_extensions.resize(extension_count);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.Data);
    if (result != VK_SUCCESS)
    {
        CheckVkResult(result);
        return false;
    }

    const char* required_extensions[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_QNX_SCREEN_SURFACE_EXTENSION_NAME
    };
    for (const char* extension : required_extensions)
    {
        if (!IsExtensionAvailable(available_extensions, extension))
        {
            fprintf(stderr, "Required Vulkan instance extension is unavailable: %s\n", extension);
            return false;
        }
    }

    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "Dear ImGui QNX Screen Vulkan example";
    application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.pEngineName = "None";
    application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application_info;
    create_info.enabledExtensionCount = IM_COUNTOF(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;
    result = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
    if (result != VK_SUCCESS)
    {
        CheckVkResult(result);
        return false;
    }
    return true;
}

static bool CreateVulkanSurface(const ScreenApp* app, VkSurfaceKHR* surface)
{
    VkScreenSurfaceCreateInfoQNX create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SCREEN_SURFACE_CREATE_INFO_QNX;
    create_info.context = app->Context;
    create_info.window = app->Window;
    const VkResult result = vkCreateScreenSurfaceQNX(g_Instance, &create_info, g_Allocator, surface);
    if (result != VK_SUCCESS)
    {
        CheckVkResult(result);
        return false;
    }
    return true;
}

static int PhysicalDeviceScore(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(device, &properties);
    switch (properties.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return 500;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 400;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return 300;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:            return 100;
        default:                                     return 200;
    }
}

static bool SelectPhysicalDeviceAndQueue(VkSurfaceKHR surface)
{
    uint32_t device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(g_Instance, &device_count, nullptr);
    if (result != VK_SUCCESS || device_count == 0)
    {
        CheckVkResult(result);
        fprintf(stderr, "No Vulkan physical device is available.\n");
        return false;
    }

    ImVector<VkPhysicalDevice> devices;
    devices.resize(device_count);
    result = vkEnumeratePhysicalDevices(g_Instance, &device_count, devices.Data);
    if (result != VK_SUCCESS)
    {
        CheckVkResult(result);
        return false;
    }

    int best_score = -1;
    for (VkPhysicalDevice device : devices)
    {
        uint32_t queue_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, nullptr);
        ImVector<VkQueueFamilyProperties> queues;
        queues.resize(queue_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, queues.Data);

        for (uint32_t queue_index = 0; queue_index < queue_count; queue_index++)
        {
            VkBool32 present_supported = VK_FALSE;
            result = vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_index, surface, &present_supported);
            if (result != VK_SUCCESS)
            {
                continue;
            }
            if ((queues[queue_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 || present_supported != VK_TRUE)
            {
                continue;
            }

            const int score = PhysicalDeviceScore(device);
            if (score > best_score)
            {
                best_score = score;
                g_PhysicalDevice = device;
                g_QueueFamily = queue_index;
            }
        }
    }

    if (g_PhysicalDevice == VK_NULL_HANDLE || g_QueueFamily == UINT32_MAX)
    {
        fprintf(stderr, "No Vulkan queue family supports both graphics and QNX Screen presentation.\n");
        return false;
    }
    return true;
}

static bool CreateVulkanDevice()
{
    uint32_t extension_count = 0;
    VkResult result = vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &extension_count, nullptr);
    if (result != VK_SUCCESS)
    {
        CheckVkResult(result);
        return false;
    }

    ImVector<VkExtensionProperties> available_extensions;
    available_extensions.resize(extension_count);
    result = vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &extension_count, available_extensions.Data);
    if (result != VK_SUCCESS)
    {
        CheckVkResult(result);
        return false;
    }
    if (!IsExtensionAvailable(available_extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
    {
        fprintf(stderr, "Required Vulkan device extension is unavailable: %s\n", VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        return false;
    }

    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = g_QueueFamily;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;

    const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_info;
    create_info.enabledExtensionCount = IM_COUNTOF(device_extensions);
    create_info.ppEnabledExtensionNames = device_extensions;
    result = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
    if (result != VK_SUCCESS)
    {
        CheckVkResult(result);
        return false;
    }
    vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
    return true;
}

static bool CreateDescriptorPool()
{
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.poolSizeCount = IM_COUNTOF(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    for (const VkDescriptorPoolSize& pool_size : pool_sizes)
    {
        pool_info.maxSets += pool_size.descriptorCount;
    }

    const VkResult result = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
    if (result != VK_SUCCESS)
    {
        CheckVkResult(result);
        return false;
    }
    return true;
}

static bool SetupVulkanWindow(ImGui_ImplVulkanH_Window* window_data, VkSurfaceKHR surface, int width, int height)
{
    const VkFormat requested_formats[] =
    {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
    };
    window_data->Surface = surface;
    window_data->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
        g_PhysicalDevice,
        surface,
        requested_formats,
        IM_COUNTOF(requested_formats),
        VK_COLORSPACE_SRGB_NONLINEAR_KHR);
    if (window_data->SurfaceFormat.format == VK_FORMAT_UNDEFINED)
    {
        fprintf(stderr, "No suitable Vulkan surface format is available.\n");
        return false;
    }

    const VkPresentModeKHR requested_present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
    window_data->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
        g_PhysicalDevice,
        surface,
        requested_present_modes,
        IM_COUNTOF(requested_present_modes));
    ImGui_ImplVulkanH_CreateOrResizeWindow(
        g_Instance,
        g_PhysicalDevice,
        g_Device,
        window_data,
        g_QueueFamily,
        g_Allocator,
        width,
        height,
        g_MinImageCount,
        0);
    return true;
}

static void FrameRender(ImGui_ImplVulkanH_Window* window_data, ImDrawData* draw_data)
{
    VkSemaphore image_acquired = window_data->FrameSemaphores[window_data->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete = window_data->FrameSemaphores[window_data->SemaphoreIndex].RenderCompleteSemaphore;
    VkResult result = vkAcquireNextImageKHR(g_Device, window_data->Swapchain, UINT64_MAX, image_acquired, VK_NULL_HANDLE, &window_data->FrameIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        g_SwapChainRebuild = true;
    }
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return;
    }
    if (result != VK_SUBOPTIMAL_KHR)
    {
        CheckVkResult(result);
    }

    ImGui_ImplVulkanH_Frame* frame = &window_data->Frames[window_data->FrameIndex];
    result = vkWaitForFences(g_Device, 1, &frame->Fence, VK_TRUE, UINT64_MAX);
    CheckVkResult(result);
    result = vkResetFences(g_Device, 1, &frame->Fence);
    CheckVkResult(result);
    result = vkResetCommandPool(g_Device, frame->CommandPool, 0);
    CheckVkResult(result);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(frame->CommandBuffer, &begin_info);
    CheckVkResult(result);

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = window_data->RenderPass;
    render_pass_info.framebuffer = frame->Framebuffer;
    render_pass_info.renderArea.extent.width = window_data->Width;
    render_pass_info.renderArea.extent.height = window_data->Height;
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &window_data->ClearValue;
    vkCmdBeginRenderPass(frame->CommandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(draw_data, frame->CommandBuffer);
    vkCmdEndRenderPass(frame->CommandBuffer);

    result = vkEndCommandBuffer(frame->CommandBuffer);
    CheckVkResult(result);

    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_acquired;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &frame->CommandBuffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_complete;
    result = vkQueueSubmit(g_Queue, 1, &submit_info, frame->Fence);
    CheckVkResult(result);
}

static void FramePresent(ImGui_ImplVulkanH_Window* window_data)
{
    if (g_SwapChainRebuild)
    {
        return;
    }

    VkSemaphore render_complete = window_data->FrameSemaphores[window_data->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_complete;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &window_data->Swapchain;
    present_info.pImageIndices = &window_data->FrameIndex;
    VkResult result = vkQueuePresentKHR(g_Queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        g_SwapChainRebuild = true;
    }
    else
    {
        CheckVkResult(result);
    }
    window_data->SemaphoreIndex = (window_data->SemaphoreIndex + 1) % window_data->SemaphoreCount;
}

static bool IsExitEvent(screen_event_t event)
{
    int type = SCREEN_EVENT_NONE;
    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &type) != 0)
    {
        return false;
    }
    if (type == SCREEN_EVENT_KEYBOARD)
    {
        int flags = 0;
        int symbol = 0;
        if (screen_get_event_property_iv(event, SCREEN_PROPERTY_FLAGS, &flags) == 0 &&
            screen_get_event_property_iv(event, SCREEN_PROPERTY_SYM, &symbol) == 0 &&
            (flags & KEY_DOWN) != 0 && symbol == KEYCODE_ESCAPE)
        {
            return true;
        }
    }
    if (type == SCREEN_EVENT_CLOSE)
    {
        return true;
    }
    return false;
}

static bool ProcessScreenEvents(ScreenApp* app)
{
    for (;;)
    {
        if (screen_get_event(app->Context, app->Event, 0) != 0)
        {
            fprintf(stderr, "screen_get_event() failed: %s\n", strerror(errno));
            return false;
        }
        int type = SCREEN_EVENT_NONE;
        if (screen_get_event_property_iv(app->Event, SCREEN_PROPERTY_TYPE, &type) != 0)
        {
            fprintf(stderr, "screen_get_event_property_iv(SCREEN_PROPERTY_TYPE) failed: %s\n", strerror(errno));
            return false;
        }
        if (type == SCREEN_EVENT_NONE)
        {
            break;
        }
        if (IsExitEvent(app->Event))
        {
            return false;
        }
        ImGui_ImplScreen_ProcessEvent(app->Event);
    }
    return true;
}

static void SleepMilliseconds(long milliseconds)
{
    struct timespec delay = {};
    delay.tv_sec = milliseconds / 1000;
    delay.tv_nsec = (milliseconds % 1000) * 1000000L;
    nanosleep(&delay, nullptr);
}

static void CleanupVulkanWindow(ImGui_ImplVulkanH_Window* window_data)
{
    if (g_Device != VK_NULL_HANDLE && window_data->Surface != VK_NULL_HANDLE)
    {
        const VkSurfaceKHR surface = window_data->Surface;
        ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, window_data, g_Allocator);
        vkDestroySurfaceKHR(g_Instance, surface, g_Allocator);
    }
}

static void CleanupVulkan()
{
    if (g_Device != VK_NULL_HANDLE)
    {
        if (g_DescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);
        }
        vkDestroyDevice(g_Device, g_Allocator);
    }
    if (g_Instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(g_Instance, g_Allocator);
    }
}

static void DestroyScreenApp(ScreenApp* app)
{
    if (app->Event != nullptr)
    {
        screen_destroy_event(app->Event);
    }
    if (app->Window != nullptr)
    {
        screen_destroy_window(app->Window);
    }
    if (app->Context != nullptr)
    {
        screen_destroy_context(app->Context);
    }
}

int main(int, char**)
{
    ScreenApp app;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!CreateScreenWindow(&app) ||
        !CreateVulkanInstance() ||
        !CreateVulkanSurface(&app, &surface) ||
        !SelectPhysicalDeviceAndQueue(surface) ||
        !CreateVulkanDevice() ||
        !CreateDescriptorPool())
    {
        if (surface != VK_NULL_HANDLE && g_Instance != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(g_Instance, surface, g_Allocator);
        }
        CleanupVulkan();
        DestroyScreenApp(&app);
        return EXIT_FAILURE;
    }

    int window_size[2] = { app.WindowSize[0], app.WindowSize[1] };
    if (!SetupVulkanWindow(&g_MainWindowData, surface, window_size[0], window_size[1]))
    {
        fprintf(stderr, "Failed to set up the Vulkan swapchain.\n");
        vkDestroySurfaceKHR(g_Instance, surface, g_Allocator);
        CleanupVulkan();
        DestroyScreenApp(&app);
        return EXIT_FAILURE;
    }

    const int visible = 1;
    if (screen_set_window_property_iv(app.Window, SCREEN_PROPERTY_VISIBLE, &visible) != 0)
    {
        fprintf(stderr, "screen_set_window_property_iv(SCREEN_PROPERTY_VISIBLE) failed: %s\n", strerror(errno));
        CleanupVulkanWindow(&g_MainWindowData);
        CleanupVulkan();
        DestroyScreenApp(&app);
        return EXIT_FAILURE;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    bool screen_backend_initialized = ImGui_ImplScreen_Init(app.Context, app.Window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_0;
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = g_MainWindowData.ImageCount;
    init_info.Allocator = g_Allocator;
    init_info.PipelineInfoMain.RenderPass = g_MainWindowData.RenderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = CheckVkResult;
    const bool renderer_backend_initialized = ImGui_ImplVulkan_Init(&init_info);
    if (!screen_backend_initialized || !renderer_backend_initialized)
    {
        fprintf(stderr, "Failed to initialize Dear ImGui backends.\n");
        if (renderer_backend_initialized)
        {
            ImGui_ImplVulkan_Shutdown();
        }
        if (screen_backend_initialized)
        {
            ImGui_ImplScreen_Shutdown();
        }
        ImGui::DestroyContext();
        CleanupVulkanWindow(&g_MainWindowData);
        CleanupVulkan();
        DestroyScreenApp(&app);
        return EXIT_FAILURE;
    }

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);
    bool running = true;
    while (running)
    {
        running = ProcessScreenEvents(&app);
        if (!running)
        {
            break;
        }

        if (screen_get_window_property_iv(app.Window, SCREEN_PROPERTY_SIZE, window_size) != 0)
        {
            fprintf(stderr, "screen_get_window_property_iv(SCREEN_PROPERTY_SIZE) failed: %s\n", strerror(errno));
            break;
        }
        if (window_size[0] <= 0 || window_size[1] <= 0)
        {
            SleepMilliseconds(10);
            continue;
        }
        if (g_SwapChainRebuild || g_MainWindowData.Width != window_size[0] || g_MainWindowData.Height != window_size[1])
        {
            ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
            ImGui_ImplVulkanH_CreateOrResizeWindow(
                g_Instance,
                g_PhysicalDevice,
                g_Device,
                &g_MainWindowData,
                g_QueueFamily,
                g_Allocator,
                window_size[0],
                window_size[1],
                g_MinImageCount,
                0);
            g_MainWindowData.FrameIndex = 0;
            g_SwapChainRebuild = false;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplScreen_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        static float value = 0.0f;
        static int counter = 0;
        static char text[128] = "Keyboard input is handled by QNX Screen";
        ImGui::Begin("QNX Screen + Vulkan");
        ImGui::Checkbox("Demo Window", &show_demo_window);
        ImGui::Checkbox("Another Window", &show_another_window);
        ImGui::SliderFloat("float", &value, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        ImGui::InputText("text", text, IM_ARRAYSIZE(text));
        if (ImGui::Button("Button"))
        {
            counter++;
        }
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::TextUnformatted("Press Esc to exit.");
        ImGui::End();

        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::TextUnformatted("Hello from QNX Screen!");
            if (ImGui::Button("Close Me"))
            {
                show_another_window = false;
            }
            ImGui::End();
        }

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool minimized = draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f;
        if (!minimized)
        {
            g_MainWindowData.ClearValue.color.float32[0] = clear_color.x * clear_color.w;
            g_MainWindowData.ClearValue.color.float32[1] = clear_color.y * clear_color.w;
            g_MainWindowData.ClearValue.color.float32[2] = clear_color.z * clear_color.w;
            g_MainWindowData.ClearValue.color.float32[3] = clear_color.w;
            FrameRender(&g_MainWindowData, draw_data);
            FramePresent(&g_MainWindowData);
        }
    }

    CheckVkResult(vkDeviceWaitIdle(g_Device));
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplScreen_Shutdown();
    ImGui::DestroyContext();
    CleanupVulkanWindow(&g_MainWindowData);
    CleanupVulkan();
    DestroyScreenApp(&app);
    return EXIT_SUCCESS;
}
