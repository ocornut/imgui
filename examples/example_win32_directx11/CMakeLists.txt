# CMakeLists for building imgui example app
# To build: $ cd example_win32_directx11
#			$ mkdir build && cd build
#			$ cmake .. (Generator flag optional: eg -G 'MSYS Makefiles')
#			$ make OR ninja OR .sln (depends on your cmake-generator)

cmake_minimum_required(VERSION 3.12) # requires 3.12 for the list(TRANSFORM ...) shortcut

project(imgui_win32_directx11 LANGUAGES CXX VERSION 0.0.1)

# set the path of imgui top-level folder here
set(IMGUI_DIR ../../)

# add platform-agnostic sources...
set(IMGUI_SRC
        imgui_draw.cpp
        imgui_demo.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
        imgui.cpp)
# add platform-specific sources...
set(IMGUI_BACKEND_SRC
        backends/imgui_impl_dx11.cpp
        backends/imgui_impl_win32.cpp)

# prepend imgui top-level path to src files
list(TRANSFORM IMGUI_SRC PREPEND ${IMGUI_DIR})
list(TRANSFORM IMGUI_BACKEND_SRC PREPEND ${IMGUI_DIR})

# compiler flags
set(CFLAGS 
        -Wall 
        -Werror 
        -Wnarrowing
        -O3
        -DNDEBUG)
# necessary graphics libraries
set(LDLIBS 
        d3d11 
        d3dcompiler 
        dwmapi)

# add library with the imgui sources
add_library(imgui_win32_directx11  ${IMGUI_SRC} ${IMGUI_BACKEND_SRC})
# set compiler flags
target_compile_options(imgui_win32_directx11  PRIVATE ${CFLAGS})
# link against necessary graphics libraries
target_link_libraries(imgui_win32_directx11  PRIVATE ${LDLIBS})
# include headers from the IMGUI_DIR
target_include_directories(imgui_win32_directx11  PRIVATE ${IMGUI_DIR})

# finally, build the example main.cpp 
add_executable(imgui_win32_directx11_example main.cpp)
# link example exe against the imgui library
target_link_libraries(imgui_win32_directx11_example PRIVATE imgui_win32_directx11)
# include headers from the IMGUI_DIR and backends
target_include_directories(imgui_win32_directx11_example  PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)

