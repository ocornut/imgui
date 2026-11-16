// dear imgui: Renderer Backend for Nintendo GameCube/Wii GX by Dakotath
// See imgui_impl_gx.h for usage and limitations.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_gx.h"

#include <gccore.h>
#include <malloc.h>
#include <string.h>

struct ImGui_ImplGX_Texture
{
    GXTexObj TexObj;
    void*    Pixels;
    int      Width;
    int      Height;
};

static ImGui_ImplGX_Texture* ImGui_ImplGX_GetTexture(ImTextureData* tex)
{
    return (ImGui_ImplGX_Texture*)tex->BackendUserData;
}

static int ImGui_ImplGX_Align4(int value) { return (value + 3) & ~3; }

// GX_TF_RGBA8 stores each 4x4 tile as AR bytes followed by GB bytes.
// GX_TF_RGBA8 stores each 4x4 tile as:
// AR AR AR AR
// AR AR AR AR
// AR AR AR AR
// AR AR AR AR
//
// GB GB GB GB
// GB GB GB GB
// GB GB GB GB
// GB GB GB GB

static void ImGui_ImplGX_ConvertRGBA8(
    void* dst_data,
    ImTextureData* tex,
    int padded_width,
    int padded_height)
{
    unsigned char* dst = (unsigned char*)dst_data;
    const unsigned char* src =
        (const unsigned char*)tex->GetPixels();

    const int pitch = tex->GetPitch();

    for (int y = 0; y < padded_height; y += 4)
    {
        for (int x = 0; x < padded_width; x += 4)
        {
            /*
             * AR plane
             */
            for (int row = 0; row < 4; row++)
            {
                for (int col = 0; col < 4; col++)
                {
                    unsigned char r = 0;
                    unsigned char a = 0;

                    if (x + col < tex->Width &&
                        y + row < tex->Height)
                    {
                        const unsigned char* pixel =
                            src +
                            (y + row) * pitch +
                            (x + col) * 4;

#ifdef GEKKO
                        // Wii/GameCube PowerPC:
                        // ImGui RGBA32 memory requires ABGR interpretation.
                        a = pixel[0];
                        r = pixel[3];
#else
                        a = pixel[3];
                        r = pixel[0];
#endif
                    }

                    // GX RGBA8 AR plane
                    *dst++ = a;
                    *dst++ = r;
                }
            }


            /*
             * GB plane
             */
            for (int row = 0; row < 4; row++)
            {
                for (int col = 0; col < 4; col++)
                {
                    unsigned char g = 0;
                    unsigned char b = 0;

                    if (x + col < tex->Width &&
                        y + row < tex->Height)
                    {
                        const unsigned char* pixel =
                            src +
                            (y + row) * pitch +
                            (x + col) * 4;

#ifdef GEKKO
                        // ABGR source
                        b = pixel[1];
                        g = pixel[2];
#else
                        // RGBA source
                        g = pixel[1];
                        b = pixel[2];
#endif
                    }

                    // GX RGBA8 GB plane
                    *dst++ = g;
                    *dst++ = b;
                }
            }
        }
    }
}

void ImGui_ImplGX_UpdateTexture(ImTextureData* tex)
{
    if (tex->Status == ImTextureStatus_WantCreate)
    {
        IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);
        ImGui_ImplGX_Texture* texture = IM_NEW(ImGui_ImplGX_Texture)();
        texture->Width = ImGui_ImplGX_Align4(tex->Width);
        texture->Height = ImGui_ImplGX_Align4(tex->Height);
        texture->Pixels = memalign(32, texture->Width * texture->Height * 4);
        IM_ASSERT(texture->Pixels != nullptr);
        ImGui_ImplGX_ConvertRGBA8(texture->Pixels, tex, texture->Width, texture->Height);
        DCFlushRange(texture->Pixels, texture->Width * texture->Height * 4);

        // TODO: Investigate fucking font problem
        GX_InitTexObj(&texture->TexObj, texture->Pixels, texture->Width, texture->Height, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
        GX_InitTexObjFilterMode(&texture->TexObj, GX_LINEAR, GX_LINEAR);
        tex->BackendUserData = texture;
        tex->SetTexID((ImTextureID)&texture->TexObj);
        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantUpdates)
    {
        // A tiled GX texture cannot safely receive arbitrary rectangular updates;
        // repacking the atlas is small and keeps all dynamic-atlas updates correct.
        ImGui_ImplGX_Texture* texture = ImGui_ImplGX_GetTexture(tex);
        IM_ASSERT(texture != nullptr && tex->Format == ImTextureFormat_RGBA32);
        ImGui_ImplGX_ConvertRGBA8(texture->Pixels, tex, texture->Width, texture->Height);
        DCFlushRange(texture->Pixels, texture->Width * texture->Height * 4);
        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantDestroy)
    {
        ImGui_ImplGX_Texture* texture = ImGui_ImplGX_GetTexture(tex);
        if (texture != nullptr)
        {
            free(texture->Pixels);
            IM_DELETE(texture);
        }
        tex->BackendUserData = nullptr;
        tex->SetTexID(ImTextureID_Invalid);
        tex->SetStatus(ImTextureStatus_Destroyed);
    }
}

