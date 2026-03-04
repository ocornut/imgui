#pragma once

#ifndef _XBOX_NO_ASSERT
void __cdecl __imgui_xbox_assert_handler(const char* const lpszAssert);
#define __imgui_xbox_assert(_EXPR) __imgui_xbox_assert_handler(#_EXPR)
#else
#define __imgui_xbox_assert(_EXPR)
#endif

#ifndef IM_ASSERT
#define IM_ASSERT(_EXPR) __imgui_xbox_assert
#endif

#include "imgui.h"

IMGUI_IMPL_API bool ImGui_ImplXbox360_Init(int primary_gamepad_port = 0);
IMGUI_IMPL_API void ImGui_ImplXbox360_Shutdown();

IMGUI_IMPL_API void  ImGui_ImplXbox360_NewFrame();

// opens the system keyboard UI (XShowKeyboardUI) and writes the
// result into ImGui's input stream when the user confirms.
// You typically call this when io.WantTextInput is true and you want a
// native keyboard experience instead of a custom on-screen keyboard widget.
// Pass 'title' and 'description' as UTF-16 strings (L"...").
IMGUI_IMPL_API void  ImGui_ImplXbox360_OpenKeyboard(
    const wchar_t* title = L"", 
    const wchar_t* description = L"", 
    const wchar_t* initial_text = L""
);
