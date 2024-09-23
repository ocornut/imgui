add_library(ImGuiPlatform SHARED
    imgui_impl_osx.mm
)

target_include_directories(ImGuiPlatform 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ImGuiPlatform
    PRIVATE
        ImGuiFramework
    PUBLIC
        "-framework Cocoa"
        "-framework Carbon"
        "-framework GameController"
)

install(TARGETS ImGuiPlatform 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)