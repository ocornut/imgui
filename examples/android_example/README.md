# Android + ImGui + SDL2 + OpenGL ES 2 example
This project aims to show the ImGui layout example found in the other opengl related examples here using Android.
OpenGL ES 2 is required, if you want to try and use OpenGL ES 3 features such as Vertex Array Objects then you can uncomment them in the ```imgui_impl_sdl_gl2.cpp``` file (
Also might need to change the version requirements in the Gradle files and the AndroidManifest as well as other places in the code).

[Screenshot](http://i.imgur.com/t6nTbyX.jpg)

## Instructions
1. Download Android Studio (or Eclipse with Gradle plugin), Android SDK (tested with API 18), 
and the Android NDK (tested with NDK-r10e).
2. Export the Android SDK and NDK PATH (```export PATH=$PATH:/home/<usr>/Android/android-ndk-r10e```)
3. Open up Android Studio and using the terminal change directories to JNI (```cd android_example/app/src/main/jni```)
4. Now create a link to the SDL2 libraries on your system. For instance, on Linux ```ln -s <path-to-SDL2-root> SDL2``` . 
	<br /> Additionally you can just copy and paste the SDL2 directory into the jni directory as well.
4. Run ```ndk-build``` from the terminal.
5. Connect your Android device (emulator most likely will not work!), run the app, and select your device.
