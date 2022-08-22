project "ImGui"
  kind "StaticLib"
  language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
	"imconfig.h",
    "imgui.cpp",
    "imgui.h",
    "imgui_demo.cpp",
    "imgui_draw.cpp",
    "imgui_internal.h",
    "imgui_tables.cpp",
    "imgui_widgets.cpp",
    "imstb_rectpack.h",
    "imstb_textedit.h",
    "imstb_truetype.h"
	}

    defines { "IMGUI_API=__declspec(dllexport)" }

	filter "system:windows"
		systemversion "latest"
    cppdialect "C++17"
		staticruntime "On"

	filter { "system:windows", "configurations:Release" }
		buildoptions "/MT"
