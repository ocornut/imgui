cmake_minimum_required(VERSION 3.15)

project(imgui)

set(IMGUI_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_library(imgui-core STATIC)
target_sources(imgui-core PUBLIC imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp)
target_include_directories(imgui-core PUBLIC ${IMGUI_INC_DIR})

add_subdirectory(misc)

# Playground is a private space where dev might test/experiment/freestyle stuffs
if(EXISTS playground)
add_subdirectory(playground)
endif()
