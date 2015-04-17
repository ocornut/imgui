void ImGui::PlotHistogram(const char* label, float (*values_getter)(void* data, int idx), const char* (*labels_getter)(void* data, int idx), void* data, void* labels_data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
{
    Plot(ImGuiPlotType_Histogram, label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size, labels_getter, labels_data);
}