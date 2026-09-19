# NOTE: These are polyfill libs for developing GLFW windows examples;
#       they are NOT intended to be used for production; instead, you
#       acquire your own version (either from pre-built binaries or
#       from building from source).
#
#       More info at https://glfw.org.

if (WIN32 AND NOT TARGET glfw)
    message (
        WARNING
            "ImGui polyfill glfw target should not be used for production! "
            "If you're seeing this while building ImGui examples, please ignore."
    )

    add_library (glfw STATIC IMPORTED)

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set_target_properties (glfw PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/glfw/lib-vc2010-64/glfw3.lib")
    else ()
        set_target_properties (glfw PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/glfw/lib-vc2010-32/glfw3.lib")
    endif ()

    target_include_directories (glfw SYSTEM INTERFACE "${CMAKE_CURRENT_LIST_DIR}/glfw/include")
endif ()
