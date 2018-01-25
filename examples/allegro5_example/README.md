
# How to Build

- On Ubuntu 14.04+

```bash
g++ -I ../.. main.cpp imgui_impl_a5.cpp ../../imgui*.cpp -lallegro -lallegro_primitives -o allegro5_example
```

- On Windows with Visual Studio's CLI

```
set ALLEGRODIR=path_to_your_allegro5_folder
cl /Zi /MD /I %ALLEGRODIR%\include /I ..\.. main.cpp imgui_impl_a5.cpp ..\..\imgui*.cpp /link /LIBPATH:%ALLEGRODIR%\lib allegro-5.0.10-monolith-md.lib user32.lib
```
