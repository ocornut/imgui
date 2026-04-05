cmake_minimum_required(VERSION 3.10)
project(JKMenu)

# 1. 强制使用静态 C++ 运行时，解决 "library libc++_shared.so not found" 报错
set(ANDROID_STL c++_static)
set(CMAKE_CXX_STANDARD 17)

# 2. 包含路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)

# 3. ImGui 源文件 (包含 Vulkan 和 OpenGL3 后端)
set(IMGUI_SRCS
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
    backends/imgui_impl_vulkan.cpp
)

# 4. 生成插件 libJKMenu.so
add_library(JKMenu SHARED
    main.cpp
    ${IMGUI_SRCS}
)

# 链接 Dobby 和 系统库
target_link_libraries(JKMenu
    ${CMAKE_CURRENT_SOURCE_DIR}/libdobby.a
    android
    GLESv3
    EGL
    vulkan
    log
)

# 5. 生成注入器 jk_injector
add_executable(jk_injector injector.cpp)
target_link_libraries(jk_injector log)

# 6. 属性设置
set_target_properties(JKMenu PROPERTIES OUTPUT_NAME "JKMenu")
set_target_properties(jk_injector PROPERTIES OUTPUT_NAME "jk_injector")
