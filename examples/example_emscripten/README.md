
# How to Build

- You need to install Emscripten from https://emscripten.org/docs/getting_started/downloads.html, and have the environment variables set, as described in https://emscripten.org/docs/getting_started/downloads.html#installation-instructions

```
em++ -I.. -I../.. main.cpp ../imgui_impl_sdl.cpp ../imgui_impl_opengl3.cpp ../../imgui*.cpp -s USE_SDL=2 -s USE_WEBGL2=1 -s WASM=1 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1 -s BINARYEN_TRAP_MODE=clamp --shell-file shell_minimal.html -o example-emscripten.html
```
