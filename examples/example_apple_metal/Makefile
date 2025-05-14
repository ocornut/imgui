# Makefile for example_apple_metal, for macOS only (**not iOS**)
TARGET      := example_apple_metal
CXX         := clang++
CXXFLAGS    := -std=c++17 -ObjC++ -fobjc-arc -Wall -Wextra \
               -I../../                             \
               -I../../backends
FRAMEWORKS  := -framework AppKit -framework Metal -framework MetalKit -framework QuartzCore -framework GameController
IMGUI_SRC   := ../../imgui.cpp ../../imgui_draw.cpp ../../imgui_demo.cpp \
               ../../imgui_tables.cpp ../../imgui_widgets.cpp
BACKENDS    := ../../backends/imgui_impl_metal.mm ../../backends/imgui_impl_osx.mm
SRC         := main.mm $(IMGUI_SRC) $(BACKENDS)

.PHONY: all run clean


all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $^ $(FRAMEWORKS) -o $@

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o
