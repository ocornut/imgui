// ImGui GLFW binding with Vulkan + shaders
// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 5 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXX_CreateFontsTexture(), ImGui_ImplXXXX_NewFrame(), ImGui_ImplXXXX_Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <imgui.h>

// GLFW
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include "imgui_impl_glfw_vulkan.h"

// GLFW Data
static GLFWwindow*  g_Window = NULL;
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;

// Vulkan Data
static VkAllocationCallbacks* g_Allocator = NULL;
static VkPhysicalDevice       g_Gpu = VK_NULL_HANDLE;
static VkDevice               g_Device = VK_NULL_HANDLE;
static VkRenderPass           g_RenderPass = VK_NULL_HANDLE;
static VkPipelineCache        g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool       g_DescriptorPool = VK_NULL_HANDLE;
static void (*g_CheckVkResult)(VkResult err) = NULL;

static VkCommandBuffer        g_CommandBuffer = VK_NULL_HANDLE;
static size_t                 g_BufferMemoryAlignment = 256;
static VkPipelineCreateFlags  g_PipelineCreateFlags = 0;
static int                    g_FrameIndex = 0;

static VkDescriptorSetLayout  g_DescriptorSetLayout = VK_NULL_HANDLE;
static VkPipelineLayout       g_PipelineLayout = VK_NULL_HANDLE;
static VkDescriptorSet        g_DescriptorSet = VK_NULL_HANDLE;
static VkPipeline             g_Pipeline = VK_NULL_HANDLE;

static VkSampler              g_FontSampler = VK_NULL_HANDLE;
static VkDeviceMemory         g_FontMemory = VK_NULL_HANDLE;
static VkImage                g_FontImage = VK_NULL_HANDLE;
static VkImageView            g_FontView = VK_NULL_HANDLE;

static VkDeviceMemory         g_VertexBufferMemory[IMGUI_VK_QUEUED_FRAMES] = {};
static VkDeviceMemory         g_IndexBufferMemory[IMGUI_VK_QUEUED_FRAMES] = {};
static size_t                 g_VertexBufferSize[IMGUI_VK_QUEUED_FRAMES] = {};
static size_t                 g_IndexBufferSize[IMGUI_VK_QUEUED_FRAMES] = {};
static VkBuffer               g_VertexBuffer[IMGUI_VK_QUEUED_FRAMES] = {};
static VkBuffer               g_IndexBuffer[IMGUI_VK_QUEUED_FRAMES] = {};

static VkDeviceMemory         g_UploadBufferMemory = VK_NULL_HANDLE;
static VkBuffer               g_UploadBuffer = VK_NULL_HANDLE;

