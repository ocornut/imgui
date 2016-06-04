Those are standalone ready-to-build applications to demonstrate ImGui.
Binaries of some of those demos: http://www.miracleworld.net/imgui/binaries

Third party languages and frameworks bindings: https://github.com/ocornut/imgui/wiki/Links
(languages: C, .net, rust, D, Python, Lua..)
(frameworks: DX12, Vulkan, Cinder, OpenGLES, openFrameworks, Cocos2d-x, SFML, Flexium, NanoRT, Irrlicht..)
(extras: RemoteImGui, ImWindow, imgui_wm..)

TL;DR; 
 - Newcomers, read 'PROGRAMMER GUIDE' in imgui.cpp for notes on how to setup ImGui in your codebase.
 - Refer to 'opengl_example' to understand how the library is setup, it is the simplest one.
   The other examples requires more boilerplate and are harder to read.
 - If you are using of the backend provided here, so you can copy the imgui_impl_xxx.cpp/h files
   to your project and use them unmodified.
 - If you have your own engine, you probably want to start from 'opengl_example' and adapt it to 
   your engine, but you can read the other examples as well.

ImGui is highly portable and only requires a few things to run:
 - Providing mouse/keyboard inputs
 - Load the font atlas texture into graphics memory
 - Providing a render function to render indexed textured triangles
 - Optional: clipboard support, mouse cursor supports, Windows IME support, etc.
So this is essentially what those examples are doing + the obligatory cruft for portability.

Unfortunately in 2016 it is still tedious to create and maintain portable build files using external 
libraries (the kind we're using here to create a window and render 3D triangles) without relying on 
third party software. For most examples here I choose to provide:
 - Makefiles for Linux/OSX
 - Batch files for Visual Studio 2008+
 - A .sln project file for Visual Studio 2010+ 
Please let me know if they don't work with your setup!
You can probably just import the imgui_impl_xxx.cpp/.h files into your own codebase or compile those
directly with a command-line compiler.

ImGui has zero frame of lag for most behaviors and one frame of lag for some behaviors. 
At 60 FPS your experience should be pleasant. Consider that OS mouse cursors are typically drawn through 
a specific hardware accelerated route and may feel smoother than other GPU rendered contents. You may 
experiment with the io.MouseDrawCursor flag to request ImGui to draw a mouse cursor itself, to visualize 
the lag between an hardware cursor and a software cursor. It might be beneficial to the user experience
to switch to a software rendered cursor when an interactive drag is in progress. 
Also note that some setup or GPU drivers may be causing extra lag (possibly by enforcing triple buffering), 
leaving you with no option but sadness/anger (Intel GPU drivers were reported as such).

opengl_example/
    OpenGL example, using GLFW + fixed pipeline.
    This is simple and should work for all OpenGL enabled applications.
    Prefer following this example to learn how ImGui works!
	(You can use this code in a GL3/GL4 context but make sure you disable the programmable pipeline
	by calling "glUseProgram(0)" before ImGui::Render.)

opengl3_example/
    OpenGL example, using GLFW/GL3W + programmable pipeline.
    This uses more modern OpenGL calls and custom shaders. It's more messy.

directx9_example/
    DirectX9 example, Windows only.
	
directx10_example/
    DirectX10 example, Windows only.
    This is quite long and tedious, because: DirectX10.

directx11_example/
    DirectX11 example, Windows only.
    This is quite long and tedious, because: DirectX11.
	
apple_example/
    OSX & iOS example.
    On iOS, Using Synergy to access keyboard/mouse data from server computer.
    Synergy keyboard integration is rather hacky.

sdl_opengl_example/
    SDL2 + OpenGL example.

sdl_opengl_example/
    SDL2 + OpenGL3 example.

allegro5_example/
    Allegro 5 example.
	 
marmalade_example/
    Marmalade example using IwGx
    
vulkan_example/
	Vulkan example.
	This is quite long and tedious, because: Vulkan.

