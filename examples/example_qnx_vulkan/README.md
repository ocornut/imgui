# Dear ImGui QNX Screen + Vulkan example

This example uses:

- `imgui_impl_qnx.cpp` as the QNX Screen platform backend
- the unmodified `imgui_impl_vulkan.cpp` renderer backend
- `vkCreateScreenSurfaceQNX()` and `VK_QNX_screen_surface`

No separate Screen/Vulkan renderer backend is required. The QNX-specific work
is limited to the Screen platform backend, native window setup, and Vulkan WSI
surface creation.

## Build

Load the QNX SDP environment, then run:

```sh
cd examples/example_qnx_vulkan
make
```

The build defines `VK_USE_PLATFORM_SCREEN_QNX` and links `libvulkan` and
`libscreen`.

## Input and resize

The application forwards Screen input events to `ImGui_ImplQNX_ProcessEvent()`.
It recreates the Vulkan swapchain when the Screen window size changes or Vulkan
reports `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR`. Escape exits.
