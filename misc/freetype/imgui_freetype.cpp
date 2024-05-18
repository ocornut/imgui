// dear imgui: FreeType font builder (used as a replacement for the stb_truetype builder)
// (code)

// Get the latest version at https://github.com/ocornut/imgui/tree/master/misc/freetype
// Original code by @vuhdo (Aleksei Skriabin). Improvements by @mikesart. Maintained since 2019 by @ocornut.

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2023/11/13: added support for ImFontConfig::RasterizationDensity field for scaling render density without scaling metrics.
//  2023/08/01: added support for SVG fonts, enable by using '#define IMGUI_ENABLE_FREETYPE_LUNASVG' (#6591)
//  2023/01/04: fixed a packing issue which in some occurrences would prevent large amount of glyphs from being packed correctly.
//  2021/08/23: fixed crash when FT_Render_Glyph() fails to render a glyph and returns NULL.
//  2021/03/05: added ImGuiFreeTypeBuilderFlags_Bitmap to load bitmap glyphs.
//  2021/03/02: set 'atlas->TexPixelsUseColors = true' to help some backends with deciding of a prefered texture format.
//  2021/01/28: added support for color-layered glyphs via ImGuiFreeTypeBuilderFlags_LoadColor (require Freetype 2.10+).
//  2021/01/26: simplified integration by using '#define IMGUI_ENABLE_FREETYPE'. renamed ImGuiFreeType::XXX flags to ImGuiFreeTypeBuilderFlags_XXX for consistency with other API. removed ImGuiFreeType::BuildFontAtlas().
//  2020/06/04: fix for rare case where FT_Get_Char_Index() succeed but FT_Load_Glyph() fails.
//  2019/02/09: added RasterizerFlags::Monochrome flag to disable font anti-aliasing (combine with ::MonoHinting for best results!)
//  2019/01/15: added support for imgui allocators + added FreeType only override function SetAllocatorFunctions().
//  2019/01/10: re-factored to match big update in STB builder. fixed texture height waste. fixed redundant glyphs when merging. support for glyph padding.
//  2018/06/08: added support for ImFontConfig::GlyphMinAdvanceX, GlyphMaxAdvanceX.
//  2018/02/04: moved to main imgui repository (away from http://www.github.com/ocornut/imgui_club)
//  2018/01/22: fix for addition of ImFontAtlas::TexUvscale member.
//  2017/10/22: minor inconsequential change to match change in master (removed an unnecessary statement).
//  2017/09/26: fixes for imgui internal changes.
//  2017/08/26: cleanup, optimizations, support for ImFontConfig::RasterizerFlags, ImFontConfig::RasterizerMultiply.
//  2017/08/16: imported from https://github.com/Vuhdo/imgui_freetype into http://www.github.com/ocornut/imgui_club, updated for latest changes in ImFontAtlas, minor tweaks.

// About Gamma Correct Blending:
// - FreeType assumes blending in linear space rather than gamma space.
// - See https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_Render_Glyph
// - For correct results you need to be using sRGB and convert to linear space in the pixel shader output.
// - The default dear imgui styles will be impacted by this change (alpha values will need tweaking).

// FIXME: cfg.OversampleH, OversampleV are not supported (but perhaps not so necessary with this rasterizer).

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_freetype.h"
#include "imgui_internal.h"     // ImMin,ImMax,ImFontAtlasBuild*,
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H          // <freetype/freetype.h>
#include FT_MODULE_H            // <freetype/ftmodapi.h>
#include FT_GLYPH_H             // <freetype/ftglyph.h>
#include FT_SYNTHESIS_H         // <freetype/ftsynth.h>

#ifdef IMGUI_ENABLE_FREETYPE_LUNASVG
#include FT_OTSVG_H             // <freetype/otsvg.h>
#include FT_BBOX_H              // <freetype/ftbbox.h>
#include <lunasvg.h>
#if !((FREETYPE_MAJOR >= 2) && (FREETYPE_MINOR >= 12))
#error IMGUI_ENABLE_FREETYPE_LUNASVG requires FreeType version >= 2.12
#endif
#endif

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4505)     // unreferenced local function has been removed (stb stuff)
#pragma warning (disable: 26812)    // [Static Analyzer] The enum type 'xxx' is unscoped. Prefer 'enum class' over 'enum' (Enum.3).
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"                  // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wsubobject-linkage"        // warning: 'xxxx' has a field 'xxxx' whose type uses the anonymous namespace
#endif
#endif

//-------------------------------------------------------------------------
// Data
//-------------------------------------------------------------------------

// Default memory allocators
static void* ImGuiFreeTypeDefaultAllocFunc(size_t size, void* user_data) { IM_UNUSED(user_data); return IM_ALLOC(size); }
static void  ImGuiFreeTypeDefaultFreeFunc(void* ptr, void* user_data) { IM_UNUSED(user_data); IM_FREE(ptr); }

// Current memory allocators
static void* (*GImGuiFreeTypeAllocFunc)(size_t size, void* user_data) = ImGuiFreeTypeDefaultAllocFunc;
static void  (*GImGuiFreeTypeFreeFunc)(void* ptr, void* user_data) = ImGuiFreeTypeDefaultFreeFunc;
static void* GImGuiFreeTypeAllocatorUserData = nullptr;

// Lunasvg support
#ifdef IMGUI_ENABLE_FREETYPE_LUNASVG
static FT_Error ImGuiLunasvgPortInit(FT_Pointer* state);
static void     ImGuiLunasvgPortFree(FT_Pointer* state);
static FT_Error ImGuiLunasvgPortRender(FT_GlyphSlot slot, FT_Pointer* _state);
static FT_Error ImGuiLunasvgPortPresetSlot(FT_GlyphSlot slot, FT_Bool cache, FT_Pointer* _state);
#endif

//-------------------------------------------------------------------------
// Code
//-------------------------------------------------------------------------

