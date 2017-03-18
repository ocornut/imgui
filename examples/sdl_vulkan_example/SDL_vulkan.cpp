#include <string.h>

#if HAVE_X11_XCB
#define VK_USE_PLATFORM_XCB_KHR
#include <X11/Xlib-xcb.h>
#else
#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "SDL_vulkan.h"
#include <SDL_syswm.h>

static SDL_bool SetNames(unsigned* count, const char** names, unsigned inCount, const char* const* inNames)
{
	unsigned capacity = *count;
	*count = inCount;
	if ( names ) {
		if ( capacity < inCount ) {
			SDL_SetError("Insufficient capacity for extension names: %u < %u", capacity, inCount);
			return SDL_FALSE;
		}
		for ( unsigned i = 0; i < inCount; ++i )
			names[i] = inNames[i];
	}
	return SDL_TRUE;
}

SDL_bool SDL_vk_GetInstanceExtensions(unsigned* count, const char** names)
{
	const char *driver = SDL_GetCurrentVideoDriver();
	if ( !driver ) {
		SDL_SetError("No video driver - has SDL_Init(SDL_INIT_VIDEO) been called?");
		return SDL_FALSE;
	}
	if ( !count ) {
		SDL_SetError("'count' is null");
		return SDL_FALSE;
	}

#if HAVE_X11_XCB
	if ( !strcmp(driver, "x11") ) {
		const char* ext[] = { VK_KHR_XCB_SURFACE_EXTENSION_NAME };
		return SetNames(count, names, 1, ext);
	}
#elif defined(_WIN32)
	if ( !strcmp(driver, "windows") ) {
		const char* ext[] = { VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
		return SetNames(count, names, 1, ext);
	}
#endif

	SDL_SetError("Unsupported video driver '%s'", driver);
	return SDL_FALSE;
}

SDL_bool SDL_vk_CreateSurface(SDL_Window* window, VkInstance instance, VkSurfaceKHR* surface)
{
	if ( !window ) {
		SDL_SetError("'window' is null");
		return SDL_FALSE;
	}
	if ( instance == VK_NULL_HANDLE ) {
		SDL_SetError("'instance' is null");
		return SDL_FALSE;
	}

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	if ( !SDL_GetWindowWMInfo(window, &wminfo) )
		return SDL_FALSE;

	switch ( wminfo.subsystem ) {
#if HAVE_X11_XCB
	case SDL_SYSWM_X11:
	{
		VkXcbSurfaceCreateInfoKHR createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
		createInfo.pNext = NULL;
		createInfo.flags = 0;
		createInfo.connection = XGetXCBConnection(wminfo.info.x11.display);
		createInfo.window = wminfo.info.x11.window;

		VkResult r = vkCreateXcbSurfaceKHR(instance, &createInfo, NULL, surface);
		if ( r != VK_SUCCESS ) {
			SDL_SetError("vkCreateXcbSurfaceKHR failed: %i", (int)r);
			return SDL_FALSE;
		}
		return SDL_TRUE;
	}
#elif defined(_WIN32)
	case SDL_SYSWM_WINDOWS:
	{
		VkWin32SurfaceCreateInfoKHR createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.pNext = NULL;
		createInfo.flags = 0;
		createInfo.hinstance = GetModuleHandle(NULL);
		createInfo.hwnd = wminfo.info.win.window;

		VkResult r = vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, surface);
		if ( r != VK_SUCCESS ) {
			SDL_SetError("vkCreateXcbSurfaceKHR failed: %i", (int)r);
			return SDL_FALSE;
		}
		return SDL_TRUE;
	}
#endif
	default:
		SDL_SetError("Unsupported subsystem %i", (int)wminfo.subsystem);
		return SDL_FALSE;
	}
}
