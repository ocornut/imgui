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

# --- 3. 引用 Dobby 静态库 ---
# 假设你的 Dobby 库放在 libs/arm64-v8a/libdobby.a
add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES IMPORTED_LOCATION 
    ${SOURCE_DIR}/libs/${ANDROID_ABI}/libdobby.a)

# --- 4. 构建主菜单插件 (libMyMenu.so) ---
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
# 该可执行文件用于 Ptrace 注入，不依赖上面的插件库
add_executable(injector injector.cpp)

# 注入器需要链接 log (用于输出调试信息) 和 dl (用于 dlopen 相关操作)
target_link_libraries(injector
    log
    dl
)

# --- 6. 编译优化与兼容性设置 ---
# 为插件设置符号隐藏，减小体积
set_target_properties(MyMenu PROPERTIES CXX_VISIBILITY_PRESET hidden)

# 为注入器设置静态链接 C++ 运行库
# 这样即使在精简版安卓系统上运行也不会报错找不到 libc++_shared.so
set_target_properties(injector PROPERTIES LINK_FLAGS "-static-libstdc++")

# 打印构建信息以便调试
message(STATUS "Android ABI: ${ANDROID_ABI}")
message(STATUS "NDK Path: ${ANDROID_NDK}")
