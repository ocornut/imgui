ImGui
=====

ImGui is a bloat-free graphical user interface library for C++. It outputs vertex buffers that you can render in your 3D-pipeline enabled application. It is portable, renderer agnostic and carries minimal amount of dependencies (only 3 files are needed). It is based on an "immediate" graphical user interface paradigm which allows you to build simple user interfaces with ease.

ImGui is designed to enable fast iteration and allow programmers to create "content creation" or "debug" tools (as opposed to tools for the average end-user). It favors simplicity and thus lacks certain features normally found in more high-level libraries, such as string localisation.

ImGui is particularly suited to integration in 3D applications, fullscreen applications, embedded applications, games, or any applications on consoles platforms where operating system features are non-standard. 

After ImGui is setup in your engine, you can use it like in this example:

![screenshot of sample code alongside its output with ImGui](/web/code_sample_01.png?raw=true)

ImGui outputs vertex buffers and simple command-lists that you can render in your application. Because it doesn't know or touch graphics state directly, you can call ImGui commands anywhere in your code (e.g. in the middle of a running algorithm, or in the middle of your own rendering process). Refer to the sample applications in the examples/ folder for instructions on how to integrate ImGui with your existing codebase. 

ImGui allows you create elaborate tools as well as very short-lived ones. On the extreme side of short-liveness: using the Edit&Continue feature of compilers you can add a few widgets to tweaks variables while your application is running, and remove the code a minute later! ImGui is not just for tweaking values. You can use it to trace a running algorithm by just emitting text commands. You can use it along with your own reflection data to browse your dataset live. You can use it to expose the internals of a subsystem in your engine, to create a logger, an inspection tool, a profiler, a debugger, etc. 

Gallery
-------

![screenshot 1](/web/test_window_01.png?raw=true)
![screenshot 2](/web/test_window_02.png?raw=true)
![screenshot 3](/web/test_window_03.png?raw=true)
![screenshot 4](/web/test_window_04.png?raw=true)

References
----------

The Immediate Mode GUI paradigm may at first appear unusual to some users. This is mainly because "Retained Mode" GUIs have been so widespread and predominant. The following links can give you a better understanding about how Immediate Mode GUIs works. 
- [Johannes 'johno' Norneby's article](http://www.johno.se/book/imgui.html).
- [A presentation by Rickard Gustafsson and Johannes Algelind](http://www.cse.chalmers.se/edu/year/2011/course/TDA361/Advanced%20Computer%20Graphics/IMGUI.pdf).
- [Jari Komppa's tutorial on building an ImGui library](http://iki.fi/sol/imgui/).
- [Casey Muratori's original video that popularized the concept](https://mollyrocket.com/861).

Frequently Asked Question
-------------------------
<b>How do you use ImGui on a platform that may not have a mouse and keyboard?</b>

I recommend using [Synergy](http://synergy-project.org). With the uSynergy.c micro client running you can seamlessly use your PC input devices from a video game console or a tablet. ImGui was also designed to function with touch inputs if you increase the padding of widgets to compensate for the lack of precision of touch devices, but it is recommended you use a mouse to allow optimising for screen real-estate.

<b>I integrated ImGui in my engine and the text or lines are blurry..</b>

- Try adjusting ImGui::GetIO().PixelCenterOffset to 0.0f or 0.5f.
- In your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f).

<b>Can you create elaborate/serious tools with ImGui?</b>

Yes. I have written data browsers, debuggers, profilers and all sort of non-trivial tools with the library. There's no reason you cannot, and in my experience the simplicity of the API is very empowering. However note that ImGui is programmer centric and the immediate-mode GUI paradigm might requires a bit of adaptation before you can realize its full potential. 

<b>Can you reskin the look of ImGui?</b>

Yes, you can alter the look of the interface to some degree: changing colors, sizes and padding, font. However, as ImGui is designed and optimised to create debug tools, the amount of skinning you can apply is limited. There is only so much you can stray away from the default look and feel of the interface. The example below uses modified settings to create a more compact UI with different colors:

![skinning screenshot 1](/web/skinning_sample_01.png?raw=true)

<b>Can you develop features xxxx for ImGui?</b>

Please use GitHub 'Issues' facilities to suggest and discuss improvements. If you are company and would like specific non-trivial features to be implemented, I am available for hire to work on or with ImGui. Donations are also welcome to support further work on the library. 


Credits
-------

Developed by [Omar Cornut](http://www.miracleworld.net). The library was developed with the support of [Media Molecule](http://www.mediamolecule.com) and first used internally on the game [Tearaway](http://tearaway.mediamolecule.com). 

Embeds [proggy_clean](http://upperbounds.net) font by Tristan Grimmer (also MIT license).

Inspiration, feedback, and testing: Casey Muratori, Atman Binstock, Mikko Mononen, Emmanuel Briney, Stefan Kamoda, Anton Mikhailov, Matt Willis. Thanks!

License
-------

ImGui is licensed under the MIT License, see LICENSE for more information.
