# This file is part of the "Learn WebGPU for C++" book.
#   https://eliemichel.github.io/LearnWebGPU
#   https://github.com/eliemichel/WebGPU-distribution/blob/dev-fetch-wgpu/cmake/FetchWgpuNative.cmake
# 
# MIT License
# Copyright (c) 2023-2024 Elie Michel and the wgpu-native authors
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

include(FetchContent)

if (TARGET webgpu)
	return()
endif()

set(WGPU_LINK_TYPE "DYNAMIC" CACHE STRING "Whether the wgpu-native WebGPU implementation must be statically or dynamically linked. Possible values are STATIC and DYNAMIC")
set(WGPU_VERSION "v0.19.4.1" CACHE STRING "Version of the wgpu-native WebGPU implementation to use. Must correspond to the tag name of an existing release on https://github.com/gfx-rs/wgpu-native/releases")

if (EMSCRIPTEN)

	add_library(webgpu INTERFACE)

	target_include_directories(webgpu INTERFACE
		"${CMAKE_CURRENT_SOURCE_DIR}/include-emscripten"
	)

	# This is used to advertise the flavor of WebGPU that this zip provides
	target_compile_definitions(webgpu INTERFACE WEBGPU_BACKEND_EMSCRIPTEN)

	target_link_options(webgpu INTERFACE
		-sUSE_WEBGPU # Handle WebGPU symbols
	)

	function(target_copy_webgpu_binaries Target)
	endfunction()

