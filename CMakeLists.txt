cmake_minimum_required(VERSION 3.15)

project(imgui)

option(IMGUI_IMPL_WIN32 "Enable ImGui Win32 implementation" ON)
option(IMGUI_IMPL_OPENGL2 "Enable ImGui OpenGL2 implementation" OFF)
option(IMGUI_IMPL_OPENGL3 "Enable ImGui OpenGL3 implementation" OFF)
option(IMGUI_IMPL_DX9 "Enable ImGui DirectX9 implementation" OFF)
option(IMGUI_IMPL_DX10 "Enable ImGui DirectX10 implementation" OFF)
option(IMGUI_IMPL_DX11 "Enable ImGui DirectX11 implementation" OFF)
option(IMGUI_IMPL_DX12 "Enable ImGui DirectX12 implementation" OFF)
option(IMGUI_ENABLE_PLAYGROUND "Enable playground subdirectory" OFF)

set(IMGUI_INC_DIR
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
$<INSTALL_INTERFACE:include>)

include(cmake/add_library_ns.cmake)
include(cmake/target_include_dir_iface.cmake)
include(cmake/install_target.cmake)

add_library_ns(imgui core STATIC imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp)
target_include_directories(imgui-core PUBLIC ${IMGUI_INC_DIR})

add_subdirectory(misc)
add_subdirectory(backends)

# Playground is a private space where dev might test/experiment/freestyle stuffs
if(IMGUI_ENABLE_PLAYGROUND)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/playground)
add_subdirectory(playground)
endif()
endif()

install_target_and_headers(imgui core
    imgui.h
    imgui_internal.h
    imconfig.h
    imstb_truetype.h
    imstb_textedit.h
    imstb_rectpack.h
    )

configure_file(cmake/imgui-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-config.cmake
    @ONLY)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-config.cmake
    DESTINATION lib/cmake/imgui)

