cmake_minimum_required(VERSION 3.5)

project(example_glfw_opengl3
    LANGUAGES C CXX)

find_package(ImGui CONFIG REQUIRED
    COMPONENTS ImplGlfw ImplOpenGL3)

add_executable(${PROJECT_NAME} WIN32 main.cpp)

if(EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX .html)
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE ImGui::ImplGlfw ImGui::ImplOpenGL3)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
