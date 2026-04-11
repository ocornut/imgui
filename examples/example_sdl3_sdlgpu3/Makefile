#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need SDL3 (http://www.libsdl.org) which is still unreleased/unpackaged.

#CXX = g++
CXX = clang++

EXE = example_sdl3_sdlgpu3
IMGUI_DIR = ../..
TE_DIR = ../../../imgui_test_engine/imgui_test_engine
SOURCES = main.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl3.cpp $(IMGUI_DIR)/backends/imgui_impl_sdlgpu3.cpp
SOURCES += $(IMGUI_DIR)/misc/freetype/imgui_freetype.cpp
SOURCES += $(TE_DIR)/imgui_te_context.cpp
SOURCES += $(TE_DIR)/imgui_te_coroutine.cpp
SOURCES += $(TE_DIR)/imgui_te_engine.cpp
SOURCES += $(TE_DIR)/imgui_te_exporters.cpp
SOURCES += $(TE_DIR)/imgui_te_perftool.cpp
SOURCES += $(TE_DIR)/imgui_te_ui.cpp
SOURCES += $(TE_DIR)/imgui_te_utils.cpp
SOURCES += $(TE_DIR)/imgui_capture_tool.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -std=c++11 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(TE_DIR)
CXXFLAGS += -g -Wall -Wformat
LIBS =


##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -ldl `pkg-config sdl3 --libs` `pkg-config --libs freetype2` `pkg-config --libs plutosvg`

	CXXFLAGS += `pkg-config sdl3 --cflags` `pkg-config --cflags freetype2` `pkg-config --cflags plutosvg`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework Cocoa -framework IOKit -framework CoreVideo `pkg-config --libs sdl3`
	LIBS += -L/usr/local/lib -L/opt/local/lib

	CXXFLAGS += `pkg-config sdl3 --cflags`
	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lgdi32 -limm32 `pkg-config --static --libs sdl3`

	CXXFLAGS += `pkg-config --cflags sdl3`
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/misc/freetype/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(TE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
