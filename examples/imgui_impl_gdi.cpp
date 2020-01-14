// dear imgui: Renderer for GDI (Based on emilk's imgui_software_renderer.)
// This needs to be used along with a Platform Binding (e.g. Win32)

// Implemented features:
//  [X] Renderer: Support for large meshes (64k+ vertices) with 16-bits indices.
// Issues:
//  [ ] You can't use User texture binding because of the imgui_software_renderer don't support that.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp
// https://github.com/ocornut/imgui

// CHANGELOG
//  2020-01-14: GDI: Support partial acceleration with Windows GDI.
//  2019-08-12: GDI: Improve the implementation of the GDI renderer.
//  2019-08-12: GDI: Add Windows GDI Renderer Support.

#include "imgui.h"
#include "imgui_impl_gdi.h"

#define NOMINMAX
#include <Windows.h>

#include <vector>

// ****************************************************************************

static int old_fb_width = 0;
static int old_fb_height = 0;

static HDC g_hDC = nullptr;
static HDC g_hBufferDC = nullptr;
static HBITMAP g_hBitmap = nullptr;
static uint32_t* g_PixelBuffer = nullptr;
static size_t g_PixelBufferSize = 0;
static HBRUSH g_BackgroundColorBrush = nullptr;

std::vector<TRIVERTEX> g_VertexBuffer;
std::vector<GRADIENT_TRIANGLE> g_MeshBuffer;

const uint16_t FPS = 60;

const uint16_t g_ShouldRenderTime = (1000 / FPS) - 5;
static uint16_t g_RenderTime = 0;

// ****************************************************************************

// By Emil Ernerfeldt 2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// WHAT:
//   This is a software renderer for Dear ImGui.
//   It is decently fast, but has a lot of room for optimization.
//   The goal was to get something fast and decently accurate in not too many lines of code.
// LIMITATIONS:
//   * It is not pixel-perfect, but it is good enough for must use cases.
//   * It does not support painting with any other texture than the default font texture.
#pragma once

#include <cstdint>

namespace imgui_sw
{

    /// Optional: tweak ImGui style to make it render faster.
    void make_style_fast();

    /// Undo what make_style_fast did.
    void restore_style();

    /// Call once a the start of your program.
    void bind_imgui_painting();

    /// The buffer is assumed to follow how ImGui packs pixels, i.e. ABGR by default.
    /// Change with IMGUI_USE_BGRA_PACKED_COLOR.
    /// If width/height differs from ImGui::GetIO().DisplaySize then
    /// the function scales the UI to fit the given pixel buffer.
    void paint_imgui(uint32_t* pixels, int width_pixels, int height_pixels);

    /// Free the resources allocated by bind_imgui_painting.
    void unbind_imgui_painting();

    /// Show rendering stats in an ImGui window if you want to.
    void show_stats();

} // namespace imgui_sw

#include <algorithm>
#include <cmath>
#include <vector>

namespace imgui_sw {
    //namespace {

    struct Stats
    {
        int    uniform_triangle_pixels = 0;
        int    textured_triangle_pixels = 0;
        int    gradient_triangle_pixels = 0;
        int    font_pixels = 0;
        double uniform_rectangle_pixels = 0;
        double textured_rectangle_pixels = 0;
        double gradient_rectangle_pixels = 0;
        double gradient_textured_rectangle_pixels = 0;
    };

    struct Texture
    {
        const uint8_t* pixels; // 8-bit.
        int            width;
        int            height;
    };

    struct PaintTarget
    {
        uint32_t* pixels;
        int       width;
        int       height;
        ImVec2    scale; // Multiply ImGui (point) coordinates with this to get pixel coordinates.
    };

    // ----------------------------------------------------------------------------

    struct ColorInt
    {
        uint32_t a, b, g, r;

        ColorInt() = default;

        explicit ColorInt(uint32_t x)
        {
            a = (x >> IM_COL32_A_SHIFT) & 0xFFu;
            b = (x >> IM_COL32_B_SHIFT) & 0xFFu;
            g = (x >> IM_COL32_G_SHIFT) & 0xFFu;
            r = (x >> IM_COL32_R_SHIFT) & 0xFFu;
        }

        uint32_t toUint32() const
        {
            // return (a << 24u) | (b << 16u) | (g << 8u) | r;
            return (a << 24u) | (r << 16u) | (g << 8u) | b;
        }
    };

