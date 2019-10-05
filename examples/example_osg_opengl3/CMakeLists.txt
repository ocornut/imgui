cmake_minimum_required(VERSION 3.8)
project(imgui_example_osg C CXX)

# Prerequisites:
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenSceneGraph REQUIRED COMPONENTS osgViewer osgGA)

# Dear ImGui
set(IMGUI_DIR ../../)

# Example app
add_executable(example_osg_opengl3
    main.cpp
    OsgImGuiHandler.hpp
    OsgImGuiHandler.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/examples/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/examples/imgui_impl_opengl3.h
)

target_link_libraries(example_osg_opengl3
    PRIVATE
        ${OPENSCENEGRAPH_LIBRARIES}
        OpenGL::GL
        GLEW::GLEW
)

target_include_directories(example_osg_opengl3
    PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/examples
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
)

target_compile_definitions(example_osg_opengl3
    PRIVATE
        IMGUI_IMPL_OPENGL_LOADER_GLEW
)