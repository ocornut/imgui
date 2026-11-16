if (TARGET imgui-win32 AND TARGET imgui-dx12)
    add_executable (imgui_example_win32_directx12 main.cpp)

    target_link_libraries (imgui_example_win32_directx12
        imgui-win32 imgui-dx12
    )

    set_target_properties (imgui_example_win32_directx12
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
    )
endif ()
