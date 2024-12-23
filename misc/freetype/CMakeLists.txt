find_package(Freetype)
if(Freetype_FOUND)
add_library_ns(imgui freetype STATIC imgui_freetype.cpp)
target_include_dir_iface(imgui-freetype PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} include PRIVATE ${IMGUI_INC_DIR})
target_link_libraries(imgui-freetype Freetype::Freetype)
install_target_and_headers(imgui freetype
    imgui_freetype.h
    )
endif()
