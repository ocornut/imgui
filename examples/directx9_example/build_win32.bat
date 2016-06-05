@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug
cl /nologo /Zi /MD /I ..\.. /I "%DXSDK_DIR%/Include" /D UNICODE /D _UNICODE *.cpp ..\..\*.cpp /FeDebug/directx9_example.exe /FoDebug/ /link /LIBPATH:"%DXSDK_DIR%/Lib/x86" d3d9.lib
