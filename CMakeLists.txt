cmake_minimum_required(VERSION 3.10)

# Project Name
project(AndKittyModProject)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Path Definitions ---
set(SOURCE_DIR ${CMAKE_SOURCE_DIR})
# Since all imgui files are in the root directory, we use SOURCE_DIR directly
set(IMGUI_DIR ${SOURCE_DIR})

# --- 2. Include Directories ---
include_directories(
    ${SOURCE_DIR}
    ${SOURCE_DIR}/include
    # Backends are also in root or a specific backends folder? 
    # Assuming standard structure: backends folder exists in root
    ${SOURCE_DIR}/backends
)

# --- 3. Dobby Static Library (Root) ---
add_library(dobby STATIC IMPORTED)
set_target_properties(dobby PROPERTIES IMPORTED_LOCATION 
    ${SOURCE_DIR}/libdobby.a)

# --- 4. Build Plugin (libMyMenu.so) ---
# All files are in the root directory
set(PLUGIN_SOURCES
    main.cpp
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp
    backends/imgui_impl_android.cpp
    backends/imgui_impl_opengl3.cpp
)

add_library(MyMenu SHARED ${PLUGIN_SOURCES})

# Link Libraries
target_link_libraries(MyMenu
    dobby
    log
    GLESv3
    EGL
    android
)

# --- 5. Build Injector Binary ---
if(EXISTS "${SOURCE_DIR}/injector.cpp")
    add_executable(injector injector.cpp)
    target_link_libraries(injector log dl)
    # Static link for better compatibility on Android
    set_target_properties(injector PROPERTIES LINK_FLAGS "-static-libstdc++")
endif()

# Debug Messages
message(STATUS "Android ABI: ${ANDROID_ABI}")
message(STATUS "Looking for source: ${SOURCE_DIR}/imgui.cpp")
