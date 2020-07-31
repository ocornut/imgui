if ! test -d Debug; then
    mkdir Debug
fi

if ! test -d Release; then
    mkdir Release
fi

cc -std=c++17 -g -I .. -I ../.. -o Debug/example_x11_vulkan main.cpp ../imgui_impl_x11.cpp ../imgui_impl_vulkan.cpp ../../imgui*.cpp -lm -lxcb -lxcb-keysyms -lxcb-xfixes -lstdc++ -lvulkan -lrt
cc -std=c++17 -O3 -I .. -I ../.. -o Release/example_x11_vulkan main.cpp ../imgui_impl_x11.cpp ../imgui_impl_vulkan.cpp ../../imgui*.cpp -lm -lxcb -lxcb-keysyms -lxcb-xfixes -lstdc++ -lvulkan -lrt
