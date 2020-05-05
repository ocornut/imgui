cmake_minimum_required(VERSION 3.0)
project(example_emscripten)

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

set(IMGUI_SRCS imgui/imgui.cpp
    ../../imgui_demo.cpp 
    ../../imgui_draw.cpp 
    ../../imgui_widgets.cpp
    ../imgui_impl_sdl.cpp
    ../imgui_impl_opengl3.cpp
    )

set(INCS ${CMAKE_CURRENT_LIST_DIR} 
    ../..
    ..
    )

set(LIBS ${OPENGL_opengl_LIBRARY} 
    ${OPENGL_glx_LIBRARY}
    ${CMAKE_DL_LIBS}
    SDL2-mt
    Threads::Threads
    )

set(PROJ_SRCS main.cpp)

add_executable(${PROJECT_NAME} ${PROJ_SRCS} ${IMGUI_SRCS})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    SUFFIX .html 
    COMPILE_FLAGS "-DEMSCRIPTEN -s USE_SDL=2" 
    LINK_FLAGS 
        "-s WASM=1 -s USE_PTHREADS=1 -s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1 -Wl,--shared-memory,--no-check-features"
    )

target_include_directories(${PROJECT_NAME} PUBLIC ${INCS})

target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS})
