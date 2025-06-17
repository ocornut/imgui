if (TARGET imgui-metal AND TARGET imgui-osx)
    # TODO proper bundling of assets for macOS and iOS

    add_executable (imgui_example_apple_metal
        main.mm
    )

    target_link_libraries (imgui_example_apple_metal
        imgui-metal imgui-osx
    )

    set_target_properties (imgui_example_apple_metal
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