namespace
{
    // Glyph metrics:
    // --------------
    //
    //                       xmin                     xmax
    //                        |                         |
    //                        |<-------- width -------->|
    //                        |                         |
    //              |         +-------------------------+----------------- ymax
    //              |         |    ggggggggg   ggggg    |     ^        ^
    //              |         |   g:::::::::ggg::::g    |     |        |
    //              |         |  g:::::::::::::::::g    |     |        |
    //              |         | g::::::ggggg::::::gg    |     |        |
    //              |         | g:::::g     g:::::g     |     |        |
    //    offsetX  -|-------->| g:::::g     g:::::g     |  offsetY     |
    //              |         | g:::::g     g:::::g     |     |        |
    //              |         | g::::::g    g:::::g     |     |        |
    //              |         | g:::::::ggggg:::::g     |     |        |
    //              |         |  g::::::::::::::::g     |     |      height
    //              |         |   gg::::::::::::::g     |     |        |
    //  baseline ---*---------|---- gggggggg::::::g-----*--------      |
    //            / |         |             g:::::g     |              |
    //     origin   |         | gggggg      g:::::g     |              |
    //              |         | g:::::gg   gg:::::g     |              |
    //              |         |  g::::::ggg:::::::g     |              |
    //              |         |   gg:::::::::::::g      |              |
    //              |         |     ggg::::::ggg        |              |
    //              |         |         gggggg          |              v
    //              |         +-------------------------+----------------- ymin
    //              |                                   |
    //              |------------- advanceX ----------->|

    // A structure that describe a glyph.
    struct GlyphInfo
    {
        int         Width;              // Glyph's width in pixels.
        int         Height;             // Glyph's height in pixels.
        FT_Int      OffsetX;            // The distance from the origin ("pen position") to the left of the glyph.
        FT_Int      OffsetY;            // The distance from the origin to the top of the glyph. This is usually a value < 0.
        float       AdvanceX;           // The distance from the origin to the origin of the next glyph. This is usually a value > 0.
        bool        IsColored;          // The glyph is colored
    };

    // Font parameters and metrics.
    struct FontInfo
    {
        uint32_t    PixelHeight;        // Size this font was generated with.
        float       Ascender;           // The pixel extents above the baseline in pixels (typically positive).
        float       Descender;          // The extents below the baseline in pixels (typically negative).
        float       LineSpacing;        // The baseline-to-baseline distance. Note that it usually is larger than the sum of the ascender and descender taken as absolute values. There is also no guarantee that no glyphs extend above or below subsequent baselines when using this distance. Think of it as a value the designer of the font finds appropriate.
        float       LineGap;            // The spacing in pixels between one row's descent and the next row's ascent.
        float       MaxAdvanceWidth;    // This field gives the maximum horizontal cursor advance for all glyphs in the font.
    };

    // FreeType glyph rasterizer.
    // NB: No ctor/dtor, explicitly call Init()/Shutdown()
    struct FreeTypeFont
    {
        bool                    InitFont(FT_Library ft_library, const ImFontConfig& cfg, unsigned int extra_user_flags); // Initialize from an external data buffer. Doesn't copy data, and you must ensure it stays valid up to this object lifetime.
        void                    CloseFont();
        void                    SetPixelHeight(int pixel_height); // Change font pixel size. All following calls to RasterizeGlyph() will use this size
        const FT_Glyph_Metrics* LoadGlyph(uint32_t in_codepoint);
        const FT_Bitmap*        RenderGlyphAndGetInfo(GlyphInfo* out_glyph_info);
        void                    BlitGlyph(const FT_Bitmap* ft_bitmap, uint32_t* dst, uint32_t dst_pitch, unsigned char* multiply_table = nullptr);
        ~FreeTypeFont()         { CloseFont(); }

        // [Internals]
        FontInfo        Info;               // Font descriptor of the current font.
        FT_Face         Face;
        unsigned int    UserFlags;          // = ImFontConfig::RasterizerFlags
        FT_Int32        LoadFlags;
        FT_Render_Mode  RenderMode;
        float           RasterizationDensity;
        float           InvRasterizationDensity;
    };

    // From SDL_ttf: Handy routines for converting from fixed point
    #define FT_CEIL(X)  (((X + 63) & -64) / 64)

    bool FreeTypeFont::InitFont(FT_Library ft_library, const ImFontConfig& cfg, unsigned int extra_font_builder_flags)
    {
        FT_Error error = FT_New_Memory_Face(ft_library, (uint8_t*)cfg.FontData, (uint32_t)cfg.FontDataSize, (uint32_t)cfg.FontNo, &Face);
        if (error != 0)
            return false;
        error = FT_Select_Charmap(Face, FT_ENCODING_UNICODE);
        if (error != 0)
            return false;

        // Convert to FreeType flags (NB: Bold and Oblique are processed separately)
        UserFlags = cfg.FontBuilderFlags | extra_font_builder_flags;

        LoadFlags = 0;
        if ((UserFlags & ImGuiFreeTypeBuilderFlags_Bitmap) == 0)
            LoadFlags |= FT_LOAD_NO_BITMAP;

        if (UserFlags & ImGuiFreeTypeBuilderFlags_NoHinting)
            LoadFlags |= FT_LOAD_NO_HINTING;
        if (UserFlags & ImGuiFreeTypeBuilderFlags_NoAutoHint)
            LoadFlags |= FT_LOAD_NO_AUTOHINT;
        if (UserFlags & ImGuiFreeTypeBuilderFlags_ForceAutoHint)
            LoadFlags |= FT_LOAD_FORCE_AUTOHINT;
        if (UserFlags & ImGuiFreeTypeBuilderFlags_LightHinting)
            LoadFlags |= FT_LOAD_TARGET_LIGHT;
        else if (UserFlags & ImGuiFreeTypeBuilderFlags_MonoHinting)
            LoadFlags |= FT_LOAD_TARGET_MONO;
        else
            LoadFlags |= FT_LOAD_TARGET_NORMAL;

        if (UserFlags & ImGuiFreeTypeBuilderFlags_Monochrome)
            RenderMode = FT_RENDER_MODE_MONO;
        else
            RenderMode = FT_RENDER_MODE_NORMAL;

        if (UserFlags & ImGuiFreeTypeBuilderFlags_LoadColor)
            LoadFlags |= FT_LOAD_COLOR;

        RasterizationDensity = cfg.RasterizerDensity;
        InvRasterizationDensity = 1.0f / RasterizationDensity;

        memset(&Info, 0, sizeof(Info));
        SetPixelHeight((uint32_t)cfg.SizePixels);

        return true;
    }

