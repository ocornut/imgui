@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@REM Requires FreeType library. Set FREETYPE_DIR to your FreeType installation path.
@set OUT_DIR=Debug
@set OUT_EXE=example_win32_directx11_freetype
@set FREETYPE_DIR=path\to\freetype
@set INCLUDES=/I..\.. /I..\..\backends /I..\..\misc\freetype /I "%FREETYPE_DIR%\include" /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared"
@set SOURCES=main.cpp ..\..\backends\imgui_impl_dx11.cpp ..\..\backends\imgui_impl_win32.cpp ..\..\misc\freetype\imgui_freetype.cpp ..\..\imgui*.cpp
@set LIBS=/LIBPATH:"%FREETYPE_DIR%\lib" freetype.lib d3d11.lib d3dcompiler.lib
mkdir %OUT_DIR%
cl /nologo /Zi /MD /utf-8 %INCLUDES% /D UNICODE /D _UNICODE /D IMGUI_ENABLE_FREETYPE %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
