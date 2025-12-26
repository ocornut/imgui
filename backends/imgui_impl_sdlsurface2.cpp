// imgui_impl_sdlsurface2.cpp
// CPU-only SDL_Surface backend for Dear ImGui

#include "imgui_impl_sdlsurface2.h"
#include "imgui.h"
#include <SDL.h>
#include <cstring>
#include <algorithm>

static SDL_Surface* g_TargetSurface = nullptr;
static SDL_Surface* g_FontSurface = nullptr;

static inline Uint32 GetPixel(SDL_Surface* s, int x, int y)
{
    Uint8* p = (Uint8*)s->pixels + y * s->pitch + x * s->format->BytesPerPixel;
    switch (s->format->BytesPerPixel)
    {
        case 1: return *p;
        case 2: return *(Uint16*)p;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return (p[0] << 16) | (p[1] << 8) | p[2];
            else
                return p[0] | (p[1] << 8) | (p[2] << 16);
        case 4: return *(Uint32*)p;
    }
    return 0;
}

static inline void BlendPixel(SDL_Surface* s, int x, int y,
                              Uint8 sr, Uint8 sg, Uint8 sb, Uint8 sa)
{
    if (!s || sa == 0) return;
    if (x < 0 || y < 0 || x >= s->w || y >= s->h) return;

    if (sa == 255)
    {
        Uint32 out_pix = SDL_MapRGBA(s->format, sr, sg, sb, sa);
        Uint8* p = (Uint8*)s->pixels + y * s->pitch + x * s->format->BytesPerPixel;
        switch (s->format->BytesPerPixel)
        {
            case 1: *p = (Uint8)out_pix; break;
            case 2: *(Uint16*)p = (Uint16)out_pix; break;
            case 3:
                if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                {
                    p[0] = (out_pix >> 16) & 0xFF;
                    p[1] = (out_pix >> 8)  & 0xFF;
                    p[2] =  out_pix        & 0xFF;
                }
                else
                {
                    p[0] =  out_pix        & 0xFF;
                    p[1] = (out_pix >> 8)  & 0xFF;
                    p[2] = (out_pix >> 16) & 0xFF;
                }
                break;
            case 4: *(Uint32*)p = out_pix; break;
        }
        return;
    }

    Uint32 dst_pix = GetPixel(s, x, y);
    Uint8 dr, dg, db, da;
    SDL_GetRGBA(dst_pix, s->format, &dr, &dg, &db, &da);

    Uint8 out_r = (Uint8)((sr * sa + dr * (255 - sa)) / 255);
    Uint8 out_g = (Uint8)((sg * sa + dg * (255 - sa)) / 255);
    Uint8 out_b = (Uint8)((sb * sa + db * (255 - sa)) / 255);
    Uint8 out_a = (Uint8)((sa + (da * (255 - sa)) / 255));

    Uint32 out_pix = SDL_MapRGBA(s->format, out_r, out_g, out_b, out_a);
    Uint8* p = (Uint8*)s->pixels + y * s->pitch + x * s->format->BytesPerPixel;
    switch (s->format->BytesPerPixel)
    {
        case 1: *p = (Uint8)out_pix; break;
        case 2: *(Uint16*)p = (Uint16)out_pix; break;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            {
                p[0] = (out_pix >> 16) & 0xFF;
                p[1] = (out_pix >> 8)  & 0xFF;
                p[2] =  out_pix        & 0xFF;
            }
            else
            {
                p[0] =  out_pix        & 0xFF;
                p[1] = (out_pix >> 8)  & 0xFF;
                p[2] = (out_pix >> 16) & 0xFF;
            }
            break;
        case 4: *(Uint32*)p = out_pix; break;
    }
}

static inline float Edge(const ImVec2& a, const ImVec2& b, float x, float y)
{
    return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
}

SDL_Surface* ImGui_ImplSDLSurface2_CreateFontAtlasSurface()
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    if (!pixels || width <= 0 || height <= 0) return nullptr;

    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) return nullptr;

    SDL_LockSurface(surf);
    std::memcpy(surf->pixels, pixels, width * height * 4);
    SDL_UnlockSurface(surf);
    SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);
    return surf;
}

