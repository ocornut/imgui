# Building for desktop (WebGPU-native) with Dawn:
#  1. git clone https://github.com/google/dawn dawn
#  2. cd dawn
# command 3. will install at <dawn>/cmake-build-debug-clang/install, it takes a while
#  3. CC=clang CXX=clang++ cmake -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" -B cmake-build-debug-clang -S . -DDAWN_ENABLE_INSTALL=ON -DCMAKE_INSTALL_PREFIX=cmake-build-debug-clang/install -DBUILD_SAMPLES=OFF -DDAWN_FETCH_DEPENDENCIES=ON -DDAWN_USE_GLFW=ON -DDAWN_ENABLE_VULKAN=ON -DTINT_BUILD_CMD_TOOLS=OFF -DTINT_BUILD_TESTS=OFF -DDAWN_ENABLE_SPIRV_VALIDATION=OFF
# build ImGui with Dawn
#  4. cmake -B build -DDawn_DIR=<dawn>/cmake-build-debug-clang/install/lib/cmake/Dawn
#  5. cmake --build build --target example_glfw_wgpu_dawn
# The resulting binary will be found at one of the following locations:
#   * build/Debug/example_glfw_wgpu_dawn[.exe]
#   * build/example_glfw_wgpu_dawn[.exe]

# Building for Emscripten:
#  1. Install Emscripten SDK following the instructions: https://emscripten.org/docs/getting_started/downloads.html
#  2. Install Ninja build system
#  3. emcmake cmake -G Ninja -B build
#  3. cmake --build build --target example_glfw_wgpu_emscripten
#  4. emrun build/index.html

cmake_minimum_required(VERSION 3.23)
project(example_glfw_wgpu)

# find_package(imgui REQUIRED)

if (HAS_IMGUI_DAWN AND 0) # example does not compile with latest Dawn
    add_executable(example_glfw_wgpu_dawn)

    target_sources(example_glfw_wgpu_dawn PRIVATE main.cpp)

    target_link_libraries(example_glfw_wgpu_dawn PRIVATE imgui::imgui imgui::backend_glfw_wgpu_dawn)
endif ()

if (HAS_IMGUI_EMSCRIPTEN)
    add_executable(example_glfw_wgpu_emscripten)

    target_sources(example_glfw_wgpu_emscripten PRIVATE main.cpp)

    target_link_libraries(example_glfw_wgpu_emscripten PRIVATE imgui::imgui imgui::backend_glfw_wgpu_emscripten)

    set_target_properties(example_glfw_wgpu_emscripten PROPERTIES OUTPUT_NAME "index")
    # copy our custom index.html to build directory
    add_custom_command(TARGET example_glfw_wgpu_emscripten POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/web/index.html" $<TARGET_FILE_DIR:example_glfw_wgpu_emscripten>
    )

    if (IMGUI_HTTP_PORT)
        set(HTTP_PORT ${IMGUI_HTTP_PORT})
    else ()
        set(HTTP_PORT 8080)
    endif ()
    if (IMGUI_HTTP_ADDRESS)
        set(HTTP_ADDRESS ${IMGUI_HTTP_ADDRESS})
    else ()
        set(HTTP_ADDRESS 127.0.0.1)
    endif ()
    add_custom_target(run_example_glfw_wgpu_emscripten
        COMMAND python -m http.server -b ${HTTP_ADDRESS} -d $<TARGET_FILE_DIR:example_glfw_wgpu_emscripten> ${HTTP_PORT}
        DEPENDS example_glfw_wgpu_emscripten
        COMMENT "Run a python http.server in example_glfw_wgpu_emscripten location http://${HTTP_ADDRESS}:${HTTP_PORT}/"
        USES_TERMINAL
    )
endif ()
