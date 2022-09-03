cmake_minimum_required(VERSION 3.2)
project(imgui VERSION 0.1.0)

set(ImGuiSrc
    ${CMAKE_CURRENT_SOURCE_DIR}/imconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imstb_rectpack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imstb_textedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imstb_truetype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp
)

set(ImGuiImpl  
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_glfw.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_glfw.h
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl3.h
)

# build as a static library
add_library(imgui STATIC ${ImGuiSrc} ${ImGuiImpl})

# compile options
target_compile_options(imgui PRIVATE -w)

# include libraries
target_include_directories(imgui PUBLIC ${PROJECT_SOURCE_DIR})