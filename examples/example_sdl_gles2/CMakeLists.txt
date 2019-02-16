cmake_minimum_required(VERSION 2.8)
project(imgui_example_sdl_gles2)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif ()

if (EMSCRIPTEN)
  SET(CMAKE_EXECUTABLE_SUFFIX ".html")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")

  set(ldflags "${ldflags} -o imgui_example_sdl_gles2.html")
  set(ldflags "${ldflags} -s USE_SDL=2")
  set(ldflags "${ldflags} -s BINARYEN_TRAP_MODE=js")
else ()
  # SDL2
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
  set(SDL2_BUILDING_LIBRARY TRUE)
  find_package(SDL2 REQUIRED)
  include_directories(${SDL2_INCLUDE_DIR})

  # OpenGL
  find_package(OpenGL REQUIRED)
  include_directories(${OPENGL_INCLUDE_DIR})

  #
  set(LIBRARIES ${SDL2_LIBRARY} ${OPENGL_LIBRARIES})
endif ()

#define
# ImGui
set(IMGUI_DIR ../../)
include_directories(${IMGUI_DIR} ..)

file(GLOB sources *.cpp)

add_executable(imgui_example_sdl_gles2 ${sources} ${IMGUI_DIR}/examples/imgui_impl_gles2.cpp ${IMGUI_DIR}/examples/imgui_impl_sdl.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
target_link_libraries(imgui_example_sdl_gles2 ${LIBRARIES})
set_target_properties(imgui_example_sdl_gles2 PROPERTIES LINK_FLAGS "${ldflags}")
