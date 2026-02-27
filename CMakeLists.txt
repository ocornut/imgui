cmake_minimum_required(VERSION 3.25)
project(imgui VERSION 1.90.8 LANGUAGES CXX)
set(imgui_VERSION "${imgui_VERSION}-WIP")
if(IMGUI_COMMIT_HASH)
  set(imgui_VERSION "${imgui_VERSION}-g${IMGUI_COMMIT_HASH}")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_DEBUG_POSTFIX  d)
set(CMAKE_CXX_STANDARD  20)
set(CMAKE_CXX_STANDARD_REQUIRED)

option(IMGUI_BUILD_EXAMPLES "Build ImGUI Examples" OFF)
option(BUILD_SHARED_LIBS "Build shared library by default" ON)
option(CMAKE_POSITION_INDEPENDENT_CODE "Enable PIC for all" ON)
option(CMAKE_VERBOSE_MAKEFILE "Enable Makefile verbose output" OFF)
option(CMAKE_EXPORT_COMPILE_COMMANDS "Export compile cmds as JSON" OFF)

option(IMGUI_IMPL_COMBINE_STATIC "Combine static IMPL into libimgui.a" ON)

include(CMakeDependentOption)
# TODO Add more graphical backends
# NOTE Linux show current OpenGL version: $ glxinfo | grep version
# NOTE set <CMAKE_PREFIX_PATH> for none standard install of SDL3, GLFW, etc.
cmake_dependent_option(IMGUI_BUILD_IMPL_directx "Build GB/IMPL DirectX" ON   "WIN32"  OFF)
cmake_dependent_option(IMGUI_BUILD_IMPL_vulkan  "Build GB/IMPL Vulkan"  OFF  "UNIX"   OFF)
cmake_dependent_option(IMGUI_BUILD_IMPL_opengl3 "Build GB/IMPL OpenGL"  ON   "UNIX"   OFF)

cmake_dependent_option(IMGUI_BUILD_IMPL_sdl3    "Build GB/IMPL SDL"     ON   "UNIX"   OFF)
cmake_dependent_option(IMGUI_BUILD_IMPL_glfw    "Build GB/IMPL GLFW"    ON   "UNIX"   OFF)

