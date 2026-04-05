cmake_minimum_required(VERSION 3.10)
project(ModMenu)

set(CMAKE_CXX_STANDARD 17)

# --- 1. 引入 ImGui 源码 (假设放在项目根目录的 imgui 文件夹) ---
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_android.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# --- 2. 引入 Dobby Hook 库 ---
# 假设 dobby.h 在根目录，libdobby.a 在根目录下的 libs/${ANDROID_ABI}/ 文件夹中
set(DOBBY_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})
set(DOBBY_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI})

add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES IMPORTED_LOCATION ${DOBBY_LIB_DIR}/libdobby.a)

# --- 3. 创建你的目标库 libmodmenu.so ---
add_library(modmenu SHARED main.cpp ${IMGUI_SOURCES})

# --- 4. 包含头文件路径 ---
target_include_directories(modmenu PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${DOBBY_INCLUDE}
)

# --- 5. 链接必要的系统库和 Dobby 库 ---
target_link_libraries(modmenu
        dobby
        android
        EGL
        GLESv3
        log
        z
)
