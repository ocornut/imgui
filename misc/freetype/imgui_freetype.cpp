// dear imgui: FreeType font builder (used as a replacement for the stb_truetype builder)
// (code)

// Get the latest version at https://github.com/ocornut/imgui/tree/master/misc/freetype
// Original code by @vuhdo (Aleksei Skriabin). Improvements by @mikesart. Maintained since 2019 by @ocornut.

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2023/07/10: added support for SVG fonts, enable by using '#define IMGUI_ENABLE_FREETYPE_LIBRSVG'
//  2023/01/04: fixed a packing issue which in some occurrences would prevent large amount of glyphs from being packed correctly.
//  2021/08/23: fixed crash when FT_Render_Glyph() fails to render a glyph and returns NULL.
//  2021/03/05: added ImGuiFreeTypeBuilderFlags_Bitmap to load bitmap glyphs.
//  2021/03/02: set 'atlas->TexPixelsUseColors = true' to help some backends with deciding of a prefered texture format.
//  2021/01/28: added support for color-layered glyphs via ImGuiFreeTypeBuilderFlags_LoadColor (require Freetype 2.10+).
//  2021/01/26: simplified integration by using '#define IMGUI_ENABLE_FREETYPE'.
//              renamed ImGuiFreeType::XXX flags to ImGuiFreeTypeBuilderFlags_XXX for consistency with other API. removed ImGuiFreeType::BuildFontAtlas().
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

#include "imgui_freetype.h"
#include "imgui_internal.h"     // ImMin,ImMax,ImFontAtlasBuild*,
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H          // <freetype/freetype.h>
#include FT_MODULE_H            // <freetype/ftmodapi.h>
#include FT_GLYPH_H             // <freetype/ftglyph.h>
#include FT_SYNTHESIS_H         // <freetype/ftsynth.h>

#ifdef IMGUI_ENABLE_FREETYPE_LIBRSVG
#include FT_OTSVG_H             // <freetype/otsvg.h>
#include FT_BBOX_H              // <freetype/ftbbox.h>
#include <librsvg/rsvg.h>
#include <cairo/cairo.h>
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
#pragma GCC diagnostic ignored "-Wsubobject-linkage"        // warning: 'xxxx' has a field 'xxxx' whose type uses the anonymous namespace
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


#ifdef IMGUI_ENABLE_FREETYPE_LIBRSVG
// librsvg hooks
FT_Error ImGuiRsvgPortInit(FT_Pointer* state);
void     ImGuiRsvgPortFree(FT_Pointer* state);
FT_Error ImGuiRsvgPortRender(FT_GlyphSlot slot, FT_Pointer* _state);
FT_Error ImGuiRsvgPortPresetSlot(FT_GlyphSlot slot, FT_Bool cache, FT_Pointer* _state);
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
        req.height = (uint32_t)pixel_height * 64;
        req.horiResolution = 0;
        req.vertResolution = 0;
        FT_Request_Size(Face, &req);

        // Update font info
        FT_Size_Metrics metrics = Face->size->metrics;
        Info.PixelHeight = (uint32_t)pixel_height;
        Info.Ascender = (float)FT_CEIL(metrics.ascender);
        Info.Descender = (float)FT_CEIL(metrics.descender);
        Info.LineSpacing = (float)FT_CEIL(metrics.height);
        Info.LineGap = (float)FT_CEIL(metrics.height - metrics.ascender + metrics.descender);
        Info.MaxAdvanceWidth = (float)FT_CEIL(metrics.max_advance);
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

#ifdef IMGUI_ENABLE_FREETYPE_LIBRSVG
        IM_ASSERT(
            slot->format == FT_GLYPH_FORMAT_OUTLINE ||
            slot->format == FT_GLYPH_FORMAT_BITMAP  ||
            slot->format == FT_GLYPH_FORMAT_SVG
        );
#else
        IM_ASSERT(
            slot->format != FT_GLYPH_FORMAT_SVG &&
            "The font contains SVG glyphs, you'll need to enable IMGUI_ENABLE_FREETYPE_LIBRSVG"
            "in imconfig.h and install required libraries in order to use this font"
        );

        IM_ASSERT(slot->format == FT_GLYPH_FORMAT_OUTLINE || slot->format == FT_GLYPH_FORMAT_BITMAP);
