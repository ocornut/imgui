# dear imgui: build and test the C++20 named modules 'imgui' (imgui.cppm) and 'imgui.impl.<backend>' (backends/imgui_impl_<backend>.cppm).
# Requires CMake with non-experimental 'import std' support (4.5, or a 4.4 development build), a Ninja generator, and Clang.
# Tested with CMake 4.4-dev + Clang 21 on Linux (libstdc++ 15), Android (NDK r29, libc++), Windows (MinGW-w64 libstdc++ 15, clang-cl + MSVC STL 14.44).
# Usage (see CMakePresets.json):
#   cmake --workflow --preset clang
# Backends are enabled when their platform library is found, e.g. on Debian/Ubuntu:
#   apt install libopengl-dev libglut-dev libsdl2-dev libsdl3-dev libglfw3-dev libvulkan-dev liballegro5-dev
# WebGPU has no system package: pass its headers with -DIMGUI_WEBGPU_INCLUDE_DIRS=<...> (compile-only test).
# Tests:
# - Every test is built against the module ('import imgui;') and, where relevant, against the headers ('#include "imgui.h"'):
#   'DIFF' tests run both builds with the same scripted input and check that they render the same draw data and textures.
# - The core library, the null backend and their tests are built for each compile-time configuration (imgui_add_config() below).

