_(You may browse this at https://github.com/ocornut/imgui/blob/master/docs/EXAMPLES.md or view this file with any Markdown viewer)_

## Dear ImGui: Examples

**The [examples/](https://github.com/ocornut/imgui/blob/master/examples) folder example applications (standalone, ready-to-build) for variety of
platforms and graphics APIs.** They all use standard backends from the [backends/](https://github.com/ocornut/imgui/blob/master/backends) folder (see [BACKENDS.md](https://github.com/ocornut/imgui/blob/master/docs/BACKENDS.md)).

The purpose of Examples is to showcase integration with backends, let you try Dear ImGui, and guide you toward
integrating Dear ImGui in your own application/game/engine.
**Once Dear ImGui is setup and running, run and refer to `ImGui::ShowDemoWindow()` in imgui_demo.cpp for usage of the end-user API.**

You can find Windows binaries for some of those example applications at:
  https://www.dearimgui.com/binaries


### Getting Started

Integration in a typical existing application, should take <20 lines when using standard backends.

```cpp
At initialization:
  call ImGui::CreateContext()
  call ImGui_ImplXXXX_Init() for each backend.

At the beginning of your frame:
  call ImGui_ImplXXXX_NewFrame() for each backend.
  call ImGui::NewFrame()

At the end of your frame:
  call ImGui::Render()
  call ImGui_ImplXXXX_RenderDrawData() for your Renderer backend.

At shutdown:
  call ImGui_ImplXXXX_Shutdown() for each backend.
  call ImGui::DestroyContext()
```

Main resource:
- Read **[Getting Started](https://github.com/ocornut/imgui/wiki/Getting-Started) wiki guide** for detailed examples of how to integrate Dear ImGui in an existing application.

Additional resources:
- Read FAQ at https://www.dearimgui.com/faq
- Read 'PROGRAMMER GUIDE' section in imgui.cpp.
- Read the comments and instruction at the top of each file.

If you are using any of the backends provided here, you can add the backends/imgui_impl_xxxx(.cpp,.h)
files to your project and use as-in. Each imgui_impl_xxxx.cpp file comes with its own individual
Changelog, so if you want to update them later it will be easier to catch up with what changed.


### Examples Applications

[example_allegro5/](https://github.com/ocornut/imgui/blob/master/examples/example_allegro5/) <BR>
Allegro 5 example. <BR>
= main.cpp + imgui_impl_allegro5.cpp

[example_android_opengl3/](https://github.com/ocornut/imgui/blob/master/examples/example_android_opengl3/) <BR>
Android + OpenGL3 (ES) example. <BR>
= main.cpp + imgui_impl_android.cpp + imgui_impl_opengl3.cpp

[example_apple_metal/](https://github.com/ocornut/imgui/blob/master/examples/example_metal/) <BR>
OSX & iOS + Metal example. <BR>
= main.m + imgui_impl_osx.mm + imgui_impl_metal.mm <BR>
It is based on the "cross-platform" game template provided with Xcode as of Xcode 9.
(NB: imgui_impl_osx.mm is currently not as feature complete as other platforms backends.
You may prefer to use the GLFW Or SDL backends, which will also support Windows and Linux.)

[example_apple_opengl2/](https://github.com/ocornut/imgui/blob/master/examples/example_apple_opengl2/) <BR>
OSX + OpenGL2 example. <BR>
= main.mm + imgui_impl_osx.mm + imgui_impl_opengl2.cpp <BR>
(NB: imgui_impl_osx.mm is currently not as feature complete as other platforms backends.
 You may prefer to use the GLFW Or SDL backends, which will also support Windows and Linux.)

[example_glfw_wgpu/](https://github.com/ocornut/imgui/blob/master/examples/example_glfw_wgpu/) <BR>
GLFW + WebGPU example. Supports Emscripten (web) or Dawn (desktop) <BR>
= main.cpp + imgui_impl_glfw.cpp + imgui_impl_wgpu.cpp
Note that the 'example_glfw_opengl3' and 'example_sdl2_opengl3' examples also supports Emscripten!

[example_glfw_metal/](https://github.com/ocornut/imgui/blob/master/examples/example_glfw_metal/) <BR>
GLFW (Mac) + Metal example. <BR>
= main.mm + imgui_impl_glfw.cpp + imgui_impl_metal.mm

[example_glfw_opengl2/](https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl2/) <BR>
GLFW + OpenGL2 example (legacy, fixed pipeline). <BR>
= main.cpp + imgui_impl_glfw.cpp + imgui_impl_opengl2.cpp <BR>
**DO NOT USE THIS IF YOUR CODE/ENGINE IS USING MODERN GL or WEBGL (SHADERS, VBO, VAO, etc.)** <BR>
This code is mostly provided as a reference to learn about Dear ImGui integration, because it is shorter.
If your code is using GL3+ context or any semi modern GL calls, using this renderer is likely to
make things more complicated, will require your code to reset many GL attributes to their initial
state, and might confuse your GPU driver. One star, not recommended.

[example_glfw_opengl3/](https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl3/) <BR>
GLFW (Win32, Mac, Linux) + OpenGL3+/ES2/ES3 example (modern, programmable pipeline). <BR>
= main.cpp + imgui_impl_glfw.cpp + imgui_impl_opengl3.cpp <BR>
This uses more modern GL calls and custom shaders.<BR>
This support building with Emscripten and targeting WebGL.<BR>
Prefer using that if you are using modern GL or WebGL in your application.

[example_glfw_vulkan/](https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/) <BR>
GLFW (Win32, Mac, Linux) + Vulkan example. <BR>
= main.cpp + imgui_impl_glfw.cpp + imgui_impl_vulkan.cpp <BR>
This is quite long and tedious, because: Vulkan.
For this example, the main.cpp file exceptionally use helpers function from imgui_impl_vulkan.h/cpp.

[example_glut_opengl2/](https://github.com/ocornut/imgui/blob/master/examples/example_glut_opengl2/) <BR>
GLUT (e.g., FreeGLUT on Linux/Windows, GLUT framework on OSX) + OpenGL2 example. <BR>
= main.cpp + imgui_impl_glut.cpp + imgui_impl_opengl2.cpp <BR>
Note that GLUT/FreeGLUT is largely obsolete software, prefer using GLFW or SDL.

[example_null/](https://github.com/ocornut/imgui/blob/master/examples/example_null/) <BR>
Null example, compile and link imgui, create context, run headless with no inputs and no graphics output. <BR>
= main.cpp <BR>
This is used to quickly test compilation of core imgui files in as many setups as possible.
Because this application doesn't create a window nor a graphic context, there's no graphics output.

[example_sdl2_directx11/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl2_directx11/) <BR>
SDL2 + DirectX11 example, Windows only. <BR>
= main.cpp + imgui_impl_sdl2.cpp + imgui_impl_dx11.cpp <BR>
This to demonstrate usage of DirectX with SDL2.

[example_sdl2_metal/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl2_metal/) <BR>
SDL2 + Metal example, Mac only. <BR>
= main.mm + imgui_impl_sdl2.cpp + imgui_impl_metal.mm

[example_sdl2_opengl2/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl2_opengl2/) <BR>
SDL2 (Win32, Mac, Linux etc.) + OpenGL example (legacy, fixed pipeline). <BR>
= main.cpp + imgui_impl_sdl2.cpp + imgui_impl_opengl2.cpp <BR>
**DO NOT USE OPENGL2 CODE IF YOUR CODE/ENGINE IS USING GL OR WEBGL (SHADERS, VBO, VAO, etc.)** <BR>
This code is mostly provided as a reference to learn about Dear ImGui integration, because it is shorter.
If your code is using GL3+ context or any semi modern GL calls, using this renderer is likely to
make things more complicated, will require your code to reset many GL attributes to their initial
state, and might confuse your GPU driver. One star, not recommended.

[example_sdl2_opengl3/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl2_opengl3/) <BR>
SDL2 (Win32, Mac, Linux, etc.) + OpenGL3+/ES2/ES3 example. <BR>
= main.cpp + imgui_impl_sdl2.cpp + imgui_impl_opengl3.cpp <BR>
This uses more modern GL calls and custom shaders. <BR>
This support building with Emscripten and targeting WebGL.<BR>
Prefer using that if you are using modern GL or WebGL in your application.

[example_sdl2_sdlrenderer2/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl2_sdlrenderer2/) <BR>
SDL2 (Win32, Mac, Linux, etc.) + SDL_Renderer for SDL2 example.<BR>
= main.cpp + imgui_impl_sdl2.cpp + imgui_impl_sdlrenderer2.cpp <BR>
This requires SDL 2.0.18+ (released November 2021) <BR>

[example_sdl2_vulkan/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl2_vulkan/) <BR>
SDL2 (Win32, Mac, Linux, etc.) + Vulkan example. <BR>
= main.cpp + imgui_impl_sdl2.cpp + imgui_impl_vulkan.cpp <BR>
This is quite long and tedious, because: Vulkan. <BR>
For this example, the main.cpp file exceptionally use helpers function from imgui_impl_vulkan.h/cpp.

[example_sdl3_opengl3/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl3_opengl3/) <BR>
SDL3 (Win32, Mac, Linux, etc.) + OpenGL3+/ES2/ES3 example. <BR>
= main.cpp + imgui_impl_sdl3.cpp + imgui_impl_opengl3.cpp <BR>
This uses more modern GL calls and custom shaders. <BR>
This support building with Emscripten and targeting WebGL.<BR>

[example_sdl3_sdlgpu3/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl3_sdlgpu3/) <BR>
SDL3 (Win32, Mac, Linux, etc.) + SDL_GPU for SDL3 example.<BR>
= main.cpp + imgui_impl_sdl3.cpp + imgui_impl_sdlrenderer3.cpp <BR>

[example_sdl3_sdlrenderer3/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl3_sdlrenderer3/) <BR>
SDL3 (Win32, Mac, Linux, etc.) + SDL_Renderer for SDL3 example.<BR>
= main.cpp + imgui_impl_sdl3.cpp + imgui_impl_sdlrenderer3.cpp <BR>

[example_sdl3_vulkan/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl3_vulkan/) <BR>
SDL3 (Win32, Mac, Linux, etc.) + Vulkan example. <BR>
= main.cpp + imgui_impl_sdl3.cpp + imgui_impl_vulkan.cpp <BR>
This is quite long and tedious, because: Vulkan. <BR>
For this example, the main.cpp file exceptionally use helpers function from imgui_impl_vulkan.h/cpp.

[example_win32_directx9/](https://github.com/ocornut/imgui/blob/master/examples/example_win32_directx9/) <BR>
DirectX9 example, Windows only. <BR>
= main.cpp + imgui_impl_win32.cpp + imgui_impl_dx9.cpp

[example_win32_directx10/](https://github.com/ocornut/imgui/blob/master/examples/example_win32_directx10/) <BR>
DirectX10 example, Windows only. <BR>
= main.cpp + imgui_impl_win32.cpp + imgui_impl_dx10.cpp

[example_win32_directx11/](https://github.com/ocornut/imgui/blob/master/examples/example_win32_directx11/) <BR>
DirectX11 example, Windows only. <BR>
= main.cpp + imgui_impl_win32.cpp + imgui_impl_dx11.cpp

[example_win32_directx12/](https://github.com/ocornut/imgui/blob/master/examples/example_win32_directx12/) <BR>
DirectX12 example, Windows only. <BR>
= main.cpp + imgui_impl_win32.cpp + imgui_impl_dx12.cpp <BR>
This is quite long and tedious, because: DirectX12.

[example_win32_opengl3/](https://github.com/ocornut/imgui/blob/master/examples/example_win32_opengl3/) <BR>
Raw Windows + OpenGL3 example (modern, programmable pipeline) <BR>
= main.cpp + imgui_impl_win32.cpp + imgui_impl_opengl3.cpp <BR>

[example_win32_vulkan/](https://github.com/ocornut/imgui/blob/master/examples/example_win32_vulkan/) <BR>
Raw Windows + Vulkan example <BR>
= main.cpp + imgui_impl_win32.cpp + imgui_impl_vulkan.cpp <BR>


### Miscellaneous

**Building**

Unfortunately, nowadays it is still tedious to create and maintain portable build files using external
libraries (the kind we're using here to create a window and render 3D triangles) without relying on
third party software and build systems. For most examples here we choose to provide:
 - Makefiles for Linux/OSX
 - Batch files for Visual Studio 2008+
 - A .sln project file for Visual Studio 2012+
 - Xcode project files for the Apple examples
Please let us know if they don't work with your setup!
You can probably just import the imgui_impl_xxx.cpp/.h files into your own codebase or compile those
directly with a command-line compiler.

If you are interested in using Cmake to build and links examples, see:
  https://github.com/ocornut/imgui/pull/1713 and https://github.com/ocornut/imgui/pull/3027

**About mouse cursor latency**

Dear ImGui does not introduce significant extra lag for most behaviors,
e.g. the last value passed to 'io.AddMousePosEvent()' before NewFrame() will result in windows being moved
to the right spot at the time of EndFrame()/Render(). At 60 FPS your experience should be pleasant.

However, consider that OS mouse cursors are typically rendered through a very specific hardware-accelerated
path, which makes them feel smoother than the majority of content rendered via regular graphics API (including,
but not limited to Dear ImGui windows). Because UI rendering and interaction happens on the same plane
as the mouse, that disconnect may be jarring to particularly sensitive users.
You may experiment with enabling the io.MouseDrawCursor flag to request Dear ImGui to draw a mouse cursor
using the regular graphics API, to help you visualize the difference between a "hardware" cursor and a
regularly rendered software cursor.
However, rendering a mouse cursor at 60 FPS will feel sluggish, so you likely won't want to enable that at
all times. It might be beneficial for the user experience to switch to a software rendered cursor _only_
when an interactive drag is in progress.

Note that some setup configurations or GPU drivers may introduce additional display lag depending on their settings.
If you notice that dragging windows is laggy and you are not sure what the cause is: try drawing a simple
2D shape directly under the mouse cursor to help identify the issue!