#endif

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
    atlas->TexID = (ImTextureID)nullptr;
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
            float x0 = info.OffsetX + font_off_x;
            float y0 = info.OffsetY + font_off_y;
            float x1 = x0 + info.Width;
            float y1 = y0 + info.Height;
            float u0 = (tx) / (float)atlas->TexWidth;
            float v0 = (ty) / (float)atlas->TexHeight;
            float u1 = (tx + info.Width) / (float)atlas->TexWidth;
            float v1 = (ty + info.Height) / (float)atlas->TexHeight;
            dst_font->AddGlyph(&cfg, (ImWchar)src_glyph.Codepoint, x0, y0, x1, y1, u0, v0, u1, v1, info.AdvanceX);

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

#ifdef IMGUI_ENABLE_FREETYPE_LIBRSVG
    // Install svg hooks for FreeType
    // https://freetype.org/freetype2/docs/reference/ft2-properties.html#svg-hooks
    // https://freetype.org/freetype2/docs/reference/ft2-svg_fonts.html#svg_fonts
    SVG_RendererHooks  hooks = {
        ImGuiRsvgPortInit,
        ImGuiRsvgPortFree,
        ImGuiRsvgPortRender,
        ImGuiRsvgPortPresetSlot
    };

    FT_Property_Set(ft_library, "ot-svg", "svg-hooks", &hooks);
#endif

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

#ifdef IMGUI_ENABLE_FREETYPE_LIBRSVG
/*
 * Librsvg-based hook functions for OT-SVG rendering in FreeType
 * https://gitlab.freedesktop.org/freetype/freetype-demos/-/blob/master/src/rsvg-port.c
 */

 /*
  * Different hook functions can access persisting data by creating a state
  * structure and putting its address in `library->svg_renderer_state`.
  * Functions can then store and retrieve data from this structure.
  */
typedef struct  Rsvg_Port_StateRec_
{
    cairo_surface_t* rec_surface;

    double  x;
    double  y;

} Rsvg_Port_StateRec;

typedef struct Rsvg_Port_StateRec_* Rsvg_Port_State;


/*
  * The init hook is called when the first OT-SVG glyph is rendered.  All
  * we do is to allocate an internal state structure and set the pointer in
  * `library->svg_renderer_state`.  This state structure becomes very
  * useful to cache some of the results obtained by one hook function that
  * the other one might use.
  */
FT_Error
ImGuiRsvgPortInit(FT_Pointer* state)
{
    /* allocate the memory upon initialization */
    *state = malloc(sizeof(Rsvg_Port_StateRec)); /* XXX error handling */

    return FT_Err_Ok;
}


/*
 * Deallocate the state structure.
 */
void
ImGuiRsvgPortFree(FT_Pointer* state)
{
    free(*state);
}


/*
 * The render hook.  The job of this hook is to simply render the glyph in
 * the buffer that has been allocated on the FreeType side.  Here we
 * simply use the recording surface by playing it back against the
 * surface.
 */