static unsigned char __glsl_shader_vert_spv[] = 
{
  0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x08, 0x00,
  0x6c, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x11, 0x00, 0x02, 0x00, 0x21, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64,
  0x2e, 0x34, 0x35, 0x30, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0a, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x1f, 0x16, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
  0x00, 0x00, 0x00, 0x00, 0x47, 0x11, 0x00, 0x00, 0x41, 0x14, 0x00, 0x00,
  0x6a, 0x16, 0x00, 0x00, 0x42, 0x13, 0x00, 0x00, 0x80, 0x14, 0x00, 0x00,
  0x47, 0x00, 0x03, 0x00, 0x1a, 0x04, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x41, 0x14, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x6a, 0x16, 0x00, 0x00,
  0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
  0xb1, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0xb1, 0x02, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x48, 0x00, 0x05, 0x00, 0xb1, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x0b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
  0xb1, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0xb1, 0x02, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x80, 0x14, 0x00, 0x00,
  0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
  0x06, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x06, 0x04, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x03, 0x00, 0x06, 0x04, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0xfa, 0x16, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x21, 0x00, 0x03, 0x00, 0x02, 0x05, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x16, 0x00, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x17, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x04, 0x00,
  0x1a, 0x04, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x97, 0x06, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x1a, 0x04, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x97, 0x06, 0x00, 0x00,
  0x47, 0x11, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x9a, 0x02, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
  0x9a, 0x02, 0x00, 0x00, 0x41, 0x14, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x9b, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x0e, 0x0a, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x90, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x3b, 0x00, 0x04, 0x00, 0x90, 0x02, 0x00, 0x00, 0x6a, 0x16, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x91, 0x02, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
  0x0b, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x2b, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0d, 0x0a, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x7f, 0x02, 0x00, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0x0d, 0x0a, 0x00, 0x00, 0x1e, 0x00, 0x06, 0x00,
  0xb1, 0x02, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
  0x7f, 0x02, 0x00, 0x00, 0x7f, 0x02, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x2e, 0x05, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xb1, 0x02, 0x00, 0x00,
  0x3b, 0x00, 0x04, 0x00, 0x2e, 0x05, 0x00, 0x00, 0x42, 0x13, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x90, 0x02, 0x00, 0x00,
  0x80, 0x14, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x04, 0x00,
  0x06, 0x04, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x83, 0x06, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
  0x06, 0x04, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x83, 0x06, 0x00, 0x00,
  0xfa, 0x16, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x92, 0x02, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x2b, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0c, 0x0a, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00,
  0x8a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x36, 0x00, 0x05, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x1f, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x05, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x6b, 0x60, 0x00, 0x00,
  0x3d, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x71, 0x4e, 0x00, 0x00,
  0x41, 0x14, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x9b, 0x02, 0x00, 0x00,
  0xaa, 0x26, 0x00, 0x00, 0x47, 0x11, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
  0x3e, 0x00, 0x03, 0x00, 0xaa, 0x26, 0x00, 0x00, 0x71, 0x4e, 0x00, 0x00,
  0x3d, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0xda, 0x35, 0x00, 0x00,
  0x6a, 0x16, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x91, 0x02, 0x00, 0x00,
  0xea, 0x50, 0x00, 0x00, 0x47, 0x11, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00,
  0x3e, 0x00, 0x03, 0x00, 0xea, 0x50, 0x00, 0x00, 0xda, 0x35, 0x00, 0x00,
  0x3d, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0xc7, 0x35, 0x00, 0x00,
  0x80, 0x14, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x92, 0x02, 0x00, 0x00,
  0xef, 0x56, 0x00, 0x00, 0xfa, 0x16, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
  0x3d, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0xe0, 0x29, 0x00, 0x00,
  0xef, 0x56, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x13, 0x00, 0x00, 0x00,
  0xa0, 0x22, 0x00, 0x00, 0xc7, 0x35, 0x00, 0x00, 0xe0, 0x29, 0x00, 0x00,
  0x41, 0x00, 0x05, 0x00, 0x92, 0x02, 0x00, 0x00, 0x42, 0x2c, 0x00, 0x00,
  0xfa, 0x16, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x09, 0x60, 0x00, 0x00, 0x42, 0x2c, 0x00, 0x00,
  0x81, 0x00, 0x05, 0x00, 0x13, 0x00, 0x00, 0x00, 0xd1, 0x4e, 0x00, 0x00,
  0xa0, 0x22, 0x00, 0x00, 0x09, 0x60, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0xa1, 0x41, 0x00, 0x00, 0xd1, 0x4e, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x0d, 0x00, 0x00, 0x00,
  0x84, 0x36, 0x00, 0x00, 0xd1, 0x4e, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x50, 0x00, 0x07, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x54, 0x47, 0x00, 0x00,
  0xa1, 0x41, 0x00, 0x00, 0x84, 0x36, 0x00, 0x00, 0x0c, 0x0a, 0x00, 0x00,
  0x8a, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x9b, 0x02, 0x00, 0x00,
  0x17, 0x2f, 0x00, 0x00, 0x42, 0x13, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
  0x3e, 0x00, 0x03, 0x00, 0x17, 0x2f, 0x00, 0x00, 0x54, 0x47, 0x00, 0x00,
  0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
};
static unsigned int __glsl_shader_vert_spv_len = 1172;

