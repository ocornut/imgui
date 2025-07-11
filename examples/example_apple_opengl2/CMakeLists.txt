if (TARGET imgui-osx AND TARGET imgui-opengl2)
    add_executable (imgui_example_apple_opengl2 main.mm)

    target_link_libraries (imgui_example_apple_opengl2
        imgui-osx imgui-opengl2
    )

    set_target_properties (imgui_example_apple_opengl2
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
