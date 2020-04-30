-----------------------------------------------------------------------
 dear imgui, v1.77 WIP
-----------------------------------------------------------------------
 examples/README.txt
 (This is the README file for the examples/ folder. See docs/ for more documentation)
-----------------------------------------------------------------------

Dear ImGui is highly portable and only requires a few things to run and render:

 - Providing mouse/keyboard inputs
 - Uploading the font atlas texture into graphics memory
 - Providing a render function to render indexed textured triangles
 - Optional: clipboard support, mouse cursor supports, Windows IME support, etc.
 - Optional (Advanced,Beta): platform window API to use multi-viewport.

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
  http://www.dearimgui.org/binaries


---------------------------------------
 MISC COMMENTS AND SUGGESTIONS
---------------------------------------

 - Read FAQ at http://dearimgui.org/faq

 - Please read 'PROGRAMMER GUIDE' in imgui.cpp for notes on how to setup Dear ImGui in your codebase.
   Please read the comments and instruction at the top of each file.

 - If you are using of the backend provided here, so you can copy the imgui_impl_xxx.cpp/h files
   to your project and use them unmodified. Each imgui_impl_xxxx.cpp comes with its own individual
   ChangeLog at the top of the .cpp files, so if you want to update them later it will be easier to
   catch up with what changed.

 - Dear ImGui has 0 to 1 frame of lag for most behaviors, at 60 FPS your experience should be pleasant.
   However, consider that OS mouse cursors are typically drawn through a specific hardware accelerated path
   and will feel smoother than common GPU rendered contents (including Dear ImGui windows).
   You may experiment with the io.MouseDrawCursor flag to request Dear ImGui to draw a mouse cursor itself,
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
   Examples: Windows (imgui_impl_win32.cpp), GLFW (imgui_impl_glfw.cpp), SDL2 (imgui_impl_sdl.cpp), etc.

 - The "Renderer" bindings, in charge of: creating the main font texture, rendering imgui draw data.
   Examples: DirectX11 (imgui_impl_dx11.cpp), GL3 (imgui_impl_opengl3.cpp), Vulkan (imgui_impl_vulkan.cpp), etc.

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

 - Road-map: Dear ImGui 1.80 (WIP currently in the "docking" branch) will allows imgui windows to be
   seamlessly detached from the main application window. This is achieved using an extra layer to the
   platform and renderer bindings, which allows Dear ImGui to communicate platform-specific requests such as
   "create an additional OS window", "create a render context", "get the OS position of this window" etc.
   When using this feature, the coupling with your OS/renderer becomes much tighter than a regular imgui
   integration. It is also much more complicated and require more work to integrate correctly.
   If you are new to imgui and you are trying to integrate it into your application, first try to ignore
   everything related to Viewport and Platform Windows. You'll be able to come back to it later!
   Note that if you decide to use unmodified imgui_impl_xxxx.cpp files, you will automatically benefit
   from improvements and fixes related to viewports and platform windows without extra work on your side.
   See 'ImGuiPlatformIO' for details.


List of Platforms Bindings in this repository:

    imgui_impl_glfw.cpp       ; GLFW (Windows, macOS, Linux, etc.) http://www.glfw.org/
    imgui_impl_osx.mm         ; macOS native API (not as feature complete as glfw/sdl back-ends)
    imgui_impl_sdl.cpp        ; SDL2 (Windows, macOS, Linux, iOS, Android) https://www.libsdl.org
    imgui_impl_win32.cpp      ; Win32 native API (Windows)
    imgui_impl_glut.cpp       ; GLUT/FreeGLUT (absolutely not recommended in 2020!)

