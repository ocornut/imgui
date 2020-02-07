#
# You will need SDL2 (http://www.libsdl.org):
#   brew install sdl2
#

#CXX = g++
#CXX = clang++

EXE = example_sdl_metal
SOURCES = main.mm
SOURCES += ../imgui_impl_sdl.cpp ../imgui_impl_metal.mm
SOURCES += ../../imgui.cpp ../../imgui_widgets.cpp ../../imgui_demo.cpp ../../imgui_draw.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

LIBS = -framework Metal -framework MetalKit -framework Cocoa -framework IOKit -framework CoreVideo -framework QuartzCore
LIBS += `sdl2-config --libs`
LIBS += -L/usr/local/lib

CXXFLAGS = -I../ -I../../ -I/usr/local/include
CXXFLAGS += `sdl2-config --cflags`
CXXFLAGS += -Wall -Wformat
CFLAGS = $(CXXFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../%.mm
	$(CXX) $(CXXFLAGS) -ObjC++ -fobjc-weak -fobjc-arc -c -o $@ $<

%.o:%.mm
	$(CXX) $(CXXFLAGS) -ObjC++ -fobjc-weak -fobjc-arc -c -o $@ $<

all: $(EXE)
	@echo Build complete

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