static unsigned char __glsl_shader_frag_spv[] = 
{
  0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x08, 0x00,
  0x6c, 0x5d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
  0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x1f, 0x16, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
  0x7a, 0x0c, 0x00, 0x00, 0x35, 0x16, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00,
  0x1f, 0x16, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
  0x7a, 0x0c, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x47, 0x00, 0x04, 0x00, 0x7a, 0x0c, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x1a, 0x04, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0xec, 0x14, 0x00, 0x00,
  0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
  0xec, 0x14, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x13, 0x00, 0x02, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00,
  0x02, 0x05, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x9a, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x9a, 0x02, 0x00, 0x00,
  0x7a, 0x0c, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x1e, 0x00, 0x04, 0x00, 0x1a, 0x04, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x97, 0x06, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x1a, 0x04, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
  0x97, 0x06, 0x00, 0x00, 0x35, 0x16, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x15, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
  0x0b, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
  0x9b, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
  0x19, 0x00, 0x09, 0x00, 0x96, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x1b, 0x00, 0x03, 0x00, 0xfe, 0x01, 0x00, 0x00, 0x96, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x7b, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xfe, 0x01, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x7b, 0x04, 0x00, 0x00,
  0xec, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x04, 0x00, 0x90, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x1f, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00,
  0xf8, 0x00, 0x02, 0x00, 0x6b, 0x5d, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00,
  0x9b, 0x02, 0x00, 0x00, 0x8d, 0x1b, 0x00, 0x00, 0x35, 0x16, 0x00, 0x00,
  0x0b, 0x0a, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00,
  0x0b, 0x40, 0x00, 0x00, 0x8d, 0x1b, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
  0xfe, 0x01, 0x00, 0x00, 0xc0, 0x36, 0x00, 0x00, 0xec, 0x14, 0x00, 0x00,
  0x41, 0x00, 0x05, 0x00, 0x90, 0x02, 0x00, 0x00, 0xc2, 0x43, 0x00, 0x00,
  0x35, 0x16, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x02, 0x4e, 0x00, 0x00, 0xc2, 0x43, 0x00, 0x00,
  0x57, 0x00, 0x05, 0x00, 0x1d, 0x00, 0x00, 0x00, 0xb9, 0x46, 0x00, 0x00,
  0xc0, 0x36, 0x00, 0x00, 0x02, 0x4e, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0xe4, 0x23, 0x00, 0x00, 0x0b, 0x40, 0x00, 0x00,
  0xb9, 0x46, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x7a, 0x0c, 0x00, 0x00,
  0xe4, 0x23, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
};
static unsigned int __glsl_shader_frag_spv_len = 660;

static uint32_t ImGui_ImplGlfwVulkan_MemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
{
    VkPhysicalDeviceMemoryProperties prop;
    vkGetPhysicalDeviceMemoryProperties(g_Gpu, &prop);
    for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
        if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1<<i))
            return i;
    return 0xffffffff; // Unable to find memoryType
}

