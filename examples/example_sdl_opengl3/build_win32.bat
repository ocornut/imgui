@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I .. /I ..\.. /I ..\libs\gl3w /I %SDL2_DIR%\include *.cpp ..\imgui_impl_opengl3.cpp ..\imgui_impl_sdl.cpp ..\..\imgui*.cpp ..\libs\gl3w\GL\gl3w.c /FeDebug/example_sdl_opengl3.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x86 SDL2.lib SDL2main.lib opengl32.lib /subsystem:console
