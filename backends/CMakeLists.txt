if(IMGUI_IMPL_OPENGL2 OR IMGUI_IMPL_OPENGL3)
find_package(OpenGL)
endif()

if(IMGUI_IMPL_WIN32)
add_library_ns(imgui win32 STATIC imgui_impl_win32.cpp)
target_include_dir_iface(imgui-win32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} include PRIVATE ${IMGUI_INC_DIR})
install_target_and_headers(imgui win32
    imgui_impl_win32.h
    )
endif()

if(OpenGL_FOUND)

if(IMGUI_IMPL_OPENGL2)
add_library_ns(imgui opengl2 STATIC imgui_impl_opengl2.cpp)
target_include_dir_iface(imgui-opengl2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} include PRIVATE ${IMGUI_INC_DIR})
target_link_libraries(imgui-opengl2 OpenGL::GL)
install_target_and_headers(imgui opengl2
    imgui_impl_opengl2.h
    )
endif()

if(IMGUI_IMPL_OPENGL3)
add_library_ns(imgui opengl3 STATIC imgui_impl_opengl3.cpp)
target_include_dir_iface(imgui-opengl3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} include PRIVATE ${IMGUI_INC_DIR})
target_link_libraries(imgui-opengl3 OpenGL::GL)
install_target_and_headers(imgui opengl3
    imgui_impl_opengl3.h
    )
endif()

endif()
