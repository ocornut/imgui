cmake_minimum_required(VERSION 3.23)
project(ImGui VERSION 1.92.0)

option(BUILD_EXAMPLES "Build examples" OFF)
option(WITH_EXTRA_WARNINGS "Pass extra warnings to targets" OFF)
option(EMSCRIPTEN_USE_FILE_SYSTEM "Include the misc/fonts/ folder as part of the build" OFF)

if (WITH_EXTRA_WARNINGS)
    if (MSVC)
        add_compile_options(/W4 /permissive-)
    else ()    
        add_compile_options(-Wno-zero-as-null-pointer-constant -Wno-double-promotion -Wno-variadic-macros)
        if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
            add_compile_options(-Wextra -Wpedantic)
            if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                add_compile_options(-Wshadow -Wsign-conversion)
            endif ()
        elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            add_compile_options(-Weverything -Wno-reserved-id-macro -Wno-c++98-compat-pedantic -Wno-padded -Wno-poison-system-directories)
        elseif (MINGW)
            add_compile_options(-Wextra -Wpedantic)
        endif ()
    endif ()
endif ()

add_library(imgui STATIC)
add_library(imgui::imgui ALIAS imgui)

target_sources(imgui
PUBLIC
    FILE_SET HEADERS
    FILES
        imconfig.h
        imgui.h
        imgui_internal.h
        imstb_rectpack.h
        imstb_textedit.h
        imstb_truetype.h
PRIVATE
        imgui.cpp
        imgui_demo.cpp
        imgui_draw.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
)

if (WITH_FREETYPE)
    target_sources(imgui
        PRIVATE
            misc/freetype/imgui_freetype.cpp
    )

    if (EMSCRIPTEN)
        target_compile_options(imgui PUBLIC "-sUSE_FREETYPE=1")
        target_link_options(imgui PUBLIC "-sUSE_FREETYPE=1")
    else ()
        find_package(freetype REQUIRED CONFIG)
        target_link_libraries(imgui PRIVATE Freetype::Freetype)
    endif ()
endif ()

target_compile_features(imgui PRIVATE cxx_std_11)
if (MSVC)
    target_compile_options(imgui PRIVATE /W3)
else ()
    target_compile_options(imgui PRIVATE -Wall -Wformat)
endif ()

target_include_directories(imgui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS imgui
        EXPORT imguiTargets
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include
)

add_subdirectory(backends)

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

install(EXPORT imguiTargets
        NAMESPACE imgui::
        DESTINATION share/cmake/imgui
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfigVersion.cmake"
    DESTINATION share/cmake/imgui
)

configure_file(cmake/imguiConfig.cmake.in imguiConfig.cmake @ONLY)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfig.cmake"
    DESTINATION share/cmake/imgui
)
