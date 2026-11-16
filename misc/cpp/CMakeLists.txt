add_library(imgui_stdlib imgui_stdlib.cpp)
target_include_directories(imgui_stdlib PUBLIC .)
target_link_libraries(imgui_stdlib PUBLIC imgui)