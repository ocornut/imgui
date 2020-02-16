if (TARGET imgui-sdl AND TARGET imgui-metal)
    add_executable (imgui_example_sdl_metal main.mm)

    target_link_libraries (imgui_example_sdl_metal
        imgui-sdl imgui-metal
    )

    set_target_properties (imgui_example_sdl_metal
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
