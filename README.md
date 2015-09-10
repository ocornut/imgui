ImGui
=====
[![Build Status](https://travis-ci.org/ocornut/imgui.svg?branch=master)](https://travis-ci.org/ocornut/imgui)
[![Coverity Status](https://scan.coverity.com/projects/4720/badge.svg)](https://scan.coverity.com/projects/4720)

<sub>(This library is free but I need your support to sustain development and maintenance. If you work for a company, please consider financial support)</sub>

[![Patreon](https://cloud.githubusercontent.com/assets/8225057/5990484/70413560-a9ab-11e4-8942-1a63607c0b00.png)](http://www.patreon.com/imgui) [![PayPal](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=5Q73FPZ9C526U)

ImGui is a bloat-free graphical user interface library for C++. It outputs vertex buffers that you can render in your 3D-pipeline enabled application. It is portable, renderer agnostic and self-contained (no external dependencies). It is based on an "immediate mode" graphical user interface paradigm which enables you to build user interfaces with ease.

ImGui is designed to enable fast iteration and empower programmers to create content creation tools and visualization/debug tools (as opposed to UI for the average end-user). It favors simplicity and productivity toward this goal, and thus lacks certain features normally found in more high-level libraries.

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

ImGui outputs vertex buffers and simple command-lists that you can render in your application. Because it doesn't know or touch graphics state directly, you can call ImGui commands anywhere in your code (e.g. in the middle of a running algorithm, or in the middle of your own rendering process). Refer to the sample applications in the examples/ folder for instructions on how to integrate ImGui with your existing codebase. 

ImGui allows you create elaborate tools as well as very short-lived ones. On the extreme side of short-liveness: using the Edit&Continue feature of modern compilers you can add a few widgets to tweaks variables while your application is running, and remove the code a minute later! ImGui is not just for tweaking values. You can use it to trace a running algorithm by just emitting text commands. You can use it along with your own reflection data to browse your dataset live. You can use it to expose the internals of a subsystem in your engine, to create a logger, an inspection tool, a profiler, a debugger, etc. 

Some of the features supported in some way by ImGui: window management (ordering, moving, resizing, collapsing, persistent settings), button, slider, dragging value, checkbox, radio button, text input (with selection, clipboard support, and standard keyboard controls), multiline text input, filtering text input, tree nodes, collapsing header, word-wrapping, utf-8 text, images, selectable items, vertical and horizontal scrolling, sub-window with independant scrolling/clipping, ttf font loading, basic styling, logging any ui output into text data (clipboard/tty/file), hovering, tooltips, popup windows, modal windows, menu bars, menu items, context menus, combo boxes, list box, plotting lines and histograms, resizable columns, keyboard tabbing, dragging, simple drawing api (anti-aliased, with stroking, convex fill), and low-level primitives to create custom widgets.

Demo
----

You should be able to build the examples from sources (tested on Windows/Mac/Linux). If you don't, let me know! If you want to have a quick look at the features of ImGui, you can download Windows binaries of the demo app here.
- [imgui-demo-binaries-20150909.zip](http://www.miracleworld.net/imgui/binaries/imgui-demo-binaries-20150909.zip) (Windows binaries, ImGui 1.46 WIP 2015/09/09, 4 executables, 505 KB)


Gallery
-------

![screenshot 1](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v143/test_window_01.png)
![screenshot 2](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v143/test_window_02.png)
![screenshot 3](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v143/test_window_03.png)
![screenshot 5](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v140/test_window_05_menus.png)
![screenshot 4](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v143/skinning_sample_02.png)
![screenshot 6](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v143/examples_04.png)
![screenshot 7](https://cloud.githubusercontent.com/assets/8225057/7903336/96f0fb7c-07d0-11e5-95d6-41c6a1595e5a.png)

ImGui can load TTF fonts. UTF-8 is supported for text display and input. Here using Arial Unicode font to display Japanese. Initialize custom font with:
```
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontFromFileTTF("ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
```
For Microsoft IME, pass your HWND to enable IME positioning:
```
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

Frequently Asked Question
-------------------------

<b>Where is the documentation?</b>

- The documentation is at the top of imgui.cpp + effectively imgui.h.
- Example code is in imgui_demo.cpp and particularly the ImGui::ShowTestWindow() function. It covers most features of ImGui so you can read the code and call the function itself to see its output. 
- Standalone example applications using e.g. OpenGL/DirectX are provided in the examples/ folder. 
- It obviously needs better documentation! Consider helping or becoming a [Patron](http://www.patreon.com/imgui) to promote this effort.

<b>How do you use ImGui on a platform that may not have a mouse or keyboard?</b>

I recommend using [Synergy](http://synergy-project.org) ([sources](https://github.com/synergy/synergy)). In particular, the _src/micro/uSynergy.c_ file contains a small client that you can use on any platform to connect to your host PC. You can seamlessly use your PC input devices from a video game console or a tablet. ImGui allows to increase the hit box of widgets (via the _TouchPadding_ setting) to accomodate a little for the lack of precision of touch inputs, but it is recommended you use a mouse to allow optimising for screen real-estate.

<b>I integrated ImGui in my engine and the text or lines are blurry..</b>

In your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f).

<b>Can you create elaborate/serious tools with ImGui?</b>

Yes. I have written data browsers, debuggers, profilers and all sort of non-trivial tools with the library. In my experience the simplicity of the API is very empowering. Your UI runs close to your live data. Make the tools always-on and everybody in the team will be inclined to create new tools (as opposed to more "offline" UI toolkits where only a fraction of your team effectively creates tools).

ImGui is very programmer centric and the immediate-mode GUI paradigm might requires a bit of adaptation before you can realize its full potential. Many programmers have unfortunately been taught by their environment to make unnecessarily complicated things. ImGui is about making things that are simple, efficient and powerful.

<b>Is ImGui fast?</b>

Down to the fundation of its visual design, ImGui is engineered to be fairly performant both in term of CPU and GPU usage. Running elaborate code and creating elaborate UI will of course have a cost but ImGui aims to minimize it.

Mileage may vary but the following screenshot can give you a rough idea of the cost of running and rendering UI code (In the case of a trivial demo application like this one, your driver/os setup are likely to be the bottleneck. Testing performance as part of a real application is recommended).

![performance screenshot](https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v138/performance_01.png)

This is showing framerate for the full application loop on my 2011 iMac running Windows 7, OpenGL, AMD Radeon HD 6700M with an optimized executable. In contrast, librairies featuring higher-quality rendering and layouting techniques may have a higher resources footprint.

If you intend to display large lists of items (say, 1000+) it can be beneficial for your code to perform clipping manually - using helpers such as ImGuiListClipper - in order to avoid submitting them to ImGui in the first place. Even though ImGui will discard your clipped items it still needs to calculate their size and that overhead will add up if you have thousands of items. If you can handle clipping and height positionning yourself then browsing a list with millions of items isn't a problem.

<b>Can you reskin the look of ImGui?</b>

You can alter the look of the interface to some degree: changing colors, sizes, padding, rounding, fonts. However, as ImGui is designed and optimised to create debug tools, the amount of skinning you can apply is limited. There is only so much you can stray away from the default look and feel of the interface. 

<b>Why using C++ (as opposed to C)?</b>

ImGui takes advantage of a few C++ features for convenience but nothing anywhere Boost-insanity/quagmire. In particular, function overloading and default parameters are used to make the API easier to use and code more terse. Doing so I believe the API is sitting on a sweet spot and giving up on those features would make the API more cumbersome. Other features such as namespace, constructors and templates (in the case of the ImVector<> class) are also relied on as a convenience but could be removed.

Shall someone really need to use ImGui from another language, there is an unofficial but reasonably maintained [c-api for ImGui](https://github.com/Extrawurst/cimgui) by Stephan Dilly. I would suggest using your target language functionality to try replicating the function overloading and default parameters used in C++ else the API may be harder to use. It was really designed with C++ in mind and may not make the same amount of sense with another language.

Donate
------

<b>Can I donate to support the development of ImGui?</b>

[![Patreon](https://cloud.githubusercontent.com/assets/8225057/5990484/70413560-a9ab-11e4-8942-1a63607c0b00.png)](http://www.patreon.com/imgui) [![PayPal](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=5Q73FPZ9C526U)

I'm currently an independant developer and your contributions are very meaningful to me. I have setup an [**ImGui Patreon page**](http://www.patreon.com/imgui) if you want to donate and enable me to spend more time improving the library. If your company uses ImGui please consider making a contribution. One-off donations are also greatly appreciated (PayPal link above). I am also available for hire to work on or with ImGui. Thanks!

Credits
-------

Developed by [Omar Cornut](http://www.miracleworld.net) and every direct or indirect contributors to the GitHub. The early version of this library was developed with the support of [Media Molecule](http://www.mediamolecule.com) and first used internally on the game [Tearaway](http://tearaway.mediamolecule.com). 

Embeds [ProggyClean.ttf](http://upperbounds.net) font by Tristan Grimmer (MIT license).

Embeds [stb_textedit.h, stb_truetype.h, stb_rectpack.h](https://github.com/nothings/stb/) by Sean Barrett (public domain).

Inspiration, feedback, and testing for early versions: Casey Muratori, Atman Binstock, Mikko Mononen, Emmanuel Briney, Stefan Kamoda, Anton Mikhailov, Matt Willis. And everybody posting feedback, questions and patches on the GitHub.

ImGui development is financially supported on [**Patreon**](http://www.patreon.com/imgui).

Special supporters:
- Jetha Chan, Wild Sheep Studio, Pastagames, Mārtiņš Možeiko, Daniel Collin, Stefano Cristiano.

And:
- Michel Courtine, César Leblic, Dale Kim, Alex Evans, Rui Figueira, Paul Patrashcu, Jerome Lanquetot, Ctrl Alt Ninja, Paul Fleming, Neil Henning, Stephan Dilly, Neil Blakey-Milner, Aleksei.

And other supporters; thanks!

License
-------

ImGui is licensed under the MIT License, see LICENSE for more information.
