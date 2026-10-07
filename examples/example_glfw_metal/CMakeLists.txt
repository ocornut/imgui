if (TARGET imgui-glfw AND TARGET imgui-metal)
    add_executable (imgui_example_glfw_metal main.mm)

    target_link_libraries (imgui_example_glfw_metal
        imgui-glfw imgui-metal
    )

    set_target_properties (imgui_example_glfw_metal
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
