#include "imgui_init_webgpu.h"

static void wgpu_device_lost_callback(const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message)
{
    const char* reasonName = "";
    switch (reason) {
        case wgpu::DeviceLostReason::Unknown:           reasonName = "Unknown";         break;
        case wgpu::DeviceLostReason::Destroyed:         reasonName = "Destroyed";       break;
        case wgpu::DeviceLostReason::CallbackCancelled: reasonName = "InstanceDropped"; break;
        case wgpu::DeviceLostReason::FailedCreation:    reasonName = "FailedCreation";  break;
        default:                                        reasonName = "UNREACHABLE";     break;
    }
    printf("%s device message: %s\n", reasonName, message.data);
}

static void wgpu_error_callback(const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message)
{
    const char* errorTypeName = "";
    switch (type) {
        case wgpu::ErrorType::Validation:  errorTypeName = "Validation";      break;
        case wgpu::ErrorType::OutOfMemory: errorTypeName = "Out of memory";   break;
        case wgpu::ErrorType::Unknown:     errorTypeName = "Unknown";         break;
        case wgpu::ErrorType::Internal:    errorTypeName = "Internal";        break;
        default:                           errorTypeName = "UNREACHABLE";     break;
    }
    printf("%s error: %s\n", errorTypeName, message.data);
}

bool InitWGPU(void* window)
{
    wgpu::InstanceDescriptor instanceDescriptor  = {};
    instanceDescriptor.capabilities.timedWaitAnyEnable = true;
    wgpu::Instance instance = wgpu::CreateInstance(&instanceDescriptor);

    static wgpu::Adapter localAdapter;
    wgpu::RequestAdapterOptions adapterOptions;

    auto onRequestAdapter = [](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
        if (status != wgpu::RequestAdapterStatus::Success) {
            printf("Failed to get an adapter: %s\n", message.data);
            return;
        }
        localAdapter = std::move(adapter);
    };

    // Synchronously (wait until) acquire Adapter
    wgpu::Future  waitAdapterFunc { instance.RequestAdapter(&adapterOptions, wgpu::CallbackMode::WaitAnyOnly, onRequestAdapter) };
    wgpu::WaitStatus waitStatusAdapter = instance.WaitAny(waitAdapterFunc, UINT64_MAX);
    assert(localAdapter != nullptr && waitStatusAdapter == wgpu::WaitStatus::Success && "Error on Adapter request");

#ifndef NDEBUG
    wgpu::AdapterInfo info;
    localAdapter.GetInfo(&info);
    printf("Using: \" %s \"\n", info.device.data);
#endif

    // Set device callback functions
    wgpu::DeviceDescriptor deviceDesc;
    deviceDesc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous, wgpu_device_lost_callback);
    deviceDesc.SetUncapturedErrorCallback(wgpu_error_callback);

    // Synchronously (wait until) get Device
    static wgpu::Device acquiredDevice;
    auto onRequestDevice = [](wgpu::RequestDeviceStatus status, wgpu::Device localDevice, wgpu::StringView message) {
        if (status != wgpu::RequestDeviceStatus::Success) {
            printf("Failed to get an device: %s\n", message.data);
            return;
        }
        acquiredDevice = std::move(localDevice);
    };

    wgpu::Future waitDeviceFunc { localAdapter.RequestDevice(&deviceDesc, wgpu::CallbackMode::WaitAnyOnly, onRequestDevice) };
    wgpu::WaitStatus waitStatusDevice = instance.WaitAny(waitDeviceFunc, UINT64_MAX);
    assert(acquiredDevice != nullptr && waitStatusDevice == wgpu::WaitStatus::Success && "Error on Device request");

    wgpu_device = acquiredDevice.MoveToCHandle();

    // DAWN has also a member class func to create device: adapter.CreateDevice (only native)
    //device = localAdapter.CreateDevice(&deviceDesc);
    //assert(device != nullptr && "Error creating the Device");
#ifdef __EMSCRIPTEN__
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
    canvasDesc.selector = "#canvas";

    wgpu::SurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = &canvasDesc;
    wgpu::Surface surface = instance.CreateSurface(&surfaceDesc);
#else
    // Create the surface.
#ifdef IMGUI_EXAMPLE_GLFW_WGPU
    // Google DAWN "official" glfw integrated support
    wgpu::Surface surface = wgpu::glfw::CreateSurfaceForWindow(instance, (GLFWwindow *) window);
#elif IMGUI_EXAMPLE_SDL2_WGPU
    wgpu::Surface surface = SDL2_getWGPUSurface(instance.Get(), (SDL_Window *) window);
#elif IMGUI_EXAMPLE_SDL3_WGPU
    wgpu::Surface surface = SDL3_getWGPUSurface(instance.Get(), (SDL_Window *) window);
#endif
#endif
    if (!surface)
        return false;

    // Configure the surface.
    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(localAdapter, &capabilities);
    wgpu_preferred_fmt = (WGPUTextureFormat) capabilities.formats[0];

    wgpu_surface_configuration.presentMode = WGPUPresentMode_Fifo;
    wgpu_surface_configuration.alphaMode   = WGPUCompositeAlphaMode_Auto;
    wgpu_surface_configuration.usage       = WGPUTextureUsage_RenderAttachment;
    wgpu_surface_configuration.width       = wgpu_surface_width;
    wgpu_surface_configuration.height      = wgpu_surface_height;
    wgpu_surface_configuration.device      = wgpu_device;
    wgpu_surface_configuration.format      = wgpu_preferred_fmt;

    surface.Configure(reinterpret_cast<const wgpu::SurfaceConfiguration *>(&wgpu_surface_configuration));

    wgpu_instance = instance.MoveToCHandle();
    wgpu_surface  = surface.MoveToCHandle();
    wgpu_queue    = wgpuDeviceGetQueue(wgpu_device);

    return true;
}
