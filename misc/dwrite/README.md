# imgui_dwrite

This implements font rasterization using DirectWrite instead of stb_truetype (otherwise the default).
<br>by @lhecker.

The minimum requirement is Windows 10 1709 (build 16299).

## Usage

1. Add imgui_dwrite.h/cpp alongside your project files.
2. Either set `#define IMGUI_ENABLE_DWRITE` in your [imconfig.h](https://github.com/ocornut/imgui/blob/master/imconfig.h) file or call `ImGui::GetIO().Fonts->SetFontLoader(ImGuiDWrite::GetFontLoader())`.
3. Add fonts using `ImGui::GetIO().Fonts->AddFontFromFamily("Segoe UI")`.

## The Good

- An improved rasterization over stb_truetype (particularly at small font sizes).
- You can use system fonts.
- It supports font fallback.

## The Bad

- Loads font families using regular weight, normal stretch, and normal style only.
- Not all `ImFontConfig` rasterizer options are implemented.
- `IMGUI_USE_WCHAR32` is required to render codepoints above U+FFFF, such as emoji.
  Multi-codepoint glyphs are not supported.
- `ProggyForever.ttf` will not look good.
  If you're curious: This is because the font does not set `USE_TYPO_METRICS`, which causes DirectWrite to correctly use
  the `OS/2` `usWinAscent`/`Descent` fields instead of `sTypoAscender`/`Descender`. stb_truetype on the other hand uses
  the `hhea` table (OpenType is an old standard and has competing metrics tables) whose values coincide with the latter.
  This causes the glyphs to be nice and large with stb_truetype and small and squished with DirectWrite.

...and most importantly: **Gamma correct text blending is not supported**.
DirectWrite relies on gamma correction being applied by a shader during composition.
A variant of this can be found at https://github.com/lhecker/dwrite-hlsl but this cannot be trivially integrated into ImGui.
