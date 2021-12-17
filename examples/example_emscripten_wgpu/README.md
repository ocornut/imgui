## How to Build

- You need to install Emscripten from https://emscripten.org/docs/getting_started/downloads.html, and have the environment variables set, as described in https://emscripten.org/docs/getting_started/downloads.html#installation-instructions

- Depending on your configuration, in Windows you may need to run `emsdk/emsdk_env.bat` in your console to access the Emscripten command-line tools.

- You may also refer to our [Continuous Integration setup](https://github.com/ocornut/imgui/tree/master/.github/workflows) for Emscripten setup.

- Then build using `make` while in the `example_emscripten_wgpu/` directory.

- Requires recent Emscripten as WGPU is still a work-in-progress API.

## How to Run

To run on a local machine:
- Make sure your browse supports WGPU and it is enabled. WGPU is still WIP not enabled by default in most browser.
- `make serve` will use Python3 to spawn a local webserver, you can then browse http://localhost:8000 to access your build.
- Otherwise, generally you will need a local webserver:
  - Quoting [https://emscripten.org/docs/getting_started](https://emscripten.org/docs/getting_started/Tutorial.html#generating-html):<br>
_"Unfortunately several browsers (including Chrome, Safari, and Internet Explorer) do not support file:// [XHR](https://emscripten.org/docs/site/glossary.html#term-xhr) requests, and can’t load extra files needed by the HTML (like a .wasm file, or packaged file data as mentioned lower down). For these browsers you’ll need to serve the files using a [local webserver](https://emscripten.org/docs/getting_started/FAQ.html#faq-local-webserver) and then open http://localhost:8000/hello.html."_
  - Emscripten SDK has a handy `emrun` command: `emrun web/example_emscripten_opengl3.html --browser firefox` which will spawn a temporary local webserver (in Firefox). See https://emscripten.org/docs/compiling/Running-html-files-with-emrun.html for details.
  - You may use Python 3 builtin webserver: `python -m http.server -d web` (this is what `make serve` uses).
  - You may use Python 2 builtin webserver: `cd web && python -m SimpleHTTPServer`.
  - If you are accessing the files over a network, certain browsers, such as Firefox, will restrict Gamepad API access to secure contexts only (e.g. https only).
