cmake_minimum_required(VERSION 3.10)
project(JKMenu)

# 包含 ImGui 目录
set(IMGUI_DIR ../..) # 根据你仓库的实际层级调整
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

# 源文件列表
add_library(JKMenu SHARED
    main.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_android.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# 链接系统库
target_link_libraries(JKMenu android GLESv3 EGL log)
