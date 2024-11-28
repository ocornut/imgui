// dear imgui: FreeType font builder (used as a replacement for the stb_truetype builder)
// (code)

// Get the latest version at https://github.com/ocornut/imgui/tree/master/misc/freetype
// Original code by @vuhdo (Aleksei Skriabin) in 2017, with improvements by @mikesart.
// Maintained since 2019 by @ocornut.

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2025/XX/XX: refactored for the new ImFontLoader architecture, and ImGuiBackendFlags_RendererHasTextures support.
//  2024/10/17: added plutosvg support for SVG Fonts (seems faster/better than lunasvg). Enable by using '#define IMGUI_ENABLE_FREETYPE_PLUTOSVG'. (#7927)
//  2023/11/13: added support for ImFontConfig::RasterizationDensity field for scaling render density without scaling metrics.
//  2023/08/01: added support for SVG fonts, enable by using '#define IMGUI_ENABLE_FREETYPE_LUNASVG'. (#6591)
//  2023/01/04: fixed a packing issue which in some occurrences would prevent large amount of glyphs from being packed correctly.
//  2021/08/23: fixed crash when FT_Render_Glyph() fails to render a glyph and returns nullptr.
//  2021/03/05: added ImGuiFreeTypeBuilderFlags_Bitmap to load bitmap glyphs.
//  2021/03/02: set 'atlas->TexPixelsUseColors = true' to help some backends with deciding of a preferred texture format.
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

// FIXME: cfg.OversampleH, OversampleV are not supported, but generally not necessary with this rasterizer because Hinting makes everything look better.

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

// Handle LunaSVG and PlutoSVG
#if defined(IMGUI_ENABLE_FREETYPE_LUNASVG) && defined(IMGUI_ENABLE_FREETYPE_PLUTOSVG)
#error "Cannot enable both IMGUI_ENABLE_FREETYPE_LUNASVG and IMGUI_ENABLE_FREETYPE_PLUTOSVG"
#endif
#ifdef  IMGUI_ENABLE_FREETYPE_LUNASVG
#include FT_OTSVG_H             // <freetype/otsvg.h>
#include FT_BBOX_H              // <freetype/ftbbox.h>
#include <lunasvg.h>
#endif
#ifdef  IMGUI_ENABLE_FREETYPE_PLUTOSVG
#include <plutosvg.h>
#endif
#if defined(IMGUI_ENABLE_FREETYPE_LUNASVG) || defined (IMGUI_ENABLE_FREETYPE_PLUTOSVG)
#if !((FREETYPE_MAJOR >= 2) && (FREETYPE_MINOR >= 12))
#error IMGUI_ENABLE_FREETYPE_PLUTOSVG or IMGUI_ENABLE_FREETYPE_LUNASVG requires FreeType version >= 2.12
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