static void ImGui_ImplGlfwVulkan_VkResult(VkResult err)
{
    if (g_CheckVkResult)
        g_CheckVkResult(err);
}

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
void ImGui_ImplGlfwVulkan_RenderDrawLists(ImDrawData* draw_data)
{
    VkResult err;
    ImGuiIO& io = ImGui::GetIO();

    // Create the Vertex Buffer:
    size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    if (!g_VertexBuffer[g_FrameIndex] || g_VertexBufferSize[g_FrameIndex] < vertex_size)
    {
        if (g_VertexBuffer[g_FrameIndex])
            vkDestroyBuffer(g_Device, g_VertexBuffer[g_FrameIndex], g_Allocator);
        if (g_VertexBufferMemory[g_FrameIndex])
            vkFreeMemory(g_Device, g_VertexBufferMemory[g_FrameIndex], g_Allocator);
        size_t vertex_buffer_size = ((vertex_size-1) / g_BufferMemoryAlignment+1) * g_BufferMemoryAlignment;
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = vertex_buffer_size;
        buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        err = vkCreateBuffer(g_Device, &buffer_info, g_Allocator, &g_VertexBuffer[g_FrameIndex]);
        ImGui_ImplGlfwVulkan_VkResult(err);
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(g_Device, g_VertexBuffer[g_FrameIndex], &req);
        g_BufferMemoryAlignment = (g_BufferMemoryAlignment > req.alignment) ? g_BufferMemoryAlignment : req.alignment;
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = ImGui_ImplGlfwVulkan_MemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
        err = vkAllocateMemory(g_Device, &alloc_info, g_Allocator, &g_VertexBufferMemory[g_FrameIndex]);
        ImGui_ImplGlfwVulkan_VkResult(err);
        err = vkBindBufferMemory(g_Device, g_VertexBuffer[g_FrameIndex], g_VertexBufferMemory[g_FrameIndex], 0);
        ImGui_ImplGlfwVulkan_VkResult(err);
        g_VertexBufferSize[g_FrameIndex] = vertex_buffer_size;
    }

    // Create the Index Buffer:
    size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
    if (!g_IndexBuffer[g_FrameIndex] || g_IndexBufferSize[g_FrameIndex] < index_size)
    {
        if (g_IndexBuffer[g_FrameIndex])
            vkDestroyBuffer(g_Device, g_IndexBuffer[g_FrameIndex], g_Allocator);
        if (g_IndexBufferMemory[g_FrameIndex])
            vkFreeMemory(g_Device, g_IndexBufferMemory[g_FrameIndex], g_Allocator);
        size_t index_buffer_size = ((index_size-1) / g_BufferMemoryAlignment+1) * g_BufferMemoryAlignment;
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = index_buffer_size;
        buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        err = vkCreateBuffer(g_Device, &buffer_info, g_Allocator, &g_IndexBuffer[g_FrameIndex]);
        ImGui_ImplGlfwVulkan_VkResult(err);
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(g_Device, g_IndexBuffer[g_FrameIndex], &req);
        g_BufferMemoryAlignment = (g_BufferMemoryAlignment > req.alignment) ? g_BufferMemoryAlignment : req.alignment;
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = ImGui_ImplGlfwVulkan_MemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
        err = vkAllocateMemory(g_Device, &alloc_info, g_Allocator, &g_IndexBufferMemory[g_FrameIndex]);
        ImGui_ImplGlfwVulkan_VkResult(err);
        err = vkBindBufferMemory(g_Device, g_IndexBuffer[g_FrameIndex], g_IndexBufferMemory[g_FrameIndex], 0);
        ImGui_ImplGlfwVulkan_VkResult(err);
        g_IndexBufferSize[g_FrameIndex] = index_buffer_size;
    }

    // Upload Vertex and index Data:
    {
        ImDrawVert* vtx_dst;
        ImDrawIdx* idx_dst;
        err = vkMapMemory(g_Device, g_VertexBufferMemory[g_FrameIndex], 0, vertex_size, 0, (void**)(&vtx_dst));
        ImGui_ImplGlfwVulkan_VkResult(err);
        err = vkMapMemory(g_Device, g_IndexBufferMemory[g_FrameIndex], 0, index_size, 0, (void**)(&idx_dst));
        ImGui_ImplGlfwVulkan_VkResult(err);
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, &cmd_list->VtxBuffer[0], cmd_list->VtxBuffer.size() * sizeof(ImDrawVert));
            memcpy(idx_dst, &cmd_list->IdxBuffer[0], cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.size();
            idx_dst += cmd_list->IdxBuffer.size();
        }
        VkMappedMemoryRange range[2] = {};
        range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[0].memory = g_VertexBufferMemory[g_FrameIndex];
        range[0].size = vertex_size;
        range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[1].memory = g_IndexBufferMemory[g_FrameIndex];
        range[1].size = index_size;
        err = vkFlushMappedMemoryRanges(g_Device, 2, range);
        ImGui_ImplGlfwVulkan_VkResult(err);
        vkUnmapMemory(g_Device, g_VertexBufferMemory[g_FrameIndex]);
        vkUnmapMemory(g_Device, g_IndexBufferMemory[g_FrameIndex]);
    }

    // Bind pipeline and descriptor sets:
    {
        vkCmdBindPipeline(g_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_Pipeline);
        VkDescriptorSet desc_set[1] = {g_DescriptorSet};
        vkCmdBindDescriptorSets(g_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelineLayout, 0, 1, desc_set, 0, NULL);
    }

    // Bind Vertex And Index Buffer:
    {
        VkBuffer vertex_buffers[1] = {g_VertexBuffer[g_FrameIndex]};
        VkDeviceSize vertex_offset[1] = {0};
        vkCmdBindVertexBuffers(g_CommandBuffer, 0, 1, vertex_buffers, vertex_offset);
        vkCmdBindIndexBuffer(g_CommandBuffer, g_IndexBuffer[g_FrameIndex], 0, VK_INDEX_TYPE_UINT16);
    }

    // Setup viewport:
    {
        VkViewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = ImGui::GetIO().DisplaySize.x;
        viewport.height = ImGui::GetIO().DisplaySize.y;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(g_CommandBuffer, 0, 1, &viewport);
    }

    // Setup scale and translation:
    {
        float scale[2];
        scale[0] = 2.0f/io.DisplaySize.x;
        scale[1] = 2.0f/io.DisplaySize.y;
        float translate[2];
        translate[0] = -1.0f;
        translate[1] = -1.0f;
        vkCmdPushConstants(g_CommandBuffer, g_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
        vkCmdPushConstants(g_CommandBuffer, g_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);
    }

    // Render the command lists:
    int vtx_offset = 0;
    int idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                VkRect2D scissor;
                scissor.offset.x = static_cast<int32_t>(pcmd->ClipRect.x);
                scissor.offset.y = static_cast<int32_t>(pcmd->ClipRect.y);
                scissor.extent.width = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissor.extent.height = static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y + 1); // TODO: + 1??????
                vkCmdSetScissor(g_CommandBuffer, 0, 1, &scissor);
                vkCmdDrawIndexed(g_CommandBuffer, pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
            }
            idx_offset += pcmd->ElemCount;
        }
        vtx_offset += cmd_list->VtxBuffer.size();
    }
}

