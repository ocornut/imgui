cmake_minimum_required(VERSION 3.5)

project(example_glfw_vulkan
    LANGUAGES CXX)

find_package(ImGui CONFIG REQUIRED
    COMPONENTS ImplGlfw ImplVulkan)

add_executable(${PROJECT_NAME} WIN32 main.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE ImGui::ImplGlfw ImGui::ImplVulkan)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