List of Renderer Bindings in this repository:

    imgui_impl_dx9.cpp        ; DirectX9
    imgui_impl_dx10.cpp       ; DirectX10
    imgui_impl_dx11.cpp       ; DirectX11
    imgui_impl_dx12.cpp       ; DirectX12
    imgui_impl_metal.mm       ; Metal (with ObjC)
    imgui_impl_opengl2.cpp    ; OpenGL 2 (legacy, fixed pipeline <- don't use with modern OpenGL context)
    imgui_impl_opengl3.cpp    ; OpenGL 3/4, OpenGL ES 2, OpenGL ES 3 (modern programmable pipeline)
    imgui_impl_vulkan.cpp     ; Vulkan

List of high-level Frameworks Bindings in this repository: (combine Platform + Renderer)

    imgui_impl_allegro5.cpp
    imgui_impl_marmalade.cpp

Note that Dear ImGui works with Emscripten. The examples_emscripten/ app uses imgui_impl_sdl.cpp and
imgui_impl_opengl3.cpp, but other combinations are possible.

Third-party framework, graphics API and languages bindings are listed at:

    https://github.com/ocornut/imgui/wiki/Bindings

Including backends for:

    AGS/Adventure Game Studio, Amethyst, bsf, Cinder, Cocos2d-x, Diligent Engine, Flexium,
    GML/Game Maker Studio2, GTK3+OpenGL3, Irrlicht Engine, LÖVE+LUA, Magnum, NanoRT, Nim Game Lib,
    Ogre, openFrameworks, OSG/OpenSceneGraph, Orx, px_render, Qt/QtDirect3D, SFML, Sokol,
    Unreal Engine 4, vtk, Win32 GDI, etc.

Not sure which to use?
Recommended platform/frameworks:

    GLFW    https://github.com/glfw/glfw        Use imgui_impl_glfw.cpp
    SDL2    https://www.libsdl.org              Use imgui_impl_sdl.cpp
    Sokol   https://github.com/floooh/sokol     Use util/sokol_imgui.h in Sokol repository.

Those will allow you to create portable applications and will solve and abstract away many issues.


---------------------------------------
 EXAMPLE APPLICATIONS
---------------------------------------

Building:
  Unfortunately in 2020 it is still tedious to create and maintain portable build files using external
  libraries (the kind we're using here to create a window and render 3D triangles) without relying on
  third party software. For most examples here I choose to provide:
   - Makefiles for Linux/OSX
   - Batch files for Visual Studio 2008+
   - A .sln project file for Visual Studio 2010+
   - Xcode project files for the Apple examples
  Please let me know if they don't work with your setup!
  You can probably just import the imgui_impl_xxx.cpp/.h files into your own codebase or compile those
  directly with a command-line compiler.

  If you are interested in using Cmake to build and links examples, see:
    https://github.com/ocornut/imgui/pull/1713 and https://github.com/ocornut/imgui/pull/3027


example_allegro5/
    Allegro 5 example.
    = main.cpp + imgui_impl_allegro5.cpp

example_apple_metal/
    OSX & iOS + Metal.
    = main.m + imgui_impl_osx.mm + imgui_impl_metal.mm
    It is based on the "cross-platform" game template provided with Xcode as of Xcode 9.
    (NB: imgui_impl_osx.mm is currently not as feature complete as other platforms back-ends.
    You may prefer to use the GLFW Or SDL back-ends, which will also support Windows and Linux.)

example_apple_opengl2/
    OSX + OpenGL2.
    = main.mm + imgui_impl_osx.mm + imgui_impl_opengl2.cpp
    (NB: imgui_impl_osx.mm is currently not as feature complete as other platforms back-ends.
    You may prefer to use the GLFW Or SDL back-ends, which will also support Windows and Linux.)

example_empscripten:
    Emcripten + SDL2 + OpenGL3+/ES2/ES3 example.
    = main.cpp + imgui_impl_sdl.cpp + imgui_impl_opengl3.cpp
    Note that other examples based on SDL or GLFW + OpenGL could easily be modified to work with Emscripten.
    We provide this to make the Emscripten differences obvious, and have them not pollute all other examples.

example_glfw_metal/
    GLFW (Mac) + Metal example.
    = main.mm + imgui_impl_glfw.cpp + imgui_impl_metal.mm

example_glfw_opengl2/
    GLFW + OpenGL2 example (legacy, fixed pipeline).
    = main.cpp + imgui_impl_glfw.cpp + imgui_impl_opengl2.cpp
    **DO NOT USE OPENGL2 CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
    **Prefer using OPENGL3 code (with gl3w/glew/glad/glbinding, you can replace the OpenGL function loader)**
    This code is mostly provided as a reference to learn about Dear ImGui integration, because it is shorter.
    If your code is using GL3+ context or any semi modern OpenGL calls, using this renderer is likely to
    make things more complicated, will require your code to reset many OpenGL attributes to their initial
    state, and might confuse your GPU driver. One star, not recommended.

example_glfw_opengl3/
    GLFW (Win32, Mac, Linux) + OpenGL3+/ES2/ES3 example (programmable pipeline).
    = main.cpp + imgui_impl_glfw.cpp + imgui_impl_opengl3.cpp
    This uses more modern OpenGL calls and custom shaders.
    Prefer using that if you are using modern OpenGL in your application (anything with shaders).
    (Please be mindful that accessing OpenGL3+ functions requires a function loader, which are a frequent
    source for confusion for new users. We use a loader in imgui_impl_opengl3.cpp which may be different
    from the one your app normally use. Read imgui_impl_opengl3.h for details and how to change it.)

example_glfw_vulkan/
    GLFW (Win32, Mac, Linux) + Vulkan example.
    = main.cpp + imgui_impl_glfw.cpp + imgui_impl_vulkan.cpp
    This is quite long and tedious, because: Vulkan.
    For this example, the main.cpp file exceptionally use helpers function from imgui_impl_vulkan.h/cpp.

example_glut_opengl2/
    GLUT (e.g., FreeGLUT on Linux/Windows, GLUT framework on OSX) + OpenGL2.
    = main.cpp + imgui_impl_glut.cpp + imgui_impl_opengl2.cpp
    Note that GLUT/FreeGLUT is largely obsolete software, prefer using GLFW or SDL.

example_marmalade/
    Marmalade example using IwGx.
    = main.cpp + imgui_impl_marmalade.cpp

example_null
    Null example, compile and link imgui, create context, run headless with no inputs and no graphics output.
    = main.cpp
    This is used to quickly test compilation of core imgui files in as many setups as possible.
    Because this application doesn't create a window nor a graphic context, there's no graphics output.

example_sdl_directx11/
    SDL2 + DirectX11 example, Windows only.
    = main.cpp + imgui_impl_sdl.cpp + imgui_impl_dx11.cpp
    This to demonstrate usage of DirectX with SDL.

example_sdl_metal/
    SDL2 (Mac) + Metal example.
    = main.mm + imgui_impl_sdl.cpp + imgui_impl_metal.mm

example_sdl_opengl2/
    SDL2 (Win32, Mac, Linux etc.) + OpenGL example (legacy, fixed pipeline).
    = main.cpp + imgui_impl_sdl.cpp + imgui_impl_opengl2.cpp
    **DO NOT USE OPENGL2 CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
    **Prefer using OPENGL3 code (with gl3w/glew/glad/glbinding, you can replace the OpenGL function loader)**
    This code is mostly provided as a reference to learn about Dear ImGui integration, because it is shorter.
    If your code is using GL3+ context or any semi modern OpenGL calls, using this renderer is likely to
    make things more complicated, will require your code to reset many OpenGL attributes to their initial
    state, and might confuse your GPU driver. One star, not recommended.

example_sdl_opengl3/
    SDL2 (Win32, Mac, Linux, etc.) + OpenGL3+/ES2/ES3 example.
    = main.cpp + imgui_impl_sdl.cpp + imgui_impl_opengl3.cpp
    This uses more modern OpenGL calls and custom shaders.
    Prefer using that if you are using modern OpenGL in your application (anything with shaders).
    (Please be mindful that accessing OpenGL3+ functions requires a function loader, which are a frequent
    source for confusion for new users. We use a loader in imgui_impl_opengl3.cpp which may be different
    from the one your app normally use. Read imgui_impl_opengl3.h for details and how to change it.)

example_sdl_vulkan/
    SDL2 (Win32, Mac, Linux, etc.) + Vulkan example.
    = main.cpp + imgui_impl_sdl.cpp + imgui_impl_vulkan.cpp
    This is quite long and tedious, because: Vulkan.
    For this example, the main.cpp file exceptionally use helpers function from imgui_impl_vulkan.h/cpp.

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
    = main.cpp + imgui_impl_win32.cpp + imgui_impl_dx12.cpp
    This is quite long and tedious, because: DirectX12.
