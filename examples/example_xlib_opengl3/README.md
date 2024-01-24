
# How to Build

## Linux and similar Unixes

Use our Makefile or directly:
```
c++ -I .. -I ../.. -I ../../backends
  main.cpp ../../backends/imgui_impl_xlib.cpp ../../backends/imgui_impl_opengl3.cpp ../../imgui*.cpp
   -lGL -lX11 -lXi -ldl
```
