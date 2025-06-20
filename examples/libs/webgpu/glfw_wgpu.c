#include <webgpu/wgpu.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#if defined(GLFW_EXPOSE_NATIVE_COCOA)
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

WGPUSurface GLFW_getWGPUSurface(WGPUInstance instance, GLFWwindow* window)
{
    WGPUSurfaceDescriptor surfaceDescriptor = {};
    WGPUChainedStruct     chainedStruct     = {};

#if defined(GLFW_EXPOSE_NATIVE_COCOA)
    {
        id metal_layer = NULL;
        NSWindow *ns_window = glfwGetCocoaWindow(window);
        [ns_window.contentView setWantsLayer:YES];
        metal_layer = [CAMetalLayer layer];
        [ns_window.contentView setLayer:metal_layer];

        chainedStruct.sType = WGPUSType_SurfaceSourceMetalLayer;

        WGPUSurfaceSourceMetalLayer surfaceMetal = {};
        surfaceMetal.chain = chainedStruct;
        surfaceMetal.layer = metal_layer;

        surfaceDescriptor.nextInChain = &surfaceMetal.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND) && defined(GLFW_EXPOSE_NATIVE_X11)
    if (glfwGetPlatform() == GLFW_PLATFORM_X11) {
        Display *x11_display = glfwGetX11Display();
        Window x11_window = glfwGetX11Window(window);

        chainedStruct.sType = WGPUSType_SurfaceSourceXlibWindow;

        WGPUSurfaceSourceXlibWindow surfaceXlib = {};
        surfaceXlib.chain   = chainedStruct;
        surfaceXlib.display = x11_display;
        surfaceXlib.window  = x11_window;

        surfaceDescriptor.nextInChain = &surfaceXlib.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }

    if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
        struct wl_display *wayland_display = glfwGetWaylandDisplay();
        struct wl_surface *wayland_surface = glfwGetWaylandWindow(window);

        chainedStruct.sType = WGPUSType_SurfaceSourceWaylandSurface;

        WGPUSurfaceSourceWaylandSurface surfaceWayland = {};
        surfaceWayland.chain   = chainedStruct;
        surfaceWayland.display = wayland_display;
        surfaceWayland.surface = wayland_surface;

        surfaceDescriptor.nextInChain = &surfaceWayland.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#elif defined(GLFW_EXPOSE_NATIVE_WIN32)
    {
        HWND hwnd = glfwGetWin32Window(window);
        HINSTANCE hinstance = GetModuleHandle(NULL);

        chainedStruct.sType = WGPUSType_SurfaceSourceWindowsHWND;

        WGPUSurfaceSourceWindowsHWND surfaceHWND = {};
        surfaceHWND.chain     = chainedStruct;
        surfaceHWND.hinstance = hinstance;
        surfaceHWND.hwnd      = hwnd;

        surfaceDescriptor.nextInChain = &surfaceHWND.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#else
#error "Unsupported GLFW/WebGPU native platform"
#endif
}

#ifdef __cplusplus
}
#endif
