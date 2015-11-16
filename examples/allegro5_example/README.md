
# How to Build

- On Ubuntu 14.04+

```bash
g++ -I ../imgui main.cpp imgui_impl_a5.cpp ../imgui/imgui*.cpp -lallegro -lallegro_primitives
```

- On Windows with Visual Studio's CLI

```
set ALLEGRODIR=path_to_your_allegro5_folder
cl /Zi /MD /I %ALLEGRODIR%\include /I ..\.. main.cpp imgui_impl_a5.cpp ..\..\imgui*.cpp /link /LIBPATH:%ALLEGRODIR%\lib allegro-5.0.10-monolith-md.lib user32.lib
```
