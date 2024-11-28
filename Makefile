PREFIX ?= /usr
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

CXX = g++
CXXFLAGS = -std=c++11 -fPIC
SOURCES = imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = libimgui.so

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -shared -o $@ $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

install: $(TARGET)
	mkdir -p $(LIBDIR) $(INCLUDEDIR)
	cp $(TARGET) $(LIBDIR)/$(TARGET)
	chmod 644 $(LIBDIR)/$(TARGET)
	cp imgui.h imconfig.h imgui_internal.h $(INCLUDEDIR)

uninstall:
	rm -f $(LIBDIR)/$(TARGET) $(LIBDIR)/libimgui.so
	rm -f $(INCLUDEDIR)/imgui.h $(INCLUDEDIR)/imconfig.h $(INCLUDEDIR)/imgui_internal.h

clean:
	rm -f $(OBJECTS) $(TARGET)
