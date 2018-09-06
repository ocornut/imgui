// Wrapper to use Freetype (instead of stb_truetype) for Dear ImGui
// Get latest version at https://github.com/ocornut/imgui/tree/master/misc/freetype
// Original code by @Vuhdo (Aleksei Skriabin). Improvements by @mikesart. Maintained by @ocornut

// Changelog:
// - v0.50: (2017/08/16) imported from https://github.com/Vuhdo/imgui_freetype into http://www.github.com/ocornut/imgui_club, updated for latest changes in ImFontAtlas, minor tweaks.
// - v0.51: (2017/08/26) cleanup, optimizations, support for ImFontConfig::RasterizerFlags, ImFontConfig::RasterizerMultiply.
// - v0.52: (2017/09/26) fixes for imgui internal changes
// - v0.53: (2017/10/22) minor inconsequential change to match change in master (removed an unnecessary statement)
// - v0.54: (2018/01/22) fix for addition of ImFontAtlas::TexUvscale member
// - v0.55: (2018/02/04) moved to main imgui repository (away from http://www.github.com/ocornut/imgui_club)
// - v0.56: (2018/06/08) added support for ImFontConfig::GlyphMinAdvanceX, GlyphMaxAdvanceX

// Gamma Correct Blending:
//  FreeType assumes blending in linear space rather than gamma space.
//  See https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_Render_Glyph
//  For correct results you need to be using sRGB and convert to linear space in the pixel shader output.
//  The default imgui styles will be impacted by this change (alpha values will need tweaking).

// TODO:
// - Output texture has excessive resolution (lots of vertical waste).
// - FreeType's memory allocator is not overridden.
// - cfg.OversampleH, OversampleV are ignored (but perhaps not so necessary with this rasterizer).

#include "imgui_freetype.h"
#include "imgui_internal.h"   // ImMin,ImMax,ImFontAtlasBuild*,
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_SYNTHESIS_H

