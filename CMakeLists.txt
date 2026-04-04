cmake_minimum_required(VERSION 3.10)
project(JKMenu)

# 1. 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)

# 2. 包含头文件路径 (包含当前根目录，以便找到 dobby.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)

# 3. 定义源文件
set(IMGUI_SRCS
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
)

# 4. 生成共享库 libJKMenu.so
# 假设你的主逻辑文件名为 main.cpp (如果不是，请修改)
add_library(JKMenu SHARED
    main.cpp
    ${IMGUI_SRCS}
)

# 5. 链接 Dobby 静态库
# ${CMAKE_CURRENT_SOURCE_DIR}/libdobby.a 指向你刚上传的文件
target_link_libraries(JKMenu
    ${CMAKE_CURRENT_SOURCE_DIR}/libdobby.a
    android
    GLESv3
    EGL
    log
)

# 6. (可选) 移除调试符号，减小体积并增加逆向难度
set_target_properties(JKMenu PROPERTIES 
    OUTPUT_NAME "JKMenu"
    VERSION 1.0
)
