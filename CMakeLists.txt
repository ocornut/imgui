cmake_minimum_required(VERSION 3.15)

project(imgui)

option(IMGUI_IMPL_WIN32 "Enable ImGui Win32 implementation" ON)
option(IMGUI_IMPL_OPENGL2 "Enable ImGui OpenGL2 implementation" OFF)
option(IMGUI_IMPL_OPENGL3 "Enable ImGui OpenGL3 implementation" OFF)
option(IMGUI_ENABLE_PLAYGROUND "Enable playground subdirectory" OFF)

set(IMGUI_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_library(imgui-core STATIC)
target_sources(imgui-core PUBLIC imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp)
target_include_directories(imgui-core PUBLIC ${IMGUI_INC_DIR})

add_subdirectory(misc)
add_subdirectory(backends)

# Playground is a private space where dev might test/experiment/freestyle stuffs
if(IMGUI_ENABLE_PLAYGROUND)
if(EXISTS playground)
add_subdirectory(playground)
endif()
endif()