    void FreeTypeFont::CloseFont()
    {
        if (Face)
        {
            FT_Done_Face(Face);
            Face = nullptr;
        }
    }

    void FreeTypeFont::SetPixelHeight(int pixel_height)
    {
        // Vuhdo: I'm not sure how to deal with font sizes properly. As far as I understand, currently ImGui assumes that the 'pixel_height'
        // is a maximum height of an any given glyph, i.e. it's the sum of font's ascender and descender. Seems strange to me.
        // NB: FT_Set_Pixel_Sizes() doesn't seem to get us the same result.
        FT_Size_RequestRec req;
        req.type = (UserFlags & ImGuiFreeTypeBuilderFlags_Bitmap) ? FT_SIZE_REQUEST_TYPE_NOMINAL : FT_SIZE_REQUEST_TYPE_REAL_DIM;
        req.width = 0;
        req.height = (uint32_t)(pixel_height * 64 * RasterizationDensity);
        req.horiResolution = 0;
        req.vertResolution = 0;
        FT_Request_Size(Face, &req);

        // Update font info
        FT_Size_Metrics metrics = Face->size->metrics;
        Info.PixelHeight = (uint32_t)(pixel_height * InvRasterizationDensity);
        Info.Ascender = (float)FT_CEIL(metrics.ascender) * InvRasterizationDensity;
        Info.Descender = (float)FT_CEIL(metrics.descender) * InvRasterizationDensity;
        Info.LineSpacing = (float)FT_CEIL(metrics.height) * InvRasterizationDensity;
        Info.LineGap = (float)FT_CEIL(metrics.height - metrics.ascender + metrics.descender) * InvRasterizationDensity;
        Info.MaxAdvanceWidth = (float)FT_CEIL(metrics.max_advance) * InvRasterizationDensity;
    }

    const FT_Glyph_Metrics* FreeTypeFont::LoadGlyph(uint32_t codepoint)
    {
        uint32_t glyph_index = FT_Get_Char_Index(Face, codepoint);
        if (glyph_index == 0)
            return nullptr;

		// If this crash for you: FreeType 2.11.0 has a crash bug on some bitmap/colored fonts.
		// - https://gitlab.freedesktop.org/freetype/freetype/-/issues/1076
		// - https://github.com/ocornut/imgui/issues/4567
		// - https://github.com/ocornut/imgui/issues/4566
		// You can use FreeType 2.10, or the patched version of 2.11.0 in VcPkg, or probably any upcoming FreeType version.
        FT_Error error = FT_Load_Glyph(Face, glyph_index, LoadFlags);
        if (error)
            return nullptr;

        // Need an outline for this to work
        FT_GlyphSlot slot = Face->glyph;
#ifdef IMGUI_ENABLE_FREETYPE_LUNASVG
        IM_ASSERT(slot->format == FT_GLYPH_FORMAT_OUTLINE || slot->format == FT_GLYPH_FORMAT_BITMAP || slot->format == FT_GLYPH_FORMAT_SVG);
#else
#if ((FREETYPE_MAJOR >= 2) && (FREETYPE_MINOR >= 12))
        IM_ASSERT(slot->format != FT_GLYPH_FORMAT_SVG && "The font contains SVG glyphs, you'll need to enable IMGUI_ENABLE_FREETYPE_LUNASVG in imconfig.h and install required libraries in order to use this font");
#endif
        IM_ASSERT(slot->format == FT_GLYPH_FORMAT_OUTLINE || slot->format == FT_GLYPH_FORMAT_BITMAP);
#endif // IMGUI_ENABLE_FREETYPE_LUNASVG

        // Apply convenience transform (this is not picking from real "Bold"/"Italic" fonts! Merely applying FreeType helper transform. Oblique == Slanting)
        if (UserFlags & ImGuiFreeTypeBuilderFlags_Bold)
            FT_GlyphSlot_Embolden(slot);
        if (UserFlags & ImGuiFreeTypeBuilderFlags_Oblique)
        {
            FT_GlyphSlot_Oblique(slot);
            //FT_BBox bbox;
            //FT_Outline_Get_BBox(&slot->outline, &bbox);
            //slot->metrics.width = bbox.xMax - bbox.xMin;
            //slot->metrics.height = bbox.yMax - bbox.yMin;
        }

        return &slot->metrics;
    }

    const FT_Bitmap* FreeTypeFont::RenderGlyphAndGetInfo(GlyphInfo* out_glyph_info)
    {
        FT_GlyphSlot slot = Face->glyph;
        FT_Error error = FT_Render_Glyph(slot, RenderMode);
        if (error != 0)
            return nullptr;

        FT_Bitmap* ft_bitmap = &Face->glyph->bitmap;
        out_glyph_info->Width = (int)ft_bitmap->width;
        out_glyph_info->Height = (int)ft_bitmap->rows;
        out_glyph_info->OffsetX = Face->glyph->bitmap_left;
        out_glyph_info->OffsetY = -Face->glyph->bitmap_top;
        out_glyph_info->AdvanceX = (float)FT_CEIL(slot->advance.x);
        out_glyph_info->IsColored = (ft_bitmap->pixel_mode == FT_PIXEL_MODE_BGRA);

        return ft_bitmap;
    }

