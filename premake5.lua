project "ImGui"
	kind "StaticLib"
	language "C++"
    staticruntime "on"
	systemversion "latest"
	cppdialect "C++17"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"imconfig.h",
		"imgui.h",
		"imgui.cpp",
		"imgui_draw.cpp",
		"imgui_internal.h",
		"imgui_tables.cpp",
		"imgui_widgets.cpp",
		"imstb_rectpack.h",
		"imstb_textedit.h",
		"imstb_truetype.h",
		"imgui_demo.cpp"
	}


	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"

	filter "configurations:Development"
		runtime "Release"
		optimize "On"

	filter "configurations:Ship"
		runtime "Release"
		optimize "On"
