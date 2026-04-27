if (WIN32)
    find_library (D3D9_LIBRARY "d3d9")
    if (D3D9_LIBRARY)
        add_library (imgui-dx9 OBJECT
            "${CMAKE_CURRENT_LIST_DIR}/imgui_impl_dx9.cpp"
        )

        target_link_libraries (imgui-dx9 PUBLIC
            imgui
            ${D3D9_LIBRARY}
        )

        target_include_directories (imgui-dx9 PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
    else ()
        message (WARNING "IMGUI_IMPL_DX9 set to ON but d3d9.dll not found")
    endif ()
else ()
    message (WARNING "IMGUI_IMPL_DX9 set to ON but platform is not Win32")
endif ()
