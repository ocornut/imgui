// dear imgui: Renderer Backend for SDL_Renderer
// (Requires: SDL 2.0.17+)

// Important to understand: SDL_Renderer is an _optional_ component of SDL. We do not recommend you use SDL_Renderer
// because it provide a rather limited API to the end-user. We provide this backend for the sake of completeness.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

// Implemented features:
//  [X] Renderer: User texture binding. Use 'SDL_Texture*' as ImTextureID. Read the FAQ about ImTextureID!
// Missing features:
//  [ ] Renderer: Support for large meshes (64k+ vertices) with 16-bit indices.
//  [ ] Renderer: Multi-viewport support (multiple windows).

// You can copy and use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
//  2021-10-06: Backup and restore modified ClipRect/Viewport.
//  2021-09-21: Initial version.

#include "imgui.h"
#include "imgui_impl_sdlrenderer.h"
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif

// SDL
#include <SDL.h>
#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

// SDL_Renderer data
struct ImGui_ImplSDLRenderer_Data
{
    SDL_Renderer*   SDLRenderer;
    SDL_Texture*    FontTexture;
    ImGui_ImplSDLRenderer_Data() { memset(this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplSDLRenderer_Data* ImGui_ImplSDLRenderer_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplSDLRenderer_Data*)ImGui::GetIO().BackendRendererUserData : NULL;
}

// Functions
bool ImGui_ImplSDLRenderer_Init(SDL_Renderer* renderer)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");
    IM_ASSERT(renderer != NULL && "SDL_Renderer not initialized!");

    // Setup backend capabilities flags
    ImGui_ImplSDLRenderer_Data* bd = IM_NEW(ImGui_ImplSDLRenderer_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_sdlrenderer";

    bd->SDLRenderer = renderer;

    return true;
}

void ImGui_ImplSDLRenderer_Shutdown()
{
    ImGui_ImplSDLRenderer_Data* bd = ImGui_ImplSDLRenderer_GetBackendData();
    IM_ASSERT(bd != NULL && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDLRenderer_DestroyDeviceObjects();

    io.BackendRendererName = NULL;
    io.BackendRendererUserData = NULL;
    IM_DELETE(bd);
}

static void ImGui_ImplSDLRenderer_SetupRenderState()
{
	ImGui_ImplSDLRenderer_Data* bd = ImGui_ImplSDLRenderer_GetBackendData();

	// Clear out any viewports and cliprect set by the user
    // FIXME: Technically speaking there are lots of other things we could backup/setup/restore during our render process.
	SDL_RenderSetViewport(bd->SDLRenderer, NULL);
	SDL_RenderSetClipRect(bd->SDLRenderer, NULL);
}

void ImGui_ImplSDLRenderer_NewFrame()
{
    ImGui_ImplSDLRenderer_Data* bd = ImGui_ImplSDLRenderer_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplSDLRenderer_Init()?");

    if (!bd->FontTexture)
        ImGui_ImplSDLRenderer_CreateDeviceObjects();
}

void ImGui_ImplSDLRenderer_RenderDrawData(ImDrawData* draw_data)
{
	ImGui_ImplSDLRenderer_Data* bd = ImGui_ImplSDLRenderer_GetBackendData();

	// If there's a scale factor set by the user, use that instead
    // If the user has specified a scale factor to SDL_Renderer already via SDL_RenderSetScale(), SDL will scale whatever we pass
    // to SDL_RenderGeometryRaw() by that scale factor. In that case we don't want to be also scaling it ourselves here.
    float rsx = 1.0f;
	float rsy = 1.0f;
	SDL_RenderGetScale(bd->SDLRenderer, &rsx, &rsy);
    ImVec2 render_scale;
	render_scale.x = (rsx == 1.0f) ? draw_data->FramebufferScale.x : 1.0f;
	render_scale.y = (rsy == 1.0f) ? draw_data->FramebufferScale.y : 1.0f;

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fb_width = (int)(draw_data->DisplaySize.x * render_scale.x);
	int fb_height = (int)(draw_data->DisplaySize.y * render_scale.y);
	if (fb_width == 0 || fb_height == 0)
		return;

    // Backup SDL_Renderer state that will be modified to restore it afterwards
    struct BackupSDLRendererState
    {
        SDL_Rect    Viewport;
        bool        ClipEnabled;
        SDL_Rect    ClipRect;
    };
    BackupSDLRendererState old = {};
    old.ClipEnabled = SDL_RenderIsClipEnabled(bd->SDLRenderer);
    SDL_RenderGetViewport(bd->SDLRenderer, &old.Viewport);
    SDL_RenderGetClipRect(bd->SDLRenderer, &old.ClipRect);

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = render_scale;

    // Render command lists
    ImGui_ImplSDLRenderer_SetupRenderState();
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
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
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplSDLRenderer_SetupRenderState();
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
                if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
                if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
                if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
                if (clip_max.x < clip_min.x || clip_max.y < clip_min.y)
                    continue;

                SDL_Rect r = { (int)(clip_min.x), (int)(clip_min.y), (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y) };
                SDL_RenderSetClipRect(bd->SDLRenderer, &r);

                const float* xy = (const float*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos));
                const float* uv = (const float*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv));
                const int* color = (const int*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col));

                // Bind texture, Draw
				SDL_Texture* tex = (SDL_Texture*)pcmd->GetTexID();
                SDL_RenderGeometryRaw(bd->SDLRenderer, tex,
                    xy, (int)sizeof(ImDrawVert),
                    color, (int)sizeof(ImDrawVert),
                    uv, (int)sizeof(ImDrawVert),
                    cmd_list->VtxBuffer.Size,
                    idx_buffer + pcmd->IdxOffset, pcmd->ElemCount, sizeof(ImDrawIdx));
            }
        }
    }

    // Restore modified SDL_Renderer state
    SDL_RenderSetViewport(bd->SDLRenderer, &old.Viewport);
    SDL_RenderSetClipRect(bd->SDLRenderer, old.ClipEnabled ? &old.ClipRect : NULL);
}

// Called by Init/NewFrame/Shutdown
bool ImGui_ImplSDLRenderer_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDLRenderer_Data* bd = ImGui_ImplSDLRenderer_GetBackendData();

    // Build texture atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    bd->FontTexture = SDL_CreateTexture(bd->SDLRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (bd->FontTexture == NULL)
    {
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
    if (bd->FontTexture)
    {
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
