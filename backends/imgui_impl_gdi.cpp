// dear imgui: Renderer Backend for Windows GDI
// This needs to be used along with a Platform Backend (e.g. Win32)

// Implemented features:
//  [x] Renderer: Basic Implementation

// The aim of imgui_impl_gdi.h/.cpp is to be usable in your engine without any modification.
// IF YOU FEEL YOU NEED TO MAKE ANY CHANGE TO THIS CODE, please share them and your feedback at https://github.com/ocornut/imgui/

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2026-05-20: Initial version.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_gdi.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <math.h>

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning: use of old-style cast                            // yes, they are more terse.
#endif

namespace
{
    template<typename Type>
    Type InternalClamp(Type Value, Type Minimum, Type Maximum)
    {
        return ((Value < Minimum)
            ? Minimum
            : ((Value > Maximum) ? Maximum : Value));
    }
}

// Texture data
struct ImGui_ImplGDI_Texture
{
    int Width;
    int Height;
    ImU32* Pixels;

    ImGui_ImplGDI_Texture()
    {
        memset((void*)this, 0, sizeof(*this));
    }
};

static void ImGui_ImplGDI_UpdateTexture(ImTextureData* tex)
{
    if (tex->Status == ImTextureStatus_WantCreate)
    {
        // Create and upload new texture to graphics system
        //IMGUI_DEBUG_LOG("UpdateTexture #%03d: WantCreate %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
        IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);

        ImGui_ImplGDI_Texture* texture = IM_NEW(ImGui_ImplGDI_Texture)();
        IM_ASSERT(texture != nullptr && "Failed to allocate memory for texture data!");
        texture->Width = tex->Width;
        texture->Height = tex->Height;
        texture->Pixels = (ImU32*)IM_ALLOC(texture->Width * texture->Height * sizeof(ImU32));
        IM_ASSERT(texture->Pixels != nullptr && "Failed to allocate memory for texture pixels!");
        memcpy(texture->Pixels, tex->GetPixels(), texture->Width * texture->Height * sizeof(ImU32));
        tex->SetTexID((ImTextureID)texture);

        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantUpdates)
    {
        // Update selected blocks. We only ever write to textures regions which have never been used before!
        // This backend choose to use tex->Updates[] but you can use tex->UpdateRect to upload a single region.
        ImGui_ImplGDI_Texture* texture = (ImGui_ImplGDI_Texture*)tex->GetTexID();
        IM_ASSERT(texture != nullptr && "Trying to update a texture that was not created or already destroyed!");
        IM_ASSERT(texture->Pixels != nullptr && "Trying to update a texture that was not created or already destroyed!");
        IM_FREE(texture->Pixels);
        texture->Width = tex->Width;
        texture->Height = tex->Height;
        texture->Pixels = (ImU32*)IM_ALLOC(texture->Width * texture->Height * sizeof(ImU32));
        IM_ASSERT(texture->Pixels != nullptr && "Failed to allocate memory for texture pixels!");
        memcpy(texture->Pixels, tex->GetPixels(), texture->Width * texture->Height * sizeof(ImU32));

        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantDestroy)
    {
        ImGui_ImplGDI_Texture* texture = (ImGui_ImplGDI_Texture*)tex->GetTexID();
        IM_ASSERT(texture != nullptr && "Trying to destroy a texture that was not created or already destroyed!");
        IM_ASSERT(texture->Pixels != nullptr && "Trying to destroy a texture that was not created or already destroyed!");
        IM_FREE(texture->Pixels);
        IM_DELETE(texture);
        tex->SetTexID(ImTextureID_Invalid);

        tex->SetStatus(ImTextureStatus_Destroyed);
    }
}

IMGUI_IMPL_API bool ImGui_ImplGDI_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();

    // Setup backend capabilities flags
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    return true;
}

