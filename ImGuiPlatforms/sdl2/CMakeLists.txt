find_package(sdl2 REQUIRED)

add_library(ImGuiPlatform SHARED
    imgui_impl_sdl2.cpp
)

target_include_directories(ImGuiPlatform 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ImGuiPlatform
    PRIVATE
        ImGuiFramework
    PUBLIC
        ${SDL2_LIBRARIES}
)

install(TARGETS ImGuiPlatform 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)