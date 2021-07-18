
# How to Build

- You need to install Emscripten from https://emscripten.org/docs/getting_started/downloads.html, and have the environment variables set, as described in https://emscripten.org/docs/getting_started/downloads.html#installation-instructions

- Depending on your configuration, in Windows you may need to run `emsdk/emsdk_env.bat` in your console to access the Emscripten command-line tools.

- Then build using `make` while in the `example_emscripten_wgpu/` directory.

- Requires Emscripten 2.0.10 (December 2020) due to GLFW adaptations
