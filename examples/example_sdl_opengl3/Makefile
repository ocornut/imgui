#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need SDL2 (http://www.libsdl.org) and optionally FreeType2 (https://www.freetype.org/) for the FreeType example:
# Linux:
#   apt-get install libsdl2-dev
#   apt-get install libfreetype6-dev
# Mac OS X:
#   brew install sdl2
#   brew install freetype
# MSYS2:
#   pacman -S mingw-w64-i686-SDL
#   pacman -S mingw-w64-i686-freetype
#

#CXX = g++
#CXX = clang++

EXE = example_sdl_opengl3
EXAMPLE_SOURCES = main.cpp
EXAMPLE_SOURCES += ../imgui_impl_sdl.cpp ../imgui_impl_opengl3.cpp
SOURCES += ../../imgui.cpp ../../imgui_demo.cpp ../../imgui_draw.cpp ../../imgui_widgets.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(EXAMPLE_SOURCES) $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -I../ -I../../
CXXFLAGS += -g -Wall -Wformat
LIBS =

EXE_FREETYPE = example_sdl_opengl3_freetype
SOURCES_FREETYPE = ../../misc/freetype/imgui_freetype.cpp
OBJS_FREETYPE = $(addsuffix _freetype.o, $(basename $(notdir $(EXAMPLE_SOURCES)))) \
	$(addsuffix .o, $(basename $(notdir $(SOURCES) $(SOURCES_FREETYPE))))

##---------------------------------------------------------------------
## OPENGL LOADER
##---------------------------------------------------------------------

## Using OpenGL loader: gl3w [default]
SOURCES += ../libs/gl3w/GL/gl3w.c
CXXFLAGS += -I../libs/gl3w -DIMGUI_IMPL_OPENGL_LOADER_GL3W

## Using OpenGL loader: glew
## (This assumes a system-wide installation)
# CXXFLAGS += -lGLEW -DIMGUI_IMPL_OPENGL_LOADER_GLEW

## Using OpenGL loader: glad
# SOURCES += ../libs/glad/src/glad.c
# CXXFLAGS += -I../libs/glad/include -DIMGUI_IMPL_OPENGL_LOADER_GLAD

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL -ldl `sdl2-config --libs`

	CXXFLAGS += `sdl2-config --cflags`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
	LIBS += -L/usr/local/lib -L/opt/local/lib

	CXXFLAGS += `sdl2-config --cflags`
	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
   ECHO_MESSAGE = "MinGW"
   LIBS += -lgdi32 -lopengl32 -limm32 `pkg-config --static --libs sdl2`

   CXXFLAGS += `pkg-config --cflags sdl2`
   CFLAGS = $(CXXFLAGS)
endif

LIBS_FREETYPE = $(LIBS) `pkg-config --libs freetype2`
CXXFLAGS_FREETYPE = -DIMGUI_IMPL_FREETYPE $(CXXFLAGS) `pkg-config --cflags freetype2`
CFLAGS_FREETYPE = $(CXXFLAGS_FREETYPE)

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../libs/gl3w/GL/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o:../libs/glad/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

%_freetype.o:%.cpp
	$(CXX) $(CXXFLAGS_FREETYPE) -c -o $@ $<

%_freetype.o:../%.cpp
	$(CXX) $(CXXFLAGS_FREETYPE) -c -o $@ $<

%.o:../../misc/freetype/%.cpp
	$(CXX) $(CXXFLAGS_FREETYPE) -c -o $@ $<

$(EXE_FREETYPE): $(OBJS_FREETYPE)
	$(CXX) -o $@ $^ $(LIBS_FREETYPE)

clean:
	rm -f $(EXE) $(EXE_FREETYPE) $(OBJS) $(OBJS_FREETYPE)
