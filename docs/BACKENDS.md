_(You may browse this at https://github.com/ocornut/imgui/blob/master/docs/BACKENDS.md or view this file with any Markdown viewer)_

# Dear ImGui: Backends

## Index

- [Introduction](#introduction)
- [Using standard backends](#using-standard-backends)
- [List of third-party backends](#list-of-third-party-backends)
- [Writing your own backend](#writing-your-own-backend)
  - [Using a custom engine?](#using-a-custom-engine)
  - [Adding support for `ImGuiBackendFlags_RendererHasTextures` (1.92+)](#adding-support-for-imguibackendflags_rendererhastextures-192)

## Introduction

### Getting Started

💡 The **[Getting Started](https://github.com/ocornut/imgui/wiki/Getting-Started) wiki guide** has examples of how to integrate Dear ImGui into an existing application.
<BR> The [EXAMPLES.MD](https://github.com/ocornut/imgui/blob/master/docs/EXAMPLES.md) documentation may also be worth a read.

### What are backends?

Dear ImGui is highly portable and only requires a few things to run and render, typically:

 - Required: providing mouse/keyboard inputs (fed into the `ImGuiIO` structure).
 - Required: creating, updating and destroying textures.
 - Required: rendering indexed textured triangles with a clipping rectangle.

 Extra features are opt-in, our backends try to support as many as possible:

 - Optional: custom texture binding support.
 - Optional: clipboard support.
 - Optional: gamepad support.
 - Optional: mouse cursor shape support.
 - Optional: IME support.
 - Optional: multi-viewports support.
 etc.

This is essentially what each backend is doing + obligatory portability cruft. Using standard backends ensure you can get all those features including the ones that would be harder to implement on your side (e.g. multi-viewports support).

It is important to understand the difference between the core Dear ImGui library (files in the root folder)
and the backends which we are describing here (backends/ folder).

- Some issues may only be backend or platform specific.
- You should be able to write backends for pretty much any platform and any 3D graphics API.
  e.g. you can get creative and use software rendering or render remotely on a different machine.

## Using standard backends

**The [backends/](https://github.com/ocornut/imgui/blob/master/backends) folder contains backends for popular platforms/graphics API, which you can use in
your application or engine to easily integrate Dear ImGui.** Each backend is typically self-contained in a pair of files: imgui_impl_XXXX.cpp + imgui_impl_XXXX.h.

- The 'Platform' backends are in charge of: mouse/keyboard/gamepad inputs, cursor shape, timing, and windowing.<BR>
  e.g. Windows ([imgui_impl_win32.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_win32.cpp)), SDL3 ([imgui_impl_sdl3.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_sdl3.cpp)), GLFW ([imgui_impl_glfw.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_glfw.cpp)), etc.

- The 'Renderer' backends are in charge of: creating atlas texture, and rendering imgui draw data.<BR>
  e.g. DirectX11 ([imgui_impl_dx11.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_dx11.cpp)), OpenGL/WebGL ([imgui_impl_opengl3.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_opengl3.cpp)), Vulkan ([imgui_impl_vulkan.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_vulkan.cpp)), etc.

- For some high-level frameworks, a single backend usually handles both 'Platform' and 'Renderer' parts.<BR>
  e.g. Allegro 5 ([imgui_impl_allegro5.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_allegro5.cpp)). If you end up creating a custom backend for your engine, you may want to do the same.

An application usually combines one Platform backend + one Renderer backend + main Dear ImGui sources.
For example, the [example_win32_directx11](https://github.com/ocornut/imgui/tree/master/examples/example_win32_directx11) application combines imgui_impl_win32.cpp + imgui_impl_dx11.cpp. There are 20+ examples in the [examples/](https://github.com/ocornut/imgui/blob/master/examples/) folder. See [EXAMPLES.MD](https://github.com/ocornut/imgui/blob/master/docs/EXAMPLES.md) for details.

**Once Dear ImGui is setup and running, run and refer to `ImGui::ShowDemoWindow()` in imgui_demo.cpp for usage of the end-user API.**

### List of standard backends

In the [backends/](https://github.com/ocornut/imgui/blob/master/backends) folder:

List of Platforms Backends:

    imgui_impl_android.cpp      ; Android native app API
    imgui_impl_glfw.cpp         ; GLFW (Windows, macOS, Linux, etc.) http://www.glfw.org/
    imgui_impl_osx.mm           ; macOS native API (not as feature complete as glfw/sdl backends)
    imgui_impl_sdl2.cpp         ; SDL2 (Windows, macOS, Linux, iOS, Android) https://www.libsdl.org
    imgui_impl_sdl3.cpp         ; SDL3 (Windows, macOS, Linux, iOS, Android) https://www.libsdl.org
    imgui_impl_win32.cpp        ; Win32 native API (Windows)
    imgui_impl_glut.cpp         ; GLUT/FreeGLUT (this is prehistoric software and absolutely not recommended today!)

List of Renderer Backends:

    imgui_impl_dx9.cpp          ; DirectX9
    imgui_impl_dx10.cpp         ; DirectX10
    imgui_impl_dx11.cpp         ; DirectX11
    imgui_impl_dx12.cpp         ; DirectX12
    imgui_impl_metal.mm         ; Metal (ObjC or C++)
    imgui_impl_opengl2.cpp      ; OpenGL 2 (legacy fixed pipeline. Don't use with modern OpenGL code!)
    imgui_impl_opengl3.cpp      ; OpenGL 3/4, OpenGL ES 2/3, WebGL
    imgui_impl_sdlgpu3.cpp      ; SDL_GPU (portable 3D graphics API of SDL3)
    imgui_impl_sdlrenderer2.cpp ; SDL_Renderer (optional component of SDL2 available from SDL 2.0.18+)
    imgui_impl_sdlrenderer3.cpp ; SDL_Renderer (optional component of SDL3. Prefer using SDL_GPU!).
    imgui_impl_vulkan.cpp       ; Vulkan
    imgui_impl_wgpu.cpp         ; WebGPU (web + desktop)

List of high-level Frameworks Backends (combining Platform + Renderer):

    imgui_impl_allegro5.cpp

Emscripten is also supported!
The SDL2+GL, SDL3+GL, GLFW+GL and GLFW+WebGPU examples are all ready to build and run with Emscripten.

### Recommended Backends

If you are not sure which backend to use, the recommended platform/frameworks for portable applications:

|Library |Website |Backend |Note |
|--------|--------|--------|-----|
| SDL3 | https://www.libsdl.org | imgui_impl_sdl3.cpp | Recommended |
| SDL2 | https://www.libsdl.org | imgui_impl_sdl2.cpp | |
| GLFW | https://github.com/glfw/glfw | imgui_impl_glfw.cpp | |
| Sokol | https://github.com/floooh/sokol | [util/sokol_imgui.h](https://github.com/floooh/sokol/blob/master/util/sokol_imgui.h) | Lower-level than GLFW/SDL |

If your application runs on Windows or if you are using multi-viewport, the win32 backend handles some details a little better than other backends.

## List of third-party backends

See https://github.com/ocornut/imgui/wiki/Bindings for the full list (e.g. Adventure Game Studio, Cinder, Cocos2d-x, Game Maker Studio2, Godot, LÖVE+LUA, Magnum, Monogame, Ogre, openFrameworks, OpenSceneGraph, SFML, Sokol, Unity, Unreal Engine and many others).

## Writing your own backend

### Using a custom engine?

You will likely be tempted to start by rewrite your own backend using your own custom/high-level facilities...<BR>
Think twice!

**Consider using the existing backends as-is**.
You will save lots of time integrating the library. 
Standard backends are battle-tested and handle subtleties that you are likely to implement incorrectly.
You can LATER decide to rewrite yourself a custom backend if you really need to.
In most situations, custom backends have fewer features and more bugs than the standard backends we provide.
If you want portability, you can use multiple backends and choose between them either at compile time
or at runtime.

**Example A**: your engine is built over Windows + DirectX11 but you have your own high-level rendering
system layered over DirectX11.<BR>
Suggestion: try using imgui_impl_win32.cpp + imgui_impl_dx11.cpp first.
Once it works, if you really need it, you can replace the imgui_impl_dx11.cpp code with a
custom renderer using your own rendering functions, and keep using the standard Win32 code etc.

**Example B**: your engine runs on Windows, Mac, Linux and uses DirectX11, Metal, and Vulkan respectively.<BR>
Suggestion: use multiple generic backends!
Once it works, if you really need it, you can replace parts of backends with your own abstractions.

**Example C**: your engine runs on platforms we can't provide public backends for (e.g. PS4/PS5, Switch),
and you have high-level systems everywhere.<BR>
Suggestion: try using a non-portable backend first (e.g. win32 + underlying graphics API) to get
your desktop builds working first. This will get you running faster and get your acquainted with
how Dear ImGui works and is setup. You can then rewrite a custom backend using your own engine API...

TL;DR;
- **It is unlikely you will add value to your project by creating your own backend.**
- Writing your own Renderer Backend is easier.
- Writing your own Platform Backend is harder and you are more likely to introduce bugs.

Also:
The [multi-viewports feature](https://github.com/ocornut/imgui/wiki/Multi-Viewports) of the 'docking' branch allows
Dear ImGui windows to be seamlessly detached from the main application window. This is achieved using an
extra layer to the Platform and Renderer backends, which allows Dear ImGui to communicate platform-specific
requests such as: "create an additional OS window", "create a render context", "get the OS position of this
window" etc. See 'ImGuiPlatformIO' for details.
Supporting the multi-viewports feature correctly using 100% of your own abstractions is more difficult
than supporting single-viewport.
If you decide to use unmodified imgui_impl_XXXX.cpp files, you can automatically benefit from
improvements and fixes related to viewports and platform windows without extra work on your side.

### Adding support for `ImGuiBackendFlags_RendererHasTextures` (1.92+)

Version [1.92.0](https://github.com/ocornut/imgui/releases/tag/v1.92.0), released June 2025, added texture support in Rendering Backends, which is the backbone for supporting dynamic font scaling among other things.
<BR>**In order to move forward and take advantage of all new features, support for `ImGuiBackendFlags_RendererHasTextures` will likely be REQUIRED for all backends before June 2026.**
