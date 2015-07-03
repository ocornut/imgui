Please note that `imgui` has been designed to be used without CMake or any other
build tool, by simply adding the `imgui` source files to your project.

But in case it fits your workflow better you can use `imgui` just like
any other CMake-enabled library. To allow for the same customization features
as without CMake, `imgui` will be added to your project as a set of source
files and not as an actual compiled library.

Usage:
------

Build and install the CMakeLists.txt in this directory to create the
`imgui-config.cmake` config-module. Then,

- with CMake version 3.3 or later:

    find_package(imgui REQUIRED)
    ...
    target_link_libraries(<my-target> ... imgui ...)

- with CMake versions 2.8.12 - 3.2

    find_package(imgui REQUIRED)
    ...
    add_exutable(<my-target> ... ${IMGUI_SOURCES})
    target_link_libraries(<my-target> ... imgui ...)

The first method requires the CMake target property `INTERFACE_SOURCES` which
is implemented completely only in V3.3.

For a quick test of the CMake build and also as an example check out
`cmake-testbuild.sh` in this directory.

Customization
-------------

To use a custom imconfig.h,

- copy `include/imconfig-sample.h` from the install directory to your project and
  edit it

- define `IMGUI_INCLUDE_IMCONFIG_H` in your project and add its path:

    target_compile_definitions(<my-target> PRIVATE IMGUI_INCLUDE_IMCONFIG_H)
    target_include_directories(<my-target> PRIVATE <path-to-imgui.h>)

Also, see `examples/imconfig_example`

