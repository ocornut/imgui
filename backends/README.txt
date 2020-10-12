-----------------------------------------------------------------------
 dear imgui, v1.80 WIP
 Backends
-----------------------------------------------------------------------
 (See docs/ and examples/ for more documentation)
-----------------------------------------------------------------------

This folder contains backends for popular platforms/graphics API, which you can use in
your application or engine to easily integrate Dear ImGui.

- The 'Platform' backends are in charge of: mouse/keyboard/gamepad inputs, cursor shape, timing, windowing.
  e.g. Windows (imgui_impl_win32.cpp), GLFW (imgui_impl_glfw.cpp), SDL2 (imgui_impl_sdl.cpp), etc.

- The 'Renderer' backends are in charge of: creating atlas texture, rendering imgui draw data.
  e.g. DirectX11 (imgui_impl_dx11.cpp), OpenGL/WebGL (imgui_impl_opengl3.cpp), Vulkan (imgui_impl_vulkan.cpp), etc.

- For some high-level frameworks, a single backend usually handle both 'Platform' and 'Renderer' parts.
  e.g. Allegro 5 (imgui_impl_allegro5.cpp), Marmalade (imgui_impl_marmalade5.cpp).

An application usually combines 1 Platform backend + 1 Renderer backend + main ImGui sources.
For example, the example_win32_directx11/ application combines imgui_impl_win32.cpp + imgui_impl_dx11.cpp.
See examples/README.txt for details.


---------------------------------------
 WHAT ARE BACKENDS?
---------------------------------------

Dear ImGui is highly portable and only requires a few things to run and render, typically:

 - Required: providing mouse/keyboard inputs.
 - Required: uploading the font atlas texture into graphics memory.
 - Required: rendering indexed textured triangles with a clipping rectangle.

 Extra features are opt-in, our backends try to support as many as possible:

 - Optional: custom texture binding support.
 - Optional: clipboard support.
 - Optional: gamepad support.
 - Optional: mouse cursor shape support.
 - Optional: IME support.
 - Optional: multi-viewports support.
 etc.

This is essentially what the backends in this folder are providing + obligatory portability cruft.

It is important to understand the difference between the core Dear ImGui library (files in the root folder)
and backends which we are describing here (backends/ folder).

- Some issues may only be backend or platform specific.
- You should be able to write backends for pretty much any platform and any 3D graphics API. 
  e.g. get creative and have your backend perform rendering remotely, on a different machine
  than the one running Dear ImGui, etc.


-----------------------------------------------------------------------
 USING A CUSTOM ENGINE?
-----------------------------------------------------------------------

You will likely be tempted to start by rewrite your own backend using your own custom/high-level facilities... 
Think twice!

If you are new to Dear ImGui, first try using the existing backends as-is.
You will save lots of time integrating the library.
You can LATER decide to rewrite yourself a custom backend if you really need to.
In most situations, custom backends have less features and more bugs than the standard backends we provide.
If you want portability, you can use multiple backends and choose between them either at compile time 
or at runtime. 

Example A: your engine is built over Windows + DirectX11 but you have your own high-level rendering
system layered over DirectX11.
 - Suggestion: try using imgui_impl_win32.cpp + imgui_impl_dx11.cpp first.
   Once it works, if you really need it you can replace the imgui_impl_dx11.cpp code with a 
   custom renderer using your own rendering functions, and keep using the standard Win32 code etc.

Example B: your engine runs on Windows, Mac, Linux and uses DirectX11, Metal, Vulkan respectively.
 - Suggestion: use multiple generic backends!
   Once it works, if you really need it you can replace parts of backends with your own abstractions.

Example C: your engine runs on platforms we can't provide public backends for (e.g. PS4/PS5, Switch), 
and you have high-level systems everywhere.
 - Suggestion: try using a non-portable backend first (e.g. win32 + underlying graphics API) to get
   your desktop builds working first. This will get you running faster and get your acquainted with
   how Dear ImGui works and is setup. You can then rewrite a custom backend using your own engine API.

Also:
The multi-viewports feature of the 'docking' branch allows Dear ImGui windows to be seamlessly detached 
from the main application window. This is achieved using an extra layer to the Platform and Renderer 
backends, which allows Dear ImGui to communicate platform-specific requests.
Supporting the multi-viewports feature correctly using 100% of your own abstractions is more difficult 
than supporting single-viewport.
If you decide to use unmodified imgui_impl_xxxx.cpp files, you can automatically benefit from
improvements and fixes related to viewports and platform windows without extra work on your side.


---------------------------------------
 LIST OF BACKENDS
---------------------------------------

List of Platforms Backends in this repository:

    imgui_impl_glfw.cpp       ; GLFW (Windows, macOS, Linux, etc.) http://www.glfw.org/
    imgui_impl_osx.mm         ; macOS native API (not as feature complete as glfw/sdl backends)
    imgui_impl_sdl.cpp        ; SDL2 (Windows, macOS, Linux, iOS, Android) https://www.libsdl.org
    imgui_impl_win32.cpp      ; Win32 native API (Windows)
    imgui_impl_glut.cpp       ; GLUT/FreeGLUT (absolutely not recommended in 2020!)

List of Renderer Backends in this repository:

    imgui_impl_dx9.cpp        ; DirectX9
    imgui_impl_dx10.cpp       ; DirectX10
    imgui_impl_dx11.cpp       ; DirectX11
    imgui_impl_dx12.cpp       ; DirectX12
    imgui_impl_metal.mm       ; Metal (with ObjC)
    imgui_impl_opengl2.cpp    ; OpenGL 2 (legacy, fixed pipeline <- don't use with modern OpenGL context)
    imgui_impl_opengl3.cpp    ; OpenGL 3/4, OpenGL ES 2, OpenGL ES 3 (modern programmable pipeline)
    imgui_impl_vulkan.cpp     ; Vulkan

Emscripten is also supported.
The example_emscripten/ app uses imgui_impl_sdl.cpp + imgui_impl_opengl3.cpp, but other combos are possible.

List of high-level Frameworks Backends in this repository: (combine Platform + Renderer)

    imgui_impl_allegro5.cpp
    imgui_impl_marmalade.cpp

Backends for third-party frameworks, graphics API or other programming languages:

    https://github.com/ocornut/imgui/wiki/Bindings

    (AGS/Adventure Game Studio, Amethyst, bsf, Cinder, Cocos2d-x, Diligent Engine, Flexium,
    GML/Game Maker Studio2, GTK3+OpenGL3, Irrlicht Engine, LÖVE+LUA, Magnum, NanoRT, Nim Game Lib,
    Ogre, openFrameworks, OSG/OpenSceneGraph, Orx, px_render, Qt/QtDirect3D, SFML, Sokol,
    Unreal Engine 4, vtk, Win32 GDI, etc.)


---------------------------------------
 RECOMMENDED BACKENDS
---------------------------------------

Recommended platform/frameworks for portable applications:

    Library:  GLFW    
    Webpage:  https://github.com/glfw/glfw        
    Backend:  imgui_impl_glfw.cpp

    Library:  SDL2    
    Webpage:  https://www.libsdl.org              
    Backend:  imgui_impl_sdl.cpp

    Library:  Sokol (lower-level than GLFW/SDL)
    Webpage:  https://github.com/floooh/sokol     
    Backend:  Use util/sokol_imgui.h in Sokol repository.

