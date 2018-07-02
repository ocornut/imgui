cmake_minimum_required(VERSION 3.3)

project(sdl_opengl2_example)

find_package(ImGui REQUIRED CONFIG)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} ImGui::ImplSDL2 ImGui::ImplOpenGL2 ImGui::Demo)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME imgui_${PROJECT_NAME})

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
