cmake_minimum_required(VERSION 3.5)

# Fetching version from header file
file(STRINGS ../imgui.h ImGui_VERSION_NUM_HEADER_STRING
    REGEX "#define[ \t]+IMGUI_VERSION_NUM[ \t]+([0-9]+)"
    LIMIT_COUNT 1)
string(REGEX REPLACE "#define[ \t]+IMGUI_VERSION_NUM[ \t]+([0-9]+)" "\\1"
    IMGUI_VERSION_NUM "${ImGui_VERSION_NUM_HEADER_STRING}")
math(EXPR IMGUI_VERSION_MAJOR "${IMGUI_VERSION_NUM} / 10000")
math(EXPR IMGUI_VERSION_MINOR "(${IMGUI_VERSION_NUM} % 10000) / 100")
math(EXPR IMGUI_VERSION_PATCH "${IMGUI_VERSION_NUM} % 100")

project(imgui_examples
    VERSION "${IMGUI_VERSION_MAJOR}.${IMGUI_VERSION_MINOR}.${IMGUI_VERSION_PATCH}"
    LANGUAGES CXX)

get_filename_component(ImGui_SRCDIR "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)

include("${CMAKE_CURRENT_LIST_DIR}/ImGuiModule.cmake")

set(ImGui_OPTIONS)

imgui_option(USER_CONFIG "Dear ImGui user config for include" "" STRING)
imgui_option(EXAMPLES    "Dear ImGui example applications" ON)
imgui_option(BACKENDS    "Dear ImGui platform and render backends" ON)
imgui_option(MISC        "Dear ImGui misc features" ON)
imgui_option(3RDPARTY    "Dear ImGui example dependencies" ON)
imgui_option(OPENGL_LOADER
    "Dear ImGui OpenGL loader (IMGL3W, GL3W, GLEW, GLAD or CUSTOM)"
    "IMGL3W"
    STRINGS "IMGL3W" "GL3W" "GLEW" "GLAD" "CUSTOM")
imgui_option(FREETYPE    "Dear ImGui will build font atlases using FreeType instead of stb_truetype" OFF)
imgui_option(TOOLS       "Dear ImGui auxiliary applications" OFF)
imgui_option(PACKAGE     "Dear ImGui packaging" OFF)

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ImGuiOptions.cmake"
    CONTENT "${ImGui_OPTIONS_CMAKE}")

include("${CMAKE_CURRENT_LIST_DIR}/ImGuiTargets.cmake")

set(ImGui_DIR "${CMAKE_CURRENT_LIST_DIR}")

imgui_example(example_null
    TARGETS Core)

imgui_example(example_sdl3_sdlrenderer3
    BACKENDS ImplSDL3 ImplSDLRenderer3)

imgui_example(example_sdl3_opengl3
    BACKENDS ImplSDL3 ImplOpenGL3)

imgui_example(example_glfw_opengl3
    BACKENDS ImplGlfw ImplOpenGL3)

if(NOT EMSCRIPTEN)
    imgui_example(example_glut_opengl2
        BACKENDS ImplGLUT ImplOpenGL2)

    imgui_example(example_sdl2_sdlrenderer2
        BACKENDS ImplSDL2 ImplSDLRenderer2)

    imgui_example(example_sdl2_opengl2
        BACKENDS ImplSDL2 ImplOpenGL2)

    imgui_example(example_sdl2_opengl3
        BACKENDS ImplSDL2 ImplOpenGL3)

    imgui_example(example_sdl2_vulkan
        BACKENDS ImplSDL2 ImplVulkan)

    imgui_example(example_sdl3_vulkan
        BACKENDS ImplSDL3 ImplVulkan)

    imgui_example(example_glfw_opengl2
        BACKENDS ImplGlfw ImplOpenGL2)

    imgui_example(example_glfw_vulkan
        BACKENDS ImplGlfw ImplVulkan)
endif()

if(NOT "${CMAKE_CURRENT_LIST_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    foreach(FILE ImGuiConfig.cmake ImGuiModule.cmake ImGuiTargets.cmake)
        configure_file(${FILE} ${FILE} COPYONLY)
    endforeach()
endif()

install(FILES ImGuiConfig.cmake ImGuiModule.cmake ImGuiTargets.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/ImGuiOptions.cmake"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/imgui)

if(ImGui_PACKAGE)
    if(NOT DEFINED CPACK_PACKAGE_NAME)
        set(CPACK_PACKAGE_NAME "dear-imgui")
    endif()
    include(CPack)
endif()
