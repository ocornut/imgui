cmake_minimum_required(VERSION 3.23)
project(example_android_opengl3)

# find_package(imgui REQUIRED)

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME}
    PUBLIC
            FILE_SET HEADERS FILES ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.h
            BASE_DIRS ${ANDROID_NDK}/sources/android/native_app_glue
    PRIVATE
        main.cpp ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui imgui::backend_android_opengl3)
