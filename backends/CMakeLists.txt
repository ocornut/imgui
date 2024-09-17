if(IMGUI_IMPL_OPENGL2 OR IMGUI_IMPL_OPENGL3)
find_package(OpenGL)
endif()

if(IMGUI_IMPL_WIN32)
add_library(imgui-impl-win32 STATIC)
target_sources(imgui-impl-win32 PUBLIC imgui_impl_win32.cpp)
target_include_directories(imgui-impl-win32 PUBLIC ${IMGUI_INC_DIR})
endif()

if(OpenGL_FOUND)

if(IMGUI_IMPL_OPENGL2)
add_library(imgui-impl-opengl2 STATIC)
target_sources(imgui-impl-opengl2 PUBLIC imgui_impl_opengl2.cpp)
target_include_directories(imgui-impl-opengl2 PUBLIC ${IMGUI_INC_DIR})
target_link_libraries(imgui-impl-opengl2 OpenGL::GL)
endif()

if(IMGUI_IMPL_OPENGL3)
add_library(imgui-impl-opengl3 STATIC)
target_sources(imgui-impl-opengl3 PUBLIC imgui_impl_opengl3.cpp)
target_include_directories(imgui-impl-opengl3 PUBLIC ${IMGUI_INC_DIR})
target_link_libraries(imgui-impl-opengl3 OpenGL::GL)

endif()

endif()
