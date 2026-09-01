@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@set OUT_DIR=Debug
@set OUT_EXE=example_sdl3_directx11
@set INCLUDES=/I..\.. /I..\..\backends /I%SDL3_DIR%\include /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" /I "%DXSDK_DIR%Include"
@set SOURCES=main.cpp ..\..\backends\imgui_impl_sdl3.cpp ..\..\backends\imgui_impl_dx11.cpp ..\..\imgui*.cpp
@set LIBS=/LIBPATH:%SDL3_DIR%\lib\x86 SDL3.lib /LIBPATH:"%DXSDK_DIR%/Lib/x86" d3d11.lib d3dcompiler.lib shell32.lib
mkdir %OUT_DIR%
cl /nologo /Zi /MD /utf-8 %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console
