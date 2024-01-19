// Dear ImGui: standalone example application for SDL2 + OpenGL using Swift (5.9+ for direct C++ interoperability)
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp
import CSDL
import DearIMGUI
import Foundation

// Main code

signal(SIGINT) { _ in
    exit(0)
}

// Setup SDL
if SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0 {
    fatalError("SDL Init failed: \(String(cString: SDL_GetError(), encoding: .utf8) ?? "")")
}

// Set GLES 3.0 context
SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0)
SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, Int32(SDL_GL_CONTEXT_PROFILE_ES.rawValue))
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3)
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0)
let glsl_version = "#version 300 es"

// Create window with graphics context
SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)
SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)
SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8)

let window_flags: UInt32 = SDL_WINDOW_OPENGL.rawValue | SDL_WINDOW_RESIZABLE.rawValue | SDL_WINDOW_ALLOW_HIGHDPI.rawValue
guard let window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", 0, 0, 1280, 720, window_flags) else {
    fatalError("SDL: SDL_CreateWindow(): \(String(cString: SDL_GetError(), encoding: .utf8) ?? "")")
}

let gl_context = SDL_GL_CreateContext(window)
SDL_GL_MakeCurrent(window, gl_context)
// Enable vsync
SDL_GL_SetSwapInterval(1)

    // From 2.0.18: Enable native IME.
#if(SDL_HINT_IME_SHOW_UI)
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1")
#endif

// Setup Dear ImGui context
ImGui.DebugCheckVersionAndDataLayout(IMGUI_VERSION, MemoryLayout<ImGuiIO>.size, MemoryLayout<ImGuiStyle>.size, MemoryLayout<ImVec2>.size, MemoryLayout<ImVec4>.size, MemoryLayout<ImDrawVert>.size, MemoryLayout<ImDrawIdx>.size)

let context = ImGui.CreateContext(nil)
let io = ImGui.GetIO()
io.pointee.ConfigFlags |= Int32(ImGuiConfigFlags_NavEnableKeyboard.rawValue)     // Enable Keyboard Controls
io.pointee.ConfigFlags |= Int32(ImGuiConfigFlags_NavEnableGamepad.rawValue)      // Enable Gamepad Controls

// Setup Dear ImGui style
ImGui.StyleColorsDark(nil)
//ImGui.StyleColorsLight(nil)

// Setup Platform/Renderer backends
ImGui_ImplSDL2_InitForOpenGL(window, gl_context)
ImGui_ImplOpenGL3_Init(glsl_version)

// Load Fonts
// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
// - Read 'docs/FONTS.md' for more instructions and details.

//io.Fonts.AddFontDefault()
//io.Fonts.AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
//io.Fonts.AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
//io.Fonts.AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
//io.Fonts.AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
//let font = io.Fonts.AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nil, io.Fonts.GetGlyphRangesJapanese())
//assert(font != nil)

// Our state
var show_demo_window = true
var show_another_window = false
var clear_color: [Float] = ([0.45, 0.55, 0.60, 1.0])

// Main loop
var done = false

var f: Float = 0.0
var counter = 0

while !done {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    var event = SDL_Event()
    while SDL_PollEvent(&event) != 0 {
        if (event.type == SDL_QUIT.rawValue) {
            done = true
        }
        ImGui_ImplSDL2_ProcessEvent(&event)
        if event.type == SDL_WINDOWEVENT.rawValue && event.window.event == SDL_WINDOWEVENT_CLOSE.rawValue && event.window.windowID == SDL_GetWindowID(window) {
            done = true
        }
    }
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame()
    ImGui_ImplSDL2_NewFrame()
    ImGui.NewFrame()

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if show_demo_window {
        ImGui.ShowDemoWindow(&show_demo_window)
    }

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    ImGui.Begin("Hello, world!", nil, 0)                          // Create a window called "Hello, world!" and append into it.

    ImGui.TextUnformatted("This is some useful text.", nil)               // Display some text (you can use a format strings too)
    ImGui.Checkbox("Demo Window", &show_demo_window)      // Edit bools storing our window open/close state
    ImGui.Checkbox("Another Window", &show_another_window)

    ImGui.SliderFloat("float", &f, 0.0, 1.0, nil, 0)            // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui.ColorEdit3("clear color", &clear_color, 0) // Edit 3 floats representing a color

    if ImGui.Button("Button", ImVec2(0, 0)) {                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter += 1
    }
    ImGui.SameLine(0, 0)
    ImGui.TextUnformatted("counter = \(counter)", nil)

    ImGui.TextUnformatted(String(format: "Application average %.3f ms/frame (%.1f FPS)", 1000.0 / io.pointee.Framerate, io.pointee.Framerate), nil)
    ImGui.End()


    // 3. Show another simple window.
    if show_another_window {
        ImGui.Begin("Another Window", &show_another_window, 0)   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui.TextUnformatted("Hello from another window!", nil)
        if ImGui.Button("Close Me", ImVec2(0, 0)) {
            show_another_window = false
        }
        ImGui.End()
    }

    // Rendering
    ImGui.Render()
    glViewport(0, 0, GLsizei(io.pointee.DisplaySize.x), GLsizei(io.pointee.DisplaySize.y))
    glClearColor(clear_color[0] * clear_color[3], clear_color[1] * clear_color[3], clear_color[2] * clear_color[3], clear_color[3])
    glClear(UInt32(GL_COLOR_BUFFER_BIT))
    ImGui_ImplOpenGL3_RenderDrawData(ImGui.GetDrawData())
    SDL_GL_SwapWindow(window)
}

// Cleanup
ImGui_ImplOpenGL3_Shutdown()
ImGui_ImplSDL2_Shutdown()
ImGui.DestroyContext(context)

SDL_GL_DeleteContext(gl_context)
SDL_DestroyWindow(window)
SDL_Quit()
exit(0)
