// dear imgui, v1.68 WIP
// (main code and documentation)

// Call and read ImGui::ShowDemoWindow() in imgui_demo.cpp for demo code.
// Newcomers, read 'Programmer guide' below for notes on how to setup Dear ImGui in your codebase.
// Get latest version at https://github.com/ocornut/imgui
// Releases change-log at https://github.com/ocornut/imgui/releases
// Technical Support for Getting Started https://discourse.dearimgui.org/c/getting-started
// Gallery (please post your screenshots/video there!): https://github.com/ocornut/imgui/issues/1269

// Developed by Omar Cornut and every direct or indirect contributors to the GitHub.
// See LICENSE.txt for copyright and licensing details (standard MIT License).
// This library is free but I need your support to sustain development and maintenance.
// Businesses: you can support continued maintenance and development via support contracts or sponsoring, see docs/README.
// Individuals: you can support continued maintenance and development via donations or Patreon https://www.patreon.com/imgui.

// It is recommended that you don't modify imgui.cpp! It will become difficult for you to update the library.
// Note that 'ImGui::' being a namespace, you can add functions into the namespace from your own source files, without
// modifying imgui.h or imgui.cpp. You may include imgui_internal.h to access internal data structures, but it doesn't
// come with any guarantee of forward compatibility. Discussing your changes on the GitHub Issue Tracker may lead you
// to a better solution or official support for them.

/*

Index of this file:

DOCUMENTATION

- MISSION STATEMENT
- END-USER GUIDE
- PROGRAMMER GUIDE (read me!)
  - Read first.
  - How to update to a newer version of Dear ImGui.
  - Getting started with integrating Dear ImGui in your code/engine.
  - This is how a simple application may look like (2 variations).
  - This is how a simple rendering function may look like.
  - Using gamepad/keyboard navigation controls.
- API BREAKING CHANGES (read me when you update!)
- FREQUENTLY ASKED QUESTIONS (FAQ), TIPS
  - How can I tell whether to dispatch mouse/keyboard to imgui or to my application?
  - How can I display an image? What is ImTextureID, how does it works?
  - How can I have multiple widgets with the same label or with an empty label? A primer on labels and the ID Stack.
  - How can I use my own math types instead of ImVec2/ImVec4? 
  - How can I load a different font than the default?
  - How can I easily use icons in my application?
  - How can I load multiple fonts?
  - How can I display and input non-latin characters such as Chinese, Japanese, Korean, Cyrillic?
  - How can I interact with standard C++ types (such as std::string and std::vector)? 
  - How can I use the drawing facilities without an ImGui window? (using ImDrawList API)
  - How can I use Dear ImGui on a platform that doesn't have a mouse or a keyboard? (input share, remoting, gamepad)
  - I integrated Dear ImGui in my engine and the text or lines are blurry..
  - I integrated Dear ImGui in my engine and some elements are clipping or disappearing when I move windows around..
  - How can I help?

CODE 
(search for "[SECTION]" in the code to find them)

// [SECTION] FORWARD DECLARATIONS
// [SECTION] CONTEXT AND MEMORY ALLOCATORS
// [SECTION] MAIN USER FACING STRUCTURES (ImGuiStyle, ImGuiIO)
// [SECTION] MISC HELPER/UTILITIES (Maths, String, Format, Hash, File functions)
// [SECTION] MISC HELPER/UTILITIES (ImText* functions)
// [SECTION] MISC HELPER/UTILITIES (Color functions)
// [SECTION] ImGuiStorage
// [SECTION] ImGuiTextFilter
// [SECTION] ImGuiTextBuffer
// [SECTION] ImGuiListClipper
// [SECTION] RENDER HELPERS
// [SECTION] MAIN CODE (most of the code! lots of stuff, needs tidying up!)
// [SECTION] TOOLTIPS
// [SECTION] POPUPS
// [SECTION] VIEWPORTS, PLATFORM WINDOWS
// [SECTION] KEYBOARD/GAMEPAD NAVIGATION
// [SECTION] COLUMNS
// [SECTION] DRAG AND DROP
// [SECTION] DOCKING
// [SECTION] LOGGING/CAPTURING
// [SECTION] SETTINGS
// [SECTION] PLATFORM DEPENDENT HELPERS
// [SECTION] METRICS/DEBUG WINDOW

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
 - Minimize screen real-estate usage.
 - Minimize setup and maintenance.
 - Minimize state storage on user side.
 - Portable, minimize dependencies, run on target (consoles, phones, etc.).
 - Efficient runtime and memory consumption (NB- we do allocate when "growing" content e.g. creating a window,.
   opening a tree node for the first time, etc. but a typical frame should not allocate anything).

 Designed for developers and content-creators, not the typical end-user! Some of the weaknesses includes:
 - Doesn't look fancy, doesn't animate.
 - Limited layout features, intricate layouts are typically crafted in code.


 END-USER GUIDE
 ==============

 - Double-click on title bar to collapse window.
 - Click upper right corner to close a window, available when 'bool* p_open' is passed to ImGui::Begin().
 - Click and drag on lower right corner to resize window (double-click to auto fit window to its contents).
 - Click and drag on any empty space to move window.
 - TAB/SHIFT+TAB to cycle through keyboard editable fields.
 - CTRL+Click on a slider or drag box to input value as text.
 - Use mouse wheel to scroll.
 - Text editor:
   - Hold SHIFT or use mouse to select text.
   - CTRL+Left/Right to word jump.
   - CTRL+Shift+Left/Right to select words.
   - CTRL+A our Double-Click to select all.
   - CTRL+X,CTRL+C,CTRL+V to use OS clipboard/
   - CTRL+Z,CTRL+Y to undo/redo.
   - ESCAPE to revert text to its original value.
   - You can apply arithmetic operators +,*,/ on numerical values. Use +- to subtract (because - would set a negative value!)
   - Controls are automatically adjusted for OSX to match standard OSX text editing operations.
 - General Keyboard controls: enable with ImGuiConfigFlags_NavEnableKeyboard.
 - General Gamepad controls: enable with ImGuiConfigFlags_NavEnableGamepad. See suggested mappings in imgui.h ImGuiNavInput_ + download PNG/PSD at http://goo.gl/9LgVZW


 PROGRAMMER GUIDE
 ================

 READ FIRST:

 - Read the FAQ below this section!
 - Your code creates the UI, if your code doesn't run the UI is gone! The UI can be highly dynamic, there are no construction
   or destruction steps, less superfluous data retention on your side, less state duplication, less state synchronization, less bugs.
 - Call and read ImGui::ShowDemoWindow() for demo code demonstrating most features.
 - The library is designed to be built from sources. Avoid pre-compiled binaries and packaged versions. See imconfig.h to configure your build.
 - Dear ImGui is an implementation of the IMGUI paradigm (immediate-mode graphical user interface, a term coined by Casey Muratori).
   You can learn about IMGUI principles at http://www.johno.se/book/imgui.html, http://mollyrocket.com/861 & more links docs/README.md.
 - Dear ImGui is a "single pass" rasterizing implementation of the IMGUI paradigm, aimed at ease of use and high-performances.
   For every application frame your UI code will be called only once. This is in contrast to e.g. Unity's own implementation of an IMGUI,
   where the UI code is called multiple times ("multiple passes") from a single entry point. There are pros and cons to both approaches.
 - Our origin are on the top-left. In axis aligned bounding boxes, Min = top-left, Max = bottom-right.
 - This codebase is also optimized to yield decent performances with typical "Debug" builds settings.
 - Please make sure you have asserts enabled (IM_ASSERT redirects to assert() by default, but can be redirected).
   If you get an assert, read the messages and comments around the assert.
 - C++: this is a very C-ish codebase: we don't rely on C++11, we don't include any C++ headers, and ImGui:: is a namespace.
 - C++: ImVec2/ImVec4 do not expose math operators by default, because it is expected that you use your own math types.
   See FAQ "How can I use my own math types instead of ImVec2/ImVec4?" for details about setting up imconfig.h for that.
   However, imgui_internal.h can optionally export math operators for ImVec2/ImVec4, which we use in this codebase.
 - C++: pay attention that ImVector<> manipulates plain-old-data and does not honor construction/destruction (avoid using it in your code!).

 HOW TO UPDATE TO A NEWER VERSION OF DEAR IMGUI:

 - Overwrite all the sources files except for imconfig.h (if you have made modification to your copy of imconfig.h)
 - Or maintain your own branch where you have imconfig.h modified.
 - Read the "API BREAKING CHANGES" section (below). This is where we list occasional API breaking changes.
   If a function/type has been renamed / or marked obsolete, try to fix the name in your code before it is permanently removed
   from the public API. If you have a problem with a missing function/symbols, search for its name in the code, there will
   likely be a comment about it. Please report any issue to the GitHub page!
 - Try to keep your copy of dear imgui reasonably up to date.

 GETTING STARTED WITH INTEGRATING DEAR IMGUI IN YOUR CODE/ENGINE:

 - Run and study the examples and demo in imgui_demo.cpp to get acquainted with the library.
 - Add the Dear ImGui source files to your projects or using your preferred build system.
   It is recommended you build and statically link the .cpp files as part of your project and not as shared library (DLL).
 - You can later customize the imconfig.h file to tweak some compile-time behavior, such as integrating imgui types with your own maths types.
 - When using Dear ImGui, your programming IDE is your friend: follow the declaration of variables, functions and types to find comments about them.
 - Dear ImGui never touches or knows about your GPU state. The only function that knows about GPU is the draw function that you provide.
   Effectively it means you can create widgets at any time in your code, regardless of considerations of being in "update" vs "render"
   phases of your own application. All rendering informatioe are stored into command-lists that you will retrieve after calling ImGui::Render().
 - Refer to the bindings and demo applications in the examples/ folder for instruction on how to setup your code.
 - If you are running over a standard OS with a common graphics API, you should be able to use unmodified imgui_impl_*** files from the examples/ folder.

 HOW A SIMPLE APPLICATION MAY LOOK LIKE:
 EXHIBIT 1: USING THE EXAMPLE BINDINGS (imgui_impl_XXX.cpp files from the examples/ folder).

     // Application init: create a dear imgui context, setup some options, load fonts
     ImGui::CreateContext();
     ImGuiIO& io = ImGui::GetIO();
     // TODO: Set optional io.ConfigFlags values, e.g. 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard' to enable keyboard controls.
     // TODO: Fill optional fields of the io structure later.
     // TODO: Load TTF/OTF fonts if you don't want to use the default font.

     // Initialize helper Platform and Renderer bindings (here we are using imgui_impl_win32 and imgui_impl_dx11)
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

 HOW A SIMPLE APPLICATION MAY LOOK LIKE:
 EXHIBIT 2: IMPLEMENTING CUSTOM BINDING / CUSTOM ENGINE.

     // Application init: create a dear imgui context, setup some options, load fonts
     ImGui::CreateContext();
     ImGuiIO& io = ImGui::GetIO();
     // TODO: Set optional io.ConfigFlags values, e.g. 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard' to enable keyboard controls.
     // TODO: Fill optional fields of the io structure later.
     // TODO: Load TTF/OTF fonts if you don't want to use the default font.

     // Build and load the texture atlas into a texture
     // (In the examples/ app this is usually done within the ImGui_ImplXXX_Init() function from one of the demo Renderer)
     int width, height;
     unsigned char* pixels = NULL;
     io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

     // At this point you've got the texture data and you need to upload that your your graphic system:
     // After we have created the texture, store its pointer/identifier (_in whichever format your engine uses_) in 'io.Fonts->TexID'.
     // This will be passed back to your via the renderer. Basically ImTextureID == void*. Read FAQ below for details about ImTextureID.
     MyTexture* texture = MyEngine::CreateTextureFromMemoryPixels(pixels, width, height, TEXTURE_TYPE_RGBA32)
     io.Fonts->TexID = (void*)texture;

     // Application main loop
     while (true)
     {
        // Setup low-level inputs, e.g. on Win32: calling GetKeyboardState(), or write to those fields from your Windows message handlers, etc.
        // (In the examples/ app this is usually done within the ImGui_ImplXXX_NewFrame() function from one of the demo Platform bindings)
        io.DeltaTime = 1.0f/60.0f;              // set the time elapsed since the previous frame (in seconds)
        io.DisplaySize.x = 1920.0f;             // set the current display width
        io.DisplaySize.y = 1280.0f;             // set the current display height here
        io.MousePos = my_mouse_pos;             // set the mouse position
        io.MouseDown[0] = my_mouse_buttons[0];  // set the mouse button states
        io.MouseDown[1] = my_mouse_buttons[1];

        // Call NewFrame(), after this point you can use ImGui::* functions anytime
        // (So you want to try calling NewFrame() as early as you can in your mainloop to be able to use imgui everywhere)
        ImGui::NewFrame();

        // Most of your application code here
        ImGui::Text("Hello, world!");
        MyGameUpdate(); // may use any ImGui functions, e.g. ImGui::Begin("My window"); ImGui::Text("Hello, world!"); ImGui::End();
        MyGameRender(); // may use any ImGui functions as well!

        // Render imgui, swap buffers
        // (You want to try calling EndFrame/Render as late as you can, to be able to use imgui in your own game rendering code)
        ImGui::EndFrame();
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        MyImGuiRenderFunction(draw_data);
        SwapBuffers();
     }

     // Shutdown
     ImGui::DestroyContext();

 HOW A SIMPLE RENDERING FUNCTION MAY LOOK LIKE:

    void void MyImGuiRenderFunction(ImDrawData* draw_data)
    {
       // TODO: Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
       // TODO: Setup viewport covering draw_data->DisplayPos to draw_data->DisplayPos + draw_data->DisplaySize
       // TODO: Setup orthographic projection matrix cover draw_data->DisplayPos to draw_data->DisplayPos + draw_data->DisplaySize
       // TODO: Setup shader: vertex { float2 pos, float2 uv, u32 color }, fragment shader sample color from 1 texture, multiply by vertex color.
       for (int n = 0; n < draw_data->CmdListsCount; n++)
       {
          const ImDrawList* cmd_list = draw_data->CmdLists[n]; 
          const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;  // vertex buffer generated by ImGui
          const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;   // index buffer generated by ImGui
          for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
          {
             const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
             if (pcmd->UserCallback)
             {
                 pcmd->UserCallback(cmd_list, pcmd);
             }
             else
             {
                 // The texture for the draw call is specified by pcmd->TextureId.
                 // The vast majority of draw calls will use the imgui texture atlas, which value you have set yourself during initialization.
                 MyEngineBindTexture((MyTexture*)pcmd->TextureId);

                 // We are using scissoring to clip some objects. All low-level graphics API should supports it.
                 // - If your engine doesn't support scissoring yet, you may ignore this at first. You will get some small glitches
                 //   (some elements visible outside their bounds) but you can fix that once everything else works!
                 // - Clipping coordinates are provided in imgui coordinates space (from draw_data->DisplayPos to draw_data->DisplayPos + draw_data->DisplaySize)
                 //   In a single viewport application, draw_data->DisplayPos will always be (0,0) and draw_data->DisplaySize will always be == io.DisplaySize.
                 //   However, in the interest of supporting multi-viewport applications in the future (see 'viewport' branch on github), 
                 //   always subtract draw_data->DisplayPos from clipping bounds to convert them to your viewport space.
                 // - Note that pcmd->ClipRect contains Min+Max bounds. Some graphics API may use Min+Max, other may use Min+Size (size being Max-Min)
                 ImVec2 pos = draw_data->DisplayPos;
                 MyEngineScissor((int)(pcmd->ClipRect.x - pos.x), (int)(pcmd->ClipRect.y - pos.y), (int)(pcmd->ClipRect.z - pos.x), (int)(pcmd->ClipRect.w - pos.y));

                 // Render 'pcmd->ElemCount/3' indexed triangles.
                 // By default the indices ImDrawIdx are 16-bits, you can change them to 32-bits in imconfig.h if your engine doesn't support 16-bits indices.
                 MyEngineDrawIndexedTriangles(pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer, vtx_buffer);
             }
             idx_buffer += pcmd->ElemCount;
          }
       }
    }

 - The examples/ folders contains many actual implementation of the pseudo-codes above.
 - When calling NewFrame(), the 'io.WantCaptureMouse', 'io.WantCaptureKeyboard' and 'io.WantTextInput' flags are updated.
   They tell you if Dear ImGui intends to use your inputs. When a flag is set you want to hide the corresponding inputs 
   from the rest of your application. In every cases you need to pass on the inputs to imgui. Refer to the FAQ for more information.
 - Please read the FAQ below!. Amusingly, it is called a FAQ because people frequently run into the same issues!

 USING GAMEPAD/KEYBOARD NAVIGATION CONTROLS

 - The gamepad/keyboard navigation is fairly functional and keeps being improved. 
 - Gamepad support is particularly useful to use dear imgui on a console system (e.g. PS4, Switch, XB1) without a mouse!
 - You can ask questions and report issues at https://github.com/ocornut/imgui/issues/787
 - The initial focus was to support game controllers, but keyboard is becoming increasingly and decently usable.
 - Gamepad:
    - Set io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad to enable.
    - Backend: Set io.BackendFlags |= ImGuiBackendFlags_HasGamepad + fill the io.NavInputs[] fields before calling NewFrame().
      Note that io.NavInputs[] is cleared by EndFrame().
    - See 'enum ImGuiNavInput_' in imgui.h for a description of inputs. For each entry of io.NavInputs[], set the following values:
         0.0f= not held. 1.0f= fully held. Pass intermediate 0.0f..1.0f values for analog triggers/sticks.
    - We uses a simple >0.0f test for activation testing, and won't attempt to test for a dead-zone.
      Your code will probably need to transform your raw inputs (such as e.g. remapping your 0.2..0.9 raw input range to 0.0..1.0 imgui range, etc.).
    - You can download PNG/PSD files depicting the gamepad controls for common controllers at: http://goo.gl/9LgVZW.
    - If you need to share inputs between your game and the imgui parts, the easiest approach is to go all-or-nothing, with a buttons combo
      to toggle the target. Please reach out if you think the game vs navigation input sharing could be improved.
 - Keyboard:
    - Set io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard to enable.
      NewFrame() will automatically fill io.NavInputs[] based on your io.KeysDown[] + io.KeyMap[] arrays.
    - When keyboard navigation is active (io.NavActive + ImGuiConfigFlags_NavEnableKeyboard), the io.WantCaptureKeyboard flag
      will be set. For more advanced uses, you may want to read from:
       - io.NavActive: true when a window is focused and it doesn't have the ImGuiWindowFlags_NoNavInputs flag set.
       - io.NavVisible: true when the navigation cursor is visible (and usually goes false when mouse is used).
       - or query focus information with e.g. IsWindowFocused(ImGuiFocusedFlags_AnyWindow), IsItemFocused() etc. functions.
      Please reach out if you think the game vs navigation input sharing could be improved.
 - Mouse:
    - PS4 users: Consider emulating a mouse cursor with DualShock4 touch pad or a spare analog stick as a mouse-emulation fallback.
    - Consoles/Tablet/Phone users: Consider using a Synergy 1.x server (on your PC) + uSynergy.c (on your console/tablet/phone app) to share your PC mouse/keyboard.
    - On a TV/console system where readability may be lower or mouse inputs may be awkward, you may want to set the ImGuiConfigFlags_NavEnableSetMousePos flag.
      Enabling ImGuiConfigFlags_NavEnableSetMousePos + ImGuiBackendFlags_HasSetMousePos instructs dear imgui to move your mouse cursor along with navigation movements.
      When enabled, the NewFrame() function may alter 'io.MousePos' and set 'io.WantSetMousePos' to notify you that it wants the mouse cursor to be moved.
      When that happens your back-end NEEDS to move the OS or underlying mouse cursor on the next frame. Some of the binding in examples/ do that.
      (If you set the NavEnableSetMousePos flag but don't honor 'io.WantSetMousePos' properly, imgui will misbehave as it will see your mouse as moving back and forth!)
      (In a setup when you may not have easy control over the mouse cursor, e.g. uSynergy.c doesn't expose moving remote mouse cursor, you may want
       to set a boolean to ignore your other external mouse positions until the external source is moved again.)


 API BREAKING CHANGES
 ====================

 Occasionally introducing changes that are breaking the API. We try to make the breakage minor and easy to fix.
 Below is a change-log of API breaking changes only. If you are using one of the functions listed, expect to have to fix some code.
 When you are not sure about a old symbol or function name, try using the Search/Find function of your IDE to look for comments or references in all imgui files.
 You can read releases logs https://github.com/ocornut/imgui/releases for more details.

 (Viewport Branch)
 - 2018/XX/XX (1.XX) - when multi-viewports are enabled, all positions will be in your natural OS coordinates space. It means that:
                        - reference to hard-coded positions such as in SetNextWindowPos(ImVec2(0,0)) are probably not what you want anymore. 
                          you may use GetMainViewport()->Pos to offset hard-coded positions, e.g. SetNextWindowPos(GetMainViewport()->Pos)
                        - likewise io.MousePos and GetMousePos() will use OS coordinates. 
                          If you query mouse positions to interact with non-imgui coordinates you will need to offset them, e.g. subtract GetWindowViewport()->Pos.
 - 2018/XX/XX (1.XX) - Moved IME support functions from io.ImeSetInputScreenPosFn, io.ImeWindowHandle to the PlatformIO api.
 - 2018/XX/XX (1.XX) - removed io.DisplayVisibleMin, io.DisplayVisibleMax settings (they were used to clip within the (0,0)..DisplaySize range, I don't know of anyone using it)


 - 2019/01/06 (1.67) - renamed io.InputCharacters[], marked internal as was always intended. Please don't access directly, and use AddInputCharacter() instead!
 - 2019/01/06 (1.67) - renamed ImFontAtlas::GlyphRangesBuilder to ImFontGlyphRangesBuilder. Keep redirection typedef (will obsolete).
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
 - 2018/06/08 (1.62) - examples: the imgui_impl_xxx files have been split to separate platform (Win32, Glfw, SDL2, etc.) from renderer (DX11, OpenGL, Vulkan,  etc.).
                       old binding will still work as is, however prefer using the separated bindings as they will be updated to be multi-viewport conformant.
                       when adopting new bindings follow the main.cpp code of your preferred examples/ folder to know which functions to call.
 - 2018/06/06 (1.62) - renamed GetGlyphRangesChinese() to GetGlyphRangesChineseFull() to distinguish other variants and discourage using the full set.
 - 2018/06/06 (1.62) - TreeNodeEx()/TreeNodeBehavior(): the ImGuiTreeNodeFlags_CollapsingHeader helper now include the ImGuiTreeNodeFlags_NoTreePushOnOpen flag. See Changelog for details. 
 - 2018/05/03 (1.61) - DragInt(): the default compile-time format string has been changed from "%.0f" to "%d", as we are not using integers internally any more.
                       If you used DragInt() with custom format strings, make sure you change them to use %d or an integer-compatible format.
                       To honor backward-compatibility, the DragInt() code will currently parse and modify format strings to replace %*f with %d, giving time to users to upgrade their code.
                       If you have IMGUI_DISABLE_OBSOLETE_FUNCTIONS enabled, the code will instead assert! You may run a reg-exp search on your codebase for e.g. "DragInt.*%f" to help you find them.
 - 2018/04/28 (1.61) - obsoleted InputFloat() functions taking an optional "int decimal_precision" in favor of an equivalent and more flexible "const char* format",
                       consistent with other functions. Kept redirection functions (will obsolete).
 - 2018/04/09 (1.61) - IM_DELETE() helper function added in 1.60 doesn't clear the input _pointer_ reference, more consistent with expectation and allows passing r-value.
 - 2018/03/20 (1.60) - renamed io.WantMoveMouse to io.WantSetMousePos for consistency and ease of understanding (was added in 1.52, _not_ used by core and only honored by some binding ahead of merging the Nav branch).
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
 - 2017/10/17 (1.52) - marked the old 5-parameters version of Begin() as obsolete (still available). Use SetNextWindowSize()+Begin() instead!
 - 2017/10/11 (1.52) - renamed AlignFirstTextHeightToWidgets() to AlignTextToFramePadding(). Kept inline redirection function (will obsolete).
 - 2017/09/26 (1.52) - renamed ImFont::Glyph to ImFontGlyph. Keep redirection typedef (will obsolete).
 - 2017/09/25 (1.52) - removed SetNextWindowPosCenter() because SetNextWindowPos() now has the optional pivot information to do the same and more. Kept redirection function (will obsolete).
 - 2017/08/25 (1.52) - io.MousePos needs to be set to ImVec2(-FLT_MAX,-FLT_MAX) when mouse is unavailable/missing. Previously ImVec2(-1,-1) was enough but we now accept negative mouse coordinates. In your binding if you need to support unavailable mouse, make sure to replace "io.MousePos = ImVec2(-1,-1)" with "io.MousePos = ImVec2(-FLT_MAX,-FLT_MAX)".
 - 2017/08/22 (1.51) - renamed IsItemHoveredRect() to IsItemRectHovered(). Kept inline redirection function (will obsolete). -> (1.52) use IsItemHovered(ImGuiHoveredFlags_RectOnly)!
                     - renamed IsMouseHoveringAnyWindow() to IsAnyWindowHovered() for consistency. Kept inline redirection function (will obsolete).
                     - renamed IsMouseHoveringWindow() to IsWindowRectHovered() for consistency. Kept inline redirection function (will obsolete).
 - 2017/08/20 (1.51) - renamed GetStyleColName() to GetStyleColorName() for consistency.
 - 2017/08/20 (1.51) - added PushStyleColor(ImGuiCol idx, ImU32 col) overload, which _might_ cause an "ambiguous call" compilation error if you are using ImColor() with implicit cast. Cast to ImU32 or ImVec4 explicily to fix.
 - 2017/08/15 (1.51) - marked the weird IMGUI_ONCE_UPON_A_FRAME helper macro as obsolete. prefer using the more explicit ImGuiOnceUponAFrame.
 - 2017/08/15 (1.51) - changed parameter order for BeginPopupContextWindow() from (const char*,int buttons,bool also_over_items) to (const char*,int buttons,bool also_over_items). Note that most calls relied on default parameters completely.
 - 2017/08/13 (1.51) - renamed ImGuiCol_Columns*** to ImGuiCol_Separator***. Kept redirection enums (will obsolete).
 - 2017/08/11 (1.51) - renamed ImGuiSetCond_*** types and flags to ImGuiCond_***. Kept redirection enums (will obsolete).
 - 2017/08/09 (1.51) - removed ValueColor() helpers, they are equivalent to calling Text(label) + SameLine() + ColorButton().
 - 2017/08/08 (1.51) - removed ColorEditMode() and ImGuiColorEditMode in favor of ImGuiColorEditFlags and parameters to the various Color*() functions. The SetColorEditOptions() allows to initialize default but the user can still change them with right-click context menu.
                     - changed prototype of 'ColorEdit4(const char* label, float col[4], bool show_alpha = true)' to 'ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags = 0)', where passing flags = 0x01 is a safe no-op (hello dodgy backward compatibility!). - check and run the demo window, under "Color/Picker Widgets", to understand the various new options.
                     - changed prototype of rarely used 'ColorButton(ImVec4 col, bool small_height = false, bool outline_border = true)' to 'ColorButton(const char* desc_id, ImVec4 col, ImGuiColorEditFlags flags = 0, ImVec2 size = ImVec2(0,0))'
 - 2017/07/20 (1.51) - removed IsPosHoveringAnyWindow(ImVec2), which was partly broken and misleading. ASSERT + redirect user to io.WantCaptureMouse
 - 2017/05/26 (1.50) - removed ImFontConfig::MergeGlyphCenterV in favor of a more multipurpose ImFontConfig::GlyphOffset.
 - 2017/05/01 (1.50) - renamed ImDrawList::PathFill() (rarely used directly) to ImDrawList::PathFillConvex() for clarity.
 - 2016/11/06 (1.50) - BeginChild(const char*) now applies the stack id to the provided label, consistently with other functions as it should always have been. It shouldn't affect you unless (extremely unlikely) you were appending multiple times to a same child from different locations of the stack id. If that's the case, generate an id with GetId() and use it instead of passing string to BeginChild().
 - 2016/10/15 (1.50) - avoid 'void* user_data' parameter to io.SetClipboardTextFn/io.GetClipboardTextFn pointers. We pass io.ClipboardUserData to it.
 - 2016/09/25 (1.50) - style.WindowTitleAlign is now a ImVec2 (ImGuiAlign enum was removed). set to (0.5f,0.5f) for horizontal+vertical centering, (0.0f,0.0f) for upper-left, etc.
 - 2016/07/30 (1.50) - SameLine(x) with x>0.0f is now relative to left of column/group if any, and not always to left of window. This was sort of always the intent and hopefully breakage should be minimal.
 - 2016/05/12 (1.49) - title bar (using ImGuiCol_TitleBg/ImGuiCol_TitleBgActive colors) isn't rendered over a window background (ImGuiCol_WindowBg color) anymore.
                       If your TitleBg/TitleBgActive alpha was 1.0f or you are using the default theme it will not affect you.
                       If your TitleBg/TitleBgActive alpha was <1.0f you need to tweak your custom theme to readjust for the fact that we don't draw a WindowBg background behind the title bar.
                       This helper function will convert an old TitleBg/TitleBgActive color into a new one with the same visual output, given the OLD color and the OLD WindowBg color.
                           ImVec4 ConvertTitleBgCol(const ImVec4& win_bg_col, const ImVec4& title_bg_col)
                           {
                               float new_a = 1.0f - ((1.0f - win_bg_col.w) * (1.0f - title_bg_col.w)), k = title_bg_col.w / new_a;
                               return ImVec4((win_bg_col.x * win_bg_col.w + title_bg_col.x) * k, (win_bg_col.y * win_bg_col.w + title_bg_col.y) * k, (win_bg_col.z * win_bg_col.w + title_bg_col.z) * k, new_a);
                           }
                       If this is confusing, pick the RGB value from title bar from an old screenshot and apply this as TitleBg/TitleBgActive. Or you may just create TitleBgActive from a tweaked TitleBg color.
 - 2016/05/07 (1.49) - removed confusing set of GetInternalState(), GetInternalStateSize(), SetInternalState() functions. Now using CreateContext(), DestroyContext(), GetCurrentContext(), SetCurrentContext().
 - 2016/05/02 (1.49) - renamed SetNextTreeNodeOpened() to SetNextTreeNodeOpen(), no redirection.
 - 2016/05/01 (1.49) - obsoleted old signature of CollapsingHeader(const char* label, const char* str_id = NULL, bool display_frame = true, bool default_open = false) as extra parameters were badly designed and rarely used. You can replace the "default_open = true" flag in new API with CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen).
 - 2016/04/26 (1.49) - changed ImDrawList::PushClipRect(ImVec4 rect) to ImDrawList::PushClipRect(Imvec2 min,ImVec2 max,bool intersect_with_current_clip_rect=false). Note that higher-level ImGui::PushClipRect() is preferable because it will clip at logic/widget level, whereas ImDrawList::PushClipRect() only affect your renderer.
 - 2016/04/03 (1.48) - removed style.WindowFillAlphaDefault setting which was redundant. Bake default BG alpha inside style.Colors[ImGuiCol_WindowBg] and all other Bg color values. (ref github issue #337).
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
                     - if you are using a vanilla copy of one of the imgui_impl_XXXX.cpp provided in the example, you just need to update your copy and you can ignore the rest.
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
 - 2015/06/14 (1.41) - changed Selectable() API from (label, selected, size) to (label, selected, flags, size). Size override should have been rarely be used. Sorry!
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
              (1.30) - removed GetDefaultFontData(). uses io.Fonts->GetTextureData*() API to retrieve uncompressed pixels.
                       font init:  { const void* png_data; unsigned int png_size; ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size); <..Upload texture to GPU..>; }
                       became:     { unsigned char* pixels; int width, height; io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height); <..Upload texture to GPU>; io.Fonts->TexId = YourTextureIdentifier; }
                       you now have more flexibility to load multiple TTF fonts and manage the texture buffer for internal needs.
                       it is now recommended that you sample the font texture with bilinear interpolation.
              (1.30) - added texture identifier in ImDrawCmd passed to your render function (we can now render images). make sure to set io.Fonts->TexID.
              (1.30) - removed IO.PixelCenterOffset (unnecessary, can be handled in user projection matrix)
              (1.30) - removed ImGui::IsItemFocused() in favor of ImGui::IsItemActive() which handles all widgets
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


 FREQUENTLY ASKED QUESTIONS (FAQ), TIPS
 ======================================

 Q: How can I tell whether to dispatch mouse/keyboard to imgui or to my application?
 A: You can read the 'io.WantCaptureMouse', 'io.WantCaptureKeyboard' and 'io.WantTextInput' flags from the ImGuiIO structure (e.g. if (ImGui::GetIO().WantCaptureMouse) { ... } )
    - When 'io.WantCaptureMouse' is set, imgui wants to use your mouse state, and you may want to discard/hide the inputs from the rest of your application.
    - When 'io.WantCaptureKeyboard' is set, imgui wants to use your keyboard state, and you may want to discard/hide the inputs from the rest of your application.
    - When 'io.WantTextInput' is set to may want to notify your OS to popup an on-screen keyboard, if available (e.g. on a mobile phone, or console OS).
    Note: you should always pass your mouse/keyboard inputs to imgui, even when the io.WantCaptureXXX flag are set false.
     This is because imgui needs to detect that you clicked in the void to unfocus its own windows.
    Note: The 'io.WantCaptureMouse' is more accurate that any attempt to "check if the mouse is hovering a window" (don't do that!).
     It handle mouse dragging correctly (both dragging that started over your application or over an imgui window) and handle e.g. modal windows blocking inputs.
     Those flags are updated by ImGui::NewFrame(). Preferably read the flags after calling NewFrame() if you can afford it, but reading them before is also
     perfectly fine, as the bool toggle fairly rarely. If you have on a touch device, you might find use for an early call to UpdateHoveredWindowAndCaptureFlags().
    Note: Text input widget releases focus on "Return KeyDown", so the subsequent "Return KeyUp" event that your application receive will typically
     have 'io.WantCaptureKeyboard=false'. Depending on your application logic it may or not be inconvenient. You might want to track which key-downs
     were targeted for Dear ImGui, e.g. with an array of bool, and filter out the corresponding key-ups.)

 Q: How can I display an image? What is ImTextureID, how does it works?
 A: Short explanation:
    - You may use functions such as ImGui::Image(), ImGui::ImageButton() or lower-level ImDrawList::AddImage() to emit draw calls that will use your own textures.
    - Actual textures are identified in a way that is up to the user/engine. Those identifiers are stored and passed as ImTextureID (void*) value.
    - Loading image files from the disk and turning them into a texture is not within the scope of Dear ImGui (for a good reason). 
      Please read documentations or tutorials on your graphics API to understand how to display textures on the screen before moving onward.

    Long explanation:
    - Dear ImGui's job is to create "meshes", defined in a renderer-agnostic format made of draw commands and vertices.
      At the end of the frame those meshes (ImDrawList) will be displayed by your rendering function. They are made up of textured polygons and the code
      to render them is generally fairly short (a few dozen lines). In the examples/ folder we provide functions for popular graphics API (OpenGL, DirectX, etc.).
    - Each rendering function decides on a data type to represent "textures". The concept of what is a "texture" is entirely tied to your underlying engine/graphics API.
      We carry the information to identify a "texture" in the ImTextureID type. 
      ImTextureID is nothing more that a void*, aka 4/8 bytes worth of data: just enough to store 1 pointer or 1 integer of your choice.
      Dear ImGui doesn't know or understand what you are storing in ImTextureID, it merely pass ImTextureID values until they reach your rendering function.
    - In the examples/ bindings, for each graphics API binding we decided on a type that is likely to be a good representation for specifying 
      an image from the end-user perspective. This is what the _examples_ rendering functions are using:

         OpenGL:     ImTextureID = GLuint                       (see ImGui_ImplGlfwGL3_RenderDrawData() function in imgui_impl_glfw_gl3.cpp)
         DirectX9:   ImTextureID = LPDIRECT3DTEXTURE9           (see ImGui_ImplDX9_RenderDrawData()     function in imgui_impl_dx9.cpp)
         DirectX11:  ImTextureID = ID3D11ShaderResourceView*    (see ImGui_ImplDX11_RenderDrawData()    function in imgui_impl_dx11.cpp)
         DirectX12:  ImTextureID = D3D12_GPU_DESCRIPTOR_HANDLE  (see ImGui_ImplDX12_RenderDrawData()    function in imgui_impl_dx12.cpp)

      For example, in the OpenGL example binding we store raw OpenGL texture identifier (GLuint) inside ImTextureID. 
      Whereas in the DirectX11 example binding we store a pointer to ID3D11ShaderResourceView inside ImTextureID, which is a higher-level structure 
      tying together both the texture and information about its format and how to read it.
    - If you have a custom engine built over e.g. OpenGL, instead of passing GLuint around you may decide to use a high-level data type to carry information about
      the texture as well as how to display it (shaders, etc.). The decision of what to use as ImTextureID can always be made better knowing how your codebase
      is designed. If your engine has high-level data types for "textures" and "material" then you may want to use them.
      If you are starting with OpenGL or DirectX or Vulkan and haven't built much of a rendering engine over them, keeping the default ImTextureID 
      representation suggested by the example bindings is probably the best choice.
      (Advanced users may also decide to keep a low-level type in ImTextureID, and use ImDrawList callback and pass information to their renderer)

    User code may do:

        // Cast our texture type to ImTextureID / void*
        MyTexture* texture = g_CoffeeTableTexture;
        ImGui::Image((void*)texture, ImVec2(texture->Width, texture->Height)); 

    The renderer function called after ImGui::Render() will receive that same value that the user code passed:

        // Cast ImTextureID / void* stored in the draw command as our texture type
        MyTexture* texture = (MyTexture*)pcmd->TextureId;
        MyEngineBindTexture2D(texture);

    Once you understand this design you will understand that loading image files and turning them into displayable textures is not within the scope of Dear ImGui.
    This is by design and is actually a good thing, because it means your code has full control over your data types and how you display them.
    If you want to display an image file (e.g. PNG file) into the screen, please refer to documentation and tutorials for the graphics API you are using.

    Here's a simplified OpenGL example using stb_image.h:

        // Use stb_image.h to load a PNG from disk and turn it into raw RGBA pixel data:
        #define STB_IMAGE_IMPLEMENTATION
        #include <stb_image.h>
        [...]
        int my_image_width, my_image_height;
        unsigned char* my_image_data = stbi_load("my_image.png", &my_image_width, &my_image_height, NULL, 4);

        // Turn the RGBA pixel data into an OpenGL texture:
        GLuint my_opengl_texture;
        glGenTextures(1, &my_opengl_texture);
        glBindTexture(GL_TEXTURE_2D, my_opengl_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

        // Now that we have an OpenGL texture, assuming our imgui rendering function (imgui_impl_xxx.cpp file) takes GLuint as ImTextureID, we can display it:
        ImGui::Image((void*)(intptr_t)my_opengl_texture, ImVec2(my_image_width, my_image_height));

    C/C++ tip: a void* is pointer-sized storage. You may safely store any pointer or integer into it by casting your value to ImTexture / void*, and vice-versa. 
    Because both end-points (user code and rendering function) are under your control, you know exactly what is stored inside the ImTexture / void*.
    Examples:

        GLuint my_tex = XXX;
        void* my_void_ptr;
        my_void_ptr = (void*)(intptr_t)my_tex;                  // cast a GLuint into a void* (we don't take its address! we literally store the value inside the pointer)
        my_tex = (GLuint)(intptr_t)my_void_ptr;                 // cast a void* into a GLuint

        ID3D11ShaderResourceView* my_dx11_srv = XXX;
        void* my_void_ptr;
        my_void_ptr = (void*)my_dx11_srv;                       // cast a ID3D11ShaderResourceView* into an opaque void*
        my_dx11_srv = (ID3D11ShaderResourceView*)my_void_ptr;   // cast a void* into a ID3D11ShaderResourceView*

    Finally, you may call ImGui::ShowMetricsWindow() to explore/visualize/understand how the ImDrawList are generated.

 Q: How can I have multiple widgets with the same label or with an empty label?
 Q: I have multiple widgets with the same label, and only the first one works. Why is that?
 A: A primer on labels and the ID Stack...

    Dear ImGui internally need to uniquely identify UI elements.
    Elements that are typically not clickable (such as calls to the Text functions) don't need an ID.
    Interactive widgets (such as calls to Button buttons) need a unique ID. 
    Unique ID are used internally to track active widgets and occasionally associate state to widgets.
    Unique ID are implicitly built from the hash of multiple elements that identify the "path" to the UI element.

   - Unique ID are often derived from a string label:

       Button("OK");          // Label = "OK",     ID = hash of (..., "OK")
       Button("Cancel");      // Label = "Cancel", ID = hash of (..., "Cancel")

   - ID are uniquely scoped within windows, tree nodes, etc. which all pushes to the ID stack. Having
     two buttons labeled "OK" in different windows or different tree locations is fine.
     We used "..." above to signify whatever was already pushed to the ID stack previously:

       Begin("MyWindow");
       Button("OK");          // Label = "OK",     ID = hash of ("MyWindow", "OK")
       End();
       Begin("MyOtherWindow");
       Button("OK");          // Label = "OK",     ID = hash of ("MyOtherWindow", "OK")
       End();

   - If you have a same ID twice in the same location, you'll have a conflict:

       Button("OK");
       Button("OK");          // ID collision! Interacting with either button will trigger the first one.

     Fear not! this is easy to solve and there are many ways to solve it!

   - Solving ID conflict in a simple/local context:
     When passing a label you can optionally specify extra ID information within string itself.
     Use "##" to pass a complement to the ID that won't be visible to the end-user.
     This helps solving the simple collision cases when you know e.g. at compilation time which items
     are going to be created:

       Begin("MyWindow");
       Button("Play");        // Label = "Play",   ID = hash of ("MyWindow", "Play")
       Button("Play##foo1");  // Label = "Play",   ID = hash of ("MyWindow", "Play##foo1")  // Different from above
       Button("Play##foo2");  // Label = "Play",   ID = hash of ("MyWindow", "Play##foo2")  // Different from above
       End();

   - If you want to completely hide the label, but still need an ID:

       Checkbox("##On", &b);  // Label = "",       ID = hash of (..., "##On")   // No visible label, just a checkbox!

   - Occasionally/rarely you might want change a label while preserving a constant ID. This allows
     you to animate labels. For example you may want to include varying information in a window title bar,
     but windows are uniquely identified by their ID. Use "###" to pass a label that isn't part of ID:

       Button("Hello###ID");  // Label = "Hello",  ID = hash of (..., "ID")
       Button("World###ID");  // Label = "World",  ID = hash of (..., "ID")     // Same as above, even though the label looks different

       sprintf(buf, "My game (%f FPS)###MyGame", fps);
       Begin(buf);            // Variable title,   ID = hash of "MyGame"

   - Solving ID conflict in a more general manner:
     Use PushID() / PopID() to create scopes and manipulate the ID stack, as to avoid ID conflicts
     within the same window. This is the most convenient way of distinguishing ID when iterating and
     creating many UI elements programmatically.
     You can push a pointer, a string or an integer value into the ID stack.
     Remember that ID are formed from the concatenation of _everything_ pushed into the ID stack.
     At each level of the stack we store the seed used for items at this level of the ID stack.

     Begin("Window");
       for (int i = 0; i < 100; i++)
       {
         PushID(i);           // Push i to the id tack
         Button("Click");     // Label = "Click",  ID = hash of ("Window", i, "Click")
         PopID();
       }
       for (int i = 0; i < 100; i++)
       {
         MyObject* obj = Objects[i];
         PushID(obj);
         Button("Click");     // Label = "Click",  ID = hash of ("Window", obj pointer, "Click")
         PopID();
       }
       for (int i = 0; i < 100; i++)
       {
         MyObject* obj = Objects[i];
         PushID(obj->Name);
         Button("Click");     // Label = "Click",  ID = hash of ("Window", obj->Name, "Click")
         PopID();
       }
       End();

   - You can stack multiple prefixes into the ID stack:

       Button("Click");       // Label = "Click",  ID = hash of (..., "Click")
       PushID("node");
       Button("Click");       // Label = "Click",  ID = hash of (..., "node", "Click")
         PushID(my_ptr);
           Button("Click");   // Label = "Click",  ID = hash of (..., "node", my_ptr, "Click")
         PopID();
       PopID();

   - Tree nodes implicitly creates a scope for you by calling PushID().

       Button("Click");       // Label = "Click",  ID = hash of (..., "Click")
       if (TreeNode("node"))  // <-- this function call will do a PushID() for you (unless instructed not to, with a special flag)
       {
         Button("Click");     // Label = "Click",  ID = hash of (..., "node", "Click")
         TreePop();
       }

   - When working with trees, ID are used to preserve the open/close state of each tree node.
     Depending on your use cases you may want to use strings, indices or pointers as ID.
      e.g. when following a single pointer that may change over time, using a static string as ID
       will preserve your node open/closed state when the targeted object change.
      e.g. when displaying a list of objects, using indices or pointers as ID will preserve the
       node open/closed state differently. See what makes more sense in your situation!

 Q: How can I use my own math types instead of ImVec2/ImVec4? 
 A: You can edit imconfig.h and setup the IM_VEC2_CLASS_EXTRA/IM_VEC4_CLASS_EXTRA macros to add implicit type conversions.
    This way you'll be able to use your own types everywhere, e.g. passsing glm::vec2 to ImGui functions instead of ImVec2.

 Q: How can I load a different font than the default?
 A: Use the font atlas to load the TTF/OTF file you want:
      ImGuiIO& io = ImGui::GetIO();
      io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels);
      io.Fonts->GetTexDataAsRGBA32() or GetTexDataAsAlpha8()
    Default is ProggyClean.ttf, monospace, rendered at size 13, embedded in dear imgui's source code.
    (Tip: monospace fonts are convenient because they allow to facilitate horizontal alignment directly at the string level.) 
    (Read the 'misc/fonts/README.txt' file for more details about font loading.)

    New programmers: remember that in C/C++ and most programming languages if you want to use a
    backslash \ within a string literal, you need to write it double backslash "\\":
      io.Fonts->AddFontFromFileTTF("MyDataFolder\MyFontFile.ttf", size_in_pixels);   // WRONG (you are escape the M here!)
      io.Fonts->AddFontFromFileTTF("MyDataFolder\\MyFontFile.ttf", size_in_pixels);  // CORRECT
      io.Fonts->AddFontFromFileTTF("MyDataFolder/MyFontFile.ttf", size_in_pixels);   // ALSO CORRECT

 Q: How can I easily use icons in my application?
 A: The most convenient and practical way is to merge an icon font such as FontAwesome inside you
    main font. Then you can refer to icons within your strings. 
    You may want to see ImFontConfig::GlyphMinAdvanceX to make your icon look monospace to facilitate alignment.
    (Read the 'misc/fonts/README.txt' file for more details about icons font loading.)

 Q: How can I load multiple fonts?
 A: Use the font atlas to pack them into a single texture:
    (Read the 'misc/fonts/README.txt' file and the code in ImFontAtlas for more details.)

      ImGuiIO& io = ImGui::GetIO();
      ImFont* font0 = io.Fonts->AddFontDefault();
      ImFont* font1 = io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels);
      ImFont* font2 = io.Fonts->AddFontFromFileTTF("myfontfile2.ttf", size_in_pixels);
      io.Fonts->GetTexDataAsRGBA32() or GetTexDataAsAlpha8()
      // the first loaded font gets used by default
      // use ImGui::PushFont()/ImGui::PopFont() to change the font at runtime

      // Options
      ImFontConfig config;
      config.OversampleH = 3;
      config.OversampleV = 1;
      config.GlyphOffset.y -= 2.0f;      // Move everything by 2 pixels up
      config.GlyphExtraSpacing.x = 1.0f; // Increase spacing between characters
      io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_pixels, &config);

      // Combine multiple fonts into one (e.g. for icon fonts)
      static ImWchar ranges[] = { 0xf000, 0xf3ff, 0 };
      ImFontConfig config;
      config.MergeMode = true;
      io.Fonts->AddFontDefault();
      io.Fonts->AddFontFromFileTTF("fontawesome-webfont.ttf", 16.0f, &config, ranges); // Merge icon font
      io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_pixels, NULL, &config, io.Fonts->GetGlyphRangesJapanese()); // Merge japanese glyphs

 Q: How can I display and input non-Latin characters such as Chinese, Japanese, Korean, Cyrillic?
 A: When loading a font, pass custom Unicode ranges to specify the glyphs to load.

      // Add default Japanese ranges
      io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels, NULL, io.Fonts->GetGlyphRangesJapanese());

      // Or create your own custom ranges (e.g. for a game you can feed your entire game script and only build the characters the game need)
      ImVector<ImWchar> ranges;
      ImFontGlyphRangesBuilder builder;
      builder.AddText("Hello world");                        // Add a string (here "Hello world" contains 7 unique characters)
      builder.AddChar(0x7262);                               // Add a specific character
      builder.AddRanges(io.Fonts->GetGlyphRangesJapanese()); // Add one of the default ranges
      builder.BuildRanges(&ranges);                          // Build the final result (ordered ranges with all the unique characters submitted)
      io.Fonts->AddFontFromFileTTF("myfontfile.ttf", size_in_pixels, NULL, ranges.Data);

    All your strings needs to use UTF-8 encoding. In C++11 you can encode a string literal in UTF-8
    by using the u8"hello" syntax. Specifying literal in your source code using a local code page
    (such as CP-923 for Japanese or CP-1251 for Cyrillic) will NOT work!
    Otherwise you can convert yourself to UTF-8 or load text data from file already saved as UTF-8.

    Text input: it is up to your application to pass the right character code by calling io.AddInputCharacter(). 
    The applications in examples/ are doing that. 
    Windows: you can use the WM_CHAR or WM_UNICHAR or WM_IME_CHAR message (depending if your app is built using Unicode or MultiByte mode).
    You may also use MultiByteToWideChar() or ToUnicode() to retrieve Unicode codepoints from MultiByte characters or keyboard state.
    Windows: if your language is relying on an Input Method Editor (IME), you copy the HWND of your window to io.ImeWindowHandle in order for 
    the default implementation of io.ImeSetInputScreenPosFn() to set your Microsoft IME position correctly.

 Q: How can I interact with standard C++ types (such as std::string and std::vector)? 
 A: - Being highly portable (bindings for several languages, frameworks, programming style, obscure or older platforms/compilers),
      and aiming for compatibility & performance suitable for every modern real-time game engines, dear imgui does not use
      any of std C++ types. We use raw types (e.g. char* instead of std::string) because they adapt to more use cases.
    - To use ImGui::InputText() with a std::string or any resizable string class, see misc/cpp/imgui_stdlib.h.
    - To use combo boxes and list boxes with std::vector or any other data structure: the BeginCombo()/EndCombo() API
      lets you iterate and submit items yourself, so does the ListBoxHeader()/ListBoxFooter() API.
      Prefer using them over the old and awkward Combo()/ListBox() api.
    - Generally for most high-level types you should be able to access the underlying data type. 
      You may write your own one-liner wrappers to facilitate user code (tip: add new functions in ImGui:: namespace from your code).
    - Dear ImGui applications often need to make intensive use of strings. It is expected that many of the strings you will pass
      to the API are raw literals (free in C/C++) or allocated in a manner that won't incur a large cost on your application.
      Please bear in mind that using std::string on applications with large amount of UI may incur unsatisfactory performances.
      Modern implementations of std::string often include small-string optimization (which is often a local buffer) but those 
      are not configurable and not the same across implementations. 
    - If you are finding your UI traversal cost to be too large, make sure your string usage is not leading to excessive amount 
      of heap allocations. Consider using literals, statically sized buffers and your own helper functions. A common pattern
      is that you will need to build lots of strings on the fly, and their maximum length can be easily be scoped ahead. 
      One possible implementation of a helper to facilitate printf-style building of strings: https://github.com/ocornut/Str
      This is a small helper where you can instance strings with configurable local buffers length. Many game engines will 
      provide similar or better string helpers.

 Q: How can I use the drawing facilities without an ImGui window? (using ImDrawList API)
 A: - You can create a dummy window. Call Begin() with the NoBackground | NoDecoration | NoSavedSettings | NoInputs flags.
      (The ImGuiWindowFlags_NoDecoration flag itself is a shortcut for NoTitleBar | NoResize | NoScrollbar | NoCollapse)
      Then you can retrieve the ImDrawList* via GetWindowDrawList() and draw to it in any way you like.
    - You can call ImGui::GetOverlayDrawList() and use this draw list to display contents over every other imgui windows (1 overlay per viewport).
    - You can create your own ImDrawList instance. You'll need to initialize them ImGui::GetDrawListSharedData(), or create your own ImDrawListSharedData,
      and then call your rendered code with your own ImDrawList or ImDrawData data.

 Q: How can I use this without a mouse, without a keyboard or without a screen? (gamepad, input share, remote display)
 A: - You can control Dear ImGui with a gamepad. Read about navigation in "Using gamepad/keyboard navigation controls".
      (short version: map gamepad inputs into the io.NavInputs[] array + set io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad)
    - You can share your computer mouse seamlessly with your console/tablet/phone using Synergy (https://symless.com/synergy) 
      This is the preferred solution for developer productivity. 
      In particular, the "micro-synergy-client" repository (https://github.com/symless/micro-synergy-client) has simple
      and portable source code (uSynergy.c/.h) for a small embeddable client that you can use on any platform to connect 
      to your host computer, based on the Synergy 1.x protocol. Make sure you download the Synergy 1 server on your computer.
      Console SDK also sometimes provide equivalent tooling or wrapper for Synergy-like protocols.
    - You may also use a third party solution such as Remote ImGui (https://github.com/JordiRos/remoteimgui) which sends 
      the vertices to render over the local network, allowing you to use Dear ImGui even on a screen-less machine.
    - For touch inputs, you can increase the hit box of widgets (via the style.TouchPadding setting) to accommodate 
      for the lack of precision of touch inputs, but it is recommended you use a mouse or gamepad to allow optimizing
      for screen real-estate and precision.

 Q: I integrated Dear ImGui in my engine and the text or lines are blurry..
 A: In your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f).
    Also make sure your orthographic projection matrix and io.DisplaySize matches your actual framebuffer dimension.

 Q: I integrated Dear ImGui in my engine and some elements are clipping or disappearing when I move windows around..
 A: You are probably mishandling the clipping rectangles in your render function.
    Rectangles provided by ImGui are defined as (x1=left,y1=top,x2=right,y2=bottom) and NOT as (x1,y1,width,height).

 Q: How can I help?
 A: - If you are experienced with Dear ImGui and C++, look at the github issues, look at the Wiki, read docs/TODO.txt 
      and see how you want to help and can help!
    - Businesses: convince your company to fund development via support contracts/sponsoring! This is among the most useful thing you can do for dear imgui.
    - Individuals: you can also become a Patron (http://www.patreon.com/imgui) or donate on PayPal! See README.
    - Disclose your usage of dear imgui via a dev blog post, a tweet, a screenshot, a mention somewhere etc.
      You may post screenshot or links in the gallery threads (github.com/ocornut/imgui/issues/1902). Visuals are ideal as they inspire other programmers.
      But even without visuals, disclosing your use of dear imgui help the library grow credibility, and help other teams and programmers with taking decisions.
    - If you have issues or if you need to hack into the library, even if you don't expect any support it is useful that you share your issues (on github or privately).

 - tip: you can call Begin() multiple times with the same name during the same frame, it will keep appending to the same window.
        this is also useful to set yourself in the context of another window (to get/set other settings)
 - tip: you can create widgets without a Begin()/End() block, they will go in an implicit window called "Debug".
 - tip: the ImGuiOnceUponAFrame helper will allow run the block of code only once a frame. You can use it to quickly add custom UI in the middle
        of a deep nested inner loop in your code.
 - tip: you can call Render() multiple times (e.g for VR renders).
 - tip: call and read the ShowDemoWindow() code in imgui_demo.cpp for more example of how to use ImGui!

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"

#ifdef IMGUI_ENABLED

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <ctype.h>      // toupper, isprint
#include <stdio.h>      // vsnprintf, sscanf, printf
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif

// Debug options
#define IMGUI_DEBUG_NAV_SCORING     0   // Display navigation scoring preview when hovering items. Display last moving direction matches when holding CTRL
#define IMGUI_DEBUG_NAV_RECTS       0   // Display the reference navigation rectangle for each window
#define IMGUI_DEBUG_DOCKING_INI     0   // Save additional comments in .ini file (makes saving slower)

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)     // condition expression is constant
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

// Clang/GCC warnings with -Weverything
#ifdef __clang__
#pragma clang diagnostic ignored "-Wunknown-pragmas"        // warning : unknown warning group '-Wformat-pedantic *'        // not all warnings are known by all clang versions.. so ignoring warnings triggers new warnings on some configuration. great!
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning : use of old-style cast                              // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"            // warning : comparing floating point with == or != is unsafe   // storing and comparing against same constants (typically 0.0f) is ok.
#pragma clang diagnostic ignored "-Wformat-nonliteral"      // warning : format string is not a string literal              // passing non-literal to vsnformat(). yes, user passing incorrect format strings can crash the code.
#pragma clang diagnostic ignored "-Wexit-time-destructors"  // warning : declaration requires an exit-time destructor       // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. ImGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wglobal-constructors"    // warning : declaration requires a global destructor           // similar to above, not sure what the exact difference it.
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning : implicit conversion changes signedness             //
#pragma clang diagnostic ignored "-Wformat-pedantic"        // warning : format specifies type 'void *' but the argument has type 'xxxx *' // unreasonable, would lead to casting every %p arg to void*. probably enabled by -pedantic.
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"       // warning : cast to 'void *' from smaller integer type 'int'
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning : zero as null pointer constant              // some standard header variations use #define NULL 0
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion"       // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"      // warning: cast to pointer from integer of different size
#pragma GCC diagnostic ignored "-Wformat"                   // warning: format '%p' expects argument of type 'void*', but argument 6 has type 'ImGuiWindow*'
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wformat-nonliteral"        // warning: format not a string literal, format string not checked
#pragma GCC diagnostic ignored "-Wstrict-overflow"          // warning: assuming signed overflow does not occur when assuming that (X - c) > X is always false
#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wclass-memaccess"          // warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif
#endif

// When using CTRL+TAB (or Gamepad Square+L/R) we delay the visual a little in order to reduce visual noise doing a fast switch.
static const float NAV_WINDOWING_HIGHLIGHT_DELAY            = 0.20f;    // Time before the highlight and screen dimming starts fading in
static const float NAV_WINDOWING_LIST_APPEAR_DELAY          = 0.15f;    // Time before the window list starts to appear

// Window resizing from edges (when io.ConfigWindowsResizeFromEdges = true and ImGuiBackendFlags_HasMouseCursors is set in io.BackendFlags by back-end)
static const float WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS = 4.0f;     // Extend outside and inside windows. Affect FindHoveredWindow().
static const float WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER = 0.04f;    // Reduce visual noise by only highlighting the border after a certain time.

// Docking
static const float DOCKING_TRANSPARENT_PAYLOAD_ALPHA        = 0.50f;    // For use with io.ConfigDockingTransparentPayload. Apply to Viewport _or_ WindowBg in host viewport.

//-------------------------------------------------------------------------
// [SECTION] FORWARD DECLARATIONS
//-------------------------------------------------------------------------

static void             SetCurrentWindow(ImGuiWindow* window);
static void             SetWindowPos(ImGuiWindow* window, const ImVec2& pos, ImGuiCond cond);
static void             SetWindowSize(ImGuiWindow* window, const ImVec2& size, ImGuiCond cond);
static void             SetWindowCollapsed(ImGuiWindow* window, bool collapsed, ImGuiCond cond);
static void             SetWindowHitTestHole(ImGuiWindow* window, const ImVec2& pos, const ImVec2& size);
static void             FindHoveredWindow();
static ImGuiWindow*     CreateNewWindow(const char* name, ImVec2 size, ImGuiWindowFlags flags);
static void             CheckStacksSize(ImGuiWindow* window, bool write);
static ImVec2           CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window, bool snap_on_edges);

static void             AddDrawListToDrawData(ImVector<ImDrawList*>* out_list, ImDrawList* draw_list);
static void             AddWindowToSortBuffer(ImVector<ImGuiWindow*>* out_sorted_windows, ImGuiWindow* window);

// Settings
static void*            SettingsHandlerWindow_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
static void             SettingsHandlerWindow_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
static void             SettingsHandlerWindow_WriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);

// Platform Dependents default implementation for IO functions
static const char*      GetClipboardTextFn_DefaultImpl(void* user_data);
static void             SetClipboardTextFn_DefaultImpl(void* user_data, const char* text);

namespace ImGui
{
static bool             BeginChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags);

// Navigation
static void             NavUpdate();
static void             NavUpdateWindowing();
static void             NavUpdateWindowingList();
static void             NavUpdateMoveResult();
static float            NavUpdatePageUpPageDown(int allowed_dir_flags);
static inline void      NavUpdateAnyRequestFlag();
static void             NavProcessItem(ImGuiWindow* window, const ImRect& nav_bb, const ImGuiID id);
static ImVec2           NavCalcPreferredRefPos();
static void             NavSaveLastChildNavWindow(ImGuiWindow* nav_window);
static ImGuiWindow*     NavRestoreLastChildNavWindow(ImGuiWindow* window);

// Misc
static void             UpdateMouseInputs();
static void             UpdateMouseWheel();
static void             UpdateManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4]);
static void             EndFrameDrawDimmedBackgrounds();

// Viewports
const ImGuiID           IMGUI_VIEWPORT_DEFAULT_ID = 0x11111111; // Using an arbitrary constant instead of e.g. ImHash("ViewportDefault", 0); so it's easier to spot in the debugger. The exact value doesn't matter.
static ImGuiViewportP*  AddUpdateViewport(ImGuiWindow* window, ImGuiID id, const ImVec2& platform_pos, const ImVec2& size, ImGuiViewportFlags flags);
static void             UpdateViewportsNewFrame();
static void             UpdateViewportsEndFrame();
static void             UpdateSelectWindowViewport(ImGuiWindow* window);
static bool             UpdateTryMergeWindowIntoHostViewport(ImGuiWindow* window, ImGuiViewportP* host_viewport);
static void             SetCurrentViewport(ImGuiWindow* window, ImGuiViewportP* viewport);
static bool             GetWindowAlwaysWantOwnViewport(ImGuiWindow* window);
static int              FindPlatformMonitorForPos(const ImVec2& pos);
static int              FindPlatformMonitorForRect(const ImRect& r);

}

//-----------------------------------------------------------------------------
// [SECTION] CONTEXT AND MEMORY ALLOCATORS
//-----------------------------------------------------------------------------

// Current context pointer. Implicitly used by all Dear ImGui functions. Always assumed to be != NULL.
// CreateContext() will automatically set this pointer if it is NULL. Change to a different context by calling ImGui::SetCurrentContext().
// If you use DLL hotreloading you might need to call SetCurrentContext() after reloading code from this file.
// ImGui functions are not thread-safe because of this pointer. If you want thread-safety to allow N threads to access N different contexts, you can:
// - Change this variable to use thread local storage. You may #define GImGui in imconfig.h for that purpose. Future development aim to make this context pointer explicit to all calls. Also read https://github.com/ocornut/imgui/issues/586
// - Having multiple instances of the ImGui code compiled inside different namespace (easiest/safest, if you have a finite number of contexts)
#ifndef GImGui
ImGuiContext*   GImGui = NULL;
#endif

// Memory Allocator functions. Use SetAllocatorFunctions() to change them.
// If you use DLL hotreloading you might need to call SetAllocatorFunctions() after reloading code from this file.
// Otherwise, you probably don't want to modify them mid-program, and if you use global/static e.g. ImVector<> instances you may need to keep them accessible during program destruction.
#ifndef IMGUI_DISABLE_DEFAULT_ALLOCATORS
static void*   MallocWrapper(size_t size, void* user_data)    { (void)user_data; return malloc(size); }
static void    FreeWrapper(void* ptr, void* user_data)        { (void)user_data; free(ptr); }
#else
static void*   MallocWrapper(size_t size, void* user_data)    { (void)user_data; (void)size; IM_ASSERT(0); return NULL; }
static void    FreeWrapper(void* ptr, void* user_data)        { (void)user_data; (void)ptr; IM_ASSERT(0); }
#endif

static void*  (*GImAllocatorAllocFunc)(size_t size, void* user_data) = MallocWrapper;
static void   (*GImAllocatorFreeFunc)(void* ptr, void* user_data) = FreeWrapper;
static void*    GImAllocatorUserData = NULL;

//-----------------------------------------------------------------------------
// [SECTION] MAIN USER FACING STRUCTURES (ImGuiStyle, ImGuiIO)
//-----------------------------------------------------------------------------

ImGuiStyle::ImGuiStyle()
{
    Alpha                   = 1.0f;             // Global alpha applies to everything in ImGui
    WindowPadding           = ImVec2(8,8);      // Padding within a window
    WindowRounding          = 7.0f;             // Radius of window corners rounding. Set to 0.0f to have rectangular windows
    WindowBorderSize        = 1.0f;             // Thickness of border around windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    WindowMinSize           = ImVec2(32,32);    // Minimum window size
    WindowTitleAlign        = ImVec2(0.0f,0.5f);// Alignment for title bar text
    ChildRounding           = 0.0f;             // Radius of child window corners rounding. Set to 0.0f to have rectangular child windows
    ChildBorderSize         = 1.0f;             // Thickness of border around child windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    PopupRounding           = 0.0f;             // Radius of popup window corners rounding. Set to 0.0f to have rectangular child windows
    PopupBorderSize         = 1.0f;             // Thickness of border around popup or tooltip windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    FramePadding            = ImVec2(4,3);      // Padding within a framed rectangle (used by most widgets)
    FrameRounding           = 0.0f;             // Radius of frame corners rounding. Set to 0.0f to have rectangular frames (used by most widgets).
    FrameBorderSize         = 0.0f;             // Thickness of border around frames. Generally set to 0.0f or 1.0f. Other values not well tested.
    ItemSpacing             = ImVec2(8,4);      // Horizontal and vertical spacing between widgets/lines
    ItemInnerSpacing        = ImVec2(4,4);      // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)
    TouchExtraPadding       = ImVec2(0,0);      // Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!
    IndentSpacing           = 21.0f;            // Horizontal spacing when e.g. entering a tree node. Generally == (FontSize + FramePadding.x*2).
    ColumnsMinSpacing       = 6.0f;             // Minimum horizontal spacing between two columns
    ScrollbarSize           = 16.0f;            // Width of the vertical scrollbar, Height of the horizontal scrollbar
    ScrollbarRounding       = 9.0f;             // Radius of grab corners rounding for scrollbar
    GrabMinSize             = 10.0f;            // Minimum width/height of a grab box for slider/scrollbar
    GrabRounding            = 0.0f;             // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
    TabRounding             = 4.0f;             // Radius of upper corners of a tab. Set to 0.0f to have rectangular tabs.
    TabBorderSize           = 0.0f;             // Thickness of border around tabs.
    ButtonTextAlign         = ImVec2(0.5f,0.5f);// Alignment of button text when button is larger than text.
    DisplayWindowPadding    = ImVec2(19,19);    // Window position are clamped to be visible within the display area or monitors by at least this amount. Only applies to regular windows.
    DisplaySafeAreaPadding  = ImVec2(3,3);      // If you cannot see the edge of your screen (e.g. on a TV) increase the safe area padding. Covers popups/tooltips as well regular windows.
    MouseCursorScale        = 1.0f;             // Scale software rendered mouse cursor (when io.MouseDrawCursor is enabled). May be removed later.
    AntiAliasedLines        = true;             // Enable anti-aliasing on lines/borders. Disable if you are really short on CPU/GPU.
    AntiAliasedFill         = true;             // Enable anti-aliasing on filled shapes (rounded rectangles, circles, etc.)
    CurveTessellationTol    = 1.25f;            // Tessellation tolerance when using PathBezierCurveTo() without a specific number of segments. Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.

    // Default theme
    ImGui::StyleColorsDark(this);
}

// To scale your entire UI (e.g. if you want your app to use High DPI or generally be DPI aware) you may use this helper function. Scaling the fonts is done separately and is up to you.
// Important: This operation is lossy because we round all sizes to integer. If you need to change your scale multiples, call this over a freshly initialized ImGuiStyle structure rather than scaling multiple times.
void ImGuiStyle::ScaleAllSizes(float scale_factor)
{
    WindowPadding = ImFloor(WindowPadding * scale_factor);
    WindowRounding = ImFloor(WindowRounding * scale_factor);
    WindowMinSize = ImFloor(WindowMinSize * scale_factor);
    ChildRounding = ImFloor(ChildRounding * scale_factor);
    PopupRounding = ImFloor(PopupRounding * scale_factor);
    FramePadding = ImFloor(FramePadding * scale_factor);
    FrameRounding = ImFloor(FrameRounding * scale_factor);
    TabRounding = ImFloor(TabRounding * scale_factor);
    ItemSpacing = ImFloor(ItemSpacing * scale_factor);
    ItemInnerSpacing = ImFloor(ItemInnerSpacing * scale_factor);
    TouchExtraPadding = ImFloor(TouchExtraPadding * scale_factor);
    IndentSpacing = ImFloor(IndentSpacing * scale_factor);
    ColumnsMinSpacing = ImFloor(ColumnsMinSpacing * scale_factor);
    ScrollbarSize = ImFloor(ScrollbarSize * scale_factor);
    ScrollbarRounding = ImFloor(ScrollbarRounding * scale_factor);
    GrabMinSize = ImFloor(GrabMinSize * scale_factor);
    GrabRounding = ImFloor(GrabRounding * scale_factor);
    DisplayWindowPadding = ImFloor(DisplayWindowPadding * scale_factor);
    DisplaySafeAreaPadding = ImFloor(DisplaySafeAreaPadding * scale_factor);
    MouseCursorScale = ImFloor(MouseCursorScale * scale_factor);
}

ImGuiIO::ImGuiIO()
{
    // Most fields are initialized with zero
    memset(this, 0, sizeof(*this));

    // Settings
    ConfigFlags = ImGuiConfigFlags_None;
    BackendFlags = ImGuiBackendFlags_None;
    DisplaySize = ImVec2(-1.0f, -1.0f);
    DeltaTime = 1.0f/60.0f;
    IniSavingRate = 5.0f;
    IniFilename = "imgui.ini";
    LogFilename = "imgui_log.txt";
    MouseDoubleClickTime = 0.30f;
    MouseDoubleClickMaxDist = 6.0f;
    for (int i = 0; i < ImGuiKey_COUNT; i++)
        KeyMap[i] = -1;
    KeyRepeatDelay = 0.250f;
    KeyRepeatRate = 0.050f;
    UserData = NULL;

    Fonts = NULL;
    FontGlobalScale = 1.0f;
    FontDefault = NULL;
    FontAllowUserScaling = false;
    DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Docking options (when ImGuiConfigFlags_DockingEnable is set)
    ConfigDockingNoSplit = false;
    ConfigDockingWithShift = false;
    ConfigDockingTabBarOnSingleWindows = false;
    ConfigDockingTransparentPayload = false;

    // Viewport options (when ImGuiConfigFlags_ViewportsEnable is set)
    ConfigViewportsNoAutoMerge = false;
    ConfigViewportsNoTaskBarIcon = false;
    ConfigViewportsNoDecoration = true;
    ConfigViewportsNoParent = false;

    // Miscellaneous options
    MouseDrawCursor = false;
#ifdef __APPLE__
    ConfigMacOSXBehaviors = true;  // Set Mac OS X style defaults based on __APPLE__ compile time flag
#else
    ConfigMacOSXBehaviors = false;
#endif
    ConfigInputTextCursorBlink = true;
    ConfigWindowsResizeFromEdges = true;
    ConfigWindowsMoveFromTitleBarOnly = false;

    // Platform Functions
    BackendPlatformName = BackendRendererName = NULL;
    BackendPlatformUserData = BackendRendererUserData = BackendLanguageUserData = NULL;
    GetClipboardTextFn = GetClipboardTextFn_DefaultImpl;   // Platform dependent default implementations
    SetClipboardTextFn = SetClipboardTextFn_DefaultImpl;
    ClipboardUserData = NULL;

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    RenderDrawListsFn = NULL;
#endif

    // Input (NB: we already have memset zero the entire structure!)
    MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    MousePosPrev = ImVec2(-FLT_MAX, -FLT_MAX);
    MouseDragThreshold = 6.0f;
    for (int i = 0; i < IM_ARRAYSIZE(MouseDownDuration); i++) MouseDownDuration[i] = MouseDownDurationPrev[i] = -1.0f;
    for (int i = 0; i < IM_ARRAYSIZE(KeysDownDuration); i++) KeysDownDuration[i]  = KeysDownDurationPrev[i] = -1.0f;
    for (int i = 0; i < IM_ARRAYSIZE(NavInputsDownDuration); i++) NavInputsDownDuration[i] = -1.0f;
}

// Pass in translated ASCII characters for text input.
// - with glfw you can get those from the callback set in glfwSetCharCallback()
// - on Windows you can get those using ToAscii+keyboard state, or via the WM_CHAR message
void ImGuiIO::AddInputCharacter(ImWchar c)
{
    InputQueueCharacters.push_back(c);
}

void ImGuiIO::AddInputCharactersUTF8(const char* utf8_chars)
{
    while (*utf8_chars != 0)
    {
        unsigned int c = 0;
        utf8_chars += ImTextCharFromUtf8(&c, utf8_chars, NULL);
        if (c > 0 && c <= 0xFFFF)
            InputQueueCharacters.push_back((ImWchar)c);
    }
}

void ImGuiIO::ClearInputCharacters()
{
    InputQueueCharacters.resize(0);
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPER/UTILITIES (Maths, String, Format, Hash, File functions)
//-----------------------------------------------------------------------------

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

int ImStricmp(const char* str1, const char* str2)
{
    int d;
    while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; }
    return d;
}

int ImStrnicmp(const char* str1, const char* str2, size_t count)
{
    int d = 0;
    while (count > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; count--; }
    return d;
}

void ImStrncpy(char* dst, const char* src, size_t count)
{
    if (count < 1) return;
    strncpy(dst, src, count);
    dst[count-1] = 0;
}

char* ImStrdup(const char* str)
{
    size_t len = strlen(str);
    void* buf = ImGui::MemAlloc(len + 1);
    return (char*)memcpy(buf, (const void*)str, len + 1);
}

char* ImStrdupcpy(char* dst, size_t* p_dst_size, const char* src)
{
    size_t dst_buf_size = p_dst_size ? *p_dst_size : strlen(dst) + 1;
    size_t src_size = strlen(src) + 1;
    if (dst_buf_size < src_size)
    {
        ImGui::MemFree(dst);
        dst = (char*)ImGui::MemAlloc(src_size);
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
    //return (int)wcslen((const wchar_t*)str);	// FIXME-OPT: Could use this when wchar_t are 16-bits
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

const ImWchar* ImStrbolW(const ImWchar* buf_mid_line, const ImWchar* buf_begin) // find beginning-of-line
{
    while (buf_mid_line > buf_begin && buf_mid_line[-1] != '\n')
        buf_mid_line--;
    return buf_mid_line;
}

const char* ImStristr(const char* haystack, const char* haystack_end, const char* needle, const char* needle_end)
{
    if (!needle_end)
        needle_end = needle + strlen(needle);

    const char un0 = (char)toupper(*needle);
    while ((!haystack_end && *haystack) || (haystack_end && haystack < haystack_end))
    {
        if (toupper(*haystack) == un0)
        {
            const char* b = needle + 1;
            for (const char* a = haystack + 1; b < needle_end; a++, b++)
                if (toupper(*a) != toupper(*b))
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
#ifndef IMGUI_DISABLE_FORMAT_STRING_FUNCTIONS

//#define IMGUI_USE_STB_SPRINTF
#ifdef IMGUI_USE_STB_SPRINTF
#define STB_SPRINTF_IMPLEMENTATION
#include "imstb_sprintf.h"
#endif

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
#endif // #ifdef IMGUI_DISABLE_FORMAT_STRING_FUNCTIONS

// Pass data_size == 0 for zero-terminated strings, data_size > 0 for non-string data.
// Pay attention that data_size==0 will yield different results than passing strlen(data) because the zero-terminated codepath handles ###.
// This should technically be split into two distinct functions (ImHashData/ImHashStr), perhaps once we remove the silly static variable.
// FIXME-OPT: Replace with e.g. FNV1a hash? CRC32 pretty much randomly access 1KB. Need to do proper measurements.
ImU32 ImHash(const void* data, int data_size, ImU32 seed)
{
    static ImU32 crc32_lut[256] = { 0 };
    if (!crc32_lut[1])
    {
        const ImU32 polynomial = 0xEDB88320;
        for (ImU32 i = 0; i < 256; i++)
        {
            ImU32 crc = i;
            for (ImU32 j = 0; j < 8; j++)
                crc = (crc >> 1) ^ (ImU32(-int(crc & 1)) & polynomial);
            crc32_lut[i] = crc;
        }
    }

    seed = ~seed;
    ImU32 crc = seed;
    const unsigned char* current = (const unsigned char*)data;

    if (data_size > 0)
    {
        // Known size
        while (data_size--)
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ *current++];
    }
    else
    {
        // Zero-terminated string
        while (unsigned char c = *current++)
        {
            // We support a syntax of "label###id" where only "###id" is included in the hash, and only "label" gets displayed.
            // Because this syntax is rarely used we are optimizing for the common case.
            // - If we reach ### in the string we discard the hash so far and reset to the seed.
            // - We don't do 'current += 2; continue;' after handling ### to keep the code smaller.
            if (c == '#' && current[0] == '#' && current[1] == '#')
                crc = seed;
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        }
    }
    return ~crc;
}

FILE* ImFileOpen(const char* filename, const char* mode)
{
#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(__GNUC__)
    // We need a fopen() wrapper because MSVC/Windows fopen doesn't handle UTF-8 filenames. Converting both strings from UTF-8 to wchar format (using a single allocation, because we can)
    const int filename_wsize = ImTextCountCharsFromUtf8(filename, NULL) + 1;
    const int mode_wsize = ImTextCountCharsFromUtf8(mode, NULL) + 1;
    ImVector<ImWchar> buf;
    buf.resize(filename_wsize + mode_wsize);
    ImTextStrFromUtf8(&buf[0], filename_wsize, filename, NULL);
    ImTextStrFromUtf8(&buf[filename_wsize], mode_wsize, mode, NULL);
    return _wfopen((wchar_t*)&buf[0], (wchar_t*)&buf[filename_wsize]);
#else
    return fopen(filename, mode);
#endif
}

// Load file content into memory
// Memory allocated with ImGui::MemAlloc(), must be freed by user using ImGui::MemFree()
void* ImFileLoadToMemory(const char* filename, const char* file_open_mode, size_t* out_file_size, int padding_bytes)
{
    IM_ASSERT(filename && file_open_mode);
    if (out_file_size)
        *out_file_size = 0;

    FILE* f;
    if ((f = ImFileOpen(filename, file_open_mode)) == NULL)
        return NULL;

    long file_size_signed;
    if (fseek(f, 0, SEEK_END) || (file_size_signed = ftell(f)) == -1 || fseek(f, 0, SEEK_SET))
    {
        fclose(f);
        return NULL;
    }

    size_t file_size = (size_t)file_size_signed;
    void* file_data = ImGui::MemAlloc(file_size + padding_bytes);
    if (file_data == NULL)
    {
        fclose(f);
        return NULL;
    }
    if (fread(file_data, 1, file_size, f) != file_size)
    {
        fclose(f);
        ImGui::MemFree(file_data);
        return NULL;
    }
    if (padding_bytes > 0)
        memset((void*)(((char*)file_data) + file_size), 0, (size_t)padding_bytes);

    fclose(f);
    if (out_file_size)
        *out_file_size = file_size;

    return file_data;
}

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPERS/UTILITIES (ImText* functions)
//-----------------------------------------------------------------------------

// Convert UTF-8 to 32-bits character, process single character input.
// Based on stb_from_utf8() from github.com/nothings/stb/
// We handle UTF-8 decoding error by skipping forward.
int ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end)
{
    unsigned int c = (unsigned int)-1;
    const unsigned char* str = (const unsigned char*)in_text;
    if (!(*str & 0x80))
    {
        c = (unsigned int)(*str++);
        *out_char = c;
        return 1;
    }
    if ((*str & 0xe0) == 0xc0)
    {
        *out_char = 0xFFFD; // will be invalid but not end of string
        if (in_text_end && in_text_end - (const char*)str < 2) return 1;
        if (*str < 0xc2) return 2;
        c = (unsigned int)((*str++ & 0x1f) << 6);
        if ((*str & 0xc0) != 0x80) return 2;
        c += (*str++ & 0x3f);
        *out_char = c;
        return 2;
    }
    if ((*str & 0xf0) == 0xe0)
    {
        *out_char = 0xFFFD; // will be invalid but not end of string
        if (in_text_end && in_text_end - (const char*)str < 3) return 1;
        if (*str == 0xe0 && (str[1] < 0xa0 || str[1] > 0xbf)) return 3;
        if (*str == 0xed && str[1] > 0x9f) return 3; // str[1] < 0x80 is checked below
        c = (unsigned int)((*str++ & 0x0f) << 12);
        if ((*str & 0xc0) != 0x80) return 3;
        c += (unsigned int)((*str++ & 0x3f) << 6);
        if ((*str & 0xc0) != 0x80) return 3;
        c += (*str++ & 0x3f);
        *out_char = c;
        return 3;
    }
    if ((*str & 0xf8) == 0xf0)
    {
        *out_char = 0xFFFD; // will be invalid but not end of string
        if (in_text_end && in_text_end - (const char*)str < 4) return 1;
        if (*str > 0xf4) return 4;
        if (*str == 0xf0 && (str[1] < 0x90 || str[1] > 0xbf)) return 4;
        if (*str == 0xf4 && str[1] > 0x8f) return 4; // str[1] < 0x80 is checked below
        c = (unsigned int)((*str++ & 0x07) << 18);
        if ((*str & 0xc0) != 0x80) return 4;
        c += (unsigned int)((*str++ & 0x3f) << 12);
        if ((*str & 0xc0) != 0x80) return 4;
        c += (unsigned int)((*str++ & 0x3f) << 6);
        if ((*str & 0xc0) != 0x80) return 4;
        c += (*str++ & 0x3f);
        // utf-8 encodings of values used in surrogate pairs are invalid
        if ((c & 0xFFFFF800) == 0xD800) return 4;
        *out_char = c;
        return 4;
    }
    *out_char = 0;
    return 0;
}

int ImTextStrFromUtf8(ImWchar* buf, int buf_size, const char* in_text, const char* in_text_end, const char** in_text_remaining)
{
    ImWchar* buf_out = buf;
    ImWchar* buf_end = buf + buf_size;
    while (buf_out < buf_end-1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        if (c == 0)
            break;
        if (c < 0x10000)    // FIXME: Losing characters that don't fit in 2 bytes
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
        if (c == 0)
            break;
        if (c < 0x10000)
            char_count++;
    }
    return char_count;
}

// Based on stb_to_utf8() from github.com/nothings/stb/
static inline int ImTextCharToUtf8(char* buf, int buf_size, unsigned int c)
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
    if (c >= 0xdc00 && c < 0xe000)
    {
        return 0;
    }
    if (c >= 0xd800 && c < 0xdc00)
    {
        if (buf_size < 4) return 0;
        buf[0] = (char)(0xf0 + (c >> 18));
        buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
        buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[3] = (char)(0x80 + ((c ) & 0x3f));
        return 4;
    }
    //else if (c < 0x10000)
    {
        if (buf_size < 3) return 0;
        buf[0] = (char)(0xe0 + (c >> 12));
        buf[1] = (char)(0x80 + ((c>> 6) & 0x3f));
        buf[2] = (char)(0x80 + ((c ) & 0x3f));
        return 3;
    }
}

// Not optimal but we very rarely use this function.
int ImTextCountUtf8BytesFromChar(const char* in_text, const char* in_text_end)
{
    unsigned int dummy = 0;
    return ImTextCharFromUtf8(&dummy, in_text, in_text_end);
}

static inline int ImTextCountUtf8BytesFromChar(unsigned int c)
{
    if (c < 0x80) return 1;
    if (c < 0x800) return 2;
    if (c >= 0xdc00 && c < 0xe000) return 0;
    if (c >= 0xd800 && c < 0xdc00) return 4;
    return 3;
}

int ImTextStrToUtf8(char* buf, int buf_size, const ImWchar* in_text, const ImWchar* in_text_end)
{
    char* buf_out = buf;
    const char* buf_end = buf + buf_size;
    while (buf_out < buf_end-1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c = (unsigned int)(*in_text++);
        if (c < 0x80)
            *buf_out++ = (char)c;
        else
            buf_out += ImTextCharToUtf8(buf_out, (int)(buf_end-buf_out-1), c);
    }
    *buf_out = 0;
    return (int)(buf_out - buf);
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

//-----------------------------------------------------------------------------
// [SECTION] MISC HELPER/UTILTIES (Color functions)
// Note: The Convert functions are early design which are not consistent with other API.
//-----------------------------------------------------------------------------

ImVec4 ImGui::ColorConvertU32ToFloat4(ImU32 in)
{
    float s = 1.0f/255.0f;
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

    h = ImFmod(h, 1.0f) / (60.0f/360.0f);
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

ImU32 ImGui::GetColorU32(ImU32 col)
{
    float style_alpha = GImGui->Style.Alpha;
    if (style_alpha >= 1.0f)
        return col;
    ImU32 a = (col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT;
    a = (ImU32)(a * style_alpha); // We don't need to clamp 0..255 because Style.Alpha is in 0..1 range.
    return (col & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiStorage
// Helper: Key->value storage
//-----------------------------------------------------------------------------

// std::lower_bound but without the bullshit
static ImGuiStorage::Pair* LowerBound(ImVector<ImGuiStorage::Pair>& data, ImGuiID key)
{
    ImGuiStorage::Pair* first = data.Data;
    ImGuiStorage::Pair* last = data.Data + data.Size;
    size_t count = (size_t)(last - first);
    while (count > 0)
    {
        size_t count2 = count >> 1;
        ImGuiStorage::Pair* mid = first + count2;
        if (mid->key < key)
        {
            first = ++mid;
            count -= count2 + 1;
        }
        else
        {
            count = count2;
        }
    }
    return first;
}

// For quicker full rebuild of a storage (instead of an incremental one), you may add all your contents and then sort once.
void ImGuiStorage::BuildSortByKey()
{
    struct StaticFunc
    {
        static int IMGUI_CDECL PairCompareByID(const void* lhs, const void* rhs)
        {
            // We can't just do a subtraction because qsort uses signed integers and subtracting our ID doesn't play well with that.
            if (((const Pair*)lhs)->key > ((const Pair*)rhs)->key) return +1;
            if (((const Pair*)lhs)->key < ((const Pair*)rhs)->key) return -1;
            return 0;
        }
    };
    if (Data.Size > 1)
        ImQsort(Data.Data, (size_t)Data.Size, sizeof(Pair), StaticFunc::PairCompareByID);
}

int ImGuiStorage::GetInt(ImGuiID key, int default_val) const
{
    ImGuiStorage::Pair* it = LowerBound(const_cast<ImVector<ImGuiStorage::Pair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_i;
}

bool ImGuiStorage::GetBool(ImGuiID key, bool default_val) const
{
    return GetInt(key, default_val ? 1 : 0) != 0;
}

float ImGuiStorage::GetFloat(ImGuiID key, float default_val) const
{
    ImGuiStorage::Pair* it = LowerBound(const_cast<ImVector<ImGuiStorage::Pair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_f;
}

void* ImGuiStorage::GetVoidPtr(ImGuiID key) const
{
    ImGuiStorage::Pair* it = LowerBound(const_cast<ImVector<ImGuiStorage::Pair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return NULL;
    return it->val_p;
}

// References are only valid until a new value is added to the storage. Calling a Set***() function or a Get***Ref() function invalidates the pointer.
int* ImGuiStorage::GetIntRef(ImGuiID key, int default_val)
{
    ImGuiStorage::Pair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, Pair(key, default_val));
    return &it->val_i;
}

bool* ImGuiStorage::GetBoolRef(ImGuiID key, bool default_val)
{
    return (bool*)GetIntRef(key, default_val ? 1 : 0);
}

float* ImGuiStorage::GetFloatRef(ImGuiID key, float default_val)
{
    ImGuiStorage::Pair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, Pair(key, default_val));
    return &it->val_f;
}

void** ImGuiStorage::GetVoidPtrRef(ImGuiID key, void* default_val)
{
    ImGuiStorage::Pair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, Pair(key, default_val));
    return &it->val_p;
}

// FIXME-OPT: Need a way to reuse the result of lower_bound when doing GetInt()/SetInt() - not too bad because it only happens on explicit interaction (maximum one a frame)
void ImGuiStorage::SetInt(ImGuiID key, int val)
{
    ImGuiStorage::Pair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, Pair(key, val));
        return;
    }
    it->val_i = val;
}

void ImGuiStorage::SetBool(ImGuiID key, bool val)
{
    SetInt(key, val ? 1 : 0);
}

void ImGuiStorage::SetFloat(ImGuiID key, float val)
{
    ImGuiStorage::Pair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, Pair(key, val));
        return;
    }
    it->val_f = val;
}

void ImGuiStorage::SetVoidPtr(ImGuiID key, void* val)
{
    ImGuiStorage::Pair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, Pair(key, val));
        return;
    }
    it->val_p = val;
}

void ImGuiStorage::SetAllInt(int v)
{
    for (int i = 0; i < Data.Size; i++)
        Data[i].val_i = v;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiTextFilter
//-----------------------------------------------------------------------------

// Helper: Parse and apply text filters. In format "aaaaa[,bbbb][,ccccc]"
ImGuiTextFilter::ImGuiTextFilter(const char* default_filter)
{
    if (default_filter)
    {
        ImStrncpy(InputBuf, default_filter, IM_ARRAYSIZE(InputBuf));
        Build();
    }
    else
    {
        InputBuf[0] = 0;
        CountGrep = 0;
    }
}

bool ImGuiTextFilter::Draw(const char* label, float width)
{
    if (width != 0.0f)
        ImGui::PushItemWidth(width);
    bool value_changed = ImGui::InputText(label, InputBuf, IM_ARRAYSIZE(InputBuf));
    if (width != 0.0f)
        ImGui::PopItemWidth();
    if (value_changed)
        Build();
    return value_changed;
}

void ImGuiTextFilter::TextRange::split(char separator, ImVector<TextRange>* out) const
{
    out->resize(0);
    const char* wb = b;
    const char* we = wb;
    while (we < e)
    {
        if (*we == separator)
        {
            out->push_back(TextRange(wb, we));
            wb = we + 1;
        }
        we++;
    }
    if (wb != we)
        out->push_back(TextRange(wb, we));
}

void ImGuiTextFilter::Build()
{
    Filters.resize(0);
    TextRange input_range(InputBuf, InputBuf+strlen(InputBuf));
    input_range.split(',', &Filters);

    CountGrep = 0;
    for (int i = 0; i != Filters.Size; i++)
    {
        TextRange& f = Filters[i];
        while (f.b < f.e && ImCharIsBlankA(f.b[0]))
            f.b++;
        while (f.e > f.b && ImCharIsBlankA(f.e[-1]))
            f.e--;
        if (f.empty())
            continue;
        if (Filters[i].b[0] != '-')
            CountGrep += 1;
    }
}

bool ImGuiTextFilter::PassFilter(const char* text, const char* text_end) const
{
    if (Filters.empty())
        return true;

    if (text == NULL)
        text = "";

    for (int i = 0; i != Filters.Size; i++)
    {
        const TextRange& f = Filters[i];
        if (f.empty())
            continue;
        if (f.b[0] == '-')
        {
            // Subtract
            if (ImStristr(text, text_end, f.begin()+1, f.end()) != NULL)
                return false;
        }
        else
        {
            // Grep
            if (ImStristr(text, text_end, f.begin(), f.end()) != NULL)
                return true;
        }
    }

    // Implicit * grep
    if (CountGrep == 0)
        return true;

    return false;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiTextBuffer
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
        int double_capacity = Buf.Capacity * 2;
        Buf.reserve(needed_sz > double_capacity ? needed_sz : double_capacity);
    }

    Buf.resize(needed_sz);
    ImFormatStringV(&Buf[write_off - 1], (size_t)len + 1, fmt, args_copy);
    va_end(args_copy);
}

void ImGuiTextBuffer::appendf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiListClipper
// This is currently not as flexible/powerful as it should be, needs some rework (see TODO)
//-----------------------------------------------------------------------------

static void SetCursorPosYAndSetupDummyPrevLine(float pos_y, float line_height)
{
    // Set cursor position and a few other things so that SetScrollHereY() and Columns() can work when seeking cursor.
    // FIXME: It is problematic that we have to do that here, because custom/equivalent end-user code would stumble on the same issue.
    // The clipper should probably have a 4th step to display the last item in a regular manner.
    ImGui::SetCursorPosY(pos_y);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    window->DC.CursorPosPrevLine.y = window->DC.CursorPos.y - line_height;      // Setting those fields so that SetScrollHereY() can properly function after the end of our clipper usage.
    window->DC.PrevLineSize.y = (line_height - GImGui->Style.ItemSpacing.y);    // If we end up needing more accurate data (to e.g. use SameLine) we may as well make the clipper have a fourth step to let user process and display the last item in their list.
    if (window->DC.ColumnsSet)
        window->DC.ColumnsSet->LineMinY = window->DC.CursorPos.y;           // Setting this so that cell Y position are set properly
}

// Use case A: Begin() called from constructor with items_height<0, then called again from Sync() in StepNo 1
// Use case B: Begin() called from constructor with items_height>0
// FIXME-LEGACY: Ideally we should remove the Begin/End functions but they are part of the legacy API we still support. This is why some of the code in Step() calling Begin() and reassign some fields, spaghetti style.
void ImGuiListClipper::Begin(int count, float items_height)
{
    StartPosY = ImGui::GetCursorPosY();
    ItemsHeight = items_height;
    ItemsCount = count;
    StepNo = 0;
    DisplayEnd = DisplayStart = -1;
    if (ItemsHeight > 0.0f)
    {
        ImGui::CalcListClipping(ItemsCount, ItemsHeight, &DisplayStart, &DisplayEnd); // calculate how many to clip/display
        if (DisplayStart > 0)
            SetCursorPosYAndSetupDummyPrevLine(StartPosY + DisplayStart * ItemsHeight, ItemsHeight); // advance cursor
        StepNo = 2;
    }
}

void ImGuiListClipper::End()
{
    if (ItemsCount < 0)
        return;
    // In theory here we should assert that ImGui::GetCursorPosY() == StartPosY + DisplayEnd * ItemsHeight, but it feels saner to just seek at the end and not assert/crash the user.
    if (ItemsCount < INT_MAX)
        SetCursorPosYAndSetupDummyPrevLine(StartPosY + ItemsCount * ItemsHeight, ItemsHeight); // advance cursor
    ItemsCount = -1;
    StepNo = 3;
}

bool ImGuiListClipper::Step()
{
    if (ItemsCount == 0 || ImGui::GetCurrentWindowRead()->SkipItems)
    {
        ItemsCount = -1;
        return false;
    }
    if (StepNo == 0) // Step 0: the clipper let you process the first element, regardless of it being visible or not, so we can measure the element height.
    {
        DisplayStart = 0;
        DisplayEnd = 1;
        StartPosY = ImGui::GetCursorPosY();
        StepNo = 1;
        return true;
    }
    if (StepNo == 1) // Step 1: the clipper infer height from first element, calculate the actual range of elements to display, and position the cursor before the first element.
    {
        if (ItemsCount == 1) { ItemsCount = -1; return false; }
        float items_height = ImGui::GetCursorPosY() - StartPosY;
        IM_ASSERT(items_height > 0.0f);   // If this triggers, it means Item 0 hasn't moved the cursor vertically
        Begin(ItemsCount-1, items_height);
        DisplayStart++;
        DisplayEnd++;
        StepNo = 3;
        return true;
    }
    if (StepNo == 2) // Step 2: dummy step only required if an explicit items_height was passed to constructor or Begin() and user still call Step(). Does nothing and switch to Step 3.
    {
        IM_ASSERT(DisplayStart >= 0 && DisplayEnd >= 0);
        StepNo = 3;
        return true;
    }
    if (StepNo == 3) // Step 3: the clipper validate that we have reached the expected Y position (corresponding to element DisplayEnd), advance the cursor to the end of the list and then returns 'false' to end the loop.
        End();
    return false;
}

//-----------------------------------------------------------------------------
// [SECTION] RENDER HELPERS
// Those (internal) functions are currently quite a legacy mess - their signature and behavior will change.
// Also see imgui_draw.cpp for some more which have been reworked to not rely on ImGui:: state.
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

// Render a rectangle shaped with optional rounding and borders
void ImGui::RenderFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border, float rounding)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DrawList->AddRectFilled(p_min, p_max, fill_col, rounding);
    const float border_size = g.Style.FrameBorderSize;
    if (border && border_size > 0.0f)
    {
        window->DrawList->AddRect(p_min+ImVec2(1,1), p_max+ImVec2(1,1), GetColorU32(ImGuiCol_BorderShadow), rounding, ImDrawCornerFlags_All, border_size);
        window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, ImDrawCornerFlags_All, border_size);
    }
}

void ImGui::RenderFrameBorder(ImVec2 p_min, ImVec2 p_max, float rounding)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const float border_size = g.Style.FrameBorderSize;
    if (border_size > 0.0f)
    {
        window->DrawList->AddRect(p_min+ImVec2(1,1), p_max+ImVec2(1,1), GetColorU32(ImGuiCol_BorderShadow), rounding, ImDrawCornerFlags_All, border_size);
        window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, ImDrawCornerFlags_All, border_size);
    }
}

// Render an arrow aimed to be aligned with text (p_min is a position in the same space text would be positioned). To e.g. denote expanded/collapsed state
void ImGui::RenderArrow(ImVec2 p_min, ImGuiDir dir, float scale)
{
    ImGuiContext& g = *GImGui;

    const float h = g.FontSize * 1.00f;
    float r = h * 0.40f * scale;
    ImVec2 center = p_min + ImVec2(h * 0.50f, h * 0.50f * scale);

    ImVec2 a, b, c;
    switch (dir)
    {
    case ImGuiDir_Up:
    case ImGuiDir_Down:
        if (dir == ImGuiDir_Up) r = -r;
        a = ImVec2(+0.000f,+0.750f) * r;
        b = ImVec2(-0.866f,-0.750f) * r;
        c = ImVec2(+0.866f,-0.750f) * r;
        break;
    case ImGuiDir_Left:
    case ImGuiDir_Right:
        if (dir == ImGuiDir_Left) r = -r;
        a = ImVec2(+0.750f,+0.000f) * r;
        b = ImVec2(-0.750f,+0.866f) * r;
        c = ImVec2(-0.750f,-0.866f) * r;
        break;
    case ImGuiDir_None:
    case ImGuiDir_COUNT:
        IM_ASSERT(0);
        break;
    }

    g.CurrentWindow->DrawList->AddTriangleFilled(center + a, center + b, center + c, GetColorU32(ImGuiCol_Text));
}

void ImGui::RenderBullet(ImVec2 pos)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DrawList->AddCircleFilled(pos, g.FontSize*0.20f, GetColorU32(ImGuiCol_Text), 8);
}

void ImGui::RenderCheckMark(ImVec2 pos, ImU32 col, float sz)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    float thickness = ImMax(sz / 5.0f, 1.0f);
    sz -= thickness*0.5f;
    pos += ImVec2(thickness*0.25f, thickness*0.25f);

    float third = sz / 3.0f;
    float bx = pos.x + third;
    float by = pos.y + sz - third*0.5f;
    window->DrawList->PathLineTo(ImVec2(bx - third, by - third));
    window->DrawList->PathLineTo(ImVec2(bx, by));
    window->DrawList->PathLineTo(ImVec2(bx + third*2, by - third*2));
    window->DrawList->PathStroke(col, false, thickness);
}

void ImGui::RenderNavHighlight(const ImRect& bb, ImGuiID id, ImGuiNavHighlightFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (id != g.NavId)
        return;
    if (g.NavDisableHighlight && !(flags & ImGuiNavHighlightFlags_AlwaysDraw))
        return;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->DC.NavHideHighlightOneFrame)
        return;

    float rounding = (flags & ImGuiNavHighlightFlags_NoRounding) ? 0.0f : g.Style.FrameRounding;
    ImRect display_rect = bb;
    display_rect.ClipWith(window->ClipRect);
    if (flags & ImGuiNavHighlightFlags_TypeDefault)
    {
        const float THICKNESS = 2.0f;
        const float DISTANCE = 3.0f + THICKNESS * 0.5f;
        display_rect.Expand(ImVec2(DISTANCE,DISTANCE));
        bool fully_visible = window->ClipRect.Contains(display_rect);
        if (!fully_visible)
            window->DrawList->PushClipRect(display_rect.Min, display_rect.Max);
        window->DrawList->AddRect(display_rect.Min + ImVec2(THICKNESS*0.5f,THICKNESS*0.5f), display_rect.Max - ImVec2(THICKNESS*0.5f,THICKNESS*0.5f), GetColorU32(ImGuiCol_NavHighlight), rounding, ImDrawCornerFlags_All, THICKNESS);
        if (!fully_visible)
            window->DrawList->PopClipRect();
    }
    if (flags & ImGuiNavHighlightFlags_TypeThin)
    {
        window->DrawList->AddRect(display_rect.Min, display_rect.Max, GetColorU32(ImGuiCol_NavHighlight), rounding, ~0, 1.0f);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] MAIN CODE (most of the code! lots of stuff, needs tidying up!)
//-----------------------------------------------------------------------------

// ImGuiWindow is mostly a dumb struct. It merely has a constructor and a few helper methods
ImGuiWindow::ImGuiWindow(ImGuiContext* context, const char* name)
    : DrawListInst(&context->DrawListSharedData)
{
    Name = ImStrdup(name);
    ID = ImHash(name, 0);
    IDStack.push_back(ID);
    Flags = FlagsPreviousFrame = ImGuiWindowFlags_None;
    Viewport = NULL;
    ViewportId = 0;
    ViewportAllowPlatformMonitorExtend = -1;
    ViewportPos = ImVec2(FLT_MAX, FLT_MAX);
    Pos = ImVec2(0.0f, 0.0f);
    Size = SizeFull = ImVec2(0.0f, 0.0f);
    SizeContents = SizeContentsExplicit = ImVec2(0.0f, 0.0f);
    WindowPadding = ImVec2(0.0f, 0.0f);
    WindowRounding = 0.0f;
    WindowBorderSize = 0.0f;
    NameBufLen = (int)strlen(name) + 1;
    MoveId = GetID("#MOVE");
    ChildId = 0;
    Scroll = ImVec2(0.0f, 0.0f);
    ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);
    ScrollTargetCenterRatio = ImVec2(0.5f, 0.5f);
    ScrollbarSizes = ImVec2(0.0f, 0.0f);
    ScrollbarX = ScrollbarY = false;
    ViewportOwned = false;
    Active = WasActive = false;
    WriteAccessed = false;
    Collapsed = false;
    WantCollapseToggle = false;
    SkipItems = false;
    Appearing = false;
    Hidden = false;
    HasCloseButton = false;
    BeginCount = 0;
    BeginOrderWithinParent = -1;
    BeginOrderWithinContext = -1;
    PopupId = 0;
    AutoFitFramesX = AutoFitFramesY = -1;
    AutoFitOnlyGrows = false;
    AutoFitChildAxises = 0x00;
    AutoPosLastDirection = ImGuiDir_None;
    HiddenFramesRegular = HiddenFramesForResize = 0;
    SetWindowPosAllowFlags = SetWindowSizeAllowFlags = SetWindowCollapsedAllowFlags = SetWindowDockAllowFlags = ImGuiCond_Always | ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing;
    SetWindowPosVal = SetWindowPosPivot = ImVec2(FLT_MAX, FLT_MAX);

    LastFrameActive = -1;
    ItemWidthDefault = 0.0f;
    FontWindowScale = FontDpiScale = 1.0f;
    SettingsIdx = -1;

    DrawList = &DrawListInst;
    DrawList->_OwnerName = Name;
    ParentWindow = NULL;
    RootWindow = NULL;
    RootWindowDockStop = NULL;
    RootWindowForTitleBarHighlight = NULL;
    RootWindowForNav = NULL;

    NavLastIds[0] = NavLastIds[1] = 0;
    NavRectRel[0] = NavRectRel[1] = ImRect();
    NavLastChildNavWindow = NULL;

    FocusIdxAllCounter = FocusIdxTabCounter = -1;
    FocusIdxAllRequestCurrent = FocusIdxTabRequestCurrent = INT_MAX;
    FocusIdxAllRequestNext = FocusIdxTabRequestNext = INT_MAX;

    DockNode = DockNodeAsHost = NULL;
    DockId = 0;
    DockTabItemStatusFlags = 0;
    DockOrder = -1;
    DockIsActive = DockTabIsVisible = DockTabWantClose = false;
}

ImGuiWindow::~ImGuiWindow()
{
    IM_ASSERT(DrawList == &DrawListInst);
    IM_DELETE(Name);
    for (int i = 0; i != ColumnsStorage.Size; i++)
        ColumnsStorage[i].~ImGuiColumnsSet();
}

ImGuiID ImGuiWindow::GetID(const char* str, const char* str_end)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHash(str, str_end ? (int)(str_end - str) : 0, seed);
    ImGui::KeepAliveID(id);
    return id;
}

ImGuiID ImGuiWindow::GetID(const void* ptr)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHash(&ptr, sizeof(void*), seed);
    ImGui::KeepAliveID(id);
    return id;
}

ImGuiID ImGuiWindow::GetIDNoKeepAlive(const char* str, const char* str_end)
{
    ImGuiID seed = IDStack.back();
    return ImHash(str, str_end ? (int)(str_end - str) : 0, seed);
}

ImGuiID ImGuiWindow::GetIDNoKeepAlive(const void* ptr)
{
    ImGuiID seed = IDStack.back();
    return ImHash(&ptr, sizeof(void*), seed);
}

// This is only used in rare/specific situations to manufacture an ID out of nowhere.
ImGuiID ImGuiWindow::GetIDFromRectangle(const ImRect& r_abs)
{
    ImGuiID seed = IDStack.back();
    const int r_rel[4] = { (int)(r_abs.Min.x - Pos.x), (int)(r_abs.Min.y - Pos.y), (int)(r_abs.Max.x - Pos.x), (int)(r_abs.Max.y - Pos.y) };
    ImGuiID id = ImHash(&r_rel, sizeof(r_rel), seed);
    ImGui::KeepAliveID(id);
    return id;
}

static void SetCurrentWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.CurrentWindow = window;
    if (window)
        g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize();
}

void ImGui::SetNavID(ImGuiID id, int nav_layer)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindow);
    IM_ASSERT(nav_layer == 0 || nav_layer == 1);
    g.NavId = id;
    g.NavWindow->NavLastIds[nav_layer] = id;
}

void ImGui::SetNavIDWithRectRel(ImGuiID id, int nav_layer, const ImRect& rect_rel)
{
    ImGuiContext& g = *GImGui;
    SetNavID(id, nav_layer);
    g.NavWindow->NavRectRel[nav_layer] = rect_rel;
    g.NavMousePosDirty = true;
    g.NavDisableHighlight = false;
    g.NavDisableMouseHover = true;
}

void ImGui::SetActiveID(ImGuiID id, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.ActiveIdIsJustActivated = (g.ActiveId != id);
    if (g.ActiveIdIsJustActivated)
    {
        g.ActiveIdTimer = 0.0f;
        g.ActiveIdHasBeenEdited = false;
        if (id != 0)
        {
            g.LastActiveId = id;
            g.LastActiveIdTimer = 0.0f;
        }
    }
    g.ActiveId = id;
    g.ActiveIdAllowNavDirFlags = 0;
    g.ActiveIdAllowOverlap = false;
    g.ActiveIdWindow = window;
    if (id)
    {
        g.ActiveIdIsAlive = id;
        g.ActiveIdSource = (g.NavActivateId == id || g.NavInputId == id || g.NavJustTabbedId == id || g.NavJustMovedToId == id) ? ImGuiInputSource_Nav : ImGuiInputSource_Mouse;
    }
}

// FIXME-NAV: The existence of SetNavID/SetNavIDWithRectRel/SetFocusID is incredibly messy and confusing and needs some explanation or refactoring.
void ImGui::SetFocusID(ImGuiID id, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(id != 0);

    // Assume that SetFocusID() is called in the context where its NavLayer is the current layer, which is the case everywhere we call it.
    const ImGuiNavLayer nav_layer = window->DC.NavLayerCurrent;
    if (g.NavWindow != window)
        g.NavInitRequest = false;
    g.NavId = id;
    g.NavWindow = window;
    g.NavLayer = nav_layer;
    window->NavLastIds[nav_layer] = id;
    if (window->DC.LastItemId == id)
        window->NavRectRel[nav_layer] = ImRect(window->DC.LastItemRect.Min - window->Pos, window->DC.LastItemRect.Max - window->Pos);

    if (g.ActiveIdSource == ImGuiInputSource_Nav)
        g.NavDisableMouseHover = true;
    else
        g.NavDisableHighlight = true;
}

void ImGui::ClearActiveID()
{
    SetActiveID(0, NULL);
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

void ImGui::KeepAliveID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId == id)
        g.ActiveIdIsAlive = id;
    if (g.ActiveIdPreviousFrame == id)
        g.ActiveIdPreviousFrameIsAlive = true;
}

void ImGui::MarkItemEdited(ImGuiID id)
{
    // This marking is solely to be able to provide info for IsItemDeactivatedAfterEdit().
    // ActiveId might have been released by the time we call this (as in the typical press/release button behavior) but still need need to fill the data.
    (void)id; // Avoid unused variable warnings when asserts are compiled out.
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ActiveId == id || g.ActiveId == 0 || g.DragDropActive);
    //IM_ASSERT(g.CurrentWindow->DC.LastItemId == id);
    g.ActiveIdHasBeenEdited = true;
    g.CurrentWindow->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_Edited;
}

static inline bool IsWindowContentHoverable(ImGuiWindow* window, ImGuiHoveredFlags flags)
{
    // An active popup disable hovering on other windows (apart from its own children)
    // FIXME-OPT: This could be cached/stored within the window.
    ImGuiContext& g = *GImGui;
    if (g.NavWindow)
        if (ImGuiWindow* focused_root_window = g.NavWindow->RootWindow)
            if (focused_root_window->WasActive && focused_root_window != window->RootWindow)
            {
                // For the purpose of those flags we differentiate "standard popup" from "modal popup"
                // NB: The order of those two tests is important because Modal windows are also Popups.
                if (focused_root_window->Flags & ImGuiWindowFlags_Modal)
                    return false;
                if ((focused_root_window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                    return false;
            }

    // Filter by viewport
    if (window->Viewport != g.MouseViewport)
        if (g.MovingWindow == NULL || window->RootWindow != g.MovingWindow->RootWindow)
            return false;

    return true;
}

// Advance cursor given item size for layout.
void ImGui::ItemSize(const ImVec2& size, float text_offset_y)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    // Always align ourselves on pixel boundaries
    const float line_height = ImMax(window->DC.CurrentLineSize.y, size.y);
    const float text_base_offset = ImMax(window->DC.CurrentLineTextBaseOffset, text_offset_y);
    //if (g.IO.KeyAlt) window->DrawList->AddRect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(size.x, line_height), IM_COL32(255,0,0,200)); // [DEBUG]
    window->DC.CursorPosPrevLine = ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y);
    window->DC.CursorPos.x = (float)(int)(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
    window->DC.CursorPos.y = (float)(int)(window->DC.CursorPos.y + line_height + g.Style.ItemSpacing.y);
    window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPosPrevLine.x);
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, window->DC.CursorPos.y - g.Style.ItemSpacing.y);
    //if (g.IO.KeyAlt) window->DrawList->AddCircle(window->DC.CursorMaxPos, 3.0f, IM_COL32(255,0,0,255), 4); // [DEBUG]

    window->DC.PrevLineSize.y = line_height;
    window->DC.PrevLineTextBaseOffset = text_base_offset;
    window->DC.CurrentLineSize.y = window->DC.CurrentLineTextBaseOffset = 0.0f;

    // Horizontal layout mode
    if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
        SameLine();
}

void ImGui::ItemSize(const ImRect& bb, float text_offset_y)
{
    ItemSize(bb.GetSize(), text_offset_y);
}

// Declare item bounding box for clipping and interaction.
// Note that the size can be different than the one provided to ItemSize(). Typically, widgets that spread over available surface
// declare their minimum size requirement to ItemSize() and then use a larger region for drawing/interaction, which is passed to ItemAdd().
bool ImGui::ItemAdd(const ImRect& bb, ImGuiID id, const ImRect* nav_bb_arg)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (id != 0)
    {
        // Navigation processing runs prior to clipping early-out
        //  (a) So that NavInitRequest can be honored, for newly opened windows to select a default widget
        //  (b) So that we can scroll up/down past clipped items. This adds a small O(N) cost to regular navigation requests unfortunately, but it is still limited to one window.
        //      it may not scale very well for windows with ten of thousands of item, but at least NavMoveRequest is only set on user interaction, aka maximum once a frame.
        //      We could early out with "if (is_clipped && !g.NavInitRequest) return false;" but when we wouldn't be able to reach unclipped widgets. This would work if user had explicit scrolling control (e.g. mapped on a stick)
        window->DC.NavLayerActiveMaskNext |= window->DC.NavLayerCurrentMask;
        if (g.NavId == id || g.NavAnyRequest)
            if (g.NavWindow->RootWindowForNav == window->RootWindowForNav)
                if (window == g.NavWindow || ((window->Flags | g.NavWindow->Flags) & ImGuiWindowFlags_NavFlattened))
                    NavProcessItem(window, nav_bb_arg ? *nav_bb_arg : bb, id);
    }

    window->DC.LastItemId = id;
    window->DC.LastItemRect = bb;
    window->DC.LastItemStatusFlags = ImGuiItemStatusFlags_None;

#ifdef IMGUI_ENABLE_TEST_ENGINE
    if (id != 0)
        ImGuiTestEngineHook_ItemAdd(&g, bb, id);
#endif

    // Clipping test
    const bool is_clipped = IsClippedEx(bb, id, false);
    if (is_clipped)
        return false;
    //if (g.IO.KeyAlt) window->DrawList->AddRect(bb.Min, bb.Max, IM_COL32(255,255,0,120)); // [DEBUG]

    // We need to calculate this now to take account of the current clipping rectangle (as items like Selectable may change them)
    if (IsMouseHoveringRect(bb.Min, bb.Max))
        window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HoveredRect;
    return true;
}

// This is roughly matching the behavior of internal-facing ItemHoverable()
// - we allow hovering to be true when ActiveId==window->MoveID, so that clicking on non-interactive items such as a Text() item still returns true with IsItemHovered()
// - this should work even for non-interactive items that have no ID, so we cannot use LastItemId
bool ImGui::IsItemHovered(ImGuiHoveredFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.NavDisableMouseHover && !g.NavDisableHighlight)
        return IsItemFocused();

    // Test for bounding box overlap, as updated as ItemAdd()
    if (!(window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect))
        return false;
    IM_ASSERT((flags & (ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows)) == 0);   // Flags not supported by this function

    // Test if we are hovering the right window (our window could be behind another window)
    // [2017/10/16] Reverted commit 344d48be3 and testing RootWindow instead. I believe it is correct to NOT test for RootWindow but this leaves us unable to use IsItemHovered() after EndChild() itself.
    // Until a solution is found I believe reverting to the test from 2017/09/27 is safe since this was the test that has been running for a long while.
    //if (g.HoveredWindow != window)
    //    return false;
    if (g.HoveredRootWindow != window->RootWindow && !(flags & ImGuiHoveredFlags_AllowWhenOverlapped))
        return false;

    // Test if another item is active (e.g. being dragged)
    if (!(flags & ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        if (g.ActiveId != 0 && g.ActiveId != window->DC.LastItemId && !g.ActiveIdAllowOverlap && g.ActiveId != window->MoveId)
            return false;

    // Test if interactions on this window are blocked by an active popup or modal
    if (!IsWindowContentHoverable(window, flags))
        return false;

    // Test if the item is disabled
    if ((window->DC.ItemFlags & ImGuiItemFlags_Disabled) && !(flags & ImGuiHoveredFlags_AllowWhenDisabled))
        return false;

    // Special handling for the dummy item after Begin() which represent the title bar or tab. 
    // When the window is collapsed (SkipItems==true) that last item will never be overwritten so we need to detect the case.
    if ((window->DC.LastItemId == window->ID || window->DC.LastItemId == window->MoveId) && window->WriteAccessed)
        return false;
    return true;
}

// Internal facing ItemHoverable() used when submitting widgets. Differs slightly from IsItemHovered().
bool ImGui::ItemHoverable(const ImRect& bb, ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (g.HoveredId != 0 && g.HoveredId != id && !g.HoveredIdAllowOverlap)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    if (g.HoveredWindow != window)
        return false;
    if (g.ActiveId != 0 && g.ActiveId != id && !g.ActiveIdAllowOverlap)
        return false;
    if (!IsMouseHoveringRect(bb.Min, bb.Max))
        return false;
    if (g.NavDisableMouseHover || !IsWindowContentHoverable(window, ImGuiHoveredFlags_None))
        return false;
    if (window->DC.ItemFlags & ImGuiItemFlags_Disabled)
        return false;

    SetHoveredID(id);
    return true;
}

bool ImGui::IsClippedEx(const ImRect& bb, ImGuiID id, bool clip_even_when_logged)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!bb.Overlaps(window->ClipRect))
        if (id == 0 || id != g.ActiveId)
            if (clip_even_when_logged || !g.LogEnabled)
                return true;
    return false;
}

bool ImGui::FocusableItemRegister(ImGuiWindow* window, ImGuiID id, bool tab_stop)
{
    ImGuiContext& g = *GImGui;

    const bool is_tab_stop = (window->DC.ItemFlags & (ImGuiItemFlags_NoTabStop | ImGuiItemFlags_Disabled)) == 0;
    window->FocusIdxAllCounter++;
    if (is_tab_stop)
        window->FocusIdxTabCounter++;

    // Process keyboard input at this point: TAB/Shift-TAB to tab out of the currently focused item.
    // Note that we can always TAB out of a widget that doesn't allow tabbing in.
    if (tab_stop && (g.ActiveId == id) && window->FocusIdxAllRequestNext == INT_MAX && window->FocusIdxTabRequestNext == INT_MAX && !g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_Tab))
        window->FocusIdxTabRequestNext = window->FocusIdxTabCounter + (g.IO.KeyShift ? (is_tab_stop ? -1 : 0) : +1); // Modulo on index will be applied at the end of frame once we've got the total counter of items.

    if (window->FocusIdxAllCounter == window->FocusIdxAllRequestCurrent)
        return true;
    if (is_tab_stop && window->FocusIdxTabCounter == window->FocusIdxTabRequestCurrent)
    {
        g.NavJustTabbedId = id;
        return true;
    }

    return false;
}

void ImGui::FocusableItemUnregister(ImGuiWindow* window)
{
    window->FocusIdxAllCounter--;
    window->FocusIdxTabCounter--;
}

ImVec2 ImGui::CalcItemSize(ImVec2 size, float default_x, float default_y)
{
    ImGuiContext& g = *GImGui;
    ImVec2 content_max;
    if (size.x < 0.0f || size.y < 0.0f)
        content_max = g.CurrentWindow->Pos + GetContentRegionMax();
    if (size.x <= 0.0f)
        size.x = (size.x == 0.0f) ? default_x : ImMax(content_max.x - g.CurrentWindow->DC.CursorPos.x, 4.0f) + size.x;
    if (size.y <= 0.0f)
        size.y = (size.y == 0.0f) ? default_y : ImMax(content_max.y - g.CurrentWindow->DC.CursorPos.y, 4.0f) + size.y;
    return size;
}

float ImGui::CalcWrapWidthForPos(const ImVec2& pos, float wrap_pos_x)
{
    if (wrap_pos_x < 0.0f)
        return 0.0f;

    ImGuiWindow* window = GetCurrentWindowRead();
    if (wrap_pos_x == 0.0f)
        wrap_pos_x = GetContentRegionMax().x + window->Pos.x;
    else if (wrap_pos_x > 0.0f)
        wrap_pos_x += window->Pos.x - window->Scroll.x; // wrap_pos_x is provided is window local space

    return ImMax(wrap_pos_x - pos.x, 1.0f);
}

void* ImGui::MemAlloc(size_t size)
{
    if (ImGuiContext* ctx = GImGui)
        ctx->IO.MetricsActiveAllocations++;
    return GImAllocatorAllocFunc(size, GImAllocatorUserData);
}

void ImGui::MemFree(void* ptr)
{
    if (ptr) 
        if (ImGuiContext* ctx = GImGui)
            ctx->IO.MetricsActiveAllocations--;
    return GImAllocatorFreeFunc(ptr, GImAllocatorUserData);
}

const char* ImGui::GetClipboardText()
{
    return GImGui->IO.GetClipboardTextFn ? GImGui->IO.GetClipboardTextFn(GImGui->IO.ClipboardUserData) : "";
}

void ImGui::SetClipboardText(const char* text)
{
    if (GImGui->IO.SetClipboardTextFn)
        GImGui->IO.SetClipboardTextFn(GImGui->IO.ClipboardUserData, text);
}

const char* ImGui::GetVersion()
{
    return IMGUI_VERSION;
}

// Internal state access - if you want to share ImGui state between modules (e.g. DLL) or allocate it yourself
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

// Helper function to verify that the type sizes are matching between the calling file's compilation unit and imgui.cpp's compilation unit
// If the user has inconsistent compilation settings, imgui configuration #define, packing pragma, etc. you may see different structures from what imgui.cpp sees which is highly problematic.
bool ImGui::DebugCheckVersionAndDataLayout(const char* version, size_t sz_io, size_t sz_style, size_t sz_vec2, size_t sz_vec4, size_t sz_vert)
{
    bool error = false;
    if (strcmp(version, IMGUI_VERSION)!=0) { error = true; IM_ASSERT(strcmp(version,IMGUI_VERSION)==0 && "Mismatched version string!");  }
    if (sz_io    != sizeof(ImGuiIO))       { error = true; IM_ASSERT(sz_io    == sizeof(ImGuiIO)      && "Mismatched struct layout!"); }
    if (sz_style != sizeof(ImGuiStyle))    { error = true; IM_ASSERT(sz_style == sizeof(ImGuiStyle)   && "Mismatched struct layout!"); }
    if (sz_vec2  != sizeof(ImVec2))        { error = true; IM_ASSERT(sz_vec2  == sizeof(ImVec2)       && "Mismatched struct layout!"); }
    if (sz_vec4  != sizeof(ImVec4))        { error = true; IM_ASSERT(sz_vec4  == sizeof(ImVec4)       && "Mismatched struct layout!"); }
    if (sz_vert  != sizeof(ImDrawVert))    { error = true; IM_ASSERT(sz_vert  == sizeof(ImDrawVert)   && "Mismatched struct layout!"); }
    return !error;
}

void ImGui::SetAllocatorFunctions(void* (*alloc_func)(size_t sz, void* user_data), void(*free_func)(void* ptr, void* user_data), void* user_data)
{
    GImAllocatorAllocFunc = alloc_func;
    GImAllocatorFreeFunc = free_func;
    GImAllocatorUserData = user_data;
}

ImGuiContext* ImGui::CreateContext(ImFontAtlas* shared_font_atlas)
{
    ImGuiContext* ctx = IM_NEW(ImGuiContext)(shared_font_atlas);
    if (GImGui == NULL)
        SetCurrentContext(ctx);
    Initialize(ctx);
    return ctx;
}

void ImGui::DestroyContext(ImGuiContext* ctx)
{
    if (ctx == NULL)
        ctx = GImGui;
    Shutdown(ctx);
    if (GImGui == ctx)
        SetCurrentContext(NULL);
    IM_DELETE(ctx);
}

ImGuiIO& ImGui::GetIO()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() or ImGui::SetCurrentContext()?");
    return GImGui->IO;
}

ImGuiPlatformIO& ImGui::GetPlatformIO()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() or ImGui::SetCurrentContext()?");
    return GImGui->PlatformIO;
}

ImGuiStyle& ImGui::GetStyle()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() or ImGui::SetCurrentContext()?");
    return GImGui->Style;
}

// Same value as passed to the old io.RenderDrawListsFn function. Valid after Render() and until the next call to NewFrame()
ImDrawData* ImGui::GetDrawData()
{
    ImGuiContext& g = *GImGui;
    return g.Viewports[0]->DrawDataP.Valid ? &g.Viewports[0]->DrawDataP : NULL;
}

double ImGui::GetTime()
{
    return GImGui->Time;
}

int ImGui::GetFrameCount()
{
    return GImGui->FrameCount;
}

ImDrawList* ImGui::GetOverlayDrawList(ImGuiViewport* viewport_public)
{
    // Create the draw list on demand, because it is not frequently used for all viewports
    ImGuiContext& g = *GImGui;
    ImGuiViewportP* viewport = (ImGuiViewportP*)viewport_public;
    if (viewport->OverlayDrawList == NULL)
    {
        viewport->OverlayDrawList = IM_NEW(ImDrawList)(&g.DrawListSharedData);
        viewport->OverlayDrawList->_OwnerName = "##Overlay";
    }

    // Our ImDrawList system requires that there is always a command
    if (viewport->LastFrameOverlayDrawList != g.FrameCount)
    {
        viewport->OverlayDrawList->Clear();
        viewport->OverlayDrawList->PushTextureID(g.IO.Fonts->TexID);
        viewport->OverlayDrawList->PushClipRect(viewport->Pos, viewport->Pos + viewport->Size, false);
        viewport->OverlayDrawList->Flags = (g.Style.AntiAliasedLines ? ImDrawListFlags_AntiAliasedLines : 0) | (g.Style.AntiAliasedFill ? ImDrawListFlags_AntiAliasedFill : 0);
        viewport->LastFrameOverlayDrawList = g.FrameCount;
    }
    return viewport->OverlayDrawList;
}

ImDrawList* ImGui::GetOverlayDrawList()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return GetOverlayDrawList(window->Viewport);
}

ImDrawListSharedData* ImGui::GetDrawListSharedData()
{
    return &GImGui->DrawListSharedData;
}

void ImGui::StartMouseMovingWindow(ImGuiWindow* window)
{
    // Set ActiveId even if the _NoMove flag is set. Without it, dragging away from a window with _NoMove would activate hover on other windows.
    // We _also_ call this when clicking in a window empty space when io.ConfigWindowsMoveFromTitleBarOnly is set, but clear g.MovingWindow afterward.
    // This is because we want ActiveId to be set even when the window is stuck from moving.
    ImGuiContext& g = *GImGui;
    FocusWindow(window);
    SetActiveID(window->MoveId, window);
    g.NavDisableHighlight = true;
    g.ActiveIdClickOffset = g.IO.MousePos - window->RootWindow->Pos;
    if (!(window->Flags & ImGuiWindowFlags_NoMove) && !(window->RootWindow->Flags & ImGuiWindowFlags_NoMove))
        g.MovingWindow = window;
}

// Handle mouse moving window
// Note: moving window with the navigation keys (Square + d-pad / CTRL+TAB + Arrows) are processed in NavUpdateWindowing()
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
            if (moving_window->Pos.x != pos.x || moving_window->Pos.y != pos.y)
            {
                MarkIniSettingsDirty(moving_window);
                SetWindowPos(moving_window, pos, ImGuiCond_Always);
                if (moving_window->ViewportOwned) // Synchronize viewport immediately because some overlays may relies on clipping rectangle before we Begin() into the window.
                    moving_window->Viewport->Pos = pos;
            }
            FocusWindow(g.MovingWindow);
        }
        else
        {
            // Try to merge the window back into the main viewport. 
            // This works because MouseViewport should be != MovingWindow->Viewport on release (as per code in UpdateViewports)
            if (g.ConfigFlagsForFrame & ImGuiConfigFlags_ViewportsEnable)
                UpdateTryMergeWindowIntoHostViewport(moving_window, g.MouseViewport);

            // Restore the mouse viewport so that we don't hover the viewport _under_ the moved window during the frame we released the mouse button.
            if (!IsDragDropPayloadBeingAccepted())
                g.MouseViewport = moving_window->Viewport;

            // Clear the NoInput window flag set by the Viewport system
            moving_window->Viewport->Flags &= ~ImGuiViewportFlags_NoInputs;

            ClearActiveID();
            g.MovingWindow = NULL;
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

// Initiate moving window, handle left-click and right-click focus
void ImGui::UpdateMouseMovingWindowEndFrame()
{
    // Initiate moving window
    ImGuiContext& g = *GImGui;
    if (g.ActiveId != 0 || g.HoveredId != 0)
        return;

    // Unless we just made a window/popup appear
    if (g.NavWindow && g.NavWindow->Appearing)
        return;

    // Click to focus window and start moving (after we're done with all our widgets)
    if (g.IO.MouseClicked[0])
    {
        if (g.HoveredRootWindow != NULL)
        {
            StartMouseMovingWindow(g.HoveredWindow);
            if (g.IO.ConfigWindowsMoveFromTitleBarOnly && (!(g.HoveredRootWindow->Flags & ImGuiWindowFlags_NoTitleBar) || g.HoveredWindow->RootWindowDockStop->DockIsActive))
                if (!g.HoveredRootWindow->TitleBarRect().Contains(g.IO.MouseClickedPos[0]))
                    g.MovingWindow = NULL;
        }
        else if (g.NavWindow != NULL && GetFrontMostPopupModal() == NULL)
        {
            FocusWindow(NULL);  // Clicking on void disable focus
        }
    }

    // With right mouse button we close popups without changing focus
    // (The left mouse button path calls FocusWindow which will lead NewFrame->ClosePopupsOverWindow to trigger)
    if (g.IO.MouseClicked[1])
    {
        // Find the top-most window between HoveredWindow and the front most Modal Window.
        // This is where we can trim the popup stack.
        ImGuiWindow* modal = GetFrontMostPopupModal();
        bool hovered_window_above_modal = false;
        if (modal == NULL)
            hovered_window_above_modal = true;
        for (int i = g.Windows.Size - 1; i >= 0 && hovered_window_above_modal == false; i--)
        {
            ImGuiWindow* window = g.Windows[i];
            if (window == modal)
                break;
            if (window == g.HoveredWindow)
                hovered_window_above_modal = true;
        }
        ClosePopupsOverWindow(hovered_window_above_modal ? g.HoveredWindow : modal);
    }
}

static void TranslateWindow(ImGuiWindow* window, const ImVec2& delta)
{
    window->Pos += delta;
    window->ClipRect.Translate(delta);
    window->OuterRectClipped.Translate(delta);
    window->InnerMainRect.Translate(delta);
    window->DC.CursorPos += delta;
    window->DC.CursorStartPos += delta;
    window->DC.CursorMaxPos += delta;
    window->DC.LastItemRect.Translate(delta);
    window->DC.LastItemDisplayRect.Translate(delta);
}

static void ScaleWindow(ImGuiWindow* window, float scale)
{
    ImVec2 origin = window->Viewport->Pos;
    window->Pos = ImFloor((window->Pos - origin) * scale + origin);
    window->Size = ImFloor(window->Size * scale);
    window->SizeFull = ImFloor(window->SizeFull * scale);
    window->SizeContents = ImFloor(window->SizeContents * scale);
}

static bool IsWindowActiveAndVisible(ImGuiWindow* window)
{
    return (window->Active) && (!window->Hidden);
}

static void ImGui::UpdateMouseInputs()
{
    ImGuiContext& g = *GImGui;

    // Round mouse position to avoid spreading non-rounded position (e.g. UpdateManualResize doesn't support them well)
    if (IsMousePosValid(&g.IO.MousePos))
        g.IO.MousePos = g.LastValidMousePos = ImFloor(g.IO.MousePos);

    // If mouse just appeared or disappeared (usually denoted by -FLT_MAX components) we cancel out movement in MouseDelta
    if (IsMousePosValid(&g.IO.MousePos) && IsMousePosValid(&g.IO.MousePosPrev))
        g.IO.MouseDelta = g.IO.MousePos - g.IO.MousePosPrev;
    else
        g.IO.MouseDelta = ImVec2(0.0f, 0.0f);
    if (g.IO.MouseDelta.x != 0.0f || g.IO.MouseDelta.y != 0.0f)
        g.NavDisableMouseHover = false;

    g.IO.MousePosPrev = g.IO.MousePos;
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.MouseDown); i++)
    {
        g.IO.MouseClicked[i] = g.IO.MouseDown[i] && g.IO.MouseDownDuration[i] < 0.0f;
        g.IO.MouseReleased[i] = !g.IO.MouseDown[i] && g.IO.MouseDownDuration[i] >= 0.0f;
        g.IO.MouseDownDurationPrev[i] = g.IO.MouseDownDuration[i];
        g.IO.MouseDownDuration[i] = g.IO.MouseDown[i] ? (g.IO.MouseDownDuration[i] < 0.0f ? 0.0f : g.IO.MouseDownDuration[i] + g.IO.DeltaTime) : -1.0f;
        g.IO.MouseDoubleClicked[i] = false;
        if (g.IO.MouseClicked[i])
        {
            if ((float)(g.Time - g.IO.MouseClickedTime[i]) < g.IO.MouseDoubleClickTime)
            {
                ImVec2 delta_from_click_pos = IsMousePosValid(&g.IO.MousePos) ? (g.IO.MousePos - g.IO.MouseClickedPos[i]) : ImVec2(0.0f, 0.0f);
                if (ImLengthSqr(delta_from_click_pos) < g.IO.MouseDoubleClickMaxDist * g.IO.MouseDoubleClickMaxDist)
                    g.IO.MouseDoubleClicked[i] = true;
                g.IO.MouseClickedTime[i] = -FLT_MAX;    // so the third click isn't turned into a double-click
            }
            else
            {
                g.IO.MouseClickedTime[i] = g.Time;
            }
            g.IO.MouseClickedPos[i] = g.IO.MousePos;
            g.IO.MouseDragMaxDistanceAbs[i] = ImVec2(0.0f, 0.0f);
            g.IO.MouseDragMaxDistanceSqr[i] = 0.0f;
        }
        else if (g.IO.MouseDown[i])
        {
            // Maintain the maximum distance we reaching from the initial click position, which is used with dragging threshold
            ImVec2 delta_from_click_pos = IsMousePosValid(&g.IO.MousePos) ? (g.IO.MousePos - g.IO.MouseClickedPos[i]) : ImVec2(0.0f, 0.0f);
            g.IO.MouseDragMaxDistanceSqr[i] = ImMax(g.IO.MouseDragMaxDistanceSqr[i], ImLengthSqr(delta_from_click_pos));
            g.IO.MouseDragMaxDistanceAbs[i].x = ImMax(g.IO.MouseDragMaxDistanceAbs[i].x, delta_from_click_pos.x < 0.0f ? -delta_from_click_pos.x : delta_from_click_pos.x);
            g.IO.MouseDragMaxDistanceAbs[i].y = ImMax(g.IO.MouseDragMaxDistanceAbs[i].y, delta_from_click_pos.y < 0.0f ? -delta_from_click_pos.y : delta_from_click_pos.y);
        }
        if (g.IO.MouseClicked[i]) // Clicking any mouse button reactivate mouse hovering which may have been deactivated by gamepad/keyboard navigation
            g.NavDisableMouseHover = false;
    }
}

void ImGui::UpdateMouseWheel()
{
    ImGuiContext& g = *GImGui;
    if (!g.HoveredWindow || g.HoveredWindow->Collapsed)
        return;
    if (g.IO.MouseWheel == 0.0f && g.IO.MouseWheelH == 0.0f)
        return;

    // If a child window has the ImGuiWindowFlags_NoScrollWithMouse flag, we give a chance to scroll its parent (unless either ImGuiWindowFlags_NoInputs or ImGuiWindowFlags_NoScrollbar are also set).
    ImGuiWindow* window = g.HoveredWindow;
    ImGuiWindow* scroll_window = window;
    while ((scroll_window->Flags & ImGuiWindowFlags_ChildWindow) && (scroll_window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(scroll_window->Flags & ImGuiWindowFlags_NoScrollbar) && !(scroll_window->Flags & ImGuiWindowFlags_NoMouseInputs) && scroll_window->ParentWindow)
        scroll_window = scroll_window->ParentWindow;
    const bool scroll_allowed = !(scroll_window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(scroll_window->Flags & ImGuiWindowFlags_NoMouseInputs);

    if (g.IO.MouseWheel != 0.0f)
    {
        if (g.IO.KeyCtrl && g.IO.FontAllowUserScaling)
        {
            // Zoom / Scale window
            const float new_font_scale = ImClamp(window->FontWindowScale + g.IO.MouseWheel * 0.10f, 0.50f, 2.50f);
            const float scale = new_font_scale / window->FontWindowScale;
            window->FontWindowScale = new_font_scale;

            const ImVec2 offset = window->Size * (1.0f - scale) * (g.IO.MousePos - window->Pos) / window->Size;
            window->Pos += offset;
            window->Size *= scale;
            window->SizeFull *= scale;
        }
        else if (!g.IO.KeyCtrl && scroll_allowed)
        {
            // Mouse wheel vertical scrolling
            float scroll_amount = 5 * scroll_window->CalcFontSize();
            scroll_amount = (float)(int)ImMin(scroll_amount, (scroll_window->ContentsRegionRect.GetHeight() + scroll_window->WindowPadding.y * 2.0f) * 0.67f);
            SetWindowScrollY(scroll_window, scroll_window->Scroll.y - g.IO.MouseWheel * scroll_amount);
        }
    }
    if (g.IO.MouseWheelH != 0.0f && scroll_allowed && !g.IO.KeyCtrl)
    {
        // Mouse wheel horizontal scrolling (for hardware that supports it)
        float scroll_amount = scroll_window->CalcFontSize();
        SetWindowScrollX(scroll_window, scroll_window->Scroll.x - g.IO.MouseWheelH * scroll_amount);
    }
}

// The reason this is exposed in imgui_internal.h is: on touch-based system that don't have hovering, we want to dispatch inputs to the right target (imgui vs imgui+app)
void ImGui::UpdateHoveredWindowAndCaptureFlags()
{
    ImGuiContext& g = *GImGui;

    // Find the window hovered by mouse:
    // - Child windows can extend beyond the limit of their parent so we need to derive HoveredRootWindow from HoveredWindow.
    // - When moving a window we can skip the search, which also conveniently bypasses the fact that window->WindowRectClipped is lagging as this point of the frame.
    // - We also support the moved window toggling the NoInputs flag after moving has started in order to be able to detect windows below it, which is useful for e.g. docking mechanisms.
    FindHoveredWindow();
    IM_ASSERT(g.HoveredWindow == NULL || g.HoveredWindow == g.MovingWindow || g.HoveredWindow->Viewport == g.MouseViewport);

    // Modal windows prevents cursor from hovering behind them.
    ImGuiWindow* modal_window = GetFrontMostPopupModal();
    if (modal_window)
        if (g.HoveredRootWindow && !IsWindowChildOf(g.HoveredRootWindow, modal_window))
            g.HoveredWindow = g.HoveredRootWindow = g.HoveredWindowUnderMovingWindow = NULL;

    // Disabled mouse?
    if (g.IO.ConfigFlags & ImGuiConfigFlags_NoMouse)
        g.HoveredWindow = g.HoveredRootWindow = g.HoveredWindowUnderMovingWindow = NULL;

    // We track click ownership. When clicked outside of a window the click is owned by the application and won't report hovering nor request capture even while dragging over our windows afterward.
    int mouse_earliest_button_down = -1;
    bool mouse_any_down = false;
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.MouseDown); i++)
    {
        if (g.IO.MouseClicked[i])
            g.IO.MouseDownOwned[i] = (g.HoveredWindow != NULL) || (!g.OpenPopupStack.empty());
        mouse_any_down |= g.IO.MouseDown[i];
        if (g.IO.MouseDown[i])
            if (mouse_earliest_button_down == -1 || g.IO.MouseClickedTime[i] < g.IO.MouseClickedTime[mouse_earliest_button_down])
                mouse_earliest_button_down = i;
    }
    const bool mouse_avail_to_imgui = (mouse_earliest_button_down == -1) || g.IO.MouseDownOwned[mouse_earliest_button_down];

    // If mouse was first clicked outside of ImGui bounds we also cancel out hovering.
    // FIXME: For patterns of drag and drop across OS windows, we may need to rework/remove this test (first committed 311c0ca9 on 2015/02)
    const bool mouse_dragging_extern_payload = g.DragDropActive && (g.DragDropSourceFlags & ImGuiDragDropFlags_SourceExtern) != 0;
    if (!mouse_avail_to_imgui && !mouse_dragging_extern_payload)
        g.HoveredWindow = g.HoveredRootWindow = g.HoveredWindowUnderMovingWindow = NULL;

    // Update io.WantCaptureMouse for the user application (true = dispatch mouse info to imgui, false = dispatch mouse info to imgui + app)
    if (g.WantCaptureMouseNextFrame != -1)
        g.IO.WantCaptureMouse = (g.WantCaptureMouseNextFrame != 0);
    else
        g.IO.WantCaptureMouse = (mouse_avail_to_imgui && (g.HoveredWindow != NULL || mouse_any_down)) || (!g.OpenPopupStack.empty());

    // Update io.WantCaptureKeyboard for the user application (true = dispatch keyboard info to imgui, false = dispatch keyboard info to imgui + app)
    if (g.WantCaptureKeyboardNextFrame != -1)
        g.IO.WantCaptureKeyboard = (g.WantCaptureKeyboardNextFrame != 0);
    else
        g.IO.WantCaptureKeyboard = (g.ActiveId != 0) || (modal_window != NULL);
    if (g.IO.NavActive && (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) && !(g.IO.ConfigFlags & ImGuiConfigFlags_NavNoCaptureKeyboard))
        g.IO.WantCaptureKeyboard = true;

    // Update io.WantTextInput flag, this is to allow systems without a keyboard (e.g. mobile, hand-held) to show a software keyboard if possible
    g.IO.WantTextInput = (g.WantTextInputNextFrame != -1) ? (g.WantTextInputNextFrame != 0) : false;
}

void ImGui::NewFrame()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() or ImGui::SetCurrentContext()?");
    ImGuiContext& g = *GImGui;

#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiTestEngineHook_PreNewFrame(&g);
#endif

    // Check user data
    // (We pass an error message in the assert expression to make it visible to programmers who are not using a debugger, as most assert handlers display their argument)
    IM_ASSERT(g.Initialized);
    IM_ASSERT(g.IO.DeltaTime >= 0.0f                                    && "Need a positive DeltaTime (zero is tolerated but will cause some timing issues)");
    IM_ASSERT(g.IO.DisplaySize.x >= 0.0f && g.IO.DisplaySize.y >= 0.0f  && "Invalid DisplaySize value");
    IM_ASSERT(g.IO.Fonts->Fonts.Size > 0                                && "Font Atlas not built. Did you call io.Fonts->GetTexDataAsRGBA32() / GetTexDataAsAlpha8() ?");
    IM_ASSERT(g.IO.Fonts->Fonts[0]->IsLoaded()                          && "Font Atlas not built. Did you call io.Fonts->GetTexDataAsRGBA32() / GetTexDataAsAlpha8() ?");
    IM_ASSERT(g.Style.CurveTessellationTol > 0.0f                       && "Invalid style setting");
    IM_ASSERT(g.Style.Alpha >= 0.0f && g.Style.Alpha <= 1.0f            && "Invalid style setting. Alpha cannot be negative (allows us to avoid a few clamps in color computations)");
    IM_ASSERT((g.FrameCount == 0 || g.FrameCountEnded == g.FrameCount)  && "Forgot to call Render() or EndFrame() at the end of the previous frame?");
    for (int n = 0; n < ImGuiKey_COUNT; n++)
        IM_ASSERT(g.IO.KeyMap[n] >= -1 && g.IO.KeyMap[n] < IM_ARRAYSIZE(g.IO.KeysDown) && "io.KeyMap[] contains an out of bound value (need to be 0..512, or -1 for unmapped key)");

    // Perform simple check: required key mapping (we intentionally do NOT check all keys to not pressure user into setting up everything, but Space is required and was only recently added in 1.60 WIP)
    if (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard)
        IM_ASSERT(g.IO.KeyMap[ImGuiKey_Space] != -1 && "ImGuiKey_Space is not mapped, required for keyboard navigation.");

    // Perform simple check: the beta io.ConfigWindowsResizeFromEdges option requires back-end to honor mouse cursor changes and set the ImGuiBackendFlags_HasMouseCursors flag accordingly.
    if (g.IO.ConfigWindowsResizeFromEdges && !(g.IO.BackendFlags & ImGuiBackendFlags_HasMouseCursors))
        g.IO.ConfigWindowsResizeFromEdges = false;

    // Perform simple checks: multi-viewport and platform windows support
    if (g.IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        if ((g.IO.BackendFlags & ImGuiBackendFlags_PlatformHasViewports) && (g.IO.BackendFlags & ImGuiBackendFlags_RendererHasViewports))
        {
            IM_ASSERT((g.FrameCount == 0 || g.FrameCount == g.FrameCountPlatformEnded) && "Forgot to call UpdatePlatformWindows() in main loop after EndFrame()? Check examples/ applications for reference.");
            IM_ASSERT(g.PlatformIO.Platform_CreateWindow != NULL  && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Platform_DestroyWindow != NULL && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Platform_GetWindowPos != NULL  && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Platform_SetWindowPos != NULL  && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Platform_GetWindowSize != NULL  && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Platform_SetWindowSize != NULL  && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Monitors.Size > 0 && "Platform init didn't setup Monitors list?");
            IM_ASSERT((g.Viewports[0]->PlatformUserData != NULL || g.Viewports[0]->PlatformHandle != NULL) && "Platform init didn't setup main viewport.");
            if (g.IO.ConfigDockingTransparentPayload && (g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
                IM_ASSERT(g.PlatformIO.Platform_SetWindowAlpha != NULL && "Platform_SetWindowAlpha handler is required to use io.ConfigDockingTransparent!");
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
            IM_ASSERT(g.IO.RenderDrawListsFn == NULL);  // Call ImGui::Render() then pass ImGui::GetDrawData() yourself to your render function!
#endif
        }
        else
        {
            // Disable feature, our back-ends do not support it
            g.IO.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        }

        // Perform simple checks on platform monitor data + compute a total bounding box for quick early outs
        for (int monitor_n = 0; monitor_n < g.PlatformIO.Monitors.Size; monitor_n++)
        {
            ImGuiPlatformMonitor& mon = g.PlatformIO.Monitors[monitor_n];
			(void)mon;
            IM_ASSERT(mon.MainSize.x > 0.0f && mon.MainSize.y > 0.0f && "Monitor bounds not setup properly.");
            IM_ASSERT(mon.WorkSize.x > 0.0f && mon.WorkSize.y > 0.0f && "Monitor bounds not setup properly. If you don't have work area information, just copy Min/Max into them.");
            IM_ASSERT(mon.DpiScale != 0.0f);
        }
    }

    // Load settings on first frame (if not explicitly loaded manually before)
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

    g.Time += g.IO.DeltaTime;
    g.FrameScopeActive = true;
    g.FrameCount += 1;
    g.TooltipOverrideCount = 0;
    g.WindowsActiveCount = 0;
    g.ConfigFlagsForFrame = g.IO.ConfigFlags;

    UpdateViewportsNewFrame();

    // Setup current font, and draw list shared data
    // FIXME-VIEWPORT: the concept of a single ClipRectFullscreen is not ideal!
    g.IO.Fonts->Locked = true;
    SetCurrentFont(GetDefaultFont());
    IM_ASSERT(g.Font->IsLoaded());
    ImVec2 virtual_space_max(0,0);
    for (int n = 0; n < g.Viewports.Size; n++)
        virtual_space_max = ImMax(virtual_space_max, g.Viewports[n]->Pos + g.Viewports[n]->Size);
    g.DrawListSharedData.ClipRectFullscreen = ImVec4(0.0f, 0.0f, virtual_space_max.x, virtual_space_max.y);
    g.DrawListSharedData.CurveTessellationTol = g.Style.CurveTessellationTol;

    // Mark rendering data as invalid to prevent user who may have a handle on it to use it. Setup Overlay draw list for the viewport.
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        viewport->DrawData = NULL;
        viewport->DrawDataP.Clear();
    }

    // Drag and drop keep the source ID alive so even if the source disappear our state is consistent
    if (g.DragDropActive && g.DragDropPayload.SourceId == g.ActiveId)
        KeepAliveID(g.DragDropPayload.SourceId);

    // Clear reference to active widget if the widget isn't alive anymore
    if (!g.HoveredIdPreviousFrame)
        g.HoveredIdTimer = 0.0f;
    if (!g.HoveredIdPreviousFrame || (g.HoveredId && g.ActiveId == g.HoveredId))
        g.HoveredIdNotActiveTimer = 0.0f;
    if (g.HoveredId)
        g.HoveredIdTimer += g.IO.DeltaTime;
    if (g.HoveredId && g.ActiveId != g.HoveredId)
        g.HoveredIdNotActiveTimer += g.IO.DeltaTime;
    g.HoveredIdPreviousFrame = g.HoveredId;
    g.HoveredId = 0;
    g.HoveredIdAllowOverlap = false;
    if (g.ActiveIdIsAlive != g.ActiveId && g.ActiveIdPreviousFrame == g.ActiveId && g.ActiveId != 0)
        ClearActiveID();
    if (g.ActiveId)
        g.ActiveIdTimer += g.IO.DeltaTime;
    g.LastActiveIdTimer += g.IO.DeltaTime;
    g.ActiveIdPreviousFrame = g.ActiveId;
    g.ActiveIdPreviousFrameWindow = g.ActiveIdWindow;
    g.ActiveIdPreviousFrameHasBeenEdited = g.ActiveIdHasBeenEdited;
    g.ActiveIdIsAlive = 0;
    g.ActiveIdPreviousFrameIsAlive = false;
    g.ActiveIdIsJustActivated = false;
    if (g.ScalarAsInputTextId && g.ActiveId != g.ScalarAsInputTextId)
        g.ScalarAsInputTextId = 0;

    // Drag and drop
    g.DragDropAcceptIdPrev = g.DragDropAcceptIdCurr;
    g.DragDropAcceptIdCurr = 0;
    g.DragDropAcceptIdCurrRectSurface = FLT_MAX;
    g.DragDropWithinSourceOrTarget = false;

    // Update keyboard input state
    memcpy(g.IO.KeysDownDurationPrev, g.IO.KeysDownDuration, sizeof(g.IO.KeysDownDuration));
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.KeysDown); i++)
        g.IO.KeysDownDuration[i] = g.IO.KeysDown[i] ? (g.IO.KeysDownDuration[i] < 0.0f ? 0.0f : g.IO.KeysDownDuration[i] + g.IO.DeltaTime) : -1.0f;

    // Update gamepad/keyboard directional navigation
    NavUpdate();

    // Update mouse input state
    UpdateMouseInputs();

    // Calculate frame-rate for the user, as a purely luxurious feature
    g.FramerateSecPerFrameAccum += g.IO.DeltaTime - g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx];
    g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx] = g.IO.DeltaTime;
    g.FramerateSecPerFrameIdx = (g.FramerateSecPerFrameIdx + 1) % IM_ARRAYSIZE(g.FramerateSecPerFrame);
    g.IO.Framerate = (g.FramerateSecPerFrameAccum > 0.0f) ? (1.0f / (g.FramerateSecPerFrameAccum / (float)IM_ARRAYSIZE(g.FramerateSecPerFrame))) : FLT_MAX;

    // Undocking
    // (needs to be before UpdateMouseMovingWindow so the window is already offset and following the mouse on the detaching frame)
    DockContextNewFrameUpdateUndocking(&g);

    // Find hovered window
    // (needs to be before UpdateMouseMovingWindow() so we fill g.HoveredWindowUnderMovingWindow on the mouse release frame)
    UpdateHoveredWindowAndCaptureFlags();

    // Handle user moving window with mouse (at the beginning of the frame to avoid input lag or sheering)
    UpdateMouseMovingWindowNewFrame();

    // Background darkening/whitening
    if (GetFrontMostPopupModal() != NULL || (g.NavWindowingTarget != NULL && g.NavWindowingHighlightAlpha > 0.0f))
        g.DimBgRatio = ImMin(g.DimBgRatio + g.IO.DeltaTime * 6.0f, 1.0f);
    else
        g.DimBgRatio = ImMax(g.DimBgRatio - g.IO.DeltaTime * 10.0f, 0.0f);

    g.MouseCursor = ImGuiMouseCursor_Arrow;
    g.WantCaptureMouseNextFrame = g.WantCaptureKeyboardNextFrame = g.WantTextInputNextFrame = -1;
    g.PlatformImePos = ImVec2(1.0f, 1.0f); // OS Input Method Editor showing on top-left of our window by default
    g.PlatformImePosViewport = NULL;

    // Mouse wheel scrolling, scale
    UpdateMouseWheel();

    // Pressing TAB activate widget focus
    if (g.ActiveId == 0 && g.NavWindow != NULL && g.NavWindow->Active && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs) && !g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_Tab, false))
    {
        if (g.NavId != 0 && g.NavIdTabCounter != INT_MAX)
            g.NavWindow->FocusIdxTabRequestNext = g.NavIdTabCounter + 1 + (g.IO.KeyShift ? -1 : 1);
        else
            g.NavWindow->FocusIdxTabRequestNext = g.IO.KeyShift ? -1 : 0;
    }
    g.NavIdTabCounter = INT_MAX;

    // Mark all windows as not visible
    IM_ASSERT(g.WindowsFocusOrder.Size == g.Windows.Size);
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        window->WasActive = window->Active;
        window->BeginCount = 0;
        window->Active = false;
        window->WriteAccessed = false;
    }

    // Closing the focused window restore focus to the first active root window in descending z-order
    if (g.NavWindow && !g.NavWindow->WasActive)
        FocusPreviousWindowIgnoringOne(NULL);

    // No window should be open at the beginning of the frame.
    // But in order to allow the user to call NewFrame() multiple times without calling Render(), we are doing an explicit clear.
    g.CurrentWindowStack.resize(0);
    g.BeginPopupStack.resize(0);
    ClosePopupsOverWindow(g.NavWindow);

    // Docking
    DockContextNewFrameUpdateDocking(&g);

    // Create implicit/fallback window - which we will only render it if the user has added something to it.
    // We don't use "Debug" to avoid colliding with user trying to create a "Debug" window with custom flags.
    // This fallback is particularly important as it avoid ImGui:: calls from crashing.
    SetNextWindowSize(ImVec2(400,400), ImGuiCond_FirstUseEver);
    Begin("Debug##Default");
    g.FrameScopePushedImplicitWindow = true;

#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiTestEngineHook_PostNewFrame(&g);
#endif
}

void ImGui::Initialize(ImGuiContext* context)
{
    ImGuiContext& g = *context;
    IM_ASSERT(!g.Initialized && !g.SettingsLoaded);

    // Add .ini handle for ImGuiWindow type
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "Window";
    ini_handler.TypeHash = ImHash("Window", 0, 0);
    ini_handler.ReadOpenFn = SettingsHandlerWindow_ReadOpen;
    ini_handler.ReadLineFn = SettingsHandlerWindow_ReadLine;
    ini_handler.WriteAllFn = SettingsHandlerWindow_WriteAll;
    g.SettingsHandlers.push_back(ini_handler);

    // Create default viewport
    ImGuiViewportP* viewport = IM_NEW(ImGuiViewportP)();
    viewport->ID = IMGUI_VIEWPORT_DEFAULT_ID;
    viewport->Idx = 0;
    g.Viewports.push_back(viewport);
    g.PlatformIO.MainViewport = g.Viewports[0]; // Make it accessible in public-facing GetPlatformIO() immediately (before the first call to EndFrame)
    g.PlatformIO.Viewports.push_back(g.Viewports[0]);

    // Extensions
    IM_ASSERT(g.DockContext == NULL);
    DockContextInitialize(&g);

    g.Initialized = true;
}

// This function is merely here to free heap allocations.
void ImGui::Shutdown(ImGuiContext* context)
{
    // The fonts atlas can be used prior to calling NewFrame(), so we clear it even if g.Initialized is FALSE (which would happen if we never called NewFrame)
    ImGuiContext& g = *context;
    if (g.IO.Fonts && g.FontAtlasOwnedByContext)
    {
        g.IO.Fonts->Locked = false;
        IM_DELETE(g.IO.Fonts);
    }
    g.IO.Fonts = NULL;

    // Cleanup of other data are conditional on actually having initialized ImGui.
    if (!g.Initialized)
        return;

    // Save settings (unless we haven't attempted to load them: CreateContext/DestroyContext without a call to NewFrame shouldn't save an empty file)
    if (g.SettingsLoaded && g.IO.IniFilename != NULL)
    {
        ImGuiContext* backup_context = GImGui;
        SetCurrentContext(context);
        SaveIniSettingsToDisk(g.IO.IniFilename);
        SetCurrentContext(backup_context);
    }

    // Destroy platform windows
    ImGuiContext* backup_context = ImGui::GetCurrentContext();
    SetCurrentContext(context);
    DestroyPlatformWindows();
    SetCurrentContext(backup_context);

    // Shutdown extensions
    IM_ASSERT(g.DockContext != NULL);
    DockContextShutdown(&g);

    // Clear everything else
    for (int i = 0; i < g.Windows.Size; i++)
        IM_DELETE(g.Windows[i]);
    g.Windows.clear();
    g.WindowsFocusOrder.clear();
    g.WindowsSortBuffer.clear();
    g.CurrentWindow = NULL;
    g.CurrentWindowStack.clear();
    g.WindowsById.Clear();
    g.NavWindow = NULL;
    g.HoveredWindow = g.HoveredRootWindow = g.HoveredWindowUnderMovingWindow = NULL;
    g.ActiveIdWindow = g.ActiveIdPreviousFrameWindow = NULL;
    g.MovingWindow = NULL;
    g.ColorModifiers.clear();
    g.StyleModifiers.clear();
    g.FontStack.clear();
    g.OpenPopupStack.clear();
    g.BeginPopupStack.clear();
    g.CurrentViewport = g.MouseViewport = g.MouseLastHoveredViewport = NULL;
    for (int i = 0; i < g.Viewports.Size; i++)
        IM_DELETE(g.Viewports[i]);
    g.Viewports.clear();
    g.PrivateClipboard.clear();
    g.InputTextState.TextW.clear();
    g.InputTextState.InitialText.clear();
    g.InputTextState.TempBuffer.clear();

    for (int i = 0; i < g.SettingsWindows.Size; i++)
        IM_DELETE(g.SettingsWindows[i].Name);
    g.SettingsWindows.clear();
    g.SettingsHandlers.clear();

    if (g.LogFile && g.LogFile != stdout)
    {
        fclose(g.LogFile);
        g.LogFile = NULL;
    }
    g.LogClipboard.clear();

    g.Initialized = false;
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
        if (count > 1)
            ImQsort(window->DC.ChildWindows.begin(), (size_t)count, sizeof(ImGuiWindow*), ChildWindowComparer);
        for (int i = 0; i < count; i++)
        {
            ImGuiWindow* child = window->DC.ChildWindows[i];
            if (child->Active)
                AddWindowToSortBuffer(out_sorted_windows, child);
        }
    }
}

static void AddDrawListToDrawData(ImVector<ImDrawList*>* out_list, ImDrawList* draw_list)
{
    if (draw_list->CmdBuffer.empty())
        return;

    // Remove trailing command if unused
    ImDrawCmd& last_cmd = draw_list->CmdBuffer.back();
    if (last_cmd.ElemCount == 0 && last_cmd.UserCallback == NULL)
    {
        draw_list->CmdBuffer.pop_back();
        if (draw_list->CmdBuffer.empty())
            return;
    }

    // Draw list sanity check. Detect mismatch between PrimReserve() calls and incrementing _VtxCurrentIdx, _VtxWritePtr etc. May trigger for you if you are using PrimXXX functions incorrectly.
    IM_ASSERT(draw_list->VtxBuffer.Size == 0 || draw_list->_VtxWritePtr == draw_list->VtxBuffer.Data + draw_list->VtxBuffer.Size);
    IM_ASSERT(draw_list->IdxBuffer.Size == 0 || draw_list->_IdxWritePtr == draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size);
    IM_ASSERT((int)draw_list->_VtxCurrentIdx == draw_list->VtxBuffer.Size);

    // Check that draw_list doesn't use more vertices than indexable (default ImDrawIdx = unsigned short = 2 bytes = 64K vertices per ImDrawList = per window)
    // If this assert triggers because you are drawing lots of stuff manually:
    // A) Make sure you are coarse clipping, because ImDrawList let all your vertices pass. You can use the Metrics window to inspect draw list contents.
    // B) If you need/want meshes with more than 64K vertices, uncomment the '#define ImDrawIdx unsigned int' line in imconfig.h to set the index size to 4 bytes.
    //    You'll need to handle the 4-bytes indices to your renderer. For example, the OpenGL example code detect index size at compile-time by doing:
    //      glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
    //    Your own engine or render API may use different parameters or function calls to specify index sizes. 2 and 4 bytes indices are generally supported by most API.
    // C) If for some reason you cannot use 4 bytes indices or don't want to, a workaround is to call BeginChild()/EndChild() before reaching the 64K limit to split your draw commands in multiple draw lists.
    if (sizeof(ImDrawIdx) == 2)
        IM_ASSERT(draw_list->_VtxCurrentIdx < (1 << 16) && "Too many vertices in ImDrawList using 16-bit indices. Read comment above");

    out_list->push_back(draw_list);
}

static void AddWindowToDrawData(ImGuiWindow* window, int layer)
{
    ImGuiContext& g = *GImGui;
    g.IO.MetricsRenderWindows++;
    AddDrawListToDrawData(&window->Viewport->DrawDataBuilder.Layers[layer], window->DrawList);
    for (int i = 0; i < window->DC.ChildWindows.Size; i++)
    {
        ImGuiWindow* child = window->DC.ChildWindows[i];
        if (IsWindowActiveAndVisible(child)) // Clipped children may have been marked not active
            AddWindowToDrawData(child, layer);
    }
}

// Layer is locked for the root window, however child windows may use a different viewport (e.g. extruding menu)
static void AddRootWindowToDrawData(ImGuiWindow* window)
{
    int layer = (window->Flags & ImGuiWindowFlags_Tooltip) ? 1 : 0;
    AddWindowToDrawData(window, layer);
}

void ImDrawDataBuilder::FlattenIntoSingleLayer()
{
    int n = Layers[0].Size;
    int size = n;
    for (int i = 1; i < IM_ARRAYSIZE(Layers); i++)
        size += Layers[i].Size;
    Layers[0].resize(size);
    for (int layer_n = 1; layer_n < IM_ARRAYSIZE(Layers); layer_n++)
    {
        ImVector<ImDrawList*>& layer = Layers[layer_n];
        if (layer.empty())
            continue;
        memcpy(&Layers[0][n], &layer[0], layer.Size * sizeof(ImDrawList*));
        n += layer.Size;
        layer.resize(0);
    }
}

static void SetupViewportDrawData(ImGuiViewportP* viewport, ImVector<ImDrawList*>* draw_lists)
{
    ImDrawData* draw_data = &viewport->DrawDataP;
    viewport->DrawData = draw_data; // Make publicly accessible
    draw_data->Valid = true;
    draw_data->CmdLists = (draw_lists->Size > 0) ? draw_lists->Data : NULL;
    draw_data->CmdListsCount = draw_lists->Size;
    draw_data->TotalVtxCount = draw_data->TotalIdxCount = 0;
    draw_data->DisplayPos = viewport->Pos;
    draw_data->DisplaySize = viewport->Size;
    for (int n = 0; n < draw_lists->Size; n++)
    {
        draw_data->TotalVtxCount += draw_lists->Data[n]->VtxBuffer.Size;
        draw_data->TotalIdxCount += draw_lists->Data[n]->IdxBuffer.Size;
    }
}

// When using this function it is sane to ensure that float are perfectly rounded to integer values, to that e.g. (int)(max.x-min.x) in user's render produce correct result.
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

static ImGuiWindow* FindFrontMostVisibleChildWindow(ImGuiWindow* window)
{
    for (int n = window->DC.ChildWindows.Size - 1; n >= 0; n--)
        if (IsWindowActiveAndVisible(window->DC.ChildWindows[n]))
            return FindFrontMostVisibleChildWindow(window->DC.ChildWindows[n]);
    return window;
}

static void ImGui::EndFrameDrawDimmedBackgrounds()
{
    ImGuiContext& g = *GImGui;

    // Draw modal whitening background on _other_ viewports than the one the modal is one
    ImGuiWindow* modal_window = GetFrontMostPopupModal();
    const bool dim_bg_for_modal = (modal_window != NULL);
    const bool dim_bg_for_window_list = (g.NavWindowingTargetAnim != NULL);
    if (dim_bg_for_modal || dim_bg_for_window_list)
        for (int viewport_n = 0; viewport_n < g.Viewports.Size; viewport_n++)
        {
            ImGuiViewportP* viewport = g.Viewports[viewport_n];
            if (modal_window && viewport == modal_window->Viewport)
                continue;
            if (g.NavWindowingList && viewport == g.NavWindowingList->Viewport)
                continue;
            if (g.NavWindowingTargetAnim && viewport == g.NavWindowingTargetAnim->Viewport)
                continue;
            ImDrawList* draw_list = GetOverlayDrawList(viewport);
            const ImU32 dim_bg_col = GetColorU32(dim_bg_for_modal ? ImGuiCol_ModalWindowDimBg : ImGuiCol_NavWindowingDimBg, g.DimBgRatio);
            draw_list->AddRectFilled(viewport->Pos, viewport->Pos + viewport->Size, dim_bg_col);
        }

    // Draw modal whitening background between CTRL-TAB list
    if (dim_bg_for_window_list)
    {
        // Choose a draw list that will be front-most across all our children
        ImGuiWindow* window = g.NavWindowingTargetAnim;
        ImDrawList* draw_list = FindFrontMostVisibleChildWindow(window->RootWindow)->DrawList;
        draw_list->PushClipRectFullScreen();

        // Docking: draw modal whitening background on other nodes of a same dock tree
        if (window->RootWindowDockStop->DockIsActive)
            if (window->RootWindow != window->RootWindowDockStop)
                RenderRectFilledWithHole(draw_list, window->RootWindow->Rect(), window->RootWindowDockStop->Rect(), GetColorU32(ImGuiCol_NavWindowingDimBg, g.DimBgRatio), g.Style.WindowRounding);

        // Draw navigation selection/windowing rectangle border
        float rounding = ImMax(window->WindowRounding, g.Style.WindowRounding);
        ImRect bb = window->Rect();
        bb.Expand(g.FontSize);
        if (bb.Contains(window->Viewport->GetRect())) // If a window fits the entire viewport, adjust its highlight inward
        {
            bb.Expand(-g.FontSize - 1.0f);
            rounding = window->WindowRounding;
        }
        draw_list->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha), rounding, ~0, 3.0f);
        draw_list->PopClipRect();
    }
}

// This is normally called by Render(). You may want to call it directly if you want to avoid calling Render() but the gain will be very minimal.
void ImGui::EndFrame()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);
    if (g.FrameCountEnded == g.FrameCount)          // Don't process EndFrame() multiple times.
        return;
    IM_ASSERT(g.FrameScopeActive && "Forgot to call ImGui::NewFrame()?");

    // Notify OS when our Input Method Editor cursor has moved (e.g. CJK inputs using Microsoft IME)
    if (g.PlatformIO.Platform_SetImeInputPos && ImLengthSqr(g.PlatformImePos - g.PlatformImeLastPos) > 0.0001f && g.PlatformImePosViewport && g.PlatformImePosViewport->PlatformWindowCreated)
    {
        g.PlatformIO.Platform_SetImeInputPos(g.PlatformImePosViewport, g.PlatformImePos);
        g.PlatformImeLastPos = g.PlatformImePos;
        g.PlatformImePosViewport = NULL;
    }

    // Report when there is a mismatch of Begin/BeginChild vs End/EndChild calls. Important: Remember that the Begin/BeginChild API requires you
    // to always call End/EndChild even if Begin/BeginChild returns false! (this is unfortunately inconsistent with most other Begin* API).
    if (g.CurrentWindowStack.Size != 1)
    {
        if (g.CurrentWindowStack.Size > 1)
        {
            IM_ASSERT(g.CurrentWindowStack.Size == 1 && "Mismatched Begin/BeginChild vs End/EndChild calls: did you forget to call End/EndChild?");
            while (g.CurrentWindowStack.Size > 1) // FIXME-ERRORHANDLING
                End();
        }
        else
        {
            IM_ASSERT(g.CurrentWindowStack.Size == 1 && "Mismatched Begin/BeginChild vs End/EndChild calls: did you call End/EndChild too much?");
        }
    }

    // Hide implicit/fallback "Debug" window if it hasn't been used
    g.FrameScopePushedImplicitWindow = false;
    if (g.CurrentWindow && !g.CurrentWindow->WriteAccessed)
        g.CurrentWindow->Active = false;
    End();

    // Draw modal whitening background on _other_ viewports than the one the modal is one
    EndFrameDrawDimmedBackgrounds();

    // Show CTRL+TAB list window
    if (g.NavWindowingTarget)
        NavUpdateWindowingList();

    SetCurrentViewport(NULL, NULL);

    // Drag and Drop: Elapse payload (if delivered, or if source stops being submitted)
    if (g.DragDropActive)
    {
        bool is_delivered = g.DragDropPayload.Delivery;
        bool is_elapsed = (g.DragDropPayload.DataFrameCount + 1 < g.FrameCount) && ((g.DragDropSourceFlags & ImGuiDragDropFlags_SourceAutoExpirePayload) || !IsMouseDown(g.DragDropMouseButton));
        if (is_delivered || is_elapsed)
            ClearDragDrop();
    }

    // Drag and Drop: Fallback for source tooltip. This is not ideal but better than nothing.
    if (g.DragDropActive && g.DragDropSourceFrameCount < g.FrameCount)
    {
        g.DragDropWithinSourceOrTarget = true;
        SetTooltip("...");
        g.DragDropWithinSourceOrTarget = false;
    }

    // End frame
    g.FrameScopeActive = false;
    g.FrameCountEnded = g.FrameCount;

    // Initiate moving window + handle left-click and right-click focus
    UpdateMouseMovingWindowEndFrame();

    // Update user-facing viewport list (g.Viewports -> g.PlatformIO.Viewports after filtering out some)
    UpdateViewportsEndFrame();

    // Sort the window list so that all child windows are after their parent
    // We cannot do that on FocusWindow() because childs may not exist yet
    g.WindowsSortBuffer.resize(0);
    g.WindowsSortBuffer.reserve(g.Windows.Size);
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Active && (window->Flags & ImGuiWindowFlags_ChildWindow))       // if a child is active its parent will add it
            continue;
        AddWindowToSortBuffer(&g.WindowsSortBuffer, window);
    }

    // This usually assert if there is a mismatch between the ImGuiWindowFlags_ChildWindow / ParentWindow values and DC.ChildWindows[] in parents, aka we've done something wrong.
    IM_ASSERT(g.Windows.Size == g.WindowsSortBuffer.Size);
    g.Windows.swap(g.WindowsSortBuffer);
    g.IO.MetricsActiveWindows = g.WindowsActiveCount;

    // Unlock font atlas
    g.IO.Fonts->Locked = false;

    // Clear Input data for next frame
    g.IO.MouseWheel = g.IO.MouseWheelH = 0.0f;
    g.IO.InputQueueCharacters.resize(0);
    memset(g.IO.NavInputs, 0, sizeof(g.IO.NavInputs));
}

void ImGui::Render()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);

    if (g.FrameCountEnded != g.FrameCount)
        EndFrame();
    g.FrameCountRendered = g.FrameCount;

    // Gather ImDrawList to render (for each active window)
    g.IO.MetricsRenderVertices = g.IO.MetricsRenderIndices = g.IO.MetricsRenderWindows = 0;
    for (int n = 0; n != g.Viewports.Size; n++)
        g.Viewports[n]->DrawDataBuilder.Clear();
    ImGuiWindow* windows_to_render_front_most[2];
    windows_to_render_front_most[0] = (g.NavWindowingTarget && !(g.NavWindowingTarget->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus)) ? g.NavWindowingTarget->RootWindow : NULL;
    windows_to_render_front_most[1] = g.NavWindowingTarget ? g.NavWindowingList : NULL;
    for (int n = 0; n != g.Windows.Size; n++)
    {
        ImGuiWindow* window = g.Windows[n];
        if (IsWindowActiveAndVisible(window) && (window->Flags & ImGuiWindowFlags_ChildWindow) == 0 && window != windows_to_render_front_most[0] && window != windows_to_render_front_most[1])
            AddRootWindowToDrawData(window);
    }
    for (int n = 0; n < IM_ARRAYSIZE(windows_to_render_front_most); n++)
        if (windows_to_render_front_most[n] && IsWindowActiveAndVisible(windows_to_render_front_most[n])) // NavWindowingTarget is always temporarily displayed as the front-most window
            AddRootWindowToDrawData(windows_to_render_front_most[n]);

    // Draw software mouse cursor if requested
    if (g.IO.MouseDrawCursor)
        RenderMouseCursor(g.IO.MousePos, g.Style.MouseCursorScale, g.MouseCursor);

    // Setup ImDrawData structures for end-user
    g.IO.MetricsRenderVertices = g.IO.MetricsRenderIndices = 0;
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        viewport->DrawDataBuilder.FlattenIntoSingleLayer();
        if (viewport->OverlayDrawList != NULL)
            AddDrawListToDrawData(&viewport->DrawDataBuilder.Layers[0], GetOverlayDrawList(viewport));
        SetupViewportDrawData(viewport, &viewport->DrawDataBuilder.Layers[0]);
        g.IO.MetricsRenderVertices += viewport->DrawData->TotalVtxCount;
        g.IO.MetricsRenderIndices += viewport->DrawData->TotalIdxCount;
    }

    // (Legacy) Call the Render callback function. The current prefer way is to let the user retrieve GetDrawData() and call the render function themselves.
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    if (g.Viewports[0]->DrawData->CmdListsCount > 0 && g.IO.RenderDrawListsFn != NULL)
        g.IO.RenderDrawListsFn(g.Viewports[0]->DrawData);
#endif
}

// Calculate text size. Text can be multi-line. Optionally ignore text after a ## marker.
// CalcTextSize("") should return ImVec2(0.0f, GImGui->FontSize)
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

    // Cancel out character spacing for the last character of a line (it is baked into glyph->AdvanceX field)
    const float font_scale = font_size / font->FontSize;
    const float character_spacing_x = 1.0f * font_scale;
    if (text_size.x > 0.0f)
        text_size.x -= character_spacing_x;
    text_size.x = (float)(int)(text_size.x + 0.95f);

    return text_size;
}

// Helper to calculate coarse clipping of large list of evenly sized items.
// NB: Prefer using the ImGuiListClipper higher-level helper if you can! Read comments and instructions there on how those use this sort of pattern.
// NB: 'items_count' is only used to clamp the result, if you don't know your count you can use INT_MAX
void ImGui::CalcListClipping(int items_count, float items_height, int* out_items_display_start, int* out_items_display_end)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.LogEnabled)
    {
        // If logging is active, do not perform any clipping
        *out_items_display_start = 0;
        *out_items_display_end = items_count;
        return;
    }
    if (window->SkipItems)
    {
        *out_items_display_start = *out_items_display_end = 0;
        return;
    }

    // We create the union of the ClipRect and the NavScoringRect which at worst should be 1 page away from ClipRect
    ImRect unclipped_rect = window->ClipRect;
    if (g.NavMoveRequest)
        unclipped_rect.Add(g.NavScoringRectScreen);

    const ImVec2 pos = window->DC.CursorPos;
    int start = (int)((unclipped_rect.Min.y - pos.y) / items_height);
    int end = (int)((unclipped_rect.Max.y - pos.y) / items_height);

    // When performing a navigation request, ensure we have one item extra in the direction we are moving to
    if (g.NavMoveRequest && g.NavMoveClipDir == ImGuiDir_Up)
        start--;
    if (g.NavMoveRequest && g.NavMoveClipDir == ImGuiDir_Down)
        end++;

    start = ImClamp(start, 0, items_count);
    end = ImClamp(end + 1, start, items_count);
    *out_items_display_start = start;
    *out_items_display_end = end;
}

// Find window given position, search front-to-back
// FIXME: Note that we have an inconsequential lag here: OuterRectClipped is updated in Begin(), so windows moved programatically 
// with SetWindowPos() and not SetNextWindowPos() will have that rectangle lagging by a frame at the time FindHoveredWindow() is 
// called, aka before the next Begin(). Moving window isn't affected.
static void FindHoveredWindow()
{
    ImGuiContext& g = *GImGui;

    // Special handling for the window being moved: Ignore the mouse viewport check (because it may reset/lose its viewport during the undocking frame)
    ImGuiViewportP* moving_window_viewport = g.MovingWindow ? g.MovingWindow->Viewport : NULL;
    if (g.MovingWindow)
        g.MovingWindow->Viewport = g.MouseViewport;

    ImGuiWindow* hovered_window = NULL;
    ImGuiWindow* hovered_window_ignoring_moving_window = NULL;
    if (g.MovingWindow && !(g.MovingWindow->Flags & ImGuiWindowFlags_NoMouseInputs))
        hovered_window = g.MovingWindow;

    ImVec2 padding_regular = g.Style.TouchExtraPadding;
    ImVec2 padding_for_resize_from_edges = g.IO.ConfigWindowsResizeFromEdges ? ImMax(g.Style.TouchExtraPadding, ImVec2(WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS, WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS)) : padding_regular;
    for (int i = g.Windows.Size - 1; i >= 0; i--)
    {
        ImGuiWindow* window = g.Windows[i];
        if (!window->Active || window->Hidden)
            continue;
        if (window->Flags & ImGuiWindowFlags_NoMouseInputs)
            continue;
        IM_ASSERT(window->Viewport);
        if (window->Viewport != g.MouseViewport)
            continue;

        // Using the clipped AABB, a child window will typically be clipped by its parent (not always)
        ImRect bb(window->OuterRectClipped);
        if ((window->Flags & ImGuiWindowFlags_ChildWindow) || (window->Flags & ImGuiWindowFlags_NoResize))
            bb.Expand(padding_regular);
        else
            bb.Expand(padding_for_resize_from_edges);
        if (!bb.Contains(g.IO.MousePos))
            continue;

        if (window->HitTestHoleSize.x != 0)
        {
            // FIXME: Consider generalizing hit-testing override (with more generic data, callback, etc.) (#1512)
            ImRect hole_bb((float)(window->HitTestHoleOffset.x), (float)(window->HitTestHoleOffset.y),
                (float)(window->HitTestHoleOffset.x + window->HitTestHoleSize.x), (float)(window->HitTestHoleOffset.y + window->HitTestHoleSize.y));
            if (hole_bb.Contains(g.IO.MousePos - window->Pos))
                continue;
        }
        
        if (hovered_window == NULL)
            hovered_window = window;
        if (hovered_window_ignoring_moving_window == NULL && (!g.MovingWindow || window->RootWindow != g.MovingWindow->RootWindow))
            hovered_window_ignoring_moving_window = window;
        if (hovered_window && hovered_window_ignoring_moving_window)
            break;
    }

    g.HoveredWindow = hovered_window;
    g.HoveredRootWindow = g.HoveredWindow ? g.HoveredWindow->RootWindow : NULL;
    g.HoveredWindowUnderMovingWindow = hovered_window_ignoring_moving_window;

    if (g.MovingWindow)
        g.MovingWindow->Viewport = moving_window_viewport;
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

    // Expand for touch input
    const ImRect rect_for_touch(rect_clipped.Min - g.Style.TouchExtraPadding, rect_clipped.Max + g.Style.TouchExtraPadding);
    if (!rect_for_touch.Contains(g.IO.MousePos))
        return false;
    if (!g.MouseViewport->GetRect().Overlaps(rect_clipped))
        return false;
    return true;
}

int ImGui::GetKeyIndex(ImGuiKey imgui_key)
{
    IM_ASSERT(imgui_key >= 0 && imgui_key < ImGuiKey_COUNT);
    return GImGui->IO.KeyMap[imgui_key];
}

// Note that imgui doesn't know the semantic of each entry of io.KeysDown[]. Use your own indices/enums according to how your back-end/engine stored them into io.KeysDown[]!
bool ImGui::IsKeyDown(int user_key_index)
{
    if (user_key_index < 0) return false;
    IM_ASSERT(user_key_index >= 0 && user_key_index < IM_ARRAYSIZE(GImGui->IO.KeysDown));
    return GImGui->IO.KeysDown[user_key_index];
}

int ImGui::CalcTypematicPressedRepeatAmount(float t, float t_prev, float repeat_delay, float repeat_rate)
{
    if (t == 0.0f)
        return 1;
    if (t <= repeat_delay || repeat_rate <= 0.0f)
        return 0;
    const int count = (int)((t - repeat_delay) / repeat_rate) - (int)((t_prev - repeat_delay) / repeat_rate);
    return (count > 0) ? count : 0;
}

int ImGui::GetKeyPressedAmount(int key_index, float repeat_delay, float repeat_rate)
{
    ImGuiContext& g = *GImGui;
    if (key_index < 0) return false;
    IM_ASSERT(key_index >= 0 && key_index < IM_ARRAYSIZE(g.IO.KeysDown));
    const float t = g.IO.KeysDownDuration[key_index];
    return CalcTypematicPressedRepeatAmount(t, t - g.IO.DeltaTime, repeat_delay, repeat_rate);
}

bool ImGui::IsKeyPressed(int user_key_index, bool repeat)
{
    ImGuiContext& g = *GImGui;
    if (user_key_index < 0) return false;
    IM_ASSERT(user_key_index >= 0 && user_key_index < IM_ARRAYSIZE(g.IO.KeysDown));
    const float t = g.IO.KeysDownDuration[user_key_index];
    if (t == 0.0f)
        return true;
    if (repeat && t > g.IO.KeyRepeatDelay)
        return GetKeyPressedAmount(user_key_index, g.IO.KeyRepeatDelay, g.IO.KeyRepeatRate) > 0;
    return false;
}

bool ImGui::IsKeyReleased(int user_key_index)
{
    ImGuiContext& g = *GImGui;
    if (user_key_index < 0) return false;
    IM_ASSERT(user_key_index >= 0 && user_key_index < IM_ARRAYSIZE(g.IO.KeysDown));
    return g.IO.KeysDownDurationPrev[user_key_index] >= 0.0f && !g.IO.KeysDown[user_key_index];
}

bool ImGui::IsMouseDown(int button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseDown[button];
}

bool ImGui::IsAnyMouseDown()
{
    ImGuiContext& g = *GImGui;
    for (int n = 0; n < IM_ARRAYSIZE(g.IO.MouseDown); n++)
        if (g.IO.MouseDown[n])
            return true;
    return false;
}

bool ImGui::IsMouseClicked(int button, bool repeat)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    const float t = g.IO.MouseDownDuration[button];
    if (t == 0.0f)
        return true;

    if (repeat && t > g.IO.KeyRepeatDelay)
    {
        float delay = g.IO.KeyRepeatDelay, rate = g.IO.KeyRepeatRate;
        if ((ImFmod(t - delay, rate) > rate*0.5f) != (ImFmod(t - delay - g.IO.DeltaTime, rate) > rate*0.5f))
            return true;
    }

    return false;
}

bool ImGui::IsMouseReleased(int button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseReleased[button];
}

bool ImGui::IsMouseDoubleClicked(int button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseDoubleClicked[button];
}

bool ImGui::IsMouseDragging(int button, float lock_threshold)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (!g.IO.MouseDown[button])
        return false;
    if (lock_threshold < 0.0f)
        lock_threshold = g.IO.MouseDragThreshold;
    return g.IO.MouseDragMaxDistanceSqr[button] >= lock_threshold * lock_threshold;
}

ImVec2 ImGui::GetMousePos()
{
    return GImGui->IO.MousePos;
}

// NB: prefer to call right after BeginPopup(). At the time Selectable/MenuItem is activated, the popup is already closed!
ImVec2 ImGui::GetMousePosOnOpeningCurrentPopup()
{
    ImGuiContext& g = *GImGui;
    if (g.BeginPopupStack.Size > 0)
        return g.OpenPopupStack[g.BeginPopupStack.Size-1].OpenMousePos;
    return g.IO.MousePos;
}

// We typically use ImVec2(-FLT_MAX,-FLT_MAX) to denote an invalid mouse position
bool ImGui::IsMousePosValid(const ImVec2* mouse_pos)
{
    if (mouse_pos == NULL)
        mouse_pos = &GImGui->IO.MousePos;
    const float MOUSE_INVALID = -256000.0f;
    return mouse_pos->x >= MOUSE_INVALID && mouse_pos->y >= MOUSE_INVALID;
}

// Return the delta from the initial clicking position.
// This is locked and return 0.0f until the mouse moves past a distance threshold at least once.
// NB: This is only valid if IsMousePosValid(). Back-ends in theory should always keep mouse position valid when dragging even outside the client window.
ImVec2 ImGui::GetMouseDragDelta(int button, float lock_threshold)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (lock_threshold < 0.0f)
        lock_threshold = g.IO.MouseDragThreshold;
    if (g.IO.MouseDown[button])
        if (g.IO.MouseDragMaxDistanceSqr[button] >= lock_threshold * lock_threshold)
            return g.IO.MousePos - g.IO.MouseClickedPos[button];     // Assume we can only get active with left-mouse button (at the moment).
    return ImVec2(0.0f, 0.0f);
}

void ImGui::ResetMouseDragDelta(int button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    // NB: We don't need to reset g.IO.MouseDragMaxDistanceSqr
    g.IO.MouseClickedPos[button] = g.IO.MousePos;
}

ImGuiMouseCursor ImGui::GetMouseCursor()
{
    return GImGui->MouseCursor;
}

void ImGui::SetMouseCursor(ImGuiMouseCursor cursor_type)
{
    GImGui->MouseCursor = cursor_type;
}

void ImGui::CaptureKeyboardFromApp(bool capture)
{
    GImGui->WantCaptureKeyboardNextFrame = capture ? 1 : 0;
}

void ImGui::CaptureMouseFromApp(bool capture)
{
    GImGui->WantCaptureMouseNextFrame = capture ? 1 : 0;
}

bool ImGui::IsItemActive()
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId)
    {
        ImGuiWindow* window = g.CurrentWindow;
        return g.ActiveId == window->DC.LastItemId;
    }
    return false;
}

bool ImGui::IsItemDeactivated()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    return (g.ActiveIdPreviousFrame == window->DC.LastItemId && g.ActiveIdPreviousFrame != 0 && g.ActiveId != window->DC.LastItemId);
}

bool ImGui::IsItemDeactivatedAfterEdit()
{
    ImGuiContext& g = *GImGui;
    return IsItemDeactivated() && (g.ActiveIdPreviousFrameHasBeenEdited || (g.ActiveId == 0 && g.ActiveIdHasBeenEdited));
}

bool ImGui::IsItemFocused()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (g.NavId == 0 || g.NavDisableHighlight || g.NavId != window->DC.LastItemId)
        return false;

    // Special handling for the dummy item after Begin() which represent the title bar or tab. 
    // When the window is collapsed (SkipItems==true) that last item will never be overwritten so we need to detect the case.
    if (window->DC.LastItemId == window->ID && window->WriteAccessed)
        return false;

    return true;
}

bool ImGui::IsItemClicked(int mouse_button)
{
    return IsMouseClicked(mouse_button) && IsItemHovered(ImGuiHoveredFlags_None);
}

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
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ClipRect.Overlaps(window->DC.LastItemRect);
}

bool ImGui::IsItemEdited()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return (window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_Edited) != 0;
}

// Allow last item to be overlapped by a subsequent item. Both may be activated during the same frame before the later one takes priority.
void ImGui::SetItemAllowOverlap()
{
    ImGuiContext& g = *GImGui;
    if (g.HoveredId == g.CurrentWindow->DC.LastItemId)
        g.HoveredIdAllowOverlap = true;
    if (g.ActiveId == g.CurrentWindow->DC.LastItemId)
        g.ActiveIdAllowOverlap = true;
}

ImVec2 ImGui::GetItemRectMin()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.LastItemRect.Min;
}

ImVec2 ImGui::GetItemRectMax()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.LastItemRect.Max;
}

ImVec2 ImGui::GetItemRectSize()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.LastItemRect.GetSize();
}

static bool ImGui::BeginChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;

    flags |= ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_ChildWindow|ImGuiWindowFlags_NoDocking;
    flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);  // Inherit the NoMove flag

    // Size
    const ImVec2 content_avail = GetContentRegionAvail();
    ImVec2 size = ImFloor(size_arg);
    const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
    if (size.x <= 0.0f)
        size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
    if (size.y <= 0.0f)
        size.y = ImMax(content_avail.y + size.y, 4.0f);
    SetNextWindowSize(size);

    // Build up name. If you need to append to a same child from multiple location in the ID stack, use BeginChild(ImGuiID id) with a stable value.
    char title[256];
    if (name)
        ImFormatString(title, IM_ARRAYSIZE(title), "%s/%s_%08X", parent_window->Name, name, id);
    else
        ImFormatString(title, IM_ARRAYSIZE(title), "%s/%08X", parent_window->Name, id);

    const float backup_border_size = g.Style.ChildBorderSize;
    if (!border)
        g.Style.ChildBorderSize = 0.0f;
    bool ret = Begin(title, NULL, flags);
    g.Style.ChildBorderSize = backup_border_size;

    ImGuiWindow* child_window = g.CurrentWindow;
    child_window->ChildId = id;
    child_window->AutoFitChildAxises = auto_fit_axises;

    // Set the cursor to handle case where the user called SetNextWindowPos()+BeginChild() manually.
    // While this is not really documented/defined, it seems that the expected thing to do.
    if (child_window->BeginCount == 1)
        parent_window->DC.CursorPos = child_window->Pos;

    // Process navigation-in immediately so NavInit can run on first frame
    if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayerActiveMask != 0 || child_window->DC.NavHasScroll))
    {
        FocusWindow(child_window);
        NavInitWindow(child_window, false);
        SetActiveID(id+1, child_window); // Steal ActiveId with a dummy id so that key-press won't activate child item
        g.ActiveIdSource = ImGuiInputSource_Nav;
    }
    return ret;
}

bool ImGui::BeginChild(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    return BeginChildEx(str_id, window->GetID(str_id), size_arg, border, extra_flags);
}

bool ImGui::BeginChild(ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
    IM_ASSERT(id != 0);
    return BeginChildEx(NULL, id, size_arg, border, extra_flags);
}

void ImGui::EndChild()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);   // Mismatched BeginChild()/EndChild() callss
    if (window->BeginCount > 1)
    {
        End();
    }
    else
    {
        ImVec2 sz = window->Size;
        if (window->AutoFitChildAxises & (1 << ImGuiAxis_X)) // Arbitrary minimum zero-ish child size of 4.0f causes less trouble than a 0.0f
            sz.x = ImMax(4.0f, sz.x);
        if (window->AutoFitChildAxises & (1 << ImGuiAxis_Y))
            sz.y = ImMax(4.0f, sz.y);
        End();

        ImGuiWindow* parent_window = g.CurrentWindow;
        ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + sz);
        ItemSize(sz);
        if ((window->DC.NavLayerActiveMask != 0 || window->DC.NavHasScroll) && !(window->Flags & ImGuiWindowFlags_NavFlattened))
        {
            ItemAdd(bb, window->ChildId);
            RenderNavHighlight(bb, window->ChildId);

            // When browsing a window that has no activable items (scroll only) we keep a highlight on the child
            if (window->DC.NavLayerActiveMask == 0 && window == g.NavWindow)
                RenderNavHighlight(ImRect(bb.Min - ImVec2(2,2), bb.Max + ImVec2(2,2)), g.NavId, ImGuiNavHighlightFlags_TypeThin);
        }
        else
        {
            // Not navigable into
            ItemAdd(bb, 0);
        }
    }
}

// Helper to create a child window / scrolling region that looks like a normal widget frame.
bool ImGui::BeginChildFrame(ImGuiID id, const ImVec2& size, ImGuiWindowFlags extra_flags)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
    PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
    PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
    PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);
    bool ret = BeginChild(id, size, true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | extra_flags);
    PopStyleVar(3);
    PopStyleColor();
    return ret;
}

void ImGui::EndChildFrame()
{
    EndChild();
}

// Save and compare stack sizes on Begin()/End() to detect usage errors
static void CheckStacksSize(ImGuiWindow* window, bool write)
{
    // NOT checking: DC.ItemWidth, DC.AllowKeyboardFocus, DC.ButtonRepeat, DC.TextWrapPos (per window) to allow user to conveniently push once and not pop (they are cleared on Begin)
    ImGuiContext& g = *GImGui;
    short* p_backup = &window->DC.StackSizesBackup[0];
    { int current = window->IDStack.Size;       if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup == current && "PushID/PopID or TreeNode/TreePop Mismatch!");   p_backup++; }    // Too few or too many PopID()/TreePop()
    { int current = window->DC.GroupStack.Size; if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup == current && "BeginGroup/EndGroup Mismatch!");                p_backup++; }    // Too few or too many EndGroup()
    { int current = g.BeginPopupStack.Size;     if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup == current && "BeginMenu/EndMenu or BeginPopup/EndPopup Mismatch"); p_backup++;}// Too few or too many EndMenu()/EndPopup()
    // For color, style and font stacks there is an incentive to use Push/Begin/Pop/.../End patterns, so we relax our checks a little to allow them.
    { int current = g.ColorModifiers.Size;      if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup >= current && "PushStyleColor/PopStyleColor Mismatch!");       p_backup++; }    // Too few or too many PopStyleColor()
    { int current = g.StyleModifiers.Size;      if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup >= current && "PushStyleVar/PopStyleVar Mismatch!");           p_backup++; }    // Too few or too many PopStyleVar()
    { int current = g.FontStack.Size;           if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup >= current && "PushFont/PopFont Mismatch!");                   p_backup++; }    // Too few or too many PopFont()
    IM_ASSERT(p_backup == window->DC.StackSizesBackup + IM_ARRAYSIZE(window->DC.StackSizesBackup));
}

static void SetWindowConditionAllowFlags(ImGuiWindow* window, ImGuiCond flags, bool enabled)
{
    window->SetWindowPosAllowFlags       = enabled ? (window->SetWindowPosAllowFlags       | flags) : (window->SetWindowPosAllowFlags       & ~flags);
    window->SetWindowSizeAllowFlags      = enabled ? (window->SetWindowSizeAllowFlags      | flags) : (window->SetWindowSizeAllowFlags      & ~flags);
    window->SetWindowCollapsedAllowFlags = enabled ? (window->SetWindowCollapsedAllowFlags | flags) : (window->SetWindowCollapsedAllowFlags & ~flags);
    window->SetWindowDockAllowFlags      = enabled ? (window->SetWindowDockAllowFlags      | flags) : (window->SetWindowDockAllowFlags      & ~flags);
}

ImGuiWindow* ImGui::FindWindowByID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    return (ImGuiWindow*)g.WindowsById.GetVoidPtr(id);
}

ImGuiWindow* ImGui::FindWindowByName(const char* name)
{
    ImGuiID id = ImHash(name, 0);
    return FindWindowByID(id);
}

static ImGuiWindow* CreateNewWindow(const char* name, ImVec2 size, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;

    // Create window the first time
    ImGuiWindow* window = IM_NEW(ImGuiWindow)(&g, name);
    window->Flags = flags;
    g.WindowsById.SetVoidPtr(window->ID, window);

    // Default/arbitrary window position. Use SetNextWindowPos() with the appropriate condition flag to change the initial position of a window.
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    window->Pos = main_viewport->Pos + ImVec2(60, 60);

    // User can disable loading and saving of settings. Tooltip and child windows also don't store settings.
    if (!(flags & ImGuiWindowFlags_NoSavedSettings))
        if (ImGuiWindowSettings* settings = ImGui::FindWindowSettings(window->ID))
        {
            // Retrieve settings from .ini file
            window->SettingsIdx = g.SettingsWindows.index_from_ptr(settings);
            SetWindowConditionAllowFlags(window, ImGuiCond_FirstUseEver, false);
            if (settings->ViewportId)
            {
                window->ViewportId = settings->ViewportId;
                window->ViewportPos = settings->ViewportPos;
            }
            else
            {
                window->ViewportPos = main_viewport->Pos;
            }
            window->Pos = ImFloor(settings->Pos + window->ViewportPos);
            window->Collapsed = settings->Collapsed;
            if (ImLengthSqr(settings->Size) > 0.00001f)
                size = ImFloor(settings->Size);
            window->DockId = settings->DockId;
            window->DockOrder = settings->DockOrder;
        }
    window->Size = window->SizeFull = window->SizeFullAtLastBegin = ImFloor(size);
    window->DC.CursorMaxPos = window->Pos; // So first call to CalcSizeContents() doesn't return crazy values

    if ((flags & ImGuiWindowFlags_AlwaysAutoResize) != 0)
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

    g.WindowsFocusOrder.push_back(window);
    if (flags & ImGuiWindowFlags_NoBringToFrontOnFocus)
        g.Windows.push_front(window); // Quite slow but rare and only once
    else
        g.Windows.push_back(window);
    return window;
}

static ImGuiWindow* GetWindowForTitleDisplay(ImGuiWindow* window)
{
    return window->DockNodeAsHost ? window->DockNodeAsHost->VisibleWindow : window;
}

static ImGuiWindow* GetWindowForTitleAndMenuHeight(ImGuiWindow* window)
{
    return (window->DockNodeAsHost && window->DockNodeAsHost->VisibleWindow) ? window->DockNodeAsHost->VisibleWindow : window;
}

static ImVec2 CalcSizeAfterConstraint(ImGuiWindow* window, ImVec2 new_size)
{
    ImGuiContext& g = *GImGui;
    if (g.NextWindowData.SizeConstraintCond != 0)
    {
        // Using -1,-1 on either X/Y axis to preserve the current size.
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
    }

    // Minimum size
    if (!(window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_AlwaysAutoResize)))
    {
        ImGuiWindow* window_for_height = GetWindowForTitleAndMenuHeight(window);
        new_size = ImMax(new_size, g.Style.WindowMinSize);
        new_size.y = ImMax(new_size.y, window_for_height->TitleBarHeight() + window_for_height->MenuBarHeight() + ImMax(0.0f, g.Style.WindowRounding - 1.0f)); // Reduce artifacts with very small windows
    }
    return new_size;
}

static ImVec2 CalcSizeContents(ImGuiWindow* window)
{
    if (window->Collapsed)
        return window->SizeContents;
    if (window->Hidden && window->HiddenFramesForResize == 0 && window->HiddenFramesRegular > 0)
        return window->SizeContents;

    ImVec2 sz;
    sz.x = (float)(int)((window->SizeContentsExplicit.x != 0.0f) ? window->SizeContentsExplicit.x : (window->DC.CursorMaxPos.x - window->Pos.x + window->Scroll.x));
    sz.y = (float)(int)((window->SizeContentsExplicit.y != 0.0f) ? window->SizeContentsExplicit.y : (window->DC.CursorMaxPos.y - window->Pos.y + window->Scroll.y));
    return sz + window->WindowPadding;
}

static ImVec2 CalcSizeAutoFit(ImGuiWindow* window, const ImVec2& size_contents)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    if (window->Flags & ImGuiWindowFlags_Tooltip)
    {
        // Tooltip always resize
        return size_contents;
    }
    else
    {
        // Maximum window size is determined by the viewport size or monitor size
        const bool is_popup = (window->Flags & ImGuiWindowFlags_Popup) != 0;
        const bool is_menu = (window->Flags & ImGuiWindowFlags_ChildMenu) != 0;
        ImVec2 size_min = style.WindowMinSize;
        if (is_popup || is_menu) // Popups and menus bypass style.WindowMinSize by default, but we give then a non-zero minimum size to facilitate understanding problematic cases (e.g. empty popups)
            size_min = ImMin(size_min, ImVec2(4.0f, 4.0f));

        ImVec2 avail_size = window->Viewport->Size;
        if (window->ViewportOwned)
            avail_size = ImVec2(FLT_MAX, FLT_MAX);
        const int monitor_idx = window->ViewportAllowPlatformMonitorExtend;
        if (monitor_idx >= 0 && monitor_idx < g.PlatformIO.Monitors.Size)
            avail_size = g.PlatformIO.Monitors[monitor_idx].WorkSize;
        ImVec2 size_auto_fit = ImClamp(size_contents, size_min, ImMax(size_min, avail_size - g.Style.DisplaySafeAreaPadding * 2.0f));

        // When the window cannot fit all contents (either because of constraints, either because screen is too small),
        // we are growing the size on the other axis to compensate for expected scrollbar. FIXME: Might turn bigger than ViewportSize-WindowPadding.
        ImVec2 size_auto_fit_after_constraint = CalcSizeAfterConstraint(window, size_auto_fit);
        if (size_auto_fit_after_constraint.x < size_contents.x && !(window->Flags & ImGuiWindowFlags_NoScrollbar) && (window->Flags & ImGuiWindowFlags_HorizontalScrollbar))
            size_auto_fit.y += style.ScrollbarSize;
        if (size_auto_fit_after_constraint.y < size_contents.y && !(window->Flags & ImGuiWindowFlags_NoScrollbar))
            size_auto_fit.x += style.ScrollbarSize;
        return size_auto_fit;
    }
}

ImVec2 ImGui::CalcWindowExpectedSize(ImGuiWindow* window)
{
    ImVec2 size_contents = CalcSizeContents(window);
    return CalcSizeAfterConstraint(window, CalcSizeAutoFit(window, size_contents));
}

float ImGui::GetWindowScrollMaxX(ImGuiWindow* window)
{
    return ImMax(0.0f, window->SizeContents.x - (window->SizeFull.x - window->ScrollbarSizes.x));
}

float ImGui::GetWindowScrollMaxY(ImGuiWindow* window)
{
    return ImMax(0.0f, window->SizeContents.y - (window->SizeFull.y - window->ScrollbarSizes.y));
}

static ImVec2 CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window, bool snap_on_edges)
{
    ImGuiContext& g = *GImGui;
    ImVec2 scroll = window->Scroll;
    if (window->ScrollTarget.x < FLT_MAX)
    {
        float cr_x = window->ScrollTargetCenterRatio.x;
        scroll.x = window->ScrollTarget.x - cr_x * (window->SizeFull.x - window->ScrollbarSizes.x);
    }
    if (window->ScrollTarget.y < FLT_MAX)
    {
        // 'snap_on_edges' allows for a discontinuity at the edge of scrolling limits to take account of WindowPadding so that scrolling to make the last item visible scroll far enough to see the padding.
        float cr_y = window->ScrollTargetCenterRatio.y;
        float target_y = window->ScrollTarget.y;
        if (snap_on_edges && cr_y <= 0.0f && target_y <= window->WindowPadding.y)
            target_y = 0.0f;
        if (snap_on_edges && cr_y >= 1.0f && target_y >= window->SizeContents.y - window->WindowPadding.y + g.Style.ItemSpacing.y)
            target_y = window->SizeContents.y;
        scroll.y = target_y - (1.0f - cr_y) * (window->TitleBarHeight() + window->MenuBarHeight()) - cr_y * (window->SizeFull.y - window->ScrollbarSizes.y);
    }
    scroll = ImMax(scroll, ImVec2(0.0f, 0.0f));
    if (!window->Collapsed && !window->SkipItems)
    {
        scroll.x = ImMin(scroll.x, ImGui::GetWindowScrollMaxX(window));
        scroll.y = ImMin(scroll.y, ImGui::GetWindowScrollMaxY(window));
    }
    return scroll;
}

static ImGuiCol GetWindowBgColorIdxFromFlags(ImGuiWindowFlags flags)
{
    if (flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup))
        return ImGuiCol_PopupBg;
    if (flags & ImGuiWindowFlags_ChildWindow)
        return ImGuiCol_ChildBg;
    return ImGuiCol_WindowBg;
}

static void CalcResizePosSizeFromAnyCorner(ImGuiWindow* window, const ImVec2& corner_target, const ImVec2& corner_norm, ImVec2* out_pos, ImVec2* out_size)
{
    ImVec2 pos_min = ImLerp(corner_target, window->Pos, corner_norm);                // Expected window upper-left
    ImVec2 pos_max = ImLerp(window->Pos + window->Size, corner_target, corner_norm); // Expected window lower-right
    ImVec2 size_expected = pos_max - pos_min;
    ImVec2 size_constrained = CalcSizeAfterConstraint(window, size_expected);
    *out_pos = pos_min;
    if (corner_norm.x == 0.0f)
        out_pos->x -= (size_constrained.x - size_expected.x);
    if (corner_norm.y == 0.0f)
        out_pos->y -= (size_constrained.y - size_expected.y);
    *out_size = size_constrained;
}

struct ImGuiResizeGripDef
{
    ImVec2  CornerPos;
    ImVec2  InnerDir;
    int     AngleMin12, AngleMax12;
};

const ImGuiResizeGripDef resize_grip_def[4] =
{
    { ImVec2(1,1), ImVec2(-1,-1), 0, 3 }, // Lower right
    { ImVec2(0,1), ImVec2(+1,-1), 3, 6 }, // Lower left
    { ImVec2(0,0), ImVec2(+1,+1), 6, 9 }, // Upper left
    { ImVec2(1,0), ImVec2(-1,+1), 9,12 }, // Upper right
};

static ImRect GetResizeBorderRect(ImGuiWindow* window, int border_n, float perp_padding, float thickness)
{
    ImRect rect = window->Rect();
    if (thickness == 0.0f) rect.Max -= ImVec2(1,1);
    if (border_n == 0) return ImRect(rect.Min.x + perp_padding, rect.Min.y - thickness,    rect.Max.x - perp_padding, rect.Min.y + thickness);
    if (border_n == 1) return ImRect(rect.Max.x - thickness,    rect.Min.y + perp_padding, rect.Max.x + thickness,    rect.Max.y - perp_padding);
    if (border_n == 2) return ImRect(rect.Min.x + perp_padding, rect.Max.y - thickness,    rect.Max.x - perp_padding, rect.Max.y + thickness);
    if (border_n == 3) return ImRect(rect.Min.x - thickness,    rect.Min.y + perp_padding, rect.Min.x + thickness,    rect.Max.y - perp_padding);
    IM_ASSERT(0);
    return ImRect();
}

// Handle resize for: Resize Grips, Borders, Gamepad
static void ImGui::UpdateManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4])
{
    ImGuiContext& g = *GImGui;
    ImGuiWindowFlags flags = window->Flags;
    if ((flags & ImGuiWindowFlags_NoResize) || (flags & ImGuiWindowFlags_AlwaysAutoResize) || window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
        return;
    if (window->WasActive == false) // Early out to avoid running this code for e.g. an hidden implicit/fallback Debug window.
        return;

    const int resize_border_count = g.IO.ConfigWindowsResizeFromEdges ? 4 : 0;
    const float grip_draw_size = (float)(int)ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f);
    const float grip_hover_inner_size = (float)(int)(grip_draw_size * 0.75f);
    const float grip_hover_outer_size = g.IO.ConfigWindowsResizeFromEdges ? WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS : 0.0f;

    ImVec2 pos_target(FLT_MAX, FLT_MAX);
    ImVec2 size_target(FLT_MAX, FLT_MAX);

    // Manual resize grips
    PushID("#RESIZE");
    for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
    {
        const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
        const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPos);

        // Using the FlattenChilds button flag we make the resize button accessible even if we are hovering over a child window
        ImRect resize_rect(corner - grip.InnerDir * grip_hover_outer_size, corner + grip.InnerDir * grip_hover_inner_size);
        if (resize_rect.Min.x > resize_rect.Max.x) ImSwap(resize_rect.Min.x, resize_rect.Max.x);
        if (resize_rect.Min.y > resize_rect.Max.y) ImSwap(resize_rect.Min.y, resize_rect.Max.y);
        bool hovered, held;
        ButtonBehavior(resize_rect, window->GetID((void*)(intptr_t)resize_grip_n), &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
        //GetOverlayDrawList(window)->AddRect(resize_rect.Min, resize_rect.Max, IM_COL32(255, 255, 0, 255));
        if (hovered || held)
            g.MouseCursor = (resize_grip_n & 1) ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE;

        if (held && g.IO.MouseDoubleClicked[0] && resize_grip_n == 0)
        {
            // Manual auto-fit when double-clicking
            size_target = CalcSizeAfterConstraint(window, size_auto_fit);
            ClearActiveID();
        }
        else if (held)
        {
            // Resize from any of the four corners
            // We don't use an incremental MouseDelta but rather compute an absolute target size based on mouse position
            ImVec2 corner_target = g.IO.MousePos - g.ActiveIdClickOffset + ImLerp(grip.InnerDir * grip_hover_outer_size, grip.InnerDir * -grip_hover_inner_size, grip.CornerPos); // Corner of the window corresponding to our corner grip
            CalcResizePosSizeFromAnyCorner(window, corner_target, grip.CornerPos, &pos_target, &size_target);
        }
        if (resize_grip_n == 0 || held || hovered)
            resize_grip_col[resize_grip_n] = GetColorU32(held ? ImGuiCol_ResizeGripActive : hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip);
    }
    for (int border_n = 0; border_n < resize_border_count; border_n++)
    {
        bool hovered, held;
        ImRect border_rect = GetResizeBorderRect(window, border_n, grip_hover_inner_size, WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS);
        ButtonBehavior(border_rect, window->GetID((void*)(intptr_t)(border_n + 4)), &hovered, &held, ImGuiButtonFlags_FlattenChildren);
        //GetOverlayDrawList(window)->AddRect(border_rect.Min, border_rect.Max, IM_COL32(255, 255, 0, 255));
        if ((hovered && g.HoveredIdTimer > WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER) || held)
        {
            g.MouseCursor = (border_n & 1) ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS;
            if (held) *border_held = border_n;
        }
        if (held)
        {
            ImVec2 border_target = window->Pos;
            ImVec2 border_posn;
            if (border_n == 0) { border_posn = ImVec2(0, 0); border_target.y = (g.IO.MousePos.y - g.ActiveIdClickOffset.y + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); }
            if (border_n == 1) { border_posn = ImVec2(1, 0); border_target.x = (g.IO.MousePos.x - g.ActiveIdClickOffset.x + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); }
            if (border_n == 2) { border_posn = ImVec2(0, 1); border_target.y = (g.IO.MousePos.y - g.ActiveIdClickOffset.y + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); }
            if (border_n == 3) { border_posn = ImVec2(0, 0); border_target.x = (g.IO.MousePos.x - g.ActiveIdClickOffset.x + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); }
            CalcResizePosSizeFromAnyCorner(window, border_target, border_posn, &pos_target, &size_target);
        }
    }
    PopID();

    // Navigation resize (keyboard/gamepad)
    if (g.NavWindowingTarget && g.NavWindowingTarget->RootWindow == window)
    {
        ImVec2 nav_resize_delta;
        if (g.NavInputSource == ImGuiInputSource_NavKeyboard && g.IO.KeyShift)
            nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard, ImGuiInputReadMode_Down);
        if (g.NavInputSource == ImGuiInputSource_NavGamepad)
            nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_Down);
        if (nav_resize_delta.x != 0.0f || nav_resize_delta.y != 0.0f)
        {
            const float NAV_RESIZE_SPEED = 600.0f;
            nav_resize_delta *= ImFloor(NAV_RESIZE_SPEED * g.IO.DeltaTime * ImMin(g.IO.DisplayFramebufferScale.x, g.IO.DisplayFramebufferScale.y));
            g.NavWindowingToggleLayer = false;
            g.NavDisableMouseHover = true;
            resize_grip_col[0] = GetColorU32(ImGuiCol_ResizeGripActive);
            // FIXME-NAV: Should store and accumulate into a separate size buffer to handle sizing constraints properly, right now a constraint will make us stuck.
            size_target = CalcSizeAfterConstraint(window, window->SizeFull + nav_resize_delta);
        }
    }

    // Apply back modified position/size to window
    if (size_target.x != FLT_MAX && (size_target.x != window->SizeFull.x || size_target.y != window->SizeFull.y))
    {
        window->SizeFull = size_target;
        MarkIniSettingsDirty(window);
    }
    if (pos_target.x != FLT_MAX)
    {
        window->Pos = ImFloor(pos_target);
        MarkIniSettingsDirty(window);
    }

    window->Size = window->SizeFull;
}

static inline void ClampWindowRect(ImGuiWindow* window, const ImRect& rect, const ImVec2& padding)
{
    ImGuiContext& g = *GImGui;
    ImVec2 size_for_clamping = (g.IO.ConfigWindowsMoveFromTitleBarOnly && !(window->Flags & ImGuiWindowFlags_NoTitleBar)) ? ImVec2(window->Size.x, window->TitleBarHeight()) : window->Size;
    window->Pos = ImMin(rect.Max - padding, ImMax(window->Pos + size_for_clamping, rect.Min + padding) - size_for_clamping);
}

void ImGui::UpdateWindowParentAndRootLinks(ImGuiWindow* window, ImGuiWindowFlags flags, ImGuiWindow* parent_window)
{
    window->ParentWindow = parent_window;
    window->RootWindow = window->RootWindowDockStop = window->RootWindowForTitleBarHighlight = window->RootWindowForNav = window;
    if (parent_window && (flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Tooltip))
    {
        window->RootWindow = parent_window->RootWindow;
        if (!window->DockIsActive && !(parent_window->Flags & ImGuiWindowFlags_DockNodeHost))
            window->RootWindowDockStop = parent_window->RootWindowDockStop;
    }
    if (parent_window && !(flags & ImGuiWindowFlags_Modal) && (flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)))
        window->RootWindowForTitleBarHighlight = parent_window->RootWindowForTitleBarHighlight;
    while (window->RootWindowForNav->Flags & ImGuiWindowFlags_NavFlattened)
        window->RootWindowForNav = window->RootWindowForNav->ParentWindow;
}

// Push a new ImGui window to add widgets to.
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
    IM_ASSERT(g.FrameScopeActive);                  // Forgot to call ImGui::NewFrame()
    IM_ASSERT(g.FrameCountEnded != g.FrameCount);   // Called ImGui::Render() or ImGui::EndFrame() and haven't called ImGui::NewFrame() again yet

    // Find or create
    ImGuiWindow* window = FindWindowByName(name);
    const bool window_just_created = (window == NULL);
    const bool window_is_fallback = (g.CurrentWindowStack.Size == 0);
    if (window_just_created)
    {
        ImVec2 size_on_first_use = (g.NextWindowData.SizeCond != 0) ? g.NextWindowData.SizeVal : ImVec2(0.0f, 0.0f); // Any condition flag will do since we are creating a new window here.
        window = CreateNewWindow(name, size_on_first_use, flags);
    }

    // Automatically disable manual moving/resizing when NoInputs is set
    if ((flags & ImGuiWindowFlags_NoInputs) == ImGuiWindowFlags_NoInputs)
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    if (flags & ImGuiWindowFlags_NavFlattened)
        IM_ASSERT(flags & ImGuiWindowFlags_ChildWindow);

    const int current_frame = g.FrameCount;
    const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);

    // Update the Appearing flag
    bool window_just_activated_by_user = (window->LastFrameActive < current_frame - 1);   // Not using !WasActive because the implicit "Debug" window would always toggle off->on
    const bool window_just_appearing_after_hidden_for_resize = (window->HiddenFramesForResize > 0);
    if (flags & ImGuiWindowFlags_Popup)
    {
        ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
        window_just_activated_by_user |= (window->PopupId != popup_ref.PopupId); // We recycle popups so treat window as activated if popup id changed
        window_just_activated_by_user |= (window != popup_ref.Window);
    }
    window->Appearing = (window_just_activated_by_user || window_just_appearing_after_hidden_for_resize);
    if (window->Appearing)
        SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, true);

    // Update Flags, LastFrameActive, BeginOrderXXX fields
    if (first_begin_of_the_frame)
    {
        window->FlagsPreviousFrame = window->Flags;
        window->Flags = (ImGuiWindowFlags)flags;
        window->LastFrameActive = current_frame;
        window->BeginOrderWithinParent = 0;
        window->BeginOrderWithinContext = (short)(g.WindowsActiveCount++);
    }
    else
    {
        flags = window->Flags;
    }

    // Docking
    // (NB: during the frame dock nodes are created, it is possible that (window->DockIsActive == false) even though (window->DockNode->Windows.Size > 1)
    IM_ASSERT(window->DockNode == NULL || window->DockNodeAsHost == NULL); // Cannot be both
    if (g.NextWindowData.DockCond)
        SetWindowDock(window, g.NextWindowData.DockId, g.NextWindowData.DockCond);
    if (first_begin_of_the_frame)
    {
        bool has_dock_node = (window->DockId != 0 || window->DockNode != NULL);
        bool new_auto_dock_node = !has_dock_node && g.IO.ConfigDockingTabBarOnSingleWindows && !(flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking)) && !window_is_fallback;
        if (has_dock_node || new_auto_dock_node)
        {
            BeginDocked(window, p_open);
            flags = window->Flags;
        }
    }

    // Parent window is latched only on the first call to Begin() of the frame, so further append-calls can be done from a different window stack
    ImGuiWindow* parent_window_in_stack = window->DockIsActive ? window->DockNode->HostWindow : g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back();
    ImGuiWindow* parent_window = first_begin_of_the_frame ? ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)) ? parent_window_in_stack : NULL) : window->ParentWindow;
    IM_ASSERT(parent_window != NULL || !(flags & ImGuiWindowFlags_ChildWindow));

    // Add to stack
    // We intentionally set g.CurrentWindow to NULL to prevent usage until when the viewport is set, then will call SetCurrentWindow()
    g.CurrentWindowStack.push_back(window);
    g.CurrentWindow = NULL;
    CheckStacksSize(window, true);
    if (flags & ImGuiWindowFlags_Popup)
    {
        ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
        popup_ref.Window = window;
        g.BeginPopupStack.push_back(popup_ref);
        window->PopupId = popup_ref.PopupId;
    }

    if (window_just_appearing_after_hidden_for_resize && !(flags & ImGuiWindowFlags_ChildWindow))
        window->NavLastIds[0] = 0;

    // Process SetNextWindow***() calls
    bool window_pos_set_by_api = false;
    bool window_size_x_set_by_api = false, window_size_y_set_by_api = false;
    if (g.NextWindowData.PosCond)
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
    if (g.NextWindowData.SizeCond)
    {
        window_size_x_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.x > 0.0f);
        window_size_y_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.y > 0.0f);
        SetWindowSize(window, g.NextWindowData.SizeVal, g.NextWindowData.SizeCond);
    }
    if (g.NextWindowData.ContentSizeCond)
    {
        // Adjust passed "client size" to become a "window size"
        window->SizeContentsExplicit = g.NextWindowData.ContentSizeVal;
        if (window->SizeContentsExplicit.y != 0.0f)
            window->SizeContentsExplicit.y += window->TitleBarHeight() + window->MenuBarHeight();
    }
    else if (first_begin_of_the_frame)
    {
        window->SizeContentsExplicit = ImVec2(0.0f, 0.0f);
    }
    window->WindowClass = g.NextWindowData.WindowClass;
    if (g.NextWindowData.CollapsedCond)
        SetWindowCollapsed(window, g.NextWindowData.CollapsedVal, g.NextWindowData.CollapsedCond);
    if (g.NextWindowData.FocusCond)
        FocusWindow(window);
    if (window->Appearing)
        SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, false);

    // When reusing window again multiple times a frame, just append content (don't need to setup again)
    if (first_begin_of_the_frame)
    {
        // Initialize
        const bool window_is_child_tooltip = (flags & ImGuiWindowFlags_ChildWindow) && (flags & ImGuiWindowFlags_Tooltip); // FIXME-WIP: Undocumented behavior of Child+Tooltip for pinned tooltip (#1345)
        UpdateWindowParentAndRootLinks(window, flags, parent_window);

        window->Active = true;
        window->HasCloseButton = (p_open != NULL);
        window->ClipRect = ImVec4(-FLT_MAX,-FLT_MAX,+FLT_MAX,+FLT_MAX);
        window->IDStack.resize(1);

        // Update stored window name when it changes (which can _only_ happen with the "###" operator, so the ID would stay unchanged).
        // The title bar always display the 'name' parameter, so we only update the string storage if it needs to be visible to the end-user elsewhere.
        bool window_title_visible_elsewhere = false;
        if ((window->Viewport && window->Viewport->Window == window) || (window->DockIsActive))
            window_title_visible_elsewhere = true;
        else if (g.NavWindowingList != NULL && (window->Flags & ImGuiWindowFlags_NoNavFocus) == 0)   // Window titles visible when using CTRL+TAB
            window_title_visible_elsewhere = true;
        if (window_title_visible_elsewhere && !window_just_created && strcmp(name, window->Name) != 0)
        {
            size_t buf_len = (size_t)window->NameBufLen;
            window->Name = ImStrdupcpy(window->Name, &buf_len, name);
            window->NameBufLen = (int)buf_len;
        }

        // UPDATE CONTENTS SIZE, UPDATE HIDDEN STATUS

        // Update contents size from last frame for auto-fitting (or use explicit size)
        window->SizeContents = CalcSizeContents(window);
        if (window->HiddenFramesRegular > 0)
            window->HiddenFramesRegular--;
        if (window->HiddenFramesForResize > 0)
            window->HiddenFramesForResize--;

        // Hide new windows for one frame until they calculate their size
        if (window_just_created && (!window_size_x_set_by_api || !window_size_y_set_by_api))
            window->HiddenFramesForResize = 1;

        // Hide popup/tooltip window when re-opening while we measure size (because we recycle the windows)
        // We reset Size/SizeContents for reappearing popups/tooltips early in this function, so further code won't be tempted to use the old size.
        if (window_just_activated_by_user && (flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0)
        {
            window->HiddenFramesForResize = 1;
            if (flags & ImGuiWindowFlags_AlwaysAutoResize)
            {
                if (!window_size_x_set_by_api)
                    window->Size.x = window->SizeFull.x = 0.f;
                if (!window_size_y_set_by_api)
                    window->Size.y = window->SizeFull.y = 0.f;
                window->SizeContents = ImVec2(0.f, 0.f);
            }
        }

        // SELECT VIEWPORT
        // We need to do this before using any style/font sizes, as viewport with a different DPI may affect font sizes.

        UpdateSelectWindowViewport(window);
        SetCurrentViewport(window, window->Viewport);
        window->FontDpiScale = (g.IO.ConfigFlags & ImGuiConfigFlags_DpiEnableScaleFonts) ? window->Viewport->DpiScale : 1.0f;
        SetCurrentWindow(window);
        flags = window->Flags;

        // Lock border size and padding for the frame (so that altering them doesn't cause inconsistencies)
        if (window->DockIsActive)
            window->WindowBorderSize = 0.0f;
        else if (flags & ImGuiWindowFlags_ChildWindow)
            window->WindowBorderSize = style.ChildBorderSize;
        else
            window->WindowBorderSize = ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupBorderSize : style.WindowBorderSize;
        if (!window->DockIsActive && (flags & ImGuiWindowFlags_ChildWindow) && !(flags & (ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_Popup)) && window->WindowBorderSize == 0.0f)
            window->WindowPadding = ImVec2(0.0f, (flags & ImGuiWindowFlags_MenuBar) ? style.WindowPadding.y : 0.0f);
        else
            window->WindowPadding = style.WindowPadding;
        window->DC.MenuBarOffset.x = ImMax(ImMax(window->WindowPadding.x, style.ItemSpacing.x), g.NextWindowData.MenuBarOffsetMinVal.x);
        window->DC.MenuBarOffset.y = g.NextWindowData.MenuBarOffsetMinVal.y;

        // Collapse window by double-clicking on title bar
        // At this point we don't have a clipping rectangle setup yet, so we can use the title bar area for hit detection and drawing
        if (!(flags & ImGuiWindowFlags_NoTitleBar) && !(flags & ImGuiWindowFlags_NoCollapse) && !window->DockIsActive)
        {
            // We don't use a regular button+id to test for double-click on title bar (mostly due to legacy reason, could be fixed), so verify that we don't have items over the title bar.
            ImRect title_bar_rect = window->TitleBarRect();
            if (g.HoveredWindow == window && g.HoveredId == 0 && g.HoveredIdPreviousFrame == 0 && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max) && g.IO.MouseDoubleClicked[0])
                window->WantCollapseToggle = true;
            if (window->WantCollapseToggle)
            {
                window->Collapsed = !window->Collapsed;
                MarkIniSettingsDirty(window);
                FocusWindow(window);
            }
        }
        else
        {
            window->Collapsed = false;
        }
        window->WantCollapseToggle = false;

        // SIZE

        // Calculate auto-fit size, handle automatic resize
        const ImVec2 size_auto_fit = CalcSizeAutoFit(window, window->SizeContents);
        ImVec2 size_full_modified(FLT_MAX, FLT_MAX);
        if ((flags & ImGuiWindowFlags_AlwaysAutoResize) && !window->Collapsed)
        {
            // Using SetNextWindowSize() overrides ImGuiWindowFlags_AlwaysAutoResize, so it can be used on tooltips/popups, etc.
            if (!window_size_x_set_by_api)
                window->SizeFull.x = size_full_modified.x = size_auto_fit.x;
            if (!window_size_y_set_by_api)
                window->SizeFull.y = size_full_modified.y = size_auto_fit.y;
        }
        else if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
        {
            // Auto-fit may only grow window during the first few frames
            // We still process initial auto-fit on collapsed windows to get a window width, but otherwise don't honor ImGuiWindowFlags_AlwaysAutoResize when collapsed.
            if (!window_size_x_set_by_api && window->AutoFitFramesX > 0)
                window->SizeFull.x = size_full_modified.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
            if (!window_size_y_set_by_api && window->AutoFitFramesY > 0)
                window->SizeFull.y = size_full_modified.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
            if (!window->Collapsed)
                MarkIniSettingsDirty(window);
        }

        //if (window->DockNode && window->DockIsActive)
        //    size_full_modified = window->SizeFull;

        // Apply minimum/maximum window size constraints and final size
        window->SizeFull = CalcSizeAfterConstraint(window, window->SizeFull);
        window->Size = window->Collapsed && !(flags & ImGuiWindowFlags_ChildWindow) ? window->TitleBarRect().GetSize() : window->SizeFull;

        // SCROLLBAR STATUS

        // Update scrollbar status (based on the Size that was effective during last frame or the auto-resized Size).
        if (!window->Collapsed)
        {
            // When reading the current size we need to read it after size constraints have been applied
            float size_x_for_scrollbars = size_full_modified.x != FLT_MAX ? window->SizeFull.x : window->SizeFullAtLastBegin.x;
            float size_y_for_scrollbars = size_full_modified.y != FLT_MAX ? window->SizeFull.y : window->SizeFullAtLastBegin.y;
            window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((window->SizeContents.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar));
            window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((window->SizeContents.x > size_x_for_scrollbars - (window->ScrollbarY ? style.ScrollbarSize : 0.0f)) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
            if (window->ScrollbarX && !window->ScrollbarY)
                window->ScrollbarY = (window->SizeContents.y > size_y_for_scrollbars - style.ScrollbarSize) && !(flags & ImGuiWindowFlags_NoScrollbar);
            window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);
        }

        // POSITION

        // Popup latch its initial position, will position itself when it appears next frame
        if (window_just_activated_by_user)
        {
            window->AutoPosLastDirection = ImGuiDir_None;
            if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api)
                window->Pos = g.BeginPopupStack.back().OpenPopupPos;
        }

        // Position child window
        if (flags & ImGuiWindowFlags_ChildWindow)
        {
            IM_ASSERT(parent_window->Active);
            window->BeginOrderWithinParent = (short)parent_window->DC.ChildWindows.Size;
            parent_window->DC.ChildWindows.push_back(window);
            if (!(flags & ImGuiWindowFlags_Popup) && !window_pos_set_by_api && !window_is_child_tooltip)
                window->Pos = parent_window->DC.CursorPos;
        }

        const bool window_pos_with_pivot = (window->SetWindowPosVal.x != FLT_MAX && window->HiddenFramesForResize == 0);
        if (window_pos_with_pivot)
            SetWindowPos(window, ImMax(style.DisplaySafeAreaPadding, window->SetWindowPosVal - window->SizeFull * window->SetWindowPosPivot), 0); // Position given a pivot (e.g. for centering)
        else if ((flags & ImGuiWindowFlags_ChildMenu) != 0)
            window->Pos = FindBestWindowPosForPopup(window);
        else if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api && window_just_appearing_after_hidden_for_resize)
            window->Pos = FindBestWindowPosForPopup(window);
        else if ((flags & ImGuiWindowFlags_Tooltip) != 0 && !window_pos_set_by_api && !window_is_child_tooltip)
            window->Pos = FindBestWindowPosForPopup(window);

        if (window->ViewportAllowPlatformMonitorExtend >= 0 && !window->ViewportOwned && !window->Viewport->PlatformWindowMinimized)
            if (!window->Viewport->GetRect().Contains(window->Rect()))
            {
                // Late create viewport, based on the assumption that with our calculations, the DPI will be known ahead (same as the DPI of the selection done in UpdateSelectWindowViewport)
                //ImGuiViewport* old_viewport = window->Viewport;
                window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_NoFocusOnAppearing);

                // FIXME-DPI
                //IM_ASSERT(old_viewport->DpiScale == window->Viewport->DpiScale); // FIXME-DPI: Something went wrong
                SetCurrentViewport(window, window->Viewport);
                window->FontDpiScale = (g.IO.ConfigFlags & ImGuiConfigFlags_DpiEnableScaleFonts) ? window->Viewport->DpiScale : 1.0f;
                SetCurrentWindow(window);
            }

        if (window->ViewportOwned)
        {
            // Synchronize viewport -> window in case the platform window has been moved or resized from the OS/WM
            if (window->Viewport->PlatformRequestMove)
                window->Pos = window->Viewport->Pos;
            if (window->Viewport->PlatformRequestResize)
                window->Size = window->SizeFull = window->Viewport->Size;

            // Update common viewport flags
            ImGuiViewportFlags viewport_flags = (window->Viewport->Flags) & ~(ImGuiViewportFlags_TopMost | ImGuiViewportFlags_NoTaskBarIcon | ImGuiViewportFlags_NoDecoration);
            if (flags & ImGuiWindowFlags_Tooltip)
                viewport_flags |= ImGuiViewportFlags_TopMost;
            if (g.IO.ConfigViewportsNoTaskBarIcon || (flags & (ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup)) != 0)
                viewport_flags |= ImGuiViewportFlags_NoTaskBarIcon;
            if (g.IO.ConfigViewportsNoDecoration || (flags & (ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup)) != 0)
                viewport_flags |= ImGuiViewportFlags_NoDecoration;

            // We can overwrite viewport flags using ImGuiWindowClass (advanced users)
            // We don't default to the main viewport because.
            if (window->WindowClass.ParentViewportId)
                window->Viewport->ParentViewportId = window->WindowClass.ParentViewportId;
            else
                window->Viewport->ParentViewportId = g.IO.ConfigViewportsNoParent ? 0 : IMGUI_VIEWPORT_DEFAULT_ID;
            if (window->WindowClass.ViewportFlagsOverrideMask)
                viewport_flags = (viewport_flags & ~window->WindowClass.ViewportFlagsOverrideMask) | (window->WindowClass.ViewportFlagsOverrideValue & window->WindowClass.ViewportFlagsOverrideMask);

            // We also tell the back-end that clearing the platform window won't be necessary, as our window is filling the viewport and we have disabled BgAlpha
            viewport_flags |= ImGuiViewportFlags_NoRendererClear;
            window->Viewport->Flags = viewport_flags;
        }

        // Clamp position so window stays visible within its viewport or monitor
        // Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
        ImRect viewport_rect = window->Viewport->GetRect();
        if (!window_pos_set_by_api && !(flags & ImGuiWindowFlags_ChildWindow) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
        {
            ImVec2 clamp_padding = ImMax(style.DisplayWindowPadding, style.DisplaySafeAreaPadding);
            if (!window->ViewportOwned && viewport_rect.GetWidth() > 0 && viewport_rect.GetHeight() > 0.0f)
                ClampWindowRect(window, viewport_rect, clamp_padding);
            else if (window->ViewportOwned && g.PlatformIO.Monitors.Size > 0)
            {
                if (window->Viewport->PlatformMonitor == -1)
                {
                    // Fallback for "lost" window (e.g. a monitor disconnected): we move the window back over the main viewport
                    SetWindowPos(window, g.Viewports[0]->Pos + style.DisplayWindowPadding, ImGuiCond_Always);
                }
                else
                {
                    ImGuiPlatformMonitor& monitor = g.PlatformIO.Monitors[window->Viewport->PlatformMonitor];
                    ClampWindowRect(window, ImRect(monitor.WorkPos, monitor.WorkPos + monitor.WorkSize), clamp_padding);
                }
            }
        }
        window->Pos = ImFloor(window->Pos);

        // Lock window rounding for the frame (so that altering them doesn't cause inconsistencies)
        window->WindowRounding = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildRounding : ((flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupRounding : style.WindowRounding;
        if (window->ViewportOwned)
            window->WindowRounding = 0.0f;

        // Prepare for item focus requests
        window->FocusIdxAllRequestCurrent = (window->FocusIdxAllRequestNext == INT_MAX || window->FocusIdxAllCounter == -1) ? INT_MAX : (window->FocusIdxAllRequestNext + (window->FocusIdxAllCounter+1)) % (window->FocusIdxAllCounter+1);
        window->FocusIdxTabRequestCurrent = (window->FocusIdxTabRequestNext == INT_MAX || window->FocusIdxTabCounter == -1) ? INT_MAX : (window->FocusIdxTabRequestNext + (window->FocusIdxTabCounter+1)) % (window->FocusIdxTabCounter+1);
        window->FocusIdxAllCounter = window->FocusIdxTabCounter = -1;
        window->FocusIdxAllRequestNext = window->FocusIdxTabRequestNext = INT_MAX;

        // Apply scrolling
        window->Scroll = CalcNextScrollFromScrollTargetAndClamp(window, true);
        window->ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);

        // Apply window focus (new and reactivated windows are moved to front)
        bool want_focus = false;
        if (window_just_activated_by_user && !(flags & ImGuiWindowFlags_NoFocusOnAppearing))
        {
            if (flags & ImGuiWindowFlags_Popup)
                want_focus = true;
            else if ((window->DockIsActive || !(flags & ImGuiWindowFlags_ChildWindow)) && !(flags & ImGuiWindowFlags_Tooltip))
                want_focus = true;
        }

        // Handle manual resize: Resize Grips, Borders, Gamepad
        int border_held = -1;
        ImU32 resize_grip_col[4] = { 0 };
        const int resize_grip_count = g.IO.ConfigWindowsResizeFromEdges ? 2 : 1; // 4
        const float grip_draw_size = (float)(int)ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f);
        if (!window->Collapsed)
            UpdateManualResize(window, size_auto_fit, &border_held, resize_grip_count, &resize_grip_col[0]);

        // Synchronize window --> viewport
        if (window->ViewportOwned)
        {
            if (!window->Viewport->PlatformRequestMove)
                window->Viewport->Pos = window->Pos;
            if (!window->Viewport->PlatformRequestResize)
                window->Viewport->Size = window->Size;
            viewport_rect = window->Viewport->GetRect();
        }

        // Save last known viewport position within the window itself (so it can be saved in .ini file and restored)
        window->ViewportPos = window->Viewport->Pos;

        // Default item width. Make it proportional to window size if window manually resizes
        if (window->Size.x > 0.0f && !(flags & ImGuiWindowFlags_Tooltip) && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
            window->ItemWidthDefault = (float)(int)(window->Size.x * 0.65f);
        else
            window->ItemWidthDefault = (float)(int)(g.FontSize * 16.0f);

        // DRAWING

        // Setup draw list and outer clipping rectangle
        window->DrawList->Clear();
        window->DrawList->Flags = (g.Style.AntiAliasedLines ? ImDrawListFlags_AntiAliasedLines : 0) | (g.Style.AntiAliasedFill ? ImDrawListFlags_AntiAliasedFill : 0);
        window->DrawList->PushTextureID(g.Font->ContainerAtlas->TexID);
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip)
            PushClipRect(parent_window->ClipRect.Min, parent_window->ClipRect.Max, true);
        else
            PushClipRect(viewport_rect.Min, viewport_rect.Max, true);

        // Draw modal or window list full viewport dimming background (for other viewports we'll render them in EndFrame)
        const bool dim_bg_for_modal = (flags & ImGuiWindowFlags_Modal) && window == GetFrontMostPopupModal() && window->HiddenFramesForResize <= 0;
        const bool dim_bg_for_window_list = g.NavWindowingTargetAnim && ((window == g.NavWindowingTargetAnim->RootWindow) || (g.NavWindowingList && (window == g.NavWindowingList) && g.NavWindowingList->Viewport != g.NavWindowingTargetAnim->Viewport));
        if (dim_bg_for_modal || dim_bg_for_window_list)
        {
            const ImU32 dim_bg_col = GetColorU32(dim_bg_for_modal ? ImGuiCol_ModalWindowDimBg : ImGuiCol_NavWindowingDimBg, g.DimBgRatio);
            window->DrawList->AddRectFilled(viewport_rect.Min, viewport_rect.Max, dim_bg_col);
        }

        // Draw navigation selection/windowing rectangle background
        if (dim_bg_for_window_list && window == g.NavWindowingTargetAnim)
        {
            ImRect bb = window->Rect();
            bb.Expand(g.FontSize);
            if (!bb.Contains(viewport_rect)) // Avoid drawing if the window covers all the viewport anyway
                window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha * 0.25f), g.Style.WindowRounding);
        }

        // Draw window + handle manual resize
        const float window_rounding = window->WindowRounding;
        const float window_border_size = window->WindowBorderSize;
        const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
        const bool title_bar_is_highlight = want_focus || (window_to_highlight && (window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight || (window->DockNode && window->DockNode == window_to_highlight->DockNode)));
        const ImRect title_bar_rect = window->TitleBarRect();
        if (window->Collapsed)
        {
            // Title bar only
            float backup_border_size = style.FrameBorderSize;
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
                bool is_docking_transparent_payload = false;
                if (g.DragDropActive && (g.FrameCount - g.DragDropAcceptFrameCount) <= 1 && g.IO.ConfigDockingTransparentPayload)
                    if (g.DragDropPayload.IsDataType(IMGUI_PAYLOAD_TYPE_WINDOW) && *(ImGuiWindow**)g.DragDropPayload.Data == window)
                        is_docking_transparent_payload = true;

                ImU32 bg_col = GetColorU32(GetWindowBgColorIdxFromFlags(flags));
                if (window->ViewportOwned)
                {
                    // No alpha
                    bg_col = (bg_col | IM_COL32_A_MASK);
                    if (is_docking_transparent_payload)
                        window->Viewport->Alpha *= DOCKING_TRANSPARENT_PAYLOAD_ALPHA;
                }
                else
                {
                    // Adjust alpha. For docking
                    float alpha = 1.0f;
                    if (g.NextWindowData.BgAlphaCond != 0)
                        alpha = g.NextWindowData.BgAlphaVal;
                    if (is_docking_transparent_payload)
                        alpha *= DOCKING_TRANSPARENT_PAYLOAD_ALPHA;
                    if (alpha != 1.0f)
                        bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
                }
                window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Bot);
            }
            g.NextWindowData.BgAlphaCond = 0;

            // Title bar
            // (when docked, DockNode are drawing their own title bar. Individual windows however do NOT set the _NoTitleBar flag, 
            // in order for their pos/size to be matching their undocking state.)
            if (!(flags & ImGuiWindowFlags_NoTitleBar) && !window->DockIsActive)
            {
                ImU32 title_bar_col = GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
                window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, window_rounding, ImDrawCornerFlags_Top);
            }

            // Menu bar
            if (flags & ImGuiWindowFlags_MenuBar)
            {
                ImRect menu_bar_rect = window->MenuBarRect();
                menu_bar_rect.ClipWith(window->Rect());  // Soft clipping, in particular child window don't have minimum size covering the menu bar so this is useful for them.
                window->DrawList->AddRectFilled(menu_bar_rect.Min, menu_bar_rect.Max, GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImDrawCornerFlags_Top);
                if (style.FrameBorderSize > 0.0f && menu_bar_rect.Max.y < window->Pos.y + window->Size.y)
                    window->DrawList->AddLine(menu_bar_rect.GetBL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
            }

            // Docking: Unhide tab bar
            if (window->DockNode && window->DockNode->IsHiddenTabBar)
            {
                float unhide_sz_draw = ImFloor(g.FontSize * 0.70f);
                float unhide_sz_hit = ImFloor(g.FontSize * 0.55f);
                ImVec2 p = window->DockNode->Pos;
                ImRect r(p, p + ImVec2(unhide_sz_hit, unhide_sz_hit));
                bool hovered, held;
                if (ButtonBehavior(r, window->GetID("#UNHIDE"), &hovered, &held, ImGuiButtonFlags_FlattenChildren))
                    window->DockNode->WantHiddenTabBarToggle = true;
                // FIXME-DOCK: Ideally we'd use ImGuiCol_TitleBgActive/ImGuiCol_TitleBg here, but neither is guaranteed to be visible enough at this sort of size..
                ImU32 col = GetColorU32(((held && hovered) || (window->DockNode->IsFocused && !hovered)) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
                window->DrawList->AddTriangleFilled(p, p + ImVec2(unhide_sz_draw, 0.0f), p + ImVec2(0.0f, unhide_sz_draw), col);
            }

            // Scrollbars
            if (window->ScrollbarX)
                Scrollbar(ImGuiLayoutType_Horizontal);
            if (window->ScrollbarY)
                Scrollbar(ImGuiLayoutType_Vertical);

            // Render resize grips (after their input handling so we don't have a frame of latency)
            if (!(flags & ImGuiWindowFlags_NoResize))
            {
                for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
                {
                    const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
                    const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPos);
                    window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(window_border_size, grip_draw_size) : ImVec2(grip_draw_size, window_border_size)));
                    window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(grip_draw_size, window_border_size) : ImVec2(window_border_size, grip_draw_size)));
                    window->DrawList->PathArcToFast(ImVec2(corner.x + grip.InnerDir.x * (window_rounding + window_border_size), corner.y + grip.InnerDir.y * (window_rounding + window_border_size)), window_rounding, grip.AngleMin12, grip.AngleMax12);
                    window->DrawList->PathFillConvex(resize_grip_col[resize_grip_n]);
                }
            }

            // Borders
            if (window_border_size > 0.0f && !(flags & ImGuiWindowFlags_NoBackground))
                window->DrawList->AddRect(window->Pos, window->Pos + window->Size, GetColorU32(ImGuiCol_Border), window_rounding, ImDrawCornerFlags_All, window_border_size);
            if (border_held != -1)
            {
                ImRect border = GetResizeBorderRect(window, border_held, grip_draw_size, 0.0f);
                window->DrawList->AddLine(border.Min, border.Max, GetColorU32(ImGuiCol_SeparatorActive), ImMax(1.0f, window_border_size));
            }
            if (style.FrameBorderSize > 0 && !(flags & ImGuiWindowFlags_NoTitleBar) && !window->DockIsActive)
                window->DrawList->AddLine(title_bar_rect.GetBL() + ImVec2(style.WindowBorderSize, -1), title_bar_rect.GetBR() + ImVec2(-style.WindowBorderSize, -1), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
        }

        // Store a backup of SizeFull which we will use next frame to decide if we need scrollbars.
        window->SizeFullAtLastBegin = window->SizeFull;

        // Update various regions. Variables they depends on are set above in this function.
        // FIXME: window->ContentsRegionRect.Max is currently very misleading / partly faulty, but some BeginChild() patterns relies on it.
        window->ContentsRegionRect.Min.x = window->Pos.x - window->Scroll.x + window->WindowPadding.x;
        window->ContentsRegionRect.Min.y = window->Pos.y - window->Scroll.y + window->WindowPadding.y + window->TitleBarHeight() + window->MenuBarHeight();
        window->ContentsRegionRect.Max.x = window->Pos.x - window->Scroll.x - window->WindowPadding.x + (window->SizeContentsExplicit.x != 0.0f ? window->SizeContentsExplicit.x : (window->Size.x - window->ScrollbarSizes.x));
        window->ContentsRegionRect.Max.y = window->Pos.y - window->Scroll.y - window->WindowPadding.y + (window->SizeContentsExplicit.y != 0.0f ? window->SizeContentsExplicit.y : (window->Size.y - window->ScrollbarSizes.y));

        // Setup drawing context
        // (NB: That term "drawing context / DC" lost its meaning a long time ago. Initially was meant to hold transient data only. Nowadays difference between window-> and window->DC-> is dubious.)
        window->DC.Indent.x = 0.0f + window->WindowPadding.x - window->Scroll.x;
        window->DC.GroupOffset.x = 0.0f;
        window->DC.ColumnsOffset.x = 0.0f;
        window->DC.CursorStartPos = window->Pos + ImVec2(window->DC.Indent.x + window->DC.ColumnsOffset.x, window->TitleBarHeight() + window->MenuBarHeight() + window->WindowPadding.y - window->Scroll.y);
        window->DC.CursorPos = window->DC.CursorStartPos;
        window->DC.CursorPosPrevLine = window->DC.CursorPos;
        window->DC.CursorMaxPos = window->DC.CursorStartPos;
        window->DC.CurrentLineSize = window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
        window->DC.CurrentLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
        window->DC.NavHideHighlightOneFrame = false;
        window->DC.NavHasScroll = (GetWindowScrollMaxY(window) > 0.0f);
        window->DC.NavLayerActiveMask = window->DC.NavLayerActiveMaskNext;
        window->DC.NavLayerActiveMaskNext = 0x00;
        window->DC.MenuBarAppending = false;
        window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f;
        window->DC.ChildWindows.resize(0);
        window->DC.LayoutType = ImGuiLayoutType_Vertical;
        window->DC.ParentLayoutType = parent_window ? parent_window->DC.LayoutType : ImGuiLayoutType_Vertical;
        window->DC.ItemFlags = parent_window ? parent_window->DC.ItemFlags : ImGuiItemFlags_Default_;
        window->DC.ItemWidth = window->ItemWidthDefault;
        window->DC.TextWrapPos = -1.0f; // disabled
        window->DC.ItemFlagsStack.resize(0);
        window->DC.ItemWidthStack.resize(0);
        window->DC.TextWrapPosStack.resize(0);
        window->DC.ColumnsSet = NULL;
        window->DC.TreeDepth = 0;
        window->DC.TreeDepthMayJumpToParentOnPop = 0x00;
        window->DC.StateStorage = &window->StateStorage;
        window->DC.GroupStack.resize(0);
        window->MenuColumns.Update(3, style.ItemSpacing.x, window_just_activated_by_user);

        if ((flags & ImGuiWindowFlags_ChildWindow) && (window->DC.ItemFlags != parent_window->DC.ItemFlags))
        {
            window->DC.ItemFlags = parent_window->DC.ItemFlags;
            window->DC.ItemFlagsStack.push_back(window->DC.ItemFlags);
        }

        if (window->AutoFitFramesX > 0)
            window->AutoFitFramesX--;
        if (window->AutoFitFramesY > 0)
            window->AutoFitFramesY--;

        // Apply focus (we need to call FocusWindow() AFTER setting DC.CursorStartPos so our initial navigation reference rectangle can start around there)
        if (want_focus)
        {
            FocusWindow(window);
            NavInitWindow(window, false);
        }

        // Close from platform window
        if (p_open != NULL && window->Viewport->PlatformRequestClose && window->Viewport != GetMainViewport())
        {
            if (!window->DockIsActive || window->DockTabIsVisible)
            {
                window->Viewport->PlatformRequestClose = false;
                g.NavWindowingToggleLayer = false; // Assume user mapped PlatformRequestClose on ALT-F4 so we disable ALT for menu toggle. False positive not an issue.
                //IMGUI_DEBUG_LOG("Window '%s' PlatformRequestClose\n", window->Name);
                *p_open = false;
            }
        }

        // Title bar
        if (!(flags & ImGuiWindowFlags_NoTitleBar) && !window->DockIsActive)
        {
            // Close & collapse button are on layer 1 (same as menus) and don't default focus
            const ImGuiItemFlags item_flags_backup = window->DC.ItemFlags;
            window->DC.ItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
            window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
            window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Menu);

            // Collapse button
            if (!(flags & ImGuiWindowFlags_NoCollapse))
                if (CollapseButton(window->GetID("#COLLAPSE"), window->Pos, NULL))
                    window->WantCollapseToggle = true; // Defer collapsing to next frame as we are too far in the Begin() function

            // Close button
            if (p_open != NULL)
            {
                const float rad = g.FontSize * 0.5f;
                if (CloseButton(window->GetID("#CLOSE"), ImVec2(window->Pos.x + window->Size.x - style.FramePadding.x - rad, window->Pos.y + style.FramePadding.y + rad), rad + 1))
                    *p_open = false;
            }

            window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
            window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Main);
            window->DC.ItemFlags = item_flags_backup;

            // Title bar text (with: horizontal alignment, avoiding collapse/close button, optional "unsaved document" marker)
            // FIXME: Refactor text alignment facilities along with RenderText helpers, this is too much code..
            const char* UNSAVED_DOCUMENT_MARKER = "*";
            float marker_size_x = (flags & ImGuiWindowFlags_UnsavedDocument) ? CalcTextSize(UNSAVED_DOCUMENT_MARKER, NULL, false).x : 0.0f;
            ImVec2 text_size = CalcTextSize(name, NULL, true) + ImVec2(marker_size_x, 0.0f);
            ImRect text_r = title_bar_rect;
            float pad_left = (flags & ImGuiWindowFlags_NoCollapse) ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
            float pad_right = (p_open == NULL)                     ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
            if (style.WindowTitleAlign.x > 0.0f)
                pad_right = ImLerp(pad_right, pad_left, style.WindowTitleAlign.x);
            text_r.Min.x += pad_left;
            text_r.Max.x -= pad_right;
            ImRect clip_rect = text_r;
            clip_rect.Max.x = window->Pos.x + window->Size.x - (p_open ? title_bar_rect.GetHeight() - 3 : style.FramePadding.x); // Match the size of CloseButton()
            RenderTextClipped(text_r.Min, text_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_rect);
            if (flags & ImGuiWindowFlags_UnsavedDocument)
            {
                ImVec2 marker_pos = ImVec2(ImMax(text_r.Min.x, text_r.Min.x + (text_r.GetWidth() - text_size.x) * style.WindowTitleAlign.x) + text_size.x, text_r.Min.y) + ImVec2(2 - marker_size_x, 0.0f);
                ImVec2 off = ImVec2(0.0f, (float)(int)(-g.FontSize * 0.25f));
                RenderTextClipped(marker_pos + off, text_r.Max + off, UNSAVED_DOCUMENT_MARKER, NULL, NULL, ImVec2(0, style.WindowTitleAlign.y), &clip_rect);
            }
        }

        // Clear hit test shape every frame
        window->HitTestHoleSize.x = window->HitTestHoleSize.y = 0;

        // Save clipped aabb so we can access it in constant-time in FindHoveredWindow()
        window->OuterRectClipped = window->Rect();
        if (window->DockIsActive)
            window->OuterRectClipped.Min.y += window->TitleBarHeight();
        window->OuterRectClipped.ClipWith(window->ClipRect);

        // Pressing CTRL+C while holding on a window copy its content to the clipboard
        // This works but 1. doesn't handle multiple Begin/End pairs, 2. recursing into another Begin/End pair - so we need to work that out and add better logging scope.
        // Maybe we can support CTRL+C on every element?
        /*
        if (g.ActiveId == move_id)
            if (g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_C))
                LogToClipboard();
        */

        // Inner rectangle
        // We set this up after processing the resize grip so that our clip rectangle doesn't lag by a frame
        // Note that if our window is collapsed we will end up with an inverted (~null) clipping rectangle which is the correct behavior.
        window->InnerMainRect.Min.x = title_bar_rect.Min.x + window->WindowBorderSize;
        window->InnerMainRect.Min.y = title_bar_rect.Max.y + window->MenuBarHeight() + (((flags & ImGuiWindowFlags_MenuBar) || !(flags & ImGuiWindowFlags_NoTitleBar)) ? style.FrameBorderSize : window->WindowBorderSize);
        window->InnerMainRect.Max.x = window->Pos.x + window->Size.x - window->ScrollbarSizes.x - window->WindowBorderSize;
        window->InnerMainRect.Max.y = window->Pos.y + window->Size.y - window->ScrollbarSizes.y - window->WindowBorderSize;
        //window->DrawList->AddRect(window->InnerRect.Min, window->InnerRect.Max, IM_COL32_WHITE);

        // Inner clipping rectangle
        // Force round operator last to ensure that e.g. (int)(max.x-min.x) in user's render code produce correct result.
        window->InnerClipRect.Min.x = ImFloor(0.5f + window->InnerMainRect.Min.x + ImMax(0.0f, ImFloor(window->WindowPadding.x*0.5f - window->WindowBorderSize)));
        window->InnerClipRect.Min.y = ImFloor(0.5f + window->InnerMainRect.Min.y);
        window->InnerClipRect.Max.x = ImFloor(0.5f + window->InnerMainRect.Max.x - ImMax(0.0f, ImFloor(window->WindowPadding.x*0.5f - window->WindowBorderSize)));
        window->InnerClipRect.Max.y = ImFloor(0.5f + window->InnerMainRect.Max.y);

        if (g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            // Docking: Dragging a dockable window (or any of its child) turns it into a drag and drop source.
            // We need to do this _before_ we overwrite window->DC.LastItemId below because BeginAsDockableDragDropSource() also overwrites it.
            if ((g.ActiveId == window->MoveId) && ((g.IO.ConfigDockingWithShift && g.IO.KeyShift) || (!g.IO.ConfigDockingWithShift)))
                if ((window->RootWindow->Flags & (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking)) == 0)
                    BeginAsDockableDragDropSource(window);

            // Docking: Any dockable window can act as a target. For dock node hosts we call BeginAsDockableDragDropTarget() in DockNodeUpdate() instead.
            if (g.DragDropActive && !(flags & ImGuiWindowFlags_NoDocking))
                if (g.MovingWindow == NULL || g.MovingWindow->RootWindow != window)
                    if ((window == window->RootWindow) && !(window->Flags & ImGuiWindowFlags_DockNodeHost))
                        BeginAsDockableDragDropTarget(window);
        }

        // We fill last item data based on Title Bar or Tab, in order for IsItemHovered() and IsItemActive() to be usable after Begin().
        // This is useful to allow creating context menus on title bar only, etc.
        if (window->DockIsActive)
        {
            window->DC.LastItemId = window->ID;
            window->DC.LastItemStatusFlags = window->DockTabItemStatusFlags;
            window->DC.LastItemRect = window->DockTabItemRect;
        }
        else
        {
            window->DC.LastItemId = window->MoveId;
            window->DC.LastItemStatusFlags = IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max, false) ? ImGuiItemStatusFlags_HoveredRect : 0;
            window->DC.LastItemRect = title_bar_rect;
        }
    }
    else
    {
        // Append
        SetCurrentViewport(window, window->Viewport);
        SetCurrentWindow(window);
    }

    if (!(flags & ImGuiWindowFlags_DockNodeHost))
        PushClipRect(window->InnerClipRect.Min, window->InnerClipRect.Max, true);

    // Clear 'accessed' flag last thing (After PushClipRect which will set the flag. We want the flag to stay false when the default "Debug" window is unused)
    if (first_begin_of_the_frame)
        window->WriteAccessed = false;

    window->BeginCount++;
    g.NextWindowData.Clear();

    if (window->DockIsActive && !window->DockTabIsVisible)
        window->HiddenFramesRegular = 1;

    if (flags & ImGuiWindowFlags_ChildWindow)
    {
        // Child window can be out of sight and have "negative" clip windows.
        // Mark them as collapsed so commands are skipped earlier (we can't manually collapse them because they have no title bar).
        IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0 || (window->DockIsActive));
        if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
            if (window->OuterRectClipped.Min.x >= window->OuterRectClipped.Max.x || window->OuterRectClipped.Min.y >= window->OuterRectClipped.Max.y)
                window->HiddenFramesRegular = 1;

        // Completely hide along with parent or if parent is collapsed
        if (parent_window && (parent_window->Collapsed || parent_window->Hidden))
            window->HiddenFramesRegular = 1;
    }

    // Don't render if style alpha is 0.0 at the time of Begin(). This is arbitrary and inconsistent but has been there for a long while (may remove at some point)
    if (style.Alpha <= 0.0f)
        window->HiddenFramesRegular = 1;

    // Update the Hidden flag
    window->Hidden = (window->HiddenFramesRegular > 0) || (window->HiddenFramesForResize > 0);

    // Return false if we don't intend to display anything to allow user to perform an early out optimization
    window->SkipItems = (window->Collapsed || !window->Active || window->Hidden) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && window->HiddenFramesForResize <= 0;

    return !window->SkipItems;
}

// Old Begin() API with 5 parameters, avoid calling this version directly! Use SetNextWindowSize()/SetNextWindowBgAlpha() + Begin() instead.
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
bool ImGui::Begin(const char* name, bool* p_open, const ImVec2& size_first_use, float bg_alpha_override, ImGuiWindowFlags flags)
{
    // Old API feature: we could pass the initial window size as a parameter. This was misleading because it only had an effect if the window didn't have data in the .ini file.
    if (size_first_use.x != 0.0f || size_first_use.y != 0.0f)
        SetNextWindowSize(size_first_use, ImGuiCond_FirstUseEver);

    // Old API feature: override the window background alpha with a parameter.
    if (bg_alpha_override >= 0.0f)
        SetNextWindowBgAlpha(bg_alpha_override);

    return Begin(name, p_open, flags);
}
#endif // IMGUI_DISABLE_OBSOLETE_FUNCTIONS

void ImGui::End()
{
    ImGuiContext& g = *GImGui;

    if (g.CurrentWindowStack.Size <= 1 && g.FrameScopePushedImplicitWindow)
    {
        IM_ASSERT(g.CurrentWindowStack.Size > 1 && "Calling End() too many times!");
        return; // FIXME-ERRORHANDLING
    }
    IM_ASSERT(g.CurrentWindowStack.Size > 0);

    ImGuiWindow* window = g.CurrentWindow;

    if (window->DC.ColumnsSet != NULL)
        EndColumns();
    if (!(window->Flags & ImGuiWindowFlags_DockNodeHost))   // Pop inner window clip rectangle
        PopClipRect();

    // Stop logging
    if (!(window->Flags & ImGuiWindowFlags_ChildWindow))    // FIXME: add more options for scope of logging
        LogFinish();

    // Docking: report contents sizes to parent to allow for auto-resize
    if (window->DockNode && window->DockTabIsVisible)
        if (ImGuiWindow* host_window = window->DockNode->HostWindow)         // FIXME-DOCK
            host_window->DC.CursorMaxPos = window->DC.CursorMaxPos + window->WindowPadding - host_window->WindowPadding;

    // Pop from window stack
    g.CurrentWindowStack.pop_back();
    if (window->Flags & ImGuiWindowFlags_Popup)
        g.BeginPopupStack.pop_back();
    CheckStacksSize(window, false);
    SetCurrentWindow(g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back());
    if (g.CurrentWindow)
        SetCurrentViewport(g.CurrentWindow, g.CurrentWindow->Viewport);
}

void ImGui::BringWindowToFocusFront(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.WindowsFocusOrder.back() == window)
        return;
    for (int i = g.WindowsFocusOrder.Size - 2; i >= 0; i--) // We can ignore the front most window
        if (g.WindowsFocusOrder[i] == window)
        {
            memmove(&g.WindowsFocusOrder[i], &g.WindowsFocusOrder[i + 1], (size_t)(g.WindowsFocusOrder.Size - i - 1) * sizeof(ImGuiWindow*));
            g.WindowsFocusOrder[g.WindowsFocusOrder.Size - 1] = window;
            break;
        }
}

void ImGui::BringWindowToDisplayFront(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* current_front_window = g.Windows.back();
    if (current_front_window == window || current_front_window->RootWindow == window)
        return;
    for (int i = g.Windows.Size - 2; i >= 0; i--) // We can ignore the front most window
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

// Moving window to front of display and set focus (which happens to be back of our sorted list)
void ImGui::FocusWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;

    if (g.NavWindow != window)
    {
        g.NavWindow = window;
        if (window && g.NavDisableMouseHover)
            g.NavMousePosDirty = true;
        g.NavInitRequest = false;
        g.NavId = window ? window->NavLastIds[0] : 0; // Restore NavId
        g.NavIdIsAlive = false;
        g.NavLayer = ImGuiNavLayer_Main;
        //IMGUI_DEBUG_LOG("FocusWindow(\"%s\")\n", window ? window->Name : NULL);
    }

    // Passing NULL allow to disable keyboard focus
    if (!window)
        return;

    // Select in dock node
    if (window->DockNode && window->DockNode->TabBar)
        window->DockNode->TabBar->SelectedTabId = window->DockNode->TabBar->NextSelectedTabId = window->ID;

    // Move the root window to the top of the pile
    if (window->RootWindow)
        window = window->RootWindow;

    // Steal focus on active widgets
    if (window->Flags & ImGuiWindowFlags_Popup) // FIXME: This statement should be unnecessary. Need further testing before removing it..
        if (g.ActiveId != 0 && g.ActiveIdWindow && g.ActiveIdWindow->RootWindow != window)
            ClearActiveID();

    // Bring to front
    BringWindowToFocusFront(window);
    if (!(window->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus))
        BringWindowToDisplayFront(window);
}

void ImGui::FocusPreviousWindowIgnoringOne(ImGuiWindow* ignore_window)
{
    ImGuiContext& g = *GImGui;
    for (int i = g.WindowsFocusOrder.Size - 1; i >= 0; i--)
    {
        // We may later decide to test for different NoXXXInputs based on the active navigation input (mouse vs nav) but that may feel more confusing to the user.
        ImGuiWindow* window = g.WindowsFocusOrder[i];
        if (window != ignore_window && window->WasActive && !(window->Flags & ImGuiWindowFlags_ChildWindow))
            if ((window->Flags & (ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs)) != (ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs))
            {
                ImGuiWindow* focus_window = NavRestoreLastChildNavWindow(window);
                FocusWindow(focus_window);
                return;
            }
    }
}

void ImGui::PushItemWidth(float item_width)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ItemWidth = (item_width == 0.0f ? window->ItemWidthDefault : item_width);
    window->DC.ItemWidthStack.push_back(window->DC.ItemWidth);
}

void ImGui::PushMultiItemsWidths(int components, float w_full)
{
    ImGuiWindow* window = GetCurrentWindow();
    const ImGuiStyle& style = GImGui->Style;
    if (w_full <= 0.0f)
        w_full = CalcItemWidth();
    const float w_item_one  = ImMax(1.0f, (float)(int)((w_full - (style.ItemInnerSpacing.x) * (components-1)) / (float)components));
    const float w_item_last = ImMax(1.0f, (float)(int)(w_full - (w_item_one + style.ItemInnerSpacing.x) * (components-1)));
    window->DC.ItemWidthStack.push_back(w_item_last);
    for (int i = 0; i < components-1; i++)
        window->DC.ItemWidthStack.push_back(w_item_one);
    window->DC.ItemWidth = window->DC.ItemWidthStack.back();
}

void ImGui::PopItemWidth()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ItemWidthStack.pop_back();
    window->DC.ItemWidth = window->DC.ItemWidthStack.empty() ? window->ItemWidthDefault : window->DC.ItemWidthStack.back();
}

float ImGui::CalcItemWidth()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    float w = window->DC.ItemWidth;
    if (w < 0.0f)
    {
        // Align to a right-side limit. We include 1 frame padding in the calculation because this is how the width is always used (we add 2 frame padding to it), but we could move that responsibility to the widget as well.
        float width_to_right_edge = GetContentRegionAvail().x;
        w = ImMax(1.0f, width_to_right_edge + w);
    }
    w = (float)(int)w;
    return w;
}

void ImGui::SetCurrentFont(ImFont* font)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(font && font->IsLoaded());    // Font Atlas not created. Did you call io.Fonts->GetTexDataAsRGBA32 / GetTexDataAsAlpha8 ?
    IM_ASSERT(font->Scale > 0.0f);
    g.Font = font;
    g.FontBaseSize = g.IO.FontGlobalScale * g.Font->FontSize * g.Font->Scale;
    g.FontSize = g.CurrentWindow ? g.CurrentWindow->CalcFontSize() : 0.0f;

    ImFontAtlas* atlas = g.Font->ContainerAtlas;
    g.DrawListSharedData.TexUvWhitePixel = atlas->TexUvWhitePixel;
    g.DrawListSharedData.Font = g.Font;
    g.DrawListSharedData.FontSize = g.FontSize;
}

void ImGui::PushFont(ImFont* font)
{
    ImGuiContext& g = *GImGui;
    if (!font)
        font = GetDefaultFont();
    SetCurrentFont(font);
    g.FontStack.push_back(font);
    g.CurrentWindow->DrawList->PushTextureID(font->ContainerAtlas->TexID);
}

void  ImGui::PopFont()
{
    ImGuiContext& g = *GImGui;
    g.CurrentWindow->DrawList->PopTextureID();
    g.FontStack.pop_back();
    SetCurrentFont(g.FontStack.empty() ? GetDefaultFont() : g.FontStack.back());
}

void ImGui::PushItemFlag(ImGuiItemFlags option, bool enabled)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (enabled)
        window->DC.ItemFlags |= option;
    else
        window->DC.ItemFlags &= ~option;
    window->DC.ItemFlagsStack.push_back(window->DC.ItemFlags);
}

void ImGui::PopItemFlag()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ItemFlagsStack.pop_back();
    window->DC.ItemFlags = window->DC.ItemFlagsStack.empty() ? ImGuiItemFlags_Default_ : window->DC.ItemFlagsStack.back();
}

// FIXME: Look into renaming this once we have settled the new Focus/Activation/TabStop system.
void ImGui::PushAllowKeyboardFocus(bool allow_keyboard_focus)
{
    PushItemFlag(ImGuiItemFlags_NoTabStop, !allow_keyboard_focus);
}

void ImGui::PopAllowKeyboardFocus()
{
    PopItemFlag();
}

void ImGui::PushButtonRepeat(bool repeat)
{
    PushItemFlag(ImGuiItemFlags_ButtonRepeat, repeat);
}

void ImGui::PopButtonRepeat()
{
    PopItemFlag();
}

void ImGui::PushTextWrapPos(float wrap_pos_x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.TextWrapPos = wrap_pos_x;
    window->DC.TextWrapPosStack.push_back(wrap_pos_x);
}

void ImGui::PopTextWrapPos()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.TextWrapPosStack.pop_back();
    window->DC.TextWrapPos = window->DC.TextWrapPosStack.empty() ? -1.0f : window->DC.TextWrapPosStack.back();
}

// FIXME: This may incur a round-trip (if the end user got their data from a float4) but eventually we aim to store the in-flight colors as ImU32
void ImGui::PushStyleColor(ImGuiCol idx, ImU32 col)
{
    ImGuiContext& g = *GImGui;
    ImGuiColorMod backup;
    backup.Col = idx;
    backup.BackupValue = g.Style.Colors[idx];
    g.ColorModifiers.push_back(backup);
    g.Style.Colors[idx] = ColorConvertU32ToFloat4(col);
}

void ImGui::PushStyleColor(ImGuiCol idx, const ImVec4& col)
{
    ImGuiContext& g = *GImGui;
    ImGuiColorMod backup;
    backup.Col = idx;
    backup.BackupValue = g.Style.Colors[idx];
    g.ColorModifiers.push_back(backup);
    g.Style.Colors[idx] = col;
}

void ImGui::PopStyleColor(int count)
{
    ImGuiContext& g = *GImGui;
    while (count > 0)
    {
        ImGuiColorMod& backup = g.ColorModifiers.back();
        g.Style.Colors[backup.Col] = backup.BackupValue;
        g.ColorModifiers.pop_back();
        count--;
    }
}

struct ImGuiStyleVarInfo
{
    ImGuiDataType   Type;
    ImU32           Count;
    ImU32           Offset;
    void*           GetVarPtr(ImGuiStyle* style) const { return (void*)((unsigned char*)style + Offset); }
};

static const ImGuiStyleVarInfo GStyleVarInfo[] =
{
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, Alpha) },              // ImGuiStyleVar_Alpha
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowPadding) },      // ImGuiStyleVar_WindowPadding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowRounding) },     // ImGuiStyleVar_WindowRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowBorderSize) },   // ImGuiStyleVar_WindowBorderSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowMinSize) },      // ImGuiStyleVar_WindowMinSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowTitleAlign) },   // ImGuiStyleVar_WindowTitleAlign
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ChildRounding) },      // ImGuiStyleVar_ChildRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ChildBorderSize) },    // ImGuiStyleVar_ChildBorderSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, PopupRounding) },      // ImGuiStyleVar_PopupRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, PopupBorderSize) },    // ImGuiStyleVar_PopupBorderSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, FramePadding) },       // ImGuiStyleVar_FramePadding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, FrameRounding) },      // ImGuiStyleVar_FrameRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, FrameBorderSize) },    // ImGuiStyleVar_FrameBorderSize
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ItemSpacing) },        // ImGuiStyleVar_ItemSpacing
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ItemInnerSpacing) },   // ImGuiStyleVar_ItemInnerSpacing
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, IndentSpacing) },      // ImGuiStyleVar_IndentSpacing
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ScrollbarSize) },      // ImGuiStyleVar_ScrollbarSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ScrollbarRounding) },  // ImGuiStyleVar_ScrollbarRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, GrabMinSize) },        // ImGuiStyleVar_GrabMinSize
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, GrabRounding) },       // ImGuiStyleVar_GrabRounding
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, TabRounding) },        // ImGuiStyleVar_TabRounding
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ButtonTextAlign) },    // ImGuiStyleVar_ButtonTextAlign
};

static const ImGuiStyleVarInfo* GetStyleVarInfo(ImGuiStyleVar idx)
{
    IM_ASSERT(idx >= 0 && idx < ImGuiStyleVar_COUNT);
    IM_ASSERT(IM_ARRAYSIZE(GStyleVarInfo) == ImGuiStyleVar_COUNT);
    return &GStyleVarInfo[idx];
}

void ImGui::PushStyleVar(ImGuiStyleVar idx, float val)
{
    const ImGuiStyleVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1)
    {
        ImGuiContext& g = *GImGui;
        float* pvar = (float*)var_info->GetVarPtr(&g.Style);
        g.StyleModifiers.push_back(ImGuiStyleMod(idx, *pvar));
        *pvar = val;
        return;
    }
    IM_ASSERT(0); // Called function with wrong-type? Variable is not a float.
}

void ImGui::PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
{
    const ImGuiStyleVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 2)
    {
        ImGuiContext& g = *GImGui;
        ImVec2* pvar = (ImVec2*)var_info->GetVarPtr(&g.Style);
        g.StyleModifiers.push_back(ImGuiStyleMod(idx, *pvar));
        *pvar = val;
        return;
    }
    IM_ASSERT(0); // Called function with wrong-type? Variable is not a ImVec2.
}

void ImGui::PopStyleVar(int count)
{
    ImGuiContext& g = *GImGui;
    while (count > 0)
    {
        // We avoid a generic memcpy(data, &backup.Backup.., GDataTypeSize[info->Type] * info->Count), the overhead in Debug is not worth it.
        ImGuiStyleMod& backup = g.StyleModifiers.back();
        const ImGuiStyleVarInfo* info = GetStyleVarInfo(backup.VarIdx);
        void* data = info->GetVarPtr(&g.Style);
        if (info->Type == ImGuiDataType_Float && info->Count == 1)      { ((float*)data)[0] = backup.BackupFloat[0]; }
        else if (info->Type == ImGuiDataType_Float && info->Count == 2) { ((float*)data)[0] = backup.BackupFloat[0]; ((float*)data)[1] = backup.BackupFloat[1]; }
        g.StyleModifiers.pop_back();
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
    case ImGuiCol_Tab: return "Tab";
    case ImGuiCol_TabHovered: return "TabHovered";
    case ImGuiCol_TabActive: return "TabActive";
    case ImGuiCol_TabUnfocused: return "TabUnfocused";
    case ImGuiCol_TabUnfocusedActive: return "TabUnfocusedActive";
    case ImGuiCol_DockingPreview: return "DockingPreview";
    case ImGuiCol_DockingEmptyBg: return "DockingEmptyBg";
    case ImGuiCol_PlotLines: return "PlotLines";
    case ImGuiCol_PlotLinesHovered: return "PlotLinesHovered";
    case ImGuiCol_PlotHistogram: return "PlotHistogram";
    case ImGuiCol_PlotHistogramHovered: return "PlotHistogramHovered";
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

bool ImGui::IsWindowChildOf(ImGuiWindow* window, ImGuiWindow* potential_parent)
{
    if (window->RootWindow == potential_parent)
        return true;
    while (window != NULL)
    {
        if (window == potential_parent)
            return true;
        window = window->ParentWindow;
    }
    return false;
}

bool ImGui::IsWindowHovered(ImGuiHoveredFlags flags)
{
    IM_ASSERT((flags & ImGuiHoveredFlags_AllowWhenOverlapped) == 0);   // Flags not supported by this function
    ImGuiContext& g = *GImGui;

    if (flags & ImGuiHoveredFlags_AnyWindow)
    {
        if (g.HoveredWindow == NULL)
            return false;
    }
    else
    {
        switch (flags & (ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows))
        {
        case ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows:
            if (g.HoveredWindow == NULL || g.HoveredWindow->RootWindowDockStop != g.CurrentWindow->RootWindowDockStop)
                return false;
            break;
        case ImGuiHoveredFlags_RootWindow:
            if (g.HoveredWindow != g.CurrentWindow->RootWindowDockStop)
                return false;
            break;
        case ImGuiHoveredFlags_ChildWindows:
            if (g.HoveredWindow == NULL || !IsWindowChildOf(g.HoveredWindow, g.CurrentWindow))
                return false;
            break;
        default:
            if (g.HoveredWindow != g.CurrentWindow)
                return false;
            break;
        }
    }

    if (!IsWindowContentHoverable(g.HoveredRootWindow, flags))
        return false;
    if (!(flags & ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        if (g.ActiveId != 0 && !g.ActiveIdAllowOverlap && g.ActiveId != g.HoveredWindow->MoveId)
            return false;
    return true;
}

bool ImGui::IsWindowFocused(ImGuiFocusedFlags flags)
{
    ImGuiContext& g = *GImGui;

    if (flags & ImGuiFocusedFlags_AnyWindow)
        return g.NavWindow != NULL;

    IM_ASSERT(g.CurrentWindow);     // Not inside a Begin()/End()
    switch (flags & (ImGuiFocusedFlags_RootWindow | ImGuiFocusedFlags_ChildWindows))
    {
    case ImGuiFocusedFlags_RootWindow | ImGuiFocusedFlags_ChildWindows:
        return g.NavWindow && g.NavWindow->RootWindowDockStop == g.CurrentWindow->RootWindowDockStop;
    case ImGuiFocusedFlags_RootWindow:
        return g.NavWindow == g.CurrentWindow->RootWindowDockStop;
    case ImGuiFocusedFlags_ChildWindows:
        return g.NavWindow && IsWindowChildOf(g.NavWindow, g.CurrentWindow);
    default:
        return g.NavWindow == g.CurrentWindow;
    }
}

ImGuiID ImGui::GetWindowDockID()
{
    ImGuiContext& g = *GImGui;
    return g.CurrentWindow->DockId;
}

bool ImGui::IsWindowDocked()
{
    ImGuiContext& g = *GImGui;
    return g.CurrentWindow->DockIsActive;
}

// Can we focus this window with CTRL+TAB (or PadMenu + PadFocusPrev/PadFocusNext)
// Note that NoNavFocus makes the window not reachable with CTRL+TAB but it can still be focused with mouse or programmaticaly.
// If you want a window to never be focused, you may use the e.g. NoInputs flag.
bool ImGui::IsWindowNavFocusable(ImGuiWindow* window)
{
    return window->Active && window == window->RootWindowDockStop && !(window->Flags & ImGuiWindowFlags_NoNavFocus);
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

void ImGui::SetWindowScrollX(ImGuiWindow* window, float new_scroll_x)
{
    window->DC.CursorMaxPos.x += window->Scroll.x; // SizeContents is generally computed based on CursorMaxPos which is affected by scroll position, so we need to apply our change to it.
    window->Scroll.x = new_scroll_x;
    window->DC.CursorMaxPos.x -= window->Scroll.x;
}

void ImGui::SetWindowScrollY(ImGuiWindow* window, float new_scroll_y)
{
    window->DC.CursorMaxPos.y += window->Scroll.y; // SizeContents is generally computed based on CursorMaxPos which is affected by scroll position, so we need to apply our change to it.
    window->Scroll.y = new_scroll_y;
    window->DC.CursorMaxPos.y -= window->Scroll.y;
}

static void SetWindowPos(ImGuiWindow* window, const ImVec2& pos, ImGuiCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowPosAllowFlags & cond) == 0)
        return;

    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
    window->SetWindowPosVal = ImVec2(FLT_MAX, FLT_MAX);

    // Set
    const ImVec2 old_pos = window->Pos;
    window->Pos = ImFloor(pos);
    window->DC.CursorPos += (window->Pos - old_pos);    // As we happen to move the window while it is being appended to (which is a bad idea - will smear) let's at least offset the cursor
    window->DC.CursorMaxPos += (window->Pos - old_pos); // And more importantly we need to adjust this so size calculation doesn't get affected.
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

static void SetWindowSize(ImGuiWindow* window, const ImVec2& size, ImGuiCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowSizeAllowFlags & cond) == 0)
        return;

    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    window->SetWindowSizeAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);

    // Set
    if (size.x > 0.0f)
    {
        window->AutoFitFramesX = 0;
        window->SizeFull.x = ImFloor(size.x);
    }
    else
    {
        window->AutoFitFramesX = 2;
        window->AutoFitOnlyGrows = false;
    }
    if (size.y > 0.0f)
    {
        window->AutoFitFramesY = 0;
        window->SizeFull.y = ImFloor(size.y);
    }
    else
    {
        window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = false;
    }
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

static void SetWindowCollapsed(ImGuiWindow* window, bool collapsed, ImGuiCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowCollapsedAllowFlags & cond) == 0)
        return;
    window->SetWindowCollapsedAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);

    // Set
    window->Collapsed = collapsed;
}

static void SetWindowHitTestHole(ImGuiWindow* window, const ImVec2& pos, const ImVec2& size)
{
    IM_ASSERT(window->HitTestHoleSize.x == 0);     // We don't support multiple holes/hit test filters
    window->HitTestHoleSize = ImVec2ih((short)size.x, (short)size.y);
    window->HitTestHoleOffset = ImVec2ih((short)(pos.x - window->Pos.x), (short)(pos.y - window->Pos.y));
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
    g.NextWindowData.PosVal = pos;
    g.NextWindowData.PosPivotVal = pivot;
    g.NextWindowData.PosCond = cond ? cond : ImGuiCond_Always;
    g.NextWindowData.PosUndock = true;
}

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
void ImGui::SetNextWindowPosCenter(ImGuiCond cond) 
{ 
    ImGuiViewport* viewport = ImGui::GetMainViewport(); 
    SetNextWindowPos(viewport->Pos + viewport->Size * 0.5f, cond, ImVec2(0.5f, 0.5f)); 
    SetNextWindowViewport(viewport->ID); 
}
#endif

void ImGui::SetNextWindowSize(const ImVec2& size, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    g.NextWindowData.SizeVal = size;
    g.NextWindowData.SizeCond = cond ? cond : ImGuiCond_Always;
}

void ImGui::SetNextWindowSizeConstraints(const ImVec2& size_min, const ImVec2& size_max, ImGuiSizeCallback custom_callback, void* custom_callback_user_data)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.SizeConstraintCond = ImGuiCond_Always;
    g.NextWindowData.SizeConstraintRect = ImRect(size_min, size_max);
    g.NextWindowData.SizeCallback = custom_callback;
    g.NextWindowData.SizeCallbackUserData = custom_callback_user_data;
}

void ImGui::SetNextWindowContentSize(const ImVec2& size)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.ContentSizeVal = size;  // In Begin() we will add the size of window decorations (title bar, menu etc.) to that to form a SizeContents value.
    g.NextWindowData.ContentSizeCond = ImGuiCond_Always;
}

void ImGui::SetNextWindowCollapsed(bool collapsed, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond)); // Make sure the user doesn't attempt to combine multiple condition flags.
    g.NextWindowData.CollapsedVal = collapsed;
    g.NextWindowData.CollapsedCond = cond ? cond : ImGuiCond_Always;
}

void ImGui::SetNextWindowFocus()
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.FocusCond = ImGuiCond_Always;   // Using a Cond member for consistency (may transition all of them to single flag set for fast Clear() op)
}

void ImGui::SetNextWindowBgAlpha(float alpha)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.BgAlphaVal = alpha;
    g.NextWindowData.BgAlphaCond = ImGuiCond_Always; // Using a Cond member for consistency (may transition all of them to single flag set for fast Clear() op)
}

void ImGui::SetNextWindowViewport(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.ViewportCond = ImGuiCond_Always;
    g.NextWindowData.ViewportId = id;
}

void ImGui::SetNextWindowDockID(ImGuiID id, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.DockCond = cond ? cond : ImGuiCond_Always;
    g.NextWindowData.DockId = id;
}

void ImGui::SetNextWindowClass(const ImGuiWindowClass* window_class)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.WindowClass = *window_class;
}

// In window space (not screen space!)
ImVec2 ImGui::GetContentRegionMax()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImVec2 mx = window->ContentsRegionRect.Max - window->Pos;
    if (window->DC.ColumnsSet)
        mx.x = GetColumnOffset(window->DC.ColumnsSet->Current + 1) - window->WindowPadding.x;
    return mx;
}

ImVec2 ImGui::GetContentRegionAvail()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return GetContentRegionMax() - (window->DC.CursorPos - window->Pos);
}

float ImGui::GetContentRegionAvailWidth()
{
    return GetContentRegionAvail().x;
}

// In window space (not screen space!)
ImVec2 ImGui::GetWindowContentRegionMin()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ContentsRegionRect.Min - window->Pos;
}

ImVec2 ImGui::GetWindowContentRegionMax()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ContentsRegionRect.Max - window->Pos;
}

float ImGui::GetWindowContentRegionWidth()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ContentsRegionRect.GetWidth();
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

ImDrawList* ImGui::GetWindowDrawList()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DrawList;
}

float ImGui::GetWindowDpiScale()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentViewport != NULL);
    return g.CurrentViewport->DpiScale;
}

ImGuiViewport* ImGui::GetWindowViewport()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentViewport != NULL && g.CurrentViewport == g.CurrentWindow->Viewport);
    return g.CurrentViewport;
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
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->FontWindowScale = scale;
    g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize();
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
    window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
}

void ImGui::SetCursorPosX(float x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + x;
    window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPos.x);
}

void ImGui::SetCursorPosY(float y)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.y = window->Pos.y - window->Scroll.y + y;
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, window->DC.CursorPos.y);
}

ImVec2 ImGui::GetCursorStartPos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorStartPos - window->Pos;
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
    window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
}

float ImGui::GetScrollX()
{
    return GImGui->CurrentWindow->Scroll.x;
}

float ImGui::GetScrollY()
{
    return GImGui->CurrentWindow->Scroll.y;
}

float ImGui::GetScrollMaxX()
{
    return GetWindowScrollMaxX(GImGui->CurrentWindow);
}

float ImGui::GetScrollMaxY()
{
    return GetWindowScrollMaxY(GImGui->CurrentWindow);
}

void ImGui::SetScrollX(float scroll_x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->ScrollTarget.x = scroll_x;
    window->ScrollTargetCenterRatio.x = 0.0f;
}

void ImGui::SetScrollY(float scroll_y)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->ScrollTarget.y = scroll_y + window->TitleBarHeight() + window->MenuBarHeight(); // title bar height canceled out when using ScrollTargetRelY
    window->ScrollTargetCenterRatio.y = 0.0f;
}

void ImGui::SetScrollFromPosY(float local_y, float center_y_ratio)
{
    // We store a target position so centering can occur on the next frame when we are guaranteed to have a known window size
    ImGuiWindow* window = GetCurrentWindow();
    IM_ASSERT(center_y_ratio >= 0.0f && center_y_ratio <= 1.0f);
    window->ScrollTarget.y = (float)(int)(local_y + window->Scroll.y);
    window->ScrollTargetCenterRatio.y = center_y_ratio;
}

// center_y_ratio: 0.0f top of last item, 0.5f vertical center of last item, 1.0f bottom of last item.
void ImGui::SetScrollHereY(float center_y_ratio)
{
    ImGuiWindow* window = GetCurrentWindow();
    float target_y = window->DC.CursorPosPrevLine.y - window->Pos.y; // Top of last item, in window space
    target_y += (window->DC.PrevLineSize.y * center_y_ratio) + (GImGui->Style.ItemSpacing.y * (center_y_ratio - 0.5f) * 2.0f); // Precisely aim above, in the middle or below the last line.
    SetScrollFromPosY(target_y, center_y_ratio);
}

void ImGui::ActivateItem(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    g.NavNextActivateId = id;
}

void ImGui::SetKeyboardFocusHere(int offset)
{
    IM_ASSERT(offset >= -1);    // -1 is allowed but not below
    ImGuiWindow* window = GetCurrentWindow();
    window->FocusIdxAllRequestNext = window->FocusIdxAllCounter + 1 + offset;
    window->FocusIdxTabRequestNext = INT_MAX;
}

void ImGui::SetItemDefaultFocus()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!window->Appearing)
        return;
    if (g.NavWindow == window->RootWindowForNav && (g.NavInitRequest || g.NavInitResultId != 0) && g.NavLayer == g.NavWindow->DC.NavLayerCurrent)
    {
        g.NavInitRequest = false;
        g.NavInitResultId = g.NavWindow->DC.LastItemId;
        g.NavInitResultRectRel = ImRect(g.NavWindow->DC.LastItemRect.Min - g.NavWindow->Pos, g.NavWindow->DC.LastItemRect.Max - g.NavWindow->Pos);
        NavUpdateAnyRequestFlag();
        if (!IsItemVisible())
            SetScrollHereY();
    }
}

void ImGui::SetStateStorage(ImGuiStorage* tree)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.StateStorage = tree ? tree : &window->StateStorage;
}

ImGuiStorage* ImGui::GetStateStorage()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.StateStorage;
}

void ImGui::PushID(const char* str_id)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    window->IDStack.push_back(window->GetIDNoKeepAlive(str_id));
}

void ImGui::PushID(const char* str_id_begin, const char* str_id_end)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    window->IDStack.push_back(window->GetIDNoKeepAlive(str_id_begin, str_id_end));
}

void ImGui::PushID(const void* ptr_id)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    window->IDStack.push_back(window->GetIDNoKeepAlive(ptr_id));
}

void ImGui::PushID(int int_id)
{
    const void* ptr_id = (void*)(intptr_t)int_id;
    ImGuiWindow* window = GetCurrentWindowRead();
    window->IDStack.push_back(window->GetIDNoKeepAlive(ptr_id));
}

void ImGui::PopID()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    window->IDStack.pop_back();
}

ImGuiID ImGui::GetID(const char* str_id)
{
    return GImGui->CurrentWindow->GetID(str_id);
}

ImGuiID ImGui::GetID(const char* str_id_begin, const char* str_id_end)
{
    return GImGui->CurrentWindow->GetID(str_id_begin, str_id_end);
}

ImGuiID ImGui::GetID(const void* ptr_id)
{
    return GImGui->CurrentWindow->GetID(ptr_id);
}

bool ImGui::IsRectVisible(const ImVec2& size)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ClipRect.Overlaps(ImRect(window->DC.CursorPos, window->DC.CursorPos + size));
}

bool ImGui::IsRectVisible(const ImVec2& rect_min, const ImVec2& rect_max)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ClipRect.Overlaps(ImRect(rect_min, rect_max));
}

// Lock horizontal starting position + capture group bounding box into one "item" (so you can use IsItemHovered() or layout primitives such as SameLine() on whole group, etc.)
void ImGui::BeginGroup()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    window->DC.GroupStack.resize(window->DC.GroupStack.Size + 1);
    ImGuiGroupData& group_data = window->DC.GroupStack.back();
    group_data.BackupCursorPos = window->DC.CursorPos;
    group_data.BackupCursorMaxPos = window->DC.CursorMaxPos;
    group_data.BackupIndent = window->DC.Indent;
    group_data.BackupGroupOffset = window->DC.GroupOffset;
    group_data.BackupCurrentLineSize = window->DC.CurrentLineSize;
    group_data.BackupCurrentLineTextBaseOffset = window->DC.CurrentLineTextBaseOffset;
    group_data.BackupLogLinePosY = window->DC.LogLinePosY;
    group_data.BackupActiveIdIsAlive = g.ActiveIdIsAlive;
    group_data.BackupActiveIdPreviousFrameIsAlive = g.ActiveIdPreviousFrameIsAlive;
    group_data.AdvanceCursor = true;

    window->DC.GroupOffset.x = window->DC.CursorPos.x - window->Pos.x - window->DC.ColumnsOffset.x;
    window->DC.Indent = window->DC.GroupOffset;
    window->DC.CursorMaxPos = window->DC.CursorPos;
    window->DC.CurrentLineSize = ImVec2(0.0f, 0.0f);
    window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f; // To enforce Log carriage return
}

void ImGui::EndGroup()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    IM_ASSERT(!window->DC.GroupStack.empty());    // Mismatched BeginGroup()/EndGroup() calls

    ImGuiGroupData& group_data = window->DC.GroupStack.back();

    ImRect group_bb(group_data.BackupCursorPos, window->DC.CursorMaxPos);
    group_bb.Max = ImMax(group_bb.Min, group_bb.Max);

    window->DC.CursorPos = group_data.BackupCursorPos;
    window->DC.CursorMaxPos = ImMax(group_data.BackupCursorMaxPos, window->DC.CursorMaxPos);
    window->DC.Indent = group_data.BackupIndent;
    window->DC.GroupOffset = group_data.BackupGroupOffset;
    window->DC.CurrentLineSize = group_data.BackupCurrentLineSize;
    window->DC.CurrentLineTextBaseOffset = group_data.BackupCurrentLineTextBaseOffset;
    window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f; // To enforce Log carriage return

    if (group_data.AdvanceCursor)
    {
        window->DC.CurrentLineTextBaseOffset = ImMax(window->DC.PrevLineTextBaseOffset, group_data.BackupCurrentLineTextBaseOffset);      // FIXME: Incorrect, we should grab the base offset from the *first line* of the group but it is hard to obtain now.
        ItemSize(group_bb.GetSize(), group_data.BackupCurrentLineTextBaseOffset);
        ItemAdd(group_bb, 0);
    }

    // If the current ActiveId was declared within the boundary of our group, we copy it to LastItemId so IsItemActive(), IsItemDeactivated() etc. will be functional on the entire group.
    // It would be be neater if we replaced window.DC.LastItemId by e.g. 'bool LastItemIsActive', but would put a little more burden on individual widgets.
    // (and if you grep for LastItemId you'll notice it is only used in that context.
    if ((group_data.BackupActiveIdIsAlive != g.ActiveId) && (g.ActiveIdIsAlive == g.ActiveId) && g.ActiveId) // && g.ActiveIdWindow->RootWindow == window->RootWindow)
        window->DC.LastItemId = g.ActiveId;
    else if (!group_data.BackupActiveIdPreviousFrameIsAlive && g.ActiveIdPreviousFrameIsAlive) // && g.ActiveIdPreviousFrameWindow->RootWindow == window->RootWindow)
        window->DC.LastItemId = g.ActiveIdPreviousFrame;
    window->DC.LastItemRect = group_bb;

    window->DC.GroupStack.pop_back();

    //window->DrawList->AddRect(group_bb.Min, group_bb.Max, IM_COL32(255,0,255,255));   // [Debug]
}

// Gets back to previous line and continue with horizontal layout
//      pos_x == 0      : follow right after previous item
//      pos_x != 0      : align to specified x position (relative to window/group left)
//      spacing_w < 0   : use default spacing if pos_x == 0, no spacing if pos_x != 0
//      spacing_w >= 0  : enforce spacing amount
void ImGui::SameLine(float pos_x, float spacing_w)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    if (pos_x != 0.0f)
    {
        if (spacing_w < 0.0f) spacing_w = 0.0f;
        window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + pos_x + spacing_w + window->DC.GroupOffset.x + window->DC.ColumnsOffset.x;
        window->DC.CursorPos.y = window->DC.CursorPosPrevLine.y;
    }
    else
    {
        if (spacing_w < 0.0f) spacing_w = g.Style.ItemSpacing.x;
        window->DC.CursorPos.x = window->DC.CursorPosPrevLine.x + spacing_w;
        window->DC.CursorPos.y = window->DC.CursorPosPrevLine.y;
    }
    window->DC.CurrentLineSize = window->DC.PrevLineSize;
    window->DC.CurrentLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
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

//-----------------------------------------------------------------------------
// [SECTION] TOOLTIPS
//-----------------------------------------------------------------------------

void ImGui::BeginTooltip()
{
    ImGuiContext& g = *GImGui;
    if (g.DragDropWithinSourceOrTarget)
    {
        // The default tooltip position is a little offset to give space to see the context menu (it's also clamped within the current viewport/monitor)
        // In the context of a dragging tooltip we try to reduce that offset and we enforce following the cursor.
        // Whatever we do we want to call SetNextWindowPos() to enforce a tooltip position and disable clipping the tooltip without our display area, like regular tooltip do.
        //ImVec2 tooltip_pos = g.IO.MousePos - g.ActiveIdClickOffset - g.Style.WindowPadding;
        ImVec2 tooltip_pos = g.IO.MousePos + ImVec2(16 * g.Style.MouseCursorScale, 8 * g.Style.MouseCursorScale);
        SetNextWindowPos(tooltip_pos);
        SetNextWindowBgAlpha(g.Style.Colors[ImGuiCol_PopupBg].w * 0.60f);
        //PushStyleVar(ImGuiStyleVar_Alpha, g.Style.Alpha * 0.60f); // This would be nice but e.g ColorButton with checkboard has issue with transparent colors :(
        BeginTooltipEx(0, true);
    }
    else
    {
        BeginTooltipEx(0, false);
    }
}

// Not exposed publicly as BeginTooltip() because bool parameters are evil. Let's see if other needs arise first.
void ImGui::BeginTooltipEx(ImGuiWindowFlags extra_flags, bool override_previous_tooltip)
{
    ImGuiContext& g = *GImGui;
    char window_name[16];
    ImFormatString(window_name, IM_ARRAYSIZE(window_name), "##Tooltip_%02d", g.TooltipOverrideCount);
    if (override_previous_tooltip)
        if (ImGuiWindow* window = FindWindowByName(window_name))
            if (window->Active)
            {
                // Hide previous tooltip from being displayed. We can't easily "reset" the content of a window so we create a new one.
                window->Hidden = true;
                window->HiddenFramesRegular = 1;
                ImFormatString(window_name, IM_ARRAYSIZE(window_name), "##Tooltip_%02d", ++g.TooltipOverrideCount);
            }
    ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoDocking;
    Begin(window_name, NULL, flags | extra_flags);
}

void ImGui::EndTooltip()
{
    IM_ASSERT(GetCurrentWindowRead()->Flags & ImGuiWindowFlags_Tooltip);   // Mismatched BeginTooltip()/EndTooltip() calls
    End();
}

void ImGui::SetTooltipV(const char* fmt, va_list args)
{
    ImGuiContext& g = *GImGui;
    if (g.DragDropWithinSourceOrTarget)
        BeginTooltip();
    else
        BeginTooltipEx(0, true);
    TextV(fmt, args);
    EndTooltip();
}

void ImGui::SetTooltip(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SetTooltipV(fmt, args);
    va_end(args);
}

//-----------------------------------------------------------------------------
// [SECTION] POPUPS
//-----------------------------------------------------------------------------

bool ImGui::IsPopupOpen(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    return g.OpenPopupStack.Size > g.BeginPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].PopupId == id;
}

bool ImGui::IsPopupOpen(const char* str_id)
{
    ImGuiContext& g = *GImGui;
    return g.OpenPopupStack.Size > g.BeginPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].PopupId == g.CurrentWindow->GetID(str_id);
}

ImGuiWindow* ImGui::GetFrontMostPopupModal()
{
    ImGuiContext& g = *GImGui;
    for (int n = g.OpenPopupStack.Size-1; n >= 0; n--)
        if (ImGuiWindow* popup = g.OpenPopupStack.Data[n].Window)
            if (popup->Flags & ImGuiWindowFlags_Modal)
                return popup;
    return NULL;
}

void ImGui::OpenPopup(const char* str_id)
{
    ImGuiContext& g = *GImGui;
    OpenPopupEx(g.CurrentWindow->GetID(str_id));
}

// Mark popup as open (toggle toward open state).
// Popups are closed when user click outside, or activate a pressable item, or CloseCurrentPopup() is called within a BeginPopup()/EndPopup() block.
// Popup identifiers are relative to the current ID-stack (so OpenPopup and BeginPopup needs to be at the same level).
// One open popup per level of the popup hierarchy (NB: when assigning we reset the Window member of ImGuiPopupRef to NULL)
void ImGui::OpenPopupEx(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;
    int current_stack_size = g.BeginPopupStack.Size;
    ImGuiPopupRef popup_ref; // Tagged as new ref as Window will be set back to NULL if we write this into OpenPopupStack.
    popup_ref.PopupId = id;
    popup_ref.Window = NULL;
    popup_ref.ParentWindow = parent_window;
    popup_ref.OpenFrameCount = g.FrameCount;
    popup_ref.OpenParentId = parent_window->IDStack.back();
    popup_ref.OpenPopupPos = NavCalcPreferredRefPos();
    popup_ref.OpenMousePos = IsMousePosValid(&g.IO.MousePos) ? g.IO.MousePos : popup_ref.OpenPopupPos;

    //IMGUI_DEBUG_LOG("OpenPopupEx(0x%08X)\n", g.FrameCount, id);
    if (g.OpenPopupStack.Size < current_stack_size + 1)
    {
        g.OpenPopupStack.push_back(popup_ref);
    }
    else
    {
        // Gently handle the user mistakenly calling OpenPopup() every frame. It is a programming mistake! However, if we were to run the regular code path, the ui
        // would become completely unusable because the popup will always be in hidden-while-calculating-size state _while_ claiming focus. Which would be a very confusing
        // situation for the programmer. Instead, we silently allow the popup to proceed, it will keep reappearing and the programming error will be more obvious to understand.
        if (g.OpenPopupStack[current_stack_size].PopupId == id && g.OpenPopupStack[current_stack_size].OpenFrameCount == g.FrameCount - 1)
        {
            g.OpenPopupStack[current_stack_size].OpenFrameCount = popup_ref.OpenFrameCount;
        }
        else
        {
            // Close child popups if any, then flag popup for open/reopen
            g.OpenPopupStack.resize(current_stack_size + 1);
            g.OpenPopupStack[current_stack_size] = popup_ref;
        }

        // When reopening a popup we first refocus its parent, otherwise if its parent is itself a popup it would get closed by ClosePopupsOverWindow().
        // This is equivalent to what ClosePopupToLevel() does.
        //if (g.OpenPopupStack[current_stack_size].PopupId == id)
        //    FocusWindow(parent_window);
    }
}

bool ImGui::OpenPopupOnItemClick(const char* str_id, int mouse_button)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    if (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
    {
        ImGuiID id = str_id ? window->GetID(str_id) : window->DC.LastItemId; // If user hasn't passed an ID, we can use the LastItemID. Using LastItemID as a Popup ID won't conflict!
        IM_ASSERT(id != 0);                                                  // You cannot pass a NULL str_id if the last item has no identifier (e.g. a Text() item)
        OpenPopupEx(id);
        return true;
    }
    return false;
}

void ImGui::ClosePopupsOverWindow(ImGuiWindow* ref_window)
{
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.empty())
        return;

    // When popups are stacked, clicking on a lower level popups puts focus back to it and close popups above it.
    // Don't close our own child popup windows.
    int popup_count_to_keep = 0;
    if (ref_window)
    {
        // Find the highest popup which is a descendant of the reference window (generally reference window = NavWindow)
        for (; popup_count_to_keep < g.OpenPopupStack.Size; popup_count_to_keep++)
        {
            ImGuiPopupRef& popup = g.OpenPopupStack[popup_count_to_keep];
            if (!popup.Window)
                continue;
            IM_ASSERT((popup.Window->Flags & ImGuiWindowFlags_Popup) != 0);
            if (popup.Window->Flags & ImGuiWindowFlags_ChildWindow)
                continue;

            // Trim the stack if popups are not direct descendant of the reference window (which is often the NavWindow)
            bool popup_or_descendent_has_focus = false;
            for (int m = popup_count_to_keep; m < g.OpenPopupStack.Size && !popup_or_descendent_has_focus; m++)
                if (g.OpenPopupStack[m].Window && g.OpenPopupStack[m].Window->RootWindow == ref_window->RootWindow)
                    popup_or_descendent_has_focus = true;
            if (!popup_or_descendent_has_focus)
                break;
        }
    }
    if (popup_count_to_keep < g.OpenPopupStack.Size) // This test is not required but it allows to set a convenient breakpoint on the statement below
    {
        //IMGUI_DEBUG_LOG("ClosePopupsOverWindow(%s) -> ClosePopupToLevel(%d)\n", ref_window->Name, popup_count_to_keep);
        ClosePopupToLevel(popup_count_to_keep, false);
    }
}

void ImGui::ClosePopupToLevel(int remaining, bool apply_focus_to_window_under)
{
    IM_ASSERT(remaining >= 0);
    ImGuiContext& g = *GImGui;
    ImGuiWindow* focus_window = (remaining > 0) ? g.OpenPopupStack[remaining-1].Window : g.OpenPopupStack[0].ParentWindow;
    g.OpenPopupStack.resize(remaining);

    // FIXME: This code is faulty and we may want to eventually to replace or remove the 'apply_focus_to_window_under=true' path completely.
    // Instead of using g.OpenPopupStack[remaining-1].Window etc. we should find the highest root window that is behind the popups we are closing.
    // The current code will set focus to the parent of the popup window which is incorrect. 
    // It rarely manifested until now because UpdateMouseMovingWindow() would call FocusWindow() again on the clicked window, 
    // leading to a chain of focusing A (clicked window) then B (parent window of the popup) then A again.
    // However if the clicked window has the _NoMove flag set we would be left with B focused.
    // For now, we have disabled this path when called from ClosePopupsOverWindow() because the users of ClosePopupsOverWindow() don't need to alter focus anyway,
    // but we should inspect and fix this properly.
    if (apply_focus_to_window_under)
    {
        if (g.NavLayer == 0)
            focus_window = NavRestoreLastChildNavWindow(focus_window);
        FocusWindow(focus_window);
    }
}

// Close the popup we have begin-ed into.
void ImGui::CloseCurrentPopup()
{
    ImGuiContext& g = *GImGui;
    int popup_idx = g.BeginPopupStack.Size - 1;
    if (popup_idx < 0 || popup_idx >= g.OpenPopupStack.Size || g.BeginPopupStack[popup_idx].PopupId != g.OpenPopupStack[popup_idx].PopupId)
        return;
    while (popup_idx > 0 && g.OpenPopupStack[popup_idx].Window && (g.OpenPopupStack[popup_idx].Window->Flags & ImGuiWindowFlags_ChildMenu))
        popup_idx--;
    ClosePopupToLevel(popup_idx, true);

    // A common pattern is to close a popup when selecting a menu item/selectable that will open another window.
    // To improve this usage pattern, we avoid nav highlight for a single frame in the parent window.
    // Similarly, we could avoid mouse hover highlight in this window but it is less visually problematic.
    if (ImGuiWindow* window = g.NavWindow)
        window->DC.NavHideHighlightOneFrame = true;
}

bool ImGui::BeginPopupEx(ImGuiID id, ImGuiWindowFlags extra_flags)
{
    ImGuiContext& g = *GImGui;
    if (!IsPopupOpen(id))
    {
        g.NextWindowData.Clear(); // We behave like Begin() and need to consume those values
        return false;
    }

    char name[20];
    if (extra_flags & ImGuiWindowFlags_ChildMenu)
        ImFormatString(name, IM_ARRAYSIZE(name), "##Menu_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth
    else
        ImFormatString(name, IM_ARRAYSIZE(name), "##Popup_%08x", id); // Not recycling, so we can close/open during the same frame

    bool is_open = Begin(name, NULL, extra_flags | ImGuiWindowFlags_Popup);
    if (!is_open) // NB: Begin can return false when the popup is completely clipped (e.g. zero size display)
        EndPopup();

    return is_open;
}

bool ImGui::BeginPopup(const char* str_id, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.Size <= g.BeginPopupStack.Size) // Early out for performance
    {
        g.NextWindowData.Clear(); // We behave like Begin() and need to consume those values
        return false;
    }
    flags |= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
    return BeginPopupEx(g.CurrentWindow->GetID(str_id), flags);
}

// If 'p_open' is specified for a modal popup window, the popup will have a regular close button which will close the popup.
// Note that popup visibility status is owned by imgui (and manipulated with e.g. OpenPopup) so the actual value of *p_open is meaningless here.
bool ImGui::BeginPopupModal(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const ImGuiID id = window->GetID(name);
    if (!IsPopupOpen(id))
    {
        g.NextWindowData.Clear(); // We behave like Begin() and need to consume those values
        return false;
    }

    // Center modal windows by default
    // FIXME: Should test for (PosCond & window->SetWindowPosAllowFlags) with the upcoming window.
    if (g.NextWindowData.PosCond == 0)
        SetNextWindowPos(window->Viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    flags |= ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
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
    ImGuiContext& g = *GImGui; (void)g;
    IM_ASSERT(g.CurrentWindow->Flags & ImGuiWindowFlags_Popup);  // Mismatched BeginPopup()/EndPopup() calls
    IM_ASSERT(g.BeginPopupStack.Size > 0);

    // Make all menus and popups wrap around for now, may need to expose that policy.
    NavMoveRequestTryWrapping(g.CurrentWindow, ImGuiNavMoveFlags_LoopY);

    End();
}

// This is a helper to handle the simplest case of associating one named popup to one given widget.
// You may want to handle this on user side if you have specific needs (e.g. tweaking IsItemHovered() parameters).
// You can pass a NULL str_id to use the identifier of the last item.
bool ImGui::BeginPopupContextItem(const char* str_id, int mouse_button)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    ImGuiID id = str_id ? window->GetID(str_id) : window->DC.LastItemId; // If user hasn't passed an ID, we can use the LastItemID. Using LastItemID as a Popup ID won't conflict!
    IM_ASSERT(id != 0);                                                  // You cannot pass a NULL str_id if the last item has no identifier (e.g. a Text() item)
    if (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
        OpenPopupEx(id);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings);
}

bool ImGui::BeginPopupContextWindow(const char* str_id, int mouse_button, bool also_over_items)
{
    if (!str_id)
        str_id = "window_context";
    ImGuiID id = GImGui->CurrentWindow->GetID(str_id);
    if (IsMouseReleased(mouse_button) && IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
        if (also_over_items || !IsAnyItemHovered())
            OpenPopupEx(id);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings);
}

bool ImGui::BeginPopupContextVoid(const char* str_id, int mouse_button)
{
    if (!str_id)
        str_id = "void_context";
    ImGuiID id = GImGui->CurrentWindow->GetID(str_id);
    if (IsMouseReleased(mouse_button) && !IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        OpenPopupEx(id);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings);
}

// r_avoid = the rectangle to avoid (e.g. for tooltip it is a rectangle around the mouse cursor which we want to avoid. for popups it's a small point around the cursor.)
// r_outer = the visible area rectangle, minus safe area padding. If our popup size won't fit because of safe area padding we ignore it.
// (r_outer is usually equivalent to the viewport rectangle minus padding, but when multi-viewports are enabled and monitor
//  information are available, it may represent the entire platform monitor from the frame of reference of the current viewport.
//  this allows us to have tooltips/popups displayed out of the parent viewport.)
ImVec2 ImGui::FindBestWindowPosForPopupEx(const ImVec2& ref_pos, const ImVec2& size, ImGuiDir* last_dir, const ImRect& r_outer, const ImRect& r_avoid, ImGuiPopupPositionPolicy policy)
{
    ImVec2 base_pos_clamped = ImClamp(ref_pos, r_outer.Min, r_outer.Max - size);
    //GImGui->OverlayDrawList.AddRect(r_avoid.Min, r_avoid.Max, IM_COL32(255,0,0,255));
    //GImGui->OverlayDrawList.AddRect(r_outer.Min, r_outer.Max, IM_COL32(0,255,0,255));

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

    // Default popup policy
    const ImGuiDir dir_prefered_order[ImGuiDir_COUNT] = { ImGuiDir_Right, ImGuiDir_Down, ImGuiDir_Up, ImGuiDir_Left };
    for (int n = (*last_dir != ImGuiDir_None) ? -1 : 0; n < ImGuiDir_COUNT; n++)
    {
        const ImGuiDir dir = (n == -1) ? *last_dir : dir_prefered_order[n];
        if (n != -1 && dir == *last_dir) // Already tried this direction?
            continue;
        float avail_w = (dir == ImGuiDir_Left ? r_avoid.Min.x : r_outer.Max.x) - (dir == ImGuiDir_Right ? r_avoid.Max.x : r_outer.Min.x);
        float avail_h = (dir == ImGuiDir_Up ? r_avoid.Min.y : r_outer.Max.y) - (dir == ImGuiDir_Down ? r_avoid.Max.y : r_outer.Min.y);
        if (avail_w < size.x || avail_h < size.y)
            continue;
        ImVec2 pos;
        pos.x = (dir == ImGuiDir_Left) ? r_avoid.Min.x - size.x : (dir == ImGuiDir_Right) ? r_avoid.Max.x : base_pos_clamped.x;
        pos.y = (dir == ImGuiDir_Up)   ? r_avoid.Min.y - size.y : (dir == ImGuiDir_Down)  ? r_avoid.Max.y : base_pos_clamped.y;
        *last_dir = dir;
        return pos;
    }

    // Fallback, try to keep within display
    *last_dir = ImGuiDir_None;
    ImVec2 pos = ref_pos;
    pos.x = ImMax(ImMin(pos.x + size.x, r_outer.Max.x) - size.x, r_outer.Min.x);
    pos.y = ImMax(ImMin(pos.y + size.y, r_outer.Max.y) - size.y, r_outer.Min.y);
    return pos;
}

ImRect ImGui::GetWindowAllowedExtentRect(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    ImRect r_screen;
    if (window->ViewportAllowPlatformMonitorExtend >= 0)
    {
        // Extent with be in the frame of reference of the given viewport (so Min is likely to be negative here)
        const ImGuiPlatformMonitor& monitor = g.PlatformIO.Monitors[window->ViewportAllowPlatformMonitorExtend];
        r_screen.Min = monitor.WorkPos;
        r_screen.Max = monitor.WorkPos + monitor.WorkSize;
    }
    else
    {
        r_screen.Min = window->Viewport->Pos;
        r_screen.Max = window->Viewport->Pos + window->Viewport->Size;
    }
    ImVec2 padding = g.Style.DisplaySafeAreaPadding;
    r_screen.Expand(ImVec2((r_screen.GetWidth() > padding.x * 2) ? -padding.x : 0.0f, (r_screen.GetHeight() > padding.y * 2) ? -padding.y : 0.0f));
    return r_screen;
}

ImVec2 ImGui::FindBestWindowPosForPopup(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (window->Flags & ImGuiWindowFlags_ChildMenu)
    {
        // Child menus typically request _any_ position within the parent menu item, and then our FindBestWindowPosForPopup() function will move the new menu outside the parent bounds.
        // This is how we end up with child menus appearing (most-commonly) on the right of the parent menu.
        ImGuiWindow* parent_window = window->ParentWindow;
        float horizontal_overlap = g.Style.ItemSpacing.x;       // We want some overlap to convey the relative depth of each menu (currently the amount of overlap is hard-coded to style.ItemSpacing.x).
        ImRect r_outer = GetWindowAllowedExtentRect(window);
        ImRect r_avoid;
        if (parent_window->DC.MenuBarAppending)
            r_avoid = ImRect(-FLT_MAX, parent_window->Pos.y + parent_window->TitleBarHeight(), FLT_MAX, parent_window->Pos.y + parent_window->TitleBarHeight() + parent_window->MenuBarHeight());
        else
            r_avoid = ImRect(parent_window->Pos.x + horizontal_overlap, -FLT_MAX, parent_window->Pos.x + parent_window->Size.x - horizontal_overlap - parent_window->ScrollbarSizes.x, FLT_MAX);
        return FindBestWindowPosForPopupEx(window->Pos, window->Size, &window->AutoPosLastDirection, r_outer, r_avoid);
    }
    if (window->Flags & ImGuiWindowFlags_Popup)
    {
        ImRect r_outer = GetWindowAllowedExtentRect(window);
        ImRect r_avoid = ImRect(window->Pos.x - 1, window->Pos.y - 1, window->Pos.x + 1, window->Pos.y + 1);
        return FindBestWindowPosForPopupEx(window->Pos, window->Size, &window->AutoPosLastDirection, r_outer, r_avoid);
    }
    if (window->Flags & ImGuiWindowFlags_Tooltip)
    {
        // Position tooltip (always follows mouse)
        float sc = g.Style.MouseCursorScale;
        ImVec2 ref_pos = NavCalcPreferredRefPos();
        ImRect r_outer = GetWindowAllowedExtentRect(window);
        ImRect r_avoid;
        if (!g.NavDisableHighlight && g.NavDisableMouseHover && !(g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableSetMousePos))
            r_avoid = ImRect(ref_pos.x - 16, ref_pos.y - 8, ref_pos.x + 16, ref_pos.y + 8);
        else
            r_avoid = ImRect(ref_pos.x - 16, ref_pos.y - 8, ref_pos.x + 24 * sc, ref_pos.y + 24 * sc); // FIXME: Hard-coded based on mouse cursor shape expectation. Exact dimension not very important.
        ImVec2 pos = FindBestWindowPosForPopupEx(ref_pos, window->Size, &window->AutoPosLastDirection, r_outer, r_avoid);
        if (window->AutoPosLastDirection == ImGuiDir_None)
            pos = ref_pos + ImVec2(2, 2); // If there's not enough room, for tooltip we prefer avoiding the cursor at all cost even if it means that part of the tooltip won't be visible.
        return pos;
    }
    IM_ASSERT(0);
    return window->Pos;
}

//-----------------------------------------------------------------------------
// [SECTION] VIEWPORTS, PLATFORM WINDOWS
//-----------------------------------------------------------------------------

ImGuiViewport* ImGui::GetMainViewport() 
{ 
    ImGuiContext& g = *GImGui;
    return g.Viewports[0];
}

ImGuiViewport* ImGui::FindViewportByID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    for (int n = 0; n < g.Viewports.Size; n++)
        if (g.Viewports[n]->ID == id)
            return g.Viewports[n];
    return NULL;
}

ImGuiViewport* ImGui::FindViewportByPlatformHandle(void* platform_handle)
{
    ImGuiContext& g = *GImGui;
    for (int i = 0; i != g.Viewports.Size; i++)
        if (g.Viewports[i]->PlatformHandle == platform_handle)
            return g.Viewports[i];
    return NULL;
}

void ImGui::SetCurrentViewport(ImGuiWindow* current_window, ImGuiViewportP* viewport)
{
    ImGuiContext& g = *GImGui;
    (void)current_window;

    if (viewport)
        viewport->LastFrameActive = g.FrameCount;
    if (g.CurrentViewport == viewport)
        return;
    g.CurrentViewport = viewport;

    // Notify platform layer of viewport changes
    // FIXME-DPI: This is only currently used for experimenting with handling of multiple DPI
    if (g.CurrentViewport && g.PlatformIO.Platform_OnChangedViewport)
        g.PlatformIO.Platform_OnChangedViewport(g.CurrentViewport);
}

static void SetWindowViewport(ImGuiWindow* window, ImGuiViewportP* viewport)
{
    window->Viewport = viewport;
    window->ViewportId = viewport->ID;
    window->ViewportOwned = (viewport->Window == window);
}

static bool ImGui::GetWindowAlwaysWantOwnViewport(ImGuiWindow* window)
{
    // Tooltips and menus are not automatically forced into their own viewport when the NoMerge flag is set, however the multiplication of viewports makes them more likely to protude and create their own.
    ImGuiContext& g = *GImGui;
    if (g.IO.ConfigViewportsNoAutoMerge && (g.ConfigFlagsForFrame & ImGuiConfigFlags_ViewportsEnable))
        if (!window->DockIsActive)
            if ((window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_Tooltip)) == 0)
                return true;
    return false;
}

static bool ImGui::UpdateTryMergeWindowIntoHostViewport(ImGuiWindow* window, ImGuiViewportP* viewport)
{
    ImGuiContext& g = *GImGui;
    if (!(viewport->Flags & ImGuiViewportFlags_CanHostOtherWindows) || window->Viewport == viewport || viewport->PlatformWindowMinimized)
        return false;
    if (!viewport->GetRect().Contains(window->Rect()))
        return false;
    if (GetWindowAlwaysWantOwnViewport(window))
        return false;

    for (int n = 0; n < g.Windows.Size; n++)
    {
        ImGuiWindow* window_behind = g.Windows[n];
        if (window_behind == window)
            break;
        if (window_behind->WasActive && window_behind->ViewportOwned && !(window_behind->Flags & ImGuiWindowFlags_ChildWindow))
            if (window_behind->Viewport->GetRect().Overlaps(window->Rect()))
                return false;
    }

    // Move to the existing viewport, Move child/hosted windows as well (FIXME-OPT: iterate child)
    ImGuiViewportP* old_viewport = window->Viewport;
    if (window->ViewportOwned)
        for (int n = 0; n < g.Windows.Size; n++)
            if (g.Windows[n]->Viewport == old_viewport)
                SetWindowViewport(g.Windows[n], viewport);
    SetWindowViewport(window, viewport);
    BringWindowToDisplayFront(window);

    return true;
}

// Scale all windows (position, size). Use when e.g. changing DPI. (This is a lossy operation!)
void ImGui::ScaleWindowsInViewport(ImGuiViewportP* viewport, float scale)
{
    ImGuiContext& g = *GImGui;
    if (viewport->Window)
    {
        ScaleWindow(viewport->Window, scale);
    }
    else
    {
        for (int i = 0; i != g.Windows.Size; i++)
            if (g.Windows[i]->Viewport == viewport)
                ScaleWindow(g.Windows[i], scale);
    }
}

// If the back-end doesn't set MouseLastHoveredViewport or doesn't honor ImGuiViewportFlags_NoInputs, we do a search ourselves.
// A) It won't take account of the possibility that non-imgui windows may be in-between our dragged window and our target window. 
// B) It requires Platform_GetWindowFocus to be implemented by back-end.
static ImGuiViewportP* FindViewportHoveredFromPlatformWindowStack(const ImVec2 mouse_platform_pos)
{
    ImGuiContext& g = *GImGui;
    ImGuiViewportP* best_candidate = NULL;
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        if (!(viewport->Flags & ImGuiViewportFlags_NoInputs) && !viewport->PlatformWindowMinimized && viewport->GetRect().Contains(mouse_platform_pos))
            if (best_candidate == NULL || best_candidate->LastFrontMostStampCount < viewport->LastFrontMostStampCount)
                best_candidate = viewport;
    }
    return best_candidate;
}

static void ImGui::UpdateViewportsNewFrame()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.PlatformIO.Viewports.Size <= g.Viewports.Size);

    // Update Minimized status (we need it first in order to decide if we'll apply Pos/Size of the main viewport)
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        const bool platform_funcs_available = (n == 0 || viewport->PlatformWindowCreated);
        if ((g.ConfigFlagsForFrame & ImGuiConfigFlags_ViewportsEnable))
            if (g.PlatformIO.Platform_GetWindowMinimized && platform_funcs_available)
                viewport->PlatformWindowMinimized = g.PlatformIO.Platform_GetWindowMinimized(viewport);
    }

    // Create/update main viewport with current platform position and size
    ImGuiViewportP* main_viewport = g.Viewports[0];
    IM_ASSERT(main_viewport->ID == IMGUI_VIEWPORT_DEFAULT_ID);
    IM_ASSERT(main_viewport->Window == NULL);
    ImVec2 main_viewport_platform_pos = ImVec2(0.0f, 0.0f);
    ImVec2 main_viewport_platform_size = g.IO.DisplaySize;
    if (g.ConfigFlagsForFrame & ImGuiConfigFlags_ViewportsEnable)
        main_viewport_platform_pos = main_viewport->PlatformWindowMinimized ? main_viewport->Pos : g.PlatformIO.Platform_GetWindowPos(main_viewport);
    AddUpdateViewport(NULL, IMGUI_VIEWPORT_DEFAULT_ID, main_viewport_platform_pos, main_viewport_platform_size, ImGuiViewportFlags_CanHostOtherWindows);

    g.CurrentViewport = NULL;
    g.MouseViewport = NULL;
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        // Erase unused viewports
        ImGuiViewportP* viewport = g.Viewports[n];
        viewport->Idx = n;

        if (n > 0 && viewport->LastFrameActive < g.FrameCount - 2)
        {
            // Clear references to this viewport in windows (window->ViewportId becomes the master data)
            for (int window_n = 0; window_n < g.Windows.Size; window_n++)
                if (g.Windows[window_n]->Viewport == viewport)
                {
                    g.Windows[window_n]->Viewport = NULL;
                    g.Windows[window_n]->ViewportOwned = false;
                }
            if (viewport == g.MouseLastHoveredViewport) 
                g.MouseLastHoveredViewport = NULL;
            g.Viewports.erase(g.Viewports.Data + n);

            // Destroy
            //IMGUI_DEBUG_LOG("Delete Viewport %08X (%s)\n", viewport->ID, viewport->Window ? viewport->Window->Name : "n/a");
            DestroyPlatformWindow(viewport); // In most circumstances the platform window will already be destroyed here.
            IM_ASSERT(g.PlatformIO.Viewports.contains(viewport) == false);
            IM_DELETE(viewport);
            n--;
            continue;
        }

        const bool platform_funcs_available = (n == 0 || viewport->PlatformWindowCreated);
        if ((g.ConfigFlagsForFrame & ImGuiConfigFlags_ViewportsEnable))
        {
            // Update Position and Size (from Platform Window to ImGui) if requested. 
            // We do it early in the frame instead of waiting for UpdatePlatformWindows() to avoid a frame of lag when moving/resizing using OS facilities.
            if (!viewport->PlatformWindowMinimized && platform_funcs_available)
            {
                if (viewport->PlatformRequestMove)
                    viewport->Pos = viewport->LastPlatformPos = g.PlatformIO.Platform_GetWindowPos(viewport);
                if (viewport->PlatformRequestResize)
                    viewport->Size = viewport->LastPlatformSize = g.PlatformIO.Platform_GetWindowSize(viewport);
            }

            // Update monitor (we'll use this info to clamp windows and save windows lost in a removed monitor)
            viewport->PlatformMonitor = (short)FindPlatformMonitorForRect(viewport->GetRect());
        }

        // Reset alpha every frame. Users of transparency (docking) needs to request a lower alpha back.
        viewport->Alpha = 1.0f;

        // Translate imgui windows when a Host Viewport has been moved
        // (This additionally keeps windows at the same place when ImGuiConfigFlags_ViewportsEnable is toggled!)
        ImVec2 viewport_delta = viewport->Pos - viewport->LastPos;
        if ((viewport->Flags & ImGuiViewportFlags_CanHostOtherWindows) && (viewport_delta.x != 0.0f || viewport_delta.y != 0.0f))
            for (int window_n = 0; window_n < g.Windows.Size; window_n++)
                if (g.Windows[window_n]->Viewport == viewport || (g.ConfigFlagsForFrame & ImGuiConfigFlags_ViewportsEnable) == 0)
                    TranslateWindow(g.Windows[window_n], viewport_delta);

        // Update DPI scale
        float new_dpi_scale;
        if (g.PlatformIO.Platform_GetWindowDpiScale && platform_funcs_available)
            new_dpi_scale = g.PlatformIO.Platform_GetWindowDpiScale(viewport);
        else if (viewport->PlatformMonitor != -1)
            new_dpi_scale = g.PlatformIO.Monitors[viewport->PlatformMonitor].DpiScale;
        else
            new_dpi_scale = (viewport->DpiScale != 0.0f) ? viewport->DpiScale : 1.0f;
        if (viewport->DpiScale != 0.0f && new_dpi_scale != viewport->DpiScale)
        {
            float scale_factor = new_dpi_scale / viewport->DpiScale;
            if (g.IO.ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
                ScaleWindowsInViewport(viewport, scale_factor);
            //if (viewport == GetMainViewport())
            //    g.PlatformInterface.SetWindowSize(viewport, viewport->Size * scale_factor);

            // Scale our window moving pivot so that the window will rescale roughly around the mouse position.
            // FIXME-VIEWPORT: This currently creates a resizing feedback loop when a window is straddling a DPI transition border.
            // (Minor: since our sizes do not perfectly linearly scale, deferring the click offset scale until we know the actual window scale ratio may get us slightly more precise mouse positioning.)
            //if (g.MovingWindow != NULL && g.MovingWindow->Viewport == viewport)
            //    g.ActiveIdClickOffset = ImFloor(g.ActiveIdClickOffset * scale_factor);
        }
        viewport->DpiScale = new_dpi_scale;
    }

    if (!(g.ConfigFlagsForFrame & ImGuiConfigFlags_ViewportsEnable))
    {
        g.MouseViewport = main_viewport;
        return;
    }

    // Mouse handling: decide on the actual mouse viewport for this frame between the active/focused viewport and the hovered viewport.
    // Note that 'viewport_hovered' should skip over any viewport that has the ImGuiViewportFlags_NoInputs flags set.
    ImGuiViewportP* viewport_hovered = NULL;
    if (g.IO.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
    {
        viewport_hovered = g.IO.MouseHoveredViewport ? (ImGuiViewportP*)FindViewportByID(g.IO.MouseHoveredViewport) : NULL;
        if (viewport_hovered && (viewport_hovered->Flags & ImGuiViewportFlags_NoInputs))
        {
            // Back-end failed at honoring its contract if it returned a viewport with the _NoInputs flag.
            IM_ASSERT(0);
            viewport_hovered = FindViewportHoveredFromPlatformWindowStack(g.IO.MousePos);
        }
    }
    else
    {
        // If the back-end doesn't know how to honor ImGuiViewportFlags_NoInputs, we do a search ourselves. Note that this search:
        // A) won't take account of the possibility that non-imgui windows may be in-between our dragged window and our target window. 
        // B) uses LastFrameAsRefViewport as a flawed replacement for the last time a window was focused (we could/should fix that by introducing Focus functions in PlatformIO)
        viewport_hovered = FindViewportHoveredFromPlatformWindowStack(g.IO.MousePos);
    }
    if (viewport_hovered != NULL)
        g.MouseLastHoveredViewport = viewport_hovered;
    else if (g.MouseLastHoveredViewport == NULL)
        g.MouseLastHoveredViewport = g.Viewports[0];

    // Update mouse reference viewport
    // (when moving a window we aim at its viewport, but this will be overwritten below if we go in drag and drop mode)
    if (g.MovingWindow)
        g.MouseViewport = g.MovingWindow->Viewport; 
    else
        g.MouseViewport = g.MouseLastHoveredViewport;

    // When dragging something, always refer to the last hovered viewport. 
    // - when releasing a moving window we will revert to aiming behind (at viewport_hovered)
    // - when we are between viewports, our dragged preview will tend to show in the last viewport _even_ if we don't have tooltips in their viewports (when lacking monitor info)
    // - consider the case of holding on a menu item to browse child menus: even thou a mouse button is held, there's no active id because menu items only react on mouse release.
    const bool is_mouse_dragging_with_an_expected_destination = g.DragDropActive;
    if (is_mouse_dragging_with_an_expected_destination && viewport_hovered == NULL)
        viewport_hovered = g.MouseLastHoveredViewport;
    if (is_mouse_dragging_with_an_expected_destination || g.ActiveId == 0 || !IsAnyMouseDown())
        if (viewport_hovered != NULL && viewport_hovered != g.MouseViewport && !(viewport_hovered->Flags & ImGuiViewportFlags_NoInputs))
            g.MouseViewport = viewport_hovered;

    IM_ASSERT(g.MouseViewport != NULL);
}

// Update user-facing viewport list (g.Viewports -> g.PlatformIO.Viewports after filtering out some)
static void ImGui::UpdateViewportsEndFrame()
{
    ImGuiContext& g = *GImGui;
    g.PlatformIO.MainViewport = g.Viewports[0];
    g.PlatformIO.Viewports.resize(0);
    for (int i = 0; i < g.Viewports.Size; i++)
    {
        ImGuiViewportP* viewport = g.Viewports[i];
        viewport->LastPos = viewport->Pos;
        if (viewport->LastFrameActive < g.FrameCount || viewport->Size.x <= 0.0f || viewport->Size.y <= 0.0f)
            if (i > 0) // Always include main viewport in the list
                continue;
        if (viewport->Window && !IsWindowActiveAndVisible(viewport->Window))
            continue;
        if (i > 0)
            IM_ASSERT(viewport->Window != NULL);
        g.PlatformIO.Viewports.push_back(viewport);
    }
    g.Viewports[0]->ClearRequestFlags(); // Clear main viewport flags because UpdatePlatformWindows() won't do it and may not even be called
}

// FIXME: We should ideally refactor the system to call this every frame (we currently don't)
ImGuiViewportP* ImGui::AddUpdateViewport(ImGuiWindow* window, ImGuiID id, const ImVec2& pos, const ImVec2& size, ImGuiViewportFlags flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(id != 0);

    if (window != NULL)
    {
        if (g.MovingWindow && g.MovingWindow->RootWindow == window)
            flags |= ImGuiViewportFlags_NoInputs | ImGuiViewportFlags_NoFocusOnAppearing;
        if ((window->Flags & ImGuiWindowFlags_NoMouseInputs) && (window->Flags & ImGuiWindowFlags_NoNavInputs))
            flags |= ImGuiViewportFlags_NoInputs;
        if (window->Flags & ImGuiWindowFlags_NoFocusOnAppearing)
            flags |= ImGuiViewportFlags_NoFocusOnAppearing;
    }

    ImGuiViewportP* viewport = (ImGuiViewportP*)FindViewportByID(id);
    if (viewport)
    {
        if (!viewport->PlatformRequestMove)
            viewport->Pos = pos;
        if (!viewport->PlatformRequestResize)
            viewport->Size = size;
    }
    else
    {
        // New viewport
        viewport = IM_NEW(ImGuiViewportP)();
        viewport->ID = id;
        viewport->Idx = g.Viewports.Size;
        viewport->Pos = viewport->LastPos = pos;
        viewport->Size = size;
        viewport->PlatformMonitor = (short)FindPlatformMonitorForRect(viewport->GetRect());
        g.Viewports.push_back(viewport);
        //IMGUI_DEBUG_LOG("Add Viewport %08X (%s)\n", id, window->Name);

        // We normally setup for all viewports in NewFrame() but here need to handle the mid-frame creation of a new viewport.
        // We need to extend the fullscreen clip rect so the OverlayDrawList clip is correct for that the first frame
        g.DrawListSharedData.ClipRectFullscreen.z = ImMax(g.DrawListSharedData.ClipRectFullscreen.z, viewport->Pos.x + viewport->Size.x);
        g.DrawListSharedData.ClipRectFullscreen.w = ImMax(g.DrawListSharedData.ClipRectFullscreen.w, viewport->Pos.y + viewport->Size.y);

        // Store initial DpiScale before the OS platform window creation, based on expected monitor data.
        // This is so we can select an appropriate font size on the first frame of our window lifetime
        if (viewport->PlatformMonitor != -1)
            viewport->DpiScale = g.PlatformIO.Monitors[viewport->PlatformMonitor].DpiScale;
    }

    viewport->Window = window;
    viewport->Flags = flags;
    viewport->LastFrameActive = g.FrameCount;
    IM_ASSERT(window == NULL || viewport->ID == window->ID);

    if (window != NULL)
        window->ViewportOwned = true;

    return viewport;
}

// FIXME-VIEWPORT: This is all super messy and ought to be clarified or rewritten.
static void ImGui::UpdateSelectWindowViewport(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindowFlags flags = window->Flags;
    window->ViewportAllowPlatformMonitorExtend = -1;

    // Restore main viewport if multi-viewport is not supported by the back-end
    ImGuiViewportP* main_viewport = g.Viewports[0];
    if (!(g.ConfigFlagsForFrame & ImGuiConfigFlags_ViewportsEnable))
    {
        SetWindowViewport(window, main_viewport);
        return;
    }

    window->ViewportOwned = false;

    // Appearing popups reset their viewport so they can inherit again
    if ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && window->Appearing)
    {
        window->Viewport = NULL;
        window->ViewportId = 0;
    }

    if (!g.NextWindowData.ViewportCond)
    {
        // By default inherit from parent window
        if (window->Viewport == NULL && window->ParentWindow)
            window->Viewport = window->ParentWindow->Viewport;

        // Attempt to restore saved viewport id (= window that hasn't been activated yet), try to restore the viewport based on saved 'window->ViewportPos' restored from .ini file
        if (window->Viewport == NULL && window->ViewportId != 0)
        {
            window->Viewport = (ImGuiViewportP*)FindViewportByID(window->ViewportId);
            if (window->Viewport == NULL && window->ViewportPos.x != FLT_MAX && window->ViewportPos.y != FLT_MAX)
                window->Viewport = AddUpdateViewport(window, window->ID, window->ViewportPos, window->Size, ImGuiViewportFlags_None);
        }
    }

    if (g.NextWindowData.ViewportCond)
    {
        // Code explicitly request a viewport
        window->Viewport = (ImGuiViewportP*)FindViewportByID(g.NextWindowData.ViewportId);
        window->ViewportId = g.NextWindowData.ViewportId; // Store ID even if Viewport isn't resolved yet.
    }
    else if ((flags & ImGuiWindowFlags_ChildWindow) || (flags & ImGuiWindowFlags_ChildMenu))
    {
        // Always inherit viewport from parent window
        window->Viewport = window->ParentWindow->Viewport;
    }
    else if (flags & ImGuiWindowFlags_Tooltip)
    {
        window->Viewport = g.MouseViewport;
    }
    else if (GetWindowAlwaysWantOwnViewport(window))
    {
        window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_None);
    }
    else if (g.MovingWindow && g.MovingWindow->RootWindow == window && IsMousePosValid())
    {
        if (window->Viewport != NULL && window->Viewport->Window == window)
            window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_None);
    }
    else
    {
        // Merge into host viewport?
        // We cannot test window->ViewportOwned as it set lower in the function.
        bool try_to_merge_into_host_viewport = (window->Viewport && window == window->Viewport->Window && g.ActiveId == 0);
        if (try_to_merge_into_host_viewport)
            UpdateTryMergeWindowIntoHostViewport(window, g.Viewports[0]);
    }

    // Fallback to default viewport
    if (window->Viewport == NULL)
        window->Viewport = main_viewport;

    // Mark window as allowed to protrude outside of its viewport and into the current monitor
    if (flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup))
    {
        // We need to take account of the possibility that mouse may become invalid.
        // Popups/Tooltip always set ViewportAllowPlatformMonitorExtend so GetWindowAllowedExtentRect() will return full monitor bounds.
        ImVec2 mouse_ref = (flags & ImGuiWindowFlags_Tooltip) ? g.IO.MousePos : g.BeginPopupStack.back().OpenMousePos;
        bool use_mouse_ref = (g.NavDisableHighlight || !g.NavDisableMouseHover || !g.NavWindow);
        bool mouse_valid = IsMousePosValid(&mouse_ref);
        if ((window->Appearing || (flags & ImGuiWindowFlags_Tooltip)) && (!use_mouse_ref || mouse_valid))
            window->ViewportAllowPlatformMonitorExtend = FindPlatformMonitorForPos((use_mouse_ref && mouse_valid) ? mouse_ref : NavCalcPreferredRefPos());
        else 
            window->ViewportAllowPlatformMonitorExtend = window->Viewport->PlatformMonitor;
    }
    else if (window->Viewport && window != window->Viewport->Window && window->Viewport->Window && !(flags & ImGuiWindowFlags_ChildWindow))
    {
        // When called from Begin() we don't have access to a proper version of the Hidden flag yet, so we replicate this code.
        const bool will_be_visible = (window->DockIsActive && !window->DockTabIsVisible) ? false : true;
        if ((window->Flags & ImGuiWindowFlags_DockNodeHost) && window->Viewport->LastFrameActive < g.FrameCount && will_be_visible)
        {
            // Steal/transfer ownership
            //IMGUI_DEBUG_LOG("[%05d] Window '%s' steal Viewport %08X from Window '%s'\n", g.FrameCount, window->Name, window->Viewport->ID, window->Viewport->Window->Name);
            window->Viewport->Window = window;
            window->Viewport->ID = window->ID;
            window->Viewport->LastNameHash = 0;
        }
        else if (!UpdateTryMergeWindowIntoHostViewport(window, g.Viewports[0])) // Merge?
        {
            // New viewport
            window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_NoFocusOnAppearing);
        }
    }

    // Regular (non-child, non-popup) windows by default are also allowed to protrude
    // Child windows are kept contained within their parent.
    else if (window->ViewportAllowPlatformMonitorExtend < 0 && (flags & ImGuiWindowFlags_ChildWindow) == 0)
        window->ViewportAllowPlatformMonitorExtend = window->Viewport->PlatformMonitor;

    // Update flags
    window->ViewportOwned = (window == window->Viewport->Window);

    // If the OS window has a title bar, hide our imgui title bar
    //if (window->ViewportOwned && !(window->Viewport->Flags & ImGuiViewportFlags_NoDecoration))
    //    window->Flags |= ImGuiWindowFlags_NoTitleBar;

    window->ViewportId = window->Viewport->ID;
}

// Called by user at the end of the main loop, after EndFrame()
// This will handle the creation/update of all OS windows via function defined in the ImGuiPlatformIO api.
void ImGui::UpdatePlatformWindows()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.FrameCountEnded == g.FrameCount && "Forgot to call Render() or EndFrame() before UpdatePlatformWindows()?");
    IM_ASSERT(g.FrameCountPlatformEnded < g.FrameCount);
    g.FrameCountPlatformEnded = g.FrameCount;
    if (!(g.ConfigFlagsForFrame & ImGuiConfigFlags_ViewportsEnable))
        return;

    // Create/resize/destroy platform windows to match each active viewport.
    // Skip the main viewport (index 0), which is always fully handled by the application!
    for (int i = 1; i < g.Viewports.Size; i++)
    {
        ImGuiViewportP* viewport = g.Viewports[i];

        // Destroy platform window if the viewport hasn't been submitted or if it is hosting a hidden window (the implicit/fallback Debug window will be registered its viewport then be disabled)
        bool destroy_platform_window = false;
        destroy_platform_window |= (viewport->LastFrameActive < g.FrameCount - 1);
        destroy_platform_window |= (viewport->Window && !IsWindowActiveAndVisible(viewport->Window));
        if (destroy_platform_window)
        {
            DestroyPlatformWindow(viewport);
            continue;
        }

        // New windows that appears directly in a new viewport won't always have a size on their first frame
        if (viewport->LastFrameActive < g.FrameCount || viewport->Size.x <= 0 || viewport->Size.y <= 0)
            continue;

        // Create window
        bool is_new_platform_window = (viewport->PlatformWindowCreated == false);
        if (is_new_platform_window)
        {
            //IMGUI_DEBUG_LOG("Create Platform Window %08X (%s)\n", viewport->ID, viewport->Window ? viewport->Window->Name : "n/a");
            g.PlatformIO.Platform_CreateWindow(viewport);
            if (g.PlatformIO.Renderer_CreateWindow != NULL)
                g.PlatformIO.Renderer_CreateWindow(viewport);
            viewport->LastNameHash = 0;
            viewport->LastPlatformPos = viewport->LastPlatformSize = ImVec2(FLT_MAX, FLT_MAX); // By clearing those we'll enforce a call to Platform_SetWindowPos/Size below, before Platform_ShowWindow (FIXME: Is that necessary?)
            viewport->LastRendererSize = viewport->Size;                                       // We don't need to call Renderer_SetWindowSize() as it is expected Renderer_CreateWindow() already did it.
            viewport->PlatformWindowCreated = true;
        }

        // Apply Position and Size (from ImGui to Platform/Renderer back-ends)
        if ((viewport->LastPlatformPos.x != viewport->Pos.x || viewport->LastPlatformPos.y != viewport->Pos.y) && !viewport->PlatformRequestMove)
            g.PlatformIO.Platform_SetWindowPos(viewport, viewport->Pos);
        if ((viewport->LastPlatformSize.x != viewport->Size.x || viewport->LastPlatformSize.y != viewport->Size.y) && !viewport->PlatformRequestResize)
            g.PlatformIO.Platform_SetWindowSize(viewport, viewport->Size);
        if ((viewport->LastRendererSize.x != viewport->Size.x || viewport->LastRendererSize.y != viewport->Size.y) && g.PlatformIO.Renderer_SetWindowSize)
            g.PlatformIO.Renderer_SetWindowSize(viewport, viewport->Size);
        viewport->LastPlatformPos = viewport->Pos;
        viewport->LastPlatformSize = viewport->LastRendererSize = viewport->Size;

        // Update title bar (if it changed)
        if (ImGuiWindow* window_for_title = GetWindowForTitleDisplay(viewport->Window))
        {
            const char* title_begin = window_for_title->Name;
            char* title_end = (char*)(intptr_t)FindRenderedTextEnd(title_begin);
            const ImGuiID title_hash = ImHash(title_begin, (int)(title_end - title_begin));
            if (viewport->LastNameHash != title_hash)
            {
                char title_end_backup_c = *title_end;
                *title_end = 0; // Cut existing buffer short instead of doing an alloc/free, no small gain.
                g.PlatformIO.Platform_SetWindowTitle(viewport, title_begin);
                *title_end = title_end_backup_c;
                viewport->LastNameHash = title_hash;
            }
        }

        // Update alpha (if it changed)
        if (viewport->LastAlpha != viewport->Alpha && g.PlatformIO.Platform_SetWindowAlpha)
            g.PlatformIO.Platform_SetWindowAlpha(viewport, viewport->Alpha);
        viewport->LastAlpha = viewport->Alpha;

        // Optional, general purpose call to allow the back-end to perform general book-keeping even if things haven't changed.
        if (g.PlatformIO.Platform_UpdateWindow)
            g.PlatformIO.Platform_UpdateWindow(viewport);

        if (is_new_platform_window)
        {
            // On startup ensure new platform window don't steal focus (give it a few frames, as nested contents may lead to viewport being created a few frames late)
            if (g.FrameCount < 3)
                viewport->Flags |= ImGuiViewportFlags_NoFocusOnAppearing;

            // Show window
            g.PlatformIO.Platform_ShowWindow(viewport);

            // Even without focus, we assume the window becomes front-most. 
            // This is useful for our platform z-order heuristic when io.MouseHoveredViewport is not available.
            if (viewport->LastFrontMostStampCount != g.WindowsFrontMostStampCount)
                viewport->LastFrontMostStampCount = ++g.WindowsFrontMostStampCount;
        }

        // Clear request flags
        viewport->ClearRequestFlags();
    }

    // Update our implicit z-order knowledge of platform windows, which is used when the back-end cannot provide io.MouseHoveredViewport.
    // When setting Platform_GetWindowFocus, it is expected that the platform back-end can handle calls without crashing if it doesn't have data stored. 
    if (g.PlatformIO.Platform_GetWindowFocus != NULL)
    {
        ImGuiViewportP* focused_viewport = NULL;
        for (int n = 0; n < g.Viewports.Size && focused_viewport == NULL; n++)
        {
            ImGuiViewportP* viewport = g.Viewports[n];
            if (n == 0 || viewport->PlatformWindowCreated)
                if (g.PlatformIO.Platform_GetWindowFocus(viewport))
                    focused_viewport = viewport;
        }
        if (focused_viewport && g.PlatformLastFocusedViewport != focused_viewport->ID)
        {
            if (focused_viewport->LastFrontMostStampCount != g.WindowsFrontMostStampCount)
                focused_viewport->LastFrontMostStampCount = ++g.WindowsFrontMostStampCount;
            g.PlatformLastFocusedViewport = focused_viewport->ID;
        }
    }
}

// This is a default/basic function for performing the rendering/swap of multiple Platform Windows.
// Custom renderers may prefer to not call this function at all, and instead iterate the publicly exposed platform data and handle rendering/sync themselves.
// The Render/Swap functions stored in ImGuiPlatformIO are merely here to allow for this helper to exist, but you can do it yourself:
//
//    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
//    for (int i = 1; i < platform_io.Viewports.Size; i++)
//        MyRenderFunction(platform_io.Viewports[i], my_args);
//    for (int i = 1; i < platform_io.Viewports.Size; i++)
//        MySwapBufferFunction(platform_io.Viewports[i], my_args);
//
void ImGui::RenderPlatformWindowsDefault(void* platform_render_arg, void* renderer_render_arg)
{
    // Skip the main viewport (index 0), which is always fully handled by the application!
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int i = 1; i < platform_io.Viewports.Size; i++)
    {
        ImGuiViewport* viewport = platform_io.Viewports[i];
        if (platform_io.Platform_RenderWindow) platform_io.Platform_RenderWindow(viewport, platform_render_arg);
        if (platform_io.Renderer_RenderWindow) platform_io.Renderer_RenderWindow(viewport, renderer_render_arg);
    }
    for (int i = 1; i < platform_io.Viewports.Size; i++)
    {
        ImGuiViewport* viewport = platform_io.Viewports[i];
        if (platform_io.Platform_SwapBuffers) platform_io.Platform_SwapBuffers(viewport, platform_render_arg);
        if (platform_io.Renderer_SwapBuffers) platform_io.Renderer_SwapBuffers(viewport, renderer_render_arg);
    }
}

static int ImGui::FindPlatformMonitorForPos(const ImVec2& pos)
{
    ImGuiContext& g = *GImGui;
    for (int monitor_n = 0; monitor_n < g.PlatformIO.Monitors.Size; monitor_n++)
    {
        const ImGuiPlatformMonitor& monitor = g.PlatformIO.Monitors[monitor_n];
        if (ImRect(monitor.MainPos, monitor.MainPos + monitor.MainSize).Contains(pos))
            return monitor_n;
    }
    return -1;
}

// Search for the monitor with the largest intersection area with the given rectangle
// We generally try to avoid searching loops but the monitor count should be very small here
static int ImGui::FindPlatformMonitorForRect(const ImRect& rect)
{
    ImGuiContext& g = *GImGui;

    // Use a minimum threshold of 1.0f so a zero-sized rect will still find its monitor given its position. 
    // This is necessary for tooltips which always resize down to zero at first.
    const float surface_threshold = ImMax(rect.GetWidth() * rect.GetHeight() * 0.5f, 1.0f);
    int best_monitor_n = -1;
    float best_monitor_surface = 0.001f;

    for (int monitor_n = 0; monitor_n < g.PlatformIO.Monitors.Size && best_monitor_surface < surface_threshold; monitor_n++)
    {
        const ImGuiPlatformMonitor& monitor = g.PlatformIO.Monitors[monitor_n];
        const ImRect monitor_rect = ImRect(monitor.MainPos, monitor.MainPos + monitor.MainSize);
        if (monitor_rect.Contains(rect))
            return monitor_n;
        ImRect overlapping_rect = rect;
        overlapping_rect.ClipWithFull(monitor_rect);
        float overlapping_surface = overlapping_rect.GetWidth() * overlapping_rect.GetHeight();
        if (overlapping_surface < best_monitor_surface)
            continue;
        best_monitor_surface = overlapping_surface;
        best_monitor_n = monitor_n;
    }
    return best_monitor_n;
}

void ImGui::DestroyPlatformWindow(ImGuiViewportP* viewport)
{
    ImGuiContext& g = *GImGui;
    if (g.PlatformIO.Renderer_DestroyWindow)
        g.PlatformIO.Renderer_DestroyWindow(viewport);
    if (g.PlatformIO.Platform_DestroyWindow)
        g.PlatformIO.Platform_DestroyWindow(viewport);
    IM_ASSERT(viewport->RendererUserData == NULL);
    IM_ASSERT(viewport->PlatformUserData == NULL);
    viewport->PlatformHandle = NULL;
    viewport->RendererUserData = viewport->PlatformHandle = NULL;
    viewport->PlatformWindowCreated = false;
    viewport->ClearRequestFlags();
}

void ImGui::DestroyPlatformWindows()
{
    // We call the destroy window on every viewport (including the main viewport, index 0) to give a chance to the back-end 
    // to clear any data they may have stored in e.g. PlatformUserData, RendererUserData. 
    // It is convenient for the platform back-end code to store something in the main viewport, in order for e.g. the mouse handling 
    // code to operator a consistent manner.
    // It is expected that the back-end can handle calls to Renderer_DestroyWindow/Platform_DestroyWindow without
    // crashing if it doesn't have data stored. 
    ImGuiContext& g = *GImGui;
    for (int i = 0; i < g.Viewports.Size; i++)
        DestroyPlatformWindow(g.Viewports[i]);
}

//-----------------------------------------------------------------------------
// [SECTION] KEYBOARD/GAMEPAD NAVIGATION
//-----------------------------------------------------------------------------

ImGuiDir ImGetDirQuadrantFromDelta(float dx, float dy)
{
    if (ImFabs(dx) > ImFabs(dy))
        return (dx > 0.0f) ? ImGuiDir_Right : ImGuiDir_Left;
    return (dy > 0.0f) ? ImGuiDir_Down : ImGuiDir_Up;
}

static float inline NavScoreItemDistInterval(float a0, float a1, float b0, float b1)
{
    if (a1 < b0)
        return a1 - b0;
    if (b1 < a0)
        return a0 - b1;
    return 0.0f;
}

static void inline NavClampRectToVisibleAreaForMoveDir(ImGuiDir move_dir, ImRect& r, const ImRect& clip_rect)
{
    if (move_dir == ImGuiDir_Left || move_dir == ImGuiDir_Right)
    {
        r.Min.y = ImClamp(r.Min.y, clip_rect.Min.y, clip_rect.Max.y);
        r.Max.y = ImClamp(r.Max.y, clip_rect.Min.y, clip_rect.Max.y);
    }
    else
    {
        r.Min.x = ImClamp(r.Min.x, clip_rect.Min.x, clip_rect.Max.x);
        r.Max.x = ImClamp(r.Max.x, clip_rect.Min.x, clip_rect.Max.x);
    }
}

// Scoring function for directional navigation. Based on https://gist.github.com/rygorous/6981057
static bool NavScoreItem(ImGuiNavMoveResult* result, ImRect cand)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.NavLayer != window->DC.NavLayerCurrent)
        return false;

    const ImRect& curr = g.NavScoringRectScreen; // Current modified source rect (NB: we've applied Max.x = Min.x in NavUpdate() to inhibit the effect of having varied item width)
    g.NavScoringCount++;

    // When entering through a NavFlattened border, we consider child window items as fully clipped for scoring
    if (window->ParentWindow == g.NavWindow)
    {
        IM_ASSERT((window->Flags | g.NavWindow->Flags) & ImGuiWindowFlags_NavFlattened);
        if (!window->ClipRect.Contains(cand))
            return false;
        cand.ClipWithFull(window->ClipRect); // This allows the scored item to not overlap other candidates in the parent window
    }

    // We perform scoring on items bounding box clipped by the current clipping rectangle on the other axis (clipping on our movement axis would give us equal scores for all clipped items)
    // For example, this ensure that items in one column are not reached when moving vertically from items in another column.
    NavClampRectToVisibleAreaForMoveDir(g.NavMoveClipDir, cand, window->ClipRect);

    // Compute distance between boxes
    // FIXME-NAV: Introducing biases for vertical navigation, needs to be removed.
    float dbx = NavScoreItemDistInterval(cand.Min.x, cand.Max.x, curr.Min.x, curr.Max.x);
    float dby = NavScoreItemDistInterval(ImLerp(cand.Min.y, cand.Max.y, 0.2f), ImLerp(cand.Min.y, cand.Max.y, 0.8f), ImLerp(curr.Min.y, curr.Max.y, 0.2f), ImLerp(curr.Min.y, curr.Max.y, 0.8f)); // Scale down on Y to keep using box-distance for vertically touching items
    if (dby != 0.0f && dbx != 0.0f)
       dbx = (dbx/1000.0f) + ((dbx > 0.0f) ? +1.0f : -1.0f);
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
        quadrant = (window->DC.LastItemId < g.NavId) ? ImGuiDir_Left : ImGuiDir_Right;
    }

#if IMGUI_DEBUG_NAV_SCORING
    char buf[128];
    if (ImGui::IsMouseHoveringRect(cand.Min, cand.Max))
    {
        ImFormatString(buf, IM_ARRAYSIZE(buf), "dbox (%.2f,%.2f->%.4f)\ndcen (%.2f,%.2f->%.4f)\nd (%.2f,%.2f->%.4f)\nnav %c, quadrant %c", dbx, dby, dist_box, dcx, dcy, dist_center, dax, day, dist_axial, "WENS"[g.NavMoveDir], "WENS"[quadrant]);
        ImDrawList* draw_list = ImGui::GetOverlayDrawList(window);
        draw_list->AddRect(curr.Min, curr.Max, IM_COL32(255,200,0,100));
        draw_list->AddRect(cand.Min, cand.Max, IM_COL32(255,255,0,200));
        draw_list->AddRectFilled(cand.Max-ImVec2(4,4), cand.Max+ImGui::CalcTextSize(buf)+ImVec2(4,4), IM_COL32(40,0,0,150));
        draw_list->AddText(g.IO.FontDefault, 13.0f, cand.Max, ~0U, buf);
    }
    else if (g.IO.KeyCtrl) // Hold to preview score in matching quadrant. Press C to rotate.
    {
        if (ImGui::IsKeyPressedMap(ImGuiKey_C)) { g.NavMoveDirLast = (ImGuiDir)((g.NavMoveDirLast + 1) & 3); g.IO.KeysDownDuration[g.IO.KeyMap[ImGuiKey_C]] = 0.01f; }
        if (quadrant == g.NavMoveDir)
        {
            ImFormatString(buf, IM_ARRAYSIZE(buf), "%.0f/%.0f", dist_box, dist_center);
            ImDrawList* draw_list = ImGui::GetOverlayDrawList(window);
            draw_list->AddRectFilled(cand.Min, cand.Max, IM_COL32(255, 0, 0, 200));
            draw_list->AddText(g.IO.FontDefault, 13.0f, cand.Min, IM_COL32(255, 255, 255, 255), buf);
        }
    }
 #endif

    // Is it in the quadrant we're interesting in moving to?
    bool new_best = false;
    if (quadrant == g.NavMoveDir)
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
                if (((g.NavMoveDir == ImGuiDir_Up || g.NavMoveDir == ImGuiDir_Down) ? dby : dbx) < 0.0f) // moving bj to the right/down decreases distance
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
        if (g.NavLayer == 1 && !(g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
            if ((g.NavMoveDir == ImGuiDir_Left && dax < 0.0f) || (g.NavMoveDir == ImGuiDir_Right && dax > 0.0f) || (g.NavMoveDir == ImGuiDir_Up && day < 0.0f) || (g.NavMoveDir == ImGuiDir_Down && day > 0.0f))
            {
                result->DistAxial = dist_axial;
                new_best = true;
            }

    return new_best;
}

// We get there when either NavId == id, or when g.NavAnyRequest is set (which is updated by NavUpdateAnyRequestFlag above)
static void ImGui::NavProcessItem(ImGuiWindow* window, const ImRect& nav_bb, const ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    //if (!g.IO.NavActive)  // [2017/10/06] Removed this possibly redundant test but I am not sure of all the side-effects yet. Some of the feature here will need to work regardless of using a _NoNavInputs flag.
    //    return;

    const ImGuiItemFlags item_flags = window->DC.ItemFlags;
    const ImRect nav_bb_rel(nav_bb.Min - window->Pos, nav_bb.Max - window->Pos);

    // Process Init Request
    if (g.NavInitRequest && g.NavLayer == window->DC.NavLayerCurrent)
    {
        // Even if 'ImGuiItemFlags_NoNavDefaultFocus' is on (typically collapse/close button) we record the first ResultId so they can be used as a fallback
        if (!(item_flags & ImGuiItemFlags_NoNavDefaultFocus) || g.NavInitResultId == 0)
        {
            g.NavInitResultId = id;
            g.NavInitResultRectRel = nav_bb_rel;
        }
        if (!(item_flags & ImGuiItemFlags_NoNavDefaultFocus))
        {
            g.NavInitRequest = false; // Found a match, clear request
            NavUpdateAnyRequestFlag();
        }
    }

    // Process Move Request (scoring for navigation)
    // FIXME-NAV: Consider policy for double scoring (scoring from NavScoringRectScreen + scoring from a rect wrapped according to current wrapping policy)
    if ((g.NavId != id || (g.NavMoveRequestFlags & ImGuiNavMoveFlags_AllowCurrentNavId)) && !(item_flags & ImGuiItemFlags_NoNav))
    {
        ImGuiNavMoveResult* result = (window == g.NavWindow) ? &g.NavMoveResultLocal : &g.NavMoveResultOther;
#if IMGUI_DEBUG_NAV_SCORING
        // [DEBUG] Score all items in NavWindow at all times
        if (!g.NavMoveRequest)
            g.NavMoveDir = g.NavMoveDirLast;
        bool new_best = NavScoreItem(result, nav_bb) && g.NavMoveRequest;
#else
        bool new_best = g.NavMoveRequest && NavScoreItem(result, nav_bb);
#endif
        if (new_best)
        {
            result->ID = id;
            result->Window = window;
            result->RectRel = nav_bb_rel;
        }

        const float VISIBLE_RATIO = 0.70f;
        if ((g.NavMoveRequestFlags & ImGuiNavMoveFlags_AlsoScoreVisibleSet) && window->ClipRect.Overlaps(nav_bb))
            if (ImClamp(nav_bb.Max.y, window->ClipRect.Min.y, window->ClipRect.Max.y) - ImClamp(nav_bb.Min.y, window->ClipRect.Min.y, window->ClipRect.Max.y) >= (nav_bb.Max.y - nav_bb.Min.y) * VISIBLE_RATIO)
                if (NavScoreItem(&g.NavMoveResultLocalVisibleSet, nav_bb))
                {
                    result = &g.NavMoveResultLocalVisibleSet;
                    result->ID = id;
                    result->Window = window;
                    result->RectRel = nav_bb_rel;
                }
    }

    // Update window-relative bounding box of navigated item
    if (g.NavId == id)
    {
        g.NavWindow = window;                                           // Always refresh g.NavWindow, because some operations such as FocusItem() don't have a window.
        g.NavLayer = window->DC.NavLayerCurrent;
        g.NavIdIsAlive = true;
        g.NavIdTabCounter = window->FocusIdxTabCounter;
        window->NavRectRel[window->DC.NavLayerCurrent] = nav_bb_rel;    // Store item bounding box (relative to window position)
    }
}

bool ImGui::NavMoveRequestButNoResultYet()
{
    ImGuiContext& g = *GImGui;
    return g.NavMoveRequest && g.NavMoveResultLocal.ID == 0 && g.NavMoveResultOther.ID == 0;
}

void ImGui::NavMoveRequestCancel()
{
    ImGuiContext& g = *GImGui;
    g.NavMoveRequest = false;
    NavUpdateAnyRequestFlag();
}

void ImGui::NavMoveRequestForward(ImGuiDir move_dir, ImGuiDir clip_dir, const ImRect& bb_rel, ImGuiNavMoveFlags move_flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavMoveRequestForward == ImGuiNavForward_None);
    ImGui::NavMoveRequestCancel();
    g.NavMoveDir = move_dir;
    g.NavMoveClipDir = clip_dir;
    g.NavMoveRequestForward = ImGuiNavForward_ForwardQueued;
    g.NavMoveRequestFlags = move_flags;
    g.NavWindow->NavRectRel[g.NavLayer] = bb_rel;
}

void ImGui::NavMoveRequestTryWrapping(ImGuiWindow* window, ImGuiNavMoveFlags move_flags)
{
    ImGuiContext& g = *GImGui;
    if (g.NavWindow != window || !NavMoveRequestButNoResultYet() || g.NavMoveRequestForward != ImGuiNavForward_None || g.NavLayer != 0)
        return;
    IM_ASSERT(move_flags != 0); // No points calling this with no wrapping
    ImRect bb_rel = window->NavRectRel[0];

    ImGuiDir clip_dir = g.NavMoveDir;
    if (g.NavMoveDir == ImGuiDir_Left && (move_flags & (ImGuiNavMoveFlags_WrapX | ImGuiNavMoveFlags_LoopX)))
    {
        bb_rel.Min.x = bb_rel.Max.x = ImMax(window->SizeFull.x, window->SizeContents.x) - window->Scroll.x;
        if (move_flags & ImGuiNavMoveFlags_WrapX) { bb_rel.TranslateY(-bb_rel.GetHeight()); clip_dir = ImGuiDir_Up; }
        NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
    }
    if (g.NavMoveDir == ImGuiDir_Right && (move_flags & (ImGuiNavMoveFlags_WrapX | ImGuiNavMoveFlags_LoopX)))
    {
        bb_rel.Min.x = bb_rel.Max.x = -window->Scroll.x;
        if (move_flags & ImGuiNavMoveFlags_WrapX) { bb_rel.TranslateY(+bb_rel.GetHeight()); clip_dir = ImGuiDir_Down; }
        NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
    }
    if (g.NavMoveDir == ImGuiDir_Up && (move_flags & (ImGuiNavMoveFlags_WrapY | ImGuiNavMoveFlags_LoopY)))
    {
        bb_rel.Min.y = bb_rel.Max.y = ImMax(window->SizeFull.y, window->SizeContents.y) - window->Scroll.y;
        if (move_flags & ImGuiNavMoveFlags_WrapY) { bb_rel.TranslateX(-bb_rel.GetWidth()); clip_dir = ImGuiDir_Left; }
        NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
    }
    if (g.NavMoveDir == ImGuiDir_Down && (move_flags & (ImGuiNavMoveFlags_WrapY | ImGuiNavMoveFlags_LoopY)))
    {
        bb_rel.Min.y = bb_rel.Max.y = -window->Scroll.y;
        if (move_flags & ImGuiNavMoveFlags_WrapY) { bb_rel.TranslateX(+bb_rel.GetWidth()); clip_dir = ImGuiDir_Right; }
        NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
    }
}

static void ImGui::NavSaveLastChildNavWindow(ImGuiWindow* nav_window)
{
    ImGuiWindow* parent_window = nav_window;
    while (parent_window && (parent_window->Flags & ImGuiWindowFlags_ChildWindow) != 0 && (parent_window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu)) == 0)
        parent_window = parent_window->ParentWindow;
    if (parent_window && parent_window != nav_window)
        parent_window->NavLastChildNavWindow = nav_window;
}

// Call when we are expected to land on Layer 0 after FocusWindow()
static ImGuiWindow* ImGui::NavRestoreLastChildNavWindow(ImGuiWindow* window)
{
    return window->NavLastChildNavWindow ? window->NavLastChildNavWindow : window;
}

static void NavRestoreLayer(ImGuiNavLayer layer)
{
    ImGuiContext& g = *GImGui;
    g.NavLayer = layer;
    if (layer == 0)
        g.NavWindow = ImGui::NavRestoreLastChildNavWindow(g.NavWindow);
    if (g.NavWindow->NavLastIds[layer] != 0)
        ImGui::SetNavIDWithRectRel(g.NavWindow->NavLastIds[layer], layer, g.NavWindow->NavRectRel[layer]);
    else
        ImGui::NavInitWindow(g.NavWindow, true);
}

static inline void ImGui::NavUpdateAnyRequestFlag()
{
    ImGuiContext& g = *GImGui;
    g.NavAnyRequest = g.NavMoveRequest || g.NavInitRequest || (IMGUI_DEBUG_NAV_SCORING && g.NavWindow != NULL);
    if (g.NavAnyRequest)
        IM_ASSERT(g.NavWindow != NULL);
}

// This needs to be called before we submit any widget (aka in or before Begin)
void ImGui::NavInitWindow(ImGuiWindow* window, bool force_reinit)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(window == g.NavWindow);
    bool init_for_nav = false;
    if (!(window->Flags & ImGuiWindowFlags_NoNavInputs))
        if (!(window->Flags & ImGuiWindowFlags_ChildWindow) || (window->Flags & ImGuiWindowFlags_Popup) || (window->NavLastIds[0] == 0) || force_reinit)
            init_for_nav = true;
    if (init_for_nav)
    {
        SetNavID(0, g.NavLayer);
        g.NavInitRequest = true;
        g.NavInitRequestFromMove = false;
        g.NavInitResultId = 0;
        g.NavInitResultRectRel = ImRect();
        NavUpdateAnyRequestFlag();
    }
    else
    {
        g.NavId = window->NavLastIds[0];
    }
}

static ImVec2 ImGui::NavCalcPreferredRefPos()
{
    ImGuiContext& g = *GImGui;
    if (g.NavDisableHighlight || !g.NavDisableMouseHover || !g.NavWindow)
    {
        // Mouse (we need a fallback in case the mouse becomes invalid after being used)
        if (IsMousePosValid(&g.IO.MousePos))
            return g.IO.MousePos;
        return g.LastValidMousePos;
    }
    else
    {
        // When navigation is active and mouse is disabled, decide on an arbitrary position around the bottom left of the currently navigated item.
        const ImRect& rect_rel = g.NavWindow->NavRectRel[g.NavLayer];
        ImVec2 pos = g.NavWindow->Pos + ImVec2(rect_rel.Min.x + ImMin(g.Style.FramePadding.x * 4, rect_rel.GetWidth()), rect_rel.Max.y - ImMin(g.Style.FramePadding.y, rect_rel.GetHeight()));
        ImRect visible_rect = g.NavWindow->Viewport->GetRect();
        return ImFloor(ImClamp(pos, visible_rect.Min, visible_rect.Max));   // ImFloor() is important because non-integer mouse position application in back-end might be lossy and result in undesirable non-zero delta.
    }
}

float ImGui::GetNavInputAmount(ImGuiNavInput n, ImGuiInputReadMode mode)
{
    ImGuiContext& g = *GImGui;
    if (mode == ImGuiInputReadMode_Down)
        return g.IO.NavInputs[n];                         // Instant, read analog input (0.0f..1.0f, as provided by user)

    const float t = g.IO.NavInputsDownDuration[n];
    if (t < 0.0f && mode == ImGuiInputReadMode_Released)  // Return 1.0f when just released, no repeat, ignore analog input.
        return (g.IO.NavInputsDownDurationPrev[n] >= 0.0f ? 1.0f : 0.0f);
    if (t < 0.0f)
        return 0.0f;
    if (mode == ImGuiInputReadMode_Pressed)               // Return 1.0f when just pressed, no repeat, ignore analog input.
        return (t == 0.0f) ? 1.0f : 0.0f;
    if (mode == ImGuiInputReadMode_Repeat)
        return (float)CalcTypematicPressedRepeatAmount(t, t - g.IO.DeltaTime, g.IO.KeyRepeatDelay * 0.80f, g.IO.KeyRepeatRate * 0.80f);
    if (mode == ImGuiInputReadMode_RepeatSlow)
        return (float)CalcTypematicPressedRepeatAmount(t, t - g.IO.DeltaTime, g.IO.KeyRepeatDelay * 1.00f, g.IO.KeyRepeatRate * 2.00f);
    if (mode == ImGuiInputReadMode_RepeatFast)
        return (float)CalcTypematicPressedRepeatAmount(t, t - g.IO.DeltaTime, g.IO.KeyRepeatDelay * 0.80f, g.IO.KeyRepeatRate * 0.30f);
    return 0.0f;
}

ImVec2 ImGui::GetNavInputAmount2d(ImGuiNavDirSourceFlags dir_sources, ImGuiInputReadMode mode, float slow_factor, float fast_factor)
{
    ImVec2 delta(0.0f, 0.0f);
    if (dir_sources & ImGuiNavDirSourceFlags_Keyboard)
        delta += ImVec2(GetNavInputAmount(ImGuiNavInput_KeyRight_, mode)   - GetNavInputAmount(ImGuiNavInput_KeyLeft_,   mode), GetNavInputAmount(ImGuiNavInput_KeyDown_,   mode) - GetNavInputAmount(ImGuiNavInput_KeyUp_,   mode));
    if (dir_sources & ImGuiNavDirSourceFlags_PadDPad)
        delta += ImVec2(GetNavInputAmount(ImGuiNavInput_DpadRight, mode)   - GetNavInputAmount(ImGuiNavInput_DpadLeft,   mode), GetNavInputAmount(ImGuiNavInput_DpadDown,   mode) - GetNavInputAmount(ImGuiNavInput_DpadUp,   mode));
    if (dir_sources & ImGuiNavDirSourceFlags_PadLStick)
        delta += ImVec2(GetNavInputAmount(ImGuiNavInput_LStickRight, mode) - GetNavInputAmount(ImGuiNavInput_LStickLeft, mode), GetNavInputAmount(ImGuiNavInput_LStickDown, mode) - GetNavInputAmount(ImGuiNavInput_LStickUp, mode));
    if (slow_factor != 0.0f && IsNavInputDown(ImGuiNavInput_TweakSlow))
        delta *= slow_factor;
    if (fast_factor != 0.0f && IsNavInputDown(ImGuiNavInput_TweakFast))
        delta *= fast_factor;
    return delta;
}

// Scroll to keep newly navigated item fully into view
// NB: We modify rect_rel by the amount we scrolled for, so it is immediately updated.
static void NavScrollToBringItemIntoView(ImGuiWindow* window, const ImRect& item_rect)
{
    ImRect window_rect(window->InnerMainRect.Min - ImVec2(1, 1), window->InnerMainRect.Max + ImVec2(1, 1));
    //GetOverlayDrawList(window)->AddRect(window_rect.Min, window_rect.Max, IM_COL32_WHITE); // [DEBUG]
    if (window_rect.Contains(item_rect))
        return;

    ImGuiContext& g = *GImGui;
    if (window->ScrollbarX && item_rect.Min.x < window_rect.Min.x)
    {
        window->ScrollTarget.x = item_rect.Min.x - window->Pos.x + window->Scroll.x - g.Style.ItemSpacing.x;
        window->ScrollTargetCenterRatio.x = 0.0f;
    }
    else if (window->ScrollbarX && item_rect.Max.x >= window_rect.Max.x)
    {
        window->ScrollTarget.x = item_rect.Max.x - window->Pos.x + window->Scroll.x + g.Style.ItemSpacing.x;
        window->ScrollTargetCenterRatio.x = 1.0f;
    }
    if (item_rect.Min.y < window_rect.Min.y)
    {
        window->ScrollTarget.y = item_rect.Min.y - window->Pos.y + window->Scroll.y - g.Style.ItemSpacing.y;
        window->ScrollTargetCenterRatio.y = 0.0f;
    }
    else if (item_rect.Max.y >= window_rect.Max.y)
    {
        window->ScrollTarget.y = item_rect.Max.y - window->Pos.y + window->Scroll.y + g.Style.ItemSpacing.y;
        window->ScrollTargetCenterRatio.y = 1.0f;
    }
}

static void ImGui::NavUpdate()
{
    ImGuiContext& g = *GImGui;
    g.IO.WantSetMousePos = false;
#if 0
    if (g.NavScoringCount > 0) IMGUI_DEBUG_LOG("NavScoringCount %d for '%s' layer %d (Init:%d, Move:%d)\n", g.FrameCount, g.NavScoringCount, g.NavWindow ? g.NavWindow->Name : "NULL", g.NavLayer, g.NavInitRequest || g.NavInitResultId != 0, g.NavMoveRequest);
#endif

    // Set input source as Gamepad when buttons are pressed before we map Keyboard (some features differs when used with Gamepad vs Keyboard)
    bool nav_keyboard_active = (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
    bool nav_gamepad_active = (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (g.IO.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
    if (nav_gamepad_active)
        if (g.IO.NavInputs[ImGuiNavInput_Activate] > 0.0f || g.IO.NavInputs[ImGuiNavInput_Input] > 0.0f || g.IO.NavInputs[ImGuiNavInput_Cancel] > 0.0f || g.IO.NavInputs[ImGuiNavInput_Menu] > 0.0f)
            g.NavInputSource = ImGuiInputSource_NavGamepad;

    // Update Keyboard->Nav inputs mapping
    if (nav_keyboard_active)
    {
        #define NAV_MAP_KEY(_KEY, _NAV_INPUT) if (IsKeyDown(g.IO.KeyMap[_KEY])) { g.IO.NavInputs[_NAV_INPUT] = 1.0f; g.NavInputSource = ImGuiInputSource_NavKeyboard; }
        NAV_MAP_KEY(ImGuiKey_Space,     ImGuiNavInput_Activate );
        NAV_MAP_KEY(ImGuiKey_Enter,     ImGuiNavInput_Input    );
        NAV_MAP_KEY(ImGuiKey_Escape,    ImGuiNavInput_Cancel   );
        NAV_MAP_KEY(ImGuiKey_LeftArrow, ImGuiNavInput_KeyLeft_ );
        NAV_MAP_KEY(ImGuiKey_RightArrow,ImGuiNavInput_KeyRight_);
        NAV_MAP_KEY(ImGuiKey_UpArrow,   ImGuiNavInput_KeyUp_   );
        NAV_MAP_KEY(ImGuiKey_DownArrow, ImGuiNavInput_KeyDown_ );
        if (g.IO.KeyCtrl)   g.IO.NavInputs[ImGuiNavInput_TweakSlow] = 1.0f;
        if (g.IO.KeyShift)  g.IO.NavInputs[ImGuiNavInput_TweakFast] = 1.0f;
        if (g.IO.KeyAlt)    g.IO.NavInputs[ImGuiNavInput_KeyMenu_]  = 1.0f;
        #undef NAV_MAP_KEY
    }
    memcpy(g.IO.NavInputsDownDurationPrev, g.IO.NavInputsDownDuration, sizeof(g.IO.NavInputsDownDuration));
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.NavInputs); i++)
        g.IO.NavInputsDownDuration[i] = (g.IO.NavInputs[i] > 0.0f) ? (g.IO.NavInputsDownDuration[i] < 0.0f ? 0.0f : g.IO.NavInputsDownDuration[i] + g.IO.DeltaTime) : -1.0f;

    // Process navigation init request (select first/default focus)
    if (g.NavInitResultId != 0 && (!g.NavDisableHighlight || g.NavInitRequestFromMove))
    {
        // Apply result from previous navigation init request (will typically select the first item, unless SetItemDefaultFocus() has been called)
        IM_ASSERT(g.NavWindow);
        if (g.NavInitRequestFromMove)
            SetNavIDWithRectRel(g.NavInitResultId, g.NavLayer, g.NavInitResultRectRel);
        else
            SetNavID(g.NavInitResultId, g.NavLayer);
        g.NavWindow->NavRectRel[g.NavLayer] = g.NavInitResultRectRel;
    }
    g.NavInitRequest = false;
    g.NavInitRequestFromMove = false;
    g.NavInitResultId = 0;
    g.NavJustMovedToId = 0;

    // Process navigation move request
    if (g.NavMoveRequest)
        NavUpdateMoveResult();

    // When a forwarded move request failed, we restore the highlight that we disabled during the forward frame
    if (g.NavMoveRequestForward == ImGuiNavForward_ForwardActive)
    {
        IM_ASSERT(g.NavMoveRequest);
        if (g.NavMoveResultLocal.ID == 0 && g.NavMoveResultOther.ID == 0)
            g.NavDisableHighlight = false;
        g.NavMoveRequestForward = ImGuiNavForward_None;
    }

    // Apply application mouse position movement, after we had a chance to process move request result.
    if (g.NavMousePosDirty && g.NavIdIsAlive)
    {
        // Set mouse position given our knowledge of the navigated item position from last frame
        if ((g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableSetMousePos) && (g.IO.BackendFlags & ImGuiBackendFlags_HasSetMousePos))
        {
            if (!g.NavDisableHighlight && g.NavDisableMouseHover && g.NavWindow)
            {
                g.IO.MousePos = g.IO.MousePosPrev = NavCalcPreferredRefPos();
                g.IO.WantSetMousePos = true;
            }
        }
        g.NavMousePosDirty = false;
    }
    g.NavIdIsAlive = false;
    g.NavJustTabbedId = 0;
    IM_ASSERT(g.NavLayer == 0 || g.NavLayer == 1);

    // Store our return window (for returning from Layer 1 to Layer 0) and clear it as soon as we step back in our own Layer 0
    if (g.NavWindow)
        NavSaveLastChildNavWindow(g.NavWindow);
    if (g.NavWindow && g.NavWindow->NavLastChildNavWindow != NULL && g.NavLayer == 0)
        g.NavWindow->NavLastChildNavWindow = NULL;

    // Update CTRL+TAB and Windowing features (hold Square to move/resize/etc.)
    NavUpdateWindowing();

    // Set output flags for user application
    // FIXME: g.NavInitRequest is always false at this point, investigate the intent of operation done here.
    g.IO.NavActive = (nav_keyboard_active || nav_gamepad_active) && g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs);
    g.IO.NavVisible = (g.IO.NavActive && g.NavId != 0 && !g.NavDisableHighlight) || (g.NavWindowingTarget != NULL) || g.NavInitRequest;

    // Process NavCancel input (to close a popup, get back to parent, clear focus)
    if (IsNavInputPressed(ImGuiNavInput_Cancel, ImGuiInputReadMode_Pressed))
    {
        if (g.ActiveId != 0)
        {
            ClearActiveID();
        }
        else if (g.NavWindow && (g.NavWindow->Flags & ImGuiWindowFlags_ChildWindow) && !(g.NavWindow->Flags & ImGuiWindowFlags_Popup) && g.NavWindow->ParentWindow && g.NavWindow != g.NavWindow->RootWindowDockStop)
        {
            // Exit child window
            ImGuiWindow* child_window = g.NavWindow;
            ImGuiWindow* parent_window = g.NavWindow->ParentWindow;
            IM_ASSERT(child_window->ChildId != 0);
            FocusWindow(parent_window);
            SetNavID(child_window->ChildId, 0);
            g.NavIdIsAlive = false;
            if (g.NavDisableMouseHover)
                g.NavMousePosDirty = true;
        }
        else if (g.OpenPopupStack.Size > 0)
        {
            // Close open popup/menu
            if (!(g.OpenPopupStack.back().Window->Flags & ImGuiWindowFlags_Modal))
                ClosePopupToLevel(g.OpenPopupStack.Size - 1, true);
        }
        else if (g.NavLayer != 0)
        {
            // Leave the "menu" layer
            NavRestoreLayer(ImGuiNavLayer_Main);
        }
        else
        {
            // Clear NavLastId for popups but keep it for regular child window so we can leave one and come back where we were
            if (g.NavWindow && ((g.NavWindow->Flags & ImGuiWindowFlags_Popup) || !(g.NavWindow->Flags & ImGuiWindowFlags_ChildWindow)))
                g.NavWindow->NavLastIds[0] = 0;
            g.NavId = 0;
        }
    }

    // Process manual activation request
    g.NavActivateId = g.NavActivateDownId = g.NavActivatePressedId = g.NavInputId = 0;
    if (g.NavId != 0 && !g.NavDisableHighlight && !g.NavWindowingTarget && g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
    {
        bool activate_down = IsNavInputDown(ImGuiNavInput_Activate);
        bool activate_pressed = activate_down && IsNavInputPressed(ImGuiNavInput_Activate, ImGuiInputReadMode_Pressed);
        if (g.ActiveId == 0 && activate_pressed)
            g.NavActivateId = g.NavId;
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && activate_down)
            g.NavActivateDownId = g.NavId;
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && activate_pressed)
            g.NavActivatePressedId = g.NavId;
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && IsNavInputPressed(ImGuiNavInput_Input, ImGuiInputReadMode_Pressed))
            g.NavInputId = g.NavId;
    }
    if (g.NavWindow && (g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
        g.NavDisableHighlight = true;
    if (g.NavActivateId != 0)
        IM_ASSERT(g.NavActivateDownId == g.NavActivateId);
    g.NavMoveRequest = false;

    // Process programmatic activation request
    if (g.NavNextActivateId != 0)
        g.NavActivateId = g.NavActivateDownId = g.NavActivatePressedId = g.NavInputId = g.NavNextActivateId;
    g.NavNextActivateId = 0;

    // Initiate directional inputs request
    const int allowed_dir_flags = (g.ActiveId == 0) ? ~0 : g.ActiveIdAllowNavDirFlags;
    if (g.NavMoveRequestForward == ImGuiNavForward_None)
    {
        g.NavMoveDir = ImGuiDir_None;
        g.NavMoveRequestFlags = ImGuiNavMoveFlags_None;
        if (g.NavWindow && !g.NavWindowingTarget && allowed_dir_flags && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
        {
            if ((allowed_dir_flags & (1<<ImGuiDir_Left))  && IsNavInputPressedAnyOfTwo(ImGuiNavInput_DpadLeft, ImGuiNavInput_KeyLeft_, ImGuiInputReadMode_Repeat)) g.NavMoveDir = ImGuiDir_Left;
            if ((allowed_dir_flags & (1<<ImGuiDir_Right)) && IsNavInputPressedAnyOfTwo(ImGuiNavInput_DpadRight,ImGuiNavInput_KeyRight_,ImGuiInputReadMode_Repeat)) g.NavMoveDir = ImGuiDir_Right;
            if ((allowed_dir_flags & (1<<ImGuiDir_Up))    && IsNavInputPressedAnyOfTwo(ImGuiNavInput_DpadUp,   ImGuiNavInput_KeyUp_,   ImGuiInputReadMode_Repeat)) g.NavMoveDir = ImGuiDir_Up;
            if ((allowed_dir_flags & (1<<ImGuiDir_Down))  && IsNavInputPressedAnyOfTwo(ImGuiNavInput_DpadDown, ImGuiNavInput_KeyDown_, ImGuiInputReadMode_Repeat)) g.NavMoveDir = ImGuiDir_Down;
        }
        g.NavMoveClipDir = g.NavMoveDir;
    }
    else
    {
        // Forwarding previous request (which has been modified, e.g. wrap around menus rewrite the requests with a starting rectangle at the other side of the window)
        // (Preserve g.NavMoveRequestFlags, g.NavMoveClipDir which were set by the NavMoveRequestForward() function)
        IM_ASSERT(g.NavMoveDir != ImGuiDir_None && g.NavMoveClipDir != ImGuiDir_None);
        IM_ASSERT(g.NavMoveRequestForward == ImGuiNavForward_ForwardQueued);
        g.NavMoveRequestForward = ImGuiNavForward_ForwardActive;
    }

    // Update PageUp/PageDown scroll
    float nav_scoring_rect_offset_y = 0.0f;
    if (nav_keyboard_active)
        nav_scoring_rect_offset_y = NavUpdatePageUpPageDown(allowed_dir_flags);

    // If we initiate a movement request and have no current NavId, we initiate a InitDefautRequest that will be used as a fallback if the direction fails to find a match
    if (g.NavMoveDir != ImGuiDir_None)
    {
        g.NavMoveRequest = true;
        g.NavMoveDirLast = g.NavMoveDir;
    }
    if (g.NavMoveRequest && g.NavId == 0)
    {
        g.NavInitRequest = g.NavInitRequestFromMove = true;
        g.NavInitResultId = 0;
        g.NavDisableHighlight = false;
    }
    NavUpdateAnyRequestFlag();

    // Scrolling
    if (g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs) && !g.NavWindowingTarget)
    {
        // *Fallback* manual-scroll with Nav directional keys when window has no navigable item
        ImGuiWindow* window = g.NavWindow;
        const float scroll_speed = ImFloor(window->CalcFontSize() * 100 * g.IO.DeltaTime + 0.5f); // We need round the scrolling speed because sub-pixel scroll isn't reliably supported.
        if (window->DC.NavLayerActiveMask == 0x00 && window->DC.NavHasScroll && g.NavMoveRequest)
        {
            if (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right)
                SetWindowScrollX(window, ImFloor(window->Scroll.x + ((g.NavMoveDir == ImGuiDir_Left) ? -1.0f : +1.0f) * scroll_speed));
            if (g.NavMoveDir == ImGuiDir_Up || g.NavMoveDir == ImGuiDir_Down)
                SetWindowScrollY(window, ImFloor(window->Scroll.y + ((g.NavMoveDir == ImGuiDir_Up) ? -1.0f : +1.0f) * scroll_speed));
        }

        // *Normal* Manual scroll with NavScrollXXX keys
        // Next movement request will clamp the NavId reference rectangle to the visible area, so navigation will resume within those bounds.
        ImVec2 scroll_dir = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadLStick, ImGuiInputReadMode_Down, 1.0f/10.0f, 10.0f);
        if (scroll_dir.x != 0.0f && window->ScrollbarX)
        {
            SetWindowScrollX(window, ImFloor(window->Scroll.x + scroll_dir.x * scroll_speed));
            g.NavMoveFromClampedRefRect = true;
        }
        if (scroll_dir.y != 0.0f)
        {
            SetWindowScrollY(window, ImFloor(window->Scroll.y + scroll_dir.y * scroll_speed));
            g.NavMoveFromClampedRefRect = true;
        }
    }

    // Reset search results
    g.NavMoveResultLocal.Clear();
    g.NavMoveResultLocalVisibleSet.Clear();
    g.NavMoveResultOther.Clear();

    // When we have manually scrolled (without using navigation) and NavId becomes out of bounds, we project its bounding box to the visible area to restart navigation within visible items
    if (g.NavMoveRequest && g.NavMoveFromClampedRefRect && g.NavLayer == 0)
    {
        ImGuiWindow* window = g.NavWindow;
        ImRect window_rect_rel(window->InnerMainRect.Min - window->Pos - ImVec2(1,1), window->InnerMainRect.Max - window->Pos + ImVec2(1,1));
        if (!window_rect_rel.Contains(window->NavRectRel[g.NavLayer]))
        {
            float pad = window->CalcFontSize() * 0.5f;
            window_rect_rel.Expand(ImVec2(-ImMin(window_rect_rel.GetWidth(), pad), -ImMin(window_rect_rel.GetHeight(), pad))); // Terrible approximation for the intent of starting navigation from first fully visible item
            window->NavRectRel[g.NavLayer].ClipWith(window_rect_rel);
            g.NavId = 0;
        }
        g.NavMoveFromClampedRefRect = false;
    }

    // For scoring we use a single segment on the left side our current item bounding box (not touching the edge to avoid box overlap with zero-spaced items)
    ImRect nav_rect_rel = (g.NavWindow && !g.NavWindow->NavRectRel[g.NavLayer].IsInverted()) ? g.NavWindow->NavRectRel[g.NavLayer] : ImRect(0,0,0,0);
    g.NavScoringRectScreen = g.NavWindow ? ImRect(g.NavWindow->Pos + nav_rect_rel.Min, g.NavWindow->Pos + nav_rect_rel.Max) : ImRect(0,0,0,0);
    g.NavScoringRectScreen.TranslateY(nav_scoring_rect_offset_y);
    g.NavScoringRectScreen.Min.x = ImMin(g.NavScoringRectScreen.Min.x + 1.0f, g.NavScoringRectScreen.Max.x);
    g.NavScoringRectScreen.Max.x = g.NavScoringRectScreen.Min.x;
    IM_ASSERT(!g.NavScoringRectScreen.IsInverted()); // Ensure if we have a finite, non-inverted bounding box here will allows us to remove extraneous ImFabs() calls in NavScoreItem().
    //g.OverlayDrawList.AddRect(g.NavScoringRectScreen.Min, g.NavScoringRectScreen.Max, IM_COL32(255,200,0,255)); // [DEBUG]
    g.NavScoringCount = 0;
#if IMGUI_DEBUG_NAV_RECTS
    if (g.NavWindow) { for (int layer = 0; layer < 2; layer++) GetOverlayDrawList(g.NavWindow)->AddRect(g.NavWindow->Pos + g.NavWindow->NavRectRel[layer].Min, g.NavWindow->Pos + g.NavWindow->NavRectRel[layer].Max, IM_COL32(255,200,0,255)); } // [DEBUG]
    if (g.NavWindow) { ImU32 col = (!g.NavWindow->Hidden) ? IM_COL32(255,0,255,255) : IM_COL32(255,0,0,255); ImVec2 p = NavCalcPreferredRefPos(); char buf[32]; ImFormatString(buf, 32, "%d", g.NavLayer); GetOverlayDrawList(g.NavWindow)->AddCircleFilled(p, 3.0f, col); GetOverlayDrawList(g.NavWindow)->AddText(NULL, 13.0f, p + ImVec2(8,-4), col, buf); }
#endif
}

// Apply result from previous frame navigation directional move request
static void ImGui::NavUpdateMoveResult()
{
    ImGuiContext& g = *GImGui;
    if (g.NavMoveResultLocal.ID == 0 && g.NavMoveResultOther.ID == 0)
    {
        // In a situation when there is no results but NavId != 0, re-enable the Navigation highlight (because g.NavId is not considered as a possible result)
        if (g.NavId != 0)
        {
            g.NavDisableHighlight = false;
            g.NavDisableMouseHover = true;
        }
        return;
    }

    // Select which result to use
    ImGuiNavMoveResult* result = (g.NavMoveResultLocal.ID != 0) ? &g.NavMoveResultLocal : &g.NavMoveResultOther;

    // PageUp/PageDown behavior first jumps to the bottom/top mostly visible item, _otherwise_ use the result from the previous/next page.
    if (g.NavMoveRequestFlags & ImGuiNavMoveFlags_AlsoScoreVisibleSet)
        if (g.NavMoveResultLocalVisibleSet.ID != 0 && g.NavMoveResultLocalVisibleSet.ID != g.NavId)
            result = &g.NavMoveResultLocalVisibleSet;

    // Maybe entering a flattened child from the outside? In this case solve the tie using the regular scoring rules.
    if (result != &g.NavMoveResultOther && g.NavMoveResultOther.ID != 0 && g.NavMoveResultOther.Window->ParentWindow == g.NavWindow)
        if ((g.NavMoveResultOther.DistBox < result->DistBox) || (g.NavMoveResultOther.DistBox == result->DistBox && g.NavMoveResultOther.DistCenter < result->DistCenter))
            result = &g.NavMoveResultOther;
    IM_ASSERT(g.NavWindow && result->Window);

    // Scroll to keep newly navigated item fully into view.
    if (g.NavLayer == 0)
    {
        ImRect rect_abs = ImRect(result->RectRel.Min + result->Window->Pos, result->RectRel.Max + result->Window->Pos);
        NavScrollToBringItemIntoView(result->Window, rect_abs);

        // Estimate upcoming scroll so we can offset our result position so mouse position can be applied immediately after in NavUpdate()
        ImVec2 next_scroll = CalcNextScrollFromScrollTargetAndClamp(result->Window, false);
        ImVec2 delta_scroll = result->Window->Scroll - next_scroll;
        result->RectRel.Translate(delta_scroll);

        // Also scroll parent window to keep us into view if necessary (we could/should technically recurse back the whole the parent hierarchy).
        if (result->Window->Flags & ImGuiWindowFlags_ChildWindow)
            NavScrollToBringItemIntoView(result->Window->ParentWindow, ImRect(rect_abs.Min + delta_scroll, rect_abs.Max + delta_scroll));
    }

    ClearActiveID();
    g.NavWindow = result->Window;
    SetNavIDWithRectRel(result->ID, g.NavLayer, result->RectRel);
    g.NavJustMovedToId = result->ID;
    g.NavMoveFromClampedRefRect = false;
}

static float ImGui::NavUpdatePageUpPageDown(int allowed_dir_flags)
{
    ImGuiContext& g = *GImGui;
    if (g.NavMoveDir == ImGuiDir_None && g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs) && !g.NavWindowingTarget && g.NavLayer == 0)
    {
        ImGuiWindow* window = g.NavWindow;
        bool page_up_held = IsKeyDown(g.IO.KeyMap[ImGuiKey_PageUp]) && (allowed_dir_flags & (1 << ImGuiDir_Up));
        bool page_down_held = IsKeyDown(g.IO.KeyMap[ImGuiKey_PageDown]) && (allowed_dir_flags & (1 << ImGuiDir_Down));
        if (page_up_held != page_down_held) // If either (not both) are pressed
        {
            if (window->DC.NavLayerActiveMask == 0x00 && window->DC.NavHasScroll)
            {
                // Fallback manual-scroll when window has no navigable item
                if (IsKeyPressed(g.IO.KeyMap[ImGuiKey_PageUp], true))
                    SetWindowScrollY(window, window->Scroll.y - window->InnerClipRect.GetHeight());
                else if (IsKeyPressed(g.IO.KeyMap[ImGuiKey_PageDown], true))
                    SetWindowScrollY(window, window->Scroll.y + window->InnerClipRect.GetHeight());
            }
            else
            {
                const ImRect& nav_rect_rel = window->NavRectRel[g.NavLayer];
                const float page_offset_y = ImMax(0.0f, window->InnerClipRect.GetHeight() - window->CalcFontSize() * 1.0f + nav_rect_rel.GetHeight());
                float nav_scoring_rect_offset_y = 0.0f;
                if (IsKeyPressed(g.IO.KeyMap[ImGuiKey_PageUp], true))
                {
                    nav_scoring_rect_offset_y = -page_offset_y;
                    g.NavMoveDir = ImGuiDir_Down; // Because our scoring rect is offset, we intentionally request the opposite direction (so we can always land on the last item)
                    g.NavMoveClipDir = ImGuiDir_Up;
                    g.NavMoveRequestFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_AlsoScoreVisibleSet;
                }
                else if (IsKeyPressed(g.IO.KeyMap[ImGuiKey_PageDown], true))
                {
                    nav_scoring_rect_offset_y = +page_offset_y;
                    g.NavMoveDir = ImGuiDir_Up; // Because our scoring rect is offset, we intentionally request the opposite direction (so we can always land on the last item)
                    g.NavMoveClipDir = ImGuiDir_Down;
                    g.NavMoveRequestFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_AlsoScoreVisibleSet;
                }
                return nav_scoring_rect_offset_y;
            }
        }
    }
    return 0.0f;
}

static int FindWindowFocusIndex(ImGuiWindow* window) // FIXME-OPT O(N)
{
    ImGuiContext& g = *GImGui;
    for (int i = g.WindowsFocusOrder.Size-1; i >= 0; i--)
        if (g.WindowsFocusOrder[i] == window)
            return i;
    return -1;
}

static ImGuiWindow* FindWindowNavFocusable(int i_start, int i_stop, int dir) // FIXME-OPT O(N)
{
    ImGuiContext& g = *GImGui;
    for (int i = i_start; i >= 0 && i < g.WindowsFocusOrder.Size && i != i_stop; i += dir)
        if (ImGui::IsWindowNavFocusable(g.WindowsFocusOrder[i]))
            return g.WindowsFocusOrder[i];
    return NULL;
}

static void NavUpdateWindowingHighlightWindow(int focus_change_dir)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindowingTarget);
    if (g.NavWindowingTarget->Flags & ImGuiWindowFlags_Modal)
        return;

    const int i_current = FindWindowFocusIndex(g.NavWindowingTarget);
    ImGuiWindow* window_target = FindWindowNavFocusable(i_current + focus_change_dir, -INT_MAX, focus_change_dir);
    if (!window_target)
        window_target = FindWindowNavFocusable((focus_change_dir < 0) ? (g.WindowsFocusOrder.Size - 1) : 0, i_current, focus_change_dir);
    if (window_target) // Don't reset windowing target if there's a single window in the list
        g.NavWindowingTarget = g.NavWindowingTargetAnim = window_target;
    g.NavWindowingToggleLayer = false;
}

// Window management mode (hold to: change focus/move/resize, tap to: toggle menu layer)
static void ImGui::NavUpdateWindowing()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* apply_focus_window = NULL;
    bool apply_toggle_layer = false;

    ImGuiWindow* modal_window = GetFrontMostPopupModal();
    if (modal_window != NULL)
    {
        g.NavWindowingTarget = NULL;
        return;
    }

    // Fade out
    if (g.NavWindowingTargetAnim && g.NavWindowingTarget == NULL)
    {
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha - g.IO.DeltaTime * 10.0f, 0.0f);
        if (g.DimBgRatio <= 0.0f && g.NavWindowingHighlightAlpha <= 0.0f)
            g.NavWindowingTargetAnim = NULL;
    }

    // Start CTRL-TAB or Square+L/R window selection
    bool start_windowing_with_gamepad = !g.NavWindowingTarget && IsNavInputPressed(ImGuiNavInput_Menu, ImGuiInputReadMode_Pressed);
    bool start_windowing_with_keyboard = !g.NavWindowingTarget && g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_Tab) && (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard);
    if (start_windowing_with_gamepad || start_windowing_with_keyboard)
        if (ImGuiWindow* window = g.NavWindow ? g.NavWindow : FindWindowNavFocusable(g.WindowsFocusOrder.Size - 1, -INT_MAX, -1))
        {
            g.NavWindowingTarget = g.NavWindowingTargetAnim = window;
            g.NavWindowingTimer = g.NavWindowingHighlightAlpha = 0.0f;
            g.NavWindowingToggleLayer = start_windowing_with_keyboard ? false : true;
            g.NavInputSource = start_windowing_with_keyboard ? ImGuiInputSource_NavKeyboard : ImGuiInputSource_NavGamepad;
        }

    // Gamepad update
    g.NavWindowingTimer += g.IO.DeltaTime;
    if (g.NavWindowingTarget && g.NavInputSource == ImGuiInputSource_NavGamepad)
    {
        // Highlight only appears after a brief time holding the button, so that a fast tap on PadMenu (to toggle NavLayer) doesn't add visual noise
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha, ImSaturate((g.NavWindowingTimer - NAV_WINDOWING_HIGHLIGHT_DELAY) / 0.05f));

        // Select window to focus
        const int focus_change_dir = (int)IsNavInputPressed(ImGuiNavInput_FocusPrev, ImGuiInputReadMode_RepeatSlow) - (int)IsNavInputPressed(ImGuiNavInput_FocusNext, ImGuiInputReadMode_RepeatSlow);
        if (focus_change_dir != 0)
        {
            NavUpdateWindowingHighlightWindow(focus_change_dir);
            g.NavWindowingHighlightAlpha = 1.0f;
        }

        // Single press toggles NavLayer, long press with L/R apply actual focus on release (until then the window was merely rendered front-most)
        if (!IsNavInputDown(ImGuiNavInput_Menu))
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
    if (g.NavWindowingTarget && g.NavInputSource == ImGuiInputSource_NavKeyboard)
    {
        // Visuals only appears after a brief time after pressing TAB the first time, so that a fast CTRL+TAB doesn't add visual noise
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha, ImSaturate((g.NavWindowingTimer - NAV_WINDOWING_HIGHLIGHT_DELAY) / 0.05f)); // 1.0f
        if (IsKeyPressedMap(ImGuiKey_Tab, true))
            NavUpdateWindowingHighlightWindow(g.IO.KeyShift ? +1 : -1);
        if (!g.IO.KeyCtrl)
            apply_focus_window = g.NavWindowingTarget;
    }

    // Keyboard: Press and Release ALT to toggle menu layer
    // FIXME: We lack an explicit IO variable for "is the imgui window focused", so compare mouse validity to detect the common case of back-end clearing releases all keys on ALT-TAB
    if (IsNavInputPressed(ImGuiNavInput_KeyMenu_, ImGuiInputReadMode_Pressed))
        g.NavWindowingToggleLayer = true;
    if ((g.ActiveId == 0 || g.ActiveIdAllowOverlap) && g.NavWindowingToggleLayer && IsNavInputPressed(ImGuiNavInput_KeyMenu_, ImGuiInputReadMode_Released))
        if (IsMousePosValid(&g.IO.MousePos) == IsMousePosValid(&g.IO.MousePosPrev))
            apply_toggle_layer = true;

    // Move window
    if (g.NavWindowingTarget && !(g.NavWindowingTarget->Flags & ImGuiWindowFlags_NoMove))
    {
        ImVec2 move_delta;
        if (g.NavInputSource == ImGuiInputSource_NavKeyboard && !g.IO.KeyShift)
            move_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard, ImGuiInputReadMode_Down);
        if (g.NavInputSource == ImGuiInputSource_NavGamepad)
            move_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadLStick, ImGuiInputReadMode_Down);
        if (move_delta.x != 0.0f || move_delta.y != 0.0f)
        {
            const float NAV_MOVE_SPEED = 800.0f;
            const float move_speed = ImFloor(NAV_MOVE_SPEED * g.IO.DeltaTime * ImMin(g.IO.DisplayFramebufferScale.x, g.IO.DisplayFramebufferScale.y)); // FIXME: Doesn't code variable framerate very well
            g.NavWindowingTarget->RootWindow->Pos += move_delta * move_speed;
            g.NavDisableMouseHover = true;
            MarkIniSettingsDirty(g.NavWindowingTarget);
        }
    }

    // Apply final focus
    if (apply_focus_window && (g.NavWindow == NULL || apply_focus_window != g.NavWindow->RootWindowDockStop))
    {
        ImGuiViewport* previous_viewport = g.NavWindow ? g.NavWindow->Viewport : NULL;
        g.NavDisableHighlight = false;
        g.NavDisableMouseHover = true;
        apply_focus_window = NavRestoreLastChildNavWindow(apply_focus_window);
        ClosePopupsOverWindow(apply_focus_window);
        FocusWindow(apply_focus_window);
        if (apply_focus_window->NavLastIds[0] == 0)
            NavInitWindow(apply_focus_window, false);

        // If the window only has a menu layer, select it directly
        if (apply_focus_window->DC.NavLayerActiveMask == (1 << ImGuiNavLayer_Menu))
            g.NavLayer = ImGuiNavLayer_Menu;

        // Request OS level focus
        if (apply_focus_window->Viewport != previous_viewport && g.PlatformIO.Platform_SetWindowFocus)
            g.PlatformIO.Platform_SetWindowFocus(apply_focus_window->Viewport);
    }
    if (apply_focus_window)
        g.NavWindowingTarget = NULL;

    // Apply menu/layer toggle
    if (apply_toggle_layer && g.NavWindow)
    {
        // Move to parent menu if necessary
        ImGuiWindow* new_nav_window = g.NavWindow;
        while (new_nav_window->ParentWindow
            && (new_nav_window->DC.NavLayerActiveMask & (1 << 1)) == 0 
            && (new_nav_window->Flags & ImGuiWindowFlags_ChildWindow) != 0 
            && (new_nav_window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu)) == 0)
            new_nav_window = new_nav_window->ParentWindow;
        if (new_nav_window != g.NavWindow)
        {
            ImGuiWindow* old_nav_window = g.NavWindow;
            FocusWindow(new_nav_window);
            new_nav_window->NavLastChildNavWindow = old_nav_window;
        }
        g.NavDisableHighlight = false;
        g.NavDisableMouseHover = true;

        // When entering a regular menu bar with the Alt key, we always reinitialize the navigation ID. It however persist on docking tab tabs.
        const ImGuiNavLayer new_nav_layer = (g.NavWindow->DC.NavLayerActiveMask & (1 << ImGuiNavLayer_Menu)) ? (ImGuiNavLayer)((int)g.NavLayer ^ 1) : ImGuiNavLayer_Main;
        const bool preserve_layer_1_nav_id = (new_nav_window->DockNodeAsHost != NULL);
        if (new_nav_layer == ImGuiNavLayer_Menu && !preserve_layer_1_nav_id)
            g.NavWindow->NavLastIds[ImGuiNavLayer_Menu] = 0;
        NavRestoreLayer(new_nav_layer);
    }
}

// Window has already passed the IsWindowNavFocusable()
static const char* GetFallbackWindowNameForWindowingList(ImGuiWindow* window)
{
    if (window->Flags & ImGuiWindowFlags_Popup)
        return "(Popup)";
    if ((window->Flags & ImGuiWindowFlags_MenuBar) && strcmp(window->Name, "##MainMenuBar") == 0)
        return "(Main menu bar)";
    if (window->DockNodeAsHost)
        return "(Dock node)";
    return "(Untitled)";
}

// Overlay displayed when using CTRL+TAB. Called by EndFrame().
void ImGui::NavUpdateWindowingList()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindowingTarget != NULL);

    if (g.NavWindowingTimer < NAV_WINDOWING_LIST_APPEAR_DELAY)
        return;

    if (g.NavWindowingList == NULL)
        g.NavWindowingList = FindWindowByName("###NavWindowingList");
    ImGuiViewportP* viewport = /*g.NavWindow ? g.NavWindow->Viewport :*/ (ImGuiViewportP*)GetMainViewport();
    SetNextWindowSizeConstraints(ImVec2(viewport->Size.x * 0.20f, viewport->Size.y * 0.20f), ImVec2(FLT_MAX, FLT_MAX));
    SetNextWindowPos(viewport->Pos + viewport->Size * 0.5f, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    PushStyleVar(ImGuiStyleVar_WindowPadding, g.Style.WindowPadding * 2.0f);
    Begin("###NavWindowingList", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    for (int n = g.WindowsFocusOrder.Size - 1; n >= 0; n--)
    {
        ImGuiWindow* window = g.WindowsFocusOrder[n];
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
// [SECTION] COLUMNS
// In the current version, Columns are very weak. Needs to be replaced with a more full-featured system.
//-----------------------------------------------------------------------------

void ImGui::NextColumn()
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems || window->DC.ColumnsSet == NULL)
        return;

    ImGuiContext& g = *GImGui;
    PopItemWidth();
    PopClipRect();

    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    columns->LineMaxY = ImMax(columns->LineMaxY, window->DC.CursorPos.y);
    if (++columns->Current < columns->Count)
    {
        // Columns 1+ cancel out IndentX
        window->DC.ColumnsOffset.x = GetColumnOffset(columns->Current) - window->DC.Indent.x + g.Style.ItemSpacing.x;
        window->DrawList->ChannelsSetCurrent(columns->Current);
    }
    else
    {
        window->DC.ColumnsOffset.x = 0.0f;
        window->DrawList->ChannelsSetCurrent(0);
        columns->Current = 0;
        columns->LineMinY = columns->LineMaxY;
    }
    window->DC.CursorPos.x = (float)(int)(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
    window->DC.CursorPos.y = columns->LineMinY;
    window->DC.CurrentLineSize = ImVec2(0.0f, 0.0f);
    window->DC.CurrentLineTextBaseOffset = 0.0f;

    PushColumnClipRect();
    PushItemWidth(GetColumnWidth() * 0.65f);  // FIXME: Move on columns setup
}

int ImGui::GetColumnIndex()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.ColumnsSet ? window->DC.ColumnsSet->Current : 0;
}

int ImGui::GetColumnsCount()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.ColumnsSet ? window->DC.ColumnsSet->Count : 1;
}

static float OffsetNormToPixels(const ImGuiColumnsSet* columns, float offset_norm)
{
    return offset_norm * (columns->MaxX - columns->MinX);
}

static float PixelsToOffsetNorm(const ImGuiColumnsSet* columns, float offset)
{
    return offset / (columns->MaxX - columns->MinX);
}

static inline float GetColumnsRectHalfWidth() { return 4.0f; }

static float GetDraggedColumnOffset(ImGuiColumnsSet* columns, int column_index)
{
    // Active (dragged) column always follow mouse. The reason we need this is that dragging a column to the right edge of an auto-resizing
    // window creates a feedback loop because we store normalized positions. So while dragging we enforce absolute positioning.
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(column_index > 0); // We are not supposed to drag column 0.
    IM_ASSERT(g.ActiveId == columns->ID + ImGuiID(column_index));

    float x = g.IO.MousePos.x - g.ActiveIdClickOffset.x + GetColumnsRectHalfWidth() - window->Pos.x;
    x = ImMax(x, ImGui::GetColumnOffset(column_index - 1) + g.Style.ColumnsMinSpacing);
    if ((columns->Flags & ImGuiColumnsFlags_NoPreserveWidths))
        x = ImMin(x, ImGui::GetColumnOffset(column_index + 1) - g.Style.ColumnsMinSpacing);

    return x;
}

float ImGui::GetColumnOffset(int column_index)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    IM_ASSERT(column_index < columns->Columns.Size);

    const float t = columns->Columns[column_index].OffsetNorm;
    const float x_offset = ImLerp(columns->MinX, columns->MaxX, t);
    return x_offset;
}

static float GetColumnWidthEx(ImGuiColumnsSet* columns, int column_index, bool before_resize = false)
{
    if (column_index < 0)
        column_index = columns->Current;

    float offset_norm;
    if (before_resize)
        offset_norm = columns->Columns[column_index + 1].OffsetNormBeforeResize - columns->Columns[column_index].OffsetNormBeforeResize;
    else
        offset_norm = columns->Columns[column_index + 1].OffsetNorm - columns->Columns[column_index].OffsetNorm;
    return OffsetNormToPixels(columns, offset_norm);
}

float ImGui::GetColumnWidth(int column_index)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    return OffsetNormToPixels(columns, columns->Columns[column_index + 1].OffsetNorm - columns->Columns[column_index].OffsetNorm);
}

void ImGui::SetColumnOffset(int column_index, float offset)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    IM_ASSERT(column_index < columns->Columns.Size);

    const bool preserve_width = !(columns->Flags & ImGuiColumnsFlags_NoPreserveWidths) && (column_index < columns->Count-1);
    const float width = preserve_width ? GetColumnWidthEx(columns, column_index, columns->IsBeingResized) : 0.0f;

    if (!(columns->Flags & ImGuiColumnsFlags_NoForceWithinWindow))
        offset = ImMin(offset, columns->MaxX - g.Style.ColumnsMinSpacing * (columns->Count - column_index));
    columns->Columns[column_index].OffsetNorm = PixelsToOffsetNorm(columns, offset - columns->MinX);

    if (preserve_width)
        SetColumnOffset(column_index + 1, offset + ImMax(g.Style.ColumnsMinSpacing, width));
}

void ImGui::SetColumnWidth(int column_index, float width)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    IM_ASSERT(columns != NULL);

    if (column_index < 0)
        column_index = columns->Current;
    SetColumnOffset(column_index + 1, GetColumnOffset(column_index) + width);
}

void ImGui::PushColumnClipRect(int column_index)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    if (column_index < 0)
        column_index = columns->Current;

    PushClipRect(columns->Columns[column_index].ClipRect.Min, columns->Columns[column_index].ClipRect.Max, false);
}

static ImGuiColumnsSet* FindOrAddColumnsSet(ImGuiWindow* window, ImGuiID id)
{
    for (int n = 0; n < window->ColumnsStorage.Size; n++)
        if (window->ColumnsStorage[n].ID == id)
            return &window->ColumnsStorage[n];

    window->ColumnsStorage.push_back(ImGuiColumnsSet());
    ImGuiColumnsSet* columns = &window->ColumnsStorage.back();
    columns->ID = id;
    return columns;
}

void ImGui::BeginColumns(const char* str_id, int columns_count, ImGuiColumnsFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    IM_ASSERT(columns_count > 1);
    IM_ASSERT(window->DC.ColumnsSet == NULL); // Nested columns are currently not supported

    // Differentiate column ID with an arbitrary prefix for cases where users name their columns set the same as another widget.
    // In addition, when an identifier isn't explicitly provided we include the number of columns in the hash to make it uniquer.
    PushID(0x11223347 + (str_id ? 0 : columns_count));
    ImGuiID id = window->GetID(str_id ? str_id : "columns");
    PopID();

    // Acquire storage for the columns set
    ImGuiColumnsSet* columns = FindOrAddColumnsSet(window, id);
    IM_ASSERT(columns->ID == id);
    columns->Current = 0;
    columns->Count = columns_count;
    columns->Flags = flags;
    window->DC.ColumnsSet = columns;

    // Set state for first column
    const float content_region_width = (window->SizeContentsExplicit.x != 0.0f) ? (window->SizeContentsExplicit.x) : (window->InnerClipRect.Max.x - window->Pos.x);
    columns->MinX = window->DC.Indent.x - g.Style.ItemSpacing.x; // Lock our horizontal range
    columns->MaxX = ImMax(content_region_width - window->Scroll.x, columns->MinX + 1.0f);
    columns->StartPosY = window->DC.CursorPos.y;
    columns->StartMaxPosX = window->DC.CursorMaxPos.x;
    columns->LineMinY = columns->LineMaxY = window->DC.CursorPos.y;
    window->DC.ColumnsOffset.x = 0.0f;
    window->DC.CursorPos.x = (float)(int)(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);

    // Clear data if columns count changed
    if (columns->Columns.Size != 0 && columns->Columns.Size != columns_count + 1)
        columns->Columns.resize(0);

    // Initialize defaults
    columns->IsFirstFrame = (columns->Columns.Size == 0);
    if (columns->Columns.Size == 0)
    {
        columns->Columns.reserve(columns_count + 1);
        for (int n = 0; n < columns_count + 1; n++)
        {
            ImGuiColumnData column;
            column.OffsetNorm = n / (float)columns_count;
            columns->Columns.push_back(column);
        }
    }

    for (int n = 0; n < columns_count; n++)
    {
        // Compute clipping rectangle
        ImGuiColumnData* column = &columns->Columns[n];
        float clip_x1 = ImFloor(0.5f + window->Pos.x + GetColumnOffset(n) - 1.0f);
        float clip_x2 = ImFloor(0.5f + window->Pos.x + GetColumnOffset(n + 1) - 1.0f);
        column->ClipRect = ImRect(clip_x1, -FLT_MAX, clip_x2, +FLT_MAX);
        column->ClipRect.ClipWith(window->ClipRect);
    }

    window->DrawList->ChannelsSplit(columns->Count);
    PushColumnClipRect();
    PushItemWidth(GetColumnWidth() * 0.65f);
}

void ImGui::EndColumns()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiColumnsSet* columns = window->DC.ColumnsSet;
    IM_ASSERT(columns != NULL);

    PopItemWidth();
    PopClipRect();
    window->DrawList->ChannelsMerge();

    columns->LineMaxY = ImMax(columns->LineMaxY, window->DC.CursorPos.y);
    window->DC.CursorPos.y = columns->LineMaxY;
    if (!(columns->Flags & ImGuiColumnsFlags_GrowParentContentsSize))
        window->DC.CursorMaxPos.x = columns->StartMaxPosX;  // Restore cursor max pos, as columns don't grow parent

    // Draw columns borders and handle resize
    bool is_being_resized = false;
    if (!(columns->Flags & ImGuiColumnsFlags_NoBorder) && !window->SkipItems)
    {
        const float y1 = columns->StartPosY;
        const float y2 = window->DC.CursorPos.y;
        int dragging_column = -1;
        for (int n = 1; n < columns->Count; n++)
        {
            float x = window->Pos.x + GetColumnOffset(n);
            const ImGuiID column_id = columns->ID + ImGuiID(n);
            const float column_hw = GetColumnsRectHalfWidth(); // Half-width for interaction
            const ImRect column_rect(ImVec2(x - column_hw, y1), ImVec2(x + column_hw, y2));
            KeepAliveID(column_id);
            if (IsClippedEx(column_rect, column_id, false))
                continue;

            bool hovered = false, held = false;
            if (!(columns->Flags & ImGuiColumnsFlags_NoResize))
            {
                ButtonBehavior(column_rect, column_id, &hovered, &held);
                if (hovered || held)
                    g.MouseCursor = ImGuiMouseCursor_ResizeEW;
                if (held && !(columns->Columns[n].Flags & ImGuiColumnsFlags_NoResize))
                    dragging_column = n;
            }

            // Draw column (we clip the Y boundaries CPU side because very long triangles are mishandled by some GPU drivers.)
            const ImU32 col = GetColorU32(held ? ImGuiCol_SeparatorActive : hovered ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
            const float xi = (float)(int)x;
            window->DrawList->AddLine(ImVec2(xi, ImMax(y1 + 1.0f, window->ClipRect.Min.y)), ImVec2(xi, ImMin(y2, window->ClipRect.Max.y)), col);
        }

        // Apply dragging after drawing the column lines, so our rendered lines are in sync with how items were displayed during the frame.
        if (dragging_column != -1)
        {
            if (!columns->IsBeingResized)
                for (int n = 0; n < columns->Count + 1; n++)
                    columns->Columns[n].OffsetNormBeforeResize = columns->Columns[n].OffsetNorm;
            columns->IsBeingResized = is_being_resized = true;
            float x = GetDraggedColumnOffset(columns, dragging_column);
            SetColumnOffset(dragging_column, x);
        }
    }
    columns->IsBeingResized = is_being_resized;

    window->DC.ColumnsSet = NULL;
    window->DC.ColumnsOffset.x = 0.0f;
    window->DC.CursorPos.x = (float)(int)(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);
}

// [2018-03: This is currently the only public API, while we are working on making BeginColumns/EndColumns user-facing]
void ImGui::Columns(int columns_count, const char* id, bool border)
{
    ImGuiWindow* window = GetCurrentWindow();
    IM_ASSERT(columns_count >= 1);

    ImGuiColumnsFlags flags = (border ? 0 : ImGuiColumnsFlags_NoBorder);
    //flags |= ImGuiColumnsFlags_NoPreserveWidths; // NB: Legacy behavior
    if (window->DC.ColumnsSet != NULL && window->DC.ColumnsSet->Count == columns_count && window->DC.ColumnsSet->Flags == flags)
        return;

    if (window->DC.ColumnsSet != NULL)
        EndColumns();

    if (columns_count != 1)
        BeginColumns(id, columns_count, flags);
}

//-----------------------------------------------------------------------------
// [SECTION] DRAG AND DROP
//-----------------------------------------------------------------------------

void ImGui::ClearDragDrop()
{
    ImGuiContext& g = *GImGui;
    g.DragDropActive = false;
    g.DragDropPayload.Clear();
    g.DragDropAcceptFlags = ImGuiDragDropFlags_None;
    g.DragDropAcceptIdCurr = g.DragDropAcceptIdPrev = 0;
    g.DragDropAcceptIdCurrRectSurface = FLT_MAX;
    g.DragDropAcceptFrameCount = -1;

    g.DragDropPayloadBufHeap.clear();
    memset(&g.DragDropPayloadBufLocal, 0, sizeof(g.DragDropPayloadBufLocal));
}

// Call when current ID is active.
// When this returns true you need to: a) call SetDragDropPayload() exactly once, b) you may render the payload visual/description, c) call EndDragDropSource()
bool ImGui::BeginDragDropSource(ImGuiDragDropFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    bool source_drag_active = false;
    ImGuiID source_id = 0;
    ImGuiID source_parent_id = 0;
    int mouse_button = 0;
    if (!(flags & ImGuiDragDropFlags_SourceExtern))
    {
        source_id = window->DC.LastItemId;
        if (source_id != 0 && g.ActiveId != source_id) // Early out for most common case
            return false;
        if (g.IO.MouseDown[mouse_button] == false)
            return false;

        if (source_id == 0)
        {
            // If you want to use BeginDragDropSource() on an item with no unique identifier for interaction, such as Text() or Image(), you need to:
            // A) Read the explanation below, B) Use the ImGuiDragDropFlags_SourceAllowNullID flag, C) Swallow your programmer pride.
            if (!(flags & ImGuiDragDropFlags_SourceAllowNullID))
            {
                IM_ASSERT(0);
                return false;
            }

            // Magic fallback (=somehow reprehensible) to handle items with no assigned ID, e.g. Text(), Image()
            // We build a throwaway ID based on current ID stack + relative AABB of items in window.
            // THE IDENTIFIER WON'T SURVIVE ANY REPOSITIONING OF THE WIDGET, so if your widget moves your dragging operation will be canceled.
            // We don't need to maintain/call ClearActiveID() as releasing the button will early out this function and trigger !ActiveIdIsAlive.
            bool is_hovered = (window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) != 0;
            if (!is_hovered && (g.ActiveId == 0 || g.ActiveIdWindow != window))
                return false;
            source_id = window->DC.LastItemId = window->GetIDFromRectangle(window->DC.LastItemRect);
            if (is_hovered)
                SetHoveredID(source_id);
            if (is_hovered && g.IO.MouseClicked[mouse_button])
            {
                SetActiveID(source_id, window);
                FocusWindow(window);
            }
            if (g.ActiveId == source_id) // Allow the underlying widget to display/return hovered during the mouse release frame, else we would get a flicker.
                g.ActiveIdAllowOverlap = is_hovered;
        }
        else
        {
            g.ActiveIdAllowOverlap = false;
        }
        if (g.ActiveId != source_id)
            return false;
        source_parent_id = window->IDStack.back();
        source_drag_active = IsMouseDragging(mouse_button);
    }
    else
    {
        window = NULL;
        source_id = ImHash("#SourceExtern", 0);
        source_drag_active = true;
    }

    if (source_drag_active)
    {
        if (!g.DragDropActive)
        {
            IM_ASSERT(source_id != 0);
            ClearDragDrop();
            ImGuiPayload& payload = g.DragDropPayload;
            payload.SourceId = source_id;
            payload.SourceParentId = source_parent_id;
            g.DragDropActive = true;
            g.DragDropSourceFlags = flags;
            g.DragDropMouseButton = mouse_button;
        }
        g.DragDropSourceFrameCount = g.FrameCount;
        g.DragDropWithinSourceOrTarget = true;

        if (!(flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
        {
            // Target can request the Source to not display its tooltip (we use a dedicated flag to make this request explicit)
            // We unfortunately can't just modify the source flags and skip the call to BeginTooltip, as caller may be emitting contents. 
            BeginTooltip();
            if (g.DragDropAcceptIdPrev && (g.DragDropAcceptFlags & ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                ImGuiWindow* tooltip_window = g.CurrentWindow;
                tooltip_window->SkipItems = true;
                tooltip_window->HiddenFramesRegular = 1;
            }
        }

        if (!(flags & ImGuiDragDropFlags_SourceNoDisableHover) && !(flags & ImGuiDragDropFlags_SourceExtern))
            window->DC.LastItemStatusFlags &= ~ImGuiItemStatusFlags_HoveredRect;

        return true;
    }
    return false;
}

void ImGui::EndDragDropSource()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.DragDropActive);
    IM_ASSERT(g.DragDropWithinSourceOrTarget && "Not after a BeginDragDropSource()?");

    if (!(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
        EndTooltip();

    // Discard the drag if have not called SetDragDropPayload()
    if (g.DragDropPayload.DataFrameCount == -1)
        ClearDragDrop();
    g.DragDropWithinSourceOrTarget = false;
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
    IM_ASSERT(payload.SourceId != 0);                               // Not called between BeginDragDropSource() and EndDragDropSource()

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

    return (g.DragDropAcceptFrameCount == g.FrameCount) || (g.DragDropAcceptFrameCount == g.FrameCount - 1);
}

bool ImGui::BeginDragDropTargetCustom(const ImRect& bb, ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (!g.DragDropActive)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    if (g.HoveredWindowUnderMovingWindow == NULL || window->RootWindow != g.HoveredWindowUnderMovingWindow->RootWindow)
        return false;
    IM_ASSERT(id != 0);
    if (!IsMouseHoveringRect(bb.Min, bb.Max) || (id == g.DragDropPayload.SourceId))
        return false;
    if (window->SkipItems)
        return false;

    IM_ASSERT(g.DragDropWithinSourceOrTarget == false);
    g.DragDropTargetRect = bb;
    g.DragDropTargetId = id;
    g.DragDropWithinSourceOrTarget = true;
    return true;
}

// We don't use BeginDragDropTargetCustom() and duplicate its code because:
// 1) we use LastItemRectHoveredRect which handles items that pushes a temporarily clip rectangle in their code. Calling BeginDragDropTargetCustom(LastItemRect) would not handle them.
// 2) and it's faster. as this code may be very frequently called, we want to early out as fast as we can.
// Also note how the HoveredWindow test is positioned differently in both functions (in both functions we optimize for the cheapest early out case)
bool ImGui::BeginDragDropTarget()
{
    ImGuiContext& g = *GImGui;
    if (!g.DragDropActive)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    if (!(window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect))
        return false;
    if (g.HoveredWindowUnderMovingWindow == NULL || window->RootWindow != g.HoveredWindowUnderMovingWindow->RootWindow)
        return false;

    const ImRect& display_rect = (window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HasDisplayRect) ? window->DC.LastItemDisplayRect : window->DC.LastItemRect;
    ImGuiID id = window->DC.LastItemId;
    if (id == 0)
        id = window->GetIDFromRectangle(display_rect);
    if (g.DragDropPayload.SourceId == id)
        return false;

    IM_ASSERT(g.DragDropWithinSourceOrTarget == false);
    g.DragDropTargetRect = display_rect;
    g.DragDropTargetId = id;
    g.DragDropWithinSourceOrTarget = true;
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
    ImGuiWindow* window = g.CurrentWindow;
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
    if (r_surface < g.DragDropAcceptIdCurrRectSurface)
    {
        g.DragDropAcceptFlags = flags;
        g.DragDropAcceptIdCurr = g.DragDropTargetId;
        g.DragDropAcceptIdCurrRectSurface = r_surface;
    }

    // Render default drop visuals
    payload.Preview = was_accepted_previously;
    flags |= (g.DragDropSourceFlags & ImGuiDragDropFlags_AcceptNoDrawDefaultRect); // Source can also inhibit the preview (useful for external sources that lives for 1 frame)
    if (!(flags & ImGuiDragDropFlags_AcceptNoDrawDefaultRect) && payload.Preview)
    {
        // FIXME-DRAG: Settle on a proper default visuals for drop target.
        r.Expand(3.5f);
        bool push_clip_rect = !window->ClipRect.Contains(r);
        if (push_clip_rect) window->DrawList->PushClipRect(r.Min-ImVec2(1,1), r.Max+ImVec2(1,1));
        window->DrawList->AddRect(r.Min, r.Max, GetColorU32(ImGuiCol_DragDropTarget), 0.0f, ~0, 2.0f);
        if (push_clip_rect) window->DrawList->PopClipRect();
    }

    g.DragDropAcceptFrameCount = g.FrameCount;
    payload.Delivery = was_accepted_previously && !IsMouseDown(g.DragDropMouseButton); // For extern drag sources affecting os window focus, it's easier to just test !IsMouseDown() instead of IsMouseReleased()
    if (!payload.Delivery && !(flags & ImGuiDragDropFlags_AcceptBeforeDelivery))
        return NULL;

    return &payload;
}

const ImGuiPayload* ImGui::GetDragDropPayload()
{
    ImGuiContext& g = *GImGui;
    return g.DragDropActive ? &g.DragDropPayload : NULL;
}

// We don't really use/need this now, but added it for the sake of consistency and because we might need it later.
void ImGui::EndDragDropTarget()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.DragDropActive);
    IM_ASSERT(g.DragDropWithinSourceOrTarget);
    g.DragDropWithinSourceOrTarget = false;
}

//-----------------------------------------------------------------------------
// [SECTION] DOCKING
//-----------------------------------------------------------------------------
// Docking: Internal Types
// Docking: Forward Declarations
// Docking: ImGuiDockContext
// Docking: ImGuiDockContext Docking/Undocking functions
// Docking: ImGuiDockNode
// Docking: ImGuiDockNode Tree manipulation functions
// Docking: Public Functions (SetWindowDock, DockSpace)
// Docking: Public Builder Functions
// Docking: Begin/End Functions (called from Begin/End)
// Docking: Settings
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Docking: Internal Types
//-----------------------------------------------------------------------------

static float IMGUI_DOCK_SPLITTER_SIZE = 2.0f;

enum ImGuiDockRequestType
{
    ImGuiDockRequestType_None = 0,
    ImGuiDockRequestType_Dock,
    ImGuiDockRequestType_Undock,
    ImGuiDockRequestType_Split                  // Split is the same as Dock but without a DockPayload
};

struct ImGuiDockRequest
{
    ImGuiDockRequestType    Type;
    ImGuiWindow*            DockTargetWindow;   // Destination/Target Window to dock into (may be a loose window or a DockNode, might be NULL in which case DockTargetNode cannot be NULL)
    ImGuiDockNode*          DockTargetNode;     // Destination/Target Node to dock into 
    ImGuiWindow*            DockPayload;        // Source/Payload window to dock (may be a loose window or a DockNode), [Optional]
    ImGuiDir                DockSplitDir;
    float                   DockSplitRatio;
    bool                    DockSplitOuter;
    ImGuiWindow*            UndockTargetWindow;
    ImGuiDockNode*          UndockTargetNode;

    ImGuiDockRequest()
    {
        Type = ImGuiDockRequestType_None;
        DockTargetWindow = DockPayload = UndockTargetWindow = NULL;
        DockTargetNode = UndockTargetNode = NULL;
        DockSplitDir = ImGuiDir_None;
        DockSplitRatio = 0.5f;
        DockSplitOuter = false;
    }
};

struct ImGuiDockPreviewData
{
    ImGuiDockNode   FutureNode;
    bool            IsDropAllowed;
    bool            IsCenterAvailable;
    bool            IsSidesAvailable;           // Hold your breath, grammar freaks..
    bool            IsSplitDirExplicit;         // Set when hovered the drop rect (vs. implicit SplitDir==None when hovered the window)
    ImGuiDockNode*  SplitNode;
    ImGuiDir        SplitDir;
    float           SplitRatio;
    ImRect          DropRectsDraw[ImGuiDir_COUNT + 1];  // May be slightly different from hit-testing drop rects used in DockNodeCalcDropRects()

    ImGuiDockPreviewData() : FutureNode(0) { IsDropAllowed = IsCenterAvailable = IsSidesAvailable = IsSplitDirExplicit = false; SplitNode = NULL; SplitDir = ImGuiDir_None; SplitRatio = 0.f; }
};

// Persistent Settings data, stored contiguously in SettingsNodes (sizeof() ~32 bytes)
struct ImGuiDockNodeSettings
{
    ImGuiID         ID;
    ImGuiID         ParentID;
    ImGuiID         SelectedTabID;
    signed char     SplitAxis;
    char            Depth;
    char            IsDockSpace;
    char            IsCentralNode;
    char            IsHiddenTabBar;
    ImVec2ih        Pos;
    ImVec2ih        Size;
    ImVec2ih        SizeRef;
    ImGuiDockNodeSettings() { ID = ParentID = SelectedTabID = 0; SplitAxis = ImGuiAxis_None; Depth = 0; IsDockSpace = IsCentralNode = IsHiddenTabBar = 0; }
};

struct ImGuiDockContext
{
    ImGuiStorage                    Nodes;                      // Map ID -> ImGuiDockNode*: Active nodes
    ImVector<ImGuiDockRequest>      Requests;
    ImVector<ImGuiDockNodeSettings> SettingsNodes;
    bool                            WantFullRebuild;
    ImGuiDockContext()              { WantFullRebuild = false; }
};

//-----------------------------------------------------------------------------
// Docking: Forward Declarations
//-----------------------------------------------------------------------------

namespace ImGui
{
    // ImGuiDockContext
    static ImGuiDockNode*   DockContextAddNode(ImGuiContext* ctx, ImGuiID id);
    static ImGuiID          DockContextGenNodeID(ImGuiContext* ctx);
    static void             DockContextRemoveNode(ImGuiContext* ctx, ImGuiDockNode* node, bool merge_sibling_into_parent_node);
    static void             DockContextQueueDock(ImGuiContext* ctx, ImGuiWindow* target, ImGuiDockNode* target_node, ImGuiWindow* payload, ImGuiDir split_dir, float split_ratio, bool split_outer);
    static void             DockContextQueueNotifyRemovedNode(ImGuiContext* ctx, ImGuiDockNode* node);
    static void             DockContextProcessDock(ImGuiContext* ctx, ImGuiDockRequest* req);
    static void             DockContextProcessUndockWindow(ImGuiContext* ctx, ImGuiWindow* window, bool clear_persistent_docking_ref = true);
    static void             DockContextProcessUndockNode(ImGuiContext* ctx, ImGuiDockNode* node);
    static void             DockContextPruneUnusedSettingsNodes(ImGuiContext* ctx);
    static ImGuiDockNode*   DockContextFindNodeByID(ImGuiContext* ctx, ImGuiID id);
    static void             DockContextClearNodes(ImGuiContext* ctx, ImGuiID root_id, bool clear_persistent_docking_refs);    // Set root_id==0 to clear all
    static void             DockContextBuildNodesFromSettings(ImGuiContext* ctx, ImGuiDockNodeSettings* node_settings_array, int node_settings_count);
    static void             DockContextBuildAddWindowsToNodes(ImGuiContext* ctx, ImGuiID root_id);                                  // Use root_id==0 to add all

    // ImGuiDockNode
    static void             DockNodeAddWindow(ImGuiDockNode* node, ImGuiWindow* window, bool add_to_tab_bar);
    static void             DockNodeMoveWindows(ImGuiDockNode* dst_node, ImGuiDockNode* src_node);
    static void             DockNodeMoveChildNodes(ImGuiDockNode* dst_node, ImGuiDockNode* src_node);
    static void             DockNodeApplyPosSizeToWindows(ImGuiDockNode* node);
    static void             DockNodeRemoveWindow(ImGuiDockNode* node, ImGuiWindow* window, ImGuiID save_dock_id);
    static void             DockNodeHideHostWindow(ImGuiDockNode* node);
    static void             DockNodeUpdate(ImGuiDockNode* node);
    static void             DockNodeUpdateVisibleFlagAndInactiveChilds(ImGuiDockNode* node);
    static void             DockNodeUpdateTabBar(ImGuiDockNode* node, ImGuiWindow* host_window);
    static void             DockNodeUpdateVisibleFlag(ImGuiDockNode* node);
    static void             DockNodeStartMouseMovingWindow(ImGuiDockNode* node, ImGuiWindow* window);
    static bool             DockNodeIsDropAllowed(ImGuiWindow* host_window, ImGuiWindow* payload_window);
    static bool             DockNodePreviewDockCalc(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* payload_window, ImGuiDockPreviewData* preview_data, bool is_explicit_target, bool is_outer_docking);
    static void             DockNodePreviewDockRender(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* payload_window, const ImGuiDockPreviewData* preview_data);
    static ImRect           DockNodeCalcTabBarRect(const ImGuiDockNode* node);
    static void             DockNodeCalcSplitRects(ImVec2& pos_old, ImVec2& size_old, ImVec2& pos_new, ImVec2& size_new, ImGuiDir dir, ImVec2 size_new_desired);
    static bool             DockNodeCalcDropRects(const ImRect& parent, ImGuiDir dir, ImRect& out_draw, bool outer_docking);
    static const char*      DockNodeGetHostWindowTitle(ImGuiDockNode* node, char* buf, int buf_size) { ImFormatString(buf, buf_size, "##DockNode_%02X", node->ID); return buf; }
    static int              DockNodeGetDepth(const ImGuiDockNode* node) { int depth = 0; while (node->ParentNode) { node = node->ParentNode; depth++; } return depth; }
    static int              DockNodeGetTabOrder(ImGuiWindow* window);

    // ImGuiDockNode tree manipulations
    static void             DockNodeTreeSplit(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiAxis split_axis, int split_first_child, float split_ratio, ImGuiDockNode* new_node);
    static void             DockNodeTreeMerge(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiDockNode* merge_lead_child);
    static void             DockNodeTreeUpdatePosSize(ImGuiDockNode* node, ImVec2 pos, ImVec2 size);
    static void             DockNodeTreeUpdateSplitter(ImGuiDockNode* node);
    static ImGuiDockNode*   DockNodeTreeFindNodeByPos(ImGuiDockNode* node, ImVec2 pos);
    static ImGuiDockNode*   DockNodeTreeFindFallbackLeafNode(ImGuiDockNode* node);

    // Settings
    static void             DockSettingsMoveDockReferencesInInactiveWindow(ImGuiID old_dock_id, ImGuiID new_dock_id);
    static void             DockSettingsRemoveReferencesToNodes(ImGuiID* node_ids, int node_ids_count);
    static ImGuiDockNodeSettings*   DockSettingsFindNodeSettings(ImGuiContext* ctx, ImGuiID node_id);
    static void*            DockSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
    static void             DockSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
    static void             DockSettingsHandler_WriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);
}

//-----------------------------------------------------------------------------
// Docking: ImGuiDockContext
//-----------------------------------------------------------------------------
// The lifetime model is different from the one of regular windows: we always create a ImGuiDockNode for each ImGuiDockNodeSettings,
// or we always hold the entire docking node tree. Nodes are frequently hidden, e.g. if the window(s) or child nodes they host are not active.
// At boot time only, we run a simple GC to remove nodes that have no references.
// Because dock node settings (which are small, contiguous structures) are always mirrored by their corresponding dock nodes (more complete structures),
// we can also very easily recreate the nodes from scratch given the settings data (this is what  DockContextRebuild() does).
// This is convenient as docking reconfiguration can be implemented by mostly poking at the simpler setttings data.
//-----------------------------------------------------------------------------

void ImGui::DockContextInitialize(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    IM_ASSERT(g.DockContext == NULL);
    g.DockContext = IM_NEW(ImGuiDockContext)();

    // Add .ini handle for persistent docking data
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "Docking";
    ini_handler.TypeHash = ImHash("Docking", 0, 0);
    ini_handler.ReadOpenFn = DockSettingsHandler_ReadOpen;
    ini_handler.ReadLineFn = DockSettingsHandler_ReadLine;
    ini_handler.WriteAllFn = DockSettingsHandler_WriteAll;
    g.SettingsHandlers.push_back(ini_handler);
}

void ImGui::DockContextShutdown(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = ctx->DockContext;
    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
            IM_DELETE(node);
    IM_DELETE(g.DockContext);
    g.DockContext = NULL;
}

void ImGui::DockContextOnLoadSettings(ImGuiContext* ctx)
{
    ImGuiDockContext* dc = ctx->DockContext;
    DockContextPruneUnusedSettingsNodes(ctx);
    DockContextBuildNodesFromSettings(ctx, dc->SettingsNodes.Data, dc->SettingsNodes.Size);
}

void ImGui::DockContextClearNodes(ImGuiContext* ctx, ImGuiID root_id, bool clear_persistent_docking_references)
{
	(void)ctx;
    IM_ASSERT(ctx == GImGui);
    DockBuilderRemoveNodeDockedWindows(root_id, clear_persistent_docking_references);
    DockBuilderRemoveNodeChildNodes(root_id);
}

// This function also acts as a defacto test to make sure we can rebuild from scratch without a glitch
void ImGui::DockContextRebuild(ImGuiContext* ctx)
{
    //IMGUI_DEBUG_LOG("[docking] full rebuild\n");
    ImGuiDockContext* dc = ctx->DockContext;
    SaveIniSettingsToMemory();
    ImGuiID root_id = 0; // Rebuild all
    DockContextClearNodes(ctx, root_id, false);
    DockContextBuildNodesFromSettings(ctx, dc->SettingsNodes.Data, dc->SettingsNodes.Size);
    DockContextBuildAddWindowsToNodes(ctx, root_id);
}

// Docking context update function, called by NewFrame()
void ImGui::DockContextNewFrameUpdateUndocking(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = ctx->DockContext;
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
    {
        if (dc->Nodes.Data.Size > 0 || dc->Requests.Size > 0)
            DockContextClearNodes(ctx, 0, true);
        return;
    }
    if (g.IO.ConfigDockingNoSplit)
    {
        for (int n = 0; n < dc->Nodes.Data.Size; n++)
            if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
                if (node->IsRootNode() && node->IsSplitNode())
                {
                    DockBuilderRemoveNodeChildNodes(node->ID);
                    //dc->WantFullRebuild = true;
                }
    }

#if 0
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
        dc->WantFullRebuild = true;
#endif
    if (dc->WantFullRebuild)
    {
        DockContextRebuild(ctx);
        dc->WantFullRebuild = false;
    }

    // Process Undocking requests (we need to process them _before_ the UpdateMouseMovingWindow call in NewFrame)
    for (int n = 0; n < dc->Requests.Size; n++)
    {
        ImGuiDockRequest* req = &dc->Requests[n];
        if (req->Type == ImGuiDockRequestType_Undock && req->UndockTargetWindow)
            DockContextProcessUndockWindow(ctx, req->UndockTargetWindow);
        else if (req->Type == ImGuiDockRequestType_Undock && req->UndockTargetNode)
            DockContextProcessUndockNode(ctx, req->UndockTargetNode);
    }
}

// Docking context update function, called by NewFrame()
void ImGui::DockContextNewFrameUpdateDocking(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = ctx->DockContext;
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
        return;

    // Process Docking requests
    for (int n = 0; n < dc->Requests.Size; n++)
        if (dc->Requests[n].Type == ImGuiDockRequestType_Dock)
            DockContextProcessDock(ctx, &dc->Requests[n]);
    dc->Requests.resize(0);

    // Create windows for each automatic docking nodes
    // We can have NULL pointers when we delete nodes, but because ID are recycled this should amortize nicely (and our node count will never be very high)
    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
            if (!node->IsDockSpace && node->IsRootNode())
                DockNodeUpdate(node);
}

static ImGuiDockNode* ImGui::DockContextFindNodeByID(ImGuiContext* ctx, ImGuiID id)
{
    return (ImGuiDockNode*)ctx->DockContext->Nodes.GetVoidPtr(id);
}

static ImGuiID ImGui::DockContextGenNodeID(ImGuiContext* ctx)
{
    // Generate an ID for new node (the exact ID value doesn't matter as long as it is not already used)
    // FIXME-OPT: This is suboptimal, even if the node count is small enough not to be a worry. We should poke in ctx->Nodes to find a suitable ID faster.
    ImGuiID id = 0x0001;
    while (DockContextFindNodeByID(ctx, id) != NULL)
        id++;
    return id;
}

static ImGuiDockNode* ImGui::DockContextAddNode(ImGuiContext* ctx, ImGuiID id)
{
    // Generate an ID for the new node (the exact ID value doesn't matter as long as it is not already used) and add the first window.
    if (id == 0)
        id = DockContextGenNodeID(ctx);
    else
        IM_ASSERT(DockContextFindNodeByID(ctx, id) == NULL);
    ImGuiDockNode* node = IM_NEW(ImGuiDockNode)(id);
    node->InitFromFirstWindowPosSize = node->InitFromFirstWindowViewport = true;
    ctx->DockContext->Nodes.SetVoidPtr(node->ID, node);
    return node;
}

static void ImGui::DockContextRemoveNode(ImGuiContext* ctx, ImGuiDockNode* node, bool merge_sibling_into_parent_node)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = ctx->DockContext;

    //printf("[%05d] RemoveNode 0x%04X\n", node->ID);
    IM_ASSERT(DockContextFindNodeByID(ctx, node->ID) == node);
    IM_ASSERT(node->ChildNodes[0] == NULL && node->ChildNodes[1] == NULL);
    IM_ASSERT(node->Windows.Size == 0);

    if (node->HostWindow)
        node->HostWindow->DockNodeAsHost = NULL;

    ImGuiDockNode* parent_node = node->ParentNode;
    const bool merge = (merge_sibling_into_parent_node && parent_node != NULL);
    if (merge)
    {
        IM_ASSERT(parent_node->ChildNodes[0] == node || parent_node->ChildNodes[1] == node);
        ImGuiDockNode* sibling_node = (parent_node->ChildNodes[0] == node ? parent_node->ChildNodes[1] : parent_node->ChildNodes[0]);
        DockNodeTreeMerge(&g, parent_node, sibling_node);
    }
    else
    {
        for (int n = 0; parent_node && n < IM_ARRAYSIZE(parent_node->ChildNodes); n++)
            if (parent_node->ChildNodes[n] == node)
                node->ParentNode->ChildNodes[n] = NULL;
        dc->Nodes.SetVoidPtr(node->ID, NULL);
        IM_DELETE(node);
    }
}

static int IMGUI_CDECL DockNodeComparerDepthMostFirst(const void* lhs, const void* rhs)
{
    const ImGuiDockNode* a = *(const ImGuiDockNode* const*)lhs;
    const ImGuiDockNode* b = *(const ImGuiDockNode* const*)rhs;
    return ImGui::DockNodeGetDepth(b) - ImGui::DockNodeGetDepth(a);
}

// Pre C++0x doesn't allow us to use a local type (without linkage) as template parameter, so we moved this here.
struct ImGuiDockContextPruneNodeData
{
    int CountWindows, CountChildWindows, CountChildNodes;
    ImGuiID RootID;
    ImGuiDockContextPruneNodeData() { CountWindows = CountChildWindows = CountChildNodes = 0; RootID = 0; }
};

// Garbage collect unused nodes (run once at init time)
static void ImGui::DockContextPruneUnusedSettingsNodes(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = ctx->DockContext;
    IM_ASSERT(g.Windows.Size == 0);

    ImPool<ImGuiDockContextPruneNodeData> pool;
    pool.Reserve(dc->SettingsNodes.Size);

    // Count child nodes and compute RootID
    for (int settings_n = 0; settings_n < dc->SettingsNodes.Size; settings_n++)
    {
        ImGuiDockNodeSettings* settings = &dc->SettingsNodes[settings_n];
        ImGuiDockContextPruneNodeData* parent_data = settings->ParentID ? pool.GetByKey(settings->ParentID) : 0;
        pool.GetOrAddByKey(settings->ID)->RootID = parent_data ? parent_data->RootID : settings->ID;
        if (settings->ParentID)
            pool.GetOrAddByKey(settings->ParentID)->CountChildNodes++;
    }

    // Count reference to dock ids from window settings
    for (int settings_n = 0; settings_n < g.SettingsWindows.Size; settings_n++)
        if (ImGuiID dock_id = g.SettingsWindows[settings_n].DockId)
            if (ImGuiDockContextPruneNodeData* data = pool.GetByKey(dock_id))
            {
                ImGuiDockContextPruneNodeData* data_root = (data->RootID == dock_id) ? data : pool.GetByKey(data->RootID);
                data->CountWindows++;
                data_root->CountChildWindows++;
            }

    // Prune
    for (int settings_n = 0; settings_n < dc->SettingsNodes.Size; settings_n++)
    {
        ImGuiDockNodeSettings* settings = &dc->SettingsNodes[settings_n];
        ImGuiDockContextPruneNodeData* data = pool.GetByKey(settings->ID);
        if (data->CountWindows > 1)
            continue;
        ImGuiDockContextPruneNodeData* data_root = (data->RootID == settings->ID) ? data : pool.GetByKey(data->RootID);

        bool remove = false;
        remove |= (data->CountWindows == 1 && settings->ParentID == 0 && data->CountChildNodes == 0 && !settings->IsCentralNode);  // Floating root node with only 1 window
        remove |= (data->CountWindows == 0 && settings->ParentID == 0 && data->CountChildNodes == 0); // Leaf nodes with 0 window
        remove |= (data_root->CountChildWindows == 0);
        if (remove)
        {
            DockSettingsRemoveReferencesToNodes(&settings->ID, 1);
            settings->ID = 0;
        }
    }
}

static void ImGui::DockContextBuildNodesFromSettings(ImGuiContext* ctx, ImGuiDockNodeSettings* node_settings_array, int node_settings_count)
{
    // Build nodes
    for (int node_n = 0; node_n < node_settings_count; node_n++)
    {
        ImGuiDockNodeSettings* node_settings = &node_settings_array[node_n];
        if (node_settings->ID == 0)
            continue;
        ImGuiDockNode* node = DockContextAddNode(ctx, node_settings->ID);
        node->ParentNode = node_settings->ParentID ? DockContextFindNodeByID(ctx, node_settings->ParentID) : NULL;
        node->Pos = ImVec2(node_settings->Pos.x, node_settings->Pos.y);
        node->Size = ImVec2(node_settings->Size.x, node_settings->Size.y);
        node->SizeRef = ImVec2(node_settings->SizeRef.x, node_settings->SizeRef.y);
        if (node->ParentNode && node->ParentNode->ChildNodes[0] == NULL)
            node->ParentNode->ChildNodes[0] = node;
        else if (node->ParentNode && node->ParentNode->ChildNodes[1] == NULL)
            node->ParentNode->ChildNodes[1] = node;
        node->SelectedTabID = node_settings->SelectedTabID;
        node->SplitAxis = node_settings->SplitAxis;
        node->IsDockSpace = node_settings->IsDockSpace != 0;
        node->IsCentralNode = node_settings->IsCentralNode != 0;
        node->IsHiddenTabBar = node_settings->IsHiddenTabBar != 0;

        // Bind host window immediately if it already exist (in case of a rebuild)
        // This is useful as the RootWindowForTitleBarHighlight links necessary to highlight the currently focused node requires node->HostWindow to be set.
        char host_window_title[20];
        ImGuiDockNode* root_node = DockNodeGetRootNode(node);
        node->HostWindow = FindWindowByName(DockNodeGetHostWindowTitle(root_node, host_window_title, IM_ARRAYSIZE(host_window_title)));
    }
}

void ImGui::DockContextBuildAddWindowsToNodes(ImGuiContext* ctx, ImGuiID root_id)
{
    // Rebind all windows to nodes (they can also lazily rebind but we'll have a visible glitch during the first frame)
    ImGuiContext& g = *ctx;
    for (int n = 0; n < g.Windows.Size; n++)
    {
        ImGuiWindow* window = g.Windows[n];
        if (window->DockId == 0 || window->LastFrameActive < g.FrameCount - 1)
            continue;
        if (window->DockNode != NULL)
            continue;

        ImGuiDockNode* dock_node = DockContextFindNodeByID(ctx, window->DockId);
        IM_ASSERT(dock_node != NULL);   // This should have been called after DockContextBuildNodesFromSettings()
        if (root_id == 0 || DockNodeGetRootNode(dock_node)->ID == root_id)
            DockNodeAddWindow(dock_node, window, true);
    }
}

//-----------------------------------------------------------------------------
// Docking: ImGuiDockContext Docking/Undocking functions
//-----------------------------------------------------------------------------

void ImGui::DockContextQueueDock(ImGuiContext* ctx, ImGuiWindow* target, ImGuiDockNode* target_node, ImGuiWindow* payload, ImGuiDir split_dir, float split_ratio, bool split_outer)
{
    IM_ASSERT(target != payload);
    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Dock;
    req.DockTargetWindow = target;
    req.DockTargetNode = target_node;
    req.DockPayload = payload;
    req.DockSplitDir = split_dir;
    req.DockSplitRatio = split_ratio;
    req.DockSplitOuter = split_outer;
    ctx->DockContext->Requests.push_back(req);
}

void ImGui::DockContextQueueUndockWindow(ImGuiContext* ctx, ImGuiWindow* window)
{
    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Undock;
    req.UndockTargetWindow = window;
    ctx->DockContext->Requests.push_back(req);
}

void ImGui::DockContextQueueUndockNode(ImGuiContext* ctx, ImGuiDockNode* node)
{
    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Undock;
    req.UndockTargetNode = node;
    ctx->DockContext->Requests.push_back(req);
}

void ImGui::DockContextQueueNotifyRemovedNode(ImGuiContext* ctx, ImGuiDockNode* node)
{
    ImGuiDockContext* dc = ctx->DockContext;
    for (int n = 0; n < dc->Requests.Size; n++)
        if (dc->Requests[n].DockTargetNode == node)
            dc->Requests[n].Type = ImGuiDockRequestType_None;
}

void ImGui::DockContextProcessDock(ImGuiContext* ctx, ImGuiDockRequest* req)
{
    IM_ASSERT((req->Type == ImGuiDockRequestType_Dock && req->DockPayload != NULL) || (req->Type == ImGuiDockRequestType_Split && req->DockPayload == NULL));
    IM_ASSERT(req->DockTargetWindow != NULL || req->DockTargetNode != NULL);

    ImGuiContext& g = *ctx;
	(void)g;

    ImGuiWindow* payload_window = req->DockPayload;     // Optional
    ImGuiWindow* target_window = req->DockTargetWindow;
    ImGuiDockNode* target_node = req->DockTargetNode;

    // Decide which Tab will be selected at the end of the operation
    ImGuiID next_selected_id = 0;
    ImGuiDockNode* payload_node = NULL;
    if (payload_window)
    {
        payload_node = payload_window->DockNodeAsHost;
        payload_window->DockNodeAsHost = NULL; // Important to clear this as the node will have its life as a child which might be merged/deleted later.
        if (payload_node && payload_node->IsLeafNode())
            next_selected_id = payload_node->TabBar->NextSelectedTabId ? payload_node->TabBar->NextSelectedTabId : payload_node->TabBar->SelectedTabId;
        if (payload_node == NULL)
            next_selected_id = payload_window->ID;
    }

    // FIXME-DOCK: When we are trying to dock an existing single-window node into a loose window, transfer Node ID as well

    if (target_node)
        IM_ASSERT(target_node->LastFrameAlive < g.FrameCount);
    if (target_node && target_window && target_node == target_window->DockNodeAsHost)
        IM_ASSERT(target_node->Windows.Size > 0 || target_node->IsSplitNode() || target_node->IsCentralNode);

    // Create new node and add existing window to it
    if (target_node == NULL)
    {
        target_node = DockContextAddNode(ctx, 0);
        target_node->Pos = target_window->Pos;
        target_node->Size = target_window->Size;
        if (target_window->DockNodeAsHost == NULL)
        {
            DockNodeAddWindow(target_node, target_window, true);
            target_node->TabBar->Tabs[0].Flags &= ~ImGuiTabItemFlags_Unsorted;
            target_window->DockIsActive = true;
        }
    }

    ImGuiDir split_dir = req->DockSplitDir;
    if (split_dir != ImGuiDir_None)
    {
        // Split into one, one side will be our payload node unless we are dropping a loose window
        const ImGuiAxis split_axis = (split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;
        const int split_inheritor_child_idx = (split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? 1 : 0;
        const float split_ratio = req->DockSplitRatio;
        DockNodeTreeSplit(ctx, target_node, split_axis, split_inheritor_child_idx, split_ratio, payload_node);  // payload_node may be NULL here!
        ImGuiDockNode* inheritor_node = target_node->ChildNodes[split_inheritor_child_idx];
        ImGuiDockNode* new_node = target_node->ChildNodes[split_inheritor_child_idx ^ 1];
        new_node->HostWindow = target_node->HostWindow;
        inheritor_node->IsCentralNode = target_node->IsCentralNode;
        inheritor_node->IsHiddenTabBar = target_node->IsHiddenTabBar;
        target_node->IsCentralNode = false;
        target_node = new_node;
    }
    target_node->IsHiddenTabBar = false;

    if (target_node != payload_node)
    {
        // Create tab bar before we call DockNodeMoveWindows (which would attempt to move the old tab-bar, which would lead us to payload tabs wrongly appearing before target tabs!)
        if (target_node->Windows.Size > 0 && target_node->TabBar == NULL)
        {
            target_node->TabBar = IM_NEW(ImGuiTabBar)();
            for (int n = 0; n < target_node->Windows.Size; n++)
                TabBarAddTab(target_node->TabBar, ImGuiTabItemFlags_None, target_node->Windows[n]);
        }

        if (payload_node != NULL)
        {
            // Transfer full payload node (with 1+ child windows or child nodes)
            if (payload_node->IsSplitNode())
            {
                if (target_node->Windows.Size > 0)
                {
                    // We can dock into a node that already has windows _only_ if our payload is a node tree with a single visible node.
                    // In this situation, we move the windows of the target node into the currently visible node of the payload.
                    // This allows us to preserve some of the underlying dock tree settings nicely.
                    IM_ASSERT(payload_node->OnlyNodeWithWindows != NULL); // The docking should have been blocked by DockNodePreviewDockCalc() early on and never submitted.
                    ImGuiDockNode* visible_node = payload_node->OnlyNodeWithWindows;
                    if (visible_node->TabBar)
                        IM_ASSERT(visible_node->TabBar->Tabs.Size > 0);
                    DockNodeMoveWindows(target_node, visible_node);
                    DockNodeMoveWindows(visible_node, target_node);
                    DockSettingsMoveDockReferencesInInactiveWindow(target_node->ID, visible_node->ID);
                }
                if (target_node->IsCentralNode)
                {
                    // Central node property needs to be moved to a leaf node, pick the last focused one.
                    ImGuiDockNode* last_focused_node = DockContextFindNodeByID(ctx, payload_node->LastFocusedNodeID);
                    IM_ASSERT(last_focused_node != NULL && DockNodeGetRootNode(last_focused_node) == DockNodeGetRootNode(payload_node));
                    last_focused_node->IsCentralNode = true;
                    target_node->IsCentralNode = false;
                }

                IM_ASSERT(target_node->Windows.Size == 0);
                DockNodeMoveChildNodes(target_node, payload_node);
            }
            else
            {
                const ImGuiID payload_dock_id = payload_node->ID;
                DockNodeMoveWindows(target_node, payload_node);
                DockSettingsMoveDockReferencesInInactiveWindow(payload_dock_id, target_node->ID);
            }
            DockContextRemoveNode(ctx, payload_node, true);
        }
        else if (payload_window)
        {
            // Transfer single window
            const ImGuiID payload_dock_id = payload_window->DockId;
            target_node->VisibleWindow = payload_window;
            DockNodeAddWindow(target_node, payload_window, true);
            if (payload_dock_id != 0)
                DockSettingsMoveDockReferencesInInactiveWindow(payload_dock_id, target_node->ID);
        }
    }

    // Update selection immediately
    if (ImGuiTabBar* tab_bar = target_node->TabBar)
        tab_bar->NextSelectedTabId = next_selected_id;
    MarkIniSettingsDirty();
}

void ImGui::DockContextProcessUndockWindow(ImGuiContext* ctx, ImGuiWindow* window, bool clear_persistent_docking_ref)
{
    (void)ctx;
    if (window->DockNode)
        DockNodeRemoveWindow(window->DockNode, window, clear_persistent_docking_ref ? 0 : window->DockId);
    else
        window->DockId = 0;
    window->Collapsed = false;
    window->DockIsActive = false;
    window->DockTabIsVisible = false;
    MarkIniSettingsDirty();
}

void ImGui::DockContextProcessUndockNode(ImGuiContext* ctx, ImGuiDockNode* node)
{
    IM_ASSERT(node->IsLeafNode());
    IM_ASSERT(node->Windows.Size >= 1);

    // In the case of a root node or central node, the node will have to stay in place. Create a new node to receive the payload.
    // Otherwise delete the previous node by merging the other sibling back into the parent node.
    if (node->IsRootNode() || node->IsCentralNode)
    {
        ImGuiDockNode* new_node = DockContextAddNode(ctx, 0);
        DockNodeMoveWindows(new_node, node);
        DockSettingsMoveDockReferencesInInactiveWindow(node->ID, new_node->ID);
        for (int n = 0; n < new_node->Windows.Size; n++)
            UpdateWindowParentAndRootLinks(new_node->Windows[n], new_node->Windows[n]->Flags, NULL);
        new_node->WantMouseMove = true;
    }
    else
    {
        IM_ASSERT(node->ParentNode->ChildNodes[0] == node || node->ParentNode->ChildNodes[1] == node);
        int index_in_parent = (node->ParentNode->ChildNodes[0] == node) ? 0 : 1;
        node->ParentNode->ChildNodes[index_in_parent] = NULL;
        DockNodeTreeMerge(ctx, node->ParentNode, node->ParentNode->ChildNodes[index_in_parent ^ 1]);
        node->ParentNode->InitFromFirstWindowViewport = true; // The node that stays in place keeps the viewport, so our newly dragged out node will create a new viewport
        node->ParentNode = NULL;
        node->InitFromFirstWindowPosSize = true;
        node->WantMouseMove = true;
    }
    MarkIniSettingsDirty();
}

//-----------------------------------------------------------------------------
// Docking: ImGuiDockNode
//-----------------------------------------------------------------------------

ImGuiDockNode::ImGuiDockNode(ImGuiID id)
{
    ID = id;
    Flags = 0;
    ParentNode = ChildNodes[0] = ChildNodes[1] = NULL;
    TabBar = NULL;
    SplitAxis = ImGuiAxis_None;

    HostWindow = VisibleWindow = NULL;
    CentralNode = OnlyNodeWithWindows = NULL;
    LastFrameAlive = LastFrameActive = LastFrameFocused = -1;
    LastFocusedNodeID = 0;
    SelectedTabID = 0;
    WantCloseTabID = 0;
    InitFromFirstWindowPosSize = InitFromFirstWindowViewport = false;
    IsVisible = true;
    IsFocused = IsDockSpace = IsCentralNode = IsHiddenTabBar = HasCloseButton = HasCollapseButton = false;
    WantCloseAll = WantLockSizeOnce = WantMouseMove = WantHiddenTabBarToggle = false;
}

ImGuiDockNode::~ImGuiDockNode()
{
    IM_DELETE(TabBar);
    TabBar = NULL;
    ChildNodes[0] = ChildNodes[1] = NULL;
}

int ImGui::DockNodeGetTabOrder(ImGuiWindow* window)
{
    ImGuiTabBar* tab_bar = window->DockNode->TabBar;
    if (tab_bar == NULL)
        return -1;
    ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, window->ID);
    return tab ? tab_bar->GetTabOrder(tab) : -1;
}

static void ImGui::DockNodeAddWindow(ImGuiDockNode* node, ImGuiWindow* window, bool add_to_tab_bar)
{
    ImGuiContext& g = *GImGui; (void)g;
    if (window->DockNode)
    {
        // Can overwrite an existing window->DockNode (e.g. pointing to a disabled DockSpace node)
        IM_ASSERT(window->DockNode->ID != node->ID);
        DockNodeRemoveWindow(window->DockNode, window, 0);
    }
    IM_ASSERT(window->DockNode == NULL || window->DockNodeAsHost == NULL);

    node->Windows.push_back(window);
    window->DockNode = node;
    window->DockId = node->ID;
    window->DockIsActive = (node->Windows.Size > 1);
    window->DockTabWantClose = false;

    // If 2+ windows appeared on the same frame, creating a new DockNode+TabBar from the second window, 
    // then we need to hide the first one after the fact otherwise it would be visible as a standalone window for one frame.
    if (node->HostWindow == NULL && node->Windows.Size == 2 && node->Windows[0]->WasActive == false)
    {
        node->Windows[0]->Hidden = true;
        node->Windows[0]->HiddenFramesRegular = 1;
    }

    // When reactivating a node with one or two loose window, the window pos/size/viewport are authoritative over the node storage.
    // In particular it is important we init the viewport from the first window so we don't create two viewports and drop one.
    if (node->HostWindow == NULL && !node->IsDockSpace && node->IsRootNode())
        node->InitFromFirstWindowPosSize = node->InitFromFirstWindowViewport = true;

    // Add to tab bar if requested
    if (add_to_tab_bar)
    {
        if (node->TabBar == NULL)
        {
            node->TabBar = IM_NEW(ImGuiTabBar)();
            node->TabBar->SelectedTabId = node->TabBar->NextSelectedTabId = node->SelectedTabID;
            
            // Add existing windows
            for (int n = 0; n < node->Windows.Size - 1; n++)
                TabBarAddTab(node->TabBar, ImGuiTabItemFlags_None, node->Windows[n]);
        }
        TabBarAddTab(node->TabBar, ImGuiTabItemFlags_Unsorted, window);
    }

    DockNodeUpdateVisibleFlag(node);

    // Update this without waiting for the next time we Begin() in the window, so our host window will have the proper title bar color on its first frame.
    if (node->HostWindow)
        UpdateWindowParentAndRootLinks(window, window->Flags | ImGuiWindowFlags_ChildWindow, node->HostWindow);
}

static void ImGui::DockNodeRemoveWindow(ImGuiDockNode* node, ImGuiWindow* window, ImGuiID save_dock_id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(window->DockNode == node);
    //IM_ASSERT(window->RootWindow == node->HostWindow);
    //IM_ASSERT(window->LastFrameActive < g.FrameCount);    // We may call this from Begin()
    IM_ASSERT(save_dock_id == 0 || save_dock_id == node->ID);

    window->DockNode = NULL;
    window->DockIsActive = window->DockTabWantClose = false;
    window->DockId = save_dock_id;
    UpdateWindowParentAndRootLinks(window, window->Flags & ~ImGuiWindowFlags_ChildWindow, NULL); // Update immediately

    // Remove window
    bool erased = false;
    for (int n = 0; n < node->Windows.Size; n++)
        if (node->Windows[n] == window)
        {
            node->Windows.erase(node->Windows.Data + n);
            erased = true;
            break;
        }
    IM_ASSERT(erased);

    // Remove tab and possibly tab bar
    if (node->TabBar)
    {
        TabBarRemoveTab(node->TabBar, window->ID);
        const int tab_count_threshold_for_tab_bar = node->IsCentralNode ? 1 : 2;
        if (node->Windows.Size < tab_count_threshold_for_tab_bar)
        {
            IM_DELETE(node->TabBar);
            node->TabBar = NULL;
        }
    }

    if (node->Windows.Size == 0 && !node->IsCentralNode && window->DockId != node->ID)
    {
        // Automatic dock node delete themselves if they are not holding at least one tab
        DockContextRemoveNode(&g, node, true);
        return;
    }

    if (node->Windows.Size == 1 && !node->IsCentralNode && node->HostWindow)
    {
        ImGuiWindow* remaining_window = node->Windows[0];
        if (node->HostWindow->ViewportOwned && node->IsRootNode())
        {
            // Transfer viewport back to the remaining loose window
            IM_ASSERT(node->HostWindow->Viewport->Window == node->HostWindow);
            node->HostWindow->Viewport->Window = remaining_window;
            node->HostWindow->Viewport->ID = remaining_window->ID;
        }
        remaining_window->Collapsed = node->HostWindow->Collapsed;
    }

    // Update visibility immediately is required so the DockNodeUpdateRemoveInactiveChilds() processing can reflect changes up the tree
    DockNodeUpdateVisibleFlag(node);
}

static void ImGui::DockNodeMoveChildNodes(ImGuiDockNode* dst_node, ImGuiDockNode* src_node)
{
    IM_ASSERT(dst_node->Windows.Size == 0);
    dst_node->ChildNodes[0] = src_node->ChildNodes[0];
    dst_node->ChildNodes[1] = src_node->ChildNodes[1];
    if (dst_node->ChildNodes[0])
        dst_node->ChildNodes[0]->ParentNode = dst_node;
    if (dst_node->ChildNodes[1])
        dst_node->ChildNodes[1]->ParentNode = dst_node;
    dst_node->SplitAxis = src_node->SplitAxis;
    dst_node->SizeRef = src_node->SizeRef;
    src_node->ChildNodes[0] = src_node->ChildNodes[1] = NULL;
}

static void ImGui::DockNodeMoveWindows(ImGuiDockNode* dst_node, ImGuiDockNode* src_node)
{
    // Insert tabs in the same orders as currently ordered (node->Windows isn't ordered)
    IM_ASSERT(src_node && dst_node && dst_node != src_node);
    ImGuiTabBar* src_tab_bar = src_node->TabBar;
    if (src_tab_bar != NULL)
        IM_ASSERT(src_node->Windows.Size == src_node->TabBar->Tabs.Size);

    // If the dst_node is empty we can just move the entire tab bar (to preserve selection, scrolling, etc.)
    bool move_tab_bar = (src_tab_bar != NULL) && (dst_node->TabBar == NULL);
    if (move_tab_bar)
    {
        dst_node->TabBar = src_node->TabBar;
        src_node->TabBar = NULL;
    }

    for (int n = 0; n < src_node->Windows.Size; n++)
    {
        ImGuiWindow* window = src_tab_bar ? src_tab_bar->Tabs[n].Window : src_node->Windows[n];
        window->DockNode = NULL;
        window->DockIsActive = false;
        DockNodeAddWindow(dst_node, window, move_tab_bar ? false : true);
    }
    src_node->Windows.clear();

    if (!move_tab_bar && src_node->TabBar)
    {
        if (dst_node->TabBar)
            dst_node->TabBar->SelectedTabId = src_node->TabBar->SelectedTabId;
        IM_DELETE(src_node->TabBar);
        src_node->TabBar = NULL;
    }
}

static void ImGui::DockNodeApplyPosSizeToWindows(ImGuiDockNode* node)
{
    for (int n = 0; n < node->Windows.Size; n++)
    {
        node->Windows[n]->Pos = node->Pos;
        node->Windows[n]->SizeFull = node->Size;
    }
}

static void ImGui::DockNodeHideHostWindow(ImGuiDockNode* node)
{
    if (node->HostWindow)
    {
        if (node->HostWindow->DockNodeAsHost == node)
            node->HostWindow->DockNodeAsHost = NULL;
        node->HostWindow = NULL;
    }

    if (node->Windows.Size == 1)
    {
        node->VisibleWindow = node->Windows[0];
        node->Windows[0]->DockIsActive = false;
    }

    if (node->TabBar)
    {
        IM_DELETE(node->TabBar);
        node->TabBar = NULL;
    }
}

struct ImGuiDockNodeUpdateScanResults
{
    ImGuiDockNode*      CentralNode;
    ImGuiDockNode*      FirstNodeWithWindows;
    int                 CountNodesWithWindows;
    //ImGuiWindowClass  WindowClassForMerges;

    ImGuiDockNodeUpdateScanResults() { CentralNode = FirstNodeWithWindows = NULL; CountNodesWithWindows = 0; }
};

static void DockNodeUpdateScanRec(ImGuiDockNode* node, ImGuiDockNodeUpdateScanResults* results)
{
    if (node->Windows.Size > 0)
    {
        if (results->FirstNodeWithWindows == NULL)
            results->FirstNodeWithWindows = node;
        results->CountNodesWithWindows++;
    }
    if (node->IsCentralNode)
    {
        IM_ASSERT(results->CentralNode == NULL); // Should be only one
        IM_ASSERT(node->IsLeafNode() && "If you get this assert: please submit .ini file + repro of actions leading to this.");
        results->CentralNode = node;
    }
    if (results->CountNodesWithWindows > 1 && results->CentralNode != NULL)
        return;
    if (node->ChildNodes[0])
        DockNodeUpdateScanRec(node->ChildNodes[0], results);
    if (node->ChildNodes[1])
        DockNodeUpdateScanRec(node->ChildNodes[1], results);
}

// - Remove inactive windows/nodes.
// - Update visibility flag.
static void ImGui::DockNodeUpdateVisibleFlagAndInactiveChilds(ImGuiDockNode* node)
{
    ImGuiContext& g = *GImGui;

    IM_ASSERT(node->ParentNode == NULL || node->ParentNode->ChildNodes[0] == node || node->ParentNode->ChildNodes[1] == node);

    // Inherit flags
    if (node->ParentNode)
        node->Flags = node->ParentNode->Flags;

    // Recurse into children
    // There is the possibility that one of our child becoming empty will delete itself and moving its sibling contents into 'node'.
    // If 'node->ChildNode[0]' delete itself, then 'node->ChildNode[1]->Windows' will be moved into 'node'
    // If 'node->ChildNode[1]' delete itself, then 'node->ChildNode[0]->Windows' will be moved into 'node' and the "remove inactive windows" loop will have run twice on those windows (harmless)
    if (node->ChildNodes[0])
        DockNodeUpdateVisibleFlagAndInactiveChilds(node->ChildNodes[0]);
    if (node->ChildNodes[1])
        DockNodeUpdateVisibleFlagAndInactiveChilds(node->ChildNodes[1]);

    // Remove inactive windows
    for (int window_n = 0; window_n < node->Windows.Size; window_n++)
    {
        ImGuiWindow* window = node->Windows[window_n];
        IM_ASSERT(window->DockNode == node);

        bool node_was_active = (node->LastFrameActive + 1 == g.FrameCount);
        bool remove = false;
        remove |= node_was_active && (window->LastFrameActive + 1 < g.FrameCount);
        remove |= node_was_active && (node->WantCloseAll || node->WantCloseTabID == window->ID) && window->HasCloseButton && !(window->Flags & ImGuiWindowFlags_UnsavedDocument);  // Submit all _expected_ closure from last frame
        remove |= (window->DockTabWantClose);
        if (!remove)
            continue;
        window->DockTabWantClose = false;
        if (node->Windows.Size == 1 && !node->IsCentralNode)
        {
            DockNodeHideHostWindow(node);
            DockNodeRemoveWindow(node, window, node->ID); // Will delete the node so it'll be invalid on return
            return;
        }
        DockNodeRemoveWindow(node, window, node->ID);
        window_n--;
    }

    // Apply toggles at a single point of the frame (here!)
    if (node->Windows.Size > 1)
        node->IsHiddenTabBar = false;
    else if (node->WantHiddenTabBarToggle)
        node->IsHiddenTabBar ^= 1;
    node->WantHiddenTabBarToggle = false;

    DockNodeUpdateVisibleFlag(node);
}

static void ImGui::DockNodeUpdateVisibleFlag(ImGuiDockNode* node)
{
    // Update visibility flag
    bool is_visible = (node->ParentNode == 0) ? node->IsDockSpace : node->IsCentralNode;
    is_visible |= (node->Windows.Size > 0);
    is_visible |= (node->ChildNodes[0] && node->ChildNodes[0]->IsVisible);
    is_visible |= (node->ChildNodes[1] && node->ChildNodes[1]->IsVisible);
    node->IsVisible = is_visible;
}

static void ImGui::DockNodeStartMouseMovingWindow(ImGuiDockNode* node, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(node->WantMouseMove == true);
    ImVec2 backup_active_click_offset = g.ActiveIdClickOffset;
    StartMouseMovingWindow(window);
    g.MovingWindow = window; // If we are docked into a non moveable root widnow, StartMouseMovingWindow() won't set g.MovingWindow. OVerride that decision.
    node->WantMouseMove = false;
    g.ActiveIdClickOffset = backup_active_click_offset;
}

static void ImGui::DockNodeUpdate(ImGuiDockNode* node)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(node->LastFrameActive != g.FrameCount);
    node->LastFrameAlive = g.FrameCount;

    node->CentralNode = node->OnlyNodeWithWindows = NULL;
    if (node->IsRootNode())
    {
        DockNodeUpdateVisibleFlagAndInactiveChilds(node);

        // FIXME-DOCK: Merge this scan into the one above.
        // - Setup central node pointers
        // - Find if there's only a single visible window in the hierarchy (in which case we need to display a regular title bar -> FIXME-DOCK: that last part is not done yet!)
        ImGuiDockNodeUpdateScanResults results;
        DockNodeUpdateScanRec(node, &results);
        node->CentralNode = results.CentralNode;
        node->OnlyNodeWithWindows = (results.CountNodesWithWindows == 1) ? results.FirstNodeWithWindows : NULL;
        if (node->LastFocusedNodeID == 0 && results.FirstNodeWithWindows != NULL)
            node->LastFocusedNodeID = results.FirstNodeWithWindows->ID;

        // Copy the window class from of our first window so it can be used for proper dock filtering.
        // When node has mixed windows, prioritize the class with the most constraint (CompatibleWithClassZero = false) as the reference to copy.
        // FIXME-DOCK: We don't recurse properly, this code could be reworked to work from DockNodeUpdateScanRec.
        if (ImGuiDockNode* first_node_with_windows = results.FirstNodeWithWindows)
        {
            node->WindowClass = first_node_with_windows->Windows[0]->WindowClass;
            for (int n = 1; n < first_node_with_windows->Windows.Size; n++)
                if (first_node_with_windows->Windows[n]->WindowClass.DockingAllowUnclassed == false)
                {
                    node->WindowClass = first_node_with_windows->Windows[n]->WindowClass;
                    break;
                }
        }
    }

    // Early out for hidden root dock nodes (when all DockId references are in inactive windows, or there is only 1 floating window holding on the DockId)
    if (node->Windows.Size <= 1 && node->IsRootNode() && node->IsLeafNode() && !node->IsDockSpace && !g.IO.ConfigDockingTabBarOnSingleWindows)
    {
        if (node->Windows.Size == 1)
        {
            // Floating window pos/size is authoritative
            ImGuiWindow* single_window = node->Windows[0];
            node->Pos = single_window->Pos;
            node->Size = single_window->SizeFull;

            // Transfer focus immediately so when we revert to a regular window it is immediately selected
            if (node->HostWindow && g.NavWindow == node->HostWindow)
                FocusWindow(single_window);
            if (node->HostWindow)
            {
                single_window->Viewport = node->HostWindow->Viewport;
                single_window->ViewportId = node->HostWindow->ViewportId;
                if (node->HostWindow->ViewportOwned)
                {
                    single_window->Viewport->Window = single_window;
                    single_window->ViewportOwned = true;
                }
            }
        }

        DockNodeHideHostWindow(node);
        node->InitFromFirstWindowPosSize = node->InitFromFirstWindowViewport = false;
        node->WantCloseAll = false;
        node->WantCloseTabID = 0;
        node->HasCloseButton = node->HasCollapseButton = false;
        node->LastFrameActive = g.FrameCount;

        if (node->WantMouseMove && node->Windows.Size == 1)
            DockNodeStartMouseMovingWindow(node, node->Windows[0]);
        return;
    }

    ImGuiWindow* host_window = NULL;
    bool beginned_into_host_window = false;
    if (node->IsDockSpace)
    {
        // [Explicit root dockspace node]
        IM_ASSERT(node->HostWindow);
        node->HasCloseButton = false;
        node->HasCollapseButton = true;
        host_window = node->HostWindow;
    }
    else
    {
        // [Automatic root or child nodes]
        node->HasCloseButton = false;
        node->HasCollapseButton = (node->Windows.Size > 0);
        for (int window_n = 0; window_n < node->Windows.Size; window_n++)
        {
            // FIXME-DOCK: Setting DockIsActive here means that for single active window in a leaf node, DockIsActive will be cleared until the next Begin() call.
            ImGuiWindow* window = node->Windows[window_n];
            window->DockIsActive = (node->Windows.Size > 1);
            node->HasCloseButton |= window->HasCloseButton;
        }

        if (node->IsRootNode() && node->IsVisible)
        {
            if (node->InitFromFirstWindowPosSize && node->Windows.Size > 0)
            {
                ImGuiWindow* init_window = node->Windows[0];
                SetNextWindowPos(init_window->Pos);
                SetNextWindowSize(init_window->SizeFull);
                SetNextWindowCollapsed(init_window->Collapsed);
            }
            else if (node->HostWindow == NULL)
            {
                SetNextWindowPos(node->Pos);
                SetNextWindowSize(node->Size);
            }
            if (node->InitFromFirstWindowViewport && node->Windows.Size > 0)
                SetNextWindowViewport(node->Windows[0]->ViewportId);
            SetNextWindowClass(&node->WindowClass);

            // Begin into the host window
            char window_label[20];
            DockNodeGetHostWindowTitle(node, window_label, IM_ARRAYSIZE(window_label));
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_DockNodeHost;
            window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
            window_flags |= ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoCollapse;
            window_flags |= ImGuiWindowFlags_NoTitleBar;

            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            Begin(window_label, NULL, window_flags);
            PopStyleVar();
            beginned_into_host_window = true;

            node->HostWindow = host_window = g.CurrentWindow;
            host_window->DockNodeAsHost = node;
            host_window->DC.CursorPos = host_window->Pos;
            node->Pos = host_window->Pos;
            node->Size = host_window->Size;
        }
        else if (node->ParentNode)
        {
            node->HostWindow = host_window = node->ParentNode->HostWindow;
        }
        node->InitFromFirstWindowPosSize = node->InitFromFirstWindowViewport = false;
        if (node->WantMouseMove && node->HostWindow)
            DockNodeStartMouseMovingWindow(node, node->HostWindow);
    }

    // Update focused node (the one whose title bar is highlight) within a node tree
    if (node->IsSplitNode())
        IM_ASSERT(node->TabBar == NULL);
    if (node->IsRootNode())
        if (g.NavWindow && g.NavWindow->RootWindowDockStop->DockNode && g.NavWindow->RootWindowDockStop->ParentWindow == host_window)
            node->LastFocusedNodeID = g.NavWindow->RootWindowDockStop->DockNode->ID;

    // We need to draw a background if requested by ImGuiDockNodeFlags_PassthruDockspace, but we will only know the correct pos/size after
    // processing the resizing splitters. So we are using the DrawList channel splitting facility to submit drawing primitives out of order!
    const bool render_dockspace_bg = node->IsRootNode() && host_window && (node->Flags & ImGuiDockNodeFlags_PassthruDockspace) != 0;
    if (render_dockspace_bg)
    {
        host_window->DrawList->ChannelsSplit(2);
        host_window->DrawList->ChannelsSetCurrent(1);
    }

    // Register a hit-test hole in the window unless we are currently dragging a window that is compatible with our dockspace
    const ImGuiDockNode* central_node = node->CentralNode;
    const bool central_node_hole = node->IsRootNode() && host_window && (node->Flags & ImGuiDockNodeFlags_PassthruDockspace) != 0 && central_node != NULL && central_node->IsEmpty();
    bool central_node_hole_register_hit_test_hole = central_node_hole;
    if (central_node_hole)
        if (const ImGuiPayload* payload = ImGui::GetDragDropPayload())
            if (payload->IsDataType(IMGUI_PAYLOAD_TYPE_WINDOW) && DockNodeIsDropAllowed(host_window, *(ImGuiWindow**)payload->Data))
                central_node_hole_register_hit_test_hole = false;
    if (central_node_hole_register_hit_test_hole)
    {
        // Add a little padding to match the "resize from edges" behavior and allow grabbing the splitter easily.
        IM_ASSERT(node->IsDockSpace); // We cannot pass this flag without the DockSpace() api. Testing this because we also setup the hole in host_window->ParentNode
        ImRect central_hole(central_node->Pos, central_node->Pos + central_node->Size);
        central_hole.Expand(ImVec2(-WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS, -WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS));
        if (central_node_hole && !central_hole.IsInverted())
        {
            SetWindowHitTestHole(host_window, central_hole.Min, central_hole.Max - central_hole.Min);
            SetWindowHitTestHole(host_window->ParentWindow, central_hole.Min, central_hole.Max - central_hole.Min);
        }
    }

    // Update position/size, process and draw resizing splitters
    if (node->IsRootNode() && host_window)
    {
        DockNodeTreeUpdatePosSize(node, host_window->Pos, host_window->Size);
        DockNodeTreeUpdateSplitter(node);
    }

    // Draw empty node background (currently can only be the Central Node)
    if (host_window && node->IsEmpty() && node->IsVisible && !(node->Flags & ImGuiDockNodeFlags_PassthruDockspace))
        host_window->DrawList->AddRectFilled(node->Pos, node->Pos + node->Size, GetColorU32(ImGuiCol_DockingEmptyBg));

    // Draw whole dockspace background if ImGuiDockNodeFlags_PassthruDockspace if set.
    if (render_dockspace_bg && node->IsVisible)
    {
        host_window->DrawList->ChannelsSetCurrent(0);
        if (central_node_hole)
            RenderRectFilledWithHole(host_window->DrawList, node->Rect(), central_node->Rect(), GetColorU32(ImGuiCol_WindowBg), 0.0f);
        else
            host_window->DrawList->AddRectFilled(node->Pos, node->Pos + node->Size, GetColorU32(ImGuiCol_WindowBg), 0.0f);
        host_window->DrawList->ChannelsMerge();
    }

    // Draw and populate Tab Bar
    if (host_window && node->Windows.Size > 0)
    {
        DockNodeUpdateTabBar(node, host_window);
        if (node->TabBar->SelectedTabId)
            node->SelectedTabID = node->TabBar->SelectedTabId;
    }
    else
    {
        node->WantCloseAll = false;
        node->WantCloseTabID = 0;
        node->IsFocused = false;
        if (node->Windows.Size > 0)
            node->SelectedTabID = node->Windows[0]->ID;
    }

    // Draw payload drop target
    if (host_window && node->IsVisible)
        if (node->IsRootNode() && (g.MovingWindow == NULL || g.MovingWindow->RootWindow != host_window))
            BeginAsDockableDragDropTarget(host_window);

    node->LastFrameActive = g.FrameCount;

    // Recurse into children
    if (host_window)
    {
        if (node->ChildNodes[0])
            DockNodeUpdate(node->ChildNodes[0]);
        if (node->ChildNodes[1])
            DockNodeUpdate(node->ChildNodes[1]);

        // End host window
        if (beginned_into_host_window)
            End();
    }
}

// Compare TabItem nodes given the last known DockOrder (will persist in .ini file as hint), used to sort tabs when multiple tabs are added on the same frame.
static int IMGUI_CDECL TabItemComparerByDockOrder(const void* lhs, const void* rhs)
{
    ImGuiWindow* a = ((const ImGuiTabItem*)lhs)->Window;
    ImGuiWindow* b = ((const ImGuiTabItem*)rhs)->Window;
    if (int d = ((a->DockOrder == -1) ? INT_MAX : a->DockOrder) - ((b->DockOrder == -1) ? INT_MAX : b->DockOrder))
        return d;
    return (a->BeginOrderWithinContext - b->BeginOrderWithinContext);
}

static void ImGui::DockNodeUpdateTabBar(ImGuiDockNode* node, ImGuiWindow* host_window)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;

    const bool node_was_active = (node->LastFrameActive + 1 == g.FrameCount);
    const bool closed_all = node->WantCloseAll && node_was_active;
    const ImGuiID closed_one = node->WantCloseTabID && node_was_active;
    node->WantCloseAll = false;
    node->WantCloseTabID = 0;

    // Decide if we should use a focused title bar color
    bool is_focused = false;
    ImGuiDockNode* root_node = DockNodeGetRootNode(node);
    if (g.NavWindowingTarget)
        is_focused = (g.NavWindowingTarget->DockNode == node);
    else if (g.NavWindow && g.NavWindow->RootWindowForTitleBarHighlight == host_window->RootWindow && root_node->LastFocusedNodeID == node->ID)
        is_focused = true;

    // Hidden tab bar will show a triangle on the upper-left (in Begin)
    if (node->IsHiddenTabBar)
    {
        node->VisibleWindow = (node->Windows.Size > 0) ? node->Windows[0] : NULL;
        node->IsFocused = is_focused;
        if (is_focused)
            node->LastFrameFocused = g.FrameCount;

        // Notify root of visible window (used to display title in OS task bar)
        if (node->VisibleWindow)
        {
            if (is_focused || root_node->VisibleWindow == NULL)
                root_node->VisibleWindow = node->VisibleWindow;
            if (node->TabBar)
                node->TabBar->VisibleTabId = node->VisibleWindow->ID;
        }
        return;
    }

    // Move ourselves to the Menu layer (so we can be accessed by tapping Alt) + undo SkipItems flag in order to draw over the title bar even if the window is collapsed
    bool backup_skip_item = host_window->SkipItems;
    if (!node->IsDockSpace)
    {
        host_window->SkipItems = false;
        host_window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
        host_window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Menu);
    }

    PushID(node->ID);
    ImGuiTabBar* tab_bar = node->TabBar;
    bool tab_bar_is_recreated = (tab_bar == NULL); // Tab bar are automatically destroyed when a node gets hidden
    if (tab_bar == NULL)
        tab_bar = node->TabBar = IM_NEW(ImGuiTabBar)();

    ImGuiID focus_tab_id = 0;

    // Collapse button changes shape and display a list
    if (IsPopupOpen("#TabListMenu"))
    {
        // Try to position the menu so it is more likely to stays within the same viewport
        SetNextWindowPos(ImVec2(node->Pos.x, node->Pos.y + GetFrameHeight()));
        if (BeginPopup("#TabListMenu"))
        {
            is_focused = true;
            if (tab_bar->Tabs.Size == 1)
            {
                if (MenuItem("Hide tab bar", NULL, node->IsHiddenTabBar))
                    node->WantHiddenTabBarToggle = true;
            }
            else
            {
                for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
                {
                    ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
                    IM_ASSERT(tab->Window != NULL);
                    if (Selectable(tab->Window->Name, tab->ID == tab_bar->SelectedTabId))
                        focus_tab_id = tab_bar->NextSelectedTabId = tab->ID;
                    SameLine();
                    Text("   ");
                }
            }
            EndPopup();
        }
    }

    // Title bar
    node->IsFocused = is_focused;
    if (is_focused)
        node->LastFrameFocused = g.FrameCount;
    ImRect title_bar_rect = ImRect(node->Pos, node->Pos + ImVec2(node->Size.x, g.FontSize + style.FramePadding.y * 2.0f));
    ImU32 title_bar_col = GetColorU32(host_window->Collapsed ? ImGuiCol_TitleBgCollapsed : is_focused ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
    host_window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, host_window->WindowRounding, ImDrawCornerFlags_Top);
    host_window->DrawList->AddLine(title_bar_rect.GetBL(), title_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border), style.WindowBorderSize);

    // Collapse button
    if (CollapseButton(host_window->GetID("#COLLAPSE"), title_bar_rect.Min, node))
        OpenPopup("#TabListMenu");
    if (IsItemActive())
        focus_tab_id = tab_bar->SelectedTabId;

    // Submit new tabs and apply NavWindow focus back to the tab bar. They will be added as Unsorted and sorted below based on relative DockOrder value.
    const int tabs_count_old = tab_bar->Tabs.Size;
    for (int window_n = 0; window_n < node->Windows.Size; window_n++)
    {
        ImGuiWindow* window = node->Windows[window_n];
        if (g.NavWindow && g.NavWindow->RootWindowDockStop == window)
            tab_bar->SelectedTabId = window->ID;
        if (TabBarFindTabByID(tab_bar, window->ID) == NULL)
            TabBarAddTab(tab_bar, ImGuiTabItemFlags_Unsorted, window);
    }

    // If multiple tabs are appearing on the same frame, sort them based on their persistent DockOrder value
    int tabs_unsorted_start = tab_bar->Tabs.Size;
    for (int tab_n = tab_bar->Tabs.Size - 1; tab_n >= 0 && (tab_bar->Tabs[tab_n].Flags & ImGuiTabItemFlags_Unsorted); tab_n--)
    {
        tab_bar->Tabs[tab_n].Flags &= ~ImGuiTabItemFlags_Unsorted;
        tabs_unsorted_start = tab_n;
    }
    //printf("[%05d] Sorting %d new appearing tabs\n", g.FrameCount, tab_bar->Tabs.Size - tabs_unsorted_start);
    if (tab_bar->Tabs.Size > tabs_unsorted_start + 1)
        ImQsort(tab_bar->Tabs.Data + tabs_unsorted_start, tab_bar->Tabs.Size - tabs_unsorted_start, sizeof(ImGuiTabItem), TabItemComparerByDockOrder);

    // Selected newly added tabs, or persistent tab ID if the tab bar was just recreated
    if (tab_bar_is_recreated && TabBarFindTabByID(tab_bar, node->SelectedTabID) != NULL)
        tab_bar->SelectedTabId = tab_bar->NextSelectedTabId = node->SelectedTabID;
    else if (tab_bar->Tabs.Size > tabs_count_old)
        tab_bar->SelectedTabId = tab_bar->NextSelectedTabId = tab_bar->Tabs.back().Window->ID;

    // Begin tab bar
    const ImRect tab_bar_rect = DockNodeCalcTabBarRect(node);
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_NoTabListPopupButton;// | ImGuiTabBarFlags_NoTabListScrollingButtons);
    tab_bar_flags |= ImGuiTabBarFlags_SaveSettings;
    tab_bar_flags |= ImGuiTabBarFlags_DockNode | (node->IsDockSpace ? ImGuiTabBarFlags_DockNodeIsDockSpace : 0);
    if (!host_window->Collapsed && is_focused)
        tab_bar_flags |= ImGuiTabBarFlags_IsFocused;
    BeginTabBarEx(tab_bar, tab_bar_rect, tab_bar_flags, node);
    //host_window->DrawList->AddRect(tab_bar_rect.Min, tab_bar_rect.Max, IM_COL32(255,0,255,255));

    // Submit actual tabs
    node->VisibleWindow = NULL;
    for (int window_n = 0; window_n < node->Windows.Size; window_n++)
    {
        ImGuiWindow* window = node->Windows[window_n];
        if ((closed_all || closed_one == window->ID) && window->HasCloseButton && !(window->Flags & ImGuiWindowFlags_UnsavedDocument))
            continue;
        if (window->LastFrameActive + 1 >= g.FrameCount || !node_was_active)
        {
            ImGuiTabItemFlags tab_item_flags = ImGuiTabItemFlags_DockedWindow;
            if (window->Flags & ImGuiWindowFlags_UnsavedDocument)
                tab_item_flags |= ImGuiTabItemFlags_UnsavedDocument;
            if (tab_bar->Flags & ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)
                tab_item_flags |= ImGuiTabItemFlags_NoCloseWithMiddleMouseButton;

            bool tab_open = true;
            TabItemEx(tab_bar, window->Name, window->HasCloseButton ? &tab_open : NULL, tab_item_flags, window);
            if (!tab_open)
                node->WantCloseTabID = window->ID;
            if (tab_bar->VisibleTabId == window->ID)
                node->VisibleWindow = window;

            // Store last item data so it can be queried with IsItemXXX functions after the user Begin() call
            window->DockTabItemStatusFlags = host_window->DC.LastItemStatusFlags;
            window->DockTabItemRect = host_window->DC.LastItemRect;

            // Update navigation ID on menu layer
            if (g.NavWindow && g.NavWindow->RootWindowDockStop == window && (window->DC.NavLayerActiveMask & (1 << 1)) == 0)
                host_window->NavLastIds[1] = window->ID;
        }
    }

    // Notify root of visible window (used to display title in OS task bar)
    if (node->VisibleWindow)
        if (is_focused || root_node->VisibleWindow == NULL)
            root_node->VisibleWindow = node->VisibleWindow;

    // Close button (after VisibleWindow was updated)
    // Note that VisibleWindow may have been overrided by CTRL+Tabbing, so VisibleWindow->ID may be != from tab_bar->SelectedTabId
    if (node->VisibleWindow)
    {
        if (!node->VisibleWindow->HasCloseButton)
        {
            PushItemFlag(ImGuiItemFlags_Disabled, true);
            PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_Text] * ImVec4(1.0f,1.0f,1.0f,0.5f));
        }
        const float rad = g.FontSize * 0.5f;
        if (CloseButton(host_window->GetID("#CLOSE"), title_bar_rect.GetTR() + ImVec2(-style.FramePadding.x - rad, style.FramePadding.y + rad), rad + 1))
            if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, tab_bar->VisibleTabId))
            {
                node->WantCloseTabID = tab->ID;
                TabBarCloseTab(tab_bar, tab);
            }
        //if (IsItemActive())
        //    focus_tab_id = tab_bar->SelectedTabId;
        if (!node->VisibleWindow->HasCloseButton)
        {
            PopStyleColor();
            PopItemFlag();
        }
    }

    // When clicking on the title bar outside of tabs, we still focus the selected tab for that node
    if (g.HoveredWindow == host_window && g.HoveredId == 0 && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max))
    {
        if (IsMouseClicked(0))
        {
            focus_tab_id = tab_bar->SelectedTabId;
            if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, focus_tab_id))
                StartMouseMovingWindow(tab->Window);
        }
    }

    // Forward focus from host node to selected window
    //if (is_focused && g.NavWindow == host_window && !g.NavWindowingTarget)
    //    focus_tab_id = tab_bar->SelectedTabId;

    // When clicked on a tab we requested focus to the docked child
    // This overrides the value set by "forward focus from host node to selected window".
    if (tab_bar->NextSelectedTabId)
        focus_tab_id = tab_bar->NextSelectedTabId;

    // Apply navigation focus
    if (focus_tab_id != 0)
        if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, focus_tab_id))
        {
            FocusWindow(tab->Window);
            NavInitWindow(tab->Window, false);
        }

    EndTabBar();
    PopID();

    // Restore SkipItems flag
    if (!node->IsDockSpace)
    {
        host_window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
        host_window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Main);
        host_window->SkipItems = backup_skip_item;
    }
}

static bool DockNodeIsDropAllowedOne(ImGuiWindow* payload, ImGuiWindow* host_window)
{
    if (host_window->DockNodeAsHost && host_window->DockNodeAsHost->IsDockSpace && payload->BeginOrderWithinContext < host_window->BeginOrderWithinContext)
        return false;

    ImGuiWindowClass* host_class = host_window->DockNodeAsHost ? &host_window->DockNodeAsHost->WindowClass : &host_window->WindowClass;
    ImGuiWindowClass* payload_class = &payload->WindowClass;
    if (host_class->ClassId != payload_class->ClassId)
    {
        if (host_class->ClassId != 0 && host_class->DockingAllowUnclassed && payload_class->ClassId == 0)
            return true;
        if (payload_class->ClassId != 0 && payload_class->DockingAllowUnclassed && host_class->ClassId == 0)
            return true;
        return false;
    }

    return true;
}

static bool ImGui::DockNodeIsDropAllowed(ImGuiWindow* host_window, ImGuiWindow* root_payload)
{
    if (root_payload->DockNodeAsHost && root_payload->DockNodeAsHost->IsSplitNode())
        return true;

    const int payload_count = root_payload->DockNodeAsHost ? root_payload->DockNodeAsHost->Windows.Size : 1;
    for (int payload_n = 0; payload_n < payload_count; payload_n++)
    {
        ImGuiWindow* payload = root_payload->DockNodeAsHost ? root_payload->DockNodeAsHost->Windows[payload_n] : root_payload;
        if (DockNodeIsDropAllowedOne(payload, host_window))
            return true;
    }
    return false;
}

static ImRect ImGui::DockNodeCalcTabBarRect(const ImGuiDockNode* node)
{
    ImGuiContext& g = *GImGui;
    ImRect r = ImRect(node->Pos.x, node->Pos.y, node->Pos.x + node->Size.x, node->Pos.y + (g.FontSize + g.Style.FramePadding.y * 2.0f));
    if (node->HasCollapseButton)
        r.Min.x += g.Style.FramePadding.x + g.FontSize; // + g.Style.ItemInnerSpacing.x; // <-- Adding ItemInnerSpacing makes the title text moves slightly when in a tab bar. Instead we adjusted RenderArrowDockMenu()
    if (node->HasCloseButton)
        r.Max.x -= g.Style.FramePadding.x + g.FontSize + 1.0f;
    return r;
}

void ImGui::DockNodeCalcSplitRects(ImVec2& pos_old, ImVec2& size_old, ImVec2& pos_new, ImVec2& size_new, ImGuiDir dir, ImVec2 size_new_desired)
{
    ImGuiContext& g = *GImGui;
    const float dock_spacing = g.Style.ItemInnerSpacing.x;
    const ImGuiAxis axis = (dir == ImGuiDir_Left || dir == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;
    pos_new[axis ^ 1] = pos_old[axis ^ 1];
    size_new[axis ^ 1] = size_old[axis ^ 1];

    // Distribute size on given axis (with a desired size or equally)
    const float w_avail = size_old[axis] - dock_spacing;
    if (size_new_desired[axis] > 0.0f && size_new_desired[axis] <= w_avail * 0.5f)
    {
        size_new[axis] = size_new_desired[axis];
        size_old[axis] = (float)(int)(w_avail - size_new[axis]);
    }
    else
    {
        size_new[axis] = (float)(int)(w_avail * 0.5f);
        size_old[axis] = (float)(int)(w_avail - size_new[axis]);
    }

    // Position each node
    if (dir == ImGuiDir_Right || dir == ImGuiDir_Down)
    {
        pos_new[axis] = pos_old[axis] + size_old[axis] + dock_spacing;
    }
    else if (dir == ImGuiDir_Left || dir == ImGuiDir_Up)
    {
        pos_new[axis] = pos_old[axis];
        pos_old[axis] = pos_new[axis] + size_new[axis] + dock_spacing;
    }
}

// Retrieve the drop rectangles for a given direction or for the center + perform hit testing.
bool ImGui::DockNodeCalcDropRects(const ImRect& parent, ImGuiDir dir, ImRect& out_r, bool outer_docking)
{
    ImGuiContext& g = *GImGui;

    const float parent_smaller_axis = ImMin(parent.GetWidth(), parent.GetHeight());
    const float hs_for_central_nodes = ImMin(g.FontSize * 1.5f, ImMax(g.FontSize * 0.5f, parent_smaller_axis / 8.0f));
    float hs_w; // Half-size, longer axis
    float hs_h; // Half-size, smaller axis
    ImVec2 off; // Distance from edge or center
    if (outer_docking)
    {
        //hs_w = ImFloor(ImClamp(parent_smaller_axis - hs_for_central_nodes * 4.0f, g.FontSize * 0.5f, g.FontSize * 8.0f));
        //hs_h = ImFloor(hs_w * 0.15f);
        //off = ImVec2(ImFloor(parent.GetWidth() * 0.5f - GetFrameHeightWithSpacing() * 1.4f - hs_h), ImFloor(parent.GetHeight() * 0.5f - GetFrameHeightWithSpacing() * 1.4f - hs_h));
        hs_w = ImFloor(hs_for_central_nodes * 1.50f);
        hs_h = ImFloor(hs_for_central_nodes * 0.80f);
        off = ImVec2(ImFloor(parent.GetWidth() * 0.5f - GetFrameHeightWithSpacing() * 0.0f - hs_h), ImFloor(parent.GetHeight() * 0.5f - GetFrameHeightWithSpacing() * 0.0f - hs_h));
    }
    else
    {
        hs_w = ImFloor(hs_for_central_nodes);
        hs_h = ImFloor(hs_for_central_nodes * 0.90f);
        off = ImVec2(ImFloor(hs_w * 2.40f), ImFloor(hs_w * 2.40f));
    }

    ImVec2 c = ImFloor(parent.GetCenter());
    if      (dir == ImGuiDir_None)  { out_r = ImRect(c.x - hs_w, c.y - hs_w,         c.x + hs_w, c.y + hs_w);         }
    else if (dir == ImGuiDir_Up)    { out_r = ImRect(c.x - hs_w, c.y - off.y - hs_h, c.x + hs_w, c.y - off.y + hs_h); }
    else if (dir == ImGuiDir_Down)  { out_r = ImRect(c.x - hs_w, c.y + off.y - hs_h, c.x + hs_w, c.y + off.y + hs_h); }
    else if (dir == ImGuiDir_Left)  { out_r = ImRect(c.x - off.x - hs_h, c.y - hs_w, c.x - off.x + hs_h, c.y + hs_w); }
    else if (dir == ImGuiDir_Right) { out_r = ImRect(c.x + off.x - hs_h, c.y - hs_w, c.x + off.x + hs_h, c.y + hs_w); }

    ImRect hit_r = out_r;
    if (!outer_docking)
    {
        // Custom hit testing for the 5-way selection, designed to reduce flickering when moving diagonally between sides
        hit_r.Expand(ImFloor(hs_w * 0.30f));
        ImVec2 mouse_delta = (g.IO.MousePos - c);
        float mouse_delta_len2 = ImLengthSqr(mouse_delta);
        float r_threshold_center = hs_w * 1.4f;
        float r_threshold_sides = hs_w * (1.4f + 1.2f);
        if (mouse_delta_len2 < r_threshold_center * r_threshold_center)
            return (dir == ImGuiDir_None);
        if (mouse_delta_len2 < r_threshold_sides * r_threshold_sides)
            return (dir == ImGetDirQuadrantFromDelta(mouse_delta.x, mouse_delta.y));
    }
    return hit_r.Contains(g.IO.MousePos);
}

// host_node may be NULL if the window doesn't have a DockNode already.
static bool ImGui::DockNodePreviewDockCalc(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* root_payload, ImGuiDockPreviewData* data, bool is_explicit_target, bool is_outer_docking)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentWindow == host_window);   // Because we rely on font size to calculate tab sizes

    // There is an edge case when docking into a dockspace which only has inactive nodes.
    // In this case DockNodeTreeFindNodeByPos() will have selected a leaf node which is inactive. 
    // Because the inactive leaf node doesn't have proper pos/size yet, we'll use the root node as reference.
    ImGuiDockNode* ref_node_for_rect = (host_node && !host_node->IsVisible) ? DockNodeGetRootNode(host_node) : host_node;
    if (ref_node_for_rect)
        IM_ASSERT(ref_node_for_rect->IsVisible);

    // Build a tentative future node (reuse same structure because it is practical)
    data->FutureNode.HasCloseButton = (host_node ? host_node->HasCloseButton : host_window->HasCloseButton) || (root_payload->HasCloseButton);
    data->FutureNode.HasCollapseButton = host_node ? true : ((host_window->Flags & ImGuiWindowFlags_NoCollapse) == 0);
    data->FutureNode.Pos = host_node ? ref_node_for_rect->Pos : host_window->Pos;
    data->FutureNode.Size = host_node ? ref_node_for_rect->Size : host_window->Size;

    // Figure out here we are allowed to dock
    const bool src_is_visibly_splitted = root_payload->DockNodeAsHost && root_payload->DockNodeAsHost->IsSplitNode() && (root_payload->DockNodeAsHost->OnlyNodeWithWindows == NULL);
    data->IsCenterAvailable = !is_outer_docking;
    if (src_is_visibly_splitted && (!host_node || !host_node->IsEmpty()))
        data->IsCenterAvailable = false;
    if (host_node && (host_node->Flags & ImGuiDockNodeFlags_NoDockingInCentralNode) && host_node->IsCentralNode)
        data->IsCenterAvailable = false;

    data->IsSidesAvailable = true;
    if ((host_node && (host_node->Flags & ImGuiDockNodeFlags_NoSplit)) || g.IO.ConfigDockingNoSplit)
        data->IsSidesAvailable = false;
    if (!is_outer_docking && host_node && host_node->ParentNode == NULL && host_node->IsCentralNode)
        data->IsSidesAvailable = false;

    // Calculate drop shapes geometry for allowed splitting directions
    IM_ASSERT(ImGuiDir_None == -1);
    data->SplitNode = host_node;
    data->SplitDir = ImGuiDir_None;
    data->IsSplitDirExplicit = false;
    if (!host_window->Collapsed)
        for (int dir = ImGuiDir_None; dir < ImGuiDir_COUNT; dir++)
        {
            if (dir == ImGuiDir_None && !data->IsCenterAvailable)
                continue;
            if (dir != ImGuiDir_None && !data->IsSidesAvailable)
                continue;
            if (DockNodeCalcDropRects(data->FutureNode.Rect(), (ImGuiDir)dir, data->DropRectsDraw[dir+1], is_outer_docking))
            {
                data->SplitDir = (ImGuiDir)dir;
                data->IsSplitDirExplicit = true;
            }
        }

    // When docking without holding Shift, we only allow and preview docking when hovering over a drop rect or over the title bar
    data->IsDropAllowed = (data->SplitDir != ImGuiDir_None) || (data->IsCenterAvailable);
    if (!is_explicit_target && !data->IsSplitDirExplicit && !g.IO.ConfigDockingWithShift)
        data->IsDropAllowed = false;

    // Calculate split area
    data->SplitRatio = 0.0f;
    if (data->SplitDir != ImGuiDir_None)
    {
        ImGuiDir split_dir = data->SplitDir;
        ImGuiAxis split_axis = (split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;
        ImVec2 pos_new, pos_old = data->FutureNode.Pos;
        ImVec2 size_new, size_old = data->FutureNode.Size;
        DockNodeCalcSplitRects(pos_old, size_old, pos_new, size_new, split_dir, root_payload->Size);

        // Calculate split ratio so we can pass it down the docking request
        float split_ratio = ImSaturate(size_new[split_axis] / data->FutureNode.Size[split_axis]);
        data->FutureNode.Pos = pos_new;
        data->FutureNode.Size = size_new;
        data->SplitRatio = (split_dir == ImGuiDir_Right || split_dir == ImGuiDir_Down) ? (1.0f - split_ratio) : (split_ratio);
    }

    return data->IsSplitDirExplicit;
}

static void ImGui::DockNodePreviewDockRender(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* root_payload, const ImGuiDockPreviewData* data)
{
    ImGuiContext& g = *GImGui;

    // With this option, we only display the preview on the target viewport, and the payload viewport is made transparent.
    // To compensate for the single layer obstructed by the payload, we'll increase the alpha of the preview nodes.
    const bool is_transparent_payload = g.IO.ConfigDockingTransparentPayload;

    // In case the two windows involved are on different viewports, we will draw the overlay on each of them.
    int overlay_draw_lists_count = 0;
    ImDrawList* overlay_draw_lists[2];
    overlay_draw_lists[overlay_draw_lists_count++] = GetOverlayDrawList(host_window->Viewport);
    if (host_window->Viewport != root_payload->Viewport && !is_transparent_payload)
        overlay_draw_lists[overlay_draw_lists_count++] = GetOverlayDrawList(root_payload->Viewport);

    // Draw main preview rectangle
    const ImU32 overlay_col_tabs = GetColorU32(ImGuiCol_TabActive);
    const ImU32 overlay_col_main = GetColorU32(ImGuiCol_DockingPreview, is_transparent_payload ? 0.60f : 0.40f);
    const ImU32 overlay_col_drop = GetColorU32(ImGuiCol_DockingPreview, is_transparent_payload ? 0.90f : 0.70f);
    const ImU32 overlay_col_drop_hovered = GetColorU32(ImGuiCol_DockingPreview, is_transparent_payload ? 1.20f : 1.00f);
    const ImU32 overlay_col_lines = GetColorU32(ImGuiCol_NavWindowingHighlight, is_transparent_payload ? 0.80f : 0.60f);

    // Display area preview
    const bool can_preview_tabs = (root_payload->DockNodeAsHost == NULL || root_payload->DockNodeAsHost->Windows.Size > 0);
    if (data->IsDropAllowed)
    {
        ImRect overlay_rect = data->FutureNode.Rect();
        if (data->SplitDir == ImGuiDir_None && can_preview_tabs)
            overlay_rect.Min.y += GetFrameHeight();
        if (data->SplitDir != ImGuiDir_None || data->IsCenterAvailable)
            for (int overlay_n = 0; overlay_n < overlay_draw_lists_count; overlay_n++)
                overlay_draw_lists[overlay_n]->AddRectFilled(overlay_rect.Min, overlay_rect.Max, overlay_col_main, host_window->WindowRounding);
    }

    // Display tab shape/label preview unless we are splitting node (it generally makes the situation harder to read)
    if (data->IsDropAllowed && can_preview_tabs && data->SplitDir == ImGuiDir_None && data->IsCenterAvailable)
    {
        // Compute target tab bar geometry so we can locate our preview tabs
        ImRect tab_bar_rect = DockNodeCalcTabBarRect(&data->FutureNode);
        ImVec2 tab_pos = tab_bar_rect.Min;
        if (host_node && host_node->TabBar)
        {
            if (!host_node->IsHiddenTabBar)
                tab_pos.x += host_node->TabBar->OffsetMax + g.Style.ItemInnerSpacing.x; // We don't use OffsetNewTab because when using non-persistent-order tab bar it is incremented with each Tab submission.
            else
                tab_pos.x += g.Style.ItemInnerSpacing.x + TabItemCalcSize(host_node->Windows[0]->Name, host_node->Windows[0]->HasCloseButton).x;
        }
        else if (!(host_window->Flags & ImGuiWindowFlags_DockNodeHost))
        {
            tab_pos.x += g.Style.ItemInnerSpacing.x + TabItemCalcSize(host_window->Name, host_window->HasCloseButton).x; // Account for slight offset which will be added when changing from title bar to tab bar
        }

        // Draw tab shape/label preview (payload may be a loose window or a host window carrying multiple tabbed windows)
        if (root_payload->DockNodeAsHost)
            IM_ASSERT(root_payload->DockNodeAsHost->Windows.Size == root_payload->DockNodeAsHost->TabBar->Tabs.Size);
        const int payload_count = root_payload->DockNodeAsHost ? root_payload->DockNodeAsHost->TabBar->Tabs.Size : 1;
        for (int payload_n = 0; payload_n < payload_count; payload_n++)
        {
            // Calculate the tab bounding box for each payload window
            ImGuiWindow* payload = root_payload->DockNodeAsHost ? root_payload->DockNodeAsHost->TabBar->Tabs[payload_n].Window : root_payload;
            if (!DockNodeIsDropAllowedOne(payload, host_window))
                continue;

            ImVec2 tab_size = TabItemCalcSize(payload->Name, payload->HasCloseButton);
            ImRect tab_bb(tab_pos.x, tab_pos.y, tab_pos.x + tab_size.x, tab_pos.y + tab_size.y);
            tab_pos.x += tab_size.x + g.Style.ItemInnerSpacing.x;
            for (int overlay_n = 0; overlay_n < overlay_draw_lists_count; overlay_n++)
            {
                ImGuiTabItemFlags tab_flags = ImGuiTabItemFlags_Preview | ((payload->Flags & ImGuiWindowFlags_UnsavedDocument) ? ImGuiTabItemFlags_UnsavedDocument : 0);
                if (!tab_bar_rect.Contains(tab_bb))
                    overlay_draw_lists[overlay_n]->PushClipRect(tab_bar_rect.Min, tab_bar_rect.Max);
                TabItemBackground(overlay_draw_lists[overlay_n], tab_bb, tab_flags, overlay_col_tabs);
                TabItemLabelAndCloseButton(overlay_draw_lists[overlay_n], tab_bb, tab_flags, payload->Name, 0, 0);
                if (!tab_bar_rect.Contains(tab_bb))
                    overlay_draw_lists[overlay_n]->PopClipRect();
            }
        }
    }

    // Display drop boxes
    const float overlay_rounding = ImMax(3.0f, g.Style.FrameRounding);
    for (int dir = ImGuiDir_None; dir < ImGuiDir_COUNT; dir++)
    {
        if (!data->DropRectsDraw[dir + 1].IsInverted())
        {
            ImRect draw_r = data->DropRectsDraw[dir + 1];
            ImRect draw_r_in = draw_r;
            draw_r_in.Expand(-2.0f);
            ImU32 overlay_col = (data->SplitDir == (ImGuiDir)dir && data->IsSplitDirExplicit) ? overlay_col_drop_hovered : overlay_col_drop;
            for (int overlay_n = 0; overlay_n < overlay_draw_lists_count; overlay_n++)
            {
                ImVec2 center = ImFloor(draw_r_in.GetCenter());
                overlay_draw_lists[overlay_n]->AddRectFilled(draw_r.Min, draw_r.Max, overlay_col, overlay_rounding);
                overlay_draw_lists[overlay_n]->AddRect(draw_r_in.Min, draw_r_in.Max, overlay_col_lines, overlay_rounding);
                if (dir == ImGuiDir_Left || dir == ImGuiDir_Right)
                    overlay_draw_lists[overlay_n]->AddLine(ImVec2(center.x, draw_r_in.Min.y), ImVec2(center.x, draw_r_in.Max.y), overlay_col_lines);
                if (dir == ImGuiDir_Up || dir == ImGuiDir_Down)
                    overlay_draw_lists[overlay_n]->AddLine(ImVec2(draw_r_in.Min.x, center.y), ImVec2(draw_r_in.Max.x, center.y), overlay_col_lines);
            }
        }
        
        // Stop after ImGuiDir_None
        if ((host_node && (host_node->Flags & ImGuiDockNodeFlags_NoSplit)) || g.IO.ConfigDockingNoSplit)
            return;
    }
}

//-----------------------------------------------------------------------------
// Docking: ImGuiDockNode Tree manipulation functions
//-----------------------------------------------------------------------------

void ImGui::DockNodeTreeSplit(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiAxis split_axis, int split_inheritor_child_idx, float split_ratio, ImGuiDockNode* new_node)
{
    IM_ASSERT(split_axis != ImGuiAxis_None);

    ImGuiDockNode* child_0 = (new_node && split_inheritor_child_idx != 0) ? new_node : DockContextAddNode(ctx, 0);
    child_0->ParentNode = parent_node;

    ImGuiDockNode* child_1 = (new_node && split_inheritor_child_idx != 1) ? new_node : DockContextAddNode(ctx, 0);
    child_1->ParentNode = parent_node;

    ImGuiDockNode* child_inheritor = (split_inheritor_child_idx == 0) ? child_0 : child_1;
    DockNodeMoveChildNodes(child_inheritor, parent_node);
    parent_node->ChildNodes[0] = child_0;
    parent_node->ChildNodes[1] = child_1;
    parent_node->ChildNodes[split_inheritor_child_idx]->VisibleWindow = parent_node->VisibleWindow;
    parent_node->SplitAxis = split_axis;
    parent_node->VisibleWindow = NULL;

    float size_avail = (parent_node->Size[split_axis] - IMGUI_DOCK_SPLITTER_SIZE);
    IM_ASSERT(size_avail > 0.0f);
    child_0->SizeRef = child_1->SizeRef = parent_node->Size;
    child_0->SizeRef[split_axis] = ImFloor(size_avail * split_ratio);
    child_1->SizeRef[split_axis] = ImFloor(size_avail - child_0->SizeRef[split_axis]);

    DockNodeMoveWindows(parent_node->ChildNodes[split_inheritor_child_idx], parent_node);
    DockNodeTreeUpdatePosSize(parent_node, parent_node->Pos, parent_node->Size);
}

void ImGui::DockNodeTreeMerge(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiDockNode* merge_lead_child)
{
    // When called from DockContextProcessUndockNode() it is possible that one of the child is NULL.
    ImGuiDockNode* child_0 = parent_node->ChildNodes[0];
    ImGuiDockNode* child_1 = parent_node->ChildNodes[1];
    IM_ASSERT(child_0 || child_1);
    IM_ASSERT(merge_lead_child == child_0 || merge_lead_child == child_1);
    if ((child_0 && child_0->Windows.Size > 0) || (child_1 && child_1->Windows.Size > 0))
    {
        IM_ASSERT(parent_node->TabBar == NULL);
        IM_ASSERT(parent_node->Windows.Size == 0);
    }

    ImVec2 backup_last_explicit_size = parent_node->SizeRef;
    DockNodeMoveChildNodes(parent_node, merge_lead_child);
    if (child_0)
    {
        DockNodeMoveWindows(parent_node, child_0); // Generally only 1 of the 2 child node will have windows
        DockSettingsMoveDockReferencesInInactiveWindow(child_0->ID, parent_node->ID);
    }
    if (child_1)
    {
        DockNodeMoveWindows(parent_node, child_1);
        DockSettingsMoveDockReferencesInInactiveWindow(child_1->ID, parent_node->ID);
    }
    DockNodeApplyPosSizeToWindows(parent_node);
    parent_node->InitFromFirstWindowPosSize = parent_node->InitFromFirstWindowViewport = false;
    parent_node->VisibleWindow = merge_lead_child->VisibleWindow;
    parent_node->IsCentralNode = (child_0 && child_0->IsCentralNode) || (child_1 && child_1->IsCentralNode);
    parent_node->IsHiddenTabBar = merge_lead_child->IsHiddenTabBar;
    parent_node->SizeRef = backup_last_explicit_size;

    if (child_0)
    {
        ctx->DockContext->Nodes.SetVoidPtr(child_0->ID, NULL);
        IM_DELETE(child_0);
    }
    if (child_1)
    {
        ctx->DockContext->Nodes.SetVoidPtr(child_1->ID, NULL);
        IM_DELETE(child_1);
    }
}

// Update Pos/Size for a node hierarchy (don't affect child Windows yet)
void ImGui::DockNodeTreeUpdatePosSize(ImGuiDockNode* node, ImVec2 pos, ImVec2 size)
{
    node->Pos = pos;
    node->Size = size;
    if (node->IsLeafNode())
        return;

    ImGuiDockNode* child_0 = node->ChildNodes[0];
    ImGuiDockNode* child_1 = node->ChildNodes[1];
    ImVec2 child_0_pos = pos, child_1_pos = pos;
    ImVec2 child_0_size = size, child_1_size = size;
    if (child_0->IsVisible && child_1->IsVisible)
    {
        const float spacing = IMGUI_DOCK_SPLITTER_SIZE;
        const ImGuiAxis axis = (ImGuiAxis)node->SplitAxis;
        const float size_avail = ImMax(size[axis] - spacing, 0.0f);

        // Size allocation policy
        // 1) The first 0..WindowMinSize[axis]*2 are allocated evenly to both windows.
        ImGuiContext& g = *GImGui;
        const float size_min_each = ImFloor(ImMin(size_avail, g.Style.WindowMinSize[axis] * 2.0f) * 0.5f);

        // 2) Process locked absolute size (during a splitter resize we preserve the child of nodes not touching the splitter edge)
        IM_ASSERT(!(child_0->WantLockSizeOnce && child_1->WantLockSizeOnce));
        if (child_0->WantLockSizeOnce)
        {
            child_0->WantLockSizeOnce = false;
            child_0_size[axis] = child_0->SizeRef[axis] = child_0->Size[axis];
            child_1_size[axis] = child_1->SizeRef[axis] = (size_avail - child_0_size[axis]);

        }
        else if (child_1->WantLockSizeOnce)
        {
            child_1->WantLockSizeOnce = false;
            child_1_size[axis] = child_1->SizeRef[axis] = child_1->Size[axis];
            child_0_size[axis] = child_0->SizeRef[axis] = (size_avail - child_1_size[axis]);
        }

        // 3) If one window is the central node (~ use remaining space, should be made explicit!), use explicit size from the other, and remainder for the central node
        else if (child_1->IsCentralNode && child_0->SizeRef[axis] != 0.0f)
        {
            child_0_size[axis] = ImMin(size_avail - size_min_each, child_0->SizeRef[axis]);
            child_1_size[axis] = (size_avail - child_0_size[axis]);
        }
        else if (child_0->IsCentralNode && child_1->SizeRef[axis] != 0.0f)
        {
            child_1_size[axis] = ImMin(size_avail - size_min_each, child_1->SizeRef[axis]);
            child_0_size[axis] = (size_avail - child_1_size[axis]);
        }
        else
        {
            // 4) Otherwise distribute according to the relative ratio of each SizeRef value
            float split_ratio = child_0->SizeRef[axis] / (child_0->SizeRef[axis] + child_1->SizeRef[axis]);
            child_0_size[axis] = ImMax(size_min_each, ImFloor(size_avail * split_ratio + 0.5F));
            child_1_size[axis] = (size_avail - child_0_size[axis]);
        }
        child_1_pos[axis] += spacing + child_0_size[axis];
    }
    if (child_0->IsVisible)
        DockNodeTreeUpdatePosSize(child_0, child_0_pos, child_0_size);
    if (child_1->IsVisible)
        DockNodeTreeUpdatePosSize(child_1, child_1_pos, child_1_size);
}

static void DockNodeTreeUpdateSplitterFindTouchingNode(ImGuiDockNode* node, ImGuiAxis axis, int side, ImVector<ImGuiDockNode*>* touching_nodes)
{
    if (node->IsLeafNode())
    {
        touching_nodes->push_back(node);
        return;
    }
    if (node->ChildNodes[0]->IsVisible)
        if (node->SplitAxis != axis || side == 0 || !node->ChildNodes[1]->IsVisible)
            DockNodeTreeUpdateSplitterFindTouchingNode(node->ChildNodes[0], axis, side, touching_nodes);
    if (node->ChildNodes[1]->IsVisible)
        if (node->SplitAxis != axis || side == 1 || !node->ChildNodes[0]->IsVisible)
            DockNodeTreeUpdateSplitterFindTouchingNode(node->ChildNodes[1], axis, side, touching_nodes);
}

void ImGui::DockNodeTreeUpdateSplitter(ImGuiDockNode* node)
{
    if (node->IsLeafNode())
        return;

    ImGuiContext& g = *GImGui;

    ImGuiDockNode* child_0 = node->ChildNodes[0];
    ImGuiDockNode* child_1 = node->ChildNodes[1];
    if (child_0->IsVisible && child_1->IsVisible)
    {
        // Bounding box of the splitter cover the space between both nodes (w = Spacing, h = Size[xy^1] for when splitting horizontally)
        const ImGuiAxis axis = (ImGuiAxis)node->SplitAxis;
        IM_ASSERT(axis != ImGuiAxis_None);
        ImRect bb;
        bb.Min = child_0->Pos;
        bb.Max = child_1->Pos;
        bb.Min[axis] += child_0->Size[axis];
        bb.Max[axis ^ 1] += child_1->Size[axis ^ 1];
        //if (g.IO.KeyCtrl) GetOverlayDrawList(g.CurrentWindow->Viewport)->AddRect(bb.Min, bb.Max, IM_COL32(255,0,255,255));

        if (node->Flags & ImGuiDockNodeFlags_NoResize)
        {
            ImGuiWindow* window = g.CurrentWindow;
            window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_Separator), g.Style.FrameRounding);
        }
        else
        {
            //bb.Min[axis] += 1; // Display a little inward so highlight doesn't connect with nearby tabs on the neighbor node.
            //bb.Max[axis] -= 1;
            PushID(node->ID);

            // Gather list of nodes that are touching the splitter line. Find resizing limits based on those nodes.
            ImVector<ImGuiDockNode*> touching_nodes[2];
            float min_size = g.Style.WindowMinSize[axis];
            float resize_limits[2];
            resize_limits[0] = node->ChildNodes[0]->Pos[axis] + min_size;
            resize_limits[1] = node->ChildNodes[1]->Pos[axis] + node->ChildNodes[1]->Size[axis] - min_size;

            ImGuiID splitter_id = GetID("##Splitter");
            if (g.ActiveId == splitter_id)
            {
                // Only process when splitter is active
                DockNodeTreeUpdateSplitterFindTouchingNode(child_0, axis, 1, &touching_nodes[0]);
                DockNodeTreeUpdateSplitterFindTouchingNode(child_1, axis, 0, &touching_nodes[1]);
                for (int touching_node_n = 0; touching_node_n < touching_nodes[0].Size; touching_node_n++)
                    resize_limits[0] = ImMax(resize_limits[0], touching_nodes[0][touching_node_n]->Rect().Min[axis] + min_size);
                for (int touching_node_n = 0; touching_node_n < touching_nodes[1].Size; touching_node_n++)
                    resize_limits[1] = ImMin(resize_limits[1], touching_nodes[1][touching_node_n]->Rect().Max[axis] - min_size);

                /*
                // [DEBUG] Render limits
                ImDrawList* draw_list = node->HostWindow ? GetOverlayDrawList(node->HostWindow) : GetOverlayDrawList((ImGuiViewportP*)GetMainViewport());
                for (int n = 0; n < 2; n++)
                if (axis == ImGuiAxis_X)
                draw_list->AddLine(ImVec2(resize_limits[n], node->ChildNodes[n]->Pos.y), ImVec2(resize_limits[n], node->ChildNodes[n]->Pos.y + node->ChildNodes[n]->Size.y), IM_COL32(255, 0, 255, 255), 3.0f);
                else
                draw_list->AddLine(ImVec2(node->ChildNodes[n]->Pos.x, resize_limits[n]), ImVec2(node->ChildNodes[n]->Pos.x + node->ChildNodes[n]->Size.x, resize_limits[n]), IM_COL32(255, 0, 255, 255), 3.0f);
                */
            }

            // Use a short delay before highlighting the splitter (and changing the mouse cursor) in order for regular mouse movement to not highlight many splitters
            float cur_size_0 = child_0->Size[axis];
            float cur_size_1 = child_1->Size[axis];
            float min_size_0 = resize_limits[0] - child_0->Pos[axis];
            float min_size_1 = child_1->Pos[axis] + child_1->Size[axis] - resize_limits[1];
            if (SplitterBehavior(bb, GetID("##Splitter"), axis, &cur_size_0, &cur_size_1, min_size_0, min_size_1, WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS, WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER))
            {
                if (touching_nodes[0].Size > 0 && touching_nodes[1].Size > 0)
                {
                    child_0->Size[axis] = child_0->SizeRef[axis] = cur_size_0;
                    child_1->Pos[axis] -= cur_size_1 - child_1->Size[axis];
                    child_1->Size[axis] = child_1->SizeRef[axis] = cur_size_1;

                    // Lock the size of every node that is a sibling of the node we are touching
                    // This might be less desirable if we can merge sibling of a same axis into the same parental level.
#if 1
                    for (int side_n = 0; side_n < 2; side_n++)
                        for (int touching_node_n = 0; touching_node_n < touching_nodes[side_n].Size; touching_node_n++)
                        {
                            ImGuiDockNode* touching_node = touching_nodes[side_n][touching_node_n];
                            //ImDrawList* draw_list = node->HostWindow ? GetOverlayDrawList(node->HostWindow) : GetOverlayDrawList((ImGuiViewportP*)GetMainViewport());
                            //draw_list->AddRect(touching_node->Pos, touching_node->Pos + touching_node->Size, IM_COL32(255, 128, 0, 255));
                            while (touching_node->ParentNode != node)
                            {
                                if (touching_node->ParentNode->SplitAxis == axis)
                                {
                                    // Mark other node so its size will be preserved during the upcoming call to DockNodeTreeUpdatePosSize().
                                    ImGuiDockNode* node_to_preserve = touching_node->ParentNode->ChildNodes[side_n];
                                    node_to_preserve->WantLockSizeOnce = true;
                                    //draw_list->AddRect(touching_node->Pos, touching_node->Rect().Max, IM_COL32(255, 0, 0, 255));
                                    //draw_list->AddRectFilled(node_to_preserve->Pos, node_to_preserve->Rect().Max, IM_COL32(0, 255, 0, 100));
                                }
                                touching_node = touching_node->ParentNode;
                            }
                        }
#endif

                    DockNodeTreeUpdatePosSize(child_0, child_0->Pos, child_0->Size);
                    DockNodeTreeUpdatePosSize(child_1, child_1->Pos, child_1->Size);
                    MarkIniSettingsDirty();
                }
            }
            PopID();
        }
    }

    if (child_0->IsVisible)
        DockNodeTreeUpdateSplitter(child_0);
    if (child_1->IsVisible)
        DockNodeTreeUpdateSplitter(child_1);
}

ImGuiDockNode* ImGui::DockNodeTreeFindFallbackLeafNode(ImGuiDockNode* node)
{
    if (node->IsLeafNode())
        return node;
    if (ImGuiDockNode* leaf_node = DockNodeTreeFindFallbackLeafNode(node->ChildNodes[0]))
        return leaf_node;
    if (ImGuiDockNode* leaf_node = DockNodeTreeFindFallbackLeafNode(node->ChildNodes[1]))
        return leaf_node;
    return NULL;
}

ImGuiDockNode* ImGui::DockNodeTreeFindNodeByPos(ImGuiDockNode* node, ImVec2 pos)
{
    if (!node->IsVisible)
        return NULL;

    ImGuiContext& g = *GImGui;
    const float dock_spacing = g.Style.ItemInnerSpacing.x;
    ImRect r(node->Pos, node->Pos + node->Size);
    r.Expand(dock_spacing * 0.5f);
    bool inside = r.Contains(pos);
    if (!inside)
        return NULL;

    if (node->IsLeafNode())
        return node;
    if (ImGuiDockNode* hovered_node = DockNodeTreeFindNodeByPos(node->ChildNodes[0], pos))
        return hovered_node;
    if (ImGuiDockNode* hovered_node = DockNodeTreeFindNodeByPos(node->ChildNodes[1], pos))
        return hovered_node;

    // There is an edge case when docking into a dockspace which only has inactive nodes (because none of the windows are active)
    // In this case we need to fallback into any leaf mode, possibly the central node.
    if (node->IsDockSpace && node->IsRootNode())
    {
        if (node->CentralNode && node->IsLeafNode()) // FIXME-20181220: We should not have to test for IsLeafNode() here but we have another bug to fix first.
            return node->CentralNode;
        return DockNodeTreeFindFallbackLeafNode(node);
    }
    
    return NULL;
}

//-----------------------------------------------------------------------------
// Docking: Public Functions (SetWindowDock, DockSpace, DockSpaceOverViewport)
//-----------------------------------------------------------------------------

void ImGui::SetWindowDock(ImGuiWindow* window, ImGuiID dock_id, ImGuiCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowDockAllowFlags & cond) == 0)
        return;
    window->SetWindowDockAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);

    if (window->DockId == dock_id)
        return;

    // If the user attempt to set a dock id that is a split node, we'll dig within to find a suitable docking spot
    ImGuiContext* ctx = GImGui;
    if (ImGuiDockNode* new_node = DockContextFindNodeByID(ctx, dock_id))
        if (new_node->IsSplitNode())
        {
            // Policy: Find central node or latest focused node. We first move back to our root node.
            new_node = DockNodeGetRootNode(new_node);
            if (new_node->CentralNode)
                dock_id = new_node->CentralNode->ID;
            else
                dock_id = new_node->LastFocusedNodeID;
            }

    if (window->DockId == dock_id)
        return;

    if (window->DockNode)
        DockNodeRemoveWindow(window->DockNode, window, 0);
    window->DockId = dock_id;
}

// Create an explicit dockspace node within an existing window. Also expose dock node flags and creates a CentralNode by default.
// The Central Node is always displayed even when empty and shrink/extend according to the requested size of its neighbors.
void ImGui::DockSpace(ImGuiID id, const ImVec2& size_arg, ImGuiDockNodeFlags dockspace_flags, const ImGuiWindowClass* window_class)
{
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;
    ImGuiWindow* window = GetCurrentWindow();
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
        return;

    ImGuiDockNode* node = DockContextFindNodeByID(ctx, id);
    if (!node)
    {
        node = DockContextAddNode(ctx, id);
        node->IsCentralNode = true;
    }
    node->Flags = dockspace_flags;
    node->WindowClass = window_class ? *window_class : ImGuiWindowClass();

    // When a Dockspace transitioned form implicit to explicit this may be called a second time
    // It is possible that the node has already been claimed by a docked window which appeared before the DockSpace() node, so we overwrite IsDockSpace again.
    if (node->LastFrameActive == g.FrameCount && !(dockspace_flags & ImGuiDockNodeFlags_KeepAliveOnly))
    {
        IM_ASSERT(node->IsDockSpace == false && "Cannot call DockSpace() twice a frame with the same ID");
        node->IsDockSpace = true;
        return;
    }
    node->IsDockSpace = true;

    // Keep alive mode, this is allow windows docked into this node so stay docked even if they are not visible
    if (dockspace_flags & ImGuiDockNodeFlags_KeepAliveOnly)
    {
        node->LastFrameAlive = g.FrameCount;
        return;
    }

    const ImVec2 content_avail = GetContentRegionAvail();
    ImVec2 size = ImFloor(size_arg);
    if (size.x <= 0.0f)
        size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
    if (size.y <= 0.0f)
        size.y = ImMax(content_avail.y + size.y, 4.0f);

    node->Pos = window->DC.CursorPos;
    node->Size = node->SizeRef = size;
    SetNextWindowPos(node->Pos);
    SetNextWindowSize(node->Size);
    g.NextWindowData.PosUndock = false;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_DockNodeHost;
    window_flags |= ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    char title[256];
    ImFormatString(title, IM_ARRAYSIZE(title), "%s/DockSpace_%08X", window->Name, id);

    if (node->Windows.Size > 0 || node->IsSplitNode())
        PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
    PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    Begin(title, NULL, window_flags);
    PopStyleVar();
    if (node->Windows.Size > 0 || node->IsSplitNode())
        PopStyleColor();

    ImGuiWindow* host_window = g.CurrentWindow;
    host_window->DockNodeAsHost = node;
    host_window->ChildId = window->GetID(title);
    node->HostWindow = host_window;
    node->OnlyNodeWithWindows = NULL;

    IM_ASSERT(node->IsRootNode());
    DockNodeUpdate(node);

    End();
}

// Tips: Use with ImGuiDockNodeFlags_PassthruDockspace!
// The limitation with this call is that your window won't have a menu bar. 
// Even though we could pass window flags, it would also require the user to be able to call BeginMenuBar() somehow meaning we can't Begin/End in a single function.
// So if you want a menu bar you need to repeat this code manually ourselves. As with advanced other Docking API, we may change this function signature.
ImGuiID ImGui::DockSpaceOverViewport(ImGuiViewport* viewport, ImGuiDockNodeFlags dockspace_flags, const ImGuiWindowClass* window_class)
{
    if (viewport == NULL)
        viewport = GetMainViewport();

    SetNextWindowPos(viewport->Pos);
    SetNextWindowSize(viewport->Size);
    SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags host_window_flags = 0;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruDockspace)
        host_window_flags |= ImGuiWindowFlags_NoBackground;

    char label[32];
    ImFormatString(label, IM_ARRAYSIZE(label), "DockspaceViewport_%08X", viewport->ID);

    PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    Begin(label, NULL, host_window_flags);
    PopStyleVar(3);

    ImGuiID dockspace_id = GetID("Dockspace");
    DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags, window_class);
    End();

    return dockspace_id;
}

//-----------------------------------------------------------------------------
// Docking: Builder Functions
//-----------------------------------------------------------------------------
// Very early end-user API to manipulate dock nodes.
// It is expected that those functions are all called _before_ the dockspace node submission.
//-----------------------------------------------------------------------------

void ImGui::DockBuilderDockWindow(const char* window_name, ImGuiID node_id)
{
    ImGuiID window_id = ImHash(window_name, 0);
    if (ImGuiWindow* window = FindWindowByID(window_id))
    {
        // Apply to created window
        SetWindowDock(window, node_id, ImGuiCond_Always);
    }
    else
    {
        // Apply to settings
        ImGuiWindowSettings* settings = FindWindowSettings(window_id);
        if (settings == NULL)
            settings = CreateNewWindowSettings(window_name);
        settings->DockId = node_id;
    }
}

ImGuiDockNode* ImGui::DockBuilderGetNode(ImGuiID node_id)
{
    ImGuiContext* ctx = GImGui;
    return DockContextFindNodeByID(ctx, node_id);
}

void ImGui::DockBuilderAddNode(ImGuiID id, ImVec2 ref_size, ImGuiDockNodeFlags flags)
{
    ImGuiContext* ctx = GImGui;
    DockSpace(id, ImVec2(0,0), flags | ImGuiDockNodeFlags_KeepAliveOnly);
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, id);
    node->SizeRef = node->Size = ref_size;
    node->LastFrameAlive = -1;
}

void ImGui::DockBuilderRemoveNode(ImGuiID node_id)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, node_id);
    if (node == NULL)
        return;
    DockBuilderRemoveNodeDockedWindows(node_id, true);
    DockBuilderRemoveNodeChildNodes(node_id);
    if (node->IsCentralNode && node->ParentNode)
        node->ParentNode->IsCentralNode = true;
    DockContextRemoveNode(ctx, node, true);
}

void ImGui::DockBuilderRemoveNodeChildNodes(ImGuiID root_id)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockContext* dc = ctx->DockContext;

    ImGuiDockNode* root_node = root_id ? DockContextFindNodeByID(ctx, root_id) : NULL;
    if (root_id && root_node == NULL)
        return;
    bool has_document_root = false;

    // Process active windows
    ImVector<ImGuiDockNode*> nodes_to_remove;
    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
        {
            bool want_removal = (root_id == 0) || (node->ID != root_id && DockNodeGetRootNode(node)->ID == root_id);
            if (want_removal)
            {
                if (node->IsCentralNode)
                    has_document_root = true;
                if (root_id != 0)
                    DockContextQueueNotifyRemovedNode(ctx, node);
                if (root_node)
                    DockNodeMoveWindows(root_node, node);
                nodes_to_remove.push_back(node);
            }
        }

    // DockNodeMoveWindows->DockNodeAddWindow will normally set those when reaching two windows (which is only adequate during interactive merge)
    // Make sure we don't lose our current pos/size. (FIXME-DOCK: Consider tidying up that code in DockNodeAddWindow instead)
    if (root_node)
        root_node->InitFromFirstWindowPosSize = false;

    // Apply to settings
    for (int settings_n = 0; settings_n < ctx->SettingsWindows.Size; settings_n++)
        if (ImGuiID window_settings_dock_id = ctx->SettingsWindows[settings_n].DockId)
            for (int n = 0; n < nodes_to_remove.Size; n++)
                if (nodes_to_remove[n]->ID == window_settings_dock_id)
                {
                    ctx->SettingsWindows[settings_n].DockId = root_id;
                    break;
                }

    // Not really efficient, but easier to destroy a whole hierarchy considering DockContextRemoveNode is attempting to merge nodes
    if (nodes_to_remove.Size > 1)
        ImQsort(nodes_to_remove.Data, nodes_to_remove.Size, sizeof(ImGuiDockNode*), DockNodeComparerDepthMostFirst);
    for (int n = 0; n < nodes_to_remove.Size; n++)
        DockContextRemoveNode(ctx, nodes_to_remove[n], false);

    if (root_id == 0)
    {
        dc->Nodes.Clear();
        dc->Requests.clear();
    }
    else if (has_document_root)
    {
        root_node->IsCentralNode = true;
    }
}

void ImGui::DockBuilderRemoveNodeDockedWindows(ImGuiID root_id, bool clear_persistent_docking_references)
{
    // Clear references in settings
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;
    if (clear_persistent_docking_references)
    {
        for (int n = 0; n < g.SettingsWindows.Size; n++)
        {
            ImGuiWindowSettings* settings = &g.SettingsWindows[n];
            bool want_removal = (root_id == 0) || (settings->DockId == root_id);
            if (!want_removal && settings->DockId != 0)
                if (ImGuiDockNode* node = DockContextFindNodeByID(ctx, settings->DockId))
                    if (DockNodeGetRootNode(node)->ID == root_id)
                        want_removal = true;
            if (want_removal)
                settings->DockId = 0;
        }
    }

    // Clear references in windows
    for (int n = 0; n < g.Windows.Size; n++)
    {
        ImGuiWindow* window = g.Windows[n];
        bool want_removal = (root_id == 0) || (window->DockNode && DockNodeGetRootNode(window->DockNode)->ID == root_id) || (window->DockNodeAsHost && window->DockNodeAsHost->ID == root_id);
        if (want_removal)
        {
            const ImGuiID backup_dock_id = window->DockId;
			(void)backup_dock_id;
            DockContextProcessUndockWindow(ctx, window, clear_persistent_docking_references);
            if (!clear_persistent_docking_references)
                IM_ASSERT(window->DockId == backup_dock_id);
        }
    }
}

ImGuiID ImGui::DockBuilderSplitNode(ImGuiID id, ImGuiDir split_dir, float size_ratio_for_node_at_dir, ImGuiID* out_id_at_dir, ImGuiID* out_id_other)
{
    ImGuiContext* ctx = GImGui;
    IM_ASSERT(split_dir != ImGuiDir_None);

    ImGuiDockNode* node = DockContextFindNodeByID(ctx, id);
    if (node == NULL)
    {
        IM_ASSERT(node != NULL);
        return 0;
    }

    IM_ASSERT(!node->IsSplitNode()); // Assert if already Split

    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Split;
    req.DockTargetWindow = NULL;
    req.DockTargetNode = node;
    req.DockPayload = NULL;
    req.DockSplitDir = split_dir;
    req.DockSplitRatio = ImSaturate((split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? size_ratio_for_node_at_dir : 1.0f - size_ratio_for_node_at_dir);
    req.DockSplitOuter = false;
    DockContextProcessDock(ctx, &req);

    ImGuiID id_at_dir = node->ChildNodes[(split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? 0 : 1]->ID;
    ImGuiID id_other = node->ChildNodes[(split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? 1 : 0]->ID;
    if (out_id_at_dir)
        *out_id_at_dir = id_at_dir;
    if (out_id_other)
        *out_id_other = id_other;
    return id_at_dir;
}

static ImGuiDockNode* DockBuilderCopyNodeRec(ImGuiDockNode* src_node, ImGuiID dst_node_id_if_known, ImVector<ImGuiID>* out_node_remap_pairs)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* dst_node = ImGui::DockContextAddNode(ctx, dst_node_id_if_known);
    dst_node->Flags = src_node->Flags;
    dst_node->Pos = src_node->Pos;
    dst_node->Size = src_node->Size;
    dst_node->SizeRef = src_node->SizeRef;
    dst_node->SplitAxis = src_node->SplitAxis;
    dst_node->IsDockSpace = src_node->IsDockSpace;
    dst_node->IsCentralNode = src_node->IsCentralNode;

    out_node_remap_pairs->push_back(src_node->ID);
    out_node_remap_pairs->push_back(dst_node->ID);

    for (int child_n = 0; child_n < IM_ARRAYSIZE(src_node->ChildNodes); child_n++)
        if (src_node->ChildNodes[child_n])
        {
            dst_node->ChildNodes[child_n] = DockBuilderCopyNodeRec(src_node->ChildNodes[child_n], 0, out_node_remap_pairs);
            dst_node->ChildNodes[child_n]->ParentNode = dst_node;
        }

    //IMGUI_DEBUG_LOG("Fork node %08X -> %08X (%d childs)\n", src_node->ID, dst_node->ID, dst_node->IsSplitNode() ? 2 : 0);
    return dst_node;
}

void ImGui::DockBuilderCopyNode(ImGuiID src_node_id, ImGuiID dst_node_id, ImVector<ImGuiID>* out_node_remap_pairs)
{
    ImGuiContext* ctx = GImGui;
    IM_ASSERT(src_node_id != 0);
    IM_ASSERT(dst_node_id != 0);
    IM_ASSERT(out_node_remap_pairs != NULL);

    ImGuiDockNode* src_node = DockContextFindNodeByID(ctx, src_node_id);
    IM_ASSERT(src_node != NULL);

    out_node_remap_pairs->clear();
    DockBuilderRemoveNode(dst_node_id);
    DockBuilderCopyNodeRec(src_node, dst_node_id, out_node_remap_pairs);

    IM_ASSERT((out_node_remap_pairs->Size % 2) == 0);
}

void ImGui::DockBuilderCopyWindowSettings(const char* src_name, const char* dst_name)
{
    ImGuiWindow* src_window = FindWindowByName(src_name);
    if (src_window == NULL)
        return;
    if (ImGuiWindow* dst_window = FindWindowByName(dst_name))
    {
        dst_window->Pos = src_window->Pos;
        dst_window->Size = src_window->Size;
        dst_window->SizeFull = src_window->SizeFull;
        dst_window->Collapsed = src_window->Collapsed;
    }
    else if (ImGuiWindowSettings* dst_settings = FindOrCreateWindowSettings(dst_name))
    {
        if (src_window->ViewportId != 0 && src_window->ViewportId != IMGUI_VIEWPORT_DEFAULT_ID)
        {
            dst_settings->ViewportPos = src_window->Pos;
            dst_settings->ViewportId = src_window->ViewportId;
            dst_settings->Pos = ImVec2(0.0f, 0.0f);
        }
        else
        {
            dst_settings->Pos = src_window->Pos;
        }
        dst_settings->Size = src_window->SizeFull;
        dst_settings->Collapsed = src_window->Collapsed;
    }
}

// FIXME: Will probably want to change this signature, in particular how the window remapping pairs are passed.
void ImGui::DockBuilderCopyDockspace(ImGuiID src_dockspace_id, ImGuiID dst_dockspace_id, ImVector<const char*>* in_window_remap_pairs)
{
    IM_ASSERT(src_dockspace_id != 0);
    IM_ASSERT(dst_dockspace_id != 0);
    IM_ASSERT(in_window_remap_pairs != NULL);
    IM_ASSERT((in_window_remap_pairs->Size % 2) == 0);

    // Duplicate entire dock
    // FIXME: When overwriting dst_dockspace_id, windows that aren't part of our dockspace window class but that are docked in a same node will be split apart,
    // whereas we could attempt to at least keep them together in a new, same floating node.
    ImVector<ImGuiID> node_remap_pairs;
    DockBuilderCopyNode(src_dockspace_id, dst_dockspace_id, &node_remap_pairs);

    // Attempt to transition all the upcoming windows associated to dst_dockspace_id into the newly created hierarchy of dock nodes
    // (The windows associated to src_dockspace_id are staying in place)
    ImVector<ImGuiID> src_windows;
    for (int remap_window_n = 0; remap_window_n < in_window_remap_pairs->Size; remap_window_n += 2)
    {
        const char* src_window_name = (*in_window_remap_pairs)[remap_window_n];
        const char* dst_window_name = (*in_window_remap_pairs)[remap_window_n + 1];
        ImGuiID src_window_id = ImHash(src_window_name, 0);
        src_windows.push_back(src_window_id);

        // Search in the remapping tables
        ImGuiID src_dock_id = 0;
        if (ImGuiWindow* src_window = FindWindowByID(src_window_id))
            src_dock_id = src_window->DockId;
        else if (ImGuiWindowSettings* src_window_settings = FindWindowSettings(src_window_id))
            src_dock_id = src_window_settings->DockId;
        ImGuiID dst_dock_id = 0;
        for (int dock_remap_n = 0; dock_remap_n < node_remap_pairs.Size; dock_remap_n += 2)
            if (node_remap_pairs[dock_remap_n] == src_dock_id)
            {
                dst_dock_id = node_remap_pairs[dock_remap_n + 1];
                //node_remap_pairs[dock_remap_n] = node_remap_pairs[dock_remap_n + 1] = 0; // Clear
                break;
            }

        if (dst_dock_id != 0)
        {
            // Docked windows gets redocked into the new node hierarchy. 
            //IMGUI_DEBUG_LOG("Remap window '%s' %08X -> %08X\n", dst_window_name, src_dock_id, dst_dock_id);
            DockBuilderDockWindow(dst_window_name, dst_dock_id);
        }
        else
        {
            // Floating windows gets their settings transferred (regardless of whether the new window already exist or not)
            // When this is leading to a Copy and not a Move, we would get two overlapping floating windows. Could we possibly dock them together?
            DockBuilderCopyWindowSettings(src_window_name, dst_window_name);
        }
    }

    // Anything else in the source nodes of 'node_remap_pairs' are windows that were docked in src_dockspace_id but are not owned by it (unaffiliated windows, e.g. "ImGui Demo")
    // Find those windows and move to them to the cloned dock node. This may be optional?
    for (int dock_remap_n = 0; dock_remap_n < node_remap_pairs.Size; dock_remap_n += 2)
        if (ImGuiID src_dock_id = node_remap_pairs[dock_remap_n])
        {
            ImGuiID dst_dock_id = node_remap_pairs[dock_remap_n + 1];
            ImGuiDockNode* dock_node = DockBuilderGetNode(src_dock_id);
            for (int window_n = 0; window_n < dock_node->Windows.Size; window_n++)
            {
                ImGuiWindow* window = dock_node->Windows[window_n];
                if (src_windows.contains(window->ID))
                    continue;

                // Docked windows gets redocked into the new node hierarchy. 
                //IMGUI_DEBUG_LOG("Remap window '%s' %08X -> %08X\n", window->Name, src_dock_id, dst_dock_id);
                DockBuilderDockWindow(window->Name, dst_dock_id);
            }
        }
}

void ImGui::DockBuilderFinish(ImGuiID root_id)
{
    ImGuiContext* ctx = GImGui;
    //DockContextRebuild(ctx);
    DockContextBuildAddWindowsToNodes(ctx, root_id);
}

//-----------------------------------------------------------------------------
// Docking: Begin/End Functions (called from Begin/End)
//-----------------------------------------------------------------------------

void ImGui::BeginDocked(ImGuiWindow* window, bool* p_open)
{
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;

    const bool auto_dock_node = (g.IO.ConfigDockingTabBarOnSingleWindows) && !(window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking));

    if (auto_dock_node)
    {
        if (window->DockId == 0)
        {
            IM_ASSERT(window->DockNode == NULL);
            window->DockId = DockContextGenNodeID(ctx);
        }
    }
    else
    {
        // Calling SetNextWindowPos() undock windows by default (by setting PosUndock)
        bool want_undock = false;
        want_undock |= (window->Flags & ImGuiWindowFlags_NoDocking) != 0;
        want_undock |= (g.NextWindowData.PosCond && (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) && g.NextWindowData.PosUndock);
        g.NextWindowData.PosUndock = false;
        if (want_undock)
        {
            DockContextProcessUndockWindow(ctx, window);
            return;
        }
    }

    // Bind to our dock node
    ImGuiDockNode* dock_node = window->DockNode;
    if (dock_node != NULL)
        IM_ASSERT(window->DockId == dock_node->ID);
    if (window->DockId != 0 && dock_node == NULL)
    {
        dock_node = DockContextFindNodeByID(ctx, window->DockId);
        
        // We should not be docking into a split node (SetWindowDock should avoid this)
        if (dock_node && dock_node->IsSplitNode())
        {
            DockContextProcessUndockWindow(ctx, window);
            return;
        }

        // Create node
        if (dock_node == NULL)
        {
            dock_node = DockContextAddNode(ctx, window->DockId);
            if (auto_dock_node)
                dock_node->LastFrameAlive = g.FrameCount;
        }

        DockNodeAddWindow(dock_node, window, true);
        IM_ASSERT(dock_node == window->DockNode);

        // Fix an edge case with auto-resizing windows: if they are created on the same frame they are creating their dock node, 
        // we don't want their initial zero-size to spread to the DockNode. We preserve their size.
        SetNextWindowPos(window->Pos);
        SetNextWindowSize(window->SizeFull);
        g.NextWindowData.PosUndock = false;
    }

    // Undock if the ImGuiDockNodeFlags_NoDockingInCentralNode got set
    if (dock_node->IsCentralNode && (dock_node->Flags & ImGuiDockNodeFlags_NoDockingInCentralNode))
    {
        DockContextProcessUndockWindow(ctx, window);
        return;
    }

    // Undock if our dockspace node disappeared
    // Note how we are testing for LastFrameAlive and NOT LastFrameActive. A DockSpace node can be maintained alive while being inactive with ImGuiDockNodeFlags_KeepAliveOnly.
    if (dock_node->LastFrameAlive < g.FrameCount)
    {
        // If the window has been orphaned, transition the docknode to an implicit node processed in DockContextUpdateDocking()
        ImGuiDockNode* root_node = DockNodeGetRootNode(dock_node);
        if (root_node->LastFrameAlive < g.FrameCount)
        {
            DockContextProcessUndockWindow(ctx, window);
        }
        else
        {
            window->DockIsActive = true;
            window->DockTabIsVisible = false;
        }
        return;
    }

    // Undock if we are submitted earlier than the host window
    if (dock_node->HostWindow && window->BeginOrderWithinContext < dock_node->HostWindow->BeginOrderWithinContext)
    {
        DockContextProcessUndockWindow(ctx, window);
        return;
    }

    // FIXME-DOCK: replace ->HostWindow NULL compare with something more explicit (~was initially intended as a first frame test)
    if (dock_node->HostWindow == NULL)
    {
        window->DockTabIsVisible = false;
        return;
    }

    IM_ASSERT(dock_node->HostWindow);
    IM_ASSERT(dock_node->IsLeafNode());

    // Position window
    SetNextWindowPos(dock_node->Pos);
    SetNextWindowSize(dock_node->Size);
    g.NextWindowData.PosUndock = false; // Cancel implicit undocking of SetNextWindowPos()

    window->DockIsActive = true;
    window->DockTabIsVisible = false;
    if (dock_node->Flags & ImGuiDockNodeFlags_KeepAliveOnly)
        return;

    // When the window is selected we mark it as visible.
    if (dock_node->TabBar && dock_node->TabBar->VisibleTabId == window->ID)
        window->DockTabIsVisible = true;

    // When we are about to select this tab (which will only be visible on the _next frame_), flag it with a non-zero HiddenFramesForResize.
    // This will have the important effect of actually returning true in Begin() and not setting SkipItems, allowing an earlier submission of the window contents.
    // This is analogous to regular windows being hidden from one frame. It is especially important as nested TabBars would otherwise generate flicker in the form
    // of one empty frame.
    // Note that we set HiddenFramesForResize=2 because BeginDocked() is called just before Begin() has a chance to decrement the value. Effectively it'll be a 1 frame thing. 
    if (!window->DockTabIsVisible && dock_node->TabBar && dock_node->TabBar->NextSelectedTabId == window->ID)
        window->HiddenFramesForResize = 2;

    // Update window flag
    IM_ASSERT((window->Flags & ImGuiWindowFlags_ChildWindow) == 0);
    window->Flags |= ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoResize;
    if (dock_node->IsHiddenTabBar)
        window->Flags |= ImGuiWindowFlags_NoTitleBar;
    else
        window->Flags &= ~ImGuiWindowFlags_NoTitleBar;      // Clear the NoTitleBar flag in case the user set it: confusingly enough we need a title bar height so we are correctly offset, but it won't be displayed!

    // Save new dock order only if the tab bar is active
    if (dock_node->TabBar)
        window->DockOrder = (short)DockNodeGetTabOrder(window);

    if ((dock_node->WantCloseAll || dock_node->WantCloseTabID == window->ID) && p_open != NULL)
        *p_open = false;

    // Update ChildId to allow returning from Child to Parent with Escape
    ImGuiWindow* parent_window = window->DockNode->HostWindow;
    window->ChildId = parent_window->GetID(window->Name);
}

void ImGui::BeginAsDockableDragDropSource(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ActiveId == window->MoveId);

    window->DC.LastItemId = window->MoveId;
    window = window->RootWindow;
    IM_ASSERT((window->Flags & ImGuiWindowFlags_NoDocking) == 0);
    bool is_drag_docking = (g.IO.ConfigDockingWithShift) || ImRect(0, 0, window->SizeFull.x, GetFrameHeight()).Contains(g.ActiveIdClickOffset);
    if (is_drag_docking && BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip | ImGuiDragDropFlags_SourceNoHoldToOpenOthers | ImGuiDragDropFlags_SourceAutoExpirePayload))
    {
        SetDragDropPayload(IMGUI_PAYLOAD_TYPE_WINDOW, &window, sizeof(window));
        EndDragDropSource();
    }
}

void ImGui::BeginAsDockableDragDropTarget(ImGuiWindow* window)
{
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;

    IM_ASSERT((window->Flags & ImGuiWindowFlags_NoDocking) == 0);
    if (!g.DragDropActive)
        return;

    if (!BeginDragDropTargetCustom(window->Rect(), window->ID))
        return;

    // Peek into the payload before calling AcceptDragDropPayload() so we can handle overlapping dock nodes with filtering
    // (this is a little unusual pattern, normally most code would call AcceptDragDropPayload directly)
    const ImGuiPayload* payload = &g.DragDropPayload;
    if (!payload->IsDataType(IMGUI_PAYLOAD_TYPE_WINDOW) || !DockNodeIsDropAllowed(window, *(ImGuiWindow**)payload->Data))
    {
        EndDragDropTarget();
        return;
    }

    ImGuiWindow* payload_window = *(ImGuiWindow**)payload->Data;
    if (AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_WINDOW, ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
    {
        bool allow_null_target_node = false;
        ImGuiDockNode* target_node = NULL;
        if (window->DockNodeAsHost)
            target_node = DockNodeTreeFindNodeByPos(window->DockNodeAsHost, g.IO.MousePos);
        else if (window->DockNode) // && window->DockIsActive)
            target_node = window->DockNode;
        else
            allow_null_target_node = true; // Dock into a regular window

        const ImRect explicit_target_rect = (target_node && target_node->TabBar) ? target_node->TabBar->BarRect : ImRect(window->Pos, window->Pos + ImVec2(window->Size.x, GetFrameHeight()));
        const bool is_explicit_target = g.IO.ConfigDockingWithShift || IsMouseHoveringRect(explicit_target_rect.Min, explicit_target_rect.Max);

        // Preview docking request and find out split direction/ratio
        //const bool do_preview = true;     // Ignore testing for payload->IsPreview() which removes one frame of delay, but breaks overlapping drop targets within the same window.        
        const bool do_preview = payload->IsPreview() || payload->IsDelivery();
        if (do_preview && (target_node != NULL || allow_null_target_node))
        {
            ImGuiDockPreviewData split_inner, split_outer;
            ImGuiDockPreviewData* split_data = &split_inner;
            if (target_node && (target_node->ParentNode || target_node->IsCentralNode))
                if (ImGuiDockNode* root_node = DockNodeGetRootNode(target_node))
                    if (DockNodePreviewDockCalc(window, root_node, payload_window, &split_outer, is_explicit_target, true))
                        split_data = &split_outer;
            DockNodePreviewDockCalc(window, target_node, payload_window, &split_inner, is_explicit_target, false);
            if (split_data == &split_outer)
                split_inner.IsDropAllowed = false;

            // Draw inner then outer, so that previewed tab (in inner data) will be behind the outer drop boxes
            DockNodePreviewDockRender(window, target_node, payload_window, &split_inner);
            DockNodePreviewDockRender(window, target_node, payload_window, &split_outer);

            // Queue docking request
            if (split_data->IsDropAllowed && payload->IsDelivery())
                DockContextQueueDock(ctx, window, split_data->SplitNode, payload_window, split_data->SplitDir, split_data->SplitRatio, split_data == &split_outer);
        }
    }
    EndDragDropTarget();
}

//-----------------------------------------------------------------------------
// Docking: Settings
//-----------------------------------------------------------------------------

static void ImGui::DockSettingsMoveDockReferencesInInactiveWindow(ImGuiID old_dock_id, ImGuiID new_dock_id)
{
    ImGuiContext& g = *GImGui;
    for (int window_n = 0; window_n < g.Windows.Size; window_n++)
    {
        ImGuiWindow* window = g.Windows[window_n];
        if (window->DockId == old_dock_id && window->DockNode == NULL)
            window->DockId = new_dock_id;
    }
    for (int settings_n = 0; settings_n < g.SettingsWindows.Size; settings_n++)     // FIXME-OPT: We could remove this loop by storing the index in the map
    {
        ImGuiWindowSettings* window_settings = &g.SettingsWindows[settings_n];
        if (window_settings->DockId == old_dock_id)
            window_settings->DockId = new_dock_id;
    }
}

// Remove references stored in ImGuiWindowSettings to the given ImGuiDockNodeSettings
static void ImGui::DockSettingsRemoveReferencesToNodes(ImGuiID* node_ids, int node_ids_count)
{
    ImGuiContext& g = *GImGui;
    int found = 0;
    for (int settings_n = 0; settings_n < g.SettingsWindows.Size; settings_n++)     // FIXME-OPT: We could remove this loop by storing the index in the map
    {
        ImGuiWindowSettings* window_settings = &g.SettingsWindows[settings_n];
        for (int node_n = 0; node_n < node_ids_count; node_n++)
            if (window_settings->DockId == node_ids[node_n])
            {
                window_settings->DockId = 0;
                window_settings->DockOrder = -1;
                if (++found < node_ids_count)
                    break;
                return;
            }
    }
}

static ImGuiDockNodeSettings*   ImGui::DockSettingsFindNodeSettings(ImGuiContext* ctx, ImGuiID id)
{
    // FIXME-OPT
    ImGuiDockContext* dc = ctx->DockContext;
    for (int n = 0; n < dc->SettingsNodes.Size; n++)
        if (dc->SettingsNodes[n].ID == id)
            return &dc->SettingsNodes[n];
    return NULL;
}

static void* ImGui::DockSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    if (strcmp(name, "Data") != 0)
        return NULL;
    return (void*)1;
}

static void ImGui::DockSettingsHandler_ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler*, void*, const char* line)
{
    char c = 0;
    int x = 0, y = 0;
    int r = 0;

    // Parsing, e.g.
    // " DockNode   ID=0x00000001 Pos=383,193 Size=201,322 Split=Y,0.506 "
    // "   DockNode ID=0x00000002 Parent=0x00000001 "
    ImGuiDockNodeSettings node;
    line = ImStrSkipBlank(line);
    if      (strncmp(line, "DockNode", 8) == 0)  { line = ImStrSkipBlank(line + strlen("DockNode")); }
    else if (strncmp(line, "DockSpace", 9) == 0) { line = ImStrSkipBlank(line + strlen("DockSpace")); node.IsDockSpace = true; }
    else return;
    if (sscanf(line, "ID=0x%08X%n",      &node.ID, &r) == 1)        { line += r; } else return;
    if (sscanf(line, " Parent=0x%08X%n", &node.ParentID, &r) == 1)  { line += r; if (node.ParentID == 0) return; }
    if (node.ParentID == 0)
    {
        if (sscanf(line, " Pos=%i,%i%n",  &x, &y, &r) == 2)         { line += r; node.Pos = ImVec2ih((short)x, (short)y); } else return;
        if (sscanf(line, " Size=%i,%i%n", &x, &y, &r) == 2)         { line += r; node.Size = ImVec2ih((short)x, (short)y); } else return;
    }
    else
    {
        if (sscanf(line, " SizeRef=%i,%i%n", &x, &y, &r) == 2)      { line += r; node.SizeRef = ImVec2ih((short)x, (short)y); }
    }
    if (sscanf(line, " Split=%c%n", &c, &r) == 1)                   { line += r; if (c == 'X') node.SplitAxis = ImGuiAxis_X; else if (c == 'Y') node.SplitAxis = ImGuiAxis_Y; }
    if (sscanf(line, " CentralNode=%d%n", &x, &r) == 1)             { line += r; node.IsCentralNode = (x != 0); }
    if (sscanf(line, " HiddenTabBar=%d%n", &x, &r) == 1)            { line += r; node.IsHiddenTabBar = (x != 0); }
    if (sscanf(line, " SelectedTab=0x%08X%n", &node.SelectedTabID,&r) == 1) { line += r; }
    ImGuiDockContext* dc = ctx->DockContext;
    if (node.ParentID != 0)
        if (ImGuiDockNodeSettings* parent_settings = DockSettingsFindNodeSettings(ctx, node.ParentID))
            node.Depth = parent_settings->Depth + 1;
    dc->SettingsNodes.push_back(node);
}

static void DockSettingsHandler_DockNodeToSettings(ImGuiDockContext* dc, ImGuiDockNode* node, int depth)
{
    ImGuiDockNodeSettings node_settings;
    IM_ASSERT(depth < (1 << (sizeof(node_settings.Depth) << 3)));
    node_settings.ID = node->ID;
    node_settings.ParentID = node->ParentNode ? node->ParentNode->ID : 0;
    node_settings.SelectedTabID = node->SelectedTabID;
    node_settings.SplitAxis = node->IsSplitNode() ? (char)node->SplitAxis : ImGuiAxis_None;
    node_settings.Depth = (char)depth;
    node_settings.IsDockSpace = (char)node->IsDockSpace;
    node_settings.IsCentralNode = (char)node->IsCentralNode;
    node_settings.IsHiddenTabBar = (char)node->IsHiddenTabBar;
    node_settings.Pos = ImVec2ih((short)node->Pos.x, (short)node->Pos.y);
    node_settings.Size = ImVec2ih((short)node->Size.x, (short)node->Size.y);
    node_settings.SizeRef = ImVec2ih((short)node->SizeRef.x, (short)node->SizeRef.y);
    dc->SettingsNodes.push_back(node_settings);
    if (node->ChildNodes[0])
        DockSettingsHandler_DockNodeToSettings(dc, node->ChildNodes[0], depth + 1);
    if (node->ChildNodes[1])
        DockSettingsHandler_DockNodeToSettings(dc, node->ChildNodes[1], depth + 1);
}

static void ImGui::DockSettingsHandler_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = g.DockContext;
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
        return;

    // Gather settings data
    // (unlike our windows settings, because nodes are always built we can do a full rewrite of the SettingsNode buffer)
    dc->SettingsNodes.resize(0);
    dc->SettingsNodes.reserve(dc->Nodes.Data.Size);
    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
            if (node->IsRootNode())
                DockSettingsHandler_DockNodeToSettings(dc, node, 0);

    int max_depth = 0;
    for (int node_n = 0; node_n < dc->SettingsNodes.Size; node_n++)
        max_depth = ImMax((int)dc->SettingsNodes[node_n].Depth, max_depth);

    // Write to text buffer
    buf->appendf("[%s][Data]\n", handler->TypeName);
    for (int node_n = 0; node_n < dc->SettingsNodes.Size; node_n++)
    {
#if IMGUI_DEBUG_DOCKING_INI
        const int line_start_pos = buf->size();
#endif
        const ImGuiDockNodeSettings* node_settings = &dc->SettingsNodes[node_n];
        buf->appendf("%*s%s%*s", node_settings->Depth * 2, "", node_settings->IsDockSpace ? "DockSpace" : "DockNode ", (max_depth - node_settings->Depth) * 2, "");  // Text align nodes to facilitate looking at .ini file
        buf->appendf(" ID=0x%08X", node_settings->ID);
        if (node_settings->ParentID)
            buf->appendf(" Parent=0x%08X SizeRef=%d,%d", node_settings->ParentID, node_settings->SizeRef.x, node_settings->SizeRef.y);
        else
            buf->appendf(" Pos=%d,%d Size=%d,%d", node_settings->Pos.x, node_settings->Pos.y, node_settings->Size.x, node_settings->Size.y);
        if (node_settings->SplitAxis != ImGuiAxis_None)
            buf->appendf(" Split=%c", (node_settings->SplitAxis == ImGuiAxis_X) ? 'X' : 'Y');
        if (node_settings->IsCentralNode)
            buf->appendf(" CentralNode=1");
        if (node_settings->IsHiddenTabBar)
            buf->appendf(" HiddenTabBar=1");
        if (node_settings->SelectedTabID)
            buf->appendf(" SelectedTab=0x%08X", node_settings->SelectedTabID);

#if IMGUI_DEBUG_DOCKING_INI
        // [DEBUG] Include comments in the .ini file to ease debugging
        if (ImGuiDockNode* node = DockContextFindNodeByID(ctx, node_settings->ID))
        {
            buf->appendf("%*s", ImMax(2, (line_start_pos + 92) - buf->size()), "");        // Align everything
            if (node->IsDockSpace && node->HostWindow && node->HostWindow->ParentWindow)
                buf->appendf(" ; in '%s'", node->HostWindow->ParentWindow->Name);
            int contains_window = 0;
            for (int window_n = 0; window_n < ctx->SettingsWindows.Size; window_n++)
                if (ctx->SettingsWindows[window_n].DockId == node_settings->ID)
                {
                    if (contains_window++ == 0)
                        buf->appendf(" ; contains ");
                    buf->appendf("'%s' ", ctx->SettingsWindows[window_n].Name);
                }
        }
#endif
        buf->appendf("\n");
    }
    buf->appendf("\n");
}

//-----------------------------------------------------------------------------
// [SECTION] LOGGING/CAPTURING
//-----------------------------------------------------------------------------

// Pass text data straight to log (without being displayed)
void ImGui::LogText(const char* fmt, ...)
{
    ImGuiContext& g = *GImGui;
    if (!g.LogEnabled)
        return;

    va_list args;
    va_start(args, fmt);
    if (g.LogFile)
        vfprintf(g.LogFile, fmt, args);
    else
        g.LogClipboard.appendfv(fmt, args);
    va_end(args);
}

// Internal version that takes a position to decide on newline placement and pad items according to their depth.
// We split text into individual lines to add current tree level padding
void ImGui::LogRenderedText(const ImVec2* ref_pos, const char* text, const char* text_end)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (!text_end)
        text_end = FindRenderedTextEnd(text, text_end);

    const bool log_new_line = ref_pos && (ref_pos->y > window->DC.LogLinePosY + 1);
    if (ref_pos)
        window->DC.LogLinePosY = ref_pos->y;

    const char* text_remaining = text;
    if (g.LogStartDepth > window->DC.TreeDepth)  // Re-adjust padding if we have popped out of our starting depth
        g.LogStartDepth = window->DC.TreeDepth;
    const int tree_depth = (window->DC.TreeDepth - g.LogStartDepth);
    for (;;)
    {
        // Split the string. Each new line (after a '\n') is followed by spacing corresponding to the current depth of our log entry.
        const char* line_start = text_remaining;
        const char* line_end = ImStreolRange(line_start, text_end);
        const bool is_first_line = (line_start == text);
        const bool is_last_line = (line_end == text_end);
        if (!is_last_line || (line_start != line_end))
        {
            const int char_count = (int)(line_end - line_start);
            if (log_new_line || !is_first_line)
                LogText(IM_NEWLINE "%*s%.*s", tree_depth*4, "", char_count, line_start);
            else
                LogText(" %.*s", char_count, line_start);
        }

        if (is_last_line)
            break;
        text_remaining = line_end + 1;
    }
}

// Start logging ImGui output to TTY
void ImGui::LogToTTY(int max_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    ImGuiWindow* window = g.CurrentWindow;

    IM_ASSERT(g.LogFile == NULL);
    g.LogFile = stdout;
    g.LogEnabled = true;
    g.LogStartDepth = window->DC.TreeDepth;
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

// Start logging ImGui output to given file
void ImGui::LogToFile(int max_depth, const char* filename)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    ImGuiWindow* window = g.CurrentWindow;

    if (!filename)
    {
        filename = g.IO.LogFilename;
        if (!filename)
            return;
    }

    IM_ASSERT(g.LogFile == NULL);
    g.LogFile = ImFileOpen(filename, "ab");
    if (g.LogFile == NULL)
    {
        IM_ASSERT(g.LogFile != NULL); // Consider this an error
        return;
    }
    g.LogEnabled = true;
    g.LogStartDepth = window->DC.TreeDepth;
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

// Start logging ImGui output to clipboard
void ImGui::LogToClipboard(int max_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    ImGuiWindow* window = g.CurrentWindow;

    IM_ASSERT(g.LogFile == NULL);
    g.LogFile = NULL;
    g.LogEnabled = true;
    g.LogStartDepth = window->DC.TreeDepth;
    if (max_depth >= 0)
        g.LogAutoExpandMaxDepth = max_depth;
}

void ImGui::LogFinish()
{
    ImGuiContext& g = *GImGui;
    if (!g.LogEnabled)
        return;

    LogText(IM_NEWLINE);
    if (g.LogFile != NULL)
    {
        if (g.LogFile == stdout)
            fflush(g.LogFile);
        else
            fclose(g.LogFile);
        g.LogFile = NULL;
    }
    if (g.LogClipboard.size() > 1)
    {
        SetClipboardText(g.LogClipboard.begin());
        g.LogClipboard.clear();
    }
    g.LogEnabled = false;
}

// Helper to display logging buttons
void ImGui::LogButtons()
{
    ImGuiContext& g = *GImGui;

    PushID("LogButtons");
    const bool log_to_tty = Button("Log To TTY"); SameLine();
    const bool log_to_file = Button("Log To File"); SameLine();
    const bool log_to_clipboard = Button("Log To Clipboard"); SameLine();
    PushItemWidth(80.0f);
    PushAllowKeyboardFocus(false);
    SliderInt("Depth", &g.LogAutoExpandMaxDepth, 0, 9, NULL);
    PopAllowKeyboardFocus();
    PopItemWidth();
    PopID();

    // Start logging at the end of the function so that the buttons don't appear in the log
    if (log_to_tty)
        LogToTTY(g.LogAutoExpandMaxDepth);
    if (log_to_file)
        LogToFile(g.LogAutoExpandMaxDepth, g.IO.LogFilename);
    if (log_to_clipboard)
        LogToClipboard(g.LogAutoExpandMaxDepth);
}

//-----------------------------------------------------------------------------
// [SECTION] SETTINGS
//-----------------------------------------------------------------------------

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

ImGuiWindowSettings* ImGui::CreateNewWindowSettings(const char* name)
{
    ImGuiContext& g = *GImGui;
    g.SettingsWindows.push_back(ImGuiWindowSettings());
    ImGuiWindowSettings* settings = &g.SettingsWindows.back();
    settings->Name = ImStrdup(name);
    settings->ID = ImHash(name, 0);
    return settings;
}

ImGuiWindowSettings* ImGui::FindWindowSettings(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    for (int i = 0; i != g.SettingsWindows.Size; i++)
        if (g.SettingsWindows[i].ID == id)
            return &g.SettingsWindows[i];
    return NULL;
}

ImGuiWindowSettings* ImGui::FindOrCreateWindowSettings(const char* name)
{
    if (ImGuiWindowSettings* settings = FindWindowSettings(ImHash(name, 0)))
        return settings;
    return CreateNewWindowSettings(name);
}

void ImGui::LoadIniSettingsFromDisk(const char* ini_filename)
{
    size_t file_data_size = 0;
    char* file_data = (char*)ImFileLoadToMemory(ini_filename, "rb", &file_data_size);
    if (!file_data)
        return;
    LoadIniSettingsFromMemory(file_data, (size_t)file_data_size);
    ImGui::MemFree(file_data);
}

ImGuiSettingsHandler* ImGui::FindSettingsHandler(const char* type_name)
{
    ImGuiContext& g = *GImGui;
    const ImGuiID type_hash = ImHash(type_name, 0, 0);
    for (int handler_n = 0; handler_n < g.SettingsHandlers.Size; handler_n++)
        if (g.SettingsHandlers[handler_n].TypeHash == type_hash)
            return &g.SettingsHandlers[handler_n];
    return NULL;
}

// Zero-tolerance, no error reporting, cheap .ini parsing
void ImGui::LoadIniSettingsFromMemory(const char* ini_data, size_t ini_size)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);
    IM_ASSERT(g.SettingsLoaded == false && g.FrameCount == 0);

    // For user convenience, we allow passing a non zero-terminated string (hence the ini_size parameter).
    // For our convenience and to make the code simpler, we'll also write zero-terminators within the buffer. So let's create a writable copy..
    if (ini_size == 0)
        ini_size = strlen(ini_data);
    char* buf = (char*)ImGui::MemAlloc(ini_size + 1);
    char* buf_end = buf + ini_size;
    memcpy(buf, ini_data, ini_size);
    buf[ini_size] = 0;

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
            char* type_end = (char*)(intptr_t)ImStrchrRange(type_start, name_end, ']');
            const char* name_start = type_end ? ImStrchrRange(type_end + 1, name_end, '[') : NULL;
            if (!type_end || !name_start)
            {
                name_start = type_start; // Import legacy entries that have no type
                type_start = "Window";
            }
            else
            {
                *type_end = 0; // Overwrite first ']'
                name_start++;  // Skip second '['
            }
            entry_handler = FindSettingsHandler(type_start);
            entry_data = entry_handler ? entry_handler->ReadOpenFn(&g, entry_handler, name_start) : NULL;
        }
        else if (entry_handler != NULL && entry_data != NULL)
        {
            // Let type handler parse the line
            entry_handler->ReadLineFn(&g, entry_handler, entry_data, line);
        }
    }
    ImGui::MemFree(buf);
    g.SettingsLoaded = true;
    DockContextOnLoadSettings(&g);
}

void ImGui::SaveIniSettingsToDisk(const char* ini_filename)
{
    ImGuiContext& g = *GImGui;
    g.SettingsDirtyTimer = 0.0f;
    if (!ini_filename)
        return;

    size_t ini_data_size = 0;
    const char* ini_data = SaveIniSettingsToMemory(&ini_data_size);
    FILE* f = ImFileOpen(ini_filename, "wt");
    if (!f)
        return;
    fwrite(ini_data, sizeof(char), ini_data_size, f);
    fclose(f);
}

// Call registered handlers (e.g. SettingsHandlerWindow_WriteAll() + custom handlers) to write their stuff into a text buffer
const char* ImGui::SaveIniSettingsToMemory(size_t* out_size)
{
    ImGuiContext& g = *GImGui;
    g.SettingsDirtyTimer = 0.0f;
    g.SettingsIniData.Buf.resize(0);
    g.SettingsIniData.Buf.push_back(0);
    for (int handler_n = 0; handler_n < g.SettingsHandlers.Size; handler_n++)
    {
        ImGuiSettingsHandler* handler = &g.SettingsHandlers[handler_n];
        handler->WriteAllFn(&g, handler, &g.SettingsIniData);
    }
    if (out_size)
        *out_size = (size_t)g.SettingsIniData.size();
    return g.SettingsIniData.c_str();
}

static void* SettingsHandlerWindow_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    ImGuiWindowSettings* settings = ImGui::FindWindowSettings(ImHash(name, 0));
    if (!settings)
        settings = ImGui::CreateNewWindowSettings(name);
    return (void*)settings;
}

static void SettingsHandlerWindow_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    ImGuiWindowSettings* settings = (ImGuiWindowSettings*)entry;
    float x, y;
    int i;
    ImU32 u1;
    if (sscanf(line, "Pos=%f,%f", &x, &y) == 2)                 { settings->Pos = ImVec2(x, y); }
    else if (sscanf(line, "Size=%f,%f", &x, &y) == 2)           { settings->Size = ImMax(ImVec2(x, y), GImGui->Style.WindowMinSize); }
    else if (sscanf(line, "ViewportId=0x%08X", &u1) == 1)       { settings->ViewportId = u1; }
    else if (sscanf(line, "ViewportPos=%f,%f", &x, &y) == 2)    { settings->ViewportPos = ImVec2(x, y); }
    else if (sscanf(line, "Collapsed=%d", &i) == 1)             { settings->Collapsed = (i != 0); }
    else if (sscanf(line, "DockId=0x%X,%d", &u1, &i) == 2)      { settings->DockId = u1; settings->DockOrder = (short)i; }
    else if (sscanf(line, "DockId=0x%X", &u1) == 1)             { settings->DockId = u1; settings->DockOrder = -1; }
    else if (sscanf(line, "ClassId=0x%X", &u1) == 1)            { settings->ClassId = u1; }
}

static void SettingsHandlerWindow_WriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    // Gather data from windows that were active during this session
    // (if a window wasn't opened in this session we preserve its settings)
    ImGuiContext& g = *imgui_ctx;
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Flags & ImGuiWindowFlags_NoSavedSettings)
            continue;

        ImGuiWindowSettings* settings = (window->SettingsIdx != -1) ? &g.SettingsWindows[window->SettingsIdx] : ImGui::FindWindowSettings(window->ID);
        if (!settings)
        {
            settings = ImGui::CreateNewWindowSettings(window->Name);
            window->SettingsIdx = g.SettingsWindows.index_from_ptr(settings);
        }
        IM_ASSERT(settings->ID == window->ID);
        settings->Pos = window->Pos - window->ViewportPos;
        settings->Size = window->SizeFull;
        settings->ViewportId = window->ViewportId;
        settings->ViewportPos = window->ViewportPos;
        IM_ASSERT(window->DockNode == NULL || window->DockNode->ID == window->DockId);
        settings->DockId = window->DockId;
        settings->ClassId = window->WindowClass.ClassId;
        settings->DockOrder = window->DockOrder;
        settings->Collapsed = window->Collapsed;
    }

    // Write to text buffer
    buf->reserve(buf->size() + g.SettingsWindows.Size * 96); // ballpark reserve
    for (int i = 0; i != g.SettingsWindows.Size; i++)
    {
        const ImGuiWindowSettings* settings = &g.SettingsWindows[i];
        const char* name = settings->Name;
        if (const char* p = strstr(name, "###"))  // Skip to the "###" marker if any. We don't skip past to match the behavior of GetID()
            name = p;
        buf->appendf("[%s][%s]\n", handler->TypeName, name);
        if (settings->ViewportId != 0 && settings->ViewportId != ImGui::IMGUI_VIEWPORT_DEFAULT_ID)
        {
            buf->appendf("ViewportPos=%d,%d\n", (int)settings->ViewportPos.x, (int)settings->ViewportPos.y);
            buf->appendf("ViewportId=0x%08X\n", settings->ViewportId);
        }
        if (settings->Pos.x != 0.0f || settings->Pos.y != 0.0f || settings->ViewportId == ImGui::IMGUI_VIEWPORT_DEFAULT_ID)
            buf->appendf("Pos=%d,%d\n", (int)settings->Pos.x, (int)settings->Pos.y);
        if (settings->Size.x != 0.0f || settings->Size.y != 0.0f)
            buf->appendf("Size=%d,%d\n", (int)settings->Size.x, (int)settings->Size.y);
        buf->appendf("Collapsed=%d\n", settings->Collapsed);
        if (settings->DockId != 0)
        {
            // Write DockId as 4 digits if possible. Automatic DockId are small numbers, but full explicit DockSpace() are full ImGuiID range.
            if (settings->DockOrder == -1)
                buf->appendf("DockId=0x%08X\n", settings->DockId);
            else
                buf->appendf("DockId=0x%08X,%d\n", settings->DockId, settings->DockOrder);
            if (settings->ClassId != 0)
                buf->appendf("ClassId=0x%08X\n", settings->ClassId);
        }
        buf->appendf("\n");
    }
}

//-----------------------------------------------------------------------------
// [SECTION] PLATFORM DEPENDENT HELPERS
//-----------------------------------------------------------------------------

#if defined(_WIN32) && !defined(_WINDOWS_) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && (!defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS) || !defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS))
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef __MINGW32__
#include <Windows.h>
#else
#include <windows.h>
#endif
#endif

// Win32 API clipboard implementation
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS)

#ifdef _MSC_VER
#pragma comment(lib, "user32")
#endif

static const char* GetClipboardTextFn_DefaultImpl(void*)
{
    static ImVector<char> buf_local;
    buf_local.clear();
    if (!::OpenClipboard(NULL))
        return NULL;
    HANDLE wbuf_handle = ::GetClipboardData(CF_UNICODETEXT);
    if (wbuf_handle == NULL)
    {
        ::CloseClipboard();
        return NULL;
    }
    if (ImWchar* wbuf_global = (ImWchar*)::GlobalLock(wbuf_handle))
    {
        int buf_len = ImTextCountUtf8BytesFromStr(wbuf_global, NULL) + 1;
        buf_local.resize(buf_len);
        ImTextStrToUtf8(buf_local.Data, buf_len, wbuf_global, NULL);
    }
    ::GlobalUnlock(wbuf_handle);
    ::CloseClipboard();
    return buf_local.Data;
}

static void SetClipboardTextFn_DefaultImpl(void*, const char* text)
{
    if (!::OpenClipboard(NULL))
        return;
    const int wbuf_length = ImTextCountCharsFromUtf8(text, NULL) + 1;
    HGLOBAL wbuf_handle = ::GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wbuf_length * sizeof(ImWchar));
    if (wbuf_handle == NULL)
    {
        ::CloseClipboard();
        return;
    }
    ImWchar* wbuf_global = (ImWchar*)::GlobalLock(wbuf_handle);
    ImTextStrFromUtf8(wbuf_global, wbuf_length, text, NULL);
    ::GlobalUnlock(wbuf_handle);
    ::EmptyClipboard();
    if (::SetClipboardData(CF_UNICODETEXT, wbuf_handle) == NULL)
        ::GlobalFree(wbuf_handle);
    ::CloseClipboard();
}

#else

// Local ImGui-only clipboard implementation, if user hasn't defined better clipboard handlers
static const char* GetClipboardTextFn_DefaultImpl(void*)
{
    ImGuiContext& g = *GImGui;
    return g.PrivateClipboard.empty() ? NULL : g.PrivateClipboard.begin();
}

// Local ImGui-only clipboard implementation, if user hasn't defined better clipboard handlers
static void SetClipboardTextFn_DefaultImpl(void*, const char* text)
{
    ImGuiContext& g = *GImGui;
    g.PrivateClipboard.clear();
    const char* text_end = text + strlen(text);
    g.PrivateClipboard.resize((int)(text_end - text) + 1);
    memcpy(&g.PrivateClipboard[0], text, (size_t)(text_end - text));
    g.PrivateClipboard[(int)(text_end - text)] = 0;
}

#endif

//-----------------------------------------------------------------------------
// [SECTION] METRICS/DEBUG WINDOW
//-----------------------------------------------------------------------------

static void RenderViewportThumbnail(ImDrawList* draw_list, ImGuiViewportP* viewport, const ImRect& bb)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImVec2 scale = bb.GetSize() / viewport->Size;
    ImVec2 off = bb.Min - viewport->Pos * scale;
    float alpha_mul = viewport->PlatformWindowMinimized ? 0.30f : 1.00f;
    window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border, alpha_mul * 0.40f));
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* thumb_window = g.Windows[i];
        if (!thumb_window->WasActive || ((thumb_window->Flags & ImGuiWindowFlags_ChildWindow)))
            continue;
        if (thumb_window->SkipItems && (thumb_window->Flags & ImGuiWindowFlags_ChildWindow)) // FIXME-DOCK: Skip hidden docked windows. Identify those betters.
            continue;
        if (thumb_window->Viewport != viewport)
            continue;

        ImRect thumb_r = thumb_window->Rect();
        ImRect title_r = thumb_window->TitleBarRect();
        ImRect thumb_r_scaled = ImRect(ImFloor(off + thumb_r.Min * scale), ImFloor(off +  thumb_r.Max * scale));
        ImRect title_r_scaled = ImRect(ImFloor(off + title_r.Min * scale), ImFloor(off +  ImVec2(title_r.Max.x, title_r.Min.y) * scale) + ImVec2(0,5)); // Exaggerate title bar height
        thumb_r_scaled.ClipWithFull(bb);
        title_r_scaled.ClipWithFull(bb);
        const bool window_is_focused = (g.NavWindow && thumb_window->RootWindowForTitleBarHighlight == g.NavWindow->RootWindowForTitleBarHighlight);
        window->DrawList->AddRectFilled(thumb_r_scaled.Min, thumb_r_scaled.Max, ImGui::GetColorU32(ImGuiCol_WindowBg, alpha_mul));
        window->DrawList->AddRectFilled(title_r_scaled.Min, title_r_scaled.Max, ImGui::GetColorU32(window_is_focused ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg, alpha_mul));
        window->DrawList->AddRect(thumb_r_scaled.Min, thumb_r_scaled.Max, ImGui::GetColorU32(ImGuiCol_Border, alpha_mul));
        if (ImGuiWindow* window_for_title = GetWindowForTitleDisplay(thumb_window))
            window->DrawList->AddText(g.Font, g.FontSize * 1.0f, title_r_scaled.Min, ImGui::GetColorU32(ImGuiCol_Text, alpha_mul), window_for_title->Name, ImGui::FindRenderedTextEnd(window_for_title->Name));
    }
    draw_list->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border, alpha_mul));
}

void ImGui::ShowViewportThumbnails()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    // We don't display full monitor bounds (we could, but it often looks awkward), instead we display just enough to cover all of our viewports.
    float SCALE = 1.0f / 8.0f;
    ImRect bb_full;
    //for (int n = 0; n < g.PlatformIO.Monitors.Size; n++)
    //    bb_full.Add(GetPlatformMonitorMainRect(g.PlatformIO.Monitors[n]));
    for (int n = 0; n < g.Viewports.Size; n++)
        bb_full.Add(g.Viewports[n]->GetRect());
    ImVec2 p = window->DC.CursorPos;
    ImVec2 off = p - bb_full.Min * SCALE;
    //for (int n = 0; n < g.PlatformIO.Monitors.Size; n++)
    //    window->DrawList->AddRect(off + g.PlatformIO.Monitors[n].MainPos * SCALE, off + (g.PlatformIO.Monitors[n].MainPos + g.PlatformIO.Monitors[n].MainSize) * SCALE, ImGui::GetColorU32(ImGuiCol_Border));
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        ImRect viewport_draw_bb(off + (viewport->Pos) * SCALE, off + (viewport->Pos + viewport->Size) * SCALE);
        RenderViewportThumbnail(window->DrawList, viewport, viewport_draw_bb);
    }
    ImGui::Dummy(bb_full.GetSize() * SCALE);
}

void ImGui::ShowMetricsWindow(bool* p_open)
{
    if (!ImGui::Begin("ImGui Metrics", p_open))
    {
        ImGui::End();
        return;
    }

    static bool show_draw_cmd_clip_rects = true;
    static bool show_window_begin_order = false;
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);
    ImGui::Text("%d active windows (%d visible)", io.MetricsActiveWindows, io.MetricsRenderWindows);
    ImGui::Text("%d allocations", io.MetricsActiveAllocations);
    ImGui::Checkbox("Show clipping rectangles when hovering draw commands", &show_draw_cmd_clip_rects);
    ImGui::Checkbox("Ctrl shows window begin order", &show_window_begin_order);
    ImGui::Separator();

    struct Funcs
    {
        static void NodeDrawList(ImGuiWindow* window, ImGuiViewportP* viewport, ImDrawList* draw_list, const char* label)
        {
            bool node_open = ImGui::TreeNode(draw_list, "%s: '%s' %d vtx, %d indices, %d cmds", label, draw_list->_OwnerName ? draw_list->_OwnerName : "", draw_list->VtxBuffer.Size, draw_list->IdxBuffer.Size, draw_list->CmdBuffer.Size);
            if (draw_list == ImGui::GetWindowDrawList())
            {
                ImGui::SameLine();
                ImGui::TextColored(ImColor(255,100,100), "CURRENTLY APPENDING"); // Can't display stats for active draw list! (we don't have the data double-buffered)
                if (node_open) ImGui::TreePop();
                return;
            }

            ImDrawList* overlay_draw_list = viewport ? GetOverlayDrawList(viewport) : NULL; // Render additional visuals into the top-most draw list
            if (window && overlay_draw_list && ImGui::IsItemHovered())
                overlay_draw_list->AddRect(window->Pos, window->Pos + window->Size, IM_COL32(255, 255, 0, 255));
            if (!node_open)
                return;

            int elem_offset = 0;
            for (const ImDrawCmd* pcmd = draw_list->CmdBuffer.begin(); pcmd < draw_list->CmdBuffer.end(); elem_offset += pcmd->ElemCount, pcmd++)
            {
                if (pcmd->UserCallback == NULL && pcmd->ElemCount == 0)
                    continue;
                if (pcmd->UserCallback)
                {
                    ImGui::BulletText("Callback %p, user_data %p", pcmd->UserCallback, pcmd->UserCallbackData);
                    continue;
                }
                ImDrawIdx* idx_buffer = (draw_list->IdxBuffer.Size > 0) ? draw_list->IdxBuffer.Data : NULL;
                bool pcmd_node_open = ImGui::TreeNode((void*)(pcmd - draw_list->CmdBuffer.begin()), "Draw %4d %s vtx, tex 0x%p, clip_rect (%4.0f,%4.0f)-(%4.0f,%4.0f)", pcmd->ElemCount, draw_list->IdxBuffer.Size > 0 ? "indexed" : "non-indexed", pcmd->TextureId, pcmd->ClipRect.x, pcmd->ClipRect.y, pcmd->ClipRect.z, pcmd->ClipRect.w);
                if (show_draw_cmd_clip_rects && overlay_draw_list && ImGui::IsItemHovered())
                {
                    ImRect clip_rect = pcmd->ClipRect;
                    ImRect vtxs_rect;
                    for (int i = elem_offset; i < elem_offset + (int)pcmd->ElemCount; i++)
                        vtxs_rect.Add(draw_list->VtxBuffer[idx_buffer ? idx_buffer[i] : i].pos);
                    clip_rect.Floor(); overlay_draw_list->AddRect(clip_rect.Min, clip_rect.Max, IM_COL32(255,255,0,255));
                    vtxs_rect.Floor(); overlay_draw_list->AddRect(vtxs_rect.Min, vtxs_rect.Max, IM_COL32(255,0,255,255));
                }
                if (!pcmd_node_open)
                    continue;

                // Display individual triangles/vertices. Hover on to get the corresponding triangle highlighted.
                ImGuiListClipper clipper(pcmd->ElemCount/3); // Manually coarse clip our print out of individual vertices to save CPU, only items that may be visible.
                while (clipper.Step())
                    for (int prim = clipper.DisplayStart, vtx_i = elem_offset + clipper.DisplayStart*3; prim < clipper.DisplayEnd; prim++)
                    {
                        char buf[300];
                        char *buf_p = buf, *buf_end = buf + IM_ARRAYSIZE(buf);
                        ImVec2 triangles_pos[3];
                        for (int n = 0; n < 3; n++, vtx_i++)
                        {
                            ImDrawVert& v = draw_list->VtxBuffer[idx_buffer ? idx_buffer[vtx_i] : vtx_i];
                            triangles_pos[n] = v.pos;
                            buf_p += ImFormatString(buf_p, (int)(buf_end - buf_p), "%s %04d: pos (%8.2f,%8.2f), uv (%.6f,%.6f), col %08X\n", (n == 0) ? "vtx" : "   ", vtx_i, v.pos.x, v.pos.y, v.uv.x, v.uv.y, v.col);
                        }
                        ImGui::Selectable(buf, false);
                        if (overlay_draw_list && ImGui::IsItemHovered())
                        {
                            ImDrawListFlags backup_flags = overlay_draw_list->Flags;
                            overlay_draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines; // Disable AA on triangle outlines at is more readable for very large and thin triangles.
                            overlay_draw_list->AddPolyline(triangles_pos, 3, IM_COL32(255,255,0,255), true, 1.0f);
                            overlay_draw_list->Flags = backup_flags;
                        }
                    }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        static void NodeWindows(ImVector<ImGuiWindow*>& windows, const char* label)
        {
            if (!ImGui::TreeNode(label, "%s (%d)", label, windows.Size))
                return;
            for (int i = 0; i < windows.Size; i++)
                Funcs::NodeWindow(windows[i], "Window");
            ImGui::TreePop();
        }

        static void NodeWindow(ImGuiWindow* window, const char* label)
        {
            if (!ImGui::TreeNode(window, "%s '%s', %d @ 0x%p", label, window->Name, window->Active || window->WasActive, window))
                return;
            ImGuiWindowFlags flags = window->Flags;
            NodeDrawList(window, window->Viewport, window->DrawList, "DrawList");
            ImGui::BulletText("Pos: (%.1f,%.1f), Size: (%.1f,%.1f), SizeContents (%.1f,%.1f)", window->Pos.x, window->Pos.y, window->Size.x, window->Size.y, window->SizeContents.x, window->SizeContents.y);
            ImGui::BulletText("Flags: 0x%08X (%s%s%s%s%s%s%s%s%s..)", flags,
                (flags & ImGuiWindowFlags_ChildWindow)  ? "Child " : "",      (flags & ImGuiWindowFlags_Tooltip)     ? "Tooltip "   : "",  (flags & ImGuiWindowFlags_Popup) ? "Popup " : "",
                (flags & ImGuiWindowFlags_Modal)        ? "Modal " : "",      (flags & ImGuiWindowFlags_ChildMenu)   ? "ChildMenu " : "",  (flags & ImGuiWindowFlags_NoSavedSettings) ? "NoSavedSettings " : "",
                (flags & ImGuiWindowFlags_NoMouseInputs)? "NoMouseInputs":"", (flags & ImGuiWindowFlags_NoNavInputs) ? "NoNavInputs" : "", (flags & ImGuiWindowFlags_AlwaysAutoResize) ? "AlwaysAutoResize" : "");
            ImGui::BulletText("Scroll: (%.2f/%.2f,%.2f/%.2f)", window->Scroll.x, GetWindowScrollMaxX(window), window->Scroll.y, GetWindowScrollMaxY(window));
            ImGui::BulletText("Active: %d/%d, WriteAccessed: %d, BeginOrderWithinContext: %d", window->Active, window->WasActive, window->WriteAccessed, (window->Active || window->WasActive) ? window->BeginOrderWithinContext : -1);
            ImGui::BulletText("Appearing: %d, Hidden: %d (Reg %d Resize %d), SkipItems: %d", window->Appearing, window->Hidden, window->HiddenFramesRegular, window->HiddenFramesForResize, window->SkipItems);
            ImGui::BulletText("NavLastIds: 0x%08X,0x%08X, NavLayerActiveMask: %X", window->NavLastIds[0], window->NavLastIds[1], window->DC.NavLayerActiveMask);
            ImGui::BulletText("NavLastChildNavWindow: %s", window->NavLastChildNavWindow ? window->NavLastChildNavWindow->Name : "NULL");
            if (!window->NavRectRel[0].IsInverted())
                ImGui::BulletText("NavRectRel[0]: (%.1f,%.1f)(%.1f,%.1f)", window->NavRectRel[0].Min.x, window->NavRectRel[0].Min.y, window->NavRectRel[0].Max.x, window->NavRectRel[0].Max.y);
            else
                ImGui::BulletText("NavRectRel[0]: <None>");
            ImGui::BulletText("Viewport: %d%s, ViewportId: 0x%08X, ViewportPos: (%.1f,%.1f)", window->Viewport ? window->Viewport->Idx : -1, window->ViewportOwned ? " (Owned)" : "", window->ViewportId, window->ViewportPos.x, window->ViewportPos.y);
            ImGui::BulletText("ViewportMonitor: %d", window->Viewport ? window->Viewport->PlatformMonitor : -1);
            ImGui::BulletText("DockId: 0x%04X, DockOrder: %d, %s: 0x%p, Act: %d, Vis: %d", window->DockId, window->DockOrder, window->DockNodeAsHost ? "DockNodeAsHost" : "DockNode", window->DockNodeAsHost ? window->DockNodeAsHost : window->DockNode, window->DockIsActive, window->DockTabIsVisible);
            if (window->RootWindow != window) NodeWindow(window->RootWindow, "RootWindow");
            if (window->RootWindowDockStop != window->RootWindow) NodeWindow(window->RootWindowDockStop, "RootWindowDockStop");
            if (window->ParentWindow != NULL) NodeWindow(window->ParentWindow, "ParentWindow");
            if (window->DC.ChildWindows.Size > 0) NodeWindows(window->DC.ChildWindows, "ChildWindows");
            if (window->ColumnsStorage.Size > 0 && ImGui::TreeNode("Columns", "Columns sets (%d)", window->ColumnsStorage.Size))
            {
                for (int n = 0; n < window->ColumnsStorage.Size; n++)
                {
                    const ImGuiColumnsSet* columns = &window->ColumnsStorage[n];
                    if (ImGui::TreeNode((void*)(uintptr_t)columns->ID, "Columns Id: 0x%08X, Count: %d, Flags: 0x%04X", columns->ID, columns->Count, columns->Flags))
                    {
                        ImGui::BulletText("Width: %.1f (MinX: %.1f, MaxX: %.1f)", columns->MaxX - columns->MinX, columns->MinX, columns->MaxX);
                        for (int column_n = 0; column_n < columns->Columns.Size; column_n++)
                            ImGui::BulletText("Column %02d: OffsetNorm %.3f (= %.1f px)", column_n, columns->Columns[column_n].OffsetNorm, OffsetNormToPixels(columns, columns->Columns[column_n].OffsetNorm));
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
            ImGui::BulletText("Storage: %d bytes", window->StateStorage.Data.Size * (int)sizeof(ImGuiStorage::Pair));
            ImGui::TreePop();
        }

        static void NodeViewport(ImGuiViewportP* viewport)
        {
            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode((void*)(intptr_t)viewport->ID, "Viewport #%d, ID: 0x%08X, Parent: 0x%08X, Window: \"%s\"", viewport->Idx, viewport->ID, viewport->ParentViewportId, viewport->Window ? viewport->Window->Name : "N/A"))
            {
                ImGuiWindowFlags flags = viewport->Flags;
                ImGui::BulletText("Pos: (%.0f,%.0f), Size: (%.0f,%.0f), Monitor: %d, DpiScale: %.0f%%", viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y, viewport->PlatformMonitor, viewport->DpiScale * 100.0f);
                if (viewport->Idx > 0) { ImGui::SameLine(); if (ImGui::SmallButton("Reset Pos")) { viewport->Pos = ImVec2(200,200); if (viewport->Window) viewport->Window->Pos = ImVec2(200,200); } }
                ImGui::BulletText("Flags: 0x%04X =%s%s%s%s%s%s", viewport->Flags,
                    (flags & ImGuiViewportFlags_CanHostOtherWindows) ? " CanHostOtherWindows" : "", (flags & ImGuiViewportFlags_NoDecoration) ? " NoDecoration" : "",
                    (flags & ImGuiViewportFlags_NoFocusOnAppearing)  ? " NoFocusOnAppearing"  : "", (flags & ImGuiViewportFlags_NoInputs)     ? " NoInputs"     : "",
                    (flags & ImGuiViewportFlags_NoRendererClear)     ? " NoRendererClear"     : "", viewport->PlatformWindowMinimized ? ", PlatformWindowMinimized" : "");
                for (int layer_i = 0; layer_i < IM_ARRAYSIZE(viewport->DrawDataBuilder.Layers); layer_i++)
                    for (int draw_list_i = 0; draw_list_i < viewport->DrawDataBuilder.Layers[layer_i].Size; draw_list_i++)
                        Funcs::NodeDrawList(NULL, viewport, viewport->DrawDataBuilder.Layers[layer_i][draw_list_i], "DrawList");
                ImGui::TreePop();
            }
        }
    };

    // Access private state, we are going to display the draw lists from last frame
    ImGuiContext& g = *GImGui;
    Funcs::NodeWindows(g.Windows, "Windows");
    if (ImGui::TreeNode("Viewport", "Viewports (%d)", g.Viewports.Size))
    {
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::ShowViewportThumbnails();
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        if (g.PlatformIO.Monitors.Size > 0 && ImGui::TreeNode("Monitors", "Monitors (%d)", g.PlatformIO.Monitors.Size))
        {
            ImGui::TextWrapped("(When viewports are enabled, imgui needs uses monitor data to position popup/tooltips so they don't straddle monitors.)");
            for (int i = 0; i < g.PlatformIO.Monitors.Size; i++)
            {
                const ImGuiPlatformMonitor& mon = g.PlatformIO.Monitors[i];
                ImGui::BulletText("Monitor #%d: DPI %.0f%%\n MainMin (%.0f,%.0f), MainMax (%.0f,%.0f), MainSize (%.0f,%.0f)\n WorkMin (%.0f,%.0f), WorkMax (%.0f,%.0f), WorkSize (%.0f,%.0f)", 
                    i, mon.DpiScale * 100.0f, 
                    mon.MainPos.x, mon.MainPos.y, mon.MainPos.x + mon.MainSize.x, mon.MainPos.y + mon.MainSize.y, mon.MainSize.x, mon.MainSize.y,
                    mon.WorkPos.x, mon.WorkPos.y, mon.WorkPos.x + mon.WorkSize.x, mon.WorkPos.y + mon.WorkSize.y, mon.WorkSize.x, mon.WorkSize.y);
            }
            ImGui::TreePop();
        }
        for (int i = 0; i < g.Viewports.Size; i++)
            Funcs::NodeViewport(g.Viewports[i]);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Popups", "Popups (%d)", g.OpenPopupStack.Size))
    {
        for (int i = 0; i < g.OpenPopupStack.Size; i++)
        {
            ImGuiWindow* window = g.OpenPopupStack[i].Window;
            ImGui::BulletText("PopupID: %08x, Window: '%s'%s%s", g.OpenPopupStack[i].PopupId, window ? window->Name : "NULL", window && (window->Flags & ImGuiWindowFlags_ChildWindow) ? " ChildWindow" : "", window && (window->Flags & ImGuiWindowFlags_ChildMenu) ? " ChildMenu" : "");
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Docking & Tab Bars"))
    {
        ShowDockingDebug();
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Internal state"))
    {
        const char* input_source_names[] = { "None", "Mouse", "Nav", "NavKeyboard", "NavGamepad" }; IM_ASSERT(IM_ARRAYSIZE(input_source_names) == ImGuiInputSource_COUNT);
        ImGui::Text("HoveredWindow: '%s'", g.HoveredWindow ? g.HoveredWindow->Name : "NULL");
        ImGui::Text("HoveredRootWindow: '%s'", g.HoveredRootWindow ? g.HoveredRootWindow->Name : "NULL");
        ImGui::Text("HoveredWindowUnderMovingWindow: '%s'", g.HoveredWindowUnderMovingWindow ? g.HoveredWindowUnderMovingWindow->Name : "NULL");
        ImGui::Text("HoveredId: 0x%08X/0x%08X (%.2f sec), AllowOverlap: %d", g.HoveredId, g.HoveredIdPreviousFrame, g.HoveredIdTimer, g.HoveredIdAllowOverlap); // Data is "in-flight" so depending on when the Metrics window is called we may see current frame information or not
        ImGui::Text("ActiveId: 0x%08X/0x%08X (%.2f sec), AllowOverlap: %d, Source: %s", g.ActiveId, g.ActiveIdPreviousFrame, g.ActiveIdTimer, g.ActiveIdAllowOverlap, input_source_names[g.ActiveIdSource]);
        ImGui::Text("ActiveIdWindow: '%s'", g.ActiveIdWindow ? g.ActiveIdWindow->Name : "NULL");
        ImGui::Text("MovingWindow: '%s'", g.MovingWindow ? g.MovingWindow->Name : "NULL");
        ImGui::Text("NavWindow: '%s'", g.NavWindow ? g.NavWindow->Name : "NULL");
        ImGui::Text("NavId: 0x%08X, NavLayer: %d", g.NavId, g.NavLayer);
        ImGui::Text("NavInputSource: %s", input_source_names[g.NavInputSource]);
        ImGui::Text("NavActive: %d, NavVisible: %d", g.IO.NavActive, g.IO.NavVisible);
        ImGui::Text("NavActivateId: 0x%08X, NavInputId: 0x%08X", g.NavActivateId, g.NavInputId);
        ImGui::Text("NavDisableHighlight: %d, NavDisableMouseHover: %d", g.NavDisableHighlight, g.NavDisableMouseHover);
        ImGui::Text("NavWindowingTarget: '%s'", g.NavWindowingTarget ? g.NavWindowingTarget->Name : "NULL");
        ImGui::Text("DragDrop: %d, SourceId = 0x%08X, Payload \"%s\" (%d bytes)", g.DragDropActive, g.DragDropPayload.SourceId, g.DragDropPayload.DataType, g.DragDropPayload.DataSize);
        ImGui::Text("MouseViewport: 0x%08X (UserHovered 0x%08X, LastHovered 0x%08X)", g.MouseViewport->ID, g.IO.MouseHoveredViewport, g.MouseLastHoveredViewport ? g.MouseLastHoveredViewport->ID : 0);
        ImGui::TreePop();
    }

    if (g.IO.KeyCtrl && show_window_begin_order)
    {
        for (int n = 0; n < g.Windows.Size; n++)
        {
            ImGuiWindow* window = g.Windows[n];
            if (!window->WasActive || ((window->Flags & ImGuiWindowFlags_ChildWindow) && window->DockNode == NULL))
                continue;

            char buf[64] = "";
            char* p = buf;
            p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "Order: %d\n", window->BeginOrderWithinContext);
            ImDrawList* overlay_draw_list = GetOverlayDrawList(window->Viewport);
            overlay_draw_list->AddRectFilled(window->Pos - ImVec2(1, 1), window->Pos + CalcTextSize(buf) + ImVec2(1, 1), IM_COL32(200, 100, 100, 255));
            overlay_draw_list->AddText(NULL, 0.0f, window->Pos, IM_COL32(255, 255, 255, 255), buf);
        }
    }
    ImGui::End();
}

void ImGui::ShowDockingDebug()
{
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = ctx->DockContext;

    struct Funcs
    {
        static void NodeDockNode(ImGuiDockNode* node, const char* label)
        {
            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            bool open; 
            if (node->Windows.Size > 0)
                open = ImGui::TreeNode((void*)(intptr_t)node->ID, "%s 0x%04X%s: %d windows (vis: '%s')", label, node->ID, node->IsVisible ? "" : " (hidden)", node->Windows.Size, node->VisibleWindow ? node->VisibleWindow->Name : "NULL");
            else
                open = ImGui::TreeNode((void*)(intptr_t)node->ID, "%s 0x%04X%s: split %s (act: '%s')", label, node->ID, node->IsVisible ? "" : " (hidden)", (node->SplitAxis == ImGuiAxis_X) ? "horizontal" : (node->SplitAxis == ImGuiAxis_Y) ? "vertical" : "n/a", node->VisibleWindow ? node->VisibleWindow->Name : "NULL");
            if (open)
            {
                IM_ASSERT(node->ChildNodes[0] == NULL || node->ChildNodes[0]->ParentNode == node);
                IM_ASSERT(node->ChildNodes[1] == NULL || node->ChildNodes[1]->ParentNode == node);
                ImGui::BulletText("Pos (%.0f,%.0f), Size (%.0f, %.0f) Ref (%.0f, %.0f)",
                    node->Pos.x, node->Pos.y, node->Size.x, node->Size.y, node->SizeRef.x, node->SizeRef.y);
                ImGui::BulletText("Flags 0x%02X%s%s%s%s",
                    node->Flags, node->IsDockSpace ? ", IsDockSpace" : "", node->IsCentralNode ? ", IsCentralNode" : "",
                    (GImGui->FrameCount - node->LastFrameAlive < 2) ? ", IsAlive" : "", (GImGui->FrameCount - node->LastFrameActive < 2) ? ", IsActive" : "");
                if (node->ChildNodes[0])
                    NodeDockNode(node->ChildNodes[0], "Child[0]");
                if (node->ChildNodes[1])
                    NodeDockNode(node->ChildNodes[1], "Child[1]");
                if (node->TabBar)
                    NodeTabBar(node->TabBar);
                ImGui::TreePop();
            }
        }

        static void NodeTabBar(ImGuiTabBar* tab_bar)
        {
            // Note that standalone tab bars (not associated to docking/windows functionality) currently hold no discernable strings.
            char buf[256];
            char* p = buf;
            const char* buf_end = buf + IM_ARRAYSIZE(buf);
            p += ImFormatString(p, buf_end - p, "TabBar (%d tabs)%s", 
                tab_bar->Tabs.Size, (tab_bar->PrevFrameVisible < ImGui::GetFrameCount() - 2) ? " *Inactive*" : "");
            if (tab_bar->Flags & ImGuiTabBarFlags_DockNode)
            {
                p += ImFormatString(p, buf_end - p, "  { ");
                for (int tab_n = 0; tab_n < ImMin(tab_bar->Tabs.Size, 3); tab_n++)
                    p += ImFormatString(p, buf_end - p, "%s'%s'", tab_n > 0 ? ", " : "", tab_bar->Tabs[tab_n].Window->Name);
                p += ImFormatString(p, buf_end - p, (tab_bar->Tabs.Size > 3) ? " ... }" : " } ");
            }
            if (ImGui::TreeNode(tab_bar, "%s", buf))
            {
                for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
                {
                    const ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
                    ImGui::PushID(tab);
                    if (ImGui::SmallButton("<")) { TabBarQueueChangeTabOrder(tab_bar, tab, -1); } ImGui::SameLine(0, 2);
                    if (ImGui::SmallButton(">")) { TabBarQueueChangeTabOrder(tab_bar, tab, +1); } ImGui::SameLine();
                    ImGui::Text("%02d%c Tab 0x%08X '%s'", tab_n, (tab->ID == tab_bar->SelectedTabId) ? '*' : ' ', tab->ID, tab->Window ? tab->Window->Name : "N/A");
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }
    };

    static bool show_window_dock_info = false;
    ImGui::Checkbox("Ctrl shows window dock info", &show_window_dock_info);

    if (ImGui::TreeNode("Dock nodes"))
    {
        if (ImGui::SmallButton("Clear settings"))   { DockContextClearNodes(&g, 0, true); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Rebuild all"))      { dc->WantFullRebuild = true; }
        for (int n = 0; n < dc->Nodes.Data.Size; n++)
            if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
                if (node->IsRootNode())
                    Funcs::NodeDockNode(node, "Node");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("TabBars", "Tab Bars (%d)", g.TabBars.Data.Size))
    {
        for (int n = 0; n < g.TabBars.Data.Size; n++)
            Funcs::NodeTabBar(g.TabBars.GetByIndex(n));
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Settings"))
    {
        if (ImGui::SmallButton("Refresh"))
            SaveIniSettingsToMemory();
        ImGui::SameLine();
        if (ImGui::SmallButton("Save to disk"))
            SaveIniSettingsToDisk(g.IO.IniFilename);
        ImGui::Separator();
        ImGui::Text("Docked Windows:");
        for (int n = 0; n < g.SettingsWindows.Size; n++)
            if (g.SettingsWindows[n].DockId != 0)
                ImGui::BulletText("Window '%s' -> DockId %08X", g.SettingsWindows[n].Name, g.SettingsWindows[n].DockId);
        ImGui::Separator();
        ImGui::Text("Dock Nodes:");
        for (int n = 0; n < dc->SettingsNodes.Size; n++)
        {
            ImGuiDockNodeSettings* settings = &dc->SettingsNodes[n];
            const char* selected_tab_name = NULL;
            if (settings->SelectedTabID)
            {
                if (ImGuiWindow* window = FindWindowByID(settings->SelectedTabID))
                    selected_tab_name = window->Name;
                else if (ImGuiWindowSettings* window_settings = FindWindowSettings(settings->SelectedTabID))
                    selected_tab_name = window_settings->Name;
            }
            ImGui::BulletText("Node %08X, Parent %08X, SelectedTab %08X ('%s')", settings->ID, settings->ParentID, settings->SelectedTabID, selected_tab_name ? selected_tab_name : settings->SelectedTabID ? "N/A" : "");
        }
        ImGui::TreePop();
    }

    if (g.IO.KeyCtrl && show_window_dock_info)
    {
        for (int n = 0; n < dc->Nodes.Data.Size; n++)
            if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
            {
                ImGuiDockNode* root_node = DockNodeGetRootNode(node);
                if (ImGuiDockNode* hovered_node = DockNodeTreeFindNodeByPos(root_node, g.IO.MousePos))
                    if (hovered_node != node)
                        continue;
                char buf[64] = "";
                char* p = buf;
                ImDrawList* overlay_draw_list = node->HostWindow ? GetOverlayDrawList(node->HostWindow) : GetOverlayDrawList((ImGuiViewportP*)GetMainViewport());
                p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "DockId: %X%s\n", node->ID, node->IsCentralNode ? " *CentralNode*" : "");
                p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "Size: (%.0f, %.0f)\n", node->Size.x, node->Size.y);
                p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "SizeRef: (%.0f, %.0f)\n", node->SizeRef.x, node->SizeRef.y);
                int depth = DockNodeGetDepth(node);
                overlay_draw_list->AddRect(node->Pos + ImVec2(3,3) * (float)depth, node->Pos + node->Size - ImVec2(3,3) * (float)depth, IM_COL32(200, 100, 100, 255));
                ImVec2 pos = node->Pos + ImVec2(3,3) * (float)depth;
                overlay_draw_list->AddRectFilled(pos - ImVec2(1, 1), pos + CalcTextSize(buf) + ImVec2(1, 1), IM_COL32(200, 100, 100, 255));
                overlay_draw_list->AddText(NULL, 0.0f, pos, IM_COL32(255, 255, 255, 255), buf);
            }
    }
}

//-----------------------------------------------------------------------------


// ImGui Bezier widget. @r-lyeh, public domain
// v1.02: add BezierValue(); comments; usage
// v1.01: out-of-bounds coord snapping; custom border width; spacing; cosmetics
// v1.00: initial version
//
// [ref] http://robnapier.net/faster-bezier
// [ref] http://easings.net/es#easeInSine
//
// Usage:
// {  static float v[] = { 0.390f, 0.575f, 0.565f, 1.000f }; 
//    ImGui::Bezier( "easeOutSine", v );       // draw
//    float y = ImGui::BezierValue( 0.5f, v ); // x delta in [0..1] range
// }

//#define IMGUI_DEFINE_MATH_OPERATORS
//#include <imgui.h>
//#include <imgui_internal.h>

namespace ImGui
{
	template<int steps>
	void bezier_table(ImVec2 P[4], ImVec2 results[steps + 1]) {
		static float C[(steps + 1) * 4], *K = 0;
		if (!K) {
			K = C;
			for (unsigned step = 0; step <= steps; ++step) {
				float t = (float)step / (float)steps;
				C[step * 4 + 0] = (1 - t)*(1 - t)*(1 - t);   // * P0
				C[step * 4 + 1] = 3 * (1 - t)*(1 - t) * t; // * P1
				C[step * 4 + 2] = 3 * (1 - t) * t*t;     // * P2
				C[step * 4 + 3] = t * t*t;               // * P3
			}
		}
		for (unsigned step = 0; step <= steps; ++step) {
			ImVec2 point = {
				K[step * 4 + 0] * P[0].x + K[step * 4 + 1] * P[1].x + K[step * 4 + 2] * P[2].x + K[step * 4 + 3] * P[3].x,
				K[step * 4 + 0] * P[0].y + K[step * 4 + 1] * P[1].y + K[step * 4 + 2] * P[2].y + K[step * 4 + 3] * P[3].y
			};
			results[step] = point;
		}
	}

	float BezierValue(float dt01, float P[4]) {
		enum { STEPS = 256 };
		ImVec2 Q[4] = { { 0, 0 }, { P[0], P[1] }, { P[2], P[3] }, { 1, 1 } };
		ImVec2 results[STEPS + 1];
		bezier_table<STEPS>(Q, results);
		return results[(int)((dt01 < 0 ? 0 : dt01 > 1 ? 1 : dt01) * STEPS)].y;
	}

	int Bezier(const char *label, float P[4]) {
		// visuals
		enum { SMOOTHNESS = 64 }; // curve smoothness: the higher number of segments, the smoother curve
		enum { CURVE_WIDTH = 4 }; // main curved line width
		enum { LINE_WIDTH = 1 }; // handlers: small lines width
		enum { GRAB_RADIUS = 6 }; // handlers: circle radius
		enum { GRAB_BORDER = 2 }; // handlers: circle border width

		const ImGuiStyle& Style = GetStyle();
		//const ImGuiIO& IO = GetIO();
		ImDrawList* DrawList = GetWindowDrawList();
		ImGuiWindow* Window = GetCurrentWindow();
		if (Window->SkipItems)
			return false;

		// header and spacing
		int changed = SliderFloat4(label, P, 0, 1, "%.3f", 1.0f);
		int hovered = IsItemActive() || IsItemHovered(); // IsItemDragged() ?
		Dummy(ImVec2(0, 3));

		// prepare canvas
		const float avail = GetContentRegionAvailWidth();
		const float dim = ImMin(avail, 128.f);
		ImVec2 Canvas(dim, dim);

		ImRect bb(Window->DC.CursorPos, Window->DC.CursorPos + Canvas);
		ItemSize(bb);
		if (!ItemAdd(bb, NULL))
			return changed;

		//const ImGuiID id = Window->GetID(label);
		//hovered |= 0 != IsHovered(ImRect(bb.Min, bb.Min + ImVec2(avail, dim)), id);

		RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg, 1), true, Style.FrameRounding);

		// background grid
		for (int i = 0; i <= Canvas.x; i += static_cast<int>(Canvas.x / 4)) {
			DrawList->AddLine(
				ImVec2(bb.Min.x + i, bb.Min.y),
				ImVec2(bb.Min.x + i, bb.Max.y),
				GetColorU32(ImGuiCol_TextDisabled));
		}
		for (int i = 0; i <= Canvas.y; i += static_cast<int>(Canvas.y / 4)) {
			DrawList->AddLine(
				ImVec2(bb.Min.x, bb.Min.y + i),
				ImVec2(bb.Max.x, bb.Min.y + i),
				GetColorU32(ImGuiCol_TextDisabled));
		}

		// eval curve
		ImVec2 Q[4] = { { 0, 0 }, { P[0], P[1] }, { P[2], P[3] }, { 1, 1 } };
		ImVec2 results[SMOOTHNESS + 1];
		bezier_table<SMOOTHNESS>(Q, results);

		// control points: 2 lines and 2 circles
		{
			char buf[128];
			sprintf(buf, "0##%s", label);

			// handle grabbers
			for (int i = 0; i < 2; ++i)
			{
				ImVec2 pos = ImVec2(P[i * 2 + 0], 1 - P[i * 2 + 1]) * (bb.Max - bb.Min) + bb.Min;
				SetCursorScreenPos(pos - ImVec2(GRAB_RADIUS, GRAB_RADIUS));
				InvisibleButton((buf[0]++, buf), ImVec2(2 * GRAB_RADIUS, 2 * GRAB_RADIUS));
				if (IsItemActive() || IsItemHovered())
				{
					SetTooltip("(%4.3f, %4.3f)", P[i * 2 + 0], P[i * 2 + 1]);
				}
				if (IsItemActive() && IsMouseDragging(0))
				{
					P[i * 2 + 0] += GetIO().MouseDelta.x / Canvas.x;
					P[i * 2 + 1] -= GetIO().MouseDelta.y / Canvas.y;
					changed = true;
				}
			}

			if (hovered || changed) DrawList->PushClipRectFullScreen();

			// draw curve
			{
				ImColor color(GetStyle().Colors[ImGuiCol_PlotLines]);
				for (int i = 0; i < SMOOTHNESS; ++i) {
					ImVec2 p = { results[i + 0].x, 1 - results[i + 0].y };
					ImVec2 q = { results[i + 1].x, 1 - results[i + 1].y };
					ImVec2 r(p.x * (bb.Max.x - bb.Min.x) + bb.Min.x, p.y * (bb.Max.y - bb.Min.y) + bb.Min.y);
					ImVec2 s(q.x * (bb.Max.x - bb.Min.x) + bb.Min.x, q.y * (bb.Max.y - bb.Min.y) + bb.Min.y);
					DrawList->AddLine(r, s, color, CURVE_WIDTH);
				}
			}

			// draw lines and grabbers
			float luma = IsItemActive() || IsItemHovered() ? 0.5f : 1.0f;
			ImVec4 pink(1.00f, 0.00f, 0.75f, luma), cyan(0.00f, 0.75f, 1.00f, luma);
			ImVec4 white(GetStyle().Colors[ImGuiCol_Text]);
			ImVec2 p1 = ImVec2(P[0], 1 - P[1]) * (bb.Max - bb.Min) + bb.Min;
			ImVec2 p2 = ImVec2(P[2], 1 - P[3]) * (bb.Max - bb.Min) + bb.Min;
			DrawList->AddLine(ImVec2(bb.Min.x, bb.Max.y), p1, ImColor(white), LINE_WIDTH);
			DrawList->AddLine(ImVec2(bb.Max.x, bb.Min.y), p2, ImColor(white), LINE_WIDTH);
			DrawList->AddCircleFilled(p1, GRAB_RADIUS, ImColor(white));
			DrawList->AddCircleFilled(p1, GRAB_RADIUS - GRAB_BORDER, ImColor(pink));
			DrawList->AddCircleFilled(p2, GRAB_RADIUS, ImColor(white));
			DrawList->AddCircleFilled(p2, GRAB_RADIUS - GRAB_BORDER, ImColor(cyan));

			if (hovered || changed) DrawList->PopClipRect();

			// restore cursor pos
			SetCursorScreenPos(ImVec2(bb.Min.x, bb.Max.y + GRAB_RADIUS)); // :P
		}

		return changed;
	}

	void ShowBezierDemo()
	{
		{ static float v[] = { 0.000f, 0.000f, 1.000f, 1.000f }; Bezier("easeLinear", v); }
		{ static float v[] = { 0.470f, 0.000f, 0.745f, 0.715f }; Bezier("easeInSine", v); }
		{ static float v[] = { 0.390f, 0.575f, 0.565f, 1.000f }; Bezier("easeOutSine", v); }
		{ static float v[] = { 0.445f, 0.050f, 0.550f, 0.950f }; Bezier("easeInOutSine", v); }
		{ static float v[] = { 0.550f, 0.085f, 0.680f, 0.530f }; Bezier("easeInQuad", v); }
		{ static float v[] = { 0.250f, 0.460f, 0.450f, 0.940f }; Bezier("easeOutQuad", v); }
		{ static float v[] = { 0.455f, 0.030f, 0.515f, 0.955f }; Bezier("easeInOutQuad", v); }
		{ static float v[] = { 0.550f, 0.055f, 0.675f, 0.190f }; Bezier("easeInCubic", v); }
		{ static float v[] = { 0.215f, 0.610f, 0.355f, 1.000f }; Bezier("easeOutCubic", v); }
		{ static float v[] = { 0.645f, 0.045f, 0.355f, 1.000f }; Bezier("easeInOutCubic", v); }
		{ static float v[] = { 0.895f, 0.030f, 0.685f, 0.220f }; Bezier("easeInQuart", v); }
		{ static float v[] = { 0.165f, 0.840f, 0.440f, 1.000f }; Bezier("easeOutQuart", v); }
		{ static float v[] = { 0.770f, 0.000f, 0.175f, 1.000f }; Bezier("easeInOutQuart", v); }
		{ static float v[] = { 0.755f, 0.050f, 0.855f, 0.060f }; Bezier("easeInQuint", v); }
		{ static float v[] = { 0.230f, 1.000f, 0.320f, 1.000f }; Bezier("easeOutQuint", v); }
		{ static float v[] = { 0.860f, 0.000f, 0.070f, 1.000f }; Bezier("easeInOutQuint", v); }
		{ static float v[] = { 0.950f, 0.050f, 0.795f, 0.035f }; Bezier("easeInExpo", v); }
		{ static float v[] = { 0.190f, 1.000f, 0.220f, 1.000f }; Bezier("easeOutExpo", v); }
		{ static float v[] = { 1.000f, 0.000f, 0.000f, 1.000f }; Bezier("easeInOutExpo", v); }
		{ static float v[] = { 0.600f, 0.040f, 0.980f, 0.335f }; Bezier("easeInCirc", v); }
		{ static float v[] = { 0.075f, 0.820f, 0.165f, 1.000f }; Bezier("easeOutCirc", v); }
		{ static float v[] = { 0.785f, 0.135f, 0.150f, 0.860f }; Bezier("easeInOutCirc", v); }
		{ static float v[] = { 0.600f, -0.28f, 0.735f, 0.045f }; Bezier("easeInBack", v); }
		{ static float v[] = { 0.175f, 0.885f, 0.320f, 1.275f }; Bezier("easeOutBack", v); }
		{ static float v[] = { 0.680f, -0.55f, 0.265f, 1.550f }; Bezier("easeInOutBack", v); }
		// easeInElastic: not a bezier
		// easeOutElastic: not a bezier
		// easeInOutElastic: not a bezier
		// easeInBounce: not a bezier
		// easeOutBounce: not a bezier
		// easeInOutBounce: not a bezier
	}
}
// Include imgui_user.inl at the end of imgui.cpp to access private data/functions that aren't exposed.
// Prefer just including imgui_internal.h from your code rather than using this define. If a declaration is missing from imgui_internal.h add it or request it on the github.
#ifdef IMGUI_INCLUDE_IMGUI_USER_INL
#include "imgui_user.inl"
#endif

//-----------------------------------------------------------------------------

#endif //IMGUI_ENABLED