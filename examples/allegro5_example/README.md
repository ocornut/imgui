
# How to Build

- On Ubuntu 14.04+

```bash
g++ -I ../imgui main.cpp imgui_impl_a5.cpp ../imgui/imgui*.cpp -lallegro -lallegro_primitives
```

- On Windows with Visual Studio's CLI

\<a5path\> is your allegro5 folder.

```
cl /MD /I <a5path\include> /I ..\imgui main.cpp imgui_impl_a5.cpp ..\imgui\imgui*.cpp /link /LIBPATH:<a5path\lib> allegro-5.0.10-monolith-md.lib user32.lib
```
