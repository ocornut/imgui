#
# Cross Platform Makefile
#

OBJS = main.o imgui_impl_glfw_gl3.o
OBJS += ../../imgui.o ../../imgui_demo.o ../../imgui_draw.o
OBJS += ../libs/gl3w/GL/gl3w.o

CXXFLAGS = -I../.. -I../libs/gl3w -O2 -g -Wall -Wformat

ifneq (, $(HOSTPREFIX))
    PLATFORM = $(HOSTPREFIX)
    CXX := $(HOSTPREFIX)-$(CXX)
    CC := $(HOSTPREFIX)-$(CC)
    PKG_CONFIG = $(HOSTPREFIX)-pkg-config
else
    PLATFORM = $(shell uname -s)
    PKG_CONFIG = pkg-config
endif

PLATFORM := $(shell echo $(PLATFORM) | tr '[:upper:]' '[:lower:]')

ifneq (,$(findstring darwin,$(PLATFORM))) #APPLE
    ECHO_MESSAGE = "Mac OS X"
    LIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    LIBS += -lglfw

    LIBDIRS = -L/usr/local/lib -L/opt/local/lib
    INCDIRS = -I../../ -I../libs/gl3w -I/usr/local/Cellar/glew/1.10.0/include -I/usr/local/include -I/opt/local/include
endif

ifneq (,$(findstring mingw,$(PLATFORM))) #MINGW
    ECHO_MESSAGE = "MinGW"
    EXESUFFIX = .exe
    LIBS = -lglfw3 -lopengl32 -luser32 -limm32
    LDFLAGS = -mwindows
endif

ifeq (, $(LIBS)) #UNIX GENERIC
    ECHO_MESSAGE = "Unix"
    LIBS = -ldl `$(PKG_CONFIG) --libs glfw3 gl 2>/dev/null || echo -lglfw -lgl`

    CXXFLAGS += `$(PKG_CONFIG) --cflags glfw3 gl 2>/dev/null`
endif

ifneq (, $(HOSTPREFIX))
    LIBDIRS=
    INCDIRS=
endif

CFLAGS = $(INCDIRS) $(CXXFLAGS)
CXXFLAGS := $(INCDIRS) $(CXXFLAGS)
LDFLAGS := $(LIBDIRS) $(LIBS) $(LDFLAGS)

all:imgui_example
	@echo Build complete for $(ECHO_MESSAGE)

imgui_example:$(OBJS)
	$(CXX) -o imgui_example$(EXESUFFIX) $(OBJS) $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f $(OBJS) imgui_example{,.exe}
