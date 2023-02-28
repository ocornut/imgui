// What does this file solves?
// - Since Dear ImGui 1.00 we took pride that most of our examples applications had their entire
//   main-loop inside the main() function. That's because:
//   - It makes the examples easier to read, keeping the code sequential.
//   - It permit the use of local variables, making it easier to try things and perform quick
//     changes when someone needs to quickly test something (vs having to structure the example
//     in order to pass data around). This is very important because people use those examples
//     to craft easy-to-past repro when they want to discuss features or report issues.
//   - It conveys at a glance that this is a no-BS framework, it won't take your main loop away from you.
//   - It is generally nice and elegant.
// - However, comes Emscripten... it is a wonderful and magical tech but it requires a "main loop" function.
// - Only some of our examples would run on Emscripten. Typically the ones rendering with GL or WGPU ones.
// - I tried to refactor those examples but felt it was problematic that other examples didn't follow the
//   same layout. Why would the SDL+GL example be structured one way and the SGL+DX11 be structured differently?
//   Especially as we are trying hard to convey that using a Dear ImGui backend in an *existing application*
//   should requires only a few dozens lines of code, and this should be consistent and symmetrical for all backends.
// - So the next logical step was to refactor all examples to follow that layout of using a "main loop" function.
//   This worked, but it made us lose all the nice things we had...

// Since only about 3 examples really need to run with Emscripten, here's our solution:
// - Use some weird macros and capturing lambda to turn a loop in main() into a function.
// - Hide all that crap in this file so it doesn't make our examples unusually ugly.
//   As a stance and principle of Dear ImGui development we don't use C++ headers and we don't
//   want to suggest to the newcomer that we would ever use C++ headers as this would affect
//   the initial judgment of many of our target audience.
// - Technique is based on this idea: https://github.com/ocornut/imgui/pull/2492/
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <functional>
static std::function<void()>            MainLoopForEmscriptenP;
static void MainLoopForEmscripten()     { MainLoopForEmscriptenP(); }
#define EMSCRIPTEN_MAINLOOP_BEGIN       MainLoopForEmscriptenP = [&]()
#define EMSCRIPTEN_MAINLOOP_END         ; emscripten_set_main_loop(MainLoopForEmscripten, 0, true)
#else
#define EMSCRIPTEN_MAINLOOP_BEGIN
#define EMSCRIPTEN_MAINLOOP_END
#endif
