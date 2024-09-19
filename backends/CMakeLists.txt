if(IMGUI_IMPL_OPENGL2 OR IMGUI_IMPL_OPENGL3)
find_package(OpenGL)
endif()

macro(add_backend backend)
    # Construct source and header file names from the backend name
    set(SOURCE_FILE "imgui_impl_${backend}.cpp")
    set(HEADER_FILE "imgui_impl_${backend}.h")

    # Add a library for the backend
    add_library_ns(imgui ${backend} STATIC ${SOURCE_FILE})

    # Set up include directories
    target_include_dir_iface(imgui-${backend} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} include PRIVATE ${IMGUI_INC_DIR})

    # Install the target and the corresponding header file
    install_target_and_headers(imgui ${backend} 
        ${HEADER_FILE}
    )
endmacro()

if(IMGUI_IMPL_WIN32)
add_backend(win32)
endif()

if(OpenGL_FOUND)

if(IMGUI_IMPL_OPENGL2)
add_backend(opengl2)
target_link_libraries(imgui-opengl2 OpenGL::GL)
endif()

if(IMGUI_IMPL_OPENGL3)
add_backend(opengl3)
target_link_libraries(imgui-opengl3 OpenGL::GL)
endif()

endif()

if(IMGUI_IMPL_DX9)
add_backend(dx9)
target_link_libraries(imgui-dx9 d3d9.lib)
endif()

if(IMGUI_IMPL_DX10)
add_backend(dx10)
target_link_libraries(imgui-dx10 d3d10.lib)
endif()

if(IMGUI_IMPL_DX11)
add_backend(dx11)
target_link_libraries(imgui-dx11 d3d11.lib)
endif()

if(IMGUI_IMPL_DX12)
add_backend(dx12)
target_link_libraries(imgui-dx12 d3d12.lib)
endif()


