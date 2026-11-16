cmake_minimum_required(VERSION 3.5)

project(example_sdl3_sdlrenderer3
    LANGUAGES C CXX)

find_package(ImGui CONFIG REQUIRED
    COMPONENTS ImplSDL3 ImplSDLRenderer3)

add_executable(${PROJECT_NAME} WIN32 main.cpp)

if(EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX .html)
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE ImGui::ImplSDL3 ImGui::ImplSDLRenderer3)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
