cmake_minimum_required(VERSION 2.8)
project(imgui_example_sdl_gl)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif ()

if (EMSCRIPTEN)
  # Use GL version "ES_2" (WebGL 1) by default. "ES_3" (WebGL 2) is also supported.
  set(USE_GL_VERSION "ES_2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMGUI_IMPL_OPENGL_${USE_GL_VERSION}")

  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")
  set(ldflags "${ldflags} -o imgui_example_sdl_gl.html")
  set(ldflags "${ldflags} -s USE_SDL=2")
  if(USE_GL_VERSION STREQUAL "ES_3")
    set(ldflags "${ldflags} -s USE_WEBGL2=1")
  endif()
  set(ldflags "${ldflags} -s BINARYEN_TRAP_MODE=js")
else ()
  # Use GL version "3" by default. "4", "ES_2", "ES_3" are also supported
  set(USE_GL_VERSION "3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMGUI_IMPL_OPENGL_${USE_GL_VERSION}")

  # SDL2
  set(SDL2_BUILDING_LIBRARY TRUE)
  find_package(SDL2 REQUIRED)
  include_directories(${SDL2_INCLUDE_DIR})

  # OpenGL
  find_package(OpenGL REQUIRED)
  include_directories(${OPENGL_INCLUDE_DIR})

  # GLEW
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMGUI_IMPL_OPENGL_LOADER_GLEW")
  find_package(GLEW REQUIRED)
  include_directories(${GLEW_INCLUDE_DIR})

  # Configure libraries
  set(LIBRARIES ${SDL2_LIBRARY} ${OPENGL_LIBRARIES} ${GLEW_LIBRARY})
endif ()

#define
# ImGui
set(IMGUI_DIR ../../)
include_directories(${IMGUI_DIR} ..)

file(GLOB sources *.cpp)

add_executable(imgui_example_sdl_gl ${sources} ${IMGUI_DIR}/examples/imgui_impl_gl.cpp ${IMGUI_DIR}/examples/imgui_impl_sdl.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
target_link_libraries(imgui_example_sdl_gl ${LIBRARIES})
set_target_properties(imgui_example_sdl_gl PROPERTIES LINK_FLAGS "${ldflags}")
