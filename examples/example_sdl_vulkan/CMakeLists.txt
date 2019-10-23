# Example usage:
#  cmake -S . -B build -g "Visual Studio 15 2017"
#  cmake --build build
#  ./build/example_sdl_vulkan

# You will need SDL2 (http://www.libsdl.org) and vulkan headers:
# Debian/Ubuntu:
#   apt-get install libsdl2-dev libvulkan-dev
# Arch Linux
#   pacman -S sdl2 vulkan-headers

cmake_minimum_required(VERSION 3.7)
project(example_sdl_vulkan CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# Libraries
# SDL
find_package(SDL2 REQUIRED)
# unify includes as targets
if(NOT TARGET SDL2::SDL2)
  message(STATUS "create target SDL2::SDL2 with include dir '${SDL2_INCLUDE_DIRS}' and library '${SDL2_LIBRARIES}'")
  add_library(SDL2::SDL2 IMPORTED INTERFACE)
  target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
  set_property(TARGET SDL2::SDL2 PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARIES})
endif()

# Vulkan
# since CMake 3.7.2 https://cmake.org/cmake/help/v3.7/module/FindVulkan.html
include(FindVulkan)
if(NOT Vulkan_FOUND)
  message(FATAL_ERROR "Vulkan library was not found)")
endif()

# Add executable
add_executable(${PROJECT_NAME}
  main.cpp
  ../imgui_impl_sdl.cpp
  ../imgui_impl_sdl.h
  ../imgui_impl_vulkan.cpp
  ../imgui_impl_vulkan.h
  ../../imgui.cpp
  ../../imgui.h
  ../../imgui_internal.h
  ../../imgui_draw.cpp
  ../../imgui_demo.cpp
  ../../imgui_widgets.cpp)
# Dear ImGui
target_include_directories(${PROJECT_NAME} PRIVATE
  ..
  ../..)
target_link_libraries(${PROJECT_NAME} PRIVATE
  SDL2::SDL2
  Vulkan::Vulkan)

if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2::SDL2main)
endif()
