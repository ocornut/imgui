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
  
  includedirs 
  { 
    "../imgui",
    "../glfw/include",
    "../glad/include"
  }
  
  defines "IMGUI_IMPL_OPENGL_LOADER_GLAD"
  
	filter "system:windows"
    systemversion "latest"
    cppdialect "C++17"
    staticruntime "On"
    
    defines "IM_PLATFORM_WINDOWS"

  filter "system:linux"
    systemversion "latest"
    cppdialect "C++17"
    staticruntime "On"
    pic "On"

    filter { "system:windows", "configurations:Release" }
        buildoptions "/MT"
        
 filter "configurations:Debug*"
    defines "IM_CONFIG_DEBUG"
    runtime "Debug"
    symbols "On"

  filter "configurations:Release*"
    defines "IM_CONFIG_RELEASE"
    runtime "Release"
    optimize "On"

  filter "configurations:Dist*"
    defines 
    {
      "IM_CONFIG_DIST",
      "IM_DISABLE_ASSERTS"
    }
    runtime "Release"
    optimize "On"
