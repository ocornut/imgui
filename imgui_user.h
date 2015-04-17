#ifndef _IMGUI_USER_H_
#define _IMGUI_USER_H_

namespace ImGui
{
    IMGUI_API void          PlotHistogram(const char* label, float (*values_getter)(void* data, int idx), const char* (*labels_getter)(void* data, int idx), void* data, void* labels_data, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0));
}
#endif
