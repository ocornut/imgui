project "Imgui"
    kind "StaticLib"
    language "C++"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
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
        "imstb_truetpye.h",
        "imgui_demo.cpp",
        "misc/cpp/imgui_stdlib.h",
        "misc/cpp/imgui_stdlib.cpp"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS"
    }

    filter "system:windows"
        systemversion "latest"
        cppdialect "c++17"
        staticruntime "On"

    
    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        buildoptions "/MTd"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        buildoptions "/MT"
