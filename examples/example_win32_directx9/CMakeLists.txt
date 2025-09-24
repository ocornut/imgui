if (TARGET imgui-win32 AND TARGET imgui-dx9)
    add_executable (imgui_example_win32_directx9 main.cpp)

    target_link_libraries (imgui_example_win32_directx9
        imgui-win32 imgui-dx9
    )

    set_target_properties (imgui_example_win32_directx9
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
