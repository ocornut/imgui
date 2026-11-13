// dear imgui: stack layouts
// (stack layout internal headers)

/*

Index of this file:
// [SECTION] Stack Layout Internal API

*/

#pragma once

#include "imgui_internal.h"
#include "imgui_stacklayout.h"

#ifndef IMGUI_DISABLE

//-----------------------------------------------------------------------------
// [SECTION] Stack Layout Internal API
//-----------------------------------------------------------------------------

namespace ImGuiStackLayout
{
    IMGUI_API void ErrorRecoveryTryToRecoverWindowState(const ImGuiErrorRecoveryState* state_in);
} // namespace ImGuiStackLayout


#endif // #ifndef IMGUI_DISABLE
