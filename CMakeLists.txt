cmake_minimum_required(VERSION 3.10)
project(JKMenu)

# 1. 强制使用静态 STL，减少对系统库的依赖
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ANDROID_STL c++_static)

# 2. 包含目录（确保 backends 路径正确）
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include) # 如果 Dobby 的头文件在 include 里

# 3. ImGui 源文件定义
set(IMGUI_SRCS
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
)

# --- 重点：编译插件库 libJKMenu.so ---
# 增加渲染相关的源文件，确保 OpenGL 绑定完整
add_library(JKMenu SHARED 
    main.cpp 
    ${IMGUI_SRCS}
)

# 4. 解决符号隐藏问题 (非常重要！)
# 很多游戏会扫描导出的符号，隐藏非必要符号可以提高隐蔽性并防止冲突
set_target_properties(JKMenu PROPERTIES 
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# 5. 链接库文件
# 注意：Dobby 最好使用全路径，且放在最后链接
target_link_libraries(JKMenu
    android
    log
    EGL
    GLESv3
    -Wl,--whole-archive ${CMAKE_CURRENT_SOURCE_DIR}/libdobby.a -Wl,--no-whole-archive
    -static-libstdc++ 
)

# --- 编译注入器 ---
add_executable(jk_injector injector.cpp)
target_link_libraries(jk_injector log)

# 6. 设置输出名称
set_target_properties(JKMenu PROPERTIES OUTPUT_NAME "JKMenu")
