CMAKE_MINIMUM_REQUIRED(VERSION 3.2)
project(imgui)
SET (LIB_TYPE STATIC)

add_library(imgui ${LIB_TYPE} imgui.cpp imgui_draw.cpp)

install(TARGETS imgui DESTINATION lib)
install(FILES imgui.h imconfig.h DESTINATION include)