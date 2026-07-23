# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project(ImGuiExample)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_library(${CMAKE_PROJECT_NAME} SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_android.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${ANDROID_NDK}/sources/android/native_app_glue
)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        android
        vulkan
        log
)
