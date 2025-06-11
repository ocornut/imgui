# Building for desktop (WebGPU-native) with Dawn:
#  1. cmake -G Ninja -B build
#  2. cmake --build build
# The resulting binary will be found at one of the following locations:
#   * build/Debug/example_glfw_wgpu[.exe]
#   * build/example_glfw_wgpu[.exe]

# Building for Emscripten:
#  1. Install Emscripten SDK following the instructions: https://emscripten.org/docs/getting_started/downloads.html
#  2. Install Ninja build system
#  3. emcmake cmake -G Ninja -B build
#  3. cmake --build build
#  4. emrun build/index.html

cmake_minimum_required(VERSION 3.14)
project(imgui_example_glfw_wgpu)

file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage(
  NAME dawn
  GITHUB_REPOSITORY google/dawn
  VERSION 20250607.112309
  OPTIONS
    "DAWN_FETCH_DEPENDENCIES ON"
  EXCLUDE_FROM_ALL YES
  GIT_SHALLOW TRUE
  GIT_SUBMODULES "docs" # docs is not a submodules but is used to prevent submodule resolution
)

set(IMGUI_DIR ../../)
add_executable(example_glfw_wgpu
  main.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
)
target_compile_definitions(example_glfw_wgpu PUBLIC
  "IMGUI_IMPL_WEBGPU_BACKEND_DAWN"
)
target_include_directories(example_glfw_wgpu PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

if(EMSCRIPTEN)
  if (EMSCRIPTEN VERSION_GREATER_EQUAL "3.1.54")
    message(FATAL_ERROR "Emscripten version must be greater than 3.1.53 to support '--use-port' argument")
  endif()
  target_link_libraries(example_glfw_wgpu PRIVATE emdawnwebgpu_cpp)
  target_link_options(example_glfw_wgpu PRIVATE "-sASYNCIFY=1" "--use-port=contrib.glfw3") # if using emscripten 4.0.10 or above, you can use `--use-port=emdawnwebgpu`
  set_target_properties(example_glfw_wgpu PROPERTIES
    OUTPUT_NAME "index"
    SUFFIX ".html")
  # copy our custom index.html to build directory
  add_custom_command(TARGET example_glfw_wgpu POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/web/index.html" $<TARGET_FILE_DIR:example_glfw_wgpu>
  )
else()
  target_link_libraries(example_glfw_wgpu PRIVATE dawn::webgpu_dawn glfw webgpu_glfw)
endif()
