@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
set OUT_DIR=Debug
set OUT_EXE=example_win32_opengl3
set INCLUDES=/I.. /I..\.. /I..\libs\gl3w
set SOURCES=main.cpp ..\imgui_impl_win32.cpp ..\imgui_impl_opengl3.cpp ..\..\imgui*.cpp ..\libs\gl3w\GL\gl3w.c
set LIBS= opengl32.lib
mkdir %OUT_DIR%
cl /nologo /Zi /MD %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console