ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

NAME=example_qnx_vulkan

SRCS = \
    main.cpp \
    imgui.cpp \
    imgui_demo.cpp \
    imgui_draw.cpp \
    imgui_tables.cpp \
    imgui_widgets.cpp \
    imgui_impl_qnx.cpp \
    imgui_impl_vulkan.cpp

EXTRA_SRCVPATH += $(PROJECT_ROOT)/../.. $(PROJECT_ROOT)/../../backends
EXTRA_INCVPATH += $(PROJECT_ROOT)/../.. $(PROJECT_ROOT)/../../backends

CXXFLAGS += -std=gnu++11 -DVK_USE_PLATFORM_SCREEN_QNX=1
LIBS += vulkan screen m

INSTALLDIR=$(firstword $(INSTALLDIR_$(OS)) usr/bin)

define PINFO
PINFO DESCRIPTION=Dear ImGui QNX Screen and Vulkan example
endef

include $(MKFILES_ROOT)/qmacros.mk
include $(MKFILES_ROOT)/qtargets.mk
