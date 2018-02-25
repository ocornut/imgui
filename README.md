dear imgui,
=====
[![Build Status](https://travis-ci.org/ocornut/imgui.svg?branch=master)](https://travis-ci.org/ocornut/imgui)
[![Coverity Status](https://scan.coverity.com/projects/4720/badge.svg)](https://scan.coverity.com/projects/4720)

_(This library is free but needs your support to sustain its development. There are many desirable features and maintenance ahead If you are an individual using dear imgui, please consider donating via Patreon or PayPal. If your company is using dear imgui, please consider financial support (e.g. sponsoring a few weeks/months of development). I can invoice for technical support, custom development etc. E-mail: omarcornut at gmail.)_

Monthly donations via Patreon:
<br>[![Patreon](https://cloud.githubusercontent.com/assets/8225057/5990484/70413560-a9ab-11e4-8942-1a63607c0b00.png)](http://www.patreon.com/imgui)

One-off donations via PayPal:
<br>[![PayPal](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=5Q73FPZ9C526U)

Dear ImGui is a bloat-free graphical user interface library for C++. It outputs optimized vertex buffers that you can render anytime in your 3D-pipeline enabled application. It is fast, portable, renderer agnostic and self-contained (no external dependencies).**

Dear ImGui is designed to enable fast iterations and to empower programmers to create content creation tools and visualization / debug tools (as opposed to UI for the average end-user). It favors simplicity and productivity toward this goal, and lacks certain features normally found in more high-level libraries.

Dear ImGui is particularly suited to integration in games engine (for tooling), realtime 3D applications, fullscreen applications, embedded applications, or any applications on consoles platforms where operating system features are non-standard. 

Dear ImGui is self-contained within a few files that you can easily copy and compile into your application/engine:
- imgui.cpp
- imgui.h
- imgui_demo.cpp
- imgui_draw.cpp
- imgui_internal.h
- imconfig.h (empty by default, user-editable)
- stb_rect_pack.h
- stb_textedit.h
- stb_truetype.h

No specific build process is required. You can add the .cpp files to your project or #include them from an existing file.

### Usage

Your code passes mouse/keyboard/gamepad inputs and settings to Dear ImGui (see example applications for more details). After Dear ImGui is setup, you can use it from \_anywhere\_ in your program loop:

Code:
```cpp
ImGui::Text("Hello, world %d", 123);
if (ImGui::Button("Save"))
{
    // do stuff
}
ImGui::InputText("string", buf, IM_ARRAYSIZE(buf));
ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
```
Result:
<br>![sample code output](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v160/code_sample_02.png)
<br>_(settings: Dark style (left), Light style (right) / Font: Roboto-Medium, 16px / Rounding: 5)_

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
<br>![sample code output](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v160/code_sample_03_color.gif)

### How it works

Check out the References section if you want to understand the core principles behind the IMGUI paradigm. An IMGUI tries to minimize state duplication, state synchronization and state storage from the user's point of view. It is less error prone (less code and less bugs) than traditional retained-mode interfaces, and lends itself to create dynamic user interfaces. 

Dear ImGui outputs vertex buffers and command lists that you can easily render in your application. The number of draw calls and state changes is typically very small. Because it doesn't know or touch graphics state directly, you can call ImGui commands anywhere in your code (e.g. in the middle of a running algorithm, or in the middle of your own rendering process). Refer to the sample applications in the examples/ folder for instructions on how to integrate dear imgui with your existing codebase. 

_A common misunderstanding is to mistake immediate mode gui for immediate mode rendering, which usually implies hammering your driver/GPU with a bunch of inefficient draw calls and state changes as the gui functions are called. This is NOT what Dear ImGui does. Dear ImGui outputs vertex buffers and a small list of draw calls batches. It never touches your GPU directly. The draw call batches are decently optimal and you can render them later, in your app or even remotely._

Dear ImGui allows you create elaborate tools as well as very short-lived ones. On the extreme side of short-liveness: using the Edit&Continue (hot code reload) feature of modern compilers you can add a few widgets to tweaks variables while your application is running, and remove the code a minute later! Dear ImGui is not just for tweaking values. You can use it to trace a running algorithm by just emitting text commands. You can use it along with your own reflection data to browse your dataset live. You can use it to expose the internals of a subsystem in your engine, to create a logger, an inspection tool, a profiler, a debugger, an entire game making editor/framework, etc.  

Demo Binaries
-------------

You should be able to build the examples from sources (tested on Windows/Mac/Linux). If you don't, let me know! If you want to have a quick look at some Dear ImGui features, you can download Windows binaries of the demo app here:
- [imgui-demo-binaries-20180207.zip](http://www.miracleworld.net/imgui/binaries/imgui-demo-binaries-20180207.zip) (Windows binaries, Dear ImGui 1.60 WIP built 2018/01/07, 5 executables)

The demo applications are unfortunately not yet DPI aware so expect some blurryness on a 4K screen. For DPI awareness you can load/reload your font at different scale, and scale your Style with `style.ScaleAllSizes()`.

Bindings
--------

Integrating Dear ImGui within your custom engine is a matter of 1) wiring mouse/keyboard/gamepad inputs 2) uploading one texture to your GPU/render engine 3) providing a render function that can bind textures and render textured triangles. The [examples/](https://github.com/ocornut/imgui/tree/master/examples) folder is populated with applications doing just that. If you are an experienced programmer and at ease with those concepts, it should take you less than an hour to integrate Dear ImGui in your custom engine, but make sure to spend time reading the FAQ, the comments and other documentation!

_NB: those third-party bindings may be more or less maintained, more or less close to the original API (as people who create language bindings sometimes haven't used the C++ API themselves.. for the good reason that they aren't C++ users). Dear ImGui was designed with C++ in mind and some of the subtleties may be lost in translation with other languages. If your language supports it, I would suggest replicating the function overloading and default parameters used in the original, else the API may be harder to use. In doubt, please check the original C++ version first!_

Languages: (third-party bindinds)
- C: [cimgui](https://github.com/Extrawurst/cimgui)
- C#/.Net: [ImGui.NET](https://github.com/mellinoe/ImGui.NET)
- ChaiScript: [imgui-chaiscript](https://github.com/JuJuBoSc/imgui-chaiscript)
- D: [DerelictImgui](https://github.com/Extrawurst/DerelictImgui)
- Go: [go-imgui](https://github.com/Armored-Dragon/go-imgui)
- Haxe/hxcpp: [linc_imgui](https://github.com/Aidan63/linc_imgui)
- JavaScript: [imgui-js](https://github.com/flyover/imgui-js)
- Lua: [imgui_lua_bindings](https://github.com/patrickriordan/imgui_lua_bindings)
- Odin: [odin-dear_imgui](https://github.com/ThisDrunkDane/odin-dear_imgui)
- Pascal: [imgui-pas](https://github.com/dpethes/imgui-pas)
- Python [CyImGui](https://github.com/chromy/cyimgui)
- Python [pyimgui](https://github.com/swistakm/pyimgui)
- Rust: [imgui-rs](https://github.com/Gekkio/imgui-rs)

Frameworks:
- DirectX 9, DirectX 10, DirectX 11, DirectX 12: [examples/](https://github.com/ocornut/imgui/tree/master/examples)
- OpenGL 2/3 (with GLFW or SDL): [examples/](https://github.com/ocornut/imgui/tree/master/examples)
- Vulkan (with GLFW): [examples/](https://github.com/ocornut/imgui/tree/master/examples)
- Allegro 5, iOS, Marmalade: [examples/](https://github.com/ocornut/imgui/tree/master/examples)
- Unmerged PR: SDL2 + OpenGLES + Emscripten: [#336](https://github.com/ocornut/imgui/pull/336)
- Unmerged PR: FreeGlut + OpenGL2: [#801](https://github.com/ocornut/imgui/pull/801)
- Unmerged PR: Native Win32 and OSX: [#281](https://github.com/ocornut/imgui/pull/281)
- Unmerged PR: Android: [#421](https://github.com/ocornut/imgui/pull/421)
- Cinder: [Cinder-ImGui](https://github.com/simongeilfus/Cinder-ImGui)
- Cocos2d-x: [imguix](https://github.com/c0i/imguix), [issue #551](https://github.com/ocornut/imgui/issues/551)
- Flexium/SFML: [FlexGUI](https://github.com/DXsmiley/FlexGUI)
- GML/GameMakerStudio2: [ImGuiGML](https://marketplace.yoyogames.com/assets/6221/imguigml)
- Irrlicht: [IrrIMGUI](https://github.com/ZahlGraf/IrrIMGUI)
- Ogre: [ogreimgui](https://bitbucket.org/LMCrashy/ogreimgui/src)
- OpenFrameworks: [ofxImGui](https://github.com/jvcleave/ofxImGui)
- OpenSceneGraph/OSG: [gist](https://gist.github.com/fulezi/d2442ca7626bf270226014501357042c)
- LÖVE: [love-imgui](https://github.com/slages/love-imgui)
- NanoRT: [syoyo/imgui](https://github.com/syoyo/imgui/tree/nanort)
- Qt3d: [imgui-qt3d](https://github.com/alpqr/imgui-qt3d)
- Unreal Engine 4: [segross/UnrealImGui](https://github.com/segross/UnrealImGui) or [sronsse/UnrealEngine_ImGui](https://github.com/sronsse/UnrealEngine_ImGui)
- SFML: [imgui-sfml](https://github.com/EliasD/imgui-sfml) or [imgui-backends](https://github.com/Mischa-Alff/imgui-backends)

For other bindings: see [this page](https://github.com/ocornut/imgui/wiki/Links/).
Please contact me with the Issues tracker or Twitter to fix/update this list.

Roadmap
-------
Some of the goals for 2018 are:
- Finish work on gamepad/keyboard controls. (see [#787](https://github.com/ocornut/imgui/issues/787))
- Finish work on viewports and multiple OS windows management. (see [#1542](https://github.com/ocornut/imgui/issues/1542))
- Finish work on docking, tabs. (see [#351](https://github.com/ocornut/imgui/issues/351#issuecomment-346865709))
- Make Columns better. (they are currently pretty terrible!)
- Make the examples look better, improve styles, improve font support, make the examples hi-DPI aware.

Gallery
-------

User screenshots:
<br>[Gallery Part 1](https://github.com/ocornut/imgui/issues/123) (Feb 2015 to Feb 2016)
<br>[Gallery Part 2](https://github.com/ocornut/imgui/issues/539) (Feb 2016 to Aug 2016)
<br>[Gallery Part 3](https://github.com/ocornut/imgui/issues/772) (Aug 2016 to Jan 2017)
<br>[Gallery Part 4](https://github.com/ocornut/imgui/issues/973) (Jan 2017 to Aug 2017)
<br>[Gallery Part 5](https://github.com/ocornut/imgui/issues/1269) (Aug 2017 onward)
<br>Also see the [Mega screenshots](https://github.com/ocornut/imgui/issues/1273) for an idea of the available features.

Various tools
[![screenshot game](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v149/gallery_TheDragonsTrap-01-thumb.jpg)](https://cloud.githubusercontent.com/assets/8225057/20628927/33e14cac-b329-11e6-80f6-9524e93b048a.png)

[![screenshot tool](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v160/editor_white_preview.jpg)](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v160/editor_white.png)

![screenshot demo](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v160/v160-misc-classic.png)

[![screenshot profiler](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v148/profiler-880.jpg)](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v148/profiler.png)

Dear ImGui can load TTF/OTF fonts. UTF-8 is supported for text display and input. Here using Arial Unicode font to display Japanese. Initialize custom font with:
Code:
```cpp
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontFromFileTTF("NotoSansCJKjp-Medium.otf", 20.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
```
```cpp
ImGui::Text(u8"こんにちは！テスト %d", 123);
if (ImGui::Button(u8"ロード"))
{
    // do stuff
}
ImGui::InputText("string", buf, IM_ARRAYSIZE(buf));
ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
```
Result:
<br>![sample code output](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v160/code_sample_02_jp.png)
<br>_(settings: Dark style (left), Light style (right) / Font: NotoSansCJKjp-Medium, 20px / Rounding: 5)_

References
----------

The Immediate Mode GUI paradigm may at first appear unusual to some users. This is mainly because "Retained Mode" GUIs have been so widespread and predominant. The following links can give you a better understanding about how Immediate Mode GUIs works. 
- [Johannes 'johno' Norneby's article](http://www.johno.se/book/imgui.html).
- [A presentation by Rickard Gustafsson and Johannes Algelind](http://www.cse.chalmers.se/edu/year/2011/course/TDA361/Advanced%20Computer%20Graphics/IMGUI.pdf).
- [Jari Komppa's tutorial on building an ImGui library](http://iki.fi/sol/imgui/).
- [Casey Muratori's original video that popularized the concept](https://mollyrocket.com/861).
- [Nicolas Guillemot's CppCon'16 flashtalk about Dear ImGui](https://www.youtube.com/watch?v=LSRJ1jZq90k).
- [Thierry Excoffier's Zero Memory Widget](http://perso.univ-lyon1.fr/thierry.excoffier/ZMW/).

See the [Links page](https://github.com/ocornut/imgui/wiki/Links) for third-party bindings to different languages and frameworks.

Frequently Asked Question (FAQ)
-------------------------------

<b>Where is the documentation?</b>

- The documentation is at the top of imgui.cpp + effectively imgui.h. 
- Example code is in imgui_demo.cpp and particularly the ImGui::ShowDemoWindow() function. It covers most features of ImGui so you can read the code and call the function itself to see its output. 
- Standalone example applications using e.g. OpenGL/DirectX are provided in the examples/ folder. 
- We obviously needs better documentation! Consider contributing or becoming a [Patron](http://www.patreon.com/imgui) to promote this effort.

<b>Which version should I get?</b>

I occasionally tag [Releases](https://github.com/ocornut/imgui/releases) but it is generally safe and recommended to sync to master/latest. The library is fairly stable and regressions tend to be fixed fast when reported. 

<b>Who uses Dear ImGui?</b>

See the [Software using dear imgui page](https://github.com/ocornut/imgui/wiki/Software-using-dear-imgui) for an (incomplete) list of games/software which are publicly known to use dear imgui. Please add yours if you can!

<b>Why the odd dual naming, "dear imgui" vs "ImGui"?</b>

The library started its life and is best known as "ImGui" only due to the fact that I didn't give it a proper name when I released it. However, the term IMGUI (immediate-mode graphical user interface) was coined before and is being used in variety of other situations. It seemed confusing and unfair to hog the name. To reduce the ambiguity without affecting existing codebases, I have decided on an alternate, longer name "dear imgui" that people can use to refer to this specific library in ambiguous situations.

<b>How can I help?</b>
<br><b>How can I display an image? What is ImTextureID, how does it works?</b>
<br><b>How can I have multiple widgets with the same label, or without any label? (Yes). A primer on labels and ID stack.</b>
<br><b>How can I tell when Dear ImGui wants my mouse/keyboard inputs VS when I can pass them to my application?</b>
<br><b>How can I load a different font than the default?</b>
<br><b>How can I easily use icons in my application?</b>
<br><b>How can I load multiple fonts?</b>
<br><b>How can I display and input non-latin characters such as Chinese, Japanese, Korean, Cyrillic?</b>
<br><b>How can I preserve my Dear ImGui context across reloading a DLL? (loss of the global/static variables)</b>
<br><b>How can I use the drawing facilities without an Dear ImGui window? (using ImDrawList API)</b>
<br><b>I integrated Dear ImGui in my engine and the text or lines are blurry..</b>
<br><b>I integrated Dear ImGui in my engine and some elements are disappearing when I move windows around..</b>

See the FAQ in imgui.cpp for answers.

<b>How do you use Dear ImGui on a platform that may not have a mouse or keyboard?</b>

You can control Dear ImGui with a gamepad, see the explanation in imgui.cpp about how to use the navigation feature (short version: map your gamepad inputs into the `io.NavInputs[]` array and set `io.NavFlags |= ImGuiNavFlags_EnableGamepad`).

You can share your computer mouse seamlessy with your console/tablet/phone using [Synergy](http://synergy-project.org). This is the prefered solution for developer productivity. In particular, their [micro-synergy-client](https://github.com/symless/micro-synergy-client) repo there is _uSynergy.c_ sources for a small embeddable that you can use on any platform to connect to your host PC. You may also use a third party solution such as [Remote  ImGui](https://github.com/JordiRos/remoteimgui).

For touch inputs, you can increase the hit box of widgets (via the _style.TouchPadding_ setting) to accommodate a little for the lack of precision of touch inputs, but it is recommended you use a mouse or gamepad to allow optimising for screen real-estate and precision.

<b>Can you create elaborate/serious tools with Dear ImGui?</b>

Yes. People have written game editors, data browsers, debuggers, profilers and all sort of non-trivial tools with the library. In my experience the simplicity of the API is very empowering. Your UI runs close to your live data. Make the tools always-on and everybody in the team will be inclined to create new tools (as opposed to more "offline" UI toolkits where only a fraction of your team effectively creates tools). The list of sponsors below is also an indicator that serious game teams have been using the library.

Dear ImGui is very programmer centric and the immediate-mode GUI paradigm might requires you to readjust some habits before you can realize its full potential. Dear ImGui is about making things that are simple, efficient and powerful.

<b>Can you reskin the look of Dear ImGui?</b>

You can alter the look of the interface to some degree: changing colors, sizes, padding, rounding, fonts. However, as Dear ImGui is designed and optimised to create debug tools, the amount of skinning you can apply is limited. There is only so much you can stray away from the default look and feel of the interface. Below is a screenshot from [LumixEngine](https://github.com/nem0/LumixEngine) with custom colors + a docking/tabs extension (both of which you can find in the Issues section and will eventually be merged):

![LumixEngine](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v151/lumix-201710-rearranged.png)

<b>Why using C++ (as opposed to C)?</b>

Dear ImGui takes advantage of a few C++ languages features for convenience but nothing anywhere Boost-insanity/quagmire. Dear ImGui does NOT require C++11 so it can be used with most old C++ compilers. Dear ImGui doesn't use any C++ header file. Language-wise, function overloading and default parameters are used to make the API easier to use and code more terse. Doing so I believe the API is sitting on a sweet spot and giving up on those features would make the API more cumbersome. Other features such as namespace, constructors and templates (in the case of the ImVector<> class) are also relied on as a convenience.

There is an reasonably maintained [c-api for ImGui](https://github.com/Extrawurst/cimgui) by Stephan Dilly designed for binding in other languages. I would suggest using your target language functionalities to try replicating the function overloading and default parameters used in C++ else the API may be harder to use. Also see [Links](https://github.com/ocornut/imgui/wiki/Links) for third-party bindings to other languages.

Support dear imgui
------------------

<b>How can I help financing further development of Dear ImGui?</b>

Your contributions are keeping the library alive. If you are an individual using dear imgui, please consider donating to enable me to spend more time improving the library.

Monthly donations via Patreon:
<br>[![Patreon](https://cloud.githubusercontent.com/assets/8225057/5990484/70413560-a9ab-11e4-8942-1a63607c0b00.png)](http://www.patreon.com/imgui)

One-off donations via PayPal:
<br>[![PayPal](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=5Q73FPZ9C526U)

If your company uses dear imgui, please consider financial support (e.g. sponsoring a few weeks/months of development). I can invoice for private support, custom development etc. E-mail: omarcornut at gmail. Thanks!

Credits
-------

Developed by [Omar Cornut](http://www.miracleworld.net) and every direct or indirect contributors to the GitHub. The early version of this library was developed with the support of [Media Molecule](http://www.mediamolecule.com) and first used internally on the game [Tearaway](http://tearaway.mediamolecule.com). 

I first discovered imgui principles at [Q-Games](http://www.q-games.com) where Atman had dropped his own simple imgui implementation in the codebase, which I spent quite some time improving and thinking about. It turned out that Atman was exposed to the concept directly by working with Casey. When I moved to Media Molecule I rewrote a new library trying to overcome the flaws and limitations of the first one I've worked with. It became this library and since then I have spent an unreasonable amount of time iterating on it. 

Embeds [ProggyClean.ttf](http://upperbounds.net) font by Tristan Grimmer (MIT license).

Embeds [stb_textedit.h, stb_truetype.h, stb_rectpack.h](https://github.com/nothings/stb/) by Sean Barrett (public domain).

Inspiration, feedback, and testing for early versions: Casey Muratori, Atman Binstock, Mikko Mononen, Emmanuel Briney, Stefan Kamoda, Anton Mikhailov, Matt Willis. And everybody posting feedback, questions and patches on the GitHub.

Ongoing dear imgui development is financially supported on [**Patreon**](http://www.patreon.com/imgui) and by private sponsors.

Double-chocolate sponsors:
- Blizzard
- Media Molecule
- Mobigame
- Insomniac Games
- Aras Pranckevičius
- Lizardcube
- Greggman
- DotEmu

Salty caramel supporters:
- Jetha Chan, Wild Sheep Studio, Pastagames, Mārtiņš Možeiko, Daniel Collin, Recognition Robotics, Chris Genova, ikrima, Glenn Fiedler, Geoffrey Evans, Dakko Dakko, Mercury Labs, Singularity Demo Group, Mischa Alff, Sebastien Ronsse, Lionel Landwerlin.

Caramel supporters:
- Michel Courtine, César Leblic, Dale Kim, Alex Evans, Rui Figueira, Paul Patrashcu, Jerome Lanquetot, Ctrl Alt Ninja, Paul Fleming, Neil Henning, Stephan Dilly, Neil Blakey-Milner, Aleksei, NeiloGD, Justin Paver, FiniteSol, Vincent Pancaldi, James Billot, Robin Hübner, furrtek, Eric, Simon Barratt, Game Atelier, Julian Bosch, Simon Lundmark, Vincent Hamm, Farhan Wali, Jeff Roberts, Matt Reyer, Colin Riley, Victor Martins, Josh Simmons, Garrett Hoofman, Sergio Gonzales, Andrew Berridge, Roy Eltham, Game Preservation Society, Kit framework, Josh Faust, Martin Donlon, Quinton, Felix, Andrew Belt, Codecat, Cort Stratton, Claudio Canepa, Doug McNabb, Emmanuel Julien, Guillaume Chereau, Jeffrey Slutter, Jeremiah Deckard, r-lyeh, Roger Clark, Nekith, Joshua Fisher, Malte Hoffmann, Mustafa Karaalioglu, Merlyn Morgan-Graham, Per Vognsen, Fabian Giesen, Jan Staubach, Matt Hargett, John Shearer, Jesse Chounard, kingcoopa, Miloš Tošić, Jonas Bernemann, Johan Andersson, Nathan Hartman, Michael Labbe, Tomasz Golebiowski, Louis Schnellbach, Felipe Alfonso, Jimmy Andrews, Bojan Endrovski, Robin Berg Pettersen, Rachel Crawford, Edsel Malasig, Andrew Johnson.

And other supporters; thanks!
(Please contact me or PR if you would like to be added or removed from this list)

License
-------

Dear ImGui is licensed under the MIT License, see LICENSE for more information.
