# Building for desktop (WebGPU-native) with Dawn:
#  1. git clone https://github.com/google/dawn dawn
#  2. cmake -B build -DIMGUI_DAWN_DIR=dawn
#  3. cmake --build build
# The resulting binary will be found at one of the following locations:
#   * build/Debug/example_sdl2_wgpu[.exe]
#   * build/example_sdl2_wgpu[.exe]

# Building for Emscripten:
#  1. Install Emscripten SDK following the instructions: https://emscripten.org/docs/getting_started/downloads.html
#  2. Install Ninja build system
#  3. emcmake cmake -G Ninja -B build
#  3. cmake --build build
#  4. emrun build/index.html

cmake_minimum_required(VERSION 3.10.2)
project(imgui_example_sdl2_wgpu C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17) # Dawn requires C++17

# Dear ImGui
set(IMGUI_DIR ../../)

# Libraries
if(EMSCRIPTEN)
  add_compile_options(-sDISABLE_EXCEPTION_CATCHING=1 -DIMGUI_DISABLE_FILE_FUNCTIONS=1)
else()
  # Dawn wgpu desktop
  set(DAWN_FETCH_DEPENDENCIES ON)
  set(IMGUI_DAWN_DIR CACHE PATH "path to dawn")
  if (NOT IMGUI_DAWN_DIR)
    message(FATAL_ERROR "Please specify the Dawn repository by setting IMGUI_DAWN_DIR")
  endif()

  option(DAWN_FETCH_DEPENDENCIES "Use fetch_dawn_dependencies.py as an alternative to using depot_tools" ON)

  # Dawn builds many things by default - disable things we don't need
  option(DAWN_BUILD_SAMPLES "Enables building Dawn's samples" OFF)
  option(TINT_BUILD_CMD_TOOLS "Build the Tint command line tools" OFF)
  option(TINT_BUILD_DOCS "Build documentation" OFF)
  option(TINT_BUILD_TESTS "Build tests" OFF)
  if (NOT APPLE)
    option(TINT_BUILD_MSL_WRITER "Build the MSL output writer" OFF)
  endif()
  if(WIN32)
    option(TINT_BUILD_SPV_READER "Build the SPIR-V input reader" OFF)
    option(TINT_BUILD_WGSL_READER "Build the WGSL input reader" ON)
    option(TINT_BUILD_GLSL_WRITER "Build the GLSL output writer" OFF)
    option(TINT_BUILD_GLSL_VALIDATOR "Build the GLSL output validator" OFF)
    option(TINT_BUILD_SPV_WRITER "Build the SPIR-V output writer" OFF)
    option(TINT_BUILD_WGSL_WRITER "Build the WGSL output writer" ON)
  endif()
# check if WAYLAND is the current Session Type and enable DAWN_USE_WAYLAND Wayland option @compile time
# You can override this using: cmake -DDAWN_USE_WAYLAND=X (X = ON | OFF)
  if(UNIX)
    if ($ENV{XDG_SESSION_TYPE} MATCHES wayland)
      option(DAWN_USE_WAYLAND "Enable support for Wayland surface" ON)
    endif()
  endif()

  add_subdirectory("${IMGUI_DAWN_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/dawn" EXCLUDE_FROM_ALL)

  find_package(SDL2 REQUIRED)

  set(LIBRARIES webgpu_dawn webgpu_cpp)
endif()


add_executable(example_sdl2_wgpu
  main.cpp
  sdl2wgpu.cpp
  # backend files
  ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
  ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
  # Dear ImGui files
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
)
IF(NOT EMSCRIPTEN)
  target_compile_definitions(example_sdl2_wgpu PUBLIC
    "IMGUI_IMPL_WEBGPU_BACKEND_DAWN"
  )
endif()
target_include_directories(example_sdl2_wgpu PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(example_sdl2_wgpu PUBLIC ${LIBRARIES} ${SDL2_LIBRARIES})

# Emscripten settings
if(EMSCRIPTEN)
  target_compile_options(example_sdl2_wgpu PUBLIC "-sUSE_SDL=2" )
  target_link_options(example_sdl2_wgpu PRIVATE
    "-sUSE_WEBGPU=1"
    "-sWASM=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sNO_EXIT_RUNTIME=0"
    "-sASSERTIONS=1"
    "-sDISABLE_EXCEPTION_CATCHING=1"
    "-sNO_FILESYSTEM=1"
    "-sUSE_SDL=2"
  )
  set_target_properties(example_sdl2_wgpu PROPERTIES OUTPUT_NAME "index")
  # copy our custom index.html to build directory
  add_custom_command(TARGET example_sdl2_wgpu POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/web/index.html" $<TARGET_FILE_DIR:example_sdl2_wgpu>
  )
endif()
