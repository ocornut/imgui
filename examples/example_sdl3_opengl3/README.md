
# How to Build

## Windows with Visual Studio's IDE

Use the provided project file (.vcxproj). Add to solution (imgui_examples.sln) if necessary.

## Windows with Visual Studio's CLI

Use build_win32.bat or directly:
```
set SDL2_DIR=path_to_your_sdl3_folder
cl /Zi /MD /I.. /I..\.. /I%SDL2_DIR%\include main.cpp ..\..\backends\imgui_impl_sdl3.cpp ..\..\backends\imgui_impl_opengl3.cpp ..\..\imgui*.cpp /FeDebug/example_sdl3_opengl3.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x86 SDL3.lib opengl32.lib /subsystem:console
#          ^^ include paths     ^^ source files                                                                                    ^^ output exe                    ^^ output dir   ^^ libraries
# or for 64-bit:
cl /Zi /MD /I.. /I..\.. /I%SDL2_DIR%\include main.cpp ..\..\backends\imgui_impl_sdl3.cpp ..\..\backends\imgui_impl_opengl3.cpp ..\..\imgui*.cpp /FeDebug/example_sdl3_opengl3.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x64 SDL3.lib SDL2mainopengl32.lib /subsystem:console
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

**Building**

You need to install Emscripten from https://emscripten.org/docs/getting_started/downloads.html, and have the environment variables set, as described in https://emscripten.org/docs/getting_started/downloads.html#installation-instructions

- Depending on your configuration, in Windows you may need to run `emsdk/emsdk_env.bat` in your console to access the Emscripten command-line tools.
- You may also refer to our [Continuous Integration setup](https://github.com/ocornut/imgui/tree/master/.github/workflows) for Emscripten setup.
- Then build using `make -f Makefile.emscripten` while in the current directory.

**Running an Emscripten project**

To run on a local machine:
- `make -f Makefile.emscripten serve` will use Python3 to spawn a local webserver, you can then browse http://localhost:8000 to access your build.
- Otherwise, generally you will need a local webserver. Quoting [https://emscripten.org/docs/getting_started](https://emscripten.org/docs/getting_started/Tutorial.html#generating-html):<br>
_"Unfortunately several browsers (including Chrome, Safari, and Internet Explorer) do not support file:// [XHR](https://emscripten.org/docs/site/glossary.html#term-xhr) requests, and can’t load extra files needed by the HTML (like a .wasm file, or packaged file data as mentioned lower down). For these browsers you’ll need to serve the files using a [local webserver](https://emscripten.org/docs/getting_started/FAQ.html#faq-local-webserver) and then open http://localhost:8000/hello.html."_
- Emscripten SDK has a handy `emrun` command: `emrun web/index.html --browser firefox` which will spawn a temporary local webserver (in Firefox). See https://emscripten.org/docs/compiling/Running-html-files-with-emrun.html for details.
- You may use Python 3 builtin webserver: `python -m http.server -d web` (this is what `make serve` uses).
- You may use Python 2 builtin webserver: `cd web && python -m SimpleHTTPServer`.
- If you are accessing the files over a network, certain browsers, such as Firefox, will restrict Gamepad API access to secure contexts only (e.g. https only).
