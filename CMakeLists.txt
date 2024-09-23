cmake_minimum_required(VERSION 3.20)
project(DearImGui LANGUAGES C CXX)

# TODO: Implement cmake toolchain files to handle platform specific configurations (apple/linux/windows)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)

if(NOT DEFINED ImGuiPlatform_Option)
    # Setting a highly compatible default 
    set(ImGuiPlatform_Option glfw)
endif()

if(NOT DEFINED ImguiRenderer_Option)
    # Setting a highly compatible default
    set(ImguiRenderer_Option opengl2)
endif()

add_subdirectory(ImGuiFramework)
add_subdirectory(ImGuiPlatforms)
add_subdirectory(ImGuiRenderers)
add_subdirectory(ImGuiDemo)