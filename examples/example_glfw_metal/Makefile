# You will need GLFW (http://www.glfw.org):
# Mac OS X:
#   brew install glfw

CXX = clang++

EXE = imgui_glfw_imgui_metal
SOURCES = main.mm
SOURCES += ../imgui_impl_glfw.cpp ../imgui_impl_metal.mm
SOURCES += ../../imgui.cpp ../../imgui_demo.cpp ../../imgui_draw.cpp ../../imgui_widgets.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------
# up-to-date GLFW repo
#GLFW_DIR = /usr/local	 # homebrew
GLFW_DIR = ../../../glfw

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS = -framework Metal -framework QuartzCore -framework Cocoa -framework IOKit -framework CoreVideo
	#LIBS += -L/usr/local/lib -lglfw3
	#LIBS += -L$/usr/local/lib -lglfw
	LIBS  += $(GLFW_DIR)/lib/Mac/libglfw3.a

	CXXFLAGS += -I../ -I../../ -I/usr/local/include -I$(GLFW_DIR)/include
	CXXFLAGS += -Wall -Wformat
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.mm
	$(CXX) $(CXXFLAGS) -fobjc-arc -c -o $@ $<

%.o:../%.mm
		$(CXX) $(CXXFLAGS) -fobjc-arc -c -o $@ $<

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
