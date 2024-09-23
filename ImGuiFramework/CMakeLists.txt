
add_library(ImGuiFramework SHARED 
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
)

target_include_directories(ImGuiFramework 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS ImGuiFramework 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)