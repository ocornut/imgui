#include "imgui_dwrite.h"
#ifndef IMGUI_DISABLE
#include "imgui_internal.h"

#include <assert.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <dwrite_3.h>

#ifdef _MSC_VER
#pragma comment(lib, "dwrite")
#pragma comment(lib, "user32")
#endif

#define RETURN_FALSE_IF(cond) \
    do { \
        if (cond) { \
            IM_ASSERT(false); \
            return false; \
        } \
    } while (0)

#define RETURN_FALSE_IF_FAILED(hr) \
    RETURN_FALSE_IF(FAILED(hr))

template<typename T>
struct com_ptr
{
    ~com_ptr()
    {
        if (m_ptr)
        {
            m_ptr->Release();
        }
    }

    T* get() const
    {
        return m_ptr;
    }

    T* operator->() const
    {
        return m_ptr;
    }

    T** addressof()
    {
        return &m_ptr;
    }

    T* detach()
    {
        T* ptr = m_ptr;
        m_ptr = nullptr;
        return ptr;
    }

private:
    T* m_ptr = nullptr;
};

template<typename T>
struct unique_im
{
    unique_im(T* ptr) : m_ptr(ptr)
    {}

    ~unique_im()
    {
        IM_DELETE(m_ptr);
    }

    T* operator->()
    {
        return m_ptr;
    }

    T* detach()
    {
        T* ptr = m_ptr;
        m_ptr = nullptr;
        return ptr;
    }

private:
    T* m_ptr;
};

