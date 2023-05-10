
# How to Build

- On Windows with Visual Studio's CLI

```
set SDL2_DIR=path_to_your_sdl2_folder
cl /Zi /MD /utf-8 /I.. /I..\.. /I%SDL2_DIR%\include main.cpp ..\..\backends\imgui_impl_sdl2.cpp ..\..\backends\imgui_impl_sdlrenderer.cpp ..\..\imgui*.cpp /FeDebug/example_sdl2_sdlrenderer.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x86 SDL2.lib SDL2main.lib /subsystem:console
#                 ^^ include paths                  ^^ source files                                                                                         ^^ output exe                         ^^ output dir  ^^ libraries
# or for 64-bit:
cl /Zi /MD /utf-8 /I.. /I..\.. /I%SDL2_DIR%\include main.cpp ..\..\backends\imgui_impl_sdl2.cpp ..\..\backends\imgui_impl_sdlrenderer.cpp ..\..\imgui*.cpp /FeDebug/example_sdl2_sdlrenderer.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x64 SDL2.lib SDL2main.lib /subsystem:console
```

- On Linux and similar Unixes

```
c++ `sdl2-config --cflags` -I .. -I ../.. main.cpp ../../backends/imgui_impl_sdl2.cpp ../../backends/imgui_impl_sdlrenderer.cpp ../../imgui*.cpp `sdl2-config --libs` -lGL
```

- On Mac OS X

```
brew install sdl2
c++ `sdl2-config --cflags` -I .. -I ../.. main.cpp ../../backends/imgui_impl_sdl2.cpp ../../backends/imgui_impl_sdlrenderer.cpp ../../imgui*.cpp `sdl2-config --libs` -framework OpenGl
```