#define FT_CEIL(X)      (((X + 63) & -64) / 64) // From SDL_ttf: Handy routines for converting from fixed point
#define FT_SCALEFACTOR  64.0f

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

    // Stored in ImFontAtlas::FontLoaderData
    struct ImGui_ImplFreeType_Data
    {
        FT_Library                      Library;
        FT_MemoryRec_                   MemoryManager;
        ImGui_ImplFreeType_Data()       { memset((void*)this, 0, sizeof(*this)); }
    };

    // Font parameters and metrics.
    struct ImGui_ImplFreeType_FontInfo
    {
        float       PixelHeight;        // Size this font was generated with.
        float       Ascender;           // The pixel extents above the baseline in pixels (typically positive).
        float       Descender;          // The extents below the baseline in pixels (typically negative).
        float       LineSpacing;        // The baseline-to-baseline distance. Note that it usually is larger than the sum of the ascender and descender taken as absolute values. There is also no guarantee that no glyphs extend above or below subsequent baselines when using this distance. Think of it as a value the designer of the font finds appropriate.
        float       LineGap;            // The spacing in pixels between one row's descent and the next row's ascent.
        float       MaxAdvanceWidth;    // This field gives the maximum horizontal cursor advance for all glyphs in the font.
    };

    // Stored in ImFontConfig::FontLoaderData
    struct ImGui_ImplFreeType_FontSrcData
    {
        bool                            InitFont(FT_Library ft_library, ImFontConfig* src, unsigned int extra_user_flags); // Initialize from an external data buffer. Doesn't copy data, and you must ensure it stays valid up to this object lifetime.
        void                            CloseFont();
        void                            SetPixelHeight(float pixel_height); // Change font pixel size.
        const FT_Glyph_Metrics*         LoadGlyph(uint32_t in_codepoint);
        void                            BlitGlyph(const FT_Bitmap* ft_bitmap, uint32_t* dst, uint32_t dst_pitch);
        ImGui_ImplFreeType_FontSrcData()   { memset((void*)this, 0, sizeof(*this)); }
        ~ImGui_ImplFreeType_FontSrcData()  { CloseFont(); }

        // Members
        ImGui_ImplFreeType_FontInfo     Info;               // Font descriptor of the current font.
        FT_Face                         FtFace;
        unsigned int                    UserFlags;          // = ImFontConfig::RasterizerFlags
        FT_Int32                        LoadFlags;
        FT_Render_Mode                  RenderMode;
        float                           RasterizationDensity;
        float                           InvRasterizationDensity;
    };

    bool ImGui_ImplFreeType_FontSrcData::InitFont(FT_Library ft_library, ImFontConfig* src, unsigned int extra_font_builder_flags)
    {
        FT_Error error = FT_New_Memory_Face(ft_library, (uint8_t*)src->FontData, (uint32_t)src->FontDataSize, (uint32_t)src->FontNo, &FtFace);
        if (error != 0)
            return false;
        error = FT_Select_Charmap(FtFace, FT_ENCODING_UNICODE);
        if (error != 0)
            return false;

        // Convert to FreeType flags (NB: Bold and Oblique are processed separately)
        UserFlags = src->FontBuilderFlags | extra_font_builder_flags;

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

        RasterizationDensity = src->RasterizerDensity;
        InvRasterizationDensity = 1.0f / RasterizationDensity;

        memset(&Info, 0, sizeof(Info));
        SetPixelHeight(src->SizePixels);

        return true;
    }

    void ImGui_ImplFreeType_FontSrcData::CloseFont()
    {
        if (FtFace)
        {
            FT_Done_Face(FtFace);
            FtFace = nullptr;
        }
    }

    void ImGui_ImplFreeType_FontSrcData::SetPixelHeight(float pixel_height)
    {
        // Vuhdo (2017): "I'm not sure how to deal with font sizes properly. As far as I understand, currently ImGui assumes that the 'pixel_height'
        // is a maximum height of an any given glyph, i.e. it's the sum of font's ascender and descender. Seems strange to me.
        // FT_Set_Pixel_Sizes() doesn't seem to get us the same result."
        // (FT_Set_Pixel_Sizes() essentially calls FT_Request_Size() with FT_SIZE_REQUEST_TYPE_NOMINAL)
        FT_Size_RequestRec req;
        req.type = (UserFlags & ImGuiFreeTypeBuilderFlags_Bitmap) ? FT_SIZE_REQUEST_TYPE_NOMINAL : FT_SIZE_REQUEST_TYPE_REAL_DIM;
        req.width = 0;
        req.height = (uint32_t)(pixel_height * 64 * RasterizationDensity);
        req.horiResolution = 0;
        req.vertResolution = 0;
        FT_Request_Size(FtFace, &req);
        // Note: To handle multiple sizes later, we may need to use FT_New_Size(), FT_Activate_Size()

        // Update font info
        FT_Size_Metrics metrics = FtFace->size->metrics;
        Info.PixelHeight = pixel_height * InvRasterizationDensity;
        Info.Ascender = (float)FT_CEIL(metrics.ascender) * InvRasterizationDensity;
        Info.Descender = (float)FT_CEIL(metrics.descender) * InvRasterizationDensity;
        Info.LineSpacing = (float)FT_CEIL(metrics.height) * InvRasterizationDensity;
        Info.LineGap = (float)FT_CEIL(metrics.height - metrics.ascender + metrics.descender) * InvRasterizationDensity;
        Info.MaxAdvanceWidth = (float)FT_CEIL(metrics.max_advance) * InvRasterizationDensity;
    }

    const FT_Glyph_Metrics* ImGui_ImplFreeType_FontSrcData::LoadGlyph(uint32_t codepoint)
    {
        uint32_t glyph_index = FT_Get_Char_Index(FtFace, codepoint);
        if (glyph_index == 0)
            return nullptr;

        // If this crash for you: FreeType 2.11.0 has a crash bug on some bitmap/colored fonts.
        // - https://gitlab.freedesktop.org/freetype/freetype/-/issues/1076
        // - https://github.com/ocornut/imgui/issues/4567
        // - https://github.com/ocornut/imgui/issues/4566
        // You can use FreeType 2.10, or the patched version of 2.11.0 in VcPkg, or probably any upcoming FreeType version.
        FT_Error error = FT_Load_Glyph(FtFace, glyph_index, LoadFlags);
        if (error)
            return nullptr;

        // Need an outline for this to work
        FT_GlyphSlot slot = FtFace->glyph;
#if defined(IMGUI_ENABLE_FREETYPE_LUNASVG) || defined(IMGUI_ENABLE_FREETYPE_PLUTOSVG)
        IM_ASSERT(slot->format == FT_GLYPH_FORMAT_OUTLINE || slot->format == FT_GLYPH_FORMAT_BITMAP || slot->format == FT_GLYPH_FORMAT_SVG);
#else
#if ((FREETYPE_MAJOR >= 2) && (FREETYPE_MINOR >= 12))
        IM_ASSERT(slot->format != FT_GLYPH_FORMAT_SVG && "The font contains SVG glyphs, you'll need to enable IMGUI_ENABLE_FREETYPE_PLUTOSVG or IMGUI_ENABLE_FREETYPE_LUNASVG in imconfig.h and install required libraries in order to use this font");
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

    void ImGui_ImplFreeType_FontSrcData::BlitGlyph(const FT_Bitmap* ft_bitmap, uint32_t* dst, uint32_t dst_pitch)
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
                for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                    for (uint32_t x = 0; x < w; x++)
                        dst[x] = IM_COL32(255, 255, 255, src[x]);
                break;
            }
        case FT_PIXEL_MODE_MONO: // Monochrome image, 1 bit per pixel. The bits in each byte are ordered from MSB to LSB.
            {
                for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                {
                    uint8_t bits = 0;
                    const uint8_t* bits_ptr = src;
                    for (uint32_t x = 0; x < w; x++, bits <<= 1)
                    {
                        if ((x & 7) == 0)
                            bits = *bits_ptr++;
                        dst[x] = IM_COL32(255, 255, 255, (bits & 0x80) ? 255 : 0);
                    }
                }
                break;
            }
        case FT_PIXEL_MODE_BGRA:
            {
                // FIXME: Converting pre-multiplied alpha to straight. Doesn't smell good.
                #define DE_MULTIPLY(color, alpha) ImMin((ImU32)(255.0f * (float)color / (float)(alpha + FLT_MIN) + 0.5f), 255u)
                for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                    for (uint32_t x = 0; x < w; x++)
                    {
                        uint8_t r = src[x * 4 + 2], g = src[x * 4 + 1], b = src[x * 4], a = src[x * 4 + 3];
                        dst[x] = IM_COL32(DE_MULTIPLY(r, a), DE_MULTIPLY(g, a), DE_MULTIPLY(b, a), a);
                    }
                #undef DE_MULTIPLY
                break;
            }
        default:
            IM_ASSERT(0 && "FreeTypeFont::BlitGlyph(): Unknown bitmap pixel mode!");
        }
    }
} // namespace

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

