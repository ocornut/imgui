include("${CMAKE_CURRENT_LIST_DIR}/ImGuiModule.cmake")

set(ImGui_SUPPORTED_COMPONENTS)
set(ImGui_AVAILABLE_COMPONENTS)

set(_ImGui_DEFINITIONS)
set(_ImGui_LIBRARIES)
if(NOT "${ImGui_USER_CONFIG}" STREQUAL "")
    list(APPEND _ImGui_DEFINITIONS "IMGUI_USER_CONFIG=${ImGui_USER_CONFIG}")
endif()
if(ImGui_FREETYPE)
    list(APPEND _ImGui_DEFINITIONS "IMGUI_ENABLE_FREETYPE")
    list(APPEND _ImGui_LIBRARIES FreeType)
endif()
if(MINGW)
    list(APPEND _ImGui_LIBRARIES "imm32")
endif()
imgui_core(
    HEADERS
        imconfig.h
        imgui.h
        imgui_internal.h
    PRIVATE_HEADERS
        imstb_textedit.h
        imstb_truetype.h
        imstb_rectpack.h
    SOURCES_GLOB
        imgui*.cpp
    DEFINITIONS ${_ImGui_DEFINITIONS}
    LIBRARIES ${_ImGui_LIBRARIES})
unset(_ImGui_DEFINITIONS)
unset(_ImGui_LIBRARIES)

if(APPLE)
    set(_ImGui_PACKAGES GLUT)
    set(_ImGui_TARGETS GLUT::GLUT)
else()
    set(_ImGui_PACKAGES FreeGLUT)
    set(_ImGui_TARGETS FreeGLUT::freeglut)
endif()
imgui_backend(ImplGLUT
    HEADERS imgui_impl_glut.h
    SOURCES imgui_impl_glut.cpp
    PACKAGES ${_ImGui_PACKAGES}
    TARGETS ${_ImGui_TARGETS})
unset(_ImGui_PACKAGES)
unset(_ImGui_TARGETS)

if(EMSCRIPTEN)
    set(_ImGui_COMPILE_OPTIONS "SHELL:-s USE_SDL=2")
    set(_ImGui_LINK_OPTIONS ${_ImGui_COMPILE_OPTIONS})
else()
    set(_ImGui_PACKAGES SDL2)
    set(_ImGui_TARGETS SDL2::SDL2main SDL2::SDL2)
endif()

imgui_backend(ImplSDL2
    HEADERS imgui_impl_sdl2.h
    SOURCES imgui_impl_sdl2.cpp
    COMPILE_OPTIONS ${_ImGui_COMPILE_OPTIONS}
    LINK_OPTIONS ${_ImGui_LINK_OPTIONS}
    PACKAGES ${_ImGui_PACKAGES}
    TARGETS ${_ImGui_TARGETS})

if(NOT EMSCRIPTEN)
    set(_ImGui_TARGETS SDL2::SDL2)
endif()

imgui_backend(ImplSDLRenderer2
    HEADERS imgui_impl_sdlrenderer2.h
    SOURCES imgui_impl_sdlrenderer2.cpp
    COMPILE_OPTIONS ${_ImGui_COMPILE_OPTIONS}
    LINK_OPTIONS ${_ImGui_LINK_OPTIONS}
    PACKAGES ${_ImGui_PACKAGES}
    TARGETS ${_ImGui_TARGETS})

unset(_ImGui_COMPILE_OPTIONS)
unset(_ImGui_LINK_OPTIONS)
unset(_ImGui_PACKAGES)
unset(_ImGui_TARGETS)

imgui_backend(ImplSDL3
    HEADERS imgui_impl_sdl3.h
    SOURCES imgui_impl_sdl3.cpp
    PACKAGES SDL3
    TARGETS SDL3::SDL3)

imgui_backend(ImplSDLRenderer3
    HEADERS imgui_impl_sdlrenderer3.h
    SOURCES imgui_impl_sdlrenderer3.cpp
    PACKAGES SDL3
    TARGETS SDL3::SDL3)

