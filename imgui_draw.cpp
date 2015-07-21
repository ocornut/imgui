// ImGui library v1.44 WIP
// Drawing and font code

// Contains implementation for
// - ImDrawList
// - ImDrawData
// - ImFontAtlas
// - ImFont
// - Default font data

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_FUNCTIONS

#include "imgui_internal.h"
#include <stdio.h>      // vsnprintf, sscanf, printf
#include <new>          // new (ptr)

#ifdef _MSC_VER
#pragma warning (disable: 4505) // unreferenced local function has been removed (stb stuff)
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#define snprintf _snprintf
#endif

//-------------------------------------------------------------------------
// STB libraries implementation
//-------------------------------------------------------------------------

//#define IMGUI_STB_NAMESPACE     ImGuiStb
//#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION

#ifdef IMGUI_STB_NAMESPACE
namespace IMGUI_STB_NAMESPACE
{
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

#define STBRP_ASSERT(x)    IM_ASSERT(x)
#ifndef IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#include "stb_rect_pack.h"

#define STBTT_malloc(x,u)  ((void)(u), ImGui::MemAlloc(x))
#define STBTT_free(x,u)    ((void)(u), ImGui::MemFree(x))
#define STBTT_assert(x)    IM_ASSERT(x)
#ifndef IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#else
#define STBTT_DEF extern
#endif
#include "stb_truetype.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef IMGUI_STB_NAMESPACE
} // namespace ImGuiStb
using namespace IMGUI_STB_NAMESPACE;
#endif

//-----------------------------------------------------------------------------
// ImDrawList
//-----------------------------------------------------------------------------

static ImVec4 GNullClipRect(-9999.0f,-9999.0f, +9999.0f, +9999.0f);

void ImDrawList::Clear()
{
    CmdBuffer.resize(0);
    IdxBuffer.resize(0);
    VtxBuffer.resize(0);
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.resize(0);
    _TextureIdStack.resize(0);
    _Path.resize(0);
    _ChannelCurrent = 0;
    // NB: Do not clear channels so our allocations are re-used after the first frame.
}

void ImDrawList::ClearFreeMemory()
{
    CmdBuffer.clear();
    IdxBuffer.clear();
    VtxBuffer.clear();
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.clear();
    _TextureIdStack.clear();
    _Path.clear();
    _ChannelCurrent = 0;
    for (int i = 0; i < _Channels.Size; i++)
    {
        if (i == 0) memset(&_Channels[0], 0, sizeof(_Channels[0]));  // channel 0 is a copy of CmdBuffer/IdxBuffer, don't destruct again
        _Channels[i].CmdBuffer.clear();
        _Channels[i].IdxBuffer.clear();
    }
    _Channels.clear();
}

void ImDrawList::AddDrawCmd()
{
    ImDrawCmd draw_cmd;
    draw_cmd.ElemCount = 0;
    draw_cmd.ClipRect = _ClipRectStack.Size ? _ClipRectStack.back() : GNullClipRect;
    draw_cmd.TextureId = _TextureIdStack.Size ? _TextureIdStack.back() : NULL;
    draw_cmd.UserCallback = NULL;
    draw_cmd.UserCallbackData = NULL;

    IM_ASSERT(draw_cmd.ClipRect.x <= draw_cmd.ClipRect.z && draw_cmd.ClipRect.y <= draw_cmd.ClipRect.w);
    CmdBuffer.push_back(draw_cmd);
}

void ImDrawList::AddCallback(ImDrawCallback callback, void* callback_data)
{
    ImDrawCmd* current_cmd = CmdBuffer.Size ? &CmdBuffer.back() : NULL;
    if (!current_cmd || current_cmd->ElemCount != 0 || current_cmd->UserCallback != NULL)
    {
        AddDrawCmd();
        current_cmd = &CmdBuffer.back();
    }
    current_cmd->UserCallback = callback;
    current_cmd->UserCallbackData = callback_data;

    // Force a new command after us (we function this way so that the most common calls AddLine, AddRect, etc. always have a command to add to without doing any check).
    AddDrawCmd();
}

void ImDrawList::ChannelsSplit(int channel_count)
{
    IM_ASSERT(_ChannelCurrent == 0);
    int old_channels_count = _Channels.Size;
    if (old_channels_count < channel_count)
        _Channels.resize(channel_count);
    for (int i = 0; i < channel_count; i++)
        if (i >= old_channels_count)
            new(&_Channels[i]) ImDrawChannel();
        else
            _Channels[i].CmdBuffer.resize(0), _Channels[i].IdxBuffer.resize(0);
}

void ImDrawList::ChannelsMerge(int channel_count)
{
    // Note that we never use or rely on channels.Size because it is merely a buffer that we never shrink back to 0 to keep all sub-buffers ready for use.
    // This is why this function takes 'channel_count' as a parameter of how many channels to merge (the user knows)
    if (channel_count < 2)
        return;

    ChannelsSetCurrent(0);
    if (CmdBuffer.Size && CmdBuffer.back().ElemCount == 0) 
        CmdBuffer.pop_back();

    int new_cmd_buffer_count = 0, new_idx_buffer_count = 0;
    for (int i = 1; i < channel_count; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (ch.CmdBuffer.Size && ch.CmdBuffer.back().ElemCount == 0)
            ch.CmdBuffer.pop_back();
        new_cmd_buffer_count += ch.CmdBuffer.Size;
        new_idx_buffer_count += ch.IdxBuffer.Size;
    }
    CmdBuffer.resize(CmdBuffer.Size + new_cmd_buffer_count);
    IdxBuffer.resize(IdxBuffer.Size + new_idx_buffer_count);

    ImDrawCmd* cmd_write = CmdBuffer.Data + CmdBuffer.Size - new_cmd_buffer_count;
    _IdxWritePtr = IdxBuffer.Data + IdxBuffer.Size - new_idx_buffer_count;
    for (int i = 1; i < channel_count; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (int sz = ch.CmdBuffer.Size) { memcpy(cmd_write, ch.CmdBuffer.Data, sz * sizeof(ImDrawCmd)); cmd_write += sz; }
        if (int sz = ch.IdxBuffer.Size) { memcpy(_IdxWritePtr, ch.IdxBuffer.Data, sz * sizeof(ImDrawIdx)); _IdxWritePtr += sz; }
    }
    AddDrawCmd();
}

void ImDrawList::ChannelsSetCurrent(int idx)
{
    if (_ChannelCurrent == idx) return;
    memcpy(&_Channels.Data[_ChannelCurrent].CmdBuffer, &CmdBuffer, sizeof(CmdBuffer));
    memcpy(&_Channels.Data[_ChannelCurrent].IdxBuffer, &IdxBuffer, sizeof(IdxBuffer));
    _ChannelCurrent = idx;
    memcpy(&CmdBuffer, &_Channels.Data[_ChannelCurrent].CmdBuffer, sizeof(CmdBuffer));
    memcpy(&IdxBuffer, &_Channels.Data[_ChannelCurrent].IdxBuffer, sizeof(IdxBuffer));
    _IdxWritePtr = IdxBuffer.Data + IdxBuffer.Size;
}

void ImDrawList::UpdateClipRect()
{
    ImDrawCmd* current_cmd = CmdBuffer.Size ? &CmdBuffer.back() : NULL;
    if (!current_cmd || (current_cmd->ElemCount != 0) || current_cmd->UserCallback != NULL)
    {
        AddDrawCmd();
    }
    else
    {
        ImVec4 current_clip_rect = _ClipRectStack.Size ? _ClipRectStack.back() : GNullClipRect;
        if (CmdBuffer.Size >= 2 && ImLengthSqr(CmdBuffer.Data[CmdBuffer.Size-2].ClipRect - current_clip_rect) < 0.00001f)
            CmdBuffer.pop_back();
        else
            current_cmd->ClipRect = current_clip_rect;
    }
}

// Scissoring. The values in clip_rect are x1, y1, x2, y2.
void ImDrawList::PushClipRect(const ImVec4& clip_rect)
{
    _ClipRectStack.push_back(clip_rect);
    UpdateClipRect();
}

void ImDrawList::PushClipRectFullScreen()
{
    PushClipRect(GNullClipRect);

    // This would be more correct but we're not supposed to access ImGuiState from here?
    //ImGuiState& g = *GImGui;
    //PushClipRect(GetVisibleRect());
}

void ImDrawList::PopClipRect()
{
    IM_ASSERT(_ClipRectStack.Size > 0);
    _ClipRectStack.pop_back();
    UpdateClipRect();
}

void ImDrawList::UpdateTextureID()
{
    ImDrawCmd* current_cmd = CmdBuffer.Size ? &CmdBuffer.back() : NULL;
    const ImTextureID texture_id = _TextureIdStack.Size ? _TextureIdStack.back() : NULL;
    if (!current_cmd || (current_cmd->ElemCount != 0 && current_cmd->TextureId != texture_id) || current_cmd->UserCallback != NULL)
        AddDrawCmd();
    else
        current_cmd->TextureId = texture_id;
}

void ImDrawList::PushTextureID(const ImTextureID& texture_id)
{
    _TextureIdStack.push_back(texture_id);
    UpdateTextureID();
}

void ImDrawList::PopTextureID()
{
    IM_ASSERT(_TextureIdStack.Size > 0);
    _TextureIdStack.pop_back();
    UpdateTextureID();
}

void ImDrawList::PrimReserve(int idx_count, int vtx_count)
{
    ImDrawCmd& draw_cmd = CmdBuffer.Data[CmdBuffer.Size-1];
    draw_cmd.ElemCount += idx_count;

    int vtx_buffer_size = VtxBuffer.Size;
    VtxBuffer.resize(vtx_buffer_size + vtx_count);
    _VtxWritePtr = VtxBuffer.Data + vtx_buffer_size;

    int idx_buffer_size = IdxBuffer.Size;
    IdxBuffer.resize(idx_buffer_size + idx_count);
    _IdxWritePtr = IdxBuffer.Data + idx_buffer_size;
}

void ImDrawList::PrimRect(const ImVec2& a, const ImVec2& c, ImU32 col)
{
    const ImVec2 uv = GImGui->FontTexUvWhitePixel;
    const ImVec2 b(c.x, a.y);
    const ImVec2 d(a.x, c.y);
    _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx+1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx+2); 
    _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx+2); _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx+3); 
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col; 
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col; 
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col; 
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimRectUV(const ImVec2& a, const ImVec2& c, const ImVec2& uv_a, const ImVec2& uv_c, ImU32 col)
{
    const ImVec2 b(c.x, a.y);
    const ImVec2 d(a.x, c.y);
    const ImVec2 uv_b(uv_c.x, uv_a.y);
    const ImVec2 uv_d(uv_a.x, uv_c.y);
    _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx+1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx+2); 
    _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx+2); _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx+3); 
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col; 
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col; 
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col; 
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::AddPolyline(const ImVec2* points, const int points_count, ImU32 col, bool closed, float thickness, bool anti_aliased)
{
    if (points_count < 2)
        return;

    const ImVec2 uv = GImGui->FontTexUvWhitePixel;
    anti_aliased &= GImGui->Style.AntiAliasedLines;
    //if (ImGui::GetIO().KeyCtrl) anti_aliased = false;

    int count = points_count;
    if (!closed)
        count = points_count-1;

    if (anti_aliased && thickness == 1.0f)
    {
        // Anti-aliased stroke
        const float AA_SIZE = 1.0f;
        const ImU32 col_trans = col & 0x00ffffff;
        const int idx_count = count*12;
        const int vtx_count = points_count*3;
        PrimReserve(idx_count, vtx_count);

        // Temporary buffer
        ImVec2* temp_points = (ImVec2*)alloca(points_count * 3 * sizeof(ImVec2));
        ImVec2* temp_normals = temp_points + points_count * 2;

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1+1) == points_count ? 0 : i1+1; 
            ImVec2 diff = points[i2] - points[i1];
            diff *= ImInvLength(diff, 1.0f);
            temp_normals[i1].x = diff.y;
            temp_normals[i1].y = -diff.x;
        }

        if (!closed)
        {
            temp_normals[points_count-1] = temp_normals[points_count-2];
            temp_points[0] = points[0] + temp_normals[0] * AA_SIZE;
            temp_points[1] = points[0] - temp_normals[0] * AA_SIZE;
            temp_points[(points_count-1)*2+0] = points[points_count-1] + temp_normals[points_count-1] * AA_SIZE;
            temp_points[(points_count-1)*2+1] = points[points_count-1] - temp_normals[points_count-1] * AA_SIZE;
        }

        // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
        unsigned int idx1 = _VtxCurrentIdx;
        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1+1) == points_count ? 0 : i1+1;
            unsigned int idx2 = (i1+1) == points_count ? _VtxCurrentIdx : idx1+3;

            // Average normals
            ImVec2 dm = (temp_normals[i1] + temp_normals[i2]) * 0.5f;
            float dmr2 = dm.x*dm.x + dm.y*dm.y;
            if (dmr2 > 0.000001f)
            {
                float scale = 1.0f / dmr2;
                if (scale > 100.0f) scale = 100.0f;
                dm *= scale;
            }
            dm *= AA_SIZE;
            temp_points[i2*2+0] = points[i2] + dm;
            temp_points[i2*2+1] = points[i2] - dm;

            // Add indexes
            _IdxWritePtr[0] = (ImDrawIdx)(idx2+0); _IdxWritePtr[1] = (ImDrawIdx)(idx1+0); _IdxWritePtr[2] = (ImDrawIdx)(idx1+2);
            _IdxWritePtr[3] = (ImDrawIdx)(idx1+2); _IdxWritePtr[4] = (ImDrawIdx)(idx2+2); _IdxWritePtr[5] = (ImDrawIdx)(idx2+0);
            _IdxWritePtr[6] = (ImDrawIdx)(idx2+1); _IdxWritePtr[7] = (ImDrawIdx)(idx1+1); _IdxWritePtr[8] = (ImDrawIdx)(idx1+0);
            _IdxWritePtr[9] = (ImDrawIdx)(idx1+0); _IdxWritePtr[10]= (ImDrawIdx)(idx2+0); _IdxWritePtr[11]= (ImDrawIdx)(idx2+1);
            _IdxWritePtr += 12;

            idx1 = idx2;
        }

        // Add vertexes
        for (int i = 0; i < points_count; i++)
        {
            _VtxWritePtr[0].pos = points[i];          _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr[1].pos = temp_points[i*2+0]; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;
            _VtxWritePtr[2].pos = temp_points[i*2+1]; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col_trans;
            _VtxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Stroke
        const int idx_count = count*6;
        const int vtx_count = count*4;      // FIXME-OPT: Not sharing edges
        PrimReserve(idx_count, vtx_count);

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1+1) == points_count ? 0 : i1+1; 
            const ImVec2& p1 = points[i1];
            const ImVec2& p2 = points[i2];
            ImVec2 diff = p2 - p1;
            diff *= ImInvLength(diff, 1.0f);

            const float dx = diff.x * (thickness * 0.5f);
            const float dy = diff.y * (thickness * 0.5f);
            _VtxWritePtr[0].pos.x = p1.x + dy; _VtxWritePtr[0].pos.y = p1.y - dx; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr[1].pos.x = p2.x + dy; _VtxWritePtr[1].pos.y = p2.y - dx; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
            _VtxWritePtr[2].pos.x = p2.x - dy; _VtxWritePtr[2].pos.y = p2.y + dx; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
            _VtxWritePtr[3].pos.x = p1.x - dy; _VtxWritePtr[3].pos.y = p1.y + dx; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
            _VtxWritePtr += 4;

            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx+1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx+2); 
            _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx+2); _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx+3); 
            _IdxWritePtr += 6;
            _VtxCurrentIdx += 4;
        }
    }
}

