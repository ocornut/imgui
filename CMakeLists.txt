# manually compiled SDL main since we don't
# want to conform to the link choices of the prebuilt lib

project(imgui)

add_library(imgui STATIC
    imconfig.h
    imgui.cpp
    imgui.h
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_internal.h
    stb_rect_pack.h
    stb_textedit.h
    stb_truetype.h
)

set(IMGUI_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE STRING "ImGui include path"
)

set(IMGUI_LINK_DIR
    ${CMAKE_CURRENT_BINARY_DIR}
    CACHE STRING "ImGui link path"
)

set(IMGUI_LIB
    imgui
    CACHE STRING "ImGui library"
)

set_target_properties(imgui PROPERTIES FOLDER ThirdParty)