    void FreeTypeFont::BlitGlyph(const FT_Bitmap* ft_bitmap, uint32_t* dst, uint32_t dst_pitch, unsigned char* multiply_table)
    {
        IM_ASSERT(ft_bitmap != nullptr);
        const uint32_t w = ft_bitmap->width;
        const uint32_t h = ft_bitmap->rows;
        const uint8_t* src = ft_bitmap->buffer;
        const uint32_t src_pitch = ft_bitmap->pitch;

        switch (ft_bitmap->pixel_mode)
        {
        case FT_PIXEL_MODE_GRAY: // Grayscale image, 1 byte per pixel.
            {
                if (multiply_table == nullptr)
                {
                    for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                        for (uint32_t x = 0; x < w; x++)
                            dst[x] = IM_COL32(255, 255, 255, src[x]);
                }
                else
                {
                    for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                        for (uint32_t x = 0; x < w; x++)
                            dst[x] = IM_COL32(255, 255, 255, multiply_table[src[x]]);
                }
                break;
            }
        case FT_PIXEL_MODE_MONO: // Monochrome image, 1 bit per pixel. The bits in each byte are ordered from MSB to LSB.
            {
                uint8_t color0 = multiply_table ? multiply_table[0] : 0;
                uint8_t color1 = multiply_table ? multiply_table[255] : 255;
                for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                {
                    uint8_t bits = 0;
                    const uint8_t* bits_ptr = src;
                    for (uint32_t x = 0; x < w; x++, bits <<= 1)
                    {
                        if ((x & 7) == 0)
                            bits = *bits_ptr++;
                        dst[x] = IM_COL32(255, 255, 255, (bits & 0x80) ? color1 : color0);
                    }
                }
                break;
            }
        case FT_PIXEL_MODE_BGRA:
            {
                // FIXME: Converting pre-multiplied alpha to straight. Doesn't smell good.
                #define DE_MULTIPLY(color, alpha) (ImU32)(255.0f * (float)color / (float)alpha + 0.5f)
                if (multiply_table == nullptr)
                {
                    for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                        for (uint32_t x = 0; x < w; x++)
                        {
                            uint8_t r = src[x * 4 + 2], g = src[x * 4 + 1], b = src[x * 4], a = src[x * 4 + 3];
                            dst[x] = IM_COL32(DE_MULTIPLY(r, a), DE_MULTIPLY(g, a), DE_MULTIPLY(b, a), a);
                        }
                }
                else
                {
                    for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                    {
                        for (uint32_t x = 0; x < w; x++)
                        {
                            uint8_t r = src[x * 4 + 2], g = src[x * 4 + 1], b = src[x * 4], a = src[x * 4 + 3];
                            dst[x] = IM_COL32(multiply_table[DE_MULTIPLY(r, a)], multiply_table[DE_MULTIPLY(g, a)], multiply_table[DE_MULTIPLY(b, a)], multiply_table[a]);
                        }
                    }
                }
                #undef DE_MULTIPLY
                break;
            }
        default:
            IM_ASSERT(0 && "FreeTypeFont::BlitGlyph(): Unknown bitmap pixel mode!");
        }
    }
} // namespace

#ifndef STB_RECT_PACK_IMPLEMENTATION                        // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define STBRP_ASSERT(x)     do { IM_ASSERT(x); } while (0)
#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#ifdef IMGUI_STB_RECT_PACK_FILENAME
#include IMGUI_STB_RECT_PACK_FILENAME
#else
#include "imstb_rectpack.h"
#endif
#endif

struct ImFontBuildSrcGlyphFT
{
    GlyphInfo           Info;
    uint32_t            Codepoint;
    unsigned int*       BitmapData;         // Point within one of the dst_tmp_bitmap_buffers[] array

    ImFontBuildSrcGlyphFT() { memset((void*)this, 0, sizeof(*this)); }
};

struct ImFontBuildSrcDataFT
{
    FreeTypeFont        Font;
    stbrp_rect*         Rects;              // Rectangle to pack. We first fill in their size and the packer will give us their position.
    const ImWchar*      SrcRanges;          // Ranges as requested by user (user is allowed to request too much, e.g. 0x0020..0xFFFF)
    int                 DstIndex;           // Index into atlas->Fonts[] and dst_tmp_array[]
    int                 GlyphsHighest;      // Highest requested codepoint
    int                 GlyphsCount;        // Glyph count (excluding missing glyphs and glyphs already set by an earlier source font)
    ImBitVector         GlyphsSet;          // Glyph bit map (random access, 1-bit per codepoint. This will be a maximum of 8KB)
    ImVector<ImFontBuildSrcGlyphFT>   GlyphsList;
};

// Temporary data for one destination ImFont* (multiple source fonts can be merged into one destination ImFont)
struct ImFontBuildDstDataFT
{
    int                 SrcCount;           // Number of source fonts targeting this destination font.
    int                 GlyphsHighest;
    int                 GlyphsCount;
    ImBitVector         GlyphsSet;          // This is used to resolve collision when multiple sources are merged into a same destination font.
};

