if (TARGET imgui-win32 AND TARGET imgui-dx10)
    add_executable (imgui_example_win32_directx10 main.cpp)

    target_link_libraries (imgui_example_win32_directx10
        imgui-win32 imgui-dx10
    )

    set_target_properties (imgui_example_win32_directx10
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
