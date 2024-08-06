
-- We use Premake5 to generate project files (Visual Studio solutions, XCode solutions, Makefiles, etc.)
-- Download Premake5: at https://premake.github.io/download
-- YOU NEED PREMAKE 5.0.0-alpha10 or later

--------- HELP
-- To reduce friction for people who don't aren't used to Premake, we list some concrete usage examples.

if _ACTION == nil then
	print "-----------------------------------------"
	print " DEAR IMGUI EXAMPLES - PROJECT GENERATOR"
	print "-----------------------------------------"
	print "Usage:"
	print "  premake5 [generator] [options]"
	print "Examples:"
	print "  premake5 vs2010"
	print "  premake5 vs2019 --with-sdl2 --with-vulkan"
	print "  premake5 xcode4 --with-glfw"
    print "  premake5 gmake2 --with-glfw --cc=clang"
	print "Generators:"
	print "  codelite gmake gmake2 vs2008 vs2010 vs2012 vs2013 vs2015 vs2017 xcode4 etc."
	print "Options:"
	print "  --with-dx9       Enable dear imgui DirectX 9 example"
	print "  --with-dx10      Enable dear imgui DirectX 10 example"
	print "  --with-dx11      Enable dear imgui DirectX 11 example"
	print "  --with-dx12      Enable dear imgui DirectX 12 example (vs2015+)"
	print "  --with-glfw      Enable dear imgui GLFW examples"
	print "  --with-sdl2      Enable dear imgui SDL2 examples"
	print "  --with-vulkan    Enable dear imgui Vulkan example"
    print "  --cc=clang       Compile with Clang"
    print "  --cc=gcc         Compile with GCC"
	print "Project and object files will be created in the build/ folder. You can delete your build/ folder at any time."
	print ""
end

---------- OPTIONS

newoption { trigger = "with-dx9",    description="Enable dear imgui DirectX 9 example" }
newoption { trigger = "with-dx10",   description="Enable dear imgui DirectX 10 example" }
newoption { trigger = "with-dx11",   description="Enable dear imgui DirectX 11 example" }
newoption { trigger = "with-dx12",   description="Enable dear imgui DirectX 12 example" }
newoption { trigger = "with-glfw",   description="Enable dear imgui GLFW examples" }
newoption { trigger = "with-sdl2",   description="Enable dear imgui SDL2 examples" }
newoption { trigger = "with-vulkan", description="Enable dear imgui Vulkan example" }

-- Enable/detect default options under Windows
if _ACTION ~= nil and ((os.istarget ~= nil and os.istarget("windows")) or (os.is ~= nil and os.is("windows"))) then
	print("( enabling --with-dx9 )");
	print("( enabling --with-dx10 )");
	print("( enabling --with-dx11 )");
	_OPTIONS["with-dx9"] = 1
	_OPTIONS["with-dx10"] = 1
	_OPTIONS["with-dx11"] = 1
	if _ACTION >= "vs2015" then
		print("( enabling --with-dx12 because compiler is " .. _ACTION .. " )");
		_OPTIONS["with-dx12"] = 1
	end
	print("( enabling --with-glfw because GLFW is included in the libs/ folder )");
	_OPTIONS["with-glfw"] = 1
	if os.getenv("SDL2_DIR") then
		print("( enabling --with-sdl2 because SDL2_DIR environment variable was found )");
		_OPTIONS["with-sdl2"] = 1
	end
	if os.getenv("VULKAN_SDK") then
		print("( enabling --with-vulkan because VULKAN_SDK environment variable was found )");
		_OPTIONS["with-vulkan"] = 1
	end
end


--------- HELPER FUNCTIONS

-- Helper function: add dear imgui source files into project
function imgui_as_src(fs_path, project_path)
	if (project_path == nil) then project_path = fs_path; end;	        -- default to same virtual folder as the file system folder (in this project it would be ".." !)

	files { fs_path .. "/*.cpp", fs_path .. "/*.h" }
	includedirs { fs_path, fs_path .. "/backends" }
	vpaths { [project_path] = { fs_path .. "/*.*", fs_path .. "/misc/debuggers/*.natvis" } }  -- add in a specific folder of the Visual Studio project
	filter { "toolset:msc*" }
		files { fs_path .. "/misc/debuggers/*.natvis" }
	filter {}
