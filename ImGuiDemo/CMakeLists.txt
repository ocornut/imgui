add_library(ImGuiDemoContent SHARED
    imgui_demo.cpp
)

target_include_directories(ImGuiDemoContent 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        $<TARGET_PROPERTY:ImGuiFramework,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(ImGuiDemoContent
    ImGuiFramework
)

target_link_libraries(ImGuiDemoContent
    PRIVATE
        ImGuiFramework
)

install(TARGETS ImGuiDemoContent 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

add_subdirectory(examples/example_${ImGuiPlatform_Option}_${ImguiRenderer_Option})