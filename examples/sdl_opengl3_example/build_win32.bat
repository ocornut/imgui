set SDL2DIR=D:\T-Work\Libs\SDL\SDL2-2.0.3
cl /Zi /MD /I ..\.. /I ..\libs\gl3w /I %SDL2DIR%\include main.cpp imgui_impl_sdl_gl3.cpp ..\..\imgui*.cpp ..\libs\gl3w\GL\gl3w.c /link /libpath:%SDL2DIR%\lib\x86 SDL2.lib SDL2main.lib opengl32.lib /subsystem:console
