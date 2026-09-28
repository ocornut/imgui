// dear imgui: compile/link test for the C++20 backend module 'imgui.impl.allegro5' (needs a display to run).

#include <allegro5/allegro.h>   // application-side platform headers
import std;
import imgui;
import imgui.impl.allegro5;

static_assert(std::is_same_v<decltype(&ImGui_ImplAllegro5_Init), bool (*)(ALLEGRO_DISPLAY*)>);

void RenderFrameAllegro5(ALLEGRO_DISPLAY* display, ALLEGRO_EVENT* event)
{
    ImGui_ImplAllegro5_Init(display);
    ImGui_ImplAllegro5_ProcessEvent(event);
    ImGui_ImplAllegro5_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplAllegro5_RenderDrawData(ImGui::GetDrawData());
}

int main()
{
    return 0;
}
