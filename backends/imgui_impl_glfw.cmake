if (NOT TARGET glfw)
    find_package (GLFW3 QUIET)
endif ()

if (TARGET glfw OR GLFW3_DIR)
    add_library (imgui-glfw OBJECT
        "${CMAKE_CURRENT_LIST_DIR}/imgui_impl_glfw.cpp"
    )

    target_link_libraries (imgui-glfw PUBLIC imgui glfw)
    target_include_directories (imgui-glfw PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
else ()
    message (WARNING "IMGUI_IMPL_GLFW set to ON but glfw3 could not be located")
endif ()
