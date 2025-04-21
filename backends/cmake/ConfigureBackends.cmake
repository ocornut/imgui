function (add_backend NAME HEADER_FILES CPP_FILES TARGET_LIBS)
    add_library(imgui_backend_${NAME} STATIC)
    add_library(imgui::backend_${NAME} ALIAS imgui_backend_${NAME})

    target_sources(imgui_backend_${NAME}
    PUBLIC FILE_SET HEADERS FILES ${HEADER_FILES}
    PRIVATE ${CPP_FILES}
    )
    target_compile_features(imgui_backend_${NAME} PRIVATE cxx_std_11)
    if (MSVC)
        target_compile_options(imgui_backend_${NAME} PRIVATE /W3)
    else ()
        target_compile_options(imgui_backend_${NAME} PRIVATE -Wall -Wformat)
    endif ()

    target_include_directories(imgui_backend_${NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(imgui_backend_${NAME} PUBLIC imgui::imgui)
    target_link_libraries(imgui_backend_${NAME} PUBLIC ${TARGET_LIBS})

    set_target_properties(imgui_backend_${NAME} PROPERTIES EXPORT_NAME "backend_${NAME}")
    install(TARGETS imgui_backend_${NAME}
            EXPORT imguiTargets
            ARCHIVE DESTINATION lib
            FILE_SET HEADERS DESTINATION include
    )
endfunction ()

function (add_emscripten_flags TARGET USE_RENDER_FLAG USE_FILESYSTEM)

    target_sources(${TARGET}
        PUBLIC
            FILE_SET HEADERS
            FILES ${PROJECT_SOURCE_DIR}/backends/emscripten_mainloop_stub.h
            BASE_DIRS ${PROJECT_SOURCE_DIR}/backends
    )

    target_compile_options(${TARGET}
        PUBLIC
        -sDISABLE_EXCEPTION_CATCHING=1
        -Os
    )

    target_compile_options(${TARGET} PUBLIC ${USE_RENDER_FLAG})
    target_link_options(${TARGET} PUBLIC ${USE_RENDER_FLAG})

    target_link_options(${TARGET}
        PUBLIC
        "${IMGUI_EMSCRIPTEN_GLFW3}"
        "-sWASM=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sNO_EXIT_RUNTIME=0"
        "-sASSERTIONS=1"
        "-sDISABLE_EXCEPTION_CATCHING=1"
    )

    if (USE_FILESYSTEM)
        target_link_options(${TARGET}
            PUBLIC
            "--no-heap-copy"
            "--preload-file"
            "${PROJECT_SOURCE_DIR}/misc/fonts@/fonts"
        )
    else ()
        target_compile_options(${TARGET}
            PUBLIC
            -DIMGUI_DISABLE_FILE_FUNCTIONS
        )
        target_link_options(${TARGET}
            PUBLIC
            "-sNO_FILESYSTEM=1"
        )
    endif ()
endfunction()

if ("glfw_opengl2" IN_LIST SUPPORTED_BACKENDS)
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)

    set(HEADER_FILES imgui_impl_glfw.h imgui_impl_opengl2.h)
    set(CPP_FILES imgui_impl_glfw.cpp imgui_impl_opengl2.cpp)
    set(TARGET_LIBS glfw OpenGL::GL)

    add_backend("glfw_opengl2" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("glfw_opengl3" IN_LIST SUPPORTED_BACKENDS)
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)

    set(HEADER_FILES imgui_impl_glfw.h imgui_impl_opengl3.h)
    set(CPP_FILES imgui_impl_glfw.cpp imgui_impl_opengl3.cpp)
    set(TARGET_LIBS glfw OpenGL::GL)

    add_backend("glfw_opengl3" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("glfw_vulkan" IN_LIST SUPPORTED_BACKENDS)
    find_package(glfw3 REQUIRED)
    find_package(Vulkan REQUIRED)

    set(HEADER_FILES imgui_impl_glfw.h imgui_impl_vulkan.h)
    set(CPP_FILES imgui_impl_glfw.cpp imgui_impl_vulkan.cpp)
    set(TARGET_LIBS glfw Vulkan::Vulkan)

    add_backend("glfw_vulkan" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("sdl2_opengl2" IN_LIST SUPPORTED_BACKENDS)
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)

    set(HEADER_FILES imgui_impl_sdl2.h imgui_impl_opengl2.h)
    set(CPP_FILES imgui_impl_sdl2.cpp imgui_impl_opengl2.cpp)
    set(TARGET_LIBS SDL2::SDL2main SDL2::SDL2 OpenGL::GL)

    add_backend("sdl2_opengl2" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("sdl2_opengl3" IN_LIST SUPPORTED_BACKENDS)
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)

    set(HEADER_FILES imgui_impl_sdl2.h imgui_impl_opengl3.h)
    set(CPP_FILES imgui_impl_sdl2.cpp imgui_impl_opengl3.cpp)
    set(TARGET_LIBS SDL2::SDL2main SDL2::SDL2 OpenGL::GL)

    add_backend("sdl2_opengl3" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("sdl2_sdlrenderer2" IN_LIST SUPPORTED_BACKENDS)
    find_package(SDL2 REQUIRED)

    set(HEADER_FILES imgui_impl_sdl2.h imgui_impl_sdlrenderer2.h)
    set(CPP_FILES imgui_impl_sdl2.cpp imgui_impl_sdlrenderer2.cpp)
    set(TARGET_LIBS SDL2::SDL2main SDL2::SDL2)

    add_backend("sdl2_sdlrenderer2" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("sdl2_vulkan" IN_LIST SUPPORTED_BACKENDS)
    find_package(SDL2 REQUIRED)
    find_package(Vulkan REQUIRED)

    set(HEADER_FILES imgui_impl_sdl2.h imgui_impl_vulkan.h)
    set(CPP_FILES imgui_impl_sdl2.cpp imgui_impl_vulkan.cpp)
    set(TARGET_LIBS SDL2::SDL2main SDL2::SDL2 Vulkan::Vulkan)

    add_backend("sdl2_vulkan" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("sdl3_opengl3" IN_LIST SUPPORTED_BACKENDS)
    find_package(SDL3 REQUIRED)
    find_package(OpenGL REQUIRED)

    set(HEADER_FILES imgui_impl_sdl3.h imgui_impl_opengl3.h)
    set(CPP_FILES imgui_impl_sdl3.cpp imgui_impl_opengl3.cpp)
    set(TARGET_LIBS SDL3::SDL3 OpenGL::GL)

    add_backend("sdl3_opengl3" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("sdl3_sdlrenderer3" IN_LIST SUPPORTED_BACKENDS)
    find_package(SDL3 REQUIRED)

    set(HEADER_FILES imgui_impl_sdl3.h imgui_impl_sdlrenderer3.h)
    set(CPP_FILES imgui_impl_sdl3.cpp imgui_impl_sdlrenderer3.cpp)
    set(TARGET_LIBS SDL3::SDL3)

    add_backend("sdl3_sdlrenderer3" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("sdl3_sdlgpu3" IN_LIST SUPPORTED_BACKENDS)
    find_package(SDL3 REQUIRED)

    set(HEADER_FILES imgui_impl_sdl3.h imgui_impl_sdlgpu3.h imgui_impl_sdlgpu3_shaders.h)
    set(CPP_FILES imgui_impl_sdl3.cpp imgui_impl_sdlgpu3.cpp)
    set(TARGET_LIBS SDL3::SDL3)

    add_backend("sdl3_sdlgpu3" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("sdl3_vulkan" IN_LIST SUPPORTED_BACKENDS)
    find_package(SDL3 REQUIRED)
    find_package(Vulkan REQUIRED)

    set(HEADER_FILES imgui_impl_sdl3.h imgui_impl_vulkan.h)
    set(CPP_FILES imgui_impl_sdl3.cpp imgui_impl_vulkan.cpp)
    set(TARGET_LIBS SDL3::SDL3 Vulkan::Vulkan)

    add_backend("sdl3_vulkan" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("glut_opengl2" IN_LIST SUPPORTED_BACKENDS)
    find_package(GLUT QUIET)
    if (NOT GLUT_FOUND)
        find_package(FreeGLUT REQUIRED)
        set(TARGET_LIBS FreeGLUT::freeglut_static OpenGL::GL)
    else()
        set(TARGET_LIBS glut OpenGL)
    endif ()
    find_package(OpenGL REQUIRED)

    set(HEADER_FILES imgui_impl_glut.h imgui_impl_opengl2.h)
    set(CPP_FILES imgui_impl_glut.cpp imgui_impl_opengl2.cpp)

    add_backend("glut_opengl2" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("allegro5" IN_LIST SUPPORTED_BACKENDS)
    find_package(Allegro REQUIRED)

    add_library(imgui_alegro STATIC)
    add_library(imgui::imgui_alegro ALIAS imgui_alegro)

    target_sources(imgui_alegro
    PUBLIC
        FILE_SET imgui_headers TYPE HEADERS
            FILES
            ${PROJECT_SOURCE_DIR}/imconfig.h
            ${PROJECT_SOURCE_DIR}/imgui.h
            ${PROJECT_SOURCE_DIR}/imgui_internal.h
            ${PROJECT_SOURCE_DIR}/imstb_rectpack.h
            ${PROJECT_SOURCE_DIR}/imstb_textedit.h
            ${PROJECT_SOURCE_DIR}/imstb_truetype.h
            BASE_DIRS ${PROJECT_SOURCE_DIR}
        FILE_SET config_headers TYPE HEADERS
            FILES ${PROJECT_SOURCE_DIR}/backends/imconfig_allegro5.h
            BASE_DIRS ${PROJECT_SOURCE_DIR}/backends
    PRIVATE
            ${PROJECT_SOURCE_DIR}/imgui.cpp
            ${PROJECT_SOURCE_DIR}/imgui_demo.cpp
            ${PROJECT_SOURCE_DIR}/imgui_draw.cpp
            ${PROJECT_SOURCE_DIR}/imgui_tables.cpp
            ${PROJECT_SOURCE_DIR}/imgui_widgets.cpp
    )
    target_compile_features(imgui_alegro PRIVATE cxx_std_11)
    if (MSVC)
        target_compile_options(imgui_alegro PRIVATE /W3)
    else ()
        target_compile_options(imgui_alegro PRIVATE -Wall -Wformat)
    endif ()
    target_compile_definitions(imgui_alegro PUBLIC "IMGUI_USER_CONFIG=<imconfig_allegro5.h>")

    target_include_directories(imgui_alegro PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )

    if (WITH_FREETYPE)
        target_sources(imgui_alegro
            PRIVATE
            ${PROJECT_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
        )

        find_package(freetype REQUIRED CONFIG)
        target_link_libraries(imgui_alegro PRIVATE Freetype::Freetype)
    endif ()

    install(TARGETS imgui_alegro
            EXPORT imguiTargets
            ARCHIVE DESTINATION lib
            FILE_SET imgui_headers DESTINATION include
            FILE_SET config_headers DESTINATION include
    )

    add_library(imgui_backend_allegro5 STATIC)
    add_library(imgui::backend_allegro5 ALIAS imgui_backend_allegro5)

    target_sources(imgui_backend_allegro5
    PUBLIC
        FILE_SET imgui_backend_allegro5_headers TYPE HEADERS
            FILES ${PROJECT_SOURCE_DIR}/backends/imgui_impl_allegro5.h
            BASE_DIRS ${PROJECT_SOURCE_DIR}/backends
    PRIVATE ${PROJECT_SOURCE_DIR}/backends/imgui_impl_allegro5.cpp
    )
    target_compile_features(imgui_backend_allegro5 PRIVATE cxx_std_11)
    if (MSVC)
        target_compile_options(imgui_backend_allegro5 PRIVATE /W3)
    else ()
        target_compile_options(imgui_backend_allegro5 PRIVATE -Wall -Wformat)
    endif ()

    target_include_directories(imgui_backend_allegro5 PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/backends>
        $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(imgui_backend_allegro5 PUBLIC imgui::imgui_alegro)
    target_link_libraries(imgui_backend_allegro5 PUBLIC Allegro::allegro Allegro::allegro_main Allegro::allegro_primitives Allegro::allegro_color Allegro::allegro_image Allegro::allegro_font Allegro::allegro_audio Allegro::allegro_memfile Allegro::allegro_dialog)

    set_target_properties(imgui_backend_allegro5 PROPERTIES EXPORT_NAME "backend_allegro5")
    install(TARGETS imgui_backend_allegro5
            EXPORT imguiTargets
            ARCHIVE DESTINATION lib
            FILE_SET imgui_backend_allegro5_headers DESTINATION include
    )
endif ()

if ("glfw_wgpu_emscripten" IN_LIST SUPPORTED_BACKENDS)

    set(HEADER_FILES imgui_impl_glfw.h imgui_impl_wgpu.h)
    set(CPP_FILES imgui_impl_glfw.cpp imgui_impl_wgpu.cpp)
    set(TARGET_LIBS "")

    if(EMSCRIPTEN_VERSION VERSION_GREATER_EQUAL "3.1.57")
        set(USE_RENDER_FLAG "--use-port=contrib.glfw3")
    else()
        # cannot use contrib.glfw3 prior to 3.1.57
        set(USE_RENDER_FLAG -s USE_GLFW=3)
    endif()
    set(USE_RENDER_FLAG "${USE_RENDER_FLAG}" -sUSE_WEBGPU=1)

    add_backend("glfw_wgpu_emscripten" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    add_emscripten_flags(imgui_backend_glfw_wgpu_emscripten "${USE_RENDER_FLAG}" 0)
endif ()

if ("glfw_opengl3_emscripten" IN_LIST SUPPORTED_BACKENDS)

    set(HEADER_FILES imgui_impl_glfw.h imgui_impl_opengl3.h)
    set(CPP_FILES imgui_impl_glfw.cpp imgui_impl_opengl3.cpp)
    set(TARGET_LIBS "")

    if(EMSCRIPTEN_VERSION VERSION_GREATER_EQUAL "3.1.57")
        set(USE_RENDER_FLAG "--use-port=contrib.glfw3")
    else()
        # cannot use contrib.glfw3 prior to 3.1.57
        set(USE_RENDER_FLAG "-sUSE_GLFW=3")
    endif()

    add_backend("glfw_opengl3_emscripten" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    add_emscripten_flags(imgui_backend_glfw_opengl3_emscripten "${USE_RENDER_FLAG}" 0)
endif ()

if ("sdl2_opengl3_emscripten" IN_LIST SUPPORTED_BACKENDS)

    set(HEADER_FILES imgui_impl_sdl2.h imgui_impl_opengl3.h)
    set(CPP_FILES imgui_impl_sdl2.cpp imgui_impl_opengl3.cpp)
    set(TARGET_LIBS "")

    set(USE_RENDER_FLAG "-sUSE_SDL=2")

    add_backend("sdl2_opengl3_emscripten" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    add_emscripten_flags(imgui_backend_sdl2_opengl3_emscripten "${USE_RENDER_FLAG}" 1)
endif ()

if ("sdl3_opengl3_emscripten" IN_LIST SUPPORTED_BACKENDS)

    set(HEADER_FILES imgui_impl_sdl3.h imgui_impl_opengl3.h)
    set(CPP_FILES imgui_impl_sdl3.cpp imgui_impl_opengl3.cpp)
    set(TARGET_LIBS "")

    set(USE_RENDER_FLAG "-sUSE_SDL=3")

    add_backend("sdl3_opengl3_emscripten" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    add_emscripten_flags(imgui_backend_sdl3_opengl3_emscripten "${USE_RENDER_FLAG}" 1)
endif ()

if ("sdl3_sdlrenderer3_emscripten" IN_LIST SUPPORTED_BACKENDS)

    set(HEADER_FILES imgui_impl_sdl3.h imgui_impl_sdlrenderer3.h)
    set(CPP_FILES imgui_impl_sdl3.cpp imgui_impl_sdlrenderer3.cpp)
    set(TARGET_LIBS "")

    set(USE_RENDER_FLAG "-sUSE_SDL=3")

    add_backend("sdl3_sdlrenderer3_emscripten" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    add_emscripten_flags(imgui_backend_sdl3_sdlrenderer3_emscripten "${USE_RENDER_FLAG}" 1)
endif ()

if ("glfw_wgpu_dawn" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_glfw.h imgui_impl_wgpu.h)
    set(CPP_FILES imgui_impl_glfw.cpp imgui_impl_wgpu.cpp)
    set(TARGET_LIBS glfw dawn::webgpu_dawn)

    add_backend("glfw_wgpu_dawn" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")

    target_compile_features(imgui_backend_glfw_wgpu_dawn PRIVATE cxx_std_17)
    target_compile_definitions(imgui_backend_glfw_wgpu_dawn PUBLIC "IMGUI_IMPL_WEBGPU_BACKEND_DAWN")
endif ()

if ("android_opengl3" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_android.h imgui_impl_opengl3.h)
    set(CPP_FILES imgui_impl_android.cpp imgui_impl_opengl3.cpp)
    set(TARGET_LIBS android EGL GLESv3 log)

    # android_opengl3 is a SHARED lib, can't use `add_backend`
    add_library(imgui_backend_android_opengl3 SHARED)
    add_library(imgui::backend_android_opengl3 ALIAS imgui_backend_android_opengl3)

    target_link_options(imgui_backend_android_opengl3 PUBLIC "-uANativeActivity_onCreate")

    target_compile_features(imgui_backend_android_opengl3 PRIVATE cxx_std_11)

    # ImGui sources
    target_sources(imgui_backend_android_opengl3
    PUBLIC
            FILE_SET HEADERS FILES ${HEADER_FILES}
    PRIVATE ${CPP_FILES}
    )

    target_compile_definitions(imgui_backend_android_opengl3 PUBLIC IMGUI_IMPL_OPENGL_ES3)

    target_include_directories(imgui_backend_android_opengl3 PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(imgui_backend_android_opengl3 PUBLIC imgui::imgui)
    target_link_libraries(imgui_backend_android_opengl3 PUBLIC ${TARGET_LIBS})

    install(TARGETS imgui_backend_android_opengl3
            EXPORT imguiTargets
            ARCHIVE DESTINATION lib
            FILE_SET HEADERS DESTINATION include
    )
endif ()

if ("win32_directx9" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_dx9.h imgui_impl_win32.h)
    set(CPP_FILES imgui_impl_dx9.cpp imgui_impl_win32.cpp)
    set(TARGET_LIBS ${IMGUI_DIRECTX_9_X86_LIBRARIES})

    add_backend("win32_directx9" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    target_compile_options(imgui_backend_win32_directx9 PUBLIC /nologo /utf-8 /DUNICODE /D_UNICODE)
    target_link_directories(imgui_backend_win32_directx9 PUBLIC ${IMGUI_DIRECTX_9_X86_LIB_DIR})
endif ()

if ("win32_directx10" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_dx10.h imgui_impl_win32.h)
    set(CPP_FILES imgui_impl_dx10.cpp imgui_impl_win32.cpp)
    set(TARGET_LIBS ${IMGUI_DIRECTX_10_X86_LIBRARIES})

    add_backend("win32_directx10" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    target_compile_options(imgui_backend_win32_directx10 PUBLIC /nologo /utf-8 /DUNICODE /D_UNICODE)
    target_link_directories(imgui_backend_win32_directx10 PUBLIC ${IMGUI_DIRECTX_10_X86_LIB_DIR})
endif ()

if ("win32_directx11" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_dx11.h imgui_impl_win32.h)
    set(CPP_FILES imgui_impl_dx11.cpp imgui_impl_win32.cpp)
    set(TARGET_LIBS ${IMGUI_DIRECTX_11_X86_LIBRARIES})

    add_backend("win32_directx11" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    target_compile_options(imgui_backend_win32_directx11 PUBLIC /nologo /utf-8 /DUNICODE /D_UNICODE)
    target_link_directories(imgui_backend_win32_directx11 PUBLIC ${IMGUI_DIRECTX_11_X86_LIB_DIR})
endif ()

if ("win32_directx12" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_dx12.h imgui_impl_win32.h)
    set(CPP_FILES imgui_impl_dx12.cpp imgui_impl_win32.cpp)
    set(TARGET_LIBS ${IMGUI_DIRECTX_12_X86_LIBRARIES})

    add_backend("win32_directx12" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    target_compile_options(imgui_backend_win32_directx12 PUBLIC /nologo /utf-8 /DUNICODE /D_UNICODE)
    target_link_directories(imgui_backend_win32_directx12 PUBLIC ${IMGUI_DIRECTX_12_X86_LIB_DIR})
endif ()

if ("win64_directx9" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_dx9.h imgui_impl_win32.h)
    set(CPP_FILES imgui_impl_dx9.cpp imgui_impl_win32.cpp)
    set(TARGET_LIBS ${IMGUI_DIRECTX_9_X64_LIBRARIES})

    add_backend("win64_directx9" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    target_compile_options(imgui_backend_win64_directx9 PUBLIC /nologo /utf-8 /DUNICODE /D_UNICODE)
    target_link_directories(imgui_backend_win64_directx9 PUBLIC ${IMGUI_DIRECTX_9_X64_LIB_DIR})
endif ()

if ("win64_directx10" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_dx10.h imgui_impl_win32.h)
    set(CPP_FILES imgui_impl_dx10.cpp imgui_impl_win32.cpp)
    set(TARGET_LIBS ${IMGUI_DIRECTX_10_X64_LIBRARIES})

    add_backend("win64_directx10" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    target_compile_options(imgui_backend_win64_directx10 PUBLIC /nologo /utf-8 /DUNICODE /D_UNICODE)
    target_link_directories(imgui_backend_win64_directx10 PUBLIC ${IMGUI_DIRECTX_10_X64_LIB_DIR})
endif ()

if ("win64_directx11" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_dx11.h imgui_impl_win32.h)
    set(CPP_FILES imgui_impl_dx11.cpp imgui_impl_win32.cpp)
    set(TARGET_LIBS ${IMGUI_DIRECTX_11_X64_LIBRARIES})

    add_backend("win64_directx11" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    target_compile_options(imgui_backend_win64_directx11 PUBLIC /nologo /utf-8 /DUNICODE /D_UNICODE)
    target_link_directories(imgui_backend_win64_directx11 PUBLIC ${IMGUI_DIRECTX_11_X64_LIB_DIR})
endif ()

if ("win64_directx12" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_dx12.h imgui_impl_win32.h)
    set(CPP_FILES imgui_impl_dx12.cpp imgui_impl_win32.cpp)
    set(TARGET_LIBS ${IMGUI_DIRECTX_12_X64_LIBRARIES})

    add_backend("win64_directx12" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
    target_compile_options(imgui_backend_win64_directx12 PUBLIC /nologo /utf-8 /DUNICODE /D_UNICODE)
    target_link_directories(imgui_backend_win64_directx12 PUBLIC ${IMGUI_DIRECTX_12_X64_LIB_DIR})
endif ()

if ("win32_opengl3" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_opengl3.h imgui_impl_win32.h)
    set(CPP_FILES imgui_impl_opengl3.cpp imgui_impl_win32.cpp)
    set(TARGET_LIBS opengl32)

    add_backend("win32_opengl3" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()

if ("win32_vulkan" IN_LIST SUPPORTED_BACKENDS)
    set(HEADER_FILES imgui_impl_vulkan.h imgui_impl_win32.h)
    set(CPP_FILES imgui_impl_vulkan.cpp imgui_impl_win32.cpp)
    set(TARGET_LIBS Vulkan::Vulkan)

    add_backend("win32_vulkan" "${HEADER_FILES}" "${CPP_FILES}" "${TARGET_LIBS}")
endif ()
