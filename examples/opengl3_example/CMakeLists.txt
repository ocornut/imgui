cmake_minimum_required(VERSION 2.8.12)

project(imgui_opengl3_example)

find_package(OpenGL REQUIRED)
find_package(imgui REQUIRED)

find_package(glfw3 REQUIRED CONFIG)

add_subdirectory(../libs/gl3w gl3w)

set(srcs imgui_impl_glfw_gl3.cpp imgui_impl_glfw_gl3.h main.cpp)

if(CMAKE_VERSION VERSION_LESS 3.3)
    list(APPEND srcs ${IMGUI_SOURCES})
endif()

add_executable(imgui_opengl3_example ${srcs})

target_include_directories(imgui_opengl3_example PRIVATE
    ${OPENGL_INCLUDE_DIR})

target_link_libraries(imgui_opengl3_example
    ${OPENGL_LIBRARIES}
    gl3w
    glfw
    imgui
    )

install(TARGETS imgui_opengl3_example DESTINATION bin)
set_target_properties(imgui_opengl3_example PROPERTIES DEBUG_POSTFIX _d)
