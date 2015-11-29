LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL2

IMGUI_PATH := ../../../../../../../

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	$(IMGUI_PATH)/imgui.h \
	$(IMGUI_PATH)/imgui_internal.h \
    $(IMGUI_PATH)/stb_rect_pack.h \
    $(IMGUI_PATH)/stb_textedit.h \
	$(IMGUI_PATH)/stb_truetype.h

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	Main.cpp \
	imgui_impl_sdl_gles2.cpp \
    $(IMGUI_PATH)/imgui.cpp \
	$(IMGUI_PATH)/imgui_demo.cpp \
	$(IMGUI_PATH)/imgui_draw.cpp

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)