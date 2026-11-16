// dear imgui: Renderer Backend for Nintendo GameCube/Wii GX
// This needs to be used along with a Platform Backend (custom input handling on Wii).
//
// Implemented features:
//  [X] Renderer: User texture binding. Use a GXTexObj* as ImTextureID.
//  [X] Renderer: Texture updates support for Dear ImGui's dynamic font atlas.
//  [X] Renderer: Large meshes support with ImDrawCmd::VtxOffset.
//
// GX state is global and cannot be queried completely. RenderDrawData() sets the GX
// state it needs; callers which continue rendering afterwards must restore their own
// GX state.

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

struct ImTextureData;

IMGUI_IMPL_API bool ImGui_ImplGX_Init();
IMGUI_IMPL_API void ImGui_ImplGX_Shutdown();
IMGUI_IMPL_API void ImGui_ImplGX_NewFrame();
IMGUI_IMPL_API void ImGui_ImplGX_RenderDrawData(ImDrawData* draw_data);

// Called automatically by RenderDrawData() for entries in ImDrawData::Textures.
IMGUI_IMPL_API void ImGui_ImplGX_UpdateTexture(ImTextureData* tex);
IMGUI_IMPL_API void ImGui_ImplGX_DestroyDeviceObjects();

#endif // #ifndef IMGUI_DISABLE
