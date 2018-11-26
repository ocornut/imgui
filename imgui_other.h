#ifndef IMGUI_OTHER_H
#define IMGUI_OTHER_H

#include "imgui.h"
#include <cstdarg>

struct ExampleAppLog
{
	ImGuiTextBuffer     Buf;
	ImGuiTextFilter     Filter;
	ImVector<int>       LineOffsets;        // Index to lines offset
	bool                ScrollToBottom;

	void    Clear() { Buf.clear(); LineOffsets.clear(); }

	void    AddLog(const char* fmt, ...) IM_FMTARGS(2);

	void    Draw(const char* title, bool* p_open = nullptr);
};

#endif