    ColorInt blend(ColorInt target, ColorInt source)
    {
        ColorInt result;
        result.a = 0; // Whatever.
        result.b = (source.b * source.a + target.b * (255 - source.a)) / 255;
        result.g = (source.g * source.a + target.g * (255 - source.a)) / 255;
        result.r = (source.r * source.a + target.r * (255 - source.a)) / 255;
        return result;
    }

    // ----------------------------------------------------------------------------
    // Used for interpolating vertex attributes (color and texture coordinates) in a triangle.

    struct Barycentric
    {
        float w0, w1, w2;
    };

    Barycentric operator*(const float f, const Barycentric& va)
    {
        return { f * va.w0, f * va.w1, f * va.w2 };
    }

    void operator+=(Barycentric& a, const Barycentric& b)
    {
        a.w0 += b.w0;
        a.w1 += b.w1;
        a.w2 += b.w2;
    }

    Barycentric operator+(const Barycentric& a, const Barycentric& b)
    {
        return Barycentric{ a.w0 + b.w0, a.w1 + b.w1, a.w2 + b.w2 };
    }

    // ----------------------------------------------------------------------------
    // Useful operators on ImGui vectors:

    ImVec2 operator*(const float f, const ImVec2& v)
    {
        return ImVec2{ f * v.x, f * v.y };
    }

    ImVec2 operator+(const ImVec2& a, const ImVec2& b)
    {
        return ImVec2{ a.x + b.x, a.y + b.y };
    }

    ImVec2 operator-(const ImVec2& a, const ImVec2& b)
    {
        return ImVec2{ a.x - b.x, a.y - b.y };
    }

    bool operator!=(const ImVec2& a, const ImVec2& b)
    {
        return a.x != b.x || a.y != b.y;
    }

    ImVec4 operator*(const float f, const ImVec4& v)
    {
        return ImVec4{ f * v.x, f * v.y, f * v.z, f * v.w };
    }

