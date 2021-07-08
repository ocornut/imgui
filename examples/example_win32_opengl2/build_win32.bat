@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I .. /I ..\.. /D UNICODE /D _UNICODE *.cpp ..\imgui_impl_opengl2.cpp ..\imgui_impl_win32.cpp ..\..\imgui*.cpp /FeDebug/example_win32_opengl2.exe /FoDebug/ /link gdi32.lib opengl32.lib
