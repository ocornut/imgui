if (APPLE)
    add_library (imgui-metal OBJECT
        "${CMAKE_CURRENT_LIST_DIR}/imgui_impl_metal.mm"
    )

    target_link_libraries (imgui-metal PUBLIC
        imgui
        "-framework Cocoa" "-framework Metal" "-framework QuartzCore"
    )

    target_include_directories (imgui-metal PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

    set_property (
        TARGET imgui-metal
        APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc"
    )
else ()
    message (WARNING "IMGUI_IMPL_METAL set to ON but platform is not Apple")
endif ()
