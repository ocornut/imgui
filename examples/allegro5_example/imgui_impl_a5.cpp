// ImGui Allegro 5 bindings
// https://github.com/ocornut/imgui
// by @birthggd, public domain

#include <stdint.h>     // uint64_t
#include <cstring>      // memcpy
#include <imgui.h>
#include "imgui_impl_a5.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#ifdef _WIN32
#include <allegro5/allegro_windows.h>
#endif


static ALLEGRO_DISPLAY *g_disp = NULL;
static ALLEGRO_BITMAP *g_img = NULL;
static double g_time = 0.0;


static void ImGui_ImplA5_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
  if (cmd_lists_count == 0) return;

  const float width = ImGui::GetIO().DisplaySize.x;
  const float height = ImGui::GetIO().DisplaySize.y;

  const float bw = al_get_bitmap_width(g_img);
  const float bh = al_get_bitmap_height(g_img);

  int op, src, dst;
  al_get_blender(&op, &src, &dst);
  al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

  #define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->Element))
  for (int n=0; n < cmd_lists_count; ++n) {
    const ImDrawList* cmd_list = cmd_lists[n];
    static ImVector<ALLEGRO_VERTEX> vertices;
    vertices.reserve(cmd_list->vtx_buffer.size());
    vertices.clear();
    for (int i = 0; i < cmd_list->vtx_buffer.size(); ++i) {
      ALLEGRO_VERTEX v;
      const ImDrawVert &dv = cmd_list->vtx_buffer[i];
      v.x = dv.pos.x;
      v.y = dv.pos.y;
      v.z = 0;
      v.u = dv.uv.x * bw;
      v.v = dv.uv.y * bh;
      unsigned char *c = (unsigned char*)&dv.col;
      v.color = al_map_rgba(c[0], c[1], c[2], c[3]);
      vertices.push_back(v);
    }
    int vtx_offset = 0;
    for (size_t cmd_i=0; cmd_i < cmd_list->commands.size(); ++cmd_i) {
      const ImDrawCmd *pcmd = &cmd_list->commands[cmd_i];
      if (pcmd->user_callback) {
        pcmd->user_callback(cmd_list, pcmd);
      }
      else {
        ALLEGRO_BITMAP *tex = (ALLEGRO_BITMAP*)pcmd->texture_id;
        al_set_clipping_rectangle(pcmd->clip_rect.x, pcmd->clip_rect.y, pcmd->clip_rect.z, pcmd->clip_rect.w);
        al_draw_prim(&vertices[0], NULL, tex, vtx_offset, vtx_offset+pcmd->vtx_count, ALLEGRO_PRIM_TRIANGLE_LIST);
      }
      vtx_offset += pcmd->vtx_count;
    }
  }
  #undef OFFSETOF

  // restore state
  al_set_blender(op, src, dst);
  al_set_clipping_rectangle(0, 0, al_get_display_width(g_disp), al_get_display_height(g_disp));
}


bool Imgui_ImplA5_CreateDeviceObjects()
{
  ImGuiIO &io = ImGui::GetIO();

  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  int flags = al_get_new_bitmap_flags();
  int fmt = al_get_new_bitmap_format();
  al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP|ALLEGRO_MIN_LINEAR|ALLEGRO_MAG_LINEAR);
  al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
  ALLEGRO_BITMAP *img = al_create_bitmap(width, height);
  // restore bitmap state
  al_set_new_bitmap_flags(flags);
  al_set_new_bitmap_format(fmt);
  if (!img) return false;

  ALLEGRO_LOCKED_REGION *locked_img;
  locked_img = al_lock_bitmap(img, al_get_bitmap_format(img), ALLEGRO_LOCK_WRITEONLY);
  if (!locked_img) {
    al_destroy_bitmap(img);
    return false;
  }
  memcpy(locked_img->data, pixels, sizeof(int)*width*height);
  al_unlock_bitmap(img);

  // convert software texture to hardware texture.
  ALLEGRO_BITMAP *cloned_img = al_clone_bitmap(img);
  al_destroy_bitmap(img);
  if (!cloned_img) return false;
  
  io.Fonts->TexID = cloned_img;

  g_img = cloned_img;

  return true;
}


void ImGui_ImplA5_InvalidateDeviceObjects()
{
  if (g_img) {
    al_destroy_bitmap(g_img);
    ImGui::GetIO().Fonts->TexID = NULL;
    g_img = NULL;
  }
}


