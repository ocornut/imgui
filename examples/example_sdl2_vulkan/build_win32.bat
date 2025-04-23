@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.

@set OUT_EXE=example_sdl2_vulkan
@set INCLUDES=/I..\.. /I..\..\backends /I%SDL2_DIR%\include /I %VULKAN_SDK%\include
@set SOURCES=main.cpp ..\..\backends\imgui_impl_sdl2.cpp ..\..\backends\imgui_impl_vulkan.cpp ..\..\imgui*.cpp
@set LIBS=/LIBPATH:%SDL2_DIR%\lib\x86 /libpath:%VULKAN_SDK%\lib32 SDL2.lib SDL2main.lib shell32.lib vulkan-1.lib

@set OUT_DIR=Debug
mkdir %OUT_DIR%
cl /nologo /Zi /MD /utf-8 %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console

@set OUT_DIR=Release
@REM mkdir %OUT_DIR%
@REM cl /nologo /Zi /MD /utf-8 /Ox /Oi %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console
