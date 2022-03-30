cmake_minimum_required(VERSION 3.0.0)
project(imgui VERSION 0.1.0)

include(CTest)
enable_testing()

include_directories(${CMAKE_SOURCE_DIR}/.. )
include_directories(${CMAKE_SOURCE_DIR}/../backends)
aux_source_directory(../ SRC_FILES)

add_executable(example_win32_directx9 ${SRC_FILES}
    example_win32_directx9/main.cpp
    ../backends/imgui_impl_dx9.cpp 
    ../backends/imgui_impl_win32.cpp
)
target_link_libraries(example_win32_directx9 d3d9 Dwmapi)

add_executable(example_win32_directx10 ${SRC_FILES}
    example_win32_directx10/main.cpp
    ../backends/imgui_impl_dx10.cpp 
    ../backends/imgui_impl_win32.cpp
)
target_link_libraries(example_win32_directx10 d3d10 d3dcompiler Dwmapi)

add_executable(example_win32_directx11 ${SRC_FILES}
    example_win32_directx11/main.cpp
    ../backends/imgui_impl_dx11.cpp 
    ../backends/imgui_impl_win32.cpp
)
target_link_libraries(example_win32_directx11 d3d11 d3dcompiler Dwmapi)

add_executable(example_win32_directx12 ${SRC_FILES}
    example_win32_directx12/main.cpp
    ../backends/imgui_impl_dx12.cpp 
    ../backends/imgui_impl_win32.cpp
)
target_link_libraries(example_win32_directx12 d3d12 d3dcompiler Dwmapi DXGI)

add_executable(example_sdl_vulkan ${SRC_FILES}
    example_sdl_vulkan/main.cpp
    ../backends/imgui_impl_sdl.cpp 
    ../backends/imgui_impl_vulkan.cpp
)
target_link_libraries(example_sdl_vulkan SDL2 SDL2main shell32 vulkan)

add_executable(example_sdl_directx11 ${SRC_FILES}
    example_sdl_directx11/main.cpp
    ../backends/imgui_impl_sdl.cpp 
    ../backends/imgui_impl_dx11.cpp
)
target_link_libraries(example_sdl_directx11 SDL2 SDL2main d3d11 d3dcompiler shell32)




set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