void ImDrawList::AddConvexPolyFilled(const ImVec2* points, const int points_count, ImU32 col, bool anti_aliased)
{
    const ImVec2 uv = GImGui->FontTexUvWhitePixel;
    anti_aliased &= GImGui->Style.AntiAliasedShapes;
    //if (ImGui::GetIO().KeyCtrl) anti_aliased = false;

    if (anti_aliased)
    {
        // Anti-aliased Fill
        const float AA_SIZE = 1.0f;
        const ImU32 col_trans = col & 0x00ffffff;
        const int idx_count = (points_count-2)*3 + points_count*6;
        const int vtx_count = (points_count*2);
        PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx+1;
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+((i-1)<<1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx+(i<<1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        ImVec2* temp_normals = (ImVec2*)alloca(points_count * sizeof(ImVec2));
        for (int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            ImVec2 diff = p1 - p0;
            diff *= ImInvLength(diff, 1.0f);
            temp_normals[i0].x = diff.y;
            temp_normals[i0].y = -diff.x;
        }

        for (int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            ImVec2 dm = (n0 + n1) * 0.5f;
            float dmr2 = dm.x*dm.x + dm.y*dm.y;
            if (dmr2 > 0.000001f)
            {
                float scale = 1.0f / dmr2;
                if (scale > 100.0f) scale = 100.0f;
                dm *= scale;
            }
            dm *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos = (points[i1] - dm); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos = (points[i1] + dm); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx+(i1<<1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+(i0<<1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx+(i0<<1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx+(i0<<1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx+(i1<<1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx+(i1<<1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count-2)*3;
        const int vtx_count = points_count;
        PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx+i-1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx+i); 
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

void ImDrawList::PathArcToFast(const ImVec2& centre, float radius, int amin, int amax)
{
    static ImVec2 circle_vtx[12];
    static bool circle_vtx_builds = false;
    const int circle_vtx_count = IM_ARRAYSIZE(circle_vtx);
    if (!circle_vtx_builds)
    {
        for (int i = 0; i < circle_vtx_count; i++)
        {
            const float a = ((float)i / (float)circle_vtx_count) * 2*IM_PI;
            circle_vtx[i].x = cosf(a);
            circle_vtx[i].y = sinf(a);
        }
        circle_vtx_builds = true;
    }

    if (amin > amax) return;
    if (radius == 0.0f)
    {
        _Path.push_back(centre);
    }
    else
    {
        _Path.reserve(_Path.Size + (amax - amin + 1));
        for (int a = amin; a <= amax; a++)
        {
            const ImVec2& c = circle_vtx[a % circle_vtx_count];
            _Path.push_back(ImVec2(centre.x + c.x * radius, centre.y + c.y * radius));
        }
    }
}

void ImDrawList::PathArcTo(const ImVec2& centre, float radius, float amin, float amax, int num_segments)
{
    if (radius == 0.0f)
        _Path.push_back(centre);
    _Path.reserve(_Path.Size + (num_segments + 1));
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = amin + ((float)i / (float)num_segments) * (amax - amin);
        _Path.push_back(ImVec2(centre.x + cosf(a) * radius, centre.y + sinf(a) * radius));
    }
}

void ImDrawList::PathRect(const ImVec2& a, const ImVec2& b, float rounding, int rounding_corners)
{
    float r = rounding;
    r = ImMin(r, fabsf(b.x-a.x) * ( ((rounding_corners&(1|2))==(1|2)) || ((rounding_corners&(4|8))==(4|8)) ? 0.5f : 1.0f ) - 1.0f);
    r = ImMin(r, fabsf(b.y-a.y) * ( ((rounding_corners&(1|8))==(1|8)) || ((rounding_corners&(2|4))==(2|4)) ? 0.5f : 1.0f ) - 1.0f);

    if (r == 0.0f || rounding_corners == 0)
    {
        PathLineTo(a);
        PathLineTo(ImVec2(b.x,a.y));
        PathLineTo(b);
        PathLineTo(ImVec2(a.x,b.y));
    }
    else
    {
        const float r0 = (rounding_corners & 1) ? r : 0.0f;
        const float r1 = (rounding_corners & 2) ? r : 0.0f;
        const float r2 = (rounding_corners & 4) ? r : 0.0f;
        const float r3 = (rounding_corners & 8) ? r : 0.0f;
        PathArcToFast(ImVec2(a.x+r0,a.y+r0), r0, 6, 9);
        PathArcToFast(ImVec2(b.x-r1,a.y+r1), r1, 9, 12);
        PathArcToFast(ImVec2(b.x-r2,b.y-r2), r2, 0, 3);
        PathArcToFast(ImVec2(a.x+r3,b.y-r3), r3, 3, 6);
    }
}

void ImDrawList::AddLine(const ImVec2& a, const ImVec2& b, ImU32 col, float thickness)
{
    if ((col >> 24) == 0)
        return;
    PathLineTo(a + ImVec2(0.5f,0.5f));
    PathLineTo(b + ImVec2(0.5f,0.5f));
    PathStroke(col, false, thickness);
}

void ImDrawList::AddRect(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners)
{
    if ((col >> 24) == 0)
        return;
    PathRect(a + ImVec2(0.5f,0.5f), b + ImVec2(0.5f,0.5f), rounding, rounding_corners);
    PathStroke(col, true);
}

void ImDrawList::AddRectFilled(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners)
{
    if ((col >> 24) == 0)
        return;
    if (rounding > 0.0f)
    {
        PathRect(a, b, rounding, rounding_corners);
        PathFill(col);
    }
    else
    {
        PrimReserve(6, 4);
        PrimRect(a, b, col);
    }
}

void ImDrawList::AddRectFilledMultiColor(const ImVec2& a, const ImVec2& c, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left)
{
    if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) >> 24) == 0)
        return;

    const ImVec2 uv = GImGui->FontTexUvWhitePixel;
    PrimReserve(6, 4);
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+1)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+2));
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+2)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+3));
    PrimWriteVtx(a, uv, col_upr_left);
    PrimWriteVtx(ImVec2(c.x, a.y), uv, col_upr_right);
    PrimWriteVtx(c, uv, col_bot_right);
    PrimWriteVtx(ImVec2(a.x, c.y), uv, col_bot_left);
}

void ImDrawList::AddTriangleFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col)
{
    if ((col >> 24) == 0)
        return;
    PathLineTo(a);
    PathLineTo(b);
    PathLineTo(c);
    PathFill(col);
}

void ImDrawList::AddCircle(const ImVec2& centre, float radius, ImU32 col, int num_segments)
{
    if ((col >> 24) == 0)
        return;

    const float a_max = IM_PI*2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(centre, radius, 0.0f, a_max, num_segments);
    PathStroke(col, true);
}

void ImDrawList::AddCircleFilled(const ImVec2& centre, float radius, ImU32 col, int num_segments)
{
    if ((col >> 24) == 0)
        return;

    const float a_max = IM_PI*2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(centre, radius, 0.0f, a_max, num_segments);
    PathFill(col);
}

void ImDrawList::AddText(const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width, const ImVec4* cpu_fine_clip_rect)
{
    if ((col >> 24) == 0)
        return;

    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);
    if (text_begin == text_end)
        return;

    IM_ASSERT(font->ContainerAtlas->TexID == _TextureIdStack.back());  // Use high-level ImGui::PushFont() or low-level ImDrawList::PushTextureId() to change font.

    // reserve vertices for worse case
    const int char_count = (int)(text_end - text_begin);
    const int vtx_count_max = char_count * 4;
    const int idx_count_max = char_count * 6;
    const int vtx_begin = VtxBuffer.Size;
    const int idx_begin = IdxBuffer.Size;
    PrimReserve(idx_count_max, vtx_count_max);

    ImVec4 clip_rect = _ClipRectStack.back();
    if (cpu_fine_clip_rect)
    {
        clip_rect.x = ImMax(clip_rect.x, cpu_fine_clip_rect->x);
        clip_rect.y = ImMax(clip_rect.y, cpu_fine_clip_rect->y);
        clip_rect.z = ImMin(clip_rect.z, cpu_fine_clip_rect->z);
        clip_rect.w = ImMin(clip_rect.w, cpu_fine_clip_rect->w);
    }
    font->RenderText(font_size, pos, col, clip_rect, text_begin, text_end, this, wrap_width, cpu_fine_clip_rect != NULL);

    // give back unused vertices
    // FIXME-OPT
    VtxBuffer.resize((int)(_VtxWritePtr - VtxBuffer.Data));
    IdxBuffer.resize((int)(_IdxWritePtr - IdxBuffer.Data));
    int vtx_unused = vtx_count_max - (VtxBuffer.Size - vtx_begin);
    int idx_unused = idx_count_max - (IdxBuffer.Size - idx_begin);
    CmdBuffer.back().ElemCount -= idx_unused;
    _VtxWritePtr -= vtx_unused;
    _IdxWritePtr -= idx_unused;
    _VtxCurrentIdx = (ImDrawIdx)VtxBuffer.Size;
}

// This is one of the few function breaking the encapsulation of ImDrawLst, but it is just so useful.
void ImDrawList::AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end)
{
    if ((col >> 24) == 0)
        return;
    AddText(ImGui::GetWindowFont(), ImGui::GetWindowFontSize(), pos, col, text_begin, text_end);
}

void ImDrawList::AddImage(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv0, const ImVec2& uv1, ImU32 col)
{
    if ((col >> 24) == 0)
        return;

    // FIXME-OPT: This is wasting draw calls.
    const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimRectUV(a, b, uv0, uv1, col);

    if (push_texture_id)
        PopTextureID();
}

//-----------------------------------------------------------------------------
// ImDrawData
//-----------------------------------------------------------------------------

// For backward compatibility: convert all buffers from indexed to de-indexed, in case you cannot render indexed. Note: this is slow and most likely a waste of resources. Always prefer indexed rendering!
void ImDrawData::DeIndexAllBuffers()
{
    ImVector<ImDrawVert> new_vtx_buffer;
    TotalVtxCount = TotalIdxCount = 0;
    for (int i = 0; i < CmdListsCount; i++)
    {
        ImDrawList* cmd_list = CmdLists[i];
        if (cmd_list->IdxBuffer.empty())
            continue;
        new_vtx_buffer.resize(cmd_list->IdxBuffer.Size);
        for (int j = 0; j < cmd_list->IdxBuffer.Size; j++)
            new_vtx_buffer[j] = cmd_list->VtxBuffer[cmd_list->IdxBuffer[j]];
        cmd_list->VtxBuffer.swap(new_vtx_buffer);
        cmd_list->IdxBuffer.resize(0);
        TotalVtxCount += cmd_list->VtxBuffer.Size;
    }
}

//-----------------------------------------------------------------------------
// ImFontAtlias
//-----------------------------------------------------------------------------

ImFontConfig::ImFontConfig()
{
    FontData = NULL;
    FontDataSize = 0;
    FontDataOwnedByAtlas = true;
    FontNo = 0;
    SizePixels = 0.0f;
    OversampleH = 3;
    OversampleV = 1;
    PixelSnapH = false;
    GlyphExtraSpacing = ImVec2(0.0f, 0.0f);
    GlyphRanges = NULL;
    MergeMode = false;
    MergeGlyphCenterV = false;
    DstFont = NULL;
    memset(Name, 0, sizeof(Name));
}

ImFontAtlas::ImFontAtlas()
{
    TexID = NULL;
    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
    TexWidth = TexHeight = 0;
    TexUvWhitePixel = ImVec2(0, 0);
}

ImFontAtlas::~ImFontAtlas()
{
    Clear();
}

void    ImFontAtlas::ClearInputData()
{
    for (int i = 0; i < ConfigData.Size; i++)
        if (ConfigData[i].FontData && ConfigData[i].FontDataOwnedByAtlas)
        {
            ImGui::MemFree(ConfigData[i].FontData);
            ConfigData[i].FontData = NULL;
        }

        // When clearing this we lose access to the font name and other information used to build the font.
        for (int i = 0; i < Fonts.Size; i++)
            if (Fonts[i]->ConfigData >= ConfigData.Data && Fonts[i]->ConfigData < ConfigData.Data + ConfigData.Size)
            {
                Fonts[i]->ConfigData = NULL;
                Fonts[i]->ConfigDataCount = 0;
            }
            ConfigData.clear();
}

void    ImFontAtlas::ClearTexData()
{
    if (TexPixelsAlpha8)
        ImGui::MemFree(TexPixelsAlpha8);
    if (TexPixelsRGBA32)
        ImGui::MemFree(TexPixelsRGBA32);
    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
}

void    ImFontAtlas::ClearFonts()
{
    for (int i = 0; i < Fonts.Size; i++)
    {
        Fonts[i]->~ImFont();
        ImGui::MemFree(Fonts[i]);
    }
    Fonts.clear();
}

void    ImFontAtlas::Clear()
{
    ClearInputData(); 
    ClearTexData();
    ClearFonts();
}

void    ImFontAtlas::GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Lazily build
    if (TexPixelsAlpha8 == NULL)
    {
        if (ConfigData.empty())
            AddFontDefault();
        Build();
    }

    *out_pixels = TexPixelsAlpha8;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 1;
}

void    ImFontAtlas::GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Lazily convert to RGBA32 format
    // Although it is likely to be the most commonly used format, our font rendering is 8 bpp
    if (!TexPixelsRGBA32)
    {
        unsigned char* pixels;
        GetTexDataAsAlpha8(&pixels, NULL, NULL);
        TexPixelsRGBA32 = (unsigned int*)ImGui::MemAlloc((size_t)(TexWidth * TexHeight * 4));
        const unsigned char* src = pixels;
        unsigned int* dst = TexPixelsRGBA32;
        for (int n = TexWidth * TexHeight; n > 0; n--)
            *dst++ = ((unsigned int)(*src++) << 24) | 0x00FFFFFF;
    }

    *out_pixels = (unsigned char*)TexPixelsRGBA32;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 4;
}

ImFont* ImFontAtlas::AddFont(const ImFontConfig* font_cfg)
{
    IM_ASSERT(font_cfg->FontData != NULL && font_cfg->FontDataSize > 0);
    IM_ASSERT(font_cfg->SizePixels > 0.0f);

    // Create new font
    if (!font_cfg->MergeMode)
    {
        ImFont* font = (ImFont*)ImGui::MemAlloc(sizeof(ImFont));
        new (font) ImFont();
        Fonts.push_back(font);
    }

    ConfigData.push_back(*font_cfg);
    ImFontConfig& new_font_cfg = ConfigData.back();
    new_font_cfg.DstFont = Fonts.back();
    if (!new_font_cfg.FontDataOwnedByAtlas)
    {
        new_font_cfg.FontData = ImGui::MemAlloc(new_font_cfg.FontDataSize);
        new_font_cfg.FontDataOwnedByAtlas = true;
        memcpy(new_font_cfg.FontData, font_cfg->FontData, (size_t)new_font_cfg.FontDataSize);
    }

    // Invalidate texture
    ClearTexData();
    return Fonts.back();
}

static void GetDefaultCompressedFontDataTTF(const void** ttf_compressed_data, unsigned int* ttf_compressed_size);
static unsigned int stb_decompress_length(unsigned char *input);
static unsigned int stb_decompress(unsigned char *output, unsigned char *i, unsigned int length);

