# imgui_freetype

Build font atlases using FreeType instead of stb_truetype (which is the default font rasterizer).
<br>by @vuhdo, @mikesart, @ocornut.

### Usage

1. Get latest FreeType binaries or build yourself (under Windows you may use vcpkg with `vcpkg install freetype`, `vcpkg integrate install`).
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
- `cfg.OversampleH`, `OversampleV` are ignored (but perhaps not so necessary with this rasterizer).

