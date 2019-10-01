# Example usage:
#  cmake -S . -B build -g "Visual Studio 15 2017"
#  cmake --build build
#  ./build/example_glfw_opengl2

# You will need glfw3 and OpenGL headers:
# Debian/Ubuntu:
#   apt-get install libglfw3-dev
# Arch Linux
#   pacman -S glfw-x11
#   or for wayland
#   pacman -S glfw-wayland

cmake_minimum_required(VERSION 3.8)
project(example_glfw_opengl2 C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# Libraries
# glfw3
find_package(glfw3 CONFIG REQUIRED)

# OpenGL
# target OpenGL::GL since https://cmake.org/cmake/help/v3.8/module/FindOpenGL.html
set(OpenGL_GL_PREFERENCE "GLVND")
include(FindOpenGL)
if(NOT OPENGL_FOUND)
  message(FATAL_ERROR "OpenGL library was not found)")
endif()

# Add executable
add_executable(${PROJECT_NAME}
  main.cpp
  ../imgui_impl_glfw.cpp
  ../imgui_impl_glfw.h
  ../imgui_impl_opengl2.cpp
  ../imgui_impl_opengl2.h
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
  OpenGL::GL)

# select an OpenGL loader with choices GL3W, GLEW or GLAD
set(OPENGL_LOADER "GL3W" CACHE STRING
  "Dear ImGui OpenGL loader (GL3W, GLEW, GLAD or custom header)")
set_property(CACHE OPENGL_LOADER PROPERTY STRINGS "GL3W;GLEW;GLAD")
if(OPENGL_LOADER STREQUAL "GL3W")
  message(STATUS "Using OpenGL loader gl3w")
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GL3W)
  target_sources(${PROJECT_NAME} PRIVATE
    ../libs/gl3w/GL/gl3w.c
    ../libs/gl3w/GL/gl3w.h)
  target_include_directories(${PROJECT_NAME} PRIVATE
    ../libs/gl3w)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_DL_LIBS}) # -ldl, see https://cmake.org/cmake/help/v3.0/variable/CMAKE_DL_LIBS.html
elseif(OPENGL_LOADER STREQUAL "GLEW")
  message(STATUS "Using OpenGL loader glew")
  include(FindGLEW)
  if(NOT GLEW_FOUND)
    message(FATAL_ERROR "GLEW library was not found)")
  endif()
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLEW)
  target_include_directories(${PROJECT_NAME} PRIVATE
    ${GLEW_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GLEW_LIBRARIES})
elseif(OPENGL_LOADER STREQUAL "GLAD")
  message(STATUS "Using OpenGL loader glad")
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLAD)
  target_sources(${PROJECT_NAME} PRIVATE
    ../libs/glad/src/glad.c
    ../libs/glad/include/glad/glad.h
    ../libs/glad/include/KHR/khrplatform.h)
  target_include_directories(${PROJECT_NAME} PRIVATE
    ../libs/glad/include)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_DL_LIBS}) # -ldl, see https://cmake.org/cmake/help/v3.0/variable/CMAKE_DL_LIBS.html
else()
  message(FATAL_ERROR "Unhandled OpenGL loader '${OPENGL_LOADER}'")
endif()

if (APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    "-framework Cocoa" "-framework OpenGL" "-framework IOKit" "-framework CoreVideo")
endif()
