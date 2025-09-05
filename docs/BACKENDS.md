_(You may browse this at https://github.com/ocornut/imgui/blob/master/docs/BACKENDS.md or view this file with any Markdown viewer)_

# Dear ImGui: Backends

## Index

- [Introduction](#introduction)
   - [Getting Started](#getting-started)
   - [What are Backends?](#what-are-backends)
- [Using standard Backends](#using-standard-backends)
- [Using third-party Backends](#using-third-party-backends)
- [Writing your own Backend](#writing-your-own-backend)
  - [Using a custom engine?](#using-a-custom-engine)
  - [Platform: Implementing your Platform Backend](#platform-implementing-your-platform-backend)
  - [Rendering: Implementing your RenderDrawData function](#rendering-implementing-your-renderdrawdata-function)
  - [Rendering: Adding support for `ImGuiBackendFlags_RendererHasTextures` (1.92+)](#rendering-adding-support-for-imguibackendflags_rendererhastextures-192)

## Introduction

### Getting Started

💡 The **[Getting Started](https://github.com/ocornut/imgui/wiki/Getting-Started) wiki guide** has examples of how to integrate Dear ImGui into an existing application.
<BR> The [EXAMPLES.MD](https://github.com/ocornut/imgui/blob/master/docs/EXAMPLES.md) documentation may also be worth a read.

### What are Backends?

Dear ImGui is highly portable and only requires a few things to run and render, typically:

 - Required: providing mouse/keyboard inputs (fed into the `ImGuiIO` structure).
 - Required: creating, updating and destroying textures.
 - Required: rendering indexed textured triangles with a clipping rectangle.

 Extra features are opt-in, our backends try to support as many as possible:

 - Optional: custom texture binding support.
 - Optional: clipboard support.
 - Optional: gamepad support.
 - Optional: mouse cursor shape support.
 - Optional: IME support.
 - Optional: multi-viewports support.
 etc.

This is essentially what each backend is doing + obligatory portability cruft. Using standard backends ensure you can get all those features including the ones that would be harder to implement on your side (e.g. multi-viewports support).

It is important to understand the difference between the core Dear ImGui library (files in the root folder)
and the backends which we are describing here (backends/ folder).

- Some issues may only be backend or platform specific.
- You should be able to write backends for pretty much any platform and any 3D graphics API.
  e.g. you can get creative and use software rendering or render remotely on a different machine.

## Using standard Backends

**The [backends/](https://github.com/ocornut/imgui/blob/master/backends) folder contains backends for popular platforms/graphics API, which you can use in
your application or engine to easily integrate Dear ImGui.** Each backend is typically self-contained in a pair of files: imgui_impl_XXXX.cpp + imgui_impl_XXXX.h.

- The 'Platform' backends are in charge of: mouse/keyboard/gamepad inputs, cursor shape, timing, and windowing.<BR>
  e.g. Windows ([imgui_impl_win32.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_win32.cpp)), SDL3 ([imgui_impl_sdl3.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_sdl3.cpp)), GLFW ([imgui_impl_glfw.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_glfw.cpp)), etc.

- The 'Renderer' backends are in charge of: creating atlas texture, and rendering imgui draw data.<BR>
  e.g. DirectX11 ([imgui_impl_dx11.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_dx11.cpp)), OpenGL/WebGL ([imgui_impl_opengl3.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_opengl3.cpp)), Vulkan ([imgui_impl_vulkan.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_vulkan.cpp)), etc.

- For some high-level frameworks, a single backend usually handles both 'Platform' and 'Renderer' parts.<BR>
  e.g. Allegro 5 ([imgui_impl_allegro5.cpp](https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_allegro5.cpp)). If you end up creating a custom backend for your engine, you may want to do the same.

An application usually combines one Platform backend + one Renderer backend + main Dear ImGui sources.
For example, the [example_win32_directx11](https://github.com/ocornut/imgui/tree/master/examples/example_win32_directx11) application combines imgui_impl_win32.cpp + imgui_impl_dx11.cpp. There are 20+ examples in the [examples/](https://github.com/ocornut/imgui/blob/master/examples/) folder. See [EXAMPLES.MD](https://github.com/ocornut/imgui/blob/master/docs/EXAMPLES.md) for details.

**Once Dear ImGui is setup and running, run and refer to `ImGui::ShowDemoWindow()` in imgui_demo.cpp for usage of the end-user API.**

### List of standard Backends

In the [backends/](https://github.com/ocornut/imgui/blob/master/backends) folder:

List of Platforms Backends:

    imgui_impl_android.cpp      ; Android native app API
    imgui_impl_glfw.cpp         ; GLFW (Windows, macOS, Linux, etc.) http://www.glfw.org/
    imgui_impl_osx.mm           ; macOS native API (not as feature complete as glfw/sdl backends)
    imgui_impl_sdl2.cpp         ; SDL2 (Windows, macOS, Linux, iOS, Android) https://www.libsdl.org
    imgui_impl_sdl3.cpp         ; SDL3 (Windows, macOS, Linux, iOS, Android) https://www.libsdl.org
    imgui_impl_win32.cpp        ; Win32 native API (Windows)
    imgui_impl_glut.cpp         ; GLUT/FreeGLUT (this is prehistoric software and absolutely not recommended today!)

List of Renderer Backends:

    imgui_impl_dx9.cpp          ; DirectX9
    imgui_impl_dx10.cpp         ; DirectX10
    imgui_impl_dx11.cpp         ; DirectX11
    imgui_impl_dx12.cpp         ; DirectX12
    imgui_impl_metal.mm         ; Metal (ObjC or C++)
    imgui_impl_opengl2.cpp      ; OpenGL 2 (legacy fixed pipeline. Don't use with modern OpenGL code!)
    imgui_impl_opengl3.cpp      ; OpenGL 3/4, OpenGL ES 2/3, WebGL
    imgui_impl_sdlgpu3.cpp      ; SDL_GPU (portable 3D graphics API of SDL3)
    imgui_impl_sdlrenderer2.cpp ; SDL_Renderer (optional component of SDL2 available from SDL 2.0.18+)
    imgui_impl_sdlrenderer3.cpp ; SDL_Renderer (optional component of SDL3. Prefer using SDL_GPU!).
    imgui_impl_vulkan.cpp       ; Vulkan
    imgui_impl_wgpu.cpp         ; WebGPU (web + desktop)

List of high-level Frameworks Backends (combining Platform + Renderer):

    imgui_impl_allegro5.cpp

Emscripten is also supported!
The SDL2+GL, SDL3+GL, GLFW+GL and GLFW+WebGPU examples are all ready to build and run with Emscripten.

### Recommended Backends

If you are not sure which backend to use, the recommended platform/frameworks for portable applications:

|Library |Website |Backend |Note |
|--------|--------|--------|-----|
| SDL3 | https://www.libsdl.org | imgui_impl_sdl3.cpp | Recommended |
| SDL2 | https://www.libsdl.org | imgui_impl_sdl2.cpp | |
| GLFW | https://github.com/glfw/glfw | imgui_impl_glfw.cpp | |
| Sokol | https://github.com/floooh/sokol | [util/sokol_imgui.h](https://github.com/floooh/sokol/blob/master/util/sokol_imgui.h) | Lower-level than GLFW/SDL |

If your application runs on Windows or if you are using multi-viewport, the win32 backend handles some details a little better than other backends.

## Using third-party Backends

See https://github.com/ocornut/imgui/wiki/Bindings for the full list (e.g. Adventure Game Studio, Cinder, Cocos2d-x, Game Maker Studio2, Godot, LÖVE+LUA, Magnum, Monogame, Ogre, openFrameworks, OpenSceneGraph, SFML, Sokol, Unity, Unreal Engine and many others).

## Writing your own Backend

### Using a custom engine?

You will likely be tempted to start by rewrite your own backend using your own custom/high-level facilities...<BR>
Think twice!

TL;DR;
- Writing your own Renderer Backend is easy.
- Writing your own Platform Backend is harder and you are more likely to introduce bugs.
- **It is unlikely you will add value to your project by creating your own backend.**

**Consider using the existing backends as-is**.
You will save lots of time integrating the library.
Standard backends are battle-tested and handle subtleties that you are likely to implement incorrectly.
You can LATER decide to rewrite yourself a custom backend if you really need to.
In most situations, custom backends have fewer features and more bugs than the standard backends we provide.
If you want portability, you can use multiple backends and choose between them either at compile time
or at runtime.

**Example A**: your engine is built over Windows + DirectX11 but you have your own high-level rendering
system layered over DirectX11.<BR>
Suggestion: try using imgui_impl_win32.cpp + imgui_impl_dx11.cpp first.
Once it works, if you really need it, you can replace the imgui_impl_dx11.cpp code with a
custom renderer using your own rendering functions, and keep using the standard Win32 code etc.

**Example B**: your engine runs on Windows, Mac, Linux and uses DirectX11, Metal, and Vulkan respectively.<BR>
Suggestion: use multiple generic backends!
Once it works, if you really need it, you can replace parts of backends with your own abstractions.

**Example C**: your engine runs on platforms we can't provide public backends for (e.g. PS4/PS5, Switch),
and you have high-level systems everywhere.<BR>
Suggestion: try using a non-portable backend first (e.g. win32 + underlying graphics API) to get
your desktop builds working first. This will get you running faster and get your acquainted with
how Dear ImGui works and is setup. You can then rewrite a custom backend using your own engine API...

Also:
The [multi-viewports feature](https://github.com/ocornut/imgui/wiki/Multi-Viewports) of the 'docking' branch allows
Dear ImGui windows to be seamlessly detached from the main application window. This is achieved using an
extra layer to the Platform and Renderer backends, which allows Dear ImGui to communicate platform-specific
requests such as: "create an additional OS window", "create a render context", "get the OS position of this
window", but some things are more difficult "find OS window under mouse position BUT with some windows marked as passthrough". See 'ImGuiPlatformIO' for details.
Supporting the multi-viewports feature correctly using 100% of your own abstractions is more difficult
than supporting single-viewport.
If you decide to use unmodified imgui_impl_XXXX.cpp files, you can automatically benefit from
improvements and fixes related to viewports and platform windows without extra work on your side.

### Platform: Implementing your Platform Backend

The Platform backends in impl_impl_XXX.cpp files contain many implementations.

**In your `ImGui_ImplXXX_Init()` function:**
- You can allocate your backend data and use `io.BackendPlatformUserData` to store/retrieve it later.
- Set `io.BackendPlatformName` to a name `"imgui_impl_xxxx"` which will be available in e.g. About box.
- Set `io.BackendPlatformUserData` to your backend data.
- Set `io.BackendFlags` with supported optional features:
  - `ImGuiBackendFlags_HasGamepad`: supports gamepad and currently has one connected.
  - `ImGuiBackendFlags_HasMouseCursors`: supports honoring GetMouseCursor() value to change the OS cursor shape.
  - `ImGuiBackendFlags_HasSetMousePos`: supports io.WantSetMousePos requests to reposition the OS mouse position (only used if io.ConfigNavMoveSetMousePos is set).
  - `ImGuiBackendFlags_PlatformHasViewports` supports multiple viewports. (multi-viewports only)
  - `ImGuiBackendFlags_HasMouseHoveredViewport` supports calling io.AddMouseViewportEvent() with the viewport under the mouse. IF POSSIBLE, ignore viewports with the ImGuiViewportFlags_NoInputs flag. If this cannot be done, Dear ImGui needs to use a flawed heuristic to find the viewport under mouse position, as it doesn't know about foreign windows. (multi-viewports only)

**In your `ImGui_ImplXXX_NewFrame()` function:**
- Set `io.DeltaTime` to the time elapsed (in seconds) since last frame.
- Set `io.DisplaySize` to your window size.
- Set `io.DisplayFrameBufferSize` to your window pixel density (macOS/iOS only).
- Update mouse cursor shape is supported.

**In your `ImGui_ImplXXX_NewFrame()` function or event handlers:**
- **Mouse Support**
  - Use `io.AddMousePosEvent()`, `io.AddMouseButtonEvent()`, `io.AddMouseWheelEvent()` to pass mouse events.
  - Use `io.AddMouseSourceEvent()` if you are able to distinguish Mouse from TouchScreen from Pen inputs. TouchScreen and Pen inputs requires different logic for some Dear ImGui features.
  - Use `io.AddMouseViewportEvent()` to specify which viewport/OS window is being hovered by the mouse. Read instructions carefully as this is not as simple as it seems! (multi-viewports only)
- **Keyboard Support**
  - Use `io.AddKeyEvent()` to pass key events.
  - Use `io.AddInputCharacter()` to pass text/character events.
- **Gamepad Support**
  - Use `io.AddKeyEvent()` and `io.AddKeyAnalogEvent()` to pass gamepad events, using `ImGuiKey_GamepadXXX` values.
- **Miscellaneous**
  - Clipboard Support: setup `Platform_GetClipboardTextFn()`, `Platform_SetClipboardTextFn()` handlers in `ImGuiPlatformIO`.
  - Open in Shell support: setup `Platform_OpenInShellFn()` handler in `ImGuiPlatformIO`.
  - IME Support: setup `Platform_SetImeDataFn()` handler in `ImGuiPlatformIO`.
  - Use `io.AddFocusEvent()` to notify when application window gets focused/unfocused.
- **Multi-viewport Support**
  - Update monitor list if supported.
  - Setup all required handlers in `ImGuiPlatformIO` to create/destroy/move/resize/title/focus/etc. windows.

### Rendering: Implementing your RenderDrawData function

Note: set `ImGuiBackendFlags_RendererHasVtxOffset` to signify your backend can handle rendering with a vertex offset (`ImDrawCmd::VtxOffset` field).
Otherwise, rendering will be limited to 64K vertices per window, which may be limiting for advanced plot.
As an alternative, you may also use `#define ImDrawIdx unsigned int` in your `imconfig.h` file to support 32-bit indices.

```cpp
void MyImGuiBackend_RenderDrawData(ImDrawData* draw_data)
{
    // TODO: Update textures.
    // - Most of the times, the list will have 1 element with an OK status, aka nothing to do.
    // - This almost always points to ImGui::GetPlatformIO().Textures[] but is part of ImDrawData to allow overriding or disabling texture updates.
    if (draw_data->Textures != nullptr)
        for (ImTextureData* tex : *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                MyImGuiBackend_UpdateTexture(tex);

    // TODO: Setup render state:
    // - Alpha-blending enabled
    // - No backface culling
    // - No depth testing, no depth writing
    // - Scissor enabled
    MyEngineSetupenderState();

    // TODO: Setup texture sampling state
    // - Sample with bilinear filtering (NOT point/nearest filtering).
    // - Use 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines;' to allow point/nearest filtering.

    // TODO: Setup viewport covering draw_data->DisplayPos to draw_data->DisplayPos + draw_data->DisplaySize

    // TODO: Setup orthographic projection matrix cover draw_data->DisplayPos to draw_data->DisplayPos + draw_data->DisplaySize

    // TODO: Setup shader: vertex { float2 pos, float2 uv, u32 color }, fragment shader sample color from 1 texture, multiply by vertex color.

    // Render command lists
    ImVec2 clip_off = draw_data->DisplayPos;
    ImVec2 clip_scale = draw_data->FramebufferScale;
    for (const ImDrawList* draw_list : draw_data->CmdLists)
    {
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;  // vertex buffer generated by Dear ImGui
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;   // index buffer generated by Dear ImGui
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    MyEngineSetupenderState();
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                // - Clipping coordinates are provided in imgui coordinates space:
                //   - For a given viewport, draw_data->DisplayPos == viewport->Pos and draw_data->DisplaySize == viewport->Size
                //   - In a single viewport application, draw_data->DisplayPos == (0,0) and draw_data->DisplaySize == io.DisplaySize, but always use GetMainViewport()->Pos/Size instead of hardcoding those values.
                //   - In the interest of supporting multi-viewport applications (see 'docking' branch on github),
                //     always subtract draw_data->DisplayPos from clipping bounds to convert them to your viewport space.
                // - Note that pcmd->ClipRect contains Min+Max bounds. Some graphics API may use Min+Max, other may use Min+Size (size being Max-Min)
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // We are using scissoring to clip some objects. All low-level graphics API should support it.
                // - If your engine doesn't support scissoring yet, you may ignore this at first. You will get some small glitches
                //   (some elements visible outside their bounds) but you can fix that once everything else works!
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
```

### Rendering: Adding support for `ImGuiBackendFlags_RendererHasTextures` (1.92+)

Version [1.92.0](https://github.com/ocornut/imgui/releases/tag/v1.92.0) (June 2025), added texture support in Rendering Backends, which is the backbone for supporting dynamic font scaling among other things.

**In order to move forward and take advantage of all new features, support for `ImGuiBackendFlags_RendererHasTextures` will likely be REQUIRED for all backends before June 2026.**

`ImFontAtlas` functions such as `Build()`, `GetTexDataAsRGBA32()`, `GetTexDataAsAlpha8()`, `SetTexID()`, `IsBuilt()` were obsoleted in favor if iterating a `Textures[]` array and updating their state when requested by Dear ImGui.

**TD;DR: List of commits which added support for `ImGuiBackendFlags_RendererHasTextures` in standard backends:**

- Allegro5: [ee8941e](https://github.com/ocornut/imgui/commit/ee8941e) (+35 lines)
- DirectX9: [75efba7](https://github.com/ocornut/imgui/commit/75efba7) (+48 lines)
- DirectX10: [2d2b1bc](https://github.com/ocornut/imgui/commit/2d2b1bc) (+40 lines)
- DirectX11: [372fd27](https://github.com/ocornut/imgui/commit/372fd27) (+40 lines)
- DirectX12: [eefe5d5](https://github.com/ocornut/imgui/commit/eefe5d5) (+87 lines)
- Metal: [26c017d](https://github.com/ocornut/imgui/commit/26c017d) (+55 lines)
- OpenGL Legacy: [0430c55](https://github.com/ocornut/imgui/commit/0430c55) (+25 lines)
- OpenGL3/WebGL/ES: [dbb91a5](https://github.com/ocornut/imgui/commit/dbb91a5) (+47 lines)
- SDL_Renderer2: [9fa65cd](https://github.com/ocornut/imgui/commit/9fa65cd) (+20 lines)
- SDL_Renderer3: [e538883](https://github.com/ocornut/imgui/commit/e538883) (+19 lines)
- SDL_GPU: [16fe666](https://github.com/ocornut/imgui/commit/16fe666) (+41 lines)
- Vulkan: [abe294b](https://github.com/ocornut/imgui/commit/abe294b) (+33 lines)
- WGPU: [571dae9](https://github.com/ocornut/imgui/commit/571dae9) (+30 lines)

**Instructions:**

- Set `ImGuiBackendFlags_RendererHasTextures` to signify your backend can handle the feature.
- During rendering, e.g. in your RenderDrawData function, iterate `ImDrawData->Textures` array and process all textures.
- During shutdown, iterate the `ImGui::GetPlatformIO().Textures` and destroy all textures.
- (Both arrays are `ImVector<ImTextureData*>`. They are only in different location because: to allow advanced users to perform multi-threaded rendering, we store a pointer to the texture list in ImDrawData, with the aim that multi-threaded rendering users replace it with their own pointer.)

Pseudo-code for processing a texture:
```cpp
if (draw_data->Textures != nullptr)
    for (ImTextureData* tex : *draw_data->Textures)
        if (tex->Status != ImTextureStatus_OK)
            MyImGuiBackend_UpdateTexture(tex);
```
```cpp
void MyImGuiBackend_UpdateTexture(ImTextureData* tex)
{
    if (tex->Status == ImTextureStatus_WantCreate)
    {
        // Create texture based on tex->Width, tex->Height.
        // - Most backends only support tex->Format == ImTextureFormat_RGBA32.
        // - Backends for particularly memory constrainted platforms may support tex->Format == ImTextureFormat_Alpha8.

        // Upload all texture pixels
        // - Read from our CPU-side copy of the texture and copy to your graphics API.
        // - Use tex->Width, tex->Height, tex->GetPixels(), tex->GetPixelsAt(), tex->GetPitch() as needed.

        // Store your data, and acknowledge creation.
        tex->SetTexID(xxxx); // Specify backend-specific ImTextureID identifier which will be stored in ImDrawCmd.
        tex->SetStatus(ImTextureStatus_OK);
        tex->BackendUserData = xxxx; // Store more backend data if needed (most backend allocate a small texture to store data in there)
    }
    if (tex->Status == ImTextureStatus_WantUpdates)
    {
        // Upload a rectangle of pixels to the existing texture
        // - We only ever write to textures regions which have never been used before!
        // - Use tex->TexID or tex->BackendUserData to retrieve your stored data.
        // - Use tex->UpdateRect.x/y, tex->UpdateRect.w/h to obtain the block position and size.
        //   - Use tex->Updates[] to obtain individual sub-regions within tex->UpdateRect. Not recommended.
        // - Read from our CPU-side copy of the texture and copy to your graphics API.
        // - Use tex->Width, tex->Height, tex->GetPixels(), tex->GetPixelsAt(), tex->GetPitch() as needed.

        // Acknowledge update
        tex->SetStatus(ImTextureStatus_OK);
    }
    if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames > 0)
    {
        // If you use staged rendering and have in-flight renders, changed tex->UnusedFrames > 0 check to higher count as needed e.g. > 2

        // Destroy texture
        // - Use tex->TexID or tex->BackendUserData to retrieve your stored data.
        // - Destroy texture in your graphics API.

        // Acknowledge destruction
        tex->SetTexID(ImTextureID_Invalid);
        tex->SetStatus(ImTextureStatus_Destroyed);
    }
}
```
Refer to "List of commits which added support for `ImGuiBackendFlags_RendererHasTextures` in standard backends" above for concrete examples of this.
