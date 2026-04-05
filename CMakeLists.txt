cmake_minimum_required(VERSION 3.10)
project(JKMenu)

# 强制静态链接 C++ 运行时，解决 libc++_shared.so 缺失问题
set(ANDROID_STL c++_static)
set(CMAKE_CXX_STANDARD 17)

# 包含路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)

# ImGui 源文件列表
set(IMGUI_SRCS
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
    backends/imgui_impl_vulkan.cpp
)

# 目标一：生成注入器 (优先编译，确保它先生成)
add_executable(jk_injector injector.cpp)
target_link_libraries(jk_injector log)

# 目标二：生成插件 libJKMenu.so
add_library(JKMenu SHARED
    main.cpp
    ${IMGUI_SRCS}
)

# 链接必要的库
target_link_libraries(JKMenu
    ${CMAKE_CURRENT_SOURCE_DIR}/libdobby.a
    android
    GLESv3
    EGL
    vulkan
    log
)

# 统一输出名称
set_target_properties(jk_injector PROPERTIES OUTPUT_NAME "jk_injector")
set_target_properties(JKMenu PROPERTIES OUTPUT_NAME "JKMenu")
