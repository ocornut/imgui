cmake_minimum_required(VERSION 3.10)
project(JKMenu)

set(ANDROID_STL c++_static)
set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)

set(IMGUI_SRCS
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
    backends/imgui_impl_vulkan.cpp
)

# 注入器
add_executable(jk_injector injector.cpp)
target_link_libraries(jk_injector log)

# 插件库
add_library(JKMenu SHARED main.cpp ${IMGUI_SRCS})
target_link_libraries(JKMenu
    ${CMAKE_CURRENT_SOURCE_DIR}/libdobby.a
    android GLESv3 EGL vulkan log
)

set_target_properties(jk_injector PROPERTIES OUTPUT_NAME "jk_injector")
set_target_properties(JKMenu PROPERTIES OUTPUT_NAME "libJKMenu")
