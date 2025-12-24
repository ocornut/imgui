// imgui_impl_sdlsurface2.cpp
// CPU-only SDL_Surface backend for Dear ImGui

#include "imgui_impl_sdlsurface2.h"
#include "imgui.h"
#include <SDL.h>
#include <cstring>
#include <algorithm>

static SDL_Surface* g_TargetSurface = nullptr;
static SDL_Surface* g_FontSurface = nullptr;

static inline void PutPixel(SDL_Surface* surface, int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) return;
    Uint32 pixel = SDL_MapRGBA(surface->format, r, g, b, a);
    Uint8* pixels = (Uint8*)surface->pixels;
    int pitch = surface->pitch;
    Uint32* dst = (Uint32*)(pixels + y * pitch + x * 4);
    *dst = pixel;
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
    memcpy(surf->pixels, pixels, width * height * 4);
    SDL_UnlockSurface(surf);
    return surf;
}

bool ImGui_ImplSDLSurface2_Init(SDL_Surface* surface)
{
    if (!surface) return false;
    g_TargetSurface = surface;

    g_FontSurface = ImGui_ImplSDLSurface2_CreateFontAtlasSurface();
    if (g_FontSurface)
    {
        ImGui::GetIO().Fonts->TexID = (ImTextureID)g_FontSurface;
    }
    return true;
}

void ImGui_ImplSDLSurface2_Shutdown()
{
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
            if (pcmd->ElemCount == 0) { idx_offset += 0; continue; }

            ImVec2 clip_min = ImVec2((pcmd->ClipRect.x - display_pos.x) * fb_scale.x,
                                     (pcmd->ClipRect.y - display_pos.y) * fb_scale.y);
            ImVec2 clip_max = ImVec2((pcmd->ClipRect.z - display_pos.x) * fb_scale.x,
                                     (pcmd->ClipRect.w - display_pos.y) * fb_scale.y);

            int cx0 = (int)floor(clip_min.x);
            int cy0 = (int)floor(clip_min.y);
            int cx1 = (int)ceil(clip_max.x);
            int cy1 = (int)ceil(clip_max.y);

            cx0 = std::max(cx0, 0);
            cy0 = std::max(cy0, 0);
            cx1 = std::min(cx1, g_TargetSurface->w);
            cy1 = std::min(cy1, g_TargetSurface->h);

            if (cx0 >= cx1 || cy0 >= cy1)
            {
                idx_offset += pcmd->ElemCount;
                continue;
            }

            SDL_Rect srect;
            srect.x = cx0;
            srect.y = cy0;
            srect.w = cx1 - cx0;
            srect.h = cy1 - cy0;
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

                int minx = (int)floor(std::min(std::min(p0.x, p1.x), p2.x));
                int miny = (int)floor(std::min(std::min(p0.y, p1.y), p2.y));
                int maxx = (int)ceil(std::max(std::max(p0.x, p1.x), p2.x));
                int maxy = (int)ceil(std::max(std::max(p0.y, p1.y), p2.y));

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

                        Uint8 out_r = (Uint8)(std::min(1.0f, r) * 255.0f);
                        Uint8 out_g = (Uint8)(std::min(1.0f, g) * 255.0f);
                        Uint8 out_b = (Uint8)(std::min(1.0f, b) * 255.0f);
                        Uint8 out_a = (Uint8)(std::min(1.0f, a) * 255.0f);

                        if (tex)
                        {
                            ImVec2 uv0 = v0.uv; ImVec2 uv1 = v1.uv; ImVec2 uv2 = v2.uv;
                            float u = w0 * uv0.x + w1 * uv1.x + w2 * uv2.x;
                            float v = w0 * uv0.y + w1 * uv1.y + w2 * uv2.y;
                            int tx = (int)(u * (tex->w - 1) + 0.5f);
                            int ty = (int)(v * (tex->h - 1) + 0.5f);
                            tx = std::clamp(tx, 0, tex->w - 1);
                            ty = std::clamp(ty, 0, tex->h - 1);

                            Uint8* tpx = (Uint8*)tex->pixels + ty * tex->pitch + tx * 4;
                            Uint8 tr = tpx[0];
                            Uint8 tg = tpx[1];
                            Uint8 tb = tpx[2];
                            Uint8 ta = tpx[3];

                            Uint8 final_r = (Uint8)((tr * out_r) / 255);
                            Uint8 final_g = (Uint8)((tg * out_g) / 255);
                            Uint8 final_b = (Uint8)((tb * out_b) / 255);
                            Uint8 final_a = (Uint8)((ta * out_a) / 255);

                            PutPixel(g_TargetSurface, x, y, final_r, final_g, final_b, final_a);
                        }
                        else
                        {
                            PutPixel(g_TargetSurface, x, y, out_r, out_g, out_b, out_a);
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
