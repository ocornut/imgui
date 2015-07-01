add_library(gl3w STATIC
    GL/gl3w.c GL/gl3w.h GL/glcorearb.h)
target_include_directories(gl3w PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(gl3w PRIVATE ${CMAKE_DL_LIBS})
