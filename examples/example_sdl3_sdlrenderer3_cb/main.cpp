// Dear ImGui: standalone example application for SDL3 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important to understand: SDL_Renderer is an _optional_ component of SDL3.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <stdio.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// Struct for our common data
struct app_data
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	bool show_demo_window;
	bool show_another_window;
	ImVec4 clear_color;
	ImGuiIO* io;
	float f;
	int counter;
	
	// // Constructor not _required_, as they are all default constructable, but
	// // this shows what default construction would do.
	// app_data()
	// 	: window(nullptr),
	// 	  renderer(nullptr),
	// 	  show_demo_window(false),
	// 	  show_another_window(false),
	// 	  clear_color(ImVec4(0.0f, 0.0f, 0.0f, 0.00f)),
	// 	  io(nullptr),
	// 	  f(0.0f),
	// 	  counter(0)
	// {}
};

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
#define as reinterpret_cast<app_data*>(appstate)

	// Let imgui handle event
	ImGui_ImplSDL3_ProcessEvent(event);

	// Handle quitting/close window events
	if (event->type == SDL_EVENT_QUIT)
		return SDL_APP_SUCCESS;
	if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event->window.windowID == SDL_GetWindowID(as->window))
		return SDL_APP_SUCCESS;

	// Default lets app continue executing
    return SDL_APP_CONTINUE;

#undef as
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	// Allocate app common data
	app_data* ad = IM_NEW(app_data);
	if (ad == nullptr)
	{
		printf("Error: IM_NEW() returned nullptr\n");
		return SDL_APP_FAILURE;
	}
	
    // Create window with SDL_Renderer graphics context
    Uint32 window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
    ad->window = SDL_CreateWindow("Dear ImGui SDL3+SDL_Renderer callback example", 1280, 720, window_flags);
    if (ad->window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    ad->renderer = SDL_CreateRenderer(ad->window, nullptr);
    if (ad->renderer == nullptr)
    {
        SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderVSync(ad->renderer, 1);
    SDL_SetWindowPosition(ad->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(ad->window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ad->io = &ImGui::GetIO();
    ad->io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    ad->io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(ad->window, ad->renderer);
    ImGui_ImplSDLRenderer3_Init(ad->renderer);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    // ad->io->Fonts->AddFontDefault();
    // ad->io->Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    // ad->io->Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // ad->io->Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // ad->io->Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // ImFont* font = ad->io->Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, ad->io->Fonts->GetGlyphRangesJapanese());
	// IM_ASSERT(font != nullptr);

    // Our state
    ad->show_demo_window = true;
    ad->show_another_window = false;
    ad->clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ad->f = 0.0f;
	ad->counter = 0;
	*appstate = ad;
	
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
#define ad reinterpret_cast<app_data*>(appstate)

	// Start the Dear ImGui frame
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (ad->show_demo_window)
		ImGui::ShowDemoWindow(&ad->show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
	{
		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &ad->show_demo_window);  // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &ad->show_another_window);

		ImGui::SliderFloat("float", &ad->f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", reinterpret_cast<float*>(&ad->clear_color));
								                                // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			ad->counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", ad->counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ad->io->Framerate, ad->io->Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (ad->show_another_window)
	{
		ImGui::Begin("Another Window", &ad->show_another_window);// Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			ad->show_another_window = false;
		ImGui::End();
	}

	// Rendering
	ImGui::Render();
	SDL_SetRenderScale(ad->renderer, ad->io->DisplayFramebufferScale.x, ad->io->DisplayFramebufferScale.y);
	SDL_SetRenderDrawColorFloat(ad->renderer, ad->clear_color.x, ad->clear_color.y, ad->clear_color.z, ad->clear_color.w);
	SDL_RenderClear(ad->renderer);
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), ad->renderer);
	SDL_RenderPresent(ad->renderer);
	
	return SDL_APP_CONTINUE;
#undef ad
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
#define ad reinterpret_cast<app_data*>(appstate)

    // Cleanup
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(ad->renderer);
    SDL_DestroyWindow(ad->window);
	IM_DELETE(ad);
    SDL_Quit();

#undef ad
}
