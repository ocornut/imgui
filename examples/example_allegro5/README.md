
# Configuration

Dear ImGui outputs 16-bit vertex indices by default.
Allegro doesn't support them natively, so we have two solutions: convert the indices manually in imgui_impl_allegro5.cpp, or compile dear imgui with 32-bit indices.
You can either modify imconfig.h that comes with Dear ImGui (easier), or set a C++ preprocessor option IMGUI_USER_CONFIG to find to a filename.
We are providing `imconfig_allegro5.h` that enables 32-bit indices.
Note that the backend supports _BOTH_ 16-bit and 32-bit indices, but 32-bit indices will be slightly faster as they won't require a manual conversion.

# How to Build

### On Ubuntu 14.04+ and macOS

```bash
g++ -DIMGUI_USER_CONFIG=\"examples/example_allegro5/imconfig_allegro5.h\" -I .. -I ../.. -I ../../backends main.cpp ../../backends/imgui_impl_allegro5.cpp ../../imgui*.cpp -lallegro -lallegro_main -lallegro_primitives -o allegro5_example
```

On macOS, install Allegro with homebrew: `brew install allegro`.

### On Windows with Visual Studio's CLI

You may install Allegro using vcpkg:
```
git clone https://github.com/Microsoft/vcpkg
cd vcpkg
bootstrap-vcpkg.bat
vcpkg install allegro5 --triplet=x86-windows	; for win32
vcpkg install allegro5 --triplet=x64-windows	; for win64
vcpkg integrate install						    ; register include / libs in Visual Studio
```

Build:
```
set ALLEGRODIR=path_to_your_allegro5_folder
cl /Zi /MD /utf-8 /I %ALLEGRODIR%\include /DIMGUI_USER_CONFIG=\"examples/example_allegro5/imconfig_allegro5.h\" /I .. /I ..\.. /I ..\..\backends main.cpp ..\..\backends\imgui_impl_allegro5.cpp ..\..\imgui*.cpp /link /LIBPATH:%ALLEGRODIR%\lib allegro-5.0.10-monolith-md.lib user32.lib
```
