cmake_minimum_required(VERSION 3.21.2)

if (CMAKE_COMPILER_IS_GNUCC)
    add_compile_options(-w)
endif()
if(MSVC)
    add_compile_options(/MP)
    add_compile_options(/W0)
endif()

file(GLOB IMGUI_FILES true ABSOLUTE
${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.h)

set(BACKENDS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/backends/)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB IMGUI_BACKEND_FILES true ABSOLUTE 
${BACKENDS_DIR}imgui_impl_opengl3.cpp ${BACKENDS_DIR}imgui_impl_opengl3.h
# ${BACKENDS_DIR}imgui_impl_vulkan.cpp ${BACKENDS_DIR}imgui_impl_vulkan.h
${BACKENDS_DIR}imgui_impl_glfw.cpp ${BACKENDS_DIR}imgui_impl_glfw.h)

if(WIN32)
    file(GLOB IMGUI_WINDOWS_BACKEND_FILES true ABSOLUTE ${CMAKE_CURRENT_SOURCE_DIR}/backends/
    ${BACKENDS_DIR}imgui_impl_dx11.cpp ${BACKENDS_DIR}imgui_impl_dx11.h
    # ${BACKENDS_DIR}imgui_impl_dx12.cpp ${BACKENDS_DIR}imgui_impl_dx12.h
    ${BACKENDS_DIR}imgui_impl_win32.cpp ${BACKENDS_DIR}imgui_impl_win32.h)

    list(APPEND IMGUI_BACKEND_FILES ${IMGUI_WINDOWS_BACKEND_FILES})
endif()

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD)
include_directories(${DEPENDENCIES_DIR}GLAD/include)
include_directories(${DEPENDENCIES_DIR}GLFW/include)

add_library(imgui STATIC ${IMGUI_FILES} ${IMGUI_BACKEND_FILES})