if (TARGET imgui-glfw AND TARGET imgui-opengl2)
    add_executable (imgui_example_glfw_opengl2 main.cpp)

    target_link_libraries (imgui_example_glfw_opengl2
        imgui-glfw imgui-opengl2
    )

    set_target_properties (imgui_example_glfw_opengl2
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
