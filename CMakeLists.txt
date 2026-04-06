cmake_minimum_required(VERSION 3.10)

# 项目名称
project("AndKittyImGuiMod")

# C++ 17 标准支持
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 包含路径
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/backends
)

# 2. 库查找路径
link_directories(${CMAKE_SOURCE_DIR})

# 3. 定义插件源文件 (.so)
set(SOURCE_FILES
    main.cpp
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
)

# 4. 生成共享库 (.so)
add_library(MyMenu SHARED ${SOURCE_FILES})

# 5. 链接插件依赖库
target_link_libraries(MyMenu
    dobby
    log
    GLESv3
    EGL
    android
)

# 6. --- 新增：生成独立注入器可执行文件 (injector) ---
add_executable(injector injector.cpp)

# 注入器需要链接 dl 库来处理远程地址映射
target_link_libraries(injector
    dl
    log
)

# 设置注入器为静态链接 C++ 运行库，确保在手机上直接运行不会缺库
set_target_properties(injector PROPERTIES LINK_FLAGS "-static-libstdc++")
