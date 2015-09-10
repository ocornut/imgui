@REM Build for Visual Studio compiler
mkdir Debug
cl /nologo /MD /I ..\.. /I "%DXSDK_DIR%/Include" /D UNICODE *.cpp ..\..\*.cpp /FeDebug/directx11_example.exe /FoDebug/ /link /LIBPATH:"%DXSDK_DIR%/Lib/x86" d3d11.lib d3dcompiler.lib

