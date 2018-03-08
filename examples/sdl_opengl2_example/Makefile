#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need SDL2 (http://www.libsdl.org):
# Linux:
#   apt-get install libsdl2-dev
# Mac OS X:
#   brew install sdl2
# MSYS2:
#   pacman -S mingw-w64-i686-SDL
#

#CXX = g++
#CXX = clang++

EXE = sdl_opengl2_example
SOURCES = main.cpp imgui_impl_sdl_gl2.cpp
SOURCES += ../../imgui.cpp ../../imgui_demo.cpp ../../imgui_draw.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

UNAME_S := $(shell uname -s)


ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS = -lGL -ldl `sdl2-config --libs`

	CXXFLAGS = -I../../  `sdl2-config --cflags`
	CXXFLAGS += -Wall -Wformat
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`

	CXXFLAGS = -I../../ -I/usr/local/include `sdl2-config --cflags`
	CXXFLAGS += -Wall -Wformat
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
   ECHO_MESSAGE = "Windows"
   LIBS = -lgdi32 -lopengl32 -limm32 `pkg-config --static --libs sdl2`

   CXXFLAGS = -I../../ `pkg-config --cflags sdl2`
   CXXFLAGS += -Wall -Wformat
   CFLAGS = $(CXXFLAGS)
endif


%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
