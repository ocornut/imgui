cmake_minimum_required(VERSION 3.0)
project(imgui_example_glfw_metal C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# GLFW
set(GLFW_DIR ../../../glfw) # Set this to point to an up-to-date GLFW repo
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
include_directories(${GLFW_DIR}/include)

# ImGui
set(IMGUI_DIR "../..")

# Libraries
set(LIBRARIES "glfw")

# Includes
set(IMGUI_INCLUDE_DIR ${IMGUI_DIR} ../.)

add_executable(example_glfw_metal
  main.mm
  ${IMGUI_DIR}/examples/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/examples/imgui_impl_metal.mm
  ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
target_compile_options(example_glfw_metal PRIVATE "-fobjc-arc")
target_include_directories(example_glfw_metal PRIVATE ${IMGUI_INCLUDE_DIR})
target_link_libraries(example_glfw_metal PRIVATE ${LIBRARIES})
target_link_libraries(example_glfw_metal PRIVATE "-framework Metal" "-framework QuartzCore")
