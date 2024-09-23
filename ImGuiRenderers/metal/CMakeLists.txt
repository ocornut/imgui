add_library(ImGuiRenderer SHARED
    imgui_impl_metal.mm
)

target_include_directories(ImGuiRenderer 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ImGuiRenderer
    PRIVATE
        ImGuiFramework
    PUBLIC
        "-framework Metal"
        "-framework MetalKit"
        "-framework Foundation"
        "-framework QuartzCore"
)

install(TARGETS ImGuiRenderer 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)