find_package (GLUT QUIET)

if (GLUT_glut_LIBRARY)
    add_library (imgui-glut OBJECT
        "${CMAKE_CURRENT_LIST_DIR}/imgui_impl_glut.cpp"
    )

    target_link_libraries (imgui-glut PUBLIC
        imgui
        "${GLUT_glut_LIBRARY}"
    )

    if (APPLE)
        target_link_libraries (imgui-glut PUBLIC
            "${GLUT_cocoa_LIBRARY}"
        )
    endif ()

    target_include_directories (imgui-glut PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

    target_include_directories (imgui-glut SYSTEM PUBLIC
        "${GLUT_INCLUDE_DIR}"
    )
else ()
    message (WARNING "IMGUI_IMPL_GLUT set to ON but GLUT could not be found")
endif ()
