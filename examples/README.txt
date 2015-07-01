Those are standalone ready-to-build applications to demonstrate ImGui.
I choose to provide Visual Studio 10 .sln files and Makefile for Linux/OSX.
Also, CMake project files are provided, too. 
Please let me know if they don't work with your setup!
You can probably just import the .cpp files into your own system and figure out the linkage from there.


opengl_example/
    OpenGL example, using GLFW + fixed pipeline.
    This is simple and should work for all OpenGL enabled applications.
    Prefer following this example since it is the shortest one!

opengl3_example/
    OpenGL exemple, using GLFW/GL3W + programmable pipeline.
    This uses more modern calls and custom shaders.
    I don't think there is an advantage using this over the simpler example, but it is provided for reference.

directx9_example/
    DirectX9 example, Windows only.
	
directx11_example/
    DirectX11 example, Windows only.
    This is quite long and tedious, because: DirectX11.
	
imconfig_example/
    This example demonstrates how to use imconfig.h. The code itself
    is the same as the opengl_example but a custom imconfig.h file
    is included.