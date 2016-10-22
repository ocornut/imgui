#
# Cross Platform Makefile
# Compatible with Ubuntu 14.04.1 and Mac OS X
#
#
# You will need GLFW (http://www.glfw.org)
#
#   apt-get install libglfw-dev  # Linux
#   brew install glfw            # Mac OS X
#

#CXX = g++

EXE = opengl3_example
SOURCES = main.cpp imgui_impl_glfw_gl3.cpp
SOURCES += ../../imgui.cpp ../../imgui_demo.cpp ../../imgui_draw.cpp
SOURCES += ../libs/gl3w/GL/gl3w.c
OBJS = $(addsuffix .o, $(basename $(SOURCES)))

UNAME_S := $(shell uname -s)


ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS = -lGL `pkg-config --static --libs glfw3`

	CXXFLAGS = -I../../ -I../libs/gl3w `pkg-config --cflags glfw3`
	CXXFLAGS += -Wall -Wformat
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -lglfw3
	#LIBS += -L/usr/local/lib -lglfw

	CXXFLAGS = -I../../ -I../libs/gl3w -I/usr/local/include
	CXXFLAGS += -Wall -Wformat
#	CXXFLAGS += -D__APPLE__
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), MINGW64_NT-6.3)
   ECHO_MESSAGE = "Windows"
   LIBS = -lglfw3 -lgdi32 -lopengl32 -limm32

   CXXFLAGS = -I../../ -I../libs/gl3w `pkg-config --cflags glfw3`
   CXXFLAGS += -Wall -Wformat
   CFLAGS = $(CXXFLAGS)
endif


.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
