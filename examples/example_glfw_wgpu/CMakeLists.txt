# Building for desktop (WebGPU-native) with Dawn:
#  1. git clone https://github.com/google/dawn dawn
#  2. cmake -B build -DIMGUI_DAWN_DIR=dawn
#  3. cmake --build build
# The resulting binary will be found at one of the following locations:
#   * build/Debug/example_glfw_wgpu[.exe]
#   * build/example_glfw_wgpu[.exe]

# Building for Emscripten:
#  1. Install Emscripten SDK following the instructions: https://emscripten.org/docs/getting_started/downloads.html
#  2. Install Ninja build system
#  3. emcmake cmake -G Ninja -B build
#  3. cmake --build build
#  4. emrun build/index.html

cmake_minimum_required(VERSION 3.10.2)
project(imgui_example_glfw_wgpu C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17) # Dawn requires C++17

# Dear ImGui
set(IMGUI_DIR ../../)

# Libraries
include(FetchGLFW3WebGPU.cmake)
include(FetchWgpuNative.cmake)
include(FetchGLFW.cmake)

add_executable(example_glfw_wgpu
  main.cpp
  # backend files
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
  # Dear ImGui files
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
)
target_include_directories(example_glfw_wgpu PRIVATE
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

target_link_libraries(example_glfw_wgpu PRIVATE
  webgpu
  glfw
  glfw3webgpu
)

target_copy_webgpu_binaries(example_glfw_wgpu)

# Emscripten settings
if(EMSCRIPTEN)
  target_compile_options(example_glfw_wgpu PRIVATE
    -sDISABLE_EXCEPTION_CATCHING=1
    -DIMGUI_DISABLE_FILE_FUNCTIONS=1
  )

  target_link_options(example_glfw_wgpu PRIVATE
    "-sUSE_WEBGPU=1"
    "-sUSE_GLFW=3"
    "-sWASM=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sNO_EXIT_RUNTIME=0"
    "-sASSERTIONS=1"
    "-sDISABLE_EXCEPTION_CATCHING=1"
    "-sNO_FILESYSTEM=1"
  )
  set_target_properties(example_glfw_wgpu PROPERTIES OUTPUT_NAME "index")
  # copy our custom index.html to build directory
  add_custom_command(TARGET example_glfw_wgpu POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/web/index.html" $<TARGET_FILE_DIR:example_glfw_wgpu>
  )
endif()
