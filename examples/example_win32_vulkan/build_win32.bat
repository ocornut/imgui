@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@set OUT_DIR=Debug
@set OUT_EXE=example_win32_vulkan
@set INCLUDES=/I..\.. /I..\..\backends /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" /I %VULKAN_SDK%\include
@set SOURCES=main.cpp ..\..\backends\imgui_impl_vulkan.cpp ..\..\backends\imgui_impl_win32.cpp ..\..\imgui*.cpp
@set LIBS=/libpath:%VULKAN_SDK%\lib32 vulkan-1.lib

mkdir %OUT_DIR%
cl /nologo /Zi /MD /utf-8 %INCLUDES% /D UNICODE /D _UNICODE %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
