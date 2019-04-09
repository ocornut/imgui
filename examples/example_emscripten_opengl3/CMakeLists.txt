cmake_minimum_required(VERSION 3.1)

project(example_emscripten_opengl3
    LANGUAGES C CXX)

find_package(ImGui CONFIG REQUIRED
    COMPONENTS ImplSDL2 ImplOpenGL3)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE ImGui::ImplSDL2 ImGui::ImplOpenGL3)

target_link_options(${PROJECT_NAME} PRIVATE
    "SHELL:--shell-file ${CMAKE_CURRENT_LIST_DIR}/shell_minimal.html")

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX .html)
