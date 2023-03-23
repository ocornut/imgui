cmake_minimum_required(VERSION 3.17)
project(ExampleGlfwBgfx)

include(FetchContent)

FetchContent_Declare(
  bgfx
  GIT_REPOSITORY "https://github.com/bkaradzic/bgfx.cmake.git"
  GIT_TAG        "v1.118.8417-420"
)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY "https://github.com/glfw/glfw.git"
  GIT_TAG        "3.3.8"
)

FetchContent_MakeAvailable(bgfx glfw)

add_library(imgui
  "${CMAKE_CURRENT_SOURCE_DIR}/../../imgui.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../imgui_demo.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../imgui_draw.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../imgui_tables.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../imgui_widgets.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../backends/imgui_impl_glfw.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../backends/imgui_impl_bgfx.cpp"
)
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../")
target_link_libraries(imgui PUBLIC bgfx bx bimg glfw)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} bgfx bx bimg glfw imgui)