end

-- Helper function: add dear imgui as a library (uncomment the 'include "premake5-lib"' line)
--include "premake5-lib"
function imgui_as_lib(fs_path)
	includedirs { fs_path, fs_path .. "/backends" }
	links "imgui"
end

--------- SOLUTION, PROJECTS

workspace "imgui_examples"
	configurations { "Debug", "Release" }
	platforms { "x86", "x86_64" }

	location "build/"
	symbols "On"
	warnings "Extra"
	--flags { "FatalCompileWarnings"}

	filter { "configurations:Debug" }
		optimize "Off"
	filter { "configurations:Release" }
		optimize "On"
    filter { "toolset:clang", "system:windows" }
        buildoptions { "-Xclang", "-flto-visibility-public-std" }   -- Remove "warning LNK4049: locally defined symbol ___std_terminate imported"

-- example_win32_directx11 (Win32 + DirectX 11)
-- We have DX11 as the first project because this is what Visual Studio uses
if (_OPTIONS["with-dx11"]) then
	project "example_win32_directx11"
		kind "ConsoleApp"
		imgui_as_src ("..", "imgui")
		--imgui_as_lib ("..")
		files { "../backends/imgui_impl_win32.*", "../backends/imgui_impl_dx11.*", "example_win32_directx11/*.cpp", "example_win32_directx11/*.h", "README.txt" }
        vpaths { ["sources"] = "./**" }
		filter { "system:windows", "toolset:msc-v80 or msc-v90 or msc-v100" }
			includedirs { "$(DXSDK_DIR)/Include" }
		filter { "system:windows", "toolset:msc-v80 or msc-v90 or msc-v100", "platforms:x86" }
			libdirs { "$(DXSDK_DIR)/Lib/x86" }
		filter { "system:windows", "toolset:msc-v80 or msc-v90 or msc-v100", "platforms:x86_64" }
			libdirs { "$(DXSDK_DIR)/Lib/x64" }
		filter { "system:windows" }
			links { "d3d11", "d3dcompiler", "dxgi" }
end

-- example_win32_directx9 (Win32 + DirectX 9)
if (_OPTIONS["with-dx9"]) then
	project "example_win32_directx9"
		kind "ConsoleApp"
		imgui_as_src ("..", "imgui")
		--imgui_as_lib ("..")
		files { "../backends/imgui_impl_win32.*", "../backends/imgui_impl_dx9.*", "example_win32_directx9/*.cpp", "example_win32_directx9/*.h", "README.txt" }
        vpaths { ["sources"] = "./**" }
		filter { "system:windows" }
			links { "d3d9" }
end

-- example_win32_directx10 (Win32 + DirectX 10)
if (_OPTIONS["with-dx10"]) then
	project "example_win32_directx10"
		kind "ConsoleApp"
		imgui_as_src ("..", "imgui")
		--imgui_as_lib ("..")
		files { "../backends/imgui_impl_win32.*", "../backends/imgui_impl_dx10.*", "example_win32_directx10/*.cpp", "example_win32_directx10/*.h", "README.txt" }
        vpaths { ["sources"] = "./**" }
		filter { "system:windows", "toolset:msc-v80 or msc-v90 or msc-v100" }
			includedirs { "$(DXSDK_DIR)/Include" }
		filter { "system:windows", "toolset:msc-v80 or msc-v90 or msc-v100", "platforms:x86" }
			libdirs { "$(DXSDK_DIR)/Lib/x86" }
		filter { "system:windows", "toolset:msc-v80 or msc-v90 or msc-v100", "platforms:x86_64" }
			libdirs { "$(DXSDK_DIR)/Lib/x64" }
		filter { "system:windows" }
			links { "d3d10", "d3dcompiler", "dxgi" }
end

-- example_win32_directx12 (Win32 + DirectX 12)
if (_OPTIONS["with-dx12"]) then
	project "example_win32_directx12"
		kind "ConsoleApp"
		systemversion "10.0.16299.0"
		imgui_as_src ("..", "imgui")
		--imgui_as_lib ("..")
		files { "../backends/imgui_impl_win32.*", "../backends/imgui_impl_dx12.*", "example_win32_directx12/*.cpp", "example_win32_directx12/*.h", "README.txt" }
        vpaths { ["sources"] = "./**" }
		filter { "system:windows" }
			links { "d3d12", "d3dcompiler", "dxgi" }
