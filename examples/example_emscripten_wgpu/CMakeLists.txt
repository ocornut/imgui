# Example usage:
#  cmake -B build -g "Ninja"

cmake_minimum_required(VERSION 3.10.2)
project(imgui_example_emscripten_wgpu C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

set(BUILD_TESTING OFF)
set(BUILD_TESTING_STATIC  OFF)
set(BUILD_TESTING_SHARED OFF)

# Dear ImGui
set(IMGUI_DIR ../../)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

# Libraries
if(EMSCRIPTEN)
	set(LIBRARIES glfw)
	add_compile_options(-sDISABLE_EXCEPTION_CATCHING=1 -DIMGUI_DISABLE_FILE_FUNCTIONS=1)
else()
	# Dawn wgpu desktop
	set(DAWN_FETCH_DEPENDENCIES ON)
	add_subdirectory("external/libraries/dawn" dawn EXCLUDE_FROM_ALL) # point this to up to date dawn repo
	set(LIBRARIES webgpu_dawn webgpu_cpp webgpu_glfw glfw)
endif()

add_executable(example_emscripten_wgpu
	main.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
)

# Libraries
if(EMSCRIPTEN)
	set_target_properties(example_emscripten_wgpu PROPERTIES OUTPUT_NAME "index")
	set_target_properties(example_emscripten_wgpu PROPERTIES SUFFIX ".html")
	set_target_properties(example_emscripten_wgpu PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/web)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/../libs/emscripten/shell_minimal.html")

	target_link_options(example_emscripten_wgpu PRIVATE
		"-sUSE_WEBGPU=1"
		"-sUSE_GLFW=3"
		"-sWASM=1"
		"-sALLOW_MEMORY_GROWTH=1"
		"-sNO_EXIT_RUNTIME=0"
		"-sASSERTIONS=1"
		"-sDISABLE_EXCEPTION_CATCHING=1"
		"-sNO_FILESYSTEM=1"
	)
endif()

target_link_libraries(example_emscripten_wgpu ${LIBRARIES})
