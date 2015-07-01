//-----------------------------------------------------------------------------
// USER IMPLEMENTATION
// This file contains compile-time options for ImGui.
// Other options (memory allocation overrides, callbacks, etc.) can be set at runtime via the ImGuiIO structure - ImGui::GetIO().
// Usage: to make imgui.h include imconfig.h:
// - either don't define IMGUI_CONDITIONAL_IMCONFIG_H
// - or define both IMGUI_CONDITIONAL_IMCONFIG_H and IMGUI_INCLUDE_IMCONFIG_H
// (the cmake-installed `imgui` target defines
//  IMGUI_CONDITIONAL_IMCONFIG_H automatically)
//-----------------------------------------------------------------------------

#pragma once

#include "myvec.h"

//---- Don't implement help and test window functionality (ShowUserGuide()/ShowStyleEditor()/ShowTestWindow() methods will be empty)
#define IMGUI_DISABLE_TEST_WINDOWS

//---- Don't define obsolete functions names
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//---- Implement STB libraries in a namespace to avoid conflicts
#define IMGUI_STB_NAMESPACE     ImStb

//---- Define constructor and implicit cast operators to convert back<>forth from your math types and ImVec2/ImVec4.
#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const MyVec2& f) { x = f.x; y = f.y; }                       \
        operator MyVec2() const { return MyVec2(x,y); }

#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const MyVec4& f) { x = f.x; y = f.y; z = f.z; w = f.w; }     \
        operator MyVec4() const { return MyVec4(x,y,z,w); }