bool ImGui_ImplA5_Init(ALLEGRO_DISPLAY *disp)
{
  g_disp = disp;

  ImGuiIO& io = ImGui::GetIO();
  io.KeyMap[ImGuiKey_Tab] = ALLEGRO_KEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = ALLEGRO_KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = ALLEGRO_KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = ALLEGRO_KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow] = ALLEGRO_KEY_DOWN;
  io.KeyMap[ImGuiKey_Home] = ALLEGRO_KEY_HOME;
  io.KeyMap[ImGuiKey_End] = ALLEGRO_KEY_END;
  io.KeyMap[ImGuiKey_Delete] = ALLEGRO_KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = ALLEGRO_KEY_BACKSPACE;
  io.KeyMap[ImGuiKey_Enter] = ALLEGRO_KEY_ENTER;
  io.KeyMap[ImGuiKey_Escape] = ALLEGRO_KEY_ESCAPE;
  io.KeyMap[ImGuiKey_A] = ALLEGRO_KEY_A;
  io.KeyMap[ImGuiKey_C] = ALLEGRO_KEY_C;
  io.KeyMap[ImGuiKey_V] = ALLEGRO_KEY_V;
  io.KeyMap[ImGuiKey_X] = ALLEGRO_KEY_X;
  io.KeyMap[ImGuiKey_Y] = ALLEGRO_KEY_Y;
  io.KeyMap[ImGuiKey_Z] = ALLEGRO_KEY_Z;

  io.RenderDrawListsFn = ImGui_ImplA5_RenderDrawLists;

#ifdef _WIN32
  io.ImeWindowHandle = al_get_win_window_handle(disp);
#endif

  return true;
}


void ImGui_ImplA5_Shutdown()
{
  ImGui_ImplA5_InvalidateDeviceObjects();
  ImGui::Shutdown();
}


void ImGui_ImplA5_ProcessEvent(ALLEGRO_EVENT *ev)
{
  ImGuiIO &io = ImGui::GetIO();

  switch (ev->type) {
  case ALLEGRO_EVENT_MOUSE_AXES:
    io.MouseWheel += ev->mouse.dz;
    break;
  case ALLEGRO_EVENT_KEY_CHAR:
    if (ev->keyboard.display == g_disp) {
      io.KeysDown[ev->keyboard.keycode] = true;
      if (ev->keyboard.unichar > 0 && ev->keyboard.unichar < 0x10000)
        io.AddInputCharacter((unsigned short)ev->keyboard.unichar);
    }
    break;
  case ALLEGRO_EVENT_KEY_UP:
    if (ev->keyboard.display == g_disp)
      io.KeysDown[ev->keyboard.keycode] = false;
    break;
  default:
    break;
  }
}


void ImGui_ImplA5_NewFrame()
{
  if (!g_img) Imgui_ImplA5_CreateDeviceObjects();

  ImGuiIO &io = ImGui::GetIO();

  int w, h;
  w = al_get_display_width(g_disp);
  h = al_get_display_height(g_disp);
  io.DisplaySize = ImVec2((float)w, (float)h);

  double current_time = al_get_time();
  io.DeltaTime = g_time > 0.0 ? (float)(current_time - g_time) : (float)(1.0f/60.0f);
  g_time = current_time;
  
  ALLEGRO_KEYBOARD_STATE keys;
  al_get_keyboard_state(&keys);
  io.KeyCtrl = al_key_down(&keys, ALLEGRO_KEYMOD_CTRL);
  io.KeyShift = al_key_down(&keys, ALLEGRO_KEYMOD_SHIFT);
  io.KeyAlt = al_key_down(&keys, ALLEGRO_KEYMOD_ALT);

  ALLEGRO_MOUSE_STATE mouse;
  if (keys.display == g_disp) {
    al_get_mouse_state(&mouse);
    io.MousePos = ImVec2((float)mouse.x, (float)mouse.y);
  }
  else {
    io.MousePos = ImVec2(-1, -1);
  }

  al_get_mouse_state(&mouse);
  io.MouseDown[0] = mouse.buttons & 1;
  io.MouseDown[1] = mouse.buttons & 2;
  io.MouseDown[2] = mouse.buttons & 3;
  
  ImGui::NewFrame();
}
