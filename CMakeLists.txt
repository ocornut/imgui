cmake_minimum_required(VERSION 3.10)

# 项目名称
project(AndKittyModProject)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. 设置路径变量 ---
set(SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(IMGUI_DIR ${SOURCE_DIR}/imgui)

# --- 2. 包含头文件路径 ---
include_directories(
    ${SOURCE_DIR}
    ${SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# --- 3. 引用 Dobby 静态库 (现在指向根目录) ---
add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES IMPORTED_LOCATION 
    ${SOURCE_DIR}/libdobby.a)

# --- 4. 构建主菜单插件 (libMyMenu.so) ---
# 注意：确保你的源文件路径与实际目录结构一致
set(PLUGIN_SOURCES
    main.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_android.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(MyMenu SHARED ${PLUGIN_SOURCES})

# 链接插件所需的系统库
target_link_libraries(MyMenu
    dobby
    log
    GLESv3
    EGL
    android
)

# --- 5. 构建独立注入器 (injector) ---
add_executable(injector injector.cpp)

# 注入器需要链接 log 和 dl 库
target_link_libraries(injector
    log
    dl
)

# 为注入器设置静态链接 C++ 运行库，确保兼容性
set_target_properties(injector PROPERTIES LINK_FLAGS "-static-libstdc++")

# 打印构建信息
message(STATUS "Android ABI: ${ANDROID_ABI}")
message(STATUS "Dobby path: ${SOURCE_DIR}/libdobby.a")
