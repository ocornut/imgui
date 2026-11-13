if (TARGET imgui-win32 AND TARGET imgui-dx11)
    add_executable (imgui_example_win32_directx11 main.cpp)

    target_link_libraries (imgui_example_win32_directx11
        imgui-win32 imgui-dx11
    )

    set_target_properties (imgui_example_win32_directx11
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
