
# Uncomment this if you're using STL in your project
# See CPLUSPLUS-SUPPORT.html in the NDK documentation for more information
# stlport_static
APP_STL := gnustl_static

APP_ABI := armeabi armeabi-v7a
APP_PLATFORM := android-10
APP_CPPFLAGS := -std=c++11 -fexceptions -Wall -Wformat