# imgui_freetype

Build font atlases using FreeType instead of stb_truetype (the default imgui's font rasterizer).
<br>by @vuhdo, @mikesart, @ocornut.

### Usage

1. Get latest FreeType binaries or build yourself (under Windows you may use vcpkg with `vcpkg install freetype`).
2. Add imgui_freetype.h/cpp alongside your imgui sources.
3. Include imgui_freetype.h after imgui.h.
4. Call `ImGuiFreeType::BuildFontAtlas()` *BEFORE* calling `ImFontAtlas::GetTexDataAsRGBA32()` or `ImFontAtlas::Build()` (so normal Build() won't be called):

```cpp
// See ImGuiFreeType::RasterizationFlags
unsigned int flags = ImGuiFreeType::NoHinting;
ImGuiFreeType::BuildFontAtlas(io.Fonts, flags);
io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
```

### Gamma Correct Blending

FreeType assumes blending in linear space rather than gamma space.
See FreeType note for [FT_Render_Glyph](https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_Render_Glyph).
For correct results you need to be using sRGB and convert to linear space in the pixel shader output.
The default imgui styles will be impacted by this change (alpha values will need tweaking).

### Test code Usage
```cpp
#include "misc/freetype/imgui_freetype.h"
#include "misc/freetype/imgui_freetype.cpp"

// Load various small fonts
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 13.0f);
io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 13.0f);
io.Fonts->AddFontDefault();

FreeTypeTest freetype_test;

// Main Loop
while (true)
{
   if (freetype_test.UpdateRebuild())
   {
      // REUPLOAD FONT TEXTURE TO GPU
      ImGui_ImplXXX_DestroyDeviceObjects();
      ImGui_ImplXXX_CreateDeviceObjects();
   }
   ImGui::NewFrame();
   freetype_test.ShowFreetypeOptionsWindow();
   ...
}
```

### Test code
```cpp
#include "misc/freetype/imgui_freetype.h"
#include "misc/freetype/imgui_freetype.cpp"

struct FreeTypeTest
{
    enum FontBuildMode
    {
        FontBuildMode_FreeType,
        FontBuildMode_Stb
    };

    FontBuildMode BuildMode;
    bool          WantRebuild;
    float         FontsMultiply;
    int           FontsPadding;
    unsigned int  FontsFlags;

    FreeTypeTest()
    {
        BuildMode = FontBuildMode_FreeType;
        WantRebuild = true;
        FontsMultiply = 1.0f;
        FontsPadding = 1;
        FontsFlags = 0;
    }

    // Call _BEFORE_ NewFrame()
    bool UpdateRebuild()
    {
        if (!WantRebuild)
            return false;
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->TexGlyphPadding = FontsPadding;
        for (int n = 0; n < io.Fonts->ConfigData.Size; n++)
        {
            ImFontConfig* font_config = (ImFontConfig*)&io.Fonts->ConfigData[n];
            font_config->RasterizerMultiply = FontsMultiply;
            font_config->RasterizerFlags = (BuildMode == FontBuildMode_FreeType) ? FontsFlags : 0x00;
        }
        if (BuildMode == FontBuildMode_FreeType)
            ImGuiFreeType::BuildFontAtlas(io.Fonts, FontsFlags);
        else if (BuildMode == FontBuildMode_Stb)
            io.Fonts->Build();
        WantRebuild = false;
        return true;
    }

    // Call to draw interface
    void ShowFreetypeOptionsWindow()
    {
        ImGui::Begin("FreeType Options");
        ImGui::ShowFontSelector("Fonts");
        WantRebuild |= ImGui::RadioButton("FreeType", (int*)&BuildMode, FontBuildMode_FreeType);
        ImGui::SameLine();
        WantRebuild |= ImGui::RadioButton("Stb (Default)", (int*)&BuildMode, FontBuildMode_Stb);
        WantRebuild |= ImGui::DragFloat("Multiply", &FontsMultiply, 0.001f, 0.0f, 2.0f);
        WantRebuild |= ImGui::DragInt("Padding", &FontsPadding, 0.1f, 0, 16);
        if (BuildMode == FontBuildMode_FreeType)
        {
            WantRebuild |= ImGui::CheckboxFlags("NoHinting",     &FontsFlags, ImGuiFreeType::NoHinting);
            WantRebuild |= ImGui::CheckboxFlags("NoAutoHint",    &FontsFlags, ImGuiFreeType::NoAutoHint);
            WantRebuild |= ImGui::CheckboxFlags("ForceAutoHint", &FontsFlags, ImGuiFreeType::ForceAutoHint);
            WantRebuild |= ImGui::CheckboxFlags("LightHinting",  &FontsFlags, ImGuiFreeType::LightHinting);
            WantRebuild |= ImGui::CheckboxFlags("MonoHinting",   &FontsFlags, ImGuiFreeType::MonoHinting);
            WantRebuild |= ImGui::CheckboxFlags("Bold",          &FontsFlags, ImGuiFreeType::Bold);
            WantRebuild |= ImGui::CheckboxFlags("Oblique",       &FontsFlags, ImGuiFreeType::Oblique);
        }
        ImGui::End();
    }
};
```

### Known issues
- `cfg.OversampleH`, `OversampleV` are ignored (but perhaps not so necessary with this rasterizer).