static const char* ImGui_ImplGlfwVulkan_GetClipboardText()
{
    return glfwGetClipboardString(g_Window);
}

static void ImGui_ImplGlfwVulkan_SetClipboardText(const char* text)
{
    glfwSetClipboardString(g_Window, text);
}

void ImGui_ImplGlfwVulkan_MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/)
{
    if (action == GLFW_PRESS && button >= 0 && button < 3)
        g_MousePressed[button] = true;
}

void ImGui_ImplGlfwVulkan_ScrollCallback(GLFWwindow*, double /*xoffset*/, double yoffset)
{
    g_MouseWheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
}

void ImGui_ImplGlfwVulkan_KeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    (void)mods; // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}

void ImGui_ImplGlfwVulkan_CharCallback(GLFWwindow*, unsigned int c)
{
    ImGuiIO& io = ImGui::GetIO();
    if (c > 0 && c < 0x10000)
        io.AddInputCharacter((unsigned short)c);
}

bool ImGui_ImplGlfwVulkan_CreateFontsTexture(VkCommandBuffer command_buffer)
{
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    size_t upload_size = width*height*4*sizeof(char);

    VkResult err;

    // Create the Image:
    {
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.extent.width = width;
        info.extent.height = height;
        info.extent.depth = 1;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        err = vkCreateImage(g_Device, &info, g_Allocator, &g_FontImage);
        ImGui_ImplGlfwVulkan_VkResult(err);
        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(g_Device, g_FontImage, &req);
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = ImGui_ImplGlfwVulkan_MemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
        err = vkAllocateMemory(g_Device, &alloc_info, g_Allocator, &g_FontMemory);
        ImGui_ImplGlfwVulkan_VkResult(err);
        err = vkBindImageMemory(g_Device, g_FontImage, g_FontMemory, 0);
        ImGui_ImplGlfwVulkan_VkResult(err);
    }

    // Create the Image View:
    {
        VkResult err;
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = g_FontImage;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.layerCount = 1;
        err = vkCreateImageView(g_Device, &info, g_Allocator, &g_FontView);
        ImGui_ImplGlfwVulkan_VkResult(err);
    }

    // Update the Descriptor Set:
    {
        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = g_FontSampler;
        desc_image[0].imageView = g_FontView;
        desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkWriteDescriptorSet write_desc[1] = {};
        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = g_DescriptorSet;
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc[0].pImageInfo = desc_image;
        vkUpdateDescriptorSets(g_Device, 1, write_desc, 0, NULL);
    }

    // Create the Upload Buffer:
    {
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = upload_size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        err = vkCreateBuffer(g_Device, &buffer_info, g_Allocator, &g_UploadBuffer);
        ImGui_ImplGlfwVulkan_VkResult(err);
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(g_Device, g_UploadBuffer, &req);
        g_BufferMemoryAlignment = (g_BufferMemoryAlignment > req.alignment) ? g_BufferMemoryAlignment : req.alignment;
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = ImGui_ImplGlfwVulkan_MemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
        err = vkAllocateMemory(g_Device, &alloc_info, g_Allocator, &g_UploadBufferMemory);
        ImGui_ImplGlfwVulkan_VkResult(err);
        err = vkBindBufferMemory(g_Device, g_UploadBuffer, g_UploadBufferMemory, 0);
        ImGui_ImplGlfwVulkan_VkResult(err);
    }

    // Upload to Buffer:
    {
        char* map = NULL;
        err = vkMapMemory(g_Device, g_UploadBufferMemory, 0, upload_size, 0, (void**)(&map));
        ImGui_ImplGlfwVulkan_VkResult(err);
        memcpy(map, pixels, upload_size);
        VkMappedMemoryRange range[1] = {};
        range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[0].memory = g_UploadBufferMemory;
        range[0].size = upload_size;
        err = vkFlushMappedMemoryRanges(g_Device, 1, range);
        ImGui_ImplGlfwVulkan_VkResult(err);
        vkUnmapMemory(g_Device, g_UploadBufferMemory);
    }
    // Copy to Image:
    {
        VkImageMemoryBarrier copy_barrier[1] = {};
        copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[0].image = g_FontImage;
        copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_barrier[0].subresourceRange.levelCount = 1;
        copy_barrier[0].subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier);

        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = width;
        region.imageExtent.height = height;
        vkCmdCopyBufferToImage(command_buffer, g_UploadBuffer, g_FontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VkImageMemoryBarrier use_barrier[1] = {};
        use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[0].image = g_FontImage;
        use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        use_barrier[0].subresourceRange.levelCount = 1;
        use_barrier[0].subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier);
    }

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontImage;

    return true;
}

