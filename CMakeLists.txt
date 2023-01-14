cmake_minimum_required(VERSION 3.23)

project(imgui)

add_library(imgui STATIC "imconfig.h"
        imconfig.h
        imgui.cpp
        imgui.h
        imgui_demo.cpp
        imgui_draw.cpp
        imgui_internal.h
        imgui_tables.cpp
        imgui_widgets.cpp
        imstb_rectpack.h
        imstb_textedit.h
        imstb_truetype.h
        backends/imgui_impl_glfw.cpp
        backends/imgui_impl_glfw.h
        backends/imgui_impl_opengl3.cpp
        backends/imgui_impl_opengl3.h
        backends/imgui_impl_opengl3_loader.h
        )


target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC backends)

target_link_libraries(imgui PRIVATE D:/dev/Engine/external/imgui/examples/libs/glfw/lib-vc2010-64/glfw3.lib)
target_include_directories(imgui PRIVATE D:/dev/Engine/external/imgui/examples/libs/glfw/include)