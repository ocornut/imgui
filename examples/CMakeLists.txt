cmake_minimum_required(VERSION 3.3)

project(ImGui
    VERSION 1.61
    LANGUAGES C CXX)

option(${PROJECT_NAME}_BUILD_IMPLEMENTATION
    "Set to \"OFF\" to disable build Dear ImGui implementation (Default is \"ON\")"
    ON)

option(${PROJECT_NAME}_BUILD_EXAMPLES
    "Set to \"OFF\" to disable build Dear ImGui examples (Default is \"ON\")"
    ON)

if(${PROJECT_NAME}_BUILD_EXAMPLES AND NOT ${PROJECT_NAME}_BUILD_IMPLEMENTATION)
    set(${PROJECT_NAME}_BUILD_IMPLEMENTATION ON)
endif()

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

if(NOT DEFINED CMAKE_DEBUG_POSTFIX)
    set(CMAKE_DEBUG_POSTFIX _d)
endif()

add_library(Library
    ../imgui_internal.h
    ../imgui.cpp
    ../imgui_draw.cpp)
set_target_properties(Library PROPERTIES
    FRAMEWORK TRUE
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER "../imgui.h;../imconfig.h"
    OUTPUT_NAME ${PROJECT_NAME_LOWER})
target_include_directories(Library PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

add_library(Demo
    ../imgui_demo.cpp)
target_link_libraries(Demo PUBLIC Library)
set_target_properties(Demo PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME_LOWER}_demo)

include(GNUInstallDirs)

install(TARGETS Library Demo
    EXPORT ${PROJECT_NAME}Export
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(${PROJECT_NAME}_BUILD_IMPLEMENTATION)

    # Search depend packages

    find_package(OpenGL QUIET)
    find_package(glfw3 QUIET)
    find_package(SDL2 QUIET)


    if(OpenGL_FOUND)
        add_library(ImplOpenGL2
            imgui_impl_opengl2.h
            imgui_impl_opengl2.cpp)
        set_target_properties(ImplOpenGL2 PROPERTIES
            FRAMEWORK TRUE
            VERSION ${PROJECT_VERSION}
            PUBLIC_HEADER imgui_impl_opengl2.h
            OUTPUT_NAME ${PROJECT_NAME_LOWER}_impl_opengl2)
        target_include_directories(ImplOpenGL2 PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
        target_link_libraries(ImplOpenGL2 PUBLIC Library OpenGL::GL)
        install(TARGETS ImplOpenGL2
            EXPORT ${PROJECT_NAME}Export
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()

    if(glfw3_FOUND AND OpenGL_FOUND)
        add_library(ImplGlfw
            imgui_impl_glfw.h
            imgui_impl_glfw.cpp)
        set_target_properties(ImplGlfw PROPERTIES
            FRAMEWORK TRUE
            VERSION ${PROJECT_VERSION}
            PUBLIC_HEADER imgui_impl_glfw.h
            OUTPUT_NAME ${PROJECT_NAME_LOWER}_impl_glfw)
        target_include_directories(ImplGlfw PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
        target_link_libraries(ImplGlfw PUBLIC Library glfw OpenGL::GL)
        install(TARGETS ImplGlfw
            EXPORT ${PROJECT_NAME}Export
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()

    if((SDL2_FOUND AND OpenGL_FOUND))
        add_library(ImplSDL2
            imgui_impl_sdl2.h
            imgui_impl_sdl2.cpp)
        set_target_properties(ImplSDL2 PROPERTIES
            FRAMEWORK TRUE
            VERSION ${PROJECT_VERSION}
            PUBLIC_HEADER imgui_impl_sdl2.h
            OUTPUT_NAME ${PROJECT_NAME_LOWER}_impl_sdl2)
        target_include_directories(ImplSDL2 PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
        target_link_libraries(ImplSDL2 PUBLIC Library SDL2::SDL2 SDL2::SDL2main OpenGL::GL)
        get_target_property(SDL2_INCLUDE_DIR SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
        if(EXISTS ${SDL2_INCLUDE_DIR}/SDL2)
            target_include_directories(ImplSDL2 PUBLIC
                $<BUILD_INTERFACE:${SDL2_INCLUDE_DIR}/SDL2>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SDL2>)
        endif()
        install(TARGETS ImplSDL2
            EXPORT ${PROJECT_NAME}Export
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()
endif()

if(${PROJECT_NAME}_BUILD_EXAMPLES)
    set(${PROJECT_NAME}_DIR ${CMAKE_CURRENT_BINARY_DIR})
    find_package(${PROJECT_NAME} QUIET CONFIG NO_DEFAULT_PATH)
    if(TARGET ImplGlfw AND TARGET ImplOpenGL2 AND ${PROJECT_NAME}_FOUND)
        add_subdirectory(opengl2_example)
        add_dependencies(opengl2_example ImplGlfw ImplOpenGL2 Demo)
    endif()
    if(TARGET ImplSDL2 AND TARGET ImplOpenGL2 AND ${PROJECT_NAME}_FOUND)
        add_subdirectory(sdl_opengl2_example)
        add_dependencies(sdl_opengl2_example ImplSDL2 ImplOpenGL2 Demo)
    endif()
endif()

include(CMakePackageConfigHelpers)

configure_file(ImGuiConfig.cmake.in ${PROJECT_NAME}Config.cmake @ONLY)

write_basic_package_version_file(
  ${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME_LOWER}/cmake)
export(EXPORT ${PROJECT_NAME}Export NAMESPACE ${PROJECT_NAME}:: FILE ${PROJECT_NAME}Targets.cmake)

install(EXPORT ${PROJECT_NAME}Export NAMESPACE ${PROJECT_NAME}:: FILE ${PROJECT_NAME}Targets.cmake
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME_LOWER}/cmake)
