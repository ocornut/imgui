#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#

EXE = example_null
SOURCES = main.cpp
SOURCES += ../../imgui.cpp ../../imgui_demo.cpp ../../imgui_draw.cpp ../../imgui_widgets.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)
EXTRA_WARNINGS ?= 0

CXXFLAGS = -I../ -I../../
CXXFLAGS += -g -Wall -Wformat
LIBS =

# We use the EXTRA_WARNINGS flag on our CI setup to eagerly catch zealous warnings
ifeq ($(EXTRA_WARNINGS), 1)
	CXXFLAGS += -Wno-zero-as-null-pointer-constant -Wno-double-promotion -Wno-variadic-macros
endif

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	ifneq ($(EXTRA_WARNINGS), 0)
		CXXFLAGS += -Wextra -pedantic
	endif
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	ifneq ($(EXTRA_WARNINGS), 0)
		CXXFLAGS += -Weverything -Wno-reserved-id-macro -Wno-c++98-compat-pedantic -Wno-padded -Wno-c++11-long-long
	endif
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
	ECHO_MESSAGE = "MinGW"
	ifneq ($(EXTRA_WARNINGS), 0)
		CXXFLAGS += -Wextra -pedantic
	endif
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../libs/gl3w/GL/%.c
# %.o:../libs/glad/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
