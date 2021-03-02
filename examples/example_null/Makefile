#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# Important: This is a "null backend" application, with no visible output or interaction!
# This is used for testing purpose and continuous integration, and has little use for end-user.
#

# Options
WITH_EXTRA_WARNINGS ?= 0
WITH_FREETYPE ?= 0

EXE = example_null
IMGUI_DIR = ../..
SOURCES = main.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS += -I$(IMGUI_DIR)
CXXFLAGS += -g -Wall -Wformat
LIBS =

# We use the WITH_EXTRA_WARNINGS flag on our CI setup to eagerly catch zealous warnings
ifeq ($(WITH_EXTRA_WARNINGS), 1)
	CXXFLAGS += -Wno-zero-as-null-pointer-constant -Wno-double-promotion -Wno-variadic-macros
endif

# We use the WITH_FREETYPE flag on our CI setup to test compiling misc/freetype/imgui_freetype.cpp
# (only supported on Linux, and note that the imgui_freetype code currently won't be executed)
ifeq ($(WITH_FREETYPE), 1)
	SOURCES += $(IMGUI_DIR)/misc/freetype/imgui_freetype.cpp
	CXXFLAGS += $(shell pkg-config --cflags freetype2)
	LIBS += $(shell pkg-config --libs freetype2)
endif

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	ifneq ($(WITH_EXTRA_WARNINGS), 0)
		CXXFLAGS += -Wextra -Wpedantic
		ifeq ($(shell $(CXX) -v 2>&1 | grep -c "clang version"), 1)
			CXXFLAGS += -Wshadow -Wsign-conversion
		endif
	endif
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	ifneq ($(WITH_EXTRA_WARNINGS), 0)
		CXXFLAGS += -Weverything -Wno-reserved-id-macro -Wno-c++98-compat-pedantic -Wno-padded -Wno-c++11-long-long
	endif
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	ifneq ($(WITH_EXTRA_WARNINGS), 0)
		CXXFLAGS += -Wextra -Wpedantic
	endif
	LIBS += -limm32
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/misc/freetype/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
