cmake_minimum_required(VERSION 3.23)
project(example_win32_vulkan)

# find_package(imgui REQUIRED)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui imgui::backend_win32_vulkan)
