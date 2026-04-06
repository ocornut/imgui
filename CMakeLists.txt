cmake_minimum_required(VERSION 3.10)

# 项目名称
project(AndKittyModProject)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. 设置路径变量 ---
set(SOURCE_DIR ${CMAKE_SOURCE_DIR})

# --- 2. 包含头文件路径 ---
# 因为 imgui 文件在根目录，直接包含根目录和 backends 目录
include_directories(
    ${SOURCE_DIR}
    ${SOURCE_DIR}/include
    ${SOURCE_DIR}/backends
)

# --- 3. 引用 Dobby 静态库 (位于根目录) ---
add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES IMPORTED_LOCATION 
    ${SOURCE_DIR}/libdobby.a)

# --- 4. 构建主菜单插件 (libMyMenu.so) ---
# 根据你的实际结构，imgui 核心文件在根目录，后端在 backends/
set(PLUGIN_SOURCES
    main.cpp
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
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
# 确保项目中有 injector.cpp
if(EXISTS "${SOURCE_DIR}/injector.cpp")
    add_executable(injector injector.cpp)
    
    # 注入器需要链接 log (用于调试) 和 dl (用于 dlopen)
    target_link_libraries(injector
        log
        dl
    )
    
    # 设置注入器为静态链接 C++ 运行库，确保在不同手机上都能直接运行
    set_target_properties(injector PROPERTIES LINK_FLAGS "-static-libstdc++")
endif()

# 打印构建调试信息
message(STATUS "Android ABI: ${ANDROID_ABI}")
message(STATUS "Dobby Path: ${SOURCE_DIR}/libdobby.a")
message(STATUS "Main Source: ${SOURCE_DIR}/main.cpp")
