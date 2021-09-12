// dear imgui: Renderer Backend for SDL_Renderer, with Platform Backend SDL 
// (Requires: SDL 2.0.18+)

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

struct ImGui_ImplSDLRenderer_Data
{
    SDL_Renderer *SDLRenderer;
    SDL_Texture  *FontTexture;
    ImGui_ImplSDLRenderer_Data() { memset(this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplSDLRenderer_Data* ImGui_ImplSDLRenderer_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplSDLRenderer_Data*)ImGui::GetIO().BackendRendererUserData : NULL;
}

// Functions
bool ImGui_ImplSDLRenderer_Init(SDL_Renderer *renderer)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");
    IM_ASSERT(renderer != NULL && "SDL_Renderer not initialized!");

    // Setup backend capabilities flags
    ImGui_ImplSDLRenderer_Data* bd = IM_NEW(ImGui_ImplSDLRenderer_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_SDLRenderer";

    bd->SDLRenderer = renderer;
    return true;
}

void ImGui_ImplSDLRenderer_Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDLRenderer_Data* bd = ImGui_ImplSDLRenderer_GetBackendData();

    ImGui_ImplSDLRenderer_DestroyDeviceObjects();

    io.BackendRendererName = NULL;
    io.BackendRendererUserData = NULL;
    IM_DELETE(bd);
}

static void ImGui_ImplSDLRenderer_SetupRenderState()
{
	ImGui_ImplSDLRenderer_Data *bd = ImGui_ImplSDLRenderer_GetBackendData();

	// Clear out any viewports and cliprects set by the user
	SDL_RenderSetViewport(bd->renderer, NULL);
	SDL_RenderSetClipRect(bd->renderer, NULL);
}

void ImGui_ImplSDLRenderer_NewFrame()
{
    ImGui_ImplSDLRenderer_Data* bd = ImGui_ImplSDLRenderer_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplSDLRenderer_Init()?");

    if (!bd->FontTexture) {
        ImGui_ImplSDLRenderer_CreateDeviceObjects();
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
    //  ImGui_ImplSDLRenderer_SetupRenderState(draw_data, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDLRenderer_Data* bd = ImGui_ImplSDLRenderer_GetBackendData();

    ImGui_ImplSDLRenderer_SetupRenderState();

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
                    ImGui_ImplSDLRenderer_SetupRenderState();
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

                    SDL_RenderSetClipRect(bd->SDLRenderer, &r);


                    int xy_stride = sizeof(ImDrawVert);
                    float *xy = (float *)((char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos));

                    int uv_stride = sizeof(ImDrawVert);
                    float *uv = (float*)((char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv));

                    int col_stride = sizeof(ImDrawVert);
                    int *color = (int*)((char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col));

		    SDL_Texture *tex = (SDL_Texture*)pcmd->TextureID;

                    SDL_RenderGeometryRaw(bd->SDLRenderer, tex, 
                            xy, xy_stride, color,
                            col_stride, 
                            uv, uv_stride, 
                            cmd_list->VtxBuffer.Size,
                            idx_buffer, pcmd->ElemCount, sizeof (ImDrawIdx));

                }
            }
            idx_buffer += pcmd->ElemCount;
        }
    }
}

// Called by Init/NewFrame/Shutdown
bool ImGui_ImplSDLRenderer_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDLRenderer_Data* bd = ImGui_ImplSDLRenderer_GetBackendData();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.
    bd->FontTexture = SDL_CreateTexture(bd->SDLRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
    if (bd->FontTexture == NULL) {
        SDL_Log("error creating texture");
        return false;
    }
    SDL_UpdateTexture(bd->FontTexture, NULL, pixels, 4 * width);
    SDL_SetTextureBlendMode(bd->FontTexture, SDL_BLENDMODE_BLEND);
    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->FontTexture);

    return true;
}

void ImGui_ImplSDLRenderer_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDLRenderer_Data* bd = ImGui_ImplSDLRenderer_GetBackendData();
    if (bd->FontTexture) {
        io.Fonts->SetTexID(0);
        SDL_DestroyTexture(bd->FontTexture);
        bd->FontTexture = NULL;
    }
}

bool ImGui_ImplSDLRenderer_CreateDeviceObjects()
{
    return ImGui_ImplSDLRenderer_CreateFontsTexture();
}

void ImGui_ImplSDLRenderer_DestroyDeviceObjects()
{
    ImGui_ImplSDLRenderer_DestroyFontsTexture();
}