bool ImFontAtlasBuildWithFreeTypeEx(FT_Library ft_library, ImFontAtlas* atlas, unsigned int extra_flags)
{
    IM_ASSERT(atlas->ConfigData.Size > 0);

    ImFontAtlasBuildInit(atlas);

    // Clear atlas
    atlas->TexID = 0;
    atlas->TexWidth = atlas->TexHeight = 0;
    atlas->TexUvScale = ImVec2(0.0f, 0.0f);
    atlas->TexUvWhitePixel = ImVec2(0.0f, 0.0f);
    atlas->ClearTexData();

    // Temporary storage for building
    bool src_load_color = false;
    ImVector<ImFontBuildSrcDataFT> src_tmp_array;
    ImVector<ImFontBuildDstDataFT> dst_tmp_array;
    src_tmp_array.resize(atlas->ConfigData.Size);
    dst_tmp_array.resize(atlas->Fonts.Size);
    memset((void*)src_tmp_array.Data, 0, (size_t)src_tmp_array.size_in_bytes());
    memset((void*)dst_tmp_array.Data, 0, (size_t)dst_tmp_array.size_in_bytes());

    // 1. Initialize font loading structure, check font data validity
    for (int src_i = 0; src_i < atlas->ConfigData.Size; src_i++)
    {
        ImFontBuildSrcDataFT& src_tmp = src_tmp_array[src_i];
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        FreeTypeFont& font_face = src_tmp.Font;
        IM_ASSERT(cfg.DstFont && (!cfg.DstFont->IsLoaded() || cfg.DstFont->ContainerAtlas == atlas));

        // Find index from cfg.DstFont (we allow the user to set cfg.DstFont. Also it makes casual debugging nicer than when storing indices)
        src_tmp.DstIndex = -1;
        for (int output_i = 0; output_i < atlas->Fonts.Size && src_tmp.DstIndex == -1; output_i++)
            if (cfg.DstFont == atlas->Fonts[output_i])
                src_tmp.DstIndex = output_i;
        IM_ASSERT(src_tmp.DstIndex != -1); // cfg.DstFont not pointing within atlas->Fonts[] array?
        if (src_tmp.DstIndex == -1)
            return false;

        // Load font
        if (!font_face.InitFont(ft_library, cfg, extra_flags))
            return false;

        // Measure highest codepoints
        src_load_color |= (cfg.FontBuilderFlags & ImGuiFreeTypeBuilderFlags_LoadColor) != 0;
        ImFontBuildDstDataFT& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        src_tmp.SrcRanges = cfg.GlyphRanges ? cfg.GlyphRanges : atlas->GetGlyphRangesDefault();
        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
        {
            // Check for valid range. This may also help detect *some* dangling pointers, because a common
            // user error is to setup ImFontConfig::GlyphRanges with a pointer to data that isn't persistent.
            IM_ASSERT(src_range[0] <= src_range[1]);
            src_tmp.GlyphsHighest = ImMax(src_tmp.GlyphsHighest, (int)src_range[1]);
        }
        dst_tmp.SrcCount++;
        dst_tmp.GlyphsHighest = ImMax(dst_tmp.GlyphsHighest, src_tmp.GlyphsHighest);
    }

    // 2. For every requested codepoint, check for their presence in the font data, and handle redundancy or overlaps between source fonts to avoid unused glyphs.
    int total_glyphs_count = 0;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcDataFT& src_tmp = src_tmp_array[src_i];
        ImFontBuildDstDataFT& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        src_tmp.GlyphsSet.Create(src_tmp.GlyphsHighest + 1);
        if (dst_tmp.GlyphsSet.Storage.empty())
            dst_tmp.GlyphsSet.Create(dst_tmp.GlyphsHighest + 1);

        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
            for (int codepoint = src_range[0]; codepoint <= (int)src_range[1]; codepoint++)
            {
                if (dst_tmp.GlyphsSet.TestBit(codepoint))    // Don't overwrite existing glyphs. We could make this an option (e.g. MergeOverwrite)
                    continue;
                uint32_t glyph_index = FT_Get_Char_Index(src_tmp.Font.Face, codepoint); // It is actually in the font? (FIXME-OPT: We are not storing the glyph_index..)
                if (glyph_index == 0)
                    continue;

                // Add to avail set/counters
                src_tmp.GlyphsCount++;
                dst_tmp.GlyphsCount++;
                src_tmp.GlyphsSet.SetBit(codepoint);
                dst_tmp.GlyphsSet.SetBit(codepoint);
                total_glyphs_count++;
            }
    }

    // 3. Unpack our bit map into a flat list (we now have all the Unicode points that we know are requested _and_ available _and_ not overlapping another)
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcDataFT& src_tmp = src_tmp_array[src_i];
        src_tmp.GlyphsList.reserve(src_tmp.GlyphsCount);

        IM_ASSERT(sizeof(src_tmp.GlyphsSet.Storage.Data[0]) == sizeof(ImU32));
        const ImU32* it_begin = src_tmp.GlyphsSet.Storage.begin();
        const ImU32* it_end = src_tmp.GlyphsSet.Storage.end();
        for (const ImU32* it = it_begin; it < it_end; it++)
            if (ImU32 entries_32 = *it)
                for (ImU32 bit_n = 0; bit_n < 32; bit_n++)
                    if (entries_32 & ((ImU32)1 << bit_n))
                    {
                        ImFontBuildSrcGlyphFT src_glyph;
                        src_glyph.Codepoint = (ImWchar)(((it - it_begin) << 5) + bit_n);
                        //src_glyph.GlyphIndex = 0; // FIXME-OPT: We had this info in the previous step and lost it..
                        src_tmp.GlyphsList.push_back(src_glyph);
                    }
        src_tmp.GlyphsSet.Clear();
        IM_ASSERT(src_tmp.GlyphsList.Size == src_tmp.GlyphsCount);
    }
    for (int dst_i = 0; dst_i < dst_tmp_array.Size; dst_i++)
        dst_tmp_array[dst_i].GlyphsSet.Clear();
    dst_tmp_array.clear();

    // Allocate packing character data and flag packed characters buffer as non-packed (x0=y0=x1=y1=0)
    // (We technically don't need to zero-clear buf_rects, but let's do it for the sake of sanity)
    ImVector<stbrp_rect> buf_rects;
    buf_rects.resize(total_glyphs_count);
    memset(buf_rects.Data, 0, (size_t)buf_rects.size_in_bytes());

    // Allocate temporary rasterization data buffers.
    // We could not find a way to retrieve accurate glyph size without rendering them.
    // (e.g. slot->metrics->width not always matching bitmap->width, especially considering the Oblique transform)
    // We allocate in chunks of 256 KB to not waste too much extra memory ahead. Hopefully users of FreeType won't mind the temporary allocations.
    const int BITMAP_BUFFERS_CHUNK_SIZE = 256 * 1024;
    int buf_bitmap_current_used_bytes = 0;
    ImVector<unsigned char*> buf_bitmap_buffers;
    buf_bitmap_buffers.push_back((unsigned char*)IM_ALLOC(BITMAP_BUFFERS_CHUNK_SIZE));

    // 4. Gather glyphs sizes so we can pack them in our virtual canvas.
    // 8. Render/rasterize font characters into the texture
    int total_surface = 0;
    int buf_rects_out_n = 0;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcDataFT& src_tmp = src_tmp_array[src_i];
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        src_tmp.Rects = &buf_rects[buf_rects_out_n];
        buf_rects_out_n += src_tmp.GlyphsCount;

        // Compute multiply table if requested
        const bool multiply_enabled = (cfg.RasterizerMultiply != 1.0f);
        unsigned char multiply_table[256];
        if (multiply_enabled)
            ImFontAtlasBuildMultiplyCalcLookupTable(multiply_table, cfg.RasterizerMultiply);

        // Gather the sizes of all rectangles we will need to pack
        const int padding = atlas->TexGlyphPadding;
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsList.Size; glyph_i++)
        {
            ImFontBuildSrcGlyphFT& src_glyph = src_tmp.GlyphsList[glyph_i];

            const FT_Glyph_Metrics* metrics = src_tmp.Font.LoadGlyph(src_glyph.Codepoint);
            if (metrics == nullptr)
                continue;

            // Render glyph into a bitmap (currently held by FreeType)
            const FT_Bitmap* ft_bitmap = src_tmp.Font.RenderGlyphAndGetInfo(&src_glyph.Info);
            if (ft_bitmap == nullptr)
                continue;

            // Allocate new temporary chunk if needed
            const int bitmap_size_in_bytes = src_glyph.Info.Width * src_glyph.Info.Height * 4;
            if (buf_bitmap_current_used_bytes + bitmap_size_in_bytes > BITMAP_BUFFERS_CHUNK_SIZE)
            {
                buf_bitmap_current_used_bytes = 0;
                buf_bitmap_buffers.push_back((unsigned char*)IM_ALLOC(BITMAP_BUFFERS_CHUNK_SIZE));
            }
            IM_ASSERT(buf_bitmap_current_used_bytes + bitmap_size_in_bytes <= BITMAP_BUFFERS_CHUNK_SIZE); // We could probably allocate custom-sized buffer instead.

            // Blit rasterized pixels to our temporary buffer and keep a pointer to it.
            src_glyph.BitmapData = (unsigned int*)(buf_bitmap_buffers.back() + buf_bitmap_current_used_bytes);
            buf_bitmap_current_used_bytes += bitmap_size_in_bytes;
            src_tmp.Font.BlitGlyph(ft_bitmap, src_glyph.BitmapData, src_glyph.Info.Width, multiply_enabled ? multiply_table : nullptr);

            src_tmp.Rects[glyph_i].w = (stbrp_coord)(src_glyph.Info.Width + padding);
            src_tmp.Rects[glyph_i].h = (stbrp_coord)(src_glyph.Info.Height + padding);
            total_surface += src_tmp.Rects[glyph_i].w * src_tmp.Rects[glyph_i].h;
        }
    }

    // We need a width for the skyline algorithm, any width!
    // The exact width doesn't really matter much, but some API/GPU have texture size limitations and increasing width can decrease height.
    // User can override TexDesiredWidth and TexGlyphPadding if they wish, otherwise we use a simple heuristic to select the width based on expected surface.
    const int surface_sqrt = (int)ImSqrt((float)total_surface) + 1;
    atlas->TexHeight = 0;
    if (atlas->TexDesiredWidth > 0)
        atlas->TexWidth = atlas->TexDesiredWidth;
    else
        atlas->TexWidth = (surface_sqrt >= 4096 * 0.7f) ? 4096 : (surface_sqrt >= 2048 * 0.7f) ? 2048 : (surface_sqrt >= 1024 * 0.7f) ? 1024 : 512;

    // 5. Start packing
    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    const int TEX_HEIGHT_MAX = 1024 * 32;
    const int num_nodes_for_packing_algorithm = atlas->TexWidth - atlas->TexGlyphPadding;
    ImVector<stbrp_node> pack_nodes;
    pack_nodes.resize(num_nodes_for_packing_algorithm);
    stbrp_context pack_context;
    stbrp_init_target(&pack_context, atlas->TexWidth - atlas->TexGlyphPadding, TEX_HEIGHT_MAX - atlas->TexGlyphPadding, pack_nodes.Data, pack_nodes.Size);
    ImFontAtlasBuildPackCustomRects(atlas, &pack_context);

    // 6. Pack each source font. No rendering yet, we are working with rectangles in an infinitely tall texture at this point.
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcDataFT& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        stbrp_pack_rects(&pack_context, src_tmp.Rects, src_tmp.GlyphsCount);

        // Extend texture height and mark missing glyphs as non-packed so we won't render them.
        // FIXME: We are not handling packing failure here (would happen if we got off TEX_HEIGHT_MAX or if a single if larger than TexWidth?)
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
            if (src_tmp.Rects[glyph_i].was_packed)
                atlas->TexHeight = ImMax(atlas->TexHeight, src_tmp.Rects[glyph_i].y + src_tmp.Rects[glyph_i].h);
    }

    // 7. Allocate texture
    atlas->TexHeight = (atlas->Flags & ImFontAtlasFlags_NoPowerOfTwoHeight) ? (atlas->TexHeight + 1) : ImUpperPowerOfTwo(atlas->TexHeight);
    atlas->TexUvScale = ImVec2(1.0f / atlas->TexWidth, 1.0f / atlas->TexHeight);
    if (src_load_color)
    {
        size_t tex_size = (size_t)atlas->TexWidth * atlas->TexHeight * 4;
        atlas->TexPixelsRGBA32 = (unsigned int*)IM_ALLOC(tex_size);
        memset(atlas->TexPixelsRGBA32, 0, tex_size);
    }
    else
    {
        size_t tex_size = (size_t)atlas->TexWidth * atlas->TexHeight * 1;
        atlas->TexPixelsAlpha8 = (unsigned char*)IM_ALLOC(tex_size);
        memset(atlas->TexPixelsAlpha8, 0, tex_size);
    }

    // 8. Copy rasterized font characters back into the main texture
    // 9. Setup ImFont and glyphs for runtime
    bool tex_use_colors = false;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcDataFT& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        // When merging fonts with MergeMode=true:
        // - We can have multiple input fonts writing into a same destination font.
        // - dst_font->ConfigData is != from cfg which is our source configuration.
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        ImFont* dst_font = cfg.DstFont;

        const float ascent = src_tmp.Font.Info.Ascender;
        const float descent = src_tmp.Font.Info.Descender;
        ImFontAtlasBuildSetupFont(atlas, dst_font, &cfg, ascent, descent);
        const float font_off_x = cfg.GlyphOffset.x;
        const float font_off_y = cfg.GlyphOffset.y + IM_ROUND(dst_font->Ascent);

        const int padding = atlas->TexGlyphPadding;
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
        {
            ImFontBuildSrcGlyphFT& src_glyph = src_tmp.GlyphsList[glyph_i];
            stbrp_rect& pack_rect = src_tmp.Rects[glyph_i];
            IM_ASSERT(pack_rect.was_packed);
            if (pack_rect.w == 0 && pack_rect.h == 0)
                continue;

            GlyphInfo& info = src_glyph.Info;
            IM_ASSERT(info.Width + padding <= pack_rect.w);
            IM_ASSERT(info.Height + padding <= pack_rect.h);
            const int tx = pack_rect.x + padding;
            const int ty = pack_rect.y + padding;

            // Register glyph
            float x0 = info.OffsetX * src_tmp.Font.InvRasterizationDensity + font_off_x;
            float y0 = info.OffsetY * src_tmp.Font.InvRasterizationDensity + font_off_y;
            float x1 = x0 + info.Width * src_tmp.Font.InvRasterizationDensity;
            float y1 = y0 + info.Height * src_tmp.Font.InvRasterizationDensity;
            float u0 = (tx) / (float)atlas->TexWidth;
            float v0 = (ty) / (float)atlas->TexHeight;
            float u1 = (tx + info.Width) / (float)atlas->TexWidth;
            float v1 = (ty + info.Height) / (float)atlas->TexHeight;
            dst_font->AddGlyph(&cfg, (ImWchar)src_glyph.Codepoint, x0, y0, x1, y1, u0, v0, u1, v1, info.AdvanceX * src_tmp.Font.InvRasterizationDensity);

            ImFontGlyph* dst_glyph = &dst_font->Glyphs.back();
            IM_ASSERT(dst_glyph->Codepoint == src_glyph.Codepoint);
            if (src_glyph.Info.IsColored)
                dst_glyph->Colored = tex_use_colors = true;

            // Blit from temporary buffer to final texture
            size_t blit_src_stride = (size_t)src_glyph.Info.Width;
            size_t blit_dst_stride = (size_t)atlas->TexWidth;
            unsigned int* blit_src = src_glyph.BitmapData;
            if (atlas->TexPixelsAlpha8 != nullptr)
            {
                unsigned char* blit_dst = atlas->TexPixelsAlpha8 + (ty * blit_dst_stride) + tx;
                for (int y = 0; y < info.Height; y++, blit_dst += blit_dst_stride, blit_src += blit_src_stride)
                    for (int x = 0; x < info.Width; x++)
                        blit_dst[x] = (unsigned char)((blit_src[x] >> IM_COL32_A_SHIFT) & 0xFF);
            }
            else
            {
                unsigned int* blit_dst = atlas->TexPixelsRGBA32 + (ty * blit_dst_stride) + tx;
                for (int y = 0; y < info.Height; y++, blit_dst += blit_dst_stride, blit_src += blit_src_stride)
                    for (int x = 0; x < info.Width; x++)
                        blit_dst[x] = blit_src[x];
            }
        }

        src_tmp.Rects = nullptr;
    }
    atlas->TexPixelsUseColors = tex_use_colors;

    // Cleanup
    for (int buf_i = 0; buf_i < buf_bitmap_buffers.Size; buf_i++)
        IM_FREE(buf_bitmap_buffers[buf_i]);
    src_tmp_array.clear_destruct();

    ImFontAtlasBuildFinish(atlas);

    return true;
}

