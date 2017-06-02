dear imgui,
=====
[![Build Status](https://travis-ci.org/ocornut/imgui.svg?branch=master)](https://travis-ci.org/ocornut/imgui)
[![Coverity Status](https://scan.coverity.com/projects/4720/badge.svg)](https://scan.coverity.com/projects/4720)

(This library is free but needs your support to sustain its development. There are lots of desirable new features and maintenance to do. If you are an individual using dear imgui, please consider financial support via Patreon/PayPal. If your company is using dear imgui, please consider sponsorship (e.g. sponsoring a few weeks of development). I can invoice for private support, custom development etc. E-mail: omarcornut at gmail.)

Monthly donations via Patreon:
<br>[![Patreon](https://cloud.githubusercontent.com/assets/8225057/5990484/70413560-a9ab-11e4-8942-1a63607c0b00.png)](http://www.patreon.com/imgui)

One-off donations via PayPal:
<br>[![PayPal](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=5Q73FPZ9C526U)

dear imgui (AKA ImGui), is a bloat-free graphical user interface library for C++. It outputs optimized vertex buffers that you can render anytime in your 3D-pipeline enabled application. It is fast, portable, renderer agnostic and self-contained (no external dependencies).

ImGui is designed to enable fast iteration and empower programmers to create content creation tools and visualization/ debug tools (as opposed to UI for the average end-user). It favors simplicity and productivity toward this goal, and thus lacks certain features normally found in more high-level libraries.

ImGui is particularly suited to integration in realtime 3D applications, fullscreen applications, embedded applications, games, or any applications on consoles platforms where operating system features are non-standard. 

ImGui is self-contained within a few files that you can easily copy and compile into your application/engine:

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

Your code passes mouse/keyboard inputs and settings to ImGui (see example applications for more details). After ImGui is setup, you can use it like in this example:

![screenshot of sample code alongside its output with ImGui](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/code_sample_01.png)

ImGui outputs vertex buffers and simple command-lists that you can render in your application. The number of draw calls and state changes is typically very small. Because it doesn't know or touch graphics state directly, you can call ImGui commands anywhere in your code (e.g. in the middle of a running algorithm, or in the middle of your own rendering process). Refer to the sample applications in the examples/ folder for instructions on how to integrate ImGui with your existing codebase. 

_A common misunderstanding is to think that immediate mode gui == immediate mode rendering, which usually implies hammering your driver/GPU with a bunch of inefficient draw calls and state changes, as the gui functions are called by the user. This is NOT what Dear ImGui does. Dear ImGui outputs vertex buffers and a small list of draw calls batches. It never touches your GPU directly. The draw call batches are decently optimal and you can render them later, in your app or even remotely._

ImGui allows you create elaborate tools as well as very short-lived ones. On the extreme side of short-liveness: using the Edit&Continue feature of modern compilers you can add a few widgets to tweaks variables while your application is running, and remove the code a minute later! ImGui is not just for tweaking values. You can use it to trace a running algorithm by just emitting text commands. You can use it along with your own reflection data to browse your dataset live. You can use it to expose the internals of a subsystem in your engine, to create a logger, an inspection tool, a profiler, a debugger, an entire game making editor/framework, etc.  

Binaries/Demo
-------------

You should be able to build the examples from sources (tested on Windows/Mac/Linux). If you don't, let me know! If you want to have a quick look at the features of ImGui, you can download Windows binaries of the demo app here.
- [imgui-demo-binaries-20161113.zip](http://www.miracleworld.net/imgui/binaries/imgui-demo-binaries-20161113.zip) (Windows binaries, ImGui 1.49+ 2016/11/13, 5 executables, 588 KB)

Bindings
--------

_NB: those third-party bindings may be more or less maintained, more or less close to the spirit of original API and therefore I cannot give much guarantee about them. People who create language bindings sometimes haven't used the C++ API themselves (for the good reason that they aren't C++ users). ImGui was designed with C++ in mind and some of the subtleties may be lost in translation with other languages. If your language supports it, I would suggest replicating the function overloading and default parameters used in the original, else the API may be harder to use. In doubt, please check the original C++ version first!_

_Integrating Dear ImGui within your custom engine is a matter of wiring mouse/keyboard inputs and providing a render function that can bind a texture and render simple textured triangles. The examples/ folder is populated with applications doing just that. If you are an experienced programmer it should take you less than an hour to integrate Dear ImGui in your custom engine, but make sure to spend time reading the FAQ, the comments and other documentation!_

Languages:
- cimgui: thin c-api wrapper for ImGui https://github.com/Extrawurst/cimgui
- ImGui.NET: An ImGui wrapper for .NET Core https://github.com/mellinoe/ImGui.NET
- imgui-rs: Rust bindings for dear imgui https://github.com/Gekkio/imgui-rs
- DerelictImgui: Dynamic bindings for the D programming language: https://github.com/Extrawurst/DerelictImgui
- CyImGui: Python bindings for dear imgui using Cython: https://github.com/chromy/cyimgui
- pyimgui: Another Python bindings for dear imgui: https://github.com/swistakm/pyimgui
- LUA: https://github.com/patrickriordan/imgui_lua_bindings
- imgui-pas: P  ascal bindings for imgui https://github.com/dpethes/imgui-pas

Frameworks:
- Main ImGui repository include examples for DirectX9, DirectX10, DirectX11, OpenGL2/3, Vulkan, Allegro 5, SDL+GL2/3, iOS and Marmalade: https://github.com/ocornut/imgui/tree/master/examples
- Unmerged PR: DirectX12 example (with issues) https://github.com/ocornut/imgui/pull/301
- Unmerged PR: SDL2 + OpenGLES + Emscripten example https://github.com/ocornut/imgui/pull/336
- Unmerged PR: FreeGlut + OpenGL2 example https://github.com/ocornut/imgui/pull/801
- Unmerged PR: Native Win32 and OSX example https://github.com/ocornut/imgui/pull/281
- Unmerged PR: Android Example https://github.com/ocornut/imgui/pull/421
- Cinder backend for dear imgui https://github.com/simongeilfus/Cinder-ImGui
- FlexGUI: Flexium/SFML backend for dear imgui https://github.com/DXsmiley/FlexGUI
- IrrIMGUI: Irrlicht backend for dear imgui https://github.com/ZahlGraf/IrrIMGUI
- UnrealEngine_ImGui: Unreal Engine 4 backend for dear imgui https://github.com/sronsse/UnrealEngine_ImGui
- LÖVE backend for dear imgui https://github.com/slages/love-imgui
- Ogre backend for dear imgui https://bitbucket.org/LMCrashy/ogreimgui/src
- ofxImGui: openFrameworks backend for dear imgui https://github.com/jvcleave/ofxImGui
- SFML backend for dear imgui https://github.com/EliasD/imgui-sfml
- SFML backend for dear imgui https://github.com/Mischa-Alff/imgui-backends
- cocos2d-x with imgui https://github.com/c0i/imguix https://github.com/ocornut/imgui/issues/551
- NanoRT: software raytraced version https://github.com/syoyo/imgui/tree/nanort/examples/raytrace_example

For other bindings: see [this page](https://github.com/ocornut/imgui/wiki/Links/).
Please contact me with the Issues tracker or Twitter to fix/update this list.

Gallery
-------

See the [Screenshots Thread](https://github.com/ocornut/imgui/issues/123) for some user creations.

![screenshot 1](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v148/examples_01.png)
[![screenshot game](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v149/gallery_TheDragonsTrap-01-thumb.jpg)](https://cloud.githubusercontent.com/assets/8225057/20628927/33e14cac-b329-11e6-80f6-9524e93b048a.png)
![screenshot 2](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v148/examples_02.png)

[![screenshot profiler](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v148/profiler-880.jpg)](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v148/profiler.png)

![screenshot 3](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v143/test_window_01.png)
![screenshot 4](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v143/test_window_03.png)
![screenshot 5](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v140/test_window_05_menus.png)
![screenshot 6](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v143/skinning_sample_02.png)
![screenshot 7](https://cloud.githubusercontent.com/assets/8225057/7903336/96f0fb7c-07d0-11e5-95d6-41c6a1595e5a.png)

ImGui can load TTF fonts. UTF-8 is supported for text display and input. Here using Arial Unicode font to display Japanese. Initialize custom font with:
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

See the [Links page](https://github.com/ocornut/imgui/wiki/Links) for third-party bindings to different languages and frameworks.

Frequently Asked Question (FAQ)
-------------------------------

<b>Where is the documentation?</b>

- The documentation is at the top of imgui.cpp + effectively imgui.h. 
- Example code is in imgui_demo.cpp and particularly the ImGui::ShowTestWindow() function. It covers most features of ImGui so you can read the code and call the function itself to see its output. 
- Standalone example applications using e.g. OpenGL/DirectX are provided in the examples/ folder. 
- We obviously needs better documentation! Consider contributing or becoming a [Patron](http://www.patreon.com/imgui) to promote this effort.

<b>Why the odd dual naming, "dear imgui" vs "ImGui"?</b>

The library started its life and is best known as "ImGui" only due to the fact that I didn't give it a proper name when I released it. However, the term IMGUI (immediate-mode graphical user interface) was coined before and is being used in variety of other situations. It seemed confusing and unfair to hog the name. To reduce the ambiguity without affecting existing codebases, I have decided on an alternate, longer name "dear imgui" that people can use to refer to this specific library in ambiguous situations.

<b>How do I update to a newer version of ImGui?</b>
<br><b>What is ImTextureID and how do I display an image?</b>
<br><b>I integrated ImGui in my engine and the text or lines are blurry..</b>
<br><b>I integrated ImGui in my engine and some elements are disappearing when I move windows around..</b>
<br><b>How can I have multiple widgets with the same label? Can I have widget without a label? (Yes). A primer on the purpose of labels/IDs.</b>
<br><b>How can I tell when ImGui wants my mouse/keyboard inputs and when I can pass them to my application?</b>
<br><b>How can I load a different font than the default?</b>
<br><b>How can I easily use icons in my application?</b>
<br><b>How can I load multiple fonts?</b>
<br><b>How can I display and input non-latin characters such as Chinese, Japanese, Korean, Cyrillic?</b>
<br><b>How can I use the drawing facilities without an ImGui window? (using ImDrawList API)</b>

See the FAQ in imgui.cpp for answers.

<b>How do you use ImGui on a platform that may not have a mouse or keyboard?</b>

I recommend using [Synergy](http://synergy-project.org) ([sources](https://github.com/symless/synergy)). In particular, the _src/micro/uSynergy.c_ file contains a small client that you can use on any platform to connect to your host PC. You can seamlessly use your PC input devices from a video game console or a tablet. ImGui allows to increase the hit box of widgets (via the _TouchPadding_ setting) to accommodate a little for the lack of precision of touch inputs, but it is recommended you use a mouse to allow optimising for screen real-estate.

<b>Can you create elaborate/serious tools with ImGui?</b>

Yes. I have written data browsers, debuggers, profilers and all sort of non-trivial tools with the library. In my experience the simplicity of the API is very empowering. Your UI runs close to your live data. Make the tools always-on and everybody in the team will be inclined to create new tools (as opposed to more "offline" UI toolkits where only a fraction of your team effectively creates tools).

ImGui is very programmer centric and the immediate-mode GUI paradigm might requires you to readjust some habits before you can realize its full potential. Many programmers have unfortunately been taught by their environment to make unnecessarily complicated things. ImGui is about making things that are simple, efficient and powerful.

<b>Is ImGui fast?</b>

Probably fast enough for most uses. Down to the foundation of its visual design, ImGui is engineered to be fairly performant both in term of CPU and GPU usage. Running elaborate code and creating elaborate UI will of course have a cost but ImGui aims to minimize it.

Mileage may vary but the following screenshot can give you a rough idea of the cost of running and rendering UI code (In the case of a trivial demo application like this one, your driver/os setup are likely to be the bottleneck. Testing performance as part of a real application is recommended).

![performance screenshot](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v138/performance_01.png)

This is showing framerate for the full application loop on my 2011 iMac running Windows 7, OpenGL, AMD Radeon HD 6700M with an optimized executable. In contrast, librairies featuring higher-quality rendering and layouting techniques may have a higher resources footprint.

If you intend to display large lists of items (say, 1000+) it can be beneficial for your code to perform clipping manually - one way is using helpers such as ImGuiListClipper - in order to avoid submitting them to ImGui in the first place. Even though ImGui will discard your clipped items it still needs to calculate their size and that overhead will add up if you have thousands of items. If you can handle clipping and height positionning yourself then browsing a list with millions of items isn't a problem.

<b>Can you reskin the look of ImGui?</b>

You can alter the look of the interface to some degree: changing colors, sizes, padding, rounding, fonts. However, as ImGui is designed and optimised to create debug tools, the amount of skinning you can apply is limited. There is only so much you can stray away from the default look and feel of the interface. 

This is [LumixEngine](https://github.com/nem0/LumixEngine) with a minor skinning hack + a docking/tabs extension (both of which you can find in the Issues section and will eventually be merged).

[![Skinning in LumixEngine](https://cloud.githubusercontent.com/assets/8225057/13198792/92808c5c-d812-11e5-9507-16b63918b05b.jpg)](https://cloud.githubusercontent.com/assets/8225057/13044612/59f07aec-d3cf-11e5-8ccb-39adf2e13e69.png)

<b>Why using C++ (as opposed to C)?</b>

ImGui takes advantage of a few C++ languages features for convenience but nothing anywhere Boost-insanity/quagmire. ImGui doesn't use any C++ header file. Language-wise, function overloading and default parameters are used to make the API easier to use and code more terse. Doing so I believe the API is sitting on a sweet spot and giving up on those features would make the API more cumbersome. Other features such as namespace, constructors and templates (in the case of the ImVector<> class) are also relied on as a convenience.

There is an unofficial but reasonably maintained [c-api for ImGui](https://github.com/Extrawurst/cimgui) by Stephan Dilly. I would suggest using your target language functionality to try replicating the function overloading and default parameters used in C++ else the API may be harder to use. It was really designed with C++ in mind and may not make the same amount of sense with another language. Also see [Links](https://github.com/ocornut/imgui/wiki/Links) for third-party bindings to other languages.

Donate
------

<b>How can I help financing further development of Dear ImGui?</b>

Monthly donations via Patreon:
<br>[![Patreon](https://cloud.githubusercontent.com/assets/8225057/5990484/70413560-a9ab-11e4-8942-1a63607c0b00.png)](http://www.patreon.com/imgui)

One-off donations via PayPal:
<br>[![PayPal](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=5Q73FPZ9C526U)

Your contributions are keeping the library alive. For end-users, I have setup an [**ImGui Patreon page**](http://www.patreon.com/imgui) if you want to donate and enable me to spend more time improving the library. If your company uses ImGui please consider making a contribution. One-off donations are also greatly appreciated. I can invoice for private support, custom development or whatever makes more sense in a given context. I am available for hire to work on or with ImGui. Please e-mail omarcornut at gmail for details. Thanks!

Credits
-------

Developed by [Omar Cornut](http://www.miracleworld.net) and every direct or indirect contributors to the GitHub. The early version of this library was developed with the support of [Media Molecule](http://www.mediamolecule.com) and first used internally on the game [Tearaway](http://tearaway.mediamolecule.com). 

I first discovered imgui principles at [Q-Games](http://www.q-games.com) where Atman had dropped his own simple imgui implementation in the codebase, which I spent quite some time improving and thinking about. It turned out that Atman was exposed to the concept directly by working with Casey. When I moved to Media Molecule I rewrote a new library trying to overcome the flaws and limitations of the first one I've worked with. It became this library and since then I have spent an unreasonable amount of time iterating on it. 

Embeds [ProggyClean.ttf](http://upperbounds.net) font by Tristan Grimmer (MIT license).

Embeds [stb_textedit.h, stb_truetype.h, stb_rectpack.h](https://github.com/nothings/stb/) by Sean Barrett (public domain).

Inspiration, feedback, and testing for early versions: Casey Muratori, Atman Binstock, Mikko Mononen, Emmanuel Briney, Stefan Kamoda, Anton Mikhailov, Matt Willis. And everybody posting feedback, questions and patches on the GitHub.

Ongoing ImGui development is financially supported on [**Patreon**](http://www.patreon.com/imgui).

Double-chocolate sponsors:
- Media Molecule
- Mobigame
- Insomniac Games (sponsored the gamepad/keyboard navigation branch)
- Aras Pranckevičius

Salty caramel supporters:
- Jetha Chan, Wild Sheep Studio, Pastagames, Mārtiņš Možeiko, Daniel Collin, Recognition Robotics, Chris Genova, ikrima, Glenn Fiedler, Geoffrey Evans, Dakko Dakko.

Caramel supporters:
- Michel Courtine, César Leblic, Dale Kim, Alex Evans, Rui Figueira, Paul Patrashcu, Jerome Lanquetot, Ctrl Alt Ninja, Paul Fleming, Neil Henning, Stephan Dilly, Neil Blakey-Milner, Aleksei, NeiloGD, Justin Paver, FiniteSol, Vincent Pancaldi, James Billot, Robin Hübner, furrtek, Eric, Simon Barratt, Game Atelier, Julian Bosch, Simon Lundmark, Vincent Hamm, Farhan Wali, Jeff Roberts, Matt Reyer, Colin Riley, Victor Martins, Josh Simmons, Garrett Hoofman, Sergio Gonzales, Andrew Berridge, Roy Eltham, Game Preservation Society, [Kit framework](http://svkonsult.se/kit), Josh Faust, Martin Donlon, Quinton, Felix.

And other supporters; thanks!

License
-------

Dear ImGui is licensed under the MIT License, see LICENSE for more information.
