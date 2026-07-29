ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

NAME=example_qnx_opengl3

SRCS = \
    main.cpp \
    imgui.cpp \
    imgui_demo.cpp \
    imgui_draw.cpp \
    imgui_tables.cpp \
    imgui_widgets.cpp \
    imgui_impl_qnx.cpp \
    imgui_impl_opengl3.cpp

EXTRA_SRCVPATH += $(PROJECT_ROOT)/../.. $(PROJECT_ROOT)/../../backends
EXTRA_INCVPATH += $(PROJECT_ROOT)/../.. $(PROJECT_ROOT)/../../backends

CXXFLAGS += -std=gnu++11 -DIMGUI_IMPL_OPENGL_ES3
LIBS += EGL GLESv2 screen m

INSTALLDIR=$(firstword $(INSTALLDIR_$(OS)) usr/bin)

define PINFO
PINFO DESCRIPTION=Dear ImGui QNX Screen and OpenGL ES 3 example
endef

include $(MKFILES_ROOT)/qmacros.mk
include $(MKFILES_ROOT)/qtargets.mk
