# How to Build

## Windows with Visual Studio's IDE

Use the provided project file (`example_sdl2_surface.vcxproj`) or open `imgui_examples.sln`.

## Windows with Visual Studio's CLI

Use `build_win32.bat` or directly:
```
set SDL2_DIR=path_to_your_sdl2_folder
cl /Zi /MD /utf-8 /I.. /I..\.. /I%SDL2_DIR%\include main.cpp ..\..\backends\imgui_impl_sdl2.cpp ..\..\backends\imgui_impl_sdlsurface2.cpp ..\..\imgui*.cpp /FeDebug/example_sdl2_surface.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x86 SDL2.lib SDL2main.lib /subsystem:console
#                 ^^ include paths                  ^^ source files                                                                                       ^^ output exe                      ^^ output dir   ^^ libraries
# or for 64-bit:
cl /Zi /MD /utf-8 /I.. /I..\.. /I%SDL2_DIR%\include main.cpp ..\..\backends\imgui_impl_sdl2.cpp ..\..\backends\imgui_impl_sdlsurface2.cpp ..\..\imgui*.cpp /FeDebug/example_sdl2_surface.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x64 SDL2.lib SDL2main.lib /subsystem:console
```

## Linux and similar Unixes

Use the provided `Makefile` or directly:
```
c++ `sdl2-config --cflags` -I .. -I ../.. -I ../../backends \
  main.cpp ../../backends/imgui_impl_sdl2.cpp ../../backends/imgui_impl_sdlsurface2.cpp ../../imgui*.cpp \
  `sdl2-config --libs`
```

## macOS

Use the provided `Makefile` or directly:
```
brew install sdl2
c++ `sdl2-config --cflags` -I .. -I ../.. -I ../../backends \
  main.cpp ../../backends/imgui_impl_sdl2.cpp ../../backends/imgui_impl_sdlsurface2.cpp ../../imgui*.cpp \
  `sdl2-config --libs`
```
