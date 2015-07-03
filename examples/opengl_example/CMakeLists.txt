cmake_minimum_required(VERSION 2.8.12)

project(imgui_opengl_example)

find_package(OpenGL REQUIRED)
find_package(imgui REQUIRED)

find_package(glfw3 REQUIRED CONFIG)

set(srcs imgui_impl_glfw.cpp imgui_impl_glfw.h main.cpp)

if(CMAKE_VERSION VERSION_LESS 3.3)
    list(APPEND srcs ${IMGUI_SOURCES})
endif()

add_executable(imgui_opengl_example ${srcs})

target_include_directories(imgui_opengl_example PRIVATE
    ${OPENGL_INCLUDE_DIR})

target_link_libraries(imgui_opengl_example
    ${OPENGL_LIBRARIES}
    glfw
    imgui)

install(TARGETS imgui_opengl_example DESTINATION bin)
set_target_properties(imgui_opengl_example PROPERTIES DEBUG_POSTFIX _d)
