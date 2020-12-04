project "ImGui"
	kind "StaticLib"
	language "C++"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
        "imgui.h",
        "imconfig.h",
        "imgui_internal.h",
        "imstb_rectpack.h",
        "imstb_textedit.h",
        "imstb_truetype.h",
		"misc/cpp/imgui_stdlib.h",
		"backends/imgui_impl_glfw.h",
		"backends/imgui_impl_opengl3.h",
		"backends/imgui_impl_vulkan.h",
		
		"imgui.cpp",
		"imgui_demo.cpp",
		"imgui_draw.cpp",
		"imgui_widgets.cpp",
		"misc/cpp/imgui_stdlib.cpp",
		"backends/imgui_impl_glfw.cpp",
		"backends/imgui_impl_opengl3.cpp",
		"backends/imgui_impl_vulkan.cpp",
	}

	defines
	{
		"IMGUI_IMPL_OPENGL_LOADER_GLAD"
	}

	includedirs
	{
		"$(ProjectDir)",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.Vulkan}",
	}

	libdirs
	{
		"%{LibDirs.Vulkan}",
	}

	links 
	{
		"GLFW",
		"Glad",
		"vulkan-1",
	}

	filter "system:linux"
		pic "On"

		systemversion "latest"
		staticruntime "On"

	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