IMGUI_IMPL_API void ImGui_ImplGDI_Shutdown()
{
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
    {
        if (tex->RefCount == 1)
        {
            tex->SetStatus(ImTextureStatus_WantDestroy);
            ImGui_ImplGDI_UpdateTexture(tex);
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
}

IMGUI_IMPL_API void ImGui_ImplGDI_NewFrame()
{

}

static bool ImGui_ImplGDI_RenderDrawRectangle(HDC hdc, HDC solid_alpha_dev_ctx_handle, ImU32* solid_alpha_pixel, const int& width, const int& height, const ImVec4& clip_rect, const ImDrawVert* v1, const ImDrawVert* v2, const ImDrawVert* v3, const ImDrawVert* v4, const ImDrawVert* v5, const ImDrawVert* v6)
{
    if (!hdc || width <= 0 || height <= 0)
        return false;

    const ImDrawVert* verts[6] = { v1, v2, v3, v4, v5, v6 };

    for (int i = 0; i < 6; i++)
    {
        if (!verts[i])
            return false;
    }

    const float eps = 0.01f;

    auto near_float = [eps](float a, float b) -> bool
    {
        return fabsf(a - b) <= eps;
    };

    // Must be the same color, otherwise it's not a simple rectangle that FillRect can handle.
    ImU32 col = verts[0]->col;

    for (int i = 1; i < 6; i++)
    {
        if (verts[i]->col != col)
            return false;
    }

    // Must have the same UV, otherwise it's not a simple rectangle.
    // Text glyphs and image quads usually have different UVs and cannot be treated as solid rectangles.
    ImVec2 uv = verts[0]->uv;

    for (int i = 1; i < 6; i++)
    {
        if (!near_float(verts[i]->uv.x, uv.x) || !near_float(verts[i]->uv.y, uv.y))
            return false;
    }

    // Extract unique positions. A rectangle made of two triangles should have 4 unique vertices.

    ImVec2 unique_pos[4];
    int unique_count[4] = {};
    int unique_n = 0;

    for (int i = 0; i < 6; i++)
    {
        const ImVec2& p = verts[i]->pos;

        bool found = false;

        for (int j = 0; j < unique_n; j++)
        {
            if (near_float(unique_pos[j].x, p.x) && near_float(unique_pos[j].y, p.y))
            {
                unique_count[j]++;
                found = true;
                break;
            }
        }

        if (!found)
        {
            if (unique_n >= 4)
                return false;

            unique_pos[unique_n] = p;
            unique_count[unique_n] = 1;
            unique_n++;
        }
    }

    if (unique_n != 4)
        return false;

    // The repeat count of the two triangles in a regular rectangle should be 2,2,1,1.

    int count_1 = 0;
    int count_2 = 0;

    for (int i = 0; i < 4; i++)
    {
        if (unique_count[i] == 1)
            count_1++;
        else if (unique_count[i] == 2)
            count_2++;
        else
            return false;
    }

    if (count_1 != 2 || count_2 != 2)
        return false;

    // Calculate the bounding box of the 4 unique points.

    float min_x = unique_pos[0].x;
    float max_x = unique_pos[0].x;
    float min_y = unique_pos[0].y;
    float max_y = unique_pos[0].y;

    for (int i = 1; i < 4; i++)
    {
        if (unique_pos[i].x < min_x) min_x = unique_pos[i].x;
        if (unique_pos[i].x > max_x) max_x = unique_pos[i].x;
        if (unique_pos[i].y < min_y) min_y = unique_pos[i].y;
        if (unique_pos[i].y > max_y) max_y = unique_pos[i].y;
    }

    if (max_x <= min_x || max_y <= min_y)
        return true;

    // Check if the 4 unique points are exactly the 4 corners of the bounding box.

    int corner_mask = 0;

    for (int i = 0; i < 4; i++)
    {
        int ix = -1;
        int iy = -1;

        if (near_float(unique_pos[i].x, min_x))
            ix = 0;
        else if (near_float(unique_pos[i].x, max_x))
            ix = 1;
        else
            return false;

        if (near_float(unique_pos[i].y, min_y))
            iy = 0;
        else if (near_float(unique_pos[i].y, max_y))
            iy = 1;
        else
            return false;

        int bit = iy * 2 + ix;

        if (corner_mask & (1 << bit))
            return false;

        corner_mask |= (1 << bit);
    }

    if (corner_mask != 0x0F)
        return false;

    // Convert to pixel RECT. We use the same x+0.5/y+0.5 pixel center rule as the software triangle renderer to avoid off-by-one issues.

    int x0 = (int)ceilf(min_x - 0.5f);
    int y0 = (int)ceilf(min_y - 0.5f);
    int x1 = (int)ceilf(max_x - 0.5f);
    int y1 = (int)ceilf(max_y - 0.5f);

    int clip_x0 = (int)ceilf(clip_rect.x - 0.5f);
    int clip_y0 = (int)ceilf(clip_rect.y - 0.5f);
    int clip_x1 = (int)ceilf(clip_rect.z - 0.5f);
    int clip_y1 = (int)ceilf(clip_rect.w - 0.5f);

    if (x0 < clip_x0) x0 = clip_x0;
    if (y0 < clip_y0) y0 = clip_y0;
    if (x1 > clip_x1) x1 = clip_x1;
    if (y1 > clip_y1) y1 = clip_y1;

    x0 = ::InternalClamp(x0, 0, width);
    y0 = ::InternalClamp(y0, 0, height);
    x1 = ::InternalClamp(x1, 0, width);
    y1 = ::InternalClamp(y1, 0, height);

    if (x0 >= x1 || y0 >= y1)
        return true;

    // Extract RGBA components from ImGui's vertex color format.

    int r = (col >> IM_COL32_R_SHIFT) & 0xff;
    int g = (col >> IM_COL32_G_SHIFT) & 0xff;
    int b = (col >> IM_COL32_B_SHIFT) & 0xff;
    int a = (col >> IM_COL32_A_SHIFT) & 0xff;

    if (a <= 0)
        return true;

    RECT rect;
    rect.left = x0;
    rect.top = y0;
    rect.right = x1;
    rect.bottom = y1;

    // Fast path for fully opaque rectangles. No alpha blending needed.
    if (a >= 255)
    {
        COLORREF previous_color = ::SetDCBrushColor(hdc, RGB(r, g, b));
        ::FillRect(hdc, &rect, (HBRUSH)::GetStockObject(DC_BRUSH));
        ::SetDCBrushColor(hdc, previous_color);
        return true;
    }

    // Fast path for semi-transparent rectangles.
    
    // The 32-bit BI_RGB DIB memory byte order is B,G,R,X.
    // You can write the uint32_t value as 0x00RRGGBB.
    *solid_alpha_pixel =
        ((ImU32)r << 16) |
        ((ImU32)g << 8) |
        ((ImU32)b << 0);

    BLENDFUNCTION blend_function = {};
    blend_function.BlendOp = AC_SRC_OVER;
    blend_function.BlendFlags = 0;
    blend_function.SourceConstantAlpha = (BYTE)a;
    blend_function.AlphaFormat = 0;
    bool result = ::GdiAlphaBlend(hdc, x0, y0, x1 - x0, y1 - y0, solid_alpha_dev_ctx_handle, 0, 0, 1, 1, blend_function);

    return result;
}

static void ImGui_ImplGDI_RenderDrawTriangle(ImU32* pixel_buffer, const int& width, const int& height, const ImVec4& clip_rect, const ImDrawVert* v1, const ImDrawVert* v2, const ImDrawVert* v3, ImGui_ImplGDI_Texture* texture)
{
    if (!pixel_buffer || width <= 0 || height <= 0)
        return;

    if (!v1 || !v2 || !v3)
        return;

    // If the texture is not valid, we will treat it as a pure white texture.
    const bool has_texture = texture && texture->Pixels && texture->Width > 0 && texture->Height > 0;

    // If your pixel_buffer is in the same IM_COL32 format as ImGui, set this to false.
    // GDI 32-bit DIB is typically BGRA, so we set this to true.
    const bool framebuffer_is_gdi_bgra = true;

    auto mul255 = [](int a, int b) -> int
    {
        return (a * b + 127) / 255;
    };

    auto edge = [](const ImVec2& a, const ImVec2& b, float px, float py) -> float
    {
        return (px - a.x) * (b.y - a.y) - (py - a.y) * (b.x - a.x);
    };

    auto is_top_left = [](const ImVec2& a, const ImVec2& b) -> bool
    {
        float dx = b.x - a.x;
        float dy = b.y - a.y;

        // This rule is to avoid double blending when two triangles share an edge, which can cause a darker diagonal line in semi-transparent rectangles.
        return (dy < 0.0f) || (dy == 0.0f && dx > 0.0f);
    };

    auto unpack_imgui_color = [](ImU32 c, int& r, int& g, int& b, int& a)
    {
        r = (c >> IM_COL32_R_SHIFT) & 0xff;
        g = (c >> IM_COL32_G_SHIFT) & 0xff;
        b = (c >> IM_COL32_B_SHIFT) & 0xff;
        a = (c >> IM_COL32_A_SHIFT) & 0xff;
    };

    auto unpack_framebuffer_color = [framebuffer_is_gdi_bgra](ImU32 c, int& r, int& g, int& b, int& a)
    {
        if (framebuffer_is_gdi_bgra)
        {
            // uint32_t value: 0xAARRGGBB
            // memory bytes:   BB GG RR AA
            b = (c >> 0) & 0xff;
            g = (c >> 8) & 0xff;
            r = (c >> 16) & 0xff;
            a = (c >> 24) & 0xff;
        }
        else
        {
            r = (c >> IM_COL32_R_SHIFT) & 0xff;
            g = (c >> IM_COL32_G_SHIFT) & 0xff;
            b = (c >> IM_COL32_B_SHIFT) & 0xff;
            a = (c >> IM_COL32_A_SHIFT) & 0xff;
        }
    };

    auto pack_framebuffer_color = [framebuffer_is_gdi_bgra](int r, int g, int b, int a) -> ImU32
    {
        r = ::InternalClamp(r, 0, 255);
        g = ::InternalClamp(g, 0, 255);
        b = ::InternalClamp(b, 0, 255);
        a = ::InternalClamp(a, 0, 255);

        if (framebuffer_is_gdi_bgra)
        {
            // GDI DIB BGRA memory layout.
            return ((ImU32)a << 24) |
                ((ImU32)r << 16) |
                ((ImU32)g << 8) |
                ((ImU32)b << 0);
        }
        else
        {
            return IM_COL32(r, g, b, a);
        }
    };

    auto blend_over = [&](ImU32* dst, int src_r, int src_g, int src_b, int src_a)
    {
        if (src_a <= 0)
            return;

        if (src_a >= 255)
        {
            *dst = pack_framebuffer_color(src_r, src_g, src_b, 255);
            return;
        }

        int dst_r, dst_g, dst_b, dst_a;
        unpack_framebuffer_color(*dst, dst_r, dst_g, dst_b, dst_a);

        int inv_a = 255 - src_a;

        int out_r = (src_r * src_a + dst_r * inv_a + 127) / 255;
        int out_g = (src_g * src_a + dst_g * inv_a + 127) / 255;
        int out_b = (src_b * src_a + dst_b * inv_a + 127) / 255;

        // For normal GDI framebuffer, we can just set alpha to 255.
        *dst = pack_framebuffer_color(out_r, out_g, out_b, 255);
    };

    const ImVec2& p1 = v1->pos;
    const ImVec2& p2 = v2->pos;
    const ImVec2& p3 = v3->pos;

    float area = edge(p1, p2, p3.x, p3.y);

    if (area > -0.000001f && area < 0.000001f)
        return;

    float inv_area = 1.0f / area;

    float min_x = p1.x;
    float min_y = p1.y;
    float max_x = p1.x;
    float max_y = p1.y;

    if (p2.x < min_x) min_x = p2.x;
    if (p3.x < min_x) min_x = p3.x;

    if (p2.y < min_y) min_y = p2.y;
    if (p3.y < min_y) min_y = p3.y;

    if (p2.x > max_x) max_x = p2.x;
    if (p3.x > max_x) max_x = p3.x;

    if (p2.y > max_y) max_y = p2.y;
    if (p3.y > max_y) max_y = p3.y;

    // Triangle bounding box.

    int x0 = (int)floorf(min_x);
    int y0 = (int)floorf(min_y);
    int x1 = (int)ceilf(max_x);
    int y1 = (int)ceilf(max_y);

    // The clip_rect is in pixel boundary; here we clip with pixel center x+0.5/y+0.5.

    int clip_x0 = (int)ceilf(clip_rect.x - 0.5f);
    int clip_y0 = (int)ceilf(clip_rect.y - 0.5f);
    int clip_x1 = (int)ceilf(clip_rect.z - 0.5f);
    int clip_y1 = (int)ceilf(clip_rect.w - 0.5f);

    if (x0 < clip_x0) x0 = clip_x0;
    if (y0 < clip_y0) y0 = clip_y0;
    if (x1 > clip_x1) x1 = clip_x1;
    if (y1 > clip_y1) y1 = clip_y1;

    x0 = ::InternalClamp(x0, 0, width);
    y0 = ::InternalClamp(y0, 0, height);
    x1 = ::InternalClamp(x1, 0, width);
    y1 = ::InternalClamp(y1, 0, height);

    if (x0 >= x1 || y0 >= y1)
        return;

    int c1_r, c1_g, c1_b, c1_a;
    int c2_r, c2_g, c2_b, c2_a;
    int c3_r, c3_g, c3_b, c3_a;

    unpack_imgui_color(v1->col, c1_r, c1_g, c1_b, c1_a);
    unpack_imgui_color(v2->col, c2_r, c2_g, c2_b, c2_a);
    unpack_imgui_color(v3->col, c3_r, c3_g, c3_b, c3_a);

    bool tl1 = is_top_left(p2, p3);
    bool tl2 = is_top_left(p3, p1);
    bool tl3 = is_top_left(p1, p2);

    for (int y = y0; y < y1; y++)
    {
        float py = (float)y + 0.5f;

        for (int x = x0; x < x1; x++)
        {
            float px = (float)x + 0.5f;

            float e1 = edge(p2, p3, px, py);
            float e2 = edge(p3, p1, px, py);
            float e3 = edge(p1, p2, px, py);

            // inside test which supports both clockwise and counter-clockwise winding.

            float s1 = e1 * area;
            float s2 = e2 * area;
            float s3 = e3 * area;

            if (s1 < 0.0f) continue;
            if (s2 < 0.0f) continue;
            if (s3 < 0.0f) continue;

            // top-left rule to avoid unnecessary blending when two triangles share an edge.

            if (s1 == 0.0f && !tl1) continue;
            if (s2 == 0.0f && !tl2) continue;
            if (s3 == 0.0f && !tl3) continue;

            float w1 = e1 * inv_area;
            float w2 = e2 * inv_area;
            float w3 = e3 * inv_area;

            float u = v1->uv.x * w1 + v2->uv.x * w2 + v3->uv.x * w3;
            float v = v1->uv.y * w1 + v2->uv.y * w2 + v3->uv.y * w3;

            int tex_r = 255;
            int tex_g = 255;
            int tex_b = 255;
            int tex_a = 255;

            if (has_texture)
            {
                int tx = (int)(u * texture->Width);
                int ty = (int)(v * texture->Height);

                tx = ::InternalClamp(tx, 0, texture->Width - 1);
                ty = ::InternalClamp(ty, 0, texture->Height - 1);

                ImU32 tex_col = texture->Pixels[ty * texture->Width + tx];
                unpack_imgui_color(tex_col, tex_r, tex_g, tex_b, tex_a);
            }

            int vert_r = (int)(c1_r * w1 + c2_r * w2 + c3_r * w3 + 0.5f);
            int vert_g = (int)(c1_g * w1 + c2_g * w2 + c3_g * w3 + 0.5f);
            int vert_b = (int)(c1_b * w1 + c2_b * w2 + c3_b * w3 + 0.5f);
            int vert_a = (int)(c1_a * w1 + c2_a * w2 + c3_a * w3 + 0.5f);

            vert_r = ::InternalClamp(vert_r, 0, 255);
            vert_g = ::InternalClamp(vert_g, 0, 255);
            vert_b = ::InternalClamp(vert_b, 0, 255);
            vert_a = ::InternalClamp(vert_a, 0, 255);

            // ImGui rendering looks like this:
            // final_rgb = texture_rgb * vertex_rgb
            // final_a   = texture_a   * vertex_a

            int src_r = mul255(tex_r, vert_r);
            int src_g = mul255(tex_g, vert_g);
            int src_b = mul255(tex_b, vert_b);
            int src_a = mul255(tex_a, vert_a);

            ImU32* dst = pixel_buffer + y * width + x;
            blend_over(dst, src_r, src_g, src_b, src_a);
        }
    }
}

IMGUI_IMPL_API void ImGui_ImplGDI_RenderDrawData(ImDrawData* draw_data, void* fb_dev_ctx_handle, ImVec4* clear_color)
{
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    if (fb_width == 0 || fb_height == 0)
        return;

    HDC hdc = (HDC)fb_dev_ctx_handle;
    IM_ASSERT(hdc != nullptr && "Invalid framebuffer device context!");

    // Catch up with texture updates. Most of the times, the list will have 1 element with an OK status, aka nothing to do.
    // (This almost always points to ImGui::GetPlatformIO().Textures[] but is part of ImDrawData to allow overriding or disabling texture updates).
    if (draw_data->Textures != nullptr)
        for (ImTextureData* tex : *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_ImplGDI_UpdateTexture(tex);

    // Setup desired GDI state

    HDC solid_alpha_dev_ctx_handle = nullptr;
    ImU32* solid_alpha_pixel = nullptr;

    // Create a temporary 1x1 DIB with the rectangle color and use GdiAlphaBlend to stretch it over the target rectangle.
    {
        solid_alpha_dev_ctx_handle = ::CreateCompatibleDC(hdc);
        if (solid_alpha_dev_ctx_handle)
        {
            BITMAPINFO bitmap_info = {};
            bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmap_info.bmiHeader.biWidth = 1;
            bitmap_info.bmiHeader.biHeight = -1;
            bitmap_info.bmiHeader.biPlanes = 1;
            bitmap_info.bmiHeader.biBitCount = 32;
            bitmap_info.bmiHeader.biCompression = BI_RGB;
            HBITMAP bitmap = ::CreateDIBSection(solid_alpha_dev_ctx_handle, &bitmap_info, DIB_RGB_COLORS, (void**)&solid_alpha_pixel, nullptr, 0);
            if (bitmap)
            {
                ::DeleteObject(::SelectObject(solid_alpha_dev_ctx_handle, bitmap));
                ::DeleteObject(bitmap);
            }
            else
            {
                ::DeleteDC(solid_alpha_dev_ctx_handle);
                solid_alpha_dev_ctx_handle = nullptr;
                solid_alpha_pixel = nullptr;
            }
        }
    }
    IM_ASSERT(solid_alpha_dev_ctx_handle != nullptr && solid_alpha_pixel != nullptr && "Failed to create compatible DC and DIB section for solid rectangle alpha blending!");

    ImU32* pixel_buffer = nullptr;
    {
        BITMAPINFO bitmap_info = {};
        bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap_info.bmiHeader.biWidth = fb_width;
        bitmap_info.bmiHeader.biHeight = -fb_height;
        bitmap_info.bmiHeader.biPlanes = 1;
        bitmap_info.bmiHeader.biBitCount = 32;
        bitmap_info.bmiHeader.biCompression = BI_RGB;
        HBITMAP bitmap = ::CreateDIBSection(hdc, &bitmap_info, DIB_RGB_COLORS, (void**)&pixel_buffer, nullptr, 0);
        if (bitmap)
        {
            ::DeleteObject(::SelectObject(hdc, bitmap));
            ::DeleteObject(bitmap);
        }
    }
    IM_ASSERT(pixel_buffer != nullptr && "Failed to create DIB section for rendering!");

    // Clear the framebuffer with the background color (if any)
    if (clear_color)
    {
        COLORREF previous_color = ::SetDCBrushColor(hdc, RGB((BYTE)(clear_color->x * 255.0f), (BYTE)(clear_color->y * 255.0f), (BYTE)(clear_color->z * 255.0f)));
        RECT rect = { 0, 0, fb_width, fb_height };
        ::FillRect(hdc, &rect, (HBRUSH)::GetStockObject(DC_BRUSH));
        ::SetDCBrushColor(hdc, previous_color);
    }

    // Will project scissor/clipping rectangles into framebuffer space

    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (const ImDrawList* draw_list : draw_data->CmdLists)
    {
        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                pcmd->UserCallback(draw_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space

                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // - Apply scissor/clipping rectangle
                // - Bind texture, Draw

                const ImDrawVert* vtx_buffer = draw_list->VtxBuffer.Data + pcmd->VtxOffset;
                const ImDrawIdx* idx_buffer = draw_list->IdxBuffer.Data + pcmd->IdxOffset;

                for (unsigned int elem_i = 0; elem_i < pcmd->ElemCount;)
                {
                    if (elem_i + 6 <= pcmd->ElemCount)
                    {
                        const ImDrawVert* v1 = &vtx_buffer[idx_buffer[elem_i + 0]];
                        const ImDrawVert* v2 = &vtx_buffer[idx_buffer[elem_i + 1]];
                        const ImDrawVert* v3 = &vtx_buffer[idx_buffer[elem_i + 2]];
                        const ImDrawVert* v4 = &vtx_buffer[idx_buffer[elem_i + 3]];
                        const ImDrawVert* v5 = &vtx_buffer[idx_buffer[elem_i + 4]];
                        const ImDrawVert* v6 = &vtx_buffer[idx_buffer[elem_i + 5]];
                        if (ImGui_ImplGDI_RenderDrawRectangle(hdc, solid_alpha_dev_ctx_handle, solid_alpha_pixel, fb_width, fb_height, pcmd->ClipRect, v1, v2, v3, v4, v5, v6))
                        {
                            elem_i += 6;
                            continue;
                        }
                    }

                    const ImDrawVert* v1 = &vtx_buffer[idx_buffer[elem_i + 0]];
                    const ImDrawVert* v2 = &vtx_buffer[idx_buffer[elem_i + 1]];
                    const ImDrawVert* v3 = &vtx_buffer[idx_buffer[elem_i + 2]];
                    ImGui_ImplGDI_Texture* texture = (ImGui_ImplGDI_Texture*)pcmd->GetTexID();
                    ImGui_ImplGDI_RenderDrawTriangle(pixel_buffer, fb_width, fb_height, pcmd->ClipRect, v1, v2, v3, texture);
                    elem_i += 3;
                }
            }
        }
    }

    // Free temporary resources
    {
        ::DeleteDC(solid_alpha_dev_ctx_handle);
        solid_alpha_dev_ctx_handle = nullptr;
        solid_alpha_pixel = nullptr;
    }
}

#endif // #ifndef IMGUI_DISABLE
