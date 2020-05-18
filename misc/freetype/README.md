# imgui_freetype

Build font atlases using FreeType instead of stb_truetype (which is the default font rasterizer in Dear ImGui).
<br>by @vuhdo, @mikesart, @ocornut.

### Usage

1. Get latest FreeType binaries or build yourself (under Windows you may use vcpkg with `vcpkg install freetype`, `vcpkg integrate install`).
2. Add imgui_freetype.h/cpp alongside your imgui sources.
3. Include imgui_freetype.h after imgui.h.
4. Call `ImGuiFreeType::BuildFontAtlas()` *BEFORE* calling `ImFontAtlas::GetTexDataAsRGBA32()` or `ImFontAtlas::Build()` (so normal Build() won't be called):

```cpp
// See ImGuiFreeType::RasterizationFlags
unsigned int flags = ImGuiFreeType::NoHinter;
ImGuiFreeType::BuildFontAtlas(io.Fonts, flags);
io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
```

### Gamma Correct Blending

FreeType assumes blending in linear space rather than gamma space.
See FreeType note for [FT_Render_Glyph](https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_Render_Glyph).
For correct results you need to be using a sRGB framebuffer and convert vertex colors to linear space in the vertex shader.
The default Dear ImGui styles will be impacted by this change (alpha values will need tweaking).

### Subpixel Rendering

Subpixel rendering exploits the color-striped structure of LCD pixels to increase the resolution in the direction of the stripe. This patented technology can be switched on when building FreeType by defining `FT_CONFIG_OPTION_SUBPIXEL_RENDERING` in `ftoption.h`. ImGui implementations must retrieve the 32-bit texture using `ImFontAtlas::GetTexDataAsRGBA32()` in order to access subpixel data. On top of gamma correct blending, as explained above, implementations must also use dual source blending to output the subpixel masks for use as destination factors in the blending function.

### Example

An example implementation based on SDL and OpenGL 3 can be compiled, tested and checked in `examples/example_sdl_opengl3`.

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
    unsigned int  HinterFlags;
    unsigned int  HintingFlags;
    unsigned int  ModeFlags;
    unsigned int  OptionFlags;

    FreeTypeTest()
    {
        BuildMode = FontBuildMode_FreeType;
        WantRebuild = true;
        FontsMultiply = 1.0f;
        FontsPadding = 1;
        HinterFlags = ImGuiFreeType::PreferFontHinter;
        HintingFlags = ImGuiFreeType::LightHinting;
        ModeFlags = ImGuiFreeType::LcdMode;
        OptionFlags = ImGuiFreeType::LcdLightFilter;
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
        }
        if (BuildMode == FontBuildMode_FreeType)
            ImGuiFreeType::BuildFontAtlas(io.Fonts, HinterFlags | HintingFlags | ModeFlags | OptionFlags);
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
            ImGui::Text("Hinter:");
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("Prefer font", (int*)&HinterFlags, ImGuiFreeType::PreferFontHinter);
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("Prefer auto", (int*)&HinterFlags, ImGuiFreeType::PreferAutoHinter);
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("No auto", (int*)&HinterFlags, ImGuiFreeType::NoAutoHinter);
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("Disabled", (int*)&HinterFlags, ImGuiFreeType::NoHinter);

            ImGui::Text("Hinting:");
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("Gray##Hinting", (int*)&HintingFlags, ImGuiFreeType::GrayHinting);
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("Light", (int*)&HintingFlags, ImGuiFreeType::LightHinting);
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("Mono##Hinting", (int*)&HintingFlags, ImGuiFreeType::MonoHinting);
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("LCD##Hinting", (int*)&HintingFlags, ImGuiFreeType::LcdHinting);
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("LCD V##Hinting", (int*)&HintingFlags, ImGuiFreeType::LcdVHinting);

            ImGui::Text("Mode:");
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("Mono##Mode", (int*)&ModeFlags, ImGuiFreeType::MonoMode);
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("Gray##Mode", (int*)&ModeFlags, ImGuiFreeType::GrayMode);
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("LCD##Mode", (int*)&ModeFlags, ImGuiFreeType::LcdMode);
            ImGui::SameLine();
            WantRebuild |= ImGui::RadioButton("LCD V##Mode", (int*)&ModeFlags, ImGuiFreeType::LcdVMode);

            ImGui::Text("Options:");
            ImGui::SameLine();
            WantRebuild |= ImGui::CheckboxFlags("Bold", &OptionFlags, ImGuiFreeType::Bold);
            ImGui::SameLine();
            WantRebuild |= ImGui::CheckboxFlags("Oblique", &OptionFlags, ImGuiFreeType::Oblique);
            ImGui::SameLine();
            WantRebuild |= ImGui::CheckboxFlags("LCD light filter", &OptionFlags, ImGuiFreeType::LcdLightFilter);
        }
        ImGui::End();
    }
};
```

### Known issues
- `cfg.OversampleH`, `OversampleV` are ignored (but perhaps not so necessary with this rasterizer).

