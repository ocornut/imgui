# iOS example

----
## Introduction

This example is the default XCode "OpenGL" example code, modified to support IMGUI and [Synergy](http://synergy-project.org/). 

Synergy (remote keyboard/mouse) is not required, but it's pretty hard to use IMGUI without it. Synergy includes a "uSynergy" library that allows embedding a synergy client, this is what is used here.

## How to Use
----
0. Run the example app.
0. Tap the "servername" button in the corner
0. Enter the name or the IP of your synergy host
0. If you had previously connected to a server, you may need to kill and re-start the app.

----
## Notes and TODOs

Things that would be nice but I didn't get around to doing:

* iOS software keyboard not supported for text inputs
* iOS hardware (bluetooth) keyboards not supported
* Graceful disconnect/reconnect from uSynergy.
* Copy/Paste note well-supported

----
## C++ on iOS
IMGUI is a c++ library. If you want to include it directly, rename your Obj-C file to have the ".mm" extension. 

Alternatively, you can wrap your debug code in a C interface, this is what I am demonstrating here with the "debug_hud.h" interface. Either approach works, use whatever you prefer.

In my case, most of my game code is already in C++ so it's not really an issue and I can use IMGUI directly.