// Load embedded ProggyClean.ttf at size 13
ImFont* ImFontAtlas::AddFontDefault(const ImFontConfig* font_cfg_template)
{
    unsigned int ttf_compressed_size;
    const void* ttf_compressed;
    GetDefaultCompressedFontDataTTF(&ttf_compressed, &ttf_compressed_size);
    ImFontConfig font_cfg;
    if (font_cfg_template)
        font_cfg = *font_cfg_template;
    else
    {
        font_cfg.OversampleH = font_cfg.OversampleV = 1;
        font_cfg.PixelSnapH = true;
    }
    if (font_cfg.Name[0] == '\0') strcpy(font_cfg.Name, "<default>");
    return AddFontFromMemoryCompressedTTF(ttf_compressed, ttf_compressed_size, 13.0f, &font_cfg, GetGlyphRangesDefault());
}

ImFont* ImFontAtlas::AddFontFromFileTTF(const char* filename, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    void* data = NULL;
    int data_size = 0;
    if (!ImLoadFileToMemory(filename, "rb", (void**)&data, &data_size, 0))
    {
        IM_ASSERT(0); // Could not load file.
        return NULL;
    }
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (font_cfg.Name[0] == '\0')
    {
        const char* p; 
        for (p = filename + strlen(filename); p > filename && p[-1] != '/' && p[-1] != '\\'; p--) {}
        snprintf(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "%s", p);
    }
    return AddFontFromMemoryTTF(data, data_size, size_pixels, &font_cfg, glyph_ranges);
}

// Transfer ownership of 'ttf_data' to ImFontAtlas, will be deleted after Build()
ImFont* ImFontAtlas::AddFontFromMemoryTTF(void* ttf_data, int ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL); 
    font_cfg.FontData = ttf_data;
    font_cfg.FontDataSize = ttf_size;
    font_cfg.SizePixels = size_pixels;
    if (glyph_ranges)
        font_cfg.GlyphRanges = glyph_ranges;
    return AddFont(&font_cfg);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedTTF(const void* compressed_ttf_data, int compressed_ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    const unsigned int buf_decompressed_size = stb_decompress_length((unsigned char*)compressed_ttf_data);
    unsigned char* buf_decompressed_data = (unsigned char *)ImGui::MemAlloc(buf_decompressed_size);
    stb_decompress(buf_decompressed_data, (unsigned char*)compressed_ttf_data, (unsigned int)compressed_ttf_size);

    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL); 
    font_cfg.FontDataOwnedByAtlas = true;
    return AddFontFromMemoryTTF(buf_decompressed_data, (int)buf_decompressed_size, size_pixels, font_cfg_template, glyph_ranges);
}

bool    ImFontAtlas::Build()
{
    IM_ASSERT(ConfigData.Size > 0);

    TexID = NULL;
    TexWidth = TexHeight = 0;
    TexUvWhitePixel = ImVec2(0, 0);
    ClearTexData();

    struct ImFontTempBuildData
    {
        stbtt_fontinfo      FontInfo;
        stbrp_rect*         Rects;
        stbtt_pack_range*   Ranges;
        int                 RangesCount;
    };
    ImFontTempBuildData* tmp_array = (ImFontTempBuildData*)ImGui::MemAlloc((size_t)ConfigData.Size * sizeof(ImFontTempBuildData));

    // Initialize font information early (so we can error without any cleanup) + count glyphs
    int total_glyph_count = 0;
    int total_glyph_range_count = 0;
    for (int input_i = 0; input_i < ConfigData.Size; input_i++)
    {
        ImFontConfig& cfg = ConfigData[input_i];
        ImFontTempBuildData& tmp = tmp_array[input_i];

        IM_ASSERT(cfg.DstFont && (!cfg.DstFont->IsLoaded() || cfg.DstFont->ContainerAtlas == this));
        const int font_offset = stbtt_GetFontOffsetForIndex((unsigned char*)cfg.FontData, cfg.FontNo);
        IM_ASSERT(font_offset >= 0);
        if (!stbtt_InitFont(&tmp.FontInfo, (unsigned char*)cfg.FontData, font_offset)) 
            return false;

        if (!cfg.GlyphRanges)
            cfg.GlyphRanges = GetGlyphRangesDefault();
        for (const ImWchar* in_range = cfg.GlyphRanges; in_range[0] && in_range[1]; in_range += 2)
        {
            total_glyph_count += (in_range[1] - in_range[0]) + 1;
            total_glyph_range_count++;
        }
    }

    // Start packing
    TexWidth = (total_glyph_count > 1000) ? 1024 : 512;  // Width doesn't actually matters.
    TexHeight = 0;
    const int max_tex_height = 1024*32;
    stbtt_pack_context spc;
    stbtt_PackBegin(&spc, NULL, TexWidth, max_tex_height, 0, 1, NULL);

    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    ImVector<stbrp_rect> extra_rects;
    RenderCustomTexData(0, &extra_rects);
    stbtt_PackSetOversampling(&spc, 1, 1);
    stbrp_pack_rects((stbrp_context*)spc.pack_info, &extra_rects[0], extra_rects.Size);
    for (int i = 0; i < extra_rects.Size; i++)
        if (extra_rects[i].was_packed)
            TexHeight = ImMax(TexHeight, extra_rects[i].y + extra_rects[i].h);

    // Allocate packing character data and flag packed characters buffer as non-packed (x0=y0=x1=y1=0)
    int buf_packedchars_n = 0, buf_rects_n = 0, buf_ranges_n = 0;
    stbtt_packedchar* buf_packedchars = (stbtt_packedchar*)ImGui::MemAlloc(total_glyph_count * sizeof(stbtt_packedchar));
    stbrp_rect* buf_rects = (stbrp_rect*)ImGui::MemAlloc(total_glyph_count * sizeof(stbrp_rect));
    stbtt_pack_range* buf_ranges = (stbtt_pack_range*)ImGui::MemAlloc(total_glyph_range_count * sizeof(stbtt_pack_range));
    memset(buf_packedchars, 0, total_glyph_count * sizeof(stbtt_packedchar));
    memset(buf_rects, 0, total_glyph_count * sizeof(stbrp_rect));              // Unnecessary but let's clear this for the sake of sanity.
    memset(buf_ranges, 0, total_glyph_range_count * sizeof(stbtt_pack_range));

    // First font pass: pack all glyphs (no rendering at this point, we are working with glyph sizes only)
    for (int input_i = 0; input_i < ConfigData.Size; input_i++)
    {
        ImFontConfig& cfg = ConfigData[input_i];
        ImFontTempBuildData& tmp = tmp_array[input_i];

        // Setup ranges
        int glyph_count = 0;
        int glyph_ranges_count = 0;
        for (const ImWchar* in_range = cfg.GlyphRanges; in_range[0] && in_range[1]; in_range += 2)
        {
            glyph_count += (in_range[1] - in_range[0]) + 1;
            glyph_ranges_count++;
        }
        tmp.Ranges = buf_ranges + buf_ranges_n;
        tmp.RangesCount = glyph_ranges_count;
        buf_ranges_n += glyph_ranges_count;
        for (int i = 0; i < glyph_ranges_count; i++)
        {
            const ImWchar* in_range = &cfg.GlyphRanges[i * 2];
            stbtt_pack_range& range = tmp.Ranges[i];
            range.font_size = cfg.SizePixels;
            range.first_unicode_char_in_range = in_range[0];
            range.num_chars_in_range = (in_range[1] - in_range[0]) + 1;
            range.chardata_for_range = buf_packedchars + buf_packedchars_n;
            buf_packedchars_n += range.num_chars_in_range;
        }

        // Pack
        tmp.Rects = buf_rects + buf_rects_n;
        buf_rects_n += glyph_count;
        stbtt_PackSetOversampling(&spc, cfg.OversampleH, cfg.OversampleV);
        int n = stbtt_PackFontRangesGatherRects(&spc, &tmp.FontInfo, tmp.Ranges, tmp.RangesCount, tmp.Rects);
        stbrp_pack_rects((stbrp_context*)spc.pack_info, tmp.Rects, n);

        // Extend texture height
        for (int i = 0; i < n; i++)
            if (tmp.Rects[i].was_packed)
                TexHeight = ImMax(TexHeight, tmp.Rects[i].y + tmp.Rects[i].h);
    }
    IM_ASSERT(buf_rects_n == total_glyph_count);
    IM_ASSERT(buf_packedchars_n == total_glyph_count);
    IM_ASSERT(buf_ranges_n == total_glyph_range_count);

    // Create texture
    TexHeight = ImUpperPowerOfTwo(TexHeight);
    TexPixelsAlpha8 = (unsigned char*)ImGui::MemAlloc(TexWidth * TexHeight);
    memset(TexPixelsAlpha8, 0, TexWidth * TexHeight);
    spc.pixels = TexPixelsAlpha8;
    spc.height = TexHeight;

    // Second pass: render characters
    for (int input_i = 0; input_i < ConfigData.Size; input_i++)
    {
        ImFontConfig& cfg = ConfigData[input_i];
        ImFontTempBuildData& tmp = tmp_array[input_i];
        stbtt_PackSetOversampling(&spc, cfg.OversampleH, cfg.OversampleV);
        stbtt_PackFontRangesRenderIntoRects(&spc, &tmp.FontInfo, tmp.Ranges, tmp.RangesCount, tmp.Rects);
        tmp.Rects = NULL;
    }

    // End packing
    stbtt_PackEnd(&spc);
    ImGui::MemFree(buf_rects);
    buf_rects = NULL;

    // Third pass: setup ImFont and glyphs for runtime
    for (int input_i = 0; input_i < ConfigData.Size; input_i++)
    {
        ImFontConfig& cfg = ConfigData[input_i];
        ImFontTempBuildData& tmp = tmp_array[input_i];
        ImFont* dst_font = cfg.DstFont;

        float font_scale = stbtt_ScaleForPixelHeight(&tmp.FontInfo, cfg.SizePixels);
        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&tmp.FontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

        float ascent = unscaled_ascent * font_scale;
        float descent = unscaled_descent * font_scale;
        if (!cfg.MergeMode)
        {
            dst_font->ContainerAtlas = this;
            dst_font->ConfigData = &cfg;
            dst_font->ConfigDataCount = 0;
            dst_font->FontSize = cfg.SizePixels;
            dst_font->Ascent = ascent;
            dst_font->Descent = descent;
            dst_font->Glyphs.resize(0);
        }
        dst_font->ConfigDataCount++;
        float off_y = (cfg.MergeMode && cfg.MergeGlyphCenterV) ? (ascent - dst_font->Ascent) * 0.5f : 0.0f;

        dst_font->FallbackGlyph = NULL; // Always clear fallback so FindGlyph can return NULL. It will be set again in BuildLookupTable()
        for (int i = 0; i < tmp.RangesCount; i++)
        {
            stbtt_pack_range& range = tmp.Ranges[i];
            for (int char_idx = 0; char_idx < range.num_chars_in_range; char_idx += 1)
            {
                const stbtt_packedchar& pc = range.chardata_for_range[char_idx];
                if (!pc.x0 && !pc.x1 && !pc.y0 && !pc.y1)
                    continue;

                const int codepoint = range.first_unicode_char_in_range + char_idx;
                if (cfg.MergeMode && dst_font->FindGlyph((unsigned short)codepoint))
                    continue;

                stbtt_aligned_quad q;
                float dummy_x = 0.0f, dummy_y = 0.0f;
                stbtt_GetPackedQuad(range.chardata_for_range, TexWidth, TexHeight, char_idx, &dummy_x, &dummy_y, &q, 0);

                dst_font->Glyphs.resize(dst_font->Glyphs.Size + 1);
                ImFont::Glyph& glyph = dst_font->Glyphs.back();
                glyph.Codepoint = (ImWchar)codepoint;
                glyph.X0 = q.x0; glyph.Y0 = q.y0; glyph.X1 = q.x1; glyph.Y1 = q.y1;                
                glyph.U0 = q.s0; glyph.V0 = q.t0; glyph.U1 = q.s1; glyph.V1 = q.t1;
                glyph.Y0 += (float)(int)(dst_font->Ascent + off_y + 0.5f);
                glyph.Y1 += (float)(int)(dst_font->Ascent + off_y + 0.5f);
                glyph.XAdvance = (pc.xadvance + cfg.GlyphExtraSpacing.x);  // Bake spacing into XAdvance
                if (cfg.PixelSnapH)
                    glyph.XAdvance = (float)(int)(glyph.XAdvance + 0.5f);
            }
        }
        cfg.DstFont->BuildLookupTable();
    }

    // Cleanup temporaries
    ImGui::MemFree(buf_packedchars);
    ImGui::MemFree(buf_ranges);
    ImGui::MemFree(tmp_array);

    // Render into our custom data block
    RenderCustomTexData(1, &extra_rects);

    return true;
}

void ImFontAtlas::RenderCustomTexData(int pass, void* p_rects)
{
    // . = white layer, X = black layer, others are blank
    const int TEX_DATA_W = 90;
    const int TEX_DATA_H = 27;
    const char texture_data[TEX_DATA_W*TEX_DATA_H+1] =
    {
        "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX"
        "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X"
        "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X"
        "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X"
        "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X"
        "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X"
        "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX"
        "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      "
        "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       "
        "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        "
        "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         "
        "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          "
        "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           "
        "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            "
        "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           "
        "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          "
        "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          "
        "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       ------------------------------------"
        "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           "
        "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           "
        "      X..X          -  X...X  -         X...X         -  X..X           X..X  -           "
        "       XX           -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           "
        "------------        -    X    -           X           -X.....................X-           "
        "                    ----------------------------------- X...XXXXXXXXXXXXX...X -           "
        "                                                      -  X..X           X..X  -           "
        "                                                      -   X.X           X.X   -           "
        "                                                      -    XX           XX    -           "
    };

    ImVector<stbrp_rect>& rects = *(ImVector<stbrp_rect>*)p_rects;
    if (pass == 0)
    {
        stbrp_rect r;
        memset(&r, 0, sizeof(r));
        r.w = (TEX_DATA_W*2)+1;
        r.h = TEX_DATA_H+1;
        rects.push_back(r);
    }
    else if (pass == 1)
    {
        // Copy pixels
        const stbrp_rect& r = rects[0];
        for (int y = 0, n = 0; y < TEX_DATA_H; y++)
            for (int x = 0; x < TEX_DATA_W; x++, n++)
            {
                const int offset0 = (int)(r.x + x) + (int)(r.y + y) * TexWidth;
                const int offset1 = offset0 + 1 + TEX_DATA_W;
                TexPixelsAlpha8[offset0] = texture_data[n] == '.' ? 0xFF : 0x00;
                TexPixelsAlpha8[offset1] = texture_data[n] == 'X' ? 0xFF : 0x00;
            }
            const ImVec2 tex_uv_scale(1.0f / TexWidth, 1.0f / TexHeight);
            TexUvWhitePixel = ImVec2((r.x + 0.5f) * tex_uv_scale.x, (r.y + 0.5f) * tex_uv_scale.y);

            const ImVec2 cursor_datas[ImGuiMouseCursor_Count_][3] =
            {
                // Pos ........ Size ......... Offset ......
                { ImVec2(0,3),  ImVec2(12,19), ImVec2( 0, 0) }, // ImGuiMouseCursor_Arrow
                { ImVec2(13,0), ImVec2(7,16),  ImVec2( 4, 8) }, // ImGuiMouseCursor_TextInput
                { ImVec2(31,0), ImVec2(23,23), ImVec2(11,11) }, // ImGuiMouseCursor_Move
                { ImVec2(21,0), ImVec2( 9,23), ImVec2( 5,11) }, // ImGuiMouseCursor_ResizeNS
                { ImVec2(55,18),ImVec2(23, 9), ImVec2(11, 5) }, // ImGuiMouseCursor_ResizeEW
                { ImVec2(73,0), ImVec2(17,17), ImVec2( 9, 9) }, // ImGuiMouseCursor_ResizeNESW
                { ImVec2(55,0), ImVec2(17,17), ImVec2( 9, 9) }, // ImGuiMouseCursor_ResizeNWSE
            };

            for (int type = 0; type < ImGuiMouseCursor_Count_; type++)
            {
                ImGuiMouseCursorData& cursor_data = GImGui->MouseCursorData[type];
                ImVec2 pos = cursor_datas[type][0] + ImVec2((float)r.x, (float)r.y);
                const ImVec2 size = cursor_datas[type][1];
                cursor_data.Type = type;
                cursor_data.Size = size;
                cursor_data.HotOffset = cursor_datas[type][2];
                cursor_data.TexUvMin[0] = (pos) * tex_uv_scale;
                cursor_data.TexUvMax[0] = (pos + size) * tex_uv_scale;
                pos.x += TEX_DATA_W+1;
                cursor_data.TexUvMin[1] = (pos) * tex_uv_scale;
                cursor_data.TexUvMax[1] = (pos + size) * tex_uv_scale;
            }
    }
}

