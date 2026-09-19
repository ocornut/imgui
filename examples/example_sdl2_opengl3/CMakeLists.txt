if (TARGET imgui-sdl2 AND TARGET imgui-opengl)
    add_executable (imgui_example_sdl2_opengl3 main.cpp)

    target_link_libraries (imgui_example_sdl2_opengl3
        imgui-sdl2 imgui-opengl
    )

    set_target_properties (imgui_example_sdl2_opengl3
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
