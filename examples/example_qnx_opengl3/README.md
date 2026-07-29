# Dear ImGui QNX Screen + OpenGL ES 3 example

This example uses:

- `imgui_impl_qnx.cpp` as the QNX Screen platform backend
- `imgui_impl_opengl3.cpp` as the renderer backend
- EGL to create an OpenGL ES 3 context and Screen window surface

`imgui_impl_opengl3.cpp` supports desktop OpenGL and OpenGL ES. On QNX this
example compiles it with `IMGUI_IMPL_OPENGL_ES3` and uses `#version 300 es`.
No separate Screen/OpenGL renderer backend is required.

## Build

Load the QNX SDP environment, then run:

```sh
cd examples/example_qnx_opengl3
make
```

The QNX recursive make files build for the CPUs enabled in your SDP
configuration and install the program as `example_qnx_opengl3`.

## Input

The application polls its `screen_context_t` event queue and forwards every
non-`SCREEN_EVENT_NONE` event to `ImGui_ImplQNX_ProcessEvent()`.
Escape exits the example.
