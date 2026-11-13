#pragma once
// ═════════════════════════════════════════════════════════════════════════════
//  vulkan_helper.h — Vulkan setup, cleanup, and frame rendering
//
//  Uses ImGui_ImplVulkanH helpers for swapchain/renderpass/framebuffer
//  management. Exposes globals and functions for main.cpp to use.
// ═════════════════════════════════════════════════════════════════════════════

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <cstdlib>
#include <cstring>
#include <android/log.h>
#include <android_native_app_glue.h>
#include "imgui.h"
#include "imgui_impl_vulkan.h"

#define LOG_TAG "ImGuiVulkan"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace vkh
{

// ─────────────────────────────────────────────────────────────────────────────
// Globals
// ─────────────────────────────────────────────────────────────────────────────
static VkAllocationCallbacks*   g_Allocator      = nullptr;
static VkInstance               g_Instance       = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device         = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily    = (uint32_t)-1;
static VkQueue                  g_Queue          = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache  = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t                 g_MinImageCount  = 2;
static bool                     g_SwapChainRebuild = false;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
inline void CheckVkResult(VkResult err)
{
    if (err == VK_SUCCESS) return;
    LOGE("[vulkan] VkResult = %d", err);
    if (err < 0) abort();
}

static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& props, const char* ext)
{
    for (const auto& p : props)
        if (strcmp(p.extensionName, ext) == 0) return true;
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupVulkan — Instance, PhysicalDevice, Device, Queue, DescriptorPool
// ─────────────────────────────────────────────────────────────────────────────
inline void SetupVulkan()
{
    VkResult err;

    // Instance
    {
        ImVector<const char*> extensions;
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);

        uint32_t n;
        ImVector<VkExtensionProperties> props;
        vkEnumerateInstanceExtensionProperties(nullptr, &n, nullptr);
        props.resize(n);
        vkEnumerateInstanceExtensionProperties(nullptr, &n, props.Data);

        if (IsExtensionAvailable(props, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

        VkInstanceCreateInfo ci = {};
        ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.enabledExtensionCount   = (uint32_t)extensions.Size;
        ci.ppEnabledExtensionNames = extensions.Data;
        err = vkCreateInstance(&ci, g_Allocator, &g_Instance);
        CheckVkResult(err);
        LOGI("[vulkan] Instance created");
    }

    // PhysicalDevice + QueueFamily
    g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(g_Instance);
    g_QueueFamily    = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice);

    // Logical Device
    {
        ImVector<const char*> dev_ext;
        dev_ext.push_back("VK_KHR_swapchain");

        const float prio = 1.0f;
        VkDeviceQueueCreateInfo qci = {};
        qci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qci.queueFamilyIndex = g_QueueFamily;
        qci.queueCount       = 1;
        qci.pQueuePriorities = &prio;

        VkDeviceCreateInfo dci = {};
        dci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dci.queueCreateInfoCount    = 1;
        dci.pQueueCreateInfos       = &qci;
        dci.enabledExtensionCount   = (uint32_t)dev_ext.Size;
        dci.ppEnabledExtensionNames = dev_ext.Data;
        err = vkCreateDevice(g_PhysicalDevice, &dci, g_Allocator, &g_Device);
        CheckVkResult(err);
        vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
        LOGI("[vulkan] Device + Queue created");
    }

    // DescriptorPool
    {
        VkDescriptorPoolSize ps = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE };
        VkDescriptorPoolCreateInfo dpci = {};
        dpci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        dpci.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        dpci.maxSets       = ps.descriptorCount;
        dpci.poolSizeCount = 1;
        dpci.pPoolSizes    = &ps;
        err = vkCreateDescriptorPool(g_Device, &dpci, g_Allocator, &g_DescriptorPool);
        CheckVkResult(err);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupWindow — Surface, SwapChain, RenderPass, Framebuffers
// ─────────────────────────────────────────────────────────────────────────────
inline void SetupWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int w, int h)
{
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, surface, &res);
    if (res != VK_TRUE) { LOGE("No WSI support!"); abort(); }

    const VkFormat fmts[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM };
    wd->Surface       = surface;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, surface, fmts, IM_COUNTOF(fmts), VK_COLORSPACE_SRGB_NONLINEAR_KHR);

    VkPresentModeKHR pm[] = { VK_PRESENT_MODE_FIFO_KHR };
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, surface, pm, IM_COUNTOF(pm));

    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, w, h, g_MinImageCount, 0);
    LOGI("[vulkan] Window: %dx%d, %d images", wd->Width, wd->Height, wd->ImageCount);
}

