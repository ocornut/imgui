cmake_minimum_required(VERSION 3.23)
project(example_sdl3_sdlgpu3)

# find_package(imgui REQUIRED)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui imgui::backend_sdl3_sdlgpu3)
