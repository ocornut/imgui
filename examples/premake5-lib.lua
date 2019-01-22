project "imgui"
    kind "StaticLib"
    files { "../*.h", "../*.cpp" }
    vpaths { ["imgui"] = { "../*.cpp", "../*.h", "../misc/natvis/*.natvis" } }
    filter { "toolset:msc*" }
        files { "../misc/natvis/*.natvis" }

