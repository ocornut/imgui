if (TARGET imgui-sdl2 AND TARGET imgui-metal)
    add_executable (imgui_example_sdl2_metal main.mm)

    target_link_libraries (imgui_example_sdl2_metal
        imgui-sdl2 imgui-metal
    )

    set_target_properties (imgui_example_sdl2_metal
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
