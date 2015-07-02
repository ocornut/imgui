#!/bin/sh

# This script
#
# - clones and installs glfw3, dependency of the examples
# - installs imgui
# - builds the examples in separate build tree so the imgui config
#   module is also tested
#
# Options:
#
# - you can set the variable `generator_option` to a
#   non-default generator, example:
#
#     generator_option=-GXcode ./cmake-testbuild.sh


root=$(cd $(dirname $0)/..; pwd)

set -ex

make_install() {
    srcdir=$1
    builddir=$2
    extraflags=$3
    cmake -DCMAKE_INSTALL_PREFIX=$root/out -DCMAKE_PREFIX_PATH=$root/out \
        -H$srcdir -B$root/out/build/$builddir -DCMAKE_BUILD_TYPE=Debug $extraflags \
        $generator_option
    cmake --build $root/out/build/$builddir --target install --config Debug
    cmake $root/out/build/$builddir -DCMAKE_BUILD_TYPE=Release
    cmake --build $root/out/build/$builddir --target install --config Release
}

if test ! -f $root/out/glfw_src/CMakeLists.txt; then
    git clone --depth 1 https://github.com/shaxbee/glfw.git $root/out/glfw_src
fi

make_install $root/out/glfw_src glfw "-DGLFW_BUILD_DOCS=0 -DGLFW_BUILD_EXAMPLES=0 -DGLFW_BUILD_TESTS=0 -DCMAKE_DEBUG_POSTFIX=d"
make_install $root/cmake imgui
make_install $root/examples/opengl_example opengl_example
make_install $root/examples/opengl3_example opengl3_example

make_install $root/examples/imconfig_example imconfig_example

set +x
echo "---- You can run these examples from $root/out/bin: ----"

ls -l $root/out/bin
