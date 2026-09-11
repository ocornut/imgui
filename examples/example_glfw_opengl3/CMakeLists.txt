# You will need GLFW (http://www.glfw.org):
# https://github.com/glfw/glfw.git

cmake_minimum_required(VERSION 2.8)
project(imgui_example_glfw_opengl3 C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

# GLFW
set(GLFW_DIR ../../../glfw) # Set this to point to an up-to-date GLFW repo
set(GL_DIR ../libs/gl3w)
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
include_directories(${GLFW_DIR}/include)
include_directories(${GL_DIR})

# ImGui
set(IMGUI_DIR ../../)
include_directories(${IMGUI_DIR} ..)

# Libraries
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
set(LIBRARIES "glfw;${OPENGL_LIBRARIES}")

file(GLOB sources *.cpp)

add_executable(example_glfw_opengl3 ${sources} 
									 ${IMGUI_DIR}/examples/imgui_impl_glfw.cpp 
									 ${IMGUI_DIR}/examples/imgui_impl_opengl3.cpp 
									 ${IMGUI_DIR}/imgui.cpp 
									 ${IMGUI_DIR}/imgui_draw.cpp 
									 ${IMGUI_DIR}/imgui_demo.cpp 
									 ${IMGUI_DIR}/imgui_widgets.cpp
									 ${GL_DIR}/GL/gl3w.c)

target_link_libraries(example_glfw_opengl3 ${LIBRARIES})
