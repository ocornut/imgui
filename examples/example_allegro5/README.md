
# Configuration

Dear ImGui outputs 16-bit vertex indices by default.
Allegro doesn't support them natively, so we have two solutions: convert the indices manually in imgui_impl_allegro5.cpp, or compile dear imgui with 32-bit indices.
You can either modify imconfig.h that comes with Dear ImGui (easier), or set a C++ preprocessor option IMGUI_USER_CONFIG to find to a filename.
We are providing `imconfig_allegro5.h` that enables 32-bit indices.
Note that the back-end supports _BOTH_ 16-bit and 32-bit indices, but 32-bit indices will be slightly faster as they won't require a manual conversion.

# How to Build

### On Ubuntu 14.04+

```bash
g++ -DIMGUI_USER_CONFIG=\"examples/example_allegro5/imconfig_allegro5.h\" -I .. -I ../.. main.cpp ../imgui_impl_allegro5.cpp ../../imgui*.cpp -lallegro -lallegro_primitives -o allegro5_example
```

### On Windows with Visual Studio's CLI

You may install Allegro using vcpkg:
```
git clone https://github.com/Microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install allegro5
.\vcpkg integrate install    ; optional, automatically register include/libs in Visual Studio
```

Build:
```
set ALLEGRODIR=path_to_your_allegro5_folder
cl /Zi /MD /I %ALLEGRODIR%\include /DIMGUI_USER_CONFIG=\"examples/example_allegro5/imconfig_allegro5.h\" /I .. /I ..\.. main.cpp ..\imgui_impl_allegro5.cpp ..\..\imgui*.cpp /link /LIBPATH:%ALLEGRODIR%\lib allegro-5.0.10-monolith-md.lib user32.lib
```
