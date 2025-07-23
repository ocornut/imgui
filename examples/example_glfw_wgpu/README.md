## How to Build

### Desktop Builds

- Download and compile one of the three WebGPU on desktop implementations:
  - [Dawn](https://dawn.googlesource.com/dawn)
  - [WGPU](https://github.com/gfx-rs/wgpu-native) (requires Rust)
  - [WGVK](https://github.com/manuel5975p/WGVK) (Lightweight, Vulkan only)


Once compiled, imgui's backend code can be compiled and linked to the webgpu implementation library, as an example to WGVK on X11:

```
g++ -o example -D_GLFW_X11 -DIMGUI_IMPL_WEBGPU_BACKEND_DAWN imgui_demo.cpp imgui.cpp imgui_draw.cpp imgui_widgets.cpp imgui_tables.cpp examples/example_glfw_wgpu/main.cpp backends/imgui_impl_wgpu.cpp backends/imgui_impl_glfw.cpp -I . -I backends -I <path_to_WGVK>/include/ <path_to_WGVK>/build/libwgvk.a
```

This command is assumed to be run from imgui's root directory.

Explanation of the options:
- `-D_GLFW_X11` required to expose X11 handles. Replace with `-D_GLFW_WAYLAND` for wayland or `-D_GLFW_WIN32` for windows
- `-DIMGUI_IMPL_WEBGPU_BACKEND_DAWN` because WGVK mimics dawn
- `-I . -I backends` Include paths for imgui
- `-I <path_to_WGVK>/include/` include path for `<webgpu/webgpu.h>`

### Web Builds
- You need to install Emscripten from https://emscripten.org/docs/getting_started/downloads.html, and have the environment variables set, as described in https://emscripten.org/docs/getting_started/downloads.html#installation-instructions

- Depending on your configuration, in Windows you may need to run `emsdk/emsdk_env.bat` in your console to access the Emscripten command-line tools.

- You may also refer to our [Continuous Integration setup](https://github.com/ocornut/imgui/tree/master/.github/workflows) for Emscripten setup.

- Then build using `make -f Makefile.emscripten` while in the `example_glfw_wgpu/` directory.

- Requires recent Emscripten as WGPU is still a work-in-progress API.

## How to run Web Builds

To run on a local machine:
- For Chrome:
  - Enable the experimental flags JSPI and WebGPU in chrome://flags/
- For Firefox:
  - Enable the experimental flags wasm_js_promise_integration and WebGPU in about:config

- `make serve` will use Python3 to spawn a local webserver, you can then browse http://localhost:8000 to access your build.
- Otherwise, generally you will need a local webserver:
  - Quoting [https://emscripten.org/docs/getting_started](https://emscripten.org/docs/getting_started/Tutorial.html#generating-html):<br>
_"Unfortunately several browsers (including Chrome, Safari, and Internet Explorer) do not support file:// [XHR](https://emscripten.org/docs/site/glossary.html#term-xhr) requests, and can’t load extra files needed by the HTML (like a .wasm file, or packaged file data as mentioned lower down). For these browsers you’ll need to serve the files using a [local webserver](https://emscripten.org/docs/getting_started/FAQ.html#faq-local-webserver) and then open http://localhost:8000/hello.html."_
  - Emscripten SDK has a handy `emrun` command: `emrun web/example_glfw_wgpu.html --browser firefox` which will spawn a temporary local webserver (in Firefox). See https://emscripten.org/docs/compiling/Running-html-files-with-emrun.html for details.
  - You may use Python 3 builtin webserver: `python -m http.server -d web` (this is what `make serve` uses).
  - You may use Python 2 builtin webserver: `cd web && python -m SimpleHTTPServer`.
  - If you are accessing the files over a network, certain browsers, such as Firefox, will restrict Gamepad API access to secure contexts only (e.g. https only).
