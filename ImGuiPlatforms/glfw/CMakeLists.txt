find_package(glfw3 REQUIRED)

add_library(ImGuiPlatform SHARED
    imgui_impl_glfw.cpp
)

target_include_directories(ImGuiPlatform 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ImGuiPlatform
    PRIVATE
        ImGuiFramework
    PUBLIC
        glfw
)

install(TARGETS ImGuiPlatform 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)