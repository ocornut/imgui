cmake_minimum_required(VERSION 3.10)
project(ModMenu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========== 1. 编译注入器 (如果指定了 BUILD_INJECTOR) ==========
if(BUILD_INJECTOR)
    add_executable(xinjector injector.cpp)  # 请确保根目录下有 injector.cpp
    target_link_libraries(xinjector dl)
    # 可选: 指定输出路径，方便查找
    set_target_properties(xinjector PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    return()
endif()

# ========== 2. 编译 libmodmenu.so (默认) ==========
set(IMGUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_android.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# 引入 Dobby 预编译库
add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libdobby.a)

# 创建目标库
add_library(modmenu SHARED main.cpp ${IMGUI_SOURCES})

# 包含头文件路径
target_include_directories(modmenu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

# 链接库
target_link_libraries(modmenu
    dobby
    android
    EGL
    GLESv3
    log
    z
)

# 指定输出路径
set_target_properties(modmenu PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
