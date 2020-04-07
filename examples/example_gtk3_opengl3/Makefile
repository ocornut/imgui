#
# You will need GTK+ 3.0 :
# Linux:
#   apt-get install libgtk-3-dev
#

#CXX = g++
#CXX = clang++

EXE = example_gtk3_opengl3
SOURCES = main.cpp
SOURCES += ../imgui_impl_gtk3.cpp ../imgui_impl_opengl3.cpp
SOURCES += ../../imgui.cpp ../../imgui_demo.cpp ../../imgui_draw.cpp ../../imgui_widgets.cpp
SOURCES += ../libs/gl3w/GL/gl3w.c
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

UNAME_S := $(shell uname -s)


ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS = -lGL `pkg-config --libs gtk+-3.0 epoxy` -ldl

	CXXFLAGS = -I../ -I../../ -I../libs/gl3w `pkg-config --cflags gtk+-3.0`
	CXXFLAGS += -Wall -Wformat -Wno-deprecated-declarations -Wno-parentheses
	CFLAGS = $(CXXFLAGS)
endif

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../libs/gl3w/GL/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
