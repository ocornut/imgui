cmake_minimum_required(VERSION 2.8.12)
project(imgui)

set(CMAKE_DEBUG_POSTFIX _d)

add_library(imgui
    STATIC # remove this after implementing export macros (IMGUI_API)
    imgui.cpp
    imconfig.h
    imgui.h
    stb_rect_pack.h
    stb_textedit.h
    stb_truetype.h
    )

target_include_directories(imgui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:include>)

install(TARGETS imgui EXPORT imgui-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

install(EXPORT imgui-targets
    FILE imgui-config.cmake
    DESTINATION lib/cmake/imgui)

install(FILES imgui.h imconfig.h DESTINATION include)

