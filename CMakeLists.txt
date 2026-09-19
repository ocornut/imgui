cmake_minimum_required(VERSION 3.1)

project(imgui)
set(IMGUI_VERSION "1.62")

option(IMGUI_BUILD_EXAMPLES "Build imgui examples" OFF)

# Introduce variables:
# * CMAKE_INSTALL_LIBDIR
# * CMAKE_INSTALL_BINDIR
# * CMAKE_INSTALL_INCLUDEDIR
include(GNUInstallDirs)
set(IMGUI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(IMGUI_HEADERS
    imconfig.h
    imgui.h
    imgui_internal.h
    stb_rect_pack.h
    stb_textedit.h
    stb_truetype.h
)

set(IMGUI_SOURCE
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
)

add_library(imgui INTERFACE)

target_include_directories(imgui INTERFACE 
    $<BUILD_INTERFACE:${IMGUI_SOURCE_DIR}>)

set(IMGUI_BUILD_SOURCE)
set(IMGUI_INTERFACE_SOURCE)
foreach(SOURCE_FILE ${IMGUI_SOURCE})
    set(IMGUI_BUILD_SOURCE ${IMGUI_BUILD_SOURCE} ${IMGUI_SOURCE_DIR}/${SOURCE_FILE})
    set(IMGUI_INTERFACE_SOURCE ${IMGUI_INTERFACE_SOURCE} ${CMAKE_INSTALL_INCLUDEDIR}/${SOURCE_FILE})
endforeach()

target_sources(imgui INTERFACE 
    $<BUILD_INTERFACE:${IMGUI_BUILD_SOURCE}>
    $<INSTALL_INTERFACE:${IMGUI_INTERFACE_SOURCE}>
)

if(IMGUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

#Install imgui

#install config path
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(VERSION_CONFIG "${GENERATED_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(PROJECT_CONFIG "${GENERATED_DIR}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(NAMESPACE "${PROJECT_NAME}::")

#install header files
install(FILES ${IMGUI_HEADERS} 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${IMGUI_SOURCE} 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#build and install project config
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "cmake/Config.cmake.in"
    "${PROJECT_CONFIG}"
    INSTALL_DESTINATION "${INSTALL_CONFIGDIR}"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

#build config version into bin directory
write_basic_package_version_file(
    "${VERSION_CONFIG}"
    VERSION ${IMGUI_VERSION}
    COMPATIBILITY AnyNewerVersion
    
)

#install target
install(
    TARGETS imgui
    EXPORT "${TARGETS_EXPORT_NAME}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

#install files
install(
    FILES 
        "${VERSION_CONFIG}"
        "${PROJECT_CONFIG}"
    DESTINATION ${INSTALL_CONFIGDIR}
)

install(
    EXPORT "${TARGETS_EXPORT_NAME}"
    NAMESPACE "${NAMESPACE}"
    DESTINATION "${INSTALL_CONFIGDIR}"
)
