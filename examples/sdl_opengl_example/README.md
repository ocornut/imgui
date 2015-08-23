
# How to Build

- On Windows with Visual Studio's CLI

\<sdl2path\> is your SDL2 folder.

```
cl /MD /I <sdl2path\include> /I ..\.. main.cpp imgui_impl_sdl.cpp ..\..\imgui*.cpp /link /LIBPATH:<sdl2path\lib> SDL2.lib SDL2main.lib
```

- On Linux and similar Unixes

```
c++ `sdl2-config --cflags` -I ../.. main.cpp imgui_impl_sdl.cpp ../../imgui*.cpp `sdl2-config --libs` -lGL -o sdl2example
```

- On Mac OS X

```
brew install sdl2
c++ `sdl2-config --cflags` -I ../.. main.cpp imgui_impl_sdl.cpp ../../imgui*.cpp `sdl2-config --libs` -framework OpenGl -o sdl2example
```
