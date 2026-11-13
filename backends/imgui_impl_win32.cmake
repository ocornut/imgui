if (WIN32)
    add_library (imgui-win32 OBJECT
        "${CMAKE_CURRENT_LIST_DIR}/imgui_impl_win32.cpp"
    )

    target_link_libraries (imgui-win32 PUBLIC imgui)

    target_include_directories (imgui-win32 PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
else ()
    message (WARNING "IMGUI_IMPL_WIN32 set to ON but platform is not Win32")
endif ()
