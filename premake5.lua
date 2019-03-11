-- PREMAKE FOR IMGUI

project "imgui"
    kind "StaticLib"
    language "C++"
    
	targetdir ("bin/" .. output_dir .. "/%{prj.name}")
  objdir ("bin-int/" .. output_dir .. "/%{prj.name}")

	files
  {
    "imconfig.h",
    "imgui.h",
    "imgui.cpp",
    "imgui_draw.cpp",
    "imgui_internal.h",
    "imgui_widgets.cpp",
    "imstb_rectpack.h",
    "imstb_textedit.h",
    "imstb_truetype.h",
    "imgui_demo.cpp",
    "imgui_impl_opengl3.h",
    "imgui_impl_opengl3.cpp",
    "imgui_impl_glfw.h",
    "imgui_impl_glfw.cpp"
  }
  
	filter "system:windows"
    systemversion "latest"
    cppdialect "C++17"
    staticruntime "On"

  filter "system:linux"
    systemversion "latest"
    cppdialect "C++17"
    staticruntime "On"
    pic "On"

    filter { "system:windows", "configurations:Release" }
        buildoptions "/MT"
