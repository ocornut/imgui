file(GLOB IMGUI_SRC "*.cpp")

add_library(imgui STATIC ${IMGUI_SRC})

target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