    ImVec4 operator+(const ImVec4& a, const ImVec4& b)
    {
        return ImVec4{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    }

    // ----------------------------------------------------------------------------
    // Copies of functions in ImGui, inlined for speed:

    ImVec4 color_convert_u32_to_float4(ImU32 in)
    {
        const float s = 1.0f / 255.0f;
        return ImVec4(
            ((in >> IM_COL32_R_SHIFT) & 0xFF) * s,
            ((in >> IM_COL32_G_SHIFT) & 0xFF) * s,
            ((in >> IM_COL32_B_SHIFT) & 0xFF) * s,
            ((in >> IM_COL32_A_SHIFT) & 0xFF) * s);
    }

    ImU32 color_convert_float4_to_u32(const ImVec4& in)
    {
        ImU32 out;
        out = uint32_t(in.x * 255.0f + 0.5f) << IM_COL32_R_SHIFT;
        out |= uint32_t(in.y * 255.0f + 0.5f) << IM_COL32_G_SHIFT;
        out |= uint32_t(in.z * 255.0f + 0.5f) << IM_COL32_B_SHIFT;
        out |= uint32_t(in.w * 255.0f + 0.5f) << IM_COL32_A_SHIFT;
        return out;
    }

    // ----------------------------------------------------------------------------
    // For fast and subpixel-perfect triangle rendering we used fixed point arithmetic.
    // To keep the code simple we use 64 bits to avoid overflows.

    using Int = int64_t;
    const Int kFixedBias = 256;

    struct Point
    {
        Int x, y;
    };

    Int orient2d(const Point& a, const Point& b, const Point& c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    Int as_int(float v)
    {
        return static_cast<Int>(std::floor(v * kFixedBias));
    }

    Point as_point(ImVec2 v)
    {
        return Point{ as_int(v.x), as_int(v.y) };
    }

    // ----------------------------------------------------------------------------

    float min3(float a, float b, float c)
    {
        if (a < b&& a < c) { return a; }
        return b < c ? b : c;
    }

    float max3(float a, float b, float c)
    {
        if (a > b&& a > c) { return a; }
        return b > c ? b : c;
    }

    float barycentric(const ImVec2& a, const ImVec2& b, const ImVec2& point)
    {
        return (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
    }

    inline uint8_t sample_texture(const Texture& texture, const ImVec2& uv)
    {
        int tx = static_cast<int>(uv.x * (texture.width - 1.0f) + 0.5f);
        int ty = static_cast<int>(uv.y * (texture.height - 1.0f) + 0.5f);

        // Clamp to inside of texture:
        tx = std::max(tx, 0);
        tx = std::min(tx, texture.width - 1);
        ty = std::max(ty, 0);
        ty = std::min(ty, texture.height - 1);

        return texture.pixels[ty * texture.width + tx];
    }

    // When two triangles share an edge, we want to draw the pixels on that edge exactly once.
    // The edge will be the same, but the direction will be the opposite
    // (assuming the two triangles have the same winding order).
    // Which edge wins? This functions decides.
    bool is_dominant_edge(ImVec2 edge)
    {
        // return edge.x < 0 || (edge.x == 0 && edge.y > 0);
        return edge.y > 0 || (edge.y == 0 && edge.x < 0);
    }

    // Handles triangles in any winding order (CW/CCW)
    void paint_triangle(
        const PaintTarget& target,
        const Texture* texture,
        const ImVec4& clip_rect,
        const ImDrawVert& v0,
        const ImDrawVert& v1,
        const ImDrawVert& v2,
        Stats* stats)
    {
        const ImVec2 p0 = ImVec2(target.scale.x * v0.pos.x, target.scale.y * v0.pos.y);
        const ImVec2 p1 = ImVec2(target.scale.x * v1.pos.x, target.scale.y * v1.pos.y);
        const ImVec2 p2 = ImVec2(target.scale.x * v2.pos.x, target.scale.y * v2.pos.y);

        const auto rect_area = barycentric(p0, p1, p2); // Can be positive or negative depending on winding order
        if (rect_area == 0.0f) { return; }
        // if (rect_area < 0.0f) { return paint_triangle(target, texture, clip_rect, v0, v2, v1, stats); }

        // Find bounding box:
        float min_x_f = min3(p0.x, p1.x, p2.x);
        float min_y_f = min3(p0.y, p1.y, p2.y);
        float max_x_f = max3(p0.x, p1.x, p2.x);
        float max_y_f = max3(p0.y, p1.y, p2.y);

        // Clip against clip_rect:
        min_x_f = std::max(min_x_f, target.scale.x * clip_rect.x);
        min_y_f = std::max(min_y_f, target.scale.y * clip_rect.y);
        max_x_f = std::min(max_x_f, target.scale.x * clip_rect.z - 0.5f);
        max_y_f = std::min(max_y_f, target.scale.y * clip_rect.w - 0.5f);

        // Integer bounding box [min, max):
        int min_x_i = static_cast<int>(min_x_f);
        int min_y_i = static_cast<int>(min_y_f);
        int max_x_i = static_cast<int>(max_x_f + 1.0f);
        int max_y_i = static_cast<int>(max_y_f + 1.0f);

        // Clip against render target:
        min_x_i = std::max(min_x_i, 0);
        min_y_i = std::max(min_y_i, 0);
        max_x_i = std::min(max_x_i, target.width);
        max_y_i = std::min(max_y_i, target.height);

        // ------------------------------------------------------------------------
        // Set up interpolation of barycentric coordinates:

        const auto topleft = ImVec2(min_x_i + 0.5f * target.scale.x,
            min_y_i + 0.5f * target.scale.y);
        const auto dx = ImVec2(1, 0);
        const auto dy = ImVec2(0, 1);

        const auto w0_topleft = barycentric(p1, p2, topleft);
        const auto w1_topleft = barycentric(p2, p0, topleft);
        const auto w2_topleft = barycentric(p0, p1, topleft);

        const auto w0_dx = barycentric(p1, p2, topleft + dx) - w0_topleft;
        const auto w1_dx = barycentric(p2, p0, topleft + dx) - w1_topleft;
        const auto w2_dx = barycentric(p0, p1, topleft + dx) - w2_topleft;

        const auto w0_dy = barycentric(p1, p2, topleft + dy) - w0_topleft;
        const auto w1_dy = barycentric(p2, p0, topleft + dy) - w1_topleft;
        const auto w2_dy = barycentric(p0, p1, topleft + dy) - w2_topleft;

        const Barycentric bary_0{ 1, 0, 0 };
        const Barycentric bary_1{ 0, 1, 0 };
        const Barycentric bary_2{ 0, 0, 1 };

        const auto inv_area = 1 / rect_area;
        const Barycentric bary_topleft = inv_area * (w0_topleft * bary_0 + w1_topleft * bary_1 + w2_topleft * bary_2);
        const Barycentric bary_dx = inv_area * (w0_dx * bary_0 + w1_dx * bary_1 + w2_dx * bary_2);
        const Barycentric bary_dy = inv_area * (w0_dy * bary_0 + w1_dy * bary_1 + w2_dy * bary_2);

        Barycentric bary_current_row = bary_topleft;

        // ------------------------------------------------------------------------
        // For pixel-perfect inside/outside testing:

        const int sign = rect_area > 0 ? 1 : -1; // winding order?

        const int bias0i = is_dominant_edge(p2 - p1) ? 0 : -1;
        const int bias1i = is_dominant_edge(p0 - p2) ? 0 : -1;
        const int bias2i = is_dominant_edge(p1 - p0) ? 0 : -1;

        const auto p0i = as_point(p0);
        const auto p1i = as_point(p1);
        const auto p2i = as_point(p2);

        // ------------------------------------------------------------------------

        const bool has_uniform_color = (v0.col == v1.col && v0.col == v2.col);

        const ImVec4 c0 = color_convert_u32_to_float4(v0.col);
        const ImVec4 c1 = color_convert_u32_to_float4(v1.col);
        const ImVec4 c2 = color_convert_u32_to_float4(v2.col);;

        // We often blend the same colors over and over again, so optimize for this (saves 10% total cpu):
        uint32_t last_target_pixel = 0;
        uint32_t last_output = blend(ColorInt(last_target_pixel), ColorInt(v0.col)).toUint32();

        for (int y = min_y_i; y < max_y_i; ++y) {
            auto bary = bary_current_row;

            bool has_been_inside_this_row = false;

            for (int x = min_x_i; x < max_x_i; ++x) {
                const auto w0 = bary.w0;
                const auto w1 = bary.w1;
                const auto w2 = bary.w2;
                bary += bary_dx;

                {
                    // Inside/outside test:
                    const auto p = Point{ kFixedBias * x + kFixedBias / 2, kFixedBias * y + kFixedBias / 2 };
                    const auto w0i = sign * orient2d(p1i, p2i, p) + bias0i;
                    const auto w1i = sign * orient2d(p2i, p0i, p) + bias1i;
                    const auto w2i = sign * orient2d(p0i, p1i, p) + bias2i;
                    if (w0i < 0 || w1i < 0 || w2i < 0) {
                        if (has_been_inside_this_row) {
                            break; // Gives a nice 10% speedup
                        }
                        else {
                            continue;
                        }
                    }
                }
                has_been_inside_this_row = true;

                uint32_t& target_pixel = target.pixels[y * target.width + x];

                if (has_uniform_color && !texture) {
                    stats->uniform_triangle_pixels += 1;
                    if (target_pixel == last_target_pixel) {
                        target_pixel = last_output;
                        continue;
                    }
                    last_target_pixel = target_pixel;
                    target_pixel = blend(ColorInt(target_pixel), ColorInt(v0.col)).toUint32();
                    last_output = target_pixel;
                    continue;
                }

                ImVec4 src_color;

                if (has_uniform_color) {
                    src_color = c0;
                }
                else {
                    stats->gradient_triangle_pixels += 1;
                    src_color = w0 * c0 + w1 * c1 + w2 * c2;
                }

                if (texture) {
                    stats->textured_triangle_pixels += 1;
                    const ImVec2 uv = w0 * v0.uv + w1 * v1.uv + w2 * v2.uv;
                    src_color.w *= sample_texture(*texture, uv) / 255.0f;
                }

                if (src_color.w <= 0.0f) { continue; } // Transparent.
                if (src_color.w >= 1.0f) {
                    // Opaque, no blending needed:
                    target_pixel = color_convert_float4_to_u32(src_color);
                    continue;
                }

                ImVec4 target_color = color_convert_u32_to_float4(target_pixel);
                const auto blended_color = src_color.w * src_color + (1.0f - src_color.w) * target_color;
                target_pixel = color_convert_float4_to_u32(blended_color);
            }

            bary_current_row += bary_dy;
        }
    }

    void pre_paint_draw_cmd(
        const PaintTarget& target,
        const ImDrawVert* vertices,
        const ImDrawIdx* idx_buffer,
        const ImDrawCmd& pcmd,
        Stats* stats,
        uint32_t &count)
    {
        const auto texture = reinterpret_cast<const Texture*>(pcmd.TextureId);
        assert(texture);

        // ImGui uses the first pixel for "white".
        const ImVec2 white_uv = ImVec2(0.5f / texture->width, 0.5f / texture->height);

        for (int i = 0; i + 3 <= pcmd.ElemCount; i += 3)
        {
            const ImDrawIdx& i0 = idx_buffer[i + 0];
            const ImDrawIdx& i1 = idx_buffer[i + 1];
            const ImDrawIdx& i2 = idx_buffer[i + 2];

            const ImDrawVert& v0 = vertices[i0];
            const ImDrawVert& v1 = vertices[i1];
            const ImDrawVert& v2 = vertices[i2];

            const bool has_texture = (v0.uv != white_uv || v1.uv != white_uv || v2.uv != white_uv);

            if (has_texture)
            {
                continue;
            }

            size_t CurrentVertexIndex = g_VertexBuffer.size();
            g_VertexBuffer.resize(CurrentVertexIndex + 3);

            PTRIVERTEX CurrentVertexes = &g_VertexBuffer[CurrentVertexIndex];

            CurrentVertexes[0].x = v0.pos.x;
            CurrentVertexes[0].y = v0.pos.y;
            CurrentVertexes[0].Red = ((uint8_t*)&v0.col)[0] << 8;
            CurrentVertexes[0].Green = ((uint8_t*)&v0.col)[1] << 8;
            CurrentVertexes[0].Blue = ((uint8_t*)&v0.col)[2] << 8;
            CurrentVertexes[0].Alpha = ((uint8_t*)&v0.col)[3] << 8;

            CurrentVertexes[1].x = v1.pos.x;
            CurrentVertexes[1].y = v1.pos.y;
            CurrentVertexes[1].Red = ((uint8_t*)&v1.col)[0] << 8;
            CurrentVertexes[1].Green = ((uint8_t*)&v1.col)[1] << 8;
            CurrentVertexes[1].Blue = ((uint8_t*)&v1.col)[2] << 8;
            CurrentVertexes[1].Alpha = ((uint8_t*)&v1.col)[3] << 8;

            CurrentVertexes[2].x = v2.pos.x;
            CurrentVertexes[2].y = v2.pos.y;
            CurrentVertexes[2].Red = ((uint8_t*)&v2.col)[0] << 8;
            CurrentVertexes[2].Green = ((uint8_t*)&v2.col)[1] << 8;
            CurrentVertexes[2].Blue = ((uint8_t*)&v2.col)[2] << 8;
            CurrentVertexes[2].Alpha = ((uint8_t*)&v2.col)[3] << 8;

            size_t CurrentMeshIndex = g_MeshBuffer.size();
            g_MeshBuffer.resize(CurrentMeshIndex + 1);

            PGRADIENT_TRIANGLE Mesh = &g_MeshBuffer[CurrentMeshIndex];

            Mesh->Vertex1 = count;
            Mesh->Vertex2 = count + 1;
            Mesh->Vertex3 = count + 2;

            count += 3;
        }
    }

    void pre_paint_draw_list(const PaintTarget& target, const ImDrawList* cmd_list, Stats* stats)
    {
        const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer[0];
        const ImDrawVert* vertices = cmd_list->VtxBuffer.Data;

        uint32_t count = 0;

        g_VertexBuffer.clear();
        g_MeshBuffer.clear();

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
        {
            const ImDrawCmd& pcmd = cmd_list->CmdBuffer[cmd_i];
            if (pcmd.UserCallback)
            {
                pcmd.UserCallback(cmd_list, &pcmd);
            }
            else
            {
                pre_paint_draw_cmd(target, vertices, idx_buffer, pcmd, stats, count);
            }
            idx_buffer += pcmd.ElemCount;
        }

        if (count)
        {
            GdiGradientFill(
                g_hBufferDC,
                &g_VertexBuffer[0],
                g_VertexBuffer.size(),
                &g_MeshBuffer[0],
                g_MeshBuffer.size(),
                GRADIENT_FILL_TRIANGLE);
        }
    }

    void paint_draw_cmd(
        const PaintTarget& target,
        const ImDrawVert* vertices,
        const ImDrawIdx* idx_buffer,
        const ImDrawCmd& pcmd,
        Stats* stats)
    {
        const auto texture = reinterpret_cast<const Texture*>(pcmd.TextureId);
        assert(texture);

        // ImGui uses the first pixel for "white".
        const ImVec2 white_uv = ImVec2(0.5f / texture->width, 0.5f / texture->height);

        for (int i = 0; i + 3 <= pcmd.ElemCount; i += 3)
        {
            const ImDrawIdx& i0 = idx_buffer[i + 0];
            const ImDrawIdx& i1 = idx_buffer[i + 1];
            const ImDrawIdx& i2 = idx_buffer[i + 2];

            const ImDrawVert& v0 = vertices[i0];
            const ImDrawVert& v1 = vertices[i1];
            const ImDrawVert& v2 = vertices[i2];

            const bool has_texture = (v0.uv != white_uv || v1.uv != white_uv || v2.uv != white_uv);

            if (!has_texture)
            {
                continue;
            }

            paint_triangle(target, texture, pcmd.ClipRect, v0, v1, v2, stats);
        }
    }

    void paint_draw_list(const PaintTarget& target, const ImDrawList* cmd_list, Stats* stats)
    {
        pre_paint_draw_list(target, cmd_list, stats);

        const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer[0];
        const ImDrawVert* vertices = cmd_list->VtxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
        {
            const ImDrawCmd& pcmd = cmd_list->CmdBuffer[cmd_i];
            if (pcmd.UserCallback)
            {
                pcmd.UserCallback(cmd_list, &pcmd);
            }
            else
            {
                paint_draw_cmd(target, vertices, idx_buffer, pcmd, stats);
            }
            idx_buffer += pcmd.ElemCount;
        }
    }

    //} // namespace

    void make_style_fast()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.AntiAliasedLines = false;
        style.AntiAliasedFill = false;
        style.WindowRounding = 0;
    }

    void restore_style()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        const ImGuiStyle default_style = ImGuiStyle();
        style.AntiAliasedLines = default_style.AntiAliasedLines;
        style.AntiAliasedFill = default_style.AntiAliasedFill;
        style.WindowRounding = default_style.WindowRounding;
    }

    void bind_imgui_painting()
    {
        ImGuiIO& io = ImGui::GetIO();

        // Load default font (embedded in code):
        uint8_t* tex_data;
        int font_width, font_height;
        io.Fonts->GetTexDataAsAlpha8(&tex_data, &font_width, &font_height);
        const auto texture = new Texture{ tex_data, font_width, font_height };
        io.Fonts->TexID = texture;
    }

    static Stats s_stats; // TODO: pass as an argument?

    void paint_imgui(uint32_t* pixels, int width_pixels, int height_pixels)
    {
        const float width_points = ImGui::GetIO().DisplaySize.x;
        const float height_points = ImGui::GetIO().DisplaySize.y;
        const ImVec2 scale{ width_pixels / width_points, height_pixels / height_points };
        PaintTarget target{ pixels, width_pixels, height_pixels, scale };
        const ImDrawData* draw_data = ImGui::GetDrawData();

        s_stats = Stats{};
        for (int i = 0; i < draw_data->CmdListsCount; ++i) {
            paint_draw_list(target, draw_data->CmdLists[i], &s_stats);
        }
    }

    void unbind_imgui_painting()
    {
        ImGuiIO& io = ImGui::GetIO();
        delete reinterpret_cast<Texture*>(io.Fonts->TexID);
        io.Fonts = nullptr;
    }

    void show_stats()
    {
        ImGui::Text("uniform_triangle_pixels:            %7d", s_stats.uniform_triangle_pixels);
        ImGui::Text("textured_triangle_pixels:           %7d", s_stats.textured_triangle_pixels);
        ImGui::Text("gradient_triangle_pixels:           %7d", s_stats.gradient_triangle_pixels);
        ImGui::Text("font_pixels:                        %7d", s_stats.font_pixels);
        ImGui::Text("uniform_rectangle_pixels:           %7.0f", s_stats.uniform_rectangle_pixels);
        ImGui::Text("textured_rectangle_pixels:          %7.0f", s_stats.textured_rectangle_pixels);
        ImGui::Text("gradient_rectangle_pixels:          %7.0f", s_stats.gradient_rectangle_pixels);
        ImGui::Text("gradient_textured_rectangle_pixels: %7.0f", s_stats.gradient_textured_rectangle_pixels);
    }

} // namespace imgui_sw

// ****************************************************************************

/**
 * Retrieves the number of milliseconds that have elapsed since the system was
 * started.
 *
 * @return The number of milliseconds.
 */
ULONGLONG M2GetTickCount()
{
    LARGE_INTEGER Frequency = { 0 }, PerformanceCount = { 0 };

    if (QueryPerformanceFrequency(&Frequency))
    {
        if (QueryPerformanceCounter(&PerformanceCount))
        {
            return (PerformanceCount.QuadPart * 1000 / Frequency.QuadPart);
        }
    }

    return GetTickCount64();
}

// ****************************************************************************

bool ImGui_ImplGDI_Init()
{
    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_gdi";

    // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    return true;
}

void ImGui_ImplGDI_Shutdown()
{
    imgui_sw::unbind_imgui_painting();
}

void ImGui_ImplGDI_NewFrame()
{
    g_RenderTime = M2GetTickCount();

    imgui_sw::bind_imgui_painting();
}

// Render function
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(),
// but you can now call this directly from your main loop)
void ImGui_ImplGDI_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays.
    // (screen coordinates != framebuffer coordinates)
    int fb_width = static_cast<int>(
        draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = static_cast<int>(
        draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;

    if (old_fb_width != fb_width || old_fb_height != fb_height)
    {
        // Get the handle of the current window.
        ImGuiIO& io = ImGui::GetIO();
        HWND hWnd = reinterpret_cast<HWND>(io.ImeWindowHandle);

        if (g_hDC)
        {
            ReleaseDC(hWnd, g_hDC);
            g_hDC = nullptr;
        }

        if (g_hBufferDC)
        {
            DeleteDC(g_hBufferDC);
            g_hBufferDC = nullptr;
        }

        if (g_hBitmap)
        {
            DeleteObject(g_hBitmap);
            g_hBitmap = nullptr;
            g_PixelBuffer = nullptr;
            g_PixelBufferSize = 0;
        }


        g_hDC = GetDC(hWnd);
        if (!g_hDC)
            return;

        g_hBufferDC = CreateCompatibleDC(g_hDC);
        if (!g_hBufferDC)
            return;

        BITMAPINFO BitmapInfo;
        BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        BitmapInfo.bmiHeader.biWidth = fb_width;
        BitmapInfo.bmiHeader.biHeight = -fb_height;
        BitmapInfo.bmiHeader.biPlanes = 1;
        BitmapInfo.bmiHeader.biBitCount = 32;
        BitmapInfo.bmiHeader.biCompression = BI_RGB;

        g_hBitmap = CreateDIBSection(
            g_hBufferDC,
            &BitmapInfo,
            DIB_RGB_COLORS,
            (void**)&g_PixelBuffer,
            NULL,
            0);
        if (!g_hBitmap)
            return;

        g_PixelBufferSize = fb_width * fb_height * sizeof(uint32_t);
    }

    old_fb_width = fb_width;
    old_fb_height = fb_height;

    HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(
        SelectObject(g_hBufferDC, g_hBitmap));

    if (g_BackgroundColorBrush)
    {
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.right = fb_width;
        rc.bottom = fb_height;

        FillRect(g_hBufferDC, &rc, g_BackgroundColorBrush);
    }
    else
    {
        memset(g_PixelBuffer, 0, g_PixelBufferSize);
    }

    imgui_sw::paint_imgui(g_PixelBuffer, fb_width, fb_height);

    BitBlt(
        g_hDC,
        0,
        0,
        fb_width,
        fb_height,
        g_hBufferDC,
        0,
        0,
        SRCCOPY);

    SelectObject(g_hBufferDC, hOldBitmap);

    g_RenderTime = M2GetTickCount() - g_RenderTime;

    //printf("g_RenderTime = %d\n", g_RenderTime);

    int32_t WaitTime = g_ShouldRenderTime - g_RenderTime;
    printf("WaitTime = %d\n", WaitTime);
    if (WaitTime > 0)
    {
        Sleep(WaitTime);
    }
}

void ImGui_ImplGDI_SetBackgroundColor(ImVec4* BackgroundColor)
{
    if (g_BackgroundColorBrush)
    {
        DeleteObject(g_BackgroundColorBrush);
        g_BackgroundColorBrush = nullptr;
    }

    g_BackgroundColorBrush = CreateSolidBrush(RGB(
        BackgroundColor->x * 256.0,
        BackgroundColor->y * 256.0,
        BackgroundColor->z * 256.0));
}
