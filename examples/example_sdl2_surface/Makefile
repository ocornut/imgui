# Makefile for example_sdl2_surface (MinGW)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wformat
INCLUDES = -I../.. -I../../backends -I/mingw64/include/SDL2
LIBS = -lmingw32 -lSDL2main -lSDL2 -lgdi32

IMGUI_DIR = ../..
SOURCES = main.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdlsurface2.cpp

OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

%.o:%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

all: example_sdl2_surface.exe

example_sdl2_surface.exe: $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

clean:
	del /Q *.o *.exe
