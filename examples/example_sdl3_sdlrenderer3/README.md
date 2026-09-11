## How to Build the Android project

- You will need at least Java 9, preferably Java 17, to handle recent Gradle 8.9.x versions.

- You need a way to install Android SDK and NDK and add their PATHS, and have access to a virtual device or real Android device to run the app.
  - Android SDK version 21+
  - Android NDK version 19+
  
- Android Studio is an option for handling Java, Android SDK and NDK dependencies altogether: [https://developer.android.com/studio](https://developer.android.com/studio).

- The provided Makefile will download SDL from the main Git branch and create an Android project using SDL's provided `create-android-project.py` script.

- Run the Makefile using `make all`. It downloads and unpacks SDL, runs the included Python script, and generate a folder with the Android project in this directory. The project contains the Dear ImGui sources and SDL3 backends.

- After the project is generated, the Gradle version can be automatically updated in Android Studio, or updated manually via setting the `classpath` and `distributionUrl` values.

In `com.imgui.example/build.gradle`:
```
dependencies {
    classpath 'com.android.tools.build:gradle:8.7.2'
    ...
}
```

In `com.imgui.example/gradle/wrapper/gradle-wrapper.properties`:
```
distributionUrl=https\://services.gradle.org/distributions/gradle-8.9-bin.zip
```

In `com.imgui.example/app/build.grade`, update the min SDK version:
```
minSdkVersion 21
```


## How to Run

To run on a local machine using Windows PowerShell:

 1. Run `make all` and change into the generated Android project folder, for example: `cd com.imgui.example/`
 2. Run `.\gradlew.bat build`  to start the Gradle Daemons and build the project. This requires the `JAVA_HOME`, `ANDROID_HOME`, and `ANDROID_NDK_HOME` environment variables if not done in Android Studio.
     - Android Studio handles these environment variables itself
 3. Run `.\gradlew.bat installDebug` to install the APK package file onto an Android virtual device or a connected real device.


## Some other notes

On Windows, this variable may need to get set in the `com.imgui.sdlrenderer3/app/jni/SDL/Android.mk` file after variables are cleared.

```
include $(CLEAR_VARS)

# Add this if there's Makefile error 87
LOCAL_SHORT_COMMANDS := true

LOCAL_MODULE := SDL3
```
