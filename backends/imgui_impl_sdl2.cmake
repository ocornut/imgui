find_package (SDL2 QUIET)

if (SDL2_DIR)
    add_library (imgui-sdl2 OBJECT
        "${CMAKE_CURRENT_LIST_DIR}/imgui_impl_sdl2.cpp"
    )

    target_link_libraries (imgui-sdl2 PUBLIC imgui sdl2)
    target_include_directories (imgui-sdl2 PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

    # Fixes a strange inclusion problem on MacOS due to an unconventional
    # SDL installation with Homebrew. Innocuous if SDL is being pulled in
    # from somewhere else.
    if (APPLE)
        target_include_directories (imgui-sdl2 SYSTEM PUBLIC "/usr/local/include/SDL2")
    endif ()
else ()
    message (WARNING "IMGUI_IMPL_SDL2 set to ON but SDL2 not found on system or in project")
endif ()
