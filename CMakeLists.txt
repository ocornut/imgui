cmake_minimum_required(VERSION 3.10)
project(JKMenu)

# 1. 设置 C++ 标准 (Vulkan 后端通常需要 C++11 或更高，17 没问题)
set(CMAKE_CXX_STANDARD 17)

# 2. 包含头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)

# 3. 定义 ImGui 插件源文件 (新增了 vulkan 后端)
set(IMGUI_SRCS
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
    backends/imgui_impl_vulkan.cpp # <--- 新增这一行
)

# 4. 目标一：生成插件 libJKMenu.so
add_library(JKMenu SHARED
    main.cpp
    ${IMGUI_SRCS}
)

# 链接库：增加 vulkan 库链接
target_link_libraries(JKMenu
    ${CMAKE_CURRENT_SOURCE_DIR}/libdobby.a
    android
    GLESv3
    EGL
    vulkan  # <--- 新增链接 vulkan 库
    log
)

# 5. 目标二：生成 ELF 注入器 jk_injector
add_executable(jk_injector injector.cpp)
target_link_libraries(jk_injector log)

# 6. 设置输出属性 (保持原样)
set_target_properties(JKMenu PROPERTIES OUTPUT_NAME "JKMenu")
set_target_properties(jk_injector PROPERTIES OUTPUT_NAME "jk_injector")
