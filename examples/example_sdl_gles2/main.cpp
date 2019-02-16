// dear imgui: standalone example application for SDL2 + GLES2
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_gles2.h"
#include <stdio.h>
#include <SDL.h>

#include <GLES2/gl2.h>

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif

static bool g_show_demo_window = true;
static bool g_show_another_window = false;
static ImVec4 g_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
static bool g_done = false;
static SDL_Window* g_window = NULL;
static SDL_GLContext g_gl_context = NULL;

static void draw()
{
  // Poll and handle events (inputs, window resize, etc.)
  // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
  // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
  // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
  // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
  SDL_Event event;
  while ( SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if ( event.type == SDL_QUIT ) {
      g_done = true;
    }
    if ( event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
         event.window.windowID == SDL_GetWindowID(g_window)) {
      g_done = true;
    }
  }

  // Start the Dear ImGui frame
  ImGui_ImplGLES2_NewFrame();
  ImGui_ImplSDL2_NewFrame(g_window);
  ImGui::NewFrame();

  // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
  if ( g_show_demo_window ) {
    ImGui::ShowDemoWindow(&g_show_demo_window);
  }

  // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
  {
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin(
      "Hello, world!");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &g_show_demo_window);      // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &g_show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", (float*) &g_clear_color); // Edit 3 floats representing a color

    if ( ImGui::Button(
      "Button")) {                            // Buttons return true when clicked (most widgets return true when edited/activated)
      counter++;
    }
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::End();
  }

  // 3. Show another simple window.
  if ( g_show_another_window ) {
    ImGui::Begin("Another Window",
                 &g_show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::Text("Hello from another window!");
    if ( ImGui::Button("Close Me")) {
      g_show_another_window = false;
    }
    ImGui::End();
  }

  // Rendering
  ImGui::Render();
  SDL_GL_MakeCurrent(g_window, g_gl_context);
  glViewport(0, 0, (int) ImGui::GetIO().DisplaySize.x, (int) ImGui::GetIO().DisplaySize.y);
  glClearColor(g_clear_color.x, g_clear_color.y, g_clear_color.z, g_clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplGLES2_RenderDrawData(ImGui::GetDrawData());

#ifndef __EMSCRIPTEN__          // Swap window is done implicitly on WebGL
  SDL_GL_SwapWindow(g_window);
#endif

#ifdef __EMSCRIPTEN__
  // On Emscripten, we must cancel the main loop if we are done, and then shutdown GL/SDL
  if ( g_done ) {
    emscripten_cancel_main_loop();

    ImGui_ImplGLES2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(g_gl_context);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
  }
#endif
}

int main(int, char**) {
  // Setup SDL
  if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0 ) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }

  // Target GLES 2
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_DisplayMode current;
  SDL_GetCurrentDisplayMode(0, &current);
  SDL_WindowFlags window_flags = ( SDL_WindowFlags )(
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  g_window = SDL_CreateWindow("Dear ImGui SDL2+GLES2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 640, window_flags);
  g_gl_context = SDL_GL_CreateContext(g_window);

#ifndef __EMSCRIPTEN__        // Vsync is not supported on WebGL. This is done automatically
  SDL_GL_SetSwapInterval(1);
#endif

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Setup Platform/Renderer bindings
  ImGui_ImplSDL2_InitForOpenGL(g_window, g_gl_context);
  ImGui_ImplGLES2_Init();

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
  // - Read 'misc/fonts/README.txt' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
  //io.Fonts->AddFontDefault();
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
  //IM_ASSERT(font != NULL);

  // Main loop
#ifdef __EMSCRIPTEN__
  // Main loop have to be handled differently on emscripten: a callback function will be called by the browser at each
  // RequestAnimationFrame at 60hz.
  emscripten_set_main_loop( draw, 0, false );

  // Note that main() will nevertheless exit, but without cleaning up anything. The main loop will start and the
  // callback function will be called!,
#else
  while ( !g_done ) {
    draw();
  }

  // Cleanup
  ImGui_ImplGLES2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(g_gl_context);
  SDL_DestroyWindow(g_window);
  SDL_Quit();
#endif

  return 0;
}
