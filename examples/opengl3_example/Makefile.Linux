#
# Quick and dirty makefile to build on Linux
# tested on Ubuntu 14.04.1 32bit
#

SRC = main.cpp ../../imgui.cpp

OBJ = $(SRC:.cpp=.o)

CXXFLAGS = -I../../ `pkg-config --cflags glfw3`

LIBS = `pkg-config --static --libs glfw3` -lGLEW

all: $(OBJ)
	$(CXX) $(OBJ) $(LIBS)

clean:
	$(RM) -f $(OBJ)
