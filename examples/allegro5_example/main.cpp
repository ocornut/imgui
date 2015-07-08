// public domain

#include <stdint.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <imgui.h>
#include "imgui_impl_a5.h"

int main(int argc, char **argv)
{
  ALLEGRO_DISPLAY *disp;
  ALLEGRO_EVENT_QUEUE *queue;

  al_init();
  al_install_keyboard();
  al_install_mouse();

  al_init_primitives_addon();

  //al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_DONTCARE);
  disp = al_create_display(1280, 800);

  queue = al_create_event_queue();
  al_register_event_source(queue, al_get_display_event_source(disp));
  al_register_event_source(queue, al_get_keyboard_event_source());
  al_register_event_source(queue, al_get_mouse_event_source());

  ImGui_ImplA5_Init(disp);

  bool show_test_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImColor(114, 144, 154);

  bool running = true;
  while (running) {

    ALLEGRO_EVENT ev;
    while (al_get_next_event(queue, &ev)) {
      ImGui_ImplA5_ProcessEvent(&ev);
      if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) running = false;
    }

    ImGui_ImplA5_NewFrame();

    {
      static float f;
      ImGui::Text("Hello, world!");
      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
      ImGui::ColorEdit3("clear color", (float*)&clear_color);
      if (ImGui::Button("Test Window")) show_test_window ^= 1;
      if (ImGui::Button("Another Window")) show_another_window ^= 1;
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f/ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    }

    if (show_another_window) {
      ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
      ImGui::Begin("Another Window", &show_another_window);
      ImGui::Text("Hello");
      ImGui::End();
    }

    if (show_test_window) {
      ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
      ImGui::ShowTestWindow(&show_test_window);
    }
 
    al_clear_to_color(al_map_rgba_f(clear_color.x, clear_color.y, clear_color.z, clear_color.w));
    ImGui::Render();
    al_flip_display();
  }

  ImGui_ImplA5_Shutdown();

  al_destroy_event_queue(queue);
  al_destroy_display(disp);

  return 0;
}

