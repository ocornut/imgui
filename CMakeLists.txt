set(SOURCES
        imconfig.h
        imgui.h
        imgui.cpp
        imgui_draw.cpp
        imgui_internal.h
        imgui_tables.cpp
        imgui_widgets.cpp
        imstb_rectpack.h
        imstb_textedit.h
        imstb_truetype.h
        imgui_demo.cpp
        backends/imgui_impl_glfw.cpp
        backends/imgui_impl_vulkan.cpp
)

add_library(ImGui STATIC ${SOURCES})

target_include_directories(
        ImGui
        PRIVATE
        ${CMAKE_SOURCE_DIR}/Pine/extern/imgui
)