// Retrieve list of range (2 int per range, values are inclusive)
const ImWchar*   ImFontAtlas::GetGlyphRangesDefault()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesChinese()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesJapanese()
{
    // Store the 1946 ideograms code points as successive offsets from the initial unicode codepoint 0x4E00. Each offset has an implicit +1.
    // This encoding helps us reduce the source code size.
    static const short offsets_from_0x4E00[] = 
    {
        -1,0,1,3,0,0,0,0,1,0,5,1,1,0,7,4,6,10,0,1,9,9,7,1,3,19,1,10,7,1,0,1,0,5,1,0,6,4,2,6,0,0,12,6,8,0,3,5,0,1,0,9,0,0,8,1,1,3,4,5,13,0,0,8,2,17,
        4,3,1,1,9,6,0,0,0,2,1,3,2,22,1,9,11,1,13,1,3,12,0,5,9,2,0,6,12,5,3,12,4,1,2,16,1,1,4,6,5,3,0,6,13,15,5,12,8,14,0,0,6,15,3,6,0,18,8,1,6,14,1,
        5,4,12,24,3,13,12,10,24,0,0,0,1,0,1,1,2,9,10,2,2,0,0,3,3,1,0,3,8,0,3,2,4,4,1,6,11,10,14,6,15,3,4,15,1,0,0,5,2,2,0,0,1,6,5,5,6,0,3,6,5,0,0,1,0,
        11,2,2,8,4,7,0,10,0,1,2,17,19,3,0,2,5,0,6,2,4,4,6,1,1,11,2,0,3,1,2,1,2,10,7,6,3,16,0,8,24,0,0,3,1,1,3,0,1,6,0,0,0,2,0,1,5,15,0,1,0,0,2,11,19,
        1,4,19,7,6,5,1,0,0,0,0,5,1,0,1,9,0,0,5,0,2,0,1,0,3,0,11,3,0,2,0,0,0,0,0,9,3,6,4,12,0,14,0,0,29,10,8,0,14,37,13,0,31,16,19,0,8,30,1,20,8,3,48,
        21,1,0,12,0,10,44,34,42,54,11,18,82,0,2,1,2,12,1,0,6,2,17,2,12,7,0,7,17,4,2,6,24,23,8,23,39,2,16,23,1,0,5,1,2,15,14,5,6,2,11,0,8,6,2,2,2,14,
        20,4,15,3,4,11,10,10,2,5,2,1,30,2,1,0,0,22,5,5,0,3,1,5,4,1,0,0,2,2,21,1,5,1,2,16,2,1,3,4,0,8,4,0,0,5,14,11,2,16,1,13,1,7,0,22,15,3,1,22,7,14,
        22,19,11,24,18,46,10,20,64,45,3,2,0,4,5,0,1,4,25,1,0,0,2,10,0,0,0,1,0,1,2,0,0,9,1,2,0,0,0,2,5,2,1,1,5,5,8,1,1,1,5,1,4,9,1,3,0,1,0,1,1,2,0,0,
        2,0,1,8,22,8,1,0,0,0,0,4,2,1,0,9,8,5,0,9,1,30,24,2,6,4,39,0,14,5,16,6,26,179,0,2,1,1,0,0,0,5,2,9,6,0,2,5,16,7,5,1,1,0,2,4,4,7,15,13,14,0,0,
        3,0,1,0,0,0,2,1,6,4,5,1,4,9,0,3,1,8,0,0,10,5,0,43,0,2,6,8,4,0,2,0,0,9,6,0,9,3,1,6,20,14,6,1,4,0,7,2,3,0,2,0,5,0,3,1,0,3,9,7,0,3,4,0,4,9,1,6,0,
        9,0,0,2,3,10,9,28,3,6,2,4,1,2,32,4,1,18,2,0,3,1,5,30,10,0,2,2,2,0,7,9,8,11,10,11,7,2,13,7,5,10,0,3,40,2,0,1,6,12,0,4,5,1,5,11,11,21,4,8,3,7,
        8,8,33,5,23,0,0,19,8,8,2,3,0,6,1,1,1,5,1,27,4,2,5,0,3,5,6,3,1,0,3,1,12,5,3,3,2,0,7,7,2,1,0,4,0,1,1,2,0,10,10,6,2,5,9,7,5,15,15,21,6,11,5,20,
        4,3,5,5,2,5,0,2,1,0,1,7,28,0,9,0,5,12,5,5,18,30,0,12,3,3,21,16,25,32,9,3,14,11,24,5,66,9,1,2,0,5,9,1,5,1,8,0,8,3,3,0,1,15,1,4,8,1,2,7,0,7,2,
        8,3,7,5,3,7,10,2,1,0,0,2,25,0,6,4,0,10,0,4,2,4,1,12,5,38,4,0,4,1,10,5,9,4,0,14,4,2,5,18,20,21,1,3,0,5,0,7,0,3,7,1,3,1,1,8,1,0,0,0,3,2,5,2,11,
        6,0,13,1,3,9,1,12,0,16,6,2,1,0,2,1,12,6,13,11,2,0,28,1,7,8,14,13,8,13,0,2,0,5,4,8,10,2,37,42,19,6,6,7,4,14,11,18,14,80,7,6,0,4,72,12,36,27,
        7,7,0,14,17,19,164,27,0,5,10,7,3,13,6,14,0,2,2,5,3,0,6,13,0,0,10,29,0,4,0,3,13,0,3,1,6,51,1,5,28,2,0,8,0,20,2,4,0,25,2,10,13,10,0,16,4,0,1,0,
        2,1,7,0,1,8,11,0,0,1,2,7,2,23,11,6,6,4,16,2,2,2,0,22,9,3,3,5,2,0,15,16,21,2,9,20,15,15,5,3,9,1,0,0,1,7,7,5,4,2,2,2,38,24,14,0,0,15,5,6,24,14,
        5,5,11,0,21,12,0,3,8,4,11,1,8,0,11,27,7,2,4,9,21,59,0,1,39,3,60,62,3,0,12,11,0,3,30,11,0,13,88,4,15,5,28,13,1,4,48,17,17,4,28,32,46,0,16,0,
        18,11,1,8,6,38,11,2,6,11,38,2,0,45,3,11,2,7,8,4,30,14,17,2,1,1,65,18,12,16,4,2,45,123,12,56,33,1,4,3,4,7,0,0,0,3,2,0,16,4,2,4,2,0,7,4,5,2,26,
        2,25,6,11,6,1,16,2,6,17,77,15,3,35,0,1,0,5,1,0,38,16,6,3,12,3,3,3,0,9,3,1,3,5,2,9,0,18,0,25,1,3,32,1,72,46,6,2,7,1,3,14,17,0,28,1,40,13,0,20,
        15,40,6,38,24,12,43,1,1,9,0,12,6,0,6,2,4,19,3,7,1,48,0,9,5,0,5,6,9,6,10,15,2,11,19,3,9,2,0,1,10,1,27,8,1,3,6,1,14,0,26,0,27,16,3,4,9,6,2,23,
        9,10,5,25,2,1,6,1,1,48,15,9,15,14,3,4,26,60,29,13,37,21,1,6,4,0,2,11,22,23,16,16,2,2,1,3,0,5,1,6,4,0,0,4,0,0,8,3,0,2,5,0,7,1,7,3,13,2,4,10,
        3,0,2,31,0,18,3,0,12,10,4,1,0,7,5,7,0,5,4,12,2,22,10,4,2,15,2,8,9,0,23,2,197,51,3,1,1,4,13,4,3,21,4,19,3,10,5,40,0,4,1,1,10,4,1,27,34,7,21,
        2,17,2,9,6,4,2,3,0,4,2,7,8,2,5,1,15,21,3,4,4,2,2,17,22,1,5,22,4,26,7,0,32,1,11,42,15,4,1,2,5,0,19,3,1,8,6,0,10,1,9,2,13,30,8,2,24,17,19,1,4,
        4,25,13,0,10,16,11,39,18,8,5,30,82,1,6,8,18,77,11,13,20,75,11,112,78,33,3,0,0,60,17,84,9,1,1,12,30,10,49,5,32,158,178,5,5,6,3,3,1,3,1,4,7,6,
        19,31,21,0,2,9,5,6,27,4,9,8,1,76,18,12,1,4,0,3,3,6,3,12,2,8,30,16,2,25,1,5,5,4,3,0,6,10,2,3,1,0,5,1,19,3,0,8,1,5,2,6,0,0,0,19,1,2,0,5,1,2,5,
        1,3,7,0,4,12,7,3,10,22,0,9,5,1,0,2,20,1,1,3,23,30,3,9,9,1,4,191,14,3,15,6,8,50,0,1,0,0,4,0,0,1,0,2,4,2,0,2,3,0,2,0,2,2,8,7,0,1,1,1,3,3,17,11,
        91,1,9,3,2,13,4,24,15,41,3,13,3,1,20,4,125,29,30,1,0,4,12,2,21,4,5,5,19,11,0,13,11,86,2,18,0,7,1,8,8,2,2,22,1,2,6,5,2,0,1,2,8,0,2,0,5,2,1,0,
        2,10,2,0,5,9,2,1,2,0,1,0,4,0,0,10,2,5,3,0,6,1,0,1,4,4,33,3,13,17,3,18,6,4,7,1,5,78,0,4,1,13,7,1,8,1,0,35,27,15,3,0,0,0,1,11,5,41,38,15,22,6,
        14,14,2,1,11,6,20,63,5,8,27,7,11,2,2,40,58,23,50,54,56,293,8,8,1,5,1,14,0,1,12,37,89,8,8,8,2,10,6,0,0,0,4,5,2,1,0,1,1,2,7,0,3,3,0,4,6,0,3,2,
        19,3,8,0,0,0,4,4,16,0,4,1,5,1,3,0,3,4,6,2,17,10,10,31,6,4,3,6,10,126,7,3,2,2,0,9,0,0,5,20,13,0,15,0,6,0,2,5,8,64,50,3,2,12,2,9,0,0,11,8,20,
        109,2,18,23,0,0,9,61,3,0,28,41,77,27,19,17,81,5,2,14,5,83,57,252,14,154,263,14,20,8,13,6,57,39,38,
    };
    static int ranges_unpacked = false;
    static ImWchar ranges[8 + IM_ARRAYSIZE(offsets_from_0x4E00)*2 + 1] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
    };
    if (!ranges_unpacked)
    {
        // Unpack
        int codepoint = 0x4e00;
        ImWchar* dst = &ranges[8];
        for (int n = 0; n < IM_ARRAYSIZE(offsets_from_0x4E00); n++, dst += 2)
            dst[0] = dst[1] = (ImWchar)(codepoint += (offsets_from_0x4E00[n] + 1));
        dst[0] = 0;
        ranges_unpacked = true;
    }
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesCyrillic()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    return &ranges[0];
}

//-----------------------------------------------------------------------------
// ImFont
//-----------------------------------------------------------------------------

ImFont::ImFont()
{
    Scale = 1.0f;
    FallbackChar = (ImWchar)'?';
    Clear();
}

ImFont::~ImFont()
{
    // Invalidate active font so that the user gets a clear crash instead of a dangling pointer.
    // If you want to delete fonts you need to do it between Render() and NewFrame().
    // FIXME-CLEANUP
    /*
    ImGuiState& g = *GImGui;
    if (g.Font == this)
        g.Font = NULL;
    */
    Clear();
}

void    ImFont::Clear()
{
    FontSize = 0.0f;
    DisplayOffset = ImVec2(0.0f, 1.0f);
    ConfigData = NULL;
    ConfigDataCount = 0;
    Ascent = Descent = 0.0f;
    ContainerAtlas = NULL;
    Glyphs.clear();
    FallbackGlyph = NULL;
    FallbackXAdvance = 0.0f;
    IndexXAdvance.clear();
    IndexLookup.clear();
}

void ImFont::BuildLookupTable()
{
    int max_codepoint = 0;
    for (int i = 0; i != Glyphs.Size; i++)
        max_codepoint = ImMax(max_codepoint, (int)Glyphs[i].Codepoint);

    IndexXAdvance.clear();
    IndexXAdvance.resize(max_codepoint + 1);
    IndexLookup.clear();
    IndexLookup.resize(max_codepoint + 1);
    for (int i = 0; i < max_codepoint + 1; i++)
    {
        IndexXAdvance[i] = -1.0f;
        IndexLookup[i] = -1;
    }
    for (int i = 0; i < Glyphs.Size; i++)
    {
        int codepoint = (int)Glyphs[i].Codepoint;
        IndexXAdvance[codepoint] = Glyphs[i].XAdvance;
        IndexLookup[codepoint] = i;
    }

    // Create a glyph to handle TAB
    // FIXME: Needs proper TAB handling but it needs to be contextualized (can arbitrary say that each string starts at "column 0"
    if (FindGlyph((unsigned short)' '))
    {
        if (Glyphs.back().Codepoint != '\t')   // So we can call this function multiple times
            Glyphs.resize(Glyphs.Size + 1);
        ImFont::Glyph& tab_glyph = Glyphs.back();
        tab_glyph = *FindGlyph((unsigned short)' ');
        tab_glyph.Codepoint = '\t';
        tab_glyph.XAdvance *= 4;
        IndexXAdvance[(int)tab_glyph.Codepoint] = (float)tab_glyph.XAdvance;
        IndexLookup[(int)tab_glyph.Codepoint] = (int)(Glyphs.Size-1);
    }

    FallbackGlyph = NULL;
    FallbackGlyph = FindGlyph(FallbackChar);
    FallbackXAdvance = FallbackGlyph ? FallbackGlyph->XAdvance : 0.0f;
    for (int i = 0; i < max_codepoint + 1; i++)
        if (IndexXAdvance[i] < 0.0f)
            IndexXAdvance[i] = FallbackXAdvance;
}

void ImFont::SetFallbackChar(ImWchar c)
{
    FallbackChar = c;
    BuildLookupTable();
}

const ImFont::Glyph* ImFont::FindGlyph(unsigned short c) const
{
    if (c < IndexLookup.Size)
    {
        const int i = IndexLookup[c];
        if (i != -1)
            return &Glyphs[i];
    }
    return FallbackGlyph;
}

