
# How to Build

## Windows with Visual Studio's IDE

Use the provided project file (.vcxproj). Add to solution (imgui_examples.sln) if necessary.

## Windows with Visual Studio's CLI

Use build_win32.bat or directly:
```
set SDL2_DIR=path_to_your_sdl3_folder
cl /Zi /MD /utf-8 /I.. /I..\.. /I%SDL2_DIR%\include main.cpp ..\..\backends\imgui_impl_sdl3.cpp ..\..\backends\imgui_impl_opengl3.cpp ..\..\imgui*.cpp /FeDebug/example_sdl3_opengl3.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x86 SDL3.lib opengl32.lib /subsystem:console
#                 ^^ include paths     ^^ source files                                                                                    ^^ output exe                    ^^ output dir   ^^ libraries
# or for 64-bit:
cl /Zi /MD /utf-8 /I.. /I..\.. /I%SDL2_DIR%\include main.cpp ..\..\backends\imgui_impl_sdl3.cpp ..\..\backends\imgui_impl_opengl3.cpp ..\..\imgui*.cpp /FeDebug/example_sdl3_opengl3.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x64 SDL3.lib SDL2mainopengl32.lib /subsystem:console
```

## Linux and similar Unixes

Use our Makefile or directly:
```
c++ `sdl3-config --cflags` -I .. -I ../.. -I ../../backends
  main.cpp ../../backends/imgui_impl_sdl3.cpp ../../backends/imgui_impl_opengl3.cpp ../../imgui*.cpp
  `sdl3-config --libs` -lGL -ldl
```

## macOS

Use our Makefile or directly:
```
brew install sdl3
c++ `sdl3-config --cflags` -I .. -I ../.. -I ../../backends
  main.cpp ../../backends/imgui_impl_sdl3.cpp ../../backends/imgui_impl_opengl3.cpp ../../imgui*.cpp
  `sdl3-config --libs` -framework OpenGl -framework CoreFoundation
```

## Emscripten

As of 2023-05-30 Emscripten doesn't support SDL3 yet.
