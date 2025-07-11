cmake_minimum_required(VERSION 3.23)
project(example_win_directx12)

# find_package(imgui REQUIRED)

if (HAS_IMGUI_DIRECTX_12_X86)
    add_executable(example_win32_directx12)

    target_sources(example_win32_directx12 PRIVATE main.cpp)

    target_link_libraries(example_win32_directx12 PRIVATE imgui::imgui imgui::backend_win32_directx12)
endif ()

if (HAS_IMGUI_DIRECTX_12_X64)
    add_executable(example_win64_directx12)

    target_sources(example_win64_directx12 PRIVATE main.cpp)

    target_link_libraries(example_win64_directx12 PRIVATE imgui::imgui imgui::backend_win64_directx12)
endif ()
