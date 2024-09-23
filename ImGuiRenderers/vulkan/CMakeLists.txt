find_package(Vulkan REQUIRED)

add_library(ImGuiRenderer SHARED
    imgui_impl_vulkan.cpp
)

target_include_directories(ImGuiRenderer 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(ImGuiRenderer
    PRIVATE
        ImGuiFramework
    PUBLIC
        ${Vulkan_LIBRARIES}
)

install(TARGETS ImGuiRenderer 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)