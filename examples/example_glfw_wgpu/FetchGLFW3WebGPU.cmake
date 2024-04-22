include(FetchContent)

if (TARGET glfw3webgpu)
	return()
endif()

set(GLFW3WEBGPU_VERSION "v1.2.0" CACHE STRING "Version of GLFW to use. Must correspond to the tag name of an existing release on https://github.com/eliemichel/glfw3webgpu")

set(URL "https://github.com/eliemichel/glfw3webgpu/releases/download/${GLFW3WEBGPU_VERSION}/glfw3webgpu-${GLFW3WEBGPU_VERSION}.zip")
FetchContent_Declare(glfw3webgpu
	URL "${URL}"
	DOWNLOAD_EXTRACT_TIMESTAMP ON
)

message(STATUS "Downloading glfw3webgpu from '${URL}'")
FetchContent_MakeAvailable(glfw3webgpu)
