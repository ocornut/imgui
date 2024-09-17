add_library(imgui-cpp STATIC)
target_sources(imgui-cpp PUBLIC imgui_stdlib.cpp)
target_include_directories(imgui-cpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${IMGUI_INC_DIR})
