# Makefile for example_apple_metal, for macOS only (**not iOS**)
CXX = clang++
EXE = example_apple_opengl2
IMGUI_DIR = ../../
SOURCES = main.mm
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_osx.mm $(IMGUI_DIR)/backends/imgui_impl_opengl2.cpp

CXXFLAGS = -std=c++11 -ObjC++ -fobjc-arc -Wall -Wextra -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
FRAMEWORKS = -framework Cocoa -framework OpenGL -framework GameController

all: $(EXE)

$(EXE): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(EXE) $(FRAMEWORKS)

run: all
	./$(EXE)

clean:
	rm -f $(EXE) *.o
