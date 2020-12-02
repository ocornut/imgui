_(You may browse this at https://github.com/ocornut/imgui/blob/master/docs/EXAMPLES.md or view this file with any Markdown viewer)_

## Dear ImGui: Examples

**The [examples/](https://github.com/ocornut/imgui/blob/master/examples) folder example applications (standalone, ready-to-build) for variety of
platforms and graphics APIs.** They all use standard backends from the [backends/](https://github.com/ocornut/imgui/blob/master/backends) folder.

You can find Windows binaries for some of those example applications at:
  http://www.dearimgui.org/binaries


### Getting Started

Integration in a typical existing application, should take <20 lines when using standard backends.

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

Example (using [backends/imgui_impl_win32.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_win32.cpp) + [backends/imgui_impl_dx11.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_dx11.cpp)):

    // Create a Dear ImGui context, setup some options
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable some options

    // Initialize Platform + Renderer backends (here: using imgui_impl_win32.cpp + imgui_impl_dx11.cpp)
    ImGui_ImplWin32_Init(my_hwnd);
    ImGui_ImplDX11_Init(my_d3d_device, my_d3d_device_context);

    // Application main loop
    while (true)
    {
        // Beginning of frame: update Renderer + Platform backend, start Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Any application code here
        ImGui::Text("Hello, world!");

        // End of frame: render Dear ImGui
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Swap
        g_pSwapChain->Present(1, 0);
    }

    // Shutdown
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

Please read 'PROGRAMMER GUIDE' in imgui.cpp for notes on how to setup Dear ImGui in your codebase.
Please read the comments and instruction at the top of each file.
Please read FAQ at http://www.dearimgui.org/faq

If you are using of the backend provided here, you can add the backends/imgui_impl_xxxx(.cpp,.h)
files to your project and use as-in. Each imgui_impl_xxxx.cpp file comes with its own individual
Changelog, so if you want to update them later it will be easier to catch up with what changed.


### Examples Applications

[example_allegro5/](https://github.com/ocornut/imgui/blob/master/examples/example_allegro5/) <BR>
Allegro 5 example. <BR>
= main.cpp + imgui_impl_allegro5.cpp

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

[example_emscripten_opengl3/](https://github.com/ocornut/imgui/blob/master/examples/example_emscripten_opengl3/) <BR>
Emcripten + SDL2 + OpenGL3+/ES2/ES3 example. <BR>
= main.cpp + imgui_impl_sdl.cpp + imgui_impl_opengl3.cpp <BR>
Note that other examples based on SDL or GLFW + OpenGL could easily be modified to work with Emscripten.
We provide this to make the Emscripten differences obvious, and have them not pollute all other examples.

[example_glfw_metal/](https://github.com/ocornut/imgui/blob/master/examples/example_glfw_metal/) <BR>
GLFW (Mac) + Metal example. <BR>
= main.mm + imgui_impl_glfw.cpp + imgui_impl_metal.mm

[example_glfw_opengl2/](https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl2/) <BR>
GLFW + OpenGL2 example (legacy, fixed pipeline). <BR>
= main.cpp + imgui_impl_glfw.cpp + imgui_impl_opengl2.cpp <BR>
**DO NOT USE OPENGL2 CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)** <BR>
**Prefer using OPENGL3 code (with gl3w/glew/glad/glad2/glbinding, you can replace the OpenGL function loader)** <BR>
This code is mostly provided as a reference to learn about Dear ImGui integration, because it is shorter.
If your code is using GL3+ context or any semi modern OpenGL calls, using this renderer is likely to
make things more complicated, will require your code to reset many OpenGL attributes to their initial
state, and might confuse your GPU driver. One star, not recommended.

[example_glfw_opengl3/](https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl3/) <BR>
GLFW (Win32, Mac, Linux) + OpenGL3+/ES2/ES3 example (programmable pipeline). <BR>
= main.cpp + imgui_impl_glfw.cpp + imgui_impl_opengl3.cpp <BR>
This uses more modern OpenGL calls and custom shaders. <BR>
Prefer using that if you are using modern OpenGL in your application (anything with shaders).
(Please be mindful that accessing OpenGL3+ functions requires a function loader, which are a frequent
source for confusion for new users. We use a loader in imgui_impl_opengl3.cpp which may be different
from the one your app normally use. Read imgui_impl_opengl3.h for details and how to change it.)

[example_glfw_vulkan/](https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/) <BR>
GLFW (Win32, Mac, Linux) + Vulkan example. <BR>
= main.cpp + imgui_impl_glfw.cpp + imgui_impl_vulkan.cpp <BR>
This is quite long and tedious, because: Vulkan.
For this example, the main.cpp file exceptionally use helpers function from imgui_impl_vulkan.h/cpp.

[example_glut_opengl2/](https://github.com/ocornut/imgui/blob/master/examples/example_glut_opengl2/) <BR>
GLUT (e.g., FreeGLUT on Linux/Windows, GLUT framework on OSX) + OpenGL2 example. <BR>
= main.cpp + imgui_impl_glut.cpp + imgui_impl_opengl2.cpp <BR>
Note that GLUT/FreeGLUT is largely obsolete software, prefer using GLFW or SDL.

[example_marmalade/](https://github.com/ocornut/imgui/blob/master/examples/example_marmalade/) <BR>
Marmalade example using IwGx. <BR>
= main.cpp + imgui_impl_marmalade.cpp

[example_null/](https://github.com/ocornut/imgui/blob/master/examples/example_null/) <BR>
Null example, compile and link imgui, create context, run headless with no inputs and no graphics output. <BR>
= main.cpp <BR>
This is used to quickly test compilation of core imgui files in as many setups as possible.
Because this application doesn't create a window nor a graphic context, there's no graphics output.

[example_sdl_directx11/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl_directx11/) <BR>
SDL2 + DirectX11 example, Windows only. <BR>
= main.cpp + imgui_impl_sdl.cpp + imgui_impl_dx11.cpp <BR>
This to demonstrate usage of DirectX with SDL.

[example_sdl_metal/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl_metal/) <BR>
SDL2 (Mac) + Metal example. <BR>
= main.mm + imgui_impl_sdl.cpp + imgui_impl_metal.mm

[example_sdl_opengl2/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl_opengl2/) <BR>
SDL2 (Win32, Mac, Linux etc.) + OpenGL example (legacy, fixed pipeline). <BR>
= main.cpp + imgui_impl_sdl.cpp + imgui_impl_opengl2.cpp <BR>
**DO NOT USE OPENGL2 CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)** <BR>
**Prefer using OPENGL3 code (with gl3w/glew/glad/glad2/glbinding, you can replace the OpenGL function loader)** <BR>
This code is mostly provided as a reference to learn about Dear ImGui integration, because it is shorter.
If your code is using GL3+ context or any semi modern OpenGL calls, using this renderer is likely to
make things more complicated, will require your code to reset many OpenGL attributes to their initial
state, and might confuse your GPU driver. One star, not recommended.

[example_sdl_opengl3/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl_opengl3/) <BR>
SDL2 (Win32, Mac, Linux, etc.) + OpenGL3+/ES2/ES3 example. <BR>
= main.cpp + imgui_impl_sdl.cpp + imgui_impl_opengl3.cpp <BR>
This uses more modern OpenGL calls and custom shaders. <BR>
Prefer using that if you are using modern OpenGL in your application (anything with shaders).
(Please be mindful that accessing OpenGL3+ functions requires a function loader, which are a frequent
source for confusion for new users. We use a loader in imgui_impl_opengl3.cpp which may be different
from the one your app normally use. Read imgui_impl_opengl3.h for details and how to change it.)

[example_sdl_vulkan/](https://github.com/ocornut/imgui/blob/master/examples/example_sdl_vulkan/) <BR>
SDL2 (Win32, Mac, Linux, etc.) + Vulkan example. <BR>
= main.cpp + imgui_impl_sdl.cpp + imgui_impl_vulkan.cpp <BR>
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


### Miscallaneous

**Building**

Unfortunately in 2020 it is still tedious to create and maintain portable build files using external
libraries (the kind we're using here to create a window and render 3D triangles) without relying on
third party software. For most examples here we choose to provide:
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

Dear ImGui has no particular extra lag for most behaviors,
e.g. the value of 'io.MousePos' provided at the time of NewFrame() will result in windows being moved
to the right spot at the time of EndFrame()/Render(). At 60 FPS your experience should be pleasant.

However, consider that OS mouse cursors are typically drawn through a very specific hardware accelerated
path and will feel smoother than the majority of contents rendered via regular graphics API (including,
but not limited to Dear ImGui windows). Because UI rendering and interaction happens on the same plane
as the mouse, that disconnect may be jarring to particularly sensitive users.
You may experiment with enabling the io.MouseDrawCursor flag to request Dear ImGui to draw a mouse cursor
using the regular graphics API, to help you visualize the difference between a "hardware" cursor and a
regularly rendered software cursor.
However, rendering a mouse cursor at 60 FPS will feel sluggish so you likely won't want to enable that at
all times. It might be beneficial for the user experience to switch to a software rendered cursor _only_
when an interactive drag is in progress.

Note that some setup or GPU drivers are likely to be causing extra display lag depending on their settings.
If you feel that dragging windows feels laggy and you are not sure what the cause is: try to build a simple
drawing a flat 2D shape directly under the mouse cursor!

