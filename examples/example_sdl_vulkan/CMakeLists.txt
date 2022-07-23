cmake_minimum_required(VERSION 3.16...3.21)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

set(CMAKE_CXX_STANDARD 11)

Project(imgui_example_sdl_vulkan C CXX)
add_executable(example_sdl_vulkan)

#Use ccache if available
find_program(CCACHE_FOUND "ccache")
set(CCACHE_SUPPORT ON CACHE BOOL "Enable ccache support")
if (CCACHE_FOUND AND CCACHE_SUPPORT)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" # GNU is GNU GCC
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # without this compiler messages in `make` backend would be uncolored
    target_compile_options(example_sdl_vulkan PRIVATE -fdiagnostics-color=auto)
  endif()
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "ccache")
endif()


# Find required dependencies
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)
set(FOUND_LIBRARIES "Vulkan::Vulkan;SDL2::SDL2")


# Get compile flags
execute_process(COMMAND pkg-config --cflags-only-other sdl2 OUTPUT_VARIABLE SDL2_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND pkg-config --cflags-only-other vulkan OUTPUT_VARIABLE VULKAN_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
set(FOUND_CFLAGS ${SDL2_CFLAGS} ${VULKAN_CFLAGS})


# Set compile flags
target_compile_definitions(example_sdl_vulkan PUBLIC -DImTextureID=ImU64 -DVK_PROTOTYPES) # Preprocessor definitions
target_compile_options(example_sdl_vulkan PRIVATE ${FOUND_CFLAGS}) # Compile flags
target_link_libraries(example_sdl_vulkan ${FOUND_LIBRARIES}) # Link against vulkan, sdl, etc


set(IMGUI_DIR ../..) # Relative path to the Dear Imgui repo
target_include_directories(example_sdl_vulkan
    PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_sources(example_sdl_vulkan PRIVATE
    main.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp

    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

)