static void ImGui_ImplGX_SetupRenderState(ImDrawData* draw_data)
{
    Mtx44 projection;
    Mtx model;
    const float left = draw_data->DisplayPos.x;
    const float right = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    const float top = draw_data->DisplayPos.y;
    const float bottom = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

    guOrtho(projection, top, bottom, left, right, -1.0f, 1.0f);
    guMtxIdentity(model);
    GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);
    GX_LoadPosMtxImm(model, GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);

    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    GX_SetNumTexGens(1);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetNumTevStages(1);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_TRUE);

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
}

static void ImGui_ImplGX_DrawCallback_ResetRenderState(const ImDrawList*, const ImDrawCmd*) {}

void ImGui_ImplGX_RenderDrawData(ImDrawData* draw_data)
{
    const int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    const int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    if (draw_data->Textures != nullptr)
        for (ImTextureData* tex : *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_ImplGX_UpdateTexture(tex);

    ImGui_ImplGX_SetupRenderState(draw_data);
    const ImVec2 clip_off = draw_data->DisplayPos;
    const ImVec2 clip_scale = draw_data->FramebufferScale;
    for (const ImDrawList* draw_list : draw_data->CmdLists)
    {
        const ImDrawVert* vtx_buffer = draw_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = draw_list->IdxBuffer.Data;
        for (const ImDrawCmd* pcmd = draw_list->CmdBuffer.begin(); pcmd != draw_list->CmdBuffer.end(); pcmd++)
        {
            if (pcmd->UserCallback != nullptr)
            {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplGX_SetupRenderState(draw_data);
                else
                    pcmd->UserCallback(draw_list, pcmd);
                continue;
            }
            ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
            ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
            if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                continue;
            const int x = (int)clip_min.x < 0 ? 0 : (int)clip_min.x;
            const int y = (int)clip_min.y < 0 ? 0 : (int)clip_min.y;
            const int w = (int)(clip_max.x - clip_min.x);
            const int h = (int)(clip_max.y - clip_min.y);
            GX_SetScissor(x, y, w, h);
            GXTexObj* texture = (GXTexObj*)pcmd->GetTexID();
            IM_ASSERT(texture != nullptr && "GX backend expects GXTexObj* texture IDs");
            GX_LoadTexObj(texture, GX_TEXMAP0);
            // GX_Begin() accepts a 16-bit vertex count. Split large commands while
            // retaining ImGui's VtxOffset support for 16-bit index buffers.
            unsigned int remaining = pcmd->ElemCount;
            unsigned int index_offset = 0;
            while (remaining > 0)
            {
                const unsigned int count = remaining > 65535 ? 65535 : remaining;
                GX_Begin(GX_TRIANGLES, GX_VTXFMT0, (u16)count);
                for (unsigned int index = 0; index < count; index++)
                {
                    const ImDrawVert& v = vtx_buffer[pcmd->VtxOffset + idx_buffer[pcmd->IdxOffset + index_offset + index]];
                    GX_Position3f32(v.pos.x, v.pos.y, 0.0f);
                    GX_Color4u8((u8)(v.col & 0xff), (u8)((v.col >> 8) & 0xff), (u8)((v.col >> 16) & 0xff), (u8)((v.col >> 24) & 0xff));
                    GX_TexCoord2f32(v.uv.x, v.uv.y);
                }
                index_offset += count;
                remaining -= count;
            }
        }
    }
}

void ImGui_ImplGX_NewFrame()
{
    IM_ASSERT(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().BackendRendererUserData != nullptr && "Did you call ImGui_ImplGX_Init()?");
}

void ImGui_ImplGX_DestroyDeviceObjects()
{
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
        if (tex->RefCount == 1)
        {
            tex->SetStatus(ImTextureStatus_WantDestroy);
            ImGui_ImplGX_UpdateTexture(tex);
        }
}

bool ImGui_ImplGX_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");
    io.BackendRendererUserData = (void*)1;
    io.BackendRendererName = "imgui_impl_gx";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    ImGui::GetPlatformIO().DrawCallback_ResetRenderState = ImGui_ImplGX_DrawCallback_ResetRenderState;
    return true;
}

void ImGui_ImplGX_Shutdown()
{
    ImGui_ImplGX_DestroyDeviceObjects();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::GetPlatformIO().ClearRendererHandlers();
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset);
}

#endif // #ifndef IMGUI_DISABLE