end

-- example_glfw_opengl2 (GLFW + OpenGL2)
if (_OPTIONS["with-glfw"]) then
	project "example_glfw_opengl2"
		kind "ConsoleApp"
		imgui_as_src ("..", "imgui")
		--imgui_as_lib ("..")
		files { "../backends/imgui_impl_glfw.*", "../backends/imgui_impl_opengl2.*", "example_glfw_opengl2/*.h", "example_glfw_opengl2/*.cpp", "README.txt"}
        vpaths { ["sources"] = "./**" }
		includedirs { "libs/glfw/include" }
		filter { "system:windows", "platforms:x86" }
			libdirs { "libs/glfw/lib-vc2010-32" }
		filter { "system:windows", "platforms:x86_64" }
			libdirs { "libs/glfw/lib-vc2010-64" }
		filter { "system:windows" }
			ignoredefaultlibraries { "msvcrt" }
			links { "opengl32", "glfw3" }
		filter { "system:linux" }
			links { "glfw3" }
		filter { "system:macosx" }
			libdirs { "/usr/local/lib" }
			links { "glfw" }
			linkoptions { "-framework OpenGL" }
end

-- example_glfw_opengl3 (GLFW + OpenGL3)
if (_OPTIONS["with-glfw"]) then
	project "example_glfw_opengl3"
		kind "ConsoleApp"
		imgui_as_src ("..", "imgui")
		--imgui_as_lib ("..")
        vpaths { ["sources"] = "./**" }
		files { "../backends/imgui_impl_glfw.*", "../backends/imgui_impl_opengl3.*", "example_glfw_opengl3/*.h", "example_glfw_opengl3/*.cpp", "./README.txt" }
		includedirs { "libs/glfw/include" }
		filter { "system:windows", "platforms:x86" }
			libdirs { "libs/glfw/lib-vc2010-32" }
		filter { "system:windows", "platforms:x86_64" }
			libdirs { "libs/glfw/lib-vc2010-64" }
		filter { "system:windows" }
			ignoredefaultlibraries { "msvcrt" }
			links { "opengl32", "glfw3" }
		filter { "system:linux" }
			links { "glfw3" }
		filter { "system:macosx" }
			libdirs { "/usr/local/lib" }
			links { "glfw" }
			linkoptions { "-framework OpenGL" }
end

-- example_glfw_vulkan (GLFW + Vulkan)
if (_OPTIONS["with-vulkan"]) then
    project "example_glfw_vulkan"
        kind "ConsoleApp"
        imgui_as_src ("..", "imgui")
        --imgui_as_lib ("..")
        vpaths { ["sources"] = "./**" }
        files { "../backends/imgui_impl_glfw*", "../backends/imgui_impl_vulkan.*", "example_glfw_vulkan/*.h", "example_glfw_vulkan/*.cpp", "./README.txt" }
        includedirs { "libs/glfw/include", "%VULKAN_SDK%/include" }
        filter { "system:windows", "platforms:x86" }
            libdirs { "libs/glfw/lib-vc2010-32", "%VULKAN_SDK%/lib32" }
        filter { "system:windows", "platforms:x86_64" }
            libdirs { "libs/glfw/lib-vc2010-64", "%VULKAN_SDK%/lib" }
        filter { "system:windows" }
            ignoredefaultlibraries { "msvcrt" }
            links { "vulkan-1", "glfw3" }
		filter { "system:linux" }
			links { "glfw3" } -- FIXME: missing Vulkan. missing macosx version
end

-- example_null (no rendering)
if (true) then
	project "example_null"
		kind "ConsoleApp"
		imgui_as_src ("..", "imgui")
		--imgui_as_lib ("..")
        vpaths { ["sources"] = "./**" }
		files { "example_null/*.h", "example_null/*.cpp", "./README.txt" }
		filter { "system:windows" }
			ignoredefaultlibraries { "msvcrt" }
