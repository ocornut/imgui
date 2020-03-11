
# How to Build

- You need to install Emscripten from https://emscripten.org/docs/getting_started/downloads.html, and have the environment variables set, as described in https://emscripten.org/docs/getting_started/downloads.html#installation-instructions

- Depending on your configuration, in Windows you may need to run `emsdk/emsdk_env.bat` in your console to access the Emscripten command-line tools.

- Then build using `make` while in the `example_emscripten/` directory.

- Note that Emscripten 1.39.0 (October 2019) obsoleted the `BINARYEN_TRAP_MODE=clamp` compilation flag which was required with version older than 1.39.0 to avoid rendering artefacts. See [#2877](https://github.com/ocornut/imgui/issues/2877) for details. If you use an older version, uncomment this line in the Makefile:

`#EMS += -s BINARYEN_TRAP_MODE=clamp`
