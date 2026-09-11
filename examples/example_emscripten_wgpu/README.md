## How to Build

- You need to install Emscripten from https://emscripten.org/docs/getting_started/downloads.html, and have the environment variables set, as described in https://emscripten.org/docs/getting_started/downloads.html#installation-instructions

- Depending on your configuration, in Windows you may need to run `emsdk/emsdk_env.bat` in your console to access the Emscripten command-line tools.

- You may also refer to our [Continuous Integration setup](https://github.com/ocornut/imgui/tree/master/.github/workflows) for Emscripten setup.

- Then build using `make -f Makefile.emscripten` while in the `example_emscripten_wgpu/` directory.

- This example is browser-only. It uses `imgui_impl_emscripten` for platform input and `imgui_impl_wgpu` for rendering.

- Requires recent Emscripten with the `emdawnwebgpu` port available.

## How to Run

To run on a local machine:
- Make sure your browser supports WebGPU and it is enabled.
- `emrun web/index.html` will spawn a temporary local webserver and open the example in your browser. See https://emscripten.org/docs/compiling/Running-html-files-with-emrun.html for details.
- Otherwise, generally you will need a local webserver:
  - Quoting [https://emscripten.org/docs/getting_started](https://emscripten.org/docs/getting_started/Tutorial.html#generating-html):<br>
_"Unfortunately several browsers (including Chrome, Safari, and Internet Explorer) do not support file:// [XHR](https://emscripten.org/docs/site/glossary.html#term-xhr) requests, and can’t load extra files needed by the HTML (like a .wasm file, or packaged file data as mentioned lower down). For these browsers you’ll need to serve the files using a [local webserver](https://emscripten.org/docs/getting_started/FAQ.html#faq-local-webserver) and then open http://localhost:8000/hello.html."_
  - You may use Python 3 builtin webserver: `python -m http.server -d web` (this is what `make serve` uses).
  - You may use Python 2 builtin webserver: `cd web && python -m SimpleHTTPServer`.
