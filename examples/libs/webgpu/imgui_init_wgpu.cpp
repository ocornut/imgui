#include <assert.h>
#include "imgui_init_webgpu.h"


#ifdef __EMSCRIPTEN__
// Adapter and device initialization via JS
EM_ASYNC_JS( void, getAdapterAndDeviceViaJS, (),
{
    if (!navigator.gpu) throw Error("WebGPU not supported.");

    const adapter = await navigator.gpu.requestAdapter();
    const device = await adapter.requestDevice();
    Module.preinitializedWebGPUDevice = device;
} );
#else

#ifdef __cplusplus
extern "C" {
#endif
    WGPUSurface GLFW_getWGPUSurface(WGPUInstance instance, void* window);
#ifdef __cplusplus
}
#endif

static void handle_request_adapter(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *userdata1, void *userdata2)
{
    if (status == WGPURequestAdapterStatus_Success)
    {
        WGPUAdapter *extAdapter = (WGPUAdapter *) userdata1;
        *extAdapter = adapter;
    }
    else
        printf("Request_adapter status=%#.8x message=%.*s\n", status, (int) message.length, message.data);
}

static void handle_request_device(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *userdata1, void *userdata2)
{
    if (status == WGPURequestDeviceStatus_Success)
    {
        WGPUDevice *extDevice = (WGPUDevice *) userdata1;
        *extDevice = device;
    }
    else
        printf("Request_device status=%#.8x message=%.*s\n", status, (int) message.length, message.data);
}

static void log_callback(WGPULogLevel level, WGPUStringView message, void *userdata)
{
    const char *level_str = "";
    switch (level) {
        case WGPULogLevel_Error: level_str = "error"; break;
        case WGPULogLevel_Warn:  level_str = "warn";  break;
        case WGPULogLevel_Info:  level_str = "info";  break;
        case WGPULogLevel_Debug: level_str = "debug"; break;
        case WGPULogLevel_Trace: level_str = "trace"; break;
        default:                 level_str = "unknown_level";
    }
    fprintf(stderr, "[wgpu] [%s] %.*s\n", level_str, (int) message.length, message.data);
}
#endif

bool InitWGPU(void* window)
{
    wgpu_instance = wgpuCreateInstance(nullptr);

#ifdef __EMSCRIPTEN__
    getAdapterAndDeviceViaJS();

    wgpu_device   = emscripten_webgpu_get_device();
    assert(wgpu_device != nullptr && "Error creating the Device");

    WGPUSurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
    html_surface_desc.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
    html_surface_desc.selector    = "#canvas";

    WGPUSurfaceDescriptor surface_desc = {};
    surface_desc.nextInChain   = &html_surface_desc.chain;

    wgpu_surface       = wgpuInstanceCreateSurface(wgpu_instance, &surface_desc);
    wgpu_preferred_fmt = wgpuSurfaceGetPreferredFormat(wgpu_surface, {} /* adapter */);
#else
    wgpuSetLogCallback(log_callback, NULL);
    wgpuSetLogLevel(WGPULogLevel_Warn);

    WGPURequestAdapterOptions adapterOptions = {};
    adapterOptions.compatibleSurface = wgpu_surface;

    static WGPUAdapter localAdapter;
    WGPURequestAdapterCallbackInfo adapterCallbackInfo = {};
    adapterCallbackInfo.callback = handle_request_adapter;
    adapterCallbackInfo.userdata1 = &localAdapter;

    wgpuInstanceRequestAdapter(wgpu_instance, &adapterOptions, adapterCallbackInfo);
    assert(localAdapter && "Error on Adapter request");

#ifndef NDEBUG
    WGPUAdapterInfo info = {0};
    wgpuAdapterGetInfo(localAdapter, &info);
    printf("Using: %.*s - %.*s\n", (int) info.device.length, info.device.data, (int) info.description.length, info.description.data);
#endif

    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.callback = handle_request_device;
    deviceCallbackInfo.userdata1 = &wgpu_device;

    wgpuAdapterRequestDevice(localAdapter, NULL, deviceCallbackInfo);
    assert(wgpu_device && "Error on Device request");

#ifdef IMGUI_EXAMPLE_GLFW_WGPU
    wgpu_surface = GLFW_getWGPUSurface( wgpu_instance, (GLFWwindow*) window);
#elif IMGUI_EXAMPLE_SDL2_WGPU
    wgpu_surface = SDL2_getWGPUSurface( wgpu_instance, (SDL_Window*) window);
#elif IMGUI_EXAMPLE_SDL3_WGPU
    wgpu_surface = SDL3_getWGPUSurface( wgpu_instance, (SDL_Window *) window);
#endif
    if (!wgpu_surface)
        return false;

    WGPUSurfaceCapabilities surface_capabilities = {0};
    wgpuSurfaceGetCapabilities(wgpu_surface, localAdapter, &surface_capabilities);

    wgpu_preferred_fmt = surface_capabilities.formats[0];
#endif

    wgpu_surface_configuration.presentMode = WGPUPresentMode_Fifo;
    wgpu_surface_configuration.alphaMode   = WGPUCompositeAlphaMode_Auto;
    wgpu_surface_configuration.usage       = WGPUTextureUsage_RenderAttachment;
    wgpu_surface_configuration.width       = wgpu_surface_width;
    wgpu_surface_configuration.height      = wgpu_surface_height;
    wgpu_surface_configuration.device      = wgpu_device;
    wgpu_surface_configuration.format      = wgpu_preferred_fmt;

    wgpuSurfaceConfigure(wgpu_surface, &wgpu_surface_configuration);

    wgpu_queue    = wgpuDeviceGetQueue(wgpu_device);

    return true;
}
