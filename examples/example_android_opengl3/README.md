# example_android_opengl3

## Build without SDL3 using CMake

## Build with SDL3 using CMake

This example runs Dear ImGui demo windows with SDL3 on modern Android (targeting Android SDK 34).

Before running CMake, ensure the `JAVA_HOME``, `ANDROID_HOME``, and `ANDROID_NDK_HOME`` environment variables are set.

This command will configure the Android files (it doesn't actually build anything): `cmake -S . -B build .. -DCONFIGURE_WITH_SDL3=ON -DLINK_SDL_FILES=OFF`
  * Using `-DLINK_SDL_FILES=OFF` will not use symbolic links, instead will copy the SDL sources and includes into the Android JNI directory.

Once CMake has finished successfully, change into the `android` directory and run `./gradlew installDebug`. 

This should install the demo app on a connected emulator or physical device.
