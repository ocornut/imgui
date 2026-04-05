cmake_minimum_required(VERSION 3.10)
project(ModMenu)

set(CMAKE_CXX_STANDARD 17)

# 1. 引入 ImGui 源码（假设放在根目录下的 imgui 文件夹）
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_android.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# 2. 引入根目录下的 Dobby
# dobby.h 也在根目录
set(DOBBY_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})
# libdobby.a 直接位于根目录
set(DOBBY_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libdobby.a)

add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES IMPORTED_LOCATION ${DOBBY_LIB})

# 3. 创建目标库
add_library(modmenu SHARED main.cpp ${IMGUI_SOURCES})

# 4. 包含头文件路径
target_include_directories(modmenu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${DOBBY_INCLUDE}
)

# 5. 链接库
target_link_libraries(modmenu
    dobby
    android
    EGL
    GLESv3
    log
    z
)
