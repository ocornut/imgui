-- imgui/premake5.lua

	-------------------------------
	-- [ PROJECT CONFIGURATION ] --
	-------------------------------
	project "imgui"
		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		targetdir ("%{prj.location}/bin/%{cfg.platform}/%{cfg.buildcfg}")
		objdir "%{prj.location}/obj/%{prj.name}/%{cfg.platform}/%{cfg.buildcfg}"

		local srcDir = "./"

		files
		{
			srcDir .. "*.h",
			srcDir .. "*.hpp",
			srcDir .. "*.inl",
			srcDir .. "*.c",
			srcDir .. "*.cpp",
			srcDir .. "examples/imgui_impl_dx11.h",
			srcDir .. "examples/imgui_impl_dx11.cpp",
			srcDir .. "examples/imgui_impl_win32.h",
			srcDir .. "examples/imgui_impl_win32.cpp",
		}

		includedirs { srcDir }