cmake_minimum_required(VERSION 3.30)
project(imgui_cxx_module LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Dear ImGui root directory")
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(IMGUI_WARNINGS /W4 /WX -Wno-reserved-module-identifier              # clang-cl: /W4 = -Wall -Wextra (its -Wall = -Weverything)
        -Wno-include-angled-in-module-purview                               # MSVC STL's std.ixx includes its headers after 'export module std;'
        -Wno-unused-command-line-argument                                   # CMake passes both -TP and '-x c++-module' for module units
        /D_CRT_SECURE_NO_WARNINGS)                                          # imgui sources #define it before CRT headers, but modules get those from 'std'
else()
    set(IMGUI_WARNINGS -Wall -Wextra -Werror -Wno-reserved-module-identifier)
endif()                                                                     # the synthesized 'std' targets inherit these flags
if(ANDROID)
    # bionic defines some C library functions 'static inline' (ctype.h, FORTIFY wrappers), which libc++'s 'std' module can't export.
    # (options, not definitions: the synthesized 'std' targets only inherit options)
    add_compile_options(-D__BIONIC_CTYPE_INLINE=inline -U_FORTIFY_SOURCE)
endif()

enable_testing()

#-----------------------------------------------------------------------------
# Helpers
#-----------------------------------------------------------------------------
# Targets are suffixed with ${IMGUI_SFX}: empty for the default configuration, '_<config>' otherwise (see imgui_add_config()).

# imgui_add_backend(<name> <libs...> [OPTIONS <compile options...>])
# - imgui_impl_<name>: module 'imgui.impl.<name>'
# - imgui_header_impl_<name>: regular (header) build of the backend, to check it's unaffected by the module support
function(imgui_add_backend name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "OPTIONS")
    set(module imgui_impl_${name}${IMGUI_SFX})
    add_library(${module} STATIC)
    target_sources(${module} PUBLIC FILE_SET CXX_MODULES BASE_DIRS "${IMGUI_DIR}" FILES "${IMGUI_DIR}/backends/imgui_impl_${name}.cppm")
    target_link_libraries(${module} PUBLIC imgui${IMGUI_SFX} ${arg_UNPARSED_ARGUMENTS})
    target_compile_options(${module} PRIVATE ${IMGUI_WARNINGS} ${arg_OPTIONS})
    set(header imgui_header_impl_${name}${IMGUI_SFX})
    add_library(${header} STATIC "${IMGUI_DIR}/backends/imgui_impl_${name}.cpp")
    target_include_directories(${header} PUBLIC "${IMGUI_DIR}/backends")
    target_link_libraries(${header} PUBLIC imgui_header${IMGUI_SFX} ${arg_UNPARSED_ARGUMENTS})
    target_compile_options(${header} PRIVATE ${IMGUI_WARNINGS} ${arg_OPTIONS})
    set(IMGUI_BACKENDS ${IMGUI_BACKENDS} ${name} PARENT_SCOPE)
endfunction()

# imgui_add_test(<name> RUN|DIFF|LINK|COMPILE <sources...> LIBS <targets...>)
# - RUN: compile, link, and run (natively or with CMAKE_CROSSCOMPILING_EMULATOR); LINK: compile and link; COMPILE: compile only.
# - DIFF: like RUN, and also build <sources> in header mode (IMGUI_TEST_HEADER defined, linking the 'imgui_header*' equivalent
#   of each LIBS target), run both, and check that their outputs match.
# - Skipped if any of the LIBS targets doesn't exist (backend not found).
function(imgui_add_test name kind)
    cmake_parse_arguments(PARSE_ARGV 2 arg "" "" "LIBS")
    foreach(lib IN LISTS arg_LIBS)
        if(NOT TARGET ${lib})
            return()
        endif()
    endforeach()
    set(name ${name}${IMGUI_SFX})
    if(kind STREQUAL "COMPILE")
        add_library(${name} OBJECT ${arg_UNPARSED_ARGUMENTS})
    else()
        add_executable(${name} ${arg_UNPARSED_ARGUMENTS})
    endif()
    target_link_libraries(${name} PRIVATE ${arg_LIBS})
    target_compile_options(${name} PRIVATE ${IMGUI_WARNINGS})
    if(kind STREQUAL "DIFF")
        list(TRANSFORM arg_LIBS REPLACE "^imgui" "imgui_header")
        add_executable(${name}_header ${arg_UNPARSED_ARGUMENTS})
        target_link_libraries(${name}_header PRIVATE ${arg_LIBS})
        target_compile_definitions(${name}_header PRIVATE IMGUI_TEST_HEADER)
        target_compile_options(${name}_header PRIVATE ${IMGUI_WARNINGS})
    endif()
    if(NOT CMAKE_CROSSCOMPILING OR CMAKE_CROSSCOMPILING_EMULATOR)
        if(kind STREQUAL "RUN")
            add_test(NAME ${name} COMMAND ${name})
        elseif(kind STREQUAL "DIFF")
            add_test(NAME ${name} COMMAND "${CMAKE_COMMAND}" "-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
                "-DEXPECTED=$<TARGET_FILE:${name}_header>" "-DACTUAL=$<TARGET_FILE:${name}>" -P "${CMAKE_CURRENT_SOURCE_DIR}/compare_output.cmake")
        endif()
    endif()
endfunction()

# imgui_add_config(<name> [LIBRARIES_ONLY] <compile definitions...>)
# Builds the 'imgui' module, its header-mode equivalent, the null backend, and (unless LIBRARIES_ONLY) the core tests
# with a compile-time configuration. Definitions are PUBLIC: importers must use the same configuration (see imgui_macros.h).
function(imgui_add_config cfg)
    cmake_parse_arguments(PARSE_ARGV 1 arg "LIBRARIES_ONLY" "" "")
    if(NOT cfg STREQUAL "default")
        set(IMGUI_SFX _${cfg})
    endif()

    # Module 'imgui'
    add_library(imgui${IMGUI_SFX} STATIC)
    target_sources(imgui${IMGUI_SFX} PUBLIC FILE_SET CXX_MODULES BASE_DIRS "${IMGUI_DIR}" FILES "${IMGUI_DIR}/imgui.cppm")
    target_include_directories(imgui${IMGUI_SFX} PUBLIC "${IMGUI_DIR}")    # imgui_macros.h, imconfig.h
    target_compile_definitions(imgui${IMGUI_SFX} PUBLIC ${arg_UNPARSED_ARGUMENTS})
    target_compile_options(imgui${IMGUI_SFX} PRIVATE ${IMGUI_WARNINGS})

    # Regular (header) build of the same sources, to check it's unaffected by the module support
    add_library(imgui_header${IMGUI_SFX} STATIC
        "${IMGUI_DIR}/imgui.cpp" "${IMGUI_DIR}/imgui_demo.cpp" "${IMGUI_DIR}/imgui_draw.cpp" "${IMGUI_DIR}/imgui_tables.cpp" "${IMGUI_DIR}/imgui_widgets.cpp")
    target_include_directories(imgui_header${IMGUI_SFX} PUBLIC "${IMGUI_DIR}")
    target_compile_definitions(imgui_header${IMGUI_SFX} PUBLIC ${arg_UNPARSED_ARGUMENTS})
    target_compile_options(imgui_header${IMGUI_SFX} PRIVATE ${IMGUI_WARNINGS})

    imgui_add_backend(null)
    if(arg_LIBRARIES_ONLY)
        return()
    endif()

    imgui_add_test(test_module              RUN     test_module.cpp test_module_macros.cpp  LIBS imgui${IMGUI_SFX})
    imgui_add_test(test_header_internal     RUN     test_header_internal.cpp                LIBS imgui_header${IMGUI_SFX})
    imgui_add_test(test_backend_null        DIFF    test_backend_null.cpp                   LIBS imgui_impl_null${IMGUI_SFX})
endfunction()

#-----------------------------------------------------------------------------
# Compile-time configurations (same as the example_null builds of .github/workflows/build.yml)
#-----------------------------------------------------------------------------

imgui_add_config(default)
imgui_add_config(no_obsolete        IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
imgui_add_config(wchar32            IMGUI_USE_WCHAR32)
imgui_add_config(large_idx          "ImDrawIdx=unsigned int" "ImTextureID=void*")
imgui_add_config(no_demo_debug      IMGUI_DISABLE_DEMO_WINDOWS IMGUI_DISABLE_DEBUG_TOOLS)
imgui_add_config(no_file            IMGUI_DISABLE_FILE_FUNCTIONS)
imgui_add_config(bgra               IMGUI_USE_BGRA_PACKED_COLOR)
imgui_add_config(math_operators     IMGUI_DEFINE_MATH_OPERATORS)
imgui_add_config(disable            LIBRARIES_ONLY IMGUI_DISABLE)

#-----------------------------------------------------------------------------
# Modules 'imgui.impl.<backend>' (default configuration)
#-----------------------------------------------------------------------------

if(WIN32)
    imgui_add_backend(win32 gdi32 dwmapi)
    imgui_add_backend(dx9 d3d9)
    imgui_add_backend(dx10 d3d10 d3dcompiler)
    imgui_add_backend(dx11 d3d11 d3dcompiler)
    imgui_add_backend(dx12 d3d12 dxgi d3dcompiler OPTIONS -Wno-unused-but-set-variable)  # upstream warning with NDEBUG (header build too): 'hr' only used by IM_ASSERT()
endif()

if(ANDROID)
    imgui_add_backend(android android log OPTIONS -Wno-unused-variable)  # upstream warning (header build too): unused 'g_LogTag'
    imgui_add_backend(opengl3 GLESv3)               # OpenGL ES 3 is selected automatically on Android
else()
    find_package(OpenGL)
    if(TARGET OpenGL::GL)
        imgui_add_backend(opengl2 OpenGL::GL)
        if(WIN32)
            imgui_add_backend(opengl3 OpenGL::GL)
        else()
            imgui_add_backend(opengl3 ${CMAKE_DL_LIBS}) # uses its own loader (imgui_impl_opengl3_loader.h)
        endif()
    endif()
    find_package(GLUT)
    if(TARGET GLUT::GLUT AND TARGET OpenGL::GL)
        imgui_add_backend(glut GLUT::GLUT OpenGL::GL)
    endif()
endif()

find_package(SDL2 CONFIG)
if(TARGET SDL2::SDL2)
    imgui_add_backend(sdl2 SDL2::SDL2)
    imgui_add_backend(sdlrenderer2 SDL2::SDL2)
endif()

find_package(SDL3 CONFIG)
if(TARGET SDL3::SDL3)
    imgui_add_backend(sdl3 SDL3::SDL3)
    imgui_add_backend(sdlrenderer3 SDL3::SDL3)
    imgui_add_backend(sdlgpu3 SDL3::SDL3)
endif()

find_package(glfw3 CONFIG)
if(TARGET glfw)
    imgui_add_backend(glfw glfw)
endif()

find_package(Vulkan)
if(TARGET Vulkan::Vulkan)
    imgui_add_backend(vulkan Vulkan::Vulkan)
endif()

if(NOT CMAKE_CROSSCOMPILING)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(ALLEGRO5 IMPORTED_TARGET allegro-5 allegro_primitives-5)
    endif()
    if(TARGET PkgConfig::ALLEGRO5)
        imgui_add_backend(allegro5 PkgConfig::ALLEGRO5 OPTIONS -Wno-unused-variable)  # upstream warning (header build too): unused 'platform_io'
    endif()
endif()

set(IMGUI_WEBGPU_INCLUDE_DIRS "" CACHE STRING "WebGPU include directories providing <webgpu/webgpu.h> (enables the compile-only 'wgpu' backend)")
set(IMGUI_WEBGPU_BACKEND "DAWN" CACHE STRING "WebGPU implementation: DAWN or WGPU")
if(IMGUI_WEBGPU_INCLUDE_DIRS)
    add_library(imgui_webgpu_headers INTERFACE)
    target_include_directories(imgui_webgpu_headers SYSTEM INTERFACE ${IMGUI_WEBGPU_INCLUDE_DIRS})
    target_compile_definitions(imgui_webgpu_headers INTERFACE IMGUI_IMPL_WEBGPU_BACKEND_${IMGUI_WEBGPU_BACKEND})
    imgui_add_backend(wgpu imgui_webgpu_headers)
endif()

message(STATUS "imgui backend modules: ${IMGUI_BACKENDS}")

#-----------------------------------------------------------------------------
# Backend tests (default configuration)
#-----------------------------------------------------------------------------

imgui_add_test(test_backend_sdl2        DIFF    test_backend_sdl2.cpp                   LIBS imgui_impl_sdl2 imgui_impl_sdlrenderer2)
imgui_add_test(test_backend_sdl3        DIFF    test_backend_sdl3.cpp                   LIBS imgui_impl_sdl3 imgui_impl_sdlrenderer3 imgui_impl_sdlgpu3)
# Backends that need a GPU/display to run
imgui_add_test(test_backend_gpu         LINK    test_backend_gpu.cpp                    LIBS imgui_impl_opengl2 imgui_impl_opengl3 imgui_impl_glut imgui_impl_glfw imgui_impl_vulkan)
imgui_add_test(test_backend_allegro5    LINK    test_backend_allegro5.cpp               LIBS imgui_impl_allegro5)
imgui_add_test(test_backend_windows     LINK    test_backend_windows.cpp                LIBS imgui_impl_win32 imgui_impl_dx9 imgui_impl_dx10 imgui_impl_dx11 imgui_impl_dx12 imgui_impl_opengl2 imgui_impl_opengl3)
imgui_add_test(test_backend_android     LINK    test_backend_android.cpp                LIBS imgui_impl_android imgui_impl_opengl3 imgui_impl_vulkan)
imgui_add_test(test_backend_wgpu        COMPILE test_backend_wgpu.cpp                   LIBS imgui_impl_wgpu)  # headers only, nothing to link