// FreeType memory allocation callbacks
static void* FreeType_Alloc(FT_Memory /*memory*/, long size)
{
    return GImGuiFreeTypeAllocFunc((size_t)size, GImGuiFreeTypeAllocatorUserData);
}

static void FreeType_Free(FT_Memory /*memory*/, void* block)
{
    GImGuiFreeTypeFreeFunc(block, GImGuiFreeTypeAllocatorUserData);
}

static void* FreeType_Realloc(FT_Memory /*memory*/, long cur_size, long new_size, void* block)
{
    // Implement realloc() as we don't ask user to provide it.
    if (block == nullptr)
        return GImGuiFreeTypeAllocFunc((size_t)new_size, GImGuiFreeTypeAllocatorUserData);

    if (new_size == 0)
    {
        GImGuiFreeTypeFreeFunc(block, GImGuiFreeTypeAllocatorUserData);
        return nullptr;
    }

    if (new_size > cur_size)
    {
        void* new_block = GImGuiFreeTypeAllocFunc((size_t)new_size, GImGuiFreeTypeAllocatorUserData);
        memcpy(new_block, block, (size_t)cur_size);
        GImGuiFreeTypeFreeFunc(block, GImGuiFreeTypeAllocatorUserData);
        return new_block;
    }

    return block;
}

