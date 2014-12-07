#
# Cross Platform Make file
#
# Compatible with Ubuntu 14.04.1 and Mac OS X
#
#
# if you using Mac OS X:
#  You should install glew via homebrew
#    brew install glew
#  Also you'll need glfw
#    http://www.glfw.org
#

#CXX = g++

OBJS = main.o
OBJS += ../../imgui.o

UNAME_S := $(shell uname -s)


ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	CXXFLAGS = -I../../ `pkg-config --cflags glfw3`
	CXXFLAGS += -Wall
	LIBS = `pkg-config --static --libs glfw3` -lGLEW
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"

	LIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/Cellar/glew/1.10.0/lib -L/usr/local/lib
	LIBS += -lglew -lglfw3

	CXXFLAGS = -I../../ -I/usr/local/Cellar/glew/1.10.0/include -I/usr/local/include
	CXXFLAGS += -Wall
#	CXXFLAGS += -D__APPLE__
endif

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all:imgui_example
	@echo Build complete for $(ECHO_MESSAGE)

imgui_example:$(OBJS)
	$(CXX) -o imgui_example $(OBJS) $(CXXFLAGS) $(LIBS)

clean:
	rm $(OBJS)

