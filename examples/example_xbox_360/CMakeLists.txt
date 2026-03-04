cmake_minimum_required(VERSION 3.20)

project(imgui_example_xbox_360 CXX C)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Xbox360")
    message(FATAL_ERROR
        "This CMakeLists.txt is for Xbox 360.\n"
        "Use: cmake -DCMAKE_TOOLCHAIN_FILE=XDKToolchain.cmake")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Tipo de build" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(IMGUI_DIR ../../)

set(IMGUI_EXAMPLE_SOURCE_FILES
    main.cpp
    # backend files
    ${IMGUI_DIR}/backends/imgui_impl_xdk_dx9.cpp
    ${IMGUI_DIR}/backends/imgui_impl_xbox_360.cpp
    # Dear ImGui files
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

add_executable(${PROJECT_NAME} ${IMGUI_EXAMPLE_SOURCE_FILES})

xdk_target_link_libs(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${SDL2_INCLUDE_DIRS}
)

xdk_add_xex(${PROJECT_NAME}
    TITLE_ID FFFF0030
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)