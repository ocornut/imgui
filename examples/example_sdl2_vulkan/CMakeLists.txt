cmake_minimum_required(VERSION 2.8)

project(imgui_example_sdl2_vulkan C CXX)

set(CMAKE_CXX_STANDARD 11)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/../..")

add_executable(example_sdl2_vulkan
               "${IMGUI_DIR}/imconfig.h"
               "${IMGUI_DIR}/imgui.cpp"
               "${IMGUI_DIR}/imgui.h"
               "${IMGUI_DIR}/imgui_demo.cpp"
               "${IMGUI_DIR}/imgui_draw.cpp"
               "${IMGUI_DIR}/imgui_internal.h"
               "${IMGUI_DIR}/imgui_tables.cpp"
               "${IMGUI_DIR}/imgui_widgets.cpp"
               "${IMGUI_DIR}/imstb_rectpack.h"
               "${IMGUI_DIR}/imstb_textedit.h"
               "${IMGUI_DIR}/imstb_truetype.h"

               "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp"
               "${IMGUI_DIR}/backends/imgui_impl_sdl2.h"
               "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
               "${IMGUI_DIR}/backends/imgui_impl_vulkan.h"

               main.cpp)

target_include_directories(example_sdl2_vulkan
                           PRIVATE
                           ${IMGUI_DIR}
                           "${IMGUI_DIR}/backends"

                           SYSTEM PRIVATE
                           ${SDL2_INCLUDE_DIRS}
                           )

target_link_libraries(example_sdl2_vulkan
                      PRIVATE
                      Vulkan::Vulkan
                      ${SDL2_LIBRARIES}
                      )
