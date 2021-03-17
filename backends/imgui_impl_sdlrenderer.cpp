// dear imgui: Renderer Backend for SDL_Renderer, with Platform Backend SDL 

// Implemented features:

// You can copy and use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs


// CHANGELOG
//  2021-16-03: Creation 

#include "imgui.h"
#include "imgui_impl_sdlrenderer.h"
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif

// Include OpenGL header (without an OpenGL loader) requires a bit of fiddling
#if defined(_WIN32) && !defined(APIENTRY)
#  define APIENTRY __stdcall                  // It is customary to use APIENTRY for OpenGL function pointer declarations on all platforms.  Additionally, the Windows OpenGL header needs APIENTRY.
#endif
#if defined(_WIN32) && !defined(WINGDIAPI)
#  define WINGDIAPI __declspec(dllimport)     // Some Windows OpenGL headers need this
#endif

SDL_Renderer *g_SDLRenderer = NULL;
SDL_Texture  *g_SDLFontTexture = NULL;

bool ImGui_ImplSDLRenderer_Init(SDL_Window *window)
{
    // Setup backend capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_SDLRenderer";

    g_SDLRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (g_SDLRenderer == NULL) {
        SDL_Log("Error creating SDL renderer");
        return false;
    } else {
        SDL_RendererInfo info;
        SDL_GetRendererInfo(g_SDLRenderer, &info);
        SDL_Log("Current SDL Renderer: %s", info.name);
    }
    return true;
}

void ImGui_ImplSDLRenderer_Shutdown()
{
    SDL_DestroyRenderer(g_SDLRenderer);

    if (SDL_GetError()) {
        SDL_Log("Ending with SDL error: %s", SDL_GetError());
    } else {
        SDL_Log("Ending with no SDL error");
    }
}

void ImGui_ImplSDLRenderer_NewFrame()
{
    if (! g_SDLFontTexture) {
        ImGui_ImplSDLRenderer_CreateFontsTexture();
    }
}

void ImGui_ImplSDLRenderer_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    
    // Setup desired GL state
    //  ImGui_ImplOpenGL2_SetupRenderState(draw_data, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    ImGuiIO& io = ImGui::GetIO();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    SDL_SetRenderDrawColor(g_SDLRenderer, clear_color.x * 255, clear_color.y * 255, clear_color.z * 255 , clear_color.w * 255);
    SDL_RenderClear(g_SDLRenderer);

    for (int n = 0; n < draw_data->CmdListsCount; n++) {

        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
        
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) {
                    SDL_Log("Aie SetupRenderState ?");
                    // ImGui_ImplOpenGL2_SetupRenderState(draw_data, fb_width, fb_height);
                } else { 
                    pcmd->UserCallback(cmd_list, pcmd);
                }
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    SDL_Rect r;
                    r.x = clip_rect.x;
                    r.y = clip_rect.y;
                    r.w = clip_rect.z - clip_rect.x;
                    r.h = clip_rect.w - clip_rect.y;

                    SDL_RenderSetClipRect(g_SDLRenderer, &r);



                    for (unsigned int i = 0; i + 3 <= pcmd->ElemCount; i += 3)
                    {
                        const ImDrawVert& v0 = vtx_buffer[idx_buffer[i + 0]];
                        const ImDrawVert& v1 = vtx_buffer[idx_buffer[i + 1]];
                        const ImDrawVert& v2 = vtx_buffer[idx_buffer[i + 2]];

                        SDL_Vertex tab[3];

                        tab[0].position.x = v0.pos.x;
                        tab[0].position.y = v0.pos.y;
                        tab[0].color.r = (v0.col >> 0) & 0xff;
                        tab[0].color.g = (v0.col >> 8) & 0xff;
                        tab[0].color.b = (v0.col >> 16) & 0xff;
                        tab[0].color.a = (v0.col >> 24) & 0xff;
                        tab[0].tex_coord.x = v0.uv.x;
                        tab[0].tex_coord.y = v0.uv.y;

                        tab[1].position.x = v1.pos.x;
                        tab[1].position.y = v1.pos.y;
                        tab[1].color.r = (v1.col >> 0) & 0xff;
                        tab[1].color.g = (v1.col >> 8) & 0xff;
                        tab[1].color.b = (v1.col >> 16) & 0xff;
                        tab[1].color.a = (v1.col >> 24) & 0xff;
                        tab[1].tex_coord.x = v1.uv.x;
                        tab[1].tex_coord.y = v1.uv.y;

                        tab[2].position.x = v2.pos.x;
                        tab[2].position.y = v2.pos.y;
                        tab[2].color.r = (v2.col >> 0) & 0xff;
                        tab[2].color.g = (v2.col >> 8) & 0xff;
                        tab[2].color.b = (v2.col >> 16) & 0xff;
                        tab[2].color.a = (v2.col >> 24) & 0xff;
                        tab[2].tex_coord.x = v2.uv.x;
                        tab[2].tex_coord.y = v2.uv.y;

                        SDL_Texture *tex = (pcmd->TextureId == io.Fonts->TexID ? g_SDLFontTexture : NULL);
                        SDL_RenderGeometry(g_SDLRenderer, tex, tab, 3, NULL, 0);
                    }
                }
            }
            idx_buffer += pcmd->ElemCount;
        }
    }

    SDL_RenderPresent(g_SDLRenderer);
}

// Called by Init/NewFrame/Shutdown
bool ImGui_ImplSDLRenderer_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.
    g_SDLFontTexture = SDL_CreateTexture(g_SDLRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
    if (g_SDLFontTexture == NULL) {
        SDL_Log("error creating texture");
        return false;
    }
    SDL_UpdateTexture(g_SDLFontTexture, NULL, pixels, 4 * width);
    SDL_SetTextureBlendMode(g_SDLFontTexture, SDL_BLENDMODE_BLEND);
    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)g_SDLFontTexture);

    return true;
}

void ImGui_ImplSDLRenderer_DestroyFontsTexture()
{
    if (g_SDLFontTexture) {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->SetTexID(0);
        SDL_DestroyTexture(g_SDLFontTexture);
        g_SDLFontTexture = NULL;
    }
}

