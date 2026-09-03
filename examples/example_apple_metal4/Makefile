# Makefile for example_apple_metal4, for macOS only (**not iOS**)
CXX = clang++
EXE = example_apple_metal4
IMGUI_DIR = ../../
SOURCES = main.mm
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_osx.mm $(IMGUI_DIR)/backends/imgui_impl_metal4.mm

CXXFLAGS = -std=c++11 -ObjC++ -fobjc-arc -Wall -Wextra -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
FRAMEWORKS = -framework AppKit -framework Metal -framework MetalKit -framework QuartzCore -framework GameController

all: $(EXE)

$(EXE): $(SOURCES)
	$(CXX) $(CXXFLAGS) $^ $(FRAMEWORKS) -o $@

run: all
	./$(EXE)

clean:
	rm -f $(EXE) *.o