bool ImGui_ImplFreeType_LoaderInit(ImFontAtlas* atlas)
{
    IM_ASSERT(atlas->FontLoaderData == NULL);
    ImGui_ImplFreeType_Data* bd = IM_NEW(ImGui_ImplFreeType_Data)();

    // FreeType memory management: https://www.freetype.org/freetype2/docs/design/design-4.html
    bd->MemoryManager.user = nullptr;
    bd->MemoryManager.alloc = &FreeType_Alloc;
    bd->MemoryManager.free = &FreeType_Free;
    bd->MemoryManager.realloc = &FreeType_Realloc;

    // https://www.freetype.org/freetype2/docs/reference/ft2-module_management.html#FT_New_Library
    FT_Error error = FT_New_Library(&bd->MemoryManager, &bd->Library);
    if (error != 0)
    {
        IM_DELETE(bd);
        return false;
    }

    // If you don't call FT_Add_Default_Modules() the rest of code may work, but FreeType won't use our custom allocator.
    FT_Add_Default_Modules(bd->Library);

#ifdef IMGUI_ENABLE_FREETYPE_LUNASVG
    // Install svg hooks for FreeType
    // https://freetype.org/freetype2/docs/reference/ft2-properties.html#svg-hooks
    // https://freetype.org/freetype2/docs/reference/ft2-svg_fonts.html#svg_fonts
    SVG_RendererHooks hooks = { ImGuiLunasvgPortInit, ImGuiLunasvgPortFree, ImGuiLunasvgPortRender, ImGuiLunasvgPortPresetSlot };
    FT_Property_Set(bd->Library, "ot-svg", "svg-hooks", &hooks);
#endif // IMGUI_ENABLE_FREETYPE_LUNASVG
#ifdef IMGUI_ENABLE_FREETYPE_PLUTOSVG
    // With plutosvg, use provided hooks
    FT_Property_Set(bd->Library, "ot-svg", "svg-hooks", plutosvg_ft_svg_hooks());
#endif // IMGUI_ENABLE_FREETYPE_PLUTOSVG

    // Store our data
    atlas->FontLoaderData = (void*)bd;

    return true;
}

