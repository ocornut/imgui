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

if(IMGUI_IMPL_DX9)
add_library_ns(imgui dx9 STATIC imgui_impl_dx9.cpp)
target_include_dir_iface(imgui-dx9 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} include PRIVATE ${IMGUI_INC_DIR})
target_link_libraries(imgui-dx9 d3d9.lib)
install_target_and_headers(imgui dx9
    imgui_impl_dx9.h
    )
endif()

if(IMGUI_IMPL_DX10)
add_library_ns(imgui dx10 STATIC imgui_impl_dx10.cpp)
target_include_dir_iface(imgui-dx10 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} include PRIVATE ${IMGUI_INC_DIR})
target_link_libraries(imgui-dx10 d3d10.lib)
install_target_and_headers(imgui dx10
    imgui_impl_dx10.h
    )
endif()

if(IMGUI_IMPL_DX11)
add_library_ns(imgui dx11 STATIC imgui_impl_dx11.cpp)
target_include_dir_iface(imgui-dx11 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} include PRIVATE ${IMGUI_INC_DIR})
target_link_libraries(imgui-dx11 d3d11.lib)
install_target_and_headers(imgui dx11
    imgui_impl_dx11.h
    )
endif()

if(IMGUI_IMPL_DX12)
add_library_ns(imgui dx12 STATIC imgui_impl_dx12.cpp)
target_include_dir_iface(imgui-dx12 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} include PRIVATE ${IMGUI_INC_DIR})
target_link_libraries(imgui-dx12 d3d12.lib)
install_target_and_headers(imgui dx12
    imgui_impl_dx12.h
    )
endif()


