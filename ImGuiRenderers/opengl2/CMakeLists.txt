find_package(OpenGL REQUIRED)

add_library(ImGuiRenderer SHARED
    imgui_impl_opengl2.cpp
)

target_include_directories(ImGuiRenderer 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ImGuiRenderer
    PRIVATE
        ImGuiFramework
    PUBLIC
        OpenGL::GL
)

install(TARGETS ImGuiRenderer 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)