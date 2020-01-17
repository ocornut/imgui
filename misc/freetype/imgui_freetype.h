// Wrapper to use FreeType (instead of stb_truetype) for Dear ImGui
// Get latest version at https://github.com/ocornut/imgui/tree/master/misc/freetype
// Original code by @Vuhdo (Aleksei Skriabin), maintained by @ocornut

#pragma once

#include "imgui.h"      // IMGUI_API, ImFontAtlas

namespace ImGuiFreeType
{
    // Hinting greatly impacts visuals (and glyph sizes).
    // When disabled, FreeType generates blurrier glyphs, more or less matches the stb's output.
    // The Default hinting mode usually looks good, but may distort glyphs in an unusual way.
    // The Light hinting mode generates fuzzier glyphs but better matches Microsoft's rasterizer.

    // You can set those flags on a per font basis in ImFontConfig::RasterizerFlags.
    // Use the 'extra_flags' parameter of BuildFontAtlas() to force a flag on all your fonts.
    enum RasterizerFlags
    {
        // Hinter options. By default, PreferFontHinter is enabled.
        PreferFontHinter     = 1 << 0,   // Indicates that the font's native hinter is prefered over the auto hinter.
        PreferAutoHinter     = 1 << 1,   // Indicates that the auto-hinter is prefered over the font's native hinter.
        NoAutoHinter         = 1 << 2,   // Disable auto-hinter.
        NoHinter             = 1 << 3,   // Disable hinting. This generally generates 'blurrier' bitmap glyphs when the glyph are rendered in any of the anti-aliased modes.

        // Hinting algorithms. By default, GrayHinting is enabled.
        GrayHinting          = 1 << 4,   // Hinting algorithm for gray-level rendering.
        LightHinting         = 1 << 5,   // A lighter hinting algorithm for gray-level modes. Many generated glyphs are fuzzier but better resemble their original shape. This is achieved by snapping glyphs to the pixel grid only vertically (Y-axis), as is done by Microsoft's ClearType and Adobe's proprietary font renderer. This preserves inter-glyph spacing in horizontal text.
        MonoHinting          = 1 << 6,   // Strong hinting algorithm that should only be used for monochrome output.
        LcdHinting           = 1 << 7,   // Optimal hinting for horizontally decimated LCD displays.
        LcdVHinting          = 1 << 8,   // Optimal hinting for vertically decimated LCD displays.

        // Render mode. By default, GrayMode is enabled.
        GrayMode             = 1 << 9,   // 8-bit anti-aliased gray-level output.
        MonoMode             = 1 << 10,  // 8-bit aliased monochrome output (either 0x00 or 0xff). Combine this with MonoHinting for best results!
        LcdMode              = 1 << 11,  // Horizontal LCD output. Enable subpixel rendering by exploiting the color-striped structure of LCD pixels, increasing the available resolution in the direction of the stripe by a factor of 3. Mutually exclusive with Monochrome and LcdV. Combine this with LcdHinting for best results! Implementations must retrieve the resulting RGBA32 texture data with GetTexDataAsRGBA32().
        LcdVMode             = 1 << 12,  // Vertical LCD output. Same as LcdMode but for vertically decimated LCD displays (or when a text must be rendered rotated by 90° or 270°). Mutually exclusive with Monochrome and Lcd. Combine this with LcdVHinting for best results! Implementations must retrieve the resulting RGBA32 texture data with GetTexDataAsRGBA32().

        // Various options.
        Bold                 = 1 << 13,  // Styling: Should we artificially embolden the font?
        Oblique              = 1 << 14,  // Styling: Should we slant the font, emulating italic style?
        LcdLightFilter       = 1 << 15,  // A sharper LCD filter but less tolerant of uncalibrated screens.

        HinterMask           = PreferFontHinter | PreferAutoHinter | NoAutoHinter | NoHinter,
        HintingAlgorithmMask = GrayHinting | LightHinting | MonoHinting | LcdHinting | LcdVHinting,
        RenderModeMask       = GrayMode | MonoMode | LcdMode | LcdVMode,
        OptionsMask          = Bold | Oblique | LcdLightFilter
    };
    enum RasterizerFlagsObsolete
    {
        NoHinting            = NoHinter,
        NoAutoHint           = NoAutoHinter,
        ForceAutoHint        = PreferAutoHinter,
        Monochrome           = MonoMode
    };

    IMGUI_API bool BuildFontAtlas(ImFontAtlas* atlas, unsigned int extra_flags = 0);

    // By default ImGuiFreeType will use IM_ALLOC()/IM_FREE().
    // However, as FreeType does lots of allocations we provide a way for the user to redirect it to a separate memory heap if desired:
    IMGUI_API void SetAllocatorFunctions(void* (*alloc_func)(size_t sz, void* user_data), void (*free_func)(void* ptr, void* user_data), void* user_data = NULL);
}
