cmake_minimum_required(VERSION 3.10)
project(JKMenu)

# 设置 C++ 标准和 STL
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ANDROID_STL c++_static)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)

# ImGui 核心源文件
# 将所有 ImGui 源文件打入主库，防止报 liblibcimgui.so 找不到的错误
set(IMGUI_SRCS
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
)

# 1. 编译注入器 (jk_injector)
add_executable(jk_injector injector.cpp)
target_link_libraries(jk_injector log)

# 2. 编译插件库 (libJKMenu.so)
add_library(JKMenu SHARED main.cpp ${IMGUI_SRCS})

# 链接库文件
# 确保 libdobby.a 在项目根目录下
target_link_libraries(JKMenu
    ${CMAKE_CURRENT_SOURCE_DIR}/libdobby.a
    android 
    GLESv3 
    EGL 
    log
)

# 设置输出名称
set_target_properties(jk_injector PROPERTIES OUTPUT_NAME "jk_injector")
set_target_properties(JKMenu PROPERTIES OUTPUT_NAME "JKMenu")
