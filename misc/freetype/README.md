# imgui_freetype

Build font atlases using FreeType instead of stb_truetype (which is the default font rasterizer).
<br>by @vuhdo, @mikesart, @ocornut.

### Usage

1. Get latest FreeType binaries or build yourself (under Windows you may use vcpkg with `vcpkg install freetype --triplet=x64-windows`, `vcpkg integrate install`).
2. Add imgui_freetype.h/cpp alongside your project files.
3. Add `#define IMGUI_ENABLE_FREETYPE` in your [imconfig.h](https://github.com/ocornut/imgui/blob/master/imconfig.h) file

### About Gamma Correct Blending

FreeType assumes blending in linear space rather than gamma space.
See FreeType note for [FT_Render_Glyph](https://freetype.org/freetype2/docs/reference/ft2-glyph_retrieval.html#ft_render_glyph).
For correct results you need to be using sRGB and convert to linear space in the pixel shader output.
The default Dear ImGui styles will be impacted by this change (alpha values will need tweaking).

### Testbed for toying with settings (for developers)

See https://gist.github.com/ocornut/b3a9ecf13502fd818799a452969649ad

### Known issues

- Oversampling settins are ignored but also not so much necessary with the higher quality rendering.

### Comparison

Small, thin anti-aliased fonts typically benefit a lot from FreeType's hinting:
![comparing_font_rasterizers](https://user-images.githubusercontent.com/8225057/107550178-fef87f00-6bd0-11eb-8d09-e2edb2f0ccfc.gif)

### Colorful glyphs/emojis

You can use the `ImGuiFreeTypeBuilderFlags_LoadColor` flag to load certain colorful glyphs. See the
["Using Colorful Glyphs/Emojis"](https://github.com/ocornut/imgui/blob/master/docs/FONTS.md#using-colorful-glyphsemojis) section of FONTS.md.

![colored glyphs](https://user-images.githubusercontent.com/8225057/106171241-9dc4ba80-6191-11eb-8a69-ca1467b206d1.png)


### Using OpenType SVG fonts (SVGinOT)
- *SVG in Open Type* is a standard by Adobe and Mozilla for color OpenType and Open Font Format fonts. It allows font creators to embed complete SVG files within a font enabling full color and even animations.
- Popular fonts such as [twemoji](https://github.com/13rac1/twemoji-color-font) and fonts made with [scfbuild](https://github.com/13rac1/scfbuild) is SVGinOT
- You will need to add `#define IMGUI_ENABLE_FREETYPE_LIBRSVG` in your `imconfig.h` and install [librsvg](https://github.com/GNOME/librsvg).
- Get latest librsvg binaries or build yourself. Under Windows you may use vcpkg with: `vcpkg install freetype --triplet=x64-windows` and add the following additional include directories:
    ```
    $(VcpkgRoot)\installed\$(VcpkgTriplet)\include\cairo;
    $(VcpkgRoot)\installed\$(VcpkgTriplet)\include\glib-2.0;
    $(VcpkgRoot)\installed\$(VcpkgTriplet)\include\gdk-pixbuf-2.0;
    $(VcpkgRoot)\installed\$(VcpkgTriplet)\lib\glib-2.0\include;
    ```
- Note that the vcpkg version is extremely outdated (v2.40.20 from 2016) so you may want to build the latest version instead. Be aware that the vcpkg version will use `rsvg_handle_get_dimensions` instead of the more accurate `rsvg_handle_get_intrinsic_dimensions` function which is not available in v2.40, [see more](https://gnome.pages.gitlab.gnome.org/librsvg/Rsvg-2.0/method.Handle.get_dimensions.html).