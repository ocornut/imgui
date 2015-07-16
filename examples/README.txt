Those are standalone ready-to-build applications to demonstrate ImGui.
Binaries of those demos are available from the main GitHub page.

TL;DR; 
 Refer to 'opengl_example' (not 'opengl3_example') to understand how the library is setup.
 Copy the imgui_impl_xxx.cpp/.h file you need if you are using one of the rendering backend used in an example.
 If you are using a different or your own backend, copy opengl_example/imgui_impl_opengl.cpp/.h to get started.
 

ImGui is highly portable and only requires a few things to run:
 - Providing mouse/keyboard inputs
 - Load the font atlas texture into GPU memory
 - Providing a render function to process the drawing commands (we rendere indexed textured triangles)
 - Extra just as clipboard support, mouse cursor supports, Windows IME support.
So this is essentially what those examples are doing + the obligatory cruft for portability.

Unfortunately in 2015 it is still a massive pain to create and maintain portable build files using
external library like the ones we're using here. 
For most example here I choose to provide Visual Studio 10 .sln files and Makefile for Linux/OSX. 
Please let me know if they don't work with your setup!
You can probably just import the imgui_impl_xxx.cpp/.h files into your own codebase or compile those
directly with a command-line compiler.

opengl_example/
    OpenGL example, using GLFW + fixed pipeline.
    This is simple and should work for all OpenGL enabled applications.
    Prefer following this example to learn how ImGui works!

opengl3_example/
    OpenGL example, using GLFW/GL3W + programmable pipeline.
    This uses more modern OpenGL calls and custom shaders.
    Even if your application is using modern OpenGL you are better off copying the code from the fixed pipeline version!
    I don't think there is an advantage using this over the simpler example, but it is provided for reference.

directx9_example/
    DirectX9 example, Windows only.
	
directx11_example/
    DirectX11 example, Windows only.
    This is quite long and tedious, because: DirectX11.
	
ios_example/
    iOS example.
    Using Synergy to access keyboard/mouse data from server computer. Synergy keyboard integration is rather hacky.

sdl_opengl_example/
    SDL2 + OpenGL example.

allegro5_example/
    Allegro 5 example.
