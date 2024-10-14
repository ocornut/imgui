// dear imgui, v1.91.4 WIP
// (main code and documentation)

// Help:
// - See links below.
// - Call and read ImGui::ShowDemoWindow() in imgui_demo.cpp. All applications in examples/ are doing that.
// - Read top of imgui.cpp for more details, links and comments.

// Resources:
// - FAQ ........................ https://dearimgui.com/faq (in repository as docs/FAQ.md)
// - Homepage ................... https://github.com/ocornut/imgui
// - Releases & changelog ....... https://github.com/ocornut/imgui/releases
// - Gallery .................... https://github.com/ocornut/imgui/issues?q=label%3Agallery (please post your screenshots/video there!)
// - Wiki ....................... https://github.com/ocornut/imgui/wiki (lots of good stuff there)
//   - Getting Started            https://github.com/ocornut/imgui/wiki/Getting-Started (how to integrate in an existing app by adding ~25 lines of code)
//   - Third-party Extensions     https://github.com/ocornut/imgui/wiki/Useful-Extensions (ImPlot & many more)
//   - Bindings/Backends          https://github.com/ocornut/imgui/wiki/Bindings (language bindings, backends for various tech/engines)
//   - Glossary                   https://github.com/ocornut/imgui/wiki/Glossary
//   - Debug Tools                https://github.com/ocornut/imgui/wiki/Debug-Tools
//   - Software using Dear ImGui  https://github.com/ocornut/imgui/wiki/Software-using-dear-imgui
// - Issues & support ........... https://github.com/ocornut/imgui/issues
// - Test Engine & Automation ... https://github.com/ocornut/imgui_test_engine (test suite, test engine to automate your apps)

// For first-time users having issues compiling/linking/running/loading fonts:
// please post in https://github.com/ocornut/imgui/discussions if you cannot find a solution in resources above.
// Everything else should be asked in 'Issues'! We are building a database of cross-linked knowledge there.

// Copyright (c) 2014-2024 Omar Cornut
// Developed by Omar Cornut and every direct or indirect contributors to the GitHub.
// See LICENSE.txt for copyright and licensing details (standard MIT License).
// This library is free but needs your support to sustain development and maintenance.
// Businesses: you can support continued development via B2B invoiced technical support, maintenance and sponsoring contracts.
// PLEASE reach out at omar AT dearimgui DOT com. See https://github.com/ocornut/imgui/wiki/Funding
// Businesses: you can also purchase licenses for the Dear ImGui Automation/Test Engine.

// It is recommended that you don't modify imgui.cpp! It will become difficult for you to update the library.
// Note that 'ImGui::' being a namespace, you can add functions into the namespace from your own source files, without
// modifying imgui.h or imgui.cpp. You may include imgui_internal.h to access internal data structures, but it doesn't
// come with any guarantee of forward compatibility. Discussing your changes on the GitHub Issue Tracker may lead you
// to a better solution or official support for them.

/*

Index of this file:

DOCUMENTATION

- MISSION STATEMENT
- CONTROLS GUIDE
- PROGRAMMER GUIDE
  - READ FIRST
  - HOW TO UPDATE TO A NEWER VERSION OF DEAR IMGUI
  - GETTING STARTED WITH INTEGRATING DEAR IMGUI IN YOUR CODE/ENGINE
  - HOW A SIMPLE APPLICATION MAY LOOK LIKE
  - HOW A SIMPLE RENDERING FUNCTION MAY LOOK LIKE
- API BREAKING CHANGES (read me when you update!)
- FREQUENTLY ASKED QUESTIONS (FAQ)
  - Read all answers online: https://www.dearimgui.com/faq, or in docs/FAQ.md (with a Markdown viewer)

CODE
(search for "[SECTION]" in the code to find them)

// [SECTION] INCLUDES
// [SECTION] FORWARD DECLARATIONS
// [SECTION] CONTEXT AND MEMORY ALLOCATORS
// [SECTION] USER FACING STRUCTURES (ImGuiStyle, ImGuiIO, ImGuiPlatformIO)
// [SECTION] MISC HELPERS/UTILITIES (Geometry functions)
// [SECTION] MISC HELPERS/UTILITIES (String, Format, Hash functions)
// [SECTION] MISC HELPERS/UTILITIES (File functions)
// [SECTION] MISC HELPERS/UTILITIES (ImText* functions)
// [SECTION] MISC HELPERS/UTILITIES (Color functions)
// [SECTION] ImGuiStorage
// [SECTION] ImGuiTextFilter
// [SECTION] ImGuiTextBuffer, ImGuiTextIndex
// [SECTION] ImGuiListClipper
// [SECTION] STYLING
// [SECTION] RENDER HELPERS
// [SECTION] INITIALIZATION, SHUTDOWN
// [SECTION] MAIN CODE (most of the code! lots of stuff, needs tidying up!)
// [SECTION] ID STACK
// [SECTION] INPUTS
// [SECTION] ERROR CHECKING, STATE RECOVERY
// [SECTION] ITEM SUBMISSION
// [SECTION] LAYOUT
// [SECTION] SCROLLING
// [SECTION] TOOLTIPS
// [SECTION] POPUPS
// [SECTION] KEYBOARD/GAMEPAD NAVIGATION
// [SECTION] DRAG AND DROP
// [SECTION] LOGGING/CAPTURING
// [SECTION] SETTINGS
// [SECTION] LOCALIZATION
// [SECTION] VIEWPORTS, PLATFORM WINDOWS
// [SECTION] PLATFORM DEPENDENT HELPERS
// [SECTION] METRICS/DEBUGGER WINDOW
// [SECTION] DEBUG LOG WINDOW
// [SECTION] OTHER DEBUG TOOLS (ITEM PICKER, ID STACK TOOL)

*/

//-----------------------------------------------------------------------------
// DOCUMENTATION
//-----------------------------------------------------------------------------

/*

 MISSION STATEMENT
 =================

 - Easy to use to create code-driven and data-driven tools.
 - Easy to use to create ad hoc short-lived tools and long-lived, more elaborate tools.
 - Easy to hack and improve.
 - Minimize setup and maintenance.
 - Minimize state storage on user side.
 - Minimize state synchronization.
 - Portable, minimize dependencies, run on target (consoles, phones, etc.).
 - Efficient runtime and memory consumption.

 Designed primarily for developers and content-creators, not the typical end-user!
 Some of the current weaknesses (which we aim to address in the future) includes:

 - Doesn't look fancy.
 - Limited layout features, intricate layouts are typically crafted in code.


 CONTROLS GUIDE
 ==============

 - MOUSE CONTROLS
   - Mouse wheel:                   Scroll vertically.
   - SHIFT+Mouse wheel:             Scroll horizontally.
   - Click [X]:                     Close a window, available when 'bool* p_open' is passed to ImGui::Begin().
   - Click ^, Double-Click title:   Collapse window.
   - Drag on corner/border:         Resize window (double-click to auto fit window to its contents).
   - Drag on any empty space:       Move window (unless io.ConfigWindowsMoveFromTitleBarOnly = true).
   - Left-click outside popup:      Close popup stack (right-click over underlying popup: Partially close popup stack).

 - TEXT EDITOR
   - Hold SHIFT or Drag Mouse:      Select text.
   - CTRL+Left/Right:               Word jump.
   - CTRL+Shift+Left/Right:         Select words.
   - CTRL+A or Double-Click:        Select All.
   - CTRL+X, CTRL+C, CTRL+V:        Use OS clipboard.
   - CTRL+Z, CTRL+Y:                Undo, Redo.
   - ESCAPE:                        Revert text to its original value.
   - On OSX, controls are automatically adjusted to match standard OSX text editing 2ts and behaviors.

 - KEYBOARD CONTROLS
   - Basic:
     - Tab, SHIFT+Tab               Cycle through text editable fields.
     - CTRL+Tab, CTRL+Shift+Tab     Cycle through windows.
     - CTRL+Click                   Input text into a Slider or Drag widget.
   - Extended features with `io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard`:
     - Tab, SHIFT+Tab:              Cycle through every items.
     - Arrow keys                   Move through items using directional navigation. Tweak value.
     - Arrow keys + Alt, Shift      Tweak slower, tweak faster (when using arrow keys).
     - Enter                        Activate item (prefer text input when possible).
     - Space                        Activate item (prefer tweaking with arrows when possible).
     - Escape                       Deactivate item, leave child window, close popup.
     - Page Up, Page Down           Previous page, next page.
     - Home, End                    Scroll to top, scroll to bottom.
     - Alt                          Toggle between scrolling layer and menu layer.
     - CTRL+Tab then Ctrl+Arrows    Move window. Hold SHIFT to resize instead of moving.
   - Output when ImGuiConfigFlags_NavEnableKeyboard set,
     - io.WantCaptureKeyboard flag is set when keyboard is claimed.
     - io.NavActive: true when a window is focused and it doesn't have the ImGuiWindowFlags_NoNavInputs flag set.
     - io.NavVisible: true when the navigation cursor is visible (usually goes to back false when mouse is used).

 - GAMEPAD CONTROLS
   - Enable with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
   - Particularly useful to use Dear ImGui on a console system (e.g. PlayStation, Switch, Xbox) without a mouse!
   - Download controller mapping PNG/PSD at http://dearimgui.com/controls_sheets
   - Backend support: backend needs to:
      - Set 'io.BackendFlags |= ImGuiBackendFlags_HasGamepad' + call io.AddKeyEvent/AddKeyAnalogEvent() with ImGuiKey_Gamepad_XXX keys.
      - For analog values (0.0f to 1.0f), backend is responsible to handling a dead-zone and rescaling inputs accordingly.
        Backend code will probably need to transform your raw inputs (such as e.g. remapping your 0.2..0.9 raw input range to 0.0..1.0 imgui range, etc.).
      - BEFORE 1.87, BACKENDS USED TO WRITE TO io.NavInputs[]. This is now obsolete. Please call io functions instead!
   - If you need to share inputs between your game and the Dear ImGui interface, the easiest approach is to go all-or-nothing,
     with a buttons combo to toggle the target. Please reach out if you think the game vs navigation input sharing could be improved.

 - REMOTE INPUTS SHARING & MOUSE EMULATION
   - PS4/PS5 users: Consider emulating a mouse cursor with DualShock touch pad or a spare analog stick as a mouse-emulation fallback.
   - Consoles/Tablet/Phone users: Consider using a Synergy 1.x server (on your PC) + run examples/libs/synergy/uSynergy.c (on your console/tablet/phone app)
     in order to share your PC mouse/keyboard.
   - See https://github.com/ocornut/imgui/wiki/Useful-Extensions#remoting for other remoting solutions.
   - On a TV/console system where readability may be lower or mouse inputs may be awkward, you may want to set the io.ConfigNavMoveSetMousePos flag.
     Enabling io.ConfigNavMoveSetMousePos + ImGuiBackendFlags_HasSetMousePos instructs Dear ImGui to move your mouse cursor along with navigation movements.
     When enabled, the NewFrame() function may alter 'io.MousePos' and set 'io.WantSetMousePos' to notify you that it wants the mouse cursor to be moved.
     When that happens your backend NEEDS to move the OS or underlying mouse cursor on the next frame. Some of the backends in examples/ do that.
     (If you set the NavEnableSetMousePos flag but don't honor 'io.WantSetMousePos' properly, Dear ImGui will misbehave as it will see your mouse moving back & forth!)
     (In a setup when you may not have easy control over the mouse cursor, e.g. uSynergy.c doesn't expose moving remote mouse cursor, you may want
     to set a boolean to ignore your other external mouse positions until the external source is moved again.)


 PROGRAMMER GUIDE
 ================

 READ FIRST
 ----------
 - Remember to check the wonderful Wiki (https://github.com/ocornut/imgui/wiki)
 - Your code creates the UI every frame of your application loop, if your code doesn't run the UI is gone!
   The UI can be highly dynamic, there are no construction or destruction steps, less superfluous
   data retention on your side, less state duplication, less state synchronization, fewer bugs.
 - Call and read ImGui::ShowDemoWindow() for demo code demonstrating most features.
   Or browse https://pthom.github.io/imgui_manual_online/manual/imgui_manual.html for interactive web version.
 - The library is designed to be built from sources. Avoid pre-compiled binaries and packaged versions. See imconfig.h to configure your build.
 - Dear ImGui is an implementation of the IMGUI paradigm (immediate-mode graphical user interface, a term coined by Casey Muratori).
   You can learn about IMGUI principles at http://www.johno.se/book/imgui.html, http://mollyrocket.com/861 & more links in Wiki.
 - Dear ImGui is a "single pass" rasterizing implementation of the IMGUI paradigm, aimed at ease of use and high-performances.
   For every application frame, your UI code will be called only once. This is in contrast to e.g. Unity's implementation of an IMGUI,
   where the UI code is called multiple times ("multiple passes") from a single entry point. There are pros and cons to both approaches.
 - Our origin is on the top-left. In axis aligned bounding boxes, Min = top-left, Max = bottom-right.
 - Please make sure you have asserts enabled (IM_ASSERT redirects to assert() by default, but can be redirected).
   If you get an assert, read the messages and comments around the assert.
 - This codebase aims to be highly optimized:
   - A typical idle frame should never call malloc/free.
   - We rely on a maximum of constant-time or O(N) algorithms. Limiting searches/scans as much as possible.
   - We put particular energy in making sure performances are decent with typical "Debug" build settings as well.
     Which mean we tend to avoid over-relying on "zero-cost abstraction" as they aren't zero-cost at all.
 - This codebase aims to be both highly opinionated and highly flexible:
   - This code works because of the things it choose to solve or not solve.
   - C++: this is a pragmatic C-ish codebase: we don't use fancy C++ features, we don't include C++ headers,
     and ImGui:: is a namespace. We rarely use member functions (and when we did, I am mostly regretting it now).
     This is to increase compatibility, increase maintainability and facilitate use from other languages.
   - C++: ImVec2/ImVec4 do not expose math operators by default, because it is expected that you use your own math types.
     See FAQ "How can I use my own math types instead of ImVec2/ImVec4?" for details about setting up imconfig.h for that.
     We can can optionally export math operators for ImVec2/ImVec4 using IMGUI_DEFINE_MATH_OPERATORS, which we use internally.
   - C++: pay attention that ImVector<> manipulates plain-old-data and does not honor construction/destruction
     (so don't use ImVector in your code or at our own risk!).
   - Building: We don't use nor mandate a build system for the main library.
     This is in an effort to ensure that it works in the real world aka with any esoteric build setup.
     This is also because providing a build system for the main library would be of little-value.
     The build problems are almost never coming from the main library but from specific backends.


 HOW TO UPDATE TO A NEWER VERSION OF DEAR IMGUI
 ----------------------------------------------
 - Update submodule or copy/overwrite every file.
 - About imconfig.h:
   - You may modify your copy of imconfig.h, in this case don't overwrite it.
   - or you may locally branch to modify imconfig.h and merge/rebase latest.
   - or you may '#define IMGUI_USER_CONFIG "my_config_file.h"' globally from your build system to
     specify a custom path for your imconfig.h file and instead not have to modify the default one.

 - Overwrite all the sources files except for imconfig.h (if you have modified your copy of imconfig.h)
 - Or maintain your own branch where you have imconfig.h modified as a top-most commit which you can regularly rebase over "master".
 - You can also use '#define IMGUI_USER_CONFIG "my_config_file.h" to redirect configuration to your own file.
 - Read the "API BREAKING CHANGES" section (below). This is where we list occasional API breaking changes.
   If a function/type has been renamed / or marked obsolete, try to fix the name in your code before it is permanently removed
   from the public API. If you have a problem with a missing function/symbols, search for its name in the code, there will
   likely be a comment about it. Please report any issue to the GitHub page!
 - To find out usage of old API, you can add '#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS' in your configuration file.
 - Try to keep your copy of Dear ImGui reasonably up to date!


 GETTING STARTED WITH INTEGRATING DEAR IMGUI IN YOUR CODE/ENGINE
 ---------------------------------------------------------------
 - See https://github.com/ocornut/imgui/wiki/Getting-Started.
 - Run and study the examples and demo in imgui_demo.cpp to get acquainted with the library.
 - In the majority of cases you should be able to use unmodified backends files available in the backends/ folder.
 - Add the Dear ImGui source files + selected backend source files to your projects or using your preferred build system.
   It is recommended you build and statically link the .cpp files as part of your project and NOT as a shared library (DLL).
 - You can later customize the imconfig.h file to tweak some compile-time behavior, such as integrating Dear ImGui types with your own maths types.
 - When using Dear ImGui, your programming IDE is your friend: follow the declaration of variables, functions and types to find comments about them.
 - Dear ImGui never touches or knows about your GPU state. The only function that knows about GPU is the draw function that you provide.
   Effectively it means you can create widgets at any time in your code, regardless of considerations of being in "update" vs "render"
   phases of your own application. All rendering information is stored into command-lists that you will retrieve after calling ImGui::Render().
 - Refer to the backends and demo applications in the examples/ folder for instruction on how to setup your code.
 - If you are running over a standard OS with a common graphics API, you should be able to use unmodified imgui_impl_*** files from the examples/ folder.


 HOW A SIMPLE APPLICATION MAY LOOK LIKE
 --------------------------------------
 EXHIBIT 1: USING THE EXAMPLE BACKENDS (= imgui_impl_XXX.cpp files from the backends/ folder).
 The sub-folders in examples/ contain examples applications following this structure.

     // Application init: create a dear imgui context, setup some options, load fonts
     ImGui::CreateContext();
     ImGuiIO& io = ImGui::GetIO();
     // TODO: Set optional io.ConfigFlags values, e.g. 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard' to enable keyboard controls.
     // TODO: Fill optional fields of the io structure later.
     // TODO: Load TTF/OTF fonts if you don't want to use the default font.

     // Initialize helper Platform and Renderer backends (here we are using imgui_impl_win32.cpp and imgui_impl_dx11.cpp)
     ImGui_ImplWin32_Init(hwnd);
     ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

     // Application main loop
     while (true)
     {
         // Feed inputs to dear imgui, start new frame
         ImGui_ImplDX11_NewFrame();
         ImGui_ImplWin32_NewFrame();
         ImGui::NewFrame();

         // Any application code here
         ImGui::Text("Hello, world!");

         // Render dear imgui into screen
         ImGui::Render();
         ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
         g_pSwapChain->Present(1, 0);
     }

     // Shutdown
     ImGui_ImplDX11_Shutdown();
     ImGui_ImplWin32_Shutdown();
     ImGui::DestroyContext();

 EXHIBIT 2: IMPLEMENTING CUSTOM BACKEND / CUSTOM ENGINE

     // Application init: create a dear imgui context, setup some options, load fonts
     ImGui::CreateContext();
     ImGuiIO& io = ImGui::GetIO();
     // TODO: Set optional io.ConfigFlags values, e.g. 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard' to enable keyboard controls.
     // TODO: Fill optional fields of the io structure later.
     // TODO: Load TTF/OTF fonts if you don't want to use the default font.

     // Build and load the texture atlas into a texture
     // (In the examples/ app this is usually done within the ImGui_ImplXXX_Init() function from one of the demo Renderer)
     int width, height;
     unsigned char* pixels = nullptr;
     io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

     // At this point you've got the texture data and you need to upload that to your graphic system:
     // After we have created the texture, store its pointer/identifier (_in whichever format your engine uses_) in 'io.Fonts->TexID'.
     // This will be passed back to your via the renderer. Basically ImTextureID == void*. Read FAQ for details about ImTextureID.
     MyTexture* texture = MyEngine::CreateTextureFromMemoryPixels(pixels, width, height, TEXTURE_TYPE_RGBA32)
     io.Fonts->SetTexID((void*)texture);

     // Application main loop
     while (true)
     {
        // Setup low-level inputs, e.g. on Win32: calling GetKeyboardState(), or write to those fields from your Windows message handlers, etc.
        // (In the examples/ app this is usually done within the ImGui_ImplXXX_NewFrame() function from one of the demo Platform Backends)
        io.DeltaTime = 1.0f/60.0f;              // set the time elapsed since the previous frame (in seconds)
        io.DisplaySize.x = 1920.0f;             // set the current display width
        io.DisplaySize.y = 1280.0f;             // set the current display height here
        io.AddMousePosEvent(mouse_x, mouse_y);  // update mouse position
        io.AddMouseButtonEvent(0, mouse_b[0]);  // update mouse button states
        io.AddMouseButtonEvent(1, mouse_b[1]);  // update mouse button states

        // Call NewFrame(), after this point you can use ImGui::* functions anytime
        // (So you want to try calling NewFrame() as early as you can in your main loop to be able to use Dear ImGui everywhere)
        ImGui::NewFrame();

        // Most of your application code here
        ImGui::Text("Hello, world!");
        MyGameUpdate(); // may use any Dear ImGui functions, e.g. ImGui::Begin("My window"); ImGui::Text("Hello, world!"); ImGui::End();
        MyGameRender(); // may use any Dear ImGui functions as well!

        // Render dear imgui, swap buffers
        // (You want to try calling EndFrame/Render as late as you can, to be able to use Dear ImGui in your own game rendering code)
        ImGui::EndFrame();
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        MyImGuiRenderFunction(draw_data);
        SwapBuffers();
     }

     // Shutdown
     ImGui::DestroyContext();

 To decide whether to dispatch mouse/keyboard inputs to Dear ImGui to the rest of your application,
 you should read the 'io.WantCaptureMouse', 'io.WantCaptureKeyboard' and 'io.WantTextInput' flags!
 Please read the FAQ entry "How can I tell whether to dispatch mouse/keyboard to Dear ImGui or my application?" about this.


 HOW A SIMPLE RENDERING FUNCTION MAY LOOK LIKE
 ---------------------------------------------
 The backends in impl_impl_XXX.cpp files contain many working implementations of a rendering function.

    void MyImGuiRenderFunction(ImDrawData* draw_data)
    {
       // TODO: Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
       // TODO: Setup texture sampling state: sample with bilinear filtering (NOT point/nearest filtering). Use 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines;' to allow point/nearest filtering.
       // TODO: Setup viewport covering draw_data->DisplayPos to draw_data->DisplayPos + draw_data->DisplaySize
       // TODO: Setup orthographic projection matrix cover draw_data->DisplayPos to draw_data->DisplayPos + draw_data->DisplaySize
       // TODO: Setup shader: vertex { float2 pos, float2 uv, u32 color }, fragment shader sample color from 1 texture, multiply by vertex color.
       ImVec2 clip_off = draw_data->DisplayPos;
       for (int n = 0; n < draw_data->CmdListsCount; n++)
       {
          const ImDrawList* cmd_list = draw_data->CmdLists[n];
          const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;  // vertex buffer generated by Dear ImGui
          const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;   // index buffer generated by Dear ImGui
          for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
          {
             const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
             if (pcmd->UserCallback)
             {
                 pcmd->UserCallback(cmd_list, pcmd);
             }
             else
             {
                 // Project scissor/clipping rectangles into framebuffer space
                 ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
                 ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
                 if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                     continue;

                 // We are using scissoring to clip some objects. All low-level graphics API should support it.
                 // - If your engine doesn't support scissoring yet, you may ignore this at first. You will get some small glitches
                 //   (some elements visible outside their bounds) but you can fix that once everything else works!
                 // - Clipping coordinates are provided in imgui coordinates space:
                 //   - For a given viewport, draw_data->DisplayPos == viewport->Pos and draw_data->DisplaySize == viewport->Size
                 //   - In a single viewport application, draw_data->DisplayPos == (0,0) and draw_data->DisplaySize == io.DisplaySize, but always use GetMainViewport()->Pos/Size instead of hardcoding those values.
                 //   - In the interest of supporting multi-viewport applications (see 'docking' branch on github),
                 //     always subtract draw_data->DisplayPos from clipping bounds to convert them to your viewport space.
                 // - Note that pcmd->ClipRect contains Min+Max bounds. Some graphics API may use Min+Max, other may use Min+Size (size being Max-Min)
                 MyEngineSetScissor(clip_min.x, clip_min.y, clip_max.x, clip_max.y);

                 // The texture for the draw call is specified by pcmd->GetTexID().
                 // The vast majority of draw calls will use the Dear ImGui texture atlas, which value you have set yourself during initialization.
                 MyEngineBindTexture((MyTexture*)pcmd->GetTexID());

                 // Render 'pcmd->ElemCount/3' indexed triangles.
                 // By default the indices ImDrawIdx are 16-bit, you can change them to 32-bit in imconfig.h if your engine doesn't support 16-bit indices.
                 MyEngineDrawIndexedTriangles(pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer + pcmd->IdxOffset, vtx_buffer, pcmd->VtxOffset);
             }
          }
       }
    }


 API BREAKING CHANGES
 ====================

 Occasionally introducing changes that are breaking the API. We try to make the breakage minor and easy to fix.
 Below is a change-log of API breaking changes only. If you are using one of the functions listed, expect to have to fix some code.
 When you are not sure about an old symbol or function name, try using the Search/Find function of your IDE to look for comments or references in all imgui files.
 You can read releases logs https://github.com/ocornut/imgui/releases for more details.

 - 2024/10/14 (1.91.4) - moved ImGuiConfigFlags_NavEnableSetMousePos to standalone io.ConfigNavMoveSetMousePos bool.
                         moved ImGuiConfigFlags_NavNoCaptureKeyboard to standalone io.ConfigNavCaptureKeyboard bool (note the inverted value!).
                         kept legacy names (will obsolete) + code that copies settings once the first time. Dynamically changing the old value won't work. Switch to using the new value!
 - 2024/10/10 (1.91.4) - the typedef for ImTextureID now defaults to ImU64 instead of void*. (#1641)
                         this removes the requirement to redefine it for backends which are e.g. storing descriptor sets or other 64-bits structures when building on 32-bits archs. It therefore simplify various building scripts/helpers.
                         you may have compile-time issues if you were casting to 'void*' instead of 'ImTextureID' when passing your types to functions taking ImTextureID values, e.g. ImGui::Image().
                         in doubt it is almost always better to do an intermediate intptr_t cast, since it allows casting any pointer/integer type without warning:
                            - May warn:    ImGui::Image((void*)MyTextureData, ...);
                            - May warn:    ImGui::Image((void*)(intptr_t)MyTextureData, ...);
                            - Won't warn:  ImGui::Image((ImTextureID)(intptr_t)MyTextureData), ...);
  -                      note that you can always define ImTextureID to be your own high-level structures (with dedicated constructors) if you like.
 - 2024/10/03 (1.91.3) - drags: treat v_min==v_max as a valid clamping range when != 0.0f. Zero is a still special value due to legacy reasons, unless using ImGuiSliderFlags_ClampZeroRange. (#7968, #3361, #76)
                       - drags: extended behavior of ImGuiSliderFlags_AlwaysClamp to include _ClampZeroRange. It considers v_min==v_max==0.0f as a valid clamping range (aka edits not allowed).
                         although unlikely, it you wish to only clamp on text input but want v_min==v_max==0.0f to mean unclamped drags, you can use _ClampOnInput instead of _AlwaysClamp. (#7968, #3361, #76)
 - 2024/09/10 (1.91.2) - internals: using multiple overlayed ButtonBehavior() with same ID will now have io.ConfigDebugHighlightIdConflicts=true feature emit a warning. (#8030)
                         it was one of the rare case where using same ID is legal. workarounds: (1) use single ButtonBehavior() call with multiple _MouseButton flags, or (2) surround the calls with PushItemFlag(ImGuiItemFlags_AllowDuplicateId, true); ... PopItemFlag()
 - 2024/08/23 (1.91.1) - renamed ImGuiChildFlags_Border to ImGuiChildFlags_Borders for consistency. kept inline redirection flag.
 - 2024/08/22 (1.91.1) - moved some functions from ImGuiIO to ImGuiPlatformIO structure:
                            - io.GetClipboardTextFn         -> platform_io.Platform_GetClipboardTextFn + changed 'void* user_data' to 'ImGuiContext* ctx'. Pull your user data from platform_io.ClipboardUserData.
                            - io.SetClipboardTextFn         -> platform_io.Platform_SetClipboardTextFn + same as above line.
                            - io.PlatformOpenInShellFn      -> platform_io.Platform_OpenInShellFn (#7660)
                            - io.PlatformSetImeDataFn       -> platform_io.Platform_SetImeDataFn
                            - io.PlatformLocaleDecimalPoint -> platform_io.Platform_LocaleDecimalPoint (#7389, #6719, #2278)
                            - access those via GetPlatformIO() instead of GetIO().
                         some were introduced very recently and often automatically setup by core library and backends, so for those we are exceptionally not maintaining a legacy redirection symbol.
                       - commented the old ImageButton() signature obsoleted in 1.89 (~August 2022). As a reminder:
                            - old ImageButton() before 1.89 used ImTextureId as item id (created issue with e.g. multiple buttons in same scope, transient texture id values, opaque computation of ID)
                            - new ImageButton() since 1.89 requires an explicit 'const char* str_id'
                            - old ImageButton() before 1.89 had frame_padding' override argument.
                            - new ImageButton() since 1.89 always use style.FramePadding, which you can freely override with PushStyleVar()/PopStyleVar().
 - 2024/07/25 (1.91.0) - obsoleted GetContentRegionMax(), GetWindowContentRegionMin() and GetWindowContentRegionMax(). (see #7838 on GitHub for more info)
                         you should never need those functions. you can do everything with GetCursorScreenPos() and GetContentRegionAvail() in a more simple way.
                            - instead of:  GetWindowContentRegionMax().x - GetCursorPos().x
                            - you can use: GetContentRegionAvail().x
                            - instead of:  GetWindowContentRegionMax().x + GetWindowPos().x
                            - you can use: GetCursorScreenPos().x + GetContentRegionAvail().x // when called from left edge of window
                            - instead of:  GetContentRegionMax()
                            - you can use: GetContentRegionAvail() + GetCursorScreenPos() - GetWindowPos() // right edge in local coordinates
                            - instead of:  GetWindowContentRegionMax().x - GetWindowContentRegionMin().x
                            - you can use: GetContentRegionAvail() // when called from left edge of window
 - 2024/07/15 (1.91.0) - renamed ImGuiSelectableFlags_DontClosePopups to ImGuiSelectableFlags_NoAutoClosePopups. (#1379, #1468, #2200, #4936, #5216, #7302, #7573)
                         (internals: also renamed ImGuiItemFlags_SelectableDontClosePopup into ImGuiItemFlags_AutoClosePopups with inverted behaviors)
 - 2024/07/15 (1.91.0) - obsoleted PushButtonRepeat()/PopButtonRepeat() in favor of using new PushItemFlag(ImGuiItemFlags_ButtonRepeat, ...)/PopItemFlag().
 - 2024/07/02 (1.91.0) - commented out obsolete ImGuiModFlags (renamed to ImGuiKeyChord in 1.89). (#4921, #456)
                       - commented out obsolete ImGuiModFlags_XXX values (renamed to ImGuiMod_XXX in 1.89). (#4921, #456)
                            - ImGuiModFlags_Ctrl -> ImGuiMod_Ctrl, ImGuiModFlags_Shift -> ImGuiMod_Shift etc.
 - 2024/07/02 (1.91.0) - IO, IME: renamed platform IME hook and added explicit context for consistency and future-proofness.
                            - old: io.SetPlatformImeDataFn(ImGuiViewport* viewport, ImGuiPlatformImeData* data);
                            - new: io.PlatformSetImeDataFn(ImGuiContext* ctx, ImGuiViewport* viewport, ImGuiPlatformImeData* data);
 - 2024/06/21 (1.90.9) - BeginChild: added ImGuiChildFlags_NavFlattened as a replacement for the window flag ImGuiWindowFlags_NavFlattened: the feature only ever made sense for BeginChild() anyhow.
                            - old: BeginChild("Name", size, 0, ImGuiWindowFlags_NavFlattened);
                            - new: BeginChild("Name", size, ImGuiChildFlags_NavFlattened, 0);
 - 2024/06/21 (1.90.9) - io: ClearInputKeys() (first exposed in 1.89.8) doesn't clear mouse data, newly added ClearInputMouse() does.
 - 2024/06/20 (1.90.9) - renamed ImGuiDragDropFlags_SourceAutoExpirePayload to ImGuiDragDropFlags_PayloadAutoExpire.
 - 2024/06/18 (1.90.9) - style: renamed ImGuiCol_TabActive -> ImGuiCol_TabSelected, ImGuiCol_TabUnfocused -> ImGuiCol_TabDimmed, ImGuiCol_TabUnfocusedActive -> ImGuiCol_TabDimmedSelected.
 - 2024/06/10 (1.90.9) - removed old nested structure: renaming ImGuiStorage::ImGuiStoragePair type to ImGuiStoragePair (simpler for many languages).
 - 2024/06/06 (1.90.8) - reordered ImGuiInputTextFlags values. This should not be breaking unless you are using generated headers that have values not matching the main library.
 - 2024/06/06 (1.90.8) - removed 'ImGuiButtonFlags_MouseButtonDefault_ = ImGuiButtonFlags_MouseButtonLeft', was mostly unused and misleading.
 - 2024/05/27 (1.90.7) - commented out obsolete symbols marked obsolete in 1.88 (May 2022):
                            - old: CaptureKeyboardFromApp(bool)
                            - new: SetNextFrameWantCaptureKeyboard(bool)
                            - old: CaptureMouseFromApp(bool)
                            - new: SetNextFrameWantCaptureMouse(bool)
 - 2024/05/22 (1.90.7) - inputs (internals): renamed ImGuiKeyOwner_None to ImGuiKeyOwner_NoOwner, to make use more explicit and reduce confusion with the default it is a non-zero value and cannot be the default value (never made public, but disclosing as I expect a few users caught on owner-aware inputs).
                       - inputs (internals): renamed ImGuiInputFlags_RouteGlobalLow -> ImGuiInputFlags_RouteGlobal, ImGuiInputFlags_RouteGlobal -> ImGuiInputFlags_RouteGlobalOverFocused, ImGuiInputFlags_RouteGlobalHigh -> ImGuiInputFlags_RouteGlobalHighest.
                       - inputs (internals): Shortcut(), SetShortcutRouting(): swapped last two parameters order in function signatures:
                            - old: Shortcut(ImGuiKeyChord key_chord, ImGuiID owner_id = 0, ImGuiInputFlags flags = 0);
                            - new: Shortcut(ImGuiKeyChord key_chord, ImGuiInputFlags flags = 0, ImGuiID owner_id = 0);
                       - inputs (internals): owner-aware versions of IsKeyPressed(), IsKeyChordPressed(), IsMouseClicked(): swapped last two parameters order in function signatures.
                            - old: IsKeyPressed(ImGuiKey key, ImGuiID owner_id, ImGuiInputFlags flags = 0);
                            - new: IsKeyPressed(ImGuiKey key, ImGuiInputFlags flags, ImGuiID owner_id = 0);
                            - old: IsMouseClicked(ImGuiMouseButton button, ImGuiID owner_id, ImGuiInputFlags flags = 0);
                            - new: IsMouseClicked(ImGuiMouseButton button, ImGuiInputFlags flags, ImGuiID owner_id = 0);
                         for various reasons those changes makes sense. They are being made because making some of those API public.
                         only past users of imgui_internal.h with the extra parameters will be affected. Added asserts for valid flags in various functions to detect _some_ misuses, BUT NOT ALL.
 - 2024/05/16 (1.90.7) - inputs: on macOS X, Cmd and Ctrl keys are now automatically swapped by io.AddKeyEvent() as this naturally align with how macOS X uses those keys.
                           - it shouldn't really affect you unless you had custom shortcut swapping in place for macOS X apps.
                           - removed ImGuiMod_Shortcut which was previously dynamically remapping to Ctrl or Cmd/Super. It is now unnecessary to specific cross-platform idiomatic shortcuts. (#2343, #4084, #5923, #456)
 - 2024/05/14 (1.90.7) - backends: SDL_Renderer2 and SDL_Renderer3 backend now take a SDL_Renderer* in their RenderDrawData() functions.
 - 2024/04/18 (1.90.6) - TreeNode: Fixed a layout inconsistency when using an empty/hidden label followed by a SameLine() call. (#7505, #282)
                           - old: TreeNode("##Hidden"); SameLine(); Text("Hello");     // <-- This was actually incorrect! BUT appeared to look ok with the default style where ItemSpacing.x == FramePadding.x * 2 (it didn't look aligned otherwise).
                           - new: TreeNode("##Hidden"); SameLine(0, 0); Text("Hello"); // <-- This is correct for all styles values.
                         with the fix, IF you were successfully using TreeNode("")+SameLine(); you will now have extra spacing between your TreeNode and the following item.
                         You'll need to change the SameLine() call to SameLine(0,0) to remove this extraneous spacing. This seemed like the more sensible fix that's not making things less consistent.
                         (Note: when using this idiom you are likely to also use ImGuiTreeNodeFlags_SpanAvailWidth).
 - 2024/03/18 (1.90.5) - merged the radius_x/radius_y parameters in ImDrawList::AddEllipse(), AddEllipseFilled() and PathEllipticalArcTo() into a single ImVec2 parameter. Exceptionally, because those functions were added in 1.90, we are not adding inline redirection functions. The transition is easy and should affect few users. (#2743, #7417)
 - 2024/03/08 (1.90.5) - inputs: more formally obsoleted GetKeyIndex() when IMGUI_DISABLE_OBSOLETE_FUNCTIONS is set. It has been unnecessary and a no-op since 1.87 (it returns the same value as passed when used with a 1.87+ backend using io.AddKeyEvent() function). (#4921)
                           - IsKeyPressed(GetKeyIndex(ImGuiKey_XXX)) -> use IsKeyPressed(ImGuiKey_XXX)
 - 2024/01/15 (1.90.2) - commented out obsolete ImGuiIO::ImeWindowHandle marked obsolete in 1.87, favor of writing to 'void* ImGuiViewport::PlatformHandleRaw'.
 - 2023/12/19 (1.90.1) - commented out obsolete ImGuiKey_KeyPadEnter redirection to ImGuiKey_KeypadEnter.
 - 2023/11/06 (1.90.1) - removed CalcListClipping() marked obsolete in 1.86. Prefer using ImGuiListClipper which can return non-contiguous ranges.
 - 2023/11/05 (1.90.1) - imgui_freetype: commented out ImGuiFreeType::BuildFontAtlas() obsoleted in 1.81. prefer using #define IMGUI_ENABLE_FREETYPE or see commented code for manual calls.
 - 2023/11/05 (1.90.1) - internals,columns: commented out legacy ImGuiColumnsFlags_XXX symbols redirecting to ImGuiOldColumnsFlags_XXX, obsoleted from imgui_internal.h in 1.80.
 - 2023/11/09 (1.90.0) - removed IM_OFFSETOF() macro in favor of using offsetof() available in C++11. Kept redirection define (will obsolete).
 - 2023/11/07 (1.90.0) - removed BeginChildFrame()/EndChildFrame() in favor of using BeginChild() with the ImGuiChildFlags_FrameStyle flag. kept inline redirection function (will obsolete).
                         those functions were merely PushStyle/PopStyle helpers, the removal isn't so much motivated by needing to add the feature in BeginChild(), but by the necessity to avoid BeginChildFrame() signature mismatching BeginChild() signature and features.
 - 2023/11/02 (1.90.0) - BeginChild: upgraded 'bool border = true' parameter to 'ImGuiChildFlags flags' type, added ImGuiChildFlags_Border equivalent. As with our prior "bool-to-flags" API updates, the ImGuiChildFlags_Border value is guaranteed to be == true forever to ensure a smoother transition, meaning all existing calls will still work.
                           - old: BeginChild("Name", size, true)
                           - new: BeginChild("Name", size, ImGuiChildFlags_Border)
                           - old: BeginChild("Name", size, false)
                           - new: BeginChild("Name", size) or BeginChild("Name", 0) or BeginChild("Name", size, ImGuiChildFlags_None)
                         **AMEND FROM THE FUTURE: from 1.91.1, 'ImGuiChildFlags_Border' is called 'ImGuiChildFlags_Borders'**
 - 2023/11/02 (1.90.0) - BeginChild: added child-flag ImGuiChildFlags_AlwaysUseWindowPadding as a replacement for the window-flag ImGuiWindowFlags_AlwaysUseWindowPadding: the feature only ever made sense for BeginChild() anyhow.
                           - old: BeginChild("Name", size, 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
                           - new: BeginChild("Name", size, ImGuiChildFlags_AlwaysUseWindowPadding, 0);
 - 2023/09/27 (1.90.0) - io: removed io.MetricsActiveAllocations introduced in 1.63. Same as 'g.DebugMemAllocCount - g.DebugMemFreeCount' (still displayed in Metrics, unlikely to be accessed by end-user).
 - 2023/09/26 (1.90.0) - debug tools: Renamed ShowStackToolWindow() ("Stack Tool") to ShowIDStackToolWindow() ("ID Stack Tool"), as earlier name was misleading. Kept inline redirection function. (#4631)
 - 2023/09/15 (1.90.0) - ListBox, Combo: changed signature of "name getter" callback in old one-liner ListBox()/Combo() apis. kept inline redirection function (will obsolete).
                           - old: bool Combo(const char* label, int* current_item, bool (*getter)(void* user_data, int idx, const char** out_text), ...)
                           - new: bool Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), ...);
                           - old: bool ListBox(const char* label, int* current_item, bool (*getting)(void* user_data, int idx, const char** out_text), ...);
                           - new: bool ListBox(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), ...);
 - 2023/09/08 (1.90.0) - commented out obsolete redirecting functions:
                           - GetWindowContentRegionWidth()  -> use GetWindowContentRegionMax().x - GetWindowContentRegionMin().x. Consider that generally 'GetContentRegionAvail().x' is more useful.
                           - ImDrawCornerFlags_XXX          -> use ImDrawFlags_RoundCornersXXX flags. Read 1.82 Changelog for details + grep commented names in sources.
                       - commented out runtime support for hardcoded ~0 or 0x01..0x0F rounding flags values for AddRect()/AddRectFilled()/PathRect()/AddImageRounded() -> use ImDrawFlags_RoundCornersXXX flags. Read 1.82 Changelog for details
 - 2023/08/25 (1.89.9) - clipper: Renamed IncludeRangeByIndices() (also called ForceDisplayRangeByIndices() before 1.89.6) to IncludeItemsByIndex(). Kept inline redirection function. Sorry!
 - 2023/07/12 (1.89.8) - ImDrawData: CmdLists now owned, changed from ImDrawList** to ImVector<ImDrawList*>. Majority of users shouldn't be affected, but you cannot compare to NULL nor reassign manually anymore. Instead use AddDrawList(). (#6406, #4879, #1878)
 - 2023/06/28 (1.89.7) - overlapping items: obsoleted 'SetItemAllowOverlap()' (called after item) in favor of calling 'SetNextItemAllowOverlap()' (called before item). 'SetItemAllowOverlap()' didn't and couldn't work reliably since 1.89 (2022-11-15).
 - 2023/06/28 (1.89.7) - overlapping items: renamed 'ImGuiTreeNodeFlags_AllowItemOverlap' to 'ImGuiTreeNodeFlags_AllowOverlap', 'ImGuiSelectableFlags_AllowItemOverlap' to 'ImGuiSelectableFlags_AllowOverlap'. Kept redirecting enums (will obsolete).
 - 2023/06/28 (1.89.7) - overlapping items: IsItemHovered() now by default return false when querying an item using AllowOverlap mode which is being overlapped. Use ImGuiHoveredFlags_AllowWhenOverlappedByItem to revert to old behavior.
 - 2023/06/28 (1.89.7) - overlapping items: Selectable and TreeNode don't allow overlap when active so overlapping widgets won't appear as hovered. While this fixes a common small visual issue, it also means that calling IsItemHovered() after a non-reactive elements - e.g. Text() - overlapping an active one may fail if you don't use IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem). (#6610)
 - 2023/06/20 (1.89.7) - moved io.HoverDelayShort/io.HoverDelayNormal to style.HoverDelayShort/style.HoverDelayNormal. As the fields were added in 1.89 and expected to be left unchanged by most users, or only tweaked once during app initialization, we are exceptionally accepting the breakage.
 - 2023/05/30 (1.89.6) - backends: renamed "imgui_impl_sdlrenderer.cpp" to "imgui_impl_sdlrenderer2.cpp" and "imgui_impl_sdlrenderer.h" to "imgui_impl_sdlrenderer2.h". This is in prevision for the future release of SDL3.
 - 2023/05/22 (1.89.6) - listbox: commented out obsolete/redirecting functions that were marked obsolete more than two years ago:
                           - ListBoxHeader()  -> use BeginListBox() (note how two variants of ListBoxHeader() existed. Check commented versions in imgui.h for reference)
                           - ListBoxFooter()  -> use EndListBox()
 - 2023/05/15 (1.89.6) - clipper: commented out obsolete redirection constructor 'ImGuiListClipper(int items_count, float items_height = -1.0f)' that was marked obsolete in 1.79. Use default constructor + clipper.Begin().
 - 2023/05/15 (1.89.6) - clipper: renamed ImGuiListClipper::ForceDisplayRangeByIndices() to ImGuiListClipper::IncludeRangeByIndices().
 - 2023/03/14 (1.89.4) - commented out redirecting enums/functions names that were marked obsolete two years ago:
                           - ImGuiSliderFlags_ClampOnInput        -> use ImGuiSliderFlags_AlwaysClamp
                           - ImGuiInputTextFlags_AlwaysInsertMode -> use ImGuiInputTextFlags_AlwaysOverwrite
                           - ImDrawList::AddBezierCurve()         -> use ImDrawList::AddBezierCubic()
                           - ImDrawList::PathBezierCurveTo()      -> use ImDrawList::PathBezierCubicCurveTo()
 - 2023/03/09 (1.89.4) - renamed PushAllowKeyboardFocus()/PopAllowKeyboardFocus() to PushTabStop()/PopTabStop(). Kept inline redirection functions (will obsolete).
 - 2023/03/09 (1.89.4) - tooltips: Added 'bool' return value to BeginTooltip() for API consistency. Please only submit contents and call EndTooltip() if BeginTooltip() returns true. In reality the function will _currently_ always return true, but further changes down the line may change this, best to clarify API sooner.
 - 2023/02/15 (1.89.4) - moved the optional "courtesy maths operators" implementation from imgui_internal.h in imgui.h.
                         Even though we encourage using your own maths types and operators by setting up IM_VEC2_CLASS_EXTRA,
                         it has been frequently requested by people to use our own. We had an opt-in define which was
                         previously fulfilled in imgui_internal.h. It is now fulfilled in imgui.h. (#6164)
                           - OK:     #define IMGUI_DEFINE_MATH_OPERATORS / #include "imgui.h" / #include "imgui_internal.h"
                           - Error:  #include "imgui.h" / #define IMGUI_DEFINE_MATH_OPERATORS / #include "imgui_internal.h"
 - 2023/02/07 (1.89.3) - backends: renamed "imgui_impl_sdl.cpp" to "imgui_impl_sdl2.cpp" and "imgui_impl_sdl.h" to "imgui_impl_sdl2.h". (#6146) This is in prevision for the future release of SDL3.
 - 2022/10/26 (1.89)   - commented out redirecting OpenPopupContextItem() which was briefly the name of OpenPopupOnItemClick() from 1.77 to 1.79.
 - 2022/10/12 (1.89)   - removed runtime patching of invalid "%f"/"%0.f" format strings for DragInt()/SliderInt(). This was obsoleted in 1.61 (May 2018). See 1.61 changelog for details.
 - 2022/09/26 (1.89)   - renamed and merged keyboard modifiers key enums and flags into a same set. Kept inline redirection enums (will obsolete).
                           - ImGuiKey_ModCtrl  and ImGuiModFlags_Ctrl  -> ImGuiMod_Ctrl
                           - ImGuiKey_ModShift and ImGuiModFlags_Shift -> ImGuiMod_Shift
                           - ImGuiKey_ModAlt   and ImGuiModFlags_Alt   -> ImGuiMod_Alt
                           - ImGuiKey_ModSuper and ImGuiModFlags_Super -> ImGuiMod_Super
                         the ImGuiKey_ModXXX were introduced in 1.87 and mostly used by backends.
                         the ImGuiModFlags_XXX have been exposed in imgui.h but not really used by any public api only by third-party extensions.
                         exceptionally commenting out the older ImGuiKeyModFlags_XXX names ahead of obsolescence schedule to reduce confusion and because they were not meant to be used anyway.
 - 2022/09/20 (1.89)   - ImGuiKey is now a typed enum, allowing ImGuiKey_XXX symbols to be named in debuggers.
                         this will require uses of legacy backend-dependent indices to be casted, e.g.
                            - with imgui_impl_glfw:  IsKeyPressed(GLFW_KEY_A) -> IsKeyPressed((ImGuiKey)GLFW_KEY_A);
                            - with imgui_impl_win32: IsKeyPressed('A')        -> IsKeyPressed((ImGuiKey)'A')
                            - etc. However if you are upgrading code you might well use the better, backend-agnostic IsKeyPressed(ImGuiKey_A) now!
 - 2022/09/12 (1.89) - removed the bizarre legacy default argument for 'TreePush(const void* ptr = NULL)', always pass a pointer value explicitly. NULL/nullptr is ok but require cast, e.g. TreePush((void*)nullptr);
 - 2022/09/05 (1.89) - commented out redirecting functions/enums names that were marked obsolete in 1.77 and 1.78 (June 2020):
                         - DragScalar(), DragScalarN(), DragFloat(), DragFloat2(), DragFloat3(), DragFloat4(): For old signatures ending with (..., const char* format, float power = 1.0f) -> use (..., format ImGuiSliderFlags_Logarithmic) if power != 1.0f.
                         - SliderScalar(), SliderScalarN(), SliderFloat(), SliderFloat2(), SliderFloat3(), SliderFloat4(): For old signatures ending with (..., const char* format, float power = 1.0f) -> use (..., format ImGuiSliderFlags_Logarithmic) if power != 1.0f.
                         - BeginPopupContextWindow(const char*, ImGuiMouseButton, bool) -> use BeginPopupContextWindow(const char*, ImGuiPopupFlags)
 - 2022/09/02 (1.89) - obsoleted using SetCursorPos()/SetCursorScreenPos() to extend parent window/cell boundaries.
                       this relates to when moving the cursor position beyond current boundaries WITHOUT submitting an item.
                         - previously this would make the window content size ~200x200:
                              Begin(...) + SetCursorScreenPos(GetCursorScreenPos() + ImVec2(200,200)) + End();
                         - instead, please submit an item:
                              Begin(...) + SetCursorScreenPos(GetCursorScreenPos() + ImVec2(200,200)) + Dummy(ImVec2(0,0)) + End();
                         - alternative:
                              Begin(...) + Dummy(ImVec2(200,200)) + End();
                         - content size is now only extended when submitting an item!
                         - with '#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS' this will now be detected and assert.
                         - without '#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS' this will silently be fixed until we obsolete it.
 - 2022/08/03 (1.89) - changed signature of ImageButton() function. Kept redirection function (will obsolete).
                        - added 'const char* str_id' parameter + removed 'int frame_padding = -1' parameter.
                        - old signature: bool ImageButton(ImTextureID tex_id, ImVec2 size, ImVec2 uv0 = ImVec2(0,0), ImVec2 uv1 = ImVec2(1,1), int frame_padding = -1, ImVec4 bg_col = ImVec4(0,0,0,0), ImVec4 tint_col = ImVec4(1,1,1,1));
                          - used the ImTextureID value to create an ID. This was inconsistent with other functions, led to ID conflicts, and caused problems with engines using transient ImTextureID values.
                          - had a FramePadding override which was inconsistent with other functions and made the already-long signature even longer.
                        - new signature: bool ImageButton(const char* str_id, ImTextureID tex_id, ImVec2 size, ImVec2 uv0 = ImVec2(0,0), ImVec2 uv1 = ImVec2(1,1), ImVec4 bg_col = ImVec4(0,0,0,0), ImVec4 tint_col = ImVec4(1,1,1,1));
                          - requires an explicit identifier. You may still use e.g. PushID() calls and then pass an empty identifier.
                          - always uses style.FramePadding for padding, to be consistent with other buttons. You may use PushStyleVar() to alter this.
 - 2022/07/08 (1.89) - inputs: removed io.NavInputs[] and ImGuiNavInput enum (following 1.87 changes).
                        - Official backends from 1.87+                  -> no issue.
                        - Official backends from 1.60 to 1.86           -> will build and convert gamepad inputs, unless IMGUI_DISABLE_OBSOLETE_KEYIO is defined. Need updating!
                        - Custom backends not writing to io.NavInputs[] -> no issue.
                        - Custom backends writing to io.NavInputs[]     -> will build and convert gamepad inputs, unless IMGUI_DISABLE_OBSOLETE_KEYIO is defined. Need fixing!
                        - TL;DR: Backends should call io.AddKeyEvent()/io.AddKeyAnalogEvent() with ImGuiKey_GamepadXXX values instead of filling io.NavInput[].
 - 2022/06/15 (1.88) - renamed IMGUI_DISABLE_METRICS_WINDOW to IMGUI_DISABLE_DEBUG_TOOLS for correctness. kept support for old define (will obsolete).
 - 2022/05/03 (1.88) - backends: osx: removed ImGui_ImplOSX_HandleEvent() from backend API in favor of backend automatically handling event capture. All ImGui_ImplOSX_HandleEvent() calls should be removed as they are now unnecessary.
 - 2022/04/05 (1.88) - inputs: renamed ImGuiKeyModFlags to ImGuiModFlags. Kept inline redirection enums (will obsolete). This was never used in public API functions but technically present in imgui.h and ImGuiIO.
 - 2022/01/20 (1.87) - inputs: reworded gamepad IO.
                        - Backend writing to io.NavInputs[]            -> backend should call io.AddKeyEvent()/io.AddKeyAnalogEvent() with ImGuiKey_GamepadXXX values.
 - 2022/01/19 (1.87) - sliders, drags: removed support for legacy arithmetic operators (+,+-,*,/) when inputing text. This doesn't break any api/code but a feature that used to be accessible by end-users (which seemingly no one used).
 - 2022/01/17 (1.87) - inputs: reworked mouse IO.
                        - Backend writing to io.MousePos               -> backend should call io.AddMousePosEvent()
                        - Backend writing to io.MouseDown[]            -> backend should call io.AddMouseButtonEvent()
                        - Backend writing to io.MouseWheel             -> backend should call io.AddMouseWheelEvent()
                        - Backend writing to io.MouseHoveredViewport   -> backend should call io.AddMouseViewportEvent() [Docking branch w/ multi-viewports only]
                       note: for all calls to IO new functions, the Dear ImGui context should be bound/current.
                       read https://github.com/ocornut/imgui/issues/4921 for details.
 - 2022/01/10 (1.87) - inputs: reworked keyboard IO. Removed io.KeyMap[], io.KeysDown[] in favor of calling io.AddKeyEvent(). Removed GetKeyIndex(), now unnecessary. All IsKeyXXX() functions now take ImGuiKey values. All features are still functional until IMGUI_DISABLE_OBSOLETE_KEYIO is defined. Read Changelog and Release Notes for details.
                        - IsKeyPressed(MY_NATIVE_KEY_XXX)              -> use IsKeyPressed(ImGuiKey_XXX)
                        - IsKeyPressed(GetKeyIndex(ImGuiKey_XXX))      -> use IsKeyPressed(ImGuiKey_XXX)
                        - Backend writing to io.KeyMap[],io.KeysDown[] -> backend should call io.AddKeyEvent() (+ call io.SetKeyEventNativeData() if you want legacy user code to stil function with legacy key codes).
                        - Backend writing to io.KeyCtrl, io.KeyShift.. -> backend should call io.AddKeyEvent() with ImGuiMod_XXX values. *IF YOU PULLED CODE BETWEEN 2021/01/10 and 2021/01/27: We used to have a io.AddKeyModsEvent() function which was now replaced by io.AddKeyEvent() with ImGuiMod_XXX values.*
                     - one case won't work with backward compatibility: if your custom backend used ImGuiKey as mock native indices (e.g. "io.KeyMap[ImGuiKey_A] = ImGuiKey_A") because those values are now larger than the legacy KeyDown[] array. Will assert.
                     - inputs: added ImGuiKey_ModCtrl/ImGuiKey_ModShift/ImGuiKey_ModAlt/ImGuiKey_ModSuper values to submit keyboard modifiers using io.AddKeyEvent(), instead of writing directly to io.KeyCtrl, io.KeyShift, io.KeyAlt, io.KeySuper.
 - 2022/01/05 (1.87) - inputs: renamed ImGuiKey_KeyPadEnter to ImGuiKey_KeypadEnter to align with new symbols. Kept redirection enum.
 - 2022/01/05 (1.87) - removed io.ImeSetInputScreenPosFn() in favor of more flexible io.SetPlatformImeDataFn(). Removed 'void* io.ImeWindowHandle' in favor of writing to 'void* ImGuiViewport::PlatformHandleRaw'.
 - 2022/01/01 (1.87) - commented out redirecting functions/enums names that were marked obsolete in 1.69, 1.70, 1.71, 1.72 (March-July 2019)
                        - ImGui::SetNextTreeNodeOpen()        -> use ImGui::SetNextItemOpen()
                        - ImGui::GetContentRegionAvailWidth() -> use ImGui::GetContentRegionAvail().x
                        - ImGui::TreeAdvanceToLabelPos()      -> use ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing());
                        - ImFontAtlas::CustomRect             -> use ImFontAtlasCustomRect
                        - ImGuiColorEditFlags_RGB/HSV/HEX     -> use ImGuiColorEditFlags_DisplayRGB/HSV/Hex
 - 2021/12/20 (1.86) - backends: removed obsolete Marmalade backend (imgui_impl_marmalade.cpp) + example. Find last supported version at https://github.com/ocornut/imgui/wiki/Bindings
 - 2021/11/04 (1.86) - removed CalcListClipping() function. Prefer using ImGuiListClipper which can return non-contiguous ranges. Please open an issue if you think you really need this function.
 - 2021/08/23 (1.85) - removed GetWindowContentRegionWidth() function. keep inline redirection helper. can use 'GetWindowContentRegionMax().x - GetWindowContentRegionMin().x' instead for generally 'GetContentRegionAvail().x' is more useful.
 - 2021/07/26 (1.84) - commented out redirecting functions/enums names that were marked obsolete in 1.67 and 1.69 (March 2019):
                        - ImGui::GetOverlayDrawList() -> use ImGui::GetForegroundDrawList()
                        - ImFont::GlyphRangesBuilder  -> use ImFontGlyphRangesBuilder
 - 2021/05/19 (1.83) - backends: obsoleted direct access to ImDrawCmd::TextureId in favor of calling ImDrawCmd::GetTexID().
                        - if you are using official backends from the source tree: you have nothing to do.
                        - if you have copied old backend code or using your own: change access to draw_cmd->TextureId to draw_cmd->GetTexID().
 - 2021/03/12 (1.82) - upgraded ImDrawList::AddRect(), AddRectFilled(), PathRect() to use ImDrawFlags instead of ImDrawCornersFlags.
                        - ImDrawCornerFlags_TopLeft  -> use ImDrawFlags_RoundCornersTopLeft
                        - ImDrawCornerFlags_BotRight -> use ImDrawFlags_RoundCornersBottomRight
                        - ImDrawCornerFlags_None     -> use ImDrawFlags_RoundCornersNone etc.
                       flags now sanely defaults to 0 instead of 0x0F, consistent with all other flags in the API.
                       breaking: the default with rounding > 0.0f is now "round all corners" vs old implicit "round no corners":
                        - rounding == 0.0f + flags == 0 --> meant no rounding  --> unchanged (common use)
                        - rounding  > 0.0f + flags != 0 --> meant rounding     --> unchanged (common use)
                        - rounding == 0.0f + flags != 0 --> meant no rounding  --> unchanged (unlikely use)
                        - rounding  > 0.0f + flags == 0 --> meant no rounding  --> BREAKING (unlikely use): will now round all corners --> use ImDrawFlags_RoundCornersNone or rounding == 0.0f.
                       this ONLY matters for hard coded use of 0 + rounding > 0.0f. Use of named ImDrawFlags_RoundCornersNone (new) or ImDrawCornerFlags_None (old) are ok.
                       the old ImDrawCornersFlags used awkward default values of ~0 or 0xF (4 lower bits set) to signify "round all corners" and we sometimes encouraged using them as shortcuts.
                       legacy path still support use of hard coded ~0 or any value from 0x1 or 0xF. They will behave the same with legacy paths enabled (will assert otherwise).
 - 2021/03/11 (1.82) - removed redirecting functions/enums names that were marked obsolete in 1.66 (September 2018):
                        - ImGui::SetScrollHere()              -> use ImGui::SetScrollHereY()
 - 2021/03/11 (1.82) - clarified that ImDrawList::PathArcTo(), ImDrawList::PathArcToFast() won't render with radius < 0.0f. Previously it sorts of accidentally worked but would generally lead to counter-clockwise paths and have an effect on anti-aliasing.
 - 2021/03/10 (1.82) - upgraded ImDrawList::AddPolyline() and PathStroke() "bool closed" parameter to "ImDrawFlags flags". The matching ImDrawFlags_Closed value is guaranteed to always stay == 1 in the future.
 - 2021/02/22 (1.82) - (*undone in 1.84*) win32+mingw: Re-enabled IME functions by default even under MinGW. In July 2016, issue #738 had me incorrectly disable those default functions for MinGW. MinGW users should: either link with -limm32, either set their imconfig file  with '#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS'.
 - 2021/02/17 (1.82) - renamed rarely used style.CircleSegmentMaxError (old default = 1.60f) to style.CircleTessellationMaxError (new default = 0.30f) as the meaning of the value changed.
 - 2021/02/03 (1.81) - renamed ListBoxHeader(const char* label, ImVec2 size) to BeginListBox(). Kept inline redirection function (will obsolete).
                     - removed ListBoxHeader(const char* label, int items_count, int height_in_items = -1) in favor of specifying size. Kept inline redirection function (will obsolete).
                     - renamed ListBoxFooter() to EndListBox(). Kept inline redirection function (will obsolete).
 - 2021/01/26 (1.81) - removed ImGuiFreeType::BuildFontAtlas(). Kept inline redirection function. Prefer using '#define IMGUI_ENABLE_FREETYPE', but there's a runtime selection path available too. The shared extra flags parameters (very rarely used) are now stored in ImFontAtlas::FontBuilderFlags.
                     - renamed ImFontConfig::RasterizerFlags (used by FreeType) to ImFontConfig::FontBuilderFlags.
                     - renamed ImGuiFreeType::XXX flags to ImGuiFreeTypeBuilderFlags_XXX for consistency with other API.
 - 2020/10/12 (1.80) - removed redirecting functions/enums that were marked obsolete in 1.63 (August 2018):
                        - ImGui::IsItemDeactivatedAfterChange() -> use ImGui::IsItemDeactivatedAfterEdit().
                        - ImGuiCol_ModalWindowDarkening       -> use ImGuiCol_ModalWindowDimBg
                        - ImGuiInputTextCallback              -> use ImGuiTextEditCallback
                        - ImGuiInputTextCallbackData          -> use ImGuiTextEditCallbackData
 - 2020/12/21 (1.80) - renamed ImDrawList::AddBezierCurve() to AddBezierCubic(), and PathBezierCurveTo() to PathBezierCubicCurveTo(). Kept inline redirection function (will obsolete).
 - 2020/12/04 (1.80) - added imgui_tables.cpp file! Manually constructed project files will need the new file added!
 - 2020/11/18 (1.80) - renamed undocumented/internals ImGuiColumnsFlags_* to ImGuiOldColumnFlags_* in prevision of incoming Tables API.
 - 2020/11/03 (1.80) - renamed io.ConfigWindowsMemoryCompactTimer to io.ConfigMemoryCompactTimer as the feature will apply to other data structures
 - 2020/10/14 (1.80) - backends: moved all backends files (imgui_impl_XXXX.cpp, imgui_impl_XXXX.h) from examples/ to backends/.
 - 2020/10/12 (1.80) - removed redirecting functions/enums that were marked obsolete in 1.60 (April 2018):
                        - io.RenderDrawListsFn pointer        -> use ImGui::GetDrawData() value and call the render function of your backend
                        - ImGui::IsAnyWindowFocused()         -> use ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)
                        - ImGui::IsAnyWindowHovered()         -> use ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)
                        - ImGuiStyleVar_Count_                -> use ImGuiStyleVar_COUNT
                        - ImGuiMouseCursor_Count_             -> use ImGuiMouseCursor_COUNT
                      - removed redirecting functions names that were marked obsolete in 1.61 (May 2018):
                        - InputFloat (... int decimal_precision ...) -> use InputFloat (... const char* format ...) with format = "%.Xf" where X is your value for decimal_precision.
                        - same for InputFloat2()/InputFloat3()/InputFloat4() variants taking a `int decimal_precision` parameter.
 - 2020/10/05 (1.79) - removed ImGuiListClipper: Renamed constructor parameters which created an ambiguous alternative to using the ImGuiListClipper::Begin() function, with misleading edge cases (note: imgui_memory_editor <0.40 from imgui_club/ used this old clipper API. Update your copy if needed).
 - 2020/09/25 (1.79) - renamed ImGuiSliderFlags_ClampOnInput to ImGuiSliderFlags_AlwaysClamp. Kept redirection enum (will obsolete sooner because previous name was added recently).
 - 2020/09/25 (1.79) - renamed style.TabMinWidthForUnselectedCloseButton to style.TabMinWidthForCloseButton.
 - 2020/09/21 (1.79) - renamed OpenPopupContextItem() back to OpenPopupOnItemClick(), reverting the change from 1.77. For varieties of reason this is more self-explanatory.
 - 2020/09/21 (1.79) - removed return value from OpenPopupOnItemClick() - returned true on mouse release on an item - because it is inconsistent with other popup APIs and makes others misleading. It's also and unnecessary: you can use IsWindowAppearing() after BeginPopup() for a similar result.
 - 2020/09/17 (1.79) - removed ImFont::DisplayOffset in favor of ImFontConfig::GlyphOffset. DisplayOffset was applied after scaling and not very meaningful/useful outside of being needed by the default ProggyClean font. If you scaled this value after calling AddFontDefault(), this is now done automatically. It was also getting in the way of better font scaling, so let's get rid of it now!
 - 2020/08/17 (1.78) - obsoleted use of the trailing 'float power=1.0f' parameter for DragFloat(), DragFloat2(), DragFloat3(), DragFloat4(), DragFloatRange2(), DragScalar(), DragScalarN(), SliderFloat(), SliderFloat2(), SliderFloat3(), SliderFloat4(), SliderScalar(), SliderScalarN(), VSliderFloat() and VSliderScalar().
                       replaced the 'float power=1.0f' argument with integer-based flags defaulting to 0 (as with all our flags).
                       worked out a backward-compatibility scheme so hopefully most C++ codebase should not be affected. in short, when calling those functions:
                       - if you omitted the 'power' parameter (likely!), you are not affected.
                       - if you set the 'power' parameter to 1.0f (same as previous default value): 1/ your compiler may warn on float>int conversion, 2/ everything else will work. 3/ you can replace the 1.0f value with 0 to fix the warning, and be technically correct.
                       - if you set the 'power' parameter to >1.0f (to enable non-linear editing): 1/ your compiler may warn on float>int conversion, 2/ code will assert at runtime, 3/ in case asserts are disabled, the code will not crash and enable the _Logarithmic flag. 4/ you can replace the >1.0f value with ImGuiSliderFlags_Logarithmic to fix the warning/assert and get a _similar_ effect as previous uses of power >1.0f.
                       see https://github.com/ocornut/imgui/issues/3361 for all details.
                       kept inline redirection functions (will obsolete) apart for: DragFloatRange2(), VSliderFloat(), VSliderScalar(). For those three the 'float power=1.0f' version was removed directly as they were most unlikely ever used.
                       for shared code, you can version check at compile-time with `#if IMGUI_VERSION_NUM >= 17704`.
                     - obsoleted use of v_min > v_max in DragInt, DragFloat, DragScalar to lock edits (introduced in 1.73, was not demoed nor documented very), will be replaced by a more generic ReadOnly feature. You may use the ImGuiSliderFlags_ReadOnly internal flag in the meantime.
 - 2020/06/23 (1.77) - removed BeginPopupContextWindow(const char*, int mouse_button, bool also_over_items) in favor of BeginPopupContextWindow(const char*, ImGuiPopupFlags flags) with ImGuiPopupFlags_NoOverItems.
 - 2020/06/15 (1.77) - renamed OpenPopupOnItemClick() to OpenPopupContextItem(). Kept inline redirection function (will obsolete). [NOTE: THIS WAS REVERTED IN 1.79]
 - 2020/06/15 (1.77) - removed CalcItemRectClosestPoint() entry point which was made obsolete and asserting in December 2017.
 - 2020/04/23 (1.77) - removed unnecessary ID (first arg) of ImFontAtlas::AddCustomRectRegular().
 - 2020/01/22 (1.75) - ImDrawList::AddCircle()/AddCircleFilled() functions don't accept negative radius any more.
 - 2019/12/17 (1.75) - [undid this change in 1.76] made Columns() limited to 64 columns by asserting above that limit. While the current code technically supports it, future code may not so we're putting the restriction ahead.
 - 2019/12/13 (1.75) - [imgui_internal.h] changed ImRect() default constructor initializes all fields to 0.0f instead of (FLT_MAX,FLT_MAX,-FLT_MAX,-FLT_MAX). If you used ImRect::Add() to create bounding boxes by adding multiple points into it, you may need to fix your initial value.
 - 2019/12/08 (1.75) - removed redirecting functions/enums that were marked obsolete in 1.53 (December 2017):
                       - ShowTestWindow()                    -> use ShowDemoWindow()
                       - IsRootWindowFocused()               -> use IsWindowFocused(ImGuiFocusedFlags_RootWindow)
                       - IsRootWindowOrAnyChildFocused()     -> use IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
                       - SetNextWindowContentWidth(w)        -> use SetNextWindowContentSize(ImVec2(w, 0.0f)
                       - GetItemsLineHeightWithSpacing()     -> use GetFrameHeightWithSpacing()
                       - ImGuiCol_ChildWindowBg              -> use ImGuiCol_ChildBg
                       - ImGuiStyleVar_ChildWindowRounding   -> use ImGuiStyleVar_ChildRounding
                       - ImGuiTreeNodeFlags_AllowOverlapMode -> use ImGuiTreeNodeFlags_AllowItemOverlap
                       - IMGUI_DISABLE_TEST_WINDOWS          -> use IMGUI_DISABLE_DEMO_WINDOWS
 - 2019/12/08 (1.75) - obsoleted calling ImDrawList::PrimReserve() with a negative count (which was vaguely documented and rarely if ever used). Instead, we added an explicit PrimUnreserve() API.
 - 2019/12/06 (1.75) - removed implicit default parameter to IsMouseDragging(int button = 0) to be consistent with other mouse functions (none of the other functions have it).
 - 2019/11/21 (1.74) - ImFontAtlas::AddCustomRectRegular() now requires an ID larger than 0x110000 (instead of 0x10000) to conform with supporting Unicode planes 1-16 in a future update. ID below 0x110000 will now assert.
 - 2019/11/19 (1.74) - renamed IMGUI_DISABLE_FORMAT_STRING_FUNCTIONS to IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS for consistency.
 - 2019/11/19 (1.74) - renamed IMGUI_DISABLE_MATH_FUNCTIONS to IMGUI_DISABLE_DEFAULT_MATH_FUNCTIONS for consistency.
 - 2019/10/22 (1.74) - removed redirecting functions/enums that were marked obsolete in 1.52 (October 2017):
                       - Begin() [old 5 args version]        -> use Begin() [3 args], use SetNextWindowSize() SetNextWindowBgAlpha() if needed
                       - IsRootWindowOrAnyChildHovered()     -> use IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)
                       - AlignFirstTextHeightToWidgets()     -> use AlignTextToFramePadding()
                       - SetNextWindowPosCenter()            -> use SetNextWindowPos() with a pivot of (0.5f, 0.5f)
                       - ImFont::Glyph                       -> use ImFontGlyph
 - 2019/10/14 (1.74) - inputs: Fixed a miscalculation in the keyboard/mouse "typematic" repeat delay/rate calculation, used by keys and e.g. repeating mouse buttons as well as the GetKeyPressedAmount() function.
                       if you were using a non-default value for io.KeyRepeatRate (previous default was 0.250), you can add +io.KeyRepeatDelay to it to compensate for the fix.
                       The function was triggering on: 0.0 and (delay+rate*N) where (N>=1). Fixed formula responds to (N>=0). Effectively it made io.KeyRepeatRate behave like it was set to (io.KeyRepeatRate + io.KeyRepeatDelay).
                       If you never altered io.KeyRepeatRate nor used GetKeyPressedAmount() this won't affect you.
 - 2019/07/15 (1.72) - removed TreeAdvanceToLabelPos() which is rarely used and only does SetCursorPosX(GetCursorPosX() + GetTreeNodeToLabelSpacing()). Kept redirection function (will obsolete).
 - 2019/07/12 (1.72) - renamed ImFontAtlas::CustomRect to ImFontAtlasCustomRect. Kept redirection typedef (will obsolete).
 - 2019/06/14 (1.72) - removed redirecting functions/enums names that were marked obsolete in 1.51 (June 2017): ImGuiCol_Column*, ImGuiSetCond_*, IsItemHoveredRect(), IsPosHoveringAnyWindow(), IsMouseHoveringAnyWindow(), IsMouseHoveringWindow(), IMGUI_ONCE_UPON_A_FRAME. Grep this log for details and new names, or see how they were implemented until 1.71.
 - 2019/06/07 (1.71) - rendering of child window outer decorations (bg color, border, scrollbars) is now performed as part of the parent window. If you have
                       overlapping child windows in a same parent, and relied on their relative z-order to be mapped to their submission order, this will affect your rendering.
                       This optimization is disabled if the parent window has no visual output, because it appears to be the most common situation leading to the creation of overlapping child windows.
                       Please reach out if you are affected.
 - 2019/05/13 (1.71) - renamed SetNextTreeNodeOpen() to SetNextItemOpen(). Kept inline redirection function (will obsolete).
 - 2019/05/11 (1.71) - changed io.AddInputCharacter(unsigned short c) signature to io.AddInputCharacter(unsigned int c).
 - 2019/04/29 (1.70) - improved ImDrawList thick strokes (>1.0f) preserving correct thickness up to 90 degrees angles (e.g. rectangles). If you have custom rendering using thick lines, they will appear thicker now.
 - 2019/04/29 (1.70) - removed GetContentRegionAvailWidth(), use GetContentRegionAvail().x instead. Kept inline redirection function (will obsolete).
 - 2019/03/04 (1.69) - renamed GetOverlayDrawList() to GetForegroundDrawList(). Kept redirection function (will obsolete).
 - 2019/02/26 (1.69) - renamed ImGuiColorEditFlags_RGB/ImGuiColorEditFlags_HSV/ImGuiColorEditFlags_HEX to ImGuiColorEditFlags_DisplayRGB/ImGuiColorEditFlags_DisplayHSV/ImGuiColorEditFlags_DisplayHex. Kept redirection enums (will obsolete).
 - 2019/02/14 (1.68) - made it illegal/assert when io.DisplayTime == 0.0f (with an exception for the first frame). If for some reason your time step calculation gives you a zero value, replace it with an arbitrarily small value!
 - 2019/02/01 (1.68) - removed io.DisplayVisibleMin/DisplayVisibleMax (which were marked obsolete and removed from viewport/docking branch already).
 - 2019/01/06 (1.67) - renamed io.InputCharacters[], marked internal as was always intended. Please don't access directly, and use AddInputCharacter() instead!
 - 2019/01/06 (1.67) - renamed ImFontAtlas::GlyphRangesBuilder to ImFontGlyphRangesBuilder. Kept redirection typedef (will obsolete).
 - 2018/12/20 (1.67) - made it illegal to call Begin("") with an empty string. This somehow half-worked before but had various undesirable side-effects.
 - 2018/12/10 (1.67) - renamed io.ConfigResizeWindowsFromEdges to io.ConfigWindowsResizeFromEdges as we are doing a large pass on configuration flags.
 - 2018/10/12 (1.66) - renamed misc/stl/imgui_stl.* to misc/cpp/imgui_stdlib.* in prevision for other C++ helper files.
 - 2018/09/28 (1.66) - renamed SetScrollHere() to SetScrollHereY(). Kept redirection function (will obsolete).
 - 2018/09/06 (1.65) - renamed stb_truetype.h to imstb_truetype.h, stb_textedit.h to imstb_textedit.h, and stb_rect_pack.h to imstb_rectpack.h.
                       If you were conveniently using the imgui copy of those STB headers in your project you will have to update your include paths.
 - 2018/09/05 (1.65) - renamed io.OptCursorBlink/io.ConfigCursorBlink to io.ConfigInputTextCursorBlink. (#1427)
 - 2018/08/31 (1.64) - added imgui_widgets.cpp file, extracted and moved widgets code out of imgui.cpp into imgui_widgets.cpp. Re-ordered some of the code remaining in imgui.cpp.
                       NONE OF THE FUNCTIONS HAVE CHANGED. THE CODE IS SEMANTICALLY 100% IDENTICAL, BUT _EVERY_ FUNCTION HAS BEEN MOVED.
                       Because of this, any local modifications to imgui.cpp will likely conflict when you update. Read docs/CHANGELOG.txt for suggestions.
 - 2018/08/22 (1.63) - renamed IsItemDeactivatedAfterChange() to IsItemDeactivatedAfterEdit() for consistency with new IsItemEdited() API. Kept redirection function (will obsolete soonish as IsItemDeactivatedAfterChange() is very recent).
 - 2018/08/21 (1.63) - renamed ImGuiTextEditCallback to ImGuiInputTextCallback, ImGuiTextEditCallbackData to ImGuiInputTextCallbackData for consistency. Kept redirection types (will obsolete).
 - 2018/08/21 (1.63) - removed ImGuiInputTextCallbackData::ReadOnly since it is a duplication of (ImGuiInputTextCallbackData::Flags & ImGuiInputTextFlags_ReadOnly).
 - 2018/08/01 (1.63) - removed per-window ImGuiWindowFlags_ResizeFromAnySide beta flag in favor of a global io.ConfigResizeWindowsFromEdges [update 1.67 renamed to ConfigWindowsResizeFromEdges] to enable the feature.
 - 2018/08/01 (1.63) - renamed io.OptCursorBlink to io.ConfigCursorBlink [-> io.ConfigInputTextCursorBlink in 1.65], io.OptMacOSXBehaviors to ConfigMacOSXBehaviors for consistency.
 - 2018/07/22 (1.63) - changed ImGui::GetTime() return value from float to double to avoid accumulating floating point imprecisions over time.
 - 2018/07/08 (1.63) - style: renamed ImGuiCol_ModalWindowDarkening to ImGuiCol_ModalWindowDimBg for consistency with other features. Kept redirection enum (will obsolete).
 - 2018/06/08 (1.62) - examples: the imgui_impl_XXX files have been split to separate platform (Win32, GLFW, SDL2, etc.) from renderer (DX11, OpenGL, Vulkan,  etc.).
                       old backends will still work as is, however prefer using the separated backends as they will be updated to support multi-viewports.
                       when adopting new backends follow the main.cpp code of your preferred examples/ folder to know which functions to call.
                       in particular, note that old backends called ImGui::NewFrame() at the end of their ImGui_ImplXXXX_NewFrame() function.
 - 2018/06/06 (1.62) - renamed GetGlyphRangesChinese() to GetGlyphRangesChineseFull() to distinguish other variants and discourage using the full set.
 - 2018/06/06 (1.62) - TreeNodeEx()/TreeNodeBehavior(): the ImGuiTreeNodeFlags_CollapsingHeader helper now include the ImGuiTreeNodeFlags_NoTreePushOnOpen flag. See Changelog for details.
 - 2018/05/03 (1.61) - DragInt(): the default compile-time format string has been changed from "%.0f" to "%d", as we are not using integers internally any more.
                       If you used DragInt() with custom format strings, make sure you change them to use %d or an integer-compatible format.
                       To honor backward-compatibility, the DragInt() code will currently parse and modify format strings to replace %*f with %d, giving time to users to upgrade their code.
                       If you have IMGUI_DISABLE_OBSOLETE_FUNCTIONS enabled, the code will instead assert! You may run a reg-exp search on your codebase for e.g. "DragInt.*%f" to help you find them.
 - 2018/04/28 (1.61) - obsoleted InputFloat() functions taking an optional "int decimal_precision" in favor of an equivalent and more flexible "const char* format",
                       consistent with other functions. Kept redirection functions (will obsolete).
 - 2018/04/09 (1.61) - IM_DELETE() helper function added in 1.60 doesn't clear the input _pointer_ reference, more consistent with expectation and allows passing r-value.
 - 2018/03/20 (1.60) - renamed io.WantMoveMouse to io.WantSetMousePos for consistency and ease of understanding (was added in 1.52, _not_ used by core and only honored by some backend ahead of merging the Nav branch).
 - 2018/03/12 (1.60) - removed ImGuiCol_CloseButton, ImGuiCol_CloseButtonActive, ImGuiCol_CloseButtonHovered as the closing cross uses regular button colors now.
 - 2018/03/08 (1.60) - changed ImFont::DisplayOffset.y to default to 0 instead of +1. Fixed rounding of Ascent/Descent to match TrueType renderer. If you were adding or subtracting to ImFont::DisplayOffset check if your fonts are correctly aligned vertically.
 - 2018/03/03 (1.60) - renamed ImGuiStyleVar_Count_ to ImGuiStyleVar_COUNT and ImGuiMouseCursor_Count_ to ImGuiMouseCursor_COUNT for consistency with other public enums.
 - 2018/02/18 (1.60) - BeginDragDropSource(): temporarily removed the optional mouse_button=0 parameter because it is not really usable in many situations at the moment.
 - 2018/02/16 (1.60) - obsoleted the io.RenderDrawListsFn callback, you can call your graphics engine render function after ImGui::Render(). Use ImGui::GetDrawData() to retrieve the ImDrawData* to display.
 - 2018/02/07 (1.60) - reorganized context handling to be more explicit,
                       - YOU NOW NEED TO CALL ImGui::CreateContext() AT THE BEGINNING OF YOUR APP, AND CALL ImGui::DestroyContext() AT THE END.
                       - removed Shutdown() function, as DestroyContext() serve this purpose.
                       - you may pass a ImFontAtlas* pointer to CreateContext() to share a font atlas between contexts. Otherwise CreateContext() will create its own font atlas instance.
                       - removed allocator parameters from CreateContext(), they are now setup with SetAllocatorFunctions(), and shared by all contexts.
                       - removed the default global context and font atlas instance, which were confusing for users of DLL reloading and users of multiple contexts.
 - 2018/01/31 (1.60) - moved sample TTF files from extra_fonts/ to misc/fonts/. If you loaded files directly from the imgui repo you may need to update your paths.
 - 2018/01/11 (1.60) - obsoleted IsAnyWindowHovered() in favor of IsWindowHovered(ImGuiHoveredFlags_AnyWindow). Kept redirection function (will obsolete).
 - 2018/01/11 (1.60) - obsoleted IsAnyWindowFocused() in favor of IsWindowFocused(ImGuiFocusedFlags_AnyWindow). Kept redirection function (will obsolete).
 - 2018/01/03 (1.60) - renamed ImGuiSizeConstraintCallback to ImGuiSizeCallback, ImGuiSizeConstraintCallbackData to ImGuiSizeCallbackData.
 - 2017/12/29 (1.60) - removed CalcItemRectClosestPoint() which was weird and not really used by anyone except demo code. If you need it it's easy to replicate on your side.
 - 2017/12/24 (1.53) - renamed the emblematic ShowTestWindow() function to ShowDemoWindow(). Kept redirection function (will obsolete).
 - 2017/12/21 (1.53) - ImDrawList: renamed style.AntiAliasedShapes to style.AntiAliasedFill for consistency and as a way to explicitly break code that manipulate those flag at runtime. You can now manipulate ImDrawList::Flags
 - 2017/12/21 (1.53) - ImDrawList: removed 'bool anti_aliased = true' final parameter of ImDrawList::AddPolyline() and ImDrawList::AddConvexPolyFilled(). Prefer manipulating ImDrawList::Flags if you need to toggle them during the frame.
 - 2017/12/14 (1.53) - using the ImGuiWindowFlags_NoScrollWithMouse flag on a child window forwards the mouse wheel event to the parent window, unless either ImGuiWindowFlags_NoInputs or ImGuiWindowFlags_NoScrollbar are also set.
 - 2017/12/13 (1.53) - renamed GetItemsLineHeightWithSpacing() to GetFrameHeightWithSpacing(). Kept redirection function (will obsolete).
 - 2017/12/13 (1.53) - obsoleted IsRootWindowFocused() in favor of using IsWindowFocused(ImGuiFocusedFlags_RootWindow). Kept redirection function (will obsolete).
                     - obsoleted IsRootWindowOrAnyChildFocused() in favor of using IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows). Kept redirection function (will obsolete).
 - 2017/12/12 (1.53) - renamed ImGuiTreeNodeFlags_AllowOverlapMode to ImGuiTreeNodeFlags_AllowItemOverlap. Kept redirection enum (will obsolete).
 - 2017/12/10 (1.53) - removed SetNextWindowContentWidth(), prefer using SetNextWindowContentSize(). Kept redirection function (will obsolete).
 - 2017/11/27 (1.53) - renamed ImGuiTextBuffer::append() helper to appendf(), appendv() to appendfv(). If you copied the 'Log' demo in your code, it uses appendv() so that needs to be renamed.
 - 2017/11/18 (1.53) - Style, Begin: removed ImGuiWindowFlags_ShowBorders window flag. Borders are now fully set up in the ImGuiStyle structure (see e.g. style.FrameBorderSize, style.WindowBorderSize). Use ImGui::ShowStyleEditor() to look them up.
                       Please note that the style system will keep evolving (hopefully stabilizing in Q1 2018), and so custom styles will probably subtly break over time. It is recommended you use the StyleColorsClassic(), StyleColorsDark(), StyleColorsLight() functions.
 - 2017/11/18 (1.53) - Style: removed ImGuiCol_ComboBg in favor of combo boxes using ImGuiCol_PopupBg for consistency.
 - 2017/11/18 (1.53) - Style: renamed ImGuiCol_ChildWindowBg to ImGuiCol_ChildBg.
 - 2017/11/18 (1.53) - Style: renamed style.ChildWindowRounding to style.ChildRounding, ImGuiStyleVar_ChildWindowRounding to ImGuiStyleVar_ChildRounding.
 - 2017/11/02 (1.53) - obsoleted IsRootWindowOrAnyChildHovered() in favor of using IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
 - 2017/10/24 (1.52) - renamed IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCS/IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS to IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS/IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS for consistency.
 - 2017/10/20 (1.52) - changed IsWindowHovered() default parameters behavior to return false if an item is active in another window (e.g. click-dragging item from another window to this window). You can use the newly introduced IsWindowHovered() flags to requests this specific behavior if you need it.
 - 2017/10/20 (1.52) - marked IsItemHoveredRect()/IsMouseHoveringWindow() as obsolete, in favor of using the newly introduced flags for IsItemHovered() and IsWindowHovered(). See https://github.com/ocornut/imgui/issues/1382 for details.
                       removed the IsItemRectHovered()/IsWindowRectHovered() names introduced in 1.51 since they were merely more consistent names for the two functions we are now obsoleting.
                         IsItemHoveredRect()        --> IsItemHovered(ImGuiHoveredFlags_RectOnly)
                         IsMouseHoveringAnyWindow() --> IsWindowHovered(ImGuiHoveredFlags_AnyWindow)
                         IsMouseHoveringWindow()    --> IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) [weird, old behavior]
 - 2017/10/17 (1.52) - marked the old 5-parameters version of Begin() as obsolete (still available). Use SetNextWindowSize()+Begin() instead!
 - 2017/10/11 (1.52) - renamed AlignFirstTextHeightToWidgets() to AlignTextToFramePadding(). Kept inline redirection function (will obsolete).
 - 2017/09/26 (1.52) - renamed ImFont::Glyph to ImFontGlyph. Kept redirection typedef (will obsolete).
 - 2017/09/25 (1.52) - removed SetNextWindowPosCenter() because SetNextWindowPos() now has the optional pivot information to do the same and more. Kept redirection function (will obsolete).
 - 2017/08/25 (1.52) - io.MousePos needs to be set to ImVec2(-FLT_MAX,-FLT_MAX) when mouse is unavailable/missing. Previously ImVec2(-1,-1) was enough but we now accept negative mouse coordinates. In your backend if you need to support unavailable mouse, make sure to replace "io.MousePos = ImVec2(-1,-1)" with "io.MousePos = ImVec2(-FLT_MAX,-FLT_MAX)".
 - 2017/08/22 (1.51) - renamed IsItemHoveredRect() to IsItemRectHovered(). Kept inline redirection function (will obsolete). -> (1.52) use IsItemHovered(ImGuiHoveredFlags_RectOnly)!
                     - renamed IsMouseHoveringAnyWindow() to IsAnyWindowHovered() for consistency. Kept inline redirection function (will obsolete).
                     - renamed IsMouseHoveringWindow() to IsWindowRectHovered() for consistency. Kept inline redirection function (will obsolete).
 - 2017/08/20 (1.51) - renamed GetStyleColName() to GetStyleColorName() for consistency.
 - 2017/08/20 (1.51) - added PushStyleColor(ImGuiCol idx, ImU32 col) overload, which _might_ cause an "ambiguous call" compilation error if you are using ImColor() with implicit cast. Cast to ImU32 or ImVec4 explicily to fix.
 - 2017/08/15 (1.51) - marked the weird IMGUI_ONCE_UPON_A_FRAME helper macro as obsolete. prefer using the more explicit ImGuiOnceUponAFrame type.
 - 2017/08/15 (1.51) - changed parameter order for BeginPopupContextWindow() from (const char*,int buttons,bool also_over_items) to (const char*,int buttons,bool also_over_items). Note that most calls relied on default parameters completely.
 - 2017/08/13 (1.51) - renamed ImGuiCol_Column to ImGuiCol_Separator, ImGuiCol_ColumnHovered to ImGuiCol_SeparatorHovered, ImGuiCol_ColumnActive to ImGuiCol_SeparatorActive. Kept redirection enums (will obsolete).
 - 2017/08/11 (1.51) - renamed ImGuiSetCond_Always to ImGuiCond_Always, ImGuiSetCond_Once to ImGuiCond_Once, ImGuiSetCond_FirstUseEver to ImGuiCond_FirstUseEver, ImGuiSetCond_Appearing to ImGuiCond_Appearing. Kept redirection enums (will obsolete).
 - 2017/08/09 (1.51) - removed ValueColor() helpers, they are equivalent to calling Text(label) + SameLine() + ColorButton().
 - 2017/08/08 (1.51) - removed ColorEditMode() and ImGuiColorEditMode in favor of ImGuiColorEditFlags and parameters to the various Color*() functions. The SetColorEditOptions() allows to initialize default but the user can still change them with right-click context menu.
                     - changed prototype of 'ColorEdit4(const char* label, float col[4], bool show_alpha = true)' to 'ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags = 0)', where passing flags = 0x01 is a safe no-op (hello dodgy backward compatibility!). - check and run the demo window, under "Color/Picker Widgets", to understand the various new options.
                     - changed prototype of rarely used 'ColorButton(ImVec4 col, bool small_height = false, bool outline_border = true)' to 'ColorButton(const char* desc_id, ImVec4 col, ImGuiColorEditFlags flags = 0, ImVec2 size = ImVec2(0, 0))'
 - 2017/07/20 (1.51) - removed IsPosHoveringAnyWindow(ImVec2), which was partly broken and misleading. ASSERT + redirect user to io.WantCaptureMouse
 - 2017/05/26 (1.50) - removed ImFontConfig::MergeGlyphCenterV in favor of a more multipurpose ImFontConfig::GlyphOffset.
 - 2017/05/01 (1.50) - renamed ImDrawList::PathFill() (rarely used directly) to ImDrawList::PathFillConvex() for clarity.
 - 2016/11/06 (1.50) - BeginChild(const char*) now applies the stack id to the provided label, consistently with other functions as it should always have been. It shouldn't affect you unless (extremely unlikely) you were appending multiple times to a same child from different locations of the stack id. If that's the case, generate an id with GetID() and use it instead of passing string to BeginChild().
 - 2016/10/15 (1.50) - avoid 'void* user_data' parameter to io.SetClipboardTextFn/io.GetClipboardTextFn pointers. We pass io.ClipboardUserData to it.
 - 2016/09/25 (1.50) - style.WindowTitleAlign is now a ImVec2 (ImGuiAlign enum was removed). set to (0.5f,0.5f) for horizontal+vertical centering, (0.0f,0.0f) for upper-left, etc.
 - 2016/07/30 (1.50) - SameLine(x) with x>0.0f is now relative to left of column/group if any, and not always to left of window. This was sort of always the intent and hopefully, breakage should be minimal.
 - 2016/05/12 (1.49) - title bar (using ImGuiCol_TitleBg/ImGuiCol_TitleBgActive colors) isn't rendered over a window background (ImGuiCol_WindowBg color) anymore.
                       If your TitleBg/TitleBgActive alpha was 1.0f or you are using the default theme it will not affect you, otherwise if <1.0f you need to tweak your custom theme to readjust for the fact that we don't draw a WindowBg background behind the title bar.
                       This helper function will convert an old TitleBg/TitleBgActive color into a new one with the same visual output, given the OLD color and the OLD WindowBg color:
                       ImVec4 ConvertTitleBgCol(const ImVec4& win_bg_col, const ImVec4& title_bg_col) { float new_a = 1.0f - ((1.0f - win_bg_col.w) * (1.0f - title_bg_col.w)), k = title_bg_col.w / new_a; return ImVec4((win_bg_col.x * win_bg_col.w + title_bg_col.x) * k, (win_bg_col.y * win_bg_col.w + title_bg_col.y) * k, (win_bg_col.z * win_bg_col.w + title_bg_col.z) * k, new_a); }
                       If this is confusing, pick the RGB value from title bar from an old screenshot and apply this as TitleBg/TitleBgActive. Or you may just create TitleBgActive from a tweaked TitleBg color.
 - 2016/05/07 (1.49) - removed confusing set of GetInternalState(), GetInternalStateSize(), SetInternalState() functions. Now using CreateContext(), DestroyContext(), GetCurrentContext(), SetCurrentContext().
 - 2016/05/02 (1.49) - renamed SetNextTreeNodeOpened() to SetNextTreeNodeOpen(), no redirection.
 - 2016/05/01 (1.49) - obsoleted old signature of CollapsingHeader(const char* label, const char* str_id = NULL, bool display_frame = true, bool default_open = false) as extra parameters were badly designed and rarely used. You can replace the "default_open = true" flag in new API with CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen).
 - 2016/04/26 (1.49) - changed ImDrawList::PushClipRect(ImVec4 rect) to ImDrawList::PushClipRect(Imvec2 min,ImVec2 max,bool intersect_with_current_clip_rect=false). Note that higher-level ImGui::PushClipRect() is preferable because it will clip at logic/widget level, whereas ImDrawList::PushClipRect() only affect your renderer.
 - 2016/04/03 (1.48) - removed style.WindowFillAlphaDefault setting which was redundant. Bake default BG alpha inside style.Colors[ImGuiCol_WindowBg] and all other Bg color values. (ref GitHub issue #337).
 - 2016/04/03 (1.48) - renamed ImGuiCol_TooltipBg to ImGuiCol_PopupBg, used by popups/menus and tooltips. popups/menus were previously using ImGuiCol_WindowBg. (ref github issue #337)
 - 2016/03/21 (1.48) - renamed GetWindowFont() to GetFont(), GetWindowFontSize() to GetFontSize(). Kept inline redirection function (will obsolete).
 - 2016/03/02 (1.48) - InputText() completion/history/always callbacks: if you modify the text buffer manually (without using DeleteChars()/InsertChars() helper) you need to maintain the BufTextLen field. added an assert.
 - 2016/01/23 (1.48) - fixed not honoring exact width passed to PushItemWidth(), previously it would add extra FramePadding.x*2 over that width. if you had manual pixel-perfect alignment in place it might affect you.
 - 2015/12/27 (1.48) - fixed ImDrawList::AddRect() which used to render a rectangle 1 px too large on each axis.
 - 2015/12/04 (1.47) - renamed Color() helpers to ValueColor() - dangerously named, rarely used and probably to be made obsolete.
 - 2015/08/29 (1.45) - with the addition of horizontal scrollbar we made various fixes to inconsistencies with dealing with cursor position.
                       GetCursorPos()/SetCursorPos() functions now include the scrolled amount. It shouldn't affect the majority of users, but take note that SetCursorPosX(100.0f) puts you at +100 from the starting x position which may include scrolling, not at +100 from the window left side.
                       GetContentRegionMax()/GetWindowContentRegionMin()/GetWindowContentRegionMax() functions allow include the scrolled amount. Typically those were used in cases where no scrolling would happen so it may not be a problem, but watch out!
 - 2015/08/29 (1.45) - renamed style.ScrollbarWidth to style.ScrollbarSize
 - 2015/08/05 (1.44) - split imgui.cpp into extra files: imgui_demo.cpp imgui_draw.cpp imgui_internal.h that you need to add to your project.
 - 2015/07/18 (1.44) - fixed angles in ImDrawList::PathArcTo(), PathArcToFast() (introduced in 1.43) being off by an extra PI for no justifiable reason
 - 2015/07/14 (1.43) - add new ImFontAtlas::AddFont() API. For the old AddFont***, moved the 'font_no' parameter of ImFontAtlas::AddFont** functions to the ImFontConfig structure.
                       you need to render your textured triangles with bilinear filtering to benefit from sub-pixel positioning of text.
 - 2015/07/08 (1.43) - switched rendering data to use indexed rendering. this is saving a fair amount of CPU/GPU and enables us to get anti-aliasing for a marginal cost.
                       this necessary change will break your rendering function! the fix should be very easy. sorry for that :(
                     - if you are using a vanilla copy of one of the imgui_impl_XXX.cpp provided in the example, you just need to update your copy and you can ignore the rest.
                     - the signature of the io.RenderDrawListsFn handler has changed!
                       old: ImGui_XXXX_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
                       new: ImGui_XXXX_RenderDrawLists(ImDrawData* draw_data).
                         parameters: 'cmd_lists' becomes 'draw_data->CmdLists', 'cmd_lists_count' becomes 'draw_data->CmdListsCount'
                         ImDrawList: 'commands' becomes 'CmdBuffer', 'vtx_buffer' becomes 'VtxBuffer', 'IdxBuffer' is new.
                         ImDrawCmd:  'vtx_count' becomes 'ElemCount', 'clip_rect' becomes 'ClipRect', 'user_callback' becomes 'UserCallback', 'texture_id' becomes 'TextureId'.
                     - each ImDrawList now contains both a vertex buffer and an index buffer. For each command, render ElemCount/3 triangles using indices from the index buffer.
                     - if you REALLY cannot render indexed primitives, you can call the draw_data->DeIndexAllBuffers() method to de-index the buffers. This is slow and a waste of CPU/GPU. Prefer using indexed rendering!
                     - refer to code in the examples/ folder or ask on the GitHub if you are unsure of how to upgrade. please upgrade!
 - 2015/07/10 (1.43) - changed SameLine() parameters from int to float.
 - 2015/07/02 (1.42) - renamed SetScrollPosHere() to SetScrollFromCursorPos(). Kept inline redirection function (will obsolete).
 - 2015/07/02 (1.42) - renamed GetScrollPosY() to GetScrollY(). Necessary to reduce confusion along with other scrolling functions, because positions (e.g. cursor position) are not equivalent to scrolling amount.
 - 2015/06/14 (1.41) - changed ImageButton() default bg_col parameter from (0,0,0,1) (black) to (0,0,0,0) (transparent) - makes a difference when texture have transparence
 - 2015/06/14 (1.41) - changed Selectable() API from (label, selected, size) to (label, selected, flags, size). Size override should have been rarely used. Sorry!
 - 2015/05/31 (1.40) - renamed GetWindowCollapsed() to IsWindowCollapsed() for consistency. Kept inline redirection function (will obsolete).
 - 2015/05/31 (1.40) - renamed IsRectClipped() to IsRectVisible() for consistency. Note that return value is opposite! Kept inline redirection function (will obsolete).
 - 2015/05/27 (1.40) - removed the third 'repeat_if_held' parameter from Button() - sorry! it was rarely used and inconsistent. Use PushButtonRepeat(true) / PopButtonRepeat() to enable repeat on desired buttons.
 - 2015/05/11 (1.40) - changed BeginPopup() API, takes a string identifier instead of a bool. ImGui needs to manage the open/closed state of popups. Call OpenPopup() to actually set the "open" state of a popup. BeginPopup() returns true if the popup is opened.
 - 2015/05/03 (1.40) - removed style.AutoFitPadding, using style.WindowPadding makes more sense (the default values were already the same).
 - 2015/04/13 (1.38) - renamed IsClipped() to IsRectClipped(). Kept inline redirection function until 1.50.
 - 2015/04/09 (1.38) - renamed ImDrawList::AddArc() to ImDrawList::AddArcFast() for compatibility with future API
 - 2015/04/03 (1.38) - removed ImGuiCol_CheckHovered, ImGuiCol_CheckActive, replaced with the more general ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive.
 - 2014/04/03 (1.38) - removed support for passing -FLT_MAX..+FLT_MAX as the range for a SliderFloat(). Use DragFloat() or Inputfloat() instead.
 - 2015/03/17 (1.36) - renamed GetItemBoxMin()/GetItemBoxMax()/IsMouseHoveringBox() to GetItemRectMin()/GetItemRectMax()/IsMouseHoveringRect(). Kept inline redirection function until 1.50.
 - 2015/03/15 (1.36) - renamed style.TreeNodeSpacing to style.IndentSpacing, ImGuiStyleVar_TreeNodeSpacing to ImGuiStyleVar_IndentSpacing
 - 2015/03/13 (1.36) - renamed GetWindowIsFocused() to IsWindowFocused(). Kept inline redirection function until 1.50.
 - 2015/03/08 (1.35) - renamed style.ScrollBarWidth to style.ScrollbarWidth (casing)
 - 2015/02/27 (1.34) - renamed OpenNextNode(bool) to SetNextTreeNodeOpened(bool, ImGuiSetCond). Kept inline redirection function until 1.50.
 - 2015/02/27 (1.34) - renamed ImGuiSetCondition_*** to ImGuiSetCond_***, and _FirstUseThisSession becomes _Once.
 - 2015/02/11 (1.32) - changed text input callback ImGuiTextEditCallback return type from void-->int. reserved for future use, return 0 for now.
 - 2015/02/10 (1.32) - renamed GetItemWidth() to CalcItemWidth() to clarify its evolving behavior
 - 2015/02/08 (1.31) - renamed GetTextLineSpacing() to GetTextLineHeightWithSpacing()
 - 2015/02/01 (1.31) - removed IO.MemReallocFn (unused)
 - 2015/01/19 (1.30) - renamed ImGuiStorage::GetIntPtr()/GetFloatPtr() to GetIntRef()/GetIntRef() because Ptr was conflicting with actual pointer storage functions.
 - 2015/01/11 (1.30) - big font/image API change! now loads TTF file. allow for multiple fonts. no need for a PNG loader.
 - 2015/01/11 (1.30) - removed GetDefaultFontData(). uses io.Fonts->GetTextureData*() API to retrieve uncompressed pixels.
                       - old:  const void* png_data; unsigned int png_size; ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size); [..Upload texture to GPU..];
                       - new:  unsigned char* pixels; int width, height; io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height); [..Upload texture to GPU..]; io.Fonts->SetTexID(YourTexIdentifier);
                       you now have more flexibility to load multiple TTF fonts and manage the texture buffer for internal needs. It is now recommended that you sample the font texture with bilinear interpolation.
 - 2015/01/11 (1.30) - added texture identifier in ImDrawCmd passed to your render function (we can now render images). make sure to call io.Fonts->SetTexID()
 - 2015/01/11 (1.30) - removed IO.PixelCenterOffset (unnecessary, can be handled in user projection matrix)
 - 2015/01/11 (1.30) - removed ImGui::IsItemFocused() in favor of ImGui::IsItemActive() which handles all widgets
 - 2014/12/10 (1.18) - removed SetNewWindowDefaultPos() in favor of new generic API SetNextWindowPos(pos, ImGuiSetCondition_FirstUseEver)
 - 2014/11/28 (1.17) - moved IO.Font*** options to inside the IO.Font-> structure (FontYOffset, FontTexUvForWhite, FontBaseScale, FontFallbackGlyph)
 - 2014/11/26 (1.17) - reworked syntax of IMGUI_ONCE_UPON_A_FRAME helper macro to increase compiler compatibility
 - 2014/11/07 (1.15) - renamed IsHovered() to IsItemHovered()
 - 2014/10/02 (1.14) - renamed IMGUI_INCLUDE_IMGUI_USER_CPP to IMGUI_INCLUDE_IMGUI_USER_INL and imgui_user.cpp to imgui_user.inl (more IDE friendly)
 - 2014/09/25 (1.13) - removed 'text_end' parameter from IO.SetClipboardTextFn (the string is now always zero-terminated for simplicity)
 - 2014/09/24 (1.12) - renamed SetFontScale() to SetWindowFontScale()
 - 2014/09/24 (1.12) - moved IM_MALLOC/IM_REALLOC/IM_FREE preprocessor defines to IO.MemAllocFn/IO.MemReallocFn/IO.MemFreeFn
 - 2014/08/30 (1.09) - removed IO.FontHeight (now computed automatically)
 - 2014/08/30 (1.09) - moved IMGUI_FONT_TEX_UV_FOR_WHITE preprocessor define to IO.FontTexUvForWhite
 - 2014/08/28 (1.09) - changed the behavior of IO.PixelCenterOffset following various rendering fixes


 FREQUENTLY ASKED QUESTIONS (FAQ)
 ================================

 Read all answers online:
   https://www.dearimgui.com/faq or https://github.com/ocornut/imgui/blob/master/docs/FAQ.md (same url)
 Read all answers locally (with a text editor or ideally a Markdown viewer):
   docs/FAQ.md
 Some answers are copied down here to facilitate searching in code.

 Q&A: Basics
 ===========

 Q: Where is the documentation?
 A: This library is poorly documented at the moment and expects the user to be acquainted with C/C++.
    - Run the examples/ applications and explore them.
    - Read Getting Started (https://github.com/ocornut/imgui/wiki/Getting-Started) guide.
    - See demo code in imgui_demo.cpp and particularly the ImGui::ShowDemoWindow() function.
    - The demo covers most features of Dear ImGui, so you can read the code and see its output.
    - See documentation and comments at the top of imgui.cpp + effectively imgui.h.
    - 20+ standalone example applications using e.g. OpenGL/DirectX are provided in the
      examples/ folder to explain how to integrate Dear ImGui with your own engine/application.
    - The Wiki (https://github.com/ocornut/imgui/wiki) has many resources and links.
    - The Glossary (https://github.com/ocornut/imgui/wiki/Glossary) page also may be useful.
    - Your programming IDE is your friend, find the type or function declaration to find comments
      associated with it.

 Q: What is this library called?
 Q: Which version should I get?
 >> This library is called "Dear ImGui", please don't call it "ImGui" :)
 >> See https://www.dearimgui.com/faq for details.

 Q&A: Integration
 ================

 Q: How to get started?
 A: Read https://github.com/ocornut/imgui/wiki/Getting-Started. Read 'PROGRAMMER GUIDE' above. Read examples/README.txt.

 Q: How can I tell whether to dispatch mouse/keyboard to Dear ImGui or my application?
 A: You should read the 'io.WantCaptureMouse', 'io.WantCaptureKeyboard' and 'io.WantTextInput' flags!
 >> See https://www.dearimgui.com/faq for a fully detailed answer. You really want to read this.

 Q. How can I enable keyboard or gamepad controls?
 Q: How can I use this on a machine without mouse, keyboard or screen? (input share, remote display)
 Q: I integrated Dear ImGui in my engine and little squares are showing instead of text...
 Q: I integrated Dear ImGui in my engine and some elements are clipping or disappearing when I move windows around...
 Q: I integrated Dear ImGui in my engine and some elements are displaying outside their expected windows boundaries...
 >> See https://www.dearimgui.com/faq

 Q&A: Usage
 ----------

 Q: About the ID Stack system..
   - Why is my widget not reacting when I click on it?
   - How can I have widgets with an empty label?
   - How can I have multiple widgets with the same label?
   - How can I have multiple windows with the same label?
 Q: How can I display an image? What is ImTextureID, how does it work?
 Q: How can I use my own math types instead of ImVec2?
 Q: How can I interact with standard C++ types (such as std::string and std::vector)?
 Q: How can I display custom shapes? (using low-level ImDrawList API)
 >> See https://www.dearimgui.com/faq

 Q&A: Fonts, Text
 ================

 Q: How should I handle DPI in my application?
 Q: How can I load a different font than the default?
 Q: How can I easily use icons in my application?
 Q: How can I load multiple fonts?
 Q: How can I display and input non-Latin characters such as Chinese, Japanese, Korean, Cyrillic?
 >> See https://www.dearimgui.com/faq and https://github.com/ocornut/imgui/blob/master/docs/FONTS.md

 Q&A: Concerns
 =============

 Q: Who uses Dear ImGui?
 Q: Can you create elaborate/serious tools with Dear ImGui?
 Q: Can you reskin the look of Dear ImGui?
 Q: Why using C++ (as opposed to C)?
 >> See https://www.dearimgui.com/faq

 Q&A: Community
 ==============

 Q: How can I help?
 A: - Businesses: please reach out to "omar AT dearimgui DOT com" if you work in a place using Dear ImGui!
      We can discuss ways for your company to fund development via invoiced technical support, maintenance or sponsoring contacts.
      This is among the most useful thing you can do for Dear ImGui. With increased funding, we sustain and grow work on this project.
      >>> See https://github.com/ocornut/imgui/wiki/Funding
    - Businesses: you can also purchase licenses for the Dear ImGui Automation/Test Engine.
    - If you are experienced with Dear ImGui and C++, look at the GitHub issues, look at the Wiki, and see how you want to help and can help!
    - Disclose your usage of Dear ImGui via a dev blog post, a tweet, a screenshot, a mention somewhere etc.
      You may post screenshot or links in the gallery threads. Visuals are ideal as they inspire other programmers.
      But even without visuals, disclosing your use of dear imgui helps the library grow credibility, and help other teams and programmers with taking decisions.
    - If you have issues or if you need to hack into the library, even if you don't expect any support it is useful that you share your issues (on GitHub or privately).

*/

//-------------------------------------------------------------------------
// [SECTION] INCLUDES
//-------------------------------------------------------------------------

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_internal.h"

// System includes
#include <stdio.h>      // vsnprintf, sscanf, printf
#include <stdint.h>     // intptr_t

// [Windows] On non-Visual Studio compilers, we default to IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS unless explicitly enabled
#if defined(_WIN32) && !defined(_MSC_VER) && !defined(IMGUI_ENABLE_WIN32_DEFAULT_IME_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS)
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
#endif

// [Windows] OS specific includes (optional)
#if defined(_WIN32) && defined(IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS) && defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS) && defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS) && defined(IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#define IMGUI_DISABLE_WIN32_FUNCTIONS
#endif
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef __MINGW32__
#include <Windows.h>        // _wfopen, OpenClipboard
#else
#include <windows.h>
#endif
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP || WINAPI_FAMILY == WINAPI_FAMILY_GAMES)
// The UWP and GDK Win32 API subsets don't support clipboard nor IME functions
#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
#define IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
#endif
#endif

// [Apple] OS specific includes
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)             // condition expression is constant
#pragma warning (disable: 4996)             // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#if defined(_MSC_VER) && _MSC_VER >= 1922   // MSVC 2019 16.2 or later
#pragma warning (disable: 5054)             // operator '|': deprecated between enumerations of different types
#endif
#pragma warning (disable: 26451)            // [Static Analyzer] Arithmetic overflow : Using operator 'xxx' on a 4 byte value and then casting the result to an 8 byte value. Cast the value to the wider type before calling operator 'xxx' to avoid overflow(io.2).
#pragma warning (disable: 26495)            // [Static Analyzer] Variable 'XXX' is uninitialized. Always initialize a member variable (type.6).
#pragma warning (disable: 26812)            // [Static Analyzer] The enum type 'xxx' is unscoped. Prefer 'enum class' over 'enum' (Enum.3).
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                      // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"                    // warning: comparing floating point with == or != is unsafe // storing and comparing against same constants (typically 0.0f) is ok.
#pragma clang diagnostic ignored "-Wformat-nonliteral"              // warning: format string is not a string literal            // passing non-literal to vsnformat(). yes, user passing incorrect format strings can crash the code.
#pragma clang diagnostic ignored "-Wexit-time-destructors"          // warning: declaration requires an exit-time destructor     // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. ImGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wglobal-constructors"            // warning: declaration requires a global destructor         // similar to above, not sure what the exact difference is.
#pragma clang diagnostic ignored "-Wsign-conversion"                // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wformat-pedantic"                // warning: format specifies type 'void *' but the argument has type 'xxxx *' // unreasonable, would lead to casting every %p arg to void*. probably enabled by -pedantic.
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"       // warning: cast to 'void *' from smaller integer type 'int'
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                    // some standard header variations use #define NULL 0
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"            // warning: 'xxx' is an unsafe pointer used for buffer access
#elif defined(__GNUC__)
// We disable -Wpragmas because GCC doesn't provide a has_warning equivalent and some forks/patches may not follow the warning/version association.
#pragma GCC diagnostic ignored "-Wpragmas"                  // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"      // warning: cast to pointer from integer of different size
#pragma GCC diagnostic ignored "-Wformat"                   // warning: format '%p' expects argument of type 'void*', but argument 6 has type 'ImGuiWindow*'
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wformat-nonliteral"        // warning: format not a string literal, format string not checked
#pragma GCC diagnostic ignored "-Wstrict-overflow"          // warning: assuming signed overflow does not occur when assuming that (X - c) > X is always false
#pragma GCC diagnostic ignored "-Wclass-memaccess"          // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif

// Debug options
#define IMGUI_DEBUG_NAV_SCORING     0   // Display navigation scoring preview when hovering items. Hold CTRL to display for all candidates. CTRL+Arrow to change last direction.
#define IMGUI_DEBUG_NAV_RECTS       0   // Display the reference navigation rectangle for each window

// When using CTRL+TAB (or Gamepad Square+L/R) we delay the visual a little in order to reduce visual noise doing a fast switch.
static const float NAV_WINDOWING_HIGHLIGHT_DELAY            = 0.20f;    // Time before the highlight and screen dimming starts fading in
static const float NAV_WINDOWING_LIST_APPEAR_DELAY          = 0.15f;    // Time before the window list starts to appear

static const float NAV_ACTIVATE_HIGHLIGHT_TIMER             = 0.10f;    // Time to highlight an item activated by a shortcut.

// Window resizing from edges (when io.ConfigWindowsResizeFromEdges = true and ImGuiBackendFlags_HasMouseCursors is set in io.BackendFlags by backend)
static const float WINDOWS_HOVER_PADDING                    = 4.0f;     // Extend outside window for hovering/resizing (maxxed with TouchPadding) and inside windows for borders. Affect FindHoveredWindow().
static const float WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER = 0.04f;    // Reduce visual noise by only highlighting the border after a certain time.
static const float WINDOWS_MOUSE_WHEEL_SCROLL_LOCK_TIMER    = 0.70f;    // Lock scrolled window (so it doesn't pick child windows that are scrolling through) for a certain time, unless mouse moved.

// Tooltip offset
static const ImVec2 TOOLTIP_DEFAULT_OFFSET_MOUSE = ImVec2(16, 10);      // Multiplied by g.Style.MouseCursorScale
static const ImVec2 TOOLTIP_DEFAULT_OFFSET_TOUCH = ImVec2(0, -20);      // Multiplied by g.Style.MouseCursorScale
static const ImVec2 TOOLTIP_DEFAULT_PIVOT_TOUCH = ImVec2(0.5f, 1.0f);   // Multiplied by g.Style.MouseCursorScale

//-------------------------------------------------------------------------
// [SECTION] FORWARD DECLARATIONS
//-------------------------------------------------------------------------

static void             SetCurrentWindow(ImGuiWindow* window);
static ImGuiWindow*     CreateNewWindow(const char* name, ImGuiWindowFlags flags);
static ImVec2           CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window);

static void             AddWindowToSortBuffer(ImVector<ImGuiWindow*>* out_sorted_windows, ImGuiWindow* window);

// Settings
static void             WindowSettingsHandler_ClearAll(ImGuiContext*, ImGuiSettingsHandler*);
static void*            WindowSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
static void             WindowSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
static void             WindowSettingsHandler_ApplyAll(ImGuiContext*, ImGuiSettingsHandler*);
static void             WindowSettingsHandler_WriteAll(ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer* buf);

// Platform Dependents default implementation for ImGuiPlatformIO functions
static const char*      Platform_GetClipboardTextFn_DefaultImpl(ImGuiContext* ctx);
static void             Platform_SetClipboardTextFn_DefaultImpl(ImGuiContext* ctx, const char* text);
static void             Platform_SetImeDataFn_DefaultImpl(ImGuiContext* ctx, ImGuiViewport* viewport, ImGuiPlatformImeData* data);
static bool             Platform_OpenInShellFn_DefaultImpl(ImGuiContext* ctx, const char* path);

namespace ImGui
{
// Item
static void             ItemHandleShortcut(ImGuiID id);

// Navigation
static void             NavUpdate();
static void             NavUpdateWindowing();
static void             NavUpdateWindowingOverlay();
static void             NavUpdateCancelRequest();
static void             NavUpdateCreateMoveRequest();
static void             NavUpdateCreateTabbingRequest();
static float            NavUpdatePageUpPageDown();
static inline void      NavUpdateAnyRequestFlag();
static void             NavUpdateCreateWrappingRequest();
static void             NavEndFrame();
static bool             NavScoreItem(ImGuiNavItemData* result);
static void             NavApplyItemToResult(ImGuiNavItemData* result);
static void             NavProcessItem();
static void             NavProcessItemForTabbingRequest(ImGuiID id, ImGuiItemFlags item_flags, ImGuiNavMoveFlags move_flags);
static ImVec2           NavCalcPreferredRefPos();
static void             NavSaveLastChildNavWindowIntoParent(ImGuiWindow* nav_window);
static ImGuiWindow*     NavRestoreLastChildNavWindow(ImGuiWindow* window);
static void             NavRestoreLayer(ImGuiNavLayer layer);
static int              FindWindowFocusIndex(ImGuiWindow* window);

// Error Checking and Debug Tools
static void             ErrorCheckNewFrameSanityChecks();
static void             ErrorCheckEndFrameSanityChecks();
static void             UpdateDebugToolItemPicker();
static void             UpdateDebugToolStackQueries();
static void             UpdateDebugToolFlashStyleColor();

// Inputs
static void             UpdateKeyboardInputs();
static void             UpdateMouseInputs();
static void             UpdateMouseWheel();
static void             UpdateKeyRoutingTable(ImGuiKeyRoutingTable* rt);

// Misc
static void             UpdateSettings();
static int              UpdateWindowManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_hovered, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4], const ImRect& visibility_rect);
static void             RenderWindowOuterBorders(ImGuiWindow* window);
static void             RenderWindowDecorations(ImGuiWindow* window, const ImRect& title_bar_rect, bool title_bar_is_highlight, bool handle_borders_and_resize_grips, int resize_grip_count, const ImU32 resize_grip_col[4], float resize_grip_draw_size);
static void             RenderWindowTitleBarContents(ImGuiWindow* window, const ImRect& title_bar_rect, const char* name, bool* p_open);
static void             RenderDimmedBackgroundBehindWindow(ImGuiWindow* window, ImU32 col);
static void             RenderDimmedBackgrounds();
static void             SetLastItemDataForWindow(ImGuiWindow* window, const ImRect& rect);

// Viewports
const ImGuiID           IMGUI_VIEWPORT_DEFAULT_ID = 0x11111111; // Using an arbitrary constant instead of e.g. ImHashStr("ViewportDefault", 0); so it's easier to spot in the debugger. The exact value doesn't matter.
static void             UpdateViewportsNewFrame();

}

//-----------------------------------------------------------------------------
// [SECTION] CONTEXT AND MEMORY ALLOCATORS
//-----------------------------------------------------------------------------

// DLL users:
// - Heaps and globals are not shared across DLL boundaries!
// - You will need to call SetCurrentContext() + SetAllocatorFunctions() for each static/DLL boundary you are calling from.
// - Same applies for hot-reloading mechanisms that are reliant on reloading DLL (note that many hot-reloading mechanisms work without DLL).
// - Using Dear ImGui via a shared library is not recommended, because of function call overhead and because we don't guarantee backward nor forward ABI compatibility.
// - Confused? In a debugger: add GImGui to your watch window and notice how its value changes depending on your current location (which DLL boundary you are in).

// Current context pointer. Implicitly used by all Dear ImGui functions. Always assumed to be != NULL.
// - ImGui::CreateContext() will automatically set this pointer if it is NULL.
//   Change to a different context by calling ImGui::SetCurrentContext().
// - Important: Dear ImGui functions are not thread-safe because of this pointer.
//   If you want thread-safety to allow N threads to access N different contexts:
//   - Change this variable to use thread local storage so each thread can refer to a different context, in your imconfig.h:
//         struct ImGuiContext;
//         extern thread_local ImGuiContext* MyImGuiTLS;
//         #define GImGui MyImGuiTLS
//     And then define MyImGuiTLS in one of your cpp files. Note that thread_local is a C++11 keyword, earlier C++ uses compiler-specific keyword.
//   - Future development aims to make this context pointer explicit to all calls. Also read https://github.com/ocornut/imgui/issues/586
//   - If you need a finite number of contexts, you may compile and use multiple instances of the ImGui code from a different namespace.
// - DLL users: read comments above.
#ifndef GImGui
ImGuiContext*   GImGui = NULL;
#endif

// Memory Allocator functions. Use SetAllocatorFunctions() to change them.
// - You probably don't want to modify that mid-program, and if you use global/static e.g. ImVector<> instances you may need to keep them accessible during program destruction.
// - DLL users: read comments above.
#ifndef IMGUI_DISABLE_DEFAULT_ALLOCATORS
static void*   MallocWrapper(size_t size, void* user_data)    { IM_UNUSED(user_data); return malloc(size); }
static void    FreeWrapper(void* ptr, void* user_data)        { IM_UNUSED(user_data); free(ptr); }
#else
static void*   MallocWrapper(size_t size, void* user_data)    { IM_UNUSED(user_data); IM_UNUSED(size); IM_ASSERT(0); return NULL; }
static void    FreeWrapper(void* ptr, void* user_data)        { IM_UNUSED(user_data); IM_UNUSED(ptr); IM_ASSERT(0); }
#endif
static ImGuiMemAllocFunc    GImAllocatorAllocFunc = MallocWrapper;
static ImGuiMemFreeFunc     GImAllocatorFreeFunc = FreeWrapper;
static void*                GImAllocatorUserData = NULL;

//-----------------------------------------------------------------------------
// [SECTION] USER FACING STRUCTURES (ImGuiStyle, ImGuiIO, ImGuiPlatformIO)
//-----------------------------------------------------------------------------

ImGuiStyle::ImGuiStyle()
{
    Alpha                       = 1.0f;             // Global alpha applies to everything in Dear ImGui.
    DisabledAlpha               = 0.60f;            // Additional alpha multiplier applied by BeginDisabled(). Multiply over current value of Alpha.
    WindowPadding               = ImVec2(8,8);      // Padding within a window
    WindowRounding              = 0.0f;             // Radius of window corners rounding. Set to 0.0f to have rectangular windows. Large values tend to lead to variety of artifacts and are not recommended.
    WindowBorderSize            = 1.0f;             // Thickness of border around windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    WindowMinSize               = ImVec2(32,32);    // Minimum window size
    WindowTitleAlign            = ImVec2(0.0f,0.5f);// Alignment for title bar text
    WindowMenuButtonPosition    = ImGuiDir_Left;    // Position of the collapsing/docking button in the title bar (left/right). Defaults to ImGuiDir_Left.
    ChildRounding               = 0.0f;             // Radius of child window corners rounding. Set to 0.0f to have rectangular child windows
    ChildBorderSize             = 1.0f;             // Thickness of border around child windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    PopupRounding               = 0.0f;             // Radius of popup window corners rounding. Set to 0.0f to have rectangular child windows
    PopupBorderSize             = 1.0f;             // Thickness of border around popup or tooltip windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    FramePadding                = ImVec2(4,3);      // Padding within a framed rectangle (used by most widgets)
    FrameRounding               = 0.0f;             // Radius of frame corners rounding. Set to 0.0f to have rectangular frames (used by most widgets).
    FrameBorderSize             = 0.0f;             // Thickness of border around frames. Generally set to 0.0f or 1.0f. Other values not well tested.
    ItemSpacing                 = ImVec2(8,4);      // Horizontal and vertical spacing between widgets/lines
    ItemInnerSpacing            = ImVec2(4,4);      // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)
    CellPadding                 = ImVec2(4,2);      // Padding within a table cell. Cellpadding.x is locked for entire table. CellPadding.y may be altered between different rows.
    TouchExtraPadding           = ImVec2(0,0);      // Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!
    IndentSpacing               = 21.0f;            // Horizontal spacing when e.g. entering a tree node. Generally == (FontSize + FramePadding.x*2).
    ColumnsMinSpacing           = 6.0f;             // Minimum horizontal spacing between two columns. Preferably > (FramePadding.x + 1).
    ScrollbarSize               = 14.0f;            // Width of the vertical scrollbar, Height of the horizontal scrollbar
    ScrollbarRounding           = 9.0f;             // Radius of grab corners rounding for scrollbar
    GrabMinSize                 = 12.0f;            // Minimum width/height of a grab box for slider/scrollbar
    GrabRounding                = 0.0f;             // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
    LogSliderDeadzone           = 4.0f;             // The size in pixels of the dead-zone around zero on logarithmic sliders that cross zero.
    TabRounding                 = 4.0f;             // Radius of upper corners of a tab. Set to 0.0f to have rectangular tabs.
    TabBorderSize               = 0.0f;             // Thickness of border around tabs.
    TabMinWidthForCloseButton   = 0.0f;             // Minimum width for close button to appear on an unselected tab when hovered. Set to 0.0f to always show when hovering, set to FLT_MAX to never show close button unless selected.
    TabBarBorderSize            = 1.0f;             // Thickness of tab-bar separator, which takes on the tab active color to denote focus.
    TabBarOverlineSize          = 2.0f;             // Thickness of tab-bar overline, which highlights the selected tab-bar.
    TableAngledHeadersAngle     = 35.0f * (IM_PI / 180.0f); // Angle of angled headers (supported values range from -50 degrees to +50 degrees).
    TableAngledHeadersTextAlign = ImVec2(0.5f,0.0f);// Alignment of angled headers within the cell
    ColorButtonPosition         = ImGuiDir_Right;   // Side of the color button in the ColorEdit4 widget (left/right). Defaults to ImGuiDir_Right.
    ButtonTextAlign             = ImVec2(0.5f,0.5f);// Alignment of button text when button is larger than text.
    SelectableTextAlign         = ImVec2(0.0f,0.0f);// Alignment of selectable text. Defaults to (0.0f, 0.0f) (top-left aligned). It's generally important to keep this left-aligned if you want to lay multiple items on a same line.
    SeparatorTextBorderSize     = 3.0f;             // Thickness of border in SeparatorText()
    SeparatorTextAlign          = ImVec2(0.0f,0.5f);// Alignment of text within the separator. Defaults to (0.0f, 0.5f) (left aligned, center).
    SeparatorTextPadding        = ImVec2(20.0f,3.f);// Horizontal offset of text from each edge of the separator + spacing on other axis. Generally small values. .y is recommended to be == FramePadding.y.
    DisplayWindowPadding        = ImVec2(19,19);    // Window position are clamped to be visible within the display area or monitors by at least this amount. Only applies to regular windows.
    DisplaySafeAreaPadding      = ImVec2(3,3);      // If you cannot see the edge of your screen (e.g. on a TV) increase the safe area padding. Covers popups/tooltips as well regular windows.
    MouseCursorScale            = 1.0f;             // Scale software rendered mouse cursor (when io.MouseDrawCursor is enabled). May be removed later.
    AntiAliasedLines            = true;             // Enable anti-aliased lines/borders. Disable if you are really tight on CPU/GPU.
    AntiAliasedLinesUseTex      = true;             // Enable anti-aliased lines/borders using textures where possible. Require backend to render with bilinear filtering (NOT point/nearest filtering).
    AntiAliasedFill             = true;             // Enable anti-aliased filled shapes (rounded rectangles, circles, etc.).
    CurveTessellationTol        = 1.25f;            // Tessellation tolerance when using PathBezierCurveTo() without a specific number of segments. Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.
    CircleTessellationMaxError  = 0.30f;            // Maximum error (in pixels) allowed when using AddCircle()/AddCircleFilled() or drawing rounded corner rectangles with no explicit segment count specified. Decrease for higher quality but more geometry.

    // Behaviors
    HoverStationaryDelay        = 0.15f;            // Delay for IsItemHovered(ImGuiHoveredFlags_Stationary). Time required to consider mouse stationary.
    HoverDelayShort             = 0.15f;            // Delay for IsItemHovered(ImGuiHoveredFlags_DelayShort). Usually used along with HoverStationaryDelay.
    HoverDelayNormal            = 0.40f;            // Delay for IsItemHovered(ImGuiHoveredFlags_DelayNormal). "
    HoverFlagsForTooltipMouse   = ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled;    // Default flags when using IsItemHovered(ImGuiHoveredFlags_ForTooltip) or BeginItemTooltip()/SetItemTooltip() while using mouse.
    HoverFlagsForTooltipNav     = ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled;  // Default flags when using IsItemHovered(ImGuiHoveredFlags_ForTooltip) or BeginItemTooltip()/SetItemTooltip() while using keyboard/gamepad.

    // Default theme
    ImGui::StyleColorsDark(this);
}

// To scale your entire UI (e.g. if you want your app to use High DPI or generally be DPI aware) you may use this helper function. Scaling the fonts is done separately and is up to you.
// Important: This operation is lossy because we round all sizes to integer. If you need to change your scale multiples, call this over a freshly initialized ImGuiStyle structure rather than scaling multiple times.
void ImGuiStyle::ScaleAllSizes(float scale_factor)
{
    WindowPadding = ImTrunc(WindowPadding * scale_factor);
    WindowRounding = ImTrunc(WindowRounding * scale_factor);
    WindowMinSize = ImTrunc(WindowMinSize * scale_factor);
    ChildRounding = ImTrunc(ChildRounding * scale_factor);
    PopupRounding = ImTrunc(PopupRounding * scale_factor);
    FramePadding = ImTrunc(FramePadding * scale_factor);
    FrameRounding = ImTrunc(FrameRounding * scale_factor);
    ItemSpacing = ImTrunc(ItemSpacing * scale_factor);
    ItemInnerSpacing = ImTrunc(ItemInnerSpacing * scale_factor);
    CellPadding = ImTrunc(CellPadding * scale_factor);
    TouchExtraPadding = ImTrunc(TouchExtraPadding * scale_factor);
    IndentSpacing = ImTrunc(IndentSpacing * scale_factor);
    ColumnsMinSpacing = ImTrunc(ColumnsMinSpacing * scale_factor);
    ScrollbarSize = ImTrunc(ScrollbarSize * scale_factor);
    ScrollbarRounding = ImTrunc(ScrollbarRounding * scale_factor);
    GrabMinSize = ImTrunc(GrabMinSize * scale_factor);
    GrabRounding = ImTrunc(GrabRounding * scale_factor);
    LogSliderDeadzone = ImTrunc(LogSliderDeadzone * scale_factor);
    TabRounding = ImTrunc(TabRounding * scale_factor);
    TabMinWidthForCloseButton = (TabMinWidthForCloseButton != FLT_MAX) ? ImTrunc(TabMinWidthForCloseButton * scale_factor) : FLT_MAX;
    TabBarOverlineSize = ImTrunc(TabBarOverlineSize * scale_factor);
    SeparatorTextPadding = ImTrunc(SeparatorTextPadding * scale_factor);
    DisplayWindowPadding = ImTrunc(DisplayWindowPadding * scale_factor);
    DisplaySafeAreaPadding = ImTrunc(DisplaySafeAreaPadding * scale_factor);
    MouseCursorScale = ImTrunc(MouseCursorScale * scale_factor);
}

ImGuiIO::ImGuiIO()
{
    // Most fields are initialized with zero
    memset(this, 0, sizeof(*this));
    IM_STATIC_ASSERT(IM_ARRAYSIZE(ImGuiIO::MouseDown) == ImGuiMouseButton_COUNT && IM_ARRAYSIZE(ImGuiIO::MouseClicked) == ImGuiMouseButton_COUNT);

    // Settings
    ConfigFlags = ImGuiConfigFlags_None;
    BackendFlags = ImGuiBackendFlags_None;
    DisplaySize = ImVec2(-1.0f, -1.0f);
    DeltaTime = 1.0f / 60.0f;
    IniSavingRate = 5.0f;
    IniFilename = "imgui.ini"; // Important: "imgui.ini" is relative to current working dir, most apps will want to lock this to an absolute path (e.g. same path as executables).
    LogFilename = "imgui_log.txt";
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    for (int i = 0; i < ImGuiKey_COUNT; i++)
        KeyMap[i] = -1;
#endif
    UserData = NULL;

    Fonts = NULL;
    FontGlobalScale = 1.0f;
    FontDefault = NULL;
    FontAllowUserScaling = false;
    DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Miscellaneous options
    MouseDrawCursor = false;
#ifdef __APPLE__
    ConfigMacOSXBehaviors = true;  // Set Mac OS X style defaults based on __APPLE__ compile time flag
#else
    ConfigMacOSXBehaviors = false;
#endif
    ConfigNavSwapGamepadButtons = false;
    ConfigNavMoveSetMousePos = false;
    ConfigNavCaptureKeyboard = true;
    ConfigNavEscapeClearFocusWindow = false;
    ConfigInputTrickleEventQueue = true;
    ConfigInputTextCursorBlink = true;
    ConfigInputTextEnterKeepActive = false;
    ConfigDragClickToInputText = false;
    ConfigWindowsResizeFromEdges = true;
    ConfigWindowsMoveFromTitleBarOnly = false;
    ConfigScrollbarScrollByPage = true;
    ConfigMemoryCompactTimer = 60.0f;
    ConfigDebugIsDebuggerPresent = false;
    ConfigDebugHighlightIdConflicts = true;
    ConfigDebugBeginReturnValueOnce = false;
    ConfigDebugBeginReturnValueLoop = false;

    ConfigErrorRecovery = true;
    ConfigErrorRecoveryEnableAssert = true;
    ConfigErrorRecoveryEnableDebugLog = true;
    ConfigErrorRecoveryEnableTooltip = true;

    // Inputs Behaviors
    MouseDoubleClickTime = 0.30f;
    MouseDoubleClickMaxDist = 6.0f;
    MouseDragThreshold = 6.0f;
    KeyRepeatDelay = 0.275f;
    KeyRepeatRate = 0.050f;

    // Platform Functions
    // Note: Initialize() will setup default clipboard/ime handlers.
    BackendPlatformName = BackendRendererName = NULL;
    BackendPlatformUserData = BackendRendererUserData = BackendLanguageUserData = NULL;

    // Input (NB: we already have memset zero the entire structure!)
    MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    MousePosPrev = ImVec2(-FLT_MAX, -FLT_MAX);
    MouseSource = ImGuiMouseSource_Mouse;
    for (int i = 0; i < IM_ARRAYSIZE(MouseDownDuration); i++) MouseDownDuration[i] = MouseDownDurationPrev[i] = -1.0f;
    for (int i = 0; i < IM_ARRAYSIZE(KeysData); i++) { KeysData[i].DownDuration = KeysData[i].DownDurationPrev = -1.0f; }
    AppAcceptingEvents = true;
    BackendUsingLegacyKeyArrays = (ImS8)-1;
    BackendUsingLegacyNavInputArray = true; // assume using legacy array until proven wrong
}

// Pass in translated ASCII characters for text input.
// - with glfw you can get those from the callback set in glfwSetCharCallback()
// - on Windows you can get those using ToAscii+keyboard state, or via the WM_CHAR message
// FIXME: Should in theory be called "AddCharacterEvent()" to be consistent with new API
void ImGuiIO::AddInputCharacter(unsigned int c)
{
    IM_ASSERT(Ctx != NULL);
    ImGuiContext& g = *Ctx;
    if (c == 0 || !AppAcceptingEvents)
        return;

    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_Text;
    e.Source = ImGuiInputSource_Keyboard;
    e.EventId = g.InputEventsNextEventId++;
    e.Text.Char = c;
    g.InputEventsQueue.push_back(e);
}

// UTF16 strings use surrogate pairs to encode codepoints >= 0x10000, so
// we should save the high surrogate.
void ImGuiIO::AddInputCharacterUTF16(ImWchar16 c)
{
    if ((c == 0 && InputQueueSurrogate == 0) || !AppAcceptingEvents)
        return;

    if ((c & 0xFC00) == 0xD800) // High surrogate, must save
    {
        if (InputQueueSurrogate != 0)
            AddInputCharacter(IM_UNICODE_CODEPOINT_INVALID);
        InputQueueSurrogate = c;
        return;
    }

    ImWchar cp = c;
    if (InputQueueSurrogate != 0)
    {
        if ((c & 0xFC00) != 0xDC00) // Invalid low surrogate
        {
            AddInputCharacter(IM_UNICODE_CODEPOINT_INVALID);
        }
        else
        {
#if IM_UNICODE_CODEPOINT_MAX == 0xFFFF
            cp = IM_UNICODE_CODEPOINT_INVALID; // Codepoint will not fit in ImWchar
#else
            cp = (ImWchar)(((InputQueueSurrogate - 0xD800) << 10) + (c - 0xDC00) + 0x10000);
#endif
        }

        InputQueueSurrogate = 0;
    }
    AddInputCharacter((unsigned)cp);
}

void ImGuiIO::AddInputCharactersUTF8(const char* utf8_chars)
{
    if (!AppAcceptingEvents)
        return;
    while (*utf8_chars != 0)
    {
        unsigned int c = 0;
        utf8_chars += ImTextCharFromUtf8(&c, utf8_chars, NULL);
        AddInputCharacter(c);
    }
}

// Clear all incoming events.
void ImGuiIO::ClearEventsQueue()
{
    IM_ASSERT(Ctx != NULL);
    ImGuiContext& g = *Ctx;
    g.InputEventsQueue.clear();
}

// Clear current keyboard/gamepad state + current frame text input buffer. Equivalent to releasing all keys/buttons.
void ImGuiIO::ClearInputKeys()
{
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    memset(KeysDown, 0, sizeof(KeysDown));
#endif
    for (int n = 0; n < IM_ARRAYSIZE(KeysData); n++)
    {
        if (ImGui::IsMouseKey((ImGuiKey)(n + ImGuiKey_KeysData_OFFSET)))
            continue;
        KeysData[n].Down             = false;
        KeysData[n].DownDuration     = -1.0f;
        KeysData[n].DownDurationPrev = -1.0f;
    }
    KeyCtrl = KeyShift = KeyAlt = KeySuper = false;
    KeyMods = ImGuiMod_None;
    InputQueueCharacters.resize(0); // Behavior of old ClearInputCharacters().
}

void ImGuiIO::ClearInputMouse()
{
    for (ImGuiKey key = ImGuiKey_Mouse_BEGIN; key < ImGuiKey_Mouse_END; key = (ImGuiKey)(key + 1))
    {
        ImGuiKeyData* key_data = &KeysData[key - ImGuiKey_KeysData_OFFSET];
        key_data->Down = false;
        key_data->DownDuration = -1.0f;
        key_data->DownDurationPrev = -1.0f;
    }
    MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    for (int n = 0; n < IM_ARRAYSIZE(MouseDown); n++)
    {
        MouseDown[n] = false;
        MouseDownDuration[n] = MouseDownDurationPrev[n] = -1.0f;
    }
    MouseWheel = MouseWheelH = 0.0f;
}

// Removed this as it is ambiguous/misleading and generally incorrect to use with the existence of a higher-level input queue.
// Current frame character buffer is now also cleared by ClearInputKeys().
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
void ImGuiIO::ClearInputCharacters()
{
    InputQueueCharacters.resize(0);
}
#endif

static ImGuiInputEvent* FindLatestInputEvent(ImGuiContext* ctx, ImGuiInputEventType type, int arg = -1)
{
    ImGuiContext& g = *ctx;
    for (int n = g.InputEventsQueue.Size - 1; n >= 0; n--)
    {
        ImGuiInputEvent* e = &g.InputEventsQueue[n];
        if (e->Type != type)
            continue;
        if (type == ImGuiInputEventType_Key && e->Key.Key != arg)
            continue;
        if (type == ImGuiInputEventType_MouseButton && e->MouseButton.Button != arg)
            continue;
        return e;
    }
    return NULL;
}

// Queue a new key down/up event.
// - ImGuiKey key:       Translated key (as in, generally ImGuiKey_A matches the key end-user would use to emit an 'A' character)
// - bool down:          Is the key down? use false to signify a key release.
// - float analog_value: 0.0f..1.0f
// IMPORTANT: THIS FUNCTION AND OTHER "ADD" GRABS THE CONTEXT FROM OUR INSTANCE.
// WE NEED TO ENSURE THAT ALL FUNCTION CALLS ARE FULFILLING THIS, WHICH IS WHY GetKeyData() HAS AN EXPLICIT CONTEXT.
void ImGuiIO::AddKeyAnalogEvent(ImGuiKey key, bool down, float analog_value)
{
    //if (e->Down) { IMGUI_DEBUG_LOG_IO("AddKeyEvent() Key='%s' %d, NativeKeycode = %d, NativeScancode = %d\n", ImGui::GetKeyName(e->Key), e->Down, e->NativeKeycode, e->NativeScancode); }
    IM_ASSERT(Ctx != NULL);
    if (key == ImGuiKey_None || !AppAcceptingEvents)
        return;
    ImGuiContext& g = *Ctx;
    IM_ASSERT(ImGui::IsNamedKeyOrMod(key)); // Backend needs to pass a valid ImGuiKey_ constant. 0..511 values are legacy native key codes which are not accepted by this API.
    IM_ASSERT(ImGui::IsAliasKey(key) == false); // Backend cannot submit ImGuiKey_MouseXXX values they are automatically inferred from AddMouseXXX() events.

    // MacOS: swap Cmd(Super) and Ctrl
    if (g.IO.ConfigMacOSXBehaviors)
    {
        if (key == ImGuiMod_Super)          { key = ImGuiMod_Ctrl; }
        else if (key == ImGuiMod_Ctrl)      { key = ImGuiMod_Super; }
        else if (key == ImGuiKey_LeftSuper) { key = ImGuiKey_LeftCtrl; }
        else if (key == ImGuiKey_RightSuper){ key = ImGuiKey_RightCtrl; }
        else if (key == ImGuiKey_LeftCtrl)  { key = ImGuiKey_LeftSuper; }
        else if (key == ImGuiKey_RightCtrl) { key = ImGuiKey_RightSuper; }
    }

    // Verify that backend isn't mixing up using new io.AddKeyEvent() api and old io.KeysDown[] + io.KeyMap[] data.
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    IM_ASSERT((BackendUsingLegacyKeyArrays == -1 || BackendUsingLegacyKeyArrays == 0) && "Backend needs to either only use io.AddKeyEvent(), either only fill legacy io.KeysDown[] + io.KeyMap[]. Not both!");
    if (BackendUsingLegacyKeyArrays == -1)
        for (int n = ImGuiKey_NamedKey_BEGIN; n < ImGuiKey_NamedKey_END; n++)
            IM_ASSERT(KeyMap[n] == -1 && "Backend needs to either only use io.AddKeyEvent(), either only fill legacy io.KeysDown[] + io.KeyMap[]. Not both!");
    BackendUsingLegacyKeyArrays = 0;
#endif
    if (ImGui::IsGamepadKey(key))
        BackendUsingLegacyNavInputArray = false;

    // Filter duplicate (in particular: key mods and gamepad analog values are commonly spammed)
    const ImGuiInputEvent* latest_event = FindLatestInputEvent(&g, ImGuiInputEventType_Key, (int)key);
    const ImGuiKeyData* key_data = ImGui::GetKeyData(&g, key);
    const bool latest_key_down = latest_event ? latest_event->Key.Down : key_data->Down;
    const float latest_key_analog = latest_event ? latest_event->Key.AnalogValue : key_data->AnalogValue;
    if (latest_key_down == down && latest_key_analog == analog_value)
        return;

    // Add event
    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_Key;
    e.Source = ImGui::IsGamepadKey(key) ? ImGuiInputSource_Gamepad : ImGuiInputSource_Keyboard;
    e.EventId = g.InputEventsNextEventId++;
    e.Key.Key = key;
    e.Key.Down = down;
    e.Key.AnalogValue = analog_value;
    g.InputEventsQueue.push_back(e);
}

void ImGuiIO::AddKeyEvent(ImGuiKey key, bool down)
{
    if (!AppAcceptingEvents)
        return;
    AddKeyAnalogEvent(key, down, down ? 1.0f : 0.0f);
}

// [Optional] Call after AddKeyEvent().
// Specify native keycode, scancode + Specify index for legacy <1.87 IsKeyXXX() functions with native indices.
// If you are writing a backend in 2022 or don't use IsKeyXXX() with native values that are not ImGuiKey values, you can avoid calling this.
void ImGuiIO::SetKeyEventNativeData(ImGuiKey key, int native_keycode, int native_scancode, int native_legacy_index)
{
    if (key == ImGuiKey_None)
        return;
    IM_ASSERT(ImGui::IsNamedKey(key)); // >= 512
    IM_ASSERT(native_legacy_index == -1 || ImGui::IsLegacyKey((ImGuiKey)native_legacy_index)); // >= 0 && <= 511
    IM_UNUSED(native_keycode);  // Yet unused
    IM_UNUSED(native_scancode); // Yet unused

    // Build native->imgui map so old user code can still call key functions with native 0..511 values.
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    const int legacy_key = (native_legacy_index != -1) ? native_legacy_index : native_keycode;
    if (!ImGui::IsLegacyKey((ImGuiKey)legacy_key))
        return;
    KeyMap[legacy_key] = key;
    KeyMap[key] = legacy_key;
#else
    IM_UNUSED(key);
    IM_UNUSED(native_legacy_index);
#endif
}

// Set master flag for accepting key/mouse/text events (default to true). Useful if you have native dialog boxes that are interrupting your application loop/refresh, and you want to disable events being queued while your app is frozen.
void ImGuiIO::SetAppAcceptingEvents(bool accepting_events)
{
    AppAcceptingEvents = accepting_events;
}

// Queue a mouse move event
void ImGuiIO::AddMousePosEvent(float x, float y)
{
    IM_ASSERT(Ctx != NULL);
    ImGuiContext& g = *Ctx;
    if (!AppAcceptingEvents)
        return;

    // Apply same flooring as UpdateMouseInputs()
    ImVec2 pos((x > -FLT_MAX) ? ImFloor(x) : x, (y > -FLT_MAX) ? ImFloor(y) : y);

    // Filter duplicate
    const ImGuiInputEvent* latest_event = FindLatestInputEvent(&g, ImGuiInputEventType_MousePos);
    const ImVec2 latest_pos = latest_event ? ImVec2(latest_event->MousePos.PosX, latest_event->MousePos.PosY) : g.IO.MousePos;
    if (latest_pos.x == pos.x && latest_pos.y == pos.y)
        return;

    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_MousePos;
    e.Source = ImGuiInputSource_Mouse;
    e.EventId = g.InputEventsNextEventId++;
    e.MousePos.PosX = pos.x;
    e.MousePos.PosY = pos.y;
    e.MousePos.MouseSource = g.InputEventsNextMouseSource;
    g.InputEventsQueue.push_back(e);
}

void ImGuiIO::AddMouseButtonEvent(int mouse_button, bool down)
{
    IM_ASSERT(Ctx != NULL);
    ImGuiContext& g = *Ctx;
    IM_ASSERT(mouse_button >= 0 && mouse_button < ImGuiMouseButton_COUNT);
    if (!AppAcceptingEvents)
        return;

    // On MacOS X: Convert Ctrl(Super)+Left click into Right-click: handle held button.
    if (ConfigMacOSXBehaviors && mouse_button == 0 && MouseCtrlLeftAsRightClick)
    {
        // Order of both statements matterns: this event will still release mouse button 1
        mouse_button = 1;
        if (!down)
            MouseCtrlLeftAsRightClick = false;
    }

    // Filter duplicate
    const ImGuiInputEvent* latest_event = FindLatestInputEvent(&g, ImGuiInputEventType_MouseButton, (int)mouse_button);
    const bool latest_button_down = latest_event ? latest_event->MouseButton.Down : g.IO.MouseDown[mouse_button];
    if (latest_button_down == down)
        return;

    // On MacOS X: Convert Ctrl(Super)+Left click into Right-click.
    // - Note that this is actual physical Ctrl which is ImGuiMod_Super for us.
    // - At this point we want from !down to down, so this is handling the initial press.
    if (ConfigMacOSXBehaviors && mouse_button == 0 && down)
    {
        const ImGuiInputEvent* latest_super_event = FindLatestInputEvent(&g, ImGuiInputEventType_Key, (int)ImGuiMod_Super);
        if (latest_super_event ? latest_super_event->Key.Down : g.IO.KeySuper)
        {
            IMGUI_DEBUG_LOG_IO("[io] Super+Left Click aliased into Right Click\n");
            MouseCtrlLeftAsRightClick = true;
            AddMouseButtonEvent(1, true); // This is just quicker to write that passing through, as we need to filter duplicate again.
            return;
        }
    }

    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_MouseButton;
    e.Source = ImGuiInputSource_Mouse;
    e.EventId = g.InputEventsNextEventId++;
    e.MouseButton.Button = mouse_button;
    e.MouseButton.Down = down;
    e.MouseButton.MouseSource = g.InputEventsNextMouseSource;
    g.InputEventsQueue.push_back(e);
}

// Queue a mouse wheel event (some mouse/API may only have a Y component)
void ImGuiIO::AddMouseWheelEvent(float wheel_x, float wheel_y)
{
    IM_ASSERT(Ctx != NULL);
    ImGuiContext& g = *Ctx;

    // Filter duplicate (unlike most events, wheel values are relative and easy to filter)
    if (!AppAcceptingEvents || (wheel_x == 0.0f && wheel_y == 0.0f))
        return;

    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_MouseWheel;
    e.Source = ImGuiInputSource_Mouse;
    e.EventId = g.InputEventsNextEventId++;
    e.MouseWheel.WheelX = wheel_x;
    e.MouseWheel.WheelY = wheel_y;
    e.MouseWheel.MouseSource = g.InputEventsNextMouseSource;
    g.InputEventsQueue.push_back(e);
}

// This is not a real event, the data is latched in order to be stored in actual Mouse events.
// This is so that duplicate events (e.g. Windows sending extraneous WM_MOUSEMOVE) gets filtered and are not leading to actual source changes.
void ImGuiIO::AddMouseSourceEvent(ImGuiMouseSource source)
{
    IM_ASSERT(Ctx != NULL);
    ImGuiContext& g = *Ctx;
    g.InputEventsNextMouseSource = source;
}

void ImGuiIO::AddFocusEvent(bool focused)
{
    IM_ASSERT(Ctx != NULL);
    ImGuiContext& g = *Ctx;

    // Filter duplicate
    const ImGuiInputEvent* latest_event = FindLatestInputEvent(&g, ImGuiInputEventType_Focus);
    const bool latest_focused = latest_event ? latest_event->AppFocused.Focused : !g.IO.AppFocusLost;
    if (latest_focused == focused || (ConfigDebugIgnoreFocusLoss && !focused))
        return;

    ImGuiInputEvent e;
    e.Type = ImGuiInputEventType_Focus;
    e.EventId = g.InputEventsNextEventId++;
    e.AppFocused.Focused = focused;
    g.InputEventsQueue.push_back(e);
}

ImGuiPlatformIO::ImGuiPlatformIO()
{
    // Most fields are initialized with zero
    memset(this, 0, sizeof(*this));
    Platform_LocaleDecimalPoint = '.';
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (Geometry functions)
//-----------------------------------------------------------------------------

ImVec2 ImBezierCubicClosestPoint(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& p, int num_segments)
{
    IM_ASSERT(num_segments > 0); // Use ImBezierCubicClosestPointCasteljau()
    ImVec2 p_last = p1;
    ImVec2 p_closest;
    float p_closest_dist2 = FLT_MAX;
    float t_step = 1.0f / (float)num_segments;
    for (int i_step = 1; i_step <= num_segments; i_step++)
    {
        ImVec2 p_current = ImBezierCubicCalc(p1, p2, p3, p4, t_step * i_step);
        ImVec2 p_line = ImLineClosestPoint(p_last, p_current, p);
        float dist2 = ImLengthSqr(p - p_line);
        if (dist2 < p_closest_dist2)
        {
            p_closest = p_line;
            p_closest_dist2 = dist2;
        }
        p_last = p_current;
    }
    return p_closest;
}

// Closely mimics PathBezierToCasteljau() in imgui_draw.cpp
static void ImBezierCubicClosestPointCasteljauStep(const ImVec2& p, ImVec2& p_closest, ImVec2& p_last, float& p_closest_dist2, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level)
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = ((x2 - x4) * dy - (y2 - y4) * dx);
    float d3 = ((x3 - x4) * dy - (y3 - y4) * dx);
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy))
    {
        ImVec2 p_current(x4, y4);
        ImVec2 p_line = ImLineClosestPoint(p_last, p_current, p);
        float dist2 = ImLengthSqr(p - p_line);
        if (dist2 < p_closest_dist2)
        {
            p_closest = p_line;
            p_closest_dist2 = dist2;
        }
        p_last = p_current;
    }
    else if (level < 10)
    {
        float x12 = (x1 + x2)*0.5f,       y12 = (y1 + y2)*0.5f;
        float x23 = (x2 + x3)*0.5f,       y23 = (y2 + y3)*0.5f;
        float x34 = (x3 + x4)*0.5f,       y34 = (y3 + y4)*0.5f;
        float x123 = (x12 + x23)*0.5f,    y123 = (y12 + y23)*0.5f;
        float x234 = (x23 + x34)*0.5f,    y234 = (y23 + y34)*0.5f;
        float x1234 = (x123 + x234)*0.5f, y1234 = (y123 + y234)*0.5f;
        ImBezierCubicClosestPointCasteljauStep(p, p_closest, p_last, p_closest_dist2, x1, y1, x12, y12, x123, y123, x1234, y1234, tess_tol, level + 1);
        ImBezierCubicClosestPointCasteljauStep(p, p_closest, p_last, p_closest_dist2, x1234, y1234, x234, y234, x34, y34, x4, y4, tess_tol, level + 1);
    }
}

// tess_tol is generally the same value you would find in ImGui::GetStyle().CurveTessellationTol
// Because those ImXXX functions are lower-level than ImGui:: we cannot access this value automatically.
ImVec2 ImBezierCubicClosestPointCasteljau(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& p, float tess_tol)
{
    IM_ASSERT(tess_tol > 0.0f);
    ImVec2 p_last = p1;
    ImVec2 p_closest;
    float p_closest_dist2 = FLT_MAX;
    ImBezierCubicClosestPointCasteljauStep(p, p_closest, p_last, p_closest_dist2, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, tess_tol, 0);
    return p_closest;
}

ImVec2 ImLineClosestPoint(const ImVec2& a, const ImVec2& b, const ImVec2& p)
{
    ImVec2 ap = p - a;
    ImVec2 ab_dir = b - a;
    float dot = ap.x * ab_dir.x + ap.y * ab_dir.y;
    if (dot < 0.0f)
        return a;
    float ab_len_sqr = ab_dir.x * ab_dir.x + ab_dir.y * ab_dir.y;
    if (dot > ab_len_sqr)
        return b;
    return a + ab_dir * dot / ab_len_sqr;
}

bool ImTriangleContainsPoint(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p)
{
    bool b1 = ((p.x - b.x) * (a.y - b.y) - (p.y - b.y) * (a.x - b.x)) < 0.0f;
    bool b2 = ((p.x - c.x) * (b.y - c.y) - (p.y - c.y) * (b.x - c.x)) < 0.0f;
    bool b3 = ((p.x - a.x) * (c.y - a.y) - (p.y - a.y) * (c.x - a.x)) < 0.0f;
    return ((b1 == b2) && (b2 == b3));
}

void ImTriangleBarycentricCoords(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p, float& out_u, float& out_v, float& out_w)
{
    ImVec2 v0 = b - a;
    ImVec2 v1 = c - a;
    ImVec2 v2 = p - a;
    const float denom = v0.x * v1.y - v1.x * v0.y;
    out_v = (v2.x * v1.y - v1.x * v2.y) / denom;
    out_w = (v0.x * v2.y - v2.x * v0.y) / denom;
    out_u = 1.0f - out_v - out_w;
}

ImVec2 ImTriangleClosestPoint(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p)
{
    ImVec2 proj_ab = ImLineClosestPoint(a, b, p);
    ImVec2 proj_bc = ImLineClosestPoint(b, c, p);
    ImVec2 proj_ca = ImLineClosestPoint(c, a, p);
    float dist2_ab = ImLengthSqr(p - proj_ab);
    float dist2_bc = ImLengthSqr(p - proj_bc);
    float dist2_ca = ImLengthSqr(p - proj_ca);
    float m = ImMin(dist2_ab, ImMin(dist2_bc, dist2_ca));
    if (m == dist2_ab)
        return proj_ab;
    if (m == dist2_bc)
        return proj_bc;
    return proj_ca;
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (String, Format, Hash functions)
//-----------------------------------------------------------------------------

// Consider using _stricmp/_strnicmp under Windows or strcasecmp/strncasecmp. We don't actually use either ImStricmp/ImStrnicmp in the codebase any more.
int ImStricmp(const char* str1, const char* str2)
{
    int d;
    while ((d = ImToUpper(*str2) - ImToUpper(*str1)) == 0 && *str1) { str1++; str2++; }
    return d;
}

int ImStrnicmp(const char* str1, const char* str2, size_t count)
{
    int d = 0;
    while (count > 0 && (d = ImToUpper(*str2) - ImToUpper(*str1)) == 0 && *str1) { str1++; str2++; count--; }
    return d;
}

void ImStrncpy(char* dst, const char* src, size_t count)
{
    if (count < 1)
        return;
    if (count > 1)
        strncpy(dst, src, count - 1);
    dst[count - 1] = 0;
}

char* ImStrdup(const char* str)
{
    size_t len = strlen(str);
    void* buf = IM_ALLOC(len + 1);
    return (char*)memcpy(buf, (const void*)str, len + 1);
}

char* ImStrdupcpy(char* dst, size_t* p_dst_size, const char* src)
{
    size_t dst_buf_size = p_dst_size ? *p_dst_size : strlen(dst) + 1;
    size_t src_size = strlen(src) + 1;
    if (dst_buf_size < src_size)
    {
        IM_FREE(dst);
        dst = (char*)IM_ALLOC(src_size);
        if (p_dst_size)
            *p_dst_size = src_size;
    }
    return (char*)memcpy(dst, (const void*)src, src_size);
}

const char* ImStrchrRange(const char* str, const char* str_end, char c)
{
    const char* p = (const char*)memchr(str, (int)c, str_end - str);
    return p;
}

int ImStrlenW(const ImWchar* str)
{
    //return (int)wcslen((const wchar_t*)str);  // FIXME-OPT: Could use this when wchar_t are 16-bit
    int n = 0;
    while (*str++) n++;
    return n;
}

// Find end-of-line. Return pointer will point to either first \n, either str_end.
const char* ImStreolRange(const char* str, const char* str_end)
{
    const char* p = (const char*)memchr(str, '\n', str_end - str);
    return p ? p : str_end;
}

const char* ImStrbol(const char* buf_mid_line, const char* buf_begin) // find beginning-of-line
{
    while (buf_mid_line > buf_begin && buf_mid_line[-1] != '\n')
        buf_mid_line--;
    return buf_mid_line;
}

const char* ImStristr(const char* haystack, const char* haystack_end, const char* needle, const char* needle_end)
{
    if (!needle_end)
        needle_end = needle + strlen(needle);

    const char un0 = (char)ImToUpper(*needle);
    while ((!haystack_end && *haystack) || (haystack_end && haystack < haystack_end))
    {
        if (ImToUpper(*haystack) == un0)
        {
            const char* b = needle + 1;
            for (const char* a = haystack + 1; b < needle_end; a++, b++)
                if (ImToUpper(*a) != ImToUpper(*b))
                    break;
            if (b == needle_end)
                return haystack;
        }
        haystack++;
    }
    return NULL;
}

// Trim str by offsetting contents when there's leading data + writing a \0 at the trailing position. We use this in situation where the cost is negligible.
void ImStrTrimBlanks(char* buf)
{
    char* p = buf;
    while (p[0] == ' ' || p[0] == '\t')     // Leading blanks
        p++;
    char* p_start = p;
    while (*p != 0)                         // Find end of string
        p++;
    while (p > p_start && (p[-1] == ' ' || p[-1] == '\t'))  // Trailing blanks
        p--;
    if (p_start != buf)                     // Copy memory if we had leading blanks
        memmove(buf, p_start, p - p_start);
    buf[p - p_start] = 0;                   // Zero terminate
}

const char* ImStrSkipBlank(const char* str)
{
    while (str[0] == ' ' || str[0] == '\t')
        str++;
    return str;
}

// A) MSVC version appears to return -1 on overflow, whereas glibc appears to return total count (which may be >= buf_size).
// Ideally we would test for only one of those limits at runtime depending on the behavior the vsnprintf(), but trying to deduct it at compile time sounds like a pandora can of worm.
// B) When buf==NULL vsnprintf() will return the output size.
#ifndef IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS

// We support stb_sprintf which is much faster (see: https://github.com/nothings/stb/blob/master/stb_sprintf.h)
// You may set IMGUI_USE_STB_SPRINTF to use our default wrapper, or set IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS
// and setup the wrapper yourself. (FIXME-OPT: Some of our high-level operations such as ImGuiTextBuffer::appendfv() are
// designed using two-passes worst case, which probably could be improved using the stbsp_vsprintfcb() function.)
#ifdef IMGUI_USE_STB_SPRINTF
#ifndef IMGUI_DISABLE_STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_IMPLEMENTATION
#endif
#ifdef IMGUI_STB_SPRINTF_FILENAME
#include IMGUI_STB_SPRINTF_FILENAME
#else
#include "stb_sprintf.h"
#endif
#endif // #ifdef IMGUI_USE_STB_SPRINTF

#if defined(_MSC_VER) && !defined(vsnprintf)
#define vsnprintf _vsnprintf
#endif

int ImFormatString(char* buf, size_t buf_size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef IMGUI_USE_STB_SPRINTF
    int w = stbsp_vsnprintf(buf, (int)buf_size, fmt, args);
#else
    int w = vsnprintf(buf, buf_size, fmt, args);
#endif
    va_end(args);
    if (buf == NULL)
        return w;
    if (w == -1 || w >= (int)buf_size)
        w = (int)buf_size - 1;
    buf[w] = 0;
    return w;
}

int ImFormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args)
{
#ifdef IMGUI_USE_STB_SPRINTF
    int w = stbsp_vsnprintf(buf, (int)buf_size, fmt, args);
#else
    int w = vsnprintf(buf, buf_size, fmt, args);
#endif
    if (buf == NULL)
        return w;
    if (w == -1 || w >= (int)buf_size)
        w = (int)buf_size - 1;
    buf[w] = 0;
    return w;
}
#endif // #ifdef IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS

void ImFormatStringToTempBuffer(const char** out_buf, const char** out_buf_end, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImFormatStringToTempBufferV(out_buf, out_buf_end, fmt, args);
    va_end(args);
}

// FIXME: Should rework API toward allowing multiple in-flight temp buffers (easier and safer for caller)
// by making the caller acquire a temp buffer token, with either explicit or destructor release, e.g.
//  ImGuiTempBufferToken token;
//  ImFormatStringToTempBuffer(token, ...);
void ImFormatStringToTempBufferV(const char** out_buf, const char** out_buf_end, const char* fmt, va_list args)
{
    ImGuiContext& g = *GImGui;
    if (fmt[0] == '%' && fmt[1] == 's' && fmt[2] == 0)
    {
        const char* buf = va_arg(args, const char*); // Skip formatting when using "%s"
        if (buf == NULL)
            buf = "(null)";
        *out_buf = buf;
        if (out_buf_end) { *out_buf_end = buf + strlen(buf); }
    }
    else if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '*' && fmt[3] == 's' && fmt[4] == 0)
    {
        int buf_len = va_arg(args, int); // Skip formatting when using "%.*s"
        const char* buf = va_arg(args, const char*);
        if (buf == NULL)
        {
            buf = "(null)";
            buf_len = ImMin(buf_len, 6);
        }
        *out_buf = buf;
        *out_buf_end = buf + buf_len; // Disallow not passing 'out_buf_end' here. User is expected to use it.
    }
    else
    {
        int buf_len = ImFormatStringV(g.TempBuffer.Data, g.TempBuffer.Size, fmt, args);
        *out_buf = g.TempBuffer.Data;
        if (out_buf_end) { *out_buf_end = g.TempBuffer.Data + buf_len; }
    }
}

// CRC32 needs a 1KB lookup table (not cache friendly)
// Although the code to generate the table is simple and shorter than the table itself, using a const table allows us to easily:
// - avoid an unnecessary branch/memory tap, - keep the ImHashXXX functions usable by static constructors, - make it thread-safe.
static const ImU32 GCrc32LookupTable[256] =
{
    0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
    0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
    0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
    0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,
    0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
    0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
    0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
    0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
    0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
    0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
    0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
    0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
    0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
    0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
    0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
    0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D,
};

// Known size hash
// It is ok to call ImHashData on a string with known length but the ### operator won't be supported.
// FIXME-OPT: Replace with e.g. FNV1a hash? CRC32 pretty much randomly access 1KB. Need to do proper measurements.
ImGuiID ImHashData(const void* data_p, size_t data_size, ImGuiID seed)
{
    ImU32 crc = ~seed;
    const unsigned char* data = (const unsigned char*)data_p;
    const ImU32* crc32_lut = GCrc32LookupTable;
    while (data_size-- != 0)
        crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ *data++];
    return ~crc;
}

// Zero-terminated string hash, with support for ### to reset back to seed value
// We support a syntax of "label###id" where only "###id" is included in the hash, and only "label" gets displayed.
// Because this syntax is rarely used we are optimizing for the common case.
// - If we reach ### in the string we discard the hash so far and reset to the seed.
// - We don't do 'current += 2; continue;' after handling ### to keep the code smaller/faster (measured ~10% diff in Debug build)
// FIXME-OPT: Replace with e.g. FNV1a hash? CRC32 pretty much randomly access 1KB. Need to do proper measurements.
ImGuiID ImHashStr(const char* data_p, size_t data_size, ImGuiID seed)
{
    seed = ~seed;
    ImU32 crc = seed;
    const unsigned char* data = (const unsigned char*)data_p;
    const ImU32* crc32_lut = GCrc32LookupTable;
    if (data_size != 0)
    {
        while (data_size-- != 0)
        {
            unsigned char c = *data++;
            if (c == '#' && data_size >= 2 && data[0] == '#' && data[1] == '#')
                crc = seed;
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        }
    }
    else
    {
        while (unsigned char c = *data++)
        {
            if (c == '#' && data[0] == '#' && data[1] == '#')
                crc = seed;
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        }
    }
    return ~crc;
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (File functions)
//-----------------------------------------------------------------------------

// Default file functions
#ifndef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

ImFileHandle ImFileOpen(const char* filename, const char* mode)
{
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && !defined(__CYGWIN__) && !defined(__GNUC__)
    // We need a fopen() wrapper because MSVC/Windows fopen doesn't handle UTF-8 filenames.
    // Previously we used ImTextCountCharsFromUtf8/ImTextStrFromUtf8 here but we now need to support ImWchar16 and ImWchar32!
    const int filename_wsize = ::MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
    const int mode_wsize = ::MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0);

    // Use stack buffer if possible, otherwise heap buffer. Sizes include zero terminator.
    // We don't rely on current ImGuiContext as this is implied to be a helper function which doesn't depend on it (see #7314).
    wchar_t local_temp_stack[FILENAME_MAX];
    ImVector<wchar_t> local_temp_heap;
    if (filename_wsize + mode_wsize > IM_ARRAYSIZE(local_temp_stack))
        local_temp_heap.resize(filename_wsize + mode_wsize);
    wchar_t* filename_wbuf = local_temp_heap.Data ? local_temp_heap.Data : local_temp_stack;
    wchar_t* mode_wbuf = filename_wbuf + filename_wsize;
    ::MultiByteToWideChar(CP_UTF8, 0, filename, -1, filename_wbuf, filename_wsize);
    ::MultiByteToWideChar(CP_UTF8, 0, mode, -1, mode_wbuf, mode_wsize);
    return ::_wfopen(filename_wbuf, mode_wbuf);
#else
    return fopen(filename, mode);
#endif
}

// We should in theory be using fseeko()/ftello() with off_t and _fseeki64()/_ftelli64() with __int64, waiting for the PR that does that in a very portable pre-C++11 zero-warnings way.
bool    ImFileClose(ImFileHandle f)     { return fclose(f) == 0; }
ImU64   ImFileGetSize(ImFileHandle f)   { long off = 0, sz = 0; return ((off = ftell(f)) != -1 && !fseek(f, 0, SEEK_END) && (sz = ftell(f)) != -1 && !fseek(f, off, SEEK_SET)) ? (ImU64)sz : (ImU64)-1; }
ImU64   ImFileRead(void* data, ImU64 sz, ImU64 count, ImFileHandle f)           { return fread(data, (size_t)sz, (size_t)count, f); }
ImU64   ImFileWrite(const void* data, ImU64 sz, ImU64 count, ImFileHandle f)    { return fwrite(data, (size_t)sz, (size_t)count, f); }
#endif // #ifndef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

// Helper: Load file content into memory
// Memory allocated with IM_ALLOC(), must be freed by user using IM_FREE() == ImGui::MemFree()
// This can't really be used with "rt" because fseek size won't match read size.
void*   ImFileLoadToMemory(const char* filename, const char* mode, size_t* out_file_size, int padding_bytes)
{
    IM_ASSERT(filename && mode);
    if (out_file_size)
        *out_file_size = 0;

    ImFileHandle f;
    if ((f = ImFileOpen(filename, mode)) == NULL)
        return NULL;

    size_t file_size = (size_t)ImFileGetSize(f);
    if (file_size == (size_t)-1)
    {
        ImFileClose(f);
        return NULL;
    }

    void* file_data = IM_ALLOC(file_size + padding_bytes);
    if (file_data == NULL)
    {
        ImFileClose(f);
        return NULL;
    }
    if (ImFileRead(file_data, 1, file_size, f) != file_size)
    {
        ImFileClose(f);
        IM_FREE(file_data);
        return NULL;
    }
    if (padding_bytes > 0)
        memset((void*)(((char*)file_data) + file_size), 0, (size_t)padding_bytes);

    ImFileClose(f);
    if (out_file_size)
        *out_file_size = file_size;

    return file_data;
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (ImText* functions)
//-----------------------------------------------------------------------------

IM_MSVC_RUNTIME_CHECKS_OFF

// Convert UTF-8 to 32-bit character, process single character input.
// A nearly-branchless UTF-8 decoder, based on work of Christopher Wellons (https://github.com/skeeto/branchless-utf8).
// We handle UTF-8 decoding error by skipping forward.
int ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end)
{
    static const char lengths[32] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0 };
    static const int masks[]  = { 0x00, 0x7f, 0x1f, 0x0f, 0x07 };
    static const uint32_t mins[] = { 0x400000, 0, 0x80, 0x800, 0x10000 };
    static const int shiftc[] = { 0, 18, 12, 6, 0 };
    static const int shifte[] = { 0, 6, 4, 2, 0 };
    int len = lengths[*(const unsigned char*)in_text >> 3];
    int wanted = len + (len ? 0 : 1);

    if (in_text_end == NULL)
        in_text_end = in_text + wanted; // Max length, nulls will be taken into account.

    // Copy at most 'len' bytes, stop copying at 0 or past in_text_end. Branch predictor does a good job here,
    // so it is fast even with excessive branching.
    unsigned char s[4];
    s[0] = in_text + 0 < in_text_end ? in_text[0] : 0;
    s[1] = in_text + 1 < in_text_end ? in_text[1] : 0;
    s[2] = in_text + 2 < in_text_end ? in_text[2] : 0;
    s[3] = in_text + 3 < in_text_end ? in_text[3] : 0;

    // Assume a four-byte character and load four bytes. Unused bits are shifted out.
    *out_char  = (uint32_t)(s[0] & masks[len]) << 18;
    *out_char |= (uint32_t)(s[1] & 0x3f) << 12;
    *out_char |= (uint32_t)(s[2] & 0x3f) <<  6;
    *out_char |= (uint32_t)(s[3] & 0x3f) <<  0;
    *out_char >>= shiftc[len];

    // Accumulate the various error conditions.
    int e = 0;
    e  = (*out_char < mins[len]) << 6; // non-canonical encoding
    e |= ((*out_char >> 11) == 0x1b) << 7;  // surrogate half?
    e |= (*out_char > IM_UNICODE_CODEPOINT_MAX) << 8;  // out of range?
    e |= (s[1] & 0xc0) >> 2;
    e |= (s[2] & 0xc0) >> 4;
    e |= (s[3]       ) >> 6;
    e ^= 0x2a; // top two bits of each tail byte correct?
    e >>= shifte[len];

    if (e)
    {
        // No bytes are consumed when *in_text == 0 || in_text == in_text_end.
        // One byte is consumed in case of invalid first byte of in_text.
        // All available bytes (at most `len` bytes) are consumed on incomplete/invalid second to last bytes.
        // Invalid or incomplete input may consume less bytes than wanted, therefore every byte has to be inspected in s.
        wanted = ImMin(wanted, !!s[0] + !!s[1] + !!s[2] + !!s[3]);
        *out_char = IM_UNICODE_CODEPOINT_INVALID;
    }

    return wanted;
}

int ImTextStrFromUtf8(ImWchar* buf, int buf_size, const char* in_text, const char* in_text_end, const char** in_text_remaining)
{
    ImWchar* buf_out = buf;
    ImWchar* buf_end = buf + buf_size;
    while (buf_out < buf_end - 1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        *buf_out++ = (ImWchar)c;
    }
    *buf_out = 0;
    if (in_text_remaining)
        *in_text_remaining = in_text;
    return (int)(buf_out - buf);
}

int ImTextCountCharsFromUtf8(const char* in_text, const char* in_text_end)
{
    int char_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        char_count++;
    }
    return char_count;
}

// Based on stb_to_utf8() from github.com/nothings/stb/
static inline int ImTextCharToUtf8_inline(char* buf, int buf_size, unsigned int c)
{
    if (c < 0x80)
    {
        buf[0] = (char)c;
        return 1;
    }
    if (c < 0x800)
    {
        if (buf_size < 2) return 0;
        buf[0] = (char)(0xc0 + (c >> 6));
        buf[1] = (char)(0x80 + (c & 0x3f));
        return 2;
    }
    if (c < 0x10000)
    {
        if (buf_size < 3) return 0;
        buf[0] = (char)(0xe0 + (c >> 12));
        buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[2] = (char)(0x80 + ((c ) & 0x3f));
        return 3;
    }
    if (c <= 0x10FFFF)
    {
        if (buf_size < 4) return 0;
        buf[0] = (char)(0xf0 + (c >> 18));
        buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
        buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[3] = (char)(0x80 + ((c ) & 0x3f));
        return 4;
    }
    // Invalid code point, the max unicode is 0x10FFFF
    return 0;
}

const char* ImTextCharToUtf8(char out_buf[5], unsigned int c)
{
    int count = ImTextCharToUtf8_inline(out_buf, 5, c);
    out_buf[count] = 0;
    return out_buf;
}

// Not optimal but we very rarely use this function.
int ImTextCountUtf8BytesFromChar(const char* in_text, const char* in_text_end)
{
    unsigned int unused = 0;
    return ImTextCharFromUtf8(&unused, in_text, in_text_end);
}

static inline int ImTextCountUtf8BytesFromChar(unsigned int c)
{
    if (c < 0x80) return 1;
    if (c < 0x800) return 2;
    if (c < 0x10000) return 3;
    if (c <= 0x10FFFF) return 4;
    return 3;
}

int ImTextStrToUtf8(char* out_buf, int out_buf_size, const ImWchar* in_text, const ImWchar* in_text_end)
{
    char* buf_p = out_buf;
    const char* buf_end = out_buf + out_buf_size;
    while (buf_p < buf_end - 1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c = (unsigned int)(*in_text++);
        if (c < 0x80)
            *buf_p++ = (char)c;
        else
            buf_p += ImTextCharToUtf8_inline(buf_p, (int)(buf_end - buf_p - 1), c);
    }
    *buf_p = 0;
    return (int)(buf_p - out_buf);
}

int ImTextCountUtf8BytesFromStr(const ImWchar* in_text, const ImWchar* in_text_end)
{
    int bytes_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c = (unsigned int)(*in_text++);
        if (c < 0x80)
            bytes_count++;
        else
            bytes_count += ImTextCountUtf8BytesFromChar(c);
    }
    return bytes_count;
}

const char* ImTextFindPreviousUtf8Codepoint(const char* in_text_start, const char* in_text_curr)
{
    while (in_text_curr > in_text_start)
    {
        in_text_curr--;
        if ((*in_text_curr & 0xC0) != 0x80)
            return in_text_curr;
    }
    return in_text_start;
}

int ImTextCountLines(const char* in_text, const char* in_text_end)
{
    if (in_text_end == NULL)
        in_text_end = in_text + strlen(in_text); // FIXME-OPT: Not optimal approach, discourage use for now.
    int count = 0;
    while (in_text < in_text_end)
    {
        const char* line_end = (const char*)memchr(in_text, '\n', in_text_end - in_text);
        in_text = line_end ? line_end + 1 : in_text_end;
        count++;
    }
    return count;
}

IM_MSVC_RUNTIME_CHECKS_RESTORE

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (Color functions)
// Note: The Convert functions are early design which are not consistent with other API.
//-----------------------------------------------------------------------------

IMGUI_API ImU32 ImAlphaBlendColors(ImU32 col_a, ImU32 col_b)
{
    float t = ((col_b >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;
    int r = ImLerp((int)(col_a >> IM_COL32_R_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_R_SHIFT) & 0xFF, t);
    int g = ImLerp((int)(col_a >> IM_COL32_G_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_G_SHIFT) & 0xFF, t);
    int b = ImLerp((int)(col_a >> IM_COL32_B_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_B_SHIFT) & 0xFF, t);
    return IM_COL32(r, g, b, 0xFF);
}

ImVec4 ImGui::ColorConvertU32ToFloat4(ImU32 in)
{
    float s = 1.0f / 255.0f;
    return ImVec4(
        ((in >> IM_COL32_R_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_G_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_B_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_A_SHIFT) & 0xFF) * s);
}

ImU32 ImGui::ColorConvertFloat4ToU32(const ImVec4& in)
{
    ImU32 out;
    out  = ((ImU32)IM_F32_TO_INT8_SAT(in.x)) << IM_COL32_R_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.y)) << IM_COL32_G_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.z)) << IM_COL32_B_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.w)) << IM_COL32_A_SHIFT;
    return out;
}

// Convert rgb floats ([0-1],[0-1],[0-1]) to hsv floats ([0-1],[0-1],[0-1]), from Foley & van Dam p592
// Optimized http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
void ImGui::ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v)
{
    float K = 0.f;
    if (g < b)
    {
        ImSwap(g, b);
        K = -1.f;
    }
    if (r < g)
    {
        ImSwap(r, g);
        K = -2.f / 6.f - K;
    }

    const float chroma = r - (g < b ? g : b);
    out_h = ImFabs(K + (g - b) / (6.f * chroma + 1e-20f));
    out_s = chroma / (r + 1e-20f);
    out_v = r;
}

// Convert hsv floats ([0-1],[0-1],[0-1]) to rgb floats ([0-1],[0-1],[0-1]), from Foley & van Dam p593
// also http://en.wikipedia.org/wiki/HSL_and_HSV
void ImGui::ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b)
{
    if (s == 0.0f)
    {
        // gray
        out_r = out_g = out_b = v;
        return;
    }

    h = ImFmod(h, 1.0f) / (60.0f / 360.0f);
    int   i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i)
    {
    case 0: out_r = v; out_g = t; out_b = p; break;
    case 1: out_r = q; out_g = v; out_b = p; break;
    case 2: out_r = p; out_g = v; out_b = t; break;
    case 3: out_r = p; out_g = q; out_b = v; break;
    case 4: out_r = t; out_g = p; out_b = v; break;
    case 5: default: out_r = v; out_g = p; out_b = q; break;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiStorage
// Helper: Key->value storage
//-----------------------------------------------------------------------------

// std::lower_bound but without the bullshit
ImGuiStoragePair* ImLowerBound(ImGuiStoragePair* in_begin, ImGuiStoragePair* in_end, ImGuiID key)
{
    ImGuiStoragePair* in_p = in_begin;
    for (size_t count = (size_t)(in_end - in_p); count > 0; )
    {
        size_t count2 = count >> 1;
        ImGuiStoragePair* mid = in_p + count2;
        if (mid->key < key)
        {
            in_p = ++mid;
            count -= count2 + 1;
        }
        else
        {
            count = count2;
        }
    }
    return in_p;
}

IM_MSVC_RUNTIME_CHECKS_OFF
static int IMGUI_CDECL PairComparerByID(const void* lhs, const void* rhs)
{
    // We can't just do a subtraction because qsort uses signed integers and subtracting our ID doesn't play well with that.
    ImGuiID lhs_v = ((const ImGuiStoragePair*)lhs)->key;
    ImGuiID rhs_v = ((const ImGuiStoragePair*)rhs)->key;
    return (lhs_v > rhs_v ? +1 : lhs_v < rhs_v ? -1 : 0);
}

// For quicker full rebuild of a storage (instead of an incremental one), you may add all your contents and then sort once.
void ImGuiStorage::BuildSortByKey()
{
    ImQsort(Data.Data, (size_t)Data.Size, sizeof(ImGuiStoragePair), PairComparerByID);
}

int ImGuiStorage::GetInt(ImGuiID key, int default_val) const
{
    ImGuiStoragePair* it = ImLowerBound(const_cast<ImGuiStoragePair*>(Data.Data), const_cast<ImGuiStoragePair*>(Data.Data + Data.Size), key);
    if (it == Data.Data + Data.Size || it->key != key)
        return default_val;
    return it->val_i;
}

bool ImGuiStorage::GetBool(ImGuiID key, bool default_val) const
{
    return GetInt(key, default_val ? 1 : 0) != 0;
}

float ImGuiStorage::GetFloat(ImGuiID key, float default_val) const
{
    ImGuiStoragePair* it = ImLowerBound(const_cast<ImGuiStoragePair*>(Data.Data), const_cast<ImGuiStoragePair*>(Data.Data + Data.Size), key);
    if (it == Data.Data + Data.Size || it->key != key)
        return default_val;
    return it->val_f;
}

void* ImGuiStorage::GetVoidPtr(ImGuiID key) const
{
    ImGuiStoragePair* it = ImLowerBound(const_cast<ImGuiStoragePair*>(Data.Data), const_cast<ImGuiStoragePair*>(Data.Data + Data.Size), key);
    if (it == Data.Data + Data.Size || it->key != key)
        return NULL;
    return it->val_p;
}

// References are only valid until a new value is added to the storage. Calling a Set***() function or a Get***Ref() function invalidates the pointer.
int* ImGuiStorage::GetIntRef(ImGuiID key, int default_val)
{
    ImGuiStoragePair* it = ImLowerBound(Data.Data, Data.Data + Data.Size, key);
    if (it == Data.Data + Data.Size || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_i;
}

bool* ImGuiStorage::GetBoolRef(ImGuiID key, bool default_val)
{
    return (bool*)GetIntRef(key, default_val ? 1 : 0);
}

float* ImGuiStorage::GetFloatRef(ImGuiID key, float default_val)
{
    ImGuiStoragePair* it = ImLowerBound(Data.Data, Data.Data + Data.Size, key);
    if (it == Data.Data + Data.Size || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_f;
}

void** ImGuiStorage::GetVoidPtrRef(ImGuiID key, void* default_val)
{
    ImGuiStoragePair* it = ImLowerBound(Data.Data, Data.Data + Data.Size, key);
    if (it == Data.Data + Data.Size || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_p;
}

// FIXME-OPT: Need a way to reuse the result of lower_bound when doing GetInt()/SetInt() - not too bad because it only happens on explicit interaction (maximum one a frame)
void ImGuiStorage::SetInt(ImGuiID key, int val)
{
    ImGuiStoragePair* it = ImLowerBound(Data.Data, Data.Data + Data.Size, key);
    if (it == Data.Data + Data.Size || it->key != key)
        Data.insert(it, ImGuiStoragePair(key, val));
    else
        it->val_i = val;
}

void ImGuiStorage::SetBool(ImGuiID key, bool val)
{
    SetInt(key, val ? 1 : 0);
}

void ImGuiStorage::SetFloat(ImGuiID key, float val)
{
    ImGuiStoragePair* it = ImLowerBound(Data.Data, Data.Data + Data.Size, key);
    if (it == Data.Data + Data.Size || it->key != key)
        Data.insert(it, ImGuiStoragePair(key, val));
    else
        it->val_f = val;
}

void ImGuiStorage::SetVoidPtr(ImGuiID key, void* val)
{
    ImGuiStoragePair* it = ImLowerBound(Data.Data, Data.Data + Data.Size, key);
    if (it == Data.Data + Data.Size || it->key != key)
        Data.insert(it, ImGuiStoragePair(key, val));
    else
        it->val_p = val;
}

void ImGuiStorage::SetAllInt(int v)
{
    for (int i = 0; i < Data.Size; i++)
        Data[i].val_i = v;
}
IM_MSVC_RUNTIME_CHECKS_RESTORE

//-----------------------------------------------------------------------------
// [SECTION] ImGuiTextFilter
//-----------------------------------------------------------------------------

// Helper: Parse and apply text filters. In format "aaaaa[,bbbb][,ccccc]"
ImGuiTextFilter::ImGuiTextFilter(const char* default_filter) //-V1077
{
    InputBuf[0] = 0;
    CountGrep = 0;
    if (default_filter)
    {
        ImStrncpy(InputBuf, default_filter, IM_ARRAYSIZE(InputBuf));
        Build();
    }
}

bool ImGuiTextFilter::Draw(const char* label, float width)
{
    if (width != 0.0f)
        ImGui::SetNextItemWidth(width);
    bool value_changed = ImGui::InputText(label, InputBuf, IM_ARRAYSIZE(InputBuf));
    if (value_changed)
        Build();
    return value_changed;
}

void ImGuiTextFilter::ImGuiTextRange::split(char separator, ImVector<ImGuiTextRange>* out) const
{
    out->resize(0);
    const char* wb = b;
    const char* we = wb;
    while (we < e)
    {
        if (*we == separator)
        {
            out->push_back(ImGuiTextRange(wb, we));
            wb = we + 1;
        }
        we++;
    }
    if (wb != we)
        out->push_back(ImGuiTextRange(wb, we));
}

void ImGuiTextFilter::Build()
{
    Filters.resize(0);
    ImGuiTextRange input_range(InputBuf, InputBuf + strlen(InputBuf));
    input_range.split(',', &Filters);

    CountGrep = 0;
    for (ImGuiTextRange& f : Filters)
    {
        while (f.b < f.e && ImCharIsBlankA(f.b[0]))
            f.b++;
        while (f.e > f.b && ImCharIsBlankA(f.e[-1]))
            f.e--;
        if (f.empty())
            continue;
        if (f.b[0] != '-')
            CountGrep += 1;
    }
}

bool ImGuiTextFilter::PassFilter(const char* text, const char* text_end) const
{
    if (Filters.Size == 0)
        return true;

    if (text == NULL)
        text = text_end = "";

    for (const ImGuiTextRange& f : Filters)
    {
        if (f.b == f.e)
            continue;
        if (f.b[0] == '-')
        {
            // Subtract
            if (ImStristr(text, text_end, f.b + 1, f.e) != NULL)
                return false;
        }
        else
        {
            // Grep
            if (ImStristr(text, text_end, f.b, f.e) != NULL)
                return true;
        }
    }

    // Implicit * grep
    if (CountGrep == 0)
        return true;

    return false;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiTextBuffer, ImGuiTextIndex
//-----------------------------------------------------------------------------

// On some platform vsnprintf() takes va_list by reference and modifies it.
// va_copy is the 'correct' way to copy a va_list but Visual Studio prior to 2013 doesn't have it.
#ifndef va_copy
#if defined(__GNUC__) || defined(__clang__)
#define va_copy(dest, src) __builtin_va_copy(dest, src)
#else
#define va_copy(dest, src) (dest = src)
#endif
#endif

char ImGuiTextBuffer::EmptyString[1] = { 0 };

void ImGuiTextBuffer::append(const char* str, const char* str_end)
{
    int len = str_end ? (int)(str_end - str) : (int)strlen(str);

    // Add zero-terminator the first time
    const int write_off = (Buf.Size != 0) ? Buf.Size : 1;
    const int needed_sz = write_off + len;
    if (write_off + len >= Buf.Capacity)
    {
        int new_capacity = Buf.Capacity * 2;
        Buf.reserve(needed_sz > new_capacity ? needed_sz : new_capacity);
    }

    Buf.resize(needed_sz);
    memcpy(&Buf[write_off - 1], str, (size_t)len);
    Buf[write_off - 1 + len] = 0;
}

void ImGuiTextBuffer::appendf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
}

// Helper: Text buffer for logging/accumulating text
void ImGuiTextBuffer::appendfv(const char* fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    int len = ImFormatStringV(NULL, 0, fmt, args);         // FIXME-OPT: could do a first pass write attempt, likely successful on first pass.
    if (len <= 0)
    {
        va_end(args_copy);
        return;
    }

    // Add zero-terminator the first time
    const int write_off = (Buf.Size != 0) ? Buf.Size : 1;
    const int needed_sz = write_off + len;
    if (write_off + len >= Buf.Capacity)
    {
        int new_capacity = Buf.Capacity * 2;
        Buf.reserve(needed_sz > new_capacity ? needed_sz : new_capacity);
    }

    Buf.resize(needed_sz);
    ImFormatStringV(&Buf[write_off - 1], (size_t)len + 1, fmt, args_copy);
    va_end(args_copy);
}

void ImGuiTextIndex::append(const char* base, int old_size, int new_size)
{
    IM_ASSERT(old_size >= 0 && new_size >= old_size && new_size >= EndOffset);
    if (old_size == new_size)
        return;
    if (EndOffset == 0 || base[EndOffset - 1] == '\n')
        LineOffsets.push_back(EndOffset);
    const char* base_end = base + new_size;
    for (const char* p = base + old_size; (p = (const char*)memchr(p, '\n', base_end - p)) != 0; )
        if (++p < base_end) // Don't push a trailing offset on last \n
            LineOffsets.push_back((int)(intptr_t)(p - base));
    EndOffset = ImMax(EndOffset, new_size);
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiListClipper
//-----------------------------------------------------------------------------

// FIXME-TABLE: This prevents us from using ImGuiListClipper _inside_ a table cell.
// The problem we have is that without a Begin/End scheme for rows using the clipper is ambiguous.
static bool GetSkipItemForListClipping()
{
    ImGuiContext& g = *GImGui;
    return (g.CurrentTable ? g.CurrentTable->HostSkipItems : g.CurrentWindow->SkipItems);
}

static void ImGuiListClipper_SortAndFuseRanges(ImVector<ImGuiListClipperRange>& ranges, int offset = 0)
{
    if (ranges.Size - offset <= 1)
        return;

    // Helper to order ranges and fuse them together if possible (bubble sort is fine as we are only sorting 2-3 entries)
    for (int sort_end = ranges.Size - offset - 1; sort_end > 0; --sort_end)
        for (int i = offset; i < sort_end + offset; ++i)
            if (ranges[i].Min > ranges[i + 1].Min)
                ImSwap(ranges[i], ranges[i + 1]);

    // Now fuse ranges together as much as possible.
    for (int i = 1 + offset; i < ranges.Size; i++)
    {
        IM_ASSERT(!ranges[i].PosToIndexConvert && !ranges[i - 1].PosToIndexConvert);
        if (ranges[i - 1].Max < ranges[i].Min)
            continue;
        ranges[i - 1].Min = ImMin(ranges[i - 1].Min, ranges[i].Min);
        ranges[i - 1].Max = ImMax(ranges[i - 1].Max, ranges[i].Max);
        ranges.erase(ranges.Data + i);
        i--;
    }
}

static void ImGuiListClipper_SeekCursorAndSetupPrevLine(float pos_y, float line_height)
{
    // Set cursor position and a few other things so that SetScrollHereY() and Columns() can work when seeking cursor.
    // FIXME: It is problematic that we have to do that here, because custom/equivalent end-user code would stumble on the same issue.
    // The clipper should probably have a final step to display the last item in a regular manner, maybe with an opt-out flag for data sets which may have costly seek?
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    float off_y = pos_y - window->DC.CursorPos.y;
    window->DC.CursorPos.y = pos_y;
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, pos_y - g.Style.ItemSpacing.y);
    window->DC.CursorPosPrevLine.y = window->DC.CursorPos.y - line_height;  // Setting those fields so that SetScrollHereY() can properly function after the end of our clipper usage.
    window->DC.PrevLineSize.y = (line_height - g.Style.ItemSpacing.y);      // If we end up needing more accurate data (to e.g. use SameLine) we may as well make the clipper have a fourth step to let user process and display the last item in their list.
    if (ImGuiOldColumns* columns = window->DC.CurrentColumns)
        columns->LineMinY = window->DC.CursorPos.y;                         // Setting this so that cell Y position are set properly
    if (ImGuiTable* table = g.CurrentTable)
    {
        if (table->IsInsideRow)
            ImGui::TableEndRow(table);
        table->RowPosY2 = window->DC.CursorPos.y;
        const int row_increase = (int)((off_y / line_height) + 0.5f);
        //table->CurrentRow += row_increase; // Can't do without fixing TableEndRow()
        table->RowBgColorCounter += row_increase;
    }
}

ImGuiListClipper::ImGuiListClipper()
{
    memset(this, 0, sizeof(*this));
}

ImGuiListClipper::~ImGuiListClipper()
{
    End();
}

void ImGuiListClipper::Begin(int items_count, float items_height)
{
    if (Ctx == NULL)
        Ctx = ImGui::GetCurrentContext();

    ImGuiContext& g = *Ctx;
    ImGuiWindow* window = g.CurrentWindow;
    IMGUI_DEBUG_LOG_CLIPPER("Clipper: Begin(%d,%.2f) in '%s'\n", items_count, items_height, window->Name);

    if (ImGuiTable* table = g.CurrentTable)
        if (table->IsInsideRow)
            ImGui::TableEndRow(table);

    StartPosY = window->DC.CursorPos.y;
    ItemsHeight = items_height;
    ItemsCount = items_count;
    DisplayStart = -1;
    DisplayEnd = 0;

    // Acquire temporary buffer
    if (++g.ClipperTempDataStacked > g.ClipperTempData.Size)
        g.ClipperTempData.resize(g.ClipperTempDataStacked, ImGuiListClipperData());
    ImGuiListClipperData* data = &g.ClipperTempData[g.ClipperTempDataStacked - 1];
    data->Reset(this);
    data->LossynessOffset = window->DC.CursorStartPosLossyness.y;
    TempData = data;
    StartSeekOffsetY = data->LossynessOffset;
}

void ImGuiListClipper::End()
{
    if (ImGuiListClipperData* data = (ImGuiListClipperData*)TempData)
    {
        // In theory here we should assert that we are already at the right position, but it seems saner to just seek at the end and not assert/crash the user.
        ImGuiContext& g = *Ctx;
        IMGUI_DEBUG_LOG_CLIPPER("Clipper: End() in '%s'\n", g.CurrentWindow->Name);
        if (ItemsCount >= 0 && ItemsCount < INT_MAX && DisplayStart >= 0)
            SeekCursorForItem(ItemsCount);

        // Restore temporary buffer and fix back pointers which may be invalidated when nesting
        IM_ASSERT(data->ListClipper == this);
        data->StepNo = data->Ranges.Size;
        if (--g.ClipperTempDataStacked > 0)
        {
            data = &g.ClipperTempData[g.ClipperTempDataStacked - 1];
            data->ListClipper->TempData = data;
        }
        TempData = NULL;
    }
    ItemsCount = -1;
}

void ImGuiListClipper::IncludeItemsByIndex(int item_begin, int item_end)
{
    ImGuiListClipperData* data = (ImGuiListClipperData*)TempData;
    IM_ASSERT(DisplayStart < 0); // Only allowed after Begin() and if there has not been a specified range yet.
    IM_ASSERT(item_begin <= item_end);
    if (item_begin < item_end)
        data->Ranges.push_back(ImGuiListClipperRange::FromIndices(item_begin, item_end));
}

// This is already called while stepping.
// The ONLY reason you may want to call this is if you passed INT_MAX to ImGuiListClipper::Begin() because you couldn't step item count beforehand.
void ImGuiListClipper::SeekCursorForItem(int item_n)
{
    // - Perform the add and multiply with double to allow seeking through larger ranges.
    // - StartPosY starts from ItemsFrozen, by adding SeekOffsetY we generally cancel that out (SeekOffsetY == LossynessOffset - ItemsFrozen * ItemsHeight).
    // - The reason we store SeekOffsetY instead of inferring it, is because we want to allow user to perform Seek after the last step, where ImGuiListClipperData is already done.
    float pos_y = (float)((double)StartPosY + StartSeekOffsetY + (double)item_n * ItemsHeight);
    ImGuiListClipper_SeekCursorAndSetupPrevLine(pos_y, ItemsHeight);
}

static bool ImGuiListClipper_StepInternal(ImGuiListClipper* clipper)
{
    ImGuiContext& g = *clipper->Ctx;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiListClipperData* data = (ImGuiListClipperData*)clipper->TempData;
    IM_ASSERT(data != NULL && "Called ImGuiListClipper::Step() too many times, or before ImGuiListClipper::Begin() ?");

    ImGuiTable* table = g.CurrentTable;
    if (table && table->IsInsideRow)
        ImGui::TableEndRow(table);

    // No items
    if (clipper->ItemsCount == 0 || GetSkipItemForListClipping())
        return false;

    // While we are in frozen row state, keep displaying items one by one, unclipped
    // FIXME: Could be stored as a table-agnostic state.
    if (data->StepNo == 0 && table != NULL && !table->IsUnfrozenRows)
    {
        clipper->DisplayStart = data->ItemsFrozen;
        clipper->DisplayEnd = ImMin(data->ItemsFrozen + 1, clipper->ItemsCount);
        if (clipper->DisplayStart < clipper->DisplayEnd)
            data->ItemsFrozen++;
        return true;
    }

    // Step 0: Let you process the first element (regardless of it being visible or not, so we can measure the element height)
    bool calc_clipping = false;
    if (data->StepNo == 0)
    {
        clipper->StartPosY = window->DC.CursorPos.y;
        if (clipper->ItemsHeight <= 0.0f)
        {
            // Submit the first item (or range) so we can measure its height (generally the first range is 0..1)
            data->Ranges.push_front(ImGuiListClipperRange::FromIndices(data->ItemsFrozen, data->ItemsFrozen + 1));
            clipper->DisplayStart = ImMax(data->Ranges[0].Min, data->ItemsFrozen);
            clipper->DisplayEnd = ImMin(data->Ranges[0].Max, clipper->ItemsCount);
            data->StepNo = 1;
            return true;
        }
        calc_clipping = true;   // If on the first step with known item height, calculate clipping.
    }

    // Step 1: Let the clipper infer height from first range
    if (clipper->ItemsHeight <= 0.0f)
    {
        IM_ASSERT(data->StepNo == 1);
        if (table)
            IM_ASSERT(table->RowPosY1 == clipper->StartPosY && table->RowPosY2 == window->DC.CursorPos.y);

        clipper->ItemsHeight = (window->DC.CursorPos.y - clipper->StartPosY) / (float)(clipper->DisplayEnd - clipper->DisplayStart);
        bool affected_by_floating_point_precision = ImIsFloatAboveGuaranteedIntegerPrecision(clipper->StartPosY) || ImIsFloatAboveGuaranteedIntegerPrecision(window->DC.CursorPos.y);
        if (affected_by_floating_point_precision)
            clipper->ItemsHeight = window->DC.PrevLineSize.y + g.Style.ItemSpacing.y; // FIXME: Technically wouldn't allow multi-line entries.
        if (clipper->ItemsHeight == 0.0f && clipper->ItemsCount == INT_MAX) // Accept that no item have been submitted if in indeterminate mode.
            return false;
        IM_ASSERT(clipper->ItemsHeight > 0.0f && "Unable to calculate item height! First item hasn't moved the cursor vertically!");
        calc_clipping = true;   // If item height had to be calculated, calculate clipping afterwards.
    }

    // Step 0 or 1: Calculate the actual ranges of visible elements.
    const int already_submitted = clipper->DisplayEnd;
    if (calc_clipping)
    {
        // Record seek offset, this is so ImGuiListClipper::Seek() can be called after ImGuiListClipperData is done
        clipper->StartSeekOffsetY = (double)data->LossynessOffset - data->ItemsFrozen * (double)clipper->ItemsHeight;

        if (g.LogEnabled)
        {
            // If logging is active, do not perform any clipping
            data->Ranges.push_back(ImGuiListClipperRange::FromIndices(0, clipper->ItemsCount));
        }
        else
        {
            // Add range selected to be included for navigation
            const bool is_nav_request = (g.NavMoveScoringItems && g.NavWindow && g.NavWindow->RootWindowForNav == window->RootWindowForNav);
            if (is_nav_request)
                data->Ranges.push_back(ImGuiListClipperRange::FromPositions(g.NavScoringNoClipRect.Min.y, g.NavScoringNoClipRect.Max.y, 0, 0));
            if (is_nav_request && (g.NavMoveFlags & ImGuiNavMoveFlags_IsTabbing) && g.NavTabbingDir == -1)
                data->Ranges.push_back(ImGuiListClipperRange::FromIndices(clipper->ItemsCount - 1, clipper->ItemsCount));

            // Add focused/active item
            ImRect nav_rect_abs = ImGui::WindowRectRelToAbs(window, window->NavRectRel[0]);
            if (g.NavId != 0 && window->NavLastIds[0] == g.NavId)
                data->Ranges.push_back(ImGuiListClipperRange::FromPositions(nav_rect_abs.Min.y, nav_rect_abs.Max.y, 0, 0));

            // Add visible range
            float min_y = window->ClipRect.Min.y;
            float max_y = window->ClipRect.Max.y;

            // Add box selection range
            ImGuiBoxSelectState* bs = &g.BoxSelectState;
            if (bs->IsActive && bs->Window == window)
            {
                // FIXME: Selectable() use of half-ItemSpacing isn't consistent in matter of layout, as ItemAdd(bb) stray above ItemSize()'s CursorPos.
                // RangeSelect's BoxSelect relies on comparing overlap of previous and current rectangle and is sensitive to that.
                // As a workaround we currently half ItemSpacing worth on each side.
                min_y -= g.Style.ItemSpacing.y;
                max_y += g.Style.ItemSpacing.y;

                // Box-select on 2D area requires different clipping.
                if (bs->UnclipMode)
                    data->Ranges.push_back(ImGuiListClipperRange::FromPositions(bs->UnclipRect.Min.y, bs->UnclipRect.Max.y, 0, 0));
            }

            const int off_min = (is_nav_request && g.NavMoveClipDir == ImGuiDir_Up) ? -1 : 0;
            const int off_max = (is_nav_request && g.NavMoveClipDir == ImGuiDir_Down) ? 1 : 0;
            data->Ranges.push_back(ImGuiListClipperRange::FromPositions(min_y, max_y, off_min, off_max));
        }

        // Convert position ranges to item index ranges
        // - Very important: when a starting position is after our maximum item, we set Min to (ItemsCount - 1). This allows us to handle most forms of wrapping.
        // - Due to how Selectable extra padding they tend to be "unaligned" with exact unit in the item list,
        //   which with the flooring/ceiling tend to lead to 2 items instead of one being submitted.
        for (ImGuiListClipperRange& range : data->Ranges)
            if (range.PosToIndexConvert)
            {
                int m1 = (int)(((double)range.Min - window->DC.CursorPos.y - data->LossynessOffset) / clipper->ItemsHeight);
                int m2 = (int)((((double)range.Max - window->DC.CursorPos.y - data->LossynessOffset) / clipper->ItemsHeight) + 0.999999f);
                range.Min = ImClamp(already_submitted + m1 + range.PosToIndexOffsetMin, already_submitted, clipper->ItemsCount - 1);
                range.Max = ImClamp(already_submitted + m2 + range.PosToIndexOffsetMax, range.Min + 1, clipper->ItemsCount);
                range.PosToIndexConvert = false;
            }
        ImGuiListClipper_SortAndFuseRanges(data->Ranges, data->StepNo);
    }

    // Step 0+ (if item height is given in advance) or 1+: Display the next range in line.
    while (data->StepNo < data->Ranges.Size)
    {
        clipper->DisplayStart = ImMax(data->Ranges[data->StepNo].Min, already_submitted);
        clipper->DisplayEnd = ImMin(data->Ranges[data->StepNo].Max, clipper->ItemsCount);
        if (clipper->DisplayStart > already_submitted) //-V1051
            clipper->SeekCursorForItem(clipper->DisplayStart);
        data->StepNo++;
        if (clipper->DisplayStart == clipper->DisplayEnd && data->StepNo < data->Ranges.Size)
            continue;
        return true;
    }

    // After the last step: Let the clipper validate that we have reached the expected Y position (corresponding to element DisplayEnd),
    // Advance the cursor to the end of the list and then returns 'false' to end the loop.
    if (clipper->ItemsCount < INT_MAX)
        clipper->SeekCursorForItem(clipper->ItemsCount);

    return false;
}

bool ImGuiListClipper::Step()
{
    ImGuiContext& g = *Ctx;
    bool need_items_height = (ItemsHeight <= 0.0f);
    bool ret = ImGuiListClipper_StepInternal(this);
    if (ret && (DisplayStart == DisplayEnd))
        ret = false;
    if (g.CurrentTable && g.CurrentTable->IsUnfrozenRows == false)
        IMGUI_DEBUG_LOG_CLIPPER("Clipper: Step(): inside frozen table row.\n");
    if (need_items_height && ItemsHeight > 0.0f)
        IMGUI_DEBUG_LOG_CLIPPER("Clipper: Step(): computed ItemsHeight: %.2f.\n", ItemsHeight);
    if (ret)
    {
        IMGUI_DEBUG_LOG_CLIPPER("Clipper: Step(): display %d to %d.\n", DisplayStart, DisplayEnd);
    }
    else
    {
        IMGUI_DEBUG_LOG_CLIPPER("Clipper: Step(): End.\n");
        End();
    }
    return ret;
}

//-----------------------------------------------------------------------------
// [SECTION] STYLING
//-----------------------------------------------------------------------------

ImGuiStyle& ImGui::GetStyle()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext() ?");
    return GImGui->Style;
}

ImU32 ImGui::GetColorU32(ImGuiCol idx, float alpha_mul)
{
    ImGuiStyle& style = GImGui->Style;
    ImVec4 c = style.Colors[idx];
    c.w *= style.Alpha * alpha_mul;
    return ColorConvertFloat4ToU32(c);
}

ImU32 ImGui::GetColorU32(const ImVec4& col)
{
    ImGuiStyle& style = GImGui->Style;
    ImVec4 c = col;
    c.w *= style.Alpha;
    return ColorConvertFloat4ToU32(c);
}

const ImVec4& ImGui::GetStyleColorVec4(ImGuiCol idx)
{
    ImGuiStyle& style = GImGui->Style;
    return style.Colors[idx];
}

ImU32 ImGui::GetColorU32(ImU32 col, float alpha_mul)
{
    ImGuiStyle& style = GImGui->Style;
    alpha_mul *= style.Alpha;
    if (alpha_mul >= 1.0f)
        return col;
    ImU32 a = (col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT;
    a = (ImU32)(a * alpha_mul); // We don't need to clamp 0..255 because alpha is in 0..1 range.
    return (col & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

// FIXME: This may incur a round-trip (if the end user got their data from a float4) but eventually we aim to store the in-flight colors as ImU32
void ImGui::PushStyleColor(ImGuiCol idx, ImU32 col)
{
    ImGuiContext& g = *GImGui;
    ImGuiColorMod backup;
    backup.Col = idx;
    backup.BackupValue = g.Style.Colors[idx];
    g.ColorStack.push_back(backup);
    if (g.DebugFlashStyleColorIdx != idx)
        g.Style.Colors[idx] = ColorConvertU32ToFloat4(col);
}

void ImGui::PushStyleColor(ImGuiCol idx, const ImVec4& col)
{
    ImGuiContext& g = *GImGui;
    ImGuiColorMod backup;
    backup.Col = idx;
    backup.BackupValue = g.Style.Colors[idx];
    g.ColorStack.push_back(backup);
    if (g.DebugFlashStyleColorIdx != idx)
        g.Style.Colors[idx] = col;
}

void ImGui::PopStyleColor(int count)
{
    ImGuiContext& g = *GImGui;
    if (g.ColorStack.Size < count)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PopStyleColor() too many times!");
        count = g.ColorStack.Size;
    }
    while (count > 0)
    {
        ImGuiColorMod& backup = g.ColorStack.back();
        g.Style.Colors[backup.Col] = backup.BackupValue;
        g.ColorStack.pop_back();
        count--;
    }
}

static const ImGuiDataVarInfo GStyleVarInfo[] =
{
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, Alpha) },                     // ImGuiStyleVar_Alpha
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, DisabledAlpha) },             // ImGuiStyleVar_DisabledAlpha
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, WindowPadding) },             // ImGuiStyleVar_WindowPadding
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, WindowRounding) },            // ImGuiStyleVar_WindowRounding
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, WindowBorderSize) },          // ImGuiStyleVar_WindowBorderSize
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, WindowMinSize) },             // ImGuiStyleVar_WindowMinSize
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, WindowTitleAlign) },          // ImGuiStyleVar_WindowTitleAlign
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, ChildRounding) },             // ImGuiStyleVar_ChildRounding
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, ChildBorderSize) },           // ImGuiStyleVar_ChildBorderSize
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, PopupRounding) },             // ImGuiStyleVar_PopupRounding
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, PopupBorderSize) },           // ImGuiStyleVar_PopupBorderSize
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, FramePadding) },              // ImGuiStyleVar_FramePadding
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, FrameRounding) },             // ImGuiStyleVar_FrameRounding
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, FrameBorderSize) },           // ImGuiStyleVar_FrameBorderSize
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, ItemSpacing) },               // ImGuiStyleVar_ItemSpacing
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, ItemInnerSpacing) },          // ImGuiStyleVar_ItemInnerSpacing
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, IndentSpacing) },             // ImGuiStyleVar_IndentSpacing
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, CellPadding) },               // ImGuiStyleVar_CellPadding
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, ScrollbarSize) },             // ImGuiStyleVar_ScrollbarSize
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, ScrollbarRounding) },         // ImGuiStyleVar_ScrollbarRounding
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, GrabMinSize) },               // ImGuiStyleVar_GrabMinSize
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, GrabRounding) },              // ImGuiStyleVar_GrabRounding
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, TabRounding) },               // ImGuiStyleVar_TabRounding
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, TabBorderSize) },             // ImGuiStyleVar_TabBorderSize
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, TabBarBorderSize) },          // ImGuiStyleVar_TabBarBorderSize
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, TabBarOverlineSize) },        // ImGuiStyleVar_TabBarOverlineSize
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, TableAngledHeadersAngle)},    // ImGuiStyleVar_TableAngledHeadersAngle
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, TableAngledHeadersTextAlign)},// ImGuiStyleVar_TableAngledHeadersTextAlign
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, ButtonTextAlign) },           // ImGuiStyleVar_ButtonTextAlign
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, SelectableTextAlign) },       // ImGuiStyleVar_SelectableTextAlign
    { ImGuiDataType_Float, 1, (ImU32)offsetof(ImGuiStyle, SeparatorTextBorderSize)},    // ImGuiStyleVar_SeparatorTextBorderSize
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, SeparatorTextAlign) },        // ImGuiStyleVar_SeparatorTextAlign
    { ImGuiDataType_Float, 2, (ImU32)offsetof(ImGuiStyle, SeparatorTextPadding) },      // ImGuiStyleVar_SeparatorTextPadding
};

const ImGuiDataVarInfo* ImGui::GetStyleVarInfo(ImGuiStyleVar idx)
{
    IM_ASSERT(idx >= 0 && idx < ImGuiStyleVar_COUNT);
    IM_STATIC_ASSERT(IM_ARRAYSIZE(GStyleVarInfo) == ImGuiStyleVar_COUNT);
    return &GStyleVarInfo[idx];
}

void ImGui::PushStyleVar(ImGuiStyleVar idx, float val)
{
    ImGuiContext& g = *GImGui;
    const ImGuiDataVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type != ImGuiDataType_Float || var_info->Count != 1)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PushStyleVar() variant with wrong type!");
        return;
    }
    float* pvar = (float*)var_info->GetVarPtr(&g.Style);
    g.StyleVarStack.push_back(ImGuiStyleMod(idx, *pvar));
    *pvar = val;
}

void ImGui::PushStyleVarX(ImGuiStyleVar idx, float val_x)
{
    ImGuiContext& g = *GImGui;
    const ImGuiDataVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type != ImGuiDataType_Float || var_info->Count != 2)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PushStyleVar() variant with wrong type!");
        return;
    }
    ImVec2* pvar = (ImVec2*)var_info->GetVarPtr(&g.Style);
    g.StyleVarStack.push_back(ImGuiStyleMod(idx, *pvar));
    pvar->x = val_x;
}

void ImGui::PushStyleVarY(ImGuiStyleVar idx, float val_y)
{
    ImGuiContext& g = *GImGui;
    const ImGuiDataVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type != ImGuiDataType_Float || var_info->Count != 2)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PushStyleVar() variant with wrong type!");
        return;
    }
    ImVec2* pvar = (ImVec2*)var_info->GetVarPtr(&g.Style);
    g.StyleVarStack.push_back(ImGuiStyleMod(idx, *pvar));
    pvar->y = val_y;
}

void ImGui::PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
{
    ImGuiContext& g = *GImGui;
    const ImGuiDataVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type != ImGuiDataType_Float || var_info->Count != 2)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PushStyleVar() variant with wrong type!");
        return;
    }
    ImVec2* pvar = (ImVec2*)var_info->GetVarPtr(&g.Style);
    g.StyleVarStack.push_back(ImGuiStyleMod(idx, *pvar));
    *pvar = val;
}

void ImGui::PopStyleVar(int count)
{
    ImGuiContext& g = *GImGui;
    if (g.StyleVarStack.Size < count)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PopStyleVar() too many times!");
        count = g.StyleVarStack.Size;
    }
    while (count > 0)
    {
        // We avoid a generic memcpy(data, &backup.Backup.., GDataTypeSize[info->Type] * info->Count), the overhead in Debug is not worth it.
        ImGuiStyleMod& backup = g.StyleVarStack.back();
        const ImGuiDataVarInfo* info = GetStyleVarInfo(backup.VarIdx);
        void* data = info->GetVarPtr(&g.Style);
        if (info->Type == ImGuiDataType_Float && info->Count == 1)      { ((float*)data)[0] = backup.BackupFloat[0]; }
        else if (info->Type == ImGuiDataType_Float && info->Count == 2) { ((float*)data)[0] = backup.BackupFloat[0]; ((float*)data)[1] = backup.BackupFloat[1]; }
        g.StyleVarStack.pop_back();
        count--;
    }
}

const char* ImGui::GetStyleColorName(ImGuiCol idx)
{
    // Create switch-case from enum with regexp: ImGuiCol_{.*}, --> case ImGuiCol_\1: return "\1";
    switch (idx)
    {
    case ImGuiCol_Text: return "Text";
    case ImGuiCol_TextDisabled: return "TextDisabled";
    case ImGuiCol_WindowBg: return "WindowBg";
    case ImGuiCol_ChildBg: return "ChildBg";
    case ImGuiCol_PopupBg: return "PopupBg";
    case ImGuiCol_Border: return "Border";
    case ImGuiCol_BorderShadow: return "BorderShadow";
    case ImGuiCol_FrameBg: return "FrameBg";
    case ImGuiCol_FrameBgHovered: return "FrameBgHovered";
    case ImGuiCol_FrameBgActive: return "FrameBgActive";
    case ImGuiCol_TitleBg: return "TitleBg";
    case ImGuiCol_TitleBgActive: return "TitleBgActive";
    case ImGuiCol_TitleBgCollapsed: return "TitleBgCollapsed";
    case ImGuiCol_MenuBarBg: return "MenuBarBg";
    case ImGuiCol_ScrollbarBg: return "ScrollbarBg";
    case ImGuiCol_ScrollbarGrab: return "ScrollbarGrab";
    case ImGuiCol_ScrollbarGrabHovered: return "ScrollbarGrabHovered";
    case ImGuiCol_ScrollbarGrabActive: return "ScrollbarGrabActive";
    case ImGuiCol_CheckMark: return "CheckMark";
    case ImGuiCol_SliderGrab: return "SliderGrab";
    case ImGuiCol_SliderGrabActive: return "SliderGrabActive";
    case ImGuiCol_Button: return "Button";
    case ImGuiCol_ButtonHovered: return "ButtonHovered";
    case ImGuiCol_ButtonActive: return "ButtonActive";
    case ImGuiCol_Header: return "Header";
    case ImGuiCol_HeaderHovered: return "HeaderHovered";
    case ImGuiCol_HeaderActive: return "HeaderActive";
    case ImGuiCol_Separator: return "Separator";
    case ImGuiCol_SeparatorHovered: return "SeparatorHovered";
    case ImGuiCol_SeparatorActive: return "SeparatorActive";
    case ImGuiCol_ResizeGrip: return "ResizeGrip";
    case ImGuiCol_ResizeGripHovered: return "ResizeGripHovered";
    case ImGuiCol_ResizeGripActive: return "ResizeGripActive";
    case ImGuiCol_TabHovered: return "TabHovered";
    case ImGuiCol_Tab: return "Tab";
    case ImGuiCol_TabSelected: return "TabSelected";
    case ImGuiCol_TabSelectedOverline: return "TabSelectedOverline";
    case ImGuiCol_TabDimmed: return "TabDimmed";
    case ImGuiCol_TabDimmedSelected: return "TabDimmedSelected";
    case ImGuiCol_TabDimmedSelectedOverline: return "TabDimmedSelectedOverline";
    case ImGuiCol_PlotLines: return "PlotLines";
    case ImGuiCol_PlotLinesHovered: return "PlotLinesHovered";
    case ImGuiCol_PlotHistogram: return "PlotHistogram";
    case ImGuiCol_PlotHistogramHovered: return "PlotHistogramHovered";
    case ImGuiCol_TableHeaderBg: return "TableHeaderBg";
    case ImGuiCol_TableBorderStrong: return "TableBorderStrong";
    case ImGuiCol_TableBorderLight: return "TableBorderLight";
    case ImGuiCol_TableRowBg: return "TableRowBg";
    case ImGuiCol_TableRowBgAlt: return "TableRowBgAlt";
    case ImGuiCol_TextLink: return "TextLink";
    case ImGuiCol_TextSelectedBg: return "TextSelectedBg";
    case ImGuiCol_DragDropTarget: return "DragDropTarget";
    case ImGuiCol_NavHighlight: return "NavHighlight";
    case ImGuiCol_NavWindowingHighlight: return "NavWindowingHighlight";
    case ImGuiCol_NavWindowingDimBg: return "NavWindowingDimBg";
    case ImGuiCol_ModalWindowDimBg: return "ModalWindowDimBg";
    }
    IM_ASSERT(0);
    return "Unknown";
}


//-----------------------------------------------------------------------------
// [SECTION] RENDER HELPERS
// Some of those (internal) functions are currently quite a legacy mess - their signature and behavior will change,
// we need a nicer separation between low-level functions and high-level functions relying on the ImGui context.
// Also see imgui_draw.cpp for some more which have been reworked to not rely on ImGui:: context.
//-----------------------------------------------------------------------------

const char* ImGui::FindRenderedTextEnd(const char* text, const char* text_end)
{
    const char* text_display_end = text;
    if (!text_end)
        text_end = (const char*)-1;

    while (text_display_end < text_end && *text_display_end != '\0' && (text_display_end[0] != '#' || text_display_end[1] != '#'))
        text_display_end++;
    return text_display_end;
}

// Internal ImGui functions to render text
// RenderText***() functions calls ImDrawList::AddText() calls ImBitmapFont::RenderText()
void ImGui::RenderText(ImVec2 pos, const char* text, const char* text_end, bool hide_text_after_hash)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // Hide anything after a '##' string
    const char* text_display_end;
    if (hide_text_after_hash)
    {
        text_display_end = FindRenderedTextEnd(text, text_end);
    }
    else
    {
        if (!text_end)
            text_end = text + strlen(text); // FIXME-OPT
        text_display_end = text_end;
    }

    if (text != text_display_end)
    {
        window->DrawList->AddText(g.Font, g.FontSize, pos, GetColorU32(ImGuiCol_Text), text, text_display_end);
        if (g.LogEnabled)
            LogRenderedText(&pos, text, text_display_end);
    }
}

void ImGui::RenderTextWrapped(ImVec2 pos, const char* text, const char* text_end, float wrap_width)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (!text_end)
        text_end = text + strlen(text); // FIXME-OPT

    if (text != text_end)
    {
        window->DrawList->AddText(g.Font, g.FontSize, pos, GetColorU32(ImGuiCol_Text), text, text_end, wrap_width);
        if (g.LogEnabled)
            LogRenderedText(&pos, text, text_end);
    }
}

// Default clip_rect uses (pos_min,pos_max)
// Handle clipping on CPU immediately (vs typically let the GPU clip the triangles that are overlapping the clipping rectangle edges)
// FIXME-OPT: Since we have or calculate text_size we could coarse clip whole block immediately, especally for text above draw_list->DrawList.
// Effectively as this is called from widget doing their own coarse clipping it's not very valuable presently. Next time function will take
// better advantage of the render function taking size into account for coarse clipping.
void ImGui::RenderTextClippedEx(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_display_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect)
{
    // Perform CPU side clipping for single clipped element to avoid using scissor state
    ImVec2 pos = pos_min;
    const ImVec2 text_size = text_size_if_known ? *text_size_if_known : CalcTextSize(text, text_display_end, false, 0.0f);

    const ImVec2* clip_min = clip_rect ? &clip_rect->Min : &pos_min;
    const ImVec2* clip_max = clip_rect ? &clip_rect->Max : &pos_max;
    bool need_clipping = (pos.x + text_size.x >= clip_max->x) || (pos.y + text_size.y >= clip_max->y);
    if (clip_rect) // If we had no explicit clipping rectangle then pos==clip_min
        need_clipping |= (pos.x < clip_min->x) || (pos.y < clip_min->y);

    // Align whole block. We should defer that to the better rendering function when we'll have support for individual line alignment.
    if (align.x > 0.0f) pos.x = ImMax(pos.x, pos.x + (pos_max.x - pos.x - text_size.x) * align.x);
    if (align.y > 0.0f) pos.y = ImMax(pos.y, pos.y + (pos_max.y - pos.y - text_size.y) * align.y);

    // Render
    if (need_clipping)
    {
        ImVec4 fine_clip_rect(clip_min->x, clip_min->y, clip_max->x, clip_max->y);
        draw_list->AddText(NULL, 0.0f, pos, GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, &fine_clip_rect);
    }
    else
    {
        draw_list->AddText(NULL, 0.0f, pos, GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, NULL);
    }
}

void ImGui::RenderTextClipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect)
{
    // Hide anything after a '##' string
    const char* text_display_end = FindRenderedTextEnd(text, text_end);
    const int text_len = (int)(text_display_end - text);
    if (text_len == 0)
        return;

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    RenderTextClippedEx(window->DrawList, pos_min, pos_max, text, text_display_end, text_size_if_known, align, clip_rect);
    if (g.LogEnabled)
        LogRenderedText(&pos_min, text, text_display_end);
}

// Another overly complex function until we reorganize everything into a nice all-in-one helper.
// This is made more complex because we have dissociated the layout rectangle (pos_min..pos_max) which define _where_ the ellipsis is, from actual clipping of text and limit of the ellipsis display.
// This is because in the context of tabs we selectively hide part of the text when the Close Button appears, but we don't want the ellipsis to move.
void ImGui::RenderTextEllipsis(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, float clip_max_x, float ellipsis_max_x, const char* text, const char* text_end_full, const ImVec2* text_size_if_known)
{
    ImGuiContext& g = *GImGui;
    if (text_end_full == NULL)
        text_end_full = FindRenderedTextEnd(text);
    const ImVec2 text_size = text_size_if_known ? *text_size_if_known : CalcTextSize(text, text_end_full, false, 0.0f);

    //draw_list->AddLine(ImVec2(pos_max.x, pos_min.y - 4), ImVec2(pos_max.x, pos_max.y + 4), IM_COL32(0, 0, 255, 255));
    //draw_list->AddLine(ImVec2(ellipsis_max_x, pos_min.y-2), ImVec2(ellipsis_max_x, pos_max.y+2), IM_COL32(0, 255, 0, 255));
    //draw_list->AddLine(ImVec2(clip_max_x, pos_min.y), ImVec2(clip_max_x, pos_max.y), IM_COL32(255, 0, 0, 255));
    // FIXME: We could technically remove (last_glyph->AdvanceX - last_glyph->X1) from text_size.x here and save a few pixels.
    if (text_size.x > pos_max.x - pos_min.x)
    {
        // Hello wo...
        // |       |   |
        // min   max   ellipsis_max
        //          <-> this is generally some padding value

        const ImFont* font = draw_list->_Data->Font;
        const float font_size = draw_list->_Data->FontSize;
        const float font_scale = draw_list->_Data->FontScale;
        const char* text_end_ellipsis = NULL;
        const float ellipsis_width = font->EllipsisWidth * font_scale;

        // We can now claim the space between pos_max.x and ellipsis_max.x
        const float text_avail_width = ImMax((ImMax(pos_max.x, ellipsis_max_x) - ellipsis_width) - pos_min.x, 1.0f);
        float text_size_clipped_x = font->CalcTextSizeA(font_size, text_avail_width, 0.0f, text, text_end_full, &text_end_ellipsis).x;
        if (text == text_end_ellipsis && text_end_ellipsis < text_end_full)
        {
            // Always display at least 1 character if there's no room for character + ellipsis
            text_end_ellipsis = text + ImTextCountUtf8BytesFromChar(text, text_end_full);
            text_size_clipped_x = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text, text_end_ellipsis).x;
        }
        while (text_end_ellipsis > text && ImCharIsBlankA(text_end_ellipsis[-1]))
        {
            // Trim trailing space before ellipsis (FIXME: Supporting non-ascii blanks would be nice, for this we need a function to backtrack in UTF-8 text)
            text_end_ellipsis--;
            text_size_clipped_x -= font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text_end_ellipsis, text_end_ellipsis + 1).x; // Ascii blanks are always 1 byte
        }

        // Render text, render ellipsis
        RenderTextClippedEx(draw_list, pos_min, ImVec2(clip_max_x, pos_max.y), text, text_end_ellipsis, &text_size, ImVec2(0.0f, 0.0f));
        ImVec2 ellipsis_pos = ImTrunc(ImVec2(pos_min.x + text_size_clipped_x, pos_min.y));
        if (ellipsis_pos.x + ellipsis_width <= ellipsis_max_x)
            for (int i = 0; i < font->EllipsisCharCount; i++, ellipsis_pos.x += font->EllipsisCharStep * font_scale)
                font->RenderChar(draw_list, font_size, ellipsis_pos, GetColorU32(ImGuiCol_Text), font->EllipsisChar);
    }
    else
    {
        RenderTextClippedEx(draw_list, pos_min, ImVec2(clip_max_x, pos_max.y), text, text_end_full, &text_size, ImVec2(0.0f, 0.0f));
    }

    if (g.LogEnabled)
        LogRenderedText(&pos_min, text, text_end_full);
}

// Render a rectangle shaped with optional rounding and borders
void ImGui::RenderFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool borders, float rounding)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DrawList->AddRectFilled(p_min, p_max, fill_col, rounding);
    const float border_size = g.Style.FrameBorderSize;
    if (borders && border_size > 0.0f)
    {
        window->DrawList->AddRect(p_min + ImVec2(1, 1), p_max + ImVec2(1, 1), GetColorU32(ImGuiCol_BorderShadow), rounding, 0, border_size);
        window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, 0, border_size);
    }
}

void ImGui::RenderFrameBorder(ImVec2 p_min, ImVec2 p_max, float rounding)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const float border_size = g.Style.FrameBorderSize;
    if (border_size > 0.0f)
    {
        window->DrawList->AddRect(p_min + ImVec2(1, 1), p_max + ImVec2(1, 1), GetColorU32(ImGuiCol_BorderShadow), rounding, 0, border_size);
        window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, 0, border_size);
    }
}

void ImGui::RenderNavHighlight(const ImRect& bb, ImGuiID id, ImGuiNavHighlightFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (id != g.NavId)
        return;
    if (g.NavDisableHighlight && !(flags & ImGuiNavHighlightFlags_AlwaysDraw))
        return;
    if (id == g.LastItemData.ID && (g.LastItemData.InFlags & ImGuiItemFlags_NoNav))
        return;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->DC.NavHideHighlightOneFrame)
        return;

    float rounding = (flags & ImGuiNavHighlightFlags_NoRounding) ? 0.0f : g.Style.FrameRounding;
    ImRect display_rect = bb;
    display_rect.ClipWith(window->ClipRect);
    const float thickness = 2.0f;
    if (flags & ImGuiNavHighlightFlags_Compact)
    {
        window->DrawList->AddRect(display_rect.Min, display_rect.Max, GetColorU32(ImGuiCol_NavHighlight), rounding, 0, thickness);
    }
    else
    {
        const float distance = 3.0f + thickness * 0.5f;
        display_rect.Expand(ImVec2(distance, distance));
        bool fully_visible = window->ClipRect.Contains(display_rect);
        if (!fully_visible)
            window->DrawList->PushClipRect(display_rect.Min, display_rect.Max);
        window->DrawList->AddRect(display_rect.Min, display_rect.Max, GetColorU32(ImGuiCol_NavHighlight), rounding, 0, thickness);
        if (!fully_visible)
            window->DrawList->PopClipRect();
    }
}

void ImGui::RenderMouseCursor(ImVec2 base_pos, float base_scale, ImGuiMouseCursor mouse_cursor, ImU32 col_fill, ImU32 col_border, ImU32 col_shadow)
{
    ImGuiContext& g = *GImGui;
    if (mouse_cursor <= ImGuiMouseCursor_None || mouse_cursor >= ImGuiMouseCursor_COUNT) // We intentionally accept out of bound values.
        mouse_cursor = ImGuiMouseCursor_Arrow;
    ImFontAtlas* font_atlas = g.DrawListSharedData.Font->ContainerAtlas;
    for (ImGuiViewportP* viewport : g.Viewports)
    {
        // We scale cursor with current viewport/monitor, however Windows 10 for its own hardware cursor seems to be using a different scale factor.
        ImVec2 offset, size, uv[4];
        if (!font_atlas->GetMouseCursorTexData(mouse_cursor, &offset, &size, &uv[0], &uv[2]))
            continue;
        const ImVec2 pos = base_pos - offset;
        const float scale = base_scale;
        if (!viewport->GetMainRect().Overlaps(ImRect(pos, pos + ImVec2(size.x + 2, size.y + 2) * scale)))
            continue;
        ImDrawList* draw_list = GetForegroundDrawList(viewport);
        ImTextureID tex_id = font_atlas->TexID;
        draw_list->PushTextureID(tex_id);
        draw_list->AddImage(tex_id, pos + ImVec2(1, 0) * scale, pos + (ImVec2(1, 0) + size) * scale, uv[2], uv[3], col_shadow);
        draw_list->AddImage(tex_id, pos + ImVec2(2, 0) * scale, pos + (ImVec2(2, 0) + size) * scale, uv[2], uv[3], col_shadow);
        draw_list->AddImage(tex_id, pos,                        pos + size * scale,                  uv[2], uv[3], col_border);
        draw_list->AddImage(tex_id, pos,                        pos + size * scale,                  uv[0], uv[1], col_fill);
        draw_list->PopTextureID();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] INITIALIZATION, SHUTDOWN
//-----------------------------------------------------------------------------

// Internal state access - if you want to share Dear ImGui state between modules (e.g. DLL) or allocate it yourself
// Note that we still point to some static data and members (such as GFontAtlas), so the state instance you end up using will point to the static data within its module
ImGuiContext* ImGui::GetCurrentContext()
{
    return GImGui;
}

void ImGui::SetCurrentContext(ImGuiContext* ctx)
{
#ifdef IMGUI_SET_CURRENT_CONTEXT_FUNC
    IMGUI_SET_CURRENT_CONTEXT_FUNC(ctx); // For custom thread-based hackery you may want to have control over this.
#else
    GImGui = ctx;
#endif
}

void ImGui::SetAllocatorFunctions(ImGuiMemAllocFunc alloc_func, ImGuiMemFreeFunc free_func, void* user_data)
{
    GImAllocatorAllocFunc = alloc_func;
    GImAllocatorFreeFunc = free_func;
    GImAllocatorUserData = user_data;
}

// This is provided to facilitate copying allocators from one static/DLL boundary to another (e.g. retrieve default allocator of your executable address space)
void ImGui::GetAllocatorFunctions(ImGuiMemAllocFunc* p_alloc_func, ImGuiMemFreeFunc* p_free_func, void** p_user_data)
{
    *p_alloc_func = GImAllocatorAllocFunc;
    *p_free_func = GImAllocatorFreeFunc;
    *p_user_data = GImAllocatorUserData;
}

ImGuiContext* ImGui::CreateContext(ImFontAtlas* shared_font_atlas)
{
    ImGuiContext* prev_ctx = GetCurrentContext();
    ImGuiContext* ctx = IM_NEW(ImGuiContext)(shared_font_atlas);
    SetCurrentContext(ctx);
    Initialize();
    if (prev_ctx != NULL)
        SetCurrentContext(prev_ctx); // Restore previous context if any, else keep new one.
    return ctx;
}

void ImGui::DestroyContext(ImGuiContext* ctx)
{
    ImGuiContext* prev_ctx = GetCurrentContext();
    if (ctx == NULL) //-V1051
        ctx = prev_ctx;
    SetCurrentContext(ctx);
    Shutdown();
    SetCurrentContext((prev_ctx != ctx) ? prev_ctx : NULL);
    IM_DELETE(ctx);
}

// IMPORTANT: interactive elements requires a fixed ###xxx suffix, it must be same in ALL languages to allow for automation.
static const ImGuiLocEntry GLocalizationEntriesEnUS[] =
{
    { ImGuiLocKey_VersionStr,           "Dear ImGui " IMGUI_VERSION " (" IM_STRINGIFY(IMGUI_VERSION_NUM) ")" },
    { ImGuiLocKey_TableSizeOne,         "Size column to fit###SizeOne"          },
    { ImGuiLocKey_TableSizeAllFit,      "Size all columns to fit###SizeAll"     },
    { ImGuiLocKey_TableSizeAllDefault,  "Size all columns to default###SizeAll" },
    { ImGuiLocKey_TableResetOrder,      "Reset order###ResetOrder"              },
    { ImGuiLocKey_WindowingMainMenuBar, "(Main menu bar)"                       },
    { ImGuiLocKey_WindowingPopup,       "(Popup)"                               },
    { ImGuiLocKey_WindowingUntitled,    "(Untitled)"                            },
    { ImGuiLocKey_OpenLink_s,           "Open '%s'"                             },
    { ImGuiLocKey_CopyLink,             "Copy Link###CopyLink"                  },
};

ImGuiContext::ImGuiContext(ImFontAtlas* shared_font_atlas)
{
    IO.Ctx = this;
    InputTextState.Ctx = this;

    Initialized = false;
    FontAtlasOwnedByContext = shared_font_atlas ? false : true;
    Font = NULL;
    FontSize = FontBaseSize = FontScale = CurrentDpiScale = 0.0f;
    IO.Fonts = shared_font_atlas ? shared_font_atlas : IM_NEW(ImFontAtlas)();
    Time = 0.0f;
    FrameCount = 0;
    FrameCountEnded = FrameCountRendered = -1;
    WithinFrameScope = WithinFrameScopeWithImplicitWindow = WithinEndChild = false;
    GcCompactAll = false;
    TestEngineHookItems = false;
    TestEngine = NULL;
    memset(ContextName, 0, sizeof(ContextName));

    InputEventsNextMouseSource = ImGuiMouseSource_Mouse;
    InputEventsNextEventId = 1;

    WindowsActiveCount = 0;
    CurrentWindow = NULL;
    HoveredWindow = NULL;
    HoveredWindowUnderMovingWindow = NULL;
    HoveredWindowBeforeClear = NULL;
    MovingWindow = NULL;
    WheelingWindow = NULL;
    WheelingWindowStartFrame = WheelingWindowScrolledFrame = -1;
    WheelingWindowReleaseTimer = 0.0f;

    DebugDrawIdConflicts = 0;
    DebugHookIdInfo = 0;
    HoveredId = HoveredIdPreviousFrame = 0;
    HoveredIdPreviousFrameItemCount = 0;
    HoveredIdAllowOverlap = false;
    HoveredIdIsDisabled = false;
    HoveredIdTimer = HoveredIdNotActiveTimer = 0.0f;
    ItemUnclipByLog = false;
    ActiveId = 0;
    ActiveIdIsAlive = 0;
    ActiveIdTimer = 0.0f;
    ActiveIdIsJustActivated = false;
    ActiveIdAllowOverlap = false;
    ActiveIdNoClearOnFocusLoss = false;
    ActiveIdHasBeenPressedBefore = false;
    ActiveIdHasBeenEditedBefore = false;
    ActiveIdHasBeenEditedThisFrame = false;
    ActiveIdFromShortcut = false;
    ActiveIdClickOffset = ImVec2(-1, -1);
    ActiveIdWindow = NULL;
    ActiveIdSource = ImGuiInputSource_None;
    ActiveIdMouseButton = -1;
    ActiveIdPreviousFrame = 0;
    ActiveIdPreviousFrameIsAlive = false;
    ActiveIdPreviousFrameHasBeenEditedBefore = false;
    ActiveIdPreviousFrameWindow = NULL;
    LastActiveId = 0;
    LastActiveIdTimer = 0.0f;

    LastKeyboardKeyPressTime = LastKeyModsChangeTime = LastKeyModsChangeFromNoneTime = -1.0;

    ActiveIdUsingNavDirMask = 0x00;
    ActiveIdUsingAllKeyboardKeys = false;

    CurrentFocusScopeId = 0;
    CurrentItemFlags = ImGuiItemFlags_None;
    DebugShowGroupRects = false;

    NavWindow = NULL;
    NavId = NavFocusScopeId = NavActivateId = NavActivateDownId = NavActivatePressedId = 0;
    NavLayer = ImGuiNavLayer_Main;
    NavNextActivateId = 0;
    NavActivateFlags = NavNextActivateFlags = ImGuiActivateFlags_None;
    NavHighlightActivatedId = 0;
    NavHighlightActivatedTimer = 0.0f;
    NavInputSource = ImGuiInputSource_Keyboard;
    NavLastValidSelectionUserData = ImGuiSelectionUserData_Invalid;
    NavIdIsAlive = false;
    NavMousePosDirty = false;
    NavDisableHighlight = true;
    NavDisableMouseHover = false;

    NavAnyRequest = false;
    NavInitRequest = false;
    NavInitRequestFromMove = false;
    NavMoveSubmitted = false;
    NavMoveScoringItems = false;
    NavMoveForwardToNextFrame = false;
    NavMoveFlags = ImGuiNavMoveFlags_None;
    NavMoveScrollFlags = ImGuiScrollFlags_None;
    NavMoveKeyMods = ImGuiMod_None;
    NavMoveDir = NavMoveDirForDebug = NavMoveClipDir = ImGuiDir_None;
    NavScoringDebugCount = 0;
    NavTabbingDir = 0;
    NavTabbingCounter = 0;

    NavJustMovedFromFocusScopeId = NavJustMovedToId = NavJustMovedToFocusScopeId = 0;
    NavJustMovedToKeyMods = ImGuiMod_None;
    NavJustMovedToIsTabbing = false;
    NavJustMovedToHasSelectionData = false;

    // All platforms use Ctrl+Tab but Ctrl<>Super are swapped on Mac...
    // FIXME: Because this value is stored, it annoyingly interfere with toggling io.ConfigMacOSXBehaviors updating this..
    ConfigNavWindowingKeyNext = IO.ConfigMacOSXBehaviors ? (ImGuiMod_Super | ImGuiKey_Tab) : (ImGuiMod_Ctrl | ImGuiKey_Tab);
    ConfigNavWindowingKeyPrev = IO.ConfigMacOSXBehaviors ? (ImGuiMod_Super | ImGuiMod_Shift | ImGuiKey_Tab) : (ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Tab);
    NavWindowingTarget = NavWindowingTargetAnim = NavWindowingListWindow = NULL;
    NavWindowingTimer = NavWindowingHighlightAlpha = 0.0f;
    NavWindowingToggleLayer = false;
    NavWindowingToggleKey = ImGuiKey_None;

    DimBgRatio = 0.0f;

    DragDropActive = DragDropWithinSource = DragDropWithinTarget = false;
    DragDropSourceFlags = ImGuiDragDropFlags_None;
    DragDropSourceFrameCount = -1;
    DragDropMouseButton = -1;
    DragDropTargetId = 0;
    DragDropAcceptFlags = ImGuiDragDropFlags_None;
    DragDropAcceptIdCurrRectSurface = 0.0f;
    DragDropAcceptIdPrev = DragDropAcceptIdCurr = 0;
    DragDropAcceptFrameCount = -1;
    DragDropHoldJustPressedId = 0;
    memset(DragDropPayloadBufLocal, 0, sizeof(DragDropPayloadBufLocal));

    ClipperTempDataStacked = 0;

    CurrentTable = NULL;
    TablesTempDataStacked = 0;
    CurrentTabBar = NULL;
    CurrentMultiSelect = NULL;
    MultiSelectTempDataStacked = 0;

    HoverItemDelayId = HoverItemDelayIdPreviousFrame = HoverItemUnlockedStationaryId = HoverWindowUnlockedStationaryId = 0;
    HoverItemDelayTimer = HoverItemDelayClearTimer = 0.0f;

    MouseCursor = ImGuiMouseCursor_Arrow;
    MouseStationaryTimer = 0.0f;

    TempInputId = 0;
    memset(&DataTypeZeroValue, 0, sizeof(DataTypeZeroValue));
    BeginMenuDepth = BeginComboDepth = 0;
    ColorEditOptions = ImGuiColorEditFlags_DefaultOptions_;
    ColorEditCurrentID = ColorEditSavedID = 0;
    ColorEditSavedHue = ColorEditSavedSat = 0.0f;
    ColorEditSavedColor = 0;
    WindowResizeRelativeMode = false;
    ScrollbarSeekMode = 0;
    ScrollbarClickDeltaToGrabCenter = 0.0f;
    SliderGrabClickOffset = 0.0f;
    SliderCurrentAccum = 0.0f;
    SliderCurrentAccumDirty = false;
    DragCurrentAccumDirty = false;
    DragCurrentAccum = 0.0f;
    DragSpeedDefaultRatio = 1.0f / 100.0f;
    DisabledAlphaBackup = 0.0f;
    DisabledStackSize = 0;
    LockMarkEdited = 0;
    TooltipOverrideCount = 0;
    TooltipPreviousWindow = NULL;

    PlatformImeData.InputPos = ImVec2(0.0f, 0.0f);
    PlatformImeDataPrev.InputPos = ImVec2(-1.0f, -1.0f); // Different to ensure initial submission

    SettingsLoaded = false;
    SettingsDirtyTimer = 0.0f;
    HookIdNext = 0;

    memset(LocalizationTable, 0, sizeof(LocalizationTable));

    LogEnabled = false;
    LogType = ImGuiLogType_None;
    LogNextPrefix = LogNextSuffix = NULL;
    LogFile = NULL;
    LogLinePosY = FLT_MAX;
    LogLineFirstItem = false;
    LogDepthRef = 0;
    LogDepthToExpand = LogDepthToExpandDefault = 2;

    ErrorCallback = NULL;
    ErrorCallbackUserData = NULL;
    ErrorFirst = true;
    ErrorCountCurrentFrame = 0;
    StackSizesInBeginForCurrentWindow = NULL;

    DebugDrawIdConflictsCount = 0;
    DebugLogFlags = ImGuiDebugLogFlags_EventError | ImGuiDebugLogFlags_OutputToTTY;
    DebugLocateId = 0;
    DebugLogSkippedErrors = 0;
    DebugLogAutoDisableFlags = ImGuiDebugLogFlags_None;
    DebugLogAutoDisableFrames = 0;
    DebugLocateFrames = 0;
    DebugBeginReturnValueCullDepth = -1;
    DebugItemPickerActive = false;
    DebugItemPickerMouseButton = ImGuiMouseButton_Left;
    DebugItemPickerBreakId = 0;
    DebugFlashStyleColorTime = 0.0f;
    DebugFlashStyleColorIdx = ImGuiCol_COUNT;

    // Same as DebugBreakClearData(). Those fields are scattered in their respective subsystem to stay in hot-data locations
    DebugBreakInWindow = 0;
    DebugBreakInTable = 0;
    DebugBreakInLocateId = false;
    DebugBreakKeyChord = ImGuiKey_Pause;
    DebugBreakInShortcutRouting = ImGuiKey_None;

    memset(FramerateSecPerFrame, 0, sizeof(FramerateSecPerFrame));
    FramerateSecPerFrameIdx = FramerateSecPerFrameCount = 0;
    FramerateSecPerFrameAccum = 0.0f;
    WantCaptureMouseNextFrame = WantCaptureKeyboardNextFrame = WantTextInputNextFrame = -1;
    memset(TempKeychordName, 0, sizeof(TempKeychordName));
}

void ImGui::Initialize()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(!g.Initialized && !g.SettingsLoaded);

    // Add .ini handle for ImGuiWindow and ImGuiTable types
    {
        ImGuiSettingsHandler ini_handler;
        ini_handler.TypeName = "Window";
        ini_handler.TypeHash = ImHashStr("Window");
        ini_handler.ClearAllFn = WindowSettingsHandler_ClearAll;
        ini_handler.ReadOpenFn = WindowSettingsHandler_ReadOpen;
        ini_handler.ReadLineFn = WindowSettingsHandler_ReadLine;
        ini_handler.ApplyAllFn = WindowSettingsHandler_ApplyAll;
        ini_handler.WriteAllFn = WindowSettingsHandler_WriteAll;
        AddSettingsHandler(&ini_handler);
    }
    TableSettingsAddSettingsHandler();

    // Setup default localization table
    LocalizeRegisterEntries(GLocalizationEntriesEnUS, IM_ARRAYSIZE(GLocalizationEntriesEnUS));

    // Setup default ImGuiPlatformIO clipboard/IME handlers.
    g.PlatformIO.Platform_GetClipboardTextFn = Platform_GetClipboardTextFn_DefaultImpl;    // Platform dependent default implementations
    g.PlatformIO.Platform_SetClipboardTextFn = Platform_SetClipboardTextFn_DefaultImpl;
    g.PlatformIO.Platform_OpenInShellFn = Platform_OpenInShellFn_DefaultImpl;
    g.PlatformIO.Platform_SetImeDataFn = Platform_SetImeDataFn_DefaultImpl;

    // Create default viewport
    ImGuiViewportP* viewport = IM_NEW(ImGuiViewportP)();
    viewport->ID = IMGUI_VIEWPORT_DEFAULT_ID;
    g.Viewports.push_back(viewport);
    g.TempBuffer.resize(1024 * 3 + 1, 0);

    // Build KeysMayBeCharInput[] lookup table (1 bool per named key)
    for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1))
        if ((key >= ImGuiKey_0 && key <= ImGuiKey_9) || (key >= ImGuiKey_A && key <= ImGuiKey_Z) || (key >= ImGuiKey_Keypad0 && key <= ImGuiKey_Keypad9)
            || key == ImGuiKey_Tab || key == ImGuiKey_Space || key == ImGuiKey_Apostrophe || key == ImGuiKey_Comma || key == ImGuiKey_Minus || key == ImGuiKey_Period
            || key == ImGuiKey_Slash || key == ImGuiKey_Semicolon || key == ImGuiKey_Equal || key == ImGuiKey_LeftBracket || key == ImGuiKey_RightBracket || key == ImGuiKey_GraveAccent
            || key == ImGuiKey_KeypadDecimal || key == ImGuiKey_KeypadDivide || key == ImGuiKey_KeypadMultiply || key == ImGuiKey_KeypadSubtract || key == ImGuiKey_KeypadAdd || key == ImGuiKey_KeypadEqual)
            g.KeysMayBeCharInput.SetBit(key);

#ifdef IMGUI_HAS_DOCK
#endif

    g.Initialized = true;
}

// This function is merely here to free heap allocations.
void ImGui::Shutdown()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT_USER_ERROR(g.IO.BackendPlatformUserData == NULL, "Forgot to shutdown Platform backend?");
    IM_ASSERT_USER_ERROR(g.IO.BackendRendererUserData == NULL, "Forgot to shutdown Renderer backend?");

    // The fonts atlas can be used prior to calling NewFrame(), so we clear it even if g.Initialized is FALSE (which would happen if we never called NewFrame)
    if (g.IO.Fonts && g.FontAtlasOwnedByContext)
    {
        g.IO.Fonts->Locked = false;
        IM_DELETE(g.IO.Fonts);
    }
    g.IO.Fonts = NULL;
    g.DrawListSharedData.TempBuffer.clear();

    // Cleanup of other data are conditional on actually having initialized Dear ImGui.
    if (!g.Initialized)
        return;

    // Save settings (unless we haven't attempted to load them: CreateContext/DestroyContext without a call to NewFrame shouldn't save an empty file)
    if (g.SettingsLoaded && g.IO.IniFilename != NULL)
        SaveIniSettingsToDisk(g.IO.IniFilename);

    CallContextHooks(&g, ImGuiContextHookType_Shutdown);

    // Clear everything else
    g.Windows.clear_delete();
    g.WindowsFocusOrder.clear();
    g.WindowsTempSortBuffer.clear();
    g.CurrentWindow = NULL;
    g.CurrentWindowStack.clear();
    g.WindowsById.Clear();
    g.NavWindow = NULL;
    g.HoveredWindow = g.HoveredWindowUnderMovingWindow = NULL;
    g.ActiveIdWindow = g.ActiveIdPreviousFrameWindow = NULL;
    g.MovingWindow = NULL;

    g.KeysRoutingTable.Clear();

    g.ColorStack.clear();
    g.StyleVarStack.clear();
    g.FontStack.clear();
    g.OpenPopupStack.clear();
    g.BeginPopupStack.clear();
    g.TreeNodeStack.clear();

    g.Viewports.clear_delete();

    g.TabBars.Clear();
    g.CurrentTabBarStack.clear();
    g.ShrinkWidthBuffer.clear();

    g.ClipperTempData.clear_destruct();

    g.Tables.Clear();
    g.TablesTempData.clear_destruct();
    g.DrawChannelsTempMergeBuffer.clear();

    g.MultiSelectStorage.Clear();
    g.MultiSelectTempData.clear_destruct();

    g.ClipboardHandlerData.clear();
    g.MenusIdSubmittedThisFrame.clear();
    g.InputTextState.ClearFreeMemory();
    g.InputTextDeactivatedState.ClearFreeMemory();

    g.SettingsWindows.clear();
    g.SettingsHandlers.clear();

    if (g.LogFile)
    {
#ifndef IMGUI_DISABLE_TTY_FUNCTIONS
        if (g.LogFile != stdout)
#endif
            ImFileClose(g.LogFile);
        g.LogFile = NULL;
    }
    g.LogBuffer.clear();
    g.DebugLogBuf.clear();
    g.DebugLogIndex.clear();

    g.Initialized = false;
}

// No specific ordering/dependency support, will see as needed
ImGuiID ImGui::AddContextHook(ImGuiContext* ctx, const ImGuiContextHook* hook)
{
    ImGuiContext& g = *ctx;
    IM_ASSERT(hook->Callback != NULL && hook->HookId == 0 && hook->Type != ImGuiContextHookType_PendingRemoval_);
    g.Hooks.push_back(*hook);
    g.Hooks.back().HookId = ++g.HookIdNext;
    return g.HookIdNext;
}

// Deferred removal, avoiding issue with changing vector while iterating it
void ImGui::RemoveContextHook(ImGuiContext* ctx, ImGuiID hook_id)
{
    ImGuiContext& g = *ctx;
    IM_ASSERT(hook_id != 0);
    for (ImGuiContextHook& hook : g.Hooks)
        if (hook.HookId == hook_id)
            hook.Type = ImGuiContextHookType_PendingRemoval_;
}

// Call context hooks (used by e.g. test engine)
// We assume a small number of hooks so all stored in same array
void ImGui::CallContextHooks(ImGuiContext* ctx, ImGuiContextHookType hook_type)
{
    ImGuiContext& g = *ctx;
    for (ImGuiContextHook& hook : g.Hooks)
        if (hook.Type == hook_type)
            hook.Callback(&g, &hook);
}


//-----------------------------------------------------------------------------
// [SECTION] MAIN CODE (most of the code! lots of stuff, needs tidying up!)
//-----------------------------------------------------------------------------

// ImGuiWindow is mostly a dumb struct. It merely has a constructor and a few helper methods
ImGuiWindow::ImGuiWindow(ImGuiContext* ctx, const char* name) : DrawListInst(NULL)
{
    memset(this, 0, sizeof(*this));
    Ctx = ctx;
    Name = ImStrdup(name);
    NameBufLen = (int)strlen(name) + 1;
    ID = ImHashStr(name);
    IDStack.push_back(ID);
    MoveId = GetID("#MOVE");
    ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);
    ScrollTargetCenterRatio = ImVec2(0.5f, 0.5f);
    AutoFitFramesX = AutoFitFramesY = -1;
    AutoPosLastDirection = ImGuiDir_None;
    SetWindowPosAllowFlags = SetWindowSizeAllowFlags = SetWindowCollapsedAllowFlags = 0;
    SetWindowPosVal = SetWindowPosPivot = ImVec2(FLT_MAX, FLT_MAX);
    LastFrameActive = -1;
    LastTimeActive = -1.0f;
    FontWindowScale = 1.0f;
    SettingsOffset = -1;
    DrawList = &DrawListInst;
    DrawList->_Data = &Ctx->DrawListSharedData;
    DrawList->_OwnerName = Name;
    NavPreferredScoringPosRel[0] = NavPreferredScoringPosRel[1] = ImVec2(FLT_MAX, FLT_MAX);
}

ImGuiWindow::~ImGuiWindow()
{
    IM_ASSERT(DrawList == &DrawListInst);
    IM_DELETE(Name);
    ColumnsStorage.clear_destruct();
}

static void SetCurrentWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.CurrentWindow = window;
    g.StackSizesInBeginForCurrentWindow = g.CurrentWindow ? &g.CurrentWindowStack.back().StackSizesInBegin : NULL;
    g.CurrentTable = window && window->DC.CurrentTableIdx != -1 ? g.Tables.GetByIndex(window->DC.CurrentTableIdx) : NULL;
    g.CurrentDpiScale = 1.0f; // FIXME-DPI: WIP this is modified in docking
    if (window)
    {
        g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize();
        g.FontScale = g.DrawListSharedData.FontScale = g.FontSize / g.Font->FontSize;
        ImGui::NavUpdateCurrentWindowIsScrollPushableX();
    }
}

void ImGui::GcCompactTransientMiscBuffers()
{
    ImGuiContext& g = *GImGui;
    g.ItemFlagsStack.clear();
    g.GroupStack.clear();
    g.MultiSelectTempDataStacked = 0;
    g.MultiSelectTempData.clear_destruct();
    TableGcCompactSettings();
}

// Free up/compact internal window buffers, we can use this when a window becomes unused.
// Not freed:
// - ImGuiWindow, ImGuiWindowSettings, Name, StateStorage, ColumnsStorage (may hold useful data)
// This should have no noticeable visual effect. When the window reappear however, expect new allocation/buffer growth/copy cost.
void ImGui::GcCompactTransientWindowBuffers(ImGuiWindow* window)
{
    window->MemoryCompacted = true;
    window->MemoryDrawListIdxCapacity = window->DrawList->IdxBuffer.Capacity;
    window->MemoryDrawListVtxCapacity = window->DrawList->VtxBuffer.Capacity;
    window->IDStack.clear();
    window->DrawList->_ClearFreeMemory();
    window->DC.ChildWindows.clear();
    window->DC.ItemWidthStack.clear();
    window->DC.TextWrapPosStack.clear();
}

void ImGui::GcAwakeTransientWindowBuffers(ImGuiWindow* window)
{
    // We stored capacity of the ImDrawList buffer to reduce growth-caused allocation/copy when awakening.
    // The other buffers tends to amortize much faster.
    window->MemoryCompacted = false;
    window->DrawList->IdxBuffer.reserve(window->MemoryDrawListIdxCapacity);
    window->DrawList->VtxBuffer.reserve(window->MemoryDrawListVtxCapacity);
    window->MemoryDrawListIdxCapacity = window->MemoryDrawListVtxCapacity = 0;
}

void ImGui::SetActiveID(ImGuiID id, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;

    // Clear previous active id
    if (g.ActiveId != 0)
    {
        // While most behaved code would make an effort to not steal active id during window move/drag operations,
        // we at least need to be resilient to it. Canceling the move is rather aggressive and users of 'master' branch
        // may prefer the weird ill-defined half working situation ('docking' did assert), so may need to rework that.
        if (g.MovingWindow != NULL && g.ActiveId == g.MovingWindow->MoveId)
        {
            IMGUI_DEBUG_LOG_ACTIVEID("SetActiveID() cancel MovingWindow\n");
            g.MovingWindow = NULL;
        }

        // This could be written in a more general way (e.g associate a hook to ActiveId),
        // but since this is currently quite an exception we'll leave it as is.
        // One common scenario leading to this is: pressing Key ->NavMoveRequestApplyResult() -> ClearActiveId()
        if (g.InputTextState.ID == g.ActiveId)
            InputTextDeactivateHook(g.ActiveId);
    }

    // Set active id
    g.ActiveIdIsJustActivated = (g.ActiveId != id);
    if (g.ActiveIdIsJustActivated)
    {
        IMGUI_DEBUG_LOG_ACTIVEID("SetActiveID() old:0x%08X (window \"%s\") -> new:0x%08X (window \"%s\")\n", g.ActiveId, g.ActiveIdWindow ? g.ActiveIdWindow->Name : "", id, window ? window->Name : "");
        g.ActiveIdTimer = 0.0f;
        g.ActiveIdHasBeenPressedBefore = false;
        g.ActiveIdHasBeenEditedBefore = false;
        g.ActiveIdMouseButton = -1;
        if (id != 0)
        {
            g.LastActiveId = id;
            g.LastActiveIdTimer = 0.0f;
        }
    }
    g.ActiveId = id;
    g.ActiveIdAllowOverlap = false;
    g.ActiveIdNoClearOnFocusLoss = false;
    g.ActiveIdWindow = window;
    g.ActiveIdHasBeenEditedThisFrame = false;
    g.ActiveIdFromShortcut = false;
    if (id)
    {
        g.ActiveIdIsAlive = id;
        g.ActiveIdSource = (g.NavActivateId == id || g.NavJustMovedToId == id) ? g.NavInputSource : ImGuiInputSource_Mouse;
        IM_ASSERT(g.ActiveIdSource != ImGuiInputSource_None);
    }

    // Clear declaration of inputs claimed by the widget
    // (Please note that this is WIP and not all keys/inputs are thoroughly declared by all widgets yet)
    g.ActiveIdUsingNavDirMask = 0x00;
    g.ActiveIdUsingAllKeyboardKeys = false;
}

void ImGui::ClearActiveID()
{
    SetActiveID(0, NULL); // g.ActiveId = 0;
}

void ImGui::SetHoveredID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    g.HoveredId = id;
    g.HoveredIdAllowOverlap = false;
    if (id != 0 && g.HoveredIdPreviousFrame != id)
        g.HoveredIdTimer = g.HoveredIdNotActiveTimer = 0.0f;
}

ImGuiID ImGui::GetHoveredID()
{
    ImGuiContext& g = *GImGui;
    return g.HoveredId ? g.HoveredId : g.HoveredIdPreviousFrame;
}

void ImGui::MarkItemEdited(ImGuiID id)
{
    // This marking is solely to be able to provide info for IsItemDeactivatedAfterEdit().
    // ActiveId might have been released by the time we call this (as in the typical press/release button behavior) but still need to fill the data.
    ImGuiContext& g = *GImGui;
    if (g.LockMarkEdited > 0)
        return;
    if (g.ActiveId == id || g.ActiveId == 0)
    {
        g.ActiveIdHasBeenEditedThisFrame = true;
        g.ActiveIdHasBeenEditedBefore = true;
    }

    // We accept a MarkItemEdited() on drag and drop targets (see https://github.com/ocornut/imgui/issues/1875#issuecomment-978243343)
    // We accept 'ActiveIdPreviousFrame == id' for InputText() returning an edit after it has been taken ActiveId away (#4714)
    IM_ASSERT(g.DragDropActive || g.ActiveId == id || g.ActiveId == 0 || g.ActiveIdPreviousFrame == id || (g.CurrentMultiSelect != NULL && g.BoxSelectState.IsActive));

    //IM_ASSERT(g.CurrentWindow->DC.LastItemId == id);
    g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_Edited;
}

bool ImGui::IsWindowContentHoverable(ImGuiWindow* window, ImGuiHoveredFlags flags)
{
    // An active popup disable hovering on other windows (apart from its own children)
    // FIXME-OPT: This could be cached/stored within the window.
    ImGuiContext& g = *GImGui;
    if (g.NavWindow)
        if (ImGuiWindow* focused_root_window = g.NavWindow->RootWindow)
            if (focused_root_window->WasActive && focused_root_window != window->RootWindow)
            {
                // For the purpose of those flags we differentiate "standard popup" from "modal popup"
                // NB: The 'else' is important because Modal windows are also Popups.
                bool want_inhibit = false;
                if (focused_root_window->Flags & ImGuiWindowFlags_Modal)
                    want_inhibit = true;
                else if ((focused_root_window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                    want_inhibit = true;

                // Inhibit hover unless the window is within the stack of our modal/popup
                if (want_inhibit)
                    if (!IsWindowWithinBeginStackOf(window->RootWindow, focused_root_window))
                        return false;
            }
    return true;
}

static inline float CalcDelayFromHoveredFlags(ImGuiHoveredFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (flags & ImGuiHoveredFlags_DelayNormal)
        return g.Style.HoverDelayNormal;
    if (flags & ImGuiHoveredFlags_DelayShort)
        return g.Style.HoverDelayShort;
    return 0.0f;
}

static ImGuiHoveredFlags ApplyHoverFlagsForTooltip(ImGuiHoveredFlags user_flags, ImGuiHoveredFlags shared_flags)
{
    // Allow instance flags to override shared flags
    if (user_flags & (ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_DelayNormal))
        shared_flags &= ~(ImGuiHoveredFlags_DelayNone | ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_DelayNormal);
    return user_flags | shared_flags;
}

// This is roughly matching the behavior of internal-facing ItemHoverable()
// - we allow hovering to be true when ActiveId==window->MoveID, so that clicking on non-interactive items such as a Text() item still returns true with IsItemHovered()
// - this should work even for non-interactive items that have no ID, so we cannot use LastItemId
bool ImGui::IsItemHovered(ImGuiHoveredFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT_USER_ERROR((flags & ~ImGuiHoveredFlags_AllowedMaskForIsItemHovered) == 0, "Invalid flags for IsItemHovered()!");

    if (g.NavDisableMouseHover && !g.NavDisableHighlight && !(flags & ImGuiHoveredFlags_NoNavOverride))
    {
        if (g.LastItemData.ID == g.NavId && g.NavId != 0) // IsItemFocused()
            return false;
        if ((g.LastItemData.InFlags & ImGuiItemFlags_Disabled) && !(flags & ImGuiHoveredFlags_AllowWhenDisabled))
            return false;

        if (flags & ImGuiHoveredFlags_ForTooltip)
            flags = ApplyHoverFlagsForTooltip(flags, g.Style.HoverFlagsForTooltipNav);
    }
    else
    {
        // Test for bounding box overlap, as updated as ItemAdd()
        ImGuiItemStatusFlags status_flags = g.LastItemData.StatusFlags;
        if (!(status_flags & ImGuiItemStatusFlags_HoveredRect))
            return false;

        if (flags & ImGuiHoveredFlags_ForTooltip)
            flags = ApplyHoverFlagsForTooltip(flags, g.Style.HoverFlagsForTooltipMouse);

        // Done with rectangle culling so we can perform heavier checks now
        // Test if we are hovering the right window (our window could be behind another window)
        // [2021/03/02] Reworked / reverted the revert, finally. Note we want e.g. BeginGroup/ItemAdd/EndGroup to work as well. (#3851)
        // [2017/10/16] Reverted commit 344d48be3 and testing RootWindow instead. I believe it is correct to NOT test for RootWindow but this leaves us unable
        // to use IsItemHovered() after EndChild() itself. Until a solution is found I believe reverting to the test from 2017/09/27 is safe since this was
        // the test that has been running for a long while.
        if (g.HoveredWindow != window && (status_flags & ImGuiItemStatusFlags_HoveredWindow) == 0)
            if ((flags & ImGuiHoveredFlags_AllowWhenOverlappedByWindow) == 0)
                return false;

        // Test if another item is active (e.g. being dragged)
        const ImGuiID id = g.LastItemData.ID;
        if ((flags & ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) == 0)
            if (g.ActiveId != 0 && g.ActiveId != id && !g.ActiveIdAllowOverlap)
                if (g.ActiveId != window->MoveId)
                    return false;

        // Test if interactions on this window are blocked by an active popup or modal.
        // The ImGuiHoveredFlags_AllowWhenBlockedByPopup flag will be tested here.
        if (!IsWindowContentHoverable(window, flags) && !(g.LastItemData.InFlags & ImGuiItemFlags_NoWindowHoverableCheck))
            return false;

        // Test if the item is disabled
        if ((g.LastItemData.InFlags & ImGuiItemFlags_Disabled) && !(flags & ImGuiHoveredFlags_AllowWhenDisabled))
            return false;

        // Special handling for calling after Begin() which represent the title bar or tab.
        // When the window is skipped/collapsed (SkipItems==true) that last item (always ->MoveId submitted by Begin)
        // will never be overwritten so we need to detect the case.
        if (id == window->MoveId && window->WriteAccessed)
            return false;

        // Test if using AllowOverlap and overlapped
        if ((g.LastItemData.InFlags & ImGuiItemFlags_AllowOverlap) && id != 0)
            if ((flags & ImGuiHoveredFlags_AllowWhenOverlappedByItem) == 0)
                if (g.HoveredIdPreviousFrame != g.LastItemData.ID)
                    return false;
    }

    // Handle hover delay
    // (some ideas: https://www.nngroup.com/articles/timing-exposing-content)
    const float delay = CalcDelayFromHoveredFlags(flags);
    if (delay > 0.0f || (flags & ImGuiHoveredFlags_Stationary))
    {
        ImGuiID hover_delay_id = (g.LastItemData.ID != 0) ? g.LastItemData.ID : window->GetIDFromPos(g.LastItemData.Rect.Min);
        if ((flags & ImGuiHoveredFlags_NoSharedDelay) && (g.HoverItemDelayIdPreviousFrame != hover_delay_id))
            g.HoverItemDelayTimer = 0.0f;
        g.HoverItemDelayId = hover_delay_id;

        // When changing hovered item we requires a bit of stationary delay before activating hover timer,
        // but once unlocked on a given item we also moving.
        //if (g.HoverDelayTimer >= delay && (g.HoverDelayTimer - g.IO.DeltaTime < delay || g.MouseStationaryTimer - g.IO.DeltaTime < g.Style.HoverStationaryDelay)) { IMGUI_DEBUG_LOG("HoverDelayTimer = %f/%f, MouseStationaryTimer = %f\n", g.HoverDelayTimer, delay, g.MouseStationaryTimer); }
        if ((flags & ImGuiHoveredFlags_Stationary) != 0 && g.HoverItemUnlockedStationaryId != hover_delay_id)
            return false;

        if (g.HoverItemDelayTimer < delay)
            return false;
    }

    return true;
}

// Internal facing ItemHoverable() used when submitting widgets. Differs slightly from IsItemHovered().
// (this does not rely on LastItemData it can be called from a ButtonBehavior() call not following an ItemAdd() call)
// FIXME-LEGACY: the 'ImGuiItemFlags item_flags' parameter was added on 2023-06-28.
// If you used this in your legacy/custom widgets code:
// - Commonly: if your ItemHoverable() call comes after an ItemAdd() call: pass 'item_flags = g.LastItemData.InFlags'.
// - Rare: otherwise you may pass 'item_flags = 0' (ImGuiItemFlags_None) unless you want to benefit from special behavior handled by ItemHoverable.
bool ImGui::ItemHoverable(const ImRect& bb, ImGuiID id, ImGuiItemFlags item_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // Detect ID conflicts
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    if (id != 0 && g.HoveredIdPreviousFrame == id && (item_flags & ImGuiItemFlags_AllowDuplicateId) == 0)
    {
        g.HoveredIdPreviousFrameItemCount++;
        if (g.DebugDrawIdConflicts == id)
            window->DrawList->AddRect(bb.Min - ImVec2(1,1), bb.Max + ImVec2(1,1), IM_COL32(255, 0, 0, 255), 0.0f, ImDrawFlags_None, 2.0f);
    }
#endif

    if (g.HoveredWindow != window)
        return false;
    if (!IsMouseHoveringRect(bb.Min, bb.Max))
        return false;

    if (g.HoveredId != 0 && g.HoveredId != id && !g.HoveredIdAllowOverlap)
        return false;
    if (g.ActiveId != 0 && g.ActiveId != id && !g.ActiveIdAllowOverlap)
        if (!g.ActiveIdFromShortcut)
            return false;

    // Done with rectangle culling so we can perform heavier checks now.
    if (!(item_flags & ImGuiItemFlags_NoWindowHoverableCheck) && !IsWindowContentHoverable(window, ImGuiHoveredFlags_None))
    {
        g.HoveredIdIsDisabled = true;
        return false;
    }

    // We exceptionally allow this function to be called with id==0 to allow using it for easy high-level
    // hover test in widgets code. We could also decide to split this function is two.
    if (id != 0)
    {
        // Drag source doesn't report as hovered
        if (g.DragDropActive && g.DragDropPayload.SourceId == id && !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoDisableHover))
            return false;

        SetHoveredID(id);

        // AllowOverlap mode (rarely used) requires previous frame HoveredId to be null or to match.
        // This allows using patterns where a later submitted widget overlaps a previous one. Generally perceived as a front-to-back hit-test.
        if (item_flags & ImGuiItemFlags_AllowOverlap)
        {
            g.HoveredIdAllowOverlap = true;
            if (g.HoveredIdPreviousFrame != id)
                return false;
        }

        // Display shortcut (only works with mouse)
        // (ImGuiItemStatusFlags_HasShortcut in LastItemData denotes we want a tooltip)
        if (id == g.LastItemData.ID && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HasShortcut) && g.ActiveId != id)
            if (IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_DelayNormal))
                SetTooltip("%s", GetKeyChordName(g.LastItemData.Shortcut));
    }

    // When disabled we'll return false but still set HoveredId
    if (item_flags & ImGuiItemFlags_Disabled)
    {
        // Release active id if turning disabled
        if (g.ActiveId == id && id != 0)
            ClearActiveID();
        g.HoveredIdIsDisabled = true;
        return false;
    }

#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    if (id != 0)
    {
        // [DEBUG] Item Picker tool!
        // We perform the check here because reaching is path is rare (1~ time a frame),
        // making the cost of this tool near-zero! We could get better call-stack and support picking non-hovered
        // items if we performed the test in ItemAdd(), but that would incur a bigger runtime cost.
        if (g.DebugItemPickerActive && g.HoveredIdPreviousFrame == id)
            GetForegroundDrawList()->AddRect(bb.Min, bb.Max, IM_COL32(255, 255, 0, 255));
        if (g.DebugItemPickerBreakId == id)
            IM_DEBUG_BREAK();
    }
#endif

    if (g.NavDisableMouseHover && (item_flags & ImGuiItemFlags_NoNavDisableMouseHover) == 0)
        return false;

    return true;
}

// FIXME: This is inlined/duplicated in ItemAdd()
// FIXME: The id != 0 path is not used by our codebase, may get rid of it?
bool ImGui::IsClippedEx(const ImRect& bb, ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!bb.Overlaps(window->ClipRect))
        if (id == 0 || (id != g.ActiveId && id != g.ActiveIdPreviousFrame && id != g.NavId && id != g.NavActivateId))
            if (!g.ItemUnclipByLog)
                return true;
    return false;
}

// This is also inlined in ItemAdd()
// Note: if ImGuiItemStatusFlags_HasDisplayRect is set, user needs to set g.LastItemData.DisplayRect.
void ImGui::SetLastItemData(ImGuiID item_id, ImGuiItemFlags in_flags, ImGuiItemStatusFlags item_flags, const ImRect& item_rect)
{
    ImGuiContext& g = *GImGui;
    g.LastItemData.ID = item_id;
    g.LastItemData.InFlags = in_flags;
    g.LastItemData.StatusFlags = item_flags;
    g.LastItemData.Rect = g.LastItemData.NavRect = item_rect;
}

float ImGui::CalcWrapWidthForPos(const ImVec2& pos, float wrap_pos_x)
{
    if (wrap_pos_x < 0.0f)
        return 0.0f;

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (wrap_pos_x == 0.0f)
    {
        // We could decide to setup a default wrapping max point for auto-resizing windows,
        // or have auto-wrap (with unspecified wrapping pos) behave as a ContentSize extending function?
        //if (window->Hidden && (window->Flags & ImGuiWindowFlags_AlwaysAutoResize))
        //    wrap_pos_x = ImMax(window->WorkRect.Min.x + g.FontSize * 10.0f, window->WorkRect.Max.x);
        //else
        wrap_pos_x = window->WorkRect.Max.x;
    }
    else if (wrap_pos_x > 0.0f)
    {
        wrap_pos_x += window->Pos.x - window->Scroll.x; // wrap_pos_x is provided is window local space
    }

    return ImMax(wrap_pos_x - pos.x, 1.0f);
}

// IM_ALLOC() == ImGui::MemAlloc()
void* ImGui::MemAlloc(size_t size)
{
    void* ptr = (*GImAllocatorAllocFunc)(size, GImAllocatorUserData);
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    if (ImGuiContext* ctx = GImGui)
        DebugAllocHook(&ctx->DebugAllocInfo, ctx->FrameCount, ptr, size);
#endif
    return ptr;
}

// IM_FREE() == ImGui::MemFree()
void ImGui::MemFree(void* ptr)
{
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    if (ptr != NULL)
        if (ImGuiContext* ctx = GImGui)
            DebugAllocHook(&ctx->DebugAllocInfo, ctx->FrameCount, ptr, (size_t)-1);
#endif
    return (*GImAllocatorFreeFunc)(ptr, GImAllocatorUserData);
}

// We record the number of allocation in recent frames, as a way to audit/sanitize our guiding principles of "no allocations on idle/repeating frames"
void ImGui::DebugAllocHook(ImGuiDebugAllocInfo* info, int frame_count, void* ptr, size_t size)
{
    ImGuiDebugAllocEntry* entry = &info->LastEntriesBuf[info->LastEntriesIdx];
    IM_UNUSED(ptr);
    if (entry->FrameCount != frame_count)
    {
        info->LastEntriesIdx = (info->LastEntriesIdx + 1) % IM_ARRAYSIZE(info->LastEntriesBuf);
        entry = &info->LastEntriesBuf[info->LastEntriesIdx];
        entry->FrameCount = frame_count;
        entry->AllocCount = entry->FreeCount = 0;
    }
    if (size != (size_t)-1)
    {
        entry->AllocCount++;
        info->TotalAllocCount++;
        //printf("[%05d] MemAlloc(%d) -> 0x%p\n", frame_count, size, ptr);
    }
    else
    {
        entry->FreeCount++;
        info->TotalFreeCount++;
        //printf("[%05d] MemFree(0x%p)\n", frame_count, ptr);
    }
}

const char* ImGui::GetClipboardText()
{
    ImGuiContext& g = *GImGui;
    return g.PlatformIO.Platform_GetClipboardTextFn ? g.PlatformIO.Platform_GetClipboardTextFn(&g) : "";
}

void ImGui::SetClipboardText(const char* text)
{
    ImGuiContext& g = *GImGui;
    if (g.PlatformIO.Platform_SetClipboardTextFn != NULL)
        g.PlatformIO.Platform_SetClipboardTextFn(&g, text);
}

const char* ImGui::GetVersion()
{
    return IMGUI_VERSION;
}

ImGuiIO& ImGui::GetIO()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext() ?");
    return GImGui->IO;
}

ImGuiPlatformIO& ImGui::GetPlatformIO()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext()?");
    return GImGui->PlatformIO;
}

// Pass this to your backend rendering function! Valid after Render() and until the next call to NewFrame()
ImDrawData* ImGui::GetDrawData()
{
    ImGuiContext& g = *GImGui;
    ImGuiViewportP* viewport = g.Viewports[0];
    return viewport->DrawDataP.Valid ? &viewport->DrawDataP : NULL;
}

double ImGui::GetTime()
{
    return GImGui->Time;
}

int ImGui::GetFrameCount()
{
    return GImGui->FrameCount;
}

static ImDrawList* GetViewportBgFgDrawList(ImGuiViewportP* viewport, size_t drawlist_no, const char* drawlist_name)
{
    // Create the draw list on demand, because they are not frequently used for all viewports
    ImGuiContext& g = *GImGui;
    IM_ASSERT(drawlist_no < IM_ARRAYSIZE(viewport->BgFgDrawLists));
    ImDrawList* draw_list = viewport->BgFgDrawLists[drawlist_no];
    if (draw_list == NULL)
    {
        draw_list = IM_NEW(ImDrawList)(&g.DrawListSharedData);
        draw_list->_OwnerName = drawlist_name;
        viewport->BgFgDrawLists[drawlist_no] = draw_list;
    }

    // Our ImDrawList system requires that there is always a command
    if (viewport->BgFgDrawListsLastFrame[drawlist_no] != g.FrameCount)
    {
        draw_list->_ResetForNewFrame();
        draw_list->PushTextureID(g.IO.Fonts->TexID);
        draw_list->PushClipRect(viewport->Pos, viewport->Pos + viewport->Size, false);
        viewport->BgFgDrawListsLastFrame[drawlist_no] = g.FrameCount;
    }
    return draw_list;
}

ImDrawList* ImGui::GetBackgroundDrawList(ImGuiViewport* viewport)
{
    return GetViewportBgFgDrawList((ImGuiViewportP*)viewport, 0, "##Background");
}

ImDrawList* ImGui::GetBackgroundDrawList()
{
    ImGuiContext& g = *GImGui;
    return GetBackgroundDrawList(g.Viewports[0]);
}

ImDrawList* ImGui::GetForegroundDrawList(ImGuiViewport* viewport)
{
    return GetViewportBgFgDrawList((ImGuiViewportP*)viewport, 1, "##Foreground");
}

ImDrawList* ImGui::GetForegroundDrawList()
{
    ImGuiContext& g = *GImGui;
    return GetForegroundDrawList(g.Viewports[0]);
}

ImDrawListSharedData* ImGui::GetDrawListSharedData()
{
    return &GImGui->DrawListSharedData;
}

void ImGui::StartMouseMovingWindow(ImGuiWindow* window)
{
    // Set ActiveId even if the _NoMove flag is set. Without it, dragging away from a window with _NoMove would activate hover on other windows.
    // We _also_ call this when clicking in a window empty space when io.ConfigWindowsMoveFromTitleBarOnly is set, but clear g.MovingWindow afterward.
    // This is because we want ActiveId to be set even when the window is not permitted to move.
    ImGuiContext& g = *GImGui;
    FocusWindow(window);
    SetActiveID(window->MoveId, window);
    g.NavDisableHighlight = true;
    g.ActiveIdClickOffset = g.IO.MouseClickedPos[0] - window->RootWindow->Pos;
    g.ActiveIdNoClearOnFocusLoss = true;
    SetActiveIdUsingAllKeyboardKeys();

    bool can_move_window = true;
    if ((window->Flags & ImGuiWindowFlags_NoMove) || (window->RootWindow->Flags & ImGuiWindowFlags_NoMove))
        can_move_window = false;
    if (can_move_window)
        g.MovingWindow = window;
}

// Handle mouse moving window
// Note: moving window with the navigation keys (Square + d-pad / CTRL+TAB + Arrows) are processed in NavUpdateWindowing()
// FIXME: We don't have strong guarantee that g.MovingWindow stay synched with g.ActiveId == g.MovingWindow->MoveId.
// This is currently enforced by the fact that BeginDragDropSource() is setting all g.ActiveIdUsingXXXX flags to inhibit navigation inputs,
// but if we should more thoroughly test cases where g.ActiveId or g.MovingWindow gets changed and not the other.
void ImGui::UpdateMouseMovingWindowNewFrame()
{
    ImGuiContext& g = *GImGui;
    if (g.MovingWindow != NULL)
    {
        // We actually want to move the root window. g.MovingWindow == window we clicked on (could be a child window).
        // We track it to preserve Focus and so that generally ActiveIdWindow == MovingWindow and ActiveId == MovingWindow->MoveId for consistency.
        KeepAliveID(g.ActiveId);
        IM_ASSERT(g.MovingWindow && g.MovingWindow->RootWindow);
        ImGuiWindow* moving_window = g.MovingWindow->RootWindow;
        if (g.IO.MouseDown[0] && IsMousePosValid(&g.IO.MousePos))
        {
            ImVec2 pos = g.IO.MousePos - g.ActiveIdClickOffset;
            SetWindowPos(moving_window, pos, ImGuiCond_Always);
            FocusWindow(g.MovingWindow);
        }
        else
        {
            g.MovingWindow = NULL;
            ClearActiveID();
        }
    }
    else
    {
        // When clicking/dragging from a window that has the _NoMove flag, we still set the ActiveId in order to prevent hovering others.
        if (g.ActiveIdWindow && g.ActiveIdWindow->MoveId == g.ActiveId)
        {
            KeepAliveID(g.ActiveId);
            if (!g.IO.MouseDown[0])
                ClearActiveID();
        }
    }
}

// Initiate moving window when clicking on empty space or title bar.
// Handle left-click and right-click focus.
void ImGui::UpdateMouseMovingWindowEndFrame()
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId != 0 || g.HoveredId != 0)
        return;

    // Unless we just made a window/popup appear
    if (g.NavWindow && g.NavWindow->Appearing)
        return;

    // Click on empty space to focus window and start moving
    // (after we're done with all our widgets)
    if (g.IO.MouseClicked[0])
    {
        // Handle the edge case of a popup being closed while clicking in its empty space.
        // If we try to focus it, FocusWindow() > ClosePopupsOverWindow() will accidentally close any parent popups because they are not linked together any more.
        ImGuiWindow* root_window = g.HoveredWindow ? g.HoveredWindow->RootWindow : NULL;
        const bool is_closed_popup = root_window && (root_window->Flags & ImGuiWindowFlags_Popup) && !IsPopupOpen(root_window->PopupId, ImGuiPopupFlags_AnyPopupLevel);

        if (root_window != NULL && !is_closed_popup)
        {
            StartMouseMovingWindow(g.HoveredWindow); //-V595

            // Cancel moving if clicked outside of title bar
            if (g.IO.ConfigWindowsMoveFromTitleBarOnly)
                if (!(root_window->Flags & ImGuiWindowFlags_NoTitleBar))
                    if (!root_window->TitleBarRect().Contains(g.IO.MouseClickedPos[0]))
                        g.MovingWindow = NULL;

            // Cancel moving if clicked over an item which was disabled or inhibited by popups (note that we know HoveredId == 0 already)
            if (g.HoveredIdIsDisabled)
                g.MovingWindow = NULL;
        }
        else if (root_window == NULL && g.NavWindow != NULL)
        {
            // Clicking on void disable focus
            FocusWindow(NULL, ImGuiFocusRequestFlags_UnlessBelowModal);
        }
    }

    // With right mouse button we close popups without changing focus based on where the mouse is aimed
    // Instead, focus will be restored to the window under the bottom-most closed popup.
    // (The left mouse button path calls FocusWindow on the hovered window, which will lead NewFrame->ClosePopupsOverWindow to trigger)
    if (g.IO.MouseClicked[1])
    {
        // Find the top-most window between HoveredWindow and the top-most Modal Window.
        // This is where we can trim the popup stack.
        ImGuiWindow* modal = GetTopMostPopupModal();
        bool hovered_window_above_modal = g.HoveredWindow && (modal == NULL || IsWindowAbove(g.HoveredWindow, modal));
        ClosePopupsOverWindow(hovered_window_above_modal ? g.HoveredWindow : modal, true);
    }
}

static bool IsWindowActiveAndVisible(ImGuiWindow* window)
{
    return (window->Active) && (!window->Hidden);
}

// The reason this is exposed in imgui_internal.h is: on touch-based system that don't have hovering, we want to dispatch inputs to the right target (imgui vs imgui+app)
void ImGui::UpdateHoveredWindowAndCaptureFlags()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    // FIXME-DPI: This storage was added on 2021/03/31 for test engine, but if we want to multiply WINDOWS_HOVER_PADDING
    // by DpiScale, we need to make this window-agnostic anyhow, maybe need storing inside ImGuiWindow.
    g.WindowsHoverPadding = ImMax(g.Style.TouchExtraPadding, ImVec2(WINDOWS_HOVER_PADDING, WINDOWS_HOVER_PADDING));

    // Find the window hovered by mouse:
    // - Child windows can extend beyond the limit of their parent so we need to derive HoveredRootWindow from HoveredWindow.
    // - When moving a window we can skip the search, which also conveniently bypasses the fact that window->WindowRectClipped is lagging as this point of the frame.
    // - We also support the moved window toggling the NoInputs flag after moving has started in order to be able to detect windows below it, which is useful for e.g. docking mechanisms.
    bool clear_hovered_windows = false;
    FindHoveredWindowEx(g.IO.MousePos, false, &g.HoveredWindow, &g.HoveredWindowUnderMovingWindow);
    g.HoveredWindowBeforeClear = g.HoveredWindow;

    // Modal windows prevents mouse from hovering behind them.
    ImGuiWindow* modal_window = GetTopMostPopupModal();
    if (modal_window && g.HoveredWindow && !IsWindowWithinBeginStackOf(g.HoveredWindow->RootWindow, modal_window))
        clear_hovered_windows = true;

    // Disabled mouse hovering (we don't currently clear MousePos, we could)
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouse)
        clear_hovered_windows = true;

    // We track click ownership. When clicked outside of a window the click is owned by the application and
    // won't report hovering nor request capture even while dragging over our windows afterward.
    const bool has_open_popup = (g.OpenPopupStack.Size > 0);
    const bool has_open_modal = (modal_window != NULL);
    int mouse_earliest_down = -1;
    bool mouse_any_down = false;
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        if (io.MouseClicked[i])
        {
            io.MouseDownOwned[i] = (g.HoveredWindow != NULL) || has_open_popup;
            io.MouseDownOwnedUnlessPopupClose[i] = (g.HoveredWindow != NULL) || has_open_modal;
        }
        mouse_any_down |= io.MouseDown[i];
        if (io.MouseDown[i] || io.MouseReleased[i]) // Increase release frame for our evaluation of earliest button (#1392)
            if (mouse_earliest_down == -1 || io.MouseClickedTime[i] < io.MouseClickedTime[mouse_earliest_down])
                mouse_earliest_down = i;
    }
    const bool mouse_avail = (mouse_earliest_down == -1) || io.MouseDownOwned[mouse_earliest_down];
    const bool mouse_avail_unless_popup_close = (mouse_earliest_down == -1) || io.MouseDownOwnedUnlessPopupClose[mouse_earliest_down];

    // If mouse was first clicked outside of ImGui bounds we also cancel out hovering.
    // FIXME: For patterns of drag and drop across OS windows, we may need to rework/remove this test (first committed 311c0ca9 on 2015/02)
    const bool mouse_dragging_extern_payload = g.DragDropActive && (g.DragDropSourceFlags & ImGuiDragDropFlags_SourceExtern) != 0;
    if (!mouse_avail && !mouse_dragging_extern_payload)
        clear_hovered_windows = true;

    if (clear_hovered_windows)
        g.HoveredWindow = g.HoveredWindowUnderMovingWindow = NULL;

    // Update io.WantCaptureMouse for the user application (true = dispatch mouse info to Dear ImGui only, false = dispatch mouse to Dear ImGui + underlying app)
    // Update io.WantCaptureMouseAllowPopupClose (experimental) to give a chance for app to react to popup closure with a drag
    if (g.WantCaptureMouseNextFrame != -1)
    {
        io.WantCaptureMouse = io.WantCaptureMouseUnlessPopupClose = (g.WantCaptureMouseNextFrame != 0);
    }
    else
    {
        io.WantCaptureMouse = (mouse_avail && (g.HoveredWindow != NULL || mouse_any_down)) || has_open_popup;
        io.WantCaptureMouseUnlessPopupClose = (mouse_avail_unless_popup_close && (g.HoveredWindow != NULL || mouse_any_down)) || has_open_modal;
    }

    // Update io.WantCaptureKeyboard for the user application (true = dispatch keyboard info to Dear ImGui only, false = dispatch keyboard info to Dear ImGui + underlying app)
    io.WantCaptureKeyboard = false;
    if ((io.ConfigFlags & ImGuiConfigFlags_NoKeyboard) == 0)
    {
        if ((g.ActiveId != 0) || (modal_window != NULL))
            io.WantCaptureKeyboard = true;
        else if (io.NavActive && (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) && io.ConfigNavCaptureKeyboard)
            io.WantCaptureKeyboard = true;
    }
    if (g.WantCaptureKeyboardNextFrame != -1) // Manual override
        io.WantCaptureKeyboard = (g.WantCaptureKeyboardNextFrame != 0);

    // Update io.WantTextInput flag, this is to allow systems without a keyboard (e.g. mobile, hand-held) to show a software keyboard if possible
    io.WantTextInput = (g.WantTextInputNextFrame != -1) ? (g.WantTextInputNextFrame != 0) : false;
}

// Calling SetupDrawListSharedData() is followed by SetCurrentFont() which sets up the remaining data.
static void SetupDrawListSharedData()
{
    ImGuiContext& g = *GImGui;
    ImRect virtual_space(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (ImGuiViewportP* viewport : g.Viewports)
        virtual_space.Add(viewport->GetMainRect());
    g.DrawListSharedData.ClipRectFullscreen = virtual_space.ToVec4();
    g.DrawListSharedData.CurveTessellationTol = g.Style.CurveTessellationTol;
    g.DrawListSharedData.SetCircleTessellationMaxError(g.Style.CircleTessellationMaxError);
    g.DrawListSharedData.InitialFlags = ImDrawListFlags_None;
    if (g.Style.AntiAliasedLines)
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AntiAliasedLines;
    if (g.Style.AntiAliasedLinesUseTex && !(g.IO.Fonts->Flags & ImFontAtlasFlags_NoBakedLines))
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AntiAliasedLinesUseTex;
    if (g.Style.AntiAliasedFill)
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AntiAliasedFill;
    if (g.IO.BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset)
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AllowVtxOffset;
}

void ImGui::NewFrame()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext() ?");
    ImGuiContext& g = *GImGui;

    // Remove pending delete hooks before frame start.
    // This deferred removal avoid issues of removal while iterating the hook vector
    for (int n = g.Hooks.Size - 1; n >= 0; n--)
        if (g.Hooks[n].Type == ImGuiContextHookType_PendingRemoval_)
            g.Hooks.erase(&g.Hooks[n]);

    CallContextHooks(&g, ImGuiContextHookType_NewFramePre);

    // Check and assert for various common IO and Configuration mistakes
    ErrorCheckNewFrameSanityChecks();

    // Load settings on first frame, save settings when modified (after a delay)
    UpdateSettings();

    g.Time += g.IO.DeltaTime;
    g.WithinFrameScope = true;
    g.FrameCount += 1;
    g.TooltipOverrideCount = 0;
    g.WindowsActiveCount = 0;
    g.MenusIdSubmittedThisFrame.resize(0);

    // Calculate frame-rate for the user, as a purely luxurious feature
    g.FramerateSecPerFrameAccum += g.IO.DeltaTime - g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx];
    g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx] = g.IO.DeltaTime;
    g.FramerateSecPerFrameIdx = (g.FramerateSecPerFrameIdx + 1) % IM_ARRAYSIZE(g.FramerateSecPerFrame);
    g.FramerateSecPerFrameCount = ImMin(g.FramerateSecPerFrameCount + 1, IM_ARRAYSIZE(g.FramerateSecPerFrame));
    g.IO.Framerate = (g.FramerateSecPerFrameAccum > 0.0f) ? (1.0f / (g.FramerateSecPerFrameAccum / (float)g.FramerateSecPerFrameCount)) : FLT_MAX;

    // Process input queue (trickle as many events as possible), turn events into writes to IO structure
    g.InputEventsTrail.resize(0);
    UpdateInputEvents(g.IO.ConfigInputTrickleEventQueue);

    // Update viewports (after processing input queue, so io.MouseHoveredViewport is set)
    UpdateViewportsNewFrame();

    // Setup current font and draw list shared data
    g.IO.Fonts->Locked = true;
    SetupDrawListSharedData();
    SetCurrentFont(GetDefaultFont());
    IM_ASSERT(g.Font->IsLoaded());

    // Mark rendering data as invalid to prevent user who may have a handle on it to use it.
    for (ImGuiViewportP* viewport : g.Viewports)
        viewport->DrawDataP.Valid = false;

    // Drag and drop keep the source ID alive so even if the source disappear our state is consistent
    if (g.DragDropActive && g.DragDropPayload.SourceId == g.ActiveId)
        KeepAliveID(g.DragDropPayload.SourceId);

    // [DEBUG]
    if (!g.IO.ConfigDebugHighlightIdConflicts || !g.IO.KeyCtrl) // Count is locked while holding CTRL
        g.DebugDrawIdConflicts = 0;
    if (g.IO.ConfigDebugHighlightIdConflicts && g.HoveredIdPreviousFrameItemCount > 1)
        g.DebugDrawIdConflicts = g.HoveredIdPreviousFrame;

    // Update HoveredId data
    if (!g.HoveredIdPreviousFrame)
        g.HoveredIdTimer = 0.0f;
    if (!g.HoveredIdPreviousFrame || (g.HoveredId && g.ActiveId == g.HoveredId))
        g.HoveredIdNotActiveTimer = 0.0f;
    if (g.HoveredId)
        g.HoveredIdTimer += g.IO.DeltaTime;
    if (g.HoveredId && g.ActiveId != g.HoveredId)
        g.HoveredIdNotActiveTimer += g.IO.DeltaTime;
    g.HoveredIdPreviousFrame = g.HoveredId;
    g.HoveredIdPreviousFrameItemCount = 0;
    g.HoveredId = 0;
    g.HoveredIdAllowOverlap = false;
    g.HoveredIdIsDisabled = false;

    // Clear ActiveID if the item is not alive anymore.
    // In 1.87, the common most call to KeepAliveID() was moved from GetID() to ItemAdd().
    // As a result, custom widget using ButtonBehavior() _without_ ItemAdd() need to call KeepAliveID() themselves.
    if (g.ActiveId != 0 && g.ActiveIdIsAlive != g.ActiveId && g.ActiveIdPreviousFrame == g.ActiveId)
    {
        IMGUI_DEBUG_LOG_ACTIVEID("NewFrame(): ClearActiveID() because it isn't marked alive anymore!\n");
        ClearActiveID();
    }

    // Update ActiveId data (clear reference to active widget if the widget isn't alive anymore)
    if (g.ActiveId)
        g.ActiveIdTimer += g.IO.DeltaTime;
    g.LastActiveIdTimer += g.IO.DeltaTime;
    g.ActiveIdPreviousFrame = g.ActiveId;
    g.ActiveIdPreviousFrameWindow = g.ActiveIdWindow;
    g.ActiveIdPreviousFrameHasBeenEditedBefore = g.ActiveIdHasBeenEditedBefore;
    g.ActiveIdIsAlive = 0;
    g.ActiveIdHasBeenEditedThisFrame = false;
    g.ActiveIdPreviousFrameIsAlive = false;
    g.ActiveIdIsJustActivated = false;
    if (g.TempInputId != 0 && g.ActiveId != g.TempInputId)
        g.TempInputId = 0;
    if (g.ActiveId == 0)
    {
        g.ActiveIdUsingNavDirMask = 0x00;
        g.ActiveIdUsingAllKeyboardKeys = false;
    }

    // Record when we have been stationary as this state is preserved while over same item.
    // FIXME: The way this is expressed means user cannot alter HoverStationaryDelay during the frame to use varying values.
    // To allow this we should store HoverItemMaxStationaryTime+ID and perform the >= check in IsItemHovered() function.
    if (g.HoverItemDelayId != 0 && g.MouseStationaryTimer >= g.Style.HoverStationaryDelay)
        g.HoverItemUnlockedStationaryId = g.HoverItemDelayId;
    else if (g.HoverItemDelayId == 0)
        g.HoverItemUnlockedStationaryId = 0;
    if (g.HoveredWindow != NULL && g.MouseStationaryTimer >= g.Style.HoverStationaryDelay)
        g.HoverWindowUnlockedStationaryId = g.HoveredWindow->ID;
    else if (g.HoveredWindow == NULL)
        g.HoverWindowUnlockedStationaryId = 0;

    // Update hover delay for IsItemHovered() with delays and tooltips
    g.HoverItemDelayIdPreviousFrame = g.HoverItemDelayId;
    if (g.HoverItemDelayId != 0)
    {
        g.HoverItemDelayTimer += g.IO.DeltaTime;
        g.HoverItemDelayClearTimer = 0.0f;
        g.HoverItemDelayId = 0;
    }
    else if (g.HoverItemDelayTimer > 0.0f)
    {
        // This gives a little bit of leeway before clearing the hover timer, allowing mouse to cross gaps
        // We could expose 0.25f as style.HoverClearDelay but I am not sure of the logic yet, this is particularly subtle.
        g.HoverItemDelayClearTimer += g.IO.DeltaTime;
        if (g.HoverItemDelayClearTimer >= ImMax(0.25f, g.IO.DeltaTime * 2.0f)) // ~7 frames at 30 Hz + allow for low framerate
            g.HoverItemDelayTimer = g.HoverItemDelayClearTimer = 0.0f; // May want a decaying timer, in which case need to clamp at max first, based on max of caller last requested timer.
    }

    // Drag and drop
    g.DragDropAcceptIdPrev = g.DragDropAcceptIdCurr;
    g.DragDropAcceptIdCurr = 0;
    g.DragDropAcceptIdCurrRectSurface = FLT_MAX;
    g.DragDropWithinSource = false;
    g.DragDropWithinTarget = false;
    g.DragDropHoldJustPressedId = 0;
    g.TooltipPreviousWindow = NULL;

    // Close popups on focus lost (currently wip/opt-in)
    //if (g.IO.AppFocusLost)
    //    ClosePopupsExceptModals();

    // Update keyboard input state
    UpdateKeyboardInputs();

    //IM_ASSERT(g.IO.KeyCtrl == IsKeyDown(ImGuiKey_LeftCtrl) || IsKeyDown(ImGuiKey_RightCtrl));
    //IM_ASSERT(g.IO.KeyShift == IsKeyDown(ImGuiKey_LeftShift) || IsKeyDown(ImGuiKey_RightShift));
    //IM_ASSERT(g.IO.KeyAlt == IsKeyDown(ImGuiKey_LeftAlt) || IsKeyDown(ImGuiKey_RightAlt));
    //IM_ASSERT(g.IO.KeySuper == IsKeyDown(ImGuiKey_LeftSuper) || IsKeyDown(ImGuiKey_RightSuper));

    // Update gamepad/keyboard navigation
    NavUpdate();

    // Update mouse input state
    UpdateMouseInputs();

    // Mark all windows as not visible and compact unused memory.
    IM_ASSERT(g.WindowsFocusOrder.Size <= g.Windows.Size);
    const float memory_compact_start_time = (g.GcCompactAll || g.IO.ConfigMemoryCompactTimer < 0.0f) ? FLT_MAX : (float)g.Time - g.IO.ConfigMemoryCompactTimer;
    for (ImGuiWindow* window : g.Windows)
    {
        window->WasActive = window->Active;
        window->Active = false;
        window->WriteAccessed = false;
        window->BeginCountPreviousFrame = window->BeginCount;
        window->BeginCount = 0;

        // Garbage collect transient buffers of recently unused windows
        if (!window->WasActive && !window->MemoryCompacted && window->LastTimeActive < memory_compact_start_time)
            GcCompactTransientWindowBuffers(window);
    }

    // Find hovered window
    // (needs to be before UpdateMouseMovingWindowNewFrame so we fill g.HoveredWindowUnderMovingWindow on the mouse release frame)
    // (currently needs to be done after the WasActive=Active loop and FindHoveredWindowEx uses ->Active)
    UpdateHoveredWindowAndCaptureFlags();

    // Handle user moving window with mouse (at the beginning of the frame to avoid input lag or sheering)
    UpdateMouseMovingWindowNewFrame();

    // Background darkening/whitening
    if (GetTopMostPopupModal() != NULL || (g.NavWindowingTarget != NULL && g.NavWindowingHighlightAlpha > 0.0f))
        g.DimBgRatio = ImMin(g.DimBgRatio + g.IO.DeltaTime * 6.0f, 1.0f);
    else
        g.DimBgRatio = ImMax(g.DimBgRatio - g.IO.DeltaTime * 10.0f, 0.0f);

    g.MouseCursor = ImGuiMouseCursor_Arrow;
    g.WantCaptureMouseNextFrame = g.WantCaptureKeyboardNextFrame = g.WantTextInputNextFrame = -1;

    // Platform IME data: reset for the frame
    g.PlatformImeDataPrev = g.PlatformImeData;
    g.PlatformImeData.WantVisible = false;

    // Mouse wheel scrolling, scale
    UpdateMouseWheel();

    // Garbage collect transient buffers of recently unused tables
    for (int i = 0; i < g.TablesLastTimeActive.Size; i++)
        if (g.TablesLastTimeActive[i] >= 0.0f && g.TablesLastTimeActive[i] < memory_compact_start_time)
            TableGcCompactTransientBuffers(g.Tables.GetByIndex(i));
    for (ImGuiTableTempData& table_temp_data : g.TablesTempData)
        if (table_temp_data.LastTimeActive >= 0.0f && table_temp_data.LastTimeActive < memory_compact_start_time)
            TableGcCompactTransientBuffers(&table_temp_data);
    if (g.GcCompactAll)
        GcCompactTransientMiscBuffers();
    g.GcCompactAll = false;

    // Closing the focused window restore focus to the first active root window in descending z-order
    if (g.NavWindow && !g.NavWindow->WasActive)
        FocusTopMostWindowUnderOne(NULL, NULL, NULL, ImGuiFocusRequestFlags_RestoreFocusedChild);

    // No window should be open at the beginning of the frame.
    // But in order to allow the user to call NewFrame() multiple times without calling Render(), we are doing an explicit clear.
    g.CurrentWindowStack.resize(0);
    g.BeginPopupStack.resize(0);
    g.ItemFlagsStack.resize(0);
    g.ItemFlagsStack.push_back(ImGuiItemFlags_AutoClosePopups); // Default flags
    g.CurrentItemFlags = g.ItemFlagsStack.back();
    g.GroupStack.resize(0);

    // [DEBUG] Update debug features
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    UpdateDebugToolItemPicker();
    UpdateDebugToolStackQueries();
    UpdateDebugToolFlashStyleColor();
    if (g.DebugLocateFrames > 0 && --g.DebugLocateFrames == 0)
    {
        g.DebugLocateId = 0;
        g.DebugBreakInLocateId = false;
    }
    if (g.DebugLogAutoDisableFrames > 0 && --g.DebugLogAutoDisableFrames == 0)
    {
        DebugLog("(Debug Log: Auto-disabled some ImGuiDebugLogFlags after 2 frames)\n");
        g.DebugLogFlags &= ~g.DebugLogAutoDisableFlags;
        g.DebugLogAutoDisableFlags = ImGuiDebugLogFlags_None;
    }
#endif

    // Create implicit/fallback window - which we will only render it if the user has added something to it.
    // We don't use "Debug" to avoid colliding with user trying to create a "Debug" window with custom flags.
    // This fallback is particularly important as it prevents ImGui:: calls from crashing.
    g.WithinFrameScopeWithImplicitWindow = true;
    SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    Begin("Debug##Default");
    IM_ASSERT(g.CurrentWindow->IsFallbackWindow == true);

    // Store stack sizes
    g.ErrorCountCurrentFrame = 0;
    ErrorRecoveryStoreState(&g.StackSizesInNewFrame);

    // [DEBUG] When io.ConfigDebugBeginReturnValue is set, we make Begin()/BeginChild() return false at different level of the window-stack,
    // allowing to validate correct Begin/End behavior in user code.
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    if (g.IO.ConfigDebugBeginReturnValueLoop)
        g.DebugBeginReturnValueCullDepth = (g.DebugBeginReturnValueCullDepth == -1) ? 0 : ((g.DebugBeginReturnValueCullDepth + ((g.FrameCount % 4) == 0 ? 1 : 0)) % 10);
    else
        g.DebugBeginReturnValueCullDepth = -1;
#endif

    CallContextHooks(&g, ImGuiContextHookType_NewFramePost);
}

// FIXME: Add a more explicit sort order in the window structure.
static int IMGUI_CDECL ChildWindowComparer(const void* lhs, const void* rhs)
{
    const ImGuiWindow* const a = *(const ImGuiWindow* const *)lhs;
    const ImGuiWindow* const b = *(const ImGuiWindow* const *)rhs;
    if (int d = (a->Flags & ImGuiWindowFlags_Popup) - (b->Flags & ImGuiWindowFlags_Popup))
        return d;
    if (int d = (a->Flags & ImGuiWindowFlags_Tooltip) - (b->Flags & ImGuiWindowFlags_Tooltip))
        return d;
    return (a->BeginOrderWithinParent - b->BeginOrderWithinParent);
}

static void AddWindowToSortBuffer(ImVector<ImGuiWindow*>* out_sorted_windows, ImGuiWindow* window)
{
    out_sorted_windows->push_back(window);
    if (window->Active)
    {
        int count = window->DC.ChildWindows.Size;
        ImQsort(window->DC.ChildWindows.Data, (size_t)count, sizeof(ImGuiWindow*), ChildWindowComparer);
        for (int i = 0; i < count; i++)
        {
            ImGuiWindow* child = window->DC.ChildWindows[i];
            if (child->Active)
                AddWindowToSortBuffer(out_sorted_windows, child);
        }
    }
}

static void AddWindowToDrawData(ImGuiWindow* window, int layer)
{
    ImGuiContext& g = *GImGui;
    ImGuiViewportP* viewport = g.Viewports[0];
    g.IO.MetricsRenderWindows++;
    if (window->DrawList->_Splitter._Count > 1)
        window->DrawList->ChannelsMerge(); // Merge if user forgot to merge back. Also required in Docking branch for ImGuiWindowFlags_DockNodeHost windows.
    ImGui::AddDrawListToDrawDataEx(&viewport->DrawDataP, viewport->DrawDataBuilder.Layers[layer], window->DrawList);
    for (ImGuiWindow* child : window->DC.ChildWindows)
        if (IsWindowActiveAndVisible(child)) // Clipped children may have been marked not active
            AddWindowToDrawData(child, layer);
}

static inline int GetWindowDisplayLayer(ImGuiWindow* window)
{
    return (window->Flags & ImGuiWindowFlags_Tooltip) ? 1 : 0;
}

// Layer is locked for the root window, however child windows may use a different viewport (e.g. extruding menu)
static inline void AddRootWindowToDrawData(ImGuiWindow* window)
{
    AddWindowToDrawData(window, GetWindowDisplayLayer(window));
}

static void FlattenDrawDataIntoSingleLayer(ImDrawDataBuilder* builder)
{
    int n = builder->Layers[0]->Size;
    int full_size = n;
    for (int i = 1; i < IM_ARRAYSIZE(builder->Layers); i++)
        full_size += builder->Layers[i]->Size;
    builder->Layers[0]->resize(full_size);
    for (int layer_n = 1; layer_n < IM_ARRAYSIZE(builder->Layers); layer_n++)
    {
        ImVector<ImDrawList*>* layer = builder->Layers[layer_n];
        if (layer->empty())
            continue;
        memcpy(builder->Layers[0]->Data + n, layer->Data, layer->Size * sizeof(ImDrawList*));
        n += layer->Size;
        layer->resize(0);
    }
}

static void InitViewportDrawData(ImGuiViewportP* viewport)
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawData* draw_data = &viewport->DrawDataP;

    viewport->DrawDataBuilder.Layers[0] = &draw_data->CmdLists;
    viewport->DrawDataBuilder.Layers[1] = &viewport->DrawDataBuilder.LayerData1;
    viewport->DrawDataBuilder.Layers[0]->resize(0);
    viewport->DrawDataBuilder.Layers[1]->resize(0);

    draw_data->Valid = true;
    draw_data->CmdListsCount = 0;
    draw_data->TotalVtxCount = draw_data->TotalIdxCount = 0;
    draw_data->DisplayPos = viewport->Pos;
    draw_data->DisplaySize = viewport->Size;
    draw_data->FramebufferScale = io.DisplayFramebufferScale;
    draw_data->OwnerViewport = viewport;
}

// Push a clipping rectangle for both ImGui logic (hit-testing etc.) and low-level ImDrawList rendering.
// - When using this function it is sane to ensure that float are perfectly rounded to integer values,
//   so that e.g. (int)(max.x-min.x) in user's render produce correct result.
// - If the code here changes, may need to update code of functions like NextColumn() and PushColumnClipRect():
//   some frequently called functions which to modify both channels and clipping simultaneously tend to use the
//   more specialized SetWindowClipRectBeforeSetChannel() to avoid extraneous updates of underlying ImDrawCmds.
// - This is analoguous to PushFont()/PopFont() in the sense that are a mixing a global stack and a window stack,
//   which in the case of ClipRect is not so problematic but tends to be more restrictive for fonts.
void ImGui::PushClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DrawList->PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect);
    window->ClipRect = window->DrawList->_ClipRectStack.back();
}

void ImGui::PopClipRect()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DrawList->PopClipRect();
    window->ClipRect = window->DrawList->_ClipRectStack.back();
}

static void ImGui::RenderDimmedBackgroundBehindWindow(ImGuiWindow* window, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    ImGuiViewportP* viewport = (ImGuiViewportP*)GetMainViewport();
    ImRect viewport_rect = viewport->GetMainRect();

    // Draw behind window by moving the draw command at the FRONT of the draw list
    {
        // We've already called AddWindowToDrawData() which called DrawList->ChannelsMerge() on DockNodeHost windows,
        // and draw list have been trimmed already, hence the explicit recreation of a draw command if missing.
        // FIXME: This is creating complication, might be simpler if we could inject a drawlist in drawdata at a given position and not attempt to manipulate ImDrawCmd order.
        ImDrawList* draw_list = window->RootWindow->DrawList;
        if (draw_list->CmdBuffer.Size == 0)
            draw_list->AddDrawCmd();
        draw_list->PushClipRect(viewport_rect.Min - ImVec2(1, 1), viewport_rect.Max + ImVec2(1, 1), false); // FIXME: Need to stricty ensure ImDrawCmd are not merged (ElemCount==6 checks below will verify that)
        draw_list->AddRectFilled(viewport_rect.Min, viewport_rect.Max, col);
        ImDrawCmd cmd = draw_list->CmdBuffer.back();
        IM_ASSERT(cmd.ElemCount == 6);
        draw_list->CmdBuffer.pop_back();
        draw_list->CmdBuffer.push_front(cmd);
        draw_list->AddDrawCmd(); // We need to create a command as CmdBuffer.back().IdxOffset won't be correct if we append to same command.
        draw_list->PopClipRect();
    }
}

ImGuiWindow* ImGui::FindBottomMostVisibleWindowWithinBeginStack(ImGuiWindow* parent_window)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* bottom_most_visible_window = parent_window;
    for (int i = FindWindowDisplayIndex(parent_window); i >= 0; i--)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Flags & ImGuiWindowFlags_ChildWindow)
            continue;
        if (!IsWindowWithinBeginStackOf(window, parent_window))
            break;
        if (IsWindowActiveAndVisible(window) && GetWindowDisplayLayer(window) <= GetWindowDisplayLayer(parent_window))
            bottom_most_visible_window = window;
    }
    return bottom_most_visible_window;
}

static void ImGui::RenderDimmedBackgrounds()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* modal_window = GetTopMostAndVisiblePopupModal();
    if (g.DimBgRatio <= 0.0f && g.NavWindowingHighlightAlpha <= 0.0f)
        return;
    const bool dim_bg_for_modal = (modal_window != NULL);
    const bool dim_bg_for_window_list = (g.NavWindowingTargetAnim != NULL && g.NavWindowingTargetAnim->Active);
    if (!dim_bg_for_modal && !dim_bg_for_window_list)
        return;

    if (dim_bg_for_modal)
    {
        // Draw dimming behind modal or a begin stack child, whichever comes first in draw order.
        ImGuiWindow* dim_behind_window = FindBottomMostVisibleWindowWithinBeginStack(modal_window);
        RenderDimmedBackgroundBehindWindow(dim_behind_window, GetColorU32(modal_window->DC.ModalDimBgColor, g.DimBgRatio));
    }
    else if (dim_bg_for_window_list)
    {
        // Draw dimming behind CTRL+Tab target window and behind CTRL+Tab UI window
        RenderDimmedBackgroundBehindWindow(g.NavWindowingTargetAnim, GetColorU32(ImGuiCol_NavWindowingDimBg, g.DimBgRatio));

        // Draw border around CTRL+Tab target window
        ImGuiWindow* window = g.NavWindowingTargetAnim;
        ImGuiViewport* viewport = GetMainViewport();
        float distance = g.FontSize;
        ImRect bb = window->Rect();
        bb.Expand(distance);
        if (bb.GetWidth() >= viewport->Size.x && bb.GetHeight() >= viewport->Size.y)
            bb.Expand(-distance - 1.0f); // If a window fits the entire viewport, adjust its highlight inward
        if (window->DrawList->CmdBuffer.Size == 0)
            window->DrawList->AddDrawCmd();
        window->DrawList->PushClipRect(viewport->Pos, viewport->Pos + viewport->Size);
        window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha), window->WindowRounding, 0, 3.0f);
        window->DrawList->PopClipRect();
    }
}

// This is normally called by Render(). You may want to call it directly if you want to avoid calling Render() but the gain will be very minimal.
void ImGui::EndFrame()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);

    // Don't process EndFrame() multiple times.
    if (g.FrameCountEnded == g.FrameCount)
        return;
    IM_ASSERT(g.WithinFrameScope && "Forgot to call ImGui::NewFrame()?");

    CallContextHooks(&g, ImGuiContextHookType_EndFramePre);

    // [EXPERIMENTAL] Recover from errors
    if (g.IO.ConfigErrorRecovery)
        ErrorRecoveryTryToRecoverState(&g.StackSizesInNewFrame);
    ErrorCheckEndFrameSanityChecks();
    ErrorCheckEndFrameFinalizeErrorTooltip();

    // Notify Platform/OS when our Input Method Editor cursor has moved (e.g. CJK inputs using Microsoft IME)
    ImGuiPlatformImeData* ime_data = &g.PlatformImeData;
    if (g.PlatformIO.Platform_SetImeDataFn != NULL && memcmp(ime_data, &g.PlatformImeDataPrev, sizeof(ImGuiPlatformImeData)) != 0)
    {
        IMGUI_DEBUG_LOG_IO("[io] Calling Platform_SetImeDataFn(): WantVisible: %d, InputPos (%.2f,%.2f)\n", ime_data->WantVisible, ime_data->InputPos.x, ime_data->InputPos.y);
        ImGuiViewport* viewport = GetMainViewport();
        g.PlatformIO.Platform_SetImeDataFn(&g, viewport, ime_data);
    }

    // Hide implicit/fallback "Debug" window if it hasn't been used
    g.WithinFrameScopeWithImplicitWindow = false;
    if (g.CurrentWindow && !g.CurrentWindow->WriteAccessed)
        g.CurrentWindow->Active = false;
    End();

    // Update navigation: CTRL+Tab, wrap-around requests
    NavEndFrame();

    // Drag and Drop: Elapse payload (if delivered, or if source stops being submitted)
    if (g.DragDropActive)
    {
        bool is_delivered = g.DragDropPayload.Delivery;
        bool is_elapsed = (g.DragDropSourceFrameCount + 1 < g.FrameCount) && ((g.DragDropSourceFlags & ImGuiDragDropFlags_PayloadAutoExpire) || g.DragDropMouseButton == -1 || !IsMouseDown(g.DragDropMouseButton));
        if (is_delivered || is_elapsed)
            ClearDragDrop();
    }

    // Drag and Drop: Fallback for missing source tooltip. This is not ideal but better than nothing.
    // If you want to handle source item disappearing: instead of submitting your description tooltip
    // in the BeginDragDropSource() block of the dragged item, you can submit them from a safe single spot
    // (e.g. end of your item loop, or before EndFrame) by reading payload data.
    // In the typical case, the contents of drag tooltip should be possible to infer solely from payload data.
    if (g.DragDropActive && g.DragDropSourceFrameCount + 1 < g.FrameCount && !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
    {
        g.DragDropWithinSource = true;
        SetTooltip("...");
        g.DragDropWithinSource = false;
    }

    // End frame
    g.WithinFrameScope = false;
    g.FrameCountEnded = g.FrameCount;

    // Initiate moving window + handle left-click and right-click focus
    UpdateMouseMovingWindowEndFrame();

    // Sort the window list so that all child windows are after their parent
    // We cannot do that on FocusWindow() because children may not exist yet
    g.WindowsTempSortBuffer.resize(0);
    g.WindowsTempSortBuffer.reserve(g.Windows.Size);
    for (ImGuiWindow* window : g.Windows)
    {
        if (window->Active && (window->Flags & ImGuiWindowFlags_ChildWindow))       // if a child is active its parent will add it
            continue;
        AddWindowToSortBuffer(&g.WindowsTempSortBuffer, window);
    }

    // This usually assert if there is a mismatch between the ImGuiWindowFlags_ChildWindow / ParentWindow values and DC.ChildWindows[] in parents, aka we've done something wrong.
    IM_ASSERT(g.Windows.Size == g.WindowsTempSortBuffer.Size);
    g.Windows.swap(g.WindowsTempSortBuffer);
    g.IO.MetricsActiveWindows = g.WindowsActiveCount;

    // Unlock font atlas
    g.IO.Fonts->Locked = false;

    // Clear Input data for next frame
    g.IO.MousePosPrev = g.IO.MousePos;
    g.IO.AppFocusLost = false;
    g.IO.MouseWheel = g.IO.MouseWheelH = 0.0f;
    g.IO.InputQueueCharacters.resize(0);

    CallContextHooks(&g, ImGuiContextHookType_EndFramePost);
}

// Prepare the data for rendering so you can call GetDrawData()
// (As with anything within the ImGui:: namspace this doesn't touch your GPU or graphics API at all:
// it is the role of the ImGui_ImplXXXX_RenderDrawData() function provided by the renderer backend)
void ImGui::Render()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);

    if (g.FrameCountEnded != g.FrameCount)
        EndFrame();
    if (g.FrameCountRendered == g.FrameCount)
        return;
    g.FrameCountRendered = g.FrameCount;

    g.IO.MetricsRenderWindows = 0;
    CallContextHooks(&g, ImGuiContextHookType_RenderPre);

    // Add background ImDrawList (for each active viewport)
    for (ImGuiViewportP* viewport : g.Viewports)
    {
        InitViewportDrawData(viewport);
        if (viewport->BgFgDrawLists[0] != NULL)
            AddDrawListToDrawDataEx(&viewport->DrawDataP, viewport->DrawDataBuilder.Layers[0], GetBackgroundDrawList(viewport));
    }

    // Draw modal/window whitening backgrounds
    RenderDimmedBackgrounds();

    // Add ImDrawList to render
    ImGuiWindow* windows_to_render_top_most[2];
    windows_to_render_top_most[0] = (g.NavWindowingTarget && !(g.NavWindowingTarget->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus)) ? g.NavWindowingTarget->RootWindow : NULL;
    windows_to_render_top_most[1] = (g.NavWindowingTarget ? g.NavWindowingListWindow : NULL);
    for (ImGuiWindow* window : g.Windows)
    {
        IM_MSVC_WARNING_SUPPRESS(6011); // Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
        if (IsWindowActiveAndVisible(window) && (window->Flags & ImGuiWindowFlags_ChildWindow) == 0 && window != windows_to_render_top_most[0] && window != windows_to_render_top_most[1])
            AddRootWindowToDrawData(window);
    }
    for (int n = 0; n < IM_ARRAYSIZE(windows_to_render_top_most); n++)
        if (windows_to_render_top_most[n] && IsWindowActiveAndVisible(windows_to_render_top_most[n])) // NavWindowingTarget is always temporarily displayed as the top-most window
            AddRootWindowToDrawData(windows_to_render_top_most[n]);

    // Draw software mouse cursor if requested by io.MouseDrawCursor flag
    if (g.IO.MouseDrawCursor && g.MouseCursor != ImGuiMouseCursor_None)
        RenderMouseCursor(g.IO.MousePos, g.Style.MouseCursorScale, g.MouseCursor, IM_COL32_WHITE, IM_COL32_BLACK, IM_COL32(0, 0, 0, 48));

    // Setup ImDrawData structures for end-user
    g.IO.MetricsRenderVertices = g.IO.MetricsRenderIndices = 0;
    for (ImGuiViewportP* viewport : g.Viewports)
    {
        FlattenDrawDataIntoSingleLayer(&viewport->DrawDataBuilder);

        // Add foreground ImDrawList (for each active viewport)
        if (viewport->BgFgDrawLists[1] != NULL)
            AddDrawListToDrawDataEx(&viewport->DrawDataP, viewport->DrawDataBuilder.Layers[0], GetForegroundDrawList(viewport));

        // We call _PopUnusedDrawCmd() last thing, as RenderDimmedBackgrounds() rely on a valid command being there (especially in docking branch).
        ImDrawData* draw_data = &viewport->DrawDataP;
        IM_ASSERT(draw_data->CmdLists.Size == draw_data->CmdListsCount);
        for (ImDrawList* draw_list : draw_data->CmdLists)
            draw_list->_PopUnusedDrawCmd();

        g.IO.MetricsRenderVertices += draw_data->TotalVtxCount;
        g.IO.MetricsRenderIndices += draw_data->TotalIdxCount;
    }

    CallContextHooks(&g, ImGuiContextHookType_RenderPost);
}

// Calculate text size. Text can be multi-line. Optionally ignore text after a ## marker.
// CalcTextSize("") should return ImVec2(0.0f, g.FontSize)
ImVec2 ImGui::CalcTextSize(const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width)
{
    ImGuiContext& g = *GImGui;

    const char* text_display_end;
    if (hide_text_after_double_hash)
        text_display_end = FindRenderedTextEnd(text, text_end);      // Hide anything after a '##' string
    else
        text_display_end = text_end;

    ImFont* font = g.Font;
    const float font_size = g.FontSize;
    if (text == text_display_end)
        return ImVec2(0.0f, font_size);
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, wrap_width, text, text_display_end, NULL);

    // Round
    // FIXME: This has been here since Dec 2015 (7b0bf230) but down the line we want this out.
    // FIXME: Investigate using ceilf or e.g.
    // - https://git.musl-libc.org/cgit/musl/tree/src/math/ceilf.c
    // - https://embarkstudios.github.io/rust-gpu/api/src/libm/math/ceilf.rs.html
    text_size.x = IM_TRUNC(text_size.x + 0.99999f);

    return text_size;
}

// Find window given position, search front-to-back
// - Typically write output back to g.HoveredWindow and g.HoveredWindowUnderMovingWindow.
// - FIXME: Note that we have an inconsequential lag here: OuterRectClipped is updated in Begin(), so windows moved programmatically
//   with SetWindowPos() and not SetNextWindowPos() will have that rectangle lagging by a frame at the time FindHoveredWindow() is
//   called, aka before the next Begin(). Moving window isn't affected.
// - The 'find_first_and_in_any_viewport = true' mode is only used by TestEngine. It is simpler to maintain here.
void ImGui::FindHoveredWindowEx(const ImVec2& pos, bool find_first_and_in_any_viewport, ImGuiWindow** out_hovered_window, ImGuiWindow** out_hovered_window_under_moving_window)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* hovered_window = NULL;
    ImGuiWindow* hovered_window_under_moving_window = NULL;

    if (find_first_and_in_any_viewport == false && g.MovingWindow && !(g.MovingWindow->Flags & ImGuiWindowFlags_NoMouseInputs))
        hovered_window = g.MovingWindow;

    ImVec2 padding_regular = g.Style.TouchExtraPadding;
    ImVec2 padding_for_resize = g.IO.ConfigWindowsResizeFromEdges ? g.WindowsHoverPadding : padding_regular;
    for (int i = g.Windows.Size - 1; i >= 0; i--)
    {
        ImGuiWindow* window = g.Windows[i];
        IM_MSVC_WARNING_SUPPRESS(28182); // [Static Analyzer] Dereferencing NULL pointer.
        if (!window->WasActive || window->Hidden)
            continue;
        if (window->Flags & ImGuiWindowFlags_NoMouseInputs)
            continue;

        // Using the clipped AABB, a child window will typically be clipped by its parent (not always)
        ImVec2 hit_padding = (window->Flags & (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) ? padding_regular : padding_for_resize;
        if (!window->OuterRectClipped.ContainsWithPad(pos, hit_padding))
            continue;

        // Support for one rectangular hole in any given window
        // FIXME: Consider generalizing hit-testing override (with more generic data, callback, etc.) (#1512)
        if (window->HitTestHoleSize.x != 0)
        {
            ImVec2 hole_pos(window->Pos.x + (float)window->HitTestHoleOffset.x, window->Pos.y + (float)window->HitTestHoleOffset.y);
            ImVec2 hole_size((float)window->HitTestHoleSize.x, (float)window->HitTestHoleSize.y);
            if (ImRect(hole_pos, hole_pos + hole_size).Contains(pos))
                continue;
        }

        if (find_first_and_in_any_viewport)
        {
            hovered_window = window;
            break;
        }
        else
        {
            if (hovered_window == NULL)
                hovered_window = window;
            IM_MSVC_WARNING_SUPPRESS(28182); // [Static Analyzer] Dereferencing NULL pointer.
            if (hovered_window_under_moving_window == NULL && (!g.MovingWindow || window->RootWindow != g.MovingWindow->RootWindow))
                hovered_window_under_moving_window = window;
            if (hovered_window && hovered_window_under_moving_window)
                break;
        }
    }

    *out_hovered_window = hovered_window;
    if (out_hovered_window_under_moving_window != NULL)
        *out_hovered_window_under_moving_window = hovered_window_under_moving_window;
}

bool ImGui::IsItemActive()
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId)
        return g.ActiveId == g.LastItemData.ID;
    return false;
}

bool ImGui::IsItemActivated()
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId)
        if (g.ActiveId == g.LastItemData.ID && g.ActiveIdPreviousFrame != g.LastItemData.ID)
            return true;
    return false;
}

bool ImGui::IsItemDeactivated()
{
    ImGuiContext& g = *GImGui;
    if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HasDeactivated)
        return (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Deactivated) != 0;
    return (g.ActiveIdPreviousFrame == g.LastItemData.ID && g.ActiveIdPreviousFrame != 0 && g.ActiveId != g.LastItemData.ID);
}

bool ImGui::IsItemDeactivatedAfterEdit()
{
    ImGuiContext& g = *GImGui;
    return IsItemDeactivated() && (g.ActiveIdPreviousFrameHasBeenEditedBefore || (g.ActiveId == 0 && g.ActiveIdHasBeenEditedBefore));
}

// == GetItemID() == GetFocusID()
bool ImGui::IsItemFocused()
{
    ImGuiContext& g = *GImGui;
    return g.NavId == g.LastItemData.ID && g.NavId != 0;
}

// Important: this can be useful but it is NOT equivalent to the behavior of e.g.Button()!
// Most widgets have specific reactions based on mouse-up/down state, mouse position etc.
bool ImGui::IsItemClicked(ImGuiMouseButton mouse_button)
{
    return IsMouseClicked(mouse_button) && IsItemHovered(ImGuiHoveredFlags_None);
}

bool ImGui::IsItemToggledOpen()
{
    ImGuiContext& g = *GImGui;
    return (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_ToggledOpen) ? true : false;
}

// Call after a Selectable() or TreeNode() involved in multi-selection.
// Useful if you need the per-item information before reaching EndMultiSelect(), e.g. for rendering purpose.
// This is only meant to be called inside a BeginMultiSelect()/EndMultiSelect() block.
// (Outside of multi-select, it would be misleading/ambiguous to report this signal, as widgets
// return e.g. a pressed event and user code is in charge of altering selection in ways we cannot predict.)
bool ImGui::IsItemToggledSelection()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentMultiSelect != NULL); // Can only be used inside a BeginMultiSelect()/EndMultiSelect()
    return (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_ToggledSelection) ? true : false;
}

// IMPORTANT: If you are trying to check whether your mouse should be dispatched to Dear ImGui or to your underlying app,
// you should not use this function! Use the 'io.WantCaptureMouse' boolean for that!
// Refer to FAQ entry "How can I tell whether to dispatch mouse/keyboard to Dear ImGui or my application?" for details.
bool ImGui::IsAnyItemHovered()
{
    ImGuiContext& g = *GImGui;
    return g.HoveredId != 0 || g.HoveredIdPreviousFrame != 0;
}

bool ImGui::IsAnyItemActive()
{
    ImGuiContext& g = *GImGui;
    return g.ActiveId != 0;
}

bool ImGui::IsAnyItemFocused()
{
    ImGuiContext& g = *GImGui;
    return g.NavId != 0 && !g.NavDisableHighlight;
}

bool ImGui::IsItemVisible()
{
    ImGuiContext& g = *GImGui;
    return (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible) != 0;
}

bool ImGui::IsItemEdited()
{
    ImGuiContext& g = *GImGui;
    return (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Edited) != 0;
}

// Allow next item to be overlapped by subsequent items.
// This works by requiring HoveredId to match for two subsequent frames,
// so if a following items overwrite it our interactions will naturally be disabled.
void ImGui::SetNextItemAllowOverlap()
{
    ImGuiContext& g = *GImGui;
    g.NextItemData.ItemFlags |= ImGuiItemFlags_AllowOverlap;
}

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
// Allow last item to be overlapped by a subsequent item. Both may be activated during the same frame before the later one takes priority.
// FIXME-LEGACY: Use SetNextItemAllowOverlap() *before* your item instead.
void ImGui::SetItemAllowOverlap()
{
    ImGuiContext& g = *GImGui;
    ImGuiID id = g.LastItemData.ID;
    if (g.HoveredId == id)
        g.HoveredIdAllowOverlap = true;
    if (g.ActiveId == id) // Before we made this obsolete, most calls to SetItemAllowOverlap() used to avoid this path by testing g.ActiveId != id.
        g.ActiveIdAllowOverlap = true;
}
#endif

// This is a shortcut for not taking ownership of 100+ keys, frequently used by drag operations.
// FIXME: It might be undesirable that this will likely disable KeyOwner-aware shortcuts systems. Consider a more fine-tuned version if needed?
void ImGui::SetActiveIdUsingAllKeyboardKeys()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ActiveId != 0);
    g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_COUNT) - 1;
    g.ActiveIdUsingAllKeyboardKeys = true;
    NavMoveRequestCancel();
}

ImGuiID ImGui::GetItemID()
{
    ImGuiContext& g = *GImGui;
    return g.LastItemData.ID;
}

ImVec2 ImGui::GetItemRectMin()
{
    ImGuiContext& g = *GImGui;
    return g.LastItemData.Rect.Min;
}

ImVec2 ImGui::GetItemRectMax()
{
    ImGuiContext& g = *GImGui;
    return g.LastItemData.Rect.Max;
}

ImVec2 ImGui::GetItemRectSize()
{
    ImGuiContext& g = *GImGui;
    return g.LastItemData.Rect.GetSize();
}

// Prior to v1.90 2023/10/16, the BeginChild() function took a 'bool border = false' parameter instead of 'ImGuiChildFlags child_flags = 0'.
// ImGuiChildFlags_Borders is defined as always == 1 in order to allow old code passing 'true'. Read comments in imgui.h for details!
bool ImGui::BeginChild(const char* str_id, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
{
    ImGuiID id = GetCurrentWindow()->GetID(str_id);
    return BeginChildEx(str_id, id, size_arg, child_flags, window_flags);
}

bool ImGui::BeginChild(ImGuiID id, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
{
    return BeginChildEx(NULL, id, size_arg, child_flags, window_flags);
}

bool ImGui::BeginChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;
    IM_ASSERT(id != 0);

    // Sanity check as it is likely that some user will accidentally pass ImGuiWindowFlags into the ImGuiChildFlags argument.
    const ImGuiChildFlags ImGuiChildFlags_SupportedMask_ = ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_FrameStyle | ImGuiChildFlags_NavFlattened;
    IM_UNUSED(ImGuiChildFlags_SupportedMask_);
    IM_ASSERT((child_flags & ~ImGuiChildFlags_SupportedMask_) == 0 && "Illegal ImGuiChildFlags value. Did you pass ImGuiWindowFlags values instead of ImGuiChildFlags?");
    IM_ASSERT((window_flags & ImGuiWindowFlags_AlwaysAutoResize) == 0 && "Cannot specify ImGuiWindowFlags_AlwaysAutoResize for BeginChild(). Use ImGuiChildFlags_AlwaysAutoResize!");
    if (child_flags & ImGuiChildFlags_AlwaysAutoResize)
    {
        IM_ASSERT((child_flags & (ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY)) == 0 && "Cannot use ImGuiChildFlags_ResizeX or ImGuiChildFlags_ResizeY with ImGuiChildFlags_AlwaysAutoResize!");
        IM_ASSERT((child_flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) != 0 && "Must use ImGuiChildFlags_AutoResizeX or ImGuiChildFlags_AutoResizeY with ImGuiChildFlags_AlwaysAutoResize!");
    }
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    if (window_flags & ImGuiWindowFlags_AlwaysUseWindowPadding)
        child_flags |= ImGuiChildFlags_AlwaysUseWindowPadding;
    if (window_flags & ImGuiWindowFlags_NavFlattened)
        child_flags |= ImGuiChildFlags_NavFlattened;
#endif
    if (child_flags & ImGuiChildFlags_AutoResizeX)
        child_flags &= ~ImGuiChildFlags_ResizeX;
    if (child_flags & ImGuiChildFlags_AutoResizeY)
        child_flags &= ~ImGuiChildFlags_ResizeY;

    // Set window flags
    window_flags |= ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoTitleBar;
    window_flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove); // Inherit the NoMove flag
    if (child_flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize))
        window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
    if ((child_flags & (ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY)) == 0)
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

    // Special framed style
    if (child_flags & ImGuiChildFlags_FrameStyle)
    {
        PushStyleColor(ImGuiCol_ChildBg, g.Style.Colors[ImGuiCol_FrameBg]);
        PushStyleVar(ImGuiStyleVar_ChildRounding, g.Style.FrameRounding);
        PushStyleVar(ImGuiStyleVar_ChildBorderSize, g.Style.FrameBorderSize);
        PushStyleVar(ImGuiStyleVar_WindowPadding, g.Style.FramePadding);
        child_flags |= ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding;
        window_flags |= ImGuiWindowFlags_NoMove;
    }

    // Forward size
    // Important: Begin() has special processing to switch condition to ImGuiCond_FirstUseEver for a given axis when ImGuiChildFlags_ResizeXXX is set.
    // (the alternative would to store conditional flags per axis, which is possible but more code)
    const ImVec2 size_avail = GetContentRegionAvail();
    const ImVec2 size_default((child_flags & ImGuiChildFlags_AutoResizeX) ? 0.0f : size_avail.x, (child_flags & ImGuiChildFlags_AutoResizeY) ? 0.0f : size_avail.y);
    ImVec2 size = CalcItemSize(size_arg, size_default.x, size_default.y);

    // A SetNextWindowSize() call always has priority (#8020)
    // (since the code in Begin() never supported SizeVal==0.0f aka auto-resize via SetNextWindowSize() call, we don't support it here for now)
    // FIXME: We only support ImGuiCond_Always in this path. Supporting other paths would requires to obtain window pointer.
    if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize) != 0 && (g.NextWindowData.SizeCond & ImGuiCond_Always) != 0)
    {
        if (g.NextWindowData.SizeVal.x > 0.0f)
        {
            size.x = g.NextWindowData.SizeVal.x;
            child_flags &= ~ImGuiChildFlags_ResizeX;
        }
        if (g.NextWindowData.SizeVal.y > 0.0f)
        {
            size.y = g.NextWindowData.SizeVal.y;
            child_flags &= ~ImGuiChildFlags_ResizeY;
        }
    }
    SetNextWindowSize(size);

    // Forward child flags
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasChildFlags;
    g.NextWindowData.ChildFlags = child_flags;

    // Build up name. If you need to append to a same child from multiple location in the ID stack, use BeginChild(ImGuiID id) with a stable value.
    // FIXME: 2023/11/14: commented out shorted version. We had an issue with multiple ### in child window path names, which the trailing hash helped workaround.
    // e.g. "ParentName###ParentIdentifier/ChildName###ChildIdentifier" would get hashed incorrectly by ImHashStr(), trailing _%08X somehow fixes it.
    const char* temp_window_name;
    /*if (name && parent_window->IDStack.back() == parent_window->ID)
        ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%s", parent_window->Name, name); // May omit ID if in root of ID stack
    else*/
    if (name)
        ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%s_%08X", parent_window->Name, name, id);
    else
        ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%08X", parent_window->Name, id);

    // Set style
    const float backup_border_size = g.Style.ChildBorderSize;
    if ((child_flags & ImGuiChildFlags_Borders) == 0)
        g.Style.ChildBorderSize = 0.0f;

    // Begin into window
    const bool ret = Begin(temp_window_name, NULL, window_flags);

    // Restore style
    g.Style.ChildBorderSize = backup_border_size;
    if (child_flags & ImGuiChildFlags_FrameStyle)
    {
        PopStyleVar(3);
        PopStyleColor();
    }

    ImGuiWindow* child_window = g.CurrentWindow;
    child_window->ChildId = id;

    // Set the cursor to handle case where the user called SetNextWindowPos()+BeginChild() manually.
    // While this is not really documented/defined, it seems that the expected thing to do.
    if (child_window->BeginCount == 1)
        parent_window->DC.CursorPos = child_window->Pos;

    // Process navigation-in immediately so NavInit can run on first frame
    // Can enter a child if (A) it has navigable items or (B) it can be scrolled.
    const ImGuiID temp_id_for_activation = ImHashStr("##Child", 0, id);
    if (g.ActiveId == temp_id_for_activation)
        ClearActiveID();
    if (g.NavActivateId == id && !(child_flags & ImGuiChildFlags_NavFlattened) && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY))
    {
        FocusWindow(child_window);
        NavInitWindow(child_window, false);
        SetActiveID(temp_id_for_activation, child_window); // Steal ActiveId with another arbitrary id so that key-press won't activate child item
        g.ActiveIdSource = g.NavInputSource;
    }
    return ret;
}

void ImGui::EndChild()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* child_window = g.CurrentWindow;

    IM_ASSERT(g.WithinEndChild == false);
    IM_ASSERT(child_window->Flags & ImGuiWindowFlags_ChildWindow);   // Mismatched BeginChild()/EndChild() calls

    g.WithinEndChild = true;
    ImVec2 child_size = child_window->Size;
    End();
    if (child_window->BeginCount == 1)
    {
        ImGuiWindow* parent_window = g.CurrentWindow;
        ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + child_size);
        ItemSize(child_size);
        const bool nav_flattened = (child_window->ChildFlags & ImGuiChildFlags_NavFlattened) != 0;
        if ((child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY) && !nav_flattened)
        {
            ItemAdd(bb, child_window->ChildId);
            RenderNavHighlight(bb, child_window->ChildId);

            // When browsing a window that has no activable items (scroll only) we keep a highlight on the child (pass g.NavId to trick into always displaying)
            if (child_window->DC.NavLayersActiveMask == 0 && child_window == g.NavWindow)
                RenderNavHighlight(ImRect(bb.Min - ImVec2(2, 2), bb.Max + ImVec2(2, 2)), g.NavId, ImGuiNavHighlightFlags_Compact);
        }
        else
        {
            // Not navigable into
            // - This is a bit of a fringe use case, mostly useful for undecorated, non-scrolling contents childs, or empty childs.
            // - We could later decide to not apply this path if ImGuiChildFlags_FrameStyle or ImGuiChildFlags_Borders is set.
            ItemAdd(bb, child_window->ChildId, NULL, ImGuiItemFlags_NoNav);

            // But when flattened we directly reach items, adjust active layer mask accordingly
            if (nav_flattened)
                parent_window->DC.NavLayersActiveMaskNext |= child_window->DC.NavLayersActiveMaskNext;
        }
        if (g.HoveredWindow == child_window)
            g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
    }
    g.WithinEndChild = false;
    g.LogLinePosY = -FLT_MAX; // To enforce a carriage return
}

static void SetWindowConditionAllowFlags(ImGuiWindow* window, ImGuiCond flags, bool enabled)
{
    window->SetWindowPosAllowFlags       = enabled ? (window->SetWindowPosAllowFlags       | flags) : (window->SetWindowPosAllowFlags       & ~flags);
    window->SetWindowSizeAllowFlags      = enabled ? (window->SetWindowSizeAllowFlags      | flags) : (window->SetWindowSizeAllowFlags      & ~flags);
    window->SetWindowCollapsedAllowFlags = enabled ? (window->SetWindowCollapsedAllowFlags | flags) : (window->SetWindowCollapsedAllowFlags & ~flags);
}

ImGuiWindow* ImGui::FindWindowByID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    return (ImGuiWindow*)g.WindowsById.GetVoidPtr(id);
}

ImGuiWindow* ImGui::FindWindowByName(const char* name)
{
    ImGuiID id = ImHashStr(name);
    return FindWindowByID(id);
}

static void ApplyWindowSettings(ImGuiWindow* window, ImGuiWindowSettings* settings)
{
    window->Pos = ImTrunc(ImVec2(settings->Pos.x, settings->Pos.y));
    if (settings->Size.x > 0 && settings->Size.y > 0)
        window->Size = window->SizeFull = ImTrunc(ImVec2(settings->Size.x, settings->Size.y));
    window->Collapsed = settings->Collapsed;
}

static void UpdateWindowInFocusOrderList(ImGuiWindow* window, bool just_created, ImGuiWindowFlags new_flags)
{
    ImGuiContext& g = *GImGui;

    const bool new_is_explicit_child = (new_flags & ImGuiWindowFlags_ChildWindow) != 0 && ((new_flags & ImGuiWindowFlags_Popup) == 0 || (new_flags & ImGuiWindowFlags_ChildMenu) != 0);
    const bool child_flag_changed = new_is_explicit_child != window->IsExplicitChild;
    if ((just_created || child_flag_changed) && !new_is_explicit_child)
    {
        IM_ASSERT(!g.WindowsFocusOrder.contains(window));
        g.WindowsFocusOrder.push_back(window);
        window->FocusOrder = (short)(g.WindowsFocusOrder.Size - 1);
    }
    else if (!just_created && child_flag_changed && new_is_explicit_child)
    {
        IM_ASSERT(g.WindowsFocusOrder[window->FocusOrder] == window);
        for (int n = window->FocusOrder + 1; n < g.WindowsFocusOrder.Size; n++)
            g.WindowsFocusOrder[n]->FocusOrder--;
        g.WindowsFocusOrder.erase(g.WindowsFocusOrder.Data + window->FocusOrder);
        window->FocusOrder = -1;
    }
    window->IsExplicitChild = new_is_explicit_child;
}

static void InitOrLoadWindowSettings(ImGuiWindow* window, ImGuiWindowSettings* settings)
{
    // Initial window state with e.g. default/arbitrary window position
    // Use SetNextWindowPos() with the appropriate condition flag to change the initial position of a window.
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    window->Pos = main_viewport->Pos + ImVec2(60, 60);
    window->Size = window->SizeFull = ImVec2(0, 0);
    window->SetWindowPosAllowFlags = window->SetWindowSizeAllowFlags = window->SetWindowCollapsedAllowFlags = ImGuiCond_Always | ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing;

    if (settings != NULL)
    {
        SetWindowConditionAllowFlags(window, ImGuiCond_FirstUseEver, false);
        ApplyWindowSettings(window, settings);
    }
    window->DC.CursorStartPos = window->DC.CursorMaxPos = window->DC.IdealMaxPos = window->Pos; // So first call to CalcWindowContentSizes() doesn't return crazy values

    if ((window->Flags & ImGuiWindowFlags_AlwaysAutoResize) != 0)
    {
        window->AutoFitFramesX = window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = false;
    }
    else
    {
        if (window->Size.x <= 0.0f)
            window->AutoFitFramesX = 2;
        if (window->Size.y <= 0.0f)
            window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = (window->AutoFitFramesX > 0) || (window->AutoFitFramesY > 0);
    }
}

static ImGuiWindow* CreateNewWindow(const char* name, ImGuiWindowFlags flags)
{
    // Create window the first time
    //IMGUI_DEBUG_LOG("CreateNewWindow '%s', flags = 0x%08X\n", name, flags);
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = IM_NEW(ImGuiWindow)(&g, name);
    window->Flags = flags;
    g.WindowsById.SetVoidPtr(window->ID, window);

    ImGuiWindowSettings* settings = NULL;
    if (!(flags & ImGuiWindowFlags_NoSavedSettings))
        if ((settings = ImGui::FindWindowSettingsByWindow(window)) != 0)
            window->SettingsOffset = g.SettingsWindows.offset_from_ptr(settings);

    InitOrLoadWindowSettings(window, settings);

    if (flags & ImGuiWindowFlags_NoBringToFrontOnFocus)
        g.Windows.push_front(window); // Quite slow but rare and only once
    else
        g.Windows.push_back(window);

    return window;
}

static inline ImVec2 CalcWindowMinSize(ImGuiWindow* window)
{
    // We give windows non-zero minimum size to facilitate understanding problematic cases (e.g. empty popups)
    // FIXME: Essentially we want to restrict manual resizing to WindowMinSize+Decoration, and allow api resizing to be smaller.
    // Perhaps should tend further a neater test for this.
    ImGuiContext& g = *GImGui;
    ImVec2 size_min;
    if ((window->Flags & ImGuiWindowFlags_ChildWindow) && !(window->Flags & ImGuiWindowFlags_Popup))
    {
        size_min.x = (window->ChildFlags & ImGuiChildFlags_ResizeX) ? g.Style.WindowMinSize.x : 4.0f;
        size_min.y = (window->ChildFlags & ImGuiChildFlags_ResizeY) ? g.Style.WindowMinSize.y : 4.0f;
    }
    else
    {
        size_min.x = ((window->Flags & ImGuiWindowFlags_AlwaysAutoResize) == 0) ? g.Style.WindowMinSize.x : 4.0f;
        size_min.y = ((window->Flags & ImGuiWindowFlags_AlwaysAutoResize) == 0) ? g.Style.WindowMinSize.y : 4.0f;
    }

    // Reduce artifacts with very small windows
    ImGuiWindow* window_for_height = window;
    size_min.y = ImMax(size_min.y, window_for_height->TitleBarHeight + window_for_height->MenuBarHeight + ImMax(0.0f, g.Style.WindowRounding - 1.0f));
    return size_min;
}

static ImVec2 CalcWindowSizeAfterConstraint(ImGuiWindow* window, const ImVec2& size_desired)
{
    ImGuiContext& g = *GImGui;
    ImVec2 new_size = size_desired;
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)
    {
        // See comments in SetNextWindowSizeConstraints() for details about setting size_min an size_max.
        ImRect cr = g.NextWindowData.SizeConstraintRect;
        new_size.x = (cr.Min.x >= 0 && cr.Max.x >= 0) ? ImClamp(new_size.x, cr.Min.x, cr.Max.x) : window->SizeFull.x;
        new_size.y = (cr.Min.y >= 0 && cr.Max.y >= 0) ? ImClamp(new_size.y, cr.Min.y, cr.Max.y) : window->SizeFull.y;
        if (g.NextWindowData.SizeCallback)
        {
            ImGuiSizeCallbackData data;
            data.UserData = g.NextWindowData.SizeCallbackUserData;
            data.Pos = window->Pos;
            data.CurrentSize = window->SizeFull;
            data.DesiredSize = new_size;
            g.NextWindowData.SizeCallback(&data);
            new_size = data.DesiredSize;
        }
        new_size.x = IM_TRUNC(new_size.x);
        new_size.y = IM_TRUNC(new_size.y);
    }

    // Minimum size
    ImVec2 size_min = CalcWindowMinSize(window);
    return ImMax(new_size, size_min);
}

static void CalcWindowContentSizes(ImGuiWindow* window, ImVec2* content_size_current, ImVec2* content_size_ideal)
{
    bool preserve_old_content_sizes = false;
    if (window->Collapsed && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
        preserve_old_content_sizes = true;
    else if (window->Hidden && window->HiddenFramesCannotSkipItems == 0 && window->HiddenFramesCanSkipItems > 0)
        preserve_old_content_sizes = true;
    if (preserve_old_content_sizes)
    {
        *content_size_current = window->ContentSize;
        *content_size_ideal = window->ContentSizeIdeal;
        return;
    }

    content_size_current->x = (window->ContentSizeExplicit.x != 0.0f) ? window->ContentSizeExplicit.x : IM_TRUNC(window->DC.CursorMaxPos.x - window->DC.CursorStartPos.x);
    content_size_current->y = (window->ContentSizeExplicit.y != 0.0f) ? window->ContentSizeExplicit.y : IM_TRUNC(window->DC.CursorMaxPos.y - window->DC.CursorStartPos.y);
    content_size_ideal->x = (window->ContentSizeExplicit.x != 0.0f) ? window->ContentSizeExplicit.x : IM_TRUNC(ImMax(window->DC.CursorMaxPos.x, window->DC.IdealMaxPos.x) - window->DC.CursorStartPos.x);
    content_size_ideal->y = (window->ContentSizeExplicit.y != 0.0f) ? window->ContentSizeExplicit.y : IM_TRUNC(ImMax(window->DC.CursorMaxPos.y, window->DC.IdealMaxPos.y) - window->DC.CursorStartPos.y);
}

static ImVec2 CalcWindowAutoFitSize(ImGuiWindow* window, const ImVec2& size_contents)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    const float decoration_w_without_scrollbars = window->DecoOuterSizeX1 + window->DecoOuterSizeX2 - window->ScrollbarSizes.x;
    const float decoration_h_without_scrollbars = window->DecoOuterSizeY1 + window->DecoOuterSizeY2 - window->ScrollbarSizes.y;
    ImVec2 size_pad = window->WindowPadding * 2.0f;
    ImVec2 size_desired = size_contents + size_pad + ImVec2(decoration_w_without_scrollbars, decoration_h_without_scrollbars);
    if (window->Flags & ImGuiWindowFlags_Tooltip)
    {
        // Tooltip always resize
        return size_desired;
    }
    else
    {
        // Maximum window size is determined by the viewport size or monitor size
        ImVec2 size_min = CalcWindowMinSize(window);
        ImVec2 size_max = ((window->Flags & ImGuiWindowFlags_ChildWindow) && !(window->Flags & ImGuiWindowFlags_Popup)) ? ImVec2(FLT_MAX, FLT_MAX) : ImGui::GetMainViewport()->WorkSize - style.DisplaySafeAreaPadding * 2.0f;
        ImVec2 size_auto_fit = ImClamp(size_desired, size_min, size_max);

        // FIXME: CalcWindowAutoFitSize() doesn't take into account that only one axis may be auto-fit when calculating scrollbars,
        // we may need to compute/store three variants of size_auto_fit, for x/y/xy.
        // Here we implement a workaround for child windows only, but a full solution would apply to normal windows as well:
        if ((window->ChildFlags & ImGuiChildFlags_ResizeX) && !(window->ChildFlags & ImGuiChildFlags_ResizeY))
            size_auto_fit.y = window->SizeFull.y;
        else if (!(window->ChildFlags & ImGuiChildFlags_ResizeX) && (window->ChildFlags & ImGuiChildFlags_ResizeY))
            size_auto_fit.x = window->SizeFull.x;

        // When the window cannot fit all contents (either because of constraints, either because screen is too small),
        // we are growing the size on the other axis to compensate for expected scrollbar. FIXME: Might turn bigger than ViewportSize-WindowPadding.
        ImVec2 size_auto_fit_after_constraint = CalcWindowSizeAfterConstraint(window, size_auto_fit);
        bool will_have_scrollbar_x = (size_auto_fit_after_constraint.x - size_pad.x - decoration_w_without_scrollbars < size_contents.x && !(window->Flags & ImGuiWindowFlags_NoScrollbar) && (window->Flags & ImGuiWindowFlags_HorizontalScrollbar)) || (window->Flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar);
        bool will_have_scrollbar_y = (size_auto_fit_after_constraint.y - size_pad.y - decoration_h_without_scrollbars < size_contents.y && !(window->Flags & ImGuiWindowFlags_NoScrollbar)) || (window->Flags & ImGuiWindowFlags_AlwaysVerticalScrollbar);
        if (will_have_scrollbar_x)
            size_auto_fit.y += style.ScrollbarSize;
        if (will_have_scrollbar_y)
            size_auto_fit.x += style.ScrollbarSize;
        return size_auto_fit;
    }
}

ImVec2 ImGui::CalcWindowNextAutoFitSize(ImGuiWindow* window)
{
    ImVec2 size_contents_current;
    ImVec2 size_contents_ideal;
    CalcWindowContentSizes(window, &size_contents_current, &size_contents_ideal);
    ImVec2 size_auto_fit = CalcWindowAutoFitSize(window, size_contents_ideal);
    ImVec2 size_final = CalcWindowSizeAfterConstraint(window, size_auto_fit);
    return size_final;
}

static ImGuiCol GetWindowBgColorIdx(ImGuiWindow* window)
{
    if (window->Flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup))
        return ImGuiCol_PopupBg;
    if (window->Flags & ImGuiWindowFlags_ChildWindow)
        return ImGuiCol_ChildBg;
    return ImGuiCol_WindowBg;
}

static void CalcResizePosSizeFromAnyCorner(ImGuiWindow* window, const ImVec2& corner_target, const ImVec2& corner_norm, ImVec2* out_pos, ImVec2* out_size)
{
    ImVec2 pos_min = ImLerp(corner_target, window->Pos, corner_norm);                // Expected window upper-left
    ImVec2 pos_max = ImLerp(window->Pos + window->Size, corner_target, corner_norm); // Expected window lower-right
    ImVec2 size_expected = pos_max - pos_min;
    ImVec2 size_constrained = CalcWindowSizeAfterConstraint(window, size_expected);
    *out_pos = pos_min;
    if (corner_norm.x == 0.0f)
        out_pos->x -= (size_constrained.x - size_expected.x);
    if (corner_norm.y == 0.0f)
        out_pos->y -= (size_constrained.y - size_expected.y);
    *out_size = size_constrained;
}

// Data for resizing from resize grip / corner
struct ImGuiResizeGripDef
{
    ImVec2  CornerPosN;
    ImVec2  InnerDir;
    int     AngleMin12, AngleMax12;
};
static const ImGuiResizeGripDef resize_grip_def[4] =
{
    { ImVec2(1, 1), ImVec2(-1, -1), 0, 3 },  // Lower-right
    { ImVec2(0, 1), ImVec2(+1, -1), 3, 6 },  // Lower-left
    { ImVec2(0, 0), ImVec2(+1, +1), 6, 9 },  // Upper-left (Unused)
    { ImVec2(1, 0), ImVec2(-1, +1), 9, 12 }  // Upper-right (Unused)
};

// Data for resizing from borders
struct ImGuiResizeBorderDef
{
    ImVec2  InnerDir;               // Normal toward inside
    ImVec2  SegmentN1, SegmentN2;   // End positions, normalized (0,0: upper left)
    float   OuterAngle;             // Angle toward outside
};
static const ImGuiResizeBorderDef resize_border_def[4] =
{
    { ImVec2(+1, 0), ImVec2(0, 1), ImVec2(0, 0), IM_PI * 1.00f }, // Left
    { ImVec2(-1, 0), ImVec2(1, 0), ImVec2(1, 1), IM_PI * 0.00f }, // Right
    { ImVec2(0, +1), ImVec2(0, 0), ImVec2(1, 0), IM_PI * 1.50f }, // Up
    { ImVec2(0, -1), ImVec2(1, 1), ImVec2(0, 1), IM_PI * 0.50f }  // Down
};

static ImRect GetResizeBorderRect(ImGuiWindow* window, int border_n, float perp_padding, float thickness)
{
    ImRect rect = window->Rect();
    if (thickness == 0.0f)
        rect.Max -= ImVec2(1, 1);
    if (border_n == ImGuiDir_Left)  { return ImRect(rect.Min.x - thickness,    rect.Min.y + perp_padding, rect.Min.x + thickness,    rect.Max.y - perp_padding); }
    if (border_n == ImGuiDir_Right) { return ImRect(rect.Max.x - thickness,    rect.Min.y + perp_padding, rect.Max.x + thickness,    rect.Max.y - perp_padding); }
    if (border_n == ImGuiDir_Up)    { return ImRect(rect.Min.x + perp_padding, rect.Min.y - thickness,    rect.Max.x - perp_padding, rect.Min.y + thickness);    }
    if (border_n == ImGuiDir_Down)  { return ImRect(rect.Min.x + perp_padding, rect.Max.y - thickness,    rect.Max.x - perp_padding, rect.Max.y + thickness);    }
    IM_ASSERT(0);
    return ImRect();
}

// 0..3: corners (Lower-right, Lower-left, Unused, Unused)
ImGuiID ImGui::GetWindowResizeCornerID(ImGuiWindow* window, int n)
{
    IM_ASSERT(n >= 0 && n < 4);
    ImGuiID id = window->ID;
    id = ImHashStr("#RESIZE", 0, id);
    id = ImHashData(&n, sizeof(int), id);
    return id;
}

// Borders (Left, Right, Up, Down)
ImGuiID ImGui::GetWindowResizeBorderID(ImGuiWindow* window, ImGuiDir dir)
{
    IM_ASSERT(dir >= 0 && dir < 4);
    int n = (int)dir + 4;
    ImGuiID id = window->ID;
    id = ImHashStr("#RESIZE", 0, id);
    id = ImHashData(&n, sizeof(int), id);
    return id;
}

// Handle resize for: Resize Grips, Borders, Gamepad
// Return true when using auto-fit (double-click on resize grip)
static int ImGui::UpdateWindowManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_hovered, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4], const ImRect& visibility_rect)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindowFlags flags = window->Flags;

    if ((flags & ImGuiWindowFlags_NoResize) || (flags & ImGuiWindowFlags_AlwaysAutoResize) || window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
        return false;
    if (window->WasActive == false) // Early out to avoid running this code for e.g. a hidden implicit/fallback Debug window.
        return false;

    int ret_auto_fit_mask = 0x00;
    const float grip_draw_size = IM_TRUNC(ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f));
    const float grip_hover_inner_size = (resize_grip_count > 0) ? IM_TRUNC(grip_draw_size * 0.75f) : 0.0f;
    const float grip_hover_outer_size = g.IO.ConfigWindowsResizeFromEdges ? WINDOWS_HOVER_PADDING : 0.0f;

    ImRect clamp_rect = visibility_rect;
    const bool window_move_from_title_bar = g.IO.ConfigWindowsMoveFromTitleBarOnly && !(window->Flags & ImGuiWindowFlags_NoTitleBar);
    if (window_move_from_title_bar)
        clamp_rect.Min.y -= window->TitleBarHeight;

    ImVec2 pos_target(FLT_MAX, FLT_MAX);
    ImVec2 size_target(FLT_MAX, FLT_MAX);

    // Resize grips and borders are on layer 1
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

    // Manual resize grips
    PushID("#RESIZE");
    for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
    {
        const ImGuiResizeGripDef& def = resize_grip_def[resize_grip_n];
        const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, def.CornerPosN);

        // Using the FlattenChilds button flag we make the resize button accessible even if we are hovering over a child window
        bool hovered, held;
        ImRect resize_rect(corner - def.InnerDir * grip_hover_outer_size, corner + def.InnerDir * grip_hover_inner_size);
        if (resize_rect.Min.x > resize_rect.Max.x) ImSwap(resize_rect.Min.x, resize_rect.Max.x);
        if (resize_rect.Min.y > resize_rect.Max.y) ImSwap(resize_rect.Min.y, resize_rect.Max.y);
        ImGuiID resize_grip_id = window->GetID(resize_grip_n); // == GetWindowResizeCornerID()
        ItemAdd(resize_rect, resize_grip_id, NULL, ImGuiItemFlags_NoNav);
        ButtonBehavior(resize_rect, resize_grip_id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
        //GetForegroundDrawList(window)->AddRect(resize_rect.Min, resize_rect.Max, IM_COL32(255, 255, 0, 255));
        if (hovered || held)
            SetMouseCursor((resize_grip_n & 1) ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE);

        if (held && g.IO.MouseDoubleClicked[0])
        {
            // Auto-fit when double-clicking
            size_target = CalcWindowSizeAfterConstraint(window, size_auto_fit);
            ret_auto_fit_mask = 0x03; // Both axises
            ClearActiveID();
        }
        else if (held)
        {
            // Resize from any of the four corners
            // We don't use an incremental MouseDelta but rather compute an absolute target size based on mouse position
            ImVec2 clamp_min = ImVec2(def.CornerPosN.x == 1.0f ? clamp_rect.Min.x : -FLT_MAX, (def.CornerPosN.y == 1.0f || (def.CornerPosN.y == 0.0f && window_move_from_title_bar)) ? clamp_rect.Min.y : -FLT_MAX);
            ImVec2 clamp_max = ImVec2(def.CornerPosN.x == 0.0f ? clamp_rect.Max.x : +FLT_MAX, def.CornerPosN.y == 0.0f ? clamp_rect.Max.y : +FLT_MAX);
            ImVec2 corner_target = g.IO.MousePos - g.ActiveIdClickOffset + ImLerp(def.InnerDir * grip_hover_outer_size, def.InnerDir * -grip_hover_inner_size, def.CornerPosN); // Corner of the window corresponding to our corner grip
            corner_target = ImClamp(corner_target, clamp_min, clamp_max);
            CalcResizePosSizeFromAnyCorner(window, corner_target, def.CornerPosN, &pos_target, &size_target);
        }

        // Only lower-left grip is visible before hovering/activating
        if (resize_grip_n == 0 || held || hovered)
            resize_grip_col[resize_grip_n] = GetColorU32(held ? ImGuiCol_ResizeGripActive : hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip);
    }

    int resize_border_mask = 0x00;
    if (window->Flags & ImGuiWindowFlags_ChildWindow)
        resize_border_mask |= ((window->ChildFlags & ImGuiChildFlags_ResizeX) ? 0x02 : 0) | ((window->ChildFlags & ImGuiChildFlags_ResizeY) ? 0x08 : 0);
    else
        resize_border_mask = g.IO.ConfigWindowsResizeFromEdges ? 0x0F : 0x00;
    for (int border_n = 0; border_n < 4; border_n++)
    {
        if ((resize_border_mask & (1 << border_n)) == 0)
            continue;
        const ImGuiResizeBorderDef& def = resize_border_def[border_n];
        const ImGuiAxis axis = (border_n == ImGuiDir_Left || border_n == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;

        bool hovered, held;
        ImRect border_rect = GetResizeBorderRect(window, border_n, grip_hover_inner_size, WINDOWS_HOVER_PADDING);
        ImGuiID border_id = window->GetID(border_n + 4); // == GetWindowResizeBorderID()
        ItemAdd(border_rect, border_id, NULL, ImGuiItemFlags_NoNav);
        ButtonBehavior(border_rect, border_id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
        //GetForegroundDrawList(window)->AddRect(border_rect.Min, border_rect.Max, IM_COL32(255, 255, 0, 255));
        if (hovered && g.HoveredIdTimer <= WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER)
            hovered = false;
        if (hovered || held)
            SetMouseCursor((axis == ImGuiAxis_X) ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
        if (held && g.IO.MouseDoubleClicked[0])
        {
            // Double-clicking bottom or right border auto-fit on this axis
            // FIXME: CalcWindowAutoFitSize() doesn't take into account that only one side may be auto-fit when calculating scrollbars.
            // FIXME: Support top and right borders: rework CalcResizePosSizeFromAnyCorner() to be reusable in both cases.
            if (border_n == 1 || border_n == 3) // Right and bottom border
            {
                size_target[axis] = CalcWindowSizeAfterConstraint(window, size_auto_fit)[axis];
                ret_auto_fit_mask |= (1 << axis);
                hovered = held = false; // So border doesn't show highlighted at new position
            }
            ClearActiveID();
        }
        else if (held)
        {
            // Switch to relative resizing mode when border geometry moved (e.g. resizing a child altering parent scroll), in order to avoid resizing feedback loop.
            // Currently only using relative mode on resizable child windows, as the problem to solve is more likely noticeable for them, but could apply for all windows eventually.
            // FIXME: May want to generalize this idiom at lower-level, so more widgets can use it!
            const bool just_scrolled_manually_while_resizing = (g.WheelingWindow != NULL && g.WheelingWindowScrolledFrame == g.FrameCount && IsWindowChildOf(window, g.WheelingWindow, false));
            if (g.ActiveIdIsJustActivated || just_scrolled_manually_while_resizing)
            {
                g.WindowResizeBorderExpectedRect = border_rect;
                g.WindowResizeRelativeMode = false;
            }
            if ((window->Flags & ImGuiWindowFlags_ChildWindow) && memcmp(&g.WindowResizeBorderExpectedRect, &border_rect, sizeof(ImRect)) != 0)
                g.WindowResizeRelativeMode = true;

            const ImVec2 border_curr = (window->Pos + ImMin(def.SegmentN1, def.SegmentN2) * window->Size);
            const float border_target_rel_mode_for_axis = border_curr[axis] + g.IO.MouseDelta[axis];
            const float border_target_abs_mode_for_axis = g.IO.MousePos[axis] - g.ActiveIdClickOffset[axis] + WINDOWS_HOVER_PADDING; // Match ButtonBehavior() padding above.

            // Use absolute mode position
            ImVec2 border_target = window->Pos;
            border_target[axis] = border_target_abs_mode_for_axis;

            // Use relative mode target for child window, ignore resize when moving back toward the ideal absolute position.
            bool ignore_resize = false;
            if (g.WindowResizeRelativeMode)
            {
                //GetForegroundDrawList()->AddText(GetMainViewport()->WorkPos, IM_COL32_WHITE, "Relative Mode");
                border_target[axis] = border_target_rel_mode_for_axis;
                if (g.IO.MouseDelta[axis] == 0.0f || (g.IO.MouseDelta[axis] > 0.0f) == (border_target_rel_mode_for_axis > border_target_abs_mode_for_axis))
                    ignore_resize = true;
            }

            // Clamp, apply
            ImVec2 clamp_min(border_n == ImGuiDir_Right ? clamp_rect.Min.x : -FLT_MAX, border_n == ImGuiDir_Down || (border_n == ImGuiDir_Up && window_move_from_title_bar) ? clamp_rect.Min.y : -FLT_MAX);
            ImVec2 clamp_max(border_n == ImGuiDir_Left ? clamp_rect.Max.x : +FLT_MAX, border_n == ImGuiDir_Up ? clamp_rect.Max.y : +FLT_MAX);
            border_target = ImClamp(border_target, clamp_min, clamp_max);
            if (flags & ImGuiWindowFlags_ChildWindow) // Clamp resizing of childs within parent
            {
                ImGuiWindow* parent_window = window->ParentWindow;
                ImGuiWindowFlags parent_flags = parent_window->Flags;
                ImRect border_limit_rect = parent_window->InnerRect;
                border_limit_rect.Expand(ImVec2(-ImMax(parent_window->WindowPadding.x, parent_window->WindowBorderSize), -ImMax(parent_window->WindowPadding.y, parent_window->WindowBorderSize)));
                if ((axis == ImGuiAxis_X) && ((parent_flags & (ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar)) == 0 || (parent_flags & ImGuiWindowFlags_NoScrollbar)))
                    border_target.x = ImClamp(border_target.x, border_limit_rect.Min.x, border_limit_rect.Max.x);
                if ((axis == ImGuiAxis_Y) && (parent_flags & ImGuiWindowFlags_NoScrollbar))
                    border_target.y = ImClamp(border_target.y, border_limit_rect.Min.y, border_limit_rect.Max.y);
            }
            if (!ignore_resize)
                CalcResizePosSizeFromAnyCorner(window, border_target, ImMin(def.SegmentN1, def.SegmentN2), &pos_target, &size_target);
        }
        if (hovered)
            *border_hovered = border_n;
        if (held)
            *border_held = border_n;
    }
    PopID();

    // Restore nav layer
    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;

    // Navigation resize (keyboard/gamepad)
    // FIXME: This cannot be moved to NavUpdateWindowing() because CalcWindowSizeAfterConstraint() need to callback into user.
    // Not even sure the callback works here.
    if (g.NavWindowingTarget && g.NavWindowingTarget->RootWindow == window)
    {
        ImVec2 nav_resize_dir;
        if (g.NavInputSource == ImGuiInputSource_Keyboard && g.IO.KeyShift)
            nav_resize_dir = GetKeyMagnitude2d(ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_UpArrow, ImGuiKey_DownArrow);
        if (g.NavInputSource == ImGuiInputSource_Gamepad)
            nav_resize_dir = GetKeyMagnitude2d(ImGuiKey_GamepadDpadLeft, ImGuiKey_GamepadDpadRight, ImGuiKey_GamepadDpadUp, ImGuiKey_GamepadDpadDown);
        if (nav_resize_dir.x != 0.0f || nav_resize_dir.y != 0.0f)
        {
            const float NAV_RESIZE_SPEED = 600.0f;
            const float resize_step = NAV_RESIZE_SPEED * g.IO.DeltaTime * ImMin(g.IO.DisplayFramebufferScale.x, g.IO.DisplayFramebufferScale.y);
            g.NavWindowingAccumDeltaSize += nav_resize_dir * resize_step;
            g.NavWindowingAccumDeltaSize = ImMax(g.NavWindowingAccumDeltaSize, clamp_rect.Min - window->Pos - window->Size); // We need Pos+Size >= clmap_rect.Min, so Size >= clmap_rect.Min - Pos, so size_delta >= clmap_rect.Min - window->Pos - window->Size
            g.NavWindowingToggleLayer = false;
            g.NavDisableMouseHover = true;
            resize_grip_col[0] = GetColorU32(ImGuiCol_ResizeGripActive);
            ImVec2 accum_floored = ImTrunc(g.NavWindowingAccumDeltaSize);
            if (accum_floored.x != 0.0f || accum_floored.y != 0.0f)
            {
                // FIXME-NAV: Should store and accumulate into a separate size buffer to handle sizing constraints properly, right now a constraint will make us stuck.
                size_target = CalcWindowSizeAfterConstraint(window, window->SizeFull + accum_floored);
                g.NavWindowingAccumDeltaSize -= accum_floored;
            }
        }
    }

    // Apply back modified position/size to window
    const ImVec2 curr_pos = window->Pos;
    const ImVec2 curr_size = window->SizeFull;
    if (size_target.x != FLT_MAX && (window->Size.x != size_target.x || window->SizeFull.x != size_target.x))
        window->Size.x = window->SizeFull.x = size_target.x;
    if (size_target.y != FLT_MAX && (window->Size.y != size_target.y || window->SizeFull.y != size_target.y))
        window->Size.y = window->SizeFull.y = size_target.y;
    if (pos_target.x != FLT_MAX && window->Pos.x != ImTrunc(pos_target.x))
        window->Pos.x = ImTrunc(pos_target.x);
    if (pos_target.y != FLT_MAX && window->Pos.y != ImTrunc(pos_target.y))
        window->Pos.y = ImTrunc(pos_target.y);
    if (curr_pos.x != window->Pos.x || curr_pos.y != window->Pos.y || curr_size.x != window->SizeFull.x || curr_size.y != window->SizeFull.y)
        MarkIniSettingsDirty(window);

    // Recalculate next expected border expected coordinates
    if (*border_held != -1)
        g.WindowResizeBorderExpectedRect = GetResizeBorderRect(window, *border_held, grip_hover_inner_size, WINDOWS_HOVER_PADDING);

    return ret_auto_fit_mask;
}

static inline void ClampWindowPos(ImGuiWindow* window, const ImRect& visibility_rect)
{
    ImGuiContext& g = *GImGui;
    ImVec2 size_for_clamping = window->Size;
    if (g.IO.ConfigWindowsMoveFromTitleBarOnly && !(window->Flags & ImGuiWindowFlags_NoTitleBar))
        size_for_clamping.y = window->TitleBarHeight;
    window->Pos = ImClamp(window->Pos, visibility_rect.Min - size_for_clamping, visibility_rect.Max);
}

static void RenderWindowOuterSingleBorder(ImGuiWindow* window, int border_n, ImU32 border_col, float border_size)
{
    const ImGuiResizeBorderDef& def = resize_border_def[border_n];
    const float rounding = window->WindowRounding;
    const ImRect border_r = GetResizeBorderRect(window, border_n, rounding, 0.0f);
    window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.SegmentN1) + ImVec2(0.5f, 0.5f) + def.InnerDir * rounding, rounding, def.OuterAngle - IM_PI * 0.25f, def.OuterAngle);
    window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.SegmentN2) + ImVec2(0.5f, 0.5f) + def.InnerDir * rounding, rounding, def.OuterAngle, def.OuterAngle + IM_PI * 0.25f);
    window->DrawList->PathStroke(border_col, ImDrawFlags_None, border_size);
}

static void ImGui::RenderWindowOuterBorders(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    const float border_size = window->WindowBorderSize;
    const ImU32 border_col = GetColorU32(ImGuiCol_Border);
    if (border_size > 0.0f && (window->Flags & ImGuiWindowFlags_NoBackground) == 0)
        window->DrawList->AddRect(window->Pos, window->Pos + window->Size, border_col, window->WindowRounding, 0, window->WindowBorderSize);
    else if (border_size > 0.0f)
    {
        if (window->ChildFlags & ImGuiChildFlags_ResizeX) // Similar code as 'resize_border_mask' computation in UpdateWindowManualResize() but we specifically only always draw explicit child resize border.
            RenderWindowOuterSingleBorder(window, 1, border_col, border_size);
        if (window->ChildFlags & ImGuiChildFlags_ResizeY)
            RenderWindowOuterSingleBorder(window, 3, border_col, border_size);
    }
    if (window->ResizeBorderHovered != -1 || window->ResizeBorderHeld != -1)
    {
        const int border_n = (window->ResizeBorderHeld != -1) ? window->ResizeBorderHeld : window->ResizeBorderHovered;
        const ImU32 border_col_resizing = GetColorU32((window->ResizeBorderHeld != -1) ? ImGuiCol_SeparatorActive : ImGuiCol_SeparatorHovered);
        RenderWindowOuterSingleBorder(window, border_n, border_col_resizing, ImMax(2.0f, window->WindowBorderSize)); // Thicker than usual
    }
    if (g.Style.FrameBorderSize > 0 && !(window->Flags & ImGuiWindowFlags_NoTitleBar))
    {
        float y = window->Pos.y + window->TitleBarHeight - 1;
        window->DrawList->AddLine(ImVec2(window->Pos.x + border_size, y), ImVec2(window->Pos.x + window->Size.x - border_size, y), border_col, g.Style.FrameBorderSize);
    }
}

// Draw background and borders
// Draw and handle scrollbars
void ImGui::RenderWindowDecorations(ImGuiWindow* window, const ImRect& title_bar_rect, bool title_bar_is_highlight, bool handle_borders_and_resize_grips, int resize_grip_count, const ImU32 resize_grip_col[4], float resize_grip_draw_size)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiWindowFlags flags = window->Flags;

    // Ensure that ScrollBar doesn't read last frame's SkipItems
    IM_ASSERT(window->BeginCount == 0);
    window->SkipItems = false;

    // Draw window + handle manual resize
    // As we highlight the title bar when want_focus is set, multiple reappearing windows will have their title bar highlighted on their reappearing frame.
    const float window_rounding = window->WindowRounding;
    const float window_border_size = window->WindowBorderSize;
    if (window->Collapsed)
    {
        // Title bar only
        const float backup_border_size = style.FrameBorderSize;
        g.Style.FrameBorderSize = window->WindowBorderSize;
        ImU32 title_bar_col = GetColorU32((title_bar_is_highlight && !g.NavDisableHighlight) ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBgCollapsed);
        RenderFrame(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, true, window_rounding);
        g.Style.FrameBorderSize = backup_border_size;
    }
    else
    {
        // Window background
        if (!(flags & ImGuiWindowFlags_NoBackground))
        {
            ImU32 bg_col = GetColorU32(GetWindowBgColorIdx(window));
            bool override_alpha = false;
            float alpha = 1.0f;
            if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasBgAlpha)
            {
                alpha = g.NextWindowData.BgAlphaVal;
                override_alpha = true;
            }
            if (override_alpha)
                bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
            window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? 0 : ImDrawFlags_RoundCornersBottom);
        }

        // Title bar
        if (!(flags & ImGuiWindowFlags_NoTitleBar))
        {
            ImU32 title_bar_col = GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
            window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, window_rounding, ImDrawFlags_RoundCornersTop);
        }

        // Menu bar
        if (flags & ImGuiWindowFlags_MenuBar)
        {
            ImRect menu_bar_rect = window->MenuBarRect();
            menu_bar_rect.ClipWith(window->Rect());  // Soft clipping, in particular child window don't have minimum size covering the menu bar so this is useful for them.
            window->DrawList->AddRectFilled(menu_bar_rect.Min + ImVec2(window_border_size, 0), menu_bar_rect.Max - ImVec2(window_border_size, 0), GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImDrawFlags_RoundCornersTop);
            if (style.FrameBorderSize > 0.0f && menu_bar_rect.Max.y < window->Pos.y + window->Size.y)
                window->DrawList->AddLine(menu_bar_rect.GetBL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
        }

        // Scrollbars
        if (window->ScrollbarX)
            Scrollbar(ImGuiAxis_X);
        if (window->ScrollbarY)
            Scrollbar(ImGuiAxis_Y);

        // Render resize grips (after their input handling so we don't have a frame of latency)
        if (handle_borders_and_resize_grips && !(flags & ImGuiWindowFlags_NoResize))
        {
            for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
            {
                const ImU32 col = resize_grip_col[resize_grip_n];
                if ((col & IM_COL32_A_MASK) == 0)
                    continue;
                const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
                const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPosN);
                window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(window_border_size, resize_grip_draw_size) : ImVec2(resize_grip_draw_size, window_border_size)));
                window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(resize_grip_draw_size, window_border_size) : ImVec2(window_border_size, resize_grip_draw_size)));
                window->DrawList->PathArcToFast(ImVec2(corner.x + grip.InnerDir.x * (window_rounding + window_border_size), corner.y + grip.InnerDir.y * (window_rounding + window_border_size)), window_rounding, grip.AngleMin12, grip.AngleMax12);
                window->DrawList->PathFillConvex(col);
            }
        }

        // Borders
        if (handle_borders_and_resize_grips)
            RenderWindowOuterBorders(window);
    }
}

// Render title text, collapse button, close button
void ImGui::RenderWindowTitleBarContents(ImGuiWindow* window, const ImRect& title_bar_rect, const char* name, bool* p_open)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiWindowFlags flags = window->Flags;

    const bool has_close_button = (p_open != NULL);
    const bool has_collapse_button = !(flags & ImGuiWindowFlags_NoCollapse) && (style.WindowMenuButtonPosition != ImGuiDir_None);

    // Close & Collapse button are on the Menu NavLayer and don't default focus (unless there's nothing else on that layer)
    // FIXME-NAV: Might want (or not?) to set the equivalent of ImGuiButtonFlags_NoNavFocus so that mouse clicks on standard title bar items don't necessarily set nav/keyboard ref?
    const ImGuiItemFlags item_flags_backup = g.CurrentItemFlags;
    g.CurrentItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

    // Layout buttons
    // FIXME: Would be nice to generalize the subtleties expressed here into reusable code.
    float pad_l = style.FramePadding.x;
    float pad_r = style.FramePadding.x;
    float button_sz = g.FontSize;
    ImVec2 close_button_pos;
    ImVec2 collapse_button_pos;
    if (has_close_button)
    {
        close_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - button_sz, title_bar_rect.Min.y + style.FramePadding.y);
        pad_r += button_sz + style.ItemInnerSpacing.x;
    }
    if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Right)
    {
        collapse_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - button_sz, title_bar_rect.Min.y + style.FramePadding.y);
        pad_r += button_sz + style.ItemInnerSpacing.x;
    }
    if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Left)
    {
        collapse_button_pos = ImVec2(title_bar_rect.Min.x + pad_l, title_bar_rect.Min.y + style.FramePadding.y);
        pad_l += button_sz + style.ItemInnerSpacing.x;
    }

    // Collapse button (submitting first so it gets priority when choosing a navigation init fallback)
    if (has_collapse_button)
        if (CollapseButton(window->GetID("#COLLAPSE"), collapse_button_pos))
            window->WantCollapseToggle = true; // Defer actual collapsing to next frame as we are too far in the Begin() function

    // Close button
    if (has_close_button)
        if (CloseButton(window->GetID("#CLOSE"), close_button_pos))
            *p_open = false;

    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
    g.CurrentItemFlags = item_flags_backup;

    // Title bar text (with: horizontal alignment, avoiding collapse/close button, optional "unsaved document" marker)
    // FIXME: Refactor text alignment facilities along with RenderText helpers, this is WAY too much messy code..
    const float marker_size_x = (flags & ImGuiWindowFlags_UnsavedDocument) ? button_sz * 0.80f : 0.0f;
    const ImVec2 text_size = CalcTextSize(name, NULL, true) + ImVec2(marker_size_x, 0.0f);

    // As a nice touch we try to ensure that centered title text doesn't get affected by visibility of Close/Collapse button,
    // while uncentered title text will still reach edges correctly.
    if (pad_l > style.FramePadding.x)
        pad_l += g.Style.ItemInnerSpacing.x;
    if (pad_r > style.FramePadding.x)
        pad_r += g.Style.ItemInnerSpacing.x;
    if (style.WindowTitleAlign.x > 0.0f && style.WindowTitleAlign.x < 1.0f)
    {
        float centerness = ImSaturate(1.0f - ImFabs(style.WindowTitleAlign.x - 0.5f) * 2.0f); // 0.0f on either edges, 1.0f on center
        float pad_extend = ImMin(ImMax(pad_l, pad_r), title_bar_rect.GetWidth() - pad_l - pad_r - text_size.x);
        pad_l = ImMax(pad_l, pad_extend * centerness);
        pad_r = ImMax(pad_r, pad_extend * centerness);
    }

    ImRect layout_r(title_bar_rect.Min.x + pad_l, title_bar_rect.Min.y, title_bar_rect.Max.x - pad_r, title_bar_rect.Max.y);
    ImRect clip_r(layout_r.Min.x, layout_r.Min.y, ImMin(layout_r.Max.x + g.Style.ItemInnerSpacing.x, title_bar_rect.Max.x), layout_r.Max.y);
    if (flags & ImGuiWindowFlags_UnsavedDocument)
    {
        ImVec2 marker_pos;
        marker_pos.x = ImClamp(layout_r.Min.x + (layout_r.GetWidth() - text_size.x) * style.WindowTitleAlign.x + text_size.x, layout_r.Min.x, layout_r.Max.x);
        marker_pos.y = (layout_r.Min.y + layout_r.Max.y) * 0.5f;
        if (marker_pos.x > layout_r.Min.x)
        {
            RenderBullet(window->DrawList, marker_pos, GetColorU32(ImGuiCol_Text));
            clip_r.Max.x = ImMin(clip_r.Max.x, marker_pos.x - (int)(marker_size_x * 0.5f));
        }
    }
    //if (g.IO.KeyShift) window->DrawList->AddRect(layout_r.Min, layout_r.Max, IM_COL32(255, 128, 0, 255)); // [DEBUG]
    //if (g.IO.KeyCtrl) window->DrawList->AddRect(clip_r.Min, clip_r.Max, IM_COL32(255, 128, 0, 255)); // [DEBUG]
    RenderTextClipped(layout_r.Min, layout_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_r);
}

void ImGui::UpdateWindowParentAndRootLinks(ImGuiWindow* window, ImGuiWindowFlags flags, ImGuiWindow* parent_window)
{
    window->ParentWindow = parent_window;
    window->RootWindow = window->RootWindowPopupTree = window->RootWindowForTitleBarHighlight = window->RootWindowForNav = window;
    if (parent_window && (flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Tooltip))
        window->RootWindow = parent_window->RootWindow;
    if (parent_window && (flags & ImGuiWindowFlags_Popup))
        window->RootWindowPopupTree = parent_window->RootWindowPopupTree;
    if (parent_window && !(flags & ImGuiWindowFlags_Modal) && (flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)))
        window->RootWindowForTitleBarHighlight = parent_window->RootWindowForTitleBarHighlight;
    while (window->RootWindowForNav->ChildFlags & ImGuiChildFlags_NavFlattened)
    {
        IM_ASSERT(window->RootWindowForNav->ParentWindow != NULL);
        window->RootWindowForNav = window->RootWindowForNav->ParentWindow;
    }
}

// [EXPERIMENTAL] Called by Begin(). NextWindowData is valid at this point.
// This is designed as a toy/test-bed for
void ImGui::UpdateWindowSkipRefresh(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    window->SkipRefresh = false;
    if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasRefreshPolicy) == 0)
        return;
    if (g.NextWindowData.RefreshFlagsVal & ImGuiWindowRefreshFlags_TryToAvoidRefresh)
    {
        // FIXME-IDLE: Tests for e.g. mouse clicks or keyboard while focused.
        if (window->Appearing) // If currently appearing
            return;
        if (window->Hidden) // If was hidden (previous frame)
            return;
        if ((g.NextWindowData.RefreshFlagsVal & ImGuiWindowRefreshFlags_RefreshOnHover) && g.HoveredWindow)
            if (window->RootWindow == g.HoveredWindow->RootWindow || IsWindowWithinBeginStackOf(g.HoveredWindow->RootWindow, window))
                return;
        if ((g.NextWindowData.RefreshFlagsVal & ImGuiWindowRefreshFlags_RefreshOnFocus) && g.NavWindow)
            if (window->RootWindow == g.NavWindow->RootWindow || IsWindowWithinBeginStackOf(g.NavWindow->RootWindow, window))
                return;
        window->DrawList = NULL;
        window->SkipRefresh = true;
    }
}

static void SetWindowActiveForSkipRefresh(ImGuiWindow* window)
{
    window->Active = true;
    for (ImGuiWindow* child : window->DC.ChildWindows)
        if (!child->Hidden)
        {
            child->Active = child->SkipRefresh = true;
            SetWindowActiveForSkipRefresh(child);
        }
}

// When a modal popup is open, newly created windows that want focus (i.e. are not popups and do not specify ImGuiWindowFlags_NoFocusOnAppearing)
// should be positioned behind that modal window, unless the window was created inside the modal begin-stack.
// In case of multiple stacked modals newly created window honors begin stack order and does not go below its own modal parent.
// - WindowA            // FindBlockingModal() returns Modal1
//   - WindowB          //                  .. returns Modal1
//   - Modal1           //                  .. returns Modal2
//      - WindowC       //                  .. returns Modal2
//          - WindowD   //                  .. returns Modal2
//          - Modal2    //                  .. returns Modal2
//            - WindowE //                  .. returns NULL
// Notes:
// - FindBlockingModal(NULL) == NULL is generally equivalent to GetTopMostPopupModal() == NULL.
//   Only difference is here we check for ->Active/WasActive but it may be unnecessary.
ImGuiWindow* ImGui::FindBlockingModal(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.Size <= 0)
        return NULL;

    // Find a modal that has common parent with specified window. Specified window should be positioned behind that modal.
    for (ImGuiPopupData& popup_data : g.OpenPopupStack)
    {
        ImGuiWindow* popup_window = popup_data.Window;
        if (popup_window == NULL || !(popup_window->Flags & ImGuiWindowFlags_Modal))
            continue;
        if (!popup_window->Active && !popup_window->WasActive)      // Check WasActive, because this code may run before popup renders on current frame, also check Active to handle newly created windows.
            continue;
        if (window == NULL)                                         // FindBlockingModal(NULL) test for if FocusWindow(NULL) is naturally possible via a mouse click.
            return popup_window;
        if (IsWindowWithinBeginStackOf(window, popup_window))       // Window may be over modal
            continue;
        return popup_window;                                        // Place window right below first block modal
    }
    return NULL;
}

// Push a new Dear ImGui window to add widgets to.
// - A default window called "Debug" is automatically stacked at the beginning of every frame so you can use widgets without explicitly calling a Begin/End pair.
// - Begin/End can be called multiple times during the frame with the same window name to append content.
// - The window name is used as a unique identifier to preserve window information across frames (and save rudimentary information to the .ini file).
//   You can use the "##" or "###" markers to use the same label with different id, or same id with different label. See documentation at the top of this file.
// - Return false when window is collapsed, so you can early out in your code. You always need to call ImGui::End() even if false is returned.
// - Passing 'bool* p_open' displays a Close button on the upper-right corner of the window, the pointed value will be set to false when the button is pressed.
bool ImGui::Begin(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    IM_ASSERT(name != NULL && name[0] != '\0');     // Window name required
    IM_ASSERT(g.WithinFrameScope);                  // Forgot to call ImGui::NewFrame()
    IM_ASSERT(g.FrameCountEnded != g.FrameCount);   // Called ImGui::Render() or ImGui::EndFrame() and haven't called ImGui::NewFrame() again yet

    // Find or create
    ImGuiWindow* window = FindWindowByName(name);
    const bool window_just_created = (window == NULL);
    if (window_just_created)
        window = CreateNewWindow(name, flags);

    // [DEBUG] Debug break requested by user
    if (g.DebugBreakInWindow == window->ID)
        IM_DEBUG_BREAK();

    // Automatically disable manual moving/resizing when NoInputs is set
    if ((flags & ImGuiWindowFlags_NoInputs) == ImGuiWindowFlags_NoInputs)
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    const int current_frame = g.FrameCount;
    const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);
    window->IsFallbackWindow = (g.CurrentWindowStack.Size == 0 && g.WithinFrameScopeWithImplicitWindow);

    // Update the Appearing flag
    bool window_just_activated_by_user = (window->LastFrameActive < current_frame - 1);   // Not using !WasActive because the implicit "Debug" window would always toggle off->on
    if (flags & ImGuiWindowFlags_Popup)
    {
        ImGuiPopupData& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
        window_just_activated_by_user |= (window->PopupId != popup_ref.PopupId); // We recycle popups so treat window as activated if popup id changed
        window_just_activated_by_user |= (window != popup_ref.Window);
    }
    window->Appearing = window_just_activated_by_user;
    if (window->Appearing)
        SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, true);

    // Update Flags, LastFrameActive, BeginOrderXXX fields
    if (first_begin_of_the_frame)
    {
        UpdateWindowInFocusOrderList(window, window_just_created, flags);
        window->Flags = (ImGuiWindowFlags)flags;
        window->ChildFlags = (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasChildFlags) ? g.NextWindowData.ChildFlags : 0;
        window->LastFrameActive = current_frame;
        window->LastTimeActive = (float)g.Time;
        window->BeginOrderWithinParent = 0;
        window->BeginOrderWithinContext = (short)(g.WindowsActiveCount++);
    }
    else
    {
        flags = window->Flags;
    }

    // Parent window is latched only on the first call to Begin() of the frame, so further append-calls can be done from a different window stack
    ImGuiWindow* parent_window_in_stack = g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back().Window;
    ImGuiWindow* parent_window = first_begin_of_the_frame ? ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)) ? parent_window_in_stack : NULL) : window->ParentWindow;
    IM_ASSERT(parent_window != NULL || !(flags & ImGuiWindowFlags_ChildWindow));

    // We allow window memory to be compacted so recreate the base stack when needed.
    if (window->IDStack.Size == 0)
        window->IDStack.push_back(window->ID);

    // Add to stack
    g.CurrentWindow = window;
    g.CurrentWindowStack.resize(g.CurrentWindowStack.Size + 1);
    ImGuiWindowStackData& window_stack_data = g.CurrentWindowStack.back();
    window_stack_data.Window = window;
    window_stack_data.ParentLastItemDataBackup = g.LastItemData;
    window_stack_data.DisabledOverrideReenable = (flags & ImGuiWindowFlags_Tooltip) && (g.CurrentItemFlags & ImGuiItemFlags_Disabled);
    ErrorRecoveryStoreState(&window_stack_data.StackSizesInBegin);
    g.StackSizesInBeginForCurrentWindow = &window_stack_data.StackSizesInBegin;
    if (flags & ImGuiWindowFlags_ChildMenu)
        g.BeginMenuDepth++;

    // Update ->RootWindow and others pointers (before any possible call to FocusWindow)
    if (first_begin_of_the_frame)
    {
        UpdateWindowParentAndRootLinks(window, flags, parent_window);
        window->ParentWindowInBeginStack = parent_window_in_stack;

        // There's little point to expose a flag to set this: because the interesting cases won't be using parent_window_in_stack,
        // e.g. linking a tool window in a standalone viewport to a document window, regardless of their Begin() stack parenting. (#6798)
        window->ParentWindowForFocusRoute = (flags & ImGuiWindowFlags_ChildWindow) ? parent_window_in_stack : NULL;
    }

    // Add to focus scope stack
    PushFocusScope((window->ChildFlags & ImGuiChildFlags_NavFlattened) ? g.CurrentFocusScopeId : window->ID);
    window->NavRootFocusScopeId = g.CurrentFocusScopeId;

    // Add to popup stacks: update OpenPopupStack[] data, push to BeginPopupStack[]
    if (flags & ImGuiWindowFlags_Popup)
    {
        ImGuiPopupData& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
        popup_ref.Window = window;
        popup_ref.ParentNavLayer = parent_window_in_stack->DC.NavLayerCurrent;
        g.BeginPopupStack.push_back(popup_ref);
        window->PopupId = popup_ref.PopupId;
    }

    // Process SetNextWindow***() calls
    // (FIXME: Consider splitting the HasXXX flags into X/Y components
    bool window_pos_set_by_api = false;
    bool window_size_x_set_by_api = false, window_size_y_set_by_api = false;
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos)
    {
        window_pos_set_by_api = (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) != 0;
        if (window_pos_set_by_api && ImLengthSqr(g.NextWindowData.PosPivotVal) > 0.00001f)
        {
            // May be processed on the next frame if this is our first frame and we are measuring size
            // FIXME: Look into removing the branch so everything can go through this same code path for consistency.
            window->SetWindowPosVal = g.NextWindowData.PosVal;
            window->SetWindowPosPivot = g.NextWindowData.PosPivotVal;
            window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
        }
        else
        {
            SetWindowPos(window, g.NextWindowData.PosVal, g.NextWindowData.PosCond);
        }
    }
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize)
    {
        window_size_x_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.x > 0.0f);
        window_size_y_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.y > 0.0f);
        if ((window->ChildFlags & ImGuiChildFlags_ResizeX) && (window->SetWindowSizeAllowFlags & ImGuiCond_FirstUseEver) == 0) // Axis-specific conditions for BeginChild()
            g.NextWindowData.SizeVal.x = window->SizeFull.x;
        if ((window->ChildFlags & ImGuiChildFlags_ResizeY) && (window->SetWindowSizeAllowFlags & ImGuiCond_FirstUseEver) == 0)
            g.NextWindowData.SizeVal.y = window->SizeFull.y;
        SetWindowSize(window, g.NextWindowData.SizeVal, g.NextWindowData.SizeCond);
    }
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasScroll)
    {
        if (g.NextWindowData.ScrollVal.x >= 0.0f)
        {
            window->ScrollTarget.x = g.NextWindowData.ScrollVal.x;
            window->ScrollTargetCenterRatio.x = 0.0f;
        }
        if (g.NextWindowData.ScrollVal.y >= 0.0f)
        {
            window->ScrollTarget.y = g.NextWindowData.ScrollVal.y;
            window->ScrollTargetCenterRatio.y = 0.0f;
        }
    }
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasContentSize)
        window->ContentSizeExplicit = g.NextWindowData.ContentSizeVal;
    else if (first_begin_of_the_frame)
        window->ContentSizeExplicit = ImVec2(0.0f, 0.0f);
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasCollapsed)
        SetWindowCollapsed(window, g.NextWindowData.CollapsedVal, g.NextWindowData.CollapsedCond);
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasFocus)
        FocusWindow(window);
    if (window->Appearing)
        SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, false);

    // [EXPERIMENTAL] Skip Refresh mode
    UpdateWindowSkipRefresh(window);

    // Nested root windows (typically tooltips) override disabled state
    if (window_stack_data.DisabledOverrideReenable && window->RootWindow == window)
        BeginDisabledOverrideReenable();

    // We intentionally set g.CurrentWindow to NULL to prevent usage until when the viewport is set, then will call SetCurrentWindow()
    g.CurrentWindow = NULL;

    // When reusing window again multiple times a frame, just append content (don't need to setup again)
    if (first_begin_of_the_frame && !window->SkipRefresh)
    {
        // Initialize
        const bool window_is_child_tooltip = (flags & ImGuiWindowFlags_ChildWindow) && (flags & ImGuiWindowFlags_Tooltip); // FIXME-WIP: Undocumented behavior of Child+Tooltip for pinned tooltip (#1345)
        const bool window_just_appearing_after_hidden_for_resize = (window->HiddenFramesCannotSkipItems > 0);
        window->Active = true;
        window->HasCloseButton = (p_open != NULL);
        window->ClipRect = ImVec4(-FLT_MAX, -FLT_MAX, +FLT_MAX, +FLT_MAX);
        window->IDStack.resize(1);
        window->DrawList->_ResetForNewFrame();
        window->DC.CurrentTableIdx = -1;

        // Restore buffer capacity when woken from a compacted state, to avoid
        if (window->MemoryCompacted)
            GcAwakeTransientWindowBuffers(window);

        // Update stored window name when it changes (which can _only_ happen with the "###" operator, so the ID would stay unchanged).
        // The title bar always display the 'name' parameter, so we only update the string storage if it needs to be visible to the end-user elsewhere.
        bool window_title_visible_elsewhere = false;
        if (g.NavWindowingListWindow != NULL && (window->Flags & ImGuiWindowFlags_NoNavFocus) == 0)   // Window titles visible when using CTRL+TAB
            window_title_visible_elsewhere = true;
        if (window_title_visible_elsewhere && !window_just_created && strcmp(name, window->Name) != 0)
        {
            size_t buf_len = (size_t)window->NameBufLen;
            window->Name = ImStrdupcpy(window->Name, &buf_len, name);
            window->NameBufLen = (int)buf_len;
        }

        // UPDATE CONTENTS SIZE, UPDATE HIDDEN STATUS

        // Update contents size from last frame for auto-fitting (or use explicit size)
        CalcWindowContentSizes(window, &window->ContentSize, &window->ContentSizeIdeal);
        if (window->HiddenFramesCanSkipItems > 0)
            window->HiddenFramesCanSkipItems--;
        if (window->HiddenFramesCannotSkipItems > 0)
            window->HiddenFramesCannotSkipItems--;
        if (window->HiddenFramesForRenderOnly > 0)
            window->HiddenFramesForRenderOnly--;

        // Hide new windows for one frame until they calculate their size
        if (window_just_created && (!window_size_x_set_by_api || !window_size_y_set_by_api))
            window->HiddenFramesCannotSkipItems = 1;

        // Hide popup/tooltip window when re-opening while we measure size (because we recycle the windows)
        // We reset Size/ContentSize for reappearing popups/tooltips early in this function, so further code won't be tempted to use the old size.
        if (window_just_activated_by_user && (flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0)
        {
            window->HiddenFramesCannotSkipItems = 1;
            if (flags & ImGuiWindowFlags_AlwaysAutoResize)
            {
                if (!window_size_x_set_by_api)
                    window->Size.x = window->SizeFull.x = 0.f;
                if (!window_size_y_set_by_api)
                    window->Size.y = window->SizeFull.y = 0.f;
                window->ContentSize = window->ContentSizeIdeal = ImVec2(0.f, 0.f);
            }
        }

        // SELECT VIEWPORT
        // FIXME-VIEWPORT: In the docking/viewport branch, this is the point where we select the current viewport (which may affect the style)

        ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)GetMainViewport();
        SetWindowViewport(window, viewport);
        SetCurrentWindow(window);

        // LOCK BORDER SIZE AND PADDING FOR THE FRAME (so that altering them doesn't cause inconsistencies)

        if (flags & ImGuiWindowFlags_ChildWindow)
            window->WindowBorderSize = style.ChildBorderSize;
        else
            window->WindowBorderSize = ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupBorderSize : style.WindowBorderSize;
        window->WindowPadding = style.WindowPadding;
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !(window->ChildFlags & ImGuiChildFlags_AlwaysUseWindowPadding) && window->WindowBorderSize == 0.0f)
            window->WindowPadding = ImVec2(0.0f, (flags & ImGuiWindowFlags_MenuBar) ? style.WindowPadding.y : 0.0f);

        // Lock menu offset so size calculation can use it as menu-bar windows need a minimum size.
        window->DC.MenuBarOffset.x = ImMax(ImMax(window->WindowPadding.x, style.ItemSpacing.x), g.NextWindowData.MenuBarOffsetMinVal.x);
        window->DC.MenuBarOffset.y = g.NextWindowData.MenuBarOffsetMinVal.y;
        window->TitleBarHeight = (flags & ImGuiWindowFlags_NoTitleBar) ? 0.0f : g.FontSize + g.Style.FramePadding.y * 2.0f;
        window->MenuBarHeight = (flags & ImGuiWindowFlags_MenuBar) ? window->DC.MenuBarOffset.y + g.FontSize + g.Style.FramePadding.y * 2.0f : 0.0f;

        // Depending on condition we use previous or current window size to compare against contents size to decide if a scrollbar should be visible.
        // Those flags will be altered further down in the function depending on more conditions.
        bool use_current_size_for_scrollbar_x = window_just_created;
        bool use_current_size_for_scrollbar_y = window_just_created;
        if (window_size_x_set_by_api && window->ContentSizeExplicit.x != 0.0f)
            use_current_size_for_scrollbar_x = true;
        if (window_size_y_set_by_api && window->ContentSizeExplicit.y != 0.0f) // #7252
            use_current_size_for_scrollbar_y = true;

        // Collapse window by double-clicking on title bar
        // At this point we don't have a clipping rectangle setup yet, so we can use the title bar area for hit detection and drawing
        if (!(flags & ImGuiWindowFlags_NoTitleBar) && !(flags & ImGuiWindowFlags_NoCollapse))
        {
            // We don't use a regular button+id to test for double-click on title bar (mostly due to legacy reason, could be fixed),
            // so verify that we don't have items over the title bar.
            ImRect title_bar_rect = window->TitleBarRect();
            if (g.HoveredWindow == window && g.HoveredId == 0 && g.HoveredIdPreviousFrame == 0 && g.ActiveId == 0 && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max))
                if (g.IO.MouseClickedCount[0] == 2 && GetKeyOwner(ImGuiKey_MouseLeft) == ImGuiKeyOwner_NoOwner)
                    window->WantCollapseToggle = true;
            if (window->WantCollapseToggle)
            {
                window->Collapsed = !window->Collapsed;
                if (!window->Collapsed)
                    use_current_size_for_scrollbar_y = true;
                MarkIniSettingsDirty(window);
            }
        }
        else
        {
            window->Collapsed = false;
        }
        window->WantCollapseToggle = false;

        // SIZE

        // Outer Decoration Sizes
        // (we need to clear ScrollbarSize immediately as CalcWindowAutoFitSize() needs it and can be called from other locations).
        const ImVec2 scrollbar_sizes_from_last_frame = window->ScrollbarSizes;
        window->DecoOuterSizeX1 = 0.0f;
        window->DecoOuterSizeX2 = 0.0f;
        window->DecoOuterSizeY1 = window->TitleBarHeight + window->MenuBarHeight;
        window->DecoOuterSizeY2 = 0.0f;
        window->ScrollbarSizes = ImVec2(0.0f, 0.0f);

        // Calculate auto-fit size, handle automatic resize
        const ImVec2 size_auto_fit = CalcWindowAutoFitSize(window, window->ContentSizeIdeal);
        if ((flags & ImGuiWindowFlags_AlwaysAutoResize) && !window->Collapsed)
        {
            // Using SetNextWindowSize() overrides ImGuiWindowFlags_AlwaysAutoResize, so it can be used on tooltips/popups, etc.
            if (!window_size_x_set_by_api)
            {
                window->SizeFull.x = size_auto_fit.x;
                use_current_size_for_scrollbar_x = true;
            }
            if (!window_size_y_set_by_api)
            {
                window->SizeFull.y = size_auto_fit.y;
                use_current_size_for_scrollbar_y = true;
            }
        }
        else if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
        {
            // Auto-fit may only grow window during the first few frames
            // We still process initial auto-fit on collapsed windows to get a window width, but otherwise don't honor ImGuiWindowFlags_AlwaysAutoResize when collapsed.
            if (!window_size_x_set_by_api && window->AutoFitFramesX > 0)
            {
                window->SizeFull.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
                use_current_size_for_scrollbar_x = true;
            }
            if (!window_size_y_set_by_api && window->AutoFitFramesY > 0)
            {
                window->SizeFull.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
                use_current_size_for_scrollbar_y = true;
            }
            if (!window->Collapsed)
                MarkIniSettingsDirty(window);
        }

        // Apply minimum/maximum window size constraints and final size
        window->SizeFull = CalcWindowSizeAfterConstraint(window, window->SizeFull);
        window->Size = window->Collapsed && !(flags & ImGuiWindowFlags_ChildWindow) ? window->TitleBarRect().GetSize() : window->SizeFull;

        // POSITION

        // Popup latch its initial position, will position itself when it appears next frame
        if (window_just_activated_by_user)
        {
            window->AutoPosLastDirection = ImGuiDir_None;
            if ((flags & ImGuiWindowFlags_Popup) != 0 && !(flags & ImGuiWindowFlags_Modal) && !window_pos_set_by_api) // FIXME: BeginPopup() could use SetNextWindowPos()
                window->Pos = g.BeginPopupStack.back().OpenPopupPos;
        }

        // Position child window
        if (flags & ImGuiWindowFlags_ChildWindow)
        {
            IM_ASSERT(parent_window && parent_window->Active);
            window->BeginOrderWithinParent = (short)parent_window->DC.ChildWindows.Size;
            parent_window->DC.ChildWindows.push_back(window);
            if (!(flags & ImGuiWindowFlags_Popup) && !window_pos_set_by_api && !window_is_child_tooltip)
                window->Pos = parent_window->DC.CursorPos;
        }

        const bool window_pos_with_pivot = (window->SetWindowPosVal.x != FLT_MAX && window->HiddenFramesCannotSkipItems == 0);
        if (window_pos_with_pivot)
            SetWindowPos(window, window->SetWindowPosVal - window->Size * window->SetWindowPosPivot, 0); // Position given a pivot (e.g. for centering)
        else if ((flags & ImGuiWindowFlags_ChildMenu) != 0)
            window->Pos = FindBestWindowPosForPopup(window);
        else if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api && window_just_appearing_after_hidden_for_resize)
            window->Pos = FindBestWindowPosForPopup(window);
        else if ((flags & ImGuiWindowFlags_Tooltip) != 0 && !window_pos_set_by_api && !window_is_child_tooltip)
            window->Pos = FindBestWindowPosForPopup(window);

        // Calculate the range of allowed position for that window (to be movable and visible past safe area padding)
        // When clamping to stay visible, we will enforce that window->Pos stays inside of visibility_rect.
        ImRect viewport_rect(viewport->GetMainRect());
        ImRect viewport_work_rect(viewport->GetWorkRect());
        ImVec2 visibility_padding = ImMax(style.DisplayWindowPadding, style.DisplaySafeAreaPadding);
        ImRect visibility_rect(viewport_work_rect.Min + visibility_padding, viewport_work_rect.Max - visibility_padding);

        // Clamp position/size so window stays visible within its viewport or monitor
        // Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
        if (!window_pos_set_by_api && !(flags & ImGuiWindowFlags_ChildWindow))
            if (viewport_rect.GetWidth() > 0.0f && viewport_rect.GetHeight() > 0.0f)
                ClampWindowPos(window, visibility_rect);
        window->Pos = ImTrunc(window->Pos);

        // Lock window rounding for the frame (so that altering them doesn't cause inconsistencies)
        // Large values tend to lead to variety of artifacts and are not recommended.
        window->WindowRounding = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildRounding : ((flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupRounding : style.WindowRounding;

        // For windows with title bar or menu bar, we clamp to FrameHeight(FontSize + FramePadding.y * 2.0f) to completely hide artifacts.
        //if ((window->Flags & ImGuiWindowFlags_MenuBar) || !(window->Flags & ImGuiWindowFlags_NoTitleBar))
        //    window->WindowRounding = ImMin(window->WindowRounding, g.FontSize + style.FramePadding.y * 2.0f);

        // Apply window focus (new and reactivated windows are moved to front)
        bool want_focus = false;
        if (window_just_activated_by_user && !(flags & ImGuiWindowFlags_NoFocusOnAppearing))
        {
            if (flags & ImGuiWindowFlags_Popup)
                want_focus = true;
            else if ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip)) == 0)
                want_focus = true;
        }

        // [Test Engine] Register whole window in the item system (before submitting further decorations)
#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (g.TestEngineHookItems)
        {
            IM_ASSERT(window->IDStack.Size == 1);
            window->IDStack.Size = 0; // As window->IDStack[0] == window->ID here, make sure TestEngine doesn't erroneously see window as parent of itself.
            IMGUI_TEST_ENGINE_ITEM_ADD(window->ID, window->Rect(), NULL);
            IMGUI_TEST_ENGINE_ITEM_INFO(window->ID, window->Name, (g.HoveredWindow == window) ? ImGuiItemStatusFlags_HoveredRect : 0);
            window->IDStack.Size = 1;
        }
#endif

        // Handle manual resize: Resize Grips, Borders, Gamepad
        int border_hovered = -1, border_held = -1;
        ImU32 resize_grip_col[4] = {};
        const int resize_grip_count = ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup)) ? 0 : g.IO.ConfigWindowsResizeFromEdges ? 2 : 1; // Allow resize from lower-left if we have the mouse cursor feedback for it.
        const float resize_grip_draw_size = IM_TRUNC(ImMax(g.FontSize * 1.10f, window->WindowRounding + 1.0f + g.FontSize * 0.2f));
        if (!window->Collapsed)
            if (int auto_fit_mask = UpdateWindowManualResize(window, size_auto_fit, &border_hovered, &border_held, resize_grip_count, &resize_grip_col[0], visibility_rect))
            {
                if (auto_fit_mask & (1 << ImGuiAxis_X))
                    use_current_size_for_scrollbar_x = true;
                if (auto_fit_mask & (1 << ImGuiAxis_Y))
                    use_current_size_for_scrollbar_y = true;
            }
        window->ResizeBorderHovered = (signed char)border_hovered;
        window->ResizeBorderHeld = (signed char)border_held;

        // SCROLLBAR VISIBILITY

        // Update scrollbar visibility (based on the Size that was effective during last frame or the auto-resized Size).
        if (!window->Collapsed)
        {
            // When reading the current size we need to read it after size constraints have been applied.
            // Intentionally use previous frame values for InnerRect and ScrollbarSizes.
            // And when we use window->DecorationUp here it doesn't have ScrollbarSizes.y applied yet.
            ImVec2 avail_size_from_current_frame = ImVec2(window->SizeFull.x, window->SizeFull.y - (window->DecoOuterSizeY1 + window->DecoOuterSizeY2));
            ImVec2 avail_size_from_last_frame = window->InnerRect.GetSize() + scrollbar_sizes_from_last_frame;
            ImVec2 needed_size_from_last_frame = window_just_created ? ImVec2(0, 0) : window->ContentSize + window->WindowPadding * 2.0f;
            float size_x_for_scrollbars = use_current_size_for_scrollbar_x ? avail_size_from_current_frame.x : avail_size_from_last_frame.x;
            float size_y_for_scrollbars = use_current_size_for_scrollbar_y ? avail_size_from_current_frame.y : avail_size_from_last_frame.y;
            //bool scrollbar_y_from_last_frame = window->ScrollbarY; // FIXME: May want to use that in the ScrollbarX expression? How many pros vs cons?
            window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((needed_size_from_last_frame.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar));
            window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((needed_size_from_last_frame.x > size_x_for_scrollbars - (window->ScrollbarY ? style.ScrollbarSize : 0.0f)) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
            if (window->ScrollbarX && !window->ScrollbarY)
                window->ScrollbarY = (needed_size_from_last_frame.y > size_y_for_scrollbars - style.ScrollbarSize) && !(flags & ImGuiWindowFlags_NoScrollbar);
            window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);

            // Amend the partially filled window->DecorationXXX values.
            window->DecoOuterSizeX2 += window->ScrollbarSizes.x;
            window->DecoOuterSizeY2 += window->ScrollbarSizes.y;
        }

        // UPDATE RECTANGLES (1- THOSE NOT AFFECTED BY SCROLLING)
        // Update various regions. Variables they depend on should be set above in this function.
        // We set this up after processing the resize grip so that our rectangles doesn't lag by a frame.

        // Outer rectangle
        // Not affected by window border size. Used by:
        // - FindHoveredWindow() (w/ extra padding when border resize is enabled)
        // - Begin() initial clipping rect for drawing window background and borders.
        // - Begin() clipping whole child
        const ImRect host_rect = ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip) ? parent_window->ClipRect : viewport_rect;
        const ImRect outer_rect = window->Rect();
        const ImRect title_bar_rect = window->TitleBarRect();
        window->OuterRectClipped = outer_rect;
        window->OuterRectClipped.ClipWith(host_rect);

        // Inner rectangle
        // Not affected by window border size. Used by:
        // - InnerClipRect
        // - ScrollToRectEx()
        // - NavUpdatePageUpPageDown()
        // - Scrollbar()
        window->InnerRect.Min.x = window->Pos.x + window->DecoOuterSizeX1;
        window->InnerRect.Min.y = window->Pos.y + window->DecoOuterSizeY1;
        window->InnerRect.Max.x = window->Pos.x + window->Size.x - window->DecoOuterSizeX2;
        window->InnerRect.Max.y = window->Pos.y + window->Size.y - window->DecoOuterSizeY2;

        // Inner clipping rectangle.
        // - Extend a outside of normal work region up to borders.
        // - This is to allow e.g. Selectable or CollapsingHeader or some separators to cover that space.
        // - It also makes clipped items be more noticeable.
        // - And is consistent on both axis (prior to 2024/05/03 ClipRect used WindowPadding.x * 0.5f on left and right edge), see #3312
        // - Force round operator last to ensure that e.g. (int)(max.x-min.x) in user's render code produce correct result.
        // Note that if our window is collapsed we will end up with an inverted (~null) clipping rectangle which is the correct behavior.
        // Affected by window/frame border size. Used by:
        // - Begin() initial clip rect
        float top_border_size = (((flags & ImGuiWindowFlags_MenuBar) || !(flags & ImGuiWindowFlags_NoTitleBar)) ? style.FrameBorderSize : window->WindowBorderSize);

        // Try to match the fact that our border is drawn centered over the window rectangle, rather than inner.
        // This is why we do a *0.5f here. We don't currently even technically support large values for WindowBorderSize,
        // see e.g #7887 #7888, but may do after we move the window border to become an inner border (and then we can remove the 0.5f here).
        window->InnerClipRect.Min.x = ImFloor(0.5f + window->InnerRect.Min.x + window->WindowBorderSize * 0.5f);
        window->InnerClipRect.Min.y = ImFloor(0.5f + window->InnerRect.Min.y + top_border_size * 0.5f);
        window->InnerClipRect.Max.x = ImFloor(window->InnerRect.Max.x - window->WindowBorderSize * 0.5f);
        window->InnerClipRect.Max.y = ImFloor(window->InnerRect.Max.y - window->WindowBorderSize * 0.5f);
        window->InnerClipRect.ClipWithFull(host_rect);

        // Default item width. Make it proportional to window size if window manually resizes
        if (window->Size.x > 0.0f && !(flags & ImGuiWindowFlags_Tooltip) && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
            window->ItemWidthDefault = ImTrunc(window->Size.x * 0.65f);
        else
            window->ItemWidthDefault = ImTrunc(g.FontSize * 16.0f);

        // SCROLLING

        // Lock down maximum scrolling
        // The value of ScrollMax are ahead from ScrollbarX/ScrollbarY which is intentionally using InnerRect from previous rect in order to accommodate
        // for right/bottom aligned items without creating a scrollbar.
        window->ScrollMax.x = ImMax(0.0f, window->ContentSize.x + window->WindowPadding.x * 2.0f - window->InnerRect.GetWidth());
        window->ScrollMax.y = ImMax(0.0f, window->ContentSize.y + window->WindowPadding.y * 2.0f - window->InnerRect.GetHeight());

        // Apply scrolling
        window->Scroll = CalcNextScrollFromScrollTargetAndClamp(window);
        window->ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);
        window->DecoInnerSizeX1 = window->DecoInnerSizeY1 = 0.0f;

        // DRAWING

        // Setup draw list and outer clipping rectangle
        IM_ASSERT(window->DrawList->CmdBuffer.Size == 1 && window->DrawList->CmdBuffer[0].ElemCount == 0);
        window->DrawList->PushTextureID(g.Font->ContainerAtlas->TexID);
        PushClipRect(host_rect.Min, host_rect.Max, false);

        // Child windows can render their decoration (bg color, border, scrollbars, etc.) within their parent to save a draw call (since 1.71)
        // When using overlapping child windows, this will break the assumption that child z-order is mapped to submission order.
        // FIXME: User code may rely on explicit sorting of overlapping child window and would need to disable this somehow. Please get in contact if you are affected (github #4493)
        {
            bool render_decorations_in_parent = false;
            if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip)
            {
                // - We test overlap with the previous child window only (testing all would end up being O(log N) not a good investment here)
                // - We disable this when the parent window has zero vertices, which is a common pattern leading to laying out multiple overlapping childs
                ImGuiWindow* previous_child = parent_window->DC.ChildWindows.Size >= 2 ? parent_window->DC.ChildWindows[parent_window->DC.ChildWindows.Size - 2] : NULL;
                bool previous_child_overlapping = previous_child ? previous_child->Rect().Overlaps(window->Rect()) : false;
                bool parent_is_empty = (parent_window->DrawList->VtxBuffer.Size == 0);
                if (window->DrawList->CmdBuffer.back().ElemCount == 0 && !parent_is_empty && !previous_child_overlapping)
                    render_decorations_in_parent = true;
            }
            if (render_decorations_in_parent)
                window->DrawList = parent_window->DrawList;

            // Handle title bar, scrollbar, resize grips and resize borders
            const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
            const bool title_bar_is_highlight = want_focus || (window_to_highlight && window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight);
            const bool handle_borders_and_resize_grips = true; // This exists to facilitate merge with 'docking' branch.
            RenderWindowDecorations(window, title_bar_rect, title_bar_is_highlight, handle_borders_and_resize_grips, resize_grip_count, resize_grip_col, resize_grip_draw_size);

            if (render_decorations_in_parent)
                window->DrawList = &window->DrawListInst;
        }

        // UPDATE RECTANGLES (2- THOSE AFFECTED BY SCROLLING)

        // Work rectangle.
        // Affected by window padding and border size. Used by:
        // - Columns() for right-most edge
        // - TreeNode(), CollapsingHeader() for right-most edge
        // - BeginTabBar() for right-most edge
        const bool allow_scrollbar_x = !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar);
        const bool allow_scrollbar_y = !(flags & ImGuiWindowFlags_NoScrollbar);
        const float work_rect_size_x = (window->ContentSizeExplicit.x != 0.0f ? window->ContentSizeExplicit.x : ImMax(allow_scrollbar_x ? window->ContentSize.x : 0.0f, window->Size.x - window->WindowPadding.x * 2.0f - (window->DecoOuterSizeX1 + window->DecoOuterSizeX2)));
        const float work_rect_size_y = (window->ContentSizeExplicit.y != 0.0f ? window->ContentSizeExplicit.y : ImMax(allow_scrollbar_y ? window->ContentSize.y : 0.0f, window->Size.y - window->WindowPadding.y * 2.0f - (window->DecoOuterSizeY1 + window->DecoOuterSizeY2)));
        window->WorkRect.Min.x = ImTrunc(window->InnerRect.Min.x - window->Scroll.x + ImMax(window->WindowPadding.x, window->WindowBorderSize));
        window->WorkRect.Min.y = ImTrunc(window->InnerRect.Min.y - window->Scroll.y + ImMax(window->WindowPadding.y, window->WindowBorderSize));
        window->WorkRect.Max.x = window->WorkRect.Min.x + work_rect_size_x;
        window->WorkRect.Max.y = window->WorkRect.Min.y + work_rect_size_y;
        window->ParentWorkRect = window->WorkRect;

        // [LEGACY] Content Region
        // FIXME-OBSOLETE: window->ContentRegionRect.Max is currently very misleading / partly faulty, but some BeginChild() patterns relies on it.
        // Unless explicit content size is specified by user, this currently represent the region leading to no scrolling.
        // Used by:
        // - Mouse wheel scrolling + many other things
        window->ContentRegionRect.Min.x = window->Pos.x - window->Scroll.x + window->WindowPadding.x + window->DecoOuterSizeX1;
        window->ContentRegionRect.Min.y = window->Pos.y - window->Scroll.y + window->WindowPadding.y + window->DecoOuterSizeY1;
        window->ContentRegionRect.Max.x = window->ContentRegionRect.Min.x + (window->ContentSizeExplicit.x != 0.0f ? window->ContentSizeExplicit.x : (window->Size.x - window->WindowPadding.x * 2.0f - (window->DecoOuterSizeX1 + window->DecoOuterSizeX2)));
        window->ContentRegionRect.Max.y = window->ContentRegionRect.Min.y + (window->ContentSizeExplicit.y != 0.0f ? window->ContentSizeExplicit.y : (window->Size.y - window->WindowPadding.y * 2.0f - (window->DecoOuterSizeY1 + window->DecoOuterSizeY2)));

        // Setup drawing context
        // (NB: That term "drawing context / DC" lost its meaning a long time ago. Initially was meant to hold transient data only. Nowadays difference between window-> and window->DC-> is dubious.)
        window->DC.Indent.x = window->DecoOuterSizeX1 + window->WindowPadding.x - window->Scroll.x;
        window->DC.GroupOffset.x = 0.0f;
        window->DC.ColumnsOffset.x = 0.0f;

        // Record the loss of precision of CursorStartPos which can happen due to really large scrolling amount.
        // This is used by clipper to compensate and fix the most common use case of large scroll area. Easy and cheap, next best thing compared to switching everything to double or ImU64.
        double start_pos_highp_x = (double)window->Pos.x + window->WindowPadding.x - (double)window->Scroll.x + window->DecoOuterSizeX1 + window->DC.ColumnsOffset.x;
        double start_pos_highp_y = (double)window->Pos.y + window->WindowPadding.y - (double)window->Scroll.y + window->DecoOuterSizeY1;
        window->DC.CursorStartPos  = ImVec2((float)start_pos_highp_x, (float)start_pos_highp_y);
        window->DC.CursorStartPosLossyness = ImVec2((float)(start_pos_highp_x - window->DC.CursorStartPos.x), (float)(start_pos_highp_y - window->DC.CursorStartPos.y));
        window->DC.CursorPos = window->DC.CursorStartPos;
        window->DC.CursorPosPrevLine = window->DC.CursorPos;
        window->DC.CursorMaxPos = window->DC.CursorStartPos;
        window->DC.IdealMaxPos = window->DC.CursorStartPos;
        window->DC.CurrLineSize = window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
        window->DC.CurrLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
        window->DC.IsSameLine = window->DC.IsSetPos = false;

        window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
        window->DC.NavLayersActiveMask = window->DC.NavLayersActiveMaskNext;
        window->DC.NavLayersActiveMaskNext = 0x00;
        window->DC.NavIsScrollPushableX = true;
        window->DC.NavHideHighlightOneFrame = false;
        window->DC.NavWindowHasScrollY = (window->ScrollMax.y > 0.0f);

        window->DC.MenuBarAppending = false;
        window->DC.MenuColumns.Update(style.ItemSpacing.x, window_just_activated_by_user);
        window->DC.TreeDepth = 0;
        window->DC.TreeHasStackDataDepthMask = 0x00;
        window->DC.ChildWindows.resize(0);
        window->DC.StateStorage = &window->StateStorage;
        window->DC.CurrentColumns = NULL;
        window->DC.LayoutType = ImGuiLayoutType_Vertical;
        window->DC.ParentLayoutType = parent_window ? parent_window->DC.LayoutType : ImGuiLayoutType_Vertical;

        window->DC.ItemWidth = window->ItemWidthDefault;
        window->DC.TextWrapPos = -1.0f; // disabled
        window->DC.ItemWidthStack.resize(0);
        window->DC.TextWrapPosStack.resize(0);
        if (flags & ImGuiWindowFlags_Modal)
            window->DC.ModalDimBgColor = ColorConvertFloat4ToU32(GetStyleColorVec4(ImGuiCol_ModalWindowDimBg));

        if (window->AutoFitFramesX > 0)
            window->AutoFitFramesX--;
        if (window->AutoFitFramesY > 0)
            window->AutoFitFramesY--;

        // Apply focus (we need to call FocusWindow() AFTER setting DC.CursorStartPos so our initial navigation reference rectangle can start around there)
        // We ImGuiFocusRequestFlags_UnlessBelowModal to:
        // - Avoid focusing a window that is created outside of a modal. This will prevent active modal from being closed.
        // - Position window behind the modal that is not a begin-parent of this window.
        if (want_focus)
            FocusWindow(window, ImGuiFocusRequestFlags_UnlessBelowModal);
        if (want_focus && window == g.NavWindow)
            NavInitWindow(window, false); // <-- this is in the way for us to be able to defer and sort reappearing FocusWindow() calls

        // Title bar
        if (!(flags & ImGuiWindowFlags_NoTitleBar))
            RenderWindowTitleBarContents(window, ImRect(title_bar_rect.Min.x + window->WindowBorderSize, title_bar_rect.Min.y, title_bar_rect.Max.x - window->WindowBorderSize, title_bar_rect.Max.y), name, p_open);

        // Clear hit test shape every frame
        window->HitTestHoleSize.x = window->HitTestHoleSize.y = 0;

        if (flags & ImGuiWindowFlags_Tooltip)
            g.TooltipPreviousWindow = window;

        // Pressing CTRL+C while holding on a window copy its content to the clipboard
        // This works but 1. doesn't handle multiple Begin/End pairs, 2. recursing into another Begin/End pair - so we need to work that out and add better logging scope.
        // Maybe we can support CTRL+C on every element?
        /*
        //if (g.NavWindow == window && g.ActiveId == 0)
        if (g.ActiveId == window->MoveId)
            if (g.IO.KeyCtrl && IsKeyPressed(ImGuiKey_C))
                LogToClipboard();
        */

        // We fill last item data based on Title Bar/Tab, in order for IsItemHovered() and IsItemActive() to be usable after Begin().
        // This is useful to allow creating context menus on title bar only, etc.
        SetLastItemDataForWindow(window, title_bar_rect);

        // [DEBUG]
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
        if (g.DebugLocateId != 0 && (window->ID == g.DebugLocateId || window->MoveId == g.DebugLocateId))
            DebugLocateItemResolveWithLastItem();
#endif

        // [Test Engine] Register title bar / tab with MoveId.
#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
            IMGUI_TEST_ENGINE_ITEM_ADD(g.LastItemData.ID, g.LastItemData.Rect, &g.LastItemData);
#endif
    }
    else
    {
        // Skip refresh always mark active
        if (window->SkipRefresh)
            SetWindowActiveForSkipRefresh(window);

        // Append
        SetCurrentWindow(window);
        SetLastItemDataForWindow(window, window->TitleBarRect());
    }

    if (!window->SkipRefresh)
        PushClipRect(window->InnerClipRect.Min, window->InnerClipRect.Max, true);

    // Clear 'accessed' flag last thing (After PushClipRect which will set the flag. We want the flag to stay false when the default "Debug" window is unused)
    window->WriteAccessed = false;
    window->BeginCount++;
    g.NextWindowData.ClearFlags();

    // Update visibility
    if (first_begin_of_the_frame && !window->SkipRefresh)
    {
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_ChildMenu))
        {
            // Child window can be out of sight and have "negative" clip windows.
            // Mark them as collapsed so commands are skipped earlier (we can't manually collapse them because they have no title bar).
            IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0);
            const bool nav_request = (window->ChildFlags & ImGuiChildFlags_NavFlattened) && (g.NavAnyRequest && g.NavWindow && g.NavWindow->RootWindowForNav == window->RootWindowForNav);
            if (!g.LogEnabled && !nav_request)
                if (window->OuterRectClipped.Min.x >= window->OuterRectClipped.Max.x || window->OuterRectClipped.Min.y >= window->OuterRectClipped.Max.y)
                {
                    if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
                        window->HiddenFramesCannotSkipItems = 1;
                    else
                        window->HiddenFramesCanSkipItems = 1;
                }

            // Hide along with parent or if parent is collapsed
            if (parent_window && (parent_window->Collapsed || parent_window->HiddenFramesCanSkipItems > 0))
                window->HiddenFramesCanSkipItems = 1;
            if (parent_window && (parent_window->Collapsed || parent_window->HiddenFramesCannotSkipItems > 0))
                window->HiddenFramesCannotSkipItems = 1;
        }

        // Don't render if style alpha is 0.0 at the time of Begin(). This is arbitrary and inconsistent but has been there for a long while (may remove at some point)
        if (style.Alpha <= 0.0f)
            window->HiddenFramesCanSkipItems = 1;

        // Update the Hidden flag
        bool hidden_regular = (window->HiddenFramesCanSkipItems > 0) || (window->HiddenFramesCannotSkipItems > 0);
        window->Hidden = hidden_regular || (window->HiddenFramesForRenderOnly > 0);

        // Disable inputs for requested number of frames
        if (window->DisableInputsFrames > 0)
        {
            window->DisableInputsFrames--;
            window->Flags |= ImGuiWindowFlags_NoInputs;
        }

        // Update the SkipItems flag, used to early out of all items functions (no layout required)
        bool skip_items = false;
        if (window->Collapsed || !window->Active || hidden_regular)
            if (window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && window->HiddenFramesCannotSkipItems <= 0)
                skip_items = true;
        window->SkipItems = skip_items;
    }
    else if (first_begin_of_the_frame)
    {
        // Skip refresh mode
        window->SkipItems = true;
    }

    // [DEBUG] io.ConfigDebugBeginReturnValue override return value to test Begin/End and BeginChild/EndChild behaviors.
    // (The implicit fallback window is NOT automatically ended allowing it to always be able to receive commands without crashing)
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    if (!window->IsFallbackWindow)
        if ((g.IO.ConfigDebugBeginReturnValueOnce && window_just_created) || (g.IO.ConfigDebugBeginReturnValueLoop && g.DebugBeginReturnValueCullDepth == g.CurrentWindowStack.Size))
        {
            if (window->AutoFitFramesX > 0) { window->AutoFitFramesX++; }
            if (window->AutoFitFramesY > 0) { window->AutoFitFramesY++; }
            return false;
        }
#endif

    return !window->SkipItems;
}

static void ImGui::SetLastItemDataForWindow(ImGuiWindow* window, const ImRect& rect)
{
    ImGuiContext& g = *GImGui;
    SetLastItemData(window->MoveId, g.CurrentItemFlags, IsMouseHoveringRect(rect.Min, rect.Max, false) ? ImGuiItemStatusFlags_HoveredRect : 0, rect);
}

void ImGui::End()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // Error checking: verify that user hasn't called End() too many times!
    if (g.CurrentWindowStack.Size <= 1 && g.WithinFrameScopeWithImplicitWindow)
    {
        IM_ASSERT_USER_ERROR(g.CurrentWindowStack.Size > 1, "Calling End() too many times!");
        return;
    }
    ImGuiWindowStackData& window_stack_data = g.CurrentWindowStack.back();

    // Error checking: verify that user doesn't directly call End() on a child window.
    if (window->Flags & ImGuiWindowFlags_ChildWindow)
        IM_ASSERT_USER_ERROR(g.WithinEndChild, "Must call EndChild() and not End()!");

    // Close anything that is open
    if (window->DC.CurrentColumns)
        EndColumns();
    if (!window->SkipRefresh)
        PopClipRect();   // Inner window clip rectangle
    PopFocusScope();
    if (window_stack_data.DisabledOverrideReenable && window->RootWindow == window)
        EndDisabledOverrideReenable();

    if (window->SkipRefresh)
    {
        IM_ASSERT(window->DrawList == NULL);
        window->DrawList = &window->DrawListInst;
    }

    // Stop logging
    if (!(window->Flags & ImGuiWindowFlags_ChildWindow))    // FIXME: add more options for scope of logging
        LogFinish();

    if (window->DC.IsSetPos)
        ErrorCheckUsingSetCursorPosToExtendParentBoundaries();

    // Pop from window stack
    g.LastItemData = window_stack_data.ParentLastItemDataBackup;
    if (window->Flags & ImGuiWindowFlags_ChildMenu)
        g.BeginMenuDepth--;
    if (window->Flags & ImGuiWindowFlags_Popup)
        g.BeginPopupStack.pop_back();

    // Error handling, state recovery
    if (g.IO.ConfigErrorRecovery)
        ErrorRecoveryTryToRecoverWindowState(&window_stack_data.StackSizesInBegin);

    g.CurrentWindowStack.pop_back();
    SetCurrentWindow(g.CurrentWindowStack.Size == 0 ? NULL : g.CurrentWindowStack.back().Window);
}

void ImGui::BringWindowToFocusFront(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(window == window->RootWindow);

    const int cur_order = window->FocusOrder;
    IM_ASSERT(g.WindowsFocusOrder[cur_order] == window);
    if (g.WindowsFocusOrder.back() == window)
        return;

    const int new_order = g.WindowsFocusOrder.Size - 1;
    for (int n = cur_order; n < new_order; n++)
    {
        g.WindowsFocusOrder[n] = g.WindowsFocusOrder[n + 1];
        g.WindowsFocusOrder[n]->FocusOrder--;
        IM_ASSERT(g.WindowsFocusOrder[n]->FocusOrder == n);
    }
    g.WindowsFocusOrder[new_order] = window;
    window->FocusOrder = (short)new_order;
}

void ImGui::BringWindowToDisplayFront(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* current_front_window = g.Windows.back();
    if (current_front_window == window || current_front_window->RootWindow == window) // Cheap early out (could be better)
        return;
    for (int i = g.Windows.Size - 2; i >= 0; i--) // We can ignore the top-most window
        if (g.Windows[i] == window)
        {
            memmove(&g.Windows[i], &g.Windows[i + 1], (size_t)(g.Windows.Size - i - 1) * sizeof(ImGuiWindow*));
            g.Windows[g.Windows.Size - 1] = window;
            break;
        }
}

void ImGui::BringWindowToDisplayBack(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.Windows[0] == window)
        return;
    for (int i = 0; i < g.Windows.Size; i++)
        if (g.Windows[i] == window)
        {
            memmove(&g.Windows[1], &g.Windows[0], (size_t)i * sizeof(ImGuiWindow*));
            g.Windows[0] = window;
            break;
        }
}

void ImGui::BringWindowToDisplayBehind(ImGuiWindow* window, ImGuiWindow* behind_window)
{
    IM_ASSERT(window != NULL && behind_window != NULL);
    ImGuiContext& g = *GImGui;
    window = window->RootWindow;
    behind_window = behind_window->RootWindow;
    int pos_wnd = FindWindowDisplayIndex(window);
    int pos_beh = FindWindowDisplayIndex(behind_window);
    if (pos_wnd < pos_beh)
    {
        size_t copy_bytes = (pos_beh - pos_wnd - 1) * sizeof(ImGuiWindow*);
        memmove(&g.Windows.Data[pos_wnd], &g.Windows.Data[pos_wnd + 1], copy_bytes);
        g.Windows[pos_beh - 1] = window;
    }
    else
    {
        size_t copy_bytes = (pos_wnd - pos_beh) * sizeof(ImGuiWindow*);
        memmove(&g.Windows.Data[pos_beh + 1], &g.Windows.Data[pos_beh], copy_bytes);
        g.Windows[pos_beh] = window;
    }
}

int ImGui::FindWindowDisplayIndex(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    return g.Windows.index_from_ptr(g.Windows.find(window));
}

// Moving window to front of display and set focus (which happens to be back of our sorted list)
void ImGui::FocusWindow(ImGuiWindow* window, ImGuiFocusRequestFlags flags)
{
    ImGuiContext& g = *GImGui;

    // Modal check?
    if ((flags & ImGuiFocusRequestFlags_UnlessBelowModal) && (g.NavWindow != window)) // Early out in common case.
        if (ImGuiWindow* blocking_modal = FindBlockingModal(window))
        {
            // This block would typically be reached in two situations:
            // - API call to FocusWindow() with a window under a modal and ImGuiFocusRequestFlags_UnlessBelowModal flag.
            // - User clicking on void or anything behind a modal while a modal is open (window == NULL)
            IMGUI_DEBUG_LOG_FOCUS("[focus] FocusWindow(\"%s\", UnlessBelowModal): prevented by \"%s\".\n", window ? window->Name : "<NULL>", blocking_modal->Name);
            if (window && window == window->RootWindow && (window->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus) == 0)
                BringWindowToDisplayBehind(window, blocking_modal); // Still bring right under modal. (FIXME: Could move in focus list too?)
            ClosePopupsOverWindow(GetTopMostPopupModal(), false); // Note how we need to use GetTopMostPopupModal() aad NOT blocking_modal, to handle nested modals
            return;
        }

    // Find last focused child (if any) and focus it instead.
    if ((flags & ImGuiFocusRequestFlags_RestoreFocusedChild) && window != NULL)
        window = NavRestoreLastChildNavWindow(window);

    // Apply focus
    if (g.NavWindow != window)
    {
        SetNavWindow(window);
        if (window && g.NavDisableMouseHover)
            g.NavMousePosDirty = true;
        g.NavId = window ? window->NavLastIds[0] : 0; // Restore NavId
        g.NavLayer = ImGuiNavLayer_Main;
        SetNavFocusScope(window ? window->NavRootFocusScopeId : 0);
        g.NavIdIsAlive = false;
        g.NavLastValidSelectionUserData = ImGuiSelectionUserData_Invalid;

        // Close popups if any
        ClosePopupsOverWindow(window, false);
    }

    // Move the root window to the top of the pile
    IM_ASSERT(window == NULL || window->RootWindow != NULL);
    ImGuiWindow* focus_front_window = window ? window->RootWindow : NULL; // NB: In docking branch this is window->RootWindowDockStop
    ImGuiWindow* display_front_window = window ? window->RootWindow : NULL;

    // Steal active widgets. Some of the cases it triggers includes:
    // - Focus a window while an InputText in another window is active, if focus happens before the old InputText can run.
    // - When using Nav to activate menu items (due to timing of activating on press->new window appears->losing ActiveId)
    if (g.ActiveId != 0 && g.ActiveIdWindow && g.ActiveIdWindow->RootWindow != focus_front_window)
        if (!g.ActiveIdNoClearOnFocusLoss)
            ClearActiveID();

    // Passing NULL allow to disable keyboard focus
    if (!window)
        return;

    // Bring to front
    BringWindowToFocusFront(focus_front_window);
    if (((window->Flags | display_front_window->Flags) & ImGuiWindowFlags_NoBringToFrontOnFocus) == 0)
        BringWindowToDisplayFront(display_front_window);
}

void ImGui::FocusTopMostWindowUnderOne(ImGuiWindow* under_this_window, ImGuiWindow* ignore_window, ImGuiViewport* filter_viewport, ImGuiFocusRequestFlags flags)
{
    ImGuiContext& g = *GImGui;
    IM_UNUSED(filter_viewport); // Unused in master branch.
    int start_idx = g.WindowsFocusOrder.Size - 1;
    if (under_this_window != NULL)
    {
        // Aim at root window behind us, if we are in a child window that's our own root (see #4640)
        int offset = -1;
        while (under_this_window->Flags & ImGuiWindowFlags_ChildWindow)
        {
            under_this_window = under_this_window->ParentWindow;
            offset = 0;
        }
        start_idx = FindWindowFocusIndex(under_this_window) + offset;
    }
    for (int i = start_idx; i >= 0; i--)
    {
        // We may later decide to test for different NoXXXInputs based on the active navigation input (mouse vs nav) but that may feel more confusing to the user.
        ImGuiWindow* window = g.WindowsFocusOrder[i];
        if (window == ignore_window || !window->WasActive)
            continue;
        if ((window->Flags & (ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs)) != (ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs))
        {
            FocusWindow(window, flags);
            return;
        }
    }
    FocusWindow(NULL, flags);
}

// Important: this alone doesn't alter current ImDrawList state. This is called by PushFont/PopFont only.
void ImGui::SetCurrentFont(ImFont* font)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(font && font->IsLoaded());    // Font Atlas not created. Did you call io.Fonts->GetTexDataAsRGBA32 / GetTexDataAsAlpha8 ?
    IM_ASSERT(font->Scale > 0.0f);
    g.Font = font;
    g.FontBaseSize = ImMax(1.0f, g.IO.FontGlobalScale * g.Font->FontSize * g.Font->Scale);
    g.FontSize = g.CurrentWindow ? g.CurrentWindow->CalcFontSize() : 0.0f;
    g.FontScale = g.FontSize / g.Font->FontSize;

    ImFontAtlas* atlas = g.Font->ContainerAtlas;
    g.DrawListSharedData.TexUvWhitePixel = atlas->TexUvWhitePixel;
    g.DrawListSharedData.TexUvLines = atlas->TexUvLines;
    g.DrawListSharedData.Font = g.Font;
    g.DrawListSharedData.FontSize = g.FontSize;
    g.DrawListSharedData.FontScale = g.FontScale;
}

// Use ImDrawList::_SetTextureID(), making our shared g.FontStack[] authorative against window-local ImDrawList.
// - Whereas ImDrawList::PushTextureID()/PopTextureID() is not to be used across Begin() calls.
// - Note that we don't propagate current texture id when e.g. Begin()-ing into a new window, we never really did...
//   - Some code paths never really fully worked with multiple atlas textures.
//   - The right-ish solution may be to remove _SetTextureID() and make AddText/RenderText lazily call PushTextureID()/PopTextureID()
//     the same way AddImage() does, but then all other primitives would also need to? I don't think we should tackle this problem
//     because we have a concrete need and a test bed for multiple atlas textures.
void ImGui::PushFont(ImFont* font)
{
    ImGuiContext& g = *GImGui;
    if (font == NULL)
        font = GetDefaultFont();
    g.FontStack.push_back(font);
    SetCurrentFont(font);
    g.CurrentWindow->DrawList->_SetTextureID(font->ContainerAtlas->TexID);
}

void  ImGui::PopFont()
{
    ImGuiContext& g = *GImGui;
    if (g.FontStack.Size <= 0)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PopFont() too many times!");
        return;
    }
    g.FontStack.pop_back();
    ImFont* font = g.FontStack.Size == 0 ? GetDefaultFont() : g.FontStack.back();
    SetCurrentFont(font);
    g.CurrentWindow->DrawList->_SetTextureID(font->ContainerAtlas->TexID);
}

void ImGui::PushItemFlag(ImGuiItemFlags option, bool enabled)
{
    ImGuiContext& g = *GImGui;
    ImGuiItemFlags item_flags = g.CurrentItemFlags;
    IM_ASSERT(item_flags == g.ItemFlagsStack.back());
    if (enabled)
        item_flags |= option;
    else
        item_flags &= ~option;
    g.CurrentItemFlags = item_flags;
    g.ItemFlagsStack.push_back(item_flags);
}

void ImGui::PopItemFlag()
{
    ImGuiContext& g = *GImGui;
    if (g.ItemFlagsStack.Size <= 1)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PopItemFlag() too many times!");
        return;
    }
    g.ItemFlagsStack.pop_back();
    g.CurrentItemFlags = g.ItemFlagsStack.back();
}

// BeginDisabled()/EndDisabled()
// - Those can be nested but it cannot be used to enable an already disabled section (a single BeginDisabled(true) in the stack is enough to keep everything disabled)
// - Visually this is currently altering alpha, but it is expected that in a future styling system this would work differently.
// - Feedback welcome at https://github.com/ocornut/imgui/issues/211
// - BeginDisabled(false)/EndDisabled() essentially does nothing but is provided to facilitate use of boolean expressions.
//   (as a micro-optimization: if you have tens of thousands of BeginDisabled(false)/EndDisabled() pairs, you might want to reformulate your code to avoid making those calls)
// - Note: mixing up BeginDisabled() and PushItemFlag(ImGuiItemFlags_Disabled) is currently NOT SUPPORTED.
void ImGui::BeginDisabled(bool disabled)
{
    ImGuiContext& g = *GImGui;
    bool was_disabled = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
    if (!was_disabled && disabled)
    {
        g.DisabledAlphaBackup = g.Style.Alpha;
        g.Style.Alpha *= g.Style.DisabledAlpha; // PushStyleVar(ImGuiStyleVar_Alpha, g.Style.Alpha * g.Style.DisabledAlpha);
    }
    if (was_disabled || disabled)
        g.CurrentItemFlags |= ImGuiItemFlags_Disabled;
    g.ItemFlagsStack.push_back(g.CurrentItemFlags); // FIXME-OPT: can we simply skip this and use DisabledStackSize?
    g.DisabledStackSize++;
}

void ImGui::EndDisabled()
{
    ImGuiContext& g = *GImGui;
    if (g.DisabledStackSize <= 0)
    {
        IM_ASSERT_USER_ERROR(0, "Calling EndDisabled() too many times!");
        return;
    }
    g.DisabledStackSize--;
    bool was_disabled = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
    //PopItemFlag();
    g.ItemFlagsStack.pop_back();
    g.CurrentItemFlags = g.ItemFlagsStack.back();
    if (was_disabled && (g.CurrentItemFlags & ImGuiItemFlags_Disabled) == 0)
        g.Style.Alpha = g.DisabledAlphaBackup; //PopStyleVar();
}

// Could have been called BeginDisabledDisable() but it didn't want to be award nominated for most awkward function name.
// Ideally we would use a shared e.g. BeginDisabled()->BeginDisabledEx() but earlier needs to be optimal.
// The whole code for this is awkward, will reevaluate if we find a way to implement SetNextItemDisabled().
void ImGui::BeginDisabledOverrideReenable()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentItemFlags & ImGuiItemFlags_Disabled);
    g.Style.Alpha = g.DisabledAlphaBackup;
    g.CurrentItemFlags &= ~ImGuiItemFlags_Disabled;
    g.ItemFlagsStack.push_back(g.CurrentItemFlags);
    g.DisabledStackSize++;
}

void ImGui::EndDisabledOverrideReenable()
{
    ImGuiContext& g = *GImGui;
    g.DisabledStackSize--;
    IM_ASSERT(g.DisabledStackSize > 0);
    g.ItemFlagsStack.pop_back();
    g.CurrentItemFlags = g.ItemFlagsStack.back();
    g.Style.Alpha = g.DisabledAlphaBackup * g.Style.DisabledAlpha;
}

void ImGui::PushTextWrapPos(float wrap_pos_x)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DC.TextWrapPosStack.push_back(window->DC.TextWrapPos);
    window->DC.TextWrapPos = wrap_pos_x;
}

void ImGui::PopTextWrapPos()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->DC.TextWrapPosStack.Size <= 0)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PopTextWrapPos() too many times!");
        return;
    }
    window->DC.TextWrapPos = window->DC.TextWrapPosStack.back();
    window->DC.TextWrapPosStack.pop_back();
}

static ImGuiWindow* GetCombinedRootWindow(ImGuiWindow* window, bool popup_hierarchy)
{
    ImGuiWindow* last_window = NULL;
    while (last_window != window)
    {
        last_window = window;
        window = window->RootWindow;
        if (popup_hierarchy)
            window = window->RootWindowPopupTree;
    }
    return window;
}

bool ImGui::IsWindowChildOf(ImGuiWindow* window, ImGuiWindow* potential_parent, bool popup_hierarchy)
{
    ImGuiWindow* window_root = GetCombinedRootWindow(window, popup_hierarchy);
    if (window_root == potential_parent)
        return true;
    while (window != NULL)
    {
        if (window == potential_parent)
            return true;
        if (window == window_root) // end of chain
            return false;
        window = window->ParentWindow;
    }
    return false;
}

bool ImGui::IsWindowWithinBeginStackOf(ImGuiWindow* window, ImGuiWindow* potential_parent)
{
    if (window->RootWindow == potential_parent)
        return true;
    while (window != NULL)
    {
        if (window == potential_parent)
            return true;
        window = window->ParentWindowInBeginStack;
    }
    return false;
}

bool ImGui::IsWindowAbove(ImGuiWindow* potential_above, ImGuiWindow* potential_below)
{
    ImGuiContext& g = *GImGui;

    // It would be saner to ensure that display layer is always reflected in the g.Windows[] order, which would likely requires altering all manipulations of that array
    const int display_layer_delta = GetWindowDisplayLayer(potential_above) - GetWindowDisplayLayer(potential_below);
    if (display_layer_delta != 0)
        return display_layer_delta > 0;

    for (int i = g.Windows.Size - 1; i >= 0; i--)
    {
        ImGuiWindow* candidate_window = g.Windows[i];
        if (candidate_window == potential_above)
            return true;
        if (candidate_window == potential_below)
            return false;
    }
    return false;
}

// Is current window hovered and hoverable (e.g. not blocked by a popup/modal)? See ImGuiHoveredFlags_ for options.
// IMPORTANT: If you are trying to check whether your mouse should be dispatched to Dear ImGui or to your underlying app,
// you should not use this function! Use the 'io.WantCaptureMouse' boolean for that!
// Refer to FAQ entry "How can I tell whether to dispatch mouse/keyboard to Dear ImGui or my application?" for details.
bool ImGui::IsWindowHovered(ImGuiHoveredFlags flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT_USER_ERROR((flags & ~ImGuiHoveredFlags_AllowedMaskForIsWindowHovered) == 0, "Invalid flags for IsWindowHovered()!");

    ImGuiWindow* ref_window = g.HoveredWindow;
    ImGuiWindow* cur_window = g.CurrentWindow;
    if (ref_window == NULL)
        return false;

    if ((flags & ImGuiHoveredFlags_AnyWindow) == 0)
    {
        IM_ASSERT(cur_window); // Not inside a Begin()/End()
        const bool popup_hierarchy = (flags & ImGuiHoveredFlags_NoPopupHierarchy) == 0;
        if (flags & ImGuiHoveredFlags_RootWindow)
            cur_window = GetCombinedRootWindow(cur_window, popup_hierarchy);

        bool result;
        if (flags & ImGuiHoveredFlags_ChildWindows)
            result = IsWindowChildOf(ref_window, cur_window, popup_hierarchy);
        else
            result = (ref_window == cur_window);
        if (!result)
            return false;
    }

    if (!IsWindowContentHoverable(ref_window, flags))
        return false;
    if (!(flags & ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        if (g.ActiveId != 0 && !g.ActiveIdAllowOverlap && g.ActiveId != ref_window->MoveId)
            return false;

    // When changing hovered window we requires a bit of stationary delay before activating hover timer.
    // FIXME: We don't support delay other than stationary one for now, other delay would need a way
    // to fulfill the possibility that multiple IsWindowHovered() with varying flag could return true
    // for different windows of the hierarchy. Possibly need a Hash(Current+Flags) ==> (Timer) cache.
    // We can implement this for _Stationary because the data is linked to HoveredWindow rather than CurrentWindow.
    if (flags & ImGuiHoveredFlags_ForTooltip)
        flags = ApplyHoverFlagsForTooltip(flags, g.Style.HoverFlagsForTooltipMouse);
    if ((flags & ImGuiHoveredFlags_Stationary) != 0 && g.HoverWindowUnlockedStationaryId != ref_window->ID)
        return false;

    return true;
}

bool ImGui::IsWindowFocused(ImGuiFocusedFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* ref_window = g.NavWindow;
    ImGuiWindow* cur_window = g.CurrentWindow;

    if (ref_window == NULL)
        return false;
    if (flags & ImGuiFocusedFlags_AnyWindow)
        return true;

    IM_ASSERT(cur_window); // Not inside a Begin()/End()
    const bool popup_hierarchy = (flags & ImGuiFocusedFlags_NoPopupHierarchy) == 0;
    if (flags & ImGuiHoveredFlags_RootWindow)
        cur_window = GetCombinedRootWindow(cur_window, popup_hierarchy);

    if (flags & ImGuiHoveredFlags_ChildWindows)
        return IsWindowChildOf(ref_window, cur_window, popup_hierarchy);
    else
        return (ref_window == cur_window);
}

// Can we focus this window with CTRL+TAB (or PadMenu + PadFocusPrev/PadFocusNext)
// Note that NoNavFocus makes the window not reachable with CTRL+TAB but it can still be focused with mouse or programmatically.
// If you want a window to never be focused, you may use the e.g. NoInputs flag.
bool ImGui::IsWindowNavFocusable(ImGuiWindow* window)
{
    return window->WasActive && window == window->RootWindow && !(window->Flags & ImGuiWindowFlags_NoNavFocus);
}

float ImGui::GetWindowWidth()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Size.x;
}

float ImGui::GetWindowHeight()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Size.y;
}

ImVec2 ImGui::GetWindowPos()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    return window->Pos;
}

void ImGui::SetWindowPos(ImGuiWindow* window, const ImVec2& pos, ImGuiCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowPosAllowFlags & cond) == 0)
        return;

    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
    window->SetWindowPosVal = ImVec2(FLT_MAX, FLT_MAX);

    // Set
    const ImVec2 old_pos = window->Pos;
    window->Pos = ImTrunc(pos);
    ImVec2 offset = window->Pos - old_pos;
    if (offset.x == 0.0f && offset.y == 0.0f)
        return;
    MarkIniSettingsDirty(window);
    window->DC.CursorPos += offset;         // As we happen to move the window while it is being appended to (which is a bad idea - will smear) let's at least offset the cursor
    window->DC.CursorMaxPos += offset;      // And more importantly we need to offset CursorMaxPos/CursorStartPos this so ContentSize calculation doesn't get affected.
    window->DC.IdealMaxPos += offset;
    window->DC.CursorStartPos += offset;
}

void ImGui::SetWindowPos(const ImVec2& pos, ImGuiCond cond)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    SetWindowPos(window, pos, cond);
}

void ImGui::SetWindowPos(const char* name, const ImVec2& pos, ImGuiCond cond)
{
    if (ImGuiWindow* window = FindWindowByName(name))
        SetWindowPos(window, pos, cond);
}

ImVec2 ImGui::GetWindowSize()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->Size;
}

void ImGui::SetWindowSize(ImGuiWindow* window, const ImVec2& size, ImGuiCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowSizeAllowFlags & cond) == 0)
        return;

    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    window->SetWindowSizeAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);

    // Enable auto-fit (not done in BeginChild() path unless appearing or combined with ImGuiChildFlags_AlwaysAutoResize)
    if ((window->Flags & ImGuiWindowFlags_ChildWindow) == 0 || window->Appearing || (window->ChildFlags & ImGuiChildFlags_AlwaysAutoResize) != 0)
        window->AutoFitFramesX = (size.x <= 0.0f) ? 2 : 0;
    if ((window->Flags & ImGuiWindowFlags_ChildWindow) == 0 || window->Appearing || (window->ChildFlags & ImGuiChildFlags_AlwaysAutoResize) != 0)
        window->AutoFitFramesY = (size.y <= 0.0f) ? 2 : 0;

    // Set
    ImVec2 old_size = window->SizeFull;
    if (size.x <= 0.0f)
        window->AutoFitOnlyGrows = false;
    else
        window->SizeFull.x = IM_TRUNC(size.x);
    if (size.y <= 0.0f)
        window->AutoFitOnlyGrows = false;
    else
        window->SizeFull.y = IM_TRUNC(size.y);
    if (old_size.x != window->SizeFull.x || old_size.y != window->SizeFull.y)
        MarkIniSettingsDirty(window);
}

void ImGui::SetWindowSize(const ImVec2& size, ImGuiCond cond)
{
    SetWindowSize(GImGui->CurrentWindow, size, cond);
}

void ImGui::SetWindowSize(const char* name, const ImVec2& size, ImGuiCond cond)
{
    if (ImGuiWindow* window = FindWindowByName(name))
        SetWindowSize(window, size, cond);
}

void ImGui::SetWindowCollapsed(ImGuiWindow* window, bool collapsed, ImGuiCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowCollapsedAllowFlags & cond) == 0)
        return;
    window->SetWindowCollapsedAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);

    // Set
    window->Collapsed = collapsed;
}

void ImGui::SetWindowHitTestHole(ImGuiWindow* window, const ImVec2& pos, const ImVec2& size)
{
    IM_ASSERT(window->HitTestHoleSize.x == 0);     // We don't support multiple holes/hit test filters
    window->HitTestHoleSize = ImVec2ih(size);
    window->HitTestHoleOffset = ImVec2ih(pos - window->Pos);
}

void ImGui::SetWindowHiddenAndSkipItemsForCurrentFrame(ImGuiWindow* window)
{
    window->Hidden = window->SkipItems = true;
    window->HiddenFramesCanSkipItems = 1;
}

void ImGui::SetWindowCollapsed(bool collapsed, ImGuiCond cond)
{
    SetWindowCollapsed(GImGui->CurrentWindow, collapsed, cond);
}

bool ImGui::IsWindowCollapsed()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->Collapsed;
}

bool ImGui::IsWindowAppearing()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->Appearing;
}

void ImGui::SetWindowCollapsed(const char* name, bool collapsed, ImGuiCond cond)
{
    if (ImGuiWindow* window = FindWindowByName(name))
        SetWindowCollapsed(window, collapsed, cond);
}

void ImGui::SetWindowFocus()
{
    FocusWindow(GImGui->CurrentWindow);
}

void ImGui::SetWindowFocus(const char* name)
{
    if (name)
    {
        if (ImGuiWindow* window = FindWindowByName(name))
            FocusWindow(window);
    }
    else
    {
        FocusWindow(NULL);
    }
}

void ImGui::SetNextWindowPos(const ImVec2& pos, ImGuiCond cond, const ImVec2& pivot)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasPos;
    g.NextWindowData.PosVal = pos;
    g.NextWindowData.PosPivotVal = pivot;
    g.NextWindowData.PosCond = cond ? cond : ImGuiCond_Always;
}

void ImGui::SetNextWindowSize(const ImVec2& size, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasSize;
    g.NextWindowData.SizeVal = size;
    g.NextWindowData.SizeCond = cond ? cond : ImGuiCond_Always;
}

// For each axis:
// - Use 0.0f as min or FLT_MAX as max if you don't want limits, e.g. size_min = (500.0f, 0.0f), size_max = (FLT_MAX, FLT_MAX) sets a minimum width.
// - Use -1 for both min and max of same axis to preserve current size which itself is a constraint.
// - See "Demo->Examples->Constrained-resizing window" for examples.
void ImGui::SetNextWindowSizeConstraints(const ImVec2& size_min, const ImVec2& size_max, ImGuiSizeCallback custom_callback, void* custom_callback_user_data)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasSizeConstraint;
    g.NextWindowData.SizeConstraintRect = ImRect(size_min, size_max);
    g.NextWindowData.SizeCallback = custom_callback;
    g.NextWindowData.SizeCallbackUserData = custom_callback_user_data;
}

// Content size = inner scrollable rectangle, padded with WindowPadding.
// SetNextWindowContentSize(ImVec2(100,100) + ImGuiWindowFlags_AlwaysAutoResize will always allow submitting a 100x100 item.
void ImGui::SetNextWindowContentSize(const ImVec2& size)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasContentSize;
    g.NextWindowData.ContentSizeVal = ImTrunc(size);
}

void ImGui::SetNextWindowScroll(const ImVec2& scroll)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasScroll;
    g.NextWindowData.ScrollVal = scroll;
}

void ImGui::SetNextWindowCollapsed(bool collapsed, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasCollapsed;
    g.NextWindowData.CollapsedVal = collapsed;
    g.NextWindowData.CollapsedCond = cond ? cond : ImGuiCond_Always;
}

void ImGui::SetNextWindowFocus()
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasFocus;
}

void ImGui::SetNextWindowBgAlpha(float alpha)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasBgAlpha;
    g.NextWindowData.BgAlphaVal = alpha;
}

// This is experimental and meant to be a toy for exploring a future/wider range of features.
void ImGui::SetNextWindowRefreshPolicy(ImGuiWindowRefreshFlags flags)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasRefreshPolicy;
    g.NextWindowData.RefreshFlagsVal = flags;
}

ImDrawList* ImGui::GetWindowDrawList()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DrawList;
}

ImFont* ImGui::GetFont()
{
    return GImGui->Font;
}

float ImGui::GetFontSize()
{
    return GImGui->FontSize;
}

ImVec2 ImGui::GetFontTexUvWhitePixel()
{
    return GImGui->DrawListSharedData.TexUvWhitePixel;
}

void ImGui::SetWindowFontScale(float scale)
{
    IM_ASSERT(scale > 0.0f);
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->FontWindowScale = scale;
    g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize();
    g.FontScale = g.DrawListSharedData.FontScale = g.FontSize / g.Font->FontSize;
}

void ImGui::PushFocusScope(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    ImGuiFocusScopeData data;
    data.ID = id;
    data.WindowID = g.CurrentWindow->ID;
    g.FocusScopeStack.push_back(data);
    g.CurrentFocusScopeId = id;
}

void ImGui::PopFocusScope()
{
    ImGuiContext& g = *GImGui;
    if (g.FocusScopeStack.Size <= g.StackSizesInBeginForCurrentWindow->SizeOfFocusScopeStack)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PopFocusScope() too many times!");
        return;
    }
    g.FocusScopeStack.pop_back();
    g.CurrentFocusScopeId = g.FocusScopeStack.Size ? g.FocusScopeStack.back().ID : 0;
}

void ImGui::SetNavFocusScope(ImGuiID focus_scope_id)
{
    ImGuiContext& g = *GImGui;
    g.NavFocusScopeId = focus_scope_id;
    g.NavFocusRoute.resize(0); // Invalidate
    if (focus_scope_id == 0)
        return;
    IM_ASSERT(g.NavWindow != NULL);

    // Store current path (in reverse order)
    if (focus_scope_id == g.CurrentFocusScopeId)
    {
        // Top of focus stack contains local focus scopes inside current window
        for (int n = g.FocusScopeStack.Size - 1; n >= 0 && g.FocusScopeStack.Data[n].WindowID == g.CurrentWindow->ID; n--)
            g.NavFocusRoute.push_back(g.FocusScopeStack.Data[n]);
    }
    else if (focus_scope_id == g.NavWindow->NavRootFocusScopeId)
        g.NavFocusRoute.push_back({ focus_scope_id, g.NavWindow->ID });
    else
        return;

    // Then follow on manually set ParentWindowForFocusRoute field (#6798)
    for (ImGuiWindow* window = g.NavWindow->ParentWindowForFocusRoute; window != NULL; window = window->ParentWindowForFocusRoute)
        g.NavFocusRoute.push_back({ window->NavRootFocusScopeId, window->ID });
    IM_ASSERT(g.NavFocusRoute.Size < 100); // Maximum depth is technically 251 as per CalcRoutingScore(): 254 - 3
}

// Focus = move navigation cursor, set scrolling, set focus window.
void ImGui::FocusItem()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IMGUI_DEBUG_LOG_FOCUS("FocusItem(0x%08x) in window \"%s\"\n", g.LastItemData.ID, window->Name);
    if (g.DragDropActive || g.MovingWindow != NULL) // FIXME: Opt-in flags for this?
    {
        IMGUI_DEBUG_LOG_FOCUS("FocusItem() ignored while DragDropActive!\n");
        return;
    }

    ImGuiNavMoveFlags move_flags = ImGuiNavMoveFlags_IsTabbing | ImGuiNavMoveFlags_FocusApi | ImGuiNavMoveFlags_NoSetNavHighlight | ImGuiNavMoveFlags_NoSelect;
    ImGuiScrollFlags scroll_flags = window->Appearing ? ImGuiScrollFlags_KeepVisibleEdgeX | ImGuiScrollFlags_AlwaysCenterY : ImGuiScrollFlags_KeepVisibleEdgeX | ImGuiScrollFlags_KeepVisibleEdgeY;
    SetNavWindow(window);
    NavMoveRequestSubmit(ImGuiDir_None, ImGuiDir_Up, move_flags, scroll_flags);
    NavMoveRequestResolveWithLastItem(&g.NavMoveResultLocal);
}

void ImGui::ActivateItemByID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    g.NavNextActivateId = id;
    g.NavNextActivateFlags = ImGuiActivateFlags_None;
}

// Note: this will likely be called ActivateItem() once we rework our Focus/Activation system!
// But ActivateItem() should function without altering scroll/focus?
void ImGui::SetKeyboardFocusHere(int offset)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(offset >= -1);    // -1 is allowed but not below
    IMGUI_DEBUG_LOG_FOCUS("SetKeyboardFocusHere(%d) in window \"%s\"\n", offset, window->Name);

    // It makes sense in the vast majority of cases to never interrupt a drag and drop.
    // When we refactor this function into ActivateItem() we may want to make this an option.
    // MovingWindow is protected from most user inputs using SetActiveIdUsingNavAndKeys(), but
    // is also automatically dropped in the event g.ActiveId is stolen.
    if (g.DragDropActive || g.MovingWindow != NULL)
    {
        IMGUI_DEBUG_LOG_FOCUS("SetKeyboardFocusHere() ignored while DragDropActive!\n");
        return;
    }

    SetNavWindow(window);

    ImGuiNavMoveFlags move_flags = ImGuiNavMoveFlags_IsTabbing | ImGuiNavMoveFlags_Activate | ImGuiNavMoveFlags_FocusApi | ImGuiNavMoveFlags_NoSetNavHighlight;
    ImGuiScrollFlags scroll_flags = window->Appearing ? ImGuiScrollFlags_KeepVisibleEdgeX | ImGuiScrollFlags_AlwaysCenterY : ImGuiScrollFlags_KeepVisibleEdgeX | ImGuiScrollFlags_KeepVisibleEdgeY;
    NavMoveRequestSubmit(ImGuiDir_None, offset < 0 ? ImGuiDir_Up : ImGuiDir_Down, move_flags, scroll_flags); // FIXME-NAV: Once we refactor tabbing, add LegacyApi flag to not activate non-inputable.
    if (offset == -1)
    {
        NavMoveRequestResolveWithLastItem(&g.NavMoveResultLocal);
    }
    else
    {
        g.NavTabbingDir = 1;
        g.NavTabbingCounter = offset + 1;
    }
}

void ImGui::SetItemDefaultFocus()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!window->Appearing)
        return;
    if (g.NavWindow != window->RootWindowForNav || (!g.NavInitRequest && g.NavInitResult.ID == 0) || g.NavLayer != window->DC.NavLayerCurrent)
        return;

    g.NavInitRequest = false;
    NavApplyItemToResult(&g.NavInitResult);
    NavUpdateAnyRequestFlag();

    // Scroll could be done in NavInitRequestApplyResult() via an opt-in flag (we however don't want regular init requests to scroll)
    if (!window->ClipRect.Contains(g.LastItemData.Rect))
        ScrollToRectEx(window, g.LastItemData.Rect, ImGuiScrollFlags_None);
}

void ImGui::SetStateStorage(ImGuiStorage* tree)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    window->DC.StateStorage = tree ? tree : &window->StateStorage;
}

ImGuiStorage* ImGui::GetStateStorage()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->DC.StateStorage;
}

bool ImGui::IsRectVisible(const ImVec2& size)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ClipRect.Overlaps(ImRect(window->DC.CursorPos, window->DC.CursorPos + size));
}

bool ImGui::IsRectVisible(const ImVec2& rect_min, const ImVec2& rect_max)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ClipRect.Overlaps(ImRect(rect_min, rect_max));
}

//-----------------------------------------------------------------------------
// [SECTION] ID STACK
//-----------------------------------------------------------------------------

// This is one of the very rare legacy case where we use ImGuiWindow methods,
// it should ideally be flattened at some point but it's been used a lots by widgets.
IM_MSVC_RUNTIME_CHECKS_OFF
ImGuiID ImGuiWindow::GetID(const char* str, const char* str_end)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashStr(str, str_end ? (str_end - str) : 0, seed);
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    ImGuiContext& g = *Ctx;
    if (g.DebugHookIdInfo == id)
        ImGui::DebugHookIdInfo(id, ImGuiDataType_String, str, str_end);
#endif
    return id;
}

ImGuiID ImGuiWindow::GetID(const void* ptr)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashData(&ptr, sizeof(void*), seed);
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    ImGuiContext& g = *Ctx;
    if (g.DebugHookIdInfo == id)
        ImGui::DebugHookIdInfo(id, ImGuiDataType_Pointer, ptr, NULL);
#endif
    return id;
}

ImGuiID ImGuiWindow::GetID(int n)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashData(&n, sizeof(n), seed);
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    ImGuiContext& g = *Ctx;
    if (g.DebugHookIdInfo == id)
        ImGui::DebugHookIdInfo(id, ImGuiDataType_S32, (void*)(intptr_t)n, NULL);
#endif
    return id;
}

// This is only used in rare/specific situations to manufacture an ID out of nowhere.
// FIXME: Consider instead storing last non-zero ID + count of successive zero-ID, and combine those?
ImGuiID ImGuiWindow::GetIDFromPos(const ImVec2& p_abs)
{
    ImGuiID seed = IDStack.back();
    ImVec2 p_rel = ImGui::WindowPosAbsToRel(this, p_abs);
    ImGuiID id = ImHashData(&p_rel, sizeof(p_rel), seed);
    return id;
}

// "
ImGuiID ImGuiWindow::GetIDFromRectangle(const ImRect& r_abs)
{
    ImGuiID seed = IDStack.back();
    ImRect r_rel = ImGui::WindowRectAbsToRel(this, r_abs);
    ImGuiID id = ImHashData(&r_rel, sizeof(r_rel), seed);
    return id;
}

void ImGui::PushID(const char* str_id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID(str_id);
    window->IDStack.push_back(id);
}

void ImGui::PushID(const char* str_id_begin, const char* str_id_end)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID(str_id_begin, str_id_end);
    window->IDStack.push_back(id);
}

void ImGui::PushID(const void* ptr_id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID(ptr_id);
    window->IDStack.push_back(id);
}

void ImGui::PushID(int int_id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID(int_id);
    window->IDStack.push_back(id);
}

// Push a given id value ignoring the ID stack as a seed.
void ImGui::PushOverrideID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    if (g.DebugHookIdInfo == id)
        DebugHookIdInfo(id, ImGuiDataType_ID, NULL, NULL);
#endif
    window->IDStack.push_back(id);
}

// Helper to avoid a common series of PushOverrideID -> GetID() -> PopID() call
// (note that when using this pattern, ID Stack Tool will tend to not display the intermediate stack level.
//  for that to work we would need to do PushOverrideID() -> ItemAdd() -> PopID() which would alter widget code a little more)
ImGuiID ImGui::GetIDWithSeed(const char* str, const char* str_end, ImGuiID seed)
{
    ImGuiID id = ImHashStr(str, str_end ? (str_end - str) : 0, seed);
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    ImGuiContext& g = *GImGui;
    if (g.DebugHookIdInfo == id)
        DebugHookIdInfo(id, ImGuiDataType_String, str, str_end);
#endif
    return id;
}

ImGuiID ImGui::GetIDWithSeed(int n, ImGuiID seed)
{
    ImGuiID id = ImHashData(&n, sizeof(n), seed);
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    ImGuiContext& g = *GImGui;
    if (g.DebugHookIdInfo == id)
        DebugHookIdInfo(id, ImGuiDataType_S32, (void*)(intptr_t)n, NULL);
#endif
    return id;
}

void ImGui::PopID()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    if (window->IDStack.Size <= 1)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PopID() too many times!");
        return;
    }
    window->IDStack.pop_back();
}

ImGuiID ImGui::GetID(const char* str_id)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->GetID(str_id);
}

ImGuiID ImGui::GetID(const char* str_id_begin, const char* str_id_end)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->GetID(str_id_begin, str_id_end);
}

ImGuiID ImGui::GetID(const void* ptr_id)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->GetID(ptr_id);
}

ImGuiID ImGui::GetID(int int_id)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->GetID(int_id);
}
IM_MSVC_RUNTIME_CHECKS_RESTORE

//-----------------------------------------------------------------------------
// [SECTION] INPUTS
//-----------------------------------------------------------------------------
// - GetModForLRModKey() [Internal]
// - FixupKeyChord() [Internal]
// - GetKeyData() [Internal]
// - GetKeyIndex() [Internal]
// - GetKeyName()
// - GetKeyChordName() [Internal]
// - CalcTypematicRepeatAmount() [Internal]
// - GetTypematicRepeatRate() [Internal]
// - GetKeyPressedAmount() [Internal]
// - GetKeyMagnitude2d() [Internal]
//-----------------------------------------------------------------------------
// - UpdateKeyRoutingTable() [Internal]
// - GetRoutingIdFromOwnerId() [Internal]
// - GetShortcutRoutingData() [Internal]
// - CalcRoutingScore() [Internal]
// - SetShortcutRouting() [Internal]
// - TestShortcutRouting() [Internal]
//-----------------------------------------------------------------------------
// - IsKeyDown()
// - IsKeyPressed()
// - IsKeyReleased()
//-----------------------------------------------------------------------------
// - IsMouseDown()
// - IsMouseClicked()
// - IsMouseReleased()
// - IsMouseDoubleClicked()
// - GetMouseClickedCount()
// - IsMouseHoveringRect() [Internal]
// - IsMouseDragPastThreshold() [Internal]
// - IsMouseDragging()
// - GetMousePos()
// - SetMousePos() [Internal]
// - GetMousePosOnOpeningCurrentPopup()
// - IsMousePosValid()
// - IsAnyMouseDown()
// - GetMouseDragDelta()
// - ResetMouseDragDelta()
// - GetMouseCursor()
// - SetMouseCursor()
//-----------------------------------------------------------------------------
// - UpdateAliasKey()
// - GetMergedModsFromKeys()
// - UpdateKeyboardInputs()
// - UpdateMouseInputs()
//-----------------------------------------------------------------------------
// - LockWheelingWindow [Internal]
// - FindBestWheelingWindow [Internal]
// - UpdateMouseWheel() [Internal]
//-----------------------------------------------------------------------------
// - SetNextFrameWantCaptureKeyboard()
// - SetNextFrameWantCaptureMouse()
//-----------------------------------------------------------------------------
// - GetInputSourceName() [Internal]
// - DebugPrintInputEvent() [Internal]
// - UpdateInputEvents() [Internal]
//-----------------------------------------------------------------------------
// - GetKeyOwner() [Internal]
// - TestKeyOwner() [Internal]
// - SetKeyOwner() [Internal]
// - SetItemKeyOwner() [Internal]
// - Shortcut() [Internal]
//-----------------------------------------------------------------------------

static ImGuiKeyChord GetModForLRModKey(ImGuiKey key)
{
    if (key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl)
        return ImGuiMod_Ctrl;
    if (key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift)
        return ImGuiMod_Shift;
    if (key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt)
        return ImGuiMod_Alt;
    if (key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper)
        return ImGuiMod_Super;
    return ImGuiMod_None;
}

ImGuiKeyChord ImGui::FixupKeyChord(ImGuiKeyChord key_chord)
{
    // Add ImGuiMod_XXXX when a corresponding ImGuiKey_LeftXXX/ImGuiKey_RightXXX is specified.
    ImGuiKey key = (ImGuiKey)(key_chord & ~ImGuiMod_Mask_);
    if (IsLRModKey(key))
        key_chord |= GetModForLRModKey(key);
    return key_chord;
}

ImGuiKeyData* ImGui::GetKeyData(ImGuiContext* ctx, ImGuiKey key)
{
    ImGuiContext& g = *ctx;

    // Special storage location for mods
    if (key & ImGuiMod_Mask_)
        key = ConvertSingleModFlagToKey(key);

#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    IM_ASSERT(key >= ImGuiKey_LegacyNativeKey_BEGIN && key < ImGuiKey_NamedKey_END);
    if (IsLegacyKey(key) && g.IO.KeyMap[key] != -1)
        key = (ImGuiKey)g.IO.KeyMap[key];  // Remap native->imgui or imgui->native
#else
    IM_ASSERT(IsNamedKey(key) && "Support for user key indices was dropped in favor of ImGuiKey. Please update backend & user code.");
#endif
    return &g.IO.KeysData[key - ImGuiKey_KeysData_OFFSET];
}

#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
// Formally moved to obsolete section in 1.90.5 in spite of documented as obsolete since 1.87
ImGuiKey ImGui::GetKeyIndex(ImGuiKey key)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(IsNamedKey(key));
    const ImGuiKeyData* key_data = GetKeyData(key);
    return (ImGuiKey)(key_data - g.IO.KeysData);
}
#endif

// Those names a provided for debugging purpose and are not meant to be saved persistently not compared.
static const char* const GKeyNames[] =
{
    "Tab", "LeftArrow", "RightArrow", "UpArrow", "DownArrow", "PageUp", "PageDown",
    "Home", "End", "Insert", "Delete", "Backspace", "Space", "Enter", "Escape",
    "LeftCtrl", "LeftShift", "LeftAlt", "LeftSuper", "RightCtrl", "RightShift", "RightAlt", "RightSuper", "Menu",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G", "H",
    "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
    "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24",
    "Apostrophe", "Comma", "Minus", "Period", "Slash", "Semicolon", "Equal", "LeftBracket",
    "Backslash", "RightBracket", "GraveAccent", "CapsLock", "ScrollLock", "NumLock", "PrintScreen",
    "Pause", "Keypad0", "Keypad1", "Keypad2", "Keypad3", "Keypad4", "Keypad5", "Keypad6",
    "Keypad7", "Keypad8", "Keypad9", "KeypadDecimal", "KeypadDivide", "KeypadMultiply",
    "KeypadSubtract", "KeypadAdd", "KeypadEnter", "KeypadEqual",
    "AppBack", "AppForward",
    "GamepadStart", "GamepadBack",
    "GamepadFaceLeft", "GamepadFaceRight", "GamepadFaceUp", "GamepadFaceDown",
    "GamepadDpadLeft", "GamepadDpadRight", "GamepadDpadUp", "GamepadDpadDown",
    "GamepadL1", "GamepadR1", "GamepadL2", "GamepadR2", "GamepadL3", "GamepadR3",
    "GamepadLStickLeft", "GamepadLStickRight", "GamepadLStickUp", "GamepadLStickDown",
    "GamepadRStickLeft", "GamepadRStickRight", "GamepadRStickUp", "GamepadRStickDown",
    "MouseLeft", "MouseRight", "MouseMiddle", "MouseX1", "MouseX2", "MouseWheelX", "MouseWheelY",
    "ModCtrl", "ModShift", "ModAlt", "ModSuper", // ReservedForModXXX are showing the ModXXX names.
};
IM_STATIC_ASSERT(ImGuiKey_NamedKey_COUNT == IM_ARRAYSIZE(GKeyNames));

const char* ImGui::GetKeyName(ImGuiKey key)
{
    if (key == ImGuiKey_None)
        return "None";
#ifdef IMGUI_DISABLE_OBSOLETE_KEYIO
    IM_ASSERT(IsNamedKeyOrMod(key) && "Support for user key indices was dropped in favor of ImGuiKey. Please update backend and user code.");
#else
    ImGuiContext& g = *GImGui;
    if (IsLegacyKey(key))
    {
        if (g.IO.KeyMap[key] == -1)
            return "N/A";
        IM_ASSERT(IsNamedKey((ImGuiKey)g.IO.KeyMap[key]));
        key = (ImGuiKey)g.IO.KeyMap[key];
    }
#endif
    if (key & ImGuiMod_Mask_)
        key = ConvertSingleModFlagToKey(key);
    if (!IsNamedKey(key))
        return "Unknown";

    return GKeyNames[key - ImGuiKey_NamedKey_BEGIN];
}

// Return untranslated names: on macOS, Cmd key will show as Ctrl, Ctrl key will show as super.
// Lifetime of return value: valid until next call to same function.
const char* ImGui::GetKeyChordName(ImGuiKeyChord key_chord)
{
    ImGuiContext& g = *GImGui;

    const ImGuiKey key = (ImGuiKey)(key_chord & ~ImGuiMod_Mask_);
    if (IsLRModKey(key))
        key_chord &= ~GetModForLRModKey(key); // Return "Ctrl+LeftShift" instead of "Ctrl+Shift+LeftShift"
    ImFormatString(g.TempKeychordName, IM_ARRAYSIZE(g.TempKeychordName), "%s%s%s%s%s",
        (key_chord & ImGuiMod_Ctrl) ? "Ctrl+" : "",
        (key_chord & ImGuiMod_Shift) ? "Shift+" : "",
        (key_chord & ImGuiMod_Alt) ? "Alt+" : "",
        (key_chord & ImGuiMod_Super) ? "Super+" : "",
        (key != ImGuiKey_None || key_chord == ImGuiKey_None) ? GetKeyName(key) : "");
    size_t len;
    if (key == ImGuiKey_None && key_chord != 0)
        if ((len = strlen(g.TempKeychordName)) != 0) // Remove trailing '+'
            g.TempKeychordName[len - 1] = 0;
    return g.TempKeychordName;
}

// t0 = previous time (e.g.: g.Time - g.IO.DeltaTime)
// t1 = current time (e.g.: g.Time)
// An event is triggered at:
//  t = 0.0f     t = repeat_delay,    t = repeat_delay + repeat_rate*N
int ImGui::CalcTypematicRepeatAmount(float t0, float t1, float repeat_delay, float repeat_rate)
{
    if (t1 == 0.0f)
        return 1;
    if (t0 >= t1)
        return 0;
    if (repeat_rate <= 0.0f)
        return (t0 < repeat_delay) && (t1 >= repeat_delay);
    const int count_t0 = (t0 < repeat_delay) ? -1 : (int)((t0 - repeat_delay) / repeat_rate);
    const int count_t1 = (t1 < repeat_delay) ? -1 : (int)((t1 - repeat_delay) / repeat_rate);
    const int count = count_t1 - count_t0;
    return count;
}

void ImGui::GetTypematicRepeatRate(ImGuiInputFlags flags, float* repeat_delay, float* repeat_rate)
{
    ImGuiContext& g = *GImGui;
    switch (flags & ImGuiInputFlags_RepeatRateMask_)
    {
    case ImGuiInputFlags_RepeatRateNavMove:             *repeat_delay = g.IO.KeyRepeatDelay * 0.72f; *repeat_rate = g.IO.KeyRepeatRate * 0.80f; return;
    case ImGuiInputFlags_RepeatRateNavTweak:            *repeat_delay = g.IO.KeyRepeatDelay * 0.72f; *repeat_rate = g.IO.KeyRepeatRate * 0.30f; return;
    case ImGuiInputFlags_RepeatRateDefault: default:    *repeat_delay = g.IO.KeyRepeatDelay * 1.00f; *repeat_rate = g.IO.KeyRepeatRate * 1.00f; return;
    }
}

// Return value representing the number of presses in the last time period, for the given repeat rate
// (most often returns 0 or 1. The result is generally only >1 when RepeatRate is smaller than DeltaTime, aka large DeltaTime or fast RepeatRate)
int ImGui::GetKeyPressedAmount(ImGuiKey key, float repeat_delay, float repeat_rate)
{
    ImGuiContext& g = *GImGui;
    const ImGuiKeyData* key_data = GetKeyData(key);
    if (!key_data->Down) // In theory this should already be encoded as (DownDuration < 0.0f), but testing this facilitates eating mechanism (until we finish work on key ownership)
        return 0;
    const float t = key_data->DownDuration;
    return CalcTypematicRepeatAmount(t - g.IO.DeltaTime, t, repeat_delay, repeat_rate);
}

// Return 2D vector representing the combination of four cardinal direction, with analog value support (for e.g. ImGuiKey_GamepadLStick* values).
ImVec2 ImGui::GetKeyMagnitude2d(ImGuiKey key_left, ImGuiKey key_right, ImGuiKey key_up, ImGuiKey key_down)
{
    return ImVec2(
        GetKeyData(key_right)->AnalogValue - GetKeyData(key_left)->AnalogValue,
        GetKeyData(key_down)->AnalogValue - GetKeyData(key_up)->AnalogValue);
}

// Rewrite routing data buffers to strip old entries + sort by key to make queries not touch scattered data.
//   Entries   D,A,B,B,A,C,B     --> A,A,B,B,B,C,D
//   Index     A:1 B:2 C:5 D:0   --> A:0 B:2 C:5 D:6
// See 'Metrics->Key Owners & Shortcut Routing' to visualize the result of that operation.
static void ImGui::UpdateKeyRoutingTable(ImGuiKeyRoutingTable* rt)
{
    ImGuiContext& g = *GImGui;
    rt->EntriesNext.resize(0);
    for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1))
    {
        const int new_routing_start_idx = rt->EntriesNext.Size;
        ImGuiKeyRoutingData* routing_entry;
        for (int old_routing_idx = rt->Index[key - ImGuiKey_NamedKey_BEGIN]; old_routing_idx != -1; old_routing_idx = routing_entry->NextEntryIndex)
        {
            routing_entry = &rt->Entries[old_routing_idx];
            routing_entry->RoutingCurrScore = routing_entry->RoutingNextScore;
            routing_entry->RoutingCurr = routing_entry->RoutingNext; // Update entry
            routing_entry->RoutingNext = ImGuiKeyOwner_NoOwner;
            routing_entry->RoutingNextScore = 255;
            if (routing_entry->RoutingCurr == ImGuiKeyOwner_NoOwner)
                continue;
            rt->EntriesNext.push_back(*routing_entry); // Write alive ones into new buffer

            // Apply routing to owner if there's no owner already (RoutingCurr == None at this point)
            // This is the result of previous frame's SetShortcutRouting() call.
            if (routing_entry->Mods == g.IO.KeyMods)
            {
                ImGuiKeyOwnerData* owner_data = GetKeyOwnerData(&g, key);
                if (owner_data->OwnerCurr == ImGuiKeyOwner_NoOwner)
                {
                    owner_data->OwnerCurr = routing_entry->RoutingCurr;
                    //IMGUI_DEBUG_LOG("SetKeyOwner(%s, owner_id=0x%08X) via Routing\n", GetKeyName(key), routing_entry->RoutingCurr);
                }
            }
        }

        // Rewrite linked-list
        rt->Index[key - ImGuiKey_NamedKey_BEGIN] = (ImGuiKeyRoutingIndex)(new_routing_start_idx < rt->EntriesNext.Size ? new_routing_start_idx : -1);
        for (int n = new_routing_start_idx; n < rt->EntriesNext.Size; n++)
            rt->EntriesNext[n].NextEntryIndex = (ImGuiKeyRoutingIndex)((n + 1 < rt->EntriesNext.Size) ? n + 1 : -1);
    }
    rt->Entries.swap(rt->EntriesNext); // Swap new and old indexes
}

// owner_id may be None/Any, but routing_id needs to be always be set, so we default to GetCurrentFocusScope().
static inline ImGuiID GetRoutingIdFromOwnerId(ImGuiID owner_id)
{
    ImGuiContext& g = *GImGui;
    return (owner_id != ImGuiKeyOwner_NoOwner && owner_id != ImGuiKeyOwner_Any) ? owner_id : g.CurrentFocusScopeId;
}

ImGuiKeyRoutingData* ImGui::GetShortcutRoutingData(ImGuiKeyChord key_chord)
{
    // Majority of shortcuts will be Key + any number of Mods
    // We accept _Single_ mod with ImGuiKey_None.
    //  - Shortcut(ImGuiKey_S | ImGuiMod_Ctrl);                    // Legal
    //  - Shortcut(ImGuiKey_S | ImGuiMod_Ctrl | ImGuiMod_Shift);   // Legal
    //  - Shortcut(ImGuiMod_Ctrl);                                 // Legal
    //  - Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift);                // Not legal
    ImGuiContext& g = *GImGui;
    ImGuiKeyRoutingTable* rt = &g.KeysRoutingTable;
    ImGuiKeyRoutingData* routing_data;
    ImGuiKey key = (ImGuiKey)(key_chord & ~ImGuiMod_Mask_);
    ImGuiKey mods = (ImGuiKey)(key_chord & ImGuiMod_Mask_);
    if (key == ImGuiKey_None)
        key = ConvertSingleModFlagToKey(mods);
    IM_ASSERT(IsNamedKey(key));

    // Get (in the majority of case, the linked list will have one element so this should be 2 reads.
    // Subsequent elements will be contiguous in memory as list is sorted/rebuilt in NewFrame).
    for (ImGuiKeyRoutingIndex idx = rt->Index[key - ImGuiKey_NamedKey_BEGIN]; idx != -1; idx = routing_data->NextEntryIndex)
    {
        routing_data = &rt->Entries[idx];
        if (routing_data->Mods == mods)
            return routing_data;
    }

    // Add to linked-list
    ImGuiKeyRoutingIndex routing_data_idx = (ImGuiKeyRoutingIndex)rt->Entries.Size;
    rt->Entries.push_back(ImGuiKeyRoutingData());
    routing_data = &rt->Entries[routing_data_idx];
    routing_data->Mods = (ImU16)mods;
    routing_data->NextEntryIndex = rt->Index[key - ImGuiKey_NamedKey_BEGIN]; // Setup linked list
    rt->Index[key - ImGuiKey_NamedKey_BEGIN] = routing_data_idx;
    return routing_data;
}

// Current score encoding (lower is highest priority):
//  -   0: ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverActive
//  -   1: ImGuiInputFlags_ActiveItem or ImGuiInputFlags_RouteFocused (if item active)
//  -   2: ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused
//  -  3+: ImGuiInputFlags_RouteFocused (if window in focus-stack)
//  - 254: ImGuiInputFlags_RouteGlobal
//  - 255: never route
// 'flags' should include an explicit routing policy
static int CalcRoutingScore(ImGuiID focus_scope_id, ImGuiID owner_id, ImGuiInputFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (flags & ImGuiInputFlags_RouteFocused)
    {
        // ActiveID gets top priority
        // (we don't check g.ActiveIdUsingAllKeys here. Routing is applied but if input ownership is tested later it may discard it)
        if (owner_id != 0 && g.ActiveId == owner_id)
            return 1;

        // Score based on distance to focused window (lower is better)
        // Assuming both windows are submitting a routing request,
        // - When Window....... is focused -> Window scores 3 (best), Window/ChildB scores 255 (no match)
        // - When Window/ChildB is focused -> Window scores 4,        Window/ChildB scores 3 (best)
        // Assuming only WindowA is submitting a routing request,
        // - When Window/ChildB is focused -> Window scores 4 (best), Window/ChildB doesn't have a score.
        // This essentially follow the window->ParentWindowForFocusRoute chain.
        if (focus_scope_id == 0)
            return 255;
        for (int index_in_focus_path = 0; index_in_focus_path < g.NavFocusRoute.Size; index_in_focus_path++)
            if (g.NavFocusRoute.Data[index_in_focus_path].ID == focus_scope_id)
                return 3 + index_in_focus_path;
        return 255;
    }
    else if (flags & ImGuiInputFlags_RouteActive)
    {
        if (owner_id != 0 && g.ActiveId == owner_id)
            return 1;
        return 255;
    }
    else if (flags & ImGuiInputFlags_RouteGlobal)
    {
        if (flags & ImGuiInputFlags_RouteOverActive)
            return 0;
        if (flags & ImGuiInputFlags_RouteOverFocused)
            return 2;
        return 254;
    }
    IM_ASSERT(0);
    return 0;
}

// - We need this to filter some Shortcut() routes when an item e.g. an InputText() is active
//   e.g. ImGuiKey_G won't be considered a shortcut when item is active, but ImGuiMod|ImGuiKey_G can be.
// - This is also used by UpdateInputEvents() to avoid trickling in the most common case of e.g. pressing ImGuiKey_G also emitting a G character.
static bool IsKeyChordPotentiallyCharInput(ImGuiKeyChord key_chord)
{
    // Mimic 'ignore_char_inputs' logic in InputText()
    ImGuiContext& g = *GImGui;

    // When the right mods are pressed it cannot be a char input so we won't filter the shortcut out.
    ImGuiKey mods = (ImGuiKey)(key_chord & ImGuiMod_Mask_);
    const bool ignore_char_inputs = ((mods & ImGuiMod_Ctrl) && !(mods & ImGuiMod_Alt)) || (g.IO.ConfigMacOSXBehaviors && (mods & ImGuiMod_Ctrl));
    if (ignore_char_inputs)
        return false;

    // Return true for A-Z, 0-9 and other keys associated to char inputs. Other keys such as F1-F12 won't be filtered.
    ImGuiKey key = (ImGuiKey)(key_chord & ~ImGuiMod_Mask_);
    if (key == ImGuiKey_None)
        return false;
    return g.KeysMayBeCharInput.TestBit(key);
}

// Request a desired route for an input chord (key + mods).
// Return true if the route is available this frame.
// - Routes and key ownership are attributed at the beginning of next frame based on best score and mod state.
//   (Conceptually this does a "Submit for next frame" + "Test for current frame".
//   As such, it could be called TrySetXXX or SubmitXXX, or the Submit and Test operations should be separate.)
bool ImGui::SetShortcutRouting(ImGuiKeyChord key_chord, ImGuiInputFlags flags, ImGuiID owner_id)
{
    ImGuiContext& g = *GImGui;
    if ((flags & ImGuiInputFlags_RouteTypeMask_) == 0)
        flags |= ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused | ImGuiInputFlags_RouteOverActive; // IMPORTANT: This is the default for SetShortcutRouting() but NOT Shortcut()
    else
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiInputFlags_RouteTypeMask_)); // Check that only 1 routing flag is used
    IM_ASSERT(owner_id != ImGuiKeyOwner_Any && owner_id != ImGuiKeyOwner_NoOwner);
    if (flags & (ImGuiInputFlags_RouteOverFocused | ImGuiInputFlags_RouteOverActive | ImGuiInputFlags_RouteUnlessBgFocused))
        IM_ASSERT(flags & ImGuiInputFlags_RouteGlobal);

    // Add ImGuiMod_XXXX when a corresponding ImGuiKey_LeftXXX/ImGuiKey_RightXXX is specified.
    key_chord = FixupKeyChord(key_chord);

    // [DEBUG] Debug break requested by user
    if (g.DebugBreakInShortcutRouting == key_chord)
        IM_DEBUG_BREAK();

    if (flags & ImGuiInputFlags_RouteUnlessBgFocused)
        if (g.NavWindow == NULL)
            return false;

    // Note how ImGuiInputFlags_RouteAlways won't set routing and thus won't set owner. May want to rework this?
    if (flags & ImGuiInputFlags_RouteAlways)
    {
        IMGUI_DEBUG_LOG_INPUTROUTING("SetShortcutRouting(%s, flags=%04X, owner_id=0x%08X) -> always, no register\n", GetKeyChordName(key_chord), flags, owner_id);
        return true;
    }

    // Specific culling when there's an active item.
    if (g.ActiveId != 0 && g.ActiveId != owner_id)
    {
        if (flags & ImGuiInputFlags_RouteActive)
            return false;

        // Cull shortcuts with no modifiers when it could generate a character.
        // e.g. Shortcut(ImGuiKey_G) also generates 'g' character, should not trigger when InputText() is active.
        // but  Shortcut(Ctrl+G) should generally trigger when InputText() is active.
        // TL;DR: lettered shortcut with no mods or with only Alt mod will not trigger while an item reading text input is active.
        // (We cannot filter based on io.InputQueueCharacters[] contents because of trickling and key<>chars submission order are undefined)
        if (g.IO.WantTextInput && IsKeyChordPotentiallyCharInput(key_chord))
        {
            IMGUI_DEBUG_LOG_INPUTROUTING("SetShortcutRouting(%s, flags=%04X, owner_id=0x%08X) -> filtered as potential char input\n", GetKeyChordName(key_chord), flags, owner_id);
            return false;
        }

        // ActiveIdUsingAllKeyboardKeys trumps all for ActiveId
        if ((flags & ImGuiInputFlags_RouteOverActive) == 0 && g.ActiveIdUsingAllKeyboardKeys)
        {
            ImGuiKey key = (ImGuiKey)(key_chord & ~ImGuiMod_Mask_);
            if (key == ImGuiKey_None)
                key = ConvertSingleModFlagToKey((ImGuiKey)(key_chord & ImGuiMod_Mask_));
            if (key >= ImGuiKey_Keyboard_BEGIN && key < ImGuiKey_Keyboard_END)
                return false;
        }
    }

    // Where do we evaluate route for?
    ImGuiID focus_scope_id = g.CurrentFocusScopeId;
    if (flags & ImGuiInputFlags_RouteFromRootWindow)
        focus_scope_id = g.CurrentWindow->RootWindow->ID; // See PushFocusScope() call in Begin()

    const int score = CalcRoutingScore(focus_scope_id, owner_id, flags);
    IMGUI_DEBUG_LOG_INPUTROUTING("SetShortcutRouting(%s, flags=%04X, owner_id=0x%08X) -> score %d\n", GetKeyChordName(key_chord), flags, owner_id, score);
    if (score == 255)
        return false;

    // Submit routing for NEXT frame (assuming score is sufficient)
    // FIXME: Could expose a way to use a "serve last" policy for same score resolution (using <= instead of <).
    ImGuiKeyRoutingData* routing_data = GetShortcutRoutingData(key_chord);
    //const bool set_route = (flags & ImGuiInputFlags_ServeLast) ? (score <= routing_data->RoutingNextScore) : (score < routing_data->RoutingNextScore);
    if (score < routing_data->RoutingNextScore)
    {
        routing_data->RoutingNext = owner_id;
        routing_data->RoutingNextScore = (ImU8)score;
    }

    // Return routing state for CURRENT frame
    if (routing_data->RoutingCurr == owner_id)
        IMGUI_DEBUG_LOG_INPUTROUTING("--> granting current route\n");
    return routing_data->RoutingCurr == owner_id;
}

// Currently unused by core (but used by tests)
// Note: this cannot be turned into GetShortcutRouting() because we do the owner_id->routing_id translation, name would be more misleading.
bool ImGui::TestShortcutRouting(ImGuiKeyChord key_chord, ImGuiID owner_id)
{
    const ImGuiID routing_id = GetRoutingIdFromOwnerId(owner_id);
    key_chord = FixupKeyChord(key_chord);
    ImGuiKeyRoutingData* routing_data = GetShortcutRoutingData(key_chord); // FIXME: Could avoid creating entry.
    return routing_data->RoutingCurr == routing_id;
}

// Note that Dear ImGui doesn't know the meaning/semantic of ImGuiKey from 0..511: they are legacy native keycodes.
// Consider transitioning from 'IsKeyDown(MY_ENGINE_KEY_A)' (<1.87) to IsKeyDown(ImGuiKey_A) (>= 1.87)
bool ImGui::IsKeyDown(ImGuiKey key)
{
    return IsKeyDown(key, ImGuiKeyOwner_Any);
}

bool ImGui::IsKeyDown(ImGuiKey key, ImGuiID owner_id)
{
    const ImGuiKeyData* key_data = GetKeyData(key);
    if (!key_data->Down)
        return false;
    if (!TestKeyOwner(key, owner_id))
        return false;
    return true;
}

bool ImGui::IsKeyPressed(ImGuiKey key, bool repeat)
{
    return IsKeyPressed(key, repeat ? ImGuiInputFlags_Repeat : ImGuiInputFlags_None, ImGuiKeyOwner_Any);
}

// Important: unlike legacy IsKeyPressed(ImGuiKey, bool repeat=true) which DEFAULT to repeat, this requires EXPLICIT repeat.
bool ImGui::IsKeyPressed(ImGuiKey key, ImGuiInputFlags flags, ImGuiID owner_id)
{
    const ImGuiKeyData* key_data = GetKeyData(key);
    if (!key_data->Down) // In theory this should already be encoded as (DownDuration < 0.0f), but testing this facilitates eating mechanism (until we finish work on key ownership)
        return false;
    const float t = key_data->DownDuration;
    if (t < 0.0f)
        return false;
    IM_ASSERT((flags & ~ImGuiInputFlags_SupportedByIsKeyPressed) == 0); // Passing flags not supported by this function!
    if (flags & (ImGuiInputFlags_RepeatRateMask_ | ImGuiInputFlags_RepeatUntilMask_)) // Setting any _RepeatXXX option enables _Repeat
        flags |= ImGuiInputFlags_Repeat;

    bool pressed = (t == 0.0f);
    if (!pressed && (flags & ImGuiInputFlags_Repeat) != 0)
    {
        float repeat_delay, repeat_rate;
        GetTypematicRepeatRate(flags, &repeat_delay, &repeat_rate);
        pressed = (t > repeat_delay) && GetKeyPressedAmount(key, repeat_delay, repeat_rate) > 0;
        if (pressed && (flags & ImGuiInputFlags_RepeatUntilMask_))
        {
            // Slightly bias 'key_pressed_time' as DownDuration is an accumulation of DeltaTime which we compare to an absolute time value.
            // Ideally we'd replace DownDuration with KeyPressedTime but it would break user's code.
            ImGuiContext& g = *GImGui;
            double key_pressed_time = g.Time - t + 0.00001f;
            if ((flags & ImGuiInputFlags_RepeatUntilKeyModsChange) && (g.LastKeyModsChangeTime > key_pressed_time))
                pressed = false;
            if ((flags & ImGuiInputFlags_RepeatUntilKeyModsChangeFromNone) && (g.LastKeyModsChangeFromNoneTime > key_pressed_time))
                pressed = false;
            if ((flags & ImGuiInputFlags_RepeatUntilOtherKeyPress) && (g.LastKeyboardKeyPressTime > key_pressed_time))
                pressed = false;
        }
    }
    if (!pressed)
        return false;
    if (!TestKeyOwner(key, owner_id))
        return false;
    return true;
}

bool ImGui::IsKeyReleased(ImGuiKey key)
{
    return IsKeyReleased(key, ImGuiKeyOwner_Any);
}

bool ImGui::IsKeyReleased(ImGuiKey key, ImGuiID owner_id)
{
    const ImGuiKeyData* key_data = GetKeyData(key);
    if (key_data->DownDurationPrev < 0.0f || key_data->Down)
        return false;
    if (!TestKeyOwner(key, owner_id))
        return false;
    return true;
}

bool ImGui::IsMouseDown(ImGuiMouseButton button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseDown[button] && TestKeyOwner(MouseButtonToKey(button), ImGuiKeyOwner_Any); // should be same as IsKeyDown(MouseButtonToKey(button), ImGuiKeyOwner_Any), but this allows legacy code hijacking the io.Mousedown[] array.
}

bool ImGui::IsMouseDown(ImGuiMouseButton button, ImGuiID owner_id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseDown[button] && TestKeyOwner(MouseButtonToKey(button), owner_id); // Should be same as IsKeyDown(MouseButtonToKey(button), owner_id), but this allows legacy code hijacking the io.Mousedown[] array.
}

bool ImGui::IsMouseClicked(ImGuiMouseButton button, bool repeat)
{
    return IsMouseClicked(button, repeat ? ImGuiInputFlags_Repeat : ImGuiInputFlags_None, ImGuiKeyOwner_Any);
}

bool ImGui::IsMouseClicked(ImGuiMouseButton button, ImGuiInputFlags flags, ImGuiID owner_id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (!g.IO.MouseDown[button]) // In theory this should already be encoded as (DownDuration < 0.0f), but testing this facilitates eating mechanism (until we finish work on key ownership)
        return false;
    const float t = g.IO.MouseDownDuration[button];
    if (t < 0.0f)
        return false;
    IM_ASSERT((flags & ~ImGuiInputFlags_SupportedByIsMouseClicked) == 0); // Passing flags not supported by this function! // FIXME: Could support RepeatRate and RepeatUntil flags here.

    const bool repeat = (flags & ImGuiInputFlags_Repeat) != 0;
    const bool pressed = (t == 0.0f) || (repeat && t > g.IO.KeyRepeatDelay && CalcTypematicRepeatAmount(t - g.IO.DeltaTime, t, g.IO.KeyRepeatDelay, g.IO.KeyRepeatRate) > 0);
    if (!pressed)
        return false;

    if (!TestKeyOwner(MouseButtonToKey(button), owner_id))
        return false;

    return true;
}

bool ImGui::IsMouseReleased(ImGuiMouseButton button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseReleased[button] && TestKeyOwner(MouseButtonToKey(button), ImGuiKeyOwner_Any); // Should be same as IsKeyReleased(MouseButtonToKey(button), ImGuiKeyOwner_Any)
}

bool ImGui::IsMouseReleased(ImGuiMouseButton button, ImGuiID owner_id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseReleased[button] && TestKeyOwner(MouseButtonToKey(button), owner_id); // Should be same as IsKeyReleased(MouseButtonToKey(button), owner_id)
}

bool ImGui::IsMouseDoubleClicked(ImGuiMouseButton button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseClickedCount[button] == 2 && TestKeyOwner(MouseButtonToKey(button), ImGuiKeyOwner_Any);
}

bool ImGui::IsMouseDoubleClicked(ImGuiMouseButton button, ImGuiID owner_id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseClickedCount[button] == 2 && TestKeyOwner(MouseButtonToKey(button), owner_id);
}

int ImGui::GetMouseClickedCount(ImGuiMouseButton button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseClickedCount[button];
}

// Test if mouse cursor is hovering given rectangle
// NB- Rectangle is clipped by our current clip setting
// NB- Expand the rectangle to be generous on imprecise inputs systems (g.Style.TouchExtraPadding)
bool ImGui::IsMouseHoveringRect(const ImVec2& r_min, const ImVec2& r_max, bool clip)
{
    ImGuiContext& g = *GImGui;

    // Clip
    ImRect rect_clipped(r_min, r_max);
    if (clip)
        rect_clipped.ClipWith(g.CurrentWindow->ClipRect);

    // Hit testing, expanded for touch input
    if (!rect_clipped.ContainsWithPad(g.IO.MousePos, g.Style.TouchExtraPadding))
        return false;
    return true;
}

// Return if a mouse click/drag went past the given threshold. Valid to call during the MouseReleased frame.
// [Internal] This doesn't test if the button is pressed
bool ImGui::IsMouseDragPastThreshold(ImGuiMouseButton button, float lock_threshold)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (lock_threshold < 0.0f)
        lock_threshold = g.IO.MouseDragThreshold;
    return g.IO.MouseDragMaxDistanceSqr[button] >= lock_threshold * lock_threshold;
}

bool ImGui::IsMouseDragging(ImGuiMouseButton button, float lock_threshold)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (!g.IO.MouseDown[button])
        return false;
    return IsMouseDragPastThreshold(button, lock_threshold);
}

ImVec2 ImGui::GetMousePos()
{
    ImGuiContext& g = *GImGui;
    return g.IO.MousePos;
}

// This is called TeleportMousePos() and not SetMousePos() to emphasis that setting MousePosPrev will effectively clear mouse delta as well.
// It is expected you only call this if (io.BackendFlags & ImGuiBackendFlags_HasSetMousePos) is set and supported by backend.
void ImGui::TeleportMousePos(const ImVec2& pos)
{
    ImGuiContext& g = *GImGui;
    g.IO.MousePos = g.IO.MousePosPrev = pos;
    g.IO.MouseDelta = ImVec2(0.0f, 0.0f);
    g.IO.WantSetMousePos = true;
    //IMGUI_DEBUG_LOG_IO("TeleportMousePos: (%.1f,%.1f)\n", io.MousePos.x, io.MousePos.y);
}

// NB: prefer to call right after BeginPopup(). At the time Selectable/MenuItem is activated, the popup is already closed!
ImVec2 ImGui::GetMousePosOnOpeningCurrentPopup()
{
    ImGuiContext& g = *GImGui;
    if (g.BeginPopupStack.Size > 0)
        return g.OpenPopupStack[g.BeginPopupStack.Size - 1].OpenMousePos;
    return g.IO.MousePos;
}

// We typically use ImVec2(-FLT_MAX,-FLT_MAX) to denote an invalid mouse position.
bool ImGui::IsMousePosValid(const ImVec2* mouse_pos)
{
    // The assert is only to silence a false-positive in XCode Static Analysis.
    // Because GImGui is not dereferenced in every code path, the static analyzer assume that it may be NULL (which it doesn't for other functions).
    IM_ASSERT(GImGui != NULL);
    const float MOUSE_INVALID = -256000.0f;
    ImVec2 p = mouse_pos ? *mouse_pos : GImGui->IO.MousePos;
    return p.x >= MOUSE_INVALID && p.y >= MOUSE_INVALID;
}

// [WILL OBSOLETE] This was designed for backends, but prefer having backend maintain a mask of held mouse buttons, because upcoming input queue system will make this invalid.
bool ImGui::IsAnyMouseDown()
{
    ImGuiContext& g = *GImGui;
    for (int n = 0; n < IM_ARRAYSIZE(g.IO.MouseDown); n++)
        if (g.IO.MouseDown[n])
            return true;
    return false;
}

// Return the delta from the initial clicking position while the mouse button is clicked or was just released.
// This is locked and return 0.0f until the mouse moves past a distance threshold at least once.
// NB: This is only valid if IsMousePosValid(). backends in theory should always keep mouse position valid when dragging even outside the client window.
ImVec2 ImGui::GetMouseDragDelta(ImGuiMouseButton button, float lock_threshold)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (lock_threshold < 0.0f)
        lock_threshold = g.IO.MouseDragThreshold;
    if (g.IO.MouseDown[button] || g.IO.MouseReleased[button])
        if (g.IO.MouseDragMaxDistanceSqr[button] >= lock_threshold * lock_threshold)
            if (IsMousePosValid(&g.IO.MousePos) && IsMousePosValid(&g.IO.MouseClickedPos[button]))
                return g.IO.MousePos - g.IO.MouseClickedPos[button];
    return ImVec2(0.0f, 0.0f);
}

void ImGui::ResetMouseDragDelta(ImGuiMouseButton button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    // NB: We don't need to reset g.IO.MouseDragMaxDistanceSqr
    g.IO.MouseClickedPos[button] = g.IO.MousePos;
}

// Get desired mouse cursor shape.
// Important: this is meant to be used by a platform backend, it is reset in ImGui::NewFrame(),
// updated during the frame, and locked in EndFrame()/Render().
// If you use software rendering by setting io.MouseDrawCursor then Dear ImGui will render those for you
ImGuiMouseCursor ImGui::GetMouseCursor()
{
    ImGuiContext& g = *GImGui;
    return g.MouseCursor;
}

// We intentionally accept values of ImGuiMouseCursor that are outside our bounds, in case users needs to hack-in a custom cursor value.
// Custom cursors may be handled by custom backends. If you are using a standard backend and want to hack in a custom cursor, you may
// handle it before the backend _NewFrame() call and temporarily set ImGuiConfigFlags_NoMouseCursorChange during the backend _NewFrame() call.
void ImGui::SetMouseCursor(ImGuiMouseCursor cursor_type)
{
    ImGuiContext& g = *GImGui;
    g.MouseCursor = cursor_type;
}

static void UpdateAliasKey(ImGuiKey key, bool v, float analog_value)
{
    IM_ASSERT(ImGui::IsAliasKey(key));
    ImGuiKeyData* key_data = ImGui::GetKeyData(key);
    key_data->Down = v;
    key_data->AnalogValue = analog_value;
}

// [Internal] Do not use directly
static ImGuiKeyChord GetMergedModsFromKeys()
{
    ImGuiKeyChord mods = 0;
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl))     { mods |= ImGuiMod_Ctrl; }
    if (ImGui::IsKeyDown(ImGuiMod_Shift))    { mods |= ImGuiMod_Shift; }
    if (ImGui::IsKeyDown(ImGuiMod_Alt))      { mods |= ImGuiMod_Alt; }
    if (ImGui::IsKeyDown(ImGuiMod_Super))    { mods |= ImGuiMod_Super; }
    return mods;
}

static void ImGui::UpdateKeyboardInputs()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    if (io.ConfigFlags & ImGuiConfigFlags_NoKeyboard)
        io.ClearInputKeys();

    // Import legacy keys or verify they are not used
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    if (io.BackendUsingLegacyKeyArrays == 0)
    {
        // Backend used new io.AddKeyEvent() API: Good! Verify that old arrays are never written to externally.
        for (int n = 0; n < ImGuiKey_LegacyNativeKey_END; n++)
            IM_ASSERT((io.KeysDown[n] == false || IsKeyDown((ImGuiKey)n)) && "Backend needs to either only use io.AddKeyEvent(), either only fill legacy io.KeysDown[] + io.KeyMap[]. Not both!");
    }
    else
    {
        if (g.FrameCount == 0)
            for (int n = ImGuiKey_LegacyNativeKey_BEGIN; n < ImGuiKey_LegacyNativeKey_END; n++)
                IM_ASSERT(g.IO.KeyMap[n] == -1 && "Backend is not allowed to write to io.KeyMap[0..511]!");

        // Build reverse KeyMap (Named -> Legacy)
        for (int n = ImGuiKey_NamedKey_BEGIN; n < ImGuiKey_NamedKey_END; n++)
            if (io.KeyMap[n] != -1)
            {
                IM_ASSERT(IsLegacyKey((ImGuiKey)io.KeyMap[n]));
                io.KeyMap[io.KeyMap[n]] = n;
            }

        // Import legacy keys into new ones
        for (int n = ImGuiKey_LegacyNativeKey_BEGIN; n < ImGuiKey_LegacyNativeKey_END; n++)
            if (io.KeysDown[n] || io.BackendUsingLegacyKeyArrays == 1)
            {
                const ImGuiKey key = (ImGuiKey)(io.KeyMap[n] != -1 ? io.KeyMap[n] : n);
                IM_ASSERT(io.KeyMap[n] == -1 || IsNamedKey(key));
                io.KeysData[key].Down = io.KeysDown[n];
                if (key != n)
                    io.KeysDown[key] = io.KeysDown[n]; // Allow legacy code using io.KeysDown[GetKeyIndex()] with old backends
                io.BackendUsingLegacyKeyArrays = 1;
            }
        if (io.BackendUsingLegacyKeyArrays == 1)
        {
            GetKeyData(ImGuiMod_Ctrl)->Down = io.KeyCtrl;
            GetKeyData(ImGuiMod_Shift)->Down = io.KeyShift;
            GetKeyData(ImGuiMod_Alt)->Down = io.KeyAlt;
            GetKeyData(ImGuiMod_Super)->Down = io.KeySuper;
        }
    }
#endif

    // Import legacy ImGuiNavInput_ io inputs and convert to gamepad keys
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    const bool nav_gamepad_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
    if (io.BackendUsingLegacyNavInputArray && nav_gamepad_active)
    {
        #define MAP_LEGACY_NAV_INPUT_TO_KEY1(_KEY, _NAV1)           do { io.KeysData[_KEY].Down = (io.NavInputs[_NAV1] > 0.0f); io.KeysData[_KEY].AnalogValue = io.NavInputs[_NAV1]; } while (0)
        #define MAP_LEGACY_NAV_INPUT_TO_KEY2(_KEY, _NAV1, _NAV2)    do { io.KeysData[_KEY].Down = (io.NavInputs[_NAV1] > 0.0f) || (io.NavInputs[_NAV2] > 0.0f); io.KeysData[_KEY].AnalogValue = ImMax(io.NavInputs[_NAV1], io.NavInputs[_NAV2]); } while (0)
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadFaceDown, ImGuiNavInput_Activate);
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadFaceRight, ImGuiNavInput_Cancel);
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadFaceLeft, ImGuiNavInput_Menu);
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadFaceUp, ImGuiNavInput_Input);
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadDpadLeft, ImGuiNavInput_DpadLeft);
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadDpadRight, ImGuiNavInput_DpadRight);
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadDpadUp, ImGuiNavInput_DpadUp);
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadDpadDown, ImGuiNavInput_DpadDown);
        MAP_LEGACY_NAV_INPUT_TO_KEY2(ImGuiKey_GamepadL1, ImGuiNavInput_FocusPrev, ImGuiNavInput_TweakSlow);
        MAP_LEGACY_NAV_INPUT_TO_KEY2(ImGuiKey_GamepadR1, ImGuiNavInput_FocusNext, ImGuiNavInput_TweakFast);
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadLStickLeft, ImGuiNavInput_LStickLeft);
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadLStickRight, ImGuiNavInput_LStickRight);
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadLStickUp, ImGuiNavInput_LStickUp);
        MAP_LEGACY_NAV_INPUT_TO_KEY1(ImGuiKey_GamepadLStickDown, ImGuiNavInput_LStickDown);
        #undef NAV_MAP_KEY
    }
#endif

    // Update aliases
    for (int n = 0; n < ImGuiMouseButton_COUNT; n++)
        UpdateAliasKey(MouseButtonToKey(n), io.MouseDown[n], io.MouseDown[n] ? 1.0f : 0.0f);
    UpdateAliasKey(ImGuiKey_MouseWheelX, io.MouseWheelH != 0.0f, io.MouseWheelH);
    UpdateAliasKey(ImGuiKey_MouseWheelY, io.MouseWheel != 0.0f, io.MouseWheel);

    // Synchronize io.KeyMods and io.KeyCtrl/io.KeyShift/etc. values.
    // - New backends (1.87+): send io.AddKeyEvent(ImGuiMod_XXX) ->                                      -> (here) deriving io.KeyMods + io.KeyXXX from key array.
    // - Legacy backends:      set io.KeyXXX bools               -> (above) set key array from io.KeyXXX -> (here) deriving io.KeyMods + io.KeyXXX from key array.
    // So with legacy backends the 4 values will do a unnecessary back-and-forth but it makes the code simpler and future facing.
    const ImGuiKeyChord prev_key_mods = io.KeyMods;
    io.KeyMods = GetMergedModsFromKeys();
    io.KeyCtrl = (io.KeyMods & ImGuiMod_Ctrl) != 0;
    io.KeyShift = (io.KeyMods & ImGuiMod_Shift) != 0;
    io.KeyAlt = (io.KeyMods & ImGuiMod_Alt) != 0;
    io.KeySuper = (io.KeyMods & ImGuiMod_Super) != 0;
    if (prev_key_mods != io.KeyMods)
        g.LastKeyModsChangeTime = g.Time;
    if (prev_key_mods != io.KeyMods && prev_key_mods == 0)
        g.LastKeyModsChangeFromNoneTime = g.Time;

    // Clear gamepad data if disabled
    if ((io.BackendFlags & ImGuiBackendFlags_HasGamepad) == 0)
        for (int i = ImGuiKey_Gamepad_BEGIN; i < ImGuiKey_Gamepad_END; i++)
        {
            io.KeysData[i - ImGuiKey_KeysData_OFFSET].Down = false;
            io.KeysData[i - ImGuiKey_KeysData_OFFSET].AnalogValue = 0.0f;
        }

    // Update keys
    for (int i = 0; i < ImGuiKey_KeysData_SIZE; i++)
    {
        ImGuiKeyData* key_data = &io.KeysData[i];
        key_data->DownDurationPrev = key_data->DownDuration;
        key_data->DownDuration = key_data->Down ? (key_data->DownDuration < 0.0f ? 0.0f : key_data->DownDuration + io.DeltaTime) : -1.0f;
        if (key_data->DownDuration == 0.0f)
        {
            ImGuiKey key = (ImGuiKey)(ImGuiKey_KeysData_OFFSET + i);
            if (IsKeyboardKey(key))
                g.LastKeyboardKeyPressTime = g.Time;
            else if (key == ImGuiKey_ReservedForModCtrl || key == ImGuiKey_ReservedForModShift || key == ImGuiKey_ReservedForModAlt || key == ImGuiKey_ReservedForModSuper)
                g.LastKeyboardKeyPressTime = g.Time;
        }
    }

    // Update keys/input owner (named keys only): one entry per key
    for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1))
    {
        ImGuiKeyData* key_data = &io.KeysData[key - ImGuiKey_KeysData_OFFSET];
        ImGuiKeyOwnerData* owner_data = &g.KeysOwnerData[key - ImGuiKey_NamedKey_BEGIN];
        owner_data->OwnerCurr = owner_data->OwnerNext;
        if (!key_data->Down) // Important: ownership is released on the frame after a release. Ensure a 'MouseDown -> CloseWindow -> MouseUp' chain doesn't lead to someone else seeing the MouseUp.
            owner_data->OwnerNext = ImGuiKeyOwner_NoOwner;
        owner_data->LockThisFrame = owner_data->LockUntilRelease = owner_data->LockUntilRelease && key_data->Down;  // Clear LockUntilRelease when key is not Down anymore
    }

    // Update key routing (for e.g. shortcuts)
    UpdateKeyRoutingTable(&g.KeysRoutingTable);
}

static void ImGui::UpdateMouseInputs()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    // Mouse Wheel swapping flag
    // As a standard behavior holding SHIFT while using Vertical Mouse Wheel triggers Horizontal scroll instead
    // - We avoid doing it on OSX as it the OS input layer handles this already.
    // - FIXME: However this means when running on OSX over Emscripten, Shift+WheelY will incur two swapping (1 in OS, 1 here), canceling the feature.
    // - FIXME: When we can distinguish e.g. touchpad scroll events from mouse ones, we'll set this accordingly based on input source.
    io.MouseWheelRequestAxisSwap = io.KeyShift && !io.ConfigMacOSXBehaviors;

    // Round mouse position to avoid spreading non-rounded position (e.g. UpdateManualResize doesn't support them well)
    if (IsMousePosValid(&io.MousePos))
        io.MousePos = g.MouseLastValidPos = ImFloor(io.MousePos);

    // If mouse just appeared or disappeared (usually denoted by -FLT_MAX components) we cancel out movement in MouseDelta
    if (IsMousePosValid(&io.MousePos) && IsMousePosValid(&io.MousePosPrev))
        io.MouseDelta = io.MousePos - io.MousePosPrev;
    else
        io.MouseDelta = ImVec2(0.0f, 0.0f);

    // Update stationary timer.
    // FIXME: May need to rework again to have some tolerance for occasional small movement, while being functional on high-framerates.
    const float mouse_stationary_threshold = (io.MouseSource == ImGuiMouseSource_Mouse) ? 2.0f : 3.0f; // Slightly higher threshold for ImGuiMouseSource_TouchScreen/ImGuiMouseSource_Pen, may need rework.
    const bool mouse_stationary = (ImLengthSqr(io.MouseDelta) <= mouse_stationary_threshold * mouse_stationary_threshold);
    g.MouseStationaryTimer = mouse_stationary ? (g.MouseStationaryTimer + io.DeltaTime) : 0.0f;
    //IMGUI_DEBUG_LOG("%.4f\n", g.MouseStationaryTimer);

    // If mouse moved we re-enable mouse hovering in case it was disabled by gamepad/keyboard. In theory should use a >0.0f threshold but would need to reset in everywhere we set this to true.
    if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)
        g.NavDisableMouseHover = false;

    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        io.MouseClicked[i] = io.MouseDown[i] && io.MouseDownDuration[i] < 0.0f;
        io.MouseClickedCount[i] = 0; // Will be filled below
        io.MouseReleased[i] = !io.MouseDown[i] && io.MouseDownDuration[i] >= 0.0f;
        io.MouseDownDurationPrev[i] = io.MouseDownDuration[i];
        io.MouseDownDuration[i] = io.MouseDown[i] ? (io.MouseDownDuration[i] < 0.0f ? 0.0f : io.MouseDownDuration[i] + io.DeltaTime) : -1.0f;
        if (io.MouseClicked[i])
        {
            bool is_repeated_click = false;
            if ((float)(g.Time - io.MouseClickedTime[i]) < io.MouseDoubleClickTime)
            {
                ImVec2 delta_from_click_pos = IsMousePosValid(&io.MousePos) ? (io.MousePos - io.MouseClickedPos[i]) : ImVec2(0.0f, 0.0f);
                if (ImLengthSqr(delta_from_click_pos) < io.MouseDoubleClickMaxDist * io.MouseDoubleClickMaxDist)
                    is_repeated_click = true;
            }
            if (is_repeated_click)
                io.MouseClickedLastCount[i]++;
            else
                io.MouseClickedLastCount[i] = 1;
            io.MouseClickedTime[i] = g.Time;
            io.MouseClickedPos[i] = io.MousePos;
            io.MouseClickedCount[i] = io.MouseClickedLastCount[i];
            io.MouseDragMaxDistanceSqr[i] = 0.0f;
        }
        else if (io.MouseDown[i])
        {
            // Maintain the maximum distance we reaching from the initial click position, which is used with dragging threshold
            float delta_sqr_click_pos = IsMousePosValid(&io.MousePos) ? ImLengthSqr(io.MousePos - io.MouseClickedPos[i]) : 0.0f;
            io.MouseDragMaxDistanceSqr[i] = ImMax(io.MouseDragMaxDistanceSqr[i], delta_sqr_click_pos);
        }

        // We provide io.MouseDoubleClicked[] as a legacy service
        io.MouseDoubleClicked[i] = (io.MouseClickedCount[i] == 2);

        // Clicking any mouse button reactivate mouse hovering which may have been deactivated by gamepad/keyboard navigation
        if (io.MouseClicked[i])
            g.NavDisableMouseHover = false;
    }
}

static void LockWheelingWindow(ImGuiWindow* window, float wheel_amount)
{
    ImGuiContext& g = *GImGui;
    if (window)
        g.WheelingWindowReleaseTimer = ImMin(g.WheelingWindowReleaseTimer + ImAbs(wheel_amount) * WINDOWS_MOUSE_WHEEL_SCROLL_LOCK_TIMER, WINDOWS_MOUSE_WHEEL_SCROLL_LOCK_TIMER);
    else
        g.WheelingWindowReleaseTimer = 0.0f;
    if (g.WheelingWindow == window)
        return;
    IMGUI_DEBUG_LOG_IO("[io] LockWheelingWindow() \"%s\"\n", window ? window->Name : "NULL");
    g.WheelingWindow = window;
    g.WheelingWindowRefMousePos = g.IO.MousePos;
    if (window == NULL)
    {
        g.WheelingWindowStartFrame = -1;
        g.WheelingAxisAvg = ImVec2(0.0f, 0.0f);
    }
}

static ImGuiWindow* FindBestWheelingWindow(const ImVec2& wheel)
{
    // For each axis, find window in the hierarchy that may want to use scrolling
    ImGuiContext& g = *GImGui;
    ImGuiWindow* windows[2] = { NULL, NULL };
    for (int axis = 0; axis < 2; axis++)
        if (wheel[axis] != 0.0f)
            for (ImGuiWindow* window = windows[axis] = g.HoveredWindow; window->Flags & ImGuiWindowFlags_ChildWindow; window = windows[axis] = window->ParentWindow)
            {
                // Bubble up into parent window if:
                // - a child window doesn't allow any scrolling.
                // - a child window has the ImGuiWindowFlags_NoScrollWithMouse flag.
                //// - a child window doesn't need scrolling because it is already at the edge for the direction we are going in (FIXME-WIP)
                const bool has_scrolling = (window->ScrollMax[axis] != 0.0f);
                const bool inputs_disabled = (window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs);
                //const bool scrolling_past_limits = (wheel_v < 0.0f) ? (window->Scroll[axis] <= 0.0f) : (window->Scroll[axis] >= window->ScrollMax[axis]);
                if (has_scrolling && !inputs_disabled) // && !scrolling_past_limits)
                    break; // select this window
            }
    if (windows[0] == NULL && windows[1] == NULL)
        return NULL;

    // If there's only one window or only one axis then there's no ambiguity
    if (windows[0] == windows[1] || windows[0] == NULL || windows[1] == NULL)
        return windows[1] ? windows[1] : windows[0];

    // If candidate are different windows we need to decide which one to prioritize
    // - First frame: only find a winner if one axis is zero.
    // - Subsequent frames: only find a winner when one is more than the other.
    if (g.WheelingWindowStartFrame == -1)
        g.WheelingWindowStartFrame = g.FrameCount;
    if ((g.WheelingWindowStartFrame == g.FrameCount && wheel.x != 0.0f && wheel.y != 0.0f) || (g.WheelingAxisAvg.x == g.WheelingAxisAvg.y))
    {
        g.WheelingWindowWheelRemainder = wheel;
        return NULL;
    }
    return (g.WheelingAxisAvg.x > g.WheelingAxisAvg.y) ? windows[0] : windows[1];
}

// Called by NewFrame()
void ImGui::UpdateMouseWheel()
{
    // Reset the locked window if we move the mouse or after the timer elapses.
    // FIXME: Ideally we could refactor to have one timer for "changing window w/ same axis" and a shorter timer for "changing window or axis w/ other axis" (#3795)
    ImGuiContext& g = *GImGui;
    if (g.WheelingWindow != NULL)
    {
        g.WheelingWindowReleaseTimer -= g.IO.DeltaTime;
        if (IsMousePosValid() && ImLengthSqr(g.IO.MousePos - g.WheelingWindowRefMousePos) > g.IO.MouseDragThreshold * g.IO.MouseDragThreshold)
            g.WheelingWindowReleaseTimer = 0.0f;
        if (g.WheelingWindowReleaseTimer <= 0.0f)
            LockWheelingWindow(NULL, 0.0f);
    }

    ImVec2 wheel;
    wheel.x = TestKeyOwner(ImGuiKey_MouseWheelX, ImGuiKeyOwner_NoOwner) ? g.IO.MouseWheelH : 0.0f;
    wheel.y = TestKeyOwner(ImGuiKey_MouseWheelY, ImGuiKeyOwner_NoOwner) ? g.IO.MouseWheel : 0.0f;

    //IMGUI_DEBUG_LOG("MouseWheel X:%.3f Y:%.3f\n", wheel_x, wheel_y);
    ImGuiWindow* mouse_window = g.WheelingWindow ? g.WheelingWindow : g.HoveredWindow;
    if (!mouse_window || mouse_window->Collapsed)
        return;

    // Zoom / Scale window
    // FIXME-OBSOLETE: This is an old feature, it still works but pretty much nobody is using it and may be best redesigned.
    if (wheel.y != 0.0f && g.IO.KeyCtrl && g.IO.FontAllowUserScaling)
    {
        LockWheelingWindow(mouse_window, wheel.y);
        ImGuiWindow* window = mouse_window;
        const float new_font_scale = ImClamp(window->FontWindowScale + g.IO.MouseWheel * 0.10f, 0.50f, 2.50f);
        const float scale = new_font_scale / window->FontWindowScale;
        window->FontWindowScale = new_font_scale;
        if (window == window->RootWindow)
        {
            const ImVec2 offset = window->Size * (1.0f - scale) * (g.IO.MousePos - window->Pos) / window->Size;
            SetWindowPos(window, window->Pos + offset, 0);
            window->Size = ImTrunc(window->Size * scale);
            window->SizeFull = ImTrunc(window->SizeFull * scale);
        }
        return;
    }
    if (g.IO.KeyCtrl)
        return;

    // Mouse wheel scrolling
    // Read about io.MouseWheelRequestAxisSwap and its issue on Mac+Emscripten in UpdateMouseInputs()
    if (g.IO.MouseWheelRequestAxisSwap)
        wheel = ImVec2(wheel.y, 0.0f);

    // Maintain a rough average of moving magnitude on both axises
    // FIXME: should by based on wall clock time rather than frame-counter
    g.WheelingAxisAvg.x = ImExponentialMovingAverage(g.WheelingAxisAvg.x, ImAbs(wheel.x), 30);
    g.WheelingAxisAvg.y = ImExponentialMovingAverage(g.WheelingAxisAvg.y, ImAbs(wheel.y), 30);

    // In the rare situation where FindBestWheelingWindow() had to defer first frame of wheeling due to ambiguous main axis, reinject it now.
    wheel += g.WheelingWindowWheelRemainder;
    g.WheelingWindowWheelRemainder = ImVec2(0.0f, 0.0f);
    if (wheel.x == 0.0f && wheel.y == 0.0f)
        return;

    // Mouse wheel scrolling: find target and apply
    // - don't renew lock if axis doesn't apply on the window.
    // - select a main axis when both axises are being moved.
    if (ImGuiWindow* window = (g.WheelingWindow ? g.WheelingWindow : FindBestWheelingWindow(wheel)))
        if (!(window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))
        {
            bool do_scroll[2] = { wheel.x != 0.0f && window->ScrollMax.x != 0.0f, wheel.y != 0.0f && window->ScrollMax.y != 0.0f };
            if (do_scroll[ImGuiAxis_X] && do_scroll[ImGuiAxis_Y])
                do_scroll[(g.WheelingAxisAvg.x > g.WheelingAxisAvg.y) ? ImGuiAxis_Y : ImGuiAxis_X] = false;
            if (do_scroll[ImGuiAxis_X])
            {
                LockWheelingWindow(window, wheel.x);
                float max_step = window->InnerRect.GetWidth() * 0.67f;
                float scroll_step = ImTrunc(ImMin(2 * window->CalcFontSize(), max_step));
                SetScrollX(window, window->Scroll.x - wheel.x * scroll_step);
                g.WheelingWindowScrolledFrame = g.FrameCount;
            }
            if (do_scroll[ImGuiAxis_Y])
            {
                LockWheelingWindow(window, wheel.y);
                float max_step = window->InnerRect.GetHeight() * 0.67f;
                float scroll_step = ImTrunc(ImMin(5 * window->CalcFontSize(), max_step));
                SetScrollY(window, window->Scroll.y - wheel.y * scroll_step);
                g.WheelingWindowScrolledFrame = g.FrameCount;
            }
        }
}

void ImGui::SetNextFrameWantCaptureKeyboard(bool want_capture_keyboard)
{
    ImGuiContext& g = *GImGui;
    g.WantCaptureKeyboardNextFrame = want_capture_keyboard ? 1 : 0;
}

void ImGui::SetNextFrameWantCaptureMouse(bool want_capture_mouse)
{
    ImGuiContext& g = *GImGui;
    g.WantCaptureMouseNextFrame = want_capture_mouse ? 1 : 0;
}

#ifndef IMGUI_DISABLE_DEBUG_TOOLS
static const char* GetInputSourceName(ImGuiInputSource source)
{
    const char* input_source_names[] = { "None", "Mouse", "Keyboard", "Gamepad" };
    IM_ASSERT(IM_ARRAYSIZE(input_source_names) == ImGuiInputSource_COUNT && source >= 0 && source < ImGuiInputSource_COUNT);
    return input_source_names[source];
}
static const char* GetMouseSourceName(ImGuiMouseSource source)
{
    const char* mouse_source_names[] = { "Mouse", "TouchScreen", "Pen" };
    IM_ASSERT(IM_ARRAYSIZE(mouse_source_names) == ImGuiMouseSource_COUNT && source >= 0 && source < ImGuiMouseSource_COUNT);
    return mouse_source_names[source];
}
static void DebugPrintInputEvent(const char* prefix, const ImGuiInputEvent* e)
{
    ImGuiContext& g = *GImGui;
    if (e->Type == ImGuiInputEventType_MousePos)    { if (e->MousePos.PosX == -FLT_MAX && e->MousePos.PosY == -FLT_MAX) IMGUI_DEBUG_LOG_IO("[io] %s: MousePos (-FLT_MAX, -FLT_MAX)\n", prefix); else IMGUI_DEBUG_LOG_IO("[io] %s: MousePos (%.1f, %.1f) (%s)\n", prefix, e->MousePos.PosX, e->MousePos.PosY, GetMouseSourceName(e->MousePos.MouseSource)); return; }
    if (e->Type == ImGuiInputEventType_MouseButton) { IMGUI_DEBUG_LOG_IO("[io] %s: MouseButton %d %s (%s)\n", prefix, e->MouseButton.Button, e->MouseButton.Down ? "Down" : "Up", GetMouseSourceName(e->MouseButton.MouseSource)); return; }
    if (e->Type == ImGuiInputEventType_MouseWheel)  { IMGUI_DEBUG_LOG_IO("[io] %s: MouseWheel (%.3f, %.3f) (%s)\n", prefix, e->MouseWheel.WheelX, e->MouseWheel.WheelY, GetMouseSourceName(e->MouseWheel.MouseSource)); return; }
    if (e->Type == ImGuiInputEventType_Key)         { IMGUI_DEBUG_LOG_IO("[io] %s: Key \"%s\" %s\n", prefix, ImGui::GetKeyName(e->Key.Key), e->Key.Down ? "Down" : "Up"); return; }
    if (e->Type == ImGuiInputEventType_Text)        { IMGUI_DEBUG_LOG_IO("[io] %s: Text: %c (U+%08X)\n", prefix, e->Text.Char, e->Text.Char); return; }
    if (e->Type == ImGuiInputEventType_Focus)       { IMGUI_DEBUG_LOG_IO("[io] %s: AppFocused %d\n", prefix, e->AppFocused.Focused); return; }
}
#endif

// Process input queue
// We always call this with the value of 'bool g.IO.ConfigInputTrickleEventQueue'.
// - trickle_fast_inputs = false : process all events, turn into flattened input state (e.g. successive down/up/down/up will be lost)
// - trickle_fast_inputs = true  : process as many events as possible (successive down/up/down/up will be trickled over several frames so nothing is lost) (new feature in 1.87)
void ImGui::UpdateInputEvents(bool trickle_fast_inputs)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    // Only trickle chars<>key when working with InputText()
    // FIXME: InputText() could parse event trail?
    // FIXME: Could specialize chars<>keys trickling rules for control keys (those not typically associated to characters)
    const bool trickle_interleaved_nonchar_keys_and_text = (trickle_fast_inputs && g.WantTextInputNextFrame == 1);

    bool mouse_moved = false, mouse_wheeled = false, key_changed = false, key_changed_nonchar = false, text_inputted = false;
    int  mouse_button_changed = 0x00;
    ImBitArray<ImGuiKey_KeysData_SIZE> key_changed_mask;

    int event_n = 0;
    for (; event_n < g.InputEventsQueue.Size; event_n++)
    {
        ImGuiInputEvent* e = &g.InputEventsQueue[event_n];
        if (e->Type == ImGuiInputEventType_MousePos)
        {
            if (g.IO.WantSetMousePos)
                continue;
            // Trickling Rule: Stop processing queued events if we already handled a mouse button change
            ImVec2 event_pos(e->MousePos.PosX, e->MousePos.PosY);
            if (trickle_fast_inputs && (mouse_button_changed != 0 || mouse_wheeled || key_changed || text_inputted))
                break;
            io.MousePos = event_pos;
            io.MouseSource = e->MousePos.MouseSource;
            mouse_moved = true;
        }
        else if (e->Type == ImGuiInputEventType_MouseButton)
        {
            // Trickling Rule: Stop processing queued events if we got multiple action on the same button
            const ImGuiMouseButton button = e->MouseButton.Button;
            IM_ASSERT(button >= 0 && button < ImGuiMouseButton_COUNT);
            if (trickle_fast_inputs && ((mouse_button_changed & (1 << button)) || mouse_wheeled))
                break;
            if (trickle_fast_inputs && e->MouseButton.MouseSource == ImGuiMouseSource_TouchScreen && mouse_moved) // #2702: TouchScreen have no initial hover.
                break;
            io.MouseDown[button] = e->MouseButton.Down;
            io.MouseSource = e->MouseButton.MouseSource;
            mouse_button_changed |= (1 << button);
        }
        else if (e->Type == ImGuiInputEventType_MouseWheel)
        {
            // Trickling Rule: Stop processing queued events if we got multiple action on the event
            if (trickle_fast_inputs && (mouse_moved || mouse_button_changed != 0))
                break;
            io.MouseWheelH += e->MouseWheel.WheelX;
            io.MouseWheel += e->MouseWheel.WheelY;
            io.MouseSource = e->MouseWheel.MouseSource;
            mouse_wheeled = true;
        }
        else if (e->Type == ImGuiInputEventType_Key)
        {
            // Trickling Rule: Stop processing queued events if we got multiple action on the same button
            if (io.ConfigFlags & ImGuiConfigFlags_NoKeyboard)
                continue;
            ImGuiKey key = e->Key.Key;
            IM_ASSERT(key != ImGuiKey_None);
            ImGuiKeyData* key_data = GetKeyData(key);
            const int key_data_index = (int)(key_data - g.IO.KeysData);
            if (trickle_fast_inputs && key_data->Down != e->Key.Down && (key_changed_mask.TestBit(key_data_index) || mouse_button_changed != 0))
                break;

            const bool key_is_potentially_for_char_input = IsKeyChordPotentiallyCharInput(GetMergedModsFromKeys() | key);
            if (trickle_interleaved_nonchar_keys_and_text && (text_inputted && !key_is_potentially_for_char_input))
                break;

            key_data->Down = e->Key.Down;
            key_data->AnalogValue = e->Key.AnalogValue;
            key_changed = true;
            key_changed_mask.SetBit(key_data_index);
            if (trickle_interleaved_nonchar_keys_and_text && !key_is_potentially_for_char_input)
                key_changed_nonchar = true;

            // Allow legacy code using io.KeysDown[GetKeyIndex()] with new backends
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
            io.KeysDown[key_data_index] = key_data->Down;
            if (io.KeyMap[key_data_index] != -1)
                io.KeysDown[io.KeyMap[key_data_index]] = key_data->Down;
#endif
        }
        else if (e->Type == ImGuiInputEventType_Text)
        {
            if (io.ConfigFlags & ImGuiConfigFlags_NoKeyboard)
                continue;
            // Trickling Rule: Stop processing queued events if keys/mouse have been interacted with
            if (trickle_fast_inputs && (mouse_button_changed != 0 || mouse_moved || mouse_wheeled))
                break;
            if (trickle_interleaved_nonchar_keys_and_text && key_changed_nonchar)
                break;
            unsigned int c = e->Text.Char;
            io.InputQueueCharacters.push_back(c <= IM_UNICODE_CODEPOINT_MAX ? (ImWchar)c : IM_UNICODE_CODEPOINT_INVALID);
            if (trickle_interleaved_nonchar_keys_and_text)
                text_inputted = true;
        }
        else if (e->Type == ImGuiInputEventType_Focus)
        {
            // We intentionally overwrite this and process in NewFrame(), in order to give a chance
            // to multi-viewports backends to queue AddFocusEvent(false) + AddFocusEvent(true) in same frame.
            const bool focus_lost = !e->AppFocused.Focused;
            io.AppFocusLost = focus_lost;
        }
        else
        {
            IM_ASSERT(0 && "Unknown event!");
        }
    }

    // Record trail (for domain-specific applications wanting to access a precise trail)
    //if (event_n != 0) IMGUI_DEBUG_LOG_IO("Processed: %d / Remaining: %d\n", event_n, g.InputEventsQueue.Size - event_n);
    for (int n = 0; n < event_n; n++)
        g.InputEventsTrail.push_back(g.InputEventsQueue[n]);

    // [DEBUG]
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    if (event_n != 0 && (g.DebugLogFlags & ImGuiDebugLogFlags_EventIO))
        for (int n = 0; n < g.InputEventsQueue.Size; n++)
            DebugPrintInputEvent(n < event_n ? "Processed" : "Remaining", &g.InputEventsQueue[n]);
#endif

    // Remaining events will be processed on the next frame
    if (event_n == g.InputEventsQueue.Size)
        g.InputEventsQueue.resize(0);
    else
        g.InputEventsQueue.erase(g.InputEventsQueue.Data, g.InputEventsQueue.Data + event_n);

    // Clear buttons state when focus is lost
    // - this is useful so e.g. releasing Alt after focus loss on Alt-Tab doesn't trigger the Alt menu toggle.
    // - we clear in EndFrame() and not now in order allow application/user code polling this flag
    //   (e.g. custom backend may want to clear additional data, custom widgets may want to react with a "canceling" event).
    if (g.IO.AppFocusLost)
    {
        g.IO.ClearInputKeys();
        g.IO.ClearInputMouse();
    }
}

ImGuiID ImGui::GetKeyOwner(ImGuiKey key)
{
    if (!IsNamedKeyOrMod(key))
        return ImGuiKeyOwner_NoOwner;

    ImGuiContext& g = *GImGui;
    ImGuiKeyOwnerData* owner_data = GetKeyOwnerData(&g, key);
    ImGuiID owner_id = owner_data->OwnerCurr;

    if (g.ActiveIdUsingAllKeyboardKeys && owner_id != g.ActiveId && owner_id != ImGuiKeyOwner_Any)
        if (key >= ImGuiKey_Keyboard_BEGIN && key < ImGuiKey_Keyboard_END)
            return ImGuiKeyOwner_NoOwner;

    return owner_id;
}

// TestKeyOwner(..., ID)   : (owner == None || owner == ID)
// TestKeyOwner(..., None) : (owner == None)
// TestKeyOwner(..., Any)  : no owner test
// All paths are also testing for key not being locked, for the rare cases that key have been locked with using ImGuiInputFlags_LockXXX flags.
bool ImGui::TestKeyOwner(ImGuiKey key, ImGuiID owner_id)
{
    if (!IsNamedKeyOrMod(key))
        return true;

    ImGuiContext& g = *GImGui;
    if (g.ActiveIdUsingAllKeyboardKeys && owner_id != g.ActiveId && owner_id != ImGuiKeyOwner_Any)
        if (key >= ImGuiKey_Keyboard_BEGIN && key < ImGuiKey_Keyboard_END)
            return false;

    ImGuiKeyOwnerData* owner_data = GetKeyOwnerData(&g, key);
    if (owner_id == ImGuiKeyOwner_Any)
        return (owner_data->LockThisFrame == false);

    // Note: SetKeyOwner() sets OwnerCurr. It is not strictly required for most mouse routing overlap (because of ActiveId/HoveredId
    // are acting as filter before this has a chance to filter), but sane as soon as user tries to look into things.
    // Setting OwnerCurr in SetKeyOwner() is more consistent than testing OwnerNext here: would be inconsistent with getter and other functions.
    if (owner_data->OwnerCurr != owner_id)
    {
        if (owner_data->LockThisFrame)
            return false;
        if (owner_data->OwnerCurr != ImGuiKeyOwner_NoOwner)
            return false;
    }

    return true;
}

// _LockXXX flags are useful to lock keys away from code which is not input-owner aware.
// When using _LockXXX flags, you can use ImGuiKeyOwner_Any to lock keys from everyone.
// - SetKeyOwner(..., None)              : clears owner
// - SetKeyOwner(..., Any, !Lock)        : illegal (assert)
// - SetKeyOwner(..., Any or None, Lock) : set lock
void ImGui::SetKeyOwner(ImGuiKey key, ImGuiID owner_id, ImGuiInputFlags flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(IsNamedKeyOrMod(key) && (owner_id != ImGuiKeyOwner_Any || (flags & (ImGuiInputFlags_LockThisFrame | ImGuiInputFlags_LockUntilRelease)))); // Can only use _Any with _LockXXX flags (to eat a key away without an ID to retrieve it)
    IM_ASSERT((flags & ~ImGuiInputFlags_SupportedBySetKeyOwner) == 0); // Passing flags not supported by this function!
    //IMGUI_DEBUG_LOG("SetKeyOwner(%s, owner_id=0x%08X, flags=%08X)\n", GetKeyName(key), owner_id, flags);

    ImGuiKeyOwnerData* owner_data = GetKeyOwnerData(&g, key);
    owner_data->OwnerCurr = owner_data->OwnerNext = owner_id;

    // We cannot lock by default as it would likely break lots of legacy code.
    // In the case of using LockUntilRelease while key is not down we still lock during the frame (no key_data->Down test)
    owner_data->LockUntilRelease = (flags & ImGuiInputFlags_LockUntilRelease) != 0;
    owner_data->LockThisFrame = (flags & ImGuiInputFlags_LockThisFrame) != 0 || (owner_data->LockUntilRelease);
}

// Rarely used helper
void ImGui::SetKeyOwnersForKeyChord(ImGuiKeyChord key_chord, ImGuiID owner_id, ImGuiInputFlags flags)
{
    if (key_chord & ImGuiMod_Ctrl)      { SetKeyOwner(ImGuiMod_Ctrl, owner_id, flags); }
    if (key_chord & ImGuiMod_Shift)     { SetKeyOwner(ImGuiMod_Shift, owner_id, flags); }
    if (key_chord & ImGuiMod_Alt)       { SetKeyOwner(ImGuiMod_Alt, owner_id, flags); }
    if (key_chord & ImGuiMod_Super)     { SetKeyOwner(ImGuiMod_Super, owner_id, flags); }
    if (key_chord & ~ImGuiMod_Mask_)    { SetKeyOwner((ImGuiKey)(key_chord & ~ImGuiMod_Mask_), owner_id, flags); }
}

// This is more or less equivalent to:
//   if (IsItemHovered() || IsItemActive())
//       SetKeyOwner(key, GetItemID());
// Extensive uses of that (e.g. many calls for a single item) may want to manually perform the tests once and then call SetKeyOwner() multiple times.
// More advanced usage scenarios may want to call SetKeyOwner() manually based on different condition.
// Worth noting is that only one item can be hovered and only one item can be active, therefore this usage pattern doesn't need to bother with routing and priority.
void ImGui::SetItemKeyOwner(ImGuiKey key, ImGuiInputFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiID id = g.LastItemData.ID;
    if (id == 0 || (g.HoveredId != id && g.ActiveId != id))
        return;
    if ((flags & ImGuiInputFlags_CondMask_) == 0)
        flags |= ImGuiInputFlags_CondDefault_;
    if ((g.HoveredId == id && (flags & ImGuiInputFlags_CondHovered)) || (g.ActiveId == id && (flags & ImGuiInputFlags_CondActive)))
    {
        IM_ASSERT((flags & ~ImGuiInputFlags_SupportedBySetItemKeyOwner) == 0); // Passing flags not supported by this function!
        SetKeyOwner(key, id, flags & ~ImGuiInputFlags_CondMask_);
    }
}

void ImGui::SetItemKeyOwner(ImGuiKey key)
{
    SetItemKeyOwner(key, ImGuiInputFlags_None);
}

// This is the only public API until we expose owner_id versions of the API as replacements.
bool ImGui::IsKeyChordPressed(ImGuiKeyChord key_chord)
{
    return IsKeyChordPressed(key_chord, ImGuiInputFlags_None, ImGuiKeyOwner_Any);
}

// This is equivalent to comparing KeyMods + doing a IsKeyPressed()
bool ImGui::IsKeyChordPressed(ImGuiKeyChord key_chord, ImGuiInputFlags flags, ImGuiID owner_id)
{
    ImGuiContext& g = *GImGui;
    key_chord = FixupKeyChord(key_chord);
    ImGuiKey mods = (ImGuiKey)(key_chord & ImGuiMod_Mask_);
    if (g.IO.KeyMods != mods)
        return false;

    // Special storage location for mods
    ImGuiKey key = (ImGuiKey)(key_chord & ~ImGuiMod_Mask_);
    if (key == ImGuiKey_None)
        key = ConvertSingleModFlagToKey(mods);
    if (!IsKeyPressed(key, (flags & ImGuiInputFlags_RepeatMask_), owner_id))
        return false;
    return true;
}

void ImGui::SetNextItemShortcut(ImGuiKeyChord key_chord, ImGuiInputFlags flags)
{
    ImGuiContext& g = *GImGui;
    g.NextItemData.Flags |= ImGuiNextItemDataFlags_HasShortcut;
    g.NextItemData.Shortcut = key_chord;
    g.NextItemData.ShortcutFlags = flags;
}

// Called from within ItemAdd: at this point we can read from NextItemData and write to LastItemData
void ImGui::ItemHandleShortcut(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    ImGuiInputFlags flags = g.NextItemData.ShortcutFlags;
    IM_ASSERT((flags & ~ImGuiInputFlags_SupportedBySetNextItemShortcut) == 0); // Passing flags not supported by SetNextItemShortcut()!

    if (g.LastItemData.InFlags & ImGuiItemFlags_Disabled)
        return;
    if (flags & ImGuiInputFlags_Tooltip)
    {
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasShortcut;
        g.LastItemData.Shortcut = g.NextItemData.Shortcut;
    }
    if (!Shortcut(g.NextItemData.Shortcut, flags & ImGuiInputFlags_SupportedByShortcut, id) || g.NavActivateId != 0)
        return;

    // FIXME: Generalize Activation queue?
    g.NavActivateId = id; // Will effectively disable clipping.
    g.NavActivateFlags = ImGuiActivateFlags_PreferInput | ImGuiActivateFlags_FromShortcut;
    //if (g.ActiveId == 0 || g.ActiveId == id)
    g.NavActivateDownId = g.NavActivatePressedId = id;
    NavHighlightActivated(id);
}

bool ImGui::Shortcut(ImGuiKeyChord key_chord, ImGuiInputFlags flags)
{
    return Shortcut(key_chord, flags, ImGuiKeyOwner_Any);
}

bool ImGui::Shortcut(ImGuiKeyChord key_chord, ImGuiInputFlags flags, ImGuiID owner_id)
{
    ImGuiContext& g = *GImGui;
    //IMGUI_DEBUG_LOG("Shortcut(%s, flags=%X, owner_id=0x%08X)\n", GetKeyChordName(key_chord, g.TempBuffer.Data, g.TempBuffer.Size), flags, owner_id);

    // When using (owner_id == 0/Any): SetShortcutRouting() will use CurrentFocusScopeId and filter with this, so IsKeyPressed() is fine with he 0/Any.
    if ((flags & ImGuiInputFlags_RouteTypeMask_) == 0)
        flags |= ImGuiInputFlags_RouteFocused;

    // Using 'owner_id == ImGuiKeyOwner_Any/0': auto-assign an owner based on current focus scope (each window has its focus scope by default)
    // Effectively makes Shortcut() always input-owner aware.
    if (owner_id == ImGuiKeyOwner_Any || owner_id == ImGuiKeyOwner_NoOwner)
        owner_id = GetRoutingIdFromOwnerId(owner_id);

    if (g.CurrentItemFlags & ImGuiItemFlags_Disabled)
        return false;

    // Submit route
    if (!SetShortcutRouting(key_chord, flags, owner_id))
        return false;

    // Default repeat behavior for Shortcut()
    // So e.g. pressing Ctrl+W and releasing Ctrl while holding W will not trigger the W shortcut.
    if ((flags & ImGuiInputFlags_Repeat) != 0 && (flags & ImGuiInputFlags_RepeatUntilMask_) == 0)
        flags |= ImGuiInputFlags_RepeatUntilKeyModsChange;

    if (!IsKeyChordPressed(key_chord, flags, owner_id))
        return false;

    // Claim mods during the press
    SetKeyOwnersForKeyChord(key_chord & ImGuiMod_Mask_, owner_id);

    IM_ASSERT((flags & ~ImGuiInputFlags_SupportedByShortcut) == 0); // Passing flags not supported by this function!
    return true;
}


//-----------------------------------------------------------------------------
// [SECTION] ERROR CHECKING, STATE RECOVERY
//-----------------------------------------------------------------------------
// - DebugCheckVersionAndDataLayout() (called via IMGUI_CHECKVERSION() macros)
// - ErrorCheckUsingSetCursorPosToExtendParentBoundaries()
// - ErrorCheckNewFrameSanityChecks()
// - ErrorCheckEndFrameSanityChecks()
// - ErrorRecoveryStoreState()
// - ErrorRecoveryTryToRecoverState()
// - ErrorRecoveryTryToRecoverWindowState()
// - ErrorLog()
//-----------------------------------------------------------------------------

// Verify ABI compatibility between caller code and compiled version of Dear ImGui. This helps detects some build issues.
// Called by IMGUI_CHECKVERSION().
// Verify that the type sizes are matching between the calling file's compilation unit and imgui.cpp's compilation unit
// If this triggers you have mismatched headers and compiled code versions.
// - It could be because of a build issue (using new headers with old compiled code)
// - It could be because of mismatched configuration #define, compilation settings, packing pragma etc.
//   THE CONFIGURATION SETTINGS MENTIONED IN imconfig.h MUST BE SET FOR ALL COMPILATION UNITS INVOLVED WITH DEAR IMGUI.
//   Which is why it is required you put them in your imconfig file (and NOT only before including imgui.h).
//   Otherwise it is possible that different compilation units would see different structure layout.
//   If you don't want to modify imconfig.h you can use the IMGUI_USER_CONFIG define to change filename.
bool ImGui::DebugCheckVersionAndDataLayout(const char* version, size_t sz_io, size_t sz_style, size_t sz_vec2, size_t sz_vec4, size_t sz_vert, size_t sz_idx)
{
    bool error = false;
    if (strcmp(version, IMGUI_VERSION) != 0) { error = true; IM_ASSERT(strcmp(version, IMGUI_VERSION) == 0 && "Mismatched version string!"); }
    if (sz_io    != sizeof(ImGuiIO))    { error = true; IM_ASSERT(sz_io == sizeof(ImGuiIO) && "Mismatched struct layout!"); }
    if (sz_style != sizeof(ImGuiStyle)) { error = true; IM_ASSERT(sz_style == sizeof(ImGuiStyle) && "Mismatched struct layout!"); }
    if (sz_vec2  != sizeof(ImVec2))     { error = true; IM_ASSERT(sz_vec2 == sizeof(ImVec2) && "Mismatched struct layout!"); }
    if (sz_vec4  != sizeof(ImVec4))     { error = true; IM_ASSERT(sz_vec4 == sizeof(ImVec4) && "Mismatched struct layout!"); }
    if (sz_vert  != sizeof(ImDrawVert)) { error = true; IM_ASSERT(sz_vert == sizeof(ImDrawVert) && "Mismatched struct layout!"); }
    if (sz_idx   != sizeof(ImDrawIdx))  { error = true; IM_ASSERT(sz_idx == sizeof(ImDrawIdx) && "Mismatched struct layout!"); }
    return !error;
}

// Until 1.89 (IMGUI_VERSION_NUM < 18814) it was legal to use SetCursorPos() to extend the boundary of a parent (e.g. window or table cell)
// This is causing issues and ambiguity and we need to retire that.
// See https://github.com/ocornut/imgui/issues/5548 for more details.
// [Scenario 1]
//  Previously this would make the window content size ~200x200:
//    Begin(...) + SetCursorScreenPos(GetCursorScreenPos() + ImVec2(200,200)) + End();  // NOT OK
//  Instead, please submit an item:
//    Begin(...) + SetCursorScreenPos(GetCursorScreenPos() + ImVec2(200,200)) + Dummy(ImVec2(0,0)) + End(); // OK
//  Alternative:
//    Begin(...) + Dummy(ImVec2(200,200)) + End(); // OK
// [Scenario 2]
//  For reference this is one of the issue what we aim to fix with this change:
//    BeginGroup() + SomeItem("foobar") + SetCursorScreenPos(GetCursorScreenPos()) + EndGroup()
//  The previous logic made SetCursorScreenPos(GetCursorScreenPos()) have a side-effect! It would erroneously incorporate ItemSpacing.y after the item into content size, making the group taller!
//  While this code is a little twisted, no-one would expect SetXXX(GetXXX()) to have a side-effect. Using vertical alignment patterns could trigger this issue.
void ImGui::ErrorCheckUsingSetCursorPosToExtendParentBoundaries()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(window->DC.IsSetPos);
    window->DC.IsSetPos = false;
#ifdef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    if (window->DC.CursorPos.x <= window->DC.CursorMaxPos.x && window->DC.CursorPos.y <= window->DC.CursorMaxPos.y)
        return;
    if (window->SkipItems)
        return;
    IM_ASSERT(0 && "Code uses SetCursorPos()/SetCursorScreenPos() to extend window/parent boundaries. Please submit an item e.g. Dummy() to validate extent.");
#else
    window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
#endif
}

static void ImGui::ErrorCheckNewFrameSanityChecks()
{
    ImGuiContext& g = *GImGui;

    // Check user IM_ASSERT macro
    // (IF YOU GET A WARNING OR COMPILE ERROR HERE: it means your assert macro is incorrectly defined!
    //  If your macro uses multiple statements, it NEEDS to be surrounded by a 'do { ... } while (0)' block.
    //  This is a common C/C++ idiom to allow multiple statements macros to be used in control flow blocks.)
    // #define IM_ASSERT(EXPR)   if (SomeCode(EXPR)) SomeMoreCode();                    // Wrong!
    // #define IM_ASSERT(EXPR)   do { if (SomeCode(EXPR)) SomeMoreCode(); } while (0)   // Correct!
    if (true) IM_ASSERT(1); else IM_ASSERT(0);

    // Emscripten backends are often imprecise in their submission of DeltaTime. (#6114, #3644)
    // Ideally the Emscripten app/backend should aim to fix or smooth this value and avoid feeding zero, but we tolerate it.
#ifdef __EMSCRIPTEN__
    if (g.IO.DeltaTime <= 0.0f && g.FrameCount > 0)
        g.IO.DeltaTime = 0.00001f;
#endif

    // Check user data
    // (We pass an error message in the assert expression to make it visible to programmers who are not using a debugger, as most assert handlers display their argument)
    IM_ASSERT(g.Initialized);
    IM_ASSERT((g.IO.DeltaTime > 0.0f || g.FrameCount == 0)              && "Need a positive DeltaTime!");
    IM_ASSERT((g.FrameCount == 0 || g.FrameCountEnded == g.FrameCount)  && "Forgot to call Render() or EndFrame() at the end of the previous frame?");
    IM_ASSERT(g.IO.DisplaySize.x >= 0.0f && g.IO.DisplaySize.y >= 0.0f  && "Invalid DisplaySize value!");
    IM_ASSERT(g.IO.Fonts->IsBuilt()                                     && "Font Atlas not built! Make sure you called ImGui_ImplXXXX_NewFrame() function for renderer backend, which should call io.Fonts->GetTexDataAsRGBA32() / GetTexDataAsAlpha8()");
    IM_ASSERT(g.Style.CurveTessellationTol > 0.0f                       && "Invalid style setting!");
    IM_ASSERT(g.Style.CircleTessellationMaxError > 0.0f                 && "Invalid style setting!");
    IM_ASSERT(g.Style.Alpha >= 0.0f && g.Style.Alpha <= 1.0f            && "Invalid style setting!"); // Allows us to avoid a few clamps in color computations
    IM_ASSERT(g.Style.WindowMinSize.x >= 1.0f && g.Style.WindowMinSize.y >= 1.0f && "Invalid style setting.");
    IM_ASSERT(g.Style.WindowMenuButtonPosition == ImGuiDir_None || g.Style.WindowMenuButtonPosition == ImGuiDir_Left || g.Style.WindowMenuButtonPosition == ImGuiDir_Right);
    IM_ASSERT(g.Style.ColorButtonPosition == ImGuiDir_Left || g.Style.ColorButtonPosition == ImGuiDir_Right);
#ifndef IMGUI_DISABLE_OBSOLETE_KEYIO
    for (int n = ImGuiKey_NamedKey_BEGIN; n < ImGuiKey_COUNT; n++)
        IM_ASSERT(g.IO.KeyMap[n] >= -1 && g.IO.KeyMap[n] < ImGuiKey_LegacyNativeKey_END && "io.KeyMap[] contains an out of bound value (need to be 0..511, or -1 for unmapped key)");

    // Check: required key mapping (we intentionally do NOT check all keys to not pressure user into setting up everything, but Space is required and was only added in 1.60 WIP)
    if ((g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) && g.IO.BackendUsingLegacyKeyArrays == 1)
        IM_ASSERT(g.IO.KeyMap[ImGuiKey_Space] != -1 && "ImGuiKey_Space is not mapped, required for keyboard navigation.");
#endif

    // Error handling: we do not accept 100% silent recovery! Please contact me if you feel this is getting in your way.
    if (g.IO.ConfigErrorRecovery)
        IM_ASSERT(g.IO.ConfigErrorRecoveryEnableAssert || g.IO.ConfigErrorRecoveryEnableDebugLog || g.IO.ConfigErrorRecoveryEnableTooltip || g.ErrorCallback != NULL);

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    // Remap legacy names
    if (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableSetMousePos)
    {
        g.IO.ConfigNavMoveSetMousePos = true;
        g.IO.ConfigFlags &= ~ImGuiConfigFlags_NavEnableSetMousePos;
    }
    if (g.IO.ConfigFlags & ImGuiConfigFlags_NavNoCaptureKeyboard)
    {
        g.IO.ConfigNavCaptureKeyboard = false;
        g.IO.ConfigFlags &= ~ImGuiConfigFlags_NavNoCaptureKeyboard;
    }

    // Remap legacy clipboard handlers (OBSOLETED in 1.91.1, August 2024)
    if (g.IO.GetClipboardTextFn != NULL && (g.PlatformIO.Platform_GetClipboardTextFn == NULL || g.PlatformIO.Platform_GetClipboardTextFn == Platform_GetClipboardTextFn_DefaultImpl))
        g.PlatformIO.Platform_GetClipboardTextFn = [](ImGuiContext* ctx) { return ctx->IO.GetClipboardTextFn(ctx->IO.ClipboardUserData); };
    if (g.IO.SetClipboardTextFn != NULL && (g.PlatformIO.Platform_SetClipboardTextFn == NULL || g.PlatformIO.Platform_SetClipboardTextFn == Platform_SetClipboardTextFn_DefaultImpl))
        g.PlatformIO.Platform_SetClipboardTextFn = [](ImGuiContext* ctx, const char* text) { return ctx->IO.SetClipboardTextFn(ctx->IO.ClipboardUserData, text); };
#endif
}

static void ImGui::ErrorCheckEndFrameSanityChecks()
{
    // Verify that io.KeyXXX fields haven't been tampered with. Key mods should not be modified between NewFrame() and EndFrame()
    // One possible reason leading to this assert is that your backends update inputs _AFTER_ NewFrame().
    // It is known that when some modal native windows called mid-frame takes focus away, some backends such as GLFW will
    // send key release events mid-frame. This would normally trigger this assertion and lead to sheared inputs.
    // We silently accommodate for this case by ignoring the case where all io.KeyXXX modifiers were released (aka key_mod_flags == 0),
    // while still correctly asserting on mid-frame key press events.
    ImGuiContext& g = *GImGui;
    const ImGuiKeyChord key_mods = GetMergedModsFromKeys();
    IM_UNUSED(g);
    IM_UNUSED(key_mods);
    IM_ASSERT((key_mods == 0 || g.IO.KeyMods == key_mods) && "Mismatching io.KeyCtrl/io.KeyShift/io.KeyAlt/io.KeySuper vs io.KeyMods");
    IM_UNUSED(key_mods);

    IM_ASSERT(g.CurrentWindowStack.Size == 1);
    IM_ASSERT(g.CurrentWindowStack[0].Window->IsFallbackWindow);
}

// Save current stack sizes. Called e.g. by NewFrame() and by Begin() but may be called for manual recovery.
void ImGui::ErrorRecoveryStoreState(ImGuiErrorRecoveryState* state_out)
{
    ImGuiContext& g = *GImGui;
    state_out->SizeOfWindowStack = (short)g.CurrentWindowStack.Size;
    state_out->SizeOfIDStack = (short)g.CurrentWindow->IDStack.Size;
    state_out->SizeOfTreeStack = (short)g.CurrentWindow->DC.TreeDepth; // NOT g.TreeNodeStack.Size which is a partial stack!
    state_out->SizeOfColorStack = (short)g.ColorStack.Size;
    state_out->SizeOfStyleVarStack = (short)g.StyleVarStack.Size;
    state_out->SizeOfFontStack = (short)g.FontStack.Size;
    state_out->SizeOfFocusScopeStack = (short)g.FocusScopeStack.Size;
    state_out->SizeOfGroupStack = (short)g.GroupStack.Size;
    state_out->SizeOfItemFlagsStack = (short)g.ItemFlagsStack.Size;
    state_out->SizeOfBeginPopupStack = (short)g.BeginPopupStack.Size;
    state_out->SizeOfDisabledStack = (short)g.DisabledStackSize;
}

// Chosen name "Try to recover" over e.g. "Restore" to suggest this is not a 100% guaranteed recovery.
// Called by e.g. EndFrame() but may be called for manual recovery.
// Attempt to recover full window stack.
void ImGui::ErrorRecoveryTryToRecoverState(const ImGuiErrorRecoveryState* state_in)
{
    // PVS-Studio V1044 is "Loop break conditions do not depend on the number of iterations"
    ImGuiContext& g = *GImGui;
    while (g.CurrentWindowStack.Size > state_in->SizeOfWindowStack) //-V1044
    {
        // Recap:
        // - Begin()/BeginChild() return false to indicate the window is collapsed or fully clipped.
        // - Always call a matching End() for each Begin() call, regardless of its return value!
        // - Begin/End and BeginChild/EndChild logic is KNOWN TO BE INCONSISTENT WITH ALL OTHER BEGIN/END FUNCTIONS.
        // - We will fix that in a future major update.
        ImGuiWindow* window = g.CurrentWindow;
        if (window->Flags & ImGuiWindowFlags_ChildWindow)
        {
            IM_ASSERT_USER_ERROR(0, "Missing EndChild()");
            EndChild();
        }
        else
        {
            IM_ASSERT_USER_ERROR(0, "Missing End()");
            End();
        }
    }
    if (g.CurrentWindowStack.Size == state_in->SizeOfWindowStack)
        ErrorRecoveryTryToRecoverWindowState(state_in);
}

// Called by e.g. End() but may be called for manual recovery.
// Read '// Error Handling [BETA]' block in imgui_internal.h for details.
// Attempt to recover from incorrect usage of BeginXXX/EndXXX/PushXXX/PopXXX calls.
void    ImGui::ErrorRecoveryTryToRecoverWindowState(const ImGuiErrorRecoveryState* state_in)
{
    ImGuiContext& g = *GImGui;

    while (g.CurrentTable != NULL && g.CurrentTable->InnerWindow == g.CurrentWindow) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing EndTable()");
        EndTable();
    }

    ImGuiWindow* window = g.CurrentWindow;

    // FIXME: Can't recover from inside BeginTabItem/EndTabItem yet.
    while (g.CurrentTabBar != NULL && g.CurrentTabBar->Window == window) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing EndTabBar()");
        EndTabBar();
    }
    while (g.CurrentMultiSelect != NULL && g.CurrentMultiSelect->Storage->Window == window) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing EndMultiSelect()");
        EndMultiSelect();
    }
    while (window->DC.TreeDepth > state_in->SizeOfTreeStack) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing TreePop()");
        TreePop();
    }
    while (g.GroupStack.Size > state_in->SizeOfGroupStack) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing EndGroup()");
        EndGroup();
    }
    IM_ASSERT(g.GroupStack.Size == state_in->SizeOfGroupStack);
    while (window->IDStack.Size > state_in->SizeOfIDStack) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing PopID()");
        PopID();
    }
    while (g.DisabledStackSize > state_in->SizeOfDisabledStack) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing EndDisabled()");
        if (g.CurrentItemFlags & ImGuiItemFlags_Disabled)
            EndDisabled();
        else
        {
            EndDisabledOverrideReenable();
            g.CurrentWindowStack.back().DisabledOverrideReenable = false;
        }
    }
    IM_ASSERT(g.DisabledStackSize == state_in->SizeOfDisabledStack);
    while (g.ColorStack.Size > state_in->SizeOfColorStack) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing PopStyleColor()");
        PopStyleColor();
    }
    while (g.ItemFlagsStack.Size > state_in->SizeOfItemFlagsStack) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing PopItemFlag()");
        PopItemFlag();
    }
    while (g.StyleVarStack.Size > state_in->SizeOfStyleVarStack) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing PopStyleVar()");
        PopStyleVar();
    }
    while (g.FontStack.Size > state_in->SizeOfFontStack) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing PopFont()");
        PopFont();
    }
    while (g.FocusScopeStack.Size > state_in->SizeOfFocusScopeStack) //-V1044
    {
        IM_ASSERT_USER_ERROR(0, "Missing PopFocusScope()");
        PopFocusScope();
    }
    //IM_ASSERT(g.FocusScopeStack.Size == state_in->SizeOfFocusScopeStack);
}

bool    ImGui::ErrorLog(const char* msg)
{
    ImGuiContext& g = *GImGui;

    // Output to debug log
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    ImGuiWindow* window = g.CurrentWindow;

    if (g.IO.ConfigErrorRecoveryEnableDebugLog)
    {
        if (g.ErrorFirst)
            IMGUI_DEBUG_LOG_ERROR("[imgui-error] (current settings: Assert=%d, Log=%d, Tooltip=%d)\n",
                g.IO.ConfigErrorRecoveryEnableAssert, g.IO.ConfigErrorRecoveryEnableDebugLog, g.IO.ConfigErrorRecoveryEnableTooltip);
        IMGUI_DEBUG_LOG_ERROR("[imgui-error] In window '%s': %s\n", window ? window->Name : "NULL", msg);
    }
    g.ErrorFirst = false;

    // Output to tooltip
    if (g.IO.ConfigErrorRecoveryEnableTooltip)
    {
        if (BeginErrorTooltip())
        {
            if (g.ErrorCountCurrentFrame < 20)
            {
                Text("In window '%s': %s", window ? window->Name : "NULL", msg);
                if (window && (!window->IsFallbackWindow || window->WasActive))
                    GetForegroundDrawList(window)->AddRect(window->Pos, window->Pos + window->Size, IM_COL32(255, 0, 0, 255));
            }
            if (g.ErrorCountCurrentFrame == 20)
                Text("(and more errors)");
            // EndFrame() will amend debug buttons to this window, after all errors have been submitted.
            EndErrorTooltip();
        }
        g.ErrorCountCurrentFrame++;
    }
#endif

    // Output to callback
    if (g.ErrorCallback != NULL)
        g.ErrorCallback(&g, g.ErrorCallbackUserData, msg);

    // Return whether we should assert
    return g.IO.ConfigErrorRecoveryEnableAssert;
}

void ImGui::ErrorCheckEndFrameFinalizeErrorTooltip()
{
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    ImGuiContext& g = *GImGui;
    if (g.DebugDrawIdConflicts != 0 && g.IO.KeyCtrl == false)
        g.DebugDrawIdConflictsCount = g.HoveredIdPreviousFrameItemCount;
    if (g.DebugDrawIdConflicts != 0 && g.DebugItemPickerActive == false && BeginErrorTooltip())
    {
        Text("Programmer error: %d visible items with conflicting ID!", g.DebugDrawIdConflictsCount);
        BulletText("Code should use PushID()/PopID() in loops, or append \"##xx\" to same-label identifiers!");
        BulletText("Empty label e.g. Button(\"\") == same ID as parent widget/node. Use Button(\"##xx\") instead!");
        //BulletText("Code intending to use duplicate ID may use e.g. PushItemFlag(ImGuiItemFlags_AllowDuplicateId, true); ... PopItemFlag()"); // Not making this too visible for fear of it being abused.
        BulletText("Set io.ConfigDebugDetectIdConflicts=false to disable this warning in non-programmers builds.");
        Separator();
        Text("(Hold CTRL to: use");
        SameLine();
        if (SmallButton("Item Picker"))
            DebugStartItemPicker();
        SameLine();
        Text("to break in item call-stack, or");
        SameLine();
        if (SmallButton("Open FAQ->About ID Stack System") && g.PlatformIO.Platform_OpenInShellFn != NULL)
            g.PlatformIO.Platform_OpenInShellFn(&g, "https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#qa-usage");
        EndErrorTooltip();
    }

    if (g.ErrorCountCurrentFrame > 0 && BeginErrorTooltip()) // Amend at end of frame
    {
        Separator();
        Text("(Hold CTRL to:");
        SameLine();
        if (SmallButton("Enable Asserts"))
            g.IO.ConfigErrorRecoveryEnableAssert = true;
        //SameLine();
        //if (SmallButton("Hide Error Tooltips"))
        //    g.IO.ConfigErrorRecoveryEnableTooltip = false; // Too dangerous
        SameLine(0, 0);
        Text(")");
        EndErrorTooltip();
    }
#endif
}

// Pseudo-tooltip. Follow mouse until CTRL is held. When CTRL is held we lock position, allowing to click it.
bool ImGui::BeginErrorTooltip()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = FindWindowByName("##Tooltip_Error");
    const bool use_locked_pos = (g.IO.KeyCtrl && window && window->WasActive);
    PushStyleColor(ImGuiCol_PopupBg, ImLerp(g.Style.Colors[ImGuiCol_PopupBg], ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 0.15f));
    if (use_locked_pos)
        SetNextWindowPos(g.ErrorTooltipLockedPos);
    bool is_visible = Begin("##Tooltip_Error", NULL, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
    PopStyleColor();
    if (is_visible && g.CurrentWindow->BeginCount == 1)
    {
        SeparatorText("MESSAGE FROM DEAR IMGUI");
        BringWindowToDisplayFront(g.CurrentWindow);
        BringWindowToFocusFront(g.CurrentWindow);
        g.ErrorTooltipLockedPos = GetWindowPos();
    }
    else if (!is_visible)
    {
        End();
    }
    return is_visible;
}

void ImGui::EndErrorTooltip()
{
    End();
}

//-----------------------------------------------------------------------------
// [SECTION] ITEM SUBMISSION
//-----------------------------------------------------------------------------
// - KeepAliveID()
// - ItemAdd()
//-----------------------------------------------------------------------------

// Code not using ItemAdd() may need to call this manually otherwise ActiveId will be cleared. In IMGUI_VERSION_NUM < 18717 this was called by GetID().
void ImGui::KeepAliveID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId == id)
        g.ActiveIdIsAlive = id;
    if (g.ActiveIdPreviousFrame == id)
        g.ActiveIdPreviousFrameIsAlive = true;
}

// Declare item bounding box for clipping and interaction.
// Note that the size can be different than the one provided to ItemSize(). Typically, widgets that spread over available surface
// declare their minimum size requirement to ItemSize() and provide a larger region to ItemAdd() which is used drawing/interaction.
// THIS IS IN THE PERFORMANCE CRITICAL PATH (UNTIL THE CLIPPING TEST AND EARLY-RETURN)
IM_MSVC_RUNTIME_CHECKS_OFF
bool ImGui::ItemAdd(const ImRect& bb, ImGuiID id, const ImRect* nav_bb_arg, ImGuiItemFlags extra_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // Set item data
    // (DisplayRect is left untouched, made valid when ImGuiItemStatusFlags_HasDisplayRect is set)
    g.LastItemData.ID = id;
    g.LastItemData.Rect = bb;
    g.LastItemData.NavRect = nav_bb_arg ? *nav_bb_arg : bb;
    g.LastItemData.InFlags = g.CurrentItemFlags | g.NextItemData.ItemFlags | extra_flags;
    g.LastItemData.StatusFlags = ImGuiItemStatusFlags_None;
    // Note: we don't copy 'g.NextItemData.SelectionUserData' to an hypothetical g.LastItemData.SelectionUserData: since the former is not cleared.

    if (id != 0)
    {
        KeepAliveID(id);

        // Directional navigation processing
        // Runs prior to clipping early-out
        //  (a) So that NavInitRequest can be honored, for newly opened windows to select a default widget
        //  (b) So that we can scroll up/down past clipped items. This adds a small O(N) cost to regular navigation requests
        //      unfortunately, but it is still limited to one window. It may not scale very well for windows with ten of
        //      thousands of item, but at least NavMoveRequest is only set on user interaction, aka maximum once a frame.
        //      We could early out with "if (is_clipped && !g.NavInitRequest) return false;" but when we wouldn't be able
        //      to reach unclipped widgets. This would work if user had explicit scrolling control (e.g. mapped on a stick).
        // We intentionally don't check if g.NavWindow != NULL because g.NavAnyRequest should only be set when it is non null.
        // If we crash on a NULL g.NavWindow we need to fix the bug elsewhere.
        if (!(g.LastItemData.InFlags & ImGuiItemFlags_NoNav))
        {
            // FIMXE-NAV: investigate changing the window tests into a simple 'if (g.NavFocusScopeId == g.CurrentFocusScopeId)' test.
            window->DC.NavLayersActiveMaskNext |= (1 << window->DC.NavLayerCurrent);
            if (g.NavId == id || g.NavAnyRequest)
                if (g.NavWindow->RootWindowForNav == window->RootWindowForNav)
                    if (window == g.NavWindow || ((window->ChildFlags | g.NavWindow->ChildFlags) & ImGuiChildFlags_NavFlattened))
                        NavProcessItem();
        }

        if (g.NextItemData.Flags & ImGuiNextItemDataFlags_HasShortcut)
            ItemHandleShortcut(id);
    }

    // Lightweight clear of SetNextItemXXX data.
    g.NextItemData.Flags = ImGuiNextItemDataFlags_None;
    g.NextItemData.ItemFlags = ImGuiItemFlags_None;

#ifdef IMGUI_ENABLE_TEST_ENGINE
    if (id != 0)
        IMGUI_TEST_ENGINE_ITEM_ADD(id, g.LastItemData.NavRect, &g.LastItemData);
#endif

    // Clipping test
    // (this is an inline copy of IsClippedEx() so we can reuse the is_rect_visible value, otherwise we'd do 'if (IsClippedEx(bb, id)) return false')
    // g.NavActivateId is not necessarily == g.NavId, in the case of remote activation (e.g. shortcuts)
    const bool is_rect_visible = bb.Overlaps(window->ClipRect);
    if (!is_rect_visible)
        if (id == 0 || (id != g.ActiveId && id != g.ActiveIdPreviousFrame && id != g.NavId && id != g.NavActivateId))
            if (!g.ItemUnclipByLog)
                return false;

    // [DEBUG]
#ifndef IMGUI_DISABLE_DEBUG_TOOLS
    if (id != 0)
    {
        if (id == g.DebugLocateId)
            DebugLocateItemResolveWithLastItem();

        // [DEBUG] People keep stumbling on this problem and using "" as identifier in the root of a window instead of "##something".
        // Empty identifier are valid and useful in a small amount of cases, but 99.9% of the time you want to use "##something".
        // READ THE FAQ: https://dearimgui.com/faq
        IM_ASSERT(id != window->ID && "Cannot have an empty ID at the root of a window. If you need an empty label, use ## and read the FAQ about how the ID Stack works!");
    }
    //if (g.IO.KeyAlt) window->DrawList->AddRect(bb.Min, bb.Max, IM_COL32(255,255,0,120)); // [DEBUG]
    //if ((g.LastItemData.InFlags & ImGuiItemFlags_NoNav) == 0)
    //    window->DrawList->AddRect(g.LastItemData.NavRect.Min, g.LastItemData.NavRect.Max, IM_COL32(255,255,0,255)); // [DEBUG]
#endif

    // We need to calculate this now to take account of the current clipping rectangle (as items like Selectable may change them)
    if (is_rect_visible)
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_Visible;
    if (IsMouseHoveringRect(bb.Min, bb.Max))
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredRect;
    return true;
}
IM_MSVC_RUNTIME_CHECKS_RESTORE

//-----------------------------------------------------------------------------
// [SECTION] LAYOUT
//-----------------------------------------------------------------------------
// - ItemSize()
// - SameLine()
// - GetCursorScreenPos()
// - SetCursorScreenPos()
// - GetCursorPos(), GetCursorPosX(), GetCursorPosY()
// - SetCursorPos(), SetCursorPosX(), SetCursorPosY()
// - GetCursorStartPos()
// - Indent()
// - Unindent()
// - SetNextItemWidth()
// - PushItemWidth()
// - PushMultiItemsWidths()
// - PopItemWidth()
// - CalcItemWidth()
// - CalcItemSize()
// - GetTextLineHeight()
// - GetTextLineHeightWithSpacing()
// - GetFrameHeight()
// - GetFrameHeightWithSpacing()
// - GetContentRegionMax()
// - GetContentRegionAvail(),
// - BeginGroup()
// - EndGroup()
// Also see in imgui_widgets: tab bars, and in imgui_tables: tables, columns.
//-----------------------------------------------------------------------------

// Advance cursor given item size for layout.
// Register minimum needed size so it can extend the bounding box used for auto-fit calculation.
// See comments in ItemAdd() about how/why the size provided to ItemSize() vs ItemAdd() may often different.
// THIS IS IN THE PERFORMANCE CRITICAL PATH.
IM_MSVC_RUNTIME_CHECKS_OFF
void ImGui::ItemSize(const ImVec2& size, float text_baseline_y)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    // We increase the height in this function to accommodate for baseline offset.
    // In theory we should be offsetting the starting position (window->DC.CursorPos), that will be the topic of a larger refactor,
    // but since ItemSize() is not yet an API that moves the cursor (to handle e.g. wrapping) enlarging the height has the same effect.
    const float offset_to_match_baseline_y = (text_baseline_y >= 0) ? ImMax(0.0f, window->DC.CurrLineTextBaseOffset - text_baseline_y) : 0.0f;

    const float line_y1 = window->DC.IsSameLine ? window->DC.CursorPosPrevLine.y : window->DC.CursorPos.y;
    const float line_height = ImMax(window->DC.CurrLineSize.y, /*ImMax(*/window->DC.CursorPos.y - line_y1/*, 0.0f)*/ + size.y + offset_to_match_baseline_y);

    // Always align ourselves on pixel boundaries
    //if (g.IO.KeyAlt) window->DrawList->AddRect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(size.x, line_height), IM_COL32(255,0,0,200)); // [DEBUG]
    window->DC.CursorPosPrevLine.x = window->DC.CursorPos.x + size.x;
    window->DC.CursorPosPrevLine.y = line_y1;
    window->DC.CursorPos.x = IM_TRUNC(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);    // Next line
    window->DC.CursorPos.y = IM_TRUNC(line_y1 + line_height + g.Style.ItemSpacing.y);                       // Next line
    window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPosPrevLine.x);
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, window->DC.CursorPos.y - g.Style.ItemSpacing.y);
    //if (g.IO.KeyAlt) window->DrawList->AddCircle(window->DC.CursorMaxPos, 3.0f, IM_COL32(255,0,0,255), 4); // [DEBUG]

    window->DC.PrevLineSize.y = line_height;
    window->DC.CurrLineSize.y = 0.0f;
    window->DC.PrevLineTextBaseOffset = ImMax(window->DC.CurrLineTextBaseOffset, text_baseline_y);
    window->DC.CurrLineTextBaseOffset = 0.0f;
    window->DC.IsSameLine = window->DC.IsSetPos = false;

    // Horizontal layout mode
    if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
        SameLine();
}
IM_MSVC_RUNTIME_CHECKS_RESTORE

// Gets back to previous line and continue with horizontal layout
//      offset_from_start_x == 0 : follow right after previous item
//      offset_from_start_x != 0 : align to specified x position (relative to window/group left)
//      spacing_w < 0            : use default spacing if offset_from_start_x == 0, no spacing if offset_from_start_x != 0
//      spacing_w >= 0           : enforce spacing amount
void ImGui::SameLine(float offset_from_start_x, float spacing_w)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    if (offset_from_start_x != 0.0f)
    {
        if (spacing_w < 0.0f)
            spacing_w = 0.0f;
        window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + offset_from_start_x + spacing_w + window->DC.GroupOffset.x + window->DC.ColumnsOffset.x;
        window->DC.CursorPos.y = window->DC.CursorPosPrevLine.y;
    }
    else
    {
        if (spacing_w < 0.0f)
            spacing_w = g.Style.ItemSpacing.x;
        window->DC.CursorPos.x = window->DC.CursorPosPrevLine.x + spacing_w;
        window->DC.CursorPos.y = window->DC.CursorPosPrevLine.y;
    }
    window->DC.CurrLineSize = window->DC.PrevLineSize;
    window->DC.CurrLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
    window->DC.IsSameLine = true;
}

ImVec2 ImGui::GetCursorScreenPos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos;
}

void ImGui::SetCursorScreenPos(const ImVec2& pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos = pos;
    //window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
    window->DC.IsSetPos = true;
}

// User generally sees positions in window coordinates. Internally we store CursorPos in absolute screen coordinates because it is more convenient.
// Conversion happens as we pass the value to user, but it makes our naming convention confusing because GetCursorPos() == (DC.CursorPos - window.Pos). May want to rename 'DC.CursorPos'.
ImVec2 ImGui::GetCursorPos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos - window->Pos + window->Scroll;
}

float ImGui::GetCursorPosX()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos.x - window->Pos.x + window->Scroll.x;
}

float ImGui::GetCursorPosY()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos.y - window->Pos.y + window->Scroll.y;
}

void ImGui::SetCursorPos(const ImVec2& local_pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos = window->Pos - window->Scroll + local_pos;
    //window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
    window->DC.IsSetPos = true;
}

void ImGui::SetCursorPosX(float x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + x;
    //window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPos.x);
    window->DC.IsSetPos = true;
}

void ImGui::SetCursorPosY(float y)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.y = window->Pos.y - window->Scroll.y + y;
    //window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, window->DC.CursorPos.y);
    window->DC.IsSetPos = true;
}

ImVec2 ImGui::GetCursorStartPos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorStartPos - window->Pos;
}

void ImGui::Indent(float indent_w)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.Indent.x += (indent_w != 0.0f) ? indent_w : g.Style.IndentSpacing;
    window->DC.CursorPos.x = window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x;
}

void ImGui::Unindent(float indent_w)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.Indent.x -= (indent_w != 0.0f) ? indent_w : g.Style.IndentSpacing;
    window->DC.CursorPos.x = window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x;
}

// Affect large frame+labels widgets only.
void ImGui::SetNextItemWidth(float item_width)
{
    ImGuiContext& g = *GImGui;
    g.NextItemData.Flags |= ImGuiNextItemDataFlags_HasWidth;
    g.NextItemData.Width = item_width;
}

// FIXME: Remove the == 0.0f behavior?
void ImGui::PushItemWidth(float item_width)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DC.ItemWidthStack.push_back(window->DC.ItemWidth); // Backup current width
    window->DC.ItemWidth = (item_width == 0.0f ? window->ItemWidthDefault : item_width);
    g.NextItemData.Flags &= ~ImGuiNextItemDataFlags_HasWidth;
}

void ImGui::PushMultiItemsWidths(int components, float w_full)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(components > 0);
    const ImGuiStyle& style = g.Style;
    window->DC.ItemWidthStack.push_back(window->DC.ItemWidth); // Backup current width
    float w_items = w_full - style.ItemInnerSpacing.x * (components - 1);
    float prev_split = w_items;
    for (int i = components - 1; i > 0; i--)
    {
        float next_split = IM_TRUNC(w_items * i / components);
        window->DC.ItemWidthStack.push_back(ImMax(prev_split - next_split, 1.0f));
        prev_split = next_split;
    }
    window->DC.ItemWidth = ImMax(prev_split, 1.0f);
    g.NextItemData.Flags &= ~ImGuiNextItemDataFlags_HasWidth;
}

void ImGui::PopItemWidth()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->DC.ItemWidthStack.Size <= 0)
    {
        IM_ASSERT_USER_ERROR(0, "Calling PopItemWidth() too many times!");
        return;
    }
    window->DC.ItemWidth = window->DC.ItemWidthStack.back();
    window->DC.ItemWidthStack.pop_back();
}

// Calculate default item width given value passed to PushItemWidth() or SetNextItemWidth().
// The SetNextItemWidth() data is generally cleared/consumed by ItemAdd() or NextItemData.ClearFlags()
float ImGui::CalcItemWidth()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    float w;
    if (g.NextItemData.Flags & ImGuiNextItemDataFlags_HasWidth)
        w = g.NextItemData.Width;
    else
        w = window->DC.ItemWidth;
    if (w < 0.0f)
    {
        float region_avail_x = GetContentRegionAvail().x;
        w = ImMax(1.0f, region_avail_x + w);
    }
    w = IM_TRUNC(w);
    return w;
}

// [Internal] Calculate full item size given user provided 'size' parameter and default width/height. Default width is often == CalcItemWidth().
// Those two functions CalcItemWidth vs CalcItemSize are awkwardly named because they are not fully symmetrical.
// Note that only CalcItemWidth() is publicly exposed.
// The 4.0f here may be changed to match CalcItemWidth() and/or BeginChild() (right now we have a mismatch which is harmless but undesirable)
ImVec2 ImGui::CalcItemSize(ImVec2 size, float default_w, float default_h)
{
    ImVec2 avail;
    if (size.x < 0.0f || size.y < 0.0f)
        avail = GetContentRegionAvail();

    if (size.x == 0.0f)
        size.x = default_w;
    else if (size.x < 0.0f)
        size.x = ImMax(4.0f, avail.x + size.x); // <-- size.x is negative here so we are subtracting

    if (size.y == 0.0f)
        size.y = default_h;
    else if (size.y < 0.0f)
        size.y = ImMax(4.0f, avail.y + size.y); // <-- size.y is negative here so we are subtracting

    return size;
}

float ImGui::GetTextLineHeight()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize;
}

float ImGui::GetTextLineHeightWithSpacing()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize + g.Style.ItemSpacing.y;
}

float ImGui::GetFrameHeight()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize + g.Style.FramePadding.y * 2.0f;
}

float ImGui::GetFrameHeightWithSpacing()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize + g.Style.FramePadding.y * 2.0f + g.Style.ItemSpacing.y;
}

ImVec2 ImGui::GetContentRegionAvail()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImVec2 mx = (window->DC.CurrentColumns || g.CurrentTable) ? window->WorkRect.Max : window->ContentRegionRect.Max;
    return mx - window->DC.CursorPos;
}

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS

// You should never need those functions. Always use GetCursorScreenPos() and GetContentRegionAvail()!
// They are bizarre local-coordinates which don't play well with scrolling.
ImVec2 ImGui::GetContentRegionMax()
{
    return GetContentRegionAvail() + GetCursorScreenPos() - GetWindowPos();
}

ImVec2 ImGui::GetWindowContentRegionMin()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ContentRegionRect.Min - window->Pos;
}

ImVec2 ImGui::GetWindowContentRegionMax()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ContentRegionRect.Max - window->Pos;
}
#endif

// Lock horizontal starting position + capture group bounding box into one "item" (so you can use IsItemHovered() or layout primitives such as SameLine() on whole group, etc.)
// Groups are currently a mishmash of functionalities which should perhaps be clarified and separated.
// FIXME-OPT: Could we safely early out on ->SkipItems?
void ImGui::BeginGroup()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    g.GroupStack.resize(g.GroupStack.Size + 1);
    ImGuiGroupData& group_data = g.GroupStack.back();
    group_data.WindowID = window->ID;
    group_data.BackupCursorPos = window->DC.CursorPos;
    group_data.BackupCursorPosPrevLine = window->DC.CursorPosPrevLine;
    group_data.BackupCursorMaxPos = window->DC.CursorMaxPos;
    group_data.BackupIndent = window->DC.Indent;
    group_data.BackupGroupOffset = window->DC.GroupOffset;
    group_data.BackupCurrLineSize = window->DC.CurrLineSize;
    group_data.BackupCurrLineTextBaseOffset = window->DC.CurrLineTextBaseOffset;
    group_data.BackupActiveIdIsAlive = g.ActiveIdIsAlive;
    group_data.BackupHoveredIdIsAlive = g.HoveredId != 0;
    group_data.BackupIsSameLine = window->DC.IsSameLine;
    group_data.BackupActiveIdPreviousFrameIsAlive = g.ActiveIdPreviousFrameIsAlive;
    group_data.EmitItem = true;

    window->DC.GroupOffset.x = window->DC.CursorPos.x - window->Pos.x - window->DC.ColumnsOffset.x;
    window->DC.Indent = window->DC.GroupOffset;
    window->DC.CursorMaxPos = window->DC.CursorPos;
    window->DC.CurrLineSize = ImVec2(0.0f, 0.0f);
    if (g.LogEnabled)
        g.LogLinePosY = -FLT_MAX; // To enforce a carriage return
}

void ImGui::EndGroup()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(g.GroupStack.Size > 0); // Mismatched BeginGroup()/EndGroup() calls

    ImGuiGroupData& group_data = g.GroupStack.back();
    IM_ASSERT(group_data.WindowID == window->ID); // EndGroup() in wrong window?

    if (window->DC.IsSetPos)
        ErrorCheckUsingSetCursorPosToExtendParentBoundaries();

    // Include LastItemData.Rect.Max as a workaround for e.g. EndTable() undershooting with CursorMaxPos report. (#7543)
    ImRect group_bb(group_data.BackupCursorPos, ImMax(ImMax(window->DC.CursorMaxPos, g.LastItemData.Rect.Max), group_data.BackupCursorPos));
    window->DC.CursorPos = group_data.BackupCursorPos;
    window->DC.CursorPosPrevLine = group_data.BackupCursorPosPrevLine;
    window->DC.CursorMaxPos = ImMax(group_data.BackupCursorMaxPos, group_bb.Max);
    window->DC.Indent = group_data.BackupIndent;
    window->DC.GroupOffset = group_data.BackupGroupOffset;
    window->DC.CurrLineSize = group_data.BackupCurrLineSize;
    window->DC.CurrLineTextBaseOffset = group_data.BackupCurrLineTextBaseOffset;
    window->DC.IsSameLine = group_data.BackupIsSameLine;
    if (g.LogEnabled)
        g.LogLinePosY = -FLT_MAX; // To enforce a carriage return

    if (!group_data.EmitItem)
    {
        g.GroupStack.pop_back();
        return;
    }

    window->DC.CurrLineTextBaseOffset = ImMax(window->DC.PrevLineTextBaseOffset, group_data.BackupCurrLineTextBaseOffset); // FIXME: Incorrect, we should grab the base offset from the *first line* of the group but it is hard to obtain now.
    ItemSize(group_bb.GetSize());
    ItemAdd(group_bb, 0, NULL, ImGuiItemFlags_NoTabStop);

    // If the current ActiveId was declared within the boundary of our group, we copy it to LastItemId so IsItemActive(), IsItemDeactivated() etc. will be functional on the entire group.
    // It would be neater if we replaced window.DC.LastItemId by e.g. 'bool LastItemIsActive', but would put a little more burden on individual widgets.
    // Also if you grep for LastItemId you'll notice it is only used in that context.
    // (The two tests not the same because ActiveIdIsAlive is an ID itself, in order to be able to handle ActiveId being overwritten during the frame.)
    const bool group_contains_curr_active_id = (group_data.BackupActiveIdIsAlive != g.ActiveId) && (g.ActiveIdIsAlive == g.ActiveId) && g.ActiveId;
    const bool group_contains_prev_active_id = (group_data.BackupActiveIdPreviousFrameIsAlive == false) && (g.ActiveIdPreviousFrameIsAlive == true);
    if (group_contains_curr_active_id)
        g.LastItemData.ID = g.ActiveId;
    else if (group_contains_prev_active_id)
        g.LastItemData.ID = g.ActiveIdPreviousFrame;
    g.LastItemData.Rect = group_bb;

    // Forward Hovered flag
    const bool group_contains_curr_hovered_id = (group_data.BackupHoveredIdIsAlive == false) && g.HoveredId != 0;
    if (group_contains_curr_hovered_id)
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;

    // Forward Edited flag
    if (group_contains_curr_active_id && g.ActiveIdHasBeenEditedThisFrame)
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_Edited;

    // Forward Deactivated flag
    g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasDeactivated;
    if (group_contains_prev_active_id && g.ActiveId != g.ActiveIdPreviousFrame)
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_Deactivated;

    g.GroupStack.pop_back();
    if (g.DebugShowGroupRects)
        window->DrawList->AddRect(group_bb.Min, group_bb.Max, IM_COL32(255,0,255,255));   // [Debug]
}


//-----------------------------------------------------------------------------
// [SECTION] SCROLLING
//-----------------------------------------------------------------------------

// Helper to snap on edges when aiming at an item very close to the edge,
// So the difference between WindowPadding and ItemSpacing will be in the visible area after scrolling.
// When we refactor the scrolling API this may be configurable with a flag?
// Note that the effect for this won't be visible on X axis with default Style settings as WindowPadding.x == ItemSpacing.x by default.
static float CalcScrollEdgeSnap(float target, float snap_min, float snap_max, float snap_threshold, float center_ratio)
{
    if (target <= snap_min + snap_threshold)
        return ImLerp(snap_min, target, center_ratio);
    if (target >= snap_max - snap_threshold)
        return ImLerp(target, snap_max, center_ratio);
    return target;
}

static ImVec2 CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window)
{
    ImVec2 scroll = window->Scroll;
    ImVec2 decoration_size(window->DecoOuterSizeX1 + window->DecoInnerSizeX1 + window->DecoOuterSizeX2, window->DecoOuterSizeY1 + window->DecoInnerSizeY1 + window->DecoOuterSizeY2);
    for (int axis = 0; axis < 2; axis++)
    {
        if (window->ScrollTarget[axis] < FLT_MAX)
        {
            float center_ratio = window->ScrollTargetCenterRatio[axis];
            float scroll_target = window->ScrollTarget[axis];
            if (window->ScrollTargetEdgeSnapDist[axis] > 0.0f)
            {
                float snap_min = 0.0f;
                float snap_max = window->ScrollMax[axis] + window->SizeFull[axis] - decoration_size[axis];
                scroll_target = CalcScrollEdgeSnap(scroll_target, snap_min, snap_max, window->ScrollTargetEdgeSnapDist[axis], center_ratio);
            }
            scroll[axis] = scroll_target - center_ratio * (window->SizeFull[axis] - decoration_size[axis]);
        }
        scroll[axis] = IM_ROUND(ImMax(scroll[axis], 0.0f));
        if (!window->Collapsed && !window->SkipItems)
            scroll[axis] = ImMin(scroll[axis], window->ScrollMax[axis]);
    }
    return scroll;
}

void ImGui::ScrollToItem(ImGuiScrollFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ScrollToRectEx(window, g.LastItemData.NavRect, flags);
}

void ImGui::ScrollToRect(ImGuiWindow* window, const ImRect& item_rect, ImGuiScrollFlags flags)
{
    ScrollToRectEx(window, item_rect, flags);
}

// Scroll to keep newly navigated item fully into view
ImVec2 ImGui::ScrollToRectEx(ImGuiWindow* window, const ImRect& item_rect, ImGuiScrollFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImRect scroll_rect(window->InnerRect.Min - ImVec2(1, 1), window->InnerRect.Max + ImVec2(1, 1));
    scroll_rect.Min.x = ImMin(scroll_rect.Min.x + window->DecoInnerSizeX1, scroll_rect.Max.x);
    scroll_rect.Min.y = ImMin(scroll_rect.Min.y + window->DecoInnerSizeY1, scroll_rect.Max.y);
    //GetForegroundDrawList(window)->AddRect(item_rect.Min, item_rect.Max, IM_COL32(255,0,0,255), 0.0f, 0, 5.0f); // [DEBUG]
    //GetForegroundDrawList(window)->AddRect(scroll_rect.Min, scroll_rect.Max, IM_COL32_WHITE); // [DEBUG]

    // Check that only one behavior is selected per axis
    IM_ASSERT((flags & ImGuiScrollFlags_MaskX_) == 0 || ImIsPowerOfTwo(flags & ImGuiScrollFlags_MaskX_));
    IM_ASSERT((flags & ImGuiScrollFlags_MaskY_) == 0 || ImIsPowerOfTwo(flags & ImGuiScrollFlags_MaskY_));

    // Defaults
    ImGuiScrollFlags in_flags = flags;
    if ((flags & ImGuiScrollFlags_MaskX_) == 0 && window->ScrollbarX)
        flags |= ImGuiScrollFlags_KeepVisibleEdgeX;
    if ((flags & ImGuiScrollFlags_MaskY_) == 0)
        flags |= window->Appearing ? ImGuiScrollFlags_AlwaysCenterY : ImGuiScrollFlags_KeepVisibleEdgeY;

    const bool fully_visible_x = item_rect.Min.x >= scroll_rect.Min.x && item_rect.Max.x <= scroll_rect.Max.x;
    const bool fully_visible_y = item_rect.Min.y >= scroll_rect.Min.y && item_rect.Max.y <= scroll_rect.Max.y;
    const bool can_be_fully_visible_x = (item_rect.GetWidth() + g.Style.ItemSpacing.x * 2.0f) <= scroll_rect.GetWidth() || (window->AutoFitFramesX > 0) || (window->Flags & ImGuiWindowFlags_AlwaysAutoResize) != 0;
    const bool can_be_fully_visible_y = (item_rect.GetHeight() + g.Style.ItemSpacing.y * 2.0f) <= scroll_rect.GetHeight() || (window->AutoFitFramesY > 0) || (window->Flags & ImGuiWindowFlags_AlwaysAutoResize) != 0;

    if ((flags & ImGuiScrollFlags_KeepVisibleEdgeX) && !fully_visible_x)
    {
        if (item_rect.Min.x < scroll_rect.Min.x || !can_be_fully_visible_x)
            SetScrollFromPosX(window, item_rect.Min.x - g.Style.ItemSpacing.x - window->Pos.x, 0.0f);
        else if (item_rect.Max.x >= scroll_rect.Max.x)
            SetScrollFromPosX(window, item_rect.Max.x + g.Style.ItemSpacing.x - window->Pos.x, 1.0f);
    }
    else if (((flags & ImGuiScrollFlags_KeepVisibleCenterX) && !fully_visible_x) || (flags & ImGuiScrollFlags_AlwaysCenterX))
    {
        if (can_be_fully_visible_x)
            SetScrollFromPosX(window, ImTrunc((item_rect.Min.x + item_rect.Max.x) * 0.5f) - window->Pos.x, 0.5f);
        else
            SetScrollFromPosX(window, item_rect.Min.x - window->Pos.x, 0.0f);
    }

    if ((flags & ImGuiScrollFlags_KeepVisibleEdgeY) && !fully_visible_y)
    {
        if (item_rect.Min.y < scroll_rect.Min.y || !can_be_fully_visible_y)
            SetScrollFromPosY(window, item_rect.Min.y - g.Style.ItemSpacing.y - window->Pos.y, 0.0f);
        else if (item_rect.Max.y >= scroll_rect.Max.y)
            SetScrollFromPosY(window, item_rect.Max.y + g.Style.ItemSpacing.y - window->Pos.y, 1.0f);
    }
    else if (((flags & ImGuiScrollFlags_KeepVisibleCenterY) && !fully_visible_y) || (flags & ImGuiScrollFlags_AlwaysCenterY))
    {
        if (can_be_fully_visible_y)
            SetScrollFromPosY(window, ImTrunc((item_rect.Min.y + item_rect.Max.y) * 0.5f) - window->Pos.y, 0.5f);
        else
            SetScrollFromPosY(window, item_rect.Min.y - window->Pos.y, 0.0f);
    }

    ImVec2 next_scroll = CalcNextScrollFromScrollTargetAndClamp(window);
    ImVec2 delta_scroll = next_scroll - window->Scroll;

    // Also scroll parent window to keep us into view if necessary
    if (!(flags & ImGuiScrollFlags_NoScrollParent) && (window->Flags & ImGuiWindowFlags_ChildWindow))
    {
        // FIXME-SCROLL: May be an option?
        if ((in_flags & (ImGuiScrollFlags_AlwaysCenterX | ImGuiScrollFlags_KeepVisibleCenterX)) != 0)
            in_flags = (in_flags & ~ImGuiScrollFlags_MaskX_) | ImGuiScrollFlags_KeepVisibleEdgeX;
        if ((in_flags & (ImGuiScrollFlags_AlwaysCenterY | ImGuiScrollFlags_KeepVisibleCenterY)) != 0)
            in_flags = (in_flags & ~ImGuiScrollFlags_MaskY_) | ImGuiScrollFlags_KeepVisibleEdgeY;
        delta_scroll += ScrollToRectEx(window->ParentWindow, ImRect(item_rect.Min - delta_scroll, item_rect.Max - delta_scroll), in_flags);
    }

    return delta_scroll;
}

float ImGui::GetScrollX()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Scroll.x;
}

float ImGui::GetScrollY()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Scroll.y;
}

float ImGui::GetScrollMaxX()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ScrollMax.x;
}

float ImGui::GetScrollMaxY()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ScrollMax.y;
}

void ImGui::SetScrollX(ImGuiWindow* window, float scroll_x)
{
    window->ScrollTarget.x = scroll_x;
    window->ScrollTargetCenterRatio.x = 0.0f;
    window->ScrollTargetEdgeSnapDist.x = 0.0f;
}

void ImGui::SetScrollY(ImGuiWindow* window, float scroll_y)
{
    window->ScrollTarget.y = scroll_y;
    window->ScrollTargetCenterRatio.y = 0.0f;
    window->ScrollTargetEdgeSnapDist.y = 0.0f;
}

void ImGui::SetScrollX(float scroll_x)
{
    ImGuiContext& g = *GImGui;
    SetScrollX(g.CurrentWindow, scroll_x);
}

void ImGui::SetScrollY(float scroll_y)
{
    ImGuiContext& g = *GImGui;
    SetScrollY(g.CurrentWindow, scroll_y);
}

// Note that a local position will vary depending on initial scroll value,
// This is a little bit confusing so bear with us:
//  - local_pos = (absolution_pos - window->Pos)
//  - So local_x/local_y are 0.0f for a position at the upper-left corner of a window,
//    and generally local_x/local_y are >(padding+decoration) && <(size-padding-decoration) when in the visible area.
//  - They mostly exist because of legacy API.
// Following the rules above, when trying to work with scrolling code, consider that:
//  - SetScrollFromPosY(0.0f) == SetScrollY(0.0f + scroll.y) == has no effect!
//  - SetScrollFromPosY(-scroll.y) == SetScrollY(-scroll.y + scroll.y) == SetScrollY(0.0f) == reset scroll. Of course writing SetScrollY(0.0f) directly then makes more sense
// We store a target position so centering and clamping can occur on the next frame when we are guaranteed to have a known window size
void ImGui::SetScrollFromPosX(ImGuiWindow* window, float local_x, float center_x_ratio)
{
    IM_ASSERT(center_x_ratio >= 0.0f && center_x_ratio <= 1.0f);
    window->ScrollTarget.x = IM_TRUNC(local_x - window->DecoOuterSizeX1 - window->DecoInnerSizeX1 + window->Scroll.x); // Convert local position to scroll offset
    window->ScrollTargetCenterRatio.x = center_x_ratio;
    window->ScrollTargetEdgeSnapDist.x = 0.0f;
}

void ImGui::SetScrollFromPosY(ImGuiWindow* window, float local_y, float center_y_ratio)
{
    IM_ASSERT(center_y_ratio >= 0.0f && center_y_ratio <= 1.0f);
    window->ScrollTarget.y = IM_TRUNC(local_y - window->DecoOuterSizeY1 - window->DecoInnerSizeY1 + window->Scroll.y); // Convert local position to scroll offset
    window->ScrollTargetCenterRatio.y = center_y_ratio;
    window->ScrollTargetEdgeSnapDist.y = 0.0f;
}

void ImGui::SetScrollFromPosX(float local_x, float center_x_ratio)
{
    ImGuiContext& g = *GImGui;
    SetScrollFromPosX(g.CurrentWindow, local_x, center_x_ratio);
}

void ImGui::SetScrollFromPosY(float local_y, float center_y_ratio)
{
    ImGuiContext& g = *GImGui;
    SetScrollFromPosY(g.CurrentWindow, local_y, center_y_ratio);
}

// center_x_ratio: 0.0f left of last item, 0.5f horizontal center of last item, 1.0f right of last item.
void ImGui::SetScrollHereX(float center_x_ratio)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    float spacing_x = ImMax(window->WindowPadding.x, g.Style.ItemSpacing.x);
    float target_pos_x = ImLerp(g.LastItemData.Rect.Min.x - spacing_x, g.LastItemData.Rect.Max.x + spacing_x, center_x_ratio);
    SetScrollFromPosX(window, target_pos_x - window->Pos.x, center_x_ratio); // Convert from absolute to local pos

    // Tweak: snap on edges when aiming at an item very close to the edge
    window->ScrollTargetEdgeSnapDist.x = ImMax(0.0f, window->WindowPadding.x - spacing_x);
}

// center_y_ratio: 0.0f top of last item, 0.5f vertical center of last item, 1.0f bottom of last item.
void ImGui::SetScrollHereY(float center_y_ratio)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    float spacing_y = ImMax(window->WindowPadding.y, g.Style.ItemSpacing.y);
    float target_pos_y = ImLerp(window->DC.CursorPosPrevLine.y - spacing_y, window->DC.CursorPosPrevLine.y + window->DC.PrevLineSize.y + spacing_y, center_y_ratio);
    SetScrollFromPosY(window, target_pos_y - window->Pos.y, center_y_ratio); // Convert from absolute to local pos

    // Tweak: snap on edges when aiming at an item very close to the edge
    window->ScrollTargetEdgeSnapDist.y = ImMax(0.0f, window->WindowPadding.y - spacing_y);
}

//-----------------------------------------------------------------------------
// [SECTION] TOOLTIPS
//-----------------------------------------------------------------------------

bool ImGui::BeginTooltip()
{
    return BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);
}

bool ImGui::BeginItemTooltip()
{
    if (!IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        return false;
    return BeginTooltipEx(ImGuiTooltipFlags_None, ImGuiWindowFlags_None);
}

bool ImGui::BeginTooltipEx(ImGuiTooltipFlags tooltip_flags, ImGuiWindowFlags extra_window_flags)
{
    ImGuiContext& g = *GImGui;

    const bool is_dragdrop_tooltip = g.DragDropWithinSource || g.DragDropWithinTarget;
    if (is_dragdrop_tooltip)
    {
        // Drag and Drop tooltips are positioning differently than other tooltips:
        // - offset visibility to increase visibility around mouse.
        // - never clamp within outer viewport boundary.
        // We call SetNextWindowPos() to enforce position and disable clamping.
        // See FindBestWindowPosForPopup() for positionning logic of other tooltips (not drag and drop ones).
        //ImVec2 tooltip_pos = g.IO.MousePos - g.ActiveIdClickOffset - g.Style.WindowPadding;
        const bool is_touchscreen = (g.IO.MouseSource == ImGuiMouseSource_TouchScreen);
        if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos) == 0)
        {
            ImVec2 tooltip_pos = is_touchscreen ? (g.IO.MousePos + TOOLTIP_DEFAULT_OFFSET_TOUCH * g.Style.MouseCursorScale) : (g.IO.MousePos + TOOLTIP_DEFAULT_OFFSET_MOUSE * g.Style.MouseCursorScale);
            ImVec2 tooltip_pivot = is_touchscreen ? TOOLTIP_DEFAULT_PIVOT_TOUCH : ImVec2(0.0f, 0.0f);
            SetNextWindowPos(tooltip_pos, ImGuiCond_None, tooltip_pivot);
        }

        SetNextWindowBgAlpha(g.Style.Colors[ImGuiCol_PopupBg].w * 0.60f);
        //PushStyleVar(ImGuiStyleVar_Alpha, g.Style.Alpha * 0.60f); // This would be nice but e.g ColorButton with checkboard has issue with transparent colors :(
        tooltip_flags |= ImGuiTooltipFlags_OverridePrevious;
    }

    const char* window_name_template = is_dragdrop_tooltip ? "##Tooltip_DragDrop_%02d" : "##Tooltip_%02d";
    char window_name[32];
    ImFormatString(window_name, IM_ARRAYSIZE(window_name), window_name_template, g.TooltipOverrideCount);
    if ((tooltip_flags & ImGuiTooltipFlags_OverridePrevious) && g.TooltipPreviousWindow != NULL && g.TooltipPreviousWindow->Active)
    {
        // Hide previous tooltip from being displayed. We can't easily "reset" the content of a window so we create a new one.
        //IMGUI_DEBUG_LOG("[tooltip] '%s' already active, using +1 for this frame\n", window_name);
        SetWindowHiddenAndSkipItemsForCurrentFrame(g.TooltipPreviousWindow);
        ImFormatString(window_name, IM_ARRAYSIZE(window_name), window_name_template, ++g.TooltipOverrideCount);
    }
    ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
    Begin(window_name, NULL, flags | extra_window_flags);
    // 2023-03-09: Added bool return value to the API, but currently always returning true.
    // If this ever returns false we need to update BeginDragDropSource() accordingly.
    //if (!ret)
    //    End();
    //return ret;
    return true;
}

void ImGui::EndTooltip()
{
    IM_ASSERT(GetCurrentWindowRead()->Flags & ImGuiWindowFlags_Tooltip);   // Mismatched BeginTooltip()/EndTooltip() calls
    End();
}

void ImGui::SetTooltip(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SetTooltipV(fmt, args);
    va_end(args);
}

void ImGui::SetTooltipV(const char* fmt, va_list args)
{
    if (!BeginTooltipEx(ImGuiTooltipFlags_OverridePrevious, ImGuiWindowFlags_None))
        return;
    TextV(fmt, args);
    EndTooltip();
}

// Shortcut to use 'style.HoverFlagsForTooltipMouse' or 'style.HoverFlagsForTooltipNav'.
// Defaults to == ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayShort when using the mouse.
void ImGui::SetItemTooltip(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        SetTooltipV(fmt, args);
    va_end(args);
}

void ImGui::SetItemTooltipV(const char* fmt, va_list args)
{
    if (IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        SetTooltipV(fmt, args);
}


//-----------------------------------------------------------------------------
// [SECTION] POPUPS
//-----------------------------------------------------------------------------

// Supported flags: ImGuiPopupFlags_AnyPopupId, ImGuiPopupFlags_AnyPopupLevel
bool ImGui::IsPopupOpen(ImGuiID id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    if (popup_flags & ImGuiPopupFlags_AnyPopupId)
    {
        // Return true if any popup is open at the current BeginPopup() level of the popup stack
        // This may be used to e.g. test for another popups already opened to handle popups priorities at the same level.
        IM_ASSERT(id == 0);
        if (popup_flags & ImGuiPopupFlags_AnyPopupLevel)
            return g.OpenPopupStack.Size > 0;
        else
            return g.OpenPopupStack.Size > g.BeginPopupStack.Size;
    }
    else
    {
        if (popup_flags & ImGuiPopupFlags_AnyPopupLevel)
        {
            // Return true if the popup is open anywhere in the popup stack
            for (int n = 0; n < g.OpenPopupStack.Size; n++)
                if (g.OpenPopupStack[n].PopupId == id)
                    return true;
            return false;
        }
        else
        {
            // Return true if the popup is open at the current BeginPopup() level of the popup stack (this is the most-common query)
            return g.OpenPopupStack.Size > g.BeginPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].PopupId == id;
        }
    }
}

bool ImGui::IsPopupOpen(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiID id = (popup_flags & ImGuiPopupFlags_AnyPopupId) ? 0 : g.CurrentWindow->GetID(str_id);
    if ((popup_flags & ImGuiPopupFlags_AnyPopupLevel) && id != 0)
        IM_ASSERT(0 && "Cannot use IsPopupOpen() with a string id and ImGuiPopupFlags_AnyPopupLevel."); // But non-string version is legal and used internally
    return IsPopupOpen(id, popup_flags);
}

// Also see FindBlockingModal(NULL)
ImGuiWindow* ImGui::GetTopMostPopupModal()
{
    ImGuiContext& g = *GImGui;
    for (int n = g.OpenPopupStack.Size - 1; n >= 0; n--)
        if (ImGuiWindow* popup = g.OpenPopupStack.Data[n].Window)
            if (popup->Flags & ImGuiWindowFlags_Modal)
                return popup;
    return NULL;
}

// See Demo->Stacked Modal to confirm what this is for.
ImGuiWindow* ImGui::GetTopMostAndVisiblePopupModal()
{
    ImGuiContext& g = *GImGui;
    for (int n = g.OpenPopupStack.Size - 1; n >= 0; n--)
        if (ImGuiWindow* popup = g.OpenPopupStack.Data[n].Window)
            if ((popup->Flags & ImGuiWindowFlags_Modal) && IsWindowActiveAndVisible(popup))
                return popup;
    return NULL;
}

void ImGui::OpenPopup(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiID id = g.CurrentWindow->GetID(str_id);
    IMGUI_DEBUG_LOG_POPUP("[popup] OpenPopup(\"%s\" -> 0x%08X)\n", str_id, id);
    OpenPopupEx(id, popup_flags);
}

void ImGui::OpenPopup(ImGuiID id, ImGuiPopupFlags popup_flags)
{
    OpenPopupEx(id, popup_flags);
}

// Mark popup as open (toggle toward open state).
// Popups are closed when user click outside, or activate a pressable item, or CloseCurrentPopup() is called within a BeginPopup()/EndPopup() block.
// Popup identifiers are relative to the current ID-stack (so OpenPopup and BeginPopup needs to be at the same level).
// One open popup per level of the popup hierarchy (NB: when assigning we reset the Window member of ImGuiPopupRef to NULL)
void ImGui::OpenPopupEx(ImGuiID id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;
    const int current_stack_size = g.BeginPopupStack.Size;

    if (popup_flags & ImGuiPopupFlags_NoOpenOverExistingPopup)
        if (IsPopupOpen((ImGuiID)0, ImGuiPopupFlags_AnyPopupId))
            return;

    ImGuiPopupData popup_ref; // Tagged as new ref as Window will be set back to NULL if we write this into OpenPopupStack.
    popup_ref.PopupId = id;
    popup_ref.Window = NULL;
    popup_ref.RestoreNavWindow = g.NavWindow;           // When popup closes focus may be restored to NavWindow (depend on window type).
    popup_ref.OpenFrameCount = g.FrameCount;
    popup_ref.OpenParentId = parent_window->IDStack.back();
    popup_ref.OpenPopupPos = NavCalcPreferredRefPos();
    popup_ref.OpenMousePos = IsMousePosValid(&g.IO.MousePos) ? g.IO.MousePos : popup_ref.OpenPopupPos;

    IMGUI_DEBUG_LOG_POPUP("[popup] OpenPopupEx(0x%08X)\n", id);
    if (g.OpenPopupStack.Size < current_stack_size + 1)
    {
        g.OpenPopupStack.push_back(popup_ref);
    }
    else
    {
        // Gently handle the user mistakenly calling OpenPopup() every frames: it is likely a programming mistake!
        // However, if we were to run the regular code path, the ui would become completely unusable because the popup will always be
        // in hidden-while-calculating-size state _while_ claiming focus. Which is extremely confusing situation for the programmer.
        // Instead, for successive frames calls to OpenPopup(), we silently avoid reopening even if ImGuiPopupFlags_NoReopen is not specified.
        bool keep_existing = false;
        if (g.OpenPopupStack[current_stack_size].PopupId == id)
            if ((g.OpenPopupStack[current_stack_size].OpenFrameCount == g.FrameCount - 1) || (popup_flags & ImGuiPopupFlags_NoReopen))
                keep_existing = true;
        if (keep_existing)
        {
            // No reopen
            g.OpenPopupStack[current_stack_size].OpenFrameCount = popup_ref.OpenFrameCount;
        }
        else
        {
            // Reopen: close child popups if any, then flag popup for open/reopen (set position, focus, init navigation)
            ClosePopupToLevel(current_stack_size, true);
            g.OpenPopupStack.push_back(popup_ref);
        }

        // When reopening a popup we first refocus its parent, otherwise if its parent is itself a popup it would get closed by ClosePopupsOverWindow().
        // This is equivalent to what ClosePopupToLevel() does.
        //if (g.OpenPopupStack[current_stack_size].PopupId == id)
        //    FocusWindow(parent_window);
    }
}

// When popups are stacked, clicking on a lower level popups puts focus back to it and close popups above it.
// This function closes any popups that are over 'ref_window'.
void ImGui::ClosePopupsOverWindow(ImGuiWindow* ref_window, bool restore_focus_to_window_under_popup)
{
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.Size == 0)
        return;

    // Don't close our own child popup windows.
    //IMGUI_DEBUG_LOG_POPUP("[popup] ClosePopupsOverWindow(\"%s\") restore_under=%d\n", ref_window ? ref_window->Name : "<NULL>", restore_focus_to_window_under_popup);
    int popup_count_to_keep = 0;
    if (ref_window)
    {
        // Find the highest popup which is a descendant of the reference window (generally reference window = NavWindow)
        for (; popup_count_to_keep < g.OpenPopupStack.Size; popup_count_to_keep++)
        {
            ImGuiPopupData& popup = g.OpenPopupStack[popup_count_to_keep];
            if (!popup.Window)
                continue;
            IM_ASSERT((popup.Window->Flags & ImGuiWindowFlags_Popup) != 0);

            // Trim the stack unless the popup is a direct parent of the reference window (the reference window is often the NavWindow)
            // - Clicking/Focusing Window2 won't close Popup1:
            //     Window -> Popup1 -> Window2(Ref)
            // - Clicking/focusing Popup1 will close Popup2 and Popup3:
            //     Window -> Popup1(Ref) -> Popup2 -> Popup3
            // - Each popups may contain child windows, which is why we compare ->RootWindow!
            //     Window -> Popup1 -> Popup1_Child -> Popup2 -> Popup2_Child
            // We step through every popup from bottom to top to validate their position relative to reference window.
            bool ref_window_is_descendent_of_popup = false;
            for (int n = popup_count_to_keep; n < g.OpenPopupStack.Size; n++)
                if (ImGuiWindow* popup_window = g.OpenPopupStack[n].Window)
                    if (IsWindowWithinBeginStackOf(ref_window, popup_window))
                    {
                        ref_window_is_descendent_of_popup = true;
                        break;
                    }
            if (!ref_window_is_descendent_of_popup)
                break;
        }
    }
    if (popup_count_to_keep < g.OpenPopupStack.Size) // This test is not required but it allows to set a convenient breakpoint on the statement below
    {
        IMGUI_DEBUG_LOG_POPUP("[popup] ClosePopupsOverWindow(\"%s\")\n", ref_window ? ref_window->Name : "<NULL>");
        ClosePopupToLevel(popup_count_to_keep, restore_focus_to_window_under_popup);
    }
}

void ImGui::ClosePopupsExceptModals()
{
    ImGuiContext& g = *GImGui;

    int popup_count_to_keep;
    for (popup_count_to_keep = g.OpenPopupStack.Size; popup_count_to_keep > 0; popup_count_to_keep--)
    {
        ImGuiWindow* window = g.OpenPopupStack[popup_count_to_keep - 1].Window;
        if (!window || (window->Flags & ImGuiWindowFlags_Modal))
            break;
    }
    if (popup_count_to_keep < g.OpenPopupStack.Size) // This test is not required but it allows to set a convenient breakpoint on the statement below
        ClosePopupToLevel(popup_count_to_keep, true);
}

void ImGui::ClosePopupToLevel(int remaining, bool restore_focus_to_window_under_popup)
{
    ImGuiContext& g = *GImGui;
    IMGUI_DEBUG_LOG_POPUP("[popup] ClosePopupToLevel(%d), restore_under=%d\n", remaining, restore_focus_to_window_under_popup);
    IM_ASSERT(remaining >= 0 && remaining < g.OpenPopupStack.Size);
    if (g.DebugLogFlags & ImGuiDebugLogFlags_EventPopup)
        for (int n = remaining; n < g.OpenPopupStack.Size; n++)
            IMGUI_DEBUG_LOG_POPUP("[popup] - Closing PopupID 0x%08X Window \"%s\"\n", g.OpenPopupStack[n].PopupId, g.OpenPopupStack[n].Window ? g.OpenPopupStack[n].Window->Name : NULL);

    // Trim open popup stack
    ImGuiPopupData prev_popup = g.OpenPopupStack[remaining];
    g.OpenPopupStack.resize(remaining);

    // Restore focus (unless popup window was not yet submitted, and didn't have a chance to take focus anyhow. See #7325 for an edge case)
    if (restore_focus_to_window_under_popup && prev_popup.Window)
    {
        ImGuiWindow* popup_window = prev_popup.Window;
        ImGuiWindow* focus_window = (popup_window->Flags & ImGuiWindowFlags_ChildMenu) ? popup_window->ParentWindow : prev_popup.RestoreNavWindow;
        if (focus_window && !focus_window->WasActive)
            FocusTopMostWindowUnderOne(popup_window, NULL, NULL, ImGuiFocusRequestFlags_RestoreFocusedChild); // Fallback
        else
            FocusWindow(focus_window, (g.NavLayer == ImGuiNavLayer_Main) ? ImGuiFocusRequestFlags_RestoreFocusedChild : ImGuiFocusRequestFlags_None);
    }
}

// Close the popup we have begin-ed into.
void ImGui::CloseCurrentPopup()
{
    ImGuiContext& g = *GImGui;
    int popup_idx = g.BeginPopupStack.Size - 1;
    if (popup_idx < 0 || popup_idx >= g.OpenPopupStack.Size || g.BeginPopupStack[popup_idx].PopupId != g.OpenPopupStack[popup_idx].PopupId)
        return;

    // Closing a menu closes its top-most parent popup (unless a modal)
    while (popup_idx > 0)
    {
        ImGuiWindow* popup_window = g.OpenPopupStack[popup_idx].Window;
        ImGuiWindow* parent_popup_window = g.OpenPopupStack[popup_idx - 1].Window;
        bool close_parent = false;
        if (popup_window && (popup_window->Flags & ImGuiWindowFlags_ChildMenu))
            if (parent_popup_window && !(parent_popup_window->Flags & ImGuiWindowFlags_MenuBar))
                close_parent = true;
        if (!close_parent)
            break;
        popup_idx--;
    }
    IMGUI_DEBUG_LOG_POPUP("[popup] CloseCurrentPopup %d -> %d\n", g.BeginPopupStack.Size - 1, popup_idx);
    ClosePopupToLevel(popup_idx, true);

    // A common pattern is to close a popup when selecting a menu item/selectable that will open another window.
    // To improve this usage pattern, we avoid nav highlight for a single frame in the parent window.
    // Similarly, we could avoid mouse hover highlight in this window but it is less visually problematic.
    if (ImGuiWindow* window = g.NavWindow)
        window->DC.NavHideHighlightOneFrame = true;
}

// Attention! BeginPopup() adds default flags when calling BeginPopupEx()!
bool ImGui::BeginPopupEx(ImGuiID id, ImGuiWindowFlags extra_window_flags)
{
    ImGuiContext& g = *GImGui;
    if (!IsPopupOpen(id, ImGuiPopupFlags_None))
    {
        g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
        return false;
    }

    char name[20];
    if (extra_window_flags & ImGuiWindowFlags_ChildMenu)
        ImFormatString(name, IM_ARRAYSIZE(name), "##Menu_%02d", g.BeginMenuDepth); // Recycle windows based on depth
    else
        ImFormatString(name, IM_ARRAYSIZE(name), "##Popup_%08x", id); // Not recycling, so we can close/open during the same frame

    bool is_open = Begin(name, NULL, extra_window_flags | ImGuiWindowFlags_Popup);
    if (!is_open) // NB: Begin can return false when the popup is completely clipped (e.g. zero size display)
        EndPopup();

    //g.CurrentWindow->FocusRouteParentWindow = g.CurrentWindow->ParentWindowInBeginStack;

    return is_open;
}

bool ImGui::BeginPopup(const char* str_id, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.Size <= g.BeginPopupStack.Size) // Early out for performance
    {
        g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
        return false;
    }
    flags |= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
    ImGuiID id = g.CurrentWindow->GetID(str_id);
    return BeginPopupEx(id, flags);
}

// If 'p_open' is specified for a modal popup window, the popup will have a regular close button which will close the popup.
// Note that popup visibility status is owned by Dear ImGui (and manipulated with e.g. OpenPopup).
// - *p_open set back to false in BeginPopupModal() when popup is not open.
// - if you set *p_open to false before calling BeginPopupModal(), it will close the popup.
bool ImGui::BeginPopupModal(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const ImGuiID id = window->GetID(name);
    if (!IsPopupOpen(id, ImGuiPopupFlags_None))
    {
        g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
        if (p_open && *p_open)
            *p_open = false;
        return false;
    }

    // Center modal windows by default for increased visibility
    // (this won't really last as settings will kick in, and is mostly for backward compatibility. user may do the same themselves)
    // FIXME: Should test for (PosCond & window->SetWindowPosAllowFlags) with the upcoming window.
    if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos) == 0)
    {
        const ImGuiViewport* viewport = GetMainViewport();
        SetNextWindowPos(viewport->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    }

    flags |= ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoCollapse;
    const bool is_open = Begin(name, p_open, flags);
    if (!is_open || (p_open && !*p_open)) // NB: is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
    {
        EndPopup();
        if (is_open)
            ClosePopupToLevel(g.BeginPopupStack.Size, true);
        return false;
    }
    return is_open;
}

void ImGui::EndPopup()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(window->Flags & ImGuiWindowFlags_Popup);  // Mismatched BeginPopup()/EndPopup() calls
    IM_ASSERT(g.BeginPopupStack.Size > 0);

    // Make all menus and popups wrap around for now, may need to expose that policy (e.g. focus scope could include wrap/loop policy flags used by new move requests)
    if (g.NavWindow == window)
        NavMoveRequestTryWrapping(window, ImGuiNavMoveFlags_LoopY);

    // Child-popups don't need to be laid out
    IM_ASSERT(g.WithinEndChild == false);
    if (window->Flags & ImGuiWindowFlags_ChildWindow)
        g.WithinEndChild = true;
    End();
    g.WithinEndChild = false;
}

// Helper to open a popup if mouse button is released over the item
// - This is essentially the same as BeginPopupContextItem() but without the trailing BeginPopup()
void ImGui::OpenPopupOnItemClick(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
    if (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
    {
        ImGuiID id = str_id ? window->GetID(str_id) : g.LastItemData.ID;    // If user hasn't passed an ID, we can use the LastItemID. Using LastItemID as a Popup ID won't conflict!
        IM_ASSERT(id != 0);                                             // You cannot pass a NULL str_id if the last item has no identifier (e.g. a Text() item)
        OpenPopupEx(id, popup_flags);
    }
}

// This is a helper to handle the simplest case of associating one named popup to one given widget.
// - To create a popup associated to the last item, you generally want to pass a NULL value to str_id.
// - To create a popup with a specific identifier, pass it in str_id.
//    - This is useful when using using BeginPopupContextItem() on an item which doesn't have an identifier, e.g. a Text() call.
//    - This is useful when multiple code locations may want to manipulate/open the same popup, given an explicit id.
// - You may want to handle the whole on user side if you have specific needs (e.g. tweaking IsItemHovered() parameters).
//   This is essentially the same as:
//       id = str_id ? GetID(str_id) : GetItemID();
//       OpenPopupOnItemClick(str_id, ImGuiPopupFlags_MouseButtonRight);
//       return BeginPopup(id);
//   Which is essentially the same as:
//       id = str_id ? GetID(str_id) : GetItemID();
//       if (IsItemHovered() && IsMouseReleased(ImGuiMouseButton_Right))
//           OpenPopup(id);
//       return BeginPopup(id);
//   The main difference being that this is tweaked to avoid computing the ID twice.
bool ImGui::BeginPopupContextItem(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;
    ImGuiID id = str_id ? window->GetID(str_id) : g.LastItemData.ID;    // If user hasn't passed an ID, we can use the LastItemID. Using LastItemID as a Popup ID won't conflict!
    IM_ASSERT(id != 0);                                             // You cannot pass a NULL str_id if the last item has no identifier (e.g. a Text() item)
    int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
    if (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
        OpenPopupEx(id, popup_flags);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
}

bool ImGui::BeginPopupContextWindow(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!str_id)
        str_id = "window_context";
    ImGuiID id = window->GetID(str_id);
    int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
    if (IsMouseReleased(mouse_button) && IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
        if (!(popup_flags & ImGuiPopupFlags_NoOpenOverItems) || !IsAnyItemHovered())
            OpenPopupEx(id, popup_flags);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
}

bool ImGui::BeginPopupContextVoid(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!str_id)
        str_id = "void_context";
    ImGuiID id = window->GetID(str_id);
    int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
    if (IsMouseReleased(mouse_button) && !IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        if (GetTopMostPopupModal() == NULL)
            OpenPopupEx(id, popup_flags);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
}

// r_avoid = the rectangle to avoid (e.g. for tooltip it is a rectangle around the mouse cursor which we want to avoid. for popups it's a small point around the cursor.)
// r_outer = the visible area rectangle, minus safe area padding. If our popup size won't fit because of safe area padding we ignore it.
// (r_outer is usually equivalent to the viewport rectangle minus padding, but when multi-viewports are enabled and monitor
//  information are available, it may represent the entire platform monitor from the frame of reference of the current viewport.
//  this allows us to have tooltips/popups displayed out of the parent viewport.)
ImVec2 ImGui::FindBestWindowPosForPopupEx(const ImVec2& ref_pos, const ImVec2& size, ImGuiDir* last_dir, const ImRect& r_outer, const ImRect& r_avoid, ImGuiPopupPositionPolicy policy)
{
    ImVec2 base_pos_clamped = ImClamp(ref_pos, r_outer.Min, r_outer.Max - size);
    //GetForegroundDrawList()->AddRect(r_avoid.Min, r_avoid.Max, IM_COL32(255,0,0,255));
    //GetForegroundDrawList()->AddRect(r_outer.Min, r_outer.Max, IM_COL32(0,255,0,255));

    // Combo Box policy (we want a connecting edge)
    if (policy == ImGuiPopupPositionPolicy_ComboBox)
    {
        const ImGuiDir dir_prefered_order[ImGuiDir_COUNT] = { ImGuiDir_Down, ImGuiDir_Right, ImGuiDir_Left, ImGuiDir_Up };
        for (int n = (*last_dir != ImGuiDir_None) ? -1 : 0; n < ImGuiDir_COUNT; n++)
        {
            const ImGuiDir dir = (n == -1) ? *last_dir : dir_prefered_order[n];
            if (n != -1 && dir == *last_dir) // Already tried this direction?
                continue;
            ImVec2 pos;
            if (dir == ImGuiDir_Down)  pos = ImVec2(r_avoid.Min.x, r_avoid.Max.y);          // Below, Toward Right (default)
            if (dir == ImGuiDir_Right) pos = ImVec2(r_avoid.Min.x, r_avoid.Min.y - size.y); // Above, Toward Right
            if (dir == ImGuiDir_Left)  pos = ImVec2(r_avoid.Max.x - size.x, r_avoid.Max.y); // Below, Toward Left
            if (dir == ImGuiDir_Up)    pos = ImVec2(r_avoid.Max.x - size.x, r_avoid.Min.y - size.y); // Above, Toward Left
            if (!r_outer.Contains(ImRect(pos, pos + size)))
                continue;
            *last_dir = dir;
            return pos;
        }
    }

    // Tooltip and Default popup policy
    // (Always first try the direction we used on the last frame, if any)
    if (policy == ImGuiPopupPositionPolicy_Tooltip || policy == ImGuiPopupPositionPolicy_Default)
    {
        const ImGuiDir dir_prefered_order[ImGuiDir_COUNT] = { ImGuiDir_Right, ImGuiDir_Down, ImGuiDir_Up, ImGuiDir_Left };
        for (int n = (*last_dir != ImGuiDir_None) ? -1 : 0; n < ImGuiDir_COUNT; n++)
        {
            const ImGuiDir dir = (n == -1) ? *last_dir : dir_prefered_order[n];
            if (n != -1 && dir == *last_dir) // Already tried this direction?
                continue;

            const float avail_w = (dir == ImGuiDir_Left ? r_avoid.Min.x : r_outer.Max.x) - (dir == ImGuiDir_Right ? r_avoid.Max.x : r_outer.Min.x);
            const float avail_h = (dir == ImGuiDir_Up ? r_avoid.Min.y : r_outer.Max.y) - (dir == ImGuiDir_Down ? r_avoid.Max.y : r_outer.Min.y);

            // If there's not enough room on one axis, there's no point in positioning on a side on this axis (e.g. when not enough width, use a top/bottom position to maximize available width)
            if (avail_w < size.x && (dir == ImGuiDir_Left || dir == ImGuiDir_Right))
                continue;
            if (avail_h < size.y && (dir == ImGuiDir_Up || dir == ImGuiDir_Down))
                continue;

            ImVec2 pos;
            pos.x = (dir == ImGuiDir_Left) ? r_avoid.Min.x - size.x : (dir == ImGuiDir_Right) ? r_avoid.Max.x : base_pos_clamped.x;
            pos.y = (dir == ImGuiDir_Up) ? r_avoid.Min.y - size.y : (dir == ImGuiDir_Down) ? r_avoid.Max.y : base_pos_clamped.y;

            // Clamp top-left corner of popup
            pos.x = ImMax(pos.x, r_outer.Min.x);
            pos.y = ImMax(pos.y, r_outer.Min.y);

            *last_dir = dir;
            return pos;
        }
    }

    // Fallback when not enough room:
    *last_dir = ImGuiDir_None;

    // For tooltip we prefer avoiding the cursor at all cost even if it means that part of the tooltip won't be visible.
    if (policy == ImGuiPopupPositionPolicy_Tooltip)
        return ref_pos + ImVec2(2, 2);

    // Otherwise try to keep within display
    ImVec2 pos = ref_pos;
    pos.x = ImMax(ImMin(pos.x + size.x, r_outer.Max.x) - size.x, r_outer.Min.x);
    pos.y = ImMax(ImMin(pos.y + size.y, r_outer.Max.y) - size.y, r_outer.Min.y);
    return pos;
}

// Note that this is used for popups, which can overlap the non work-area of individual viewports.
ImRect ImGui::GetPopupAllowedExtentRect(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_UNUSED(window);
    ImRect r_screen = ((ImGuiViewportP*)(void*)GetMainViewport())->GetMainRect();
    ImVec2 padding = g.Style.DisplaySafeAreaPadding;
    r_screen.Expand(ImVec2((r_screen.GetWidth() > padding.x * 2) ? -padding.x : 0.0f, (r_screen.GetHeight() > padding.y * 2) ? -padding.y : 0.0f));
    return r_screen;
}

ImVec2 ImGui::FindBestWindowPosForPopup(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;

    ImRect r_outer = GetPopupAllowedExtentRect(window);
    if (window->Flags & ImGuiWindowFlags_ChildMenu)
    {
        // Child menus typically request _any_ position within the parent menu item, and then we move the new menu outside the parent bounds.
        // This is how we end up with child menus appearing (most-commonly) on the right of the parent menu.
        IM_ASSERT(g.CurrentWindow == window);
        ImGuiWindow* parent_window = g.CurrentWindowStack[g.CurrentWindowStack.Size - 2].Window;
        float horizontal_overlap = g.Style.ItemInnerSpacing.x; // We want some overlap to convey the relative depth of each menu (currently the amount of overlap is hard-coded to style.ItemSpacing.x).
        ImRect r_avoid;
        if (parent_window->DC.MenuBarAppending)
            r_avoid = ImRect(-FLT_MAX, parent_window->ClipRect.Min.y, FLT_MAX, parent_window->ClipRect.Max.y); // Avoid parent menu-bar. If we wanted multi-line menu-bar, we may instead want to have the calling window setup e.g. a NextWindowData.PosConstraintAvoidRect field
        else
            r_avoid = ImRect(parent_window->Pos.x + horizontal_overlap, -FLT_MAX, parent_window->Pos.x + parent_window->Size.x - horizontal_overlap - parent_window->ScrollbarSizes.x, FLT_MAX);
        return FindBestWindowPosForPopupEx(window->Pos, window->Size, &window->AutoPosLastDirection, r_outer, r_avoid, ImGuiPopupPositionPolicy_Default);
    }
    if (window->Flags & ImGuiWindowFlags_Popup)
    {
        return FindBestWindowPosForPopupEx(window->Pos, window->Size, &window->AutoPosLastDirection, r_outer, ImRect(window->Pos, window->Pos), ImGuiPopupPositionPolicy_Default); // Ideally we'd disable r_avoid here
    }
    if (window->Flags & ImGuiWindowFlags_Tooltip)
    {
        // Position tooltip (always follows mouse + clamp within outer boundaries)
        // FIXME:
        // - Too many paths. One problem is that FindBestWindowPosForPopupEx() doesn't allow passing a suggested position (so touch screen path doesn't use it by default).
        // - Drag and drop tooltips are not using this path either: BeginTooltipEx() manually sets their position.
        // - Require some tidying up. In theory we could handle both cases in same location, but requires a bit of shuffling
        //   as drag and drop tooltips are calling SetNextWindowPos() leading to 'window_pos_set_by_api' being set in Begin().
        IM_ASSERT(g.CurrentWindow == window);
        const float scale = g.Style.MouseCursorScale;
        const ImVec2 ref_pos = NavCalcPreferredRefPos();

        if (g.IO.MouseSource == ImGuiMouseSource_TouchScreen)
        {
            ImVec2 tooltip_pos = g.IO.MousePos + TOOLTIP_DEFAULT_OFFSET_TOUCH * scale - (TOOLTIP_DEFAULT_PIVOT_TOUCH * window->Size);
            if (r_outer.Contains(ImRect(tooltip_pos, tooltip_pos + window->Size)))
                return tooltip_pos;
        }

        ImVec2 tooltip_pos = g.IO.MousePos + TOOLTIP_DEFAULT_OFFSET_MOUSE * scale;
        ImRect r_avoid;
        if (!g.NavDisableHighlight && g.NavDisableMouseHover && !g.IO.ConfigNavMoveSetMousePos)
            r_avoid = ImRect(ref_pos.x - 16, ref_pos.y - 8, ref_pos.x + 16, ref_pos.y + 8);
        else
            r_avoid = ImRect(ref_pos.x - 16, ref_pos.y - 8, ref_pos.x + 24 * scale, ref_pos.y + 24 * scale); // FIXME: Hard-coded based on mouse cursor shape expectation. Exact dimension not very important.
        //GetForegroundDrawList()->AddRect(r_avoid.Min, r_avoid.Max, IM_COL32(255, 0, 255, 255));

        return FindBestWindowPosForPopupEx(tooltip_pos, window->Size, &window->AutoPosLastDirection, r_outer, r_avoid, ImGuiPopupPositionPolicy_Tooltip);
    }
    IM_ASSERT(0);
    return window->Pos;
}

//-----------------------------------------------------------------------------
// [SECTION] KEYBOARD/GAMEPAD NAVIGATION
//-----------------------------------------------------------------------------

// FIXME-NAV: The existence of SetNavID vs SetFocusID vs FocusWindow() needs to be clarified/reworked.
// In our terminology those should be interchangeable, yet right now this is super confusing.
// Those two functions are merely a legacy artifact, so at minimum naming should be clarified.

void ImGui::SetNavWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.NavWindow != window)
    {
        IMGUI_DEBUG_LOG_FOCUS("[focus] SetNavWindow(\"%s\")\n", window ? window->Name : "<NULL>");
        g.NavWindow = window;
        g.NavLastValidSelectionUserData = ImGuiSelectionUserData_Invalid;
    }
    g.NavInitRequest = g.NavMoveSubmitted = g.NavMoveScoringItems = false;
    NavUpdateAnyRequestFlag();
}

void ImGui::NavHighlightActivated(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    g.NavHighlightActivatedId = id;
    g.NavHighlightActivatedTimer = NAV_ACTIVATE_HIGHLIGHT_TIMER;
}

void ImGui::NavClearPreferredPosForAxis(ImGuiAxis axis)
{
    ImGuiContext& g = *GImGui;
    g.NavWindow->RootWindowForNav->NavPreferredScoringPosRel[g.NavLayer][axis] = FLT_MAX;
}

void ImGui::SetNavID(ImGuiID id, ImGuiNavLayer nav_layer, ImGuiID focus_scope_id, const ImRect& rect_rel)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindow != NULL);
    IM_ASSERT(nav_layer == ImGuiNavLayer_Main || nav_layer == ImGuiNavLayer_Menu);
    g.NavId = id;
    g.NavLayer = nav_layer;
    SetNavFocusScope(focus_scope_id);
    g.NavWindow->NavLastIds[nav_layer] = id;
    g.NavWindow->NavRectRel[nav_layer] = rect_rel;

    // Clear preferred scoring position (NavMoveRequestApplyResult() will tend to restore it)
    NavClearPreferredPosForAxis(ImGuiAxis_X);
    NavClearPreferredPosForAxis(ImGuiAxis_Y);
}

void ImGui::SetFocusID(ImGuiID id, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(id != 0);

    if (g.NavWindow != window)
       SetNavWindow(window);

    // Assume that SetFocusID() is called in the context where its window->DC.NavLayerCurrent and g.CurrentFocusScopeId are valid.
    // Note that window may be != g.CurrentWindow (e.g. SetFocusID call in InputTextEx for multi-line text)
    const ImGuiNavLayer nav_layer = window->DC.NavLayerCurrent;
    g.NavId = id;
    g.NavLayer = nav_layer;
    SetNavFocusScope(g.CurrentFocusScopeId);
    window->NavLastIds[nav_layer] = id;
    if (g.LastItemData.ID == id)
        window->NavRectRel[nav_layer] = WindowRectAbsToRel(window, g.LastItemData.NavRect);

    if (g.ActiveIdSource == ImGuiInputSource_Keyboard || g.ActiveIdSource == ImGuiInputSource_Gamepad)
        g.NavDisableMouseHover = true;
    else
        g.NavDisableHighlight = true;

    // Clear preferred scoring position (NavMoveRequestApplyResult() will tend to restore it)
    NavClearPreferredPosForAxis(ImGuiAxis_X);
    NavClearPreferredPosForAxis(ImGuiAxis_Y);
}

static ImGuiDir ImGetDirQuadrantFromDelta(float dx, float dy)
{
    if (ImFabs(dx) > ImFabs(dy))
        return (dx > 0.0f) ? ImGuiDir_Right : ImGuiDir_Left;
    return (dy > 0.0f) ? ImGuiDir_Down : ImGuiDir_Up;
}

static float inline NavScoreItemDistInterval(float cand_min, float cand_max, float curr_min, float curr_max)
{
    if (cand_max < curr_min)
        return cand_max - curr_min;
    if (curr_max < cand_min)
        return cand_min - curr_max;
    return 0.0f;
}

// Scoring function for gamepad/keyboard directional navigation. Based on https://gist.github.com/rygorous/6981057
static bool ImGui::NavScoreItem(ImGuiNavItemData* result)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.NavLayer != window->DC.NavLayerCurrent)
        return false;

    // FIXME: Those are not good variables names
    ImRect cand = g.LastItemData.NavRect;   // Current item nav rectangle
    const ImRect curr = g.NavScoringRect;   // Current modified source rect (NB: we've applied Max.x = Min.x in NavUpdate() to inhibit the effect of having varied item width)
    g.NavScoringDebugCount++;

    // When entering through a NavFlattened border, we consider child window items as fully clipped for scoring
    if (window->ParentWindow == g.NavWindow)
    {
        IM_ASSERT((window->ChildFlags | g.NavWindow->ChildFlags) & ImGuiChildFlags_NavFlattened);
        if (!window->ClipRect.Overlaps(cand))
            return false;
        cand.ClipWithFull(window->ClipRect); // This allows the scored item to not overlap other candidates in the parent window
    }

    // Compute distance between boxes
    // FIXME-NAV: Introducing biases for vertical navigation, needs to be removed.
    float dbx = NavScoreItemDistInterval(cand.Min.x, cand.Max.x, curr.Min.x, curr.Max.x);
    float dby = NavScoreItemDistInterval(ImLerp(cand.Min.y, cand.Max.y, 0.2f), ImLerp(cand.Min.y, cand.Max.y, 0.8f), ImLerp(curr.Min.y, curr.Max.y, 0.2f), ImLerp(curr.Min.y, curr.Max.y, 0.8f)); // Scale down on Y to keep using box-distance for vertically touching items
    if (dby != 0.0f && dbx != 0.0f)
        dbx = (dbx / 1000.0f) + ((dbx > 0.0f) ? +1.0f : -1.0f);
    float dist_box = ImFabs(dbx) + ImFabs(dby);

    // Compute distance between centers (this is off by a factor of 2, but we only compare center distances with each other so it doesn't matter)
    float dcx = (cand.Min.x + cand.Max.x) - (curr.Min.x + curr.Max.x);
    float dcy = (cand.Min.y + cand.Max.y) - (curr.Min.y + curr.Max.y);
    float dist_center = ImFabs(dcx) + ImFabs(dcy); // L1 metric (need this for our connectedness guarantee)

    // Determine which quadrant of 'curr' our candidate item 'cand' lies in based on distance
    ImGuiDir quadrant;
    float dax = 0.0f, day = 0.0f, dist_axial = 0.0f;
    if (dbx != 0.0f || dby != 0.0f)
    {
        // For non-overlapping boxes, use distance between boxes
        // FIXME-NAV: Quadrant may be incorrect because of (1) dbx bias and (2) curr.Max.y bias applied by NavBiasScoringRect() where typically curr.Max.y==curr.Min.y
        // One typical case where this happens, with style.WindowMenuButtonPosition == ImGuiDir_Right, pressing Left to navigate from Close to Collapse tends to fail.
        // Also see #6344. Calling ImGetDirQuadrantFromDelta() with unbiased values may be good but side-effects are plenty.
        dax = dbx;
        day = dby;
        dist_axial = dist_box;
        quadrant = ImGetDirQuadrantFromDelta(dbx, dby);
    }
    else if (dcx != 0.0f || dcy != 0.0f)
    {
        // For overlapping boxes with different centers, use distance between centers
        dax = dcx;
        day = dcy;
        dist_axial = dist_center;
        quadrant = ImGetDirQuadrantFromDelta(dcx, dcy);
    }
    else
    {
        // Degenerate case: two overlapping buttons with same center, break ties arbitrarily (note that LastItemId here is really the _previous_ item order, but it doesn't matter)
        quadrant = (g.LastItemData.ID < g.NavId) ? ImGuiDir_Left : ImGuiDir_Right;
    }

    const ImGuiDir move_dir = g.NavMoveDir;
#if IMGUI_DEBUG_NAV_SCORING
    char buf[200];
    if (g.IO.KeyCtrl) // Hold CTRL to preview score in matching quadrant. CTRL+Arrow to rotate.
    {
        if (quadrant == move_dir)
        {
            ImFormatString(buf, IM_ARRAYSIZE(buf), "%.0f/%.0f", dist_box, dist_center);
            ImDrawList* draw_list = GetForegroundDrawList(window);
            draw_list->AddRectFilled(cand.Min, cand.Max, IM_COL32(255, 0, 0, 80));
            draw_list->AddRectFilled(cand.Min, cand.Min + CalcTextSize(buf), IM_COL32(255, 0, 0, 200));
            draw_list->AddText(cand.Min, IM_COL32(255, 255, 255, 255), buf);
        }
    }
    const bool debug_hovering = IsMouseHoveringRect(cand.Min, cand.Max);
    const bool debug_tty = (g.IO.KeyCtrl && IsKeyPressed(ImGuiKey_Space));
    if (debug_hovering || debug_tty)
    {
        ImFormatString(buf, IM_ARRAYSIZE(buf),
            "d-box    (%7.3f,%7.3f) -> %7.3f\nd-center (%7.3f,%7.3f) -> %7.3f\nd-axial  (%7.3f,%7.3f) -> %7.3f\nnav %c, quadrant %c",
            dbx, dby, dist_box, dcx, dcy, dist_center, dax, day, dist_axial, "-WENS"[move_dir+1], "-WENS"[quadrant+1]);
        if (debug_hovering)
        {
            ImDrawList* draw_list = GetForegroundDrawList(window);
            draw_list->AddRect(curr.Min, curr.Max, IM_COL32(255, 200, 0, 100));
            draw_list->AddRect(cand.Min, cand.Max, IM_COL32(255, 255, 0, 200));
            draw_list->AddRectFilled(cand.Max - ImVec2(4, 4), cand.Max + CalcTextSize(buf) + ImVec2(4, 4), IM_COL32(40, 0, 0, 200));
            draw_list->AddText(cand.Max, ~0U, buf);
        }
        if (debug_tty) { IMGUI_DEBUG_LOG_NAV("id 0x%08X\n%s\n", g.LastItemData.ID, buf); }
    }
#endif

    // Is it in the quadrant we're interested in moving to?
    bool new_best = false;
    if (quadrant == move_dir)
    {
        // Does it beat the current best candidate?
        if (dist_box < result->DistBox)
        {
            result->DistBox = dist_box;
            result->DistCenter = dist_center;
            return true;
        }
        if (dist_box == result->DistBox)
        {
            // Try using distance between center points to break ties
            if (dist_center < result->DistCenter)
            {
                result->DistCenter = dist_center;
                new_best = true;
            }
            else if (dist_center == result->DistCenter)
            {
                // Still tied! we need to be extra-careful to make sure everything gets linked properly. We consistently break ties by symbolically moving "later" items
                // (with higher index) to the right/downwards by an infinitesimal amount since we the current "best" button already (so it must have a lower index),
                // this is fairly easy. This rule ensures that all buttons with dx==dy==0 will end up being linked in order of appearance along the x axis.
                if (((move_dir == ImGuiDir_Up || move_dir == ImGuiDir_Down) ? dby : dbx) < 0.0f) // moving bj to the right/down decreases distance
                    new_best = true;
            }
        }
    }

    // Axial check: if 'curr' has no link at all in some direction and 'cand' lies roughly in that direction, add a tentative link. This will only be kept if no "real" matches
    // are found, so it only augments the graph produced by the above method using extra links. (important, since it doesn't guarantee strong connectedness)
    // This is just to avoid buttons having no links in a particular direction when there's a suitable neighbor. you get good graphs without this too.
    // 2017/09/29: FIXME: This now currently only enabled inside menu bars, ideally we'd disable it everywhere. Menus in particular need to catch failure. For general navigation it feels awkward.
    // Disabling it may lead to disconnected graphs when nodes are very spaced out on different axis. Perhaps consider offering this as an option?
    if (result->DistBox == FLT_MAX && dist_axial < result->DistAxial)  // Check axial match
        if (g.NavLayer == ImGuiNavLayer_Menu && !(g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
            if ((move_dir == ImGuiDir_Left && dax < 0.0f) || (move_dir == ImGuiDir_Right && dax > 0.0f) || (move_dir == ImGuiDir_Up && day < 0.0f) || (move_dir == ImGuiDir_Down && day > 0.0f))
            {
                result->DistAxial = dist_axial;
                new_best = true;
            }

    return new_best;
}

static void ImGui::NavApplyItemToResult(ImGuiNavItemData* result)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    result->Window = window;
    result->ID = g.LastItemData.ID;
    result->FocusScopeId = g.CurrentFocusScopeId;
    result->InFlags = g.LastItemData.InFlags;
    result->RectRel = WindowRectAbsToRel(window, g.LastItemData.NavRect);
    if (result->InFlags & ImGuiItemFlags_HasSelectionUserData)
    {
        IM_ASSERT(g.NextItemData.SelectionUserData != ImGuiSelectionUserData_Invalid);
        result->SelectionUserData = g.NextItemData.SelectionUserData; // INTENTIONAL: At this point this field is not cleared in NextItemData. Avoid unnecessary copy to LastItemData.
    }
}

// True when current work location may be scrolled horizontally when moving left / right.
// This is generally always true UNLESS within a column. We don't have a vertical equivalent.
void ImGui::NavUpdateCurrentWindowIsScrollPushableX()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DC.NavIsScrollPushableX = (g.CurrentTable == NULL && window->DC.CurrentColumns == NULL);
}

// We get there when either NavId == id, or when g.NavAnyRequest is set (which is updated by NavUpdateAnyRequestFlag above)
// This is called after LastItemData is set, but NextItemData is also still valid.
static void ImGui::NavProcessItem()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const ImGuiID id = g.LastItemData.ID;
    const ImGuiItemFlags item_flags = g.LastItemData.InFlags;

    // When inside a container that isn't scrollable with Left<>Right, clip NavRect accordingly (#2221)
    if (window->DC.NavIsScrollPushableX == false)
    {
        g.LastItemData.NavRect.Min.x = ImClamp(g.LastItemData.NavRect.Min.x, window->ClipRect.Min.x, window->ClipRect.Max.x);
        g.LastItemData.NavRect.Max.x = ImClamp(g.LastItemData.NavRect.Max.x, window->ClipRect.Min.x, window->ClipRect.Max.x);
    }
    const ImRect nav_bb = g.LastItemData.NavRect;

    // Process Init Request
    if (g.NavInitRequest && g.NavLayer == window->DC.NavLayerCurrent && (item_flags & ImGuiItemFlags_Disabled) == 0)
    {
        // Even if 'ImGuiItemFlags_NoNavDefaultFocus' is on (typically collapse/close button) we record the first ResultId so they can be used as a fallback
        const bool candidate_for_nav_default_focus = (item_flags & ImGuiItemFlags_NoNavDefaultFocus) == 0;
        if (candidate_for_nav_default_focus || g.NavInitResult.ID == 0)
        {
            NavApplyItemToResult(&g.NavInitResult);
        }
        if (candidate_for_nav_default_focus)
        {
            g.NavInitRequest = false; // Found a match, clear request
            NavUpdateAnyRequestFlag();
        }
    }

    // Process Move Request (scoring for navigation)
    // FIXME-NAV: Consider policy for double scoring (scoring from NavScoringRect + scoring from a rect wrapped according to current wrapping policy)
    if (g.NavMoveScoringItems && (item_flags & ImGuiItemFlags_Disabled) == 0)
    {
        if ((g.NavMoveFlags & ImGuiNavMoveFlags_FocusApi) || (window->Flags & ImGuiWindowFlags_NoNavInputs) == 0)
        {
            const bool is_tabbing = (g.NavMoveFlags & ImGuiNavMoveFlags_IsTabbing) != 0;
            if (is_tabbing)
            {
                NavProcessItemForTabbingRequest(id, item_flags, g.NavMoveFlags);
            }
            else if (g.NavId != id || (g.NavMoveFlags & ImGuiNavMoveFlags_AllowCurrentNavId))
            {
                ImGuiNavItemData* result = (window == g.NavWindow) ? &g.NavMoveResultLocal : &g.NavMoveResultOther;
                if (NavScoreItem(result))
                    NavApplyItemToResult(result);

                // Features like PageUp/PageDown need to maintain a separate score for the visible set of items.
                const float VISIBLE_RATIO = 0.70f;
                if ((g.NavMoveFlags & ImGuiNavMoveFlags_AlsoScoreVisibleSet) && window->ClipRect.Overlaps(nav_bb))
                    if (ImClamp(nav_bb.Max.y, window->ClipRect.Min.y, window->ClipRect.Max.y) - ImClamp(nav_bb.Min.y, window->ClipRect.Min.y, window->ClipRect.Max.y) >= (nav_bb.Max.y - nav_bb.Min.y) * VISIBLE_RATIO)
                        if (NavScoreItem(&g.NavMoveResultLocalVisible))
                            NavApplyItemToResult(&g.NavMoveResultLocalVisible);
            }
        }
    }

    // Update information for currently focused/navigated item
    if (g.NavId == id)
    {
        if (g.NavWindow != window)
            SetNavWindow(window); // Always refresh g.NavWindow, because some operations such as FocusItem() may not have a window.
        g.NavLayer = window->DC.NavLayerCurrent;
        SetNavFocusScope(g.CurrentFocusScopeId); // Will set g.NavFocusScopeId AND store g.NavFocusScopePath
        g.NavFocusScopeId = g.CurrentFocusScopeId;
        g.NavIdIsAlive = true;
        if (g.LastItemData.InFlags & ImGuiItemFlags_HasSelectionUserData)
        {
            IM_ASSERT(g.NextItemData.SelectionUserData != ImGuiSelectionUserData_Invalid);
            g.NavLastValidSelectionUserData = g.NextItemData.SelectionUserData; // INTENTIONAL: At this point this field is not cleared in NextItemData. Avoid unnecessary copy to LastItemData.
        }
        window->NavRectRel[window->DC.NavLayerCurrent] = WindowRectAbsToRel(window, nav_bb); // Store item bounding box (relative to window position)
    }
}

// Handle "scoring" of an item for a tabbing/focusing request initiated by NavUpdateCreateTabbingRequest().
// Note that SetKeyboardFocusHere() API calls are considered tabbing requests!
// - Case 1: no nav/active id:    set result to first eligible item, stop storing.
// - Case 2: tab forward:         on ref id set counter, on counter elapse store result
// - Case 3: tab forward wrap:    set result to first eligible item (preemptively), on ref id set counter, on next frame if counter hasn't elapsed store result. // FIXME-TABBING: Could be done as a next-frame forwarded request
// - Case 4: tab backward:        store all results, on ref id pick prev, stop storing
// - Case 5: tab backward wrap:   store all results, on ref id if no result keep storing until last // FIXME-TABBING: Could be done as next-frame forwarded requested
void ImGui::NavProcessItemForTabbingRequest(ImGuiID id, ImGuiItemFlags item_flags, ImGuiNavMoveFlags move_flags)
{
    ImGuiContext& g = *GImGui;

    if ((move_flags & ImGuiNavMoveFlags_FocusApi) == 0)
    {
        if (g.NavLayer != g.CurrentWindow->DC.NavLayerCurrent)
            return;
        if (g.NavFocusScopeId != g.CurrentFocusScopeId)
            return;
    }

    // - Can always land on an item when using API call.
    // - Tabbing with _NavEnableKeyboard (space/enter/arrows): goes through every item.
    // - Tabbing without _NavEnableKeyboard: goes through inputable items only.
    bool can_stop;
    if (move_flags & ImGuiNavMoveFlags_FocusApi)
        can_stop = true;
    else
        can_stop = (item_flags & ImGuiItemFlags_NoTabStop) == 0 && ((g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) || (item_flags & ImGuiItemFlags_Inputable));

    // Always store in NavMoveResultLocal (unlike directional request which uses NavMoveResultOther on sibling/flattened windows)
    ImGuiNavItemData* result = &g.NavMoveResultLocal;
    if (g.NavTabbingDir == +1)
    {
        // Tab Forward or SetKeyboardFocusHere() with >= 0
        if (can_stop && g.NavTabbingResultFirst.ID == 0)
            NavApplyItemToResult(&g.NavTabbingResultFirst);
        if (can_stop && g.NavTabbingCounter > 0 && --g.NavTabbingCounter == 0)
            NavMoveRequestResolveWithLastItem(result);
        else if (g.NavId == id)
            g.NavTabbingCounter = 1;
    }
    else if (g.NavTabbingDir == -1)
    {
        // Tab Backward
        if (g.NavId == id)
        {
            if (result->ID)
            {
                g.NavMoveScoringItems = false;
                NavUpdateAnyRequestFlag();
            }
        }
        else if (can_stop)
        {
            // Keep applying until reaching NavId
            NavApplyItemToResult(result);
        }
    }
    else if (g.NavTabbingDir == 0)
    {
        if (can_stop && g.NavId == id)
            NavMoveRequestResolveWithLastItem(result);
        if (can_stop && g.NavTabbingResultFirst.ID == 0) // Tab init
            NavApplyItemToResult(&g.NavTabbingResultFirst);
    }
}

bool ImGui::NavMoveRequestButNoResultYet()
{
    ImGuiContext& g = *GImGui;
    return g.NavMoveScoringItems && g.NavMoveResultLocal.ID == 0 && g.NavMoveResultOther.ID == 0;
}

// FIXME: ScoringRect is not set
void ImGui::NavMoveRequestSubmit(ImGuiDir move_dir, ImGuiDir clip_dir, ImGuiNavMoveFlags move_flags, ImGuiScrollFlags scroll_flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindow != NULL);
    //IMGUI_DEBUG_LOG_NAV("[nav] NavMoveRequestSubmit: dir %c, window \"%s\"\n", "-WENS"[move_dir + 1], g.NavWindow->Name);

    if (move_flags & ImGuiNavMoveFlags_IsTabbing)
        move_flags |= ImGuiNavMoveFlags_AllowCurrentNavId;

    g.NavMoveSubmitted = g.NavMoveScoringItems = true;
    g.NavMoveDir = move_dir;
    g.NavMoveDirForDebug = move_dir;
    g.NavMoveClipDir = clip_dir;
    g.NavMoveFlags = move_flags;
    g.NavMoveScrollFlags = scroll_flags;
    g.NavMoveForwardToNextFrame = false;
    g.NavMoveKeyMods = (move_flags & ImGuiNavMoveFlags_FocusApi) ? 0 : g.IO.KeyMods;
    g.NavMoveResultLocal.Clear();
    g.NavMoveResultLocalVisible.Clear();
    g.NavMoveResultOther.Clear();
    g.NavTabbingCounter = 0;
    g.NavTabbingResultFirst.Clear();
    NavUpdateAnyRequestFlag();
}

void ImGui::NavMoveRequestResolveWithLastItem(ImGuiNavItemData* result)
{
    ImGuiContext& g = *GImGui;
    g.NavMoveScoringItems = false; // Ensure request doesn't need more processing
    NavApplyItemToResult(result);
    NavUpdateAnyRequestFlag();
}

// Called by TreePop() to implement ImGuiTreeNodeFlags_NavLeftJumpsBackHere
void ImGui::NavMoveRequestResolveWithPastTreeNode(ImGuiNavItemData* result, ImGuiTreeNodeStackData* tree_node_data)
{
    ImGuiContext& g = *GImGui;
    g.NavMoveScoringItems = false;
    g.LastItemData.ID = tree_node_data->ID;
    g.LastItemData.InFlags = tree_node_data->InFlags & ~ImGuiItemFlags_HasSelectionUserData; // Losing SelectionUserData, recovered next-frame (cheaper).
    g.LastItemData.NavRect = tree_node_data->NavRect;
    NavApplyItemToResult(result); // Result this instead of implementing a NavApplyPastTreeNodeToResult()
    NavClearPreferredPosForAxis(ImGuiAxis_Y);
    NavUpdateAnyRequestFlag();
}

void ImGui::NavMoveRequestCancel()
{
    ImGuiContext& g = *GImGui;
    g.NavMoveSubmitted = g.NavMoveScoringItems = false;
    NavUpdateAnyRequestFlag();
}

// Forward will reuse the move request again on the next frame (generally with modifications done to it)
void ImGui::NavMoveRequestForward(ImGuiDir move_dir, ImGuiDir clip_dir, ImGuiNavMoveFlags move_flags, ImGuiScrollFlags scroll_flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavMoveForwardToNextFrame == false);
    NavMoveRequestCancel();
    g.NavMoveForwardToNextFrame = true;
    g.NavMoveDir = move_dir;
    g.NavMoveClipDir = clip_dir;
    g.NavMoveFlags = move_flags | ImGuiNavMoveFlags_Forwarded;
    g.NavMoveScrollFlags = scroll_flags;
}

// Navigation wrap-around logic is delayed to the end of the frame because this operation is only valid after entire
// popup is assembled and in case of appended popups it is not clear which EndPopup() call is final.
void ImGui::NavMoveRequestTryWrapping(ImGuiWindow* window, ImGuiNavMoveFlags wrap_flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT((wrap_flags & ImGuiNavMoveFlags_WrapMask_ ) != 0 && (wrap_flags & ~ImGuiNavMoveFlags_WrapMask_) == 0); // Call with _WrapX, _WrapY, _LoopX, _LoopY

    // In theory we should test for NavMoveRequestButNoResultYet() but there's no point doing it:
    // as NavEndFrame() will do the same test. It will end up calling NavUpdateCreateWrappingRequest().
    if (g.NavWindow == window && g.NavMoveScoringItems && g.NavLayer == ImGuiNavLayer_Main)
        g.NavMoveFlags = (g.NavMoveFlags & ~ImGuiNavMoveFlags_WrapMask_) | wrap_flags;
}

// FIXME: This could be replaced by updating a frame number in each window when (window == NavWindow) and (NavLayer == 0).
// This way we could find the last focused window among our children. It would be much less confusing this way?
static void ImGui::NavSaveLastChildNavWindowIntoParent(ImGuiWindow* nav_window)
{
    ImGuiWindow* parent = nav_window;
    while (parent && parent->RootWindow != parent && (parent->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu)) == 0)
        parent = parent->ParentWindow;
    if (parent && parent != nav_window)
        parent->NavLastChildNavWindow = nav_window;
}

// Restore the last focused child.
// Call when we are expected to land on the Main Layer (0) after FocusWindow()
static ImGuiWindow* ImGui::NavRestoreLastChildNavWindow(ImGuiWindow* window)
{
    if (window->NavLastChildNavWindow && window->NavLastChildNavWindow->WasActive)
        return window->NavLastChildNavWindow;
    return window;
}

void ImGui::NavRestoreLayer(ImGuiNavLayer layer)
{
    ImGuiContext& g = *GImGui;
    if (layer == ImGuiNavLayer_Main)
    {
        ImGuiWindow* prev_nav_window = g.NavWindow;
        g.NavWindow = NavRestoreLastChildNavWindow(g.NavWindow);    // FIXME-NAV: Should clear ongoing nav requests?
        g.NavLastValidSelectionUserData = ImGuiSelectionUserData_Invalid;
        if (prev_nav_window)
            IMGUI_DEBUG_LOG_FOCUS("[focus] NavRestoreLayer: from \"%s\" to SetNavWindow(\"%s\")\n", prev_nav_window->Name, g.NavWindow->Name);
    }
    ImGuiWindow* window = g.NavWindow;
    if (window->NavLastIds[layer] != 0)
    {
        SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
    }
    else
    {
        g.NavLayer = layer;
        NavInitWindow(window, true);
    }
}

void ImGui::NavRestoreHighlightAfterMove()
{
    ImGuiContext& g = *GImGui;
    g.NavDisableHighlight = false;
    g.NavDisableMouseHover = g.NavMousePosDirty = true;
}

static inline void ImGui::NavUpdateAnyRequestFlag()
{
    ImGuiContext& g = *GImGui;
    g.NavAnyRequest = g.NavMoveScoringItems || g.NavInitRequest || (IMGUI_DEBUG_NAV_SCORING && g.NavWindow != NULL);
    if (g.NavAnyRequest)
        IM_ASSERT(g.NavWindow != NULL);
}

// This needs to be called before we submit any widget (aka in or before Begin)
void ImGui::NavInitWindow(ImGuiWindow* window, bool force_reinit)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(window == g.NavWindow);

    if (window->Flags & ImGuiWindowFlags_NoNavInputs)
    {
        g.NavId = 0;
        SetNavFocusScope(window->NavRootFocusScopeId);
        return;
    }

    bool init_for_nav = false;
    if (window == window->RootWindow || (window->Flags & ImGuiWindowFlags_Popup) || (window->NavLastIds[0] == 0) || force_reinit)
        init_for_nav = true;
    IMGUI_DEBUG_LOG_NAV("[nav] NavInitRequest: from NavInitWindow(), init_for_nav=%d, window=\"%s\", layer=%d\n", init_for_nav, window->Name, g.NavLayer);
    if (init_for_nav)
    {
        SetNavID(0, g.NavLayer, window->NavRootFocusScopeId, ImRect());
        g.NavInitRequest = true;
        g.NavInitRequestFromMove = false;
        g.NavInitResult.ID = 0;
        NavUpdateAnyRequestFlag();
    }
    else
    {
        g.NavId = window->NavLastIds[0];
        SetNavFocusScope(window->NavRootFocusScopeId);
    }
}

static ImVec2 ImGui::NavCalcPreferredRefPos()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.NavWindow;
    const bool activated_shortcut = g.ActiveId != 0 && g.ActiveIdFromShortcut && g.ActiveId == g.LastItemData.ID;

    // Testing for !activated_shortcut here could in theory be removed if we decided that activating a remote shortcut altered one of the g.NavDisableXXX flag.
    if ((g.NavDisableHighlight || !g.NavDisableMouseHover || !window) && !activated_shortcut)
    {
        // Mouse (we need a fallback in case the mouse becomes invalid after being used)
        // The +1.0f offset when stored by OpenPopupEx() allows reopening this or another popup (same or another mouse button) while not moving the mouse, it is pretty standard.
        // In theory we could move that +1.0f offset in OpenPopupEx()
        ImVec2 p = IsMousePosValid(&g.IO.MousePos) ? g.IO.MousePos : g.MouseLastValidPos;
        return ImVec2(p.x + 1.0f, p.y);
    }
    else
    {
        // When navigation is active and mouse is disabled, pick a position around the bottom left of the currently navigated item
        ImRect ref_rect;
        if (activated_shortcut)
            ref_rect = g.LastItemData.NavRect;
        else
            ref_rect = WindowRectRelToAbs(window, window->NavRectRel[g.NavLayer]);

        // Take account of upcoming scrolling (maybe set mouse pos should be done in EndFrame?)
        if (window->LastFrameActive != g.FrameCount && (window->ScrollTarget.x != FLT_MAX || window->ScrollTarget.y != FLT_MAX))
        {
            ImVec2 next_scroll = CalcNextScrollFromScrollTargetAndClamp(window);
            ref_rect.Translate(window->Scroll - next_scroll);
        }
        ImVec2 pos = ImVec2(ref_rect.Min.x + ImMin(g.Style.FramePadding.x * 4, ref_rect.GetWidth()), ref_rect.Max.y - ImMin(g.Style.FramePadding.y, ref_rect.GetHeight()));
        ImGuiViewport* viewport = GetMainViewport();
        return ImTrunc(ImClamp(pos, viewport->Pos, viewport->Pos + viewport->Size)); // ImTrunc() is important because non-integer mouse position application in backend might be lossy and result in undesirable non-zero delta.
    }
}

float ImGui::GetNavTweakPressedAmount(ImGuiAxis axis)
{
    ImGuiContext& g = *GImGui;
    float repeat_delay, repeat_rate;
    GetTypematicRepeatRate(ImGuiInputFlags_RepeatRateNavTweak, &repeat_delay, &repeat_rate);

    ImGuiKey key_less, key_more;
    if (g.NavInputSource == ImGuiInputSource_Gamepad)
    {
        key_less = (axis == ImGuiAxis_X) ? ImGuiKey_GamepadDpadLeft : ImGuiKey_GamepadDpadUp;
        key_more = (axis == ImGuiAxis_X) ? ImGuiKey_GamepadDpadRight : ImGuiKey_GamepadDpadDown;
    }
    else
    {
        key_less = (axis == ImGuiAxis_X) ? ImGuiKey_LeftArrow : ImGuiKey_UpArrow;
        key_more = (axis == ImGuiAxis_X) ? ImGuiKey_RightArrow : ImGuiKey_DownArrow;
    }
    float amount = (float)GetKeyPressedAmount(key_more, repeat_delay, repeat_rate) - (float)GetKeyPressedAmount(key_less, repeat_delay, repeat_rate);
    if (amount != 0.0f && IsKeyDown(key_less) && IsKeyDown(key_more)) // Cancel when opposite directions are held, regardless of repeat phase
        amount = 0.0f;
    return amount;
}

static void ImGui::NavUpdate()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    io.WantSetMousePos = false;
    //if (g.NavScoringDebugCount > 0) IMGUI_DEBUG_LOG_NAV("[nav] NavScoringDebugCount %d for '%s' layer %d (Init:%d, Move:%d)\n", g.NavScoringDebugCount, g.NavWindow ? g.NavWindow->Name : "NULL", g.NavLayer, g.NavInitRequest || g.NavInitResultId != 0, g.NavMoveRequest);

    // Set input source based on which keys are last pressed (as some features differs when used with Gamepad vs Keyboard)
    // FIXME-NAV: Now that keys are separated maybe we can get rid of NavInputSource?
    const bool nav_gamepad_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
    const ImGuiKey nav_gamepad_keys_to_change_source[] = { ImGuiKey_GamepadFaceRight, ImGuiKey_GamepadFaceLeft, ImGuiKey_GamepadFaceUp, ImGuiKey_GamepadFaceDown, ImGuiKey_GamepadDpadRight, ImGuiKey_GamepadDpadLeft, ImGuiKey_GamepadDpadUp, ImGuiKey_GamepadDpadDown };
    if (nav_gamepad_active)
        for (ImGuiKey key : nav_gamepad_keys_to_change_source)
            if (IsKeyDown(key))
                g.NavInputSource = ImGuiInputSource_Gamepad;
    const bool nav_keyboard_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
    const ImGuiKey nav_keyboard_keys_to_change_source[] = { ImGuiKey_Space, ImGuiKey_Enter, ImGuiKey_Escape, ImGuiKey_RightArrow, ImGuiKey_LeftArrow, ImGuiKey_UpArrow, ImGuiKey_DownArrow };
    if (nav_keyboard_active)
        for (ImGuiKey key : nav_keyboard_keys_to_change_source)
            if (IsKeyDown(key))
                g.NavInputSource = ImGuiInputSource_Keyboard;

    // Process navigation init request (select first/default focus)
    g.NavJustMovedToId = 0;
    g.NavJustMovedToFocusScopeId = g.NavJustMovedFromFocusScopeId = 0;
    if (g.NavInitResult.ID != 0)
        NavInitRequestApplyResult();
    g.NavInitRequest = false;
    g.NavInitRequestFromMove = false;
    g.NavInitResult.ID = 0;

    // Process navigation move request
    if (g.NavMoveSubmitted)
        NavMoveRequestApplyResult();
    g.NavTabbingCounter = 0;
    g.NavMoveSubmitted = g.NavMoveScoringItems = false;

    // Schedule mouse position update (will be done at the bottom of this function, after 1) processing all move requests and 2) updating scrolling)
    bool set_mouse_pos = false;
    if (g.NavMousePosDirty && g.NavIdIsAlive)
        if (!g.NavDisableHighlight && g.NavDisableMouseHover && g.NavWindow)
            set_mouse_pos = true;
    g.NavMousePosDirty = false;
    IM_ASSERT(g.NavLayer == ImGuiNavLayer_Main || g.NavLayer == ImGuiNavLayer_Menu);

    // Store our return window (for returning from Menu Layer to Main Layer) and clear it as soon as we step back in our own Layer 0
    if (g.NavWindow)
        NavSaveLastChildNavWindowIntoParent(g.NavWindow);
    if (g.NavWindow && g.NavWindow->NavLastChildNavWindow != NULL && g.NavLayer == ImGuiNavLayer_Main)
        g.NavWindow->NavLastChildNavWindow = NULL;

    // Update CTRL+TAB and Windowing features (hold Square to move/resize/etc.)
    NavUpdateWindowing();

    // Set output flags for user application
    io.NavActive = (nav_keyboard_active || nav_gamepad_active) && g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs);
    io.NavVisible = (io.NavActive && g.NavId != 0 && !g.NavDisableHighlight) || (g.NavWindowingTarget != NULL);

    // Process NavCancel input (to close a popup, get back to parent, clear focus)
    NavUpdateCancelRequest();

    // Process manual activation request
    g.NavActivateId = g.NavActivateDownId = g.NavActivatePressedId = 0;
    g.NavActivateFlags = ImGuiActivateFlags_None;
    if (g.NavId != 0 && !g.NavDisableHighlight && !g.NavWindowingTarget && g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
    {
        const bool activate_down = (nav_keyboard_active && IsKeyDown(ImGuiKey_Space, ImGuiKeyOwner_NoOwner)) || (nav_gamepad_active && IsKeyDown(ImGuiKey_NavGamepadActivate, ImGuiKeyOwner_NoOwner));
        const bool activate_pressed = activate_down && ((nav_keyboard_active && IsKeyPressed(ImGuiKey_Space, 0, ImGuiKeyOwner_NoOwner)) || (nav_gamepad_active && IsKeyPressed(ImGuiKey_NavGamepadActivate, 0, ImGuiKeyOwner_NoOwner)));
        const bool input_down = (nav_keyboard_active && (IsKeyDown(ImGuiKey_Enter, ImGuiKeyOwner_NoOwner) || IsKeyDown(ImGuiKey_KeypadEnter, ImGuiKeyOwner_NoOwner))) || (nav_gamepad_active && IsKeyDown(ImGuiKey_NavGamepadInput, ImGuiKeyOwner_NoOwner));
        const bool input_pressed = input_down && ((nav_keyboard_active && (IsKeyPressed(ImGuiKey_Enter, 0, ImGuiKeyOwner_NoOwner) || IsKeyPressed(ImGuiKey_KeypadEnter, 0, ImGuiKeyOwner_NoOwner))) || (nav_gamepad_active && IsKeyPressed(ImGuiKey_NavGamepadInput, 0, ImGuiKeyOwner_NoOwner)));
        if (g.ActiveId == 0 && activate_pressed)
        {
            g.NavActivateId = g.NavId;
            g.NavActivateFlags = ImGuiActivateFlags_PreferTweak;
        }
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && input_pressed)
        {
            g.NavActivateId = g.NavId;
            g.NavActivateFlags = ImGuiActivateFlags_PreferInput;
        }
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && (activate_down || input_down))
            g.NavActivateDownId = g.NavId;
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && (activate_pressed || input_pressed))
        {
            g.NavActivatePressedId = g.NavId;
            NavHighlightActivated(g.NavId);
        }
    }
    if (g.NavWindow && (g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
        g.NavDisableHighlight = true;
    if (g.NavActivateId != 0)
        IM_ASSERT(g.NavActivateDownId == g.NavActivateId);

    // Highlight
    if (g.NavHighlightActivatedTimer > 0.0f)
        g.NavHighlightActivatedTimer = ImMax(0.0f, g.NavHighlightActivatedTimer - io.DeltaTime);
    if (g.NavHighlightActivatedTimer == 0.0f)
        g.NavHighlightActivatedId = 0;

    // Process programmatic activation request
    // FIXME-NAV: Those should eventually be queued (unlike focus they don't cancel each others)
    if (g.NavNextActivateId != 0)
    {
        g.NavActivateId = g.NavActivateDownId = g.NavActivatePressedId = g.NavNextActivateId;
        g.NavActivateFlags = g.NavNextActivateFlags;
    }
    g.NavNextActivateId = 0;

    // Process move requests
    NavUpdateCreateMoveRequest();
    if (g.NavMoveDir == ImGuiDir_None)
        NavUpdateCreateTabbingRequest();
    NavUpdateAnyRequestFlag();
    g.NavIdIsAlive = false;

    // Scrolling
    if (g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs) && !g.NavWindowingTarget)
    {
        // *Fallback* manual-scroll with Nav directional keys when window has no navigable item
        ImGuiWindow* window = g.NavWindow;
        const float scroll_speed = IM_ROUND(window->CalcFontSize() * 100 * io.DeltaTime); // We need round the scrolling speed because sub-pixel scroll isn't reliably supported.
        const ImGuiDir move_dir = g.NavMoveDir;
        if (window->DC.NavLayersActiveMask == 0x00 && window->DC.NavWindowHasScrollY && move_dir != ImGuiDir_None)
        {
            if (move_dir == ImGuiDir_Left || move_dir == ImGuiDir_Right)
                SetScrollX(window, ImTrunc(window->Scroll.x + ((move_dir == ImGuiDir_Left) ? -1.0f : +1.0f) * scroll_speed));
            if (move_dir == ImGuiDir_Up || move_dir == ImGuiDir_Down)
                SetScrollY(window, ImTrunc(window->Scroll.y + ((move_dir == ImGuiDir_Up) ? -1.0f : +1.0f) * scroll_speed));
        }

        // *Normal* Manual scroll with LStick
        // Next movement request will clamp the NavId reference rectangle to the visible area, so navigation will resume within those bounds.
        if (nav_gamepad_active)
        {
            const ImVec2 scroll_dir = GetKeyMagnitude2d(ImGuiKey_GamepadLStickLeft, ImGuiKey_GamepadLStickRight, ImGuiKey_GamepadLStickUp, ImGuiKey_GamepadLStickDown);
            const float tweak_factor = IsKeyDown(ImGuiKey_NavGamepadTweakSlow) ? 1.0f / 10.0f : IsKeyDown(ImGuiKey_NavGamepadTweakFast) ? 10.0f : 1.0f;
            if (scroll_dir.x != 0.0f && window->ScrollbarX)
                SetScrollX(window, ImTrunc(window->Scroll.x + scroll_dir.x * scroll_speed * tweak_factor));
            if (scroll_dir.y != 0.0f)
                SetScrollY(window, ImTrunc(window->Scroll.y + scroll_dir.y * scroll_speed * tweak_factor));
        }
    }

    // Always prioritize mouse highlight if navigation is disabled
    if (!nav_keyboard_active && !nav_gamepad_active)
    {
        g.NavDisableHighlight = true;
        g.NavDisableMouseHover = set_mouse_pos = false;
    }

    // Update mouse position if requested
    // (This will take into account the possibility that a Scroll was queued in the window to offset our absolute mouse position before scroll has been applied)
    if (set_mouse_pos && io.ConfigNavMoveSetMousePos && (io.BackendFlags & ImGuiBackendFlags_HasSetMousePos))
        TeleportMousePos(NavCalcPreferredRefPos());

    // [DEBUG]
    g.NavScoringDebugCount = 0;
#if IMGUI_DEBUG_NAV_RECTS
    if (ImGuiWindow* debug_window = g.NavWindow)
    {
        ImDrawList* draw_list = GetForegroundDrawList(debug_window);
        int layer = g.NavLayer; /* for (int layer = 0; layer < 2; layer++)*/ { ImRect r = WindowRectRelToAbs(debug_window, debug_window->NavRectRel[layer]); draw_list->AddRect(r.Min, r.Max, IM_COL32(255, 200, 0, 255)); }
        //if (1) { ImU32 col = (!debug_window->Hidden) ? IM_COL32(255,0,255,255) : IM_COL32(255,0,0,255); ImVec2 p = NavCalcPreferredRefPos(); char buf[32]; ImFormatString(buf, 32, "%d", g.NavLayer); draw_list->AddCircleFilled(p, 3.0f, col); draw_list->AddText(NULL, 13.0f, p + ImVec2(8,-4), col, buf); }
    }
#endif
}

void ImGui::NavInitRequestApplyResult()
{
    // In very rare cases g.NavWindow may be null (e.g. clearing focus after requesting an init request, which does happen when releasing Alt while clicking on void)
    ImGuiContext& g = *GImGui;
    if (!g.NavWindow)
        return;

    ImGuiNavItemData* result = &g.NavInitResult;
    if (g.NavId != result->ID)
    {
        g.NavJustMovedFromFocusScopeId = g.NavFocusScopeId;
        g.NavJustMovedToId = result->ID;
        g.NavJustMovedToFocusScopeId = result->FocusScopeId;
        g.NavJustMovedToKeyMods = 0;
        g.NavJustMovedToIsTabbing = false;
        g.NavJustMovedToHasSelectionData = (result->InFlags & ImGuiItemFlags_HasSelectionUserData) != 0;
    }

    // Apply result from previous navigation init request (will typically select the first item, unless SetItemDefaultFocus() has been called)
    // FIXME-NAV: On _NavFlattened windows, g.NavWindow will only be updated during subsequent frame. Not a problem currently.
    IMGUI_DEBUG_LOG_NAV("[nav] NavInitRequest: ApplyResult: NavID 0x%08X in Layer %d Window \"%s\"\n", result->ID, g.NavLayer, g.NavWindow->Name);
    SetNavID(result->ID, g.NavLayer, result->FocusScopeId, result->RectRel);
    g.NavIdIsAlive = true; // Mark as alive from previous frame as we got a result
    if (result->SelectionUserData != ImGuiSelectionUserData_Invalid)
        g.NavLastValidSelectionUserData = result->SelectionUserData;
    if (g.NavInitRequestFromMove)
        NavRestoreHighlightAfterMove();
}

// Bias scoring rect ahead of scoring + update preferred pos (if missing) using source position
static void NavBiasScoringRect(ImRect& r, ImVec2& preferred_pos_rel, ImGuiDir move_dir, ImGuiNavMoveFlags move_flags)
{
    // Bias initial rect
    ImGuiContext& g = *GImGui;
    const ImVec2 rel_to_abs_offset = g.NavWindow->DC.CursorStartPos;

    // Initialize bias on departure if we don't have any. So mouse-click + arrow will record bias.
    // - We default to L/U bias, so moving down from a large source item into several columns will land on left-most column.
    // - But each successful move sets new bias on one axis, only cleared when using mouse.
    if ((move_flags & ImGuiNavMoveFlags_Forwarded) == 0)
    {
        if (preferred_pos_rel.x == FLT_MAX)
            preferred_pos_rel.x = ImMin(r.Min.x + 1.0f, r.Max.x) - rel_to_abs_offset.x;
        if (preferred_pos_rel.y == FLT_MAX)
            preferred_pos_rel.y = r.GetCenter().y - rel_to_abs_offset.y;
    }

    // Apply general bias on the other axis
    if ((move_dir == ImGuiDir_Up || move_dir == ImGuiDir_Down) && preferred_pos_rel.x != FLT_MAX)
        r.Min.x = r.Max.x = preferred_pos_rel.x + rel_to_abs_offset.x;
    else if ((move_dir == ImGuiDir_Left || move_dir == ImGuiDir_Right) && preferred_pos_rel.y != FLT_MAX)
        r.Min.y = r.Max.y = preferred_pos_rel.y + rel_to_abs_offset.y;
}

void ImGui::NavUpdateCreateMoveRequest()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    ImGuiWindow* window = g.NavWindow;
    const bool nav_gamepad_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
    const bool nav_keyboard_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;

    if (g.NavMoveForwardToNextFrame && window != NULL)
    {
        // Forwarding previous request (which has been modified, e.g. wrap around menus rewrite the requests with a starting rectangle at the other side of the window)
        // (preserve most state, which were already set by the NavMoveRequestForward() function)
        IM_ASSERT(g.NavMoveDir != ImGuiDir_None && g.NavMoveClipDir != ImGuiDir_None);
        IM_ASSERT(g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded);
        IMGUI_DEBUG_LOG_NAV("[nav] NavMoveRequestForward %d\n", g.NavMoveDir);
    }
    else
    {
        // Initiate directional inputs request
        g.NavMoveDir = ImGuiDir_None;
        g.NavMoveFlags = ImGuiNavMoveFlags_None;
        g.NavMoveScrollFlags = ImGuiScrollFlags_None;
        if (window && !g.NavWindowingTarget && !(window->Flags & ImGuiWindowFlags_NoNavInputs))
        {
            const ImGuiInputFlags repeat_mode = ImGuiInputFlags_Repeat | (ImGuiInputFlags)ImGuiInputFlags_RepeatRateNavMove;
            if (!IsActiveIdUsingNavDir(ImGuiDir_Left)  && ((nav_gamepad_active && IsKeyPressed(ImGuiKey_GamepadDpadLeft,  repeat_mode, ImGuiKeyOwner_NoOwner)) || (nav_keyboard_active && IsKeyPressed(ImGuiKey_LeftArrow,  repeat_mode, ImGuiKeyOwner_NoOwner)))) { g.NavMoveDir = ImGuiDir_Left; }
            if (!IsActiveIdUsingNavDir(ImGuiDir_Right) && ((nav_gamepad_active && IsKeyPressed(ImGuiKey_GamepadDpadRight, repeat_mode, ImGuiKeyOwner_NoOwner)) || (nav_keyboard_active && IsKeyPressed(ImGuiKey_RightArrow, repeat_mode, ImGuiKeyOwner_NoOwner)))) { g.NavMoveDir = ImGuiDir_Right; }
            if (!IsActiveIdUsingNavDir(ImGuiDir_Up)    && ((nav_gamepad_active && IsKeyPressed(ImGuiKey_GamepadDpadUp,    repeat_mode, ImGuiKeyOwner_NoOwner)) || (nav_keyboard_active && IsKeyPressed(ImGuiKey_UpArrow,    repeat_mode, ImGuiKeyOwner_NoOwner)))) { g.NavMoveDir = ImGuiDir_Up; }
            if (!IsActiveIdUsingNavDir(ImGuiDir_Down)  && ((nav_gamepad_active && IsKeyPressed(ImGuiKey_GamepadDpadDown,  repeat_mode, ImGuiKeyOwner_NoOwner)) || (nav_keyboard_active && IsKeyPressed(ImGuiKey_DownArrow,  repeat_mode, ImGuiKeyOwner_NoOwner)))) { g.NavMoveDir = ImGuiDir_Down; }
        }
        g.NavMoveClipDir = g.NavMoveDir;
        g.NavScoringNoClipRect = ImRect(+FLT_MAX, +FLT_MAX, -FLT_MAX, -FLT_MAX);
    }

    // Update PageUp/PageDown/Home/End scroll
    // FIXME-NAV: Consider enabling those keys even without the master ImGuiConfigFlags_NavEnableKeyboard flag?
    float scoring_rect_offset_y = 0.0f;
    if (window && g.NavMoveDir == ImGuiDir_None && nav_keyboard_active)
        scoring_rect_offset_y = NavUpdatePageUpPageDown();
    if (scoring_rect_offset_y != 0.0f)
    {
        g.NavScoringNoClipRect = window->InnerRect;
        g.NavScoringNoClipRect.TranslateY(scoring_rect_offset_y);
    }

    // [DEBUG] Always send a request when holding CTRL. Hold CTRL + Arrow change the direction.
#if IMGUI_DEBUG_NAV_SCORING
    //if (io.KeyCtrl && IsKeyPressed(ImGuiKey_C))
    //    g.NavMoveDirForDebug = (ImGuiDir)((g.NavMoveDirForDebug + 1) & 3);
    if (io.KeyCtrl)
    {
        if (g.NavMoveDir == ImGuiDir_None)
            g.NavMoveDir = g.NavMoveDirForDebug;
        g.NavMoveClipDir = g.NavMoveDir;
        g.NavMoveFlags |= ImGuiNavMoveFlags_DebugNoResult;
    }
#endif

    // Submit
    g.NavMoveForwardToNextFrame = false;
    if (g.NavMoveDir != ImGuiDir_None)
        NavMoveRequestSubmit(g.NavMoveDir, g.NavMoveClipDir, g.NavMoveFlags, g.NavMoveScrollFlags);

    // Moving with no reference triggers an init request (will be used as a fallback if the direction fails to find a match)
    if (g.NavMoveSubmitted && g.NavId == 0)
    {
        IMGUI_DEBUG_LOG_NAV("[nav] NavInitRequest: from move, window \"%s\", layer=%d\n", window ? window->Name : "<NULL>", g.NavLayer);
        g.NavInitRequest = g.NavInitRequestFromMove = true;
        g.NavInitResult.ID = 0;
        g.NavDisableHighlight = false;
    }

    // When using gamepad, we project the reference nav bounding box into window visible area.
    // This is to allow resuming navigation inside the visible area after doing a large amount of scrolling,
    // since with gamepad all movements are relative (can't focus a visible object like we can with the mouse).
    if (g.NavMoveSubmitted && g.NavInputSource == ImGuiInputSource_Gamepad && g.NavLayer == ImGuiNavLayer_Main && window != NULL)// && (g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded))
    {
        bool clamp_x = (g.NavMoveFlags & (ImGuiNavMoveFlags_LoopX | ImGuiNavMoveFlags_WrapX)) == 0;
        bool clamp_y = (g.NavMoveFlags & (ImGuiNavMoveFlags_LoopY | ImGuiNavMoveFlags_WrapY)) == 0;
        ImRect inner_rect_rel = WindowRectAbsToRel(window, ImRect(window->InnerRect.Min - ImVec2(1, 1), window->InnerRect.Max + ImVec2(1, 1)));

        // Take account of changing scroll to handle triggering a new move request on a scrolling frame. (#6171)
        // Otherwise 'inner_rect_rel' would be off on the move result frame.
        inner_rect_rel.Translate(CalcNextScrollFromScrollTargetAndClamp(window) - window->Scroll);

        if ((clamp_x || clamp_y) && !inner_rect_rel.Contains(window->NavRectRel[g.NavLayer]))
        {
            IMGUI_DEBUG_LOG_NAV("[nav] NavMoveRequest: clamp NavRectRel for gamepad move\n");
            float pad_x = ImMin(inner_rect_rel.GetWidth(), window->CalcFontSize() * 0.5f);
            float pad_y = ImMin(inner_rect_rel.GetHeight(), window->CalcFontSize() * 0.5f); // Terrible approximation for the intent of starting navigation from first fully visible item
            inner_rect_rel.Min.x = clamp_x ? (inner_rect_rel.Min.x + pad_x) : -FLT_MAX;
            inner_rect_rel.Max.x = clamp_x ? (inner_rect_rel.Max.x - pad_x) : +FLT_MAX;
            inner_rect_rel.Min.y = clamp_y ? (inner_rect_rel.Min.y + pad_y) : -FLT_MAX;
            inner_rect_rel.Max.y = clamp_y ? (inner_rect_rel.Max.y - pad_y) : +FLT_MAX;
            window->NavRectRel[g.NavLayer].ClipWithFull(inner_rect_rel);
            g.NavId = 0;
        }
    }

    // For scoring we use a single segment on the left side our current item bounding box (not touching the edge to avoid box overlap with zero-spaced items)
    ImRect scoring_rect;
    if (window != NULL)
    {
        ImRect nav_rect_rel = !window->NavRectRel[g.NavLayer].IsInverted() ? window->NavRectRel[g.NavLayer] : ImRect(0, 0, 0, 0);
        scoring_rect = WindowRectRelToAbs(window, nav_rect_rel);
        scoring_rect.TranslateY(scoring_rect_offset_y);
        if (g.NavMoveSubmitted)
            NavBiasScoringRect(scoring_rect, window->RootWindowForNav->NavPreferredScoringPosRel[g.NavLayer], g.NavMoveDir, g.NavMoveFlags);
        IM_ASSERT(!scoring_rect.IsInverted()); // Ensure we have a non-inverted bounding box here will allow us to remove extraneous ImFabs() calls in NavScoreItem().
        //GetForegroundDrawList()->AddRect(scoring_rect.Min, scoring_rect.Max, IM_COL32(255,200,0,255)); // [DEBUG]
        //if (!g.NavScoringNoClipRect.IsInverted()) { GetForegroundDrawList()->AddRect(g.NavScoringNoClipRect.Min, g.NavScoringNoClipRect.Max, IM_COL32(255, 200, 0, 255)); } // [DEBUG]
    }
    g.NavScoringRect = scoring_rect;
    g.NavScoringNoClipRect.Add(scoring_rect);
}

void ImGui::NavUpdateCreateTabbingRequest()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.NavWindow;
    IM_ASSERT(g.NavMoveDir == ImGuiDir_None);
    if (window == NULL || g.NavWindowingTarget != NULL || (window->Flags & ImGuiWindowFlags_NoNavInputs))
        return;

    const bool tab_pressed = IsKeyPressed(ImGuiKey_Tab, ImGuiInputFlags_Repeat, ImGuiKeyOwner_NoOwner) && !g.IO.KeyCtrl && !g.IO.KeyAlt;
    if (!tab_pressed)
        return;

    // Initiate tabbing request
    // (this is ALWAYS ENABLED, regardless of ImGuiConfigFlags_NavEnableKeyboard flag!)
    // See NavProcessItemForTabbingRequest() for a description of the various forward/backward tabbing cases with and without wrapping.
    const bool nav_keyboard_active = (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
    if (nav_keyboard_active)
        g.NavTabbingDir = g.IO.KeyShift ? -1 : (g.NavDisableHighlight == true && g.ActiveId == 0) ? 0 : +1;
    else
        g.NavTabbingDir = g.IO.KeyShift ? -1 : (g.ActiveId == 0) ? 0 : +1;
    ImGuiNavMoveFlags move_flags = ImGuiNavMoveFlags_IsTabbing | ImGuiNavMoveFlags_Activate;
    ImGuiScrollFlags scroll_flags = window->Appearing ? ImGuiScrollFlags_KeepVisibleEdgeX | ImGuiScrollFlags_AlwaysCenterY : ImGuiScrollFlags_KeepVisibleEdgeX | ImGuiScrollFlags_KeepVisibleEdgeY;
    ImGuiDir clip_dir = (g.NavTabbingDir < 0) ? ImGuiDir_Up : ImGuiDir_Down;
    NavMoveRequestSubmit(ImGuiDir_None, clip_dir, move_flags, scroll_flags); // FIXME-NAV: Once we refactor tabbing, add LegacyApi flag to not activate non-inputable.
    g.NavTabbingCounter = -1;
}

// Apply result from previous frame navigation directional move request. Always called from NavUpdate()
void ImGui::NavMoveRequestApplyResult()
{
    ImGuiContext& g = *GImGui;
#if IMGUI_DEBUG_NAV_SCORING
    if (g.NavMoveFlags & ImGuiNavMoveFlags_DebugNoResult) // [DEBUG] Scoring all items in NavWindow at all times
        return;
#endif

    // Select which result to use
    ImGuiNavItemData* result = (g.NavMoveResultLocal.ID != 0) ? &g.NavMoveResultLocal : (g.NavMoveResultOther.ID != 0) ? &g.NavMoveResultOther : NULL;

    // Tabbing forward wrap
    if ((g.NavMoveFlags & ImGuiNavMoveFlags_IsTabbing) && result == NULL)
        if ((g.NavTabbingCounter == 1 || g.NavTabbingDir == 0) && g.NavTabbingResultFirst.ID)
            result = &g.NavTabbingResultFirst;

    // In a situation when there are no results but NavId != 0, re-enable the Navigation highlight (because g.NavId is not considered as a possible result)
    const ImGuiAxis axis = (g.NavMoveDir == ImGuiDir_Up || g.NavMoveDir == ImGuiDir_Down) ? ImGuiAxis_Y : ImGuiAxis_X;
    if (result == NULL)
    {
        if (g.NavMoveFlags & ImGuiNavMoveFlags_IsTabbing)
            g.NavMoveFlags |= ImGuiNavMoveFlags_NoSetNavHighlight;
        if (g.NavId != 0 && (g.NavMoveFlags & ImGuiNavMoveFlags_NoSetNavHighlight) == 0)
            NavRestoreHighlightAfterMove();
        NavClearPreferredPosForAxis(axis); // On a failed move, clear preferred pos for this axis.
        IMGUI_DEBUG_LOG_NAV("[nav] NavMoveSubmitted but not led to a result!\n");
        return;
    }

    // PageUp/PageDown behavior first jumps to the bottom/top mostly visible item, _otherwise_ use the result from the previous/next page.
    if (g.NavMoveFlags & ImGuiNavMoveFlags_AlsoScoreVisibleSet)
        if (g.NavMoveResultLocalVisible.ID != 0 && g.NavMoveResultLocalVisible.ID != g.NavId)
            result = &g.NavMoveResultLocalVisible;

    // Maybe entering a flattened child from the outside? In this case solve the tie using the regular scoring rules.
    if (result != &g.NavMoveResultOther && g.NavMoveResultOther.ID != 0 && g.NavMoveResultOther.Window->ParentWindow == g.NavWindow)
        if ((g.NavMoveResultOther.DistBox < result->DistBox) || (g.NavMoveResultOther.DistBox == result->DistBox && g.NavMoveResultOther.DistCenter < result->DistCenter))
            result = &g.NavMoveResultOther;
    IM_ASSERT(g.NavWindow && result->Window);

    // Scroll to keep newly navigated item fully into view.
    if (g.NavLayer == ImGuiNavLayer_Main)
    {
        ImRect rect_abs = WindowRectRelToAbs(result->Window, result->RectRel);
        ScrollToRectEx(result->Window, rect_abs, g.NavMoveScrollFlags);

        if (g.NavMoveFlags & ImGuiNavMoveFlags_ScrollToEdgeY)
        {
            // FIXME: Should remove this? Or make more precise: use ScrollToRectEx() with edge?
            float scroll_target = (g.NavMoveDir == ImGuiDir_Up) ? result->Window->ScrollMax.y : 0.0f;
            SetScrollY(result->Window, scroll_target);
        }
    }

    if (g.NavWindow != result->Window)
    {
        IMGUI_DEBUG_LOG_FOCUS("[focus] NavMoveRequest: SetNavWindow(\"%s\")\n", result->Window->Name);
        g.NavWindow = result->Window;
        g.NavLastValidSelectionUserData = ImGuiSelectionUserData_Invalid;
    }

    // Clear active id unless requested not to
    // FIXME: ImGuiNavMoveFlags_NoClearActiveId is currently unused as we don't have a clear strategy to preserve active id after interaction,
    // so this is mostly provided as a gateway for further experiments (see #1418, #2890)
    if (g.ActiveId != result->ID && (g.NavMoveFlags & ImGuiNavMoveFlags_NoClearActiveId) == 0)
        ClearActiveID();

    // Don't set NavJustMovedToId if just landed on the same spot (which may happen with ImGuiNavMoveFlags_AllowCurrentNavId)
    // PageUp/PageDown however sets always set NavJustMovedTo (vs Home/End which doesn't) mimicking Windows behavior.
    if ((g.NavId != result->ID || (g.NavMoveFlags & ImGuiNavMoveFlags_IsPageMove)) && (g.NavMoveFlags & ImGuiNavMoveFlags_NoSelect) == 0)
    {
        g.NavJustMovedFromFocusScopeId = g.NavFocusScopeId;
        g.NavJustMovedToId = result->ID;
        g.NavJustMovedToFocusScopeId = result->FocusScopeId;
        g.NavJustMovedToKeyMods = g.NavMoveKeyMods;
        g.NavJustMovedToIsTabbing = (g.NavMoveFlags & ImGuiNavMoveFlags_IsTabbing) != 0;
        g.NavJustMovedToHasSelectionData = (result->InFlags & ImGuiItemFlags_HasSelectionUserData) != 0;
        //IMGUI_DEBUG_LOG_NAV("[nav] NavJustMovedFromFocusScopeId = 0x%08X, NavJustMovedToFocusScopeId = 0x%08X\n", g.NavJustMovedFromFocusScopeId, g.NavJustMovedToFocusScopeId);
    }

    // Apply new NavID/Focus
    IMGUI_DEBUG_LOG_NAV("[nav] NavMoveRequest: result NavID 0x%08X in Layer %d Window \"%s\"\n", result->ID, g.NavLayer, g.NavWindow->Name);
    ImVec2 preferred_scoring_pos_rel = g.NavWindow->RootWindowForNav->NavPreferredScoringPosRel[g.NavLayer];
    SetNavID(result->ID, g.NavLayer, result->FocusScopeId, result->RectRel);
    if (result->SelectionUserData != ImGuiSelectionUserData_Invalid)
        g.NavLastValidSelectionUserData = result->SelectionUserData;

    // Restore last preferred position for current axis
    // (storing in RootWindowForNav-> as the info is desirable at the beginning of a Move Request. In theory all storage should use RootWindowForNav..)
    if ((g.NavMoveFlags & ImGuiNavMoveFlags_IsTabbing) == 0)
    {
        preferred_scoring_pos_rel[axis] = result->RectRel.GetCenter()[axis];
        g.NavWindow->RootWindowForNav->NavPreferredScoringPosRel[g.NavLayer] = preferred_scoring_pos_rel;
    }

    // Tabbing: Activates Inputable, otherwise only Focus
    if ((g.NavMoveFlags & ImGuiNavMoveFlags_IsTabbing) && (result->InFlags & ImGuiItemFlags_Inputable) == 0)
        g.NavMoveFlags &= ~ImGuiNavMoveFlags_Activate;

    // Activate
    if (g.NavMoveFlags & ImGuiNavMoveFlags_Activate)
    {
        g.NavNextActivateId = result->ID;
        g.NavNextActivateFlags = ImGuiActivateFlags_None;
        if (g.NavMoveFlags & ImGuiNavMoveFlags_IsTabbing)
            g.NavNextActivateFlags |= ImGuiActivateFlags_PreferInput | ImGuiActivateFlags_TryToPreserveState | ImGuiActivateFlags_FromTabbing;
    }

    // Enable nav highlight
    if ((g.NavMoveFlags & ImGuiNavMoveFlags_NoSetNavHighlight) == 0)
        NavRestoreHighlightAfterMove();
}

// Process Escape/NavCancel input (to close a popup, get back to parent, clear focus)
// FIXME: In order to support e.g. Escape to clear a selection we'll need:
// - either to store the equivalent of ActiveIdUsingKeyInputMask for a FocusScope and test for it.
// - either to move most/all of those tests to the epilogue/end functions of the scope they are dealing with (e.g. exit child window in EndChild()) or in EndFrame(), to allow an earlier intercept
static void ImGui::NavUpdateCancelRequest()
{
    ImGuiContext& g = *GImGui;
    const bool nav_gamepad_active = (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (g.IO.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
    const bool nav_keyboard_active = (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
    if (!(nav_keyboard_active && IsKeyPressed(ImGuiKey_Escape, 0, ImGuiKeyOwner_NoOwner)) && !(nav_gamepad_active && IsKeyPressed(ImGuiKey_NavGamepadCancel, 0, ImGuiKeyOwner_NoOwner)))
        return;

    IMGUI_DEBUG_LOG_NAV("[nav] NavUpdateCancelRequest()\n");
    if (g.ActiveId != 0)
    {
        ClearActiveID();
    }
    else if (g.NavLayer != ImGuiNavLayer_Main)
    {
        // Leave the "menu" layer
        NavRestoreLayer(ImGuiNavLayer_Main);
        NavRestoreHighlightAfterMove();
    }
    else if (g.NavWindow && g.NavWindow != g.NavWindow->RootWindow && !(g.NavWindow->RootWindowForNav->Flags & ImGuiWindowFlags_Popup) && g.NavWindow->RootWindowForNav->ParentWindow)
    {
        // Exit child window
        ImGuiWindow* child_window = g.NavWindow->RootWindowForNav;
        ImGuiWindow* parent_window = child_window->ParentWindow;
        IM_ASSERT(child_window->ChildId != 0);
        FocusWindow(parent_window);
        SetNavID(child_window->ChildId, ImGuiNavLayer_Main, 0, WindowRectAbsToRel(parent_window, child_window->Rect()));
        NavRestoreHighlightAfterMove();
    }
    else if (g.OpenPopupStack.Size > 0 && g.OpenPopupStack.back().Window != NULL && !(g.OpenPopupStack.back().Window->Flags & ImGuiWindowFlags_Modal))
    {
        // Close open popup/menu
        ClosePopupToLevel(g.OpenPopupStack.Size - 1, true);
    }
    else
    {
        // Clear NavLastId for popups but keep it for regular child window so we can leave one and come back where we were
        // FIXME-NAV: This should happen on window appearing.
        if (g.NavWindow && ((g.NavWindow->Flags & ImGuiWindowFlags_Popup)))// || !(g.NavWindow->Flags & ImGuiWindowFlags_ChildWindow)))
            g.NavWindow->NavLastIds[0] = 0;

        // Clear nav focus
        g.NavId = 0;
        if (g.IO.ConfigNavEscapeClearFocusWindow)
            FocusWindow(NULL);
    }
}

// Handle PageUp/PageDown/Home/End keys
// Called from NavUpdateCreateMoveRequest() which will use our output to create a move request
// FIXME-NAV: This doesn't work properly with NavFlattened siblings as we use NavWindow rectangle for reference
// FIXME-NAV: how to get Home/End to aim at the beginning/end of a 2D grid?
static float ImGui::NavUpdatePageUpPageDown()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.NavWindow;
    if ((window->Flags & ImGuiWindowFlags_NoNavInputs) || g.NavWindowingTarget != NULL)
        return 0.0f;

    const bool page_up_held = IsKeyDown(ImGuiKey_PageUp, ImGuiKeyOwner_NoOwner);
    const bool page_down_held = IsKeyDown(ImGuiKey_PageDown, ImGuiKeyOwner_NoOwner);
    const bool home_pressed = IsKeyPressed(ImGuiKey_Home, ImGuiInputFlags_Repeat, ImGuiKeyOwner_NoOwner);
    const bool end_pressed = IsKeyPressed(ImGuiKey_End, ImGuiInputFlags_Repeat, ImGuiKeyOwner_NoOwner);
    if (page_up_held == page_down_held && home_pressed == end_pressed) // Proceed if either (not both) are pressed, otherwise early out
        return 0.0f;

    if (g.NavLayer != ImGuiNavLayer_Main)
        NavRestoreLayer(ImGuiNavLayer_Main);

    if (window->DC.NavLayersActiveMask == 0x00 && window->DC.NavWindowHasScrollY)
    {
        // Fallback manual-scroll when window has no navigable item
        if (IsKeyPressed(ImGuiKey_PageUp, ImGuiInputFlags_Repeat, ImGuiKeyOwner_NoOwner))
            SetScrollY(window, window->Scroll.y - window->InnerRect.GetHeight());
        else if (IsKeyPressed(ImGuiKey_PageDown, ImGuiInputFlags_Repeat, ImGuiKeyOwner_NoOwner))
            SetScrollY(window, window->Scroll.y + window->InnerRect.GetHeight());
        else if (home_pressed)
            SetScrollY(window, 0.0f);
        else if (end_pressed)
            SetScrollY(window, window->ScrollMax.y);
    }
    else
    {
        ImRect& nav_rect_rel = window->NavRectRel[g.NavLayer];
        const float page_offset_y = ImMax(0.0f, window->InnerRect.GetHeight() - window->CalcFontSize() * 1.0f + nav_rect_rel.GetHeight());
        float nav_scoring_rect_offset_y = 0.0f;
        if (IsKeyPressed(ImGuiKey_PageUp, true))
        {
            nav_scoring_rect_offset_y = -page_offset_y;
            g.NavMoveDir = ImGuiDir_Down; // Because our scoring rect is offset up, we request the down direction (so we can always land on the last item)
            g.NavMoveClipDir = ImGuiDir_Up;
            g.NavMoveFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_AlsoScoreVisibleSet | ImGuiNavMoveFlags_IsPageMove;
        }
        else if (IsKeyPressed(ImGuiKey_PageDown, true))
        {
            nav_scoring_rect_offset_y = +page_offset_y;
            g.NavMoveDir = ImGuiDir_Up; // Because our scoring rect is offset down, we request the up direction (so we can always land on the last item)
            g.NavMoveClipDir = ImGuiDir_Down;
            g.NavMoveFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_AlsoScoreVisibleSet | ImGuiNavMoveFlags_IsPageMove;
        }
        else if (home_pressed)
        {
            // FIXME-NAV: handling of Home/End is assuming that the top/bottom most item will be visible with Scroll.y == 0/ScrollMax.y
            // Scrolling will be handled via the ImGuiNavMoveFlags_ScrollToEdgeY flag, we don't scroll immediately to avoid scrolling happening before nav result.
            // Preserve current horizontal position if we have any.
            nav_rect_rel.Min.y = nav_rect_rel.Max.y = 0.0f;
            if (nav_rect_rel.IsInverted())
                nav_rect_rel.Min.x = nav_rect_rel.Max.x = 0.0f;
            g.NavMoveDir = ImGuiDir_Down;
            g.NavMoveFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_ScrollToEdgeY;
            // FIXME-NAV: MoveClipDir left to _None, intentional?
        }
        else if (end_pressed)
        {
            nav_rect_rel.Min.y = nav_rect_rel.Max.y = window->ContentSize.y;
            if (nav_rect_rel.IsInverted())
                nav_rect_rel.Min.x = nav_rect_rel.Max.x = 0.0f;
            g.NavMoveDir = ImGuiDir_Up;
            g.NavMoveFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_ScrollToEdgeY;
            // FIXME-NAV: MoveClipDir left to _None, intentional?
        }
        return nav_scoring_rect_offset_y;
    }
    return 0.0f;
}

static void ImGui::NavEndFrame()
{
    ImGuiContext& g = *GImGui;

    // Show CTRL+TAB list window
    if (g.NavWindowingTarget != NULL)
        NavUpdateWindowingOverlay();

    // Perform wrap-around in menus
    // FIXME-NAV: Wrap may need to apply a weight bias on the other axis. e.g. 4x4 grid with 2 last items missing on last item won't handle LoopY/WrapY correctly.
    // FIXME-NAV: Wrap (not Loop) support could be handled by the scoring function and then WrapX would function without an extra frame.
    if (g.NavWindow && NavMoveRequestButNoResultYet() && (g.NavMoveFlags & ImGuiNavMoveFlags_WrapMask_) && (g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded) == 0)
        NavUpdateCreateWrappingRequest();
}

static void ImGui::NavUpdateCreateWrappingRequest()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.NavWindow;

    bool do_forward = false;
    ImRect bb_rel = window->NavRectRel[g.NavLayer];
    ImGuiDir clip_dir = g.NavMoveDir;

    const ImGuiNavMoveFlags move_flags = g.NavMoveFlags;
    //const ImGuiAxis move_axis = (g.NavMoveDir == ImGuiDir_Up || g.NavMoveDir == ImGuiDir_Down) ? ImGuiAxis_Y : ImGuiAxis_X;
    if (g.NavMoveDir == ImGuiDir_Left && (move_flags & (ImGuiNavMoveFlags_WrapX | ImGuiNavMoveFlags_LoopX)))
    {
        bb_rel.Min.x = bb_rel.Max.x = window->ContentSize.x + window->WindowPadding.x;
        if (move_flags & ImGuiNavMoveFlags_WrapX)
        {
            bb_rel.TranslateY(-bb_rel.GetHeight()); // Previous row
            clip_dir = ImGuiDir_Up;
        }
        do_forward = true;
    }
    if (g.NavMoveDir == ImGuiDir_Right && (move_flags & (ImGuiNavMoveFlags_WrapX | ImGuiNavMoveFlags_LoopX)))
    {
        bb_rel.Min.x = bb_rel.Max.x = -window->WindowPadding.x;
        if (move_flags & ImGuiNavMoveFlags_WrapX)
        {
            bb_rel.TranslateY(+bb_rel.GetHeight()); // Next row
            clip_dir = ImGuiDir_Down;
        }
        do_forward = true;
    }
    if (g.NavMoveDir == ImGuiDir_Up && (move_flags & (ImGuiNavMoveFlags_WrapY | ImGuiNavMoveFlags_LoopY)))
    {
        bb_rel.Min.y = bb_rel.Max.y = window->ContentSize.y + window->WindowPadding.y;
        if (move_flags & ImGuiNavMoveFlags_WrapY)
        {
            bb_rel.TranslateX(-bb_rel.GetWidth()); // Previous column
            clip_dir = ImGuiDir_Left;
        }
        do_forward = true;
    }
    if (g.NavMoveDir == ImGuiDir_Down && (move_flags & (ImGuiNavMoveFlags_WrapY | ImGuiNavMoveFlags_LoopY)))
    {
        bb_rel.Min.y = bb_rel.Max.y = -window->WindowPadding.y;
        if (move_flags & ImGuiNavMoveFlags_WrapY)
        {
            bb_rel.TranslateX(+bb_rel.GetWidth()); // Next column
            clip_dir = ImGuiDir_Right;
        }
        do_forward = true;
    }
    if (!do_forward)
        return;
    window->NavRectRel[g.NavLayer] = bb_rel;
    NavClearPreferredPosForAxis(ImGuiAxis_X);
    NavClearPreferredPosForAxis(ImGuiAxis_Y);
    NavMoveRequestForward(g.NavMoveDir, clip_dir, move_flags, g.NavMoveScrollFlags);
}

static int ImGui::FindWindowFocusIndex(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_UNUSED(g);
    int order = window->FocusOrder;
    IM_ASSERT(window->RootWindow == window); // No child window (not testing _ChildWindow because of docking)
    IM_ASSERT(g.WindowsFocusOrder[order] == window);
    return order;
}

static ImGuiWindow* FindWindowNavFocusable(int i_start, int i_stop, int dir) // FIXME-OPT O(N)
{
    ImGuiContext& g = *GImGui;
    for (int i = i_start; i >= 0 && i < g.WindowsFocusOrder.Size && i != i_stop; i += dir)
        if (ImGui::IsWindowNavFocusable(g.WindowsFocusOrder[i]))
            return g.WindowsFocusOrder[i];
    return NULL;
}

static void NavUpdateWindowingTarget(int focus_change_dir)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindowingTarget);
    if (g.NavWindowingTarget->Flags & ImGuiWindowFlags_Modal)
        return;

    const int i_current = ImGui::FindWindowFocusIndex(g.NavWindowingTarget);
    ImGuiWindow* window_target = FindWindowNavFocusable(i_current + focus_change_dir, -INT_MAX, focus_change_dir);
    if (!window_target)
        window_target = FindWindowNavFocusable((focus_change_dir < 0) ? (g.WindowsFocusOrder.Size - 1) : 0, i_current, focus_change_dir);
    if (window_target) // Don't reset windowing target if there's a single window in the list
    {
        g.NavWindowingTarget = g.NavWindowingTargetAnim = window_target;
        g.NavWindowingAccumDeltaPos = g.NavWindowingAccumDeltaSize = ImVec2(0.0f, 0.0f);
    }
    g.NavWindowingToggleLayer = false;
}

// Windowing management mode
// Keyboard: CTRL+Tab (change focus/move/resize), Alt (toggle menu layer)
// Gamepad:  Hold Menu/Square (change focus/move/resize), Tap Menu/Square (toggle menu layer)
static void ImGui::NavUpdateWindowing()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    ImGuiWindow* apply_focus_window = NULL;
    bool apply_toggle_layer = false;

    ImGuiWindow* modal_window = GetTopMostPopupModal();
    bool allow_windowing = (modal_window == NULL); // FIXME: This prevent CTRL+TAB from being usable with windows that are inside the Begin-stack of that modal.
    if (!allow_windowing)
        g.NavWindowingTarget = NULL;

    // Fade out
    if (g.NavWindowingTargetAnim && g.NavWindowingTarget == NULL)
    {
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha - io.DeltaTime * 10.0f, 0.0f);
        if (g.DimBgRatio <= 0.0f && g.NavWindowingHighlightAlpha <= 0.0f)
            g.NavWindowingTargetAnim = NULL;
    }

    // Start CTRL+Tab or Square+L/R window selection
    // (g.ConfigNavWindowingKeyNext/g.ConfigNavWindowingKeyPrev defaults are ImGuiMod_Ctrl|ImGuiKey_Tab and ImGuiMod_Ctrl|ImGuiMod_Shift|ImGuiKey_Tab)
    const ImGuiID owner_id = ImHashStr("###NavUpdateWindowing");
    const bool nav_gamepad_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
    const bool nav_keyboard_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
    const bool keyboard_next_window = allow_windowing && g.ConfigNavWindowingKeyNext && Shortcut(g.ConfigNavWindowingKeyNext, ImGuiInputFlags_Repeat | ImGuiInputFlags_RouteAlways, owner_id);
    const bool keyboard_prev_window = allow_windowing && g.ConfigNavWindowingKeyPrev && Shortcut(g.ConfigNavWindowingKeyPrev, ImGuiInputFlags_Repeat | ImGuiInputFlags_RouteAlways, owner_id);
    const bool start_windowing_with_gamepad = allow_windowing && nav_gamepad_active && !g.NavWindowingTarget && IsKeyPressed(ImGuiKey_NavGamepadMenu, ImGuiInputFlags_None);
    const bool start_windowing_with_keyboard = allow_windowing && !g.NavWindowingTarget && (keyboard_next_window || keyboard_prev_window); // Note: enabled even without NavEnableKeyboard!
    bool just_started_windowing_from_null_focus = false;
    if (start_windowing_with_gamepad || start_windowing_with_keyboard)
        if (ImGuiWindow* window = g.NavWindow ? g.NavWindow : FindWindowNavFocusable(g.WindowsFocusOrder.Size - 1, -INT_MAX, -1))
        {
            g.NavWindowingTarget = g.NavWindowingTargetAnim = window->RootWindow; // Current location
            g.NavWindowingTimer = g.NavWindowingHighlightAlpha = 0.0f;
            g.NavWindowingAccumDeltaPos = g.NavWindowingAccumDeltaSize = ImVec2(0.0f, 0.0f);
            g.NavWindowingToggleLayer = start_windowing_with_gamepad ? true : false; // Gamepad starts toggling layer
            g.NavInputSource = start_windowing_with_keyboard ? ImGuiInputSource_Keyboard : ImGuiInputSource_Gamepad;
            if (g.NavWindow == NULL)
                just_started_windowing_from_null_focus = true;

            // Manually register ownership of our mods. Using a global route in the Shortcut() calls instead would probably be correct but may have more side-effects.
            if (keyboard_next_window || keyboard_prev_window)
                SetKeyOwnersForKeyChord((g.ConfigNavWindowingKeyNext | g.ConfigNavWindowingKeyPrev) & ImGuiMod_Mask_, owner_id);
        }

    // Gamepad update
    g.NavWindowingTimer += io.DeltaTime;
    if (g.NavWindowingTarget && g.NavInputSource == ImGuiInputSource_Gamepad)
    {
        // Highlight only appears after a brief time holding the button, so that a fast tap on PadMenu (to toggle NavLayer) doesn't add visual noise
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha, ImSaturate((g.NavWindowingTimer - NAV_WINDOWING_HIGHLIGHT_DELAY) / 0.05f));

        // Select window to focus
        const int focus_change_dir = (int)IsKeyPressed(ImGuiKey_GamepadL1) - (int)IsKeyPressed(ImGuiKey_GamepadR1);
        if (focus_change_dir != 0 && !just_started_windowing_from_null_focus)
        {
            NavUpdateWindowingTarget(focus_change_dir);
            g.NavWindowingHighlightAlpha = 1.0f;
        }

        // Single press toggles NavLayer, long press with L/R apply actual focus on release (until then the window was merely rendered top-most)
        if (!IsKeyDown(ImGuiKey_NavGamepadMenu))
        {
            g.NavWindowingToggleLayer &= (g.NavWindowingHighlightAlpha < 1.0f); // Once button was held long enough we don't consider it a tap-to-toggle-layer press anymore.
            if (g.NavWindowingToggleLayer && g.NavWindow)
                apply_toggle_layer = true;
            else if (!g.NavWindowingToggleLayer)
                apply_focus_window = g.NavWindowingTarget;
            g.NavWindowingTarget = NULL;
        }
    }

    // Keyboard: Focus
    if (g.NavWindowingTarget && g.NavInputSource == ImGuiInputSource_Keyboard)
    {
        // Visuals only appears after a brief time after pressing TAB the first time, so that a fast CTRL+TAB doesn't add visual noise
        ImGuiKeyChord shared_mods = ((g.ConfigNavWindowingKeyNext ? g.ConfigNavWindowingKeyNext : ImGuiMod_Mask_) & (g.ConfigNavWindowingKeyPrev ? g.ConfigNavWindowingKeyPrev : ImGuiMod_Mask_)) & ImGuiMod_Mask_;
        IM_ASSERT(shared_mods != 0); // Next/Prev shortcut currently needs a shared modifier to "hold", otherwise Prev actions would keep cycling between two windows.
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha, ImSaturate((g.NavWindowingTimer - NAV_WINDOWING_HIGHLIGHT_DELAY) / 0.05f)); // 1.0f
        if ((keyboard_next_window || keyboard_prev_window) && !just_started_windowing_from_null_focus)
            NavUpdateWindowingTarget(keyboard_next_window ? -1 : +1);
        else if ((io.KeyMods & shared_mods) != shared_mods)
            apply_focus_window = g.NavWindowingTarget;
    }

    // Keyboard: Press and Release ALT to toggle menu layer
    const ImGuiKey windowing_toggle_keys[] = { ImGuiKey_LeftAlt, ImGuiKey_RightAlt };
    bool windowing_toggle_layer_start = false;
    for (ImGuiKey windowing_toggle_key : windowing_toggle_keys)
        if (nav_keyboard_active && IsKeyPressed(windowing_toggle_key, 0, ImGuiKeyOwner_NoOwner))
        {
            windowing_toggle_layer_start = true;
            g.NavWindowingToggleLayer = true;
            g.NavWindowingToggleKey = windowing_toggle_key;
            g.NavInputSource = ImGuiInputSource_Keyboard;
            break;
        }
    if (g.NavWindowingToggleLayer && g.NavInputSource == ImGuiInputSource_Keyboard)
    {
        // We cancel toggling nav layer when any text has been typed (generally while holding Alt). (See #370)
        // We cancel toggling nav layer when other modifiers are pressed. (See #4439)
        // - AltGR is Alt+Ctrl on some layout but we can't reliably detect it (not all backends/systems/layout emit it as Alt+Ctrl).
        // We cancel toggling nav layer if an owner has claimed the key.
        if (io.InputQueueCharacters.Size > 0 || io.KeyCtrl || io.KeyShift || io.KeySuper)
            g.NavWindowingToggleLayer = false;
        else if (windowing_toggle_layer_start == false && g.LastKeyboardKeyPressTime == g.Time)
            g.NavWindowingToggleLayer = false;
        else if (TestKeyOwner(g.NavWindowingToggleKey, ImGuiKeyOwner_NoOwner) == false || TestKeyOwner(ImGuiMod_Alt, ImGuiKeyOwner_NoOwner) == false)
            g.NavWindowingToggleLayer = false;

        // Apply layer toggle on Alt release
        // Important: as before version <18314 we lacked an explicit IO event for focus gain/loss, we also compare mouse validity to detect old backends clearing mouse pos on focus loss.
        if (IsKeyReleased(g.NavWindowingToggleKey) && g.NavWindowingToggleLayer)
            if (g.ActiveId == 0 || g.ActiveIdAllowOverlap)
                if (IsMousePosValid(&io.MousePos) == IsMousePosValid(&io.MousePosPrev))
                    apply_toggle_layer = true;
        if (!IsKeyDown(g.NavWindowingToggleKey))
            g.NavWindowingToggleLayer = false;
    }

    // Move window
    if (g.NavWindowingTarget && !(g.NavWindowingTarget->Flags & ImGuiWindowFlags_NoMove))
    {
        ImVec2 nav_move_dir;
        if (g.NavInputSource == ImGuiInputSource_Keyboard && !io.KeyShift)
            nav_move_dir = GetKeyMagnitude2d(ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_UpArrow, ImGuiKey_DownArrow);
        if (g.NavInputSource == ImGuiInputSource_Gamepad)
            nav_move_dir = GetKeyMagnitude2d(ImGuiKey_GamepadLStickLeft, ImGuiKey_GamepadLStickRight, ImGuiKey_GamepadLStickUp, ImGuiKey_GamepadLStickDown);
        if (nav_move_dir.x != 0.0f || nav_move_dir.y != 0.0f)
        {
            const float NAV_MOVE_SPEED = 800.0f;
            const float move_step = NAV_MOVE_SPEED * io.DeltaTime * ImMin(io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
            g.NavWindowingAccumDeltaPos += nav_move_dir * move_step;
            g.NavDisableMouseHover = true;
            ImVec2 accum_floored = ImTrunc(g.NavWindowingAccumDeltaPos);
            if (accum_floored.x != 0.0f || accum_floored.y != 0.0f)
            {
                ImGuiWindow* moving_window = g.NavWindowingTarget->RootWindow;
                SetWindowPos(moving_window, moving_window->Pos + accum_floored, ImGuiCond_Always);
                g.NavWindowingAccumDeltaPos -= accum_floored;
            }
        }
    }

    // Apply final focus
    if (apply_focus_window && (g.NavWindow == NULL || apply_focus_window != g.NavWindow->RootWindow))
    {
        ClearActiveID();
        NavRestoreHighlightAfterMove();
        ClosePopupsOverWindow(apply_focus_window, false);
        FocusWindow(apply_focus_window, ImGuiFocusRequestFlags_RestoreFocusedChild);
        apply_focus_window = g.NavWindow;
        if (apply_focus_window->NavLastIds[0] == 0)
            NavInitWindow(apply_focus_window, false);

        // If the window has ONLY a menu layer (no main layer), select it directly
        // Use NavLayersActiveMaskNext since windows didn't have a chance to be Begin()-ed on this frame,
        // so CTRL+Tab where the keys are only held for 1 frame will be able to use correct layers mask since
        // the target window as already been previewed once.
        // FIXME-NAV: This should be done in NavInit.. or in FocusWindow... However in both of those cases,
        // we won't have a guarantee that windows has been visible before and therefore NavLayersActiveMask*
        // won't be valid.
        if (apply_focus_window->DC.NavLayersActiveMaskNext == (1 << ImGuiNavLayer_Menu))
            g.NavLayer = ImGuiNavLayer_Menu;
    }
    if (apply_focus_window)
        g.NavWindowingTarget = NULL;

    // Apply menu/layer toggle
    if (apply_toggle_layer && g.NavWindow)
    {
        ClearActiveID();

        // Move to parent menu if necessary
        ImGuiWindow* new_nav_window = g.NavWindow;
        while (new_nav_window->ParentWindow
            && (new_nav_window->DC.NavLayersActiveMask & (1 << ImGuiNavLayer_Menu)) == 0
            && (new_nav_window->Flags & ImGuiWindowFlags_ChildWindow) != 0
            && (new_nav_window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu)) == 0)
            new_nav_window = new_nav_window->ParentWindow;
        if (new_nav_window != g.NavWindow)
        {
            ImGuiWindow* old_nav_window = g.NavWindow;
            FocusWindow(new_nav_window);
            new_nav_window->NavLastChildNavWindow = old_nav_window;
        }

        // Toggle layer
        const ImGuiNavLayer new_nav_layer = (g.NavWindow->DC.NavLayersActiveMask & (1 << ImGuiNavLayer_Menu)) ? (ImGuiNavLayer)((int)g.NavLayer ^ 1) : ImGuiNavLayer_Main;
        if (new_nav_layer != g.NavLayer)
        {
            // Reinitialize navigation when entering menu bar with the Alt key (FIXME: could be a properly of the layer?)
            if (new_nav_layer == ImGuiNavLayer_Menu)
                g.NavWindow->NavLastIds[new_nav_layer] = 0;
            NavRestoreLayer(new_nav_layer);
            NavRestoreHighlightAfterMove();
        }
    }
}

// Window has already passed the IsWindowNavFocusable()
static const char* GetFallbackWindowNameForWindowingList(ImGuiWindow* window)
{
    if (window->Flags & ImGuiWindowFlags_Popup)
        return ImGui::LocalizeGetMsg(ImGuiLocKey_WindowingPopup);
    if ((window->Flags & ImGuiWindowFlags_MenuBar) && strcmp(window->Name, "##MainMenuBar") == 0)
        return ImGui::LocalizeGetMsg(ImGuiLocKey_WindowingMainMenuBar);
    return ImGui::LocalizeGetMsg(ImGuiLocKey_WindowingUntitled);
}

// Overlay displayed when using CTRL+TAB. Called by EndFrame().
void ImGui::NavUpdateWindowingOverlay()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindowingTarget != NULL);

    if (g.NavWindowingTimer < NAV_WINDOWING_LIST_APPEAR_DELAY)
        return;

    if (g.NavWindowingListWindow == NULL)
        g.NavWindowingListWindow = FindWindowByName("###NavWindowingList");
    const ImGuiViewport* viewport = GetMainViewport();
    SetNextWindowSizeConstraints(ImVec2(viewport->Size.x * 0.20f, viewport->Size.y * 0.20f), ImVec2(FLT_MAX, FLT_MAX));
    SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    PushStyleVar(ImGuiStyleVar_WindowPadding, g.Style.WindowPadding * 2.0f);
    Begin("###NavWindowingList", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    if (g.ContextName[0] != 0)
        SeparatorText(g.ContextName);
    for (int n = g.WindowsFocusOrder.Size - 1; n >= 0; n--)
    {
        ImGuiWindow* window = g.WindowsFocusOrder[n];
        IM_ASSERT(window != NULL); // Fix static analyzers
        if (!IsWindowNavFocusable(window))
            continue;
        const char* label = window->Name;
        if (label == FindRenderedTextEnd(label))
            label = GetFallbackWindowNameForWindowingList(window);
        Selectable(label, g.NavWindowingTarget == window);
    }
    End();
    PopStyleVar();
}


//-----------------------------------------------------------------------------
// [SECTION] DRAG AND DROP
//-----------------------------------------------------------------------------

bool ImGui::IsDragDropActive()
{
    ImGuiContext& g = *GImGui;
    return g.DragDropActive;
}

void ImGui::ClearDragDrop()
{
    ImGuiContext& g = *GImGui;
    if (g.DragDropActive)
        IMGUI_DEBUG_LOG_ACTIVEID("[dragdrop] ClearDragDrop()\n");
    g.DragDropActive = false;
    g.DragDropPayload.Clear();
    g.DragDropAcceptFlags = ImGuiDragDropFlags_None;
    g.DragDropAcceptIdCurr = g.DragDropAcceptIdPrev = 0;
    g.DragDropAcceptIdCurrRectSurface = FLT_MAX;
    g.DragDropAcceptFrameCount = -1;

    g.DragDropPayloadBufHeap.clear();
    memset(&g.DragDropPayloadBufLocal, 0, sizeof(g.DragDropPayloadBufLocal));
}

bool ImGui::BeginTooltipHidden()
{
    ImGuiContext& g = *GImGui;
    bool ret = Begin("##Tooltip_Hidden", NULL, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
    SetWindowHiddenAndSkipItemsForCurrentFrame(g.CurrentWindow);
    return ret;
}

// When this returns true you need to: a) call SetDragDropPayload() exactly once, b) you may render the payload visual/description, c) call EndDragDropSource()
// If the item has an identifier:
// - This assume/require the item to be activated (typically via ButtonBehavior).
// - Therefore if you want to use this with a mouse button other than left mouse button, it is up to the item itself to activate with another button.
// - We then pull and use the mouse button that was used to activate the item and use it to carry on the drag.
// If the item has no identifier:
// - Currently always assume left mouse button.
bool ImGui::BeginDragDropSource(ImGuiDragDropFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // FIXME-DRAGDROP: While in the common-most "drag from non-zero active id" case we can tell the mouse button,
    // in both SourceExtern and id==0 cases we may requires something else (explicit flags or some heuristic).
    ImGuiMouseButton mouse_button = ImGuiMouseButton_Left;

    bool source_drag_active = false;
    ImGuiID source_id = 0;
    ImGuiID source_parent_id = 0;
    if ((flags & ImGuiDragDropFlags_SourceExtern) == 0)
    {
        source_id = g.LastItemData.ID;
        if (source_id != 0)
        {
            // Common path: items with ID
            if (g.ActiveId != source_id)
                return false;
            if (g.ActiveIdMouseButton != -1)
                mouse_button = g.ActiveIdMouseButton;
            if (g.IO.MouseDown[mouse_button] == false || window->SkipItems)
                return false;
            g.ActiveIdAllowOverlap = false;
        }
        else
        {
            // Uncommon path: items without ID
            if (g.IO.MouseDown[mouse_button] == false || window->SkipItems)
                return false;
            if ((g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredRect) == 0 && (g.ActiveId == 0 || g.ActiveIdWindow != window))
                return false;

            // If you want to use BeginDragDropSource() on an item with no unique identifier for interaction, such as Text() or Image(), you need to:
            // A) Read the explanation below, B) Use the ImGuiDragDropFlags_SourceAllowNullID flag.
            if (!(flags & ImGuiDragDropFlags_SourceAllowNullID))
            {
                IM_ASSERT(0);
                return false;
            }

            // Magic fallback to handle items with no assigned ID, e.g. Text(), Image()
            // We build a throwaway ID based on current ID stack + relative AABB of items in window.
            // THE IDENTIFIER WON'T SURVIVE ANY REPOSITIONING/RESIZINGG OF THE WIDGET, so if your widget moves your dragging operation will be canceled.
            // We don't need to maintain/call ClearActiveID() as releasing the button will early out this function and trigger !ActiveIdIsAlive.
            // Rely on keeping other window->LastItemXXX fields intact.
            source_id = g.LastItemData.ID = window->GetIDFromRectangle(g.LastItemData.Rect);
            KeepAliveID(source_id);
            bool is_hovered = ItemHoverable(g.LastItemData.Rect, source_id, g.LastItemData.InFlags);
            if (is_hovered && g.IO.MouseClicked[mouse_button])
            {
                SetActiveID(source_id, window);
                FocusWindow(window);
            }
            if (g.ActiveId == source_id) // Allow the underlying widget to display/return hovered during the mouse release frame, else we would get a flicker.
                g.ActiveIdAllowOverlap = is_hovered;
        }
        if (g.ActiveId != source_id)
            return false;
        source_parent_id = window->IDStack.back();
        source_drag_active = IsMouseDragging(mouse_button);

        // Disable navigation and key inputs while dragging + cancel existing request if any
        SetActiveIdUsingAllKeyboardKeys();
    }
    else
    {
        // When ImGuiDragDropFlags_SourceExtern is set:
        window = NULL;
        source_id = ImHashStr("#SourceExtern");
        source_drag_active = true;
        mouse_button = g.IO.MouseDown[0] ? 0 : -1;
        KeepAliveID(source_id);
        SetActiveID(source_id, NULL);
    }

    IM_ASSERT(g.DragDropWithinTarget == false); // Can't nest BeginDragDropSource() and BeginDragDropTarget()
    if (!source_drag_active)
        return false;

    // Activate drag and drop
    if (!g.DragDropActive)
    {
        IM_ASSERT(source_id != 0);
        ClearDragDrop();
        IMGUI_DEBUG_LOG_ACTIVEID("[dragdrop] BeginDragDropSource() DragDropActive = true, source_id = 0x%08X%s\n",
            source_id, (flags & ImGuiDragDropFlags_SourceExtern) ? " (EXTERN)" : "");
        ImGuiPayload& payload = g.DragDropPayload;
        payload.SourceId = source_id;
        payload.SourceParentId = source_parent_id;
        g.DragDropActive = true;
        g.DragDropSourceFlags = flags;
        g.DragDropMouseButton = mouse_button;
        if (payload.SourceId == g.ActiveId)
            g.ActiveIdNoClearOnFocusLoss = true;
    }
    g.DragDropSourceFrameCount = g.FrameCount;
    g.DragDropWithinSource = true;

    if (!(flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
    {
        // Target can request the Source to not display its tooltip (we use a dedicated flag to make this request explicit)
        // We unfortunately can't just modify the source flags and skip the call to BeginTooltip, as caller may be emitting contents.
        bool ret;
        if (g.DragDropAcceptIdPrev && (g.DragDropAcceptFlags & ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            ret = BeginTooltipHidden();
        else
            ret = BeginTooltip();
        IM_ASSERT(ret); // FIXME-NEWBEGIN: If this ever becomes false, we need to Begin("##Hidden", NULL, ImGuiWindowFlags_NoSavedSettings) + SetWindowHiddendAndSkipItemsForCurrentFrame().
        IM_UNUSED(ret);
    }

    if (!(flags & ImGuiDragDropFlags_SourceNoDisableHover) && !(flags & ImGuiDragDropFlags_SourceExtern))
        g.LastItemData.StatusFlags &= ~ImGuiItemStatusFlags_HoveredRect;

    return true;
}

void ImGui::EndDragDropSource()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.DragDropActive);
    IM_ASSERT(g.DragDropWithinSource && "Not after a BeginDragDropSource()?");

    if (!(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
        EndTooltip();

    // Discard the drag if have not called SetDragDropPayload()
    if (g.DragDropPayload.DataFrameCount == -1)
        ClearDragDrop();
    g.DragDropWithinSource = false;
}

// Use 'cond' to choose to submit payload on drag start or every frame
bool ImGui::SetDragDropPayload(const char* type, const void* data, size_t data_size, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    ImGuiPayload& payload = g.DragDropPayload;
    if (cond == 0)
        cond = ImGuiCond_Always;

    IM_ASSERT(type != NULL);
    IM_ASSERT(strlen(type) < IM_ARRAYSIZE(payload.DataType) && "Payload type can be at most 32 characters long");
    IM_ASSERT((data != NULL && data_size > 0) || (data == NULL && data_size == 0));
    IM_ASSERT(cond == ImGuiCond_Always || cond == ImGuiCond_Once);
    IM_ASSERT(payload.SourceId != 0); // Not called between BeginDragDropSource() and EndDragDropSource()

    if (cond == ImGuiCond_Always || payload.DataFrameCount == -1)
    {
        // Copy payload
        ImStrncpy(payload.DataType, type, IM_ARRAYSIZE(payload.DataType));
        g.DragDropPayloadBufHeap.resize(0);
        if (data_size > sizeof(g.DragDropPayloadBufLocal))
        {
            // Store in heap
            g.DragDropPayloadBufHeap.resize((int)data_size);
            payload.Data = g.DragDropPayloadBufHeap.Data;
            memcpy(payload.Data, data, data_size);
        }
        else if (data_size > 0)
        {
            // Store locally
            memset(&g.DragDropPayloadBufLocal, 0, sizeof(g.DragDropPayloadBufLocal));
            payload.Data = g.DragDropPayloadBufLocal;
            memcpy(payload.Data, data, data_size);
        }
        else
        {
            payload.Data = NULL;
        }
        payload.DataSize = (int)data_size;
    }
    payload.DataFrameCount = g.FrameCount;

    // Return whether the payload has been accepted
    return (g.DragDropAcceptFrameCount == g.FrameCount) || (g.DragDropAcceptFrameCount == g.FrameCount - 1);
}

bool ImGui::BeginDragDropTargetCustom(const ImRect& bb, ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (!g.DragDropActive)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    ImGuiWindow* hovered_window = g.HoveredWindowUnderMovingWindow;
    if (hovered_window == NULL || window->RootWindow != hovered_window->RootWindow)
        return false;
    IM_ASSERT(id != 0);
    if (!IsMouseHoveringRect(bb.Min, bb.Max) || (id == g.DragDropPayload.SourceId))
        return false;
    if (window->SkipItems)
        return false;

    IM_ASSERT(g.DragDropWithinTarget == false && g.DragDropWithinSource == false); // Can't nest BeginDragDropSource() and BeginDragDropTarget()
    g.DragDropTargetRect = bb;
    g.DragDropTargetClipRect = window->ClipRect; // May want to be overridden by user depending on use case?
    g.DragDropTargetId = id;
    g.DragDropWithinTarget = true;
    return true;
}

// We don't use BeginDragDropTargetCustom() and duplicate its code because:
// 1) we use LastItemData's ImGuiItemStatusFlags_HoveredRect which handles items that push a temporarily clip rectangle in their code. Calling BeginDragDropTargetCustom(LastItemRect) would not handle them.
// 2) and it's faster. as this code may be very frequently called, we want to early out as fast as we can.
// Also note how the HoveredWindow test is positioned differently in both functions (in both functions we optimize for the cheapest early out case)
bool ImGui::BeginDragDropTarget()
{
    ImGuiContext& g = *GImGui;
    if (!g.DragDropActive)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    if (!(g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredRect))
        return false;
    ImGuiWindow* hovered_window = g.HoveredWindowUnderMovingWindow;
    if (hovered_window == NULL || window->RootWindow != hovered_window->RootWindow || window->SkipItems)
        return false;

    const ImRect& display_rect = (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HasDisplayRect) ? g.LastItemData.DisplayRect : g.LastItemData.Rect;
    ImGuiID id = g.LastItemData.ID;
    if (id == 0)
    {
        id = window->GetIDFromRectangle(display_rect);
        KeepAliveID(id);
    }
    if (g.DragDropPayload.SourceId == id)
        return false;

    IM_ASSERT(g.DragDropWithinTarget == false && g.DragDropWithinSource == false); // Can't nest BeginDragDropSource() and BeginDragDropTarget()
    g.DragDropTargetRect = display_rect;
    g.DragDropTargetClipRect = (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HasClipRect) ? g.LastItemData.ClipRect : window->ClipRect;
    g.DragDropTargetId = id;
    g.DragDropWithinTarget = true;
    return true;
}

bool ImGui::IsDragDropPayloadBeingAccepted()
{
    ImGuiContext& g = *GImGui;
    return g.DragDropActive && g.DragDropAcceptIdPrev != 0;
}

const ImGuiPayload* ImGui::AcceptDragDropPayload(const char* type, ImGuiDragDropFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiPayload& payload = g.DragDropPayload;
    IM_ASSERT(g.DragDropActive);                        // Not called between BeginDragDropTarget() and EndDragDropTarget() ?
    IM_ASSERT(payload.DataFrameCount != -1);            // Forgot to call EndDragDropTarget() ?
    if (type != NULL && !payload.IsDataType(type))
        return NULL;

    // Accept smallest drag target bounding box, this allows us to nest drag targets conveniently without ordering constraints.
    // NB: We currently accept NULL id as target. However, overlapping targets requires a unique ID to function!
    const bool was_accepted_previously = (g.DragDropAcceptIdPrev == g.DragDropTargetId);
    ImRect r = g.DragDropTargetRect;
    float r_surface = r.GetWidth() * r.GetHeight();
    if (r_surface > g.DragDropAcceptIdCurrRectSurface)
        return NULL;

    g.DragDropAcceptFlags = flags;
    g.DragDropAcceptIdCurr = g.DragDropTargetId;
    g.DragDropAcceptIdCurrRectSurface = r_surface;
    //IMGUI_DEBUG_LOG("AcceptDragDropPayload(): %08X: accept\n", g.DragDropTargetId);

    // Render default drop visuals
    payload.Preview = was_accepted_previously;
    flags |= (g.DragDropSourceFlags & ImGuiDragDropFlags_AcceptNoDrawDefaultRect); // Source can also inhibit the preview (useful for external sources that live for 1 frame)
    if (!(flags & ImGuiDragDropFlags_AcceptNoDrawDefaultRect) && payload.Preview)
        RenderDragDropTargetRect(r, g.DragDropTargetClipRect);

    g.DragDropAcceptFrameCount = g.FrameCount;
    if ((g.DragDropSourceFlags & ImGuiDragDropFlags_SourceExtern) && g.DragDropMouseButton == -1)
        payload.Delivery = was_accepted_previously && (g.DragDropSourceFrameCount < g.FrameCount);
    else
        payload.Delivery = was_accepted_previously && !IsMouseDown(g.DragDropMouseButton); // For extern drag sources affecting OS window focus, it's easier to just test !IsMouseDown() instead of IsMouseReleased()
    if (!payload.Delivery && !(flags & ImGuiDragDropFlags_AcceptBeforeDelivery))
        return NULL;

    if (payload.Delivery)
        IMGUI_DEBUG_LOG_ACTIVEID("[dragdrop] AcceptDragDropPayload(): 0x%08X: payload delivery\n", g.DragDropTargetId);
    return &payload;
}

// FIXME-STYLE FIXME-DRAGDROP: Settle on a proper default visuals for drop target.
void ImGui::RenderDragDropTargetRect(const ImRect& bb, const ImRect& item_clip_rect)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImRect bb_display = bb;
    bb_display.ClipWith(item_clip_rect); // Clip THEN expand so we have a way to visualize that target is not entirely visible.
    bb_display.Expand(3.5f);
    bool push_clip_rect = !window->ClipRect.Contains(bb_display);
    if (push_clip_rect)
        window->DrawList->PushClipRectFullScreen();
    window->DrawList->AddRect(bb_display.Min, bb_display.Max, GetColorU32(ImGuiCol_DragDropTarget), 0.0f, 0, 2.0f);
    if (push_clip_rect)
        window->DrawList->PopClipRect();
}

const ImGuiPayload* ImGui::GetDragDropPayload()
{
    ImGuiContext& g = *GImGui;
    return (g.DragDropActive && g.DragDropPayload.DataFrameCount != -1) ? &g.DragDropPayload : NULL;
}

void ImGui::EndDragDropTarget()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.DragDropActive);
    IM_ASSERT(g.DragDropWithinTarget);
    g.DragDropWithinTarget = false;

    // Clear drag and drop state payload right after delivery
    if (g.DragDropPayload.Delivery)
        ClearDragDrop();
}

//-----------------------------------------------------------------------------
// [SECTION] LOGGING/CAPTURING
//-----------------------------------------------------------------------------
// All text output from the interface can be captured into tty/file/clipboard.
// By default, tree nodes are automatically opened during logging.
//-----------------------------------------------------------------------------

// Pass text data straight to log (without being displayed)
static inline void LogTextV(ImGuiContext& g, const char* fmt, va_list args)
{
    if (g.LogFile)
    {
        g.LogBuffer.Buf.resize(0);
        g.LogBuffer.appendfv(fmt, args);
        ImFileWrite(g.LogBuffer.c_str(), sizeof(char), (ImU64)g.LogBuffer.size(), g.LogFile);
    }
    else
    {
        g.LogBuffer.appendfv(fmt, args);
    }
}

void ImGui::LogText(const char* fmt, ...)
{
    ImGuiContext& g = *GImGui;
    if (!g.LogEnabled)
        return;

    va_list args;
    va_start(args, fmt);
    LogTextV(g, fmt, args);
    va_end(args);
}

void ImGui::LogTextV(const char* fmt, va_list args)
{
    ImGuiContext& g = *GImGui;
    if (!g.LogEnabled)
        return;

    LogTextV(g, fmt, args);
}

// Internal version that takes a position to decide on newline placement and pad items according to their depth.
// We split text into individual lines to add current tree level padding
// FIXME: This code is a little complicated perhaps, considering simplifying the whole system.
void ImGui::LogRenderedText(const ImVec2* ref_pos, const char* text, const char* text_end)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    const char* prefix = g.LogNextPrefix;
    const char* suffix = g.LogNextSuffix;
    g.LogNextPrefix = g.LogNextSuffix = NULL;

    if (!text_end)
        text_end = FindRenderedTextEnd(text, text_end);

    const bool log_new_line = ref_pos && (ref_pos->y > g.LogLinePosY + g.Style.FramePadding.y + 1);
    if (ref_pos)
        g.LogLinePosY = ref_pos->y;
    if (log_new_line)
    {
        LogText(IM_NEWLINE);
        g.LogLineFirstItem = true;
    }

    if (prefix)
        LogRenderedText(ref_pos, prefix, prefix + strlen(prefix)); // Calculate end ourself to ensure "##" are included here.

    // Re-adjust padding if we have popped out of our starting depth
    if (g.LogDepthRef > window->DC.TreeDepth)
        g.LogDepthRef = window->DC.TreeDepth;
    const int tree_depth = (window->DC.TreeDepth - g.LogDepthRef);

    const char* text_remaining = text;
    for (;;)
    {
        // Split the string. Each new line (after a '\n') is followed by indentation corresponding to the current depth of our log entry.
        // We don't add a trailing \n yet to allow a subsequent item on the same line to be captured.
        const char* line_start = text_remaining;
        const char* line_end = ImStreolRange(line_start, text_end);
        const bool is_last_line = (line_end == text_end);
        if (line_start != line_end || !is_last_line)
        {
            const int line_length = (int)(line_end - line_start);
            const int indentation = g.LogLineFirstItem ? tree_depth * 4 : 1;
            LogText("%*s%.*s", indentation, "", line_length, line_start);
            g.LogLineFirstItem = false;
            if (*line_end == '\n')
            {
                LogText(IM_NEWLINE);
                g.LogLineFirstItem = true;
            }
        }
        if (is_last_line)
            break;
        text_remaining = line_end + 1;
    }

    if (suffix)
        LogRenderedText(ref_pos, suffix, suffix + strlen(suffix));
}

// Start logging/capturing text output
void ImGui::LogBegin(ImGuiLogType type, int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(g.LogEnabled == false);
    IM_ASSERT(g.LogFile == NULL);
    IM_ASSERT(g.LogBuffer.empty());
    g.LogEnabled = g.ItemUnclipByLog = true;
    g.LogType = type;
    g.LogNextPrefix = g.LogNextSuffix = NULL;
    g.LogDepthRef = window->DC.TreeDepth;
    g.LogDepthToExpand = ((auto_open_depth >= 0) ? auto_open_depth : g.LogDepthToExpandDefault);
    g.LogLinePosY = FLT_MAX;
    g.LogLineFirstItem = true;
}

// Important: doesn't copy underlying data, use carefully (prefix/suffix must be in scope at the time of the next LogRenderedText)
void ImGui::LogSetNextTextDecoration(const char* prefix, const char* suffix)
{
    ImGuiContext& g = *GImGui;
    g.LogNextPrefix = prefix;
    g.LogNextSuffix = suffix;
}

void ImGui::LogToTTY(int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    IM_UNUSED(auto_open_depth);
#ifndef IMGUI_DISABLE_TTY_FUNCTIONS
    LogBegin(ImGuiLogType_TTY, auto_open_depth);
    g.LogFile = stdout;
#endif
}

// Start logging/capturing text output to given file
void ImGui::LogToFile(int auto_open_depth, const char* filename)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;

    // FIXME: We could probably open the file in text mode "at", however note that clipboard/buffer logging will still
    // be subject to outputting OS-incompatible carriage return if within strings the user doesn't use IM_NEWLINE.
    // By opening the file in binary mode "ab" we have consistent output everywhere.
    if (!filename)
        filename = g.IO.LogFilename;
    if (!filename || !filename[0])
        return;
    ImFileHandle f = ImFileOpen(filename, "ab");
    if (!f)
    {
        IM_ASSERT(0);
        return;
    }

    LogBegin(ImGuiLogType_File, auto_open_depth);
    g.LogFile = f;
}

// Start logging/capturing text output to clipboard
void ImGui::LogToClipboard(int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    LogBegin(ImGuiLogType_Clipboard, auto_open_depth);
}

void ImGui::LogToBuffer(int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    LogBegin(ImGuiLogType_Buffer, auto_open_depth);
}

void ImGui::LogFinish()
{
    ImGuiContext& g = *GImGui;
    if (!g.LogEnabled)
        return;

    LogText(IM_NEWLINE);
    switch (g.LogType)
    {
    case ImGuiLogType_TTY:
#ifndef IMGUI_DISABLE_TTY_FUNCTIONS
        fflush(g.LogFile);
#endif
        break;
    case ImGuiLogType_File:
        ImFileClose(g.LogFile);
        break;
    case ImGuiLogType_Buffer:
        break;
    case ImGuiLogType_Clipboard:
        if (!g.LogBuffer.empty())
            SetClipboardText(g.LogBuffer.begin());
        break;
    case ImGuiLogType_None:
        IM_ASSERT(0);
        break;
    }

    g.LogEnabled = g.ItemUnclipByLog = false;
    g.LogType = ImGuiLogType_None;
    g.LogFile = NULL;
    g.LogBuffer.clear();
}

// Helper to display logging buttons
// FIXME-OBSOLETE: We should probably obsolete this and let the user have their own helper (this is one of the oldest function alive!)
void ImGui::LogButtons()
{
    ImGuiContext& g = *GImGui;

    PushID("LogButtons");
#ifndef IMGUI_DISABLE_TTY_FUNCTIONS
    const bool log_to_tty = Button("Log To TTY"); SameLine();
#else
    const bool log_to_tty = false;
#endif
    const bool log_to_file = Button("Log To File"); SameLine();
    const bool log_to_clipboard = Button("Log To Clipboard"); SameLine();
    PushItemFlag(ImGuiItemFlags_NoTabStop, true);
    SetNextItemWidth(80.0f);
    SliderInt("Default Depth", &g.LogDepthToExpandDefault, 0, 9, NULL);
    PopItemFlag();
    PopID();

    // Start logging at the end of the function so that the buttons don't appear in the log
    if (log_to_tty)
        LogToTTY();
    if (log_to_file)
        LogToFile();
    if (log_to_clipboard)
        LogToClipboard();
}


//-----------------------------------------------------------------------------
// [SECTION] SETTINGS
//-----------------------------------------------------------------------------
// - UpdateSettings() [Internal]
// - MarkIniSettingsDirty() [Internal]
// - FindSettingsHandler() [Internal]
// - ClearIniSettings() [Internal]
// - LoadIniSettingsFromDisk()
// - LoadIniSettingsFromMemory()
// - SaveIniSettingsToDisk()
// - SaveIniSettingsToMemory()
//-----------------------------------------------------------------------------
// - CreateNewWindowSettings() [Internal]
// - FindWindowSettingsByID() [Internal]
// - FindWindowSettingsByWindow() [Internal]
// - ClearWindowSettings() [Internal]
// - WindowSettingsHandler_***() [Internal]
//-----------------------------------------------------------------------------

// Called by NewFrame()
void ImGui::UpdateSettings()
{
    // Load settings on first frame (if not explicitly loaded manually before)
    ImGuiContext& g = *GImGui;
    if (!g.SettingsLoaded)
    {
        IM_ASSERT(g.SettingsWindows.empty());
        if (g.IO.IniFilename)
            LoadIniSettingsFromDisk(g.IO.IniFilename);
        g.SettingsLoaded = true;
    }

    // Save settings (with a delay after the last modification, so we don't spam disk too much)
    if (g.SettingsDirtyTimer > 0.0f)
    {
        g.SettingsDirtyTimer -= g.IO.DeltaTime;
        if (g.SettingsDirtyTimer <= 0.0f)
        {
            if (g.IO.IniFilename != NULL)
                SaveIniSettingsToDisk(g.IO.IniFilename);
            else
                g.IO.WantSaveIniSettings = true;  // Let user know they can call SaveIniSettingsToMemory(). user will need to clear io.WantSaveIniSettings themselves.
            g.SettingsDirtyTimer = 0.0f;
        }
    }
}

void ImGui::MarkIniSettingsDirty()
{
    ImGuiContext& g = *GImGui;
    if (g.SettingsDirtyTimer <= 0.0f)
        g.SettingsDirtyTimer = g.IO.IniSavingRate;
}

void ImGui::MarkIniSettingsDirty(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (!(window->Flags & ImGuiWindowFlags_NoSavedSettings))
        if (g.SettingsDirtyTimer <= 0.0f)
            g.SettingsDirtyTimer = g.IO.IniSavingRate;
}

void ImGui::AddSettingsHandler(const ImGuiSettingsHandler* handler)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(FindSettingsHandler(handler->TypeName) == NULL);
    g.SettingsHandlers.push_back(*handler);
}

void ImGui::RemoveSettingsHandler(const char* type_name)
{
    ImGuiContext& g = *GImGui;
    if (ImGuiSettingsHandler* handler = FindSettingsHandler(type_name))
        g.SettingsHandlers.erase(handler);
}

ImGuiSettingsHandler* ImGui::FindSettingsHandler(const char* type_name)
{
    ImGuiContext& g = *GImGui;
    const ImGuiID type_hash = ImHashStr(type_name);
    for (ImGuiSettingsHandler& handler : g.SettingsHandlers)
        if (handler.TypeHash == type_hash)
            return &handler;
    return NULL;
}

// Clear all settings (windows, tables, docking etc.)
void ImGui::ClearIniSettings()
{
    ImGuiContext& g = *GImGui;
    g.SettingsIniData.clear();
    for (ImGuiSettingsHandler& handler : g.SettingsHandlers)
        if (handler.ClearAllFn != NULL)
            handler.ClearAllFn(&g, &handler);
}

void ImGui::LoadIniSettingsFromDisk(const char* ini_filename)
{
    size_t file_data_size = 0;
    char* file_data = (char*)ImFileLoadToMemory(ini_filename, "rb", &file_data_size);
    if (!file_data)
        return;
    if (file_data_size > 0)
        LoadIniSettingsFromMemory(file_data, (size_t)file_data_size);
    IM_FREE(file_data);
}

// Zero-tolerance, no error reporting, cheap .ini parsing
// Set ini_size==0 to let us use strlen(ini_data). Do not call this function with a 0 if your buffer is actually empty!
void ImGui::LoadIniSettingsFromMemory(const char* ini_data, size_t ini_size)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);
    //IM_ASSERT(!g.WithinFrameScope && "Cannot be called between NewFrame() and EndFrame()");
    //IM_ASSERT(g.SettingsLoaded == false && g.FrameCount == 0);

    // For user convenience, we allow passing a non zero-terminated string (hence the ini_size parameter).
    // For our convenience and to make the code simpler, we'll also write zero-terminators within the buffer. So let's create a writable copy..
    if (ini_size == 0)
        ini_size = strlen(ini_data);
    g.SettingsIniData.Buf.resize((int)ini_size + 1);
    char* const buf = g.SettingsIniData.Buf.Data;
    char* const buf_end = buf + ini_size;
    memcpy(buf, ini_data, ini_size);
    buf_end[0] = 0;

    // Call pre-read handlers
    // Some types will clear their data (e.g. dock information) some types will allow merge/override (window)
    for (ImGuiSettingsHandler& handler : g.SettingsHandlers)
        if (handler.ReadInitFn != NULL)
            handler.ReadInitFn(&g, &handler);

    void* entry_data = NULL;
    ImGuiSettingsHandler* entry_handler = NULL;

    char* line_end = NULL;
    for (char* line = buf; line < buf_end; line = line_end + 1)
    {
        // Skip new lines markers, then find end of the line
        while (*line == '\n' || *line == '\r')
            line++;
        line_end = line;
        while (line_end < buf_end && *line_end != '\n' && *line_end != '\r')
            line_end++;
        line_end[0] = 0;
        if (line[0] == ';')
            continue;
        if (line[0] == '[' && line_end > line && line_end[-1] == ']')
        {
            // Parse "[Type][Name]". Note that 'Name' can itself contains [] characters, which is acceptable with the current format and parsing code.
            line_end[-1] = 0;
            const char* name_end = line_end - 1;
            const char* type_start = line + 1;
            char* type_end = (char*)(void*)ImStrchrRange(type_start, name_end, ']');
            const char* name_start = type_end ? ImStrchrRange(type_end + 1, name_end, '[') : NULL;
            if (!type_end || !name_start)
                continue;
            *type_end = 0; // Overwrite first ']'
            name_start++;  // Skip second '['
            entry_handler = FindSettingsHandler(type_start);
            entry_data = entry_handler ? entry_handler->ReadOpenFn(&g, entry_handler, name_start) : NULL;
        }
        else if (entry_handler != NULL && entry_data != NULL)
        {
            // Let type handler parse the line
            entry_handler->ReadLineFn(&g, entry_handler, entry_data, line);
        }
    }
    g.SettingsLoaded = true;

    // [DEBUG] Restore untouched copy so it can be browsed in Metrics (not strictly necessary)
    memcpy(buf, ini_data, ini_size);

    // Call post-read handlers
    for (ImGuiSettingsHandler& handler : g.SettingsHandlers)
        if (handler.ApplyAllFn != NULL)
            handler.ApplyAllFn(&g, &handler);
}

void ImGui::SaveIniSettingsToDisk(const char* ini_filename)
{
    ImGuiContext& g = *GImGui;
    g.SettingsDirtyTimer = 0.0f;
    if (!ini_filename)
        return;

    size_t ini_data_size = 0;
    const char* ini_data = SaveIniSettingsToMemory(&ini_data_size);
    ImFileHandle f = ImFileOpen(ini_filename, "wt");
    if (!f)
        return;
    ImFileWrite(ini_data, sizeof(char), ini_data_size, f);
    ImFileClose(f);
}

// Call registered handlers (e.g. SettingsHandlerWindow_WriteAll() + custom handlers) to write their stuff into a text buffer
const char* ImGui::SaveIniSettingsToMemory(size_t* out_size)
{
    ImGuiContext& g = *GImGui;
    g.SettingsDirtyTimer = 0.0f;
    g.SettingsIniData.Buf.resize(0);
    g.SettingsIniData.Buf.push_back(0);
    for (ImGuiSettingsHandler& handler : g.SettingsHandlers)
        handler.WriteAllFn(&g, &handler, &g.SettingsIniData);
    if (out_size)
        *out_size = (size_t)g.SettingsIniData.size();
    return g.SettingsIniData.c_str();
}

ImGuiWindowSettings* ImGui::CreateNewWindowSettings(const char* name)
{
    ImGuiContext& g = *GImGui;

    if (g.IO.ConfigDebugIniSettings == false)
    {
        // Skip to the "###" marker if any. We don't skip past to match the behavior of GetID()
        // Preserve the full string when ConfigDebugVerboseIniSettings is set to make .ini inspection easier.
        if (const char* p = strstr(name, "###"))
            name = p;
    }
    const size_t name_len = strlen(name);

    // Allocate chunk
    const size_t chunk_size = sizeof(ImGuiWindowSettings) + name_len + 1;
    ImGuiWindowSettings* settings = g.SettingsWindows.alloc_chunk(chunk_size);
    IM_PLACEMENT_NEW(settings) ImGuiWindowSettings();
    settings->ID = ImHashStr(name, name_len);
    memcpy(settings->GetName(), name, name_len + 1);   // Store with zero terminator

    return settings;
}

// We don't provide a FindWindowSettingsByName() because Docking system doesn't always hold on names.
// This is called once per window .ini entry + once per newly instantiated window.
ImGuiWindowSettings* ImGui::FindWindowSettingsByID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
        if (settings->ID == id && !settings->WantDelete)
            return settings;
    return NULL;
}

// This is faster if you are holding on a Window already as we don't need to perform a search.
ImGuiWindowSettings* ImGui::FindWindowSettingsByWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (window->SettingsOffset != -1)
        return g.SettingsWindows.ptr_from_offset(window->SettingsOffset);
    return FindWindowSettingsByID(window->ID);
}

// This will revert window to its initial state, including enabling the ImGuiCond_FirstUseEver/ImGuiCond_Once conditions once more.
void ImGui::ClearWindowSettings(const char* name)
{
    //IMGUI_DEBUG_LOG("ClearWindowSettings('%s')\n", name);
    ImGuiWindow* window = FindWindowByName(name);
    if (window != NULL)
    {
        window->Flags |= ImGuiWindowFlags_NoSavedSettings;
        InitOrLoadWindowSettings(window, NULL);
    }
    if (ImGuiWindowSettings* settings = window ? FindWindowSettingsByWindow(window) : FindWindowSettingsByID(ImHashStr(name)))
        settings->WantDelete = true;
}

static void WindowSettingsHandler_ClearAll(ImGuiContext* ctx, ImGuiSettingsHandler*)
{
    ImGuiContext& g = *ctx;
    for (ImGuiWindow* window : g.Windows)
        window->SettingsOffset = -1;
    g.SettingsWindows.clear();
}

static void* WindowSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    ImGuiID id = ImHashStr(name);
    ImGuiWindowSettings* settings = ImGui::FindWindowSettingsByID(id);
    if (settings)
        *settings = ImGuiWindowSettings(); // Clear existing if recycling previous entry
    else
        settings = ImGui::CreateNewWindowSettings(name);
    settings->ID = id;
    settings->WantApply = true;
    return (void*)settings;
}

static void WindowSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    ImGuiWindowSettings* settings = (ImGuiWindowSettings*)entry;
    int x, y;
    int i;
    if (sscanf(line, "Pos=%i,%i", &x, &y) == 2)         { settings->Pos = ImVec2ih((short)x, (short)y); }
    else if (sscanf(line, "Size=%i,%i", &x, &y) == 2)   { settings->Size = ImVec2ih((short)x, (short)y); }
    else if (sscanf(line, "Collapsed=%d", &i) == 1)     { settings->Collapsed = (i != 0); }
    else if (sscanf(line, "IsChild=%d", &i) == 1)       { settings->IsChild = (i != 0); }
}

// Apply to existing windows (if any)
static void WindowSettingsHandler_ApplyAll(ImGuiContext* ctx, ImGuiSettingsHandler*)
{
    ImGuiContext& g = *ctx;
    for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
        if (settings->WantApply)
        {
            if (ImGuiWindow* window = ImGui::FindWindowByID(settings->ID))
                ApplyWindowSettings(window, settings);
            settings->WantApply = false;
        }
}

static void WindowSettingsHandler_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    // Gather data from windows that were active during this session
    // (if a window wasn't opened in this session we preserve its settings)
    ImGuiContext& g = *ctx;
    for (ImGuiWindow* window : g.Windows)
    {
        if (window->Flags & ImGuiWindowFlags_NoSavedSettings)
            continue;

        ImGuiWindowSettings* settings = ImGui::FindWindowSettingsByWindow(window);
        if (!settings)
        {
            settings = ImGui::CreateNewWindowSettings(window->Name);
            window->SettingsOffset = g.SettingsWindows.offset_from_ptr(settings);
        }
        IM_ASSERT(settings->ID == window->ID);
        settings->Pos = ImVec2ih(window->Pos);
        settings->Size = ImVec2ih(window->SizeFull);
        settings->IsChild = (window->Flags & ImGuiWindowFlags_ChildWindow) != 0;
        settings->Collapsed = window->Collapsed;
        settings->WantDelete = false;
    }

    // Write to text buffer
    buf->reserve(buf->size() + g.SettingsWindows.size() * 6); // ballpark reserve
    for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
    {
        if (settings->WantDelete)
            continue;
        const char* settings_name = settings->GetName();
        buf->appendf("[%s][%s]\n", handler->TypeName, settings_name);
        if (settings->IsChild)
        {
            buf->appendf("IsChild=1\n");
            buf->appendf("Size=%d,%d\n", settings->Size.x, settings->Size.y);
        }
        else
        {
            buf->appendf("Pos=%d,%d\n", settings->Pos.x, settings->Pos.y);
            buf->appendf("Size=%d,%d\n", settings->Size.x, settings->Size.y);
            if (settings->Collapsed)
                buf->appendf("Collapsed=1\n");
        }
        buf->append("\n");
    }
}


//-----------------------------------------------------------------------------
// [SECTION] LOCALIZATION
//-----------------------------------------------------------------------------

void ImGui::LocalizeRegisterEntries(const ImGuiLocEntry* entries, int count)
{
    ImGuiContext& g = *GImGui;
    for (int n = 0; n < count; n++)
        g.LocalizationTable[entries[n].Key] = entries[n].Text;
}


//-----------------------------------------------------------------------------
// [SECTION] VIEWPORTS, PLATFORM WINDOWS
//-----------------------------------------------------------------------------
// - GetMainViewport()
// - SetWindowViewport() [Internal]
// - UpdateViewportsNewFrame() [Internal]
// (this section is more complete in the 'docking' branch)
//-----------------------------------------------------------------------------

ImGuiViewport* ImGui::GetMainViewport()
{
    ImGuiContext& g = *GImGui;
    return g.Viewports[0];
}

void ImGui::SetWindowViewport(ImGuiWindow* window, ImGuiViewportP* viewport)
{
    window->Viewport = viewport;
}

// Update viewports and monitor infos
static void ImGui::UpdateViewportsNewFrame()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Viewports.Size == 1);

    // Update main viewport with current platform position.
    // FIXME-VIEWPORT: Size is driven by backend/user code for backward-compatibility but we should aim to make this more consistent.
    ImGuiViewportP* main_viewport = g.Viewports[0];
    main_viewport->Flags = ImGuiViewportFlags_IsPlatformWindow | ImGuiViewportFlags_OwnedByApp;
    main_viewport->Pos = ImVec2(0.0f, 0.0f);
    main_viewport->Size = g.IO.DisplaySize;

    for (ImGuiViewportP* viewport : g.Viewports)
    {
        // Lock down space taken by menu bars and status bars
        // Setup initial value for functions like BeginMainMenuBar(), DockSpaceOverViewport() etc.
        viewport->WorkInsetMin = viewport->BuildWorkInsetMin;
        viewport->WorkInsetMax = viewport->BuildWorkInsetMax;
        viewport->BuildWorkInsetMin = viewport->BuildWorkInsetMax = ImVec2(0.0f, 0.0f);
        viewport->UpdateWorkRect();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DOCKING
//-----------------------------------------------------------------------------

// (this section is filled in the 'docking' branch)


//-----------------------------------------------------------------------------
// [SECTION] PLATFORM DEPENDENT HELPERS
//-----------------------------------------------------------------------------
// - Default clipboard handlers
// - Default shell function handlers
// - Default IME handlers
//-----------------------------------------------------------------------------

#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS)

#ifdef _MSC_VER
#pragma comment(lib, "user32")
#pragma comment(lib, "kernel32")
#endif

// Win32 clipboard implementation
// We use g.ClipboardHandlerData for temporary storage to ensure it is freed on Shutdown()
static const char* Platform_GetClipboardTextFn_DefaultImpl(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    g.ClipboardHandlerData.clear();
    if (!::OpenClipboard(NULL))
        return NULL;
    HANDLE wbuf_handle = ::GetClipboardData(CF_UNICODETEXT);
    if (wbuf_handle == NULL)
    {
        ::CloseClipboard();
        return NULL;
    }
    if (const WCHAR* wbuf_global = (const WCHAR*)::GlobalLock(wbuf_handle))
    {
        int buf_len = ::WideCharToMultiByte(CP_UTF8, 0, wbuf_global, -1, NULL, 0, NULL, NULL);
        g.ClipboardHandlerData.resize(buf_len);
        ::WideCharToMultiByte(CP_UTF8, 0, wbuf_global, -1, g.ClipboardHandlerData.Data, buf_len, NULL, NULL);
    }
    ::GlobalUnlock(wbuf_handle);
    ::CloseClipboard();
    return g.ClipboardHandlerData.Data;
}

static void Platform_SetClipboardTextFn_DefaultImpl(ImGuiContext*, const char* text)
{
    if (!::OpenClipboard(NULL))
        return;
    const int wbuf_length = ::MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    HGLOBAL wbuf_handle = ::GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wbuf_length * sizeof(WCHAR));
    if (wbuf_handle == NULL)
    {
        ::CloseClipboard();
        return;
    }
    WCHAR* wbuf_global = (WCHAR*)::GlobalLock(wbuf_handle);
    ::MultiByteToWideChar(CP_UTF8, 0, text, -1, wbuf_global, wbuf_length);
    ::GlobalUnlock(wbuf_handle);
    ::EmptyClipboard();
    if (::SetClipboardData(CF_UNICODETEXT, wbuf_handle) == NULL)
        ::GlobalFree(wbuf_handle);
    ::CloseClipboard();
}

#elif defined(__APPLE__) && TARGET_OS_OSX && defined(IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS)

#include <Carbon/Carbon.h>  // Use old API to avoid need for separate .mm file
static PasteboardRef main_clipboard = 0;

// OSX clipboard implementation
// If you enable this you will need to add '-framework ApplicationServices' to your linker command-line!
static void Platform_SetClipboardTextFn_DefaultImpl(ImGuiContext*, const char* text)
{
    if (!main_clipboard)
        PasteboardCreate(kPasteboardClipboard, &main_clipboard);
    PasteboardClear(main_clipboard);
    CFDataRef cf_data = CFDataCreate(kCFAllocatorDefault, (const UInt8*)text, strlen(text));
    if (cf_data)
    {
        PasteboardPutItemFlavor(main_clipboard, (PasteboardItemID)1, CFSTR("public.utf8-plain-text"), cf_data, 0);
        CFRelease(cf_data);
    }
}

static const char* Platform_GetClipboardTextFn_DefaultImpl(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    if (!main_clipboard)
        PasteboardCreate(kPasteboardClipboard, &main_clipboard);
    PasteboardSynchronize(main_clipboard);

    ItemCount item_count = 0;
    PasteboardGetItemCount(main_clipboard, &item_count);
    for (ItemCount i = 0; i < item_count; i++)
    {
        PasteboardItemID item_id = 0;
        PasteboardGetItemIdentifier(main_clipboard, i + 1, &item_id);
        CFArrayRef flavor_type_array = 0;
        PasteboardCopyItemFlavors(main_clipboard, item_id, &flavor_type_array);
        for (CFIndex j = 0, nj = CFArrayGetCount(flavor_type_array); j < nj; j++)
        {
            CFDataRef cf_data;
            if (PasteboardCopyItemFlavorData(main_clipboard, item_id, CFSTR("public.utf8-plain-text"), &cf_data) == noErr)
            {
                g.ClipboardHandlerData.clear();
                int length = (int)CFDataGetLength(cf_data);
                g.ClipboardHandlerData.resize(length + 1);
                CFDataGetBytes(cf_data, CFRangeMake(0, length), (UInt8*)g.ClipboardHandlerData.Data);
                g.ClipboardHandlerData[length] = 0;
                CFRelease(cf_data);
                return g.ClipboardHandlerData.Data;
            }
        }
    }
    return NULL;
}

#else

// Local Dear ImGui-only clipboard implementation, if user hasn't defined better clipboard handlers.
static const char* Platform_GetClipboardTextFn_DefaultImpl(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    return g.ClipboardHandlerData.empty() ? NULL : g.ClipboardHandlerData.begin();
}

static void Platform_SetClipboardTextFn_DefaultImpl(ImGuiContext* ctx, const char* text)
{
    ImGuiContext& g = *ctx;
    g.ClipboardHandlerData.clear();
    const char* text_end = text + strlen(text);
    g.ClipboardHandlerData.resize((int)(text_end - text) + 1);
    memcpy(&g.ClipboardHandlerData[0], text, (size_t)(text_end - text));
    g.ClipboardHandlerData[(int)(text_end - text)] = 0;
}

#endif // Default clipboard handlers

//-----------------------------------------------------------------------------

#ifndef IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
#if defined(__APPLE__) && TARGET_OS_IPHONE
#define IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
#endif

#if defined(_WIN32) && defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#define IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
#endif
#endif

#ifndef IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
#ifdef _WIN32
#include <shellapi.h>   // ShellExecuteA()
#ifdef _MSC_VER
#pragma comment(lib, "shell32")
#endif
static bool Platform_OpenInShellFn_DefaultImpl(ImGuiContext*, const char* path)
{
    return (INT_PTR)::ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOWDEFAULT) > 32;
}
#else
#include <sys/wait.h>
#include <unistd.h>
static bool Platform_OpenInShellFn_DefaultImpl(ImGuiContext*, const char* path)
{
#if defined(__APPLE__)
    const char* args[] { "open", "--", path, NULL };
#else
    const char* args[] { "xdg-open", path, NULL };
#endif
    pid_t pid = fork();
    if (pid < 0)
        return false;
    if (!pid)
    {
        execvp(args[0], const_cast<char **>(args));
        exit(-1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status) == 0;
    }
}
#endif
#else
static bool Platform_OpenInShellFn_DefaultImpl(ImGuiContext*, const char*) { return false; }
#endif // Default shell handlers

//-----------------------------------------------------------------------------

// Win32 API IME support (for Asian languages, etc.)
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS)

#include <imm.h>
#ifdef _MSC_VER
#pragma comment(lib, "imm32")
#endif

static void Platform_SetImeDataFn_DefaultImpl(ImGuiContext*, ImGuiViewport* viewport, ImGuiPlatformImeData* data)
{
    // Notify OS Input Method Editor of text input position
    HWND hwnd = (HWND)viewport->PlatformHandleRaw;
    if (hwnd == 0)
        return;

    //::ImmAssociateContextEx(hwnd, NULL, data->WantVisible ? IACE_DEFAULT : 0);
    if (HIMC himc = ::ImmGetContext(hwnd))
    {
        COMPOSITIONFORM composition_form = {};
        composition_form.ptCurrentPos.x = (LONG)data->InputPos.x;
        composition_form.ptCurrentPos.y = (LONG)data->InputPos.y;
        composition_form.dwStyle = CFS_FORCE_POSITION;
        ::ImmSetCompositionWindow(himc, &composition_form);
        CANDIDATEFORM candidate_form = {};
        candidate_form.dwStyle = CFS_CANDIDATEPOS;
        candidate_form.ptCurrentPos.x = (LONG)data->InputPos.x;
        candidate_form.ptCurrentPos.y = (LONG)data->InputPos.y;
        ::ImmSetCandidateWindow(himc, &candidate_form);
        ::ImmReleaseContext(hwnd, himc);
    }
}

#else

static void Platform_SetImeDataFn_DefaultImpl(ImGuiContext*, ImGuiViewport*, ImGuiPlatformImeData*) {}

#endif // Default IME handlers

//-----------------------------------------------------------------------------
// [SECTION] METRICS/DEBUGGER WINDOW
//-----------------------------------------------------------------------------
// - DebugRenderViewportThumbnail() [Internal]
// - RenderViewportsThumbnails() [Internal]
// - DebugTextEncoding()
// - MetricsHelpMarker() [Internal]
// - ShowFontAtlas() [Internal]
// - ShowMetricsWindow()
// - DebugNodeColumns() [Internal]
// - DebugNodeDrawList() [Internal]
// - DebugNodeDrawCmdShowMeshAndBoundingBox() [Internal]
// - DebugNodeFont() [Internal]
// - DebugNodeFontGlyph() [Internal]
// - DebugNodeStorage() [Internal]
// - DebugNodeTabBar() [Internal]
// - DebugNodeViewport() [Internal]
// - DebugNodeWindow() [Internal]
// - DebugNodeWindowSettings() [Internal]
// - DebugNodeWindowsList() [Internal]
// - DebugNodeWindowsListByBeginStackParent() [Internal]
//-----------------------------------------------------------------------------

#ifndef IMGUI_DISABLE_DEBUG_TOOLS

void ImGui::DebugRenderViewportThumbnail(ImDrawList* draw_list, ImGuiViewportP* viewport, const ImRect& bb)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImVec2 scale = bb.GetSize() / viewport->Size;
    ImVec2 off = bb.Min - viewport->Pos * scale;
    float alpha_mul = 1.0f;
    window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border, alpha_mul * 0.40f));
    for (ImGuiWindow* thumb_window : g.Windows)
    {
        if (!thumb_window->WasActive || (thumb_window->Flags & ImGuiWindowFlags_ChildWindow))
            continue;

        ImRect thumb_r = thumb_window->Rect();
        ImRect title_r = thumb_window->TitleBarRect();
        thumb_r = ImRect(ImTrunc(off + thumb_r.Min * scale), ImTrunc(off +  thumb_r.Max * scale));
        title_r = ImRect(ImTrunc(off + title_r.Min * scale), ImTrunc(off +  ImVec2(title_r.Max.x, title_r.Min.y + title_r.GetHeight() * 3.0f) * scale)); // Exaggerate title bar height
        thumb_r.ClipWithFull(bb);
        title_r.ClipWithFull(bb);
        const bool window_is_focused = (g.NavWindow && thumb_window->RootWindowForTitleBarHighlight == g.NavWindow->RootWindowForTitleBarHighlight);
        window->DrawList->AddRectFilled(thumb_r.Min, thumb_r.Max, GetColorU32(ImGuiCol_WindowBg, alpha_mul));
        window->DrawList->AddRectFilled(title_r.Min, title_r.Max, GetColorU32(window_is_focused ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg, alpha_mul));
        window->DrawList->AddRect(thumb_r.Min, thumb_r.Max, GetColorU32(ImGuiCol_Border, alpha_mul));
        window->DrawList->AddText(g.Font, g.FontSize * 1.0f, title_r.Min, GetColorU32(ImGuiCol_Text, alpha_mul), thumb_window->Name, FindRenderedTextEnd(thumb_window->Name));
    }
    draw_list->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border, alpha_mul));
    if (viewport->ID == g.DebugMetricsConfig.HighlightViewportID)
        window->DrawList->AddRect(bb.Min, bb.Max, IM_COL32(255, 255, 0, 255));
}

static void RenderViewportsThumbnails()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    float SCALE = 1.0f / 8.0f;
    ImRect bb_full(g.Viewports[0]->Pos, g.Viewports[0]->Pos + g.Viewports[0]->Size);
    ImVec2 p = window->DC.CursorPos;
    ImVec2 off = p - bb_full.Min * SCALE;

    // Draw viewports
    for (ImGuiViewportP* viewport : g.Viewports)
    {
        ImRect viewport_draw_bb(off + (viewport->Pos) * SCALE, off + (viewport->Pos + viewport->Size) * SCALE);
        ImGui::DebugRenderViewportThumbnail(window->DrawList, viewport, viewport_draw_bb);
    }
    ImGui::Dummy(bb_full.GetSize() * SCALE);
}

// Draw an arbitrary US keyboard layout to visualize translated keys
void ImGui::DebugRenderKeyboardPreview(ImDrawList* draw_list)
{
    const float scale = ImGui::GetFontSize() / 13.0f;
    const ImVec2 key_size = ImVec2(35.0f, 35.0f) * scale;
    const float  key_rounding = 3.0f * scale;
    const ImVec2 key_face_size = ImVec2(25.0f, 25.0f) * scale;
    const ImVec2 key_face_pos = ImVec2(5.0f, 3.0f) * scale;
    const float  key_face_rounding = 2.0f * scale;
    const ImVec2 key_label_pos = ImVec2(7.0f, 4.0f) * scale;
    const ImVec2 key_step = ImVec2(key_size.x - 1.0f, key_size.y - 1.0f);
    const float  key_row_offset = 9.0f * scale;

    ImVec2 board_min = GetCursorScreenPos();
    ImVec2 board_max = ImVec2(board_min.x + 3 * key_step.x + 2 * key_row_offset + 10.0f, board_min.y + 3 * key_step.y + 10.0f);
    ImVec2 start_pos = ImVec2(board_min.x + 5.0f - key_step.x, board_min.y);

    struct KeyLayoutData { int Row, Col; const char* Label; ImGuiKey Key; };
    const KeyLayoutData keys_to_display[] =
    {
        { 0, 0, "", ImGuiKey_Tab },      { 0, 1, "Q", ImGuiKey_Q }, { 0, 2, "W", ImGuiKey_W }, { 0, 3, "E", ImGuiKey_E }, { 0, 4, "R", ImGuiKey_R },
        { 1, 0, "", ImGuiKey_CapsLock }, { 1, 1, "A", ImGuiKey_A }, { 1, 2, "S", ImGuiKey_S }, { 1, 3, "D", ImGuiKey_D }, { 1, 4, "F", ImGuiKey_F },
        { 2, 0, "", ImGuiKey_LeftShift },{ 2, 1, "Z", ImGuiKey_Z }, { 2, 2, "X", ImGuiKey_X }, { 2, 3, "C", ImGuiKey_C }, { 2, 4, "V", ImGuiKey_V }
    };

    // Elements rendered manually via ImDrawList API are not clipped automatically.
    // While not strictly necessary, here IsItemVisible() is used to avoid rendering these shapes when they are out of view.
    Dummy(board_max - board_min);
    if (!IsItemVisible())
        return;
    draw_list->PushClipRect(board_min, board_max, true);
    for (int n = 0; n < IM_ARRAYSIZE(keys_to_display); n++)
    {
        const KeyLayoutData* key_data = &keys_to_display[n];
        ImVec2 key_min = ImVec2(start_pos.x + key_data->Col * key_step.x + key_data->Row * key_row_offset, start_pos.y + key_data->Row * key_step.y);
        ImVec2 key_max = key_min + key_size;
        draw_list->AddRectFilled(key_min, key_max, IM_COL32(204, 204, 204, 255), key_rounding);
        draw_list->AddRect(key_min, key_max, IM_COL32(24, 24, 24, 255), key_rounding);
        ImVec2 face_min = ImVec2(key_min.x + key_face_pos.x, key_min.y + key_face_pos.y);
        ImVec2 face_max = ImVec2(face_min.x + key_face_size.x, face_min.y + key_face_size.y);
        draw_list->AddRect(face_min, face_max, IM_COL32(193, 193, 193, 255), key_face_rounding, ImDrawFlags_None, 2.0f);
        draw_list->AddRectFilled(face_min, face_max, IM_COL32(252, 252, 252, 255), key_face_rounding);
        ImVec2 label_min = ImVec2(key_min.x + key_label_pos.x, key_min.y + key_label_pos.y);
        draw_list->AddText(label_min, IM_COL32(64, 64, 64, 255), key_data->Label);
        if (IsKeyDown(key_data->Key))
            draw_list->AddRectFilled(key_min, key_max, IM_COL32(255, 0, 0, 128), key_rounding);
    }
    draw_list->PopClipRect();
}

// Helper tool to diagnose between text encoding issues and font loading issues. Pass your UTF-8 string and verify that there are correct.
void ImGui::DebugTextEncoding(const char* str)
{
    Text("Text: \"%s\"", str);
    if (!BeginTable("##DebugTextEncoding", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable))
        return;
    TableSetupColumn("Offset");
    TableSetupColumn("UTF-8");
    TableSetupColumn("Glyph");
    TableSetupColumn("Codepoint");
    TableHeadersRow();
    for (const char* p = str; *p != 0; )
    {
        unsigned int c;
        const int c_utf8_len = ImTextCharFromUtf8(&c, p, NULL);
        TableNextColumn();
        Text("%d", (int)(p - str));
        TableNextColumn();
        for (int byte_index = 0; byte_index < c_utf8_len; byte_index++)
        {
            if (byte_index > 0)
                SameLine();
            Text("0x%02X", (int)(unsigned char)p[byte_index]);
        }
        TableNextColumn();
        if (GetFont()->FindGlyphNoFallback((ImWchar)c))
            TextUnformatted(p, p + c_utf8_len);
        else
            TextUnformatted((c == IM_UNICODE_CODEPOINT_INVALID) ? "[invalid]" : "[missing]");
        TableNextColumn();
        Text("U+%04X", (int)c);
        p += c_utf8_len;
    }
    EndTable();
}

static void DebugFlashStyleColorStop()
{
    ImGuiContext& g = *GImGui;
    if (g.DebugFlashStyleColorIdx != ImGuiCol_COUNT)
        g.Style.Colors[g.DebugFlashStyleColorIdx] = g.DebugFlashStyleColorBackup;
    g.DebugFlashStyleColorIdx = ImGuiCol_COUNT;
}

// Flash a given style color for some + inhibit modifications of this color via PushStyleColor() calls.
void ImGui::DebugFlashStyleColor(ImGuiCol idx)
{
    ImGuiContext& g = *GImGui;
    DebugFlashStyleColorStop();
    g.DebugFlashStyleColorTime = 0.5f;
    g.DebugFlashStyleColorIdx = idx;
    g.DebugFlashStyleColorBackup = g.Style.Colors[idx];
}

void ImGui::UpdateDebugToolFlashStyleColor()
{
    ImGuiContext& g = *GImGui;
    if (g.DebugFlashStyleColorTime <= 0.0f)
        return;
    ColorConvertHSVtoRGB(ImCos(g.DebugFlashStyleColorTime * 6.0f) * 0.5f + 0.5f, 0.5f, 0.5f, g.Style.Colors[g.DebugFlashStyleColorIdx].x, g.Style.Colors[g.DebugFlashStyleColorIdx].y, g.Style.Colors[g.DebugFlashStyleColorIdx].z);
    g.Style.Colors[g.DebugFlashStyleColorIdx].w = 1.0f;
    if ((g.DebugFlashStyleColorTime -= g.IO.DeltaTime) <= 0.0f)
        DebugFlashStyleColorStop();
}

// Avoid naming collision with imgui_demo.cpp's HelpMarker() for unity builds.
static void MetricsHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// [DEBUG] List fonts in a font atlas and display its texture
void ImGui::ShowFontAtlas(ImFontAtlas* atlas)
{
    for (ImFont* font : atlas->Fonts)
    {
        PushID(font);
        DebugNodeFont(font);
        PopID();
    }
    if (TreeNode("Font Atlas", "Font Atlas (%dx%d pixels)", atlas->TexWidth, atlas->TexHeight))
    {
        ImGuiContext& g = *GImGui;
        ImGuiMetricsConfig* cfg = &g.DebugMetricsConfig;
        Checkbox("Tint with Text Color", &cfg->ShowAtlasTintedWithTextColor); // Using text color ensure visibility of core atlas data, but will alter custom colored icons
        ImVec4 tint_col = cfg->ShowAtlasTintedWithTextColor ? GetStyleColorVec4(ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImVec4 border_col = GetStyleColorVec4(ImGuiCol_Border);
        Image(atlas->TexID, ImVec2((float)atlas->TexWidth, (float)atlas->TexHeight), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint_col, border_col);
        TreePop();
    }
}

void ImGui::ShowMetricsWindow(bool* p_open)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    ImGuiMetricsConfig* cfg = &g.DebugMetricsConfig;
    if (cfg->ShowDebugLog)
        ShowDebugLogWindow(&cfg->ShowDebugLog);
    if (cfg->ShowIDStackTool)
        ShowIDStackToolWindow(&cfg->ShowIDStackTool);

    if (!Begin("Dear ImGui Metrics/Debugger", p_open) || GetCurrentWindow()->BeginCount > 1)
    {
        End();
        return;
    }

    // [DEBUG] Clear debug breaks hooks after exactly one cycle.
    DebugBreakClearData();

    // Basic info
    Text("Dear ImGui %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
    if (g.ContextName[0] != 0)
    {
        SameLine();
        Text("(Context Name: \"%s\")", g.ContextName);
    }
    Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);
    Text("%d visible windows, %d current allocations", io.MetricsRenderWindows, g.DebugAllocInfo.TotalAllocCount - g.DebugAllocInfo.TotalFreeCount);
    //SameLine(); if (SmallButton("GC")) { g.GcCompactAll = true; }

    Separator();

    // Debugging enums
    enum { WRT_OuterRect, WRT_OuterRectClipped, WRT_InnerRect, WRT_InnerClipRect, WRT_WorkRect, WRT_Content, WRT_ContentIdeal, WRT_ContentRegionRect, WRT_Count }; // Windows Rect Type
    const char* wrt_rects_names[WRT_Count] = { "OuterRect", "OuterRectClipped", "InnerRect", "InnerClipRect", "WorkRect", "Content", "ContentIdeal", "ContentRegionRect" };
    enum { TRT_OuterRect, TRT_InnerRect, TRT_WorkRect, TRT_HostClipRect, TRT_InnerClipRect, TRT_BackgroundClipRect, TRT_ColumnsRect, TRT_ColumnsWorkRect, TRT_ColumnsClipRect, TRT_ColumnsContentHeadersUsed, TRT_ColumnsContentHeadersIdeal, TRT_ColumnsContentFrozen, TRT_ColumnsContentUnfrozen, TRT_Count }; // Tables Rect Type
    const char* trt_rects_names[TRT_Count] = { "OuterRect", "InnerRect", "WorkRect", "HostClipRect", "InnerClipRect", "BackgroundClipRect", "ColumnsRect", "ColumnsWorkRect", "ColumnsClipRect", "ColumnsContentHeadersUsed", "ColumnsContentHeadersIdeal", "ColumnsContentFrozen", "ColumnsContentUnfrozen" };
    if (cfg->ShowWindowsRectsType < 0)
        cfg->ShowWindowsRectsType = WRT_WorkRect;
    if (cfg->ShowTablesRectsType < 0)
        cfg->ShowTablesRectsType = TRT_WorkRect;

    struct Funcs
    {
        static ImRect GetTableRect(ImGuiTable* table, int rect_type, int n)
        {
            ImGuiTableInstanceData* table_instance = TableGetInstanceData(table, table->InstanceCurrent); // Always using last submitted instance
            if (rect_type == TRT_OuterRect)                     { return table->OuterRect; }
            else if (rect_type == TRT_InnerRect)                { return table->InnerRect; }
            else if (rect_type == TRT_WorkRect)                 { return table->WorkRect; }
            else if (rect_type == TRT_HostClipRect)             { return table->HostClipRect; }
            else if (rect_type == TRT_InnerClipRect)            { return table->InnerClipRect; }
            else if (rect_type == TRT_BackgroundClipRect)       { return table->BgClipRect; }
            else if (rect_type == TRT_ColumnsRect)              { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->MinX, table->InnerClipRect.Min.y, c->MaxX, table->InnerClipRect.Min.y + table_instance->LastOuterHeight); }
            else if (rect_type == TRT_ColumnsWorkRect)          { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->WorkRect.Min.y, c->WorkMaxX, table->WorkRect.Max.y); }
            else if (rect_type == TRT_ColumnsClipRect)          { ImGuiTableColumn* c = &table->Columns[n]; return c->ClipRect; }
            else if (rect_type == TRT_ColumnsContentHeadersUsed){ ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y, c->ContentMaxXHeadersUsed, table->InnerClipRect.Min.y + table_instance->LastTopHeadersRowHeight); } // Note: y1/y2 not always accurate
            else if (rect_type == TRT_ColumnsContentHeadersIdeal){ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y, c->ContentMaxXHeadersIdeal, table->InnerClipRect.Min.y + table_instance->LastTopHeadersRowHeight); }
            else if (rect_type == TRT_ColumnsContentFrozen)     { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y, c->ContentMaxXFrozen, table->InnerClipRect.Min.y + table_instance->LastFrozenHeight); }
            else if (rect_type == TRT_ColumnsContentUnfrozen)   { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y + table_instance->LastFrozenHeight, c->ContentMaxXUnfrozen, table->InnerClipRect.Max.y); }
            IM_ASSERT(0);
            return ImRect();
        }

        static ImRect GetWindowRect(ImGuiWindow* window, int rect_type)
        {
            if (rect_type == WRT_OuterRect)                 { return window->Rect(); }
            else if (rect_type == WRT_OuterRectClipped)     { return window->OuterRectClipped; }
            else if (rect_type == WRT_InnerRect)            { return window->InnerRect; }
            else if (rect_type == WRT_InnerClipRect)        { return window->InnerClipRect; }
            else if (rect_type == WRT_WorkRect)             { return window->WorkRect; }
            else if (rect_type == WRT_Content)              { ImVec2 min = window->InnerRect.Min - window->Scroll + window->WindowPadding; return ImRect(min, min + window->ContentSize); }
            else if (rect_type == WRT_ContentIdeal)         { ImVec2 min = window->InnerRect.Min - window->Scroll + window->WindowPadding; return ImRect(min, min + window->ContentSizeIdeal); }
            else if (rect_type == WRT_ContentRegionRect)    { return window->ContentRegionRect; }
            IM_ASSERT(0);
            return ImRect();
        }
    };

    // Tools
    if (TreeNode("Tools"))
    {
        // Debug Break features
        // The Item Picker tool is super useful to visually select an item and break into the call-stack of where it was submitted.
        SeparatorTextEx(0, "Debug breaks", NULL, CalcTextSize("(?)").x + g.Style.SeparatorTextPadding.x);
        SameLine();
        MetricsHelpMarker("Will call the IM_DEBUG_BREAK() macro to break in debugger.\nWarning: If you don't have a debugger attached, this will probably crash.");
        if (Checkbox("Show Item Picker", &g.DebugItemPickerActive) && g.DebugItemPickerActive)
            DebugStartItemPicker();
        Checkbox("Show \"Debug Break\" buttons in other sections (io.ConfigDebugIsDebuggerPresent)", &g.IO.ConfigDebugIsDebuggerPresent);

        SeparatorText("Visualize");

        Checkbox("Show Debug Log", &cfg->ShowDebugLog);
        SameLine();
        MetricsHelpMarker("You can also call ImGui::ShowDebugLogWindow() from your code.");

        Checkbox("Show ID Stack Tool", &cfg->ShowIDStackTool);
        SameLine();
        MetricsHelpMarker("You can also call ImGui::ShowIDStackToolWindow() from your code.");

        Checkbox("Show windows begin order", &cfg->ShowWindowsBeginOrder);
        Checkbox("Show windows rectangles", &cfg->ShowWindowsRects);
        SameLine();
        SetNextItemWidth(GetFontSize() * 12);
        cfg->ShowWindowsRects |= Combo("##show_windows_rect_type", &cfg->ShowWindowsRectsType, wrt_rects_names, WRT_Count, WRT_Count);
        if (cfg->ShowWindowsRects && g.NavWindow != NULL)
        {
            BulletText("'%s':", g.NavWindow->Name);
            Indent();
            for (int rect_n = 0; rect_n < WRT_Count; rect_n++)
            {
                ImRect r = Funcs::GetWindowRect(g.NavWindow, rect_n);
                Text("(%6.1f,%6.1f) (%6.1f,%6.1f) Size (%6.1f,%6.1f) %s", r.Min.x, r.Min.y, r.Max.x, r.Max.y, r.GetWidth(), r.GetHeight(), wrt_rects_names[rect_n]);
            }
            Unindent();
        }

        Checkbox("Show tables rectangles", &cfg->ShowTablesRects);
        SameLine();
        SetNextItemWidth(GetFontSize() * 12);
        cfg->ShowTablesRects |= Combo("##show_table_rects_type", &cfg->ShowTablesRectsType, trt_rects_names, TRT_Count, TRT_Count);
        if (cfg->ShowTablesRects && g.NavWindow != NULL)
        {
            for (int table_n = 0; table_n < g.Tables.GetMapSize(); table_n++)
            {
                ImGuiTable* table = g.Tables.TryGetMapData(table_n);
                if (table == NULL || table->LastFrameActive < g.FrameCount - 1 || (table->OuterWindow != g.NavWindow && table->InnerWindow != g.NavWindow))
                    continue;

                BulletText("Table 0x%08X (%d columns, in '%s')", table->ID, table->ColumnsCount, table->OuterWindow->Name);
                if (IsItemHovered())
                    GetForegroundDrawList()->AddRect(table->OuterRect.Min - ImVec2(1, 1), table->OuterRect.Max + ImVec2(1, 1), IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
                Indent();
                char buf[128];
                for (int rect_n = 0; rect_n < TRT_Count; rect_n++)
                {
                    if (rect_n >= TRT_ColumnsRect)
                    {
                        if (rect_n != TRT_ColumnsRect && rect_n != TRT_ColumnsClipRect)
                            continue;
                        for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
                        {
                            ImRect r = Funcs::GetTableRect(table, rect_n, column_n);
                            ImFormatString(buf, IM_ARRAYSIZE(buf), "(%6.1f,%6.1f) (%6.1f,%6.1f) Size (%6.1f,%6.1f) Col %d %s", r.Min.x, r.Min.y, r.Max.x, r.Max.y, r.GetWidth(), r.GetHeight(), column_n, trt_rects_names[rect_n]);
                            Selectable(buf);
                            if (IsItemHovered())
                                GetForegroundDrawList()->AddRect(r.Min - ImVec2(1, 1), r.Max + ImVec2(1, 1), IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
                        }
                    }
                    else
                    {
                        ImRect r = Funcs::GetTableRect(table, rect_n, -1);
                        ImFormatString(buf, IM_ARRAYSIZE(buf), "(%6.1f,%6.1f) (%6.1f,%6.1f) Size (%6.1f,%6.1f) %s", r.Min.x, r.Min.y, r.Max.x, r.Max.y, r.GetWidth(), r.GetHeight(), trt_rects_names[rect_n]);
                        Selectable(buf);
                        if (IsItemHovered())
                            GetForegroundDrawList()->AddRect(r.Min - ImVec2(1, 1), r.Max + ImVec2(1, 1), IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
                    }
                }
                Unindent();
            }
        }
        Checkbox("Show groups rectangles", &g.DebugShowGroupRects); // Storing in context as this is used by group code and prefers to be in hot-data

        SeparatorText("Validate");

        Checkbox("Debug Begin/BeginChild return value", &io.ConfigDebugBeginReturnValueLoop);
        SameLine();
        MetricsHelpMarker("Some calls to Begin()/BeginChild() will return false.\n\nWill cycle through window depths then repeat. Windows should be flickering while running.");

        Checkbox("UTF-8 Encoding viewer", &cfg->ShowTextEncodingViewer);
        SameLine();
        MetricsHelpMarker("You can also call ImGui::DebugTextEncoding() from your code with a given string to test that your UTF-8 encoding settings are correct.");
        if (cfg->ShowTextEncodingViewer)
        {
            static char buf[64] = "";
            SetNextItemWidth(-FLT_MIN);
            InputText("##DebugTextEncodingBuf", buf, IM_ARRAYSIZE(buf));
            if (buf[0] != 0)
                DebugTextEncoding(buf);
        }

        TreePop();
    }

    // Windows
    if (TreeNode("Windows", "Windows (%d)", g.Windows.Size))
    {
        //SetNextItemOpen(true, ImGuiCond_Once);
        DebugNodeWindowsList(&g.Windows, "By display order");
        DebugNodeWindowsList(&g.WindowsFocusOrder, "By focus order (root windows)");
        if (TreeNode("By submission order (begin stack)"))
        {
            // Here we display windows in their submitted order/hierarchy, however note that the Begin stack doesn't constitute a Parent<>Child relationship!
            ImVector<ImGuiWindow*>& temp_buffer = g.WindowsTempSortBuffer;
            temp_buffer.resize(0);
            for (ImGuiWindow* window : g.Windows)
                if (window->LastFrameActive + 1 >= g.FrameCount)
                    temp_buffer.push_back(window);
            struct Func { static int IMGUI_CDECL WindowComparerByBeginOrder(const void* lhs, const void* rhs) { return ((int)(*(const ImGuiWindow* const *)lhs)->BeginOrderWithinContext - (*(const ImGuiWindow* const*)rhs)->BeginOrderWithinContext); } };
            ImQsort(temp_buffer.Data, (size_t)temp_buffer.Size, sizeof(ImGuiWindow*), Func::WindowComparerByBeginOrder);
            DebugNodeWindowsListByBeginStackParent(temp_buffer.Data, temp_buffer.Size, NULL);
            TreePop();
        }

        TreePop();
    }

    // DrawLists
    int drawlist_count = 0;
    for (ImGuiViewportP* viewport : g.Viewports)
        drawlist_count += viewport->DrawDataP.CmdLists.Size;
    if (TreeNode("DrawLists", "DrawLists (%d)", drawlist_count))
    {
        Checkbox("Show ImDrawCmd mesh when hovering", &cfg->ShowDrawCmdMesh);
        Checkbox("Show ImDrawCmd bounding boxes when hovering", &cfg->ShowDrawCmdBoundingBoxes);
        for (ImGuiViewportP* viewport : g.Viewports)
            for (ImDrawList* draw_list : viewport->DrawDataP.CmdLists)
                DebugNodeDrawList(NULL, viewport, draw_list, "DrawList");
        TreePop();
    }

    // Viewports
    if (TreeNode("Viewports", "Viewports (%d)", g.Viewports.Size))
    {
        SetNextItemOpen(true, ImGuiCond_Once);
        if (TreeNode("Windows Minimap"))
        {
            RenderViewportsThumbnails();
            TreePop();
        }
        cfg->HighlightViewportID = 0;

        for (ImGuiViewportP* viewport : g.Viewports)
            DebugNodeViewport(viewport);
        TreePop();
    }

    // Details for Popups
    if (TreeNode("Popups", "Popups (%d)", g.OpenPopupStack.Size))
    {
        for (const ImGuiPopupData& popup_data : g.OpenPopupStack)
        {
            // As it's difficult to interact with tree nodes while popups are open, we display everything inline.
            ImGuiWindow* window = popup_data.Window;
            BulletText("PopupID: %08x, Window: '%s' (%s%s), RestoreNavWindow '%s', ParentWindow '%s'",
                popup_data.PopupId, window ? window->Name : "NULL", window && (window->Flags & ImGuiWindowFlags_ChildWindow) ? "Child;" : "", window && (window->Flags & ImGuiWindowFlags_ChildMenu) ? "Menu;" : "",
                popup_data.RestoreNavWindow ? popup_data.RestoreNavWindow->Name : "NULL", window && window->ParentWindow ? window->ParentWindow->Name : "NULL");
        }
        TreePop();
    }

    // Details for TabBars
    if (TreeNode("TabBars", "Tab Bars (%d)", g.TabBars.GetAliveCount()))
    {
        for (int n = 0; n < g.TabBars.GetMapSize(); n++)
            if (ImGuiTabBar* tab_bar = g.TabBars.TryGetMapData(n))
            {
                PushID(tab_bar);
                DebugNodeTabBar(tab_bar, "TabBar");
                PopID();
            }
        TreePop();
    }

    // Details for Tables
    if (TreeNode("Tables", "Tables (%d)", g.Tables.GetAliveCount()))
    {
        for (int n = 0; n < g.Tables.GetMapSize(); n++)
            if (ImGuiTable* table = g.Tables.TryGetMapData(n))
                DebugNodeTable(table);
        TreePop();
    }

    // Details for Fonts
    ImFontAtlas* atlas = g.IO.Fonts;
    if (TreeNode("Fonts", "Fonts (%d)", atlas->Fonts.Size))
    {
        ShowFontAtlas(atlas);
        TreePop();
    }

    // Details for InputText
    if (TreeNode("InputText"))
    {
        DebugNodeInputTextState(&g.InputTextState);
        TreePop();
    }

    // Details for TypingSelect
    if (TreeNode("TypingSelect", "TypingSelect (%d)", g.TypingSelectState.SearchBuffer[0] != 0 ? 1 : 0))
    {
        DebugNodeTypingSelectState(&g.TypingSelectState);
        TreePop();
    }

    // Details for MultiSelect
    if (TreeNode("MultiSelect", "MultiSelect (%d)", g.MultiSelectStorage.GetAliveCount()))
    {
        ImGuiBoxSelectState* bs = &g.BoxSelectState;
        BulletText("BoxSelect ID=0x%08X, Starting = %d, Active %d", bs->ID, bs->IsStarting, bs->IsActive);
        for (int n = 0; n < g.MultiSelectStorage.GetMapSize(); n++)
            if (ImGuiMultiSelectState* state = g.MultiSelectStorage.TryGetMapData(n))
                DebugNodeMultiSelectState(state);
        TreePop();
    }

    // Details for Docking
#ifdef IMGUI_HAS_DOCK
    if (TreeNode("Docking"))
    {
        TreePop();
    }
#endif // #ifdef IMGUI_HAS_DOCK

    // Settings
    if (TreeNode("Settings"))
    {
        if (SmallButton("Clear"))
            ClearIniSettings();
        SameLine();
        if (SmallButton("Save to memory"))
            SaveIniSettingsToMemory();
        SameLine();
        if (SmallButton("Save to disk"))
            SaveIniSettingsToDisk(g.IO.IniFilename);
        SameLine();
        if (g.IO.IniFilename)
            Text("\"%s\"", g.IO.IniFilename);
        else
            TextUnformatted("<NULL>");
        Checkbox("io.ConfigDebugIniSettings", &io.ConfigDebugIniSettings);
        Text("SettingsDirtyTimer %.2f", g.SettingsDirtyTimer);
        if (TreeNode("SettingsHandlers", "Settings handlers: (%d)", g.SettingsHandlers.Size))
        {
            for (ImGuiSettingsHandler& handler : g.SettingsHandlers)
                BulletText("\"%s\"", handler.TypeName);
            TreePop();
        }
        if (TreeNode("SettingsWindows", "Settings packed data: Windows: %d bytes", g.SettingsWindows.size()))
        {
            for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
                DebugNodeWindowSettings(settings);
            TreePop();
        }

        if (TreeNode("SettingsTables", "Settings packed data: Tables: %d bytes", g.SettingsTables.size()))
        {
            for (ImGuiTableSettings* settings = g.SettingsTables.begin(); settings != NULL; settings = g.SettingsTables.next_chunk(settings))
                DebugNodeTableSettings(settings);
            TreePop();
        }

#ifdef IMGUI_HAS_DOCK
#endif // #ifdef IMGUI_HAS_DOCK

        if (TreeNode("SettingsIniData", "Settings unpacked data (.ini): %d bytes", g.SettingsIniData.size()))
        {
            InputTextMultiline("##Ini", (char*)(void*)g.SettingsIniData.c_str(), g.SettingsIniData.Buf.Size, ImVec2(-FLT_MIN, GetTextLineHeight() * 20), ImGuiInputTextFlags_ReadOnly);
            TreePop();
        }
        TreePop();
    }

    // Settings
    if (TreeNode("Memory allocations"))
    {
        ImGuiDebugAllocInfo* info = &g.DebugAllocInfo;
        Text("%d current allocations", info->TotalAllocCount - info->TotalFreeCount);
        if (SmallButton("GC now")) { g.GcCompactAll = true; }
        Text("Recent frames with allocations:");
        int buf_size = IM_ARRAYSIZE(info->LastEntriesBuf);
        for (int n = buf_size - 1; n >= 0; n--)
        {
            ImGuiDebugAllocEntry* entry = &info->LastEntriesBuf[(info->LastEntriesIdx - n + buf_size) % buf_size];
            BulletText("Frame %06d: %+3d ( %2d alloc, %2d free )", entry->FrameCount, entry->AllocCount - entry->FreeCount, entry->AllocCount, entry->FreeCount);
            if (n == 0)
            {
                SameLine();
                Text("<- %d frames ago", g.FrameCount - entry->FrameCount);
            }
        }
        TreePop();
    }

    if (TreeNode("Inputs"))
    {
        Text("KEYBOARD/GAMEPAD/MOUSE KEYS");
        {
            // We iterate both legacy native range and named ImGuiKey ranges, which is a little odd but this allows displaying the data for old/new backends.
            // User code should never have to go through such hoops! You can generally iterate between ImGuiKey_NamedKey_BEGIN and ImGuiKey_NamedKey_END.
            Indent();
#ifdef IMGUI_DISABLE_OBSOLETE_KEYIO
            struct funcs { static bool IsLegacyNativeDupe(ImGuiKey) { return false; } };
#else
            struct funcs { static bool IsLegacyNativeDupe(ImGuiKey key) { return key >= 0 && key < 512 && GetIO().KeyMap[key] != -1; } }; // Hide Native<>ImGuiKey duplicates when both exists in the array
            //Text("Legacy raw:");      for (ImGuiKey key = ImGuiKey_KeysData_OFFSET; key < ImGuiKey_COUNT; key++) { if (io.KeysDown[key]) { SameLine(); Text("\"%s\" %d", GetKeyName(key), key); } }
#endif
            Text("Keys down:");         for (ImGuiKey key = ImGuiKey_KeysData_OFFSET; key < ImGuiKey_COUNT; key = (ImGuiKey)(key + 1)) { if (funcs::IsLegacyNativeDupe(key) || !IsKeyDown(key)) continue;     SameLine(); Text(IsNamedKey(key) ? "\"%s\"" : "\"%s\" %d", GetKeyName(key), key); SameLine(); Text("(%.02f)", GetKeyData(key)->DownDuration); }
            Text("Keys pressed:");      for (ImGuiKey key = ImGuiKey_KeysData_OFFSET; key < ImGuiKey_COUNT; key = (ImGuiKey)(key + 1)) { if (funcs::IsLegacyNativeDupe(key) || !IsKeyPressed(key)) continue;  SameLine(); Text(IsNamedKey(key) ? "\"%s\"" : "\"%s\" %d", GetKeyName(key), key); }
            Text("Keys released:");     for (ImGuiKey key = ImGuiKey_KeysData_OFFSET; key < ImGuiKey_COUNT; key = (ImGuiKey)(key + 1)) { if (funcs::IsLegacyNativeDupe(key) || !IsKeyReleased(key)) continue; SameLine(); Text(IsNamedKey(key) ? "\"%s\"" : "\"%s\" %d", GetKeyName(key), key); }
            Text("Keys mods: %s%s%s%s", io.KeyCtrl ? "CTRL " : "", io.KeyShift ? "SHIFT " : "", io.KeyAlt ? "ALT " : "", io.KeySuper ? "SUPER " : "");
            Text("Chars queue:");       for (int i = 0; i < io.InputQueueCharacters.Size; i++) { ImWchar c = io.InputQueueCharacters[i]; SameLine(); Text("\'%c\' (0x%04X)", (c > ' ' && c <= 255) ? (char)c : '?', c); } // FIXME: We should convert 'c' to UTF-8 here but the functions are not public.
            DebugRenderKeyboardPreview(GetWindowDrawList());
            Unindent();
        }

        Text("MOUSE STATE");
        {
            Indent();
            if (IsMousePosValid())
                Text("Mouse pos: (%g, %g)", io.MousePos.x, io.MousePos.y);
            else
                Text("Mouse pos: <INVALID>");
            Text("Mouse delta: (%g, %g)", io.MouseDelta.x, io.MouseDelta.y);
            int count = IM_ARRAYSIZE(io.MouseDown);
            Text("Mouse down:");     for (int i = 0; i < count; i++) if (IsMouseDown(i)) { SameLine(); Text("b%d (%.02f secs)", i, io.MouseDownDuration[i]); }
            Text("Mouse clicked:");  for (int i = 0; i < count; i++) if (IsMouseClicked(i)) { SameLine(); Text("b%d (%d)", i, io.MouseClickedCount[i]); }
            Text("Mouse released:"); for (int i = 0; i < count; i++) if (IsMouseReleased(i)) { SameLine(); Text("b%d", i); }
            Text("Mouse wheel: %.1f", io.MouseWheel);
            Text("MouseStationaryTimer: %.2f", g.MouseStationaryTimer);
            Text("Mouse source: %s", GetMouseSourceName(io.MouseSource));
            Text("Pen Pressure: %.1f", io.PenPressure); // Note: currently unused
            Unindent();
        }

        Text("MOUSE WHEELING");
        {
            Indent();
            Text("WheelingWindow: '%s'", g.WheelingWindow ? g.WheelingWindow->Name : "NULL");
            Text("WheelingWindowReleaseTimer: %.2f", g.WheelingWindowReleaseTimer);
            Text("WheelingAxisAvg[] = { %.3f, %.3f }, Main Axis: %s", g.WheelingAxisAvg.x, g.WheelingAxisAvg.y, (g.WheelingAxisAvg.x > g.WheelingAxisAvg.y) ? "X" : (g.WheelingAxisAvg.x < g.WheelingAxisAvg.y) ? "Y" : "<none>");
            Unindent();
        }

        Text("KEY OWNERS");
        {
            Indent();
            if (BeginChild("##owners", ImVec2(-FLT_MIN, GetTextLineHeightWithSpacing() * 8), ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeY, ImGuiWindowFlags_NoSavedSettings))
                for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1))
                {
                    ImGuiKeyOwnerData* owner_data = GetKeyOwnerData(&g, key);
                    if (owner_data->OwnerCurr == ImGuiKeyOwner_NoOwner)
                        continue;
                    Text("%s: 0x%08X%s", GetKeyName(key), owner_data->OwnerCurr,
                        owner_data->LockUntilRelease ? " LockUntilRelease" : owner_data->LockThisFrame ? " LockThisFrame" : "");
                    DebugLocateItemOnHover(owner_data->OwnerCurr);
                }
            EndChild();
            Unindent();
        }
        Text("SHORTCUT ROUTING");
        SameLine();
        MetricsHelpMarker("Declared shortcut routes automatically set key owner when mods matches.");
        {
            Indent();
            if (BeginChild("##routes", ImVec2(-FLT_MIN, GetTextLineHeightWithSpacing() * 8), ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeY, ImGuiWindowFlags_NoSavedSettings))
                for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1))
                {
                    ImGuiKeyRoutingTable* rt = &g.KeysRoutingTable;
                    for (ImGuiKeyRoutingIndex idx = rt->Index[key - ImGuiKey_NamedKey_BEGIN]; idx != -1; )
                    {
                        ImGuiKeyRoutingData* routing_data = &rt->Entries[idx];
                        ImGuiKeyChord key_chord = key | routing_data->Mods;
                        Text("%s: 0x%08X (scored %d)", GetKeyChordName(key_chord), routing_data->RoutingCurr, routing_data->RoutingCurrScore);
                        DebugLocateItemOnHover(routing_data->RoutingCurr);
                        if (g.IO.ConfigDebugIsDebuggerPresent)
                        {
                            SameLine();
                            if (DebugBreakButton("**DebugBreak**", "in SetShortcutRouting() for this KeyChord"))
                                g.DebugBreakInShortcutRouting = key_chord;
                        }
                        idx = routing_data->NextEntryIndex;
                    }
                }
            EndChild();
            Text("(ActiveIdUsing: AllKeyboardKeys: %d, NavDirMask: 0x%X)", g.ActiveIdUsingAllKeyboardKeys, g.ActiveIdUsingNavDirMask);
            Unindent();
        }
        TreePop();
    }

    if (TreeNode("Internal state"))
    {
        Text("WINDOWING");
        Indent();
        Text("HoveredWindow: '%s'", g.HoveredWindow ? g.HoveredWindow->Name : "NULL");
        Text("HoveredWindow->Root: '%s'", g.HoveredWindow ? g.HoveredWindow->RootWindow->Name : "NULL");
        Text("HoveredWindowUnderMovingWindow: '%s'", g.HoveredWindowUnderMovingWindow ? g.HoveredWindowUnderMovingWindow->Name : "NULL");
        Text("MovingWindow: '%s'", g.MovingWindow ? g.MovingWindow->Name : "NULL");
        Unindent();

        Text("ITEMS");
        Indent();
        Text("ActiveId: 0x%08X/0x%08X (%.2f sec), AllowOverlap: %d, Source: %s", g.ActiveId, g.ActiveIdPreviousFrame, g.ActiveIdTimer, g.ActiveIdAllowOverlap, GetInputSourceName(g.ActiveIdSource));
        DebugLocateItemOnHover(g.ActiveId);
        Text("ActiveIdWindow: '%s'", g.ActiveIdWindow ? g.ActiveIdWindow->Name : "NULL");
        Text("ActiveIdUsing: AllKeyboardKeys: %d, NavDirMask: %X", g.ActiveIdUsingAllKeyboardKeys, g.ActiveIdUsingNavDirMask);
        Text("HoveredId: 0x%08X (%.2f sec), AllowOverlap: %d", g.HoveredIdPreviousFrame, g.HoveredIdTimer, g.HoveredIdAllowOverlap); // Not displaying g.HoveredId as it is update mid-frame
        Text("HoverItemDelayId: 0x%08X, Timer: %.2f, ClearTimer: %.2f", g.HoverItemDelayId, g.HoverItemDelayTimer, g.HoverItemDelayClearTimer);
        Text("DragDrop: %d, SourceId = 0x%08X, Payload \"%s\" (%d bytes)", g.DragDropActive, g.DragDropPayload.SourceId, g.DragDropPayload.DataType, g.DragDropPayload.DataSize);
        DebugLocateItemOnHover(g.DragDropPayload.SourceId);
        Unindent();

        Text("NAV,FOCUS");
        Indent();
        Text("NavWindow: '%s'", g.NavWindow ? g.NavWindow->Name : "NULL");
        Text("NavId: 0x%08X, NavLayer: %d", g.NavId, g.NavLayer);
        DebugLocateItemOnHover(g.NavId);
        Text("NavInputSource: %s", GetInputSourceName(g.NavInputSource));
        Text("NavLastValidSelectionUserData = %" IM_PRId64 " (0x%" IM_PRIX64 ")", g.NavLastValidSelectionUserData, g.NavLastValidSelectionUserData);
        Text("NavActive: %d, NavVisible: %d", g.IO.NavActive, g.IO.NavVisible);
        Text("NavActivateId/DownId/PressedId: %08X/%08X/%08X", g.NavActivateId, g.NavActivateDownId, g.NavActivatePressedId);
        Text("NavActivateFlags: %04X", g.NavActivateFlags);
        Text("NavDisableHighlight: %d, NavDisableMouseHover: %d", g.NavDisableHighlight, g.NavDisableMouseHover);
        Text("NavFocusScopeId = 0x%08X", g.NavFocusScopeId);
        Text("NavFocusRoute[] = ");
        for (int path_n = g.NavFocusRoute.Size - 1; path_n >= 0; path_n--)
        {
            const ImGuiFocusScopeData& focus_scope = g.NavFocusRoute[path_n];
            SameLine(0.0f, 0.0f);
            Text("0x%08X/", focus_scope.ID);
            SetItemTooltip("In window \"%s\"", FindWindowByID(focus_scope.WindowID)->Name);
        }
        Text("NavWindowingTarget: '%s'", g.NavWindowingTarget ? g.NavWindowingTarget->Name : "NULL");
        Unindent();

        TreePop();
    }

    // Overlay: Display windows Rectangles and Begin Order
    if (cfg->ShowWindowsRects || cfg->ShowWindowsBeginOrder)
    {
        for (ImGuiWindow* window : g.Windows)
        {
            if (!window->WasActive)
                continue;
            ImDrawList* draw_list = GetForegroundDrawList(window);
            if (cfg->ShowWindowsRects)
            {
                ImRect r = Funcs::GetWindowRect(window, cfg->ShowWindowsRectsType);
                draw_list->AddRect(r.Min, r.Max, IM_COL32(255, 0, 128, 255));
            }
            if (cfg->ShowWindowsBeginOrder && !(window->Flags & ImGuiWindowFlags_ChildWindow))
            {
                char buf[32];
                ImFormatString(buf, IM_ARRAYSIZE(buf), "%d", window->BeginOrderWithinContext);
                float font_size = GetFontSize();
                draw_list->AddRectFilled(window->Pos, window->Pos + ImVec2(font_size, font_size), IM_COL32(200, 100, 100, 255));
                draw_list->AddText(window->Pos, IM_COL32(255, 255, 255, 255), buf);
            }
        }
    }

    // Overlay: Display Tables Rectangles
    if (cfg->ShowTablesRects)
    {
        for (int table_n = 0; table_n < g.Tables.GetMapSize(); table_n++)
        {
            ImGuiTable* table = g.Tables.TryGetMapData(table_n);
            if (table == NULL || table->LastFrameActive < g.FrameCount - 1)
                continue;
            ImDrawList* draw_list = GetForegroundDrawList(table->OuterWindow);
            if (cfg->ShowTablesRectsType >= TRT_ColumnsRect)
            {
                for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
                {
                    ImRect r = Funcs::GetTableRect(table, cfg->ShowTablesRectsType, column_n);
                    ImU32 col = (table->HoveredColumnBody == column_n) ? IM_COL32(255, 255, 128, 255) : IM_COL32(255, 0, 128, 255);
                    float thickness = (table->HoveredColumnBody == column_n) ? 3.0f : 1.0f;
                    draw_list->AddRect(r.Min, r.Max, col, 0.0f, 0, thickness);
                }
            }
            else
            {
                ImRect r = Funcs::GetTableRect(table, cfg->ShowTablesRectsType, -1);
                draw_list->AddRect(r.Min, r.Max, IM_COL32(255, 0, 128, 255));
            }
        }
    }

#ifdef IMGUI_HAS_DOCK
    // Overlay: Display Docking info
    if (show_docking_nodes && g.IO.KeyCtrl)
    {
    }
#endif // #ifdef IMGUI_HAS_DOCK

    End();
}

void ImGui::DebugBreakClearData()
{
    // Those fields are scattered in their respective subsystem to stay in hot-data locations
    ImGuiContext& g = *GImGui;
    g.DebugBreakInWindow = 0;
    g.DebugBreakInTable = 0;
    g.DebugBreakInShortcutRouting = ImGuiKey_None;
}

void ImGui::DebugBreakButtonTooltip(bool keyboard_only, const char* description_of_location)
{
    if (!BeginItemTooltip())
        return;
    Text("To call IM_DEBUG_BREAK() %s:", description_of_location);
    Separator();
    TextUnformatted(keyboard_only ? "- Press 'Pause/Break' on keyboard." : "- Press 'Pause/Break' on keyboard.\n- or Click (may alter focus/active id).\n- or navigate using keyboard and press space.");
    Separator();
    TextUnformatted("Choose one way that doesn't interfere with what you are trying to debug!\nYou need a debugger attached or this will crash!");
    EndTooltip();
}

// Special button that doesn't take focus, doesn't take input owner, and can be activated without a click etc.
// In order to reduce interferences with the contents we are trying to debug into.
bool ImGui::DebugBreakButton(const char* label, const char* description_of_location)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    ImVec2 pos = window->DC.CursorPos + ImVec2(0.0f, window->DC.CurrLineTextBaseOffset);
    ImVec2 size = ImVec2(label_size.x + g.Style.FramePadding.x * 2.0f, label_size.y);

    const ImRect bb(pos, pos + size);
    ItemSize(size, 0.0f);
    if (!ItemAdd(bb, id))
        return false;

    // WE DO NOT USE ButtonEx() or ButtonBehavior() in order to reduce our side-effects.
    bool hovered = ItemHoverable(bb, id, g.CurrentItemFlags);
    bool pressed = hovered && (IsKeyChordPressed(g.DebugBreakKeyChord) || IsMouseClicked(0) || g.NavActivateId == id);
    DebugBreakButtonTooltip(false, description_of_location);

    ImVec4 col4f = GetStyleColorVec4(hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    ImVec4 hsv;
    ColorConvertRGBtoHSV(col4f.x, col4f.y, col4f.z, hsv.x, hsv.y, hsv.z);
    ColorConvertHSVtoRGB(hsv.x + 0.20f, hsv.y, hsv.z, col4f.x, col4f.y, col4f.z);

    RenderNavHighlight(bb, id);
    RenderFrame(bb.Min, bb.Max, GetColorU32(col4f), true, g.Style.FrameRounding);
    RenderTextClipped(bb.Min, bb.Max, label, NULL, &label_size, g.Style.ButtonTextAlign, &bb);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

// [DEBUG] Display contents of Columns
void ImGui::DebugNodeColumns(ImGuiOldColumns* columns)
{
    if (!TreeNode((void*)(uintptr_t)columns->ID, "Columns Id: 0x%08X, Count: %d, Flags: 0x%04X", columns->ID, columns->Count, columns->Flags))
        return;
    BulletText("Width: %.1f (MinX: %.1f, MaxX: %.1f)", columns->OffMaxX - columns->OffMinX, columns->OffMinX, columns->OffMaxX);
    for (ImGuiOldColumnData& column : columns->Columns)
        BulletText("Column %02d: OffsetNorm %.3f (= %.1f px)", (int)columns->Columns.index_from_ptr(&column), column.OffsetNorm, GetColumnOffsetFromNorm(columns, column.OffsetNorm));
    TreePop();
}

static void FormatTextureIDForDebugDisplay(char* buf, int buf_size, ImTextureID tex_id)
{
    union { void* ptr; int integer; } tex_id_opaque;
    memcpy(&tex_id_opaque, &tex_id, ImMin(sizeof(void*), sizeof(tex_id)));
    if (sizeof(tex_id) >= sizeof(void*))
        ImFormatString(buf, buf_size, "0x%p", tex_id_opaque.ptr);
    else
        ImFormatString(buf, buf_size, "0x%04X", tex_id_opaque.integer);
}

// [DEBUG] Display contents of ImDrawList
void ImGui::DebugNodeDrawList(ImGuiWindow* window, ImGuiViewportP* viewport, const ImDrawList* draw_list, const char* label)
{
    ImGuiContext& g = *GImGui;
    IM_UNUSED(viewport); // Used in docking branch
    ImGuiMetricsConfig* cfg = &g.DebugMetricsConfig;
    int cmd_count = draw_list->CmdBuffer.Size;
    if (cmd_count > 0 && draw_list->CmdBuffer.back().ElemCount == 0 && draw_list->CmdBuffer.back().UserCallback == NULL)
        cmd_count--;
    bool node_open = TreeNode(draw_list, "%s: '%s' %d vtx, %d indices, %d cmds", label, draw_list->_OwnerName ? draw_list->_OwnerName : "", draw_list->VtxBuffer.Size, draw_list->IdxBuffer.Size, cmd_count);
    if (draw_list == GetWindowDrawList())
    {
        SameLine();
        TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "CURRENTLY APPENDING"); // Can't display stats for active draw list! (we don't have the data double-buffered)
        if (node_open)
            TreePop();
        return;
    }

    ImDrawList* fg_draw_list = GetForegroundDrawList(window); // Render additional visuals into the top-most draw list
    if (window && IsItemHovered() && fg_draw_list)
        fg_draw_list->AddRect(window->Pos, window->Pos + window->Size, IM_COL32(255, 255, 0, 255));
    if (!node_open)
        return;

    if (window && !window->WasActive)
        TextDisabled("Warning: owning Window is inactive. This DrawList is not being rendered!");

    for (const ImDrawCmd* pcmd = draw_list->CmdBuffer.Data; pcmd < draw_list->CmdBuffer.Data + cmd_count; pcmd++)
    {
        if (pcmd->UserCallback)
        {
            BulletText("Callback %p, user_data %p", pcmd->UserCallback, pcmd->UserCallbackData);
            continue;
        }

        char texid_desc[20];
        FormatTextureIDForDebugDisplay(texid_desc, IM_ARRAYSIZE(texid_desc), pcmd->TextureId);
        char buf[300];
        ImFormatString(buf, IM_ARRAYSIZE(buf), "DrawCmd:%5d tris, Tex %s, ClipRect (%4.0f,%4.0f)-(%4.0f,%4.0f)",
            pcmd->ElemCount / 3, texid_desc, pcmd->ClipRect.x, pcmd->ClipRect.y, pcmd->ClipRect.z, pcmd->ClipRect.w);
        bool pcmd_node_open = TreeNode((void*)(pcmd - draw_list->CmdBuffer.begin()), "%s", buf);
        if (IsItemHovered() && (cfg->ShowDrawCmdMesh || cfg->ShowDrawCmdBoundingBoxes) && fg_draw_list)
            DebugNodeDrawCmdShowMeshAndBoundingBox(fg_draw_list, draw_list, pcmd, cfg->ShowDrawCmdMesh, cfg->ShowDrawCmdBoundingBoxes);
        if (!pcmd_node_open)
            continue;

        // Calculate approximate coverage area (touched pixel count)
        // This will be in pixels squared as long there's no post-scaling happening to the renderer output.
        const ImDrawIdx* idx_buffer = (draw_list->IdxBuffer.Size > 0) ? draw_list->IdxBuffer.Data : NULL;
        const ImDrawVert* vtx_buffer = draw_list->VtxBuffer.Data + pcmd->VtxOffset;
        float total_area = 0.0f;
        for (unsigned int idx_n = pcmd->IdxOffset; idx_n < pcmd->IdxOffset + pcmd->ElemCount; )
        {
            ImVec2 triangle[3];
            for (int n = 0; n < 3; n++, idx_n++)
                triangle[n] = vtx_buffer[idx_buffer ? idx_buffer[idx_n] : idx_n].pos;
            total_area += ImTriangleArea(triangle[0], triangle[1], triangle[2]);
        }

        // Display vertex information summary. Hover to get all triangles drawn in wire-frame
        ImFormatString(buf, IM_ARRAYSIZE(buf), "Mesh: ElemCount: %d, VtxOffset: +%d, IdxOffset: +%d, Area: ~%0.f px", pcmd->ElemCount, pcmd->VtxOffset, pcmd->IdxOffset, total_area);
        Selectable(buf);
        if (IsItemHovered() && fg_draw_list)
            DebugNodeDrawCmdShowMeshAndBoundingBox(fg_draw_list, draw_list, pcmd, true, false);

        // Display individual triangles/vertices. Hover on to get the corresponding triangle highlighted.
        ImGuiListClipper clipper;
        clipper.Begin(pcmd->ElemCount / 3); // Manually coarse clip our print out of individual vertices to save CPU, only items that may be visible.
        while (clipper.Step())
            for (int prim = clipper.DisplayStart, idx_i = pcmd->IdxOffset + clipper.DisplayStart * 3; prim < clipper.DisplayEnd; prim++)
            {
                char* buf_p = buf, * buf_end = buf + IM_ARRAYSIZE(buf);
                ImVec2 triangle[3];
                for (int n = 0; n < 3; n++, idx_i++)
                {
                    const ImDrawVert& v = vtx_buffer[idx_buffer ? idx_buffer[idx_i] : idx_i];
                    triangle[n] = v.pos;
                    buf_p += ImFormatString(buf_p, buf_end - buf_p, "%s %04d: pos (%8.2f,%8.2f), uv (%.6f,%.6f), col %08X\n",
                        (n == 0) ? "Vert:" : "     ", idx_i, v.pos.x, v.pos.y, v.uv.x, v.uv.y, v.col);
                }

                Selectable(buf, false);
                if (fg_draw_list && IsItemHovered())
                {
                    ImDrawListFlags backup_flags = fg_draw_list->Flags;
                    fg_draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines; // Disable AA on triangle outlines is more readable for very large and thin triangles.
                    fg_draw_list->AddPolyline(triangle, 3, IM_COL32(255, 255, 0, 255), ImDrawFlags_Closed, 1.0f);
                    fg_draw_list->Flags = backup_flags;
                }
            }
        TreePop();
    }
    TreePop();
}

// [DEBUG] Display mesh/aabb of a ImDrawCmd
void ImGui::DebugNodeDrawCmdShowMeshAndBoundingBox(ImDrawList* out_draw_list, const ImDrawList* draw_list, const ImDrawCmd* draw_cmd, bool show_mesh, bool show_aabb)
{
    IM_ASSERT(show_mesh || show_aabb);

    // Draw wire-frame version of all triangles
    ImRect clip_rect = draw_cmd->ClipRect;
    ImRect vtxs_rect(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    ImDrawListFlags backup_flags = out_draw_list->Flags;
    out_draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines; // Disable AA on triangle outlines is more readable for very large and thin triangles.
    for (unsigned int idx_n = draw_cmd->IdxOffset, idx_end = draw_cmd->IdxOffset + draw_cmd->ElemCount; idx_n < idx_end; )
    {
        ImDrawIdx* idx_buffer = (draw_list->IdxBuffer.Size > 0) ? draw_list->IdxBuffer.Data : NULL; // We don't hold on those pointers past iterations as ->AddPolyline() may invalidate them if out_draw_list==draw_list
        ImDrawVert* vtx_buffer = draw_list->VtxBuffer.Data + draw_cmd->VtxOffset;

        ImVec2 triangle[3];
        for (int n = 0; n < 3; n++, idx_n++)
            vtxs_rect.Add((triangle[n] = vtx_buffer[idx_buffer ? idx_buffer[idx_n] : idx_n].pos));
        if (show_mesh)
            out_draw_list->AddPolyline(triangle, 3, IM_COL32(255, 255, 0, 255), ImDrawFlags_Closed, 1.0f); // In yellow: mesh triangles
    }
    // Draw bounding boxes
    if (show_aabb)
    {
        out_draw_list->AddRect(ImTrunc(clip_rect.Min), ImTrunc(clip_rect.Max), IM_COL32(255, 0, 255, 255)); // In pink: clipping rectangle submitted to GPU
        out_draw_list->AddRect(ImTrunc(vtxs_rect.Min), ImTrunc(vtxs_rect.Max), IM_COL32(0, 255, 255, 255)); // In cyan: bounding box of triangles
    }
    out_draw_list->Flags = backup_flags;
}

// [DEBUG] Display details for a single font, called by ShowStyleEditor().
void ImGui::DebugNodeFont(ImFont* font)
{
    bool opened = TreeNode(font, "Font: \"%s\"\n%.2f px, %d glyphs, %d file(s)",
        font->ConfigData ? font->ConfigData[0].Name : "", font->FontSize, font->Glyphs.Size, font->ConfigDataCount);
    SameLine();
    if (SmallButton("Set as default"))
        GetIO().FontDefault = font;
    if (!opened)
        return;

    // Display preview text
    PushFont(font);
    Text("The quick brown fox jumps over the lazy dog");
    PopFont();

    // Display details
    SetNextItemWidth(GetFontSize() * 8);
    DragFloat("Font scale", &font->Scale, 0.005f, 0.3f, 2.0f, "%.1f");
    SameLine(); MetricsHelpMarker(
        "Note that the default embedded font is NOT meant to be scaled.\n\n"
        "Font are currently rendered into bitmaps at a given size at the time of building the atlas. "
        "You may oversample them to get some flexibility with scaling. "
        "You can also render at multiple sizes and select which one to use at runtime.\n\n"
        "(Glimmer of hope: the atlas system will be rewritten in the future to make scaling more flexible.)");
    Text("Ascent: %f, Descent: %f, Height: %f", font->Ascent, font->Descent, font->Ascent - font->Descent);
    char c_str[5];
    Text("Fallback character: '%s' (U+%04X)", ImTextCharToUtf8(c_str, font->FallbackChar), font->FallbackChar);
    Text("Ellipsis character: '%s' (U+%04X)", ImTextCharToUtf8(c_str, font->EllipsisChar), font->EllipsisChar);
    const int surface_sqrt = (int)ImSqrt((float)font->MetricsTotalSurface);
    Text("Texture Area: about %d px ~%dx%d px", font->MetricsTotalSurface, surface_sqrt, surface_sqrt);
    for (int config_i = 0; config_i < font->ConfigDataCount; config_i++)
        if (font->ConfigData)
            if (const ImFontConfig* cfg = &font->ConfigData[config_i])
                BulletText("Input %d: \'%s\', Oversample: (%d,%d), PixelSnapH: %d, Offset: (%.1f,%.1f)",
                    config_i, cfg->Name, cfg->OversampleH, cfg->OversampleV, cfg->PixelSnapH, cfg->GlyphOffset.x, cfg->GlyphOffset.y);

    // Display all glyphs of the fonts in separate pages of 256 characters
    if (TreeNode("Glyphs", "Glyphs (%d)", font->Glyphs.Size))
    {
        ImDrawList* draw_list = GetWindowDrawList();
        const ImU32 glyph_col = GetColorU32(ImGuiCol_Text);
        const float cell_size = font->FontSize * 1;
        const float cell_spacing = GetStyle().ItemSpacing.y;
        for (unsigned int base = 0; base <= IM_UNICODE_CODEPOINT_MAX; base += 256)
        {
            // Skip ahead if a large bunch of glyphs are not present in the font (test in chunks of 4k)
            // This is only a small optimization to reduce the number of iterations when IM_UNICODE_MAX_CODEPOINT
            // is large // (if ImWchar==ImWchar32 we will do at least about 272 queries here)
            if (!(base & 4095) && font->IsGlyphRangeUnused(base, base + 4095))
            {
                base += 4096 - 256;
                continue;
            }

            int count = 0;
            for (unsigned int n = 0; n < 256; n++)
                if (font->FindGlyphNoFallback((ImWchar)(base + n)))
                    count++;
            if (count <= 0)
                continue;
            if (!TreeNode((void*)(intptr_t)base, "U+%04X..U+%04X (%d %s)", base, base + 255, count, count > 1 ? "glyphs" : "glyph"))
                continue;

            // Draw a 16x16 grid of glyphs
            ImVec2 base_pos = GetCursorScreenPos();
            for (unsigned int n = 0; n < 256; n++)
            {
                // We use ImFont::RenderChar as a shortcut because we don't have UTF-8 conversion functions
                // available here and thus cannot easily generate a zero-terminated UTF-8 encoded string.
                ImVec2 cell_p1(base_pos.x + (n % 16) * (cell_size + cell_spacing), base_pos.y + (n / 16) * (cell_size + cell_spacing));
                ImVec2 cell_p2(cell_p1.x + cell_size, cell_p1.y + cell_size);
                const ImFontGlyph* glyph = font->FindGlyphNoFallback((ImWchar)(base + n));
                draw_list->AddRect(cell_p1, cell_p2, glyph ? IM_COL32(255, 255, 255, 100) : IM_COL32(255, 255, 255, 50));
                if (!glyph)
                    continue;
                font->RenderChar(draw_list, cell_size, cell_p1, glyph_col, (ImWchar)(base + n));
                if (IsMouseHoveringRect(cell_p1, cell_p2) && BeginTooltip())
                {
                    DebugNodeFontGlyph(font, glyph);
                    EndTooltip();
                }
            }
            Dummy(ImVec2((cell_size + cell_spacing) * 16, (cell_size + cell_spacing) * 16));
            TreePop();
        }
        TreePop();
    }
    TreePop();
}

void ImGui::DebugNodeFontGlyph(ImFont*, const ImFontGlyph* glyph)
{
    Text("Codepoint: U+%04X", glyph->Codepoint);
    Separator();
    Text("Visible: %d", glyph->Visible);
    Text("AdvanceX: %.1f", glyph->AdvanceX);
    Text("Pos: (%.2f,%.2f)->(%.2f,%.2f)", glyph->X0, glyph->Y0, glyph->X1, glyph->Y1);
    Text("UV: (%.3f,%.3f)->(%.3f,%.3f)", glyph->U0, glyph->V0, glyph->U1, glyph->V1);
}

// [DEBUG] Display contents of ImGuiStorage
void ImGui::DebugNodeStorage(ImGuiStorage* storage, const char* label)
{
    if (!TreeNode(label, "%s: %d entries, %d bytes", label, storage->Data.Size, storage->Data.size_in_bytes()))
        return;
    for (const ImGuiStoragePair& p : storage->Data)
    {
        BulletText("Key 0x%08X Value { i: %d }", p.key, p.val_i); // Important: we currently don't store a type, real value may not be integer.
        DebugLocateItemOnHover(p.key);
    }
    TreePop();
}

// [DEBUG] Display contents of ImGuiTabBar
void ImGui::DebugNodeTabBar(ImGuiTabBar* tab_bar, const char* label)
{
    // Standalone tab bars (not associated to docking/windows functionality) currently hold no discernible strings.
    char buf[256];
    char* p = buf;
    const char* buf_end = buf + IM_ARRAYSIZE(buf);
    const bool is_active = (tab_bar->PrevFrameVisible >= GetFrameCount() - 2);
    p += ImFormatString(p, buf_end - p, "%s 0x%08X (%d tabs)%s  {", label, tab_bar->ID, tab_bar->Tabs.Size, is_active ? "" : " *Inactive*");
    for (int tab_n = 0; tab_n < ImMin(tab_bar->Tabs.Size, 3); tab_n++)
    {
        ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
        p += ImFormatString(p, buf_end - p, "%s'%s'", tab_n > 0 ? ", " : "", TabBarGetTabName(tab_bar, tab));
    }
    p += ImFormatString(p, buf_end - p, (tab_bar->Tabs.Size > 3) ? " ... }" : " } ");
    if (!is_active) { PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled)); }
    bool open = TreeNode(label, "%s", buf);
    if (!is_active) { PopStyleColor(); }
    if (is_active && IsItemHovered())
    {
        ImDrawList* draw_list = GetForegroundDrawList();
        draw_list->AddRect(tab_bar->BarRect.Min, tab_bar->BarRect.Max, IM_COL32(255, 255, 0, 255));
        draw_list->AddLine(ImVec2(tab_bar->ScrollingRectMinX, tab_bar->BarRect.Min.y), ImVec2(tab_bar->ScrollingRectMinX, tab_bar->BarRect.Max.y), IM_COL32(0, 255, 0, 255));
        draw_list->AddLine(ImVec2(tab_bar->ScrollingRectMaxX, tab_bar->BarRect.Min.y), ImVec2(tab_bar->ScrollingRectMaxX, tab_bar->BarRect.Max.y), IM_COL32(0, 255, 0, 255));
    }
    if (open)
    {
        for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
        {
            ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
            PushID(tab);
            if (SmallButton("<")) { TabBarQueueReorder(tab_bar, tab, -1); } SameLine(0, 2);
            if (SmallButton(">")) { TabBarQueueReorder(tab_bar, tab, +1); } SameLine();
            Text("%02d%c Tab 0x%08X '%s' Offset: %.2f, Width: %.2f/%.2f",
                tab_n, (tab->ID == tab_bar->SelectedTabId) ? '*' : ' ', tab->ID, TabBarGetTabName(tab_bar, tab), tab->Offset, tab->Width, tab->ContentWidth);
            PopID();
        }
        TreePop();
    }
}

void ImGui::DebugNodeViewport(ImGuiViewportP* viewport)
{
    ImGuiContext& g = *GImGui;
    SetNextItemOpen(true, ImGuiCond_Once);
    bool open = TreeNode("viewport0", "Viewport #%d", 0);
    if (IsItemHovered())
        g.DebugMetricsConfig.HighlightViewportID = viewport->ID;
    if (open)
    {
        ImGuiWindowFlags flags = viewport->Flags;
        BulletText("Main Pos: (%.0f,%.0f), Size: (%.0f,%.0f)\nWorkArea Inset Left: %.0f Top: %.0f, Right: %.0f, Bottom: %.0f",
            viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y,
            viewport->WorkInsetMin.x, viewport->WorkInsetMin.y, viewport->WorkInsetMax.x, viewport->WorkInsetMax.y);
        BulletText("Flags: 0x%04X =%s%s%s", viewport->Flags,
            (flags & ImGuiViewportFlags_IsPlatformWindow)  ? " IsPlatformWindow"  : "",
            (flags & ImGuiViewportFlags_IsPlatformMonitor) ? " IsPlatformMonitor" : "",
            (flags & ImGuiViewportFlags_OwnedByApp)        ? " OwnedByApp"        : "");
        for (ImDrawList* draw_list : viewport->DrawDataP.CmdLists)
            DebugNodeDrawList(NULL, viewport, draw_list, "DrawList");
        TreePop();
    }
}

void ImGui::DebugNodeWindow(ImGuiWindow* window, const char* label)
{
    if (window == NULL)
    {
        BulletText("%s: NULL", label);
        return;
    }

    ImGuiContext& g = *GImGui;
    const bool is_active = window->WasActive;
    ImGuiTreeNodeFlags tree_node_flags = (window == g.NavWindow) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;
    if (!is_active) { PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled)); }
    const bool open = TreeNodeEx(label, tree_node_flags, "%s '%s'%s", label, window->Name, is_active ? "" : " *Inactive*");
    if (!is_active) { PopStyleColor(); }
    if (IsItemHovered() && is_active)
        GetForegroundDrawList(window)->AddRect(window->Pos, window->Pos + window->Size, IM_COL32(255, 255, 0, 255));
    if (!open)
        return;

    if (window->MemoryCompacted)
        TextDisabled("Note: some memory buffers have been compacted/freed.");

    if (g.IO.ConfigDebugIsDebuggerPresent && DebugBreakButton("**DebugBreak**", "in Begin()"))
        g.DebugBreakInWindow = window->ID;

    ImGuiWindowFlags flags = window->Flags;
    DebugNodeDrawList(window, window->Viewport, window->DrawList, "DrawList");
    BulletText("Pos: (%.1f,%.1f), Size: (%.1f,%.1f), ContentSize (%.1f,%.1f) Ideal (%.1f,%.1f)", window->Pos.x, window->Pos.y, window->Size.x, window->Size.y, window->ContentSize.x, window->ContentSize.y, window->ContentSizeIdeal.x, window->ContentSizeIdeal.y);
    BulletText("Flags: 0x%08X (%s%s%s%s%s%s%s%s%s..)", flags,
        (flags & ImGuiWindowFlags_ChildWindow)  ? "Child " : "",      (flags & ImGuiWindowFlags_Tooltip)     ? "Tooltip "   : "",  (flags & ImGuiWindowFlags_Popup) ? "Popup " : "",
        (flags & ImGuiWindowFlags_Modal)        ? "Modal " : "",      (flags & ImGuiWindowFlags_ChildMenu)   ? "ChildMenu " : "",  (flags & ImGuiWindowFlags_NoSavedSettings) ? "NoSavedSettings " : "",
        (flags & ImGuiWindowFlags_NoMouseInputs)? "NoMouseInputs":"", (flags & ImGuiWindowFlags_NoNavInputs) ? "NoNavInputs" : "", (flags & ImGuiWindowFlags_AlwaysAutoResize) ? "AlwaysAutoResize" : "");
    if (flags & ImGuiWindowFlags_ChildWindow)
        BulletText("ChildFlags: 0x%08X (%s%s%s%s..)", window->ChildFlags,
            (window->ChildFlags & ImGuiChildFlags_Borders) ? "Borders " : "",
            (window->ChildFlags & ImGuiChildFlags_ResizeX) ? "ResizeX " : "",
            (window->ChildFlags & ImGuiChildFlags_ResizeY) ? "ResizeY " : "",
            (window->ChildFlags & ImGuiChildFlags_NavFlattened) ? "NavFlattened " : "");
    BulletText("Scroll: (%.2f/%.2f,%.2f/%.2f) Scrollbar:%s%s", window->Scroll.x, window->ScrollMax.x, window->Scroll.y, window->ScrollMax.y, window->ScrollbarX ? "X" : "", window->ScrollbarY ? "Y" : "");
    BulletText("Active: %d/%d, WriteAccessed: %d, BeginOrderWithinContext: %d", window->Active, window->WasActive, window->WriteAccessed, (window->Active || window->WasActive) ? window->BeginOrderWithinContext : -1);
    BulletText("Appearing: %d, Hidden: %d (CanSkip %d Cannot %d), SkipItems: %d", window->Appearing, window->Hidden, window->HiddenFramesCanSkipItems, window->HiddenFramesCannotSkipItems, window->SkipItems);
    for (int layer = 0; layer < ImGuiNavLayer_COUNT; layer++)
    {
        ImRect r = window->NavRectRel[layer];
        if (r.Min.x >= r.Max.y && r.Min.y >= r.Max.y)
            BulletText("NavLastIds[%d]: 0x%08X", layer, window->NavLastIds[layer]);
        else
            BulletText("NavLastIds[%d]: 0x%08X at +(%.1f,%.1f)(%.1f,%.1f)", layer, window->NavLastIds[layer], r.Min.x, r.Min.y, r.Max.x, r.Max.y);
        DebugLocateItemOnHover(window->NavLastIds[layer]);
    }
    const ImVec2* pr = window->NavPreferredScoringPosRel;
    for (int layer = 0; layer < ImGuiNavLayer_COUNT; layer++)
        BulletText("NavPreferredScoringPosRel[%d] = {%.1f,%.1f)", layer, (pr[layer].x == FLT_MAX ? -99999.0f : pr[layer].x), (pr[layer].y == FLT_MAX ? -99999.0f : pr[layer].y)); // Display as 99999.0f so it looks neater.
    BulletText("NavLayersActiveMask: %X, NavLastChildNavWindow: %s", window->DC.NavLayersActiveMask, window->NavLastChildNavWindow ? window->NavLastChildNavWindow->Name : "NULL");
    if (window->RootWindow != window)               { DebugNodeWindow(window->RootWindow, "RootWindow"); }
    if (window->ParentWindow != NULL)               { DebugNodeWindow(window->ParentWindow, "ParentWindow"); }
    if (window->ParentWindowForFocusRoute != NULL)  { DebugNodeWindow(window->ParentWindowForFocusRoute, "ParentWindowForFocusRoute"); }
    if (window->DC.ChildWindows.Size > 0)           { DebugNodeWindowsList(&window->DC.ChildWindows, "ChildWindows"); }
    if (window->ColumnsStorage.Size > 0 && TreeNode("Columns", "Columns sets (%d)", window->ColumnsStorage.Size))
    {
        for (ImGuiOldColumns& columns : window->ColumnsStorage)
            DebugNodeColumns(&columns);
        TreePop();
    }
    DebugNodeStorage(&window->StateStorage, "Storage");
    TreePop();
}

void ImGui::DebugNodeWindowSettings(ImGuiWindowSettings* settings)
{
    if (settings->WantDelete)
        BeginDisabled();
    Text("0x%08X \"%s\" Pos (%d,%d) Size (%d,%d) Collapsed=%d",
        settings->ID, settings->GetName(), settings->Pos.x, settings->Pos.y, settings->Size.x, settings->Size.y, settings->Collapsed);
    if (settings->WantDelete)
        EndDisabled();
}

void ImGui::DebugNodeWindowsList(ImVector<ImGuiWindow*>* windows, const char* label)
{
    if (!TreeNode(label, "%s (%d)", label, windows->Size))
        return;
    for (int i = windows->Size - 1; i >= 0; i--) // Iterate front to back
    {
        PushID((*windows)[i]);
        DebugNodeWindow((*windows)[i], "Window");
        PopID();
    }
    TreePop();
}

// FIXME-OPT: This is technically suboptimal, but it is simpler this way.
void ImGui::DebugNodeWindowsListByBeginStackParent(ImGuiWindow** windows, int windows_size, ImGuiWindow* parent_in_begin_stack)
{
    for (int i = 0; i < windows_size; i++)
    {
        ImGuiWindow* window = windows[i];
        if (window->ParentWindowInBeginStack != parent_in_begin_stack)
            continue;
        char buf[20];
        ImFormatString(buf, IM_ARRAYSIZE(buf), "[%04d] Window", window->BeginOrderWithinContext);
        //BulletText("[%04d] Window '%s'", window->BeginOrderWithinContext, window->Name);
        DebugNodeWindow(window, buf);
        Indent();
        DebugNodeWindowsListByBeginStackParent(windows + i + 1, windows_size - i - 1, window);
        Unindent();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DEBUG LOG WINDOW
//-----------------------------------------------------------------------------

void ImGui::DebugLog(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DebugLogV(fmt, args);
    va_end(args);
}

void ImGui::DebugLogV(const char* fmt, va_list args)
{
    ImGuiContext& g = *GImGui;
    const int old_size = g.DebugLogBuf.size();
    if (g.ContextName[0] != 0)
        g.DebugLogBuf.appendf("[%s] [%05d] ", g.ContextName, g.FrameCount);
    else
        g.DebugLogBuf.appendf("[%05d] ", g.FrameCount);
    g.DebugLogBuf.appendfv(fmt, args);
    g.DebugLogIndex.append(g.DebugLogBuf.c_str(), old_size, g.DebugLogBuf.size());
    if (g.DebugLogFlags & ImGuiDebugLogFlags_OutputToTTY)
        IMGUI_DEBUG_PRINTF("%s", g.DebugLogBuf.begin() + old_size);
#ifdef IMGUI_ENABLE_TEST_ENGINE
    // IMGUI_TEST_ENGINE_LOG() adds a trailing \n automatically
    const int new_size = g.DebugLogBuf.size();
    const bool trailing_carriage_return = (g.DebugLogBuf[new_size - 1] == '\n');
    if (g.DebugLogFlags & ImGuiDebugLogFlags_OutputToTestEngine)
        IMGUI_TEST_ENGINE_LOG("%.*s", new_size - old_size - (trailing_carriage_return ? 1 : 0), g.DebugLogBuf.begin() + old_size);
#endif
}

// FIXME-LAYOUT: To be done automatically via layout mode once we rework ItemSize/ItemAdd into ItemLayout.
static void SameLineOrWrap(const ImVec2& size)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImVec2 pos(window->DC.CursorPosPrevLine.x + g.Style.ItemSpacing.x, window->DC.CursorPosPrevLine.y);
    if (window->WorkRect.Contains(ImRect(pos, pos + size)))
        ImGui::SameLine();
}

static void ShowDebugLogFlag(const char* name, ImGuiDebugLogFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImVec2 size(ImGui::GetFrameHeight() + g.Style.ItemInnerSpacing.x + ImGui::CalcTextSize(name).x, ImGui::GetFrameHeight());
    SameLineOrWrap(size); // FIXME-LAYOUT: To be done automatically once we rework ItemSize/ItemAdd into ItemLayout.

    bool highlight_errors = (flags == ImGuiDebugLogFlags_EventError && g.DebugLogSkippedErrors > 0);
    if (highlight_errors)
        ImGui::PushStyleColor(ImGuiCol_Text, ImLerp(g.Style.Colors[ImGuiCol_Text], ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 0.30f));
    if (ImGui::CheckboxFlags(name, &g.DebugLogFlags, flags) && g.IO.KeyShift && (g.DebugLogFlags & flags) != 0)
    {
        g.DebugLogAutoDisableFrames = 2;
        g.DebugLogAutoDisableFlags |= flags;
    }
    if (highlight_errors)
    {
        ImGui::PopStyleColor();
        ImGui::SetItemTooltip("%d past errors skipped.", g.DebugLogSkippedErrors);
    }
    else
    {
        ImGui::SetItemTooltip("Hold SHIFT when clicking to enable for 2 frames only (useful for spammy log entries)");
    }
}

void ImGui::ShowDebugLogWindow(bool* p_open)
{
    ImGuiContext& g = *GImGui;
    if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize) == 0)
        SetNextWindowSize(ImVec2(0.0f, GetFontSize() * 12.0f), ImGuiCond_FirstUseEver);
    if (!Begin("Dear ImGui Debug Log", p_open) || GetCurrentWindow()->BeginCount > 1)
    {
        End();
        return;
    }

    ImGuiDebugLogFlags all_enable_flags = ImGuiDebugLogFlags_EventMask_ & ~ImGuiDebugLogFlags_EventInputRouting;
    CheckboxFlags("All", &g.DebugLogFlags, all_enable_flags);
    SetItemTooltip("(except InputRouting which is spammy)");

    ShowDebugLogFlag("Errors", ImGuiDebugLogFlags_EventError);
    ShowDebugLogFlag("ActiveId", ImGuiDebugLogFlags_EventActiveId);
    ShowDebugLogFlag("Clipper", ImGuiDebugLogFlags_EventClipper);
    ShowDebugLogFlag("Focus", ImGuiDebugLogFlags_EventFocus);
    ShowDebugLogFlag("IO", ImGuiDebugLogFlags_EventIO);
    ShowDebugLogFlag("Nav", ImGuiDebugLogFlags_EventNav);
    ShowDebugLogFlag("Popup", ImGuiDebugLogFlags_EventPopup);
    ShowDebugLogFlag("Selection", ImGuiDebugLogFlags_EventSelection);
    ShowDebugLogFlag("InputRouting", ImGuiDebugLogFlags_EventInputRouting);

    if (SmallButton("Clear"))
    {
        g.DebugLogBuf.clear();
        g.DebugLogIndex.clear();
        g.DebugLogSkippedErrors = 0;
    }
    SameLine();
    if (SmallButton("Copy"))
        SetClipboardText(g.DebugLogBuf.c_str());
    SameLine();
    if (SmallButton("Configure Outputs.."))
        OpenPopup("Outputs");
    if (BeginPopup("Outputs"))
    {
        CheckboxFlags("OutputToTTY", &g.DebugLogFlags, ImGuiDebugLogFlags_OutputToTTY);
#ifndef IMGUI_ENABLE_TEST_ENGINE
        BeginDisabled();
#endif
        CheckboxFlags("OutputToTestEngine", &g.DebugLogFlags, ImGuiDebugLogFlags_OutputToTestEngine);
#ifndef IMGUI_ENABLE_TEST_ENGINE
        EndDisabled();
#endif
        EndPopup();
    }

    BeginChild("##log", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);

    const ImGuiDebugLogFlags backup_log_flags = g.DebugLogFlags;
    g.DebugLogFlags &= ~ImGuiDebugLogFlags_EventClipper;

    ImGuiListClipper clipper;
    clipper.Begin(g.DebugLogIndex.size());
    while (clipper.Step())
        for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
            DebugTextUnformattedWithLocateItem(g.DebugLogIndex.get_line_begin(g.DebugLogBuf.c_str(), line_no), g.DebugLogIndex.get_line_end(g.DebugLogBuf.c_str(), line_no));
    g.DebugLogFlags = backup_log_flags;
    if (GetScrollY() >= GetScrollMaxY())
        SetScrollHereY(1.0f);
    EndChild();

    End();
}

// Display line, search for 0xXXXXXXXX identifiers and call DebugLocateItemOnHover() when hovered.
void ImGui::DebugTextUnformattedWithLocateItem(const char* line_begin, const char* line_end)
{
    TextUnformatted(line_begin, line_end);
    if (!IsItemHovered())
        return;
    ImGuiContext& g = *GImGui;
    ImRect text_rect = g.LastItemData.Rect;
    for (const char* p = line_begin; p <= line_end - 10; p++)
    {
        ImGuiID id = 0;
        if (p[0] != '0' || (p[1] != 'x' && p[1] != 'X') || sscanf(p + 2, "%X", &id) != 1 || ImCharIsXdigitA(p[10]))
            continue;
        ImVec2 p0 = CalcTextSize(line_begin, p);
        ImVec2 p1 = CalcTextSize(p, p + 10);
        g.LastItemData.Rect = ImRect(text_rect.Min + ImVec2(p0.x, 0.0f), text_rect.Min + ImVec2(p0.x + p1.x, p1.y));
        if (IsMouseHoveringRect(g.LastItemData.Rect.Min, g.LastItemData.Rect.Max, true))
            DebugLocateItemOnHover(id);
        p += 10;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] OTHER DEBUG TOOLS (ITEM PICKER, ID STACK TOOL)
//-----------------------------------------------------------------------------

// Draw a small cross at current CursorPos in current window's DrawList
void ImGui::DebugDrawCursorPos(ImU32 col)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImVec2 pos = window->DC.CursorPos;
    window->DrawList->AddLine(ImVec2(pos.x, pos.y - 3.0f), ImVec2(pos.x, pos.y + 4.0f), col, 1.0f);
    window->DrawList->AddLine(ImVec2(pos.x - 3.0f, pos.y), ImVec2(pos.x + 4.0f, pos.y), col, 1.0f);
}

// Draw a 10px wide rectangle around CurposPos.x using Line Y1/Y2 in current window's DrawList
void ImGui::DebugDrawLineExtents(ImU32 col)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    float curr_x = window->DC.CursorPos.x;
    float line_y1 = (window->DC.IsSameLine ? window->DC.CursorPosPrevLine.y : window->DC.CursorPos.y);
    float line_y2 = line_y1 + (window->DC.IsSameLine ? window->DC.PrevLineSize.y : window->DC.CurrLineSize.y);
    window->DrawList->AddLine(ImVec2(curr_x - 5.0f, line_y1), ImVec2(curr_x + 5.0f, line_y1), col, 1.0f);
    window->DrawList->AddLine(ImVec2(curr_x - 0.5f, line_y1), ImVec2(curr_x - 0.5f, line_y2), col, 1.0f);
    window->DrawList->AddLine(ImVec2(curr_x - 5.0f, line_y2), ImVec2(curr_x + 5.0f, line_y2), col, 1.0f);
}

// Draw last item rect in ForegroundDrawList (so it is always visible)
void ImGui::DebugDrawItemRect(ImU32 col)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    GetForegroundDrawList(window)->AddRect(g.LastItemData.Rect.Min, g.LastItemData.Rect.Max, col);
}

// [DEBUG] Locate item position/rectangle given an ID.
static const ImU32 DEBUG_LOCATE_ITEM_COLOR = IM_COL32(0, 255, 0, 255);  // Green

void ImGui::DebugLocateItem(ImGuiID target_id)
{
    ImGuiContext& g = *GImGui;
    g.DebugLocateId = target_id;
    g.DebugLocateFrames = 2;
    g.DebugBreakInLocateId = false;
}

// FIXME: Doesn't work over through a modal window, because they clear HoveredWindow.
void ImGui::DebugLocateItemOnHover(ImGuiID target_id)
{
    if (target_id == 0 || !IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenBlockedByPopup))
        return;
    ImGuiContext& g = *GImGui;
    DebugLocateItem(target_id);
    GetForegroundDrawList(g.CurrentWindow)->AddRect(g.LastItemData.Rect.Min - ImVec2(3.0f, 3.0f), g.LastItemData.Rect.Max + ImVec2(3.0f, 3.0f), DEBUG_LOCATE_ITEM_COLOR);

    // Can't easily use a context menu here because it will mess with focus, active id etc.
    if (g.IO.ConfigDebugIsDebuggerPresent && g.MouseStationaryTimer > 1.0f)
    {
        DebugBreakButtonTooltip(false, "in ItemAdd()");
        if (IsKeyChordPressed(g.DebugBreakKeyChord))
            g.DebugBreakInLocateId = true;
    }
}

void ImGui::DebugLocateItemResolveWithLastItem()
{
    ImGuiContext& g = *GImGui;

    // [DEBUG] Debug break requested by user
    if (g.DebugBreakInLocateId)
        IM_DEBUG_BREAK();

    ImGuiLastItemData item_data = g.LastItemData;
    g.DebugLocateId = 0;
    ImDrawList* draw_list = GetForegroundDrawList(g.CurrentWindow);
    ImRect r = item_data.Rect;
    r.Expand(3.0f);
    ImVec2 p1 = g.IO.MousePos;
    ImVec2 p2 = ImVec2((p1.x < r.Min.x) ? r.Min.x : (p1.x > r.Max.x) ? r.Max.x : p1.x, (p1.y < r.Min.y) ? r.Min.y : (p1.y > r.Max.y) ? r.Max.y : p1.y);
    draw_list->AddRect(r.Min, r.Max, DEBUG_LOCATE_ITEM_COLOR);
    draw_list->AddLine(p1, p2, DEBUG_LOCATE_ITEM_COLOR);
}

void ImGui::DebugStartItemPicker()
{
    ImGuiContext& g = *GImGui;
    g.DebugItemPickerActive = true;
}

// [DEBUG] Item picker tool - start with DebugStartItemPicker() - useful to visually select an item and break into its call-stack.
void ImGui::UpdateDebugToolItemPicker()
{
    ImGuiContext& g = *GImGui;
    g.DebugItemPickerBreakId = 0;
    if (!g.DebugItemPickerActive)
        return;

    const ImGuiID hovered_id = g.HoveredIdPreviousFrame;
    SetMouseCursor(ImGuiMouseCursor_Hand);
    if (IsKeyPressed(ImGuiKey_Escape))
        g.DebugItemPickerActive = false;
    const bool change_mapping = g.IO.KeyMods == (ImGuiMod_Ctrl | ImGuiMod_Shift);
    if (!change_mapping && IsMouseClicked(g.DebugItemPickerMouseButton) && hovered_id)
    {
        g.DebugItemPickerBreakId = hovered_id;
        g.DebugItemPickerActive = false;
    }
    for (int mouse_button = 0; mouse_button < 3; mouse_button++)
        if (change_mapping && IsMouseClicked(mouse_button))
            g.DebugItemPickerMouseButton = (ImU8)mouse_button;
    SetNextWindowBgAlpha(0.70f);
    if (!BeginTooltip())
        return;
    Text("HoveredId: 0x%08X", hovered_id);
    Text("Press ESC to abort picking.");
    const char* mouse_button_names[] = { "Left", "Right", "Middle" };
    if (change_mapping)
        Text("Remap w/ Ctrl+Shift: click anywhere to select new mouse button.");
    else
        TextColored(GetStyleColorVec4(hovered_id ? ImGuiCol_Text : ImGuiCol_TextDisabled), "Click %s Button to break in debugger! (remap w/ Ctrl+Shift)", mouse_button_names[g.DebugItemPickerMouseButton]);
    EndTooltip();
}

// [DEBUG] ID Stack Tool: update queries. Called by NewFrame()
void ImGui::UpdateDebugToolStackQueries()
{
    ImGuiContext& g = *GImGui;
    ImGuiIDStackTool* tool = &g.DebugIDStackTool;

    // Clear hook when id stack tool is not visible
    g.DebugHookIdInfo = 0;
    if (g.FrameCount != tool->LastActiveFrame + 1)
        return;

    // Update queries. The steps are: -1: query Stack, >= 0: query each stack item
    // We can only perform 1 ID Info query every frame. This is designed so the GetID() tests are cheap and constant-time
    const ImGuiID query_id = g.HoveredIdPreviousFrame ? g.HoveredIdPreviousFrame : g.ActiveId;
    if (tool->QueryId != query_id)
    {
        tool->QueryId = query_id;
        tool->StackLevel = -1;
        tool->Results.resize(0);
    }
    if (query_id == 0)
        return;

    // Advance to next stack level when we got our result, or after 2 frames (in case we never get a result)
    int stack_level = tool->StackLevel;
    if (stack_level >= 0 && stack_level < tool->Results.Size)
        if (tool->Results[stack_level].QuerySuccess || tool->Results[stack_level].QueryFrameCount > 2)
            tool->StackLevel++;

    // Update hook
    stack_level = tool->StackLevel;
    if (stack_level == -1)
        g.DebugHookIdInfo = query_id;
    if (stack_level >= 0 && stack_level < tool->Results.Size)
    {
        g.DebugHookIdInfo = tool->Results[stack_level].ID;
        tool->Results[stack_level].QueryFrameCount++;
    }
}

// [DEBUG] ID Stack tool: hooks called by GetID() family functions
void ImGui::DebugHookIdInfo(ImGuiID id, ImGuiDataType data_type, const void* data_id, const void* data_id_end)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiIDStackTool* tool = &g.DebugIDStackTool;

    // Step 0: stack query
    // This assumes that the ID was computed with the current ID stack, which tends to be the case for our widget.
    if (tool->StackLevel == -1)
    {
        tool->StackLevel++;
        tool->Results.resize(window->IDStack.Size + 1, ImGuiStackLevelInfo());
        for (int n = 0; n < window->IDStack.Size + 1; n++)
            tool->Results[n].ID = (n < window->IDStack.Size) ? window->IDStack[n] : id;
        return;
    }

    // Step 1+: query for individual level
    IM_ASSERT(tool->StackLevel >= 0);
    if (tool->StackLevel != window->IDStack.Size)
        return;
    ImGuiStackLevelInfo* info = &tool->Results[tool->StackLevel];
    IM_ASSERT(info->ID == id && info->QueryFrameCount > 0);

    switch (data_type)
    {
    case ImGuiDataType_S32:
        ImFormatString(info->Desc, IM_ARRAYSIZE(info->Desc), "%d", (int)(intptr_t)data_id);
        break;
    case ImGuiDataType_String:
        ImFormatString(info->Desc, IM_ARRAYSIZE(info->Desc), "%.*s", data_id_end ? (int)((const char*)data_id_end - (const char*)data_id) : (int)strlen((const char*)data_id), (const char*)data_id);
        break;
    case ImGuiDataType_Pointer:
        ImFormatString(info->Desc, IM_ARRAYSIZE(info->Desc), "(void*)0x%p", data_id);
        break;
    case ImGuiDataType_ID:
        if (info->Desc[0] != 0) // PushOverrideID() is often used to avoid hashing twice, which would lead to 2 calls to DebugHookIdInfo(). We prioritize the first one.
            return;
        ImFormatString(info->Desc, IM_ARRAYSIZE(info->Desc), "0x%08X [override]", id);
        break;
    default:
        IM_ASSERT(0);
    }
    info->QuerySuccess = true;
    info->DataType = data_type;
}

static int StackToolFormatLevelInfo(ImGuiIDStackTool* tool, int n, bool format_for_ui, char* buf, size_t buf_size)
{
    ImGuiStackLevelInfo* info = &tool->Results[n];
    ImGuiWindow* window = (info->Desc[0] == 0 && n == 0) ? ImGui::FindWindowByID(info->ID) : NULL;
    if (window)                                                                 // Source: window name (because the root ID don't call GetID() and so doesn't get hooked)
        return ImFormatString(buf, buf_size, format_for_ui ? "\"%s\" [window]" : "%s", window->Name);
    if (info->QuerySuccess)                                                     // Source: GetID() hooks (prioritize over ItemInfo() because we frequently use patterns like: PushID(str), Button("") where they both have same id)
        return ImFormatString(buf, buf_size, (format_for_ui && info->DataType == ImGuiDataType_String) ? "\"%s\"" : "%s", info->Desc);
    if (tool->StackLevel < tool->Results.Size)                                  // Only start using fallback below when all queries are done, so during queries we don't flickering ??? markers.
        return (*buf = 0);
#ifdef IMGUI_ENABLE_TEST_ENGINE
    if (const char* label = ImGuiTestEngine_FindItemDebugLabel(GImGui, info->ID))   // Source: ImGuiTestEngine's ItemInfo()
        return ImFormatString(buf, buf_size, format_for_ui ? "??? \"%s\"" : "%s", label);
#endif
    return ImFormatString(buf, buf_size, "???");
}

// ID Stack Tool: Display UI
void ImGui::ShowIDStackToolWindow(bool* p_open)
{
    ImGuiContext& g = *GImGui;
    if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize) == 0)
        SetNextWindowSize(ImVec2(0.0f, GetFontSize() * 8.0f), ImGuiCond_FirstUseEver);
    if (!Begin("Dear ImGui ID Stack Tool", p_open) || GetCurrentWindow()->BeginCount > 1)
    {
        End();
        return;
    }

    // Display hovered/active status
    ImGuiIDStackTool* tool = &g.DebugIDStackTool;
    const ImGuiID hovered_id = g.HoveredIdPreviousFrame;
    const ImGuiID active_id = g.ActiveId;
#ifdef IMGUI_ENABLE_TEST_ENGINE
    Text("HoveredId: 0x%08X (\"%s\"), ActiveId:  0x%08X (\"%s\")", hovered_id, hovered_id ? ImGuiTestEngine_FindItemDebugLabel(&g, hovered_id) : "", active_id, active_id ? ImGuiTestEngine_FindItemDebugLabel(&g, active_id) : "");
#else
    Text("HoveredId: 0x%08X, ActiveId:  0x%08X", hovered_id, active_id);
#endif
    SameLine();
    MetricsHelpMarker("Hover an item with the mouse to display elements of the ID Stack leading to the item's final ID.\nEach level of the stack correspond to a PushID() call.\nAll levels of the stack are hashed together to make the final ID of a widget (ID displayed at the bottom level of the stack).\nRead FAQ entry about the ID stack for details.");

    // CTRL+C to copy path
    const float time_since_copy = (float)g.Time - tool->CopyToClipboardLastTime;
    Checkbox("Ctrl+C: copy path to clipboard", &tool->CopyToClipboardOnCtrlC);
    SameLine();
    TextColored((time_since_copy >= 0.0f && time_since_copy < 0.75f && ImFmod(time_since_copy, 0.25f) < 0.25f * 0.5f) ? ImVec4(1.f, 1.f, 0.3f, 1.f) : ImVec4(), "*COPIED*");
    if (tool->CopyToClipboardOnCtrlC && Shortcut(ImGuiMod_Ctrl | ImGuiKey_C, ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused))
    {
        tool->CopyToClipboardLastTime = (float)g.Time;
        char* p = g.TempBuffer.Data;
        char* p_end = p + g.TempBuffer.Size;
        for (int stack_n = 0; stack_n < tool->Results.Size && p + 3 < p_end; stack_n++)
        {
            *p++ = '/';
            char level_desc[256];
            StackToolFormatLevelInfo(tool, stack_n, false, level_desc, IM_ARRAYSIZE(level_desc));
            for (int n = 0; level_desc[n] && p + 2 < p_end; n++)
            {
                if (level_desc[n] == '/')
                    *p++ = '\\';
                *p++ = level_desc[n];
            }
        }
        *p = '\0';
        SetClipboardText(g.TempBuffer.Data);
    }

    // Display decorated stack
    tool->LastActiveFrame = g.FrameCount;
    if (tool->Results.Size > 0 && BeginTable("##table", 3, ImGuiTableFlags_Borders))
    {
        const float id_width = CalcTextSize("0xDDDDDDDD").x;
        TableSetupColumn("Seed", ImGuiTableColumnFlags_WidthFixed, id_width);
        TableSetupColumn("PushID", ImGuiTableColumnFlags_WidthStretch);
        TableSetupColumn("Result", ImGuiTableColumnFlags_WidthFixed, id_width);
        TableHeadersRow();
        for (int n = 0; n < tool->Results.Size; n++)
        {
            ImGuiStackLevelInfo* info = &tool->Results[n];
            TableNextColumn();
            Text("0x%08X", (n > 0) ? tool->Results[n - 1].ID : 0);
            TableNextColumn();
            StackToolFormatLevelInfo(tool, n, true, g.TempBuffer.Data, g.TempBuffer.Size);
            TextUnformatted(g.TempBuffer.Data);
            TableNextColumn();
            Text("0x%08X", info->ID);
            if (n == tool->Results.Size - 1)
                TableSetBgColor(ImGuiTableBgTarget_CellBg, GetColorU32(ImGuiCol_Header));
        }
        EndTable();
    }
    End();
}

#else

void ImGui::ShowMetricsWindow(bool*) {}
void ImGui::ShowFontAtlas(ImFontAtlas*) {}
void ImGui::DebugNodeColumns(ImGuiOldColumns*) {}
void ImGui::DebugNodeDrawList(ImGuiWindow*, ImGuiViewportP*, const ImDrawList*, const char*) {}
void ImGui::DebugNodeDrawCmdShowMeshAndBoundingBox(ImDrawList*, const ImDrawList*, const ImDrawCmd*, bool, bool) {}
void ImGui::DebugNodeFont(ImFont*) {}
void ImGui::DebugNodeStorage(ImGuiStorage*, const char*) {}
void ImGui::DebugNodeTabBar(ImGuiTabBar*, const char*) {}
void ImGui::DebugNodeWindow(ImGuiWindow*, const char*) {}
void ImGui::DebugNodeWindowSettings(ImGuiWindowSettings*) {}
void ImGui::DebugNodeWindowsList(ImVector<ImGuiWindow*>*, const char*) {}
void ImGui::DebugNodeViewport(ImGuiViewportP*) {}

void ImGui::ShowDebugLogWindow(bool*) {}
void ImGui::ShowIDStackToolWindow(bool*) {}
void ImGui::DebugStartItemPicker() {}
void ImGui::DebugHookIdInfo(ImGuiID, ImGuiDataType, const void*, const void*) {}

#endif // #ifndef IMGUI_DISABLE_DEBUG_TOOLS

//-----------------------------------------------------------------------------

// Include imgui_user.inl at the end of imgui.cpp to access private data/functions that aren't exposed.
// Prefer just including imgui_internal.h from your code rather than using this define. If a declaration is missing from imgui_internal.h add it or request it on the github.
#ifdef IMGUI_INCLUDE_IMGUI_USER_INL
#include "imgui_user.inl"
#endif

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
