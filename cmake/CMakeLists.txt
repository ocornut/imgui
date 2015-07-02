cmake_minimum_required(VERSION 2.8.12)
project(imgui)

set(inst_src_dir include/imgui_src)

set(root ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(phdrs ${root}/imgui.h)
set(hdrs ${root}/stb_rect_pack.h ${root}/stb_textedit.h ${root}/stb_truetype.h)
set(srcs ${root}/imgui.cpp)
set(inst_srcs ${inst_src_dir}/imgui.cpp)

add_library(imgui INTERFACE)

if(CMAKE_VERSION VERSION_LESS 3.3)
    message(WARNING "The config-module created by your CMake version (< 3.3) "
        "can't attach the imgui source files to the exported library. Please see the "
        "documentation in `_IMGUI_WORKS_WITHOUT_CMAKE.md` for further instructions.")
else()
    target_sources(imgui INTERFACE
        $<BUILD_INTERFACE:${srcs}>
        $<INSTALL_INTERFACE:${inst_srcs}>)
endif()

set(IMGUI_SOURCES ${srcs})

install(FILES ${srcs} ${hdrs} DESTINATION ${inst_src_dir})

target_include_directories(imgui INTERFACE
    $<BUILD_INTERFACE:${root}>
    $<INSTALL_INTERFACE:include>)

target_compile_definitions(imgui INTERFACE IMGUI_CONDITIONAL_IMCONFIG_H)

install(TARGETS imgui EXPORT imgui-targets DESTINATION lib)

install(EXPORT imgui-targets
    DESTINATION lib/cmake/imgui)

install(FILES ${phdrs} DESTINATION include)
install(FILES ${root}/imconfig.h DESTINATION include RENAME imconfig-sample.h)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/hide/imgui-config.cmake "
    include(\${CMAKE_CURRENT_LIST_DIR}/imgui-targets.cmake)
    get_filename_component(IMGUI_SOURCES \"\${CMAKE_CURRENT_LIST_DIR}/../../../include/imgui_src/imgui.cpp\" ABSOLUTE)
    ")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/hide/imgui-config.cmake DESTINATION lib/cmake/imgui)
