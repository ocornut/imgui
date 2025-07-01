cmake_minimum_required(VERSION 3.25)
project(example_sdl2_vulkan)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(Vulkan REQUIRED)

set(IMGUI_DIR ../../)

add_executable(example_sdl2_vulkan
        main.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp)

target_compile_definitions(example_sdl2_vulkan
        PRIVATE ImTextureID=ImU64)

target_include_directories(example_sdl2_vulkan
        PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)

target_link_libraries(example_sdl2_vulkan
        PRIVATE SDL2::SDL2 Vulkan::Vulkan)
