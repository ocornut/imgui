project "imgui"
    kind "StaticLib"
    files { "../*.h", "../*.cpp" }
    vpaths { ["imgui"] = { "../*.cpp", "../*.h", "../misc/debuggers/*.natvis" } }
    filter { "toolset:msc*" }
        files { "../misc/debuggers/*.natvis" }