struct TextAnalysisSource final : IDWriteTextAnalysisSource
{
    TextAnalysisSource(const wchar_t* locale, const wchar_t* text, UINT32 text_length) : m_locale(locale), m_text(text), m_text_length(text_length) {}

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return 1;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        return 1;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObject) override
    {
        if (IsEqualGUID(riid, __uuidof(IDWriteTextAnalysisSource)))
        {
            *ppvObject = this;
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE GetTextAtPosition(UINT32 textPosition, const WCHAR** textString, UINT32* textLength) override
    {
        textPosition = ImMin(textPosition, m_text_length);
        *textString = m_text + textPosition;
        *textLength = m_text_length - textPosition;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetTextBeforePosition(UINT32 textPosition, const WCHAR** textString, UINT32* textLength) override
    {
        textPosition = ImMin(textPosition, m_text_length);
        *textString = m_text;
        *textLength = textPosition;
        return S_OK;
    }

    DWRITE_READING_DIRECTION STDMETHODCALLTYPE GetParagraphReadingDirection() override
    {
        return DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    }

    HRESULT STDMETHODCALLTYPE GetLocaleName(UINT32 textPosition, UINT32* textLength, const WCHAR** localeName) override
    {
        *textLength = m_text_length - textPosition;
        *localeName = m_locale;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetNumberSubstitution(UINT32, UINT32*, IDWriteNumberSubstitution**) override
    {
        return E_NOTIMPL;
    }

private:
    const wchar_t* m_locale;
    const wchar_t* m_text;
    UINT32 m_text_length;
};

struct DWriteFontSource
{
    com_ptr<IDWriteFactory5> factory;
    com_ptr<IDWriteInMemoryFontFileLoader> memory_loader;
    com_ptr<IDWriteRenderingParams> rendering_params;
    com_ptr<IDWriteFontFace3> font_face;
    com_ptr<IDWriteFontCollection> font_collection;
    com_ptr<IDWriteFontFallback1> font_fallback;
    wchar_t family_name[IM_COUNTOF(ImFontConfig::Name)];
    float em_per_unit;
    float ascent;
    float descent;

    ~DWriteFontSource()
    {
        if (memory_loader.get())
            factory->UnregisterFontFileLoader(memory_loader.get());
    }
};

static bool font_src_init(ImFontAtlas*, ImFontConfig* src)
{
    unique_im<DWriteFontSource> data(IM_NEW(DWriteFontSource));

    // Create "global" objects. System font collection, font fallback handler, etc.
    com_ptr<IDWriteFontFallback> font_fallback;
    RETURN_FALSE_IF_FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory5), (IUnknown**)data->factory.addressof()));
    RETURN_FALSE_IF_FAILED(data->factory->CreateRenderingParams(data->rendering_params.addressof()));
    RETURN_FALSE_IF_FAILED(data->factory->GetSystemFontCollection(data->font_collection.addressof(), FALSE));
    RETURN_FALSE_IF_FAILED(data->factory->GetSystemFontFallback(font_fallback.addressof()));
    RETURN_FALSE_IF_FAILED(font_fallback->QueryInterface(data->font_fallback.addressof()));

    // Create a `IDWriteFontFace` (a single stylistic variant) out of `src`.
    if (src->FontData || src->FontDataSize)
    {
        // In-memory font? `IDWriteInMemoryFontFileLoader`!

        IM_ASSERT_USER_ERROR(src->FontData != nullptr && src->FontDataSize > 0, "Invalid font data");

        // DWriteFontSource assumes that memory_loader != null means it needs to be unregistered.
        // As such, we create and register the loader first and then transfer ownership into `data`.
        com_ptr<IDWriteInMemoryFontFileLoader> memory_loader;
        RETURN_FALSE_IF_FAILED(data->factory->CreateInMemoryFontFileLoader(memory_loader.addressof()));
        RETURN_FALSE_IF_FAILED(data->factory->RegisterFontFileLoader(memory_loader.get()));
        *data->memory_loader.addressof() = memory_loader.detach();

        com_ptr<IDWriteFontFile> font_file;
        com_ptr<IDWriteFontFaceReference> font_reference;
        RETURN_FALSE_IF_FAILED(data->memory_loader->CreateInMemoryFontFileReference(data->factory.get(), src->FontData, (UINT32)src->FontDataSize, nullptr, font_file.addressof()));
        RETURN_FALSE_IF_FAILED(data->factory->CreateFontFaceReference(font_file.get(), src->FontNo, DWRITE_FONT_SIMULATIONS_NONE, font_reference.addressof()));
        RETURN_FALSE_IF_FAILED(font_reference->CreateFontFace(data->font_face.addressof()));
    }
    else
    {
        // If no font data is present, we assume the Name field contains the family name of the font.
        // Name -> Name, but wchar -> Find font family -> Get any variant out of it.

        const int name_utf8_len = (int)strnlen(src->Name, IM_COUNTOF(src->Name));
        const int name_utf16_len = MultiByteToWideChar(CP_UTF8, 0, src->Name, name_utf8_len, &data->family_name[0], IM_COUNTOF(data->family_name) - 1);
        RETURN_FALSE_IF(name_utf16_len <= 0);
        data->family_name[name_utf16_len] = L'\0';

        UINT32 family_index;
        BOOL family_exists;
        RETURN_FALSE_IF_FAILED(data->font_collection->FindFamilyName(&data->family_name[0], &family_index, &family_exists));
        RETURN_FALSE_IF(!family_exists);

        com_ptr<IDWriteFontFamily> font_family;
        com_ptr<IDWriteFont> font;
        com_ptr<IDWriteFontFace> font_face;
        RETURN_FALSE_IF_FAILED(data->font_collection->GetFontFamily(family_index, font_family.addressof()));
        RETURN_FALSE_IF_FAILED(font_family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, font.addressof()));
        RETURN_FALSE_IF_FAILED(font->CreateFontFace(font_face.addressof()));
        RETURN_FALSE_IF_FAILED(font_face->QueryInterface(data->font_face.addressof()));
    }

    DWRITE_FONT_METRICS metrics;
    data->font_face->GetMetrics(&metrics);

    // Calculate the ascent/descent of the font in "em".
    // 1.1em (in total) means that a 20px font has a 22px line height.
    //
    // Modern typographical metrics feature a line gap, which indicates the spacing between lines.
    // Since ImGui has no concept of a line gap, we add 50% onto the ascent/descent respectively.
    const float ascent = metrics.ascent;
    const float descent = metrics.descent;
    const float half_line_gap = (float)metrics.lineGap * 0.5f;
    const float em_per_unit = 1.0f / (float)metrics.designUnitsPerEm;
    data->em_per_unit = em_per_unit;
    data->ascent = (ascent + half_line_gap) * em_per_unit;
    data->descent = (descent + half_line_gap) * em_per_unit;

    src->FontLoaderData = data.detach();
    return true;
}

static void font_src_destroy(ImFontAtlas*, ImFontConfig* src)
{
    IM_DELETE((DWriteFontSource*)src->FontLoaderData);
    src->FontLoaderData = nullptr;
}