void ImGui_ImplFreeType_LoaderShutdown(ImFontAtlas* atlas)
{
    ImGui_ImplFreeType_Data* bd = (ImGui_ImplFreeType_Data*)atlas->FontLoaderData;
    IM_ASSERT(bd != NULL);
    FT_Done_Library(bd->Library);
    IM_DELETE(bd);
    atlas->FontLoaderData = NULL;
}

bool ImGui_ImplFreeType_FontSrcInit(ImFontAtlas* atlas, ImFontConfig* src)
{
    ImGui_ImplFreeType_Data* bd = (ImGui_ImplFreeType_Data*)atlas->FontLoaderData;
    ImGui_ImplFreeType_FontSrcData* bd_font_data = IM_NEW(ImGui_ImplFreeType_FontSrcData);
    IM_ASSERT(src->FontLoaderData == NULL);
    src->FontLoaderData = bd_font_data;

    if (!bd_font_data->InitFont(bd->Library, src, atlas->FontBuilderFlags))
        return false;

    if (src->MergeMode == false)
    {
        ImFont* font = src->DstFont;
        font->Ascent = bd_font_data->Info.Ascender;
        font->Descent = bd_font_data->Info.Descender;
    }
    return true;
}

void ImGui_ImplFreeType_FontSrcDestroy(ImFontAtlas* atlas, ImFontConfig* src)
{
    IM_UNUSED(atlas);
    ImGui_ImplFreeType_FontSrcData* bd_font_data = (ImGui_ImplFreeType_FontSrcData*)src->FontLoaderData;
    IM_DELETE(bd_font_data);
    src->FontLoaderData = NULL;
}