static bool ImFontAtlasBuildWithFreeType(ImFontAtlas* atlas)
{
    // FreeType memory management: https://www.freetype.org/freetype2/docs/design/design-4.html
    FT_MemoryRec_ memory_rec = {};
    memory_rec.user = nullptr;
    memory_rec.alloc = &FreeType_Alloc;
    memory_rec.free = &FreeType_Free;
    memory_rec.realloc = &FreeType_Realloc;

    // https://www.freetype.org/freetype2/docs/reference/ft2-module_management.html#FT_New_Library
    FT_Library ft_library;
    FT_Error error = FT_New_Library(&memory_rec, &ft_library);
    if (error != 0)
        return false;

    // If you don't call FT_Add_Default_Modules() the rest of code may work, but FreeType won't use our custom allocator.
    FT_Add_Default_Modules(ft_library);

#ifdef IMGUI_ENABLE_FREETYPE_LUNASVG
    // Install svg hooks for FreeType
    // https://freetype.org/freetype2/docs/reference/ft2-properties.html#svg-hooks
    // https://freetype.org/freetype2/docs/reference/ft2-svg_fonts.html#svg_fonts
    SVG_RendererHooks hooks = { ImGuiLunasvgPortInit, ImGuiLunasvgPortFree, ImGuiLunasvgPortRender, ImGuiLunasvgPortPresetSlot };
    FT_Property_Set(ft_library, "ot-svg", "svg-hooks", &hooks);
#endif // IMGUI_ENABLE_FREETYPE_LUNASVG

    bool ret = ImFontAtlasBuildWithFreeTypeEx(ft_library, atlas, atlas->FontBuilderFlags);
    FT_Done_Library(ft_library);

    return ret;
}

