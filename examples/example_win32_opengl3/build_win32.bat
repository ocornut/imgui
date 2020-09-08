@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I .. /I ..\.. /I ..\libs\gl3w /I ..\libs\glext *.cpp ..\imgui_impl_win32.cpp ..\imgui_impl_opengl3.cpp ..\..\imgui*.cpp ..\libs\gl3w\GL\gl3w.c /FeDebug/example_win32_opengl3.exe /FoDebug/ /link opengl32.lib gdi32.lib 
