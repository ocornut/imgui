cmake_minimum_required(VERSION 3.18)

project(imgui
        LANGUAGES CXX C
        VERSION 1.92.7)

set(CMAKE_CXX_STANDARD  20)

set(LIBRARY_TARGET_NAME ${PROJECT_NAME})

set(${LIBRARY_TARGET_NAME}_HEADER_FILES
      imgui.h
      imgui_internal.h
      imconfig.h
      imstb_rectpack.h
      imstb_truetype.h
   )

set(${LIBRARY_TARGET_NAME}_SOURCE_FILES
      imgui.cpp
      imgui_demo.cpp
      imgui_draw.cpp
      imgui_tables.cpp
      imgui_widgets.cpp
   )

# set options for backends
option(IMGUI_BACKEND_GLFW    "Use GLFW backend"    ON)
option(IMGUI_BACKEND_GLUT    "Use GLUT backend"    OFF)
option(IMGUI_BACKEND_OPENGL3 "Use OpenGL3 backend" OFF)
option(IMGUI_BACKEND_VULKAN  "Use Vulkan  backend" ON)

set(${LIBRARY_TARGET_NAME}_BACKEND_GLFW_HEADER_FILES
      backends/imgui_impl_glfw.h
   )

set(${LIBRARY_TARGET_NAME}_BACKEND_GLFW_SOURCE_FILES
      backends/imgui_impl_glfw.cpp
   )

set(${LIBRARY_TARGET_NAME}_BACKEND_GLUT_HEADER_FILES
      backends/imgui_impl_glut.h
   )

set(${LIBRARY_TARGET_NAME}_BACKEND_GLUT_SOURCE_FILES
      backends/imgui_impl_glut.cpp
   )

set(${LIBRARY_TARGET_NAME}_BACKEND_OPENGL3_HEADER_FILES
      backends/imgui_impl_opengl3.h
   )

set(${LIBRARY_TARGET_NAME}_BACKEND_OPENGL3_SOURCE_FILES
      backends/imgui_impl_opengl3.cpp
   )

set(${LIBRARY_TARGET_NAME}_BACKEND_VULKAN_HEADER_FILES
      backends/imgui_impl_vulkan.h
   )

set(${LIBRARY_TARGET_NAME}_BACKEND_VULKAN_SOURCE_FILES
      backends/imgui_impl_vulkan.cpp
   )

# conditionally inlcude backends
if(IMGUI_BACKEND_GLFW)
    find_package(glfw3 CONFIG REQUIRED)

    list(APPEND ${LIBRARY_TARGET_NAME}_HEADER_FILES ${${LIBRARY_TARGET_NAME}_BACKEND_GLFW_HEADER_FILES})
    list(APPEND ${LIBRARY_TARGET_NAME}_SOURCE_FILES ${${LIBRARY_TARGET_NAME}_BACKEND_GLFW_SOURCE_FILES})
    list(APPEND ${LIBRARY_TARGET_NAME}_LINK_LIBRARIES glfw)
endif()

if(IMGUI_BACKEND_GLUT)
    list(APPEND ${LIBRARY_TARGET_NAME}_HEADER_FILES ${${LIBRARY_TARGET_NAME}_BACKEND_GLUT_HEADER_FILES})
    list(APPEND ${LIBRARY_TARGET_NAME}_SOURCE_FILES ${${LIBRARY_TARGET_NAME}_BACKEND_GLUT_SOURCE_FILES})
endif()

if(IMGUI_BACKEND_OPENGL3)
    list(APPEND ${LIBRARY_TARGET_NAME}_HEADER_FILES ${${LIBRARY_TARGET_NAME}_BACKEND_OPENGL3_HEADER_FILES})
    list(APPEND ${LIBRARY_TARGET_NAME}_SOURCE_FILES ${${LIBRARY_TARGET_NAME}_BACKEND_OPENGL3_SOURCE_FILES})
endif()

if(IMGUI_BACKEND_VULKAN)
    find_package(Vulkan REQUIRED)

    list(APPEND ${LIBRARY_TARGET_NAME}_HEADER_FILES ${${LIBRARY_TARGET_NAME}_BACKEND_VULKAN_HEADER_FILES})
    list(APPEND ${LIBRARY_TARGET_NAME}_SOURCE_FILES ${${LIBRARY_TARGET_NAME}_BACKEND_VULKAN_SOURCE_FILES})
    list(APPEND ${LIBRARY_TARGET_NAME}_LINK_LIBRARIES Vulkan::Vulkan)
endif()

add_library(
  ${LIBRARY_TARGET_NAME}
  ${${LIBRARY_TARGET_NAME}_SOURCE_FILES}
  ${${LIBRARY_TARGET_NAME}_HEADER_FILES})

add_library(
  ${PROJECT_NAME}::${LIBRARY_TARGET_NAME}
  ALIAS ${LIBRARY_TARGET_NAME})

set_target_properties(${LIBRARY_TARGET_NAME}
  PROPERTIES
    VERSION "${${PROJECT_NAME}_VERSION}"
    PUBLIC_HEADER "${${LIBRARY_TARGET_NAME}_HEADER_FILES}")

# check if the CONDA_PREFIX environment variable is defined
if(DEFINED ENV{CONDA_PREFIX})
    message(STATUS "Conda environment detected. Prefix: $ENV{CONDA_PREFIX}")
    set(CONDA_FOUND 1)
else()
    message(STATUS "No active Conda environment detected.")
    set(CONDA_FOUND 0)
endif()

# specify include directories for both compilation and installation process.
target_include_directories(${LIBRARY_TARGET_NAME}
  PUBLIC
    # This path is included if the condition "CONDA_FOUND" is true (1)
    # The $<...: ...> syntax is a generator expression
    $<IF:${CONDA_FOUND},$<BUILD_INTERFACE:$ENV{CONDA_PREFIX}/include>,>
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/backends>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

# specify target link libraries.
target_link_libraries(${LIBRARY_TARGET_NAME}
  PRIVATE
    ${${LIBRARY_TARGET_NAME}_LINK_LIBRARIES})

# specify default install location
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Specify install location." FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# defines CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_BINDIR and many other useful macros.
# see https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
include(GNUInstallDirs)

# specify installation targets, typology and destination folders.
install(TARGETS ${LIBRARY_TARGET_NAME}
        EXPORT  ${LIBRARY_TARGET_NAME}   # associates this target with an export name
        LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}"                            COMPONENT shlib
        ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}"                            COMPONENT lib
        RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}"                            COMPONENT bin
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${LIBRARY_TARGET_NAME}" COMPONENT dev)

# specify export configuration to make the library discoverable by other cmake project using find_package()
install(EXPORT ${LIBRARY_TARGET_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    NAMESPACE   ${PROJECT_NAME}::
)

message(STATUS "Created target ${LIBRARY_TARGET_NAME} for ${PROJECT_NAME}.")
