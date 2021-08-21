project "ImGui"

	-- Output Directories --
	location "%{wks.location}/Dependencies/imgui"

	targetdir (target_dir)
	objdir    (object_dir)

	-- Compiler --
	kind "StaticLib"
	language "C++"

	-- Project Files ---
	files
	{
        -- imgui
		"imconfig.h",
		"imgui.cpp",
		"imgui.h",
		"imgui_demo.cpp",
		"imgui_demo.h",
		"imgui_draw.cpp",
		"imgui_internal.h",
		"imgui_tables.cpp",
		"imgui_widgets.cpp",
		"imstb_rectpact.h",
		"imstb_textedit.h",
		"imstb_truetype.h",

        -- backends
		"backends/imgui_impl_glfw.cpp",
		"backends/imgui_impl_glfw.h",
		"backends/imgui_impl_opengl3.cpp",
		"backends/imgui_impl_opengl3.h",

        -- build.lua
		"%{prj.location}/build.lua",
	}
	
	-- Includes --
	includedirs
	{
		"%{prj.location}/",

        "%{include_dirs.glad}",
        "%{include_dirs.glfw}",
	}

    -- Links --
	links
	{
		"GLAD",
		"GLFW"
	}

	--- Filters ---
	-- windows
	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

		files
		{
			"backends/imgui_impl_dx11.cpp",
			"backends/imgui_impl_dx11.h",
			"backends/imgui_impl_win32.cpp",
			"backends/imgui_impl_win32.h",
		}

		defines 
		{ 
			"_CRT_SECURE_NO_WARNINGS",
		}

		flags { "MultiProcessorCompile" }

	filter "system:linux"
		links
		{
			"GL",
		}

	-- debug
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	-- release
	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	-- distribution
	filter "configurations:Distribution"
		runtime "Release"
		optimize "full"