if(EMSCRIPTEN)
    set(_ImGui_COMPILE_OPTIONS "SHELL:-s USE_GLFW=3")
    set(_ImGui_LINK_OPTIONS ${_ImGui_COMPILE_OPTIONS})
else()
    set(_ImGui_PACKAGES glfw3)
    set(_ImGui_TARGETS glfw)
endif()
imgui_backend(ImplGlfw
    HEADERS imgui_impl_glfw.h
    SOURCES imgui_impl_glfw.cpp
    COMPILE_OPTIONS ${_ImGui_COMPILE_OPTIONS}
    LINK_OPTIONS ${_ImGui_LINK_OPTIONS}
    PACKAGES ${_ImGui_PACKAGES}
    TARGETS ${_ImGui_TARGETS})
unset(_ImGui_COMPILE_OPTIONS)
unset(_ImGui_LINK_OPTIONS)
unset(_ImGui_PACKAGES)
unset(_ImGui_TARGETS)

imgui_backend(ImplOpenGL2
    HEADERS imgui_impl_opengl2.h
    SOURCES imgui_impl_opengl2.cpp
    PACKAGES OpenGL
    TARGETS OpenGL::GL)

if("${ImGui_OPENGL_LOADER}" STREQUAL "GL3W")
    set(_ImGui_DEFINITIONS "IMGUI_IMPL_OPENGL_LOADER_GL3W=1")
    set(_ImGui_PACKAGES gl3w)
    set(_ImGui_TARGETS gl3w)
elseif("${ImGui_OPENGL_LOADER}" STREQUAL "GLEW")
    set(_ImGui_DEFINITIONS "IMGUI_IMPL_OPENGL_LOADER_GLEW=1")
    set(_ImGui_PACKAGES GLEW)
    set(_ImGui_TARGETS GLEW::GLEW)
elseif("${ImGui_OPENGL_LOADER}" STREQUAL "GLAD")
    set(_ImGui_DEFINITIONS "IMGUI_IMPL_OPENGL_LOADER_GLAD=1")
    set(_ImGui_PACKAGES glad)
    set(_ImGui_TARGETS glad::glad)
elseif("${ImGui_OPENGL_LOADER}" STREQUAL "CUSTOM")
    set(_ImGui_DEFINITIONS "IMGUI_IMPL_OPENGL_LOADER_CUSTOM=1")
    set(_ImGui_PACKAGES)
    set(_ImGui_TARGETS)
endif()
if(EMSCRIPTEN)
    set(_ImGui_PACKAGES)
    set(_ImGui_TARGETS)
else()
    list(APPEND _ImGui_PACKAGES OpenGL)
    list(APPEND _ImGui_TARGETS OpenGL::GL)
endif()
imgui_backend(ImplOpenGL3
    HEADERS
        imgui_impl_opengl3.h
        imgui_impl_opengl3_loader.h
    SOURCES imgui_impl_opengl3.cpp
    DEFINITIONS ${_ImGui_DEFINITIONS}
    PACKAGES ${_ImGui_PACKAGES}
    TARGETS ${_ImGui_TARGETS})
unset(_ImGui_DEFINITIONS)
unset(_ImGui_PACKAGES)
unset(_ImGui_TARGETS)

imgui_backend(ImplVulkan
    HEADERS imgui_impl_vulkan.h
    SOURCES imgui_impl_vulkan.cpp
    PACKAGES Vulkan
    TARGETS Vulkan::Vulkan)

imgui_misc(FreeType freetype
    HEADERS imgui_freetype.h
    SOURCES imgui_freetype.cpp
    PACKAGES Freetype
    TARGETS Freetype::Freetype)

imgui_misc(StdLib cpp
    HEADERS imgui_stdlib.h
    SOURCES imgui_stdlib.cpp)

imgui_tool(BinaryToCompressedC
    OUTPUT_NAME binary_to_compressed_c
    SOURCES misc/fonts/binary_to_compressed_c.cpp)
