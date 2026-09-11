#pragma once
#include "imgui.h"
#ifndef IMGUI_DISABLE

struct ImFontLoader;

namespace ImGuiDWrite
{
    IMGUI_API const wchar_t* GetUserDefaultLocaleName();
    IMGUI_API const ImFontLoader* GetFontLoader();
}

#endif
