dear imgui,
=====
[![Build Status](https://travis-ci.org/ocornut/imgui.svg?branch=master)](https://travis-ci.org/ocornut/imgui)
[![Coverity Status](https://scan.coverity.com/projects/4720/badge.svg)](https://scan.coverity.com/projects/4720)

(This library is free but needs your support to sustain its development. There are lots of desirable new features and maintenance to do. If you are an individual using dear imgui, please consider donating via Patreon or PayPal. If your company is using dear imgui, please consider financial support (e.g. sponsoring a few weeks/months of development). I can invoice for private support, custom development etc. E-mail: omarcornut at gmail.)

Monthly donations via Patreon:
<br>[![Patreon](https://cloud.githubusercontent.com/assets/8225057/5990484/70413560-a9ab-11e4-8942-1a63607c0b00.png)](http://www.patreon.com/imgui)

One-off donations via PayPal:
<br>[![PayPal](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=5Q73FPZ9C526U)

Dear ImGui is a bloat-free graphical user interface library for C++. It outputs optimized vertex buffers that you can render anytime in your 3D-pipeline enabled application. It is fast, portable, renderer agnostic and self-contained (no external dependencies).

Dear ImGui is designed to enable fast iteration and empower programmers to create content creation tools and visualization/ debug tools (as opposed to UI for the average end-user). It favors simplicity and productivity toward this goal, and thus lacks certain features normally found in more high-level libraries.

Dear ImGui is particularly suited to integration in realtime 3D applications, fullscreen applications, embedded applications, games, or any applications on consoles platforms where operating system features are non-standard. 

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

Your code passes mouse/keyboard inputs and settings to Dear ImGui (see example applications for more details). After Dear ImGui is setup, you can use it like in this example:

![screenshot of sample code alongside its output with dear imgui](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/code_sample_01.png)

Dear ImGui outputs vertex buffers and simple command-lists that you can render in your application. The number of draw calls and state changes is typically very small. Because it doesn't know or touch graphics state directly, you can call ImGui commands anywhere in your code (e.g. in the middle of a running algorithm, or in the middle of your own rendering process). Refer to the sample applications in the examples/ folder for instructions on how to integrate dear imgui with your existing codebase. 

_A common misunderstanding is to think that immediate mode gui == immediate mode rendering, which usually implies hammering your driver/GPU with a bunch of inefficient draw calls and state changes, as the gui functions are called by the user. This is NOT what Dear ImGui does. Dear ImGui outputs vertex buffers and a small list of draw calls batches. It never touches your GPU directly. The draw call batches are decently optimal and you can render them later, in your app or even remotely._

Dear ImGui allows you create elaborate tools as well as very short-lived ones. On the extreme side of short-liveness: using the Edit&Continue feature of modern compilers you can add a few widgets to tweaks variables while your application is running, and remove the code a minute later! Dear ImGui is not just for tweaking values. You can use it to trace a running algorithm by just emitting text commands. You can use it along with your own reflection data to browse your dataset live. You can use it to expose the internals of a subsystem in your engine, to create a logger, an inspection tool, a profiler, a debugger, an entire game making editor/framework, etc.  

Binaries/Demo
-------------

You should be able to build the examples from sources (tested on Windows/Mac/Linux). If you don't, let me know! If you want to have a quick look at some Dear ImGui features, you can download Windows binaries of the demo app here:
- [imgui-demo-binaries-20171226.zip](http://www.miracleworld.net/imgui/binaries/imgui-demo-binaries-20171226.zip) (Windows binaries, Dear ImGui 1.53 built 2017/12/26, 5 executables)

Bindings
--------

Integrating Dear ImGui within your custom engine is a matter of wiring mouse/keyboard inputs and providing a render function that can bind a texture and render simple textured triangles. The examples/ folder is populated with applications doing just that. If you are an experienced programmer it should take you less than an hour to integrate Dear ImGui in your custom engine, but make sure to spend time reading the FAQ, the comments and other documentation!

_NB: those third-party bindings may be more or less maintained, more or less close to the spirit of original API and therefore I cannot give much guarantee about them. People who create language bindings sometimes haven't used the C++ API themselves (for the good reason that they aren't C++ users). Dear ImGui was designed with C++ in mind and some of the subtleties may be lost in translation with other languages. If your language supports it, I would suggest replicating the function overloading and default parameters used in the original, else the API may be harder to use. In doubt, please check the original C++ version first!_

Languages:
- C (cimgui): https://github.com/Extrawurst/cimgui
- C#/.Net (ImGui.NET): https://github.com/mellinoe/ImGui.NET
- ChaiScript: https://github.com/JuJuBoSc/imgui-chaiscript
- D (DerelictImgui): https://github.com/Extrawurst/DerelictImgui
- Go (go-imgui): https://github.com/Armored-Dragon/go-imgui
- Haxe/hxcpp (linc_imgui): https://github.com/Aidan63/linc_imgui    
- Lua: https://github.com/patrickriordan/imgui_lua_bindings
- Odin: https://github.com/ThisDrunkDane/odin-dear_imgui
- Pascal (imgui-pas): https://github.com/dpethes/imgui-pas
- Python (CyImGui): https://github.com/chromy/cyimgui
- Python (pyimgui): https://github.com/swistakm/pyimgui
- Rust (imgui-rs): https://github.com/Gekkio/imgui-rs

Frameworks:
- Main ImGui repository include examples for DirectX9, DirectX10, DirectX11, OpenGL2/3, Vulkan, Allegro 5, SDL+GL2/3, iOS and Marmalade: https://github.com/ocornut/imgui/tree/master/examples
- Unmerged PR: DirectX12: https://github.com/ocornut/imgui/pull/301
- Unmerged PR: SDL2 + OpenGLES + Emscripten: https://github.com/ocornut/imgui/pull/336
- Unmerged PR: FreeGlut + OpenGL2: https://github.com/ocornut/imgui/pull/801
- Unmerged PR: Native Win32 and OSX: https://github.com/ocornut/imgui/pull/281
- Unmerged PR: Android: https://github.com/ocornut/imgui/pull/421
- Cinder: https://github.com/simongeilfus/Cinder-ImGui
- cocos2d-x: https://github.com/c0i/imguix https://github.com/ocornut/imgui/issues/551
- Flexium/SFML (FlexGUI): https://github.com/DXsmiley/FlexGUI
- GML/GameMakerStudio2 (ImGuiGML): https://marketplace.yoyogames.com/assets/6221/imguigml
- Irrlicht (IrrIMGUI): https://github.com/ZahlGraf/IrrIMGUI
- Ogre: https://bitbucket.org/LMCrashy/ogreimgui/src
- openFrameworks (ofxImGui): https://github.com/jvcleave/ofxImGui
- OpenSceneGraph/OSG: https://gist.github.com/fulezi/d2442ca7626bf270226014501357042c
- LÖVE: https://github.com/slages/love-imgui
- NanoRT (software raytraced) https://github.com/syoyo/imgui/tree/nanort/examples/raytrace_example
- Qt3d https://github.com/alpqr/imgui-qt3d
- Unreal Engine 4: https://github.com/segross/UnrealImGui or https://github.com/sronsse/UnrealEngine_ImGui
- SFML: https://github.com/EliasD/imgui-sfml or https://github.com/Mischa-Alff/imgui-backends

For other bindings: see [this page](https://github.com/ocornut/imgui/wiki/Links/).
Please contact me with the Issues tracker or Twitter to fix/update this list.

Gallery
-------

See the [Screenshots Thread](https://github.com/ocornut/imgui/issues/123) for some user creations.
Also see the [Mega screenshots](https://github.com/ocornut/imgui/issues/1273) for an idea of the available features.

![screenshot 1](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v148/examples_01.png)
[![screenshot game](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v149/gallery_TheDragonsTrap-01-thumb.jpg)](https://cloud.githubusercontent.com/assets/8225057/20628927/33e14cac-b329-11e6-80f6-9524e93b048a.png)
![screenshot 2](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v148/examples_02.png)

[![screenshot profiler](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v148/profiler-880.jpg)](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v148/profiler.png)

![screenshot picker](https://user-images.githubusercontent.com/8225057/29062188-471e95ba-7c53-11e7-9618-c4484c0b75fe.PNG)

![screenshot 5](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v151/menus.png)
![screenshot 6](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v143/skinning_sample_02.png)
![screenshot 7](https://cloud.githubusercontent.com/assets/8225057/7903336/96f0fb7c-07d0-11e5-95d6-41c6a1595e5a.png)

Dear ImGui can load TTF/OTF fonts. UTF-8 is supported for text display and input. Here using Arial Unicode font to display Japanese. Initialize custom font with:
```
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontFromFileTTF("ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

// For Microsoft IME, pass your HWND to enable IME positioning:
io.ImeWindowHandle = my_hwnd;
```
![Japanese screenshot](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/code_sample_01_jp.png)

References
----------

The Immediate Mode GUI paradigm may at first appear unusual to some users. This is mainly because "Retained Mode" GUIs have been so widespread and predominant. The following links can give you a better understanding about how Immediate Mode GUIs works. 
- [Johannes 'johno' Norneby's article](http://www.johno.se/book/imgui.html).
- [A presentation by Rickard Gustafsson and Johannes Algelind](http://www.cse.chalmers.se/edu/year/2011/course/TDA361/Advanced%20Computer%20Graphics/IMGUI.pdf).
- [Jari Komppa's tutorial on building an ImGui library](http://iki.fi/sol/imgui/).
- [Casey Muratori's original video that popularized the concept](https://mollyrocket.com/861).
- [Nicolas Guillemot's CppCon'16 flashtalk about Dear ImGui](https://www.youtube.com/watch?v=LSRJ1jZq90k).
- [Thierry Excoffier's Zero Memory Widget](http://perso.univ-lyon1.fr/thierry.excoffier/ZMW/).

See the [Software using dear imgui page](https://github.com/ocornut/imgui/wiki/Software-using-dear-imgui) for an incomplete list of software which are publicly known to use dear migui.

See the [Links page](https://github.com/ocornut/imgui/wiki/Links) for third-party bindings to different languages and frameworks.

Frequently Asked Question (FAQ)
-------------------------------

<b>Where is the documentation?</b>

- The documentation is at the top of imgui.cpp + effectively imgui.h. 
- Example code is in imgui_demo.cpp and particularly the ImGui::ShowDemoWindow() function. It covers most features of ImGui so you can read the code and call the function itself to see its output. 
- Standalone example applications using e.g. OpenGL/DirectX are provided in the examples/ folder. 
- We obviously needs better documentation! Consider contributing or becoming a [Patron](http://www.patreon.com/imgui) to promote this effort.

<b>Which version should I get?</b>

I occasionally tag [Releases](https://github.com/ocornut/imgui/releases) but it is generally safe and recommended to sync to master. The library is fairly stable and regressions tend to be fixed fast when reported. You may also want to checkout the [navigation branch](https://github.com/ocornut/imgui/tree/navigation) if you want to use Dear ImGui with a gamepad (it is also possible to map keyboard inputs to some degree). The Navigation branch is being kept up to date with Master.

<b>Why the odd dual naming, "dear imgui" vs "ImGui"?</b>

The library started its life and is best known as "ImGui" only due to the fact that I didn't give it a proper name when I released it. However, the term IMGUI (immediate-mode graphical user interface) was coined before and is being used in variety of other situations. It seemed confusing and unfair to hog the name. To reduce the ambiguity without affecting existing codebases, I have decided on an alternate, longer name "dear imgui" that people can use to refer to this specific library in ambiguous situations.

<b>How can I help</b>
<br><b>How can I display an image? What is ImTextureID, how does it works?</b>
<br><b>How can I have multiple widgets with the same label? Can I have widget without a label? (Yes). A primer on labels and the ID stack.</b>
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

I recommend using [Synergy](http://synergy-project.org) ([sources](https://github.com/symless/synergy)). In particular, the _src/micro/uSynergy.c_ file contains a small client that you can use on any platform to connect to your host PC. You can seamlessly use your PC input devices from a video game console or a tablet. Dear ImGui allows to increase the hit box of widgets (via the _style.TouchPadding_ setting) to accommodate a little for the lack of precision of touch inputs, but it is recommended you use a mouse to allow optimising for screen real-estate. You can also checkout the beta [navigation branch](https://github.com/ocornut/imgui/tree/navigation) which provides support for using Dear ImGui with a game controller.

<b>Can you create elaborate/serious tools with Dear ImGui?</b>

Yes. People have written game editors, data browsers, debuggers, profilers and all sort of non-trivial tools with the library. In my experience the simplicity of the API is very empowering. Your UI runs close to your live data. Make the tools always-on and everybody in the team will be inclined to create new tools (as opposed to more "offline" UI toolkits where only a fraction of your team effectively creates tools).

Dear ImGui is very programmer centric and the immediate-mode GUI paradigm might requires you to readjust some habits before you can realize its full potential. Dear ImGui is about making things that are simple, efficient and powerful.

<b>Is Dear ImGui fast?</b>

Probably fast enough for most uses. Down to the foundation of its visual design, Dear ImGui is engineered to be fairly performant both in term of CPU and GPU usage. Running elaborate code and creating elaborate UI will of course have a cost but Dear ImGui aims to minimize it.

Mileage may vary but the following screenshot can give you a rough idea of the cost of running and rendering UI code (In the case of a trivial demo application like this one, your driver/os setup are likely to be the bottleneck. Testing performance as part of a real application is recommended).

![performance screenshot](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v138/performance_01.png)

This is showing framerate for the full application loop on my 2011 iMac running Windows 7, OpenGL, AMD Radeon HD 6700M with an optimized executable. In contrast, librairies featuring higher-quality rendering and layouting techniques may have a higher resources footprint.

If you intend to display large lists of items (say, 1000+) it can be beneficial for your code to perform clipping manually - one way is using helpers such as ImGuiListClipper - in order to avoid submitting them to Dear ImGui in the first place. Even though ImGui will discard your clipped items it still needs to calculate their size and that overhead will add up if you have thousands of items. If you can handle clipping and height positionning yourself then browsing a list with millions of items isn't a problem.

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

Ongoing dear imgui development is financially supported on [**Patreon**](http://www.patreon.com/imgui).

Double-chocolate sponsors:
- Media Molecule
- Mobigame
- Insomniac Games (sponsored the gamepad/keyboard navigation branch)
- Aras Pranckevičius
- Lizardcube
- Greggman

Salty caramel supporters:
- Jetha Chan, Wild Sheep Studio, Pastagames, Mārtiņš Možeiko, Daniel Collin, Recognition Robotics, Chris Genova, ikrima, Glenn Fiedler, Geoffrey Evans, Dakko Dakko, Mercury Labs, Singularity Demo Group, Mischa Alff, Sebastien Ronsse.

Caramel supporters:
- Michel Courtine, César Leblic, Dale Kim, Alex Evans, Rui Figueira, Paul Patrashcu, Jerome Lanquetot, Ctrl Alt Ninja, Paul Fleming, Neil Henning, Stephan Dilly, Neil Blakey-Milner, Aleksei, NeiloGD, Justin Paver, FiniteSol, Vincent Pancaldi, James Billot, Robin Hübner, furrtek, Eric, Simon Barratt, Game Atelier, Julian Bosch, Simon Lundmark, Vincent Hamm, Farhan Wali, Jeff Roberts, Matt Reyer, Colin Riley, Victor Martins, Josh Simmons, Garrett Hoofman, Sergio Gonzales, Andrew Berridge, Roy Eltham, Game Preservation Society, Kit framework, Josh Faust, Martin Donlon, Quinton, Felix, Andrew Belt, Codecat, Cort Stratton, Claudio Canepa, Doug McNabb, Emmanuel Julien, Guillaume Chereau, Jeffrey Slutter, Jeremiah Deckard, r-lyeh, Roger Clark, Nekith, Joshua Fisher, Malte Hoffmann, Mustafa Karaalioglu, Merlyn Morgan-Graham, Per Vognsen, Fabian Giesen, Jan Staubach, Matt Hargett, John Shearer, Jesse Chounard, kingcoopa, Miloš Tošić.

And other supporters; thanks!
(Please contact me or PR if you would like to be added or removed from this list)

License
-------

Dear ImGui is licensed under the MIT License, see LICENSE for more information.
