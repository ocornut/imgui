@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I .. /I ..\.. /D UNICODE /D _UNICODE *.cpp ..\imgui_impl_dx9.cpp ..\imgui_impl_win32.cpp ..\..\imgui*.cpp /FeDebug/example_win32_directx9.exe /FoDebug/ /link d3d9.lib
