
## How to Build

- You need to install Emscripten from https://emscripten.org/docs/getting_started/downloads.html, and have the environment variables set, as described in https://emscripten.org/docs/getting_started/downloads.html#installation-instructions
- You may also refer to our [Continuous Integration setup](https://github.com/ocornut/imgui/tree/master/.github/workflows) for Emscripten setup.
- Depending on your configuration, in Windows you may need to run `emsdk/emsdk_env.bat` in your console to access the Emscripten command-line tools.
- Then build using `make` while in the `example_emscripten_opengl3/` directory.

## How to Run

To run on a local machine:
- Generally you may need a local webserver. Quoting [https://emscripten.org/docs/getting_started](https://emscripten.org/docs/getting_started/Tutorial.html#generating-html):<br>
_"Unfortunately several browsers (including Chrome, Safari, and Internet Explorer) do not support file:// [XHR](https://emscripten.org/docs/site/glossary.html#term-xhr) requests, and can’t load extra files needed by the HTML (like a .wasm file, or packaged file data as mentioned lower down). For these browsers you’ll need to serve the files using a [local webserver](https://emscripten.org/docs/getting_started/FAQ.html#faq-local-webserver) and then open http://localhost:8000/hello.html."_
- Emscripten SDK has a handy `emrun` command: `emrun example_emscripten_opengl3.html` which will spawn a temporary local webserver. See https://emscripten.org/docs/compiling/Running-html-files-with-emrun.html for details.
- Otherwise you may use Python builtin webserver: `python -m http.server` in Python 3 or `python -m SimpleHTTPServer` in Python 2. After doing that, you can visit http://localhost:8000/.

## Obsolete features:

- Emscripten 2.0 (August 2020) obsoleted the fastcomp backend, only llvm is supported.
- Emscripten 1.39.0 (October 2019) obsoleted the `BINARYEN_TRAP_MODE=clamp` compilation flag which was required with version older than 1.39.0 to avoid rendering artefacts. See [#2877](https://github.com/ocornut/imgui/issues/2877) for details. If you use an older version, uncomment this line in the Makefile: `#EMS += -s BINARYEN_TRAP_MODE=clamp`
