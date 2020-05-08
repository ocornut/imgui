cmake_minimum_required(VERSION 3.0)
project(example_emscripten)

option(USE_FILE_SYSTEM "Enable using the filesystem" ON)

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

set(LIBS ${CMAKE_DL_LIBS} SDL2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2 -s WASM=1 -s DISABLE_EXCEPTION_CATCHING=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_PTHREADS=1 -s ALLOW_MEMORY_GROWTH=1 -Wl,--shared-memory,--no-check-features --shell-file shell_minimal.html")

if(USE_FILE_SYSTEM)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --no-heap-copy --preload-file ${CMAKE_CURRENT_LIST_DIR}/../../misc/fonts@/fonts")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s NO_FILESYSTEM=1 -DIMGUI_DISABLE_FILE_FUNCTIONS")
endif()

add_executable(${PROJECT_NAME} ${PROJ_SRCS} ${IMGUI_SRCS})

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX .html)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCS})

target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS})

