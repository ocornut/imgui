if (WIN32)
    find_library (D3D11_LIBRARY "d3d11")
    if (NOT D3D11_LIBRARY)
        message (WARNING "IMGUI_IMPL_DX11 set to ON but d3d11.dll not found")
    endif ()

    find_library (DXGI_LIBRARY "dxgi")
    if (NOT DXGI_LIBRARY)
        message (WARNING "IMGUI_IMPL_DX11 set to ON but dxgi.dll not found")
    endif ()

    if (D3D11_LIBRARY AND DXGI_LIBRARY)
        add_library (imgui-dx11 OBJECT
            "${CMAKE_CURRENT_LIST_DIR}/imgui_impl_dx11.cpp"
        )

        target_link_libraries (imgui-dx11 PUBLIC
            imgui
            ${D3D11_LIBRARY} ${DXGI_LIBRARY}
        )

        target_include_directories (imgui-dx11 PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
    endif ()
else ()
    message (WARNING "IMGUI_IMPL_DX11 set to ON but platform is not Win32")
endif ()
