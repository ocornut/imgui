
# How to Build

- On Windows with Visual Studio's CLI

```
set SDL2DIR=path_to_your_sdl2_folder
cl /Zi /MD /I ..\.. /I ..\libs\gl3w /I %SDL2DIR%\include main.cpp imgui_impl_sdl_gl3.cpp ..\..\imgui*.cpp ..\libs\gl3w\GL\gl3w.c /link /libpath:%SDL2DIR%\lib\x86 SDL2.lib SDL2main.lib opengl32.lib /subsystem:console
```

- On Linux and similar Unixes

```
c++ `sdl2-config --cflags` -I ../.. -I ../libs/gl3w main.cpp imgui_impl_sdl_gl3.cpp ../../imgui*.cpp ../libs/gl3w/GL/gl3w.c `sdl2-config --libs` -lGL -ldl -o sdl2example
```

- On Mac OS X

```
brew install sdl2
c++ `sdl2-config --cflags` -I ../.. -I ../libs/gl3w main.cpp imgui_impl_sdl_gl3.cpp ../../imgui*.cpp ../libs/gl3w/GL/gl3w.c `sdl2-config --libs` -framework OpenGl -o sdl2example
```
