#!/bin/sh

# This script
#
# - clones and installs glfw3, dependency of the examples
# - builds imgui as a static library
# - builds the examples in separate build tree so the imgui config
#   module is also tested

set -ex

make_install() {
    srcdir=$1
    builddir=$2
    extraflags=$3
    cmake -DCMAKE_INSTALL_PREFIX=${PWD}/out -DCMAKE_PREFIX_PATH=${PWD}/out \
        -H$srcdir -Bout/build/$builddir -DCMAKE_BUILD_TYPE=Debug $extraflags
    cmake --build out/build/$builddir --target install --config Debug
    cmake out/build/$builddir -DCMAKE_BUILD_TYPE=Release
    cmake --build out/build/$builddir --target install --config Release
}

if test ! -f out/glfw_src/CMakeLists.txt; then
    git clone --depth 1 https://github.com/shaxbee/glfw.git out/glfw_src
fi

make_install out/glfw_src glfw "-DGLFW_BUILD_DOCS=0 -DGLFW_BUILD_EXAMPLES=0 -DGLFW_BUILD_TESTS=0 -DCMAKE_DEBUG_POSTFIX=d"
make_install . imgui
make_install examples/opengl_example opengl_example
make_install examples/opengl3_example opengl3_example

set +x
echo -e "\n\nYou can run these examples from ./out/bin:\n\n"

ls out/bin -l
