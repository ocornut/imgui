cmake_minimum_required(VERSION 2.8.12)
project(imgui)

set(CMAKE_DEBUG_POSTFIX _d)

set(inst_src_dir include/imgui_src)

set(root ${CMAKE_CURRENT_SOURCE_DIR})
set(phdrs ${root}/imgui.h)
set(hdrs ${root}/stb_rect_pack.h ${root}/stb_textedit.h ${root}/stb_truetype.h)
set(srcs ${root}/imgui.cpp)
set(inst_srcs ${inst_src_dir}/imgui.cpp)

if(CMAKE_VERSION VERSION_LESS 3.3)
    message(WARNING "You need CMake 3.3 or later to export and use "
        "`imgui` as a set of source files and make use of the `imconfig.h` feature. "
        "Currently `imgui` will be exported as a static library.")
    add_library(imgui STATIC ${srcs} ${hdrs} ${phdrs})
    set(mode PUBLIC)
else()
    add_library(imgui INTERFACE)
    target_sources(imgui INTERFACE
        $<BUILD_INTERFACE:${srcs}>
        $<INSTALL_INTERFACE:${inst_srcs}>)
    set(mode INTERFACE)
    install(FILES ${srcs} ${hdrs} DESTINATION ${inst_src_dir})
endif()

target_include_directories(imgui ${mode}
    $<BUILD_INTERFACE:${root}>
    $<INSTALL_INTERFACE:include>)

target_compile_definitions(imgui ${mode} IMGUI_CONDITIONAL_IMCONFIG_H)

install(TARGETS imgui EXPORT imgui-targets DESTINATION lib)

install(EXPORT imgui-targets FILE imgui-config.cmake
    DESTINATION lib/cmake/imgui)

install(FILES ${phdrs} DESTINATION include)
install(FILES ${root}/imconfig.h DESTINATION include RENAME imconfig-sample.h)
