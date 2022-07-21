cmake_minimum_required(VERSION 3.16...3.21)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

Project(RayTracing)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE FILEPATH "Path to a compiler launcher program, e.g. ccache")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE FILEPATH "Path to a compiler launcher program, e.g. ccache")
endif()

add_compile_options(-Wall)
add_compile_options(-Wextra)
add_compile_options(-Werror)
add_compile_options(-Wpedantic)

set(SDL2_DIR /usr/include/SDL2)
set(IMGUI_DIR ../..)
set(VULKAN_SDK_DIR /usr/include/vulkan)

add_executable(rayTracing)

target_sources(rayTracing PRIVATE
    main.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp

    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

)


execute_process(COMMAND pkg-config --libs sdl2 OUTPUT_VARIABLE SDL2_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND pkg-config --libs vulkan OUTPUT_VARIABLE VULKAN_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

target_include_directories(rayTracing
    PRIVATE
    ${SDL2_DIR}
    ${VULKAN_SDK_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(rayTracing
    PRIVATE
    ${SDL2_LIBS}
    ${VULKAN_LIBS}
    -ldl
)
