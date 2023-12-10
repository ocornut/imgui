cmake_minimum_required(VERSION 3.7)
project("imgui_vulkan" VERSION 1.0.0)


set(IMGUI_SOURCES 
imconfig.h
imgui.h
imgui.cpp
imgui_draw.cpp
imgui_demo.cpp
imgui_internal.h
imgui_tables.cpp
imgui_widgets.cpp
imstb_rectpack.h
imstb_textedit.h
imstb_truetype.h
)

if(USE_BACKEND_VULKAN)
set(IMGUI_SOURCES ${IMGUI_SOURCES}
backends/imgui_impl_vulkan.h
backends/imgui_impl_vulkan.cpp
)
endif()

if(USE_BACKEND_WGPU)
set(IMGUI_SOURCES ${IMGUI_SOURCES}
backends/imgui_impl_wgpu.h
backends/imgui_impl_wgpu.cpp
)
endif()

if(USE_BACKEND_GLFW)
set(IMGUI_SOURCES ${IMGUI_SOURCES}
backends/imgui_impl_glfw.h
backends/imgui_impl_glfw.cpp
)
endif()

if(USE_BACKEND_SDL3)
set(IMGUI_SOURCES ${IMGUI_SOURCES}
backends/imgui_impl_sdl3.h
backends/imgui_impl_sdl3.cpp
)
endif()

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC "/")
target_include_directories(imgui PUBLIC "/backends")

target_link_libraries(imgui PRIVATE glfw)

if(USE_BACKEND_VULKAN)
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
target_link_libraries(imgui PRIVATE ${Vulkan_LIBRARIES})
target_include_directories(imgui PUBLIC ${Vulkan_INCLUDE_DIR})
else()
message(ERROR "Vulkan SDK has to be installed")
endif()
endif()