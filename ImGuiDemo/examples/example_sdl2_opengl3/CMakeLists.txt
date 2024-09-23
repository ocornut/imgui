add_executable(ImGuiDemo
    main.cpp
)

target_include_directories(ImGuiDemo 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ImGuiDemo 
    PRIVATE
        ImGuiFramework
        ImGuiPlatform
        ImGuiRenderer
        ImGuiDemoContent
)
