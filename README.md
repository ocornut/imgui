ImGui
=====

ImGui is a bloat-free graphical user interface library for C++. It is portable, renderer agnostic and carries minimal amount of dependencies (only 3 files are needed). It is based on an "immediate" graphical user interface paradigm which allows you to build simple user interfaces with ease.

ImGui is designed to allow programmers to create "content creation" or "debug" tools (as opposed to tools for the average end-user). It favors simplicity and thus lacks certain features normally found in more high-level libraries, such as string localisation.

Usage example:

![screenshot of sample code alongside its output with ImGui](/web/code_sample_01.png?raw=true)

ImGui outputs vertex buffers and simple command-lists that you can render in your application. Because it doesn't know or touch graphics state directly, you can call ImGui commands anywhere in your code (e.g. in the middle of a running algorithm, or in the middle of your own rendering process). Refer to the sample applications in the examples/ folder for instructions on how to integrate ImGui with your existing codebase. 


Gallery
-------

![screenshot 1](/web/test_window_01.png?raw=true)
![screenshot 2](/web/test_window_02.png?raw=true)
![screenshot 3](/web/test_window_03.png?raw=true)
![screenshot 4](/web/test_window_04.png?raw=true)

Credits
-------

Developed by [Omar Cornut](http://www.miracleworld.net). The library was developed with the support of [Media Molecule](http://www.mediamolecule.com) and first used internally on the game [Tearaway](http://tearaway.mediamolecule.com). 

Embeds [proggy_clean](http://www.proggyfonts.net/) font by Tristan Grimmer (also MIT license).

Inspiration, feedback, and testing: Casey Muratori, Atman Binstock, Mikko Mononen, Emmanuel Briney, Stefan Kamoda, Matt Willis. Thanks!

License
-------

ImGui is licensed under the MIT License, see LICENSE for more information.