bool ImGui_ImplSDLSurface2_Init(SDL_Surface* surface)
{
    if (!surface) return false;

    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererName == nullptr && "Already initialized a renderer backend!");
    io.BackendRendererName = "imgui_impl_sdlsurface2";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures; 

    g_TargetSurface = surface;
    g_FontSurface = ImGui_ImplSDLSurface2_CreateFontAtlasSurface();
    if (g_FontSurface)
    {
        if (g_TargetSurface && g_FontSurface->format->format != g_TargetSurface->format->format)
        {
            SDL_Surface* converted = SDL_ConvertSurfaceFormat(g_FontSurface, g_TargetSurface->format->format, 0);
            if (converted)
            {
                SDL_FreeSurface(g_FontSurface);
                g_FontSurface = converted;
            }
        }
        ImGui::GetIO().Fonts->TexID = (ImTextureID)g_FontSurface;
    }
    return true;
}

void ImGui_ImplSDLSurface2_Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasTextures;

    ImGui::GetIO().Fonts->TexID = nullptr;
    if (g_FontSurface)
    {
        SDL_FreeSurface(g_FontSurface);
        g_FontSurface = nullptr;
    }
    g_TargetSurface = nullptr;
}

void ImGui_ImplSDLSurface2_NewFrame()
{
    // noop
}