FT_Error
ImGuiRsvgPortRender(FT_GlyphSlot  slot,
    FT_Pointer* _state)
{
    FT_Error  error = FT_Err_Ok;

    Rsvg_Port_State   state;
    cairo_status_t    status;
    cairo_t* cr;
    cairo_surface_t* surface;


    state = *(Rsvg_Port_State*)_state;

    /* Create an image surface to store the rendered image.  However,   */
    /* don't allocate memory; instead use the space already provided in */
    /* `slot->bitmap.buffer`.                                           */
    surface = cairo_image_surface_create_for_data(slot->bitmap.buffer,
        CAIRO_FORMAT_ARGB32,
        (int)slot->bitmap.width,
        (int)slot->bitmap.rows,
        slot->bitmap.pitch);
    status = cairo_surface_status(surface);

    if (status != CAIRO_STATUS_SUCCESS)
    {
        if (status == CAIRO_STATUS_NO_MEMORY)
            return FT_Err_Out_Of_Memory;
        else
            return FT_Err_Invalid_Outline;
    }

    cr = cairo_create(surface);
    status = cairo_status(cr);

    if (status != CAIRO_STATUS_SUCCESS)
    {
        if (status == CAIRO_STATUS_NO_MEMORY)
            return FT_Err_Out_Of_Memory;
        else
            return FT_Err_Invalid_Outline;
    }

    /* Set a translate transform that translates the points in such a way */
    /* that we get a tight rendering with least redundant white spac.     */
    cairo_translate(cr, -state->x, -state->y);

    /* Replay from the recorded surface.  This saves us from parsing the */
    /* document again and redoing what was already done in the preset    */
    /* hook.                                                             */
    cairo_set_source_surface(cr, state->rec_surface, 0.0, 0.0);
    cairo_paint(cr);

    cairo_surface_flush(surface);

    slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;
    slot->bitmap.num_grays = 256;
    slot->format = FT_GLYPH_FORMAT_BITMAP;

    /* Clean up everything. */
    cairo_surface_destroy(surface);
    cairo_destroy(cr);
    cairo_surface_destroy(state->rec_surface);

    return error;
}

/*
 * This hook is called at two different locations.  Firstly, it is called
 * when presetting the glyphslot when `FT_Load_Glyph` is called.
 * Secondly, it is called right before the render hook is called.  When
 * `cache` is false, it is the former, when `cache` is true, it is the
 * latter.
 *
 * The job of this function is to preset the slot setting the width,
 * height, pitch, `bitmap.left`, and `bitmap.top`.  These are all
 * necessary for appropriate memory allocation, as well as ultimately
 * compositing the glyph later on by client applications.
 */
