add_executable (imgui_example_null main.cpp)
target_link_libraries (imgui_example_null imgui)

set_target_properties (imgui_example_null
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${IMGUI_EXAMPLE_OUTPUT_DIR}"
)
