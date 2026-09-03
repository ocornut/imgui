if (APPLE)
    add_library (imgui-osx OBJECT
        "${CMAKE_CURRENT_LIST_DIR}/imgui_impl_osx.mm"
    )

    target_link_libraries (imgui-osx PUBLIC imgui "-framework Cocoa" "-framework AppKit")
    target_include_directories (imgui-osx PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

    set_property (
        TARGET imgui-osx
        APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc"
    )
else ()
    message (WARNING "IMGUI_IMPL_OSX set to ON but platform is not Apple")
endif ()
