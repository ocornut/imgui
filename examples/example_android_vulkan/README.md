# Android Vulkan + Dear ImGui Example

A standalone Android application that renders [Dear ImGui](https://github.com/ocornut/imgui) using the Vulkan graphics API via `NativeActivity`. No Java/Kotlin code required — pure C++.

![ImGui Demo](https://img.shields.io/badge/ImGui-1.92.6-blue) ![Vulkan](https://img.shields.io/badge/Vulkan-1.0-red) ![Android](https://img.shields.io/badge/Android-API%2026+-green)

## Features

- **Pure native** — `NativeActivity` with `android_native_app_glue`, no Java layer
- **Vulkan rendering** — Instance, Device, Swapchain, RenderPass all managed via ImGui helpers
- **Touch input** — `imgui_impl_android.h` handles multi-touch natively
- **Orientation handling** — Swapchain auto-rebuilds on rotation
- **Modular code** — Clean separation into `vulkan_helper.h`, `menu.h`, and `main.cpp`

## Requirements

| Requirement    | Version          |
| -------------- | ---------------- |
| Android Studio | 2024.x+          |
| Android NDK    | 29.0.x           |
| CMake          | 3.22.1           |
| Min SDK        | 26 (Android 8.0) |
| Target SDK     | 36               |
| ABI            | `arm64-v8a`      |

## Project Structure

```
app/src/main/
├── AndroidManifest.xml          ← NativeActivity config
└── cpp/
    ├── CMakeLists.txt           ← Build config (links imgui + vulkan)
    ├── main.cpp                 ← Entry point: lifecycle (Init, MainLoopStep, Shutdown)
    ├── vulkan_helper.h          ← vkh:: namespace — Vulkan setup, rendering, cleanup
    └── menu.h                   ← menu:: namespace — Your ImGui UI (edit this!)
```

ImGui sources are referenced from the parent imgui directory (`../../../../../../`).

## Build & Run

```bash
# Build
./gradlew assembleDebug

# Build + install on connected device
./gradlew installDebug

# View logs
adb logcat -s ImGuiVulkan
```

## Customizing the UI

Edit **`menu.h`** — the `menu::Draw()` function is called every frame:

```cpp
namespace menu
{
    inline void Draw()
    {
        ImGui::Begin("My App");
        ImGui::Text("Hello!");
        // Add your widgets here...
        ImGui::End();
    }
}
```

All ImGui widgets work out of the box: windows, buttons, sliders, checkboxes, color pickers, popups, combo boxes, etc.

## How It Works

1. **`android_main()`** — NativeActivity entry point, runs the event + render loop
2. **`Init()`** — Called on `APP_CMD_INIT_WINDOW`:
    - Creates `VkInstance`, `VkDevice`, `VkQueue`
    - Creates `VkSurfaceKHR` from `ANativeWindow`
    - Sets up swapchain, render pass, framebuffers via `ImGui_ImplVulkanH_Window`
    - Initializes Dear ImGui with `imgui_impl_android` + `imgui_impl_vulkan`
3. **`MainLoopStep()`** — Called every frame:
    - Checks for orientation/size changes → rebuilds swapchain if needed
    - Calls `ImGui_ImplVulkan_NewFrame()` + `ImGui_ImplAndroid_NewFrame()`
    - Calls `menu::Draw()` for your UI
    - Records Vulkan command buffer + presents
4. **`Shutdown()`** — Called on `APP_CMD_TERM_WINDOW`, cleans up everything

## Key Files Reference

| File                  | What to modify                                  |
| --------------------- | ----------------------------------------------- |
| `menu.h`              | UI widgets — add buttons, sliders, windows      |
| `vulkan_helper.h`     | Vulkan settings (present mode, min image count) |
| `main.cpp`            | App lifecycle, ImGui style/scaling              |
| `AndroidManifest.xml` | App name, permissions, orientation              |

## License

This example uses [Dear ImGui](https://github.com/ocornut/imgui) which is licensed under the MIT License.
