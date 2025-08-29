if (TARGET imgui-sdl2 AND TARGET imgui-opengl2)
    add_executable (imgui_example_sdl2_opengl2 WIN32 main.cpp)

    target_link_libraries (imgui_example_sdl2_opengl2
        imgui-sdl2 imgui-opengl2
    )

    set_target_properties (imgui_example_sdl2_opengl2
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
