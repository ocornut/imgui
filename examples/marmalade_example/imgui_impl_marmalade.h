// ImGui Marmalade binding with IwGx
// Copyright (C) 2015 by Giovanni Zito
// This file is part of ImGui
// https://github.com/ocornut/imgui

IMGUI_API bool        ImGui_Marmalade_Init(bool install_callbacks);
IMGUI_API void        ImGui_Marmalade_Shutdown();
IMGUI_API void        ImGui_Marmalade_NewFrame();

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_Marmalade_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_Marmalade_CreateDeviceObjects();

// callbacks (installed by default if you enable 'install_callbacks' during initialization)
// You can also handle inputs yourself and use those as a reference.
IMGUI_API int32		ImGui_Marmalade_PointerButtonEventCallback(void* SystemData,void* pUserData);
//IMGUI_API void        ImGui_Marmalade_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
IMGUI_API int32		ImGui_Marmalade_KeyCallback(void* SystemData,void* userData);
IMGUI_API int32		ImGui_Marmalade_CharCallback(void* SystemData,void* userData);
