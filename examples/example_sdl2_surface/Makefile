# Makefile for example_sdl2_surface

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wformat

IMGUI_DIR = ../..
SOURCES = main.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdlsurface2.cpp

OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

UNAME_S := $(shell uname -s)

SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null)
EXEEXT :=
RM = rm -f

ifneq (,$(findstring MINGW,$(UNAME_S)))
	INCLUDES = -I../.. -I../../backends -I/mingw64/include/SDL2
	LIBS = -lmingw32 -lSDL2main -lSDL2 -lgdi32
	EXEEXT = .exe
	RM = del /Q
else
	ifeq ($(UNAME_S),Darwin)
		ifeq ($(SDL_CFLAGS),)
			SDL_CFLAGS = -F/Library/Frameworks -I/Library/Frameworks/SDL2.framework/Headers
			SDL_LIBS = -F/Library/Frameworks -framework SDL2
		endif
	endif

	INCLUDES = -I../.. -I../../backends $(SDL_CFLAGS)
	LIBS = $(SDL_LIBS)
endif

TARGET = example_sdl2_surface$(EXEEXT)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

clean:
	$(RM) *.o $(TARGET)
