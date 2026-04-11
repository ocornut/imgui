# Example usage:
#  mkdir build
#  cd build
#  cmake -g "Visual Studio 14 2015" ..

cmake_minimum_required(VERSION 3.10)
project(imgui_example_glfw_vulkan C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 11)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

# GLFW
find_package(glfw3 REQUIRED)

# Dear ImGui
set(IMGUI_DIR ../../)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

# ImGui Test Engine
set(TE_DIR /home/joao/vscode/imgui_test_engine/imgui_test_engine)
include_directories(${TE_DIR})

# Libraries
find_package(Vulkan REQUIRED)
find_package(Freetype REQUIRED)
find_package(plutosvg REQUIRED PATHS /usr/local/lib64/cmake/plutosvg)
set(LIBRARIES "glfw;Vulkan::Vulkan;Freetype::Freetype;plutosvg::plutosvg")

file(GLOB sources *.cpp)

add_executable(example_glfw_vulkan
    ${sources}
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
    ${TE_DIR}/imgui_te_context.cpp
    ${TE_DIR}/imgui_te_coroutine.cpp
    ${TE_DIR}/imgui_te_engine.cpp
    ${TE_DIR}/imgui_te_exporters.cpp
    ${TE_DIR}/imgui_te_perftool.cpp
    ${TE_DIR}/imgui_te_ui.cpp
    ${TE_DIR}/imgui_te_utils.cpp
    ${TE_DIR}/imgui_capture_tool.cpp
)
target_link_libraries(example_glfw_vulkan ${LIBRARIES})

