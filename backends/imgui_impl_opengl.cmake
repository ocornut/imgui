find_package (OpenGL QUIET)

if (OPENGL_gl_LIBRARY)
    if (IMGUI_IMPL_OPENGL2)
        add_library (imgui-opengl2 OBJECT
            "${CMAKE_CURRENT_LIST_DIR}/imgui_impl_opengl2.cpp"
        )

        target_link_libraries (imgui-opengl2 PUBLIC
            imgui
            "${OPENGL_gl_LIBRARY}"
        )

        target_include_directories (imgui-opengl2 PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
        target_include_directories (imgui-opengl2 SYSTEM PUBLIC "${OPENGL_INCLUDE_DIR}")
    endif ()
else ()
    message (WARNING "IMGUI_IMPL_OPENGL and/or IMGUI_IMPL_OPENGL2 set to ON but OpenGL could not be found")
endif ()
