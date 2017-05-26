@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I ..\.. /I ..\libs\gl3w /I %SDL_DIR%\include main.cpp imgui_impl_sdl.cpp ..\..\imgui*.cpp ..\libs\gl3w\GL\gl3w.c /FeDebug/sdl_opengl2_example.exe /FoDebug/ /link /libpath:%SDL_DIR%\lib\x86 SDL2.lib SDL2main.lib opengl32.lib /subsystem:console