const ImFontBuilderIO* ImGuiFreeType::GetBuilderForFreeType()
{
    static ImFontBuilderIO io;
    io.FontBuilder_Build = ImFontAtlasBuildWithFreeType;
    return &io;
}

void ImGuiFreeType::SetAllocatorFunctions(void* (*alloc_func)(size_t sz, void* user_data), void (*free_func)(void* ptr, void* user_data), void* user_data)
{
    GImGuiFreeTypeAllocFunc = alloc_func;
    GImGuiFreeTypeFreeFunc = free_func;
    GImGuiFreeTypeAllocatorUserData = user_data;
}

#ifdef IMGUI_ENABLE_FREETYPE_LUNASVG
// For more details, see https://gitlab.freedesktop.org/freetype/freetype-demos/-/blob/master/src/rsvg-port.c
// The original code from the demo is licensed under CeCILL-C Free Software License Agreement (https://gitlab.freedesktop.org/freetype/freetype/-/blob/master/LICENSE.TXT)
struct LunasvgPortState
{
    FT_Error            err = FT_Err_Ok;
    lunasvg::Matrix     matrix;
    std::unique_ptr<lunasvg::Document> svg = nullptr;
};

static FT_Error ImGuiLunasvgPortInit(FT_Pointer* _state)
{
    *_state = IM_NEW(LunasvgPortState)();
    return FT_Err_Ok;
}

static void ImGuiLunasvgPortFree(FT_Pointer* _state)
{
    IM_DELETE(*(LunasvgPortState**)_state);
}

static FT_Error ImGuiLunasvgPortRender(FT_GlyphSlot slot, FT_Pointer* _state)
{
    LunasvgPortState* state = *(LunasvgPortState**)_state;

    // If there was an error while loading the svg in ImGuiLunasvgPortPresetSlot(), the renderer hook still get called, so just returns the error.
    if (state->err != FT_Err_Ok)
        return state->err;

    // rows is height, pitch (or stride) equals to width * sizeof(int32)
    lunasvg::Bitmap bitmap((uint8_t*)slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows, slot->bitmap.pitch);
    state->svg->setMatrix(state->svg->matrix().identity()); // Reset the svg matrix to the default value
    state->svg->render(bitmap, state->matrix);              // state->matrix is already scaled and translated
    state->err = FT_Err_Ok;
    return state->err;
}

static FT_Error ImGuiLunasvgPortPresetSlot(FT_GlyphSlot slot, FT_Bool cache, FT_Pointer* _state)
{
    FT_SVG_Document   document = (FT_SVG_Document)slot->other;
    LunasvgPortState* state = *(LunasvgPortState**)_state;
    FT_Size_Metrics&  metrics = document->metrics;

    // This function is called twice, once in the FT_Load_Glyph() and another right before ImGuiLunasvgPortRender().
    // If it's the latter, don't do anything because it's // already done in the former.
    if (cache)
        return state->err;

    state->svg = lunasvg::Document::loadFromData((const char*)document->svg_document, document->svg_document_length);
    if (state->svg == nullptr)
    {
        state->err = FT_Err_Invalid_SVG_Document;
        return state->err;
    }

    lunasvg::Box box = state->svg->box();
    double scale = std::min(metrics.x_ppem / box.w, metrics.y_ppem / box.h);
    double xx = (double)document->transform.xx / (1 << 16);
    double xy = -(double)document->transform.xy / (1 << 16);
    double yx = -(double)document->transform.yx / (1 << 16);
    double yy = (double)document->transform.yy / (1 << 16);
    double x0 = (double)document->delta.x / 64 * box.w / metrics.x_ppem;
    double y0 = -(double)document->delta.y / 64 * box.h / metrics.y_ppem;

    // Scale and transform, we don't translate the svg yet
    state->matrix.identity();
    state->matrix.scale(scale, scale);
    state->matrix.transform(xx, xy, yx, yy, x0, y0);
    state->svg->setMatrix(state->matrix);

    // Pre-translate the matrix for the rendering step
    state->matrix.translate(-box.x, -box.y);

    // Get the box again after the transformation
    box = state->svg->box();

    // Calculate the bitmap size
    slot->bitmap_left = FT_Int(box.x);
    slot->bitmap_top = FT_Int(-box.y);
    slot->bitmap.rows = (unsigned int)(ImCeil((float)box.h));
    slot->bitmap.width = (unsigned int)(ImCeil((float)box.w));
    slot->bitmap.pitch = slot->bitmap.width * 4;
    slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;

    // Compute all the bearings and set them correctly. The outline is scaled already, we just need to use the bounding box.
    double metrics_width = box.w;
    double metrics_height = box.h;
    double horiBearingX = box.x;
    double horiBearingY = -box.y;
    double vertBearingX = slot->metrics.horiBearingX / 64.0 - slot->metrics.horiAdvance / 64.0 / 2.0;
    double vertBearingY = (slot->metrics.vertAdvance / 64.0 - slot->metrics.height / 64.0) / 2.0;
    slot->metrics.width = FT_Pos(IM_ROUND(metrics_width * 64.0));   // Using IM_ROUND() assume width and height are positive
    slot->metrics.height = FT_Pos(IM_ROUND(metrics_height * 64.0));
    slot->metrics.horiBearingX = FT_Pos(horiBearingX * 64);
    slot->metrics.horiBearingY = FT_Pos(horiBearingY * 64);
    slot->metrics.vertBearingX = FT_Pos(vertBearingX * 64);
    slot->metrics.vertBearingY = FT_Pos(vertBearingY * 64);

    if (slot->metrics.vertAdvance == 0)
        slot->metrics.vertAdvance = FT_Pos(metrics_height * 1.2 * 64.0);

    state->err = FT_Err_Ok;
    return state->err;
}

#endif // #ifdef IMGUI_ENABLE_FREETYPE_LUNASVG

//-----------------------------------------------------------------------------

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning (pop)
#endif

#endif // #ifndef IMGUI_DISABLE
