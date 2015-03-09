#
# Cross Platform Makefile
# Compatible with Ubuntu 14.04.1 and Mac OS X
#
#
# if you using Mac OS X:
# You'll need glfw
#    http://www.glfw.org
#

#CXX = g++

OBJS = main.o imgui_impl_glfw.o
OBJS += ../../imgui.o

UNAME_S := $(shell uname -s)


ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS = `pkg-config --static --libs glfw3`

	CXXFLAGS = -I../../ `pkg-config --cflags glfw3`
	CXXFLAGS += -Wall
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib
	LIBS += -lglfw3

	CXXFLAGS = -I../../ -I/usr/local/include
	CXXFLAGS += -Wall
#	CXXFLAGS += -D__APPLE__
	CFLAGS = $(CXXFLAGS)
endif

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all:imgui_example
	@echo Build complete for $(ECHO_MESSAGE)

imgui_example:$(OBJS)
	$(CXX) -o imgui_example $(OBJS) $(CXXFLAGS) $(LIBS)

clean:
	rm $(OBJS)

