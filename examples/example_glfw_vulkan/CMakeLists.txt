# Example usage:
#  cmake -S . -B build -g "Visual Studio 15 2017"
#  cmake --build build
#  ./build/example_glfw_vulkan

# You will need glfw3 and vulkan headers:
# Debian/Ubuntu:
#   apt-get install libglfw3-dev libvulkan-dev
# Arch Linux
#   pacman -S glfw-x11 vulkan-headers
#   or for wayland
#   pacman -S glfw-wayland vulkan-headers

cmake_minimum_required(VERSION 3.7)
project(example_glfw_vulkan CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# Libraries
# glfw3
find_package(glfw3 CONFIG REQUIRED)

# Vulkan
# since CMake 3.7.2 https://cmake.org/cmake/help/v3.7/module/FindVulkan.html
include(FindVulkan)
if(NOT Vulkan_FOUND)
  message(FATAL_ERROR "Vulkan library was not found)")
endif()

# Add executable
add_executable(${PROJECT_NAME}
  main.cpp
  ../imgui_impl_glfw.cpp
  ../imgui_impl_glfw.h
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
  glfw
  Vulkan::Vulkan)
