if (TARGET imgui-glfw AND TARGET imgui-opengl)
    add_executable (imgui_example_glfw_opengl3 main.cpp)

    target_link_libraries (imgui_example_glfw_opengl3
        imgui-glfw imgui-opengl
    )

    set_target_properties (imgui_example_glfw_opengl3
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
