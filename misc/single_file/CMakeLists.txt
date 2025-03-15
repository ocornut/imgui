add_library_ns(imgui jumbo INTERFACE)
target_include_dir_iface(imgui-jumbo INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} include INTERFACE ${IMGUI_INC_DIR})
install_target_and_headers(imgui jumbo
    imgui_single_file.h
    )