void ImGui_ImplSDLSurface2_RenderDrawData(ImDrawData* draw_data)
{
    if (!draw_data || !g_TargetSurface) return;

    if (SDL_MUSTLOCK(g_TargetSurface)) SDL_LockSurface(g_TargetSurface);

    const ImVec2 display_pos = draw_data->DisplayPos;
    const ImVec2 fb_scale = draw_data->FramebufferScale;

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

        int idx_offset = 0;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->ElemCount == 0)
                continue;

            ImVec2 clip_min = ImVec2(
                (pcmd->ClipRect.x - display_pos.x) * fb_scale.x,
                (pcmd->ClipRect.y - display_pos.y) * fb_scale.y);
            ImVec2 clip_max = ImVec2(
                (pcmd->ClipRect.z - display_pos.x) * fb_scale.x,
                (pcmd->ClipRect.w - display_pos.y) * fb_scale.y);

            int cx0 = (int)std::floor(clip_min.x);
            int cy0 = (int)std::floor(clip_min.y);
            int cx1 = (int)std::ceil (clip_max.x);
            int cy1 = (int)std::ceil (clip_max.y);

            cx0 = std::max(cx0, 0);
            cy0 = std::max(cy0, 0);
            cx1 = std::min(cx1, g_TargetSurface->w);
            cy1 = std::min(cy1, g_TargetSurface->h);

            if (cx0 >= cx1 || cy0 >= cy1)
            {
                idx_offset += pcmd->ElemCount;
                continue;
            }

            SDL_Rect srect{ cx0, cy0, cx1 - cx0, cy1 - cy0 };
            SDL_SetClipRect(g_TargetSurface, &srect);

           SDL_Surface* tex = (SDL_Surface*)pcmd->GetTexID();

            for (unsigned int i = 0; i + 2 < (unsigned int)pcmd->ElemCount; i += 3)
            {
                ImDrawIdx i0 = idx_buffer[idx_offset + i + 0];
                ImDrawIdx i1 = idx_buffer[idx_offset + i + 1];
                ImDrawIdx i2 = idx_buffer[idx_offset + i + 2];

                const ImDrawVert& v0 = vtx_buffer[i0];
                const ImDrawVert& v1 = vtx_buffer[i1];
                const ImDrawVert& v2 = vtx_buffer[i2];

                ImVec2 p0 = ImVec2((v0.pos.x - display_pos.x) * fb_scale.x, (v0.pos.y - display_pos.y) * fb_scale.y);
                ImVec2 p1 = ImVec2((v1.pos.x - display_pos.x) * fb_scale.x, (v1.pos.y - display_pos.y) * fb_scale.y);
                ImVec2 p2 = ImVec2((v2.pos.x - display_pos.x) * fb_scale.x, (v2.pos.y - display_pos.y) * fb_scale.y);

                int minx = (int)std::floor(std::min({ p0.x, p1.x, p2.x }));
                int miny = (int)std::floor(std::min({ p0.y, p1.y, p2.y }));
                int maxx = (int)std::ceil (std::max({ p0.x, p1.x, p2.x }));
                int maxy = (int)std::ceil (std::max({ p0.y, p1.y, p2.y }));

                minx = std::max(minx, cx0);
                miny = std::max(miny, cy0);
                maxx = std::min(maxx, cx1 - 1);
                maxy = std::min(maxy, cy1 - 1);

                if (minx > maxx || miny > maxy) continue;

                float area = Edge(p0, p1, p2.x, p2.y);
                if (area == 0.0f) continue;

                auto unpack_col = [](ImU32 c, float out[4]) {
                    out[0] = (float)((c >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
                    out[1] = (float)((c >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
                    out[2] = (float)((c >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
                    out[3] = (float)((c >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
                };

                float c0[4], c1[4], c2[4];
                unpack_col(v0.col, c0); unpack_col(v1.col, c1); unpack_col(v2.col, c2);

                for (int y = miny; y <= maxy; y++)
                {
                    for (int x = minx; x <= maxx; x++)
                    {
                        float px = (float)x + 0.5f;
                        float py = (float)y + 0.5f;

                        float w0 = Edge(p1, p2, px, py) / area;
                        float w1 = Edge(p2, p0, px, py) / area;
                        float w2 = Edge(p0, p1, px, py) / area;

                        if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;

                        float r = w0 * c0[0] + w1 * c1[0] + w2 * c2[0];
                        float g = w0 * c0[1] + w1 * c1[1] + w2 * c2[1];
                        float b = w0 * c0[2] + w1 * c1[2] + w2 * c2[2];
                        float a = w0 * c0[3] + w1 * c1[3] + w2 * c2[3];

                        Uint8 out_r = (Uint8)(std::clamp(r, 0.0f, 1.0f) * 255.0f);
                        Uint8 out_g = (Uint8)(std::clamp(g, 0.0f, 1.0f) * 255.0f);
                        Uint8 out_b = (Uint8)(std::clamp(b, 0.0f, 1.0f) * 255.0f);
                        Uint8 out_a = (Uint8)(std::clamp(a, 0.0f, 1.0f) * 255.0f);

                        if (tex)
                        {
                            ImVec2 uv0 = v0.uv, uv1 = v1.uv, uv2 = v2.uv;
                            float u = w0 * uv0.x + w1 * uv1.x + w2 * uv2.x;
                            float v = w0 * uv0.y + w1 * uv1.y + w2 * uv2.y;

                            int tx = (int)(u * (tex->w - 1) + 0.5f);
                            int ty = (int)(v * (tex->h - 1) + 0.5f);
                            tx = std::clamp(tx, 0, tex->w - 1);
                            ty = std::clamp(ty, 0, tex->h - 1);

                            Uint32 tpx = GetPixel(tex, tx, ty);
                            Uint8 tr, tg, tb, ta;
                            SDL_GetRGBA(tpx, tex->format, &tr, &tg, &tb, &ta);

                            Uint8 final_r = (Uint8)((tr * out_r) / 255);
                            Uint8 final_g = (Uint8)((tg * out_g) / 255);
                            Uint8 final_b = (Uint8)((tb * out_b) / 255);
                            Uint8 final_a = (Uint8)((ta * out_a) / 255);

                            BlendPixel(g_TargetSurface, x, y, final_r, final_g, final_b, final_a);
                        }
                        else
                        {
                            BlendPixel(g_TargetSurface, x, y, out_r, out_g, out_b, out_a);
                        }
                    }
                }
            }

            idx_offset += pcmd->ElemCount;
        }
    }

    SDL_SetClipRect(g_TargetSurface, nullptr);
    if (SDL_MUSTLOCK(g_TargetSurface)) SDL_UnlockSurface(g_TargetSurface);
}
