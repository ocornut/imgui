cmake_minimum_required(VERSION 3.22.1)

project(androidvulkanimgui)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../)

include_directories(
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${ANDROID_NDK}/sources/android/native_app_glue
)

set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/backends/imgui_impl_android.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(${CMAKE_PROJECT_NAME} SHARED
        main.cpp
        ${IMGUI_SOURCES}
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log
        vulkan)
