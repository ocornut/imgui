cmake_minimum_required(VERSION 3.1)

project(example_null
    LANGUAGES CXX)

find_package(ImGui CONFIG REQUIRED
    COMPONENTS Core)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE ImGui::Core)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