else (EMSCRIPTEN)

	if (NOT ARCH)
		set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
		if (ARCH STREQUAL "AMD64")
			set(ARCH "x86_64")
		endif()
	endif()

	set(USE_DYNAMIC_LIB)
	if (WGPU_LINK_TYPE STREQUAL "DYNAMIC")
		set(USE_DYNAMIC_LIB TRUE)
	elseif (WGPU_LINK_TYPE STREQUAL "STATIC")
		set(USE_DYNAMIC_LIB FALSE)
	else()
		message(FATAL_ERROR "Link type '${WGPU_LINK_TYPE}'is not valid. Possible values for WGPU_LINK_TYPE are DYNAMIC and STATIC.")
	endif()

	set(URL_OS)
	set(BINARY_FILENAME)
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")

		set(URL_OS windows)
		if (USE_DYNAMIC_LIB)
			set(BINARY_FILENAME "wgpu_native.dll")
		else()
			set(BINARY_FILENAME "wgpu_native.lib")
		endif()

	elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

		set(URL_OS linux)
		if (USE_DYNAMIC_LIB)
			set(BINARY_FILENAME "libwgpu_native.so")
		else()
			set(BINARY_FILENAME "libwgpu_native.a")
		endif()

	elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")

		set(URL_OS macos)
		if (USE_DYNAMIC_LIB)
			set(BINARY_FILENAME "libwgpu_native.dylib")
		else()
			set(BINARY_FILENAME "libwgpu_native.a")
		endif()

	else()

		message(FATAL_ERROR "Platform system ${CMAKE_SYSTEM_NAME} not supported by this release of WebGPU. You may consider building it yourself from its source (see https://github.com/gfx-rs/wgpu-native)")

	endif()

	set(URL_ARCH)
	if (ARCH STREQUAL "x86_64")
		set(URL_ARCH "x86_64")
	elseif (ARCH STREQUAL "aarch64" AND NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
		set(URL_ARCH "aarch64")
	elseif (ARCH STREQUAL "i686" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
		set(URL_ARCH "i686")
	else()
		message(FATAL_ERROR "Platform architecture ${ARCH} not supported by this release of WebGPU. You may consider building it yourself from its source (see https://github.com/gfx-rs/wgpu-native)")
	endif()

	set(URL_CONFIG release)
	set(URL_NAME "wgpu-${URL_OS}-${URL_ARCH}-${URL_CONFIG}")
	set(URL "https://github.com/gfx-rs/wgpu-native/releases/download/${WGPU_VERSION}/${URL_NAME}.zip")

	FetchContent_Declare(${URL_NAME}
		URL ${URL}
		DOWNLOAD_EXTRACT_TIMESTAMP ON
	)
	message(STATUS "Downloading WebGPU runtime from '${URL}'")
	FetchContent_MakeAvailable(${URL_NAME})
	set(ZIP_DIR "${${URL_NAME}_SOURCE_DIR}")

	# Fix folder layout
	file(MAKE_DIRECTORY "${ZIP_DIR}/include/webgpu")
	if (EXISTS "${ZIP_DIR}/webgpu.h")
		file(RENAME "${ZIP_DIR}/webgpu.h" "${ZIP_DIR}/include/webgpu/webgpu.h")
	endif()
	if (EXISTS "${ZIP_DIR}/wgpu.h")
		file(RENAME "${ZIP_DIR}/wgpu.h" "${ZIP_DIR}/include/webgpu/wgpu.h")
	endif()

	# A pre-compiled target (IMPORTED) that is a dynamically
	# linked library (SHARED, meaning .dll, .so or .dylib).
	add_library(webgpu SHARED IMPORTED GLOBAL)

	# This is used to advertise the flavor of WebGPU that this zip provides
	target_compile_definitions(webgpu INTERFACE WEBGPU_BACKEND_WGPU)

	set(WGPU_RUNTIME_LIB "${ZIP_DIR}/${BINARY_FILENAME}")
	set_target_properties(
		webgpu
		PROPERTIES
			IMPORTED_LOCATION "${WGPU_RUNTIME_LIB}"
			INTERFACE_INCLUDE_DIRECTORIES "${ZIP_DIR}/include"
	)

	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")

		set_target_properties(
			webgpu
			PROPERTIES
				IMPORTED_IMPLIB "${WGPU_RUNTIME_LIB}.lib"
		)

	elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

		set_target_properties(
			webgpu
			PROPERTIES
				IMPORTED_NO_SONAME TRUE
		)

	endif()

	message(STATUS "Using WebGPU runtime from '${WGPU_RUNTIME_LIB}'")
	#set(WGPU_RUNTIME_LIB ${WGPU_RUNTIME_LIB} PARENT_SCOPE)
	set(WGPU_RUNTIME_LIB ${WGPU_RUNTIME_LIB} CACHE INTERNAL "Path to the WebGPU library binary")

	# The application's binary must find wgpu.dll or libwgpu.so at runtime,
	# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
	# next to the binary.
	# Also make sure that the binary's RPATH is set to find this shared library.
	function(target_copy_webgpu_binaries Target)
		add_custom_command(
			TARGET ${Target} POST_BUILD
			COMMAND
				${CMAKE_COMMAND} -E copy_if_different
				${WGPU_RUNTIME_LIB}
				$<TARGET_FILE_DIR:${Target}>
			COMMENT
				"Copying '${WGPU_RUNTIME_LIB}' to '$<TARGET_FILE_DIR:${Target}>'..."
		)

		if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
			# Bug fix, there might be a cleaner way to do this but no INSTALL_RPATH
			# or related target properties seem to be a solution.
			set_target_properties(${Target} PROPERTIES INSTALL_RPATH "./")
			if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
				set(ARCH_DIR aarch64)
			else()
				set(ARCH_DIR ${CMAKE_SYSTEM_PROCESSOR})
			endif()
			add_custom_command(
				TARGET ${Target} POST_BUILD
				COMMAND
					${CMAKE_INSTALL_NAME_TOOL} "-change"
					"/Users/runner/work/wgpu-native/wgpu-native/target/${ARCH_DIR}-apple-darwin/release/deps/libwgpu_native.dylib"
					"@executable_path/libwgpu_native.dylib"
					"$<TARGET_FILE:${Target}>"
				VERBATIM
			)
		endif()
	endfunction()

endif (EMSCRIPTEN)