static UINT16 map_codepoint_with_fallback(DWriteFontSource* data, ImWchar codepoint, IDWriteFontFace3** font_face)
{
    const UINT32 codepoint32 = codepoint;

    // "Fast" path: Try the primary font. DirectWrite implements GetGlyphIndices as O(1).
    UINT16 glyph_index;
    if (FAILED(data->font_face->GetGlyphIndices(&codepoint32, 1, &glyph_index)))
        return 0;
    if (glyph_index != 0)
    {
        data->font_face->AddRef();
        *font_face = data->font_face.get();
        return glyph_index;
    }

#ifdef IMGUI_USE_WCHAR32
    wchar_t text[2];
    UINT32 text_length;
    if (codepoint > 0xFFFF)
    {
        text[0] = (wchar_t)(0xD800 + ((codepoint - 0x10000) >> 10));
        text[1] = (wchar_t)(0xDC00 + ((codepoint - 0x10000) & 0x3FF));
        text_length = 2;
    }
    else
    {
        text[0] = (wchar_t)codepoint;
        text_length = 1;
    }
#else
    const wchar_t* text = (const wchar_t*)&codepoint;
    const UINT32 text_length = 1;
#endif

    TextAnalysisSource analysis_source(ImGuiDWrite::GetUserDefaultLocaleName(), text, text_length);
    UINT32 mapped_length;
    float scale;
    IDWriteFontFace5* font_face5;
    if (FAILED(data->font_fallback->MapCharacters(
        /* analysisSource     */ &analysis_source,
        /* textPosition       */ 0,
        /* textLength         */ text_length,
        /* baseFontCollection */ data->font_collection.get(),
        /* baseFamilyName     */ data->family_name,
        /* fontAxisValues     */ nullptr,
        /* fontAxisValueCount */ 0,
        /* mappedLength       */ &mapped_length,
        /* scale              */ &scale,
        /* mappedFontFace     */ &font_face5
    )))
        return 0;

    // NOTE: MapCharacters() can return S_OK and yet mappedFontFace can be null.
    // This indicates that no font contains the requested codepoint.
    if (!font_face5 || FAILED(font_face5->GetGlyphIndices(&codepoint32, 1, &glyph_index)))
        glyph_index = 0;

    *font_face = font_face5;
    return glyph_index;
}

static bool font_src_contains_glyph(ImFontAtlas*, ImFontConfig* src, ImWchar codepoint)
{
    DWriteFontSource* data = (DWriteFontSource*)src->FontLoaderData;
    com_ptr<IDWriteFontFace3> font_face;
    return map_codepoint_with_fallback(data, codepoint, font_face.addressof()) != 0;
}

static bool font_baked_init(ImFontAtlas*, ImFontConfig* src, ImFontBaked* baked, void*)
{
    if (!src->MergeMode)
    {
        DWriteFontSource* data = (DWriteFontSource*)src->FontLoaderData;
        const float rasterizer_scale = src->RasterizerDensity * baked->RasterizerDensity * src->ExtraSizeScale;
        const float font_size = baked->Size * rasterizer_scale / (data->ascent + data->descent);
        baked->Ascent = ImCeil(font_size * data->ascent) / rasterizer_scale;
        baked->Descent = -ImCeil(font_size * data->descent) / rasterizer_scale;
    }
    return true;
}

