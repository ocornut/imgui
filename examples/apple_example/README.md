# iOS / OSX example

## Introduction

This example is the default XCode "OpenGL" example code, modified to support ImGui and [Synergy](http://synergy-project.org/) to share mouse/keyboard on an iOS device.

It is a rather complex and messy example because of all of the faff required to get an XCode/iOS application running. Refer to the regular OpenGL examples if you want to learn about integrating ImGui. **The opengl3_example/ should also work on OS X and is much simpler.** This is an integration for iOS with Synergy.

Synergy (remote keyboard/mouse) is not required, but it's pretty hard to use ImGui without it. Synergy includes a "uSynergy" library that allows embedding a synergy client, this is what is used here. ImGui supports "TouchPadding", and this is enabled when Synergy is not active.

## How to Use on iOS

* In Synergy, go to Preferences, and uncheck "Use SSL encryption"
* Run the example app.
* Tap the "servername" button in the corner
* Enter the name or the IP of your synergy host
* If you had previously connected to a server, you may need to kill and re-start the app.

## How to Build on OSX

* Make sure you have install `brew`, if not, please refer to [Homebrew Website](http://brew.sh)
* Run the command: `brew install glfw3`
* Double click `imguiex.xcodeproj` and select `imguiex-osx` scheme
* Click `Run` button

## Notes and TODOs

Things that would be nice but I didn't get around to doing:

* iOS software keyboard not supported for text inputs
* iOS hardware (bluetooth) keyboards not supported
* Graceful disconnect/reconnect from uSynergy.
* Copy/Paste not well-supported

## C++ on iOS / OSX

ImGui is a c++ library. If you want to include it directly, rename your Obj-C file to have the ".mm" extension. 

Alternatively, you can wrap your debug code in a C interface, this is what I am demonstrating here with the "debug_hud.h" interface. Either approach works, use whatever you prefer.

In my case, most of my game code is already in C++ so it's not really an issue and I can use ImGui directly.
