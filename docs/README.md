Dear ImGui
=====
[![Build Status](https://github.com/ocornut/imgui/workflows/build/badge.svg)](https://github.com/ocornut/imgui/actions?workflow=build) [![Static Analysis Status](https://github.com/ocornut/imgui/workflows/static-analysis/badge.svg)](https://github.com/ocornut/imgui/actions?workflow=static-analysis)


<sub>(This library is available under a free and permissive license, but needs financial support to sustain its continued improvements. In addition to maintenance and stability there are many desirable features yet to be added. If your company is using Dear ImGui, please consider reaching out.)</sub>

Businesses: support continued development and maintenance via invoiced technical support, maintenance, sponsoring contracts:
<br>&nbsp;&nbsp;_E-mail: contact @ dearimgui dot com_

Individuals: support continued development and maintenance [here](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=WGHNC6MBFLZ2S).

Also see [Sponsors](https://github.com/ocornut/imgui/wiki/Sponsors) page.

----

Dear ImGui is a **bloat-free graphical user interface library for C++**. It outputs optimized vertex buffers that you can render anytime in your 3D-pipeline enabled application. It is fast, portable, renderer agnostic and self-contained (no external dependencies).

Dear ImGui is designed to **enable fast iterations** and to **empower programmers** to create **content creation tools and visualization / debug tools** (as opposed to UI for the average end-user). It favors simplicity and productivity toward this goal, and lacks certain features normally found in more high-level libraries.

Dear ImGui is particularly suited to integration in games engine (for tooling), real-time 3D applications, fullscreen applications, embedded applications, or any applications on consoles platforms where operating system features are non-standard.

| [Usage](#usage) - [How it works](#how-it-works) - [Releases & Changelogs](#releases--changelogs) - [Demo](#demo) - [Integration](#integration) |
:----------------------------------------------------------: |
| [Upcoming changes](#upcoming-changes) - [Gallery](#gallery) - [Support, FAQ](#support-frequently-asked-questions-faq) -  [How to help](#how-to-help) - [Sponsors](#sponsors) - [Credits](#credits) - [License](#license) |
| [Wiki](https://github.com/ocornut/imgui/wiki) - [Languages & frameworks backends/bindings](https://github.com/ocornut/imgui/wiki/Bindings) - [Software using Dear ImGui](https://github.com/ocornut/imgui/wiki/Software-using-dear-imgui) - [User quotes](https://github.com/ocornut/imgui/wiki/Quotes) |

### Usage

**The core of Dear ImGui is self-contained within a few platform-agnostic files** which you can easily compile in your application/engine. They are all the files in the root folder of the repository (imgui.cpp, imgui.h, imgui_demo.cpp, imgui_draw.cpp etc.).

**No specific build process is required**. You can add the .cpp files to your existing project.

You will need a backend to integrate Dear ImGui in your app. The backend passes mouse/keyboard/gamepad inputs and variety of settings to Dear ImGui, and is in charge of rendering the resulting vertices.

**Backends for a variety of graphics api and rendering platforms** are provided in the [backends/](https://github.com/ocornut/imgui/tree/master/backends) folder, along with example applications in the [examples/](https://github.com/ocornut/imgui/tree/master/examples) folder. See the [Integration](#integration) section of this document for details. You may also create your own backend. Anywhere where you can render textured triangles, you can render Dear ImGui.

After Dear ImGui is setup in your application, you can use it from \_anywhere\_ in your program loop:

Code:
```cpp
ImGui::Text("Hello, world %d", 123);
if (ImGui::Button("Save"))
    MySaveFunction();
ImGui::InputText("string", buf, IM_ARRAYSIZE(buf));
ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
```
Result:
<br>![sample code output (dark)](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v175/capture_readme_styles_0001.png) ![sample code output (light)](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v175/capture_readme_styles_0002.png)
<br>_(settings: Dark style (left), Light style (right) / Font: Roboto-Medium, 16px)_

Code:
```cpp
// Create a window called "My First Tool", with a menu bar.
ImGui::Begin("My First Tool", &my_tool_active, ImGuiWindowFlags_MenuBar);
if (ImGui::BeginMenuBar())
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
        if (ImGui::MenuItem("Save", "Ctrl+S"))   { /* Do stuff */ }
        if (ImGui::MenuItem("Close", "Ctrl+W"))  { my_tool_active = false; }
        ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
}

// Edit a color (stored as ~4 floats)
ImGui::ColorEdit4("Color", my_color);

// Plot some values
const float my_values[] = { 0.2f, 0.1f, 1.0f, 0.5f, 0.9f, 2.2f };
ImGui::PlotLines("Frame Times", my_values, IM_ARRAYSIZE(my_values));

// Display contents in a scrolling region
ImGui::TextColored(ImVec4(1,1,0,1), "Important Stuff");
ImGui::BeginChild("Scrolling");
for (int n = 0; n < 50; n++)
    ImGui::Text("%04d: Some text", n);
ImGui::EndChild();
ImGui::End();
```
Result:
<br>![sample code output](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v180/code_sample_04_color.gif)

Dear ImGui allows you to **create elaborate tools** as well as very short-lived ones. On the extreme side of short-livedness: using the Edit&Continue (hot code reload) feature of modern compilers you can add a few widgets to tweaks variables while your application is running, and remove the code a minute later! Dear ImGui is not just for tweaking values. You can use it to trace a running algorithm by just emitting text commands. You can use it along with your own reflection data to browse your dataset live. You can use it to expose the internals of a subsystem in your engine, to create a logger, an inspection tool, a profiler, a debugger, an entire game making editor/framework, etc.

### How it works

Check out the Wiki's [About the IMGUI paradigm](https://github.com/ocornut/imgui/wiki#about-the-imgui-paradigm) section if you want to understand the core principles behind the IMGUI paradigm. An IMGUI tries to minimize superfluous state duplication, state synchronization and state retention from the user's point of view. It is less error prone (less code and less bugs) than traditional retained-mode interfaces, and lends itself to create dynamic user interfaces.

Dear ImGui outputs vertex buffers and command lists that you can easily render in your application. The number of draw calls and state changes required to render them is fairly small. Because Dear ImGui doesn't know or touch graphics state directly, you can call its functions  anywhere in your code (e.g. in the middle of a running algorithm, or in the middle of your own rendering process). Refer to the sample applications in the examples/ folder for instructions on how to integrate Dear ImGui with your existing codebase.

_A common misunderstanding is to mistake immediate mode gui for immediate mode rendering, which usually implies hammering your driver/GPU with a bunch of inefficient draw calls and state changes as the gui functions are called. This is NOT what Dear ImGui does. Dear ImGui outputs vertex buffers and a small list of draw calls batches. It never touches your GPU directly. The draw call batches are decently optimal and you can render them later, in your app or even remotely._

### Releases & Changelogs

See [Releases](https://github.com/ocornut/imgui/releases) page.
Reading the changelogs is a good way to keep up to date with the things Dear ImGui has to offer, and maybe will give you ideas of some features that you've been ignoring until now!

### Demo

Calling the `ImGui::ShowDemoWindow()` function will create a demo window showcasing variety of features and examples. The code is always available for reference in `imgui_demo.cpp`.

![screenshot demo](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v167/v167-misc.png)

You should be able to build the examples from sources (tested on Windows/Mac/Linux). If you don't, let us know! If you want to have a quick look at some Dear ImGui features, you can download Windows binaries of the demo app here:
- [imgui-demo-binaries-20200918.zip](https://www.dearimgui.org/binaries/imgui-demo-binaries-20200918.zip) (Windows, 1.78 WIP, built 2020/09/18, master branch) or [older demo binaries](https://www.dearimgui.org/binaries).

The demo applications are not DPI aware so expect some blurriness on a 4K screen. For DPI awareness in your application, you can load/reload your font at different scale, and scale your style with `style.ScaleAllSizes()` (see [FAQ](https://www.dearimgui.org/faq)).

### Integration

On most platforms and when using C++, **you should be able to use a combination of the [imgui_impl_xxxx](https://github.com/ocornut/imgui/tree/master/backends) backends without modification** (e.g. `imgui_impl_win32.cpp` + `imgui_impl_dx11.cpp`). If your engine supports multiple platforms, consider using more of the imgui_impl_xxxx files instead of rewriting them: this will be less work for you and you can get Dear ImGui running immediately. You can _later_ decide to rewrite a custom backend using your custom engine functions if you wish so.

Integrating Dear ImGui within your custom engine is a matter of 1) wiring mouse/keyboard/gamepad inputs 2) uploading one texture to your GPU/render engine 3) providing a render function that can bind textures and render textured triangles. The [examples/](https://github.com/ocornut/imgui/tree/master/examples) folder is populated with applications doing just that. If you are an experienced programmer at ease with those concepts, it should take you less than two hours to integrate Dear ImGui in your custom engine. **Make sure to spend time reading the [FAQ](https://www.dearimgui.org/faq), comments, and some of the examples/ application!**

Officially maintained backends/bindings (in repository):
- Renderers: DirectX9, DirectX10, DirectX11, DirectX12, Metal, OpenGL/ES/ES2, Vulkan, WebGPU.
- Platforms: GLFW, SDL2, Win32, Glut, OSX, Android.
- Frameworks: Emscripten, Allegro5, Marmalade.

[Third-party backends/bindings](https://github.com/ocornut/imgui/wiki/Bindings) wiki page:
- Languages: C, C# and: Beef, ChaiScript, Crystal, D, Go, Haskell, Haxe/hxcpp, Java, JavaScript, Julia, Kotlin, Lobster, Lua, Odin, Pascal, PureBasic, Python, Ruby, Rust, Swift...
- Frameworks: AGS/Adventure Game Studio, Amethyst, Blender, bsf, Cinder, Cocos2d-x, Diligent Engine, Flexium, GML/Game Maker Studio2, GLEQ, Godot, GTK3+OpenGL3, Irrlicht Engine, LÖVE+LUA, Magnum, Monogame, NanoRT, nCine, Nim Game Lib, Nintendo 3DS & Switch (homebrew), Ogre, openFrameworks, OSG/OpenSceneGraph, Orx, Photoshop, px_render, Qt/QtDirect3D, SDL_Renderer, SFML, Sokol, Unity, Unreal Engine 4, vtk, VulkanHpp, VulkanSceneGraph, Win32 GDI, WxWidgets.
- Note that C bindings ([cimgui](https://github.com/cimgui/cimgui)) are auto-generated, you can use its json/lua output to generate bindings for other languages.

[Useful widgets and extensions](https://github.com/ocornut/imgui/wiki/Useful-Widgets) wiki page:
- Text editors, node editors, timeline editors, plotting, software renderers, remote network access, memory editors, gizmos etc.

Also see [Wiki](https://github.com/ocornut/imgui/wiki) for more links and ideas.

### Upcoming Changes

Some of the goals for 2021 are:
- Work on Docking (see [#2109](https://github.com/ocornut/imgui/issues/2109), in public [docking](https://github.com/ocornut/imgui/tree/docking) branch)
- Work on Multi-Viewport / Multiple OS windows. (see [#1542](https://github.com/ocornut/imgui/issues/1542), in public [docking](https://github.com/ocornut/imgui/tree/docking) branch looking for feedback)
- Work on gamepad/keyboard controls. (see [#787](https://github.com/ocornut/imgui/issues/787))
- Work on automation and testing system, both to test the library and end-user apps. (see [#435](https://github.com/ocornut/imgui/issues/435))
- Make the examples look better, improve styles, improve font support, make the examples hi-DPI and multi-DPI aware.

### Gallery

For more user-submitted screenshots of projects using Dear ImGui, check out the [Gallery Threads](https://github.com/ocornut/imgui/issues/3488)!

For a list of third-party widgets and extensions, check out the [Useful Widgets](https://github.com/ocornut/imgui/wiki/Useful-Widgets) wiki page.

Custom engine
[![screenshot game](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v149/gallery_TheDragonsTrap-01-thumb.jpg)](https://cloud.githubusercontent.com/assets/8225057/20628927/33e14cac-b329-11e6-80f6-9524e93b048a.png)

Custom engine
[![screenshot tool](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v160/editor_white_preview.jpg)](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v160/editor_white.png)

[Tracy Profiler](https://github.com/wolfpld/tracy)
![tracy profiler](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v176/tracy_profiler.png)

### Support, Frequently Asked Questions (FAQ)

See: [Frequently Asked Questions (FAQ)](https://github.com/ocornut/imgui/blob/master/docs/FAQ.md) where common questions are answered.

See: [Wiki](https://github.com/ocornut/imgui/wiki) for many links, references, articles.

See: [Articles about the IMGUI paradigm](https://github.com/ocornut/imgui/wiki#about-the-imgui-paradigm) to read/learn about the Immediate Mode GUI paradigm.

For questions, bug reports, requests, feedback, you may post on [GitHub Issues](https://github.com/ocornut/imgui/issues) or [GitHub Discussions](https://github.com/ocornut/imgui/discussions). Please read and fill the New Issue template carefully.

Private support is available for paying business customers (E-mail: _contact @ dearimgui dot com_).

**Which version should I get?**

We occasionally tag [Releases](https://github.com/ocornut/imgui/releases) but it is generally safe and recommended to sync to master/latest. The library is fairly stable and regressions tend to be fixed fast when reported.

Advanced users may want to use the `docking` branch with [Multi-Viewport](https://github.com/ocornut/imgui/issues/1542) and [Docking](https://github.com/ocornut/imgui/issues/2109) features. This branch is kept in sync with master regularly.

**Who uses Dear ImGui?**

See the [Quotes](https://github.com/ocornut/imgui/wiki/Quotes), [Sponsors](https://github.com/ocornut/imgui/wiki/Sponsors), [Software using dear imgui](https://github.com/ocornut/imgui/wiki/Software-using-dear-imgui) Wiki pages for an idea of who is using Dear ImGui. Please add your game/software if you can! Also see the [Gallery Threads](https://github.com/ocornut/imgui/issues/3488)!

How to help
-----------

**How can I help?**

- See [GitHub Forum/issues](https://github.com/ocornut/imgui/issues) and [Github Discussions](https://github.com/ocornut/imgui/discussions).
- You may help with development and submit pull requests! Please understand that by submitting a PR you are also submitting a request for the maintainer to review your code and then take over its maintenance forever. PR should be crafted both in the interest in the end-users and also to ease the maintainer into understanding and accepting it.
- See [Help wanted](https://github.com/ocornut/imgui/wiki/Help-Wanted) on the [Wiki](https://github.com/ocornut/imgui/wiki/) for some more ideas.
- Have your company financially support this project (please reach by e-mail)

**How can I help financing further development of Dear ImGui?**

See [Sponsors](https://github.com/ocornut/imgui/wiki/Sponsors) page.

Sponsors
--------

Ongoing Dear ImGui development is currently financially supported by users and private sponsors:

*Platinum-chocolate sponsors*
- [Blizzard](https://careers.blizzard.com/en-us/openings/engineering/all/all/all/1), [Google](https://github.com/google/filament), [Nvidia](https://developer.nvidia.com/nvidia-omniverse), [Ubisoft](https://montreal.ubisoft.com/en/ubisoft-sponsors-user-interface-library-for-c-dear-imgui/)

*Double-chocolate and Salty caramel sponsors*
- [Activision](https://careers.activision.com/c/programmingsoftware-engineering-jobs), [Aras Pranckevičius](https://aras-p.info), [Arkane Studios](https://www.arkane-studios.com), [Framefield](http://framefield.com), [Grinding Gear Games](https://www.grindinggear.com), [Kylotonn](https://www.kylotonn.com), [Next Level Games](https://www.nextlevelgames.com), [RAD Game Tools](http://www.radgametools.com/), [Supercell](http://www.supercell.com)

Please see [detailed list of Dear ImGui supporters](https://github.com/ocornut/imgui/wiki/Sponsors) for past sponsors.
From November 2014 to December 2019, ongoing development has also been financially supported by its users on Patreon and through individual donations. 

**THANK YOU to all past and present supporters for helping to keep this project alive and thriving!**

Dear ImGui is using software and services provided free of charge for open source projects:
- [PVS-Studio](https://www.viva64.com/en/b/0570/) for static analysis.
- [GitHub actions](https://github.com/features/actions) for continuous integration systems.
- [OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage) for code coverage analysis.

Credits
-------

Developed by [Omar Cornut](http://www.miracleworld.net) and every direct or indirect [contributors](https://github.com/ocornut/imgui/graphs/contributors) to the GitHub. The early version of this library was developed with the support of [Media Molecule](http://www.mediamolecule.com) and first used internally on the game [Tearaway](http://tearaway.mediamolecule.com) (PS Vita).

Recurring contributors (2020): Omar Cornut [@ocornut](https://github.com/ocornut), Rokas Kupstys [@rokups](https://github.com/rokups), Ben Carter [@ShironekoBen](https://github.com/ShironekoBen).
A large portion of work on automation systems, regression tests and other features are currently unpublished.

Omar: "I first discovered the IMGUI paradigm at [Q-Games](http://www.q-games.com) where Atman Binstock had dropped his own simple implementation in the codebase, which I spent quite some time improving and thinking about. It turned out that Atman was exposed to the concept directly by working with Casey. When I moved to Media Molecule I rewrote a new library trying to overcome the flaws and limitations of the first one I've worked with. It became this library and since then I have spent an unreasonable amount of time iterating and improving it."

Embeds [ProggyClean.ttf](http://upperbounds.net) font by Tristan Grimmer (MIT license).

Embeds [stb_textedit.h, stb_truetype.h, stb_rect_pack.h](https://github.com/nothings/stb/) by Sean Barrett (public domain).

Inspiration, feedback, and testing for early versions: Casey Muratori, Atman Binstock, Mikko Mononen, Emmanuel Briney, Stefan Kamoda, Anton Mikhailov, Matt Willis. Also thank you to everyone posting feedback, questions and patches on GitHub.

License
-------

Dear ImGui is licensed under the MIT License, see [LICENSE.txt](https://github.com/ocornut/imgui/blob/master/LICENSE.txt) for more information.
