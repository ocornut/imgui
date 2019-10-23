# Example usage:
#  cmake -S . -B build -g "Visual Studio 15 2017"
#  cmake --build build
#  ./build/example_sdl_opengl3

# You will need SDL2 (http://www.libsdl.org) and OpenGL headers:
# Debian/Ubuntu:
#   apt-get install libsdl2-dev
# Arch Linux
#   pacman -S sdl2

cmake_minimum_required(VERSION 3.8)
project(example_sdl_opengl3 C CXX)

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
  ../imgui_impl_sdl.cpp
  ../imgui_impl_sdl.h
  ../imgui_impl_opengl3.cpp
  ../imgui_impl_opengl3.h
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

if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2::SDL2main)
endif()
if (APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    "-framework Cocoa" "-framework OpenGL" "-framework IOKit" "-framework CoreVideo")
endif()
