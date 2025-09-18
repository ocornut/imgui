add_subdirectory(example_null)
if ("glfw_opengl2" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_glfw_opengl2)
endif ()
if ("glfw_opengl3" IN_LIST SUPPORTED_BACKENDS
    OR "glfw_opengl3_emscripten" IN_LIST SUPPORTED_BACKENDS
)
    add_subdirectory(example_glfw_opengl3)
endif ()
if ("glfw_vulkan" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_glfw_vulkan)
endif ()
if ("sdl2_opengl2" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_sdl2_opengl2)
endif ()
if ("sdl2_opengl3" IN_LIST SUPPORTED_BACKENDS
    OR "sdl2_opengl3_emscripten" IN_LIST SUPPORTED_BACKENDS
)
    add_subdirectory(example_sdl2_opengl3)
endif ()
if ("sdl2_sdlrenderer2" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_sdl2_sdlrenderer2)
endif ()
if ("sdl2_vulkan" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_sdl2_vulkan)
endif ()
if ("sdl3_opengl3" IN_LIST SUPPORTED_BACKENDS
    OR "sdl3_opengl3_emscripten" IN_LIST SUPPORTED_BACKENDS
)
    add_subdirectory(example_sdl3_opengl3)
endif ()
if ("sdl3_sdlgpu3" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_sdl3_sdlgpu3)
endif ()
if ("sdl3_sdlrenderer3" IN_LIST SUPPORTED_BACKENDS
    OR "sdl3_sdlrenderer3_emscripten" IN_LIST SUPPORTED_BACKENDS
)
    add_subdirectory(example_sdl3_sdlrenderer3)
endif ()
if ("sdl3_vulkan" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_sdl3_vulkan)
endif ()
if ("allegro5" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_allegro5)
endif ()
if ("glut_opengl2" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_glut_opengl2)
endif ()
if ("android_opengl3" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_android_opengl3)
endif ()
if ("glfw_wgpu_emscripten" IN_LIST SUPPORTED_BACKENDS
    OR "glfw_wgpu_dawn" IN_LIST SUPPORTED_BACKENDS
)
    add_subdirectory(example_glfw_wgpu)
endif ()
if ("win32_directx9" IN_LIST SUPPORTED_BACKENDS
    OR "win64_directx9" IN_LIST SUPPORTED_BACKENDS
)
    add_subdirectory(example_win32_directx9)
endif ()
if ("win32_directx10" IN_LIST SUPPORTED_BACKENDS
    OR "win64_directx10" IN_LIST SUPPORTED_BACKENDS
)
    add_subdirectory(example_win32_directx10)
endif ()
if ("win32_directx11" IN_LIST SUPPORTED_BACKENDS
    OR "win64_directx11" IN_LIST SUPPORTED_BACKENDS
)
    add_subdirectory(example_win32_directx11)
endif ()
if ("win32_directx12" IN_LIST SUPPORTED_BACKENDS
    OR "win64_directx12" IN_LIST SUPPORTED_BACKENDS
)
    add_subdirectory(example_win32_directx12)
endif ()
if ("win32_opengl3" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_win32_opengl3)
endif ()
if ("win32_vulkan" IN_LIST SUPPORTED_BACKENDS)
    add_subdirectory(example_win32_vulkan)
endif ()