#ifdef _MSC_VER
#pragma warning (disable: 4505) // unreferenced local function has been removed (stb stuff)
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#endif

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

    /// A structure that describe a glyph.
    struct GlyphInfo 
    {
        float Width;		// Glyph's width in pixels.
        float Height;		// Glyph's height in pixels.
        float OffsetX;		// The distance from the origin ("pen position") to the left of the glyph.
        float OffsetY;		// The distance from the origin to the top of the glyph. This is usually a value < 0.
        float AdvanceX;		// The distance from the origin to the origin of the next glyph. This is usually a value > 0.
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
        bool        Init(const ImFontConfig& cfg, unsigned int extra_user_flags);   // Initialize from an external data buffer. Doesn't copy data, and you must ensure it stays valid up to this object lifetime.
        void        Shutdown();
        void        SetPixelHeight(int pixel_height);                               // Change font pixel size. All following calls to RasterizeGlyph() will use this size

        bool        CalcGlyphInfo(uint32_t codepoint, GlyphInfo& glyph_info, FT_Glyph& ft_glyph, FT_BitmapGlyph& ft_bitmap);
        void        BlitGlyph(FT_BitmapGlyph ft_bitmap, uint8_t* dst, uint32_t dst_pitch, unsigned char* multiply_table = NULL);

        // [Internals]
        FontInfo        Info;               // Font descriptor of the current font.
        unsigned int    UserFlags;          // = ImFontConfig::RasterizerFlags
        FT_Library      FreetypeLibrary;
        FT_Face         FreetypeFace;
        FT_Int32        FreetypeLoadFlags;
    };

    // From SDL_ttf: Handy routines for converting from fixed point
    #define FT_CEIL(X)  (((X + 63) & -64) / 64)

    bool FreeTypeFont::Init(const ImFontConfig& cfg, unsigned int extra_user_flags)
    {
        // FIXME: substitute allocator
        FT_Error error = FT_Init_FreeType(&FreetypeLibrary);
        if (error != 0)
            return false;
        error = FT_New_Memory_Face(FreetypeLibrary, (uint8_t*)cfg.FontData, (uint32_t)cfg.FontDataSize, (uint32_t)cfg.FontNo, &FreetypeFace);
        if (error != 0)
            return false;
        error = FT_Select_Charmap(FreetypeFace, FT_ENCODING_UNICODE);
        if (error != 0)
            return false;

        memset(&Info, 0, sizeof(Info));
        SetPixelHeight((uint32_t)cfg.SizePixels);

        // Convert to freetype flags (nb: Bold and Oblique are processed separately)
        UserFlags = cfg.RasterizerFlags | extra_user_flags;
        FreetypeLoadFlags = FT_LOAD_NO_BITMAP;
        if (UserFlags & ImGuiFreeType::NoHinting)      FreetypeLoadFlags |= FT_LOAD_NO_HINTING;
        if (UserFlags & ImGuiFreeType::NoAutoHint)     FreetypeLoadFlags |= FT_LOAD_NO_AUTOHINT;
        if (UserFlags & ImGuiFreeType::ForceAutoHint)  FreetypeLoadFlags |= FT_LOAD_FORCE_AUTOHINT;
        if (UserFlags & ImGuiFreeType::LightHinting)   
            FreetypeLoadFlags |= FT_LOAD_TARGET_LIGHT;
        else if (UserFlags & ImGuiFreeType::MonoHinting)   
            FreetypeLoadFlags |= FT_LOAD_TARGET_MONO;
        else                                                
            FreetypeLoadFlags |= FT_LOAD_TARGET_NORMAL;

        return true;
    }

    void FreeTypeFont::Shutdown()
    {
        if (FreetypeFace) 
        {
            FT_Done_Face(FreetypeFace);
            FreetypeFace = NULL;
            FT_Done_FreeType(FreetypeLibrary);
            FreetypeLibrary = NULL;
        }
    }

    void FreeTypeFont::SetPixelHeight(int pixel_height) 
    {
        // I'm not sure how to deal with font sizes properly.
        // As far as I understand, currently ImGui assumes that the 'pixel_height' is a maximum height of an any given glyph,
        // i.e. it's the sum of font's ascender and descender. Seems strange to me.
        FT_Size_RequestRec req;
        req.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
        req.width = 0;
        req.height = (uint32_t)pixel_height * 64;
        req.horiResolution = 0;
        req.vertResolution = 0;
        FT_Request_Size(FreetypeFace, &req);

        // update font info
        FT_Size_Metrics metrics = FreetypeFace->size->metrics;
        Info.PixelHeight = (uint32_t)pixel_height;
        Info.Ascender = (float)FT_CEIL(metrics.ascender);
        Info.Descender = (float)FT_CEIL(metrics.descender);
        Info.LineSpacing = (float)FT_CEIL(metrics.height);
        Info.LineGap = (float)FT_CEIL(metrics.height - metrics.ascender + metrics.descender);
        Info.MaxAdvanceWidth = (float)FT_CEIL(metrics.max_advance);
    }

    bool FreeTypeFont::CalcGlyphInfo(uint32_t codepoint, GlyphInfo &glyph_info, FT_Glyph& ft_glyph, FT_BitmapGlyph& ft_bitmap)
    {
        uint32_t glyph_index = FT_Get_Char_Index(FreetypeFace, codepoint);
        if (glyph_index == 0)
            return false;
        FT_Error error = FT_Load_Glyph(FreetypeFace, glyph_index, FreetypeLoadFlags);
        if (error)
            return false;

        // Need an outline for this to work
        FT_GlyphSlot slot = FreetypeFace->glyph;
        IM_ASSERT(slot->format == FT_GLYPH_FORMAT_OUTLINE);

        if (UserFlags & ImGuiFreeType::Bold)
            FT_GlyphSlot_Embolden(slot);
        if (UserFlags & ImGuiFreeType::Oblique)
            FT_GlyphSlot_Oblique(slot);

        // Retrieve the glyph
        error = FT_Get_Glyph(slot, &ft_glyph);
        if (error != 0)
            return false;

        // Rasterize
        error = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, NULL, true);
        if (error != 0)
            return false;

        ft_bitmap = (FT_BitmapGlyph)ft_glyph;
        glyph_info.AdvanceX = (float)FT_CEIL(slot->advance.x);
        glyph_info.OffsetX = (float)ft_bitmap->left;
        glyph_info.OffsetY = -(float)ft_bitmap->top;
        glyph_info.Width = (float)ft_bitmap->bitmap.width;
        glyph_info.Height = (float)ft_bitmap->bitmap.rows;

        return true;
    }

    void FreeTypeFont::BlitGlyph(FT_BitmapGlyph ft_bitmap, uint8_t* dst, uint32_t dst_pitch, unsigned char* multiply_table)
    {
        IM_ASSERT(ft_bitmap != NULL);

        const uint32_t w = ft_bitmap->bitmap.width;
        const uint32_t h = ft_bitmap->bitmap.rows;
        const uint8_t* src = ft_bitmap->bitmap.buffer;
        const uint32_t src_pitch = ft_bitmap->bitmap.pitch;

        if (multiply_table == NULL)
        {
            for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                memcpy(dst, src, w);
        }
        else
        {
            for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                for (uint32_t x = 0; x < w; x++)
                    dst[x] = multiply_table[src[x]];
        }
    }
}