bool ImGui_ImplFreeType_FontAddGlyph(ImFontAtlas* atlas, ImFont* font, ImFontConfig* srcs, int srcs_count, ImWchar codepoint)
{
    // Search for first font which has the glyph
    ImGui_ImplFreeType_FontSrcData* bd_font_data = NULL;
    ImFontConfig* src = NULL;
    uint32_t glyph_index = 0;
    for (int src_n = 0; src_n < srcs_count; src_n++)
    {
        src = &srcs[src_n];
        bd_font_data = (ImGui_ImplFreeType_FontSrcData*)src->FontLoaderData;
        glyph_index = FT_Get_Char_Index(bd_font_data->FtFace, codepoint);
        if (glyph_index != 0)
            break;
    }
    if (glyph_index == 0)
        return false; // Not found

    const FT_Glyph_Metrics* metrics = bd_font_data->LoadGlyph(codepoint);
    if (metrics == NULL)
        return false;

    // Render glyph into a bitmap (currently held by FreeType)
    FT_Face face = bd_font_data->FtFace;
    FT_GlyphSlot slot = face->glyph;
    FT_Error error = FT_Render_Glyph(slot, bd_font_data->RenderMode);
    if (error != 0)
        return false;

    const FT_Bitmap* ft_bitmap = &slot->bitmap;
    if (ft_bitmap == nullptr)
        return false;

    const int w = (int)ft_bitmap->width;
    const int h = (int)ft_bitmap->rows;
    const bool is_visible = (w != 0 && h != 0);

    // Prepare glyph
    ImFontGlyph glyph = {};
    glyph.Codepoint = codepoint;
    glyph.AdvanceX = (slot->advance.x / FT_SCALEFACTOR) * bd_font_data->InvRasterizationDensity;

    // Pack and retrieve position inside texture atlas
    if (is_visible)
    {
        ImFontAtlasRectId pack_id = ImFontAtlasPackAddRect(atlas, w, h);
        ImFontAtlasRect* r = ImFontAtlasPackGetRect(atlas, pack_id);
        font->MetricsTotalSurface += w * h;

        // Render pixels to our temporary buffer
        atlas->Builder->TempBuffer.resize(w * h * 4);
        uint32_t* temp_buffer = (uint32_t*)atlas->Builder->TempBuffer.Data;
        bd_font_data->BlitGlyph(ft_bitmap, temp_buffer, w);

        float font_off_x = src->GlyphOffset.x;
        float font_off_y = src->GlyphOffset.y + IM_ROUND(font->Ascent);
        float recip_h = 1.0f / src->RasterizerDensity;
        float recip_v = 1.0f / src->RasterizerDensity;

        // Register glyph
        float glyph_off_x = (float)face->glyph->bitmap_left;
        float glyph_off_y = (float)-face->glyph->bitmap_top;
        glyph.X0 = glyph_off_x * recip_h + font_off_x;
        glyph.Y0 = glyph_off_y * recip_v + font_off_y;
        glyph.X1 = (glyph_off_x + w) * recip_h + font_off_x;
        glyph.Y1 = (glyph_off_y + h) * recip_v + font_off_y;
        glyph.U0 = (r->x) * atlas->TexUvScale.x;
        glyph.V0 = (r->y) * atlas->TexUvScale.y;
        glyph.U1 = (r->x + r->w) * atlas->TexUvScale.x;
        glyph.V1 = (r->y + r->h) * atlas->TexUvScale.y;
        glyph.PackId = pack_id;
        glyph.Visible = true;
        glyph.Colored = (ft_bitmap->pixel_mode == FT_PIXEL_MODE_BGRA);
        font->BuildRegisterGlyph(src, &glyph);

        // Copy to texture, post-process and queue update for backend
        ImTextureData* tex = atlas->TexData;
        IM_ASSERT(r->x + r->w <= tex->Width && r->y + r->h <= tex->Height);
        ImFontAtlasTextureBlockConvertAndPostProcess(atlas, font, src, &font->Glyphs.back(),
            temp_buffer, ImTextureFormat_RGBA32, w * 4, tex->GetPixelsAt(r->x, r->y), tex->Format, tex->GetPitch(), w, h);
        ImFontAtlasTextureBlockQueueUpload(atlas, tex, r->x, r->y, r->w, r->h);
    }
    else
    {
        font->BuildRegisterGlyph(src, &glyph);
    }
    return true;
}

bool ImGui_ImplFreetype_FontSrcContainsGlyph(ImFontAtlas* atlas, ImFontConfig* src, ImWchar codepoint)
{
    IM_UNUSED(atlas);
    ImGui_ImplFreeType_FontSrcData* bd_font_data = (ImGui_ImplFreeType_FontSrcData*)src->FontLoaderData;
    int glyph_index = FT_Get_Char_Index(bd_font_data->FtFace, codepoint);
    return glyph_index != 0;
}

const ImFontLoader* ImGuiFreeType::GetFontLoader()
{
    static ImFontLoader loader;
    loader.Name = "freetype";
    loader.LoaderInit = ImGui_ImplFreeType_LoaderInit;
    loader.LoaderShutdown = ImGui_ImplFreeType_LoaderShutdown;
    loader.FontSrcInit = ImGui_ImplFreeType_FontSrcInit;
    loader.FontSrcDestroy = ImGui_ImplFreeType_FontSrcDestroy;
    loader.FontSrcContainsGlyph = ImGui_ImplFreetype_FontSrcContainsGlyph;
    loader.FontAddGlyph = ImGui_ImplFreeType_FontAddGlyph;
    return &loader;
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
