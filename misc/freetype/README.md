# imgui_freetype

Build font atlases using FreeType instead of stb_truetype (which is the default font rasterizer).
<br>by @vuhdo, @mikesart, @ocornut.

### Usage

1. Get latest FreeType binaries or build yourself (under Windows you may use vcpkg with `vcpkg install freetype --triplet=x64-windows`, `vcpkg integrate install`).
2. Add imgui_freetype.h/cpp alongside your project files.
3. Add `#define IMGUI_ENABLE_FREETYPE` in your [imconfig.h](https://github.com/ocornut/imgui/blob/master/imconfig.h) file

### About Gamma Correct Blending

FreeType assumes blending in linear space rather than gamma space.
See FreeType note for [FT_Render_Glyph](https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_Render_Glyph).
For correct results you need to be using sRGB and convert to linear space in the pixel shader output.
The default Dear ImGui styles will be impacted by this change (alpha values will need tweaking).

### Testbed for toying with settings (for developers)

See https://gist.github.com/ocornut/b3a9ecf13502fd818799a452969649ad

### Known issues

- Oversampling settins are ignored but also not so much necessary with the higher quality rendering.

### Comparaison

Small, thin anti-aliased fonts typically benefit a lot from FreeType's hinting:
![comparing_font_rasterizers](https://user-images.githubusercontent.com/8225057/107550178-fef87f00-6bd0-11eb-8d09-e2edb2f0ccfc.gif)

### Colorful glyphs/emojis

You can use the `ImGuiFreeTypeBuilderFlags_LoadColor` flag to load certain colorful glyphs. See the
["Using Colorful Glyphs/Emojis"](https://github.com/ocornut/imgui/edit/master/docs/FONTS.md#using-colorful-glyphsemojis) section of FONTS.md.

![colored glyphs](https://user-images.githubusercontent.com/8225057/106171241-9dc4ba80-6191-11eb-8a69-ca1467b206d1.png)
