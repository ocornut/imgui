cmake_minimum_required(VERSION 3.10)

# 项目名称
project("MyModMenu")

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件目录 (假设你的头文件在 include 和 imgui 目录下)
include_directories(
    include
    imgui
    imgui/backends
)

# 查找库文件 (Dobby 静态库)
# 注意：在 GitHub Action 中，我们会把库放在指定的路径
link_directories(${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI})

# 定义源文件
set(SOURCE_FILES
    main.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_android.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

# 生成共享库 (.so)
add_library(MyMenu SHARED ${SOURCE_FILES})

# 链接依赖项
# dobby 为静态库，log, GLESv3, EGL, android 为系统库
target_link_libraries(MyMenu
    dobby
    log
    GLESv3
    EGL
    android
)
