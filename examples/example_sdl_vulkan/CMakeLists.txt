# --- CMake Version ---
cmake_minimum_required(VERSION 3.21)

# --- vcpkg ---
SET(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")

# --- Project ---
project(
    ImGui_Demo_SDL_Vulkan
    DESCRIPTION "ImGui Demo of SDL+Vulkan"
    LANGUAGES CXX)

# --- Dependency ---
find_package(imgui CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

# --- Taget ---
add_executable(ImGui_Demo_SDL_Vulkan)

# --- Include Directory ---
target_include_directories(
    ImGui_Demo_SDL_Vulkan
    PRIVATE
    ${CMAKE_SOURCE_DIR}/../../
    ${CMAKE_SOURCE_DIR}/../../backends
)

# --- Source Directory ---
target_sources(
    ImGui_Demo_SDL_Vulkan
    PRIVATE
    ${PROJECT_SOURCE_DIR}/main.cpp
    ${PROJECT_SOURCE_DIR}/../../imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/../../imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/../../imgui_tables.cpp
    ${PROJECT_SOURCE_DIR}/../../imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/../../imgui.cpp
    ${PROJECT_SOURCE_DIR}/../../backends/imgui_impl_sdl.cpp
    ${PROJECT_SOURCE_DIR}/../../backends/imgui_impl_vulkan.cpp
)

# --- Linakge ---
target_link_libraries(
    ImGui_Demo_SDL_Vulkan
    PRIVATE
    imgui::imgui
    SDL2::SDL2
    SDL2::SDL2main
    Vulkan::Vulkan
)
