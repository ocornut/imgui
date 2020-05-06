cmake_minimum_required(VERSION 3.0)
project(example_emscripten)

set(PROJ_SRCS main.cpp)

set(IMGUI_SRCS ../../imgui.cpp
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

set(LIBS ${CMAKE_DL_LIBS} SDL2-mt)

add_executable(${PROJECT_NAME} ${PROJ_SRCS} ${IMGUI_SRCS})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    SUFFIX .html 
    COMPILE_FLAGS 
        "-s USE_SDL=2 -s WASM=1 -s DISABLE_EXCEPTION_CATCHING=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1" 
    LINK_FLAGS 
        "-s USE_PTHREADS=1 -s ALLOW_MEMORY_GROWTH=1 -Wl,--shared-memory,--no-check-features --no-heap-copy --preload-file ../../../misc/fonts@/fonts"
    )

target_include_directories(${PROJECT_NAME} PUBLIC ${INCS})

target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS})

