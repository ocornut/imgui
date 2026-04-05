cmake_minimum_required(VERSION 3.10)

# 项目名称
project("AndKittyImGuiMod")

# C++ 17 标准支持
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 包含路径
# 由于源码都在根目录，直接包含根目录和 backends 子目录
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/backends
)

# 2. 库查找路径
# 确保 libdobby.a 放在根目录，或者在 build.yml 中指定的路径
link_directories(${CMAKE_SOURCE_DIR})

# 3. 定义源文件
# 包含 ImGui 核心、backends 适配层以及你的入口文件 main.cpp
set(SOURCE_FILES
    main.cpp
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
)

# 4. 生成共享库 (.so)
add_library(MyMenu SHARED ${SOURCE_FILES})

# 5. 链接依赖库
# dobby: 静态库 (libdobby.a)
# log, GLESv3, EGL, android: Android 系统内置库
target_link_libraries(MyMenu
    dobby
    log
    GLESv3
    EGL
    android
)
