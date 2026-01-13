@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.

@set OUT_EXE=example_glfw_vulkan
@set INCLUDES=/I..\.. /I..\..\backends /I..\libs\glfw\include /I %VULKAN_SDK%\include
@set SOURCES=main.cpp ..\..\backends\imgui_impl_vulkan.cpp ..\..\backends\imgui_impl_glfw.cpp ..\..\imgui*.cpp
@set LIBS=/LIBPATH:..\libs\glfw\lib-vc2010-32 /libpath:%VULKAN_SDK%\lib32 glfw3.lib opengl32.lib gdi32.lib shell32.lib vulkan-1.lib

@set OUT_DIR=Debug
mkdir %OUT_DIR%
cl /nologo /Zi /MD /utf-8 %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%

@set OUT_DIR=Release
mkdir %OUT_DIR%
cl /nologo /Zi /MD /utf-8 /Ox /Oi %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