FT_Error
ImGuiRsvgPortPresetSlot(FT_GlyphSlot  slot,
    FT_Bool       cache,
    FT_Pointer* _state)
{
    /* FreeType variables. */
    FT_Error  error = FT_Err_Ok;

    FT_SVG_Document  document = (FT_SVG_Document)slot->other;
    FT_Size_Metrics  metrics = document->metrics;

    FT_UShort  units_per_EM = document->units_per_EM;
    FT_UShort  end_glyph_id = document->end_glyph_id;
    FT_UShort  start_glyph_id = document->start_glyph_id;

    /* Librsvg variables. */
    GError* gerror = NULL;
    gboolean  ret;

    gboolean  out_has_width;
    gboolean  out_has_height;
    gboolean  out_has_viewbox;

    RsvgHandle* handle;
    RsvgDimensionData  dimension_svg;

    cairo_t* rec_cr;
    cairo_matrix_t  transform_matrix;

    /* Rendering port's state. */
    Rsvg_Port_State     state;
    Rsvg_Port_StateRec  state_dummy;

    /* General variables. */
    double  x, y;
    double  xx, xy, yx, yy;
    double  x0, y0;
    double  width, height;
    double  x_svg_to_out, y_svg_to_out;
    double  tmpd;

    float metrics_width, metrics_height;
    float horiBearingX, horiBearingY;
    float vertBearingX, vertBearingY;
    float tmpf;

    char* id;
    char  str[32];


    /* If `cache` is `TRUE` we store calculations in the actual port */
    /* state variable, otherwise we just create a dummy variable and */
    /* store there.  This saves us from too many 'if' statements.    */
    if (cache)
        state = *(Rsvg_Port_State*)_state;
    else
        state = &state_dummy;

    /* Form an `RsvgHandle` by loading the SVG document. */
    handle = rsvg_handle_new_from_data(document->svg_document,
        document->svg_document_length,
        &gerror);
    if (handle == NULL)
    {
        error = FT_Err_Invalid_SVG_Document;
        goto CleanLibrsvg;
    }

#if (defined(RSVG_VERSION_H) && (LIBRSVG_MAJOR_VERSION >= 2) && (LIBRSVG_MINOR_VERSION >= 46))

    RsvgLength         out_width;
    RsvgLength         out_height;
    RsvgRectangle      out_viewbox;

    /* Get attributes like `viewBox` and `width`/`height`. */
    rsvg_handle_get_intrinsic_dimensions(handle,
        &out_has_width,
        &out_width,
        &out_has_height,
        &out_height,
        &out_has_viewbox,
        &out_viewbox);

    /*
     * Figure out the units in the EM square in the SVG document.  This is
     * specified by the `ViewBox` or the `width`/`height` attributes, if
     * present, otherwise it should be assumed that the units in the EM
     * square are the same as in the TTF/CFF outlines.
     *
     * TODO: I'm not sure what the standard says about the situation if
     * `ViewBox` as well as `width`/`height` are present; however, I've
     * never seen that situation in real fonts.
     */
    if (out_has_viewbox == TRUE)
    {
        dimension_svg.width = (int)out_viewbox.width; /* XXX rounding? */
        dimension_svg.height = (int)out_viewbox.height;
    }
    else if (out_has_width == TRUE && out_has_height == TRUE)
    {
        dimension_svg.width = (int)out_width.length; /* XXX rounding? */
        dimension_svg.height = (int)out_height.length;

        /*
         * librsvg 2.53+ behavior, on SVG doc without explicit width/height.
         * See `rsvg_handle_get_intrinsic_dimensions` section in
         * the `librsvg/rsvg.h` header file.
         */
        if (out_width.length == 1 &&
            out_height.length == 1)
        {
            dimension_svg.width = units_per_EM;
            dimension_svg.height = units_per_EM;
        }
    }
    else
    {
        /*
         * If neither `ViewBox` nor `width`/`height` are present, the
         * `units_per_EM` in SVG coordinates must be the same as
         * `units_per_EM` of the TTF/CFF outlines.
         *
         * librsvg up to 2.52 behavior, on SVG doc without explicit
         * width/height.
         */
        dimension_svg.width = units_per_EM;
        dimension_svg.height = units_per_EM;
    }
#else
    // Workaround for librsvg <= v2.46 doesn't have the rsvg_handle_get_intrinsic_dimensions function
    // See the problem with rsvg_handle_get_dimensions here:
    // https://gnome.pages.gitlab.gnome.org/librsvg/Rsvg-2.0/method.Handle.get_dimensions.html
    rsvg_handle_get_dimensions(handle, &dimension_svg);
#endif // (defined(RSVG_VERSION_H) && (LIBRSVG_MAJOR_VERSION >= 2) && (LIBRSVG_MINOR_VERSION >= 46))

    /* Scale factors from SVG coordinates to the needed output size. */
    x_svg_to_out = (double)metrics.x_ppem / dimension_svg.width;
    y_svg_to_out = (double)metrics.y_ppem / dimension_svg.height;

    /*
     * Create a cairo recording surface.  This is done for two reasons.
     * Firstly, it is required to get the bounding box of the final drawing
     * so we can use an appropriate translate transform to get a tight
     * rendering.  Secondly, if `cache` is true, we can save this surface
     * and later replay it against an image surface for the final rendering.
     * This saves us from loading and parsing the document again.
     */
    state->rec_surface =
        cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL);

    rec_cr = cairo_create(state->rec_surface);

    /*
     * We need to take into account any transformations applied.  The end
     * user who applied the transformation doesn't know the internal details
     * of the SVG document.  Thus, we expect that the end user should just
     * write the transformation as if the glyph is a traditional one.  We
     * then do some maths on this to get the equivalent transformation in
     * SVG coordinates.
     */
    xx = (double)document->transform.xx / (1 << 16);
    xy = -(double)document->transform.xy / (1 << 16);
    yx = -(double)document->transform.yx / (1 << 16);
    yy = (double)document->transform.yy / (1 << 16);

    x0 = (double)document->delta.x / 64 *
        dimension_svg.width / metrics.x_ppem;
    y0 = -(double)document->delta.y / 64 *
        dimension_svg.height / metrics.y_ppem;

    /* Cairo stores both transformation and translation in one matrix. */
    transform_matrix.xx = xx;
    transform_matrix.yx = yx;
    transform_matrix.xy = xy;
    transform_matrix.yy = yy;
    transform_matrix.x0 = x0;
    transform_matrix.y0 = y0;

    /* Set up a scale transformation to scale up the document to the */
    /* required output size.                                         */
    cairo_scale(rec_cr, x_svg_to_out, y_svg_to_out);
    /* Set up a transformation matrix. */
    cairo_transform(rec_cr, &transform_matrix);

    /* If the document contains only one glyph, `start_glyph_id` and */
    /* `end_glyph_id` have the same value.  Otherwise `end_glyph_id` */
    /* is larger.                                                    */
    if (start_glyph_id < end_glyph_id)
    {
        /* Render only the element with its ID equal to `glyph<ID>`. */
        sprintf(str, "#glyph%u", slot->glyph_index);
        id = str;
    }
    else
    {
        /* NULL = Render the whole document */
        id = NULL;
    }

