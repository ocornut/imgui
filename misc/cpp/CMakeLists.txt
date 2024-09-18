add_library_ns(imgui cpp STATIC imgui_stdlib.cpp)
target_include_dir_iface(imgui-cpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} include PRIVATE ${IMGUI_INC_DIR})
install_target_and_headers(imgui cpp
    imgui_stdlib.h
    )
