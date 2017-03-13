@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I ..\.. /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" /D UNICODE /D _UNICODE *.cpp ..\..\*.cpp /FeDebug/directx12_example.exe /FoDebug/ /link d3d12.lib d3dcompiler.lib dxgi.lib

