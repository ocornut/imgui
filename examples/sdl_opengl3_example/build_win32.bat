@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I ..\.. /I ..\libs\gl3w /I %SDL2_DIR%\include main.cpp imgui_impl_sdl_gl3.cpp ..\..\imgui*.cpp ..\libs\gl3w\GL\gl3w.c /FeDebug/sdl_opengl3_example.exe /FoDebug/ /link /libpath:%SDL2_DIR%\lib\x86 SDL2.lib SDL2main.lib opengl32.lib /subsystem:console
