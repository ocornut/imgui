cmake_minimum_required(VERSION 3.10)
project(JKMenu)

# 1. 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)

# 2. 包含头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)

# 3. 定义 ImGui 插件源文件
set(IMGUI_SRCS
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
)

# 4. 目标一：生成插件 libJKMenu.so
# 确保你已经上传了之前给你的 main.cpp
add_library(JKMenu SHARED
    main.cpp
    ${IMGUI_SRCS}
)

target_link_libraries(JKMenu
    ${CMAKE_CURRENT_SOURCE_DIR}/libdobby.a
    android
    GLESv3
    EGL
    log
)

# 5. 目标二：生成 ELF 注入器 jk_injector
# 使用你刚才提供的最新 injector.cpp
add_executable(jk_injector injector.cpp)

# 注入器需要链接 log 库以便查看调试信息
target_link_libraries(jk_injector log)

# 6. 设置输出属性
set_target_properties(JKMenu PROPERTIES OUTPUT_NAME "JKMenu")
set_target_properties(jk_injector PROPERTIES OUTPUT_NAME "jk_injector")
