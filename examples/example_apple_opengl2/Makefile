IMGUI_DIR = ../../
CXX = clang++
CXXFLAGS = -std=c++17 -ObjC++ -Wall -Wextra -g -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
FRAMEWORKS = -framework Cocoa -framework OpenGL -framework GameController

SOURCES = \
    main.mm \
    $(IMGUI_DIR)/backends/imgui_impl_osx.mm \
    $(IMGUI_DIR)/backends/imgui_impl_opengl2.cpp \
    $(IMGUI_DIR)/imgui.cpp \
    $(IMGUI_DIR)/imgui_draw.cpp \
    $(IMGUI_DIR)/imgui_demo.cpp \
    $(IMGUI_DIR)/imgui_tables.cpp \
    $(IMGUI_DIR)/imgui_widgets.cpp

TARGET = imgui_example_apple_opengl2

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o imgui_example_apple_opengl2 $(FRAMEWORKS)

clean:
	rm -f $(TARGET)
	rm -rf $(TARGET).dSYM
