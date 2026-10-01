// dear imgui: Renderer Backend for Vulkan, as a C++20 named module.
// See imgui.cppm for usage and design. Import with: 'import imgui; import imgui.impl.vulkan;'

module;

#define IMGUI_CXX_MODULE 1
#define IMGUI_CXX_MODULE_IMPORTED 1
#include "imgui_macros.h"
#include <stddef.h>     // offsetof
#include <stdint.h>     // UINT64_MAX

// Vulkan (same as imgui_impl_vulkan.h)
#if defined(IMGUI_IMPL_VULKAN_NO_PROTOTYPES) && !defined(VK_NO_PROTOTYPES)
#define VK_NO_PROTOTYPES
#endif
#if defined(VK_USE_PLATFORM_WIN32_KHR) && !defined(NOMINMAX)
#define NOMINMAX
#endif
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#ifdef IMGUI_IMPL_VULKAN_VOLK_FILENAME
#include IMGUI_IMPL_VULKAN_VOLK_FILENAME
#else
#include <volk.h>
#endif
#else
#include <vulkan/vulkan.h>
#endif

export module imgui.impl.vulkan;

import std.compat;
import imgui;

#include "imgui_impl_vulkan.h"

module :private;

#include "imgui_impl_vulkan.cpp"
