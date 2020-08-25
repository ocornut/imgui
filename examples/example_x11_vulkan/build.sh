#!/bin/bash

mkdir -p Debug
mkdir -p Release

FILES=(main.cpp ../imgui_impl_x11.cpp ../imgui_impl_vulkan.cpp ../../imgui*.cpp)
LIBS="xcb xcb-keysyms xcb-xfixes xcb-xkb xcb-cursor xcb-randr vulkan"
cc -Wall -g -I .. -I ../.. -DHAS_VULKAN -o Debug/example_x11_vulkan "${FILES[@]}" -lm -lstdc++ -lrt $(pkg-config --libs $LIBS)
cc -Wall -O3 -I .. -I ../.. -DHAS_VULKAN -o Release/example_x11_vulkan "${FILES[@]}" -lm -lstdc++ -lrt $(pkg-config --libs $LIBS)