#define STBRP_ASSERT(x)    IM_ASSERT(x)
#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include "imstb_rectpack.h"

bool ImGuiFreeType::BuildFontAtlas(ImFontAtlas* atlas, unsigned int extra_flags)
{
    IM_ASSERT(atlas->ConfigData.Size > 0);
    IM_ASSERT(atlas->TexGlyphPadding == 1); // Not supported

    ImFontAtlasBuildRegisterDefaultCustomRects(atlas);

    atlas->TexID = NULL;
    atlas->TexWidth = atlas->TexHeight = 0;
    atlas->TexUvScale = ImVec2(0.0f, 0.0f);
    atlas->TexUvWhitePixel = ImVec2(0.0f, 0.0f);
    atlas->ClearTexData();

    ImVector<FreeTypeFont> fonts;
    fonts.resize(atlas->ConfigData.Size);

    ImVec2 max_glyph_size(1.0f, 1.0f);

    // Count glyphs/ranges, initialize font
    int total_glyphs_count = 0;
    int total_ranges_count = 0;
    for (int input_i = 0; input_i < atlas->ConfigData.Size; input_i++) 
    {
        ImFontConfig& cfg = atlas->ConfigData[input_i];
        FreeTypeFont& font_face = fonts[input_i];
        IM_ASSERT(cfg.DstFont && (!cfg.DstFont->IsLoaded() || cfg.DstFont->ContainerAtlas == atlas));

        if (!font_face.Init(cfg, extra_flags))
            return false;

        max_glyph_size.x = ImMax(max_glyph_size.x, font_face.Info.MaxAdvanceWidth);
        max_glyph_size.y = ImMax(max_glyph_size.y, font_face.Info.Ascender - font_face.Info.Descender);

        if (!cfg.GlyphRanges)
            cfg.GlyphRanges = atlas->GetGlyphRangesDefault();
        for (const ImWchar* in_range = cfg.GlyphRanges; in_range[0] && in_range[ 1 ]; in_range += 2, total_ranges_count++) 
            total_glyphs_count += (in_range[1] - in_range[0]) + 1;
    }

    // We need a width for the skyline algorithm. Using a dumb heuristic here to decide of width. User can override TexDesiredWidth and TexGlyphPadding if they wish.
    // Width doesn't really matter much, but some API/GPU have texture size limitations and increasing width can decrease height.
    atlas->TexWidth = (atlas->TexDesiredWidth > 0) ? atlas->TexDesiredWidth : (total_glyphs_count > 4000) ? 4096 : (total_glyphs_count > 2000) ? 2048 : (total_glyphs_count > 1000) ? 1024 : 512;

    // We don't do the original first pass to determine texture height, but just rough estimate.
    // Looks ugly inaccurate and excessive, but AFAIK with FreeType we actually need to render glyphs to get exact sizes.
    // Alternatively, we could just render all glyphs into a big shadow buffer, get their sizes, do the rectangle packing and just copy back from the 
    // shadow buffer to the texture buffer. Will give us an accurate texture height, but eat a lot of temp memory. Probably no one will notice.)
    const int total_rects = total_glyphs_count + atlas->CustomRects.size();
    float min_rects_per_row = ceilf((atlas->TexWidth / (max_glyph_size.x + 1.0f)));
    float min_rects_per_column = ceilf(total_rects / min_rects_per_row);
    atlas->TexHeight = (int)(min_rects_per_column * (max_glyph_size.y + 1.0f));

    // Create texture
    atlas->TexHeight = (atlas->Flags & ImFontAtlasFlags_NoPowerOfTwoHeight) ? (atlas->TexHeight + 1) : ImUpperPowerOfTwo(atlas->TexHeight);
    atlas->TexUvScale = ImVec2(1.0f / atlas->TexWidth, 1.0f / atlas->TexHeight);
    atlas->TexPixelsAlpha8 = (unsigned char*)ImGui::MemAlloc(atlas->TexWidth * atlas->TexHeight);
    memset(atlas->TexPixelsAlpha8, 0, atlas->TexWidth * atlas->TexHeight);

    // Start packing
    ImVector<stbrp_node> pack_nodes;
    pack_nodes.resize(total_rects);
    stbrp_context context;
    stbrp_init_target(&context, atlas->TexWidth, atlas->TexHeight, pack_nodes.Data, total_rects);

    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    ImFontAtlasBuildPackCustomRects(atlas, &context);

    // Render characters, setup ImFont and glyphs for runtime
    for (int input_i = 0; input_i < atlas->ConfigData.Size; input_i++)
    {
        ImFontConfig& cfg = atlas->ConfigData[input_i];
        FreeTypeFont& font_face = fonts[input_i];
        ImFont* dst_font = cfg.DstFont;
        if (cfg.MergeMode)
            dst_font->BuildLookupTable();

        const float ascent = font_face.Info.Ascender;
        const float descent = font_face.Info.Descender;
        ImFontAtlasBuildSetupFont(atlas, dst_font, &cfg, ascent, descent);
        const float font_off_x = cfg.GlyphOffset.x;
        const float font_off_y = cfg.GlyphOffset.y + (float)(int)(dst_font->Ascent + 0.5f);

        bool multiply_enabled = (cfg.RasterizerMultiply != 1.0f);
        unsigned char multiply_table[256];
        if (multiply_enabled)
            ImFontAtlasBuildMultiplyCalcLookupTable(multiply_table, cfg.RasterizerMultiply);

        for (const ImWchar* in_range = cfg.GlyphRanges; in_range[0] && in_range[1]; in_range += 2) 
        {
            for (uint32_t codepoint = in_range[0]; codepoint <= in_range[1]; ++codepoint) 
            {
                if (cfg.MergeMode && dst_font->FindGlyphNoFallback((unsigned short)codepoint))
                    continue;

                FT_Glyph ft_glyph = NULL;
                FT_BitmapGlyph ft_glyph_bitmap = NULL; // NB: will point to bitmap within FT_Glyph
                GlyphInfo glyph_info;
                if (!font_face.CalcGlyphInfo(codepoint, glyph_info, ft_glyph, ft_glyph_bitmap))
                    continue;

                // Pack rectangle
                stbrp_rect rect;
                rect.w = (uint16_t)glyph_info.Width + 1; // Account for texture filtering
                rect.h = (uint16_t)glyph_info.Height + 1;
                stbrp_pack_rects(&context, &rect, 1);

                // Copy rasterized pixels to main texture
                uint8_t* blit_dst = atlas->TexPixelsAlpha8 + rect.y * atlas->TexWidth + rect.x;
                font_face.BlitGlyph(ft_glyph_bitmap, blit_dst, atlas->TexWidth, multiply_enabled ? multiply_table : NULL);
                FT_Done_Glyph(ft_glyph);

                float char_advance_x_org = glyph_info.AdvanceX;
                float char_advance_x_mod = ImClamp(char_advance_x_org, cfg.GlyphMinAdvanceX, cfg.GlyphMaxAdvanceX);
                float char_off_x = font_off_x;
                if (char_advance_x_org != char_advance_x_mod)
                    char_off_x += cfg.PixelSnapH ? (float)(int)((char_advance_x_mod - char_advance_x_org) * 0.5f) : (char_advance_x_mod - char_advance_x_org) * 0.5f;

                // Register glyph
                dst_font->AddGlyph((ImWchar)codepoint, 
                    glyph_info.OffsetX + char_off_x, 
                    glyph_info.OffsetY + font_off_y, 
                    glyph_info.OffsetX + char_off_x + glyph_info.Width, 
                    glyph_info.OffsetY + font_off_y + glyph_info.Height,
                    rect.x / (float)atlas->TexWidth, 
                    rect.y / (float)atlas->TexHeight, 
                    (rect.x + glyph_info.Width) / (float)atlas->TexWidth, 
                    (rect.y + glyph_info.Height) / (float)atlas->TexHeight,
                    char_advance_x_mod);
            }
        }
    }

    // Cleanup
    for (int n = 0; n < fonts.Size; n++)
        fonts[n].Shutdown();

    ImFontAtlasBuildFinish(atlas);

    return true;
}
