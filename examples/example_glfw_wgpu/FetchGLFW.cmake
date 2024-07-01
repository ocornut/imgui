include(FetchContent)

if (TARGET glfw)
	return()
endif()

set(GLFW_VERSION "3.4" CACHE STRING "Version of GLFW to use. Must correspond to the tag name of an existing release on https://github.com/glfw/glfw")

if (EMSCRIPTEN)

	add_library(glfw INTERFACE)
	target_link_options(glfw INTERFACE
		-sUSE_GLFW=3
	)

else (EMSCRIPTEN)

	set(URL "https://github.com/glfw/glfw/releases/download/${GLFW_VERSION}/glfw-${GLFW_VERSION}.zip")
	FetchContent_Declare(glfw
		URL "${URL}"
		DOWNLOAD_EXTRACT_TIMESTAMP ON
	)

	message(STATUS "Downloading GLFW from '${URL}'")
	FetchContent_MakeAvailable(glfw)

endif (EMSCRIPTEN)
