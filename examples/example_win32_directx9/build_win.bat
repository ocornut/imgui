@REM Build for Visual Studio compiler. First run vcvars32.bat (x86) or vcvars64.bat (x64).

@REM Detect target architecture (VS 2015 uses Platform, unset for native x86; VS 2017+ uses VSCMD_ARG_TGT_ARCH).
@set TARGET_ARCH=%VSCMD_ARG_TGT_ARCH%
@if not defined TARGET_ARCH if "%VisualStudioVersion%"=="14.0" (set TARGET_ARCH=x86& if defined Platform set TARGET_ARCH=%Platform%)
@if /I "%TARGET_ARCH%"=="x86" goto arch_ok
@if /I "%TARGET_ARCH%"=="x64" goto arch_ok
@echo Unsupported or uninitialized compiler environment. First run vcvars32.bat (x86) or vcvars64.bat (x64).
@exit /b 1

:arch_ok
@set OUT_EXE=example_win32_directx9
@set INCLUDES=/I..\.. /I..\..\backends /I "%DXSDK_DIR%/Include"
@set SOURCES=main.cpp ..\..\backends\imgui_impl_dx9.cpp ..\..\backends\imgui_impl_win32.cpp ..\..\imgui*.cpp
@set LIBS=d3d9.lib
@if /I "%TARGET_ARCH%"=="x86" set LIBS=%LIBS% /LIBPATH:"%DXSDK_DIR%/Lib/x86"
@if /I "%TARGET_ARCH%"=="x64" set LIBS=%LIBS% /LIBPATH:"%DXSDK_DIR%/Lib/x64"

@set OUT_DIR=Debug
mkdir %OUT_DIR%
cl /nologo /Zi /MD /utf-8 %INCLUDES% /D UNICODE /D _UNICODE %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
