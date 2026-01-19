cmake_minimum_required(VERSION 3.10)
project(example_sdl3_vulkan)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL3 源码路径（根据你的实际路径修改）
set(SDL3_SOURCE_DIR "D:/p90x/SDL3-3.4.0")

# 将 SDL3 作为子项目编译
if(EXISTS "${SDL3_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "Building SDL3 from source: ${SDL3_SOURCE_DIR}")
    
    # 禁用 SDL3 的一些不需要的选项，加快编译
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    
    # 添加 SDL3 子目录
    add_subdirectory(${SDL3_SOURCE_DIR} ${CMAKE_BINARY_DIR}/SDL3 EXCLUDE_FROM_ALL)
    
    set(SDL3_LIBRARY SDL3::SDL3)
else()
    message(FATAL_ERROR "SDL3 source not found at ${SDL3_SOURCE_DIR}")
endif()

# 查找 Vulkan
find_package(Vulkan REQUIRED)

# ImGui 源文件（dev 文件夹在项目根目录，所以返回上一级）
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# 创建可执行文件
add_executable(example_sdl3_vulkan
    main.cpp
    ${IMGUI_SOURCES}
)

# 包含目录
target_include_directories(example_sdl3_vulkan PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${Vulkan_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(example_sdl3_vulkan
    ${SDL3_LIBRARY}
    ${Vulkan_LIBRARIES}
)

# 自动复制 SDL3.dll 到可执行文件目录
add_custom_command(TARGET example_sdl3_vulkan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:example_sdl3_vulkan>
    COMMENT "Copying SDL3.dll to output directory"
)

# Windows 特定设置
if(WIN32)
    # 设置为窗口应用程序（不显示控制台）
    # 如果需要看到 printf 输出，可以注释掉这行
    # set_target_properties(example_sdl3_vulkan PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
