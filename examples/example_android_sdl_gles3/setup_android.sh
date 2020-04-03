#!/usr/bin/bash

# This script bootstraps the Dear ImGui Android+SDL2 project
#   - It pulls down the latest SDL2 from the web and then copies in the necessary files

# Learning Resources: 
# https://wiki.libsdl.org/Android
# https://hg.libsdl.org/SDL/file/default/docs/README-android.md

SDL2_LATEST='SDL2-2.0.9'

# Example commands for setting up 'Dear ImGui' Android project
wget "https://www.libsdl.org/release/$SDL2_LATEST.tar.gz"
tar xzf $SDL2_LATEST.tar.gz
cp -rn ./$SDL2_LATEST/android-project/* ./
mv -n $SDL2_LATEST app/jni/SDL
