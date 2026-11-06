cmake_minimum_required(VERSION 3.5)

project(example_sdl2_vulkan
    LANGUAGES CXX)

find_package(ImGui CONFIG REQUIRED
    COMPONENTS ImplSDL2 ImplVulkan)

add_executable(${PROJECT_NAME} WIN32 main.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE ImGui::ImplSDL2 ImGui::ImplVulkan)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
