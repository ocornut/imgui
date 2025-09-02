cmake_minimum_required(VERSION 3.23)
project(example_win_directx9)

# find_package(imgui REQUIRED)

if (HAS_IMGUI_DIRECTX_9_X86)
    add_executable(example_win32_directx9)

    target_sources(example_win32_directx9 PRIVATE main.cpp)

    target_link_libraries(example_win32_directx9 PRIVATE imgui::imgui imgui::backend_win32_directx9)
endif ()

if (HAS_IMGUI_DIRECTX_9_X64)
    add_executable(example_win64_directx9)

    target_sources(example_win64_directx9 PRIVATE main.cpp)

    target_link_libraries(example_win64_directx9 PRIVATE imgui::imgui imgui::backend_win64_directx9)
endif ()