#if LIBRSVG_CHECK_VERSION( 2, 52, 0 )
    {
        RsvgRectangle  viewport =
        {
          .x = 0,
          .y = 0,
          .width = (double)dimension_svg.width,
          .height = (double)dimension_svg.height,
        };


        ret = rsvg_handle_render_layer(handle,
            rec_cr,
            id,
            &viewport,
            NULL);
    }
#else
    ret = rsvg_handle_render_cairo_sub(handle, rec_cr, id);
#endif

    if (ret == FALSE)
    {
        error = FT_Err_Invalid_SVG_Document;
        goto CleanCairo;
    }

    /* Get the bounding box of the drawing. */
    cairo_recording_surface_ink_extents(state->rec_surface, &x, &y,
        &width, &height);

    /* We store the bounding box's `x` and `y` values so that the render */
    /* hook can apply a translation to get a tight rendering.            */
    state->x = x;
    state->y = y;

    /* Preset the values. */
    slot->bitmap_left = (FT_Int)state->x;  /* XXX rounding? */
    slot->bitmap_top = (FT_Int)-state->y;

    /* Do conversion in two steps to avoid 'bad function cast' warning. */
    tmpd = ceil(height);
    slot->bitmap.rows = (unsigned int)tmpd;
    tmpd = ceil(width);
    slot->bitmap.width = (unsigned int)tmpd;

    slot->bitmap.pitch = (int)slot->bitmap.width * 4;

    slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;

    /* Compute all the bearings and set them correctly.  The outline is */
    /* scaled already, we just need to use the bounding box.            */
    metrics_width = (float)width;
    metrics_height = (float)height;

    horiBearingX = (float)state->x;
    horiBearingY = (float)-state->y;

    vertBearingX = slot->metrics.horiBearingX / 64.0f -
        slot->metrics.horiAdvance / 64.0f / 2;
    vertBearingY = (slot->metrics.vertAdvance / 64.0f -
        slot->metrics.height / 64.0f) / 2; /* XXX parentheses correct? */

    /* Do conversion in two steps to avoid 'bad function cast' warning. */
    tmpf = roundf(metrics_width * 64);
    slot->metrics.width = (FT_Pos)tmpf;
    tmpf = roundf(metrics_height * 64);
    slot->metrics.height = (FT_Pos)tmpf;

    slot->metrics.horiBearingX = (FT_Pos)(horiBearingX * 64); /* XXX rounding? */
    slot->metrics.horiBearingY = (FT_Pos)(horiBearingY * 64);
    slot->metrics.vertBearingX = (FT_Pos)(vertBearingX * 64);
    slot->metrics.vertBearingY = (FT_Pos)(vertBearingY * 64);

    if (slot->metrics.vertAdvance == 0)
        slot->metrics.vertAdvance = (FT_Pos)(metrics_height * 1.2f * 64);

    /* If a render call is to follow, just destroy the context for the */
    /* recording surface since no more drawing will be done on it.     */
    /* However, keep the surface itself for use by the render hook.    */
    if (cache == TRUE)
    {
        cairo_destroy(rec_cr);
        goto CleanLibrsvg;
    }

    /* Destroy the recording surface as well as the context. */
CleanCairo:
    cairo_surface_destroy(state->rec_surface);
    cairo_destroy(rec_cr);

CleanLibrsvg:
    /* Destroy the handle. */
    g_object_unref(handle);

    return error;
}
#endif // !IMGUI_ENABLE_FREETYPE_LIBRSVG

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning (pop)
#endif