const char* ImFont::CalcWordWrapPositionA(float scale, const char* text, const char* text_end, float wrap_width) const
{
    // Simple word-wrapping for English, not full-featured. Please submit failing cases!
    // FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;

    const char* word_end = text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = text;
    while (s < text_end)
    {
        unsigned int c = (unsigned int)*s;
        const char* next_s;
        if (c < 0x80)
            next_s = s + 1;
        else
            next_s = s + ImTextCharFromUtf8(&c, s, text_end);
        if (c == 0)
            break;

        if (c < 32)
        {
            if (c == '\n')
            {
                line_width = word_width = blank_width = 0.0f;
                inside_word = true;
                s = next_s;
                continue;
            }
            if (c == '\r')
            {
                s = next_s;
                continue;
            }
        }

        const float char_width = ((int)c < IndexXAdvance.Size) ? IndexXAdvance[(int)c] * scale : FallbackXAdvance;
        if (ImCharIsSpace(c))
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
            }
            blank_width += char_width;
            inside_word = false;
        }
        else
        {
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            }
            else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = !(c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width >= wrap_width)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < wrap_width)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    return s;
}

ImVec2 ImFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** remaining) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin); // FIXME-OPT: Need to avoid this.

    const float line_height = size;
    const float scale = size / FontSize;

    ImVec2 text_size = ImVec2(0,0);
    float line_width = 0.0f;

    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    const char* s = text_begin;
    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - line_width);
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                if (text_size.x < line_width)
                    text_size.x = line_width;
                text_size.y += line_height;
                line_width = 0.0f;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (ImCharIsSpace(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source (handle unlikely UTF-8 decoding failure by skipping to the next byte)
        const char* prev_s = s;
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        }
        else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0)
                break;
        }

        if (c < 32)
        {
            if (c == '\n')
            {
                text_size.x = ImMax(text_size.x, line_width);
                text_size.y += line_height;
                line_width = 0.0f;
                continue;
            }
            if (c == '\r')
                continue;
        }

        const float char_width = ((int)c < IndexXAdvance.Size ? IndexXAdvance[(int)c] : FallbackXAdvance) * scale;
        if (line_width + char_width >= max_width)
        {
            s = prev_s;
            break;
        }

        line_width += char_width;
    }

    if (text_size.x < line_width)
        text_size.x = line_width;

    if (line_width > 0 || text_size.y == 0.0f)
        text_size.y += line_height;

    if (remaining)
        *remaining = s;

    return text_size;
}

void ImFont::RenderText(float size, ImVec2 pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, ImDrawList* draw_list, float wrap_width, bool cpu_fine_clip) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin);

    // Align to be pixel perfect
    pos.x = (float)(int)pos.x + DisplayOffset.x;
    pos.y = (float)(int)pos.y + DisplayOffset.y;
    float x = pos.x;
    float y = pos.y;
    if (y > clip_rect.w)
        return;

    const float scale = size / FontSize;
    const float line_height = FontSize * scale;
    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    ImDrawVert* vtx_write = draw_list->_VtxWritePtr;
    ImDrawIdx* idx_write = draw_list->_IdxWritePtr;
    unsigned int vtx_current_idx = draw_list->_VtxCurrentIdx;

    const char* s = text_begin;
    if (!word_wrap_enabled && y + line_height < clip_rect.y)
        while (s < text_end && *s != '\n')  // Fast-forward to next line
            s++;
    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - pos.x));
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                x = pos.x;
                y += line_height;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (ImCharIsSpace(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source (handle unlikely UTF-8 decoding failure by skipping to the next byte)
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        }
        else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0)
                break;
        }

        if (c < 32)
        {
            if (c == '\n')
            {
                x = pos.x;
                y += line_height;

                if (y > clip_rect.w)
                    break;
                if (!word_wrap_enabled && y + line_height < clip_rect.y)
                    while (s < text_end && *s != '\n')  // Fast-forward to next line
                        s++;
                continue;
            }
            if (c == '\r')
                continue;
        }

        float char_width = 0.0f;
        if (const Glyph* glyph = FindGlyph((unsigned short)c))
        {
            char_width = glyph->XAdvance * scale;

            // Clipping on Y is more likely
            if (c != ' ' && c != '\t')
            {
                // We don't do a second finer clipping test on the Y axis (todo: do some measurement see if it is worth it, probably not)
                float y1 = (float)(y + glyph->Y0 * scale);
                float y2 = (float)(y + glyph->Y1 * scale);

                float x1 = (float)(x + glyph->X0 * scale);
                float x2 = (float)(x + glyph->X1 * scale);
                if (x1 <= clip_rect.z && x2 >= clip_rect.x)
                {
                    // Render a character
                    float u1 = glyph->U0;
                    float v1 = glyph->V0;
                    float u2 = glyph->U1;
                    float v2 = glyph->V1;

                    // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads
                    if (cpu_fine_clip)
                    {
                        if (x1 < clip_rect.x)
                        {
                            u1 = u1 + (1.0f - (x2 - clip_rect.x) / (x2 - x1)) * (u2 - u1);
                            x1 = clip_rect.x;
                        }
                        if (y1 < clip_rect.y)
                        {
                            v1 = v1 + (1.0f - (y2 - clip_rect.y) / (y2 - y1)) * (v2 - v1);
                            y1 = clip_rect.y;
                        }
                        if (x2 > clip_rect.z)
                        {
                            u2 = u1 + ((clip_rect.z - x1) / (x2 - x1)) * (u2 - u1);
                            x2 = clip_rect.z;
                        }
                        if (y2 > clip_rect.w)
                        {
                            v2 = v1 + ((clip_rect.w - y1) / (y2 - y1)) * (v2 - v1);
                            y2 = clip_rect.w;
                        }
                        if (y1 >= y2)
                        {
                            x += char_width;
                            continue;
                        }
                    }

                    // NB: we are not calling PrimRectUV() here because non-inlined causes too much overhead in a debug build.
                    // inlined:
                    {
                        idx_write[0] = (ImDrawIdx)(vtx_current_idx); idx_write[1] = (ImDrawIdx)(vtx_current_idx+1); idx_write[2] = (ImDrawIdx)(vtx_current_idx+2); 
                        idx_write[3] = (ImDrawIdx)(vtx_current_idx); idx_write[4] = (ImDrawIdx)(vtx_current_idx+2); idx_write[5] = (ImDrawIdx)(vtx_current_idx+3); 
                        vtx_write[0].pos.x = x1; vtx_write[0].pos.y = y1; vtx_write[0].col = col; vtx_write[0].uv.x = u1; vtx_write[0].uv.y = v1;
                        vtx_write[1].pos.x = x2; vtx_write[1].pos.y = y1; vtx_write[1].col = col; vtx_write[1].uv.x = u2; vtx_write[1].uv.y = v1;
                        vtx_write[2].pos.x = x2; vtx_write[2].pos.y = y2; vtx_write[2].col = col; vtx_write[2].uv.x = u2; vtx_write[2].uv.y = v2;
                        vtx_write[3].pos.x = x1; vtx_write[3].pos.y = y2; vtx_write[3].col = col; vtx_write[3].uv.x = u1; vtx_write[3].uv.y = v2;
                        vtx_write += 4;
                        vtx_current_idx += 4;
                        idx_write += 6;
                    }
                }
            }
        }

        x += char_width;
    }

    draw_list->_VtxWritePtr = vtx_write;
    draw_list->_VtxCurrentIdx = vtx_current_idx;
    draw_list->_IdxWritePtr = idx_write;
}

//-----------------------------------------------------------------------------
// DEFAULT FONT DATA
//-----------------------------------------------------------------------------
// Compressed with stb_compress() then converted to a C array.
// Use the program in extra_fonts/binary_to_compressed_c.cpp to create the array from a TTF file.
// Decompression from stb.h (public domain) by Sean Barrett https://github.com/nothings/stb/blob/master/stb.h
//-----------------------------------------------------------------------------