static bool font_baked_load_glyph(ImFontAtlas* atlas, ImFontConfig* src, ImFontBaked* baked, void*, ImWchar codepoint, ImFontGlyph* out_glyph, float* out_advance_x)
{
    DWriteFontSource* data = (DWriteFontSource*)src->FontLoaderData;

    com_ptr<IDWriteFontFace3> font_face;
    const UINT16 glyph_index = map_codepoint_with_fallback(data, codepoint, font_face.addressof());
    if (glyph_index == 0)
        return false;

    // ImGui's font size is actually the line height. Here, we reverse-calculate the actual
    // font size by simply dividing the height in em. The result is in 96 DPI pixels.
    const float ref_size = baked->OwnerFont->Sources[0]->SizePixels;
    float font_size = baked->Size * src->ExtraSizeScale / (data->ascent + data->descent);
    if (src->MergeMode && src->SizePixels != 0.0f)
        font_size *= src->SizePixels / ref_size;

    // DirectWrite yields fractional advance widths, but simultaneously we don't implement oversampling.
    // To prevent ugly smearing, we need to ensure to only return integer advances.
    DWRITE_GLYPH_METRICS metrics;
    RETURN_FALSE_IF_FAILED(font_face->GetDesignGlyphMetrics(&glyph_index, 1, &metrics, FALSE));
    const float advance_x = IM_ROUND((float)metrics.advanceWidth * data->em_per_unit * font_size);

    if (out_advance_x)
    {
        IM_ASSERT(out_glyph == nullptr);
        *out_advance_x = advance_x;
        return true;
    }

    const DWRITE_GLYPH_OFFSET zero_offset = {};
    const float rasterizer_density = src->RasterizerDensity * baked->RasterizerDensity;
    const DWRITE_GLYPH_RUN glyph_run = {
        /* fontFace      */ font_face.get(),
        /* fontEmSize    */ font_size * rasterizer_density,
        /* glyphCount    */ 1,
        /* glyphIndices  */ &glyph_index,
        /* glyphAdvances */ nullptr,
        /* glyphOffsets  */ &zero_offset,
        /* isSideways    */ FALSE,
        /* bidiLevel     */ 0,
    };

    // If you read SDK headers you may notice the very enticing IDWriteBitmapRenderTarget3::DrawGlyphRunWithColorSupport
    // function, but (and these words are written in anger) that one is exclusive to the Windows App SDK.
    //
    // In the future, it may be reasonable to call IDWriteFactory4::TranslateColorGlyphRun here,
    // and manually compose COLRv0 layers into a single bitmap on the CPU side. Using Direct2D
    // would not be a good idea as it does not interop well with various graphics debuggers.

    // To properly support GASP, a call to GetRecommendedRenderingMode is required.
    DWRITE_RENDERING_MODE1 rendering_mode;
    DWRITE_GRID_FIT_MODE grid_fit_mode;
    com_ptr<IDWriteGlyphRunAnalysis> analysis;
    RECT bounds;
    RETURN_FALSE_IF_FAILED(font_face->GetRecommendedRenderingMode(
        /* fontEmSize       */ glyph_run.fontEmSize,
        /* dpiX             */ 96.0f, // fontEmSize is already in display pixels
        /* dpiY             */ 96.0f,
        /* transform        */ nullptr,
        /* isSideways       */ FALSE,
        /* outlineThreshold */ DWRITE_OUTLINE_THRESHOLD_ANTIALIASED,
        /* measuringMode    */ DWRITE_MEASURING_MODE_NATURAL,
        /* renderingParams  */ data->rendering_params.get(),
        /* renderingMode    */ &rendering_mode,
        /* gridFitMode      */ &grid_fit_mode
    ));
    RETURN_FALSE_IF_FAILED(data->factory->CreateGlyphRunAnalysis(
        /* glyphRun         */ &glyph_run,
        /* transform        */ nullptr,
        /* renderingMode    */ rendering_mode,
        /* measuringMode    */ DWRITE_MEASURING_MODE_NATURAL,
        /* gridFitMode      */ grid_fit_mode,
        /* antialiasMode    */ DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE,
        /* baselineOriginX  */ 0,
        /* baselineOriginY  */ 0,
        /* glyphRunAnalysis */ analysis.addressof()
    ));
    // Even if it says "aliased", it actually produces an antialiased 8-bit grayscale texture.
    // I don't get it either. I don't think anyone at Microsoft does.
    RETURN_FALSE_IF_FAILED(analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &bounds));

    out_glyph->Codepoint = codepoint;
    out_glyph->AdvanceX = advance_x;

    // Invisible glyphs (whitespace) produce empty bounds.
    // out_glyph is initialized with Visible = false, so we can just return here.
    if (IsRectEmpty(&bounds))
        return true;

    const int width = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;

    ImVector<ImU8> pixels;
    pixels.resize(width * height);
    RETURN_FALSE_IF_FAILED(analysis->CreateAlphaTexture(DWRITE_TEXTURE_ALIASED_1x1, &bounds, pixels.Data, pixels.Size));

    const ImFontAtlasRectId pack_id = ImFontAtlasPackAddRect(atlas, width, height);
    RETURN_FALSE_IF(pack_id == ImFontAtlasRectId_Invalid);

    const float offsets_scale = (ref_size != 0.0f) ? (baked->Size / ref_size) : 1.0f;
    const float offset_x = ImFloor(src->GlyphOffset.x * offsets_scale + 0.5f);
    const float offset_y = ImFloor(src->GlyphOffset.y * offsets_scale + 0.5f) + baked->Ascent;
    const float inv_density = 1.0f / rasterizer_density;
    out_glyph->X0 = (float)bounds.left * inv_density + offset_x;
    out_glyph->Y0 = (float)bounds.top * inv_density + offset_y;
    out_glyph->X1 = (float)bounds.right * inv_density + offset_x;
    out_glyph->Y1 = (float)bounds.bottom * inv_density + offset_y;
    out_glyph->Visible = true;
    out_glyph->PackId = pack_id;

    ImTextureRect* rect = ImFontAtlasPackGetRect(atlas, pack_id);
    ImFontAtlasBakedSetFontGlyphBitmap(atlas, baked, src, out_glyph, rect, pixels.Data, ImTextureFormat_Alpha8, width);
    return true;
}

const wchar_t* ImGuiDWrite::GetUserDefaultLocaleName()
{
    static wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];

    if (locale_name[0] == L'\0')
    {
        if (!::GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH))
        {
            static const wchar_t fallback_locale[] = L"en-US";
            memcpy(&locale_name[0], &fallback_locale[0], sizeof(fallback_locale));
        }
    }

    return &locale_name[0];
}

const ImFontLoader* ImGuiDWrite::GetFontLoader()
{
    static ImFontLoader loader;
    loader.Name = "DirectWrite";
    loader.FontSrcInit = font_src_init;
    loader.FontSrcDestroy = font_src_destroy;
    loader.FontSrcContainsGlyph = font_src_contains_glyph;
    loader.FontBakedInit = font_baked_init;
    loader.FontBakedDestroy = nullptr;
    loader.FontBakedLoadGlyph = font_baked_load_glyph;
    return &loader;
}

#endif
