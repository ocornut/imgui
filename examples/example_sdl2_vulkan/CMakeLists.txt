# Example usage:
#  mkdir build
#  cd build
#  cmake -g "Visual Studio 14 2015" ..

cmake_minimum_required(VERSION 2.8)
project(imgui_example_sdl2_vulkan C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 11)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

# SDL2
set(SDL2_DIR ../../../SDL) # Set this to point to an SDL2 repo
add_subdirectory(${SDL2_DIR} binary_dir EXCLUDE_FROM_ALL)

find_package(Vulkan REQUIRED)

# Dear ImGui
set(IMGUI_DIR ../../)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)


# Use vulkan headers from SDL2:
include_directories(${SDL2_DIR}/video/khronos)

file(GLOB sources *.cpp)

add_executable(example_sdl2_vulkan ${sources} ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
target_link_libraries(example_sdl2_vulkan SDL2::SDL2 Vulkan::Vulkan)
target_compile_definitions(example_sdl2_vulkan PUBLIC -DImTextureID=ImU64)