end

-- example_sdl2_sdlrenderer2 (SDL2 + SDL_Renderer)
if (_OPTIONS["with-sdl2"]) then
	project "example_sdl2_sdlrenderer2"
		kind "ConsoleApp"
		imgui_as_src ("..", "imgui")
		--imgui_as_lib ("..")
		vpaths { ["sources"] = "./**" }
		files { "../backends/imgui_impl_sdl2*", "../backends/imgui_impl_sdlrenderer2.*", "example_sdl2_sdlrenderer2/*.h", "example_sdl2_sdlrenderer2/*.cpp", "./README.txt" }
		includedirs { "%SDL2_DIR%/include" }
		filter { "system:windows", "platforms:x86" }
			libdirs { "%SDL2_DIR%/lib/x86" }
		filter { "system:windows", "platforms:x86_64" }
			libdirs { "%SDL2_DIR%/lib/x64" }
		filter { "system:windows" }
			ignoredefaultlibraries { "msvcrt" }
			links { "SDL2", "SDL2main" }
end

-- example_sdl2_opengl2 (SDL2 + OpenGL2)
if (_OPTIONS["with-sdl2"]) then
	project "example_sdl2_opengl2"
		kind "ConsoleApp"
		imgui_as_src ("..", "imgui")
		--imgui_as_lib ("..")
        vpaths { ["sources"] = "./**" }
		files { "../backends/imgui_impl_sdl2*", "../backends/imgui_impl_opengl2.*", "example_sdl2_opengl2/*.h", "example_sdl2_opengl2/*.cpp", "./README.txt" }
		includedirs { "%SDL2_DIR%/include" }
		filter { "system:windows", "platforms:x86" }
			libdirs { "%SDL2_DIR%/lib/x86" }
		filter { "system:windows", "platforms:x86_64" }
			libdirs { "%SDL2_DIR%/lib/x64" }
		filter { "system:windows" }
			ignoredefaultlibraries { "msvcrt" }
			links { "SDL2", "SDL2main", "opengl32" }
end

-- example_sdl2_opengl3 (SDL2 + OpenGL3)
if (_OPTIONS["with-sdl2"]) then
	project "example_sdl2_opengl3"
		kind "ConsoleApp"
		imgui_as_src ("..", "imgui")
		--imgui_as_lib ("..")
        vpaths { ["sources"] = "./**" }
		files { "../backends/imgui_impl_sdl2*", "../backends/imgui_impl_opengl3.*", "example_sdl2_opengl3/*.h", "example_sdl2_opengl3/*.cpp", "./README.txt" }
		includedirs { "%SDL2_DIR%/include" }
		filter { "system:windows", "platforms:x86" }
			libdirs { "%SDL2_DIR%/lib/x86" }
		filter { "system:windows", "platforms:x86_64" }
			libdirs { "%SDL2_DIR%/lib/x64" }
		filter { "system:windows" }
			ignoredefaultlibraries { "msvcrt" }
			links { "SDL2", "SDL2main", "opengl32" }
end

-- example_sdl2_vulkan (SDL2 + Vulkan)
if (_OPTIONS["with-sdl2"] and _OPTIONS["with-vulkan"]) then
    project "example_sdl2_vulkan"
        kind "ConsoleApp"
        imgui_as_src ("..", "imgui")
        --imgui_as_lib ("..")
        vpaths { ["sources"] = "./**" }
        files { "../backends/imgui_impl_sdl2*", "../backends/imgui_impl_vulkan.*", "example_sdl2_vulkan/*.h", "example_sdl2_vulkan/*.cpp", "./README.txt" }
        includedirs { "%SDL2_DIR%/include", "%VULKAN_SDK%/include" }
        filter { "system:windows", "platforms:x86" }
            libdirs { "%SDL2_DIR%/lib/x86", "%VULKAN_SDK%/lib32" }
        filter { "system:windows", "platforms:x86_64" }
            libdirs { "%SDL2_DIR%/lib/x64", "%VULKAN_SDK%/lib" }
        filter { "system:windows" }
            ignoredefaultlibraries { "msvcrt" }
            links { "SDL2", "SDL2main", "vulkan-1" }
end