bool ImGui_ImplGlfwVulkan_CreateDeviceObjects()
{
    VkResult err;
    VkShaderModule vert_module;
    VkShaderModule frag_module;

    // Create The Shader Modules:
    {
        VkShaderModuleCreateInfo vert_info = {};
        vert_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vert_info.codeSize = __glsl_shader_vert_spv_len;
        vert_info.pCode = (uint32_t*)__glsl_shader_vert_spv;
        err = vkCreateShaderModule(g_Device, &vert_info, g_Allocator, &vert_module);
        ImGui_ImplGlfwVulkan_VkResult(err);
        VkShaderModuleCreateInfo frag_info = {};
        frag_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        frag_info.codeSize = __glsl_shader_frag_spv_len;
        frag_info.pCode = (uint32_t*)__glsl_shader_frag_spv;
        err = vkCreateShaderModule(g_Device, &frag_info, g_Allocator, &frag_module);
        ImGui_ImplGlfwVulkan_VkResult(err);
    }

    if (!g_FontSampler)
    {
        VkSamplerCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.minLod = -1000;
        info.maxLod = 1000;
        err = vkCreateSampler(g_Device, &info, g_Allocator, &g_FontSampler);
        ImGui_ImplGlfwVulkan_VkResult(err);
    }

    if (!g_DescriptorSetLayout)
    {
        VkSampler sampler[1] = {g_FontSampler};
        VkDescriptorSetLayoutBinding binding[1] = {};
        binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding[0].descriptorCount = 1;
        binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding[0].pImmutableSamplers = sampler;
        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings = binding;
        err = vkCreateDescriptorSetLayout(g_Device, &info, g_Allocator, &g_DescriptorSetLayout);
        ImGui_ImplGlfwVulkan_VkResult(err);
    }

    // Create Descriptor Set:
    {
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = g_DescriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &g_DescriptorSetLayout;
        err = vkAllocateDescriptorSets(g_Device, &alloc_info, &g_DescriptorSet);
        ImGui_ImplGlfwVulkan_VkResult(err);
    }

    if (!g_PipelineLayout)
    {
        VkPushConstantRange push_constants[2] = {};
        push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[0].offset = sizeof(float) * 0;
        push_constants[0].size = sizeof(float) * 2;
        push_constants[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[1].offset = sizeof(float) * 2;
        push_constants[1].size = sizeof(float) * 2;
        VkDescriptorSetLayout set_layout[1] = {g_DescriptorSetLayout};
        VkPipelineLayoutCreateInfo layout_info = {};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = set_layout;
        layout_info.pushConstantRangeCount = 2;
        layout_info.pPushConstantRanges = push_constants;
        err = vkCreatePipelineLayout(g_Device, &layout_info, g_Allocator, &g_PipelineLayout);
        ImGui_ImplGlfwVulkan_VkResult(err);
    }

    VkPipelineShaderStageCreateInfo stage[2] = {};
    stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stage[0].module = vert_module;
    stage[0].pName = "main";
    stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stage[1].module = frag_module;
    stage[1].pName = "main";

    VkVertexInputBindingDescription binding_desc[1] = {};
    binding_desc[0].stride = sizeof(ImDrawVert);
    binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attribute_desc[3] = {};
    attribute_desc[0].location = 0;
    attribute_desc[0].binding = binding_desc[0].binding;
    attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_desc[0].offset = (size_t)(&((ImDrawVert*)0)->pos);
    attribute_desc[1].location = 1;
    attribute_desc[1].binding = binding_desc[0].binding;
    attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_desc[1].offset = (size_t)(&((ImDrawVert*)0)->uv);
    attribute_desc[2].location = 2;
    attribute_desc[2].binding = binding_desc[0].binding;
    attribute_desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    attribute_desc[2].offset = (size_t)(&((ImDrawVert*)0)->col);

    VkPipelineVertexInputStateCreateInfo vertex_info = {};
    vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_info.vertexBindingDescriptionCount = 1;
    vertex_info.pVertexBindingDescriptions = binding_desc;
    vertex_info.vertexAttributeDescriptionCount = 3;
    vertex_info.pVertexAttributeDescriptions = attribute_desc;

    VkPipelineInputAssemblyStateCreateInfo ia_info = {};
    ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster_info = {};
    raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_info.polygonMode = VK_POLYGON_MODE_FILL;
    raster_info.cullMode = VK_CULL_MODE_NONE;
    raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo ms_info = {};
    ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_attachment[1] = {};
    color_attachment[0].blendEnable = VK_TRUE;
    color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
    color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
    color_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend_info = {};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = color_attachment;

    VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    VkGraphicsPipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.flags = g_PipelineCreateFlags;
    info.stageCount = 2;
    info.pStages = stage;
    info.pVertexInputState = &vertex_info;
    info.pInputAssemblyState = &ia_info;
    info.pViewportState = &viewport_info;
    info.pRasterizationState = &raster_info;
    info.pMultisampleState = &ms_info;
    info.pColorBlendState = &blend_info;
    info.pDynamicState = &dynamic_state;
    info.layout = g_PipelineLayout;
    info.renderPass = g_RenderPass;
    err = vkCreateGraphicsPipelines(g_Device, g_PipelineCache, 1, &info, g_Allocator, &g_Pipeline);
    ImGui_ImplGlfwVulkan_VkResult(err);

    vkDestroyShaderModule(g_Device, vert_module, g_Allocator);
    vkDestroyShaderModule(g_Device, frag_module, g_Allocator);

    return true;
}

void    ImGui_ImplGlfwVulkan_InvalidateFontUploadObjects()
{
    if (g_UploadBuffer)
    {
        vkDestroyBuffer(g_Device, g_UploadBuffer, g_Allocator);
        g_UploadBuffer = VK_NULL_HANDLE;
    }
    if (g_UploadBufferMemory)
    {
        vkFreeMemory(g_Device, g_UploadBufferMemory, g_Allocator);
        g_UploadBufferMemory = VK_NULL_HANDLE;
    }
}

void    ImGui_ImplGlfwVulkan_InvalidateDeviceObjects()
{
    ImGui_ImplGlfwVulkan_InvalidateFontUploadObjects();
    for (int i=0; i<IMGUI_VK_QUEUED_FRAMES; i++)
    {
        if (g_VertexBuffer[i])
            vkDestroyBuffer(g_Device, g_VertexBuffer[i], g_Allocator);
        if (g_VertexBufferMemory[i])
            vkFreeMemory(g_Device, g_VertexBufferMemory[i], g_Allocator);
        if (g_IndexBuffer[i])
            vkDestroyBuffer(g_Device, g_IndexBuffer[i], g_Allocator);
        if (g_IndexBufferMemory[i])
            vkFreeMemory(g_Device, g_IndexBufferMemory[i], g_Allocator);
    }

    if (g_FontView)
        vkDestroyImageView(g_Device, g_FontView, g_Allocator);
    if (g_FontImage)
        vkDestroyImage(g_Device, g_FontImage, g_Allocator);
    if (g_FontMemory)
        vkFreeMemory(g_Device, g_FontMemory, g_Allocator);
    if (g_FontSampler)
        vkDestroySampler(g_Device, g_FontSampler, g_Allocator);

    if (g_DescriptorSetLayout)
        vkDestroyDescriptorSetLayout(g_Device, g_DescriptorSetLayout, g_Allocator);
    if (g_PipelineLayout)
        vkDestroyPipelineLayout(g_Device, g_PipelineLayout, g_Allocator);
    if (g_Pipeline)
        vkDestroyPipeline(g_Device, g_Pipeline, g_Allocator);
}

bool    ImGui_ImplGlfwVulkan_Init(GLFWwindow* window, bool install_callbacks, ImGui_ImplGlfwVulkan_Init_Data *init_data)
{
    g_Allocator = init_data->allocator;
    g_Gpu = init_data->gpu;
    g_Device = init_data->device;
    g_RenderPass = init_data->render_pass;
    g_PipelineCache = init_data->pipeline_cache;
    g_DescriptorPool = init_data->descriptor_pool;
    g_CheckVkResult = init_data->check_vk_result;

    g_Window = window;

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                         // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.RenderDrawListsFn = ImGui_ImplGlfwVulkan_RenderDrawLists;       // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = ImGui_ImplGlfwVulkan_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGlfwVulkan_GetClipboardText;
#ifdef _WIN32
    io.ImeWindowHandle = glfwGetWin32Window(g_Window);
#endif

    if (install_callbacks)
    {
        glfwSetMouseButtonCallback(window, ImGui_ImplGlfwVulkan_MouseButtonCallback);
        glfwSetScrollCallback(window, ImGui_ImplGlfwVulkan_ScrollCallback);
        glfwSetKeyCallback(window, ImGui_ImplGlfwVulkan_KeyCallback);
        glfwSetCharCallback(window, ImGui_ImplGlfwVulkan_CharCallback);
    }

    ImGui_ImplGlfwVulkan_CreateDeviceObjects();

    return true;
}

void ImGui_ImplGlfwVulkan_Shutdown()
{
    ImGui_ImplGlfwVulkan_InvalidateDeviceObjects();
    ImGui::Shutdown();
}

void ImGui_ImplGlfwVulkan_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(g_Window, &w, &h);
    glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

    // Setup time step
    double current_time =  glfwGetTime();
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    if (glfwGetWindowAttrib(g_Window, GLFW_FOCUSED))
    {
        double mouse_x, mouse_y;
        glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
        io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
    }
    else
    {
        io.MousePos = ImVec2(-1,-1);
    }

    for (int i = 0; i < 3; i++)
    {
        io.MouseDown[i] = g_MousePressed[i] || glfwGetMouseButton(g_Window, i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        g_MousePressed[i] = false;
    }

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
    glfwSetInputMode(g_Window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    // Start the frame
    ImGui::NewFrame();
}
void ImGui_ImplGlfwVulkan_Render(VkCommandBuffer command_buffer)
{
    g_CommandBuffer = command_buffer;
    ImGui::Render();
    g_CommandBuffer = VK_NULL_HANDLE;
    g_FrameIndex = (g_FrameIndex + 1) % IMGUI_VK_QUEUED_FRAMES;
}
