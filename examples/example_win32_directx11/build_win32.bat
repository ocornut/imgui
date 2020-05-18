@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I .. /I ..\.. /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" /D UNICODE /D _UNICODE *.cpp ..\imgui_impl_dx11.cpp ..\imgui_impl_win32.cpp ..\..\imgui*.cpp /FeDebug/example_win32_directx11.exe /FoDebug/ /link d3d11.lib d3dcompiler.lib