# https://cmake.org/cmake/help/latest/module/FindOpenGL.html
if(IMGUI_BUILD_EXAMPLES AND IMGUI_BUILD_IMPL_opengl3)
  find_package(OpenGL REQUIRED) # apt show libopengl-dev
  set(IMGUI_IMPL_API_opengl3 ${CMAKE_SOURCE_DIR}/backends/imgui_impl_opengl3.h)
  set(IMGUI_IMPL_SRC_opengl3 ${CMAKE_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
endif()

if(IMGUI_BUILD_EXAMPLES AND IMGUI_BUILD_IMPL_sdl3)
  find_package(SDL3 REQUIRED) # apt show libsdl{2,3}-dev
  set(IMGUI_IMPL_API_sdl3 ${CMAKE_SOURCE_DIR}/backends/imgui_impl_sdl3.h)
  set(IMGUI_IMPL_SRC_sdl3 ${CMAKE_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
endif()

if(IMGUI_BUILD_EXAMPLES AND IMGUI_BUILD_IMPL_glfw)
  find_package(glfw3 REQUIRED) # apt show libglfw3
  set(IMGUI_IMPL_API_glfw ${CMAKE_SOURCE_DIR}/backends/imgui_impl_glfw.h)
  set(IMGUI_IMPL_SRC_glfw ${CMAKE_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)
endif()

set(IMGUI_HEADER_FILES
  ${CMAKE_SOURCE_DIR}/imgui.h
  ${CMAKE_SOURCE_DIR}/imconfig.h
)

set(IMGUI_SOURCE_FILES
  ${CMAKE_SOURCE_DIR}/imgui.cpp
  ${CMAKE_SOURCE_DIR}/imgui_demo.cpp
  ${CMAKE_SOURCE_DIR}/imgui_draw.cpp
  ${CMAKE_SOURCE_DIR}/imgui_tables.cpp
  ${CMAKE_SOURCE_DIR}/imgui_widgets.cpp
)

if(BUILD_SHARED_LIBS)
  set(xLibType SHARED)
  set(IMGUI_IMPL_COMBINE_STATIC OFF)
else()
  set(xLibType STATIC)
endif()

add_library(imgui ${xLibType}
  ${IMGUI_HEADER_FILES}
  ${IMGUI_SOURCE_FILES}
# Internal Private Headers
  ${CMAKE_SOURCE_DIR}/imgui_internal.h
  ${CMAKE_SOURCE_DIR}/imstb_rectpack.h
  ${CMAKE_SOURCE_DIR}/imstb_textedit.h
  ${CMAKE_SOURCE_DIR}/imstb_truetype.h
)

target_include_directories(imgui PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

if(IMGUI_BUILD_EXAMPLES AND IMGUI_BUILD_IMPL_opengl3)
  if(IMGUI_IMPL_COMBINE_STATIC)
    target_sources(imgui PRIVATE
      ${IMGUI_IMPL_API_opengl3}
      ${IMGUI_IMPL_SRC_opengl3}
    )
    target_include_directories(imgui PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
      $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(imgui PUBLIC OpenGL::GL)
  else()
    add_library(imgui-impl-opengl3 ${xLibType}
      ${IMGUI_IMPL_API_opengl3}
      ${IMGUI_IMPL_SRC_opengl3}
    )
    target_include_directories(imgui-impl-opengl3 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
      $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(imgui-impl-opengl3 PUBLIC OpenGL::GL)
  endif()
endif()

if(IMGUI_BUILD_EXAMPLES AND IMGUI_BUILD_IMPL_sdl3)
  if(IMGUI_IMPL_COMBINE_STATIC)
    target_sources(imgui PRIVATE
      ${IMGUI_IMPL_API_sdl3}
      ${IMGUI_IMPL_SRC_sdl3}
    )
    target_include_directories(imgui PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
      $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(imgui PUBLIC SDL3::SDL3)
    target_compile_options(imgui PUBLIC -DSDL_ENABLE_OLD_NAMES)
  else()
    add_library(imgui-impl-sdl3 ${xLibType}
      ${IMGUI_IMPL_API_sdl3}
      ${IMGUI_IMPL_SRC_sdl3}
    )
    target_include_directories(imgui-impl-sdl3 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
      $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(imgui-impl-sdl3 PUBLIC SDL3::SDL3)
    # TODO use SDL3 New Name and this can be safely removed
    target_compile_options(imgui-impl-sdl3 PUBLIC -DSDL_ENABLE_OLD_NAMES)
  endif()
endif()

if(IMGUI_BUILD_EXAMPLES AND IMGUI_BUILD_IMPL_glfw)
  if(IMGUI_IMPL_COMBINE_STATIC)
    target_sources(imgui PRIVATE
      ${IMGUI_IMPL_API_glfw}
      ${IMGUI_IMPL_SRC_glfw}
    )
    target_include_directories(imgui PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
      $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(imgui PUBLIC glfw)
  else()
    add_library(imgui-impl-glfw ${xLibType}
      ${IMGUI_IMPL_API_glfw}
      ${IMGUI_IMPL_SRC_glfw}
    )
    target_include_directories(imgui-impl-glfw PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
      $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(imgui-impl-glfw PUBLIC glfw)
  endif()
endif()

install(TARGETS imgui DESTINATION lib EXPORT imgui-targets)
export(TARGETS  imgui NAMESPACE imgui:: FILE imgui-targets.cmake)
install(EXPORT imgui-targets NAMESPACE imgui::
  DESTINATION lib/cmake/imgui
)

install(FILES ${IMGUI_HEADER_FILES}
  DESTINATION include/imgui
)

if(IMGUI_BUILD_EXAMPLES)
  foreach(xIMPL opengl3 sdl3 glfw vulkan)
    if(IMGUI_BUILD_IMPL_${xIMPL})
      if(NOT IMGUI_IMPL_COMBINE_STATIC)
        install(TARGETS imgui-impl-${xIMPL}
          DESTINATION lib EXPORT imgui-${xIMPL}-targets
        )
        export(TARGETS  imgui-impl-${xIMPL}   NAMESPACE imgui-impl::
          FILE imgui-${xIMPL}-targets.cmake
        )
        install(EXPORT imgui-${xIMPL}-targets NAMESPACE imgui-impl::
          DESTINATION lib/cmake/imgui
        )
      endif()
      install(FILES ${IMGUI_IMPL_API_${xIMPL}}
        DESTINATION include/imgui/impl RENAME ${xIMPL}.h
      )
    endif()
  endforeach()
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/imgui-config-version.cmake"
  VERSION "${imgui_VERSION}" COMPATIBILITY AnyNewerVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/imgui-config-version.cmake"
  DESTINATION lib/cmake/imgui
)

if(IMGUI_BUILD_EXAMPLES)
  string(TOLOWER "${xLibType}" xLibType)
  # xGLP = Graphical Library Platform, xGRL = Graphical Renderer Library
  foreach(xGLP vulkan opengl3)
    if(NOT IMGUI_BUILD_IMPL_${xGLP})
      continue()
    endif()

    foreach(xGRL glfw sdl3)
      if(NOT IMGUI_BUILD_IMPL_${xGRL})
        continue()
      endif()

      set(xDemoName "demo-${xGRL}-${xGLP}-${xLibType}")
      add_executable(${xDemoName}
        "${CMAKE_SOURCE_DIR}/examples/example_${xGRL}_${xGLP}/main.cpp"
      )
      target_include_directories(${xDemoName} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/backends>
      )
      if(IMGUI_IMPL_COMBINE_STATIC)
        target_link_libraries(${xDemoName} PRIVATE
          imgui
        )
      else()
        target_link_libraries(${xDemoName} PRIVATE
          imgui  imgui-impl-${xGLP}  imgui-impl-${xGRL}
        )
      endif()
    endforeach()
  endforeach()
endif()
