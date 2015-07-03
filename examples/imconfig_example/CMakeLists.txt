cmake_minimum_required(VERSION 2.8.12)

project(imconfig_example)

find_package(OpenGL REQUIRED)
find_package(imgui REQUIRED)

find_package(glfw3 REQUIRED CONFIG)

set(srcs imgui_impl_glfw.cpp imgui_impl_glfw.h main.cpp imconfig.h)

if(CMAKE_VERSION VERSION_LESS 3.3)
    list(APPEND srcs ${IMGUI_SOURCES})
endif()

add_executable(imconfig_example ${srcs})

target_compile_definitions(imconfig_example PRIVATE IMGUI_INCLUDE_IMCONFIG_H)

target_include_directories(imconfig_example PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR} # needed for imgui.h to find our imconfig.h
    )

target_link_libraries(imconfig_example
    ${OPENGL_LIBRARIES}
    glfw
    imgui)

install(TARGETS imconfig_example DESTINATION bin)
set_target_properties(imconfig_example PROPERTIES DEBUG_POSTFIX _d)