// ─────────────────────────────────────────────────────────────────────────────
// RebuildSwapchain — handles orientation/size changes
// ─────────────────────────────────────────────────────────────────────────────
inline bool RebuildIfNeeded(ImGui_ImplVulkanH_Window* wd, ANativeWindow* window)
{
    int w = ANativeWindow_getWidth(window);
    int h = ANativeWindow_getHeight(window);

    // Detect size change
    if (w > 0 && h > 0 && (w != wd->Width || h != wd->Height))
        g_SwapChainRebuild = true;

    if (!g_SwapChainRebuild) return false;
    if (w <= 0 || h <= 0) return false;

    vkDeviceWaitIdle(g_Device);
    ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, w, h, g_MinImageCount, 0);
    wd->FrameIndex = 0;
    g_SwapChainRebuild = false;

    ImGui::GetIO().DisplaySize = ImVec2((float)w, (float)h);
    LOGI("[resize] %dx%d", w, h);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// FrameRender / FramePresent
// ─────────────────────────────────────────────────────────────────────────────
inline void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
    VkSemaphore image_acquired  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;

    VkResult err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR) return;
    if (err != VK_SUBOPTIMAL_KHR) CheckVkResult(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
    vkResetFences(g_Device, 1, &fd->Fence);
    vkResetCommandPool(g_Device, fd->CommandPool, 0);

    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(fd->CommandBuffer, &bi);

    VkRenderPassBeginInfo rp = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rp.renderPass  = wd->RenderPass;
    rp.framebuffer = fd->Framebuffer;
    rp.renderArea  = {{0,0}, {(uint32_t)wd->Width, (uint32_t)wd->Height}};
    rp.clearValueCount = 1;
    rp.pClearValues    = &wd->ClearValue;
    vkCmdBeginRenderPass(fd->CommandBuffer, &rp, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);
    vkCmdEndRenderPass(fd->CommandBuffer);

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.waitSemaphoreCount = 1;  si.pWaitSemaphores    = &image_acquired;
    si.pWaitDstStageMask  = &wait_stage;
    si.commandBufferCount = 1;  si.pCommandBuffers    = &fd->CommandBuffer;
    si.signalSemaphoreCount = 1; si.pSignalSemaphores  = &render_complete;

    vkEndCommandBuffer(fd->CommandBuffer);
    vkQueueSubmit(g_Queue, 1, &si, fd->Fence);
}

inline void FramePresent(ImGui_ImplVulkanH_Window* wd)
{
    if (g_SwapChainRebuild) return;
    VkSemaphore render_complete = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR pi = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    pi.waitSemaphoreCount = 1;  pi.pWaitSemaphores = &render_complete;
    pi.swapchainCount     = 1;  pi.pSwapchains     = &wd->Swapchain;
    pi.pImageIndices      = &wd->FrameIndex;

    VkResult err = vkQueuePresentKHR(g_Queue, &pi);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) g_SwapChainRebuild = true;
    if (err != VK_ERROR_OUT_OF_DATE_KHR && err != VK_SUBOPTIMAL_KHR) CheckVkResult(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;
}

// ─────────────────────────────────────────────────────────────────────────────
// Cleanup
// ─────────────────────────────────────────────────────────────────────────────
inline void CleanupWindow(ImGui_ImplVulkanH_Window* wd)
{
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, wd, g_Allocator);
    vkDestroySurfaceKHR(g_Instance, wd->Surface, g_Allocator);
}

inline void CleanupVulkan()
{
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);
    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

} // namespace vkh
