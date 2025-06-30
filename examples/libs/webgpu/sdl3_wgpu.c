#include <webgpu/webgpu.h>
#include <SDL3/SDL.h>

#if defined(SDL_PLATFORM_MACOS)
#  include <Cocoa/Cocoa.h>
#  include <QuartzCore/CAMetalLayer.h>
#elif defined(SDL_PLATFORM_WIN32)
#  include <windows.h>
#endif


WGPUSurface SDL3_getWGPUSurface(WGPUInstance instance, SDL_Window* window) {
    SDL_PropertiesID propertiesID = SDL_GetWindowProperties(window);
    WGPUSurfaceDescriptor surfaceDescriptor = {};
#if defined(SDL_PLATFORM_MACOS)
    {
        id metal_layer = NULL;
        NSWindow *ns_window = (__bridge NSWindow *)SDL_GetPointerProperty(propertiesID, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
        if (!ns_window) return NULL;
        [ns_window.contentView setWantsLayer : YES];
        metal_layer = [CAMetalLayer layer];
        [ns_window.contentView setLayer : metal_layer];

        WGPUSurfaceSourceMetalLayer surfaceMetal = {};
        surfaceMetal.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
        surfaceMetal.layer = metal_layer;

        surfaceDescriptor.nextInChain = &surfaceMetal.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#elif defined(SDL_PLATFORM_LINUX)
    if (!SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland")) {
        void *w_display = SDL_GetPointerProperty(propertiesID, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
        void *w_surface = SDL_GetPointerProperty(propertiesID, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
        if (!w_display || !w_surface) return NULL;

        WGPUSurfaceSourceWaylandSurface surfaceWayland = {};
        surfaceWayland.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
        surfaceWayland.display = w_display;
        surfaceWayland.surface = w_surface;

        surfaceDescriptor.nextInChain = &surfaceWayland.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    } else if (!SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11")) {
        void *x_display   = SDL_GetPointerProperty(propertiesID, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
        uint64_t x_window = SDL_GetNumberProperty(propertiesID, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        if (!x_display || !x_window) return NULL;

        WGPUSurfaceSourceXlibWindow surfaceXlib = {};
        surfaceXlib.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
        surfaceXlib.display = x_display;
        surfaceXlib.window  = x_window;

        surfaceDescriptor.nextInChain = &surfaceXlib.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }

#elif defined(SDL_PLATFORM_WIN32)
    {
        HWND hwnd = (HWND)SDL_GetPointerProperty(propertiesID, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        if (!hwnd) return NULL;
        HINSTANCE hinstance = GetModuleHandle(NULL);

        WGPUSurfaceSourceWindowsHWND surfaceHWND = {};
        surfaceHWND.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
        surfaceHWND.hinstance = hinstance;
        surfaceHWND.hwnd = hwnd;

        surfaceDescriptor.nextInChain = &surfaceHWND.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#else
    #error "Unsupported SDL3/WebGPU Backend"
#endif

}