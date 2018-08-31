-----------------------------------------------------------------------
 examples/README.txt
 (This is the README file for the examples/ folder. See docs/ for more documentation)
-----------------------------------------------------------------------

Dear ImGui is highly portable and only requires a few things to run and render:

 - Providing mouse/keyboard inputs
 - Uploading the font atlas texture into graphics memory
 - Providing a render function to render indexed textured triangles
 - Optional: clipboard support, mouse cursor supports, Windows IME support, etc.

This is essentially what the example bindings in this folder are providing + obligatory portability cruft.

It is important to understand the difference between the core Dear ImGui library (files in the root folder)
and examples bindings which we are describing here (examples/ folder).
You should be able to write bindings for pretty much any platform and any 3D graphics API. With some extra
effort you can even perform the rendering remotely, on a different machine than the one running the logic.

This folder contains two things:

 - Example bindings for popular platforms/graphics API, which you can use as is or adapt for your own use.
   They are the imgui_impl_XXXX files found in the examples/ folder.

 - Example applications (standalone, ready-to-build) using the aforementioned bindings.
   They are the in the XXXX_example/ sub-folders.

You can find binaries of some of those example applications at: 
  http://www.miracleworld.net/imgui/binaries


---------------------------------------
 MISC COMMENTS AND SUGGESTIONS
---------------------------------------

 - Please read 'PROGRAMMER GUIDE' in imgui.cpp for notes on how to setup Dear ImGui in your codebase.
   Please read the comments and instruction at the top of each file.
   
 - If you are using of the backend provided here, so you can copy the imgui_impl_xxx.cpp/h files
   to your project and use them unmodified. Each imgui_impl_xxxx.cpp comes with its own individual
   ChangeLog at the top of the .cpp files, so if you want to update them later it will be easier to
   catch up with what changed.

 - Dear ImGui has 0 to 1 frame of lag for most behaviors, at 60 FPS your experience should be pleasant.
   However, consider that OS mouse cursors are typically drawn through a specific hardware accelerated path
   and will feel smoother than common GPU rendered contents (including Dear ImGui windows). 
   You may experiment with the io.MouseDrawCursor flag to request ImGui to draw a mouse cursor itself, 
   to visualize the lag between a hardware cursor and a software cursor. However, rendering a mouse cursor
   at 60 FPS will feel slow. It might be beneficial to the user experience to switch to a software rendered
   cursor only when an interactive drag is in progress. 
   Note that some setup or GPU drivers are likely to be causing extra lag depending on their settings.
   If you feel that dragging windows feels laggy and you are not sure who to blame: try to build an 
   application drawing a shape directly under the mouse cursor.


---------------------------------------
 EXAMPLE BINDINGS
---------------------------------------

Most the example bindings are split in 2 parts:

 - The "Platform" bindings, in charge of: mouse/keyboard/gamepad inputs, cursor shape, timing, windowing.
   Examples: Windows (imgui_impl_win32.cpp), GLFW (imgui_impl_glfw.cpp), SDL2 (imgui_impl_sdl.cpp)

 - The "Renderer" bindings, in charge of: creating the main font texture, rendering imgui draw data.
   Examples: DirectX11 (imgui_impl_dx11.cpp), GL3 (imgui_impl_opengl3.cpp), Vulkan (imgui_impl_vulkan.cpp)

 - The example _applications_ usually combine 1 platform + 1 renderer binding to create a working program.
   Examples: the example_win32_directx11/ application combines imgui_impl_win32.cpp + imgui_impl_dx11.cpp.

 - Some bindings for higher level frameworks carry both "Platform" and "Renderer" parts in one file.
   This is the case for Allegro 5 (imgui_impl_allegro5.cpp), Marmalade (imgui_impl_marmalade5.cpp).

 - If you use your own engine, you may decide to use some of existing bindings and/or rewrite some using 
   your own API. As a recommendation, if you are new to Dear ImGui, try using the existing binding as-is
   first, before moving on to rewrite some of the code. Although it is tempting to rewrite both of the 
   imgui_impl_xxxx files to fit under your coding style, consider that it is not necessary!
   In fact, if you are new to Dear ImGui, rewriting them will almost always be harder.

   Example: your engine is built over Windows + DirectX11 but you have your own high-level rendering 
   system layered over DirectX11.
     Suggestion: step 1: try using imgui_impl_win32.cpp + imgui_impl_dx11.cpp first. 
     Once this work, _if_ you want you can replace the imgui_impl_dx11.cpp code with a custom renderer 
     using your own functions, etc. 
     Please consider using the bindings to the lower-level platform/graphics API as-is.

   Example: your engine is multi-platform (consoles, phones, etc.), you have high-level systems everywhere.
     Suggestion: step 1: try using a non-portable binding first (e.g. win32 + underlying graphics API)!
     This is counter-intuitive, but this will get you running faster! Once you better understand how imgui
     works and is bound, you can rewrite the code using your own systems.

 - Road-map: Dear ImGui 1.70 (WIP currently in the "viewport" branch) will allows imgui windows to be 
   seamlessly detached from the main application window. This is achieved using an extra layer to the 
   platform and renderer bindings, which allows imgui to communicate platform-specific requests.
   If you decide to use unmodified imgui_impl_xxxx.cpp files, you will automatically benefit from 
   improvements and fixes related to viewports and platform windows without extra work on your side.


