if (WIN32)
    find_library (D3D10_LIBRARY "d3d10")
    if (NOT D3D10_LIBRARY)
        message (WARNING "IMGUI_IMPL_DX10 set to ON but d3d10.dll not found")
    endif ()

    find_library (DXGI_LIBRARY "dxgi")
    if (NOT DXGI_LIBRARY)
        message (WARNING "IMGUI_IMPL_DX10 set to ON but dxgi.dll not found")
    endif ()

    if (D3D10_LIBRARY AND DXGI_LIBRARY)
        add_library (imgui-dx10 OBJECT
            "${CMAKE_CURRENT_LIST_DIR}/imgui_impl_dx10.cpp"
        )

        target_link_libraries (imgui-dx10 PUBLIC
            imgui
            ${D3D10_LIBRARY} ${DXGI_LIBRARY}
        )

        target_include_directories (imgui-dx10 PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
    endif ()
else ()
    message (WARNING "IMGUI_IMPL_DX10 set to ON but platform is not Win32")
endif ()
