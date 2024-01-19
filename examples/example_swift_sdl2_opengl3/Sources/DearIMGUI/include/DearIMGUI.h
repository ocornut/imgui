#include "../src/imgui.h"
#include "../src/imgui_impl_sdl2.h"
#include "../src/imgui_impl_opengl3.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
