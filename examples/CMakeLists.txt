cmake_minimum_required(VERSION 3.7)
project(examples)

option(BUILD_EXAMPLES_SDL     "Build examples for SDL backend"     ON)
option(BUILD_EXAMPLES_GLFW    "Build examples for glfw backend"    ON)
option(BUILD_EXAMPLES_OPENGL2 "Build examples for OpenGL2 backend" ON)
option(BUILD_EXAMPLES_OPENGL3 "Build examples for OpenGL3 backend" ON)
option(BUILD_EXAMPLES_VULKAN  "Build examples for Vulkan backend"  ON)

if (BUILD_EXAMPLES_SDL)
  # get dependency SDL2 once for all dependending examples
  find_package(SDL2 REQUIRED)
  # unify includes as targets
  if(NOT TARGET SDL2::SDL2)
    message(STATUS "create target SDL2::SDL2 with include dir '${SDL2_INCLUDE_DIRS}' and library '${SDL2_LIBRARIES}'")
    add_library(SDL2::SDL2 IMPORTED INTERFACE)
    target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
    set_property(TARGET SDL2::SDL2 PROPERTY INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARIES})
  endif()

  if (BUILD_EXAMPLES_OPENGL2)
    message(STATUS "add example_sdl_opengl2")
    add_subdirectory(example_sdl_opengl2)
  endif()

  if (BUILD_EXAMPLES_OPENGL3)
    message(STATUS "add example_sdl_opengl3")
    add_subdirectory(example_sdl_opengl3)
  endif()

  if (BUILD_EXAMPLES_VULKAN)
    message(STATUS "add example_sdl_vulkan")
    add_subdirectory(example_sdl_vulkan)
  endif()
endif()

if (BUILD_EXAMPLES_GLFW)
  # get dependency for glfw once for all dependending examples
  find_package(glfw3 CONFIG REQUIRED)

  if (BUILD_EXAMPLES_OPENGL2)
    message(STATUS "add example_glfw_opengl2")
    add_subdirectory(example_glfw_opengl2)
  endif()

  if (BUILD_EXAMPLES_OPENGL3)
    message(STATUS "add example_glfw_opengl3")
    add_subdirectory(example_glfw_opengl3)
  endif()

  if (BUILD_EXAMPLES_VULKAN)
    message(STATUS "add example_glfw_vulkan")
    add_subdirectory(example_glfw_vulkan)
  endif()
endif()