List of Platforms Bindings in this repository:

    imgui_impl_glfw.cpp       ; GLFW (Windows, macOS, Linux, etc.) http://www.glfw.org/
    imgui_impl_osx.mm         ; macOS native API
    imgui_impl_sdl.cpp        ; SDL2 (Windows, macOS, Linux, iOS, Android) https://www.libsdl.org
    imgui_impl_win32.cpp      ; Win32 native API (Windows)
    imgui_impl_freeglut.cpp   ; FreeGLUT (if you really miss the 90's)

List of Renderer Bindings in this repository:

    imgui_impl_dx9.cpp        ; DirectX9
    imgui_impl_dx10.cpp       ; DirectX10
    imgui_impl_dx11.cpp       ; DirectX11
    imgui_impl_dx12.cpp       ; DirectX12
    imgui_impl_metal.mm       ; Metal (with ObjC)
    imgui_impl_opengl2.cpp    ; OpenGL2 (legacy, fixed pipeline <- don't use with modern OpenGL context)
    imgui_impl_opengl3.cpp    ; OpenGL3, OpenGL ES 2, OpenGL ES 3 (modern programmable pipeline)
    imgui_impl_vulkan.cpp     ; Vulkan

List of high-level Frameworks Bindings in this repository: (combine Platform + Renderer)

    imgui_impl_allegro5.cpp
    imgui_impl_marmalade.cpp

Third-party framework, graphics API and languages bindings are listed at:

    https://github.com/ocornut/imgui/wiki/Bindings

    Languages: C, C#, ChaiScript, D, Go, Haxe, Java, Lua, Odin, Pascal, PureBasic, Python, Rust, Swift...
    Frameworks: Cinder, Cocoa (OSX), Cocos2d-x, Emscripten, SFML, GML/GameMaker Studio, Irrlicht, Ogre, 
    OpenSceneGraph, openFrameworks, LOVE, NanoRT, Nim Game Lib, Qt3d, SFML, Unreal Engine 4...
    Miscellaneous: Software Renderer, RemoteImgui, etc.


---------------------------------------
 EXAMPLE APPLICATIONS
---------------------------------------

Building:
  Unfortunately in 2018 it is still tedious to create and maintain portable build files using external 
  libraries (the kind we're using here to create a window and render 3D triangles) without relying on 
  third party software. For most examples here I choose to provide:
   - Makefiles for Linux/OSX
   - Batch files for Visual Studio 2008+
   - A .sln project file for Visual Studio 2010+ 
   - Xcode project files for the Apple examples
  Please let me know if they don't work with your setup!
  You can probably just import the imgui_impl_xxx.cpp/.h files into your own codebase or compile those
  directly with a command-line compiler.


example_win32_directx9/
    DirectX9 example, Windows only.
    = main.cpp + imgui_impl_win32.cpp + imgui_impl_dx9.cpp
    
example_win32_directx10/
    DirectX10 example, Windows only.
    = main.cpp + imgui_impl_win32.cpp + imgui_impl_dx10.cpp

example_win32_directx11/
    DirectX11 example, Windows only.
    = main.cpp + imgui_impl_win32.cpp + imgui_impl_dx11.cpp
    
example_win32_directx12/
    DirectX12 example, Windows only.
    This is quite long and tedious, because: DirectX12.
    = main.cpp + imgui_impl_win32.cpp + imgui_impl_dx12.cpp

example_apple_metal/
    OSX & iOS + Metal.
    It is based on the "cross-platform" game template provided with Xcode as of Xcode 9.
    (NB: you may still want to use GLFW or SDL which will also support Windows, Linux along with OSX.)
    = game template + imgui_impl_osx.mm + imgui_impl_metal.mm

example_apple_opengl2/
    OSX + OpenGL2.
    (NB: you may still want to use GLFW or SDL which will also support Windows, Linux along with OSX.)
    = main.mm + imgui_impl_osx.mm + imgui_impl_opengl2.cpp

example_glfw_opengl2/
    **DO NOT USE OPENGL2 CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
    **Prefer using OPENGL3 code (with gl3w/glew/glad, you can replace the OpenGL function loader)**
    GLFW + OpenGL2 example (legacy, fixed pipeline).
    This code is mostly provided as a reference to learn about ImGui integration, because it is shorter.
    If your code is using GL3+ context or any semi modern OpenGL calls, using this renderer is likely to
    make things more complicated, will require your code to reset many OpenGL attributes to their initial
    state, and might confuse your GPU driver. One star, not recommended.
    = main.cpp + imgui_impl_glfw.cpp + imgui_impl_opengl2.cpp

example_glfw_opengl3/
    GLFW (Win32, Mac, Linux) + OpenGL3+/ES2/ES3 example (programmable pipeline, binding modern functions with GL3W).
    This uses more modern OpenGL calls and custom shaders. 
    Prefer using that if you are using modern OpenGL in your application (anything with shaders).
    = main.cpp + imgui_impl_glfw.cpp + imgui_impl_opengl3.cpp
	
example_glfw_vulkan/
    GLFW (Win32, Mac, Linux) + Vulkan example.
    This is quite long and tedious, because: Vulkan.
    = main.cpp + imgui_impl_glfw.cpp + imgui_impl_vulkan.cpp

example_sdl_opengl2/
    **DO NOT USE OPENGL2 CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
    **Prefer using OPENGL3 code (with gl3w/glew/glad, you can replace the OpenGL function loader)**
    SDL2 (Win32, Mac, Linux etc.) + OpenGL example (legacy, fixed pipeline).
    This code is mostly provided as a reference to learn about ImGui integration, because it is shorter.
    If your code is using GL3+ context or any semi modern OpenGL calls, using this renderer is likely to
    make things more complicated, will require your code to reset many OpenGL attributes to their initial
    state, and might confuse your GPU driver. One star, not recommended. 
    = main.cpp + imgui_impl_sdl.cpp + imgui_impl_opengl2.cpp

example_sdl_opengl3/
    SDL2 (Win32, Mac, Linux, etc.) + OpenGL3+/ES2/ES3 example.
    This uses more modern OpenGL calls and custom shaders. 
    Prefer using that if you are using modern OpenGL in your application (anything with shaders).
    = main.cpp + imgui_impl_sdl.cpp + imgui_impl_opengl3.cpp

example_sdl_vulkan/
    SDL2 (Win32, Mac, Linux, etc.) + Vulkan example.
    This is quite long and tedious, because: Vulkan.
    = main.cpp + imgui_impl_sdl.cpp + imgui_impl_vulkan.cpp

example_allegro5/
    Allegro 5 example.
    = main.cpp + imgui_impl_allegro5.cpp

example_freeglut_opengl2/
    FreeGLUT + OpenGL2.
    = main.cpp + imgui_impl_freeglut.cpp + imgui_impl_opengl2.cpp

example_marmalade/
    Marmalade example using IwGx.
    = main.cpp + imgui_impl_marmalade.cpp
