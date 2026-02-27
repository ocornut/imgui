if (WIN32)
    find_library (D3D12_LIBRARY "d3d12")
    if (NOT D3D12_LIBRARY)
        message (WARNING "IMGUI_IMPL_DX12 set to ON but d3d12.dll not found")
    endif ()

    find_library (DXGI_LIBRARY "dxgi")
    if (NOT DXGI_LIBRARY)
        message (WARNING "IMGUI_IMPL_DX12 set to ON but dxgi.dll not found")
    endif ()

    if (D3D12_LIBRARY AND DXGI_LIBRARY)
        add_library (imgui-dx12 OBJECT
            "${CMAKE_CURRENT_LIST_DIR}/imgui_impl_dx12.cpp"
        )

        target_link_libraries (imgui-dx12 PUBLIC
            imgui
            ${D3D12_LIBRARY} ${DXGI_LIBRARY}
        )

        target_include_directories (imgui-dx12 PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
    endif ()
else ()
    message (WARNING "IMGUI_IMPL_DX12 set to ON but platform is not Win32")
endif ()