static unsigned int stb_decompress_length(unsigned char *input)
{
    return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

static unsigned char *stb__barrier, *stb__barrier2, *stb__barrier3, *stb__barrier4;
static unsigned char *stb__dout;
static void stb__match(unsigned char *data, unsigned int length)
{
    // INVERSE of memmove... write each byte before copying the next...
    IM_ASSERT (stb__dout + length <= stb__barrier);
    if (stb__dout + length > stb__barrier) { stb__dout += length; return; }
    if (data < stb__barrier4) { stb__dout = stb__barrier+1; return; }
    while (length--) *stb__dout++ = *data++;
}

static void stb__lit(unsigned char *data, unsigned int length)
{
    IM_ASSERT (stb__dout + length <= stb__barrier);
    if (stb__dout + length > stb__barrier) { stb__dout += length; return; }
    if (data < stb__barrier2) { stb__dout = stb__barrier+1; return; }
    memcpy(stb__dout, data, length);
    stb__dout += length;
}

#define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
#define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
#define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

static unsigned char *stb_decompress_token(unsigned char *i)
{
    if (*i >= 0x20) { // use fewer if's for cases that expand small
        if (*i >= 0x80)       stb__match(stb__dout-i[1]-1, i[0] - 0x80 + 1), i += 2;
        else if (*i >= 0x40)  stb__match(stb__dout-(stb__in2(0) - 0x4000 + 1), i[2]+1), i += 3;
        else /* *i >= 0x20 */ stb__lit(i+1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
    } else { // more ifs for cases that expand large, since overhead is amortized
        if (*i >= 0x18)       stb__match(stb__dout-(stb__in3(0) - 0x180000 + 1), i[3]+1), i += 4;
        else if (*i >= 0x10)  stb__match(stb__dout-(stb__in3(0) - 0x100000 + 1), stb__in2(3)+1), i += 5;
        else if (*i >= 0x08)  stb__lit(i+2, stb__in2(0) - 0x0800 + 1), i += 2 + (stb__in2(0) - 0x0800 + 1);
        else if (*i == 0x07)  stb__lit(i+3, stb__in2(1) + 1), i += 3 + (stb__in2(1) + 1);
        else if (*i == 0x06)  stb__match(stb__dout-(stb__in3(1)+1), i[4]+1), i += 5;
        else if (*i == 0x04)  stb__match(stb__dout-(stb__in3(1)+1), stb__in2(4)+1), i += 6;
    }
    return i;
}

static unsigned int stb_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
    const unsigned long ADLER_MOD = 65521;
    unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
    unsigned long blocklen, i;

    blocklen = buflen % 5552;
    while (buflen) {
        for (i=0; i + 7 < blocklen; i += 8) {
            s1 += buffer[0], s2 += s1;
            s1 += buffer[1], s2 += s1;
            s1 += buffer[2], s2 += s1;
            s1 += buffer[3], s2 += s1;
            s1 += buffer[4], s2 += s1;
            s1 += buffer[5], s2 += s1;
            s1 += buffer[6], s2 += s1;
            s1 += buffer[7], s2 += s1;

            buffer += 8;
        }

        for (; i < blocklen; ++i)
            s1 += *buffer++, s2 += s1;

        s1 %= ADLER_MOD, s2 %= ADLER_MOD;
        buflen -= blocklen;
        blocklen = 5552;
    }
    return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static unsigned int stb_decompress(unsigned char *output, unsigned char *i, unsigned int length)
{
    unsigned int olen;
    if (stb__in4(0) != 0x57bC0000) return 0;
    if (stb__in4(4) != 0)          return 0; // error! stream is > 4GB
    olen = stb_decompress_length(i);
    stb__barrier2 = i;
    stb__barrier3 = i+length;
    stb__barrier = output + olen;
    stb__barrier4 = output;
    i += 16;

    stb__dout = output;
    for (;;) {
        unsigned char *old_i = i;
        i = stb_decompress_token(i);
        if (i == old_i) {
            if (*i == 0x05 && i[1] == 0xfa) {
                IM_ASSERT(stb__dout == output + olen);
                if (stb__dout != output + olen) return 0;
                if (stb_adler32(1, output, olen) != (unsigned int) stb__in4(2))
                    return 0;
                return olen;
            } else {
                IM_ASSERT(0); /* NOTREACHED */
                return 0;
            }
        }
        IM_ASSERT(stb__dout <= output + olen); 
        if (stb__dout > output + olen)
            return 0;
    }
}

//-----------------------------------------------------------------------------
// ProggyClean.ttf
// Copyright (c) 2004, 2005 Tristan Grimmer
// MIT license (see License.txt in http://www.upperbounds.net/download/ProggyClean.ttf.zip)
// Download and more information at http://upperbounds.net
//-----------------------------------------------------------------------------
static const unsigned int proggy_clean_ttf_compressed_size = 9583;
static const unsigned int proggy_clean_ttf_compressed_data[9584/4] =
{
    0x0000bc57, 0x00000000, 0xf8a00000, 0x00000400, 0x00010037, 0x000c0000, 0x00030080, 0x2f534f40, 0x74eb8832, 0x01000090, 0x2c158248, 0x616d634e, 
    0x23120270, 0x03000075, 0x241382a0, 0x74766352, 0x82178220, 0xfc042102, 0x02380482, 0x66796c67, 0x5689af12, 0x04070000, 0x80920000, 0x64616568, 
    0xd36691d7, 0xcc201b82, 0x36210382, 0x27108268, 0xc3014208, 0x04010000, 0x243b0f82, 0x78746d68, 0x807e008a, 0x98010000, 0x06020000, 0x61636f6c, 
    0xd8b0738c, 0x82050000, 0x0402291e, 0x7078616d, 0xda00ae01, 0x28201f82, 0x202c1082, 0x656d616e, 0x96bb5925, 0x84990000, 0x9e2c1382, 0x74736f70, 
    0xef83aca6, 0x249b0000, 0xd22c3382, 0x70657270, 0x12010269, 0xf4040000, 0x08202f82, 0x012ecb84, 0x553c0000, 0x0f5fd5e9, 0x0300f53c, 0x00830008, 
    0x7767b722, 0x002b3f82, 0xa692bd00, 0xfe0000d7, 0x83800380, 0x21f1826f, 0x00850002, 0x41820120, 0x40fec026, 0x80030000, 0x05821083, 0x07830120, 
    0x0221038a, 0x24118200, 0x90000101, 0x82798200, 0x00022617, 0x00400008, 0x2009820a, 0x82098276, 0x82002006, 0x9001213b, 0x0223c883, 0x828a02bc, 
    0x858f2010, 0xc5012507, 0x00023200, 0x04210083, 0x91058309, 0x6c412b03, 0x40007374, 0xac200000, 0x00830008, 0x01000523, 0x834d8380, 0x80032103, 
    0x012101bf, 0x23b88280, 0x00800000, 0x0b830382, 0x07820120, 0x83800021, 0x88012001, 0x84002009, 0x2005870f, 0x870d8301, 0x2023901b, 0x83199501, 
    0x82002015, 0x84802000, 0x84238267, 0x88002027, 0x8561882d, 0x21058211, 0x13880000, 0x01800022, 0x05850d85, 0x0f828020, 0x03208384, 0x03200582, 
    0x47901b84, 0x1b850020, 0x1f821d82, 0x3f831d88, 0x3f410383, 0x84058405, 0x210982cd, 0x09830000, 0x03207789, 0xf38a1384, 0x01203782, 0x13872384, 
    0x0b88c983, 0x0d898f84, 0x00202982, 0x23900383, 0x87008021, 0x83df8301, 0x86118d03, 0x863f880d, 0x8f35880f, 0x2160820f, 0x04830300, 0x1c220382, 
    0x05820100, 0x4c000022, 0x09831182, 0x04001c24, 0x11823000, 0x0800082e, 0x00000200, 0xff007f00, 0xffffac20, 0x00220982, 0x09848100, 0xdf216682, 
    0x843586d5, 0x06012116, 0x04400684, 0xa58120d7, 0x00b127d8, 0x01b88d01, 0x2d8685ff, 0xc100c621, 0xf4be0801, 0x9e011c01, 0x88021402, 0x1403fc02, 
    0x9c035803, 0x1404de03, 0x50043204, 0xa2046204, 0x66051605, 0x1206bc05, 0xd6067406, 0x7e073807, 0x4e08ec07, 0x96086c08, 0x1009d008, 0x88094a09, 
    0x800a160a, 0x560b040b, 0x2e0cc80b, 0xea0c820c, 0xa40d5e0d, 0x500eea0d, 0x280f960e, 0x1210b00f, 0xe0107410, 0xb6115211, 0x6e120412, 0x4c13c412, 
    0xf613ac13, 0xae145814, 0x4015ea14, 0xa6158015, 0x1216b815, 0xc6167e16, 0x8e173417, 0x5618e017, 0xee18ba18, 0x96193619, 0x481ad419, 0xf01a9c1a, 
    0xc81b5c1b, 0x4c1c041c, 0xea1c961c, 0x921d2a1d, 0x401ed21d, 0xe01e8e1e, 0x761f241f, 0xa61fa61f, 0x01821020, 0x8a202e34, 0xc820b220, 0x74211421, 
    0xee219821, 0x86226222, 0x01820c23, 0x83238021, 0x23983c01, 0x24d823b0, 0x244a2400, 0x24902468, 0x250625ae, 0x25822560, 0x26f825f8, 0x82aa2658, 
    0xd8be0801, 0x9a274027, 0x68280a28, 0x0e29a828, 0xb8292029, 0x362af829, 0x602a602a, 0x2a2b022b, 0xac2b5e2b, 0x202ce62b, 0x9a2c342c, 0x5c2d282d, 
    0xaa2d782d, 0x262ee82d, 0x262fa62e, 0xf42fb62f, 0xc8305e30, 0xb4313e31, 0x9e321e32, 0x82331e33, 0x5c34ee33, 0x3a35ce34, 0xd4358635, 0x72362636, 
    0x7637e636, 0x3a38d837, 0x1239a638, 0xae397439, 0x9a3a2e3a, 0x7c3b063b, 0x3a3ce83b, 0x223d963c, 0xec3d863d, 0xc63e563e, 0x9a3f2a3f, 0x6a401240, 
    0x3641d040, 0x0842a241, 0x7a424042, 0xf042b842, 0xcc436243, 0x8a442a44, 0x5845ee44, 0xe245b645, 0xb4465446, 0x7a471447, 0x5448da47, 0x4049c648, 
    0x15462400, 0x034d0808, 0x0b000700, 0x13000f00, 0x1b001700, 0x23001f00, 0x2b002700, 0x33002f00, 0x3b003700, 0x43003f00, 0x4b004700, 0x53004f00, 
    0x5b005700, 0x63005f00, 0x6b006700, 0x73006f00, 0x7b007700, 0x83007f00, 0x8b008700, 0x00008f00, 0x15333511, 0x20039631, 0x20178205, 0xd3038221, 
    0x20739707, 0x25008580, 0x028080fc, 0x05be8080, 0x04204a85, 0x05ce0685, 0x0107002a, 0x02000080, 0x00000400, 0x250d8b41, 0x33350100, 0x03920715, 
    0x13820320, 0x858d0120, 0x0e8d0320, 0xff260d83, 0x00808000, 0x54820106, 0x04800223, 0x845b8c80, 0x41332059, 0x078b068f, 0x82000121, 0x82fe2039, 
    0x84802003, 0x83042004, 0x23598a0e, 0x00180000, 0x03210082, 0x42ab9080, 0x73942137, 0x2013bb41, 0x8f978205, 0x2027a39b, 0x20b68801, 0x84b286fd, 
    0x91c88407, 0x41032011, 0x11a51130, 0x15000027, 0x80ff8000, 0x11af4103, 0x841b0341, 0x8bd983fd, 0x9be99bc9, 0x8343831b, 0x21f1821f, 0xb58300ff, 
    0x0f84e889, 0xf78a0484, 0x8000ff22, 0x0020eeb3, 0x14200082, 0x2130ef41, 0xeb431300, 0x4133200a, 0xd7410ecb, 0x9a07200b, 0x2027871b, 0x21238221, 
    0xe7828080, 0xe784fd20, 0xe8848020, 0xfe808022, 0x08880d85, 0xba41fd20, 0x82248205, 0x85eab02a, 0x008022e7, 0x2cd74200, 0x44010021, 0xd34406eb, 
    0x44312013, 0xcf8b0eef, 0x0d422f8b, 0x82332007, 0x0001212f, 0x8023cf82, 0x83000180, 0x820583de, 0x830682d4, 0x820020d4, 0x82dc850a, 0x20e282e9, 
    0xb2ff85fe, 0x010327e9, 0x02000380, 0x0f440400, 0x0c634407, 0x68825982, 0x85048021, 0x260a825d, 0x010b0000, 0x4400ff00, 0x2746103f, 0x08d74209, 
    0x4d440720, 0x0eaf4406, 0xc3441d20, 0x23078406, 0xff800002, 0x04845b83, 0x8d05b241, 0x1781436f, 0x6b8c87a5, 0x1521878e, 0x06474505, 0x01210783, 
    0x84688c00, 0x8904828e, 0x441e8cf7, 0x0b270cff, 0x80008000, 0x45030003, 0xfb430fab, 0x080f4107, 0x410bf942, 0xd34307e5, 0x070d4207, 0x80800123, 
    0x205d85fe, 0x849183fe, 0x20128404, 0x82809702, 0x00002217, 0x41839a09, 0x6b4408cf, 0x0733440f, 0x3b460720, 0x82798707, 0x97802052, 0x0000296f, 
    0xff800004, 0x01800100, 0x0021ef89, 0x0a914625, 0x410a4d41, 0x00250ed4, 0x00050000, 0x056d4280, 0x210a7b46, 0x21481300, 0x46ed8512, 0x00210bd1, 
    0x89718202, 0x21738877, 0x2b850001, 0x00220582, 0x87450a00, 0x0ddb4606, 0x41079b42, 0x9d420c09, 0x0b09420b, 0x8d820720, 0x9742fc84, 0x42098909, 
    0x00241e0f, 0x00800014, 0x0b47da82, 0x0833442a, 0x49078d41, 0x2f450f13, 0x42278f17, 0x01200751, 0x22063742, 0x44808001, 0x20450519, 0x88068906, 
    0x83fe2019, 0x4203202a, 0x1a941a58, 0x00820020, 0xe7a40e20, 0x420ce146, 0x854307e9, 0x0fcb4713, 0xff20a182, 0xfe209b82, 0x0c867f8b, 0x0021aea4, 
    0x219fa40f, 0x7d41003b, 0x07194214, 0xbf440520, 0x071d4206, 0x6941a590, 0x80802309, 0x028900ff, 0xa9a4b685, 0xc5808021, 0x449b82ab, 0x152007eb, 
    0x42134d46, 0x61440a15, 0x051e4208, 0x222b0442, 0x47001100, 0xfd412913, 0x17194714, 0x410f5b41, 0x02220773, 0x09428080, 0x21a98208, 0xd4420001, 
    0x481c840d, 0x00232bc9, 0x42120000, 0xe74c261b, 0x149d4405, 0x07209d87, 0x410db944, 0x14421c81, 0x42fd2005, 0x80410bd2, 0x203d8531, 0x06874100, 
    0x48256f4a, 0xcb420c95, 0x13934113, 0x44075d44, 0x044c0855, 0x00ff2105, 0xfe228185, 0x45448000, 0x22c5b508, 0x410c0000, 0x7b412087, 0x1bb74514, 
    0x32429c85, 0x0a574805, 0x21208943, 0x8ba01300, 0x440dfb4e, 0x77431437, 0x245b4113, 0x200fb145, 0x41108ffe, 0x80203562, 0x00200082, 0x46362b42, 
    0x1742178d, 0x4527830f, 0x0f830b2f, 0x4a138146, 0x802409a1, 0xfe8000ff, 0x94419982, 0x09294320, 0x04000022, 0x49050f4f, 0xcb470a63, 0x48032008, 
    0x2b48067b, 0x85022008, 0x82638338, 0x00002209, 0x05af4806, 0x900e9f49, 0x84c5873f, 0x214285bd, 0x064900ff, 0x0c894607, 0x00000023, 0x4903820a, 
    0x714319f3, 0x0749410c, 0x8a07a145, 0x02152507, 0xfe808000, 0x74490386, 0x8080211b, 0x0c276f82, 0x00018000, 0x48028003, 0x2b2315db, 0x43002f00, 
    0x6f82142f, 0x44011521, 0x93510da7, 0x20e68508, 0x06494d80, 0x8e838020, 0x06821286, 0x124bff20, 0x25f3830c, 0x03800080, 0xe74a0380, 0x207b8715, 
    0x876b861d, 0x4a152007, 0x07870775, 0xf6876086, 0x8417674a, 0x0a0021f2, 0x431c9743, 0x8d421485, 0x200b830b, 0x06474d03, 0x71828020, 0x04510120, 
    0x42da8606, 0x1f831882, 0x001a0022, 0xff4d0082, 0x0b0f532c, 0x0d449b94, 0x4e312007, 0x074f12e7, 0x0bf3490b, 0xbb412120, 0x413f820a, 0xef490857, 
    0x80002313, 0xe2830001, 0x6441fc20, 0x8b802006, 0x00012108, 0xfd201582, 0x492c9b48, 0x802014ff, 0x51084347, 0x0f4327f3, 0x17bf4a14, 0x201b7944, 
    0x06964201, 0x134ffe20, 0x20d6830b, 0x25d78280, 0xfd800002, 0x05888000, 0x9318dc41, 0x21d282d4, 0xdb481800, 0x0dff542a, 0x45107743, 0xe14813f5, 
    0x0f034113, 0x83135d45, 0x47b28437, 0xe4510e73, 0x21f58e06, 0x2b8400fd, 0x1041fcac, 0x08db4b0b, 0x421fdb41, 0xdf4b18df, 0x011d210a, 0x420af350, 
    0x6e8308af, 0xac85cb86, 0x1e461082, 0x82b7a407, 0x411420a3, 0xa34130ab, 0x178f4124, 0x41139741, 0x86410d93, 0x82118511, 0x057243d8, 0x8941d9a4, 
    0x3093480c, 0x4a13474f, 0xfb5016a9, 0x07ad4108, 0x4a0f9d42, 0xfe200fad, 0x4708aa41, 0x83482dba, 0x288f4d06, 0xb398c3bb, 0x44267b41, 0xb34439d7, 
    0x0755410f, 0x200ebb45, 0x0f5f4215, 0x20191343, 0x06df5301, 0xf04c0220, 0x2ba64d07, 0x82050841, 0x430020ce, 0xa78f3627, 0x5213ff42, 0x2f970bc1, 
    0x4305ab55, 0xa084111b, 0x450bac45, 0x5f4238b8, 0x010c2106, 0x0220ed82, 0x441bb344, 0x875010af, 0x0737480f, 0x490c5747, 0x0c840c03, 0x4c204b42, 
    0x8ba905d7, 0x8b948793, 0x510c0c51, 0xfb4b24b9, 0x1b174107, 0x5709d74c, 0xd1410ca5, 0x079d480f, 0x201ff541, 0x06804780, 0x7d520120, 0x80002205, 
    0x20a983fe, 0x47bb83fe, 0x1b8409b4, 0x81580220, 0x4e00202c, 0x4f41282f, 0x0eab4f17, 0x57471520, 0x0e0f4808, 0x8221e041, 0x3e1b4a8b, 0x4407175d, 
    0x1b4b071f, 0x4a0f8b07, 0x174a0703, 0x0ba5411b, 0x430fb141, 0x0120057b, 0xfc20dd82, 0x4a056047, 0xf4850c0c, 0x01221982, 0x02828000, 0x1a5d088b, 
    0x20094108, 0x8c0e3941, 0x4900200e, 0x7744434f, 0x200b870b, 0x0e4b5a33, 0x2b41f78b, 0x8b138307, 0x0b9f450b, 0x2406f741, 0xfd808001, 0x09475a00, 
    0x84000121, 0x5980200e, 0x85450e5d, 0x832c8206, 0x4106831e, 0x00213814, 0x28b34810, 0x410c2f4b, 0x5f4a13d7, 0x0b2b4113, 0x6e43a883, 0x11174b05, 
    0x4b066a45, 0xcc470541, 0x5000202b, 0xcb472f4b, 0x44b59f0f, 0xc5430b5b, 0x0d654907, 0x21065544, 0xd6828080, 0xfe201982, 0x8230ec4a, 0x120025c2, 
    0x80ff8000, 0x4128d74d, 0x3320408b, 0x410a9f50, 0xdb822793, 0x822bd454, 0x61134b2e, 0x410b214a, 0xad4117c9, 0x0001211f, 0x4206854f, 0x4b430596, 
    0x06bb5530, 0x2025cf46, 0x0ddd5747, 0x500ea349, 0x0f840fa7, 0x5213c153, 0x634e08d1, 0x0bbe4809, 0x59316e4d, 0x5b50053f, 0x203f6323, 0x5117eb46, 
    0x94450a63, 0x246e410a, 0x63410020, 0x0bdb5f2f, 0x4233ab44, 0x39480757, 0x112d4a07, 0x7241118f, 0x000e2132, 0x9f286f41, 0x0f8762c3, 0x33350723, 
    0x094e6415, 0x2010925f, 0x067252fe, 0xd0438020, 0x63a68225, 0x11203a4f, 0x480e6360, 0x5748131f, 0x079b521f, 0x200e2f43, 0x864b8315, 0x113348e7, 
    0x85084e48, 0x06855008, 0x5880fd21, 0x7c420925, 0x0c414824, 0x37470c86, 0x1b8b422b, 0x5b0a8755, 0x23410c21, 0x0b83420b, 0x5a082047, 0xf482067f, 
    0xa80b4c47, 0x0c0021cf, 0x20207b42, 0x0fb74100, 0x420b8744, 0xeb43076f, 0x0f6f420b, 0x4261fe20, 0x439aa00c, 0x215034e3, 0x0ff9570f, 0x4b1f2d5d, 
    0x2d5d0c6f, 0x09634d0b, 0x1f51b8a0, 0x620f200c, 0xaf681e87, 0x24f94d07, 0x4e0f4945, 0xfe200c05, 0x22139742, 0x57048080, 0x23950c20, 0x97601585, 
    0x4813201f, 0xad620523, 0x200f8f0f, 0x9e638f15, 0x00002181, 0x41342341, 0x0f930f0b, 0x210b4b62, 0x978f0001, 0xfe200f84, 0x8425c863, 0x2704822b, 
    0x80000a00, 0x00038001, 0x610e9768, 0x834514bb, 0x0bc3430f, 0x2107e357, 0x80848080, 0x4400fe21, 0x2e410983, 0x00002a1a, 0x00000700, 0x800380ff, 
    0x0fdf5800, 0x59150021, 0xd142163d, 0x0c02410c, 0x01020025, 0x65800300, 0x00240853, 0x1d333501, 0x15220382, 0x35420001, 0x44002008, 0x376406d7, 
    0x096f6b19, 0x480bc142, 0x8f4908a7, 0x211f8b1f, 0x9e830001, 0x0584fe20, 0x4180fd21, 0x11850910, 0x8d198259, 0x000021d4, 0x5a08275d, 0x275d1983, 
    0x06d9420e, 0x9f08b36a, 0x0f7d47b5, 0x8d8a2f8b, 0x4c0e0b57, 0xe7410e17, 0x42d18c1a, 0xb351087a, 0x1ac36505, 0x4b4a2f20, 0x0b9f450d, 0x430beb53, 
    0xa7881015, 0xa5826a83, 0x80200f82, 0x86185a65, 0x4100208e, 0x176c3367, 0x0fe7650b, 0x4a17ad4b, 0x0f4217ed, 0x112e4206, 0x41113a42, 0xf7423169, 
    0x0cb34737, 0x560f8b46, 0xa75407e5, 0x5f01200f, 0x31590c48, 0x80802106, 0x42268841, 0x0020091e, 0x4207ef64, 0x69461df7, 0x138d4114, 0x820f5145, 
    0x53802090, 0xff200529, 0xb944b183, 0x417e8505, 0x00202561, 0x15210082, 0x42378200, 0x9b431cc3, 0x004f220d, 0x0dd54253, 0x4213f149, 0x7d41133b, 
    0x42c9870b, 0x802010f9, 0x420b2c42, 0x8f441138, 0x267c4408, 0x600cb743, 0x8f4109d3, 0x05ab701d, 0x83440020, 0x3521223f, 0x0b794733, 0xfb62fe20, 
    0x4afd2010, 0xaf410ae7, 0x25ce8525, 0x01080000, 0x7b6b0000, 0x0973710b, 0x82010021, 0x49038375, 0x33420767, 0x052c4212, 0x58464b85, 0x41fe2005, 
    0x50440c27, 0x000c2209, 0x1cb36b80, 0x9b06df44, 0x0f93566f, 0x52830220, 0xfe216e8d, 0x200f8200, 0x0fb86704, 0xb057238d, 0x050b5305, 0x7217eb47, 
    0xbd410b6b, 0x0f214610, 0x871f9956, 0x1e91567e, 0x2029b741, 0x20008200, 0x18b7410a, 0x27002322, 0x41095543, 0x0f8f0fb3, 0x41000121, 0x889d111c, 
    0x14207b82, 0x00200382, 0x73188761, 0x475013a7, 0x6e33200c, 0x234e0ea3, 0x9b138313, 0x08e54d17, 0x9711094e, 0x2ee74311, 0x4908875e, 0xd75d1f1f, 
    0x19ab5238, 0xa2084d48, 0x63a7a9b3, 0x55450b83, 0x0fd74213, 0x440d814c, 0x4f481673, 0x05714323, 0x13000022, 0x412e1f46, 0xdf493459, 0x21c7550f, 
    0x8408215f, 0x201d49cb, 0xb1103043, 0x0f0d65d7, 0x452b8d41, 0x594b0f8d, 0x0b004605, 0xb215eb46, 0x000a24d7, 0x47000080, 0x002118cf, 0x06436413, 
    0x420bd750, 0x2b500743, 0x076a470c, 0x4105c050, 0xd942053f, 0x0d00211a, 0x5f44779c, 0x0ce94805, 0x51558186, 0x14a54c0b, 0x49082b41, 0x0a4b0888, 
    0x8080261f, 0x0d000000, 0x20048201, 0x1deb6a03, 0x420cb372, 0x07201783, 0x4306854d, 0x8b830c59, 0x59093c74, 0x0020250f, 0x67070f4a, 0x2341160b, 
    0x00372105, 0x431c515d, 0x554e17ef, 0x0e5d6b05, 0x41115442, 0xb74a1ac1, 0x2243420a, 0x5b4f878f, 0x7507200f, 0x384b086f, 0x09d45409, 0x0020869a, 
    0x12200082, 0xab460382, 0x10075329, 0x54138346, 0xaf540fbf, 0x1ea75413, 0x9a0c9e54, 0x0f6b44c1, 0x41000021, 0x47412a4f, 0x07374907, 0x5310bf76, 
    0xff2009b4, 0x9a09a64c, 0x8200208d, 0x34c34500, 0x970fe141, 0x1fd74b0f, 0x440a3850, 0x206411f0, 0x27934609, 0x470c5d41, 0x555c2947, 0x1787540f, 
    0x6e0f234e, 0x7d540a1b, 0x1d736b08, 0x0026a088, 0x80000e00, 0x9b5200ff, 0x08ef4318, 0x450bff77, 0x1d4d0b83, 0x081f7006, 0xcb691b86, 0x4b022008, 
    0xc34b0b33, 0x1d0d4a0c, 0x8025a188, 0x0b000000, 0x52a38201, 0xbf7d0873, 0x0c234511, 0x8f0f894a, 0x4101200f, 0x0c880c9d, 0x2b418ea1, 0x06c74128, 
    0x66181341, 0x7b4c0bb9, 0x0c06630b, 0xfe200c87, 0x9ba10882, 0x27091765, 0x01000008, 0x02800380, 0x48113f4e, 0x29430cf5, 0x09a75a0b, 0x31618020, 
    0x6d802009, 0x61840e33, 0x8208bf51, 0x0c637d61, 0x7f092379, 0x4f470f4b, 0x1797510c, 0x46076157, 0xf5500fdf, 0x0f616910, 0x1171fe20, 0x82802006, 
    0x08696908, 0x41127a4c, 0x3f4a15f3, 0x01042607, 0x0200ff00, 0x1cf77700, 0xff204185, 0x00235b8d, 0x43100000, 0x3b22243f, 0x3b4d3f00, 0x0b937709, 
    0xad42f18f, 0x0b1f420f, 0x51084b43, 0x8020104a, 0xb557ff83, 0x052b7f2a, 0x0280ff22, 0x250beb78, 0x00170013, 0xbf6d2500, 0x07db760e, 0x410e2b7f, 
    0x00230e4f, 0x49030000, 0x0582055b, 0x07000326, 0x00000b00, 0x580bcd46, 0x00200cdd, 0x57078749, 0x8749160f, 0x0f994f0a, 0x41134761, 0x01200b31, 
    0xeb796883, 0x0b41500b, 0x0e90b38e, 0x202e7b51, 0x05d95801, 0x41080570, 0x1d530fc9, 0x0b937a0f, 0xaf8eb387, 0xf743b98f, 0x07c74227, 0x80000523, 
    0x0fcb4503, 0x430ca37b, 0x7782077f, 0x8d0a9947, 0x08af4666, 0xeb798020, 0x6459881e, 0xc3740bbf, 0x0feb6f0b, 0x20072748, 0x052b6102, 0x435e0584, 
    0x7d088308, 0x03200afd, 0x92109e41, 0x28aa8210, 0x80001500, 0x80030000, 0x0fdb5805, 0x209f4018, 0xa7418d87, 0x0aa3440f, 0x20314961, 0x073a52ff, 
    0x6108505d, 0x43181051, 0x00223457, 0xe7820500, 0x50028021, 0x81410d33, 0x063d7108, 0xdb41af84, 0x4d888205, 0x00201198, 0x463d835f, 0x152106d7, 
    0x0a355a33, 0x6917614e, 0x75411f4d, 0x184b8b07, 0x1809c344, 0x21091640, 0x0b828000, 0x42808021, 0x26790519, 0x86058605, 0x2428422d, 0x22123b42, 
    0x42000080, 0xf587513b, 0x7813677b, 0xaf4d139f, 0x00ff210c, 0x5e0a1d57, 0x3b421546, 0x01032736, 0x02000380, 0x41180480, 0x2f420f07, 0x0c624807, 
    0x00000025, 0x18000103, 0x83153741, 0x430120c3, 0x042106b2, 0x088d4d00, 0x2f830620, 0x1810434a, 0x18140345, 0x8507fb41, 0x5ee582ea, 0x0023116c, 
    0x8d000600, 0x053b56af, 0xa6554fa2, 0x0d704608, 0x40180d20, 0x47181a43, 0xd37b07ff, 0x0b79500c, 0x420fd745, 0x47450bd9, 0x8471830a, 0x095a777e, 
    0x84137542, 0x82002013, 0x2f401800, 0x0007213b, 0x4405e349, 0x0d550ff3, 0x16254c0c, 0x820ffe4a, 0x0400218a, 0x89066f41, 0x106b414f, 0xc84d0120, 
    0x80802206, 0x0c9a4b03, 0x00100025, 0x68000200, 0x9d8c2473, 0x44134344, 0xf36a0f33, 0x4678860f, 0x1b440a25, 0x41988c0a, 0x80201879, 0x43079b5e, 
    0x4a18080b, 0x0341190b, 0x1259530c, 0x43251552, 0x908205c8, 0x0cac4018, 0x86000421, 0x0e504aa2, 0x0020b891, 0xfb450082, 0x51132014, 0x8f5205f3, 
    0x35052108, 0x8505cb59, 0x0f6d4f70, 0x82150021, 0x29af5047, 0x4f004b24, 0x75795300, 0x1b595709, 0x460b6742, 0xbf4b0f0d, 0x5743870b, 0xcb6d1461, 
    0x08f64505, 0x4e05ab6c, 0x334126c3, 0x0bcb6b0d, 0x1811034d, 0x4111ef4b, 0x814f1ce5, 0x20af8227, 0x07fd7b80, 0x41188e84, 0xef410f33, 0x80802429, 
    0x410d0000, 0xa34205ab, 0x76b7881c, 0xff500b89, 0x0741430f, 0x20086f4a, 0x209d8200, 0x234c18fd, 0x05d4670a, 0x4509af51, 0x9642078d, 0x189e831d, 
    0x7c1cc74b, 0xcd4c07b9, 0x0e7c440f, 0x8b7b0320, 0x21108210, 0xc76c8080, 0x03002106, 0x6b23bf41, 0xc549060b, 0x7946180b, 0x0ff7530f, 0x17ad4618, 
    0x200ecd45, 0x208c83fd, 0x5e0488fe, 0x032009c6, 0x420d044e, 0x0d8f0d7f, 0x00820020, 0x18001021, 0x6d273b45, 0xfd4c0c93, 0xcf451813, 0x0fe5450f, 
    0x5a47c382, 0x820a8b0a, 0x282b4998, 0x410a8b5b, 0x4b232583, 0x54004f00, 0x978f0ce3, 0x500f1944, 0xa95f1709, 0x0280220b, 0x05ba7080, 0xa1530682, 
    0x06324c13, 0x91412582, 0x05536e2c, 0x63431020, 0x0f434706, 0x8c11374c, 0x176143d7, 0x4d0f454c, 0xd3680bed, 0x0bee4d17, 0x212b9a41, 0x0f530a00, 
    0x140d531c, 0x43139143, 0x95610e8d, 0x0f094415, 0x4205fb56, 0x1b4205cf, 0x17015225, 0x5e0c477f, 0xaf6e0aeb, 0x0ff36218, 0x04849a84, 0x0a454218, 
    0x9c430420, 0x23c6822b, 0x04000102, 0x45091b4b, 0xf05f0955, 0x82802007, 0x421c2023, 0x5218282b, 0x7b53173f, 0x0fe7480c, 0x74173b7f, 0x47751317, 
    0x634d1807, 0x0f6f430f, 0x24086547, 0xfc808002, 0x0b3c7f80, 0x10840120, 0x188d1282, 0x20096b43, 0x0fc24403, 0x00260faf, 0x0180000b, 0x3f500280, 
    0x18002019, 0x450b4941, 0xf3530fb9, 0x18002010, 0x8208a551, 0x06234d56, 0xcb58a39b, 0xc3421805, 0x1313461e, 0x0f855018, 0xd34b0120, 0x6cfe2008, 
    0x574f0885, 0x09204114, 0x07000029, 0x00008000, 0x44028002, 0x01420f57, 0x10c95c10, 0x11184c18, 0x80221185, 0x7f421e00, 0x00732240, 0x09cd4977, 
    0x6d0b2b42, 0x4f180f8f, 0x8f5a0bcb, 0x9b0f830f, 0x0fb9411f, 0x230b5756, 0x00fd8080, 0x82060745, 0x000121d5, 0x8e0fb277, 0x4a8d4211, 0x24061c53, 
    0x04000007, 0x12275280, 0x430c954c, 0x80201545, 0x200f764f, 0x20008200, 0x20ce8308, 0x09534f02, 0x660edf64, 0x73731771, 0xe7411807, 0x20a2820c, 
    0x13b64404, 0x8f5d6682, 0x1d6b4508, 0x0cff4d18, 0x3348c58f, 0x0fc34c07, 0x31558b84, 0x8398820f, 0x17514712, 0x240b0e46, 0x80000a00, 0x093b4502, 
    0x420f9759, 0xa54c0bf1, 0x0f2b470c, 0x410d314b, 0x2584170c, 0x73b30020, 0xb55fe782, 0x204d8410, 0x08e043fe, 0x4f147e41, 0x022008ab, 0x4b055159, 
    0x2950068f, 0x00022208, 0x48511880, 0x82002009, 0x00112300, 0x634dff00, 0x24415f27, 0x180f6d43, 0x4d0b5d45, 0x4d5f05ef, 0x01802317, 0x56188000, 
    0xa7840807, 0xc6450220, 0x21ca8229, 0x4b781a00, 0x3359182c, 0x0cf3470f, 0x180bef46, 0x420b0354, 0xff470b07, 0x4515200a, 0x9758239b, 0x4a80200c, 
    0xd2410a26, 0x05fb4a08, 0x4b05e241, 0x03200dc9, 0x92290941, 0x00002829, 0x00010900, 0x5b020001, 0x23201363, 0x460d776a, 0xef530fdb, 0x209a890c, 
    0x13fc4302, 0x00008024, 0xc4820104, 0x08820220, 0x20086b5b, 0x18518700, 0x8408d349, 0x0da449a1, 0x00080024, 0x7b690280, 0x4c438b1a, 0x01220f63, 
    0x4c878000, 0x5c149c53, 0xfb430868, 0x2f56181e, 0x0ccf7b1b, 0x0f075618, 0x2008e347, 0x14144104, 0x00207f83, 0x00207b82, 0x201adf47, 0x16c35a13, 
    0x540fdf47, 0x802006c8, 0x5418f185, 0x29430995, 0x00002419, 0x58001600, 0x5720316f, 0x4d051542, 0x4b7b1b03, 0x138f4707, 0xb747b787, 0x4aab8213, 
    0x058305fc, 0x20115759, 0x82128401, 0x0a0b44e8, 0x46800121, 0xe64210d0, 0x82129312, 0x4bffdffe, 0x3b41171b, 0x9b27870f, 0x808022ff, 0x085c68fe, 
    0x41800021, 0x01410b20, 0x001a213a, 0x47480082, 0x11374e12, 0x56130b4c, 0xdf4b0c65, 0x0b0f590b, 0x0f574c18, 0x830feb4b, 0x075f480f, 0x480b4755, 
    0x40490b73, 0x80012206, 0x09d74280, 0x80fe8022, 0x80210e86, 0x056643ff, 0x10820020, 0x420b2646, 0x0b58391a, 0xd74c1808, 0x078b4e22, 0x2007f55f, 
    0x4b491807, 0x83802017, 0x65aa82a7, 0x3152099e, 0x068b7616, 0x9b431220, 0x09bb742c, 0x500e376c, 0x8342179b, 0x0a4d5d0f, 0x8020a883, 0x180cd349, 
    0x2016bb4b, 0x14476004, 0x84136c43, 0x08cf7813, 0x4f4c0520, 0x156f420f, 0x20085f42, 0x6fd3be03, 0xd4d30803, 0xa7411420, 0x004b222c, 0x0d3b614f, 
    0x3f702120, 0x1393410a, 0x8f132745, 0x47421827, 0x41e08209, 0xb05e2bb9, 0x18b7410c, 0x18082647, 0x4107a748, 0xeb8826bf, 0x0ca76018, 0x733ecb41, 
    0xd0410d83, 0x43ebaf2a, 0x0420067f, 0x721dab4c, 0x472005bb, 0x4105d341, 0x334844cb, 0x20dba408, 0x47d6ac00, 0x034e3aef, 0x0f8f421b, 0x930f134d, 
    0x3521231f, 0xb7421533, 0x42f5ad0a, 0x1e961eaa, 0x17000022, 0x4c367b50, 0x7d491001, 0x0bf5520f, 0x4c18fda7, 0xb8460c55, 0x83fe2005, 0x00fe25b9, 
    0x80000180, 0x9e751085, 0x261b5c12, 0x82110341, 0x001123fb, 0x4518fe80, 0xf38c2753, 0x6d134979, 0x295107a7, 0xaf5f180f, 0x0fe3660c, 0x180b6079, 
    0x2007bd5f, 0x9aab9103, 0x2f4d1811, 0x05002109, 0x44254746, 0x1d200787, 0x450bab75, 0x4f180f57, 0x4f181361, 0x3b831795, 0xeb4b0120, 0x0b734805, 
    0x84078f48, 0x2e1b47bc, 0x00203383, 0xaf065f45, 0x831520d7, 0x130f51a7, 0x1797bf97, 0x2b47d783, 0x18fe2005, 0x4a18a44f, 0xa64d086d, 0x1ab0410d, 
    0x6205a258, 0xdbab069f, 0x4f06f778, 0xa963081d, 0x133b670a, 0x8323d141, 0x13195b23, 0x530f5e70, 0xe5ad0824, 0x58001421, 0x1f472b4b, 0x47bf410c, 
    0x82000121, 0x83fe20cb, 0x07424404, 0x68068243, 0xd7ad0d3d, 0x00010d26, 0x80020000, 0x4a1c6f43, 0x23681081, 0x10a14f13, 0x8a070e57, 0x430a848f, 
    0x7372243e, 0x4397a205, 0xb56c1021, 0x43978f0f, 0x64180505, 0x99aa0ff2, 0x0e000022, 0x20223341, 0x094b4f37, 0x074a3320, 0x2639410a, 0xfe208e84, 
    0x8b0e0048, 0x508020a3, 0x9e4308fe, 0x073f4115, 0xe3480420, 0x0c9b5f1b, 0x7c137743, 0x9a95185b, 0x6122b148, 0x979b08df, 0x0fe36c18, 0x48109358, 
    0x23441375, 0x0ffd5c0b, 0x180fc746, 0x2011d157, 0x07e95702, 0x58180120, 0x18770ac3, 0x51032008, 0x7d4118e3, 0x80802315, 0x3b4c1900, 0xbb5a1830, 
    0x0ceb6109, 0x5b0b3d42, 0x4f181369, 0x4f180b8d, 0x4f180f75, 0x355a1b81, 0x200d820d, 0x18e483fd, 0x4528854f, 0x89420846, 0x1321411f, 0x44086b60, 
    0x07421d77, 0x107d4405, 0x4113fd41, 0x5a181bf1, 0x4f180db3, 0x8021128f, 0x20f68280, 0x44a882fe, 0x334d249a, 0x052f6109, 0x1520c3a7, 0xef4eb783, 
    0x4ec39b1b, 0xc4c90ee7, 0x20060b4d, 0x256f4905, 0x4d0cf761, 0xcf9b1f13, 0xa213d74e, 0x0e1145d4, 0x50135b42, 0xcb4e398f, 0x20d79f27, 0x08865d80, 
    0x186d5018, 0xa90f7142, 0x067342d7, 0x3f450420, 0x65002021, 0xe3560771, 0x24d38f23, 0x15333531, 0x0eb94d01, 0x451c9f41, 0x384322fb, 0x00092108, 
    0x19af6b18, 0x6e0c6f5a, 0xbd770bfb, 0x22bb7718, 0x20090f57, 0x25e74204, 0x4207275a, 0xdb5408ef, 0x1769450f, 0x1b1b5518, 0x210b1f57, 0x5e4c8001, 
    0x55012006, 0x802107f1, 0x0a306a80, 0x45808021, 0x0d850b88, 0x31744f18, 0x1808ec54, 0x2009575b, 0x45ffa505, 0x1b420c73, 0x180f9f0f, 0x4a0cf748, 
    0x501805b2, 0x00210f40, 0x4d118f80, 0xd6823359, 0x072b5118, 0x314ad7aa, 0x8fc79f08, 0x45d78b1f, 0xfe20058f, 0x23325118, 0x7b54d9b5, 0x9fc38f46, 
    0x10bb410f, 0x41077b42, 0xc1410faf, 0x27cf441d, 0x46051b4f, 0x04200683, 0x2121d344, 0x8f530043, 0x8fcf9f0e, 0x21df8c1f, 0x50188000, 0x5d180e52, 
    0xfd201710, 0x4405c341, 0xd68528e3, 0x20071f6b, 0x1b734305, 0x6b080957, 0x7d422b1f, 0x67002006, 0x7f8317b1, 0x2024cb48, 0x08676e00, 0x8749a39b, 
    0x18132006, 0x410a6370, 0x8f490b47, 0x7e1f8f13, 0x551805c3, 0x4c180915, 0xfe200e2f, 0x244d5d18, 0x270bcf44, 0xff000019, 0x04800380, 0x5f253342, 
    0xff520df7, 0x13274c18, 0x5542dd93, 0x0776181b, 0xf94a1808, 0x084a4c0c, 0x4308ea5b, 0xde831150, 0x7900fd21, 0x00492c1e, 0x060f4510, 0x17410020, 
    0x0ce74526, 0x6206b341, 0x1f561083, 0x9d6c181b, 0x08a0500e, 0x112e4118, 0x60000421, 0xbf901202, 0x4408e241, 0xc7ab0513, 0xb40f0950, 0x055943c7, 
    0x4f18ff20, 0xc9ae1cad, 0x32b34f18, 0x7a180120, 0x3d520a05, 0x53d1b40a, 0x80200813, 0x1b815018, 0x832bf86f, 0x67731847, 0x297f4308, 0x6418d54e, 
    0x734213f7, 0x056b4b27, 0xdba5fe20, 0x1828aa4e, 0x2031a370, 0x06cb6101, 0x2040ad41, 0x07365300, 0x2558d985, 0x83fe200c, 0x0380211c, 0x542c4743, 
    0x052006b7, 0x6021df45, 0x897b0707, 0x18d3c010, 0x20090e70, 0x1d5843ff, 0x540a0e44, 0x002126c5, 0x322f7416, 0x636a5720, 0x0f317409, 0x610fe159, 
    0x294617e7, 0x08555213, 0x2006a75d, 0x6cec84fd, 0xfb5907be, 0x3a317405, 0x83808021, 0x180f20ea, 0x4626434a, 0x531818e3, 0xdb59172d, 0x0cbb460c, 
    0x2013d859, 0x18b94502, 0x8f46188d, 0x77521842, 0x0a184e38, 0x9585fd20, 0x6a180684, 0xc64507e9, 0x51cbb230, 0xd3440cf3, 0x17ff6a0f, 0x450f5b42, 
    0x276407c1, 0x4853180a, 0x21ccb010, 0xcf580013, 0x0c15442d, 0x410a1144, 0x1144359d, 0x5cfe2006, 0xa1410a43, 0x2bb64519, 0x2f5b7618, 0xb512b745, 
    0x0cfd6fd1, 0x42089f59, 0xb8450c70, 0x0000232d, 0x50180900, 0xb9491ae3, 0x0fc37610, 0x01210f83, 0x0f3b4100, 0xa01b2742, 0x0ccd426f, 0x6e8f6f94, 
    0x9c808021, 0xc7511870, 0x17c74b08, 0x9b147542, 0x44fe2079, 0xd5480c7e, 0x95ef861d, 0x101b597b, 0xf5417594, 0x9f471808, 0x86868d0e, 0x3733491c, 
    0x690f4d6d, 0x43440b83, 0x1ba94c0b, 0x660cd16b, 0x802008ae, 0x74126448, 0xcb4f38a3, 0x2cb74b0b, 0x47137755, 0xe3971777, 0x1b5d0120, 0x057a4108, 
    0x6e08664d, 0x17421478, 0x11af4208, 0x850c3f42, 0x08234f0c, 0x4321eb4a, 0xf3451095, 0x0f394e0f, 0x4310eb45, 0xc09707b1, 0x54431782, 0xaec08d1d, 
    0x0f434dbb, 0x9f0c0b45, 0x0a3b4dbb, 0x4618bdc7, 0x536032eb, 0x17354213, 0x4d134169, 0xc7a30c2f, 0x4e254342, 0x174332cf, 0x43cdae17, 0x6b4706e4, 
    0x0e16430d, 0x530b5542, 0x2f7c26bb, 0x13075f31, 0x43175342, 0x60181317, 0x6550114e, 0x28624710, 0x58070021, 0x59181683, 0x2d540cf5, 0x05d5660c, 
    0x20090c7b, 0x0e157e02, 0x8000ff2b, 0x14000080, 0x80ff8000, 0x27137e03, 0x336a4b20, 0x0f817107, 0x13876e18, 0x730f2f7e, 0x2f450b75, 0x6d02200b, 
    0x6d66094c, 0x4b802009, 0x15820a02, 0x2f45fe20, 0x5e032006, 0x00202fd9, 0x450af741, 0xeb412e0f, 0x0ff3411f, 0x420a8b65, 0xf7410eae, 0x1c664810, 
    0x540e1145, 0xbfa509f3, 0x42302f58, 0x80200c35, 0xcb066c47, 0x4b1120c1, 0x41492abb, 0x34854110, 0xa7097b72, 0x251545c7, 0x4b2c7f56, 0xc5b40bab, 
    0x940cd54e, 0x2e6151c8, 0x09f35f18, 0x4b420420, 0x09677121, 0x8f24f357, 0x1b5418e1, 0x08915a1f, 0x3143d894, 0x22541805, 0x1b9b4b0e, 0x8c0d3443, 
    0x1400240d, 0x18ff8000, 0x582e6387, 0xf99b2b3b, 0x8807a550, 0x17a14790, 0x2184fd20, 0x5758fe20, 0x2354882c, 0x15000080, 0x5e056751, 0x334c2c2f, 
    0x97c58f0c, 0x1fd7410f, 0x0d4d4018, 0x4114dc41, 0x04470ed6, 0x0dd54128, 0x00820020, 0x02011523, 0x22008700, 0x86480024, 0x0001240a, 0x8682001a, 
    0x0002240b, 0x866c000e, 0x8a03200b, 0x8a042017, 0x0005220b, 0x22218614, 0x84060000, 0x86012017, 0x8212200f, 0x250b8519, 0x000d0001, 0x0b850031, 
    0x07000224, 0x0b862600, 0x11000324, 0x0b862d00, 0x238a0420, 0x0a000524, 0x17863e00, 0x17840620, 0x01000324, 0x57820904, 0x0b85a783, 0x0b85a785, 
    0x0b85a785, 0x22000325, 0x85007a00, 0x85a7850b, 0x85a7850b, 0x22a7850b, 0x82300032, 0x00342201, 0x0805862f, 0x35003131, 0x54207962, 0x74736972, 
    0x47206e61, 0x6d6d6972, 0x65527265, 0x616c7567, 0x58545472, 0x6f725020, 0x43796767, 0x6e61656c, 0x30325454, 0x822f3430, 0x35313502, 0x79006200, 
    0x54002000, 0x69007200, 0x74007300, 0x6e006100, 0x47200f82, 0x6d240f84, 0x65006d00, 0x52200982, 0x67240582, 0x6c007500, 0x72201d82, 0x54222b82, 
    0x23825800, 0x19825020, 0x67006f22, 0x79220182, 0x1b824300, 0x3b846520, 0x1f825420, 0x41000021, 0x1422099b, 0x0b410000, 0x87088206, 0x01012102, 
    0x78080982, 0x01020101, 0x01040103, 0x01060105, 0x01080107, 0x010a0109, 0x010c010b, 0x010e010d, 0x0110010f, 0x01120111, 0x01140113, 0x01160115, 
    0x01180117, 0x011a0119, 0x011c011b, 0x011e011d, 0x0020011f, 0x00040003, 0x00060005, 0x00080007, 0x000a0009, 0x000c000b, 0x000e000d, 0x0010000f, 
    0x00120011, 0x00140013, 0x00160015, 0x00180017, 0x001a0019, 0x001c001b, 0x001e001d, 0x08bb821f, 0x22002142, 0x24002300, 0x26002500, 0x28002700, 
    0x2a002900, 0x2c002b00, 0x2e002d00, 0x30002f00, 0x32003100, 0x34003300, 0x36003500, 0x38003700, 0x3a003900, 0x3c003b00, 0x3e003d00, 0x40003f00, 
    0x42004100, 0x4b09f382, 0x00450044, 0x00470046, 0x00490048, 0x004b004a, 0x004d004c, 0x004f004e, 0x00510050, 0x00530052, 0x00550054, 0x00570056, 
    0x00590058, 0x005b005a, 0x005d005c, 0x005f005e, 0x01610060, 0x01220121, 0x01240123, 0x01260125, 0x01280127, 0x012a0129, 0x012c012b, 0x012e012d, 
    0x0130012f, 0x01320131, 0x01340133, 0x01360135, 0x01380137, 0x013a0139, 0x013c013b, 0x013e013d, 0x0140013f, 0x00ac0041, 0x008400a3, 0x00bd0085, 
    0x00e80096, 0x008e0086, 0x009d008b, 0x00a400a9, 0x008a00ef, 0x008300da, 0x00f20093, 0x008d00f3, 0x00880097, 0x00de00c3, 0x009e00f1, 0x00f500aa, 
    0x00f600f4, 0x00ad00a2, 0x00c700c9, 0x006200ae, 0x00900063, 0x00cb0064, 0x00c80065, 0x00cf00ca, 0x00cd00cc, 0x00e900ce, 0x00d30066, 0x00d100d0, 
    0x006700af, 0x009100f0, 0x00d400d6, 0x006800d5, 0x00ed00eb, 0x006a0089, 0x006b0069, 0x006c006d, 0x00a0006e, 0x0071006f, 0x00720070, 0x00750073, 
    0x00760074, 0x00ea0077, 0x007a0078, 0x007b0079, 0x007c007d, 0x00a100b8, 0x007e007f, 0x00810080, 0x00ee00ec, 0x6e750eba, 0x646f6369, 0x78302365, 
    0x31303030, 0x32200e8d, 0x33200e8d, 0x34200e8d, 0x35200e8d, 0x36200e8d, 0x37200e8d, 0x38200e8d, 0x39200e8d, 0x61200e8d, 0x62200e8d, 0x63200e8d, 
    0x64200e8d, 0x65200e8d, 0x66200e8d, 0x31210e8c, 0x8d0e8d30, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 
    0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x8d3120ef, 0x66312def, 0x6c656406, 0x04657465, 0x6f727545, 0x3820ec8c, 0x3820ec8d, 
    0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 0x3820ec8d, 
    0x3820ec8d, 0x200ddc41, 0x0ddc4139, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 
    0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0xef8d3920, 0x00663923, 0x48fa0500, 0x00f762f9, 
};

static void GetDefaultCompressedFontDataTTF(const void** ttf_compressed_data, unsigned int* ttf_compressed_size)
{
    *ttf_compressed_data = proggy_clean_ttf_compressed_data;
    *ttf_compressed_size = proggy_clean_ttf_compressed_size;
}
